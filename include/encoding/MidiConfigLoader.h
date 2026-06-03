#ifndef MIDI_CONFIG_LOADER_H
#define MIDI_CONFIG_LOADER_H

#include "MidiConfigFormat.h"
#include <Arduino.h>
#include <SD.h>

struct MidiConfigRuntime
{
    uint8_t* base; // raw file buffer
    size_t size;   // file size

    const MCFG_FileHeader* header;
    const MCFG_StringTableHeader* strHeader;
    const uint32_t* strOffsets;
    const char* strBlob;
    const MCFG_PlayMode* playModes;
};

// Load entire file into RAM and set up pointers.
// Returns true on success.
inline bool mcfg_load(File& f, MidiConfigRuntime& cfg)
{
    cfg = {};
    if (!f)
        return false;

    size_t size = f.size();
    if (size < sizeof(MCFG_FileHeader))
        return false;

    uint8_t* buf = (uint8_t*)malloc(size);
    if (!buf)
        return false;

    size_t readBytes = f.read(buf, size);
    if (readBytes != size)
    {
        free(buf);
        return false;
    }

    cfg.base = buf;
    cfg.size = size;

    const MCFG_FileHeader* header = (const MCFG_FileHeader*)buf;
    if (header->magic != MCFG_MAGIC)
    {
        free(buf);
        cfg.base = nullptr;
        return false;
    }

    if (header->stringTableOffset + sizeof(MCFG_StringTableHeader) > size ||
        header->playModeOffset + static_cast<size_t>(header->playModeCount) * sizeof(MCFG_PlayMode) > size)
    {
        free(buf);
        cfg.base = nullptr;
        return false;
    }

    const auto* strHeader = (const MCFG_StringTableHeader*)(buf + header->stringTableOffset);
    const size_t strOffsetsBytes = static_cast<size_t>(strHeader->count) * sizeof(uint32_t);
    const size_t strOffsetsStart = header->stringTableOffset + sizeof(MCFG_StringTableHeader);
    const size_t strBlobStart = strOffsetsStart + strOffsetsBytes;
    if (strBlobStart > size)
    {
        free(buf);
        cfg.base = nullptr;
        return false;
    }

    const auto* strOffsets = (const uint32_t*)(buf + strOffsetsStart);
    size_t maxStringOffset = 0;
    for (uint16_t index = 0; index < strHeader->count; ++index)
    {
        const size_t currentOffset = static_cast<size_t>(strOffsets[index]);
        if (currentOffset > maxStringOffset)
        {
            maxStringOffset = currentOffset;
        }
    }
    if (strHeader->count > 0 && strBlobStart + maxStringOffset >= size)
    {
        free(buf);
        cfg.base = nullptr;
        return false;
    }

    cfg.header = header;
    cfg.strHeader = strHeader;
    cfg.strOffsets = strOffsets;
    cfg.strBlob = (const char*)(buf + strBlobStart);
    cfg.playModes = (const MCFG_PlayMode*)(buf + header->playModeOffset);

    return true;
}

inline void mcfg_free(MidiConfigRuntime& cfg)
{
    if (cfg.base)
    {
        free(cfg.base);
        cfg.base = nullptr;
    }
}

// -------------------------------------------------------------
// Helpers
// -------------------------------------------------------------

inline const char* mcfg_get_string(const MidiConfigRuntime& cfg, uint16_t index)
{
    if (!mcfg_valid_string(index))
        return "";
    if (index >= cfg.strHeader->count)
        return "";
    uint32_t off = cfg.strOffsets[index];
    return cfg.strBlob + off;
}

inline const MCFG_PlayMode* mcfg_get_play_mode(const MidiConfigRuntime& cfg, uint16_t idx)
{
    if (idx >= cfg.header->playModeCount)
        return nullptr;
    return &cfg.playModes[idx];
}

inline const MCFG_Patch* mcfg_get_patch(const MidiConfigRuntime& cfg, const MCFG_PlayMode* pm, uint16_t idx)
{
    if (idx >= pm->patchCount)
        return nullptr;
    const uint8_t* base = cfg.base;
    const MCFG_Patch* patches = (const MCFG_Patch*)(base + pm->patchOffset);
    return &patches[idx];
}

inline const MCFG_Button* mcfg_get_button(const MidiConfigRuntime& cfg, const MCFG_Patch* patch, uint16_t idx)
{
    if (idx >= patch->buttonCount)
        return nullptr;
    const uint8_t* base = cfg.base;
    const MCFG_Button* buttons = (const MCFG_Button*)(base + patch->buttonOffset);
    return &buttons[idx];
}

inline const MCFG_Function* mcfg_get_function(const MidiConfigRuntime& cfg, const MCFG_Button* btn, uint16_t idx)
{
    if (idx >= btn->functionCount)
        return nullptr;
    const uint8_t* base = cfg.base;
    const MCFG_Function* funcs = (const MCFG_Function*)(base + btn->functionOffset);
    return &funcs[idx];
}

#endif // MIDI_CONFIG_LOADER_H
