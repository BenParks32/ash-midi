#include "ButtonOverrideStore.h"

#include <ArduinoJson.h>
#include <cstdlib>
#include <cstring>

namespace
{
constexpr uint8_t Project7PlaylistIndex = 2;
constexpr uint8_t OprPlaylistIndex = 3;
constexpr uint8_t CodeRedPlaylistIndex = 4;
constexpr uint8_t MaxButtonIndex = 7;
constexpr uint8_t MaxPatchIndex = 127;
constexpr size_t JsonCapacityMultiplier = 2;
constexpr size_t JsonCapacityPadding = 512;

const char* playlistName(byte playlistIndex)
{
    switch (playlistIndex)
    {
    case Project7PlaylistIndex:
        return "Project7";
    case OprPlaylistIndex:
        return "OPR";
    case CodeRedPlaylistIndex:
        return "CodeRed";
    default:
        return "Unknown";
    }
}

const char* configPathCandidates[] = {"/BUTTONS.JSN", "BUTTONS.JSN", "/buttons.json", "buttons.json"};
}

ButtonOverrideStore::ButtonOverrideStore(ITextFileStore* fileStore)
    : _fileStore(fileStore), _configBuffer(nullptr)
{
}

ButtonOverrideStore::~ButtonOverrideStore() { clearOverrides(); }

bool ButtonOverrideStore::refresh()
{
    clearOverrides();

    if (_fileStore == nullptr)
    {
        Serial.println("Button overrides: no file store configured.");
        return false;
    }

    char* buffer = new char[kMaxConfigBytes];
    if (buffer == nullptr)
    {
        Serial.println("Button overrides: failed to allocate config buffer.");
        return false;
    }

    const char* loadedPath = nullptr;
    bool didReadConfig = false;
    for (size_t pathIndex = 0; pathIndex < (sizeof(configPathCandidates) / sizeof(configPathCandidates[0])); ++pathIndex)
    {
        if (_fileStore->readTextFile(configPathCandidates[pathIndex], buffer, kMaxConfigBytes))
        {
            loadedPath = configPathCandidates[pathIndex];
            didReadConfig = true;
            break;
        }
    }

    if (!didReadConfig)
    {
        delete[] buffer;
        Serial.printf("Button overrides: config file unavailable. Tried '%s', '%s', '%s', '%s'.\n",
                      configPathCandidates[0], configPathCandidates[1], configPathCandidates[2],
                      configPathCandidates[3]);
        return false;
    }

    Serial.printf("Button overrides: loaded config from '%s'.\n", loadedPath);

    DynamicJsonDocument document((kMaxConfigBytes * JsonCapacityMultiplier) + JsonCapacityPadding);
    const char* jsonText = buffer;
    const DeserializationError error = deserializeJson(document, jsonText);

    if (error)
    {
        delete[] buffer;
        Serial.printf("Button overrides: JSON parse failed: %s\n", error.c_str());
        return false;
    }

    const JsonObjectConst playModes = document["playModes"].as<JsonObjectConst>();
    if (playModes.isNull())
    {
        delete[] buffer;
        Serial.println("Button overrides: missing playModes object.");
        return false;
    }

    _configBuffer = buffer;
    Serial.println("Button overrides: config validated.");
    return true;
}

