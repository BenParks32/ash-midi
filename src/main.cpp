#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>
#include "fonts/Oswald-SemiBold30.h"
#include "fonts/Lobster-Regular40.h"
#include "Button.h"
#include "Lights.h"

#define LED_PIN PA0
#define LED_COUNT 16
#define BUTTON_COUNT 2

const struct ScreenSize { int32_t width; int32_t height; } screenSize = { 480, 320 };
const byte DimBrightness = 50;
const byte FullBrightness = 200;
uint16_t calData[5] = { 254, 3649, 281, 3563, 7 };

const int RingLEDCount = LED_COUNT / BUTTON_COUNT;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();

RingLight ring1(strip, 0, RingLEDCount);
RingLight ring2(strip, RingLEDCount, RingLEDCount);

void clear(const uint8_t* f, const char* label, const int x, const int y) {
    tft.loadFont(f);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawString(label, x, y);
}

void show(const uint8_t* f, const char* label, const int x, const int y) {
    tft.loadFont(f);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(label, x, y);
}

class ButtonHandler : public IButtonDelegate
{
  public:
    void buttonPressed(const byte number) override
    {
        if(number == 0)
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

    void buttonLongPressed(const byte number) override {
        if(number == 0)
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
Button *buttons[BUTTON_COUNT] {&btn1, &btn2};

void setup() {
    pinMode(PB2, INPUT);
    Serial.begin(115200);

    strip.begin();
    strip.setBrightness(255);
    tft.setTouch(calData);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, screenSize.width, screenSize.height, tft.color565(175, 179, 186));
    show(Lobster_Regular40, "ASH", 70, 100);
    show(Oswald_SemiBold30, "guitars", 150, 100);

    ring1.setBrightness(FullBrightness);
    ring1.setColour(strip.Color(0, 255, 0));

    ring2.setBrightness(DimBrightness);
    ring2.setColour(strip.Color(220, 0, 255));
    strip.show();
}

void loop() {
        for (int i = 0; i < BUTTON_COUNT; ++i)
        {
            buttons[i]->updateState();
        }

        uint16_t x, y;

        if (tft.getTouch(&x, &y)) {
            Serial.printf("Touch: %d, %d\n", x, y);
            tft.fillCircle(x, y, 4, TFT_RED);
            delay(50);
        }
}

