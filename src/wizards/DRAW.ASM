; DRAW.ASM - WIZARDS.EXE code segment 18fc (MAGIC.EXE 186a, the same file): the planar (Mode X) drawing primitives -
; filled rectangles, pixels, lines (solid and patterned, 8.8 fixed-point DDA), the rectangle remaps (shadow / color
; tables through the VGA read map), the shade, gradient and noise fills. Hand-written assembly (Borland TASM).
; Screen layout: draw_page_seg + y * 5 paragraphs = row y (80 bytes per plane-row), x / 4 = byte, x & 3 = plane.
; Remap_Rect and Gradient_Fill_Shade keep their per-plane spans in WORDS IN THE CODE SEGMENT (18fc:000c..001f).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled; a port
; draws into a plain 320 x 200 byte buffer (pixel = buf[y * 320 + x]).
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 18fc src/wizards/DRAW.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        EXTRN   p_palette:WORD          ; DS:e7e8
        EXTRN   shade_table_block:WORD  ; DS:e7fa
        .DATA                           ; this module's own data, DS:76ec
        PUBLIC  draw_page_seg, draw_page_index, gradient_remap
draw_page_seg   dw      0A000h          ; DS:76ec  segment drawing goes to (Set_Draw_Page)
draw_page_index dw      0               ; DS:76ee  the displayed page (0 / 1)
left_masks      db      0Fh, 0Eh, 0Ch, 08h      ; DS:76f0  planes from x & 3 to the right end of a byte
right_masks     db      01h, 03h, 07h, 0Fh      ; DS:76f4  planes from the left end of a byte to x & 3
pixel_masks     db      01h, 02h, 04h, 08h      ; DS:76f8  the plane of x & 3
gradient_remap  db      64 dup (0)      ; DS:76fc  color run of the gradient fills (Gradient_Pingpong_Run)
noise_table     db      29h, 24h, 3Fh, 05h, 62h, 6Dh, 57h, 2Fh, 53h, 11h, 4Ah, 72h, 72h, 3Ch, 6Ah, 6Ch  ; DS:773c
                db      33h, 27h, 5Ch, 3Dh, 08h, 0Dh, 3Fh, 1Ah, 25h, 5Fh, 0Eh, 1Dh, 07h, 38h, 48h, 5Fh
                db      33h, 13h, 4Eh, 49h, 44h, 3Ch, 0Ch, 27h, 20h, 04h, 5Bh, 7Eh, 0Ah, 39h, 26h, 20h
                db      5Dh, 55h, 4Ch, 7Dh, 17h, 76h, 46h, 3Ch, 14h, 0Eh, 0Ah, 0Bh, 1Dh, 5Ch, 2Fh, 33h
                db      20h, 1Bh, 51h, 6Fh, 41h, 79h, 37h, 7Eh, 13h, 4Ah, 33h, 77h, 1Fh, 7Eh, 4Ah, 5Dh
                db      2Dh, 50h, 15h, 73h, 45h, 41h, 67h, 51h, 6Ch, 45h, 31h, 38h, 33h, 3Ch, 22h, 23h
                db      76h, 23h, 12h, 1Eh, 62h, 0Ch, 20h, 5Bh, 31h, 4Bh, 1Ah, 03h, 3Ah, 73h, 1Eh, 4Ah
                db      2Ch, 01h, 7Fh, 46h, 1Ah, 56h, 6Ah, 00h, 33h, 6Bh, 4Ah, 4Dh, 54h, 40h, 68h, 57h
                db      3Fh, 15h, 57h, 7Fh, 2Eh, 5Dh, 0Fh, 67h, 04h, 70h, 58h, 4Ah, 62h, 7Fh, 6Ah, 10h
                db      61h, 4Eh, 52h, 1Fh, 1Eh, 1Dh, 17h, 73h, 73h, 67h, 1Eh, 71h, 05h, 50h, 4Bh, 78h
                db      02h, 58h, 69h, 3Ah, 2Dh, 54h, 4Ch, 4Ah, 13h, 1Fh, 34h, 75h, 1Fh, 0Dh, 75h, 56h
                db      54h, 20h, 55h, 25h, 5Ah, 7Fh, 36h, 50h, 33h, 23h, 75h, 4Dh, 50h, 54h, 11h, 2Eh
                db      48h, 54h, 10h, 76h, 67h, 5Ah, 1Eh, 2Bh, 66h, 41h, 78h, 2Ch, 79h, 02h, 08h, 45h
                db      0Eh, 60h, 51h, 01h, 55h, 62h, 0Eh, 3Fh, 7Ch, 06h, 16h, 08h, 3Ch, 34h, 03h, 20h
                db      18h, 71h, 13h, 5Bh, 65h, 55h, 4Fh, 32h, 06h, 3Fh, 6Ah, 16h, 79h, 47h, 6Bh, 05h
                db      16h, 74h, 0Fh, 5Ah, 17h, 30h, 68h, 69h, 55h, 78h, 4Bh, 4Bh, 51h, 58h, 69h, 77h
        .CODE
        PUBLIC  Fill_Rect, Put_Pixel, Draw_Line, Draw_Line_Pattern, Gradient_Pingpong_Run, Gradient_Fill_Remap
        PUBLIC  Random_LFSR8, Remap_Rect, Gradient_Fill_Shade, Gradient_Fill_Remap2, Gradient_Fill_Noise
