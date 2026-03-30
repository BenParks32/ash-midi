#pragma once

#include "Modes/Mode.h"

#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButton.h"

class HomeMode : public IMode
{
  public:
    HomeMode(FootSwitchTouchButton** touchButtons, byte touchButtonCount, RingManager& ringManager, ScreenUi& screenUi);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;

  private:
    const char* functionLabel(byte number) const;
    bool isButtonEnabled(byte number) const;
    static bool isEmptyLabel(const char* label);

  private:
    FootSwitchTouchButton** _touchButtons;
    byte _touchButtonCount;
    RingManager& _ringManager;
    ScreenUi& _screenUi;
};
