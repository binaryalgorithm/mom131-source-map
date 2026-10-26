/* PARAGRPH.C - WIZARDS.EXE code segment 1b9d (MAGIC.EXE 1b0a, the same file): word-wrapped paragraphs. Paragraph_Layout_Lines
   breaks a text into lines inside a box with the current font, flowing around up to 4 exclusion rectangles, into
   four far line tables (left x, right x, y, start offset); the print routines then draw each line left / right /
   centred / justified, on screen or into a picture. A line break WRITES code 15h over the space in the caller's
   string (Paragraph_Restore_Spaces puts the spaces back). Text codes: 0Dh / 14h new line, 19h NN set y, 1Dh NN set x
   (both read the FIRST digit twice: BUG #387; TEXT.C, which draws the line, reads one binary byte
   and sets x for both), 04h .. 04h marks a hotkey region (Paragraph_Add_Hotkey_Regions).
   Does NOT include GAME.H: the print calls pass a 4th / 5th argument (width, picture) GAME.H's prototypes lack.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1b9d src/wizards/PARAGRPH.C */
#include "ENUMS.H"
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     Print_Text_Centered, Print_Text_Clipped_Centered, Print_Text_Clipped_Right, Print_Text_Right (4; TEXT.C takes 3);
     Print_Text_Centered_To_Pic, Print_Text_Right_To_Pic (5; TEXT.C takes 4). */

extern unsigned p_font_block;                   /* DS:e820 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int para_excl_left[4];                          /* DS:e854  MAGIC DS:a84a   */
int para_excl_top[4];                           /* DS:e84c  MAGIC DS:a842   */
int para_excl_right[4];                         /* DS:e844  MAGIC DS:a83a   */
int para_excl_bottom[4];                        /* DS:e83c  MAGIC DS:a832   */
int text_line_count;                            /* DS:e83a  MAGIC DS:a830   */
int far *text_line_left;                        /* DS:e836  MAGIC DS:a82c   */
int far *text_line_right;                       /* DS:e832  MAGIC DS:a828   */
int far *text_line_y;                           /* DS:e82e  MAGIC DS:a824   */
int far *text_line_starts;                      /* DS:e82a  MAGIC DS:a820   */

int text_layout_allocated = 0;                  /* DS:7840  set by Paragraph_Layout_Alloc, never read */
int para_excl_count = 0;                        /* DS:7842 */

void far *Seg_To_Far_Ptr(unsigned seg);
int Paragraph_Layout_Lines(int x, int y, int width, char *text);
void Paragraph_Restore_Spaces(char *text);
int Print_Text_Centered();                     /* no prototype here: called with a 4th word the routine never reads */
int Print_Text_Centered_To_Pic();              /* no prototype here: called with a 5th word the routine never reads */
int Print_Text_Clipped_Centered();             /* no prototype here: called with a 4th word the routine never reads */
int Print_Text_Clipped_Right();                /* no prototype here: called with a 4th word the routine never reads */
int Print_Text_Right();                        /* no prototype here: called with a 4th word the routine never reads */
int Print_Text_Right_To_Pic();                 /* no prototype here: called with a 5th word the routine never reads */
int UI_Add_Value_Region();                    /* no prototype here: the control pointer and the hotkey string travel in int variables */

/* (x, y, width, text, align) lays the text out and prints each line: 0 left, 1 right, 2 centred, 3 justified (the
   last line left); then restores the spaces and clears the exclusions.
   Simtex name Print_Paragraph (MoO2 symbols, module BILL); clarified using ReMoM repo */
void Paragraph_Print(int x, int y, int width, char *text, unsigned align)
{
    int i;

    Paragraph_Layout_Lines(x, y, width, text);
    for (i = 0; i < text_line_count; i++) {
        switch (align) {
        case PARA_LEFT:
            Print_Text(text_line_left[i], text_line_y[i], text + text_line_starts[i]);
            break;
        case PARA_RIGHT:
            Print_Text_Right(text_line_right[i], text_line_y[i], text + text_line_starts[i], width);
            break;
        case PARA_CENTER:
            Print_Text_Centered((text_line_left[i] + text_line_right[i]) / 2, text_line_y[i],
                                text + text_line_starts[i], width);
            break;
        case PARA_JUSTIFY:
            if (text_line_count - 1 != i)
                Print_Text_Justified(text_line_left[i], text_line_y[i], text + text_line_starts[i],
                                     text_line_right[i] - text_line_left[i]);
            else
                Print_Text(text_line_left[i], text_line_y[i], text + text_line_starts[i]);
            break;
        }
    }
    Paragraph_Restore_Spaces(text);
    para_excl_count = 0;
}

