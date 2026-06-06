#include <cstdio>
#include <cstring>
#include <unity.h>

#include "MidiProvider.h"
#include "Modes/SetSelectionMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/SetSelectionMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
constexpr uint16_t ExpectedActiveSetColour = 0x07FF;
constexpr byte TestPlaylistIndex = 2;

class FakeScreenUi : public IScreenUi
{
  public:
    struct DrawTextCall
    {
        char label[96];
        uint16_t textColour;
    };

    struct FillRectCall
    {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
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
    void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t) override
    {
        if (fillRectCallCount < MaxFillRectCalls)
        {
            fillRectLog[fillRectCallCount] = {x, y, width, height};
            ++fillRectCallCount;
        }
    }
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

    static constexpr int MaxDrawTextCalls = 48;
    static constexpr int MaxFillRectCalls = 32;
    DrawTextCall drawTextLog[MaxDrawTextCalls] = {};
    int drawTextCallCount = 0;
    FillRectCall fillRectLog[MaxFillRectCalls] = {};
    int fillRectCallCount = 0;
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
    size_t listSetLists(byte, SetListSummary* setLists, size_t maxSetLists) const override
    {
        if (screenUi != nullptr)
        {
            for (int index = 0; index < screenUi->drawTextCallCount; ++index)
            {
                if (std::strcmp(screenUi->drawTextLog[index].label, "Loading...") == 0)
                {
                    loadingLabelDrawnBeforeListLoad = true;
                    break;
                }
            }
        }

        const size_t copyCount = (setListCount < maxSetLists) ? setListCount : maxSetLists;
        for (size_t index = 0; index < copyCount; ++index)
        {
            setLists[index] = availableSetLists[index];
        }
        return copyCount;
    }

    bool activateSetList(byte, const char* fileName) override
    {
        if (screenUi != nullptr)
        {
            fillRectCallCountAtActivate = screenUi->fillRectCallCount;
            for (int index = 0; index < screenUi->drawTextCallCount; ++index)
            {
                if (std::strcmp(screenUi->drawTextLog[index].label, "Loading...") == 0)
                {
                    loadingLabelDrawnBeforeActivate = true;
                    break;
                }
            }
        }

        ++activateCalls;
        std::snprintf(lastActivatedFileName, sizeof(lastActivatedFileName), "%s", fileName != nullptr ? fileName : "");

        for (size_t index = 0; index < setListCount; ++index)
        {
            if (std::strcmp(availableSetLists[index].fileName, lastActivatedFileName) != 0)
            {
                continue;
            }

            active = true;
            activeSet = {};
            std::snprintf(activeSet.name, sizeof(activeSet.name), "%s", availableSetLists[index].name);
            std::snprintf(activeSet.sourcePath, sizeof(activeSet.sourcePath), "/sets/p7/%s", availableSetLists[index].fileName);
            return true;
        }

        return false;
    }

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
        const char* fileName = std::strrchr(activeSet.sourcePath, '/');
        std::snprintf(summary.fileName, sizeof(summary.fileName), "%s", fileName != nullptr ? fileName + 1 : "");
        summary.partCount = activeSet.partCount;
        summary.songCount = activeSet.songCount;
        return true;
    }
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

    bool selectSong(byte, size_t) override { return false; }
    bool selectedSong(byte, SetListSongEntry&) const override { return false; }

    SetListSummary availableSetLists[4] = {};
    size_t setListCount = 0;
    bool active = false;
    ActiveSetList activeSet = {};
    int activateCalls = 0;
    int clearActiveCalls = 0;
    char lastActivatedFileName[SetListSummary::MaxFileNameLength] = {};
    const FakeScreenUi* screenUi = nullptr;
    mutable bool loadingLabelDrawnBeforeListLoad = false;
    bool loadingLabelDrawnBeforeActivate = false;
    int fillRectCallCountAtActivate = -1;
};

class Fixture
{
  public:
    Fixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), midiProvider(), setListStore(), transitionDelegate(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, setListStore, transitionDelegate)
    {
        setListStore.screenUi = &ui;
    }

    FakeScreenUi ui;
    Adafruit_NeoPixel strip;
    RingManager ringManager;
    TouchButtonManager touchButtonManager;
    MockMidiManager midiManager;
    MockMidiProvider midiProvider;
    MockSetListStore setListStore;
    MockTransitionDelegate transitionDelegate;
    SetSelectionMode mode;
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

