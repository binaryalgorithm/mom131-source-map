/* UTIL.C - WIZARDS.EXE code segment 1d4d (MAGIC.EXE 1cbb, the same file): small utilities - the BIOS beep and key
   wait, the random number generator (32-bit LFSR, 9 bits per call), string case, near / far memory moves and fills
   (inline assembly; a segment of 0 means DS), record-array delete, bitmap bits, a dead developer choice box and the
   zoom-in screen transition. C with inline assembly.
   This module does NOT include GAME.H: its far-memory routines take (offset, segment) pairs, not far pointers.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1d4d src/wizards/UTIL.C */
#include "ENUMS.H"

extern unsigned draw_page_seg;                  /* DS:76ec */
extern int ems_vgafile_handle;                   /* DS:e5f4 */

unsigned rng_state_lo = 0x3568;                 /* DS:7846  LFSR state, low word (both words start at 0x3568) */
unsigned rng_state_hi = 0x3568;                 /* DS:7848 */
int bit_masks[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };     /* DS:784a */
int dev_box_color = 100;                        /* DS:785a  Dev_Choice_Box bevel color */
int dev_box_unused = 50;                        /* DS:785c  set by Dev_Choice_Box_Style, never read */
int dev_box_font_color = 0;                     /* DS:785e */
int dev_box_font = 0;                           /* DS:7860 */

void Fatal_Exit(char *message);
void Screen_Transition_Grab(unsigned dest_seg);
int UI_Add_Region();                          /* no prototype here: Dev_Choice_Box passes a string as the help id */

/* () beeps: BIOS teletype output of BEL (07h) */
void Beep(void)
{
    asm xor bx, bx
    asm mov ax, 0E07h
    asm int 10h
}

/* () waits for a key: BIOS INT 16h function 0 (AX = the key, not returned as a C value) */
void Wait_Key_BIOS(void)
{
    asm mov ah, 0
    asm int 16h
}

/* () no callers */
void Wait_30_Ticks(void)
{
    Timer_Mark();
    Wait_Ticks(30);
}

/* () both VGA pages filled with color 0 */
void Screen_Clear_Both_Pages(void)
{
    Set_Draw_Page_Front();
    Fill_Rect(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, 0);
    Set_Draw_Page();
    Fill_Rect(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, 0);
}

/* (seed) sets the generator state: low word, high word.
   Simtex name Set_Random_Seed (MoO2 symbols, module random); clarified using ReMoM repo
   Numbered: BUG #207 (BUGS_AND_QUIRKS.txt) */
void Random_Seed(long seed)
{
    rng_state_lo = seed;
    rng_state_hi = (unsigned long)seed >> 16;
}

/* () -> the generator state as a long (high word = rng_state_hi): Random_Seed(Random_Get_Seed()) restores it.
   Simtex name Get_Random_Seed (MoO2 symbols, module random); clarified using ReMoM repo */
long Random_Get_Seed(void)
{
    long seed;

    _AX = rng_state_hi;
    _DX = 0;
    asm add dx, rng_state_lo
    asm adc ax, 0
    asm mov word ptr seed + 2, ax
    asm mov word ptr seed, dx
    return seed;
}

/* () state = the BIOS tick count (0040:006C).
   Simtex name Randomize (MoO2 symbols, module random); clarified using ReMoM repo */
void Random_Seed_From_Timer(void)
{
    unsigned ticks_lo;
    unsigned ticks_hi;

    asm push es
    asm mov ax, 0
    asm mov es, ax
    asm mov ax, es:[46Ch]
    asm mov ticks_lo, ax
    asm mov ax, es:[46Eh]
    asm mov ticks_hi, ax
    asm pop es
    rng_state_lo = ticks_lo;
    rng_state_hi = ticks_hi;
}

