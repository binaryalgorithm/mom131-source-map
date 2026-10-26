; TRIG.ASM - WIZARDS.EXE code segment 2500 (MAGIC.EXE 2374, the same file): integer trigonometry in whole degrees
; (atan2, value * sin, value * cos) from tables, and the two warped picture-span blitters (rotating / scaling
; pictures: a source walk with 8.8 steps along and across). Hand-written assembly (Borland TASM).
; The TABLES ARE IN THE CODE SEGMENT (2500:0006..01f4), and 14 working words sit between Sin_Mul and
; Pic_Warp_Span (2500:0417..0432).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 2500 src/wizards/TRIG.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        .DATA                           ; this module's own data, DS:78b4
plane_bits      db      01h, 02h, 04h, 08h      ; DS:78b4  the VGA plane of x & 3
        .CODE
        PUBLIC  Atan2_Deg, Atan_Deg_Core, Cos_Mul, Sin_Mul, Pic_Warp_Span, Pic_Warp_Span2
atan_small      db      00h, 04h, 08h, 0Dh, 11h, 16h, 1Ah, 1Fh, 23h, 28h, 2Dh, 31h    ; 2500:0006  tan * 256 thresholds of 0..21 degrees (low quotients)
                db      36h, 3Bh, 3Fh, 44h, 49h, 4Eh, 53h, 58h, 5Dh, 62h
atan_mid        db      67h, 6Ch, 71h, 77h, 7Ch, 82h, 88h, 8Dh, 93h, 99h, 9Fh, 0A6h    ; 2500:001c  tan * 256 thresholds of 22..44 degrees
                db      0ACh, 0B3h, 0B9h, 0C0h, 0C8h, 0CFh, 0D6h, 0DEh, 0E6h, 0EEh, 0F7h
sin_table       dw      0000h, 0477h, 08EFh, 0D65h, 11DBh, 164Fh, 1AC2h, 1F32h, 23A0h, 280Ch    ; 2500:0033  sin(0..89 degrees) * 65536
                dw      2C74h, 30D8h, 3539h, 3996h, 3DEEh, 4242h, 4690h, 4AD8h, 4F1Bh, 5358h
                dw      578Eh, 5BBEh, 5FE6h, 6407h, 681Fh, 6C30h, 7039h, 7438h, 782Fh, 7C1Ch
                dw      8000h, 83D9h, 87A8h, 8B6Dh, 8F27h, 92D5h, 9679h, 9A10h, 9D9Ch, 0A11Bh
                dw      0A48Dh, 0A7F3h, 0AB4Ch, 0AE97h, 0B1D5h, 0B505h, 0B826h, 0BB3Ah, 0BE3Eh, 0C134h
                dw      0C41Bh, 0C6F3h, 0C9BBh, 0CC73h, 0CF1Bh, 0D1B4h, 0D43Bh, 0D6B3h, 0D919h, 0DB6Fh
                dw      0DDB3h, 0DFE7h, 0E208h, 0E419h, 0E617h, 0E803h, 0E9DEh, 0EBA6h, 0ED5Bh, 0EEFFh
                dw      0F08Fh, 0F20Dh, 0F378h, 0F4D0h, 0F615h, 0F746h, 0F865h, 0F970h, 0FA67h, 0FB4Bh
                dw      0FC1Ch, 0FCD9h, 0FD82h, 0FE17h, 0FE99h, 0FF06h, 0FF60h, 0FFA6h, 0FFD8h, 0FFF6h
atan_high       dw      0100h, 0109h, 0112h, 011Ch, 0126h, 0131h, 013Ch, 0147h, 0153h, 0160h    ; 2500:00e7  tan * 256 thresholds of 45..66 degrees
                dw      016Dh, 017Bh, 018Ah, 0199h, 01AAh, 01BBh, 01CDh, 01E1h, 01F6h, 020Ch
                dw      0224h, 023Eh
atan_top        dw      025Bh, 0279h, 029Ah, 02BFh, 02E7h, 0313h, 0345h, 037Ch, 03BBh, 0402h    ; 2500:0113  tan * 256 thresholds of 67..89 degrees
                dw      0454h, 04B4h, 0525h, 05ABh, 0650h, 071Dh, 0825h, 0983h, 0B6Eh, 0E4Dh
                dw      1315h, 1CA3h, 394Dh
