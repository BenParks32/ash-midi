#include "Modes/PlayMode.h"
#include "ColorUtils.h"
#include "HxStompMidi.h"

namespace
{
void applyButtonVisualFromFunctionColour(FootSwitchTouchButton& button, uint16_t colour565, uint8_t ringBrightness)
{
    if (ringBrightness == 0)
    {
        button.setPillColour(TFT_BLACK);
        button.setBorderVisible(false);
        return;
    }

    button.setPillColour(colour565);
    button.setBorderVisible(ringBrightness >= RingManager::FullBrightness);
}
} // namespace

PlayMode::PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager)
    : _touchButtonManager(touchButtonManager), _ringManager(ringManager), _screenUi(screenUi),
      _midiManager(midiManager), _selectedHomeProgramChange(0), _selectedButton(0)
{
    setupFunctions();
}

void PlayMode::setSelectedHomeProgramChange(byte selectedHomeProgramChange)
{
    _selectedHomeProgramChange = selectedHomeProgramChange;
}

void PlayMode::setTransitionValue(byte transitionValue) { _selectedHomeProgramChange = transitionValue; }

void PlayMode::setupFunctions()
{
    _functions[0] = Function("Clean", 0x07E0, ActionType::SendMidiControlChange, 0, ActionType::None, 0);
    _functions[1] = Function("Crunch", 0xFD20, ActionType::SendMidiControlChange, 1, ActionType::None, 0);
    _functions[2] = Function("Lead", 0xF800, ActionType::SendMidiControlChange, 2, ActionType::None, 0);

    _functions[3] = Function();
    _functions[4] = Function();
    _functions[5] = Function();
    _functions[6] = Function();
    _functions[7] = Function();
}

void PlayMode::activate()
{
    if (_selectedHomeProgramChange > 0)
    {
        _midiManager.sendProgramChange(_selectedHomeProgramChange);
    }

    if (!isButtonEnabled(_selectedButton))
    {
        _selectedButton = 0;
    }

    updateVisuals();
}

void PlayMode::updateVisuals()
{
    // In play mode, exactly one enabled button is selected (full brightness).
    // Other enabled play buttons are dimmed.
    static constexpr byte kFirstPlayButton = 0;
    static constexpr byte kLastPlayButton = 2;

    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        FootSwitchTouchButton* button = _touchButtonManager.getButton(i);
        if (button == nullptr)
        {
            continue;
        }

        const Function& func = getFunction(i);
        const uint16_t colour565 = func.colour();
        const uint32_t ringColour888 = ColorUtils::rgb565To888(colour565);
        const bool enabled = isButtonEnabled(i);
        uint8_t ringBrightness = 0;
        if (enabled)
        {
            const bool isPlayFunctionButton = (i >= kFirstPlayButton && i <= kLastPlayButton);
            ringBrightness = (isPlayFunctionButton && i == _selectedButton) ? RingManager::FullBrightness
                                                                            : RingManager::DimBrightness;
        }

        button->setEnabled(enabled);
        button->setLabel(func.label());
        applyButtonVisualFromFunctionColour(*button, colour565, ringBrightness);

        if (enabled)
        {
            _ringManager.setRingColour(i, ringColour888);
            _ringManager.setRingBrightness(i, ringBrightness);
        }
        else
        {
            _ringManager.setRingColour(i, 0);
            _ringManager.setRingBrightness(i, 0);
        }

        button->draw(_screenUi);
    }
}

void PlayMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (!isButtonEnabled(number))
    {
        return;
    }

    const Function& func = getFunction(number);
    executeAction(func.pressAction(), func.pressActionValue());

    _selectedButton = number;
    updateVisuals();
}

void PlayMode::buttonLongPressed(byte number)
{
    (void)number;
    // Explicitly no action for Play mode long press.
}

void PlayMode::frameTick()
{
    // Play mode currently has no per-frame work.
}

const Function& PlayMode::getFunction(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return _functions[0];
    }
    return _functions[number];
}

bool PlayMode::isButtonEnabled(byte number) const { return !isEmptyLabel(getFunction(number).label()); }

void PlayMode::executeAction(ActionType action, byte actionValue)
{
    switch (action)
    {
    case ActionType::None:
        break;
    case ActionType::SendMidiProgramChange:
        _midiManager.sendProgramChange(actionValue);
        break;
    case ActionType::SendMidiControlChange:
        _midiManager.sendControlChange(STOMP_SNAPSHOT_CC, actionValue);
        break;
    case ActionType::ChangeMode:
        break;
    }
}

bool PlayMode::isEmptyLabel(const char* label)
{
    if (label == nullptr)
    {
        return true;
    }

    while (*label != '\0')
    {
        if (*label != ' ')
        {
            return false;
        }
        ++label;
    }

    return true;
}
