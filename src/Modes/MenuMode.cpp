#include "Modes/MenuMode.h"

#include <cstdio>

namespace
{
constexpr uint8_t MenuItemScale = 1;
constexpr uint8_t MenuValueScale = 1;
constexpr byte MenuExitButtonIndex = 7;
constexpr uint8_t BrightnessStep = 8;
constexpr int32_t PanelPadding = 8;
constexpr int32_t SplitLineWidth = 2;
constexpr int32_t RowHeight = 32;
constexpr int32_t RowGap = 16;
constexpr int32_t RowHorizontalPadding = 8;
constexpr int32_t ValuePanelHorizontalPadding = 12;
constexpr int32_t ValuePanelSizePercent = 105;
constexpr int32_t SavingIndicatorWidth = 94;
constexpr int32_t SavingIndicatorHeight = 18;

struct MenuLayout
{
    int32_t screenWidth;
    int32_t centerTopY;
    int32_t centerBottomY;
    int32_t centerHeight;

    int32_t splitX;

    int32_t leftPanelX;
    int32_t leftPanelY;
    int32_t leftPanelWidth;
    int32_t panelHeight;

    int32_t rightPanelX;
    int32_t rightPanelWidth;

    int32_t valuePanelX;
    int32_t valuePanelY;
    int32_t valuePanelWidth;
    int32_t valuePanelHeight;
};

MenuLayout buildMenuLayout(const ScreenUi& screenUi)
{
    MenuLayout layout = {};
    layout.screenWidth = screenUi.boxWidth() * 4;
    layout.centerTopY = screenUi.boxHeight();
    layout.centerBottomY = screenUi.bottomRowY();
    layout.centerHeight = layout.centerBottomY - layout.centerTopY;

    layout.splitX = (layout.screenWidth * 2) / 3;

    layout.leftPanelX = PanelPadding;
    layout.leftPanelY = layout.centerTopY + PanelPadding;
    layout.leftPanelWidth = layout.splitX - layout.leftPanelX - PanelPadding;
    layout.panelHeight = layout.centerHeight - (PanelPadding * 2);

    layout.rightPanelX = layout.splitX + SplitLineWidth + PanelPadding;
    layout.rightPanelWidth = layout.screenWidth - layout.rightPanelX - PanelPadding;

    const int32_t baseValuePanelX = layout.rightPanelX + ValuePanelHorizontalPadding;
    const int32_t baseValuePanelY = layout.centerTopY + 26;
    const int32_t baseValuePanelWidth = layout.rightPanelWidth - (ValuePanelHorizontalPadding * 2);
    const int32_t baseValuePanelHeight = layout.centerHeight - 52;

    layout.valuePanelWidth = (baseValuePanelWidth * ValuePanelSizePercent) / 100;
    layout.valuePanelHeight = (baseValuePanelHeight * ValuePanelSizePercent) / 100;
    layout.valuePanelX = baseValuePanelX - ((layout.valuePanelWidth - baseValuePanelWidth) / 2);
    layout.valuePanelY = baseValuePanelY - ((layout.valuePanelHeight - baseValuePanelHeight) / 2);

    return layout;
}

uint32_t randomRingColour()
{
    const uint8_t red = static_cast<uint8_t>(random(256));
    const uint8_t green = static_cast<uint8_t>(random(256));
    const uint8_t blue = static_cast<uint8_t>(random(256));
    return (static_cast<uint32_t>(red) << 16) | (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
}
} // namespace

MenuMode::MenuMode(TouchButtonManager& touchButtonManager, RingManager& ringManager, ScreenUi& screenUi,
                   IMidiManager& midiManager, IModeTransistionDelegate& transitionDelegate,
                   ISettingsStore& settingsStore, ISdCardManager& sdCardManager, AppSettings& settings)
    : FunctionModeBase(touchButtonManager, ringManager, screenUi, midiManager, transitionDelegate),
      _settingsStore(settingsStore), _sdCardManager(sdCardManager), _settings(settings),
      _selectedItem(MenuItem::Brightness), _isEditMode(false)
{
    setupFunctions();
}

void MenuMode::setupFunctions()
{
    for (byte i = 0; i < TouchButtonManager::BUTTON_COUNT; ++i)
    {
        _functions[i] = Function();
    }

    _functions[MenuExitButtonIndex] =
        Function("Home", 0xFFFF, ActionType::ChangeMode, static_cast<byte>(Modes::Home), ActionType::None, 0);
}

void MenuMode::activate()
{
    _isEditMode = false;
    _screenUi.hideSdStatus();
    renderAllButtons();
    applyRandomRingColours();
    renderMenu();
}

void MenuMode::deactivate() { clearMenu(); }

void MenuMode::buttonPressed(byte number)
{
    if (number >= TouchButtonManager::BUTTON_COUNT)
    {
        return;
    }

    if (number != MenuExitButtonIndex)
    {
        return;
    }

    _transitionDelegate.enterMode(Modes::Home, ModeTransitionNone);
}

void MenuMode::buttonLongPressed(byte number)
{
    (void)number;
    // Menu mode is encoder-only by design.
}

void MenuMode::frameTick()
{
    // Menu mode currently has no per-frame animation.
}

void MenuMode::encoderRotated(int16_t steps)
{
    if (steps == 0)
    {
        return;
    }

    if (_isEditMode)
    {
        applyEditDelta(steps);
    }
    else
    {
        moveSelection(steps);
    }
}

void MenuMode::encoderPressed()
{
    if (_selectedItem == MenuItem::SdCard)
    {
        _isEditMode = false;
        handleSdCardAction();
        return;
    }

    if (_isEditMode)
    {
        _isEditMode = false;
        renderMenuItem(_selectedItem, true);
        renderValuePanel(false);
        renderSavingIndicator(true);

        _settingsStore.save(_settings);

        renderSavingIndicator(false);
        return;
    }

    _isEditMode = true;
    renderMenuItem(_selectedItem, false);
    renderValuePanel(true);
}

void MenuMode::moveSelection(int16_t steps)
{
    const int16_t itemCount = static_cast<int16_t>(MenuItem::Count);
    int16_t nextIndex = static_cast<int16_t>(_selectedItem);

    nextIndex = static_cast<int16_t>((nextIndex + steps) % itemCount);
    if (nextIndex < 0)
    {
        nextIndex = static_cast<int16_t>(nextIndex + itemCount);
    }

    if (nextIndex == static_cast<int16_t>(_selectedItem))
    {
        return;
    }

    const MenuItem previousItem = _selectedItem;
    _selectedItem = static_cast<MenuItem>(nextIndex);
    renderMenuItem(previousItem, false);
    renderMenuItem(_selectedItem, !_isEditMode);
    renderValuePanel(_isEditMode);
}

void MenuMode::applyEditDelta(int16_t steps)
{
    if (_selectedItem == MenuItem::Brightness)
    {
        const int32_t requested = static_cast<int32_t>(_settings.masterBrightness) +
                                  (static_cast<int32_t>(steps) * static_cast<int32_t>(BrightnessStep));

        int32_t clamped = requested;
        if (clamped < 0)
        {
            clamped = 0;
        }
        else if (clamped > RingManager::MaxBrightness)
        {
            clamped = RingManager::MaxBrightness;
        }

        const uint8_t nextValue = static_cast<uint8_t>(clamped);
        if (nextValue != _settings.masterBrightness)
        {
            _settings.masterBrightness = nextValue;
            _ringManager.setMasterBrightness(nextValue);
            _ringManager.show();
            renderValuePanel(true);
        }
        return;
    }

    if (_selectedItem == MenuItem::MidiChannel)
    {
        const int32_t requested = static_cast<int32_t>(_settings.midiChannel) + steps;

        int32_t clamped = requested;
        if (clamped < 1)
        {
            clamped = 1;
        }
        else if (clamped > 16)
        {
            clamped = 16;
        }

        const uint8_t nextValue = static_cast<uint8_t>(clamped);
        if (nextValue != _settings.midiChannel)
        {
            _settings.midiChannel = nextValue;
            _midiManager.setChannel(nextValue);
            renderValuePanel(true);
        }
    }
}

void MenuMode::handleSdCardAction()
{
    if (_sdCardManager.isMounted())
    {
        if (_sdCardManager.unmount())
        {
            Serial.println("Menu: SD card unmounted.");
            _screenUi.setSdStatusNotMounted();
        }
    }
    else
    {
        if (_sdCardManager.mount())
        {
            Serial.println("Menu: SD card mounted.");
            _screenUi.setSdStatusReady();
        }
        else
        {
            Serial.println("Menu: SD card mount failed.");
            _screenUi.setSdStatusFailed();
        }
    }

    renderMenuItem(_selectedItem, true);
    renderValuePanel(false);
}

void MenuMode::applyRandomRingColours()
{
    for (uint8_t ringIndex = 0; ringIndex < RingManager::RingCount; ++ringIndex)
    {
        _ringManager.setRingColour(ringIndex, randomRingColour());
        _ringManager.setRingBrightness(ringIndex, RingManager::FullBrightness);
    }

    _ringManager.show();
}

const char* MenuMode::menuItemLabel(MenuMode::MenuItem item)
{
    switch (item)
    {
    case MenuItem::Brightness:
        return "Brightness";
    case MenuItem::MidiChannel:
        return "MIDI Channel";
    case MenuItem::SdCard:
        return "SD Card";
    case MenuItem::Count:
    default:
        return "";
    }
}

void MenuMode::formatSelectedValue(MenuMode::MenuItem item, char* buffer, size_t bufferSize) const
{
    if (buffer == nullptr || bufferSize == 0)
    {
        return;
    }

    switch (item)
    {
    case MenuItem::Brightness:
    {
        const uint16_t percentage = static_cast<uint16_t>((static_cast<uint32_t>(_settings.masterBrightness) * 100U) /
                                                          RingManager::MaxBrightness);
        std::snprintf(buffer, bufferSize, "%u%%", static_cast<unsigned int>(percentage));
        break;
    }
    case MenuItem::MidiChannel:
        std::snprintf(buffer, bufferSize, "%u", static_cast<unsigned int>(_settings.midiChannel));
        break;
    case MenuItem::SdCard:
        std::snprintf(buffer, bufferSize, "%s", _sdCardManager.isMounted() ? "Unmount" : "Mount");
        break;
    case MenuItem::Count:
    default:
        buffer[0] = '\0';
        break;
    }
}

void MenuMode::renderMenu()
{
    renderStaticMenuPanels();

    for (uint8_t itemIndex = 0; itemIndex < static_cast<uint8_t>(MenuItem::Count); ++itemIndex)
    {
        const MenuItem item = static_cast<MenuItem>(itemIndex);
        renderMenuItem(item, (item == _selectedItem) && !_isEditMode);
    }

    renderValuePanel(_isEditMode);
}

void MenuMode::clearMenu()
{
    const MenuLayout layout = buildMenuLayout(_screenUi);

    _screenUi.fillRect(layout.splitX, layout.leftPanelY, SplitLineWidth, layout.panelHeight, TFT_BLACK);

    for (uint8_t itemIndex = 0; itemIndex < static_cast<uint8_t>(MenuItem::Count); ++itemIndex)
    {
        const MenuItem item = static_cast<MenuItem>(itemIndex);
        const int32_t rowY = menuRowY(item);
        const int32_t rowX = layout.leftPanelX + RowHorizontalPadding;
        const int32_t rowWidth = layout.leftPanelWidth - (RowHorizontalPadding * 2);

        _screenUi.fillRect(rowX, rowY, rowWidth, RowHeight, TFT_BLACK);
        _screenUi.drawText(FF22, MenuItemScale, menuItemLabel(item), rowX + 10, rowY + 6, TFT_BLACK, TFT_BLACK);
    }

    _screenUi.fillRect(layout.valuePanelX, layout.valuePanelY, layout.valuePanelWidth, layout.valuePanelHeight,
                       TFT_BLACK);
    _screenUi.drawRect(layout.valuePanelX, layout.valuePanelY, layout.valuePanelWidth, layout.valuePanelHeight,
                       TFT_BLACK);

    char valueLabel[16] = {'\0'};
    formatSelectedValue(_selectedItem, valueLabel, sizeof(valueLabel));

    const int32_t valueCenterX = layout.valuePanelX + (layout.valuePanelWidth / 2);
    const GFXfont* valueFont = (_selectedItem == MenuItem::SdCard) ? FF22 : FF32;
    const int32_t valueTextHeight = (_selectedItem == MenuItem::SdCard) ? 16 : 24;
    const int32_t valueY = layout.valuePanelY + ((layout.valuePanelHeight - valueTextHeight) / 2);

    _screenUi.drawCenteredText(valueFont, MenuValueScale, valueLabel, valueCenterX, valueY, TFT_BLACK, TFT_BLACK);
    renderSavingIndicator(false);
}

void MenuMode::renderStaticMenuPanels()
{
    const MenuLayout layout = buildMenuLayout(_screenUi);
    _screenUi.fillRect(layout.splitX, layout.leftPanelY, SplitLineWidth, layout.panelHeight, TFT_DARKGREY);
}

int32_t MenuMode::menuListStartY() const
{
    const MenuLayout layout = buildMenuLayout(_screenUi);
    const int32_t listHeight =
        (static_cast<int32_t>(MenuItem::Count) * RowHeight) + ((static_cast<int32_t>(MenuItem::Count) - 1) * RowGap);

    return layout.centerTopY + ((layout.centerHeight - listHeight) / 2);
}

int32_t MenuMode::menuRowY(MenuMode::MenuItem item) const
{
    return menuListStartY() + (static_cast<int32_t>(item) * (RowHeight + RowGap));
}

void MenuMode::renderMenuItem(MenuMode::MenuItem item, bool hasLeftFocus)
{
    const MenuLayout layout = buildMenuLayout(_screenUi);
    const int32_t rowY = menuRowY(item);
    const int32_t rowX = layout.leftPanelX + RowHorizontalPadding;
    const int32_t rowWidth = layout.leftPanelWidth - (RowHorizontalPadding * 2);

    _screenUi.fillRect(rowX, rowY, rowWidth, RowHeight, hasLeftFocus ? TFT_WHITE : TFT_BLACK);
    _screenUi.drawText(FF22, MenuItemScale, menuItemLabel(item), rowX + 10, rowY + 6,
                       hasLeftFocus ? TFT_BLACK : TFT_WHITE, hasLeftFocus ? TFT_WHITE : TFT_BLACK);
}

void MenuMode::renderValuePanel(bool hasRightFocus)
{
    const MenuLayout layout = buildMenuLayout(_screenUi);

    _screenUi.fillRect(layout.valuePanelX, layout.valuePanelY, layout.valuePanelWidth, layout.valuePanelHeight,
                       hasRightFocus ? TFT_WHITE : TFT_BLACK);
    _screenUi.drawRect(layout.valuePanelX, layout.valuePanelY, layout.valuePanelWidth, layout.valuePanelHeight,
                       hasRightFocus ? TFT_BLACK : TFT_DARKGREY);

    renderValueLabel(hasRightFocus);
}

void MenuMode::renderValueLabel(bool hasRightFocus)
{
    const MenuLayout layout = buildMenuLayout(_screenUi);

    char valueLabel[16] = {'\0'};
    formatSelectedValue(_selectedItem, valueLabel, sizeof(valueLabel));

    const int32_t valueCenterX = layout.valuePanelX + (layout.valuePanelWidth / 2);
    const GFXfont* valueFont = (_selectedItem == MenuItem::SdCard) ? FF22 : FF32;
    const int32_t valueTextHeight = (_selectedItem == MenuItem::SdCard) ? 16 : 24;
    const int32_t valueY = layout.valuePanelY + ((layout.valuePanelHeight - valueTextHeight) / 2);

    _screenUi.drawCenteredText(valueFont, MenuValueScale, valueLabel, valueCenterX, valueY,
                               hasRightFocus ? TFT_BLACK : TFT_WHITE, hasRightFocus ? TFT_WHITE : TFT_BLACK);
}

void MenuMode::renderSavingIndicator(bool visible)
{
    const MenuLayout layout = buildMenuLayout(_screenUi);
    const int32_t indicatorX = layout.valuePanelX + ((layout.valuePanelWidth - SavingIndicatorWidth) / 2);
    const int32_t indicatorY = layout.valuePanelY + layout.valuePanelHeight - SavingIndicatorHeight - 8;
    _screenUi.drawSmallText("Saving..", indicatorX, indicatorY + 2, visible ? TFT_WHITE : TFT_BLACK, TFT_BLACK);
}

uint8_t MenuMode::ringBrightnessForButton(byte number) const
{
    if (number == MenuExitButtonIndex)
    {
        return RingManager::FullBrightness;
    }

    return 0;
}
