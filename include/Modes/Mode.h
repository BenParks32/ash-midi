#pragma once
#include <Arduino.h>

enum class Modes : uint8_t
{
    Home = 0,
    Play = 1,
    Patch = 2,
    Menu = 3,
  ButtonDiagnostic = 4,
    Count,
};

static constexpr uint8_t ModeCount = static_cast<uint8_t>(Modes::Count);
static constexpr byte ModeTransitionNone = 0xFF;
static constexpr byte ModeTransitionPatchReturnFlag = 0x80;
static constexpr byte ModeTransitionPatchValueMask = 0x3F;

class IModeTransistionDelegate
{
  public:
    virtual ~IModeTransistionDelegate() = default;
    virtual void enterMode(Modes mode, byte transitionValue) = 0;
};

class IMode
{
  public:
    virtual ~IMode() = default;

    virtual void activate() = 0;
    virtual void deactivate() {}
    virtual void buttonPressed(const byte number) = 0;
    virtual void buttonLongPressed(const byte number) = 0;
    virtual void frameTick() = 0;
    virtual void setTransitionValue(byte transitionValue) { (void)transitionValue; }
    virtual void encoderRotated(int16_t steps) { (void)steps; }
    virtual void encoderPressed() {}
};