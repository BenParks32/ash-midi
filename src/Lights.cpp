#include "Lights.h"

RingLight::RingLight(Adafruit_NeoPixel& strip, const byte number, const byte count) :
    _strip(strip),
    _number(number),
    _count(count),
    _currentColour(0)
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
    uint8_t r = (uint8_t)(_currentColour >> 16);
    uint8_t g = (uint8_t)(_currentColour >> 8);
    uint8_t b = (uint8_t)(_currentColour);

    r = (r * _brightness) / 255;
    g = (g * _brightness) / 255;
    b = (b * _brightness) / 255;

    uint32_t gammaColour = _strip.gamma32(_strip.Color(r, g, b));
    for (int led = _number; led < _number + _count; led++)
    {
        _strip.setPixelColor(led, gammaColour);
    }
}