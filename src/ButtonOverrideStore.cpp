#include "ButtonOverrideStore.h"

#include <cstdlib>
#include <cstring>

namespace
{
const char* binaryConfigPathCandidates[] = {"/buttons.mcfg", "buttons.mcfg", "/BUTTONS.MCFG", "BUTTONS.MCFG",
                                            "/BUTTONS~1.MCF", "BUTTONS~1.MCF", "/BUTTONS.MCF", "BUTTONS.MCF"};
const char* project7SongPathCandidates[] = {"/p7songs.msg", "p7songs.msg", "/P7SONGS.MSG", "P7SONGS.MSG",
                                             "/P7SONGS~1.MSG", "P7SONGS~1.MSG"};
const char* oprSongPathCandidates[] = {"/oprsongs.msg", "oprsongs.msg", "/OPRSONGS.MSG", "OPRSONGS.MSG",
                                       "/OPRSON~1.MSG", "OPRSON~1.MSG"};
const char* codeRedSongPathCandidates[] = {"/crsongs.msg", "crsongs.msg", "/CRSONGS.MSG", "CRSONGS.MSG",
                                           "/CRSONGS~1.MSG", "CRSONGS~1.MSG"};

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

const MCFG_PlayMode* findPlayModeByName(const MidiConfigRuntime& config, const char* modeKey)
{
    if (modeKey == nullptr)
    {
        return nullptr;
    }

    for (uint16_t playModeIndex = 0; playModeIndex < config.header->playModeCount; ++playModeIndex)
    {
        const MCFG_PlayMode* playMode = mcfg_get_play_mode(config, playModeIndex);
        if (playMode == nullptr)
        {
            continue;
        }

        if (std::strcmp(mcfg_get_string(config, playMode->nameIndex), modeKey) == 0)
        {
            return playMode;
        }
    }

    return nullptr;
}

const MCFG_Patch* findPatchByNumber(const MidiConfigRuntime& config, const MCFG_PlayMode* playMode, byte patchNumber)
{
    if (playMode == nullptr)
    {
        return nullptr;
    }

    for (uint16_t patchIndex = 0; patchIndex < playMode->patchCount; ++patchIndex)
    {
        const MCFG_Patch* patch = mcfg_get_patch(config, playMode, patchIndex);
        if (patch != nullptr && patch->patchNumber == patchNumber)
        {
            return patch;
        }
    }

    return nullptr;
}

bool behaviourForMcfgEventType(uint8_t eventType, FunctionBehaviour& behaviour)
{
    switch (eventType)
    {
    case 0:
        behaviour = FunctionBehaviour::ShortPress;
        return true;
    case 1:
        behaviour = FunctionBehaviour::ButtonDown;
        return true;
    case 2:
        behaviour = FunctionBehaviour::ButtonRelease;
        return true;
    default:
        return false;
    }
}

void copyConfigString(const char* source, char* destination, size_t destinationCapacity)
{
    if (destination == nullptr || destinationCapacity == 0U)
    {
        return;
    }

    destination[0] = '\0';
    if (source == nullptr)
    {
        return;
    }

    std::strncpy(destination, source, destinationCapacity - 1U);
    destination[destinationCapacity - 1U] = '\0';
}

void copySongNotes(const SongCatalogue& songsCatalogue, const MCFG_Song& song, SongNotes* outNotes)
{
    if (outNotes == nullptr)
    {
        return;
    }

    *outNotes = SongNotes{};
    if (song.notesCount == 0U)
    {
        return;
    }

    const size_t notesToCopy =
        (static_cast<size_t>(song.notesCount) < SongNotes::MaxLines) ? static_cast<size_t>(song.notesCount)
                                                                     : SongNotes::MaxLines;
    for (size_t noteIndex = 0; noteIndex < notesToCopy; ++noteIndex)
    {
        copyConfigString(songNoteString(songsCatalogue, song, noteIndex), outNotes->lines[noteIndex],
                         sizeof(outNotes->lines[noteIndex]));
    }

    outNotes->lineCount = static_cast<uint8_t>(notesToCopy);
}

const char* const* songPathCandidatesForPlaylist(byte playlistIndex, size_t& candidateCount)
{
    switch (playlistIndex)
    {
    case Project7PlaylistIndex:
        candidateCount = sizeof(project7SongPathCandidates) / sizeof(project7SongPathCandidates[0]);
        return project7SongPathCandidates;
    case OprPlaylistIndex:
        candidateCount = sizeof(oprSongPathCandidates) / sizeof(oprSongPathCandidates[0]);
        return oprSongPathCandidates;
    case CodeRedPlaylistIndex:
        candidateCount = sizeof(codeRedSongPathCandidates) / sizeof(codeRedSongPathCandidates[0]);
        return codeRedSongPathCandidates;
    default:
        candidateCount = 0U;
        return nullptr;
    }
}
}

