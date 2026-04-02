#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "Modes/HomeMode.h"
#include "Modes/ModeManager.h"

// Pull in implementation units required by HomeMode without linking app main.cpp.
#include "../../src/Function.cpp"
#include "../../src/Lights.cpp"
#include "../../src/Modes/HomeMode.cpp"
#include "../../src/Modes/ModeManager.cpp"
#include "../../src/RingManager.cpp"
#include "../../src/ScreenUi.cpp"
#include "../../src/TouchButton.cpp"
#include "../../src/TouchButtonManager.cpp"

namespace
{
class MockMidiManager : public IMidiManager
{
  public:
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
    Modes lastMode = Modes::Home;
    byte lastTransitionValue = 0;
};

class HomeModeFixture
{
  public:
    HomeModeFixture()
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
    HomeMode mode;
};

class TestModeSlot : public IMode
{
  public:
    void activate() override { ++activateCalls; }
    void buttonPressed(const byte) override {}
    void buttonLongPressed(const byte) override {}
    void frameTick() override {}

    void setTransitionValue(byte transitionValue) override
    {
        ++setTransitionValueCalls;
        lastTransitionValue = transitionValue;
    }

    int activateCalls = 0;
    int setTransitionValueCalls = 0;
    byte lastTransitionValue = 0;
};

void test_activate_sets_home_labels_and_ring_matched_visuals()
{
    HomeModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Amp", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Ampless", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("CodeRed", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT32(0, fixture.strip.getPixelColor(RingManager::LedsPerRing * 0));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.strip.getPixelColor(RingManager::LedsPerRing * 5));
}

void test_button_pressed_selects_requested_button_when_valid()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT8(6, fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
}

void test_button_pressed_selects_amp_home_program_for_play_mode()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT8(0, fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
}

void test_button_pressed_selects_code_red_home_program_for_play_mode()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT8(20, fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
}

void test_button_pressed_ignores_disabled_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_button_pressed_ignores_out_of_range_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(TouchButtonManager::BUTTON_COUNT);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_button_long_pressed_ignores_out_of_range_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonLongPressed(TouchButtonManager::BUTTON_COUNT);
}

void test_button_long_pressed_on_enabled_home_button_is_noop()
{
    HomeModeFixture fixture;

    fixture.mode.buttonLongPressed(0);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_frame_tick_noop()
{
    HomeModeFixture fixture;

    fixture.mode.activate();
    const uint32_t before = fixture.strip.getPixelColor(RingManager::LedsPerRing * 5);

    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_UINT32(before, fixture.strip.getPixelColor(RingManager::LedsPerRing * 5));
}

void test_button_press_transitions_via_mode_manager_to_play_slot()
{
    const Size screenSize = {480, 320};
    TFT_eSPI tft;
    ScreenUi ui(tft, screenSize);
    Adafruit_NeoPixel strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    TouchButtonManager touchButtonManager(ui);
    MockMidiManager midiManager;

    TestModeSlot homeSlot;
    TestModeSlot playSlot;

    IMode* activeMode = &homeSlot;
    IMode* modes[ModeCount] = {&homeSlot, &playSlot};
    ModeManager modeManager(activeMode, modes);
    HomeMode mode(touchButtonManager, ringManager, ui, midiManager, modeManager);

    mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_PTR(&playSlot, activeMode);
    TEST_ASSERT_EQUAL_INT(1, playSlot.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT8(20, playSlot.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, playSlot.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, midiManager.programChangeCalls);
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
    RUN_TEST(test_activate_sets_home_labels_and_ring_matched_visuals);
    RUN_TEST(test_button_pressed_selects_requested_button_when_valid);
    RUN_TEST(test_button_pressed_selects_amp_home_program_for_play_mode);
    RUN_TEST(test_button_pressed_selects_code_red_home_program_for_play_mode);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_button_pressed_ignores_out_of_range_button);
    RUN_TEST(test_button_long_pressed_ignores_out_of_range_button);
    RUN_TEST(test_button_long_pressed_on_enabled_home_button_is_noop);
    RUN_TEST(test_frame_tick_noop);
    RUN_TEST(test_button_press_transitions_via_mode_manager_to_play_slot);
    UNITY_END();
}

void loop() {}
