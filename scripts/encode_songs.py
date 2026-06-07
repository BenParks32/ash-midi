import json
import struct
import sys

MSNG_MAGIC = 0x4D534E47
MSNG_VERSION = 2
NONE = 0xFFFF

def add_string(table, s):
    if s is None:
        return NONE
    if s not in table:
        table.append(s)
    return table.index(s)

def pack_u8(v):  return struct.pack("<B", v)
def pack_u16(v): return struct.pack("<H", v)
def pack_u32(v): return struct.pack("<I", v)

def encode(json_data, out_path):

    strings = []
    songs = []
    note_indices = []

    for entry in json_data:
        name_idx = add_string(strings, entry["name"])
        long_idx = add_string(strings, entry.get("longName"))
        id_idx = add_string(strings, entry.get("id"))
        notes = entry.get("notes", [])
        if notes is None:
            notes = []
        if not isinstance(notes, list):
            raise ValueError("notes must be an array of strings")
        if len(notes) > 255:
            raise ValueError("notes array too long (max 255 lines)")
        notes_start = len(note_indices)
        for note in notes:
            if not isinstance(note, str):
                raise ValueError("notes entries must be strings")
            note_indices.append(add_string(strings, note))
        patch = int(entry["patch"])
        if patch < 0 or patch > 255:
            raise ValueError(f"patch out of range (0..255): {patch}")
        songs.append((name_idx, long_idx, id_idx, notes_start, len(notes), patch))

    # Build string table
    offsets = []
    blob = b""
    for s in strings:
        offsets.append(len(blob))
        blob += s.encode("utf-8") + b"\x00"

    string_table = (
        pack_u16(len(strings)) +
        b"".join(pack_u32(o) for o in offsets) +
        blob
    )

    notes_table = b"".join(pack_u16(index) for index in note_indices)

    # Build song table
    song_table = b""
    for name_idx, long_idx, id_idx, notes_start, notes_count, patch in songs:
        song_table += pack_u16(name_idx)
        song_table += pack_u16(long_idx)
        song_table += pack_u16(id_idx)
        song_table += pack_u16(notes_start)
        song_table += pack_u8(notes_count)
        song_table += pack_u8(patch)

    # Compute offsets
    header_size = 4 + 2 + 2 + 4 + 4 + 4
    offset_string = header_size
    offset_songs = offset_string + len(string_table)
    offset_notes = offset_songs + len(song_table)

    header = (
        pack_u32(MSNG_MAGIC) +
        pack_u16(MSNG_VERSION) +
        pack_u16(len(songs)) +
        pack_u32(offset_string) +
        pack_u32(offset_songs) +
        pack_u32(offset_notes)
    )

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(string_table)
        f.write(song_table)
        f.write(notes_table)

    print("Wrote", out_path)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: encode_songs.py songs.json songs.msg")
        sys.exit(1)

    with open(sys.argv[1], "r") as f:
        data = json.load(f)

    encode(data, sys.argv[2])
