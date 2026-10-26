/* HEAP.C - WIZARDS.EXE code segment 150a (from 150a:000b): the near scratch arena and the far block heap.
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 150a src/wizards/HEAP.C
   NO <string.h> on purpose: Near_Alloc_Error calls strcat with ONE argument (BUG #381 in the original: it only compiles
   without the prototype; the stray call appends to whatever pointer is left on the stack). */
#include <stdlib.h>
#include <alloc.h>
#include <dos.h>
#include "ENUMS.H"

#define NEAR_HEAP_SIZE 0x1130          /* near_heap: the text scratch, then the arena */
#define NEAR_TEXT_SCRATCH 0x100
#define NEAR_ARENA_SIZE 0x1030

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int required_free_kb;                                /* DS:e5ce  MAGIC DS:a5c4   */
                                                /* the free KB the fatal "Insufficient memory" message asks for */
void far *heap_last_farmalloc;                  /* DS:e5ca  MAGIC DS:a5c0   */
char near_heap[4400];                           /* DS:d49a  MAGIC DS:9490   */
                                                /* 0x1130: a 0x100-byte text scratch, then the 0x1030-byte arena */
unsigned near_alloc_mark;                       /* DS:d498  MAGIC DS:948e   */


/* the first data of this module (link test, batch 36): two words no code reads, then the near arena top */
unsigned heap_unused_7278 = 0;                  /* DS:7278  no reader */
unsigned heap_unused_727a = 0;                  /* DS:727a  no reader */
unsigned near_alloc_top = 0x100;                /* DS:727c */

void Fatal_Exit(char *message);
void Mem_Track_Min_Free(void);
int Check_Release_Version(void);
unsigned Far_Peek_Word(unsigned seg, unsigned off);
void Far_Poke_Word(unsigned seg, unsigned off, unsigned value);
int Heap_Block_Valid(unsigned block);
void Near_Alloc_Error(int amount);
void Heap_Fatal_Error(int error_kind, int amount);
unsigned Heap_Free_Paragraphs(unsigned block);

/* () remembers the near arena's top (Near_Alloc_Release goes back to it) */
void Near_Alloc_Mark(void)
{
    near_alloc_mark = near_alloc_top;
}

/* () frees everything allocated in the near arena since Near_Alloc_Mark */
void Near_Alloc_Release(void)
{
    near_alloc_top = near_alloc_mark;
}

/* () empties the near arena (its first 0x100 bytes stay a text scratch: allocations start at near_heap +
   0x100) */
void Near_Alloc_Reset_All(void)
{
    near_alloc_top = NEAR_TEXT_SCRATCH;
    near_alloc_mark = NEAR_TEXT_SCRATCH;
}

/* (bytes) empties the near arena and returns its start, reserving bytes (at most 0x1030, else a fatal error) */
char *Near_Alloc_Reset(unsigned bytes)
{
    if (bytes > NEAR_ARENA_SIZE)
        Near_Alloc_Error(bytes - NEAR_HEAP_SIZE + NEAR_TEXT_SCRATCH);
    near_alloc_top = bytes + NEAR_TEXT_SCRATCH;
    near_alloc_mark = bytes + NEAR_TEXT_SCRATCH;
    return (char *)((unsigned)near_heap + NEAR_TEXT_SCRATCH);
}

/* (bytes) -> bytes of the near arena (near_heap, 0x1130 in all), or a fatal error when they do not fit */
char *Near_Alloc(unsigned bytes)
{
    unsigned offset;

    if (NEAR_HEAP_SIZE - near_alloc_top < bytes)
        Near_Alloc_Error(bytes - NEAR_HEAP_SIZE + near_alloc_top);
    offset = near_alloc_top;
    near_alloc_top += bytes;
    return (char *)((unsigned)near_heap + offset);
}

/* (amount) the fatal "Near Allocation too large by n bytes" (see the file comment on its strcat BUG #381)
   Numbered: BUG #381 (BUGS_AND_QUIRKS.txt) */
void Near_Alloc_Error(int amount)
{
    strcpy(near_heap, "Near Allocation too large by ");
    itoa(amount, near_heap + 100, 10);
    strcat(near_heap, near_heap + 100);
    strcat(" bytes");
    Fatal_Exit(near_heap);
}

