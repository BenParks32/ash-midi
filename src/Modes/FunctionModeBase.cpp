#include "Modes/FunctionModeBase.h"
#include "ColorUtils.h"

#include <cstring>

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

FunctionModeBase::FunctionModeBase(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                                   IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate)
    : _touchButtonManager(touchButtonManager), _ringManager(ringManager), _screenUi(screenUi),
      _midiManager(midiManager), _transitionDelegate(transitionDelegate)
{
}

const Function& FunctionModeBase::getFunction(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return _functions[0];
    }
    return _functions[number];
}

bool FunctionModeBase::isButtonEnabled(byte number) const { return !isEmptyLabel(getFunction(number).label()); }

bool FunctionModeBase::isEmptyLabel(const char* label)
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

void FunctionModeBase::renderAllButtons()
{
    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        FootSwitchTouchButton* button = _touchButtonManager.getButton(i);
        if (button == nullptr)
        {
            continue;
        }

        const Function& func = getFunction(i);
        const uint16_t colour565 = func.colour();
        const bool enabled = isButtonEnabled(i);
        const uint8_t ringBrightness = enabled ? ringBrightnessForButton(i) : 0;
        const uint16_t desiredPillColour = (ringBrightness == 0) ? TFT_BLACK : colour565;
        const bool desiredBorderVisible = (ringBrightness >= RingManager::FullBrightness);
        const char* desiredLabel = func.label();

        const bool isUnchanged = (button->isEnabled() == enabled) && (button->pillColour() == desiredPillColour) &&
                                 (button->hasBorder() == desiredBorderVisible) &&
                                 (std::strcmp(button->label(), desiredLabel) == 0);

        if (enabled)
        {
            _ringManager.setRingColour(i, ColorUtils::rgb565To888(colour565));
            _ringManager.setRingBrightness(i, ringBrightness);
        }
        else
        {
            _ringManager.setRingColour(i, 0);
            _ringManager.setRingBrightness(i, 0);
        }

        if (isUnchanged)
        {
            continue;
        }

        button->setEnabled(enabled);
        button->setLabel(desiredLabel);
        applyButtonVisualFromFunctionColour(*button, colour565, ringBrightness);

        button->draw(_screenUi);
    }
}