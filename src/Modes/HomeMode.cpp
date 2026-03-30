#include "Modes/HomeMode.h"

namespace
{
constexpr byte HomeFunctionCount = RingManager::RingCount;
const char* HomeFunctionLabels[HomeFunctionCount] = {"Amp", "Ampless", "CodeRed", " ", " ", " ", " ", " "};

static uint16_t rgb888To565(uint32_t rgb)
{
    const uint8_t r = (uint8_t)((rgb >> 16) & 0xFFU);
    const uint8_t g = (uint8_t)((rgb >> 8) & 0xFFU);
    const uint8_t b = (uint8_t)(rgb & 0xFFU);
    return (uint16_t)(((uint16_t)(r & 0xF8U) << 8) | ((uint16_t)(g & 0xFCU) << 3) | ((uint16_t)b >> 3));
}
} // namespace

HomeMode::HomeMode(FootSwitchTouchButton** touchButtons, byte touchButtonCount, RingManager& ringManager,
                   ScreenUi& screenUi)
    : _touchButtons(touchButtons), _touchButtonCount(touchButtonCount), _ringManager(ringManager), _screenUi(screenUi)
{
}

void HomeMode::activate()
{
    for (byte i = 0; i < _touchButtonCount; ++i)
    {
        FootSwitchTouchButton* button = _touchButtons[i];
        const byte number = button->buttonNumber();
        const bool enabled = isButtonEnabled(number);

        button->setSelected(false);
        button->setLabel(functionLabel(number));
        button->setPillColour(enabled ? rgb888To565(RingManager::defaultRingColour(number)) : TFT_BLACK);

        _ringManager.setRingColour(number, enabled ? RingManager::defaultRingColour(number) : 0);
        _ringManager.setRingBrightness(number, enabled ? RingManager::FullBrightness : 0);

        button->draw(_screenUi);
    }
}

void HomeMode::buttonPressed(byte number)
{
    if (number >= _touchButtonCount)
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

    Serial.printf("Home mode: button %u -> %s\n", number + 1U, label);
}

void HomeMode::buttonLongPressed(byte number)
{
    if (number >= _touchButtonCount)
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
