#include "ButtonHandler.h"

#include "Modes/Mode.h"
#include "RingManager.h"

ButtonHandler::ButtonHandler(IMode*& activeMode, RingManager& ringManager, void (*selectTouchButton)(byte))
    : _activeMode(activeMode), _ringManager(ringManager), _selectTouchButton(selectTouchButton)
{
}

void ButtonHandler::buttonPressed(const byte number)
{
    if (_activeMode != nullptr)
    {
        _activeMode->buttonPressed(number);
        return;
    }

    // Fallback path if no mode is currently active.
    _ringManager.selectRing(number);
    if (_selectTouchButton != nullptr)
    {
        _selectTouchButton(number);
    }
}

void ButtonHandler::buttonLongPressed(const byte number)
{
    if (_activeMode != nullptr)
    {
        _activeMode->buttonLongPressed(number);
        return;
    }

    Serial.printf("Button %d long pressed\n", number + 1);
}
