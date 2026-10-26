; PALUPLD.ASM - WIZARDS.EXE code segment 1cde (MAGIC.EXE 1c4c, the same file): getting the palette into the VGA DAC.
; p_palette is a segment: 256 x 3 bytes (6-bit r, g, b) and at +300h one DIRTY flag per color; the uploads send only
; the dirty colors (128 per vertical retrace) and then clear all 256 flags. Also the remap tables (color -> nearest
; tinted color, 256 bytes each at remap_tables_seg + 16 * table) and the palette embedded in a picture header.
; Hand-written assembly (Borland TASM): port I/O to the DAC with interrupts off around each color.
; 22 WORDS IN THE CODE SEGMENT at the module start (1cde:000c), shared by the procedures.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names: outportb / inportb / disable / enable) - a
; reference for a port, not compiled; a port keeps the palette as an array and sets it whole.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1cde src/wizards/PALUPLD.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   ems_frame_seg:WORD      ; DS:760c
        EXTRN   remap_tables_seg:WORD   ; DS:e7e4
        EXTRN   p_palette:WORD          ; DS:e7e8
        .CODE
        PUBLIC  Palette_Upload, Palette_Darken_Percent, Palette_Tint_Upload, Palette_Rotate_Upload, Palette_Rotate
        PUBLIC  Remap_Build_Table, Pic_Load_Palette
tint_r          dw      0               ; 1cde:000c  target color (Remap_Build_Table: as passed; Palette_Tint_Upload:
tint_b          dw      0               ; 1cde:000e      scaled by the percentage) - note the order r, b, g
tint_g          dw      0               ; 1cde:0010
rot_dac1        dw      0               ; 1cde:0012  Palette_Rotate_Upload: first DAC index of part 1
rot_count1      dw      0               ; 1cde:0014      colors in part 1
rot_src1        dw      0               ; 1cde:0016      first palette color of part 1
rot_dac2        dw      0               ; 1cde:0018      part 2 (the colors that wrap around)
rot_count2      dw      0               ; 1cde:001a
rot_src2        dw      0               ; 1cde:001c
cyc_first       dw      0               ; 1cde:001e  Palette_Rotate: first color
cyc_count       dw      0               ; 1cde:0020      colors
cyc_step        dw      0               ; 1cde:0022      step (mod 2 * count)
best_index      dw      0               ; 1cde:0024  Remap_Build_Table: nearest color so far
best_dist       dw      0               ; 1cde:0026      its squared distance (start 10000)
blend_r         dw      0               ; 1cde:0028      the blended color being matched (low bytes)
blend_g         dw      0               ; 1cde:002a
blend_b         dw      0               ; 1cde:002c
keep_scale      dw      0               ; 1cde:002e  (100 - percent) * 256 / 100
color_index     dw      0               ; 1cde:0030  Remap_Build_Table: the color being remapped
add_r           dw      0               ; 1cde:0032  percent * 256 / 100 * r >> 8 (high byte of the product, as a byte)
add_g           dw      0               ; 1cde:0034
add_b           dw      0               ; 1cde:0036

; () uploads every dirty color, 128 per vertical retrace, then clears the 256 dirty flags
; C:  void Palette_Upload(void) { int i, n = 0; unsigned char far *pal = MK_FP(p_palette, 0);
; C:      for (i = 0; i < 256; i++) {
; C:          if (n == 0) { n = 128; while (inportb(0x3DA) & 8) ; while (!(inportb(0x3DA) & 8)) ; }
; C:          if (pal[0x300 + i]) { disable(); outportb(0x3C8, i); outportb(0x3C9, pal[i * 3]);
; C:              outportb(0x3C9, pal[i * 3 + 1]); outportb(0x3C9, pal[i * 3 + 2]); n--; enable(); } }
; C:      _fmemset(pal + 0x300, 0, 256); }
Palette_Upload PROC USES si di es ds
        mov     ax, p_palette
        mov     ds, ax
        mov     si, 0
        mov     bx, 0
        mov     di, 300h
        mov     cx, 0
