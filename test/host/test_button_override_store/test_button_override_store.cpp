#include <cstdio>
#include <cstring>
#include <unity.h>

#include "ButtonOverrideStore.h"
#include "Modes/Mode.h"

#include "../../../src/Function.cpp"
#include "../../../src/ButtonOverrideStore.cpp"

namespace
{
constexpr size_t MockConfigCapacity = 12288;

class MockTextFileStore : public ITextFileStore
{
  public:
    bool readTextFile(const char* path, char* buffer, size_t bufferSize) const override
    {
        ++readCalls;
        lastPath = path;

        const char* source = contentsForPath(path);
        if (source == nullptr)
        {
            return false;
        }

        const size_t length = std::strlen(source);
        if ((length + 1U) > bufferSize)
        {
            return false;
        }

        std::memcpy(buffer, source, length + 1U);
        return true;
    }

    const char* contentsForPath(const char* path) const
    {
        if (path == nullptr)
        {
            return nullptr;
        }

        if ((std::strcmp(path, "/p7songs.jsn") == 0 || std::strcmp(path, "p7songs.jsn") == 0) && hasProject7SongsContents)
        {
            return project7SongsContents;
        }

        if ((std::strcmp(path, "/oprsongs.jsn") == 0 || std::strcmp(path, "oprsongs.jsn") == 0) && hasOprSongsContents)
        {
            return oprSongsContents;
        }

        if ((std::strcmp(path, "/crsongs.jsn") == 0 || std::strcmp(path, "crsongs.jsn") == 0) && hasCodeRedSongsContents)
        {
            return codeRedSongsContents;
        }

        if ((std::strcmp(path, "/BUTTONS.JSN") == 0 || std::strcmp(path, "BUTTONS.JSN") == 0 ||
             std::strcmp(path, "/buttons.json") == 0 || std::strcmp(path, "buttons.json") == 0) &&
            hasContents)
        {
            return contents;
        }

        return nullptr;
    }

    mutable int readCalls = 0;
    mutable const char* lastPath = nullptr;
    bool hasContents = false;
    bool hasProject7SongsContents = false;
    bool hasOprSongsContents = false;
    bool hasCodeRedSongsContents = false;
    char contents[MockConfigCapacity] = {0};
    char project7SongsContents[MockConfigCapacity] = {0};
    char oprSongsContents[MockConfigCapacity] = {0};
    char codeRedSongsContents[MockConfigCapacity] = {0};
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

void test_apply_overrides_prefers_patch_long_name_for_play_mode_display()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"name\":\"Big Sky\",\"longName\":\"Big Sky Intro\"}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    PatchDisplayConfig patchDisplay;
    store.applyOverrides(2, 5, functions, 8, &patchDisplay);

    TEST_ASSERT_EQUAL_STRING("Big Sky Intro", patchDisplay.name);
    TEST_ASSERT_EQUAL_STRING(" ", functions[0].label());
}

void test_apply_overrides_falls_back_to_patch_name_when_long_name_is_empty()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"5\":{\"name\":\"Big Sky Intro\",\"longName\":\"\"}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    PatchDisplayConfig patchDisplay;
    store.applyOverrides(2, 5, functions, 8, &patchDisplay);

    TEST_ASSERT_EQUAL_STRING("Big Sky Intro", patchDisplay.name);
}

void test_list_songs_preserves_declared_order_and_song_indices()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"songs\":[{\"name\":\"Outro\",\"patch\":7},{\"name\":\"Main\","
                "\"patch\":0},{\"name\":\"Verse\",\"patch\":3}]}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongListEntry entries[4] = {};
    const size_t count = store.listSongs(2, entries, 4);

    TEST_ASSERT_EQUAL_UINT32(3, count);
    TEST_ASSERT_EQUAL_UINT8(0, entries[0].songIndex);
    TEST_ASSERT_EQUAL_UINT8(7, entries[0].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Outro", entries[0].name);
    TEST_ASSERT_EQUAL_UINT8(1, entries[1].songIndex);
    TEST_ASSERT_EQUAL_UINT8(0, entries[1].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Main", entries[1].name);
    TEST_ASSERT_EQUAL_UINT8(2, entries[2].songIndex);
    TEST_ASSERT_EQUAL_UINT8(3, entries[2].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Verse", entries[2].name);
}

void test_song_for_index_reads_patch_and_prefers_long_name_for_display()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"songs\":[{\"name\":\"Big Sky\",\"longName\":\"Big Sky Intro\","
                "\"patch\":5}]}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongConfig song = {};
    TEST_ASSERT_TRUE(store.songForIndex(2, 0, &song));
    TEST_ASSERT_EQUAL_UINT8(5, song.patchNumber);
    TEST_ASSERT_EQUAL_STRING("Big Sky", song.name);
    TEST_ASSERT_EQUAL_STRING("Big Sky Intro", song.displayName);
}

void test_song_for_index_falls_back_to_short_name_when_long_name_is_empty()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"songs\":[{\"name\":\"Big Sky\",\"longName\":\"\",\"patch\":5}]}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongConfig song = {};
    TEST_ASSERT_TRUE(store.songForIndex(2, 0, &song));
    TEST_ASSERT_EQUAL_STRING("Big Sky", song.name);
    TEST_ASSERT_EQUAL_STRING("Big Sky", song.displayName);
}

