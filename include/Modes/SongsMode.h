#pragma once

#include "ButtonOverrideStore.h"
#include "Modes/FunctionModeBase.h"
#include "MidiProvider.h"

class SongsMode : public FunctionModeBase
{
  public:
    SongsMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
              IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
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
    static constexpr size_t MaxSongs = 64;

  private:
    void resetButtons();
    void loadSongs();
    void initializeSelection();
    void moveSelection(int16_t steps);
    void setHighlightedSongListIndex(size_t songListIndex);
    void selectHighlightedSong();
    void configureBackButton();
    void configureSelectButton();
    void configureDownButton();
    void configureUpButton();
    void renderList() const;
    void renderVisibleSongs() const;
    void renderSongRow(size_t songListIndex, bool highlighted) const;
    void clearListArea() const;
    void renderEmptyState() const;
    size_t visibleSongCapacity() const;
    size_t visibleSongStartIndexFor(size_t songListIndex) const;
    int32_t songListStartX() const;
    int32_t songListStartY() const;
    int32_t songListWidth() const;
    int32_t songListHeight() const;
    int32_t songRowGap() const;
    int32_t songRowY(size_t visibleRow) const;
    int32_t songColumnWidth() const;
    int32_t songColumnX(size_t columnIndex) const;
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    IMidiProvider& _midiProvider;
    IButtonOverrideStore& _buttonOverrideStore;
    byte _selectedPlaylist;
    byte _currentPatch;
    byte _currentSongIndex;
    bool _hasCurrentSong;
    SongListEntry _songEntries[MaxSongs];
    size_t _songCount;
    size_t _highlightedSongListIndex;
};
