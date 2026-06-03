#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <unity.h>

#include "ButtonOverrideStore.h"
#include "Modes/Mode.h"

#include "../../../src/Function.cpp"
#include "../../../src/ButtonOverrideStore.cpp"

namespace
{
struct SongFixture
{
    const char* id;
    const char* name;
    const char* longName;
    uint8_t patch;
};

struct ButtonActionFixture
{
    uint8_t eventType;
    const char* actionName;
    uint16_t value;
    uint16_t secondaryValue;
};

struct ButtonFixture
{
    uint8_t buttonId;
    const char* label;
    const char* colour;
    bool toggle;
    std::vector<ButtonActionFixture> actions;
};

struct PatchFixture
{
    uint8_t patchNumber;
    const char* name;
    const char* longName;
    std::vector<ButtonFixture> buttons;
};

struct PlayModeFixture
{
    const char* name;
    std::vector<PatchFixture> patches;
};

ButtonActionFixture makeAction(uint8_t eventType, const char* actionName, uint16_t value, uint16_t secondaryValue)
{
    return ButtonActionFixture{eventType, actionName, value, secondaryValue};
}

ButtonFixture makeButton(uint8_t buttonId, const char* label, const char* colour, bool toggle,
                         std::vector<ButtonActionFixture> actions)
{
    return ButtonFixture{buttonId, label, colour, toggle, std::move(actions)};
}

PatchFixture makePatch(uint8_t patchNumber, const char* name, const char* longName, std::vector<ButtonFixture> buttons)
{
    return PatchFixture{patchNumber, name, longName, std::move(buttons)};
}

PlayModeFixture makePlayMode(const char* name, std::vector<PatchFixture> patches)
{
    return PlayModeFixture{name, std::move(patches)};
}

SongFixture makeSong(const char* id, const char* name, const char* longName, uint8_t patch)
{
    return SongFixture{id, name, longName, patch};
}

void appendU16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
}

void appendU32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFFU));
}

uint16_t addString(std::vector<std::string>& strings, const char* value)
{
    if (value == nullptr)
    {
        return MCFG_STRING_INDEX_NONE;
    }

    const std::string candidate(value);
    for (size_t index = 0; index < strings.size(); ++index)
    {
        if (strings[index] == candidate)
        {
            return static_cast<uint16_t>(index);
        }
    }

    strings.push_back(candidate);
    return static_cast<uint16_t>(strings.size() - 1U);
}

std::vector<uint8_t> buildSongCatalogue(const std::vector<SongFixture>& songs)
{
    std::vector<std::string> strings;
    for (const SongFixture& song : songs)
    {
        addString(strings, song.id);
        addString(strings, song.name);
        addString(strings, song.longName);
    }

    std::vector<uint8_t> stringBlob;
    std::vector<uint32_t> stringOffsets;
    for (const std::string& entry : strings)
    {
        stringOffsets.push_back(static_cast<uint32_t>(stringBlob.size()));
        stringBlob.insert(stringBlob.end(), entry.begin(), entry.end());
        stringBlob.push_back('\0');
    }

    std::vector<uint8_t> stringTable;
    appendU16(stringTable, static_cast<uint16_t>(strings.size()));
    for (uint32_t offset : stringOffsets)
    {
        appendU32(stringTable, offset);
    }
    stringTable.insert(stringTable.end(), stringBlob.begin(), stringBlob.end());

    std::vector<uint8_t> songTable;
    for (const SongFixture& song : songs)
    {
        appendU16(songTable, addString(strings, song.name));
        appendU16(songTable, addString(strings, song.longName));
        appendU16(songTable, addString(strings, song.id));
        songTable.push_back(song.patch);
    }

    std::vector<uint8_t> output;
    const uint32_t stringTableOffset = static_cast<uint32_t>(sizeof(MCFG_SongFileHeader));
    const uint32_t songTableOffset = stringTableOffset + static_cast<uint32_t>(stringTable.size());

    appendU32(output, MSNG_MAGIC);
    appendU16(output, MSNG_VERSION);
    appendU16(output, static_cast<uint16_t>(songs.size()));
    appendU32(output, stringTableOffset);
    appendU32(output, songTableOffset);
    output.insert(output.end(), stringTable.begin(), stringTable.end());
    output.insert(output.end(), songTable.begin(), songTable.end());
    return output;
}

