#include "Resources.h"

#include <SPI.h>
#include <cstdio>
#include <cstdlib>

#include "TFT_Setup.h"

namespace
{
constexpr uint32_t SdInitClockHz = 100000;
constexpr uint8_t SdInitRetryCount = 3;
constexpr uint8_t SdIdleClockBytes = 20;
constexpr uint32_t SdRetryDelayMs = 50;
constexpr uint32_t SdBusSettleDelayMs = 10;
constexpr uint32_t SdPowerSettleDelayMs = 250;

const char* alternateSdPath(const char* path)
{
    if (path == nullptr || path[0] != '/' || path[1] == '\0')
    {
        return nullptr;
    }

    return path + 1;
}

bool openExistingSdPath(const char* requestedPath, File& openedFile, const char*& resolvedPath)
{
    if (requestedPath == nullptr || requestedPath[0] == '\0')
    {
        return false;
    }

    resolvedPath = requestedPath;
    openedFile = SD.open(requestedPath, FILE_READ);
    if (openedFile)
    {
        return true;
    }

    const char* alternatePath = alternateSdPath(requestedPath);
    if (alternatePath == nullptr)
    {
        return false;
    }

    openedFile = SD.open(alternatePath, FILE_READ);
    if (openedFile)
    {
        resolvedPath = alternatePath;
        return true;
    }

    return false;
}

const char* preferredWritableSdPath(const char* path)
{
    const char* alternatePath = alternateSdPath(path);
    return (alternatePath != nullptr) ? alternatePath : path;
}
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

