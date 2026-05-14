#pragma once

#include <TFT_eSPI.h>

#include "SettingsStore.h"

class ITouchCalibrator
{
  public:
    virtual ~ITouchCalibrator() = default;

    virtual bool calibrate(TouchCalibrationData& outCalibration) = 0;
    virtual void apply(const TouchCalibrationData& calibration) = 0;
};

class TftTouchCalibrator : public ITouchCalibrator
{
  public:
    explicit TftTouchCalibrator(TFT_eSPI& tft);

    bool calibrate(TouchCalibrationData& outCalibration) override;
    void apply(const TouchCalibrationData& calibration) override;

  private:
    TFT_eSPI& _tft;
    TouchCalibrationData _appliedCalibration = {};
};
