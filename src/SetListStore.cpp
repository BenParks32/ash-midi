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
        Serial.printf("Set lists: list skipped for playlist %u (summaries=%s, max=%u).\n",
                      static_cast<unsigned int>(playlistIndex), summaries != nullptr ? "set" : "null",
                      static_cast<unsigned int>(maxSummaries));
        return 0;
    }

    char directoryPath[TextFilePathEntry::MaxPathLength];
    if (!directoryForPlaylist(playlistIndex, directoryPath, sizeof(directoryPath)))
    {
        Serial.printf("Set lists: no directory mapping for playlist %u.\n", static_cast<unsigned int>(playlistIndex));
        return 0;
    }

    Serial.printf("Set lists: listing playlist %u from '%s'.\n", static_cast<unsigned int>(playlistIndex), directoryPath);

    TextFilePathEntry entries[ActiveSetList::MaxSongs];
    const size_t entryCount = _directoryStore.listTextFiles(directoryPath, entries, ActiveSetList::MaxSongs);
    Serial.printf("Set lists: directory '%s' returned %u candidate file(s).\n", directoryPath,
                  static_cast<unsigned int>(entryCount));

    size_t loadedCount = 0;
    for (size_t i = 0; i < entryCount && loadedCount < maxSummaries; ++i)
    {
        Serial.printf("Set lists: loading summary from '%s'.\n", entries[i].path);
        if (!loadSetListSummary(entries[i].path, summaries[loadedCount]))
        {
            Serial.printf("Set lists: failed to load summary from '%s'.\n", entries[i].path);
            continue;
        }

        Serial.printf("Set lists: summary[%u] name='%s' file='%s' songs=%u parts=%u.\n",
                      static_cast<unsigned int>(loadedCount), summaries[loadedCount].name, summaries[loadedCount].fileName,
                      static_cast<unsigned int>(summaries[loadedCount].songCount),
                      static_cast<unsigned int>(summaries[loadedCount].partCount));
        ++loadedCount;
    }

    Serial.printf("Set lists: playlist %u loaded %u visible set list(s).\n", static_cast<unsigned int>(playlistIndex),
                  static_cast<unsigned int>(loadedCount));
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

    Serial.printf("Set lists: activating '%s' for playlist %u.\n", fullPath, static_cast<unsigned int>(playlistIndex));
    PlaylistState &state = _playlistStates[playlistIndex];
    state.setList = {};
    if (!loadSetList(playlistIndex, fullPath, state.setList))
    {
        state.hasActiveSet = false;
        Serial.printf("Set lists: failed to activate '%s'.\n", fullPath);
        return false;
    }

    state.hasActiveSet = true;
    Serial.printf("Set lists: active set for playlist %u is now '%s'.\n", static_cast<unsigned int>(playlistIndex),
                  state.setList.name);
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
    Serial.printf("Set lists: cleared active set for playlist %u.\n", static_cast<unsigned int>(playlistIndex));
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
        DynamicJsonDocument document(FullSetListJsonCapacity);
        if (document.capacity() == 0)
        {
            Serial.printf("Set lists: failed to allocate %u-byte JSON document for '%s'.\n",
                          static_cast<unsigned int>(FullSetListJsonCapacity), path != nullptr ? path : "<null>");
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
    Serial.printf("Set lists: loaded '%s' with %u part(s), %u song(s), %u resolved, %u unresolved.\n",
                  path != nullptr ? path : "<null>", static_cast<unsigned int>(setList.partCount),
                  static_cast<unsigned int>(setList.songCount), static_cast<unsigned int>(resolvedSongCount),
                  static_cast<unsigned int>(unresolvedSongCount));
    return true;
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

    StaticJsonDocument<JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + (2 * JSON_OBJECT_SIZE(1))> filter;
    filter["name"] = true;
    filter["parts"][0]["part"] = true;
    filter["songs"][0]["songId"] = true;

    DynamicJsonDocument document(SummaryJsonCapacity);
    if (document.capacity() == 0)
    {
        Serial.printf("Set lists: failed to allocate %u-byte summary JSON document for '%s'.\n",
                      static_cast<unsigned int>(SummaryJsonCapacity), path != nullptr ? path : "<null>");
        free(jsonBuffer);
        return false;
    }

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
    summary.partCount = document["parts"].is<JsonArray>() ? document["parts"].as<JsonArray>().size() : 0;
    summary.songCount = document["songs"].is<JsonArray>() ? document["songs"].as<JsonArray>().size() : 0;
    Serial.printf("Set lists: parsed summary '%s' from '%s'.\n", summary.name, path != nullptr ? path : "<null>");
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
    Serial.printf("Set lists: playlist %u directory is '%s'.\n", static_cast<unsigned int>(playlistIndex), directoryPath);
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