/* (x, y, width, text, align) no callers: as Paragraph_Print with the clipped print routines, no justification
   found because ReMoM's numbering ran ahead of ours (10/17 audit); clarified using ReMoM repo */
void Paragraph_Print_Clipped(int x, int y, int width, char *text, int align)
{
    int i;

    Paragraph_Layout_Lines(x, y, width, text);
    for (i = 0; i < text_line_count; i++) {
        switch (align) {
        case PARA_LEFT:
            Print_Text_Clipped(text_line_left[i], text_line_y[i], text + text_line_starts[i]);
            break;
        case PARA_RIGHT:
            Print_Text_Clipped_Right(text_line_right[i], text_line_y[i], text + text_line_starts[i], width);
            break;
        case PARA_CENTER:
            Print_Text_Clipped_Centered((text_line_left[i] + text_line_right[i]) / 2, text_line_y[i],
                                        text + text_line_starts[i], width);
            break;
        }
    }
    Paragraph_Restore_Spaces(text);
    para_excl_count = 0;
}

/* (x, y, width, text, align, value_out, hotkey_ptr, help) no callers: lays the text out and registers every region
   marked by a pair of 04h codes with UI_Add_Value_Region (value_out: its int * written on a hit, hotkey_ptr its
   hotkey string, both passed as ints; help its help id), positioned as the
   aligned / justified line would print. QUIRK #388: the justified space count stops at INDEX 29 ('j != 0x1d', meant
   text[j] != 0x1d)
   Numbered: BUG #388 (BUGS_AND_QUIRKS.txt) */
void Paragraph_Add_Hotkey_Regions(int x, int y, int width, char *text, unsigned align, int value_out, int hotkey_ptr,
                              int help)
{
    int font_height;
    int char_gap;
    int cur_x;
    int pos;
    int slack;
    int ch;
    int control_count;
    int hotkey_index;
    int region_count;
    int in_region;
    int spaces_done;
    int extra_rem;
    int space_count;
    int extra_per_space;
    int line_width;
    unsigned char far *widths;
    int region_start[10];
    int region_end[10];
    register int j;
    register int i;

    widths = (unsigned char far *)((long)Seg_To_Far_Ptr(p_font_block) + FONT_GLYPH_WIDTHS);   /* a 32-bit add (adc) in the game */
    font_height = Far_Peek_Byte(p_font_block, FONT_HEIGHT);
    char_gap = Far_Peek_Byte(p_font_block, FONT_CHAR_GAP);
    control_count = UI_Get_Control_Count();
    hotkey_index = 1;
    Paragraph_Layout_Lines(x, y, width, text);
    slack = 0;
    for (i = 0; i < text_line_count; i++) {
        line_width = Text_Width(text + text_line_starts[i]);
        if (text_line_count - 1 == i && align == PARA_JUSTIFY)
            align = PARA_LEFT;
        switch (align) {
        case PARA_LEFT:
            cur_x = text_line_left[i];
            break;
        case PARA_RIGHT:
            cur_x = text_line_right[i] - line_width;
            break;
        case PARA_CENTER:
            cur_x = (text_line_left[i] + text_line_right[i]) / 2 - line_width / 2;
            break;
        case PARA_JUSTIFY:
            cur_x = text_line_left[i];
            slack = text_line_right[i] - cur_x;
            if (slack < 0)
                slack = 0;
            if (slack != 0) {
                spaces_done = 0;
                extra_rem = 0;
                j = text_line_starts[i];
                space_count = 0;
                for (; text[j] != 0 && text[j] != TEXT_NEWLINE && text[j] != TEXT_NEWLINE_2 && text[j] != TEXT_GOTO_Y && text[j] != TEXT_END && j != TEXT_GOTO_X;
                     j++)
                    if (text[j] == ' ')
                        space_count++;
                if (space_count == 0)
                    slack = 0;
                slack -= line_width;
                if (slack <= 0)
                    slack = 0;
                else {
                    extra_rem = slack % space_count;
                    extra_per_space = slack / space_count;
                }
            }
            break;
        }
        pos = text_line_starts[i];
        region_count = 0;
        in_region = 0;
        for (ch = text[pos]; ch != 0 && ch != TEXT_END && ch != TEXT_NEWLINE && ch != TEXT_NEWLINE_2 && ch != TEXT_GOTO_Y && ch != TEXT_GOTO_X; pos++, ch = text[pos]) {
            if (ch == 4) {
                if (in_region == 0) {
                    in_region = 1;
                    region_start[region_count] = cur_x;
                } else {
                    in_region = 0;
                    region_end[region_count] = cur_x;
                    region_count++;
                }
            }
            if (ch >= 32) {
                cur_x += widths[ch - 32] + char_gap;
                if (slack != 0 && ch == ' ') {
                    cur_x += extra_per_space;
                    if (spaces_done < extra_rem)
                        cur_x++;
                    spaces_done++;
                }
            }
        }
        for (j = 0; j < region_count; j++) {
            UI_Add_Value_Region(control_count, value_out, hotkey_index, region_start[j] - 1, text_line_y[i] - 1,
                                 region_end[j], text_line_y[i] + font_height, hotkey_ptr, help);
            hotkey_index++;
        }
    }
    Paragraph_Restore_Spaces(text);
}

