/* GRAPHICS.C - WIZARDS.EXE code segment 1869 (MAGIC.EXE 17d6, the same file): engine start-up (EMS, video, font,
   sound, UI), the clip rectangle, clipped rectangles / pixels / lines (the line clipper with the dash-pattern phase),
   box outlines and the box-fill style dispatcher. The drawing itself is in the assembly module 18fc.
   This module's data (DS:76da): the clip rectangle 0,0 .. 319,199, then its literal "FONTS.LBX".
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1869 src/wizards/GRAPHICS.C */
#include <string.h>
#include "ENUMS.H"

int clip_x1 = 0;
int clip_y1 = 0;
int clip_x2 = SCREEN_MAX_X;
int clip_y2 = SCREEN_MAX_Y;

extern unsigned p_font_block;                   /* DS:e820 */
extern unsigned p_font_entry1;                  /* DS:e822  entry 1 of the font LBX, loaded and never read */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
unsigned palette_load_buf;                      /* DS:e7fe  MAGIC DS:a7f4   */
unsigned palette_entry_seg;                     /* DS:e7fc  MAGIC DS:a7f2   */
unsigned shade_table_block;                     /* DS:e7fa  MAGIC DS:a7f0   */
char font_lbx_name[16];                         /* DS:e7ea  MAGIC DS:a7e0   */
unsigned p_palette;                             /* DS:e7e8  MAGIC DS:a7de   */
unsigned palette_backup_seg;                    /* DS:e7e6  MAGIC DS:a7dc   */
unsigned remap_tables_seg;                      /* DS:e7e4  MAGIC DS:a7da   */
unsigned remap_color_defs;                      /* DS:e7e2  MAGIC DS:a7d8   */
unsigned palette_flags_seg;                     /* DS:e7e0  MAGIC DS:a7d6   */
unsigned anim_header_block;                     /* DS:e7de  MAGIC DS:a7d4   */

void Error_Hook_Init(void);
void EMS_Startup(void);
void VGA_Set_ModeY(void);
void UI_Init(int mode);
void Random_Seed_From_Timer(void);
void Set_Draw_Page(void);
unsigned LBX_Load(char *name, int entry);
unsigned Heap_Alloc_Block(unsigned paragraphs);
void Paragraph_Layout_Alloc(void);
void Far_Poke_Byte(unsigned seg, unsigned off, int value);
void Fill_Rect(int x1, int y1, int x2, int y2, int color);
void Put_Pixel(int x, int y, int color);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Draw_Line_Pattern(int x1, int y1, int x2, int y2, char *pattern, int length, int phase);
void Remap_Rect(int x1, int y1, int x2, int y2, int table);
void Gradient_Fill_Remap(int x1, int y1, int x2, int y2, int first, int n, int slope, int seed);
void Gradient_Fill_Remap2(int x1, int y1, int x2, int y2, int first, int n, int slope, int seed);
void Gradient_Fill_Noise(int x1, int y1, int x2, int y2, int color, int speck, int unused, int seed);
void Gradient_Fill_Shade(int x1, int y1, int x2, int y2, int base);
void Font_And_Palette_Init(char *lbx);
int Sound_Init();                              /* no prototype here: Engine_Init passes 11 words with input_mode -1 (2 never read), Engine_Init_Short 2 (QUIRK #407) */
void Draw_Line_Clip_Pattern(int x1, int y1, int x2, int y2, int patterned, int color, char *pattern, int length,
                            int phase);
/* Sound_Init is called WITHOUT a prototype: 11 arguments when input_mode == -1, 9 otherwise (2 in
   Engine_Init_Short) - the driver set-up reads what it needs */

/* (input_mode: UI_Init's 0 / 2 keyboard, 1 mouse; -1 defaults, sound mode, font LBX, music driver type / io / irq / dma, digital driver type / io / irq /
   dma) the engine start: hooks, EMS, Mode X, font + palette blocks, sound, UI, random seed, draw page.
   Simtex name Init_Drivers (MoO2 symbols, module init); clarified using ReMoM repo */
