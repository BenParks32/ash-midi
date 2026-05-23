#pragma once

#include "Modes/FunctionModeBase.h"

class HomeMode : public FunctionModeBase
{
  public:
    HomeMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
             IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate);

    void activate() override;
    void deactivate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;

  private:
    void executeAction(const FunctionAction& action);
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    void setupFunctions();
};
