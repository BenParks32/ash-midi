#pragma once

#include <Arduino.h>

#include "BinaryFileStore.h"
#include "encoding/SetListLoader.h"
#include "TextDirectoryStore.h"

struct SetListResolvedSong
{
    static constexpr size_t MaxNameLength = 32;

    char name[MaxNameLength];
    char displayName[MaxNameLength];
    bool hasDisplayName = false;
    byte patch = 0;
};

class ISetListSongResolver
{
public:
    virtual ~ISetListSongResolver() = default;

    virtual bool resolveSong(byte playlistIndex,
                             const char *songId,
                             byte &songIndex,
                             SetListResolvedSong &song) const = 0;
};

struct SetListPart
{
    static constexpr size_t MaxNameLength = 20;

    uint16_t part = 0;
    char name[MaxNameLength];
};

struct SetListSongEntry
{
    static constexpr size_t MaxSongIdLength = 41;
    static constexpr size_t MaxNameLength = 32;

    uint16_t number = 0;
    uint16_t part = 0;
    char songId[MaxSongIdLength];
    char name[MaxNameLength];
    bool available = false;
    byte songIndex = 0;
    byte patch = 0;
};

struct SetListSummary
{
    static constexpr size_t MaxNameLength = 32;
    static constexpr size_t MaxFileNameLength = 48;

    char name[MaxNameLength];
    char fileName[MaxFileNameLength];
    size_t partCount = 0;
    size_t songCount = 0;
};

struct ActiveSetList
{
    static constexpr size_t MaxNameLength = 32;
    static constexpr size_t MaxPathLength = TextFilePathEntry::MaxPathLength;
    static constexpr size_t MaxParts = 8;
    static constexpr size_t MaxSongs = 64;

    char name[MaxNameLength];
    char sourcePath[MaxPathLength];
    SetListPart parts[MaxParts];
    size_t partCount = 0;
    SetListSongEntry songs[MaxSongs];
    size_t songCount = 0;
    size_t selectedSongIndex = 0;
};

class ISetListStore
{
public:
    virtual ~ISetListStore() = default;

    virtual size_t listSetLists(byte playlistIndex,
                                SetListSummary *summaries,
                                size_t maxSummaries) const = 0;
    virtual bool activateSetList(byte playlistIndex, const char *fileName) = 0;
    virtual bool clearActiveSetList(byte playlistIndex) = 0;
    virtual bool activeSetList(byte playlistIndex, ActiveSetList &setList) const = 0;
    virtual bool activeSetSummary(byte playlistIndex, SetListSummary &summary) const = 0;
    virtual bool activeSetPosition(byte playlistIndex, size_t &songCount, size_t &selectedSongIndex) const = 0;
    virtual bool selectSong(byte playlistIndex, size_t setSongIndex) = 0;
    virtual bool selectedSong(byte playlistIndex, SetListSongEntry &song) const = 0;
};

class SetListStore final : public ISetListStore
{
public:
    static constexpr size_t MaxPlaylistCount = 8;

    SetListStore(const IBinaryFileStore &binaryFileStore,
                 const ITextDirectoryStore &directoryStore,
                 const ISetListSongResolver &songResolver);

    size_t listSetLists(byte playlistIndex,
                        SetListSummary *summaries,
                        size_t maxSummaries) const override;
    bool activateSetList(byte playlistIndex, const char *fileName) override;
    bool clearActiveSetList(byte playlistIndex) override;
    bool activeSetList(byte playlistIndex, ActiveSetList &setList) const override;
    bool activeSetSummary(byte playlistIndex, SetListSummary &summary) const override;
    bool activeSetPosition(byte playlistIndex, size_t &songCount, size_t &selectedSongIndex) const override;
    bool selectSong(byte playlistIndex, size_t setSongIndex) override;
    bool selectedSong(byte playlistIndex, SetListSongEntry &song) const override;

private:
    const IBinaryFileStore &_binaryFileStore;
    const ITextDirectoryStore &_directoryStore;
    const ISetListSongResolver &_songResolver;
    mutable TextFilePathEntry _listEntries[ActiveSetList::MaxSongs];
    bool _hasActiveSet = false;
    byte _activePlaylistIndex = 0;
    ActiveSetList _activeSet;

    bool loadSetList(byte playlistIndex, const char *path, ActiveSetList &setList) const;
    bool loadSetListSummary(const char *path, SetListSummary &summary) const;
    static bool isSetListPath(const char* path);
    static bool directoryForPlaylist(byte playlistIndex, char *directoryPath, size_t directoryPathSize);
    static void sortParts(SetListPart *parts, size_t count);
    static void sortSongs(SetListSongEntry *songs, size_t count);
    static void safeCopy(char *destination, size_t destinationSize, const char *source);
    static const char *fileNameFromPath(const char *path);
};