rows            dw      0               ; 18fc:000c  rows of the rectangle (Remap_Rect / Gradient_Fill_Shade)
plane0_x        dw      0               ; 18fc:000e  first byte column of plane 0
plane0_n        dw      0               ; 18fc:0010  byte columns of plane 0
plane1_x        dw      0               ; 18fc:0012
plane1_n        dw      0               ; 18fc:0014
plane2_x        dw      0               ; 18fc:0016
plane2_n        dw      0               ; 18fc:0018
plane3_x        dw      0               ; 18fc:001a
plane3_n        dw      0               ; 18fc:001c
row_count       dw      0               ; 18fc:001e  rows left in the current plane

; (x1, y1, x2, y2, color) filled rectangle, inclusive, unclipped (x1 <= x2, y1 <= y2): masked left and right byte
; columns, then all 4 planes for the bytes between
; C:  void Fill_Rect(int x1, int y1, int x2, int y2, int color) { int x, y;
; C:      for (y = y1; y <= y2; y++) for (x = x1; x <= x2; x++) screen[y * 320 + x] = color; }
; Simtex name Fill (MoO2 symbols, module graphics); clarified using ReMoM repo
Fill_Rect PROC USES ds di si es, x1:WORD, y1:WORD, x2:WORD, y2:WORD, color:WORD
        LOCAL   unused:WORD, height:WORD
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, x2
        mov     bx, x1
        shr     ax, 1
        shr     ax, 1
        shr     bx, 1
        shr     bx, 1
        cmp     ax, bx
        jne     @@wide
        mov     di, ax
        mov     si, offset left_masks
        mov     ax, x1
        and     ax, 3
        add     si, ax
        lodsb
        mov     bl, al
        mov     si, offset right_masks
        mov     ax, x2
        and     ax, 3
        add     si, ax
        lodsb
        and     bl, al
        mov     dx, 03C5h
        mov     al, bl
        out     dx, al
        mov     ax, y1
        mov     dx, ax
        mov     cx, y2
        inc     cx
        sub     cx, ax
        shl     dx, 1
        shl     dx, 1
        add     ax, dx
        add     ax, draw_page_seg
        mov     es, ax
        mov     al, byte ptr color
@@column:
        stosb
        add     di, 79
        loop    @@column
        ret
@@wide:
        mov     dx, y2
        mov     bx, y1
        sub     dx, bx
        inc     dx
        mov     height, dx
        mov     ax, bx
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     di, x1
        mov     bx, di
        shr     di, 1
        shr     di, 1
        and     bx, 3
        add     bx, offset left_masks
        mov     al, [bx]
        mov     dx, 03C5h
        out     dx, al
        mov     cx, height
        mov     al, byte ptr color
@@left:
        stosb
        add     di, 79
        loop    @@left
        mov     di, x2
        mov     bx, di
        shr     di, 1
        shr     di, 1
        and     bx, 3
        add     bx, offset right_masks
        mov     al, [bx]
        mov     dx, 03C5h
        out     dx, al
        mov     cx, height
        mov     al, byte ptr color
@@right:
        stosb
        add     di, 79
        loop    @@right
        mov     al, 0Fh
        mov     dx, 03C5h
        out     dx, al
        mov     bx, x2
        shr     bx, 1
        shr     bx, 1
        mov     ax, x1
        shr     ax, 1
        shr     ax, 1
        sub     bx, ax
        dec     bx
        je      @@done
        mov     di, ax
        inc     di
        mov     dx, 80
        sub     dx, bx
        mov     si, height
        mov     al, byte ptr color
@@row:
        mov     cx, bx
        rep     stosb
        add     di, dx
        dec     si
        jne     @@row
@@done:
        ret
Fill_Rect ENDP

