# Stripboard Carrier V1 (Black Pill + SN74AHCT125 + Multi-Buck Power)

This is an implementation-ready first pass for a plug-in carrier board.
It assumes you already have four separate 5V buck outputs available:

- Buck A (logic): MCU + TFT + AHCT125 + SD
- Buck B (LED group 1): rings 1-3
- Buck C (LED group 2): rings 4-6
- Buck D (LED group 3): rings 7-8

## 1. Design Rules

- Use stripboard for signals and low-current routing only.
- Do not run high LED currents through raw strip copper traces.
- Use dedicated heavy wires for all LED branch 5V and high-current returns.
- Grounds are common, but all buck grounds must meet at a single physical star point.
- Keep AHCT125 close to Black Pill pins and away from heavy LED current wiring.

## 2. Board Coordinate System

- Board size: 40 columns x 26 rows.
- Columns: 1-40 (left to right).
- Rows: A-Z (top to bottom).
- Copper strips run left-right per row.
- Coordinate notation: row+column, example H24.

## 3. Physical Placement

### 3.1 Black Pill sockets (plug-in carrier)

- J_BP_L: 1x20 female header at column 8, rows C-V.
- J_BP_R: 1x20 female header at column 16, rows C-V.
- Header gap: 7 clear columns between socket rails (columns 9-15).
- Keepout for USB-C and BOOT/NRST access: rows A-B, columns 6-19.
- Insert board with USB-C toward row A.

Verified row map with USB-C toward row A:

- J_BP_L rows used here: O=PB5, P=PB6.
- J_BP_R rows used here: F=PB10, G=PB2, H=PB1, I=PB0, J=PA7, K=PA6, L=PA5, M=PA4, N=PA3, O=PA2, P=PA1, Q=PA0.

### 3.2 Buffer IC and passives

- U1 (SN74AHCT125 DIP-14 socket):
  - Left pins at column 24, rows H-N.
  - Right pins at column 27, rows H-N.
  - Notch toward row H.
- C1 (100 nF decoupling): between U1 pin 14 (H27) and U1 pin 7 (N24), shortest possible leads.
- R1 (220R, MIDI signal): vertical at J31-K31.
- R2 (220R, MIDI source 5V): vertical at H31-I31.
- R3 (330R, LED data): vertical at M31-N31.

### 3.3 Connectors and power section

- J_PWR_A_IN (logic 5V/GND): column 3, rows D-E.
- J_PWR_B_IN (LED group 1 5V/GND): column 3, rows H-I.
- J_PWR_C_IN (LED group 2 5V/GND): column 3, rows L-M.
- J_PWR_D_IN (LED group 3 5V/GND): column 3, rows P-Q.

- J_LED_DATA (2-way): column 38, rows A-B.
  - A38: LED data out
   - B38: LED ground companion (same net as LED power ground)

- J_LED_B_OUT (2-way, rings 1-3 power): column 38, rows C-D.
- J_LED_C_OUT (2-way, rings 4-6 power): column 38, rows E-F.
- J_LED_D_OUT (2-way, rings 7-8 power): column 38, rows G-H.

- J_MIDI (DIN wiring terminal, 3-way): column 38, rows I-K.
  - I38: DIN pin 5 path
  - J38: DIN pin 4 path
  - K38: DIN pin 2 (GND)

- STAR_GND lug/pad: Y20 (single star node for all buck grounds and branch returns).

### 3.4 TFT, touch, and SD connectors

- J_TFT (9-way): column 40, rows L-T.
   - L40: TFT_CS (PA1)
   - M40: TFT_DC (PA2)
   - N40: TFT_RST (PA3)
   - O40: TFT_BL optional
   - P40: TFT_SCLK (PA5)
   - Q40: TFT_MISO (PA6)
   - R40: TFT_MOSI (PA7)
   - S40: TFT_VCC
   - T40: TFT_GND

