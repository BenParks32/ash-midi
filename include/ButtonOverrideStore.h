#pragma once

#include "Function.h"
#include "encoding/MidiConfigLoader.h"
#include "encoding/SongCatalogueLoader.h"
#include "TextFileStore.h"

#include <Arduino.h>
#include <stddef.h>

constexpr byte Project7PlaylistIndex = 2;
constexpr byte OprPlaylistIndex = 3;
constexpr byte CodeRedPlaylistIndex = 4;
constexpr size_t PlaylistIndexCapacity = static_cast<size_t>(CodeRedPlaylistIndex) + 1U;
constexpr size_t SongIdCapacity = 40;

struct PatchDisplayConfig
{
    static constexpr size_t NameCapacity = 48;

    char name[NameCapacity];

    PatchDisplayConfig() : name{0} {}
};

struct PatchListEntry
{
    byte patchNumber;
    char name[PatchDisplayConfig::NameCapacity];

    PatchListEntry() : patchNumber(0), name{0} {}
};

struct SongListEntry
{
    byte songIndex;
    byte patchNumber;
    char name[PatchDisplayConfig::NameCapacity];
    char id[SongIdCapacity + 1];

    SongListEntry() : songIndex(0), patchNumber(0), name{0}, id{0} {}
};

struct SongConfig
{
    byte patchNumber;
    char name[PatchDisplayConfig::NameCapacity];
    char displayName[PatchDisplayConfig::NameCapacity];
    char id[SongIdCapacity + 1];

    SongConfig() : patchNumber(0), name{0}, displayName{0}, id{0} {}
};

struct SongNotes
{
    static constexpr size_t MaxLines = 4;
    static constexpr size_t LineCapacity = 64;

    char lines[MaxLines][LineCapacity];
    uint8_t lineCount;

    SongNotes() : lines{}, lineCount(0) {}
};

class IButtonOverrideStore
{
  public:
    virtual ~IButtonOverrideStore() = default;

    virtual bool refresh() = 0;
    virtual void applyOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount,
                                PatchDisplayConfig* patchDisplay = nullptr) const = 0;
    virtual size_t listPatches(byte playlistIndex, PatchListEntry* entries, size_t capacity) const
    {
        (void)playlistIndex;
        (void)entries;
        (void)capacity;
        return 0;
    }
    virtual size_t listSongs(byte playlistIndex, SongListEntry* entries, size_t capacity) const
    {
        (void)playlistIndex;
        (void)entries;
        (void)capacity;
        return 0;
    }
    virtual bool songForIndex(byte playlistIndex, byte songIndex, SongConfig* outSong) const
    {
        (void)playlistIndex;
        (void)songIndex;
        (void)outSong;
        return false;
    }
    virtual bool songNotesForIndex(byte playlistIndex, byte songIndex, SongNotes* outNotes) const
    {
        (void)playlistIndex;
        (void)songIndex;
        (void)outNotes;
        return false;
    }
    virtual bool songForId(byte playlistIndex, const char* songId, byte& songIndex, SongConfig& outSong) const
    {
        (void)playlistIndex;
        (void)songId;
        (void)songIndex;
        (void)outSong;
        return false;
    }
};

class ButtonOverrideStore : public IButtonOverrideStore
{
  public:
    explicit ButtonOverrideStore(ITextFileStore* fileStore = nullptr);
    ~ButtonOverrideStore();

    bool refresh() override;
    void applyOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount,
                        PatchDisplayConfig* patchDisplay = nullptr) const override;
    size_t listPatches(byte playlistIndex, PatchListEntry* entries, size_t capacity) const override;
    size_t listSongs(byte playlistIndex, SongListEntry* entries, size_t capacity) const override;
    bool songForIndex(byte playlistIndex, byte songIndex, SongConfig* outSong) const override;
    bool songNotesForIndex(byte playlistIndex, byte songIndex, SongNotes* outNotes) const override;
    bool songForId(byte playlistIndex, const char* songId, byte& songIndex, SongConfig& outSong) const override;

  private:
    struct ParsedActionOverride
    {
        bool isDefined;
        FunctionAction action;
    };

    struct ParsedButtonOverride
    {
        bool hasLabel;
        char label[Function::LabelCapacity];
        bool hasColour;
        uint16_t colour;
        bool hasToggle;
        bool toggle;
        ParsedActionOverride actions[static_cast<uint8_t>(FunctionBehaviour::Count)];
    };

    struct PatchOverride
    {
        byte playlistIndex;
        byte patchNumber;
        bool hasButton[8];
        ParsedButtonOverride buttons[8];
    };

  private:
    static bool playlistIndexForModeKey(const char* key, byte& outPlaylistIndex);
    static const char* modeKeyForPlaylistIndex(byte playlistIndex);
    static bool parseColourValue(const char* rawValue, uint16_t& outColour);
    static bool parseActionName(const char* name, ActionType& outActionType);
    static const char* songsConfigPathForPlaylist(byte playlistIndex);
    static void clearButtonOverride(ParsedButtonOverride& buttonOverride);
    static void applyButtonOverride(const ParsedButtonOverride& buttonOverride, Function& target);
    bool loadSongCatalogueForPlaylist(byte playlistIndex, SongCatalogue& outCatalogue, const char*& loadedPath) const;

    void clearOverrides();
    bool applyBinaryOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount,
                              PatchDisplayConfig* patchDisplay) const;
    size_t listBinaryPatches(byte playlistIndex, PatchListEntry* entries, size_t capacity) const;

  private:
    ITextFileStore* _fileStore;
    MidiConfigRuntime _binaryConfig = {};
    bool _hasBinaryConfig = false;
};
