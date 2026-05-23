#pragma once

#include <cstdint>

class TapTempoEngine
{
  public:
    static constexpr uint32_t InactivityTimeoutMs = 3000U;
    static constexpr uint8_t TrailingMessageCount = 3U;

    struct TickResult
    {
        bool shouldSendMidi = false;
    };

    TapTempoEngine();

    void clear();
    void registerPress(uint32_t pressedAtMs);
    TickResult tick(uint32_t nowMs);

    bool hasFlashInterval() const;
    uint32_t flashHalfPeriodMs() const;
    uint32_t intervalMs() const;

  private:
    uint32_t _pressTimes[3];
    uint8_t _pressCount;
    uint32_t _intervalMs;
    uint32_t _flashHalfPeriodMs;
    uint32_t _nextMidiSendMs;
    uint32_t _inactivityDeadlineMs;
    uint8_t _trailingMessagesRemaining;
    bool _hasFlashInterval;
};
