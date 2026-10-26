; PICEMS.ASM - WIZARDS.EXE code segment 23f7 (MAGIC.EXE 226b, the same file): Draw_Pic_Clipped for pictures cached in
; EMS - the clipped column-RLE blitters of PICCLIP.ASM reading the frame straight out of the EMS page frame (4 pages
; mapped; the near helper moves the window on when the data pointer passes C000h), plain and remap variants.
; Hand-written assembly (Borland TASM). The remap variant is PICCLIP's remap procedure nearly verbatim - including the
; segment re-base of BUG #339 (here the pointer is in the EMS frame, so DS = CX is just as wrong).
; Seven WORDS IN THE CODE SEGMENT at the module start (23f7:0002).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 23f7 src/wizards/PICEMS.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   ems_frame_seg:WORD      ; DS:760c
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        EXTRN   EMS_Map_Pages:FAR
        .DATA                           ; this module's own data, DS:78a8
plane_bits      db      01h, 02h, 04h, 08h      ; DS:78a8  the VGA plane of x & 3
        .CODE
        PUBLIC  Pic_Draw_Clipped_EMS, Pic_Clipped_EMS_Map, Pic_Draw_Clipped_EMS_Remap
skip_rows_cs    dw      0               ; 23f7:0002  rows cut off at the top of every column
unused_cs       dw      0               ; 23f7:0004
draw_rows_cs    dw      0               ; 23f7:0006  rows to draw per column
columns_cs      dw      0               ; 23f7:0008  columns left to draw
ems_page        dw      0               ; 23f7:000a  first logical EMS page mapped
ems_handle      dw      0               ; 23f7:000c
anim_width      dw      0               ; 23f7:000e  picture header word 0 (not used further)

; (x, y, cols, rows, skip_cols, skip_rows, pic_seg, frame) the picture header (pic_seg:0 .. +0Ah handle, +0Bh first
; page, +0Ch offset, +11h remap flag, +12h frame offsets) locates the frame in EMS, then as Pic_Blit_Clipped_Planar
; C:  void Pic_Draw_Clipped_EMS(int x, int y, int cols, int rows, int skip_cols, int skip_rows, unsigned pic, int frame)
; C:  { (map the EMS pages of the frame); if (header remap flag) Pic_Draw_Clipped_EMS_Remap(...); else (clipped blit) }
Pic_Draw_Clipped_EMS PROC USES si di es ds, x:WORD, y:WORD, cols:WORD, rows:WORD, skip_cols:WORD, skip_rows:WORD, pseg:WORD, frame:WORD
        mov     ax, pseg
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
        push    dx
        push    bx
        call    EMS_Map_Pages
        add     sp, 4
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
        cmp     dx, 0C000h
        jb      @@page_ok
        and     dx, 3FFFh
        inc     ax
        inc     ax
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
        mov     dx, word ptr es:[0Ah]
        xor     dh, dh
        mov     cs:ems_handle, dx
        push    dx
        push    bx
        call    EMS_Map_Pages
        add     sp, 4
        cmp     di, 0
        je      @@plain
        push    skip_rows
        push    skip_cols
        push    rows
        push    cols
        push    y
        push    x
        call    far ptr Pic_Draw_Clipped_EMS_Remap
        add     sp, 0Ch
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
        mov     bx, skip_rows
        mov     cs:skip_rows_cs, bx
        mov     bx, cols
        mov     cs:columns_cs, bx
        mov     bx, rows
        mov     cs:draw_rows_cs, bx
        mov     cx, skip_cols
        mov     bx, ems_frame_seg
        mov     ds, bx
        cmp     cx, 0
        je      @@column
        mov     dx, ax
@@skip_column:
        lodsw
        cmp     al, 0FFh
        je      @@empty
        xchg    al, ah
        and     ax, 7FFFh
        add     si, ax
@@check:
        cmp     si, 0C000h
        jae     @@map
        loop    @@skip_column
        jmp     @@skipped
@@map:
        call    Pic_Clipped_EMS_Map
        loop    @@skip_column
        jmp     @@skipped
@@empty:
        dec     si
        jmp     @@check
@@skipped:
        mov     ax, dx
@@column:
        push    di
        cmp     si, 0C000h
        jb      @@mapped
        call    Pic_Clipped_EMS_Map
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
        jns     @@runs
        jmp     @@packed
@@runs:
        mov     bx, cs:skip_rows_cs
        xor     ch, ch
@@skip_run:
        cmp     bx, 0
        je      @@draw
        mov     cl, [si+1]
        sub     bx, cx
        jbe     @@part_gap
        lodsb
        mov     cl, al
        sub     bx, cx
        jb      @@part_run
        inc     si
        add     si, cx
        dec     dx
        dec     dx
        sub     dx, cx
        jne     @@skip_run
        jmp     @@column_done
@@draw:
        mov     bx, cs:draw_rows_cs
@@run:
        mov     cl, [si+1]
        xor     ch, ch
