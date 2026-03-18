#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Button.h"
#include "Gfx.h"
#include "Lights.h"
#include "TouchButton.h"

#include "fonts/Lobster-Regular40.h"
#include "fonts/Oswald-SemiBold30.h"

#include "icons/footswitch.h"

#define LED_PIN PA0
#define LED_COUNT 16
#define BUTTON_COUNT 2

const Size screenSize = {480, 320};
const byte DimBrightness = 50;
const byte FullBrightness = 200;
uint16_t calData[5] = {254, 3649, 281, 3563, 7};

const int RingLEDCount = LED_COUNT / BUTTON_COUNT;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();

RingLight ring1(strip, 0, RingLEDCount);
RingLight ring2(strip, RingLEDCount, RingLEDCount);

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

class ButtonHandler : public IButtonDelegate, public ITouchButtonDelegate
{
  public:
    void buttonPressed(const byte number) override
    {
        if (number == 0)
        {
            ring1.setBrightness(FullBrightness);
            ring2.setBrightness(DimBrightness);
            clear(Oswald_SemiBold30, "Pressed 2", 15, 150);
            show(Oswald_SemiBold30, "Pressed 1", 15, 150);
        }
        else
        {
            ring2.setBrightness(FullBrightness);
            ring1.setBrightness(DimBrightness);
            clear(Oswald_SemiBold30, "Pressed 1", 15, 150);
            show(Oswald_SemiBold30, "Pressed 2", 15, 150);
        }
        strip.show();
    }

    void buttonLongPressed(const byte number) override
    {
        if (number == 0)
        {
            clear(Oswald_SemiBold30, "Long Pressed 2", 15, 180);
            show(Oswald_SemiBold30, "Long Pressed 1", 15, 180);
        }
        else
        {
            clear(Oswald_SemiBold30, "Long Pressed 1", 15, 180);
            show(Oswald_SemiBold30, "Long Pressed 2", 15, 180);
        }
    }
};

ButtonHandler buttonHandler;

Button btn1(0, PB0, buttonHandler);
Button btn2(1, PB1, buttonHandler);
Button* buttons[BUTTON_COUNT]{&btn1, &btn2};

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

    ring1.setBrightness(FullBrightness);
    ring1.setColour(strip.Color(0, 255, 0));

    ring2.setBrightness(DimBrightness);
    ring2.setColour(strip.Color(220, 0, 255));
    strip.show();

    touchBtnBottom1.draw(tft);
    touchBtnBottom2.draw(tft);
}

void loop()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        buttons[i]->updateState();
    }

    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        Serial.printf("Touch: %d, %d\n", x, y);
        for (int i = 0; i < BUTTON_COUNT; ++i)
        {
            if (touchButtons[i]->handleTouch(x, y))
            {
                break;
            }
        }
    }
}
