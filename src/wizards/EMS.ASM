; EMS.ASM - WIZARDS.EXE code segment 16a9 (MAGIC.EXE 1617, the same file): the expanded-memory driver calls
; (INT 67h, LIM EMS 4.0) - detection, handles, mapping 4 logical pages into the 64 KB page frame, and copies in and
; out of EMS through the frame. Hand-written assembly (Borland TASM).
; The module starts with a WORD IN THE CODE SEGMENT (16a9:000a): EMS_Read / EMS_Write keep the first logical page
; there (cs: override); the local word their frames reserve is never used.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 16a9 src/wizards/EMS.ASM
        .MODEL  MEDIUM, C
        LOCALS
        .DATA                           ; this module's own data, DS:760c
        PUBLIC  ems_frame_seg
ems_frame_seg   dw      0               ; DS:760c  the EMS page frame segment (EMS_Detect)
ems_device_name db      'EMMXXXX0', 0   ; DS:760e  the EMS driver's device name
ems_map         dw      0, 0, 0, 1, 0, 2, 0, 3      ; DS:7617  {logical page, physical page} x 4 for AX=5000h
        .CODE
        PUBLIC  EMS_Detect, EMS_Handle_Count, EMS_Free_Pages, EMS_Handle_Pages, EMS_Alloc_Named
        PUBLIC  EMS_Get_Handle_Name, EMS_Free_Handle, EMS_Read, EMS_Write, EMS_Page_Frame
        PUBLIC  EMS_Map_Handle_Pages, EMS_Map_Pages
ems_first_page  dw      0               ; 16a9:000a  scratch of EMS_Read / EMS_Write (CS-relative)

; () -> -1 if an EMS 3.2+ driver is present, ready and has a page frame (stored in ems_frame_seg), else 0.
; The "EMMXXXX0" device is opened and its handle closed again; a failed IOCTL leaves it open.
; C:  int EMS_Detect(void) { int h = _open("EMMXXXX0", O_RDONLY); if (h == -1) return 0;
; C:      if (!(ioctl(h, 0) & 0x80) || !ioctl(h, 7)) return 0; _close(h);
; C:      _AH = 0x46; geninterrupt(0x67); if (_AH || _AL < 0x40) return 0;          (version 4.0 BCD = 40h)
; C:      _AH = 0x41; geninterrupt(0x67); if (_AH) return 0; ems_frame_seg = _BX; return -1; }
EMS_Detect PROC USES si di es ds
        mov     dx, offset ems_device_name
        mov     ax, 3D00h
        int     21h
        jb      @@no
        mov     bx, ax
        mov     ax, 4400h
        int     21h
        jb      @@no
        and     dx, 80h
        je      @@no
        mov     ax, 4407h
        int     21h
        jb      @@no
        or      al, al
        je      @@no
        mov     ah, 3Eh
        int     21h
        jb      @@no
        mov     ah, 46h
        int     67h
        cmp     ah, 0
        jne     @@no
        cmp     al, 40h
        js      @@no
        mov     ah, 41h
        int     67h
        cmp     ah, 0
        jne     @@no
        mov     ems_frame_seg, bx
        mov     ax, 0FFFFh
        ret
@@no:
        mov     ax, 0
        ret
EMS_Detect ENDP

; () the number of open EMS handles, 0 on error
; C:  int EMS_Handle_Count(void) { _AH = 0x4B; geninterrupt(0x67); return _AH ? 0 : _BX; }
EMS_Handle_Count PROC USES es ds si di
        mov     ah, 4Bh
        int     67h
        cmp     ah, 0
        jne     @@error
        mov     ax, bx
        ret
@@error:
        mov     ax, 0
        ret
EMS_Handle_Count ENDP

; () unallocated EMS pages, 0 on error
; C:  int EMS_Free_Pages(void) { _AH = 0x42; geninterrupt(0x67); return _AH ? 0 : _BX; }
EMS_Free_Pages PROC USES es ds si di
        mov     ah, 42h
        int     67h
        cmp     ah, 0
        jne     @@error
        mov     ax, bx
        ret
@@error:
        mov     ax, 0
        ret
EMS_Free_Pages ENDP

