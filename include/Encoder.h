#pragma once

#include <Arduino.h>

class RotaryEncoder
{
  public:
    static constexpr uint8_t NoPin = 0xFF;

    RotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t pushPin = NoPin);

  private:
    RotaryEncoder() = delete;
    RotaryEncoder(const RotaryEncoder&) = delete;
    RotaryEncoder& operator=(const RotaryEncoder&) = delete;

  public:
    bool begin();
    int16_t consumeSteps();
    bool consumeButtonPress();

  private:
    void handleInterrupt();
    void updatePushButtonState();

    static void isrA();
    static void isrB();

  private:
    const uint8_t _pinA;
    const uint8_t _pinB;
    const uint8_t _pushPin;
    volatile uint8_t _previousState;
    volatile int8_t _pulseAccumulator;
    volatile int16_t _pendingSteps;
    bool _lastPushReading;
    bool _stablePushState;
    uint32_t _lastPushDebounceMs;
    uint8_t _pendingButtonPresses;

    static RotaryEncoder* s_active;
};
