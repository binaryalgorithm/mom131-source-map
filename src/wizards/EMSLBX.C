/* EMSLBX.C - WIZARDS.EXE code segment 16d5 (MAGIC.EXE 1643, the same file): the EMS side of the LBX loader. At start
   the game claims EMS (fatal without it), finds or creates its marker handle "YO MOMA", and adopts every named
   handle a previous run left behind; whole LBX files (or single entries / ranges) are copied into named EMS handles
   (table ems_lbx_table, 40 entries); the LBX loader then reads entries from EMS instead of disk. Also the screen
   backup in EMS, the blocks carved out of the EMS page frame, and the "You must have at least ..K of expanded
   memory" message. C with a little inline assembly (INT 67h).
   Does NOT include GAME.H: LBX_Error is called with 3 and 4 arguments and the EMS reads pass ints where GAME.H
   would want char pointers (no prototypes in the original).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 16d5 src/wizards/EMSLBX.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     EMS_LBX_Load_Entry_Data (6; its definition below takes 5);
     EMS_Named_Handle (2; its definition below takes 3). */
#pragma warn -pro
#include <stdlib.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

extern unsigned ems_frame_seg;                  /* DS:760c */
extern char near_heap[];                        /* DS:d49a  0x1130 bytes; the first 0x100 are a text scratch */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int ems_required_k;                             /* DS:e7dc  MAGIC DS:a7d2   */
struct EmsLbxEntry ems_lbx_table[EMS_LBX_TABLE_SIZE];           /* DS:e5fc  MAGIC DS:a5f2   */
int ems_available;                              /* DS:e5fa  MAGIC DS:a5f0   */
int ems_yomoma_handle;                          /* DS:e5f8  MAGIC DS:a5ee   */
int ems_lbx_found_reserved;                     /* DS:e5f6  MAGIC DS:a5ec   */
int ems_vgafile_handle;                          /* DS:e5f4  MAGIC DS:a5ea   */
                                                /* the "VGAFILEH" EMS handle, 5 pages: 0-3 the animation stream / 64 KB scratch backup, 4 (at 64 KB) the screen backup */
int ems_frame_handle_spare;                     /* DS:e5f2  MAGIC DS:a5e8  no code in either program reads it */
int ems_frame_handle;                           /* DS:e5f0  MAGIC DS:a5e6   */
unsigned ems_frame_top;                         /* DS:e5ee  MAGIC DS:a5e4   */
int ems_main_handle;                            /* DS:e5ec  MAGIC DS:a5e2   */
unsigned ems_frame_mark;                        /* DS:e5ea  MAGIC DS:a5e0   */

int ems_reserved_pages = 40;                    /* DS:7628 */
int ems_lbx_count = 0;                          /* DS:762a */
char ems_marker_name[] = "YO MOMA";             /* DS:762c */

long DOS_File_Size(char *name);
int EMS_Cache_LBX_File(char *name, int required);
void EMS_Cache_LBX_Entry(char *lbx, int entry, int mode, char *handle_name);
void EMS_Cache_LBX_Range_Core(char *lbx, int entry, int mode, char *handle_name, long offset, long max_len);
int EMS_LBX_Find_Handle(char *name);
void EMS_Map_Default(void);
void EMS_Required_Message(char *buf);
int EMS_LBX_Load_Entry_Data();                 /* no prototype here: one call passes an extra word the routine never reads */
int EMS_Named_Handle();                        /* no prototype here: the dead EMS_Cache_LBX_Range_Core passes 2 of 3 words (#374 (latent)) */

/* () fatal without EMS; adopts the "YO MOMA" handle and the named handles after it (LBX files cached by a previous
   run), checks the reserve, opens the VGAFILEH and EMMDATAH handles */
