#include <unity.h>

#include "RingManager.h"
#define private public
#include "SettingsStore.h"
#undef private
#include "SmallFileStore.h"

#include "../../../src/SettingsStore.cpp"

namespace
{
class MockSmallFileStore : public ISmallFileStore
{
  public:
    bool readSmallFile(const char* path, uint8_t* buffer, size_t expectedSize) const override
    {
        ++readCalls;
        lastReadPath = path;
        if (!hasStoredData || expectedSize != storedSize)
        {
            return false;
        }

        for (size_t i = 0; i < expectedSize; ++i)
        {
            buffer[i] = storedData[i];
        }

        return true;
    }

    bool writeSmallFile(const char* path, const uint8_t* data, size_t size) override
    {
        ++writeCalls;
        lastWritePath = path;
        storedSize = size;
        if (storedSize > sizeof(storedData))
        {
            return false;
        }

        for (size_t i = 0; i < size; ++i)
        {
            storedData[i] = data[i];
        }

        hasStoredData = true;
        return true;
    }

    mutable int readCalls = 0;
    int writeCalls = 0;
    mutable const char* lastReadPath = nullptr;
    const char* lastWritePath = nullptr;
    bool hasStoredData = false;
    size_t storedSize = 0;
    uint8_t storedData[64] = {0};
};

void test_defaults_include_touch_calibration()
{
    const AppSettings defaults = SettingsStore::defaults();

    TEST_ASSERT_EQUAL_UINT8(RingManager::DefaultBrightness, defaults.masterBrightness);
    TEST_ASSERT_EQUAL_UINT8(1, defaults.midiChannel);
    TEST_ASSERT_EQUAL_UINT16(254, defaults.touchCalibration.values[0]);
    TEST_ASSERT_EQUAL_UINT16(3649, defaults.touchCalibration.values[1]);
    TEST_ASSERT_EQUAL_UINT16(281, defaults.touchCalibration.values[2]);
    TEST_ASSERT_EQUAL_UINT16(3563, defaults.touchCalibration.values[3]);
    TEST_ASSERT_EQUAL_UINT16(7, defaults.touchCalibration.values[4]);
}

void test_save_and_load_round_trip_touch_calibration()
{
    MockSmallFileStore fileStore;
    SettingsStore settingsStore(&fileStore);

    AppSettings saved = SettingsStore::defaults();
    saved.masterBrightness = 99;
    saved.midiChannel = 11;
    saved.touchCalibration.values[0] = 111;
    saved.touchCalibration.values[1] = 2222;
    saved.touchCalibration.values[2] = 333;
    saved.touchCalibration.values[3] = 4444;
    saved.touchCalibration.values[4] = 5;

    TEST_ASSERT_TRUE(settingsStore.save(saved));

    AppSettings loaded = {};
    TEST_ASSERT_TRUE(settingsStore.load(loaded));

    TEST_ASSERT_EQUAL_UINT8(99, loaded.masterBrightness);
    TEST_ASSERT_EQUAL_UINT8(11, loaded.midiChannel);
    TEST_ASSERT_EQUAL_UINT16(111, loaded.touchCalibration.values[0]);
    TEST_ASSERT_EQUAL_UINT16(2222, loaded.touchCalibration.values[1]);
    TEST_ASSERT_EQUAL_UINT16(333, loaded.touchCalibration.values[2]);
    TEST_ASSERT_EQUAL_UINT16(4444, loaded.touchCalibration.values[3]);
    TEST_ASSERT_EQUAL_UINT16(5, loaded.touchCalibration.values[4]);
}

void test_load_invalid_version_falls_back_to_defaults()
{
    MockSmallFileStore fileStore;
    SettingsStore settingsStore(&fileStore);

    SettingsStore::StoredSettings stored = {};
    stored.magic = SettingsStore::kMagic;
    stored.version = 1;
    stored.masterBrightness = 42;
    stored.midiChannel = 6;
    stored.touchCalibration[0] = 900;
    stored.touchCalibration[1] = 901;
    stored.touchCalibration[2] = 902;
    stored.touchCalibration[3] = 903;
    stored.touchCalibration[4] = 4;
    stored.checksum = SettingsStore::computeChecksum(stored);

    fileStore.hasStoredData = true;
    fileStore.storedSize = sizeof(stored);
    const uint8_t* storedBytes = reinterpret_cast<const uint8_t*>(&stored);
    for (size_t i = 0; i < sizeof(stored); ++i)
    {
        fileStore.storedData[i] = storedBytes[i];
    }

    AppSettings loaded = {};
    TEST_ASSERT_FALSE(settingsStore.load(loaded));

    const AppSettings defaults = SettingsStore::defaults();
    TEST_ASSERT_EQUAL_UINT8(defaults.masterBrightness, loaded.masterBrightness);
    TEST_ASSERT_EQUAL_UINT8(defaults.midiChannel, loaded.midiChannel);
    TEST_ASSERT_EQUAL_UINT16(defaults.touchCalibration.values[0], loaded.touchCalibration.values[0]);
    TEST_ASSERT_EQUAL_UINT16(defaults.touchCalibration.values[4], loaded.touchCalibration.values[4]);
}
} // namespace

void setUp() {}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_defaults_include_touch_calibration);
    RUN_TEST(test_save_and_load_round_trip_touch_calibration);
    RUN_TEST(test_load_invalid_version_falls_back_to_defaults);
    return UNITY_END();
}
