#pragma once

#include <Arduino.h>

class Resources;

struct AppSettings
{
    uint8_t masterBrightness;
    uint8_t midiChannel;
};

class ISettingsStore
{
  public:
    virtual ~ISettingsStore() = default;

    virtual bool load(AppSettings& outSettings) = 0;
    virtual bool save(const AppSettings& settings) = 0;
};

class SettingsStore : public ISettingsStore
{
  public:
    explicit SettingsStore(Resources* resources = nullptr);

    bool load(AppSettings& outSettings) override;
    bool save(const AppSettings& settings) override;

    static AppSettings defaults();

  private:
    static constexpr uint16_t kMagic = 0xA541;
    static constexpr uint8_t kVersion = 1;
    static constexpr const char* kSettingsPath = "/settings.bin";

  private:
    struct StoredSettings
    {
        uint16_t magic;
        uint8_t version;
        uint8_t masterBrightness;
        uint8_t midiChannel;
        uint8_t checksum;
    };

  private:
    static bool isValid(const StoredSettings& stored);
    static uint8_t computeChecksum(const StoredSettings& stored);
    static StoredSettings buildStored(const AppSettings& settings);
    static AppSettings sanitize(const AppSettings& settings);

    bool readStored(StoredSettings& outStored) const;
    bool writeStored(const StoredSettings& stored) const;

  private:
    Resources* _resources;
};
