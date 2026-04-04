#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>

#include "ButtonHandler.h"
#include "Do.h"
#include "Encoder.h"
#include "Gfx.h"
#include "MidiManager.h"
#include "Modes/HomeMode.h"
#include "Modes/Mode.h"
#include "Modes/ModeManager.h"
#include "Modes/PatchMode.h"
#include "Modes/PlayMode.h"
#include "Resources.h"
#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButtonManager.h"

#define LED_PIN PA0

#define LED_COUNT RingManager::LedCount

#define SD_CS PA4
#define ENCODER_PIN_A PB0
#define ENCODER_PIN_B PB1

const Size screenSize = {480, 320};
const uint8_t BrightnessStep = 8;
uint16_t calData[5] = {254, 3649, 281, 3563, 7};

void HandleTouch();
void HandleEncoder();

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();
ScreenUi screenUi(tft, screenSize);
RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);
RingManager ringManager(strip);

const GFXfont* DefaultUiFont = FF22;
const uint8_t DefaultUiScale = 1;

Resources resources(SD_CS);
IMode* activeMode = nullptr;

ButtonHandler buttonHandler(activeMode, ringManager);
TouchButtonManager touchButtonManager(screenUi, &buttonHandler);
MidiManager midiManager;
IMode* modeRegistry[ModeCount] = {nullptr};
ModeManager modeManager(activeMode, modeRegistry);
PlayMode playMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);
PatchMode patchMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);
HomeMode homeMode(touchButtonManager, ringManager, screenUi, midiManager, modeManager);

bool initResourcesSD()
{
    Serial.println("Initializing SD card...");
    screenUi.drawSdStatusInitializing();

    if (!resources.init())
    {
        Serial.println("SD card initialization failed!");
        screenUi.drawSdStatusFailed();
        return false;
    }

    if (!resources.loadAll())
    {
        Serial.println("Failed to load resources!");
        screenUi.drawSdStatusFailed();
        return false;
    }

    Serial.println("SD card initialized and resources loaded successfully.");
    screenUi.drawSdStatusReady();
    return true;
}

void setup()
{
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    Serial.begin(115200);
    if (!encoder.begin())
    {
        Serial.println("Encoder initialization failed.");
    }
    buttonHandler.begin();

    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    ringManager.begin();

    screenUi.drawBackgroundAndBorder();
    screenUi.setTouchButtonLabelStyle(DefaultUiFont, DefaultUiScale);

    touchButtonManager.initialize();

    initResourcesSD();

    modeRegistry[static_cast<uint8_t>(Modes::Home)] = &homeMode;
    modeRegistry[static_cast<uint8_t>(Modes::Play)] = &playMode;
    modeRegistry[static_cast<uint8_t>(Modes::Patch)] = &patchMode;

    activeMode = &homeMode;
    activeMode->activate();
}

void loop()
{
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

    if (!ringManager.adjustMasterBrightness(steps, BrightnessStep))
    {
        return;
    }

    Serial.printf("Master ring brightness: %u\n", ringManager.masterBrightness());
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
