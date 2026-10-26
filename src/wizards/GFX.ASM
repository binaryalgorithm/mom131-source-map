; GFX.ASM - WIZARDS.EXE code segment 1edd: the VGA page-presentation effects (dissolve, left-to-right and centre-out
; wipes) around the mouse-safe VGA_Present_Begin / _End, the line reveal and the transition copy steps that TRANSIT.C
; uses (most of them dead). Hand-written assembly (Borland TASM).
; The module starts at 1edd:000e, WORD aligned (a pad byte after ANIMEMS). Tables and variables IN THE CODE SEGMENT
; at its start: dissolve_planes (256 plane numbers, 1edd:000e), the dissolve row and column orders (1edd:010e / 0140)
; and the procedures' word variables (1edd:0168..0193, shared: gfx_* by the copy loops, line_* by
; VGA_Reveal_Line, colcopy_* by VGA_Effect_Column_Copy).
; "; C:" lines = the same behaviour as C (Borland C 3.1) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1edd src/wizards/GFX.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   draw_page_index:WORD    ; DS:76ee
        EXTRN   Mouse_Hide:FAR, Mouse_Show:FAR, Mouse_X:FAR, Mouse_Y:FAR, Mouse_Find_Region:FAR
        EXTRN   Mouse_Save_Under_Back:FAR, Mouse_Draw_Pointer_Back:FAR, Mouse_Restore_Under_Back:FAR
        EXTRN   Mouse_Copy_Save_Buffer:FAR, Page_Flip:FAR, Timer_Mark:FAR, Wait_Ticks:FAR
        .DATA                           ; this module's own data, DS:7898
vga_present_ds  dw      0               ; DS:7898  DS kept by VGA_Present_Begin
column_plane_masks db   01h, 02h, 04h, 08h      ; DS:789a  map-mask bit per plane (VGA_Effect_Column_Copy)
        .CODE
        PUBLIC  VGA_Present_Dissolve, VGA_Present_Dissolve_Slow, VGA_Present_Wipe_LR, VGA_Interlace_Shift, VGA_Interlace_Fill_Edges, VGA_Interlace_Shift_Band, VGA_Interlace_Fill_Band_Edges
        PUBLIC  VGA_Present_Wipe_Centre, VGA_Reveal_Line, VGA_Present_Begin, VGA_Present_End, VGA_Push_Left_Step, VGA_Push_Right_Step, VGA_Effect_Column_Copy
dissolve_planes db      02h, 02h, 02h, 01h, 01h, 03h, 00h, 03h, 03h, 02h, 00h, 01h, 03h, 03h, 01h, 03h    ; 1edd:000e  plane (0..3) per step
                db      03h, 00h, 03h, 01h, 02h, 03h, 00h, 02h, 01h, 01h, 02h, 02h, 01h, 01h, 03h, 03h
                db      02h, 03h, 03h, 00h, 02h, 03h, 00h, 02h, 00h, 03h, 02h, 00h, 02h, 00h, 00h, 03h
                db      01h, 00h, 01h, 01h, 01h, 03h, 03h, 01h, 01h, 00h, 00h, 02h, 01h, 01h, 02h, 01h
                db      00h, 00h, 00h, 03h, 03h, 01h, 01h, 02h, 02h, 01h, 02h, 02h, 00h, 02h, 03h, 01h
                db      02h, 02h, 02h, 00h, 00h, 00h, 01h, 01h, 00h, 02h, 00h, 03h, 02h, 02h, 01h, 00h
                db      03h, 01h, 02h, 03h, 00h, 00h, 01h, 00h, 02h, 02h, 03h, 02h, 01h, 01h, 02h, 02h
                db      00h, 03h, 00h, 00h, 02h, 00h, 00h, 03h, 03h, 01h, 02h, 01h, 00h, 00h, 01h, 03h
                db      01h, 01h, 01h, 00h, 00h, 02h, 03h, 01h, 00h, 03h, 03h, 00h, 01h, 00h, 00h, 00h
                db      01h, 03h, 01h, 03h, 03h, 02h, 02h, 03h, 02h, 00h, 01h, 01h, 00h, 03h, 00h, 02h
                db      00h, 02h, 01h, 01h, 03h, 01h, 02h, 01h, 03h, 01h, 00h, 01h, 03h, 03h, 01h, 01h
                db      03h, 02h, 03h, 02h, 03h, 01h, 01h, 02h, 00h, 02h, 03h, 03h, 02h, 02h, 00h, 02h
                db      03h, 03h, 03h, 02h, 02h, 00h, 02h, 00h, 01h, 00h, 01h, 03h, 02h, 01h, 02h, 02h
                db      00h, 01h, 00h, 02h, 01h, 01h, 03h, 00h, 03h, 03h, 03h, 00h, 03h, 00h, 02h, 01h
                db      01h, 00h, 00h, 02h, 01h, 02h, 03h, 03h, 01h, 00h, 01h, 03h, 00h, 02h, 03h, 00h
                db      02h, 01h, 02h, 03h, 00h, 02h, 02h, 00h, 02h, 03h, 01h, 00h, 03h, 03h, 03h, 00h