/* (n) -> 1..n: 9 steps of the 32-bit LFSR (taps 0, 1, 2, 4, 6, 31 xored into bit 31) collect a 9-bit value v,
   result v % n + 1 (never above 512, modulo bias: BUG #206); state 0 -> 30BEh; n = 0 -> fatal error.
   Simtex name Random (MoO2 symbols, module random); clarified using ReMoM repo */
int Random(int n)
{
    int result;
    int bits;

    if (n == 0)
        Fatal_Exit("RND no 0's");
    bits = 0;
    asm push si
    asm push di
    asm mov si, rng_state_lo
    asm mov di, rng_state_hi
    asm mov cx, 9
next_bit:
    asm mov ax, si
    asm mov bx, si
    asm mov dx, di
    asm shr dx, 1
    asm rcr bx, 1
    asm xor ax, bx
    asm shr dx, 1
    asm rcr bx, 1
    asm xor ax, bx
    asm shr dx, 1
    asm rcr bx, 1
    asm shr dx, 1
    asm rcr bx, 1
    asm xor ax, bx
    asm shr dx, 1
    asm rcr bx, 1
    asm shr dx, 1
    asm rcr bx, 1
    asm xor ax, bx
    asm shr dx, 1
    asm xor al, dh
    asm mov dx, ax
    asm shr dx, 1
    asm rcl bits, 1
    asm shr ax, 1
    asm rcr di, 1
    asm rcr si, 1
    asm loop next_bit
    asm cmp si, 0
    asm jne store
    asm cmp di, 0
    asm jne store
    asm mov si, 30BEh
store:
    asm mov rng_state_lo, si
    asm mov rng_state_hi, di
    asm pop di
    asm pop si
    result = bits % n + 1;
    return result;
}

/* (str) a-z -> A-Z in place. Simtex name String_To_Upper (MoO2 symbols, module strings); clarified using ReMoM repo */
void Str_To_Upper(char *str)
{
    int i;

    for (i = 0; str[i] != 0; i++)
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] = str[i] - 32;
}

/* (str) A-Z -> a-z in place. Simtex name String_To_Lower (MoO2 symbols, module strings); clarified using ReMoM repo */
void Str_To_Lower(char *str)
{
    int i;

    for (i = 0; str[i] != 0; i++)
        if (str[i] >= 'A' && str[i] <= 'Z')
            str[i] = str[i] + 32;
}

/* the int memory routines from here (Mem_Copy .. Mem_Copy_Far) return 0 for a count of 0, else nothing: the
   value is whatever AX holds */

/* (dst, src, count) near byte copy */
int Mem_Copy(char *dst, char *src, int count)
{
    if (count == 0)
        return 0;
    asm push es
    asm push si
    asm push di
    asm mov ax, ds
    asm mov es, ax
    asm mov di, dst
    asm mov si, src
    asm mov cx, count
    asm rep movsb
    asm pop di
    asm pop si
    asm pop es
}

/* (dst, src, count) dst[i] = min(dst[i], src[i]) (unsigned) - near twin of Bytes_Min_Far; no callers */
int Bytes_Min_Near(char *dst, char *src, int count)
{
    if (count == 0)
        return 0;
    asm push es
    asm push si
    asm push di
    asm mov ax, ds
    asm mov es, ax
    asm mov di, dst
    asm mov si, src
    asm mov cx, count
min_next:
    asm lodsb
    asm cmp es:[di], al
    asm jb min_keep
    asm stosb
    asm dec di
min_keep:
    asm inc di
    asm loop min_next
    asm pop di
    asm pop si
    asm pop es
}

/* (dst, count) no callers */
int Mem_Zero(char *dst, int count)
{
    if (count == 0)
        return 0;
    asm push es
    asm push si
    asm push di
    asm mov ax, ds
    asm mov es, ax
    asm mov di, dst
    asm mov cx, count
    asm mov al, 0
    asm rep stosb
    asm pop di
    asm pop si
    asm pop es
}