std::vector<uint8_t> buildButtonCatalogue(const std::vector<PlayModeFixture>& playModes)
{
    std::vector<std::string> strings;
    for (const PlayModeFixture& playMode : playModes)
    {
        addString(strings, playMode.name);
        for (const PatchFixture& patch : playMode.patches)
        {
            addString(strings, patch.name);
            addString(strings, patch.longName);
            for (const ButtonFixture& button : patch.buttons)
            {
                addString(strings, button.label);
                addString(strings, button.colour);
                for (const ButtonActionFixture& action : button.actions)
                {
                    addString(strings, action.actionName);
                }
            }
        }
    }

    std::vector<uint8_t> stringBlob;
    std::vector<uint32_t> stringOffsets;
    for (const std::string& entry : strings)
    {
        stringOffsets.push_back(static_cast<uint32_t>(stringBlob.size()));
        stringBlob.insert(stringBlob.end(), entry.begin(), entry.end());
        stringBlob.push_back('\0');
    }

    std::vector<MCFG_PlayMode> playModeRecords;
    std::vector<MCFG_Patch> patchRecords;
    std::vector<MCFG_Button> buttonRecords;
    std::vector<MCFG_Function> functionRecords;

    size_t totalPatchCount = 0;
    size_t totalButtonCount = 0;
    size_t totalFunctionCount = 0;
    for (const PlayModeFixture& playMode : playModes)
    {
        totalPatchCount += playMode.patches.size();
        for (const PatchFixture& patch : playMode.patches)
        {
            totalButtonCount += patch.buttons.size();
            for (const ButtonFixture& button : patch.buttons)
            {
                totalFunctionCount += button.actions.size();
            }
        }
    }

    const uint32_t headerSize = static_cast<uint32_t>(sizeof(MCFG_FileHeader));
    const uint32_t stringTableSize = 2U + static_cast<uint32_t>(strings.size()) * 4U + static_cast<uint32_t>(stringBlob.size());
    const uint32_t playModeOffset = headerSize + stringTableSize;
    const uint32_t patchBase = playModeOffset + static_cast<uint32_t>(playModes.size()) * sizeof(MCFG_PlayMode);
    const uint32_t buttonBase = patchBase + static_cast<uint32_t>(totalPatchCount) * sizeof(MCFG_Patch);
    const uint32_t functionBase = buttonBase + static_cast<uint32_t>(totalButtonCount) * sizeof(MCFG_Button);

    uint32_t patchOffsetCursor = patchBase;
    uint32_t buttonOffsetCursor = buttonBase;
    uint32_t functionOffsetCursor = functionBase;
    for (const PlayModeFixture& playMode : playModes)
    {
        MCFG_PlayMode mode = {};
        mode.nameIndex = addString(strings, playMode.name);
        mode.patchCount = static_cast<uint16_t>(playMode.patches.size());
        mode.patchOffset = patchOffsetCursor;
        playModeRecords.push_back(mode);
        patchOffsetCursor += static_cast<uint32_t>(playMode.patches.size()) * sizeof(MCFG_Patch);

        for (const PatchFixture& patch : playMode.patches)
        {
            MCFG_Patch patchRecord = {};
            patchRecord.nameIndex = addString(strings, patch.name);
            patchRecord.longNameIndex = addString(strings, patch.longName);
            patchRecord.patchNumber = patch.patchNumber;
            patchRecord.buttonCount = static_cast<uint8_t>(patch.buttons.size());
            patchRecord.buttonOffset = buttonOffsetCursor;
            patchRecords.push_back(patchRecord);
            buttonOffsetCursor += static_cast<uint32_t>(patch.buttons.size()) * sizeof(MCFG_Button);

            for (const ButtonFixture& button : patch.buttons)
            {
                MCFG_Button buttonRecord = {};
                buttonRecord.buttonId = button.buttonId;
                buttonRecord.labelIndex = addString(strings, button.label);
                buttonRecord.colourIndex = addString(strings, button.colour);
                buttonRecord.toggle = button.toggle ? 1U : 0U;
                buttonRecord.functionCount = static_cast<uint8_t>(button.actions.size());
                buttonRecord.functionOffset = functionOffsetCursor;
                buttonRecords.push_back(buttonRecord);
                functionOffsetCursor += static_cast<uint32_t>(button.actions.size()) * sizeof(MCFG_Function);

                for (const ButtonActionFixture& action : button.actions)
                {
                    MCFG_Function functionRecord = {};
                    functionRecord.eventType = action.eventType;
                    functionRecord.actionType = 0;
                    functionRecord.actionNameIndex = addString(strings, action.actionName);
                    functionRecord.value = action.value;
                    functionRecord.secondaryValue = action.secondaryValue;
                    functionRecords.push_back(functionRecord);
                }
            }
        }
    }

    std::vector<uint8_t> stringTable;
    appendU16(stringTable, static_cast<uint16_t>(strings.size()));
    for (uint32_t offset : stringOffsets)
    {
        appendU32(stringTable, offset);
    }
    stringTable.insert(stringTable.end(), stringBlob.begin(), stringBlob.end());

    std::vector<uint8_t> output;
    const uint32_t stringTableOffset = headerSize;
    const uint32_t playModeTableOffset = stringTableOffset + static_cast<uint32_t>(stringTable.size());

    appendU32(output, MCFG_MAGIC);
    appendU16(output, 1);
    appendU16(output, static_cast<uint16_t>(playModes.size()));
    appendU32(output, stringTableOffset);
    appendU32(output, playModeTableOffset);

    output.insert(output.end(), stringTable.begin(), stringTable.end());
    for (const auto& mode : playModeRecords)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&mode);
        output.insert(output.end(), bytes, bytes + sizeof(mode));
    }
    for (const auto& patch : patchRecords)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&patch);
        output.insert(output.end(), bytes, bytes + sizeof(patch));
    }
    for (const auto& button : buttonRecords)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&button);
        output.insert(output.end(), bytes, bytes + sizeof(button));
    }
    for (const auto& function : functionRecords)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&function);
        output.insert(output.end(), bytes, bytes + sizeof(function));
    }

    return output;
}

