/* PIC.C - WIZARDS.EXE code segment 2072 (MAGIC.EXE 1ee7, the same file): pictures. Two kinds share the header
   layout: LBX pictures (frames, struct LbxPicHeader: drawn with Draw_Pic_Frame / Draw_Pic_Clipped, decoded into a
   bitmap with Pic_Decode_Frame_To_Bitmap) and decoded column-major BITMAPS (width, height, 0DE0Ah marker, +8 remap flag, the
   pixels from +10h, column by column). Here: bitmap create / copy / shear / shift / recolor / clear / outline, the
   frame counter, the rotated and warped bitmap draws (the quadrilateral texture mapper over Pic_Warp_Span), bitmap
   onto bitmap, and the EMS-streamed animation (Anim_*).
   The callees are declared locally (no GAME.H); only those that need one have a prototype.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 2072 src/wizards/PIC.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     LBX_Error (3; LBX.C takes 4). */
#pragma warn -pro
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

extern int clip_x1;                             /* DS:76da */
extern int clip_y1;                             /* DS:76dc */
extern int clip_x2;                             /* DS:76de */
extern int clip_y2;                             /* DS:76e0 */
extern unsigned ems_frame_seg;                  /* DS:760c */
extern int ems_vgafile_handle;                   /* DS:e5f4 */
extern unsigned anim_header_block;              /* DS:e7de */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int anim_lbx_entry;                             /* DS:e870  MAGIC DS:a866   */
char anim_lbx_name[16];                         /* DS:e860  MAGIC DS:a856   */

unsigned Far_Peek_Word(unsigned seg, unsigned off);
unsigned Far_Peek_Byte(unsigned seg, unsigned off);
void far *Seg_To_Far_Ptr(unsigned seg);
void Pic_Rotate_Corners(int angle, int width, int height, int *x1, int *y1, int *x2, int *y2, int *x3, int *y3,
                        int *x4, int *y4);
void Anim_EMS_Read_Frame(long offset, long max_len, int first_page);

/* (dst, src) copies the bitmap src (header + width * height pixels) over dst.
   Simtex name Copy_Bitmap_To_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo */
void Pic_Copy(unsigned dst, unsigned src)
{
    int bytes;

    bytes = Far_Peek_Word(src, PIC_WIDTH) * Far_Peek_Word(src, PIC_HEIGHT) + 16;
    Mem_Copy_Far(0, dst, 0, src, bytes);
}

/* (width, height, pic) writes a bitmap header (width, height, marker 0DE0Ah, 0, remap flag 0) and clears the
   pixels to color 0 */
void Pic_Create_Blank(register int width, int height, unsigned pic)
{
    int count;

    Far_Poke_Word(pic, PIC_WIDTH, width);
    Far_Poke_Word(pic, PIC_HEIGHT, height);
    Far_Poke_Word(pic, PIC_FRAME, PIC_BITMAP_MARKER);
    Far_Poke_Word(pic, PIC_FRAME_COUNT, 0);
    Far_Poke_Word(pic, BITMAP_REMAP, 0);
    count = width * height;
    asm push es
    asm push di
    asm mov ax, pic
    asm mov es, ax
    asm mov di, 10h
    asm mov al, 0
    asm mov cx, count
    asm rep stosb
    asm pop di
    asm pop es
}

/* (x1, dy1, x2, dy2, pic) shifts the columns x1..x2 of the bitmap down (dy > 0) or up by an amount interpolated
   from dy1 to dy2 in 1/1000 steps, color 0 into the gap (the spellbook page curl).
   Simtex name Shear_Bitmap_Y (MoO2 symbols, module bitmap); clarified using ReMoM repo */
void Pic_Shear_Columns(int x1, int dy1, int x2, int dy2, unsigned pic)
{
    int width;
    int height;
    int x;
    long step;
    long shift_k;
    int shift;
    int keep;
    char far *pixels;
    register int i;
    register int ofs;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    if (x1 < 0)
        x1 = 0;
    if (x2 > width)
        x2 = width;
    if (x1 > x2) {
        Swap_Words(&x1, &x2);
        Swap_Words(&dy1, &dy2);
    }
    shift_k = dy1 * 1000L;
    if (x2 == x1)
        step = 0;
    else
        step = (long)(dy2 - dy1) * 1000 / (x2 - x1);
    pixels = Seg_To_Far_Ptr(PIC_PIXEL_SEG(pic));
    ofs = height * x1;
    for (x = x1; x <= x2; x++) {
        shift = shift_k / 1000;
        if (shift != 0) {
            if (shift < 0) {
                keep = height + shift;
                if (keep > 0) {
                    for (i = ofs; i < ofs + keep; i++)
                        pixels[i] = pixels[i - shift];
                    for (i = keep; i < height; i++)
                        pixels[i + ofs] = 0;
                } else if (keep < 0) {
                    for (i = ofs; i < ofs + height; i++)
                        pixels[i] = 0;
                }
            } else {
                keep = height - shift;
                if (keep > 0) {
                    for (i = ofs + height - 1; i >= ofs + shift; i--)
                        pixels[i] = pixels[i - shift];
                    for (i = 0; i < shift; i++)
                        pixels[i + ofs] = 0;
                } else if (keep < 0) {
                    for (i = ofs; i < ofs + height; i++)
                        pixels[i] = 0;
                }
            }
        }
        shift_k += step;
        ofs += height;
    }
}

/* (shifts, pic) shifts every column of the bitmap down (> 0) or up by shifts[column], color 0 into the gap.
   Simtex name Shear_Array_Bitmap_Y (MoO2 symbols, module bitmap); clarified using ReMoM repo */
void Pic_Shift_Columns_V(char *shifts, unsigned pic)
{
    int width;
    int height;
    int col;
    int shift;
    int keep;
    char far *pixels;
    register int i;
    register int ofs;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    pixels = Seg_To_Far_Ptr(PIC_PIXEL_SEG(pic));
    ofs = 0;
    for (col = 0; col < width; col++) {
        shift = shifts[col];
        if (shift != 0) {
            if (shift < 0) {
                keep = height + shift;
                if (keep > 0) {
                    for (i = ofs; i < ofs + keep; i++)
                        pixels[i] = pixels[i - shift];
                    for (i = keep; i < height; i++)
                        pixels[i + ofs] = 0;
                } else if (keep < 0) {
                    for (i = ofs; i < ofs + height; i++)
                        pixels[i] = 0;
                }
            } else {
                keep = height - shift;
                if (keep > 0) {
                    for (i = ofs + height - 1; i >= ofs + shift; i--)
                        pixels[i] = pixels[i - shift];
                    for (i = 0; i < shift; i++)
                        pixels[i + ofs] = 0;
                } else if (keep < 0) {
                    for (i = ofs; i < ofs + height; i++)
                        pixels[i] = 0;
                }
            }
        }
        ofs += height;
    }
}

/* (shifts, pic) shifts every row of the bitmap right (> 0) or left by shifts[row] columns, color 0 into the gap.
   Copied from Pic_Shift_Columns_V: the right shift keeps 'height - shift' (not width), copies down to index 0
   (reading before the pixels), and reads one pixel through an unset index first - no visible effect (#393); the left shift reads past the end
   of the pixels too (up to |shift| columns).
   Simtex name Shear_Array_Bitmap_X (MoO2 symbols, module bitmap); clarified using ReMoM repo
   Numbered: BUG #393 (BUGS_AND_QUIRKS.txt) */
void Pic_Shift_Rows_H(char *shifts, unsigned pic)
{
    int width;
    int row;
    int shift;
    int unused;
    int ofs;
    int keep;
    int shift_ofs;
    char far *pixels;
    int total;
    register int i;
    register int height;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    pixels = Seg_To_Far_Ptr(PIC_PIXEL_SEG(pic));
    ofs = 0;
    total = width * height;
    for (row = 0; row < height; row++) {
        shift = shifts[row];
        shift_ofs = shift * height;
        if (shift != 0) {
            if (shift < 0) {
                keep = width + shift;
                if (keep > 0) {
                    for (i = row; i < total; i += height)
                        pixels[i] = pixels[i - shift_ofs];
                    for (i = row + height * keep; i < total; i += height)
                        pixels[i + ofs] = 0;
                } else if (keep < 0) {
                    for (i = row; i < total; i += height)
                        pixels[i] = 0;
                }
            } else {
                keep = height - shift;
                if (keep > 0) {
                    unused = pixels[i];
                    for (i = total - height + row; i >= 0; i -= height)
                        pixels[i] = pixels[i - shift_ofs];
                    for (i = row; i < shift_ofs; i += height)
                        pixels[i] = 0;
                } else if (keep < 0) {
                    for (i = row; i < total; i += height)
                        pixels[i] = 0;
                }
            }
        }
    }
}

