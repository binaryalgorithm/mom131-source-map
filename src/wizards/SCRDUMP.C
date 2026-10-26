/* SCRDUMP.C - WIZARDS.EXE code segment 1e46 (MAGIC.EXE 1db4, the same file): the developer screenshot (an Autodesk
   FLI file of the displayed screen), the release flag that turns the developer keys off, and grabbing a screen
   rectangle into a picture. C with inline assembly for the VGA reads.
   This module's data (DS:7876): release_flag, then its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1e46 src/wizards/SCRDUMP.C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ENUMS.H"

int release_flag = 0;                           /* 1 in the shipped game (Release_Version at start-up) */

#define FLI_MAGIC 0xAF11                        /* the Autodesk FLI file header's type word */
#define FLI_FRAME_MAGIC 0xF1FA                  /* a frame header's */
#define FLI_COLOR 0x0B                          /* chunk types: the 64-level palette */
#define FLI_COPY 0x10                           /* and the uncompressed 320 x 200 picture */

extern unsigned draw_page_seg;                  /* DS:76ec */

void Memory_Report_Exit(void);
void Set_Draw_Page_Front(void);
void Set_Draw_Page(void);
void Mouse_Hide(void);
void Mouse_Show(void);
int DOS_Find_File(char *pattern, char *out);
void Pic_Create_Blank(int width, int height, unsigned pic);
void VGA_Read_DAC_Block(int block, char *buffer);
void VGA_Copy_Row(int row, char *buffer);
void Screen_Grab_Rect_Core(unsigned dest_seg, int x1, int y1, int x2, int y2);

/* developer key: the memory report (not in the release build).
   Simtex name F11_Key (MoO2 symbols, module fields); clarified using ReMoM repo */
int Debug_Memory_Report_Key(void)
{
    if (release_flag == 1)
        return 0;
    Memory_Report_Exit();                            /* no return: its AX is the result */
}

/* developer key: saves the displayed screen as SCRDMPnn.FLI (the first free number) - an FLI file of one frame
   (header: size, magic AF11h, 1 frame, 320 x 200 x 8, speed 5; a COLOR chunk with the 256 DAC entries and a COPY
   chunk with the 200 rows) and an empty second frame. The fopen is not checked. */
int Debug_Screenshot_FLI(void)
{
    long size;
    int w;
    unsigned n;
    char buffer[350];
    char file_name[16];
    char num_text[16];
    FILE *fp;
    unsigned i;

    if (release_flag == 1)
        return 0;
    Set_Draw_Page_Front();
    Mouse_Hide();
    n = 0;
    strcpy(file_name, "SCRDMP00.FLI");
    while (DOS_Find_File(file_name, buffer) != 0) {
        n++;
        if (n < 10) {
            strcpy(buffer, "0");
            itoa(n, num_text, 10);
            strcat(buffer, num_text);
        } else
            itoa(n, buffer, 10);
        strcpy(file_name, "SCRDMP");
        strcat(file_name, buffer);
        strcat(file_name, ".FLI");
    }
    fp = fopen(file_name, "wb");
    size = 0xFDB0L;                             /* the file: 128-byte header + this frame + the empty one */
    fwrite(&size, 4, 1, fp);
    w = FLI_MAGIC;
    fwrite(&w, 2, 1, fp);
    w = 1;
    fwrite(&w, 2, 1, fp);
    w = 320;
    fwrite(&w, 2, 1, fp);
    w = 200;
    fwrite(&w, 2, 1, fp);
    w = 8;
    fwrite(&w, 2, 1, fp);
    w = 0;
    fwrite(&w, 2, 1, fp);
    w = 5;
    fwrite(&w, 2, 1, fp);
    w = 0;
    for (i = 0; i < 55; i++)
        fwrite(&w, 2, 1, fp);
    size = 0xFD20L;                             /* the frame: 16-byte header + the two chunks */
    fwrite(&size, 4, 1, fp);
    w = FLI_FRAME_MAGIC;
    fwrite(&w, 2, 1, fp);
    w = 2;
    fwrite(&w, 2, 1, fp);
    size = 0;
    fwrite(&size, 4, 1, fp);
    fwrite(&size, 4, 1, fp);
    size = 0x30AL;                              /* COLOR: 6 + 1 packet (skip 0, count 0 = 256) + 768 */
    fwrite(&size, 4, 1, fp);
    w = FLI_COLOR;
    fwrite(&w, 2, 1, fp);
    w = 1;
    fwrite(&w, 2, 1, fp);
    w = 0;
    fwrite(&w, 2, 1, fp);
    for (i = 0; i < 4; i++) {
        VGA_Read_DAC_Block(i, buffer);
        fwrite(buffer, 0xC0, 1, fp);
    }
    size = 0xFA06L;                             /* COPY: 6 + 64000 */
    fwrite(&size, 4, 1, fp);
    w = FLI_COPY;
    fwrite(&w, 2, 1, fp);
    for (i = 0; i < 200; i++) {
        VGA_Copy_Row(i, buffer);
        fwrite(buffer, 320, 1, fp);
    }
    size = 0x10L;
    fwrite(&size, 4, 1, fp);
    w = FLI_FRAME_MAGIC;
    fwrite(&w, 2, 1, fp);
    size = 0;
    w = 0;
    fwrite(&w, 2, 1, fp);
    fwrite(&size, 4, 1, fp);
    fwrite(&size, 4, 1, fp);
    fclose(fp);
    Mouse_Show();
    Set_Draw_Page();
}