; (x, y, color) one pixel, unclipped
; C:  void Put_Pixel(int x, int y, int color) { screen[y * 320 + x] = color; }
; Simtex name Dot (MoO2 symbols, module graphics); clarified using ReMoM repo
Put_Pixel PROC USES di es, x:WORD, y:WORD, color:WORD
        mov     bx, x
        mov     di, bx
        shr     di, 1
        shr     di, 1
        and     bx, 3
        add     bx, offset pixel_masks
        mov     ah, [bx]
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, ah
        out     dx, al
        mov     ax, y
        mov     dx, ax
        shl     dx, 1
        shl     dx, 1
        add     ax, dx
        add     ax, draw_page_seg
        mov     es, ax
        mov     al, byte ptr color
        stosb
        ret
Put_Pixel ENDP

; (x1, y1, x2, y2, color) unclipped line, 8.8 fixed-point DDA: drawn left to right; the longer axis steps by 1.00,
; the other by d_short * 256 / d_long (the 16-bit dividend is d_short * 256 - so d_short must stay below 256)
; C:  void Draw_Line(int x1, int y1, int x2, int y2, int color) { int n, i; long fx, fy; (swap so x1 <= x2)
; C:      n = max(x2 - x1, abs(y2 - y1)) + 1; for (i = 0; i < n; i++) plot(x1 + i * dx / (n - 1), y1 + i * dy / (n - 1)); }
; Simtex name Line (MoO2 symbols, module line); clarified using ReMoM repo
Draw_Line PROC USES es ds si di, x1:WORD, y1:WORD, x2:WORD, y2:WORD, color:WORD
        LOCAL   y_step:WORD, count:WORD, x_step:WORD, row_step:WORD, unused:WORD
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, x2
        cmp     ax, x1
        jns     @@ordered
        mov     si, ax
        mov     ax, x1
        mov     x2, ax
        mov     x1, si
        mov     si, y2
        mov     ax, y1
        mov     y2, ax
        mov     y1, si
@@ordered:
        mov     ax, x1
        mov     bx, y1
        mov     cx, x2
        mov     dx, y2
        sub     cx, ax
        sub     dx, bx
        mov     row_step, 5
        jns     @@down
        neg     dx
        mov     row_step, 0FFFBh
@@down:
        cmp     cx, dx
        jns     @@x_major
        inc     dx
        mov     count, dx
        mov     y_step, 100h
        dec     dx
        je      @@start
        mov     bx, dx
        mov     ah, cl
        xor     al, al
        mov     dl, ch
        xor     dh, dh
        mov     cx, bx
        div     cx
        mov     x_step, ax
        jmp     @@start
@@x_major:
        inc     cx
        mov     count, cx
        dec     cx
        je      @@start
        mov     x_step, 100h
        mov     ah, dl
        xor     al, al
        mov     dl, dh
        xor     dh, dh
        div     cx
        mov     y_step, ax
@@start:
        mov     bx, y1
        mov     ax, bx
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     cx, x1
        mov     ax, cx
        shr     ax, 1
        shr     ax, 1
        mov     di, ax
        and     cx, 3
        mov     ax, 1
        shl     ax, cl
        mov     ah, al
        mov     al, byte ptr color
        mov     bx, 80h
        mov     cx, 80h
        mov     si, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, ah
        out     dx, al
        mov     ax, si
        jmp     @@plot
@@next:
        add     bx, x_step
        cmp     bh, 0
        je      @@same_x
        xor     bh, bh
        shl     ah, 1
        cmp     ah, 9
        js      @@plane_ok
        mov     ah, 1
        inc     di
@@plane_ok:
        mov     si, ax
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     ax, si
@@same_x:
        add     cx, y_step
        cmp     ch, 0
        je      @@plot
        xor     ch, ch
        mov     dx, es
        add     dx, row_step
        mov     es, dx
@@plot:
        mov     es:[di], al
        dec     count
        jne     @@next
        ret
Draw_Line ENDP

; (x1, y1, x2, y2, pattern, length, phase) the same line drawn with a repeating pattern of colors (0 = leave the pixel),
; starting at pattern[phase] (Draw_Line_Clip_Pattern)
; C:  as Draw_Line, pixel i gets pattern[(phase + i) % length] unless that is 0
; Simtex name Multi_Colored_Line (MoO2 symbols, module line); clarified using ReMoM repo
Draw_Line_Pattern PROC USES es ds si di, x1:WORD, y1:WORD, x2:WORD, y2:WORD, pattern:WORD, len:WORD, phase:WORD
        LOCAL   y_step:WORD, count:WORD, x_step:WORD, row_step:WORD, unused:WORD, pat_end:WORD
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, x2
        cmp     ax, x1
        jns     @@ordered
        mov     si, ax
        mov     ax, x1
        mov     x2, ax
        mov     x1, si
        mov     si, y2
        mov     ax, y1
        mov     y2, ax
        mov     y1, si
