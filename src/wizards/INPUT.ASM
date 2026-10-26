; INPUT.ASM - WIZARDS.EXE code segment 2579 (MAGIC.EXE 23ed, the same file): mouse and keyboard - the INT 33h driver
; interface and event handler, the pointer regions, saving / restoring the 16 x 16 area under the pointer on both
; VGA pages and drawing the pointer, BIOS key reading with the key translation (Alt + letter through the table at
; the module start), and the developer input recorder (never set up in v1.31).
; Hand-written assembly (Borland TASM). 26 Alt + letter entries in the code segment at 2579:0004.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 2579 src/wizards/INPUT.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   draw_page_index:WORD    ; DS:76ee
        EXTRN   mouse_pointer_pics:WORD ; DS:e824
        .DATA                           ; this module's own data, DS:78b8
mouse_cur_image         dw      0       ; DS:78b8  pointer image of the region under the pointer (0 = none)
mouse_prev_image        dw      0       ; DS:78ba
mouse_draw_enabled      dw      0       ; DS:78bc  1 = the event handler redraws the pointer
mouse_driver_present    dw      0       ; DS:78be
mouse_enabled           dw      0       ; DS:78c0  set by Mouse_Enable, read by Mouse_Available (ReMoM MOUSE_Usable)
mouse_x                 dw      158     ; DS:78c2
mouse_y                 dw      100     ; DS:78c4
mouse_region_count      dw      1       ; DS:78c6
mouse_region_offset     dw      0       ; DS:78c8
mouse_click_pending     dw      0       ; DS:78ca
mouse_click_x           dw      0       ; DS:78cc
mouse_click_y           dw      0       ; DS:78ce
mouse_click_buttons     dw      0       ; DS:78d0
mouse_event_pending     dw      0       ; DS:78d2
mouse_in_handler        dw      0       ; DS:78d4
mouse_saved_once        dw      0       ; DS:78d6  lets Mouse_Save_Under / _Back run once without an image (ReMoM mouse_save_flag)
mouse_saved_x           dw      158     ; DS:78d8  position kept by Mouse_Shutdown for the next Mouse_Init
mouse_saved_y           dw      100     ; DS:78da
mouse_draw_saved        dw      0       ; DS:78dc  mouse_draw_enabled kept by Mouse_Hide
p_mouse_regions         dw      0       ; DS:78de
mouse_under_front       dw      600 dup (0)     ; DS:78e0  saved area: offset, width (words), height, then the planes
mouse_under_back        dw      600 dup (0)     ; DS:7d90
input_record_ptr        dw      0       ; DS:8240  record position (bytes)
input_record_play       dw      0       ; DS:8242  playback position (bytes)
input_record_paused     dw      0       ; DS:8244
input_record_seg        dw      0       ; DS:8246
input_record_base       dw      0       ; DS:8248  events in the buffer
        .CODE
        PUBLIC  input_record_seg, input_record_base       ; read by UI.C (link test, batch 36)
        PUBLIC  Mouse_Set_Cursor_Regions, Mouse_Find_Region, Mouse_Get_Pointer_Image, Mouse_Get_Pointer_Offset
        PUBLIC  Mouse_Get_Buttons, Mouse_Wait_Release, Mouse_Init, Mouse_Enable, Mouse_Shutdown
        PUBLIC  Mouse_Click_Handler_Off, Mouse_Click_Handler_On, Mouse_Event_Handler, Mouse_Hide, Mouse_Show
        PUBLIC  Mouse_Available, Key_Pressed, Key_Read_Translated, Mouse_X, Mouse_Y, Mouse_Set_Position
        PUBLIC  Mouse_Driver_Set_Pos, Mouse_Click_X, Mouse_Click_Y, Mouse_Click_Buttons, Mouse_Take_Click
        PUBLIC  Mouse_Take_Event, Mouse_Post_Move, Mouse_Post_Click, Mouse_Handler_Buttons, Mouse_Save_Under
        PUBLIC  Mouse_Save_Under_Back, Mouse_Cursor_Setup, Mouse_Copy_Save_Buffer, Mouse_Restore_Under
        PUBLIC  Mouse_Restore_Under_Back, Mouse_Restore_Under_Alt, Mouse_Draw_Pointer, Mouse_Draw_Pointer_Back
        PUBLIC  Mouse_Pointer_Blit, Mouse_Pointer_Blit_Back, Input_Record_Clear, Input_Record_Off
        PUBLIC  Input_Record_Pause, Input_Record_Close, Input_Record_Seek_End, Input_Record_Paused
        PUBLIC  Input_Record_Event, Input_Record_Play_Next
alt_keys:                               ; 2579:0004  (BIOS key, code): Alt + A .. Z -> A0h ..
        dw      01E00h, 000A0h
        dw      03000h, 000A1h
        dw      02E00h, 000A2h
        dw      02000h, 000A3h
        dw      01200h, 000A4h
        dw      02100h, 000A5h
        dw      02200h, 000A6h
        dw      02300h, 000A7h
        dw      01700h, 000A8h
        dw      02400h, 000A9h
        dw      02500h, 000AAh
        dw      02600h, 000ABh
        dw      03200h, 000ACh
        dw      03100h, 000ADh
        dw      01800h, 000AEh
        dw      01900h, 000AFh
        dw      01000h, 000B0h
        dw      01300h, 000B1h
        dw      01F00h, 000B2h
        dw      01400h, 000B3h
        dw      01600h, 000B4h
        dw      02F00h, 000B5h
        dw      01100h, 000B6h
        dw      02D00h, 000B7h
        dw      01500h, 000B8h
alt_keys_last:
        dw      02C00h, 000B9h
