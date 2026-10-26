/* LBX.C - WIZARDS.EXE code segment 1584 (MAGIC.EXE 14f2, the same file): the LBX archive loader. An LBX file has a
   512-byte header (entry count, FEADh marker, two more words - +6 the file type EMSLBX.C reads - then from
   +8, LBX_OFFSET_TABLE, a long offset per entry); one file stays open between calls.
   Entries load into a new heap block (mode 0), back into an existing block (mode 1, 'Reload') or after what a
   block already holds (mode 2, 'Reload_Next'); record files ('_Records') hold a count and record size first. EMS
   copies (EMSLBX.C) are tried first. Errors end the program through LBX_Error.
   The three loaders (LBX_Load_Core, LBX_Load_Records_Core, LBX_Load_Records_Near) are copies of one another, and
   LBX_Entry_Locate a fourth: their path buffers differ (60 / 80 bytes), only LBX_Load_Records_Core checks the file
   handle twice, and some errors name the open file instead of the requested one.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1584 src/wizards/LBX.C */
#include "ENUMS.H"
#pragma warn -pro

extern int required_free_kb;                         /* DS:e5ce */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
char lbx_open_name[16];                         /* DS:e5d6  MAGIC DS:a5cc   */
unsigned lbx_header_seg;                        /* DS:e5d4  MAGIC DS:a5ca   */
int lbx_open_mode;                              /* DS:e5d2  MAGIC DS:a5c8   */
int lbx_entry_count;                            /* DS:e5d0  MAGIC DS:a5c6   */

unsigned Far_Peek_Word(unsigned seg, unsigned off);
void Far_Poke_Word(unsigned seg, unsigned off, unsigned value);
long Far_Peek_DWord(unsigned seg, unsigned off);
unsigned Heap_Free_Paragraphs(unsigned block);
unsigned LBX_Load_Core(char *name, int entry, unsigned block, int mode, int alt_header);
unsigned LBX_Load_Records_Core(char *name, int entry, unsigned block, int mode, unsigned first, unsigned count,
                               unsigned size);
void LBX_Error();                              /* no prototype: called with 3 and 4 arguments */
void LBX_Normalize_Name(char *name);

int lbx_loader_ready = 0;                       /* DS:738e */
int lbx_file_handle = -1;                       /* DS:7390 */
int lbx_header_mode = 0;                        /* DS:7392  2 would force header offset 0 - never set */
char lbx_path[50] = "";                         /* DS:7394  directory tried when NAME.LBX is not in the current one */

/* the unsigned wrappers below have no return statement: their value is LBX_Load_Core's AX, the segment */

/* (name, entry) loads an entry into a new heap block; returns its segment */
unsigned LBX_Load(char *name, int entry)
{
    LBX_Load_Core(name, entry, 0, 0, 0);
}

/* (name, entry, block) loads an entry into the block (replacing its contents) */
unsigned LBX_Reload(char *name, int entry, unsigned block)
{
    LBX_Load_Core(name, entry, block, 1, 0);
}

/* (name, entry, block) appends an entry to the block */
unsigned LBX_Reload_Next(char *name, int entry, unsigned block)
{
    LBX_Load_Core(name, entry, block, 2, 0);
}

/* (name, entry, first, count, size) records first .. first + count - 1 (size bytes each) of an entry into a
   new heap block; returns its segment */
unsigned LBX_Load_Records(char *name, int entry, unsigned first, unsigned count, unsigned size)
{
    LBX_Load_Records_Core(name, entry, 0, 0, first, count, size);
}

/* (name, entry, block, first, count, size) the same into the block (replacing its contents) */
unsigned LBX_Reload_Records(char *name, int entry, unsigned block, unsigned first, unsigned count, unsigned size)
{
    LBX_Load_Records_Core(name, entry, block, 1, first, count, size);
}

unsigned LBX_Reload_Next_Records(char *name, int entry, unsigned block, unsigned first, unsigned count,
                                 unsigned size)
{
    LBX_Load_Records_Core(name, entry, block, 2, first, count, size);
}

