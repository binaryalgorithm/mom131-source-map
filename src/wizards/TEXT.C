/* TEXT.C - WIZARDS.EXE code segment 19b4 (MAGIC.EXE 1922, the same file): text output on top of the font engine
   (FONT.ASM): font styles (shadow / outline passes), color sets, print with left / right / centred / justified
   alignment, numbers, clipped printing, printing into a bitmap, the developer debug prints, and small string
   helpers. Control codes in the text: 1 normal colors, 2 / 4 toggle the highlight set, 3 toggle color set 2,
   0Dh / 14h new line, 19h / 1Dh NN set x (NN one binary byte; PARAGRPH.C's layout reads two ASCII digits and
   takes 19h as set y), 15h end.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 19b4 src/wizards/TEXT.C */
#include "ENUMS.H"
#pragma warn -pro

extern int clip_x1;                             /* DS:76da */
extern int clip_y1;                             /* DS:76dc */
extern int clip_x2;                             /* DS:76de */
extern int clip_y2;                             /* DS:76e0 */
extern unsigned draw_page_seg;                  /* DS:76ec */
extern char near_heap[];                        /* DS:d49a */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
unsigned font_color_block;                      /* DS:e828  MAGIC DS:a81e   */
unsigned palette_unread_block;                     /* DS:e826  MAGIC DS:a81c   */
unsigned mouse_pointer_pics;                    /* DS:e824  MAGIC DS:a81a   */
unsigned p_font_entry1;                         /* DS:e822  MAGIC DS:a818   */
                                                /* p_font_entry1: the font LBX's entry 1, loaded and never read */
unsigned p_font_block;                          /* DS:e820  MAGIC DS:a816   */
int text_cursor_x;                              /* DS:e81e  MAGIC DS:a814   */
int text_cursor_y;                              /* DS:e81c  MAGIC DS:a812   */
int font_current;                               /* DS:e81a  MAGIC DS:a810   */
int font_color1;                                /* DS:e818  MAGIC DS:a80e   */
int font_color2;                                /* DS:e816  MAGIC DS:a80c   */
int font_color3;                                /* DS:e814  MAGIC DS:a80a   */
char font_saved_colors[20];                     /* DS:e800  MAGIC DS:a7f6   */


void Far_Poke_Word(unsigned seg, unsigned off, unsigned value);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Text_Color_Normal(void);
void Text_Toggle_Highlight(void);
void Text_Toggle_Color2(void);
void Font_Use_Color_Set(int set);
void Debug_Print_Box(int x, int y, char *text);

int text_shadow_color = 0;                      /* DS:783c  shadow / outline color of the styled fonts */
int text_shadow_pass = 0;                       /* DS:783e  1 while the shadow passes are drawn */

/* (font, color1, color2, color3) Font_Select, then style 1..5 (font block +12h): 1 shadow down-right, 2 shadow
   up-left, 3 a wider down-right shadow, 4 outline, 5 outline + wide shadow.
   Simtex name Set_Font_Style_Shadow_Down (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Select_Shadow(int font, int c1, int c2, int c3)
{
    Font_Select(font, c1, c2, c3);
    Far_Poke_Byte(p_font_block, FONT_STYLE, 1);
}

/* (font, colors) Font_Select, style 2: shadow up-left.
   Simtex name Set_Font_Style_Shadow_Up (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Select_Shadow_UL(int font, int c1, int c2, int c3)
{
    Font_Select(font, c1, c2, c3);
    Far_Poke_Byte(p_font_block, FONT_STYLE, 2);
}

/* (font, colors) Font_Select, style 3: the wider down-right shadow.
   Simtex name Set_Font_Style_Shadow_Heavy (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Select_Shadow_Wide(int font, int c1, int c2, int c3)
{
    Font_Select(font, c1, c2, c3);
    Far_Poke_Byte(p_font_block, FONT_STYLE, 3);
}

/* (font, colors) Font_Select, style 4: outline.
   Simtex name Set_Font_Style_Outline (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Select_Outline(int font, int c1, int c2, int c3)
{
    Font_Select(font, c1, c2, c3);
    Far_Poke_Byte(p_font_block, FONT_STYLE, 4);
}

/* (font, colors) Font_Select, style 5: outline + wide shadow.
   Simtex name Set_Font_Style_Outline_Heavy (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Select_Outline_Shadow(int font, int c1, int c2, int c3)
{
    Font_Select(font, c1, c2, c3);
    Far_Poke_Byte(p_font_block, FONT_STYLE, 5);
}

/* (color) the color of the styled fonts' shadow / outline.
   Simtex name Set_Outline_Color (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Set_Text_Shadow_Color(int color)
{
    text_shadow_color = color;
}

/* (line_gap) line advance (+44h) = font height + extra.
   Simtex name Set_Font_LF (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Set_Line_Spacing(int line_gap)
{
    Far_Poke_Word(p_font_block, FONT_LINE_STEP, Far_Peek_Word(p_font_block, FONT_HEIGHT) + line_gap);
}

/* (width) the space glyph's width (the first entry of the font's glyph widths).
   Simtex name Set_Font_Spacing_Width (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Set_Space_Width(int width)
{
    Far_Poke_Byte(p_font_block, FONT_GLYPH_WIDTHS, width);
}

/* (gap) the extra pixels between characters (font block +48h) */
void Set_Text_Char_Gap(int gap)
{
    Far_Poke_Word(p_font_block, FONT_CHAR_GAP, gap);
}

