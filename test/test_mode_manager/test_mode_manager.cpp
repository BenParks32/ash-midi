#include <Arduino.h>
#include <unity.h>

#include "Modes/ModeManager.h"

// Pull in implementation unit required by ModeManager.
#include "../../src/Modes/ModeManager.cpp"

namespace
{
class FakeMode : public IMode
{
  public:
    void activate() override { ++activateCalls; }
    void deactivate() override { ++deactivateCalls; }
    void buttonPressed(const byte) override {}
    void buttonLongPressed(const byte) override {}
    void frameTick() override {}

    void setTransitionValue(byte transitionValue) override
    {
        ++setTransitionValueCalls;
        lastTransitionValue = transitionValue;
    }

    int activateCalls = 0;
    int deactivateCalls = 0;
    int setTransitionValueCalls = 0;
    byte lastTransitionValue = 0xFF;
};

void test_enter_mode_switches_active_and_activates_target()
{
    FakeMode home;
    FakeMode play;
    FakeMode patch;
    FakeMode menu;

    IMode* activeMode = &home;
    IMode* modes[ModeCount] = {&home, &play, &patch, &menu};
    ModeManager manager(activeMode, modes);

    manager.enterMode(Modes::Play, 42);

    TEST_ASSERT_EQUAL_PTR(&play, activeMode);
    TEST_ASSERT_EQUAL_INT(1, play.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT8(42, play.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, play.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, home.activateCalls);
    TEST_ASSERT_EQUAL_INT(1, home.deactivateCalls);
}

void test_enter_mode_ignores_null_target_mode()
{
    FakeMode home;
    FakeMode play;
    FakeMode menu;

    IMode* activeMode = &home;
    IMode* modes[ModeCount] = {&home, &play, nullptr, &menu};
    ModeManager manager(activeMode, modes);

    manager.enterMode(Modes::Patch, 10);

    TEST_ASSERT_EQUAL_PTR(&home, activeMode);
    TEST_ASSERT_EQUAL_INT(0, home.activateCalls);
}

void test_enter_mode_ignores_out_of_range_mode()
{
    FakeMode home;
    FakeMode play;
    FakeMode patch;
    FakeMode menu;

    IMode* activeMode = &home;
    IMode* modes[ModeCount] = {&home, &play, &patch, &menu};
    ModeManager manager(activeMode, modes);

    manager.enterMode(static_cast<Modes>(ModeCount), 99);

    TEST_ASSERT_EQUAL_PTR(&home, activeMode);
    TEST_ASSERT_EQUAL_INT(0, home.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, play.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, patch.activateCalls);
    TEST_ASSERT_EQUAL_INT(0, menu.activateCalls);
}

void test_enter_mode_can_switch_to_home()
{
    FakeMode home;
    FakeMode play;
    FakeMode patch;
    FakeMode menu;

    IMode* activeMode = &play;
    IMode* modes[ModeCount] = {&home, &play, &patch, &menu};
    ModeManager manager(activeMode, modes);

    manager.enterMode(Modes::Home, 7);

    TEST_ASSERT_EQUAL_PTR(&home, activeMode);
    TEST_ASSERT_EQUAL_INT(1, home.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT8(7, home.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, home.activateCalls);
}

void test_enter_mode_can_switch_to_patch()
{
    FakeMode home;
    FakeMode play;
    FakeMode patch;
    FakeMode menu;

    IMode* activeMode = &play;
    IMode* modes[ModeCount] = {&home, &play, &patch, &menu};
    ModeManager manager(activeMode, modes);

    manager.enterMode(Modes::Patch, 33);

    TEST_ASSERT_EQUAL_PTR(&patch, activeMode);
    TEST_ASSERT_EQUAL_INT(1, patch.setTransitionValueCalls);
    TEST_ASSERT_EQUAL_UINT8(33, patch.lastTransitionValue);
    TEST_ASSERT_EQUAL_INT(1, patch.activateCalls);
    TEST_ASSERT_EQUAL_INT(1, play.deactivateCalls);
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
    RUN_TEST(test_enter_mode_switches_active_and_activates_target);
    RUN_TEST(test_enter_mode_ignores_null_target_mode);
    RUN_TEST(test_enter_mode_ignores_out_of_range_mode);
    RUN_TEST(test_enter_mode_can_switch_to_home);
    RUN_TEST(test_enter_mode_can_switch_to_patch);
    UNITY_END();
}

void loop() {}
