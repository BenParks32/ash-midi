#include "TouchCalibration.h"

namespace
{
void copyCalibrationData(TouchCalibrationData& target, const TouchCalibrationData& source)
{
    for (uint8_t i = 0; i < TouchCalibrationData::WordCount; ++i)
    {
        target.values[i] = source.values[i];
    }
}
} // namespace

TftTouchCalibrator::TftTouchCalibrator(TFT_eSPI& tft) : _tft(tft) {}

bool TftTouchCalibrator::calibrate(TouchCalibrationData& outCalibration)
{
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextFont(2);
    _tft.drawString("Touch corners to calibrate", 20, 10);
    _tft.calibrateTouch(outCalibration.values, TFT_MAGENTA, TFT_BLACK, 15);
    return true;
}

void TftTouchCalibrator::apply(const TouchCalibrationData& calibration)
{
    copyCalibrationData(_appliedCalibration, calibration);
    _tft.setTouch(_appliedCalibration.values);
}