ButtonOverrideStore::ButtonOverrideStore(ITextFileStore* fileStore)
    : _fileStore(fileStore)
{
}

ButtonOverrideStore::~ButtonOverrideStore() { clearOverrides(); }

bool ButtonOverrideStore::refresh()
{
    clearOverrides();

    for (size_t pathIndex = 0; pathIndex < (sizeof(binaryConfigPathCandidates) / sizeof(binaryConfigPathCandidates[0]));
         ++pathIndex)
    {
        File configFile = SD.open(binaryConfigPathCandidates[pathIndex], FILE_READ);
        if (!configFile)
        {
            continue;
        }

        MidiConfigRuntime loadedConfig = {};
        if (mcfg_load(configFile, loadedConfig))
        {
            _binaryConfig = loadedConfig;
            _hasBinaryConfig = true;
            configFile.close();
            Serial.printf("Button overrides: loaded config from '%s'.\n", binaryConfigPathCandidates[pathIndex]);
            return true;
        }
        configFile.close();
        mcfg_free(loadedConfig);
    }

    Serial.println("Button overrides: binary config unavailable.");
    return true;
}

void ButtonOverrideStore::applyOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount,
                                         PatchDisplayConfig* patchDisplay) const
{
    if (patchDisplay != nullptr)
    {
        patchDisplay->name[0] = '\0';
    }

    if (!_hasBinaryConfig)
    {
        Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(patchNumber));
        return;
    }

    applyBinaryOverrides(playlistIndex, patchNumber, functions, functionCount, patchDisplay);
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

const char* ButtonOverrideStore::songsConfigPathForPlaylist(byte playlistIndex)
{
    switch (playlistIndex)
    {
    case Project7PlaylistIndex:
        return "/p7songs.msg";
    case OprPlaylistIndex:
        return "/oprsongs.msg";
    case CodeRedPlaylistIndex:
        return "/crsongs.msg";
    default:
        return nullptr;
    }
}

