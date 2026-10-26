; PICDEC.ASM - WIZARDS.EXE code segment 2459 (MAGIC.EXE 22cd, the same file): bitmap helpers in assembly - the
; bitmap header, decoding an LBX frame (column RLE) into a bitmap (conventional memory or EMS), the planar blit of a
; bitmap (plain / remap), bitmap-onto-bitmap copy and mask, mirror, color replace, the brightness table, memset,
; shrink / enlarge by percent, the dissolve, the three shade remaps and the module's own random generator.
; Hand-written assembly (Borland TASM). Code-segment words between the procedures (2459:0048, 00ee-00f2, 024d-0251,
; 056b-056f, 06e3 + the 256-byte dissolve table, 09e3-09e5).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 2459 src/wizards/PICDEC.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   ems_frame_seg:WORD      ; DS:760c
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        EXTRN   p_palette:WORD          ; DS:e7e8
        EXTRN   shade_table_block:WORD  ; DS:e7fa
        EXTRN   EMS_Map_Pages:FAR
        EXTRN   Random_LFSR8:FAR
        .DATA                           ; this module's own data, DS:78ac
unused_ds       dw      0               ; DS:78ac
plane_bits      db      01h, 02h, 04h, 08h      ; DS:78ae  the VGA plane of x & 3
unused_ds2      dw      0               ; DS:78b2
        .CODE
        PUBLIC  Pic_Header_Write, Pic_Decode_Frame, Pic_Decode_Frame_EMS, Pic_EMS_Next_Pages, Pic_Blit_Planar
        PUBLIC  Pic_Blit_Planar_Inner, Pic_Copy_Into_Pic_Core, Pic_Mirror, Pic_Replace_Color, Pic_Brightness_Table
        PUBLIC  Pic_Cut_To_Mask_Core, Pic_Mem_Clear_Far, Pic_Shrink, Pic_Enlarge, Pic_Dissolve, Pic_Remap_Shade
        PUBLIC  Pic_Monochrome_Remap, Pic_Remap_Shade2, Pic_Random_Mod

; (src, dst, value) bitmap header at dst:0 from src: width, height, 0DE0Ah, value, 0 (remap flag); then clears the
; width * height pixels at dst:10h (a word count rounded up)
; C:  void Pic_Header_Write(unsigned src, unsigned dst, int value)
; C:  { w = peek(src, 0); h = peek(src, 2); poke(dst, 0, w); poke(dst, 2, h); poke(dst, 4, 0xDE0A);
; C:    poke(dst, 6, value); poke(dst, 8, 0); _fmemset(MK_FP(dst, 16), 0, ((w * h + 1) >> 1) * 2); }
; Simtex name Create_Multi_Frame_Blank_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo
Pic_Header_Write PROC USES si di es ds, hsrc:WORD, hdst:WORD, hvalue:WORD
        mov     ax, hsrc
        mov     bx, hdst
        mov     cx, hvalue
        mov     ds, ax
        mov     es, bx
        mov     si, 0
        mov     di, 0
        movsw
        movsw
        mov     ax, 0DE0Ah
        stosw
        mov     ax, cx
        stosw
        mov     ax, 0
        stosw
        mov     di, 10h
        mov     si, 0
        lodsw
        mov     bx, ax
        lodsw
        mul     bx
        mov     cx, ax
        inc     cx
        shr     cx, 1
        xor     ax, ax
        rep     stosw
        ret
Pic_Header_Write ENDP

dec_columns     dw      0               ; 2459:0048  columns left (Pic_Decode_Frame)

; (frame_ofs, frame_seg, bitmap) decodes one LBX frame into the bitmap (column-major, stride = bitmap height): per
; column FFh = empty, else a header word (bit 15 = RLE) = bytes in the column, then runs [count, skip, data]; in an
; RLE run a byte E0h + n repeats the next byte n + 1 times
; C:  void Pic_Decode_Frame(unsigned frame_ofs, unsigned frame_seg, unsigned bitmap) { (as described) }
; Simtex name Add_Picture_To_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo
Pic_Decode_Frame PROC USES si di es ds, fofs:WORD, fseg:WORD, fbitmap:WORD
        mov     ax, fbitmap
        mov     es, ax
        mov     di, 10h
        mov     si, fofs
        mov     bx, fseg
        mov     ds, bx
        mov     ax, word ptr es:[0]
        mov     cs:dec_columns, ax
