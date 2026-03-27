#include "ScreenUi.h"

ScreenUi::ScreenUi(TFT_eSPI& tft, const Size& screenSize) : _tft(tft), _screenSize(screenSize) {}

void ScreenUi::drawBackgroundAndBorder()
{
    // At high SPI rates this is often bandwidth-limited; hide the sweep by disabling panel output while drawing.
    _tft.writecommand(TFT_DISPOFF);

    // Keep one SPI transaction open to reduce command overhead during startup draw.
    _tft.startWrite();
    fillScreenFast(TFT_BLACK);
    drawBorder();
    _tft.endWrite();

    _tft.writecommand(TFT_DISPON);
}

void ScreenUi::drawLogo(const GFXfont* titleFont, uint8_t titleScale, const char* title, const GFXfont* subtitleFont,
                        uint8_t subtitleScale, const char* subtitle)
{
    drawLogoFrame();

    // Keep legacy visual spacing, but center the two-line pair vertically in the frame.
    const int32_t logoFrameCenterY = _logoFrameTop + (_logoFrameHeight / 2);
    const int32_t titleY = logoFrameCenterY - (_logoTextLineSpacing / 2) - _logoTextVerticalOffset;
    const int32_t subtitleY = titleY + _logoTextLineSpacing;

    drawCenteredText(titleFont, titleScale, title, _titleCenterX, titleY);
    drawCenteredText(subtitleFont, subtitleScale, subtitle, _titleCenterX, subtitleY);
}

void ScreenUi::drawText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y,
                        uint16_t textColour, uint16_t backgroundColour)
{
    _tft.setFreeFont(font);
    _tft.setTextSize(scale);
    _tft.setTextColor(textColour, backgroundColour);
    _tft.drawString(label, x, y, GFXFF);
}

void ScreenUi::clearText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y)
{
    drawText(font, scale, label, x, y, TFT_BLACK, TFT_BLACK);
}

void ScreenUi::updateText(const GFXfont* font, uint8_t scale, const char* previousLabel, const char* newLabel,
                          int32_t x, int32_t y)
{
    clearText(font, scale, previousLabel, x, y);
    drawText(font, scale, newLabel, x, y);
}

void ScreenUi::drawCenteredText(const GFXfont* font, uint8_t scale, const char* label, int32_t centerX, int32_t y,
                                uint16_t textColour, uint16_t backgroundColour)
{
    _tft.setFreeFont(font);
    _tft.setTextSize(scale);
    _tft.setTextColor(textColour, backgroundColour);
    const int32_t x = centerX - (_tft.textWidth(label, GFXFF) / 2);
    _tft.drawString(label, x, y, GFXFF);
}

void ScreenUi::drawIcon(const Icon& icon, int32_t x, int32_t y)
{
    const Size& iconSize = icon.iconSize();
    _tft.pushImage(x, y, iconSize.width, iconSize.height, icon.data());
}

void ScreenUi::drawIconCentered(const Icon& icon, const Point& areaLocation, const Size& areaSize)
{
    const Size& iconSize = icon.iconSize();
    const int32_t x = areaLocation.x + (areaSize.width - iconSize.width) / 2;
    const int32_t y = areaLocation.y + (areaSize.height - iconSize.height) / 2;
    drawIcon(icon, x, y);
}

int32_t ScreenUi::boxWidth() const { return _boxWidth; }

int32_t ScreenUi::boxHeight() const { return _boxHeight; }

int32_t ScreenUi::bottomRowY() const { return _bottomRowY; }

Size ScreenUi::boxSize() const { return {_boxWidth, _boxHeight}; }

void ScreenUi::fillScreenFast(uint16_t color)
{
    // Direct full-window block fill is faster than generic fillRect clipping path.
    _tft.setAddrWindow(0, 0, _screenSize.width, _screenSize.height);
    _tft.pushBlock(color, (uint32_t)_screenSize.width * (uint32_t)_screenSize.height);
}

void ScreenUi::drawBorder()
{
    // Fast border pass: axis-aligned filled rectangles are much faster than drawWideLine.
    _tft.fillRect(0, 0, _screenSize.width, _lineWidth, _borderColour);
    _tft.fillRect(0, _boxHeight, _screenSize.width, _lineWidth, _borderColour);
    _tft.fillRect(0, _boxHeight * 3, _screenSize.width, _lineWidth, _borderColour);
    _tft.fillRect(0, _screenSize.height - _lineWidth, _screenSize.width, _lineWidth, _borderColour);

    _tft.fillRect(0, 0, _lineWidth, _screenSize.height, _borderColour);
    _tft.fillRect(_screenSize.width - _lineWidth, 0, _lineWidth, _screenSize.height, _borderColour);

    for (int i = 1; i < 4; ++i)
    {
        const int32_t left = _boxWidth * i;
        _tft.fillRect(left, 0, _lineWidth, _boxHeight, _borderColour);
    }

    for (int i = 1; i < 4; ++i)
    {
        const int32_t left = _boxWidth * i;
        _tft.fillRect(left, _bottomRowY, _lineWidth, _screenSize.height - _bottomRowY, _borderColour);
    }
}

void ScreenUi::drawLogoFrame()
{
    _tft.drawRoundRect(_logoFrameLeft, _logoFrameTop, _logoFrameWidth, _logoFrameHeight, _logoFrameRadius,
                       _logoFrameOuterColour);
    _tft.drawRoundRect(_logoFrameLeft + 2, _logoFrameTop + 2, _logoFrameWidth - 4, _logoFrameHeight - 4,
                       _logoFrameRadius - 2, _logoFrameInnerColour);
}
