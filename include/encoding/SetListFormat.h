#ifndef SET_LIST_FORMAT_H
#define SET_LIST_FORMAT_H

#include <stdint.h>

#define MSLT_MAGIC 0x4D534C54u /* 'MSLT' */
#define MSLT_VERSION 1u
#define MSLT_STRING_NONE 0xFFFFu

#if defined(__GNUC__)
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

struct PACKED MSLT_FileHeader
{
    uint32_t magic;
    uint16_t version;
    uint16_t partCount;
    uint16_t songCount;
    uint16_t nameIndex;
    uint32_t stringTableOffset;
    uint32_t partTableOffset;
    uint32_t songTableOffset;
};

struct PACKED MSLT_Part
{
    uint16_t part;
    uint16_t nameIndex;
};

struct PACKED MSLT_Song
{
    uint16_t number;
    uint16_t part;
    uint16_t songIdIndex;
};

#endif
