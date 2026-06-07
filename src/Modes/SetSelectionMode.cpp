#include "Modes/SetSelectionMode.h"

#include <TFT_eSPI.h>
#include <cstdio>
#include <cstring>

namespace
{
const byte SelectButtonIndex = 0;
const byte LastButtonIndex = 1;
const byte PageDownButtonIndex = 2;
const byte SetDownButtonIndex = 3;
const byte BackButtonIndex = 4;
const byte FirstButtonIndex = 5;
const byte PageUpButtonIndex = 6;
const byte SetUpButtonIndex = 7;

const uint16_t SelectButtonColour = 0x07E0;
const uint16_t LastButtonColour = 0x780F;
const uint16_t PageDownButtonColour = 0x001F;
const uint16_t SetDownButtonColour = 0xFD20;
const uint16_t BackButtonColour = 0xFFFF;
const uint16_t FirstButtonColour = 0xF81F;
const uint16_t PageUpButtonColour = 0x07FF;
const uint16_t SetUpButtonColour = 0xFFE0;
const uint16_t ActiveSetColour = 0x07FF;

const char* NoActiveSetLabel = "None";
const char* LoadingLabel = "Loading...";
const char* LoadedSetPrefix = "Loaded: ";
const char* SelectLabel = "Select";
const char* LastLabel = "Last";
const char* PageDownLabel = "PgDn";
const char* SetDownLabel = "Down";
const char* BackLabel = "Back";
const char* FirstLabel = "First";
const char* PageUpLabel = "PgUp";
const char* SetUpLabel = "Up";

const uint8_t HeaderScale = 1;
const uint8_t RowScale = 1;
const int32_t ListPadding = 5;
const int32_t CenterSectionInset = 2;
const int32_t HeaderLineHeight = 18;
const int32_t HeaderGap = 4;
const int32_t RowHeight = 28;
const int32_t CursorHeight = RowHeight + 1;
const int32_t RowTextInsetX = 10;
const int32_t RowTextInsetY = 6;
const size_t ColumnCount = 1;
const size_t VisibleRowCount = 4;

const char* fileNameFromPath(const char* path)
{
    if (path == nullptr)
    {
        return "";
    }

    const char* fileName = std::strrchr(path, '/');
    return (fileName == nullptr) ? path : fileName + 1;
}
} // namespace

SetSelectionMode::SetSelectionMode(TouchButtonManager& touchButtonManager, RingManager& ringManager,
                                   IScreenUi& screenUi, IMidiManager& midiManager, IMidiProvider& midiProvider,
                                   ISetListStore& setListStore, IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _setListStore(setListStore), _selectedPlaylist(midiProvider.defaultPlaylistIndex()),
      _currentPatch(0), _currentSongIndex(0), _hasCurrentSong(false), _isLoading(false), _setLists{}, _setListCount(0),
      _highlightedSetListIndex(0), _hasActiveSet(false), _activeSetFileName{0}, _activeSetName{0}, _headerLabel{{0}}
{
    resetButtons();
}

void SetSelectionMode::activate()
{
    _isLoading = true;
    clearListArea();
    renderAllButtons();
    renderHeaderLabel();
    loadSetLists();
    initializeSelection();
    _isLoading = false;
    renderList();
}

void SetSelectionMode::deactivate()
{
    clearListArea();
    clearTrackedTextLabel(_headerLabel);
}

void SetSelectionMode::buttonPressed(byte number)
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

    if (number == SelectButtonIndex)
    {
        selectHighlightedSetList();
        return;
    }

    if (number == LastButtonIndex)
    {
        if (_setListCount > 0)
        {
            setHighlightedSetListIndex(_setListCount - 1);
        }
        return;
    }

    if (number == SetDownButtonIndex)
    {
        moveSelection(1);
        return;
    }

    if (number == PageDownButtonIndex)
    {
        moveSelection(static_cast<int16_t>(visibleSetListCapacity()));
        return;
    }

    if (number == FirstButtonIndex)
    {
        if (_setListCount > 0)
        {
            setHighlightedSetListIndex(0);
        }
        return;
    }

    if (number == PageUpButtonIndex)
    {
        moveSelection(-static_cast<int16_t>(visibleSetListCapacity()));
        return;
    }

    if (number == SetUpButtonIndex)
    {
        moveSelection(-1);
    }
}

void SetSelectionMode::buttonLongPressed(byte number) { (void)number; }

void SetSelectionMode::frameTick() {}

void SetSelectionMode::encoderRotated(int16_t steps) { moveSelection(steps); }

void SetSelectionMode::encoderPressed() { selectHighlightedSetList(); }

void SetSelectionMode::setTransitionValue(ModeTransitionValue transitionValue)
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

