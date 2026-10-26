; VGAPAGE.ASM - WIZARDS.EXE code segment 1fd7 (MAGIC.EXE 1e4b, the same file): the two VGA display pages and the
; off-screen stores. Video memory: page 0 at A000, page 1 at A400 (16000 bytes per plane each), stores at A800 and
; AC00. draw_page_index is the DISPLAYED page; drawing goes to the other one (draw_page_seg). Copies between pages use
; the VGA latches (map mask 0Fh, bit mask 0 - one byte moves all 4 planes). Hand-written assembly (Borland TASM).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled; a port
; keeps 4 off-screen 320 x 200 buffers and copies whole pictures.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1fd7 src/wizards/VGAPAGE.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   draw_page_index:WORD    ; DS:76ee
        EXTRN   Wait_Vertical_Retrace:FAR, Mouse_Get_Pointer_Image:FAR, Mouse_Hide:FAR, Mouse_Show:FAR
        EXTRN   Mouse_X:FAR, Mouse_Y:FAR, Mouse_Restore_Under_Back:FAR, Mouse_Find_Region:FAR
        EXTRN   Mouse_Restore_Under:FAR, Mouse_Save_Under:FAR, Mouse_Draw_Pointer:FAR, Mouse_Set_Position:FAR
        .CODE
        PUBLIC  Set_Draw_Page_Front, Set_Draw_Page, VGA_Front_Page_Check, Page_Flip, Screen_Copy_Front
        PUBLIC  VGA_Copy_Front_With_Cursor, VGA_Copy_Front, Screen_Save_A800, Screen_Restore_A800
        PUBLIC  Screen_Save_AC00, Screen_Restore_AC00, VGA_Load_Planar_Image

; () draw straight onto the displayed page
; C:  void Set_Draw_Page_Front(void) { draw_page_seg = 0xA000 + draw_page_index * 0x400; }
; Simtex name Set_Page_On (MoO2 symbols, module video); clarified using ReMoM repo
Set_Draw_Page_Front PROC
        mov     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     draw_page_seg, ax
        ret
Set_Draw_Page_Front ENDP

; () draw onto the hidden page
; C:  void Set_Draw_Page(void) { draw_page_seg = 0xA000 + (1 - draw_page_index) * 0x400; }
; Simtex name Set_Page_Off (MoO2 symbols, module video); clarified using ReMoM repo
Set_Draw_Page PROC
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     draw_page_seg, ax
        ret
Set_Draw_Page ENDP

; () 1 if drawing does not go to A000 (page 1 or a store), else 0
; C:  int VGA_Front_Page_Check(void) { return draw_page_seg != 0xA000; }
VGA_Front_Page_Check PROC
        mov     ax, draw_page_seg
        cmp     ax, 0A000h
        je      @@page0
        mov     ax, 1
        ret
@@page0:
        sub     ax, ax
        ret
VGA_Front_Page_Check ENDP

; () show the page just drawn: CRTC start address high byte (index 0Ch) = index * 40h, wait for the retrace that
; latches it, then draw on the other page
; C:  void Page_Flip(void) { draw_page_index = 1 - draw_page_index; outportb(0x3D4, 0x0C);
; C:      outportb(0x3D5, draw_page_index << 6); Wait_Vertical_Retrace(); Set_Draw_Page(); }
Page_Flip PROC USES es ds si di
        mov     ax, 1
        sub     ax, draw_page_index
        mov     draw_page_index, ax
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     bx, ax
        mov     dx, 03D4h
        mov     al, 0Ch
        out     dx, al
        inc     dx
        mov     al, bl
        out     dx, al
        call    Wait_Vertical_Retrace
        call    Set_Draw_Page
        ret
Page_Flip ENDP

