#include <cstdio>
#include <cstring>
#include <unity.h>

#include "ButtonOverrideStore.h"
#include "Modes/ModeManager.h"
#include "Modes/PlayMode.h"

#include "../../../src/Function.cpp"
#include "../../../src/Lights.cpp"
#include "../../../src/TapTempoEngine.cpp"
#include "../../../src/Modes/FunctionModeBase.cpp"
#include "../../../src/Modes/ModeManager.cpp"
#include "../../../src/Modes/PlayMode.cpp"
#include "../../../src/RingManager.cpp"
#include "../../../src/Touch/TouchButton.cpp"
#include "../../../src/Touch/TouchButtonManager.cpp"

namespace
{
constexpr ModeTransitionValue HomePlaylistTransitionFlag = 0x0200;
constexpr byte DefaultPlaylistIndex = 1;
constexpr byte V40PlaylistIndex = 2;
constexpr byte AmplessPlaylistIndex = 3;
constexpr byte CodeRedPlaylistIndex = 4;

constexpr ModeTransitionValue hostHomePlaylistTransitionValue(byte playlistIndex)
{
    return static_cast<ModeTransitionValue>(HomePlaylistTransitionFlag | playlistIndex);
}

class FakeScreenUi : public IScreenUi
{
  public:
    struct DrawTextCall
    {
        char label[64];
        const GFXfont* font;
        uint8_t scale;
        int32_t x;
        int32_t y;
        uint16_t textColour;
        uint16_t backgroundColour;
    };

    void drawBackgroundAndBorder() override { ++drawBackgroundAndBorderCalls; }

    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t) override { ++drawCenteredFrameCalls; }

    void drawCenteredFrame(int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t, uint16_t) override
    {
        ++drawCenteredFrameCalls;
    }

    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*) override
    {
        ++drawLogoCalls;
    }

    void drawLogo(const GFXfont*, uint8_t, const char*, const GFXfont*, uint8_t, const char*, uint16_t, uint16_t,
                  uint16_t, uint16_t) override
    {
        ++drawLogoWithColoursCalls;
    }

    void drawText(const GFXfont* font, uint8_t scale, const char* label, int32_t x, int32_t y, uint16_t textColour,
                  uint16_t backgroundColour) override
    {
        ++drawTextCalls;
        if (drawTextCallCount < MaxDrawTextCalls)
        {
            DrawTextCall& call = drawTextLog[drawTextCallCount++];
            std::snprintf(call.label, sizeof(call.label), "%s", (label != nullptr) ? label : "");
            call.font = font;
            call.scale = scale;
            call.x = x;
            call.y = y;
            call.textColour = textColour;
            call.backgroundColour = backgroundColour;
        }
    }

    void fillRect(int32_t, int32_t, int32_t, int32_t, uint16_t) override { ++fillRectCalls; }
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

    static constexpr int MaxDrawTextCalls = 64;
    DrawTextCall drawTextLog[MaxDrawTextCalls] = {};
    int drawTextCallCount = 0;
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
    Modes lastMode = Modes::Play;
    ModeTransitionValue lastTransitionValue = ModeTransitionNone;
};

class MockButtonOverrideStore : public IButtonOverrideStore
{
  public:
    bool refresh() override
    {
        ++refreshCalls;
        return refreshResult;
    }

    void applyOverrides(byte playlistIndex, byte patchNumber, Function* functions, size_t functionCount,
                        PatchDisplayConfig* patchDisplay = nullptr) const override
    {
        ++applyCalls;
        lastPlaylistIndex = playlistIndex;
        lastPatchNumber = patchNumber;

        if (patchDisplay != nullptr)
        {
            patchDisplay->name[0] = '\0';
            if (enablePatchName)
            {
                const char* patchName = (patchNumber == alternatePatchNumber) ? alternatePatchName : defaultPatchName;
                const char* patchLongName =
                    (patchNumber == alternatePatchNumber) ? alternatePatchLongName : defaultPatchLongName;
                const char* patchDisplayName =
                    (enableLongPatchName && patchLongName[0] != '\0') ? patchLongName : patchName;
                std::snprintf(patchDisplay->name, PatchDisplayConfig::NameCapacity, "%s", patchDisplayName);
            }
        }

        if (functions == nullptr)
        {
            return;
        }

        if (enableMomentaryOverride && functionCount > 3)
        {
            functions[3] = Function("Hold", 0xF81F, FunctionAction(ActionType::SendMidiControlChange, 21, 127),
                                    FunctionAction(ActionType::SendMidiControlChange, 21, 0),
                                    FunctionAction(ActionType::SelectScene, 2, 0),
                                    FunctionAction(ActionType::ChangeMode, static_cast<byte>(Modes::Home), 0));
        }

        if (disableTunerToggle && functionCount > 7)
        {
            functions[7] = Function("Tuner", 0x801F, ActionType::SetTuner, 1, ActionType::SetTuner, 0);
            functions[7].setToggle(false);
        }

        if (enableGenericToggleOverride && functionCount > 6)
        {
            functions[6] = Function("E Flat", 0xFD20, ActionType::SendMidiControlChange, 35, ActionType::None, 0);
            functions[6].setToggle(true);
        }

        if (enablePatchButtonOverride && functionCount > 4)
        {
            functions[4] = Function("Override", 0xF800, ActionType::ChangeMode, static_cast<byte>(Modes::Home),
                                    ActionType::ChangeMode, static_cast<byte>(Modes::Home));
        }

        if (!enableTapOverride || tapButtonIndex >= functionCount)
        {
            return;
        }

        functions[tapButtonIndex] = Function("Tap", 0x001F, ActionType::TapTempo, 0, ActionType::None, 0);
    }