/* (table[8]) the first 8 words of the font block from a char table */
void Font_Set_Shade_Table(char *table)
{
    int i;

    for (i = 0; i < 8; i++)
        Far_Poke_Word(p_font_block, i + i, table[i]);
}

/* (font, colors[16]) color slot 15 (font_color_block + F0h) = colors, then Font_Select(font, 15) */
void Font_Select_Palette(int font, char *colors)
{
    int base;
    int i;

    base = 0xf0;
    for (i = 0; i < 16; i++)
        Far_Poke_Byte(font_color_block, base + i, colors[i]);
    Font_Select(font, 15);
}

/* (color) color 0 of every slot and of the 4 font color sets = color.
   Simtex name Set_Alias_Color (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Set_Font_Color(int color)
{
    int base;
    int i;

    base = 0xf0;                                /* never used here (copied from Font_Select_Palette) */
    for (i = 0; i < 16; i++)
        Far_Poke_Byte(font_color_block, i * 16, color);
    Far_Poke_Byte(p_font_block, FONT_ACTIVE_COLORS, color);
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_1, color);
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_2, color);
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_3, color);
}

/* () saves color 0 of the 16 color slots and of the active / three saved color sets.
   Simtex name Save_Alias_Colors (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Save_Colors(void)
{
    int i;

    for (i = 0; i < 16; i++)
        font_saved_colors[i] = Far_Peek_Byte(font_color_block, i * 16);
    font_saved_colors[16] = Far_Peek_Byte(p_font_block, FONT_ACTIVE_COLORS);
    font_saved_colors[17] = Far_Peek_Byte(p_font_block, FONT_COLOR_SET_1);
    font_saved_colors[18] = Far_Peek_Byte(p_font_block, FONT_COLOR_SET_2);
    font_saved_colors[19] = Far_Peek_Byte(p_font_block, FONT_COLOR_SET_3);
}

/* () restores what Font_Save_Colors saved.
   Simtex name Restore_Alias_Colors (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Font_Restore_Colors(void)
{
    int i;

    for (i = 0; i < 16; i++)
        Far_Poke_Byte(font_color_block, i * 16, font_saved_colors[i]);
    Far_Poke_Byte(p_font_block, FONT_ACTIVE_COLORS, font_saved_colors[16]);
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_1, font_saved_colors[17]);
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_2, font_saved_colors[18]);
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_3, font_saved_colors[19]);
}

/* the far-string printers copy the text into near_heap first */
/* int with no return statement (as Print_Far_Text_Clipped): Print_Text's result is left in AX and callers use it.
   The same holds for Print_Text_Justified, Print_Text, Print_Text_Styled, Print_Text_Clipped and the
   Print_Text_To_Pic family: their value is the callee's AX */
int Print_Far_Text(int x, int y, char far *text)
{
    far_strcpy(near_heap, 0, text);
    Print_Text(x, y, near_heap);
}

/* prints a far string (through near_heap) with Print_Text_Centered */
void Print_Far_Text_Centered(int x, int y, char far *text)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Centered(x, y, near_heap);
}

/* prints a far string (through near_heap) with Print_Text_Right */
void Print_Far_Text_Right(int x, int y, char far *text)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Right(x, y, near_heap);
}

/* prints a far string (through near_heap) with Print_Text_Clipped */
int Print_Far_Text_Clipped(int x, int y, char far *text)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Clipped(x, y, near_heap);
}