@@next:
        cmp     cx, 0
        jne     @@ready
        mov     dx, 03DAh
        mov     cx, 80h
@@in_retrace:
        in      al, dx
        and     al, 8
        jne     @@in_retrace
@@wait_retrace:
        in      al, dx
        and     al, 8
        je      @@wait_retrace
@@ready:
        add     si, 3
        cmp     byte ptr [di], 0
        je      @@skip
        mov     dx, 03C8h
        sub     si, 3
        cli
        mov     al, bl
        out     dx, al
        inc     dx
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        dec     cx
        sti
@@skip:
        inc     di
        inc     bx
        cmp     bh, 0
        je      @@next
        mov     ax, ds
        mov     es, ax
        mov     di, 300h
        mov     cx, 80h
        mov     ax, 0
        rep     stosw
        ret
Palette_Upload ENDP

; (percent) uploads the dirty colors darkened by percent: component * ((100 - percent) * 256 / 100) >> 8;
; percent <= 0 -> Palette_Upload, percent >= 100 -> black
; C:  void Palette_Darken_Percent(int percent) { unsigned k;
; C:      if (percent >= 100) { (as Palette_Upload, every component 0) return; }
; C:      if (percent <= 0) { Palette_Upload(); return; }
; C:      k = (100 - percent) * 256 / 100; (as Palette_Upload, component c sent as (c * k) >> 8) }
Palette_Darken_Percent PROC USES si di es ds, percent:WORD
        mov     ax, 100
        sub     ax, percent
        jbe     @@black
        cmp     ax, 100
        jns     @@plain
        mov     bx, 100h
        mul     bx
        mov     bx, 100
        div     bx
        mov     dx, p_palette
        mov     ds, dx
        mov     si, 0
        mov     bl, al
        mov     bh, 0
        mov     di, 300h
        mov     cx, 0
@@next:
        cmp     cx, 0
        jne     @@ready
        mov     dx, 03DAh
        mov     cx, 80h
@@in_retrace:
        in      al, dx
        and     al, 8
        jne     @@in_retrace
@@wait_retrace:
        in      al, dx
        and     al, 8
        je      @@wait_retrace
@@ready:
        add     si, 3
        cmp     byte ptr [di], 0
        je      @@skip
        mov     dx, 03C8h
        sub     si, 3
        cli
        mov     al, bh
        out     dx, al
        inc     dx
        lodsb
        mul     bl
        mov     al, ah
        out     dx, al
        lodsb
        mul     bl
        mov     al, ah
        out     dx, al
        lodsb
        mul     bl
        mov     al, ah
        out     dx, al
        sti
        dec     cx
@@skip:
        inc     di
        inc     bh
        cmp     bh, 0
        jne     @@next
        jmp     @@clear
        ret
@@plain:
        call    Palette_Upload
        ret
@@black:
        mov     ax, p_palette
        mov     ds, ax
        mov     si, 0
        mov     bx, 0
        mov     di, 300h
        mov     cx, 0
@@b_next:
        cmp     cx, 0
        jne     @@b_ready
        mov     dx, 03DAh
        mov     cx, 80h
@@b_in_retrace:
        in      al, dx
        and     al, 8
        jne     @@b_in_retrace
@@b_wait_retrace:
        in      al, dx
        and     al, 8
        je      @@b_wait_retrace
@@b_ready:
        add     si, 3
        cmp     byte ptr [di], 0
        je      @@b_skip
        mov     dx, 03C8h
        sub     si, 3
        cli
        mov     al, bl
        out     dx, al
        inc     dx
        xor     al, al
        out     dx, al
        out     dx, al
        out     dx, al
        sti
        dec     cx
@@b_skip:
        inc     di
        inc     bx
        cmp     bh, 0
        je      @@b_next
@@clear:
        mov     ax, ds
        mov     es, ax
        mov     di, 300h
        mov     cx, 80h
        mov     ax, 0
        rep     stosw
        ret
Palette_Darken_Percent ENDP

