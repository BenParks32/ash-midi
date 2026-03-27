#include "Encoder.h"

RotaryEncoder* RotaryEncoder::s_active = nullptr;

RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB)
    : _pinA(pinA), _pinB(pinB), _previousState(0), _pulseAccumulator(0), _pendingSteps(0)
{
}

bool RotaryEncoder::begin()
{
    if (s_active != nullptr && s_active != this)
    {
        return false;
    }

    s_active = this;

    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    _previousState = ((digitalRead(_pinA) & 0x01) << 1) | (digitalRead(_pinB) & 0x01);
    _pulseAccumulator = 0;
    _pendingSteps = 0;

    attachInterrupt(digitalPinToInterrupt(_pinA), RotaryEncoder::isrA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_pinB), RotaryEncoder::isrB, CHANGE);
    return true;
}

int16_t RotaryEncoder::consumeSteps()
{
    int16_t steps = 0;
    noInterrupts();
    steps = _pendingSteps;
    _pendingSteps = 0;
    interrupts();
    return steps;
}

void RotaryEncoder::handleInterrupt()
{
    const uint8_t currentState = ((digitalRead(_pinA) & 0x01) << 1) | (digitalRead(_pinB) & 0x01);
    const uint8_t previousState = _previousState;
    if (currentState == previousState)
    {
        return;
    }

    const uint8_t transition = (previousState << 2) | currentState;
    _previousState = currentState;

    int8_t delta = 0;
    switch (transition)
    {
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
        delta = 1;
        break;

    case 0b0010:
    case 0b1011:
    case 0b1101:
    case 0b0100:
        delta = -1;
        break;

    default:
        // Invalid jump (contact bounce or missed edge), restart detent accumulation.
        _pulseAccumulator = 0;
        return;
    }

    _pulseAccumulator += delta;
    if (_pulseAccumulator >= 4)
    {
        _pulseAccumulator = 0;
        _pendingSteps++;
        return;
    }

    if (_pulseAccumulator <= -4)
    {
        _pulseAccumulator = 0;
        _pendingSteps--;
        return;
    }
}

void RotaryEncoder::isrA()
{
    if (s_active != nullptr)
    {
        s_active->handleInterrupt();
    }
}

void RotaryEncoder::isrB()
{
    if (s_active != nullptr)
    {
        s_active->handleInterrupt();
    }
}
