#pragma once

#include "Modes/FunctionModeBase.h"

class ButtonDiagnosticMode : public FunctionModeBase
{
  public:
    ButtonDiagnosticMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                         IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate);

    void activate() override;
    void deactivate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void encoderPressed() override;

  private:
    enum class ButtonState : uint8_t
    {
        Off = 0,
        ShortPress = 1,
        LongPress = 2,
    };

  private:
    void setupFunctions();
    void resetStates();
    void renderCenterUi(uint16_t textColour);
    void setButtonState(byte number, ButtonState state);
    void applyButtonVisual(byte number);
    uint16_t colourForState(ButtonState state) const;
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    ButtonState _buttonStates[TouchButtonManager::BUTTON_COUNT];
};
