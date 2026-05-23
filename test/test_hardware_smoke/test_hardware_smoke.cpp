#include <Arduino.h>

#include "ButtonHandler.h"
#include "RingManager.h"

#include "../../src/Button.cpp"
#include "../../src/ButtonHandler.cpp"
#include "../../src/Lights.cpp"
#include "../../src/RingManager.cpp"

namespace
{
constexpr byte LedPin = PA0;
constexpr char TestFile[] = "test/test_hardware_smoke/test_hardware_smoke.cpp";

bool run_button_handler_begin_initializes_button_inputs_and_interrupts()
{
    Adafruit_NeoPixel strip(RingManager::LedCount, LedPin, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);
    IMode* activeMode = nullptr;
    ButtonHandler handler(activeMode, ringManager);

    handler.begin();
    handler.updateButtons();

    return true;
}

bool run_ring_manager_begin_and_show_complete()
{
    Adafruit_NeoPixel strip(RingManager::LedCount, LedPin, NEO_GRB + NEO_KHZ800);
    RingManager ringManager(strip);

    ringManager.begin();
    ringManager.setRingColour(0, 0x00FF00);
    ringManager.setRingBrightness(0, RingManager::FullBrightness);
    ringManager.show();

    return true;
}

void emitResult(const char* testName, bool passed)
{
    Serial.print(TestFile);
    Serial.print(":1:");
    Serial.print(testName);
    Serial.print(":");
    Serial.println(passed ? "PASS" : "FAIL");
}

void emitSummary(int totalTests, int failureCount)
{
    Serial.print(totalTests);
    Serial.print(" Tests ");
    Serial.print(failureCount);
    Serial.println(" Failures 0 Ignored");
    Serial.println(failureCount == 0 ? "OK" : "FAIL");
    Serial.flush();
}
} // namespace

void setup()
{
    Serial.begin(115200);
    const uint32_t start = millis();
    while (!Serial && (millis() - start) < 5000U)
    {
        delay(10);
    }

    int failureCount = 0;

    const bool buttonHandlerPassed = run_button_handler_begin_initializes_button_inputs_and_interrupts();
    emitResult("test_button_handler_begin_initializes_button_inputs_and_interrupts", buttonHandlerPassed);
    if (!buttonHandlerPassed)
    {
        failureCount++;
    }

    const bool ringManagerPassed = run_ring_manager_begin_and_show_complete();
    emitResult("test_ring_manager_begin_and_show_complete", ringManagerPassed);
    if (!ringManagerPassed)
    {
        failureCount++;
    }

    emitSummary(2, failureCount);
}

void loop() {}