void EMS_Startup(void)
{
    int handle_count;
    char message[100];
    int i;

    if (EMS_Detect()) {
        handle_count = EMS_Handle_Count();
        ems_yomoma_handle = 0;
        for (i = 1; i < handle_count && ems_yomoma_handle == 0; i++)
            if (EMS_Get_Handle_Name(message, i) != 0 && stricmp(ems_marker_name, message) == 0)
                ems_yomoma_handle = i;
        ems_lbx_count = 0;
        if (ems_yomoma_handle == 0)
            ems_yomoma_handle = EMS_Named_Handle(1, "YO MOMA\0\0", 0);   /* padded to the 8-byte EMS name */
        else {
            strcpy(ems_lbx_table[ems_lbx_count].name, ems_marker_name);
            ems_lbx_table[ems_lbx_count].in_reserved_pages = 0;
            ems_lbx_table[ems_lbx_count].handle = ems_yomoma_handle;
            ems_lbx_count = 1;
        }
        if (ems_yomoma_handle <= 0) {
            EMS_Required_Message(message);
            Fatal_Exit(message);
        }
        ems_available = 1;
        for (i = ems_yomoma_handle + 1; i < 50 && ems_lbx_count < EMS_LBX_TABLE_SIZE; i++)   /* the handles after YO MOMA, up to 49 */
            if (EMS_Get_Handle_Name(message, i) != 0 && message[0] != 0) {
                ems_lbx_table[ems_lbx_count].in_reserved_pages = 0;
                strcpy(ems_lbx_table[ems_lbx_count].name, message);
                ems_lbx_table[ems_lbx_count].handle = i;
                ems_lbx_count++;
            }
        if (EMS_Free_Pages() < ems_reserved_pages) {
            EMS_Required_Message(message);
            Fatal_Exit(message);
        }
        ems_vgafile_handle = EMS_Named_Handle(5, "VGAFILEH", 1);
        ems_frame_handle = EMS_Named_Handle(4, "EMMDATAH", 1);
        ems_frame_top = 0;
    } else {
        EMS_Required_Message(message);
        Fatal_Exit(message);
    }
}

/* (name) no callers */
void EMS_Cache_LBX_File_Optional(char *name)
{
    EMS_Cache_LBX_File(name, 0);
}

/* (name) the start-up preloads */
void EMS_Cache_LBX_File_Required(char *name)
{
    EMS_Cache_LBX_File(name, 1);
}

/* (name, required) copies a whole LBX file into a named EMS handle (16 KB pages); a not-required load keeps
   ems_reserved_pages free. Returns the handle (0: no room). QUIRK #459: the handle is not set when EMS is missing, the
   table is full or the file is empty - the function then returns an uninitialised local */
int EMS_Cache_LBX_File(char *name, int required)
{
    int fh;
    int handle;
    int cached;
    unsigned pages;
    unsigned frame;
    unsigned chunk;
    int page;
    long size;
    long pos;
    char path[20];
    char base_name[20];
    int i;
    int j;

    LBX_Normalize_Name(name);
    strcpy(base_name, name);
    for (i = 0; base_name[i] != 0; i++)
        ;
    for (j = i; j < 9; j++)                     /* an EMS name: 8 characters + NUL */
        base_name[j] = 0;
    cached = 0;
    for (i = 0; i < ems_lbx_count; i++)
        if (stricmp(ems_lbx_table[i].name, base_name) == 0)
            cached = 1;
    if (cached != 0)
        LBX_Error(base_name, LBX_ERR_EMS_NOT_RELEASED, 0);
    if (ems_available != 0 && ems_lbx_count < EMS_LBX_TABLE_SIZE) {
        strcpy(path, base_name);
        strcat(path, ".LBX");
        size = DOS_File_Size(path);
        if (size != 0) {
            pages = size / EMS_PAGE_SIZE;
            if (size % EMS_PAGE_SIZE != 0)
                pages++;
            if (required == 0 && EMS_Free_Pages() < ems_reserved_pages + pages)
                return 0;
            handle = EMS_Named_Handle(pages, base_name, required);
            if (handle != 0) {
                frame = EMS_Page_Frame();
                page = 0;
                if (frame != 0) {
                    fh = DOS_Open_Read(path);
                    if (fh != 0) {
                        pos = 0;
                        chunk = EMS_PAGE_SIZE;
                        while (size >= EMS_PAGE_SIZE) {
                            size -= EMS_PAGE_SIZE;
                            EMS_Map_Handle_Pages(handle, page);
                            DOS_Read_To_Segment(frame, chunk, fh);
                            page++;
                            pos += EMS_PAGE_SIZE;
                        }
                        if (size > 0) {
                            chunk = size;
                            EMS_Map_Handle_Pages(handle, page);
                            DOS_Read_To_Segment(frame, chunk, fh);
                        }
                        DOS_Close(fh);
                    }
                }
            } else if (required == 1)
                LBX_Error(name, LBX_ERR_EMS_RESERVED_LOAD, 0);
        }
    }
    return handle;
}

/* (lbx, entry, handle_name) no callers */
void EMS_Cache_LBX_Named(char *lbx, int entry, char *handle_name)
{
    EMS_Cache_LBX_Entry(lbx, entry, 0, handle_name);
}

/* (lbx, entry, handle_name) no callers */
void EMS_Cache_LBX_Named_Reserved(char *lbx, int entry, char *handle_name)
{
    EMS_Cache_LBX_Entry(lbx, entry, 1, handle_name);
}

