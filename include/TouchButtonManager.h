#pragma once
#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButton.h"

class TouchButtonManager : public ITouchButtonDelegate
{
  public:
    static constexpr uint8_t BUTTON_COUNT = RingManager::RingCount;

    TouchButtonManager(ScreenUi& screenUi, RingManager& ringManager);
    ~TouchButtonManager();

    void initialize();
    void handleTouch(uint16_t x, uint16_t y);

    FootSwitchTouchButton* getButton(uint8_t index);
    FootSwitchTouchButton* getSelectedButton() const;

    // ITouchButtonDelegate implementation
    void buttonPressed(const byte number) override;

  private:
    void selectButton(uint8_t buttonNumber);

  private:
    ScreenUi& screenUi;
    RingManager& ringManager;

    FootSwitchTouchButton* buttons[BUTTON_COUNT];
    FootSwitchTouchButton* selectedButton;

    const Size boxSize;
    const int32_t boxWidth;
    const int32_t bottomRowY;

    void createButtons();
};
