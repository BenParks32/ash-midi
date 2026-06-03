#ifndef SET_LIST_LOADER_H
#define SET_LIST_LOADER_H

#include "SetListFormat.h"

#include <Arduino.h>
#include <SD.h>
#include <stdlib.h>
#include <string.h>

struct SetListCatalogue
{
    uint8_t* base = nullptr;
    size_t size = 0U;

    const MSLT_FileHeader* header = nullptr;
    uint16_t stringCount = 0U;
    const uint32_t* stringOffsets = nullptr;
    const char* stringBlob = nullptr;
    const MSLT_Part* parts = nullptr;
    const MSLT_Song* songs = nullptr;
};

inline bool loadSetListCatalogue(uint8_t* buf, size_t size, SetListCatalogue& cat)
{
    cat = {};
    if (buf == nullptr)
        return false;

    if (size < sizeof(MSLT_FileHeader))
        return false;

    const MSLT_FileHeader* header = (const MSLT_FileHeader*)buf;
    if (header->magic != MSLT_MAGIC || header->version != MSLT_VERSION)
    {
        free(buf);
        return false;
    }

    const size_t partTableBytes = static_cast<size_t>(header->partCount) * sizeof(MSLT_Part);
    const size_t songTableBytes = static_cast<size_t>(header->songCount) * sizeof(MSLT_Song);
    if (header->stringTableOffset + sizeof(uint16_t) > size ||
        header->partTableOffset + partTableBytes > size ||
        header->songTableOffset + songTableBytes > size)
    {
        free(buf);
        return false;
    }

    const uint8_t* stringTableBase = buf + header->stringTableOffset;
    uint16_t stringCount = 0U;
    memcpy(&stringCount, stringTableBase, sizeof(stringCount));
    const size_t stringOffsetsStart = header->stringTableOffset + sizeof(uint16_t);
    const size_t stringOffsetsBytes = static_cast<size_t>(stringCount) * sizeof(uint32_t);
    const size_t stringBlobStart = stringOffsetsStart + stringOffsetsBytes;
    if (stringBlobStart > size)
    {
        free(buf);
        return false;
    }

    const uint32_t* stringOffsets = (const uint32_t*)(buf + stringOffsetsStart);
    size_t maxOffset = 0U;
    for (uint16_t index = 0; index < stringCount; ++index)
    {
        const size_t currentOffset = static_cast<size_t>(stringOffsets[index]);
        if (currentOffset > maxOffset)
        {
            maxOffset = currentOffset;
        }
    }
    if (stringCount > 0U && stringBlobStart + maxOffset >= size)
    {
        free(buf);
        return false;
    }

    cat.base = buf;
    cat.size = size;
    cat.header = header;
    cat.stringCount = stringCount;
    cat.stringOffsets = stringOffsets;
    cat.stringBlob = (const char*)(buf + stringBlobStart);
    cat.parts = (const MSLT_Part*)(buf + header->partTableOffset);
    cat.songs = (const MSLT_Song*)(buf + header->songTableOffset);

    return true;
}

inline bool loadSetListCatalogue(File& f, SetListCatalogue& cat)
{
    if (!f)
        return false;

    const size_t size = f.size();
    uint8_t* buf = (uint8_t*)malloc(size);
    if (!buf)
        return false;

    const size_t readBytes = f.read(buf, size);
    if (readBytes != size)
    {
        free(buf);
        return false;
    }

    return loadSetListCatalogue(buf, size, cat);
}

inline void freeSetListCatalogue(SetListCatalogue& cat)
{
    if (cat.base != nullptr)
    {
        free(cat.base);
    }
    cat = {};
}

inline const char* mslt_get_string(const SetListCatalogue& cat, uint16_t index)
{
    if (index == MSLT_STRING_NONE || cat.header == nullptr || cat.stringOffsets == nullptr || cat.stringBlob == nullptr)
    {
        return "";
    }

    if (index >= cat.stringCount)
    {
        return "";
    }

    return cat.stringBlob + cat.stringOffsets[index];
}

inline const MSLT_Part* mslt_get_part(const SetListCatalogue& cat, uint16_t index)
{
    if (cat.header == nullptr || index >= cat.header->partCount)
    {
        return nullptr;
    }

    return &cat.parts[index];
}

inline const MSLT_Song* mslt_get_song(const SetListCatalogue& cat, uint16_t index)
{
    if (cat.header == nullptr || index >= cat.header->songCount)
    {
        return nullptr;
    }

    return &cat.songs[index];
}

#endif