/* (lbx, entry, mode, handle_name) DEAD: copies one LBX entry into a new named EMS handle. QUIRKS: the last partial
   page reads (bytes left % 16), not the bytes left, and the file is never closed */
void EMS_Cache_LBX_Entry(char *lbx, int entry, int mode, char *handle_name)
{
    int fh;
    int handle;
    int cached;
    int page;
    unsigned pages;
    unsigned frame;
    unsigned chunk;
    long left;
    long size;
    long offset;
    char lbx_name[20];
    char name[20];
    int i;
    int j;

    LBX_Normalize_Name(lbx);
    strcpy(lbx_name, lbx);
    strcpy(name, handle_name);
    for (i = 0; name[i] != 0; i++)
        ;
    for (j = i; j < 9; j++)                     /* an EMS name: 8 characters + NUL */
        name[j] = 0;
    cached = 0;
    for (i = 0; i < ems_lbx_count; i++)
        if (stricmp(ems_lbx_table[i].name, name) == 0)
            cached = 1;
    if (cached != 0)
        LBX_Error(lbx_name, LBX_ERR_EMS_NOT_RELEASED, 0);
    if (ems_available != 0 && ems_lbx_count < EMS_LBX_TABLE_SIZE) {
        fh = LBX_Entry_Locate(lbx_name, entry, &offset, &size, mode);
        if (DOS_Seek(offset, fh) == 0)
            LBX_Error(lbx_name, LBX_ERR_CORRUPT, entry);
        pages = size / EMS_PAGE_SIZE;
        if (size % EMS_PAGE_SIZE != 0)
            pages++;
        handle = EMS_Named_Handle(pages, name, 0);
        if (handle == 0)
            LBX_Error(lbx_name, LBX_ERR_EMS_FULL, 0);
        frame = EMS_Page_Frame();
        page = 0;
        left = size;
        chunk = EMS_PAGE_SIZE;
        while (left >= EMS_PAGE_SIZE) {
            left -= EMS_PAGE_SIZE;
            EMS_Map_Handle_Pages(handle, page);
            DOS_Read_To_Segment(frame, chunk, fh);
            page++;
        }
        if (left > 0) {
            chunk = left % 16;
            EMS_Map_Handle_Pages(handle, page);
            DOS_Read_To_Segment(frame, chunk, fh);
        }
    }
}

/* (lbx, entry, handle_name, offset, max_len) no callers */
void EMS_Cache_LBX_Range(char *lbx, int entry, char *handle_name, long offset, long max_len)
{
    EMS_Cache_LBX_Range_Core(lbx, entry, 0, handle_name, offset, max_len);
}

/* (lbx, entry, handle_name, offset, max_len) no callers */
void EMS_Cache_LBX_Range_Reserved(char *lbx, int entry, char *handle_name, long offset, long max_len)
{
    EMS_Cache_LBX_Range_Core(lbx, entry, 1, handle_name, offset, max_len);
}

/* (lbx, entry, mode, handle_name, offset, max_len) DEAD: as EMS_Cache_LBX_Entry for the part of the entry from offset,
   at most max_len bytes (error 2 when offset is past the end)
   Numbered: BUG #374 (BUGS_AND_QUIRKS.txt) */
void EMS_Cache_LBX_Range_Core(char *lbx, int entry, int mode, char *handle_name, long offset, long max_len)
{
    int fh;
    int handle;
    int cached;
    int page;
    unsigned pages;
    unsigned frame;
    unsigned chunk;
    long left;
    long size;
    long pos;
    char lbx_name[20];
    char name[20];
    int i;
    int j;

    LBX_Normalize_Name(lbx);
    strcpy(lbx_name, lbx);
    strcpy(name, handle_name);
    for (i = 0; name[i] != 0; i++)
        ;
    for (j = i; j < 9; j++)                     /* an EMS name: 8 characters + NUL */
        name[j] = 0;
    cached = 0;
    for (i = 0; i < ems_lbx_count; i++)
        if (stricmp(ems_lbx_table[i].name, name) == 0)
            cached = 1;
    if (cached != 0)
        LBX_Error(lbx_name, LBX_ERR_EMS_NOT_RELEASED, 0);
    if (ems_available != 0 && ems_lbx_count < EMS_LBX_TABLE_SIZE) {
        fh = LBX_Entry_Locate(lbx_name, entry, &pos, &size, mode);
        pos += offset;
        size -= offset;
        if (size <= 0)
            LBX_Error(lbx_name, LBX_ERR_CORRUPT, entry);
        if (max_len < size)
            size = max_len;
        if (DOS_Seek(pos, fh) == 0)
            LBX_Error(lbx_name, LBX_ERR_CORRUPT, entry);
        pages = size / EMS_PAGE_SIZE;
        if (size % EMS_PAGE_SIZE != 0)
            pages++;
        handle = EMS_Named_Handle(pages, name);
        if (handle == 0)
            LBX_Error(lbx_name, LBX_ERR_EMS_FULL, 0);
        frame = EMS_Page_Frame();
        page = 0;
        left = size;
        chunk = EMS_PAGE_SIZE;
        while (left >= EMS_PAGE_SIZE) {
            left -= EMS_PAGE_SIZE;
            EMS_Map_Handle_Pages(handle, page);
            DOS_Read_To_Segment(frame, chunk, fh);
            page++;
        }
        if (left > 0) {
            chunk = left;
            EMS_Map_Handle_Pages(handle, page);
            DOS_Read_To_Segment(frame, chunk, fh);
        }
    }
}

