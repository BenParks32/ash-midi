#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "Modes/ButtonDiagnosticMode.h"

// Pull in implementation units required by ButtonDiagnosticMode without linking app main.cpp.
#include "../../src/Function.cpp"
#include "../../src/Lights.cpp"
#include "../../src/Modes/ButtonDiagnosticMode.cpp"
#include "../../src/Modes/FunctionModeBase.cpp"
#include "../../src/RingManager.cpp"
#include "../../src/ScreenUi.cpp"
#include "../../src/Touch/TouchButton.cpp"
#include "../../src/Touch/TouchButtonManager.cpp"

namespace
{
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
    void enterMode(Modes mode, byte transitionValue) override
    {
        ++calls;
        lastMode = mode;
        lastTransitionValue = transitionValue;
    }

    int calls = 0;
    Modes lastMode = Modes::ButtonDiagnostic;
    byte lastTransitionValue = 0xFF;
};

class ButtonDiagnosticModeFixture
{
  public:
    ButtonDiagnosticModeFixture()
        : screenSize{480, 320}, ui(tft, screenSize), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800),
          ringManager(strip), touchButtonManager(ui), midiManager(), transitionDelegate(),
          mode(touchButtonManager, ringManager, ui, midiManager, transitionDelegate)
    {
    }

    const Size screenSize;
    TFT_eSPI tft;
    ScreenUi ui;
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
        TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, button->pillColour());
        TEST_ASSERT_FALSE(button->hasBorder());
        TEST_ASSERT_EQUAL_UINT32(0, fixture.strip.getPixelColor(i * RingManager::LedsPerRing));
    }
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
    TEST_ASSERT_EQUAL_UINT8(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}
} // namespace

void setUp() {}

void tearDown() {}

void setup()
{
    Serial.begin(115200);
    const uint32_t start = millis();
    while (!Serial && (millis() - start) < 5000U)
    {
        delay(10);
    }

    UNITY_BEGIN();
    RUN_TEST(test_activate_initializes_all_buttons_off);
    RUN_TEST(test_short_press_sets_green_for_button);
    RUN_TEST(test_long_press_sets_red_for_button);
    RUN_TEST(test_encoder_press_exits_to_menu_mode);
    UNITY_END();
}

void loop() {}
