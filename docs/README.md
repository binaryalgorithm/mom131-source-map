# Notes and references

Working notes from the decompilation, cleaned up for readers. The source (`src/`) is the final word: these files
explain it, record how it was recovered, and collect facts that are spread over many functions. Each file says when it
was written and how sure it is.

## How the game works

| File | What |
|---|---|
| [`GRAPHICS_ENGINE.txt`](GRAPHICS_ENGINE.txt) | The graphics engine of `WIZARDS.EXE`: Mode X pages, latch copies, the two picture formats (LBX RLE and run-time bitmaps), translucency through remap tables, palette fades and colour cycling, screen transitions (which ones are used, which are dead code), fonts and text layout, the mouse pointer. Read in the code and checked in the assembly; the dead-code claims were also checked with debugger breakpoints. |
| [`SAVE_LAYOUT.txt`](SAVE_LAYOUT.txt) | The `SAVEn.GAM` file, block by block: offset, record size, count, and the structure in `STRUCTS.H` (123,300 bytes, no header). From `Save_Game` / `Load_Game`; the total checked against real saves. |
| [`AUDIO_LABELS.txt`](AUDIO_LABELS.txt) | The developers' own labels for every entry of `MUSIC.LBX` and `SOUNDFX.LBX`, read from the files' name tables. |
| [`LONG_MATH_PORTING.txt`](LONG_MATH_PORTING.txt) | 32-bit arithmetic in a 16-bit program: what `long` means here, the Borland helper calls behind it, and what a port to a modern compiler must keep (16-bit `int` wrap-around, cast placement). |

## Input and screens (`ui/`)

| File | What |
|---|---|
| [`ui/UI_INPUT_MAP.txt`](ui/UI_INPUT_MAP.txt) | Every control of every screen: type, box (x1, y1)-(x2, y2), hotkey, help title. Part A: all 365 `UI_Add_*` calls of both programs read from the source (274 exact boxes, the rest computed at run time - shown as formulas). Part B: 26 screens captured live from the running game's control table. How a click is resolved (lowest index wins, bounds inclusive). |
| [`ui/STATIC_CALLS.txt`](ui/STATIC_CALLS.txt), [`.json`](ui/STATIC_CALLS.json) | Part A as a table and as JSON (file, line, function, control variable, box or formula). |
| [`ui/CROSSCHECK.txt`](ui/CROSSCHECK.txt) | Live versus static, screen by screen: which source function made each screen, which live boxes the static reading reproduces exactly (155 of 287), which are computed, any hotkey differences. |
| [`ui/screens/`](ui/screens) | The 26 live captures as JSON: control table, help regions, pointer regions, and the input route that reached the screen. The screenshots taken with them are not included (game art). |
| [`ui/HARNESS_UI_NOTES.txt`](ui/HARNESS_UI_NOTES.txt) | Driving the game from a script: sending keys and clicks to DOSBox-X, the game's key codes, which screen is up, where the live state is, and per-screen tips (diplomacy menus, end of turn, autosave). |

## Debugging the running game

| File | What |
|---|---|
| [`DOSBOX_X_DEBUG.txt`](DOSBOX_X_DEBUG.txt) | Driving the DOSBox-X debugger from a script over its TCP control channel: protocol, commands, the gotchas, mapping runtime addresses to the map's, breakpoints in VROOMM overlay code (thunk tracking), save states, memory diffs, watchpoints. |

## How the source was matched (`matching/`)

How the decompiled C was brought to compile byte for byte into the original executables. These are the working
notebooks, dated, as written during the work; the tools they name are the project's own and are not published.

| File | What |
|---|---|
| [`matching/MATCH_PLAYBOOK.txt`](matching/MATCH_PLAYBOOK.txt) | The rules learned for Borland C++ 3.1 (`-mm -a -d`): register allocation, stack layout, branch and tail merging, switch tables, prototypes and argument counts, data and literal order, identifier length - each with the batch where it was found. Useful for any Borland C++ 3.x matching project. |
| [`matching/MATCH_CALIBRATION.txt`](matching/MATCH_CALIBRATION.txt) | Module by module: what byte-matching proved about the earlier reading - types, signedness, array shapes, source forms, wrong names and wrong bugs corrected. |
| [`matching/LINK_METHOD.txt`](matching/LINK_METHOD.txt) | From matched modules to identical executables: VROOMM overlay structure, object order recovered from the data segment, initialised and uninitialised data placement, overlay stub order, link date. |
| [`matching/LINK_TEST_MAGIC.txt`](matching/LINK_TEST_MAGIC.txt) | The first identical link (`MAGIC.EXE`, 2026-10-19): what was rebuilt, and how the original was built. |