/* (x, y, width, text) word-wraps text with the current font into the line tables; flows around the exclusion
   rectangles; a line that would reach row 200 aborts with 0 lines (returns 0). No return value otherwise (AX is
   whatever the last statement left). A word wider than the box never breaks: the layout then runs down to row 200
   and nothing is printed */
int Paragraph_Layout_Lines(int x, int y, int width, char *text)
{
    int pos;
    int font_height;
    int char_gap;
    int line_step;
    int stop_code;
    int line_x;
    int line_y;
    int line_end;
    int blocked;
    int last_space;
    int ch;
    int coord;
    int room;
    int next_x;
    unsigned char far *widths;
    register int i;
    register int k;

    pos = 0;
    text_line_count = 0;
    next_x = -1;
    widths = (unsigned char far *)((long)Seg_To_Far_Ptr(p_font_block) + FONT_GLYPH_WIDTHS);   /* a 32-bit add (adc) in the game */
    font_height = Far_Peek_Byte(p_font_block, FONT_HEIGHT);
    char_gap = Far_Peek_Byte(p_font_block, FONT_CHAR_GAP);
    line_step = Far_Peek_Byte(p_font_block, FONT_LINE_STEP);
    line_y = y;
    while (text[pos] != 0) {
        blocked = 0;
        if (next_x == -1) {
            line_x = x;
            line_end = x + width - 1;
        } else {
            line_x = next_x;
            line_end = x + width - 1;
            next_x = -1;
        }
        for (k = 0; k < para_excl_count && blocked == 0; k++) {
            if ((para_excl_top[k] <= line_y && para_excl_bottom[k] >= line_y) ||
                (para_excl_top[k] <= line_y + line_step && para_excl_bottom[k] >= line_y + font_height)) {
                if (para_excl_right[k] > line_x && para_excl_left[k] < line_end) {
                    if (para_excl_left[k] > line_x)
                        line_end = para_excl_left[k] - 1;
                    else
                        line_x = para_excl_right[k] + 1;
                }
                if (line_x >= line_end)
                    blocked = 1;
            }
        }
        if (blocked == 0) {
            if (line_y + font_height >= SCREEN_H) {
                text_line_count = 0;
                return 0;
            }
            room = line_end - line_x + 1;
            last_space = -1;
            stop_code = -1;
            for (i = pos; -char_gap <= room; i++) {
                ch = text[i];
                if (ch >= 32) {
                    room -= widths[ch - 32] + char_gap;
                    if (ch == ' ')
                        last_space = i;
                } else if (ch == TEXT_NEWLINE_2 || ch == TEXT_NEWLINE || ch == 0 || ch == TEXT_GOTO_Y || ch == TEXT_GOTO_X) {
                    room = -char_gap - 1;
                    stop_code = ch;
                }
            }
            if (stop_code == 0)
                i--;
            if (stop_code != -1) {
                text_line_left[text_line_count] = line_x;
                text_line_right[text_line_count] = line_end;
                text_line_y[text_line_count] = line_y;
                text_line_starts[text_line_count] = pos;
                text_line_count++;
                if (stop_code == TEXT_GOTO_Y) {
                    coord = (text[i] - '0') * 10;
                    coord += text[i] - '0';
                    line_y = coord - line_step;
                    i += 2;
                }
                if (stop_code == TEXT_GOTO_X) {
                    coord = (text[i] - '0') * 10;
                    coord += text[i] - '0';
                    line_y -= line_step;
                    next_x = coord;
                    i += 2;
                }
                pos = i;
            } else {
                text_line_left[text_line_count] = line_x;
                text_line_right[text_line_count] = line_end;
                text_line_y[text_line_count] = line_y;
                text_line_starts[text_line_count] = pos;
                text_line_count++;
                if (last_space != -1) {
                    text[last_space] = TEXT_END;
                    pos = last_space + 1;
                }
            }
        }
        if (x + width - 1 > line_end)           /* cut short by an exclusion box: go on along the same row past it;
                                                   else the next row */
            next_x = line_end + 1;
        else
            line_y += line_step;
    }
}