/* (width, height, pic, color) writes a bitmap header and fills the pixels with color */
void Pic_Create(register int width, int height, unsigned pic, int color)
{
    int count;

    Far_Poke_Word(pic, PIC_WIDTH, width);
    Far_Poke_Word(pic, PIC_HEIGHT, height);
    Far_Poke_Word(pic, PIC_FRAME, PIC_BITMAP_MARKER);
    Far_Poke_Word(pic, PIC_FRAME_COUNT, 0);
    Far_Poke_Word(pic, BITMAP_REMAP, 0);
    count = width * height;
    asm push es
    asm push di
    asm mov ax, pic
    asm mov es, ax
    asm mov di, 10h
    asm mov ax, color
    asm mov cx, count
    asm rep stosb
    asm pop di
    asm pop es
}

/* (x1, y1, x2, y2, pic, amount) adds amount to every pixel of the rectangle (color 0 too) */
void Pic_Add_Rect(int x1, int y1, int x2, int y2, unsigned pic, int amount)
{
    int height;
    int width;
    int start;
    int rows;
    int cols;
    int skip;
    int value;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    rows = y2 - y1 + 1;
    start = x1 * height + 16 - height + rows + y1;
    skip = height - rows;
    cols = x2 - x1 + 1;
    value = amount;                             /* never used */
    asm push ds
    asm push si
    asm push es
    asm push di
    asm mov si, start
    asm mov di, start
    asm mov bx, amount
    asm mov ax, skip
    asm mov bh, al
    asm mov cx, cols
    asm mov dx, rows
    asm mov ax, pic
    asm mov es, ax
    asm mov ds, ax
add_col:
    asm mov al, bh
    asm xor ah, ah
    asm add si, ax
    asm add di, ax
    asm mov dh, dl
add_px:
    asm lodsb
    asm add al, bl
    asm stosb
    asm dec dh
    asm jnz add_px
    asm loop add_col
    asm pop di
    asm pop es
    asm pop si
    asm pop ds
}

/* (x1, y1, x2, y2, pic, from, to) replaces color 'from' by 'to' inside the rectangle */
void Pic_Replace_Color_Rect(int x1, int y1, int x2, int y2, unsigned pic, int from, int to)
{
    int height;
    int width;
    int start;
    int rows;
    int cols;
    int skip;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    rows = y2 - y1 + 1;
    start = x1 * height + 16 - height + rows + y1;
    skip = height - rows;
    cols = x2 - x1 + 1;
    asm push ds
    asm push si
    asm push es
    asm push di
    asm mov si, start
    asm mov bx, to
    asm mov ax, skip
    asm mov bh, al
    asm mov cx, cols
    asm mov dx, rows
    asm mov ax, from
    asm mov ah, al
    asm mov di, pic
    asm mov es, di
    asm mov ds, di
    asm mov di, si
rc_col:
    asm push ax
    asm mov al, bh
    asm xor ah, ah
    asm add si, ax
    asm add di, ax
    asm mov dh, dl
    asm pop ax
rc_px:
    asm lodsb
    asm inc di
    asm cmp al, ah
    asm jne rc_next
    asm dec di
    asm mov al, bl
    asm stosb
rc_next:
    asm dec dh
    asm jnz rc_px
    asm loop rc_col
    asm pop di
    asm pop es
    asm pop si
    asm pop ds
}

/* (pic, color) every non-zero pixel of the bitmap becomes color (a silhouette) */
void Pic_Recolor_Opaque(unsigned pic, int color)
{
    int height;
    int width;
    int count;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    count = width * height;
    asm push ds
    asm push si
    asm push es
    asm push di
    asm mov ax, pic
    asm mov es, ax
    asm mov di, 10h
    asm mov cx, count
    asm mov dx, color
    asm mov ds, ax
    asm mov si, 10h
ro_px:
    asm lodsb
    asm inc di
    asm cmp al, 0
    asm je ro_next
    asm dec di
    asm mov al, dl
    asm stosb
ro_next:
    asm loop ro_px
    asm pop di
    asm pop es
    asm pop si
    asm pop ds
}

/* (pic) loads the LBX picture's embedded palette (frame 0), if it has one */
void Pic_Apply_Palette(unsigned pic)
{
    struct LbxPicHeader hdr;

    Mem_Copy_Far(&hdr, 0, 0, pic, PIC_HEADER_SIZE);
    if (hdr.palette_offset != 0)
        Pic_Load_Palette(pic, 0);
}

/* (x, y, pic) draws the current frame of an LBX picture unclipped and advances the frame (wrapping to the loop
   frame); embedded palette first; EMS pictures through Pic_Draw_EMS_Frame */
void Draw_Pic_Frame(register int x, register int y, unsigned pic)
{
    int remap;
    int frame;
    int skip;
    unsigned data_seg;
    struct LbxPicHeader hdr;

    Mem_Copy_Far(&hdr, 0, 0, pic, PIC_HEADER_SIZE);
    frame = hdr.frame;
    hdr.frame++;
    if (hdr.frame >= hdr.frame_count)
        Far_Poke_Word(pic, PIC_FRAME, hdr.loop_frame);
    else
        Far_Poke_Word(pic, PIC_FRAME, hdr.frame);
    if (hdr.palette_offset != 0)
        Pic_Load_Palette(pic, frame);
    /* the asm reads the picture header: byte +11h the remap flag; from +12h 4 bytes per frame, whose low nibble
       + 1 is the row skip and the offset >> 4 the frame's paragraph (the same in the decoders below) */
    if (hdr.ems_handle == 0) {
        asm push es
        asm mov ax, pic
        asm mov es, ax
        asm mov bx, 11h
        asm mov al, es:[bx]
        asm xor ah, ah
        asm mov remap, ax
        asm inc bx
        asm mov ax, frame
        asm shl ax, 1
        asm shl ax, 1
        asm add bx, ax
        asm mov ax, es:[bx]
        asm mov dx, es:[bx+2]
        asm mov bx, ax
        asm and bx, 0Fh
        asm inc bx
        asm mov skip, bx
        asm shr dx, 1
        asm rcr ax, 1
        asm shr dx, 1
        asm rcr ax, 1
        asm shr dx, 1
        asm rcr ax, 1
        asm shr dx, 1
        asm rcr ax, 1
        asm add ax, pic
        asm mov data_seg, ax
        asm pop es
        if (remap == 0)
            Pic_Blit_Planar_Rows(x, y, hdr.width, skip, data_seg);
        else
            Pic_Blit_Planar_Rows_Remap(x, y, hdr.width, skip, data_seg);
    } else
        Pic_Draw_EMS_Frame(x, y, pic, frame);
}

/* (x, y, pic) Draw_Pic_Frame clipped to clip_x1..clip_y2; returns 0 when nothing is visible */
int Draw_Pic_Clipped(int x, int y, unsigned pic)
{
    int remap;
    int frame;
    int x2;
    int y2;
    int cols;
    int rows;
    int skip_x;
    int skip_y;
    int skip;
    unsigned data_seg;
    struct LbxPicHeader hdr;
    register int sx;
    register int sy;

    if (x > clip_x2)
        return 0;
    if (y > clip_y2)
        return 0;
    Mem_Copy_Far(&hdr, 0, 0, pic, PIC_HEADER_SIZE);
    x2 = x + hdr.width - 1;
    if (x2 < clip_x1)
        return 0;
    y2 = y + hdr.height - 1;
    if (y2 < clip_y1)
        return 0;
    if (x >= clip_x1) {
        skip_x = 0;
        sx = x;
    } else {
        skip_x = clip_x1 - x;
        sx = clip_x1;
    }
    if (y >= clip_y1) {
        skip_y = 0;
        sy = y;
    } else {
        skip_y = clip_y1 - y;
        sy = clip_y1;
    }
    cols = (x2 < clip_x2 ? x2 : clip_x2) - sx + 1;
    rows = (y2 < clip_y2 ? y2 : clip_y2) - sy + 1;
    frame = hdr.frame;
    hdr.frame++;
    if (hdr.frame >= hdr.frame_count)
        Far_Poke_Word(pic, PIC_FRAME, hdr.loop_frame);
    else
        Far_Poke_Word(pic, PIC_FRAME, hdr.frame);
    if (hdr.palette_offset != 0)
        Pic_Load_Palette(pic, frame);
    if (hdr.ems_handle == 0) {
        asm push es
        asm mov ax, pic
        asm mov es, ax
        asm mov bx, 11h
        asm mov al, es:[bx]
        asm xor ah, ah
        asm mov remap, ax
        asm inc bx
        asm mov ax, frame
        asm shl ax, 1
        asm shl ax, 1
        asm add bx, ax
        asm mov ax, es:[bx]
        asm mov dx, es:[bx+2]
        asm mov bx, ax
        asm and bx, 0Fh
        asm inc bx
        asm mov skip, bx
        asm shr dx, 1
        asm rcr ax, 1
        asm shr dx, 1
        asm rcr ax, 1
        asm shr dx, 1
        asm rcr ax, 1
        asm shr dx, 1
        asm rcr ax, 1
        asm add ax, pic
        asm mov data_seg, ax
        asm pop es
        if (remap == 0)
            Pic_Blit_Clipped_Planar(sx, sy, cols, rows, skip_x, skip_y, skip, data_seg);
        else
            Pic_Blit_Clipped_Planar_Remap(sx, sy, cols, rows, skip_x, skip_y, skip, data_seg);
    } else
        Pic_Draw_Clipped_EMS(sx, sy, cols, rows, skip_x, skip_y, pic, frame);
}

