#include <cstring>
#include <unity.h>

#include "ButtonOverrideStore.h"
#include "Modes/Mode.h"

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

void test_apply_overrides_reads_patch_name_without_button_overrides()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents, "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"name\":\"Big Sky Intro\"}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    PatchDisplayConfig patchDisplay;
    store.applyOverrides(2, 5, functions, 8, &patchDisplay);

    TEST_ASSERT_EQUAL_STRING("Big Sky Intro", patchDisplay.name);
    TEST_ASSERT_EQUAL_STRING(" ", functions[0].label());
}

void test_apply_overrides_sets_toggle_flag_when_configured()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"buttons\":{\"7\":{\"toggle\":true,"
                "\"function\":{\"shortPress\":{\"action\":\"SetTuner\",\"value\":1},"
                "\"longPress\":{\"action\":\"SetTuner\",\"value\":0}}}}}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    functions[7] = Function("Tuner", 0x801F, ActionType::SetTuner, 1, ActionType::SetTuner, 0);
    store.applyOverrides(2, 5, functions, 8);

    TEST_ASSERT_TRUE(functions[7].isToggle());
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

void test_list_patches_preserves_buttons_jsn_order()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"7\":{\"name\":\"Outro\"},\"0\":{\"name\":\"Main\"},"
                "\"abc\":{\"name\":\"Ignored\"},\"3\":{\"name\":\"Verse\"}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    PatchListEntry entries[4] = {};
    const size_t count = store.listPatches(2, entries, 4);

    TEST_ASSERT_EQUAL_UINT32(3, count);
    TEST_ASSERT_EQUAL_UINT8(7, entries[0].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Outro", entries[0].name);
    TEST_ASSERT_EQUAL_UINT8(0, entries[1].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Main", entries[1].name);
    TEST_ASSERT_EQUAL_UINT8(3, entries[2].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Verse", entries[2].name);
}

void test_apply_overrides_parses_change_mode_patches_value()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"buttons\":{\"4\":{\"function\":{\"shortPress\":"
                "{\"action\":\"ChangeMode\",\"value\":\"Patches\"}}}}}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    functions[4] = Function("Patch", 0xFFE0, ActionType::ChangeMode, static_cast<byte>(Modes::Patch), ActionType::None, 0);
    store.applyOverrides(2, 5, functions, 8);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ActionType::ChangeMode),
                            static_cast<uint8_t>(functions[4].action(FunctionBehaviour::ShortPress).type));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patches),
                            functions[4].action(FunctionBehaviour::ShortPress).value);
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
    RUN_TEST(test_apply_overrides_reads_patch_name_without_button_overrides);
    RUN_TEST(test_apply_overrides_sets_toggle_flag_when_configured);
    RUN_TEST(test_apply_overrides_matches_playlist_and_patch);
    RUN_TEST(test_apply_overrides_can_reparse_same_config_multiple_times);
    RUN_TEST(test_list_patches_preserves_buttons_jsn_order);
    RUN_TEST(test_apply_overrides_parses_change_mode_patches_value);
    return UNITY_END();
}