void Engine_Init(int input_mode, int sound_mode, char *lbx, int music_type, int music_io, int music_irq, int music_dma,
                 int digi_type, int digi_io, int digi_irq, int digi_dma)
{
    Error_Hook_Init();
    EMS_Startup();
    VGA_Set_ModeY();
    if (input_mode == -1) {
        Font_And_Palette_Init("FONTS.LBX");
        Sound_Init(0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        UI_Init(1);
    } else {
        Font_And_Palette_Init(lbx);
        Sound_Init(music_type, sound_mode, music_io, music_irq, music_dma, digi_type, digi_io, digi_irq, digi_dma);
        UI_Init(input_mode);
    }
    Random_Seed_From_Timer();
    Set_Draw_Page();
}

/* () the same without the video mode and the draw page, and only 2 sound arguments. No callers. */
void Engine_Init_Short(int input_mode, int music_type, int sound_mode, char *lbx)
{
    Error_Hook_Init();
    EMS_Startup();
    if (input_mode == -1) {
        Font_And_Palette_Init("FONTS.LBX");
        Sound_Init(2, 2);
        UI_Init(1);
    } else {
        Font_And_Palette_Init(lbx);
        Sound_Init(music_type, sound_mode);
        UI_Init(input_mode);
    }
    Random_Seed_From_Timer();
}

/* (lbx) the font LBX: entry 0 = the fonts, entry 1 kept; allocates the palette, flag, backup, remap, animation
   header and shade blocks; the palette starts black with every entry flagged for upload */
void Font_And_Palette_Init(char *lbx)
{
    int i;

    strcpy(font_lbx_name, lbx);
    p_font_block = LBX_Load(lbx, 0);
    p_font_entry1 = LBX_Load(lbx, 1);
    palette_load_buf = Heap_Alloc_Block(0x15c);
    p_palette = Heap_Alloc_Block(0x40);         /* 0x300 bytes of palette + 0x100 upload flags */
    palette_flags_seg = p_palette + 0x30;       /* the upload flags after the 0x300 palette bytes */
    palette_backup_seg = Heap_Alloc_Block(0x30);   /* a palette */
    remap_tables_seg = Heap_Alloc_Block(0x180);   /* NUM_REMAP_TABLES remap tables of 256 bytes */
    anim_header_block = Heap_Alloc_Block(2);
    shade_table_block = Heap_Alloc_Block(0x60);   /* 0x600 bytes */
    Paragraph_Layout_Alloc();
    for (i = 0; i < 0x300; i++)
        Far_Poke_Byte(p_palette, i, 0);
    for (i = 0; i < 0x100; i++)
        Far_Poke_Byte(palette_flags_seg, i, 1);
}

/* (x1, y1, x2, y2) the clip rectangle of the _Clipped drawing calls, kept on the 320 x 200 screen (the
   corners are put in order). Simtex name Set_Window (MoO2 symbols, module graphics); clarified using ReMoM repo */
void Set_Clip_Rect(int x1, int y1, int x2, int y2)
{
    int swap;

    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 > SCREEN_MAX_X)
        x2 = SCREEN_MAX_X;
    if (y2 > SCREEN_MAX_Y)
        y2 = SCREEN_MAX_Y;
    if (x1 > x2) {
        swap = x2;
        x2 = x1;
        x1 = swap;
    }
    if (y1 > y2) {
        swap = y2;
        y2 = y1;
        y1 = swap;
    }
    clip_x1 = x1;
    clip_x2 = x2;
    clip_y1 = y1;
    clip_y2 = y2;
}

/* () the clip rectangle = the whole screen.
   Simtex name Reset_Window (MoO2 symbols, module graphics); clarified using ReMoM repo */
void Reset_Clip_Rect(void)
{
    clip_x1 = 0;
    clip_x2 = SCREEN_MAX_X;
    clip_y1 = 0;
    clip_y2 = SCREEN_MAX_Y;
}

int Fill_Rect_Clipped(int x1, int y1, int x2, int y2, int color)
{
    if (x2 < clip_x1)
        return 0;
    if (x1 > clip_x2)
        return 0;
    if (y2 < clip_y1)
        return 0;
    if (y1 > clip_y2)
        return 0;
    if (x1 < clip_x1)
        x1 = clip_x1;
    if (y1 < clip_y1)
        y1 = clip_y1;
    if (x2 > clip_x2)
        x2 = clip_x2;
    if (y2 > clip_y2)
        y2 = clip_y2;
    Fill_Rect(x1, y1, x2, y2, color);          /* no return: AX of Fill_Rect is the result */
}

