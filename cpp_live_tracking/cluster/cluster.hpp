#include <opencv2/viz/types.hpp>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

class Cluster {
    private: 
        static int globId;
        int id;
        unsigned int x, y, prev_x, prev_y, eventCount{0};
        double alpha, radius{25.0}, vel_x{0.0}, vel_y{0.0};
        cv::viz::Color color;

    public: 
        Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha);

        double distance(unsigned int x, unsigned int y);

        double inRange(unsigned int x, unsigned int y);

        bool borderRange(unsigned int x, unsigned int y);

        bool otherClusterRange(unsigned int x, unsigned int y);

        void shift(unsigned int x, unsigned int y);

        void contMomentum(int64_t eventT, int64_t prevT);

        void updateVelocity(unsigned int delay);

        void updateRadius(float growthFactor);

        bool aboveThreshold(unsigned int threshold);

        void newEvent();

        void resetEvents();

        void draw(cv::Mat img);

        friend std::ostream& operator<<(std::ostream& out, const Cluster& src);

        bool operator==(const Cluster& comp);

};

