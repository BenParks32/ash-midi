#include <cstdio>
#include <cstring>
#include <unity.h>

#include "ButtonOverrideStore.h"
#include "MidiProvider.h"
#include "Modes/SongsMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/SongsMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
constexpr size_t TestSongCapacity = 64;

class FakeScreenUi : public IScreenUi
{
  public:
    struct DrawTextCall
    {
        char label[64];
        int32_t x;
        int32_t y;
        uint16_t textColour;
        uint16_t backgroundColour;
    };

    struct FillRectCall
    {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
        uint16_t colour;
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
    void drawText(const GFXfont*, uint8_t, const char* label, int32_t x, int32_t y, uint16_t textColour,
                  uint16_t backgroundColour) override
    {
        ++drawTextCalls;
        if (drawTextCallCount < MaxDrawTextCalls)
        {
            DrawTextCall& call = drawTextLog[drawTextCallCount++];
            std::snprintf(call.label, sizeof(call.label), "%s", label != nullptr ? label : "");
            call.x = x;
            call.y = y;
            call.textColour = textColour;
            call.backgroundColour = backgroundColour;
        }
    }
    void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t colour) override
    {
        ++fillRectCalls;
        if (fillRectCallCount < MaxFillRectCalls)
        {
            FillRectCall& call = fillRectLog[fillRectCallCount++];
            call.x = x;
            call.y = y;
            call.width = width;
            call.height = height;
            call.colour = colour;
        }
    }
    void drawRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override { ++drawRectCalls; }
    void drawSmallText(const char*, int32_t, int32_t, uint16_t, uint16_t) override { ++drawSmallTextCalls; }
    void drawCenteredText(const GFXfont*, uint8_t, const char*, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawCenteredTextCalls;
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

    static constexpr int MaxDrawTextCalls = 128;
    static constexpr int MaxFillRectCalls = 128;

    DrawTextCall drawTextLog[MaxDrawTextCalls] = {};
    FillRectCall fillRectLog[MaxFillRectCalls] = {};
    int drawBackgroundAndBorderCalls = 0;
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
    int drawTextCallCount = 0;
    int fillRectCallCount = 0;
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
    Modes lastMode = Modes::Songs;
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

    size_t listSongs(byte playlistIndex, SongListEntry* entries, size_t capacity) const override
    {
        lastPlaylistIndex = playlistIndex;
        lastCapacity = capacity;
        const size_t copyCount = (entryCount < capacity) ? entryCount : capacity;
        for (size_t i = 0; i < copyCount; ++i)
        {
            entries[i] = storedEntries[i];
        }
        return copyCount;
    }

    mutable byte lastPlaylistIndex = 0;
    mutable size_t lastCapacity = 0;
    SongListEntry storedEntries[TestSongCapacity] = {};
    size_t entryCount = 0;
};

class SongsModeFixture
{
  public:
    SongsModeFixture()
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
    SongsMode mode;
};

void seedSongs(SongsModeFixture& fixture, size_t count)
{
    fixture.overrideStore.entryCount = count;
    for (size_t i = 0; i < count; ++i)
    {
        fixture.overrideStore.storedEntries[i].songIndex = static_cast<byte>(i);
        fixture.overrideStore.storedEntries[i].patchNumber = static_cast<byte>(i + 1);
        std::snprintf(fixture.overrideStore.storedEntries[i].name, sizeof(fixture.overrideStore.storedEntries[i].name),
                      "Song %u", static_cast<unsigned int>(i));
    }
}

int findLatestDrawTextCallIndex(const FakeScreenUi& ui, const char* label)
{
    for (int i = ui.drawTextCallCount - 1; i >= 0; --i)
    {
        if (std::strcmp(ui.drawTextLog[i].label, label) == 0)
        {
            return i;
        }
    }

    return -1;
}

void test_activate_clears_screen_and_renders_top_left_song_list()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 18);

    fixture.mode.setTransitionValue(makePlayModeSongTransition(4, 2, false));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(0, fixture.ui.drawBackgroundAndBorderCalls);
    TEST_ASSERT_EQUAL_UINT8(4, fixture.overrideStore.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(TestSongCapacity), static_cast<uint32_t>(fixture.overrideStore.lastCapacity));
    TEST_ASSERT_EQUAL_STRING("Select", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Last", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("PgDn", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING("Down", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Back", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("First", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING("PgUp", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Up", fixture.touchButtonManager.getButton(7)->label());
    TEST_ASSERT_EQUAL_HEX16(0x07E0, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_EQUAL_HEX16(0x780F, fixture.touchButtonManager.getButton(1)->pillColour());
    TEST_ASSERT_EQUAL_HEX16(0x001F, fixture.touchButtonManager.getButton(2)->pillColour());
    TEST_ASSERT_EQUAL_HEX16(0xFD20, fixture.touchButtonManager.getButton(3)->pillColour());
    TEST_ASSERT_EQUAL_HEX16(0xF81F, fixture.touchButtonManager.getButton(5)->pillColour());
    TEST_ASSERT_EQUAL_HEX16(0x07FF, fixture.touchButtonManager.getButton(6)->pillColour());
    TEST_ASSERT_EQUAL_HEX16(0xFFE0, fixture.touchButtonManager.getButton(7)->pillColour());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(1)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(2)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(3)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(4)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(5)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(6)->isEnabled());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(7)->isEnabled());

    const int song0CallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 0");
    const int song1CallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 1");
    const int song4CallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 4");
    const int song2CallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 2");
    TEST_ASSERT_TRUE(song0CallIndex >= 0);
    TEST_ASSERT_TRUE(song1CallIndex >= 0);
    TEST_ASSERT_TRUE(song4CallIndex >= 0);
    TEST_ASSERT_TRUE(song2CallIndex >= 0);
    TEST_ASSERT_EQUAL_INT(15, fixture.ui.drawTextLog[song0CallIndex].x);
    TEST_ASSERT_EQUAL_INT(91, fixture.ui.drawTextLog[song0CallIndex].y);
    TEST_ASSERT_EQUAL_INT(15, fixture.ui.drawTextLog[song1CallIndex].x);
    TEST_ASSERT_EQUAL_INT(130, fixture.ui.drawTextLog[song1CallIndex].y);
    TEST_ASSERT_EQUAL_INT(252, fixture.ui.drawTextLog[song4CallIndex].x);
    TEST_ASSERT_EQUAL_INT(91, fixture.ui.drawTextLog[song4CallIndex].y);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[song2CallIndex].textColour);
    TEST_ASSERT_EQUAL_HEX16(TFT_WHITE, fixture.ui.drawTextLog[song2CallIndex].backgroundColour);
}

