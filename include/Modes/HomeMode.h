#pragma once

#include "Modes/Mode.h"

#include "MidiManager.h"
#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButtonManager.h"

class HomeMode : public IMode
{
  public:
    HomeMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
             IMidiManager& midiManager);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;

  private:
    const char* functionLabel(byte number) const;
    bool isButtonEnabled(byte number) const;
    void sendProgramChangeForButton(byte number);
    static bool tryGetProgramChangeValue(byte number, byte& programChangeValue);
    static bool isEmptyLabel(const char* label);

  private:
    TouchButtonManager& _touchButtonManager;
    RingManager& _ringManager;
    ScreenUi& _screenUi;
    IMidiManager& _midiManager;
};
