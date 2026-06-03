#ifndef SONG_CATALOGUE_LOADER_H
#define SONG_CATALOGUE_LOADER_H

#include "SongCatalogueFormat.h"
#include <Arduino.h>
#include <SD.h>

struct SongCatalogue
{
    uint8_t* base = nullptr;
    size_t size = 0U;

    const MCFG_SongFileHeader* header = nullptr;
    uint16_t stringCount = 0U;
    const uint32_t* strOffsets = nullptr;
    const char* strBlob = nullptr;
    const MCFG_Song* songs = nullptr;
};

inline bool loadSongCatalogue(File& f, SongCatalogue& cat)
{
    cat = {};
    if (!f)
        return false;

    const size_t size = f.size();
    if (size < sizeof(MCFG_SongFileHeader))
        return false;

    uint8_t* buf = (uint8_t*)malloc(size);
    if (!buf)
        return false;

    if (f.read(buf, size) != size)
    {
        free(buf);
        return false;
    }

    cat.base = buf;
    cat.size = size;

    const MCFG_SongFileHeader* header = (const MCFG_SongFileHeader*)buf;
    if (header->magic != MSNG_MAGIC || header->version != MSNG_VERSION)
    {
        free(buf);
        return false;
    }

    if (header->stringTableOffset + sizeof(uint16_t) > size ||
        header->songTableOffset + static_cast<size_t>(header->songCount) * sizeof(MCFG_Song) > size)
    {
        free(buf);
        return false;
    }

    const uint8_t* strBase = buf + header->stringTableOffset;
    uint16_t count = 0;
    memcpy(&count, strBase, sizeof(count));
    const size_t strOffsetsStart = header->stringTableOffset + sizeof(uint16_t);
    const size_t strOffsetsBytes = static_cast<size_t>(count) * sizeof(uint32_t);
    const size_t strBlobStart = strOffsetsStart + strOffsetsBytes;
    if (strBlobStart > size)
    {
        free(buf);
        return false;
    }

    const uint32_t* strOffsets = (const uint32_t*)(buf + strOffsetsStart);
    size_t maxOffset = 0U;
    for (uint16_t index = 0; index < count; ++index)
    {
        const size_t currentOffset = static_cast<size_t>(strOffsets[index]);
        if (currentOffset > maxOffset)
        {
            maxOffset = currentOffset;
        }
    }
    if (count > 0U && strBlobStart + maxOffset >= size)
    {
        free(buf);
        return false;
    }

    cat.base = buf;
    cat.size = size;
    cat.header = header;
    cat.stringCount = count;
    cat.strOffsets = strOffsets;
    cat.strBlob = (const char*)(buf + strBlobStart);
    cat.songs = (const MCFG_Song*)(buf + header->songTableOffset);

    return true;
}

inline void freeSongCatalogue(SongCatalogue& cat)
{
    if (cat.base != nullptr)
    {
        free(cat.base);
    }
    cat = {};
}

inline const char* songString(const SongCatalogue& cat, uint16_t idx)
{
    if (idx == MSNG_STRING_NONE)
        return "";
    if (cat.header == nullptr || cat.strOffsets == nullptr || cat.strBlob == nullptr)
        return "";
    if (idx >= cat.stringCount)
        return "";
    return cat.strBlob + cat.strOffsets[idx];
}

#endif