void SetSelectionMode::resetButtons()
{
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        _functions[buttonIndex] = Function();
    }

    _functions[SelectButtonIndex] = Function(SelectLabel, SelectButtonColour, ActionType::None, ActionType::None);
    _functions[LastButtonIndex] = Function(LastLabel, LastButtonColour, ActionType::None, ActionType::None);
    _functions[PageDownButtonIndex] = Function(PageDownLabel, PageDownButtonColour, ActionType::None, ActionType::None);
    _functions[SetDownButtonIndex] = Function(SetDownLabel, SetDownButtonColour, ActionType::None, ActionType::None);
    _functions[BackButtonIndex] = Function(BackLabel, BackButtonColour, ActionType::None, ActionType::None);
    _functions[FirstButtonIndex] = Function(FirstLabel, FirstButtonColour, ActionType::None, ActionType::None);
    _functions[PageUpButtonIndex] = Function(PageUpLabel, PageUpButtonColour, ActionType::None, ActionType::None);
    _functions[SetUpButtonIndex] = Function(SetUpLabel, SetUpButtonColour, ActionType::None, ActionType::None);
}

void SetSelectionMode::loadSetLists()
{
    const size_t listedCount = _setListStore.listSetLists(_selectedPlaylist, _setLists, MaxVisibleSetLists);
    _setListCount = listedCount;

    _hasActiveSet = false;
    _activeSetFileName[0] = '\0';
    std::snprintf(_activeSetName, sizeof(_activeSetName), "%s", NoActiveSetLabel);

    SetListSummary activeSet = {};
    if (_setListStore.activeSetSummary(_selectedPlaylist, activeSet))
    {
        _hasActiveSet = true;
        std::snprintf(_activeSetFileName, sizeof(_activeSetFileName), "%s", activeSet.fileName);
        std::snprintf(_activeSetName, sizeof(_activeSetName), "%s", activeSet.name);
    }
}

void SetSelectionMode::initializeSelection()
{
    _highlightedSetListIndex = 0;
    if (!_hasActiveSet)
    {
        return;
    }

    for (size_t index = 0; index < _setListCount; ++index)
    {
        if (std::strcmp(_setLists[index].fileName, _activeSetFileName) == 0)
        {
            _highlightedSetListIndex = index;
            return;
        }
    }
}

void SetSelectionMode::moveSelection(int16_t steps)
{
    if (steps == 0 || _setListCount == 0)
    {
        return;
    }

    const int32_t setListCount = static_cast<int32_t>(_setListCount);
    int32_t nextIndex = (static_cast<int32_t>(_highlightedSetListIndex) + steps) % setListCount;
    if (nextIndex < 0)
    {
        nextIndex += setListCount;
    }

    setHighlightedSetListIndex(static_cast<size_t>(nextIndex));
}

void SetSelectionMode::setHighlightedSetListIndex(size_t setListIndex)
{
    if (_setListCount == 0 || setListIndex >= _setListCount || setListIndex == _highlightedSetListIndex)
    {
        return;
    }

    const size_t previousSetListIndex = _highlightedSetListIndex;
    const size_t previousVisibleSetListStartIndex = visibleSetListStartIndexFor(previousSetListIndex);
    _highlightedSetListIndex = setListIndex;
    const size_t nextVisibleSetListStartIndex = visibleSetListStartIndexFor(_highlightedSetListIndex);
    if (previousVisibleSetListStartIndex != nextVisibleSetListStartIndex)
    {
        renderVisibleSetListPage();
        return;
    }

    renderSetListRow(previousSetListIndex, false);
    renderSetListRow(_highlightedSetListIndex, true);
}

