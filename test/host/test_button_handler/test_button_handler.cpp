#include <unity.h>

#include "ButtonHandler.h"
#include "Modes/Mode.h"
#include "RingManager.h"

#include "../../../src/Button.cpp"
#include "../../../src/ButtonHandler.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/RingManager.cpp"

namespace
{
class ModeSpy : public IMode
{
  public:
    void activate() override {}

    void buttonDown(const byte number) override
    {
        lastDown = number;
        ++downCalls;
    }

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

    void buttonReleased(const byte number) override
    {
        lastReleased = number;
        ++releasedCalls;
    }

    byte lastDown = 0xFF;
    int downCalls = 0;
    byte lastPressed = 0xFF;
    int pressedCalls = 0;
    byte lastLongPressed = 0xFF;
    int longPressedCalls = 0;
    byte lastReleased = 0xFF;
    int releasedCalls = 0;
};

class SwitchingModeSpy : public ModeSpy
{
  public:
    SwitchingModeSpy(IMode*& activeMode, IMode* nextMode) : _activeMode(activeMode), _nextMode(nextMode) {}

    void buttonPressed(const byte number) override
    {
        ModeSpy::buttonPressed(number);
        _activeMode = _nextMode;
    }

  private:
    IMode*& _activeMode;
    IMode* _nextMode;
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

void test_button_down_routes_to_active_mode()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = &fixture.modeSpy;

    fixture.handler.buttonDown(2);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.downCalls);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.modeSpy.lastDown);
}

void test_button_pressed_routes_to_active_mode()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = &fixture.modeSpy;

    fixture.handler.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.pressedCalls);
    TEST_ASSERT_EQUAL_UINT8(3, fixture.modeSpy.lastPressed);
}

void test_button_released_routes_to_active_mode()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = &fixture.modeSpy;

    fixture.handler.buttonReleased(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.releasedCalls);
    TEST_ASSERT_EQUAL_UINT8(7, fixture.modeSpy.lastReleased);
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

    fixture.handler.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(0, fixture.modeSpy.pressedCalls);
}

void test_button_long_pressed_no_mode_does_not_select_touch_button()
{
    ButtonHandlerFixture fixture;
    fixture.activeMode = nullptr;

    fixture.handler.buttonLongPressed(2);

    TEST_ASSERT_EQUAL_INT(0, fixture.modeSpy.longPressedCalls);
}

void test_button_handler_reads_active_mode_pointer_by_reference()
{
    ButtonHandlerFixture fixture;

    fixture.handler.buttonPressed(1);
    TEST_ASSERT_EQUAL_INT(0, fixture.modeSpy.pressedCalls);

    fixture.activeMode = &fixture.modeSpy;
    fixture.handler.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(1, fixture.modeSpy.pressedCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.modeSpy.lastPressed);
}

void test_button_release_stays_with_mode_that_received_press()
{
    Adafruit_NeoPixel strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    IMode* activeMode = nullptr;
    ButtonHandler handler(activeMode, ringManager);
    ModeSpy releasedMode;
    SwitchingModeSpy pressingMode(activeMode, &releasedMode);
    activeMode = &pressingMode;

    handler.buttonDown(4);
    handler.buttonPressed(4);
    handler.buttonReleased(4);

    TEST_ASSERT_EQUAL_INT(1, pressingMode.pressedCalls);
    TEST_ASSERT_EQUAL_INT(1, pressingMode.releasedCalls);
    TEST_ASSERT_EQUAL_INT(0, releasedMode.releasedCalls);
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
    RUN_TEST(test_button_down_routes_to_active_mode);
    RUN_TEST(test_button_pressed_routes_to_active_mode);
    RUN_TEST(test_button_long_pressed_routes_to_active_mode);
    RUN_TEST(test_button_released_routes_to_active_mode);
    RUN_TEST(test_button_pressed_does_nothing_when_no_active_mode);
    RUN_TEST(test_button_long_pressed_no_mode_does_not_select_touch_button);
    RUN_TEST(test_button_handler_reads_active_mode_pointer_by_reference);
    RUN_TEST(test_button_release_stays_with_mode_that_received_press);
    return UNITY_END();
}