- J_TOUCH (2-way): column 40, rows J-K.
   - J40: TOUCH_CS (PB10)
   - K40: TOUCH_IRQ (PB2)

- J_SD (6-way): column 36, rows O-T.
   - O36: SD_CS (PA4)
   - P36: SD_SCLK (PA5)
   - Q36: SD_MISO (PA6)
   - R36: SD_MOSI (PA7)
   - S36: SD_VCC
   - T36: SD_GND

All TFT, touch, and SD connector pads are isolated by cuts. The moved TFT/SD placement keeps related SPI rows grouped together, but U1 row cuts at H-N prevent direct strip routing from the MCU header to the edge connectors.

### 3.5 Rotary encoder connector (isolated edge pads)

- J_ENC (5-way): column 1, rows U-Y.
   - U1: ENC_A (PB0)
   - V1: ENC_B (PB1)
   - W1: ENC_SW (PB5)
   - X1: ENC_GND
   - Y1: ENC_3V3 optional

All left-edge encoder pads above are isolated from the main strip using a cut at column 2 on the same row.

### 3.6 Branch protection

- F1 (logic branch, polyfuse 1.1A to 1.5A hold): between J_PWR_A_IN + and logic +5V feed.
- F2 (LED group 1, polyfuse about 2.0A hold): between J_PWR_B_IN + and J_LED_B_OUT +.
- F3 (LED group 2, polyfuse about 2.0A hold): between J_PWR_C_IN + and J_LED_C_OUT +.
- F4 (LED group 3, polyfuse about 1.25A hold): between J_PWR_D_IN + and J_LED_D_OUT +.

Use real measurements to adjust fuse values after first current tests.

## 4. U1 Pin Usage

With notch at row H:

- Pin 1 (H24): 1OE -> GND
- Pin 2 (I24): 1A <- Black Pill PB6 (MIDI TX) via explicit jumper
- Pin 3 (J24): 1Y -> MIDI DIN5 via R1 (220R)
- Pin 4 (K24): 2OE -> GND
- Pin 5 (L24): 2A <- Black Pill PA0 (LED data) via explicit jumper
- Pin 6 (M24): 2Y -> LED data out via R3 (330R)
- Pin 7 (N24): GND
- Pin 8 (N27): 3Y (unused)
- Pin 9 (M27): 3A (unused, tie low)
- Pin 10 (L27): 3OE (unused, tie high)
- Pin 11 (K27): 4Y (unused)
- Pin 12 (J27): 4A (unused, tie low)
- Pin 13 (I27): 4OE (unused, tie high)
- Pin 14 (H27): +5V logic

## 5. Track Cuts

Required cuts for U1 isolation:

- H25
- I25
- J25
- K25
- L25
- M25
- N25

These seven cuts isolate left-side and right-side U1 pins per row.

Required cuts for Black Pill left/right header separation:

- C11
- D11
- E11
- F11
- G11
- H11
- I11
- J11
- K11
- L11
- M11
- N11
- O11
- P11
- Q11
- R11
- S11
- T11
- U11
- V11

These twenty cuts split each MCU row between `J_BP_L` at column 8 and `J_BP_R` at column 16 so opposite-side pins do not short through strip copper.

Required cuts for U1-to-MCU row isolation:

- H20
- I20
- J20
- K20
- L20
- M20
- N20

These seven cuts stop accidental direct row connections from MCU header rows into U1 rows. Without them, U1 inputs may land on the wrong MCU pin by row alignment.

Required cuts for isolated column-38 connectors:

- A37
- B37
- C37
- D37
- E37
- F37
- G37
- H37
- I37
- J37
- K37

These cuts isolate the column-38 connectors from the main strip so they only connect through their intended jumper or power link.

Required cuts for isolated TFT and touch pads:

- J39
- K39
- L39
- M39
- N39
- O39
- P39
- Q39
- R39
- S39
- T39

These cuts isolate all TFT and touch connector pads at column 40 so each connection is made only by its intended jumper or power link.

