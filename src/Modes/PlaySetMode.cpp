#include "Modes/PlaySetMode.h"
#include "ColorUtils.h"

#include <TFT_eSPI.h>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace
{
const char* PlayPatchBadgeTitle = "Patch";
const uint8_t PlayPatchBadgeTitleScale = 1;
const uint8_t PlayPatchBadgeNumberScale = 1;
const uint8_t PlayPatchNameScale = 1;
const uint8_t PlaySongNameScale = 1;
const uint8_t PlaySnapshotLabelScale = 1;

const int32_t PlayPatchBadgeFrameWidth = 118;
const int32_t PlayPatchBadgeFrameHeight = 72;
const int32_t PlayPatchBadgeFrameRadius = 12;
const int32_t PlayPatchBadgeRightMargin = 20;
const int32_t PlaySnapshotLabelOffsetY = 72;
const int32_t PlayPatchBadgeTitleBorderOffset = 5;
const int32_t PlayPatchBadgeNumberOffset = 22;
const int32_t PlayPatchNameOffsetY = 40;
const int32_t PlayPatchNameExtraOffsetY = 10;
const int32_t PlayLabelRowSpacing = 27;
const int32_t PlaySceneWithSongAndPatchExtraOffsetY = 20;
const size_t PlayPatchNameMaxChars = 18;
const char* PlayPatchNamePrefix = "Patch: ";
const char* PlaySongNamePrefix = "Song: ";

const int32_t PlaySnapshotLabelLeftX = 40;
const byte FirstSnapshotButtonIndex = 0;
const byte LastSnapshotButtonIndex = 2;
const byte PatchButtonIndex = 4;
const byte SetButtonIndex = 5;
const uint16_t SetButtonColour = 0x07FF;
const byte GigViewButtonIndex = 6;
const byte TapTempoCc = 44;
const byte TapTempoValue = 100;
const uint32_t TapTempoFlashDurationMs = 10000;
const uint32_t TunerFlashHalfPeriodMs = 500;
const byte TunerButtonIndex = 7;
const uint16_t TunerButtonColour = 0x801F;
const uint32_t SetAdvanceDebounceMs = 200;
const uint32_t SetWrapConfirmWindowMs = 2000;
const int32_t CenterSectionInset = 2;
const int32_t CenterSectionPadding = 6;
const int32_t CenterSplitRightWidth = 150;
const int32_t CenterLine1OffsetY = 46;
const int32_t CenterLine2OffsetY = 98;
const int32_t CenterSceneOffsetY = 128;
const size_t MaxUpcomingRows = 4;

bool isHomePlaylistTransition(ModeTransitionValue transitionValue)
{
    return (transitionValue & ModeTransitionHomePlaylistFlag) != 0;
}
} // namespace

PlaySetMode::PlaySetMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                         IMidiManager& midiManager, IMidiProvider& midiProvider,
                         IButtonOverrideStore& buttonOverrideStore, ISetListStore& setListStore,
                         IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _buttonOverrideStore(buttonOverrideStore), _setListStore(setListStore),
      _selectedPreset(0),
      _selectedPlaylist(midiProvider.defaultPlaylistIndex()), _selectedSongIndex(0), _hasSelectedPreset(false),
      _hasSelectedSong(false), _hasSelectedSetSong(false), _selectedSetSongUnavailable(false),
      _showEndOfSetPrompt(false), _isPlaySetMode(false),
      _selectedButton(0), _lastSetAdvancePressedAtMs(0), _setWrapConfirmUntilMs(0),
      _tapTempoEngine(), _nextTapTempoFlashToggleMs(0), _tapTempoFlashUntilMs(0), _isTapTempoLit(true),
      _nextTunerFlashToggleMs(0), _isTunerEnabled(false), _isGigViewEnabled(false), _isTunerFlashLit(true),
      _toggleStates{false, false, false, false, false, false, false, false},
      _patchDisplayConfig(), _selectedSongDisplayName{0}, _songNameLabel{{0}}, _patchNameLabel{{0}}, _snapshotLabel{{0}},
      _tapTempoDisplayLabel{0}
{
    setupFunctions();
}

void PlaySetMode::setSelectedPreset(byte selectedPreset)
{
    _selectedPreset = selectedPreset;
    _hasSelectedPreset = true;
    _selectedSongIndex = 0;
    _hasSelectedSong = false;
    _hasSelectedSetSong = false;
    _selectedSetSongUnavailable = false;
    _showEndOfSetPrompt = false;
    _isPlaySetMode = false;
    _setWrapConfirmUntilMs = 0;
    _selectedSongDisplayName[0] = '\0';
}

void PlaySetMode::setTransitionValue(ModeTransitionValue transitionValue)
{
    if (transitionValue == ModeTransitionNone)
    {
        _selectedPlaylist = _midiProvider.defaultPlaylistIndex();
        _hasSelectedPreset = false;
        _selectedSongIndex = 0;
        _hasSelectedSong = false;
        _hasSelectedSetSong = false;
        _selectedSetSongUnavailable = false;
        _isPlaySetMode = false;
        _setWrapConfirmUntilMs = 0;
        _selectedSongDisplayName[0] = '\0';
        return;
    }

    if (isPlayModeTransition(transitionValue))
    {
        _selectedPlaylist = playModeTransitionPlaylist(transitionValue);
        _selectedPreset = playModeTransitionPatch(transitionValue);
        _hasSelectedPreset = playModeTransitionShouldRecall(transitionValue);
        _selectedSongDisplayName[0] = '\0';
        _selectedSetSongUnavailable = false;
        _showEndOfSetPrompt = false;
        _hasSelectedSetSong = isPlayModeSetSongTransition(transitionValue);
        if (_hasSelectedSetSong)
        {
            _selectedSongIndex = 0;
            _hasSelectedSong = false;
            return;
        }
        if (isPlayModeSongTransition(transitionValue))
        {
            _selectedSongIndex = playModeTransitionSongIndex(transitionValue);
            _hasSelectedSong = true;
            return;
        }

        _selectedSongIndex = 0;
        _hasSelectedSong = false;
        return;
    }

    if ((transitionValue & ModeTransitionPatchReturnFlag) != 0)
    {
        _selectedPreset = static_cast<byte>(transitionValue & ModeTransitionPatchValueMask);
        _hasSelectedPreset = false;
        _selectedSongIndex = 0;
        _hasSelectedSong = false;
        _hasSelectedSetSong = false;
        _selectedSetSongUnavailable = false;
        _showEndOfSetPrompt = false;
        _isPlaySetMode = false;
        _setWrapConfirmUntilMs = 0;
        _selectedSongDisplayName[0] = '\0';
        return;
    }

    if (isHomePlaylistTransition(transitionValue))
    {
        _selectedPlaylist = static_cast<byte>(transitionValue & ModeTransitionHomePlaylistValueMask);
        setSelectedPreset(0);
        return;
    }

    _selectedPlaylist = _midiProvider.defaultPlaylistIndex();
    setSelectedPreset(static_cast<byte>(transitionValue));
}