/* (lbx, entry, block, mode, alt_table) LBX_Load_Core for an LBX cached in EMS: the entry's offsets from the table at +8
   (alt_table: the second table at +208h), a heap block by mode (0 new, 1 into block, 2 appended to block), then the data
   in 32 KB reads. Reserved-page LBX files go through EMS_LBX_Load_Entry_Data (only the header).
   Returns the data segment, 0 on failure. QUIRKS: the paragraph count rounds up when (paragraphs & 15) instead of
   (bytes & 15); mode 2 tests Heap_Block_Valid(block == 0) (a misplaced parenthesis: meant Heap_Block_Valid(block) == 0); a length of exactly n * 32 KB loses its last 32 KB
   Numbered: BUG #335 (BUGS_AND_QUIRKS.txt) */
int EMS_LBX_Load_Entry(char *lbx, int entry, unsigned block, int mode, int alt_table)
{
    unsigned result;
    int handle;
    int n_entries;
    int lbx_type;                               /* the LBX header's file type (word +6): 0 pictures */
    unsigned chunk;
    unsigned dest;
    unsigned paragraphs;
    long pos;
    long len;
    long table_pos;

    result = 0;
    handle = EMS_LBX_Find_Handle(lbx);
    if (handle != 0) {
        if (ems_lbx_found_reserved == 1) {
            EMS_Read(&lbx_type, 0, 6L, 2, handle);
            if (lbx_type != 0)
                LBX_Error(lbx, LBX_ERR_EMS_PICTURES_ONLY, entry);
            else
                result = EMS_LBX_Load_Entry_Data(handle, lbx, entry, block, mode, alt_table);
        } else {
            EMS_Read(&n_entries, 0, 0, 0, 2, handle);   /* 0, 0: the long offset 0 */
            if (entry >= n_entries)
                return 0;
            if (alt_table == 0)
                table_pos = ((long)entry << 2) + LBX_OFFSET_TABLE;
            else
                table_pos = ((long)entry << 2) + 0x208;   /* the second header's table (0x200 + 8) */
            EMS_Read(&pos, 0, table_pos, 4, handle);
            EMS_Read(&len, 0, table_pos + 4, 4, handle);
            len -= pos;
            if (len > 0) {
                paragraphs = len / 16;
                if (paragraphs & 0xf)
                    paragraphs++;
                switch (mode) {
                case 0:
                    result = Heap_Alloc_Raw(paragraphs);
                    if (result == 0)
                        return 0;
                    break;
                case 1:
                    if (Heap_Block_Valid(block) == 0)
                        return 0;
                    if (Far_Peek_Word(block, HEAP_HDR_SIZE) - 1 < paragraphs)
                        return 0;
                    result = block + 1;
                    Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + 1);
                    break;
                case 2:
                    if (Heap_Block_Valid(block == 0))
                        return 0;
                    if (Heap_Free_Paragraphs(block) < paragraphs)
                        return 0;
                    result = block + Far_Peek_Word(block, HEAP_HDR_USED);
                    Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + Far_Peek_Word(block, HEAP_HDR_USED));
                    break;
                }
                if (result != 0) {
                    dest = result;
                    chunk = LBX_READ_CHUNK;
                    while (len > LBX_READ_CHUNK) {
                        len -= LBX_READ_CHUNK;
                        EMS_Read(0, dest, pos, chunk, handle);
                        pos += LBX_READ_CHUNK;
                        dest += LBX_READ_CHUNK_PARAS;
                    }
                    if (len > 0) {
                        chunk = len % LBX_READ_CHUNK;
                        EMS_Read(0, dest, pos, chunk, handle);
                    }
                }
            }
        }
    }
    return result;
}