/* (pic, bitmap) decodes the current frame of an LBX picture into the bitmap and advances the frame; when the
   header's full-frames byte (PIC_FULL_FRAMES, +10h) is 0 (frames drawn over each other) every frame from 0 is decoded again.
   Simtex name Draw_Picture_To_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo */
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap)
{
    int remap;
    int frame_no;
    int full;
    int last;
    int ems_ofs;
    int ems_page;
    int ems_handle;
    int skip;
    unsigned data_seg;
    struct LbxPicHeader hdr;
    register int frame;
    register int first;

    Mem_Copy_Far(&hdr, 0, 0, pic, PIC_HEADER_SIZE);
    frame = hdr.frame;
    hdr.frame++;
    if (hdr.frame >= hdr.frame_count)
        Far_Poke_Word(pic, PIC_FRAME, hdr.loop_frame);
    else
        Far_Poke_Word(pic, PIC_FRAME, hdr.frame);
    if (hdr.palette_offset != 0)
        Pic_Load_Palette(pic, frame);
    Pic_Header_Write(pic, bitmap, frame);
    if (hdr.ems_handle == 0) {
        full = Far_Peek_Byte(pic, PIC_FULL_FRAMES);
        if (full == 0)
            first = 0;
        else
            first = frame;
        last = frame;
        for (frame_no = first; frame_no <= last; frame_no++) {
            asm push es
            asm mov ax, pic
            asm mov es, ax
            asm mov bx, 11h
            asm mov al, es:[bx]
            asm xor ah, ah
            asm mov remap, ax
            asm inc bx
            asm mov ax, frame_no
            asm shl ax, 1
            asm shl ax, 1
            asm add bx, ax
            asm mov ax, es:[bx]
            asm mov dx, es:[bx+2]
            asm mov bx, ax
            asm and bx, 0Fh
            asm inc bx
            asm mov skip, bx
            asm shr dx, 1
            asm rcr ax, 1
            asm shr dx, 1
            asm rcr ax, 1
            asm shr dx, 1
            asm rcr ax, 1
            asm shr dx, 1
            asm rcr ax, 1
            asm add ax, pic
            asm mov data_seg, ax
            asm pop es
            Pic_Decode_Frame(skip, data_seg, bitmap);
        }
    } else {
        ems_ofs = hdr.ems_offset + 16;
        ems_page = hdr.ems_page;
        ems_handle = hdr.ems_handle;
        if (ems_ofs >= EMS_PAGE_SIZE) {
            ems_ofs -= EMS_PAGE_SIZE;
            ems_page++;
        }
        asm push ds
        asm mov bx, ems_page
        asm xor bh, bh
        asm mov dx, ems_handle
        asm xor dh, dh
        asm mov ax, 4400h
        asm int 67h
        asm mov bx, ems_ofs
        asm mov ax, ems_frame_seg
        asm mov ds, ax
        asm mov al, [bx]
        asm xor ah, ah
        asm mov full, ax
        asm pop ds
        if (full == 0)
            first = 0;
        else
            first = frame;
        last = frame;
        for (frame_no = first; frame_no <= last; frame_no++)
            Pic_Decode_Frame_EMS(pic, frame_no, bitmap);
    }
}

/* (pic, frame) sets the LBX picture's current frame; past the end it wraps into the loop frames.
   Simtex name Set_Animation_Frame (MoO2 symbols, module animate); clarified using ReMoM repo */
void Set_Pic_Frame(unsigned pic, unsigned frame)
{
    unsigned count;
    unsigned loop;
    unsigned span;

    frame = frame & 0x7fff;
    count = Far_Peek_Word(pic, PIC_FRAME_COUNT);
    loop = Far_Peek_Word(pic, PIC_LOOP_FRAME);
    span = count - loop;
    if (frame >= count)
        frame = loop + (frame - count) % span;
    Far_Poke_Word(pic, PIC_FRAME, frame);
}

/* (pic) back to frame 0.
   Simtex name Reset_Animation_Frame (MoO2 symbols, module animate); clarified using ReMoM repo */
void Pic_Reset_Frame(unsigned pic)
{
    Far_Poke_Word(pic, PIC_FRAME, 0);
}

/* (pic) -> the LBX picture's current frame (header +4) */
int Pic_Get_Frame(unsigned pic)
{
    return Far_Peek_Word(pic, PIC_FRAME);
}

/* (pic) -> its number of frames (header +6) */
int Pic_Frame_Count(unsigned pic)
{
    return Far_Peek_Word(pic, PIC_FRAME_COUNT);
}

/* (pic) -> its width (header +0) */
int Pic_Width(unsigned pic)
{
    return Far_Peek_Word(pic, PIC_WIDTH);
}

/* (pic) -> its height (header +2) */
int Pic_Height(unsigned pic)
{
    return Far_Peek_Word(pic, PIC_HEIGHT);
}

/* (x, y, angle, pic) draws the bitmap rotated by angle around its centre */
void Pic_Draw_Rotated(int x, int y, int angle, unsigned pic)
{
    int x2;
    int y2;
    int x3;
    int y3;
    int x4;
    int y4;
    int height;
    int width;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    Pic_Rotate_Corners(angle, width, height, &x, &y, &x2, &y2, &x3, &y3, &x4, &y4);
    Pic_Draw_Warped_Core(x, y, x2, y2, x3, y3, x4, y4, pic, height, width);
}

/* (x1, y1, x2, y2, x3, y3, x4, y4, pic, height, width) fills the quadrilateral of the 4 screen corners (bitmap
   corners top left, top right, bottom right, bottom left) with the bitmap: the corners are sorted by x and the
   area drawn as up to 3 vertical strips by Pic_Warp_Span (remap bitmaps: Pic_Warp_Span2), source and edge steps in
   8.8 fixed point, screen rows 80 bytes apart. Returns 0 when off the clip rectangle; the y range starts swapped
   (min 0, max 10000), so the vertical test misfires (#378)
   Numbered: BUG #378 (BUGS_AND_QUIRKS.txt) */
