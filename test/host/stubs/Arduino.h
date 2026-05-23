#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>

using byte = std::uint8_t;
using uint = unsigned int;

constexpr int HIGH = 0x1;
constexpr int LOW = 0x0;
constexpr int INPUT_PULLUP = 0x2;
constexpr int FALLING = 0x3;

constexpr byte PA8 = 8;
constexpr byte PA9 = 9;
constexpr byte PA10 = 10;
constexpr byte PA15 = 15;
constexpr byte PB12 = 12;
constexpr byte PB13 = 13;
constexpr byte PB14 = 14;
constexpr byte PB15 = 15;

class HostSerialStub
{
  public:
    void begin(unsigned long) {}

    template <typename... Args> int printf(const char*, Args...)
    {
        return 0;
    }

    template <typename T> void print(const T&) {}

    template <typename T> void println(const T&) {}

    void println() {}

    explicit operator bool() const { return true; }
};

static HostSerialStub Serial;

namespace HostArduino
{
static unsigned long g_millis = 0UL;
static int g_pinValues[256] = {};
static unsigned long g_randomState = 1UL;

static inline void resetTime() { g_millis = 0UL; }

static inline void setMillis(unsigned long value) { g_millis = value; }

static inline void advanceMillis(unsigned long delta) { g_millis += delta; }

static inline void resetPins()
{
    for (std::size_t i = 0; i < (sizeof(g_pinValues) / sizeof(g_pinValues[0])); ++i)
    {
        g_pinValues[i] = HIGH;
    }
}

static inline void setDigitalPinState(byte pin, int value) { g_pinValues[pin] = value; }

static inline void resetRandom() { g_randomState = 1UL; }
} // namespace HostArduino

static inline unsigned long millis() { return HostArduino::g_millis; }

static inline void delay(unsigned long ms) { HostArduino::advanceMillis(ms); }

static inline void pinMode(byte, int) {}

static inline int digitalRead(byte pin) { return HostArduino::g_pinValues[pin]; }

static inline int digitalPinToInterrupt(byte pin) { return static_cast<int>(pin); }

static inline void attachInterrupt(int, void (*)(), int) {}

static inline void randomSeed(unsigned long seed)
{
    HostArduino::g_randomState = (seed == 0UL) ? 1UL : seed;
}

static inline long random(long max)
{
    if (max <= 0)
    {
        return 0;
    }

    HostArduino::g_randomState = (HostArduino::g_randomState * 1103515245UL) + 12345UL;
    return static_cast<long>((HostArduino::g_randomState >> 16) % static_cast<unsigned long>(max));
}

static inline long random(long min, long max)
{
    if (max <= min)
    {
        return min;
    }

    return min + random(max - min);
}
