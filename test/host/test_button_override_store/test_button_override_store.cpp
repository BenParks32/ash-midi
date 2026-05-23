#include <cstring>
#include <unity.h>

#include "ButtonOverrideStore.h"

#include "../../../src/Function.cpp"
#include "../../../src/ButtonOverrideStore.cpp"

namespace
{
class MockTextFileStore : public ITextFileStore
{
  public:
    bool readTextFile(const char* path, char* buffer, size_t bufferSize) const override
    {
        ++readCalls;
        lastPath = path;
        if (!hasContents)
        {
            return false;
        }

        const size_t length = std::strlen(contents);
        if ((length + 1U) > bufferSize)
        {
            return false;
        }

        std::memcpy(buffer, contents, length + 1U);
        return true;
    }

    mutable int readCalls = 0;
    mutable const char* lastPath = nullptr;
    bool hasContents = false;
    char contents[2048] = {0};
};

void test_refresh_parses_tap_alias_for_blank_default_button()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"buttons\":{\"3\":{\"label\":\"Tap\","
                "\"colour\":\"001F\",\"function\":{\"shortPress\":\"Tap\"}}}}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    functions[3] = Function();
    store.applyOverrides(2, 5, functions, 8);

    TEST_ASSERT_EQUAL_STRING("Tap", functions[3].label());
    TEST_ASSERT_EQUAL_HEX16(0x001F, functions[3].colour());
    TEST_ASSERT_FALSE(functions[3].hasMomentaryBehaviour());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ActionType::TapTempo),
                            static_cast<uint8_t>(functions[3].action(FunctionBehaviour::ShortPress).type));
}

void test_apply_overrides_matches_playlist_and_patch()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"buttons\":{\"0\":{\"label\":\"Alt\"}}}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    functions[0] = Function("Clean", 0x07E0, ActionType::SelectScene, 0, ActionType::None, 0);

    store.applyOverrides(3, 5, functions, 8);
    TEST_ASSERT_EQUAL_STRING("Clean", functions[0].label());

    store.applyOverrides(2, 6, functions, 8);
    TEST_ASSERT_EQUAL_STRING("Clean", functions[0].label());

    store.applyOverrides(2, 5, functions, 8);
    TEST_ASSERT_EQUAL_STRING("Alt", functions[0].label());
}

void test_apply_overrides_can_reparse_same_config_multiple_times()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"CodeRed\":{\"patches\":{\"0\":{\"buttons\":{\"3\":{\"label\":\"CC37\","
                "\"colour\":\"07E0\",\"function\":{\"press\":{\"action\":\"SendMidiControlChange\",\"value\":37,"
                "\"secondaryValue\":100},\"release\":{\"action\":\"SendMidiControlChange\",\"value\":37,"
                "\"secondaryValue\":100}}}}},\"1\":{\"buttons\":{\"3\":{\"label\":\"CC37\"}}}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function patchZeroFunctions[8] = {};
    Function patchOneFunctions[8] = {};

    store.applyOverrides(4, 0, patchZeroFunctions, 8);
    TEST_ASSERT_EQUAL_STRING("CC37", patchZeroFunctions[3].label());
    TEST_ASSERT_EQUAL_HEX16(0x07E0, patchZeroFunctions[3].colour());
    TEST_ASSERT_TRUE(patchZeroFunctions[3].hasMomentaryBehaviour());
    TEST_ASSERT_EQUAL_UINT8(37, patchZeroFunctions[3].action(FunctionBehaviour::ButtonDown).value);
    TEST_ASSERT_EQUAL_UINT8(100, patchZeroFunctions[3].action(FunctionBehaviour::ButtonDown).secondaryValue);
    TEST_ASSERT_EQUAL_UINT8(37, patchZeroFunctions[3].action(FunctionBehaviour::ButtonRelease).value);
    TEST_ASSERT_EQUAL_UINT8(100, patchZeroFunctions[3].action(FunctionBehaviour::ButtonRelease).secondaryValue);

    store.applyOverrides(4, 1, patchOneFunctions, 8);
    TEST_ASSERT_EQUAL_STRING("CC37", patchOneFunctions[3].label());
}
} // namespace

void setUp() {}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_refresh_parses_tap_alias_for_blank_default_button);
    RUN_TEST(test_apply_overrides_matches_playlist_and_patch);
    RUN_TEST(test_apply_overrides_can_reparse_same_config_multiple_times);
    return UNITY_END();
}
