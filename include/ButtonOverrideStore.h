#pragma once

#include "Function.h"
#include "TextFileStore.h"

#include <ArduinoJson.h>
#include <Arduino.h>
#include <stddef.h>

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

  private:
    static constexpr const char* kPrimaryConfigPath = "/BUTTONS.JSN";
    static constexpr const char* kLegacyConfigPath = "/buttons.json";
    static constexpr size_t kMaxConfigBytes = 4096;

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
    static bool parsePatchNumber(const char* key, byte& outPatchNumber);
    static bool parseButtonIndex(const char* key, byte& outButtonIndex);
    static bool parseColourValue(const char* rawValue, uint16_t& outColour);
    static bool parseActionName(const char* name, ActionType& outActionType);
    static bool parseModeValue(const char* name, byte& outModeValue);
    static bool parseActionObject(const JsonVariantConst& actionValue, FunctionAction& outAction);
    static void clearButtonOverride(ParsedButtonOverride& buttonOverride);
    static void applyButtonOverride(const ParsedButtonOverride& buttonOverride, Function& target);
    static bool parseButtonOverrideObject(const JsonObjectConst& buttonObject, ParsedButtonOverride& outButtonOverride);

    void clearOverrides();

  private:
    ITextFileStore* _fileStore;
    char* _configBuffer;
};
