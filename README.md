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
`src/wizards/`, compiled again. `MODULES.TSV` in each folder maps each module to its code segment in the original
executable (the link order is in `build/`). `src/NAMING.TXT` explains how things are named.

## Building the identical executables

The steps below rebuild `WIZARDS.EXE` and `MAGIC.EXE` byte for byte. They are written so that a person - or an AI
assistant working for one - can follow them by hand; `build/` also has scripts that do all of it.

**Quick build** (Python 3, 7-Zip on the PATH, DOSBox-X; tested on Windows):

```
python build/install_bc31.py "path/to/Borland CPP 3.1 and Application Frameworks (1992) (3.5-1.44mb)" tools
python build/build.py --bc tools --game "path/to/your/Master of Magic" --dosbox path/to/dosbox-x
```

The first installs the needed parts of Borland C++ 3.1 into `tools/BORLANDC` and checks them; the second makes the
sound-library stand-ins from your game, compiles all 203 modules in silent DOSBox-X sessions (about a minute),
links, and prints both executables' SHA-256 against the originals. The steps it performs:

### What you need

| What | Where | Check |
|---|---|---|
| **Master of Magic v1.31** - your own copy | GOG ("Master of Magic Classic"); the GOG files are the ones verified | `WIZARDS.EXE` SHA-256 `83e273105b49ebb758b041c965298e45d491513e448e84ede2c6152232ad4e3d`, `MAGIC.EXE` `e9b96da031097fee92d8f00213f7406d5d22003c63e486b7f6394eaa078c3ebb` |
| **Borland C++ 3.1** | WinWorld (winworldpc.com): "Borland C++ 3.1 and Application Frameworks (1992)", the 3.5" 1.44 MB disk set - `Borland CPP 3.1 and Application Frameworks (1992) (3.5-1.44mb).7z` | archive SHA-256 `76e155a7fb7eb0590930ecd513644c977497a020466a272943776fbf683d0f27` |
| **DOSBox-X** (Borland C++ is a 16-bit DOS program) | dosbox-x.com / github.com/joncampbell123/dosbox-x - verified with 2026.10.01 | |
| **Python 3** | python.org - for the scripts in `build/` | |
| **7-Zip** | 7-zip.org (or `p7zip` / Info-ZIP `unzip` elsewhere) - opens the floppy images and the 1992 PKZIP 1.x archives, which Python's own `zipfile` cannot read | |