void test_song_id_is_exposed_for_listing_and_lookup()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"songs\":[{\"id\":\"big-sky\",\"name\":\"Big Sky\","
                "\"longName\":\"Big Sky Intro\",\"patch\":5}]}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongListEntry entries[2] = {};
    TEST_ASSERT_EQUAL_UINT32(1, store.listSongs(2, entries, 2));
    TEST_ASSERT_EQUAL_STRING("big-sky", entries[0].id);

    SongConfig song = {};
    byte songIndex = 99;
    TEST_ASSERT_TRUE(store.songForId(2, "big-sky", songIndex, song));
    TEST_ASSERT_EQUAL_UINT8(0, songIndex);
    TEST_ASSERT_EQUAL_UINT8(5, song.patchNumber);
    TEST_ASSERT_EQUAL_STRING("big-sky", song.id);
    TEST_ASSERT_EQUAL_STRING("Big Sky Intro", song.displayName);
}

void test_list_songs_reads_project7_external_song_file_when_present()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents, "{\"playModes\":{\"Project7\":{}}}");
    std::strcpy(fileStore.project7SongsContents,
                "[{\"name\":\"Zombie\",\"patch\":1},{\"name\":\"Boulevard\",\"patch\":2}]");
    fileStore.hasContents = true;
    fileStore.hasProject7SongsContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongListEntry entries[4] = {};
    const size_t count = store.listSongs(2, entries, 4);

    TEST_ASSERT_EQUAL_UINT32(2, count);
    TEST_ASSERT_EQUAL_UINT8(0, entries[0].songIndex);
    TEST_ASSERT_EQUAL_UINT8(1, entries[0].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Zombie", entries[0].name);
    TEST_ASSERT_EQUAL_UINT8(1, entries[1].songIndex);
    TEST_ASSERT_EQUAL_UINT8(2, entries[1].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Boulevard", entries[1].name);
}

void test_song_for_index_reads_project7_external_song_file_when_present()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents, "{\"playModes\":{\"Project7\":{}}}");
    std::strcpy(fileStore.project7SongsContents,
                "[{\"name\":\"Zombie\",\"patch\":1},{\"name\":\"Boulevard\",\"longName\":\"Boulevard of Broken "
                "Dreams\",\"patch\":2}]");
    fileStore.hasContents = true;
    fileStore.hasProject7SongsContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongConfig song = {};
    TEST_ASSERT_TRUE(store.songForIndex(2, 1, &song));
    TEST_ASSERT_EQUAL_UINT8(2, song.patchNumber);
    TEST_ASSERT_EQUAL_STRING("Boulevard", song.name);
    TEST_ASSERT_EQUAL_STRING("Boulevard of Broken Dreams", song.displayName);
}

void test_song_for_id_reads_project7_external_song_file_when_present()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents, "{\"playModes\":{\"Project7\":{}}}");
    std::strcpy(fileStore.project7SongsContents,
                "[{\"id\":\"zombie\",\"name\":\"Zombie\",\"patch\":1},{\"id\":\"boulevard\",\"name\":\"Boulevard\","
                "\"longName\":\"Boulevard of Broken Dreams\",\"patch\":2}]");
    fileStore.hasContents = true;
    fileStore.hasProject7SongsContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongConfig song = {};
    byte songIndex = 99;
    TEST_ASSERT_TRUE(store.songForId(2, "boulevard", songIndex, song));
    TEST_ASSERT_EQUAL_UINT8(1, songIndex);
    TEST_ASSERT_EQUAL_UINT8(2, song.patchNumber);
    TEST_ASSERT_EQUAL_STRING("boulevard", song.id);
    TEST_ASSERT_EQUAL_STRING("Boulevard", song.name);
    TEST_ASSERT_EQUAL_STRING("Boulevard of Broken Dreams", song.displayName);
}

