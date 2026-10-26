; ANIMEMS.ASM - WIZARDS.EXE code segment 1e9f (MAGIC.EXE 1e0d, the same file): drawing one frame of an LBX animation
; that is cached in EMS, straight out of the page frame (2 x 16 KB pages mapped at a time). Hand-written assembly
; (Borland TASM). Frame data: per plane column a run list {count, skip, ..} - see the C reference below.
; Three WORDS IN THE CODE SEGMENT at the module start (1e9f:0008): the EMS logical page, the EMS handle and the
; animation width, shared with the near helper Anim_EMS_Map and the remap variant.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1e9f src/wizards/ANIMEMS.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   ems_frame_seg:WORD      ; DS:760c
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        .DATA                           ; this module's own data, DS:7894
                dw      0               ; DS:7892  no reader (link test, batch 36: the word before plane_bits in both programs)
plane_bits      db      01h, 02h, 04h, 08h      ; DS:7894  the VGA plane of x & 3
        .CODE
        PUBLIC  Anim_Blit_EMS_Frame, Anim_EMS_Map, Anim_Blit_EMS_Frame_Remap
ems_page        dw      0               ; 1e9f:0008  logical EMS page now mapped to physical page 0
ems_handle      dw      0               ; 1e9f:000a  the animation's EMS handle
anim_width      dw      0               ; 1e9f:000c  columns to draw (animation header word 0)

; (x, y, anim_seg, frame) the animation header (at anim_seg:0: width, .., +0Ah handle, +0Bh first page, +0Ch offset,
; +11h remap flag, +12h frame offsets) locates the frame in EMS; the frame is decoded column by column: a run list
; per column, runs of 'count' pixels after 'skip' rows (bit 15 of the run word: packed runs with E0h..FFh = repeat
; counts). Frames whose header asks for it go to Anim_Blit_EMS_Frame_Remap instead.
; C:  void Anim_Blit_EMS_Frame(int x, int y, unsigned anim_seg, int frame) { (map pages; locate frame;
; C:      for each column c < width: for each run: skip rows, copy pixels (0xE0+n = n+1 copies of the next byte)) }
Anim_Blit_EMS_Frame PROC USES si di es ds, x:WORD, y:WORD, aseg:WORD, frame:WORD
        mov     ax, aseg
        mov     ds, ax
        mov     es, ax
        mov     ax, word ptr ds:[0]
        mov     cs:anim_width, ax
        mov     si, 0Ah
        lodsb
        xor     ah, ah
        mov     dx, ax
        lodsb
        xor     ah, ah
        mov     bx, ax
        lodsw
        mov     si, ax
        mov     di, si
        push    bx
        push    dx
        mov     ax, 4400h
        int     67h
        mov     ax, 4401h
        pop     dx
        pop     bx
        inc     bx
        int     67h
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, frame
        shl     ax, 1
        shl     ax, 1
        add     si, ax
        add     si, 12h
        mov     ax, ems_frame_seg
        mov     ds, ax
        lodsw
        mov     dx, ax
        lodsw
        inc     dx
        adc     ax, 0
        shl     dx, 1
        rcl     ax, 1
        shl     dx, 1
        rcl     ax, 1
        shr     dx, 1
        shr     dx, 1
        add     al, byte ptr es:[0Bh]
        adc     ah, 0
        add     dx, word ptr es:[0Ch]
        cmp     dx, 4000h
        jb      @@page_ok
        and     dx, 3FFFh
        inc     ax
@@page_ok:
        mov     cs:ems_page, ax
        mov     bx, 11h
        add     bx, di
        mov     bl, [bx]
        xor     bh, bh
        mov     di, bx
        mov     si, dx
        mov     bx, ax
        mov     dl, byte ptr es:[0Ah]
        xor     dh, dh
        mov     cs:ems_handle, dx
        push    bx
        push    dx
        mov     ax, 4400h
        int     67h
        mov     ax, 4401h
        pop     dx
        pop     bx
        inc     bx
        int     67h
        cmp     di, 0
        je      @@plain
        push    rseg                    ; the remap procedure's argument name - a forward reference (TASM pads it)
        push    y
        push    x
        call    far ptr Anim_Blit_EMS_Frame_Remap
        add     sp, 6
        ret
