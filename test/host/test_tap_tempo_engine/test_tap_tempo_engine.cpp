#include <unity.h>

#include "TapTempoEngine.h"

#include "../../../src/TapTempoEngine.cpp"

void test_does_not_emit_midi_until_third_tap()
{
    TapTempoEngine engine;

    engine.registerPress(0U);
    engine.registerPress(500U);

    TEST_ASSERT_FALSE(engine.tick(999U).shouldSendMidi);

    engine.registerPress(1000U);

    TEST_ASSERT_FALSE(engine.tick(1499U).shouldSendMidi);
    TEST_ASSERT_TRUE(engine.tick(1500U).shouldSendMidi);
}

void test_continued_tapping_does_not_delay_next_scheduled_send()
{
    TapTempoEngine engine;

    engine.registerPress(0U);
    engine.registerPress(500U);
    engine.registerPress(1000U);
    engine.registerPress(1450U);

    TEST_ASSERT_TRUE(engine.tick(1500U).shouldSendMidi);
}

void test_pre_arm_taps_reset_after_inactivity_timeout()
{
    TapTempoEngine engine;

    engine.registerPress(0U);
    engine.registerPress(TapTempoEngine::InactivityTimeoutMs + 100U);
    engine.registerPress(TapTempoEngine::InactivityTimeoutMs + 600U);

    TEST_ASSERT_FALSE(engine.tick(TapTempoEngine::InactivityTimeoutMs + 1099U).shouldSendMidi);

    engine.registerPress(TapTempoEngine::InactivityTimeoutMs + 1100U);

    TEST_ASSERT_TRUE(engine.tick(TapTempoEngine::InactivityTimeoutMs + 1600U).shouldSendMidi);
}

void test_inactivity_emits_three_trailing_messages_then_stops()
{
    TapTempoEngine engine;

    engine.registerPress(0U);
    engine.registerPress(1000U);
    engine.registerPress(2000U);

    TEST_ASSERT_TRUE(engine.tick(3000U).shouldSendMidi);
    TEST_ASSERT_TRUE(engine.tick(4000U).shouldSendMidi);
    TEST_ASSERT_TRUE(engine.tick(5000U).shouldSendMidi);
    TEST_ASSERT_TRUE(engine.tick(6000U).shouldSendMidi);
    TEST_ASSERT_TRUE(engine.tick(7000U).shouldSendMidi);
    TEST_ASSERT_FALSE(engine.tick(8000U).shouldSendMidi);
}

void setUp() {}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_does_not_emit_midi_until_third_tap);
    RUN_TEST(test_continued_tapping_does_not_delay_next_scheduled_send);
    RUN_TEST(test_pre_arm_taps_reset_after_inactivity_timeout);
    RUN_TEST(test_inactivity_emits_three_trailing_messages_then_stops);
    return UNITY_END();
}
