#include "cluster_v2.hpp"

long Cluster::globId = 0;
const double pi = 3.14159;

Cluster::Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha, int64_t time) {
    this->alpha = alpha;
    this->x = (double)x;
    this->y = (double)y;
    this->prev_x = (double)x;
    this->prev_y = (double)y;
    this->color = color;
    this->id = globId++;

    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            pixels[i][j] = std::make_tuple(true, -1, 0.0, -1);
        }
    }
}

double Cluster::distance(unsigned int x, unsigned int y) {
    //return pow(pow(x - this->x, 2) + pow(y - this->y, 2), 0.5);
    return std::max(fabs((double)x - this->x), fabs((double)y - this->y));
}

bool Cluster::inRange(unsigned int x, unsigned int y) {
    return distance(x, y) < radius;
}

bool Cluster::borderRange(unsigned int x, unsigned int y) {
    return distance(x, y) < radius * 1.33;
}

bool Cluster::otherClusterRange(unsigned int x, unsigned int y) {
    return distance(x, y) < radius * 2;
}

void Cluster::shift(unsigned int x, unsigned int y) {
    this->x = (1 - alpha) * this->x + alpha * (double)x;
    this->y = (1 - alpha) * this->y + alpha * (double)y;
}

void Cluster::contMomentum(int64_t eventT, int64_t prevT) {
    x = x + vel_x * (eventT - prevT);
    y = y + vel_y * (eventT - prevT);
}

void Cluster::updateVelocity(unsigned int delay) {
    vel_x = (x - prev_x) / (double)delay;
    vel_y = (y - prev_y) / (double)delay;

    prev_x = x;
    prev_y = y;
}

void Cluster::updateRadius(float growthFactor) {
    radius *= growthFactor * ((40-radius)/15);
}

bool Cluster::aboveThreshold(unsigned int threshold) {
    return eventCount >= threshold;
}

void Cluster::newEvent() {
    eventCount++;
}

void Cluster::updateFreq(dv::Event event) {
    // adjust x and y so it is the relative position in the array
    // with the center at [10][10]

    if (distance(event.x(), event.y()) > 10) {
        return;
    }

    int surfaceX = ((int)(event.x() - this->x) + 10) / 3;
    int surfaceY = ((int)(event.y() - this->y) + 10) / 3;

    auto pixelData = pixels[surfaceX][surfaceY];
    
    int64_t prevTime = std::get<1>(pixelData);
    double runningAvg = std::get<2>(pixelData);
    int transitionCount = std::get<3>(pixelData);

    if (event.polarity() && !std::get<0>(pixelData)) {
        if (event.timestamp() - prevTime > 10000) {
            transitionCount = 0;
            runningAvg = 0;
            prevTime = 0;
        }

        if (prevTime > 0) {
            runningAvg = (7 * runningAvg + (event.timestamp() - prevTime)) / 8;
        }

        prevTime = event.timestamp();
        transitionCount++;
    }

    pixels[surfaceX][surfaceY] = std::make_tuple(event.polarity(), prevTime, runningAvg, transitionCount);

    newFreq();
}


void Cluster::resetEvents() {
    eventCount = 0;
}

int Cluster::getSide(int width) {
  if (x < (double)(width/2 - 10))
    return -1;
  else if (x > (double)(width/2 + 10))
    return 1;
  return 0;
}

int Cluster::updateSide(int width) {
  int newSide = getSide(width);
  if (newSide != side && newSide != 0) {
    bool sideZero = (side == 0);
    side = newSide;
    if(!sideZero)
      return side;
  }
  return 0;
}

void Cluster::draw(cv::Mat img) {
    cv::circle(img, cv::Point(x, y), radius, color);
}

void Cluster::newFreq() {
    double sum = 0.0;
    int total = 0;
    int newFrequency = -1;

    std::vector<float> averages = std::vector<float>();

    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            auto pixelData = pixels[i][j];

            if (std::get<3>(pixelData) >= 8) {
                //sum += std::get<2>(pixelData);
                //total++;
                averages.push_back(std::get<2>(pixelData));
            }
        }
    }

    std::sort(averages.begin(), averages.end());

    if (averages.size() > 0) {
        float median;

        if (averages.size() % 2 != 0)
            median = averages.at(averages.size() / 2);
        else 
            median = (averages.at(averages.size() / 2 - 1) + averages.at(averages.size() / 2)) / 2;

        newFrequency = 1000000 / median;
    }

    freq = newFrequency;
}

int Cluster::getFrequency() const {
    return freq;
}

long Cluster::getID() {
  return id;
}

// overloading outstream operator to print info in csv format
std::ostream& operator<<(std::ostream& out, const Cluster& src) {
    out << src.x << "," << src.y << "," << src.radius << "," << src.vel_x << "," << src.vel_y << ", " << src.freq << ",";
    return out;
}

bool Cluster::operator==(const Cluster& comp) {
    return id == comp.id;
}
