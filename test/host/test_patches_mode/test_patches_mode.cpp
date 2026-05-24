#include <cstring>
#include <unity.h>

#include "ButtonOverrideStore.h"
#include "MidiProvider.h"
#include "Modes/PatchesMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/PatchesMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
class FakeScreenUi : public IScreenUi
{
  public:
    struct DrawCenteredTextCall
    {
        char label[64];
        uint16_t textColour;
    };

    void drawBackgroundAndBorder() override { ++drawBackgroundAndBorderCalls; }
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t) override { ++drawCenteredFrameCalls; }
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawCenteredFrameCalls;
    }
    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*) override { ++drawLogoCalls; }
    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*, uint16_t, uint16_t,
                  uint16_t, uint16_t) override
    {
        ++drawLogoWithColoursCalls;
    }
    void drawText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawTextCalls;
    }
    void fillRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override { ++fillRectCalls; }
    void drawRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override { ++drawRectCalls; }
    void drawSmallText(const char*, int32_t, int32_t, uint16_t, uint16_t) override { ++drawSmallTextCalls; }
    void drawCenteredText(const GFXfont*, uint8_t, const char* label, int32_t, int32_t, uint16_t textColour,
                          uint16_t) override
    {
        ++drawCenteredTextCalls;
        if (drawCenteredTextCallCount < MaxDrawCenteredTextCalls)
        {
            DrawCenteredTextCall& call = drawCenteredTextLog[drawCenteredTextCallCount++];
            std::snprintf(call.label, sizeof(call.label), "%s", label != nullptr ? label : "");
            call.textColour = textColour;
        }
    }
    void drawTouchButtonLabelAndPill(const char*, const Point&, const Size&, uint16_t, bool, uint16_t,
                                     uint16_t) override
    {
        ++drawTouchButtonLabelAndPillCalls;
    }
    void drawTouchButtonPill(const Point&, const Size&, uint16_t, uint16_t) override { ++drawTouchButtonPillCalls; }
    void setSdStatusInitializing() override { ++setSdStatusInitializingCalls; }
    void setSdStatusFailed() override { ++setSdStatusFailedCalls; }
    void setSdStatusReady() override { ++setSdStatusReadyCalls; }
    void setSdStatusNotMounted() override { ++setSdStatusNotMountedCalls; }
    void hideSdStatus() override { ++hideSdStatusCalls; }
    void redrawSdStatus() override { ++redrawSdStatusCalls; }

    uint16_t touchButtonPillBorderColour() const override { return TFT_WHITE; }
    int32_t boxWidth() const override { return 120; }
    int32_t boxHeight() const override { return 80; }
    int32_t bottomRowY() const override { return 240; }
    Size boxSize() const override { return {120, 80}; }

    int drawBackgroundAndBorderCalls = 0;
    static constexpr int MaxDrawCenteredTextCalls = 16;
    DrawCenteredTextCall drawCenteredTextLog[MaxDrawCenteredTextCalls] = {};
    int drawCenteredTextCallCount = 0;
    int drawCenteredFrameCalls = 0;
    int drawLogoCalls = 0;
    int drawLogoWithColoursCalls = 0;
    int drawTextCalls = 0;
    int fillRectCalls = 0;
    int drawRectCalls = 0;
    int drawSmallTextCalls = 0;
    int drawCenteredTextCalls = 0;
    int drawTouchButtonLabelAndPillCalls = 0;
    int drawTouchButtonPillCalls = 0;
    int setSdStatusInitializingCalls = 0;
    int setSdStatusFailedCalls = 0;
    int setSdStatusReadyCalls = 0;
    int setSdStatusNotMountedCalls = 0;
    int hideSdStatusCalls = 0;
    int redrawSdStatusCalls = 0;
};

class MockMidiManager : public IMidiManager
{
  public:
    void setChannel(byte channel) override { currentChannel = channel; }
    byte channel() const override { return currentChannel; }
    void sendProgramChange(byte programChangeValue) override
    {
        ++programChangeCalls;
        lastProgramChangeValue = programChangeValue;
    }
    void sendControlChange(byte controlChangeNumber, byte controlChangeValue) override
    {
        ++controlChangeCalls;
        lastControlChangeNumber = controlChangeNumber;
        lastControlChangeValue = controlChangeValue;
    }

    int programChangeCalls = 0;
    int controlChangeCalls = 0;
    byte lastProgramChangeValue = 0;
    byte lastControlChangeNumber = 0;
    byte lastControlChangeValue = 0;
    byte currentChannel = 1;
};

class MockTransitionDelegate : public IModeTransistionDelegate
{
  public:
    void enterMode(Modes mode, ModeTransitionValue transitionValue) override
    {
        ++calls;
        lastMode = mode;
        lastTransitionValue = transitionValue;
    }

    int calls = 0;
    Modes lastMode = Modes::Patches;
    ModeTransitionValue lastTransitionValue = ModeTransitionNone;
};

class MockMidiProvider : public IMidiProvider
{
  public:
    byte maxPresetIndex() const override { return 127; }
    byte defaultPlaylistIndex() const override { return 1; }
    void selectPlaylist(byte playlistIndex) override { lastPlaylistIndex = playlistIndex; }
    void recallPreset(byte presetIndex) override { lastRecallPreset = presetIndex; }
    void selectScene(byte sceneIndex) override { lastSceneIndex = sceneIndex; }
    void setTunerEnabled(bool enabled) override { lastTunerEnabled = enabled; }
    void setGigViewEnabled(bool enabled) override { lastGigViewEnabled = enabled; }