    bool songForIndex(byte playlistIndex, byte songIndex, SongConfig* outSong) const override
    {
        ++songLookupCalls;
        lastSongPlaylistIndex = playlistIndex;
        lastSongIndex = songIndex;

        if (!enableSongLookup || outSong == nullptr || songIndex != configuredSongIndex)
        {
            return false;
        }

        outSong->patchNumber = configuredSongPatchNumber;
        std::snprintf(outSong->name, sizeof(outSong->name), "%s", configuredSongName);
        const char* displayName = (configuredSongLongName[0] != '\0') ? configuredSongLongName : configuredSongName;
        std::snprintf(outSong->displayName, sizeof(outSong->displayName), "%s", displayName);
        return true;
    }

    mutable int applyCalls = 0;
    mutable byte lastPlaylistIndex = 0;
    mutable byte lastPatchNumber = 0;
    mutable int songLookupCalls = 0;
    mutable byte lastSongPlaylistIndex = 0;
    mutable byte lastSongIndex = 0;
    int refreshCalls = 0;
    bool refreshResult = true;
    bool disableTunerToggle = false;
    bool enableGenericToggleOverride = false;
    bool enableMomentaryOverride = false;
    bool enablePatchButtonOverride = false;
    bool enablePatchName = false;
    bool enableLongPatchName = false;
    bool enableSongLookup = false;
    bool enableTapOverride = false;
    char defaultPatchName[PatchDisplayConfig::NameCapacity] = "Big Sky Intro";
    char alternatePatchName[PatchDisplayConfig::NameCapacity] = "Massive Lead Wall";
    char defaultPatchLongName[PatchDisplayConfig::NameCapacity] = "";
    char alternatePatchLongName[PatchDisplayConfig::NameCapacity] = "";
    char configuredSongName[PatchDisplayConfig::NameCapacity] = "Oughta";
    char configuredSongLongName[PatchDisplayConfig::NameCapacity] = "Oughta Know";
    byte configuredSongIndex = 2;
    byte configuredSongPatchNumber = 9;
    byte alternatePatchNumber = 9;
    byte tapButtonIndex = 3;
};

class MockSetListStore : public ISetListStore
{
  public:
    size_t listSetLists(byte, SetListSummary*, size_t) const override { return 0; }
    bool activateSetList(byte, const char*) override { return false; }
    bool clearActiveSetList(byte) override { return true; }
    bool activeSetList(byte, ActiveSetList& setList) const override
    {
        if (!hasActiveSet)
        {
            return false;
        }

        setList = activeSet;
        return true;
    }
    bool activeSetSummary(byte, SetListSummary&) const override { return false; }
    bool activeSetPosition(byte, size_t& songCount, size_t& selectedSongIndex) const override
    {
        if (!hasActiveSet)
        {
            return false;
        }

        songCount = activeSet.songCount;
        selectedSongIndex = activeSet.selectedSongIndex;
        return true;
    }
    bool activeSetSongAt(byte, size_t setSongIndex, SetListSongEntry& song) const override
    {
        if (!hasActiveSet || setSongIndex >= activeSet.songCount)
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
    bool selectSong(byte playlistIndex, size_t setSongIndex) override
    {
        lastPlaylistIndex = playlistIndex;
        lastSetSongIndex = static_cast<byte>(setSongIndex);
        ++selectSongCalls;
        hasSelectedSong = true;
        selectedSongIndex = static_cast<byte>(setSongIndex);
        if (hasActiveSet && setSongIndex < activeSet.songCount)
        {
            activeSet.selectedSongIndex = setSongIndex;
            hasActiveSong = true;
            activeSong = activeSet.songs[setSongIndex];
        }
        return true;
    }
    bool selectedSong(byte playlistIndex, SetListSongEntry& song) const override
    {
        lastPlaylistIndex = playlistIndex;
        if (hasActiveSet && activeSet.songCount > 0 && activeSet.selectedSongIndex < activeSet.songCount)
        {
            song = activeSet.songs[activeSet.selectedSongIndex];
            return true;
        }

        if (!hasSelectedSong || !hasActiveSong)
        {
            return false;
        }

        song = activeSong;
        return true;
    }

    mutable byte lastPlaylistIndex = 0;
    mutable byte lastSetSongIndex = 0;
    int selectSongCalls = 0;
    bool hasSelectedSong = false;
    byte selectedSongIndex = 0;
    bool hasActiveSong = false;
    SetListSongEntry activeSong = {};
    bool hasActiveSet = false;
    ActiveSetList activeSet = {};
};

class MockMidiProvider : public IMidiProvider
{
  public:
    enum class CallType : uint8_t
    {
        SelectPlaylist,
        RecallPreset,
        SelectScene,
        SetTuner,
        SetGigView,
    };

    byte maxPresetIndex() const override { return 127; }
    byte defaultPlaylistIndex() const override { return DefaultPlaylistIndex; }

    void selectPlaylist(byte playlistIndex) override
    {
        ++selectPlaylistCalls;
        lastPlaylistIndex = playlistIndex;
        callOrder[callCount++] = CallType::SelectPlaylist;
    }

    void recallPreset(byte presetIndex) override
    {
        ++recallPresetCalls;
        lastRecallPreset = presetIndex;
        callOrder[callCount++] = CallType::RecallPreset;
    }

    void selectScene(byte sceneIndex) override
    {
        ++selectSceneCalls;
        lastSceneIndex = sceneIndex;
        callOrder[callCount++] = CallType::SelectScene;
    }

    void setTunerEnabled(bool enabled) override
    {
        ++setTunerCalls;
        lastTunerEnabled = enabled;
        callOrder[callCount++] = CallType::SetTuner;
    }

    void setGigViewEnabled(bool enabled) override
    {
        ++setGigViewCalls;
        lastGigViewEnabled = enabled;
        callOrder[callCount++] = CallType::SetGigView;
    }

