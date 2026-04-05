#pragma once

#include "Modes/FunctionModeBase.h"

class PlayMode : public FunctionModeBase
{
  public:
    PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
             IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate);

    void setSelectedHomeProgramChange(byte selectedHomeProgramChange);

    void activate() override;
    void deactivate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void setTransitionValue(byte transitionValue) override;

  private:
    void executeAction(ActionType action, byte actionValue);
    void renderPlayCenterUi();
    void clearPlayCenterUi();
    void renderPatchBadge();
    void clearPatchBadge();
    void renderPatchBadgeNumber(byte patchNumber, uint16_t textColour);
    void renderSnapshotLabel(byte snapshotButton, uint16_t textColour);
    static void formatPatchNumberLabel(byte patchNumber, char* buffer, size_t bufferSize);
    void formatSnapshotLabelUppercase(byte snapshotButton, char* buffer, size_t bufferSize) const;
    int32_t patchBadgeFrameCenterX() const;
    int32_t patchBadgeFrameTopY() const;
    void renderButton(byte number);
    void updateSnapshotSelectionVisuals(byte previousSelected, byte currentSelected);
    void updateVisuals();
    bool usesSelectionBorder(byte number) const override;
    uint8_t ringBrightnessForButton(byte number) const override;
    void setupFunctions();

  private:
    byte _selectedHomeProgramChange;
    bool _hasSelectedHomeProgramChange;
    byte _selectedButton;
};