/* (width, text) the height the paragraph needs: the last line's y + the font height (0 for no lines).
   Simtex name Get_Paragraph_Max_Height (MoO2 symbols, module BILL); clarified using ReMoM repo */
int Paragraph_Height(int width, char *text)
{
    int font_height;
    int result;

    font_height = Far_Peek_Byte(p_font_block, FONT_HEIGHT);
    Paragraph_Layout_Lines(0, 0, width, text);
    if (text_line_count < 1)
        return 0;
    result = text_line_y[text_line_count - 1] + font_height;
    Paragraph_Restore_Spaces(text);
    return result;
}

/* (width, text, align 0-3) the width the laid-out paragraph really covers: rightmost - leftmost pixel + 1.
   Simtex name Get_Paragraph_Max_Width (MoO2 symbols, module BILL); clarified using ReMoM repo */
int Paragraph_Used_Width(int width, char *text, unsigned align)
{
    int result;
    int right;
    int mid;
    int w;
    int min_x;
    int max_x;
    register int i;
    register int left;

    Paragraph_Layout_Lines(0, 0, width, text);
    min_x = 1000;                               /* wider than any screen */
    max_x = 0;
    for (i = 0; i < text_line_count; i++) {
        w = Text_Width(text + text_line_starts[i]) - 1;
        switch (align) {
        case PARA_LEFT:
            left = text_line_left[i];
            right = left + w;
            break;
        case PARA_RIGHT:
            right = text_line_right[i];
            left = right - w;
            break;
        case PARA_CENTER:
            mid = (text_line_right[i] + text_line_left[i]) / 2;
            left = mid - w / 2;
            right = mid + w / 2;
            break;
        case PARA_JUSTIFY:
            left = text_line_left[i];
            right = text_line_right[i];
            break;
        }
        if (left < min_x)
            min_x = left;
        if (right > max_x)
            max_x = right;
    }
    Paragraph_Restore_Spaces(text);
    result = max_x - min_x + 1;
    return result;
}

/* () drops the exclusion rectangles.
   Simtex name Reset_Paragraph_Exclusions (MoO2 symbols, module paragrph); clarified using ReMoM repo */
void Paragraph_Clear_Exclusions(void)
{
    para_excl_count = 0;
}

/* (left, top, right, bottom) a rectangle the next paragraph flows around (max 4).
   Simtex name Set_Paragraph_Exclusion_Area (MoO2 symbols, module paragrph); clarified using ReMoM repo */
void Paragraph_Add_Exclusion(int left, int top, int right, int bottom)
{
    if (para_excl_count < 4) {
        para_excl_left[para_excl_count] = left;
        para_excl_top[para_excl_count] = top;
        para_excl_right[para_excl_count] = right;
        para_excl_bottom[para_excl_count] = bottom;
        para_excl_count++;
    }
}

