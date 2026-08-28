#pragma once
#include "Arduino.h"
struct Stepper {
  long netSteps = 0;
  unsigned int calls = 0;
  Stepper(int,int,int,int,int) {}
  void setSpeed(int) {}
  void step(int steps) { netSteps += steps; calls++; delay(std::abs(steps) * 3UL); }
};

