#pragma once
#include <Arduino.h>

const byte STOMP_SNAPSHOT_CC = 69;
const byte STOMP_TUNER_CC = 68;
const byte STOMP_LOOP_RECORD_OVERDUB_CC = 60;
const byte STOMP_LOOP_PLAY_STOP_CC = 61;
const byte STOMP_LOOP_PLAY_ONCE_CC = 62;
const byte STOMP_LOOP_UNDO_REDO_CC = 63;
const byte STOMP_LOOP_FORWARD_REV_CC = 65;
const byte STOMP_LOOP_FULL_HALF_SPEED_CC = 66;

const byte STOMP_LOOP_OVERDUB_VAL = 0;
const byte STOMP_LOOP_RECORD_VAL = 64;
const byte STOMP_LOOP_STOP_VAL = 0;
const byte STOMP_LOOP_PLAY_VAL = 64;
const byte STOMP_LOOP_PLAY_ONCE_VAL = 64;
const byte STOMP_LOOP_UNDO_REDO_VAL = 64;
const byte STOMP_LOOP_FORWARD_VAL = 0;
const byte STOMP_LOOP_REVERSE_VAL = 64;
const byte STOMP_LOOP_FULL_SPEED_VAL = 0;
const byte STOMP_LOOP_HALF_SPEED_VAL = 0;