/* (x, y, pic) the picture's rectangle at (x, y) as an exclusion */
void Paragraph_Exclude_Pic(int x, int y, unsigned pic)
{
    int w;
    int h;

    w = Far_Peek_Word(pic, 0);                  /* PIC_WIDTH, PIC_HEIGHT (STRUCTS.H, not included here) */
    h = Far_Peek_Word(pic, 2);
    if (para_excl_count < 4) {
        para_excl_left[para_excl_count] = x;
        para_excl_top[para_excl_count] = y;
        para_excl_right[para_excl_count] = x + w - 1;
        para_excl_bottom[para_excl_count] = y + h - 1;
        para_excl_count++;
    }
}

/* (text) the 15h break marks back to spaces */
void Paragraph_Restore_Spaces(char *text)
{
    int i;

    for (i = 0; text[i] != 0; i++)
        if (text[i] == TEXT_END)
            text[i] = ' ';
}

/* () the current font's height (font +10h).
   Simtex name Get_Font_Height (MoO2 symbols, module fonts); clarified using ReMoM repo */
int Font_Get_Height(void)
{
    int h;

    h = Far_Peek_Word(p_font_block, FONT_HEIGHT);
    return h;
}

/* () the widest glyph of the current font + the character spacing */
int Font_Max_Width(void)
{
    int w;
    int max;
    int i;

    max = 0;
    for (i = 0; i < 96; i++) {                  /* the table's 96 widths; Print_Char_Clipped draws 95 (' ' .. '~') */
        w = Far_Peek_Byte(p_font_block, i + FONT_GLYPH_WIDTHS);
        if (w > max)
            max = w;
    }
    max += Far_Peek_Byte(p_font_block, FONT_CHAR_GAP);
    return max;
}

/* (x, y, width, text, align, pic) Paragraph_Print into a picture.
   Simtex name Print_Paragraph_To_Bitmap (MoO2 symbols, module BILL); clarified using ReMoM repo
   Numbered: BUG #341 (BUGS_AND_QUIRKS.txt) */
void Paragraph_Print_To_Pic(int x, int y, int width, char *text, unsigned align, unsigned pic)
{
    int i;

    Paragraph_Layout_Lines(x, y, width, text);
    for (i = 0; i < text_line_count; i++) {
        switch (align) {
        case PARA_LEFT:
            Print_Text_To_Pic(text_line_left[i], text_line_y[i], text + text_line_starts[i], pic);
            break;
        case PARA_RIGHT:
            Print_Text_Right_To_Pic(text_line_right[i], text_line_y[i], text + text_line_starts[i], width, pic);
            break;
        case PARA_CENTER:
            Print_Text_Centered_To_Pic((text_line_left[i] + text_line_right[i]) / 2, text_line_y[i],
                                       text + text_line_starts[i], width, pic);
            break;
        case PARA_JUSTIFY:
            if (text_line_count - 1 != i)
                Print_Justified_To_Pic_Clamped(text_line_left[i], text_line_y[i], text + text_line_starts[i],
                                                    text_line_right[i] - text_line_left[i], pic);
            else
                Print_Text_To_Pic(text_line_left[i], text_line_y[i], text + text_line_starts[i], pic);
            break;
        }
    }
    Paragraph_Restore_Spaces(text);
    para_excl_count = 0;
}

/* () allocates the four line tables (8 x 16-byte blocks = 64 ints each) */
void Paragraph_Layout_Alloc(void)
{
    unsigned seg;

    text_layout_allocated = 1;
    seg = Heap_Alloc_Block(8);
    text_line_left = (int far *)Seg_To_Far_Ptr(seg);
    seg = Heap_Alloc_Block(8);
    text_line_right = (int far *)Seg_To_Far_Ptr(seg);
    seg = Heap_Alloc_Block(8);
    text_line_y = (int far *)Seg_To_Far_Ptr(seg);
    seg = Heap_Alloc_Block(8);
    text_line_starts = (int far *)Seg_To_Far_Ptr(seg);
}
