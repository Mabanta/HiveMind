#include "cluster.hpp"

int Cluster::globId = 0;

Cluster::Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha) {
    this->alpha = alpha;
    this->x = (double)x;
    this->y = (double)y;
    this->prev_x = (double)x;
    this->prev_y = (double)y;
    this->color = color;
    this->id = globId++;
}

Cluster::Cluster(unsigned int x, unsigned int y, float alpha) {
    this->alpha = alpha;
    this->x = (double)x;
    this->y = (double)y;
    this->prev_x = (double)x;
    this->prev_y = (double)y;
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

bool Cluster::aboveThreshold(unsigned int threshold, unsigned int width, unsigned int height) {
    return eventCount >= threshold && x >= 0 && y >= 0 && x <= width && y <= height;
}

void Cluster::newEvent() {
    eventCount++;
}


void Cluster::resetEvents() {
    eventCount = 0;
}

int Cluster::getSide(int width, int height) {

  double leftSide = (double)(width*0.4);
  double rightSide = (double)(width*0.9);
  double top = (double)(height*0.85);
  double bottom = (double)(height*0.15);

  if (x > (leftSide + 5) && x < (rightSide - 5) && y > (bottom + 5) && y < (top - 5))
    return 1;
  else if ((x < (leftSide - 5) || x > (rightSide + 5)) || (y < (bottom - 5) || y > (top + 5)))
    return -1;
  return 0;

  /*
  if (x < (double)(width/2 - 10))
    return -1;
  else if (x > (double)(width/2 + 10))
    return 1;
  return 0;*/
}

int Cluster::updateSide(int width, int height) {
  int newSide = getSide(width, height);
  if (newSide != side && newSide != 0) {
    bool sideZero = (side == 0);
    side = newSide;
    if(!sideZero)
      return side;
  }
  return 0;
}

void Cluster::draw(cv::Mat img) {
    cv::rectangle(
        img, 
        cv::Point(int(x - radius / 2), int(y - radius / 2)), 
        cv::Point(int(x + radius / 2), int(y + radius / 2)), 
        color);
}

float Cluster::getRadius() {
  return radius;
}
int Cluster::getX() {
  return x;
}
int Cluster::getY() {
  return y;
}
int Cluster::getID() {
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