@@plain:
        mov     ax, DGROUP
        mov     ds, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, y
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, x
        mov     bx, ax
        shr     bx, 1
        shr     bx, 1
        mov     di, bx
        and     ax, 3
        mov     bx, offset plane_bits
        add     bx, ax
        mov     ah, [bx]
        mov     bx, ems_frame_seg
        mov     ds, bx
        mov     bx, cs:anim_width
@@column:
        push    di
        cmp     si, 7C00h
        jb      @@mapped
        call    Anim_EMS_Map
@@mapped:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        lodsb
        cmp     al, 0FFh
        je      @@column_done
        mov     dh, al
        lodsb
        mov     dl, al
        cmp     dh, 0
        js      @@packed
@@run:
        mov     cl, [si+1]
        xor     ch, ch
        mov     al, cl
        shl     cx, 1
        shl     cx, 1
        add     cl, al
        adc     ch, 0
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        add     di, cx
        lodsb
        mov     cl, al
        xor     ch, ch
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@copy:
        movsb
        add     di, 79
        loop    @@copy
        cmp     dx, 1
        jns     @@run
@@column_done:
        pop     di
        dec     bx
        je      @@done
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        jmp     @@column
@@packed:
        and     dx, 7FFFh
@@prun:
        mov     cl, [si+1]
        xor     ch, ch
        mov     al, cl
        shl     cx, 1
        shl     cx, 1
        add     cl, al
        adc     ch, 0
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        add     di, cx
        lodsb
        mov     cl, al
        xor     ch, ch
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@pixel:
        lodsb
        cmp     al, 0E0h
        jae     @@repeat
        stosb
        add     di, 79
        loop    @@pixel
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
@@repeat:
        and     al, 1Fh
        inc     al
        push    cx
        mov     cl, al
        xor     ch, ch
        lodsb
@@fill:
        stosb
        add     di, 79
        loop    @@fill
        pop     cx
        dec     cx
        loop    @@pixel
        cmp     dx, 1
        jns     @@prun
        pop     di
        dec     bx
        je      @@done
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte2
        mov     ah, 1
        inc     di
@@same_byte2:
        jmp     @@column
@@done:
        ret
Anim_Blit_EMS_Frame ENDP

; near register helper: SI ran past 7C00h - map the next logical page pair (ems_page + 1, + 2) and move SI back 16 KB;
; keeps AX, BX
; C:  static void Anim_EMS_Map(void) { ems_page++; EMS_Map(ems_handle, ems_page, 0); EMS_Map(ems_handle, ems_page + 1, 1);
; C:      src -= 0x4000; }
Anim_EMS_Map PROC NEAR
        push    bx
        mov     cx, ax
        inc     cs:ems_page
        mov     ax, 4400h
        mov     bx, cs:ems_page
        mov     dx, cs:ems_handle
        int     67h
        mov     ax, 4401h
        mov     bx, cs:ems_page
        inc     bx
        mov     dx, cs:ems_handle
        int     67h
        sub     si, 4000h
        mov     ax, cx
        pop     bx
        ret
Anim_EMS_Map ENDP

; (x, y, anim_seg) the translucent variant, called by Anim_Blit_EMS_Frame with its SI (frame data in the EMS frame) and
; anim_width: pixels E8h..FFh are remap-table numbers - the screen pixel under them goes through that remap table
; (VGA read map via 3CEh index 4); E0h..E7h = repeat counts as in the plain blitter. Saves no registers.
; C:  as Anim_Blit_EMS_Frame, but pixel p >= 0xE8: screen = remap[p - 0xE8][screen]
Anim_Blit_EMS_Frame_Remap PROC, rx:WORD, ry:WORD, rseg:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, ry
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, rx
        mov     bx, ax
        shr     bx, 1
        shr     bx, 1
        mov     di, bx
        and     ax, 3
        mov     cl, al
        shl     cl, 1
        shl     cl, 1
        shl     cl, 1
        shl     cl, 1
        mov     bx, offset plane_bits
        add     bx, ax
        mov     al, [bx]
        mov     ah, al
        or      ah, cl
        mov     bx, ems_frame_seg
        mov     ds, bx
        mov     bx, cs:anim_width
