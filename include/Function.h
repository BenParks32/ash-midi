#pragma once
#include <Arduino.h>

// Action type enum - describes what happens when a button is pressed/long pressed
enum class ActionType : uint8_t
{
    None = 0,
    SendMidiProgramChange = 1,
    SendMidiControlChange = 2,
    ChangeMode = 3,
    SelectScene = 4,
    SetTuner = 5,
    SelectHomePlaylist = 6,
    TapTempo = 7,
    SetGigView = 8
};

enum class FunctionBehaviour : uint8_t
{
    ButtonDown = 0,
    ButtonRelease = 1,
    ShortPress = 2,
    LongPress = 3,
    Count
};

struct FunctionAction
{
    ActionType type;
    uint8_t value;
    uint8_t secondaryValue;

    FunctionAction(ActionType type = ActionType::None, uint8_t value = 0, uint8_t secondaryValue = 0)
        : type(type), value(value), secondaryValue(secondaryValue)
    {
    }
};

class Function
{
  public:
    static constexpr size_t LabelCapacity = 16;

    Function();
    explicit Function(const char* label, uint16_t colour, ActionType shortPressAction, ActionType longPressAction);
    explicit Function(const char* label, uint16_t colour, ActionType shortPressAction, uint8_t shortPressActionValue,
                      ActionType longPressAction, uint8_t longPressActionValue);
    explicit Function(const char* label, uint16_t colour, const FunctionAction& buttonDownAction,
                      const FunctionAction& buttonReleaseAction, const FunctionAction& shortPressAction,
                      const FunctionAction& longPressAction);

    // Getters
    const char* label() const;
    uint16_t colour() const;
    const FunctionAction& action(FunctionBehaviour behaviour) const;
    bool hasMomentaryBehaviour() const;
    bool isToggle() const;

    // Setters
    void setLabel(const char* label);
    void setColour(uint16_t colour);
    void setAction(FunctionBehaviour behaviour, const FunctionAction& action);
    void setToggle(bool toggle);

  private:
    static FunctionAction buildLegacyAction(ActionType action, uint8_t actionValue);

    char _label[LabelCapacity];
    uint16_t _colour;
    FunctionAction _actions[static_cast<uint8_t>(FunctionBehaviour::Count)];
    bool _isToggle;
};
