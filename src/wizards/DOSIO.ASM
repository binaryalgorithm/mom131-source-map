; DOSIO.ASM - WIZARDS.EXE code segment 1573 (MAGIC.EXE 14e1, the same file): the DOS file calls of the LBX reader
; and far_strcpy. Hand-written assembly (Borland TASM): every procedure saves SI DI ES DS and each return pops them.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1573 src/wizards/DOSIO.ASM
        .MODEL  MEDIUM, C
        LOCALS
        .CODE
        PUBLIC  DOS_Open_Read, DOS_Close, DOS_Seek, DOS_File_Length, DOS_Read_To_Segment, DOS_Read_Near, far_strcpy

; (path) -> handle, 0 if the open fails (INT 21h AX=3D00h, read only)
; C:  int DOS_Open_Read(char *path) { int h = _open(path, O_RDONLY); return h == -1 ? 0 : h; }
DOS_Open_Read PROC USES si di es ds, path:WORD
        mov     ax, 3D00h
        mov     dx, path
        int     21h
        jb      @@fail
        ret
@@fail:
        mov     ax, 0
        ret
DOS_Open_Read ENDP

; (handle) INT 21h AH=3Eh
; C:  void DOS_Close(int handle) { _close(handle); }
DOS_Close PROC USES si di es ds, handle:WORD
        mov     ah, 3Eh
        mov     bx, handle
        int     21h
        ret
DOS_Close ENDP

; (lo, hi, handle) seek from the start (INT 21h AX=4200h): -1 = done, 0 = error
; C:  int DOS_Seek(unsigned lo, unsigned hi, int handle)
; C:      { return lseek(handle, ((long)hi << 16) | lo, SEEK_SET) == -1L ? 0 : -1; }
DOS_Seek PROC USES si di es ds, lo:WORD, hi:WORD, handle:WORD
        mov     ax, 4200h
        mov     bx, handle
        mov     cx, hi
        mov     dx, lo
        int     21h
        jb      @@fail
        mov     ax, 0FFFFh
        ret
@@fail:
        mov     ax, 0
        ret
DOS_Seek ENDP

; (handle) seek to the end (INT 21h AX=4202h): the file length in DX:AX, 0 on error. No callers.
; C:  long DOS_File_Length(int handle) { long n = lseek(handle, 0L, SEEK_END); return n == -1L ? 0L : n; }
DOS_File_Length PROC USES si di es ds, handle:WORD
        mov     ax, 4202h
        mov     bx, handle
        mov     cx, 0
        mov     dx, 0
        int     21h
        jb      @@fail
        ret
@@fail:
        mov     ax, 0
        mov     dx, 0
        ret
DOS_File_Length ENDP

; (seg, bytes, handle) read to seg:0 (INT 21h AH=3Fh): -1 = done, 0 = error
; C:  int DOS_Read_To_Segment(unsigned seg, unsigned bytes, int handle)
; C:      { return _read(handle, MK_FP(seg, 0), bytes) == -1 ? 0 : -1; }   (a far read)
DOS_Read_To_Segment PROC USES si di es ds, dest:WORD, bytes:WORD, handle:WORD
        mov     ah, 3Fh
        mov     bx, handle
        mov     cx, bytes
        mov     dx, dest
        mov     ds, dx
        mov     dx, 0
        int     21h
        jb      @@fail
        mov     ax, 0FFFFh
        ret
@@fail:
        mov     ax, 0
        ret
DOS_Read_To_Segment ENDP

; (buf, bytes, handle) read to DS:buf (INT 21h AH=3Fh): -1 = done, 0 = error
; C:  int DOS_Read_Near(char *buf, unsigned bytes, int handle) { return _read(handle, buf, bytes) == -1 ? 0 : -1; }
DOS_Read_Near PROC USES si di es ds, buf:WORD, bytes:WORD, handle:WORD
        mov     ah, 3Fh
        mov     bx, handle
        mov     cx, bytes
        mov     dx, buf
        int     21h
        jb      @@fail
        mov     ax, 0FFFFh
        ret
@@fail:
        mov     ax, 0
        ret
DOS_Read_Near ENDP

; (dst, dst_seg, src, src_seg) copy a 0-terminated string between segments; a segment of 0 means DGROUP
; C:  void far_strcpy(char *dst, unsigned dst_seg, char *src, unsigned src_seg)
; C:      { if (dst_seg == 0) dst_seg = _DS; if (src_seg == 0) src_seg = _DS;
; C:        _fstrcpy(MK_FP(dst_seg, dst), MK_FP(src_seg, src)); }
far_strcpy PROC USES si di es ds, dst:WORD, dst_seg:WORD, src:WORD, src_seg:WORD
        cmp     dst_seg, 0
        jne     @@dst_ok
        mov     ax, ds
        mov     dst_seg, ax
@@dst_ok:
        cmp     src_seg, 0
        jne     @@src_ok
        mov     ax, ds
        mov     src_seg, ax
@@src_ok:
        mov     ax, dst_seg
        mov     es, ax
        mov     si, src
        mov     di, dst
        mov     ax, src_seg
        mov     ds, ax
@@copy:
        lodsb
        stosb
        cmp     al, 0
        jne     @@copy
        ret
far_strcpy ENDP

        END