Borland C++ 3.1 is not freeware (the rights are Embarcadero's); WinWorld hosts it as abandonware. It is not
included here - get it yourself, and judge for yourself. Only 3.1 gives identical code (3.0 does not).

### 1. Install the compiler (only the parts the build uses)

(`build/install_bc31.py` does this step.) Unpack the WinWorld archive: it holds the original install disks as
floppy images, `Disk01.img` .. `Disk15.img`. Open each image with 7-Zip into a folder of its own (`Disk01/` ...).
Make a folder `BORLANDC` with `BIN`, `INCLUDE` and `LIB` in it (unzip everything with 7-Zip: the archives are
PKZIP 1.x):

- `BIN`: the command-line compiler and linker are in the multi-disk archive `CMDLINE` (`Disk03/CMDLINE.CA1` +
  `Disk13/CMDLINE.CA2`), the assembler in `TASM` (`Disk07/TASM.CA1` + `Disk09/TASM.CA2`). Each `.CA` part is a
  piece of a PKZIP file behind a 5-byte header: drop the first 5 bytes of every part, join the parts in order, and
  unzip the result into `BIN`. Also unzip `Disk09/BIN.ZIP` into `BIN` (the DPMI files the compiler runs on).
- `INCLUDE`: unzip `Disk01/INCLUDE.ZIP`.
- `LIB`: unzip `Disk08/MLIB.ZIP` (`C0M.OBJ`, `CM.LIB` - the medium memory model) and `Disk07/XLIB.ZIP`
  (`OVERLAY.LIB`).

Check the six files that decide the output:

```
87c6dfbad817eeca31c069043f08063f7122663e51932ad089e6d1c563450d3b  BIN/BCC.EXE
ab8b8b96c1bde5ff6f56c8be2c3b928d21ead7430e72b01e3b22f1fa179ad006  BIN/TLINK.EXE
80534cd6a1e2c2b4376ae3f21954c6dbfa77a6998d5e0bf8b8bee6153f3174ea  BIN/TASM.EXE
54e9630201745b88a0f2d22c3ba18743b54f351974b7bd67f2a0902ed1f7b480  LIB/C0M.OBJ
8b658567c42c70e09543bd181ce4fff698d59d024b13e9e593c7003e08508e80  LIB/CM.LIB
3504bd256c5564c52d73ea3b4b08ddbe113a016d7fcfbf2f186dcd83f7fc9b24  LIB/OVERLAY.LIB
```

In DOSBox-X the folder that holds `BORLANDC` is mounted as `C:` (so the compiler is `C:\BORLANDC\BIN\BCC.EXE`) and
the build folder as another drive. The verified setup: `memsize=16`, `cycles=max`, `core=normal`, `xms=true`,
`ems=false`, no sound; `PATH=C:\BORLANDC\BIN`.

### 2. Make the sound-library stand-ins from your game

Both programs link the Miles Design AIL 2.14 sound library, which is not Simtex code and not in this repository.
The build links a stand-in made from your own executables:

```
python build/make_ail.py "path/to/your/Master of Magic" OUT
```

writes `OUT/S2FC1.ASM` (for `WIZARDS.EXE`) and `OUT/AIL.ASM` (for `MAGIC.EXE`). It checks your files' SHA-256 first.

### 3. Compile - one fresh DOSBox-X session per module

Use one build folder per program: `W` for `WIZARDS.EXE`, `L` for `MAGIC.EXE`. Copy into it the five headers of
`src/include/` and the program's modules (`W`: every file of `src/wizards/`; `L`: the files listed in
`src/magic/MODULES.TSV` - its own from `src/magic/`, the shared ones from `src/wizards/`), plus the stand-in
(`S2FC1.ASM` into `W`, `AIL.ASM` into `L`). Then compile every module:

```
BCC -c -mm -a -d -IC:\BORLANDC\INCLUDE NAME.C        resident modules
BCC -Y -c -mm -a -d -IC:\BORLANDC\INCLUDE NAME.C     overlay modules
TASM /ml NAME.ASM                                     assembly modules and the stand-in
```

A C module is an **overlay module** (`-Y`) when its object file is listed between `/o` and `/o-` in that program's
response file (`build/WIZARDS.RSP`, `build/MAGIC.RSP`); the rest are resident. Assembly modules always take plain
`TASM /ml`, also the one overlay assembly module (`PATHRLX.ASM`). A shared file can be an overlay in one
program and resident in the other - that is why each program has its own folder.

**Important: start a NEW DOSBox-X session for every compile** (a batch file that runs one command, then exits).
Borland C++ 3.1 reads leftovers of the previous run from memory: compiled one after another in one session, a few
large modules (`WIZARDS.C`, `MAGIC.C`, `HEAP.C`) come out with different register choices and the executables no
longer match. Sessions can run in parallel; a full build takes about a minute.

**Running DOSBox-X headless:** start each session as
`dosbox-x -silent -fastlaunch -nogui -conf SESSION.CONF`, with the compile in the config's `[autoexec]` section
followed by `exit`. `-silent` runs without a window and exits when the autoexec is done, so a script can start 200
sessions without anything appearing on screen and simply wait for each process to end. `build/build.py` does exactly
this - read it as the worked example (the config it writes, the batch per module, the parallel runner).

After assembling the WIZARDS stand-in, put its relocation where the original link had it:

```
python build/fix_ail_obj.py W/S2FC1.OBJ
```

### 4. Link

In each folder, with the DOS date set to the original link date - the overlay manager's `_EXEINFO_` record stores
the link date in the executable - link with the response file:

