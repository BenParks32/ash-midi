#pragma once

#include "Touch/TouchButtonUi.h"

#include <Arduino.h>

struct GFXfont;

class IScreenUi : public ITouchButtonLayout
{
  public:
    virtual ~IScreenUi() = default;

    virtual void drawBackgroundAndBorder() = 0;
    virtual void drawCenteredFrame(int32_t centerX, int32_t topY, int32_t width, int32_t height, int32_t radius) = 0;
    virtual void drawCenteredFrame(int32_t centerX, int32_t topY, int32_t width, int32_t height, int32_t radius,
                                   uint16_t outerColour, uint16_t innerColour) = 0;
    virtual void drawLogo(const GFXfont* titleFont, uint8_t titleScale, const char* title, const GFXfont* subtitleFont,
                          uint8_t subtitleScale, const char* subtitle) = 0;
    virtual void drawLogo(const GFXfont* titleFont, uint8_t titleScale, const char* title,
                          const GFXfont* subtitleFont, uint8_t subtitleScale, const char* subtitle,
                          uint16_t titleColour, uint16_t subtitleColour, uint16_t outerFrameColour,
                          uint16_t innerFrameColour) = 0;
    virtual void drawText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y,
                          uint16_t textColour = 0xFFFF, uint16_t backgroundColour = 0x0000) = 0;
    virtual void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t colour) = 0;
    virtual void drawRect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t colour) = 0;
    virtual void drawSmallText(const char* label, int32_t x, int32_t y, uint16_t textColour = 0xFFFF,
                               uint16_t backgroundColour = 0x0000) = 0;
    virtual void drawCenteredText(const GFXfont* font, uint8_t scale, const char* label, int32_t centerX, int32_t y,
                                  uint16_t textColour = 0xFFFF, uint16_t backgroundColour = 0x0000) = 0;
    virtual void drawTouchButtonPill(const Point& areaLocation, const Size& areaSize, uint16_t pillColour,
                                     uint16_t pillBorderColour = 0xFFFF) = 0;
    virtual void setSdStatusInitializing() = 0;
    virtual void setSdStatusFailed() = 0;
    virtual void setSdStatusReady() = 0;
    virtual void setSdStatusNotMounted() = 0;
    virtual void hideSdStatus() = 0;
    virtual void redrawSdStatus() = 0;
    virtual uint16_t touchButtonPillBorderColour() const = 0;
    virtual int32_t boxHeight() const = 0;
};