        Serial.printf("SD mount attempt %u/%u failed.\n", static_cast<unsigned int>(attempt + 1U),
                      static_cast<unsigned int>(SdInitRetryCount));
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

bool Resources::readSmallFile(const char* path, uint8_t* buffer, size_t expectedSize) const
{
    if (path == nullptr || *path == '\0' || buffer == nullptr || expectedSize == 0)
    {
        return false;
    }

    if (!_isMounted)
    {
        Serial.println("SD small-file read: card not mounted, attempting remount.");
        if (!const_cast<Resources*>(this)->mount())
        {
            Serial.println("SD small-file read: remount failed.");
            return false;
        }
    }

    deselectSharedSpiDevices();

    const char* resolvedPath = nullptr;
    File file;
    if (!openExistingSdPath(path, file, resolvedPath))
    {
        Serial.printf("SD small-file read: '%s' not found (alternate '%s').\n", path,
                      alternateSdPath(path) != nullptr ? alternateSdPath(path) : "<none>");
        return false;
    }

    if (static_cast<size_t>(file.size()) != expectedSize)
    {
        Serial.printf("SD small-file read: size mismatch for '%s' (expected %u, actual %u).\n", resolvedPath,
                      static_cast<unsigned int>(expectedSize), static_cast<unsigned int>(file.size()));
        file.close();
        return false;
    }

    size_t bytesRead = 0;
    while (bytesRead < expectedSize)
    {
        const size_t remaining = expectedSize - bytesRead;
        const size_t chunkSize = remaining > 64 ? 64 : remaining;
        const int chunkRead = file.read(buffer + bytesRead, chunkSize);
        if (chunkRead <= 0)
        {
            Serial.printf("SD small-file read: failed while reading '%s' at offset %u.\n", resolvedPath,
                          static_cast<unsigned int>(bytesRead));
            file.close();
            return false;
        }
        bytesRead += static_cast<size_t>(chunkRead);
    }

    file.close();
    return true;
}

bool Resources::readBinaryFile(const char* path, uint8_t*& buffer, size_t& size) const
{
    buffer = nullptr;
    size = 0U;

    if (path == nullptr || *path == '\0')
    {
        return false;
    }

    if (!_isMounted)
    {
        Serial.println("SD binary read: card not mounted, attempting remount.");
        if (!const_cast<Resources*>(this)->mount())
        {
            Serial.println("SD binary read: remount failed.");
            return false;
        }
    }

    deselectSharedSpiDevices();

    const char* resolvedPath = nullptr;
    File file;
    if (!openExistingSdPath(path, file, resolvedPath))
    {
        Serial.printf("SD binary read: '%s' not found (alternate '%s').\n", path,
                      alternateSdPath(path) != nullptr ? alternateSdPath(path) : "<none>");
        return false;
    }

    const size_t fileSize = static_cast<size_t>(file.size());
    if (fileSize == 0U)
    {
        file.close();
        return false;
    }

    uint8_t* data = static_cast<uint8_t*>(malloc(fileSize));
    if (data == nullptr)
    {
        Serial.printf("SD binary read: failed to allocate %u bytes for '%s'.\n", static_cast<unsigned int>(fileSize),
                      resolvedPath);
        file.close();
        return false;
    }

    size_t bytesRead = 0U;
    while (bytesRead < fileSize)
    {
        const size_t remaining = fileSize - bytesRead;
        const size_t chunkSize = remaining > 64 ? 64 : remaining;
        const int chunkRead = file.read(data + bytesRead, chunkSize);
        if (chunkRead <= 0)
        {
            Serial.printf("SD binary read: failed while reading '%s' at offset %u.\n", resolvedPath,
                          static_cast<unsigned int>(bytesRead));
            free(data);
            file.close();
            return false;
        }
        bytesRead += static_cast<size_t>(chunkRead);
    }

    file.close();
    buffer = data;
    size = fileSize;
    return true;
}

bool Resources::readTextFile(const char* path, char* buffer, size_t bufferSize) const
{
    if (path == nullptr || *path == '\0' || buffer == nullptr || bufferSize < 2)
    {
        Serial.println("SD text read: invalid parameters.");
        Serial.printf("  path: '%s'\n", path != nullptr ? path : "<null>");
        Serial.printf("  buffer: %s\n", buffer != nullptr ? "valid" : "<null>");
        Serial.printf("  bufferSize: %u\n", static_cast<unsigned int>(bufferSize));
        return false;
    }

    if (!_isMounted)
    {
        Serial.println("SD text read: card not mounted, attempting remount.");
        if (!const_cast<Resources*>(this)->mount())
        {
            Serial.println("SD text read: remount failed.");
            return false;
        }
    }

    deselectSharedSpiDevices();

    const char* resolvedPath = nullptr;
    File file;
    if (!openExistingSdPath(path, file, resolvedPath))
    {
        Serial.printf("SD text read: '%s' not found (alternate '%s').\n", path,
                      alternateSdPath(path) != nullptr ? alternateSdPath(path) : "<none>");
        return false;
    }

    const size_t fileSize = static_cast<size_t>(file.size());
    if (fileSize >= bufferSize)
    {
        Serial.printf("SD text read: '%s' too large for buffer (size %u, capacity %u).\n", resolvedPath,
                      static_cast<unsigned int>(fileSize), static_cast<unsigned int>(bufferSize));
        file.close();
        return false;
    }

    size_t bytesRead = 0;
    while (bytesRead < fileSize)
    {
        const size_t remaining = fileSize - bytesRead;
        const size_t chunkSize = remaining > 64 ? 64 : remaining;
        const int chunkRead = file.read(reinterpret_cast<uint8_t*>(buffer) + bytesRead, chunkSize);
        if (chunkRead <= 0)
        {
            Serial.printf("SD text read: failed while reading '%s' at offset %u.\n", resolvedPath,
                          static_cast<unsigned int>(bytesRead));
            file.close();
            return false;
        }
        bytesRead += static_cast<size_t>(chunkRead);
    }

    buffer[bytesRead] = '\0';
    file.close();
    return true;
}

size_t Resources::listTextFiles(const char* directoryPath, TextFilePathEntry* entries, size_t maxEntries) const
{
    if (directoryPath == nullptr || directoryPath[0] == '\0' || entries == nullptr || maxEntries == 0)
    {
        return 0;
    }

    if (!_isMounted)
    {
        Serial.println("SD directory list: card not mounted, attempting remount.");
        if (!const_cast<Resources*>(this)->mount())
        {
            Serial.println("SD directory list: remount failed.");
            return 0;
        }
    }

    deselectSharedSpiDevices();

    const char* resolvedPath = nullptr;
    File directory;
    if (!openExistingSdPath(directoryPath, directory, resolvedPath))
    {
        Serial.printf("SD directory list: '%s' not found (alternate '%s').\n", directoryPath,
                      alternateSdPath(directoryPath) != nullptr ? alternateSdPath(directoryPath) : "<none>");
        return 0;
    }
    if (!directory.isDirectory())
    {
        Serial.printf("SD directory list: failed to open directory '%s' (resolved from '%s').\n", resolvedPath,
                      directoryPath);
        if (directory)
        {
            directory.close();
        }
        return 0;
    }

    size_t count = 0;
    while (count < maxEntries)
    {
        File entry = directory.openNextFile();
        if (!entry)
        {
            break;
        }

        if (!entry.isDirectory())
        {
            const char* entryName = entry.name();
            if (entryName != nullptr && entryName[0] != '\0')
            {
                std::snprintf(entries[count].path, sizeof(entries[count].path), "%s/%s", directoryPath, entryName);
                ++count;
            }
        }

        entry.close();
    }

    directory.close();
    return count;
}

bool Resources::writeSmallFile(const char* path, const uint8_t* data, size_t size)
{
    if (!_isMounted || data == nullptr || size == 0)
    {
        return false;
    }

    if (path == nullptr || *path == '\0')
    {
        return false;
    }

    const char* resolvedPath = preferredWritableSdPath(path);
    deselectSharedSpiDevices();

    if (SD.exists(resolvedPath) && !SD.remove(resolvedPath))
    {
        Serial.printf("SD small-file write: failed to remove '%s'.\n", resolvedPath);
        return false;
    }

    File file = SD.open(resolvedPath, FILE_WRITE);
    if (!file)
    {
        Serial.printf("SD small-file write: failed to open '%s' for writing.\n", resolvedPath);
        return false;
    }

    size_t bytesWritten = 0;
    while (bytesWritten < size)
    {
        const size_t remaining = size - bytesWritten;
        const size_t chunkSize = remaining > 64 ? 64 : remaining;
        const int chunkWritten = file.write(data + bytesWritten, chunkSize);
        if (chunkWritten <= 0)
        {
            Serial.printf("SD small-file write: failed while writing '%s' at offset %u.\n", resolvedPath,
                          static_cast<unsigned int>(bytesWritten));
            file.close();
            return false;
        }
        bytesWritten += static_cast<size_t>(chunkWritten);
    }

    file.flush();
    file.close();
    return true;
}

const uint16_t* Resources::readFile(const char* path, const size_t expectedSizeBytes) const
{
    Serial.printf("Loading resource from path: %s\n", path);

    if (path == nullptr || *path == '\0')
    {
        Serial.println("Failed to load resource: path was empty");
        return nullptr;
    }

    deselectSharedSpiDevices();

    const char* resolvedPath = nullptr;
    File file;
    if (!openExistingSdPath(path, file, resolvedPath))
    {
        Serial.printf("File not found: %s (alternate '%s')\n", path,
                      alternateSdPath(path) != nullptr ? alternateSdPath(path) : "<none>");
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
