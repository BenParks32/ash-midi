#include <unity.h>

#define private public
#include "Modes/MenuMode.h"
#undef private

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/MenuMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
class FakeScreenUi : public IScreenUi
{
  public:
    enum class SdStatusState : uint8_t
    {
        None,
        Initializing,
        Failed,
        Ready,
        NotMounted,
    };

    void drawBackgroundAndBorder() override { ++drawBackgroundAndBorderCalls; }
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t) override { ++drawCenteredFrameCalls; }

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

    void setSdStatusInitializing() override
    {
        ++setSdStatusInitializingCalls;
        sdStatusState = SdStatusState::Initializing;
    }

    void setSdStatusFailed() override
    {
        ++setSdStatusFailedCalls;
        sdStatusState = SdStatusState::Failed;
    }

    void setSdStatusReady() override
    {
        ++setSdStatusReadyCalls;
        sdStatusState = SdStatusState::Ready;
    }

    void setSdStatusNotMounted() override
    {
        ++setSdStatusNotMountedCalls;
        sdStatusState = SdStatusState::NotMounted;
    }

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
    SdStatusState sdStatusState = SdStatusState::None;
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
    Modes lastMode = Modes::Menu;
    ModeTransitionValue lastTransitionValue = ModeTransitionNone;
};

class MockSettingsStore : public ISettingsStore
{
  public:
    bool load(AppSettings& outSettings) override
    {
        ++loadCalls;
        outSettings = stored;
        return true;
    }

    bool save(const AppSettings& settings) override
    {
        ++saveCalls;
        stored = settings;
        return true;
    }

    int loadCalls = 0;
    int saveCalls = 0;
    AppSettings stored = {RingManager::DefaultBrightness, 1, {{254, 3649, 281, 3563, 7}}};
};

class MockSdCardManager : public ISdCardManager
{
  public:
    bool mount() override
    {
        ++mountCalls;
        mounted = true;
        return true;
    }

    bool unmount() override
    {
        ++unmountCalls;
        mounted = false;
        return true;
    }

    bool isMounted() const override { return mounted; }

    int mountCalls = 0;
    int unmountCalls = 0;
    bool mounted = true;
};

class MockTouchCalibrator : public ITouchCalibrator
{
  public:
    bool calibrate(TouchCalibrationData& outCalibration) override
    {
        ++calibrateCalls;
        if (!shouldSucceed)
        {
            return false;
        }

        outCalibration = calibrationToReturn;
        return true;
    }

    void apply(const TouchCalibrationData& calibration) override
    {
        ++applyCalls;
        lastApplied = calibration;
    }

    int calibrateCalls = 0;
    int applyCalls = 0;
    bool shouldSucceed = true;
    TouchCalibrationData calibrationToReturn = {{101, 202, 303, 404, 5}};
    TouchCalibrationData lastApplied = {{0, 0, 0, 0, 0}};
};

class MenuModeFixture
{
  public:
    MenuModeFixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), transitionDelegate(), settingsStore(), sdCardManager(), touchCalibrator(),
          settings{120, 3, {{254, 3649, 281, 3563, 7}}},
          mode(touchButtonManager, ringManager, ui, midiManager, transitionDelegate, settingsStore, sdCardManager,
               touchCalibrator, settings)
    {
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockTransitionDelegate transitionDelegate;
    MockSettingsStore settingsStore;
    MockSdCardManager sdCardManager;
    MockTouchCalibrator touchCalibrator;
    AppSettings settings;
    MenuMode mode;
};

void test_activate_disables_all_footswitch_labels()
{
    MenuModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Home", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(7)->label());
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.hideSdStatusCalls);
}

void test_button_five_exits_menu_to_home()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}

void test_button_eight_is_noop_in_menu_mode()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_encoder_rotate_in_navigation_mode_does_not_change_values()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderRotated(1);

    TEST_ASSERT_EQUAL_UINT8(120, fixture.settings.masterBrightness);
    TEST_ASSERT_EQUAL_UINT8(3, fixture.settings.midiChannel);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.midiManager.channel());
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);
}

