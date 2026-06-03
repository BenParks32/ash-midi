#include <unity.h>

#include <cstdlib>
#include <map>
#include <string>
#include <string.h>
#include <vector>

#include "ButtonOverrideStore.h"
#include "Modes/Mode.h"
#include "SetListStore.h"

#include "../../../src/Function.cpp"
#include "../../../src/ButtonOverrideStore.cpp"
#include "../../../src/SetListStore.cpp"

namespace
{
constexpr byte TestPlaylistIndex = Project7PlaylistIndex;

class FakeBinaryFileStore final : public IBinaryFileStore
{
public:
    bool readBinaryFile(const char* path, uint8_t*& buffer, size_t& size) const override
    {
        const auto it = _files.find(path != nullptr ? path : "");
        if (it == _files.end())
        {
            return false;
        }

        size = it->second.size();
        buffer = static_cast<uint8_t*>(malloc(size));
        if (buffer == nullptr)
        {
            size = 0U;
            return false;
        }

        memcpy(buffer, it->second.data(), size);
        return true;
    }

    void setFileContents(const char* path, const std::vector<uint8_t>& data) { _files[path != nullptr ? path : ""] = data; }

    void clear() { _files.clear(); }

private:
    std::map<std::string, std::vector<uint8_t>> _files;
};

class FakeDirectoryStore final : public ITextDirectoryStore
{
public:
    FakeDirectoryStore(const char *directoryPath, const char *firstFile, const char *secondFile = nullptr)
        : _directoryPath(directoryPath), _firstFile(firstFile), _secondFile(secondFile)
    {
    }

    size_t listTextFiles(const char *directoryPath,
                         TextFilePathEntry *entries,
                         size_t maxEntries) const override
    {
        if (strcmp(directoryPath, _directoryPath) != 0 || entries == nullptr || maxEntries == 0)
        {
            return 0;
        }

        size_t count = 0;
        strncpy(entries[count].path, _firstFile, sizeof(entries[count].path) - 1);
        entries[count].path[sizeof(entries[count].path) - 1] = '\0';
        ++count;

        if (_secondFile != nullptr && count < maxEntries)
        {
            strncpy(entries[count].path, _secondFile, sizeof(entries[count].path) - 1);
            entries[count].path[sizeof(entries[count].path) - 1] = '\0';
            ++count;
        }

        return count;
    }

private:
    const char *_directoryPath;
    const char *_firstFile;
    const char *_secondFile;
};

class FakeSongResolver final : public ISetListSongResolver
{
public:
    bool resolveSong(byte,
                     const char *songId,
                     byte &songIndex,
                     SetListResolvedSong &song) const override
    {
        if (strcmp(songId, "song-a") == 0)
        {
            songIndex = 1;
            strncpy(song.name, "Song A", sizeof(song.name) - 1);
            strncpy(song.displayName, "Song A Display", sizeof(song.displayName) - 1);
            song.name[sizeof(song.name) - 1] = '\0';
            song.displayName[sizeof(song.displayName) - 1] = '\0';
            song.hasDisplayName = true;
            song.patch = 10;
            return true;
        }

        if (strcmp(songId, "song-b") == 0)
        {
            songIndex = 4;
            strncpy(song.name, "Song B", sizeof(song.name) - 1);
            song.name[sizeof(song.name) - 1] = '\0';
            song.patch = 22;
            return true;
        }

        return false;
    }
};

class ButtonOverrideSongResolver final : public ISetListSongResolver
{
public:
    explicit ButtonOverrideSongResolver(IButtonOverrideStore& buttonOverrideStore)
        : _buttonOverrideStore(buttonOverrideStore)
    {
    }

    bool resolveSong(byte playlistIndex,
                     const char* songId,
                     byte& songIndex,
                     SetListResolvedSong& song) const override
    {
        SongConfig resolvedSong = {};
        if (!_buttonOverrideStore.songForId(playlistIndex, songId, songIndex, resolvedSong))
        {
            return false;
        }

        strncpy(song.name, resolvedSong.name, sizeof(song.name) - 1);
        song.name[sizeof(song.name) - 1] = '\0';
        strncpy(song.displayName, resolvedSong.displayName, sizeof(song.displayName) - 1);
        song.displayName[sizeof(song.displayName) - 1] = '\0';
        song.hasDisplayName = resolvedSong.displayName[0] != '\0';
        song.patch = resolvedSong.patchNumber;
        return true;
    }

