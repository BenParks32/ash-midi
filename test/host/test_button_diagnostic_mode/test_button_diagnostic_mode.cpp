#include <unity.h>

#include "Modes/ButtonDiagnosticMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/ButtonDiagnosticMode.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
class FakeScreenUi : public IScreenUi
{
  public:
    void drawBackgroundAndBorder() override { ++drawBackgroundAndBorderCalls; }

    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t) override { ++drawCenteredFrameCalls; }

    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawCenteredFrameCalls;
    }

    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*) override
    {
        ++drawLogoCalls;
    }

    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*, uint16_t, uint16_t,
                  uint16_t, uint16_t) override
    {
        ++drawLogoWithColoursCalls;
    }

    void drawText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawTextCalls;
    }

    void fillRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override { ++fillRectCalls; }

    void drawRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override { ++drawRectCalls; }

    void drawSmallText(const char*, int32_t, int32_t, uint16_t, uint16_t) override { ++drawSmallTextCalls; }

    void drawCenteredText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawCenteredTextCalls;
    }

    void drawTouchButtonLabelAndPill(const char*, const Point&, const Size&, uint16_t, bool, uint16_t,
                                     uint16_t) override
    {
        ++drawTouchButtonLabelAndPillCalls;
    }

    void drawTouchButtonPill(const Point&, const Size&, uint16_t, uint16_t) override { ++drawTouchButtonPillCalls; }

    void setSdStatusInitializing() override { ++setSdStatusInitializingCalls; }
    void setSdStatusFailed() override { ++setSdStatusFailedCalls; }
    void setSdStatusReady() override { ++setSdStatusReadyCalls; }
    void setSdStatusNotMounted() override { ++setSdStatusNotMountedCalls; }
    void hideSdStatus() override { ++hideSdStatusCalls; }
    void redrawSdStatus() override { ++redrawSdStatusCalls; }

    uint16_t touchButtonPillBorderColour() const override { return TFT_WHITE; }
    int32_t boxWidth() const override { return 120; }
    int32_t boxHeight() const override { return 80; }
    int32_t bottomRowY() const override { return 240; }
    Size boxSize() const override { return {120, 80}; }

    int drawBackgroundAndBorderCalls = 0;
    int drawCenteredFrameCalls = 0;
    int drawLogoCalls = 0;
    int drawLogoWithColoursCalls = 0;
    int drawTextCalls = 0;
    int fillRectCalls = 0;
    int drawRectCalls = 0;
    int drawSmallTextCalls = 0;
    int drawCenteredTextCalls = 0;
    int drawTouchButtonLabelAndPillCalls = 0;
    int drawTouchButtonPillCalls = 0;
    int setSdStatusInitializingCalls = 0;
    int setSdStatusFailedCalls = 0;
    int setSdStatusReadyCalls = 0;
    int setSdStatusNotMountedCalls = 0;
    int hideSdStatusCalls = 0;
    int redrawSdStatusCalls = 0;
};

class MockMidiManager : public IMidiManager
{
  public:
    void setChannel(byte channel) override { _channel = channel; }
    byte channel() const override { return _channel; }

    void sendProgramChange(byte programChangeValue) override
    {
        ++programChangeCalls;
        lastProgramChangeValue = programChangeValue;
    }

    void sendControlChange(byte controlChangeNumber, byte controlChangeValue) override
    {
        ++controlChangeCalls;
        lastControlChangeNumber = controlChangeNumber;
        lastControlChangeValue = controlChangeValue;
    }

    int programChangeCalls = 0;
    int controlChangeCalls = 0;
    byte lastProgramChangeValue = 0;
    byte lastControlChangeNumber = 0;
    byte lastControlChangeValue = 0;

  private:
    byte _channel = 1;
};

class MockTransitionDelegate : public IModeTransistionDelegate
{
  public:
    void enterMode(Modes mode, ModeTransitionValue transitionValue) override
    {
        ++calls;
        lastMode = mode;
        lastTransitionValue = transitionValue;
    }

    int calls = 0;
    Modes lastMode = Modes::ButtonDiagnostic;
    ModeTransitionValue lastTransitionValue = ModeTransitionNone;
};

class ButtonDiagnosticModeFixture
{
  public:
    ButtonDiagnosticModeFixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), transitionDelegate(), mode(touchButtonManager, ringManager, ui, midiManager, transitionDelegate)
    {
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockTransitionDelegate transitionDelegate;
    ButtonDiagnosticMode mode;
};

void test_activate_initializes_all_buttons_off()
{
    ButtonDiagnosticModeFixture fixture;

    fixture.mode.activate();

    for (uint8_t i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        FootSwitchTouchButton* button = fixture.touchButtonManager.getButton(i);
        TEST_ASSERT_NOT_NULL(button);
        TEST_ASSERT_TRUE(button->isEnabled());
        TEST_ASSERT_EQUAL_STRING_LEN("B1", button->label(), 1);
        TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, button->pillColour());
        TEST_ASSERT_FALSE(button->hasBorder());
        TEST_ASSERT_EQUAL_UINT32(0, fixture.strip.getPixelColor(i * RingManager::LedsPerRing));
    }

    TEST_ASSERT_EQUAL_INT(1, fixture.ui.drawCenteredTextCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.drawSmallTextCalls);
}

void test_short_press_sets_green_for_button()
{
    ButtonDiagnosticModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_UINT16(0x07E0, fixture.touchButtonManager.getButton(2)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT32(0, fixture.strip.getPixelColor(2 * RingManager::LedsPerRing));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.strip.getPixelColor(1 * RingManager::LedsPerRing));
}

void test_long_press_sets_red_for_button()
{
    ButtonDiagnosticModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(5);
    fixture.mode.buttonLongPressed(5);

    TEST_ASSERT_EQUAL_UINT16(0xF800, fixture.touchButtonManager.getButton(5)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT32(0, fixture.strip.getPixelColor(5 * RingManager::LedsPerRing));
}

void test_encoder_press_exits_to_menu_mode()
{
    ButtonDiagnosticModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Menu),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}
} // namespace

void setUp()
{
    HostArduino::resetTime();
    HostArduino::resetPins();
}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_activate_initializes_all_buttons_off);
    RUN_TEST(test_short_press_sets_green_for_button);
    RUN_TEST(test_long_press_sets_red_for_button);
    RUN_TEST(test_encoder_press_exits_to_menu_mode);
    return UNITY_END();
}
