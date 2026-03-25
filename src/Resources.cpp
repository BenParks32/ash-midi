#include "Resources.h"

Resources::Resources(const byte sdPin) : _sdPin(sdPin), FootSwitchIcon(nullptr) {}

bool Resources::init() { return SD.begin(_sdPin); }

const uint16_t* Resources::readFile(const char* path, const uint16_t expectedSize) const
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

    uint16_t* data = new uint16_t[expectedSize];
    size_t bytesRead = file.read((uint8_t*)data, expectedSize);
    if (bytesRead != expectedSize)
    {
        Serial.println("Failed to read complete data");
        delete[] data;
        file.close();
        return nullptr;
    }

    Serial.printf("Successfully read %d bytes for resource\n", bytesRead);
    return data;
}

const Icon* const Resources::loadIcon(const char* path, const Size& size) const
{
    const uint16_t expectedSize = size.width * size.height * sizeof(uint16_t);
    const uint16_t* data = readFile(path, expectedSize);
    if (data == nullptr)
    {
        Serial.printf("Failed to load icon from path: %s\n", path);
        return nullptr;
    }

    return new Icon(size, data);
}

bool Resources::loadAll()
{
    Serial.println("Loading all resources...");
    FootSwitchIcon = loadIcon("/switch.raw", {SZ_ICON_FOOTSWITCH, SZ_ICON_FOOTSWITCH});
    if (FootSwitchIcon == nullptr)
    {
        Serial.println("Failed to load foot switch icon");
        return false;
    }

    return true;
}

Resources::~Resources() { delete FootSwitchIcon; }