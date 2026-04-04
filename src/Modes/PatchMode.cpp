#include "Modes/PatchMode.h"

#include "ColorUtils.h"

#include <cstdio>

namespace
{
const byte PatchMin = 0;
const byte PatchMax = 39;
const byte PatchDownAction = 0;
const byte PatchUpAction = 1;
const byte PlayButtonIndex = 6;
const uint32_t PlayButtonFlashHalfPeriodMs = 500;
const char* PatchSelectorTitle = "Select Patch";
const uint8_t PatchSelectorTitleScale = 1;
const uint8_t PatchNumberScale = 2;
const int32_t PatchNumberFrameWidth = 196;
const int32_t PatchNumberFrameHeight = 110;
const int32_t PatchNumberFrameRadius = 16;
const int32_t PatchNumberTextOffsetFromFrameTop = 16;
const int32_t PatchTitleBorderOffset = 12;
} // namespace

PatchMode::PatchMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                     IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate), _currentPatch(0),
      _isPlayButtonLit(true), _nextPlayFlashToggleMs(0)
{
    setupFunctions();
}

void PatchMode::setupFunctions()
{
    _functions[0] = Function();
    _functions[1] = Function();
    _functions[2] = Function();
    _functions[3] = Function("Down", 0x07FF, ActionType::SendMidiProgramChange, PatchDownAction, ActionType::None, 0);
    _functions[4] =
        Function("Home", 0xFFFF, ActionType::ChangeMode, static_cast<byte>(Modes::Home), ActionType::None, 0);
    _functions[5] = Function();
    _functions[6] =
        Function("Play", 0x07E0, ActionType::ChangeMode, static_cast<byte>(Modes::Play), ActionType::None, 0);
    _functions[7] = Function("Up", 0xFD20, ActionType::SendMidiProgramChange, PatchUpAction, ActionType::None, 0);
}

void PatchMode::activate()
{
    _isPlayButtonLit = true;
    _nextPlayFlashToggleMs = millis() + PlayButtonFlashHalfPeriodMs;
    renderAllButtons();
    renderPatchSelector();
}

void PatchMode::buttonPressed(byte number)
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
    executeAction(func.pressAction(), func.pressActionValue());
}

void PatchMode::buttonLongPressed(byte number)
{
    (void)number;
    // Patch mode long press is currently unused.
}

void PatchMode::frameTick()
{
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - _nextPlayFlashToggleMs) < 0)
    {
        return;
    }

    _isPlayButtonLit = !_isPlayButtonLit;
    _nextPlayFlashToggleMs = now + PlayButtonFlashHalfPeriodMs;
    renderPlayButton();
}

void PatchMode::renderPlayButton()
{
    FootSwitchTouchButton* button = _touchButtonManager.getButton(PlayButtonIndex);
    if (button == nullptr)
    {
        return;
    }

    const Function& func = getFunction(PlayButtonIndex);
    const bool enabled = isButtonEnabled(PlayButtonIndex);
    const uint8_t ringBrightness = enabled ? ringBrightnessForButton(PlayButtonIndex) : 0;
    const uint16_t baseColour = func.colour();
    const uint16_t pillColour = (enabled && ringBrightness > 0) ? baseColour : 0;

    button->setEnabled(enabled);
    button->setLabel(func.label());

    if (!enabled)
    {
        button->setPillColour(0);
        button->setBorderVisible(false);
        _ringManager.setRingColour(PlayButtonIndex, 0);
        _ringManager.setRingBrightness(PlayButtonIndex, 0);
        button->draw(_screenUi);
        return;
    }

    button->setPillColour(pillColour);
    button->setBorderVisible(true);

    if (ringBrightness == 0)
    {
        _ringManager.setRingColour(PlayButtonIndex, 0);
        _ringManager.setRingBrightness(PlayButtonIndex, 0);
    }
    else
    {
        _ringManager.setRingColour(PlayButtonIndex, ColorUtils::rgb565To888(baseColour));
        _ringManager.setRingBrightness(PlayButtonIndex, ringBrightness);
    }

    _screenUi.drawTouchButtonPill(button->getLocation(), button->getSize(), pillColour);
}