void test_edit_brightness_applies_immediately_and_saves_on_confirm()
{
    MenuModeFixture fixture;

    fixture.mode.activate();

    fixture.mode.encoderPressed();
    fixture.mode.encoderRotated(2);

    TEST_ASSERT_EQUAL_UINT8(136, fixture.settings.masterBrightness);
    TEST_ASSERT_EQUAL_UINT8(136, fixture.ringManager.masterBrightness());
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);

    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.settingsStore.saveCalls);
    TEST_ASSERT_EQUAL_UINT8(136, fixture.settingsStore.stored.masterBrightness);
}

void test_menu_layout_is_inset_within_center_space()
{
    MenuModeFixture fixture;

    const MenuLayout layout = buildMenuLayout(fixture.ui);
    const int32_t rowWidth = layout.leftPanelWidth - (RowHorizontalPadding * 2);
    const int32_t rightMargin = layout.screenWidth - (layout.rightPanelX + layout.rightPanelWidth);

    TEST_ASSERT_EQUAL_INT(MenuHorizontalInset, layout.leftPanelX);
    TEST_ASSERT_EQUAL_INT(MenuHorizontalInset, rightMargin);
    TEST_ASSERT_TRUE(rowWidth <= 224);
    TEST_ASSERT_TRUE(layout.valuePanelX > layout.rightPanelX);
    TEST_ASSERT_TRUE((layout.valuePanelX + layout.valuePanelWidth) <
                     (layout.rightPanelX + layout.rightPanelWidth));
}

void test_menu_row_gap_is_reduced_by_seven_pixels()
{
    MenuModeFixture fixture;

    const MenuLayout layout = buildMenuLayout(fixture.ui);
    const int32_t baselineGap =
        (layout.centerHeight - (static_cast<int32_t>(MenuMode::MenuItem::Count) * RowHeight)) /
        (static_cast<int32_t>(MenuMode::MenuItem::Count) - 1);
    const int32_t expectedGap = (baselineGap > 6) ? (baselineGap - 7) : 0;

    TEST_ASSERT_EQUAL_INT(expectedGap, menuRowGapForLayout(layout, static_cast<uint8_t>(MenuMode::MenuItem::Count)));
}

void test_first_menu_item_is_offset_two_pixels_lower()
{
    MenuModeFixture fixture;

    TEST_ASSERT_EQUAL_INT(fixture.mode.menuListStartY() + 2,
                          fixture.mode.menuRowY(static_cast<MenuMode::MenuItem>(0)));
}

void test_random_ring_colours_are_assigned_on_activate_and_stable_during_midi_edit()
{
    MenuModeFixture fixture;

    randomSeed(1234);
    fixture.mode.activate();

    const uint8_t menuExitRingIndex = 4;
    const uint8_t scaledBrightness = static_cast<uint8_t>(
        (static_cast<uint16_t>(RingManager::FullBrightness) * fixture.ringManager.masterBrightness()) / 255U);
    const uint8_t channel = static_cast<uint8_t>(((static_cast<uint16_t>(255U) * scaledBrightness) + 127U) / 255U);
    const uint32_t white = (static_cast<uint32_t>(channel) << 16) | (static_cast<uint32_t>(channel) << 8) | channel;
    TEST_ASSERT_EQUAL_UINT32(white, fixture.strip.getPixelColor(menuExitRingIndex * RingManager::LedsPerRing));

    uint32_t coloursBefore[RingManager::RingCount] = {0};
    bool anyNonZero = false;
    for (uint8_t ringIndex = 0; ringIndex < RingManager::RingCount; ++ringIndex)
    {
        coloursBefore[ringIndex] = fixture.strip.getPixelColor(ringIndex * RingManager::LedsPerRing);
        if (coloursBefore[ringIndex] != 0)
        {
            anyNonZero = true;
        }
    }
    TEST_ASSERT_TRUE(anyNonZero);

    fixture.mode.encoderRotated(1);
    fixture.mode.encoderPressed();
    fixture.mode.encoderRotated(1);

    for (uint8_t ringIndex = 0; ringIndex < RingManager::RingCount; ++ringIndex)
    {
        const uint32_t colourAfter = fixture.strip.getPixelColor(ringIndex * RingManager::LedsPerRing);
        TEST_ASSERT_EQUAL_UINT32(coloursBefore[ringIndex], colourAfter);
    }
}

