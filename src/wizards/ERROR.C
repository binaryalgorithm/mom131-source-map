/* ERROR.C - WIZARDS.EXE code segment 14b5 (MAGIC.EXE 1422, the same file): leaving the game - fatal error with a
   message or a number, the memory report, the DOS error screen of a message file, and Exec_Program (WIZARDS.EXE <->
   MAGIC.EXE). Every exit path shuts music, mouse, sound and EMS down and returns to text mode first.
   This module's data (DS:71ea): lowest_free_dos_kb, then its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 14b5 src/wizards/ERROR.C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloc.h>
#include <process.h>

/* Error_Exit_Message's control bytes: below 16 a color pair, 16 back to the top, 17 an empty line */
#define ERRMSG_RESTART 16
#define ERRMSG_EMPTY_LINE 17

unsigned lowest_free_dos_kb = 0xFA00;              /* lowest free DOS memory seen (KB), Mem_Track_Min_Free; starts at
                                                   64000, above any real figure */

extern int ems_reserved_pages;                  /* DS:7628 */

unsigned DOS_Free_KB(void);
long DOS_File_Size(char *name);
int Music_Release_Sequence(void);
void Mouse_Shutdown(void);
void Sound_Shutdown(void);
void EMS_Free_All(void);
void EMS_Free_Reserved(void);
void Error_Hook_Exit(void);
void Video_Text_Mode(void);
void Text_Add_CRLF_Print(char *text);
int VGA_Front_Page_Check(void);
void Screen_Copy_Front(void);
void UI_Present_Frame(void);
void Fatal_Exit(char *message);
void Error_Text_Print(int x, int y, int fg, int bg, char *text, unsigned text_seg);

/* () "Data Free: <coreleft> bytes   Memory Free: <n>k   Worst Free: <n>k   EMM: <n> blocks", then Fatal_Exit */
void Memory_Report_Exit(void)
{
    int free_kb;
    int min_kb;
    unsigned core_left;
    char message[120];
    char num_text[20];

    free_kb = DOS_Free_KB();
    min_kb = lowest_free_dos_kb;
    core_left = coreleft();
    strcpy(message, "Data");
    strcat(message, " Free: ");
    ltoa(core_left, num_text, 10);
    strcat(message, num_text);
    strcat(message, " bytes   Memory");
    strcat(message, " Free: ");
    itoa(free_kb, num_text, 10);
    strcat(message, num_text);
    strcat(message, "k   Worst");
    strcat(message, " Free: ");
    itoa(min_kb, num_text, 10);
    strcat(message, num_text);
    strcat(message, "k   EMM: ");
    itoa(ems_reserved_pages, num_text, 10);
    strcat(message, num_text);
    strcat(message, " blocks");
    Fatal_Exit(message);
}

/* (message) ends the program: music, mouse, sound and EMS released, the error hooks removed, text mode,
   the message printed */
void Fatal_Exit(char *message)
{
    Music_Release_Sequence();
    Mouse_Shutdown();
    Sound_Shutdown();
    EMS_Free_All();
    Error_Hook_Exit();
    Video_Text_Mode();
    Text_Add_CRLF_Print(message);
}

/* (code) the same with the number as the message */
void Error_Exit_Code(int code)
{
    char num_text[10];

    itoa(code, num_text, 10);
    Music_Release_Sequence();
    Mouse_Shutdown();
    Sound_Shutdown();
    EMS_Free_All();
    Error_Hook_Exit();
    Video_Text_Mode();
    Text_Add_CRLF_Print(num_text);
}

/* (path, arg0, arg1, arg2) shuts everything down, closes DOS handles 5..29 and runs path; on return (the exec
   failed) "Error- game file <path> could not be found" */
void Exec_Program(char *path, char *arg0, char *arg1, char *arg2)
{
    int handle;
    char message[80];

    Music_Release_Sequence();
    if (VGA_Front_Page_Check() == 0) {
        Screen_Copy_Front();
        UI_Present_Frame();
    }
    Mouse_Shutdown();
    Sound_Shutdown();
    fcloseall();
    for (handle = 5; handle < 30; handle++) {   /* every DOS handle past the five standard ones */
        asm mov ah, 3Eh
        asm mov bx, handle
        asm int 21h
    }
    EMS_Free_Reserved();
    execl(path, path, arg0, arg1, arg2, NULL);
    strcpy(message, "Error- game file ");
    strcat(message, path);
    strcat(message, " could not be found");
    Fatal_Exit(message);
}

/* () keeps the lowest free DOS memory seen (KB) */
void Mem_Track_Min_Free(void)
{
    unsigned free_kb;

    free_kb = DOS_Free_KB();
    if (free_kb < lowest_free_dos_kb)
        lowest_free_dos_kb = free_kb;
}

/* (file) the DOS error screen: the file holds 82-byte records {int x; char line[80]}; a line whose first byte is
   below 16 sets the colors (fg, bg) and does not advance, 16 restarts at the top, 17 leaves an empty line, anything else is
   printed at (x, y) in text mode. Then 20 empty lines and the exit..
   Simtex name Exit_With_Message (MoO2 symbols, module exit); clarified using ReMoM repo */
void Error_Exit_Message(char *file)
{
    long size;
    int records;
    int x;
    int fg;
    int bg;
    FILE *fp;
    char line[100];
    register int y;
    register int i;

    Music_Release_Sequence();
    Mouse_Shutdown();
    Sound_Shutdown();
    EMS_Free_All();
    Error_Hook_Exit();
    Video_Text_Mode();
    size = DOS_File_Size(file);
    records = size / 82;
    fp = fopen(file, "rb");
    y = 0;
    fg = 15;
    bg = 0;
    for (i = 0; i < records; i++) {
        fread(&x, 2, 1, fp);
        fread(line, 80, 1, fp);
        if (line[0] < ERRMSG_RESTART) {
            fg = line[0];
            bg = line[1];
            y--;
        } else if (line[0] == ERRMSG_RESTART)
            y = -1;
        else if (line[0] == ERRMSG_EMPTY_LINE)
            ;
        else
            Error_Text_Print(x, y, fg, bg, line, 0);
        y++;
    }
    fclose(fp);
    Text_Add_CRLF_Print("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
}

/* (x, y, fg, bg, text, seg) writes text into the text screen at B800:(y * 160 + x * 2), attribute bg * 16 + fg;
   a segment of 0 means DGROUP */
void Error_Text_Print(int x, int y, int fg, int bg, char *text, unsigned text_seg)
{
    int attr;

    if (text_seg == 0) {
        asm mov ax, ds
        asm mov text_seg, ax
    }
    asm mov ax, bg
    asm shl ax, 1
    asm shl ax, 1
    asm shl ax, 1
    asm shl ax, 1
    asm add ax, fg
    asm mov attr, ax
    asm mov ax, y
    asm shl ax, 1
    asm shl ax, 1
    asm add ax, y
    asm shl ax, 1
    asm add ax, 0B800h
    asm mov es, ax
    asm mov di, x
    asm shl di, 1
    asm push ds
    asm mov si, text
    asm mov bx, attr
    asm mov ax, text_seg
    asm mov ds, ax
next_char:
    asm lodsb
    asm cmp al, 0
    asm je done
    asm stosb
    asm mov al, bl
    asm stosb
    asm jmp next_char
done:
    asm pop ds
}
