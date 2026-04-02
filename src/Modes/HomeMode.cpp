#include "Modes/HomeMode.h"

HomeMode::HomeMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate)
{
    setupFunctions();
}

void HomeMode::setupFunctions()
{
    // Button 0: Amp (Green) -> enter Play and select Home Program 0
    _functions[0] = Function("Amp", 0x07E0, ActionType::ChangeMode, 0, ActionType::None, 0);

    // Button 1: Ampless (Blue) -> enter Play and select Home Program 6
    _functions[1] = Function("Ampless", 0x001F, ActionType::ChangeMode, 6, ActionType::None, 0);

    // Button 2: CodeRed (Red) -> enter Play and select Home Program 20
    _functions[2] = Function("CodeRed", 0xF800, ActionType::ChangeMode, 20, ActionType::None, 0);

    // Buttons 3-7: Disabled slots use Function default constructor values.
    _functions[3] = Function();
    _functions[4] = Function();
    _functions[5] = Function();
    _functions[6] = Function();
    _functions[7] = Function();
}

void HomeMode::activate() { renderAllButtons(); }

void HomeMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        Serial.printf("Home mode: button %u is disabled\n", number + 1U);
        return;
    }

    const Function& func = getFunction(number);
    if (isEmptyLabel(func.label()))
    {
        Serial.printf("Home mode: button %u is empty\n", number + 1U);
        return;
    }

    executeAction(func.pressAction(), func.pressActionValue());
    Serial.printf("Home mode: button %u -> %s\n", number + 1U, func.label());
}

void HomeMode::buttonLongPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        Serial.printf("Home mode: button %u long press (disabled)\n", number + 1U);
        return;
    }

    const Function& func = getFunction(number);
    if (isEmptyLabel(func.label()))
    {
        Serial.printf("Home mode: button %u long press (empty)\n", number + 1U);
        return;
    }

    executeAction(func.longPressAction(), func.longPressActionValue());
    Serial.printf("Home mode: button %u long press -> %s\n", number + 1U, func.label());
}

void HomeMode::frameTick()
{
    // Home mode currently has no per-frame work.
}

uint8_t HomeMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}

void HomeMode::executeAction(ActionType action, byte actionValue)
{
    switch (action)
    {
    case ActionType::None:
        break;
    case ActionType::SendMidiProgramChange:
        _midiManager.sendProgramChange(actionValue);
        break;
    case ActionType::SendMidiControlChange:
        _midiManager.sendControlChange(actionValue, 127);
        break;
    case ActionType::ChangeMode:
        _transitionDelegate.enterMode(Modes::Play, actionValue);
        break;
    }
}