; (handle) pages the handle owns, 0 on error
; C:  int EMS_Handle_Pages(int handle) { _DX = handle; _AH = 0x4C; geninterrupt(0x67); return _AH ? 0 : _BX; }
EMS_Handle_Pages PROC USES es ds si di, handle:WORD
        mov     ah, 4Ch
        mov     dx, handle
        int     67h
        cmp     ah, 0
        jne     @@error
        mov     ax, bx
        ret
@@error:
        mov     ax, 0
        ret
EMS_Handle_Pages ENDP

; (pages, name) allocates pages under a new handle and names it (8 bytes at DS:name) -> handle, 0 on error
; C:  int EMS_Alloc_Named(int pages, char *name) { int h; _BX = pages; _AH = 0x43; geninterrupt(0x67);
; C:      if (_AH) return 0; h = _DX; _SI = (unsigned)name; _AX = 0x5301; geninterrupt(0x67); return _AH ? 0 : h; }
EMS_Alloc_Named PROC USES es ds si di, pages:WORD, ename:WORD
        LOCAL   handle:WORD
        mov     ah, 43h
        mov     bx, pages
        int     67h
        cmp     ah, 0
        jne     @@error
        mov     handle, dx
        mov     ax, 5301h
        mov     si, ename
        int     67h
        cmp     ah, 0
        jne     @@error
        mov     ax, handle
        ret
@@error:
        mov     ax, 0
        ret
EMS_Alloc_Named ENDP

; (buf, handle) the handle's 8-byte name into buf, 0-terminated -> -1, or 0 on error
; C:  int EMS_Get_Handle_Name(char *buf, int handle) { _DX = handle; _DI = (unsigned)buf; _ES = _DS; _AX = 0x5300;
; C:      geninterrupt(0x67); if (_AH) return 0; buf[8] = 0; return -1; }
EMS_Get_Handle_Name PROC USES es ds si di, buf:WORD, handle:WORD
        mov     ax, 5300h
        mov     dx, handle
        mov     di, buf
        mov     bx, ds
        mov     es, bx
        int     67h
        cmp     ah, 0
        jne     @@error
        mov     di, buf
        add     di, 8
        mov     al, 0
        stosb
        mov     ax, 0FFFFh
        ret
@@error:
        mov     ax, 0
        ret
EMS_Get_Handle_Name ENDP

; (handle) frees it (INT 67h AH=45h); handle 0 is left alone
; C:  void EMS_Free_Handle(int handle) { if (handle) { _DX = handle; _AH = 0x45; geninterrupt(0x67); } }
EMS_Free_Handle PROC USES es ds si di, handle:WORD
        cmp     handle, 0
        je      @@done
        mov     ah, 45h
        mov     dx, handle
        int     67h
@@done:
        ret
EMS_Free_Handle ENDP

; (dst, dst_seg, long ems_offset, count, handle) maps the 4 pages from ems_offset / 16 KB and copies count bytes
; out of the frame to dst_seg:dst (seg 0 = DGROUP). count must fit in the 64 KB the 4 pages cover.
; C:  void EMS_Read(char *dst, unsigned dst_seg, long ems_offset, unsigned count, int handle) {
; C:      int first = ems_offset >> 14; if (!dst_seg) dst_seg = _DS; EMS_Map_Handle_Pages(handle, first);   (pages 0..3)
; C:      movedata(ems_frame_seg, (unsigned)ems_offset & 0x3FFF, dst_seg, (unsigned)dst, count); }
EMS_Read PROC USES es ds si di, dst:WORD, dst_seg:WORD, ofs_lo:WORD, ofs_hi:WORD, count:WORD, handle:WORD
        LOCAL   unused:WORD
        cmp     dst_seg, 0
        jne     @@seg_ok
        mov     ax, ds
        mov     dst_seg, ax