dissolve_rows   db      00h, 03h, 1Eh, 12h, 1Ch, 16h, 1Dh, 0Ch, 22h, 2Fh, 1Fh, 20h, 07h, 31h, 2Eh, 0Eh    ; 1edd:010e  the 50 row groups in dissolve order
                db      26h, 2Bh, 23h, 28h, 0Bh, 09h, 24h, 04h, 21h, 1Ah, 27h, 0Fh, 05h, 15h, 02h, 10h
                db      19h, 0Ah, 30h, 0Dh, 18h, 25h, 11h, 13h, 2Ch, 06h, 08h, 14h, 01h, 17h, 29h, 1Bh
                db      2Dh, 2Ah
dissolve_cols   db      02h, 09h, 1Eh, 26h, 0Dh, 1Bh, 1Fh, 01h, 13h, 08h, 18h, 24h, 25h, 1Ah, 00h, 0Bh    ; 1edd:0140  the 40 byte-column pairs in dissolve order
                db      17h, 20h, 16h, 0Ch, 10h, 14h, 1Dh, 12h, 1Ch, 07h, 0Ah, 23h, 0Eh, 19h, 05h, 21h
                db      0Fh, 15h, 04h, 11h, 06h, 03h, 27h, 22h
cs_unused_168   dw      0               ; 1edd:0168  no reader
cs_unused_16a   dw      0               ; 1edd:016a  no reader
dis_pass        dw      0               ; 1edd:016c  dissolve pass 4..1
dis_col         dw      0               ; 1edd:016e  dissolve column step 39..0
dis_plane_base  dw      0               ; 1edd:0170  (pass - 1) * 64: the pass's quarter of dissolve_planes
dis_row         dw      0               ; 1edd:0172  dissolve row step 49..0
gfx_column      dw      0               ; 1edd:0174  the byte column 0..79 of the wipes
gfx_plane_mask  dw      0               ; 1edd:0176  map-mask bit of the plane being copied (1, 2, 4, 8)
gfx_plane       dw      0               ; 1edd:0178  that plane's number (read-map select)
gfx_shift_bytes dw      0               ; 1edd:017a  step * 2: how far the incoming picture has moved
gfx_band_rows   dw      0               ; 1edd:017c  rows in the band (VGA_Interlace_*_Band*)
gfx_band_offset dw      0               ; 1edd:017e  byte offset of the band's first row
cs_unused_180   dw      0               ; 1edd:0180  no reader
line_x_step     dw      0               ; 1edd:0182  VGA_Reveal_Line: 8.8 x step per point
line_y_step     dw      0               ; 1edd:0184  8.8 y step per point
line_row_step   dw      0               ; 1edd:0186  +5 / -5 paragraphs: one row down / up
line_points     dw      0               ; 1edd:0188  points left to copy
dest_page_seg   dw      0               ; 1edd:018a  ES: A000h + draw_page_index * 400h
src_page_seg    dw      0               ; 1edd:018c  DS: the other page (A400h - ES + A000h)
line_plane      dw      0               ; 1edd:018e  plane of the current x (read-map select)
colcopy_row_ofs dw      0               ; 1edd:0190  VGA_Effect_Column_Copy: y * 80
colcopy_dest    dw      0               ; 1edd:0192  the destination offset, +1 after every 4th plane

