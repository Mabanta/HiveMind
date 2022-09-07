#define LIBCAER_FRAMECPP_OPENCV_INSTALLED 0
#include <libcaercpp/devices/dvs128.hpp>

#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <csignal>
#include <chrono>

using namespace std;
using namespace cv;

static atomic_bool globalShutdown(false);

static void globalShutdownSignalHandler(int signal) {
	// Simply set the running flag to false on SIGTERM and SIGINT (CTRL+C) for global shutdown.
	if (signal == SIGTERM || signal == SIGINT) {
		globalShutdown.store(true);
	}
}

static void usbShutdownHandler(void *ptr) {
	(void) (ptr); // UNUSED.

	globalShutdown.store(true);
}

int main(void) {
	//----------- Timmer's Code -----------
	int chs = 8; // cluster half size box starts at 16x16

	// matrix holding the time surface
	Mat ts_img(128, 128, CV_8UC1, 128);
	Mat clust_img(128, 128, CV_8UC3);
	Mat sub_mat;
	// Mat_<float> tmpfloat_img (128, 128);
	bool not_in_a_cluster;
	int cntr = 0;
	int xx, yy;
	int numclusters;
	int cluster_thresh = 20;
	double cluster_activity;
	float clusters[5][4] = {{-100, 0, 0, 0}, {-100, 0, 0, 0}, {-100, 0, 0, 0}, {-100, 0, 0, 0}, {-100, 0, 0, 0}}; // 4 clusters of x, y, size (in pixels)

	vector<int> sortvec;
	// -----------------------

	// y1 is defined in another library, so must be defined locally
	//  x, y, size, timeout
	int x1, x2, y1, y2, subx, suby;

	// Timmer's opencv setup code
	namedWindow("Clusters");
	// namedWindow ("Time Surface Image");

// Install signal handler for global shutdown.
#if defined(_WIN32)
	if (signal(SIGTERM, &globalShutdownSignalHandler) == SIG_ERR) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGTERM. Error: %d.", errno);
		return (EXIT_FAILURE);
	}

	if (signal(SIGINT, &globalShutdownSignalHandler) == SIG_ERR) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGINT. Error: %d.", errno);
		return (EXIT_FAILURE);
	}
#else
	struct sigaction shutdownAction;

	shutdownAction.sa_handler = &globalShutdownSignalHandler;
	shutdownAction.sa_flags   = 0;
	sigemptyset(&shutdownAction.sa_mask);
	sigaddset(&shutdownAction.sa_mask, SIGTERM);
	sigaddset(&shutdownAction.sa_mask, SIGINT);

	if (sigaction(SIGTERM, &shutdownAction, NULL) == -1) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGTERM. Error: %d.", errno);
		return (EXIT_FAILURE);
	}

	if (sigaction(SIGINT, &shutdownAction, NULL) == -1) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGINT. Error: %d.", errno);
		return (EXIT_FAILURE);
	}