Required cuts for isolated SD pads:

- O35
- P35
- Q35
- R35
- S35
- T35

These cuts isolate all SD pads at column 36 so each connection is made only by its intended jumper or power link.

Required cuts for left-edge isolated pads (encoder):

- U2
- V2
- W2
- X2
- Y2

These cuts isolate pads at column 1 so each encoder pin only connects through its intended jumper.

## 6. Jumper and Net List

No-inference rule: every isolated connector pad must be wired by an explicit jumper or power link from its source net. Do not assume any TFT, touch, or SD signal reaches the connector by strip continuity.

### 6.1 Control and signal jumpers

- Important: install the U1-to-MCU row isolation cuts (H20-N20) before these jumpers.
- (PB6) P6 -> I24 (U1 pin 2).
- (PA0) Q18 -> L24 (U1 pin 5).
- J24 (U1 pin 3) -> J31 (R1 top).
- K31 (R1 bottom) -> I38 (J_MIDI DIN5).
- M24 (U1 pin 6) -> M31 (R3 top).
- N31 (R3 bottom) -> A38 (J_LED_DATA data).

### 6.2 MIDI supply and ground

- Logic +5V -> H31 (R2 top).
- I31 (R2 bottom) -> J38 (J_MIDI DIN4).
- STAR_GND -> K38 (J_MIDI DIN2).

### 6.3 U1 supply and static ties

- Logic +5V -> H27 (U1 pin 14).
- STAR_GND -> N24 (U1 pin 7).
- STAR_GND -> H24 (U1 pin 1).
- STAR_GND -> K24 (U1 pin 4).
- Logic +5V -> I27 (U1 pin 13).
- Logic +5V -> L27 (U1 pin 10).
- STAR_GND -> M27 (U1 pin 9).
- STAR_GND -> J27 (U1 pin 12).

### 6.4 LED data companion ground

- STAR_GND -> B38 (J_LED_DATA ground companion).

The LED rings do not use a separate data-reference ground pin. B38 is the same LED ground net, provided only as an optional companion conductor alongside the data wire if you want the data cable to travel with a local ground.

### 6.5 TFT connector wiring

- (PA1) P18 -> L40.
- (PA2) O18 -> M40.
- (PA3) N18 -> N40.
- (PA5) L18 -> P40.
- (PA6) K18 -> Q40.
- (PA7) J18 -> R40.
- Logic +5V (or 3V3, per TFT module requirement) -> S40.
- STAR_GND -> T40.
- Optional backlight feed or PWM-controlled feed -> O40.

All TFT pads are isolated by cuts and require explicit links.

### 6.6 Touch connector wiring

- (PB10) F18 -> J40.
- (PB2) G18 -> K40.

### 6.7 SD connector wiring

- (PA4) M18 -> O36.
- Logic +5V or 3V3 (per SD module requirement) -> S36.
- STAR_GND -> T36.

For SD SPI lines choose one option set (A or B):

- Option A (direct from MCU taps):
   - (PA5) L18 -> P36.
   - (PA6) K18 -> Q36.
   - (PA7) J18 -> R36.
- Option B (branch from TFT pads):
   - P40 -> P36.
   - Q40 -> Q36.
   - R40 -> R36.

### 6.8 Encoder connector wiring

- (PB0) I18 -> U1.
- (PB1) H18 -> V1.
- (PB5) O6 -> W1.
- STAR_GND -> X1.
- Optional 3V3 feed -> Y1.

The encoder inputs in firmware use INPUT_PULLUP, so A, B, and SW are expected to switch to ground.

### 6.9 Authoritative connection manifest (build from this list)

Wire jumpers:

