# Copilot instructions for ash-midi

## Project overview

This repository contains firmware for an STM32 Black Pill (`blackpill_f401cc`) using PlatformIO and the Arduino framework. The codebase mixes hardware integration (MIDI, LEDs, display, SD card, touch input) with host-testable business logic.

## Repository layout

- `src\` contains firmware sources.
- `src\Modes\` contains mode-specific behavior.
- `src\Touch\` contains touch-related logic.
- `include\` contains shared headers and board-specific setup such as the TFT configuration.
- `test\host\` contains host-based business-logic tests.
- `test\test_hardware_smoke\` contains the limited MCU-backed smoke suite.
- `scripts\` contains helper scripts used by the PlatformIO test flow.

## Build and test commands

- Build firmware with `pio run -e blackpill_f401cc`.
- Run host business-logic tests with `pio test -e native`.
- Run hardware smoke tests with `pio test -e blackpill_f401cc`.

## Working conventions

- Prefer adding or updating tests in `test\host\` for business logic.
- Keep MCU-backed tests only for seams that genuinely require real hardware integration.
- Do not duplicate business-logic coverage in hardware tests.
- Reuse existing PlatformIO environments and project helpers instead of introducing new ad hoc build or test flows.
- Before modifying any pin assignments, timing constants, or peripheral configurations, read `HARDWARE.md` and confirm the change does not violate the pin mapping or power constraints listed there.
- In user discussions, button numbers are 1-based: button 1 is the first button and button 8 is the last, even if the code uses 0-based indices `0..7`.

## Change guidance

- Limit each change to the smallest set of functions and files necessary to satisfy the task. Do not refactor, rename, or restructure code that is not directly required by the task.
- When touching firmware logic, check whether a host-testable path exists before changing hardware-specific code. If a host-testable path exists, implement the logic there and keep the hardware layer as a thin adapter. If no host-testable path exists, document why in a comment and proceed with the hardware-specific change.
- Follow the existing structure and naming patterns in `src\`, especially under `Modes` and `Touch`.