void test_selection_moves_down_first_column_then_right_column_and_wraps()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 16);

    fixture.mode.activate();
    fixture.mode.encoderRotated(4);

    int highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 4");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_INT(252, fixture.ui.drawTextLog[highlightedSongCallIndex].x);
    TEST_ASSERT_EQUAL_INT(91, fixture.ui.drawTextLog[highlightedSongCallIndex].y);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);

    fixture.mode.encoderRotated(12);
    highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 0");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);
    TEST_ASSERT_EQUAL_HEX16(TFT_WHITE, fixture.ui.drawTextLog[highlightedSongCallIndex].backgroundColour);
}

void test_page_change_repaints_next_song_page()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 18);

    fixture.mode.activate();
    const int drawTextCallCountAfterActivate = fixture.ui.drawTextCallCount;
    fixture.mode.encoderRotated(16);

    const int song0CallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 0");
    const int song16CallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 16");
    TEST_ASSERT_TRUE(song0CallIndex >= 0);
    TEST_ASSERT_TRUE(song16CallIndex >= 0);
    TEST_ASSERT_TRUE(song0CallIndex < drawTextCallCountAfterActivate);
    TEST_ASSERT_TRUE(song16CallIndex >= drawTextCallCountAfterActivate);
    TEST_ASSERT_EQUAL_INT(15, fixture.ui.drawTextLog[song16CallIndex].x);
    TEST_ASSERT_EQUAL_INT(91, fixture.ui.drawTextLog[song16CallIndex].y);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[song16CallIndex].textColour);
}

void test_same_page_selection_updates_only_changed_rows()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 10);

    fixture.mode.activate();
    const int fillRectCallsAfterActivate = fixture.ui.fillRectCallCount;
    const int drawTextCallsAfterActivate = fixture.ui.drawTextCallCount;

    fixture.mode.encoderRotated(1);

    TEST_ASSERT_EQUAL_INT(fillRectCallsAfterActivate + 2, fixture.ui.fillRectCallCount);
    TEST_ASSERT_EQUAL_INT(drawTextCallsAfterActivate + 2, fixture.ui.drawTextCallCount);
}

