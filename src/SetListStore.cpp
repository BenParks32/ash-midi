#include "SetListStore.h"

#include "ButtonOverrideStore.h"

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

bool hasSetListExtension(const char* path)
{
    if (path == nullptr)
    {
        return false;
    }

    const char* extension = strrchr(path, '.');
    if (extension == nullptr)
    {
        return false;
    }

    return strcmp(extension, ".msl") == 0 || strcmp(extension, ".MSL") == 0;
}

} // namespace

SetListStore::SetListStore(const IBinaryFileStore &binaryFileStore,
                           const ITextDirectoryStore &directoryStore,
                           const ISetListSongResolver &songResolver)
    : _binaryFileStore(binaryFileStore), _directoryStore(directoryStore), _songResolver(songResolver)
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
        if (!isSetListPath(entries[i].path))
        {
            continue;
        }

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

    if (!isSetListPath(fullPath))
    {
        Serial.printf("Set lists: unsupported set list file '%s'.\n", fullPath);
        return false;
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
    uint8_t* buffer = nullptr;
    size_t size = 0U;
    if (!_binaryFileStore.readBinaryFile(path, buffer, size))
    {
        Serial.printf("Set lists: failed to read '%s' for playlist %u.\n", path != nullptr ? path : "<null>",
                      static_cast<unsigned int>(playlistIndex));
        return false;
    }

    SetListCatalogue catalogue;
    if (!loadSetListCatalogue(buffer, size, catalogue))
    {
        Serial.printf("Set lists: failed to parse binary set list '%s'.\n", path != nullptr ? path : "<null>");
        return false;
    }

    if (catalogue.header->partCount > ActiveSetList::MaxParts || catalogue.header->songCount > ActiveSetList::MaxSongs)
    {
        Serial.printf("Set lists: '%s' exceeds capacity (parts=%u songs=%u).\n", path != nullptr ? path : "<null>",
                      static_cast<unsigned int>(catalogue.header->partCount),
                      static_cast<unsigned int>(catalogue.header->songCount));
        freeSetListCatalogue(catalogue);
        return false;
    }

    const char* setName = mslt_get_string(catalogue, catalogue.header->nameIndex);
    safeCopy(setList.name, sizeof(setList.name), setName[0] != '\0' ? setName : fileNameFromPath(path));
    safeCopy(setList.sourcePath, sizeof(setList.sourcePath), path);

    for (uint16_t partIndex = 0; partIndex < catalogue.header->partCount; ++partIndex)
    {
        const MSLT_Part* partRecord = mslt_get_part(catalogue, partIndex);
        if (partRecord == nullptr)
        {
            Serial.printf("Set lists: invalid part index %u in '%s'.\n", static_cast<unsigned int>(partIndex),
                          path != nullptr ? path : "<null>");
            freeSetListCatalogue(catalogue);
            return false;
        }

        SetListPart &part = setList.parts[setList.partCount++];
        part.part = partRecord->part;
        safeCopy(part.name, sizeof(part.name), mslt_get_string(catalogue, partRecord->nameIndex));
    }
    sortParts(setList.parts, setList.partCount);

    for (uint16_t songIndex = 0; songIndex < catalogue.header->songCount; ++songIndex)
    {
        const MSLT_Song* songRecord = mslt_get_song(catalogue, songIndex);
        if (songRecord == nullptr)
        {
            Serial.printf("Set lists: invalid song index %u in '%s'.\n", static_cast<unsigned int>(songIndex),
                          path != nullptr ? path : "<null>");
            freeSetListCatalogue(catalogue);
            return false;
        }

        SetListSongEntry &song = setList.songs[setList.songCount++];
        song.number = songRecord->number;
        song.part = songRecord->part;
        safeCopy(song.songId, sizeof(song.songId), mslt_get_string(catalogue, songRecord->songIdIndex));
    }

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
    Serial.printf("Set lists: loaded '%s' with %u parts and %u songs (%u resolved, %u unresolved).\n",
                  path != nullptr ? path : "<null>", static_cast<unsigned int>(setList.partCount),
                  static_cast<unsigned int>(setList.songCount), static_cast<unsigned int>(resolvedSongCount),
                  static_cast<unsigned int>(unresolvedSongCount));
    freeSetListCatalogue(catalogue);
    return true;
}

bool SetListStore::loadSetListSummary(const char *path, SetListSummary &summary) const
{
    uint8_t* buffer = nullptr;
    size_t size = 0U;
    if (!_binaryFileStore.readBinaryFile(path, buffer, size))
    {
        Serial.printf("Set lists: summary read failed for '%s'.\n", path != nullptr ? path : "<null>");
        return false;
    }

    SetListCatalogue catalogue;
    if (!loadSetListCatalogue(buffer, size, catalogue))
    {
        Serial.printf("Set lists: summary parse failed for '%s'.\n", path != nullptr ? path : "<null>");
        return false;
    }

    summary.partCount = catalogue.header->partCount;
    summary.songCount = catalogue.header->songCount;
    const char* summaryName = mslt_get_string(catalogue, catalogue.header->nameIndex);
    safeCopy(summary.name, sizeof(summary.name), summaryName[0] != '\0' ? summaryName : fileNameFromPath(path));
    safeCopy(summary.fileName, sizeof(summary.fileName), fileNameFromPath(path));
    freeSetListCatalogue(catalogue);
    return true;
}

bool SetListStore::isSetListPath(const char* path)
{
    return hasSetListExtension(path);
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
