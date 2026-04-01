#include "TouchButton.h"

#include <cstdio>
#include <cstring>

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

byte TouchButton::buttonNumber() const { return number; }

FootSwitchTouchButton::FootSwitchTouchButton(const byte number, const Point location, const Size size,
                                             const char* label, uint16_t pillColour, ITouchButtonDelegate& delegate)
    : TouchButton(number, location, size, delegate), _label{0}, _pillColour(pillColour), _isSelected(false)
{
    setLabel(label);
}

void FootSwitchTouchButton::draw(ITouchButtonCanvas& ui)
{
    ui.drawTouchButtonLabelAndPill(_label, location, size, _pillColour, _isSelected, _pillColour);
}

void FootSwitchTouchButton::setLabel(const char* label)
{
    if (label == nullptr || label[0] == '\0')
    {
        snprintf(_label, LabelCapacity, "%u", (unsigned int)number + 1U);
        return;
    }

    strncpy(_label, label, LabelCapacity - 1U);
    _label[LabelCapacity - 1U] = '\0';
}

const char* FootSwitchTouchButton::label() const { return _label; }

void FootSwitchTouchButton::setPillColour(uint16_t pillColour) { _pillColour = pillColour; }

uint16_t FootSwitchTouchButton::pillColour() const { return _pillColour; }

void FootSwitchTouchButton::setSelected(bool selected) { _isSelected = selected; }

bool FootSwitchTouchButton::isSelected() const { return _isSelected; }