void test_button_navigation_supports_song_page_first_and_last_moves()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 18);

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

    int highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 1");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);

    fixture.mode.buttonPressed(7);
    highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 0");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);

    fixture.mode.buttonPressed(2);

    highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 8");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);
    TEST_ASSERT_EQUAL_INT(15, fixture.ui.drawTextLog[highlightedSongCallIndex].x);

    fixture.mode.buttonPressed(6);
    highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 0");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);

    fixture.mode.buttonPressed(1);
    highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 17");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);

    fixture.mode.buttonPressed(5);
    highlightedSongCallIndex = findLatestDrawTextCallIndex(fixture.ui, "Song 0");
    TEST_ASSERT_TRUE(highlightedSongCallIndex >= 0);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, fixture.ui.drawTextLog[highlightedSongCallIndex].textColour);
}

void test_encoder_press_selects_highlighted_song()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 6);

    fixture.mode.setTransitionValue(makePlayModeTransition(3, 2, false));
    fixture.mode.activate();
    fixture.mode.encoderRotated(5);
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeSongTransition(3, 5, true), fixture.transitionDelegate.lastTransitionValue);
}

void test_select_button_selects_highlighted_song()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 6);

    fixture.mode.setTransitionValue(makePlayModeTransition(3, 2, false));
    fixture.mode.activate();
    fixture.mode.encoderRotated(4);
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeSongTransition(3, 4, true), fixture.transitionDelegate.lastTransitionValue);
}

void test_back_button_returns_to_play_without_recall_for_song_context()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 6);

    fixture.mode.setTransitionValue(makePlayModeSongTransition(4, 3, true));
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeSongTransition(4, 3, false), fixture.transitionDelegate.lastTransitionValue);
}

void test_empty_list_keeps_back_enabled_and_blocks_selection()
{
    SongsModeFixture fixture;

    fixture.mode.setTransitionValue(makePlayModeTransition(4, 9, false));
    fixture.mode.activate();
    fixture.mode.encoderPressed();
    fixture.mode.buttonPressed(0);
    fixture.mode.buttonPressed(1);
    fixture.mode.buttonPressed(5);
    fixture.mode.buttonPressed(6);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_TRUE(findLatestDrawTextCallIndex(fixture.ui, "No songs") >= 0);

    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(4, 9, false), fixture.transitionDelegate.lastTransitionValue);
}

void test_deactivate_clears_list_area()
{
    SongsModeFixture fixture;
    seedSongs(fixture, 4);

    fixture.mode.setTransitionValue(makePlayModeSongTransition(4, 3, false));
    fixture.mode.activate();
    fixture.mode.deactivate();

    const FakeScreenUi::FillRectCall& lastFillRectCall = fixture.ui.fillRectLog[fixture.ui.fillRectCallCount - 1];
    TEST_ASSERT_EQUAL_INT(5, lastFillRectCall.x);
    TEST_ASSERT_EQUAL_INT(85, lastFillRectCall.y);
    TEST_ASSERT_EQUAL_INT(470, lastFillRectCall.width);
    TEST_ASSERT_EQUAL_INT(150, lastFillRectCall.height);
    TEST_ASSERT_EQUAL_HEX16(TFT_BLACK, lastFillRectCall.colour);
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
    RUN_TEST(test_activate_clears_screen_and_renders_top_left_song_list);
    RUN_TEST(test_selection_moves_down_first_column_then_right_column_and_wraps);
    RUN_TEST(test_page_change_repaints_next_song_page);
    RUN_TEST(test_same_page_selection_updates_only_changed_rows);
    RUN_TEST(test_button_navigation_supports_song_page_first_and_last_moves);
    RUN_TEST(test_encoder_press_selects_highlighted_song);
    RUN_TEST(test_select_button_selects_highlighted_song);
    RUN_TEST(test_back_button_returns_to_play_without_recall_for_song_context);
    RUN_TEST(test_empty_list_keeps_back_enabled_and_blocks_selection);
    RUN_TEST(test_deactivate_clears_list_area);
    return UNITY_END();
}
