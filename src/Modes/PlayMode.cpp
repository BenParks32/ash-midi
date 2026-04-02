#include "Modes/PlayMode.h"
#include "HxStompMidi.h"

PlayMode::PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _selectedHomeProgramChange(0), _selectedButton(0)
{
    setupFunctions();
}

void PlayMode::setSelectedHomeProgramChange(byte selectedHomeProgramChange)
{
    _selectedHomeProgramChange = selectedHomeProgramChange;
}

void PlayMode::setTransitionValue(byte transitionValue) { _selectedHomeProgramChange = transitionValue; }

void PlayMode::setupFunctions()
{
    _functions[0] = Function("Clean", 0x07E0, ActionType::SendMidiControlChange, 0, ActionType::None, 0);
    _functions[1] = Function("Crunch", 0xFD20, ActionType::SendMidiControlChange, 1, ActionType::None, 0);
    _functions[2] = Function("Lead", 0xF800, ActionType::SendMidiControlChange, 2, ActionType::None, 0);

    _functions[3] = Function();
    _functions[4] = Function("Home", 0xFFFF, ActionType::ChangeMode, 0, ActionType::None, 0);
    _functions[5] = Function();
    _functions[6] = Function();
    _functions[7] = Function();
}

void PlayMode::activate()
{
    if (_selectedHomeProgramChange > 0)
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

uint8_t PlayMode::ringBrightnessForButton(byte number) const
{
    // In play mode, exactly one enabled play button is selected (full brightness).
    // Other enabled buttons are dimmed.
    static constexpr byte kFirstPlayButton = 0;
    static constexpr byte kLastPlayButton = 2;

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

    _selectedButton = number;
    updateVisuals();
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
        _transitionDelegate.enterMode(Modes::Home, actionValue);
        break;
    }
}