; () copy the displayed page to the hidden one (keeping the mouse pointer alive if one is shown)
; C:  void Screen_Copy_Front(void) { if (Mouse_Get_Pointer_Image()) VGA_Copy_Front_With_Cursor(); else VGA_Copy_Front(); }
Screen_Copy_Front PROC USES ds es si di
        call    Mouse_Get_Pointer_Image
        cmp     ax, 0
        je      @@plain
        call    far ptr VGA_Copy_Front_With_Cursor
        ret
@@plain:
        call    far ptr VGA_Copy_Front
        ret
Screen_Copy_Front ENDP

; () latch copy displayed -> hidden page in 4000-byte steps; after each step, if the mouse moved, the pointer is
; moved on the displayed page so it does not freeze during the copy. The local word at [bp-8] is never used.
; C:  void VGA_Copy_Front_With_Cursor(void) { unsigned o; int x, y; Mouse_Hide(); x = Mouse_X(); y = Mouse_Y();
; C:      for (o = 0; o < 16000; o += 4000) { latch_copy(front + o, back + o, 4000); Mouse_Restore_Under_Back();
; C:          if (Mouse_X() != x || Mouse_Y() != y) { Mouse_Find_Region(nx, ny); Mouse_Restore_Under(..);
; C:              Mouse_Save_Under(..); Mouse_Find_Region(..); Mouse_Draw_Pointer(..); Mouse_Set_Position(..); } }
; C:      Mouse_Restore_Under_Back(); Mouse_Show(); }   (x, y are NOT updated after a move: every later step redraws)
VGA_Copy_Front_With_Cursor PROC USES ds es si di
        LOCAL   front_seg:WORD, mouse_x:WORD, mouse_y:WORD, unused:WORD, data_seg:WORD
        mov     ax, ds
        mov     data_seg, ax
        call    Mouse_Hide
        call    Mouse_X
        mov     mouse_x, ax
        call    Mouse_Y
        mov     mouse_y, ax
        mov     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     front_seg, ax
        mov     bx, 0A400h
        sub     bx, ax
        add     bx, 0A000h
        mov     es, bx
        sub     si, si
        sub     di, di
@@chunk:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 0Fh
        out     dx, al
        mov     ax, 8
        mov     dx, 03CEh
        out     dx, al
        inc     dx
        mov     al, 0
        out     dx, al
        mov     cx, 4000
        rep     movsb
        mov     al, 0FFh
        out     dx, al
        mov     ax, data_seg
        mov     ds, ax
        call    Mouse_Restore_Under_Back
        call    Mouse_Y
        push    ax
        call    Mouse_X
        cmp     ax, mouse_x
        jne     @@moved
        pop     bx
        cmp     bx, mouse_y
        je      @@next
        push    bx
@@moved:
        push    ax
        call    Mouse_Find_Region
        call    Mouse_Restore_Under
        call    Mouse_Save_Under
        call    Mouse_Find_Region
        call    Mouse_Draw_Pointer
        call    Mouse_Set_Position
        add     sp, 4
@@next:
        mov     ds, front_seg
        cmp     si, 16000
        jb      @@chunk
        mov     ax, data_seg
        mov     ds, ax
        call    Mouse_Restore_Under_Back
        call    Mouse_Show
        ret
VGA_Copy_Front_With_Cursor ENDP

; () latch copy displayed -> hidden page, mouse hidden
; C:  void VGA_Copy_Front(void) { Mouse_Hide(); latch_copy(front page, hidden page, 16000); Mouse_Show(); }
VGA_Copy_Front PROC USES ds es si di
        call    Mouse_Hide
        mov     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     bx, 0A400h
        sub     bx, ax
        add     bx, 0A000h
        mov     es, bx
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 0Fh
        out     dx, al
        sub     si, si
        sub     di, di
        mov     ax, 8
        mov     dx, 03CEh
        out     dx, al
        inc     dx
        xor     al, al
        out     dx, al
        mov     cx, 16000
        rep     movsb
        mov     al, 0FFh
        out     dx, al
        call    Mouse_Show
        ret
VGA_Copy_Front ENDP

