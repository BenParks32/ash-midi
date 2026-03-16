#pragma once
#include <Arduino.h>

class IButtonDelegate
{
  public:
    virtual void buttonPressed(const byte number) = 0;
    virtual void buttonLongPressed(const byte number) = 0;
};

class ButtonManager
{
  public:
    static ButtonManager& instance();
    void registerButton(class Button* button);

  private:
    ButtonManager() = default;
    ButtonManager(const ButtonManager& rhs) = default;

    static constexpr int kMaxInterrupts = 8;
    void handleInterrupt(int interruptNum);

    static void isr0();
    static void isr1();
    static void isr2();
    static void isr3();
    static void isr4();
    static void isr5();
    static void isr6();
    static void isr7();

    static Button* s_instances[kMaxInterrupts];
    static void (* const s_isr[kMaxInterrupts])();
};

class Button
{
  public:
    Button(const byte number, const byte pin, IButtonDelegate& delegate);

  private:
    Button();
    Button(const Button& rhs);

  public:
    void updateState();

  private:
    void handleInterrupt();

    // Only ButtonManager should call the interrupt handler.
    friend class ButtonManager;

  private:
    const byte _number;
    const byte _pin;
    IButtonDelegate& _delegate;
    long _chrono;
    bool _buttonDown;
    bool _longPressed;

    // Debounce state (prevents bounce-triggered repeats)
    bool _debouncing;
    uint32_t _debounceStart;
};