#include "SettingsStore.h"

#include "Resources.h"
#include "RingManager.h"

namespace
{
constexpr uint8_t kDefaultMidiChannel = 1;
}

SettingsStore::SettingsStore(Resources* resources) : _resources(resources) {}

AppSettings SettingsStore::defaults()
{
    AppSettings settings = {};
    settings.masterBrightness = RingManager::DefaultBrightness;
    settings.midiChannel = kDefaultMidiChannel;
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

    const AppSettings loaded = {stored.masterBrightness, stored.midiChannel};
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
    return value;
}

SettingsStore::StoredSettings SettingsStore::buildStored(const AppSettings& settings)
{
    StoredSettings stored = {};
    stored.magic = kMagic;
    stored.version = kVersion;
    stored.masterBrightness = settings.masterBrightness;
    stored.midiChannel = settings.midiChannel;
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

    return safe;
}

bool SettingsStore::readStored(StoredSettings& outStored) const
{
    if (_resources == nullptr)
    {
        return false;
    }

    uint8_t* target = reinterpret_cast<uint8_t*>(&outStored);
    return _resources->readSmallFile(kSettingsPath, target, sizeof(StoredSettings));
}

bool SettingsStore::writeStored(const StoredSettings& stored) const
{
    if (_resources == nullptr)
    {
        return false;
    }

    const uint8_t* source = reinterpret_cast<const uint8_t*>(&stored);
    return _resources->writeSmallFile(kSettingsPath, source, sizeof(StoredSettings));
}