```
DATE /F                     (DOSBox-X: use its own clock, so the date can be set)
DATE 03/03/1995
TLINK @WIZARDS.RSP          (in W; MAGIC.RSP in L)
```

`build/WIZARDS.RSP` / `build/MAGIC.RSP` are the exact response files of the verified link: the startup code, the
main module, the overlays in stub order between `/o` and `/o-`, the resident modules in segment order, the map
file, `CM.LIB` and `OVERLAY.LIB`. The link order decides the layout - do not reorder.

### 5. Check

The new `WIZARDS.EXE` and `MAGIC.EXE` must have the SHA-256 of the originals given above. If a hash differs, find the
first differing byte: one module compiled in a shared session or with the wrong `-Y` is the usual cause.

### Which game files the build needs - and playing the result

The build reads only two files of your game: `WIZARDS.EXE` and `MAGIC.EXE` (the sound-library stand-ins are taken
from them, and they are the reference for the final check). `make_ail.py` stops with a message if either is missing
or is not the v1.31 file. The data files (`*.LBX`, `CONFIG.MOM`, the sound drivers ...) are not needed to build.

To play the rebuilt executables, copy your whole game folder and put the new `WIZARDS.EXE` / `MAGIC.EXE` over the
old ones in the copy - the programs load all their art, sound and rules from the `.LBX` files beside them. Start it as
you would the original (in DOSBox / DOSBox-X: `MAGIC.EXE` for a new game, which then runs `WIZARDS.EXE`).

## The bugs (`BUGS_AND_QUIRKS.txt`)

491 numbered bugs and quirks found while decompiling - what goes wrong, where (function and address), why, and
how sure we are (read in the C, checked in the assembly, or reproduced in the running game). The source comments
cite them as `BUG #n`. Nearly every entry has a regression case that reproduces it in the running game or checks
its code; the latest result is under the entry and in the map. Some entries are marked LATENT: the faulty code is
real but cannot change play.

`CONVENTIONS.txt` describes how the source is written and how sure each statement is. It also refers to the
project's analysis tools and evidence files (`tools/`, `work/`), which are not published - apart from the
regression harness in `tools/harness/` and its runners.

## The regression suite (`tools/harness/`)

Every numbered entry of `BUGS_AND_QUIRKS.txt` has a case that reproduces it - in the running game (a script drives
the real v1.31 in DOSBox-X on a hidden Windows desktop: it edits a save, presses the game's own keys, reads the
emulated memory, sets debugger breakpoints and watchpoints) or statically (the faulty code is present in the
executable under test). The latest tally: 193 entries shown in the running game, 289 statically, 3 LATENT (the code
is wrong but cannot change play), 4 without a case by design. Run against a fixed build, a fixed entry turns NOT
SHOWN - so the suite doubles as a test bench for patches and for ports.

Windows, Python 3, DOSBox-X and your own game; `python tools/harness/setup.py ...` prepares a git-ignored `work/`
folder, then `python -P tools/harness/regress.py --kind ST` runs the static cases, `regress.py 2 47 203` single
cases. Everything else - what you need, the debugger build, writing a case, the lessons - is in
[`tools/harness/README.txt`](tools/harness/README.txt); every case with its setup and last result is in
[`tools/harness/TEST_CASES.txt`](tools/harness/TEST_CASES.txt). `re/` holds the address / name tables the harness
reads (the same names as the source).

## Notes (`docs/`)

[`docs/`](docs/README.md) holds reference notes from the work: [how the source was recovered](docs/HOW_IT_WAS_DONE.md),
the game's rules as the code computes them, a cross-check of our bugs against the community patch, how the graphics
engine works, the save file layout,
every screen's controls and hotkeys (from the source and from the running game), the music / sound labels, a porting
note on 32-bit math, how to drive the DOSBox-X debugger from a script, and how the source was matched byte for byte
to the original (Borland C++ 3.1 code-generation rules, the link method).

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
