#include "Modes/PlayMode.h"
#include "ColorUtils.h"

#include <cctype>
#include <cstdio>

namespace
{
const char* PlayPatchBadgeTitle = "Patch";
const uint8_t PlayPatchBadgeTitleScale = 1;
const uint8_t PlayPatchBadgeNumberScale = 1;
const uint8_t PlaySnapshotLabelScale = 1;

const int32_t PlayPatchBadgeFrameWidth = 118;
const int32_t PlayPatchBadgeFrameHeight = 72;
const int32_t PlayPatchBadgeFrameRadius = 12;
const int32_t PlayPatchBadgeRightMargin = 20;
const int32_t PlaySnapshotLabelOffsetY = 72;
const int32_t PlayPatchBadgeTitleBorderOffset = 5;
const int32_t PlayPatchBadgeNumberOffset = 22;

const int32_t PlaySnapshotLabelLeftX = 40;
const byte FirstSnapshotButtonIndex = 0;
const byte LastSnapshotButtonIndex = 2;
const byte PatchButtonIndex = 4;
const byte GigViewButtonIndex = 5;
const uint16_t GigViewButtonColour = 0xF81F;
const byte TapTempoButtonIndex = 6;
const uint16_t TapTempoButtonColour = 0x001F;
const byte TapTempoCc = 44;
const byte TapTempoValue = 100;
const uint32_t TapTempoFlashDurationMs = 10000;
const byte TunerButtonIndex = 7;
const uint16_t TunerButtonColour = 0x801F;

bool isHomePlaylistTransition(ModeTransitionValue transitionValue)
{
    return (transitionValue & ModeTransitionHomePlaylistFlag) != 0;
}

const char* playPlaylistName(byte playlistIndex)
{
    switch (playlistIndex)
    {
    case 2:
        return "Project7";
    case 3:
        return "OPR";
    case 4:
        return "CodeRed";
    default:
        return "Unknown";
    }
}

const char* actionTypeName(ActionType action)
{
    switch (action)
    {
    case ActionType::None:
        return "None";
    case ActionType::SendMidiProgramChange:
        return "SendMidiProgramChange";
    case ActionType::SendMidiControlChange:
        return "SendMidiControlChange";
    case ActionType::ChangeMode:
        return "ChangeMode";
    case ActionType::SelectScene:
        return "SelectScene";
    case ActionType::SetTuner:
        return "SetTuner";
    case ActionType::SelectHomePlaylist:
        return "SelectHomePlaylist";
    case ActionType::TapTempo:
        return "TapTempo";
    case ActionType::SetGigView:
        return "SetGigView";
    default:
        return "Unknown";
    }
}

void logFunctionAction(const char* prefix, const FunctionAction& action)
{
    Serial.printf("%s%s(value=%u, secondary=%u)", prefix, actionTypeName(action.type),
                  static_cast<unsigned int>(action.value), static_cast<unsigned int>(action.secondaryValue));
}
} // namespace

PlayMode::PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
                   IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _buttonOverrideStore(buttonOverrideStore), _selectedPreset(0),
      _selectedPlaylist(midiProvider.defaultPlaylistIndex()), _hasSelectedPreset(false), _selectedButton(0),
      _tapTempoPressTimes{0, 0, 0}, _tapTempoPressCount(0),
      _tapTempoFlashHalfPeriodMs(0), _nextTapTempoFlashToggleMs(0), _tapTempoFlashUntilMs(0),
      _hasTapTempoFlashInterval(false), _isTapTempoLit(true)
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
    _functions[4] = Function("Patch", 0xFFE0, ActionType::ChangeMode, static_cast<byte>(Modes::Patch),
                             ActionType::ChangeMode, static_cast<byte>(Modes::Home));
    _functions[5] = Function("Gig", GigViewButtonColour, ActionType::SetGigView, 1, ActionType::SetGigView, 0);
    _functions[6] = Function("Tap", TapTempoButtonColour, ActionType::TapTempo, 0, ActionType::None, 0);
    _functions[7] = Function("Tuner", TunerButtonColour, ActionType::SetTuner, 1, ActionType::SetTuner, 0);
}