int Pic_Draw_Warped_Core(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned pic,
                         unsigned height, unsigned width)
{
    int i;
    int j;
    unsigned xs[4];
    unsigned ys[4];
    int corner[4];
    int x;
    int y;
    int len_step;
    int col_step;
    int min_y;
    int max_y;
    int frac;
    int col_step2;
    int src_step;
    /* a1 / a2: the 8.8 fractions of the source position along the bitmap's two axes, add1 / add2 the source step
       of a whole unit on each (set by the corner order); step_a / step_b the per-row steps; the *_add words their
       increments (the names are ours) */
    int a1_add;
    int a1;
    int a2;
    int a2_add;
    int step_base;
    int step_b;
    int step_b_add;
    int step_a;
    int step_a_add;
    int cols;
    int add2;
    int add1;
    int remap;
    int skip;
    unsigned len2;
    unsigned len;
    int src;

    remap = Far_Peek_Byte(pic, BITMAP_REMAP);
    xs[0] = x1;
    xs[1] = x2;
    xs[2] = x3;
    xs[3] = x4;
    ys[0] = y1;
    ys[1] = y2;
    ys[2] = y3;
    ys[3] = y4;
    corner[0] = 1;
    corner[1] = 2;
    corner[2] = 3;
    corner[3] = 4;
    max_y = 10000;
    min_y = 0;
    for (i = 0; i < 4; i++) {
        if (ys[i] < min_y)
            min_y = ys[i];
        if (ys[i] > max_y)
            max_y = ys[i];
        for (j = 0; j < 3; j++)
            if (xs[j] > xs[j + 1]) {
                Swap_Words((int *)&xs[j], (int *)&xs[j + 1]);
                Swap_Words((int *)&ys[j], (int *)&ys[j + 1]);
                Swap_Words(&corner[j], &corner[j + 1]);
            }
    }
    if (xs[0] > clip_x2 || xs[3] < clip_x1 || min_y > clip_y2 || max_y < clip_y1)
        return 0;
    if (xs[0] == xs[1]) {
        if (ys[0] > ys[1]) {
            Swap_Words((int *)&xs[0], (int *)&xs[1]);
            Swap_Words((int *)&ys[0], (int *)&ys[1]);
            Swap_Words(&corner[0], &corner[1]);
        }
        x = xs[0];
        y = ys[0];
        len = ys[1] - ys[0] + 1;
        len_step = 0;
        cols = xs[2] - xs[0] + 1;
        col_step = 0;
        frac = 0;
        col_step2 = 0;
        switch (corner[0]) {
        case 1:
            src = 0;
            add1 = 1;
            add2 = height;
            a1 = 0;
            a2 = ((long)width << 8) / (xs[2] - xs[0] + 1);
            step_b = ((long)height << 8) / (ys[1] - ys[0] + 1);
            step_a = 0;
            break;
        case 2:
            src = (width - 1) * height;
            add1 = -height;
            add2 = 1;
            a2 = ((long)height << 8) / (xs[2] - xs[0] + 1);
            a1 = 0;
            step_a = 0;
            step_b = ((long)width << 8) / (ys[1] - ys[0] + 1);
            break;
        case 3:
            src = width * height - 1;
            add1 = -1;
            add2 = -height;
            a2 = ((long)width << 8) / (xs[2] - xs[0] + 1);
            a1 = 0;
            step_a = 0;
            step_b = ((long)height << 8) / (ys[1] - ys[0] + 1);
            break;
        case 4:
            src = height - 1;
            add1 = height;
            add2 = -1;
            a1 = 0;
            a2 = ((long)height << 8) / (xs[2] - xs[0] + 1);
            step_b = ((long)width << 8) / (ys[1] - ys[0] + 1);
            step_a = 0;
            break;
        }
        src_step = 0;
        if (a1 >= 256) {
            src_step = a1 / 256 * add1;
            a1 = a1 % 256;
        }
        if (a2 >= 256) {
            src_step += a2 / 256 * add2;
            a2 = a2 % 256;
        }
        step_base = 0;
        if (step_b >= 256) {
            step_base = step_b / 256 * add1;
            step_b = step_b % 256;
        }
        if (step_a >= 256) {
            step_base += step_a / 256 * add2;
            step_a = step_a % 256;
        }
        if (xs[2] >= clip_x1 && xs[0] <= clip_x2) {
            skip = clip_x1 - xs[0] - 1;
            if (clip_x2 < xs[2])
                cols = clip_x2 - xs[0] + 1;
            if (remap == 0)
                Pic_Warp_Span(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, add1,
                              a2, add2, step_base, step_b, add1, step_a, add2, skip, clip_x1 * VGA_ROW_BYTES, (clip_x2 + 1) * VGA_ROW_BYTES);
            else
                Pic_Warp_Span2(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, add1,
                               a2, add2, step_base, step_b, add1, step_a, add2, skip, clip_x1 * VGA_ROW_BYTES, (clip_x2 + 1) * VGA_ROW_BYTES);
        }
    } else {
        x = xs[0];
        y = ys[0];
        cols = xs[1] - xs[0] + 1;
        if (ys[1] < ys[0]) {
            len2 = (ys[2] - ys[0]) * (xs[1] - xs[0]) / (xs[2] - xs[0]) + ys[0] - ys[1] + 1;
            if (len < 1)                         /* BUG #378: clamps len (unset here, set to 1 below) where len2 was
                                                   meant - len2 is never clamped; clarified using ReMoM repo */
                len = 1;
        } else
            len2 = (ys[0] - ys[2]) * (xs[1] - xs[0]) / (xs[2] - xs[0]) + ys[1] - ys[0] + 1;
        len = 1;
        len_step = ((len2 - len) << 8) / (cols - 1);
        col_step2 = -VGA_ROW_BYTES;
        if (ys[1] < ys[2])
            frac = ((ys[0] - ys[1]) << 8) / (xs[1] - xs[0]);
        else
            frac = ((ys[0] - ys[2]) << 8) / (xs[2] - xs[0]);
        if (frac < 256)
            col_step = 0;
        else {
            col_step = frac / 256 * -VGA_ROW_BYTES;
            frac = frac % 256;
        }
        if (corner[0] == 1 || corner[0] == 3) {
            if (ys[1] < ys[2]) {
                step_a = ((long)(width - 1) << 8) / (len2 - 1);
                step_b = (int)(((long)(height - 1) << 8) * (xs[1] - xs[0]) / (len2 - 1)) / (long)(xs[2] - xs[0]);
            } else {
                step_a = (int)(((long)(width - 1) << 8) * (xs[1] - xs[0]) / (len2 - 1)) / (long)(xs[2] - xs[0]);
                step_b = ((long)(height - 1) << 8) / (len2 - 1);
            }
        } else {
            if (ys[1] < ys[2]) {
                step_a = (int)(((long)(width - 1) << 8) * (xs[3] - xs[2]) / (len2 - 1)) / (long)(xs[3] - xs[1]);
                step_b = ((long)(height - 1) << 8) / (len2 - 1);
            } else {
                step_a = ((long)(width - 1) << 8) / (len2 - 1);
                step_b = (int)(((long)(height - 1) << 8) * (xs[1] - xs[0]) / (len2 - 1)) / (long)(xs[2] - xs[0]);
            }
        }
        switch (corner[0]) {
        case 1:
            src = 0;
            a1_add = 1;
            a2_add = height;
            step_b_add = 1;
            step_a_add = -height;
            a1 = 0;
            if (ys[1] < ys[2])
                a2 = ((long)(width - 1) << 8) / (xs[1] - xs[0]);
            else
                a2 = ((long)(width - 1) << 8) / (xs[2] - xs[0]);
            break;
        case 2:
            src = (width - 1) * height;
            a1_add = 1;
            a2_add = -height;
            step_b_add = -1;
            step_a_add = -height;
            a2 = 0;
            if (ys[1] < ys[2])
                a1 = ((long)(height - 1) << 8) / (xs[1] - xs[0]);
            else
                a1 = ((long)(height - 1) << 8) / (xs[2] - xs[0]);
            break;
        case 3:
            src = width * height - 1;
            a1_add = -1;
            a2_add = -height;
            step_b_add = -1;
            step_a_add = height;
            a1 = 0;
            if (ys[1] < ys[2])
                a2 = ((long)(width - 1) << 8) / (xs[1] - xs[0]);
            else
                a2 = ((long)(width - 1) << 8) / (xs[2] - xs[0]);
            break;
        case 4:
            src = height - 1;
            a1_add = -1;
            a2_add = height;
            step_b_add = 1;
            step_a_add = height;
            a2 = 0;
            if (ys[1] < ys[2])
                a1 = ((long)(height - 1) << 8) / (xs[1] - xs[0]);
            else
                a1 = ((long)(height - 1) << 8) / (xs[2] - xs[0]);
            break;
        }
        src_step = 0;
        if (a1 >= 256) {
            src_step = a1 / 256 * a1_add;
            a1 = a1 % 256;
        }
        if (a2 >= 256) {
            src_step += a2 / 256 * a2_add;
            a2 = a2 % 256;
        }
        step_base = 0;
        if (step_b >= 256) {
            step_base = step_b / 256 * step_b_add;
            step_b = step_b % 256;
        }
        if (step_a >= 256) {
            step_base += step_a / 256 * step_a_add;
            step_a = step_a % 256;
        }
        if (xs[1] >= clip_x1 && xs[0] <= clip_x2) {
            skip = clip_x1 - xs[0] - 1;
            if (clip_x2 < xs[1])
                cols = clip_x2 - xs[0] + 1;
            if (remap == 0)
                Pic_Warp_Span(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, a1_add,
                               a2, a2_add, step_base, step_b, step_b_add, step_a, step_a_add, skip, clip_x1 * VGA_ROW_BYTES,
                               (clip_x2 + 1) * VGA_ROW_BYTES);
            else
                Pic_Warp_Span2(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, a1_add,
                                a2, a2_add, step_base, step_b, step_b_add, step_a, step_a_add, skip, clip_x1 * VGA_ROW_BYTES,
                                (clip_x2 + 1) * VGA_ROW_BYTES);
        }
        x = xs[1];
        cols = xs[2] - xs[1] + 1;
        len = len2;
        len_step = 0;
        if (ys[1] < ys[2]) {
            y = ys[1];
            col_step2 = VGA_ROW_BYTES;
            frac = ((ys[3] - ys[1]) << 8) / (xs[3] - xs[1]);
            if (frac < 256)
                col_step = 0;
            else {
                col_step = frac / 256 * VGA_ROW_BYTES;
                frac = frac % 256;
            }
        } else
            y = ys[0] - (ys[0] - ys[2]) * (xs[1] - xs[0]) / (xs[2] - xs[0]);
        switch (corner[0]) {
        case 1:
            if (ys[1] < ys[2]) {
                src = (width - 1) * height;
                a2 = 0;
                a1 = ((height - 1) << 8) / (xs[3] - xs[1]);
            } else
                src = (int)((long)(width - 1) * (xs[1] - xs[0]) / (xs[2] - xs[0])) * height;
            break;
        case 2:
            if (ys[1] < ys[2]) {
                src = width * height - 1;
                a2 = ((width - 1) << 8) / (xs[3] - xs[1]);
                a1 = 0;
            } else
                src = (width - 1) * height + (int)((long)(height - 1) * (xs[1] - xs[0]) / (xs[2] - xs[0]));
            break;
        case 3:
            if (ys[1] < ys[2]) {
                src = height - 1;
                a2 = 0;
                a1 = ((height - 1) << 8) / (xs[3] - xs[1]);
            } else
                src = width * height - 1 - (int)((long)(width - 1) * (xs[1] - xs[0]) / (xs[2] - xs[0])) * height;
            break;
        case 4:
            if (ys[1] < ys[2]) {
                src = 0;
                a2 = ((width - 1) << 8) / (xs[3] - xs[1]);
                a1 = 0;
            } else
                src = height - 1 - (int)((long)(height - 1) * (xs[1] - xs[0]) / (xs[2] - xs[0]));
            break;
        }
        if (ys[1] < ys[2]) {
            src_step = 0;
            if (a1 >= 256) {
                src_step = a1 / 256 * a1_add;
                a1 = a1 % 256;
            }
            if (a2 >= 256) {
                src_step += a2 / 256 * a2_add;
                a2 = a2 % 256;
            }
        }
        if (xs[2] >= clip_x1 && xs[1] <= clip_x2) {
            skip = clip_x1 - xs[1] - 1;
            if (clip_x2 < xs[2])
                cols = clip_x2 - xs[1] + 1;
            if (remap == 0)
                Pic_Warp_Span(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, a1_add,
                               a2, a2_add, step_base, step_b, step_b_add, step_a, step_a_add, skip, clip_x1 * VGA_ROW_BYTES,
                               (clip_x2 + 1) * VGA_ROW_BYTES);
            else
                Pic_Warp_Span2(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, a1_add,
                                a2, a2_add, step_base, step_b, step_b_add, step_a, step_a_add, skip, clip_x1 * VGA_ROW_BYTES,
                                (clip_x2 + 1) * VGA_ROW_BYTES);
        }
        x = xs[2];
        cols = xs[3] - xs[2] + 1;
        len_step = -(((len2 - 1) << 8) / (cols - 1));
        if (ys[1] < ys[2])
            y = ys[3] - (ys[3] - ys[1]) * (xs[3] - xs[2]) / (xs[3] - xs[1]);
        else {
            y = ys[2];
            col_step2 = VGA_ROW_BYTES;
            frac = ((ys[3] - ys[2]) << 8) / (xs[3] - xs[2]);
            if (frac < 256)
                col_step = 0;
            else {
                col_step = frac / 256 * VGA_ROW_BYTES;
                frac = frac % 256;
            }
        }
        switch (corner[0]) {
        case 1:
            if (ys[1] < ys[2])
                src = (int)((long)(height - 1) * (xs[2] - xs[1]) / (xs[3] - xs[1])) + (width - 1) * height;
            else {
                src = (width - 1) * height;
                a2 = 0;
                a1 = ((height - 1) << 8) / (xs[3] - xs[2]);
            }
            break;
        case 2:
            if (ys[1] < ys[2])
                src = width * height - 1 - (int)((long)(width - 1) * (xs[2] - xs[1]) / (xs[3] - xs[1])) * height;
            else {
                src = width * height - 1;
                a2 = ((width - 1) << 8) / (xs[3] - xs[2]);
                a1 = 0;
            }
            break;
        case 3:
            if (ys[1] < ys[2])
                src = height - 1 - (int)((long)(height - 1) * (xs[2] - xs[1]) / (xs[3] - xs[1]));
            else {
                src = height - 1;
                a2 = 0;
                a1 = ((height - 1) << 8) / (xs[3] - xs[2]);
            }
            break;
        case 4:
            if (ys[1] < ys[2])
                src = (int)((long)(width - 1) * (xs[2] - xs[1]) / (xs[3] - xs[1])) * height;
            else {
                src = 0;
                a2 = ((width - 1) << 8) / (xs[3] - xs[2]);
                a1 = 0;
            }
            break;
        }
        if (ys[1] >= ys[2]) {
            src_step = 0;
            if (a1 >= 256) {
                src_step = a1 / 256 * a1_add;
                a1 = a1 % 256;
            }
            if (a2 >= 256) {
                src_step += a2 / 256 * a2_add;
                a2 = a2 % 256;
            }
        }
        if (xs[3] >= clip_x1 && xs[2] <= clip_x2) {
            skip = clip_x1 - xs[2] - 1;
            if (clip_x2 < xs[3])
                cols = clip_x2 - xs[2] + 1;
            if (remap == 0)
                Pic_Warp_Span(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, a1_add,
                               a2, a2_add, step_base, step_b, step_b_add, step_a, step_a_add, skip, clip_x1 * VGA_ROW_BYTES,
                               (clip_x2 + 1) * VGA_ROW_BYTES);
            else
                Pic_Warp_Span2(x, y, cols, len, len_step, col_step, frac, col_step2, src, pic + 1, src_step, a1, a1_add,
                                a2, a2_add, step_base, step_b, step_b_add, step_a, step_a_add, skip, clip_x1 * VGA_ROW_BYTES,
                                (clip_x2 + 1) * VGA_ROW_BYTES);
        }
    }
}
/* (x1, y1, x2, y2, pic) DEAD: draws the bitmap stretched along the line (x1, y1) - (x2, y2), half its height to
   each side; found because ReMoM's numbering ran ahead of ours (10/17 audit); clarified using ReMoM repo */