; (percent, r, g, b) uploads the dirty colors blended toward (r, g, b) by percent (combat spell flash, 0..40%);
; color 0 is left out of the blend; percent <= 0 -> Palette_Upload; percent >= 100 -> every dirty color = tint_r /
; tint_g / tint_b AS LEFT BY THE PREVIOUS CALL (the blend values, or Remap_Build_Table's raw r, g, b) - the caller's
; r, g, b are not stored on that path. No caller passes 100 (BUG #385: latent).
; C:  void Palette_Tint_Upload(int percent, int r, int g, int b) { unsigned keep, add;
; C:      if (percent <= 0) { Palette_Upload(); return; }   if (percent >= 100) { (upload tint_r/g/b) return; }
; C:      keep = (100 - percent) * 256 / 100; add = percent * 256 / 100;
; C:      tint_r = (unsigned char)r * add >> 8; tint_g = ...; tint_b = ...;
; C:      (as Palette_Upload for colors 1..255, component c sent as (c * keep >> 8) + tint) }
; Simtex name Tint_Palette (MoO2 symbols, module palstore); clarified using ReMoM repo
Palette_Tint_Upload PROC USES si di es ds, tpercent:WORD, tr:WORD, tg:WORD, tb:WORD
        mov     ax, 100
        sub     ax, tpercent
        ja      @@some
        jmp     @@solid
@@some:
        cmp     ax, 100
        js      @@blend
        jmp     @@plain
@@blend:
        mov     bx, 100h
        mul     bx
        mov     bx, 100
        div     bx
        mov     cs:keep_scale, ax
        mov     ax, tpercent
        mov     bx, 100h
        mul     bx
        mov     bx, 100
        div     bx
        mov     bx, ax
        mov     ax, tr
        mul     bl
        mov     al, ah
        xor     ah, ah
        mov     cs:tint_r, ax
        mov     ax, tg
        mul     bl
        mov     al, ah
        xor     ah, ah
        mov     cs:tint_g, ax
        mov     ax, tb
        mul     bl
        mov     al, ah
        xor     ah, ah
        mov     cs:tint_b, ax
        mov     si, 3
        mov     bx, cs:keep_scale
        mov     bh, 1
        mov     di, 303h
        mov     cx, 0
        mov     ax, p_palette
        mov     ds, ax
@@next:
        cmp     cx, 0
        jne     @@ready
        mov     dx, 03DAh
        mov     cx, 80h
@@in_retrace:
        in      al, dx
        and     al, 8
        jne     @@in_retrace
@@wait_retrace:
        in      al, dx
        and     al, 8
        je      @@wait_retrace
@@ready:
        add     si, 3
        cmp     byte ptr [di], 0
        je      @@skip
        mov     dx, 03C8h
        sub     si, 3
        cli
        mov     al, bh
        out     dx, al
        inc     dx
        lodsb
        mul     bl
        mov     al, ah
        add     al, byte ptr cs:tint_r
        out     dx, al
        lodsb
        mul     bl
        mov     al, ah
        add     al, byte ptr cs:tint_g
        out     dx, al
        lodsb
        mul     bl
        mov     al, ah
        add     al, byte ptr cs:tint_b
        out     dx, al
        sti
        dec     cx
@@skip:
        inc     di
        inc     bh
        cmp     bh, 0
        jne     @@next
        jmp     @@clear
@@plain:
        call    Palette_Upload
        ret
@@solid:
        mov     ax, p_palette
        mov     ds, ax
        mov     bx, 0
        mov     di, 300h
        mov     cx, 0
@@s_next:
        cmp     cx, 0
        jne     @@s_ready
        mov     dx, 03DAh
        mov     cx, 80h
@@s_in_retrace:
        in      al, dx
        and     al, 8
        jne     @@s_in_retrace
@@s_wait_retrace:
        in      al, dx
        and     al, 8
        je      @@s_wait_retrace
@@s_ready:
        add     si, 3
        cmp     byte ptr [di], 0
        je      @@s_skip
        mov     dx, 03C8h
        sub     si, 3
        cli
        mov     al, bl
        out     dx, al
        inc     dx
        mov     al, byte ptr cs:tint_r
        out     dx, al
        mov     al, byte ptr cs:tint_g
        out     dx, al
        mov     al, byte ptr cs:tint_b
        out     dx, al
        sti
        dec     cx
@@s_skip:
        inc     di
        inc     bx
        cmp     bh, 0
        je      @@s_next
@@clear:
        mov     ax, ds
        mov     es, ax
        mov     di, 300h
        mov     cx, 80h
        mov     ax, 0
        rep     stosw
        ret
Palette_Tint_Upload ENDP

; (first, count, step) color cycling: palette colors first .. first + count - 1 go to the DAC rotated by step
; (color first + i -> DAC first + (i + step) mod count), all of them, during one retrace; then clears the dirty flags
; C:  void Palette_Rotate_Upload(int first, unsigned count, unsigned step) { unsigned i; step %= count;
; C:      (wait for retrace) for (i = 0; i < count - step; i++) (DAC first + step + i <- color first + i);
; C:      if (step) { (wait for retrace) for (i = 0; i < step; i++) (DAC first + i <- color first + count - step + i); }
; C:      _fmemset(pal + 0x300, 0, 256); }
Palette_Rotate_Upload PROC USES si di es ds, rfirst:WORD, rcount:WORD, rstep:WORD
        mov     dx, 0
        mov     ax, rstep
        mov     bx, rcount
        div     bx
        mov     rstep, dx
        add     dx, rfirst
        mov     cs:rot_dac1, dx
        mov     ax, rcount
        sub     ax, rstep
        mov     cs:rot_count1, ax
        mov     ax, rfirst
        mov     cs:rot_src1, ax
        mov     ax, rfirst
        mov     cs:rot_dac2, ax
        mov     ax, rstep
        mov     cs:rot_count2, ax
        mov     ax, rcount
        sub     ax, rstep
        add     ax, rfirst
        mov     cs:rot_src2, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     si, cs:rot_src1
        mov     ax, si
        shl     si, 1
        add     si, ax
        mov     bx, cs:rot_dac1
        mov     cx, cs:rot_count1
        mov     dx, 03DAh
@@in_retrace:
        in      al, dx
        and     al, 8
        jne     @@in_retrace
@@wait_retrace:
        in      al, dx
        and     al, 8
        je      @@wait_retrace
        mov     dx, 03C8h
@@part1:
        cli
        mov     al, bl
        out     dx, al
        inc     dx
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        sti
        dec     dx
        inc     bx
        loop    @@part1
        mov     si, cs:rot_src2
        mov     ax, si
        shl     si, 1
        add     si, ax
        mov     bx, cs:rot_dac2
        mov     cx, cs:rot_count2
        cmp     cx, 0
        je      @@clear
        mov     dx, 03DAh
@@in_retrace2:
        in      al, dx
        and     al, 8
        jne     @@in_retrace2
@@wait_retrace2:
        in      al, dx
        and     al, 8
        je      @@wait_retrace2
        mov     dx, 03C8h
@@part2:
        cli
        mov     al, bl
        out     dx, al
        inc     dx
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        sti
        dec     dx
        inc     bx
        loop    @@part2
@@clear:
        mov     ax, ds
        mov     es, ax
        mov     di, 300h
        mov     cx, 80h
        mov     ax, 0
        rep     stosw
        ret
Palette_Rotate_Upload ENDP

; (first, count, step) no callers: ping-pong color cycling - DAC first + i <- color first + j, j = (i + step) mod
; 2 * count folded back at count (count .. 2 * count - 1 run backwards); all during one retrace
; C:  void Palette_Rotate(int first, int count, unsigned step) { int i, j; step %= 2 * count; (wait for retrace)
; C:      for (i = 0; i < count; i++) { j = i + step; if (j >= count) { j -= count; j = j >= count ? j - count
; C:          : count - 1 - j; } (DAC first + i <- color first + j); }  _fmemset(pal + 0x300, 0, 256); }
Palette_Rotate PROC USES si di es ds, cfirst:WORD, ccount:WORD, cstep:WORD
        mov     dx, 0
        mov     ax, cstep
        mov     bx, ccount
        shl     bx, 1
        div     bx
        mov     cstep, dx
        mov     cs:cyc_step, dx
        mov     bx, cfirst
        mov     cs:cyc_first, bx
        mov     cx, ccount
        mov     cs:cyc_count, cx
        mov     di, 0
        mov     ax, p_palette
        mov     ds, ax
        mov     dx, 03DAh
@@in_retrace:
        in      al, dx
        and     al, 8
        jne     @@in_retrace
@@wait_retrace:
        in      al, dx
        and     al, 8
        je      @@wait_retrace
        mov     dx, 03C8h
@@entry:
        mov     si, cs:cyc_count
        mov     ax, di
        add     ax, cs:cyc_step
        cmp     ax, si
        js      @@index
        sub     ax, si
        cmp     ax, si
        js      @@backward
        sub     ax, si
        jmp     @@index
@@backward:
        sub     si, ax
        dec     si
        mov     ax, si
@@index:
        add     ax, cs:cyc_first
        mov     si, ax
        shl     si, 1
        add     si, ax
        cli
        mov     al, bl
        out     dx, al
        inc     dx
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        lodsb
        out     dx, al
        sti
        dec     dx
        inc     di
        inc     bx
        loop    @@entry
        mov     ax, ds
        mov     es, ax
        mov     di, 300h
        mov     cx, 80h
        mov     ax, 0
        rep     stosw
        ret
Palette_Rotate ENDP

; (table, r, g, b, percent) builds remap table 'table' (256 bytes at remap_tables_seg + 16 * table):
; percent >= 100 -> every entry = the color nearest (r, g, b); percent <= 0 -> identity; otherwise for each DIRTY
; color c: blend c * (100 - percent)% + rgb * percent%, entry c = the nearest DIRTY color (squared distance, only
; colors within 20 on every channel count; none -> 0). Clean colors keep their old entries.
; C:  void Remap_Build_Table(int table, int r, int g, int b, int percent) {
; C:      unsigned char far *pal = MK_FP(p_palette, 0), far *map = MK_FP(remap_tables_seg + table * 16, 0);
; C:      int c, k, best, dist, d;  tint_r = r; tint_g = g; tint_b = b;
; C:      if (percent >= 100) { best = 0; dist = 10000; for (k = 0; k < 256; k++) { d = (distance of color k to
; C:          r, g, b; skip if a channel differs by 21 or more); if (d < dist) { dist = d; best = k; } }
; C:          _fmemset(map, best, 256); return; }
; C:      if (percent <= 0) { for (c = 0; c < 256; c++) map[c] = c; return; }
; C:      for (c = 0; c < 256; c++) if (pal[0x300 + c]) { (blend color c); best = 0; dist = 10000;
; C:          for (k = 0; k < 256; k++) if (pal[0x300 + k]) { (as above, d < dist, unsigned) } map[c] = best; } }
Remap_Build_Table PROC USES si di es ds, table:WORD, mr:WORD, mg:WORD, mb:WORD, mpercent:WORD
        mov     ax, mr
        mov     bx, mg
        mov     cx, mb
        mov     cs:tint_r, ax
        mov     cs:tint_g, bx
        mov     cs:tint_b, cx
        mov     ax, 100
        sub     ax, mpercent
        ja      @@some
        jmp     @@nearest
@@some:
        cmp     ax, 100
        js      @@blend
        jmp     @@identity
@@blend:
        mov     bx, 100h
        mul     bx
        mov     bx, 100
        div     bx
        mov     cs:keep_scale, ax
        mov     ax, mpercent
        mov     bx, 100h
        mul     bx
        mov     bx, 100
        div     bx
        mov     bx, ax
        mov     al, byte ptr mr
        mul     bl
        mov     byte ptr cs:add_r, ah
        mov     al, byte ptr mg
        mul     bl
        mov     byte ptr cs:add_g, ah
        mov     al, byte ptr mb
        mul     bl
        mov     byte ptr cs:add_b, ah
        mov     bx, table
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        mov     ax, remap_tables_seg
        add     ax, bx
        mov     es, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     cs:color_index, 0
@@color:
        mov     si, cs:color_index
        add     si, 300h
        lodsb
        or      al, al
        jne     @@dirty
        jmp     @@next_color
@@dirty:
        mov     si, cs:color_index
        mov     ax, si
        shl     si, 1
        add     si, ax
        mov     bl, byte ptr cs:keep_scale
        lodsb
        mul     bl
        add     ah, byte ptr cs:add_r
        mov     byte ptr cs:blend_r, ah
        lodsb
        mul     bl
        add     ah, byte ptr cs:add_g
        mov     byte ptr cs:blend_g, ah
        lodsb
        mul     bl
        add     ah, byte ptr cs:add_b
        mov     byte ptr cs:blend_b, ah
        mov     cs:best_index, 0
        mov     cs:best_dist, 10000
        mov     cx, 100h
        mov     si, 0
        mov     di, 0
@@candidate:
        mov     al, [di+300h]
        add     si, 3
        or      al, al
        je      @@next_candidate
        sub     si, 3
        lodsb
        sub     al, byte ptr cs:blend_r
        jns     @@r_abs
        neg     al
@@r_abs:
        add     si, 2
        cmp     al, 15h
        jns     @@next_candidate
        sub     si, 2
        mov     bl, al
        lodsb
        sub     al, byte ptr cs:blend_g
        jns     @@g_abs
        neg     al
@@g_abs:
        inc     si
        cmp     al, 15h
        jns     @@next_candidate
        dec     si
        mov     bh, al
        lodsb
        sub     al, byte ptr cs:blend_b
        jns     @@b_abs
        neg     al
@@b_abs:
        cmp     al, 15h
        jns     @@next_candidate
        mul     al
        mov     dx, ax
        mov     al, bl
        mul     al
        add     dx, ax
        mov     al, bh
        mul     al
        add     dx, ax
        cmp     dx, cs:best_dist
        jae     @@next_candidate
        mov     cs:best_dist, dx
        mov     cs:best_index, di
@@next_candidate:
        inc     di
        loop    @@candidate
        mov     di, cs:color_index
        mov     al, byte ptr cs:best_index
        stosb
@@next_color:
        mov     ax, cs:color_index
        inc     ax
        mov     cs:color_index, ax
        cmp     ah, 0
        jne     @@done
        jmp     @@color
@@done:
        ret
@@identity:
        mov     ax, table
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        add     ax, remap_tables_seg
        mov     es, ax
        mov     di, 0
        mov     al, 0
        mov     cx, 100h
@@same:
        stosb
        inc     al
        loop    @@same
        ret
@@nearest:
        mov     bx, table
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        mov     ax, remap_tables_seg
        add     ax, bx
        mov     es, ax
        mov     ax, p_palette
        mov     ds, ax
        mov     cs:best_index, 0
        mov     cs:best_dist, 10000
        mov     cx, 100h
        mov     si, 0
        mov     di, 0
@@n_candidate:
        lodsb
        sub     al, byte ptr cs:tint_r
        jns     @@n_r_abs
        neg     al
@@n_r_abs:
        add     si, 2
        cmp     al, 15h
        jns     @@n_next
        sub     si, 2
        mov     bl, al
        lodsb
        sub     al, byte ptr cs:tint_g
        jns     @@n_g_abs
        neg     al
@@n_g_abs:
        inc     si
        cmp     al, 15h
        jns     @@n_next
        dec     si
        mov     bh, al
        lodsb
        sub     al, byte ptr cs:tint_b
        jns     @@n_b_abs
        neg     al
@@n_b_abs:
        cmp     al, 15h
        jns     @@n_next
        mul     al
        mov     dx, ax
        mov     al, bl
        mul     al
        add     dx, ax
        mov     al, bh
        mul     al
        add     dx, ax
        cmp     dx, cs:best_dist
        jns     @@n_next
        mov     cs:best_dist, dx
        mov     cs:best_index, di
@@n_next:
        inc     di
        loop    @@n_candidate
        mov     di, 0
        mov     al, byte ptr cs:best_index
        mov     cx, 100h
        rep     stosb
        ret
Remap_Build_Table ENDP

; (pic, frame) copies the palette embedded in a picture into p_palette and marks those colors dirty. Header:
; +0Ah EMS handle (0 = picture in normal memory), +0Bh first EMS page, +0Ch offset in it, +0Eh offset of the palette
; block: +0 data offset, +2 first color, +4 count, +6 per-frame flag, +8 per-frame entries (offset, first byte, count
; byte; count 0 = none). EMS pictures: maps the first page to physical pages 0 AND 1 (the same logical page - no
; 'inc bx' as in PICRLE.ASM: BUG #386, latent) and reads through the page frame.
; C:  void Pic_Load_Palette(unsigned pic, int frame) { unsigned char far *p = MK_FP(pic, 0), far *pal;
; C:      unsigned base = 0, block, src, first, count;
; C:      if (p[0x0A]) { base = *(unsigned far *)(p + 0x0C); (EMS map p[0x0B] -> 0 and 1, handle p[0x0A]);
; C:          p = MK_FP(ems_frame_seg, 0); }
; C:      block = *(unsigned far *)(p + base + 0x0E) + base;
; C:      if (frame && p[block + 6]) { (entry at block + 8 + (frame - 1) * 4: src = word + base, first = low byte,
; C:          count = high byte; if (!count) return; }
; C:      else { src = word(block) + base; first = word(block + 2); count = word(block + 4); }
; C:      pal = MK_FP(p_palette, 0); _fmemcpy(pal + first * 3, p + src, count * 3); _fmemset(pal + 0x300 + first, 1, count); }
Pic_Load_Palette PROC USES si di es ds, pic:WORD, pframe:WORD
        mov     cx, pframe
        mov     ax, pic
        mov     dx, p_palette
        mov     ds, ax
        mov     es, dx
        mov     bl, byte ptr ds:[0Ah]
        cmp     bl, 0
        jne     @@ems
        mov     di, 0
@@header:
        mov     si, 0Eh
        add     si, di
        lodsw
        add     ax, di
        mov     si, ax
        mov     bx, ax
        cmp     cx, 0
        je      @@whole
        cmp     byte ptr [bx+6], 0
        je      @@whole
        dec     cx
        shl     cx, 1
        shl     cx, 1
        add     si, cx
        add     si, 8
        lodsw
        mov     dx, ax
        add     dx, di
        lodsw
        cmp     ah, 0
        je      @@none
        mov     cl, ah
        xor     ch, ch
        xor     ah, ah
        mov     di, ax
        mov     si, dx
        jmp     @@copy
@@none:
        ret
@@whole:
        lodsw
        mov     dx, ax
        add     dx, di
        lodsw
        mov     di, ax
        lodsw
        mov     cx, ax
        mov     si, dx
        jmp     @@copy
@@ems:
        mov     di, word ptr ds:[0Ch]
        xor     bh, bh
        mov     dx, bx
        push    dx
        mov     ax, 4400h
        mov     bl, byte ptr ds:[0Bh]
        xor     bh, bh
        int     67h
        pop     dx
        mov     bl, byte ptr ds:[0Bh]
        xor     bh, bh
        mov     ax, 4401h
        int     67h
        mov     ax, DGROUP
        mov     ds, ax
        mov     ax, ems_frame_seg
        mov     ds, ax
        jmp     @@header
@@copy:
        mov     bx, di
        shl     di, 1
        add     di, bx
        add     bx, 300h
@@color:
        movsw
        movsb
        mov     byte ptr es:[bx], 1
        inc     bx
        loop    @@color
        ret
Pic_Load_Palette ENDP

        END
