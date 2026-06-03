import json
import struct
import sys

MSLT_MAGIC = 0x4D534C54
MSLT_VERSION = 1
NONE = 0xFFFF


def add_string(table, value):
    if value is None:
        return NONE
    if value not in table:
        table.append(value)
    return table.index(value)


def pack_u16(value):
    return struct.pack("<H", value)


def pack_u32(value):
    return struct.pack("<I", value)


def encode(json_data, out_path):
    strings = []
    parts = []
    songs = []

    name_index = add_string(strings, json_data["name"])

    for entry in json_data.get("parts", []):
        parts.append((int(entry["part"]), add_string(strings, entry["name"])))

    for entry in json_data.get("songs", []):
        songs.append((int(entry["number"]), int(entry["part"]), add_string(strings, entry["songId"])))

    blob = b""
    offsets = []
    for value in strings:
        offsets.append(len(blob))
        blob += value.encode("utf-8") + b"\x00"

    string_table = pack_u16(len(strings))
    string_table += b"".join(pack_u32(offset) for offset in offsets)
    string_table += blob

    part_table = b"".join(pack_u16(part) + pack_u16(name_idx) for part, name_idx in parts)
    song_table = b"".join(pack_u16(number) + pack_u16(part) + pack_u16(song_id_idx) for number, part, song_id_idx in songs)

    header_size = struct.calcsize("<IHHHHIII")
    string_table_offset = header_size
    part_table_offset = string_table_offset + len(string_table)
    song_table_offset = part_table_offset + len(part_table)

    header = (
        pack_u32(MSLT_MAGIC)
        + pack_u16(MSLT_VERSION)
        + pack_u16(len(parts))
        + pack_u16(len(songs))
        + pack_u16(name_index)
        + pack_u32(string_table_offset)
        + pack_u32(part_table_offset)
        + pack_u32(song_table_offset)
    )

    with open(out_path, "wb") as handle:
        handle.write(header)
        handle.write(string_table)
        handle.write(part_table)
        handle.write(song_table)

    print("Wrote", out_path)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: encode_setlists.py setlist.json setlist.msl")
        sys.exit(1)

    with open(sys.argv[1], "r", encoding="utf-8") as handle:
        data = json.load(handle)

    encode(data, sys.argv[2])
