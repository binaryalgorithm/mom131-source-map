; DOSUTIL.ASM - WIZARDS.EXE code segment 1491 (MAGIC.EXE 13ff, the same file): drives, directory, file size / date
; and delete through INT 21h. Hand-written assembly (Borland TASM). Only DOS_File_Size and DOS_Current_Path have
; callers; the rest are dead.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1491 src/wizards/DOSUTIL.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   Mouse_Hide:FAR, Mouse_Show:FAR
        .DATA                           ; this module's own data (78c-Z)
saved_int24_vector      dd      0       ; DS:71e6  the INT 24h vector while DOS_Drive_Ready probes a drive
        .CODE
        PUBLIC  DOS_Crit_Error_Ignore, DOS_Drive_Ready, DOS_Get_Drive, DOS_Delete_File, DOS_File_Size
        PUBLIC  DOS_Find_First, DOS_Drive_Exists, DOS_Current_Path

; INT 24h (critical error) handler: AL = 0 = ignore the error
; C:  void interrupt DOS_Crit_Error_Ignore(void) { _AL = 0; }   (Borland's interrupt prologue would differ)
DOS_Crit_Error_Ignore PROC
        mov     al, 0
        iret
DOS_Crit_Error_Ignore ENDP

; (drive: 0 = default, 1 = A ...) -> -1 if DOS reports the drive invalid (AH=36h free space), else 0; critical errors
; are ignored meanwhile. Drives above 2 are probed as A. No callers.
; QUIRK #384: DS is set to CS for the INT 24h install and never restored before the saved vector is read back - the
; vector restored is two words of the CODE segment, not saved_int24_vector (dead code, no effect).
; Found because ReMoM's numbering ran ahead of ours (10/17 audit); clarified using ReMoM repo
; Numbered: BUG #384 (BUGS_AND_QUIRKS.txt)
; C:  int DOS_Drive_Ready(int drive) { void interrupt (*old)() = getvect(0x24); unsigned r;
; C:      setvect(0x24, DOS_Crit_Error_Ignore); if (drive >= 3 || drive == 0) drive = 1;
; C:      _DL = drive; _AH = 0x36; geninterrupt(0x21); r = _AX; setvect(0x24, old); return r == 0xFFFF ? -1 : 0; }
DOS_Drive_Ready PROC USES es di si ds, drive:WORD
        mov     al, 24h
        mov     ah, 35h
        int     21h
        mov     word ptr saved_int24_vector, bx
        mov     word ptr saved_int24_vector + 2, es
        mov     ax, cs
        mov     ds, ax
        mov     dx, offset DOS_Crit_Error_Ignore
        mov     al, 24h
        mov     ah, 25h
        int     21h
        mov     dx, drive
        cmp     dx, 3
        js      @@low
        mov     dx, 1
        jmp     @@probe
@@low:
        cmp     dx, 0
        jne     @@probe
        mov     dx, 1
@@probe:
        mov     ah, 36h
        int     21h
        push    ax
        mov     dx, word ptr saved_int24_vector
        mov     ds, word ptr saved_int24_vector + 2
        mov     al, 24h
        mov     ah, 25h
        int     21h
        pop     ax
        cmp     ax, 0FFFFh
        je      @@done
        mov     ax, 0
@@done:
        ret
DOS_Drive_Ready ENDP

; () the current drive, 0 = A. No callers.
; C:  int DOS_Get_Drive(void) { return getdisk(); }
DOS_Get_Drive PROC USES es ds si di
        mov     ah, 19h
        int     21h
        xor     ah, ah
        ret
DOS_Get_Drive ENDP

; (name) deletes the file; returns AL of INT 21h AH=41h (the error code low byte if it failed). No callers.
; Found because ReMoM's numbering ran ahead of ours (10/17 audit); clarified using ReMoM repo
; C:  int DOS_Delete_File(char *name) { _DX = (unsigned)name; _AH = 0x41; geninterrupt(0x21); return _AL; }
DOS_Delete_File PROC USES es di ds si, fname:WORD
        mov     dx, fname
        mov     ah, 41h
        int     21h
        xor     ah, ah
        ret
DOS_Delete_File ENDP

; (name) -> the file's size in DX:AX from find-first (DTA + 1Ah), 0 if not found; the mouse is hidden meanwhile
; C:  long DOS_File_Size(char *name) { struct ffblk f; long n = 0; Mouse_Hide();
; C:      if (findfirst(name, &f, 0) == 0) n = f.ff_fsize; Mouse_Show(); return n; }
; Simtex name LOF (MoO2 symbols, module dos); clarified using ReMoM repo
DOS_File_Size PROC USES es di ds si, fname:WORD
        LOCAL   dta:DWORD
        push    ds
        call    Mouse_Hide
        mov     ah, 2Fh
        int     21h
        add     bx, 1Ah
        mov     word ptr dta, bx
        mov     bx, es
        mov     word ptr dta + 2, bx
        push    ds
        mov     ah, 4Eh
        mov     cx, 0
        mov     dx, fname
        int     21h
        pop     ds
        jb      @@none
        mov     ax, word ptr dta + 2
        mov     es, ax
        mov     di, word ptr dta
        mov     ax, word ptr es:[di]
        add     di, 2
        mov     dx, word ptr es:[di]
        pop     ds
        push    ax
        push    dx
        call    Mouse_Show
        pop     dx
        pop     ax
        ret
@@none:
        mov     ax, 0
        mov     dx, 0
        pop     ds
        push    ax
        push    dx
        call    Mouse_Show
        pop     dx
        pop     ax
        ret
DOS_File_Size ENDP

; (pattern) -> DX:AX = the time and date words (DTA + 16h) of the first match - read even when nothing matched
; (no carry test: then the DTA's old contents). No callers.
; C:  long DOS_Find_First(char *pattern) { struct ffblk f; Mouse_Hide(); findfirst(pattern, &f, 0); Mouse_Show();
; C:      return ((long)f.ff_fdate << 16) | f.ff_ftime; }
DOS_Find_First PROC USES es di ds si, pattern:WORD
        LOCAL   dta:DWORD
        push    ds
        call    Mouse_Hide
        mov     ah, 2Fh
        int     21h
        add     bx, 16h
        mov     word ptr dta, bx
        mov     bx, es
        mov     word ptr dta + 2, bx
        push    ds
        mov     ah, 4Eh
        mov     cx, 0
        mov     dx, pattern
        int     21h
        pop     ds
        mov     ax, word ptr dta + 2
        mov     es, ax
        mov     di, word ptr dta
        mov     ax, word ptr es:[di]
        add     di, 2
        mov     dx, word ptr es:[di]
        pop     ds
        push    ax
        push    dx
        call    Mouse_Show
        pop     dx
        pop     ax
        ret
DOS_Find_First ENDP

; (drive, 0 = A) -> -1 if selecting it works (the current drive is then that drive), else 0; the current drive is
; restored. No callers.
; C:  int DOS_Drive_Exists(int drive) { int cur = getdisk(), ok; setdisk(drive); ok = getdisk() == drive ? -1 : 0;
; C:      setdisk(cur); return ok; }
DOS_Drive_Exists PROC USES es ds si di, drive:WORD
        mov     ax, drive
        push    ax
        mov     ah, 19h
        int     21h
        pop     dx
        push    ax
        push    dx
        mov     al, dl
        mov     ah, 0Eh
        int     21h
        mov     ah, 19h
        int     21h
        pop     bx
        pop     dx
        cmp     al, bl
        jne     @@no
        mov     bx, 0FFFFh
        jmp     @@restore
@@no:
        mov     bx, 0
@@restore:
        push    bx
        mov     ah, 0Eh
        int     21h
        pop     ax
        ret
DOS_Drive_Exists ENDP

; (buf) writes "X:\" + the current directory of the current drive
; C:  int DOS_Current_Path(char *buf) { buf[0] = 'A' + getdisk(); buf[1] = ':'; buf[2] = '\\';
; C:      return getcurdir(0, buf + 3) & 0xFF; }
DOS_Current_Path PROC USES si di es ds, buf:WORD
        mov     ah, 19h
        int     21h
        mov     si, buf
        add     al, 41h
        mov     ah, 3Ah
        mov     word ptr [si], ax
        add     si, 2
        mov     al, 5Ch
        mov     byte ptr [si], al
        add     si, 1
        mov     dx, 0
        mov     ah, 47h
        int     21h
        xor     ah, ah
        ret
DOS_Current_Path ENDP

        END
