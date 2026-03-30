#include "Modes/HomeMode.h"

namespace
{
constexpr byte HomeFunctionCount = RingManager::RingCount;
const char* HomeFunctionLabels[HomeFunctionCount] = {"Amp", "Ampless", "CodeRed", " ", " ", " ", " ", " "};
} // namespace

HomeMode::HomeMode(FootSwitchTouchButton** touchButtons, byte touchButtonCount, RingManager& ringManager,
                   ScreenUi& screenUi, void (*selectTouchButton)(byte))
    : _touchButtons(touchButtons), _touchButtonCount(touchButtonCount), _ringManager(ringManager), _screenUi(screenUi),
      _selectTouchButton(selectTouchButton)
{
}

void HomeMode::activate()
{
    for (byte i = 0; i < _touchButtonCount; ++i)
    {
        FootSwitchTouchButton* button = _touchButtons[i];
        button->setSelected(false);
        button->setLabel(functionLabel(button->buttonNumber()));
        button->draw(_screenUi);
    }

    if (_selectTouchButton != nullptr)
    {
        _selectTouchButton(0);
    }
}

void HomeMode::buttonPressed(byte number)
{
    if (number >= _touchButtonCount)
    {
        return;
    }

    _ringManager.selectRing(number);

    if (_selectTouchButton != nullptr)
    {
        _selectTouchButton(number);
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
