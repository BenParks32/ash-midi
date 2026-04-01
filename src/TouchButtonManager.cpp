#include "TouchButtonManager.h"

static constexpr uint16_t ringRgb888To565(uint32_t rgb)
{
    const uint8_t r = (uint8_t)((rgb >> 16) & 0xFFU);
    const uint8_t g = (uint8_t)((rgb >> 8) & 0xFFU);
    const uint8_t b = (uint8_t)(rgb & 0xFFU);
    return (uint16_t)(((uint16_t)(r & 0xF8U) << 8) | ((uint16_t)(g & 0xFCU) << 3) | ((uint16_t)b >> 3));
}

TouchButtonManager::TouchButtonManager(ITouchButtonLayout& screenUi, const IRingColourProvider& ringColourProvider)
    : screenUi(screenUi), ringColourProvider(ringColourProvider), selectedButton(nullptr), boxSize(screenUi.boxSize()),
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
    // Top row (buttons 4-7, displayed as 5-8)
    buttons[4] = new FootSwitchTouchButton(4, {0, 0}, boxSize, "5",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(4)), *this);
    buttons[5] = new FootSwitchTouchButton(5, {boxWidth, 0}, boxSize, "6",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(5)), *this);
    buttons[6] = new FootSwitchTouchButton(6, {boxWidth * 2, 0}, boxSize, "7",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(6)), *this);
    buttons[7] = new FootSwitchTouchButton(7, {boxWidth * 3, 0}, boxSize, "8",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(7)), *this);

    // Bottom row (buttons 0-3, displayed as 1-4)
    buttons[0] = new FootSwitchTouchButton(0, {0, bottomRowY}, boxSize, "1",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(0)), *this);
    buttons[1] = new FootSwitchTouchButton(1, {boxWidth, bottomRowY}, boxSize, "2",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(1)), *this);
    buttons[2] = new FootSwitchTouchButton(2, {boxWidth * 2, bottomRowY}, boxSize, "3",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(2)), *this);
    buttons[3] = new FootSwitchTouchButton(3, {boxWidth * 3, bottomRowY}, boxSize, "4",
                                           ringRgb888To565(ringColourProvider.defaultRingColour(3)), *this);
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
    FootSwitchTouchButton* nextSelectedButton = nullptr;
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        if (buttons[i]->buttonNumber() == number)
        {
            nextSelectedButton = buttons[i];
            break;
        }
    }

    if (nextSelectedButton == nullptr || nextSelectedButton == selectedButton)
    {
        return;
    }

    if (selectedButton != nullptr)
    {
        selectedButton->setSelected(false);
        selectedButton->draw(screenUi);
    }

    nextSelectedButton->setSelected(true);
    nextSelectedButton->draw(screenUi);
    selectedButton = nextSelectedButton;
}

FootSwitchTouchButton* TouchButtonManager::getButton(uint8_t index)
{
    if (index < BUTTON_COUNT)
    {
        return buttons[index];
    }
    return nullptr;
}

FootSwitchTouchButton* TouchButtonManager::getSelectedButton() const { return selectedButton; }
