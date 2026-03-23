#include "Button.h"

const uint16_t LONG_PRESS_MS = 1000;

Button* ButtonManager::s_instances[ButtonManager::kMaxInterrupts] = {nullptr};

void (*const ButtonManager::s_isr[ButtonManager::kMaxInterrupts])() = {
    ButtonManager::isr0, ButtonManager::isr1, ButtonManager::isr2, ButtonManager::isr3,
    ButtonManager::isr4, ButtonManager::isr5, ButtonManager::isr6, ButtonManager::isr7};

ButtonManager& ButtonManager::instance()
{
    static ButtonManager inst;
    return inst;
}

void ButtonManager::registerButton(Button* button)
{
    int interruptNum = digitalPinToInterrupt(button->_pin);
    s_instances[button->_number] = button;
    attachInterrupt(interruptNum, s_isr[button->_number], FALLING);
}

void ButtonManager::handleInterrupt(int interruptNum)
{
    Button* btn = s_instances[interruptNum];
    if (btn)
    {
        btn->handleInterrupt();
    }
}

void ButtonManager::isr0() { instance().handleInterrupt(0); }
void ButtonManager::isr1() { instance().handleInterrupt(1); }
void ButtonManager::isr2() { instance().handleInterrupt(2); }
void ButtonManager::isr3() { instance().handleInterrupt(3); }
void ButtonManager::isr4() { instance().handleInterrupt(4); }
void ButtonManager::isr5() { instance().handleInterrupt(5); }
void ButtonManager::isr6() { instance().handleInterrupt(6); }
void ButtonManager::isr7() { instance().handleInterrupt(7); }

static constexpr uint16_t DEBOUNCE_MS = 20;

Button::Button(const byte number, const byte pin, IButtonDelegate& delegate)
    : _number(number), _pin(pin), _delegate(delegate), _chrono(0), _buttonDown(false), _debouncing(false),
      _debounceStart(0)
{
    pinMode(_pin, INPUT_PULLUP);
    ButtonManager::instance().registerButton(this);
}

void Button::handleInterrupt()
{
    // Interrupts are noisy; just start the debounce timer.
    if (!_buttonDown && !_debouncing)
    {
        _debouncing = true;
        _debounceStart = millis();
    }
}

void Button::updateState()
{
    byte pinIs = digitalRead(_pin);
    uint32_t now = millis();

    // Detect a clean press (falling edge + debounce)
    if (!_buttonDown)
    {
        if (!_debouncing)
        {
            if (pinIs == LOW)
            {
                _debouncing = true;
                _debounceStart = now;
            }
        }
        else
        {
            if (pinIs == LOW && (now - _debounceStart) >= DEBOUNCE_MS)
            {
                _debouncing = false;
                _buttonDown = true;
                _longPressed = false;
                _chrono = now;
            }
            else if (pinIs == HIGH)
            {
                // Bounce cancelled
                _debouncing = false;
            }
        }
    }
    else
    {
        // Button is held; fire long-press as soon as duration passes, then wait for release.
        if (!_longPressed && (now - _chrono) >= LONG_PRESS_MS)
        {
            _longPressed = true;
            _delegate.buttonLongPressed(_number);
        }

        if (!_debouncing)
        {
            if (pinIs == HIGH)
            {
                _debouncing = true;
                _debounceStart = now;
            }
        }
        else
        {
            if (pinIs == HIGH && (now - _debounceStart) >= DEBOUNCE_MS)
            {
                _debouncing = false;
                _buttonDown = false;

                if (!_longPressed)
                {
                    _delegate.buttonPressed(_number);
                }
            }
            else if (pinIs == LOW)
            {
                // Bounce cancelled; keep holding
                _debouncing = false;
            }
        }
    }
}
