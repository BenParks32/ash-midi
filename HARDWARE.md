# Hardware Notes

## SN74AHCT125N

The final stable hardware fix for both MIDI output and LED data was an SN74AHCT125N powered from 5V.

Reason:
- The previous level-shifter path was corrupting MIDI output.
- The same path also degraded NeoPixel / WS2812 data quality.
- Replacing it with SN74AHCT125N cleaned up both signals.

### Pinout

With the DIP package notch facing up:

- Pin 1: 1OE
- Pin 2: 1A
- Pin 3: 1Y
- Pin 4: 2OE
- Pin 5: 2A
- Pin 6: 2Y
- Pin 7: GND
- Pin 8: 3Y
- Pin 9: 3A
- Pin 10: 3OE
- Pin 11: 4Y
- Pin 12: 4A
- Pin 13: 4OE
- Pin 14: VCC

### MIDI OUT Wiring

Recommended channel assignment:

- Pin 14 -> +5V
- Pin 7 -> GND
- 100 nF capacitor between pin 14 and pin 7, placed close to the IC
- Pin 1 (1OE) -> GND
- MCU UART TX -> Pin 2 (1A)
- Pin 3 (1Y) -> 220R -> DIN pin 5
- +5V -> 220R -> DIN pin 4
- DIN pin 2 -> GND

DIN pins 1 and 3 are not used.

### LED Data Wiring

Recommended channel assignment:

- Pin 4 (2OE) -> GND
- MCU LED data pin -> Pin 5 (2A)
- Pin 6 (2Y) -> 330R -> first LED DIN

Notes:

- Grounds must be common between MCU, SN74AHCT125N, LED supply, and MIDI hardware.
- The SN74AHCT125N works well here because it accepts a 3.3V logic high on its input while producing a clean 5V output.

### Unused Gates

- Tie unused OE pins high to disable their outputs.
- Tie unused A inputs to GND or +5V so they do not float.

## SD Card Findings

The SD card behavior turned out to be much more dependent on the card than expected.

Observed behavior:

- Full-size SD cards were stable.
- Multiple microSD cards showed unreliable remount behavior.
- Passive microSD-to-SD adapters did not solve the issue.
- A better brand microSD card, after reformatting, behaved reliably.

Practical takeaway:

- Prefer a known-good branded card.
- Reformat suspicious cards before blaming firmware.
- Do not assume all microSD cards behave equally well in SPI mode.

## Current Firmware Assumptions

The firmware still contains the hardened SD mount path:

- shared SPI chip-select deselection
- slower SD init clock
- retry-based mount sequence
- SD init before TFT startup

Those changes are worth keeping even though the final root cause was mainly hardware / card quality.

## Stripboard Carrier Implementation

An implementation-ready stripboard plan for the Black Pill carrier is in:

- STRIPBOARD_CARRIER_V1.md

That guide includes:

- hole-grid placement
- AHCT125 track cuts and jumper nets
- four-buck power-domain distribution
- branch fuse suggestions
- staged bring-up and test procedure