#include "TapTempoEngine.h"

TapTempoEngine::TapTempoEngine()
    : _pressTimes{0U, 0U, 0U}, _pressCount(0U), _intervalMs(0U), _flashHalfPeriodMs(0U), _nextMidiSendMs(0U),
      _inactivityDeadlineMs(0U), _trailingMessagesRemaining(0U), _hasFlashInterval(false)
{
}

void TapTempoEngine::clear()
{
    _pressTimes[0] = 0U;
    _pressTimes[1] = 0U;
    _pressTimes[2] = 0U;
    _pressCount = 0U;
    _intervalMs = 0U;
    _flashHalfPeriodMs = 0U;
    _nextMidiSendMs = 0U;
    _inactivityDeadlineMs = 0U;
    _trailingMessagesRemaining = 0U;
    _hasFlashInterval = false;
}

void TapTempoEngine::registerPress(uint32_t pressedAtMs)
{
    const uint32_t previousNextMidiSendMs = _nextMidiSendMs;

    if (_pressCount > 0U)
    {
        const uint32_t lastPressedAtMs = _pressTimes[_pressCount - 1U];
        if ((pressedAtMs - lastPressedAtMs) >= InactivityTimeoutMs)
        {
            clear();
        }
    }

    if (_pressCount < 3U)
    {
        _pressTimes[_pressCount++] = pressedAtMs;
    }
    else
    {
        _pressTimes[0] = _pressTimes[1];
        _pressTimes[1] = _pressTimes[2];
        _pressTimes[2] = pressedAtMs;
    }

    _intervalMs = 0U;
    if (_pressCount >= 2U)
    {
        uint32_t intervalTotalMs = 0U;
        for (uint8_t i = 1U; i < _pressCount; ++i)
        {
            intervalTotalMs += (_pressTimes[i] - _pressTimes[i - 1U]);
        }

        const uint32_t averageIntervalMs = intervalTotalMs / static_cast<uint32_t>(_pressCount - 1U);
        _intervalMs = (averageIntervalMs > 0U) ? averageIntervalMs : 1U;
        _flashHalfPeriodMs = (averageIntervalMs > 1U) ? (averageIntervalMs / 2U) : 1U;
        _hasFlashInterval = true;
    }
    else
    {
        _flashHalfPeriodMs = 0U;
        _hasFlashInterval = false;
    }

    _inactivityDeadlineMs = 0U;
    _trailingMessagesRemaining = 0U;
    if (_pressCount >= 3U)
    {
        const uint32_t candidateNextMidiSendMs = pressedAtMs + _intervalMs;
        if (previousNextMidiSendMs == 0U ||
            static_cast<int32_t>(candidateNextMidiSendMs - previousNextMidiSendMs) < 0)
        {
            _nextMidiSendMs = candidateNextMidiSendMs;
        }
        else
        {
            _nextMidiSendMs = previousNextMidiSendMs;
        }

        _inactivityDeadlineMs = pressedAtMs + InactivityTimeoutMs;
    }
    else
    {
        _nextMidiSendMs = 0U;
    }
}

TapTempoEngine::TickResult TapTempoEngine::tick(uint32_t nowMs)
{
    TickResult result;

    if (_nextMidiSendMs == 0U)
    {
        return result;
    }

    if (_inactivityDeadlineMs != 0U && static_cast<int32_t>(nowMs - _inactivityDeadlineMs) >= 0)
    {
        _inactivityDeadlineMs = 0U;
        _trailingMessagesRemaining = TrailingMessageCount;
    }

    if (static_cast<int32_t>(nowMs - _nextMidiSendMs) < 0)
    {
        return result;
    }

    result.shouldSendMidi = true;
    _nextMidiSendMs += _intervalMs;

    if (_inactivityDeadlineMs == 0U && _trailingMessagesRemaining > 0U)
    {
        --_trailingMessagesRemaining;
        if (_trailingMessagesRemaining == 0U)
        {
            _nextMidiSendMs = 0U;
        }
    }

    return result;
}

bool TapTempoEngine::hasFlashInterval() const { return _hasFlashInterval; }

uint32_t TapTempoEngine::flashHalfPeriodMs() const { return _flashHalfPeriodMs; }
