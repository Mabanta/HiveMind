#include "../cluster/cluster.hpp"
#include "constants.hpp"

#include <dv-processing/core/core.hpp>
#include <libcaercpp/devices/dvxplorer.hpp>
#include <libcaercpp/devices/device.hpp>
#include <dv-processing/io/mono_camera_writer.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

#include <iostream> 
#include <fstream>
#include <atomic>
#include <csignal>
#include <chrono>
#include <cstdlib>

int main(int argc, char* argv[])
{
	// initialize to negative values to signal needed update
	// all timestamps are 64-bit ints to avoid overflow/wraparound
	int64_t nextTime = -1;
	int64_t nextFrame = -1;
	int64_t nextSustain = -1;
	int64_t prevTime = -1;

	int netCrossing = 0, totalCrossing = 0;
	std::vector<Cluster> clusters = std::vector<Cluster>();

	// create a capture object to read events from any DVS device connected
	dv::io::CameraCapture capture("", dv::io::CameraCapture::CameraType::DVS);

	// retrieve the event resolution
	std::optional<cv::Size> resolutionWrapper = capture.getEventResolution();

	int imageWidth, imageHeight;

	// extract the width and height of the camera resolution
	if (resolutionWrapper.has_value())
	{
		imageWidth = resolutionWrapper.value().width;
		imageHeight = resolutionWrapper.value().height;
	}
	else
	{
		std::cerr << "Could not retrieve camera resolution" << std::endl;
	}

	// Initializes the blurred time surface, used to control the creation of new clusters
	cv::Mat tsBlurred(imageHeight / constants::blurScale, imageWidth / constants::blurScale, CV_64FC1, cv::Scalar(0));

	// log file for clusters
	std::ofstream countLog;
	countLog.open("./count_log_001.csv");
 	countLog << "Timestamp,";
  	countLog << "Total Crossed,";
  	countLog << "Net Crossed,";
	countLog << "Number of Clusters,"
  	countLog << std::endl;

	// infinite loop as long as a shutdown signal is not sent
	while (capture.isRunning())
	{
		std::optional<dv::EventStore> eventsWrapper = capture.getNextEventBatch();

		// if there have been events
		if (eventsWrapper.has_value())
		{
			dv::EventStore events = eventsWrapper.value();

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

						}
						// If there is an event very near but outside the cluster, increase the cluster's radius
						else if (minCluster.borderRange(x, y)) 
						{
							clusters.at(minDistance).updateRadius(constants::radiusGrowth);
						}

					}
					prevTime = timeStamp;
				} // end polarity check
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
						for (int i = 0; i < clusters.size(); i++)
						{
							// delete a cluster if it did not have enough events
							Cluster cluster = clusters.at(i);
							if (!cluster.aboveThreshold(constants::clusterSustainThresh))
							{
								clusters.erase(clusters.begin() + i);
								i--;
							} 
							else 
							{// if it's above the threshold, reset the number of events
								clusters.at(i).resetEvents();
							}
						}
					}
					for (int i = 0; i < imageWidth / constants::blurScale  && clusters.size() < constants::maxClusters; i++)
					{
						for (int j = 0; j < imageHeight / constants::blurScale  && clusters.size() < constants::maxClusters; j++) 
						{
							// Adds a new cluster if three criteria are met:
							// The region must be greater than the cluster initialization threshold
							// The region can't be inside an already existing cluster
							// There can't be more clusters than the max limit
							if (tsBlurred.at<double>(j,i) > constants::clusterInitThresh) 
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
									Cluster newCluster = Cluster(i * constants::blurScale, j * constants::blurScale, constants::alpha);
									clusters.push_back(newCluster);
								}
							}
						}
					} // end blurred matrix loop
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
              					std::cout << "Total Crossed: " << totalCrossing << "\t Net Crossed: " << netCrossing << "\r";
	              				std::cout.flush();
						}
					}

					if (timeStamp > nextFrame) {
						nextFrame += 10000000;
						countLog << timeStamp << ",";
						countLog << totalCrossing << ",";
						countLog << netCrossing << ",";
						countLog << clusters.size() << ",";
						countLog << std::endl;
					}
				}
			}
		}
	}
	return 0;
}