bool ButtonOverrideStore::loadSongCatalogueForPlaylist(byte playlistIndex, SongCatalogue& outCatalogue,
                                                       const char*& loadedPath) const
{
    outCatalogue = {};
    loadedPath = nullptr;

    size_t candidateCount = 0U;
    const char* const* pathCandidates = songPathCandidatesForPlaylist(playlistIndex, candidateCount);
    for (size_t pathIndex = 0; pathIndex < candidateCount; ++pathIndex)
    {
        File songsFile = SD.open(pathCandidates[pathIndex], FILE_READ);
        if (!songsFile)
        {
            continue;
        }

        if (loadSongCatalogue(songsFile, outCatalogue))
        {
            loadedPath = pathCandidates[pathIndex];
            songsFile.close();
            return true;
        }

        songsFile.close();
        freeSongCatalogue(outCatalogue);
    }

    return false;
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
    if (std::strcmp(name, "Tap") == 0 || std::strcmp(name, "TapTempo") == 0)
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

size_t ButtonOverrideStore::listPatches(byte playlistIndex, PatchListEntry* entries, size_t capacity) const
{
    if (!_hasBinaryConfig)
    {
        return 0;
    }
    return listBinaryPatches(playlistIndex, entries, capacity);
}

bool ButtonOverrideStore::applyBinaryOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount,
                                               PatchDisplayConfig* patchDisplay) const
{
    if (patchDisplay != nullptr)
    {
        patchDisplay->name[0] = '\0';
    }

    const char* modeKey = modeKeyForPlaylistIndex(playlistIndex);
    const MCFG_PlayMode* playMode = findPlayModeByName(_binaryConfig, modeKey);
    const MCFG_Patch* patch = findPatchByNumber(_binaryConfig, playMode, patchNumber);
    if (playMode == nullptr || patch == nullptr)
    {
        Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(patchNumber));
        return true;
    }

    if (patchDisplay != nullptr)
    {
        const char* patchName = mcfg_get_string(_binaryConfig, patch->nameIndex);
        const char* patchLongName = mcfg_get_string(_binaryConfig, patch->longNameIndex);
        const char* patchDisplayName = (patchLongName != nullptr && patchLongName[0] != '\0') ? patchLongName : patchName;
        copyConfigString(patchDisplayName, patchDisplay->name, sizeof(patchDisplay->name));
    }

    const bool hasPatchDisplayName = (patchDisplay != nullptr && patchDisplay->name[0] != '\0');
    if (functions == nullptr || functionCount == 0)
    {
        if (!hasPatchDisplayName)
        {
            Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                          static_cast<unsigned int>(patchNumber));
        }
        return true;
    }

    uint8_t appliedCount = 0;
    for (uint16_t buttonOffset = 0; buttonOffset < patch->buttonCount; ++buttonOffset)
    {
        const MCFG_Button* button = mcfg_get_button(_binaryConfig, patch, buttonOffset);
        if (button == nullptr || button->buttonId >= functionCount)
        {
            continue;
        }

        ParsedButtonOverride parsedButton = {};
        clearButtonOverride(parsedButton);

        const char* label = mcfg_get_string(_binaryConfig, button->labelIndex);
        if (label != nullptr && label[0] != '\0')
        {
            parsedButton.hasLabel = true;
            copyConfigString(label, parsedButton.label, sizeof(parsedButton.label));
        }

        const char* colour = mcfg_get_string(_binaryConfig, button->colourIndex);
        if (colour != nullptr && colour[0] != '\0')
        {
            uint16_t parsedColour = 0;
            if (parseColourValue(colour, parsedColour))
            {
                parsedButton.hasColour = true;
                parsedButton.colour = parsedColour;
            }
        }

        parsedButton.hasToggle = true;
        parsedButton.toggle = (button->toggle != 0);

        for (uint16_t functionOffset = 0; functionOffset < button->functionCount; ++functionOffset)
        {
            const MCFG_Function* function = mcfg_get_function(_binaryConfig, button, functionOffset);
            if (function == nullptr)
            {
                continue;
            }

            FunctionBehaviour behaviour = FunctionBehaviour::ShortPress;
            if (!behaviourForMcfgEventType(function->eventType, behaviour))
            {
                continue;
            }

            ActionType actionType = ActionType::None;
            const char* actionName = mcfg_get_string(_binaryConfig, function->actionNameIndex);
            if (!parseActionName(actionName, actionType))
            {
                continue;
            }

            const uint8_t behaviourIndex = static_cast<uint8_t>(behaviour);
            parsedButton.actions[behaviourIndex].isDefined = true;
            parsedButton.actions[behaviourIndex].action =
                FunctionAction(actionType, static_cast<byte>(function->value), static_cast<uint8_t>(function->secondaryValue));
        }

        applyButtonOverride(parsedButton, functions[button->buttonId]);
        ++appliedCount;
        Serial.printf("Button overrides: applied override for %s patch %u button %u.\n",
                      playlistName(playlistIndex), static_cast<unsigned int>(patchNumber),
                      static_cast<unsigned int>(button->buttonId + 1U));
    }

    if (appliedCount == 0 && !hasPatchDisplayName)
    {
        Serial.printf("Button overrides: no override matched for %s patch %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(patchNumber));
    }

    return true;
}

