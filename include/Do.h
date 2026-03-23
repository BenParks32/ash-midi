#pragma once
#include <Arduino.h>

class Every
{
  public:
    Every(const unsigned long interval, std::function<void()> func);

  private:
    Every() = default;
    Every(const Every&) = default;
    Every& operator=(const Every&) = default;

  private:
    unsigned long interval;
    unsigned long lastTime;
    std::function<void()> func;

  public:
    void tick();
};
