#pragma once

#include "Modes/Mode.h"

#include "Function.h"
#include "MidiManager.h"
#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButtonManager.h"

class PlayMode : public IMode
{
  public:
    PlayMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
             IMidiManager& midiManager);

    void setSelectedHomeProgramChange(byte selectedHomeProgramChange);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void setTransitionValue(byte transitionValue) override;

  private:
    const Function& getFunction(byte number) const;
    bool isButtonEnabled(byte number) const;
    void executeAction(ActionType action, byte actionValue);
    void updateVisuals();
    static bool isEmptyLabel(const char* label);
    void setupFunctions();

  private:
    TouchButtonManager& _touchButtonManager;
    RingManager& _ringManager;
    ScreenUi& _screenUi;
    IMidiManager& _midiManager;

    byte _selectedHomeProgramChange;
    byte _selectedButton;
    Function _functions[TouchButtonManager::BUTTON_COUNT];
};