void SetSelectionMode::selectHighlightedSetList()
{
    if (_highlightedSetListIndex >= _setListCount)
    {
        Serial.printf("[PlaySetDiag] set-load ignored highlighted=%u setCount=%u\n",
                      static_cast<unsigned int>(_highlightedSetListIndex), static_cast<unsigned int>(_setListCount));
        return;
    }

    const size_t previousHighlightedSetListIndex = _highlightedSetListIndex;
    const size_t previousVisibleSetListStartIndex = visibleSetListStartIndexFor(previousHighlightedSetListIndex);
    const bool hadPreviousActiveSet = _hasActiveSet;
    char previousActiveSetFileName[SetListSummary::MaxFileNameLength] = {};
    if (hadPreviousActiveSet)
    {
        std::snprintf(previousActiveSetFileName, sizeof(previousActiveSetFileName), "%s", _activeSetFileName);
    }

    _isLoading = true;
    renderHeaderLabel();

    const bool selectionApplied =
        _setListStore.activateSetList(_selectedPlaylist, _setLists[_highlightedSetListIndex].fileName);
    Serial.printf("[PlaySetDiag] set-activate playlist=%u file='%s' result=%u\n",
                  static_cast<unsigned int>(_selectedPlaylist), _setLists[_highlightedSetListIndex].fileName,
                  selectionApplied ? 1U : 0U);

    _isLoading = false;
    loadSetLists();
    initializeSelection();
    renderHeaderLabel();

    if (selectionApplied)
    {
        const ModeTransitionValue transition = currentPlayTransitionValue(false);
        _transitionDelegate.enterMode(Modes::PlaySet, transition);
        return;
    }

    const size_t nextVisibleSetListStartIndex = visibleSetListStartIndexFor(_highlightedSetListIndex);
    if (previousVisibleSetListStartIndex != nextVisibleSetListStartIndex)
    {
        renderVisibleSetListPage();
        return;
    }

    bool previousHighlightRedrawn = false;
    if (previousHighlightedSetListIndex != _highlightedSetListIndex)
    {
        renderSetListRow(previousHighlightedSetListIndex, false);
        renderSetListRow(_highlightedSetListIndex, true);
        previousHighlightRedrawn = true;
    }

    if (!hadPreviousActiveSet)
    {
        return;
    }

    size_t previousActiveSetIndex = 0;
    if (!setListIndexForFileName(previousActiveSetFileName, previousActiveSetIndex) ||
        previousActiveSetIndex == _highlightedSetListIndex ||
        (previousHighlightRedrawn && previousActiveSetIndex == previousHighlightedSetListIndex) ||
        visibleSetListStartIndexFor(previousActiveSetIndex) != nextVisibleSetListStartIndex)
    {
        return;
    }

    renderSetListRow(previousActiveSetIndex, false);
}

void SetSelectionMode::returnToPlay()
{
    const ModeTransitionValue transition = currentPlayTransitionValue(false);
    Serial.printf("[PlaySetDiag] set-selection back -> sets playlist=%u transition=0x%04X hasCurrentSong=%u currentPatch=%u "
                  "currentSongIndex=%u\n",
                  static_cast<unsigned int>(_selectedPlaylist), static_cast<unsigned int>(transition),
                  _hasCurrentSong ? 1U : 0U, static_cast<unsigned int>(_currentPatch),
                  static_cast<unsigned int>(_currentSongIndex));
    _transitionDelegate.enterMode(Modes::Songs, transition);
}

ModeTransitionValue SetSelectionMode::currentPlayTransitionValue(bool shouldRecall) const
{
    return _hasCurrentSong ? makePlayModeSongTransition(_selectedPlaylist, _currentSongIndex, shouldRecall)
                           : makePlayModeTransition(_selectedPlaylist, _currentPatch, shouldRecall);
}

void SetSelectionMode::renderList() const
{
    clearListArea();
    renderHeader();
    renderVisibleSetLists();
}

void SetSelectionMode::renderVisibleSetListPage() const
{
    clearVisibleSetListArea();
    renderVisibleSetLists();
}

void SetSelectionMode::renderHeader() const
{
    renderHeaderLabel();
}

void SetSelectionMode::renderHeaderLabel() const
{
    if (_isLoading)
    {
        updateTrackedTextLabel(_headerLabel, LoadingLabel, TFT_WHITE);
        return;
    }

    char activeLabel[SetListSummary::MaxNameLength + 16] = {};
    std::snprintf(activeLabel, sizeof(activeLabel), "%s%s", LoadedSetPrefix, _activeSetName);
    updateTrackedTextLabel(_headerLabel, activeLabel, _hasActiveSet ? ActiveSetColour : TFT_WHITE);
}

void SetSelectionMode::updateTrackedTextLabel(TrackedTextLabel& trackedLabel, const char* newText,
                                              uint16_t textColour) const
{
    if (trackedLabel.text[0] != '\0')
    {
        _screenUi.drawText(FF22, HeaderScale, trackedLabel.text, listStartX() + RowTextInsetX, centerTopY() + ListPadding,
                           TFT_BLACK, TFT_BLACK);
    }

    if (newText == nullptr || newText[0] == '\0')
    {
        trackedLabel.text[0] = '\0';
        return;
    }

    std::snprintf(trackedLabel.text, sizeof(trackedLabel.text), "%s", newText);
    _screenUi.drawText(FF22, HeaderScale, trackedLabel.text, listStartX() + RowTextInsetX, centerTopY() + ListPadding,
                       textColour, TFT_BLACK);
}

void SetSelectionMode::clearTrackedTextLabel(TrackedTextLabel& trackedLabel) const
{
    if (trackedLabel.text[0] == '\0')
    {
        return;
    }

    _screenUi.drawText(FF22, HeaderScale, trackedLabel.text, listStartX() + RowTextInsetX, centerTopY() + ListPadding,
                       TFT_BLACK, TFT_BLACK);
    trackedLabel.text[0] = '\0';
}

