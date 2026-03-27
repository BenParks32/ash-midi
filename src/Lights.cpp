#include "Lights.h"

RingLight::RingLight(Adafruit_NeoPixel& strip, const byte number, const byte count)
    : _strip(strip), _number(number), _count(count), _currentColour(0), _brightness(255)
{
}

void RingLight::setColour(uint32_t colour)
{
    _currentColour = colour;
    floodFill();
}

void RingLight::clear()
{
    _currentColour = 0;
    floodFill();
}

void RingLight::setBrightness(byte brightness)
{
    _brightness = brightness;
    floodFill();
}

void RingLight::floodFill()
{
    const uint8_t sourceR = (uint8_t)(_currentColour >> 16);
    const uint8_t sourceG = (uint8_t)(_currentColour >> 8);
    const uint8_t sourceB = (uint8_t)(_currentColour);

    // Linear channel scaling preserves color ratios while dimming.
    const uint8_t r = (uint8_t)((((uint16_t)sourceR * (uint16_t)_brightness) + 127U) / 255U);
    const uint8_t g = (uint8_t)((((uint16_t)sourceG * (uint16_t)_brightness) + 127U) / 255U);
    const uint8_t b = (uint8_t)((((uint16_t)sourceB * (uint16_t)_brightness) + 127U) / 255U);

    const uint32_t gammaColour = _strip.Color(r, g, b);
    for (int led = _number; led < _number + _count; led++)
    {
        _strip.setPixelColor(led, gammaColour);
    }
}