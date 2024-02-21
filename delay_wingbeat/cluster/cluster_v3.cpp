#include "cluster_v3.hpp"

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

    blurredPixel = std::make_tuple(true, -1, 0.0, -1);
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

    if (distance(event.x(), event.y()) > 2) {
        return;
    }

    int64_t prevTime = std::get<1>(blurredPixel);
    double runningAvg = std::get<2>(blurredPixel);
    int transitionCount = std::get<3>(blurredPixel);

    if (event.polarity() && !std::get<0>(blurredPixel)) {
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

    blurredPixel = std::make_tuple(event.polarity(), prevTime, runningAvg, transitionCount);
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

int Cluster::getFrequency() {
    
    if (std::get<3>(blurredPixel) >= 8 && std::get<2>(blurredPixel) > 0) {
        return 1000000 / std::get<2>(blurredPixel) / 2;
    }

    return -1;
}

long Cluster::getID() {
  return id;
}

// overloading outstream operator to print info in csv format
std::ostream& operator<<(std::ostream& out, const Cluster& src) {
    out << src.x << "," << src.y << "," << src.radius << "," << src.vel_x << "," << src.vel_y << ", ";
    return out;
}

bool Cluster::operator==(const Cluster& comp) {
    return id == comp.id;
}