/* (handle, lbx, entry, block, mode) an entry of an LBX in reserved EMS pages stays in EMS: only its first 16 bytes
   go into a 1-paragraph heap block, with the EMS handle (+0Ah), page (+0Bh) and offset in the page (+0Ch)
   Numbered: BUG #335 (BUGS_AND_QUIRKS.txt) */
int EMS_LBX_Load_Entry_Data(int handle, char *lbx, int entry, unsigned block, int mode)
{
    int n_entries;
    unsigned paragraphs;
    unsigned ofs;
    int page;
    long pos;
    long table_pos;
    unsigned result;

    result = 0;
    EMS_Read(&n_entries, 0, 0, 0, 2, handle);
    if (entry >= n_entries)
        LBX_Error(lbx, LBX_ERR_NO_SUCH_ENTRY, entry);
    table_pos = ((long)entry << 2) + LBX_OFFSET_TABLE;
    EMS_Read(&pos, 0, table_pos, 4, handle);
    paragraphs = 1;
    switch (mode) {
    case 0:
        result = Heap_Alloc_Raw(paragraphs);
        if (result == 0)
            LBX_Error(lbx, LBX_ERR_NO_MEMORY, entry);
        break;
    case 1:
        if (Heap_Block_Valid(block) == 0)
            LBX_Error(lbx, LBX_ERR_BAD_BLOCK, entry);
        if (Far_Peek_Word(block, HEAP_HDR_SIZE) - 1 < paragraphs)
            LBX_Error(lbx, LBX_ERR_BLOCK_TOO_SMALL, entry, 1);
        result = block + 1;
        Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + 1);
        break;
    case 2:
        if (Heap_Block_Valid(block == 0))
            LBX_Error(lbx, LBX_ERR_BAD_BLOCK, entry);
        if (Heap_Free_Paragraphs(block) < paragraphs)
            LBX_Error(lbx, LBX_ERR_BLOCK_TOO_SMALL, entry, 1);
        result = block + Far_Peek_Word(block, HEAP_HDR_USED);
        Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + Far_Peek_Word(block, HEAP_HDR_USED));
        break;
    }
    page = pos / EMS_PAGE_SIZE;
    ofs = pos % EMS_PAGE_SIZE;
    EMS_Read(0, result, pos, 16, handle);
    Far_Poke_Byte(result, PIC_EMS_HANDLE, handle);
    Far_Poke_Byte(result, PIC_EMS_PAGE, page);
    Far_Poke_Word(result, PIC_EMS_OFFSET, ofs);
    return result;
}

/* (lbx, entry, block, mode, first, count, recsize) LBX_Load_Records_Core for an EMS-cached LBX: records first ..
   first + count - 1 of a record entry (header: count, size) into a heap block. Returns the segment, 0 on failure
   Numbered: BUG #335 (BUGS_AND_QUIRKS.txt) */
int EMS_LBX_Load_Records(char *lbx, int entry, unsigned block, int mode, int first, int count, int recsize)
{
    unsigned result;
    int n_entries;
    unsigned chunk;
    unsigned dest;
    unsigned paragraphs;
    unsigned rec_size;
    unsigned rec_count;
    long pos;
    long len;
    long table_pos;
    int handle;

    result = 0;
    handle = EMS_LBX_Find_Handle(lbx);
    if (handle != 0) {
        EMS_Read(&n_entries, 0, 0, 0, 2, handle);
        if (entry >= n_entries)
            return 0;
        table_pos = ((long)entry << 2) + LBX_OFFSET_TABLE;
        EMS_Read(&pos, 0, table_pos, 4, handle);
        EMS_Read(&len, 0, table_pos + 4, 4, handle);
        len -= pos;
        if (len > 0) {
            table_pos = pos;
            EMS_Read(&rec_count, 0, table_pos, 2, handle);
            EMS_Read(&rec_size, 0, table_pos + 2, 2, handle);
            if (rec_size == 0 || recsize != rec_size)
                return 0;
            if (first + count > rec_count)
                return 0;
            pos += (long)first * rec_size + 4;
            len = (long)count * rec_size;
            paragraphs = len / 16 + 1;
            switch (mode) {
            case 0:
                result = Heap_Alloc_Raw(paragraphs);
                if (result == 0)
                    return 0;
                break;
            case 1:
                if (Heap_Block_Valid(block) == 0)
                    return 0;
                if (Far_Peek_Word(block, HEAP_HDR_SIZE) - 1 < paragraphs)
                    return 0;
                result = block + 1;
                Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + 1);
                break;
            case 2:
                if (Heap_Block_Valid(block == 0))
                    return 0;
                if (Heap_Free_Paragraphs(block) < paragraphs)
                    return 0;
                result = block + Far_Peek_Word(block, HEAP_HDR_USED);
                Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + Far_Peek_Word(block, HEAP_HDR_USED));
                break;
            }
            if (result != 0) {
                dest = result;
                chunk = LBX_READ_CHUNK;
                while (len > LBX_READ_CHUNK) {
                    len -= LBX_READ_CHUNK;
                    EMS_Read(0, dest, pos, chunk, handle);
                    pos += LBX_READ_CHUNK;
                    dest += LBX_READ_CHUNK_PARAS;
                }
                if (len > 0) {
                    chunk = len % LBX_READ_CHUNK;
                    EMS_Read(0, dest, pos, chunk, handle);
                }
            }
        }
    }
    return result;
}

