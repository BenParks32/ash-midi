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
using ModeTransitionValue = uint16_t;
static constexpr ModeTransitionValue ModeTransitionNone = 0xFFFF;
static constexpr ModeTransitionValue ModeTransitionPatchReturnFlag = 0x0100;
static constexpr ModeTransitionValue ModeTransitionHomePlaylistFlag = 0x0200;
static constexpr ModeTransitionValue ModeTransitionPatchValueMask = 0x00FF;
static constexpr ModeTransitionValue ModeTransitionHomePlaylistValueMask = 0x00FF;

class IModeTransistionDelegate
{
  public:
    virtual ~IModeTransistionDelegate() = default;
    virtual void enterMode(Modes mode, ModeTransitionValue transitionValue) = 0;
};

class IMode
{
  public:
    virtual ~IMode() = default;

    virtual void activate() = 0;
    virtual void deactivate() {}
    virtual void buttonDown(const byte number) { (void)number; }
    virtual void buttonPressed(const byte number) = 0;
    virtual void buttonLongPressed(const byte number) = 0;
    virtual void buttonReleased(const byte number) { (void)number; }
    virtual void frameTick() = 0;
    virtual void setTransitionValue(ModeTransitionValue transitionValue) { (void)transitionValue; }
    virtual void encoderRotated(int16_t steps) { (void)steps; }
    virtual void encoderPressed() {}
};