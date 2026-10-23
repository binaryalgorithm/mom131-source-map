# mom131-source-map

An interactive map of the recovered source code of **Master of Magic v1.31** (MicroProse / Simtex, 1994-95):
`WIZARDS.EXE` (the game) and `MAGIC.EXE` (the new-game setup and world generator).

**Open the map:** https://binaryalgorithm.github.io/mom131-source-map/

## What it shows

- **One box per source file** of the recovered C / assembly source, grouped into areas (AI, combat, cities,
  magic, map, screens, graphics, engine). Box area = the file's share of the compiled code.
- **Every function** with its name, address in the original program, size, and a description of what it does
  (the header comment from the recovered source).
- **Every numbered bug and quirk** found while decompiling (489 entries): where it sits in the code, how it was
  confirmed (read in the code, checked in the assembly, or reproduced in the running game), and the latest result
  of the regression suite that reproduces it in the game.
- **Search** by function name, file, description text, or bug number (e.g. `#103`), and a sortable table view.
- Colour: numbered bugs per KB of code, or the share of a file's bugs that already have a regression result.

## How it was made

The source was decompiled and then matched function by function until it compiles (Borland C++ 3.1 and TASM)
back into executables **byte-identical** to the shipped v1.31 `WIZARDS.EXE` and `MAGIC.EXE` (all 152 and 49
modules). The map is generated from that source tree. The source itself, with instructions to reproduce the
build, will be published here once the current review pass is finished.

**This repository contains no game code, data or assets** - only names, descriptions and bug notes. You need your
own copy of the game to play it, and later to build anything from the source.

## Status

Work in progress: comments are still being refined while the regression suite reproduces each bug in the running
game. The map is regenerated as that work proceeds; the date it was built is shown at the top of the page.

## Credits

- Some names and findings were clarified using Jim Balcomb's ReMoM project (github.com/jbalcomb/ReMoM); those
  places are marked in the source.
- Bug entries name the community patch fixes that address them where one exists (the community patch's file
  names, e.g. W625FIX), and the Master of Magic wiki where it was the source.

Master of Magic is a trademark of its respective owners. This is an unofficial fan research project.
