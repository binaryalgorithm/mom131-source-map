; FONT.ASM - WIZARDS.EXE code segment 1b42 (MAGIC.EXE 1aaf, the same file): the bitmap fonts - selecting a font and its
; colors (copied into the font block's color slots), drawing a character to the screen or into a picture, text
; width, and the clipped glyph blitter. Hand-written assembly (Borland TASM).
; Font block (p_font_block): +10h line gap?, +12h, +14h color slots (16 bytes each), +44h/+46h/+48h spacing words,
; +4Ah widths of chars 20h..7Dh, +AAh near offsets of the glyphs; at +16Ah/+17Ah/+18Ah/+19Ah/+49Ah the per-font
; tables Font_Select copies in. Glyph data: per column bytes cnt:4 | color:4 (a run), 80h+n = skip n rows, 80h = end.
; The module starts with 68h bytes of CODE-SEGMENT storage (1b42:0000): 12 words of state and an 80-byte column
; buffer (cs:18h) the clipped blitter decodes into.
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 1b42 src/wizards/FONT.ASM
        .MODEL  MEDIUM, C
        LOCALS
        EXTRN   draw_page_seg:WORD      ; DS:76ec
        EXTRN   font_color3:WORD        ; DS:e814  the third color of Font_Select: the special one (Simtex Get_Current_Special_Color)
        EXTRN   font_color2:WORD        ; DS:e816  the second: the highlight color (Simtex Get_Current_Highlight_Color)
        EXTRN   font_color1:WORD        ; DS:e818  the first: the normal color (ReMoM m_current_normal_color; clarified using ReMoM repo)
        EXTRN   font_current:WORD       ; DS:e81a
        EXTRN   p_font_block:WORD       ; DS:e820
        EXTRN   font_color_block:WORD   ; DS:e828
        .CODE
        PUBLIC  Font_Select, Font_Draw_Char, Font_Draw_Char_Alt, Font_Draw_Char_To_Pic, Font_Glyph_Blit
        PUBLIC  Font_Glyph_Blit_Alt, Text_Width, Font_Draw_Char_Clip_Core, Font_Glyph_Blit_Clipped
sel_color2      dw      0               ; 1b42:0000  Font_Select's color sets 2, 3 and the font
sel_color3      dw      0               ; 1b42:0002
sel_font        dw      0               ; 1b42:0004
char_x          dw      0               ; 1b42:0006  x of the character being drawn
pic_height      dw      0               ; 1b42:0008  Font_Draw_Char_To_Pic: the picture's height (column length)
next_x          dw      0               ; 1b42:000a  x after the character (the return value)
column_start    dw      0               ; 1b42:000c
columns_left    dw      0               ; 1b42:000e  glyph columns left to draw
clip_top        dw      0               ; 1b42:0010  Font_Glyph_Blit_Clipped: first and end row of the band
clip_bottom     dw      0               ; 1b42:0012
clip_rows       dw      0               ; 1b42:0014
screen_seg      dw      0               ; 1b42:0016
column_buf      db      80 dup (0)      ; 1b42:0018  one decoded glyph column (FFh = transparent)

; (font, color1, color2, color3) colors >= 16 count as 0; copies color set 1 into the font block's slots 0 and 1
; (+0, +14h), sets 2 and 3 after them, and the font's line gap, spacing, widths and glyph offsets into place
; C:  void Font_Select(int font, int c1, int c2, int c3) { (clamp; font_current = font; font_color1..3 = c1..c3;
; C:      _fmemcpy(font slots, color_block + c * 16, 16) ...; per-font tables -> +10h, +44h..+48h, +4Ah, +AAh) }
; Simtex name Set_Font_Style (MoO2 symbols, module fonts); clarified using ReMoM repo
Font_Select PROC USES es ds si di, font:WORD, c1:WORD, c2:WORD, c3:WORD
        LOCAL   unused2:WORD, unused4:WORD, col1:WORD, col2:WORD, col3:WORD
        mov     ax, c1
        cmp     ax, 10h
        jb      @@c1_ok
        mov     ax, 0
@@c1_ok:
        mov     col1, ax
        mov     ax, c2
        cmp     ax, 10h
        jb      @@c2_ok
        mov     ax, 0
@@c2_ok:
        mov     col2, ax
        mov     ax, c3
        cmp     ax, 10h
        jb      @@c3_ok
        mov     ax, 0
@@c3_ok:
        mov     col3, ax
        mov     ax, col2
        mov     bx, col3
        mov     cx, font
        mov     cs:sel_color2, ax
        mov     cs:sel_color3, bx
        mov     cs:sel_font, cx
        mov     ax, font
        mov     font_current, ax
        mov     ax, col1
        mov     font_color1, ax
        mov     ax, col2
        mov     font_color2, ax
        mov     ax, col3
        mov     font_color3, ax
        mov     ax, p_font_block
        mov     es, ax
        mov     di, 0
        mov     si, col1
        mov     ax, font_color_block
        mov     ds, ax
        and     si, 0Fh
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     dx, si
        mov     cx, 8
        rep     movsw
        mov     di, 14h
        mov     si, dx
        mov     cx, 8
        rep     movsw
        mov     si, cs:sel_color2
        and     si, 0Fh
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, 8
        rep     movsw
        mov     si, cs:sel_color3
        and     si, 0Fh
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, 8
        rep     movsw
        mov     ax, es
        mov     ds, ax
        mov     di, 10h
        mov     si, cs:sel_font
        shl     si, 1
        add     si, 16Ah
        movsw
        mov     di, 46h
        mov     si, cs:sel_font
        shl     si, 1
        add     si, 18Ah
        lodsw
        stosw
        mov     di, 44h
        add     ax, ds:[10h]
        stosw
        mov     di, 48h
        mov     si, cs:sel_font
        shl     si, 1
        add     si, 17Ah
        movsw
        mov     di, 12h
        mov     ax, 0
        stosw
        mov     di, 4Ah
        mov     si, cs:sel_font
        mov     ax, si
        shl     si, 1
        add     si, ax
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        add     si, 19Ah
        mov     cx, 30h
        rep     movsw
        mov     di, 0AAh
        mov     si, cs:sel_font
        mov     ax, si
        shl     si, 1
        add     si, ax
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        shl     si, 1
        add     si, 49Ah
        mov     cx, 60h
        rep     movsw
        ret
Font_Select ENDP

; (x, y, char) draws char (20h..7Dh, others draw nothing) at x, y -> the x after it (x + width + spacing)
; C:  int Font_Draw_Char(int x, int y, int ch) { if (ch < 0x20 || ch >= 0x7E) return x;
; C:      Font_Glyph_Blit(glyph[ch - 0x20], x, y); return x + font->spacing + font->width[ch - 0x20]; }
; Simtex name Print_Character (MoO2 symbols, module fonts); clarified using ReMoM repo
Font_Draw_Char PROC USES es ds si di, x:WORD, y:WORD, char:WORD
        LOCAL   unused:WORD
        mov     ax, x
        mov     cs:char_x, ax
        mov     bx, char
        xor     bh, bh
        cmp     bx, 20h
        js      @@none
        cmp     bx, 7Eh
        jns     @@none
        mov     ax, y
        mov     cx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, cx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, p_font_block
        mov     ds, ax
        sub     bx, 20h
        mov     dl, [bx+4Ah]
        xor     dh, dh
        mov     ax, ds:[48h]
        add     ax, dx
        add     ax, cs:char_x
        mov     cs:next_x, ax
        shl     bx, 1
        mov     si, [bx+0AAh]
        mov     bx, cs:char_x
        call    far ptr Font_Glyph_Blit
        mov     ax, cs:next_x
        ret
@@none:
        mov     ax, cs:char_x
        ret
Font_Draw_Char ENDP

; (x, y, char) the same with the transparent-color-0 blitter
; C:  as Font_Draw_Char with Font_Glyph_Blit_Alt (color slot 0 leaves the pixel)
; Simtex name Print_Character_No_Alias (MoO2 symbols, module fonts); clarified using ReMoM repo
Font_Draw_Char_Alt PROC USES es ds si di, ax_x:WORD, ax_y:WORD, ax_char:WORD
        LOCAL   unused:WORD
        mov     ax, ax_x
        mov     cs:char_x, ax
        mov     bx, ax_char
        xor     bh, bh
        cmp     bx, 20h
        js      @@none
        cmp     bx, 7Eh
        jns     @@none
        mov     ax, ax_y
        mov     cx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, cx
        add     ax, draw_page_seg
        mov     es, ax
        mov     ax, p_font_block
        mov     ds, ax
        sub     bx, 20h
        mov     dl, [bx+4Ah]
        xor     dh, dh
        mov     ax, ds:[48h]
        add     ax, dx
        add     ax, cs:char_x
        mov     cs:next_x, ax
        shl     bx, 1
        mov     si, [bx+0AAh]
        mov     bx, cs:char_x
        call    far ptr Font_Glyph_Blit_Alt
        mov     ax, cs:next_x
        ret
@@none:
        mov     ax, cs:char_x
        ret
Font_Draw_Char_Alt ENDP

; (x, y, char, pic_seg) draws char into a picture (column-major, height at pic_seg:2, pixels from pic_seg+1:0)
; -> the x after it; the column count comes from DX = the char's width as left by the width lookup
; C:  int Font_Draw_Char_To_Pic(int x, int y, int ch, unsigned pic) { (glyph runs written at pixel x * height + y) }
; Simtex name Print_Character_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo
Font_Draw_Char_To_Pic PROC USES es ds si di, px:WORD, py:WORD, pchar:WORD, pseg:WORD
        LOCAL   unused2:WORD, unused4:WORD, unused6:WORD
        mov     ax, px
        mov     cs:next_x, ax
        mov     ax, pseg
        mov     es, ax
        mov     ax, word ptr es:[2]
        mov     cs:pic_height, ax
        mov     bx, pchar
        xor     bh, bh
        cmp     bx, 20h
        js      @@none
        cmp     bx, 7Eh
        jns     @@none
        mov     ax, px
        mov     cx, cs:pic_height
        mul     cx
        add     ax, py
        mov     di, ax
        mov     ax, pseg
        inc     ax
        mov     es, ax
        mov     ax, px
        mov     cs:char_x, ax
        mov     ax, p_font_block
        mov     ds, ax
        sub     bx, 20h
        mov     dl, [bx+4Ah]
        xor     dh, dh
        mov     ax, ds:[48h]
        add     ax, dx
        add     ax, cs:char_x
        mov     cs:next_x, ax
        shl     bx, 1
        mov     si, [bx+0AAh]
@@column:
        mov     cs:column_start, di
@@byte:
        lodsb
        cmp     al, 0
        jns     @@run
        cmp     al, 80h
        je      @@column_done
        and     ax, 7Fh
        add     di, ax
        jmp     @@byte
@@run:
        mov     cl, al
        xor     ch, ch
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        mov     bl, al
        and     bx, 0Fh
        mov     al, [bx]
        rep     stosb
        jmp     @@byte
@@column_done:
        mov     di, cs:column_start
        add     di, cs:pic_height
        dec     dx
        jne     @@column
@@none:
        mov     ax, cs:next_x
        ret
Font_Draw_Char_To_Pic ENDP

; register call (DS:SI = glyph, ES = screen row segment, BX = x, DX = columns): draws the glyph column by column
; through the font block's color slots
; C:  (see Font_Draw_Char)
; Simtex name Print_Character_ASM (MoO2 symbols, module fontdraw); clarified using ReMoM repo
Font_Glyph_Blit PROC
        mov     cs:columns_left, dx
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     di, bx
        shr     di, 1
        shr     di, 1
        mov     ah, 1
        and     bx, 3
        je      @@plane0
        mov     cx, bx
@@plane:
        shl     ah, 1
        loop    @@plane
@@plane0:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
@@byte:
        lodsb
        cmp     al, 0
        js      @@skip
        mov     cl, al
        xor     ch, ch
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        mov     bl, al
        and     bx, 0Fh
        mov     al, [bx]
@@pixel:
        stosb
        add     di, 79
        loop    @@pixel
        jmp     @@byte
@@skip:
        and     al, 7Fh
        je      @@column_done
        mov     bl, al
        xor     bh, bh
        mov     cx, bx
        shl     bx, 1
        shl     bx, 1
        add     bx, cx
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        add     di, bx
        jmp     @@byte
@@column_done:
        mov     di, dx
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
        dec     cs:columns_left
        jne     @@byte
        ret
Font_Glyph_Blit ENDP

; register call as Font_Glyph_Blit, but color slot 0 is transparent (the rows are skipped)
; Simtex name Print_Character_No_Alias_ASM (MoO2 symbols, module fontdraw); clarified using ReMoM repo
Font_Glyph_Blit_Alt PROC
        mov     cs:columns_left, dx
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     di, bx
        shr     di, 1
        shr     di, 1
        mov     ah, 1
        and     bx, 3
        je      @@plane0
        mov     cx, bx
@@plane:
        shl     ah, 1
        loop    @@plane
@@plane0:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
@@byte:
        lodsb
        cmp     al, 0
        js      @@skip
        mov     cl, al
        xor     ch, ch
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        mov     bl, al
        and     bx, 0Fh
        je      @@clear
        mov     al, [bx]
@@pixel:
        stosb
        add     di, 79
        loop    @@pixel
        jmp     @@byte
@@clear:
        add     di, 80
        loop    @@pixel
        jmp     @@byte
@@skip:
        and     al, 7Fh
        je      @@column_done
        mov     bl, al
        xor     bh, bh
        mov     cx, bx
        shl     bx, 1
        shl     bx, 1
        add     bx, cx
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        add     di, bx
        jmp     @@byte
@@column_done:
        mov     di, dx
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
        dec     cs:columns_left
        jne     @@byte
        ret
Font_Glyph_Blit_Alt ENDP

; (text) width in pixels of text in the current font, up to the end or a control char 0Dh, 14h, 15h, 19h, 1Dh
; (other chars below 20h and above 7Dh are skipped): sum of (width + spacing) - spacing
; C:  int Text_Width(char *s) { int w = 0; for (; *s && !strchr("\r\x14\x15\x19\x1d", *s); s++)
; C:      if (*s >= 0x20 && *s < 0x7E) w += font->width[*s - 0x20] + font->spacing; return w - font->spacing; }
; Simtex name Get_String_Width (MoO2 symbols, module fonts); clarified using ReMoM repo
Text_Width PROC USES es ds si di, text:WORD
        mov     si, text
        mov     ax, p_font_block
        mov     es, ax
        mov     bx, 48h
        mov     cx, es:[bx]
        mov     dx, 0
        mov     bh, 0
        mov     ah, 0
@@char:
        lodsb
        sub     al, 20h
        js      @@control
        cmp     al, 5Eh
        jns     @@char
        mov     bl, al
        mov     al, es:[bx+4Ah]
        add     dx, ax
        add     dx, cx
        jmp     @@char
@@control:
        add     al, 20h
        je      @@end
        cmp     al, 0Dh
        je      @@end
        cmp     al, 14h
        je      @@end
        cmp     al, 15h
        je      @@end
        cmp     al, 19h
        je      @@end
        cmp     al, 1Dh
        je      @@end
        jmp     @@char
@@end:
        mov     ax, dx
        sub     ax, cx
        ret
Text_Width ENDP

; (x, y, char_index, skip_columns, columns, top, bottom) the clipped character: skips skip_columns glyph columns,
; then draws columns columns keeping only rows top .. bottom - 1
; C:  void Font_Draw_Char_Clip_Core(int x, int y, int ci, int skip, int cols, int top, int bottom) { ... }
; Simtex name Print_Clipped_Character (MoO2 symbols, module fonts); clarified using ReMoM repo
Font_Draw_Char_Clip_Core PROC USES es ds si di, cx_x:WORD, cx_y:WORD, cx_char:WORD, cx_skip:WORD, cx_cols:WORD, cx_top:WORD, cx_bottom:WORD
        push    ds
        mov     ax, cx_y
        mov     cx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, cx
        add     ax, draw_page_seg
        mov     es, ax
        mov     bx, cx_char
        mov     cx, cx_skip
        mov     ax, p_font_block
        mov     ds, ax
        shl     bx, 1
        mov     si, [bx+0AAh]
        cmp     cx, 0
        je      @@skipped
@@skip:
        lodsb
        cmp     al, 80h
        jne     @@skip
        loop    @@skip
@@skipped:
        pop     ds
        mov     dx, cx_cols
        mov     bx, cx_x
        mov     ax, cx_top
        mov     cx, cx_bottom
        mov     di, p_font_block
        mov     ds, di
        call    far ptr Font_Glyph_Blit_Clipped
        ret
Font_Draw_Char_Clip_Core ENDP

; register call (DS:SI = glyph, ES = screen row segment, BX = x, DX = columns, AX = top, CX = bottom): decodes each
; column into column_buf (FFh = transparent) and copies rows top .. bottom - 1 to the screen
; Simtex name Print_Clipped_Letter (MoO2 symbols, module fonts); clarified using ReMoM repo
Font_Glyph_Blit_Clipped PROC
        mov     cs:clip_top, ax
        mov     cs:clip_bottom, cx
        sub     cx, ax
        mov     cs:clip_rows, cx
        mov     cs:columns_left, dx
        mov     ax, es
        mov     cs:screen_seg, ax
        mov     dx, 03C4h
        mov     al, 2
        out     dx, al
        mov     di, bx
        shr     di, 1
        shr     di, 1
        mov     ah, 1
        and     bx, 3
        je      @@plane0
        mov     cx, bx
@@plane:
        shl     ah, 1
        loop    @@plane
@@plane0:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
@@column:
        mov     bx, cs
        mov     es, bx
        mov     di, offset column_buf
        mov     cx, cs:clip_bottom
        mov     al, 0FFh
        rep     stosb
        mov     di, offset column_buf
@@byte:
        lodsb
        cmp     al, 0
        js      @@skip
        mov     cl, al
        xor     ch, ch
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        mov     bl, al
        and     bx, 0Fh
        mov     al, [bx]
        rep     stosb
        jmp     @@byte
@@copy:
        push    ds
        push    si
        mov     bx, cs:screen_seg
        mov     es, bx
        mov     di, dx
        mov     bx, cs
        mov     ds, bx
        mov     si, offset column_buf
        add     si, cs:clip_top
        mov     cx, cs:clip_rows
@@row:
        lodsb
        inc     di
        cmp     al, 0FFh
        je      @@transparent
        dec     di
        stosb
@@transparent:
        add     di, 79
        loop    @@row
        mov     di, dx
        shl     ah, 1
        cmp     ah, 9
        js      @@same_byte
        mov     ah, 1
        inc     di
@@same_byte:
        mov     dx, 03C5h
        mov     al, ah
        out     dx, al
        mov     dx, di
        pop     si
        pop     ds
        dec     cs:columns_left
        jne     @@column
        ret
@@skip:
        and     al, 7Fh
        je      @@copy
        mov     cx, ax
        xor     ch, ch
        add     di, cx
        jmp     @@byte
Font_Glyph_Blit_Clipped ENDP

        END
