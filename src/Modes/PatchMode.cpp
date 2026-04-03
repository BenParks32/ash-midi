#include "Modes/PatchMode.h"

#include "ColorUtils.h"

namespace
{
const byte PatchMin = 0;
const byte PatchMax = 39;
const byte PatchDownAction = 0;
const byte PatchUpAction = 1;
const byte PlayButtonIndex = 6;
const uint32_t PlayButtonFlashHalfPeriodMs = 500;
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
    if (delta > 0)
    {
        _currentPatch = (_currentPatch >= PatchMax) ? PatchMin : static_cast<byte>(_currentPatch + 1);
    }
    else if (delta < 0)
    {
        _currentPatch = (_currentPatch <= PatchMin) ? PatchMax : static_cast<byte>(_currentPatch - 1);
    }

    _midiManager.sendProgramChange(_currentPatch);
}
