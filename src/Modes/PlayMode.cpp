#include "Modes/PlayMode.h"
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
const uint8_t PlaySnapshotLabelScale = 1;

const int32_t PlayPatchBadgeFrameWidth = 118;
const int32_t PlayPatchBadgeFrameHeight = 72;
const int32_t PlayPatchBadgeFrameRadius = 12;
const int32_t PlayPatchBadgeRightMargin = 20;
const int32_t PlaySnapshotLabelOffsetY = 72;
const int32_t PlayPatchBadgeTitleBorderOffset = 5;
const int32_t PlayPatchBadgeNumberOffset = 22;
const int32_t PlayPatchNameOffsetY = 35;
const size_t PlayPatchNameMaxChars = 18;
const char* PlayPatchNamePrefix = "Patch: ";

const int32_t PlaySnapshotLabelLeftX = 40;
const byte FirstSnapshotButtonIndex = 0;
const byte LastSnapshotButtonIndex = 2;
const byte PatchButtonIndex = 4;
const byte GigViewButtonIndex = 6;
const uint16_t GigViewButtonColour = 0xF81F;
const byte TapTempoCc = 44;
const byte TapTempoValue = 100;
const uint32_t TapTempoFlashDurationMs = 10000;
const uint32_t TunerFlashHalfPeriodMs = 500;
const byte TunerButtonIndex = 7;
const uint16_t TunerButtonColour = 0x801F;

bool isHomePlaylistTransition(ModeTransitionValue transitionValue)
{
    return (transitionValue & ModeTransitionHomePlaylistFlag) != 0;
}
} // namespace

PlayMode::PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                   IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
                   IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _buttonOverrideStore(buttonOverrideStore), _selectedPreset(0),
      _selectedPlaylist(midiProvider.defaultPlaylistIndex()), _hasSelectedPreset(false), _selectedButton(0),
      _tapTempoEngine(), _nextTapTempoFlashToggleMs(0), _tapTempoFlashUntilMs(0), _isTapTempoLit(true),
      _nextTunerFlashToggleMs(0), _isTunerEnabled(false), _isGigViewEnabled(false), _isTunerFlashLit(true),
      _toggleStates{false, false, false, false, false, false, false, false},
      _patchDisplayConfig(), _patchNameLabel{{0}}, _snapshotLabel{{0}},
      _tapTempoDisplayLabel{0}
{
    setupFunctions();
}

void PlayMode::setSelectedPreset(byte selectedPreset)
{
    _selectedPreset = selectedPreset;
    _hasSelectedPreset = true;
}