/* (dst, count, value) no callers */
int Mem_Fill(char *dst, int count, int value)
{
    if (count == 0)
        return 0;
    asm push es
    asm push si
    asm push di
    asm mov ax, ds
    asm mov es, ax
    asm mov di, dst
    asm mov cx, count
    asm mov ax, value
    asm rep stosb
    asm pop di
    asm pop si
    asm pop es
}

/* (index, array, record_size, count) near record array: records after index move down one, the last is zeroed */
int Array_Delete_Near(int index, char *array, int record_size, int count)
{
    int pos;
    int bytes;

    pos = record_size * index;
    bytes = (count - index - 1) * record_size;
    if (bytes < 0)
        return 0;
    if (bytes == 0) {
        asm push es
        asm push si
        asm push di
        asm mov ax, ds
        asm mov es, ax
        asm mov ax, count
        asm dec ax
        asm mov bx, record_size
        asm mul bx
        asm add ax, array
        asm mov di, ax
        asm mov cx, record_size
        asm mov al, 0
        asm rep stosb
        asm pop di
        asm pop si
        asm pop es
    } else {
        asm push es
        asm push si
        asm push di
        asm mov ax, ds
        asm mov es, ax
        asm mov di, pos
        asm add di, array
        asm mov si, di
        asm add si, record_size
        asm mov cx, bytes
        asm rep movsb
        asm mov al, 0
        asm mov cx, record_size
        asm rep stosb
        asm pop di
        asm pop si
        asm pop es
    }
}

/* (index, array offset, array segment (0 = DS), record_size, count) records after index move down one (the last
   record is left as it was). Simtex name Delete_Structure (MoO2 symbols, module strings); clarified using ReMoM repo */
int Array_Delete(register int index, unsigned array, unsigned array_seg, int record_size, unsigned count)
{
    int pos;
    int bytes;

    if (array_seg == 0) {
        asm mov ax, ds
        asm mov array_seg, ax
    }
    if (index + 1 >= count)
        return 0;
    pos = record_size * index;
    bytes = (count - index - 1) * record_size;
    if (bytes != 0) {
        asm push ds
        asm push es
        asm push si
        asm push di
        asm mov di, pos
        asm add di, array
        asm mov si, di
        asm add si, record_size
        asm mov cx, bytes
        asm mov ax, array_seg
        asm mov es, ax
        asm mov ds, ax
        asm rep movsb
        asm pop di
        asm pop si
        asm pop es
        asm pop ds
    }
}

/* (dst, dst_seg, src, src_seg, count) dst[i] = min(dst[i], src[i]) - merges movement cost maps (cheapest wins) */
int Bytes_Min_Far(unsigned dst, unsigned dst_seg, unsigned src, unsigned src_seg, int count)
{
    if (count == 0)
        return 0;
    if (dst_seg == 0) {
        asm mov ax, ds
        asm mov dst_seg, ax
    }
    if (src_seg == 0) {
        asm mov ax, ds
        asm mov src_seg, ax
    }
    asm push ds
    asm push es
    asm push si
    asm push di
    asm mov di, dst
    asm mov si, src
    asm mov cx, count
    asm mov bx, src_seg
    asm mov ax, dst_seg
    asm mov es, ax
    asm mov ds, bx
far_min_next:
    asm lodsb
    asm cmp es:[di], al
    asm jb far_min_keep
    asm stosb
    asm dec di
far_min_keep:
    asm inc di
    asm loop far_min_next
    asm pop di
    asm pop si
    asm pop es
    asm pop ds
}

/* (dst, dst_seg (0 = DS), count) zeroes count bytes */
int Mem_Clear_Far(unsigned dst, unsigned dst_seg, int count)
{
    if (count == 0)
        return 0;
    if (dst_seg == 0) {
        asm mov ax, ds
        asm mov dst_seg, ax
    }
    asm push ds
    asm push es
    asm push si
    asm push di
    asm mov di, dst
    asm mov cx, count
    asm mov ax, dst_seg
    asm mov es, ax
    asm mov ds, ax
    asm mov al, 0
    asm rep stosb
    asm pop di
    asm pop si
    asm pop es
    asm pop ds
}

