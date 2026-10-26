; PICCLIP.ASM - WIZARDS.EXE code segment 23a4 (MAGIC.EXE 2218, the same file): the clipped picture blitters behind
; Draw_Pic_Clipped - a column-RLE frame drawn with columns and rows cut off at the left / top, plain and translucent
; (remap) variants. Hand-written assembly (Borland TASM).
; Frame data (per column): FFh = empty column, else a word (high byte first) = the column's byte length, bit 15 = the
; column holds packed runs; then runs {count, skip}: skip rows, count pixels. In packed columns E0h..FFh are repeat
; codes ((code & 1Fh) + 1 copies of the next byte); in the remap variant pixels E8h..FFh are remap-table numbers.
; Four WORDS IN THE CODE SEGMENT at the module start (23a4:000a): the rows to skip, (unused), the rows to draw and the
; column count.
; BUG #339 (both procedures): when the data pointer passes FC00h the segment re-base is written 'mov ds, cx / add cx,
; 0FC0h / mov cx, ds' - DS gets CX (the loop counter), not DS + 0FC0h.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 23a4 src/wizards/PICCLIP.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        .DATA                           ; this module's own data, DS:78a4
plane_bits      db      01h, 02h, 04h, 08h      ; DS:78a4  the VGA plane of x & 3
        .CODE
        PUBLIC  Pic_Blit_Clipped_Planar, Pic_Blit_Clipped_Planar_Remap
skip_rows_cs    dw      0               ; 23a4:000a  rows cut off at the top of every column
unused_cs       dw      0               ; 23a4:000c
draw_rows_cs    dw      0               ; 23a4:000e  rows to draw per column
columns_cs      dw      0               ; 23a4:0010  columns left to draw

; (x, y, cols, rows, skip_cols, skip_rows, data, data_seg) draws cols columns of rows rows from the frame data, after
; skipping skip_cols whole columns and skip_rows rows of each column
; C:  void Pic_Blit_Clipped_Planar(int x, int y, int cols, int rows, int skip_cols, int skip_rows, char *data, unsigned seg)
; C:  { (skip skip_cols columns); for c < cols: decode the column's runs, drop the first skip_rows rows, stop after
; C:    rows rows, write pixels (packed: repeat codes) at (x + c, y + row) }
Pic_Blit_Clipped_Planar PROC USES si di es ds, x:WORD, y:WORD, cols:WORD, rows:WORD, skip_cols:WORD, skip_rows:WORD, data:WORD, dseg:WORD
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
        mov     si, offset plane_bits
        add     si, ax
        lodsb
        mov     ah, al
        mov     bx, skip_rows
        mov     cs:skip_rows_cs, bx
        mov     bx, cols
        mov     cs:columns_cs, bx
        mov     bx, rows
        mov     cs:draw_rows_cs, bx
        mov     cx, skip_cols
        mov     dx, dseg
        mov     si, data
        mov     ds, dx
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
Pic_Blit_Clipped_Planar ENDP

; (x, y, cols, rows, skip_cols, skip_rows, data, data_seg) the translucent variant: pixels E8h..FFh replace the screen
; pixel by remap table (pixel - E8h) of it (read through the VGA read map, 3CEh index 4)
; C:  as Pic_Blit_Clipped_Planar, pixel p >= 0xE8: screen = remap[p - 0xE8][screen]
Pic_Blit_Clipped_Planar_Remap PROC USES si di es ds, rx:WORD, ry:WORD, rcols:WORD, rrows:WORD, rskip_cols:WORD, rskip_rows:WORD, rdata:WORD, rdseg:WORD
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
        mov     si, offset plane_bits
        add     si, ax
        lodsb
        mov     ah, al
        or      ah, bl
        mov     bx, rskip_rows
        mov     cs:skip_rows_cs, bx
        mov     bx, rcols
        mov     cs:columns_cs, bx
        mov     bx, rrows
        mov     cs:draw_rows_cs, bx
        mov     cx, rskip_cols
        mov     dx, rdseg
        mov     si, rdata
        mov     ds, dx
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
Pic_Blit_Clipped_Planar_Remap ENDP

        END
