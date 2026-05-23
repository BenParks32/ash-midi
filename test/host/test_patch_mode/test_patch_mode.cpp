#include <unity.h>

#include "MidiProvider.h"
#include "Modes/ModeManager.h"
#include "Modes/PatchMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/ModeManager.cpp"
#include "../../../src/Modes/PatchMode.cpp"
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
    byte defaultPlaylistIndex() const override { return 1; }
    void selectPlaylist(byte playlistIndex) override { lastPlaylistIndex = playlistIndex; }

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

    void setGigViewEnabled(bool enabled) override
    {
        ++setGigViewCalls;
        lastGigViewEnabled = enabled;
    }

    int recallPresetCalls = 0;
    int selectSceneCalls = 0;
    int setTunerCalls = 0;
    int setGigViewCalls = 0;
    byte lastRecallPreset = 0;
    byte lastPlaylistIndex = 1;
    byte lastSceneIndex = 0;
    bool lastTunerEnabled = false;
    bool lastGigViewEnabled = false;
};

class PatchModeFixture
{
  public:
    PatchModeFixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), midiProvider(), transitionDelegate(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, transitionDelegate)
    {
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockMidiProvider midiProvider;
    MockTransitionDelegate transitionDelegate;
    PatchMode mode;
};

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
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.drawCenteredFrameCalls);
    TEST_ASSERT_EQUAL_INT(2, fixture.ui.drawCenteredTextCalls);
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
    FakeScreenUi ui;
    Adafruit_NeoPixel strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    TouchButtonManager touchButtonManager(ui);
    MockMidiManager midiManager;

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
    return UNITY_END();
}