; (count, regions) sets the pointer region table (12-byte MouseRegion records; the first is the default) and takes
; the last region with a zero corner (or the first) as the current one
; C:  void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions)
; Simtex name Set_Mouse_List (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Set_Cursor_Regions PROC USES ds, rcount:WORD, rregions:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     cx, rcount
        mov     mouse_region_count, cx
        sub     ax, ax
        mov     bx, rregions
        mov     p_mouse_regions, bx
        dec     cx
        je      @@found
        mov     dx, cx
        mov     ax, 0Ch
        mul     dx
        add     bx, ax
@@scan:
        sub     ax, ax
        cmp     ax, [bx+4]
        jne     @@prev
        cmp     ax, [bx+6]
        je      @@found
@@prev:
        sub     bx, 0Ch
        loop    @@scan
@@found:
        mov     ax, [bx+2]
        mov     mouse_region_offset, ax
        mov     ax, [bx]
        mov     mouse_cur_image, ax
        ret
Mouse_Set_Cursor_Regions ENDP

; (x, y) the last region (from the end, the first = default) containing (x, y) becomes current; the old image is
; kept in mouse_prev_image
; Simtex name Check_Mouse_Shape (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Find_Region PROC USES ds es, fx:WORD, fy:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     es, ax
        mov     ax, mouse_cur_image
        mov     mouse_prev_image, ax
        mov     bx, p_mouse_regions
        mov     cx, mouse_region_count
        dec     cx
        je      @@found
        mov     dx, cx
        mov     ax, 0Ch
        mul     dx
        add     bx, ax
@@scan:
        mov     ax, [bx+4]
        cmp     fx, ax
        jb      @@prev
        mov     ax, [bx+6]
        cmp     fy, ax
        jb      @@prev
        mov     ax, [bx+8]
        cmp     ax, fx
        jb      @@prev
        mov     ax, [bx+0Ah]
        cmp     ax, fy
        jae     @@found
@@prev:
        sub     bx, 0Ch
        loop    @@scan
@@found:
        mov     ax, [bx+2]
        mov     mouse_region_offset, ax
        mov     ax, [bx]
        mov     mouse_cur_image, ax
        ret
Mouse_Find_Region ENDP

; Simtex name Get_Pointer_Image_Number (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Get_Pointer_Image PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_cur_image
        pop     ds
        ret
Mouse_Get_Pointer_Image ENDP

; Simtex name Get_Pointer_Offset (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Get_Pointer_Offset PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_region_offset
        pop     ds
        ret
Mouse_Get_Pointer_Offset ENDP

; () the button state (bit 0 left, bit 1 right); 0 without a driver
; Simtex name Mouse_Button (MoO2 symbols, module mouse); clarified using ReMoM repo
; Numbered: BUG #338 (BUGS_AND_QUIRKS.txt)
Mouse_Get_Buttons PROC
        push    es
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_driver_present, 0
        je      @@done
        mov     ax, 3
        mov     bx, 0
        mov     cx, 0
        mov     dx, 0
        cli
        int     33h
        sti
        and     bx, 3
        mov     ax, bx
@@done:
        pop     ds
        pop     es
        ret
Mouse_Get_Buttons ENDP

; () waits until no button is down
; Simtex name Mouse_Wait (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Wait_Release PROC
        push    si
        push    di
        push    es
        push    ds
@@wait:
        mov     ax, 3
        mov     bx, 0
        mov     cx, 0
        mov     dx, 0
        cli
        int     33h
        sti
        and     bx, 3
        jne     @@wait
        mov     ax, bx
        pop     ds
        pop     es
        pop     di
        pop     si
        ret
Mouse_Wait_Release ENDP

; () resets the driver (returns with no change when there is none), restores the saved position, limits x 0..638
; and y 0..199, waits for the buttons, installs Mouse_Event_Handler for movement (mask 1). Returns -1 (DS = DGROUP
; assumed)
; Simtex name Init_Mouse_Driver (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Init PROC USES si di es ds
        LOCAL   unused:WORD
        cli
        mov     ax, 0
        mov     bx, 0
        mov     cx, 0
        mov     dx, 0
        int     33h
        cmp     ax, 0
        jne     @@found
        sti
        ret
@@found:
        mov     ax, 3
        int     33h
        mov     mouse_driver_present, 1
        mov     mouse_in_handler, 0
        mov     dx, mouse_saved_y
        mov     cx, mouse_saved_x
        mov     mouse_x, cx
        mov     mouse_y, dx
        mov     ax, 7
        mov     bx, 0
        mov     cx, 0
        mov     dx, 27Eh
        int     33h
        mov     ax, 8
        mov     bx, 0
        mov     cx, 0
        mov     dx, 0C7h
        int     33h
        call    Mouse_Wait_Release
        mov     ax, SEG Mouse_Event_Handler
        mov     es, ax
        mov     ax, 0Ch
        mov     cx, 1
        mov     dx, offset Mouse_Event_Handler
        int     33h
        mov     mouse_draw_enabled, 0
        mov     mouse_click_pending, 0
        mov     mouse_event_pending, 0
        mov     ax, 0FFFFh
        sti
        ret
Mouse_Init ENDP

Mouse_Enable PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     mouse_enabled, 1
        pop     ds
        ret
Mouse_Enable ENDP

; () waits for the buttons, keeps the position for the next Mouse_Init and resets the driver (INT 33h / 21h)
; Simtex name Reset_System_Mouse (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Shutdown PROC
        push    si
        push    di
        push    es
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_driver_present, 0
        je      @@off
        call    Mouse_Wait_Release
        mov     mouse_draw_enabled, 0
        mov     mouse_click_pending, 0
        mov     mouse_event_pending, 0
        call    far ptr Mouse_X
        mov     mouse_saved_x, ax
        call    far ptr Mouse_Y
        mov     mouse_saved_y, ax
        cli
        mov     ax, 21h
        int     33h
        sti
@@off:
        mov     mouse_driver_present, 0
        pop     ds
        pop     es
        pop     di
        pop     si
        ret
Mouse_Shutdown ENDP

; () the event handler for movement only (INT 33h / 14h, mask 1)
; Simtex name Mouse_Movement_Handler (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Click_Handler_Off PROC
        push    si
        push    di
        push    es
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_driver_present, 0
        je      @@done
        cli
        mov     ax, SEG Mouse_Event_Handler
        mov     es, ax
        mov     ax, 14h
        mov     cx, 1
        mov     dx, offset Mouse_Event_Handler
        int     33h
        mov     ax, 0FFFFh
        mov     mouse_draw_enabled, 1
        mov     ax, 3
        int     33h
        sti
@@done:
        pop     ds
        pop     es
        pop     di
        pop     si
        ret
Mouse_Click_Handler_Off ENDP

; () the event handler for movement and the button presses / releases (mask 2Bh)
; Simtex name Mouse_Button_Handler (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Click_Handler_On PROC
        push    si
        push    di
        push    es
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_driver_present, 0
        je      @@done
        cli
        mov     ax, SEG Mouse_Event_Handler
        mov     es, ax
        mov     ax, 14h
        mov     cx, 2Bh
        mov     dx, offset Mouse_Event_Handler
        int     33h
        mov     ax, 0FFFFh
        mov     mouse_draw_enabled, 1
        mov     ax, 3
        int     33h
        sti
@@done:
        pop     ds
        pop     es
        pop     di
        pop     si
        ret
Mouse_Click_Handler_On ENDP

; the INT 33h event handler (CX = x * 2, DX = y, BX = buttons): stores the position, latches clicks
; (Mouse_Handler_Buttons) and, when drawing is enabled, moves the pointer on the displayed page with the VGA
; sequencer / graphics registers saved and restored
Mouse_Event_Handler PROC
        LOCAL   unused2:WORD, seq_mask:WORD, gc_readmap:WORD, gc_bitmask:WORD, gc_index:WORD
        cli
        pushf
        push    ds
        push    ax
        cld
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, cx
        shr     ax, 1
        mov     mouse_x, ax
        mov     mouse_y, dx
        cmp     mouse_in_handler, 0
        je      @@enter
        jmp     @@leave
@@enter:
        push    si
        push    di
        push    es
        push    bx
        push    cx
        push    dx
        mov     mouse_in_handler, 1
        push    bx
        push    dx
        shr     cx, 1
        push    cx
        call    far ptr Mouse_Handler_Buttons
        add     sp, 6
        cmp     mouse_draw_enabled, 0
        jne     @@draw
        jmp     @@restore
@@draw:
        mov     mouse_draw_enabled, 0
        mov     bx, dx
        mov     dx, 03C4h
        mov     ax, 2
        out     dx, al
        inc     dx
        in      al, dx
        mov     seq_mask, ax
        mov     dx, 03CEh
        in      al, dx
        mov     gc_index, ax
        mov     ax, 4
        out     dx, al
        inc     dx
        in      al, dx
        mov     gc_readmap, ax
        mov     dx, 03CEh
        mov     ax, 8
        out     dx, al
        inc     dx
        in      al, dx
        mov     gc_bitmask, ax
        mov     al, 0FFh
        out     dx, al
        push    bx
        push    cx
        mov     ax, mouse_region_count
        cmp     ax, 2
        jb      @@one_region
        call    Mouse_Find_Region
@@one_region:
        call    far ptr Mouse_Restore_Under
        call    far ptr Mouse_Save_Under
        call    far ptr Mouse_Draw_Pointer
        add     sp, 4
        mov     dx, 03C4h
        mov     ax, 2
        out     dx, al
        inc     dx
        mov     ax, seq_mask
        out     dx, al
        mov     dx, 03CEh
        mov     ax, 4
        out     dx, al
        inc     dx
        mov     ax, gc_readmap
        out     dx, al
        mov     dx, 03CEh
        mov     ax, 8
        out     dx, al
        inc     dx
        mov     ax, gc_bitmask
        out     dx, al
        mov     dx, 03CEh
        mov     ax, gc_index
        out     dx, al
        mov     mouse_draw_enabled, 1
@@restore:
        pop     dx
        pop     cx
        pop     bx
        pop     es
        pop     di
        pop     si
        mov     mouse_in_handler, 0
@@leave:
        pop     ax
        pop     ds
        popf
        sti
        ret
Mouse_Event_Handler ENDP

; Simtex name Save_Mouse_State (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Hide PROC
        push    ds
        push    bx
        mov     bx, DGROUP
        mov     ds, bx
        mov     bx, mouse_draw_enabled
        mov     mouse_draw_saved, bx
        sub     bx, bx
        mov     mouse_draw_enabled, bx
        pop     bx
        pop     ds
        ret
Mouse_Hide ENDP

; Simtex name Restore_Mouse_State (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Show PROC
        push    ds
        push    bx
        mov     bx, DGROUP
        mov     ds, bx
        mov     bx, mouse_draw_saved
        mov     mouse_draw_enabled, bx
        pop     bx
        pop     ds
        ret
Mouse_Show ENDP

; () named from its body - no caller (no Ghidra function, 2579:03c6), not in ReMoM: mouse_driver_present, or mouse_enabled when there is none
Mouse_Available PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_driver_present
        cmp     ax, 0
        je      @@none
        pop     ds
        ret
@@none:
        mov     ax, mouse_enabled
        pop     ds
        ret
Mouse_Available ENDP

; () 1 when a key is waiting (INT 16h / 11h), the pointer hidden meanwhile
; Simtex name Keyboard_Status (MoO2 symbols, module key); clarified using ReMoM repo
Key_Pressed PROC
        push    ds
        push    es
        call    Mouse_Hide
        mov     ah, 11h
        int     16h
        je      @@none
        mov     ax, 1
        call    Mouse_Show
        pop     es
        pop     ds
        ret
@@none:
        mov     ax, 0
        call    Mouse_Show
        pop     es
        pop     ds
        ret
Key_Pressed ENDP

; () waits for a key (INT 16h / 10h) and returns the game's key code: Esc 1Bh, the arrows / keypad 1-9 and the
; grey keys, Backspace 0Bh, Enter 0Ch, Tab 0Dh, F1-F10 0Eh-17h, Ctrl + arrows / Home / End etc. 80h-87h, Alt-F11
; 90h-92h, Alt + letter through alt_keys; anything else as the BIOS gives it
; Simtex name Read_Key (MoO2 symbols, module key); clarified using ReMoM repo
Key_Read_Translated PROC
        push    es
        push    ds
        mov     ah, 10h
        int     16h
        cmp     ax, 11Bh
        jne     @@k0
        mov     ax, 1Bh
        pop     ds
        pop     es
        ret
@@k0:
        cmp     ax, 4BE0h
        jne     @@k1
        mov     ax, 1
        pop     ds
        pop     es
        ret
@@k1:
        cmp     ax, 4DE0h
        jne     @@k2
        mov     ax, 2
        pop     ds
        pop     es
        ret
@@k2:
        cmp     ax, 48E0h
        jne     @@k3
        mov     ax, 3
        pop     ds
        pop     es
        ret
@@k3:
        cmp     ax, 50E0h
        jne     @@k4
        mov     ax, 4
        pop     ds
        pop     es
        ret
@@k4:
        cmp     ax, 49E0h
        jne     @@k5
        mov     ax, 5
        pop     ds
        pop     es
        ret
@@k5:
        cmp     ax, 51E0h
        jne     @@k6
        mov     ax, 6
        pop     ds
        pop     es
        ret
@@k6:
        cmp     ax, 47E0h
        jne     @@k7
        mov     ax, 7
        pop     ds
        pop     es
        ret
@@k7:
        cmp     ax, 4FE0h
        jne     @@k8
        mov     ax, 8
        pop     ds
        pop     es
        ret
@@k8:
        cmp     ax, 52E0h
        jne     @@k9
        mov     ax, 9
        pop     ds
        pop     es
        ret
@@k9:
        cmp     ax, 53E0h
        jne     @@k10
        mov     ax, 0Ah
        pop     ds
        pop     es
        ret
@@k10:
        cmp     ax, 0E08h
        jne     @@k11
        mov     ax, 0Bh
        pop     ds
        pop     es
        ret
@@k11:
        cmp     ax, 1C0Dh
        jne     @@k12
        mov     ax, 0Ch
        pop     ds
        pop     es
        ret
@@k12:
        cmp     ax, 0F09h
        jne     @@k13
        mov     ax, 0Dh
        pop     ds
        pop     es
        ret
@@k13:
        cmp     ax, 3B00h
        jne     @@k14
        mov     ax, 0Eh
        pop     ds
        pop     es
        ret
@@k14:
        cmp     ax, 3C00h
        jne     @@k15
        mov     ax, 0Fh
        pop     ds
        pop     es
        ret
@@k15:
        cmp     ax, 3D00h
        jne     @@k16
        mov     ax, 10h
        pop     ds
        pop     es
        ret
@@k16:
        cmp     ax, 3E00h
        jne     @@k17
        mov     ax, 11h
        pop     ds
        pop     es
        ret
@@k17:
        cmp     ax, 3F00h
        jne     @@k18
        mov     ax, 12h
        pop     ds
        pop     es
        ret
@@k18:
        cmp     ax, 4000h
        jne     @@k19
        mov     ax, 13h
        pop     ds
        pop     es
        ret
@@k19:
        cmp     ax, 4100h
        jne     @@k20
        mov     ax, 14h
        pop     ds
        pop     es
        ret
@@k20:
        cmp     ax, 4200h
        jne     @@k21
        mov     ax, 15h
        pop     ds
        pop     es
        ret
@@k21:
        cmp     ax, 4300h
        jne     @@k22
        mov     ax, 16h
        pop     ds
        pop     es
        ret
@@k22:
        cmp     ax, 4400h
        jne     @@k23
        mov     ax, 17h
        pop     ds
        pop     es
        ret
@@k23:
        cmp     ah, 47h
        jne     @@k24
        mov     ax, 1Ah
        pop     ds
        pop     es
        ret
@@k24:
        cmp     ah, 49h
        jne     @@k25
        mov     ax, 19h
        pop     ds
        pop     es
        ret
@@k25:
        cmp     ah, 4Fh
        jne     @@k26
        mov     ax, 1Ch
        pop     ds
        pop     es
        ret
@@k26:
        cmp     ah, 51h
        jne     @@k27
        mov     ax, 1Dh
        pop     ds
        pop     es
        ret
@@k27:
        cmp     ah, 48h
        jne     @@k28
        mov     ax, 3
        pop     ds
        pop     es
        ret
@@k28:
        cmp     ah, 50h
        jne     @@k29
        mov     ax, 4
        pop     ds
        pop     es
        ret
@@k29:
        cmp     ah, 4Bh
        jne     @@k30
        mov     ax, 1
        pop     ds
        pop     es
        ret
@@k30:
        cmp     ah, 4Dh
        jne     @@k31
        mov     ax, 2
        pop     ds
        pop     es
        ret
@@k31:
        cmp     ah, 4Eh
        jne     @@k32
        mov     ax, 2Bh
        pop     ds
        pop     es
        ret
@@k32:
        cmp     ah, 4Ah
        jne     @@k33
        mov     ax, 2Dh
        pop     ds
        pop     es
        ret
@@k33:
        cmp     ah, 4Ch
        jne     @@k34
        mov     ax, 1Eh
        pop     ds
        pop     es
        ret
@@k34:
        cmp     ax, 74E0h
        jne     @@k35
        mov     ax, 80h
        pop     ds
        pop     es
        ret
@@k35:
        cmp     ax, 73E0h
        jne     @@k36
        mov     ax, 81h
        pop     ds
        pop     es
        ret
@@k36:
        cmp     ax, 8DE0h
        jne     @@k37
        mov     ax, 82h
        pop     ds
        pop     es
        ret
@@k37:
        cmp     ax, 91E0h
        jne     @@k38
        mov     ax, 83h
        pop     ds
        pop     es
        ret
@@k38:
        cmp     ax, 7400h
        jne     @@k39
        mov     ax, 80h
        pop     ds
        pop     es
        ret
@@k39:
        cmp     ax, 7300h
        jne     @@k40
        mov     ax, 81h
        pop     ds
        pop     es
        ret
@@k40:
        cmp     ax, 8D00h
        jne     @@k41
        mov     ax, 82h
        pop     ds
        pop     es
        ret
@@k41:
        cmp     ax, 9100h
        jne     @@k42
        mov     ax, 83h
        pop     ds
        pop     es
        ret
@@k42:
        cmp     ax, 7700h
        jne     @@k43
        mov     ax, 84h
        pop     ds
        pop     es
        ret
@@k43:
        cmp     ax, 8400h
        jne     @@k44
        mov     ax, 85h
        pop     ds
        pop     es
        ret
@@k44:
        cmp     ax, 7500h
        jne     @@k45
        mov     ax, 86h
        pop     ds
        pop     es
        ret
@@k45:
        cmp     ax, 7600h
        jne     @@k46
        mov     ax, 87h
        pop     ds
        pop     es
        ret
@@k46:
        cmp     ax, 092Ah
        jne     @@k47
        sub     ax, ax
        pop     ds
        pop     es
        ret
@@k47:
        cmp     ax, 0E00Dh
        jne     @@k48
        mov     ax, 0Ch
        pop     ds
        pop     es
        ret
@@k48:
        cmp     ax, 1F13h
        jne     @@k49
        mov     ax, 90h
        pop     ds
        pop     es
        ret
@@k49:
        cmp     ax, 8500h
        jne     @@k50
        mov     ax, 91h
        pop     ds
        pop     es
        ret
@@k50:
        cmp     ax, 8600h
        jne     @@lookup
        mov     ax, 92h
@@lookup:
        mov     bx, offset alt_keys
        mov     dx, offset alt_keys_last
        add     dx, 4
@@search:
        mov     cx, cs:[bx]
        cmp     ax, cx
        je      @@found
        add     bx, 4
        cmp     bx, dx
        je      @@done
        jmp     @@search
@@found:
        inc     bx
        inc     bx
        mov     ax, cs:[bx]
@@done:
        pop     ds
        pop     es
        ret
Key_Read_Translated ENDP

; Simtex name Pointer_X (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_X PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_x
        pop     ds
        ret
Mouse_X ENDP

; Simtex name Pointer_Y (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Y PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_y
        pop     ds
        ret
Mouse_Y ENDP

; (x, y) sets the pointer position (and the driver's, if there is one)
; Simtex name Set_Pointer_Position (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Set_Position PROC USES ds, px:WORD, py:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, px
        mov     mouse_x, ax
        mov     ax, py
        mov     mouse_y, ax
        cmp     mouse_driver_present, 0
        je      @@done
        push    ax
        push    px
        call    far ptr Mouse_Driver_Set_Pos
        add     sp, 4
@@done:
        ret
Mouse_Set_Position ENDP

; Simtex name Set_Mouse_Position (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Driver_Set_Pos PROC USES ds, dpx:WORD, dpy:WORD
        mov     cx, dpx
        shl     cx, 1
        mov     dx, dpy
        mov     ax, 4
        cli
        int     33h
        sti
        ret
Mouse_Driver_Set_Pos ENDP

; Simtex name Mouse_Buffer_X (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Click_X PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_click_x
        pop     ds
        ret
Mouse_Click_X ENDP

; Simtex name Mouse_Buffer_Y (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Click_Y PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_click_y
        pop     ds
        ret
Mouse_Click_Y ENDP

; Simtex name Mouse_Buffer_Button (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Click_Buttons PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_click_buttons
        pop     ds
        ret
Mouse_Click_Buttons ENDP

; Simtex name Mouse_Buffer (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Take_Click PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_click_pending
        sub     bx, bx
        mov     mouse_click_pending, bx
        pop     ds
        ret
Mouse_Take_Click ENDP

; Simtex name Mouse_Buffer2 (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Take_Event PROC
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, mouse_event_pending
        sub     bx, bx
        mov     mouse_event_pending, bx
        pop     ds
        ret
Mouse_Take_Event ENDP

; Simtex name Set_Buffer_2 (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Post_Move PROC USES ds, mx:WORD, my:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     mouse_event_pending, 1
        mov     ax, mx
        mov     mouse_click_x, ax
        mov     ax, my
        mov     mouse_click_y, ax
        ret
Mouse_Post_Move ENDP

; Simtex name Set_Mouse_Buffer (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Post_Click PROC USES ds, cx_:WORD, cy:WORD, cbuttons:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     mouse_event_pending, 1
        mov     mouse_click_pending, 1
        mov     ax, cx_
        mov     mouse_click_x, ax
        mov     ax, cy
        mov     mouse_click_y, ax
        mov     ax, cbuttons
        mov     mouse_click_buttons, ax
        ret
Mouse_Post_Click ENDP

; (x, y, buttons) from the event handler: a button down latches a click at (x, y)
; Simtex name Check_Mouse_Buffer (MoO2 symbols, module mouse); clarified using ReMoM repo
Mouse_Handler_Buttons PROC USES ds, hx:WORD, hy:WORD, hbuttons:WORD
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, hbuttons
        and     ax, 3
        cmp     ax, 0
        jne     @@click
        pop     ds
        ret
@@click:
        mov     mouse_click_buttons, ax
        mov     mouse_click_pending, 1
        mov     mouse_event_pending, 1
        mov     ax, hx
        mov     mouse_click_x, ax
        mov     ax, hy
        mov     mouse_click_y, ax
        sub     ax, ax
        pop     ds
        ret
Mouse_Handler_Buttons ENDP

; (x, y) saves the area under a pointer at (x, y) on the displayed page into mouse_under_front: offset, width
; in words (3, less at the right edge), height (16, less at the bottom), then per plane the rows; without a
; pointer image only the first time
Mouse_Save_Under PROC USES si di es ds, sx:WORD, sy:WORD
        LOCAL   start:WORD, height:WORD, cols:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_cur_image, 0
        jne     @@save
        cmp     mouse_saved_once, 0
        je      @@first
        ret
@@first:
        mov     mouse_saved_once, 1
@@save:
        mov     ax, ds
        mov     es, ax
        mov     ax, draw_page_index
        mov     ah, al
        sub     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     si, sx
        shr     si, 1
        shr     si, 1
        mov     ax, si
        mov     bx, 3
        add     ax, bx
        sub     ax, 50h
        js      @@width_ok
        sub     bx, ax
@@width_ok:
        mov     ax, sy
        mov     cx, 50h
        mul     cx
        add     si, ax
        sub     di, di
        mov     word ptr es:[di+mouse_under_front], si
        add     di, 2
        mov     word ptr es:[di+mouse_under_front], bx
        add     di, 2
        mov     byte ptr cols, bl
        mov     ax, sy
        mov     bx, 10h
        add     ax, bx
        sub     ax, 0C8h
        js      @@height_ok
        sub     bx, ax
@@height_ok:
        mov     word ptr es:[di+mouse_under_front], bx
        add     di, 2
        mov     height, bx
        mov     start, si
        sub     bl, bl
        mov     al, 4
        mov     dx, 03CEh
        out     dx, al
        inc     dx
@@plane:
        mov     al, bl
        out     dx, al
        sub     bh, bh
@@column:
        mov     cx, height
@@row:
        lodsw
        mov     word ptr es:[di+mouse_under_front], ax
        add     di, 2
        add     si, 4Eh
        loop    @@row
        mov     si, start
        inc     bh
        sub     ah, ah
        mov     al, bh
        shl     ax, 1
        add     si, ax
        cmp     bh, byte ptr cols
        jne     @@column
        inc     bl
        mov     si, start
        cmp     bl, 4
        jne     @@plane
        ret
Mouse_Save_Under ENDP

; (x, y) the same for the other page into mouse_under_back (also when only the previous region had an image)
Mouse_Save_Under_Back PROC USES si di es ds, sx:WORD, sy:WORD
        LOCAL   start:WORD, height:WORD, cols:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_cur_image, 0
        jne     @@save
        cmp     mouse_prev_image, 0
        jne     @@save
        cmp     mouse_saved_once, 0
        je      @@first
        ret
@@first:
        mov     mouse_saved_once, 1
@@save:
        mov     ax, ds
        mov     es, ax
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     si, sx
        shr     si, 1
        shr     si, 1
        mov     ax, si
        mov     bx, 3
        add     ax, bx
        sub     ax, 50h
        js      @@width_ok
        sub     bx, ax
@@width_ok:
        mov     ax, sy
        mov     cx, 50h
        mul     cx
        add     si, ax
        sub     di, di
        mov     word ptr es:[di+mouse_under_back], si
        add     di, 2
        mov     word ptr es:[di+mouse_under_back], bx
        add     di, 2
        mov     byte ptr cols, bl
        mov     ax, sy
        mov     bx, 10h
        add     ax, bx
        sub     ax, 0C8h
        js      @@height_ok
        sub     bx, ax
@@height_ok:
        mov     word ptr es:[di+mouse_under_back], bx
        add     di, 2
        mov     height, bx
        mov     start, si
        sub     bl, bl
        mov     al, 4
        mov     dx, 03CEh
        out     dx, al
        inc     dx
@@plane:
        mov     al, bl
        out     dx, al
        sub     bh, bh
@@column:
        mov     cx, height
@@row:
        lodsw
        mov     word ptr es:[di+mouse_under_back], ax
        add     di, 2
        add     si, 4Eh
        loop    @@row
        mov     si, start
        inc     bh
        sub     ah, ah
        mov     al, bh
        shl     ax, 1
        add     si, ax
        cmp     bh, byte ptr cols
        jne     @@column
        inc     bl
        mov     si, start
        cmp     bl, 4
        jne     @@plane
        ret
Mouse_Save_Under_Back ENDP

; (x, y) Mouse_Save_Under without the image checks
Mouse_Cursor_Setup PROC USES si di es ds, sx:WORD, sy:WORD
        LOCAL   start:WORD, height:WORD, cols:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, ds
        mov     es, ax
        mov     ax, draw_page_index
        mov     ah, al
        sub     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     ds, ax
        mov     si, sx
        shr     si, 1
        shr     si, 1
        mov     ax, si
        mov     bx, 3
        add     ax, bx
        sub     ax, 50h
        js      @@width_ok
        sub     bx, ax
@@width_ok:
        mov     ax, sy
        mov     cx, 50h
        mul     cx
        add     si, ax
        sub     di, di
        mov     word ptr es:[di+mouse_under_front], si
        add     di, 2
        mov     word ptr es:[di+mouse_under_front], bx
        add     di, 2
        mov     byte ptr cols, bl
        mov     ax, sy
        mov     bx, 10h
        add     ax, bx
        sub     ax, 0C8h
        js      @@height_ok
        sub     bx, ax
@@height_ok:
        mov     word ptr es:[di+mouse_under_front], bx
        add     di, 2
        mov     height, bx
        mov     start, si
        sub     bl, bl
        mov     al, 4
        mov     dx, 03CEh
        out     dx, al
        inc     dx
@@plane:
        mov     al, bl
        out     dx, al
        sub     bh, bh
@@column:
        mov     cx, height
@@row:
        lodsw
        mov     word ptr es:[di+mouse_under_front], ax
        add     di, 2
        add     si, 4Eh
        loop    @@row
        mov     si, start
        inc     bh
        sub     ah, ah
        mov     al, bh
        shl     ax, 1
        add     si, ax
        cmp     bh, byte ptr cols
        jne     @@column
        inc     bl
        mov     si, start
        cmp     bl, 4
        jne     @@plane
        ret
Mouse_Cursor_Setup ENDP
; () copies the back-page save buffer over the front one
Mouse_Copy_Save_Buffer PROC
        push    si
        push    di
        push    es
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        mov     es, ax
        mov     di, offset mouse_under_front
        mov     si, offset mouse_under_back
        mov     cx, 258h
        rep     movsw
        pop     ds
        pop     es
        pop     di
        pop     si
        ret
Mouse_Copy_Save_Buffer ENDP

; () puts the area saved in mouse_under_front back on the displayed page (when the previous region had an image)
Mouse_Restore_Under PROC USES si di es ds
        LOCAL   start:WORD, height:WORD, cols:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_prev_image, 0
        jne     @@restore
        ret
@@restore:
        sub     si, si
        mov     di, [si+mouse_under_front]
        add     si, 2
        mov     start, di
        mov     ax, [si+mouse_under_front]
        add     si, 2
        mov     byte ptr cols, al
        mov     ax, [si+mouse_under_front]
        add     si, 2
        mov     height, ax
        mov     ax, draw_page_index
        mov     ah, al
        sub     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     es, ax
        mov     bl, 1
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
@@plane:
        mov     al, bl
        out     dx, al
        sub     bh, bh
@@column:
        mov     cx, height
@@row:
        mov     ax, [si+mouse_under_front]
        stosw
        add     si, 2
        add     di, 4Eh
        loop    @@row
        mov     di, start
        inc     bh
        sub     ah, ah
        mov     al, bh
        shl     ax, 1
        add     di, ax
        cmp     bh, byte ptr cols
        jne     @@column
        shl     bl, 1
        mov     di, start
        cmp     bl, 10h
        jne     @@plane
        ret
Mouse_Restore_Under ENDP

; () the same on the other page - but from mouse_under_front, not mouse_under_back (#400)
; Numbered: BUG #400 (BUGS_AND_QUIRKS.txt)
Mouse_Restore_Under_Back PROC USES si di es ds
        LOCAL   start:WORD, height:WORD, cols:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_prev_image, 0
        jne     @@restore
        ret
@@restore:
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        mov     es, ax
        sub     si, si
        mov     di, [si+mouse_under_front]
        add     si, 2
        mov     start, di
        mov     ax, [si+mouse_under_front]
        add     si, 2
        mov     byte ptr cols, al
        mov     ax, [si+mouse_under_front]
        add     si, 2
        mov     height, ax
        mov     bl, 1
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
@@plane:
        mov     al, bl
        out     dx, al
        sub     bh, bh
@@column:
        mov     cx, height
@@row:
        mov     ax, [si+mouse_under_front]
        stosw
        add     si, 2
        add     di, 4Eh
        loop    @@row
        mov     di, start
        inc     bh
        sub     ah, ah
        mov     al, bh
        shl     ax, 1
        add     di, ax
        cmp     bh, byte ptr cols
        jne     @@column
        shl     bl, 1
        mov     di, start
        cmp     bl, 10h
        jne     @@plane
        ret
Mouse_Restore_Under_Back ENDP

; () the same on the page at A800h
Mouse_Restore_Under_Alt PROC USES si di es ds
        LOCAL   start:WORD, height:WORD, cols:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     mouse_prev_image, 0
        jne     @@restore
        ret
@@restore:
        sub     si, si
        mov     di, [si+mouse_under_front]
        add     si, 2
        mov     start, di
        mov     ax, [si+mouse_under_front]
        add     si, 2
        mov     byte ptr cols, al
        mov     ax, [si+mouse_under_front]
        add     si, 2
        mov     height, ax
        mov     ax, 0A800h
        mov     es, ax
        mov     bl, 1
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
@@plane:
        mov     al, bl
        out     dx, al
        sub     bh, bh
@@column:
        mov     cx, height
@@row:
        mov     ax, [si+mouse_under_front]
        stosw
        add     si, 2
        add     di, 4Eh
        loop    @@row
        mov     di, start
        inc     bh
        sub     ah, ah
        mov     al, bh
        shl     ax, 1
        add     di, ax
        cmp     bh, byte ptr cols
        jne     @@column
        shl     bl, 1
        mov     di, start
        cmp     bl, 10h
        jne     @@plane
        ret
Mouse_Restore_Under_Alt ENDP
; (x, y) draws the current pointer image at (x, y) on the draw page
Mouse_Draw_Pointer PROC USES si di es ds, dx_:WORD, dy:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     dx, mouse_cur_image
        cmp     dx, 0
        je      @@done
        mov     bx, dx_
        mov     cx, dy
        call    far ptr Mouse_Pointer_Blit
@@done:
        ret
Mouse_Draw_Pointer ENDP

; (x, y) the same on the other page
Mouse_Draw_Pointer_Back PROC USES si di es ds, bdx:WORD, bdy:WORD
        mov     ax, DGROUP
        mov     ds, ax
        mov     dx, mouse_cur_image
        cmp     dx, 0
        je      @@done
        mov     bx, bdx
        mov     cx, bdy
        call    far ptr Mouse_Pointer_Blit_Back
@@done:
        ret
Mouse_Draw_Pointer_Back ENDP

; (BX = x, CX = y, DX = pointer image 1..) draws the 16 x 16 pointer (color 0 transparent) on the displayed
; page, clipped at the right and bottom edges
Mouse_Pointer_Blit PROC USES es ds si di
        LOCAL   pic_seg:WORD, rows:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     dx, 0
        jne     @@draw
        ret
@@draw:
        dec     dx
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        add     dx, mouse_pointer_pics
        mov     pic_seg, dx
        mov     dx, cx
        shl     dx, 1
        shl     dx, 1
        add     dx, cx
        mov     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, dx
        add     ax, 0A000h
        mov     es, ax
        mov     ax, 0C8h
        sub     ax, cx
        jbe     @@done
        cmp     ax, 11h
        js      @@rows_ok
        mov     ax, 10h
@@rows_ok:
        mov     rows, ax
        mov     ax, bx
        mov     ah, 1
        and     al, 3
        je      @@plane0
        mov     cl, al
        xor     ch, ch
@@shift:
        shl     ah, 1
        loop    @@shift
@@plane0:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     dx, bx
        shr     dx, 1
        shr     dx, 1
        mov     cx, 140h
        sub     cx, bx
        jbe     @@done
        cmp     cx, 11h
        js      @@cols_ok
        mov     cx, 10h
@@cols_ok:
        mov     bl, cl
        mov     bh, byte ptr rows
        mov     si, 0
        mov     cx, pic_seg
        mov     ds, cx
@@column:
        mov     di, dx
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
        mov     cl, bh
        xor     ch, ch
@@pixel:
        lodsb
        cmp     al, 0
        jne     @@opaque
        add     di, 50h
        loop    @@pixel
        jmp     @@next
@@opaque:
        stosb
        add     di, 4Fh
        loop    @@pixel
@@next:
        mov     cl, 10h
        sub     cl, bh
        xor     ch, ch
        add     si, cx
        dec     bl
        je      @@done
        shl     ah, 1
        cmp     ah, 9
        js      @@same
        inc     dx
        mov     ah, 1
@@same:
        jmp     @@column
@@done:
        ret
Mouse_Pointer_Blit ENDP

; the same on the other page
Mouse_Pointer_Blit_Back PROC USES es ds si di
        LOCAL   pic_seg:WORD, rows:WORD
        mov     ax, DGROUP
        mov     ds, ax
        cmp     dx, 0
        jne     @@draw
        ret
@@draw:
        dec     dx
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        add     dx, mouse_pointer_pics
        mov     pic_seg, dx
        mov     dx, cx
        shl     dx, 1
        shl     dx, 1
        add     dx, cx
        mov     ax, 1
        sub     ax, draw_page_index
        mov     ah, al
        xor     al, al
        shl     ax, 1
        shl     ax, 1
        add     ax, 0A000h
        add     ax, dx
        mov     es, ax
        mov     ax, 0C8h
        sub     ax, cx
        jbe     @@done
        cmp     ax, 11h
        js      @@rows_ok
        mov     ax, 10h
@@rows_ok:
        mov     rows, ax
        mov     ax, bx
        mov     ah, 1
        and     al, 3
        je      @@plane0
        mov     cl, al
        xor     ch, ch
@@shift:
        shl     ah, 1
        loop    @@shift
@@plane0:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     dx, bx
        shr     dx, 1
        shr     dx, 1
        mov     cx, 140h
        sub     cx, bx
        jbe     @@done
        cmp     cx, 11h
        js      @@cols_ok
        mov     cx, 10h
@@cols_ok:
        mov     bl, cl
        mov     bh, byte ptr rows
        mov     si, 0
        mov     cx, pic_seg
        mov     ds, cx
@@column:
        mov     di, dx
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
        mov     cl, bh
        xor     ch, ch
@@pixel:
        lodsb
        cmp     al, 0
        jne     @@opaque
        add     di, 50h
        loop    @@pixel
        jmp     @@next
@@opaque:
        stosb
        add     di, 4Fh
        loop    @@pixel
@@next:
        mov     cl, 10h
        sub     cl, bh
        xor     ch, ch
        add     si, cx
        dec     bl
        je      @@done
        shl     ah, 1
        cmp     ah, 9
        js      @@same
        inc     dx
        mov     ah, 1
@@same:
        jmp     @@column
@@done:
        ret
Mouse_Pointer_Blit_Back ENDP
; () fills the recorder buffer with FFFFh and resets both positions (DS = DGROUP assumed)
Input_Record_Clear PROC
        mov     ax, input_record_seg
        mov     es, ax
        mov     ax, 0FFFFh
        mov     di, 0
        mov     cx, input_record_base
        shl     cx, 1
@@fill:
        stosw
        loop    @@fill
        mov     ax, 0
        mov     input_record_ptr, ax
        mov     input_record_play, ax
        ret
Input_Record_Clear ENDP

Input_Record_Off PROC
        mov     input_record_play, 0
        ret
Input_Record_Off ENDP

Input_Record_Pause PROC
        mov     input_record_paused, 1
        ret
Input_Record_Pause ENDP

; () writes the FFFFh end marker - at word ptr / 2 of the byte position (slip) - and unpauses
Input_Record_Close PROC
        mov     ax, input_record_seg
        mov     es, ax
        mov     ax, 0FFFFh
        mov     di, input_record_ptr
        shr     di, 1
        mov     bx, input_record_base
        cmp     di, bx
        je      @@done
        stosw
@@done:
        mov     input_record_paused, 0
        ret
Input_Record_Close ENDP

; () input_record_ptr = the position of the end marker
Input_Record_Seek_End PROC
        push    es
        push    ds
        push    si
        push    di
        mov     input_record_ptr, 0
        mov     ax, input_record_seg
        mov     es, ax
        mov     ax, 0FFFFh
        xor     di, di
        mov     bx, input_record_base
        shl     bx, 1
        jmp     @@test
@@next:
        inc     di
        inc     di
@@test:
        cmp     es:[di], ax
        je      @@found
        cmp     di, bx
        jne     @@next
@@found:
        mov     input_record_ptr, di
        mov     ax, di
        pop     di
        pop     si
        pop     ds
        pop     es
        ret
Input_Record_Seek_End ENDP

; () named from its body - no caller (no Ghidra function, 2579:0e10)
Input_Record_Paused PROC
        mov     ax, input_record_paused
        ret
Input_Record_Paused ENDP

; (code, x, y) appends an event: the word code, then x and y bytes in two arrays after the codes
Input_Record_Event PROC USES ds es si di, ecode:WORD, ex:WORD, ey:WORD
        mov     ax, input_record_seg
        mov     es, ax
        mov     si, input_record_ptr
        mov     cx, input_record_base
        shl     cx, 1
        cmp     si, cx
        jne     @@room
        call    Input_Record_Close
        ret
@@room:
        mov     di, si
        shr     di, 1
        add     di, cx
        mov     ax, ecode
        mov     es:[si], ax
        mov     ax, ex
        xor     ah, ah
        mov     es:[di], al
        mov     ax, ey
        add     di, input_record_base
        mov     es:[di], al
        inc     si
        inc     si
        mov     input_record_ptr, si
        ret
Input_Record_Event ENDP

; () named from its body - no caller (no Ghidra function, 2579:0e63): replays the next recorded event as a click / pointer move
Input_Record_Play_Next PROC
        push    ds
        push    es
        push    si
        push    di
        mov     ax, input_record_seg
        mov     es, ax
        mov     si, input_record_play
        mov     ax, input_record_ptr
        cmp     ax, si
        jne     @@play
@@end:
        call    Input_Record_Close
        pop     di
        pop     si
        pop     es
        pop     ds
        ret
@@play:
        mov     cx, input_record_base
        shl     cx, 1
        mov     di, si
        shr     di, 1
        add     di, cx
        xor     bx, bx
        mov     ax, es:[si]
        cmp     ax, 0FFFFh
        je      @@end
        mov     bl, es:[di]
        xor     cx, cx
        add     di, input_record_base
        mov     cl, es:[di]
        push    cx
        push    bx
        push    ax
        call    Mouse_Post_Click
        cmp     cl, 0
        jne     @@moved
        call    Mouse_Take_Click
@@moved:
        call    Mouse_Set_Position
        mov     ax, mouse_region_count
        cmp     ax, 2
        jb      @@one_region
        call    Mouse_Find_Region
@@one_region:
        call    Mouse_Restore_Under
        call    Mouse_Save_Under
        call    Mouse_Draw_Pointer
        add     sp, 6
        inc     si
        inc     si
        mov     input_record_play, si
        pop     di
        pop     si
        pop     es
        pop     ds
        ret
Input_Record_Play_Next ENDP

        END