void ButtonOverrideStore::applyOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount) const
{
    if (functions == nullptr || functionCount == 0 || _configBuffer == nullptr)
    {
        if (_configBuffer == nullptr)
        {
            Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                          static_cast<unsigned int>(patchNumber));
        }
        return;
    }

    DynamicJsonDocument document((kMaxConfigBytes * JsonCapacityMultiplier) + JsonCapacityPadding);
    const char* jsonText = _configBuffer;
    const DeserializationError error = deserializeJson(document, jsonText);
    if (error)
    {
        Serial.printf("Button overrides: JSON reparse failed: %s\n", error.c_str());
        return;
    }

    const char* modeKey = modeKeyForPlaylistIndex(playlistIndex);
    if (modeKey == nullptr)
    {
        Serial.printf("Button overrides: no mode key for playlist %u.\n", static_cast<unsigned int>(playlistIndex));
        return;
    }

    char patchKey[4] = {'\0'};
    std::snprintf(patchKey, sizeof(patchKey), "%u", static_cast<unsigned int>(patchNumber));

    const JsonObjectConst playModes = document["playModes"].as<JsonObjectConst>();
    const JsonObjectConst modeObject = playModes[modeKey].as<JsonObjectConst>();
    const JsonObjectConst patches = modeObject["patches"].as<JsonObjectConst>();
    const JsonObjectConst patchObject = patches[patchKey].as<JsonObjectConst>();
    const JsonObjectConst buttons = patchObject["buttons"].as<JsonObjectConst>();
    if (playModes.isNull() || modeObject.isNull() || patches.isNull() || patchObject.isNull() || buttons.isNull())
    {
        Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(patchNumber));
        return;
    }

    uint8_t appliedCount = 0;
    for (JsonPairConst buttonPair : buttons)
    {
        byte buttonIndex = 0;
        if (!parseButtonIndex(buttonPair.key().c_str(), buttonIndex))
        {
            Serial.printf("Button overrides: invalid button index '%s'.\n", buttonPair.key().c_str());
            continue;
        }

        if (buttonIndex >= functionCount)
        {
            continue;
        }

        ParsedButtonOverride parsedButton = {};
        clearButtonOverride(parsedButton);
        const JsonObjectConst buttonObject = buttonPair.value().as<JsonObjectConst>();
        if (buttonObject.isNull())
        {
            Serial.printf("Button overrides: button %u override is not an object.\n",
                          static_cast<unsigned int>(buttonIndex + 1U));
            continue;
        }

        if (!parseButtonOverrideObject(buttonObject, parsedButton))
        {
            Serial.printf("Button overrides: invalid button override for %s patch %u button %u.\n",
                          playlistName(playlistIndex), static_cast<unsigned int>(patchNumber),
                          static_cast<unsigned int>(buttonIndex + 1U));
            continue;
        }

        applyButtonOverride(parsedButton, functions[buttonIndex]);
        ++appliedCount;
        Serial.printf("Button overrides: applied override for %s patch %u button %u.\n",
                      playlistName(playlistIndex), static_cast<unsigned int>(patchNumber),
                      static_cast<unsigned int>(buttonIndex + 1U));
    }

    if (appliedCount == 0)
    {
        Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(patchNumber));
    }
}

bool ButtonOverrideStore::playlistIndexForModeKey(const char* key, byte& outPlaylistIndex)
{
    if (key == nullptr)
    {
        return false;
    }

    if (std::strcmp(key, "Project7") == 0)
    {
        outPlaylistIndex = Project7PlaylistIndex;
        return true;
    }

    if (std::strcmp(key, "OPR") == 0)
    {
        outPlaylistIndex = OprPlaylistIndex;
        return true;
    }

    if (std::strcmp(key, "CodeRed") == 0)
    {
        outPlaylistIndex = CodeRedPlaylistIndex;
        return true;
    }

    return false;
}

const char* ButtonOverrideStore::modeKeyForPlaylistIndex(byte playlistIndex)
{
    switch (playlistIndex)
    {
    case Project7PlaylistIndex:
        return "Project7";
    case OprPlaylistIndex:
        return "OPR";
    case CodeRedPlaylistIndex:
        return "CodeRed";
    default:
        return nullptr;
    }
}

bool ButtonOverrideStore::parsePatchNumber(const char* key, byte& outPatchNumber)
{
    if (key == nullptr || *key == '\0')
    {
        return false;
    }

    char* end = nullptr;
    const unsigned long value = std::strtoul(key, &end, 10);
    if (end == nullptr || *end != '\0' || value > MaxPatchIndex)
    {
        return false;
    }

    outPatchNumber = static_cast<byte>(value);
    return true;
}

bool ButtonOverrideStore::parseButtonIndex(const char* key, byte& outButtonIndex)
{
    if (key == nullptr || *key == '\0')
    {
        return false;
    }

    char* end = nullptr;
    const unsigned long value = std::strtoul(key, &end, 10);
    if (end == nullptr || *end != '\0' || value > MaxButtonIndex)
    {
        return false;
    }

    outButtonIndex = static_cast<byte>(value);
    return true;
}