void PlaySetMode::setupFunctions()
{
    _functions[0] = Function("Clean", 0x07E0, ActionType::SelectScene, 0, ActionType::None, 0);
    _functions[1] = Function("Crunch", 0xFD20, ActionType::SelectScene, 1, ActionType::None, 0);
    _functions[2] = Function("Lead", 0xF800, ActionType::SelectScene, 2, ActionType::None, 0);

    _functions[3] = Function();
    configurePatchButton();
    configureSetButton();
    _functions[GigViewButtonIndex] = Function();
    _functions[TunerButtonIndex] =
        Function("Tuner", TunerButtonColour, ActionType::SetTuner, 1, ActionType::SetGigView, 1);
    _functions[TunerButtonIndex].setToggle(true);
}

void PlaySetMode::configurePatchButton()
{
    _functions[4] = Function("Patch", 0xFFE0, ActionType::ChangeMode, static_cast<byte>(Modes::Patches),
                             ActionType::ChangeMode, static_cast<byte>(Modes::Patch));
}

void PlaySetMode::configureSetButton()
{
    _functions[SetButtonIndex] =
        Function("Set", SetButtonColour, ActionType::ChangeMode, static_cast<byte>(Modes::Songs), ActionType::None, 0);
}

void PlaySetMode::configureManageSetButton()
{
    _functions[TunerButtonIndex] =
        Function("Set", SetButtonColour, ActionType::ChangeMode, static_cast<byte>(Modes::Songs), ActionType::None, 0);
    _functions[TunerButtonIndex].setToggle(false);
}

void PlaySetMode::configurePlaySetButtons()
{
    _functions[SetButtonIndex] = Function("Next", SetButtonColour, ActionType::None, 0, ActionType::None, 0);
    configureManageSetButton();
}

void PlaySetMode::refreshPlaySetState()
{
    size_t songCount = 0;
    size_t selectedSongIndex = 0;
    _isPlaySetMode = _setListStore.activeSetPosition(_selectedPlaylist, songCount, selectedSongIndex) && songCount > 0;
    Serial.printf("[PlaySetDiag] refresh playlist=%u active=%u songCount=%u selectedIndex=%u\n",
                  static_cast<unsigned int>(_selectedPlaylist), _isPlaySetMode ? 1U : 0U, static_cast<unsigned int>(songCount),
                  static_cast<unsigned int>(selectedSongIndex));
    if (!_isPlaySetMode)
    {
        return;
    }

    if (!_hasSelectedSetSong && !_hasSelectedSong && !_hasSelectedPreset)
    {
        _hasSelectedSetSong = true;
    }

    if (_hasSelectedSetSong)
    {
        _hasSelectedSong = false;
    }
}

bool PlaySetMode::resolveSelectedSetSong()
{
    _selectedSongDisplayName[0] = '\0';
    _selectedSetSongUnavailable = false;
    if (!_hasSelectedSetSong)
    {
        return false;
    }

    SetListSongEntry setSong = {};
    if (!_setListStore.selectedSong(_selectedPlaylist, setSong))
    {
        Serial.printf("[PlaySetDiag] resolveSelectedSetSong failed playlist=%u\n",
                      static_cast<unsigned int>(_selectedPlaylist));
        _hasSelectedSetSong = false;
        return false;
    }

    Serial.printf("[PlaySetDiag] resolveSelectedSetSong ok playlist=%u song='%s' available=%u patch=%u songIndex=%u\n",
                  static_cast<unsigned int>(_selectedPlaylist), setSong.name, setSong.available ? 1U : 0U,
                  static_cast<unsigned int>(setSong.patch), static_cast<unsigned int>(setSong.songIndex));
    std::snprintf(_selectedSongDisplayName, sizeof(_selectedSongDisplayName), "%s", setSong.name);
    _selectedSetSongUnavailable = !setSong.available;
    if (setSong.available)
    {
        _selectedPreset = setSong.patch;
        _hasSelectedPreset = true;
    }
    return true;
}

bool PlaySetMode::resolveSelectedSong()
{
    _selectedSongDisplayName[0] = '\0';
    if (!_hasSelectedSong)
    {
        return false;
    }

    SongConfig song = {};
    if (!_buttonOverrideStore.songForIndex(_selectedPlaylist, _selectedSongIndex, &song))
    {
        _selectedPreset = 0;
        _hasSelectedPreset = false;
        _hasSelectedSong = false;
        _selectedSongIndex = 0;
        return false;
    }

    _selectedPreset = song.patchNumber;
    std::snprintf(_selectedSongDisplayName, sizeof(_selectedSongDisplayName), "%s", song.displayName);
    return true;
}

ModeTransitionValue PlaySetMode::currentPlayTransitionValue(bool shouldRecall) const
{
    if (_hasSelectedSetSong)
    {
        return makePlayModeSetSongTransition(_selectedPlaylist, _selectedPreset, shouldRecall);
    }

    return _hasSelectedSong ? makePlayModeSongTransition(_selectedPlaylist, _selectedSongIndex, shouldRecall)
                            : makePlayModeTransition(_selectedPlaylist, _selectedPreset, shouldRecall);
}