    byte lastPlaylistIndex = 1;
    byte lastRecallPreset = 0;
    byte lastSceneIndex = 0;
    bool lastTunerEnabled = false;
    bool lastGigViewEnabled = false;
};

class MockButtonOverrideStore : public IButtonOverrideStore
{
  public:
    bool refresh() override { return true; }

    void applyOverrides(byte, byte, Function*, size_t, PatchDisplayConfig*) const override {}

    size_t listPatches(byte playlistIndex, PatchListEntry* entries, size_t capacity) const override
    {
        lastPlaylistIndex = playlistIndex;
        const size_t copyCount = (entryCount < capacity) ? entryCount : capacity;
        for (size_t i = 0; i < copyCount; ++i)
        {
            entries[i] = storedEntries[i];
        }
        return copyCount;
    }

    mutable byte lastPlaylistIndex = 0;
    PatchListEntry storedEntries[8] = {};
    size_t entryCount = 0;
};

class PatchesModeFixture
{
  public:
    PatchesModeFixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), midiProvider(), transitionDelegate(), overrideStore(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, overrideStore, transitionDelegate)
    {
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockMidiProvider midiProvider;
    MockTransitionDelegate transitionDelegate;
    MockButtonOverrideStore overrideStore;
    PatchesMode mode;
};

void test_activate_sets_main_and_populates_remaining_patch_buttons()
{
    PatchesModeFixture fixture;
    fixture.overrideStore.entryCount = 7;
    fixture.overrideStore.storedEntries[0].patchNumber = 0;
    std::strcpy(fixture.overrideStore.storedEntries[0].name, "Main from JSON");
    fixture.overrideStore.storedEntries[1].patchNumber = 1;
    std::strcpy(fixture.overrideStore.storedEntries[1].name, "Verse");
    fixture.overrideStore.storedEntries[2].patchNumber = 3;
    std::strcpy(fixture.overrideStore.storedEntries[2].name, "Crunch");
    fixture.overrideStore.storedEntries[3].patchNumber = 7;
    fixture.overrideStore.storedEntries[4].patchNumber = 8;
    std::strcpy(fixture.overrideStore.storedEntries[4].name, "Bridge");
    fixture.overrideStore.storedEntries[5].patchNumber = 9;
    std::strcpy(fixture.overrideStore.storedEntries[5].name, "Solo");
    fixture.overrideStore.storedEntries[6].patchNumber = 10;
    std::strcpy(fixture.overrideStore.storedEntries[6].name, "Outro");

    fixture.mode.setTransitionValue(makePlayModeTransition(4, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_UINT8(4, fixture.overrideStore.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_STRING("Main", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Verse", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("Crunch", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING("07", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Back", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, fixture.touchButtonManager.getButton(4)->pillColour());
    TEST_ASSERT_EQUAL_STRING("Bridge", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING("Solo", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Outro", fixture.touchButtonManager.getButton(7)->label());
    TEST_ASSERT_EQUAL_INT(2, fixture.ui.drawCenteredTextCallCount);
    TEST_ASSERT_EQUAL_STRING("Code Red", fixture.ui.drawCenteredTextLog[0].label);
    TEST_ASSERT_EQUAL_STRING("patches", fixture.ui.drawCenteredTextLog[1].label);
}

void test_button_press_enters_play_on_selected_patch()
{
    PatchesModeFixture fixture;
    fixture.overrideStore.entryCount = 2;
    fixture.overrideStore.storedEntries[0].patchNumber = 0;
    fixture.overrideStore.storedEntries[1].patchNumber = 5;
    std::strcpy(fixture.overrideStore.storedEntries[1].name, "Solo");

    fixture.mode.setTransitionValue(makePlayModeTransition(3, 2, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(3, 5, true), fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_press_returns_to_play_without_recall()
{
    PatchesModeFixture fixture;

    fixture.mode.setTransitionValue(makePlayModeTransition(4, 9, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(4, 9, false), fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_long_press_is_noop()
{
    PatchesModeFixture fixture;

    fixture.mode.setTransitionValue(makePlayModeTransition(4, 9, false));
    fixture.mode.activate();
    fixture.mode.buttonLongPressed(4);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_deactivate_clears_center_title()
{
    PatchesModeFixture fixture;

    fixture.mode.setTransitionValue(makePlayModeTransition(4, 9, false));
    fixture.mode.activate();
    fixture.mode.deactivate();

    TEST_ASSERT_EQUAL_STRING("Code Red", fixture.ui.drawCenteredTextLog[2].label);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawCenteredTextLog[2].textColour);
    TEST_ASSERT_EQUAL_STRING("patches", fixture.ui.drawCenteredTextLog[3].label);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawCenteredTextLog[3].textColour);
}
} // namespace

void setUp()
{
    HostArduino::resetTime();
    HostArduino::resetPins();
}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_activate_sets_main_and_populates_remaining_patch_buttons);
    RUN_TEST(test_button_press_enters_play_on_selected_patch);
    RUN_TEST(test_button_five_press_returns_to_play_without_recall);
    RUN_TEST(test_button_five_long_press_is_noop);
    RUN_TEST(test_deactivate_clears_center_title);
    return UNITY_END();
}
