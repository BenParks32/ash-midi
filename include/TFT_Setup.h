// TFT_eSPI User Setup for ILI9488 on Black Pill F401CC

#define ILI9488_DRIVER // 16-bit driver (use this for standard ILI9488)

// For 18-bit color, uncomment the next line instead:
// #define ILI9488_18BIT

#define TFT_WIDTH 320
#define TFT_HEIGHT 480

// SPI pins (hardware SPI)
#define TFT_MISO PA6
#define TFT_MOSI PA7
#define TFT_SCLK PA5

// Control pins
#define TFT_CS PA1
#define TFT_DC PA2
#define TFT_RST PA3

// Backlight (optional, set to -1 if not used)
#define TFT_BL -1

// Touch controller pins
#define TOUCH_CS PB10
#define TOUCH_IRQ PB2

// SPI frequency
#define SPI_FREQUENCY 40000000
#define SPI_TOUCH_FREQUENCY 2000000

#define GFXFF 1
#define LOAD_GFXFF
#define SMOOTH_FONT

#define FF22 &FreeSansBold12pt7b
#define FF32 &FreeSansBoldOblique24pt7b