void PlaySetMode::activate()
{
    size_t setSongCount = 0;
    size_t setSelectedSongIndex = 0;
    if (!_setListStore.activeSetPosition(_selectedPlaylist, setSongCount, setSelectedSongIndex) || setSongCount == 0)
    {
        _transitionDelegate.enterMode(Modes::Play, currentPlayTransitionValue(_hasSelectedPreset));
        return;
    }

    _midiProvider.selectPlaylist(_selectedPlaylist);
    setupFunctions();
    _patchDisplayConfig.name[0] = '\0';
    _selectedSongDisplayName[0] = '\0';
    _selectedSetSongUnavailable = false;
    _showEndOfSetPrompt = false;
    _setWrapConfirmUntilMs = 0;
    _lastSetAdvancePressedAtMs = 0;
    _buttonOverrideStore.refresh();
    refreshPlaySetState();
    if (_isPlaySetMode)
    {
        resolveSelectedSetSong();
    }
    else if (!resolveSelectedSetSong())
    {
        resolveSelectedSong();
    }
    _buttonOverrideStore.applyOverrides(_selectedPlaylist, _selectedPreset, _functions, TouchButtonManager::BUTTON_COUNT,
                                        &_patchDisplayConfig);
    configurePatchButton();
    if (_isPlaySetMode)
    {
        configurePlaySetButtons();
    }
    else
    {
        configureSetButton();
    }
    _isTapTempoLit = true;
    const uint32_t now = millis();
    if (_tapTempoEngine.hasFlashInterval() && _tapTempoFlashUntilMs != 0 &&
        static_cast<int32_t>(now - _tapTempoFlashUntilMs) < 0)
    {
        _nextTapTempoFlashToggleMs = now + _tapTempoEngine.flashHalfPeriodMs();
    }
    else
    {
        _tapTempoFlashUntilMs = 0;
    }

    if (_isTunerEnabled)
    {
        _isTunerFlashLit = true;
        _nextTunerFlashToggleMs = now + TunerFlashHalfPeriodMs;
    }
    else
    {
        _isTunerFlashLit = true;
        _nextTunerFlashToggleMs = 0;
    }

    if (_hasSelectedPreset)
    {
        _midiProvider.recallPreset(_selectedPreset);
    }

    const int8_t firstSceneButton = firstSceneSelectionButton();
    if (!isSceneSelectionButton(_selectedButton))
    {
        _selectedButton = (firstSceneButton >= 0) ? static_cast<byte>(firstSceneButton) : 0;
    }

    if (firstSceneButton >= 0)
    {
        _midiProvider.selectScene(_selectedButton);
    }

    updateVisuals();
    renderPlayCenterUi();
}

void PlaySetMode::deactivate()
{
    clearTapTempoState();
    clearPlayCenterUi();
}

void PlaySetMode::renderPlayCenterUi()
{
    const int32_t centerX = CenterSectionInset + CenterSectionPadding;
    const int32_t centerY = _screenUi.boxHeight() + CenterSectionInset;
    const int32_t centerWidth = (_screenUi.boxWidth() * 4) - ((CenterSectionInset + CenterSectionPadding) * 2);
    const int32_t centerHeight = (_screenUi.bottomRowY() - CenterSectionInset) - centerY;
    const int32_t splitX = centerX + (centerWidth - CenterSplitRightWidth);
    const int32_t leftX = centerX + CenterSectionPadding;
    const int32_t rightX = splitX + CenterSectionPadding;

    _screenUi.fillRect(centerX, centerY, centerWidth, centerHeight, TFT_BLACK);
    _screenUi.drawRect(centerX, centerY, centerWidth, centerHeight, TFT_DARKGREY);
    _screenUi.fillRect(splitX, centerY + 1, 1, centerHeight - 2, TFT_DARKGREY);
    _screenUi.fillRect(centerX, centerY + CenterLine1OffsetY, splitX - centerX, 1, TFT_DARKGREY);
    _screenUi.fillRect(centerX, centerY + CenterLine2OffsetY, splitX - centerX, 1, TFT_DARKGREY);

    SetListSongEntry currentSong = {};
    size_t songCount = 0;
    size_t selectedSongIndex = 0;
    const bool hasSetPosition = _setListStore.activeSetPosition(_selectedPlaylist, songCount, selectedSongIndex);
    const bool hasCurrentSong =
        hasSetPosition && selectedSongIndex < songCount && _setListStore.activeSetSongAt(_selectedPlaylist, selectedSongIndex, currentSong);

    uint16_t currentPart = hasCurrentSong ? currentSong.part : 0;
    size_t songInPart = 0;
    size_t songsInPart = 0;
    if (hasCurrentSong)
    {
        SetListSongEntry song = {};
        for (size_t index = 0; index < songCount; ++index)
        {
            if (!_setListStore.activeSetSongAt(_selectedPlaylist, index, song) || song.part != currentPart)
            {
                continue;
            }

            ++songsInPart;
            if (index <= selectedSongIndex)
            {
                ++songInPart;
            }
        }
    }

    char songLine[TrackedTextLabel::Capacity] = {};
    if (hasCurrentSong)
    {
        std::snprintf(songLine, sizeof(songLine), "S: %s (%u/%u)", currentSong.name, static_cast<unsigned int>(songInPart),
                      static_cast<unsigned int>(songsInPart));
    }
    else
    {
        std::snprintf(songLine, sizeof(songLine), "%s", "S: No active song");
    }
    _screenUi.drawText(FF22, 1, songLine, leftX, centerY + 18, _selectedSetSongUnavailable ? TFT_RED : TFT_WHITE, TFT_BLACK);

    char patchLine[TrackedTextLabel::Capacity] = {};
    if (_patchDisplayConfig.name[0] != '\0')
    {
        std::snprintf(patchLine, sizeof(patchLine), "P: %s", _patchDisplayConfig.name);
    }
    else
    {
        std::snprintf(patchLine, sizeof(patchLine), "P: %u", static_cast<unsigned int>(_selectedPreset));
    }
    _screenUi.drawText(FF22, 1, patchLine, leftX, centerY + 36, TFT_WHITE, TFT_BLACK);

    const char* notesLabel = _selectedSetSongUnavailable ? "Notes: unresolved" : "Notes: -";
    _screenUi.drawText(FF22, 1, notesLabel, leftX, centerY + 66, _selectedSetSongUnavailable ? TFT_RED : TFT_WHITE, TFT_BLACK);

    char sceneLine[TrackedTextLabel::Capacity] = {};
    if (_showEndOfSetPrompt)
    {
        std::snprintf(sceneLine, sizeof(sceneLine), "%s", "END OF SET");
        _screenUi.drawText(FF32, 1, sceneLine, leftX, centerY + CenterSceneOffsetY, SetButtonColour, TFT_BLACK);
    }
    else if (isSceneSelectionButton(_selectedButton))
    {
        char sceneLabel[16] = {};
        formatSnapshotLabelUppercase(_selectedButton, sceneLabel, sizeof(sceneLabel));
        std::snprintf(sceneLine, sizeof(sceneLine), "Scene: %s", sceneLabel);
        _screenUi.drawText(FF22, 1, sceneLine, leftX, centerY + CenterSceneOffsetY, TFT_WHITE, TFT_BLACK);
    }

    char partName[SetListPart::MaxNameLength] = {};
    if (hasCurrentSong && !_setListStore.activeSetPartName(_selectedPlaylist, currentPart, partName, sizeof(partName)))
    {
        std::snprintf(partName, sizeof(partName), "Part %u", static_cast<unsigned int>(currentPart));
    }
    _screenUi.drawText(FF22, 1, partName, rightX, centerY + 18, TFT_WHITE, TFT_BLACK);

    size_t shownUpcoming = 0;
    if (hasCurrentSong)
    {
        SetListSongEntry upcomingSong = {};
        for (size_t index = selectedSongIndex + 1; index < songCount && shownUpcoming < MaxUpcomingRows; ++index)
        {
            if (!_setListStore.activeSetSongAt(_selectedPlaylist, index, upcomingSong))
            {
                continue;
            }

            if (upcomingSong.part != currentPart)
            {
                _screenUi.fillRect(rightX, centerY + 34 + static_cast<int32_t>(shownUpcoming * 20), CenterSplitRightWidth - 16, 1,
                                   TFT_DARKGREY);
                break;
            }

            char upcomingLine[TrackedTextLabel::Capacity] = {};
            std::snprintf(upcomingLine, sizeof(upcomingLine), "%u %s", static_cast<unsigned int>(upcomingSong.number),
                          upcomingSong.name);
            _screenUi.drawText(FF22, 1, upcomingLine, rightX, centerY + 34 + static_cast<int32_t>(shownUpcoming * 20), TFT_WHITE,
                               TFT_BLACK);
            ++shownUpcoming;
        }
    }
}

