#include "ButtonHandler.h"

#include "Button.h"
#include "Modes/Mode.h"
#include "RingManager.h"

namespace
{
const byte kButtonPins[ButtonHandler::ButtonCount] = {PB12, PB13, PB14, PB15, PA8, PA9, PA10, PA15};
}

ButtonHandler::ButtonHandler(IMode*& activeMode, RingManager& ringManager)
    : _activeMode(activeMode), _ringManager(ringManager), _buttons{nullptr}, _trackedModes{nullptr}
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

void ButtonHandler::buttonDown(const byte number)
{
    if (number >= ButtonCount)
    {
        return;
    }

    _trackedModes[number] = _activeMode;
    if (_trackedModes[number] != nullptr)
    {
        _trackedModes[number]->buttonDown(number);
    }
}

void ButtonHandler::buttonPressed(const byte number)
{
    IMode* const mode = modeForButton(number);
    if (mode != nullptr)
    {
        mode->buttonPressed(number);
        return;
    }

    Serial.printf("Button %d pressed\n", number + 1);
}

void ButtonHandler::buttonLongPressed(const byte number)
{
    IMode* const mode = modeForButton(number);
    if (mode != nullptr)
    {
        mode->buttonLongPressed(number);
        return;
    }

    Serial.printf("Button %d long pressed\n", number + 1);
}

void ButtonHandler::buttonReleased(const byte number)
{
    IMode* const mode = modeForButton(number);
    if (mode != nullptr)
    {
        mode->buttonReleased(number);
    }

    clearTrackedMode(number);
}

IMode* ButtonHandler::modeForButton(byte number) const
{
    if (number >= ButtonCount)
    {
        return _activeMode;
    }

    if (_trackedModes[number] != nullptr)
    {
        return _trackedModes[number];
    }

    return _activeMode;
}

void ButtonHandler::clearTrackedMode(byte number)
{
    if (number < ButtonCount)
    {
        _trackedModes[number] = nullptr;
    }
}