@@seg_ok:
        mov     ax, ofs_lo
        mov     dx, ofs_hi
        shl     ax, 1
        rcl     dx, 1
        shl     ax, 1
        rcl     dx, 1
        mov     bx, dx
        mov     cs:ems_first_page, bx
        shr     ax, 1
        shr     ax, 1
        mov     si, ax
        mov     ax, 4400h
        mov     dx, handle
        int     67h
        mov     ax, 4401h
        mov     bx, cs:ems_first_page
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, 4402h
        mov     bx, cs:ems_first_page
        inc     bx
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, 4403h
        mov     bx, cs:ems_first_page
        inc     bx
        inc     bx
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, dst_seg
        mov     es, ax
        mov     di, dst
        mov     cx, count
        mov     ax, ems_frame_seg
        mov     ds, ax
        rep     movsb
        ret
EMS_Read ENDP

; (src, src_seg, long ems_offset, count, handle) the same into EMS
; C:  void EMS_Write(char *src, unsigned src_seg, long ems_offset, unsigned count, int handle) {
; C:      (as EMS_Read) movedata(src_seg, (unsigned)src, ems_frame_seg, (unsigned)ems_offset & 0x3FFF, count); }
EMS_Write PROC USES es ds si di, src:WORD, src_seg:WORD, ofs_lo:WORD, ofs_hi:WORD, count:WORD, handle:WORD
        LOCAL   unused:WORD
        cmp     src_seg, 0
        jne     @@seg_ok
        mov     ax, ds
        mov     src_seg, ax
@@seg_ok:
        mov     ax, ofs_lo
        mov     dx, ofs_hi
        shl     ax, 1
        rcl     dx, 1
        shl     ax, 1
        rcl     dx, 1
        mov     bx, dx
        mov     cs:ems_first_page, bx
        shr     ax, 1
        shr     ax, 1
        mov     di, ax
        mov     ax, 4400h
        mov     dx, handle
        int     67h
        mov     ax, 4401h
        mov     bx, cs:ems_first_page
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, 4402h
        mov     bx, cs:ems_first_page
        inc     bx
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, 4403h
        mov     bx, cs:ems_first_page
        inc     bx
        inc     bx
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, ems_frame_seg
        mov     es, ax
        mov     cx, count
        mov     si, src
        mov     ax, src_seg
        mov     ds, ax
        rep     movsb
        ret
EMS_Write ENDP

; () ems_frame_seg, read with DS loaded from DGROUP (callable with any DS)
; C:  unsigned EMS_Page_Frame(void) { return ems_frame_seg; }
EMS_Page_Frame PROC USES ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, ems_frame_seg
        ret
EMS_Page_Frame ENDP

; (handle, first) maps logical pages first .. first + 3 of handle to physical pages 0..3 (AH=44h, one call each)
; C:  void EMS_Map_Handle_Pages(int handle, int first) { int i;
; C:      for (i = 0; i < 4; i++) { _DX = handle; _BX = first + i; _AX = 0x4400 + i; geninterrupt(0x67); } }
EMS_Map_Handle_Pages PROC USES es ds, handle:WORD, first:WORD
        mov     ax, 4400h
        mov     dx, handle
        mov     bx, first
        int     67h
        mov     ax, 4401h
        mov     bx, first
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, 4402h
        mov     bx, first
        inc     bx
        inc     bx
        mov     dx, handle
        int     67h
        mov     ax, 4403h
        mov     bx, first
        inc     bx
        inc     bx
        inc     bx
        mov     dx, handle
        int     67h
        ret
EMS_Map_Handle_Pages ENDP

; (first, handle) the same in one call (AX=5000h with the ems_map table); keeps every register
; C:  void EMS_Map_Pages(int first, int handle) { int i; for (i = 0; i < 4; i++) ems_map[i].logical = first + i;
; C:      _CX = 4; _SI = (unsigned)ems_map; _DX = handle; _AX = 0x5000; geninterrupt(0x67); }
EMS_Map_Pages PROC USES ax bx cx dx si ds, first:WORD, handle:WORD
        mov     bx, first
        mov     dx, handle
        mov     ax, DGROUP
        mov     ds, ax
        mov     ems_map, bx
        inc     bx
        mov     ems_map + 4, bx
        inc     bx
        mov     ems_map + 8, bx
        inc     bx
        mov     ems_map + 12, bx
        inc     bx
        mov     cx, 4
        mov     si, offset ems_map
        mov     ax, 5000h
        int     67h
        ret
EMS_Map_Pages ENDP

        END
