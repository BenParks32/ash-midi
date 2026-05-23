#include <unity.h>

#include "Modes/HomeMode.h"
#include "Modes/ModeManager.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/HomeMode.cpp"
#include "../../../src/Modes/ModeManager.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
constexpr ModeTransitionValue HomePlaylistTransitionFlag = 0x0200;
constexpr byte Project7PlaylistIndex = 2;
constexpr byte OprPlaylistIndex = 3;
constexpr byte CodeRedPlaylistIndex = 4;

constexpr ModeTransitionValue hostHomePlaylistTransitionValue(byte playlistIndex)
{
    return static_cast<ModeTransitionValue>(HomePlaylistTransitionFlag | playlistIndex);
}

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
    Modes lastMode = Modes::Home;
    ModeTransitionValue lastTransitionValue = 0;
};

class HomeModeFixture
{
  public:
    HomeModeFixture()
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
    HomeMode mode;
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

void test_activate_sets_home_labels_and_ring_matched_visuals()
{
    HomeModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Project7", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("OPR", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("CodeRed", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Play", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("Patch", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Menu", fixture.touchButtonManager.getButton(7)->label());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(4)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(5)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(7)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT32(0, fixture.strip.getPixelColor(RingManager::LedsPerRing * 0));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.strip.getPixelColor(RingManager::LedsPerRing * 6));
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.drawLogoCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.redrawSdStatusCalls);
}

void test_button_pressed_selects_opr_home_playlist_for_play_mode()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(hostHomePlaylistTransitionValue(OprPlaylistIndex), fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
}

void test_button_pressed_selects_project7_home_playlist_for_play_mode()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(hostHomePlaylistTransitionValue(Project7PlaylistIndex),
                             fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
}

void test_button_pressed_selects_code_red_home_playlist_for_play_mode()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(hostHomePlaylistTransitionValue(CodeRedPlaylistIndex),
                             fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
}

void test_button_eight_transitions_to_menu_mode()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Menu),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_transitions_to_play_mode_with_patch_zero()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(0, fixture.transitionDelegate.lastTransitionValue);
}

void test_button_six_transitions_to_patch_mode_with_no_transition_override()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patch),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}

void test_button_pressed_ignores_disabled_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(6);

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

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.programChangeCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
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

void test_deactivate_hides_status_and_draws_logo_in_black()
{
    HomeModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.deactivate();

    TEST_ASSERT_EQUAL_INT(1, fixture.ui.hideSdStatusCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.drawLogoWithColoursCalls);
}

void test_button_press_transitions_via_mode_manager_to_play_slot()
{
    FakeScreenUi ui;
    Adafruit_NeoPixel strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    TouchButtonManager touchButtonManager(ui);
    MockMidiManager midiManager;

    TestModeSlot homeSlot;
    TestModeSlot playSlot;

    IMode* activeMode = &homeSlot;
    IMode* menuSlot = nullptr;
    IMode* patchSlot = nullptr;
    IMode* buttonDiagnosticSlot = nullptr;
    IMode* modes[ModeCount] = {&homeSlot, &playSlot, patchSlot, menuSlot, buttonDiagnosticSlot};
    ModeManager modeManager(activeMode, modes);
    HomeMode mode(touchButtonManager, ringManager, ui, midiManager, modeManager);

    mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_PTR(&playSlot, activeMode);
    TEST_ASSERT_EQUAL_INT(1, playSlot.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT16(hostHomePlaylistTransitionValue(CodeRedPlaylistIndex), playSlot.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, playSlot.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, midiManager.programChangeCalls);
}

void test_button_six_transitions_via_mode_manager_to_patch_slot()
{
    FakeScreenUi ui;
    Adafruit_NeoPixel strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    TouchButtonManager touchButtonManager(ui);
    MockMidiManager midiManager;

    TestModeSlot homeSlot;
    TestModeSlot playSlot;
    TestModeSlot patchSlot;

    IMode* activeMode = &homeSlot;
    IMode* menuSlot = nullptr;
    IMode* buttonDiagnosticSlot = nullptr;
    IMode* modes[ModeCount] = {&homeSlot, &playSlot, &patchSlot, menuSlot, buttonDiagnosticSlot};
    ModeManager modeManager(activeMode, modes);
    HomeMode mode(touchButtonManager, ringManager, ui, midiManager, modeManager);

    mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_PTR(&patchSlot, activeMode);
    TEST_ASSERT_EQUAL_INT(1, patchSlot.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, patchSlot.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, patchSlot.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, midiManager.programChangeCalls);
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
    RUN_TEST(test_activate_sets_home_labels_and_ring_matched_visuals);
    RUN_TEST(test_button_pressed_selects_opr_home_playlist_for_play_mode);
    RUN_TEST(test_button_pressed_selects_project7_home_playlist_for_play_mode);
    RUN_TEST(test_button_pressed_selects_code_red_home_playlist_for_play_mode);
    RUN_TEST(test_button_eight_transitions_to_menu_mode);
    RUN_TEST(test_button_five_transitions_to_play_mode_with_patch_zero);
    RUN_TEST(test_button_six_transitions_to_patch_mode_with_no_transition_override);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_button_pressed_ignores_out_of_range_button);
    RUN_TEST(test_button_long_pressed_ignores_out_of_range_button);
    RUN_TEST(test_button_long_pressed_on_enabled_home_button_is_noop);
    RUN_TEST(test_frame_tick_noop);
    RUN_TEST(test_deactivate_hides_status_and_draws_logo_in_black);
    RUN_TEST(test_button_press_transitions_via_mode_manager_to_play_slot);
    RUN_TEST(test_button_six_transitions_via_mode_manager_to_patch_slot);
    return UNITY_END();
}
