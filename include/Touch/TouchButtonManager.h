#pragma once

#include "Touch/TouchButton.h"
#include "Touch/TouchButtonUi.h"

class TouchButtonManager : public ITouchButtonDelegate
{
  public:
    static constexpr uint8_t BUTTON_COUNT = 8;

    explicit TouchButtonManager(ITouchButtonLayout& screenUi, ITouchButtonDelegate* buttonPressDelegate = nullptr);
    ~TouchButtonManager();

    void initialize();
    void handleTouch(uint16_t x, uint16_t y);
    void handleTouchRelease();

    FootSwitchTouchButton* getButton(uint8_t index);

    // ITouchButtonDelegate implementation
    void buttonDown(const byte number) override;
    void buttonPressed(const byte number) override;
    void buttonLongPressed(const byte number) override;
    void buttonReleased(const byte number) override;

  private:
    TouchButtonManager() = delete;
    TouchButtonManager(const TouchButtonManager&) = delete;
    TouchButtonManager& operator=(const TouchButtonManager&) = delete;

    ITouchButtonLayout& screenUi;

    FootSwitchTouchButton* buttons[BUTTON_COUNT];
    ITouchButtonDelegate* _buttonPressDelegate;
    int8_t _lastPressedButton;
    uint32_t _pressStartedAtMs;
    bool _longPressDispatched;

    const Size boxSize;
    const int32_t boxWidth;
    const int32_t bottomRowY;

    void createButtons();
    bool isEnabledButton(int8_t number) const;
    void beginTouch(int8_t buttonNumber);
    void endTouch(bool fireShortPress);
};