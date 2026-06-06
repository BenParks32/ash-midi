# Play set mode feature plan

## Summary

When a set list has been activated the controller will enter a new version of play mode called "Play set" mode. The purpose of this mode is make the performance of a set of songs as friction free as possible. In this mode I need to be able to:

- See the how many songs are in this part of the set.
- The name of song that is currently active.
- See the upcomming songs that follow the current song.
- Access the scenes of the patch selected on the QC for the song.
- Access the tuner and QC gig mode function.
- Move to the next song in the set.
- See any performance notes e.g.
  - Drop D tuning
  - E Flat tuning
  - Capo 3
  - Chord reminders
  - Flanger on
- Ideally, the settings within song should make any configuration changes on QC, e.g.
  - Turn on E Flat tuning
  - Turn on other effects blocks on the QC

  The design should not be so rigid that I can not deviate from the set. Often in performances, a different song will be called which is not planned. In that case I need to access the default/main patch (or the appropriate patch from the patches mode) and manually setup patch as required for song. It's important this fallback mode can executed quickly so that performance is not held up. Once the unplanned song has been finished I need to be able to resume the setlist.

## Implementation ideas

### Button functions

The main functions of play set mode need to stay performance focus. The priority functions are:

- Scene selection, as normal, with buttons 1, 2 and 3.
- Extra button functions configured through buttons.mcf
- Next song
- Patch selection (to allow quick set changes)

Secondary functions will be

- Tuner/gig view access
- Set list mode
- Load set
- Next song/previous song in set
- Exit play set mode and return to normal play mode
- Activate the next part of the set

For the secondary functions a manage set mode is needed. This will provide functions for managing the active set.

#### Button layout for play set mode

Porposed button layout for play set mode:

| Button# | Function      | Description                                                   |
| :------ | :------------ | :------------------------------------------------------------ |
| 1       | cfg           | Available for config (or default behaviour)                   |
| 2       | cfg           | Available for config (or default behaviour)                   |
| 3       | cfg           | Available for config (or default behaviour)                   |
| 4       | cfg           | Available for config (or default behaviour)                   |
| 5       | Patches/Patch | Opens patches mode (short press), Patch mode (long press)     |
| 6       | Next          | Next song in the set                                          |
| 7       | cfg           | Available for config                                          |
| 8       | Set/Tuner     | Short press opens set management mode, long press opens tuner |

```text
| 5     | 6    | 7   | 8   |
| Patch | Song | cfg | Set |
<----------UI-------------->
| cfg  | cfg   | cfg | cfg |
| 1    | 2     | 3   | 4   |
```

#### Button layout for manage set mode

Porposed button layout for manage set mode:

| Button# | Function  | Description                                               |
| :------ | :-------- | :---------------------------------------------------------|
| 1       | Next      | Next song in the set                                      |
| 2       | Select    | Activate play mode with the highlighted song selected     |
| 3       | Unused    |                                                           |
| 4       | Play      | Return to page 1 and Flashes while in set management mode |
| 5       | Prev      | Previous song in the list                                 |
| 6       | Part      | Rotates through the parts of a set                        |
| 7       | Load      | Opens the set selection mode                              |
| 8       | Exit      | Exits play set mode and returns to normal play mode       |

```text
| 5    | 6        | 7    | 8    |
| Prev | Part #/n | Load | Exit |
<---------------UI-------------->
| Next | Select   |      | Play |
| 1    | 2        | 3    | 4    |
```

### Changes to play mode

Currently, button 6 is used to enter set selection mode. Instead this will now change to enter play set mode. From here the user can enter the set management mode and from there they can enter the set selection mode. This is a more intuitive flow as the user is already in a set focused mode when they want to select a set.

## Play set mode centre section UI layout

This needs to display:

- Selected patch name
- Current song name
- Current song number out of the total song count for that part of the set.
- Upcoming songs.
- Any performance notes.

*The song name and performance notes should be in red if the song is not resolved against the song catalog.*

```text
-----------------------------------------------------------
| S: <Song name (#/n)>                     | Upcoming     |
| P: <Patch name>                          | <Part name>  |
-------------------------------------------| # Song       |
|  Performance notes for the song          | # Song       |
-------------------------------------------| # Song       |
| Active scene name                        | # Song       |
-----------------------------------------------------------
```

The upcoming section will not have a title, instead it will just show the name of the part and the songs that follow the current song in that part. The end of the part will be marked by a solid line.

Pressing the next song will advance the current song and update the upcoming songs. When the end of the part is reached, the next song will be the first song of the next part. When the end of the set is reached, the next song will be the first song of the first part.

**Safety default:** end-of-set wrap is disabled by default for live use. At end of set, pressing `Next` shows an `End of set` state and requires a second press within 2s to wrap to part 1/song 1. This should be configurable later if needed.

## Manage set mode centre section UI layout

This needs to display:

- All the songs in the set with the current song highlighted.
- Pressing prev/next will move the highlight up and down the list and select will select the song. The rotary encoder can optionally be used to also scroll the list and select a song.
- Each song entry will show the song number, song name and P# which represent the part number the song is in. If the song is not resolved against the song catalog, it will be shown in red and the song name will be the name from the set list file.
- The list will be sorted by part number and then song number.
- The list will scroll as the user moves the highlight up and down. The current song will always be visible and ideally the next song will also be visible.

Pressing the part button will move the selection to the start of next part, e.g. for a three part set p1 > p2 > Encore > p1. The soft touch button will "Prt #/n", indicating the currtent part and the total number of parts in the set.

## Implementation readiness decisions (proposed defaults)

### Mode and transition model

`Play Set` is a sub-state of `Play` mode, not a top-level mode. `Manage Set` is a temporary control surface entered from `Play Set`. Returning from `Manage Set` always returns to `Play Set` unless `Exit` is used.

| From state     | Trigger                     | To state           | Notes |
| :------------- | :-------------------------- | :----------------- | :---- |
| Play           | Button 6 short              | Play Set           | Enters set-driven playback surface |
| Play Set       | Button 8 short              | Manage Set         | Set management overlay |
| Play Set       | Button 5 short              | Patches            | Quick fallback for unplanned song |
| Play Set       | Button 5 long               | Patch              | Quick fallback for unplanned song |
| Play Set       | Button 6 short              | Play Set           | Advance to next song |
| Manage Set     | Button 2 (Select)           | Play Set           | Activate highlighted song |
| Manage Set     | Button 8 (Exit)             | Play               | Leave set-driven flow completely |
| Manage Set     | Button 4 (Play)             | Play Set           | Return to primary set surface |
| Any set state  | Set load failure/unresolved | Stay in current    | Non-blocking warning, no forced mode jump |

### Button precedence and input rules

1. Fixed live controls have highest priority: `Button 5` (Patch/Patches), `Button 6` (Next), `Button 8` (Set/Tuner).
2. Scene behavior remains default for buttons 1-3 unless explicitly overridden by play-set-aware config.
3. `buttons.mcf` can customize only buttons marked `cfg`; it cannot replace fixed controls in Play Set.
4. Long-press threshold is 450ms with release-to-activate semantics for all long-press actions.
5. Repeated `Next` presses are debounced for 200ms to prevent accidental double-advance.

### Set/song data and compatibility

Add optional song-level fields to set content:

- `notes`: ordered list of short strings for performance hints.
- `actions`: ordered list of optional QC operations to apply when song is activated.

Compatibility defaults:

- If `notes` is missing, use empty list.
- If `actions` is missing, use empty list.
- If song cannot be resolved in catalog, keep set-provided song name, mark unresolved in red, and allow manual fallback.
- Unknown fields are ignored; malformed `notes/actions` entries are skipped with warning and do not abort set loading.

## Failure and fallback handling

| Failure case | Runtime behavior | UI behavior |
| :-- | :-- | :-- |
| SD load/read failure for active set | Keep current patch and current mode; do not clear live state | Red status line: `Set load failed` |
| Active song missing from catalog | Keep set cursor and allow `Next`/`Prev` | Song title + notes in red with `unresolved` marker |
| Next at end-of-set | Do not wrap on first press | `End of set` prompt; second press within 2s wraps |
| Fallback to manual patch for unplanned song | Preserve set cursor + selected song index | `Manual override` indicator; `Resume set` returns to preserved index |

## Performance and responsiveness constraints

- `Next` must update visible song/notes state immediately; QC actions run asynchronously after UI state change.
- No blocking waits in button handlers; queue external actions and process in main loop tick.
- If multiple actions are attached to a song, dispatch them in deterministic order as fire-and-forget MIDI sends.

## Test strategy (host-first)

Primary coverage in `test\host\`:

1. State transitions across Play, Play Set, and Manage Set.
2. Song advance behavior across part boundaries and end-of-set double-press wrap.
3. Manual override + resume-set cursor preservation.
4. Unresolved song rendering flags and non-blocking behavior.
5. Parser compatibility for missing/invalid `notes/actions` fields.
6. Button precedence rules (`cfg` vs fixed live controls).
7. Non-blocking action dispatch ordering for multi-action songs.

Hardware smoke coverage only for seams that require device integration:

- Button press timing for short vs long press on target hardware.
- Basic display update and MIDI/QC command dispatch plumbing.

## Observability and diagnostics

Add lightweight counters/log events:

- Mode transitions (`Play` <-> `Play Set` <-> `Manage Set`).
- Song index changes (`part`, `song`, reason = next/select/resume).
- Action dispatch count per song activation.
- Set parsing warnings (invalid notes/actions entry count).

These are for troubleshooting and should not interrupt live behavior.

## Confirmed decisions

1. Rotary encoder selection in `Manage Set` is enabled by default.
2. End-of-set wrap remains a fixed safe default in this phase; `Menu` configurability is deferred to a later phase.