@@gap:
        sub     bx, cx
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
        dec     bx
        js      @@cut
        movsb
        add     di, 79
        loop    @@pixel
        cmp     dx, 1
        jns     @@run
@@column_done:
        pop     di
        dec     cs:columns_cs
        je      @@done
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        jmp     @@column
@@done:
        ret
@@cut:
        add     si, cx
        cmp     dx, 1
        jns     @@run
        jmp     @@column_done
@@part_gap:
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@gap
@@part_run:
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
        add     si, bx
        add     si, cx
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@pixel
@@packed:
        and     dx, 7FFFh
        mov     bx, cs:skip_rows_cs
        xor     ch, ch
@@pskip:
        cmp     bx, 0
        je      @@pdraw
        mov     cl, [si+1]
        sub     bx, cx
        js      @@ppart_gap
        lodsb
        mov     cl, al
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@pskip_pixel:
        lodsb
        cmp     al, 0E0h
        jae     @@pskip_repeat
        dec     bx
        js      @@pskip_cut
        loop    @@pskip_pixel
        cmp     dx, 1
        jns     @@pskip
        jmp     @@column_done
@@pskip_cut:
        add     cx, bx
        mov     bx, cs:draw_rows_cs
        dec     bx
        inc     cx
        jmp     @@pstore
@@pskip_repeat:
        and     al, 1Fh
        inc     al
        inc     si
        sub     bl, al
        sbb     bh, 0
        js      @@pskip_repeat_cut
        dec     cx
        loop    @@pskip_pixel
        cmp     dx, 1
        jns     @@pskip
        jmp     @@column_done
@@ppart_gap:
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@pgap
@@pskip_repeat_cut:
        dec     si
        lodsb
        push    cx
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@prepeat_pixel
@@pdraw:
        mov     bx, cs:draw_rows_cs
@@prun:
        mov     cl, [si+1]
        xor     ch, ch
@@pgap:
        sub     bx, cx
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
        dec     bx
        js      @@pcut
        lodsb
        cmp     al, 0E0h
        jae     @@prepeat
@@pstore:
        stosb
        add     di, 79
        loop    @@ppixel
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
@@prepeat:
        and     al, 1Fh
        inc     al
        push    cx
        mov     cl, al
        xor     ch, ch
        lodsb
        inc     bx
@@prepeat_pixel:
        dec     bx
        js      @@prepeat_cut
        stosb
        add     di, 79
        loop    @@prepeat_pixel
        pop     cx
        dec     cx
        loop    @@ppixel
        cmp     dx, 1
        jns     @@prun
        pop     di
        dec     cs:columns_cs
        je      @@done2
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte2
        mov     ah, 1
        inc     di
@@same_byte2:
        jmp     @@column
@@done2:
        ret
@@prepeat_cut:
        pop     cx
        dec     cx
        dec     cx
@@pcut:
        add     si, cx
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
Pic_Draw_Clipped_EMS ENDP

; near register helper: the data pointer SI passed C000h - map the next 4 logical pages (ems_page + 3 ..) and move SI
; back 48 KB; keeps AX BX DX
; C:  static void Pic_Clipped_EMS_Map(void) { ems_page += 3; EMS_Map_Pages(ems_page, ems_handle); src -= 0xC000;
; C:      ems_page++; }
Pic_Clipped_EMS_Map PROC NEAR
        push    bx
        push    ax
        push    dx
        mov     bx, cs:ems_page
        inc     bx
        inc     bx
        inc     bx
        mov     cs:ems_page, bx
        mov     dx, cs:ems_handle
        push    dx
        push    bx
        call    EMS_Map_Pages
        add     sp, 4
        sub     si, 0C000h
        inc     cs:ems_page
        pop     dx
        pop     ax
        pop     bx
        ret
Pic_Clipped_EMS_Map ENDP

; (x, y, cols, rows, skip_cols, skip_rows) the remap variant, called by Pic_Draw_Clipped_EMS with SI at the frame
; data in the EMS frame (no EMS window moves here - only the copied FC00h re-base)
; C:  as Pic_Draw_Clipped_EMS, pixel p >= 0xE8: screen = remap[p - 0xE8][screen]
Pic_Draw_Clipped_EMS_Remap PROC USES si di es ds, rx:WORD, ry:WORD, rcols:WORD, rrows:WORD, rskip_cols:WORD, rskip_rows:WORD
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
        mov     bl, al
        shl     bl, 1
        shl     bl, 1
        shl     bl, 1
        shl     bl, 1
        mov     cx, si
        mov     si, offset plane_bits
        add     si, ax
        lodsb
        mov     ah, al
        or      ah, bl
        mov     si, cx
        mov     bx, rskip_rows
        mov     cs:skip_rows_cs, bx
        mov     bx, rcols
        mov     cs:columns_cs, bx
        mov     bx, rrows
        mov     cs:draw_rows_cs, bx
        mov     cx, rskip_cols
        mov     bx, ems_frame_seg
        mov     ds, bx
        cmp     cx, 0
        je      @@column
        mov     dx, ax
