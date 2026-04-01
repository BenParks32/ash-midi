#pragma once

#include <Arduino.h>

#include "Button.h"
#include "TouchButton.h"

class IMode;
class RingManager;
class ITouchButtonDelegate;

class ButtonHandler : public IButtonDelegate, public ITouchButtonDelegate
{
  public:
    static constexpr byte ButtonCount = 8;

    ButtonHandler(IMode*& activeMode, RingManager& ringManager, ITouchButtonDelegate* touchDelegate = nullptr);
    ~ButtonHandler();

    void begin();
    void updateButtons();

    void buttonPressed(const byte number) override;
    void buttonLongPressed(const byte number) override;

  private:
    IMode*& _activeMode;
    RingManager& _ringManager;
    ITouchButtonDelegate* _touchDelegate;
    Button* _buttons[ButtonCount];
};
