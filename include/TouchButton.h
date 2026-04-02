#pragma once
#include "Gfx.h"
#include "Touch/TouchButtonUi.h"
#include <Arduino.h>

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
    byte buttonNumber() const;
    virtual void draw(ITouchButtonCanvas& ui) = 0;

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
    FootSwitchTouchButton(const byte number, const Point location, const Size size, const char* label,
                          uint16_t pillColour, ITouchButtonDelegate& delegate);

    bool handleTouch(const uint16_t x, const uint16_t y);
    void draw(ITouchButtonCanvas& ui);
    void setLabel(const char* label);
    const char* label() const;

    void setPillColour(uint16_t pillColour);
    uint16_t pillColour() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setBorderVisible(bool visible);
    bool hasBorder() const;

  private:
    FootSwitchTouchButton() = default;
    FootSwitchTouchButton(const FootSwitchTouchButton&) = default;
    FootSwitchTouchButton& operator=(const FootSwitchTouchButton&) = default;

  private:
    static constexpr size_t LabelCapacity = 16;
    char _label[LabelCapacity];
    uint16_t _pillColour;
    bool _hasBorder;
    bool _isEnabled;
};