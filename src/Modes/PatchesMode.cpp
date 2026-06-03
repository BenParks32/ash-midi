#include "Modes/PatchesMode.h"

#include <cstdio>
#include <cstring>

namespace
{
const byte MainButtonIndex = 0;
const byte BackButtonIndex = 4;
const uint16_t PatchButtonColour = 0xFFE0;
const uint16_t BackButtonColour = 0xFFFF;
const char* MainLabel = "Main";
const char* BackLabel = "Back";
const char* PatchesSubtitle = "patches";
const uint8_t PatchesTitleScale = 1;
const uint8_t PatchesSubtitleScale = 1;
const int32_t PatchesTitleOffsetY = 26;
const int32_t PatchesSubtitleOffsetY = 72;

const char* patchesDisplayTitle(byte playlistIndex)
{
    switch (playlistIndex)
    {
    case 2:
        return "Project7";
    case 3:
        return "OPR";
    case 4:
        return "Code Red";
    default:
        return "Unknown";
    }
}
}

PatchesMode::PatchesMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, IScreenUi& screenUi,
                         IMidiManager& midiManager, IMidiProvider& midiProvider, IButtonOverrideStore& buttonOverrideStore,
                         IModeTransistionDelegate& transitionDelegate)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _midiProvider(midiProvider), _buttonOverrideStore(buttonOverrideStore),
      _selectedPlaylist(midiProvider.defaultPlaylistIndex()), _currentPatch(0), _buttonPatchNumbers{InvalidPatchNumber}
{
    resetButtons();
}

void PatchesMode::activate()
{
    Serial.printf("Patches mode: loading patches for %s (playlist %u), current patch %u.\n",
                  patchesDisplayTitle(_selectedPlaylist), static_cast<unsigned int>(_selectedPlaylist),
                  static_cast<unsigned int>(_currentPatch));
    populateButtons();
    renderAllButtons();
    renderCenterTitle(TFT_WHITE);
}

void PatchesMode::deactivate() { renderCenterTitle(TFT_BLACK); }

void PatchesMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT || !isButtonEnabled(number))
    {
        return;
    }

    if (number == BackButtonIndex)
    {
        const ModeTransitionValue transitionValue = makePlayModeTransition(_selectedPlaylist, _currentPatch, false);
        _transitionDelegate.enterMode(Modes::Play, transitionValue);
        return;
    }

    const byte patchNumber = _buttonPatchNumbers[number];
    if (patchNumber == InvalidPatchNumber)
    {
        return;
    }

    const ModeTransitionValue transitionValue = makePlayModeTransition(_selectedPlaylist, patchNumber, true);
    _transitionDelegate.enterMode(Modes::Play, transitionValue);
}

void PatchesMode::buttonLongPressed(byte number)
{
    (void)number;
}

void PatchesMode::frameTick()
{
    // Patches mode currently has no per-frame work.
}

void PatchesMode::setTransitionValue(ModeTransitionValue transitionValue)
{
    if (transitionValue == ModeTransitionNone)
    {
        _selectedPlaylist = _midiProvider.defaultPlaylistIndex();
        _currentPatch = 0;
        return;
    }

    if (isPlayModeTransition(transitionValue))
    {
        _selectedPlaylist = playModeTransitionPlaylist(transitionValue);
        _currentPatch = playModeTransitionPatch(transitionValue);
        return;
    }

    if ((transitionValue & ModeTransitionHomePlaylistFlag) != 0)
    {
        _selectedPlaylist = static_cast<byte>(transitionValue & ModeTransitionHomePlaylistValueMask);
        _currentPatch = 0;
        return;
    }

    _currentPatch = static_cast<byte>(transitionValue & ModeTransitionPatchValueMask);
}

void PatchesMode::resetButtons()
{
    for (byte buttonIndex = 0; buttonIndex < TouchButtonManager::BUTTON_COUNT; ++buttonIndex)
    {
        _functions[buttonIndex] = Function();
        _buttonPatchNumbers[buttonIndex] = InvalidPatchNumber;
    }
}

