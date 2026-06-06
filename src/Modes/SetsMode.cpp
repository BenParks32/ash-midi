#include "Modes/SetsMode.h"

#include <TFT_eSPI.h>
#include <cstdio>

namespace
{
const byte NextButtonIndex = 0;
const byte SelectButtonIndex = 1;
const byte PlayButtonIndex = 3;
const byte PrevButtonIndex = 4;
const byte PartButtonIndex = 5;
const byte LoadButtonIndex = 6;
const byte ExitButtonIndex = 7;

const uint16_t NextButtonColour = 0xFD20;
const uint16_t SelectButtonColour = 0x07E0;
const uint16_t PlayButtonColour = 0x07FF;
const uint16_t PrevButtonColour = 0xFFE0;
const uint16_t PartButtonColour = 0x001F;
const uint16_t LoadButtonColour = 0xF81F;
const uint16_t ExitButtonColour = 0xFFFF;

const char* NextLabel = "Next";
const char* SelectLabel = "Select";
const char* PlayLabel = "Play";
const char* PrevLabel = "Prev";
const char* LoadLabel = "Load";
const char* ExitLabel = "Exit";
const char* EmptySetLabel = "No active set";
const char* PartLabelPrefix = "Prt ";

const uint16_t ActiveSetColour = 0x07FF;
const uint8_t RowScale = 1;
const int32_t ListPadding = 5;
const int32_t CenterSectionInset = 2;
const int32_t RowHeight = 28;
const int32_t CursorHeight = RowHeight + 1;
const int32_t RowTextInsetX = 10;
const int32_t RowTextInsetY = 6;
const size_t VisibleRowCount = 4;
} // namespace

SetsMode::SetsMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                   IMidiManager& midiManager, IMidiProvider& midiProvider, ISetListStore& setListStore,
                   IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _setListStore(setListStore), _selectedPlaylist(midiProvider.defaultPlaylistIndex()),
      _currentPatch(0), _currentSongIndex(0), _currentSetSongIndex(0), _hasCurrentSong(false), _hasCurrentSetSong(false),
      _hasActiveSet(false), _activeSet(), _highlightedSongIndex(0), _activeSetName{0}
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

    switch (number)
    {
    case NextButtonIndex:
        moveSelection(1);
        return;
    case SelectButtonIndex:
        selectHighlightedSong();
        return;
    case PlayButtonIndex:
        returnToPlaySet();
        return;
    case PrevButtonIndex:
        moveSelection(-1);
        return;
    case PartButtonIndex:
        jumpToNextPart();
        return;
    case LoadButtonIndex:
        openSetSelection();
        return;
    case ExitButtonIndex:
        exitToPlay();
        return;
    default:
        break;
    }
}

void SetsMode::buttonLongPressed(byte number) { (void)number; }

void SetsMode::frameTick() {}

void SetsMode::encoderRotated(int16_t steps) { moveSelection(steps); }

void SetsMode::encoderPressed() { selectHighlightedSong(); }

