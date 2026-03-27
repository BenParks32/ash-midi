#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "Lights.h"

class RingManager
{
  public:
    static constexpr uint8_t RingCount = 8;
    static constexpr uint8_t LedsPerRing = 8;
    static constexpr uint16_t LedCount = RingCount * LedsPerRing;

    static constexpr uint8_t DimBrightness = 5;
    static constexpr uint8_t FullBrightness = 200;
    static constexpr uint8_t DefaultBrightness = 200;
    static constexpr uint8_t MaxBrightness = 220;

  public:
    explicit RingManager(Adafruit_NeoPixel& strip);

  private:
    RingManager() = delete;
    RingManager(const RingManager&) = delete;
    RingManager& operator=(const RingManager&) = delete;

  public:
    void setRingColour(uint8_t ringIndex, uint32_t colour);
    void selectRing(uint8_t ringIndex);

    bool adjustMasterBrightness(int16_t steps, uint8_t stepSize);
    void setMasterBrightness(uint8_t brightness);
    uint8_t masterBrightness() const;

  private:
    void applyBrightness();

  private:
    Adafruit_NeoPixel& _strip;

    uint8_t _masterBrightness;
    uint8_t _selectedRing;

    RingLight _ring1;
    RingLight _ring2;
    RingLight _ring3;
    RingLight _ring4;
    RingLight _ring5;
    RingLight _ring6;
    RingLight _ring7;
    RingLight _ring8;

    RingLight* _rings[RingCount];
};
