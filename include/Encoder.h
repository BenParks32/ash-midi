#pragma once

#include <Arduino.h>

class RotaryEncoder
{
  public:
    RotaryEncoder(uint8_t pinA, uint8_t pinB);

  private:
    RotaryEncoder() = delete;
    RotaryEncoder(const RotaryEncoder&) = delete;
    RotaryEncoder& operator=(const RotaryEncoder&) = delete;

  public:
    bool begin();
    int16_t consumeSteps();

  private:
    void handleInterrupt();

    static void isrA();
    static void isrB();

  private:
    const uint8_t _pinA;
    const uint8_t _pinB;
    volatile uint8_t _previousState;
    volatile int8_t _pulseAccumulator;
    volatile int16_t _pendingSteps;

    static RotaryEncoder* s_active;
};
