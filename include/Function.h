#pragma once
#include <Arduino.h>

// Action type enum - describes what happens when a button is pressed/long pressed
enum class ActionType : uint8_t
{
    None = 0,
    SendMidiProgramChange = 1,
    SendMidiControlChange = 2,
    ChangeMode = 3
};

// Data-only class describing a button function
// Contains visual appearance (label, colour) and action types
// No behaviour - used for serialisation and configuration
class Function
{
  public:
    static constexpr size_t LabelCapacity = 16;

    Function();
    explicit Function(const char* label, uint16_t colour, ActionType pressAction, ActionType longPressAction);

    // Getters
    const char* label() const;
    uint16_t colour() const;
    ActionType pressAction() const;
    ActionType longPressAction() const;

    // Setters
    void setLabel(const char* label);
    void setColour(uint16_t colour);
    void setPressAction(ActionType action);
    void setLongPressAction(ActionType action);

  private:
    char _label[LabelCapacity];
    uint16_t _colour;
    ActionType _pressAction;
    ActionType _longPressAction;
};
