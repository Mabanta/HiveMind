#include <opencv2/viz/types.hpp>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <tgmath.h>
#include <dv-processing/core/core.hpp>
#include <tuple>

class Cluster {
    private:
        static long globId;
        int id, side{0};
        int64_t prevTime = -1;
        int eventCount = 0;
        double x, y, prev_x, prev_y;
        double alpha, radius{25.0}, vel_x{0.0}, vel_y{0.0};
        cv::viz::Color color;
        std::tuple<bool, int64_t, double, int> pixels[11][11];

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

        long getID();

        void draw(cv::Mat img);

        friend std::ostream& operator<<(std::ostream& out, const Cluster& src);

        bool operator==(const Cluster& comp);

};
