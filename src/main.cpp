#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Button.h"
#include "Do.h"
#include "Gfx.h"
#include "Lights.h"
#include "TouchButton.h"

#include "fonts/Lobster-Regular40.h"
#include "fonts/Oswald-SemiBold30.h"

#include "icons/footswitch.h"

#define LED_PIN PA0
#define RING_LED_COUNT 8
#define BUTTON_COUNT 8

#define LED_COUNT RING_LED_COUNT* BUTTON_COUNT
const Size screenSize = {480, 320};
const byte DimBrightness = 50;
const byte FullBrightness = 255;
uint16_t calData[5] = {254, 3649, 281, 3563, 7};

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

void clear(const uint8_t* f, const char* label, const int x, const int y)
{
    tft.loadFont(f);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(label, x, y);
}

void show(const uint8_t* f, const char* label, const int x, const int y)
{
    tft.loadFont(f);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(label, x, y);
}

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

        clear(Oswald_SemiBold30, pressed, 15, 200);
        snprintf(pressed, sizeof(pressed), "Pressed %d", (int)number + 1);
        show(Oswald_SemiBold30, pressed, 15, 200);
    }

    void buttonLongPressed(const byte number) override
    {
        Serial.printf("Button %d long pressed\n", number);
        clear(Oswald_SemiBold30, longPressed, 240, 200);
        snprintf(longPressed, sizeof(longPressed), "Long Pressed %d", (int)number + 1);
        show(Oswald_SemiBold30, longPressed, 240, 200);
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

const int32_t rightBorderX = screenSize.width - 2;
const int32_t bottomBorderY = screenSize.height - 2;
const int32_t boxHeight = screenSize.height / 4;
const int32_t boxWidth = screenSize.width / 4;
const int32_t lineWidth = 2;
const Size boxSize = {boxWidth, boxHeight};
const int32_t bottomRowY = boxHeight * 3;

FootSwitchTouchButton touchBtnBottom1(0, {0, bottomRowY}, boxSize, icon_footswitch, buttonHandler);
FootSwitchTouchButton touchBtnBottom2(1, {boxWidth, bottomRowY}, boxSize, icon_footswitch, buttonHandler);
FootSwitchTouchButton* touchButtons[BUTTON_COUNT]{&touchBtnBottom1, &touchBtnBottom2};

void drawBoxLine(const int x1, const int y1, const int x2, const int y2)
{
    const int lineWidth = 2;
    tft.drawWideLine(x1, y1, x2, y2, lineWidth, borderColour);
}

void drawBorder()
{
    drawBoxLine(0, 0, screenSize.width, 0);
    drawBoxLine(0, boxHeight, screenSize.width, boxHeight);
    drawBoxLine(0, boxHeight * 3, screenSize.width, boxHeight * 3);
    drawBoxLine(0, screenSize.height - 2, screenSize.width, screenSize.height - 2);
    drawBoxLine(0, 0, 0, screenSize.height);
    drawBoxLine(screenSize.width - 2, 0, screenSize.width - 2, screenSize.height);

    for (int i = 1; i < 4; ++i)
    {
        const int left = boxWidth * i;
        drawBoxLine(left, 0, left, boxHeight);
    }

    for (int i = 1; i < 4; ++i)
    {
        const int left = boxWidth * i;
        const int top = boxHeight * 3;
        drawBoxLine(left, top, left, screenSize.height);
    }
}

void setup()
{
    pinMode(PB2, INPUT);
    Serial.begin(115200);

    strip.begin();
    strip.setBrightness(255);

    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    drawBorder();

    show(Lobster_Regular40, "ASH", 70, 140);
    show(Oswald_SemiBold30, "guitars", 150, 140);
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

    touchBtnBottom1.draw(tft);
    touchBtnBottom2.draw(tft);
}

/*
int currentRing = 0;
void testRing()
{
    rings[currentRing]->setBrightness(DimBrightness);
    currentRing = (currentRing + 1) % BUTTON_COUNT;
    rings[currentRing]->setBrightness(FullBrightness);
    strip.show();
}
Every every500ms(500, testRing);
*/

void loop()
{
    HandleButtons();
    HandleTouch();
    // every500ms.tick();
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
        for (int i = 0; i < BUTTON_COUNT; ++i)
        {
            if (touchButtons[i]->handleTouch(x, y))
            {
                break;
            }
        }
    }
}
