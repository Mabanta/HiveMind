#include <opencv2/viz/types.hpp>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdlib>
#include <tgmath.h>
#include <dv-processing/core/core.hpp>

class Cluster {
    private:
        static int globId;
        int id, side{0}, transitionCount{0};
        int64_t prevTime = -1;
        int eventCount = 0;
        float runningAvg{0.0}, running_sum{0.0};
        double x, y, prev_x, prev_y;
        double alpha, radius{25.0}, vel_x{0.0}, vel_y{0.0};
        bool prevPol {true};
        cv::viz::Color color;

    public:
        Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha, int64_t time);

        double distance(unsigned int x, unsigned int y);

        bool inRange(unsigned int x, unsigned int y);

        bool borderRange(unsigned int x, unsigned int y);

        bool otherClusterRange(unsigned int x, unsigned int y);

        void shift(unsigned int x, unsigned int y);

        void contMomentum(int64_t eventT, int64_t prevT);

        void updateVelocity(unsigned int delay);

        void updateRadius(float growthFactor);

        bool aboveThreshold(unsigned int threshold);

        void newEvent();

        void updateFreq(dv::Event);

        int getSide(int width);

        int updateSide(int width);

        void resetEvents();

        int getFrequency();

        int getID();

        void draw(cv::Mat img);

        friend std::ostream& operator<<(std::ostream& out, const Cluster& src);

        bool operator==(const Cluster& comp);

};
