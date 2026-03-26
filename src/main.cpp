#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>

#include "Button.h"
#include "Do.h"
#include "Gfx.h"
#include "Lights.h"
#include "Resources.h"
#include "TouchButton.h"

#define LED_PIN PA0
#define RING_LED_COUNT 8
#define BUTTON_COUNT 8
#define TOUCH_BUTTON_COUNT 2

#define LED_COUNT RING_LED_COUNT* BUTTON_COUNT

#define SD_CS PA4

const Size screenSize = {480, 320};
const byte DimBrightness = 50;
const byte FullBrightness = 255;
uint16_t calData[5] = {254, 3649, 281, 3563, 7};
const int32_t titleCenterX = screenSize.width / 2;
const int32_t titleY = 132;
const int32_t subtitleY = 166;

const int RingLEDCount = LED_COUNT / BUTTON_COUNT;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();

RingLight ring1(strip, 0, RingLEDCount);
RingLight ring2(strip, RingLEDCount, RingLEDCount);
RingLight ring3(strip, RingLEDCount * 2, RingLEDCount);
RingLight ring4(strip, RingLEDCount * 3, RingLEDCount);
RingLight ring5(strip, RingLEDCount * 4, RingLEDCount);
RingLight ring6(strip, RingLEDCount * 5, RingLEDCount);
RingLight ring7(strip, RingLEDCount * 6, RingLEDCount);
RingLight ring8(strip, RingLEDCount * 7, RingLEDCount);
RingLight* rings[BUTTON_COUNT]{&ring1, &ring2, &ring3, &ring4, &ring5, &ring6, &ring7, &ring8};

const GFXfont* DefaultUiFont = FF22;
const GFXfont* LogoUiFont = FF32;
const uint8_t DefaultUiScale = 1;
const uint8_t LogoUiScale = 1;

void clear(const GFXfont* font, const uint8_t scale, const char* label, const int x, const int y)
{
    tft.setFreeFont(font);
    tft.setTextSize(scale);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(label, x, y, GFXFF);
}

void show(const GFXfont* font, const uint8_t scale, const char* label, const int x, const int y)
{
    tft.setFreeFont(font);
    tft.setTextSize(scale);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(label, x, y, GFXFF);
}

void showCentered(const GFXfont* font, const uint8_t scale, const char* label, const int centerX, const int y)
{
    tft.setFreeFont(font);
    tft.setTextSize(scale);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    const int x = centerX - (tft.textWidth(label, GFXFF) / 2);
    tft.drawString(label, x, y, GFXFF);
}

Resources resources(SD_CS);

RingLight* selectedRing = &ring1;
char pressed[10];
char longPressed[15];
class ButtonHandler : public IButtonDelegate, public ITouchButtonDelegate
{
  public:
    void buttonPressed(const byte number) override
    {
        Serial.printf("Button %d pressed\n", number);
        selectedRing->setBrightness(DimBrightness);
        selectedRing = rings[number];
        selectedRing->setBrightness(FullBrightness);
        strip.show();

        Serial.printf("Drawing text for button %d\n", number);
        clear(DefaultUiFont, DefaultUiScale, pressed, 15, 200);
        snprintf(pressed, sizeof(pressed), "Pressed %d", (int)number + 1);
        show(DefaultUiFont, DefaultUiScale, pressed, 15, 200);
    }

