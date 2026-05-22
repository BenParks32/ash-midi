#pragma once

#include "Modes/FunctionModeBase.h"
#include "ButtonOverrideStore.h"
#include "MidiProvider.h"

class PlayMode : public FunctionModeBase
{
  public:
    PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
             IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
             IModeTransistionDelegate& transitionDelegate);

    void setSelectedPreset(byte selectedPreset);

    void activate() override;
    void deactivate() override;
    void buttonDown(byte number) override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void buttonReleased(byte number) override;
    void frameTick() override;
    void setTransitionValue(ModeTransitionValue transitionValue) override;

  private:
    struct PatchBadgeMetrics
    {
        int32_t frameCenterX;
        int32_t frameTopY;
        int32_t titleY;
    };

  private:
    void executeAction(const FunctionAction& action);
    void registerTapTempoPress(uint32_t pressedAtMs);
    void renderTapTempoPill();
    void renderPlayCenterUi();
    void clearPlayCenterUi();
    void renderPatchBadge();
    void clearPatchBadge();
    void renderPatchBadgeNumber(byte patchNumber, uint16_t textColour);
    void renderSnapshotLabel(byte snapshotButton, uint16_t textColour);
    static void formatPatchNumberLabel(byte patchNumber, char* buffer, size_t bufferSize);
    void formatSnapshotLabelUppercase(byte snapshotButton, char* buffer, size_t bufferSize) const;
    PatchBadgeMetrics patchBadgeMetrics() const;
    int32_t patchBadgeFrameCenterX() const;
    int32_t patchBadgeFrameTopY() const;
    void renderButton(byte number);
    void updateSnapshotSelectionVisuals(byte previousSelected, byte currentSelected);
    void updateVisuals();
    int8_t firstSceneSelectionButton() const;
    bool isSceneSelectionButton(byte number) const;
    bool usesSelectionBorder(byte number) const override;
    uint8_t ringBrightnessForButton(byte number) const override;
    void setupFunctions();

  private:
    IMidiProvider& _midiProvider;
    IButtonOverrideStore& _buttonOverrideStore;
    byte _selectedPreset;
    byte _selectedPlaylist;
    bool _hasSelectedPreset;
    byte _selectedButton;
    uint32_t _tapTempoPressTimes[3];
    uint8_t _tapTempoPressCount;
    uint32_t _tapTempoFlashHalfPeriodMs;
    uint32_t _nextTapTempoFlashToggleMs;
    uint32_t _tapTempoFlashUntilMs;
    bool _hasTapTempoFlashInterval;
    bool _isTapTempoLit;
};