; dissolve: copies page DS (src_page_seg) onto page ES (dest_page_seg) in a shuffled order (dissolve_rows /
; dissolve_cols), 4 passes x 40 x 50 steps, 2 bytes x 4 rows (y, y+50, y+100, y+150) per step, one
; plane each (dissolve_planes), then VGA_Present_End flips
; C:  void VGA_Present_Dissolve(void)
VGA_Present_Dissolve PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 12h
            push    es
            push    di
            push    si
            push    ds
            call    FAR PTR VGA_Present_Begin
            mov     WORD PTR [bp-0Ch], 0
            mov     WORD PTR [bp-10h], 13Fh
            mov     WORD PTR [bp-0Eh], 0
            mov     WORD PTR [bp-12h], 31h
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            mov     WORD PTR cs:dis_pass, 4
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
@@L01ea:    mov     ax, WORD PTR cs:dis_pass
            dec     ax
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            mov     WORD PTR cs:dis_plane_base, ax
            mov     WORD PTR cs:dis_col, 27h
@@L0206:    mov     WORD PTR cs:dis_row, 31h
@@L020d:    mov     bx, WORD PTR cs:dis_row
            mov     bl, BYTE PTR cs:dissolve_rows[bx]
            xor     bh, bh
            mov     cx, bx
            mov     dx, bx
            shl     bx, 1
            shl     bx, 1
            add     bx, cx
            shl     bx, 1
            shl     bx, 1
            shl     bx, 1
            shl     bx, 1
            mov     ax, bx
            add     cx, WORD PTR cs:dis_col
            cmp     cx, 28h
            js      @@L0242
            sub     cx, 28h
            cmp     cx, 28h
            js      @@L0242
            sub     cx, 28h
@@L0242:    mov     bx, cx
            mov     bl, BYTE PTR cs:dissolve_cols[bx]
            xor     bh, bh
            mov     cx, bx
            shl     bx, 1
            add     ax, bx
            mov     si, ax
            mov     di, ax
            add     cx, dx
            add     cx, WORD PTR cs:dis_plane_base
            xor     ch, ch
            mov     bx, cx
            mov     ah, BYTE PTR cs:dissolve_planes[bx]
            inc     bl
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            mov     cl, ah
            mov     ah, 1
            cmp     cl, 0
            je      @@L027e
@@L027a:    shl     ah, 1
            loop    @@L027a
@@L027e:    mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            sub     si, 0FA1h
            sub     di, 0FA1h
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            sub     si, 2EE0h
            sub     di, 2EE0h
            mov     ah, BYTE PTR cs:dissolve_planes[bx]
            inc     bl
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            mov     cl, ah
            mov     ah, 1
            cmp     cl, 0
            je      @@L02ca
@@L02c6:    shl     ah, 1
            loop    @@L02c6
@@L02ca:    mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            sub     si, 0FA1h
            sub     di, 0FA1h
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            dec     WORD PTR cs:dis_row
            js      @@L02fa
            jmp     @@L020d
@@L02fa:    dec     WORD PTR cs:dis_col
            js      @@L0304
            jmp     @@L0206
@@L0304:    dec     WORD PTR cs:dis_pass
            je      @@L030e
            jmp     @@L01ea
@@L030e:    call    FAR PTR VGA_Present_End
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Present_Dissolve ENDP

; a variant of VGA_Present_Dissolve (the same tables) with a Wait_Ticks(3) per pass; no callers
; C:  void VGA_Present_Dissolve_Slow(void)
VGA_Present_Dissolve_Slow PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 12h
            push    es
            push    di
            push    si
            push    ds
            call    FAR PTR VGA_Present_Begin
            mov     WORD PTR [bp-0Ch], 0
            mov     WORD PTR [bp-10h], 13Fh
            mov     WORD PTR [bp-0Eh], 0
            mov     WORD PTR [bp-12h], 31h
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            mov     WORD PTR cs:dis_pass, 4
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
@@L0371:    mov     ax, WORD PTR cs:dis_pass
            dec     ax
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            shl     ax, 1
            mov     WORD PTR cs:dis_plane_base, ax
            mov     WORD PTR cs:dis_col, 27h
@@L038d:    mov     WORD PTR cs:dis_row, 31h
@@L0394:    mov     bx, WORD PTR cs:dis_row
            mov     bl, BYTE PTR cs:dissolve_rows[bx]
            xor     bh, bh
            mov     cx, bx
            mov     dx, bx
            shl     bx, 1
            shl     bx, 1
            add     bx, cx
            shl     bx, 1
            shl     bx, 1
            shl     bx, 1
            shl     bx, 1
            mov     ax, bx
            add     cx, WORD PTR cs:dis_col
            cmp     cx, 28h
            js      @@L03c9
            sub     cx, 28h
            cmp     cx, 28h
            js      @@L03c9
            sub     cx, 28h