void Pic_Draw_Along_Line(register int x1, int y1, int x2, int y2, unsigned pic)
{
    int x3;
    int y3;
    int x4;
    int y4;
    int dy;
    int dx;
    int width;
    int angle;
    int remap_flag;
    int perp;
    int ox;
    int oy;
    register int height;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    remap_flag = Far_Peek_Word(pic, BITMAP_REMAP);   /* the remap flag; never used */
    dx = x2 - x1;
    dy = y2 - y1;
    angle = Atan2_Deg(dx, dy);
    perp = 90 - angle;
    if (perp < 0)
        perp += 360;
    ox = Cos_Mul(perp, height) / 2;
    oy = Sin_Mul(perp, height) / 2;
    x1 += ox;
    y1 -= oy;
    x2 += ox;
    y2 -= oy;
    x3 = x2 - Sin_Mul(angle, height * 12) / 10;
    y3 = y2 + Cos_Mul(angle, height);
    x4 = x1 - Sin_Mul(angle, height * 12) / 10;
    y4 = y1 + Cos_Mul(angle, height);
    Pic_Draw_Warped_Core(x1, y1, x2, y2, x3, y3, x4, y4, pic, height, width);
}

/* (angle, width, height, &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4) the bitmap's 4 corners from its top left
   (x1, y1), rotated by angle around the centre with the aspect corrections 12/10 (x) and 22/25 (y) */
void Pic_Rotate_Corners(int angle, int width, int height, int *x1, int *y1, int *x2, int *y2, int *x3, int *y3,
                        int *x4, int *y4)
{
    int cx;
    int cy;
    int old_x;

    cx = *x1 + width / 2;
    cy = *y1 + height / 2;
    *x2 = *x1 + width - 1;
    *y2 = *y1;
    *x3 = *x1 + width - 1;
    *y3 = *y1 + height - 1;
    *x4 = *x1;
    *y4 = *y1 + height - 1;
    old_x = *x1;
    *x1 = cx + Cos_Mul(angle, *x1 - cx) - Sin_Mul(angle, (*y1 - cy) * 12) / 10;
    *y1 = cy + Cos_Mul(angle, *y1 - cy) + Sin_Mul(angle, (old_x - cx) * 22) / 25;
    old_x = *x2;
    *x2 = cx + Cos_Mul(angle, *x2 - cx) - Sin_Mul(angle, (*y2 - cy) * 12) / 10;
    *y2 = cy + Cos_Mul(angle, *y2 - cy) + Sin_Mul(angle, (old_x - cx) * 22) / 25;
    old_x = *x3;
    *x3 = cx + Cos_Mul(angle, *x3 - cx) - Sin_Mul(angle, (*y3 - cy) * 12) / 10;
    *y3 = cy + Cos_Mul(angle, *y3 - cy) + Sin_Mul(angle, (old_x - cx) * 22) / 25;
    old_x = *x4;
    *x4 = cx + Cos_Mul(angle, *x4 - cx) - Sin_Mul(angle, (*y4 - cy) * 12) / 10;
    *y4 = cy + Cos_Mul(angle, *y4 - cy) + Sin_Mul(angle, (old_x - cx) * 22) / 25;
}

/* (x, y, pic) draws the bitmap unclipped */
void Draw_Bitmap(int x, int y, unsigned pic)
{
    Pic_Blit_Planar(x, y, PIC_HEADER_SIZE, pic, Far_Peek_Word(pic, PIC_WIDTH), Far_Peek_Word(pic, PIC_HEIGHT), 0);
}