    int selectPlaylistCalls = 0;
    int recallPresetCalls = 0;
    int selectSceneCalls = 0;
    int setTunerCalls = 0;
    int setGigViewCalls = 0;
    byte lastPlaylistIndex = DefaultPlaylistIndex;
    byte lastRecallPreset = 0;
    byte lastSceneIndex = 0;
    bool lastTunerEnabled = false;
    bool lastGigViewEnabled = false;
    CallType callOrder[8] = {};
    int callCount = 0;
};

class PlayModeFixture
{
  public:
    PlayModeFixture()
        : ui(), strip(RingManager::LedCount, 0, NEO_GRB + NEO_KHZ800), ringManager(strip), touchButtonManager(ui),
          midiManager(), midiProvider(), transitionDelegate(), overrideStore(), setListStore(),
          mode(touchButtonManager, ringManager, ui, midiManager, midiProvider, overrideStore, setListStore,
               transitionDelegate)
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
    MockSetListStore setListStore;
    PlayMode mode;
};

uint32_t ringPixelColour(const PlayModeFixture& fixture, byte ringIndex)
{
    return fixture.strip.getPixelColor(static_cast<uint16_t>(ringIndex) * RingManager::LedsPerRing);
}

int findDrawTextCallIndex(const FakeScreenUi& ui, const char* label, uint16_t textColour, int startIndex = 0)
{
    for (int i = startIndex; i < ui.drawTextCallCount; ++i)
    {
        if (std::strcmp(ui.drawTextLog[i].label, label) == 0 && ui.drawTextLog[i].textColour == textColour)
        {
            return i;
        }
    }

    return -1;
}

bool hasDrawTextPrefix(const FakeScreenUi& ui, const char* prefix, uint16_t textColour)
{
    const size_t prefixLength = std::strlen(prefix);
    for (int i = 0; i < ui.drawTextCallCount; ++i)
    {
        if (ui.drawTextLog[i].textColour == textColour &&
            std::strncmp(ui.drawTextLog[i].label, prefix, prefixLength) == 0)
        {
            return true;
        }
    }

    return false;
}

void test_activate_sets_play_labels()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Clean", fixture.touchButtonManager.getButton(0)->label());
    TEST_ASSERT_EQUAL_STRING("Crunch", fixture.touchButtonManager.getButton(1)->label());
    TEST_ASSERT_EQUAL_STRING("Lead", fixture.touchButtonManager.getButton(2)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_EQUAL_STRING("Patch", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_STRING("Set", fixture.touchButtonManager.getButton(5)->label());
    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_STRING("Tuner", fixture.touchButtonManager.getButton(7)->label());
}

void test_activate_applies_tap_to_configured_button()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Tap", fixture.touchButtonManager.getButton(3)->label());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());
}

void test_activate_renders_patch_name_with_prefix_when_provided()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;

    fixture.mode.activate();

    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Patch: Big Sky Intro", TFT_WHITE) >= 0);
}

void test_activate_prefers_patch_long_name_when_provided()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    fixture.overrideStore.enableLongPatchName = true;
    std::snprintf(fixture.overrideStore.defaultPatchName, sizeof(fixture.overrideStore.defaultPatchName), "%s", "Big Sky");
    std::snprintf(fixture.overrideStore.defaultPatchLongName, sizeof(fixture.overrideStore.defaultPatchLongName), "%s",
                  "Big Sky Intro");

    fixture.mode.activate();

    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Patch: Big Sky Intro", TFT_WHITE) >= 0);
}

void test_activate_renders_patch_name_at_updated_offset()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;

    fixture.mode.activate();

    const int patchNameCallIndex = findDrawTextCallIndex(fixture.ui, "Patch: Big Sky Intro", TFT_WHITE);
    TEST_ASSERT_TRUE(patchNameCallIndex >= 0);
    TEST_ASSERT_EQUAL_INT(24, fixture.ui.drawTextLog[patchNameCallIndex].x);
    TEST_ASSERT_EQUAL_INT(130, fixture.ui.drawTextLog[patchNameCallIndex].y);
}

void test_activate_song_context_recalls_linked_patch_and_uses_song_display_name()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    fixture.overrideStore.enableSongLookup = true;
    fixture.mode.setTransitionValue(makePlayModeSongTransition(DefaultPlaylistIndex, fixture.overrideStore.configuredSongIndex, true));

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.overrideStore.songLookupCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.overrideStore.lastSongPlaylistIndex);
    TEST_ASSERT_EQUAL_UINT8(fixture.overrideStore.configuredSongIndex, fixture.overrideStore.lastSongIndex);
    TEST_ASSERT_EQUAL_UINT8(fixture.overrideStore.configuredSongPatchNumber, fixture.overrideStore.lastPatchNumber);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(fixture.overrideStore.configuredSongPatchNumber, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Song: Oughta Know", TFT_WHITE) >= 0);
    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Patch: Massive Lead Wall", TFT_WHITE) >= 0);
}

void test_activate_song_context_falls_back_to_song_short_name_when_long_name_missing()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    fixture.overrideStore.enableSongLookup = true;
    fixture.overrideStore.configuredSongLongName[0] = '\0';
    fixture.mode.setTransitionValue(makePlayModeSongTransition(DefaultPlaylistIndex, fixture.overrideStore.configuredSongIndex, true));

    fixture.mode.activate();

    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Song: Oughta", TFT_WHITE) >= 0);
}

