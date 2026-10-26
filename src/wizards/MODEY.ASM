; MODEY.ASM - WIZARDS.EXE code segment 18f8 (MAGIC.EXE 1866, the same file): switch the VGA to the unchained 256-color
; mode "Mode Y" (unchained 320 x 200, 4 pages of planar memory; "Mode X" is the 320 x 240 variant).
; Hand-written assembly (Borland TASM).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 18f8 src/wizards/MODEY.ASM
        .MODEL  MEDIUM, C
        LOCALS
        .CODE
        PUBLIC  VGA_Set_ModeY

; () BIOS mode 13h, then: sequencer memory mode chain-4 off / odd-even off (planar), clear all 4 planes (64 KB each,
; the map mask is still 0Fh after the mode set), CRTC underline: dword mode off, mode control: byte mode on; leaves
; the sequencer index at 2 (map mask) for the drawing code.
; C:  void VGA_Set_ModeY(void) { _AX = 0x13; geninterrupt(0x10);
; C:      outportb(0x3C4, 4); outportb(0x3C5, (inportb(0x3C5) & 0xF7) | 4);
; C:      _fmemset(MK_FP(0xA000, 0), 0, 0xFFFF); _fmemset(MK_FP(0xA000, 0xFFFF), 0, 1);     (all planes, words)
; C:      outportb(0x3D4, 0x14); outportb(0x3D5, inportb(0x3D5) & 0xBF);
; C:      outportb(0x3D4, 0x17); outportb(0x3D5, inportb(0x3D5) | 0x40); outportb(0x3C4, 2); }
VGA_Set_ModeY PROC USES si di es ds
        mov     ax, 13h
        int     10h
        mov     dx, 03C4h
        mov     al, 4
        out     dx, al
        inc     dx
        in      al, dx
        and     al, 0F7h
        or      al, 4
        out     dx, al
        mov     ax, 0A000h
        mov     es, ax
        sub     di, di
        mov     ax, di
        mov     cx, 8000h
        cld
        rep     stosw
        mov     dx, 03D4h
        mov     al, 14h
        out     dx, al
        inc     dx
        in      al, dx
        and     al, 0BFh
        out     dx, al
        dec     dx
        mov     al, 17h
        out     dx, al
        inc     dx
        in      al, dx
        or      al, 40h
        out     dx, al
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        ret
VGA_Set_ModeY ENDP

        END