void PlayMode::setTransitionValue(ModeTransitionValue transitionValue)
{
    if (transitionValue == ModeTransitionNone)
    {
        _selectedPlaylist = _midiProvider.defaultPlaylistIndex();
        _hasSelectedPreset = false;
        return;
    }

    if (isPlayModeTransition(transitionValue))
    {
        _selectedPlaylist = playModeTransitionPlaylist(transitionValue);
        _selectedPreset = playModeTransitionPatch(transitionValue);
        _hasSelectedPreset = playModeTransitionShouldRecall(transitionValue);
        return;
    }

    if ((transitionValue & ModeTransitionPatchReturnFlag) != 0)
    {
        _selectedPreset = static_cast<byte>(transitionValue & ModeTransitionPatchValueMask);
        _hasSelectedPreset = false;
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

void PlayMode::setupFunctions()
{
    _functions[0] = Function("Clean", 0x07E0, ActionType::SelectScene, 0, ActionType::None, 0);
    _functions[1] = Function("Crunch", 0xFD20, ActionType::SelectScene, 1, ActionType::None, 0);
    _functions[2] = Function("Lead", 0xF800, ActionType::SelectScene, 2, ActionType::None, 0);

    _functions[3] = Function();
    configurePatchButton();
    _functions[5] = Function();
    _functions[GigViewButtonIndex] = Function("Gig", GigViewButtonColour, ActionType::SetGigView, 1,
                                              ActionType::SetGigView, 0);
    _functions[GigViewButtonIndex].setToggle(true);
    _functions[TunerButtonIndex] = Function("Tuner", TunerButtonColour, ActionType::SetTuner, 1, ActionType::SetTuner,
                                            0);
    _functions[TunerButtonIndex].setToggle(true);
}

void PlayMode::configurePatchButton()
{
    _functions[4] = Function("Patch", 0xFFE0, ActionType::ChangeMode, static_cast<byte>(Modes::Patches),
                             ActionType::ChangeMode, static_cast<byte>(Modes::Patch));
}

void PlayMode::activate()
{
    _midiProvider.selectPlaylist(_selectedPlaylist);
    setupFunctions();
    _patchDisplayConfig.name[0] = '\0';
    _buttonOverrideStore.refresh();
    _buttonOverrideStore.applyOverrides(_selectedPlaylist, _selectedPreset, _functions, TouchButtonManager::BUTTON_COUNT,
                                        &_patchDisplayConfig);
    configurePatchButton();
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

void PlayMode::deactivate()
{
    clearTapTempoState();
    clearPlayCenterUi();
}

void PlayMode::renderPlayCenterUi()
{
    renderPatchBadge();
    renderPatchNameLabel(TFT_WHITE);
    renderSnapshotLabel(_selectedButton, TFT_WHITE);
}

void PlayMode::clearPlayCenterUi()
{
    clearPatchBadge();
    clearTrackedTextLabel(_patchNameLabel, FF22, PlayPatchNameScale, PlaySnapshotLabelLeftX, patchNameLabelY());
    clearTrackedTextLabel(_snapshotLabel, FF32, PlaySnapshotLabelScale, PlaySnapshotLabelLeftX, snapshotLabelY());
}

void PlayMode::renderPatchBadge()
{
    const PatchBadgeMetrics metrics = patchBadgeMetrics();

    _screenUi.drawCenteredFrame(metrics.frameCenterX, metrics.frameTopY, PlayPatchBadgeFrameWidth,
                                PlayPatchBadgeFrameHeight,
                                PlayPatchBadgeFrameRadius);
    _screenUi.drawCenteredText(FF22, PlayPatchBadgeTitleScale, PlayPatchBadgeTitle, metrics.frameCenterX,
                               metrics.titleY, TFT_WHITE, TFT_BLACK);
    renderPatchBadgeNumber(_selectedPreset, TFT_WHITE);
}

void PlayMode::clearPatchBadge()
{
    const PatchBadgeMetrics metrics = patchBadgeMetrics();

    _screenUi.drawCenteredFrame(metrics.frameCenterX, metrics.frameTopY, PlayPatchBadgeFrameWidth,
                                PlayPatchBadgeFrameHeight,
                                PlayPatchBadgeFrameRadius, TFT_BLACK, TFT_BLACK);
    _screenUi.drawCenteredText(FF22, PlayPatchBadgeTitleScale, PlayPatchBadgeTitle, metrics.frameCenterX,
                               metrics.titleY, TFT_BLACK, TFT_BLACK);
    renderPatchBadgeNumber(_selectedPreset, TFT_BLACK);
}

void PlayMode::renderPatchBadgeNumber(byte patchNumber, uint16_t textColour)
{
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t numberY = patchBadgeFrameTopY() + PlayPatchBadgeNumberOffset;

    char patchLabel[4] = {'0', '0', '\0', '\0'};
    formatPatchNumberLabel(patchNumber, patchLabel, sizeof(patchLabel));
    _screenUi.drawCenteredText(FF32, PlayPatchBadgeNumberScale, patchLabel, frameCenterX, numberY, textColour,
                               TFT_BLACK);
}

void PlayMode::renderSnapshotLabel(byte snapshotButton, uint16_t textColour)
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

void PlayMode::formatPatchNumberLabel(byte patchNumber, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    std::snprintf(buffer, bufferSize, "%02u", static_cast<unsigned int>(patchNumber));
}

void PlayMode::formatPatchNameDisplayLabel(const char* patchName, char* buffer, size_t bufferSize)
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

void PlayMode::formatSnapshotLabelUppercase(byte snapshotButton, char* buffer, size_t bufferSize) const
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

void PlayMode::updateTrackedTextLabel(TrackedTextLabel& trackedLabel, const char* newText, const GFXfont* font,
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

void PlayMode::clearTrackedTextLabel(TrackedTextLabel& trackedLabel, const GFXfont* font, uint8_t scale, int32_t x,
                                     int32_t y)
{
    if (trackedLabel.text[0] == '\0')
    {
        return;
    }

    _screenUi.drawText(font, scale, trackedLabel.text, x, y, TFT_BLACK, TFT_BLACK);
    trackedLabel.text[0] = '\0';
}

int32_t PlayMode::patchBadgeFrameCenterX() const
{
    const int32_t screenWidth = _screenUi.boxWidth() * 4;
    return screenWidth - PlayPatchBadgeRightMargin - (PlayPatchBadgeFrameWidth / 2);
}

PlayMode::PatchBadgeMetrics PlayMode::patchBadgeMetrics() const
{
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t frameTopY = patchBadgeFrameTopY();

    PatchBadgeMetrics metrics = {};
    metrics.frameCenterX = frameCenterX;
    metrics.frameTopY = frameTopY;
    metrics.titleY = frameTopY - PlayPatchBadgeTitleBorderOffset - 2;
    return metrics;
}

int32_t PlayMode::patchBadgeFrameTopY() const
{
    const int32_t centerTopY = _screenUi.boxHeight();
    const int32_t centerHeight = _screenUi.bottomRowY() - centerTopY;

    // Current alignment gap from center top (before swap).
    const int32_t topGap = PlaySnapshotLabelOffsetY - (PlayPatchBadgeFrameHeight / 2);
    // Swap top and bottom gaps: newTop = centerHeight - oldTop - frameHeight.
    const int32_t swappedTopGap = centerHeight - topGap - PlayPatchBadgeFrameHeight;

    return centerTopY + swappedTopGap;
}

int32_t PlayMode::patchNameLabelY() const { return _screenUi.boxHeight() + PlayPatchNameOffsetY; }

int32_t PlayMode::snapshotLabelY() const { return _screenUi.boxHeight() + PlaySnapshotLabelOffsetY; }

void PlayMode::updateVisuals() { renderAllButtons(); }

void PlayMode::renderButton(byte number)
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

void PlayMode::updateSnapshotSelectionVisuals(byte previousSelected, byte currentSelected)
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

bool PlayMode::usesSelectionBorder(byte number) const
{
    return isSceneSelectionButton(number);
}

int8_t PlayMode::firstSceneSelectionButton() const
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

bool PlayMode::isSceneSelectionButton(byte number) const
{
    if (number < FirstSnapshotButtonIndex || number > LastSnapshotButtonIndex || !isButtonEnabled(number))
    {
        return false;
    }

    const Function& func = getFunction(number);
    return !func.hasMomentaryBehaviour() &&
           func.action(FunctionBehaviour::ShortPress).type == ActionType::SelectScene;
}

bool PlayMode::isTapTempoButton(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return false;
    }

    return getFunction(number).action(FunctionBehaviour::ShortPress).type == ActionType::TapTempo;
}

ActionType PlayMode::toggleActionTypeForButton(byte number) const
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

bool PlayMode::isToggleButton(byte number) const
{
    return number < TouchButtonManager::BUTTON_COUNT && isButtonEnabled(number) && getFunction(number).isToggle() &&
           !getFunction(number).hasMomentaryBehaviour();
}

bool PlayMode::isToggleActionEnabled(ActionType actionType) const
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

uint8_t PlayMode::ringBrightnessForButton(byte number) const
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

void PlayMode::buttonDown(byte number)
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

void PlayMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        return;
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

void PlayMode::buttonLongPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        return;
    }

    const Function& func = getFunction(number);
    if (func.hasMomentaryBehaviour())
    {
        return;
    }

    if (isToggleButton(number))
    {
        return;
    }

    executeAction(func.action(FunctionBehaviour::LongPress));
}

void PlayMode::buttonReleased(byte number)
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

void PlayMode::frameTick()
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

void PlayMode::encoderPressed() { _transitionDelegate.enterMode(Modes::Home, ModeTransitionNone); }

void PlayMode::executeAction(const FunctionAction& action)
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
            _transitionDelegate.enterMode(targetMode, makePlayModeTransition(_selectedPlaylist, _selectedPreset, false));
        }
        break;
    }
    }
}

void PlayMode::toggleAction(ActionType actionType)
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

void PlayMode::registerTapTempoPress(uint32_t pressedAtMs)
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

void PlayMode::clearTapTempoState()
{
    _tapTempoEngine.clear();
    _nextTapTempoFlashToggleMs = 0;
    _tapTempoFlashUntilMs = 0;
    _isTapTempoLit = true;
    _tapTempoDisplayLabel[0] = '\0';
}

void PlayMode::renderPatchNameLabel(uint16_t textColour)
{
    char patchNameLabel[TrackedTextLabel::Capacity] = {'\0'};
    formatPatchNameDisplayLabel(_patchDisplayConfig.name, patchNameLabel, sizeof(patchNameLabel));
    updateTrackedTextLabel(_patchNameLabel, patchNameLabel, FF22, PlayPatchNameScale, PlaySnapshotLabelLeftX,
                           patchNameLabelY(), textColour);
}

void PlayMode::renderTapTempoButtons(bool redrawLabel)
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

void PlayMode::renderToggleButtons(ActionType actionType, bool redrawLabel)
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

void PlayMode::formatTapTempoBpmLabel(uint32_t intervalMs, char* buffer, size_t bufferSize)
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
