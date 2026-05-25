#include "Modes/SongsMode.h"

#include <cstdio>
#include <cstring>

namespace
{
const byte BackButtonIndex = 4;
const uint16_t SongButtonColour = 0x07FF;
const uint16_t BackButtonColour = 0xFFFF;
const char* BackLabel = "Back";
const char* SongsSubtitle = "songs";
const uint8_t SongsTitleScale = 1;
const uint8_t SongsSubtitleScale = 1;
const int32_t SongsTitleOffsetY = 26;
const int32_t SongsSubtitleOffsetY = 72;

const char* songsDisplayTitle(byte playlistIndex)
{
    switch (playlistIndex)
    {
    case 2:
        return "Project7";
    case 3:
        return "OPR";
    case 4:
        return "Code Red";
    default:
        return "Unknown";
    }
}
} // namespace

SongsMode::SongsMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                     IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
                     IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _buttonOverrideStore(buttonOverrideStore),
      _selectedPlaylist(midiProvider.defaultPlaylistIndex()), _currentPatch(0), _currentSongIndex(0), _hasCurrentSong(false),
      _buttonSongIndices{InvalidSongIndex}
{
    resetButtons();
}

void SongsMode::activate()
{
    populateButtons();
    renderAllButtons();
    renderCenterTitle(TFT_WHITE);
}

void SongsMode::deactivate() { renderCenterTitle(TFT_BLACK); }

void SongsMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return;
    }

    if (number == BackButtonIndex)
    {
        const ModeTransitionValue transitionValue =
            _hasCurrentSong ? makePlayModeSongTransition(_selectedPlaylist, _currentSongIndex, false)
                            : makePlayModeTransition(_selectedPlaylist, _currentPatch, false);
        _transitionDelegate.enterMode(Modes::Play, transitionValue);
        return;
    }

    const byte songIndex = _buttonSongIndices[number];
    if (songIndex == InvalidSongIndex)
    {
        return;
    }

    _transitionDelegate.enterMode(Modes::Play, makePlayModeSongTransition(_selectedPlaylist, songIndex, true));
}

void SongsMode::buttonLongPressed(byte number)
{
    (void)number;
}

void SongsMode::frameTick()
{
    // Songs mode currently has no per-frame work.
}

void SongsMode::setTransitionValue(ModeTransitionValue transitionValue)
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
            _hasCurrentSong = true;
            _currentSongIndex = playModeTransitionSongIndex(transitionValue);
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

void SongsMode::resetButtons()
{
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        _functions[buttonIndex] = Function();
        _buttonSongIndices[buttonIndex] = InvalidSongIndex;
    }
}

void SongsMode::populateButtons()
{
    resetButtons();
    configureBackButton();

    SongListEntry entries[TouchButtonManager::BUTTON_COUNT] = {};
    const size_t entryCount = _buttonOverrideStore.listSongs(_selectedPlaylist, entries, TouchButtonManager::BUTTON_COUNT);
    byte nextButtonIndex = 0;
    for (size_t entryIndex = 0; entryIndex < entryCount && nextButtonIndex < TouchButtonManager::BUTTON_COUNT; ++entryIndex)
    {
        if (nextButtonIndex == BackButtonIndex)
        {
            ++nextButtonIndex;
        }

        if (nextButtonIndex >= TouchButtonManager::BUTTON_COUNT)
        {
            break;
        }

        char label[Function::LabelCapacity] = {'\0'};
        formatSongLabel(entries[entryIndex].name, entries[entryIndex].patchNumber, label, sizeof(label));
        configureButton(nextButtonIndex, entries[entryIndex].songIndex, label);
        ++nextButtonIndex;
    }
}

void SongsMode::configureButton(byte buttonIndex, byte songIndex, const char* label)
{
    if (buttonIndex >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    _functions[buttonIndex] = Function(label, SongButtonColour, ActionType::None, ActionType::None);
    _buttonSongIndices[buttonIndex] = songIndex;
}

void SongsMode::configureBackButton()
{
    _functions[BackButtonIndex] = Function(BackLabel, BackButtonColour, ActionType::None, ActionType::None);
    _buttonSongIndices[BackButtonIndex] = InvalidSongIndex;
}

void SongsMode::renderCenterTitle(uint16_t textColour)
{
    const int32_t centerX = _screenUi.boxWidth() * 2;
    const int32_t centerTopY = _screenUi.boxHeight();

    _screenUi.drawCenteredText(FF32, SongsTitleScale, songsDisplayTitle(_selectedPlaylist), centerX,
                               centerTopY + SongsTitleOffsetY, textColour, TFT_BLACK);
    _screenUi.drawCenteredText(FF22, SongsSubtitleScale, SongsSubtitle, centerX,
                               centerTopY + SongsSubtitleOffsetY, textColour, TFT_BLACK);
}

void SongsMode::formatSongLabel(const char* songName, byte patchNumber, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0U)
    {
        return;
    }

    if (songName != nullptr && songName[0] != '\0')
    {
        std::snprintf(buffer, bufferSize, "%s", songName);
        return;
    }

    std::snprintf(buffer, bufferSize, "%02u", static_cast<unsigned int>(patchNumber));
}

uint8_t SongsMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}