void PlaySetMode::clearPlayCenterUi()
{
    const int32_t centerX = CenterSectionInset + CenterSectionPadding;
    const int32_t centerY = _screenUi.boxHeight() + CenterSectionInset;
    const int32_t centerWidth = (_screenUi.boxWidth() * 4) - ((CenterSectionInset + CenterSectionPadding) * 2);
    const int32_t centerHeight = (_screenUi.bottomRowY() - CenterSectionInset) - centerY;
    _screenUi.fillRect(centerX, centerY, centerWidth, centerHeight, TFT_BLACK);
    _songNameLabel.text[0] = '\0';
    _patchNameLabel.text[0] = '\0';
    _snapshotLabel.text[0] = '\0';
}

void PlaySetMode::renderPatchBadge()
{
    const PatchBadgeMetrics metrics = patchBadgeMetrics();

    _screenUi.drawCenteredFrame(metrics.frameCenterX, metrics.frameTopY, PlayPatchBadgeFrameWidth,
                                PlayPatchBadgeFrameHeight,
                                PlayPatchBadgeFrameRadius);
    _screenUi.drawCenteredText(FF22, PlayPatchBadgeTitleScale, PlayPatchBadgeTitle, metrics.frameCenterX,
                               metrics.titleY, TFT_WHITE, TFT_BLACK);
    renderPatchBadgeNumber(_selectedPreset, TFT_WHITE);
}

void PlaySetMode::clearPatchBadge()
{
    const PatchBadgeMetrics metrics = patchBadgeMetrics();

    _screenUi.drawCenteredFrame(metrics.frameCenterX, metrics.frameTopY, PlayPatchBadgeFrameWidth,
                                PlayPatchBadgeFrameHeight,
                                PlayPatchBadgeFrameRadius, TFT_BLACK, TFT_BLACK);
    _screenUi.drawCenteredText(FF22, PlayPatchBadgeTitleScale, PlayPatchBadgeTitle, metrics.frameCenterX,
                               metrics.titleY, TFT_BLACK, TFT_BLACK);
    renderPatchBadgeNumber(_selectedPreset, TFT_BLACK);
}

void PlaySetMode::renderPatchBadgeNumber(byte patchNumber, uint16_t textColour)
{
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t numberY = patchBadgeFrameTopY() + PlayPatchBadgeNumberOffset;

    char patchLabel[4] = {'0', '0', '\0', '\0'};
    formatPatchNumberLabel(patchNumber, patchLabel, sizeof(patchLabel));
    _screenUi.drawCenteredText(FF32, PlayPatchBadgeNumberScale, patchLabel, frameCenterX, numberY, textColour,
                               TFT_BLACK);
}

void PlaySetMode::renderSongNameLabel(uint16_t textColour)
{
    char songNameLabel[TrackedTextLabel::Capacity] = {'\0'};
    formatSongNameDisplayLabel(_selectedSongDisplayName, songNameLabel, sizeof(songNameLabel));
    updateTrackedTextLabel(_songNameLabel, songNameLabel, FF22, PlaySongNameScale, songAndPatchLabelLeftX(),
                           songNameLabelY(), textColour);
}

void PlaySetMode::renderSnapshotLabel(byte snapshotButton, uint16_t textColour)
{
    if (!isSceneSelectionButton(snapshotButton))
    {
        clearTrackedTextLabel(_snapshotLabel, FF32, PlaySnapshotLabelScale, PlaySnapshotLabelLeftX, snapshotLabelY());
        return;
    }

    char snapshotLabel[16] = {'\0'};
    formatSnapshotLabelUppercase(snapshotButton, snapshotLabel, sizeof(snapshotLabel));
    updateTrackedTextLabel(_snapshotLabel, snapshotLabel, FF32, PlaySnapshotLabelScale, PlaySnapshotLabelLeftX,
                           snapshotLabelY(), textColour);
}

void PlaySetMode::formatPatchNumberLabel(byte patchNumber, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    std::snprintf(buffer, bufferSize, "%02u", static_cast<unsigned int>(patchNumber));
}

void PlaySetMode::formatSongNameDisplayLabel(const char* songName, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0U)
    {
        return;
    }

    if (songName == nullptr || songName[0] == '\0')
    {
        buffer[0] = '\0';
        return;
    }

    const size_t songNameLength = std::strlen(songName);
    if (songNameLength <= PlayPatchNameMaxChars)
    {
        std::snprintf(buffer, bufferSize, "%s%s", PlaySongNamePrefix, songName);
        return;
    }

    std::snprintf(buffer, bufferSize, "%s%.*s...", PlaySongNamePrefix, static_cast<int>(PlayPatchNameMaxChars),
                  songName);
}