@@ordered:
        mov     ax, x1
        mov     bx, y1
        mov     cx, x2
        mov     dx, y2
        sub     cx, ax
        sub     dx, bx
        mov     row_step, 5
        jns     @@down
        neg     dx
        mov     row_step, 0FFFBh
@@down:
        cmp     cx, dx
        jns     @@x_major
        inc     dx
        mov     count, dx
        mov     y_step, 100h
        dec     dx
        je      @@start
        mov     bx, dx
        mov     ah, cl
        xor     al, al
        mov     dl, ch
        xor     dh, dh
        mov     cx, bx
        div     cx
        mov     x_step, ax
        jmp     @@start
@@x_major:
        inc     cx
        mov     count, cx
        dec     cx
        je      @@start
        mov     x_step, 100h
        mov     ah, dl
        xor     al, al
        mov     dl, dh
        xor     dh, dh
        div     cx
        mov     y_step, ax
@@start:
        mov     ax, pattern
        mov     si, ax
        add     si, phase
        add     ax, len
        mov     pat_end, ax
        mov     bx, y1
        mov     ax, bx
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     cx, x1
        mov     ax, cx
        shr     ax, 1
        shr     ax, 1
        mov     di, ax
        and     cx, 3
        mov     ax, 1
        shl     ax, cl
        mov     ah, al
        mov     bx, 80h
        mov     cx, 80h
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        jmp     @@plot
@@next:
        add     bx, x_step
        cmp     bh, 0
        je      @@same_x
        xor     bh, bh
        shl     ah, 1
        cmp     ah, 9
        js      @@plane_ok
        mov     ah, 1
        inc     di
@@plane_ok:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
@@same_x:
        add     cx, y_step
        cmp     ch, 0
        je      @@plot
        xor     ch, ch
        mov     dx, es
        add     dx, row_step
        mov     es, dx
@@plot:
        lodsb
        cmp     al, 0
        je      @@skip
        mov     es:[di], al
@@skip:
        cmp     si, pat_end
        je      @@wrap
        dec     count
        jne     @@next
        ret
@@wrap:
        mov     si, pattern
        dec     count
        jne     @@next
        ret
Draw_Line_Pattern ENDP

; register call (DX = first color, or a DS offset of a color list if DX >= 256; BX = n): fills gradient_remap with
; n colors going up and n coming back (a ping-pong run). In the list case it copies the list and then walks it
; backwards from its last entry, two bytes at a time (SI -= 2 after each movsb = one entry back).
; In the color case the 'dec si / sub si, 2' lines are leftovers of the list case (SI is not used).
; C:  void Gradient_Pingpong_Run(int first /*DX*/, int n /*BX*/) { int i;
; C:      for (i = 0; i < n; i++) gradient_remap[i] = first + i; for (i = 0; i < n; i++) gradient_remap[n + i] = first + n - 1 - i; }
Gradient_Pingpong_Run PROC USES es
        mov     ax, ds
        mov     es, ax
        mov     di, offset gradient_remap
        cmp     dx, 100h
        jb      @@colors
        mov     si, dx
        mov     cx, bx
        rep     movsb
        dec     si
        mov     cx, bx
@@back:
        movsb
        sub     si, 2
        loop    @@back
        ret
@@colors:
        mov     al, dl
        mov     cx, bx
@@up:
        stosb
        inc     al
        loop    @@up
        dec     al
        dec     si
        mov     cx, bx
@@down:
        stosb
        sub     si, 2
        dec     al
        loop    @@down
        ret
Gradient_Pingpong_Run ENDP