void loadButtonConfig(const std::vector<PlayModeFixture>& playModes)
{
    SD.setFileContents("/buttons.mcfg", buildButtonCatalogue(playModes));
}

void loadProject7Songs(const std::vector<SongFixture>& songs)
{
    SD.setFileContents("/p7songs.msg", buildSongCatalogue(songs));
}

void test_refresh_applies_tap_alias_and_button_override()
{
    const std::vector<PlayModeFixture> config = {
        makePlayMode("Project7",
                     {makePatch(5, "Big Sky Intro", nullptr,
                                {makeButton(3, "Tap", "001F", false, {makeAction(0, "Tap", 0, 0)})})})};
    loadButtonConfig(config);

    ButtonOverrideStore store;
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    functions[3] = Function();
    store.applyOverrides(Project7PlaylistIndex, 5, functions, 8);

    TEST_ASSERT_EQUAL_STRING("Tap", functions[3].label());
    TEST_ASSERT_EQUAL_HEX16(0x001F, functions[3].colour());
    TEST_ASSERT_FALSE(functions[3].hasMomentaryBehaviour());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ActionType::TapTempo),
                            static_cast<uint8_t>(functions[3].action(FunctionBehaviour::ShortPress).type));
}

void test_apply_overrides_uses_patch_display_name()
{
    const std::vector<PlayModeFixture> config = {makePlayMode("Project7", {makePatch(5, "Big Sky", "Big Sky Intro", {})})};
    loadButtonConfig(config);

    ButtonOverrideStore store;
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    PatchDisplayConfig patchDisplay;
    store.applyOverrides(Project7PlaylistIndex, 5, functions, 8, &patchDisplay);

    TEST_ASSERT_EQUAL_STRING("Big Sky Intro", patchDisplay.name);
}

