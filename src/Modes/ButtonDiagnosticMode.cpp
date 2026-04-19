#include "Modes/ButtonDiagnosticMode.h"

#include "ColorUtils.h"

namespace
{
constexpr uint16_t OffColour = TFT_BLACK;
constexpr uint16_t ShortPressColour = 0x07E0;
constexpr uint16_t LongPressColour = 0xF800;
constexpr const char* DiagnosticTitleText = "Button Diagnostic";
constexpr const char* DiagnosticHintText = "Click wheel to exit";
constexpr uint8_t DiagnosticTitleScale = 1;
constexpr int32_t DiagnosticTitleOffsetY = 18;
constexpr int32_t DiagnosticHintRightPadding = 16;
constexpr int32_t DiagnosticHintBottomPadding = 20;
constexpr int32_t SmallTextCharWidthPx = 12;
} // namespace

ButtonDiagnosticMode::ButtonDiagnosticMode(TouchButtonManager& touchButtonManager, RingManager& ringManager,
                                           ScreenUi& screenUi, IMidiManager& midiManager,
                                           IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _buttonStates{ButtonState::Off}
{
    setupFunctions();
    resetStates();
}

void ButtonDiagnosticMode::setupFunctions()
{
    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        char label[4] = {'\0'};
        label[0] = 'B';
        label[1] = static_cast<char>('1' + i);
        _functions[i] = Function(label, OffColour, ActionType::None, 0, ActionType::None, 0);
    }
}

void ButtonDiagnosticMode::resetStates()
{
    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        _buttonStates[i] = ButtonState::Off;
        _functions[i].setColour(OffColour);
    }
}

void ButtonDiagnosticMode::activate()
{
    resetStates();
    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        applyButtonVisual(i);
    }

    renderCenterUi(TFT_WHITE);
}

void ButtonDiagnosticMode::deactivate() { renderCenterUi(TFT_BLACK); }

void ButtonDiagnosticMode::buttonPressed(byte number) { setButtonState(number, ButtonState::ShortPress); }

void ButtonDiagnosticMode::buttonLongPressed(byte number) { setButtonState(number, ButtonState::LongPress); }

void ButtonDiagnosticMode::frameTick()
{
    // Diagnostic mode currently has no per-frame animation.
}

void ButtonDiagnosticMode::encoderPressed() { _transitionDelegate.enterMode(Modes::Menu, ModeTransitionNone); }

void ButtonDiagnosticMode::setButtonState(byte number, ButtonState state)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    _buttonStates[number] = state;
    _functions[number].setColour(colourForState(state));
    applyButtonVisual(number);
}

void ButtonDiagnosticMode::renderCenterUi(uint16_t textColour)
{
    const int32_t screenWidth = _screenUi.boxWidth() * 4;
    const int32_t centerTopY = _screenUi.boxHeight();
    const int32_t centerBottomY = _screenUi.bottomRowY();
    const int32_t hintTextWidth =
        static_cast<int32_t>((sizeof("Click wheel to exit") - 1U) * static_cast<size_t>(SmallTextCharWidthPx));

    _screenUi.drawCenteredText(FF32, DiagnosticTitleScale, DiagnosticTitleText, screenWidth / 2,
                               centerTopY + DiagnosticTitleOffsetY, textColour, TFT_BLACK);

    const int32_t hintX = screenWidth - DiagnosticHintRightPadding - hintTextWidth;
    const int32_t hintY = centerBottomY - DiagnosticHintBottomPadding;
    _screenUi.drawSmallText(DiagnosticHintText, hintX, hintY, textColour, TFT_BLACK);
}

void ButtonDiagnosticMode::applyButtonVisual(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    FootSwitchTouchButton* button = _touchButtonManager.getButton(number);
    if (button == nullptr)
    {
        return;
    }

    const bool isOffState = (_buttonStates[number] == ButtonState::Off);
    const uint16_t pillColour = _functions[number].colour();

    button->setEnabled(true);
    button->setLabel(_functions[number].label());
    button->setBorderVisible(false);

    if (isOffState)
    {
        button->setPillColour(TFT_BLACK);
        _ringManager.setRingColour(number, 0);
        _ringManager.setRingBrightness(number, 0);
    }
    else
    {
        button->setPillColour(pillColour);
        _ringManager.setRingColour(number, ColorUtils::rgb565To888(pillColour));
        _ringManager.setRingBrightness(number, RingManager::FullBrightness);
    }

    button->draw(_screenUi);
}

uint16_t ButtonDiagnosticMode::colourForState(ButtonState state) const
{
    switch (state)
    {
    case ButtonState::ShortPress:
        return ShortPressColour;
    case ButtonState::LongPress:
        return LongPressColour;
    case ButtonState::Off:
    default:
        return OffColour;
    }
}

uint8_t ButtonDiagnosticMode::ringBrightnessForButton(byte number) const
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return 0;
    }

    return (_buttonStates[number] == ButtonState::Off) ? 0 : RingManager::FullBrightness;
}
