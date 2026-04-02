#pragma once

#include "Modes/FunctionModeBase.h"

class PlayMode : public FunctionModeBase
{
  public:
    PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
             IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate);

    void setSelectedHomeProgramChange(byte selectedHomeProgramChange);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void setTransitionValue(byte transitionValue) override;

  private:
    void executeAction(ActionType action, byte actionValue);
    void updateVisuals();
    uint8_t ringBrightnessForButton(byte number) const override;
    void setupFunctions();

  private:
    byte _selectedHomeProgramChange;
    byte _selectedButton;
};
