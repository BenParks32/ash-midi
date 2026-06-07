#pragma once

#include <cstdint>

#define GFXFONT_STUB 1

struct GFXfont
{
};

static GFXfont g_ff22 = {};
static GFXfont g_ff24 = {};
static GFXfont g_ff32 = {};

static GFXfont* const FF22 = &g_ff22;
static GFXfont* const FF24 = &g_ff24;
static GFXfont* const FF32 = &g_ff32;

#ifndef GFXFF
#define GFXFF 1
#endif

constexpr std::uint16_t TFT_BLACK = 0x0000;
constexpr std::uint16_t TFT_WHITE = 0xFFFF;
constexpr std::uint16_t TFT_RED = 0xF800;
constexpr std::uint16_t TFT_GREEN = 0x07E0;
constexpr std::uint16_t TFT_YELLOW = 0xFFE0;
constexpr std::uint16_t TFT_DARKGREY = 0x7BEF;

class TFT_eSPI
{
};
