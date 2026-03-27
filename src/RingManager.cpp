#include "RingManager.h"

RingManager::RingManager(Adafruit_NeoPixel& strip)
    : _strip(strip), _masterBrightness(DefaultBrightness), _selectedRing(0), _ring1(strip, 0, LedsPerRing),
      _ring2(strip, LedsPerRing, LedsPerRing), _ring3(strip, LedsPerRing * 2, LedsPerRing),
      _ring4(strip, LedsPerRing * 3, LedsPerRing), _ring5(strip, LedsPerRing * 4, LedsPerRing),
      _ring6(strip, LedsPerRing * 5, LedsPerRing), _ring7(strip, LedsPerRing * 6, LedsPerRing),
      _ring8(strip, LedsPerRing * 7, LedsPerRing),
      _rings{&_ring1, &_ring2, &_ring3, &_ring4, &_ring5, &_ring6, &_ring7, &_ring8}
{
    if (_masterBrightness > MaxBrightness)
    {
        _masterBrightness = MaxBrightness;
    }

    applyBrightness();
}

void RingManager::setRingColour(uint8_t ringIndex, uint32_t colour)
{
    if (ringIndex >= RingCount)
    {
        return;
    }

    _rings[ringIndex]->setColour(colour);
}

void RingManager::selectRing(uint8_t ringIndex)
{
    if (ringIndex >= RingCount)
    {
        return;
    }

    _selectedRing = ringIndex;
    applyBrightness();
}

bool RingManager::adjustMasterBrightness(int16_t steps, uint8_t stepSize)
{
    if (steps == 0 || stepSize == 0)
    {
        return false;
    }

    const int32_t requested = (int32_t)_masterBrightness + ((int32_t)steps * (int32_t)stepSize);
    int32_t clamped = requested;
    if (clamped < 0)
    {
        clamped = 0;
    }
    else if (clamped > MaxBrightness)
    {
        clamped = MaxBrightness;
    }

    const uint8_t nextBrightness = (uint8_t)clamped;
    if (nextBrightness == _masterBrightness)
    {
        return false;
    }

    _masterBrightness = nextBrightness;
    applyBrightness();
    return true;
}

void RingManager::setMasterBrightness(uint8_t brightness)
{
    uint8_t nextBrightness = brightness;
    if (nextBrightness > MaxBrightness)
    {
        nextBrightness = MaxBrightness;
    }

    if (nextBrightness == _masterBrightness)
    {
        return;
    }

    _masterBrightness = nextBrightness;
    applyBrightness();
}

uint8_t RingManager::masterBrightness() const { return _masterBrightness; }

void RingManager::applyBrightness()
{
    for (int i = 0; i < RingCount; ++i)
    {
        const uint8_t baseBrightness = (i == _selectedRing) ? FullBrightness : DimBrightness;
        const uint8_t scaledBrightness = (uint8_t)(((uint16_t)baseBrightness * (uint16_t)_masterBrightness) / 255U);
        _rings[i]->setBrightness(scaledBrightness);
    }
}