void SetsMode::setTransitionValue(ModeTransitionValue transitionValue)
{
    _hasCurrentSong = false;
    _hasCurrentSetSong = false;
    _currentSongIndex = 0;
    _currentSetSongIndex = 0;

    if (transitionValue == ModeTransitionNone)
    {
        _selectedPlaylist = _midiProvider.defaultPlaylistIndex();
        _currentPatch = 0;
        return;
    }

    if (isPlayModeTransition(transitionValue))
    {
        _selectedPlaylist = playModeTransitionPlaylist(transitionValue);
        if (isPlayModeSetSongTransition(transitionValue))
        {
            _hasCurrentSetSong = true;
        }
        else if (isPlayModeSongTransition(transitionValue))
        {
            _hasCurrentSong = true;
            _currentSongIndex = playModeTransitionSongIndex(transitionValue);
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

    _functions[NextButtonIndex] = Function(NextLabel, NextButtonColour, ActionType::None, ActionType::None);
    _functions[SelectButtonIndex] = Function(SelectLabel, SelectButtonColour, ActionType::None, ActionType::None);
    _functions[PlayButtonIndex] = Function(PlayLabel, PlayButtonColour, ActionType::None, ActionType::None);
    _functions[PrevButtonIndex] = Function(PrevLabel, PrevButtonColour, ActionType::None, ActionType::None);
    _functions[PartButtonIndex] = Function(PartLabelPrefix, PartButtonColour, ActionType::None, ActionType::None);
    _functions[LoadButtonIndex] = Function(LoadLabel, LoadButtonColour, ActionType::None, ActionType::None);
    _functions[ExitButtonIndex] = Function(ExitLabel, ExitButtonColour, ActionType::None, ActionType::None);
}

void SetsMode::loadActiveSet()
{
    _hasActiveSet = _setListStore.activeSetList(_selectedPlaylist, _activeSet) && _activeSet.songCount > 0;
    if (_hasActiveSet)
    {
        std::snprintf(_activeSetName, sizeof(_activeSetName), "%s", _activeSet.name);
        _highlightedSongIndex = _activeSet.selectedSongIndex;
        _currentSetSongIndex = static_cast<byte>(_activeSet.selectedSongIndex);
        _hasCurrentSetSong = true;
    }
    else
    {
        _activeSet = {};
        _activeSetName[0] = '\0';
        _highlightedSongIndex = 0;
    }

    if (_hasActiveSet && _activeSet.partCount > 0 && _highlightedSongIndex < _activeSet.songCount)
    {
        char partLabel[Function::LabelCapacity] = {};
        std::snprintf(partLabel, sizeof(partLabel), "%s%u/%u", PartLabelPrefix,
                      static_cast<unsigned int>(_activeSet.songs[_highlightedSongIndex].part),
                      static_cast<unsigned int>(_activeSet.partCount));
        _functions[PartButtonIndex] = Function(partLabel, PartButtonColour, ActionType::None, ActionType::None);
    }
    else
    {
        _functions[PartButtonIndex] = Function(PartLabelPrefix, PartButtonColour, ActionType::None, ActionType::None);
    }
}

void SetsMode::openSetSelection()
{
    _transitionDelegate.enterMode(Modes::SetSelection, currentPlaySetTransitionValue(false));
}

void SetsMode::returnToPlaySet()
{
    _transitionDelegate.enterMode(Modes::PlaySet, currentPlaySetTransitionValue(false));
}

void SetsMode::exitToPlay()
{
    _setListStore.clearActiveSetList(_selectedPlaylist);
    _transitionDelegate.enterMode(Modes::Play, makePlayModeTransition(_selectedPlaylist, _currentPatch, false));
}

void SetsMode::moveSelection(int16_t steps)
{
    if (!_hasActiveSet || _activeSet.songCount == 0 || steps == 0)
    {
        return;
    }

    const int32_t songCount = static_cast<int32_t>(_activeSet.songCount);
    int32_t nextSongIndex = (static_cast<int32_t>(_highlightedSongIndex) + steps) % songCount;
    if (nextSongIndex < 0)
    {
        nextSongIndex += songCount;
    }

    setHighlightedSongIndex(static_cast<size_t>(nextSongIndex));
}

void SetsMode::jumpToNextPart()
{
    if (!_hasActiveSet || _activeSet.songCount == 0 || _highlightedSongIndex >= _activeSet.songCount)
    {
        return;
    }

    const uint16_t currentPart = _activeSet.songs[_highlightedSongIndex].part;
    for (size_t index = _highlightedSongIndex + 1; index < _activeSet.songCount; ++index)
    {
        if (_activeSet.songs[index].part != currentPart)
        {
            setHighlightedSongIndex(index);
            return;
        }
    }

    setHighlightedSongIndex(0);
}

void SetsMode::selectHighlightedSong()
{
    if (!_hasActiveSet || _highlightedSongIndex >= _activeSet.songCount)
    {
        return;
    }

    if (!_setListStore.selectSong(_selectedPlaylist, _highlightedSongIndex))
    {
        return;
    }

    _currentSetSongIndex = static_cast<byte>(_highlightedSongIndex);
    _hasCurrentSetSong = true;
    returnToPlaySet();
}

void SetsMode::setHighlightedSongIndex(size_t songIndex)
{
    if (!_hasActiveSet || songIndex >= _activeSet.songCount || songIndex == _highlightedSongIndex)
    {
        return;
    }

    const size_t previousSongIndex = _highlightedSongIndex;
    const size_t previousVisibleStart = visibleSongStartIndexFor(previousSongIndex);
    _highlightedSongIndex = songIndex;
    const size_t nextVisibleStart = visibleSongStartIndexFor(_highlightedSongIndex);

    if (_activeSet.partCount > 0 && _highlightedSongIndex < _activeSet.songCount)
    {
        char partLabel[Function::LabelCapacity] = {};
        std::snprintf(partLabel, sizeof(partLabel), "%s%u/%u", PartLabelPrefix,
                      static_cast<unsigned int>(_activeSet.songs[_highlightedSongIndex].part),
                      static_cast<unsigned int>(_activeSet.partCount));
        _functions[PartButtonIndex] = Function(partLabel, PartButtonColour, ActionType::None, ActionType::None);
    }
    renderAllButtons();

    if (previousVisibleStart != nextVisibleStart)
    {
        renderScreen();
        return;
    }

    renderSongRow(previousSongIndex, false);
    renderSongRow(_highlightedSongIndex, true);
}

ModeTransitionValue SetsMode::currentPlaySetTransitionValue(bool shouldRecall) const
{
    const byte patch = (_hasActiveSet && _highlightedSongIndex < _activeSet.songCount)
                           ? _activeSet.songs[_highlightedSongIndex].patch
                           : _currentPatch;
    return makePlayModeSetSongTransition(_selectedPlaylist, patch, shouldRecall);
}

ModeTransitionValue SetsMode::currentPlayTransitionValue(bool shouldRecall) const
{
    return _hasCurrentSong ? makePlayModeSongTransition(_selectedPlaylist, _currentSongIndex, shouldRecall)
                           : makePlayModeTransition(_selectedPlaylist, _currentPatch, shouldRecall);
}

void SetsMode::renderScreen() const
{
    if (!_hasActiveSet)
    {
        renderEmptyState();
        return;
    }

    renderVisibleSongs();
}

void SetsMode::clearScreen() const
{
    const int32_t centerTopY = _screenUi.boxHeight() + CenterSectionInset;
    const int32_t centerBottomY = _screenUi.bottomRowY() - CenterSectionInset;
    _screenUi.fillRect(listStartX(), centerTopY, listWidth(), centerBottomY - centerTopY, TFT_BLACK);
}

void SetsMode::renderVisibleSongs() const
{
    if (!_hasActiveSet || _activeSet.songCount == 0)
    {
        return;
    }

    const size_t visibleSongStart = visibleSongStartIndexFor(_highlightedSongIndex);
    const size_t visibleCount = ((_activeSet.songCount - visibleSongStart) < visibleSongCapacity())
                                    ? (_activeSet.songCount - visibleSongStart)
                                    : visibleSongCapacity();

    for (size_t visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex)
    {
        const size_t songIndex = visibleSongStart + visibleIndex;
        renderSongRow(songIndex, songIndex == _highlightedSongIndex);
    }
}

void SetsMode::renderSongRow(size_t songIndex, bool highlighted) const
{
    if (!_hasActiveSet || songIndex >= _activeSet.songCount)
    {
        return;
    }

    const size_t visibleStart = visibleSongStartIndexFor(songIndex);
    const size_t visibleRow = songIndex - visibleStart;
    const int32_t rowX = listStartX();
    const int32_t rowTop = rowY(visibleRow);
    const uint16_t backgroundColour = highlighted ? TFT_WHITE : TFT_BLACK;
    const uint16_t textColour =
        highlighted ? TFT_BLACK : (_activeSet.songs[songIndex].available ? TFT_WHITE : TFT_RED);

    char rowLabel[64] = {};
    formatSongRowLabel(_activeSet.songs[songIndex], rowLabel, sizeof(rowLabel));
    _screenUi.fillRect(rowX, rowTop, listWidth(), CursorHeight, backgroundColour);
    _screenUi.drawText(FF22, RowScale, rowLabel, rowX + RowTextInsetX, rowTop + RowTextInsetY, textColour,
                       backgroundColour);
}

void SetsMode::renderEmptyState() const
{
    _screenUi.drawText(FF22, RowScale, EmptySetLabel, listStartX() + RowTextInsetX, rowY(0) + RowTextInsetY, TFT_WHITE,
                       TFT_BLACK);
}

size_t SetsMode::visibleSongCapacity() const { return VisibleRowCount; }

size_t SetsMode::visibleSongStartIndexFor(size_t songIndex) const
{
    return (songIndex / visibleSongCapacity()) * visibleSongCapacity();
}

int32_t SetsMode::listStartX() const { return ListPadding; }

int32_t SetsMode::listStartY() const { return _screenUi.boxHeight() + CenterSectionInset + ListPadding; }

int32_t SetsMode::listWidth() const { return (_screenUi.boxWidth() * 4) - (ListPadding * 2); }

int32_t SetsMode::listHeight() const
{
    return (_screenUi.bottomRowY() - CenterSectionInset) - listStartY() - ListPadding;
}

int32_t SetsMode::rowGap() const
{
    const int32_t totalGapHeight = listHeight() - (static_cast<int32_t>(VisibleRowCount) * RowHeight);
    return totalGapHeight / static_cast<int32_t>(VisibleRowCount - 1);
}

int32_t SetsMode::rowY(size_t visibleRow) const
{
    return listStartY() + (static_cast<int32_t>(visibleRow) * (RowHeight + rowGap()));
}

void SetsMode::formatSongRowLabel(const SetListSongEntry& song, char* buffer, size_t bufferSize) const
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    std::snprintf(buffer, bufferSize, "%u.%u %s", static_cast<unsigned int>(song.part),
                  static_cast<unsigned int>(song.number), song.name);
}

uint8_t SetsMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}
