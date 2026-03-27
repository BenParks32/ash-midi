#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>

#include "Button.h"
#include "Do.h"
#include "Encoder.h"
#include "Gfx.h"
#include "Resources.h"
#include "RingManager.h"
#include "ScreenUi.h"
#include "TouchButton.h"

#define LED_PIN PA0
#define BUTTON_COUNT RingManager::RingCount
#define TOUCH_BUTTON_COUNT BUTTON_COUNT

#define LED_COUNT RingManager::LedCount

#define SD_CS PA4
#define ENCODER_PIN_A PB0
#define ENCODER_PIN_B PB1

const Size screenSize = {480, 320};
const uint8_t BrightnessStep = 8;
uint16_t calData[5] = {254, 3649, 281, 3563, 7};

void HandleButtons();
void HandleTouch();
void HandleEncoder();

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();
ScreenUi screenUi(tft, screenSize);
RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);
RingManager ringManager(strip);

const GFXfont* DefaultUiFont = FF22;
const GFXfont* LogoUiFont = FF32;
const uint8_t DefaultUiScale = 1;
const uint8_t LogoUiScale = 1;

Resources resources(SD_CS);

class ButtonHandler : public IButtonDelegate, public ITouchButtonDelegate
{
  public:
    void buttonPressed(const byte number) override
    {
        Serial.printf("Button %d pressed\n", number);
        ringManager.selectRing(number);
    }

    void buttonLongPressed(const byte number) override { Serial.printf("Button %d long pressed\n", number); }
};

ButtonHandler buttonHandler;

Button btn1(0, PB12, buttonHandler);
Button btn2(1, PB13, buttonHandler);
Button btn3(2, PB14, buttonHandler);
Button btn4(3, PB15, buttonHandler);
Button btn5(4, PA8, buttonHandler);
Button btn6(5, PA9, buttonHandler);
Button btn7(6, PA10, buttonHandler);
Button btn8(7, PA15, buttonHandler);
Button* buttons[BUTTON_COUNT]{&btn1, &btn2, &btn3, &btn4, &btn5, &btn6, &btn7, &btn8};

const Size boxSize = screenUi.boxSize();
const int32_t boxWidth = screenUi.boxWidth();
const int32_t bottomRowY = screenUi.bottomRowY();

FootSwitchTouchButton touchBtnTop1(4, {0, 0}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnTop2(5, {boxWidth, 0}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnTop3(6, {boxWidth * 2, 0}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnTop4(7, {boxWidth * 3, 0}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnBottom1(0, {0, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnBottom2(1, {boxWidth, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnBottom3(2, {boxWidth * 2, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnBottom4(3, {boxWidth * 3, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton* touchButtons[TOUCH_BUTTON_COUNT]{&touchBtnTop1,    &touchBtnTop2,    &touchBtnTop3,
                                                        &touchBtnTop4,    &touchBtnBottom1, &touchBtnBottom2,
                                                        &touchBtnBottom3, &touchBtnBottom4};

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

    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    screenUi.drawBackgroundAndBorder();

    ringManager.begin();

    screenUi.drawLogo(LogoUiFont, LogoUiScale, "ASH", DefaultUiFont, DefaultUiScale, "guitars");

    const bool resourcesLoaded = initResourcesSD();
    if (resourcesLoaded)
    {
        for (int i = 0; i < TOUCH_BUTTON_COUNT; ++i)
        {
            touchButtons[i]->draw(screenUi);
        }
    }
}

void loop()
{
    HandleEncoder();
    HandleButtons();
    HandleTouch();
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

void HandleButtons()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        buttons[i]->updateState();
    }
}

void HandleTouch()
{
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        for (int i = 0; i < TOUCH_BUTTON_COUNT; ++i)
        {
            if (touchButtons[i]->handleTouch(x, y))
            {
                break;
            }
        }
    }
}