/* prints a far string (through near_heap) with Print_Text_Clipped_Centered */
void Print_Far_Text_Clipped_Centered(int x, int y, char far *text)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Clipped_Centered(x, y, near_heap);
}

/* prints a far string (through near_heap) with Print_Text_Clipped_Right */
void Print_Far_Text_Clipped_Right(int x, int y, char far *text)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Clipped_Right(x, y, near_heap);
}

/* prints an int in decimal with Print_Text.
   Simtex name Print_Integer (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Number(int x, int y, int value)
{
    char text[10];
    int result;

    itoa(value, text, 10);
    result = Print_Text(x, y, text);
    return result;
}

/* prints a long in decimal with Print_Text */
int Print_Long(int x, int y, long value)
{
    char text[10];
    int result;

    ltoa(value, text, 10);
    result = Print_Text(x, y, text);
    return result;
}

/* prints the text: x is the right edge with Print_Text.
   Simtex name Print_Right (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Right(int x, int y, char *text)
{
    int width;
    int result;

    width = Text_Width(text) - 1;
    result = Print_Text(x - width, y, text);
    return result;
}

/* prints the text: x is the centre with Print_Text.
   Simtex name Print_Centered (MoO2 symbols, module MISC); clarified using ReMoM repo */
int Print_Text_Centered(int x, int y, char *text)
{
    int width;
    int result;

    width = Text_Width(text);
    result = Print_Text(x - width / 2, y, text);
    return result;
}

/* prints an int in decimal with Print_Text_Right.
   Simtex name Print_Integer_Right (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Number_Right(int x, int y, int value)
{
    char text[10];
    int result;

    itoa(value, text, 10);
    result = Print_Text_Right(x, y, text);
    return result;
}

/* prints an int in decimal with Print_Text_Centered.
   Simtex name Print_Integer_Centered (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Number_Centered(int x, int y, int value)
{
    char text[10];
    int result;

    itoa(value, text, 10);
    result = Print_Text_Centered(x, y, text);
    return result;
}

/* prints a long in decimal with Print_Text_Right */
int Print_Long_Right(int x, int y, long value)
{
    char text[10];
    int result;

    ltoa(value, text, 10);
    result = Print_Text_Right(x, y, text);
    return result;
}

/* prints an int in decimal with Print_Text_Clipped */
int Print_Number_Clipped(int x, int y, int value)
{
    char text[10];
    int result;

    itoa(value, text, 10);
    result = Print_Text_Clipped(x, y, text);
    return result;
}

/* prints a long in decimal with Print_Text_Clipped */
int Print_Long_Clipped(int x, int y, long value)
{
    char text[10];
    int result;

    ltoa(value, text, 10);
    result = Print_Text_Clipped(x, y, text);
    return result;
}

/* prints the text: x is the right edge with Print_Text_Clipped */
int Print_Text_Clipped_Right(int x, int y, char *text)
{
    int width;
    int result;

    width = Text_Width(text) - 1;
    result = Print_Text_Clipped(x - width, y, text);
    return result;
}

/* prints the text: x is the centre with Print_Text_Clipped */
int Print_Text_Clipped_Centered(int x, int y, char *text)
{
    int width;
    int result;

    width = Text_Width(text);
    result = Print_Text_Clipped(x - width / 2, y, text);
    return result;
}

/* prints an int in decimal with Print_Text_Clipped_Right */
int Print_Number_Clipped_Right(int x, int y, int value)
{
    char text[10];
    int result;

    itoa(value, text, 10);
    result = Print_Text_Clipped_Right(x, y, text);
    return result;
}

/* prints a long in decimal with Print_Text_Clipped_Right */
int Print_Long_Clipped_Right(int x, int y, long value)
{
    char text[10];
    int result;

    ltoa(value, text, 10);
    result = Print_Text_Clipped_Right(x, y, text);
    return result;
}