@@L03c9:    mov     bx, cx
            mov     bl, BYTE PTR cs:dissolve_cols[bx]
            xor     bh, bh
            mov     cx, bx
            shl     bx, 1
            add     ax, bx
            mov     si, ax
            mov     di, ax
            add     cx, dx
            add     cx, WORD PTR cs:dis_plane_base
            xor     ch, ch
            mov     bx, cx
            mov     ah, BYTE PTR cs:dissolve_planes[bx]
            inc     bl
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            mov     cl, ah
            mov     ah, 1
            cmp     cl, 0
            je      @@L0405
@@L0401:    shl     ah, 1
            loop    @@L0401
@@L0405:    mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            sub     si, 0FA1h
            sub     di, 0FA1h
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            sub     si, 2EE0h
            sub     di, 2EE0h
            mov     ah, BYTE PTR cs:dissolve_planes[bx]
            inc     bl
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            mov     cl, ah
            mov     ah, 1
            cmp     cl, 0
            je      @@L0451
@@L044d:    shl     ah, 1
            loop    @@L044d
@@L0451:    mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, ah
            out     dx, al
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            sub     si, 0FA1h
            sub     di, 0FA1h
            movsb
            add     si, 1F3Fh
            add     di, 1F3Fh
            movsb
            dec     WORD PTR cs:dis_row
            js      @@L0481
            jmp     @@L0394
@@L0481:    dec     WORD PTR cs:dis_col
            js      @@L048b
            jmp     @@L038d
@@L048b:    dec     WORD PTR cs:dis_pass
            je      @@L04b6
            push    ax
            push    bx
            push    cx
            push    dx
            push    ds
            push    es
            push    si
            push    di
            call    Timer_Mark
            mov     ax, 3
            push    ax
            call    Wait_Ticks
            add     sp, 2
            pop     di
            pop     si
            pop     es
            pop     ds
            pop     dx
            pop     cx
            pop     bx
            pop     ax
            jmp     @@L0371
@@L04b6:    call    FAR PTR VGA_Present_End
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Present_Dissolve_Slow ENDP

; left-to-right wipe: per byte column (80) and plane, the 200-pixel column copied page DS -> page ES, then a
; 1000-iteration busy loop (its speed depends on the CPU); then VGA_Present_End
; C:  void VGA_Present_Wipe_LR(void)
VGA_Present_Wipe_LR PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 2
            push    es
            push    di
            push    si
            push    ds
            call    FAR PTR VGA_Present_Begin
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
            mov     WORD PTR cs:gfx_column, 0
@@L0505:    mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L0513:    mov     ax, WORD PTR cs:gfx_column
            mov     si, ax
            mov     di, ax
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     cx, 0C8h
@@L0536:    movsb
            add     si, 4Fh
            add     di, 4Fh
            loop    @@L0536
            mov     cx, 3E8h
@@L0542:    nop
            loop    @@L0542
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L0513
            inc     WORD PTR cs:gfx_column
            mov     ax, WORD PTR cs:gfx_column
            cmp     ax, 50h
            jne     @@L0505
            call    FAR PTR VGA_Present_End
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Present_Wipe_LR ENDP

; DEAD (no caller runs): per plane, shifts the page contents by one byte column
; C:  void VGA_Interlace_Shift(void)
VGA_Interlace_Shift PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 4
            push    es
            push    di
            push    si
            push    ds
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     ds, ax
            mov     es, ax
            mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L059c:    mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     dx, 64h
            mov     si, 52h
            mov     di, 50h
@@L05bd:    mov     cx, 27h
            rep     movsw
            add     si, 52h
            add     di, 52h
            dec     dx
            jne     @@L05bd
            mov     dx, 64h
            mov     si, 4Ch
            mov     di, 4Eh
@@L05d4:    mov     cx, 27h
@@L05d7:    movsw
            sub     si, 4
            sub     di, 4
            loop    @@L05d7
            add     si, 0EEh
            add     di, 0EEh
            dec     dx
            jne     @@L05d4
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L059c
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Interlace_Shift ENDP