bool ButtonOverrideStore::parseColourValue(const char* rawValue, uint16_t& outColour)
{
    if (rawValue == nullptr || *rawValue == '\0')
    {
        return false;
    }

    const char* colourText = rawValue;
    if (*colourText == '#')
    {
        ++colourText;
    }

    char* end = nullptr;
    const unsigned long value = std::strtoul(colourText, &end, 16);
    if (end == nullptr || *end != '\0' || value > 0xFFFFUL)
    {
        return false;
    }

    outColour = static_cast<uint16_t>(value);
    return true;
}

bool ButtonOverrideStore::parseActionName(const char* name, ActionType& outActionType)
{
    if (name == nullptr)
    {
        return false;
    }

    if (std::strcmp(name, "None") == 0)
    {
        outActionType = ActionType::None;
        return true;
    }
    if (std::strcmp(name, "SendMidiProgramChange") == 0)
    {
        outActionType = ActionType::SendMidiProgramChange;
        return true;
    }
    if (std::strcmp(name, "SendMidiControlChange") == 0)
    {
        outActionType = ActionType::SendMidiControlChange;
        return true;
    }
    if (std::strcmp(name, "ChangeMode") == 0)
    {
        outActionType = ActionType::ChangeMode;
        return true;
    }
    if (std::strcmp(name, "SelectScene") == 0)
    {
        outActionType = ActionType::SelectScene;
        return true;
    }
    if (std::strcmp(name, "SetTuner") == 0)
    {
        outActionType = ActionType::SetTuner;
        return true;
    }
    if (std::strcmp(name, "SelectHomePlaylist") == 0)
    {
        outActionType = ActionType::SelectHomePlaylist;
        return true;
    }
    if (std::strcmp(name, "TapTempo") == 0)
    {
        outActionType = ActionType::TapTempo;
        return true;
    }
    if (std::strcmp(name, "SetGigView") == 0)
    {
        outActionType = ActionType::SetGigView;
        return true;
    }

    return false;
}

bool ButtonOverrideStore::parseModeValue(const char* name, byte& outModeValue)
{
    if (name == nullptr)
    {
        return false;
    }

    if (std::strcmp(name, "Home") == 0)
    {
        outModeValue = 0;
        return true;
    }
    if (std::strcmp(name, "Play") == 0)
    {
        outModeValue = 1;
        return true;
    }
    if (std::strcmp(name, "Patch") == 0)
    {
        outModeValue = 2;
        return true;
    }
    if (std::strcmp(name, "Menu") == 0)
    {
        outModeValue = 3;
        return true;
    }
    if (std::strcmp(name, "ButtonDiagnostic") == 0)
    {
        outModeValue = 4;
        return true;
    }

    return false;
}

bool ButtonOverrideStore::parseActionObject(const JsonVariantConst& actionValue, FunctionAction& outAction)
{
    if (actionValue.isNull())
    {
        return false;
    }

    ActionType actionType = ActionType::None;
    byte value = 0;
    uint8_t secondaryValue = 0;

    if (actionValue.is<const char*>())
    {
        const char* actionName = actionValue.as<const char*>();
        if (!parseActionName(actionName, actionType))
        {
            return false;
        }
    }
    else
    {
        const JsonObjectConst actionObject = actionValue.as<JsonObjectConst>();
        if (actionObject.isNull())
        {
            return false;
        }

        const char* actionName = actionObject["action"].as<const char*>();
        if (!parseActionName(actionName, actionType))
        {
            return false;
        }

        if (actionType == ActionType::ChangeMode && actionObject["value"].is<const char*>())
        {
            if (!parseModeValue(actionObject["value"].as<const char*>(), value))
            {
                return false;
            }
        }
        else if (actionType == ActionType::SelectHomePlaylist && actionObject["value"].is<const char*>())
        {
            if (!playlistIndexForModeKey(actionObject["value"].as<const char*>(), value))
            {
                return false;
            }
        }
        else if (!actionObject["value"].isNull())
        {
            const unsigned int parsedValue = actionObject["value"].as<unsigned int>();
            if (parsedValue > 0xFFU)
            {
                return false;
            }
            value = static_cast<byte>(parsedValue);
        }

        if (!actionObject["secondaryValue"].isNull())
        {
            const unsigned int parsedSecondaryValue = actionObject["secondaryValue"].as<unsigned int>();
            if (parsedSecondaryValue > 0xFFU)
            {
                return false;
            }
            secondaryValue = static_cast<uint8_t>(parsedSecondaryValue);
        }
        else if (actionType == ActionType::SendMidiControlChange)
        {
            secondaryValue = 127;
        }
    }

    outAction = FunctionAction(actionType, value, secondaryValue);
    return true;
}

