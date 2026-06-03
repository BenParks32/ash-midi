#ifndef MIDI_CONFIG_FORMAT_H
#define MIDI_CONFIG_FORMAT_H

#include <stdint.h>

#define MCFG_MAGIC 0x4D434647u /* 'MCFG' */
#define MCFG_STRING_INDEX_NONE 0xFFFFu

/* Force packed structs for exact binary layout */
#if defined(__GNUC__)
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

/* -------------------------------------------------------------
   ENUMS
------------------------------------------------------------- */

enum MCFG_EventType : uint8_t
{
    MCFG_EVENT_SHORT_PRESS = 0,
    MCFG_EVENT_PRESS = 1,
    MCFG_EVENT_RELEASE = 2
};

enum MCFG_ActionType : uint8_t
{
    MCFG_ACTION_TAP = 0,
    MCFG_ACTION_SEND_MIDI_CC = 1
};

/* -------------------------------------------------------------
   FILE HEADER
------------------------------------------------------------- */

struct PACKED MCFG_FileHeader
{
    uint32_t magic;             /* MCFG_MAGIC */
    uint16_t version;           /* format version */
    uint16_t playModeCount;     /* number of PlayMode records */
    uint32_t stringTableOffset; /* offset from start of file */
    uint32_t playModeOffset;    /* offset from start of file */
};

/* -------------------------------------------------------------
   STRING TABLE
------------------------------------------------------------- */

struct PACKED MCFG_StringTableHeader
{
    uint16_t count; /* number of strings */
    /* Followed by:
         uint32_t offsets[count];
         char blob[];
    */
};

/* -------------------------------------------------------------
   PLAY MODES
------------------------------------------------------------- */

struct PACKED MCFG_PlayMode
{
    uint16_t nameIndex;   /* string index */
    uint16_t patchCount;  /* number of Patch records */
    uint32_t patchOffset; /* offset from start of file */
};

/* -------------------------------------------------------------
   PATCHES
------------------------------------------------------------- */

struct PACKED MCFG_Patch
{
    uint16_t nameIndex;     /* 0xFFFF if none */
    uint16_t longNameIndex; /* 0xFFFF if none */
    uint8_t patchNumber;    /* original JSON key */
    uint8_t buttonCount;    /* number of Button records */
    uint32_t buttonOffset;  /* offset from start of file */
};

/* -------------------------------------------------------------
   BUTTONS
------------------------------------------------------------- */

struct PACKED MCFG_Button
{
    uint8_t buttonId;        /* original JSON key */
    uint16_t labelIndex;     /* string index */
    uint16_t colourIndex;    /* string index, 0xFFFF if none */
    uint8_t toggle;          /* 0 or 1 */
    uint8_t functionCount;   /* number of Function records */
    uint32_t functionOffset; /* offset from start of file */
};

/* -------------------------------------------------------------
   FUNCTIONS
------------------------------------------------------------- */

struct PACKED MCFG_Function
{
    uint8_t eventType;        /* MCFG_EventType */
    uint8_t actionType;       /* MCFG_ActionType */
    uint16_t actionNameIndex; /* string index, 0xFFFF if none */
    uint16_t value;           /* e.g. CC number */
    uint16_t secondaryValue;  /* e.g. 100 */
};

/* -------------------------------------------------------------
   HELPERS
------------------------------------------------------------- */

static inline bool mcfg_valid_string(uint16_t idx) { return idx != MCFG_STRING_INDEX_NONE; }

#endif /* MIDI_CONFIG_FORMAT_H */
