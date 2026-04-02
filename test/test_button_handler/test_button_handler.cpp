#include <Arduino.h>
#include <unity.h>

#include "ButtonHandler.h"
#include "Modes/Mode.h"
#include "RingManager.h"
#include "TouchButton.h"

// Pull in implementation units required by ButtonHandler without linking app main.cpp.
#include "../../src/Button.cpp"
#include "../../src/ButtonHandler.cpp"
#include "../../src/Lights.cpp"
#include "../../src/RingManager.cpp"

namespace
{
static byte g_lastSelected = 0xFF;

class ModeSpy : public IMode
{
  public:
    void activate() override {}

    void buttonPressed(const byte number) override
    {
        lastPressed = number;
        ++pressedCalls;
    }

    void buttonLongPressed(const byte number) override
    {
        lastLongPressed = number;
        ++longPressedCalls;
    }

    void frameTick() override {}

    byte lastPressed = 0xFF;
    int pressedCalls = 0;
    byte lastLongPressed = 0xFF;
    int longPressedCalls = 0;
};

class ButtonHandlerFixture
{
  public:
    ButtonHandlerFixture()
        : strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), activeMode(nullptr),
          handler(activeMode, ringManager)
    {
    }

    Adafruit_NeoPixel strip;
    RingManager ringManager;
    IMode* activeMode;
    ButtonHandler handler;
    ModeSpy modeSpy;
};

void test_button_pressed_routes_to_active_mode()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = &fixture.modeSpy;
    g_lastSelected = 0xFF;

    fixture.handler.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.pressedCalls);
    TEST_ASSERT_EQUAL_UINT8(3, fixture.modeSpy.lastPressed);
    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_button_long_pressed_routes_to_active_mode()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = &fixture.modeSpy;

    fixture.handler.buttonLongPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.longPressedCalls);
    TEST_ASSERT_EQUAL_UINT8(5, fixture.modeSpy.lastLongPressed);
}

void test_button_pressed_does_nothing_when_no_active_mode()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = nullptr;
    g_lastSelected = 0xFF;

    fixture.handler.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(0, fixture.modeSpy.pressedCalls);
    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_button_long_pressed_no_mode_does_not_select_touch_button()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = nullptr;
    g_lastSelected = 0xFF;

    fixture.handler.buttonLongPressed(2);

    TEST_ASSERT_EQUAL_INT(0, fixture.modeSpy.longPressedCalls);
    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);
}

void test_button_handler_reads_active_mode_pointer_by_reference()
{
    ButtonHandlerFixture fixture;
    g_lastSelected = 0xFF;

    fixture.handler.buttonPressed(1);
    TEST_ASSERT_EQUAL_UINT8(0xFF, g_lastSelected);

    fixture.activeMode = &fixture.modeSpy;
    fixture.handler.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.pressedCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.modeSpy.lastPressed);
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
    RUN_TEST(test_button_pressed_routes_to_active_mode);
    RUN_TEST(test_button_long_pressed_routes_to_active_mode);
    RUN_TEST(test_button_pressed_does_nothing_when_no_active_mode);
    RUN_TEST(test_button_long_pressed_no_mode_does_not_select_touch_button);
    RUN_TEST(test_button_handler_reads_active_mode_pointer_by_reference);
    UNITY_END();
}

void loop() {}