/* (x, y, text, width) justified to width pixels (spaces widened).
   Simtex name Print_Full (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Justified(int x, int y, char *text, int width)
{
    if (width < 0)
        width = 0;
    Print_Text_Styled(x, y, text, width);
}

/* (x, y, text) Print_Text_Styled without justification; returns the x after the text (no return
   statement: Print_Text_Styled's result is left in AX).
   Simtex name Print (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text(int x, int y, char *text)
{
    Print_Text_Styled(x, y, text, 0);
}

/* (x, y, text, justify) draws the font style's shadow / outline copies in text_shadow_color (color codes ignored),
   then the text itself; returns the x after the text.
   Simtex name Print_Display (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Styled(int x, int y, char *text, int justify)
{
    int style;
    int i;

    style = Far_Peek_Byte(p_font_block, FONT_STYLE);
    if (style != 0) {
        for (i = 0; i < 16; i++)
            Far_Poke_Byte(p_font_block, i, text_shadow_color);
        text_shadow_pass = 1;
        if (style != 2) {
            Print_Text_Core(x + 1, y + 1, text, 0, justify);
            Print_Text_Core(x, y + 1, text, 0, justify);
            Print_Text_Core(x + 1, y, text, 0, justify);
        }
        if (style != 1 && style != 3) {
            Print_Text_Core(x - 1, y, text, 0, justify);
            Print_Text_Core(x - 1, y - 1, text, 0, justify);
            Print_Text_Core(x, y - 1, text, 0, justify);
        }
        if (style == 3 || style == 5) {
            Print_Text_Core(x + 2, y + 2, text, 0, justify);
            Print_Text_Core(x + 1, y + 2, text, 0, justify);
            Print_Text_Core(x + 2, y + 1, text, 0, justify);
        }
        if (style > 3) {
            Print_Text_Core(x + 1, y - 1, text, 0, justify);
            Print_Text_Core(x - 1, y + 1, text, 0, justify);
        }
        if (style == 5) {
            Print_Text_Core(x + 2, y, text, 0, justify);
            Print_Text_Core(x, y + 2, text, 0, justify);
        }
        Font_Use_Color_Set(Far_Peek_Byte(p_font_block, FONT_COLOR_SET_INDEX));
    }
    text_shadow_pass = 0;
    Print_Text_Core(x, y, text, 1, justify);
}

/* (x, y, text, colors, justify) prints the text from (x, y), handling the control codes (color codes only when
   'colors'); justify > 0 = the width to fill by widening the spaces. Returns the x after the text.
   Simtex name Print_String (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Core(int x, int y, char *text, int colors, int justify)
{
    int i;
    int ch;
    int spaces;
    int widened;
    int leftover;
    int per_space;
    int j;

    i = 0;
    text_cursor_x = x;
    text_cursor_y = y;
    if (justify != 0) {
        widened = 0;
        leftover = 0;
        j = 0;
        spaces = 0;
        while (text[j] != 0 && text[j] != TEXT_NEWLINE && text[j] != TEXT_NEWLINE_2 && text[j] != TEXT_GOTO_Y && text[j] != TEXT_END &&
               text[j] != TEXT_GOTO_X) {
            if (text[j] == ' ')
                spaces++;
            j++;
        }
        if (spaces == 0)
            justify = 0;
        justify -= Text_Width(text);
        if (justify <= 0)
            justify = 0;
        else {
            leftover = justify % spaces;
            per_space = justify / spaces;
        }
    }
    while ((ch = text[i]) != 0) {
        switch (ch) {
        case 1:
            if (colors != 0)
                Text_Color_Normal();
            break;
        case 2:
            if (colors != 0)
                Text_Toggle_Highlight();
            break;
        case 3:
            if (colors != 0)
                Text_Toggle_Color2();
            break;
        case 4:
            if (colors != 0)
                Text_Toggle_Highlight();
            break;
        case TEXT_NEWLINE:
        case TEXT_NEWLINE_2:
            text_cursor_y += Far_Peek_Word(p_font_block, FONT_LINE_STEP);
            text_cursor_x = x;
            break;
        case TEXT_GOTO_Y:                       /* here 19h sets x too (PARAGRPH.C treats it as y) */
        case TEXT_GOTO_X:
            text_cursor_x = x + (unsigned char)text[i + 1];
            i++;
            break;
        case TEXT_END:
            return text_cursor_x;
        default:
            if (text_shadow_pass == 0)
                text_cursor_x = Font_Draw_Char(text_cursor_x, text_cursor_y, ch);
            else
                text_cursor_x = Font_Draw_Char_Alt(text_cursor_x, text_cursor_y, ch);
            if (justify != 0 && ch == ' ') {
                text_cursor_x += per_space;
                if (widened < leftover)
                    text_cursor_x++;
                widened++;
            }
            break;
        }
        i++;
    }
    return text_cursor_x;
}