size_t ButtonOverrideStore::listBinaryPatches(byte playlistIndex, PatchListEntry* entries, size_t capacity) const
{
    if (entries == nullptr || capacity == 0)
    {
        return 0;
    }

    const char* modeKey = modeKeyForPlaylistIndex(playlistIndex);
    const MCFG_PlayMode* playMode = findPlayModeByName(_binaryConfig, modeKey);
    if (playMode == nullptr)
    {
        return 0;
    }

    size_t parsedCount = 0;
    for (uint16_t patchIndex = 0; patchIndex < playMode->patchCount && parsedCount < capacity; ++patchIndex)
    {
        const MCFG_Patch* patch = mcfg_get_patch(_binaryConfig, playMode, patchIndex);
        if (patch == nullptr)
        {
            continue;
        }

        PatchListEntry& entry = entries[parsedCount++];
        entry.patchNumber = patch->patchNumber;
        copyConfigString(mcfg_get_string(_binaryConfig, patch->nameIndex), entry.name, sizeof(entry.name));
    }

    return parsedCount;
}

size_t ButtonOverrideStore::listSongs(byte playlistIndex, SongListEntry* entries, size_t capacity) const
{
    if (entries == nullptr || capacity == 0)
    {
        return 0;
    }

    SongCatalogue songsCatalogue;
    const char* loadedPath = nullptr;
    if (!loadSongCatalogueForPlaylist(playlistIndex, songsCatalogue, loadedPath))
    {
        const char* expectedPath = songsConfigPathForPlaylist(playlistIndex);
        Serial.printf("Button overrides: songs catalogue unavailable for %s (expected '%s').\n", playlistName(playlistIndex),
                      expectedPath != nullptr ? expectedPath : "<none>");
        return 0;
    }

    size_t parsedCount = 0;
    for (size_t songIndex = 0; songIndex < songsCatalogue.header->songCount && parsedCount < capacity; ++songIndex)
    {
        if (songIndex > 0xFFU)
        {
            break;
        }

        SongListEntry& entry = entries[parsedCount++];
        entry.songIndex = static_cast<byte>(songIndex);
        entry.patchNumber = songsCatalogue.songs[songIndex].patch;
        entry.name[0] = '\0';
        entry.id[0] = '\0';
        copyConfigString(songString(songsCatalogue, songsCatalogue.songs[songIndex].nameIndex), entry.name, sizeof(entry.name));
        copyConfigString(songString(songsCatalogue, songsCatalogue.songs[songIndex].idIndex), entry.id, sizeof(entry.id));
    }

    freeSongCatalogue(songsCatalogue);

    Serial.printf("Button overrides: loaded %u songs for %s from '%s'.\n", static_cast<unsigned int>(parsedCount),
                  playlistName(playlistIndex), loadedPath != nullptr ? loadedPath : "<unknown>");
    return parsedCount;
}

