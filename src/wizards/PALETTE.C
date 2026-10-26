/* PALETTE.C - WIZARDS.EXE code segment 1c79 (MAGIC.EXE 1be7, the same file): the VGA palette in memory - loading a
   palette entry of the font LBX, the update flags Palette_Upload uses, remap tables, save / restore, and the
   single-entry color cycling that sets the DAC directly through the BIOS (INT 10h AX=1010h).
   The block-copy parts are inline assembly in the C source (as in LINEPATH.C).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1c79 src/wizards/PALETTE.C */
#include "GAME.H"

/* palette_cycle_state: Palette_Cycle_Color's direction - toward the 'to' color, or back toward 'from'
   (whether the value rises or falls depends on which end is larger) */
#define PAL_CYCLE_IDLE (-1)
#define PAL_CYCLE_TO_END 0
#define PAL_CYCLE_TO_START 1

extern unsigned palette_flags_seg;              /* DS:e7e0 */
extern unsigned remap_color_defs;               /* DS:e7e2 */
extern unsigned palette_backup_seg;             /* DS:e7e6  0x300-byte copy of the palette (Palette_Save / _Restore) */
extern unsigned p_palette;                      /* DS:e7e8 */
extern char font_lbx_name[];                    /* DS:e7ea  the font LBX's name, kept by Font_And_Palette_Init */
extern unsigned palette_entry_seg;              /* DS:e7fc  the palette entry last loaded (palette, then font colors,
                                                   mouse pointers, remap definitions) */
extern unsigned palette_load_buf;               /* DS:e7fe  load buffer of Palette_Load (0x15c paragraphs) */
extern unsigned mouse_pointer_pics;             /* DS:e824  entry + 0x500: the 16 x 16 mouse pointers, 0x10 paragraphs each */
extern unsigned palette_unread_block;              /* DS:e826  the 256 bytes at palette entry offset 0x400, between the font colors
                                                   and the mouse pointers; nothing reads them */
extern unsigned font_color_block;               /* DS:e828 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int palette_cycle_value;                        /* DS:e85e  MAGIC DS:a854   */
int palette_cycle_speed;                        /* DS:e85c  MAGIC DS:a852   */

int palette_cycle_state = PAL_CYCLE_IDLE;                   /* DS:7844  the module's only data (link test, batch 36) */

void Font_Select();                             /* no prototype here: this module calls it with 2 arguments (FONT.ASM takes 4) */
void Remap_Build_Table(int table, int r, int g, int b, int percent);
unsigned Far_Peek_Byte(unsigned seg, unsigned off);
void Far_Poke_Byte(unsigned seg, unsigned off, int value);
void Swap_Words(int *a, int *b);
void Palette_Cycle_Step(int *from, int *to);

/* (entry, first, last) loads palette entry + 2 of the fonts LBX (FONTS.LBX 0 is the font data, 1 a 652-byte
   block, 2..8 the 5472-byte palette entries: the 768-byte palette, then the font colors,
   an unread block, the mouse pointers and the remap definitions - their segments are kept), copies colors
   first .. last (all 256 for first -1) into the working palette, flagging each changed color (the flag
   array 0x300 bytes after the palette), selects font 0 and marks the range for the next upload.
   Simtex name Load_Palette (MoO2 symbols, module fonts); clarified using ReMoM repo */
void Palette_Load(int entry, int first, int last)
{
    int count;
    int start;

    palette_entry_seg = LBX_Reload(font_lbx_name, entry + 2, palette_load_buf);
    font_color_block = palette_entry_seg + 0x30;
    palette_unread_block = font_color_block + 0x10;
    mouse_pointer_pics = palette_unread_block + 0x10;
    remap_color_defs = mouse_pointer_pics + 0x100;
    if (first == -1) {
        start = 0;
        count = 0x100;
    } else {
        start = first;
        count = last - first + 1;
    }
    asm push si
    asm push di
    asm push es
    asm push ds
    asm mov di, start
    asm mov bx, di
    asm add bx, 0x300                           /* the upload flags: palette_flags_seg's bytes, after the 300h palette */
    asm mov ax, di
    asm shl di, 1
    asm add di, ax
    asm mov si, di
    asm mov cx, count
    asm mov ax, palette_entry_seg
    asm mov dx, p_palette
    asm mov ds, ax
    asm mov es, dx
next_color:
    asm lodsb
    asm cmp al, es:[di]
    asm je same_r
    asm mov byte ptr es:[bx], 1
same_r:
    asm stosb
    asm lodsb
    asm cmp al, es:[di]
    asm je same_g
    asm mov byte ptr es:[bx], 1
same_g:
    asm stosb
    asm lodsb
    asm cmp al, es:[di]
    asm je same_b
    asm mov byte ptr es:[bx], 1
same_b:
    asm stosb
    asm inc bx
    asm loop next_color
    asm pop ds
    asm pop es
    asm pop di
    asm pop si
    Font_Select(0, 0);
    if (first == -1)
        Palette_Mark_Range(0, 0xff);
    else
        Palette_Mark_Range(first, last);
}

