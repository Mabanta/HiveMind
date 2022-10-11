#include "cluster.hpp"
#include <fftw3.h>

int Cluster::globId = 0;

Cluster::Cluster(unsigned int x, unsigned int y, cv::viz::Color color, float alpha,
                 unsigned int sampleFreq, unsigned int numPositions) {
    this->alpha = alpha;
    this->x = (double)x;
    this->y = (double)y;
    this->prev_x = (double)x;
    this->prev_y = (double)y;
    this->color = color;
    this->id = globId++;
    this->sampleFreq = sampleFreq;
    this->numPositions = numPositions;

    this->polarity_history = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numPositions);
    this->freq_spectrum = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numPositions);
    this->plan = fftw_plan_dft_1d(numPositions, polarity_history, freq_spectrum, FFTW_FORWARD, FFTW_ESTIMATE);
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
    radius *= growthFactor *((40 - radius) / 15);
}

bool Cluster::aboveThreshold(unsigned int threshold) {
    return eventCount >= threshold;
}

void Cluster::newEvent(bool polarity) {
    if (polarity) posCount ++;
    else {
      negCount ++;
      eventCount ++;
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
    if(!sideZero) return side;
  }
  return 0;
}

void Cluster::draw(cv::Mat img) {
    cv::circle(img, cv::Point(x, y), radius, color);
}

void Cluster::addHistory() {
  if (posIndex < numPositions) {
    if (posCount + negCount == 0) polarity_history[posIndex][0] = 0;
    else polarity_history[posIndex][0] = (double)(posCount - negCount) / (double)(posCount + negCount);

    polarity_history[posIndex][1] = 0;
    posIndex ++;

  } else {
    fft();
    posIndex = 0;
    newFrequency = true;
  }

  negCount = 0;
  posCount = 0;
}

void Cluster::fft() {
  fftw_execute(plan);
  int maxFreqIndex = 0;
  double maxMagnitude = 0;
  double magnitude;

  int minFrequency = (int)((100 * numPositions) / sampleFreq);
  int maxFrequency = (int)((400 * numPositions) / sampleFreq);

  for (int i = minFrequency; i < maxFrequency; i ++) {
    magnitude = freq_spectrum[i][0]*freq_spectrum[i][0] + freq_spectrum[i][1]*freq_spectrum[i][1];
    if (magnitude > maxMagnitude) {
      maxMagnitude = magnitude;
      maxFreqIndex = i;
    }
  }

  frequency = (double)maxFreqIndex * ((double)sampleFreq / (double)numPositions);
}

double Cluster::getFrequency() {
  if (newFrequency == true) {
    newFrequency = false;
    return frequency;
  }
  return -1;
}

// overloading outstream operator to print info in csv format
std::ostream& operator<<(std::ostream& out, const Cluster& src) {
    out << src.x << "," << src.y << "," << src.radius << "," << src.frequency << "," << src.newFrequency << ", ";
    return out;
}

bool Cluster::operator==(const Cluster& comp) {
    return id == comp.id;
}