cos_table       dw      0FFF6h, 0FFD8h, 0FFA6h, 0FF60h, 0FF06h, 0FE98h, 0FE17h, 0FD82h, 0FCD9h, 0FC1Ch    ; 2500:0141  cos(1..90 degrees) * 65536
                dw      0FB4Bh, 0FA67h, 0F970h, 0F865h, 0F746h, 0F615h, 0F4D0h, 0F378h, 0F20Dh, 0F08Fh
                dw      0EEFFh, 0ED5Bh, 0EBA6h, 0E9DEh, 0E803h, 0E617h, 0E418h, 0E208h, 0DFE7h, 0DDB3h
                dw      0DB6Fh, 0D919h, 0D6B3h, 0D43Bh, 0D1B3h, 0CF1Bh, 0CC73h, 0C9BBh, 0C6F2h, 0C41Bh
                dw      0C134h, 0BE3Eh, 0BB39h, 0B826h, 0B504h, 0B1D5h, 0AE97h, 0AB4Ch, 0A7F3h, 0A48Dh
                dw      0A11Bh, 9D9Bh, 9A10h, 9678h, 92D5h, 8F27h, 8B6Dh, 87A8h, 83D9h, 7FFFh
                dw      7C1Ch, 782Fh, 7438h, 7038h, 6C30h, 681Fh, 6406h, 5FE6h, 5BBDh, 578Eh
                dw      5358h, 4F1Bh, 4AD8h, 468Fh, 4241h, 3DEEh, 3996h, 3539h, 30D8h, 2C73h
                dw      280Bh, 23A0h, 1F32h, 1AC2h, 164Fh, 11DBh, 0D65h, 08EEh, 0477h, 0000h

; (dx, dy) -> the angle of the vector in degrees 0..359 (0 = +x, counter-clockwise with y up)
; C:  int Atan2_Deg(int dx, int dy) { int a = atan_deg(abs(dy), abs(dx));   (Atan_Deg_Core: 0..90)
; C:      if (dx >= 0) return dy < 0 ? 360 - a : a; return dy < 0 ? 180 + a : 180 - a; }
Atan2_Deg PROC USES es ds si di, vx:WORD, vy:WORD
        mov     bx, vx
        mov     ax, vy
        cmp     bx, 0
        js      @@left
        cmp     ax, 0
        jns     @@q1
        neg     ax
        call    Atan_Deg_Core
        mov     bx, 360
        sub     bx, ax
        mov     ax, bx
        ret
@@q1:
        call    Atan_Deg_Core
        ret
@@left:
        cmp     ax, 0
        jns     @@q2
        neg     ax
        neg     bx
        call    Atan_Deg_Core
        add     ax, 180
        ret
@@q2:
        neg     bx
        call    Atan_Deg_Core
        mov     bx, 180
        sub     bx, ax
        mov     ax, bx
        ret
Atan2_Deg ENDP

; near register call (AX = |dy|, BX = |dx|) -> AX = atan(dy / dx) in degrees 0..90: q = dy * 256 / dx, then the first
; table entry above q (the byte tables for q < 100h, the word tables above); dx = 0 -> 90
; C:  int Atan_Deg_Core(unsigned dy, unsigned dx) { unsigned q; if (!dx) return 90; q = ((long)dy << 8) / dx;
; C:      return the degree d whose tan(d) * 256 threshold is the last one <= q; }
Atan_Deg_Core PROC NEAR USES es ds si di
        xor     dh, dh
        mov     dl, ah
        mov     ah, al
        xor     al, al
        cmp     bx, 0
        je      @@vertical
        div     bx
        mov     bx, ax
        mov     ax, ds
        mov     es, ax
        mov     ax, cs
        mov     ds, ax
        cmp     bh, 0
        jne     @@big
        cmp     bl, 67h
        jb      @@small
        mov     si, offset atan_mid
        mov     cx, 23
        mov     di, 22
@@mid_loop:
        lodsb
        cmp     bl, al
        jb      @@mid_found
        inc     di
        loop    @@mid_loop
        mov     di, 44
@@mid_found:
        dec     di
        mov     ax, di
        ret
@@small:
        mov     si, offset atan_small
        mov     cx, 22
        mov     di, 0
@@small_loop:
        lodsb
        cmp     bl, al
        jb      @@small_found
        inc     di
        loop    @@small_loop
        mov     di, 22
@@small_found:
        dec     di
        mov     ax, di
        ret
@@big:
        cmp     bx, 25Bh
        jb      @@high
        mov     si, offset atan_top
        mov     cx, 23
        mov     di, 0
