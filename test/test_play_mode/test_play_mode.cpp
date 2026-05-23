#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "ButtonOverrideStore.h"
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
constexpr ModeTransitionValue HomePlaylistTransitionFlag = 0x0200;
constexpr byte DefaultPlaylistIndex = 1;
constexpr byte V40PlaylistIndex = 2;
constexpr byte AmplessPlaylistIndex = 3;
constexpr byte CodeRedPlaylistIndex = 4;

constexpr ModeTransitionValue homePlaylistTransitionValue(byte playlistIndex)
{
    return static_cast<ModeTransitionValue>(HomePlaylistTransitionFlag | playlistIndex);
}

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

class MockButtonOverrideStore : public IButtonOverrideStore
{
  public:
    bool refresh() override
    {
        ++refreshCalls;
        return refreshResult;
    }

    void applyOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount) const override
    {
        ++applyCalls;
        lastPlaylistIndex = playlistIndex;
        lastPatchNumber = patchNumber;

        if (!enableMomentaryOverride || functions == nullptr || functionCount <= 5)
        {
            return;
        }

        functions[5] = Function("Hold", 0xF81F, FunctionAction(ActionType::SendMidiControlChange, 21, 127),
                                FunctionAction(ActionType::SendMidiControlChange, 21, 0),
                                FunctionAction(ActionType::SelectScene, 2, 0),
                                FunctionAction(ActionType::ChangeMode, static_cast<byte>(Modes::Home), 0));
    }

    mutable int applyCalls = 0;
    mutable byte lastPlaylistIndex = 0;
    mutable byte lastPatchNumber = 0;
    int refreshCalls = 0;
    bool refreshResult = true;
    bool enableMomentaryOverride = false;
};

class MockMidiProvider : public IMidiProvider
{
  public:
    enum class CallType : uint8_t
    {
        SelectPlaylist,
        RecallPreset,
        SelectScene,
        SetTuner,
        SetGigView,
    };

    byte maxPresetIndex() const override { return 127; }
    byte defaultPlaylistIndex() const override { return DefaultPlaylistIndex; }

    void selectPlaylist(byte playlistIndex) override
    {
        ++selectPlaylistCalls;
        lastPlaylistIndex = playlistIndex;
        callOrder[callCount++] = CallType::SelectPlaylist;
    }

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

    void setGigViewEnabled(bool enabled) override
    {
        ++setGigViewCalls;
        lastGigViewEnabled = enabled;
        callOrder[callCount++] = CallType::SetGigView;
    }

    int selectPlaylistCalls = 0;
    int recallPresetCalls = 0;
    int selectSceneCalls = 0;
    int setTunerCalls = 0;
    int setGigViewCalls = 0;
    byte lastPlaylistIndex = DefaultPlaylistIndex;
    byte lastRecallPreset = 0;
    byte lastSceneIndex = 0;
    bool lastTunerEnabled = false;
    bool lastGigViewEnabled = false;
    CallType callOrder[8] = {};
    int callCount = 0;
};

class PlayModeFixture
{
  public:
    PlayModeFixture()
        : screenSize{480, 320}, ui(tft, screenSize), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800),
          ringManager(strip), touchButtonManager(ui), midiManager(), midiProvider(), transitionDelegate(),
          overrideStore(), mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, overrideStore,
                                transitionDelegate)
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
    MockButtonOverrideStore overrideStore;
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
    TEST_ASSERT_EQUAL_STRING("Patch", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("Gig", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING("Tap", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Tuner", fixture.touchButtonManager.getButton(7)->label());
}

void test_activate_refreshes_button_overrides_for_selected_playlist_and_patch()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(homePlaylistTransitionValue(CodeRedPlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.overrideStore.refreshCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.overrideStore.applyCalls);
    TEST_ASSERT_EQUAL_UINT8(CodeRedPlaylistIndex, fixture.overrideStore.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.overrideStore.lastPatchNumber);
}

void test_activate_selects_single_button_and_dims_others()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(2)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(4)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(5)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(1)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(2)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(4)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(5)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(7)->pillColour());
}

void test_activate_recalls_selected_preset_before_scene_select()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
    TEST_ASSERT_EQUAL_INT(3, fixture.midiProvider.callCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::SelectPlaylist),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[0]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::RecallPreset),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[1]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::SelectScene),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[2]));
}

void test_momentary_override_uses_button_down_and_release_and_suppresses_short_and_long_press()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableMomentaryOverride = true;

    fixture.mode.setSelectedPreset(9);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Hold", fixture.touchButtonManager.getButton(5)->label());

    const int baselineControlChanges = fixture.midiManager.controlChangeCalls;
    fixture.mode.buttonDown(5);
    TEST_ASSERT_EQUAL_INT(baselineControlChanges + 1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(21, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(127, fixture.midiManager.lastControlChangeValue);

    fixture.mode.buttonPressed(5);
    fixture.mode.buttonLongPressed(5);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(baselineControlChanges + 1, fixture.midiManager.controlChangeCalls);

    fixture.mode.buttonReleased(5);
    TEST_ASSERT_EQUAL_INT(baselineControlChanges + 2, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(21, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiManager.lastControlChangeValue);
}

void test_activate_recalls_selected_preset_zero()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(0);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
}

void test_activate_recalls_v40_playlist_preset_1a()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(homePlaylistTransitionValue(V40PlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(V40PlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_recalls_ampless_playlist_preset_1a()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(homePlaylistTransitionValue(AmplessPlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(AmplessPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_recalls_code_red_playlist_preset_1a()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(homePlaylistTransitionValue(CodeRedPlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(CodeRedPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_skips_program_change_for_none_transition()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionNone);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_skips_program_change_for_patch_return_transition()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | 9);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_selects_first_scene()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
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

void test_gig_view_button_opens_gig_view_without_changing_scene_selection()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setGigViewCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastGigViewEnabled);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(5)->hasBorder());
}

void test_gig_view_button_long_press_closes_gig_view()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setGigViewCalls);
    TEST_ASSERT_FALSE(fixture.midiProvider.lastGigViewEnabled);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_button_does_not_send_midi_on_first_press_without_changing_scene_selection()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
}

void test_tap_tempo_button_does_not_send_midi_on_second_press()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_button_sends_cc44_value100_on_scheduled_interval_after_third_press()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(49);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(1);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(44, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(100, fixture.midiManager.lastControlChangeValue);
}

void test_tap_tempo_continued_tapping_does_not_delay_next_scheduled_midi_send()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(45);
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(5);
    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(44, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(100, fixture.midiManager.lastControlChangeValue);
}

void test_tap_tempo_light_flashes_using_recent_tap_average()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());

    delay(35);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());

    delay(35);
    fixture.mode.frameTick();
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());
}