@@column:
        push    di
        lodsb
        cmp     al, 0FFh
        je      @@next_column
        mov     dh, al
        lodsb
        mov     dl, al
        cmp     dh, 0
        js      @@rle
@@copy_run:
        mov     cl, [si+1]
        xor     ch, ch
        add     di, cx
        lodsb
        mov     cl, al
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
        rep     movsb
        cmp     dx, 1
        jns     @@copy_run
@@next_column:
        pop     di
        add     di, word ptr es:[2]
        dec     cs:dec_columns
        jne     @@column
        ret
@@rle:
        and     dx, 7FFFh
@@rle_run:
        mov     cl, [si+1]
        xor     ch, ch
        add     di, cx
        lodsb
        mov     cl, al
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@rle_pixel:
        lodsb
        cmp     al, 0E0h
        jae     @@rle_repeat
        stosb
        loop    @@rle_pixel
        cmp     dx, 1
        jns     @@rle_run
        pop     di
        add     di, word ptr es:[2]
        dec     cs:dec_columns
        jne     @@column
        ret
@@rle_repeat:
        and     al, 1Fh
        inc     al
        mov     bx, cx
        mov     cl, al
        xor     ch, ch
        lodsb
        rep     stosb
        mov     cx, bx
        dec     cx
        loop    @@rle_pixel
        cmp     dx, 1
        jns     @@rle_run
        jmp     @@next_column
Pic_Decode_Frame ENDP

ems_page        dw      0               ; 2459:00ee  first logical EMS page mapped
ems_handle      dw      0               ; 2459:00f0
ems_columns     dw      0               ; 2459:00f2  columns left (Pic_Decode_Frame_EMS)

; (pic, frame, bitmap) Pic_Decode_Frame for a picture held in EMS: header +0Ah handle, +0Bh first page, +0Ch offset;
; maps 4 pages at the frame and moves the window on when the data pointer passes C000h (Pic_EMS_Next_Pages)
; C:  void Pic_Decode_Frame_EMS(unsigned pic, int frame, unsigned bitmap) { (as described) }
Pic_Decode_Frame_EMS PROC USES si di es ds, epic:WORD, eframe:WORD, ebitmap:WORD
        mov     ax, epic
        mov     ds, ax
        mov     es, ax
        mov     ax, word ptr ds:[0]
        mov     cs:ems_columns, ax
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
        mov     ax, eframe
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
        jb      @@mapped
        and     dx, 3FFFh
        inc     ax
        inc     ax
        inc     ax
@@mapped:
        mov     cs:ems_page, ax
        mov     si, dx
        mov     bx, ax
        mov     dx, word ptr es:[0Ah]
        xor     dh, dh
        mov     cs:ems_handle, dx
        push    dx
        push    bx
        call    EMS_Map_Pages
        add     sp, 4
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, ebitmap
        mov     es, ax
        mov     di, 10h
        mov     bx, ems_frame_seg
        mov     ds, bx
@@column:
        push    di
        cmp     si, 0C000h
        jb      @@in_window
        call    Pic_EMS_Next_Pages
@@in_window:
        lodsb
        cmp     al, 0FFh
        je      @@next_column
        mov     dh, al
        lodsb
        mov     dl, al
        cmp     dh, 0
        js      @@rle
@@copy_run:
        mov     cl, [si+1]
        xor     ch, ch
        add     di, cx
        lodsb
        mov     cl, al
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
        rep     movsb
        cmp     dx, 1
        jns     @@copy_run
@@next_column:
        pop     di
        add     di, word ptr es:[2]
        dec     cs:ems_columns
        jne     @@column
        ret
@@rle:
        and     dx, 7FFFh
@@rle_run:
        mov     cl, [si+1]
        xor     ch, ch
        add     di, cx
        lodsb
        mov     cl, al
        inc     si
        dec     dx
        dec     dx
        sub     dx, cx
@@rle_pixel:
        lodsb
        cmp     al, 0E0h
        jae     @@rle_repeat
        stosb
        loop    @@rle_pixel
        cmp     dx, 1
        jns     @@rle_run
        pop     di
        add     di, word ptr es:[2]
        dec     cs:ems_columns
        jne     @@column
        ret