; (x1, y1, x2, y2, first, n, slope, seed) diagonal gradient with the ping-pong color run (period 2n), per pixel
; offset by a noise byte; drawn column by column
; C:  color(x, y) = gradient_remap[((acc_x + acc_y + 2 * noise[k++]) >> 8 & 63) % (2 * n)]
Gradient_Fill_Remap PROC USES es ds si di, x1:WORD, y1:WORD, x2:WORD, y2:WORD, first:WORD, n:WORD, slope:WORD, seed:WORD
        LOCAL   x_step:WORD, y_step:WORD, acc:WORD, acc2:WORD, height:WORD, width:WORD, mask:WORD, col:WORD, noise_i:WORD, period:WORD
        mov     ax, n
        shl     ax, 1
        mov     period, ax
        mov     ax, seed
        xor     ah, ah
        mov     noise_i, ax
        mov     bx, n
        mov     dx, first
        call    Gradient_Pingpong_Run
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     bx, x2
        sub     bx, x1
        inc     bx
        mov     cx, 80h
        mov     ax, n
        mul     cx
        mov     cx, slope
        mul     cx
        div     bx
        mov     x_step, ax
        mov     bx, y2
        sub     bx, y1
        inc     bx
        mov     cx, 80h
        mov     ax, n
        mul     cx
        mov     cx, slope
        mul     cx
        div     bx
        mov     y_step, ax
        mov     acc, 0
        mov     ax, x2
        sub     ax, x1
        inc     ax
        mov     width, ax
        mov     ax, y2
        sub     ax, y1
        inc     ax
        mov     height, ax
        mov     ax, y1
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     cx, x1
        mov     bx, cx
        shr     bx, 1
        shr     bx, 1
        mov     col, bx
        mov     al, 1
        and     cx, 3
        je      @@plane0
        shl     al, cl
@@plane0:
        xor     ah, ah
        mov     mask, ax
        mov     bx, offset gradient_remap
        jmp     @@column
@@next_column:
        mov     al, byte ptr mask
        shl     al, 1
        cmp     al, 9
        js      @@column
        mov     al, 1
        inc     col
@@column:
        mov     byte ptr mask, al
        mov     di, col
        mov     dx, 03C5h
        out     dx, al
        mov     ax, acc
        add     ax, x_step
        mov     acc, ax
        mov     acc2, ax
        mov     cx, height
@@pixel:
        mov     dx, acc2
        add     dx, y_step
        mov     acc2, dx
        mov     si, noise_i
        inc     byte ptr noise_i
        add     si, offset noise_table
        lodsb
        xor     ah, ah
        shl     ax, 1
        add     dx, ax
        mov     al, dh
        and     ax, 3Fh
@@mod:
        cmp     ax, period
        js      @@in_range
        sub     ax, period
        jmp     @@mod
@@in_range:
        add     ax, bx
        mov     si, ax
        movsb
        add     di, 79
        loop    @@pixel
        dec     width
        jne     @@next_column
        ret
Gradient_Fill_Remap ENDP

; register call (AX = state, 0 = seed 35C8h) -> AX after 8 steps of a 16-bit Fibonacci LFSR; keeps BX CX DX
; C:  unsigned Random_LFSR8(unsigned s) { int i; if (!s) s = 0x35C8; for (i = 0; i < 8; i++)
; C:      s = (s >> 1) | ((s ^ s >> 1 ^ s >> 2 ^ s >> 4 ^ s >> 6 ^ s >> 7 ... feedback bit) << 15); return s; }  (see asm)
Random_LFSR8 PROC
        push    bx
        push    cx
        push    dx
        cmp     ax, 0
        je      @@seed
@@go:
        mov     cx, 8
@@step:
        mov     dx, ax
        mov     bx, ax
        shr     bx, 1
        xor     ax, bx
        shr     bx, 1
        xor     ax, bx
        shr     bx, 1
        shr     bx, 1
        xor     ax, bx
        shr     bx, 1
        shr     bx, 1
        xor     ax, bx
        shr     bx, 1
        xor     al, bh
        shr     ax, 1
        rcr     dx, 1
        mov     ax, dx
        loop    @@step
        pop     dx
        pop     cx
        pop     bx
        ret
@@seed:
        mov     ax, 35C8h
        jmp     @@go
        ret
Random_LFSR8 ENDP

; (x1, y1, x2, y2, table) every pixel of the rectangle through remap table 'table' (256 bytes at remap_tables_seg +
; table * 16 paragraphs) - plane by plane, reading through the VGA read map (3CEh index 4)
; C:  void Remap_Rect(int x1, int y1, int x2, int y2, int table) { int x, y; unsigned char far *t = remap table;
; C:      for (y = y1; y <= y2; y++) for (x = x1; x <= x2; x++) screen[y * 320 + x] = t[screen[y * 320 + x]]; }
Remap_Rect PROC USES ds di si es, x1:WORD, y1:WORD, x2:WORD, y2:WORD, table:WORD
        mov     ax, x2
        mov     bx, x1
        shr     ax, 1
        shr     ax, 1
        shr     bx, 1
        shr     bx, 1
        sub     ax, bx
        inc     ax
        mov     cs:plane0_n, ax
        mov     cs:plane1_n, ax
        mov     cs:plane2_n, ax
        mov     cs:plane3_n, ax
        mov     bx, x1
        mov     ax, bx
        shr     ax, 1
        shr     ax, 1
        mov     cs:plane0_x, ax
        mov     cs:plane1_x, ax
        mov     cs:plane2_x, ax
        mov     cs:plane3_x, ax
        and     bx, 3
        cmp     bx, 0
        je      @@left_done
        cmp     bx, 1
        je      @@left_1
        cmp     bx, 2
        je      @@left_2
        inc     cs:plane2_x
        dec     cs:plane2_n