void test_tap_tempo_light_returns_to_solid_blue_after_ten_seconds()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    delay(35);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());

    delay(10020);
    fixture.mode.frameTick();
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());

    delay(35);
    fixture.mode.frameTick();
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());
}

void test_tap_tempo_press_restarts_ten_second_flash_window()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    delay(9990);
    fixture.mode.buttonPressed(6);
    delay(35);
    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());
}

void test_tap_tempo_pre_arm_taps_reset_after_three_seconds_of_silence()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);

    delay(3100);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    delay(50);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    fixture.mode.buttonPressed(6);
    delay(49);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(1);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_inactivity_sends_three_trailing_messages_then_stops()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(1000);
    fixture.mode.buttonPressed(6);
    delay(1000);
    fixture.mode.buttonPressed(6);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(2, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(3, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(4, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(5, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(5, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_deactivate_clears_pending_scheduler()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);
    delay(50);
    fixture.mode.buttonPressed(6);

    fixture.mode.deactivate();
    delay(60);
    fixture.mode.activate();
    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_button_long_press_is_noop()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_tuner_button_enables_tuner_without_changing_scene_selection()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastTunerEnabled);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
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
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
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
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(5)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());
}

void test_button_pressed_ignores_disabled_button()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

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

void test_tap_tempo_button_does_not_change_mode()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
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

void test_button_five_uses_home_playlist_preset_zero_for_next_patch_entry()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(homePlaylistTransitionValue(CodeRedPlaylistIndex));
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patch),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(0, fixture.transitionDelegate.lastTransitionValue);
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
    RUN_TEST(test_activate_refreshes_button_overrides_for_selected_playlist_and_patch);
    RUN_TEST(test_activate_selects_single_button_and_dims_others);
    RUN_TEST(test_activate_recalls_selected_preset_before_scene_select);
    RUN_TEST(test_momentary_override_uses_button_down_and_release_and_suppresses_short_and_long_press);
    RUN_TEST(test_activate_recalls_selected_preset_zero);
    RUN_TEST(test_activate_recalls_v40_playlist_preset_1a);
    RUN_TEST(test_activate_recalls_ampless_playlist_preset_1a);
    RUN_TEST(test_activate_recalls_code_red_playlist_preset_1a);
    RUN_TEST(test_activate_skips_program_change_for_none_transition);
    RUN_TEST(test_activate_skips_program_change_for_patch_return_transition);
    RUN_TEST(test_activate_selects_first_scene);
    RUN_TEST(test_scene_a_selects_scene_zero);
    RUN_TEST(test_scene_b_selects_scene_one);
    RUN_TEST(test_scene_c_selects_scene_two);
    RUN_TEST(test_gig_view_button_opens_gig_view_without_changing_scene_selection);
    RUN_TEST(test_gig_view_button_long_press_closes_gig_view);
    RUN_TEST(test_tap_tempo_button_does_not_send_midi_on_first_press_without_changing_scene_selection);
    RUN_TEST(test_tap_tempo_button_does_not_send_midi_on_second_press);
    RUN_TEST(test_tap_tempo_button_sends_cc44_value100_on_scheduled_interval_after_third_press);
    RUN_TEST(test_tap_tempo_continued_tapping_does_not_delay_next_scheduled_midi_send);
    RUN_TEST(test_tap_tempo_light_flashes_using_recent_tap_average);
    RUN_TEST(test_tap_tempo_light_returns_to_solid_blue_after_ten_seconds);
    RUN_TEST(test_tap_tempo_press_restarts_ten_second_flash_window);
    RUN_TEST(test_tap_tempo_pre_arm_taps_reset_after_three_seconds_of_silence);
    RUN_TEST(test_tap_tempo_inactivity_sends_three_trailing_messages_then_stops);
    RUN_TEST(test_tap_tempo_deactivate_clears_pending_scheduler);
    RUN_TEST(test_tap_tempo_button_long_press_is_noop);
    RUN_TEST(test_tuner_button_enables_tuner_without_changing_scene_selection);
    RUN_TEST(test_tuner_button_long_press_disables_tuner);
    RUN_TEST(test_button_press_changes_selection_to_single_button);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_long_press_is_noop);
    RUN_TEST(test_button_five_long_press_transitions_to_home);
    RUN_TEST(test_tap_tempo_button_does_not_change_mode);
    RUN_TEST(test_button_five_transitions_to_patch_mode);
    RUN_TEST(test_button_five_uses_patch_return_value_for_next_patch_entry);
    RUN_TEST(test_button_five_uses_home_playlist_preset_zero_for_next_patch_entry);
    UNITY_END();
}

void loop() {}