void test_list_patches_preserves_config_order()
{
    const std::vector<PlayModeFixture> config = {
        makePlayMode("Project7", {makePatch(7, "Outro", nullptr, {}), makePatch(0, "Main", nullptr, {}),
                                  makePatch(3, "Verse", nullptr, {})})};
    loadButtonConfig(config);

    ButtonOverrideStore store;
    TEST_ASSERT_TRUE(store.refresh());

    PatchListEntry entries[4] = {};
    const size_t count = store.listPatches(Project7PlaylistIndex, entries, 4);

    TEST_ASSERT_EQUAL_UINT32(3, count);
    TEST_ASSERT_EQUAL_UINT8(7, entries[0].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Outro", entries[0].name);
    TEST_ASSERT_EQUAL_UINT8(0, entries[1].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Main", entries[1].name);
    TEST_ASSERT_EQUAL_UINT8(3, entries[2].patchNumber);
    TEST_ASSERT_EQUAL_STRING("Verse", entries[2].name);
}

void test_song_catalogue_round_trips_ids_and_display_names()
{
    const std::vector<PlayModeFixture> config = {makePlayMode("Project7", {})};
    loadButtonConfig(config);
    const std::vector<SongFixture> songs = {makeSong("zombie", "Zombie", nullptr, 1),
                                            makeSong("boulevard", "Boulevard", "Boulevard of Broken Dreams", 2)};
    loadProject7Songs(songs);

    ButtonOverrideStore store;
    TEST_ASSERT_TRUE(store.refresh());

    SongListEntry entries[4] = {};
    TEST_ASSERT_EQUAL_UINT32(2, store.listSongs(Project7PlaylistIndex, entries, 4));
    TEST_ASSERT_EQUAL_STRING("zombie", entries[0].id);
    TEST_ASSERT_EQUAL_STRING("Zombie", entries[0].name);

    SongConfig song = {};
    TEST_ASSERT_TRUE(store.songForIndex(Project7PlaylistIndex, 1, &song));
    TEST_ASSERT_EQUAL_STRING("Boulevard", song.name);
    TEST_ASSERT_EQUAL_STRING("Boulevard of Broken Dreams", song.displayName);

    byte songIndex = 0;
    TEST_ASSERT_TRUE(store.songForId(Project7PlaylistIndex, "boulevard", songIndex, song));
    TEST_ASSERT_EQUAL_UINT8(1, songIndex);
    TEST_ASSERT_EQUAL_STRING("boulevard", song.id);
}

void test_apply_overrides_sets_toggle_and_cc_action()
{
    const std::vector<PlayModeFixture> config = {
        makePlayMode("CodeRed",
                     {makePatch(0, "Code Red", nullptr,
                                {makeButton(6, "E Flat", "FD20", true,
                                            {makeAction(0, "SendMidiControlChange", 35, 100)})})})};
    loadButtonConfig(config);

    ButtonOverrideStore store;
    TEST_ASSERT_TRUE(store.refresh());

    Function functions[8] = {};
    store.applyOverrides(CodeRedPlaylistIndex, 0, functions, 8);

    TEST_ASSERT_EQUAL_STRING("E Flat", functions[6].label());
    TEST_ASSERT_EQUAL_HEX16(0xFD20, functions[6].colour());
    TEST_ASSERT_TRUE(functions[6].isToggle());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ActionType::SendMidiControlChange),
                            static_cast<uint8_t>(functions[6].action(FunctionBehaviour::ShortPress).type));
    TEST_ASSERT_EQUAL_UINT8(35, functions[6].action(FunctionBehaviour::ShortPress).value);
}

} // namespace

void setUp()
{
    SD.clear();
}

void tearDown()
{
    SD.clear();
}

int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_refresh_applies_tap_alias_and_button_override);
    RUN_TEST(test_apply_overrides_uses_patch_display_name);
    RUN_TEST(test_list_patches_preserves_config_order);
    RUN_TEST(test_song_catalogue_round_trips_ids_and_display_names);
    RUN_TEST(test_apply_overrides_sets_toggle_and_cc_action);
    return UNITY_END();
}