void PlaySetMode::formatPatchNameDisplayLabel(const char* patchName, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0U)
    {
        return;
    }

    if (patchName == nullptr || patchName[0] == '\0')
    {
        buffer[0] = '\0';
        return;
    }

    const size_t patchNameLength = std::strlen(patchName);
    if (patchNameLength <= PlayPatchNameMaxChars)
    {
        std::snprintf(buffer, bufferSize, "%s%s", PlayPatchNamePrefix, patchName);
        return;
    }

    std::snprintf(buffer, bufferSize, "%s%.*s...", PlayPatchNamePrefix, static_cast<int>(PlayPatchNameMaxChars),
                  patchName);
}

void PlaySetMode::formatSnapshotLabelUppercase(byte snapshotButton, char* buffer, size_t bufferSize) const
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    const char* source = getFunction(snapshotButton).label();
    if (source == nullptr)
    {
        buffer[0] = '\0';
        return;
    }

    size_t out = 0;
    while (source[out] != '\0' && (out + 1) < bufferSize)
    {
        buffer[out] = static_cast<char>(std::toupper(static_cast<unsigned char>(source[out])));
        ++out;
    }
    buffer[out] = '\0';
}

void PlaySetMode::updateTrackedTextLabel(TrackedTextLabel& trackedLabel, const char* newText, const GFXfont* font,
                                      uint8_t scale, int32_t x, int32_t y, uint16_t textColour)
{
    if (trackedLabel.text[0] != '\0')
    {
        _screenUi.drawText(font, scale, trackedLabel.text, x, y, TFT_BLACK, TFT_BLACK);
    }

    if (newText == nullptr || newText[0] == '\0')
    {
        trackedLabel.text[0] = '\0';
        return;
    }

    std::snprintf(trackedLabel.text, sizeof(trackedLabel.text), "%s", newText);
    _screenUi.drawText(font, scale, trackedLabel.text, x, y, textColour, TFT_BLACK);
}

void PlaySetMode::clearTrackedTextLabel(TrackedTextLabel& trackedLabel, const GFXfont* font, uint8_t scale, int32_t x,
                                     int32_t y)
{
    if (trackedLabel.text[0] == '\0')
    {
        return;
    }

    _screenUi.drawText(font, scale, trackedLabel.text, x, y, TFT_BLACK, TFT_BLACK);
    trackedLabel.text[0] = '\0';
}

int32_t PlaySetMode::patchBadgeFrameCenterX() const
{
    const int32_t screenWidth = _screenUi.boxWidth() * 4;
    return screenWidth - PlayPatchBadgeRightMargin - (PlayPatchBadgeFrameWidth / 2);
}

PlaySetMode::PatchBadgeMetrics PlaySetMode::patchBadgeMetrics() const
{
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t frameTopY = patchBadgeFrameTopY();

    PatchBadgeMetrics metrics = {};
    metrics.frameCenterX = frameCenterX;
    metrics.frameTopY = frameTopY;
    metrics.titleY = frameTopY - PlayPatchBadgeTitleBorderOffset - 2;
    return metrics;
}

int32_t PlaySetMode::patchBadgeFrameTopY() const
{
    const int32_t centerTopY = _screenUi.boxHeight();
    const int32_t centerHeight = _screenUi.bottomRowY() - centerTopY;

    // Current alignment gap from center top (before swap).
    const int32_t topGap = PlaySnapshotLabelOffsetY - (PlayPatchBadgeFrameHeight / 2);
    // Swap top and bottom gaps: newTop = centerHeight - oldTop - frameHeight.
    const int32_t swappedTopGap = centerHeight - topGap - PlayPatchBadgeFrameHeight;

    return centerTopY + swappedTopGap;
}

int32_t PlaySetMode::songNameLabelY() const
{
    return _screenUi.boxHeight() + PlayPatchNameOffsetY - PlayLabelRowSpacing;
}

int32_t PlaySetMode::songAndPatchLabelLeftX() const
{
    const int32_t pillWidth = _screenUi.boxWidth() * 3 / 5;
    return (_screenUi.boxWidth() / 2) - (pillWidth / 2);
}

int32_t PlaySetMode::patchNameLabelY() const
{
    return _screenUi.boxHeight() + PlayPatchNameOffsetY + PlayPatchNameExtraOffsetY;
}

int32_t PlaySetMode::snapshotLabelY() const
{
    if (hasSongDisplayName() && hasPatchDisplayName())
    {
        return _screenUi.boxHeight() + PlaySnapshotLabelOffsetY + PlaySceneWithSongAndPatchExtraOffsetY;
    }

    if (hasSongDisplayName() || hasPatchDisplayName())
    {
        return patchNameLabelY() + PlayLabelRowSpacing;
    }

    return _screenUi.boxHeight() + PlaySnapshotLabelOffsetY;
}

bool PlaySetMode::hasSongDisplayName() const { return _selectedSongDisplayName[0] != '\0'; }

bool PlaySetMode::hasPatchDisplayName() const { return _patchDisplayConfig.name[0] != '\0'; }

void PlaySetMode::updateVisuals() { renderAllButtons(); }

void PlaySetMode::renderButton(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    FootSwitchTouchButton* button = _touchButtonManager.getButton(number);
    if (button == nullptr)
    {
        return;
    }

    const Function& func = getFunction(number);
    const bool enabled = isButtonEnabled(number);
    const uint8_t ringBrightness = enabled ? ringBrightnessForButton(number) : 0;

    button->setEnabled(enabled);
    button->setLabel(func.label());

    if (!enabled || ringBrightness == 0)
    {
        button->setPillColour(0);
        button->setBorderVisible(false);
        _ringManager.setRingColour(number, 0);
        _ringManager.setRingBrightness(number, 0);
    }
    else
    {
        const uint16_t colour565 = func.colour();
        button->setPillColour(colour565);
        button->setBorderVisible(usesSelectionBorder(number) && (ringBrightness >= RingManager::FullBrightness));
        _ringManager.setRingColour(number, ColorUtils::rgb565To888(colour565));
        _ringManager.setRingBrightness(number, ringBrightness);
    }

    button->draw(_screenUi);
}