/* (x, y, pic) draws the bitmap clipped to clip_x1..clip_y2; returns 0 when nothing is visible */
int Draw_Bitmap_Clipped(register int x, register int y, unsigned pic)
{
    int x2;
    int y2;
    int skip;
    int width;
    int height;
    int skip_x;
    int skip_y;
    int sx;
    int sy;
    int rows;
    int cols;
    int src;

    if (x > clip_x2)
        return 0;
    if (y > clip_y2)
        return 0;
    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    x2 = x + width - 1;
    if (x2 < clip_x1)
        return 0;
    y2 = y + height - 1;
    if (y2 < clip_y1)
        return 0;
    if (x >= clip_x1) {
        skip_x = 0;
        sx = x;
    } else {
        skip_x = clip_x1 - x;
        sx = clip_x1;
    }
    if (y >= clip_y1) {
        skip_y = 0;
        sy = y;
    } else {
        skip_y = clip_y1 - y;
        sy = clip_y1;
    }
    cols = (x2 < clip_x2 ? x2 : clip_x2) - sx + 1;
    if (cols > width)
        cols = width;
    rows = (y2 < clip_y2 ? y2 : clip_y2) - sy + 1;
    if (rows > height)
        rows = height;
    src = skip_x * height + skip_y + 16;
    skip = height - rows;
    Pic_Blit_Planar(sx, sy, src, pic, cols, rows, skip);
}

/* (x, y, dst, src) copies the bitmap src onto the bitmap dst at (x, y), clipped to dst; src color 0 is
   transparent. Returns 0 when nothing overlaps */
int Pic_Draw_Into_Pic(int x, int y, unsigned dst, unsigned src)
{
    int x2;
    int y2;
    int src_skip;
    int dst_skip;
    int width;
    int skip_x;
    int skip_y;
    int dx;
    int dy;
    int cols;
    int src_ofs;
    int dst_ofs;
    register int height;
    register int rows;

    width = Far_Peek_Word(dst, PIC_WIDTH);
    height = Far_Peek_Word(dst, PIC_HEIGHT);
    if (width - 1 < x)
        return 0;
    if (height - 1 < y)
        return 0;
    x2 = x + Far_Peek_Word(src, PIC_WIDTH) - 1;
    if (x2 < 0)
        return 0;
    y2 = y + Far_Peek_Word(src, PIC_HEIGHT) - 1;
    if (y2 < 0)
        return 0;
    if (x >= 0) {
        skip_x = 0;
        dx = x;
    } else {
        skip_x = -x;
        dx = 0;
    }
    if (y >= 0) {
        skip_y = 0;
        dy = y;
    } else {
        skip_y = -y;
        dy = 0;
    }
    cols = width - 1 > x2 ? x2 - dx + 1 : width - dx;
    if (cols > width)
        cols = width;
    rows = (height - 1 > y2 ? y2 : height - 1) - dy + 1;
    if (rows > height)
        rows = height;
    dst_ofs = dx * height + dy + 16;
    dst_skip = height - rows;
    src_ofs = Far_Peek_Word(src, PIC_HEIGHT) * skip_x + skip_y + 16;
    src_skip = Far_Peek_Word(src, PIC_HEIGHT) - rows;
    Pic_Copy_Into_Pic_Core(dst_ofs, dst, src_ofs, src, dst_skip, src_skip, cols, rows);
}

/* (x, y, dst, mask) clears the pixels of dst where the bitmap mask (placed at x, y) is 0; the same clipping as
   Pic_Draw_Into_Pic. Returns 0 when nothing overlaps */
int Pic_Cut_To_Mask(int x, int y, unsigned dst, unsigned mask)
{
    int x2;
    int y2;
    int src_skip;
    int dst_skip;
    int width;
    int skip_x;
    int skip_y;
    int dx;
    int dy;
    int cols;
    int src_ofs;
    int dst_ofs;
    register int height;
    register int rows;

    width = Far_Peek_Word(dst, PIC_WIDTH);
    height = Far_Peek_Word(dst, PIC_HEIGHT);
    if (width - 1 < x)
        return 0;
    if (height - 1 < y)
        return 0;
    x2 = x + Far_Peek_Word(mask, PIC_WIDTH) - 1;
    if (x2 < 0)
        return 0;
    y2 = y + Far_Peek_Word(mask, PIC_HEIGHT) - 1;
    if (y2 < 0)
        return 0;
    if (x >= 0) {
        skip_x = 0;
        dx = x;
    } else {
        skip_x = -x;
        dx = 0;
    }
    if (y >= 0) {
        skip_y = 0;
        dy = y;
    } else {
        skip_y = -y;
        dy = 0;
    }
    cols = width - 1 > x2 ? x2 - dx + 1 : width - dx;
    if (cols > width)
        cols = width;
    rows = (height - 1 > y2 ? y2 : height - 1) - dy + 1;
    if (rows > height)
        rows = height;
    dst_ofs = dx * height + dy + 16;
    dst_skip = height - rows;
    src_ofs = Far_Peek_Word(mask, PIC_HEIGHT) * skip_x + skip_y + 16;
    src_skip = Far_Peek_Word(mask, PIC_HEIGHT) - rows;
    Pic_Cut_To_Mask_Core(dst_ofs, dst, src_ofs, mask, dst_skip, src_skip, cols, rows);
}

/* (pic, lo, hi) pixels with colors lo..hi become 0 */
void Pic_Clear_Color_Range(unsigned pic, int lo, int hi)
{
    int count;
    int width;
    int height;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    count = width * height;
    asm push ds
    asm push es
    asm push si
    asm push di
    asm mov si, 10h
    asm mov di, 10h
    asm mov cx, count
    asm mov bx, lo
    asm mov dx, hi
    asm mov ax, pic
    asm mov es, ax
    asm mov ds, ax
cr_px:
    asm lodsb
    asm cmp al, bl
    asm jb cr_next
    asm cmp al, dl
    asm ja cr_next
    asm xor al, al
    asm stosb
    asm dec di
cr_next:
    asm inc di
    asm loop cr_px
    asm pop di
    asm pop si
    asm pop es
    asm pop ds
}

/* (x1, y1, x2, y2, pic) DEAD (no caller): clears everything of the bitmap outside the rectangle (all of it when the rectangle is off
   the bitmap); the right-hand clear starts AT column x2, wiping the rectangle's last column (#394); likewise the clear below
   starts AT row y2 and stops one row short of the bottom
   Numbered: BUG #394 (BUGS_AND_QUIRKS.txt) */
int Pic_Keep_Rect(int x1, int y1, register int x2, int y2, unsigned pic)
{
    int width;
    int col;
    int tail;
    register int height;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    if (x1 >= width || y1 >= height || x2 < 0 || y2 < 0) {
        Pic_Mem_Clear_Far(16, pic, height * width);
        return 0;
    }
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (width - 1 < x2)
        x2 = width - 1;
    if (height - 1 < y2)
        y2 = height - 1;
    if (x1 > 0)
        Pic_Mem_Clear_Far(16, pic, x1 * height);
    tail = height - 1 - y2;
    for (col = x1; col <= x2; col++) {
        if (y1 != 0)
            Pic_Mem_Clear_Far(col * height + 16, pic, y1);
        if (tail != 0)
            Pic_Mem_Clear_Far(col * height + y2 + 16, pic, tail);
    }
    if (width - 1 > x2)
        Pic_Mem_Clear_Far(x2 * height + 16, pic, (width - x2) * height);
}

/* (x1, y1, x2, y2, pic) clears the rectangle of the bitmap to color 0 (all of it when the rectangle is off the
   bitmap) */
int Pic_Clear_Rect(int x1, int y1, int x2, int y2, unsigned pic)
{
    int width;
    int height;
    int skip;
    int start;
    int cols;
    int rows;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    if (x1 >= width || y1 >= height || x2 < 0 || y2 < 0) {
        Pic_Mem_Clear_Far(16, pic, height * width);
        return 0;
    }
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (width - 1 < x2)
        x2 = width - 1;
    if (height - 1 < y2)
        y2 = height - 1;
    rows = y2 - y1 + 1;
    skip = height - rows;
    start = x1 * height + y1;
    cols = x2 - x1 + 1;
    asm push di
    asm push es
    asm mov dx, rows
    asm mov bx, cols
    asm mov si, skip
    asm mov di, start
    asm mov ax, pic
    asm inc ax
    asm mov es, ax
    asm xor al, al
clr_col:
    asm mov cx, dx
    asm rep stosb
    asm add di, si
    asm dec bx
    asm jnz clr_col
    asm pop es
    asm pop di
}

/* (pic, ramp, len) Pic_Recolor_By_Brightness over the whole ramp */
void Pic_Recolor_By_Brightness_All(unsigned pic, unsigned char *ramp, int len)
{
    Pic_Recolor_By_Brightness(pic, ramp, len, 0, len - 1);
}

/* (pic, ramp, len, from, to) Pic_Recolor_By_Brightness with from / to clamped to the ramp */
void Pic_Recolor_By_Brightness_Range(unsigned pic, unsigned char *ramp, int len, int from, int to)
{
    if (from < 0)
        from = 0;
    if (to >= len)
        to = len - 1;
    Pic_Recolor_By_Brightness(pic, ramp, len, from, to);
}

/* (pic, ramp, len, from, to) maps every color used in the bitmap linearly by brightness (darkest .. brightest)
   onto ramp[0 .. len - 1]; colors landing outside ramp[from .. to] are left alone. Returns 0 for len 0 */