/* (name, entry) LBX_Load with the header read at file offset 200h (alt_header 1; not when lbx_header_mode is 2) */
unsigned LBX_Load_Alt_Header(char *name, int entry)
{
    LBX_Load_Core(name, entry, 0, 0, 1);
}

/* (name, entry, block) LBX_Reload with the header read at file offset 200h */
unsigned LBX_Reload_Alt_Header(char *name, int entry, unsigned block)
{
    LBX_Load_Core(name, entry, block, 1, 1);
}

/* (name, entry, block) LBX_Reload_Next with the header read at file offset 200h */
unsigned LBX_Reload_Next_Alt_Header(char *name, int entry, unsigned block)
{
    LBX_Load_Core(name, entry, block, 2, 1);
}

/* (name, entry, block, mode, alt_header) loads entry 'entry' of NAME.LBX: mode 0 into a new heap block, 1 into 'block'
   (from its start), 2 after what 'block' already holds; returns the data segment */
unsigned LBX_Load_Core(char *name, int entry, unsigned block, int mode, int alt_header)
{
    unsigned result;
    int table_ofs;
    int open_mode;
    int paragraphs;
    unsigned dst;
    unsigned chunk;
    long header_pos;
    long start;
    long end;
    long size;
    char file_name[20];
    char path[60];

    if (entry < 0)
        LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
    if (lbx_loader_ready == 0) {
        lbx_loader_ready = 1;
        lbx_header_seg = Heap_Alloc_LBX_Header(32);
    }
    LBX_Normalize_Name(name);
    if (alt_header == 0 || lbx_header_mode == 2)
        open_mode = 0;
    else
        open_mode = 1;
    result = EMS_LBX_Load_Entry(name, entry, block, mode, open_mode);
    if (result == 0) {
        if (lbx_file_handle == -1 || stricmp(name, lbx_open_name) != 0 || lbx_open_mode != open_mode) {
            lbx_open_mode = open_mode;
            if (lbx_file_handle != -1)
                DOS_Close(lbx_file_handle);
            strcpy(lbx_open_name, name);
            strcpy(file_name, name);
            strcat(file_name, ".LBX");
            lbx_file_handle = DOS_Open_Read(file_name);
            if (lbx_file_handle == 0) {
                if (lbx_path[0] == 0)
                    LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
                strcpy(path, lbx_path);
                strcat(path, file_name);
                lbx_file_handle = DOS_Open_Read(path);
                if (lbx_file_handle == 0)
                    LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
            }
            if (lbx_open_mode != 0)
                header_pos = 512;
            else
                header_pos = 0;
            if (DOS_Seek(header_pos, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            else
                DOS_Read_To_Segment(lbx_header_seg, 512, lbx_file_handle);
            if (Far_Peek_Word(lbx_header_seg, 2) != LBX_MAGIC)
                LBX_Error(name, LBX_ERR_NOT_LBX, entry);
            lbx_entry_count = Far_Peek_Word(lbx_header_seg, 0);
        }
        if (entry >= lbx_entry_count)
            LBX_Error(name, LBX_ERR_NO_SUCH_ENTRY, entry);
        table_ofs = entry * 4 + LBX_OFFSET_TABLE;
        start = Far_Peek_DWord(lbx_header_seg, table_ofs);
        end = Far_Peek_DWord(lbx_header_seg, table_ofs + 4);
        size = end - start;
        if (DOS_Seek(start, lbx_file_handle) == 0)
            LBX_Error(lbx_open_name, LBX_ERR_CORRUPT, entry);
        paragraphs = size / 16 + 1;
        switch (mode) {
        case 0:
            result = Heap_Alloc_Raw(paragraphs);
            if (result == 0)
                LBX_Error(name, LBX_ERR_NO_MEMORY, entry);
            break;
        case 1:
            if (Heap_Block_Valid(block) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            if (Far_Peek_Word(block, HEAP_HDR_SIZE) - 1 < paragraphs)
                LBX_Error(name, LBX_ERR_BAD_BLOCK, entry, paragraphs - Far_Peek_Word(block, HEAP_HDR_SIZE) + 1);
            result = block + 1;
            Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + 1);
            break;
        case 2:
            if (Heap_Block_Valid(block) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            if (Heap_Free_Paragraphs(block) < paragraphs)
                LBX_Error(name, LBX_ERR_BLOCK_TOO_SMALL, entry, paragraphs - Heap_Free_Paragraphs(block));
            result = block + Far_Peek_Word(block, HEAP_HDR_USED);
            Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + Far_Peek_Word(block, HEAP_HDR_USED));
            break;
        }
        dst = result;
        chunk = LBX_READ_CHUNK;
        while (size >= LBX_READ_CHUNK) {
            size -= LBX_READ_CHUNK;
            if (DOS_Read_To_Segment(dst, chunk, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            dst += LBX_READ_CHUNK_PARAS;
        }
        if (size > 0) {
            chunk = size;
            if (DOS_Read_To_Segment(dst, chunk, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
        }
    }
    Mem_Track_Min_Free();
    return result;
}

/* (name, entry, block, mode, first, count, size) loads records first .. first + count - 1 of a record entry
   (word count, word record size, then the records; size must equal the file's record size) */
unsigned LBX_Load_Records_Core(char *name, int entry, unsigned block, int mode, unsigned first, unsigned count,
                               unsigned size)
{
    unsigned result;
    int table_ofs;
    int open_mode;
    unsigned rec_count;
    int paragraphs;
    unsigned dst;
    unsigned chunk;
    unsigned rec_size;
    long header_pos;
    long start;
    long end;
    long bytes;
    char file_name[20];
    char path[80];

    if (entry < 0)
        LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
    if (lbx_loader_ready == 0) {
        lbx_loader_ready = 1;
        lbx_header_seg = Heap_Alloc_LBX_Header(32);
    }
    LBX_Normalize_Name(name);
    result = EMS_LBX_Load_Records(name, entry, block, mode, first, count, size);
    open_mode = 0;
    if (result == 0) {
        if (lbx_file_handle == -1 || stricmp(name, lbx_open_name) != 0 || lbx_open_mode != open_mode) {
            lbx_open_mode = open_mode;
            if (lbx_file_handle != -1)
                DOS_Close(lbx_file_handle);
            strcpy(lbx_open_name, name);
            strcpy(file_name, name);
            strcat(file_name, ".LBX");
            lbx_file_handle = DOS_Open_Read(file_name);
            if (lbx_file_handle == 0) {
                if (lbx_path[0] == 0)
                    LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
                strcpy(path, lbx_path);
                strcat(path, file_name);
                lbx_file_handle = DOS_Open_Read(path);
                if (lbx_file_handle == 0)
                    LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
            }
            if (lbx_file_handle == 0)
                LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
            header_pos = 0;
            if (DOS_Seek(header_pos, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            else
                DOS_Read_To_Segment(lbx_header_seg, 512, lbx_file_handle);
            if (Far_Peek_Word(lbx_header_seg, 2) != LBX_MAGIC)
                LBX_Error(name, LBX_ERR_NOT_LBX, entry);
            lbx_entry_count = Far_Peek_Word(lbx_header_seg, 0);
        }
        if (entry >= lbx_entry_count)
            LBX_Error(name, LBX_ERR_NO_SUCH_ENTRY, entry);
        table_ofs = entry * 4 + LBX_OFFSET_TABLE;
        start = Far_Peek_DWord(lbx_header_seg, table_ofs);
        end = Far_Peek_DWord(lbx_header_seg, table_ofs + 4);
        bytes = end - start;
        if (DOS_Seek(start, lbx_file_handle) == 0)
            LBX_Error(lbx_open_name, LBX_ERR_CORRUPT, entry);
        DOS_Read_Near(&rec_count, 2, lbx_file_handle);
        DOS_Read_Near(&rec_size, 2, lbx_file_handle);
        if (rec_size == 0 || size != rec_size)
            LBX_Error(lbx_open_name, LBX_ERR_RECORD_SIZE, entry);
        if (first + count > rec_count)
            LBX_Error(lbx_open_name, LBX_ERR_NO_SUCH_RECORD, entry);
        start += (long)first * rec_size + 4;
        if (DOS_Seek(start, lbx_file_handle) == 0)
            LBX_Error(lbx_open_name, LBX_ERR_CORRUPT, entry);
        bytes = (long)count * rec_size;
        paragraphs = bytes / 16 + 1;
        switch (mode) {
        case 0:
            result = Heap_Alloc_Raw(paragraphs);
            if (result == 0)
                LBX_Error(name, LBX_ERR_NO_MEMORY, entry);
            break;
        case 1:
            if (Heap_Block_Valid(block) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            if (Far_Peek_Word(block, HEAP_HDR_SIZE) - 1 < paragraphs)
                LBX_Error(name, LBX_ERR_BAD_BLOCK, entry, paragraphs - Far_Peek_Word(block, HEAP_HDR_SIZE) + 1);
            result = block + 1;
            Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + 1);
            break;
        case 2:
            if (Heap_Block_Valid(block) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            if (Heap_Free_Paragraphs(block) < paragraphs)
                LBX_Error(name, LBX_ERR_BLOCK_TOO_SMALL, entry, paragraphs - Heap_Free_Paragraphs(block));
            result = block + Far_Peek_Word(block, HEAP_HDR_USED);
            Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + Far_Peek_Word(block, HEAP_HDR_USED));
            break;
        }
        dst = result;
        chunk = LBX_READ_CHUNK;
        while (bytes >= LBX_READ_CHUNK) {
            bytes -= LBX_READ_CHUNK;
            if (DOS_Read_To_Segment(dst, chunk, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            dst += LBX_READ_CHUNK_PARAS;
        }
        if (bytes > 0) {
            chunk = bytes;
            if (DOS_Read_To_Segment(dst, chunk, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
        }
    }
    Mem_Track_Min_Free();
    return result;
}

/* (name, entry, dest, first, count, size) the record loader into a near buffer; reads (count x size) % 60000
   bytes */
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size)
{
    int result;
    int table_ofs;
    int open_mode;
    unsigned rec_count;
    int read_bytes;
    unsigned rec_size;
    long header_pos;
    long start;
    long end;
    long bytes;
    char file_name[20];
    char path[80];

    if (entry < 0)
        LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
    if (lbx_loader_ready == 0) {
        lbx_loader_ready = 1;
        lbx_header_seg = Heap_Alloc_LBX_Header(32);
    }
    LBX_Normalize_Name(name);
    result = EMS_LBX_Load_Records_Near(name, entry, dest, first, count, size);
    open_mode = 0;
    if (result == 0) {
        if (lbx_file_handle == -1 || stricmp(name, lbx_open_name) != 0 || lbx_open_mode != open_mode) {
            lbx_open_mode = open_mode;
            if (lbx_file_handle != -1)
                DOS_Close(lbx_file_handle);
            strcpy(lbx_open_name, name);
            strcpy(file_name, name);
            strcat(file_name, ".LBX");
            lbx_file_handle = DOS_Open_Read(file_name);
            if (lbx_file_handle == 0) {
                if (lbx_path[0] == 0)
                    LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
                strcpy(path, lbx_path);
                strcat(path, file_name);
                lbx_file_handle = DOS_Open_Read(path);
                if (lbx_file_handle == 0)
                    LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
            }
            header_pos = 0;
            if (DOS_Seek(header_pos, lbx_file_handle) == 0)
                LBX_Error(name, LBX_ERR_CORRUPT, entry);
            else
                DOS_Read_To_Segment(lbx_header_seg, 512, lbx_file_handle);
            if (Far_Peek_Word(lbx_header_seg, 2) != LBX_MAGIC)
                LBX_Error(name, LBX_ERR_NOT_LBX, entry);
            lbx_entry_count = Far_Peek_Word(lbx_header_seg, 0);
        }
        if (entry >= lbx_entry_count)
            LBX_Error(name, LBX_ERR_NO_SUCH_ENTRY, entry);
        table_ofs = entry * 4 + LBX_OFFSET_TABLE;
        start = Far_Peek_DWord(lbx_header_seg, table_ofs);
        end = Far_Peek_DWord(lbx_header_seg, table_ofs + 4);
        bytes = end - start;
        if (DOS_Seek(start, lbx_file_handle) == 0)
            LBX_Error(lbx_open_name, LBX_ERR_CORRUPT, entry);
        DOS_Read_Near(&rec_count, 2, lbx_file_handle);
        DOS_Read_Near(&rec_size, 2, lbx_file_handle);
        if (rec_size == 0 || size != rec_size)
            LBX_Error(lbx_open_name, LBX_ERR_RECORD_SIZE, entry);
        if (first + count > rec_count)
            LBX_Error(lbx_open_name, LBX_ERR_NO_SUCH_RECORD, entry);
        start += (long)first * rec_size + 4;
        if (DOS_Seek(start, lbx_file_handle) == 0)
            LBX_Error(lbx_open_name, LBX_ERR_CORRUPT, entry);
        bytes = (long)count * rec_size;
        read_bytes = bytes % 60000L;
        if (DOS_Read_Near(dest, read_bytes, lbx_file_handle) == 0)
            LBX_Error(name, LBX_ERR_CORRUPT, entry);
    }
    Mem_Track_Min_Free();
}

/* (name, entry, &offset, &size, alt_header) opens NAME.LBX like LBX_Load_Core and returns its DOS handle with the
   entry's file offset and size */
int LBX_Entry_Locate(char *name, int entry, long *offset, long *size, int alt_header)
{
    int open_mode;
    int table_ofs;
    long end;
    long header_pos;
    char file_name[20];
    char path[80];

    if (entry < 0)
        LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
    if (lbx_loader_ready == 0) {
        lbx_loader_ready = 1;
        lbx_header_seg = Heap_Alloc_LBX_Header(32);
    }
    LBX_Normalize_Name(name);
    if (alt_header == 0 || lbx_header_mode == 2)
        open_mode = 0;
    else
        open_mode = 1;
    if (lbx_file_handle == -1 || stricmp(name, lbx_open_name) != 0 || lbx_open_mode != open_mode) {
        lbx_open_mode = open_mode;
        if (lbx_file_handle != -1)
            DOS_Close(lbx_file_handle);
        strcpy(lbx_open_name, name);
        strcpy(file_name, name);
        strcat(file_name, ".LBX");
        lbx_file_handle = DOS_Open_Read(file_name);
        if (lbx_file_handle == 0) {
            if (lbx_path[0] == 0)
                LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
            strcpy(path, lbx_path);
            strcat(path, file_name);
            lbx_file_handle = DOS_Open_Read(path);
            if (lbx_file_handle == 0)
                LBX_Error(name, LBX_ERR_NOT_FOUND, entry);
        }
        if (lbx_open_mode != 0)
            header_pos = 512;
        else
            header_pos = 0;
        if (DOS_Seek(header_pos, lbx_file_handle) == 0)
            LBX_Error(name, LBX_ERR_CORRUPT, entry);
        else
            DOS_Read_To_Segment(lbx_header_seg, 512, lbx_file_handle);
        if (Far_Peek_Word(lbx_header_seg, 2) != LBX_MAGIC)
            LBX_Error(name, LBX_ERR_NOT_LBX, entry);
        lbx_entry_count = Far_Peek_Word(lbx_header_seg, 0);
    }
    if (entry >= lbx_entry_count)
        LBX_Error(name, LBX_ERR_NO_SUCH_ENTRY, entry);
    table_ofs = entry * 4 + LBX_OFFSET_TABLE;
    *offset = Far_Peek_DWord(lbx_header_seg, table_ofs);
    end = Far_Peek_DWord(lbx_header_seg, table_ofs + 4);
    *size = end - *offset;
    return lbx_file_handle;
}

/* (path) the directory tried when an LBX file is not in the current one.
   Simtex name Set_Alternate_Path (MoO2 symbols, module farload); clarified using ReMoM repo */
void LBX_Set_Path(char *path)
{
    strcpy(lbx_path, path);
}

/* (name, code, entry, extra) ends the program with "NAME.LBX [entry n] " + the message for code (none for 6).
   Code 14 has no break: its message runs on into code 15's (#398).
   Simtex name Error_Handler (MoO2 symbols, module farload); clarified using ReMoM repo
   Numbered: BUG #398 (BUGS_AND_QUIRKS.txt) */
void LBX_Error(char *name, int code, int entry, int extra)
{
    char message[120];
    char num_text[20];

    strcpy(message, name);
    itoa(entry, num_text, 10);
    strcat(message, ".LBX [entry ");
    strcat(message, num_text);
    strcat(message, "] ");
    switch (code) {
    case LBX_ERR_NOT_FOUND:
        strcat(message, " could not be found.");
        break;
    case LBX_ERR_CORRUPT:
        strcat(message, " has been corrupted.");
        break;
    case LBX_ERR_NO_MEMORY:
        strcpy(message, "Insufficient memory. You need at least ");
        itoa(required_free_kb, num_text, 10);
        strcat(message, num_text);
        strcat(message, "K free. Try removing all TSR's.");
        break;
    case LBX_ERR_BAD_BLOCK:
        strcat(message, " was not properly allocated or has been corrupted.");
        break;
    case LBX_ERR_BLOCK_TOO_SMALL:
        strcat(message, " failed to reload. Allocation too small by ");
        itoa(extra, num_text, 10);
        strcat(message, num_text);
        strcat(message, " pages");
        break;
    case LBX_ERR_NOT_LBX:
        strcat(message, " is not an LBX file");
        break;
    case LBX_ERR_NO_SUCH_ENTRY:
        strcat(message, " exceeds number of LBX entries");
        break;
    case LBX_ERR_RECORD_SIZE:
        strcat(message, " has an incorrect record size");
        break;
    case LBX_ERR_NO_SUCH_RECORD:
        strcat(message, " exceeds number of defined records");
        break;
    case LBX_ERR_EMS_NOT_RELEASED:
        strcpy(message, name);
        strcat(message, ".LBX");
        strcat(message, " cannot be reloaded into EMS w/o being first released.");
        break;
    case LBX_ERR_EMS_FULL:
        strcat(message, " EMM loading error. Insufficient EMM.");
        break;
    case LBX_ERR_EMS_PICTURES_ONLY:
        strcat(message, " Only pictures may be loaded into reserved EMM");
        break;
    case LBX_ERR_EMS_RESERVED_FULL:
        strcat(message, " EMM loading error. Insufficient EMM.");
        strcat(message, " (Reserved EMM) ");
    case LBX_ERR_EMS_RESERVED_LOAD:
        strcat(message, " LBX to");
        strcat(message, " EMM loading error. Insufficient EMM.");
        strcat(message, " (Reserved EMM) ");
        break;
    case LBX_ERR_FRAME_TOO_BIG:
        strcat(message, " Vga file animation frames cannot exceed 65536 bytes");
    }
    Fatal_Exit(message);
}

/* (name) upper-cases the name in place and cuts it at the first '.' */
void LBX_Normalize_Name(char *name)
{
    int i;

    for (i = 0; name[i] != 0; i++) {
        if (name[i] > 0x60)                     /* every byte above '`', not only a .. z */
            name[i] = name[i] - 0x20;
        if (name[i] == '.')
            name[i] = 0;
    }
}

/* (kb) the free memory that the fatal "Insufficient memory" message asks for */
void LBX_Set_Min_Free(int kb)
{
    required_free_kb = kb;
}