@@top_loop:
        lodsw
        cmp     bx, ax
        jb      @@top_found
        inc     di
        loop    @@top_loop
@@vertical:
        mov     ax, 90
        ret
@@top_found:
        dec     di
        mov     ax, di
        add     ax, 67
        ret
@@high:
        mov     si, offset atan_high
        mov     cx, 22
        mov     di, 0
@@high_loop:
        lodsw
        cmp     bx, ax
        jb      @@high_found
        inc     di
        loop    @@high_loop
        mov     ax, 66
        ret
@@high_found:
        dec     di
        mov     ax, di
        add     ax, 45
        ret
Atan_Deg_Core ENDP

; (angle 0..359, value) -> value * cos(angle) (the high word of the 16 x 16 product with the table's 0.16 cosine)
; C:  int Cos_Mul(int angle, int value) { return (int)(value * cos(angle * PI / 180)); }   (truncated toward 0)
Cos_Mul PROC USES es ds si di, angle:WORD, value:WORD
        mov     bx, value
        mov     dx, 0
        mov     ax, angle
        cmp     ax, 91
        js      @@first
        cmp     ax, 181
        js      @@second
        cmp     ax, 271
        js      @@third
        mov     cx, 360
        sub     cx, ax
        mov     ax, cx
        jmp     @@first
@@third:
        sub     ax, 180
        mov     dx, 1
        jmp     @@first
@@second:
        mov     cx, 180
        sub     cx, ax
        mov     ax, cx
        mov     dx, 1
        jmp     @@first
@@first:
        mov     cx, dx
        mov     si, ax
        cmp     si, 0
        je      @@zero
        dec     si
        shl     si, 1
        mov     ax, cs
        mov     ds, ax
        add     si, offset cos_table
        lodsw
        cmp     bx, 0
        js      @@negative
        mul     bx
        mov     ax, dx
        cmp     cx, 0
        je      @@done
        neg     ax
@@done:
        ret
@@negative:
        neg     bx
        mul     bx
        mov     ax, dx
        neg     ax
        cmp     cx, 0
        je      @@done
        neg     ax
        ret
@@zero:
        mov     ax, bx
        cmp     cx, 0
        je      @@done
        neg     ax
        ret
Cos_Mul ENDP

; (angle 0..359, value) -> value * sin(angle)
; C:  int Sin_Mul(int angle, int value) { return (int)(value * sin(angle * PI / 180)); }
Sin_Mul PROC USES es ds si di, sangle:WORD, svalue:WORD
        mov     bx, svalue
        mov     dx, 0
        mov     ax, sangle
        cmp     ax, 91
        js      @@first
        cmp     ax, 181
        js      @@second
        cmp     ax, 271
        js      @@third
        mov     cx, 360
        sub     cx, ax
        mov     ax, cx
        mov     dx, 1
        jmp     @@first
@@third:
        sub     ax, 180
        mov     dx, 1
        jmp     @@first
@@second:
        mov     cx, 180
        sub     cx, ax
        mov     ax, cx
        jmp     @@first
@@first:
        mov     cx, dx
        mov     si, ax
        cmp     si, 90
        je      @@right
        shl     si, 1
        mov     ax, cs
        mov     ds, ax
        add     si, offset sin_table
        lodsw
        cmp     bx, 0
        js      @@negative
        mul     bx
        mov     ax, dx
        cmp     cx, 0
        je      @@done
        neg     ax
@@done:
        ret
@@negative:
        neg     bx
        mul     bx
        mov     ax, dx
        neg     ax
        cmp     cx, 0
        je      @@done
        neg     ax
        ret
@@right:
        mov     ax, svalue
        cmp     cx, 0
        je      @@done
        neg     ax
        ret
Sin_Mul ENDP

acc_across      dw      0               ; 2500:0417  8.8 accumulator of the step across columns
span_len        dw      0               ; 2500:0419  8.8 span length (rows drawn per column)
acc_src1        dw      0               ; 2500:041b  8.8 accumulators of the source walk between columns
acc_src2        dw      0               ; 2500:041d
unused_41f      dw      0               ; 2500:041f
unused_421      dw      0               ; 2500:0421
step_b          dw      0               ; 2500:0423  8.8 inner source steps (arguments +26h..+2Ch)
step_b_add      dw      0               ; 2500:0425
step_a          dw      0               ; 2500:0427
step_a_add      dw      0               ; 2500:0429
step_base       dw      0               ; 2500:042b  source step per row (argument +24h)
start_skip      dw      0               ; 2500:042d  columns not drawn at the start (argument +2Eh; counts up from < 0)
clip_lo         dw      0               ; 2500:042f  screen offsets outside which nothing is written
clip_hi         dw      0               ; 2500:0431

; (x, y, columns, len, len_step, col_step, frac, col_step2, src, src_seg, src_step, ... 23 words) draws a warped
; picture column by column: each column walks the source with 8.8 steps (step_a / step_b), each next column moves
; the screen start and the source start by 8.8 steps; pixel 0 = transparent; offsets outside clip_lo .. clip_hi are
; not written
; C:  (see the argument list: for each of 'columns' columns, for each of span_len >> 8 rows: if (src_pixel &&
; C:      clip_lo <= dst < clip_hi) screen[dst] = src_pixel; advance src by the 8.8 steps) }
Pic_Warp_Span PROC USES es ds si di, wx:WORD, wy:WORD, wcols:WORD, wlen:WORD, wlen_step:WORD, wcol_step:WORD, wfrac:WORD, wcol_step2:WORD, wsrc:WORD, wsrc_seg:WORD, wsrc_step:WORD, wa1:WORD, wa1_add:WORD, wa2:WORD, wa2_add:WORD, wstep_base:WORD, wstep_b:WORD, wstep_b_add:WORD, wstep_a:WORD, wstep_a_add:WORD, wskip:WORD, wclip_lo:WORD, wclip_hi:WORD
        mov     ax, wskip
        mov     cs:start_skip, ax
        mov     ax, wclip_lo
        mov     cs:clip_lo, ax
        mov     ax, wclip_hi
        mov     cs:clip_hi, ax
        mov     ax, wstep_b
        mov     cs:step_b, ax
        mov     ax, wstep_b_add
        mov     cs:step_b_add, ax
        mov     ax, wstep_a
        mov     cs:step_a, ax
        mov     ax, wstep_a_add
        mov     cs:step_a_add, ax
        mov     ax, wstep_base
        mov     cs:step_base, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     ax, draw_page_seg
        mov     es, ax
        mov     cx, wy
        mov     ax, cx
        shl     cx, 1
        shl     cx, 1
        add     cx, ax
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        mov     ax, wx
        mov     bx, ax
        shr     bx, 1
        shr     bx, 1
        add     bx, cx
        mov     di, bx
        and     ax, 3
        mov     si, offset plane_bits
        add     si, ax
        lodsb
        mov     ah, al
        mov     si, wsrc
        mov     cs:acc_across, 80h
        mov     cs:acc_src1, 80h
        mov     cs:acc_src2, 80h
        mov     cx, wlen
        mov     ch, cl
        xor     cl, cl
        mov     cs:span_len, cx