- J01: (PB6) P6 -> I24 (U1 pin 2, MIDI TX into buffer)
- J02: (PA0) Q18 -> L24 (U1 pin 5, LED data into buffer)
- J03: J24 -> J31 (U1 MIDI output to R1)
- J04: K31 -> I38 (R1 to MIDI DIN pin 5)
- J05: M24 -> M31 (U1 LED output to R3)
- J06: N31 -> A38 (R3 to LED data output)
- J07: (PA1) P18 -> L40 (TFT CS)
- J08: (PA2) O18 -> M40 (TFT DC)
- J09: (PA3) N18 -> N40 (TFT RST)
- J10: (PA5) L18 -> P40 (TFT SCLK)
- J11: (PA6) K18 -> Q40 (TFT MISO)
- J12: (PA7) J18 -> R40 (TFT MOSI)
- J13: (PB10) F18 -> J40 (TOUCH CS)
- J14: (PB2) G18 -> K40 (TOUCH IRQ)
- J15: (PA4) M18 -> O36 (SD CS)
- J19: (PB0) I18 -> U1 (ENC A)
- J20: (PB1) H18 -> V1 (ENC B)
- J21: (PB5) O6 -> W1 (ENC SW)
- J22: STAR_GND -> K38 (MIDI DIN2)
- J23: STAR_GND -> B38 (LED ground companion, same LED ground net)
- J24: STAR_GND -> T40 (TFT GND)
- J25: STAR_GND -> T36 (SD GND)
- J26: STAR_GND -> X1 (encoder GND)
- J27: Logic rail (5V or 3V3 per TFT module) -> S40
- J28: Logic rail (5V or 3V3 per SD module) -> S36
- J29: Optional backlight feed/PWM -> O40
- J30: Optional encoder VCC (normally not needed) -> Y1

For SD SPI lines choose one option set (A or B):

- Option A (direct from MCU taps):
   - J16A: (PA5) L18 -> P36 (SD SCLK)
   - J17A: (PA6) K18 -> Q36 (SD MISO)
   - J18A: (PA7) J18 -> R36 (SD MOSI)
- Option B (branch from TFT pads):
   - J16B: P40 -> P36 (SCLK branch)
   - J17B: Q40 -> Q36 (MISO branch)
   - J18B: R40 -> R36 (MOSI branch)

Acceptance check before power:

- Continuity exists for J01 through J15 and J19 through J30, plus exactly one SD SPI option set.
- No continuity from P36/Q36/R36 to PA5/PA6/PA7 unless Option A or Option B is installed.
- J40, K40, O40, S40, T40, O36, S36, and T36 remain open until their explicit links are installed.

### 6.10 Checkbox build checklist

Use this as a bench card while building.

Track cuts:

- [ ] H25
- [ ] I25
- [ ] J25
- [ ] K25
- [ ] L25
- [ ] M25
- [ ] N25
- [ ] C11
- [ ] D11
- [ ] E11
- [ ] F11
- [ ] G11
- [ ] H11
- [ ] I11
- [ ] J11
- [ ] K11
- [ ] L11
- [ ] M11
- [ ] N11
- [ ] O11
- [ ] P11
- [ ] Q11
- [ ] R11
- [ ] S11
- [ ] T11
- [ ] U11
- [ ] V11
- [ ] H20
- [ ] I20
- [ ] J20
- [ ] K20
- [ ] L20
- [ ] M20
- [ ] N20
- [ ] A37
- [ ] B37
- [ ] C37
- [ ] D37
- [ ] E37
- [ ] F37
- [ ] G37
- [ ] H37
- [ ] I37
- [ ] J37
- [ ] K37
- [ ] J39
- [ ] K39
- [ ] L39
- [ ] M39
- [ ] N39
- [ ] O39
- [ ] P39
- [ ] Q39
- [ ] R39
- [ ] S39
- [ ] T39
- [ ] O35
- [ ] P35
- [ ] Q35
- [ ] R35
- [ ] S35
- [ ] T35
- [ ] U2
- [ ] V2
- [ ] W2
- [ ] X2
- [ ] Y2

Mandatory build connections:

- Note: J16 uses exactly one SD SPI option set, either A or B.

- [ ] J01: (PB6) P6 -> I24
- [ ] J02: (PA0) Q18 -> L24
- [ ] J03: J24 -> J31
- [ ] J04: K31 -> I38
- [ ] J05: M24 -> M31
- [ ] J06: N31 -> A38
- [ ] J07: (PA1) P18 -> L40
- [ ] J08: (PA2) O18 -> M40
- [ ] J09: (PA3) N18 -> N40
- [ ] J10: (PA5) L18 -> P40
- [ ] J11: (PA6) K18 -> Q40
- [ ] J12: (PA7) J18 -> R40
- [ ] J13: (PB10) F18 -> J40
- [ ] J14: (PB2) G18 -> K40
- [ ] J15: (PA4) M18 -> O36
- [ ] J19: (PB0) I18 -> U1
- [ ] J20: (PB1) H18 -> V1
- [ ] J21: (PB5) O6 -> W1
- [ ] J22: (STAR_GND) Y20 -> K38
- [ ] J23: (STAR_GND) Y20 -> B38
- [ ] J24: (STAR_GND) Y20 -> T40
- [ ] J25: (STAR_GND) Y20 -> T36
- [ ] J26: (STAR_GND) Y20 -> X1

SD SPI option selection (pick exactly one set):

- [ ] Option A: J16A, J17A, J18A
- [ ] Option B: J16B, J17B, J18B

Power and optional jumpers:

- [ ] J27
- [ ] J28
- [ ] J29 (if used)
- [ ] J30 (if used)

Pre-power continuity checks:

- [ ] No short between logic rail and STAR_GND
- [ ] H20-N20 are open
- [ ] No continuity from P36/Q36/R36 to PA5/PA6/PA7 unless the selected SD SPI option is installed
- [ ] J40, K40, O40, S40, T40, O36, S36, and T36 are open unless linked
- [ ] PA0 to L24 continuity (J02)
- [ ] PB6 to I24 continuity (J01)
- [ ] U1 pin 14 to logic rail continuity
- [ ] U1 pin 7 to STAR_GND continuity

## 7. Wire Gauge Guidance

- Signal wiring (PB6, PA0, MIDI signal, LED data): 24-26 AWG.
- Logic branch power and ground: 22-24 AWG.
- LED branch power and return wiring: 18-20 AWG.
- Star-ground trunk links: 18 AWG recommended.

## 8. Current Budget (64 WS2812 total)

Worst-case LED current:

- I_total_max ~= 64 x 0.06A = 3.84A

Estimated branch peaks:

- Rings 1-3 (24 LEDs): about 1.44A
- Rings 4-6 (24 LEDs): about 1.44A
- Rings 7-8 (16 LEDs): about 0.96A

Keep thermal and startup headroom. Target at least 25 percent margin on branch hardware.

## 9. Bring-Up Procedure

1. Mechanical check: verify Black Pill can plug in and USB-C is unobstructed.
2. Continuity check: verify all seven U1 cuts are open.
   - Verify H20-N20 are open (no direct continuity between MCU header rows and U1 rows until jumpers are added).
   - Verify each TFT, touch, and SD signal pad only gains continuity to its MCU pin after its jumper is installed.
   - Verify O40/S40/T40/O36/S36/T36 remain open until their explicit links are installed.
3. No-power resistance check: verify no short between logic +5V and STAR_GND.
4. Apply Buck A only (logic):
   - Confirm +5V at U1 pin 14 and GND at pin 7.
   - Confirm MCU boots and TFT/SD still stable.
   - Confirm TFT initializes and touch input is responsive.
   - Confirm SD mount succeeds repeatedly after reset.
   - Confirm encoder rotation and push events are detected.
5. MIDI test:
   - Send known CC/PC messages.
   - Verify DIN output behavior on target device.
6. LED test (one branch at a time):
   - Power Buck B, then Buck C, then Buck D.
   - Verify stable ring updates and no random flicker.