/* Simtex name Clipped_Dot (MoO2 symbols, module fields); clarified using ReMoM repo */
int Put_Pixel_Clipped(int x, int y, int color)
{
    if (x < clip_x1)
        return 0;
    if (x > clip_x2)
        return 0;
    if (y < clip_y1)
        return 0;
    if (y > clip_y2)
        return 0;
    Put_Pixel(x, y, color);                    /* no return: AX of Put_Pixel is the result */
}

/* (x1, y1, x2, y2, color) a solid line clipped to the clip rectangle */
void Draw_Line_Clipped(int x1, int y1, int x2, int y2, int color)
{
    Draw_Line_Clip_Pattern(x1, y1, x2, y2, 0, color, 0, 0, 0);
}

/* (x1, y1, x2, y2, pattern, length, phase) a clipped line drawn with the dash pattern (length entries),
   starting at pattern[phase] */
void Draw_Line_Dashed(int x1, int y1, int x2, int y2, char *pattern, int length, int phase)
{
    Draw_Line_Clip_Pattern(x1, y1, x2, y2, 1, 0, pattern, length, phase);
}

/* (x1, y1, x2, y2, patterned, color, pattern, length, phase) clips the line to the clip rectangle - vertical and
   horizontal lines directly, others by the parametric 32-bit formula clipping x first, then y - and draws it solid
   or with the dash pattern from phase (start + pixels cut at the left) % length. When the first stage (by x) swaps the ends
   the phase is mirrored (length - 1 - phase); the second stage's swap (by y, any line) does not mirror it, and
   pixels cut at the top or bottom do not advance the phase (cosmetic). */
void Draw_Line_Clip_Pattern(int x1, int y1, int x2, int y2, int patterned, int color, char *pattern, int length,
                            int phase)
{
    int outside;
    int swap;
    int t2;
    int skip;
    long ldx;
    long ldy;

    outside = 0;
    skip = 0;
    if (x1 == x2) {
        if (x1 < clip_x1 || x2 > clip_x2)
            outside = 1;
        else {
            if (y2 < y1) {
                t2 = y2;
                y2 = y1;
                y1 = t2;
                phase = length - 1 - phase;
            }
            if (y2 < clip_y1 || y1 > clip_y2)
                outside = 1;
            else {
                if (y1 < clip_y1)
                    y1 = clip_y1;
                if (y2 > clip_y2)
                    y2 = clip_y2;
            }
        }
    } else {
        if (x2 < x1) {
            swap = x2;
            x2 = x1;
            x1 = swap;
            t2 = y2;
            y2 = y1;
            y1 = t2;
            phase = length - 1 - phase;
        }
        ldy = (long)y2 - (long)y1;
        ldx = (long)x2 - (long)x1;
        if (x1 < clip_x1) {
            skip = clip_x1 - x1;
            y1 = ldy * (clip_x1 - x1) / ldx + y1;
            x1 = clip_x1;
        }
        if (x1 > x2)
            outside = 1;
        else {
            if (x2 > clip_x2) {
                y2 = ldy * (clip_x2 - x1) / ldx + y1;
                x2 = clip_x2;
            }
            if (x2 < x1)
                outside = 1;
        }
    }
    if (y1 == y2) {
        if (y1 < clip_y1 || y2 > clip_y2)
            outside = 1;
        else {
            if (x2 < x1) {
                swap = x2;
                x2 = x1;
                x1 = swap;
            }
            if (x2 < clip_x1 || x1 > clip_x2)
                outside = 1;
            else {
                if (x1 < clip_x1) {
                    skip = clip_x1 - x1;
                    x1 = clip_x1;
                }
                if (x2 > clip_x2)
                    x2 = clip_x2;
            }
        }
    } else {
        if (y2 < y1) {
            t2 = y2;
            y2 = y1;
            y1 = t2;
            swap = x2;
            x2 = x1;
            x1 = swap;
        }
        ldx = (long)x2 - (long)x1;
        ldy = (long)y2 - (long)y1;
        if (y1 < clip_y1) {
            x1 = ldx * (clip_y1 - y1) / ldy + x1;
            y1 = clip_y1;
        }
        if (y1 > y2)
            outside = 1;
        else {
            if (y2 > clip_y2) {
                x2 = ldx * (clip_y2 - y1) / ldy + x1;
                y2 = clip_y2;
            }
            if (y2 < y1)
                outside = 1;
        }
    }
    if (outside == 0) {
        if (patterned == 0)
            Draw_Line(x1, y1, x2, y2, color);
        else
            Draw_Line_Pattern(x1, y1, x2, y2, pattern, length, (phase + skip) % length);
    }
}

