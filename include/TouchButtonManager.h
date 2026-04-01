#pragma once
#include "Touch/IRingColourProvider.h"
#include "Touch/TouchButtonUi.h"
#include "TouchButton.h"

class TouchButtonManager : public ITouchButtonDelegate
{
  public:
    static constexpr uint8_t BUTTON_COUNT = 8;

    TouchButtonManager(ITouchButtonLayout& screenUi, const IRingColourProvider& ringColourProvider);
    ~TouchButtonManager();

    void initialize();
    void handleTouch(uint16_t x, uint16_t y);

    FootSwitchTouchButton* getButton(uint8_t index);
    FootSwitchTouchButton* getSelectedButton() const;

    // ITouchButtonDelegate implementation
    void buttonPressed(const byte number) override;

  private:
    TouchButtonManager() = delete;
    TouchButtonManager(const TouchButtonManager&) = delete;
    TouchButtonManager& operator=(const TouchButtonManager&) = delete;

    ITouchButtonLayout& screenUi;
    const IRingColourProvider& ringColourProvider;

    FootSwitchTouchButton* buttons[BUTTON_COUNT];
    FootSwitchTouchButton* selectedButton;

    const Size boxSize;
    const int32_t boxWidth;
    const int32_t bottomRowY;

    void createButtons();
};
