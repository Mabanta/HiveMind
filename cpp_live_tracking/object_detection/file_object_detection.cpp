#include "../cluster/cluster.hpp"
#include "constants.hpp"

#define LIBCAER_FRAMECPP_OPENCV_INSTALLED 0

#include <dv-processing/core/core.hpp>
#include <libcaercpp/devices/dvxplorer.hpp>
#include <libcaercpp/devices/device.hpp>
#include <dv-processing/io/mono_camera_recording.hpp>

#include <iostream>
#include <fstream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <csignal>
#include <chrono>
#include <cstdlib>

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
	namedWindow("Tracker Image");

  	//String filePath = "./summer_bees_video_2022_08_13.aedat4";
	std::string filePath = "./event_log_10_7_board.aedat4";
	if(argc > 1)
	{
		filePath = argv[1];
	}
    
  	auto reader = dv::io::MonoCameraRecording(filePath);
	dv::io::DataReadHandler handler;
  	
	const int imageWidth = 640, imageHeight = 480;

	// Initializes a screen - its grayscale but uses 3 channels so that clusters can be drawn on the screen in RGB
	cv::Mat tsImg(imageHeight, imageWidth, CV_8UC3, cv::Scalar(1));
	// Initializes the blurred time surface, used to control the creation of new clusters
	cv::Mat tsBlurred(imageHeight / constants::blurScale, imageWidth / constants::blurScale, CV_64FC1, cv::Scalar(0));

	int64_t lastTimeStamp = -1;
	
	// initialize to negative values to signal needed update
	// all timestamps are 64-bit ints to avoid overflow/wraparound
	int64_t nextTime = -1;
	int64_t nextFrame = -1;
	int64_t nextSustain = -1;
	int64_t prevTime = -1;

	std::vector<Cluster> clusters = std::vector<Cluster>();

	int colorIndex = 0;
	int netCrossing = 0;
	int totalCrossing = 0;

	// define a function for when the file reader encounters an event packet
	handler.mEventHandler = [&tsImg, &tsBlurred, &lastTimeStamp, &nextTime, &nextFrame, &nextSustain, &prevTime,
					&clusters, &imageWidth, &imageHeight, /* &constants::blurScale, */ &colorIndex, &netCrossing, &totalCrossing](const dv::EventStore &nextEvent)
	{

	    	// choice of colors
	    	const int numColors = 8;
		const cv::viz::Color colors[numColors] = {cv::viz::Color::amethyst(), cv::viz::Color::blue(),
									cv::viz::Color::orange(), cv::viz::Color::red(),
									cv::viz::Color::yellow(), cv::viz::Color::pink(),
									cv::viz::Color::lime(), cv::viz::Color::cyan()};

		if (nextEvent.isEmpty())
		{
			return;
		}

		// make sure you get the first timestamp
		if (lastTimeStamp < 0)
		{
			lastTimeStamp = nextEvent.getLowestTime();
		}

		// use total time elapsed from end of last packet to end of this one
		// to determine the delay length
		int64_t timeElapsed = nextEvent.getHighestTime() - lastTimeStamp;
		lastTimeStamp = nextEvent.getHighestTime();
		// extract the events from the EventStore
		dv::cvector<dv::Event> events = nextEvent.toPacket().elements;

		// loop through each event in the batch
		for (int i = 0; i < events.size(); i++)
		{
			dv::Event event = events.at(i);
			int64_t timeStamp = event.timestamp();
			uint16_t x = event.x();
			uint16_t y = event.y();
			bool pol = event.polarity();

			// set initial timestamps
			if (prevTime < 0)
			{
				prevTime = timeStamp;
			}
			if (nextTime < 0)
			{
				nextTime = timeStamp;
			}
			if (nextFrame < 0)
			{
				nextFrame = timeStamp;
			}
			if (nextSustain < 0)
			{
				nextSustain = timeStamp;
			}
			// only update on off spikes
			if (!pol)
			{
				// Updates the time surface - this is purely for visualization purposes at this point
				tsImg.at<cv::Vec3b>(y, x) = cv::Vec3b(255, 255, 255);
				// Increases the value of the corresponding region in the blurred time surface
				tsBlurred.at<double>(y / constants::blurScale, x / constants::blurScale) += constants::blurIncreaseFactor;
				// Finds the distance of the event from each existing cluster
				std::vector<double> distances = std::vector<double>();
				for (int i = 0; i < clusters.size(); i++)
				{
					distances.push_back(clusters.at(i).distance(x, y));
					// continue movement based on velocity and time elapsed
					clusters.at(i).contMomentum(timeStamp, prevTime);
				}
				if (!clusters.empty()) 
				{
					// retrieve the closest cluster
		            	int minDistance = distance(begin(distances), min_element(begin(distances), end(distances)));
					Cluster minCluster = clusters.at(distance(begin(distances), min_element(begin(distances), end(distances))));
					// If the event is inside the closest cluster, it updates the location of that cluster
					if (minCluster.inRange(x, y))
					{
						clusters.at(minDistance).shift(x, y);
						clusters.at(minDistance).newEvent();
					} // If there is an event very near but outside the cluster, increase the cluster's radius
					else if (minCluster.borderRange(x, y)) 
					{
						clusters.at(minDistance).updateRadius(constants::radiusGrowth);
	      	      	}
				}
				prevTime = timeStamp;
			}
			// exponential decay of blurred time surface
			tsBlurred *= constants::scaleFactor;
			// enforce maximum value of 1
			threshold(tsBlurred, tsBlurred, 1, 1, cv::THRESH_TRUNC);

			// only update clusters after a certain period of time
			// this is a costly computation, so is not performed with every event
			if (timeStamp > nextTime)
			{
				nextTime += constants::delayTime;
				// check of clusters need to be deleted
				if (timeStamp > nextSustain)
				{
					nextSustain += constants::clusterSustainTime;
					for (int i = 0; i < clusters.size(); i ++)
					{
						// delete a cluster if it did not have enough events
						Cluster cluster = clusters.at(i);
						if (!cluster.aboveThreshold(constants::clusterSustainThresh))
						{
							clusters.erase(remove(clusters.begin(), clusters.end(), cluster), clusters.end());
						}
						else // if it's above the threshold, reset the number of events
						{
							clusters.at(i).resetEvents();
						}
					}
				}
				for (int i = 0; i < imageWidth / constants::blurScale; i++)
				{
					for (int j = 0; j < imageHeight / constants::blurScale; j++)
					{
						// Adds a new cluster if three criteria are met:
	                           	// The region must be greater than the cluster initialization threshold
	                           	// The region can't be inside an already existing cluster
	                           	// There can't be more clusters than the max limit
						if (tsBlurred.at<double>(j,i) > constants::clusterInitThresh && clusters.size() < constants::maxClusters)
						{
							bool alreadyAdded = false;
							// check that it is not inside an already existing cluster
							for (Cluster cluster : clusters)
							{
								if (cluster.otherClusterRange(i * constants::blurScale, j * constants::blurScale))
								{
									alreadyAdded = true;
								}
							}
							// create a new cluster if it doesn't already exist
							if (!alreadyAdded)
							{
								Cluster newCluster = Cluster(i * constants::blurScale, j * constants::blurScale, colors[colorIndex++ % numColors], constants::alpha);
								clusters.push_back(newCluster);
							}
						}
					}
				}
				// update the velocity and shrink the radius
				for (int i = 0; i < clusters.size(); i ++)
				{
					clusters.at(i).updateVelocity(constants::delayTime);
					clusters.at(i).updateRadius(constants::radiusShrink);
	          			int newCrossing = clusters.at(i).updateSide(imageWidth, imageHeight);
	           			if (newCrossing != 0)
	           			{
	              			netCrossing -= newCrossing;
	              			totalCrossing += abs(newCrossing);
	              			std::cout << "Total Crossed: " << totalCrossing << std::endl;
	              			std::cout << "Net Crossed: " << netCrossing << std::endl;
	            		}
				}
			}
			// display update condition
			if (timeStamp > nextFrame)
			{
				nextFrame += constants::displayTime;
				// copy of time surface matrix to draw clusters on
				cv::Mat trackImg(imageHeight, imageWidth, CV_8UC3, cv::Scalar(1));
				tsImg.copyTo(trackImg);

	          		cv::line(trackImg, cv::Point(imageWidth/2, imageHeight / 4), cv::Point(imageWidth/2, 3 * imageHeight / 4), cv::viz::Color::red());
				cv::line(trackImg, cv::Point(0, imageHeight / 4), cv::Point(imageWidth / 2, imageHeight / 4), cv::viz::Color::red());
				cv::line(trackImg, cv::Point(0, 3 * imageHeight / 4), cv::Point(imageWidth / 2, 3 * imageHeight / 4), cv::viz::Color::red());
				cv::Mat resized;
	          		//resize(tsBlurred, resized, Size(imageWidth, imageHeight));
				// draw each cluster
				for (Cluster cluster : clusters)
				{
					cluster.draw(trackImg);
	           			//cluster.draw(resized);
				}
	          		//imshow("Blurred Image",resized);
	          		cv::imshow("Tracker Image", trackImg);
	          		//resizeWindow("Blurred Image", imageHeight, imageWidth);
				cv::waitKey(1);
				// time surface exponential decay
				tsImg *= constants::imgScaleFactor;
			}
		}
	};

	reader.run(handler);
	printf("End of recording.\n");
	return 0;
}
