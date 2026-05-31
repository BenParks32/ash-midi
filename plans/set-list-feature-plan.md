# Set List Feature Plan

## Summary

Add live-performance set lists as an optional layer on top of the existing per-band song catalogs. A set list is selected after choosing a band and only affects the new **Sets** flow. Each set contains numbered songs and named, numbered parts so the display can present the set in the intended running order. When no set list is selected, existing **Play** and **Patch** behavior stays unchanged.

Song notes are **out of scope** for this feature and should be handled by a separate future feature.

## Confirmed Product Decisions

- Set lists are stored on the SD card as JSON files, organised per band/playlist.
- The firmware enumerates available set lists for the current band/playlist.
- Set lists reference songs by stable song IDs.
- Each song in the existing per-band song catalog gets a required string `id`.
- The selected set list remains active only while the device is powered on.
- Startup behavior does not change; the device still boots to Home mode.
- If no set list is selected, **Play** and **Patch** work exactly as they do now.
- The current **Songs** entry point is replaced by **Sets** on button 6.
- **Sets** mode shows the active set list and its songs.
- Set songs are numbered and presented sorted by song number.
- Set lists contain an array of parts; each part has a display name and a number.
- Parts are presented in part-number order and use the configured part name on the display.
- **Sets** mode includes a control to enter **Set Selection** mode.
- **Set Selection** mode lists the set lists found on the SD card for the current band.
- V1 navigation within a set list happens only in **Sets** mode, not in **Play** mode.
- If a set list references a missing song, that song still appears, is selectable, and is highlighted in red.
- Selecting an unresolved set song enters **Play** mode, marks the song unavailable, and does not change the current patch.

## Recommended Data Shape

### Song catalog

Add a required `id` to each song in the existing band song catalogs:

```json
{
  "id": "intro-clean",
  "name": "Intro Clean",
  "patch": 12
}
```

### Set list

Store set lists separately from the song catalog, reference songs by ID, and define ordered set parts explicitly:

```json
{
  "name": "Friday Night Set",
  "parts": [
    { "number": 1, "name": "First half" },
    { "number": 2, "name": "Second half" }
  ],
  "songs": [
    { "number": 10, "part": 1, "songId": "intro-clean" },
    { "number": 20, "part": 1, "songId": "big-chorus" },
    { "number": 30, "part": 2, "songId": "missing-song-id" }
  ]
}
```

Recommended behavior:

- Sort parts by `number`.
- Sort songs by song `number`.
- Use part `name` for display headings.
- Resolve each set song against the current band's canonical song catalog.
- Keep unresolved songs in the runtime list with enough metadata to render and select them safely.

## Staged Implementation Plan

### Stage 1: Extend song identity and set-list storage

- Add required `id` support to the existing song parsing path.
- Add host-testable set-list parsing and enumeration logic.
- Define the runtime model for:
  - set list summaries for selection
  - active set state
  - ordered set parts
  - resolved set songs
  - unresolved set songs

### Stage 2: Add set-list loading and runtime selection state

- Add a service/store layer that:
  - lists set lists for the current band
  - loads a selected set list
  - sorts parts and songs by their number fields
  - resolves songs against the song catalog
  - exposes the active in-memory set list
- Keep this logic separate from hardware-specific UI code.

### Stage 3: Replace Songs with Sets

- Rework the current Songs entry point into **Sets**.
- Show the active set name, its named parts, and its ordered songs.
- Reuse the current Songs-mode interaction patterns where practical.
- Render unresolved songs in red.

### Stage 4: Add Set Selection mode

- Add a dedicated **Set Selection** mode modeled after the current song list UI.
- List available SD-card set lists for the current band.
- Selecting a set list activates it and returns to **Sets**.

### Stage 5: Update Play-mode handoff

- Allow **Sets** selection to hand off either:
  - a resolved canonical song, or
  - an unresolved set song
- For resolved songs, keep current patch/song behavior.
- For unresolved songs, enter **Play** without changing patch and show the song as unavailable.

### Stage 6: Final polish and failure handling

- Define empty-state behavior for bands with no set lists.
- Ensure switching bands does not leak active set state across playlists.
- Confirm the UI stays fast and clear for live use.

## Likely Files and Components to Change

- `src\ButtonOverrideStore.cpp`
- `include\ButtonOverrideStore.h`
- `src\Modes\SongsMode.cpp` (likely repurposed into Sets behavior)
- `src\Modes\PlayMode.cpp`
- `src\Modes\` new mode for set selection
- SD-card/resource helpers involved in file enumeration and loading
- Host tests under `test\host\`
- Sample config files such as `p7songs.jsn`, `oprsongs.jsn`, and `crsongs.jsn`

## Host-Test Focus

- Song catalog parsing with required `id`
- Set-list file enumeration per band
- Set-list parsing and validation
- Sorting of parts by part number
- Sorting of songs by song number
- Resolution of set songs against canonical songs
- Preservation of unresolved songs
- Active set runtime state
- Band switching behavior
- Sets-mode selection behavior
- Play-mode behavior for resolved vs unresolved songs

## Risks and Edge Cases

- Missing or duplicate song IDs in the canonical catalog
- Duplicate or missing part numbers
- Duplicate or missing song numbers
- Set lists that contain only unresolved songs
- Large set lists affecting list rendering or navigation speed
- Band switching while a set is active
- Returning from Play mode to the correct Sets context
- SD-card errors while enumerating or loading set lists

## Remaining Essential Open Issues

None for the v1 set list feature plan.

The main deferred topic is **song notes**, which should be planned separately so it does not complicate the core set-list flow.