void SetSelectionMode::renderVisibleSetLists() const
{
    const size_t visibleSetListStartIndex = visibleSetListStartIndexFor(_highlightedSetListIndex);
    const size_t visibleSetListCount = ((_setListCount - visibleSetListStartIndex) < visibleSetListCapacity())
                                           ? (_setListCount - visibleSetListStartIndex)
                                           : visibleSetListCapacity();

    for (size_t visibleIndex = 0; visibleIndex < visibleSetListCount; ++visibleIndex)
    {
        renderSetListRow(visibleSetListStartIndex + visibleIndex,
                         (visibleSetListStartIndex + visibleIndex) == _highlightedSetListIndex);
    }
}

void SetSelectionMode::renderSetListRow(size_t setListIndex, bool highlighted) const
{
    if (setListIndex >= _setListCount)
    {
        return;
    }

    const size_t visibleSetListStartIndex = visibleSetListStartIndexFor(setListIndex);
    const size_t visibleSlotIndex = setListIndex - visibleSetListStartIndex;
    const size_t columnIndex = visibleSlotIndex / VisibleRowCount;
    const size_t rowIndex = visibleSlotIndex % VisibleRowCount;
    const int32_t x = columnX(columnIndex);
    const int32_t y = rowY(rowIndex);
    const bool active = isActiveSetList(setListIndex);
    const uint16_t backgroundColour = highlighted ? TFT_WHITE : TFT_BLACK;
    const uint16_t textColour = highlighted ? TFT_BLACK : (active ? ActiveSetColour : TFT_WHITE);

    _screenUi.fillRect(x, y, columnWidth(), CursorHeight, backgroundColour);
    _screenUi.drawText(FF22, RowScale, _setLists[setListIndex].name, x + RowTextInsetX, y + RowTextInsetY, textColour,
                       backgroundColour);
}

void SetSelectionMode::clearListArea() const
{
    _screenUi.fillRect(listStartX(), centerTopY(), listWidth(), centerBottomY() - centerTopY(), TFT_BLACK);
}

void SetSelectionMode::clearVisibleSetListArea() const
{
    _screenUi.fillRect(listStartX(), listStartY(), listWidth(), centerBottomY() - listStartY(), TFT_BLACK);
}

bool SetSelectionMode::setListIndexForFileName(const char* fileName, size_t& setListIndex) const
{
    if (fileName == nullptr || fileName[0] == '\0')
    {
        return false;
    }

    for (size_t index = 0; index < _setListCount; ++index)
    {
        if (std::strcmp(_setLists[index].fileName, fileName) == 0)
        {
            setListIndex = index;
            return true;
        }
    }

    return false;
}

bool SetSelectionMode::isActiveSetList(size_t setListIndex) const
{
    return _hasActiveSet && setListIndex < _setListCount &&
           std::strcmp(_setLists[setListIndex].fileName, _activeSetFileName) == 0;
}

size_t SetSelectionMode::visibleSetListCapacity() const { return VisibleRowCount * ColumnCount; }

size_t SetSelectionMode::visibleSetListStartIndexFor(size_t setListIndex) const
{
    return (setListIndex / visibleSetListCapacity()) * visibleSetListCapacity();
}

int32_t SetSelectionMode::listStartX() const { return ListPadding; }

int32_t SetSelectionMode::centerTopY() const { return _screenUi.boxHeight() + CenterSectionInset; }

int32_t SetSelectionMode::centerBottomY() const { return _screenUi.bottomRowY() - CenterSectionInset; }

int32_t SetSelectionMode::listStartY() const
{
    return centerTopY() + ListPadding + HeaderLineHeight + HeaderGap;
}

int32_t SetSelectionMode::listWidth() const { return (_screenUi.boxWidth() * 4) - (ListPadding * 2); }

int32_t SetSelectionMode::listHeight() const
{
    return centerBottomY() - listStartY() - ListPadding;
}

int32_t SetSelectionMode::rowGap() const
{
    const int32_t totalGapHeight = listHeight() - (static_cast<int32_t>(VisibleRowCount) * RowHeight);
    return totalGapHeight / static_cast<int32_t>(VisibleRowCount - 1);
}

int32_t SetSelectionMode::rowY(size_t visibleRow) const
{
    return listStartY() + (static_cast<int32_t>(visibleRow) * (RowHeight + rowGap()));
}

int32_t SetSelectionMode::columnWidth() const
{
    return listWidth();
}

int32_t SetSelectionMode::columnX(size_t columnIndex) const
{
    return listStartX() + (static_cast<int32_t>(columnIndex) * columnWidth());
}

uint8_t SetSelectionMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}
