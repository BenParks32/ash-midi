#include <cstdio>
#include <cstring>
#include <unity.h>

#include "MidiProvider.h"
#include "Modes/SetsMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/SetsMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
constexpr byte TestPlaylistIndex = 2;

class FakeScreenUi : public IScreenUi
{
  public:
    struct DrawTextCall
    {
        char label[96];
        uint16_t textColour;
    };

    void drawBackgroundAndBorder() override {}
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t, uint16_t) override {}
    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*) override {}
    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*, uint16_t, uint16_t,
                  uint16_t, uint16_t) override
    {
    }
    void drawText(const GFXfont*, uint8_t, const char* label, int32_t, int32_t, uint16_t textColour,
                  uint16_t) override
    {
        if (drawTextCallCount < MaxDrawTextCalls)
        {
            std::snprintf(drawTextLog[drawTextCallCount].label, sizeof(drawTextLog[drawTextCallCount].label), "%s",
                          label != nullptr ? label : "");
            drawTextLog[drawTextCallCount].textColour = textColour;
            ++drawTextCallCount;
        }
    }
    void fillRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override {}
    void drawRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override {}
    void drawSmallText(const char*, int32_t, int32_t, uint16_t, uint16_t) override {}
    void drawCenteredText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override {}
    void drawTouchButtonLabelAndPill(const char*, const Point&, const Size&, uint16_t, bool, uint16_t,
                                     uint16_t) override
    {
    }
    void drawTouchButtonPill(const Point&, const Size&, uint16_t, uint16_t) override {}
    void setSdStatusInitializing() override {}
    void setSdStatusFailed() override {}
    void setSdStatusReady() override {}
    void setSdStatusNotMounted() override {}
    void hideSdStatus() override {}
    void redrawSdStatus() override {}

    uint16_t touchButtonPillBorderColour() const override { return TFT_WHITE; }
    int32_t boxWidth() const override { return 120; }
    int32_t boxHeight() const override { return 80; }
    int32_t bottomRowY() const override { return 240; }
    Size boxSize() const override { return {120, 80}; }

    static constexpr int MaxDrawTextCalls = 32;
    DrawTextCall drawTextLog[MaxDrawTextCalls] = {};
    int drawTextCallCount = 0;
};

class MockMidiManager : public IMidiManager
{
  public:
    void setChannel(byte channel) override { currentChannel = channel; }
    byte channel() const override { return currentChannel; }
    void sendProgramChange(byte) override {}
    void sendControlChange(byte, byte) override {}

    byte currentChannel = 1;
};

class MockMidiProvider : public IMidiProvider
{
  public:
    byte maxPresetIndex() const override { return 127; }
    byte defaultPlaylistIndex() const override { return TestPlaylistIndex; }
    void selectPlaylist(byte) override {}
    void recallPreset(byte) override {}
    void selectScene(byte) override {}
    void setTunerEnabled(bool) override {}
    void setGigViewEnabled(bool) override {}
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
    Modes lastMode = Modes::Home;
    ModeTransitionValue lastTransitionValue = ModeTransitionNone;
};

class MockSetListStore : public ISetListStore
{
  public:
    size_t listSetLists(byte, SetListSummary*, size_t) const override { return 0; }
    bool activateSetList(byte, const char*) override { return false; }
    bool clearActiveSetList(byte) override { return true; }
    bool activeSetList(byte, ActiveSetList& setList) const override
    {
        if (!active)
        {
            setList = ActiveSetList{};
            return false;
        }

        setList = activeSet;
        return true;
    }
    bool activeSetSummary(byte, SetListSummary& summary) const override
    {
        if (!active)
        {
            summary = SetListSummary{};
            return false;
        }

        std::snprintf(summary.name, sizeof(summary.name), "%s", activeSet.name);
        std::snprintf(summary.fileName, sizeof(summary.fileName), "%s", "active.jsn");
        summary.partCount = activeSet.partCount;
        summary.songCount = activeSet.songCount;
        return true;
    }
    bool selectSong(byte, size_t) override { return false; }
    bool selectedSong(byte, SetListSongEntry&) const override { return false; }

    bool active = false;
    ActiveSetList activeSet = {};
};

class Fixture
{
  public:
    Fixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), midiProvider(), setListStore(), transitionDelegate(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, setListStore, transitionDelegate)
    {
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockMidiProvider midiProvider;
    MockSetListStore setListStore;
    MockTransitionDelegate transitionDelegate;
    SetsMode mode;
};

int findDrawTextCall(const FakeScreenUi& ui, const char* label, uint16_t textColour)
{
    for (int index = 0; index < ui.drawTextCallCount; ++index)
    {
        if (std::strcmp(ui.drawTextLog[index].label, label) == 0 && ui.drawTextLog[index].textColour == textColour)
        {
            return index;
        }
    }

    return -1;
}

void test_sets_mode_renders_active_set_summary_and_set_button()
{
    Fixture fixture;
    fixture.setListStore.active = true;
    std::snprintf(fixture.setListStore.activeSet.name, sizeof(fixture.setListStore.activeSet.name), "%s", "Friday");

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Back", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("Set", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Loaded: Friday", 0x07FF) >= 0);
}

void test_sets_mode_set_button_opens_set_selection()
{
    Fixture fixture;

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::SetSelection),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(TestPlaylistIndex, 6, false),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_sets_mode_back_returns_to_play()
{
    Fixture fixture;

    fixture.mode.setTransitionValue(makePlayModeSongTransition(TestPlaylistIndex, 3, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeSongTransition(TestPlaylistIndex, 3, false),
                             fixture.transitionDelegate.lastTransitionValue);
}
} // namespace

void setUp() {}
void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_sets_mode_renders_active_set_summary_and_set_button);
    RUN_TEST(test_sets_mode_set_button_opens_set_selection);
    RUN_TEST(test_sets_mode_back_returns_to_play);
    return UNITY_END();
}
