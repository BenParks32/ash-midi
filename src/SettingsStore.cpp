#include "SettingsStore.h"

#include "RingManager.h"
#include "SmallFileStore.h"

namespace
{
constexpr uint8_t kDefaultMidiChannel = 1;

void copyCalibrationWords(uint16_t (&target)[TouchCalibrationData::WordCount], const uint16_t (&source)[TouchCalibrationData::WordCount])
{
    for (uint8_t i = 0; i < TouchCalibrationData::WordCount; ++i)
    {
        target[i] = source[i];
    }
}

void copyCalibrationData(TouchCalibrationData& target, const TouchCalibrationData& source)
{
    copyCalibrationWords(target.values, source.values);
}
}

SettingsStore::SettingsStore(ISmallFileStore* fileStore) : _fileStore(fileStore) {}

TouchCalibrationData SettingsStore::defaultTouchCalibration()
{
    TouchCalibrationData calibration = {};
    calibration.values[0] = 254;
    calibration.values[1] = 3649;
    calibration.values[2] = 281;
    calibration.values[3] = 3563;
    calibration.values[4] = 7;
    return calibration;
}

AppSettings SettingsStore::defaults()
{
    AppSettings settings = {};
    settings.masterBrightness = RingManager::DefaultBrightness;
    settings.midiChannel = kDefaultMidiChannel;
    settings.touchCalibration = defaultTouchCalibration();
    return settings;
}

bool SettingsStore::load(AppSettings& outSettings)
{
    StoredSettings stored = {};
    if (!readStored(stored) || !isValid(stored))
    {
        outSettings = defaults();
        return false;
    }

    AppSettings loaded = {};
    loaded.masterBrightness = stored.masterBrightness;
    loaded.midiChannel = stored.midiChannel;
    copyCalibrationWords(loaded.touchCalibration.values, stored.touchCalibration);
    outSettings = sanitize(loaded);
    return true;
}

bool SettingsStore::save(const AppSettings& settings)
{
    const AppSettings safeSettings = sanitize(settings);
    const StoredSettings stored = buildStored(safeSettings);
    return writeStored(stored);
}

bool SettingsStore::isValid(const StoredSettings& stored)
{
    if (stored.magic != kMagic)
    {
        return false;
    }

    if (stored.version != kVersion)
    {
        return false;
    }

    if (stored.checksum != computeChecksum(stored))
    {
        return false;
    }

    if (stored.midiChannel < 1 || stored.midiChannel > 16)
    {
        return false;
    }

    if (stored.masterBrightness > RingManager::MaxBrightness)
    {
        return false;
    }

    return true;
}

uint8_t SettingsStore::computeChecksum(const StoredSettings& stored)
{
    uint8_t value = 0;
    value ^= static_cast<uint8_t>(stored.magic & 0xFF);
    value ^= static_cast<uint8_t>((stored.magic >> 8) & 0xFF);
    value ^= stored.version;
    value ^= stored.masterBrightness;
    value ^= stored.midiChannel;
    for (uint8_t i = 0; i < TouchCalibrationData::WordCount; ++i)
    {
        value ^= static_cast<uint8_t>(stored.touchCalibration[i] & 0xFF);
        value ^= static_cast<uint8_t>((stored.touchCalibration[i] >> 8) & 0xFF);
    }
    return value;
}

SettingsStore::StoredSettings SettingsStore::buildStored(const AppSettings& settings)
{
    StoredSettings stored = {};
    stored.magic = kMagic;
    stored.version = kVersion;
    stored.masterBrightness = settings.masterBrightness;
    stored.midiChannel = settings.midiChannel;
    copyCalibrationWords(stored.touchCalibration, settings.touchCalibration.values);
    stored.checksum = computeChecksum(stored);
    return stored;
}

AppSettings SettingsStore::sanitize(const AppSettings& settings)
{
    AppSettings safe = settings;

    if (safe.masterBrightness > RingManager::MaxBrightness)
    {
        safe.masterBrightness = RingManager::MaxBrightness;
    }

    if (safe.midiChannel < 1 || safe.midiChannel > 16)
    {
        safe.midiChannel = kDefaultMidiChannel;
    }

    bool hasAnyTouchCalibrationValue = false;
    for (uint8_t i = 0; i < TouchCalibrationData::WordCount; ++i)
    {
        if (safe.touchCalibration.values[i] != 0)
        {
            hasAnyTouchCalibrationValue = true;
            break;
        }
    }

    if (!hasAnyTouchCalibrationValue)
    {
        copyCalibrationData(safe.touchCalibration, defaultTouchCalibration());
    }

    return safe;
}

bool SettingsStore::readStored(StoredSettings& outStored) const
{
    if (_fileStore == nullptr)
    {
        return false;
    }

    uint8_t* target = reinterpret_cast<uint8_t*>(&outStored);
    return _fileStore->readSmallFile(kSettingsPath, target, sizeof(StoredSettings));
}

bool SettingsStore::writeStored(const StoredSettings& stored) const
{
    if (_fileStore == nullptr)
    {
        return false;
    }

    const uint8_t* source = reinterpret_cast<const uint8_t*>(&stored);
    return _fileStore->writeSmallFile(kSettingsPath, source, sizeof(StoredSettings));
}
