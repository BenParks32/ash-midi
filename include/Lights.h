#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class ILight
{
    public:
        virtual void setColour(uint32_t colour) = 0;
        virtual void setBrightness(byte brightness) = 0;
        virtual void clear() = 0;
};

class RingLight : public ILight
{
    public:
        RingLight(Adafruit_NeoPixel& strip, const byte number, const byte count);
        void setColour(uint32_t colour) override;
        void setBrightness(byte brightness) override;
        void clear() override;

    private:
        RingLight() = default;
        RingLight(const RingLight& rhs) = default;

    private:
        void floodFill();

    private:
        Adafruit_NeoPixel& _strip;
        const byte _number;
        const byte _count;
        uint32_t _currentColour;
        byte _brightness;
};