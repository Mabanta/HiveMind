#include "../cluster/cluster.hpp"

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
#include <future>
#include <thread>

using namespace std;
using namespace cv;

static string inputWait() {
	string input;
	cin >> input;
	return input;
}

int main(void) {

  	// Scale factors close to 1 mean accumulation for a long time
    const double scaleFactor = 0.995; // A scale factor of 0 means no accumulation
	const double imgScaleFactor = 0.7;

  	// This controls how often certain costly procedures are performed, such as checking for new clusters
    const int updateRate = 150;
    const int delayTime = 1000000 / updateRate;

	const int frameRate = 200;
	const int displayTime = 1000000 / frameRate;

    // This algorithm uses a "blur" to make it easier to detect a lot of events occurring in the same region
    // The algorithm breaks the time surface into 20 x 20 regions and keeps track of how many events have occurred in each region
    // The blur scale controls the size of each region
    const int blurScale = 20;
    // This controls how much each event contributes to the regions in the blurred time surface
    // A higher number will make each region more sensitive to individual events

    const double blurIncreaseFactor = 0.2;

    const int maxClusters = 20; // This puts a limit on how many clusters can be formed
    const double clusterInitThresh = 0.9; // This is the value that a region in the blurred time surface must reach in order to initiate a cluster
    const int clusterSustainThresh = 18; // This is the number of events that must occur within a certain time inside a cluster in order for it to survive
    const int clusterSustainTime = 35000; // This is the amount of time that the program waits before checking if a cluster needs to be removed

    const double radiusGrowth = 1.0007; // the rate of growth of a cluster when a nearby spike is found
    //const double radiusGrowth = 1;
    const double radiusShrink = 0.998; // the rate of shrinkage of a cluster each time it is updated

  	// This factor controls how sensitive a cluster is to location change based on new spikes
  	// A higher value will cause the cluster to adapt more quickly, but it will also move more sporadically
	const double alpha = 0.1;

    //namedWindow("Tracker Image");

	// initialize to negative values to signal needed update
	// all timestamps are 64-bit ints to avoid overflow/wraparound
    int64_t nextTime = -1;
    int64_t nextFrame = -1;
    int64_t nextSustain = -1;
	int64_t prevTime = -1;

    int netCrossing = 0, totalCrossing = 0;

    vector<Cluster> clusters = vector<Cluster>();

	// create a capture object to read events from any DVS device connected
	dv::io::CameraCapture capture("", dv::io::CameraCapture::CameraType::DVS);

	// retrieve the event resolution
	std::optional<cv::Size> resolutionWrapper = capture.getEventResolution();

	int imageWidth, imageHeight;

	// extract the width and height of the camera resolution
	if (resolutionWrapper.has_value()) {
		imageWidth = resolutionWrapper.value().width;
		imageHeight = resolutionWrapper.value().height;
	} else {
		cerr << "Could not retrieve camera resolution" << std::endl;
	}

	// Initializes the blurred time surface, used to control the creation of new clusters
    Mat tsBlurred(imageHeight / blurScale, imageWidth / blurScale, CV_64FC1, Scalar(0));
	Mat tsImg(imageHeight, imageWidth, CV_8UC3, Scalar(1));

	// configure log file for events
	dv::io::MonoCameraWriter::Config config;
	config.cameraName = "Xplorer";
	config.enableEvents = true;
	config.eventResolution = Size(imageWidth, imageHeight);
	dv::io::MonoCameraWriter eventLog("./event_log_001.aedat4", config);

	// log file for clusters
	ofstream clusterLog;
	clusterLog.open("./cluster_log_001.csv");
 	clusterLog << "Timestamp, ";
  	clusterLog << "Total Crossed, ";
  	clusterLog << "Net Crossed, ";

  	for (int i = 0; i < maxClusters; i ++) {
    	clusterLog << "Cluster " << i << ", ";
  	}

  	clusterLog << std::endl;

	std::chrono::microseconds timeout(10);
	cout << "Enter any input to begin counting: " << endl;
	future<string> callBack = async(inputWait);

	// record but don't count until any input has been given
	while (capture.isRunning() && callBack.wait_for(timeout) == future_status::timeout) {
		std::optional<dv::EventStore> eventsWrapper = capture.getNextEventBatch();

		// if there have been events
		if (eventsWrapper.has_value()) {
			dv::EventStore events = eventsWrapper.value();

			// loop through each event in the batch
			for (int i = 0; i < events.size(); i++) {
        		dv::Event event = events.at(i);

				int64_t timeStamp = event.timestamp();
				uint16_t x = event.x();
				uint16_t y = event.y();
				bool pol = event.polarity();

				if (nextFrame < 0)
					nextFrame = timeStamp;

				if (!pol) {
					tsImg.at<Vec3b>(y,x) = Vec3b(255, 255, 255);
				}

				if (timeStamp > nextFrame) {
					nextFrame += displayTime;

					Mat trackImg(imageHeight, imageWidth, CV_8UC3, Scalar(1));
					tsImg.copyTo(trackImg);

					cv::line(trackImg, cv::Point(imageWidth/2, imageHeight / 4), cv::Point(imageWidth/2, 3 * imageHeight / 4), viz::Color::red());
					cv::line(trackImg, cv::Point(0, imageHeight / 4), cv::Point(imageWidth / 2, imageHeight / 4), viz::Color::red());
					cv::line(trackImg, cv::Point(0, 3 * imageHeight / 4), cv::Point(imageWidth / 2, 3 * imageHeight / 4), viz::Color::red());

					imshow("Tracker Image", trackImg);
					waitKey(1);

					// time surface exponential decay
					tsImg *= imgScaleFactor;
				}
			}

			eventLog.writeEvents(events);
		}
	}

	// infinite loop as long as a shutdown signal is not sent
	while (capture.isRunning()) {
		std::optional<dv::EventStore> eventsWrapper = capture.getNextEventBatch();

		// if there have been events
		if (eventsWrapper.has_value()) {
			dv::EventStore events = eventsWrapper.value();

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
					// Increases the value of the corresponding region in the blurred time surface
					tsBlurred.at<double>(y / blurScale, x / blurScale) += blurIncreaseFactor;

					// Finds the distance of the event from each existing cluster
					Cluster *minCluster = NULL;
					double minDistance = imageWidth + imageHeight;

					for (int i = 0; i < clusters.size(); i++) {
						double newDist = clusters.at(i).distance(x, y);

						// keep track of the min dist and the cluster associated with it
						if (newDist < minDistance) {
							minDistance = newDist;
							minCluster = &clusters.at(i);
						}

						// continue movement based on velocity and time elapsed
						clusters.at(i).contMomentum(timeStamp, prevTime);
					}

					if (minCluster != NULL) {

						// If the event is inside the closest cluster, it updates the location of that cluster
						if ((*minCluster).inRange(x, y)) {
							clusters.at(minDistance).shift(x, y);
							clusters.at(minDistance).newEvent();

						} // If there is an event very near but outside the cluster, increase the cluster's radius
						else if ((*minCluster).borderRange(x, y)) {
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

						for (int i = 0; i < clusters.size(); i++) {
							// delete a cluster if it did not have enough events
              				Cluster cluster = clusters.at(i);
							if (!cluster.aboveThreshold(clusterSustainThresh)) {
								clusters.erase(clusters.begin() + i);
								i--;
							} else {// if it's above the threshold, reset the number of events
								clusters.at(i).resetEvents();
							}
						}
					}

					for (int i = 0; i < imageWidth / blurScale  && clusters.size() < maxClusters; i++) {
						for (int j = 0; j < imageHeight / blurScale  && clusters.size() < maxClusters; j++) {
							// Adds a new cluster if three criteria are met:
                           	// The region must be greater than the cluster initialization threshold
                           	// The region can't be inside an already existing cluster
                           	// There can't be more clusters than the max limit
							if (tsBlurred.at<double>(j,i) > clusterInitThresh) {
								bool alreadyAdded = false;

								// check that it is not inside an already existing cluster
								for (Cluster cluster : clusters) {
									if (cluster.otherClusterRange(i * blurScale, j * blurScale)) {
										alreadyAdded = true;
									}
								}

								// create a new cluster if it doesn't already exist
								if (!alreadyAdded) {
									Cluster newCluster = Cluster(i * blurScale, j * blurScale, alpha);

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
              				totalCrossing++;

              				cout << "Total Crossed: " << totalCrossing << endl;
              				cout << "Net Crossed: " << netCrossing << endl;
            			}
					}

					clusterLog << timeStamp << ": ";
          			clusterLog << totalCrossing << ",";
          			clusterLog << netCrossing << ", ";

					// log cluster information to file
					for (int i = 0; i < maxClusters; i++) {
						if (i < clusters.size())
							clusterLog << clusters.at(i);
						else // create empty columns if no cluster exists
							clusterLog << ",,,,,";
					}
					clusterLog << std::endl;
				} // end cluster updates

			} // end event loop

			// log events
			eventLog.writeEvents(events);
		} // end check event store
	} // end global shutdown loop

	return 0;
} // end main