bool ButtonOverrideStore::songForIndex(byte playlistIndex, byte songIndex, SongConfig* outSong) const
{
    if (outSong == nullptr)
    {
        return false;
    }

    outSong->patchNumber = 0;
    outSong->name[0] = '\0';
    outSong->displayName[0] = '\0';
    outSong->id[0] = '\0';

    SongCatalogue songsCatalogue;
    const char* loadedPath = nullptr;
    if (!loadSongCatalogueForPlaylist(playlistIndex, songsCatalogue, loadedPath))
    {
        Serial.printf("Button overrides: no song matched for %s song %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(songIndex));
        return false;
    }
    (void)loadedPath;

    bool foundSong = false;
    if (songIndex < songsCatalogue.header->songCount)
    {
        const MCFG_Song& song = songsCatalogue.songs[songIndex];
        outSong->patchNumber = song.patch;
        copyConfigString(songString(songsCatalogue, song.nameIndex), outSong->name, sizeof(outSong->name));
        copyConfigString(songString(songsCatalogue, song.idIndex), outSong->id, sizeof(outSong->id));
        const char* longName = songString(songsCatalogue, song.longNameIndex);
        const char* displayName = (longName != nullptr && longName[0] != '\0') ? longName : outSong->name;
        copyConfigString(displayName, outSong->displayName, sizeof(outSong->displayName));
        foundSong = true;
    }

    freeSongCatalogue(songsCatalogue);

    if (!foundSong)
    {
        Serial.printf("Button overrides: invalid song entry for %s song %u.\n", playlistName(playlistIndex),
                      static_cast<unsigned int>(songIndex));
        return false;
    }

    return true;
}

bool ButtonOverrideStore::songNotesForIndex(byte playlistIndex, byte songIndex, SongNotes* outNotes) const
{
    if (outNotes == nullptr)
    {
        return false;
    }

    *outNotes = SongNotes{};

    SongCatalogue songsCatalogue;
    const char* loadedPath = nullptr;
    if (!loadSongCatalogueForPlaylist(playlistIndex, songsCatalogue, loadedPath))
    {
        return false;
    }
    (void)loadedPath;

    bool foundSong = false;
    if (songIndex < songsCatalogue.header->songCount)
    {
        copySongNotes(songsCatalogue, songsCatalogue.songs[songIndex], outNotes);
        foundSong = true;
    }

    freeSongCatalogue(songsCatalogue);

    return foundSong;
}

bool ButtonOverrideStore::songForId(byte playlistIndex, const char* songId, byte& songIndex, SongConfig& outSong) const
{
    songIndex = 0;
    outSong = SongConfig{};

    if (songId == nullptr || songId[0] == '\0')
    {
        return false;
    }

    SongCatalogue songsCatalogue;
    const char* loadedPath = nullptr;
    if (!loadSongCatalogueForPlaylist(playlistIndex, songsCatalogue, loadedPath))
    {
        return false;
    }
    (void)loadedPath;

    bool foundSong = false;
    for (size_t candidateIndex = 0; candidateIndex < songsCatalogue.header->songCount; ++candidateIndex)
    {
        const MCFG_Song& song = songsCatalogue.songs[candidateIndex];
        const char* configuredSongId = songString(songsCatalogue, song.idIndex);
        if (configuredSongId == nullptr || std::strcmp(configuredSongId, songId) != 0)
        {
            continue;
        }

        if (candidateIndex <= 0xFFU)
        {
            songIndex = static_cast<byte>(candidateIndex);
            outSong.patchNumber = song.patch;
            copyConfigString(songString(songsCatalogue, song.nameIndex), outSong.name, sizeof(outSong.name));
            copyConfigString(configuredSongId, outSong.id, sizeof(outSong.id));
            const char* longName = songString(songsCatalogue, song.longNameIndex);
            const char* displayName = (longName != nullptr && longName[0] != '\0') ? longName : outSong.name;
            copyConfigString(displayName, outSong.displayName, sizeof(outSong.displayName));
            foundSong = true;
        }

        break;
    }

    freeSongCatalogue(songsCatalogue);
    return foundSong;
}

void ButtonOverrideStore::clearButtonOverride(ParsedButtonOverride& buttonOverride)
{
    buttonOverride.hasLabel = false;
    buttonOverride.label[0] = '\0';
    buttonOverride.hasColour = false;
    buttonOverride.colour = 0;
    buttonOverride.hasToggle = false;
    buttonOverride.toggle = false;

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

    if (buttonOverride.hasToggle)
    {
        target.setToggle(buttonOverride.toggle);
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

void ButtonOverrideStore::clearOverrides()
{
    mcfg_free(_binaryConfig);
    _hasBinaryConfig = false;
}
