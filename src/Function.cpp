#include "Function.h"
#include <cstring>

Function::Function()
    : _colour(0), _actions{}
{
    setLabel(" ");
}

Function::Function(const char* label, uint16_t colour, ActionType shortPressAction, ActionType longPressAction)
    : _colour(colour), _actions{}
{
    setLabel(label);
    setAction(FunctionBehaviour::ShortPress, buildLegacyAction(shortPressAction, 0));
    setAction(FunctionBehaviour::LongPress, buildLegacyAction(longPressAction, 0));
}

Function::Function(const char* label, uint16_t colour, ActionType shortPressAction, uint8_t shortPressActionValue,
                   ActionType longPressAction, uint8_t longPressActionValue)
    : _colour(colour), _actions{}
{
    setLabel(label);
    setAction(FunctionBehaviour::ShortPress, buildLegacyAction(shortPressAction, shortPressActionValue));
    setAction(FunctionBehaviour::LongPress, buildLegacyAction(longPressAction, longPressActionValue));
}

Function::Function(const char* label, uint16_t colour, const FunctionAction& buttonDownAction,
                   const FunctionAction& buttonReleaseAction, const FunctionAction& shortPressAction,
                   const FunctionAction& longPressAction)
    : _colour(colour), _actions{}
{
    setLabel(label);
    setAction(FunctionBehaviour::ButtonDown, buttonDownAction);
    setAction(FunctionBehaviour::ButtonRelease, buttonReleaseAction);
    setAction(FunctionBehaviour::ShortPress, shortPressAction);
    setAction(FunctionBehaviour::LongPress, longPressAction);
}

const char* Function::label() const { return _label; }

uint16_t Function::colour() const { return _colour; }

const FunctionAction& Function::action(FunctionBehaviour behaviour) const
{
    return _actions[static_cast<uint8_t>(behaviour)];
}

bool Function::hasMomentaryBehaviour() const
{
    return action(FunctionBehaviour::ButtonDown).type != ActionType::None ||
           action(FunctionBehaviour::ButtonRelease).type != ActionType::None;
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

void Function::setColour(uint16_t colour) { _colour = colour; }

void Function::setAction(FunctionBehaviour behaviour, const FunctionAction& action)
{
    _actions[static_cast<uint8_t>(behaviour)] = action;
}

FunctionAction Function::buildLegacyAction(ActionType action, uint8_t actionValue)
{
    return FunctionAction(action, actionValue, action == ActionType::SendMidiControlChange ? 127U : 0U);
}
