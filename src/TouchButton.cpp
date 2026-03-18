#include "TouchButton.h"

TouchButton::TouchButton(const byte number, const Point location, const Size size, ITouchButtonDelegate& delegate)
    : number(number), location(location), size(size), delegate(delegate)
{
}

bool TouchButton::handleTouch(const uint16_t x, const uint16_t y)
{
    if (x >= location.x && x <= location.x + size.width && y >= location.y && y <= location.y + size.height)
    {
        delegate.buttonPressed(number);
        return true;
    }
    return false;
}

FootSwitchTouchButton::FootSwitchTouchButton(const byte number, const Point location, const Size size,
                                             const uint16_t* icon, ITouchButtonDelegate& delegate)
    : TouchButton(number, location, size, delegate), icon(icon)
{
}

void FootSwitchTouchButton::draw(TFT_eSPI& tft)
{
    const Size iconSize = {48, 48};
    const int32_t x = location.x + (size.width - iconSize.width) / 2;
    const int32_t y = location.y + (size.height - iconSize.height) / 2;
    tft.pushImage(x, y, iconSize.width, iconSize.height, icon);
}