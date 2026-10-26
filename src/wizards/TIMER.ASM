; TIMER.ASM - WIZARDS.EXE code segment 1484 (MAGIC.EXE 13f2, the same file): BIOS tick timing and the vertical
; retrace wait. Hand-written assembly (Borland TASM). The BIOS tick counter is the dword at 0000:046C (18.2 per second).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1484 src/wizards/TIMER.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   Key_Pressed:FAR, Mouse_Get_Buttons:FAR, Mouse_Take_Event:FAR
        .DATA                           ; this module's own data: DS:71e0 in both layouts' order (78c-Z)
timer_mark              dd      0       ; DS:71e0  BIOS tick count at the last Timer_Mark
timer_input_enabled     dw      1       ; DS:71e4  Wait_Ticks_Or_Input does nothing while 0 (starts 1;
                                        ;          no code writes it: always 1)
        .CODE
        PUBLIC  Timer_Mark, Timer_Elapsed, Wait_Ticks, Wait_Ticks_Or_Input, Wait_Vertical_Retrace

; C:  void Timer_Mark(void) { timer_mark = *(unsigned long far *)MK_FP(0, 0x46C); }
; Simtex name Mark_Time (MoO2 symbols, module timer); clarified using ReMoM repo
Timer_Mark PROC USES es
        mov     ax, 0
        mov     es, ax
        mov     ax, word ptr es:[046Ch]
        mov     word ptr timer_mark, ax
        mov     ax, word ptr es:[046Eh]
        mov     word ptr timer_mark + 2, ax
        ret
Timer_Mark ENDP

; () BIOS ticks since Timer_Mark (low word only). No callers.
; C:  int Timer_Elapsed(void) { return *(unsigned far *)MK_FP(0, 0x46C) - (unsigned)timer_mark; }
Timer_Elapsed PROC USES es
        mov     ax, 0
        mov     es, ax
        mov     ax, word ptr es:[046Ch]
        sub     ax, word ptr timer_mark
        ret
Timer_Elapsed ENDP

; (ticks) waits until ticks have passed since Timer_Mark - but returns as soon as 4 have passed, so any longer wait
; is cut to 4 ticks (BUG #333); returns at once if the count went backwards. The local word is never used.
; C:  void Wait_Ticks(int ticks) { int e;
; C:      for (;;) { e = *(unsigned far *)MK_FP(0, 0x46C) - (unsigned)timer_mark;
; C:                 if (e < 0 || e >= 4 || e >= ticks) return; } }
; Simtex name Release_Time (MoO2 symbols, module timer); clarified using ReMoM repo
Wait_Ticks PROC USES es ds si di, ticks:WORD
        LOCAL   unused:WORD
        jmp     @@check
@@done:
        ret
@@check:
        mov     ax, 0
        mov     es, ax
        mov     ax, word ptr es:[046Ch]
        sub     ax, word ptr timer_mark
        js      @@done
        cmp     ax, 4
        jns     @@done
        sub     ax, ticks
        js      @@check
        ret
Wait_Ticks ENDP

; (ticks) when timer_input_enabled: waits until ticks have passed since Timer_Mark, or a key, a mouse button or a
; mouse event; gives up after 400 ticks (about 22 s) or if the count went backwards. No callers.
; QUIRK #383: the timer segment is loaded from BX ('mov ax, 0 / mov es, bx'), not 0 - the tick count is read from wherever
; BX points after Mouse_Take_Event (dead code, no effect).
; Numbered: BUG #383 (BUGS_AND_QUIRKS.txt)
; C:  void Wait_Ticks_Or_Input(int ticks) { long e; if (!timer_input_enabled) return;
; C:      for (;;) { if (Key_Pressed() || Mouse_Get_Buttons() || Mouse_Take_Event()) return;
; C:                 e = *(long far *)MK_FP(0, 0x46C) - timer_mark;     (intended segment 0)
; C:                 if (e < 0 || e >= 400 || e >= ticks) return; } }
Wait_Ticks_Or_Input PROC USES es, ticks:WORD
        cmp     timer_input_enabled, 0
        jne     @@poll
@@done:
        ret
@@poll:
        call    Key_Pressed
        cmp     ax, 0
        je      @@no_key
        ret
@@no_key:
        call    Mouse_Get_Buttons
        cmp     ax, 0
        jne     @@input
        call    Mouse_Take_Event
        cmp     ax, 0
        je      @@time
@@input:
        ret
@@time:
        mov     ax, 0
        mov     es, bx
        mov     ax, word ptr es:[046Ch]
        mov     dx, word ptr es:[046Eh]
        sub     ax, word ptr timer_mark
        sbb     dx, word ptr timer_mark + 2
        js      @@done
        cmp     dx, 0
        jne     @@done
        cmp     ax, 0
        js      @@done
        cmp     ax, 400
        jns     @@done
        sub     ax, ticks
        js      @@poll
        ret
Wait_Ticks_Or_Input ENDP

; () waits for the end of the current vertical retrace, then for the start of the next (VGA status port 3DAh bit 3)
; C:  void Wait_Vertical_Retrace(void) { while (inportb(0x3DA) & 8) ; while (!(inportb(0x3DA) & 8)) ; }
Wait_Vertical_Retrace PROC USES es
        mov     dx, 03DAh
@@in_retrace:
        in      al, dx
        and     al, 8
        jne     @@in_retrace
@@wait_retrace:
        in      al, dx
        and     al, 8
        je      @@wait_retrace
        ret
Wait_Vertical_Retrace ENDP

        END
