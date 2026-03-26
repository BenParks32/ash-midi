#include "Resources.h"

Resources::Resources(const byte sdPin) : _sdPin(sdPin), FootSwitchIcon(nullptr), DefaultFont(nullptr), LogoFont(nullptr)
{
}

bool Resources::init() { return SD.begin(_sdPin); }

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

const Font* const Resources::loadFont(const char* path, const size_t size) const
{
    const size_t expectedSizeBytes = size * sizeof(uint16_t);
    const uint16_t* data = readFile(path, expectedSizeBytes);
    if (data == nullptr)
    {
        Serial.printf("Failed to load font from path: %s\n", path);
        return nullptr;
    }
    return new Font(size, data);
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

    // Smooth fonts are intentionally not loaded on this target to save flash and RAM.
    DefaultFont = nullptr;
    LogoFont = nullptr;

    return true;
}

Resources::~Resources()
{
    delete FootSwitchIcon;
    delete DefaultFont;
    delete LogoFont;
}