; DEAD: the page-copy step of Transition_Interlace_Push
; C:  void VGA_Interlace_Fill_Edges(int step)
VGA_Interlace_Fill_Edges PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 4
            push    es
            push    di
            push    si
            push    ds
            shl     WORD PTR [bp+6], 1
            mov     ax, WORD PTR [bp+6]
            mov     WORD PTR cs:gfx_shift_bytes, ax
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
            mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L0653:    mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     di, 0
            mov     si, 4Eh
            sub     si, WORD PTR cs:gfx_shift_bytes
            mov     cx, 64h
@@L0679:    movsw
            add     si, 9Eh
            add     di, 9Eh
            loop    @@L0679
            mov     di, 9Eh
            mov     si, 50h
            add     si, WORD PTR cs:gfx_shift_bytes
            mov     cx, 64h
@@L0692:    movsw
            add     si, 9Eh
            add     di, 9Eh
            loop    @@L0692
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L0653
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Interlace_Fill_Edges ENDP

; DEAD: the page-copy step of Transition_Interlace_Band_Push
; C:  void VGA_Interlace_Shift_Band(int offset, int rows)
VGA_Interlace_Shift_Band PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 8
            push    es
            push    di
            push    si
            push    ds
            mov     ax, WORD PTR [bp+6]
            mov     WORD PTR cs:gfx_band_offset, ax
            mov     ax, WORD PTR [bp+8]
            mov     WORD PTR cs:gfx_band_rows, ax
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     ds, ax
            mov     es, ax
            mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L06ef:    mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     dx, WORD PTR cs:gfx_band_rows
            mov     si, WORD PTR cs:gfx_band_offset
            mov     di, si
            add     si, 52h
            add     di, 50h
@@L0719:    mov     cx, 27h
            rep     movsw
            add     si, 52h
            add     di, 52h
            dec     dx
            jne     @@L0719
            mov     dx, WORD PTR cs:gfx_band_rows
            mov     si, WORD PTR cs:gfx_band_offset
            mov     di, si
            add     si, 4Ch
            add     di, 4Eh
@@L0739:    mov     cx, 27h
@@L073c:    movsw
            sub     si, 4
            sub     di, 4
            loop    @@L073c
            add     si, 0EEh
            add     di, 0EEh
            dec     dx
            jne     @@L0739
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L06ef
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Interlace_Shift_Band ENDP

; DEAD: the row-shift step of Transition_Interlace_Band_Push: per plane, the rows of the band shifted by a byte
; C:  void VGA_Interlace_Fill_Band_Edges(int first, int offset, int rows)
VGA_Interlace_Fill_Band_Edges PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 4
            push    es
            push    di
            push    si
            push    ds
            shl     WORD PTR [bp+6], 1
            mov     ax, WORD PTR [bp+8]
            mov     WORD PTR cs:gfx_band_offset, ax
            mov     ax, WORD PTR [bp+0Ah]
            mov     WORD PTR cs:gfx_band_rows, ax
            mov     ax, WORD PTR [bp+6]
            mov     WORD PTR cs:gfx_shift_bytes, ax
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
            mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L07c6:    mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     di, WORD PTR cs:gfx_band_offset
            mov     si, di
            add     si, 4Eh
            sub     si, WORD PTR cs:gfx_shift_bytes
            mov     cx, WORD PTR cs:gfx_band_rows
@@L07f2:    movsw
            add     si, 9Eh
            add     di, 9Eh
            add     si, 2
            loop    @@L07f2
            mov     di, WORD PTR cs:gfx_band_offset
            mov     si, di
            add     di, 9Eh
            add     si, 50h
            add     si, WORD PTR cs:gfx_shift_bytes
            mov     cx, WORD PTR cs:gfx_band_rows
@@L0818:    movsw
            add     si, 9Eh
            add     di, 9Eh
            sub     si, 2
            loop    @@L0818
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L07c6
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Interlace_Fill_Band_Edges ENDP

; centre-out wipe: columns 40..79 and the mirrored 39..0 together, per plane, a 2000-iteration
; busy loop each; then VGA_Present_End (its swapped push / pop of SI / DI is harmless: both hold
; the same column); no callers
; C:  void VGA_Present_Wipe_Centre(void)
VGA_Present_Wipe_Centre PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 2
            push    es
            push    di
            push    si
            push    ds
            call    FAR PTR VGA_Present_Begin
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
            mov     WORD PTR cs:gfx_column, 28h
