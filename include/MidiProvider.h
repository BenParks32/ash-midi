#pragma once

#include <Arduino.h>

#include "MidiManager.h"

class IMidiProvider
{
  public:
    virtual ~IMidiProvider() = default;

    virtual byte maxPresetIndex() const = 0;
    virtual byte defaultPlaylistIndex() const = 0;
    virtual void selectPlaylist(byte playlistIndex) = 0;
    virtual void recallPreset(byte presetIndex) = 0;
    virtual void selectScene(byte sceneIndex) = 0;
    virtual void setTunerEnabled(bool enabled) = 0;
    virtual void setGigViewEnabled(bool enabled) = 0;
};

class QCMiniMidiProvider : public IMidiProvider
{
  public:
    explicit QCMiniMidiProvider(IMidiManager& midiManager);

    byte maxPresetIndex() const override;
    byte defaultPlaylistIndex() const override;
    void selectPlaylist(byte playlistIndex) override;
    void recallPreset(byte presetIndex) override;
    void selectScene(byte sceneIndex) override;
    void setTunerEnabled(bool enabled) override;
    void setGigViewEnabled(bool enabled) override;

  private:
    IMidiManager& _midiManager;
    byte _selectedPlaylist;
};