@@column:
        push    di
        push    si
        cmp     cs:start_skip, 0
        jns     @@next_column
        mov     cx, cs:span_len
        mov     cl, ch
        xor     ch, ch
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, wsrc_seg
        mov     ds, dx
        mov     dx, 0
        mov     bx, 0
@@pixel:
        lodsb
        dec     si
        inc     di
        cmp     al, 0
        je      @@skip
        cmp     di, cs:clip_lo
        jb      @@skip
        cmp     di, cs:clip_hi
        jae     @@skip
        dec     di
        stosb
@@skip:
        add     di, 79
        add     si, cs:step_base
        add     dx, cs:step_a
        cmp     dh, 0
        je      @@no_a
        xor     dh, dh
        add     si, cs:step_a_add
@@no_a:
        add     bx, cs:step_b
        cmp     bh, 0
        je      @@no_b
        xor     bh, bh
        add     si, cs:step_b_add
@@no_b:
        loop    @@pixel
@@next_column:
        dec     cs:start_skip
        pop     si
        pop     di
        mov     cx, DGROUP
        mov     ds, cx
        dec     wcols
        je      @@done
        add     di, wcol_step
        mov     cx, cs:acc_across
        add     cx, wfrac
        cmp     ch, 0
        je      @@no_across
        xor     ch, ch
        add     di, wcol_step2
@@no_across:
        mov     cs:acc_across, cx
        mov     cx, cs:span_len
        add     cx, wlen_step
        mov     cs:span_len, cx
        add     si, wsrc_step
        mov     cx, cs:acc_src1
        add     cx, wa2
        cmp     ch, 0
        je      @@no_src1
        xor     ch, ch
        add     si, wa2_add