void PlaySetMode::updateSnapshotSelectionVisuals(byte previousSelected, byte currentSelected)
{
    if (previousSelected >= FirstSnapshotButtonIndex && previousSelected <= LastSnapshotButtonIndex)
    {
        renderButton(previousSelected);
    }

    if (currentSelected != previousSelected && currentSelected >= FirstSnapshotButtonIndex &&
        currentSelected <= LastSnapshotButtonIndex)
    {
        renderButton(currentSelected);
    }

    renderSnapshotLabel(currentSelected, TFT_WHITE);
}

bool PlaySetMode::usesSelectionBorder(byte number) const
{
    return isSceneSelectionButton(number);
}

int8_t PlaySetMode::firstSceneSelectionButton() const
{
    for (byte buttonIndex = FirstSnapshotButtonIndex; buttonIndex <= LastSnapshotButtonIndex; ++buttonIndex)
    {
        if (isSceneSelectionButton(buttonIndex))
        {
            return static_cast<int8_t>(buttonIndex);
        }
    }

    return -1;
}

bool PlaySetMode::isSceneSelectionButton(byte number) const
{
    if (number < FirstSnapshotButtonIndex || number > LastSnapshotButtonIndex || !isButtonEnabled(number))
    {
        return false;
    }

    const Function& func = getFunction(number);
    return !func.hasMomentaryBehaviour() &&
           func.action(FunctionBehaviour::ShortPress).type == ActionType::SelectScene;
}

bool PlaySetMode::isTapTempoButton(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return false;
    }

    return getFunction(number).action(FunctionBehaviour::ShortPress).type == ActionType::TapTempo;
}

ActionType PlaySetMode::toggleActionTypeForButton(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return ActionType::None;
    }

    const Function& func = getFunction(number);
    if (func.hasMomentaryBehaviour())
    {
        return ActionType::None;
    }

    const ActionType actionType = func.action(FunctionBehaviour::ShortPress).type;
    switch (actionType)
    {
    case ActionType::SetTuner:
    case ActionType::SetGigView:
        return actionType;
    default:
        return ActionType::None;
    }
}

bool PlaySetMode::isToggleButton(byte number) const
{
    return number < TouchButtonManager::BUTTON_COUNT && isButtonEnabled(number) && getFunction(number).isToggle() &&
           !getFunction(number).hasMomentaryBehaviour();
}

bool PlaySetMode::isToggleActionEnabled(ActionType actionType) const
{
    switch (actionType)
    {
    case ActionType::SetTuner:
        return _isTunerEnabled;
    case ActionType::SetGigView:
        return _isGigViewEnabled;
    default:
        return false;
    }
}

uint8_t PlaySetMode::ringBrightnessForButton(byte number) const
{
    if (isTapTempoButton(number))
    {
        return _isTapTempoLit ? RingManager::FullBrightness : 0;
    }

    if (isToggleButton(number))
    {
        switch (toggleActionTypeForButton(number))
        {
        case ActionType::SetTuner:
            return (!_isTunerEnabled || _isTunerFlashLit) ? RingManager::FullBrightness : 0;
        case ActionType::SetGigView:
            return _isGigViewEnabled ? RingManager::FullBrightness : RingManager::DimBrightness;
        default:
            return _toggleStates[number] ? RingManager::FullBrightness : RingManager::DimBrightness;
        }
    }

    if (number == PatchButtonIndex)
    {
        return RingManager::FullBrightness;
    }

    if (isSceneSelectionButton(number))
    {
        return number == _selectedButton ? RingManager::FullBrightness : RingManager::DimBrightness;
    }

    return RingManager::FullBrightness;
}

void PlaySetMode::buttonDown(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return;
    }

    const Function& func = getFunction(number);
    if (!func.hasMomentaryBehaviour())
    {
        return;
    }

    executeAction(func.action(FunctionBehaviour::ButtonDown));
}

void PlaySetMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        return;
    }

    if (_isPlaySetMode)
    {
        if (number == SetButtonIndex)
        {
            handleNextSetSongPress();
            return;
        }

        if (number == TunerButtonIndex)
        {
            _transitionDelegate.enterMode(Modes::Songs, currentPlayTransitionValue(false));
            return;
        }
    }

    const Function& func = getFunction(number);
    if (func.hasMomentaryBehaviour())
    {
        return;
    }

    const ActionType toggleActionType = toggleActionTypeForButton(number);
    if (isToggleButton(number))
    {
        if (toggleActionType != ActionType::None)
        {
            toggleAction(toggleActionType);
            return;
        }

        _toggleStates[number] = !_toggleStates[number];
        executeAction(func.action(FunctionBehaviour::ShortPress));
        renderButton(number);
        return;
    }

    const FunctionAction& shortPressAction = func.action(FunctionBehaviour::ShortPress);
    if (shortPressAction.type == ActionType::ChangeMode)
    {
        executeAction(shortPressAction);
        return;
    }

    executeAction(shortPressAction);

    if (shortPressAction.type != ActionType::SelectScene || !isSceneSelectionButton(number))
    {
        return;
    }

    const byte previousSelected = _selectedButton;
    _selectedButton = number;
    updateSnapshotSelectionVisuals(previousSelected, _selectedButton);
}

void PlaySetMode::buttonLongPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        return;
    }

    if (_isPlaySetMode && number == TunerButtonIndex)
    {
        toggleAction(ActionType::SetTuner);
        return;
    }

    const Function& func = getFunction(number);
    if (func.hasMomentaryBehaviour())
    {
        return;
    }

    const FunctionAction& longPressAction = func.action(FunctionBehaviour::LongPress);
    if (isToggleButton(number))
    {
        const ActionType toggleActionType = toggleActionTypeForButton(number);
        if ((longPressAction.type == ActionType::SetTuner || longPressAction.type == ActionType::SetGigView) &&
            longPressAction.type != toggleActionType)
        {
            toggleAction(longPressAction.type);
        }
        return;
    }

    executeAction(longPressAction);
}

void PlaySetMode::buttonReleased(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return;
    }

    const Function& func = getFunction(number);
    if (!func.hasMomentaryBehaviour())
    {
        return;
    }

    executeAction(func.action(FunctionBehaviour::ButtonRelease));
}

