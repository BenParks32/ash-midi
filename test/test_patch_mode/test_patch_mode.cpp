#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "MidiProvider.h"
#include "Modes/ModeManager.h"
#include "Modes/PatchMode.h"

// Pull in implementation units required by PatchMode without linking app main.cpp.
#include "../../src/Function.cpp"
#include "../../src/Lights.cpp"
#include "../../src/Modes/FunctionModeBase.cpp"
#include "../../src/Modes/ModeManager.cpp"
#include "../../src/Modes/PatchMode.cpp"
#include "../../src/RingManager.cpp"
#include "../../src/ScreenUi.cpp"
#include "../../src/Touch/TouchButton.cpp"
#include "../../src/Touch/TouchButtonManager.cpp"

namespace
{
class MockMidiManager : public IMidiManager
{
  public:
    void setChannel(byte channel) override { currentChannel = channel; }
    byte channel() const override { return currentChannel; }

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
    byte currentChannel = 1;
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
    Modes lastMode = Modes::Patch;
    ModeTransitionValue lastTransitionValue = 0;
};

class MockMidiProvider : public IMidiProvider
{
  public:
    byte maxPresetIndex() const override { return 127; }

    void recallPreset(byte presetIndex) override
    {
        ++recallPresetCalls;
        lastRecallPreset = presetIndex;
    }

    void selectScene(byte sceneIndex) override
    {
        ++selectSceneCalls;
        lastSceneIndex = sceneIndex;
    }

    void setTunerEnabled(bool enabled) override
    {
        ++setTunerCalls;
        lastTunerEnabled = enabled;
    }

    int recallPresetCalls = 0;
    int selectSceneCalls = 0;
    int setTunerCalls = 0;
    byte lastRecallPreset = 0;
    byte lastSceneIndex = 0;
    bool lastTunerEnabled = false;
};

class PatchModeFixture
{
  public:
    PatchModeFixture()
        : screenSize{480, 320}, ui(tft, screenSize), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800),
          ringManager(strip), touchButtonManager(ui), midiManager(), midiProvider(), transitionDelegate(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, transitionDelegate)
    {
    }

    const Size screenSize;
    TFT_eSPI tft;
    ScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockMidiProvider midiProvider;
    MockTransitionDelegate transitionDelegate;
    PatchMode mode;
};

void stepUp(PatchModeFixture& fixture, byte steps)
{
    for (byte i = 0; i < steps; ++i)
    {
        fixture.mode.buttonPressed(7);
    }
}

void test_activate_sets_patch_labels_and_disables_unused_buttons()
{
    PatchModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING("Down", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Play", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Up", fixture.touchButtonManager.getButton(7)->label());
}

void test_buttons_one_two_three_are_noop()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(0);
    fixture.mode.buttonPressed(1);
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_button_eight_increments_patch_and_sends_program_change()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.midiProvider.lastRecallPreset);
}

void test_button_eight_wraps_patch_from_127_to_0()
{
    PatchModeFixture fixture;

    fixture.mode.setTransitionValue(127);
    fixture.mode.activate();

    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
}

void test_button_four_decrements_patch_and_wraps_from_0_to_127()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(127, fixture.midiProvider.lastRecallPreset);
}

void test_transition_value_sets_starting_patch_number()
{
    PatchModeFixture fixture;

    fixture.mode.setTransitionValue(6);
    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(7, fixture.midiProvider.lastRecallPreset);
}

void test_button_five_transitions_to_play_without_program_change()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionPatchReturnFlag, fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
}

void test_button_seven_is_disabled_in_patch_mode()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
}

void test_button_five_long_press_transitions_to_home()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
}

void test_long_press_non_home_button_is_noop()
{
    PatchModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(7);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
}

void test_patch_mode_transitions_via_mode_manager_to_play_slot()
{
    const Size screenSize = {480, 320};
    TFT_eSPI tft;
    ScreenUi ui(tft, screenSize);
    Adafruit_NeoPixel strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    TouchButtonManager touchButtonManager(ui);
    MockMidiManager midiManager;

    class TestModeSlot : public IMode
    {
      public:
        void activate() override { ++activateCalls; }
        void buttonPressed(const byte) override {}
        void buttonLongPressed(const byte) override {}
        void frameTick() override {}

        void setTransitionValue(ModeTransitionValue transitionValue) override
        {
            ++setTransitionValueCalls;
            lastTransitionValue = transitionValue;
        }

        int activateCalls = 0;
        int setTransitionValueCalls = 0;
        ModeTransitionValue lastTransitionValue = 0;
    };

    TestModeSlot homeSlot;
    TestModeSlot playSlot;
    TestModeSlot patchSlot;
    IMode* menuSlot = nullptr;
    IMode* buttonDiagnosticSlot = nullptr;

    IMode* activeMode = &patchSlot;
    IMode* modes[ModeCount] = {&homeSlot, &playSlot, &patchSlot, menuSlot, buttonDiagnosticSlot};
    ModeManager modeManager(activeMode, modes);
    MockMidiProvider midiProvider;
    PatchMode mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, modeManager);

    mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_PTR(&playSlot, activeMode);
    TEST_ASSERT_EQUAL_INT(1, playSlot.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionPatchReturnFlag, playSlot.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, playSlot.activateCalls);
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
    RUN_TEST(test_activate_sets_patch_labels_and_disables_unused_buttons);
    RUN_TEST(test_buttons_one_two_three_are_noop);
    RUN_TEST(test_button_eight_increments_patch_and_sends_program_change);
    RUN_TEST(test_button_eight_wraps_patch_from_127_to_0);
    RUN_TEST(test_button_four_decrements_patch_and_wraps_from_0_to_127);
    RUN_TEST(test_transition_value_sets_starting_patch_number);
    RUN_TEST(test_button_five_transitions_to_play_without_program_change);
    RUN_TEST(test_button_seven_is_disabled_in_patch_mode);
    RUN_TEST(test_button_five_long_press_transitions_to_home);
    RUN_TEST(test_long_press_non_home_button_is_noop);
    RUN_TEST(test_patch_mode_transitions_via_mode_manager_to_play_slot);
    UNITY_END();
}

void loop() {}