@@left_2:
        inc     cs:plane1_x
        dec     cs:plane1_n
@@left_1:
        inc     cs:plane0_x
        dec     cs:plane0_n
@@left_done:
        mov     bx, x2
        and     bx, 3
        cmp     bx, 3
        je      @@right_done
        cmp     bx, 2
        je      @@right_2
        cmp     bx, 1
        je      @@right_1
        dec     cs:plane1_n
@@right_1:
        dec     cs:plane2_n
@@right_2:
        dec     cs:plane3_n
@@right_done:
        mov     ax, y1
        mov     dx, ax
        mov     cx, y2
        inc     cx
        sub     cx, ax
        shl     dx, 1
        shl     dx, 1
        add     ax, dx
        add     ax, draw_page_seg
        mov     es, ax
        mov     dx, y2
        mov     bx, y1
        sub     dx, bx
        inc     dx
        mov     cs:rows, dx
        mov     ax, table
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        add     ax, remap_tables_seg
        mov     ds, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 1
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 0
        out     dx, al
        mov     di, cs:plane0_x
        mov     si, di
        mov     dx, cs:plane0_n
        cmp     dx, 0
        jbe     @@plane1
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row0:
        mov     cx, dx
@@byte0:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte0
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row0
@@plane1:
        mov     ax, cs:plane1_n
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 2
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 1
        out     dx, al
        mov     di, cs:plane1_x
        mov     si, di
        mov     dx, cs:plane1_n
        cmp     dx, 0
        jbe     @@plane2
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row1:
        mov     cx, dx
@@byte1:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte1
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row1
@@plane2:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 4
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 2
        out     dx, al
        mov     di, cs:plane2_x
        mov     si, di
        mov     dx, cs:plane2_n
        cmp     dx, 0
        jbe     @@plane3
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row2:
        mov     cx, dx
@@byte2:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte2
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row2
@@plane3:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 8
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 3
        out     dx, al
        mov     di, cs:plane3_x
        mov     si, di
        mov     dx, cs:plane3_n
        cmp     dx, 0
        jbe     @@done
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row3:
        mov     cx, dx
@@byte3:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte3
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row3
@@done:
        ret
Remap_Rect ENDP

; (x1, y1, x2, y2, base) builds a 256-entry shade table (brightness of each palette color: (r + r/2 + g + g/2 + b) / 16
; + base, in shade_table_block) and remaps the rectangle through it, as Remap_Rect
; C:  for (c = 0; c < 256; c++) shade[c] = (pal[c].r * 3 / 2 + pal[c].g * 3 / 2 + pal[c].b) / 16 + base; then remap
; Simtex name Tint_Fill (MoO2 symbols, module graphics); clarified using ReMoM repo
Gradient_Fill_Shade PROC USES ds di si es, x1:WORD, y1:WORD, x2:WORD, y2:WORD, base:WORD
        push    ds
        mov     dx, base
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
        mov     ax, x2
        mov     bx, x1
        shr     ax, 1
        shr     ax, 1
        shr     bx, 1
        shr     bx, 1
        sub     ax, bx
        inc     ax
        mov     cs:plane0_n, ax
        mov     cs:plane1_n, ax
        mov     cs:plane2_n, ax
        mov     cs:plane3_n, ax
        mov     bx, x1
        mov     ax, bx
        shr     ax, 1
        shr     ax, 1
        mov     cs:plane0_x, ax
        mov     cs:plane1_x, ax
        mov     cs:plane2_x, ax
        mov     cs:plane3_x, ax
        and     bx, 3
        cmp     bx, 0
        je      @@left_done
        cmp     bx, 1
        je      @@left_1
        cmp     bx, 2
        je      @@left_2
        inc     cs:plane2_x
        dec     cs:plane2_n
@@left_2:
        inc     cs:plane1_x
        dec     cs:plane1_n
@@left_1:
        inc     cs:plane0_x
        dec     cs:plane0_n
@@left_done:
        mov     bx, x2
        and     bx, 3
        cmp     bx, 3
        je      @@right_done
        cmp     bx, 2
        je      @@right_2
        cmp     bx, 1
        je      @@right_1
        dec     cs:plane1_n
@@right_1:
        dec     cs:plane2_n
