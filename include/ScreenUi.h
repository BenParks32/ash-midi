#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Gfx.h"
#include "Resources.h"
#include "Touch/TouchButtonUi.h"

class ScreenUi : public ITouchButtonLayout
{
  public:
    ScreenUi(TFT_eSPI& tft, const Size& screenSize);

  private:
    static constexpr uint16_t to565(uint8_t r, uint8_t g, uint8_t b)
    {
        return (uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | ((uint16_t)b >> 3));
    }

  private:
    ScreenUi() = delete;
    ScreenUi(const ScreenUi&) = delete;
    ScreenUi& operator=(const ScreenUi&) = delete;

  public:
    void drawBackgroundAndBorder();
    void drawLogo(const GFXfont* titleFont, uint8_t titleScale, const char* title, const GFXfont* subtitleFont,
                  uint8_t subtitleScale, const char* subtitle);

    void drawText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y,
                  uint16_t textColour = TFT_WHITE, uint16_t backgroundColour = TFT_BLACK);
    void drawSmallText(const char* label, int32_t x, int32_t y, uint16_t textColour = TFT_WHITE,
                       uint16_t backgroundColour = TFT_BLACK);
    void clearText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y);
    void updateText(const GFXfont* font, uint8_t scale, const char* previousLabel, const char* newLabel, int32_t x,
                    int32_t y);
    void drawCenteredText(const GFXfont* font, uint8_t scale, const char* label, int32_t centerX, int32_t y,
                          uint16_t textColour = TFT_WHITE, uint16_t backgroundColour = TFT_BLACK);

    void drawIcon(const Icon& icon, int32_t x, int32_t y);
    void drawIconCentered(const Icon& icon, const Point& areaLocation, const Size& areaSize);
    void setTouchButtonLabelStyle(const GFXfont* font, uint8_t scale);
    void drawTouchButtonLabelAndPill(const char* label, const Point& areaLocation, const Size& areaSize,
                                     uint16_t pillColour, bool selected = false,
                                     uint16_t selectedBorderColour = TFT_WHITE,
                                     uint16_t textColour = TFT_WHITE) override;
    void drawStatusIndicator(int32_t circleX, int32_t circleY, int32_t radius, const char* label, int32_t textX,
                             int32_t textY, uint16_t colour);
    void drawSdStatusInitializing();
    void drawSdStatusFailed();
    void drawSdStatusReady();

    int32_t boxWidth() const override;
    int32_t boxHeight() const;
    int32_t bottomRowY() const override;
    Size boxSize() const override;

  private:
    void fillScreenFast(uint16_t color);
    void drawBorder();
    void drawLogoFrame();

  private:
    TFT_eSPI& _tft;
    const Size _screenSize;

    const int32_t _lineWidth = 2;
    const int32_t _boxHeight = _screenSize.height / 4;
    const int32_t _boxWidth = _screenSize.width / 4;
    const int32_t _bottomRowY = _boxHeight * 3;

    const int32_t _titleCenterX = _screenSize.width / 2;
    const int32_t _logoSectionTop = _screenSize.height / 4;
    const int32_t _logoSectionBottom = (_screenSize.height * 3) / 4;
    const int32_t _logoFrameWidth = (232 * 7) / 10;
    const int32_t _logoFrameHeight = 98;
    const int32_t _logoFrameRadius = 14;
    const int32_t _logoFrameLeft = _titleCenterX - (_logoFrameWidth / 2);
    const int32_t _logoFrameTop = _logoSectionTop + ((_logoSectionBottom - _logoSectionTop - _logoFrameHeight) / 2);
    const int32_t _logoTextLineSpacing = 34;
    const int32_t _logoTextVerticalOffset = _logoFrameHeight / 10;

    const int32_t _sdStatusCircleX = 20;
    const int32_t _sdStatusCircleY = _boxHeight + 20;
    const int32_t _sdStatusRadius = 10;
    const int32_t _sdStatusTextX = 40;
    const int32_t _sdStatusTextY = _boxHeight + 14;
    const int32_t _sdStatusTextClearWidth = 120;
    const int32_t _sdStatusTextClearHeight = 16;

    const uint16_t _borderColour = to565(175, 179, 186);
    const uint16_t _logoFrameOuterColour = to565(146, 158, 176);
    const uint16_t _logoFrameInnerColour = to565(84, 97, 116);

    const GFXfont* _touchButtonLabelFont = nullptr;
    uint8_t _touchButtonLabelScale = 1;
};