/* (dst, dst_seg (0 = DS), count, value) fills count bytes.
   Simtex name Set_Memory (MoO2 symbols, module struct); clarified using ReMoM repo */
int Mem_Fill_Far(unsigned dst, unsigned dst_seg, int count, int value)
{
    if (count == 0)
        return 0;
    if (dst_seg == 0) {
        asm mov ax, ds
        asm mov dst_seg, ax
    }
    asm push ds
    asm push es
    asm push si
    asm push di
    asm mov di, dst
    asm mov cx, count
    asm mov ax, value
    asm mov bx, dst_seg
    asm mov es, bx
    asm mov ds, bx
    asm rep stosb
    asm pop di
    asm pop si
    asm pop es
    asm pop ds
}

/* (a, b) swaps two ints. Simtex name Swap_Short (MoO2 symbols, module strings); clarified using ReMoM repo */
void Swap_Words(int *a, int *b)
{
    int t;

    t = *a;
    *a = *b;
    *b = t;
}

/* (a, b, count) swaps two near byte ranges (count 0 with a != b would run 65536 times) */
int Mem_Swap(char *a, char *b, int count)
{
    if (a == b)
        return 0;
    asm push es
    asm push si
    asm push di
    asm mov ax, ds
    asm mov es, ax
    asm mov si, a
    asm mov di, b
    asm mov cx, count
swap_next:
    asm mov bl, es:[di]
    asm movsb
    asm dec si
    asm mov es:[si], bl
    asm inc si
    asm loop swap_next
    asm pop di
    asm pop si
    asm pop es
}

/* (dst, dst_seg, src, src_seg, bytes) segments 0 = DS; an even count moves words */
int Mem_Copy_Far(unsigned dst, unsigned dst_seg, unsigned src, unsigned src_seg, unsigned bytes)
{
    if (bytes == 0)
        return 0;
    if (dst_seg == 0) {
        asm mov ax, ds
        asm mov dst_seg, ax
    }
    if (src_seg == 0) {
        asm mov ax, ds
        asm mov src_seg, ax
    }
    if (bytes & 1) {
        asm push ds
        asm push es
        asm push si
        asm push di
        asm mov ax, dst_seg
        asm mov es, ax
        asm mov di, dst
        asm mov bx, src_seg
        asm mov si, src
        asm mov cx, bytes
        asm mov ds, bx
        asm rep movsb
        asm pop di
        asm pop si
        asm pop es
        asm pop ds
    } else {
        asm push ds
        asm push es
        asm push si
        asm push di
        asm mov ax, dst_seg
        asm mov es, ax
        asm mov di, dst
        asm mov bx, src_seg
        asm mov si, src
        asm mov cx, bytes
        asm shr cx, 1
        asm mov ds, bx
        asm rep movsw
        asm pop di
        asm pop si
        asm pop es
        asm pop ds
    }
}

/* (bit, far bitmap) -> 1 if set.
   Simtex name Test_Bit_Field (MoO2 symbols, module struct); clarified using ReMoM repo */
int Bit_Test(int bit, char far *map)
{
    if (map[bit >> 3] & bit_masks[bit & 7])
        return 1;
    return 0;
}

/* (bit, far bitmap). Simtex name Set_Bit_Field (MoO2 symbols, module struct); clarified using ReMoM repo */
void Bit_Set(int bit, char far *map)
{
    int byte;

    byte = bit >> 3;
    map[byte] = map[byte] | bit_masks[bit & 7];
}

/* (bit, far bitmap). Simtex name Clear_Bit_Field (MoO2 symbols, module struct); clarified using ReMoM repo */
void Bit_Clear(int bit, char far *map)
{
    int byte;

    byte = bit >> 3;
    map[byte] = map[byte] & (0xFF - bit_masks[bit & 7]);
}