; () latch copy hidden page -> store A800
; C:  void Screen_Save_A800(void) { latch_copy(hidden page, 0xA800, 16000); }
; Simtex name Copy_Off_To_Back (MoO2 symbols, module video); clarified using ReMoM repo
Screen_Save_A800 PROC USES ds es si di
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     bx, 0A800h
        mov     es, bx
        sub     si, si
        sub     di, di
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 0Fh
        out     dx, al
        mov     ax, 8
        mov     dx, 03CEh
        out     dx, al
        inc     dx
        xor     al, al
        out     dx, al
        mov     cx, 16000
        rep     movsb
        mov     al, 0FFh
        out     dx, al
        ret
Screen_Save_A800 ENDP

; () latch copy store A800 -> hidden page
; C:  void Screen_Restore_A800(void) { latch_copy(0xA800, hidden page, 16000); }
; Simtex name Copy_Back_To_Off (MoO2 symbols, module video); clarified using ReMoM repo
Screen_Restore_A800 PROC USES ds es si di
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     es, ax
        mov     ax, 0A800h
        mov     ds, ax
        sub     si, si
        sub     di, di
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 0Fh
        out     dx, al
        mov     ax, 8
        mov     dx, 03CEh
        out     dx, al
        inc     dx
        xor     al, al
        out     dx, al
        mov     cx, 16000
        rep     movsb
        mov     al, 0FFh
        out     dx, al
        ret
Screen_Restore_A800 ENDP

; () latch copy hidden page -> store AC00
; C:  void Screen_Save_AC00(void) { latch_copy(hidden page, 0xAC00, 16000); }
Screen_Save_AC00 PROC USES ds es si di
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     bx, 0AC00h
        mov     es, bx
        sub     si, si
        sub     di, di
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 0Fh
        out     dx, al
        mov     ax, 8
        mov     dx, 03CEh
        out     dx, al
        inc     dx
        xor     al, al
        out     dx, al
        mov     cx, 16000
        rep     movsb
        mov     al, 0FFh
        out     dx, al
        ret
Screen_Save_AC00 ENDP

; () latch copy store AC00 -> hidden page
; C:  void Screen_Restore_AC00(void) { latch_copy(0xAC00, hidden page, 16000); }
Screen_Restore_AC00 PROC USES ds es si di
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     es, ax
        mov     ax, 0AC00h
        mov     ds, ax
        sub     si, si
        sub     di, di
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 0Fh
        out     dx, al
        mov     ax, 8
        mov     dx, 03CEh
        out     dx, al
        inc     dx
        xor     al, al
        out     dx, al
        mov     cx, 16000
        rep     movsb
        mov     al, 0FFh
        out     dx, al
        ret
Screen_Restore_AC00 ENDP

; (seg) copies 4 planes of 16000 bytes from seg:0 into the draw page (map mask 1, 2, 4, 8). The sequencer index
; must already be 2 (map mask). No callers.
; C:  void VGA_Load_Planar_Image(unsigned seg) { int p; for (p = 0; p < 4; p++) { outportb(0x3C5, 1 << p);
; C:      movedata(seg, p * 16000, draw_page_seg, 0, 16000); } }
VGA_Load_Planar_Image PROC USES ds es si di, image:WORD
        mov     ax, draw_page_seg
        mov     es, ax
        mov     ax, image
        mov     ds, ax
        mov     dx, 03C5h
        mov     al, 1
        out     dx, al
        sub     si, si
        sub     di, di
        mov     cx, 8000
        rep     movsw
        mov     al, 2
        out     dx, al
        sub     di, di
        mov     cx, 8000
        rep     movsw
        mov     al, 4
        out     dx, al
        sub     di, di
        mov     cx, 8000
        rep     movsw
        mov     al, 8
        out     dx, al
        sub     di, di
        mov     cx, 8000
        rep     movsw
        ret
VGA_Load_Planar_Image ENDP

        END
