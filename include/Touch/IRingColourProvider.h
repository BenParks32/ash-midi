#pragma once

#include <Arduino.h>

class IRingColourProvider
{
  public:
    virtual ~IRingColourProvider() = default;
    virtual uint32_t defaultRingColour(uint8_t ringIndex) const = 0;
};
