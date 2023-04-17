#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants
{
	// Scale factors close to 1 mean accumulation for a long time
	inline constexpr double scaleFactor { 0.995 };
	inline constexpr double imgScaleFactor { 0.700 };

	// Frame rate is used to control the display
	// The actual algorithm won't use frames, but we have to use frames if we want to see the data
 
	inline constexpr int frameRate { 100 };
	inline constexpr int displayTime = { 1000000 / frameRate };

	// This controls how often certain costly procedures are performed, such as checking for new clusters
	inline constexpr int updateRate { 150 };
	inline constexpr int delayTime { 1000000 / updateRate };

	// This algorithm uses a "blur" to make it easier to detect a lot of events occurring in the same region
	// The algorithm breaks the time surface into 20 x 20 regions and keeps track of how many events have occurred in each region
	// The blur scale controls the size of each region
	inline constexpr int blurScale { 20 };
	// This controls how much each event contributes to the regions in the blurred time surface
	// A higher number will make each region more sensitive to individual events

	inline constexpr double blurIncreaseFactor { 0.2 };

	inline constexpr int maxClusters { 20 }; // This puts a limit on how many clusters can be formed
	inline constexpr double clusterInitThresh { 0.9 }; // This is the value that a region in the blurred time surface must reach in order to initiate a cluster
	inline constexpr int clusterSustainThresh { 18 }; // This is the number of events that must occur within a certain time inside a cluster in order for it to survive
	inline constexpr int clusterSustainTime { 35000 }; // This is the amount of time that the program waits before checking if a cluster needs to be removed

	inline constexpr double radiusGrowth { 1.0007 }; // the rate of growth of a cluster when a nearby spike is found
	//const double radiusGrowth = 1;
	inline constexpr double radiusShrink { 0.998 }; // the rate of shrinkage of a cluster each time it is updated

	// This factor controls how sensitive a cluster is to location change based on new spikes
	// A higher value will cause the cluster to adapt more quickly, but it will also move more sporadically
	inline constexpr double alpha { 0.1 };

	// object detection just displays the counting information and shows tracking window
	// record doesn't show trackign window and records to csv
	// Shows tracking windows until u start recording
}

#endif