void ButtonOverrideStore::clearButtonOverride(ParsedButtonOverride& buttonOverride)
{
    buttonOverride.hasLabel = false;
    buttonOverride.label[0] = '\0';
    buttonOverride.hasColour = false;
    buttonOverride.colour = 0;

    for (uint8_t behaviourIndex = 0; behaviourIndex < static_cast<uint8_t>(FunctionBehaviour::Count); ++behaviourIndex)
    {
        buttonOverride.actions[behaviourIndex].isDefined = false;
        buttonOverride.actions[behaviourIndex].action = FunctionAction();
    }
}

void ButtonOverrideStore::applyButtonOverride(const ParsedButtonOverride& buttonOverride, Function& target)
{
    if (buttonOverride.hasLabel)
    {
        target.setLabel(buttonOverride.label);
    }

    if (buttonOverride.hasColour)
    {
        target.setColour(buttonOverride.colour);
    }

    for (uint8_t behaviourIndex = 0; behaviourIndex < static_cast<uint8_t>(FunctionBehaviour::Count); ++behaviourIndex)
    {
        if (!buttonOverride.actions[behaviourIndex].isDefined)
        {
            continue;
        }

        target.setAction(static_cast<FunctionBehaviour>(behaviourIndex), buttonOverride.actions[behaviourIndex].action);
    }
}

bool ButtonOverrideStore::parseButtonOverrideObject(const JsonObjectConst& buttonObject,
                                                    ParsedButtonOverride& outButtonOverride)
{
    if (!buttonObject["label"].isNull())
    {
        const char* label = buttonObject["label"].as<const char*>();
        if (label == nullptr)
        {
            return false;
        }

        outButtonOverride.hasLabel = true;
        std::strncpy(outButtonOverride.label, label, Function::LabelCapacity - 1U);
        outButtonOverride.label[Function::LabelCapacity - 1U] = '\0';
    }

    if (!buttonObject["colour"].isNull())
    {
        uint16_t colour = 0;
        if (buttonObject["colour"].is<unsigned int>())
        {
            const unsigned int numericColour = buttonObject["colour"].as<unsigned int>();
            if (numericColour > 0xFFFFU)
            {
                return false;
            }

            colour = static_cast<uint16_t>(numericColour);
        }
        else if (buttonObject["colour"].is<const char*>())
        {
            if (!parseColourValue(buttonObject["colour"].as<const char*>(), colour))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        outButtonOverride.hasColour = true;
        outButtonOverride.colour = colour;
    }

    const JsonObjectConst functionObject = buttonObject["function"].as<JsonObjectConst>();
    if (!functionObject.isNull())
    {
        const char* keys[] = {"press", "release", "shortPress", "longPress"};
        const FunctionBehaviour behaviours[] = {FunctionBehaviour::ButtonDown, FunctionBehaviour::ButtonRelease,
                                                FunctionBehaviour::ShortPress, FunctionBehaviour::LongPress};
        uint8_t parsedActionCount = 0;
        uint8_t configuredActionCount = 0;

        for (uint8_t keyIndex = 0; keyIndex < 4; ++keyIndex)
        {
            const JsonVariantConst actionValue = functionObject[keys[keyIndex]];
            if (actionValue.isNull())
            {
                continue;
            }

            ++configuredActionCount;
            FunctionAction parsedAction = {};
            if (!parseActionObject(actionValue, parsedAction))
            {
                Serial.printf("Button overrides: invalid '%s' action in button function override.\n", keys[keyIndex]);
                continue;
            }

            const uint8_t behaviourIndex = static_cast<uint8_t>(behaviours[keyIndex]);
            outButtonOverride.actions[behaviourIndex].isDefined = true;
            outButtonOverride.actions[behaviourIndex].action = parsedAction;
            ++parsedActionCount;
        }

        if (configuredActionCount > 0 && parsedActionCount == 0)
        {
            Serial.println("Button overrides: function override contained no valid actions.");
        }
    }

    return true;
}

void ButtonOverrideStore::clearOverrides()
{
    delete[] _configBuffer;
    _configBuffer = nullptr;
}
