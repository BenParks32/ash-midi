#include "Function.h"
#include <cstring>

Function::Function() : _colour(0), _pressAction(ActionType::None), _longPressAction(ActionType::None)
{
    _label[0] = '\0';
}

Function::Function(const char* label, uint16_t colour, ActionType pressAction, ActionType longPressAction)
    : _colour(colour), _pressAction(pressAction), _longPressAction(longPressAction)
{
    setLabel(label);
}

const char* Function::label() const
{
    return _label;
}

uint16_t Function::colour() const
{
    return _colour;
}

ActionType Function::pressAction() const
{
    return _pressAction;
}

ActionType Function::longPressAction() const
{
    return _longPressAction;
}

void Function::setLabel(const char* label)
{
    if (label == nullptr || label[0] == '\0')
    {
        _label[0] = '\0';
        return;
    }

    strncpy(_label, label, LabelCapacity - 1U);
    _label[LabelCapacity - 1U] = '\0';
}

void Function::setColour(uint16_t colour)
{
    _colour = colour;
}

void Function::setPressAction(ActionType action)
{
    _pressAction = action;
}

void Function::setLongPressAction(ActionType action)
{
    _longPressAction = action;
}
