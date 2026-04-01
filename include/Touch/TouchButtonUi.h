#pragma once

#include <Arduino.h>

#include "Gfx.h"

class ITouchButtonCanvas
{
  public:
    static constexpr uint16_t White565 = 0xFFFF;

    virtual ~ITouchButtonCanvas() = default;

    virtual void drawTouchButtonLabelAndPill(const char* label, const Point& areaLocation, const Size& areaSize,
                                             uint16_t pillColour, bool selected = false,
                                             uint16_t selectedBorderColour = White565,
                                             uint16_t textColour = White565) = 0;
};

class ITouchButtonLayout : public ITouchButtonCanvas
{
  public:
    virtual ~ITouchButtonLayout() = default;

    virtual int32_t boxWidth() const = 0;
    virtual int32_t bottomRowY() const = 0;
    virtual Size boxSize() const = 0;
};
