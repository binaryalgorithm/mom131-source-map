/* DOSFILE.C - WIZARDS.EXE code segment 169a (MAGIC.EXE 1607, the same file): an fopen / fread / fwrite / fseek /
   fclose stand-in on DOS handles for the save and settings files. ONE file at a time: the handle is the global
   dos_file_handle. C with inline assembly for the INT 21h calls.
   DOS_File_Open returns AX of INT 21h without testing the carry: a failed open returns the DOS error code as if it
   were a handle (BUG #334).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 169a src/wizards/DOSFILE.C */
#include <string.h>

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int dos_file_handle;                            /* DS:e5e8  MAGIC DS:a5de   */
int dos_io_bytes;                               /* DS:e5e6  MAGIC DS:a5dc   */


/* (path, mode) opens through DOS (INT 21h): "r" / "rb" read, "w" / "wb" create, anything else read-write;
   the handle is kept in dos_file_handle (one file at a time) and returned (the DOS error code on failure) */
int DOS_File_Open(char *path, char *mode)
{
    if (stricmp(mode, "r") == 0 || stricmp(mode, "rb") == 0) {
        asm mov ax, 3D00h
        asm mov dx, path
        asm int 21h
        asm mov dos_file_handle, ax
    } else if (stricmp(mode, "w") == 0 || stricmp(mode, "wb") == 0) {
        asm mov ax, 3C00h
        asm mov cx, 0
        asm mov dx, path
        asm int 21h
        asm mov dos_file_handle, ax
    } else {
        asm mov ax, 3D02h
        asm mov dx, path
        asm int 21h
        asm mov dos_file_handle, ax
    }
    return dos_file_handle;
}

/* (buf, buf_seg (0 = DS), size, count) reads size x count bytes from the open file; no return value */
int DOS_File_Read_Far(unsigned buf, unsigned buf_seg, int size, int count)
{
    dos_io_bytes = size * count;
    if (buf_seg == 0) {
        asm mov ax, ds
        asm mov buf_seg, ax
    }
    asm push ds
    asm mov bx, dos_file_handle
    asm mov cx, dos_io_bytes
    asm mov ax, buf_seg
    asm mov ds, ax
    asm mov dx, buf
    asm mov ah, 3Fh
    asm int 21h
    asm pop ds
}

/* (buf, buf_seg (0 = DS), size, count) writes size x count bytes to the open file; no return value */
int DOS_File_Write_Far(unsigned buf, unsigned buf_seg, int size, int count)
{
    dos_io_bytes = size * count;
    if (buf_seg == 0) {
        asm mov ax, ds
        asm mov buf_seg, ax
    }
    asm push ds
    asm mov bx, dos_file_handle
    asm mov cx, dos_io_bytes
    asm mov ax, buf_seg
    asm mov ds, ax
    asm mov dx, buf
    asm mov ah, 40h
    asm int 21h
    asm pop ds
}

/* (ignored, pos, whence 0 / 1 / 2) moves the open file's position; no return value */
int DOS_File_Seek(int unused_fp, long pos, int whence)
{
    asm mov ax, whence
    asm mov ah, 42h
    asm mov bx, dos_file_handle
    asm mov dx, word ptr pos
    asm mov cx, word ptr pos + 2
    asm int 21h
}

/* () closes the open file */
void DOS_File_Close(void)
{
    asm mov ah, 3Eh
    asm mov bx, dos_file_handle
    asm int 21h
}
