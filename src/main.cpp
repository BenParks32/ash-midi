#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>
#include "fonts/Oswald-SemiBold30.h"
#include "fonts/Lobster-Regular40.h"

#define LED_PIN PA0
#define LED_COUNT 4

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

TFT_eSPI tft = TFT_eSPI();
uint16_t hue = 0;

void show(const uint8_t* f, const char* label, const int x, const int y) {
    tft.loadFont(f);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.drawString(label, x, y);
}

void setup() {
    strip.begin();
    strip.setBrightness(100);
    strip.show();

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    tft.drawRect(5, 5, 310, 230, tft.color565(67, 70, 77));
    show(Lobster_Regular40, "ASH", 70, 100);
    show(Oswald_SemiBold30, "guitars", 150, 100);
}

void loop() {
    for (int i = 0; i < LED_COUNT; i++)
    {
        uint16_t pixelHue = hue + (i * 65536L / LED_COUNT);
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    hue += 256;        // adjust for speed
    delay(10);         // adjust for smoothness
}
