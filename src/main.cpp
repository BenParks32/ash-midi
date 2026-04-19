#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>

#include "ButtonHandler.h"
#include "Do.h"
#include "Encoder.h"
#include "Gfx.h"
#include "MidiManager.h"
#include "Modes/ButtonDiagnosticMode.h"
#include "Modes/HomeMode.h"
#include "Modes/MenuMode.h"
#include "Modes/Mode.h"
#include "Modes/ModeManager.h"
#include "Modes/PatchMode.h"
#include "Modes/PlayMode.h"
#include "Resources.h"
#include "RingManager.h"
#include "ScreenUi.h"
#include "SettingsStore.h"
#include "TFT_Setup.h"
#include "Touch/TouchButtonManager.h"

#define LED_PIN PA0

#define LED_COUNT RingManager::LedCount

#define SD_CS PA4
#define ENCODER_PIN_A PB0
#define ENCODER_PIN_B PB1
#define ENCODER_BUTTON_PIN PB5

const Size screenSize = {480, 320};
uint16_t calData[5] = {254, 3649, 281, 3563, 7};

namespace
{
} // namespace

void HandleTouch();
void HandleEncoder();
void HandleSerialDiagnosticsCommands();

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();
ScreenUi screenUi(tft, screenSize);
RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON_PIN);
RingManager ringManager(strip);

const GFXfont* DefaultUiFont = FF22;
const uint8_t DefaultUiScale = 1;

Resources resources(SD_CS);
IMode* activeMode = nullptr;

ButtonHandler buttonHandler(activeMode, ringManager);
TouchButtonManager touchButtonManager(screenUi, &buttonHandler);
MidiManager midiManager;
SettingsStore settingsStore(&resources);
AppSettings appSettings = SettingsStore::defaults();
IMode* modeRegistry[ModeCount] = {nullptr};
ModeManager modeManager(activeMode, modeRegistry);
PlayMode playMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);
PatchMode patchMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);
HomeMode homeMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);
MenuMode menuMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager, settingsStore, resources,
                  appSettings);
ButtonDiagnosticMode buttonDiagnosticMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);

bool initResourcesSD()
{
    Serial.println("Initializing SD card...");
    screenUi.setSdStatusInitializing();

    if (!resources.init())
    {
        Serial.println("SD card initialization failed!");
        screenUi.setSdStatusFailed();
        return false;
    }

    Serial.println("SD card initialized successfully.");
    screenUi.setSdStatusReady();
    return true;
}

void setup()
{
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);

    Serial.begin(115200);
    delay(250);
    initResourcesSD();

    if (!encoder.begin())
    {
        Serial.println("Encoder initialization failed.");
    }
    buttonHandler.begin();

    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    ringManager.begin();

    settingsStore.load(appSettings);
    ringManager.setMasterBrightness(appSettings.masterBrightness);
    midiManager.setChannel(appSettings.midiChannel);

    screenUi.drawBackgroundAndBorder();
    screenUi.redrawSdStatus();
    screenUi.setTouchButtonLabelStyle(DefaultUiFont, DefaultUiScale);

    touchButtonManager.initialize();

    modeRegistry[static_cast<uint8_t>(Modes::Home)] = &homeMode;
    modeRegistry[static_cast<uint8_t>(Modes::Play)] = &playMode;
    modeRegistry[static_cast<uint8_t>(Modes::Patch)] = &patchMode;
    modeRegistry[static_cast<uint8_t>(Modes::Menu)] = &menuMode;
    modeRegistry[static_cast<uint8_t>(Modes::ButtonDiagnostic)] = &buttonDiagnosticMode;

    activeMode = &homeMode;
    activeMode->activate();
}

void loop()
{
    HandleSerialDiagnosticsCommands();

    HandleEncoder();
    buttonHandler.updateButtons();
    HandleTouch();

    if (activeMode != nullptr)
    {
        activeMode->frameTick();
    }
}

void HandleEncoder()
{
    const int16_t steps = encoder.consumeSteps();
    if (activeMode != nullptr && steps != 0)
    {
        activeMode->encoderRotated(steps);
    }

    if (activeMode != nullptr && encoder.consumeButtonPress())
    {
        activeMode->encoderPressed();
    }
}

void HandleTouch()
{
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        touchButtonManager.handleTouch(x, y);
        return;
    }

    touchButtonManager.handleTouchRelease();
}

void HandleSerialDiagnosticsCommands()
{
    while (Serial.available() > 0)
    {
        Serial.read();
    }
}
