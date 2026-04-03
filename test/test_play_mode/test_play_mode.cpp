#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "HxStompMidi.h"
#include "Modes/PlayMode.h"

// Pull in implementation units required by PlayMode without linking app main.cpp.
#include "../../src/Function.cpp"
#include "../../src/Lights.cpp"
#include "../../src/Modes/FunctionModeBase.cpp"
#include "../../src/Modes/PlayMode.cpp"
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
    Modes lastMode = Modes::Play;
    byte lastTransitionValue = 0xFF;
};

class PlayModeFixture
{
  public:
    PlayModeFixture()
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
    PlayMode mode;
};

void test_activate_sets_play_labels()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Clean", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Crunch", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("Lead", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Home", fixture.touchButtonManager.getButton(4)->label());
}

void test_activate_selects_single_button_and_dims_others()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(2)->hasBorder());

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(1)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(2)->pillColour());
}

void test_activate_sends_selected_home_program_change()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedHomeProgramChange(6);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.programChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.midiManager.lastProgramChangeValue);
}

void test_activate_sends_selected_home_program_change_zero()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedHomeProgramChange(0);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.programChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiManager.lastProgramChangeValue);
}

void test_clean_sends_control_change_zero()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(STOMP_SNAPSHOT_CC, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiManager.lastControlChangeValue);
}

void test_crunch_sends_control_change_one()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(STOMP_SNAPSHOT_CC, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.midiManager.lastControlChangeValue);
}

void test_lead_sends_control_change_two()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(STOMP_SNAPSHOT_CC, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.midiManager.lastControlChangeValue);
}

void test_button_press_changes_selection_to_single_button()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(2)->hasBorder());
}

void test_button_pressed_ignores_disabled_button()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_long_press_is_noop()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(0);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_button_five_transitions_back_to_home_mode()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT8(0, fixture.transitionDelegate.lastTransitionValue);
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
    RUN_TEST(test_activate_sets_play_labels);
    RUN_TEST(test_activate_selects_single_button_and_dims_others);
    RUN_TEST(test_activate_sends_selected_home_program_change);
    RUN_TEST(test_activate_sends_selected_home_program_change_zero);
    RUN_TEST(test_clean_sends_control_change_zero);
    RUN_TEST(test_crunch_sends_control_change_one);
    RUN_TEST(test_lead_sends_control_change_two);
    RUN_TEST(test_button_press_changes_selection_to_single_button);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_long_press_is_noop);
    RUN_TEST(test_button_five_transitions_back_to_home_mode);
    UNITY_END();
}

void loop() {}