/* (block, buffer) reads 64 DAC registers from block * 64 into DS:buffer (INT 10h AX=1017h) */
void VGA_Read_DAC_Block(int block, char *buffer)
{
    int first;

    first = block << 6;
    asm push es
    asm mov ah, 10h
    asm mov al, 17h
    asm mov bx, first
    asm mov cx, 40h
    asm mov dx, ds
    asm mov es, dx
    asm mov dx, buffer
    asm int 10h
    asm pop es
}

/* (row, buffer) one row of the draw page as 320 linear bytes (reads all 4 planes of each byte column) */
void VGA_Copy_Row(int row, char *buffer)
{
    asm push si
    asm push di
    asm push ds
    asm push es
    asm mov di, buffer
    asm mov ax, row
    asm mov dx, 50h                             /* VGA_ROW_BYTES */
    asm mul dx
    asm mov si, ax
    asm mov ax, ds
    asm mov es, ax
    asm mov ax, draw_page_seg
    asm mov ds, ax
    asm mov cx, 50h
next_byte:
    asm mov al, 4
    asm mov dx, 3CEh                       /* the VGA graphics controller index */
    asm out dx, al
    asm inc dx
    asm mov al, 0
    asm out dx, al
    asm movsb
    asm dec si
    asm mov al, 4
    asm mov dx, 3CEh                       /* the VGA graphics controller index */
    asm out dx, al
    asm inc dx
    asm mov al, 1
    asm out dx, al
    asm movsb
    asm dec si
    asm mov al, 4
    asm mov dx, 3CEh                       /* the VGA graphics controller index */
    asm out dx, al
    asm inc dx
    asm mov al, 2
    asm out dx, al
    asm movsb
    asm dec si
    asm mov al, 4
    asm mov dx, 3CEh                       /* the VGA graphics controller index */
    asm out dx, al
    asm inc dx
    asm mov al, 3
    asm out dx, al
    asm movsb
    asm loop next_byte
    asm pop es
    asm pop ds
    asm pop di
    asm pop si
}

/* () marks the build as the release version (no debug output, random seeds).
   Simtex name Release_Version (MoO2 symbols, module capture); clarified using ReMoM repo */
void Release_Version(void)
{
    release_flag = 1;
}

/* () -> 1 in the release version.
   Simtex name Check_Release_Version (MoO2 symbols, module capture); clarified using ReMoM repo */
int Check_Release_Version(void)
{
    return release_flag;
}

/* (x1, y1, x2, y2, pic) copies a screen rectangle of the draw page into a new picture (pic) */
void Screen_Grab_Rect(int x1, int y1, int x2, int y2, unsigned pic)
{
    int width;
    int height;

    Set_Draw_Page();
    width = x2 - x1 + 1;
    height = y2 - y1 + 1;
    if (width < 0)
        width = -width;
    if (height < 0)
        height = -height;
    Pic_Create_Blank(width, height, pic);
    Screen_Grab_Rect_Core(pic + 1, x1, y1, x2, y2);
}

/* (dest_seg, x1, y1, x2, y2) the rectangle column by column (planar reads) into dest_seg:0, column-major */
void Screen_Grab_Rect_Core(unsigned dest_seg, int x1, int y1, int x2, int y2)
{
    int cols;
    int rows;
    int start;

    cols = x2 - x1 + 1;
    rows = y2 - y1 + 1;
    start = y1 * VGA_ROW_BYTES + x1 / 4;
    asm push ds
    asm push es
    asm push si
    asm push di
    asm mov ax, dest_seg
    asm mov es, ax
    asm mov bx, cols
    asm mov si, start
    asm mov ax, x1
    asm and al, 3
    asm mov ah, al
    asm mov di, rows
    asm mov dx, draw_page_seg
    asm mov ds, dx
    asm mov dx, di
    asm mov di, 0
next_column:
    asm push si
    asm push dx
    asm mov al, 4
    asm mov dx, 3CEh                       /* the VGA graphics controller index */
    asm out dx, al
    asm inc dx
    asm mov al, ah
    asm out dx, al
    asm pop dx
    asm mov cx, dx
next_row:
    asm lodsb
    asm stosb
    asm add si, 4Fh                             /* 80 - 1: the next row, the same byte column */
    asm loop next_row
    asm pop si
    asm inc ah
    asm cmp ah, 4
    asm js same_byte
    asm mov ah, 0
    asm inc si
same_byte:
    asm dec bx
    asm jne next_column
    asm pop di
    asm pop si
    asm pop es
    asm pop ds
}
