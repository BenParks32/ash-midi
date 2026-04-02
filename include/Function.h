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
    explicit Function(const char* label, uint16_t colour, ActionType pressAction, uint8_t pressActionValue,
                      ActionType longPressAction, uint8_t longPressActionValue);

    // Getters
    const char* label() const;
    uint16_t colour() const;
    ActionType pressAction() const;
    uint8_t pressActionValue() const;
    ActionType longPressAction() const;
    uint8_t longPressActionValue() const;

    // Setters
    void setLabel(const char* label);
    void setColour(uint16_t colour);
    void setPressAction(ActionType action);
    void setPressActionValue(uint8_t actionValue);
    void setLongPressAction(ActionType action);
    void setLongPressActionValue(uint8_t actionValue);

  private:
    char _label[LabelCapacity];
    uint16_t _colour;
    ActionType _pressAction;
    uint8_t _pressActionValue;
    ActionType _longPressAction;
    uint8_t _longPressActionValue;
};