void test_activate_song_label_renders_above_patch_label()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    fixture.overrideStore.enableSongLookup = true;
    fixture.mode.setTransitionValue(makePlayModeSongTransition(DefaultPlaylistIndex, fixture.overrideStore.configuredSongIndex, true));

    fixture.mode.activate();

    const int songLabelCallIndex = findDrawTextCallIndex(fixture.ui, "Song: Oughta Know", TFT_WHITE);
    const int patchLabelCallIndex = findDrawTextCallIndex(fixture.ui, "Patch: Massive Lead Wall", TFT_WHITE);
    const int snapshotLabelCallIndex = findDrawTextCallIndex(fixture.ui, "CLEAN", TFT_WHITE);
    TEST_ASSERT_TRUE(songLabelCallIndex >= 0);
    TEST_ASSERT_TRUE(patchLabelCallIndex >= 0);
    TEST_ASSERT_TRUE(snapshotLabelCallIndex >= 0);
    TEST_ASSERT_EQUAL_INT(24, fixture.ui.drawTextLog[songLabelCallIndex].x);
    TEST_ASSERT_EQUAL_INT(24, fixture.ui.drawTextLog[patchLabelCallIndex].x);
    TEST_ASSERT_EQUAL_INT(93, fixture.ui.drawTextLog[songLabelCallIndex].y);
    TEST_ASSERT_EQUAL_INT(130, fixture.ui.drawTextLog[patchLabelCallIndex].y);
    TEST_ASSERT_EQUAL_INT(172, fixture.ui.drawTextLog[snapshotLabelCallIndex].y);
    TEST_ASSERT_TRUE(fixture.ui.drawTextLog[songLabelCallIndex].y < fixture.ui.drawTextLog[patchLabelCallIndex].y);
    TEST_ASSERT_TRUE(fixture.ui.drawTextLog[patchLabelCallIndex].y < fixture.ui.drawTextLog[snapshotLabelCallIndex].y);
}

void test_activate_without_patch_name_does_not_render_patch_name_prefix()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_FALSE(hasDrawTextPrefix(fixture.ui, "Patch: ", TFT_WHITE));
}

void test_patch_name_is_truncated_for_long_labels()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    std::snprintf(fixture.overrideStore.defaultPatchName, sizeof(fixture.overrideStore.defaultPatchName),
                  "%s", "This Is A Very Long Patch Name For A Big Ambient Intro");

    fixture.mode.activate();

    TEST_ASSERT_TRUE(hasDrawTextPrefix(fixture.ui, "Patch: This Is A Very Lon...", TFT_WHITE));
}

void test_tap_tempo_button_label_updates_to_bpm_when_interval_is_known()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    TEST_ASSERT_EQUAL_STRING("Tap", fixture.touchButtonManager.getButton(3)->label());

    fixture.mode.buttonPressed(3);
    TEST_ASSERT_EQUAL_STRING("Tap", fixture.touchButtonManager.getButton(3)->label());

    delay(500);
    fixture.mode.buttonPressed(3);
    TEST_ASSERT_EQUAL_STRING("120", fixture.touchButtonManager.getButton(3)->label());
}

void test_activate_refreshes_button_overrides_for_selected_playlist_and_patch()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(hostHomePlaylistTransitionValue(CodeRedPlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.overrideStore.refreshCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.overrideStore.applyCalls);
    TEST_ASSERT_EQUAL_UINT8(CodeRedPlaylistIndex, fixture.overrideStore.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.overrideStore.lastPatchNumber);
}

void test_activate_selects_single_button_and_dims_others()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(2)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(4)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(5)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(0)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(1)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(2)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(4)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(5)->pillColour());
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(6)->pillColour());
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(7)->pillColour());
}

void test_songs_button_enters_song_mode_from_patch_context()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();
    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Songs), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(DefaultPlaylistIndex, 6, false), fixture.transitionDelegate.lastTransitionValue);
}

void test_songs_button_preserves_song_context_when_entering_song_mode()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableSongLookup = true;

    fixture.mode.setTransitionValue(makePlayModeSongTransition(DefaultPlaylistIndex, fixture.overrideStore.configuredSongIndex, true));
    fixture.mode.activate();
    fixture.mode.buttonPressed(5);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Songs), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(
        makePlayModeSongTransition(DefaultPlaylistIndex, fixture.overrideStore.configuredSongIndex, false),
        fixture.transitionDelegate.lastTransitionValue);
}

void test_next_button_advances_selected_set_song_context()
{
    PlayModeFixture fixture;
    fixture.setListStore.hasActiveSet = true;
    fixture.setListStore.activeSet.songCount = 2;
    fixture.setListStore.activeSet.selectedSongIndex = 0;
    fixture.setListStore.activeSet.songs[0].available = true;
    fixture.setListStore.activeSet.songs[0].patch = 12;
    std::snprintf(fixture.setListStore.activeSet.songs[0].name, sizeof(fixture.setListStore.activeSet.songs[0].name), "%s",
                  "Set Song 1");
    fixture.setListStore.activeSet.songs[1].available = true;
    fixture.setListStore.activeSet.songs[1].patch = 15;
    std::snprintf(fixture.setListStore.activeSet.songs[1].name, sizeof(fixture.setListStore.activeSet.songs[1].name), "%s",
                  "Set Song 2");

    fixture.mode.setTransitionValue(makePlayModeTransition(DefaultPlaylistIndex, 12, true));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::PlaySet), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(DefaultPlaylistIndex, 12, true),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_activate_with_unresolved_set_song_keeps_patch_and_renders_unavailable_in_red()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    fixture.setListStore.hasSelectedSong = true;
    fixture.setListStore.selectedSongIndex = 4;
    fixture.setListStore.hasActiveSong = true;
    fixture.setListStore.activeSong.available = false;
    std::snprintf(fixture.setListStore.activeSong.name, sizeof(fixture.setListStore.activeSong.name), "%s",
                  "Missing Song");

    fixture.mode.setTransitionValue(makePlayModeSetSongTransition(DefaultPlaylistIndex, 6, true));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.overrideStore.lastPatchNumber);
    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Song: Missing Song", TFT_RED) < 0);
}