int Pic_Recolor_By_Brightness(unsigned pic, unsigned char *ramp, int len, int from, int to)
{
    unsigned char lo;
    unsigned char hi;
    int range;
    int pos;
    unsigned char table[256];
    register int i;

    if (len == 0)
        return 0;
    Pic_Brightness_Table(pic, table);
    hi = 0;
    lo = 0xff;
    for (i = 0; i < 256; i++)
        if (table[i] != 0xff) {
            if (table[i] < lo)
                lo = table[i];
            if (table[i] > hi)
                hi = table[i];
        }
    range = hi - lo;
    if (range != 0) {
        for (i = 0; i < 256; i++)
            if (table[i] != 0xff) {
                pos = (table[i] + range / len / 2 - lo) * (len - 1) / range;
                if (pos >= from && pos <= to)
                    table[i] = ramp[pos];
                else
                    table[i] = 0xff;
            }
    } else {
        for (i = 0; i < 256; i++)
            if (table[i] != 0xff)
                table[i] = *ramp;
    }
    for (i = 0; i < 256; i++)
        if (table[i] != 0xff)
            Pic_Replace_Color(pic, i, table[i]);
}

/* (pic, x_pct, y_pct) rescales the bitmap in place (blank when it would shrink below 1 pixel): one axis up and the
   other down is done as two passes.
   Simtex name Scale_Bitmap (MoO2 symbols, module bitmap); clarified using ReMoM repo */
void Pic_Scale_Percent(unsigned pic, register int x_pct, register int y_pct)
{
    int shrink_x;
    int shrink_y;
    int width;
    int height;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    if (x_pct <= 0 || y_pct <= 0 || width * x_pct / 100 < 1 || height * y_pct / 100 < 1) {
        Pic_Create_Blank(width, height, pic);
        return;
    }
    shrink_x = x_pct;
    shrink_y = y_pct;
    if (x_pct < 100 && y_pct > 100)
        x_pct = 100;
    if (x_pct > 100 && y_pct < 100)
        y_pct = 100;
    if (x_pct > 100 || y_pct > 100)
        Pic_Enlarge(pic, x_pct, y_pct);
    if (shrink_x > 100)
        shrink_x = 100;
    if (shrink_y > 100)
        shrink_y = 100;
    if (shrink_x < 100 || shrink_y < 100)
        Pic_Shrink(pic, shrink_x, shrink_y);
}

/* (lbx, entry) streams an LBX animation through EMS: the first 3FACh bytes of the entry go to the VGAFILEH
   handle, the 16-byte header to anim_header_block with +0Ah = the EMS handle, +0Ch = 0 */
void Anim_Load_To_EMS(char *lbx, int entry)
{
    strcpy(anim_lbx_name, lbx);
    anim_lbx_entry = entry;
    Anim_EMS_Read_Frame(0L, 0x3facL, 0);
    EMS_Read(0, anim_header_block, 0L, 16, ems_vgafile_handle);
    Far_Poke_Word(anim_header_block, PIC_EMS_HANDLE, ems_vgafile_handle);
    Far_Poke_Word(anim_header_block, PIC_EMS_OFFSET, 0);
}

/* () reads the current frame of the EMS-streamed animation from the LBX file into EMS (its offset temporarily
   4000h) and draws it at (0, 0); advances the frame. Frames over 64 KB: LBX error 16 */
void Anim_EMS_Next_Frame(void)
{
    long frame_ofs;
    long window_ofs;
    long next_ofs;
    long size;
    register int frame;
    register int next;

    frame = Far_Peek_Word(anim_header_block, PIC_FRAME);
    next = frame + 1;
    if (Far_Peek_Word(anim_header_block, PIC_FRAME_COUNT) <= next)
        Far_Poke_Word(anim_header_block, PIC_FRAME, Far_Peek_Word(anim_header_block, PIC_LOOP_FRAME));
    else
        Far_Poke_Word(anim_header_block, PIC_FRAME, next);
    if (Far_Peek_Word(anim_header_block, PIC_PALETTE) != 0)
        Pic_Load_Palette(anim_header_block, frame);
    EMS_Read(&frame_ofs, 0, (long)(frame * 4 + PIC_FRAME_TABLE), 4, ems_vgafile_handle);
    EMS_Read(&next_ofs, 0, (long)(frame * 4 + (PIC_FRAME_TABLE + 4)), 4, ems_vgafile_handle);   /* the next frame's offset */
    size = next_ofs - frame_ofs;
    if (size > 0x10000L)
        LBX_Error(anim_lbx_name, LBX_ERR_FRAME_TOO_BIG, anim_lbx_entry);
    window_ofs = 0x4000L;
    EMS_Write(&window_ofs, 0, (long)(frame * 4 + PIC_FRAME_TABLE), 4, ems_vgafile_handle);
    Anim_EMS_Read_Frame(frame_ofs, size, 1);
    Anim_Blit_EMS_Frame(0, 0, anim_header_block, frame);
    EMS_Write(&frame_ofs, 0, (long)(frame * 4 + PIC_FRAME_TABLE), 4, ems_vgafile_handle);
}

/* (offset, max_len, first_page) reads max_len bytes from offset into entry anim_lbx_entry of the file
   anim_lbx_name into the VGAFILEH handle from logical page first_page on, 16 KB at a time. The read errors
   name an uninitialised buffer, not the file (#395)
   Numbered: BUG #395 (BUGS_AND_QUIRKS.txt) */
void Anim_EMS_Read_Frame(long offset, long max_len, int first_page)
{
    int pages;
    unsigned frame_seg;
    int chunk;
    long left;
    long size;
    long start;
    char name[20];
    register int page;
    register int handle;

    if (ems_vgafile_handle == 0)
        LBX_Error(anim_lbx_name, LBX_ERR_EMS_FULL, 0);
    handle = LBX_Entry_Locate(anim_lbx_name, anim_lbx_entry, &start, &size, 0);
    start += offset;
    size -= offset;
    if (size <= 0)
        LBX_Error(name, LBX_ERR_CORRUPT, anim_lbx_entry);
    if (max_len < size)
        size = max_len;
    if (DOS_Seek(start, handle) == 0)
        LBX_Error(name, LBX_ERR_CORRUPT, anim_lbx_entry);
    pages = size / EMS_PAGE_SIZE;               /* pages: counted, never used */
    if (size % EMS_PAGE_SIZE)
        pages++;
    frame_seg = EMS_Page_Frame();
    page = first_page;
    left = size;
    chunk = EMS_PAGE_SIZE;
    while (left >= EMS_PAGE_SIZE) {
        left -= EMS_PAGE_SIZE;
        EMS_Map_Handle_Pages(ems_vgafile_handle, page);
        DOS_Read_To_Segment(frame_seg, chunk, handle);
        page++;
    }
    if (left > 0) {
        chunk = left;
        EMS_Map_Handle_Pages(ems_vgafile_handle, page);
        DOS_Read_To_Segment(frame_seg, chunk, handle);
    }
}

/* () -> the current frame of the animation being played (its header +4).
   Simtex name Get_File_Animation_Frame (MoO2 symbols, module file_ani); clarified using ReMoM repo */
int Anim_Get_Frame(void)
{
    return Far_Peek_Word(anim_header_block, PIC_FRAME);
}

/* (frame) sets it. Simtex name Set_File_Animation_Frame (MoO2 symbols, module file_ani); clarified using ReMoM repo */
void Anim_Set_Frame(int frame)
{
    Far_Poke_Word(anim_header_block, PIC_FRAME, frame);
}

/* (pic) the LBX picture header's full-frames byte (PIC_FULL_FRAMES, +10h; 0 = frames drawn over each other from frame 0), read from EMS for
   EMS pictures. Simtex name Get_Full_Store_Flag (MoO2 symbols, module animate); clarified using ReMoM repo */
int Pic_Full_Frames_Flag(unsigned pic)
{
    int result;
    long ofs;
    int handle;

    handle = Far_Peek_Byte(pic, PIC_EMS_HANDLE);
    result = 0;
    if (handle == 0)
        result = Far_Peek_Byte(pic, PIC_FULL_FRAMES);
    else {
        ofs = ((long)Far_Peek_Byte(pic, PIC_EMS_PAGE) << 14) + Far_Peek_Word(pic, PIC_EMS_OFFSET);
        EMS_Read(&result, 0, ofs + PIC_FULL_FRAMES, 1, handle);
    }
    return result;
}

/* (pic) sets the bitmap's remap flag (+8): translucent colors E8h+ drawn through the remap tables */
void Pic_Set_Remap_Flag(unsigned pic)
{
    Far_Poke_Byte(pic, BITMAP_REMAP, 1);
}

/* (pic, color) draws an outline in color around the opaque parts of the bitmap: one pass down the columns
   marks the pixel before and after each run, one pass along the rows does the same sideways.
   Simtex name Outline_Bitmap_Pixels (MoO2 symbols, module shear); clarified using ReMoM repo */
