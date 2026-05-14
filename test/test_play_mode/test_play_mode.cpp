#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "Modes/PlayMode.h"

// Pull in implementation units required by PlayMode without linking app main.cpp.
#include "../../src/Function.cpp"
#include "../../src/Lights.cpp"
#include "../../src/Modes/FunctionModeBase.cpp"
#include "../../src/Modes/PlayMode.cpp"
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
    Modes lastMode = Modes::Play;
    ModeTransitionValue lastTransitionValue = ModeTransitionNone;
};

class MockMidiProvider : public IMidiProvider
{
  public:
    enum class CallType : uint8_t
    {
        RecallPreset,
        SelectScene,
        SetTuner,
    };

    byte maxPresetIndex() const override { return 127; }

    void recallPreset(byte presetIndex) override
    {
        ++recallPresetCalls;
        lastRecallPreset = presetIndex;
        callOrder[callCount++] = CallType::RecallPreset;
    }

    void selectScene(byte sceneIndex) override
    {
        ++selectSceneCalls;
        lastSceneIndex = sceneIndex;
        callOrder[callCount++] = CallType::SelectScene;
    }

    void setTunerEnabled(bool enabled) override
    {
        ++setTunerCalls;
        lastTunerEnabled = enabled;
        callOrder[callCount++] = CallType::SetTuner;
    }

    int recallPresetCalls = 0;
    int selectSceneCalls = 0;
    int setTunerCalls = 0;
    byte lastRecallPreset = 0;
    byte lastSceneIndex = 0;
    bool lastTunerEnabled = false;
    CallType callOrder[8] = {};
    int callCount = 0;
};

class PlayModeFixture
{
  public:
    PlayModeFixture()
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
    PlayMode mode;
};

void test_activate_sets_play_labels()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Scene A", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Scene B", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("Scene C", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Patch", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Tuner", fixture.touchButtonManager.getButton(7)->label());
}

void test_activate_selects_single_button_and_dims_others()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(2)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(4)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(1)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(2)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(4)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(7)->pillColour());
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());
}

void test_activate_recalls_selected_preset_before_scene_select()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.callCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::RecallPreset),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[0]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::SelectScene),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[1]));
}

void test_activate_recalls_selected_preset_zero()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(0);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
}

void test_activate_skips_program_change_for_none_transition()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionNone);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_skips_program_change_for_patch_return_transition()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | 9);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_selects_first_scene()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
}

void test_scene_a_selects_scene_zero()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
}

void test_scene_b_selects_scene_one()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.midiProvider.lastSceneIndex);
}

void test_scene_c_selects_scene_two()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.midiProvider.lastSceneIndex);
}

void test_tuner_button_enables_tuner_without_changing_scene_selection()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastTunerEnabled);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());
}

void test_tuner_button_long_press_disables_tuner()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_FALSE(fixture.midiProvider.lastTunerEnabled);
}

void test_button_press_changes_selection_to_single_button()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(2)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(4)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());
}

void test_button_pressed_ignores_disabled_button()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_long_press_is_noop()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_button_five_long_press_transitions_to_home()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}

void test_button_eight_is_disabled_in_play_mode()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_button_five_transitions_to_patch_mode()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patch),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(6, fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_uses_patch_return_value_for_next_patch_entry()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | 11);
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patch),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(11, fixture.transitionDelegate.lastTransitionValue);
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
    RUN_TEST(test_activate_recalls_selected_preset_before_scene_select);
    RUN_TEST(test_activate_recalls_selected_preset_zero);
    RUN_TEST(test_activate_skips_program_change_for_none_transition);
    RUN_TEST(test_activate_skips_program_change_for_patch_return_transition);
    RUN_TEST(test_activate_selects_first_scene);
    RUN_TEST(test_scene_a_selects_scene_zero);
    RUN_TEST(test_scene_b_selects_scene_one);
    RUN_TEST(test_scene_c_selects_scene_two);
    RUN_TEST(test_tuner_button_enables_tuner_without_changing_scene_selection);
    RUN_TEST(test_tuner_button_long_press_disables_tuner);
    RUN_TEST(test_button_press_changes_selection_to_single_button);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_long_press_is_noop);
    RUN_TEST(test_button_five_long_press_transitions_to_home);
    RUN_TEST(test_button_eight_is_disabled_in_play_mode);
    RUN_TEST(test_button_five_transitions_to_patch_mode);
    RUN_TEST(test_button_five_uses_patch_return_value_for_next_patch_entry);
    UNITY_END();
}

void loop() {}
