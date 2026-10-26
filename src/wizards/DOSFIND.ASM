; DOSFIND.ASM - WIZARDS.EXE code segment 14ab (MAGIC.EXE 1418, the same file): directory search by name and free
; disk space. Hand-written assembly (Borland TASM).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 14ab src/wizards/DOSFIND.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   Mouse_Hide:FAR, Mouse_Show:FAR
        .CODE
        PUBLIC  DOS_Find_File, DOS_Disk_Free

; (pattern, out) -> -1 and the found name copied to out, or 0 and out = "". An empty pattern continues the last
; search (find-next). The mouse is hidden meanwhile.
; C:  int DOS_Find_File(char *pattern, char *out) { static struct ffblk f; int r; Mouse_Hide();
; C:      r = pattern[0] ? findfirst(pattern, &f, 0) : findnext(&f);
; C:      if (r == 0) strcpy(out, f.ff_name); else out[0] = 0; Mouse_Show(); return r == 0 ? -1 : 0; }
; C:      (the original keeps no ffblk of its own: it uses whatever DTA DOS has)
; Simtex name DIR (MoO2 symbols, module dos); clarified using ReMoM repo
DOS_Find_File PROC USES es di ds si, pattern:WORD, out:WORD
        LOCAL   dta:DWORD
        push    ds
        call    Mouse_Hide
        mov     ah, 2Fh
        int     21h
        add     bx, 1Eh
        mov     word ptr dta, bx
        mov     bx, es
        mov     word ptr dta + 2, bx
        mov     si, pattern
        lodsb
        cmp     al, 0
        je      @@next
        push    ds
        mov     ah, 4Eh
        mov     cx, 0
        mov     dx, pattern
        int     21h
        pop     ds
        jb      @@none
        jmp     @@found
@@next:
        mov     ah, 4Fh
        int     21h
        jb      @@none
@@found:
        mov     ax, ds
        mov     es, ax
        mov     di, out
        mov     si, word ptr dta
        mov     ax, word ptr dta + 2
        mov     ds, ax
@@copy:
        lodsb
        stosb
        cmp     al, 0
        jne     @@copy
        mov     ax, 0FFFFh
        pop     ds
        push    ax
        call    Mouse_Show
        pop     ax
        ret
@@none:
        mov     al, 0
        mov     si, out
        mov     byte ptr [si], al
        mov     ax, 0
        pop     ds
        push    ax
        call    Mouse_Show
        pop     ax
        ret
DOS_Find_File ENDP

; () free bytes on the current drive in DX:AX (sectors per cluster x bytes per sector x free clusters, the first
; product kept to 16 bits), -1 if DOS reports the drive invalid. No callers.
; C:  long DOS_Disk_Free(void) { struct dfree d; getdfree(0, &d); if (d.df_sclus == 0xFFFF) return -1L;
; C:      return (long)(unsigned)(d.df_sclus * d.df_bsec) * d.df_avail; }
DOS_Disk_Free PROC USES es di ds si
        mov     dx, 0
        mov     ah, 36h
        int     21h
        cmp     ax, 0FFFFh
        je      @@invalid
        mul     cx
        mul     bx
        jmp     @@done
@@invalid:
        mov     dx, 0FFFFh
        mov     ax, 0FFFFh
@@done:
        ret
DOS_Disk_Free ENDP

        END
