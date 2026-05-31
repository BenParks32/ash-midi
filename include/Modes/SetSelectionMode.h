#pragma once

#include "Modes/FunctionModeBase.h"
#include "MidiProvider.h"
#include "SetListStore.h"

class SetSelectionMode : public FunctionModeBase
{
  public:
    SetSelectionMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                     IMidiManager& midiManager, IMidiProvider& midiProvider, ISetListStore& setListStore,
                     IModeTransistionDelegate& transitionDelegate);

    void activate() override;
    void deactivate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void encoderRotated(int16_t steps) override;
    void encoderPressed() override;
    void setTransitionValue(ModeTransitionValue transitionValue) override;

  private:
    struct TrackedTextLabel
    {
        static constexpr size_t Capacity = SetListSummary::MaxNameLength + 16;
        char text[Capacity];
    };

  private:
    static constexpr size_t MaxVisibleSetLists = 17;

    void resetButtons();
    void loadSetLists();
    void initializeSelection();
    void moveSelection(int16_t steps);
    void setHighlightedSetListIndex(size_t setListIndex);
    void selectHighlightedSetList();
    void returnToPlay();
    ModeTransitionValue currentPlayTransitionValue(bool shouldRecall) const;
    void renderList() const;
    void renderHeader() const;
    void renderHeaderLabel() const;
    void updateTrackedTextLabel(TrackedTextLabel& trackedLabel, const char* newText, uint16_t textColour) const;
    void clearTrackedTextLabel(TrackedTextLabel& trackedLabel) const;
    void renderVisibleSetLists() const;
    void renderVisibleSetListPage() const;
    void renderSetListRow(size_t setListIndex, bool highlighted) const;
    void clearListArea() const;
    void clearVisibleSetListArea() const;
    bool setListIndexForFileName(const char* fileName, size_t& setListIndex) const;
    bool isActiveSetList(size_t setListIndex) const;
    size_t visibleSetListCapacity() const;
    size_t visibleSetListStartIndexFor(size_t setListIndex) const;
    int32_t listStartX() const;
    int32_t centerTopY() const;
    int32_t centerBottomY() const;
    int32_t listStartY() const;
    int32_t listWidth() const;
    int32_t listHeight() const;
    int32_t rowGap() const;
    int32_t rowY(size_t visibleRow) const;
    int32_t columnWidth() const;
    int32_t columnX(size_t columnIndex) const;
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    IMidiProvider& _midiProvider;
    ISetListStore& _setListStore;
    byte _selectedPlaylist;
    byte _currentPatch;
    byte _currentSongIndex;
    bool _hasCurrentSong;
    bool _isLoading;
    SetListSummary _setLists[MaxVisibleSetLists];
    size_t _setListCount;
    size_t _highlightedSetListIndex;
    bool _hasActiveSet;
    char _activeSetFileName[SetListSummary::MaxFileNameLength];
    char _activeSetName[SetListSummary::MaxNameLength];
    mutable TrackedTextLabel _headerLabel;
};