void test_next_button_requires_second_press_to_wrap_end_of_set()
{
    PlayModeFixture fixture;
    fixture.setListStore.hasActiveSet = true;
    fixture.setListStore.activeSet.songCount = 1;
    fixture.setListStore.activeSet.selectedSongIndex = 0;
    fixture.setListStore.activeSet.songs[0].available = true;
    fixture.setListStore.activeSet.songs[0].patch = 11;
    std::snprintf(fixture.setListStore.activeSet.songs[0].name, sizeof(fixture.setListStore.activeSet.songs[0].name), "%s",
                  "Solo");

    fixture.mode.setTransitionValue(makePlayModeTransition(DefaultPlaylistIndex, 11, true));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(0, fixture.setListStore.selectSongCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::PlaySet), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(DefaultPlaylistIndex, 11, true),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_activate_with_active_set_and_manual_patch_transition_keeps_manual_patch()
{
    PlayModeFixture fixture;
    fixture.setListStore.hasActiveSet = true;
    fixture.setListStore.activeSet.songCount = 2;
    fixture.setListStore.activeSet.selectedSongIndex = 0;
    fixture.setListStore.activeSet.songs[0].available = true;
    fixture.setListStore.activeSet.songs[0].patch = 12;
    std::snprintf(fixture.setListStore.activeSet.songs[0].name, sizeof(fixture.setListStore.activeSet.songs[0].name), "%s",
                  "Set Song 1");
    fixture.setListStore.activeSet.songs[1].available = true;
    fixture.setListStore.activeSet.songs[1].patch = 15;
    std::snprintf(fixture.setListStore.activeSet.songs[1].name, sizeof(fixture.setListStore.activeSet.songs[1].name), "%s",
                  "Set Song 2");

    fixture.mode.setTransitionValue(makePlayModeTransition(DefaultPlaylistIndex, 22, true));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::PlaySet), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(DefaultPlaylistIndex, 22, true),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_play_set_set_button_opens_manage_set_mode()
{
    PlayModeFixture fixture;
    fixture.setListStore.hasActiveSet = true;
    fixture.setListStore.activeSet.songCount = 1;
    fixture.setListStore.activeSet.selectedSongIndex = 0;
    fixture.setListStore.activeSet.songs[0].available = true;
    fixture.setListStore.activeSet.songs[0].patch = 7;
    std::snprintf(fixture.setListStore.activeSet.songs[0].name, sizeof(fixture.setListStore.activeSet.songs[0].name), "%s",
                  "Song A");

    fixture.mode.setTransitionValue(makePlayModeTransition(DefaultPlaylistIndex, 7, true));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::PlaySet), static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
}

void test_activate_recalls_selected_preset_before_scene_select()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
    TEST_ASSERT_EQUAL_INT(3, fixture.midiProvider.callCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::SelectPlaylist),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[0]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::RecallPreset),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[1]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiProvider::CallType::SelectScene),
                            static_cast<uint8_t>(fixture.midiProvider.callOrder[2]));
}

void test_momentary_override_uses_button_down_and_release_and_suppresses_short_and_long_press()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableMomentaryOverride = true;

    fixture.mode.setSelectedPreset(9);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_STRING("Hold", fixture.touchButtonManager.getButton(3)->label());

    const int baselineControlChanges = fixture.midiManager.controlChangeCalls;
    fixture.mode.buttonDown(3);
    TEST_ASSERT_EQUAL_INT(baselineControlChanges + 1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(21, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(127, fixture.midiManager.lastControlChangeValue);

    fixture.mode.buttonPressed(6);
    fixture.mode.buttonLongPressed(3);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(baselineControlChanges + 1, fixture.midiManager.controlChangeCalls);

    fixture.mode.buttonReleased(3);
    TEST_ASSERT_EQUAL_INT(baselineControlChanges + 2, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(21, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiManager.lastControlChangeValue);
}

void test_activate_recalls_selected_preset_zero()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(0);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
}

void test_activate_recalls_v40_playlist_preset_1a()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(hostHomePlaylistTransitionValue(V40PlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(V40PlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_recalls_ampless_playlist_preset_1a()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(hostHomePlaylistTransitionValue(AmplessPlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(AmplessPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_recalls_code_red_playlist_preset_1a()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(hostHomePlaylistTransitionValue(CodeRedPlaylistIndex));
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(CodeRedPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastRecallPreset);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_skips_program_change_for_none_transition()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionNone);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_skips_program_change_for_patch_return_transition()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | 9);
    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.recallPresetCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_activate_selects_first_scene()
{
    PlayModeFixture fixture;

    fixture.mode.activate();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectPlaylistCalls);
    TEST_ASSERT_EQUAL_UINT8(DefaultPlaylistIndex, fixture.midiProvider.lastPlaylistIndex);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
}

void test_scene_a_selects_scene_zero()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(0);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(0, fixture.midiProvider.lastSceneIndex);
}

void test_scene_b_selects_scene_one()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(1);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.midiProvider.lastSceneIndex);
}

void test_scene_c_selects_scene_two()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.midiProvider.lastSceneIndex);
}

void test_removed_gig_button_is_empty_and_inactive()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(6);
    fixture.mode.buttonLongPressed(6);

    TEST_ASSERT_EQUAL_STRING(" ", fixture.touchButtonManager.getButton(6)->label());
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.setGigViewCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, ringPixelColour(fixture, 6));
}

void test_tuner_button_long_press_opens_gig_view_without_changing_scene_selection()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setGigViewCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastGigViewEnabled);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());
}