void Pic_Outline_Recolor(unsigned pic, int color)
{
    int height;
    int width;
    int count;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    count = width * height;
    asm push ds
    asm push si
    asm push di
    asm mov cx, count
    asm mov dx, height
    asm mov ax, color
    asm mov ah, al
    asm mov bx, pic
    asm inc bx
    asm mov ds, bx
    asm mov bx, 0
    asm mov di, 0
    asm mov si, 0
ol_v_px:
    asm lodsb
    asm cmp al, 0
    asm je ol_v_clear
    asm cmp al, ah
    asm je ol_v_clear
    asm cmp bh, 0
    asm je ol_v_set
    asm mov [si-2], ah
ol_v_set:
    asm xor bh, bh
    asm mov bl, 1
    asm jmp ol_v_next
ol_v_clear:
    asm cmp bl, 0
    asm je ol_v_mark
    asm mov [si-1], ah
ol_v_mark:
    asm xor bl, bl
    asm mov bh, 1
ol_v_next:
    asm inc di
    asm cmp di, dx
    asm js ol_v_loop
    asm xor di, di
    asm xor bx, bx
ol_v_loop:
    asm loop ol_v_px
    asm pop di
    asm pop si
    asm pop ds
    asm push ds
    asm push si
    asm push di
    asm mov cx, count
    asm mov di, cx
    asm mov dx, height
    asm mov ax, color
    asm mov ah, al
    asm mov bx, pic
    asm inc bx
    asm mov ds, bx
    asm mov bx, 0
    asm mov si, 0
ol_h_px:
    asm lodsb
    asm cmp al, 0
    asm je ol_h_clear
    asm cmp al, ah
    asm je ol_h_clear
    asm cmp bh, 0
    asm je ol_h_set
    asm sub si, dx
    asm mov [si-1], ah
    asm add si, dx
ol_h_set:
    asm xor bh, bh
    asm mov bl, 1
    asm jmp ol_h_next
ol_h_clear:
    asm cmp bl, 0
    asm je ol_h_mark
    asm mov [si-1], ah
ol_h_mark:
    asm xor bl, bl
    asm mov bh, 1
ol_h_next:
    asm dec si
    asm add si, dx
    asm cmp si, di
    asm js ol_h_loop
    asm sub si, di
    asm inc si
    asm xor bx, bx
ol_h_loop:
    asm loop ol_h_px
    asm pop di
    asm pop si
    asm pop ds
}

/* (pic, color) Pic_Outline_Recolor that treats colors above DFh as transparent too.
   Simtex name Outline_Bitmap_Pixels_No_Glass (MoO2 symbols, module shear); clarified using ReMoM repo */
void Pic_Outline_Recolor2(unsigned pic, int color)
{
    int height;
    int width;
    int count;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    count = width * height;
    asm push ds
    asm push si
    asm push di
    asm mov cx, count
    asm mov dx, height
    asm mov ax, color
    asm mov ah, al
    asm mov bx, pic
    asm inc bx
    asm mov ds, bx
    asm mov bx, 0
    asm mov di, 0
    asm mov si, 0
ol2_v_px:
    asm lodsb
    asm cmp al, 0
    asm je ol2_v_clear
    asm cmp al, ah
    asm je ol2_v_clear
    asm cmp al, 0DFh                     /* above DFh: empty, as 0 */
    asm ja ol2_v_clear
    asm cmp bh, 0
    asm je ol2_v_set
    asm mov [si-2], ah
ol2_v_set:
    asm xor bh, bh
    asm mov bl, 1
    asm jmp ol2_v_next
ol2_v_clear:
    asm cmp bl, 0
    asm je ol2_v_mark
    asm mov [si-1], ah
ol2_v_mark:
    asm xor bl, bl
    asm mov bh, 1
ol2_v_next:
    asm inc di
    asm cmp di, dx
    asm js ol2_v_loop
    asm xor di, di
    asm xor bx, bx
ol2_v_loop:
    asm loop ol2_v_px
    asm pop di
    asm pop si
    asm pop ds
    asm push ds
    asm push si
    asm push di
    asm mov cx, count
    asm mov di, cx
    asm mov dx, height
    asm mov ax, color
    asm mov ah, al
    asm mov bx, pic
    asm inc bx
    asm mov ds, bx
    asm mov bx, 0
    asm mov si, 0
ol2_h_px:
    asm lodsb
    asm cmp al, 0
    asm je ol2_h_clear
    asm cmp al, ah
    asm je ol2_h_clear
    asm cmp al, 0DFh                     /* above DFh: empty, as 0 */
    asm ja ol2_h_clear
    asm cmp bh, 0
    asm je ol2_h_set
    asm sub si, dx
    asm mov [si-1], ah
    asm add si, dx
ol2_h_set:
    asm xor bh, bh
    asm mov bl, 1
    asm jmp ol2_h_next
ol2_h_clear:
    asm cmp bl, 0
    asm je ol2_h_mark
    asm mov [si-1], ah
ol2_h_mark:
    asm xor bl, bl
    asm mov bh, 1
ol2_h_next:
    asm dec si
    asm add si, dx
    asm cmp si, di
    asm js ol2_h_loop
    asm sub si, di
    asm inc si
    asm xor bx, bx
ol2_h_loop:
    asm loop ol2_h_px
    asm pop di
    asm pop si
    asm pop ds
}

/* (pic, color, pattern) replaces color by pattern[k], k per pixel of the whole picture = 0, 0, 1, 3, 6, 2, 7, 5, 4, 4, ...
   (a step 0..7 added after each pixel, mod 8: a dither) */
void Pic_Replace_Color_Dithered(unsigned pic, int color, char *pattern)
{
    int width;
    int count;
    int height;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    count = width * height;
    asm push es
    asm push si
    asm push di
    asm mov cx, count
    asm mov ax, color
    asm mov ah, al
    asm mov si, pattern
    asm mov bx, pic
    asm inc bx
    asm mov es, bx
    asm xor bx, bx
    asm xor dx, dx
    asm xor di, di
rp_px:
    asm mov al, es:[di]
    asm cmp al, ah
    asm jne rp_next
    asm mov al, [bx+si]
    asm mov es:[di], al
rp_next:
    asm inc di
    asm add bx, dx
    asm and bx, 7
    asm inc dx
    asm and dx, 7
    asm loop rp_px
    asm pop di
    asm pop si
    asm pop es
}

/* (pic, x, y, w, h, color) DEAD (no caller): draws a rectangle outline into the bitmap, edges kept off row / column 0 and inside
   the bitmap. Indexes from the header, not the pixels, and uses h as the column stride (#396)
   Numbered: BUG #396 (BUGS_AND_QUIRKS.txt) */
void Pic_Draw_Rect_Outline(unsigned pic, int x, int y, int w, int h, int color)
{
    int height;
    int width;
    int size;                                   /* computed, never used */
    char far *pixels;
    register int i;
    register int j;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    size = height * width;                       /* never used */
    pixels = Seg_To_Far_Ptr(pic);
    if (x < width && x > 0)
        for (j = y; j < y + h; j++)
            if (j < height && j > 0)
                pixels[x * h + j] = color;
    if (x + w - 1 < width && x + w - 1 > 0)
        for (j = y; j < y + h; j++)
            if (j < height && j > 0)
                pixels[(x + w - 1) * h + j] = color;
    if (y < height && y > 0)
        for (i = x; i < x + w; i++)
            if (i < width && i > 0)
                pixels[i * h + y] = color;
    if (y + h - 1 < height && y + h - 1 > 0)
        for (i = x; i < x + w; i++)
            if (i < width && i > 0)
                pixels[y + h + i * h - 1] = color;
}

/* (pic, &x, &y, &w, &h) the bounding box of the bitmap's non-zero pixels.
   Simtex name Get_Bitmap_Actual_Size (MoO2 symbols, module bitmap); clarified using ReMoM repo */
void Pic_Get_Opaque_Bounds(unsigned pic, register int *x, register int *y, int *w, int *h)
{
    int height;
    int width;
    int max_y;
    int max_x;
    int size;                                   /* computed, never used */
    int col;
    int row;
    int src;
    char far *pixels;

    width = Far_Peek_Word(pic, PIC_WIDTH);
    height = Far_Peek_Word(pic, PIC_HEIGHT);
    size = height * width;                       /* never used */
    pixels = Seg_To_Far_Ptr(pic);
    *x = 1000;
    *y = 1000;
    max_x = 0;
    max_y = 0;
    src = PIC_HEADER_SIZE;
    for (col = 0; col < width; col++)
        for (row = 0; row < height; row++) {
            if (pixels[src] != 0) {
                if (*x > col)
                    *x = col;
                if (*y > row)
                    *y = row;
                if (max_x < col)
                    max_x = col;
                if (max_y < row)
                    max_y = row;
            }
            src++;
        }
    *h = max_y - *y + 1;
    *w = max_x - *x + 1;
}