int countDrawTextCalls(const FakeScreenUi& ui, const char* label)
{
    int count = 0;
    for (int index = 0; index < ui.drawTextCallCount; ++index)
    {
        if (std::strcmp(ui.drawTextLog[index].label, label) == 0)
        {
            ++count;
        }
    }

    return count;
}

int findFillRectCall(const FakeScreenUi& ui, int32_t x, int32_t y, int32_t width, int32_t height)
{
    for (int index = 0; index < ui.fillRectCallCount; ++index)
    {
        const FakeScreenUi::FillRectCall& call = ui.fillRectLog[index];
        if (call.x == x && call.y == y && call.width == width && call.height == height)
        {
            return index;
        }
    }

    return -1;
}

int countFillRectCalls(const FakeScreenUi& ui, int32_t x, int32_t y, int32_t width, int32_t height)
{
    int count = 0;
    for (int index = 0; index < ui.fillRectCallCount; ++index)
    {
        const FakeScreenUi::FillRectCall& call = ui.fillRectLog[index];
        if (call.x == x && call.y == y && call.width == width && call.height == height)
        {
            ++count;
        }
    }

    return count;
}

void seedSet(Fixture& fixture, size_t index, const char* name, const char* fileName)
{
    std::snprintf(fixture.setListStore.availableSetLists[index].name,
                  sizeof(fixture.setListStore.availableSetLists[index].name), "%s", name);
    std::snprintf(fixture.setListStore.availableSetLists[index].fileName,
                  sizeof(fixture.setListStore.availableSetLists[index].fileName), "%s", fileName);
}

void test_set_selection_mode_uses_agreed_button_map_and_marks_active_set()
{
    Fixture fixture;
    fixture.setListStore.setListCount = 2;
    seedSet(fixture, 0, "Friday", "friday.jsn");
    seedSet(fixture, 1, "Saturday", "saturday.jsn");
    fixture.setListStore.active = true;
    std::snprintf(fixture.setListStore.activeSet.name, sizeof(fixture.setListStore.activeSet.name), "%s", "Friday");
    std::snprintf(fixture.setListStore.activeSet.sourcePath, sizeof(fixture.setListStore.activeSet.sourcePath), "%s",
                  "/sets/p7/friday.jsn");

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    TEST_ASSERT_TRUE(fixture.setListStore.loadingLabelDrawnBeforeListLoad);
    TEST_ASSERT_EQUAL_STRING("Select", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Last", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("PgDn", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING("Down", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Back", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("First", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING("PgUp", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Up", fixture.touchButtonManager.getButton(7)->label());
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Loaded: Friday", ExpectedActiveSetColour) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Sets", TFT_WHITE) < 0);
    TEST_ASSERT_TRUE(findFillRectCall(fixture.ui, 5, 82, 470, 156) >= 0);
    TEST_ASSERT_TRUE(findFillRectCall(fixture.ui, 5, 141, 470, 29) >= 0);
}

void test_set_selection_mode_selects_highlighted_set_without_leaving_screen()
{
    Fixture fixture;
    fixture.setListStore.setListCount = 2;
    seedSet(fixture, 0, "Friday", "friday.jsn");
    seedSet(fixture, 1, "Saturday", "saturday.jsn");

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    const int fillRectCallCountBeforeSelect = fixture.ui.fillRectCallCount;
    const int fullListClearCountBeforeSelect = countFillRectCalls(fixture.ui, 5, 82, 470, 156);
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.setListStore.activateCalls);
    TEST_ASSERT_EQUAL_STRING("friday.jsn", fixture.setListStore.lastActivatedFileName);
    TEST_ASSERT_TRUE(fixture.setListStore.loadingLabelDrawnBeforeActivate);
    TEST_ASSERT_EQUAL_INT(fillRectCallCountBeforeSelect, fixture.setListStore.fillRectCallCountAtActivate);
    TEST_ASSERT_EQUAL_INT(fullListClearCountBeforeSelect, countFillRectCalls(fixture.ui, 5, 82, 470, 156));
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Loading...", TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Loaded: Friday", ExpectedActiveSetColour) >= 0);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_set_selection_mode_redraws_previous_active_row_without_clearing_page()
{
    Fixture fixture;
    fixture.setListStore.setListCount = 2;
    seedSet(fixture, 0, "Friday", "friday.jsn");
    seedSet(fixture, 1, "Saturday", "saturday.jsn");
    fixture.setListStore.active = true;
    std::snprintf(fixture.setListStore.activeSet.name, sizeof(fixture.setListStore.activeSet.name), "%s", "Saturday");
    std::snprintf(fixture.setListStore.activeSet.sourcePath, sizeof(fixture.setListStore.activeSet.sourcePath), "%s",
                  "/sets/p7/saturday.jsn");

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(7);
    const int fullListClearCountBeforeSelect = countFillRectCalls(fixture.ui, 5, 82, 470, 156);
    const int saturdayRowRedrawsBeforeSelect = countFillRectCalls(fixture.ui, 5, 173, 470, 29);
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(fullListClearCountBeforeSelect, countFillRectCalls(fixture.ui, 5, 82, 470, 156));
    TEST_ASSERT_TRUE(countFillRectCalls(fixture.ui, 5, 173, 470, 29) > saturdayRowRedrawsBeforeSelect);
    TEST_ASSERT_TRUE(findDrawTextCall(fixture.ui, "Loaded: Friday", ExpectedActiveSetColour) >= 0);
}