#endif

	// Open a DVS128, give it a device ID of 1, and don't care about USB bus or SN restrictions.
	libcaer::devices::dvs128 dvs128Handle = libcaer::devices::dvs128(1, 0, 0, "");

	// Let's take a look at the information we have on the device.
	struct caer_dvs128_info dvs128_info = dvs128Handle.infoGet();

	printf("%s --- ID: %d, Master: %d, DVS X: %d, DVS Y: %d, Firmware: %d.\n", dvs128_info.deviceString,
		dvs128_info.deviceID, dvs128_info.deviceIsMaster, dvs128_info.dvsSizeX, dvs128_info.dvsSizeY,
		dvs128_info.firmwareVersion);

	// Send the default configuration before using the device.
	// No configuration is sent automatically!
	dvs128Handle.sendDefaultConfig();

	// Tweak some biases, to increase bandwidth in this case.
	dvs128Handle.configSet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_PR, 695);
	dvs128Handle.configSet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_FOLL, 867);
	//dvs128Handle.configSet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_PR, 217);  // taken from expts on DV
	//dvs128Handle.configSet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_FOLL, 271);  // taken from expts on DV
	dvs128Handle.configSet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_DIFFOFF, 132);  // taken from expts on DV
	dvs128Handle.configSet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_DIFFON, 209996); // taken from expts on DV

	// Let's verify they really changed!
	uint32_t prBias   = dvs128Handle.configGet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_PR);
	uint32_t follBias = dvs128Handle.configGet(DVS128_CONFIG_BIAS, DVS128_CONFIG_BIAS_FOLL);

	printf("New bias values --- PR: %d, FOLL: %d.\n", prBias, follBias);

	// Now let's get start getting some data from the device. We just loop in blocking mode,
	// no notification needed regarding new events. The shutdown notification, for example if
	// the device is disconnected, should be listened to.
	dvs128Handle.dataStart(nullptr, nullptr, nullptr, &usbShutdownHandler, nullptr);

	// Let's turn on blocking data-get mode to avoid wasting resources.
	dvs128Handle.configSet(CAER_HOST_CONFIG_DATAEXCHANGE, CAER_HOST_CONFIG_DATAEXCHANGE_BLOCKING, true);

	while (!globalShutdown.load(memory_order_relaxed)) {
		std::unique_ptr<libcaer::events::EventPacketContainer> packetContainer = dvs128Handle.dataGet();
		if (packetContainer == nullptr) {
			continue; // Skip if nothing there.
		}

		//printf("\nGot event container with %d packets (allocated).\n", packetContainer->size());

		for (auto &packet : *packetContainer) {
			if (packet == nullptr) {
				//printf("Packet is empty (not present).\n");
				continue; // Skip if nothing there.
			}

			//printf("Packet of type %d -> %d events, %d capacity.\n", packet->getEventType(), packet->getEventNumber(), packet->getEventCapacity());

			// only process polarity spikes
			if (packet->getEventType() == POLARITY_EVENT) {
				std::shared_ptr<const libcaer::events::PolarityEventPacket> polarity
					= std::static_pointer_cast<libcaer::events::PolarityEventPacket>(packet);

				for (int k=0; k<(packet->getEventNumber()); ++k) {
					const libcaer::events::PolarityEvent &firstEvent = (*polarity)[k];

					//int32_t ts = firstEvent.getTimestamp();
					uint16_t x = firstEvent.getX();
					uint16_t y = firstEvent.getY();
					//bool pol   = firstEvent.getPolarity();
					//printf("Event - ts: %d, x: %d, y: %d, pol: %d.\n", ts, x, y, pol);

					//printf ("%d",ts_img.at<uchar>(x,y));
					// go through each surrounding pixel and increment their values
					for (int j = -1; j < 2; j++) {
						for (int k = -1; k < 2; k++) {
							xx = x + j;
							yy = y + k;
							if (xx < 0) xx = 0;  // limit checking - should use 'sort'
							if (yy < 0) yy = 0;
							if (xx > 127) xx = 127;
							if (yy > 127) yy = 127;
							ts_img.at<uchar>(yy,xx) += 2;
						}
					}

					// Make an on spike affect surrounding pixels
					//if (pol == 1) {
					//	ts_img.at<uchar>(y,x) += 30;
					//	ts_img.at<uchar>(y,x+1) += 30;
					//	ts_img.at<uchar>(y+1,x) += 30;
					//	ts_img.at<uchar>(y,x-1) += 30;
					//	ts_img.at<uchar>(y-1,x) += 30;
					//}
					//else {
					//	ts_img.at<uchar>(y,x) = 0; // off spikes reset value
					//}
					not_in_a_cluster = true;
					for (size_t i=0; i<5; i++) { // UPDATE CLUSTER that the spike belongs to
						if ((x < clusters[i][0]+chs) && (x > clusters[i][0]-chs) && (y < clusters[i][1]+chs) && (y > clusters[i][1]-chs)) {
							not_in_a_cluster = false;
							clusters[i][0] = 0.9*clusters[i][0] + 0.1*x;
							clusters[i][1] = 0.9*clusters[i][1] + 0.1*y;
							i=6; // break out once you've processed the spike
						}
					}
					if (not_in_a_cluster) { // SPAWN A NEW TRACKER
						for (int i=0; i<5; i++) {
							if (clusters[i][0] == -100) {  // Yes! a cluster is available
								if (ts_img.at<uchar>(x,y) > cluster_thresh) {  // is there a build up in the ts?
									clusters[i][0] = x;  // if so, create new cluster
									clusters[i][1] = y;
									//printf ("spawned cluster %d\n",i);
									i=6;
								} // should we create new ones?
							} //
						}  // end for
					} // end else
				}

				cntr++; // only do this every three spikes
				if (cntr%3 == 0) {
					cntr = 0;
					// cluster management
					clust_img = ts_img.clone();
					for (int i = 0; i < 5; i++) { // go through the clusters
						if (clusters[i][0] != -100) {  // for each active cluster
							x1 = clusters[i][0] - chs;
							y1 = clusters[i][1] - chs;
							if (x1 < 0) x1 = 0;  // limit checking - should use 'sort'
							if (y1 < 0) y1 = 0;
							if (x1 > 127) x1 = 127;
							if (y1 > 127) y1 = 127;
							Point p1 (x1, y1);

							x2 = clusters[i][0] + chs;
							y2 = clusters[i][1] + chs;
							if (x2 < 0) x2 = 0;  // limit checking - should use 'sort'
							if (y2 < 0) y2 = 0;
							if (x2 > 127) x2 = 127;
							if (y2 > 127) y2 = 127;
							Point p2 (x2, y2);
							
							sub_mat = ts_img(Rect(x1,y1,x2-x1,y2-y1)); // grab the cluster box
							if (sum(sub_mat).val[0] <1500) { // not enough activity in the box
								clusters[i][0] = -100;  // kill the cluster for the next round
								//printf ("killed cluster %d\n",i);
							} // end if
							else {
								rectangle(clust_img, p1, p2, 255); // draw rectangle
							} // end else
						} // end 'if cluster is active'
					}  // end for different clusters
					imshow("Clusters",clust_img);
					resizeWindow("Clusters", 200, 200); // bigger so that we can grab the top to move it.
					waitKey(1);
					ts_img *= 0.5; // exponential decay of ts
				}
			}
		}
	}

	dvs128Handle.dataStop();
	// Close automatically done by destructor.
	printf("Shutdown successful.\n");
	return (EXIT_SUCCESS);
}