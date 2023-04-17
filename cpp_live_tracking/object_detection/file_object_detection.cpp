#include "../cluster/cluster.hpp"

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

int main(int argc, char* argv[]) {

    namedWindow("Tracker Image");
    //namedWindow("Blurred Image");

	// initialize to negative values to signal needed update
	// all timestamps are 64-bit ints to avoid overflow/wraparound
    int64_t nextTime = -1;
    int64_t nextFrame = -1;
    int64_t nextSustain = -1;
	int64_t prevTime = -1;

    vector<Cluster> clusters = vector<Cluster>();

  	//String filePath = "./summer_bees_video_2022_08_13.aedat4";
    String filePath = "./event_log_10_7_board.aedat4";
    if(argc > 1)
    {
    	filePath = argv[1];
    }
    
  	auto reader = dv::io::MonoCameraRecording(filePath);

    dv::io::DataReadHandler handler;
  	int64_t lastTimeStamp = -1;
    int colorIndex = 0;

	const int imageWidth = 640, imageHeight = 480;

  // This algorithm uses a "blur" to make it easier to detect a lot of events occurring in the same region
  // The algorithm breaks the time surface into 20 x 20 regions and keeps track of how many events have occurred in each region
  // The blur scale controls the size of each region
    const int blurScale = 20;
  // This controls how much each event contributes to the regions in the blurred time surface
  // A higher number will make each region more sensitive to individual events

	// extract the width and height of the camera resolution

	// Initializes a screen - its grayscale but uses 3 channels so that clusters can be drawn on the screen in RGB
    Mat tsImg(imageHeight, imageWidth, CV_8UC3, Scalar(1));
	// Initializes the blurred time surface, used to control the creation of new clusters
    Mat tsBlurred(imageHeight / blurScale, imageWidth / blurScale, CV_64FC1, Scalar(0));

    int netCrossing = 0, totalCrossing = 0;

	// define a function for when the file reader encounters an event packet
  handler.mEventHandler = [&tsImg, &tsBlurred, &lastTimeStamp, &nextTime, &nextFrame, &nextSustain, &prevTime,
    &clusters, &imageWidth, &imageHeight, &blurScale, &colorIndex, &netCrossing, &totalCrossing](const dv::EventStore &nextEvent) {

      // Scale factors close to 1 mean accumulation for a long time
        const double scaleFactor = 0.995; // A scale factor of 0 means no accumulation
        const double imgScaleFactor = 0.7;

    	// Frame rate is used to control the display
    	// The actual algorithm won't use frames, but we have to use frames if we want to see the data
        const int frameRate = 200;
        const int displayTime = 1000000 / frameRate;

    	// This controls how often certain costly procedures are performed, such as checking for new clusters
        const int updateRate = 150;
        const int delayTime = 1000000 / updateRate;


        const double blurIncreaseFactor = 0.2;

        const int maxClusters = 20; // This puts a limit on how many clusters can be formed
        const double clusterInitThresh = 0.9; // This is the value that a region in the blurred time surface must reach in order to initiate a cluster
        const int clusterSustainThresh = 18; // This is the number of events that must occur within a certain time inside a cluster in order for it to survive
        const int clusterSustainTime = 35000; // This is the amount of time that the program waits before checking if a cluster needs to be removed

        const double radiusGrowth = 1.0007; // the rate of growth of a cluster when a nearby spike is found
        //const double radiusGrowth = 1;
        const double radiusShrink = 0.998; // the rate of shrinkage of a cluster each time it is updated

    	//This factor controls how sensitive a cluster is to location change based on new spikes
    	//A higher value will cause the cluster to adapt more quickly, but it will also move more sporadically
    	const double alpha = 0.1;


    	// choice of colors
    	const int numColors = 8;
        const viz::Color colors[numColors] = {viz::Color::amethyst(), viz::Color::blue(),
                                      viz::Color::orange(), viz::Color::red(),
                                      viz::Color::yellow(), viz::Color::pink(),
                                      viz::Color::lime(), viz::Color::cyan()};



		if (nextEvent.isEmpty())
			return;

		// make sure you get the first timestamp
		if (lastTimeStamp < 0)
			lastTimeStamp = nextEvent.getLowestTime();

		// use total time elapsed from end of last packet to end of this one
		// to determine the delay length
		int64_t timeElapsed = nextEvent.getHighestTime() - lastTimeStamp;
		lastTimeStamp = nextEvent.getHighestTime();

		// extract the events from the EventStore
		dv::cvector<dv::Event> events = nextEvent.toPacket().elements;

			// loop through each event in the batch
			for (int i = 0; i < events.size(); i++) {
				dv::Event event = events.at(i);

				int64_t timeStamp = event.timestamp();
				uint16_t x = event.x();
				uint16_t y = event.y();
				bool pol = event.polarity();

				// set initial timestamps
				if (prevTime < 0)
					prevTime = timeStamp;

				if (nextTime < 0)
					nextTime = timeStamp;

				if (nextFrame < 0)
					nextFrame = timeStamp;

				if (nextSustain < 0)
					nextSustain = timeStamp;

				// only update on off spikes
				if (!pol) {
					// Updates the time surface - this is purely for visualization purposes at this point
					tsImg.at<Vec3b>(y, x) = Vec3b(255, 255, 255);
					// Increases the value of the corresponding region in the blurred time surface
					tsBlurred.at<double>(y / blurScale, x / blurScale) += blurIncreaseFactor;

					// Finds the distance of the event from each existing cluster
					vector<double> distances = vector<double>();
					for (int i = 0; i < clusters.size(); i++) {
						distances.push_back(clusters.at(i).distance(x, y));
						// continue movement based on velocity and time elapsed
						clusters.at(i).contMomentum(timeStamp, prevTime);
					}

					if (!clusters.empty()) {
						// retrieve the closest cluster
            int minDistance = distance(begin(distances), min_element(begin(distances), end(distances)));
						Cluster minCluster = clusters.at(distance(begin(distances), min_element(begin(distances), end(distances))));

						// If the event is inside the closest cluster, it updates the location of that cluster
						if (minCluster.inRange(x, y)) {
							clusters.at(minDistance).shift(x, y);
							clusters.at(minDistance).newEvent();

						} // If there is an event very near but outside the cluster, increase the cluster's radius
						else if (minCluster.borderRange(x, y)) {
							clusters.at(minDistance).updateRadius(radiusGrowth);
            }

					}

					prevTime = timeStamp;
				} // end polarity check

				// exponential decay of blurred time surface
				tsBlurred *= scaleFactor;
				// enforce maximum value of 1
				threshold(tsBlurred, tsBlurred, 1, 1, THRESH_TRUNC);

				// only update clusters after a certain period of time
				// this is a costly computation, so is not performed with every event
				if (timeStamp > nextTime) {
					nextTime += delayTime;

					// check of clusters need to be deleted
					if (timeStamp > nextSustain) {
						nextSustain += clusterSustainTime;

						for (int i = 0; i < clusters.size(); i ++) {
							// delete a cluster if it did not have enough events
              Cluster cluster = clusters.at(i);
							if (!cluster.aboveThreshold(clusterSustainThresh))
								clusters.erase(remove(clusters.begin(), clusters.end(), cluster), clusters.end());
							else // if it's above the threshold, reset the number of events
								clusters.at(i).resetEvents();
						}
					}

					for (int i = 0; i < imageWidth / blurScale; i++) {
						for (int j = 0; j < imageHeight / blurScale; j++) {
							// Adds a new cluster if three criteria are met:
                           	// The region must be greater than the cluster initialization threshold
                           	// The region can't be inside an already existing cluster
                           	// There can't be more clusters than the max limit
							if (tsBlurred.at<double>(j,i) > clusterInitThresh && clusters.size() < maxClusters) {
								bool alreadyAdded = false;

								// check that it is not inside an already existing cluster
								for (Cluster cluster : clusters) {
									if (cluster.otherClusterRange(i * blurScale, j * blurScale)) {
										alreadyAdded = true;
									}
								}

								// create a new cluster if it doesn't already exist
								if (!alreadyAdded) {
									Cluster newCluster = Cluster(i * blurScale, j * blurScale, colors[colorIndex++ % numColors], alpha);

									clusters.push_back(newCluster);
								}
							}
						}
					} // end blurred matrix loop

					// update the velocity and shrink the radius
					for (int i = 0; i < clusters.size(); i ++) {
						clusters.at(i).updateVelocity(delayTime);
						clusters.at(i).updateRadius(radiusShrink);

            			int newCrossing = clusters.at(i).updateSide(imageWidth, imageHeight);
            			if (newCrossing != 0) {
              				netCrossing -= newCrossing;
              				totalCrossing += abs(newCrossing);
              				cout << "Total Crossed: " << totalCrossing << endl;
              				cout << "Net Crossed: " << netCrossing << endl;
            			}
					}
				} // end cluster updates

				// display update condition
				if (timeStamp > nextFrame) {
					nextFrame += displayTime;

					// copy of time surface matrix to draw clusters on
					Mat trackImg(imageHeight, imageWidth, CV_8UC3, Scalar(1));
					tsImg.copyTo(trackImg);

          			cv::line(trackImg, cv::Point(imageWidth/2, imageHeight / 4), cv::Point(imageWidth/2, 3 * imageHeight / 4), viz::Color::red());
					cv::line(trackImg, cv::Point(0, imageHeight / 4), cv::Point(imageWidth / 2, imageHeight / 4), viz::Color::red());
					cv::line(trackImg, cv::Point(0, 3 * imageHeight / 4), cv::Point(imageWidth / 2, 3 * imageHeight / 4), viz::Color::red());

          			Mat resized;
          			//resize(tsBlurred, resized, Size(imageWidth, imageHeight));

					// draw each cluster
					for (Cluster cluster : clusters) {
						cluster.draw(trackImg);
            			//cluster.draw(resized);
          			}


          			//imshow("Blurred Image",resized);
          			imshow("Tracker Image", trackImg);
          			//resizeWindow("Blurred Image", imageHeight, imageWidth);
					waitKey(1);

					// time surface exponential decay
					tsImg *= imgScaleFactor;
				} //end image update
			} // end event loop

	}; //End event handler function

  reader.run(handler);
  printf("End of recording.\n");

	return 0;
} // end main
