#include "Modes/PlayMode.h"
#include "ColorUtils.h"
#include "HxStompMidi.h"

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
    _functions[4] =
        Function("Patch", 0xFFE0, ActionType::ChangeMode, static_cast<byte>(Modes::Patch), ActionType::None, 0);
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

    updateVisuals();
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
        button->setBorderVisible(ringBrightness >= RingManager::FullBrightness);
        _ringManager.setRingColour(number, ColorUtils::rgb565To888(colour565));
        _ringManager.setRingBrightness(number, ringBrightness);
    }

    button->draw(_screenUi);
}

void PlayMode::updateSnapshotSelectionVisuals(byte previousSelected, byte currentSelected)
{
    static constexpr byte kFirstPlayButton = 0;
    static constexpr byte kLastPlayButton = 2;

    if (previousSelected >= kFirstPlayButton && previousSelected <= kLastPlayButton)
    {
        renderButton(previousSelected);
    }

    if (currentSelected != previousSelected && currentSelected >= kFirstPlayButton &&
        currentSelected <= kLastPlayButton)
    {
        renderButton(currentSelected);
    }
}

uint8_t PlayMode::ringBrightnessForButton(byte number) const
{
    // Snapshot buttons (0-2) are mutually exclusive: selected is bright, others are dim.
    // Patch navigation button is always bright when enabled.
    static constexpr byte kFirstPlayButton = 0;
    static constexpr byte kLastPlayButton = 2;
    static constexpr byte kPatchButton = 4;

    if (number == kPatchButton)
    {
        return RingManager::FullBrightness;
    }

    const bool isPlayFunctionButton = (number >= kFirstPlayButton && number <= kLastPlayButton);
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
    // Explicitly no action for Play mode long press.
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
