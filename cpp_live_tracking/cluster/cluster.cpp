#include "cluster.hpp"

int Cluster::globId = 0;

Cluster::Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha) {
    this->alpha = alpha;
    this->x = x;
    this->y = y;
    this->prev_x = x;
    this->prev_y = y;
    this->color = color;
    this->id = globId++;
}

double Cluster::distance(unsigned int x, unsigned int y) {
    return pow(pow(x - this->x, 2) + pow(y - this->y, 2), 0.5);
}

double Cluster::inRange(unsigned int x, unsigned int y) {
    return distance(x, y) < radius;
}

bool Cluster::borderRange(unsigned int x, unsigned int y) {
    return distance(x, y) < radius * 1.33;
}

bool Cluster::otherClusterRange(unsigned int x, unsigned int y) {
    return distance(x, y) < radius * 2;
}

void Cluster::shift(unsigned int x, unsigned int y) {
    this->x = (1 - alpha) * this->x + alpha * x;
    this->y = (1 - alpha) * this->y + alpha * y;
}

void Cluster::contMomentum(int64_t eventT, int64_t prevT) {
    x = vel_x * (eventT, - prevT);
    y = vel_y * (eventT -  prevT);
}

void Cluster::updateVelocity(unsigned int delay) {
    vel_x = (x - prev_x) / delay;
    vel_y = (y - prev_x) / delay;

    x = prev_x;
    y = prev_y;
}

void Cluster::updateRadius(float growthFactor) {
    radius *= growthFactor;
}

bool Cluster::aboveThreshold(unsigned int threshold) {
    return eventCount >= threshold;
}

void Cluster::newEvent() {
    eventCount++;
}

void Cluster::resetEvents() {
    eventCount = 0;
}

void Cluster::draw(cv::Mat img) {
    cv::circle(img, cv::Point(x, y), radius, color);
}

// overloading outstream operator to print info in csv format
std::ostream& operator<<(std::ostream& out, const Cluster& src) {
    out << src.x << "," << src.y << "," << src.radius << "," << src.vel_x << "," << src.vel_y << ",";
    return out;
}

bool Cluster::operator==(const Cluster& comp) {
    return id == comp.id;
}