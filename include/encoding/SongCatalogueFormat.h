#ifndef SONG_CATALOGUE_FORMAT_H
#define SONG_CATALOGUE_FORMAT_H

#include <stdint.h>

#define MSNG_MAGIC 0x4D534E47u /* 'MSNG' */
#define MSNG_VERSION 1u
#define MSNG_STRING_NONE 0xFFFFu

#if defined(__GNUC__)
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

struct PACKED MCFG_SongFileHeader
{
    uint32_t magic;
    uint16_t version;
    uint16_t songCount;
    uint32_t stringTableOffset;
    uint32_t songTableOffset;
};

struct PACKED MCFG_Song
{
    uint16_t nameIndex;
    uint16_t longNameIndex;
    uint16_t idIndex;
    uint8_t patch;
};

#endif
