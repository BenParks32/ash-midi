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
    void drawBackgroundAndBorder() override {}
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t, uint16_t) override {}
    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*) override {}
    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*, uint16_t, uint16_t,
                  uint16_t, uint16_t) override
    {
    }
    void drawText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override {}
    void fillRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override {}
    void drawRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override {}
    void drawSmallText(const char*, int32_t, int32_t, uint16_t, uint16_t) override {}
    void drawCenteredText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override {}
    void drawTouchButtonLabelAndPill(const char*, const Point&, const Size&, uint16_t, bool, uint16_t, uint16_t) override {}
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
            return false;
        }

        setList = activeSet;
        return true;
    }
    bool activeSetSummary(byte, SetListSummary&) const override { return false; }
    bool activeSetPosition(byte, size_t& songCount, size_t& selectedSongIndex) const override
    {
        if (!active)
        {
            return false;
        }

        songCount = activeSet.songCount;
        selectedSongIndex = activeSet.selectedSongIndex;
        return true;
    }
    bool selectSong(byte, size_t setSongIndex) override
    {
        ++selectSongCalls;
        lastSetSongIndex = setSongIndex;
        if (active && setSongIndex < activeSet.songCount)
        {
            activeSet.selectedSongIndex = setSongIndex;
        }
        return true;
    }
    bool selectedSong(byte, SetListSongEntry& song) const override
    {
        if (!active || activeSet.songCount == 0 || activeSet.selectedSongIndex >= activeSet.songCount)
        {
            return false;
        }

        song = activeSet.songs[activeSet.selectedSongIndex];
        return true;
    }

    bool active = false;
    mutable ActiveSetList activeSet = {};
    int selectSongCalls = 0;
    size_t lastSetSongIndex = 0;
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

void seedSong(SetListSongEntry& song, uint16_t number, uint16_t part, byte patch, const char* name)
{
    song.number = number;
    song.part = part;
    song.patch = patch;
    song.available = true;
    std::snprintf(song.name, sizeof(song.name), "%s", name);
}

void seedActiveSet(Fixture& fixture)
{
    fixture.setListStore.active = true;
    std::snprintf(fixture.setListStore.activeSet.name, sizeof(fixture.setListStore.activeSet.name), "%s", "Friday");
    fixture.setListStore.activeSet.songCount = 3;
    fixture.setListStore.activeSet.partCount = 2;
    fixture.setListStore.activeSet.selectedSongIndex = 0;
    seedSong(fixture.setListStore.activeSet.songs[0], 1, 1, 10, "Song A");
    seedSong(fixture.setListStore.activeSet.songs[1], 2, 1, 11, "Song B");
    seedSong(fixture.setListStore.activeSet.songs[2], 3, 2, 12, "Song C");
}

void test_sets_mode_renders_manage_set_button_map()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Next", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Select", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("Play", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Prev", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("Load", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Exit", fixture.touchButtonManager.getButton(7)->label());
}

void test_sets_mode_select_picks_song_and_returns_to_play_set()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeSetSongTransition(TestPlaylistIndex, 10, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(0);
    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(1, fixture.setListStore.selectSongCalls);
    TEST_ASSERT_EQUAL_UINT(1, fixture.setListStore.lastSetSongIndex);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeSetSongTransition(TestPlaylistIndex, 11, false),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_sets_mode_load_opens_set_selection()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeSetSongTransition(TestPlaylistIndex, 10, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::SetSelection),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
}

void test_sets_mode_exit_returns_to_normal_play_transition()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeSetSongTransition(TestPlaylistIndex, 10, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(TestPlaylistIndex, 10, false),
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
    RUN_TEST(test_sets_mode_renders_manage_set_button_map);
    RUN_TEST(test_sets_mode_select_picks_song_and_returns_to_play_set);
    RUN_TEST(test_sets_mode_load_opens_set_selection);
    RUN_TEST(test_sets_mode_exit_returns_to_normal_play_transition);
    return UNITY_END();
}