/* (bit, near bitmap) -> 0 / 1 */
int Bit_Test_Near(int bit, char *map)
{
    int result;

    asm push si
    asm mov si, map
    asm mov ax, bit
    asm mov cx, ax
    asm shr ax, 1
    asm shr ax, 1
    asm shr ax, 1
    asm add si, ax
    asm and cx, 7
    asm lodsb
    asm cmp cx, 0
    asm je test_done
test_shift:
    asm shr al, 1
    asm loop test_shift
test_done:
    asm and ax, 1
    asm mov result, ax
    asm pop si
    return result;
}

/* (bit, near bitmap) */
void Bit_Set_Near(int bit, char *map)
{
    asm mov bx, map
    asm mov ax, bit
    asm mov cx, ax
    asm shr ax, 1
    asm shr ax, 1
    asm shr ax, 1
    asm add bx, ax
    asm and cx, 7
    asm mov al, 1
    asm cmp cx, 0
    asm je set_done
set_shift:
    asm shl al, 1
    asm loop set_shift
set_done:
    asm mov cl, [bx]
    asm or al, cl
    asm mov [bx], al
}

/* (bit, near bitmap) no callers */
void Bit_Toggle_Near(int bit, char *map)
{
    asm mov bx, map
    asm mov ax, bit
    asm mov cx, ax
    asm shr ax, 1
    asm shr ax, 1
    asm shr ax, 1
    asm add bx, ax
    asm and cx, 7
    asm mov al, 1
    asm cmp cx, 0
    asm je toggle_done
toggle_shift:
    asm shl al, 1
    asm loop toggle_shift
toggle_done:
    asm mov cl, [bx]
    asm xor al, cl
    asm mov [bx], al
}

/* (box color, unused, font, font color) for Dev_Choice_Box; no callers */
void Dev_Choice_Box_Style(int color, int unused, int font, int font_color)
{
    dev_box_font = font;
    dev_box_color = color;
    dev_box_unused = unused;
    dev_box_font_color = font_color;
}

/* (title, option1, option2, option3, result_cancel, result1, result2, result3) a full-screen box with up to three
   option buttons (empty text = no button); returns the chosen option's result (cancel: a click outside or ESC).
   No callers. The buttons' help argument is the string "NO_HELP" (a pointer, where the help index belongs) */
int Dev_Choice_Box(char *title, char *option1, char *option2, char *option3, int result_cancel, int result1,
                   int result2, int result3)
{
    int done;
    int ctrl1;
    int ctrl2;
    int ctrl3;
    int ctrl_screen;
    int input;

    done = 0;
    ctrl_screen = UI_NO_CONTROL;
    ctrl1 = UI_NO_CONTROL;
    ctrl2 = UI_NO_CONTROL;
    ctrl3 = UI_NO_CONTROL;
    UI_Suspend_Redraw_Fn();
    while (done == 0) {
        input = UI_Get_Input();
        if (input == ctrl_screen || input == -1)
            return result_cancel;
        if (input == ctrl1)
            return result1;
        if (input == ctrl2)
            return result2;
        if (input == ctrl3)
            return result3;
        Set_Draw_Page();
        Fill_Rect(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, 0);
        Draw_Double_Bevel_Box(5, 5, 315, 199, dev_box_color, dev_box_color, dev_box_color, dev_box_color);
        Draw_Double_Bevel_Box(100, 90, 220, 110, dev_box_color, dev_box_color, dev_box_color, dev_box_color);
        Font_Select(dev_box_font, dev_box_font_color);
        Print_Text_Centered(160, 98, title);
        UI_Clear_Controls();
        if (*option1 != 0) {
            Draw_Double_Bevel_Box(120, 130, 200, 145, dev_box_color, dev_box_color, dev_box_color, dev_box_color);
            Print_Text_Centered(160, 136, option1);
            ctrl1 = UI_Add_Region(120, 130, 200, 145, "", "NO_HELP");
        }
        if (*option2 != 0) {
            Draw_Double_Bevel_Box(120, 150, 200, 165, dev_box_color, dev_box_color, dev_box_color, dev_box_color);
            Print_Text_Centered(160, 156, option2);
            ctrl2 = UI_Add_Region(120, 150, 200, 165, "", "NO_HELP");
        }
        if (*option3 != 0) {
            Draw_Double_Bevel_Box(120, 170, 200, 185, dev_box_color, dev_box_color, dev_box_color, dev_box_color);
            Print_Text_Centered(160, 176, option3);
            ctrl3 = UI_Add_Region(120, 170, 200, 185, "", "NO_HELP");
        }
        ctrl_screen = UI_Add_Region(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, "", -1);
        UI_Present_Frame();
    }
}

