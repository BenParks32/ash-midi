#include "Resources.h"

#include <SPI.h>

#include "TFT_Setup.h"

namespace
{
constexpr uint32_t SdInitClockHz = 100000;
constexpr uint8_t SdInitRetryCount = 3;
constexpr uint8_t SdIdleClockBytes = 20;
constexpr uint32_t SdRetryDelayMs = 50;
constexpr uint32_t SdBusSettleDelayMs = 10;
constexpr uint32_t SdPowerSettleDelayMs = 250;
} // namespace

Resources::Resources(const byte sdPin) : _sdPin(sdPin), FootSwitchIcon(nullptr), _isMounted(false) {}

bool Resources::init() { return mount(); }

void Resources::deselectSharedSpiDevices() const
{
    pinMode(_sdPin, OUTPUT);
    digitalWrite(_sdPin, HIGH);

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
}

void Resources::sendSdIdleClocks() const
{
    SPI.begin();
    SPI.beginTransaction(SPISettings(SdInitClockHz, MSBFIRST, SPI_MODE0));
    deselectSharedSpiDevices();

    for (uint8_t index = 0; index < SdIdleClockBytes; ++index)
    {
        SPI.transfer(0xFF);
    }

    SPI.endTransaction();
}

bool Resources::beginSdWithRetries()
{
    for (uint8_t attempt = 0; attempt < SdInitRetryCount; ++attempt)
    {
        deselectSharedSpiDevices();
        delay(SdBusSettleDelayMs);
        sendSdIdleClocks();
        deselectSharedSpiDevices();

        if (SD.begin(SdInitClockHz, _sdPin))
        {
            return true;
        }

        SD.end();
        delay(SdRetryDelayMs);
    }

    return false;
}

bool Resources::mount()
{
    if (_isMounted)
    {
        return true;
    }

    deselectSharedSpiDevices();
    SD.end();
    delay(SdPowerSettleDelayMs);

    _isMounted = beginSdWithRetries();
    if (_isMounted)
    {
        deselectSharedSpiDevices();
    }
    else
    {
        Serial.println("SD card mount failed.");
    }

    return _isMounted;
}

const uint16_t* Resources::readFile(const char* path, const size_t expectedSizeBytes) const
{
    Serial.printf("Loading resource from path: %s\n", path);

    if (!SD.exists(path))
    {
        Serial.printf("File not found: %s\n", path);
        return nullptr;
    }

    File file = SD.open(path, FILE_READ);
    if (!file)
    {
        Serial.printf("Failed to open file: %s\n", path);
        return nullptr;
    }

    if (file.size() < expectedSizeBytes)
    {
        Serial.printf("File too small: expected %u, actual %u\n", (unsigned int)expectedSizeBytes,
                      (unsigned int)file.size());
        file.close();
        return nullptr;
    }

    const size_t wordCount = (expectedSizeBytes + sizeof(uint16_t) - 1) / sizeof(uint16_t);
    uint16_t* data = new uint16_t[wordCount];
    if (data == nullptr)
    {
        Serial.println("Failed to allocate memory for resource");
        file.close();
        return nullptr;
    }

    size_t bytesRead = 0;
    uint8_t* rawData = (uint8_t*)data;
    while (bytesRead < expectedSizeBytes)
    {
        const size_t remaining = expectedSizeBytes - bytesRead;
        const size_t chunkSize = remaining > 512 ? 512 : remaining;
        const int chunkRead = file.read(rawData + bytesRead, chunkSize);
        if (chunkRead <= 0)
        {
            break;
        }
        bytesRead += (size_t)chunkRead;
    }

    if (bytesRead != expectedSizeBytes)
    {
        Serial.printf("Failed to read complete data: expected %u, got %u\n", (unsigned int)expectedSizeBytes,
                      (unsigned int)bytesRead);
        delete[] data;
        file.close();
        return nullptr;
    }

    Serial.printf("Successfully read %u bytes for resource\n", (unsigned int)bytesRead);
    file.close();
    return data;
}

const Icon* const Resources::loadIcon(const char* path, const Size& size) const
{
    const size_t expectedSizeBytes = size.width * size.height * sizeof(uint16_t);
    const uint16_t* data = readFile(path, expectedSizeBytes);
    if (data == nullptr)
    {
        Serial.printf("Failed to load icon from path: %s\n", path);
        return nullptr;
    }

    return new Icon(size, data);
}

bool Resources::loadAll()
{
    if (!_isMounted)
    {
        Serial.println("Cannot load resources: SD card is not mounted");
        return false;
    }

    Serial.println("Loading all resources...");
    const Icon* nextFootSwitchIcon = loadIcon("/switch.raw", {SZ_ICON_FOOTSWITCH, SZ_ICON_FOOTSWITCH});
    if (nextFootSwitchIcon == nullptr)
    {
        Serial.println("Failed to load foot switch icon");
        return false;
    }

    delete FootSwitchIcon;
    FootSwitchIcon = nextFootSwitchIcon;

    return true;
}

bool Resources::unmount()
{
    if (!_isMounted)
    {
        return true;
    }

    SD.end();
    deselectSharedSpiDevices();
    delay(SdBusSettleDelayMs);
    _isMounted = false;
    Serial.println("SD card unmounted.");
    return true;
}

Resources::~Resources() { delete FootSwitchIcon; }