@@right_2:
        dec     cs:plane3_n
@@right_done:
        mov     ax, y1
        mov     dx, ax
        mov     cx, y2
        inc     cx
        sub     cx, ax
        shl     dx, 1
        shl     dx, 1
        add     ax, dx
        add     ax, draw_page_seg
        mov     es, ax
        mov     dx, y2
        mov     bx, y1
        sub     dx, bx
        inc     dx
        mov     cs:rows, dx
        mov     ax, shade_table_block
        mov     ds, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 1
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 0
        out     dx, al
        mov     di, cs:plane0_x
        mov     si, di
        mov     dx, cs:plane0_n
        cmp     dx, 0
        jbe     @@plane1
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row0:
        mov     cx, dx
@@byte0:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte0
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row0
@@plane1:
        mov     ax, cs:plane1_n
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 2
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 1
        out     dx, al
        mov     di, cs:plane1_x
        mov     si, di
        mov     dx, cs:plane1_n
        cmp     dx, 0
        jbe     @@plane2
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row1:
        mov     cx, dx
@@byte1:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte1
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row1
@@plane2:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 4
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 2
        out     dx, al
        mov     di, cs:plane2_x
        mov     si, di
        mov     dx, cs:plane2_n
        cmp     dx, 0
        jbe     @@plane3
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row2:
        mov     cx, dx
@@byte2:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte2
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row2
@@plane3:
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        inc     dx
        mov     al, 8
        out     dx, al
        mov     dx, 03CEh
        mov     al, 4
        out     dx, al
        inc     dx
        mov     al, 3
        out     dx, al
        mov     di, cs:plane3_x
        mov     si, di
        mov     dx, cs:plane3_n
        cmp     dx, 0
        jbe     @@done
        xor     bh, bh
        mov     ax, cs:rows
        mov     cs:row_count, ax
@@row3:
        mov     cx, dx
@@byte3:
        mov     bl, es:[si]
        mov     al, [bx]
        stosb
        inc     si
        loop    @@byte3
        mov     ax, 80
        sub     ax, dx
        add     si, ax
        add     di, ax
        dec     cs:row_count
        jne     @@row3
@@done:
        ret
Gradient_Fill_Shade ENDP

; (x1, y1, x2, y2, first, n, slope, seed) horizontal gradient with the ping-pong color run, noise per pixel
; C:  color(x, y) = gradient_remap[((acc_x + 2 * noise[k++]) >> 8 & 63) % (2 * n)]
Gradient_Fill_Remap2 PROC USES es ds si di, x1:WORD, y1:WORD, x2:WORD, y2:WORD, first:WORD, n:WORD, slope:WORD, seed:WORD
        LOCAL   x_step:WORD, unused4:WORD, acc:WORD, unused8:WORD, height:WORD, width:WORD, mask:WORD, col:WORD, noise_i:WORD, unused14:WORD, period:WORD
        mov     ax, n
        shl     ax, 1
        mov     period, ax
        mov     ax, seed
        xor     ah, ah
        mov     noise_i, ax
        mov     bx, n
        mov     dx, first
        call    Gradient_Pingpong_Run
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     bx, x2
        sub     bx, x1
        inc     bx
        mov     cx, 100h
        mov     ax, n
        mul     cx
        mov     cx, slope
        mul     cx
        div     bx
        mov     x_step, ax
        mov     acc, 0
        mov     ax, x2
        sub     ax, x1
        inc     ax
        mov     width, ax
        mov     ax, y2
        sub     ax, y1
        inc     ax
        mov     height, ax
        mov     ax, y1
        mov     bx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     cx, x1
        mov     bx, cx
        shr     bx, 1
        shr     bx, 1
        mov     col, bx
        mov     al, 1
        and     cx, 3
        je      @@plane0
        shl     al, cl
@@plane0:
        xor     ah, ah
        mov     mask, ax
        mov     bx, offset gradient_remap
        jmp     @@column
@@next_column:
        mov     al, byte ptr mask
        shl     al, 1
        cmp     al, 9
        js      @@column
        mov     al, 1
        inc     col
@@column:
        mov     byte ptr mask, al
        mov     di, col
        mov     dx, 03C5h
        out     dx, al
        mov     ax, acc
        add     ax, x_step
        mov     acc, ax
        mov     dx, ax
        mov     cx, height
@@pixel:
        mov     si, noise_i
        inc     byte ptr noise_i
        add     si, offset noise_table
        lodsb
        xor     ah, ah
        shl     ax, 1
        add     ax, dx
        mov     al, ah
        and     ax, 3Fh
