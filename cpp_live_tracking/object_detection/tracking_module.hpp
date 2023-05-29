#ifndef DV_BEE_TRACKING_MODULE_H
#define DV_BEE_TRACKING_MODULE_H

#include <dv-sdk/module.hpp>
#include "constants.hpp"
#include "../cluster/cluster.hpp"
#include <opencv2/imgproc.hpp>
#include <vector>

#define NUM_COLORS 8

class BeeTrackingModule : public dv::ModuleBase {
private:
	// choice of colors
    int colorIndex = 0, maxTrackers = 20, clusterSustain = 18;
	double clusterInit = 0.9, clusterAlpha = 0.1;
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
		config.add("max_trackers", dv::ConfigOption::intOption("Max number of trackers", 20, 1, 100));
		config.add("cluster_init_thresh", dv::ConfigOption::doubleOption("Threshold to intiailize a cluster", 0.9, 0.0, 1.0));
		config.add("cluster_sustain_thresh", dv::ConfigOption::intOption("Number of events to keep a cluster active", 18, 1, 50));
		config.add("cluster_alpha", dv::ConfigOption::doubleOption("Amount each event affects the movement of a cluster", 0.1, 0.0, 1.0));
		/* Model for allowing configuration
		config.add("red", dv::ConfigOption::intOption("Value of the red color component", 255, 0, 255));
		config.add("green", dv::ConfigOption::intOption("Value of the green color component", 255, 0, 255));
		config.add("blue", dv::ConfigOption::intOption("Value of the blue color component", 255, 0, 255));
		*/
		config.setPriorityOptions({"max_trackers"});
	}

	BeeTrackingModule() {
		outputs.getFrameOutput("trackers").setup(inputs.getEventInput("events"));
		resolution = inputs.getEventInput("events").size();

        imageWidth = resolution.width;
		imageHeight = resolution.height;

		tsImg = cv::Mat(imageHeight, imageWidth, CV_8UC3, cv::Scalar(1));
		tsBlurred = cv::Mat(imageHeight / constants::blurScale, imageWidth / constants::blurScale, CV_64FC1, cv::Scalar(0));
	}

	void configUpdate() override {
		maxTrackers = config.getInt("max_trackers");
		clusterInit = config.getDouble("cluster_init_thresh");
		clusterAlpha = config.getDouble("cluster_alpha");
		clusterSustain = config.getDouble("cluster_sustain_thresh");
	}

	void run() override;
};

registerModuleClass(BeeTrackingModule)

#endif // DV_EXAMPLE_MODULE_COLOR_PAINT_EXAMPLEMODULECOLORPAINT_H
