#include <Arduino.h>
#include <unity.h>

#include "TouchButton.h"
#include "TouchButtonManager.h"

// Pull in implementation units required by TouchButtonManager
#include "../../src/TouchButton.cpp"
#include "../../src/TouchButtonManager.cpp"

// Minimal test fixtures for TouchButtonManager's dependencies
class TestTouchButtonLayout : public ITouchButtonLayout
{
  public:
    int32_t boxWidth() const override { return 60; }
    int32_t bottomRowY() const override { return 200; }
    Size boxSize() const override { return {60, 60}; }

    void drawTouchButtonLabelAndPill(const char*, const Point&, const Size&, uint16_t, bool = false,
                                     uint16_t = ITouchButtonCanvas::White565,
                                     uint16_t = ITouchButtonCanvas::White565) override
    {
        // no-op for testing
    }
};

class TestPressDelegate : public ITouchButtonDelegate
{
  public:
    void buttonPressed(const byte number) override
    {
        ++pressCount;
        lastPressed = number;
    }

    int pressCount = 0;
    byte lastPressed = 0xFF;
};

void test_touch_button_manager_buttons_start_disabled_and_blank()
{
    TestTouchButtonLayout screenUi;
    TouchButtonManager manager(screenUi);

    for (uint8_t i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        FootSwitchTouchButton* button = manager.getButton(i);
        TEST_ASSERT_NOT_NULL(button);
        TEST_ASSERT_FALSE(button->isEnabled());
        TEST_ASSERT_EQUAL_STRING("", button->label());
        TEST_ASSERT_EQUAL_UINT16(0, button->pillColour());
    }
}

void test_touch_button_manager_button_pressed_selects_button()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(0)->setEnabled(true);
    manager.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, delegate.pressCount);
    TEST_ASSERT_EQUAL_UINT8(0, delegate.lastPressed);
}

void test_touch_button_manager_button_pressed_ignores_disabled_button()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(0, delegate.pressCount);
}

void test_touch_button_manager_forwards_valid_press_to_delegate()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(2)->setEnabled(true);
    manager.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(1, delegate.pressCount);
    TEST_ASSERT_EQUAL_UINT8(2, delegate.lastPressed);
}

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
    RUN_TEST(test_touch_button_manager_buttons_start_disabled_and_blank);
    RUN_TEST(test_touch_button_manager_button_pressed_selects_button);
    RUN_TEST(test_touch_button_manager_button_pressed_ignores_disabled_button);
    RUN_TEST(test_touch_button_manager_forwards_valid_press_to_delegate);
    UNITY_END();
}

void loop() {}