void PlaySetMode::frameTick()
{
    const uint32_t now = millis();
    if (_tapTempoEngine.tick(now).shouldSendMidi)
    {
        _midiManager.sendControlChange(TapTempoCc, TapTempoValue);
    }

    if (_tapTempoFlashUntilMs != 0 && static_cast<int32_t>(now - _tapTempoFlashUntilMs) >= 0)
    {
        _tapTempoFlashUntilMs = 0;
        if (!_isTapTempoLit)
        {
            _isTapTempoLit = true;
            renderTapTempoButtons(false);
        }
    }
    else if (_tapTempoEngine.hasFlashInterval() && _tapTempoFlashUntilMs != 0 &&
             static_cast<int32_t>(now - _nextTapTempoFlashToggleMs) >= 0)
    {
        _isTapTempoLit = !_isTapTempoLit;
        _nextTapTempoFlashToggleMs = now + _tapTempoEngine.flashHalfPeriodMs();
        renderTapTempoButtons(false);
    }

    if (_isTunerEnabled && static_cast<int32_t>(now - _nextTunerFlashToggleMs) >= 0)
    {
        _isTunerFlashLit = !_isTunerFlashLit;
        _nextTunerFlashToggleMs = now + TunerFlashHalfPeriodMs;
        renderToggleButtons(ActionType::SetTuner, false);
    }
}

void PlaySetMode::encoderPressed() { _transitionDelegate.enterMode(Modes::Home, ModeTransitionNone); }

void PlaySetMode::executeAction(const FunctionAction& action)
{
    switch (action.type)
    {
    case ActionType::None:
        break;
    case ActionType::SendMidiProgramChange:
        _midiManager.sendProgramChange(action.value);
        break;
    case ActionType::SendMidiControlChange:
        _midiManager.sendControlChange(action.value, action.secondaryValue);
        break;
    case ActionType::SelectScene:
        _midiProvider.selectScene(action.value);
        break;
    case ActionType::SetTuner:
        _midiProvider.setTunerEnabled(action.value != 0);
        break;
    case ActionType::SetGigView:
        _midiProvider.setGigViewEnabled(action.value != 0);
        break;
    case ActionType::SelectHomePlaylist:
        break;
    case ActionType::TapTempo:
        registerTapTempoPress(millis());
        break;
    case ActionType::ChangeMode:
    {
        const Modes targetMode = static_cast<Modes>(action.value);
        if (targetMode == Modes::Home)
        {
            _transitionDelegate.enterMode(targetMode, ModeTransitionNone);
        }
        else if (targetMode == Modes::Patch)
        {
            _transitionDelegate.enterMode(targetMode, _selectedPreset);
        }
        else if (targetMode == Modes::Patches)
        {
            _transitionDelegate.enterMode(targetMode,
                                          makePlayModeSetSongTransition(_selectedPlaylist, _selectedPreset, false));
        }
        else if (targetMode == Modes::Songs)
        {
            _transitionDelegate.enterMode(targetMode, currentPlayTransitionValue(false));
        }
        break;
    }
    }
}

void PlaySetMode::toggleAction(ActionType actionType)
{
    const bool nextEnabled = !isToggleActionEnabled(actionType);
    switch (actionType)
    {
    case ActionType::SetTuner:
        _isTunerEnabled = nextEnabled;
        _isTunerFlashLit = true;
        _nextTunerFlashToggleMs = nextEnabled ? (millis() + TunerFlashHalfPeriodMs) : 0U;
        break;
    case ActionType::SetGigView:
        _isGigViewEnabled = nextEnabled;
        break;
    default:
        return;
    }

    executeAction(FunctionAction(actionType, nextEnabled ? 1U : 0U, 0U));
    renderToggleButtons(actionType, true);
}

void PlaySetMode::registerTapTempoPress(uint32_t pressedAtMs)
{
    _tapTempoEngine.registerPress(pressedAtMs);

    if (_tapTempoEngine.hasFlashInterval())
    {
        formatTapTempoBpmLabel(_tapTempoEngine.intervalMs(), _tapTempoDisplayLabel, sizeof(_tapTempoDisplayLabel));
    }

    _isTapTempoLit = true;
    _tapTempoFlashUntilMs = pressedAtMs + TapTempoFlashDurationMs;
    if (_tapTempoEngine.hasFlashInterval())
    {
        _nextTapTempoFlashToggleMs = pressedAtMs + _tapTempoEngine.flashHalfPeriodMs();
    }

    renderTapTempoButtons(true);
}

void PlaySetMode::clearTapTempoState()
{
    _tapTempoEngine.clear();
    _nextTapTempoFlashToggleMs = 0;
    _tapTempoFlashUntilMs = 0;
    _isTapTempoLit = true;
    _tapTempoDisplayLabel[0] = '\0';
}

bool PlaySetMode::advanceSelectedSetSong(bool allowWrap)
{
    size_t songCount = 0;
    size_t selectedSongIndex = 0;
    const bool hasActiveSet = _setListStore.activeSetPosition(_selectedPlaylist, songCount, selectedSongIndex);
    if (!hasActiveSet || songCount == 0 || selectedSongIndex >= songCount)
    {
        Serial.printf("[PlaySetDiag] advance blocked playlist=%u allowWrap=%u hasSet=%u songCount=%u selectedIndex=%u\n",
                      static_cast<unsigned int>(_selectedPlaylist), allowWrap ? 1U : 0U,
                      hasActiveSet ? 1U : 0U, static_cast<unsigned int>(songCount), static_cast<unsigned int>(selectedSongIndex));
        return false;
    }

    Serial.printf("[PlaySetDiag] advance begin playlist=%u allowWrap=%u currentIndex=%u songCount=%u\n",
                  static_cast<unsigned int>(_selectedPlaylist), allowWrap ? 1U : 0U,
                  static_cast<unsigned int>(selectedSongIndex), static_cast<unsigned int>(songCount));
    size_t nextSongIndex = selectedSongIndex + 1;
    if (nextSongIndex >= songCount)
    {
        if (!allowWrap)
        {
            return false;
        }

        nextSongIndex = 0;
    }

    if (!_setListStore.selectSong(_selectedPlaylist, nextSongIndex))
    {
        Serial.printf("[PlaySetDiag] advance selectSong failed playlist=%u nextIndex=%u\n",
                      static_cast<unsigned int>(_selectedPlaylist), static_cast<unsigned int>(nextSongIndex));
        return false;
    }

    Serial.printf("[PlaySetDiag] advance selectSong ok playlist=%u nextIndex=%u\n",
                  static_cast<unsigned int>(_selectedPlaylist), static_cast<unsigned int>(nextSongIndex));
    _hasSelectedSetSong = true;
    _hasSelectedSong = false;
    resolveSelectedSetSong();
    if (_hasSelectedPreset)
    {
        _midiProvider.recallPreset(_selectedPreset);
    }
    return true;
}