@@L0882:    mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L0890:    mov     ax, WORD PTR cs:gfx_column
            mov     si, ax
            mov     di, ax
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            push    si
            push    di
            mov     cx, 0C8h
@@L08b5:    movsb
            add     si, 4Fh
            add     di, 4Fh
            loop    @@L08b5
            pop     si
            pop     di
            mov     bx, si
            sub     bx, 28h
            mov     ax, 27h
            sub     ax, bx
            mov     si, ax
            mov     bx, di
            sub     bx, 28h
            mov     ax, 27h
            sub     ax, bx
            mov     di, ax
            mov     cx, 0C8h
@@L08db:    movsb
            add     si, 4Fh
            add     di, 4Fh
            loop    @@L08db
            mov     cx, 7D0h
@@L08e7:    nop
            loop    @@L08e7
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L0890
            inc     WORD PTR cs:gfx_column
            mov     ax, WORD PTR cs:gfx_column
            cmp     ax, 50h
            je      @@L090d
            jmp     @@L0882
@@L090d:    call    FAR PTR VGA_Present_End
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Present_Wipe_Centre ENDP

; (x1, y1, x2, y2, fine) copies the pixels along a line from the back page to the displayed page (8.8
; fixed-point steps along the major axis; fine != 0 doubles the points with half steps); used only by
; the unused Transition_Blinds / Transition_Block_Step
; C:  void VGA_Reveal_Line(int x1, int y1, int x2, int y2, int fine)
VGA_Reveal_Line PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 2
            push    es
            push    ds
            push    si
            push    di
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     ax, WORD PTR [bp+0Ah]
            cmp     ax, WORD PTR [bp+6]
            jns     @@L0963
            mov     si, ax
            mov     ax, WORD PTR [bp+6]
            mov     WORD PTR [bp+0Ah], ax
            mov     WORD PTR [bp+6], si
            mov     si, WORD PTR [bp+0Ch]
            mov     ax, WORD PTR [bp+8]
            mov     WORD PTR [bp+0Ch], ax
            mov     WORD PTR [bp+8], si
@@L0963:    mov     ax, WORD PTR [bp+6]
            mov     bx, WORD PTR [bp+8]
            mov     cx, WORD PTR [bp+0Ah]
            mov     dx, WORD PTR [bp+0Ch]
            sub     cx, ax
            sub     dx, bx
            mov     WORD PTR cs:line_row_step, 5
            jns     @@L0985
            neg     dx
            mov     WORD PTR cs:line_row_step, 0FFFBh
@@L0985:    cmp     cx, dx
            jns     @@L09ae
            inc     dx
            mov     WORD PTR cs:line_points, dx
            mov     WORD PTR cs:line_y_step, 100h
            dec     dx
            je      @@L09cc
            mov     bx, dx
            mov     ah, cl
            xor     al, al
            mov     dl, ch
            xor     dh, dh
            mov     cx, bx
            div     cx
            mov     WORD PTR cs:line_x_step, ax
            jmp     @@L09cc
@@L09ae:    inc     cx
            mov     WORD PTR cs:line_points, cx
            dec     cx
            je      @@L09cc
            mov     WORD PTR cs:line_x_step, 100h
            mov     ah, dl
            xor     al, al
            mov     dl, dh
            xor     dh, dh
            div     cx
            mov     WORD PTR cs:line_y_step, ax
@@L09cc:    cmp     WORD PTR [bp+0Eh], 0
            je      @@L09f0
            mov     ax, WORD PTR cs:line_points
            shl     ax, 1
            mov     WORD PTR cs:line_points, ax
            mov     ax, WORD PTR cs:line_x_step
            shr     ax, 1
            mov     WORD PTR cs:line_x_step, ax
            mov     ax, WORD PTR cs:line_y_step
            shr     ax, 1
            mov     WORD PTR cs:line_y_step, ax
