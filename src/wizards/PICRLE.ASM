; PICRLE.ASM - WIZARDS.EXE code segment 2006 (MAGIC.EXE 1e7a, the same file): the column-RLE picture blitters without
; clipping - from a frame in normal memory (seg:offset) and from a frame cached in EMS (read through the page frame,
; the near helper moves the 4-page window when the pointer passes C000h); plain and remap variants of each.
; Hand-written assembly (Borland TASM). The procedure bodies are the same as ANIMEMS.ASM's (that module is the
; animation version with a 2-page window).
; Three WORDS IN THE CODE SEGMENT between the memory blitters and the EMS blitters (2006:02f6).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 2006 src/wizards/PICRLE.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   ems_frame_seg:WORD      ; DS:760c
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        EXTRN   EMS_Map_Pages:FAR
        .DATA                           ; this module's own data, DS:78a0
                dw      0               ; DS:789e  no reader (link test, batch 36: the word before plane_bits in both programs)
plane_bits      db      01h, 02h, 04h, 08h      ; DS:78a0  the VGA plane of x & 3
        .CODE
        PUBLIC  Pic_Blit_Planar_Rows, Pic_Blit_Planar_Rows_Remap, Pic_Draw_EMS_Frame, Pic_EMS_Map_Next
        PUBLIC  Pic_Draw_EMS_Frame_Remap

; (x, y, columns, data, data_seg) draws a column-RLE frame from memory (no clipping)
; C:  void Pic_Blit_Planar_Rows(int x, int y, int cols, char *data, unsigned seg) { (decode cols columns at x, y) }
Pic_Blit_Planar_Rows PROC USES si di es ds, x:WORD, y:WORD, cols:WORD, data:WORD, dseg:WORD
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
        mov     si, data
        mov     cx, cols
        mov     bx, dseg
        mov     ds, bx
        mov     bx, cx
@@column:
        push    di
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
Pic_Blit_Planar_Rows ENDP

; (x, y, columns, data, data_seg) the remap variant (pixels E8h..FFh through the remap tables)
; C:  as Pic_Blit_Planar_Rows, pixel p >= 0xE8: screen = remap[p - 0xE8][screen]
Pic_Blit_Planar_Rows_Remap PROC USES si di es ds, rx:WORD, ry:WORD, rcols:WORD, rdata:WORD, rdseg:WORD
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
        mov     si, rdata
        mov     cx, rcols
        mov     bx, rdseg
        mov     ds, bx
        mov     bx, cx
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
Pic_Blit_Planar_Rows_Remap ENDP

ems_page        dw      0               ; 2006:02f6  logical EMS page mapped to physical page 0
ems_handle      dw      0               ; 2006:02f8
pic_width       dw      0               ; 2006:02fa  columns (picture header word 0)

; (x, y, pic_seg, frame) as Anim_Blit_EMS_Frame (ANIMEMS.ASM) with a 4-page (48 KB) window
; C:  void Pic_Draw_EMS_Frame(int x, int y, unsigned pic, int frame) { (map the frame's pages; plain or remap blit) }
Pic_Draw_EMS_Frame PROC USES si di es ds, ex:WORD, ey:WORD, eseg:WORD, eframe:WORD
        mov     ax, eseg
        mov     ds, ax
        mov     es, ax
        mov     ax, word ptr ds:[0]
        mov     cs:pic_width, ax
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
        jb      @@page_ok
        sub     dx, 0C000h
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
        push    eseg
        push    ey
        push    ex
        call    far ptr Pic_Draw_EMS_Frame_Remap
        add     sp, 6
        ret
@@plain:
        mov     ax, DGROUP
        mov     ds, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, ey
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, ex
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
        mov     bx, cs:pic_width
@@column:
        push    di
        cmp     si, 0C000h
        jb      @@mapped
        call    Pic_EMS_Map_Next
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
Pic_Draw_EMS_Frame ENDP

; near register helper: SI passed C000h - map the next 4 logical pages (ems_page + 3) and move SI back 48 KB;
; keeps AX, BX
; C:  static void Pic_EMS_Map_Next(void) { ems_page += 3; EMS_Map_Pages(ems_page, ems_handle); src -= 0xC000; }
Pic_EMS_Map_Next PROC NEAR
        push    bx
        mov     cx, ax
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
        mov     ax, cx
        pop     bx
        ret
Pic_EMS_Map_Next ENDP

; (x, y, pic_seg) the remap variant of Pic_Draw_EMS_Frame, in its caller's register state (as ANIMEMS.ASM)
; C:  as Pic_Draw_EMS_Frame, pixel p >= 0xE8: screen = remap[p - 0xE8][screen]
Pic_Draw_EMS_Frame_Remap PROC, rex:WORD, rey:WORD, reseg:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, rey
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, rex
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
        mov     bx, cs:pic_width
@@column:
        push    di
        cmp     si, 0C000h
        jb      @@mapped
        call    Pic_EMS_Map_Next
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
Pic_Draw_EMS_Frame_Remap ENDP

        END
