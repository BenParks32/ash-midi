#include "TouchButtonManager.h"

TouchButtonManager::TouchButtonManager(ITouchButtonLayout& screenUi, ITouchButtonDelegate* buttonPressDelegate)
    : screenUi(screenUi), _buttonPressDelegate(buttonPressDelegate), boxSize(screenUi.boxSize()),
      boxWidth(screenUi.boxWidth()), bottomRowY(screenUi.bottomRowY())
{
    createButtons();
}

TouchButtonManager::~TouchButtonManager()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        delete buttons[i];
    }
}

void TouchButtonManager::createButtons()
{
    // Top row (buttons 4-7)
    buttons[4] = new FootSwitchTouchButton(4, {0, 0}, boxSize, "", 0, *this);
    buttons[5] = new FootSwitchTouchButton(5, {boxWidth, 0}, boxSize, "", 0, *this);
    buttons[6] = new FootSwitchTouchButton(6, {boxWidth * 2, 0}, boxSize, "", 0, *this);
    buttons[7] = new FootSwitchTouchButton(7, {boxWidth * 3, 0}, boxSize, "", 0, *this);

    // Bottom row (buttons 0-3)
    buttons[0] = new FootSwitchTouchButton(0, {0, bottomRowY}, boxSize, "", 0, *this);
    buttons[1] = new FootSwitchTouchButton(1, {boxWidth, bottomRowY}, boxSize, "", 0, *this);
    buttons[2] = new FootSwitchTouchButton(2, {boxWidth * 2, bottomRowY}, boxSize, "", 0, *this);
    buttons[3] = new FootSwitchTouchButton(3, {boxWidth * 3, bottomRowY}, boxSize, "", 0, *this);
}

void TouchButtonManager::initialize()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        buttons[i]->draw(screenUi);
    }
}

void TouchButtonManager::handleTouch(uint16_t x, uint16_t y)
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        if (buttons[i]->handleTouch(x, y))
        {
            break;
        }
    }
}

void TouchButtonManager::buttonPressed(const byte number)
{
    FootSwitchTouchButton* pressedButton = nullptr;
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        if (buttons[i]->buttonNumber() == number)
        {
            pressedButton = buttons[i];
            break;
        }
    }

    if (pressedButton == nullptr || !pressedButton->isEnabled())
    {
        return;
    }

    if (_buttonPressDelegate != nullptr)
    {
        _buttonPressDelegate->buttonPressed(number);
    }
}

FootSwitchTouchButton* TouchButtonManager::getButton(uint8_t index)
{
    if (index < BUTTON_COUNT)
    {
        return buttons[index];
    }
    return nullptr;
}
