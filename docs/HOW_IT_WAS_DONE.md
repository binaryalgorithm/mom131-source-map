# How the source was recovered

Twelve days, 15-26 October 2026, from two shipped executables to commented C that compiles back into them byte for
byte. The work was done by Claude (Anthropic's AI model, in Claude Code), directed and reviewed by the repository's
owner, who set the goals, made the judgment calls, and checked results in the running game. Other AI agents were
used as blind reviewers and an adversarial checker (details below). The project kept a step-by-step process log; this
is the short version.

## 0. Starting point

- **The target:** `WIZARDS.EXE` (the game, 1,001,232 bytes) and `MAGIC.EXE` (setup and world generator, 303,264
  bytes) of Master of Magic v1.31, as sold by GOG. Both say "Borland C++ - Copyright 1991" and carry Borland VROOMM
  overlays: about 77% of the game's code is in overlays the standard tools do not load.
- **Public knowledge:** the Master of Magic wiki (formulas, part of the save layout, the community patch notes) and
  Jim Balcomb's [ReMoM](https://github.com/jbalcomb/ReMoM), a C reconstruction of v1.31. We decided to do our own
  reading and keep ReMoM closed as an answer key until our rebuild was complete; it was opened on 22 October for a
  structured comparison (section 7), and the places where it helped are credited in the source.

## 1. Opening the program (day 1)

- **Overlays, worked out from the bytes.** Each overlaid module has a resident stub (`INT 3Fh` thunks). The first
  attempt walked the stubs as one contiguous list and found only 17 of the overlays. Scanning every paragraph of the
  image for well-formed stubs found all 115 (WIZARDS) and 14 (MAGIC), every entry point on a `push bp; mov bp, sp`.
  The far calls inside overlay code looked like nonsense until a histogram showed their segment words were indexes
  into an 8-byte-record table: Borland's segment table, found by searching the image for it. Translating through it
  made 100% of the calls land on prologues, thunks or known helpers.
- **A flattened image** (resident code + every overlay as an ordinary segment, calls redirected) went into Ghidra:
  2,310 functions, about 1,300 of them in code no tool had shown before. First proof it was right: the first
  overlay's two functions are Save and Load Game, writing exactly the wiki's save blocks.
- **Ghidra's guess of the data segment was wrong** (empty tables, no strings). The real one was found from the
  strings `Save_Game` passes ("SAVE", ".GAM", "wb", "MAGIC.SET").

## 2. First reading (days 1-3)

- **Naming by grounding, not by guessing:** DOS wrappers by the `AH` value in their own code; globals from
  `Save_Game`'s 35 writes against the wiki's block table; record layouts from a census of every offset the code
  touches plus the value ranges in real saves. The code then corrected the census (the city building array starts
  at 0x1F, not where the census put it).
- **Data files decoded by correlation:** `SPELLDAT.LBX`'s bytes against each spell's wiki infobox, `BUILDDAT.LBX`
  against a city screen. The owner played and sent screenshots; predictions were checked against them (unit
  enchantments from a save's bitfield, a city's production and the buy-price formula: 400 gold = 100 x 4).
- **Game rules solved against saves:** food, production, unrest, gold and upkeep, research and power were
  re-implemented and matched for every city of several saves.
- **Coverage:** parallel reading passes over combat, AI, movement, the turn cycle, diplomacy, items and screens.
  Their findings were marked READ (not yet verified) and re-checked one by one against the matched source on 22
  October.

## 3. Line-by-line reading (days 3-4)

Every function was read again, with the assembly wherever the decompiler and the reading disagreed. **The community
patch served as an independent answer key:** each of its patch files that claims to fix a 1.31 bug was mapped onto
the code it changes, and every one we had not found was reviewed (bytes, the patch applied to a scratch copy, our
reading of the code). Coverage of those patch fixes went from 28% to 70% in that pass, and 89 new bug entries came
out of it; the rest were judged cosmetic, tuning, disputed (with reasons) or deferred. The current state is
[`COMMUNITY_PATCH_CROSSCHECK.txt`](COMMUNITY_PATCH_CROSSCHECK.txt).

## 4. Watching the game run

A harness drives the real game in DOSBox-X on a hidden Windows desktop: keys by posted messages, clicks by writing
the game's own mouse latch, memory through Game Link, and later the DOSBox-X debugger over its TCP channel
(breakpoints, watchpoints, save states). It confirmed rules, settled dead-code questions, surveyed 30 generated
worlds, and became the regression suite (section 8). See [`DOSBOX_X_DEBUG.txt`](DOSBOX_X_DEBUG.txt) and
[`ui/HARNESS_UI_NOTES.txt`](ui/HARNESS_UI_NOTES.txt).

## 5. Byte matching (days 5-7)

- **Which compiler:** Borland C++ 3.0 was tried first and rejected (its runtime and startup code differ, and it never
  emits one of the game's index-scaling patterns). Borland C++ 3.1 reproduced 87 runtime modules exactly; the
  settings `-c -mm -a -d` (`-Y` for overlays) came out of the first matched modules.
- **The first module** (`MAPVIS.C`, 8 functions) compiled to the game's bytes on 19 October. A review of the method
  found a hole: a byte comparison masks the bytes the linker fills in, so a wrong global or a wrong callee would
  still "match". A second check was added that resolves every fixup against the source's addresses, the function
  table and the game's strings (a negative test with two swapped globals is caught). A module counts as matched
  only when both pass.
- **Then module by module, 137 batches.** Byte matching turned out to be the strongest truth test of the reading:
  a wrong signedness, array shape, argument count or statement order changes the compiled code. It corrected types,
  names and a few earlier "bugs" (logged in [`matching/MATCH_CALIBRATION.txt`](matching/MATCH_CALIBRATION.txt)).
  The code-generation rules learned on the way are in [`matching/MATCH_PLAYBOOK.txt`](matching/MATCH_PLAYBOOK.txt).
- **Linking:** object order recovered from the data segment, uninitialised data in Borland's reverse declaration
  order, overlay stubs in first-declaration order, and the link date (3 March 1995, stored by the overlay manager).
  `MAGIC.EXE` linked identical on 19 October, `WIZARDS.EXE` (152 / 152 modules, 852,887 bytes) on 21 October -
  with only the Miles sound library as a stand-in taken from the game's own bytes (since replaced by Miles' released
  source, below). The rebuilt executables were then play-tested. Method: [`matching/LINK_METHOD.txt`](matching/LINK_METHOD.txt).

## 6. Making it readable (days 7-8)

Byte identity was kept through every later edit (each change rebuilt and compared). On top of the matched code:
one declaration per symbol, about 2,900 magic numbers replaced by named constants, names fixed where usage proved
them wrong, every structure field and global commented.

**Blind reviews** measured the result: groups of AI reviewer agents, each given one slice of the source and none of
our notes, listed what was misleading, vague, inconsistent, unnamed or missing. Run 1: 1,381 findings. Run 2: 1,444,
mean file grade 6.0 / 10. After fixing every finding by priority, run 3: high + medium findings per 1,000 lines 6.6 ->
3.4, grade 6.7, and a comprehension quiz (predict 40 functions from their signature and header comment, scored
against the code) at 2.38 / 3. Every finding was checked against the code before anything was changed.

## 7. The ReMoM comparison (day 8)

With the source frozen (git tag `pre-remom`), the 1,528 function pairs between ReMoM and our source were compared:
bug markers, names, globals, open questions. Our own analysis had already cut the provisional marks in the source
from 368 to 102; the comparison settled most of the rest, added bug points, and supplied 204 original Simtex function names (from Master of Orion 2's symbol table, which shares the engine), which
are kept as comments beside our names. It also found translation slips on both sides. Each place it helped carries
the line "clarified using ReMoM repo" (301 in the source). A second model (OpenAI Codex) reviewed the comparison's
claims adversarially; each objection was re-checked against the code.

## 8. Bugs and the regression suite (days 8-12)

All 491 bug and quirk entries were reconciled with the source and given a citation in it (`BUG #n`). Then every
entry got a regression case run against the real game: play cases (set up a save, act, read the result from memory
or a breakpoint) and static cases (the faulty code bytes are present in the executable under test). Final tally:
193 shown in the running game, 289 shown statically (14 of them as the fallback for a play case that could not
reach the code), 3 LATENT (real but cannot change play), 4 without a case by design. The cases tell a fixed build
apart: with three community fixes applied to a scratch copy of the game, the entries they fix (#155, #199, #200)
flipped to "not shown", while a control entry they do not touch stayed shown.

## 9. Publishing (day 12)

The build kit was tested from scratch: the compiler installed from the WinWorld disk images, every module compiled. One surprise: compiling everything in one DOSBox
session does not give identical code - Borland C++ 3.1 reads leftovers of the previous run from extended memory,
and three large modules then choose different registers. One fresh session per module does. A clean-room run that
used only the public files produced both executables with the original SHA-256.

The same day Jim Balcomb pointed us to John Miles' released AIL 2.14 source (open-source freeware since 2000):
assembled with the flags of its own makefile, it links into both programs byte-identical, so the stand-in made from
the game's bytes is gone and the build reads no file of the game at all.

## What worked

- **Use the compiler as the judge.** Once the code compiles byte-identical, every type, branch and argument in it is
  proven; most remaining doubt is about names and comments, not behaviour.
- **Answer keys after, not before.** Our reading came first; ReMoM, the community patch and the wiki were used to find
  what we had missed and to settle disagreements, with the bytes as the referee.
- **Mark certainty.** Every claim carries how it was established (read, checked in the assembly, shown in the game).
- **Measure readability.** Blind reviewers with no context found what the authors could no longer see.