/* (paragraphs) -> a new far heap block (farmalloc, one header paragraph in front): its segment, whose 16-byte
   header gets the two magic words, the size and used = 1 (the header itself); fatal error 1 when DOS memory
   runs out. Sub-blocks are carved from it with Heap_Alloc_Front / _Back */
unsigned Heap_Alloc_Block(unsigned paragraphs)
{
    volatile unsigned seg;      /* kept in memory in the original (volatile, or its address was taken) */

    heap_last_farmalloc = farmalloc(((unsigned long)paragraphs << 4) + 16);
    if (heap_last_farmalloc == NULL)
        Heap_Fatal_Error(HEAP_ERR_ALLOC, paragraphs);
    seg = FP_SEG(heap_last_farmalloc) + 1;
    Far_Poke_Word(seg, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(seg, HEAP_HDR_SIZE, paragraphs);
    Far_Poke_Word(seg, HEAP_HDR_USED, 1);
    Mem_Track_Min_Free();
    return seg;
}

/* (paragraphs) -> a raw farmalloc block's segment (no header written), 0 when DOS memory runs out; a copy of
   Heap_Alloc_Raw, used for the LBX header buffer.
   Simtex name Allocate_Space_No_Header (MoO2 symbols, module allocate); clarified using ReMoM repo */
unsigned Heap_Alloc_LBX_Header(unsigned paragraphs)
{
    unsigned seg;

    heap_last_farmalloc = farmalloc(((unsigned long)paragraphs << 4) + 16);
    if (heap_last_farmalloc == NULL)
        return 0;
    seg = FP_SEG(heap_last_farmalloc) + 1;
    Mem_Track_Min_Free();
    return seg;
}

/* (paragraphs) -> a raw farmalloc block's segment (no header written), 0 when DOS memory runs out */
unsigned Heap_Alloc_Raw(unsigned paragraphs)
{
    unsigned seg;

    heap_last_farmalloc = farmalloc(((unsigned long)paragraphs << 4) + 16);
    if (heap_last_farmalloc == NULL)
        return 0;
    seg = FP_SEG(heap_last_farmalloc) + 1;
    Mem_Track_Min_Free();
    return seg;
}

/* (block) saves the block's used count in its mark field.
   Simtex name Mark_Block (MoO2 symbols, module allocate); clarified using ReMoM repo */
void Heap_Mark(unsigned block)
{
    unsigned used;

    used = Far_Peek_Word(block, HEAP_HDR_USED);
    Far_Poke_Word(block, HEAP_HDR_MARK, used);
}

/* (block) frees every sub-block allocated since Heap_Mark (used = the mark).
   Simtex name Release_Block (MoO2 symbols, module allocate); clarified using ReMoM repo */
void Heap_Release(unsigned block)
{
    unsigned used;

    used = Far_Peek_Word(block, HEAP_HDR_MARK);
    Far_Poke_Word(block, HEAP_HDR_USED, used);
}

/* (block, paragraphs) -> a sub-block at the FRONT of the block (data segment): the block is emptied first
   (used = paragraphs + 1, so anything allocated before is overwritten); the sub-block gets its own header at
   block + 1 and can itself be allocated from. Fatal error 3 for a corrupt header, 2 when too small.
   Simtex name Allocate_First_Block (MoO2 symbols, module allocate); clarified using ReMoM repo */
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs)
{
    unsigned size;
    unsigned seg;

    paragraphs = paragraphs + 1;
    seg = block + 1;
    size = Far_Peek_Word(block, HEAP_HDR_SIZE);
    if (Heap_Block_Valid(block) == 0)
        Heap_Fatal_Error(HEAP_ERR_CORRUPT, paragraphs);
    if (size < paragraphs)
        Heap_Fatal_Error(HEAP_ERR_SHORT, paragraphs - size);
    Far_Poke_Word(block, HEAP_HDR_USED, paragraphs + 1);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(seg, HEAP_HDR_SIZE, paragraphs - 1);
    Far_Poke_Word(seg, HEAP_HDR_USED, 1);
    Far_Poke_Word(seg, HEAP_HDR_MARK, 1);
    return seg + 1;
}

/* (block) frees every sub-block (used = 1); reads the size and segment it never uses */
void Heap_Reset(unsigned block)
{
    unsigned size;
    unsigned seg;                               /* computed, never used */

    seg = block + 1;
    size = Far_Peek_Word(block, HEAP_HDR_SIZE);
    Far_Poke_Word(block, HEAP_HDR_USED, 1);
}

