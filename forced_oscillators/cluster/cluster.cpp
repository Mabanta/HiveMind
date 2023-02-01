#include "cluster.hpp"

int Cluster::globId = 0;
const double pi = 3.14159;

Cluster::Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha, int64_t time) {
    this->alpha = alpha;
    this->x = (double)x;
    this->y = (double)y;
    this->prev_x = (double)x;
    this->prev_y = (double)y;
    this->color = color;
    this->id = globId++;
    this->startTime = time;
    for (int i = 0; i < num_oscillators; i ++) {
      this->A[i] = 0;
      this->phi[i] = 0;
      this->omega[i] = omega_min + 5*i;
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

void Cluster::update_osc(int64_t timestamp, double time_constant) {
    double t_i = ((double)(timestamp - startTime))/1000000;
    double A_i  = exp(time_constant*t_i);

    for (int w = 0; w < num_oscillators; w ++) {
      double phi_i = 2*pi*omega[w]*t_i + pi/2;
      A[w] = sqrt(A[w]*A[w]+A_i*A_i+2*A[w]*A_i*cos(phi[w]-phi_i));
      phi[w] = atan((A[w]*sin(phi[w])+A_i*sin(phi_i))/(A[w]*cos(phi[w])+A_i*cos(phi_i)));
    }
}

int Cluster::getFrequency() {
    double max = 0;
    int maxFreq = 0;
    for (int w = 0; w < num_oscillators; w++) {
      if (A[w] > max) {
        max = A[w];
        maxFreq = w;
      }
    }

    if (max > 0) return omega[maxFreq];
    return -1;
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