    IButtonOverrideStore& _buttonOverrideStore;
};

const char *setListJson =
    "{"
    "\"name\":\"Friday Night\","
    "\"parts\":["
    "{\"part\":2,\"name\":\"Second half\"},"
    "{\"part\":1,\"name\":\"First half\"}"
    "],"
    "\"songs\":["
    "{\"number\":4,\"part\":2,\"songId\":\"song-b\"},"
    "{\"number\":2,\"part\":1,\"songId\":\"song-a\"},"
    "{\"number\":3,\"part\":2,\"songId\":\"song-missing\"}"
    "]"
    "}";

const char *otherSetListJson =
    "{"
    "\"name\":\"Saturday Night\","
    "\"parts\":[{\"part\":1,\"name\":\"Full set\"}],"
    "\"songs\":[{\"number\":1,\"part\":1,\"songId\":\"song-a\"}]"
    "}";

const char* externalSongsConfigJson =
    "["
    "{\"id\":\"go-your-own-way\",\"name\":\"GoYrWay\",\"longName\":\"Go Your Own Way\",\"patch\":11},"
    "{\"id\":\"champions\",\"name\":\"Champs\",\"longName\":\"Champions\",\"patch\":22}"
    "]";

const char* buttonsConfigJson =
    "{"
    "\"playModes\":{"
    "\"Project7\":{}"
    "}"
    "}";

const char* integratedSetListJson =
    "{"
    "\"name\":\"King's Head\","
    "\"parts\":[{\"part\":1,\"name\":\"First half\"}],"
    "\"songs\":["
    "{\"number\":1,\"part\":1,\"songId\":\"go-your-own-way\"},"
    "{\"number\":2,\"part\":1,\"songId\":\"champions\"}"
    "]"
    "}";

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

std::vector<uint8_t> buildProject7SongsCatalogue()
{
    const std::vector<std::string> strings = {
        "GoYrWay",
        "Go Your Own Way",
        "go-your-own-way",
        "Champs",
        "Champions",
        "champions",
    };

    std::vector<uint8_t> stringBlob;
    std::vector<uint32_t> stringOffsets;
    stringOffsets.reserve(strings.size());
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
    appendU16(songTable, 0);
    appendU16(songTable, 1);
    appendU16(songTable, 2);
    songTable.push_back(11);
    appendU16(songTable, 3);
    appendU16(songTable, 4);
    appendU16(songTable, 5);
    songTable.push_back(22);

    std::vector<uint8_t> output;
    const uint32_t stringTableOffset = 16U;
    const uint32_t songTableOffset = stringTableOffset + static_cast<uint32_t>(stringTable.size());

    appendU32(output, 0x4D534E47U);
    appendU16(output, 1);
    appendU16(output, 2);
    appendU32(output, stringTableOffset);
    appendU32(output, songTableOffset);
    output.insert(output.end(), stringTable.begin(), stringTable.end());
    output.insert(output.end(), songTable.begin(), songTable.end());

    return output;
}

struct SetListPartFixture
{
    uint16_t part;
    const char* name;
};

struct SetListSongFixture
{
    uint16_t number;
    uint16_t part;
    const char* songId;
};

std::vector<uint8_t> buildSetListCatalogue(const char* name,
                                          const SetListPartFixture* parts,
                                          size_t partCount,
                                          const SetListSongFixture* songs,
                                          size_t songCount)
{
    std::vector<std::string> strings;
    auto addString = [&strings](const char* value) -> uint16_t {
        if (value == nullptr)
        {
            return 0xFFFFU;
        }

        const std::string entry(value);
        for (size_t i = 0; i < strings.size(); ++i)
        {
            if (strings[i] == entry)
            {
                return static_cast<uint16_t>(i);
            }
        }

        strings.push_back(entry);
        return static_cast<uint16_t>(strings.size() - 1U);
    };

    const uint16_t nameIndex = addString(name);
    std::vector<uint8_t> partTable;
    std::vector<uint8_t> songTable;
    for (size_t i = 0; i < partCount; ++i)
    {
        appendU16(partTable, parts[i].part);
        appendU16(partTable, addString(parts[i].name));
    }

    for (size_t i = 0; i < songCount; ++i)
    {
        appendU16(songTable, songs[i].number);
        appendU16(songTable, songs[i].part);
        appendU16(songTable, addString(songs[i].songId));
    }

    std::vector<uint8_t> stringBlob;
    std::vector<uint32_t> stringOffsets;
    stringOffsets.reserve(strings.size());
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

    std::vector<uint8_t> output;
    const uint32_t headerSize = static_cast<uint32_t>(sizeof(MSLT_FileHeader));
    const uint32_t stringTableOffset = headerSize;
    const uint32_t partTableOffset = stringTableOffset + static_cast<uint32_t>(stringTable.size());
    const uint32_t songTableOffset = partTableOffset + static_cast<uint32_t>(partTable.size());

    appendU32(output, MSLT_MAGIC);
    appendU16(output, MSLT_VERSION);
    appendU16(output, static_cast<uint16_t>(partCount));
    appendU16(output, static_cast<uint16_t>(songCount));
    appendU16(output, nameIndex);
    appendU32(output, stringTableOffset);
    appendU32(output, partTableOffset);
    appendU32(output, songTableOffset);
    output.insert(output.end(), stringTable.begin(), stringTable.end());
    output.insert(output.end(), partTable.begin(), partTable.end());
    output.insert(output.end(), songTable.begin(), songTable.end());

    return output;
}

void test_activate_set_list_sorts_parts_and_songs_and_marks_missing_songs()
{
    const SetListPartFixture parts[] = {{2, "Second half"}, {1, "First half"}};
    const SetListSongFixture songs[] = {
        {4, 2, "song-b"},
        {2, 1, "song-a"},
        {3, 2, "song-missing"},
    };
    FakeBinaryFileStore binaryFileStore;
    binaryFileStore.setFileContents("/sets/p7/friday.msl", buildSetListCatalogue("Friday Night", parts, 2, songs, 3));
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/friday.msl");
    FakeSongResolver songResolver;
    SetListStore store(binaryFileStore, directoryStore, songResolver);

    TEST_ASSERT_TRUE(store.activateSetList(TestPlaylistIndex, "friday.msl"));

    ActiveSetList setList = {};
    TEST_ASSERT_TRUE(store.activeSetList(TestPlaylistIndex, setList));
    TEST_ASSERT_EQUAL_STRING("Friday Night", setList.name);
    TEST_ASSERT_EQUAL(2, setList.partCount);
    TEST_ASSERT_EQUAL(3, setList.songCount);

    TEST_ASSERT_EQUAL(1, setList.parts[0].part);
    TEST_ASSERT_EQUAL_STRING("First half", setList.parts[0].name);
    TEST_ASSERT_EQUAL(2, setList.parts[1].part);
    TEST_ASSERT_EQUAL_STRING("Second half", setList.parts[1].name);

    TEST_ASSERT_EQUAL(1, setList.songs[0].part);
    TEST_ASSERT_EQUAL(2, setList.songs[0].number);
    TEST_ASSERT_TRUE(setList.songs[0].available);
    TEST_ASSERT_EQUAL(1, setList.songs[0].songIndex);
    TEST_ASSERT_EQUAL(10, setList.songs[0].patch);
    TEST_ASSERT_EQUAL_STRING("Song A Display", setList.songs[0].name);

    TEST_ASSERT_EQUAL(2, setList.songs[1].part);
    TEST_ASSERT_EQUAL(3, setList.songs[1].number);
    TEST_ASSERT_FALSE(setList.songs[1].available);
    TEST_ASSERT_EQUAL_STRING("song-missing", setList.songs[1].name);

    TEST_ASSERT_EQUAL(2, setList.songs[2].part);
    TEST_ASSERT_EQUAL(4, setList.songs[2].number);
    TEST_ASSERT_TRUE(setList.songs[2].available);
    TEST_ASSERT_EQUAL_STRING("Song B", setList.songs[2].name);
}

void test_list_set_lists_reads_summaries_from_directory_entries()
{
    const SetListPartFixture fridayParts[] = {{2, "Second half"}, {1, "First half"}};
    const SetListSongFixture fridaySongs[] = {
        {4, 2, "song-b"},
        {2, 1, "song-a"},
        {3, 2, "song-missing"},
    };
    const SetListPartFixture saturdayParts[] = {{1, "Full set"}};
    const SetListSongFixture saturdaySongs[] = {{1, 1, "song-a"}};
    FakeBinaryFileStore binaryFileStore;
    binaryFileStore.setFileContents("/sets/p7/friday.msl", buildSetListCatalogue("Friday Night", fridayParts, 2, fridaySongs, 3));
    binaryFileStore.setFileContents("/sets/p7/saturday.msl", buildSetListCatalogue("Saturday Night", saturdayParts, 1, saturdaySongs, 1));
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/friday.msl", "/sets/p7/saturday.msl");
    FakeSongResolver songResolver;
    SetListStore store(binaryFileStore, directoryStore, songResolver);

    SetListSummary summaries[4] = {};
    const size_t count = store.listSetLists(TestPlaylistIndex, summaries, 4);

    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("Friday Night", summaries[0].name);
    TEST_ASSERT_EQUAL_STRING("friday.msl", summaries[0].fileName);
    TEST_ASSERT_EQUAL(2, summaries[0].partCount);
    TEST_ASSERT_EQUAL(3, summaries[0].songCount);
    TEST_ASSERT_EQUAL_STRING("Saturday Night", summaries[1].name);
}

void test_select_song_tracks_current_selection()
{
    const SetListPartFixture parts[] = {{2, "Second half"}, {1, "First half"}};
    const SetListSongFixture songs[] = {
        {4, 2, "song-b"},
        {2, 1, "song-a"},
        {3, 2, "song-missing"},
    };
    FakeBinaryFileStore binaryFileStore;
    binaryFileStore.setFileContents("/sets/p7/friday.msl", buildSetListCatalogue("Friday Night", parts, 2, songs, 3));
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/friday.msl");
    FakeSongResolver songResolver;
    SetListStore store(binaryFileStore, directoryStore, songResolver);

    TEST_ASSERT_TRUE(store.activateSetList(TestPlaylistIndex, "friday.msl"));
    TEST_ASSERT_TRUE(store.selectSong(TestPlaylistIndex, 2));

    SetListSongEntry song = {};
    TEST_ASSERT_TRUE(store.selectedSong(TestPlaylistIndex, song));
    TEST_ASSERT_EQUAL_STRING("Song B", song.name);
    TEST_ASSERT_EQUAL(4, song.number);
}

void test_activate_set_list_resolves_songs_from_button_override_catalog()
{
    const SetListPartFixture parts[] = {{1, "First half"}};
    const SetListSongFixture songs[] = {
        {1, 1, "go-your-own-way"},
        {2, 1, "champions"},
    };
    FakeBinaryFileStore binaryFileStore;
    binaryFileStore.setFileContents("/sets/p7/kings-head.msl", buildSetListCatalogue("King's Head", parts, 1, songs, 2));
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/kings-head.msl");
    ButtonOverrideStore buttonOverrideStore(nullptr);
    SD.setFileContents("/p7songs.msg", buildProject7SongsCatalogue());

    ButtonOverrideSongResolver songResolver(buttonOverrideStore);
    SetListStore store(binaryFileStore, directoryStore, songResolver);

    TEST_ASSERT_TRUE(store.activateSetList(TestPlaylistIndex, "kings-head.msl"));

    ActiveSetList setList = {};
    TEST_ASSERT_TRUE(store.activeSetList(TestPlaylistIndex, setList));
    TEST_ASSERT_EQUAL(2, setList.songCount);
    TEST_ASSERT_TRUE(setList.songs[0].available);
    TEST_ASSERT_EQUAL_UINT8(0, setList.songs[0].songIndex);
    TEST_ASSERT_EQUAL_UINT8(11, setList.songs[0].patch);
    TEST_ASSERT_EQUAL_STRING("Go Your Own Way", setList.songs[0].name);
    TEST_ASSERT_TRUE(setList.songs[1].available);
    TEST_ASSERT_EQUAL_UINT8(1, setList.songs[1].songIndex);
    TEST_ASSERT_EQUAL_UINT8(22, setList.songs[1].patch);
    TEST_ASSERT_EQUAL_STRING("Champions", setList.songs[1].name);
}
} // namespace

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_activate_set_list_sorts_parts_and_songs_and_marks_missing_songs);
    RUN_TEST(test_list_set_lists_reads_summaries_from_directory_entries);
    RUN_TEST(test_select_song_tracks_current_selection);
    RUN_TEST(test_activate_set_list_resolves_songs_from_button_override_catalog);
    return UNITY_END();
}

void setUp() { SD.clear(); }
void tearDown() { SD.clear(); }
