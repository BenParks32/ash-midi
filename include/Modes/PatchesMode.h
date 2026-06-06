#pragma once

#include "ButtonOverrideStore.h"
#include "Modes/FunctionModeBase.h"
#include "MidiProvider.h"

class PatchesMode : public FunctionModeBase
{
  public:
    PatchesMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
                IModeTransistionDelegate& transitionDelegate);

    void activate() override;
    void deactivate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void setTransitionValue(ModeTransitionValue transitionValue) override;

  private:
    static constexpr byte InvalidPatchNumber = 0xFF;

  private:
    void resetButtons();
    void populateButtons();
    void configureButton(byte buttonIndex, byte patchNumber, const char* label);
    void configureBackButton();
    void renderCenterTitle(uint16_t textColour);
    static void formatPatchLabel(byte patchNumber, const char* patchName, char* buffer, size_t bufferSize);
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    IMidiProvider& _midiProvider;
    IButtonOverrideStore& _buttonOverrideStore;
    byte _selectedPlaylist;
    byte _currentPatch;
    bool _returnToPlaySet;
    byte _buttonPatchNumbers[TouchButtonManager::BUTTON_COUNT];
};