/* () remap table 0: every color darkened 50 % toward black (the shadows) */
void Remap_Build_Shadow(void)
{
    Remap_Build_Table(0, 0, 0, 0, 50);
}

/* () remap table 0 (the shadow) and tables 1 .. 23 from the palette entry's definitions (r, g, b, percent per
   table, 4 bytes each) */
void Remap_Build_All(void)
{
    int table;
    int off;

    Remap_Build_Table(0, 0, 0, 0, 50);
    for (table = 1; table < NUM_REMAP_TABLES; table++) {
        off = table * 4;
        Remap_Build_Table(table, Far_Peek_Byte(remap_color_defs, off), Far_Peek_Byte(remap_color_defs, off + 1),
                          Far_Peek_Byte(remap_color_defs, off + 2), Far_Peek_Byte(remap_color_defs, off + 3));
    }
}

/* (first, last) the same for tables first .. last only (either order; table 0 is the shadow) */
void Remap_Build_Range(int first, int last)
{
    int table;
    int off;

    if (last < first)
        Swap_Words(&first, &last);
    if (first == 0) {
        Remap_Build_Table(0, 0, 0, 0, 50);
        first = 1;
    }
    for (table = first; table <= last; table++) {
        off = table * 4;
        Remap_Build_Table(table, Far_Peek_Byte(remap_color_defs, off), Far_Peek_Byte(remap_color_defs, off + 1),
                          Far_Peek_Byte(remap_color_defs, off + 2), Far_Peek_Byte(remap_color_defs, off + 3));
    }
}

/* (first, last) flags colors first .. last for upload at the next frame.
   Simtex name Set_Palette_Changes (MoO2 symbols, module palette); clarified using ReMoM repo */
void Palette_Mark_Range(int first, int last)
{
    while (first <= last) {
        Far_Poke_Byte(palette_flags_seg, first, 1);
        first++;
    }
}

/* (first, last) clears those upload flags */
void Palette_Unmark_Range(int first, int last)
{
    while (first <= last) {
        Far_Poke_Byte(palette_flags_seg, first, 0);
        first++;
    }
}

/* (first, last) sets colors first .. last of the working palette to black (not flagged for upload).
   Simtex name Clear_Palette_Changes (MoO2 symbols, module palette); clarified using ReMoM repo */
void Palette_Clear_Range(int first, int last)
{
    int count;

    count = last - first + 1;
    asm push es
    asm push di
    asm mov cx, count
    asm mov ax, cx
    asm shl cx, 1
    asm add cx, ax
    asm mov di, first
    asm mov ax, di
    asm shl di, 1
    asm add di, ax
    asm mov ax, p_palette
    asm mov es, ax
    asm mov al, 0
    asm rep stosb
    asm pop di
    asm pop es
}

/* () copies the working palette (768 bytes) to the backup */
void Palette_Save(void)
{
    Mem_Copy_Far(0, palette_backup_seg, 0, p_palette, 0x300);
}

/* () copies the backup back and flags every color for upload */
void Palette_Restore(void)
{
    Mem_Copy_Far(0, p_palette, 0, palette_backup_seg, 0x300);
    Palette_Mark_Range(0, 0xff);
}

/* (first, count, rgb) copies count colors (3 bytes each, near data) into the working palette and flags them */
void Palette_Set_Range(int first, int count, char *rgb)
{
    unsigned data_seg;

    asm mov ax, ds
    asm mov data_seg, ax
    Mem_Copy_Far(first * 3, p_palette, rgb, data_seg, count * 3);
    Palette_Mark_Range(first, first + count - 1);
}

/* () restarts the color cycling (the next Palette_Cycle_Color starts again from its first color).
   Simtex name Reset_Cycle_Palette_Color (MoO2 symbols, module palette); clarified using ReMoM repo */
void Palette_Cycle_Reset(void)
{
    palette_cycle_state = PAL_CYCLE_IDLE;
}

