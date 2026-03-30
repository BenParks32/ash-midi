#pragma once
#include <Arduino.h>

class IMode
{
  public:
    virtual void activate() = 0;
    virtual void buttonPressed(const byte number) = 0;
    virtual void buttonLongPressed(const byte number) = 0;
    virtual void frameTick() = 0;
};