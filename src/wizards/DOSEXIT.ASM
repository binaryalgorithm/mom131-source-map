; DOSEXIT.ASM - WIZARDS.EXE code segment 14f0 (MAGIC.EXE 145d, the same file): back to text mode, the final DOS
; message + exit, free DOS memory, and two empty hooks. Hand-written assembly (Borland TASM). Starts at 14f0:0002
; (Ghidra files Video_Text_Mode under the segment before, as 14b5:03b2).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 14f0 src/wizards/DOSEXIT.ASM
        .MODEL  MEDIUM, C
        LOCALS
        .CODE
        PUBLIC  Video_Text_Mode, Text_Add_CRLF_Print, Text_Add_CRLF_Print2, DOS_Free_KB
        PUBLIC  Error_Hook_Init, Error_Hook_Exit

; () BIOS video mode 3 (80 x 25 color text)
; C:  void Video_Text_Mode(void) { _AX = 3; geninterrupt(0x10); }
Video_Text_Mode PROC
        mov     ax, 3
        int     10h
        ret
Video_Text_Mode ENDP

; (text) appends CR LF CR LF "$" IN PLACE (past the string's end), prints it (INT 21h AH=09h) and ends the program
; (AH=4Ch; the exit code is AL as AH=09h left it, '$' = 36). SI is used without being saved.
; C:  void Text_Add_CRLF_Print(char *text) { strcat(text, "\r\n\r\n$"); _DX = (unsigned)text; _AH = 9;
; C:      geninterrupt(0x21); _AH = 0x4C; geninterrupt(0x21); }       (does not return)
Text_Add_CRLF_Print PROC, text:WORD
        mov     si, text
@@find_end:
        lodsb
        cmp     al, 0
        jne     @@find_end
        dec     si
        mov     byte ptr [si], 0Dh
        inc     si
        mov     byte ptr [si], 0Ah
        inc     si
        mov     byte ptr [si], 0Dh
        inc     si
        mov     byte ptr [si], 0Ah
        inc     si
        mov     byte ptr [si], 24h
        inc     si
        mov     dx, text
        mov     ah, 9
        int     21h
        mov     ah, 4Ch
        int     21h
        ret
Text_Add_CRLF_Print ENDP

; (text) the same without the exit. No callers.
; C:  void Text_Add_CRLF_Print2(char *text) { strcat(text, "\r\n\r\n$"); _DX = (unsigned)text; _AH = 9; geninterrupt(0x21); }
Text_Add_CRLF_Print2 PROC, text:WORD
        mov     si, text
@@find_end:
        lodsb
        cmp     al, 0
        jne     @@find_end
        dec     si
        mov     byte ptr [si], 0Dh
        inc     si
        mov     byte ptr [si], 0Ah
        inc     si
        mov     byte ptr [si], 0Dh
        inc     si
        mov     byte ptr [si], 0Ah
        inc     si
        mov     byte ptr [si], 24h
        inc     si
        mov     dx, text
        mov     ah, 9
        int     21h
        ret
Text_Add_CRLF_Print2 ENDP

; () the largest free DOS block in KB (INT 21h AH=48h with BX=FFFFh fails and returns the largest size in paragraphs)
; C:  unsigned DOS_Free_KB(void) { unsigned seg; return allocmem(0xFFFF, &seg) / 64; }
DOS_Free_KB PROC USES es ds si di
        mov     ah, 48h
        mov     bx, 0FFFFh
        int     21h
        mov     ax, bx
        shr     ax, 1
        shr     ax, 1
        shr     ax, 1
        shr     ax, 1
        shr     ax, 1
        shr     ax, 1
        ret
DOS_Free_KB ENDP

; () empty hook (Engine_Init)
; C:  void Error_Hook_Init(void) { }
Error_Hook_Init PROC USES es ds si di
        ret
Error_Hook_Init ENDP

; () empty hook (every exit path)
; C:  void Error_Hook_Exit(void) { }
Error_Hook_Exit PROC USES es ds si di
        ret
Error_Hook_Exit ENDP

        END
