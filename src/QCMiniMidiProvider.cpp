#include "MidiProvider.h"

namespace
{
constexpr byte QCMiniPresetBankMsbCc = 0;
constexpr byte QCMiniPresetFolderLsbCc = 32;
constexpr byte QCMiniSceneCc = 43;
constexpr byte QCMiniTunerCc = 45;

constexpr byte QCMiniMyPresetsFolder = 1;
constexpr byte QCMiniPresetGroupSize = 128;
constexpr byte QCMiniMaxPresetIndex = 127;
constexpr byte QCMiniMaxSceneIndex = 7;
} // namespace

QCMiniMidiProvider::QCMiniMidiProvider(IMidiManager& midiManager) : _midiManager(midiManager) {}

byte QCMiniMidiProvider::maxPresetIndex() const { return QCMiniMaxPresetIndex; }

void QCMiniMidiProvider::recallPreset(byte presetIndex)
{
    if (presetIndex > QCMiniMaxPresetIndex)
    {
        Serial.printf("QC Mini: preset %u out of range\n", presetIndex);
        return;
    }

    const byte presetGroup = static_cast<byte>(presetIndex / QCMiniPresetGroupSize);
    const byte programChangeValue = static_cast<byte>(presetIndex % QCMiniPresetGroupSize);

    _midiManager.sendControlChange(QCMiniPresetBankMsbCc, presetGroup);
    _midiManager.sendControlChange(QCMiniPresetFolderLsbCc, QCMiniMyPresetsFolder);
    _midiManager.sendProgramChange(programChangeValue);

    Serial.printf("QC Mini: recalled preset %u (group %u, folder %u)\n", presetIndex, presetGroup, QCMiniMyPresetsFolder);
}

void QCMiniMidiProvider::selectScene(byte sceneIndex)
{
    if (sceneIndex > QCMiniMaxSceneIndex)
    {
        Serial.printf("QC Mini: scene %u out of range\n", sceneIndex);
        return;
    }

    _midiManager.sendControlChange(QCMiniSceneCc, sceneIndex);
    Serial.printf("QC Mini: selected scene %u\n", sceneIndex);
}

void QCMiniMidiProvider::setTunerEnabled(bool enabled)
{
    _midiManager.sendControlChange(QCMiniTunerCc, enabled ? 127U : 0U);
    Serial.printf("QC Mini: tuner %s\n", enabled ? "on" : "off");
}
