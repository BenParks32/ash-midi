#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>
#include "fonts/Oswald-SemiBold30.h"
#include "fonts/Lobster-Regular40.h"
#include "Button.h"

#define LED_PIN PA0
#define LED_COUNT 16

#define BUTTON_COUNT 2

const int RingLEDCount = LED_COUNT / BUTTON_COUNT;

uint16_t hue = 0;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();

void clear(const uint8_t* f, const char* label, const int x, const int y) {
    tft.loadFont(f);
    tft.setTextColor(ILI9341_BLACK, ILI9341_BLACK);
    tft.drawString(label, x, y);
}

void show(const uint8_t* f, const char* label, const int x, const int y) {
    tft.loadFont(f);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.drawString(label, x, y);
}

class ButtonHandler : public IButtonDelegate
{
  public:
    void buttonPressed(const byte number) override {

        int startLED = number * RingLEDCount;
        int colour = number == 0 ? strip.Color(255, 0, 0) : strip.Color(0, 255, 0);
        strip.clear();
        for (int st = startLED; st < startLED + RingLEDCount; st++)
        {
            strip.setPixelColor(st, strip.gamma32(colour));
        }
        strip.show();

        if(number == 0)
        {
            clear(Oswald_SemiBold30, "Pressed 2", 15, 150);
            show(Oswald_SemiBold30, "Pressed 1", 15, 150);
        }
        else
        {
            clear(Oswald_SemiBold30, "Pressed 1", 15, 150);
            show(Oswald_SemiBold30, "Pressed 2", 15, 150);
        }
    }

    void buttonLongPressed(const byte number) override {
        Serial.print("Button ");
        Serial.print(number);
        Serial.println(" long pressed");
    }
};

ButtonHandler buttonHandler;

Button btn1(0, PB0, buttonHandler);
Button btn2(1, PB1, buttonHandler);
Button *buttons[BUTTON_COUNT] {&btn1, &btn2};

void setup() {
    strip.begin();
    strip.setBrightness(255);
    strip.show();
    strip.clear();

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    tft.drawRect(5, 5, 310, 230, tft.color565(67, 70, 77));
    show(Lobster_Regular40, "ASH", 70, 100);
    show(Oswald_SemiBold30, "guitars", 150, 100);


    strip.clear();
    strip.setPixelColor(0, 255, 0, 0   );
    strip.show();
}

void loop() {
      for (int i = 0; i < BUTTON_COUNT; ++i)
        {
            buttons[i]->updateState();
        }


    /*for (int i = 0; i < LED_COUNT; i++)
    {
        uint16_t pixelHue = hue + (i * 65536L / LED_COUNT);
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    hue += 256;        // adjust for speed
    delay(10);         // adjust for smoothness*/
}