/* (lbx, entry, dest, first, count, recsize) LBX_Load_Records_Near for an EMS-cached LBX: the records straight into
   a near buffer (at most 32 KB). Returns -1, or 0 if not cached / the records do not fit */
int EMS_LBX_Load_Records_Near(char *lbx, int entry, char *dest, int first, int count, int recsize)
{
    int result;
    int n_entries;
    unsigned bytes;
    unsigned paragraphs;
    unsigned rec_size;
    unsigned rec_count;
    long pos;
    long len;
    long table_pos;
    int handle;

    result = 0;
    handle = EMS_LBX_Find_Handle(lbx);
    if (handle != 0) {
        EMS_Read(&n_entries, 0, 0, 0, 2, handle);
        if (entry >= n_entries)
            return 0;
        table_pos = ((long)entry << 2) + LBX_OFFSET_TABLE;
        EMS_Read(&pos, 0, table_pos, 4, handle);
        EMS_Read(&len, 0, table_pos + 4, 4, handle);
        len -= pos;
        if (len > 0) {
            table_pos = pos;
            EMS_Read(&rec_count, 0, table_pos, 2, handle);
            EMS_Read(&rec_size, 0, table_pos + 2, 2, handle);
            if (rec_size == 0 || recsize != rec_size)
                return 0;
            if (first + count > rec_count)
                return 0;
            pos += (long)first * rec_size + 4;
            len = (long)count * rec_size;
            paragraphs = len / 16 + 1;
            bytes = len % LBX_READ_CHUNK;
            EMS_Read(dest, 0, pos, bytes, handle);
        }
    } else
        return 0;
    return -1;
}

/* (name) -> the EMS handle of a cached LBX (0: not cached); sets ems_lbx_found_reserved */
int EMS_LBX_Find_Handle(char *name)
{
    int handle;
    int i;

    handle = 0;
    for (i = 0; i < ems_lbx_count && handle == 0; i++)
        if (stricmp(ems_lbx_table[i].name, name) == 0) {
            handle = ems_lbx_table[i].handle;
            ems_lbx_found_reserved = ems_lbx_table[i].in_reserved_pages;
        }
    return handle;
}

/* () frees every cached LBX handle and the main handle (exit / error paths) */
void EMS_Free_All(void)
{
    int i;

    for (i = 0; i < ems_lbx_count; i++)
        EMS_Free_Handle(ems_lbx_table[i].handle);
    if (ems_main_handle != 0)
        EMS_Free_Handle(ems_main_handle);
}

/* () frees the reserved-page handles (before Exec_Program) */
void EMS_Free_Reserved(void)
{
    int i;

    for (i = 0; i < ems_lbx_count; i++)
        if (ems_lbx_table[i].in_reserved_pages == 1)
            EMS_Free_Handle(ems_lbx_table[i].handle);
}

/* (name) DEAD: frees a cached LBX when <name>.LBX exists on disk. QUIRK #460: the table compaction starts at the EMS
   HANDLE number instead of the table index */
void EMS_LBX_Uncache(char *name)
{
    char path[20];
    char file_name[30];
    int i;
    int handle;

    strcpy(file_name, name);
    LBX_Normalize_Name(file_name);
    handle = EMS_LBX_Find_Handle(file_name);
    strcpy(path, file_name);
    strcat(path, ".LBX");
    if (handle != 0 && DOS_Find_File(path, file_name) != 0) {
        if (ems_lbx_found_reserved == 1)
            ems_reserved_pages += EMS_Handle_Pages(handle);
        EMS_Free_Handle(handle);
        for (i = handle; ems_lbx_count - 1 > i; i++) {
            strcpy(ems_lbx_table[i].name, ems_lbx_table[i + 1].name);
            ems_lbx_table[i].handle = ems_lbx_table[i + 1].handle;
            ems_lbx_table[i].in_reserved_pages = ems_lbx_table[i + 1].in_reserved_pages;
        }
        ems_lbx_count--;
    }
}