/* (pal_index, r1, g1, b1, r2, g2, b2, speed) one step of a color cycling between two colors: the channel with the
   largest difference moves by speed per call (Palette_Cycle_Step bounces it between its ends), the other two
   follow proportionally; the color is set at once through the BIOS (INT 10h, AX 1010h), not the working
   palette. Simtex name Cycle_Palette_Color (MoO2 symbols, module palette); clarified using ReMoM repo */
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed)
{
    int db;
    int channel;
    int red;
    int blue;
    int green;
    int dr;
    int dg;

    dr = r2 - r1;
    dg = g2 - g1;
    db = b2 - b1;
    if (dr < 0)
        dr = -dr;
    if (dg < 0)
        dg = -dg;
    if (db < 0)
        db = -db;
    palette_cycle_speed = speed;
    channel = 2;
    if (dr >= dg && dr >= db)
        channel = 0;
    else if (dg >= dr && dg >= db)
        channel = 1;
    if (palette_cycle_state == PAL_CYCLE_IDLE) {
        palette_cycle_state = PAL_CYCLE_TO_END;
        switch (channel) {
        case 0:
            palette_cycle_value = r1;
            break;
        case 1:
            palette_cycle_value = g1;
            break;
        case 2:
            palette_cycle_value = b1;
            break;
        }
    }
    switch (channel) {
    case 0:
        red = palette_cycle_value;
        green = (g2 - g1) * (palette_cycle_value - r1) / (r2 - r1) + g1;
        blue = (b2 - b1) * (palette_cycle_value - r1) / (r2 - r1) + b1;
        break;
    case 1:
        green = palette_cycle_value;
        red = (r2 - r1) * (palette_cycle_value - g1) / (g2 - g1) + r1;
        blue = (b2 - b1) * (palette_cycle_value - g1) / (g2 - g1) + b1;
        break;
    case 2:
        blue = palette_cycle_value;
        green = (g2 - g1) * (palette_cycle_value - b1) / (b2 - b1) + g1;
        red = (r2 - r1) * (palette_cycle_value - b1) / (b2 - b1) + r1;
        break;
    }
    switch (channel) {
    case 0:
        Palette_Cycle_Step(&r1, &r2);
        break;
    case 1:
        Palette_Cycle_Step(&g1, &g2);
        break;
    case 2:
        Palette_Cycle_Step(&b1, &b2);
        break;
    }
    asm mov dx, red
    asm mov dh, dl
    asm mov ax, green
    asm mov ch, al
    asm mov ax, blue
    asm mov cl, al
    asm mov bx, pal_index
    asm mov ax, 0x1010
    asm int 0x10
}

/* (&from, &to) moves palette_cycle_value by palette_cycle_speed toward the current end (state 0: toward to,
   1: back toward from), turning around just inside an end. BUG #474: when to < from the turn tests compare the
   opposite ends, so the value jumps between from + 1 and to - 1 (outside the range) every step.
   Simtex name Update_Cycle (MoO2 symbols, module palette); clarified using ReMoM repo */
void Palette_Cycle_Step(int *from, int *to)
{
    if (palette_cycle_state == PAL_CYCLE_TO_END) {
        if (*to > *from) {
            palette_cycle_value += palette_cycle_speed;
            if (*to < palette_cycle_value) {
                palette_cycle_value = *to - 1;
                palette_cycle_state = PAL_CYCLE_TO_START;
            }
        } else {
            palette_cycle_value -= palette_cycle_speed;
            if (*from > palette_cycle_value) {
                palette_cycle_value = *from + 1;
                palette_cycle_state = PAL_CYCLE_TO_START;
            }
        }
    } else {
        if (*to > *from) {
            palette_cycle_value -= palette_cycle_speed;
            if (*from > palette_cycle_value) {
                palette_cycle_value = *from + 1;
                palette_cycle_state = PAL_CYCLE_TO_END;
            }
        } else {
            palette_cycle_value += palette_cycle_speed;
            if (*to < palette_cycle_value) {
                palette_cycle_value = *to - 1;
                palette_cycle_state = PAL_CYCLE_TO_END;
            }
        }
    }
}

/* (first, count, from color, to color) a linear gradient over colors first .. first + count - 1 of the
   working palette, flagged for upload (count must be at least 2) */
void Palette_Gradient_Range(int first, int count, int r1, int g1, int b1, int r2, int g2, int b2)
{
    unsigned data_seg;                          /* saved like Palette_Set_Range's, never used here */
    int red;
    int green;
    int blue;
    int off;
    int i;

    asm mov ax, ds
    asm mov data_seg, ax
    for (i = 0; i < count; i++) {
        red = (r2 - r1) * i / (count - 1) + r1;
        green = (g2 - g1) * i / (count - 1) + g1;
        blue = (b2 - b1) * i / (count - 1) + b1;
        off = (first + i) * 3;
        Far_Poke_Byte(p_palette, off, red);
        Far_Poke_Byte(p_palette, off + 1, green);
        Far_Poke_Byte(p_palette, off + 2, blue);
    }
    Palette_Mark_Range(first, first + count - 1);
}