7. Full-load test:
   - Run bright animation plus MIDI traffic.
   - Check buck temperature and rail droop.

## 10. Notes

- If rings are physically far apart, add extra bulk capacitance near each ring group input.
- If data integrity degrades on long runs, keep the 330R at the source and consider branch-local buffering.
- Keep LED power wiring physically separated from MIDI and data signal runs.

## 11. Condensed Print Card

Bench use only. This is the short-form build card.

### 11.1 Cuts

- [ ] U1 split cuts: H25, I25, J25, K25, L25, M25, N25
- [ ] MCU side-separation cuts: C11 through V11
- [ ] U1-to-MCU isolation cuts: H20, I20, J20, K20, L20, M20, N20
- [ ] Column-38 isolated connector cuts: A37 through K37
- [ ] TFT/touch isolated pads: J39, K39, L39, M39, N39, O39, P39, Q39, R39, S39, T39
- [ ] SD isolated pads: O35, P35, Q35, R35, S35, T35
- [ ] Left-edge encoder isolated pads: U2, V2, W2, X2, Y2

### 11.2 Mandatory Connections

- [ ] J01 J02 J03 J04 J05 J06
- [ ] J07 J08 J09 J10 J11 J12 J13 J14 J15
- [ ] J19 J20 J21
- [ ] J22 J23 J24 J25 J26

### 11.3 SD SPI Choice (pick one)

- [ ] Option A: J16A J17A J18A
- [ ] Option B: J16B J17B J18B

### 11.4 Power Jumpers

- [ ] J27 (TFT power)
- [ ] J28 (SD power)
- [ ] J29 (backlight, if used)
- [ ] J30 (encoder VCC, if used)

### 11.5 Final Continuity Checks

- [ ] No short: logic rail to STAR_GND
- [ ] H20-N20 open
- [ ] PA0 to L24 continuity (J02)
- [ ] PB6 to I24 continuity (J01)
- [ ] U1 pin 14 to logic rail continuity
- [ ] U1 pin 7 to STAR_GND continuity
- [ ] P36/Q36/R36 only connected through the selected SD SPI option
- [ ] J40/K40/O40/S40/T40/O36/S36/T36 only connected through their explicit links

## 12. Single-Page Build Card (Ultra-Compact)

Print this section only.

### 12.1 Cuts

- [ ] H25 I25 J25 K25 L25 M25 N25
- [ ] C11 D11 E11 F11 G11 H11 I11 J11 K11 L11 M11 N11 O11 P11 Q11 R11 S11 T11 U11 V11
- [ ] H20 I20 J20 K20 L20 M20 N20
- [ ] A37 B37 C37 D37 E37 F37 G37 H37 I37 J37 K37 J39 K39 L39 M39 N39 O39 P39 Q39 R39 S39 T39 O35 P35 Q35 R35 S35 T35
- [ ] U2 V2 W2 X2 Y2

### 12.2 Jumpers

- [ ] J01 J02 J03 J04 J05 J06
- [ ] J07 J08 J09 J10 J11 J12 J13 J14 J15
- [ ] J19 J20 J21
- [ ] J22 J23 J24 J25 J26
- [ ] J27 J28
- [ ] J29 (optional)
- [ ] J30 (optional)

### 12.3 SD Option (Choose One)

- [ ] A: J16A J17A J18A
- [ ] B: J16B J17B J18B

### 12.4 Final Checks

- [ ] No short: logic rail <-> STAR_GND
- [ ] H20-N20 open
- [ ] PA0 <-> L24 (J02)
- [ ] PB6 <-> I24 (J01)
- [ ] U1 pin 14 <-> logic rail
- [ ] U1 pin 7 <-> STAR_GND
- [ ] P36/Q36/R36 only connected through selected SD option
- [ ] J40/K40/O40/S40/T40/O36/S36/T36 only connected through explicit links