void test_edit_midi_channel_applies_immediately_and_clamps()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderRotated(1);
    fixture.mode.encoderPressed();
    fixture.mode.encoderRotated(100);

    TEST_ASSERT_EQUAL_UINT8(16, fixture.settings.midiChannel);
    TEST_ASSERT_EQUAL_UINT8(16, fixture.midiManager.channel());

    fixture.mode.encoderRotated(-100);

    TEST_ASSERT_EQUAL_UINT8(1, fixture.settings.midiChannel);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.midiManager.channel());
}

void test_edit_mode_toggle_saves_only_when_leaving_edit_mode()
{
    MenuModeFixture fixture;

    fixture.mode.activate();

    fixture.mode.encoderPressed();
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);

    fixture.mode.encoderPressed();
    TEST_ASSERT_EQUAL_INT(1, fixture.settingsStore.saveCalls);
}

void test_sd_card_item_toggles_unmount_and_mount()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderRotated(2);
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.sdCardManager.unmountCalls);
    TEST_ASSERT_FALSE(fixture.sdCardManager.mounted);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(FakeScreenUi::SdStatusState::NotMounted),
                            static_cast<uint8_t>(fixture.ui.sdStatusState));

    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.sdCardManager.mountCalls);
    TEST_ASSERT_TRUE(fixture.sdCardManager.mounted);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(FakeScreenUi::SdStatusState::Ready),
                            static_cast<uint8_t>(fixture.ui.sdStatusState));
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);
}

void test_touch_calibration_item_runs_calibrator_and_saves_result()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderRotated(3);
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.touchCalibrator.calibrateCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.touchCalibrator.applyCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.settingsStore.saveCalls);
    TEST_ASSERT_EQUAL_UINT16(101, fixture.settings.touchCalibration.values[0]);
    TEST_ASSERT_EQUAL_UINT16(404, fixture.settingsStore.stored.touchCalibration.values[3]);
    TEST_ASSERT_EQUAL_INT(1, fixture.ui.drawBackgroundAndBorderCalls);
}

void test_touch_calibration_failure_does_not_apply_or_save()
{
    MenuModeFixture fixture;
    fixture.touchCalibrator.shouldSucceed = false;

    fixture.mode.activate();
    fixture.mode.encoderRotated(3);
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.touchCalibrator.calibrateCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.touchCalibrator.applyCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);
    TEST_ASSERT_EQUAL_UINT16(254, fixture.settings.touchCalibration.values[0]);
}

void test_button_diagnostics_item_enters_button_diagnostic_mode()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderRotated(4);
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::ButtonDiagnostic),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);
}
} // namespace

void setUp()
{
    HostArduino::resetTime();
    HostArduino::resetPins();
    HostArduino::resetRandom();
}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_activate_disables_all_footswitch_labels);
    RUN_TEST(test_button_five_exits_menu_to_home);
    RUN_TEST(test_button_eight_is_noop_in_menu_mode);
    RUN_TEST(test_encoder_rotate_in_navigation_mode_does_not_change_values);
    RUN_TEST(test_edit_brightness_applies_immediately_and_saves_on_confirm);
    RUN_TEST(test_menu_layout_is_inset_within_center_space);
    RUN_TEST(test_menu_row_gap_is_reduced_by_seven_pixels);
    RUN_TEST(test_first_menu_item_is_offset_two_pixels_lower);
    RUN_TEST(test_random_ring_colours_are_assigned_on_activate_and_stable_during_midi_edit);
    RUN_TEST(test_edit_midi_channel_applies_immediately_and_clamps);
    RUN_TEST(test_edit_mode_toggle_saves_only_when_leaving_edit_mode);
    RUN_TEST(test_sd_card_item_toggles_unmount_and_mount);
    RUN_TEST(test_touch_calibration_item_runs_calibrator_and_saves_result);
    RUN_TEST(test_touch_calibration_failure_does_not_apply_or_save);
    RUN_TEST(test_button_diagnostics_item_enters_button_diagnostic_mode);
    return UNITY_END();
}
