#include "Modes/SetsMode.h"

#include <TFT_eSPI.h>
#include <cstdio>

namespace
{
const byte BackButtonIndex = 4;
const byte SetSelectionButtonIndex = 5;

const uint16_t BackButtonColour = 0xFFFF;
const uint16_t SetSelectionButtonColour = 0x07FF;

const char* BackLabel = "Back";
const char* SetSelectionLabel = "Set";
const char* NoSetLabel = "No set selected";
const char* LoadedPrefix = "Loaded: ";
} // namespace

SetsMode::SetsMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                   IMidiManager& midiManager, IMidiProvider& midiProvider, ISetListStore& setListStore,
                   IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _setListStore(setListStore), _selectedPlaylist(midiProvider.defaultPlaylistIndex()),
      _currentPatch(0), _currentSongIndex(0), _hasCurrentSong(false), _hasActiveSet(false), _activeSetName{0}
{
    resetButtons();
}

void SetsMode::activate()
{
    loadActiveSet();
    clearScreen();
    renderAllButtons();
    renderScreen();
}

void SetsMode::deactivate() { clearScreen(); }

void SetsMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return;
    }

    if (number == BackButtonIndex)
    {
        returnToPlay();
        return;
    }

    if (number == SetSelectionButtonIndex)
    {
        openSetSelection();
    }
}

void SetsMode::buttonLongPressed(byte number) { (void)number; }

void SetsMode::frameTick() {}

void SetsMode::encoderRotated(int16_t steps) { (void)steps; }

void SetsMode::encoderPressed() { openSetSelection(); }

void SetsMode::setTransitionValue(ModeTransitionValue transitionValue)
{
    _hasCurrentSong = false;
    _currentSongIndex = 0;

    if (transitionValue == ModeTransitionNone)
    {
        _selectedPlaylist = _midiProvider.defaultPlaylistIndex();
        _currentPatch = 0;
        return;
    }

    if (isPlayModeTransition(transitionValue))
    {
        _selectedPlaylist = playModeTransitionPlaylist(transitionValue);
        if (isPlayModeSongTransition(transitionValue))
        {
            _currentSongIndex = playModeTransitionSongIndex(transitionValue);
            _hasCurrentSong = true;
            _currentPatch = 0;
            return;
        }

        _currentPatch = playModeTransitionPatch(transitionValue);
        return;
    }

    if ((transitionValue & ModeTransitionHomePlaylistFlag) != 0)
    {
        _selectedPlaylist = static_cast<byte>(transitionValue & ModeTransitionHomePlaylistValueMask);
        _currentPatch = 0;
        return;
    }

    _currentPatch = static_cast<byte>(transitionValue & ModeTransitionPatchValueMask);
}

void SetsMode::resetButtons()
{
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        _functions[buttonIndex] = Function();
    }

    _functions[BackButtonIndex] = Function(BackLabel, BackButtonColour, ActionType::None, ActionType::None);
    _functions[SetSelectionButtonIndex] =
        Function(SetSelectionLabel, SetSelectionButtonColour, ActionType::None, ActionType::None);
}

void SetsMode::loadActiveSet()
{
    _hasActiveSet = false;
    std::snprintf(_activeSetName, sizeof(_activeSetName), "%s", "");

    SetListSummary activeSet = {};
    if (_setListStore.activeSetSummary(_selectedPlaylist, activeSet))
    {
        _hasActiveSet = true;
        std::snprintf(_activeSetName, sizeof(_activeSetName), "%s", activeSet.name);
    }
}

void SetsMode::openSetSelection()
{
    _transitionDelegate.enterMode(Modes::SetSelection, currentPlayTransitionValue(false));
}

void SetsMode::returnToPlay()
{
    _transitionDelegate.enterMode(Modes::Play, currentPlayTransitionValue(false));
}

ModeTransitionValue SetsMode::currentPlayTransitionValue(bool shouldRecall) const
{
    return _hasCurrentSong ? makePlayModeSongTransition(_selectedPlaylist, _currentSongIndex, shouldRecall)
                           : makePlayModeTransition(_selectedPlaylist, _currentPatch, shouldRecall);
}

void SetsMode::renderScreen() const
{
    char activeLabel[ActiveSetList::MaxNameLength + 16] = {};
    std::snprintf(activeLabel, sizeof(activeLabel), "%s%s", LoadedPrefix, _hasActiveSet ? _activeSetName : NoSetLabel);
    _screenUi.drawText(FF22, 1, activeLabel, 15, _screenUi.boxHeight() + 12, _hasActiveSet ? 0x07FF : TFT_WHITE,
                       TFT_BLACK);
}

void SetsMode::clearScreen() const
{
    _screenUi.fillRect(0, _screenUi.boxHeight(), _screenUi.boxWidth() * 4, _screenUi.bottomRowY() - _screenUi.boxHeight(),
                       TFT_BLACK);
}

uint8_t SetsMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}
