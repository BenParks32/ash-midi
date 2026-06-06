#pragma once

#include "Modes/FunctionModeBase.h"
#include "ButtonOverrideStore.h"
#include "MidiProvider.h"
#include "SetListStore.h"
#include "TapTempoEngine.h"

class PlaySetMode : public FunctionModeBase
{
  public:
    PlaySetMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
             IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
             ISetListStore& setListStore, IModeTransistionDelegate& transitionDelegate);

    void setSelectedPreset(byte selectedPreset);

    void activate() override;
    void deactivate() override;
    void buttonDown(byte number) override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void buttonReleased(byte number) override;
    void frameTick() override;
    void encoderPressed() override;
    void setTransitionValue(ModeTransitionValue transitionValue) override;

  private:
    struct TrackedTextLabel
    {
        static constexpr size_t Capacity = 64;
        char text[Capacity];
    };

    struct PatchBadgeMetrics
    {
        int32_t frameCenterX;
        int32_t frameTopY;
        int32_t titleY;
    };

  private:
    void executeAction(const FunctionAction& action);
    void toggleAction(ActionType actionType);
    void registerTapTempoPress(uint32_t pressedAtMs);
    void clearTapTempoState();
    void renderTapTempoButtons(bool redrawLabel);
    void renderToggleButtons(ActionType actionType, bool redrawLabel);
    static void formatTapTempoBpmLabel(uint32_t intervalMs, char* buffer, size_t bufferSize);
    void renderPlayCenterUi();
    void clearPlayCenterUi();
    void renderPatchBadge();
    void clearPatchBadge();
    void renderPatchBadgeNumber(byte patchNumber, uint16_t textColour);
    void renderSongNameLabel(uint16_t textColour);
    void renderPatchNameLabel(uint16_t textColour);
    void renderSnapshotLabel(byte snapshotButton, uint16_t textColour);
    static void formatPatchNumberLabel(byte patchNumber, char* buffer, size_t bufferSize);
    static void formatSongNameDisplayLabel(const char* songName, char* buffer, size_t bufferSize);
    static void formatPatchNameDisplayLabel(const char* patchName, char* buffer, size_t bufferSize);
    void formatSnapshotLabelUppercase(byte snapshotButton, char* buffer, size_t bufferSize) const;
    void updateTrackedTextLabel(TrackedTextLabel& trackedLabel, const char* newText, const GFXfont* font, uint8_t scale,
                                int32_t x, int32_t y, uint16_t textColour);
    void clearTrackedTextLabel(TrackedTextLabel& trackedLabel, const GFXfont* font, uint8_t scale, int32_t x, int32_t y);
    PatchBadgeMetrics patchBadgeMetrics() const;
    int32_t patchBadgeFrameCenterX() const;
    int32_t patchBadgeFrameTopY() const;
    int32_t songAndPatchLabelLeftX() const;
    int32_t songNameLabelY() const;
    int32_t patchNameLabelY() const;
    int32_t snapshotLabelY() const;
    void renderButton(byte number);
    void updateSnapshotSelectionVisuals(byte previousSelected, byte currentSelected);
    void updateVisuals();
    int8_t firstSceneSelectionButton() const;
    bool isSceneSelectionButton(byte number) const;
    bool isTapTempoButton(byte number) const;
    bool isToggleButton(byte number) const;
    ActionType toggleActionTypeForButton(byte number) const;
    bool isToggleActionEnabled(ActionType actionType) const;
    bool hasSongDisplayName() const;
    bool hasPatchDisplayName() const;
    void configurePatchButton();
    void configureSetButton();
    void configureManageSetButton();
    void configurePlaySetButtons();
    void refreshPlaySetState();
    bool resolveSelectedSetSong();
    bool resolveSelectedSong();
    bool advanceSelectedSetSong(bool allowWrap);
    void handleNextSetSongPress();
    ModeTransitionValue currentPlayTransitionValue(bool shouldRecall) const;
    bool usesSelectionBorder(byte number) const override;
    uint8_t ringBrightnessForButton(byte number) const override;
    void setupFunctions();

  private:
    IMidiProvider& _midiProvider;
    IButtonOverrideStore& _buttonOverrideStore;
    ISetListStore& _setListStore;
    byte _selectedPreset;
    byte _selectedPlaylist;
    byte _selectedSongIndex;
    bool _hasSelectedPreset;
    bool _hasSelectedSong;
    bool _hasSelectedSetSong;
    bool _selectedSetSongUnavailable;
    bool _showEndOfSetPrompt;
    bool _isPlaySetMode;
    byte _selectedButton;
    uint32_t _lastSetAdvancePressedAtMs;
    uint32_t _setWrapConfirmUntilMs;
    TapTempoEngine _tapTempoEngine;
    uint32_t _nextTapTempoFlashToggleMs;
    uint32_t _tapTempoFlashUntilMs;
    uint32_t _nextTunerFlashToggleMs;
    bool _isTapTempoLit;
    bool _isTunerEnabled;
    bool _isGigViewEnabled;
    bool _isTunerFlashLit;
    bool _toggleStates[TouchButtonManager::BUTTON_COUNT];
    PatchDisplayConfig _patchDisplayConfig;
    char _selectedSongDisplayName[PatchDisplayConfig::NameCapacity];
    TrackedTextLabel _songNameLabel;
    TrackedTextLabel _patchNameLabel;
    TrackedTextLabel _snapshotLabel;
    char _tapTempoDisplayLabel[Function::LabelCapacity];
};