void test_list_songs_skips_entries_without_a_valid_patch()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"songs\":[{\"name\":\"Bad\"},{\"name\":\"Good\",\"patch\":9}]}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongListEntry entries[2] = {};
    const size_t count = store.listSongs(2, entries, 2);

    TEST_ASSERT_EQUAL_UINT32(1, count);
    TEST_ASSERT_EQUAL_UINT8(1, entries[0].songIndex);
    TEST_ASSERT_EQUAL_UINT8(9, entries[0].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Good", entries[0].name);
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

void test_apply_overrides_reads_code_red_e_flat_toggle_from_buttons_config()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents,
                "{\"playModes\":{\"Project7\":{\"patches\":{\"1\":{\"name\":\"Zombie\"}}},\"CodeRed\":{\"patches\":{"
                "\"0\":{\"buttons\":{\"6\":{\"label\":\"E Flat\",\"colour\":\"FD20\",\"toggle\":true,\"function\":{"
                "\"shortPress\":{\"action\":\"SendMidiControlChange\",\"value\":35,\"secondaryValue\":100}}}}},"
                "\"1\":{\"name\":\"With\"}}}}}");
    fileStore.hasContents = true;

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    store.applyOverrides(CodeRedPlaylistIndex, 0, functions, 8);

    TEST_ASSERT_EQUAL_STRING("E Flat", functions[6].label());
    TEST_ASSERT_EQUAL_HEX16(0xFD20, functions[6].colour());
    TEST_ASSERT_TRUE(functions[6].isToggle());
    TEST_ASSERT_FALSE(functions[6].hasMomentaryBehaviour());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ActionType::SendMidiControlChange),
                            static_cast<uint8_t>(functions[6].action(FunctionBehaviour::ShortPress).type));
    TEST_ASSERT_EQUAL_UINT8(35, functions[6].action(FunctionBehaviour::ShortPress).value);
    TEST_ASSERT_EQUAL_UINT8(100, functions[6].action(FunctionBehaviour::ShortPress).secondaryValue);
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

void test_refresh_accepts_split_song_config()
{
    MockTextFileStore fileStore;
    std::strcpy(fileStore.contents, "{\"playModes\":{\"Project7\":{}}}");
    std::strcpy(fileStore.project7SongsContents, "[");
    for (int songIndex = 0; songIndex < 50; ++songIndex)
    {
        char songEntry[160] = {0};
        std::snprintf(songEntry, sizeof(songEntry),
                      "{\"name\":\"Song %02d Longish Name For External Songs\",\"longName\":\"Song %02d Long Display "
                      "Name For External Songs File\",\"patch\":%d}%s",
                      songIndex, songIndex, (songIndex % 10) + 1, (songIndex < 49) ? "," : "");
        std::strcat(fileStore.project7SongsContents, songEntry);
    }
    std::strcat(fileStore.project7SongsContents, "]");
    fileStore.hasContents = true;
    fileStore.hasProject7SongsContents = true;

    TEST_ASSERT_TRUE(std::strlen(fileStore.project7SongsContents) > 4096U);

    ButtonOverrideStore store(&fileStore);
    TEST_ASSERT_TRUE(store.refresh());

    SongListEntry entries[64] = {};
    const size_t count = store.listSongs(2, entries, 64);
    TEST_ASSERT_EQUAL_UINT32(50, count);
    TEST_ASSERT_EQUAL_STRING("Song 00 Longish Name For External Songs", entries[0].name);
    TEST_ASSERT_EQUAL_STRING("Song 49 Longish Name For External Songs", entries[49].name);
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
    RUN_TEST(test_apply_overrides_prefers_patch_long_name_for_play_mode_display);
    RUN_TEST(test_apply_overrides_falls_back_to_patch_name_when_long_name_is_empty);
    RUN_TEST(test_list_songs_preserves_declared_order_and_song_indices);
    RUN_TEST(test_song_for_index_reads_patch_and_prefers_long_name_for_display);
    RUN_TEST(test_song_for_index_falls_back_to_short_name_when_long_name_is_empty);
    RUN_TEST(test_song_id_is_exposed_for_listing_and_lookup);
    RUN_TEST(test_list_songs_reads_project7_external_song_file_when_present);
    RUN_TEST(test_song_for_index_reads_project7_external_song_file_when_present);
    RUN_TEST(test_song_for_id_reads_project7_external_song_file_when_present);
    RUN_TEST(test_list_songs_skips_entries_without_a_valid_patch);
    RUN_TEST(test_apply_overrides_sets_toggle_flag_when_configured);
    RUN_TEST(test_apply_overrides_matches_playlist_and_patch);
    RUN_TEST(test_apply_overrides_can_reparse_same_config_multiple_times);
    RUN_TEST(test_apply_overrides_reads_code_red_e_flat_toggle_from_buttons_config);
    RUN_TEST(test_list_patches_preserves_buttons_jsn_order);
    RUN_TEST(test_apply_overrides_parses_change_mode_patches_value);
    RUN_TEST(test_refresh_accepts_split_song_config);
    return UNITY_END();
}
