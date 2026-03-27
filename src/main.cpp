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
#define TOUCH_BUTTON_COUNT 2

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

char pressed[10];
char longPressed[15];
class ButtonHandler : public IButtonDelegate, public ITouchButtonDelegate
{
  public:
    void buttonPressed(const byte number) override
    {
        Serial.printf("Button %d pressed\n", number);
        ringManager.selectRing(number);
        strip.show();

        Serial.printf("Drawing text for button %d\n", number);
        char nextPressed[sizeof(pressed)];
        snprintf(nextPressed, sizeof(nextPressed), "Pressed %d", (int)number + 1);
        screenUi.updateText(DefaultUiFont, DefaultUiScale, pressed, nextPressed, 15, 200);
        snprintf(pressed, sizeof(pressed), "%s", nextPressed);
    }

    void buttonLongPressed(const byte number) override
    {
        Serial.printf("Button %d long pressed\n", number);
        char nextLongPressed[sizeof(longPressed)];
        snprintf(nextLongPressed, sizeof(nextLongPressed), "Long Pressed %d", (int)number + 1);
        screenUi.updateText(DefaultUiFont, DefaultUiScale, longPressed, nextLongPressed, 240, 200);
        snprintf(longPressed, sizeof(longPressed), "%s", nextLongPressed);
    }
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
const int32_t bottomRowY = screenUi.bottomRowY();

FootSwitchTouchButton touchBtnBottom1(0, {0, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnBottom2(1, {screenUi.boxWidth(), bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton* touchButtons[TOUCH_BUTTON_COUNT]{&touchBtnBottom1, &touchBtnBottom2};

bool initResourcesSD()
{
    Serial.println("Initializing SD card...");
    tft.fillCircle(20, screenUi.boxHeight() + 20, 10, TFT_YELLOW);
    if (!resources.init())
    {
        Serial.println("SD card initialization failed!");
        tft.fillCircle(20, screenUi.boxHeight() + 20, 10, TFT_RED);
        return false;
    }

    if (!resources.loadAll())
    {
        Serial.println("Failed to load resources!");
        tft.fillCircle(20, screenUi.boxHeight() + 20, 10, TFT_RED);
        return false;
    }

    Serial.println("SD card initialized and resources loaded successfully.");
    tft.fillCircle(20, screenUi.boxHeight() + 20, 10, TFT_GREEN);
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

    strip.begin();
    strip.setBrightness(255);

    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    screenUi.drawBackgroundAndBorder();

    strip.clear();

    ringManager.setRingColour(0, strip.Color(0, 255, 0));
    ringManager.setRingColour(1, strip.Color(0, 0, 255));
    ringManager.setRingColour(2, strip.Color(255, 0, 0));
    ringManager.setRingColour(3, strip.Color(255, 0, 0));
    ringManager.setRingColour(4, strip.Color(220, 165, 0));
    ringManager.setRingColour(5, strip.Color(0, 128, 128));
    ringManager.setRingColour(6, strip.Color(255, 128, 255));
    ringManager.setRingColour(7, strip.Color(255, 255, 255));

    strip.show();

    screenUi.drawLogo(LogoUiFont, LogoUiScale, "ASH", DefaultUiFont, DefaultUiScale, "guitars");

    const bool resourcesLoaded = initResourcesSD();
    if (resourcesLoaded)
    {
        touchBtnBottom1.draw(screenUi);
        touchBtnBottom2.draw(screenUi);
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

    strip.show();
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
