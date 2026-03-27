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
                                             const Resources& resources, ITouchButtonDelegate& delegate)
    : TouchButton(number, location, size, delegate), _resources(resources)
{
}

void FootSwitchTouchButton::draw(ScreenUi& ui) { ui.drawIconCentered(_resources.footSwitchIcon(), location, size); }