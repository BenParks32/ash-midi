#pragma once
#include "Gfx.h"
#include "Resources.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

class ITouchButtonDelegate
{
  public:
    virtual void buttonPressed(const byte number) = 0;
};

class TouchButton
{
  public:
    TouchButton(const byte number, const Point location, const Size size, ITouchButtonDelegate& delegate);

  public:
    bool handleTouch(const uint16_t x, const uint16_t y);
    virtual void draw(TFT_eSPI& tft) = 0;

  private:
    TouchButton() = default;
    TouchButton(const TouchButton&) = default;
    TouchButton& operator=(const TouchButton&) = default;

  protected:
    const byte number;
    const Point location;
    const Size size;
    ITouchButtonDelegate& delegate;
};

class FootSwitchTouchButton : public TouchButton
{
  public:
    FootSwitchTouchButton(const byte number, const Point location, const Size size, const Resources& resources,
                          ITouchButtonDelegate& delegate);

    void draw(TFT_eSPI& tft);

  private:
    FootSwitchTouchButton() = default;
    FootSwitchTouchButton(const FootSwitchTouchButton&) = default;
    FootSwitchTouchButton& operator=(const FootSwitchTouchButton&) = default;

  private:
    const Resources& _resources;
};