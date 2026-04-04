#include "MidiManager.h"

#include <MIDI.h>

namespace
{
constexpr byte kMidiChannelMin = 1;
constexpr byte kMidiChannelMax = 16;
HardwareSerial MidiUart(PB7, PB6);
MIDI_CREATE_INSTANCE(HardwareSerial, MidiUart, MidiOut);
} // namespace

MidiManager::MidiManager() : _initialized(false), _channel(kMidiChannelMin) {}

void MidiManager::setChannel(byte channel)
{
    byte nextChannel = channel;

    if (nextChannel < kMidiChannelMin)
    {
        nextChannel = kMidiChannelMin;
    }
    else if (nextChannel > kMidiChannelMax)
    {
        nextChannel = kMidiChannelMax;
    }

    if (nextChannel == _channel)
    {
        return;
    }

    _channel = nextChannel;
    if (_initialized)
    {
        MidiOut.begin(_channel);
    }
}

byte MidiManager::channel() const { return _channel; }

void MidiManager::ensureInitialized()
{
    if (_initialized)
    {
        return;
    }

    MidiUart.begin(31250, SERIAL_8N1);
    MidiOut.begin(_channel);
    _initialized = true;
}

void MidiManager::sendProgramChange(byte programChangeValue)
{
    ensureInitialized();
    MidiOut.sendProgramChange(programChangeValue, _channel);
    Serial.printf("MIDI: sent program change %u on channel %u\n", programChangeValue, _channel);
}

void MidiManager::sendControlChange(byte controlChangeNumber, byte controlChangeValue)
{
    ensureInitialized();
    MidiOut.sendControlChange(controlChangeNumber, controlChangeValue, _channel);
    Serial.printf("MIDI: sent control change %u value %u on channel %u\n", controlChangeNumber, controlChangeValue,
                  _channel);
}