/* (x1, y1, x2, y2, color_tl, color_br) top and left edges in color_tl, bottom and right in color_br */
void Draw_Bevel_Box(int x1, int y1, int x2, int y2, int color_tl, int color_br)
{
    Draw_Line(x1, y1, x2, y1, color_tl);
    Draw_Line(x1, y1, x1, y2, color_tl);
    Draw_Line(x1 + 1, y2, x2, y2, color_br);
    Draw_Line(x2, y1 + 1, x2, y2, color_br);
}

/* (x, y, w, h, color) a w x h rectangle outline (unclipped) */
void Draw_Rect_Outline(int x, int y, int w, int h, int color)
{
    int x2;
    int y2;

    x2 = x + w - 1;
    y2 = y + h - 1;
    Draw_Line(x, y, x2, y, color);
    Draw_Line(x, y, x, y2, color);
    Draw_Line(x + 1, y2, x2, y2, color);
    Draw_Line(x2, y + 1, x2, y2, color);
}

/* (x, y, w, h, color) the same, clipped to the clip rectangle */
void Draw_Box_Clipped(int x, int y, int w, int h, int color)
{
    int x2;
    int y2;

    x2 = x + w - 1;
    y2 = y + h - 1;
    Draw_Line_Clipped(x, y, x2, y, color);
    Draw_Line_Clipped(x, y, x, y2, color);
    Draw_Line_Clipped(x + 1, y2, x2, y2, color);
    Draw_Line_Clipped(x2, y + 1, x2, y2, color);
}

/* (x1, y1, x2, y2, outer_tl, inner_tl, outer_br, inner_br) two nested bevel boxes */
void Draw_Double_Bevel_Box(int x1, int y1, int x2, int y2, int outer_tl, int inner_tl, int outer_br, int inner_br)
{
    Draw_Line(x1, y1, x2, y1, outer_tl);
    Draw_Line(x1, y1, x1, y2, outer_tl);
    Draw_Line(x1 + 1, y2, x2, y2, outer_br);
    Draw_Line(x2, y1 + 1, x2, y2, outer_br);
    x1++;
    y1++;
    x2--;
    y2--;
    Draw_Line(x1, y1, x2, y1, inner_tl);
    Draw_Line(x1, y1, x1, y2, inner_tl);
    Draw_Line(x1 + 1, y2, x2, y2, inner_br);
    Draw_Line(x2, y1 + 1, x2, y2, inner_br);
}

/* (x1, y1, x2, y2, style, a, b, c, d) box fill by style: 1 color a, 3 shadow remap (table 0), 15 remap table a,
   5 / 7 two gradient remaps, 13 noise, 16 shade from base a; other styles draw nothing. Callers pass 5, 6 or 9
   arguments (no prototype): only the styles that read a..d get them..
   Simtex name Gradient_Fill (MoO2 symbols, module graphics); clarified using ReMoM repo */
void Fill_Box_Styled(int x1, int y1, int x2, int y2, int style, int a, int b, int c, int d)
{
    switch (style) {
    case FILL_NONE:
        break;
    case FILL_SOLID:
        Fill_Rect(x1, y1, x2, y2, a);
        break;
    case FILL_SHADOW:
        Remap_Rect(x1, y1, x2, y2, 0);
        break;
    case FILL_REMAP:
        Remap_Rect(x1, y1, x2, y2, a);
        break;
    case FILL_GRADIENT_2:
        Gradient_Fill_Remap2(x1, y1, x2, y2, a, b, c, d);
        break;
    case FILL_GRADIENT:
        Gradient_Fill_Remap(x1, y1, x2, y2, a, b, c, d);
        break;
    case FILL_NOISE:
        Gradient_Fill_Noise(x1, y1, x2, y2, a, b, c, d);
        break;
    case FILL_SHADE:
        Gradient_Fill_Shade(x1, y1, x2, y2, a);
        break;
    }
}

/* (text) prints a '$'-terminated string through DOS (INT 21h AH=09h) */
void DOS_Print_Dollar(char *text)
{
    asm mov si, text                            /* loaded, never used (BCC saves SI because the asm names it) */
    asm mov dx, text
    asm mov ah, 9
    asm int 21h
}