void PlaySetMode::handleNextSetSongPress()
{
    const uint32_t now = millis();
    Serial.printf("[PlaySetDiag] next pressed now=%lu last=%lu wrapUntil=%lu\n", static_cast<unsigned long>(now),
                  static_cast<unsigned long>(_lastSetAdvancePressedAtMs), static_cast<unsigned long>(_setWrapConfirmUntilMs));
    if (_lastSetAdvancePressedAtMs != 0U &&
        static_cast<int32_t>(now - _lastSetAdvancePressedAtMs) < static_cast<int32_t>(SetAdvanceDebounceMs))
    {
        Serial.printf("[PlaySetDiag] next debounced delta=%ld\n",
                      static_cast<long>(now - _lastSetAdvancePressedAtMs));
        return;
    }
    _lastSetAdvancePressedAtMs = now;

    if (advanceSelectedSetSong(false))
    {
        _showEndOfSetPrompt = false;
        _setWrapConfirmUntilMs = 0;
        renderPlayCenterUi();
        return;
    }

    size_t songCount = 0;
    size_t selectedSongIndex = 0;
    if (!_setListStore.activeSetPosition(_selectedPlaylist, songCount, selectedSongIndex) || songCount == 0)
    {
        Serial.printf("[PlaySetDiag] next no active set playlist=%u\n", static_cast<unsigned int>(_selectedPlaylist));
        return;
    }

    const bool atEndOfSet = (selectedSongIndex + 1) >= songCount;
    if (!atEndOfSet)
    {
        Serial.printf("[PlaySetDiag] next no-advance and not-end playlist=%u selectedIndex=%u songCount=%u\n",
                      static_cast<unsigned int>(_selectedPlaylist), static_cast<unsigned int>(selectedSongIndex),
                      static_cast<unsigned int>(songCount));
        return;
    }

    if (_setWrapConfirmUntilMs != 0U &&
        static_cast<int32_t>(now - _setWrapConfirmUntilMs) <= 0)
    {
        if (advanceSelectedSetSong(true))
        {
            Serial.printf("[PlaySetDiag] next wrap confirmed playlist=%u\n", static_cast<unsigned int>(_selectedPlaylist));
            _showEndOfSetPrompt = false;
            _setWrapConfirmUntilMs = 0;
            renderPlayCenterUi();
        }
        return;
    }

    Serial.printf("[PlaySetDiag] next at end - waiting confirm playlist=%u windowUntil=%lu\n",
                  static_cast<unsigned int>(_selectedPlaylist), static_cast<unsigned long>(now + SetWrapConfirmWindowMs));
    _setWrapConfirmUntilMs = now + SetWrapConfirmWindowMs;
    _showEndOfSetPrompt = true;
    renderPlayCenterUi();
}

void PlaySetMode::renderPatchNameLabel(uint16_t textColour)
{
    char patchNameLabel[TrackedTextLabel::Capacity] = {'\0'};
    formatPatchNameDisplayLabel(_patchDisplayConfig.name, patchNameLabel, sizeof(patchNameLabel));
    updateTrackedTextLabel(_patchNameLabel, patchNameLabel, FF22, PlayPatchNameScale, songAndPatchLabelLeftX(),
                           patchNameLabelY(), textColour);
}

void PlaySetMode::renderTapTempoButtons(bool redrawLabel)
{
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        if (!isTapTempoButton(buttonIndex))
        {
            continue;
        }

        FootSwitchTouchButton* button = _touchButtonManager.getButton(buttonIndex);
        if (button == nullptr)
        {
            continue;
        }

        const uint8_t ringBrightness = ringBrightnessForButton(buttonIndex);
        const uint16_t pillColour = (ringBrightness > 0U) ? getFunction(buttonIndex).colour() : TFT_BLACK;

        button->setEnabled(true);
        button->setPillColour(pillColour);
        button->setBorderVisible(false);

        const uint32_t ringColour = ColorUtils::rgb565To888(getFunction(buttonIndex).colour());
        _ringManager.setRingColour(buttonIndex, (ringBrightness > 0U) ? ringColour : 0U);
        _ringManager.setRingBrightness(buttonIndex, ringBrightness);

        if (redrawLabel)
        {
            button->setLabel((_tapTempoDisplayLabel[0] != '\0') ? _tapTempoDisplayLabel : getFunction(buttonIndex).label());
            button->draw(_screenUi);
            continue;
        }

        _screenUi.drawTouchButtonPill(button->getLocation(), button->getSize(), pillColour,
                                      _screenUi.touchButtonPillBorderColour());
    }
}

void PlaySetMode::renderToggleButtons(ActionType actionType, bool redrawLabel)
{
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        if (!isToggleButton(buttonIndex) || toggleActionTypeForButton(buttonIndex) != actionType)
        {
            continue;
        }

        FootSwitchTouchButton* button = _touchButtonManager.getButton(buttonIndex);
        if (button == nullptr)
        {
            continue;
        }

        const uint8_t ringBrightness = ringBrightnessForButton(buttonIndex);
        const uint16_t buttonColour = getFunction(buttonIndex).colour();
        const uint16_t pillColour = (ringBrightness > 0U) ? buttonColour : TFT_BLACK;
        const uint32_t ringColour = ColorUtils::rgb565To888(buttonColour);

        button->setEnabled(true);
        button->setPillColour(pillColour);
        button->setBorderVisible(false);

        _ringManager.setRingColour(buttonIndex, (ringBrightness > 0U) ? ringColour : 0U);
        _ringManager.setRingBrightness(buttonIndex, ringBrightness);

        if (redrawLabel)
        {
            button->setLabel(getFunction(buttonIndex).label());
            button->draw(_screenUi);
            continue;
        }

        _screenUi.drawTouchButtonPill(button->getLocation(), button->getSize(), pillColour,
                                      _screenUi.touchButtonPillBorderColour());
    }
}

void PlaySetMode::formatTapTempoBpmLabel(uint32_t intervalMs, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0U)
    {
        return;
    }

    if (intervalMs == 0U)
    {
        buffer[0] = '\0';
        return;
    }

    const uint32_t bpm = (60000U + (intervalMs / 2U)) / intervalMs;
    std::snprintf(buffer, bufferSize, "%lu", static_cast<unsigned long>(bpm));
}