/* () no callers */
int EMS_Is_Available(void)
{
    return ems_available;
}

/* (pages, name, reserved) -> the named EMS handle: the table entry of that name, else a new handle (a reserved one
   counts against ems_reserved_pages: "EMM reserved exceeded by N blocks [name.LBX]" fatal). An existing handle
   smaller than pages is fatal ("name reloaded into EMM, diff size = ..."). Returns 0 for an empty name or when the
   allocation fails */
int EMS_Named_Handle(int pages, char *name, int reserved)
{
    int i;
    int found;
    char num_text[6];
    int handle;

    if (*name == 0)
        return 0;
    found = -1;
    for (i = 0; i < ems_lbx_count; i++)
        if (stricmp(name, ems_lbx_table[i].name) == 0)
            found = i;
    if (found == -1) {
        strcpy(ems_lbx_table[ems_lbx_count].name, name);
        if (reserved == 1)
            ems_lbx_table[ems_lbx_count].name[0] = -ems_lbx_table[ems_lbx_count].name[0];   /* the
                                                   handle's EMS name gets a negated first byte (the table keeps
                                                   the plain name), so a later start-up scan does not take it
                                                   for that LBX (EMS_Startup adopts the named handles after YO MOMA by
                                                   their names) */
        handle = EMS_Alloc_Named(pages, ems_lbx_table[ems_lbx_count].name);
        if (handle != 0) {
            strcpy(ems_lbx_table[ems_lbx_count].name, name);
            ems_lbx_table[ems_lbx_count].in_reserved_pages = reserved;
            ems_lbx_table[ems_lbx_count].handle = handle;
            ems_lbx_count++;
            if (reserved == 1)
                ems_reserved_pages -= pages;
            if (ems_reserved_pages < 0) {
                itoa(-ems_reserved_pages, num_text, 10);
                strcpy(near_heap, "EMM reserved exceeded by ");
                strcat(near_heap, num_text);
                strcat(near_heap, " blocks [");
                strcat(near_heap, name);
                strcat(near_heap, ".LBX]");
                Fatal_Exit(near_heap);
            }
        }
    } else {
        handle = ems_lbx_table[found].handle;
        if (EMS_Handle_Pages(handle) < pages) {
            itoa(ems_lbx_table[found].handle, near_heap + 100, 10);   /* number strings for the message */
            itoa(handle, near_heap + 120, 10);
            itoa(pages, near_heap + 150, 10);
            strcpy(near_heap, name);
            strcat(near_heap, " reloaded into EMM, diff size =");
            strcat(near_heap, near_heap + 100);
            strcat(near_heap, "  ");
            strcat(near_heap, near_heap + 120);
            strcat(near_heap, "  ");
            strcat(near_heap, near_heap + 150);
            Fatal_Exit(near_heap);
        }
    }
    return handle;
}

/* (seg) 16 KB from seg:0 to the VGAFILEH handle's page 4 (at 64 KB) */
void EMS_Backup_Save(unsigned buf_seg)
{
    EMS_Write(0, buf_seg, 0x10000L, EMS_PAGE_SIZE, ems_vgafile_handle);
}

/* (seg) reads them back */
void EMS_Backup_Restore(unsigned buf_seg)
{
    EMS_Read(0, buf_seg, 0x10000L, EMS_PAGE_SIZE, ems_vgafile_handle);
}

/* (buf, size) size bytes from DS:buf to the VGAFILEH handle's page 4 (at 64 KB) */
void EMS_Backup_Save_Buf(char *buf, unsigned size)
{
    unsigned data_seg;

    asm mov ax, ds
    asm mov data_seg, ax
    EMS_Write(buf, data_seg, 0x10000L, size, ems_vgafile_handle);
}

/* (buf, size) reads them back */
void EMS_Backup_Restore_Buf(char *buf, unsigned size)
{
    unsigned data_seg;

    asm mov ax, ds
    asm mov data_seg, ax
    EMS_Read(buf, data_seg, 0x10000L, size, ems_vgafile_handle);
}

/* () maps the frame handle's first pages (called at the top of most game functions) */
void EMS_Map_Default(void)
{
    EMS_Map_Handle_Pages(ems_frame_handle, 0);
}

