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

    /*
    if (distance(event.x(), event.y()) > 3) {
        return;
    }
    */

    if (!event.polarity()) {
        surfaceVal *= pow(0.1, (event.timestamp() - prevEvent));
        prevEvent = event.timestamp();

        surfaceVal += 1;

        if (surfaceVal >= surfaceThresh) {
            //std::cout << surfaceVal << ", " << event.timestamp() - prevTime << std::endl;
            if (event.timestamp() - prevTime > 100) {
                if (prevTime != -1 && event.timestamp() - prevTime < 10000) {
                    runningAvg = (7 * runningAvg + event.timestamp() - prevTime) / 8.0;
                    samples++;
                } else if (event.timestamp() - prevTime > 200000) {
                    samples = 0;
                    runningAvg = 0;
                }

                prevTime = event.timestamp();
                
            }
        }

        
    }
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
    
    if (samples >= 8) {
        return 1000000 / runningAvg;
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