void test_tuner_button_second_long_press_closes_gig_view()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(7);
    fixture.mode.buttonLongPressed(7);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.setGigViewCalls);
    TEST_ASSERT_FALSE(fixture.midiProvider.lastGigViewEnabled);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tuner_button_long_press_does_not_light_removed_gig_button_ring()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    const uint32_t initialColour = ringPixelColour(fixture, 6);

    fixture.mode.buttonLongPressed(7);
    const uint32_t afterLongPressColour = ringPixelColour(fixture, 6);

    TEST_ASSERT_EQUAL_UINT32(0U, initialColour);
    TEST_ASSERT_EQUAL_UINT32(0U, afterLongPressColour);
}

void test_tap_tempo_button_does_not_send_midi_on_first_press_without_changing_scene_selection()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(3)->hasBorder());
}

void test_tap_tempo_button_does_not_send_midi_on_second_press()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_button_sends_cc44_value100_on_scheduled_interval_after_third_press()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(49);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(1);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(44, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(100, fixture.midiManager.lastControlChangeValue);
}

void test_tap_tempo_continued_tapping_does_not_delay_next_scheduled_midi_send()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(45);
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(5);
    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(44, fixture.midiManager.lastControlChangeNumber);
    TEST_ASSERT_EQUAL_UINT8(100, fixture.midiManager.lastControlChangeValue);
}

void test_tap_tempo_light_flashes_using_recent_tap_average()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());

    delay(35);
    const int drawLabelCallsBeforeFlash = fixture.ui.drawTouchButtonLabelAndPillCalls;
    const int drawPillCallsBeforeFlash = fixture.ui.drawTouchButtonPillCalls;
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());
    TEST_ASSERT_EQUAL_INT(drawLabelCallsBeforeFlash, fixture.ui.drawTouchButtonLabelAndPillCalls);
    TEST_ASSERT_EQUAL_INT(drawPillCallsBeforeFlash + 1, fixture.ui.drawTouchButtonPillCalls);

    delay(35);
    const int drawLabelCallsBeforeSecondFlash = fixture.ui.drawTouchButtonLabelAndPillCalls;
    const int drawPillCallsBeforeSecondFlash = fixture.ui.drawTouchButtonPillCalls;
    fixture.mode.frameTick();
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());
    TEST_ASSERT_EQUAL_INT(drawLabelCallsBeforeSecondFlash, fixture.ui.drawTouchButtonLabelAndPillCalls);
    TEST_ASSERT_EQUAL_INT(drawPillCallsBeforeSecondFlash + 1, fixture.ui.drawTouchButtonPillCalls);
}

void test_tap_tempo_light_returns_to_solid_blue_after_ten_seconds()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    delay(35);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());

    delay(10020);
    fixture.mode.frameTick();
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());

    delay(35);
    fixture.mode.frameTick();
    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());
}

void test_tap_tempo_press_after_inactivity_restarts_solid_flash_window()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    delay(9990);
    fixture.mode.buttonPressed(3);
    delay(35);
    fixture.mode.frameTick();

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(3)->pillColour());
}

void test_tap_tempo_pre_arm_taps_reset_after_three_seconds_of_silence()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

    delay(3100);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    delay(50);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    fixture.mode.buttonPressed(3);
    delay(49);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);

    delay(1);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_inactivity_sends_three_trailing_messages_then_stops()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(1000);
    fixture.mode.buttonPressed(3);
    delay(1000);
    fixture.mode.buttonPressed(3);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(1, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(2, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(3, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(4, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(5, fixture.midiManager.controlChangeCalls);

    delay(1000);
    fixture.mode.frameTick();
    TEST_ASSERT_EQUAL_INT(5, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_deactivate_clears_pending_scheduler()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);
    delay(50);
    fixture.mode.buttonPressed(3);

    fixture.mode.deactivate();
    delay(60);
    fixture.mode.activate();
    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tap_tempo_button_long_press_is_noop()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(3);

    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_tuner_button_enables_tuner_without_changing_scene_selection()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastTunerEnabled);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());
}

void test_tuner_button_second_short_press_disables_tuner()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_FALSE(fixture.midiProvider.lastTunerEnabled);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tuner_button_short_press_and_long_press_toggle_independent_states()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);
    fixture.mode.buttonLongPressed(7);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastTunerEnabled);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.setGigViewCalls);
    TEST_ASSERT_TRUE(fixture.midiProvider.lastGigViewEnabled);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_tuner_button_can_be_configured_as_non_toggle()
{
    PlayModeFixture fixture;
    fixture.overrideStore.disableTunerToggle = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);
    fixture.mode.buttonPressed(7);
    fixture.mode.buttonLongPressed(7);

    TEST_ASSERT_EQUAL_INT(3, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_FALSE(fixture.midiProvider.lastTunerEnabled);
}

void test_generic_toggle_button_dims_until_pressed_and_toggles_on_second_press()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableGenericToggleOverride = true;

    fixture.mode.activate();
    const uint32_t dimColour = ringPixelColour(fixture, 6);

    fixture.mode.buttonPressed(6);
    const uint32_t fullColour = ringPixelColour(fixture, 6);

    fixture.mode.buttonPressed(6);
    const uint32_t dimAgainColour = ringPixelColour(fixture, 6);

    TEST_ASSERT_NOT_EQUAL(0U, dimColour);
    TEST_ASSERT_TRUE(fullColour > dimColour);
    TEST_ASSERT_TRUE(dimAgainColour < fullColour);
    TEST_ASSERT_EQUAL_INT(2, fixture.midiManager.controlChangeCalls);
    TEST_ASSERT_EQUAL_UINT8(35, fixture.midiManager.lastControlChangeNumber);
}

