#pragma once

#include "Modes/FunctionModeBase.h"
#include "SettingsStore.h"

class MenuMode : public FunctionModeBase
{
  public:
    MenuMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
             IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate, ISettingsStore& settingsStore,
             AppSettings& settings);

    void activate() override;
    void buttonPressed(byte number) override;
    void buttonLongPressed(byte number) override;
    void frameTick() override;
    void encoderRotated(int16_t steps) override;
    void encoderPressed() override;

  private:
    MenuMode() = delete;
    MenuMode(const MenuMode&) = delete;
    MenuMode& operator=(const MenuMode&) = delete;

  private:
    enum class MenuItem : uint8_t
    {
        Brightness = 0,
        MidiChannel = 1,
        Count,
    };

  private:
    void setupFunctions();
    void renderMenu();
    void renderStaticMenuPanels();
    void renderMenuItem(MenuItem item, bool hasLeftFocus);
    void renderValuePanel(bool hasRightFocus);
    void renderValueLabel(bool hasRightFocus);
    void renderSavingIndicator(bool visible);
    int32_t menuListStartY() const;
    int32_t menuRowY(MenuItem item) const;
    void moveSelection(int16_t steps);
    void applyEditDelta(int16_t steps);
    void applyRandomRingColours();
    static const char* menuItemLabel(MenuItem item);
    void formatSelectedValue(MenuItem item, char* buffer, size_t bufferSize) const;
    uint8_t ringBrightnessForButton(byte number) const override;

  private:
    ISettingsStore& _settingsStore;
    AppSettings& _settings;
    MenuItem _selectedItem;
    bool _isEditMode;
};
