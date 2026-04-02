#include "Modes/HomeMode.h"

namespace
{
constexpr byte HomeFunctionCount = RingManager::RingCount;
const char* HomeFunctionLabels[HomeFunctionCount] = {"Amp", "Ampless", "CodeRed", " ", " ", " ", " ", " "};
const uint32_t HomeFunctionRingColours[HomeFunctionCount] = {
    0x00FF00, 0x0000FF, 0xFF0000, 0xFF0000, 0xDCA500, 0x008080, 0xFF80FF, 0xFFFFFF,
};

static uint16_t rgb888To565(uint32_t rgb)
{
    const uint8_t r = (uint8_t)((rgb >> 16) & 0xFFU);
    const uint8_t g = (uint8_t)((rgb >> 8) & 0xFFU);
    const uint8_t b = (uint8_t)(rgb & 0xFFU);
    return (uint16_t)(((uint16_t)(r & 0xF8U) << 8) | ((uint16_t)(g & 0xFCU) << 3) | ((uint16_t)b >> 3));
}

uint32_t homeFunctionRingColour(byte number)
{
    if (number >= HomeFunctionCount)
    {
        return 0;
    }
    return HomeFunctionRingColours[number];
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
        const byte number = button->buttonNumber();
        const bool enabled = isButtonEnabled(number);
        const uint32_t ringColour = homeFunctionRingColour(number);
        const uint8_t ringBrightness = enabled ? RingManager::FullBrightness : 0;

        button->setEnabled(enabled);
        button->setLabel(functionLabel(number));
        applyButtonVisualFromRing(*button, ringColour, ringBrightness);

        if (enabled)
        {
            _ringManager.setRingColour(number, ringColour);
            _ringManager.setRingBrightness(number, ringBrightness);
        }
        else
        {
            _ringManager.setRingColour(number, 0);
            _ringManager.setRingBrightness(number, ringBrightness);
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

    const char* label = functionLabel(number);
    if (isEmptyLabel(label))
    {
        Serial.printf("Home mode: button %u is empty\n", number + 1U);
        return;
    }

    sendProgramChangeForButton(number);
    Serial.printf("Home mode: button %u -> %s\n", number + 1U, label);
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

    const char* label = functionLabel(number);
    if (isEmptyLabel(label))
    {
        Serial.printf("Home mode: button %u long press (empty)\n", number + 1U);
        return;
    }

    Serial.printf("Home mode: button %u long press -> %s\n", number + 1U, label);
}

void HomeMode::frameTick()
{
    // Home mode currently has no per-frame work.
}

const char* HomeMode::functionLabel(byte number) const
{
    if (number >= HomeFunctionCount)
    {
        return " ";
    }
    return HomeFunctionLabels[number];
}

bool HomeMode::isButtonEnabled(byte number) const { return !isEmptyLabel(functionLabel(number)); }

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