void test_tuner_button_flashes_when_enabled_without_redrawing_label()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_NOT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(7)->pillColour());

    delay(500);
    const int drawLabelCallsBeforeFlash = fixture.ui.drawTouchButtonLabelAndPillCalls;
    const int drawPillCallsBeforeFlash = fixture.ui.drawTouchButtonPillCalls;
    fixture.mode.frameTick();

    TEST_ASSERT_EQUAL_UINT16(TFT_BLACK, fixture.touchButtonManager.getButton(7)->pillColour());
    TEST_ASSERT_EQUAL_INT(drawLabelCallsBeforeFlash, fixture.ui.drawTouchButtonLabelAndPillCalls);
    TEST_ASSERT_EQUAL_INT(drawPillCallsBeforeFlash + 1, fixture.ui.drawTouchButtonPillCalls);
}

void test_patch_change_reactivation_preserves_tuner_toggle_state()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(7);
    fixture.mode.deactivate();
    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | 9);
    fixture.mode.activate();
    fixture.mode.buttonPressed(7);

    TEST_ASSERT_EQUAL_INT(2, fixture.midiProvider.setTunerCalls);
    TEST_ASSERT_FALSE(fixture.midiProvider.lastTunerEnabled);
}

void test_deactivate_clears_rendered_patch_name_in_black()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;

    fixture.mode.activate();
    const int drawTextCallsBeforeDeactivate = fixture.ui.drawTextCallCount;

    fixture.mode.deactivate();

    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Patch: Big Sky Intro", TFT_BLACK, drawTextCallsBeforeDeactivate) >= 0);
}

void test_deactivate_clears_rendered_song_name_in_black()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;
    fixture.overrideStore.enableSongLookup = true;
    fixture.mode.setTransitionValue(makePlayModeSongTransition(DefaultPlaylistIndex, fixture.overrideStore.configuredSongIndex, true));

    fixture.mode.activate();
    const int drawTextCallsBeforeDeactivate = fixture.ui.drawTextCallCount;

    fixture.mode.deactivate();

    TEST_ASSERT_TRUE(findDrawTextCallIndex(fixture.ui, "Song: Oughta Know", TFT_BLACK, drawTextCallsBeforeDeactivate) >= 0);
}

void test_patch_change_renders_new_patch_name_after_clearing_previous_one()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchName = true;

    fixture.mode.activate();
    const int drawTextCallsBeforePatchChange = fixture.ui.drawTextCallCount;

    fixture.mode.deactivate();
    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | fixture.overrideStore.alternatePatchNumber);
    fixture.mode.activate();

    const int clearCallIndex =
        findDrawTextCallIndex(fixture.ui, "Patch: Big Sky Intro", TFT_BLACK, drawTextCallsBeforePatchChange);
    const int renderCallIndex =
        findDrawTextCallIndex(fixture.ui, "Patch: Massive Lead Wall", TFT_WHITE, drawTextCallsBeforePatchChange);

    TEST_ASSERT_TRUE(clearCallIndex >= 0);
    TEST_ASSERT_TRUE(renderCallIndex > clearCallIndex);
}

void test_button_press_changes_selection_to_single_button()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonPressed(2);

    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(0)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(1)->hasBorder());
    TEST_ASSERT_TRUE(fixture.touchButtonManager.getButton(2)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(4)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(5)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(6)->hasBorder());
    TEST_ASSERT_FALSE(fixture.touchButtonManager.getButton(7)->hasBorder());
}

void test_button_pressed_ignores_disabled_button()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
}

void test_long_press_is_noop()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.buttonLongPressed(0);

    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
}

void test_button_five_long_press_transitions_to_patch_mode()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();
    fixture.mode.buttonLongPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patch),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(6, fixture.transitionDelegate.lastTransitionValue);
}

void test_encoder_press_transitions_to_home()
{
    PlayModeFixture fixture;

    fixture.mode.activate();
    fixture.mode.encoderPressed();

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Home),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(ModeTransitionNone, fixture.transitionDelegate.lastTransitionValue);
}

void test_tap_tempo_button_does_not_change_mode()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enableTapOverride = true;

    fixture.mode.activate();
    fixture.mode.buttonPressed(3);

    TEST_ASSERT_EQUAL_INT(0, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_INT(1, fixture.midiProvider.selectSceneCalls);
    TEST_ASSERT_EQUAL_INT(0, fixture.midiManager.controlChangeCalls);
}

void test_button_five_short_press_transitions_to_patches_mode()
{
    PlayModeFixture fixture;

    fixture.mode.setSelectedPreset(6);
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patches),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(fixture.midiProvider.defaultPlaylistIndex(), 6, false),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_uses_patch_return_value_for_next_patches_entry()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(ModeTransitionPatchReturnFlag | 11);
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patches),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(fixture.midiProvider.defaultPlaylistIndex(), 11, false),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_uses_home_playlist_preset_zero_for_next_patches_entry()
{
    PlayModeFixture fixture;

    fixture.mode.setTransitionValue(hostHomePlaylistTransitionValue(CodeRedPlaylistIndex));
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patches),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(CodeRedPlaylistIndex, 0, false),
                             fixture.transitionDelegate.lastTransitionValue);
}

void test_button_five_override_is_ignored()
{
    PlayModeFixture fixture;
    fixture.overrideStore.enablePatchButtonOverride = true;

    fixture.mode.setSelectedPreset(4);
    fixture.mode.activate();
    fixture.mode.buttonPressed(4);

    TEST_ASSERT_EQUAL_STRING("Patch", fixture.touchButtonManager.getButton(4)->label());
    TEST_ASSERT_EQUAL_INT(1, fixture.transitionDelegate.calls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Modes::Patches),
                            static_cast<uint8_t>(fixture.transitionDelegate.lastMode));
    TEST_ASSERT_EQUAL_UINT16(makePlayModeTransition(fixture.midiProvider.defaultPlaylistIndex(), 4, false),
                             fixture.transitionDelegate.lastTransitionValue);
}
} // namespace

