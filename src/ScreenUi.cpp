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

void ScreenUi::clearCenterSection()
{
    const int32_t x = _lineWidth;
    const int32_t y = _logoSectionTop + _lineWidth;
    const int32_t width = _screenSize.width - (_lineWidth * 2);
    const int32_t height = (_logoSectionBottom - _logoSectionTop) - (_lineWidth * 2);

    if (width <= 0 || height <= 0)
    {
        return;
    }

    _tft.fillRect(x, y, width, height, TFT_BLACK);
}

void ScreenUi::drawCenteredFrame(int32_t centerX, int32_t topY, int32_t width, int32_t height, int32_t radius)
{
    drawCenteredFrame(centerX, topY, width, height, radius, _logoFrameOuterColour, _logoFrameInnerColour);
}

void ScreenUi::drawCenteredFrame(int32_t centerX, int32_t topY, int32_t width, int32_t height, int32_t radius,
                                 uint16_t outerColour, uint16_t innerColour)
{
    if (width <= 0 || height <= 0 || radius < 0)
    {
        return;
    }

    const int32_t left = centerX - (width / 2);
    _tft.drawRoundRect(left, topY, width, height, radius, outerColour);

    const int32_t outerBottomY = topY + height - 1;
    const int32_t outerBottomX = left + radius - 1;
    const int32_t outerBottomWidth = width - (radius * 2) + 2;
    if (outerBottomWidth > 0)
    {
        _tft.drawFastHLine(outerBottomX, outerBottomY, outerBottomWidth, outerColour);
    }

    const int32_t innerLeft = left + 2;
    const int32_t innerTop = topY + 2;
    const int32_t innerWidth = width - 4;
    const int32_t innerHeight = height - 4;
    const int32_t innerRadius = (radius > 2) ? (radius - 2) : 0;
    if (innerWidth > 0 && innerHeight > 0)
    {
        _tft.drawRoundRect(innerLeft, innerTop, innerWidth, innerHeight, innerRadius, innerColour);

        const int32_t innerBottomY = innerTop + innerHeight - 1;
        const int32_t innerBottomX = innerLeft + innerRadius - 1;
        const int32_t innerBottomWidth = innerWidth - (innerRadius * 2) + 2;
        if (innerBottomWidth > 0)
        {
            _tft.drawFastHLine(innerBottomX, innerBottomY, innerBottomWidth, innerColour);
        }
    }
}

void ScreenUi::drawLogo(const GFXfont* titleFont, uint8_t titleScale, const char* title, const GFXfont* subtitleFont,
                        uint8_t subtitleScale, const char* subtitle)
{
    drawLogo(titleFont, titleScale, title, subtitleFont, subtitleScale, subtitle, TFT_WHITE, TFT_WHITE,
             _logoFrameOuterColour, _logoFrameInnerColour);
}

void ScreenUi::drawLogo(const GFXfont* titleFont, uint8_t titleScale, const char* title, const GFXfont* subtitleFont,
                        uint8_t subtitleScale, const char* subtitle, uint16_t titleColour, uint16_t subtitleColour,
                        uint16_t outerFrameColour, uint16_t innerFrameColour)
{
    drawLogoFrame(outerFrameColour, innerFrameColour);

    // Keep legacy visual spacing, but center the two-line pair vertically in the frame.
    const int32_t logoFrameCenterY = _logoFrameTop + (_logoFrameHeight / 2);
    const int32_t titleY = logoFrameCenterY - (_logoTextLineSpacing / 2) - _logoTextVerticalOffset;
    const int32_t subtitleY = titleY + _logoTextLineSpacing;

    drawCenteredText(titleFont, titleScale, title, _titleCenterX, titleY, titleColour, TFT_BLACK);
    drawCenteredText(subtitleFont, subtitleScale, subtitle, _titleCenterX, subtitleY, subtitleColour, TFT_BLACK);
}

