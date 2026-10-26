# mom131-source-map

The recovered source code of **Master of Magic v1.31** (MicroProse / Simtex, 1994-95), fully commented, and an
interactive map of it: `WIZARDS.EXE` (the game) and `MAGIC.EXE` (the new-game setup and world generator).

**Open the map:** https://binaryalgorithm.github.io/mom131-source-map/

## The source (`src/`)

This C and assembly source compiles with Borland C++ 3.1 and TASM back into executables **byte-identical** to the
shipped v1.31 `WIZARDS.EXE` and `MAGIC.EXE` (SHA-256 equal to the GOG release). It was decompiled, then matched
function by function, then commented: every function has a header comment saying what it does, every structure
field and global is named and described, and every known bug is marked in the code where it happens.

| Folder | What | Files |
|---|---|---|
| `src/wizards/` | `WIZARDS.EXE`, the game: AI, combat, cities, diplomacy, magic, map, screens, and the engine (graphics, LBX files, UI, sound, memory, DOS I/O) | 131 C + 21 assembly |
| `src/magic/` | `MAGIC.EXE`'s own modules: new-game setup, wizard creation, world generation, credits | 7 C |
| `src/include/` | shared headers: `STRUCTS.H` (every game structure, field by field), `ENUMS.H`, `GAME.H`, declarations | 5 |

`WIZARDS.EXE` is built from 152 modules (one per file in `src/wizards/`). `MAGIC.EXE` is built from 49: its 7 own
files plus 42 shared with the game (heap, LBX loading, graphics, UI, DOS I/O ...) - the same files in
`src/wizards/`, compiled again. `MODULES.TSV` in each folder lists a program's modules in link order with the
address of their code in the original executable. `src/NAMING.TXT` explains how things are named.

**Not here yet:** the build kit (compiler settings, link order, the scripts that reproduce the identical
executables). It follows. Until then the source is for reading.

## The bugs (`BUGS_AND_QUIRKS.txt`)

491 numbered bugs and quirks found while decompiling - what goes wrong, where (function and address), why, and
how sure we are (read in the C, checked in the assembly, or reproduced in the running game). The source comments
cite them as `BUG #n`. Nearly every entry has a regression case that reproduces it in the running game or checks
its code; the latest result is under the entry and in the map. Some entries are marked LATENT: the faulty code is
real but cannot change play.

`CONVENTIONS.txt` describes how the source is written and how sure each statement is. It also refers to the
project's analysis tools and evidence files (`tools/`, `work/`), which are not published.

## The map (`index.html`)

- **One box per source file**, grouped into areas (AI, combat, cities, magic, map, screens, graphics, engine).
  Box area = the file's share of the compiled code.
- **Every function** with its name, address in the original program, size, and its header comment.
- **Every numbered bug** with its status and its latest regression result.
- **Search** by function name, file, description text, or bug number (e.g. `#103`), and a sortable table view.

## Copyright and game files

Master of Magic is © MicroProse / Simtex; the rights now belong to Slitherine, which sells the game (GOG, Steam).
This source is a reconstruction of that program for study and preservation. **This repository holds no game data,
art, sound or executables** - you need your own copy of the game to play it, and later to build anything from the
source. The one piece of the original executables not reconstructed is the Miles sound library (AIL), which is
not part of Simtex's code.

## Credits

- Some names and findings were clarified using Jim Balcomb's ReMoM project (github.com/jbalcomb/ReMoM); those
  places are marked in the source.
- Bug entries name the community patch fixes that address them where one exists (the community patch's file
  names, e.g. W625FIX), and the Master of Magic wiki where it was the source.
