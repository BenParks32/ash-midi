#pragma once

#include <Arduino.h>

class IMidiManager
{
  public:
    virtual ~IMidiManager() = default;

    virtual void sendProgramChange(byte programChangeValue) = 0;
    virtual void sendControlChange(byte controlChangeNumber, byte controlChangeValue) = 0;
};

class MidiManager : public IMidiManager
{
  public:
    MidiManager();

    void sendProgramChange(byte programChangeValue) override;
    void sendControlChange(byte controlChangeNumber, byte controlChangeValue) override;

  private:
    MidiManager(const MidiManager&) = delete;
    MidiManager(MidiManager&&) = delete;
    MidiManager& operator=(const MidiManager&) = delete;
    MidiManager& operator=(MidiManager&&) = delete;

  private:
    void ensureInitialized();

  private:
    bool _initialized;
};
