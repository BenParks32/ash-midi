#pragma once

#include <Arduino.h>

class IMidiManager
{
  public:
    virtual ~IMidiManager() = default;

    virtual void setChannel(byte channel) = 0;
    virtual byte channel() const = 0;
    virtual void sendProgramChange(byte programChangeValue) = 0;
    virtual void sendControlChange(byte controlChangeNumber, byte controlChangeValue) = 0;
};

class MidiManager : public IMidiManager
{
  public:
    MidiManager();

    void setChannel(byte channel) override;
    byte channel() const override;
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
    byte _channel;
};
