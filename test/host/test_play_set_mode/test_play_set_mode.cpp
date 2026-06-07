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
        const GFXfont* font;
        int32_t x;
        int32_t y;
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
    void drawText(const GFXfont* font, uint8_t, const char* label, int32_t x, int32_t y, uint16_t textColour,
                  uint16_t) override
    {
        if (drawTextCallCount < MaxDrawTextCalls)
        {
            std::snprintf(drawTextLog[drawTextCallCount].label, sizeof(drawTextLog[drawTextCallCount].label), "%s",
                          label != nullptr ? label : "");
            drawTextLog[drawTextCallCount].font = font;
            drawTextLog[drawTextCallCount].x = x;
            drawTextLog[drawTextCallCount].y = y;
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
    void applyOverrides(byte, byte patchNumber, Function* functions, size_t functionCount,
                        PatchDisplayConfig*) const override
    {
        ++applyOverridesCount;
        lastAppliedPatch = patchNumber;

        switch (patchNumber)
        {
        case 3:
            setButtonLabel(functions, functionCount, 0, "Everlong");
            setButtonLabel(functions, functionCount, 1, "Opener");
            break;
        case 0:
            setButtonLabel(functions, functionCount, 0, "Killing");
            setButtonLabel(functions, functionCount, 1, "Malice");
            setButtonLabel(functions, functionCount, 3, "Extra");
            break;
        default:
            break;
        }
    }

    bool songForIndex(byte, byte songIndex, SongConfig* outSong) const override
    {
        if (outSong == nullptr)
        {
            return false;
        }

        *outSong = SongConfig{};
        switch (songIndex)
        {
        case 0:
            std::snprintf(outSong->name, sizeof(outSong->name), "%s", "Opener");
            std::snprintf(outSong->displayName, sizeof(outSong->displayName), "%s",
                          "This is a very long opening song title that should truncate");
            break;
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

    bool songNotesForIndex(byte, byte songIndex, SongNotes* outNotes) const override
    {
        if (outNotes == nullptr)
        {
            return false;
        }

        *outNotes = SongNotes{};
        switch (songIndex)
        {
        case 0:
            std::snprintf(outNotes->lines[0], sizeof(outNotes->lines[0]), "%s",
                          "This note line is intentionally long and should truncate");
            std::snprintf(outNotes->lines[1], sizeof(outNotes->lines[1]), "%s", "Drop D");
            outNotes->lineCount = 2;
            return true;
        default:
            return true;
        }
    }

    bool songForId(byte, const char*, byte&, SongConfig&) const override { return false; }

    mutable int applyOverridesCount = 0;
    mutable byte lastAppliedPatch = 0;

  private:
    static void setButtonLabel(Function* functions, size_t functionCount, size_t buttonIndex, const char* label)
    {
        if (functions == nullptr || buttonIndex >= functionCount)
        {
            return;
        }

        functions[buttonIndex].setLabel(label);
    }
};

class MockSetListStore : public ISetListStore
{
  public:
    size_t listSetLists(byte, SetListSummary*, size_t) const override { return 0; }
    bool activateSetList(byte, const char*) override { return false; }
    bool clearActiveSetList(byte) override
    {
        ++clearActiveCalls;
        active = false;
        activeSet = {};
        return true;
    }
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
    bool selectSong(byte, size_t setSongIndex) override
    {
        if (!active || setSongIndex >= activeSet.songCount)
        {
            return false;
        }

        activeSet.selectedSongIndex = setSongIndex;
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
    ActiveSetList activeSet = {};
    int clearActiveCalls = 0;
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
    fixture.setListStore.activeSet.songs[0].patch = 3;
    std::snprintf(fixture.setListStore.activeSet.songs[0].name,
                  sizeof(fixture.setListStore.activeSet.songs[0].name), "%s",
                  "This is a very long opening song title that should truncate");

    fixture.setListStore.activeSet.songs[1].number = 2;
    fixture.setListStore.activeSet.songs[1].part = 1;
    fixture.setListStore.activeSet.songs[1].available = true;
    fixture.setListStore.activeSet.songs[1].songIndex = 1;
    fixture.setListStore.activeSet.songs[1].patch = 0;
    std::snprintf(fixture.setListStore.activeSet.songs[1].name,
                  sizeof(fixture.setListStore.activeSet.songs[1].name), "%s", "This is a very long song title that should truncate");

    fixture.setListStore.activeSet.songs[2].number = 3;
    fixture.setListStore.activeSet.songs[2].part = 1;
    fixture.setListStore.activeSet.songs[2].available = true;
    fixture.setListStore.activeSet.songs[2].songIndex = 2;
    fixture.setListStore.activeSet.songs[2].patch = 2;
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

int findDrawTextCallContainingOne(const FakeScreenUi& ui, const char* needle, uint16_t textColour)
{
    for (int index = 0; index < ui.drawTextCallCount; ++index)
    {
        if (ui.drawTextLog[index].textColour == textColour && std::strstr(ui.drawTextLog[index].label, needle) != nullptr)
        {
            return index;
        }
    }

    return -1;
}

int findDrawTextCallContainingWithFont(const FakeScreenUi& ui, const char* partA, const char* partB, const GFXfont* font,
                                       uint16_t textColour)
{
    for (int index = 0; index < ui.drawTextCallCount; ++index)
    {
        if (ui.drawTextLog[index].textColour == textColour && ui.drawTextLog[index].font == font &&
            std::strstr(ui.drawTextLog[index].label, partA) != nullptr &&
            std::strstr(ui.drawTextLog[index].label, partB) != nullptr)
        {
            return index;
        }
    }

    return -1;
}

int findDrawTextCallIndexWithFont(const FakeScreenUi& ui, const char* label, const GFXfont* font, uint16_t textColour)
{
    for (int index = 0; index < ui.drawTextCallCount; ++index)
    {
        if (ui.drawTextLog[index].textColour == textColour && ui.drawTextLog[index].font == font &&
            std::strcmp(ui.drawTextLog[index].label, label) == 0)
        {
            return index;
        }
    }

    return -1;
}

void test_play_set_mode_shows_end_of_set_prompt_in_notes_area()
{
    Fixture fixture;
    seedActiveSet(fixture);
    fixture.setListStore.activeSet.selectedSongIndex = 5;

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(5);

    const int promptIndex = findDrawTextCallIndexWithFont(fixture.ui, "END OF SET", FF24, SetButtonColour);
    TEST_ASSERT_TRUE(promptIndex >= 0);
    TEST_ASSERT_EQUAL_PTR(FF24, fixture.ui.drawTextLog[promptIndex].font);
    TEST_ASSERT_EQUAL_INT(8, fixture.ui.drawTextLog[promptIndex].x);
    TEST_ASSERT_EQUAL_INT(136, fixture.ui.drawTextLog[promptIndex].y);
}

void test_play_set_mode_truncates_long_upcoming_song_names()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_TRUE(findDrawTextCallContainingWithFont(fixture.ui, "2 ", "...", FF24, TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "2 This is a very long song title that should truncate", TFT_WHITE) < 0);
    TEST_ASSERT_TRUE(findDrawTextCallIndexWithFont(fixture.ui, "3 Bridge", FF24, TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCallIndexWithFont(fixture.ui, "4 Solo", FF24, TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCallIndexWithFont(fixture.ui, "5 Breakdown", FF24, TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "6 Outro", TFT_WHITE) < 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Part 1", TFT_WHITE) < 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "P: ", TFT_WHITE) < 0);
    TEST_ASSERT_TRUE(findDrawTextCallContainingOne(fixture.ui, "Scene:", TFT_WHITE) < 0);
    const int row1 = findDrawTextCallIndexWithFont(fixture.ui, "1/6 Opener", FF24, TFT_WHITE);
    TEST_ASSERT_TRUE(row1 >= 0);
    TEST_ASSERT_EQUAL_INT(92, fixture.ui.drawTextLog[row1].y);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "1/6 This is a very long opening song title that should truncate",
                                      TFT_WHITE) < 0);
    const int noteRow1 = findDrawTextCallContainingWithFont(fixture.ui, "This note line", "...", FF24, TFT_WHITE);
    const int noteRow2 = findDrawTextCallIndexWithFont(fixture.ui, "Drop D", FF24, TFT_WHITE);
    TEST_ASSERT_TRUE(noteRow1 >= 0);
    TEST_ASSERT_TRUE(noteRow2 >= 0);
    TEST_ASSERT_EQUAL_INT(136, fixture.ui.drawTextLog[noteRow1].y);
    TEST_ASSERT_EQUAL_INT(35, fixture.ui.drawTextLog[noteRow2].y - fixture.ui.drawTextLog[noteRow1].y);

    const int row2 = findDrawTextCallContainingWithFont(fixture.ui, "2 ", "...", FF24, TFT_WHITE);
    const int row3 = findDrawTextCallIndexWithFont(fixture.ui, "3 Bridge", FF24, TFT_WHITE);
    const int row4 = findDrawTextCallIndexWithFont(fixture.ui, "4 Solo", FF24, TFT_WHITE);
    const int row5 = findDrawTextCallIndexWithFont(fixture.ui, "5 Breakdown", FF24, TFT_WHITE);
    TEST_ASSERT_TRUE(row2 >= 0);
    TEST_ASSERT_TRUE(row3 >= 0);
    TEST_ASSERT_TRUE(row4 >= 0);
    TEST_ASSERT_TRUE(row5 >= 0);
    TEST_ASSERT_EQUAL_INT(87, fixture.ui.drawTextLog[row2].y);
    TEST_ASSERT_EQUAL_INT(39, fixture.ui.drawTextLog[row3].y - fixture.ui.drawTextLog[row2].y);
    TEST_ASSERT_EQUAL_INT(39, fixture.ui.drawTextLog[row4].y - fixture.ui.drawTextLog[row3].y);
    TEST_ASSERT_EQUAL_INT(39, fixture.ui.drawTextLog[row5].y - fixture.ui.drawTextLog[row4].y);
}

void test_play_set_mode_refreshes_patch_overrides_when_advancing_set_song()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.buttonOverrideStore.applyOverridesCount);
    TEST_ASSERT_EQUAL_UINT8(3, fixture.buttonOverrideStore.lastAppliedPatch);

    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(2, fixture.buttonOverrideStore.applyOverridesCount);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.buttonOverrideStore.lastAppliedPatch);
}

void test_play_set_mode_clears_previous_button_overrides_when_advancing_set_song()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    FootSwitchTouchButton* button1 = fixture.touchButtonManager.getButton(0);
    FootSwitchTouchButton* button2 = fixture.touchButtonManager.getButton(1);
    FootSwitchTouchButton* button4 = fixture.touchButtonManager.getButton(3);
    TEST_ASSERT_NOT_NULL(button1);
    TEST_ASSERT_NOT_NULL(button2);
    TEST_ASSERT_NOT_NULL(button4);

    TEST_ASSERT_EQUAL_STRING("Everlong", button1->label());
    TEST_ASSERT_EQUAL_STRING("Opener", button2->label());
    TEST_ASSERT_FALSE(button4->isEnabled());

    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_STRING("Killing", button1->label());
    TEST_ASSERT_EQUAL_STRING("Malice", button2->label());
    TEST_ASSERT_TRUE(button4->isEnabled());
    TEST_ASSERT_EQUAL_STRING("Extra", button4->label());

    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_STRING("Clean", button1->label());
    TEST_ASSERT_EQUAL_STRING("Crunch", button2->label());
    TEST_ASSERT_FALSE(button4->isEnabled());
    TEST_ASSERT_EQUAL_INT(3, fixture.buttonOverrideStore.applyOverridesCount);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.buttonOverrideStore.lastAppliedPatch);
}

void test_play_set_mode_encoder_pressed_unloads_active_set_and_returns_home()
{
    Fixture fixture;
    seedActiveSet(fixture);

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.setListStore.clearActiveCalls);
    TEST_ASSERT_FALSE(fixture.setListStore.active);
    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}
} // namespace

void setUp() {}
void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_play_set_mode_shows_end_of_set_prompt_in_notes_area);
    RUN_TEST(test_play_set_mode_truncates_long_upcoming_song_names);
    RUN_TEST(test_play_set_mode_refreshes_patch_overrides_when_advancing_set_song);
    RUN_TEST(test_play_set_mode_clears_previous_button_overrides_when_advancing_set_song);
    RUN_TEST(test_play_set_mode_encoder_pressed_unloads_active_set_and_returns_home);
    return UNITY_END();
}