void test_set_selection_mode_can_clear_active_set_without_leaving_screen()
{
    Fixture fixture;
    fixture.setListStore.setListCount = 1;
    seedSet(fixture, 0, "Friday", "friday.jsn");
    fixture.setListStore.active = true;
    std::snprintf(fixture.setListStore.activeSet.name, sizeof(fixture.setListStore.activeSet.name), "%s", "Friday");
    std::snprintf(fixture.setListStore.activeSet.sourcePath, sizeof(fixture.setListStore.activeSet.sourcePath), "%s",
                  "/sets/p7/friday.jsn");

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(5);
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.setListStore.clearActiveCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_set_selection_mode_back_returns_to_play_with_current_context()
{
    Fixture fixture;

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Play), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(TestPlaylistIndex, 6, false),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_set_selection_mode_keeps_loaded_label_visible_while_paging()
{
    Fixture fixture;
    fixture.setListStore.setListCount = 5;
    seedSet(fixture, 0, "Friday", "friday.jsn");
    seedSet(fixture, 1, "Saturday", "saturday.jsn");
    seedSet(fixture, 2, "Red Lion", "red-lion.jsn");
    seedSet(fixture, 3, "King's Head", "kings-head.jsn");
    seedSet(fixture, 4, "White Hart", "white-hart.jsn");
    fixture.setListStore.active = true;
    std::snprintf(fixture.setListStore.activeSet.name, sizeof(fixture.setListStore.activeSet.name), "%s", "Friday");
    std::snprintf(fixture.setListStore.activeSet.sourcePath, sizeof(fixture.setListStore.activeSet.sourcePath), "%s",
                  "/sets/p7/friday.jsn");

    fixture.mode.setTransitionValue(makePlayModeTransition(TestPlaylistIndex, 6, false));
    fixture.mode.activate();

    const int loadedLabelDrawsBeforePaging = countDrawTextCalls(fixture.ui, "Loaded: Friday");
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(loadedLabelDrawsBeforePaging, countDrawTextCalls(fixture.ui, "Loaded: Friday"));
    TEST_ASSERT_TRUE(findFillRectCall(fixture.ui, 5, 109, 470, 129) >= 0);
}
} // namespace

void setUp() {}
void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_set_selection_mode_uses_agreed_button_map_and_marks_active_set);
    RUN_TEST(test_set_selection_mode_selects_highlighted_set_without_leaving_screen);
    RUN_TEST(test_set_selection_mode_redraws_previous_active_row_without_clearing_page);
    RUN_TEST(test_set_selection_mode_can_clear_active_set_without_leaving_screen);
    RUN_TEST(test_set_selection_mode_back_returns_to_play_with_current_context);
    RUN_TEST(test_set_selection_mode_keeps_loaded_label_visible_while_paging);
    return UNITY_END();
}