/* () text_shadow_pass = 1 (the shadow copies are being drawn)
   found because ReMoM's numbering ran ahead of ours (10/17 audit); clarified using ReMoM repo.
   Simtex name No_Print_Alias (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Text_Shadow_Pass_On(void)
{
    text_shadow_pass = 1;
}

/* () the font's color set 0 (the text code 1).
   Simtex name Set_Normal_Colors_On (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Text_Color_Normal(void)
{
    Font_Use_Color_Set(FONT_SET_NORMAL);
}

/* () color set 1 <-> 0 (the text code 2).
   Simtex name Set_Highlight_Colors_On (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Text_Toggle_Highlight(void)
{
    Font_Use_Color_Set(Far_Peek_Byte(p_font_block, FONT_COLOR_SET_INDEX) == FONT_SET_HIGHLIGHT ? FONT_SET_NORMAL : FONT_SET_HIGHLIGHT);
}

/* () color set 2 <-> 0 (the text code 3).
   Simtex name Set_Special_Colors_On (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Text_Toggle_Color2(void)
{
    Font_Use_Color_Set(Far_Peek_Byte(p_font_block, FONT_COLOR_SET_INDEX) == FONT_SET_COLOR2 ? FONT_SET_NORMAL : FONT_SET_COLOR2);
}

/* (set) copies color set 'set' (font block +14h + 16 set) over the active colors (+0) */
void Font_Use_Color_Set(int set)
{
    int base;
    int i;

    base = set * 16 + 0x14;
    for (i = 0; i < 16; i += 2)
        Far_Poke_Word(p_font_block, i, Far_Peek_Word(p_font_block, i + base));
    Far_Poke_Byte(p_font_block, FONT_COLOR_SET_INDEX, set);
}

/* the debug prints draw on both pages in a boxed font 0 at (col x 10, row x 10) - not in the release version;
   Debug_Print_Box under them draws in any version */
int Debug_Print_Labeled_Number(int col, int row, int value, char *label)
{
    int column;
    int color;
    int font;
    char text[16];
    int len;

    if (Check_Release_Version() == 1)
        return 0;
    for (len = 0; label[len] != 0; len++)     /* the length is never used */
        ;
    font = font_current;
    color = font_color1;
    Font_Select(0, 0);
    Debug_Print_Box(col, row, label);
    column = (Text_Width(label) + 2) / 10;
    itoa(value, text, 10);
    Debug_Print_Box(col + column + 1, row, text);
    Font_Select(font, color);
}

/* (col, row, text) Debug_Print_Box, not in the release version */
int Debug_Print(int col, int row, char *text)
{
    if (Check_Release_Version() == 1)
        return 0;
    Debug_Print_Box(col, row, text);
}

/* (col, row, far text) the same for a far string (through near_heap).
   Simtex name PSTRS (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Debug_Print_Far(int col, int row, char far *text)
{
    if (Check_Release_Version() == 1)
        return 0;
    far_strcpy(near_heap, 0, text);
    Debug_Print_Box(col, row, near_heap);
}

/* (col, row, value) the same for an int */
int Debug_Print_Number(int col, int row, int value)
{
    char text[16];

    if (Check_Release_Version() == 1)
        return 0;
    itoa(value, text, 10);
    Debug_Print_Box(col, row, text);
}

