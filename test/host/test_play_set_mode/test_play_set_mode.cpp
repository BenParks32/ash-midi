#include <cstdio>
#include <cstring>
#include <unity.h>

#include "MidiProvider.h"
#include "Modes/PlaySetMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/PlaySetMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/TapTempoEngine.cpp"
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

    static constexpr int MaxDrawTextCalls = 64;
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

class MockButtonOverrideStore : public IButtonOverrideStore
{
  public:
    bool refresh() override { return true; }
    void applyOverrides(byte, byte, Function*, size_t, PatchDisplayConfig*) const override {}

    bool songForIndex(byte, byte songIndex, SongConfig* outSong) const override
    {
        if (outSong == nullptr)
        {
            return false;
        }

        *outSong = SongConfig{};
        switch (songIndex)
        {
        case 1:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s",
                          "This is a very long song title that should truncate");
            break;
        case 2:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s", "Bridge");
            break;
        case 3:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s", "Solo");
            break;
        case 4:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s", "Breakdown");
            break;
        case 5:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s", "Outro");
            break;
        default:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s", "Intro");
            break;
        }

        std::snprintf(outSong->displayName, sizeof(outSong->displayName), "%s", outSong->name);
        return true;
    }

    bool songForId(byte, const char*, byte&, SongConfig&) const override { return false; }
};

class MockSetListStore : public ISetListStore
{
  public:
    size_t listSetLists(byte, SetListSummary*, size_t) const override { return 0; }
    bool activateSetList(byte, const char*) override { return false; }
    bool clearActiveSetList(byte) override { return false; }
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
    bool activeSetSongAt(byte, size_t setSongIndex, SetListSongEntry& song) const override
    {
        if (!active || setSongIndex >= activeSet.songCount)
        {
            return false;
        }

        song = activeSet.songs[setSongIndex];
        return true;
    }
    bool activeSetPartName(byte, uint16_t partNumber, char* name, size_t nameSize) const override
    {
        if (name == nullptr || nameSize == 0)
        {
            return false;
        }

        name[0] = '\0';
        for (size_t index = 0; index < activeSet.partCount; ++index)
        {
            if (activeSet.parts[index].part != partNumber)
            {
                continue;
            }

            std::snprintf(name, nameSize, "%s", activeSet.parts[index].name);
            return true;
        }

        return false;
    }
    bool selectSong(byte, size_t) override { return false; }
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
    ActiveSetList activeSet = {};
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

class Fixture
{
  public:
    Fixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), midiProvider(), buttonOverrideStore(), setListStore(), transitionDelegate(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, buttonOverrideStore, setListStore,
               transitionDelegate)
    {
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockMidiProvider midiProvider;
    MockButtonOverrideStore buttonOverrideStore;
    MockSetListStore setListStore;
    MockTransitionDelegate transitionDelegate;
    PlaySetMode mode;
};

void seedActiveSet(Fixture& fixture)
{
    fixture.setListStore.active = true;
    fixture.setListStore.activeSet.songCount = 6;
    fixture.setListStore.activeSet.partCount = 1;
    fixture.setListStore.activeSet.selectedSongIndex = 0;
    fixture.setListStore.activeSet.parts[0].part = 1;
    std::snprintf(fixture.setListStore.activeSet.parts[0].name,
                  sizeof(fixture.setListStore.activeSet.parts[0].name), "%s", "Part 1");

    fixture.setListStore.activeSet.songs[0].number = 1;
    fixture.setListStore.activeSet.songs[0].part = 1;
    fixture.setListStore.activeSet.songs[0].available = true;
    fixture.setListStore.activeSet.songs[0].songIndex = 0;
    std::snprintf(fixture.setListStore.activeSet.songs[0].name,
                  sizeof(fixture.setListStore.activeSet.songs[0].name), "%s", "Intro");

    fixture.setListStore.activeSet.songs[1].number = 2;
    fixture.setListStore.activeSet.songs[1].part = 1;
    fixture.setListStore.activeSet.songs[1].available = true;
    fixture.setListStore.activeSet.songs[1].songIndex = 1;
    std::snprintf(fixture.setListStore.activeSet.songs[1].name,
                  sizeof(fixture.setListStore.activeSet.songs[1].name), "%s", "This is a very long song title that should truncate");

    fixture.setListStore.activeSet.songs[2].number = 3;
    fixture.setListStore.activeSet.songs[2].part = 1;
    fixture.setListStore.activeSet.songs[2].available = true;
    fixture.setListStore.activeSet.songs[2].songIndex = 2;
    std::snprintf(fixture.setListStore.activeSet.songs[2].name,
                  sizeof(fixture.setListStore.activeSet.songs[2].name), "%s", "Bridge");

    fixture.setListStore.activeSet.songs[3].number = 4;
    fixture.setListStore.activeSet.songs[3].part = 1;
    fixture.setListStore.activeSet.songs[3].available = true;
    fixture.setListStore.activeSet.songs[3].songIndex = 3;
    std::snprintf(fixture.setListStore.activeSet.songs[3].name,
                  sizeof(fixture.setListStore.activeSet.songs[3].name), "%s", "Solo");

    fixture.setListStore.activeSet.songs[4].number = 5;
    fixture.setListStore.activeSet.songs[4].part = 1;
    fixture.setListStore.activeSet.songs[4].available = true;
    fixture.setListStore.activeSet.songs[4].songIndex = 4;
    std::snprintf(fixture.setListStore.activeSet.songs[4].name,
                  sizeof(fixture.setListStore.activeSet.songs[4].name), "%s", "Breakdown");

    fixture.setListStore.activeSet.songs[5].number = 6;
    fixture.setListStore.activeSet.songs[5].part = 1;
    fixture.setListStore.activeSet.songs[5].available = true;
    fixture.setListStore.activeSet.songs[5].songIndex = 5;
    std::snprintf(fixture.setListStore.activeSet.songs[5].name,
                  sizeof(fixture.setListStore.activeSet.songs[5].name), "%s", "Outro");
}

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

int findDrawTextCallContaining(const FakeScreenUi& ui, const char* partA, const char* partB, uint16_t textColour)
{
    for (int index = 0; index < ui.drawTextCallCount; ++index)
    {
        if (ui.drawTextLog[index].textColour != textColour)
        {
            continue;
        }

        if (std::strstr(ui.drawTextLog[index].label, partA) != nullptr &&
            std::strstr(ui.drawTextLog[index].label, partB) != nullptr)
        {
            return index;
        }
    }

    return -1;
}

void test_play_set_mode_truncates_long_upcoming_song_names()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_TRUE(findDrawTextCallContaining(fixture.ui, "2 ", "...", TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "2 This is a very long song title that should truncate", TFT_WHITE) < 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "6 Outro", TFT_WHITE) >= 0);
}
} // namespace

void setUp() {}
void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_play_set_mode_truncates_long_upcoming_song_names);
    return UNITY_END();
}
