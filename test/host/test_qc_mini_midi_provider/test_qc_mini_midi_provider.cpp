#include <unity.h>

#include "MidiProvider.h"

#include "../../../src/QCMiniMidiProvider.cpp"

namespace
{
class MockMidiManager : public IMidiManager
{
  public:
    enum class MessageType : uint8_t
    {
        ProgramChange,
        ControlChange,
    };

    void setChannel(byte channel) override { _channel = channel; }
    byte channel() const override { return _channel; }

    void sendProgramChange(byte programChangeValue) override
    {
        messageTypes[messageCount] = MessageType::ProgramChange;
        firstValues[messageCount] = programChangeValue;
        secondValues[messageCount] = 0;
        ++messageCount;
    }

    void sendControlChange(byte controlChangeNumber, byte controlChangeValue) override
    {
        messageTypes[messageCount] = MessageType::ControlChange;
        firstValues[messageCount] = controlChangeNumber;
        secondValues[messageCount] = controlChangeValue;
        ++messageCount;
    }

    MessageType messageTypes[8] = {};
    byte firstValues[8] = {0};
    byte secondValues[8] = {0};
    int messageCount = 0;

  private:
    byte _channel = 1;
};

void test_recall_preset_sends_bank_folder_then_program_change()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.recallPreset(6);

    TEST_ASSERT_EQUAL_INT(3, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[0]));
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.secondValues[0]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[1]));
    TEST_ASSERT_EQUAL_UINT8(32, midiManager.firstValues[1]);
    TEST_ASSERT_EQUAL_UINT8(1, midiManager.secondValues[1]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ProgramChange),
                            static_cast<uint8_t>(midiManager.messageTypes[2]));
    TEST_ASSERT_EQUAL_UINT8(6, midiManager.firstValues[2]);
}

void test_select_playlist_changes_setlist_used_for_preset_recall()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.selectPlaylist(4);
    provider.recallPreset(0);

    TEST_ASSERT_EQUAL_INT(3, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.secondValues[0]);
    TEST_ASSERT_EQUAL_UINT8(32, midiManager.firstValues[1]);
    TEST_ASSERT_EQUAL_UINT8(4, midiManager.secondValues[1]);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.firstValues[2]);
}

void test_recall_preset_127_stays_in_first_group()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.recallPreset(127);

    TEST_ASSERT_EQUAL_INT(3, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.secondValues[0]);
    TEST_ASSERT_EQUAL_UINT8(127, midiManager.firstValues[2]);
}

void test_select_scene_sends_qc_scene_control_change()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.selectScene(2);

    TEST_ASSERT_EQUAL_INT(1, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[0]));
    TEST_ASSERT_EQUAL_UINT8(43, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(2, midiManager.secondValues[0]);
}

void test_enable_tuner_sends_qc_tuner_control_change()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.setTunerEnabled(true);

    TEST_ASSERT_EQUAL_INT(1, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[0]));
    TEST_ASSERT_EQUAL_UINT8(45, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(127, midiManager.secondValues[0]);
}

void test_disable_tuner_sends_qc_tuner_control_change_zero()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.setTunerEnabled(false);

    TEST_ASSERT_EQUAL_INT(1, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[0]));
    TEST_ASSERT_EQUAL_UINT8(45, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.secondValues[0]);
}

void test_enable_gig_view_sends_qc_gig_view_open_control_change()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.setGigViewEnabled(true);

    TEST_ASSERT_EQUAL_INT(1, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[0]));
    TEST_ASSERT_EQUAL_UINT8(46, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(127, midiManager.secondValues[0]);
}

void test_disable_gig_view_sends_qc_gig_view_close_control_change()
{
    MockMidiManager midiManager;
    QCMiniMidiProvider provider(midiManager);

    provider.setGigViewEnabled(false);

    TEST_ASSERT_EQUAL_INT(1, midiManager.messageCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MockMidiManager::MessageType::ControlChange),
                            static_cast<uint8_t>(midiManager.messageTypes[0]));
    TEST_ASSERT_EQUAL_UINT8(46, midiManager.firstValues[0]);
    TEST_ASSERT_EQUAL_UINT8(0, midiManager.secondValues[0]);
}
} // namespace

void setUp() {}

void tearDown() {}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_recall_preset_sends_bank_folder_then_program_change);
    RUN_TEST(test_select_playlist_changes_setlist_used_for_preset_recall);
    RUN_TEST(test_recall_preset_127_stays_in_first_group);
    RUN_TEST(test_select_scene_sends_qc_scene_control_change);
    RUN_TEST(test_enable_tuner_sends_qc_tuner_control_change);
    RUN_TEST(test_disable_tuner_sends_qc_tuner_control_change_zero);
    RUN_TEST(test_enable_gig_view_sends_qc_gig_view_open_control_change);
    RUN_TEST(test_disable_gig_view_sends_qc_gig_view_close_control_change);
    return UNITY_END();
}