void PatchesMode::populateButtons()
{
    resetButtons();
    configureButton(MainButtonIndex, 0, MainLabel);
    configureBackButton();

    PatchListEntry entries[TouchButtonManager::BUTTON_COUNT] = {};
    const size_t entryCount = _buttonOverrideStore.listPatches(_selectedPlaylist, entries, TouchButtonManager::BUTTON_COUNT);
    Serial.printf("Patches mode: listPatches returned %u entries for playlist %u.\n",
                  static_cast<unsigned int>(entryCount), static_cast<unsigned int>(_selectedPlaylist));

    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        Serial.printf("Patches mode: source entry %u patch %u name '%s'.\n", static_cast<unsigned int>(entryIndex),
                      static_cast<unsigned int>(entries[entryIndex].patchNumber),
                      entries[entryIndex].name[0] != '\0' ? entries[entryIndex].name : "<empty>");
    }

    byte nextButtonIndex = 1;
    for (size_t entryIndex = 0; entryIndex < entryCount && nextButtonIndex < TouchButtonManager::BUTTON_COUNT; ++entryIndex)
    {
        if (entries[entryIndex].patchNumber == 0)
        {
            Serial.printf("Patches mode: skipping patch 0 entry %u.\n", static_cast<unsigned int>(entryIndex));
            continue;
        }

        if (nextButtonIndex == BackButtonIndex)
        {
            ++nextButtonIndex;
        }

        if (nextButtonIndex >= TouchButtonManager::BUTTON_COUNT)
        {
            break;
        }

        char label[Function::LabelCapacity] = {'\0'};
        formatPatchLabel(entries[entryIndex].patchNumber, entries[entryIndex].name, label, sizeof(label));
        configureButton(nextButtonIndex, entries[entryIndex].patchNumber, label);
        Serial.printf("Patches mode: mapped entry %u patch %u to button %u label '%s'.\n",
                      static_cast<unsigned int>(entryIndex), static_cast<unsigned int>(entries[entryIndex].patchNumber),
                      static_cast<unsigned int>(nextButtonIndex), label);
        ++nextButtonIndex;
    }
}

void PatchesMode::configureButton(byte buttonIndex, byte patchNumber, const char* label)
{
    if (buttonIndex >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    _functions[buttonIndex] = Function(label, PatchButtonColour, ActionType::None, ActionType::None);
    _buttonPatchNumbers[buttonIndex] = patchNumber;
}

void PatchesMode::configureBackButton()
{
    _functions[BackButtonIndex] = Function(BackLabel, BackButtonColour, ActionType::None, ActionType::None);
    _buttonPatchNumbers[BackButtonIndex] = InvalidPatchNumber;
}

void PatchesMode::renderCenterTitle(uint16_t textColour)
{
    const int32_t centerX = _screenUi.boxWidth() * 2;
    const int32_t centerTopY = _screenUi.boxHeight();

    _screenUi.drawCenteredText(FF32, PatchesTitleScale, patchesDisplayTitle(_selectedPlaylist), centerX,
                               centerTopY + PatchesTitleOffsetY, textColour, TFT_BLACK);
    _screenUi.drawCenteredText(FF22, PatchesSubtitleScale, PatchesSubtitle, centerX,
                               centerTopY + PatchesSubtitleOffsetY, textColour, TFT_BLACK);
}

void PatchesMode::formatPatchLabel(byte patchNumber, const char* patchName, char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0U)
    {
        return;
    }

    if (patchName != nullptr && patchName[0] != '\0')
    {
        std::snprintf(buffer, bufferSize, "%s", patchName);
        return;
    }

    std::snprintf(buffer, bufferSize, "%02u", static_cast<unsigned int>(patchNumber));
}

uint8_t PatchesMode::ringBrightnessForButton(byte number) const
{
    (void)number;
    return RingManager::FullBrightness;
}
