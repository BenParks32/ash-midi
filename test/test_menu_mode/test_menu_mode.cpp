#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "Modes/MenuMode.h"

// Pull in implementation units required by MenuMode without linking app main.cpp.
#include "../../src/Function.cpp"
#include "../../src/Lights.cpp"
#include "../../src/Modes/FunctionModeBase.cpp"
#include "../../src/Modes/MenuMode.cpp"
#include "../../src/RingManager.cpp"
#include "../../src/ScreenUi.cpp"
#include "../../src/TouchButton.cpp"
#include "../../src/TouchButtonManager.cpp"

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
    Modes lastMode = Modes::Menu;
    byte lastTransitionValue = 0xFF;
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
    AppSettings stored = {RingManager::DefaultBrightness, 1};
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

class MenuModeFixture
{
  public:
    MenuModeFixture()
        : screenSize{480, 320}, ui(tft, screenSize), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800),
          ringManager(strip), touchButtonManager(ui), midiManager(), transitionDelegate(), settingsStore(),
          sdCardManager(), settings{120, 3}, mode(touchButtonManager, ringManager, ui, midiManager, transitionDelegate,
                                                  settingsStore, sdCardManager, settings)
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
    MockSettingsStore settingsStore;
    MockSdCardManager sdCardManager;
    AppSettings settings;
    MenuMode mode;
};

void test_activate_disables_all_footswitch_labels()
{
    MenuModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Home", fixture.touchButtonManager.getButton(7)->label());
}

void test_button_eight_exits_menu_to_home()
{
    MenuModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT8(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
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

void test_random_ring_colours_are_assigned_on_activate_and_stable_during_midi_edit()
{
    MenuModeFixture fixture;

    randomSeed(1234);
    fixture.mode.activate();

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

    // Select MIDI Channel item, enter edit mode, then change value.
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

    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.sdCardManager.mountCalls);
    TEST_ASSERT_TRUE(fixture.sdCardManager.mounted);
    TEST_ASSERT_EQUAL_INT(0, fixture.settingsStore.saveCalls);
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
    RUN_TEST(test_activate_disables_all_footswitch_labels);
    RUN_TEST(test_button_eight_exits_menu_to_home);
    RUN_TEST(test_encoder_rotate_in_navigation_mode_does_not_change_values);
    RUN_TEST(test_edit_brightness_applies_immediately_and_saves_on_confirm);
    RUN_TEST(test_random_ring_colours_are_assigned_on_activate_and_stable_during_midi_edit);
    RUN_TEST(test_edit_midi_channel_applies_immediately_and_clamps);
    RUN_TEST(test_edit_mode_toggle_saves_only_when_leaving_edit_mode);
    RUN_TEST(test_sd_card_item_toggles_unmount_and_mount);
    UNITY_END();
}

void loop() {}