/* (block, paragraphs) -> a sub-block after the ones already allocated (data segment, its header one paragraph
   before); fatal error 3 for a corrupt header, 2 when the free paragraphs do not suffice.
   Simtex name Allocate_Next_Block (MoO2 symbols, module allocate); clarified using ReMoM repo */
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs)
{
    unsigned seg;

    paragraphs = paragraphs + 1;
    if (Heap_Block_Valid(block) == 0)
        Heap_Fatal_Error(HEAP_ERR_CORRUPT, paragraphs);
    if (Heap_Free_Paragraphs(block) < paragraphs)
        Heap_Fatal_Error(HEAP_ERR_SHORT, paragraphs - Heap_Free_Paragraphs(block));
    seg = Far_Peek_Word(block, HEAP_HDR_USED) + block;
    Far_Poke_Word(block, HEAP_HDR_USED, Far_Peek_Word(block, HEAP_HDR_USED) + paragraphs);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(seg, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(seg, HEAP_HDR_SIZE, paragraphs - 1);
    Far_Poke_Word(seg, HEAP_HDR_USED, 1);
    return seg + 1;
}

/* (block) -> size - used, in paragraphs */
unsigned Heap_Free_Paragraphs(unsigned block)
{
    unsigned free;

    free = Far_Peek_Word(block, HEAP_HDR_SIZE) - Far_Peek_Word(block, HEAP_HDR_USED);
    return free;
}

/* (seg) -> the far pointer seg:0 */
void far *Seg_To_Far_Ptr(unsigned seg)
{
    void far *p;
    unsigned s;

    s = seg;
    p = MK_FP(s, 0);
    return p;
}

/* (seg, off) -> the far pointer seg:off */
void far *Heap_Make_Far_Ptr(unsigned seg, unsigned off)
{
    void far *p;
    unsigned s;
    unsigned o;

    s = seg;
    o = off;
    p = (void far *)(((unsigned long)s << 16) + o);
    return p;
}

/* (seg) frees a block from Heap_Alloc_Raw / _Block (farmalloc's pointer: one paragraph lower, offset 4) */
void Heap_Free_Raw(unsigned seg)
{
    heap_last_farmalloc = MK_FP(seg - 1, 4);
    farfree(heap_last_farmalloc);
}

/* (error_kind, amount) ends the program: the release build only says how much free memory is needed; else 1
   farmalloc failed, 2 a sub-block does not fit (short by amount), 3 a corrupt block header, 4 the EMS
   variant of 2. Simtex name Allocation_Error (MoO2 symbols, module allocate); clarified using ReMoM repo */
void Heap_Fatal_Error(int error_kind, int amount)
{
    char message[120];
    char num_text[20];

    if (Check_Release_Version() != 0) {
        strcpy(message, "Insufficient memory. You need at least ");
        itoa(required_free_kb, num_text, 10);
        strcat(message, num_text);
        strcat(message, "K free. Try removing all TSR's.");
        Fatal_Exit(message);
    }
    switch (error_kind) {
    case HEAP_ERR_ALLOC:
        strcpy(message, "Dynamic allocation too small for Allocate_Space()");
        itoa(amount, num_text, 10);
    add_of:
        strcat(message, " of ");
    add_count:
    add_count_emm:      /* a second label here: the match needs case 4's jump to differ from case 2's */
        strcat(message, num_text);
        strcat(message, " blocks");
        break;
    case HEAP_ERR_SHORT:
        strcpy(message, "Failed to reload");
        strcat(message, " Allocate_Next_Block()");
        strcat(message, ": Short by ");
        itoa(amount, num_text, 10);
        goto add_count;
    case HEAP_ERR_CORRUPT:
        strcpy(message, "Allocation space has been corrupted for");
        itoa(amount, num_text, 10);
        strcat(message, " Allocate_Next_Block()");
        goto add_of;
    case HEAP_ERR_EMS_SHORT:
        strcpy(message, "Failed to reload");
        strcat(message, " (EMM) ");
        strcat(message, " Allocate_Next_Block()");
        strcat(message, ": Short by ");
        itoa(amount, num_text, 10);
        goto add_count_emm;
    }
    Fatal_Exit(message);
}
