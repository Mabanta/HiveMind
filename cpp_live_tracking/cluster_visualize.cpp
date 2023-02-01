// based on https://gitlab.com/inivation/dv/dv-processing/-/blob/rel_1.5/samples/io/aedat4-player.cpp

#include "./cluster/cluster.hpp"

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
#include <string>

using namespace std;
using namespace cv;

float readClusterLog(int * strIndex, string line);

int main(void) {

	namedWindow("Tracker Image");

	int64_t nextFrame = -1;
	int64_t nextTime = -1;

	//cout << "Enter path to file: " << endl;

	string filePath = "./event_log_beehive_9_25_board_2.aedat4";
	//string filePath = "./event_log_beehive_9_18_hori.aedat4";
	//cin >> filePath;

	fstream clusterLog;
	clusterLog.open("./cluster_log_beehive_9_25_board_2.csv");
	//clusterLog.open("./cluster_log_beehive_9_18_hori.csv");

	auto reader = dv::io::MonoCameraRecording(filePath);
	string line;
	getline(clusterLog, line);

	// handler defines what happens when each stream has data
	// streams include events, frames, and IMU
	dv::io::DataReadHandler handler;

	int64_t lastTimeStamp = -1;

	const int imageWidth = 640, imageHeight = 480;
	Mat tsImg(imageHeight, imageWidth, CV_8UC3, Scalar(1));

	vector<float> cluster_x = vector<float>();
	vector<float> cluster_y = vector<float>();
	vector<float> cluster_r = vector<float>();

	// define a function for when the file reader encounters an event packet
	handler.mEventHandler = [&tsImg, &lastTimeStamp, &imageWidth, &imageHeight,
	&nextFrame, &clusterLog, &nextTime, &cluster_x, &cluster_y, &cluster_r, &line](const dv::EventStore &nextEvent) {
		const double imgScaleFactor = 0.7;

	// Frame rate is used to control the display
	// The actual algorithm won't use frames, but we have to use frames if we want to see the data
		const int frameRate = 200;
		const int displayTime = 1000000 / frameRate;

		const int updateRate = 150;
		const int delayTime = 1000000 / updateRate;

		const double alpha = 0.1;

		int netCrossing, totalCrossing;

		string tempLine;
		string strTimestamp;


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

		for (int k = 0; k < (events.size()); k++) {
			dv::Event event = events.at(k);

			int64_t timeStamp = event.timestamp();
			uint16_t x = event.x();
			uint16_t y = event.y();
			bool pol = event.polarity();

			if (nextFrame < 0)
				nextFrame = timeStamp;
			if (nextTime < 0) {
				getline(clusterLog, line);
				strTimestamp = line.substr(0,16);
				nextTime = stol(strTimestamp);
			}

			if (!pol) {
				tsImg.at<Vec3b>(y,x) = Vec3b(255, 255, 255);
			}

			if (timeStamp > nextTime) {

				getline(clusterLog, tempLine);
				strTimestamp = tempLine.substr(0,16);
				nextTime = stol(strTimestamp);

				int strIndex = 18;
				totalCrossing = (int)readClusterLog(&strIndex, line);
				netCrossing = (int)readClusterLog(&strIndex, line);

				cout << "Total: " << totalCrossing << ", Net: " << netCrossing << endl;

				cluster_x = vector<float>();
				cluster_y = vector<float>();
				cluster_r = vector<float>();

				strIndex ++;
				if (line[strIndex] != ',') {
					while (line[strIndex] != ',' && line[strIndex] != '\n') {

						float values[5];
						for (int i = 0; i < 5; i ++) {
							values[i] = readClusterLog(&strIndex, line);
						}

						cluster_x.push_back(values[0]);
						cluster_y.push_back(values[1]);
						cluster_r.push_back(values[2]);

						strIndex ++;
					}
				}
				line = tempLine;
			}

			if (timeStamp > nextFrame) {
				nextFrame += displayTime;

				Mat trackImg(imageHeight, imageWidth, CV_8UC3, Scalar(1));
				tsImg.copyTo(trackImg);
				cv::line(trackImg,cv::Point(imageWidth/2,0),cv::Point(imageWidth/2,imageHeight),viz::Color::red());

				if(!cluster_x.empty()) {
					for (int i = 0; i < cluster_x.size(); i ++) {
						cv::circle(trackImg, cv::Point(cluster_x.at(i), cluster_y.at(i)), cluster_r.at(i), viz::Color::blue());
					}
				}

				imshow("Tracker Image", trackImg);
				waitKey(1);

				// time surface exponential decay
				tsImg *= imgScaleFactor;
			}
		}

	};

	// automatically reads the file to the end, calling the correct handler
	// when data is found in the corresponding stream
	reader.run(handler);

	// Close automatically done by destructor.

	printf("Shutdown successful.\n");

	return (EXIT_SUCCESS);
}

float readClusterLog(int * strIndex, string line) {
	int shiftIndex;
	for (shiftIndex = 0; line[(*strIndex)+shiftIndex] != ','; shiftIndex ++);
	string out = line.substr((*strIndex),shiftIndex);
	(*strIndex) += shiftIndex + 1;
	return stof(out);
}
