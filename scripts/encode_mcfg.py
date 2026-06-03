import json
import struct
import sys

MCFG_MAGIC = 0x4D434647
MCFG_STRING_INDEX_NONE = 0xFFFF

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

def add_string(table, s):
    """Add string to table if not present, return index."""
    if s is None:
        return MCFG_STRING_INDEX_NONE
    if s not in table:
        table.append(s)
    return table.index(s)

def pack_u8(v):  return struct.pack("<B", v)
def pack_u16(v): return struct.pack("<H", v)
def pack_u32(v): return struct.pack("<I", v)

# ------------------------------------------------------------
# Main encoder
# ------------------------------------------------------------

def encode(json_data, out_path):

    play_modes = json_data["playModes"]

    # First pass: collect all strings
    strings = []

    play_mode_records = []
    patch_records = []
    button_records = []
    function_records = []

    # Temporary storage for hierarchical relationships
    play_mode_info = []

    for pm_name, pm_data in play_modes.items():
        pm_name_idx = add_string(strings, pm_name)

        patches = pm_data.get("patches", {})
        patch_list = []

        for patch_num_str, patch_data in patches.items():
            patch_num = int(patch_num_str)

            name_idx = add_string(strings, patch_data.get("name"))
            long_name_idx = add_string(strings, patch_data.get("longName"))

            buttons = patch_data.get("buttons", {})
            button_list = []

            for btn_id_str, btn_data in buttons.items():
                btn_id = int(btn_id_str)

                label_idx = add_string(strings, btn_data.get("label"))
                colour_idx = add_string(strings, btn_data.get("colour"))

                toggle = 1 if btn_data.get("toggle") else 0

                funcs = btn_data.get("function", {})
                func_list = []

                # Functions may be nested or simple
                for event_name, event_data in funcs.items():
                    if isinstance(event_data, str):
                        # e.g. "shortPress": "Tap"
                        action_name_idx = add_string(strings, event_data)
                        action_type = 0  # TAP
                        value = 0
                        secondary = 0
                    else:
                        # e.g. "shortPress": { action, value, secondaryValue }
                        action_name_idx = add_string(strings, event_data.get("action"))
                        action_type = 1  # SEND_MIDI_CC
                        value = int(event_data.get("value", 0))
                        secondary = int(event_data.get("secondaryValue", 0))

                    # Map event name to enum
                    event_map = {
                        "shortPress": 0,
                        "press": 1,
                        "release": 2
                    }
                    event_type = event_map[event_name]

                    func_list.append((event_type, action_type, action_name_idx, value, secondary))

                button_list.append((btn_id, label_idx, colour_idx, toggle, func_list))

            patch_list.append((patch_num, name_idx, long_name_idx, button_list))

        play_mode_info.append((pm_name_idx, patch_list))

    # ------------------------------------------------------------
    # Build binary sections
    # ------------------------------------------------------------

    # String table
    string_offsets = []
    string_blob = b""

    for s in strings:
        string_offsets.append(len(string_blob))
        string_blob += s.encode("utf-8") + b"\x00"

    string_table_header = pack_u16(len(strings))
    string_table_offsets = b"".join(pack_u32(o) for o in string_offsets)
    string_table_section = string_table_header + string_table_offsets + string_blob

    # PlayMode, Patch, Button, Function sections
    play_mode_section = b""
    patch_section = b""
    button_section = b""
    function_section = b""

    # Offsets (filled later)
    patch_offsets = []
    button_offsets = []
    function_offsets = []

    # Build play modes
    for pm_name_idx, patch_list in play_mode_info:
        pm_patch_offset = 0  # placeholder, fix later
        play_mode_section += pack_u16(pm_name_idx)
        play_mode_section += pack_u16(len(patch_list))
        play_mode_section += pack_u32(0)  # patchOffset placeholder
        patch_offsets.append(len(patch_section))

        # Build patches
        for patch_num, name_idx, long_name_idx, button_list in patch_list:
            patch_button_offset = 0  # placeholder
            patch_section += pack_u16(name_idx)
            patch_section += pack_u16(long_name_idx)
            patch_section += pack_u8(patch_num)
            patch_section += pack_u8(len(button_list))
            patch_section += pack_u32(0)  # buttonOffset placeholder
            button_offsets.append(len(button_section))

            # Build buttons
            for btn_id, label_idx, colour_idx, toggle, func_list in button_list:
                button_section += pack_u8(btn_id)
                button_section += pack_u16(label_idx)
                button_section += pack_u16(colour_idx)
                button_section += pack_u8(toggle)
                button_section += pack_u8(len(func_list))
                button_section += pack_u32(0)  # functionOffset placeholder
                function_offsets.append(len(function_section))

                # Build functions
                for (event_type, action_type, action_name_idx, value, secondary) in func_list:
                    function_section += pack_u8(event_type)
                    function_section += pack_u8(action_type)
                    function_section += pack_u16(action_name_idx)
                    function_section += pack_u16(value)
                    function_section += pack_u16(secondary)

    # ------------------------------------------------------------
    # Compute final offsets
    # ------------------------------------------------------------

    # Layout:
    # [Header]
    # [StringTable]
    # [PlayModes]
    # [Patches]
    # [Buttons]
    # [Functions]

    offset_header = 0
    offset_string = offset_header + struct.calcsize("<IHHII")  # header is 16 bytes
    offset_playmode = offset_string + len(string_table_section)
    offset_patch = offset_playmode + len(play_mode_section)
    offset_button = offset_patch + len(patch_section)
    offset_function = offset_button + len(button_section)

    # Fix playMode.patchOffset
    pm_cursor = 0
    for i, (pm_name_idx, patch_list) in enumerate(play_mode_info):
        # skip nameIndex + patchCount
        pm_cursor += 4
        # write patchOffset
        pm_patch_offset = offset_patch + patch_offsets[i]
        play_mode_section = (play_mode_section[:pm_cursor] +
                             pack_u32(pm_patch_offset) +
                             play_mode_section[pm_cursor+4:])
        pm_cursor += 4

    # Fix patch.buttonOffset
    patch_cursor = 0
    for i, (_, patch_list) in enumerate(play_mode_info):
        for j, _ in enumerate(patch_list):
            # skip nameIndex + longNameIndex + patchNum + buttonCount
            patch_cursor += 6
            # write buttonOffset
            patch_button_offset = offset_button + button_offsets.pop(0)
            patch_section = (patch_section[:patch_cursor] +
                             pack_u32(patch_button_offset) +
                             patch_section[patch_cursor+4:])
            patch_cursor += 4

    # Fix button.functionOffset
    button_cursor = 0
    for i, (pm_name_idx, patch_list) in enumerate(play_mode_info):
        for patch in patch_list:
            for button in patch[3]:
                # skip buttonId + labelIndex + colourIndex + toggle + functionCount
                button_cursor += 7
                # write functionOffset
                button_function_offset = offset_function + function_offsets.pop(0)
                button_section = (button_section[:button_cursor] +
                                  pack_u32(button_function_offset) +
                                  button_section[button_cursor+4:])
                button_cursor += 4

    # ------------------------------------------------------------
    # Write final file
    # ------------------------------------------------------------

    header = (
        pack_u32(MCFG_MAGIC) +
        pack_u16(1) +  # version
        pack_u16(len(play_mode_info)) +
        pack_u32(offset_string) +
        pack_u32(offset_playmode)
    )

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(string_table_section)
        f.write(play_mode_section)
        f.write(patch_section)
        f.write(button_section)
        f.write(function_section)

    print("Wrote", out_path)


# ------------------------------------------------------------
# CLI
# ------------------------------------------------------------

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: encode_mcfg.py input.json output.mcfg")
        sys.exit(1)

    with open(sys.argv[1], "r") as f:
        data = json.load(f)

    encode(data, sys.argv[2])
