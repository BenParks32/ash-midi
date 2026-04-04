#pragma once

#include "Modes/Mode.h"

#include "Function.h"
#include "MidiManager.h"
#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButtonManager.h"

class FunctionModeBase : public IMode
{
  public:
    FunctionModeBase(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                     IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate);
    virtual ~FunctionModeBase() = default;

  private:
    FunctionModeBase() = delete;
    FunctionModeBase(const FunctionModeBase&) = delete;
    FunctionModeBase& operator=(const FunctionModeBase&) = delete;

  protected:
    const Function& getFunction(byte number) const;
    bool isButtonEnabled(byte number) const;
    static bool isEmptyLabel(const char* label);
    void renderAllButtons();
    virtual uint8_t ringBrightnessForButton(byte number) const = 0;

    TouchButtonManager& _touchButtonManager;
    RingManager& _ringManager;
    ScreenUi& _screenUi;
    IMidiManager& _midiManager;
    IModeTransistionDelegate& _transitionDelegate;
    Function _functions[TouchButtonManager::BUTTON_COUNT];
};