@@mod:
        cmp     ax, period
        js      @@in_range
        sub     ax, period
        jmp     @@mod
@@in_range:
        add     ax, bx
        mov     si, ax
        movsb
        add     di, 79
        loop    @@pixel
        dec     width
        jne     @@next_column
        ret
Gradient_Fill_Remap2 ENDP

; (x1, y1, x2, y2, color, speck, unused, seed) the rectangle in color with specks of 'speck' where the noise table
; (walked from an LFSR state) has a bit set for the pixel's plane
; C:  for each pixel: screen = (noise[k++] & plane_bit(x)) ? speck : color
Gradient_Fill_Noise PROC USES ds di si es, x1:WORD, y1:WORD, x2:WORD, y2:WORD, color:WORD, speck:WORD, unused:WORD, seed:WORD
        LOCAL   unused2:WORD, height:WORD, rnd:WORD, unused8:WORD, row_skip:WORD, mid:WORD
        mov     ax, seed
        call    Random_LFSR8
        mov     rnd, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, x2
        mov     bx, x1
        shr     ax, 1
        shr     ax, 1
        shr     bx, 1
        shr     bx, 1
        cmp     ax, bx
        jne     @@wide
        mov     di, ax
        mov     si, offset left_masks
        mov     ax, x1
        and     ax, 3
        add     si, ax
        lodsb
        mov     bl, al
        mov     si, offset right_masks
        mov     ax, x2
        and     ax, 3
        add     si, ax
        lodsb
        and     bl, al
        mov     dx, 03C5h
        mov     al, bl
        out     dx, al
        mov     ax, y1
        mov     dx, ax
        mov     cx, y2
        inc     cx
        sub     cx, ax
        shl     dx, 1
        shl     dx, 1
        add     ax, dx
        add     ax, draw_page_seg
        mov     es, ax
        mov     al, byte ptr color
@@column:
        stosb
        add     di, 79
        loop    @@column
        ret
@@wide:
        mov     dx, y2
        mov     bx, y1
        sub     dx, bx
        inc     dx
        mov     height, dx
        mov     ax, bx
        shl     ax, 1
        shl     ax, 1
        add     ax, bx
        add     ax, draw_page_seg
        mov     es, ax
        mov     di, x1
        mov     bx, di
        shr     di, 1
        shr     di, 1
        and     bx, 3
        add     bx, offset left_masks
        mov     ah, [bx]
        mov     bl, byte ptr rnd
        xor     bh, bh
        mov     cx, height
        mov     dx, 03C5h
@@left:
        mov     al, ah
        out     dx, al
        mov     al, byte ptr color
        stosb
        dec     di
        mov     al, noise_table[bx]
        and     al, ah
        out     dx, al
        mov     al, byte ptr speck
        stosb
        add     di, 79
        inc     bl
        loop    @@left
        mov     ax, rnd
        call    Random_LFSR8
        mov     rnd, ax
        mov     di, x2
        mov     bx, di
        shr     di, 1
        shr     di, 1
        and     bx, 3
        add     bx, offset right_masks
        mov     ah, [bx]
        mov     bl, byte ptr rnd
        xor     bh, bh
        mov     cx, height
        mov     dx, 03C5h
@@right:
        mov     al, ah
        out     dx, al
        mov     al, byte ptr color
        stosb
        dec     di
        mov     al, noise_table[bx]
        and     al, ah
        out     dx, al
        mov     al, byte ptr speck
        stosb
        add     di, 79
        inc     bl
        loop    @@right
        mov     ax, rnd
        call    Random_LFSR8
        mov     rnd, ax
        mov     bx, x2
        shr     bx, 1
        shr     bx, 1
        mov     ax, x1
        shr     ax, 1
        shr     ax, 1
        sub     bx, ax
        dec     bx
        je      @@done
        mov     di, ax
        inc     di
        mov     dx, 80
        sub     dx, bx
        mov     row_skip, dx
        mov     mid, bx
        mov     bl, byte ptr rnd
        xor     bh, bh
        mov     si, height
        mov     dx, 03C5h
@@row:
        mov     cx, mid
@@byte:
        mov     al, 0Fh
        out     dx, al
        mov     al, byte ptr color
        stosb
        dec     di
        mov     al, noise_table[bx]
        and     al, 0Fh
        out     dx, al
        mov     al, byte ptr speck
        stosb
        inc     bl
        loop    @@byte
        add     di, row_skip
        mov     ax, rnd
        call    Random_LFSR8
        mov     rnd, ax
        mov     bl, al
        dec     si
        jne     @@row
@@done:
        ret
Gradient_Fill_Noise ENDP

        END
