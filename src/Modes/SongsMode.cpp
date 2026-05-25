#include "Modes/SongsMode.h"

#include <TFT_eSPI.h>

namespace
{
const byte PageDownButtonIndex = 2;
const byte SongDownButtonIndex = 3;
const byte BackButtonIndex = 4;
const byte SelectButtonIndex = 5;
const byte PageUpButtonIndex = 6;
const byte SongUpButtonIndex = 7;

const uint16_t PageDownButtonColour = 0x07FF;
const uint16_t SongDownButtonColour = 0x07FF;
const uint16_t BackButtonColour = 0xFFFF;
const uint16_t SelectButtonColour = 0x07E0;
const uint16_t PageUpButtonColour = 0xFD20;
const uint16_t SongUpButtonColour = 0xFD20;

const char* PageDownLabel = "PgDn";
const char* SongDownLabel = "Down";
const char* BackLabel = "Back";
const char* SelectLabel = "Select";
const char* PageUpLabel = "PgUp";
const char* SongUpLabel = "Up";
const char* NoSongsLabel = "No songs";

const uint8_t SongsRowLabelScale = 1;
const int32_t SongsListPadding = 5;
const int32_t SongsColumnGap = 5;
const int32_t SongsRowHeight = 32;
const int32_t SongsRowTextInsetX = 10;
const int32_t SongsRowTextInsetY = 6;
const size_t SongsColumnCount = 2;
const size_t SongsVisibleRowCount = 4;
} // namespace

SongsMode::SongsMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                     IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
                     IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _buttonOverrideStore(buttonOverrideStore),
      _selectedPlaylist(midiProvider.defaultPlaylistIndex()), _currentPatch(0), _currentSongIndex(0), _hasCurrentSong(false),
      _songEntries{}, _songCount(0), _highlightedSongListIndex(0)
{
    resetButtons();
}

void SongsMode::activate()
{
    loadSongs();
    initializeSelection();
    clearListArea();
    renderAllButtons();
    renderList();
}

void SongsMode::deactivate() { clearListArea(); }

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

    if (number == SelectButtonIndex)
    {
        selectHighlightedSong();
        return;
    }

    if (number == SongDownButtonIndex)
    {
        moveSelection(1);
        return;
    }

    if (number == PageDownButtonIndex)
    {
        moveSelection(static_cast<int16_t>(visibleSongCapacity()));
        return;
    }

    if (number == PageUpButtonIndex)
    {
        moveSelection(-static_cast<int16_t>(visibleSongCapacity()));
        return;
    }

    if (number == SongUpButtonIndex)
    {
        moveSelection(-1);
    }
}

void SongsMode::buttonLongPressed(byte number)
{
    (void)number;
}

void SongsMode::frameTick()
{
    // Songs mode currently has no per-frame work.
}

void SongsMode::encoderRotated(int16_t steps) { moveSelection(steps); }

void SongsMode::encoderPressed() { selectHighlightedSong(); }

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
    }

    configureDownButton();
    configureBackButton();
    configureSelectButton();
    configureUpButton();
}

void SongsMode::loadSongs()
{
    _songCount = _buttonOverrideStore.listSongs(_selectedPlaylist, _songEntries, MaxSongs);
}

void SongsMode::initializeSelection()
{
    _highlightedSongListIndex = 0;

    if (_songCount == 0)
    {
        return;
    }

    if (_hasCurrentSong)
    {
        for (size_t songListIndex = 0; songListIndex < _songCount; ++songListIndex)
        {
            if (_songEntries[songListIndex].songIndex == _currentSongIndex)
            {
                _highlightedSongListIndex = songListIndex;
                break;
            }
        }
    }
}

void SongsMode::moveSelection(int16_t steps)
{
    if (steps == 0 || _songCount == 0)
    {
        return;
    }

    const size_t previousSongListIndex = _highlightedSongListIndex;
    const size_t previousVisibleSongStartIndex = visibleSongStartIndexFor(previousSongListIndex);
    const int32_t songCount = static_cast<int32_t>(_songCount);
    int32_t nextIndex = (static_cast<int32_t>(_highlightedSongListIndex) + steps) % songCount;
    if (nextIndex < 0)
    {
        nextIndex += songCount;
    }

    _highlightedSongListIndex = static_cast<size_t>(nextIndex);
    const size_t nextVisibleSongStartIndex = visibleSongStartIndexFor(_highlightedSongListIndex);
    if (previousVisibleSongStartIndex != nextVisibleSongStartIndex)
    {
        renderList();
        return;
    }

    renderSongRow(previousSongListIndex, false);
    renderSongRow(_highlightedSongListIndex, true);
}

void SongsMode::selectHighlightedSong()
{
    if (_songCount == 0 || _highlightedSongListIndex >= _songCount)
    {
        return;
    }

    const byte songIndex = _songEntries[_highlightedSongListIndex].songIndex;
    _transitionDelegate.enterMode(Modes::Play, makePlayModeSongTransition(_selectedPlaylist, songIndex, true));
}

