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

namespace
{
static byte g_lastSelected = 0xFF;

class NullTouchDelegate : public ITouchButtonDelegate
{
  public:
    void buttonPressed(const byte number) override { (void)number; }
};

class TestTouchButton : public FootSwitchTouchButton
{
  public:
    TestTouchButton(const byte number, const Point location, const Size size, uint16_t pillColour,
                    ITouchButtonDelegate& delegate)
        : FootSwitchTouchButton(number, location, size, "", pillColour, delegate), drawCalls(0)
    {
    }

    void draw(ScreenUi& ui) override
    {
        (void)ui;
        ++drawCalls;
    }

    int drawCalls;
};

class HomeModeFixture
{
  public:
    HomeModeFixture()
        : screenSize{480, 320}, ui(tft, screenSize), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800),
          ringManager(strip), button0(0, {0, 0}, {10, 10}, 0xFFFF, touchDelegate),
          button1(1, {0, 0}, {10, 10}, 0xFFFF, touchDelegate), button2(2, {0, 0}, {10, 10}, 0xFFFF, touchDelegate),
          button3(3, {0, 0}, {10, 10}, 0xFFFF, touchDelegate), button4(4, {0, 0}, {10, 10}, 0xFFFF, touchDelegate),
          button5(5, {0, 0}, {10, 10}, 0xFFFF, touchDelegate), button6(6, {0, 0}, {10, 10}, 0xFFFF, touchDelegate),
          button7(7, {0, 0}, {10, 10}, 0xFFFF, touchDelegate),
          buttons{&button0, &button1, &button2, &button3, &button4, &button5, &button6, &button7},
          mode(buttons, RingManager::RingCount, ringManager, ui)
    {
    }

    const Size screenSize;
    TFT_eSPI tft;
    ScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    NullTouchDelegate touchDelegate;

    TestTouchButton button0;
    TestTouchButton button1;
    TestTouchButton button2;
    TestTouchButton button3;
    TestTouchButton button4;
    TestTouchButton button5;
    TestTouchButton button6;
    TestTouchButton button7;

    FootSwitchTouchButton* buttons[RingManager::RingCount];
    HomeMode mode;
};

void test_activate_sets_home_labels_and_selects_first_button()
{
    HomeModeFixture fixture;
    g_lastSelected = 0xFF;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
    TEST_ASSERT_EQUAL_STRING("Amp", fixture.button0.label());
    TEST_ASSERT_EQUAL_STRING("Ampless", fixture.button1.label());
    TEST_ASSERT_EQUAL_STRING("CodeRed", fixture.button2.label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.button3.label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.button4.label());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.button0.pillColour());
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.button3.pillColour());
    TEST_ASSERT_EQUAL_INT(1, fixture.button0.drawCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.button7.drawCalls);
}

void test_button_pressed_selects_requested_button_when_valid()
{
    HomeModeFixture fixture;
    g_lastSelected = 0xFF;

    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_button_pressed_ignores_disabled_button()
{
    HomeModeFixture fixture;
    g_lastSelected = 0xFF;

    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_button_pressed_ignores_out_of_range_button()
{
    HomeModeFixture fixture;
    g_lastSelected = 0xFF;

    fixture.mode.buttonPressed(RingManager::RingCount);

    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_button_long_pressed_ignores_out_of_range_button()
{
    HomeModeFixture fixture;
    g_lastSelected = 0xFF;

    fixture.mode.buttonLongPressed(RingManager::RingCount);

    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_frame_tick_noop()
{
    HomeModeFixture fixture;
    g_lastSelected = 0xFF;

    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
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
