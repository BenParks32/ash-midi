#pragma once

#include "Modes/FunctionModeBase.h"
#include "MidiProvider.h"
#include "SetListStore.h"

class SetsMode : public FunctionModeBase
{
  public:
    SetsMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
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
    void resetButtons();
    void loadActiveSet();
    void openSetSelection();
    void returnToPlaySet();
    void exitToPlay();
    void moveSelection(int16_t steps);
    void jumpToNextPart();
    void selectHighlightedSong();
    void setHighlightedSongIndex(size_t songIndex);
    void renderVisibleSongs() const;
    void renderSongRow(size_t songIndex, bool highlighted) const;
    void renderEmptyState() const;
    size_t visibleSongCapacity() const;
    size_t visibleSongStartIndexFor(size_t songIndex) const;
    int32_t listStartX() const;
    int32_t listStartY() const;
    int32_t listWidth() const;
    int32_t listHeight() const;
    int32_t rowGap() const;
    int32_t rowY(size_t visibleRow) const;
    void formatSongRowLabel(const SetListSongEntry& song, char* buffer, size_t bufferSize) const;
    ModeTransitionValue currentPlaySetTransitionValue(bool shouldRecall) const;
    ModeTransitionValue currentPlayTransitionValue(bool shouldRecall) const;
    void renderScreen() const;
    void clearScreen() const;
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    IMidiProvider& _midiProvider;
    ISetListStore& _setListStore;
    byte _selectedPlaylist;
    byte _currentPatch;
    byte _currentSongIndex;
    byte _currentSetSongIndex;
    bool _hasCurrentSong;
    bool _hasCurrentSetSong;
    bool _hasActiveSet;
    ActiveSetList _activeSet;
    size_t _highlightedSongIndex;
    char _activeSetName[ActiveSetList::MaxNameLength];
};