/* (x, y, steps, pic) zoom-in transition: the drawn screen is grabbed into the picture at pic + 1 (cached in EMS), then
   drawn at 1/steps .. steps/steps of full size, growing from (x, y) toward the top-left corner, one step per tick */
void Screen_Transition_Zoom(int x, int y, int steps, unsigned pic)
{
    int left;
    int top;
    int right;
    int bottom;
    int percent;
    register unsigned pic_seg;
    register int i;

    UI_Clear_Controls();
    pic_seg = pic + 1;                          /* the bitmap after the block header ... */
    Pic_Create_Blank(SCREEN_W, SCREEN_H, pic_seg);
    Set_Draw_Page();
    Screen_Transition_Grab(pic_seg + 1);        /* ... and its pixels after the bitmap's 16-byte header */
    Screen_Copy_Front();
    EMS_Write(0, pic_seg, 0L, 32000, ems_vgafile_handle);
    EMS_Write(0, pic_seg + 0x7d0, 32000L, 32000, ems_vgafile_handle);   /* the second half: 0x7d0 paragraphs
                                                   = 32000 bytes on */
    for (i = 0; i < steps; i++) {
        Timer_Mark();
        EMS_Read(0, pic_seg, 0L, 32000, ems_vgafile_handle);
        EMS_Read(0, pic_seg + 0x7d0, 32000L, 32000, ems_vgafile_handle);
        left = x - (i + 1) * x / steps;
        top = y - (i + 1) * y / steps;
        percent = (i + 1) * 100 / steps;
        right = percent * 32 / 10 + left;      /* percent of the 320 x 200 screen: x 32 / 10, x 2 */
        bottom = percent * 2 + top;
        if (right > SCREEN_W)
            left = SCREEN_W - percent * 32 / 10;
        if (bottom > SCREEN_H)
            top = SCREEN_H - percent * 2;
        Pic_Scale_Percent(pic_seg, percent, percent);
        Pic_Replace_Color(pic_seg, 0, 1);
        Set_Draw_Page();
        Draw_Bitmap(left, top, pic_seg);
        UI_Present_Frame();
        Wait_Ticks(1);
    }
}

/* (dest_seg) copies the 4 planes of the draw page into dest_seg:0 column by column (320 columns of 200 bytes) */
void Screen_Transition_Grab(unsigned dest_seg)
{
    asm push ds
    asm push es
    asm push si
    asm push di
    asm mov ax, dest_seg
    asm mov es, ax
    asm mov ax, draw_page_seg
    asm mov ds, ax
    asm mov bx, 320
    asm mov ah, 0
    asm mov si, 0
    asm mov di, 0
grab_column:
    asm push si
    asm mov al, 4
    asm mov dx, 3CEh                       /* the VGA graphics controller index */
    asm out dx, al
    asm inc dx
    asm mov al, ah
    asm out dx, al
    asm mov cx, 200
grab_pixel:
    asm lodsb
    asm stosb
    asm add si, 79
    asm loop grab_pixel
    asm pop si
    asm inc ah
    asm cmp ah, 4
    asm js grab_same
    asm mov ah, 0
    asm inc si
grab_same:
    asm dec bx
    asm jne grab_column
    asm pop di
    asm pop si
    asm pop es
    asm pop ds
}
