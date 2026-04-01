#include "ButtonHandler.h"

#include "Button.h"
#include "Modes/Mode.h"
#include "RingManager.h"

namespace
{
const byte kButtonPins[ButtonHandler::ButtonCount] = {PB12, PB13, PB14, PB15, PA8, PA9, PA10, PA15};
}

ButtonHandler::ButtonHandler(IMode*& activeMode, RingManager& ringManager, void (*selectTouchButton)(byte))
    : _activeMode(activeMode), _ringManager(ringManager), _selectTouchButton(selectTouchButton), _buttons{nullptr}
{
}

ButtonHandler::~ButtonHandler()
{
    for (byte i = 0; i < ButtonCount; ++i)
    {
        if (_buttons[i] != nullptr)
        {
            delete _buttons[i];
            _buttons[i] = nullptr;
        }
    }
}

void ButtonHandler::begin()
{
    for (byte i = 0; i < ButtonCount; ++i)
    {
        if (_buttons[i] == nullptr)
        {
            _buttons[i] = new Button(i, kButtonPins[i], *this);
        }
    }
}

void ButtonHandler::updateButtons()
{
    for (byte i = 0; i < ButtonCount; ++i)
    {
        if (_buttons[i] != nullptr)
        {
            _buttons[i]->updateState();
        }
    }
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