/* (paragraphs) the first block carved out of the EMS page frame (heap block header: magic 12FAh 4ECFh, size,
   used 1); too big -> Heap_Fatal_Error(HEAP_ERR_EMS_SHORT, missing). Returns the data segment */
unsigned EMS_Frame_Block_First(int paragraphs)
{
    int left;
    unsigned seg;

    ems_frame_top = 0;
    seg = ems_frame_seg + ems_frame_top;
    EMS_Map_Default();
    paragraphs = paragraphs + 1;
    left = 0xffa - ems_frame_top;               /* the 64 KB page frame (0x1000 paragraphs) less 6 */
    if (left < paragraphs)
        Heap_Fatal_Error(HEAP_ERR_EMS_SHORT, paragraphs - left);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(seg, HEAP_HDR_SIZE, paragraphs - 1);
    Far_Poke_Word(seg, HEAP_HDR_USED, 1);
    Far_Poke_Word(seg, HEAP_HDR_MARK, 1);
    ems_frame_top += paragraphs + 1;
    return seg + 1;
}

/* (paragraphs) the next block in the page frame */
unsigned EMS_Frame_Block_Next(int paragraphs)
{
    int left;
    unsigned seg;

    EMS_Map_Default();
    seg = ems_frame_seg + ems_frame_top;
    EMS_Map_Default();
    paragraphs = paragraphs + 1;
    left = 0xffa - ems_frame_top;               /* the 64 KB page frame (0x1000 paragraphs) less 6 */
    if (left < paragraphs)
        Heap_Fatal_Error(HEAP_ERR_EMS_SHORT, paragraphs - left);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(seg, HEAP_HDR_SIZE, paragraphs - 1);
    Far_Poke_Word(seg, HEAP_HDR_USED, 1);
    Far_Poke_Word(seg, HEAP_HDR_MARK, 1);
    ems_frame_top += paragraphs + 1;
    return seg + 1;
}

/* () saves the frame top */
void EMS_Frame_Mark(void)
{
    ems_frame_mark = ems_frame_top;
}

/* () restores it */
void EMS_Frame_Release(void)
{
    ems_frame_top = ems_frame_mark;
}

/* (lbx, handle) DEAD: reads <lbx>.LBX (extension replaced) into an existing handle page by page; fatal "EMM 2long"
   when the handle has fewer pages (INT 67h 4Ch). QUIRK #461: opens lbx as given, not the .LBX path it measured */
void EMS_LBX_Reload_Into_Handle(char *lbx, int handle)
{
    unsigned pages;
    unsigned frame;
    unsigned chunk;
    int page;
    long size;
    long pos;
    unsigned avail;
    char path[14];
    register int i;
    register int fh;

    strcpy(path, lbx);
    for (i = 0; i < 13; i++)
        if (path[i] == '.')
            path[i] = 0;
    strcat(path, ".LBX");
    size = DOS_File_Size(path);
    if (size == 0)
        LBX_Error(lbx, LBX_ERR_EMS_RESERVED_LOAD, 0);
    pages = size / EMS_PAGE_SIZE;
    if (size % EMS_PAGE_SIZE != 0)
        pages++;
    asm mov ah, 4Ch
    asm mov dx, handle
    asm int 67h
    asm mov avail, bx
    if (pages > avail)
        Fatal_Exit("EMM 2long");
    frame = EMS_Page_Frame();
    page = 0;
    if (frame != 0) {
        fh = DOS_Open_Read(lbx);
        if (fh != 0) {
            pos = 0;
            chunk = EMS_PAGE_SIZE;
            while (size >= EMS_PAGE_SIZE) {
                size -= EMS_PAGE_SIZE;
                EMS_Map_Handle_Pages(handle, page);
                DOS_Read_To_Segment(frame, chunk, fh);
                page++;
                pos += EMS_PAGE_SIZE;
            }
            if (size > 0) {
                chunk = size;
                EMS_Map_Handle_Pages(handle, page);
                DOS_Read_To_Segment(frame, chunk, fh);
            }
            DOS_Close(fh);
        }
    }
}

/* (k) the EMS the start-up message asks for */
void EMS_Set_Required_K(int k)
{
    ems_required_k = k;
}

/* (buf) "You must have at least <k>K of expanded memory." */
void EMS_Required_Message(char *buf)
{
    char num_text[20];

    itoa(ems_required_k, num_text, 10);
    strcpy(buf, "You must have at least ");
    strcat(buf, num_text);
    strcat(buf, "K of expanded memory.");
}
