#include "Function.h"
#include <cstring>

Function::Function()
    : _colour(0), _pressAction(ActionType::None), _pressActionValue(0), _longPressAction(ActionType::None),
      _longPressActionValue(0)
{
    setLabel(" ");
}

Function::Function(const char* label, uint16_t colour, ActionType pressAction, ActionType longPressAction)
    : _colour(colour), _pressAction(pressAction), _pressActionValue(0), _longPressAction(longPressAction),
      _longPressActionValue(0)
{
    setLabel(label);
}

Function::Function(const char* label, uint16_t colour, ActionType pressAction, uint8_t pressActionValue,
                   ActionType longPressAction, uint8_t longPressActionValue)
    : _colour(colour), _pressAction(pressAction), _pressActionValue(pressActionValue),
      _longPressAction(longPressAction), _longPressActionValue(longPressActionValue)
{
    setLabel(label);
}

const char* Function::label() const { return _label; }

uint16_t Function::colour() const { return _colour; }

ActionType Function::pressAction() const { return _pressAction; }

uint8_t Function::pressActionValue() const { return _pressActionValue; }

ActionType Function::longPressAction() const { return _longPressAction; }

uint8_t Function::longPressActionValue() const { return _longPressActionValue; }

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

void Function::setColour(uint16_t colour) { _colour = colour; }

void Function::setPressAction(ActionType action) { _pressAction = action; }

void Function::setPressActionValue(uint8_t actionValue) { _pressActionValue = actionValue; }

void Function::setLongPressAction(ActionType action) { _longPressAction = action; }

void Function::setLongPressActionValue(uint8_t actionValue) { _longPressActionValue = actionValue; }