void ScreenUi::drawText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y,
                        uint16_t textColour, uint16_t backgroundColour)
{
    _tft.setFreeFont(font);
    _tft.setTextSize(scale);
    _tft.setTextColor(textColour, backgroundColour);
    _tft.drawString(label, x, y, GFXFF);
}

void ScreenUi::fillRect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t colour)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    _tft.fillRect(x, y, width, height, colour);
}

void ScreenUi::drawRect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t colour)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    _tft.drawRect(x, y, width, height, colour);
}

void ScreenUi::drawSmallText(const char* label, int32_t x, int32_t y, uint16_t textColour, uint16_t backgroundColour)
{
    _tft.setTextFont(1);
    _tft.setTextSize(2);
    _tft.setTextColor(textColour, backgroundColour);
    _tft.drawString(label, x, y);
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

void ScreenUi::setTouchButtonLabelStyle(const GFXfont* font, uint8_t scale)
{
    _touchButtonLabelFont = font;
    _touchButtonLabelScale = (scale == 0U) ? 1U : scale;
}

void ScreenUi::drawTouchButtonLabelAndPill(const char* label, const Point& areaLocation, const Size& areaSize,
                                           uint16_t pillColour, bool selected, uint16_t selectedBorderColour,
                                           uint16_t textColour)
{
    const int32_t innerX = areaLocation.x + _lineWidth;
    const int32_t innerY = areaLocation.y + _lineWidth;
    const int32_t innerWidth = areaSize.width - (_lineWidth * 2);
    const int32_t innerHeight = areaSize.height - (_lineWidth * 2);
    if (innerWidth <= 0 || innerHeight <= 0)
    {
        return;
    }

    _tft.fillRect(innerX, innerY, innerWidth + 1, innerHeight, TFT_BLACK);

    if (selected)
    {
        const int32_t selectionThickness = 4;
        for (int32_t i = 0; i < selectionThickness; ++i)
        {
            const int32_t x = innerX + i;
            const int32_t y = innerY + i;
            const int32_t width = innerWidth - (i * 2);
            const int32_t height = innerHeight - (i * 2);
            if (width <= 0 || height <= 0)
            {
                break;
            }
            _tft.drawRect(x, y, width + 1, height, selectedBorderColour);
        }
    }

    const int32_t centerX = areaLocation.x + (areaSize.width / 2);
    const int32_t labelY = areaLocation.y + (areaSize.height / 2) - 22;

    if (_touchButtonLabelFont != nullptr)
    {
        _tft.setFreeFont(_touchButtonLabelFont);
        _tft.setTextSize(_touchButtonLabelScale);
        _tft.setTextColor(textColour, TFT_BLACK);

        const int32_t labelX = centerX - (_tft.textWidth(label, GFXFF) / 2);
        _tft.drawString(label, labelX, labelY, GFXFF);
    }
    else
    {
        _tft.setTextFont(1);
        _tft.setTextSize(3);
        _tft.setTextColor(textColour, TFT_BLACK);

        const int32_t labelX = centerX - (_tft.textWidth(label) / 2);
        _tft.drawString(label, labelX, labelY);
    }

    const int32_t pillWidth = areaSize.width * 3 / 5;
    const int32_t pillHeight = 12;
    const int32_t pillRadius = pillHeight / 2;
    const int32_t pillX = centerX - (pillWidth / 2);
    const int32_t pillY = labelY + 28;

    drawTouchButtonPill(areaLocation, areaSize, pillColour, _borderColour);
}

void ScreenUi::drawTouchButtonPill(const Point& areaLocation, const Size& areaSize, uint16_t pillColour,
                                   uint16_t pillBorderColour)
{
    const int32_t centerX = areaLocation.x + (areaSize.width / 2);
    const int32_t labelY = areaLocation.y + (areaSize.height / 2) - 22;
    const int32_t pillWidth = areaSize.width * 3 / 5;
    const int32_t pillHeight = 12;
    const int32_t pillRadius = pillHeight / 2;
    const int32_t pillX = centerX - (pillWidth / 2);
    const int32_t pillY = labelY + 28;

    _tft.fillRect(pillX - 1, pillY - 1, pillWidth + 2, pillHeight + 2, TFT_BLACK);

    if (pillColour != 0)
    {
        _tft.fillRoundRect(pillX, pillY, pillWidth, pillHeight, pillRadius, pillColour);
        _tft.drawRoundRect(pillX, pillY, pillWidth, pillHeight, pillRadius, pillBorderColour);
    }
}

void ScreenUi::drawStatusIndicator(int32_t circleX, int32_t circleY, int32_t radius, const char* label, int32_t textX,
                                   int32_t textY, uint16_t colour)
{
    _tft.fillCircle(circleX, circleY, radius, colour);
    _tft.fillRect(textX, textY, _sdStatusTextClearWidth, _sdStatusTextClearHeight, TFT_BLACK);
    drawSmallText(label, textX, textY, TFT_WHITE, TFT_BLACK);
}

void ScreenUi::setSdStatusInitializing() { _sdStatusState = SdStatusState::Initializing; }

void ScreenUi::setSdStatusFailed() { _sdStatusState = SdStatusState::Failed; }

void ScreenUi::setSdStatusReady() { _sdStatusState = SdStatusState::Ready; }

void ScreenUi::setSdStatusNotMounted() { _sdStatusState = SdStatusState::NotMounted; }

void ScreenUi::drawSdStatusInitializing()
{
    setSdStatusInitializing();
    drawStatusIndicator(_sdStatusCircleX, _sdStatusCircleY, _sdStatusRadius, "SD init...", _sdStatusTextX,
                        _sdStatusTextY, TFT_YELLOW);
}

void ScreenUi::drawSdStatusFailed()
{
    setSdStatusFailed();
    drawStatusIndicator(_sdStatusCircleX, _sdStatusCircleY, _sdStatusRadius, "SD failed", _sdStatusTextX,
                        _sdStatusTextY, TFT_RED);
}

void ScreenUi::drawSdStatusReady()
{
    setSdStatusReady();
    drawStatusIndicator(_sdStatusCircleX, _sdStatusCircleY, _sdStatusRadius, "SD ready", _sdStatusTextX, _sdStatusTextY,
                        TFT_GREEN);
}

void ScreenUi::drawSdStatusNotMounted()
{
    setSdStatusNotMounted();
    drawStatusIndicator(_sdStatusCircleX, _sdStatusCircleY, _sdStatusRadius, "SD unmounted", _sdStatusTextX,
                        _sdStatusTextY, TFT_DARKGREY);
}

void ScreenUi::hideSdStatus()
{
    _tft.fillCircle(_sdStatusCircleX, _sdStatusCircleY, _sdStatusRadius + 1, TFT_BLACK);
    _tft.fillRect(_sdStatusTextX, _sdStatusTextY, _sdStatusTextClearWidth, _sdStatusTextClearHeight, TFT_BLACK);
}

void ScreenUi::redrawSdStatus()
{
    switch (_sdStatusState)
    {
    case SdStatusState::Initializing:
        drawSdStatusInitializing();
        break;
    case SdStatusState::Failed:
        drawSdStatusFailed();
        break;
    case SdStatusState::Ready:
        drawSdStatusReady();
        break;
    case SdStatusState::NotMounted:
        drawSdStatusNotMounted();
        break;
    case SdStatusState::None:
    default:
        break;
    }
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
    drawLogoFrame(_logoFrameOuterColour, _logoFrameInnerColour);
}

void ScreenUi::drawLogoFrame(uint16_t outerColour, uint16_t innerColour)
{
    _tft.drawRoundRect(_logoFrameLeft, _logoFrameTop, _logoFrameWidth, _logoFrameHeight, _logoFrameRadius,
                       outerColour);
    _tft.drawRoundRect(_logoFrameLeft + 2, _logoFrameTop + 2, _logoFrameWidth - 4, _logoFrameHeight - 4,
                       _logoFrameRadius - 2, innerColour);
}