void PlayMode::activate()
{
    _midiProvider.selectPlaylist(_selectedPlaylist);
    setupFunctions();
    const bool didRefreshOverrides = _buttonOverrideStore.refresh();
    _buttonOverrideStore.applyOverrides(_selectedPlaylist, _selectedPreset, _functions, TouchButtonManager::BUTTON_COUNT);
    Serial.printf("Play mode: activate playlist=%s(%u) patch=%u refresh=%s\n", playPlaylistName(_selectedPlaylist),
                  static_cast<unsigned int>(_selectedPlaylist), static_cast<unsigned int>(_selectedPreset),
                  didRefreshOverrides ? "ok" : "failed");
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        const Function& func = getFunction(buttonIndex);
        const bool enabled = isButtonEnabled(buttonIndex);
        Serial.printf("Play mode: button %u label='%s' colour=0x%04X enabled=%s momentary=%s ",
                      static_cast<unsigned int>(buttonIndex + 1U), func.label(),
                      static_cast<unsigned int>(func.colour()), enabled ? "yes" : "no",
                      func.hasMomentaryBehaviour() ? "yes" : "no");
        logFunctionAction("press=", func.action(FunctionBehaviour::ButtonDown));
        Serial.print(" ");
        logFunctionAction("release=", func.action(FunctionBehaviour::ButtonRelease));
        Serial.print(" ");
        logFunctionAction("short=", func.action(FunctionBehaviour::ShortPress));
        Serial.print(" ");
        logFunctionAction("long=", func.action(FunctionBehaviour::LongPress));
        Serial.println();
    }
    _isTapTempoLit = true;
    const uint32_t now = millis();
    if (_hasTapTempoFlashInterval && _tapTempoFlashUntilMs != 0 &&
        static_cast<int32_t>(now - _tapTempoFlashUntilMs) < 0)
    {
        _nextTapTempoFlashToggleMs = now + _tapTempoFlashHalfPeriodMs;
    }
    else
    {
        _tapTempoFlashUntilMs = 0;
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

void PlayMode::deactivate() { clearPlayCenterUi(); }

void PlayMode::renderPlayCenterUi()
{
    renderPatchBadge();
    renderSnapshotLabel(_selectedButton, TFT_WHITE);
}

void PlayMode::clearPlayCenterUi()
{
    clearPatchBadge();
    renderSnapshotLabel(_selectedButton, TFT_BLACK);
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
        return;
    }

    char snapshotLabel[16] = {'\0'};
    formatSnapshotLabelUppercase(snapshotButton, snapshotLabel, sizeof(snapshotLabel));

    const int32_t labelY = _screenUi.boxHeight() + PlaySnapshotLabelOffsetY;
    _screenUi.drawText(FF32, PlaySnapshotLabelScale, snapshotLabel, PlaySnapshotLabelLeftX, labelY, textColour,
                       TFT_BLACK);
}

void PlayMode::formatPatchNumberLabel(byte patchNumber, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    std::snprintf(buffer, bufferSize, "%02u", static_cast<unsigned int>(patchNumber));
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
        if (previousSelected != currentSelected)
        {
            renderSnapshotLabel(previousSelected, TFT_BLACK);
        }
    }

    if (currentSelected != previousSelected && currentSelected >= FirstSnapshotButtonIndex &&
        currentSelected <= LastSnapshotButtonIndex)
    {
        renderButton(currentSelected);
        renderSnapshotLabel(currentSelected, TFT_WHITE);
    }
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

