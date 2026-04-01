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

class TestRingColourProvider : public IRingColourProvider
{
  public:
    uint32_t defaultRingColour(uint8_t ringIndex) const override { return 0x101010U * ((uint32_t)ringIndex + 1U); }
};

void test_touch_button_manager_initially_no_selected_button()
{
    TestTouchButtonLayout screenUi;
    TestRingColourProvider ringColours;
    TouchButtonManager manager(screenUi, ringColours);

    TEST_ASSERT_NULL(manager.getSelectedButton());
}

void test_touch_button_manager_button_pressed_selects_button()
{
    TestTouchButtonLayout screenUi;
    TestRingColourProvider ringColours;
    TouchButtonManager manager(screenUi, ringColours);

    // Simulate button press on button 0
    manager.buttonPressed(0);

    // Verify button 0 is now selected
    FootSwitchTouchButton* selected = manager.getSelectedButton();
    TEST_ASSERT_NOT_NULL(selected);
    TEST_ASSERT_EQUAL(0, selected->buttonNumber());
}

void test_touch_button_manager_button_pressed_deselects_previous_button()
{
    TestTouchButtonLayout screenUi;
    TestRingColourProvider ringColours;
    TouchButtonManager manager(screenUi, ringColours);

    // Select button 0
    manager.buttonPressed(0);
    FootSwitchTouchButton* first = manager.getSelectedButton();
    TEST_ASSERT_EQUAL(0, first->buttonNumber());

    // Select button 1
    manager.buttonPressed(1);
    FootSwitchTouchButton* second = manager.getSelectedButton();
    TEST_ASSERT_EQUAL(1, second->buttonNumber());

    // Verify we got a different button
    TEST_ASSERT_NOT_EQUAL(first->buttonNumber(), second->buttonNumber());
}

void test_touch_button_manager_button_pressed_same_button_is_noop()
{
    TestTouchButtonLayout screenUi;
    TestRingColourProvider ringColours;
    TouchButtonManager manager(screenUi, ringColours);

    // Select button 0
    manager.buttonPressed(0);
    FootSwitchTouchButton* first = manager.getSelectedButton();

    // Select button 0 again (should be no-op, same pointer)
    manager.buttonPressed(0);
    FootSwitchTouchButton* second = manager.getSelectedButton();

    TEST_ASSERT_EQUAL_PTR(first, second);
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
    RUN_TEST(test_touch_button_manager_initially_no_selected_button);
    RUN_TEST(test_touch_button_manager_button_pressed_selects_button);
    RUN_TEST(test_touch_button_manager_button_pressed_deselects_previous_button);
    RUN_TEST(test_touch_button_manager_button_pressed_same_button_is_noop);
    UNITY_END();
}

void loop() {}
