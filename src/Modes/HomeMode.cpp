#include "Modes/HomeMode.h"

namespace
{
static uint16_t rgb888To565(uint32_t rgb)
{
    const uint8_t r = (uint8_t)((rgb >> 16) & 0xFFU);
    const uint8_t g = (uint8_t)((rgb >> 8) & 0xFFU);
    const uint8_t b = (uint8_t)(rgb & 0xFFU);
    return (uint16_t)(((uint16_t)(r & 0xF8U) << 8) | ((uint16_t)(g & 0xFCU) << 3) | ((uint16_t)b >> 3));
}

void applyButtonVisualFromRing(FootSwitchTouchButton& button, uint32_t ringColour, uint8_t ringBrightness)
{
    if (ringBrightness == 0)
    {
        button.setPillColour(TFT_BLACK);
        button.setBorderVisible(false);
        return;
    }

    button.setPillColour(rgb888To565(ringColour));
    button.setBorderVisible(ringBrightness >= RingManager::FullBrightness);
}
} // namespace

HomeMode::HomeMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager)
    : _touchButtonManager(touchButtonManager), _ringManager(ringManager), _screenUi(screenUi), _midiManager(midiManager)
{
    setupFunctions();
}

void HomeMode::setupFunctions()
{
    // Button 0: Amp (Green) - MIDI Program 1
    _functions[0] = Function("Amp", 0x00FF00, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);

    // Button 1: Ampless (Blue) - MIDI Program 6
    _functions[1] = Function("Ampless", 0x0000FF, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);

    // Button 2: CodeRed (Red) - MIDI Program 20
    _functions[2] = Function("CodeRed", 0x0000FF, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);

    // Buttons 3-7: Additional preset slots (disabled for now)
    _functions[3] = Function(" ", 0x0000FF, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);
    _functions[4] = Function(" ", 0xDCA500, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);
    _functions[5] = Function(" ", 0x008080, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);
    _functions[6] = Function(" ", 0xFF80FF, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);
    _functions[7] = Function(" ", 0xFFFFFF, ActionType::SendMidiProgramChange, ActionType::SendMidiProgramChange);
}

void HomeMode::activate()
{
    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        FootSwitchTouchButton* button = _touchButtonManager.getButton(i);
        if (button == nullptr)
        {
            continue;
        }

        const Function& func = getFunction(i);
        const bool enabled = isButtonEnabled(i);
        const uint8_t ringBrightness = enabled ? RingManager::FullBrightness : 0;

        button->setEnabled(enabled);
        button->setLabel(func.label());
        applyButtonVisualFromRing(*button, func.colour(), ringBrightness);

        if (enabled)
        {
            _ringManager.setRingColour(i, func.colour());
            _ringManager.setRingBrightness(i, ringBrightness);
        }
        else
        {
            _ringManager.setRingColour(i, 0);
            _ringManager.setRingBrightness(i, 0);
        }

        button->draw(_screenUi);
    }
}

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

    executeAction(number, func.pressAction());
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

    executeAction(number, func.longPressAction());
    Serial.printf("Home mode: button %u long press -> %s\n", number + 1U, func.label());
}

void HomeMode::frameTick()
{
    // Home mode currently has no per-frame work.
}

const Function& HomeMode::getFunction(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return _functions[0]; // Return first function as fallback
    }
    return _functions[number];
}

bool HomeMode::isButtonEnabled(byte number) const { return !isEmptyLabel(getFunction(number).label()); }

void HomeMode::executeAction(byte number, ActionType action)
{
    switch (action)
    {
    case ActionType::SendMidiProgramChange:
        sendProgramChangeForButton(number);
        break;
    case ActionType::SendMidiControlChange:
        // TODO: Implement CC sending if needed
        break;
    case ActionType::ChangeMode:
        // TODO: Implement mode changing if needed
        break;
    }
}

void HomeMode::sendProgramChangeForButton(byte number)
{
    byte programChangeValue = 0;
    if (!tryGetProgramChangeValue(number, programChangeValue))
    {
        return;
    }

    _midiManager.sendProgramChange(programChangeValue);
}

bool HomeMode::tryGetProgramChangeValue(byte number, byte& programChangeValue)
{
    switch (number)
    {
    case 0:
        programChangeValue = 1;
        return true;
    case 1:
        programChangeValue = 6;
        return true;
    case 2:
        programChangeValue = 20;
        return true;
    default:
        return false;
    }
}

bool HomeMode::isEmptyLabel(const char* label)
{
    if (label == nullptr)
    {
        return true;
    }

    while (*label != '\0')
    {
        if (*label != ' ')
        {
            return false;
        }
        ++label;
    }

    return true;
}
