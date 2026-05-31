#include "SetListStore.h"

#include "ButtonOverrideStore.h"

#include <ArduinoJson.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace
{
template <typename T>
void sortAscending(T *items, size_t count, bool (*compare)(const T &, const T &))
{
    if (items == nullptr || count < 2)
    {
        return;
    }

    for (size_t i = 0; i + 1 < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            if (compare(items[j], items[i]))
            {
                const T temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
}

bool partCompare(const SetListPart &left, const SetListPart &right)
{
    return left.part < right.part;
}

bool songCompare(const SetListSongEntry &left, const SetListSongEntry &right)
{
    if (left.part != right.part)
    {
        return left.part < right.part;
    }

    if (left.number != right.number)
    {
        return left.number < right.number;
    }

    return strcmp(left.songId, right.songId) < 0;
}

size_t countObjectsInNamedArray(const char* json, const char* key)
{
    if (json == nullptr || key == nullptr)
    {
        return 0;
    }

    const char* keyLocation = strstr(json, key);
    if (keyLocation == nullptr)
    {
        return 0;
    }

    const char* arrayStart = strchr(keyLocation, '[');
    if (arrayStart == nullptr)
    {
        return 0;
    }

    bool inString = false;
    bool escaped = false;
    size_t arrayDepth = 1;
    size_t objectCount = 0;

    for (const char* cursor = arrayStart + 1; *cursor != '\0'; ++cursor)
    {
        const char current = *cursor;
        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (current == '\\')
            {
                escaped = true;
            }
            else if (current == '"')
            {
                inString = false;
            }

            continue;
        }

        if (current == '"')
        {
            inString = true;
            continue;
        }

        if (current == '[')
        {
            ++arrayDepth;
            continue;
        }

        if (current == ']')
        {
            if (arrayDepth == 0)
            {
                return objectCount;
            }

            --arrayDepth;
            if (arrayDepth == 0)
            {
                return objectCount;
            }

            continue;
        }

        if (current == '{' && arrayDepth == 1)
        {
            ++objectCount;
        }
    }

    return objectCount;
}

size_t clippedCount(size_t value, size_t maximum)
{
    return value < maximum ? value : maximum;
}
} // namespace

SetListStore::SetListStore(const ITextFileStore &textFileStore,
                           const ITextDirectoryStore &directoryStore,
                           const ISetListSongResolver &songResolver)
    : _textFileStore(textFileStore), _directoryStore(directoryStore), _songResolver(songResolver)
{
}

size_t SetListStore::listSetLists(byte playlistIndex,
                                  SetListSummary *summaries,
                                  size_t maxSummaries) const
{
    if (summaries == nullptr || maxSummaries == 0)
    {
        return 0;
    }

    char directoryPath[TextFilePathEntry::MaxPathLength];
    if (!directoryForPlaylist(playlistIndex, directoryPath, sizeof(directoryPath)))
    {
        Serial.printf("Set lists: no directory mapping for playlist %u.\n", static_cast<unsigned int>(playlistIndex));
        return 0;
    }

    TextFilePathEntry entries[ActiveSetList::MaxSongs];
    const size_t entryCount = _directoryStore.listTextFiles(directoryPath, entries, ActiveSetList::MaxSongs);

    size_t loadedCount = 0;
    for (size_t i = 0; i < entryCount && loadedCount < maxSummaries; ++i)
    {
        if (!loadSetListSummary(entries[i].path, summaries[loadedCount]))
        {
            Serial.printf("Set lists: failed to load summary from '%s'.\n", entries[i].path);
            continue;
        }

        ++loadedCount;
    }

    return loadedCount;
}

bool SetListStore::activateSetList(byte playlistIndex, const char *fileName)
{
    if (playlistIndex >= MaxPlaylistCount || fileName == nullptr || fileName[0] == '\0')
    {
        Serial.printf("Set lists: activate rejected for playlist %u file '%s'.\n", static_cast<unsigned int>(playlistIndex),
                      fileName != nullptr ? fileName : "<null>");
        return false;
    }

    char directoryPath[TextFilePathEntry::MaxPathLength];
    if (!directoryForPlaylist(playlistIndex, directoryPath, sizeof(directoryPath)))
    {
        Serial.printf("Set lists: activate failed because playlist %u has no directory mapping.\n",
                      static_cast<unsigned int>(playlistIndex));
        return false;
    }

    char fullPath[ActiveSetList::MaxPathLength];
    if (strchr(fileName, '/') != nullptr)
    {
        safeCopy(fullPath, sizeof(fullPath), fileName);
    }
    else
    {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", directoryPath, fileName);
    }

    PlaylistState &state = _playlistStates[playlistIndex];
    state.setList = {};
    if (!loadSetList(playlistIndex, fullPath, state.setList))
    {
        state.hasActiveSet = false;
        Serial.printf("Set lists: failed to activate '%s'.\n", fullPath);
        return false;
    }

    state.hasActiveSet = true;
    return true;
}

bool SetListStore::clearActiveSetList(byte playlistIndex)
{
    if (playlistIndex >= MaxPlaylistCount)
    {
        Serial.printf("Set lists: clear active set rejected for playlist %u.\n", static_cast<unsigned int>(playlistIndex));
        return false;
    }

    _playlistStates[playlistIndex].hasActiveSet = false;
    _playlistStates[playlistIndex].setList = {};
    return true;
}

bool SetListStore::activeSetList(byte playlistIndex, ActiveSetList &setList) const
{
    if (playlistIndex >= MaxPlaylistCount || !_playlistStates[playlistIndex].hasActiveSet)
    {
        return false;
    }

    setList = _playlistStates[playlistIndex].setList;
    return true;
}

bool SetListStore::activeSetSummary(byte playlistIndex, SetListSummary &summary) const
{
    if (playlistIndex >= MaxPlaylistCount || !_playlistStates[playlistIndex].hasActiveSet)
    {
        return false;
    }

    const ActiveSetList &setList = _playlistStates[playlistIndex].setList;
    safeCopy(summary.name, sizeof(summary.name), setList.name);
    safeCopy(summary.fileName, sizeof(summary.fileName), fileNameFromPath(setList.sourcePath));
    summary.partCount = setList.partCount;
    summary.songCount = setList.songCount;
    return true;
}

bool SetListStore::selectSong(byte playlistIndex, size_t setSongIndex)
{
    if (playlistIndex >= MaxPlaylistCount)
    {
        return false;
    }

    PlaylistState &state = _playlistStates[playlistIndex];
    if (!state.hasActiveSet || setSongIndex >= state.setList.songCount)
    {
        return false;
    }

    state.setList.selectedSongIndex = setSongIndex;
    return true;
}

bool SetListStore::selectedSong(byte playlistIndex, SetListSongEntry &song) const
{
    if (playlistIndex >= MaxPlaylistCount)
    {
        return false;
    }

    const PlaylistState &state = _playlistStates[playlistIndex];
    if (!state.hasActiveSet || state.setList.selectedSongIndex >= state.setList.songCount)
    {
        return false;
    }

    song = state.setList.songs[state.setList.selectedSongIndex];
    return true;
}

bool SetListStore::loadSetList(byte playlistIndex, const char *path, ActiveSetList &setList) const
{
    char *jsonBuffer = static_cast<char *>(malloc(MaxJsonSize));
    if (jsonBuffer == nullptr)
    {
        Serial.printf("Set lists: failed to allocate %u bytes for '%s'.\n", static_cast<unsigned int>(MaxJsonSize),
                      path != nullptr ? path : "<null>");
        return false;
    }

    if (!_textFileStore.readTextFile(path, jsonBuffer, MaxJsonSize))
    {
        Serial.printf("Set lists: failed to read '%s' for playlist %u.\n", path != nullptr ? path : "<null>",
                      static_cast<unsigned int>(playlistIndex));
        free(jsonBuffer);
        return false;
    }

    {
        const size_t partCount = clippedCount(countObjectsInNamedArray(jsonBuffer, "\"parts\""), ActiveSetList::MaxParts);
        const size_t songCount = clippedCount(countObjectsInNamedArray(jsonBuffer, "\"songs\""), ActiveSetList::MaxSongs);
        const size_t jsonCapacity = fullSetListJsonCapacity(partCount, songCount);

        DynamicJsonDocument document(jsonCapacity);
        if (document.capacity() == 0)
        {
            Serial.printf("Set lists: failed to allocate %u-byte JSON document for '%s'.\n",
                          static_cast<unsigned int>(jsonCapacity), path != nullptr ? path : "<null>");
            free(jsonBuffer);
            return false;
        }

        const DeserializationError error = deserializeJson(document, jsonBuffer);
        if (error)
        {
            Serial.printf("Set lists: JSON parse failed for '%s': %s\n", path != nullptr ? path : "<null>",
                          error.c_str());
            free(jsonBuffer);
            return false;
        }

        safeCopy(setList.name, sizeof(setList.name), document["name"] | fileNameFromPath(path));
        safeCopy(setList.sourcePath, sizeof(setList.sourcePath), path);

        JsonArray parts = document["parts"].as<JsonArray>();
        for (JsonObject partObject : parts)
        {
            if (setList.partCount >= ActiveSetList::MaxParts)
            {
                break;
            }

            SetListPart &part = setList.parts[setList.partCount++];
            part.part = partObject["part"] | 0;
            safeCopy(part.name, sizeof(part.name), partObject["name"] | "");
        }
        sortParts(setList.parts, setList.partCount);

        JsonArray songs = document["songs"].as<JsonArray>();
        for (JsonObject songObject : songs)
        {
            if (setList.songCount >= ActiveSetList::MaxSongs)
            {
                break;
            }

            SetListSongEntry &song = setList.songs[setList.songCount++];
            song.number = songObject["number"] | 0;
            song.part = songObject["part"] | 0;
            safeCopy(song.songId, sizeof(song.songId), songObject["songId"] | "");
        }
    }

    free(jsonBuffer);

    size_t resolvedSongCount = 0;
    size_t unresolvedSongCount = 0;
    for (size_t songEntryIndex = 0; songEntryIndex < setList.songCount; ++songEntryIndex)
    {
        SetListSongEntry &song = setList.songs[songEntryIndex];

        SetListResolvedSong resolvedSong = {};
        if (_songResolver.resolveSong(playlistIndex, song.songId, song.songIndex, resolvedSong))
        {
            song.available = true;
            song.patch = resolvedSong.patch;
            safeCopy(song.name,
                     sizeof(song.name),
                     resolvedSong.hasDisplayName ? resolvedSong.displayName : resolvedSong.name);
            ++resolvedSongCount;
        }
        else
        {
            song.available = false;
            safeCopy(song.name, sizeof(song.name), song.songId);
            ++unresolvedSongCount;
            Serial.printf("Set lists: unresolved song id '%s' in '%s'.\n", song.songId, path != nullptr ? path : "<null>");
        }
    }

    sortSongs(setList.songs, setList.songCount);
    setList.selectedSongIndex = 0;
    return true;
}

size_t SetListStore::fullSetListJsonCapacity(size_t partCount, size_t songCount)
{
    return JSON_OBJECT_SIZE(3) +
           JSON_ARRAY_SIZE(partCount) +
           (partCount * JSON_OBJECT_SIZE(2)) +
           JSON_ARRAY_SIZE(songCount) +
           (songCount * JSON_OBJECT_SIZE(3));
}

bool SetListStore::loadSetListSummary(const char *path, SetListSummary &summary) const
{
    char *jsonBuffer = static_cast<char *>(malloc(MaxJsonSize));
    if (jsonBuffer == nullptr)
    {
        Serial.printf("Set lists: failed to allocate summary buffer for '%s'.\n", path != nullptr ? path : "<null>");
        return false;
    }

    if (!_textFileStore.readTextFile(path, jsonBuffer, MaxJsonSize))
    {
        Serial.printf("Set lists: summary read failed for '%s'.\n", path != nullptr ? path : "<null>");
        free(jsonBuffer);
        return false;
    }

    summary.partCount = countObjectsInNamedArray(jsonBuffer, "\"parts\"");
    summary.songCount = countObjectsInNamedArray(jsonBuffer, "\"songs\"");

    StaticJsonDocument<JSON_OBJECT_SIZE(1)> filter;
    filter["name"] = true;

    StaticJsonDocument<SummaryNameJsonCapacity> document;

    const DeserializationError error = deserializeJson(document, jsonBuffer, DeserializationOption::Filter(filter));
    if (error)
    {
        Serial.printf("Set lists: summary JSON parse failed for '%s': %s\n", path != nullptr ? path : "<null>",
                      error.c_str());
        free(jsonBuffer);
        return false;
    }
    safeCopy(summary.name, sizeof(summary.name), document["name"] | fileNameFromPath(path));
    safeCopy(summary.fileName, sizeof(summary.fileName), fileNameFromPath(path));
    safeCopy(summary.fileName, sizeof(summary.fileName), fileNameFromPath(path));
    free(jsonBuffer);
    return true;
}

bool SetListStore::directoryForPlaylist(byte playlistIndex, char *directoryPath, size_t directoryPathSize)
{
    if (playlistIndex >= MaxPlaylistCount || directoryPath == nullptr || directoryPathSize == 0)
    {
        Serial.printf("Set lists: invalid directory lookup for playlist %u.\n", static_cast<unsigned int>(playlistIndex));
        return false;
    }

    const char *path = nullptr;
    switch (playlistIndex)
    {
    case Project7PlaylistIndex:
        path = "/sets/p7";
        break;
    case OprPlaylistIndex:
        path = "/sets/opr";
        break;
    case CodeRedPlaylistIndex:
        path = "/sets/cr";
        break;
    default:
        break;
    }

    if (path == nullptr)
    {
        Serial.printf("Set lists: playlist %u has no configured set directory.\n", static_cast<unsigned int>(playlistIndex));
        return false;
    }
    safeCopy(directoryPath, directoryPathSize, path);
    safeCopy(directoryPath, directoryPathSize, path);
    return true;
}

void SetListStore::sortParts(SetListPart *parts, size_t count)
{
    sortAscending(parts, count, partCompare);
}

void SetListStore::sortSongs(SetListSongEntry *songs, size_t count)
{
    sortAscending(songs, count, songCompare);
}

void SetListStore::safeCopy(char *destination, size_t destinationSize, const char *source)
{
    if (destination == nullptr || destinationSize == 0)
    {
        return;
    }

    if (source == nullptr)
    {
        source = "";
    }

    strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

const char *SetListStore::fileNameFromPath(const char *path)
{
    if (path == nullptr)
    {
        return "";
    }

    const char *fileName = strrchr(path, '/');
    return fileName == nullptr ? path : fileName + 1;
}
