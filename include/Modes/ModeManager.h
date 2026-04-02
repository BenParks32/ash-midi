#pragma once

#include "Modes/Mode.h"

class ModeManager : public IModeTransistionDelegate
{
  public:
    ModeManager(IMode*& activeMode, IMode* (&modes)[ModeCount]);

    void enterMode(Modes mode, byte transitionValue) override;

  private:
    ModeManager();
    ModeManager(const ModeManager&);
    ModeManager& operator=(const ModeManager&);

  private:
    IMode*& _activeMode;
    IMode* (&_modes)[ModeCount];
};