void PatchMode::renderPatchSelector()
{
    _screenUi.clearCenterSection();

    const int32_t centerX = _screenUi.boxWidth() * 2;
    const int32_t frameTopY = patchFrameTopY();
    const int32_t titleY = frameTopY - PatchTitleBorderOffset;

    _screenUi.drawCenteredFrame(centerX, frameTopY, PatchNumberFrameWidth, PatchNumberFrameHeight,
                                PatchNumberFrameRadius);

    _screenUi.drawCenteredText(FF22, PatchSelectorTitleScale, PatchSelectorTitle, centerX, titleY, TFT_WHITE,
                               TFT_BLACK);
    renderPatchNumber(_currentPatch, TFT_WHITE);
}

void PatchMode::renderPatchNumber(byte patchNumber, uint16_t textColour)
{
    const int32_t centerX = _screenUi.boxWidth() * 2;

    char patchLabel[3] = {'0', '0', '\0'};
    formatPatchLabel(patchNumber, patchLabel, sizeof(patchLabel));
    _screenUi.drawCenteredText(FF32, PatchNumberScale, patchLabel, centerX, patchNumberY(), textColour, TFT_BLACK);
}

int32_t PatchMode::patchFrameTopY() const
{
    const int32_t centerTopY = _screenUi.boxHeight();
    const int32_t centerBottomY = _screenUi.bottomRowY();
    const int32_t centerHeight = centerBottomY - centerTopY;
    return centerTopY + ((centerHeight - PatchNumberFrameHeight) / 2);
}

int32_t PatchMode::patchNumberY() const { return patchFrameTopY() + PatchNumberTextOffsetFromFrameTop; }

void PatchMode::formatPatchLabel(byte patchNumber, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    std::snprintf(buffer, bufferSize, "%02u", static_cast<unsigned int>(patchNumber));
}

void PatchMode::setTransitionValue(byte transitionValue)
{
    if (transitionValue == ModeTransitionNone)
    {
        return;
    }

    if (transitionValue <= PatchMax)
    {
        _currentPatch = transitionValue;
        return;
    }

    _currentPatch = static_cast<byte>(transitionValue % (PatchMax + 1));
}

uint8_t PatchMode::ringBrightnessForButton(byte number) const
{
    if (number == PlayButtonIndex && !_isPlayButtonLit)
    {
        return 0;
    }

    return RingManager::FullBrightness;
}

void PatchMode::executeAction(ActionType action, byte actionValue)
{
    switch (action)
    {
    case ActionType::None:
        break;
    case ActionType::SendMidiProgramChange:
        changePatch(actionValue == PatchUpAction ? 1 : -1);
        break;
    case ActionType::SendMidiControlChange:
        break;
    case ActionType::ChangeMode:
        if (actionValue == static_cast<byte>(Modes::Home))
        {
            _transitionDelegate.enterMode(Modes::Home, ModeTransitionNone);
        }
        else if (actionValue == static_cast<byte>(Modes::Play))
        {
            _transitionDelegate.enterMode(Modes::Play,
                                          static_cast<byte>(ModeTransitionPatchReturnFlag | _currentPatch));
        }
        break;
    }
}

void PatchMode::changePatch(int8_t delta)
{
    const byte previousPatch = _currentPatch;

    if (delta > 0)
    {
        _currentPatch = (_currentPatch >= PatchMax) ? PatchMin : static_cast<byte>(_currentPatch + 1);
    }
    else if (delta < 0)
    {
        _currentPatch = (_currentPatch <= PatchMin) ? PatchMax : static_cast<byte>(_currentPatch - 1);
    }

    if (_currentPatch == previousPatch)
    {
        return;
    }

    renderPatchNumber(previousPatch, TFT_BLACK);

    _midiManager.sendProgramChange(_currentPatch);
    renderPatchNumber(_currentPatch, TFT_WHITE);
}
