#pragma once
#include <Arduino.h>

enum class Modes : uint8_t
{
    Home = 0,
    Play = 1,
    PlaySet = 2,
    Patch = 3,
    Menu = 4,
    ButtonDiagnostic = 5,
    Patches = 6,
    Songs = 7,
    SetSelection = 8,
    Count,
};

static constexpr uint8_t ModeCount = static_cast<uint8_t>(Modes::Count);
using ModeTransitionValue = uint16_t;
static constexpr ModeTransitionValue ModeTransitionNone = 0xFFFF;
static constexpr ModeTransitionValue ModeTransitionPatchReturnFlag = 0x0100;
static constexpr ModeTransitionValue ModeTransitionHomePlaylistFlag = 0x0200;
static constexpr ModeTransitionValue ModeTransitionPatchValueMask = 0x00FF;
static constexpr ModeTransitionValue ModeTransitionHomePlaylistValueMask = 0x00FF;
static constexpr ModeTransitionValue ModeTransitionPlayContextFlag = 0x8000;
static constexpr ModeTransitionValue ModeTransitionPlayReturnFlag = 0x4000;
static constexpr ModeTransitionValue ModeTransitionPlaySetSongFlag = 0x2000;
static constexpr ModeTransitionValue ModeTransitionPlaySongFlag = 0x1000;
static constexpr ModeTransitionValue ModeTransitionPlayPlaylistMask = 0x0F00;
static constexpr uint8_t ModeTransitionPlayPlaylistShift = 8;
static constexpr ModeTransitionValue ModeTransitionPlayPatchValueMask = 0x00FF;

inline ModeTransitionValue makePlayModeTransition(byte playlistIndex, byte patchNumber, bool shouldRecall)
{
    return static_cast<ModeTransitionValue>(
        ModeTransitionPlayContextFlag |
        (shouldRecall ? 0 : ModeTransitionPlayReturnFlag) |
        ((static_cast<ModeTransitionValue>(playlistIndex) << ModeTransitionPlayPlaylistShift) &
         ModeTransitionPlayPlaylistMask) |
        (static_cast<ModeTransitionValue>(patchNumber) & ModeTransitionPlayPatchValueMask));
}

inline ModeTransitionValue makePlayModeSongTransition(byte playlistIndex, byte songIndex, bool shouldRecall)
{
    return static_cast<ModeTransitionValue>(
        ModeTransitionPlayContextFlag |
        ModeTransitionPlaySongFlag |
        (shouldRecall ? 0 : ModeTransitionPlayReturnFlag) |
        ((static_cast<ModeTransitionValue>(playlistIndex) << ModeTransitionPlayPlaylistShift) &
         ModeTransitionPlayPlaylistMask) |
        (static_cast<ModeTransitionValue>(songIndex) & ModeTransitionPlayPatchValueMask));
}

inline ModeTransitionValue makePlayModeSetSongTransition(byte playlistIndex, byte patchNumber, bool shouldRecall)
{
    return static_cast<ModeTransitionValue>(
        ModeTransitionPlayContextFlag |
        ModeTransitionPlaySetSongFlag |
        (shouldRecall ? 0 : ModeTransitionPlayReturnFlag) |
        ((static_cast<ModeTransitionValue>(playlistIndex) << ModeTransitionPlayPlaylistShift) &
        ModeTransitionPlayPlaylistMask) |
        (static_cast<ModeTransitionValue>(patchNumber) & ModeTransitionPlayPatchValueMask));
}

inline bool isPlayModeTransition(ModeTransitionValue transitionValue)
{
    return (transitionValue & ModeTransitionPlayContextFlag) != 0;
}

inline bool isPlayModeSongTransition(ModeTransitionValue transitionValue)
{
    return (transitionValue & (ModeTransitionPlayContextFlag | ModeTransitionPlaySongFlag)) ==
           (ModeTransitionPlayContextFlag | ModeTransitionPlaySongFlag);
}

inline bool isPlayModeSetSongTransition(ModeTransitionValue transitionValue)
{
    return (transitionValue & (ModeTransitionPlayContextFlag | ModeTransitionPlaySetSongFlag)) ==
           (ModeTransitionPlayContextFlag | ModeTransitionPlaySetSongFlag);
}

inline bool playModeTransitionShouldRecall(ModeTransitionValue transitionValue)
{
    return (transitionValue & ModeTransitionPlayReturnFlag) == 0;
}

inline byte playModeTransitionPlaylist(ModeTransitionValue transitionValue)
{
    return static_cast<byte>((transitionValue & ModeTransitionPlayPlaylistMask) >> ModeTransitionPlayPlaylistShift);
}

inline byte playModeTransitionPatch(ModeTransitionValue transitionValue)
{
    return static_cast<byte>(transitionValue & ModeTransitionPlayPatchValueMask);
}

inline byte playModeTransitionSongIndex(ModeTransitionValue transitionValue)
{
    return static_cast<byte>(transitionValue & ModeTransitionPlayPatchValueMask);
}

class IModeTransistionDelegate
{
  public:
    virtual ~IModeTransistionDelegate() = default;
    virtual void enterMode(Modes mode, ModeTransitionValue transitionValue) = 0;
};

class IMode
{
  public:
    virtual ~IMode() = default;

    virtual void activate() = 0;
    virtual void deactivate() {}
    virtual void buttonDown(const byte number) { (void)number; }
    virtual void buttonPressed(const byte number) = 0;
    virtual void buttonLongPressed(const byte number) = 0;
    virtual void buttonReleased(const byte number) { (void)number; }
    virtual void frameTick() = 0;
    virtual void setTransitionValue(ModeTransitionValue transitionValue) { (void)transitionValue; }
    virtual void encoderRotated(int16_t steps) { (void)steps; }
    virtual void encoderPressed() {}
};