uint8_t PlayMode::ringBrightnessForButton(byte number) const
{
    if (number == PatchButtonIndex || number == GigViewButtonIndex || number == TunerButtonIndex)
    {
        return RingManager::FullBrightness;
    }

    if (number == TapTempoButtonIndex)
    {
        return _isTapTempoLit ? RingManager::FullBrightness : 0;
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
    if (_tapTempoFlashUntilMs != 0 && static_cast<int32_t>(now - _tapTempoFlashUntilMs) >= 0)
    {
        _tapTempoFlashUntilMs = 0;
        if (!_isTapTempoLit)
        {
            _isTapTempoLit = true;
            renderTapTempoPill();
        }
        return;
    }

    if (!_hasTapTempoFlashInterval || _tapTempoFlashUntilMs == 0)
    {
        return;
    }

    if (static_cast<int32_t>(now - _nextTapTempoFlashToggleMs) < 0)
    {
        return;
    }

    _isTapTempoLit = !_isTapTempoLit;
    _nextTapTempoFlashToggleMs = now + _tapTempoFlashHalfPeriodMs;
    renderTapTempoPill();
}

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
        _midiManager.sendControlChange(TapTempoCc, TapTempoValue);
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
        break;
    }
    }
}

void PlayMode::registerTapTempoPress(uint32_t pressedAtMs)
{
    if (_tapTempoPressCount < 3U)
    {
        _tapTempoPressTimes[_tapTempoPressCount++] = pressedAtMs;
    }
    else
    {
        _tapTempoPressTimes[0] = _tapTempoPressTimes[1];
        _tapTempoPressTimes[1] = _tapTempoPressTimes[2];
        _tapTempoPressTimes[2] = pressedAtMs;
    }

    if (_tapTempoPressCount >= 2U)
    {
        uint32_t intervalTotalMs = 0;
        for (uint8_t i = 1; i < _tapTempoPressCount; ++i)
        {
            intervalTotalMs += (_tapTempoPressTimes[i] - _tapTempoPressTimes[i - 1U]);
        }

        const uint32_t averageIntervalMs = intervalTotalMs / static_cast<uint32_t>(_tapTempoPressCount - 1U);
        _tapTempoFlashHalfPeriodMs = (averageIntervalMs > 1U) ? (averageIntervalMs / 2U) : 1U;
        _hasTapTempoFlashInterval = true;
    }

    _isTapTempoLit = true;
    _tapTempoFlashUntilMs = pressedAtMs + TapTempoFlashDurationMs;
    if (_hasTapTempoFlashInterval)
    {
        _nextTapTempoFlashToggleMs = pressedAtMs + _tapTempoFlashHalfPeriodMs;
    }

    renderTapTempoPill();
}

void PlayMode::renderTapTempoPill()
{
    FootSwitchTouchButton* button = _touchButtonManager.getButton(TapTempoButtonIndex);
    if (button == nullptr)
    {
        return;
    }

    const bool enabled = isButtonEnabled(TapTempoButtonIndex);
    const uint8_t ringBrightness = enabled ? ringBrightnessForButton(TapTempoButtonIndex) : 0;
    const uint16_t pillColour = (enabled && ringBrightness > 0) ? getFunction(TapTempoButtonIndex).colour() : 0;

    button->setEnabled(enabled);
    button->setPillColour(pillColour);
    button->setBorderVisible(false);

    if (!enabled)
    {
        _ringManager.setRingColour(TapTempoButtonIndex, 0);
        _ringManager.setRingBrightness(TapTempoButtonIndex, 0);
        _screenUi.drawTouchButtonPill(button->getLocation(), button->getSize(), 0, 0);
        return;
    }

    const uint32_t ringColour = ColorUtils::rgb565To888(getFunction(TapTempoButtonIndex).colour());
    _ringManager.setRingColour(TapTempoButtonIndex, (ringBrightness > 0) ? ringColour : 0);
    _ringManager.setRingBrightness(TapTempoButtonIndex, ringBrightness);
    _screenUi.drawTouchButtonPill(button->getLocation(), button->getSize(), pillColour,
                                  _screenUi.touchButtonPillBorderColour());
}
