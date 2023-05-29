#include "tracking_module.hpp"

void BeeTrackingModule::run() {
    auto events = inputs.getEventInput("events").events();

    for (const auto &event : events) {
        int64_t timeStamp = event.timestamp();
		uint16_t x = event.x();
		uint16_t y = event.y();
		bool pol = event.polarity();

		// only update on off spikes
		if (!pol) {
			// Updates the time surface - this is purely for visualization purposes at this point
			tsImg.at<cv::Vec3b>(y, x) = cv::Vec3b(255, 255, 255);
			// Increases the value of the corresponding region in the blurred time surface
			tsBlurred.at<double>(y / constants::blurScale, x / constants::blurScale) += constants::blurIncreaseFactor;

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
				if (minCluster->inRange(x, y)) {
					minCluster->shift(x, y);
					minCluster->newEvent();

				} // If there is an event very near but outside the cluster, increase the cluster's radius
				else if (minCluster->borderRange(x, y)) {
					minCluster->updateRadius(constants::radiusGrowth);
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
			nextTime = timeStamp + constants::delayTime;

			// check of clusters need to be deleted
			if (timeStamp > nextSustain) 
			{
				nextSustain = timeStamp + constants::clusterSustainTime;

				for (auto clustIt = clusters.begin(); clustIt != clusters.end();) 
				{
					// delete a cluster if it did not have enough events
					if (!clustIt->aboveThreshold(clusterSustain))
					{
						clustIt = clusters.erase(clustIt);
					}
					else // if it's above the threshold, reset the number of events
					{
						clustIt->resetEvents();
						clustIt++;
					}
				}
			}

			for (int i = 0; i < imageWidth / constants::blurScale && clusters.size() < maxTrackers; i++) 
			{
				for (int j = 0; j < imageHeight / constants::blurScale && clusters.size() < maxTrackers; j++) 
				{
					// Adds a new cluster if three criteria are met:
					// The region must be greater than the cluster initialization threshold
					// The region can't be inside an already existing cluster
					// There can't be more clusters than the max limit
					if (tsBlurred.at<double>(j,i) > clusterInit) 
					{
						bool alreadyAdded = false;

						// check that it is not inside an already existing cluster
						for (Cluster cluster : clusters) 
						{
							if (cluster.otherClusterRange(i * constants::blurScale, j * constants::blurScale)) 
							{
								alreadyAdded = true;
								break;
							}
						}

						// create a new cluster if it doesn't already exist
						if (!alreadyAdded) 
						{
							Cluster newCluster = Cluster(i * constants::blurScale, j * constants::blurScale, colors[colorIndex++ % NUM_COLORS], clusterAlpha);
							clusters.push_back(newCluster);
						}
					}
				}
			} // end blurred matrix loop

			// update the velocity and shrink the radius
			for (int i = 0; i < clusters.size(); i++) 
			{
				clusters.at(i).updateVelocity(constants::delayTime);
				clusters.at(i).updateRadius(constants::radiusShrink);

				int newCrossing = clusters.at(i).updateSide(imageWidth, imageHeight);
	     		if (newCrossing != 0) 
	     		{
	     			netCrossing -= newCrossing;
	     			totalCrossing++;
				}
			} // end cluster updates

			// display update condition
			if (timeStamp > nextFrame)
			{
				nextFrame = timeStamp + constants::displayTime;
				// copy of time surface matrix to draw clusters on
				cv::Mat trackImg(imageHeight, imageWidth, CV_8UC3, cv::Scalar(1));
				tsImg.copyTo(trackImg);
				// draw each cluster
				for (Cluster cluster : clusters) {
					cluster.draw(trackImg);
				}
				// display to dv-gui output
				outputs.getFrameOutput("trackers") << trackImg << dv::commit;

				// time surface exponential decay
				tsImg *= constants::imgScaleFactor;
			}
		}
    }
}