@@skip_column:
        lodsw
        cmp     al, 0FFh
        je      @@empty
        xchg    al, ah
        and     ax, 7FFFh
        add     si, ax
        loop    @@skip_column
        jmp     @@skipped
@@empty:
        dec     si
        loop    @@skip_column
@@skipped:
        mov     ax, dx
@@column:
        push    di
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
        cmp     si, 0FC00h
        jb      @@seg_ok
        mov     ds, cx
        add     cx, 0FC0h
        mov     cx, ds
        sub     si, 0FC00h
@@seg_ok:
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
        jns     @@runs
        jmp     @@packed
@@part_gap:
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@gap
@@runs:
        mov     bx, cs:skip_rows_cs
        xor     ch, ch
@@skip_run:
        cmp     bx, 0
        je      @@draw
        mov     cl, [si+1]
        sub     bx, cx
        jbe     @@part_gap
        lodsb
        mov     cl, al
        sub     bx, cx
        jb      @@part_run
        inc     si
        add     si, cx
        dec     dx
        dec     dx
        sub     dx, cx
        jne     @@skip_run
        jmp     @@column_done
@@draw:
        mov     bx, cs:draw_rows_cs
@@run:
        mov     cl, [si+1]
        xor     ch, ch
@@gap:
        sub     bx, cx
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
        dec     bx
        js      @@cut
        lodsb
        cmp     al, 0E8h
        jae     @@remap
        stosb
@@next:
        add     di, 79
        loop    @@pixel
        cmp     dx, 1
        jns     @@run
@@column_done:
        pop     di
        dec     cs:columns_cs
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
@@cut:
        add     si, cx
        cmp     dx, 1
        jns     @@run
        jmp     @@column_done
@@part_run:
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
        add     si, bx
        add     si, cx
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@pixel
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
        movsb
        pop     ds
        pop     si
        jmp     @@next
@@packed:
        and     dx, 7FFFh
        mov     bx, cs:skip_rows_cs
        xor     ch, ch
@@pskip:
        cmp     bx, 0
        je      @@pdraw
        mov     cl, [si+1]
        sub     bx, cx
        js      @@ppart_gap
        lodsb
        mov     cl, al
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@pskip_pixel:
        lodsb
        cmp     al, 0E0h
        jae     @@pskip_repeat
        dec     bx
        js      @@pskip_cut
        loop    @@pskip_pixel
        cmp     dx, 1
        jns     @@pskip
        jmp     @@column_done
@@pskip_cut:
        add     cx, bx
        mov     bx, cs:draw_rows_cs
        dec     bx
        inc     cx
        jmp     @@pstore
@@pskip_repeat:
        and     al, 1Fh
        inc     al
        inc     si
        sub     bl, al
        sbb     bh, 0
        js      @@pskip_repeat_cut
        dec     cx
        loop    @@pskip_pixel
        cmp     dx, 1
        jns     @@pskip
        jmp     @@column_done
@@ppart_gap:
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@pgap
@@pskip_repeat_cut:
        dec     si
        lodsb
        push    cx
        neg     bx
        mov     cx, bx
        mov     bx, cs:draw_rows_cs
        jmp     @@prepeat_in
@@pdraw:
        mov     bx, cs:draw_rows_cs
@@prun:
        mov     cl, [si+1]
        xor     ch, ch
@@pgap:
        sub     bx, cx
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
        dec     bx
        js      @@pcut
        lodsb
        cmp     al, 0E0h
        jae     @@prepeat
@@pstore:
        cmp     al, 0E8h
        jae     @@premap
        stosb
@@pnext:
        add     di, 79
        loop    @@ppixel
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
@@prepeat:
        and     al, 1Fh
        inc     al
        push    cx
        mov     cl, al
        xor     ch, ch
        lodsb
        inc     bx
@@prepeat_in:
        cmp     al, 0E8h
        jae     @@prepeat_remap
@@prepeat_pixel:
        dec     bx
        js      @@prepeat_cut
        stosb
        add     di, 79
        loop    @@prepeat_pixel
@@prepeat_end:
        pop     cx
        dec     cx
        loop    @@ppixel
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
@@prepeat_cut_ds:
        pop     ds
        pop     si
@@prepeat_cut:
        pop     cx
        dec     cx
        dec     cx
@@pcut:
        add     si, cx
        cmp     dx, 1
        jns     @@prun
        jmp     @@column_done
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
        movsb
        pop     ds
        pop     si
        jmp     @@pnext
@@prepeat_remap:
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
@@prepeat_remap_pixel:
        dec     bx
        js      @@prepeat_cut_ds
        mov     al, es:[di]
        mov     si, ax
        and     si, 0FFh
        movsb
        add     di, 79
        loop    @@prepeat_remap_pixel
        pop     ds
        pop     si
        jmp     @@prepeat_end
Pic_Draw_Clipped_EMS_Remap ENDP

        END