void setUp()
{
    HostArduino::resetTime();
    HostArduino::resetPins();
    HostArduino::resetRandom();
}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_activate_sets_play_labels);
    RUN_TEST(test_activate_applies_tap_to_configured_button);
    RUN_TEST(test_activate_renders_patch_name_with_prefix_when_provided);
    RUN_TEST(test_activate_prefers_patch_long_name_when_provided);
    RUN_TEST(test_activate_renders_patch_name_at_updated_offset);
    RUN_TEST(test_activate_song_context_recalls_linked_patch_and_uses_song_display_name);
    RUN_TEST(test_activate_song_context_falls_back_to_song_short_name_when_long_name_missing);
    RUN_TEST(test_activate_song_label_renders_above_patch_label);
    RUN_TEST(test_activate_without_patch_name_does_not_render_patch_name_prefix);
    RUN_TEST(test_patch_name_is_truncated_for_long_labels);
    RUN_TEST(test_tap_tempo_button_label_updates_to_bpm_when_interval_is_known);
    RUN_TEST(test_activate_refreshes_button_overrides_for_selected_playlist_and_patch);
    RUN_TEST(test_activate_selects_single_button_and_dims_others);
    RUN_TEST(test_songs_button_enters_song_mode_from_patch_context);
    RUN_TEST(test_songs_button_preserves_song_context_when_entering_song_mode);
    RUN_TEST(test_next_button_advances_selected_set_song_context);
    RUN_TEST(test_activate_with_unresolved_set_song_keeps_patch_and_renders_unavailable_in_red);
    RUN_TEST(test_next_button_requires_second_press_to_wrap_end_of_set);
    RUN_TEST(test_activate_with_active_set_and_manual_patch_transition_keeps_manual_patch);
    RUN_TEST(test_play_set_set_button_opens_manage_set_mode);
    RUN_TEST(test_activate_recalls_selected_preset_before_scene_select);
    RUN_TEST(test_momentary_override_uses_button_down_and_release_and_suppresses_short_and_long_press);
    RUN_TEST(test_activate_recalls_selected_preset_zero);
    RUN_TEST(test_activate_recalls_v40_playlist_preset_1a);
    RUN_TEST(test_activate_recalls_ampless_playlist_preset_1a);
    RUN_TEST(test_activate_recalls_code_red_playlist_preset_1a);
    RUN_TEST(test_activate_skips_program_change_for_none_transition);
    RUN_TEST(test_activate_skips_program_change_for_patch_return_transition);
    RUN_TEST(test_activate_selects_first_scene);
    RUN_TEST(test_scene_a_selects_scene_zero);
    RUN_TEST(test_scene_b_selects_scene_one);
    RUN_TEST(test_scene_c_selects_scene_two);
    RUN_TEST(test_removed_gig_button_is_empty_and_inactive);
    RUN_TEST(test_tuner_button_long_press_opens_gig_view_without_changing_scene_selection);
    RUN_TEST(test_tuner_button_second_long_press_closes_gig_view);
    RUN_TEST(test_tuner_button_long_press_does_not_light_removed_gig_button_ring);
    RUN_TEST(test_tap_tempo_button_does_not_send_midi_on_first_press_without_changing_scene_selection);
    RUN_TEST(test_tap_tempo_button_does_not_send_midi_on_second_press);
    RUN_TEST(test_tap_tempo_button_sends_cc44_value100_on_scheduled_interval_after_third_press);
    RUN_TEST(test_tap_tempo_continued_tapping_does_not_delay_next_scheduled_midi_send);
    RUN_TEST(test_tap_tempo_light_flashes_using_recent_tap_average);
    RUN_TEST(test_tap_tempo_light_returns_to_solid_blue_after_ten_seconds);
    RUN_TEST(test_tap_tempo_press_after_inactivity_restarts_solid_flash_window);
    RUN_TEST(test_tap_tempo_pre_arm_taps_reset_after_three_seconds_of_silence);
    RUN_TEST(test_tap_tempo_inactivity_sends_three_trailing_messages_then_stops);
    RUN_TEST(test_tap_tempo_deactivate_clears_pending_scheduler);
    RUN_TEST(test_tap_tempo_button_long_press_is_noop);
    RUN_TEST(test_tuner_button_enables_tuner_without_changing_scene_selection);
    RUN_TEST(test_tuner_button_second_short_press_disables_tuner);
    RUN_TEST(test_tuner_button_short_press_and_long_press_toggle_independent_states);
    RUN_TEST(test_tuner_button_can_be_configured_as_non_toggle);
    RUN_TEST(test_generic_toggle_button_dims_until_pressed_and_toggles_on_second_press);
    RUN_TEST(test_tuner_button_flashes_when_enabled_without_redrawing_label);
    RUN_TEST(test_patch_change_reactivation_preserves_tuner_toggle_state);
    RUN_TEST(test_deactivate_clears_rendered_patch_name_in_black);
    RUN_TEST(test_deactivate_clears_rendered_song_name_in_black);
    RUN_TEST(test_patch_change_renders_new_patch_name_after_clearing_previous_one);
    RUN_TEST(test_button_press_changes_selection_to_single_button);
    RUN_TEST(test_button_pressed_ignores_disabled_button);
    RUN_TEST(test_long_press_is_noop);
    RUN_TEST(test_button_five_long_press_transitions_to_patch_mode);
    RUN_TEST(test_encoder_press_transitions_to_home);
    RUN_TEST(test_tap_tempo_button_does_not_change_mode);
    RUN_TEST(test_button_five_short_press_transitions_to_patches_mode);
    RUN_TEST(test_button_five_uses_patch_return_value_for_next_patches_entry);
    RUN_TEST(test_button_five_uses_home_playlist_preset_zero_for_next_patches_entry);
    RUN_TEST(test_button_five_override_is_ignored);
    return UNITY_END();
}