void SongsMode::configureBackButton()
{
    _functions[BackButtonIndex] = Function(BackLabel, BackButtonColour, ActionType::None, ActionType::None);
}

void SongsMode::configureSelectButton()
{
    _functions[SelectButtonIndex] = Function(SelectLabel, SelectButtonColour, ActionType::None, ActionType::None);
}

void SongsMode::configureDownButton()
{
    _functions[PageDownButtonIndex] = Function(PageDownLabel, PageDownButtonColour, ActionType::None, ActionType::None);
    _functions[SongDownButtonIndex] = Function(SongDownLabel, SongDownButtonColour, ActionType::None, ActionType::None);
}

void SongsMode::configureUpButton()
{
    _functions[PageUpButtonIndex] = Function(PageUpLabel, PageUpButtonColour, ActionType::None, ActionType::None);
    _functions[SongUpButtonIndex] = Function(SongUpLabel, SongUpButtonColour, ActionType::None, ActionType::None);
}

void SongsMode::renderList() const
{
    clearListArea();
    if (_songCount == 0)
    {
        renderEmptyState();
        return;
    }

    renderVisibleSongs();
}

void SongsMode::renderVisibleSongs() const
{
    const size_t visibleSongStartIndex = visibleSongStartIndexFor(_highlightedSongListIndex);
    const size_t visibleSongCount = ((_songCount - visibleSongStartIndex) < visibleSongCapacity())
                                        ? (_songCount - visibleSongStartIndex)
                                        : visibleSongCapacity();

    for (size_t visibleIndex = 0; visibleIndex < visibleSongCount; ++visibleIndex)
    {
        renderSongRow(visibleSongStartIndex + visibleIndex,
                      (visibleSongStartIndex + visibleIndex) == _highlightedSongListIndex);
    }
}

void SongsMode::renderSongRow(size_t songListIndex, bool highlighted) const
{
    if (songListIndex >= _songCount)
    {
        return;
    }

    const size_t visibleSongStartIndex = visibleSongStartIndexFor(songListIndex);
    const size_t visibleSlotIndex = songListIndex - visibleSongStartIndex;
    const size_t columnIndex = visibleSlotIndex / SongsVisibleRowCount;
    const size_t rowIndex = visibleSlotIndex % SongsVisibleRowCount;
    const int32_t rowX = songColumnX(columnIndex);
    const int32_t rowY = songRowY(rowIndex);
    const uint16_t backgroundColour = highlighted ? TFT_WHITE : TFT_BLACK;
    const uint16_t textColour = highlighted ? TFT_BLACK : TFT_WHITE;

    _screenUi.fillRect(rowX, rowY, songColumnWidth(), SongsRowHeight, backgroundColour);
    _screenUi.drawText(FF22, SongsRowLabelScale, _songEntries[songListIndex].name, rowX + SongsRowTextInsetX,
                       rowY + SongsRowTextInsetY, textColour, backgroundColour);
}

void SongsMode::clearListArea() const
{
    _screenUi.fillRect(songListStartX(), songListStartY(), songListWidth(), songListHeight(), TFT_BLACK);
}

void SongsMode::renderEmptyState() const
{
    _screenUi.drawText(FF22, SongsRowLabelScale, NoSongsLabel, songListStartX() + SongsRowTextInsetX,
                       songListStartY() + SongsRowTextInsetY, TFT_WHITE, TFT_BLACK);
}

size_t SongsMode::visibleSongCapacity() const { return SongsVisibleRowCount * SongsColumnCount; }

size_t SongsMode::visibleSongStartIndexFor(size_t songListIndex) const
{
    return (songListIndex / visibleSongCapacity()) * visibleSongCapacity();
}

int32_t SongsMode::songListStartX() const { return SongsListPadding; }

int32_t SongsMode::songListStartY() const { return _screenUi.boxHeight() + SongsListPadding; }

int32_t SongsMode::songListWidth() const { return (_screenUi.boxWidth() * 4) - (SongsListPadding * 2); }

int32_t SongsMode::songListHeight() const
{
    return (_screenUi.bottomRowY() - _screenUi.boxHeight()) - (SongsListPadding * 2);
}

int32_t SongsMode::songRowGap() const
{
    const int32_t totalGapHeight = songListHeight() - (static_cast<int32_t>(SongsVisibleRowCount) * SongsRowHeight);
    return totalGapHeight / static_cast<int32_t>(SongsVisibleRowCount - 1);
}

int32_t SongsMode::songRowY(size_t visibleRow) const
{
    return songListStartY() + (static_cast<int32_t>(visibleRow) * (SongsRowHeight + songRowGap()));
}

int32_t SongsMode::songColumnWidth() const
{
    return (songListWidth() - SongsColumnGap) / static_cast<int32_t>(SongsColumnCount);
}

int32_t SongsMode::songColumnX(size_t columnIndex) const
{
    return songListStartX() + (static_cast<int32_t>(columnIndex) * (songColumnWidth() + SongsColumnGap));
}

uint8_t SongsMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}
