#include <unity.h>

#include "Button.h"
#include "Touch/TouchButton.h"
#include "Touch/TouchButtonManager.h"

#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

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
    }

    void drawTouchButtonPill(const Point&, const Size&, uint16_t, uint16_t = ITouchButtonCanvas::White565) override {}
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

void test_touch_handle_release_fires_short_press_then_release()
{
    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(7)->setEnabled(true);

    manager.handleTouch(210, 30);
    manager.handleTouchRelease();

    TEST_ASSERT_EQUAL_INT(1, delegate.pressCount);
    TEST_ASSERT_EQUAL_UINT8(7, delegate.lastPressed);
    TEST_ASSERT_EQUAL_INT(1, delegate.releaseCount);
    TEST_ASSERT_EQUAL_UINT8(7, delegate.lastReleased);
}

void test_touch_handle_fires_long_press_after_host_time_advance()
{
    HostArduino::resetTime();

    TestTouchButtonLayout screenUi;
    TestPressDelegate delegate;
    TouchButtonManager manager(screenUi, &delegate);

    manager.getButton(0)->setEnabled(true);
    manager.handleTouch(30, 230);

    TEST_ASSERT_EQUAL_INT(1, delegate.downCount);
    TEST_ASSERT_EQUAL_INT(0, delegate.longPressCount);

    HostArduino::advanceMillis(ButtonLongPressMs);
    manager.handleTouch(30, 230);

    TEST_ASSERT_EQUAL_INT(1, delegate.longPressCount);
    TEST_ASSERT_EQUAL_UINT8(0, delegate.lastLongPressed);

    manager.handleTouchRelease();
    TEST_ASSERT_EQUAL_INT(0, delegate.pressCount);
    TEST_ASSERT_EQUAL_INT(1, delegate.releaseCount);
}

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
    RUN_TEST(test_touch_button_manager_buttons_start_disabled_and_blank);
    RUN_TEST(test_touch_handle_release_fires_short_press_then_release);
    RUN_TEST(test_touch_handle_fires_long_press_after_host_time_advance);
    return UNITY_END();
}
