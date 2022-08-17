// based on https://gitlab.com/inivation/dv/dv-processing/-/blob/rel_1.5/samples/io/aedat4-player.cpp

#define LIBCAER_FRAMECPP_OPENCV_INSTALLED 0

#include <dv-processing/core/core.hpp>
#include <dv-processing/io/mono_camera_recording.hpp>
#include <libcaercpp/devices/device.hpp>

#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main(void) {

	namedWindow("Time Surface Image");

	Mat ts_img(128, 128, CV_8UC1, 1);
	Mat resized;
	String filePath = "./bee9-2021.aedat4";

	auto reader = dv::io::MonoCameraRecording(filePath);

	// handler defines what happens when each stream has data
	// streams include events, frames, and IMU
	dv::io::DataReadHandler handler;

	int64_t lastTimeStamp = -1;

	// define a function for when the file reader encounters an event packet
	handler.mEventHandler = [&ts_img, &resized, &lastTimeStamp](const dv::EventStore &nextEvent) {
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
			const dv::Event firstEvent = events[k];

			int64_t ts = firstEvent.timestamp();
			uint16_t x = firstEvent.x();
			uint16_t y = firstEvent.y();
			bool pol = firstEvent.polarity();

			// set pixel high if there is an on spike
			if (pol == 1) 
				ts_img.at<uchar>(y, x) = 255;
			else  // and low if there is an off spike
				ts_img.at<uchar>(y, x) = 0;
		}

		// resize the matrix so that the display isn't just 128 x 128
		resize(ts_img, resized, Size(256, 256));

		imshow("Time Surface Image", resized);
		resizeWindow("Time Surface Image", 256, 256);

		// waitkey will pause program if passed a numbeer <= 0
		if (timeElapsed >= 1000) 
			waitKey(timeElapsed / 1000LL);

		ts_img = ts_img - 8; // decay the time surface
		
	};

	// automatically reads the file to the end, calling the correct handler
	// when data is found in the corresponding stream
	reader.run(handler);

	// Close automatically done by destructor.

	printf("Shutdown successful.\n");

	return (EXIT_SUCCESS);
}