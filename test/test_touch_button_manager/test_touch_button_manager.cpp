#include <Arduino.h>
#include <unity.h>

#include "Touch/TouchButton.h"
#include "Touch/TouchButtonManager.h"

// Pull in implementation units required by TouchButtonManager
#include "../../src/Touch/TouchButton.cpp"
#include "../../src/Touch/TouchButtonManager.cpp"

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

    void drawTouchButtonPill(const Point&, const Size&, uint16_t, uint16_t = ITouchButtonCanvas::White565) override
    {
        // no-op for testing
    }
};

class TestPressDelegate : public ITouchButtonDelegate
{
  public:
    enum class EventType : uint8_t
    {
        Down,
        Press,
        LongPress,
        Release,
    };

    void buttonDown(const byte number) override
    {
        events[eventCount] = EventType::Down;
        eventButtons[eventCount] = number;
        ++eventCount;
        lastDown = number;
        ++downCount;
    }

    void buttonPressed(const byte number) override
    {
        events[eventCount] = EventType::Press;
        eventButtons[eventCount] = number;
        ++eventCount;
        lastPressed = number;
        ++pressCount;
    }

    void buttonLongPressed(const byte number) override
    {
        events[eventCount] = EventType::LongPress;
        eventButtons[eventCount] = number;
        ++eventCount;
        lastLongPressed = number;
        ++longPressCount;
    }

    void buttonReleased(const byte number) override
    {
        events[eventCount] = EventType::Release;
        eventButtons[eventCount] = number;
        ++eventCount;
        lastReleased = number;
        ++releaseCount;
    }

    EventType events[8] = {};
    byte eventButtons[8] = {0};
    int eventCount = 0;
    int downCount = 0;
    int pressCount = 0;
    int longPressCount = 0;
    int releaseCount = 0;
    byte lastDown = 0xFF;
    byte lastPressed = 0xFF;
    byte lastLongPressed = 0xFF;
    byte lastReleased = 0xFF;
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

void test_touch_button_manager_button_down_selects_button()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(0)->setEnabled(true);
    manager.buttonDown(0);

    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);
    TEST_ASSERT_EQUAL_UINT8(0, delegate.lastDown);
}

void test_touch_button_manager_button_pressed_ignores_disabled_button()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(0, delegate.pressCount);
}

void test_touch_button_manager_forwards_valid_release_to_delegate()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(2)->setEnabled(true);
    manager.buttonReleased(2);

    TEST_ASSERT_EQUAL_INT(1, delegate.releaseCount);
    TEST_ASSERT_EQUAL_UINT8(2, delegate.lastReleased);
}

void test_touch_handle_detects_button_down_from_coordinates()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    // Button 0 is at (0, 200) with size (60, 60)
    manager.getButton(0)->setEnabled(true);
    manager.handleTouch(30, 230);

    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);
    TEST_ASSERT_EQUAL_INT(0, delegate.pressCount);
    TEST_ASSERT_EQUAL_UINT8(0, delegate.lastDown);
}

void test_touch_handle_ignores_repeated_press_same_button()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    // Button 0 is at (0, 200) with size (60, 60)
    manager.getButton(0)->setEnabled(true);

    // First press should trigger
    manager.handleTouch(30, 230);
    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);

    // Second press at same position should NOT trigger (still touching same button)
    manager.handleTouch(30, 230);
    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);
}

void test_touch_handle_detects_new_button_press()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(0)->setEnabled(true);
    manager.getButton(1)->setEnabled(true);

    // Press button 0
    manager.handleTouch(30, 230);
    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);
    TEST_ASSERT_EQUAL_UINT8(0, delegate.lastDown);

    // Press button 1 (at x=60, y=200)
    manager.handleTouch(90, 230);
    TEST_ASSERT_EQUAL_INT(2, delegate.downCount);
    TEST_ASSERT_EQUAL_INT(1, delegate.releaseCount);
    TEST_ASSERT_EQUAL_UINT8(0, delegate.lastReleased);
    TEST_ASSERT_EQUAL_UINT8(1, delegate.lastDown);
}

void test_touch_handle_release_fires_short_press_then_release()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(7)->setEnabled(true);

    manager.handleTouch(210, 30);
    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);
    TEST_ASSERT_EQUAL_UINT8(7, delegate.lastDown);

    manager.handleTouchRelease();

    TEST_ASSERT_EQUAL_INT(1, delegate.pressCount);
    TEST_ASSERT_EQUAL_UINT8(7, delegate.lastPressed);
    TEST_ASSERT_EQUAL_INT(1, delegate.releaseCount);
    TEST_ASSERT_EQUAL_UINT8(7, delegate.lastReleased);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TestPressDelegate::EventType::Down),
                            static_cast<uint8_t>(delegate.events[0]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TestPressDelegate::EventType::Press),
                            static_cast<uint8_t>(delegate.events[1]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TestPressDelegate::EventType::Release),
                            static_cast<uint8_t>(delegate.events[2]));
}

void test_touch_handle_allows_repeat_press_after_release()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(7)->setEnabled(true);

    manager.handleTouch(210, 30);
    manager.handleTouchRelease();
    manager.handleTouch(210, 30);

    TEST_ASSERT_EQUAL_INT(2, delegate.downCount);
    TEST_ASSERT_EQUAL_UINT8(7, delegate.lastDown);
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
    RUN_TEST(test_touch_button_manager_button_down_selects_button);
    RUN_TEST(test_touch_button_manager_button_pressed_ignores_disabled_button);
    RUN_TEST(test_touch_button_manager_forwards_valid_release_to_delegate);
    RUN_TEST(test_touch_handle_detects_button_down_from_coordinates);
    RUN_TEST(test_touch_handle_ignores_repeated_press_same_button);
    RUN_TEST(test_touch_handle_detects_new_button_press);
    RUN_TEST(test_touch_handle_release_fires_short_press_then_release);
    RUN_TEST(test_touch_handle_allows_repeat_press_after_release);
    UNITY_END();
}

void loop() {}
