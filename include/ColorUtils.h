#pragma once

#include <Arduino.h>

class ColorUtils
{
  public:
    static uint32_t rgb565To888(uint16_t rgb565)
    {
        const uint8_t r5 = (uint8_t)((rgb565 >> 11) & 0x1FU);
        const uint8_t g6 = (uint8_t)((rgb565 >> 5) & 0x3FU);
        const uint8_t b5 = (uint8_t)(rgb565 & 0x1FU);

        const uint8_t r8 = (uint8_t)(((uint16_t)r5 * 255U + 15U) / 31U);
        const uint8_t g8 = (uint8_t)(((uint16_t)g6 * 255U + 31U) / 63U);
        const uint8_t b8 = (uint8_t)(((uint16_t)b5 * 255U + 15U) / 31U);

        return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | b8;
    }
};