/* HELPSCR.C - WIZARDS.EXE code segment f23a: the right-click help popup (HELP.LBX record chains).
   This module's own data (DS:7118..712b): the two text palettes, then its literals "HELP" and "help".
   Mem_Copy_Far is called WITHOUT a prototype: the 32 KB copies pass the decimal constant 32768, which C types as a
   long (4 bytes pushed), while the 4000-byte copies push an int - the callee reads the low word either way.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS f23a src/wizards/HELPSCR.C */
#include "GAME.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
void Screen_Help_Popup(int entry);
void Help_Redraw_Nothing(void);
int EMS_Map_Backup_Frame(void);

extern char near_heap[];                        /* DS:d49a */
extern unsigned ems_frame_seg;                  /* DS:760c */
extern int ems_vgafile_handle;                   /* DS:e5f4 */
extern int ui_control_count;                    /* DS:e8a2 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
struct HelpRecord *p_help_record;               /* DS:d494  MAGIC DS:948a   */

char help_title_colors[5] = { 0xb9, 0x2f, 0x2f, 0x2f, 0x2f };
char help_text_colors[5] = { 0xb8, 0x37, 0x37, 0x37, 0x37 };

void Screen_Copy_Front(void);
void Near_Alloc_Mark(void);
void Near_Alloc_Release(void);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Pic_Reset_Frame(unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Get_Opaque_Bounds(unsigned pic, int *x, int *y, int *w, int *h);
void Pic_Set_Remap_Flag(unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Paragraph_Height(int width, char *text);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void UI_Save_Redraw_Fn(void);
void UI_Restore_Redraw_Fn(void);
int UI_Wait_Any_Input(void);
void EMS_Map_Pages(int first, int handle);

/* (entry) the help pop-up: follows the HELP.LBX record chain from entry (next 0 ends it, -1 the following record,
   else a jump), measures the records' pictures for the row height, draws the scroll's top and bottom around the
   body, then each record's picture (entries below 250 with the remap flag) and title / text; waits for a click.
   The scratch block and 4000 near bytes are saved and restored around it.
   Simtex name Draw_Help_Entry (MoO2 symbols, module TEXTBOX); clarified using ReMoM repo */
void Screen_Help_Popup(int entry)
{
    unsigned top_pic;
    unsigned bottom_pic;
    int body_h;
    int backup_seg;
    int i;
    int top_h;
    int bottom_h;
    int x;
    int y;
    int y2;
    int count;
    int title_x;
    int title_dy;
    int row_h;
    int next;
    int max_w;
    int max_h;
    int pic_x;
    int pic_y;                                  /* reused as the pictures' row offset */
    int min_row_h;
    unsigned front;
    int cur_entry;
    int pic_h;
    int pic_w;
    int saved_controls;
    int pic_y2;
    unsigned back;
    unsigned pics[10];
    int cy;
    int record;

    Screen_Copy_Front();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    backup_seg = EMS_Map_Backup_Frame();
    Mem_Copy_Far(0, backup_seg, 0, p_scratch, 32768);   /* the 64 KB scratch in two 32 KB halves, 0x800 paragraphs apart */
    Mem_Copy_Far(0, backup_seg + 0x800, 0, p_scratch + 0x800, 32768);
    saved_controls = ui_control_count;
    ui_control_count = 0;
    front = Heap_Alloc_Front(p_scratch, 0xfc);  /* 4032 bytes: the 4000 near bytes saved below */
    Mem_Copy_Far(0, front, near_heap, 0, 4000);
    back = Heap_Alloc_Back(p_scratch, 0xfa);    /* 4000 bytes: a decoded help picture */
    Near_Alloc_Mark();
    p_help_record = (struct HelpRecord *)Near_Alloc_Reset(sizeof(struct HelpRecord));
    count = 0;
    next = 1;
    max_w = 0;
    max_h = 0;
    record = entry;
    while (next != 0) {
        LBX_Load_Records_Near("HELP", 2, p_help_record, record, 1, sizeof(struct HelpRecord));
        next = p_help_record->next;
        if (next == -1)
            record++;
        else
            record = next;
        if (p_help_record->pic_lbx[0] != 0) {
            pics[count] = LBX_Reload_Next(p_help_record->pic_lbx, p_help_record->pic_index, p_scratch);
            Pic_Reset_Frame(pics[count]);
            Pic_Decode_Frame_To_Bitmap(pics[count], back);
            Pic_Get_Opaque_Bounds(back, &pic_x, &pic_y, &pic_w, &pic_h);
            if (pic_w > max_w)
                max_w = pic_w;
            if (pic_h > max_h)
                max_h = pic_h;
        } else
            pics[count] = 0;
        count++;
    }
    if (max_w != 0)
        max_w += 6;
    min_row_h = 9;                              /* the title row's height (the title prints in font 4) */
    if (max_h > min_row_h) {
        title_dy = (max_h - min_row_h) / 2;
        pic_y = 0;
        row_h = max_h + 2;
    } else {
        pic_y = (min_row_h - max_h) / 2;
        title_dy = 0;
        row_h = min_row_h + 2;
    }
    title_x = max_w;
    top_pic = LBX_Reload_Next("help", 0, p_scratch);
    bottom_pic = LBX_Reload_Next("help", 1, p_scratch);
    cy = 2;
    record = entry;
    for (i = 0; i < count; i++) {
        Font_Select_Palette(1, help_text_colors);
        LBX_Load_Records_Near("help", 2, p_help_record, record, 1, sizeof(struct HelpRecord));
        if (p_help_record->text[0] == 0)
            cy += row_h + 4;
        else
            cy += row_h + Paragraph_Height(180, p_help_record->text) + 6;
        next = p_help_record->next;
        if (next == -1)
            record++;
        else
            record = next;
    }
    cy -= 5;
    body_h = cy;
    top_h = 24;
    bottom_h = 24;
    x = 50;
    y = 100 - (body_h + top_h + bottom_h) / 2;
    y2 = y + top_h + body_h - 1;
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, 319, y2);
    Draw_Pic_Clipped(x, y, top_pic);
    Reset_Clip_Rect();
    Draw_Pic_Clipped(x, y + top_h + body_h, bottom_pic);
    Reset_Clip_Rect();
    cy = y + top_h + 2;
    record = entry;
    for (i = 0; i < count; i++) {
        cur_entry = record;
        LBX_Load_Records_Near("help", 2, p_help_record, record, 1, sizeof(struct HelpRecord));
        next = p_help_record->next;
        if (next == -1)
            record++;
        else
            record = next;
        if (pics[i] != 0) {
            pics[i] = LBX_Reload_Next(p_help_record->pic_lbx, p_help_record->pic_index, p_scratch);
            Pic_Reset_Frame(pics[i]);
            Pic_Decode_Frame_To_Bitmap(pics[i], back);
            Pic_Get_Opaque_Bounds(back, &pic_x, &pic_y2, &pic_w, &pic_h);
            if (cur_entry < 250)                /* the entries below 250: their pictures remapped */
                Pic_Set_Remap_Flag(back);
            Draw_Bitmap(x + (max_w - pic_w - 6) / 2 + 20 - pic_x, cy + pic_y - pic_y2, back);
        }
        Font_Select_Palette(4, help_title_colors);
        Print_Text(title_x + x + 18, cy + title_dy, p_help_record->title);
        Font_Select_Palette(1, help_text_colors);
        cy += row_h;
        Paragraph_Print(x + 20, cy, 180, p_help_record->text, PARA_LEFT);
        if (p_help_record->text[0] == 0)
            cy += 4;
        else
            cy += Paragraph_Height(180, p_help_record->text) + 6;
    }
    cy -= 5;
    UI_Present_Frame();
    Screen_Copy_Front();
    UI_Save_Redraw_Fn();
    UI_Set_Redraw_Fn(Help_Redraw_Nothing, 1);
    UI_Wait_Any_Input();
    UI_Restore_Redraw_Fn();
    Mem_Copy_Far(near_heap, 0, 0, front, 4000);
    Near_Alloc_Release();
    backup_seg = EMS_Map_Backup_Frame();
    Mem_Copy_Far(0, p_scratch, 0, backup_seg, 32768);
    Mem_Copy_Far(0, p_scratch + 0x800, 0, backup_seg + 0x800, 32768);
    ui_control_count = saved_controls;
}

/* () the empty redraw callback installed while the help popup waits */
void Help_Redraw_Nothing(void)
{
}

/* () maps the VGAFILEH handle's pages 1..4 into the page frame (EMS_Map_Pages maps four pages from the first)
   and returns the frame segment */
int EMS_Map_Backup_Frame(void)
{
    EMS_Map_Pages(1, ems_vgafile_handle);
    return ems_frame_seg;
}
