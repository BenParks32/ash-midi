#include "Touch/TouchButtonManager.h"

#include "Button.h"

TouchButtonManager::TouchButtonManager(ITouchButtonLayout& screenUi, ITouchButtonDelegate* buttonPressDelegate)
    : screenUi(screenUi), _buttonPressDelegate(buttonPressDelegate), _lastPressedButton(-1), _pressStartedAtMs(0),
      _longPressDispatched(false), boxSize(screenUi.boxSize()), boxWidth(screenUi.boxWidth()),
      bottomRowY(screenUi.bottomRowY())
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
    int8_t currentlyPressedButton = -1;

    // Find which button (if any) is being touched
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        FootSwitchTouchButton* button = buttons[i];
        if (!button->isEnabled())
        {
            continue;
        }

        Point loc = button->getLocation();
        Size sz = button->getSize();

        if (x >= loc.x && x <= loc.x + sz.width && y >= loc.y && y <= loc.y + sz.height)
        {
            currentlyPressedButton = i;
            break;
        }
    }

    // Only trigger lifecycle events if the pressed button changed.
    if (currentlyPressedButton != _lastPressedButton)
    {
        if (_lastPressedButton != -1)
        {
            endTouch(false);
        }

        if (currentlyPressedButton != -1)
        {
            beginTouch(currentlyPressedButton);
        }

        _lastPressedButton = currentlyPressedButton;
        return;
    }

    if (currentlyPressedButton == -1 || _longPressDispatched)
    {
        return;
    }

    if ((millis() - _pressStartedAtMs) >= ButtonLongPressMs)
    {
        _longPressDispatched = true;
        if (_buttonPressDelegate != nullptr)
        {
            _buttonPressDelegate->buttonLongPressed(static_cast<byte>(currentlyPressedButton));
        }
    }
}

void TouchButtonManager::handleTouchRelease()
{
    if (_lastPressedButton == -1)
    {
        return;
    }

    endTouch(true);
    _lastPressedButton = -1;
}

void TouchButtonManager::buttonDown(const byte number)
{
    if (!isEnabledButton(static_cast<int8_t>(number)))
    {
        return;
    }

    beginTouch(static_cast<int8_t>(number));
    _lastPressedButton = static_cast<int8_t>(number);
}

void TouchButtonManager::buttonPressed(const byte number)
{
    if (isEnabledButton(static_cast<int8_t>(number)) && _buttonPressDelegate != nullptr)
    {
        _buttonPressDelegate->buttonPressed(number);
    }
}

void TouchButtonManager::buttonLongPressed(const byte number)
{
    if (isEnabledButton(static_cast<int8_t>(number)) && _buttonPressDelegate != nullptr)
    {
        _buttonPressDelegate->buttonLongPressed(number);
    }
}

void TouchButtonManager::buttonReleased(const byte number)
{
    if (isEnabledButton(static_cast<int8_t>(number)) && _buttonPressDelegate != nullptr)
    {
        _buttonPressDelegate->buttonReleased(number);
    }
}

bool TouchButtonManager::isEnabledButton(int8_t number) const
{
    return number >= 0 && number < BUTTON_COUNT && buttons[number] != nullptr && buttons[number]->isEnabled();
}

void TouchButtonManager::beginTouch(int8_t buttonNumber)
{
    _pressStartedAtMs = millis();
    _longPressDispatched = false;

    if (_buttonPressDelegate != nullptr)
    {
        _buttonPressDelegate->buttonDown(static_cast<byte>(buttonNumber));
    }
}

void TouchButtonManager::endTouch(bool fireShortPress)
{
    if (_lastPressedButton == -1)
    {
        return;
    }

    const byte releasedButton = static_cast<byte>(_lastPressedButton);
    const bool shouldFireShortPress = fireShortPress && !_longPressDispatched;

    _pressStartedAtMs = 0;
    _longPressDispatched = false;

    if (_buttonPressDelegate != nullptr)
    {
        if (shouldFireShortPress)
        {
            _buttonPressDelegate->buttonPressed(releasedButton);
        }

        _buttonPressDelegate->buttonReleased(releasedButton);
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