@@rle_repeat:
        and     al, 1Fh
        inc     al
        mov     bx, cx
        mov     cl, al
        xor     ch, ch
        lodsb
        rep     stosb
        mov     cx, bx
        dec     cx
        loop    @@rle_pixel
        cmp     dx, 1
        jns     @@rle_run
        jmp     @@next_column
Pic_Decode_Frame_EMS ENDP

; near register helper: SI passed C000h - map the next 4 logical pages and move SI back 48 KB. Unlike PICEMS's
; Pic_Clipped_EMS_Map it never loads ems_handle: the handle pushed is whatever DX holds (#399)
; Numbered: BUG #399 (BUGS_AND_QUIRKS.txt)
; C:  static void Pic_EMS_Next_Pages(void) { ems_page += 3; EMS_Map_Pages(ems_page, (DX)); src -= 0xC000;
; C:      ems_page++; }
Pic_EMS_Next_Pages PROC NEAR
        push    bx
        push    ax
        mov     bx, cs:ems_page
        add     bx, 3
        mov     cs:ems_page, bx
        push    dx
        push    bx
        call    EMS_Map_Pages
        add     sp, 4
        sub     si, 0C000h
        inc     cs:ems_page
        pop     ax
        pop     bx
        ret
Pic_EMS_Next_Pages ENDP

blit_rows       dw      0               ; 2459:024d  pixels per column
blit_columns    dw      0               ; 2459:024f  columns left
blit_skip       dw      0               ; 2459:0251  source bytes skipped after each column

; (x, y, data_ofs, data_seg, cols, rows, skip) draws a column-major bitmap to the draw page (unclipped), color 0
; transparent; data_seg:8 (the bitmap's remap flag) set -> Pic_Blit_Planar_Inner (colors E8h+ through the remap
; tables)
; C:  void Pic_Blit_Planar(int x, int y, unsigned ofs, unsigned seg, int cols, int rows, int skip) { (as described) }
Pic_Blit_Planar PROC USES si di es ds, bx_:WORD, by:WORD, bofs:WORD, bseg:WORD, bcols:WORD, brows:WORD, bskip:WORD
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, by
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, bx_
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
        mov     cx, brows
        mov     cs:blit_rows, cx
        mov     cx, bskip
        mov     cs:blit_skip, cx
        mov     si, bofs
        mov     bx, bcols
        mov     cx, bseg
        mov     ds, cx
        cmp     byte ptr ds:[8], 0
        je      @@plain
        call    Pic_Blit_Planar_Inner
        ret
@@plain:
        and     ah, 0Fh
@@column:
        push    di
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     cx, cs:blit_rows
@@pixel:
        lodsb
        cmp     al, 0
        je      @@transparent
        stosb
        add     di, 4Fh
        loop    @@pixel
        add     si, cs:blit_skip
        pop     di
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        dec     bx
        jne     @@column
        ret
@@transparent:
        add     di, 50h
        loop    @@pixel
        add     si, cs:blit_skip
        pop     di
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte2
        mov     ah, 1
        inc     di
@@same_byte2:
        dec     bx
        jne     @@column
        ret
Pic_Blit_Planar ENDP

; near: the remap column loop of Pic_Blit_Planar - per column sets the read map (3CEh index 4, AH high nibble)
; and the map mask (AH low nibble); pixel 0 skipped, < E8h stored, >= E8h replaces the screen pixel through
; remap table (pixel - E8h)
Pic_Blit_Planar_Inner PROC NEAR
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
        mov     cx, cs:blit_rows
@@pixel:
        lodsb
        cmp     al, 0
        je      @@transparent
        cmp     al, 0E8h
        jae     @@remap
        stosb
        add     di, 4Fh
        loop    @@pixel
        add     si, cs:blit_skip
        pop     di
        dec     bx
        je      @@done
@@next_plane:
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
@@transparent:
        add     di, 50h
        loop    @@pixel
        add     si, cs:blit_skip
        pop     di
        dec     bx
        jne     @@next_plane
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
        add     di, 4Fh
        loop    @@pixel
        add     si, cs:blit_skip
        pop     di
        dec     bx
        je      @@done
        mov     cl, ah
        and     cl, 0F0h
        add     cl, 10h
        and     ah, 0Fh
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte2
        mov     cl, 0
        mov     ah, 1
        inc     di
@@same_byte2:
        or      ah, cl
        jmp     @@column
Pic_Blit_Planar_Inner ENDP

; (dst_ofs, dst_seg, src_ofs, src_seg, dst_skip, src_skip, cols, rows) copies the non-zero source pixels
; C:  void Pic_Copy_Into_Pic_Core(unsigned dofs, unsigned dseg, unsigned sofs, unsigned sseg, int dskip, int sskip,
; C:      int cols, int rows) { for each of cols columns: rows pixels, color 0 skipped; then skip both strides }
; Simtex name Mask_Bitmap_Pixels (MoO2 symbols, module shear); clarified using ReMoM repo
Pic_Copy_Into_Pic_Core PROC USES si di es ds, cdofs:WORD, cdseg:WORD, csofs:WORD, csseg:WORD, cdskip:WORD, csskip:WORD, ccols:WORD, crows:WORD
        mov     di, cdofs
        mov     si, csofs
        mov     bx, cdskip
        mov     dx, csskip
        mov     ax, crows
        mov     cs:blit_rows, ax
        mov     ax, ccols
        mov     cs:blit_columns, ax
        mov     ax, cdseg
        mov     es, ax
        mov     ax, csseg
        mov     ds, ax
@@column:
        mov     cx, cs:blit_rows
@@pixel:
        lodsb
        inc     di
        cmp     al, 0
        je      @@skip
        dec     di
        stosb
@@skip:
        loop    @@pixel
        add     di, bx
        add     si, dx
        dec     cs:blit_columns
        jne     @@column
        ret
Pic_Copy_Into_Pic_Core ENDP

; (pic) mirrors the bitmap left - right in place
; Simtex name Flip_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo
Pic_Mirror PROC USES si di es ds, mpic:WORD
        mov     ax, mpic
        mov     ds, ax
        mov     es, ax
        mov     bx, word ptr ds:[0]
        mov     cx, word ptr ds:[2]
        mov     ax, bx
        dec     ax
        mul     cx
        add     ax, 10h
        mov     si, ax
        mov     di, 10h
        shr     bx, 1
        mov     dx, cx
@@column:
        mov     cx, dx
@@pixel:
        lodsb
        mov     ah, es:[di]
        stosb
        dec     si
        mov     es:[si], ah
        inc     si
        loop    @@pixel
        sub     si, dx
        sub     si, dx
        dec     bx
        jne     @@column
        ret
Pic_Mirror ENDP

; (pic, from, to) replaces color from by to in the whole bitmap
; Simtex name Replace_Color (MoO2 symbols, module replace); clarified using ReMoM repo
Pic_Replace_Color PROC USES si di es ds, rpic:WORD, rfrom:WORD, rto:WORD
        mov     bl, byte ptr rfrom
        mov     bh, byte ptr rto
        mov     ax, rpic
        mov     ds, ax
        mov     es, ax
        mov     ax, word ptr ds:[0]
        mov     dx, word ptr ds:[2]
        mul     dx
        mov     cx, ax
        mov     si, 10h
        mov     di, 10h
@@pixel:
        lodsb
        inc     di
        cmp     al, bl
        jne     @@next
        dec     di
        mov     al, bh
        stosb
@@next:
        loop    @@pixel
        ret
Pic_Replace_Color ENDP

; (pic, table[256]) table[c] = brightness R + G + B / 2 (palette p_palette) of every color the bitmap uses
; (color 0 excluded), FFh for the others
Pic_Brightness_Table PROC USES si di es ds, tpic:WORD, ttable:WORD
        mov     ax, ds
        mov     es, ax
        mov     di, ttable
        mov     bx, di
        mov     al, 0FFh
        mov     cx, 100h
        rep     stosb
        mov     ax, tpic
        mov     ds, ax
        mov     si, 10h
        mov     ax, word ptr ds:[0]
        mov     dx, word ptr ds:[2]
        mul     dx
        mov     cx, ax
        xor     ah, ah
@@pixel:
        lodsb
        cmp     al, 0
        je      @@next
        mov     di, bx
        add     di, ax
        xor     al, al
        stosb
@@next:
        loop    @@pixel
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     si, 0
        mov     cx, 100h
        mov     di, bx
@@color:
        mov     al, es:[di]
        cmp     al, 0FFh
        je      @@unused
        lodsb
        mov     ah, al
        lodsb
        add     ah, al
        lodsb
        shr     al, 1
        add     ah, al
        mov     es:[di], ah
        inc     di
        loop    @@color
        ret
@@unused:
        inc     di
        add     si, 3
        loop    @@color
        ret
Pic_Brightness_Table ENDP

; (dst_ofs, dst_seg, mask_ofs, mask_seg, dst_skip, mask_skip, cols, rows) clears the pixels where the mask is 0
; Simtex name Mask_Bitmap_Pixels_Inverse (MoO2 symbols, module shear); clarified using ReMoM repo
Pic_Cut_To_Mask_Core PROC USES si di es ds, kdofs:WORD, kdseg:WORD, ksofs:WORD, ksseg:WORD, kdskip:WORD, ksskip:WORD, kcols:WORD, krows:WORD
        mov     di, kdofs
        mov     si, ksofs
        mov     bx, kdskip
        mov     dx, ksskip
        mov     ax, krows
        mov     cs:blit_rows, ax
        mov     ax, kcols
        mov     cs:blit_columns, ax
        mov     ax, kdseg
        mov     es, ax
        mov     ax, ksseg
        mov     ds, ax
@@column:
        mov     cx, cs:blit_rows
@@pixel:
        lodsb
        inc     di
        cmp     al, 0
        jne     @@keep
        dec     di
        stosb
@@keep:
        loop    @@pixel
        add     di, bx
        add     si, dx
        dec     cs:blit_columns
        jne     @@column
        ret
Pic_Cut_To_Mask_Core ENDP

; (ofs, seg, count) count zero bytes at seg:ofs
Pic_Mem_Clear_Far PROC USES si di es ds, zofs:WORD, zseg:WORD, zcount:WORD
        mov     cx, zcount
        mov     di, zofs
        mov     ax, zseg
        mov     es, ax
        xor     al, al
        rep     stosb
        ret
Pic_Mem_Clear_Far ENDP

scale_frac      dw      0               ; 2459:056b  8.8 column accumulator
scale_step      dw      0               ; 2459:056d  column step (25600 / percent)
scale_height    dw      0               ; 2459:056f  the old height (source column stride)

; (pic, x_percent, y_percent) shrinks the bitmap in place (forward copy, 8.8 steps); writes the new size
; Simtex name Reduce_Bitmap (MoO2 symbols, module shear); clarified using ReMoM repo
Pic_Shrink PROC USES si di es ds, spic:WORD, sxpct:WORD, sypct:WORD
        LOCAL   ystep:WORD
        mov     ax, spic
        mov     es, ax
        mov     ax, word ptr es:[0]
        mov     bx, sxpct
        mul     bx
        mov     cx, 64h
        div     cx
        mov     word ptr es:[0], ax
        mov     ax, 6400h
        xor     dx, dx
        div     bx
        mov     cs:scale_step, ax
        mov     ax, word ptr es:[2]
        mov     cs:scale_height, ax
        mov     bx, sypct
        mul     bx
        mov     cx, 64h
        div     cx
        mov     word ptr es:[2], ax
        mov     ax, 6400h
        mov     dx, 0
        div     bx
        mov     ystep, ax
        mov     si, 10h
        mov     di, 10h
        mov     bx, word ptr es:[0]
        mov     cs:blit_columns, bx
        mov     cs:scale_frac, 0
        mov     dx, ystep
        mov     ax, es
        mov     ds, ax
@@column:
        push    si
        mov     cx, word ptr ds:[2]
        mov     bx, 0
@@pixel:
        movsb
        dec     si
        add     bx, dx
        mov     al, bh
        xor     bh, bh
        xor     ah, ah
        add     si, ax
        loop    @@pixel
        pop     si
        mov     cx, cs:scale_frac
        add     cx, cs:scale_step
        mov     ax, cs:scale_height
        mul     ch
        add     si, ax
        xor     ch, ch
        mov     cs:scale_frac, cx
        dec     cs:blit_columns
        jne     @@column
        ret
Pic_Shrink ENDP

; (pic, x_percent, y_percent) enlarges the bitmap in place, copying backwards from the end
; Simtex name Enlarge_Bitmap (MoO2 symbols, module shear); clarified using ReMoM repo
Pic_Enlarge PROC USES si di es ds, epic2:WORD, expct:WORD, eypct:WORD
        LOCAL   eystep:WORD
        mov     ax, epic2
        mov     es, ax
        mov     ax, word ptr es:[0]
        mov     bx, word ptr es:[2]
        mul     bx
        add     ax, 0Fh
        mov     si, ax
        mov     ax, word ptr es:[0]
        mov     bx, expct
        mul     bx
        mov     cx, 64h
        div     cx
        mov     word ptr es:[0], ax
        mov     ax, 6400h
        xor     dx, dx
        div     bx
        mov     cs:scale_step, ax
        mov     ax, word ptr es:[2]
        mov     cs:scale_height, ax
        mov     bx, eypct
        mul     bx
        mov     cx, 64h
        div     cx
        mov     word ptr es:[2], ax
        mov     ax, 6400h
        mov     dx, 0
        div     bx
        mov     eystep, ax
        mov     ax, word ptr es:[0]
        mov     bx, word ptr es:[2]
        mul     bx
        add     ax, 0Fh
        mov     di, ax
        mov     bx, word ptr es:[0]
        mov     cs:blit_columns, bx
        mov     cs:scale_frac, 0
        mov     dx, eystep
        mov     ax, es
        mov     ds, ax
@@column:
        push    si
        mov     cx, word ptr ds:[2]
        mov     bx, 0
@@pixel:
        movsb
        dec     di
        dec     di
        dec     si
        add     bx, dx
        mov     al, bh
        xor     bh, bh
        xor     ah, ah
        sub     si, ax
        loop    @@pixel
        pop     si
        mov     cx, cs:scale_frac
        add     cx, cs:scale_step
        cmp     ch, 0
        je      @@same_column
        sub     si, cs:scale_height
        xor     ch, ch
@@same_column:
        mov     cs:scale_frac, cx
        dec     cs:blit_columns
        jne     @@column
        ret
Pic_Enlarge ENDP

dissolve_seed   dw      0               ; 2459:06e3
dissolve_table  db      2Ah, 44h, 23h, 01h, 46h, 19h, 4Fh, 3Bh, 3Fh, 41h, 06h, 2Eh, 52h, 1Ch, 3Eh, 5Ch    ; 2459:06e5  thresholds 1..100
        db      60h, 2Bh, 1Ch, 25h, 5Ch, 05h, 03h, 36h, 5Dh, 53h, 16h, 11h, 13h, 60h, 30h, 1Bh
        db      48h, 27h, 46h, 0Dh, 44h, 64h, 24h, 5Fh, 04h, 0Ch, 17h, 22h, 4Ah, 41h, 2Ah, 0Ch
        db      36h, 45h, 30h, 2Dh, 3Fh, 3Ah, 26h, 3Ch, 18h, 2Ah, 1Eh, 4Fh, 11h, 24h, 5Bh, 2Bh
        db      59h, 07h, 29h, 2Bh, 41h, 31h, 2Fh, 06h, 5Bh, 1Eh, 47h, 33h, 07h, 02h, 5Eh, 31h
        db      1Eh, 18h, 55h, 37h, 39h, 29h, 43h, 4Dh, 20h, 09h, 2Dh, 28h, 1Bh, 18h, 26h, 27h
        db      13h, 53h, 1Eh, 2Ah, 22h, 10h, 28h, 3Bh, 05h, 1Fh, 4Eh, 07h, 4Ah, 57h, 16h, 2Eh
        db      19h, 49h, 47h, 1Eh, 4Eh, 4Ah, 62h, 0Dh, 57h, 5Bh, 3Eh, 25h, 38h, 44h, 38h, 4Bh
        db      20h, 35h, 33h, 33h, 2Ah, 19h, 43h, 1Fh, 08h, 5Ch, 08h, 26h, 3Ah, 58h, 36h, 54h
        db      2Eh, 0Ah, 0Ah, 3Bh, 16h, 59h, 17h, 2Fh, 07h, 1Fh, 0Eh, 45h, 01h, 5Ch, 3Fh, 38h
        db      0Bh, 3Ch, 19h, 26h, 31h, 54h, 60h, 2Ah, 03h, 33h, 5Ch, 25h, 4Bh, 15h, 61h, 16h
        db      31h, 64h, 45h, 55h, 52h, 23h, 36h, 64h, 13h, 27h, 01h, 59h, 1Ch, 44h, 1Dh, 5Eh
        db      31h, 54h, 08h, 16h, 0Bh, 12h, 0Eh, 0Fh, 0Ah, 11h, 24h, 34h, 01h, 32h, 14h, 39h
        db      63h, 04h, 19h, 09h, 2Dh, 0Ah, 5Ah, 03h, 60h, 56h, 5Eh, 2Ch, 18h, 58h, 0Fh, 04h
        db      31h, 01h, 3Bh, 13h, 51h, 61h, 63h, 52h, 5Ah, 63h, 0Ah, 3Ah, 49h, 17h, 27h, 5Dh
        db      27h, 50h, 5Bh, 3Ah, 3Bh, 5Ch, 10h, 59h, 39h, 0Ch, 03h, 23h, 49h, 38h, 1Dh, 2Fh

; (pic, level) clears every pixel whose dissolve threshold (table, walked from a random start per column) is above
; level 0..100
Pic_Dissolve PROC USES si di es ds, dpic:WORD, dlevel:WORD
        mov     ax, 3E8h
        push    ax
        push    cs                      ; the far call to a later procedure, as TASM's forward far call assembles
        call    near ptr Pic_Random_Mod
        nop
        add     sp, 2
        mov     cs:dissolve_seed, ax
        mov     ax, dpic
        mov     es, ax
        mov     di, 10h
        mov     ax, word ptr es:[0]
        mov     cs:blit_columns, ax
        mov     dx, dlevel
        mov     ax, cs
        mov     ds, ax
@@column:
        mov     ax, cs:dissolve_seed
        call    Random_LFSR8
        mov     cs:dissolve_seed, ax
        mov     al, ah
        xor     ah, ah
        mov     bx, ax
        mov     si, bx
        add     si, offset dissolve_table
        mov     cx, word ptr es:[2]
@@pixel:
        lodsb
        inc     di
        cmp     al, dl
        je      @@keep
        jb      @@keep
        xor     al, al
        dec     di
        stosb
@@keep:
        inc     bx
        xor     bh, bh
        mov     si, bx
        add     si, offset dissolve_table
        loop    @@pixel
        dec     cs:blit_columns
        jne     @@column
        ret
Pic_Dissolve ENDP

; (pic, bias) shade_table_block[c] = brightness of palette color c (R + R/2 + G + G/2 + B) / 16 + bias, then every
; non-zero pixel of the bitmap is replaced through that table
; Simtex name Gray_Scale_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo
Pic_Remap_Shade PROC USES ds di si es, hpic:WORD, hbias:WORD
        push    ds
        mov     dx, hbias
        mov     si, 0
        mov     ax, shade_table_block
        mov     es, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     di, 0
        mov     cx, 100h
        xor     ah, ah
@@color:
        xor     bx, bx
        lodsb
        add     bx, ax
        shr     al, 1
        add     bx, ax
        lodsb
        add     bx, ax
        shr     al, 1
        add     bx, ax
        lodsb
        add     bx, ax
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        add     bx, dx
        mov     al, bl
        stosb
        loop    @@color
        pop     ds
        mov     ax, hpic
        mov     es, ax
        mov     bx, shade_table_block
        mov     ds, bx
        mov     ax, word ptr es:[0]
        mov     bx, word ptr es:[2]
        mul     bx
        mov     cx, ax
        cmp     cx, 0
        je      @@done
        xor     bh, bh
        mov     di, 10h
        mov     si, di
@@pixel:
        mov     bl, es:[si]
        cmp     bl, 0
        je      @@skip
        mov     al, [bx]
        stosb
        inc     si
        loop    @@pixel
        jmp     @@done
@@skip:
        inc     si
        inc     di
        loop    @@pixel
@@done:
        ret
Pic_Remap_Shade ENDP

; (pic, base) as Pic_Remap_Shade with table[c] = base - brightness / 32 (darker colors higher)
Pic_Monochrome_Remap PROC USES ds di si es, mpic2:WORD, mbase:WORD
        push    ds
        mov     dx, mbase
        mov     si, 0
        mov     ax, shade_table_block
        mov     es, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     di, 0
        mov     cx, 100h
        xor     ah, ah
@@color:
        xor     bx, bx
        lodsb
        add     bx, ax
        shr     al, 1
        add     bx, ax
        lodsb
        add     bx, ax
        shr     al, 1
        add     bx, ax
        lodsb
        add     bx, ax
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        mov     dh, dl
        sub     dh, bl
        mov     al, dh
        stosb
        loop    @@color
        pop     ds
        mov     ax, mpic2
        mov     es, ax
        mov     bx, shade_table_block
        mov     ds, bx
        mov     ax, word ptr es:[0]
        mov     bx, word ptr es:[2]
        mul     bx
        mov     cx, ax
        cmp     cx, 0
        je      @@done
        xor     bh, bh
        mov     di, 10h
        mov     si, di
@@pixel:
        mov     bl, es:[si]
        cmp     bl, 0
        je      @@skip
        mov     al, [bx]
        stosb
        inc     si
        loop    @@pixel
        jmp     @@done
@@skip:
        inc     di
        inc     si
        loop    @@pixel
@@done:
        ret
Pic_Monochrome_Remap ENDP

; (pic, base) the same code as Pic_Monochrome_Remap
Pic_Remap_Shade2 PROC USES ds di si es, spic2:WORD, sbase:WORD
        push    ds
        mov     dx, sbase
        mov     si, 0
        mov     ax, shade_table_block
        mov     es, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     di, 0
        mov     cx, 100h
        xor     ah, ah
@@color:
        xor     bx, bx
        lodsb
        add     bx, ax
        shr     al, 1
        add     bx, ax
        lodsb
        add     bx, ax
        shr     al, 1
        add     bx, ax
        lodsb
        add     bx, ax
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        shr     bx, 1
        mov     dh, dl
        sub     dh, bl
        mov     al, dh
        stosb
        loop    @@color
        pop     ds
        mov     ax, spic2
        mov     es, ax
        mov     bx, shade_table_block
        mov     ds, bx
        mov     ax, word ptr es:[0]
        mov     bx, word ptr es:[2]
        mul     bx
        mov     cx, ax
        cmp     cx, 0
        je      @@done
        xor     bh, bh
        mov     di, 10h
        mov     si, di
@@pixel:
        mov     bl, es:[si]
        cmp     bl, 0
        je      @@skip
        mov     al, [bx]
        stosb
        inc     si
        loop    @@pixel
        jmp     @@done
@@skip:
        inc     di
        inc     si
        loop    @@pixel
@@done:
        ret
Pic_Remap_Shade2 ENDP

rng_lo          dw      2893h           ; 2459:09e3  this module's own 32-bit LFSR state
rng_hi          dw      000Ah           ; 2459:09e5

; (n) a random number 0 .. n - 1 (n = 0 -> 1) from the module's own 32-bit LFSR (9 steps, as UTIL.C's Random), the
; low 10 bits reduced by repeated subtraction
Pic_Random_Mod PROC USES ds di si es, rn:WORD
        LOCAL   bits:WORD
        cmp     rn, 0
        jne     @@n_ok
        mov     rn, 1