/* (col, row, value) the same for a long. Simtex name PSTRL (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Debug_Print_Long(int col, int row, long value)
{
    char text[16];

    if (Check_Release_Version() == 1)
        return 0;
    ltoa(value, text, 10);
    Debug_Print_Box(col, row, text);
}

/* (col, row, value) the same for an unsigned (printed as a long).
   Simtex name PSTRU (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Debug_Print_Unsigned(int col, int row, unsigned value)
{
    long number;
    char text[16];

    if (Check_Release_Version() == 1)
        return 0;
    number = value;
    ultoa(number, text, 10);
    Debug_Print_Box(col, row, text);
}

/* (x = col, y = row, text; both made pixels, x 10, in place) a boxed debug line on the front page and the draw page, with the
   clip rectangle, font and draw page put back after.
   Simtex name Disp_PSTR (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Debug_Print_Box(int x, int y, char *text)
{
    int color;
    int font;
    int old_x1;
    int old_y1;
    int old_x2;
    int old_y2;
    unsigned page;

    page = draw_page_seg;
    x = x * 10;
    y = y * 10;
    old_x1 = clip_x1;
    old_y1 = clip_y1;
    old_x2 = clip_x2;
    old_y2 = clip_y2;
    Set_Clip_Rect(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y);
    font = font_current;
    color = font_color1;
    Font_Select(0, 0);
    Set_Draw_Page_Front();
    Fill_Rect_Clipped(x - 3, y - 3, x + Text_Width(text) + 4, y + Far_Peek_Byte(p_font_block, FONT_HEIGHT) + 5, 0);
    Print_Text_Clipped(x + 1, y + 1, text);
    Set_Draw_Page();
    Fill_Rect_Clipped(x - 3, y - 3, x + Text_Width(text) + 4, y + Far_Peek_Byte(p_font_block, FONT_HEIGHT) + 5, 0);
    Print_Text_Clipped(x + 1, y + 1, text);
    Set_Clip_Rect(old_x1, old_y1, old_x2, old_y2);
    Font_Select(font, color);
    draw_page_seg = page;
}

/* Print_Text_Styled through the clip rectangle (no justification) */
int Print_Text_Clipped(int x, int y, char *text)
{
    int style;
    int i;

    style = Far_Peek_Byte(p_font_block, FONT_STYLE);
    if (style != 0) {
        for (i = 0; i < 16; i++)
            Far_Poke_Byte(p_font_block, i, text_shadow_color);
        if (style != 2) {
            Print_String_Clipped(x + 1, y + 1, text, 0);
            Print_String_Clipped(x, y + 1, text, 0);
            Print_String_Clipped(x + 1, y, text, 0);
        }
        if (style != 1 && style != 3) {
            Print_String_Clipped(x - 1, y, text, 0);
            Print_String_Clipped(x - 1, y - 1, text, 0);
            Print_String_Clipped(x, y - 1, text, 0);
        }
        if (style == 3 || style == 5) {
            Print_String_Clipped(x + 2, y + 2, text, 0);
            Print_String_Clipped(x + 1, y + 2, text, 0);
            Print_String_Clipped(x + 2, y + 1, text, 0);
        }
        if (style > 3) {
            Print_String_Clipped(x + 1, y - 1, text, 0);
            Print_String_Clipped(x - 1, y + 1, text, 0);
        }
        if (style == 5) {
            Print_String_Clipped(x + 2, y, text, 0);
            Print_String_Clipped(x, y + 2, text, 0);
        }
        Font_Use_Color_Set(Far_Peek_Byte(p_font_block, FONT_COLOR_SET_INDEX));
    }
    Print_String_Clipped(x, y, text, 1);
}

/* (x, y, text, colors) one line clipped: stops at any line / position / end code; returns the x after it.
   Simtex name Clipped_Print_String (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_String_Clipped(int x, int y, char *text, int colors)
{
    int i;
    int height;
    int width;
    int ch;

    i = 0;
    text_cursor_x = x;
    text_cursor_y = y;
    height = Far_Peek_Word(p_font_block, FONT_HEIGHT);
    if (y > clip_y2)
        return 0;
    if (y + height <= clip_y1)
        return 0;
    if (x > clip_x2)
        return x;
    width = Text_Width(text);
    if (x + width < clip_x1)
        return x + width;
    while ((ch = text[i]) != 0) {
        switch (ch) {
        case 1:
            if (colors != 0)
                Text_Color_Normal();
            break;
        case 2:
            if (colors != 0)
                Text_Toggle_Highlight();
            break;
        case 3:
            if (colors != 0)
                Text_Toggle_Color2();
            break;
        case 4:
            if (colors != 0)
                Text_Toggle_Highlight();
            break;
        case TEXT_NEWLINE:
        case TEXT_NEWLINE_2:
        case TEXT_END:
        case TEXT_GOTO_Y:
        case TEXT_GOTO_X:
            return text_cursor_x;
        default:
            text_cursor_x = Print_Char_Clipped(text_cursor_x, text_cursor_y, ch);
            break;
        }
        i++;
    }
    return text_cursor_x;
}

/* (x, y, ch) one character through the clip rectangle; returns the x after it */
int Print_Char_Clipped(register int x, register int y, int ch)
{
    int result;
    int skip_x;
    int height;
    int width;
    int gap;
    int glyph;
    int skip_y;
    int rows;

    glyph = ch - 32;
    if (glyph < 0 || glyph > 94)                /* glyphs 0 .. 94: ' ' .. '~' */
        return x;
    asm push ds
    asm mov ax, p_font_block
    asm mov ds, ax
    asm mov ax, ds:[10h]
    asm mov height, ax
    asm mov ax, ds:[48h]
    asm mov gap, ax
    asm mov bx, glyph
    asm mov al, [bx+4Ah]
    asm xor ah, ah
    asm mov width, ax
    asm pop ds
    if (x >= clip_x1 && x + width <= clip_x2 && y >= clip_y1 && y + height <= clip_y2)
        result = Font_Draw_Char(x, y, ch);
    else {
        result = x + width + gap;
        if (x < clip_x1) {
            skip_x = clip_x1 - x;
            if (skip_x >= width)
                return result;
            x = clip_x1;
            width -= skip_x;
        } else
            skip_x = 0;
        if (x + width > clip_x2) {
            width = clip_x2 + 1 - x;
            if (width < 1)
                return result;
        }
        if (y < clip_y1) {
            skip_y = clip_y1 - y;
            y = clip_y1;
        } else
            skip_y = 0;
        rows = y + height > clip_y2 ? clip_y2 + 1 - y : height;
        Font_Draw_Char_Clip_Core(x, y, glyph, skip_x, width, skip_y, rows);
    }
    return result;
}

