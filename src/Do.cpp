#include "Do.h"

Every::Every(const unsigned long interval, std::function<void()> func) : interval(interval), func(func), lastTime(0) {}

void Every::tick()
{
    const unsigned long currentTime = millis();
    if (currentTime - lastTime >= interval)
    {
        func();
        lastTime = currentTime;
    }
}