@@n_ok:
        mov     bits, 0
        mov     si, cs:rng_lo
        mov     di, cs:rng_hi
        mov     cx, 9
@@step:
        mov     ax, si
        mov     bx, si
        mov     dx, di
        shr     dx, 1
        rcr     bx, 1
        xor     ax, bx
        shr     dx, 1
        rcr     bx, 1
        xor     ax, bx
        shr     dx, 1
        rcr     bx, 1
        shr     dx, 1
        rcr     bx, 1
        xor     ax, bx
        shr     dx, 1
        rcr     bx, 1
        shr     dx, 1
        rcr     bx, 1
        xor     ax, bx
        shr     dx, 1
        xor     al, dh
        mov     dx, ax
        shr     dx, 1
        rcl     bits, 1
        shr     ax, 1
        rcr     di, 1
        rcr     si, 1
        loop    @@step
        cmp     si, 0
        jne     @@store
        cmp     di, 0
        jne     @@store
        mov     si, 30BEh
@@store:
        mov     cs:rng_lo, si
        mov     cs:rng_hi, di
        mov     ax, si
        and     ax, 3FFh
@@mod:
        cmp     ax, rn
        jb      @@done
        sub     ax, rn
        jmp     @@mod
@@done:
        ret
Pic_Random_Mod ENDP

        END
