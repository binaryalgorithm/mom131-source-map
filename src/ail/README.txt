MILES AIL 2.14 - THE SOUND LIBRARY BOTH PROGRAMS LINK
=====================================================
AIL.ASM, AIL.INC and AIL.MAC are the Application Program Interface module of the IBM Audio Interface Library (AIL)
V2.14 by John Miles / Miles Design, Inc. (1991-92), unchanged - as published in github.com/Tronix286/AIL2 (commit
f9c0559), which carries the original 2.14 package. They are not Simtex code and not part of our reconstruction.

Assembled with TASM from the Borland C++ 3.1 tree with the flags of the library's own makefile (AIL.MAK):

    TASM /m /w+ /ml AIL.ASM

the object links into both programs in place of the module: WIZARDS.EXE (as S2FC1.OBJ, its segment 2fc1) and
MAGIC.EXE (as AIL.OBJ), and both executables come out with the original SHA-256 (checked 2026-10-26). The sound
drivers themselves (Sound Blaster, Ad Lib, Roland ...) are not part of the executables: the game loads them from
SNDDRV.LBX at run time.

Licence: John Miles released the AIL V2.14 and AIL/32 source and object code on 26 May 2000 as "open-source
freeware," usable by anyone for any purpose, commercial or otherwise, without restriction or limitation (his
statement is reproduced in the README of the repository above). Thanks to Jim Balcomb (ReMoM) for pointing us to it.
