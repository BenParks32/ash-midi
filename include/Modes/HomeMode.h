#pragma once

#include "Modes/Mode.h"

#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButtonManager.h"

class HomeMode : public IMode
{
  public:
    HomeMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;

  private:
    const char* functionLabel(byte number) const;
    bool isButtonEnabled(byte number) const;
    static bool isEmptyLabel(const char* label);

  private:
    TouchButtonManager& _touchButtonManager;
    RingManager& _ringManager;
    ScreenUi& _screenUi;
};