/* () -> the font selected last.
   Simtex name Get_Current_Font_Style (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Font_Get_Current(void)
{
    int result;

    result = font_current;
    return result;
}

/* () -> the first color passed to Font_Select */
int Font_Get_Color1(void)
{
    int result;

    result = font_color1;
    return result;
}

/* () -> the second. Simtex name Get_Current_Highlight_Color (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Font_Get_Color2(void)
{
    int result;

    result = font_color2;
    return result;
}

/* () -> the third. Simtex name Get_Current_Special_Color (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Font_Get_Color3(void)
{
    int result;

    result = font_color3;
    return result;
}

/* (offset, far text) copies the text into near_heap + offset */
void Text_Buffer_Load_Far(int offset, char far *text)
{
    far_strcpy(near_heap + offset, 0, text);
}

/* () line advance (+44h, low byte) - font height.
   Simtex name Get_Font_Vertical_Spacing (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Get_Text_Spacing(void)
{
    int result;

    result = Far_Peek_Byte(p_font_block, FONT_LINE_STEP) - Far_Peek_Word(p_font_block, FONT_HEIGHT);
    return result;
}

/* (text) removes the leading spaces and ONE trailing space (last_space is reset by every non-space, so
   only a space in the last position is cut).
   Simtex name Trim (MoO2 symbols, module strings); clarified using ReMoM repo
   Numbered: BUG #340 (BUGS_AND_QUIRKS.txt) */
void Str_Trim(char *text)
{
    int lead;
    int last_space;
    int i;

    for (lead = 0; text[lead] == ' '; lead++)
        ;
    strcpy(text, text + lead);
    for (i = 0, last_space = -1; text[i] != 0; i++)
        if (text[i] == ' ')
            last_space = i;
        else
            last_space = -1;
    if (last_space != -1)
        text[last_space] = 0;
}