@@L09f0:    mov     bx, WORD PTR [bp+8]
            mov     ax, bx
            shl     ax, 1
            shl     ax, 1
            add     ax, bx
            mov     bx, ax
            mov     cx, WORD PTR [bp+6]
            add     ax, WORD PTR cs:dest_page_seg
            mov     es, ax
            add     bx, WORD PTR cs:src_page_seg
            mov     ds, bx
            mov     ax, cx
            shr     ax, 1
            shr     ax, 1
            mov     di, ax
            mov     ax, 1
            and     cx, 3
            mov     WORD PTR cs:line_plane, cx
            je      @@L0a25
            shl     ax, cl
@@L0a25:    xor     ah, ah
            mov     WORD PTR cs:gfx_plane_mask, ax
            mov     bx, 80h
            mov     cx, 80h
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:line_plane
            out     dx, al
            mov     si, di
            jmp     @@L0aad
@@L0a4e:    add     bx, WORD PTR cs:line_x_step
            cmp     bh, 0
            je      @@L0a8f
            xor     bh, bh
            inc     BYTE PTR cs:line_plane
            mov     al, BYTE PTR cs:gfx_plane_mask
            shl     al, 1
            cmp     al, 9
            js      @@L0a73
            mov     BYTE PTR cs:line_plane, 0
            mov     al, 1
            inc     di
            inc     si
@@L0a73:    mov     BYTE PTR cs:gfx_plane_mask, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:line_plane
            out     dx, al
@@L0a8f:    add     cx, WORD PTR cs:line_y_step
            cmp     ch, 0
            je      @@L0aad
            xor     ch, ch
            mov     dx, es
            add     dx, WORD PTR cs:line_row_step
            mov     es, dx
            mov     dx, ds
            add     dx, WORD PTR cs:line_row_step
            mov     ds, dx
@@L0aad:    movsb
            dec     si
            dec     di
            dec     WORD PTR cs:line_points
            je      @@L0ab9
            jmp     @@L0a4e
@@L0ab9:    pop     di
            pop     si
            pop     ds
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Reveal_Line ENDP

; before a page copy: keeps DS for VGA_Present_End and hides the mouse
; C:  void VGA_Present_Begin(void) { vga_present_ds = _DS; Mouse_Hide(); }
VGA_Present_Begin PROC FAR
            push    ds
            mov     ax, ds
            mov     vga_present_ds, ax
            call    Mouse_Hide
            pop     ds
            retf
VGA_Present_Begin ENDP

; after a page copy: redraws the mouse cursor on the back page, flips, restores
; C:  void VGA_Present_End(void) { x, y = Mouse_X / Y; Mouse_Find_Region(x, y); Mouse_Save_Under_Back;
; C:    Mouse_Draw_Pointer_Back; Page_Flip(); Mouse_Restore_Under_Back(); Mouse_Copy_Save_Buffer(); Mouse_Show(); }
VGA_Present_End PROC FAR
            push    ds
            mov     ax, DGROUP
            mov     ds, ax
            mov     ax, vga_present_ds
            mov     ds, ax
            call    Mouse_Y
            push    ax
            call    Mouse_X
            push    ax
            call    Mouse_Find_Region
            call    Mouse_Save_Under_Back
            call    Mouse_Draw_Pointer_Back
            add     sp, 4
            call    Page_Flip
            call    Mouse_Restore_Under_Back
            call    Mouse_Copy_Save_Buffer
            call    Mouse_Show
            pop     ds
            retf
VGA_Present_End ENDP

; DEAD: the page-copy step of Transition_Push_Left
; C:  void VGA_Push_Left_Step(int step)
VGA_Push_Left_Step PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 4
            push    es
            push    di
            push    si
            push    ds
            shl     WORD PTR [bp+6], 1
            mov     ax, WORD PTR [bp+6]
            mov     WORD PTR cs:gfx_shift_bytes, ax
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     es, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L0b51:    mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     ax, WORD PTR cs:dest_page_seg
            mov     ds, ax
            mov     dx, 0C8h
            mov     si, 52h
            mov     di, 50h
@@L0b78:    mov     cx, 27h
            rep     movsw
            add     si, 2
            add     di, 2
            dec     dx
            jne     @@L0b78
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
            mov     di, 9Eh
            mov     si, 50h
            add     si, WORD PTR cs:gfx_shift_bytes
            mov     cx, 0C8h
@@L0b9a:    movsw
            add     si, 4Eh
            add     di, 4Eh
            loop    @@L0b9a
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L0b51
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Push_Left_Step ENDP

