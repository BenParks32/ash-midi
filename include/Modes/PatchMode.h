#pragma once

#include "Modes/FunctionModeBase.h"

class PatchMode : public FunctionModeBase
{
  public:
    PatchMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
              IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void setTransitionValue(byte transitionValue) override;

  private:
    void setupFunctions();
    void renderPlayButton();
    void renderPatchSelector();
    void renderPatchNumber(byte patchNumber, uint16_t textColour);
    int32_t patchFrameTopY() const;
    int32_t patchNumberY() const;
    static void formatPatchLabel(byte patchNumber, char* buffer, size_t bufferSize);
    void executeAction(ActionType action, byte actionValue);
    void changePatch(int8_t delta);
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    byte _currentPatch;
    bool _isPlayButtonLit;
    uint32_t _nextPlayFlashToggleMs;
};