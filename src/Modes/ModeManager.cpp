#include "Modes/ModeManager.h"

ModeManager::ModeManager(IMode*& activeMode, IMode* (&modes)[ModeCount]) : _activeMode(activeMode), _modes(modes) {}

void ModeManager::enterMode(Modes mode, byte transitionValue)
{
    const uint8_t index = static_cast<uint8_t>(mode);
    if (index >= ModeCount)
    {
        return;
    }

    IMode* nextMode = _modes[index];
    if (nextMode == nullptr)
    {
        return;
    }

    if (_activeMode != nullptr && _activeMode != nextMode)
    {
        _activeMode->deactivate();
    }

    nextMode->setTransitionValue(transitionValue);
    _activeMode = nextMode;
    _activeMode->activate();
}