; DEAD: the page-copy step of Transition_Push_Right
; C:  void VGA_Push_Right_Step(int step)
VGA_Push_Right_Step PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 4
            push    es
            push    di
            push    si
            push    ds
            shl     WORD PTR [bp+6], 1
            mov     ax, WORD PTR [bp+6]
            mov     WORD PTR cs:gfx_shift_bytes, ax
            mov     ax, draw_page_index
            mov     ah, al
            xor     al, al
            shl     ax, 1
            shl     ax, 1
            add     ax, 0A000h
            mov     WORD PTR cs:dest_page_seg, ax
            mov     es, ax
            mov     bx, 0A400h
            sub     bx, ax
            add     bx, 0A000h
            mov     WORD PTR cs:src_page_seg, bx
            mov     WORD PTR cs:gfx_plane_mask, 1
            mov     WORD PTR cs:gfx_plane, 0
@@L0c01:    mov     dx, 3CEh
            mov     al, 4
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, BYTE PTR cs:gfx_plane_mask
            out     dx, al
            mov     ax, WORD PTR cs:dest_page_seg
            mov     ds, ax
            mov     dx, 0C8h
            mov     si, 4Ch
            mov     di, 4Eh
@@L0c28:    mov     cx, 27h
@@L0c2b:    movsw
            sub     si, 4
            sub     di, 4
            loop    @@L0c2b
            add     si, 9Eh
            add     di, 9Eh
            dec     dx
            jne     @@L0c28
            mov     ax, WORD PTR cs:src_page_seg
            mov     ds, ax
            mov     di, 0
            mov     si, 4Eh
            sub     si, WORD PTR cs:gfx_shift_bytes
            mov     cx, 0C8h
@@L0c53:    movsw
            add     si, 4Eh
            add     di, 4Eh
            loop    @@L0c53
            inc     WORD PTR cs:gfx_plane
            shl     WORD PTR cs:gfx_plane_mask, 1
            cmp     WORD PTR cs:gfx_plane_mask, 10h
            jne     @@L0c01
            pop     ds
            pop     si
            pop     di
            pop     es
            mov     sp, bp
            pop     bp
            retf
VGA_Push_Right_Step ENDP

; (x, y) copies the back page into the displayed page from (x, y) to the end of the page, plane
; by plane (column_plane_masks); Transition_Wipe_Left
; C:  void VGA_Effect_Column_Copy(int x, int y)
VGA_Effect_Column_Copy PROC FAR
            push    bp
            mov     bp, sp
            sub     sp, 4
            push    ds
            push    es
            push    si
            push    di
            mov     bx, 1
            sub     bx, draw_page_index
            mov     bh, bl
            xor     bl, bl
            shl     bx, 1
            shl     bx, 1
            add     bx, 0A000h
            mov     es, bx
            mov     ax, WORD PTR [bp+8]
            mov     cx, 50h
            mul     cx
            mov     WORD PTR cs:colcopy_row_ofs, ax
            mov     di, ax
            mov     ax, WORD PTR [bp+6]
            shr     ax, 1
            shr     ax, 1
            add     di, ax
            mov     WORD PTR cs:colcopy_dest, di
            mov     bx, WORD PTR [bp+6]
            and     bx, 3
            add     bx, OFFSET column_plane_masks
            mov     bh, BYTE PTR [bx]
            xor     bl, bl
            mov     ax, 0A800h
            mov     ds, ax
@@L0cc4:    mov     al, 4
            mov     dx, 3CEh
            out     dx, al
            inc     dx
            mov     al, bl
            out     dx, al
            mov     dx, 3C4h
            mov     al, 2
            out     dx, al
            inc     dx
            mov     al, bh
            out     dx, al
            xor     si, si
            mov     cx, 1F40h
            mov     ax, WORD PTR cs:colcopy_row_ofs
            shr     ax, 1
            sub     cx, ax
            rep     movsw
            inc     bl
            shl     bh, 1
            cmp     bh, 9
            js      @@L0cf7
            mov     bh, 1
            inc     WORD PTR cs:colcopy_dest
@@L0cf7:    mov     di, WORD PTR cs:colcopy_dest
            cmp     bl, 4
            jne     @@L0cc4
            pop     di
            pop     si
            pop     es
            pop     ds
            mov     sp, bp
            pop     bp
            retf
VGA_Effect_Column_Copy ENDP
        END