    void buttonLongPressed(const byte number) override
    {
        Serial.printf("Button %d long pressed\n", number);
        clear(DefaultUiFont, DefaultUiScale, longPressed, 240, 200);
        snprintf(longPressed, sizeof(longPressed), "Long Pressed %d", (int)number + 1);
        show(DefaultUiFont, DefaultUiScale, longPressed, 240, 200);
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

const uint16_t borderColour = tft.color565(175, 179, 186);

const int32_t boxHeight = screenSize.height / 4;
const int32_t boxWidth = screenSize.width / 4;
const int32_t lineWidth = 2;
const Size boxSize = {boxWidth, boxHeight};
const int32_t bottomRowY = boxHeight * 3;

FootSwitchTouchButton touchBtnBottom1(0, {0, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton touchBtnBottom2(1, {boxWidth, bottomRowY}, boxSize, resources, buttonHandler);
FootSwitchTouchButton* touchButtons[TOUCH_BUTTON_COUNT]{&touchBtnBottom1, &touchBtnBottom2};

void drawBorder()
{
    // Fast border pass: axis-aligned filled rectangles are much faster than drawWideLine.
    tft.fillRect(0, 0, screenSize.width, lineWidth, borderColour);
    tft.fillRect(0, boxHeight, screenSize.width, lineWidth, borderColour);
    tft.fillRect(0, boxHeight * 3, screenSize.width, lineWidth, borderColour);
    tft.fillRect(0, screenSize.height - lineWidth, screenSize.width, lineWidth, borderColour);

    tft.fillRect(0, 0, lineWidth, screenSize.height, borderColour);
    tft.fillRect(screenSize.width - lineWidth, 0, lineWidth, screenSize.height, borderColour);

    for (int i = 1; i < 4; ++i)
    {
        const int left = boxWidth * i;
        tft.fillRect(left, 0, lineWidth, boxHeight, borderColour);
    }

    for (int i = 1; i < 4; ++i)
    {
        const int left = boxWidth * i;
        const int top = boxHeight * 3;
        tft.fillRect(left, top, lineWidth, screenSize.height - top, borderColour);
    }
}

void fillScreenFast(const uint16_t color)
{
    // Direct full-window block fill is faster than generic fillRect clipping path.
    tft.setAddrWindow(0, 0, screenSize.width, screenSize.height);
    tft.pushBlock(color, (uint32_t)screenSize.width * (uint32_t)screenSize.height);
}

void drawBackgroundAndBorder()
{
    // At high SPI rates this is often bandwidth-limited; hide the sweep by disabling panel output while drawing.
    tft.writecommand(TFT_DISPOFF);

    // Keep one SPI transaction open to reduce command overhead during startup draw.
    tft.startWrite();
    fillScreenFast(TFT_BLACK);
    drawBorder();
    tft.endWrite();

    tft.writecommand(TFT_DISPON);
}

bool initResourcesSD()
{
    Serial.println("Initializing SD card...");
    tft.fillCircle(20, boxHeight + 20, 10, TFT_YELLOW);
    if (!resources.init())
    {
        Serial.println("SD card initialization failed!");
        tft.fillCircle(20, boxHeight + 20, 10, TFT_RED);
        return false;
    }

    if (!resources.loadAll())
    {
        Serial.println("Failed to load resources!");
        tft.fillCircle(20, boxHeight + 20, 10, TFT_RED);
        return false;
    }

    Serial.println("SD card initialized and resources loaded successfully.");
    tft.fillCircle(20, boxHeight + 20, 10, TFT_GREEN);
    return true;
}

void setup()
{
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    Serial.begin(115200);

    strip.begin();
    strip.setBrightness(255);

    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    drawBackgroundAndBorder();

    strip.clear();

    ring1.setBrightness(FullBrightness);
    ring1.setColour(strip.Color(0, 255, 0));

    ring2.setBrightness(DimBrightness);
    ring2.setColour(strip.Color(0, 0, 255));

    ring3.setBrightness(DimBrightness);
    ring3.setColour(strip.Color(255, 0, 0));

    ring4.setBrightness(DimBrightness);
    ring4.setColour(strip.Color(255, 0, 0));

    ring5.setBrightness(DimBrightness);
    ring5.setColour(strip.Color(220, 165, 0));

    ring6.setBrightness(DimBrightness);
    ring6.setColour(strip.Color(0, 128, 128));

    ring7.setBrightness(DimBrightness);
    ring7.setColour(strip.Color(255, 128, 255));

    ring8.setBrightness(DimBrightness);
    ring8.setColour(strip.Color(255, 255, 255));

    strip.show();

    showCentered(LogoUiFont, LogoUiScale, "ASH", titleCenterX, titleY);
    showCentered(DefaultUiFont, DefaultUiScale, "guitars", titleCenterX, subtitleY);

    const bool resourcesLoaded = initResourcesSD();
    if (resourcesLoaded)
    {
        touchBtnBottom1.draw(tft);
        touchBtnBottom2.draw(tft);
    }
}

void loop()
{
    HandleButtons();
    HandleTouch();
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