@@column:
        push    di
        cmp     si, 7C00h
        jb      @@mapped
        call    Anim_EMS_Map
@@mapped:
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, ah
        shr     al, 1
        shr     al, 1
        shr     al, 1
        shr     al, 1
        out     dx, al
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, ah
        and     al, 0Fh
        out     dx, al
        lodsb
        cmp     al, 0FFh
        je      @@column_done
        mov     dh, al
        lodsb
        mov     dl, al
        cmp     dh, 0
        jns     @@run
        jmp     @@packed
@@run:
        mov     cl, [si+1]
        xor     ch, ch
        mov     al, cl
        shl     cx, 1
        shl     cx, 1
        add     cl, al
        adc     ch, 0
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        add     di, cx
        lodsb
        mov     cl, al
        xor     ch, ch
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@pixel:
        lodsb
        cmp     al, 0E8h
        jae     @@remap
        stosb
        add     di, 79
        loop    @@pixel
        cmp     dx, 1
        jns     @@run
@@column_done:
        pop     di
        dec     bx
        je      @@done
        mov     cl, ah
        and     cl, 0F0h
        add     cl, 10h
        and     ah, 0Fh
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     cl, 0
        mov     ah, 1
        inc     di
@@same_byte:
        or      ah, cl
        jmp     @@column
@@done:
        ret
@@remap:
        sub     al, 0E8h
        push    si
        push    ds
        mov     si, DGROUP
        mov     ds, si
        mov     si, ax
        and     si, 0FFh
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        add     si, remap_tables_seg
        mov     ds, si
        mov     al, es:[di]
        mov     si, ax
        and     si, 0FFh
        lodsb
        pop     ds
        pop     si
        stosb
        add     di, 79
        loop    @@pixel
        cmp     dx, 1
        js      @@column_done
        jmp     @@run
@@packed:
        and     dx, 7FFFh
@@prun:
        mov     cl, [si+1]
        xor     ch, ch
        mov     al, cl
        shl     cx, 1
        shl     cx, 1
        add     cl, al
        adc     ch, 0
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        add     di, cx
        lodsb
        mov     cl, al
        xor     ch, ch
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@ppixel:
        lodsb
        cmp     al, 0E0h
        jae     @@repeat
        cmp     al, 0E8h
        jae     @@premap
        stosb
        add     di, 79
        loop    @@ppixel
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
        ret
@@premap:
        sub     al, 0E8h
        push    si
        push    ds
        mov     si, DGROUP
        mov     ds, si
        mov     si, ax
        and     si, 0FFh
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        add     si, remap_tables_seg
        mov     ds, si
        mov     al, es:[di]
        mov     si, ax
        and     si, 0FFh
        lodsb
        pop     ds
        pop     si
        stosb
        add     di, 79
        loop    @@ppixel
        cmp     dx, 1
        jns     @@packed
        jmp     @@column_done
@@repeat:
        and     al, 1Fh
        inc     al
        push    cx
        mov     cl, al
        xor     ch, ch
        lodsb
        cmp     al, 0E8h
        jae     @@rremap
@@rfill:
        stosb
        add     di, 79
        loop    @@rfill
        pop     cx
        dec     cx
        loop    @@ppixel
        cmp     dx, 1
        jns     @@prun_far
        jmp     @@column_done
@@prun_far:
        jmp     @@prun
@@rremap:
        sub     al, 0E8h
        push    si
        push    ds
        mov     si, DGROUP
        mov     ds, si
        mov     si, ax
        and     si, 0FFh
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        add     si, remap_tables_seg
        mov     ds, si
@@rremap_pixel:
        mov     al, es:[di]
        mov     si, ax
        and     si, 0FFh
        movsb
        add     di, 79
        loop    @@rremap_pixel
        pop     ds
        pop     si
        pop     cx
        dec     cx
        dec     cx
        je      @@rdone
        jmp     @@ppixel
@@rdone:
        cmp     dx, 1
        jns     @@prun_far
        jmp     @@column_done
        ret
Anim_Blit_EMS_Frame_Remap ENDP

        END
