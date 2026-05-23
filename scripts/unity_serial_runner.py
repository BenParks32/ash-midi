import re
import sys
import time

import serial
from serial.tools import list_ports


SUMMARY_RE = re.compile(r"Tests\s+\d+\s+Failures\s+\d+\s+Ignored\s+\d+")
TESTCASE_RE = re.compile(r"^[^:\n]+:\d+:[^:\n]+:(PASS|FAIL|IGNORE)")
STM32_USB_VID = 0x0483
STM32_USB_PID = 0x5740


def resolve_port(port_hint: str) -> str | None:
    ports = list(list_ports.comports())
    stm32_ports = [
        port.device
        for port in ports
        if port.vid == STM32_USB_VID and port.pid == STM32_USB_PID
    ]

    if port_hint and port_hint.lower() != "auto":
        if port_hint in stm32_ports or not stm32_ports:
            return port_hint
        if len(stm32_ports) == 1:
            return stm32_ports[0]

    if len(stm32_ports) == 1:
        return stm32_ports[0]
    if port_hint and port_hint.lower() != "auto":
        return port_hint
    return None


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: unity_serial_runner.py <port> <baud> [timeout_seconds]", file=sys.stderr)
        return 2

    port = sys.argv[1]
    baud = int(sys.argv[2])
    timeout_s = float(sys.argv[3]) if len(sys.argv) > 3 else 30.0

    start = time.time()
    ser = None
    while (time.time() - start) < timeout_s:
        resolved_port = resolve_port(port)
        if not resolved_port:
            time.sleep(0.2)
            continue
        try:
            ser = serial.Serial(port=resolved_port, baudrate=baud, timeout=0.2)
            break
        except serial.SerialException:
            time.sleep(0.2)

    if ser is None:
        print(f"ERROR: Timed out waiting for serial port {port}", file=sys.stderr)
        return 1

    try:
        # Some STM32 USB CDC implementations do not support modem-control ioctls
        # (DTR/RTS/ClearCommError), so keep reads simple and avoid control toggles.
        try:
            ser.reset_input_buffer()
        except serial.SerialException:
            pass

        deadline = time.time() + timeout_s
        buffered = ""
        saw_testcase = False
        last_rx = time.time()
        while time.time() < deadline:
            try:
                chunk = ser.read(64)
            except serial.SerialException as exc:
                if saw_testcase:
                    return 0
                print(f"ERROR: Serial read failed: {exc}", file=sys.stderr)
                return 1

            if not chunk:
                if saw_testcase and (time.time() - last_rx) > 1.0:
                    return 0
                continue

            text = chunk.decode("utf-8", "ignore")
            sys.stdout.write(text)
            sys.stdout.flush()
            buffered += text
            last_rx = time.time()

            if "\n" in buffered:
                lines = buffered.split("\n")
                buffered = lines[-1]
                for line in lines[:-1]:
                    if TESTCASE_RE.search(line.strip()):
                        saw_testcase = True
                    if SUMMARY_RE.search(line):
                        return 0

        print("ERROR: Timed out waiting for Unity summary output", file=sys.stderr)
        return 1
    finally:
        ser.close()


if __name__ == "__main__":
    raise SystemExit(main())
