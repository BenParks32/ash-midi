#include <unity.h>

#include <string.h>

#include "ButtonOverrideStore.h"
#include "Modes/Mode.h"
#include "SetListStore.h"

#include "../../../src/Function.cpp"
#include "../../../src/ButtonOverrideStore.cpp"
#include "../../../src/SetListStore.cpp"

namespace
{
constexpr byte TestPlaylistIndex = Project7PlaylistIndex;

struct FileFixture
{
    const char *path;
    const char *content;
};

class FakeTextFileStore final : public ITextFileStore
{
public:
    explicit FakeTextFileStore(const FileFixture *files, size_t fileCount)
        : _files(files), _fileCount(fileCount)
    {
    }

    bool readTextFile(const char *path, char *buffer, size_t bufferSize) const override
    {
        for (size_t i = 0; i < _fileCount; ++i)
        {
            if (strcmp(_files[i].path, path) != 0)
            {
                continue;
            }

            strncpy(buffer, _files[i].content, bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            return true;
        }

        return false;
    }

private:
    const FileFixture *_files;
    size_t _fileCount;
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

void test_activate_set_list_sorts_parts_and_songs_and_marks_missing_songs()
{
    const FileFixture files[] = {
        {"/sets/p7/friday.jsn", setListJson},
    };
    FakeTextFileStore textFileStore(files, 1);
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/friday.jsn");
    FakeSongResolver songResolver;
    SetListStore store(textFileStore, directoryStore, songResolver);

    TEST_ASSERT_TRUE(store.activateSetList(TestPlaylistIndex, "friday.jsn"));

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
    const FileFixture files[] = {
        {"/sets/p7/friday.jsn", setListJson},
        {"/sets/p7/saturday.jsn", otherSetListJson},
    };
    FakeTextFileStore textFileStore(files, 2);
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/friday.jsn", "/sets/p7/saturday.jsn");
    FakeSongResolver songResolver;
    SetListStore store(textFileStore, directoryStore, songResolver);

    SetListSummary summaries[4] = {};
    const size_t count = store.listSetLists(TestPlaylistIndex, summaries, 4);

    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("Friday Night", summaries[0].name);
    TEST_ASSERT_EQUAL_STRING("friday.jsn", summaries[0].fileName);
    TEST_ASSERT_EQUAL(2, summaries[0].partCount);
    TEST_ASSERT_EQUAL(3, summaries[0].songCount);
    TEST_ASSERT_EQUAL_STRING("Saturday Night", summaries[1].name);
}

void test_select_song_tracks_current_selection()
{
    const FileFixture files[] = {
        {"/sets/p7/friday.jsn", setListJson},
    };
    FakeTextFileStore textFileStore(files, 1);
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/friday.jsn");
    FakeSongResolver songResolver;
    SetListStore store(textFileStore, directoryStore, songResolver);

    TEST_ASSERT_TRUE(store.activateSetList(TestPlaylistIndex, "friday.jsn"));
    TEST_ASSERT_TRUE(store.selectSong(TestPlaylistIndex, 2));

    SetListSongEntry song = {};
    TEST_ASSERT_TRUE(store.selectedSong(TestPlaylistIndex, song));
    TEST_ASSERT_EQUAL_STRING("Song B", song.name);
    TEST_ASSERT_EQUAL(4, song.number);
}

void test_activate_set_list_resolves_songs_from_button_override_catalog()
{
    const FileFixture files[] = {
        {"/BUTTONS.JSN", buttonsConfigJson},
        {"/p7songs.jsn", externalSongsConfigJson},
        {"/sets/p7/kings-head.jsn", integratedSetListJson},
    };
    FakeTextFileStore textFileStore(files, 3);
    FakeDirectoryStore directoryStore("/sets/p7", "/sets/p7/kings-head.jsn");
    ButtonOverrideStore buttonOverrideStore(&textFileStore);
    TEST_ASSERT_TRUE(buttonOverrideStore.refresh());

    ButtonOverrideSongResolver songResolver(buttonOverrideStore);
    SetListStore store(textFileStore, directoryStore, songResolver);

    TEST_ASSERT_TRUE(store.activateSetList(TestPlaylistIndex, "kings-head.jsn"));

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

void setUp() {}
void tearDown() {}
