#pragma once

#include <cstdint>
#include <vector>

#ifndef NEO_GRB
#define NEO_GRB 0x00
#endif

#ifndef NEO_KHZ800
#define NEO_KHZ800 0x00
#endif

class Adafruit_NeoPixel
{
  public:
    Adafruit_NeoPixel(std::uint16_t count = 0, std::int16_t = 0, std::uint8_t = 0)
        : _pixels(count, 0), _brightness(255)
    {
    }

    void begin() {}
    void show() {}
    void setBrightness(std::uint8_t brightness) { _brightness = brightness; }
    void setPixelColor(std::uint16_t index, std::uint32_t colour)
    {
        if (index < _pixels.size())
        {
            _pixels[index] = colour;
        }
    }
    std::uint32_t getPixelColor(std::uint16_t index) const
    {
        return (index < _pixels.size()) ? _pixels[index] : 0U;
    }
    void clear()
    {
        for (std::size_t i = 0; i < _pixels.size(); ++i)
        {
            _pixels[i] = 0U;
        }
    }
    std::uint32_t Color(std::uint8_t r, std::uint8_t g, std::uint8_t b)
    {
        return (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) |
               static_cast<std::uint32_t>(b);
    }

  private:
    std::vector<std::uint32_t> _pixels;
    std::uint8_t _brightness;
};