/* (x, y, text, pic) Print_Text into a bitmap (no justification).
   Simtex name Print_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_To_Pic(int x, int y, char *text, unsigned pic)
{
    Print_Text_Justified_To_Pic(x, y, text, 0, pic);
}

/* Print_Text_Styled into a bitmap.
   Simtex name Print_Display_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Justified_To_Pic(int x, int y, char *text, int justify, unsigned pic)
{
    int style;
    int i;

    style = Far_Peek_Byte(p_font_block, FONT_STYLE);
    if (style != 0) {
        for (i = 0; i < 16; i++)
            Far_Poke_Byte(p_font_block, i, text_shadow_color);
        if (style != 2) {
            Print_Text_Core_To_Pic(x + 1, y + 1, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x, y + 1, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x + 1, y, text, 0, justify, pic);
        }
        if (style != 1 && style != 3) {
            Print_Text_Core_To_Pic(x - 1, y, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x - 1, y - 1, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x, y - 1, text, 0, justify, pic);
        }
        if (style == 3 || style == 5) {
            Print_Text_Core_To_Pic(x + 2, y + 2, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x + 1, y + 2, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x + 2, y + 1, text, 0, justify, pic);
        }
        if (style > 3) {
            Print_Text_Core_To_Pic(x + 1, y - 1, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x - 1, y + 1, text, 0, justify, pic);
        }
        if (style == 5) {
            Print_Text_Core_To_Pic(x + 2, y, text, 0, justify, pic);
            Print_Text_Core_To_Pic(x, y + 2, text, 0, justify, pic);
        }
        Font_Use_Color_Set(Far_Peek_Byte(p_font_block, FONT_COLOR_SET_INDEX));
    }
    Print_Text_Core_To_Pic(x, y, text, 1, justify, pic);
}

/* Print_Text_Core into a bitmap; here only 1Dh sets x (19h is printed as a character).
   Simtex name Print_String_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Core_To_Pic(int x, int y, char *text, int colors, int justify, unsigned pic)
{
    int i;
    int ch;
    int spaces;
    int widened;
    int leftover;
    int per_space;
    int j;

    i = 0;
    text_cursor_x = x;
    text_cursor_y = y;
    if (justify != 0) {
        widened = 0;
        leftover = 0;
        j = 0;
        spaces = 0;
        while (text[j] != 0 && text[j] != TEXT_NEWLINE && text[j] != TEXT_NEWLINE_2 && text[j] != TEXT_GOTO_Y && text[j] != TEXT_END &&
               text[j] != TEXT_GOTO_X) {
            if (text[j] == ' ')
                spaces++;
            j++;
        }
        if (spaces == 0)
            justify = 0;
        justify -= Text_Width(text);
        if (justify <= 0)
            justify = 0;
        else {
            leftover = justify % spaces;
            per_space = justify / spaces;
        }
    }
    while ((ch = text[i]) != 0) {
        switch (ch) {
        case 1:
            if (colors != 0)
                Text_Color_Normal();
            break;
        case 2:
            if (colors != 0)
                Text_Toggle_Highlight();
            break;
        case 3:
            if (colors != 0)
                Text_Toggle_Color2();
            break;
        case 4:
            if (colors != 0)
                Text_Toggle_Highlight();
            break;
        case TEXT_NEWLINE:
        case TEXT_NEWLINE_2:
            text_cursor_y += Far_Peek_Word(p_font_block, FONT_LINE_STEP);
            text_cursor_x = x;
            break;
        case TEXT_GOTO_X:
            text_cursor_x = x + (unsigned char)text[i + 1];
            i++;
            break;
        case TEXT_END:
            return text_cursor_x;
        default:
            text_cursor_x = Font_Draw_Char_To_Pic(text_cursor_x, text_cursor_y, ch, pic);
            if (justify != 0 && ch == ' ') {
                text_cursor_x += per_space;
                if (widened < leftover)
                    text_cursor_x++;
                widened++;
            }
            break;
        }
        i++;
    }
    return text_cursor_x;
}

/* prints the text: x is the right edge with Print_Text_To_Pic.
   Simtex name Print_Right_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Right_To_Pic(int x, int y, char *text, unsigned pic)
{
    int width;
    int result;

    width = Text_Width(text) - 1;
    result = Print_Text_To_Pic(x - width, y, text, pic);
    return result;
}

/* prints the text: x is the centre with Print_Text_To_Pic.
   Simtex name Print_Centered_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Text_Centered_To_Pic(int x, int y, char *text, unsigned pic)
{
    int width;
    int result;

    width = Text_Width(text);
    result = Print_Text_To_Pic(x - width / 2, y, text, pic);
    return result;
}

/* (x, y, text, justify, pic) the same, justified (a negative width counts as 0).
   Simtex name Print_Full_To_Bitmap (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Print_Justified_To_Pic_Clamped(int x, int y, char *text, int justify, unsigned pic)
{
    if (justify < 0)
        justify = 0;
    Print_Text_Justified_To_Pic(x, y, text, justify, pic);
}

/* prints a far string (through near_heap) with Print_Text_To_Pic */
void Print_Far_Text_To_Pic(int x, int y, char far *text, unsigned pic)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_To_Pic(x, y, near_heap, pic);
}

/* prints a far string (through near_heap) with Print_Text_Right_To_Pic */
void Print_Far_Text_Right_To_Pic(int x, int y, char far *text, unsigned pic)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Right_To_Pic(x, y, near_heap, pic);
}

/* prints a far string (through near_heap) with Print_Text_Centered_To_Pic */
void Print_Far_Text_Centered_To_Pic(int x, int y, char far *text, unsigned pic)
{
    far_strcpy(near_heap, 0, text);
    Print_Text_Centered_To_Pic(x, y, near_heap, pic);
}

/* prints a far string (through near_heap) with Print_Justified_To_Pic_Clamped */
void Print_Far_Text_Justified_To_Pic(int x, int y, char far *text, int justify, unsigned pic)
{
    far_strcpy(near_heap, 0, text);
    Print_Justified_To_Pic_Clamped(x, y, near_heap, justify, pic);
}
