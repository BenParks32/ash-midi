#pragma once

#include "Modes/FunctionModeBase.h"
#include "MidiProvider.h"

class PatchMode : public FunctionModeBase
{
  public:
    PatchMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
              IMidiManager& midiManager, IMidiProvider& midiProvider, IModeTransistionDelegate& transitionDelegate);

    void activate() override;
    void deactivate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void setTransitionValue(ModeTransitionValue transitionValue) override;

  private:
    void setupFunctions();
    void renderPlayButton();
    void renderPatchSelector();
    void clearPatchSelector();
    void renderPatchNumber(byte patchNumber, uint16_t textColour);
    int32_t patchFrameTopY() const;
    int32_t patchNumberY() const;
    static void formatPatchLabel(byte patchNumber, char* buffer, size_t bufferSize);
    void executeAction(const FunctionAction& action);
    void changePatch(int8_t delta);
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    IMidiProvider& _midiProvider;
    byte _currentPatch;
    bool _isPlayButtonLit;
    uint32_t _nextPlayFlashToggleMs;
};