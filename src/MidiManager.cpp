#include "MidiManager.h"

#include <MIDI.h>

namespace
{
constexpr byte kMidiChannel = 1;
HardwareSerial MidiUart(PB7, PB6);
MIDI_CREATE_INSTANCE(HardwareSerial, MidiUart, MidiOut);
} // namespace

MidiManager::MidiManager() : _initialized(false) {}

void MidiManager::ensureInitialized()
{
    if (_initialized)
    {
        return;
    }

    MidiUart.begin(31250, SERIAL_8N1);
    MidiOut.begin(kMidiChannel);
    _initialized = true;
}

void MidiManager::sendProgramChange(byte programChangeValue)
{
    ensureInitialized();
    MidiOut.sendProgramChange(programChangeValue, kMidiChannel);
    Serial.printf("MIDI: sent program change %u on channel %u\n", programChangeValue, kMidiChannel);
}
