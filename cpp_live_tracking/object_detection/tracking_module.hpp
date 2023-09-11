#ifndef DV_BEE_TRACKING_MODULE_H
#define DV_BEE_TRACKING_MODULE_H

#include <dv-sdk/module.hpp>
#include "../cluster/cluster.hpp"
#include <opencv2/imgproc.hpp>
#include <vector>

#define NUM_COLORS 8;

class BeeTrackingModule : public dv::ModuleBase {
private:
	// Scale factors close to 1 mean accumulation for a long time
    const double scaleFactor = 0.995; // A scale factor of 0 means no accumulation
    const double imgScaleFactor = 0.7;

  	// Frame rate is used to control the display
  	// The actual algorithm won't use frames, but we have to use frames if we want to see the data
    const int frameRate = 100;
    const int displayTime = 1000000 / frameRate;

  	// This controls how often certain costly procedures are performed, such as checking for new clusters
    const int updateRate = 150;
    const int delayTime = 1000000 / updateRate;

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

	// choice of colors
    int colorIndex = 0;
    const cv::viz::Color colors[NUM_COLORS] = {cv::viz::Color::amethyst(), cv::viz::Color::navy(),
                                  cv::viz::Color::orange(), cv::viz::Color::red(),
                                  cv::viz::Color::yellow(), cv::viz::Color::pink(),
                                  cv::viz::Color::lime(), cv::viz::Color::cyan()};

    std::vector<Cluster> clusters = std::vector<Cluster>();

    int64_t nextTime = 0;
    int64_t nextFrame = 0;
    int64_t nextSustain = 0;
	int64_t prevTime = 0;

    // retrieve the event resolution
	cv::Size resolution;

	int imageWidth, imageHeight;
  	int netCrossing = 0, totalCrossing = 0;

    // Initializes a screen - its grayscale but uses 3 channels so that clusters can be drawn on the screen in RGB
    cv::Mat tsImg;
	// Initializes the blurred time surface, used to control the creation of new clusters
    cv::Mat tsBlurred;

public:
	static void initInputs(dv::InputDefinitionList &in) {
		in.addEventInput("events");
	}

	static void initOutputs(dv::OutputDefinitionList &out) {
		out.addFrameOutput("trackers");
	}

	static const char *initDescription() {
		return ("This module displays the tracking boxes for the bees in the input date");
	}

	static void initConfigOptions(dv::RuntimeConfig &config) {
		config.add("red", dv::ConfigOption::intOption("Value of the red color component", 255, 0, 255));
		config.add("green", dv::ConfigOption::intOption("Value of the green color component", 255, 0, 255));
		config.add("blue", dv::ConfigOption::intOption("Value of the blue color component", 255, 0, 255));

		config.setPriorityOptions({"red", "green", "blue"});
	}

	BeeTrackingModule() {
		outputs.getFrameOutput("frames").setup(inputs.getEventInput("events"));
		resolution = inputs.getEventInput("events").size();

        imageWidth = 
	}

	void configUpdate() override {
		color = cv::Vec3b(static_cast<uint8_t>(config.getInt("blue")), static_cast<uint8_t>(config.getInt("green")),
			static_cast<uint8_t>(config.getInt("red")));
	}

	void run() override;
};

registerModuleClass(BeeTrackingModule)

#endif // DV_EXAMPLE_MODULE_COLOR_PAINT_EXAMPLEMODULECOLORPAINT_H
