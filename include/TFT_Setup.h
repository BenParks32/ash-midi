#pragma once

#define ILI9341_DRIVER

// SPI pins (hardware SPI)
#define TFT_MOSI PA7
#define TFT_MISO PA6
#define TFT_SCLK PA5

// Control pins
#define TFT_DC   PA2
#define TFT_RST  PA3
#define TFT_CS   -1    // No CS pin on your module

// Backlight (optional)
#define TFT_BL   -1    // Or assign a PWM pin if you want dimming

// SPI speed
#define SPI_FREQUENCY  40000000

#define LOAD_GFXFF
#define SMOOTH_FONT
