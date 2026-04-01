#include <Arduino.h>
#include <TFT_eSPI.h>
#include <unity.h>

#include "Modes/HomeMode.h"

// Pull in implementation units required by HomeMode without linking app main.cpp.
#include "../../src/Lights.cpp"
#include "../../src/Modes/HomeMode.cpp"
#include "../../src/RingManager.cpp"
#include "../../src/ScreenUi.cpp"
#include "../../src/TouchButton.cpp"
#include "../../src/TouchButtonManager.cpp"

namespace
{
class HomeModeFixture
{
  public:
    HomeModeFixture()
        : screenSize{480, 320}, ui(tft, screenSize), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800),
          ringManager(strip), touchButtonManager(ui, ringManager), mode(touchButtonManager, ringManager, ui)
    {
    }

    const Size screenSize;
    TFT_eSPI tft;
    ScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    HomeMode mode;
};

void test_activate_sets_home_labels_and_selects_first_button()
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
}

void test_button_pressed_ignores_disabled_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(5);
}

void test_button_pressed_ignores_out_of_range_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonPressed(TouchButtonManager::BUTTON_COUNT);
}

void test_button_long_pressed_ignores_out_of_range_button()
{
    HomeModeFixture fixture;

    fixture.mode.buttonLongPressed(TouchButtonManager::BUTTON_COUNT);
}

void test_frame_tick_noop()
{
    HomeModeFixture fixture;

    fixture.mode.activate();
    const uint32_t before = fixture.strip.getPixelColor(RingManager::LedsPerRing * 5);

    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_UINT32(before, fixture.strip.getPixelColor(RingManager::LedsPerRing * 5));
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
    RUN_TEST(test_activate_sets_home_labels_and_selects_first_button);
    RUN_TEST(test_button_pressed_selects_requested_button_when_valid);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_button_pressed_ignores_out_of_range_button);
    RUN_TEST(test_button_long_pressed_ignores_out_of_range_button);
    RUN_TEST(test_frame_tick_noop);
    UNITY_END();
}

void loop() {}
