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
    void returnToPlay();
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
    bool _hasCurrentSong;
    bool _hasActiveSet;
    char _activeSetName[ActiveSetList::MaxNameLength];
};
