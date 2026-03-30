#pragma once

#include <Arduino.h>

#include "Button.h"
#include "TouchButton.h"

class IMode;
class RingManager;

class ButtonHandler : public IButtonDelegate, public ITouchButtonDelegate
{
  public:
    ButtonHandler(IMode*& activeMode, RingManager& ringManager, void (*selectTouchButton)(byte));

    void buttonPressed(const byte number) override;
    void buttonLongPressed(const byte number) override;

  private:
    IMode*& _activeMode;
    RingManager& _ringManager;
    void (*_selectTouchButton)(byte);
};
