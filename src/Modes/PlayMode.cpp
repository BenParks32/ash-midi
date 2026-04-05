#include "Modes/PlayMode.h"
#include "ColorUtils.h"
#include "HxStompMidi.h"

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
} // namespace

PlayMode::PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _selectedHomeProgramChange(0), _hasSelectedHomeProgramChange(false), _selectedButton(0)
{
    setupFunctions();
}

void PlayMode::setSelectedHomeProgramChange(byte selectedHomeProgramChange)
{
    _selectedHomeProgramChange = selectedHomeProgramChange;
    _hasSelectedHomeProgramChange = true;
}

void PlayMode::setTransitionValue(byte transitionValue)
{
    if (transitionValue == ModeTransitionNone)
    {
        _hasSelectedHomeProgramChange = false;
        return;
    }

    if ((transitionValue & ModeTransitionPatchReturnFlag) != 0)
    {
        _selectedHomeProgramChange = static_cast<byte>(transitionValue & ModeTransitionPatchValueMask);
        _hasSelectedHomeProgramChange = false;
        return;
    }

    setSelectedHomeProgramChange(transitionValue);
}

void PlayMode::setupFunctions()
{
    _functions[0] = Function("Clean", 0x07E0, ActionType::SendMidiControlChange, 0, ActionType::None, 0);
    _functions[1] = Function("Crunch", 0xFD20, ActionType::SendMidiControlChange, 1, ActionType::None, 0);
    _functions[2] = Function("Lead", 0xF800, ActionType::SendMidiControlChange, 2, ActionType::None, 0);

    _functions[3] = Function();
    _functions[4] = Function("Patch", 0xFFE0, ActionType::ChangeMode, static_cast<byte>(Modes::Patch),
                             ActionType::ChangeMode, static_cast<byte>(Modes::Home));
    _functions[5] = Function();
    _functions[6] = Function();
    _functions[7] = Function();
}

void PlayMode::activate()
{
    if (_hasSelectedHomeProgramChange)
    {
        _midiManager.sendProgramChange(_selectedHomeProgramChange);
    }

    if (!isButtonEnabled(_selectedButton))
    {
        _selectedButton = 0;
    }

    _midiManager.sendControlChange(STOMP_SNAPSHOT_CC, _selectedButton);

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
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t frameTopY = patchBadgeFrameTopY();
    const int32_t titleY = frameTopY - PlayPatchBadgeTitleBorderOffset;

    _screenUi.drawCenteredFrame(frameCenterX, frameTopY, PlayPatchBadgeFrameWidth, PlayPatchBadgeFrameHeight,
                                PlayPatchBadgeFrameRadius);
    _screenUi.drawCenteredText(FF22, PlayPatchBadgeTitleScale, PlayPatchBadgeTitle, frameCenterX, titleY, TFT_WHITE,
                               TFT_BLACK);
    renderPatchBadgeNumber(_selectedHomeProgramChange, TFT_WHITE);
}

void PlayMode::clearPatchBadge()
{
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t frameTopY = patchBadgeFrameTopY();
    const int32_t titleY = frameTopY - PlayPatchBadgeTitleBorderOffset;

    _screenUi.drawCenteredFrame(frameCenterX, frameTopY, PlayPatchBadgeFrameWidth, PlayPatchBadgeFrameHeight,
                                PlayPatchBadgeFrameRadius, TFT_BLACK, TFT_BLACK);
    _screenUi.drawCenteredText(FF22, PlayPatchBadgeTitleScale, PlayPatchBadgeTitle, frameCenterX, titleY, TFT_BLACK,
                               TFT_BLACK);
    renderPatchBadgeNumber(_selectedHomeProgramChange, TFT_BLACK);
}

void PlayMode::renderPatchBadgeNumber(byte patchNumber, uint16_t textColour)
{
    const int32_t frameCenterX = patchBadgeFrameCenterX();
    const int32_t numberY = patchBadgeFrameTopY() + PlayPatchBadgeNumberOffset;

    char patchLabel[3] = {'0', '0', '\0'};
    formatPatchNumberLabel(patchNumber, patchLabel, sizeof(patchLabel));
    _screenUi.drawCenteredText(FF32, PlayPatchBadgeNumberScale, patchLabel, frameCenterX, numberY, textColour,
                               TFT_BLACK);
}

void PlayMode::renderSnapshotLabel(byte snapshotButton, uint16_t textColour)
{
    if (snapshotButton < FirstSnapshotButtonIndex || snapshotButton > LastSnapshotButtonIndex)
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
    return number >= FirstSnapshotButtonIndex && number <= LastSnapshotButtonIndex;
}

uint8_t PlayMode::ringBrightnessForButton(byte number) const
{
    // Snapshot buttons (0-2) are mutually exclusive: selected is bright, others are dim.
    // Patch navigation button is always bright when enabled.
    static constexpr byte kPatchButton = 4;

    if (number == kPatchButton)
    {
        return RingManager::FullBrightness;
    }

    const bool isPlayFunctionButton = (number >= FirstSnapshotButtonIndex && number <= LastSnapshotButtonIndex);
    return (isPlayFunctionButton && number == _selectedButton) ? RingManager::FullBrightness
                                                               : RingManager::DimBrightness;
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
    if (func.pressAction() == ActionType::ChangeMode)
    {
        executeAction(func.pressAction(), func.pressActionValue());
        return;
    }

    executeAction(func.pressAction(), func.pressActionValue());

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
    executeAction(func.longPressAction(), func.longPressActionValue());
}

void PlayMode::frameTick()
{
    // Play mode currently has no per-frame work.
}

void PlayMode::executeAction(ActionType action, byte actionValue)
{
    switch (action)
    {
    case ActionType::None:
        break;
    case ActionType::SendMidiProgramChange:
        _midiManager.sendProgramChange(actionValue);
        break;
    case ActionType::SendMidiControlChange:
        _midiManager.sendControlChange(STOMP_SNAPSHOT_CC, actionValue);
        break;
    case ActionType::ChangeMode:
    {
        const Modes targetMode = static_cast<Modes>(actionValue);
        if (targetMode == Modes::Home)
        {
            _transitionDelegate.enterMode(targetMode, ModeTransitionNone);
        }
        else if (targetMode == Modes::Patch)
        {
            _transitionDelegate.enterMode(targetMode, _selectedHomeProgramChange);
        }
        break;
    }
    }
}