@@no_src1:
        mov     cs:acc_src1, cx
        mov     cx, cs:acc_src2
        add     cx, wa1
        cmp     ch, 0
        je      @@no_src2
        xor     ch, ch
        add     si, wa1_add
@@no_src2:
        mov     cs:acc_src2, cx
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        jmp     @@column
@@done:
        ret
Pic_Warp_Span ENDP

; the same with the translucent remap pixels (E8h..FFh through the remap tables, read map via 3CEh index 4)
; C:  as Pic_Warp_Span, pixel p >= 0xE8: screen = remap[p - 0xE8][screen]
Pic_Warp_Span2 PROC USES es ds si di, vx2:WORD, vy2:WORD, vcols:WORD, vlen:WORD, vlen_step:WORD, vcol_step:WORD, vfrac:WORD, vcol_step2:WORD, vsrc:WORD, vsrc_seg:WORD, vsrc_step:WORD, va1:WORD, va1_add:WORD, va2:WORD, va2_add:WORD, vstep_base:WORD, vstep_b:WORD, vstep_b_add:WORD, vstep_a:WORD, vstep_a_add:WORD, vskip:WORD, vclip_lo:WORD, vclip_hi:WORD
        mov     ax, vskip
        mov     cs:start_skip, ax
        mov     ax, vclip_lo
        mov     cs:clip_lo, ax
        mov     ax, vclip_hi
        mov     cs:clip_hi, ax
        mov     ax, vstep_b
        mov     cs:step_b, ax
        mov     ax, vstep_b_add
        mov     cs:step_b_add, ax
        mov     ax, vstep_a
        mov     cs:step_a, ax
        mov     ax, vstep_a_add
        mov     cs:step_a_add, ax
        mov     ax, vstep_base
        mov     cs:step_base, ax
        mov     ax, draw_page_seg
        mov     es, ax
        mov     cx, vy2
        mov     ax, cx
        shl     cx, 1
        shl     cx, 1
        add     cx, ax
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        mov     ax, vx2
        mov     bx, ax
        shr     bx, 1
        shr     bx, 1
        add     bx, cx
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
        mov     si, vsrc
        mov     cs:acc_across, 80h
        mov     cs:acc_src1, 80h
        mov     cs:acc_src2, 80h
        mov     cx, vlen
        mov     ch, cl
        xor     cl, cl
        mov     cs:span_len, cx
@@column:
        push    di
        push    si
        cmp     cs:start_skip, 0
        js      @@draw
        jmp     @@next_column
@@draw:
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
        mov     cx, cs:span_len
        mov     cl, ch
        xor     ch, ch
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, vsrc_seg
        mov     ds, dx
        mov     dx, 0
        mov     bx, 0
@@pixel:
        lodsb
        dec     si
        inc     di
        cmp     al, 0
        je      @@skip
        cmp     di, cs:clip_lo
        jb      @@skip
        cmp     di, cs:clip_hi
        jae     @@skip
        dec     di
        cmp     al, 0E8h
        jae     @@remap
        stosb
@@skip:
        add     di, 79
        add     si, cs:step_base
        add     dx, cs:step_a
        cmp     dh, 0
        je      @@no_a
        xor     dh, dh
        add     si, cs:step_a_add
@@no_a:
        add     bx, cs:step_b
        cmp     bh, 0
        je      @@no_b
        xor     bh, bh
        add     si, cs:step_b_add
@@no_b:
        loop    @@pixel
        jmp     @@next_column
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
        jmp     @@skip
@@next_column:
        dec     cs:start_skip
        pop     si
        pop     di
        mov     cx, DGROUP
        mov     ds, cx
        dec     vcols
        je      @@done
        add     di, vcol_step
        mov     cx, cs:acc_across
        add     cx, vfrac
        cmp     ch, 0
        je      @@no_across
        xor     ch, ch
        add     di, vcol_step2
@@no_across:
        mov     cs:acc_across, cx
        mov     cx, cs:span_len
        add     cx, vlen_step
        mov     cs:span_len, cx
        add     si, vsrc_step
        mov     cx, cs:acc_src1
        add     cx, va2
        cmp     ch, 0
        je      @@no_src1
        xor     ch, ch
        add     si, va2_add
@@no_src1:
        mov     cs:acc_src1, cx
        mov     cx, cs:acc_src2
        add     cx, va1
        cmp     ch, 0
        je      @@no_src2
        xor     ch, ch
        add     si, va1_add
@@no_src2:
        mov     cs:acc_src2, cx
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
Pic_Warp_Span2 ENDP

        END
