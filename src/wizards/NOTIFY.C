/* NOTIFY.C - WIZARDS.EXE code segment d677 (MAGIC.EXE 4d5b, the same file): the pop-up boxes - the Yes / No
   confirm box, the message and warning boxes, the pick-one list dialog, the notification boxes with pictures and
   the human's realm gem, the zooming help window with its connector lines, and saving the scratch block to EMS
   around them. All pictures come from RESOURCE.LBX.
   Does NOT include GAME.H: Font_Select is called with 2 and 3 arguments (no prototype in the original).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS d677 src/wizards/NOTIFY.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     Font_Select_Shadow (2; TEXT.C takes 4);
     Fill_Box_Styled (5; GRAPHICS.C takes 9). */
#pragma warn -pro
#include "STRUCTS.H"
#include "ENUMS.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
int Confirm_Box(char *text);
void Confirm_Box_Redraw(void);
void Warning_Box(char *text);
void Message_Box(char *text);
void Message_Box_Common(register char *text);
void Message_Box_Redraw(void);
int List_Select_Dialog(int count, char **names, int more, char *title);
void List_Select_Dialog_Redraw(void);
void List_Select_Layout(int count, char **names, char *title);
void Notify_Box_Load(unsigned style, unsigned pic);
void Notify_Picture_Box_Centered(int center_x, int y, register int style, char *text, int use_gem_frame, register unsigned pic1, int pic1_x, int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
void Notify_Picture_Box_Center_Redraw(void);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
void Notify_Picture_Box_Redraw(void);
void Window_Zoom_Open_In_Place(register int x1, register int y1, int x2, int y2, int src_x1, int src_y1, int src_x2, int src_y2, int lines);
int Window_Zoom_Open(int dst_x1, int dst_y1, int dst_x2, int dst_y2, int img_x1, int img_y1, int img_x2, int img_y2, int src_x1, int src_y1, int src_x2, int src_y2, int lines);
int Window_Zoom_Lines_Redraw(void);
void Window_Zoom_Draw(register int from_x1, int from_y1, int from_x2, int from_y2, int to_x1, int to_y1, int to_x2, int to_y2, int col_a1, int col_a2, int col_b1, int col_b2, int col_c1, int col_c2);
void Zoom_Step_Rect(int cx, int cy, unsigned pic, register int steps, int *x, int *y);
void Zoom_Reset(void);
void Scratch_Save_To_EMS(void);
void Scratch_Restore_From_EMS(void);

extern unsigned notify_realm_pics[5];           /* DS:9408 */
extern unsigned p_pic_work;                     /* DS:998c */
extern unsigned p_scratch;                      /* DS:998e */
extern struct Wizard wizards[6];                /* DS:9eca */
extern struct GameConfig game_config;           /* DS:bd9e */
extern unsigned ems_frame_seg;                  /* DS:760c */
extern int ems_vgafile_handle;                   /* DS:e5f4 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int mapdraw_saved_view_y;                     /* DS:cb56  MAGIC DS:9116  WIZARDS MapDraw_State_Save / _Restore keep map_drawn_view_y here (batch 40; was "spare") */
int msgbox_is_warning;                          /* DS:cb54  MAGIC DS:9114   */
int notify_box_x;                               /* DS:cb52  MAGIC DS:9112   */
int notify_box_y;                               /* DS:cb50  MAGIC DS:9110   */
char *notify_text;                              /* DS:cb4e  MAGIC DS:910e   */
int notify_realm;                               /* DS:cb4c  MAGIC DS:910c   */
unsigned list_dialog_left_pic;                  /* DS:cb4a  MAGIC DS:910a   */
unsigned list_dialog_corner_bl_pic;             /* DS:cb48  MAGIC DS:9108   */
unsigned list_dialog_top_pic;                   /* DS:cb46  MAGIC DS:9106   */
unsigned list_dialog_bottom_pic;                   /* DS:cb44  MAGIC DS:9104   */
unsigned list_dialog_right_pic;                 /* DS:cb42  MAGIC DS:9102   */
unsigned list_dialog_corner_br_pic;             /* DS:cb40  MAGIC DS:9100   */
unsigned list_dialog_scroll_pic;                  /* DS:cb3e  MAGIC DS:90fe   */
unsigned list_dialog_row_pics[10];              /* DS:cb2a  MAGIC DS:90ea   */
unsigned list_dialog_row_end_pics[10];          /* DS:cb16  MAGIC DS:90d6   */
/* list_dialog_row_end_pics: RESOURCE 22, the right end piece beside each row (frame 1 when highlighted) */
unsigned list_dialog_top_arrow_pic;                      /* DS:cb14  MAGIC DS:90d4   */
unsigned list_dialog_bottom_arrow_pic;                       /* DS:cb12  MAGIC DS:90d2   */
unsigned list_dialog_scroll_top_pic;            /* DS:cb10  MAGIC DS:90d0   */
unsigned list_dialog_scroll_bottom_pic;         /* DS:cb0e  MAGIC DS:90ce   */
unsigned list_dialog_up_locked_pic;                    /* DS:cb0c  MAGIC DS:90cc   */
unsigned list_dialog_down_locked_pic;                  /* DS:cb0a  MAGIC DS:90ca   */
unsigned msgbox_top_pic;                        /* DS:cb08  MAGIC DS:90c8   */
unsigned msgbox_bottom_pic;                     /* DS:cb06  MAGIC DS:90c6   */
unsigned warnbox_top_pic;                       /* DS:cb04  MAGIC DS:90c4   */
unsigned warnbox_bottom_pic;                    /* DS:cb02  MAGIC DS:90c2   */
int notify_color_step;                          /* DS:cb00  MAGIC DS:90c0   */
int notify_color_cycle;                         /* DS:cafe  MAGIC DS:90be   */
unsigned notify_box_pics[4];                    /* DS:caf6  MAGIC DS:90b6   */
unsigned notify_pict_box_pics[4];                  /* DS:caee  MAGIC DS:90ae   */
unsigned notify_gem_box_pics[4];              /* DS:cae6  MAGIC DS:90a6   */
int notify_style_spare;                         /* DS:cae4  MAGIC DS:90a4  no code in either program reads it */
int notify_style;                               /* DS:cae2  MAGIC DS:90a2   */
unsigned notify_pic1;                           /* DS:cae0  MAGIC DS:90a0   */
unsigned notify_pic2;                           /* DS:cade  MAGIC DS:909e   */
int notify_use_gem_frame;                       /* DS:cadc  MAGIC DS:909c   */
int notify_pic1_dx;                              /* DS:cada  MAGIC DS:909a   */
int notify_pic1_dy;                              /* DS:cad8  MAGIC DS:9098   */
int notify_pic2_dy;                             /* DS:cad6  MAGIC DS:9096   */
int notify_pic2_dx;                             /* DS:cad4  MAGIC DS:9094   */
unsigned confirm_box_top_pic;                   /* DS:cad2  MAGIC DS:9092   */
unsigned confirm_box_bottom_pic;                /* DS:cad0  MAGIC DS:9090   */
unsigned confirm_right_button_pic;              /* DS:cace  MAGIC DS:908e   */
unsigned confirm_left_button_pic;               /* DS:cacc  MAGIC DS:908c   */
int list_dialog_rows;                           /* DS:caca  MAGIC DS:908a   */
int list_dialog_more;                           /* DS:cac8  MAGIC DS:9088   */
int list_dialog_x;                              /* DS:cac6  MAGIC DS:9086   */
int list_dialog_y;                              /* DS:cac4  MAGIC DS:9084   */
int list_dialog_x2;                             /* DS:cac2  MAGIC DS:9082   */
int list_dialog_y2;                             /* DS:cac0  MAGIC DS:9080   */
int list_dialog_last_row_spare;                 /* DS:cabe  MAGIC DS:907e  no code in either program reads it */
int list_dialog_highlight_row;                  /* DS:cabc  MAGIC DS:907c   */
int list_dialog_first;                          /* DS:caba  MAGIC DS:907a   */
int list_dialog_count;                          /* DS:cab8  MAGIC DS:9078   */
char **list_dialog_names;                       /* DS:cab6  MAGIC DS:9076   */
char *list_dialog_title;                        /* DS:cab4  MAGIC DS:9074   */
int list_dialog_row_ctls[10];                   /* DS:caa0  MAGIC DS:9060   */
int zoom_dst_x1;                                /* DS:ca9e  MAGIC DS:905e   */
int zoom_dst_y1;                                /* DS:ca9c  MAGIC DS:905c   */
int zoom_dst_x2;                                /* DS:ca9a  MAGIC DS:905a   */
int zoom_dst_y2;                                /* DS:ca98  MAGIC DS:9058   */
int zoom_src_x1;                                /* DS:ca96  MAGIC DS:9056   */
int zoom_src_y1;                                /* DS:ca94  MAGIC DS:9054   */
int zoom_src_x2;                                /* DS:ca92  MAGIC DS:9052   */
int zoom_src_y2;                                /* DS:ca90  MAGIC DS:9050   */
int zoom_lines;                                 /* DS:ca8e  MAGIC DS:904e   */

char notify_shadow_colors[5] = { 0x11, 0x11, 0x11, 0x11, 0x11 };        /* DS:6f78 */
char confirm_text_colors[5] = { 0x06, 0x09, 0x0b, 0x0d, 0x0f };         /* DS:6f7d */
char list_title_colors[5] = { 0x14, 0xb2, 0xb3, 0xb4, 0xb4 };           /* DS:6f82 */
char list_title_shadow_colors[5] = { 0x66, 0xfb, 0xfb, 0xfb, 0xfb };    /* DS:6f87 */
char msgbox_colors[5] = { 0xe4, 0xe4, 0xe4, 0xe4, 0xe4 };               /* DS:6f8c */
char warnbox_colors[5] = { 0x2d, 0x2d, 0x2d, 0x2d, 0x2d };              /* DS:6f91 */
int confirm_box_dx = 0;                         /* DS:6f96  x offset of the confirm box (ReMoM confirmation_box_x_offset; clarified using ReMoM repo) */
int confirm_box_dy = 0;                         /* DS:6f98  y offset of the confirm box */
int notify_box_mode = 0;                        /* DS:6f9a */
/* notify_box_mode: 0 = draw and wait for input, 1 = draw once (the centred redraw) and return, 2 = copy the front
   page first; only 0 is ever stored (LAIR.C), so 1 and 2 are dead in 1.31 */
char notify_shadow_palettes[4][5] = {
    { 0xf0, 0xf0, 0xf0, 0xf0, 0xf0 }, { 0xf0, 0xf0, 0xf0, 0xf0, 0xf0 }, { 0xf0, 0xf0, 0xf0, 0xf0, 0xf0 },
    { 0x14, 0x14, 0x14, 0x14, 0x14 } };         /* DS:6f9c  text shadow palette per box style */
char notify_text_palettes[4][5] = {
    { 0x19, 0xb2, 0xb3, 0xb4, 0xb4 }, { 0x19, 0xb2, 0xb3, 0xb4, 0xb4 }, { 0x19, 0xb2, 0xb3, 0xb4, 0xb4 },
    { 0x19, 0xb2, 0xb3, 0xb4, 0xb4 } };         /* DS:6fb0  text palette per box style */
int notify_gem_dx[5] = { -2, 0, -22, 8, 2 };    /* DS:6fc4  realm gem offset from the box, per GEM_* */
int notify_gem_dy[5] = { 2, -20, -4, -12, -3 }; /* DS:6fce */
int notify_text_dx[5] = { 70, 73, 62, 68, 70 }; /* DS:6fd8  text start per realm gem */
int zoom_step = 1;                              /* DS:6fe2 */

#define CYCLE1_FIRST 0xf4                      /* color_cycle 1 rotates colors F4h .. FEh */
#define CYCLE1_COUNT 11
#define CYCLE2_FIRST 0xf7                      /* color_cycle 2 rotates colors F7h .. FEh */
#define CYCLE2_COUNT 8
#define ZOOM_LINE_COLORS 0xb4, 0xb5, 0xb9, 0xba, 0xbb, 0xba   /* Window_Zoom_Draw's three color pairs */

void Window_Zoom_Draw(int from_x1, int from_y1, int from_x2, int from_y2, int to_x1, int to_y1, int to_x2, int to_y2,
                      int col_a1, int col_a2, int col_b1, int col_b2, int col_c1, int col_c2);
long Random_Get_Seed(void);
void Font_Select_Shadow();                     /* no prototype here: some calls pass 2 or 3 of the 4 words (QUIRK #371) */
void Fill_Box_Styled();                           /* no prototype here: called with 5 or 6 words (the definition in GRAPHICS.C takes 9, read by style) */

/* (text) the Yes / No box with its own input loop -> 1 Yes, 0 No. QUIRK #389: abs() turns a -1 input into 1, the id of
   yes_ctl (the first control added): a -1 answers Yes; the full-screen Esc control is never tested.
   Simtex name Confirmation_Box (MoO2 symbols, module GENDRAW); clarified using ReMoM repo
   Numbered: BUG #389 (BUGS_AND_QUIRKS.txt) */
int Confirm_Box(char *text)
{
    int total;
    int input;
    int yes_ctl;
    int no_ctl;
    int screen_ctl;
    int box_ctl;
    int answer;
    register int height;
    register int done;

    Font_Save_Colors();
    Font_Select_Palette(0, confirm_text_colors);
    Scratch_Save_To_EMS();
    confirm_box_top_pic = LBX_Reload("RESOURCE", 0, p_scratch);
    confirm_box_bottom_pic = LBX_Reload_Next("RESOURCE", 1, p_scratch);
    confirm_right_button_pic = LBX_Reload_Next("RESOURCE", 3, p_scratch);
    confirm_left_button_pic = LBX_Reload_Next("RESOURCE", 4, p_scratch);
    Screen_Copy_Front();
    notify_text = text;
    Font_Select(4, 4, 15);
    height = Paragraph_Height(166, text);
    total = height + 34;
    notify_box_x = confirm_box_dx + 68;
    notify_box_y = (200 - total) / 2 + confirm_box_dy;
    UI_Clear_Controls();
    yes_ctl = UI_Add_Button(notify_box_x + 101, notify_box_y + height + 15, "", confirm_right_button_pic, "Y", -1);
    no_ctl = UI_Add_Button(notify_box_x + 18, notify_box_y + height + 15, "", confirm_left_button_pic, "N", -1);
    box_ctl = UI_Add_Region(notify_box_x, notify_box_y, notify_box_x + 185, notify_box_y + 63, "", -1);
    screen_ctl = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    UI_Set_Redraw_Fn(Confirm_Box_Redraw, 1);
    done = 0;
    while (done == 0) {
        input = abs(UI_Get_Input());
        if (input == -1 || input == no_ctl) {
            done = 1;
            answer = 0;
        }
        if (input == yes_ctl) {
            done = 1;
            answer = 1;
        }
        Confirm_Box_Redraw();
        Screen_Present();
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Font_Restore_Colors();
    Reset_Clip_Rect();
    Scratch_Restore_From_EMS();
    return answer;
}

/* () the box top clipped to the text height, the bottom under it, the text three times (shadow, then face) */
void Confirm_Box_Redraw(void)
{
    int height;

    Font_Select(4, 4, 15);
    height = Paragraph_Height(166, notify_text);
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, 319, notify_box_y + height + 12);
    Draw_Pic_Clipped(notify_box_x, notify_box_y, confirm_box_top_pic);
    Reset_Clip_Rect();
    Draw_Pic_Frame(notify_box_x, notify_box_y + height + 10, confirm_box_bottom_pic);
    Font_Select_Palette(4, notify_shadow_colors);
    Font_Select(4, 15, 15);
    Paragraph_Print(notify_box_x + 10, notify_box_y + 10, 166, notify_text, PARA_LEFT);
    Paragraph_Print(notify_box_x + 9, notify_box_y + 10, 166, notify_text, PARA_LEFT);
    Set_Font_Color(18);
    Font_Select_Palette(4, confirm_text_colors);
    Font_Select(4, 4, 15);
    Paragraph_Print(notify_box_x + 9, notify_box_y + 9, 166, notify_text, PARA_LEFT);
}

/* (text) the red warning box */
void Warning_Box(char *text)
{
    msgbox_is_warning = 1;
    Message_Box_Common(text);
}

/* (text) the message box */
void Message_Box(char *text)
{
    msgbox_is_warning = 0;
    Message_Box_Common(text);
}

/* (text) the message / warning box: centred, waits for any input */
void Message_Box_Common(register char *text)
{
    int height;
    int total;
    int unused;

    Scratch_Save_To_EMS();
    msgbox_top_pic = LBX_Reload("RESOURCE", 38, p_scratch);
    msgbox_bottom_pic = LBX_Reload_Next("RESOURCE", 39, p_scratch);
    warnbox_top_pic = LBX_Reload_Next("RESOURCE", 49, p_scratch);
    warnbox_bottom_pic = LBX_Reload_Next("RESOURCE", 50, p_scratch);
    UI_Save_Redraw_Fn();
    Font_Save_Colors();
    Font_Select_Palette(0, confirm_text_colors);
    Screen_Copy_Front();
    notify_text = text;
    Font_Select(4, 4, 4);
    height = Paragraph_Height(166, text);
    total = height + 33;
    notify_box_x = 68;
    notify_box_y = (200 - total) / 2;
    UI_Set_Redraw_Fn(Message_Box_Redraw, 1);
    unused = 0;
    Play_Message_Sound();
    UI_Wait_Any_Input();
    Font_Restore_Colors();
    Reset_Clip_Rect();
    UI_Restore_Redraw_Fn();
    Scratch_Restore_From_EMS();
}

/* () the message / warning box frame, text (shadow + face) and bottom */
void Message_Box_Redraw(void)
{
    int height;

    Font_Select(4, 4, 4);
    height = Paragraph_Height(166, notify_text);
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, 319, notify_box_y + height + 12);
    if (msgbox_is_warning == 0)
        Draw_Pic_Clipped(notify_box_x, notify_box_y, msgbox_top_pic);
    else
        Draw_Pic_Clipped(notify_box_x, notify_box_y, warnbox_top_pic);
    Reset_Clip_Rect();
    if (msgbox_is_warning == 0)
        Font_Select_Palette(4, msgbox_colors);
    else
        Font_Select_Palette(4, warnbox_colors);
    Font_Select(4, 15, 15);
    Paragraph_Print(notify_box_x + 11, notify_box_y + 11, 166, notify_text, PARA_CENTER);
    Paragraph_Print(notify_box_x + 10, notify_box_y + 11, 166, notify_text, PARA_CENTER);
    Set_Font_Color(0xb8);
    Font_Select(4, 4, 4);
    Paragraph_Print(notify_box_x + 10, notify_box_y + 10, 166, notify_text, PARA_CENTER);
    if (msgbox_is_warning == 0)
        Draw_Pic_Frame(notify_box_x, notify_box_y + height + 10, msgbox_bottom_pic);
    else
        Draw_Pic_Frame(notify_box_x, notify_box_y + height + 10, warnbox_bottom_pic);
}

/* (count, names, more, title) pick one of the names (an "" entry ends the list; count rows visible, scrolling with
   the up / down buttons when more = 1) -> the index picked, -1 for Esc / a click outside */
int List_Select_Dialog(int count, char **names, int more, char *title)
{
    int done;
    int up_ctl;
    int down_ctl;
    int cancel_ctl;
    int picked;
    int box_ctl;
    int i;
    int input;

    Font_Save_Colors();
    Screen_Copy_Front();
    Screen_Save_A800();
    Scratch_Save_To_EMS();
    list_dialog_left_pic = LBX_Reload("RESOURCE", 5, p_scratch);
    list_dialog_corner_bl_pic = LBX_Reload_Next("RESOURCE", 6, p_scratch);
    list_dialog_top_pic = LBX_Reload_Next("RESOURCE", 7, p_scratch);
    list_dialog_bottom_pic = LBX_Reload_Next("RESOURCE", 9, p_scratch);
    list_dialog_right_pic = LBX_Reload_Next("RESOURCE", 8, p_scratch);
    list_dialog_corner_br_pic = LBX_Reload_Next("RESOURCE", 10, p_scratch);
    list_dialog_scroll_pic = LBX_Reload_Next("RESOURCE", 11, p_scratch);
    list_dialog_top_arrow_pic = LBX_Reload_Next("RESOURCE", 32, p_scratch);
    list_dialog_bottom_arrow_pic = LBX_Reload_Next("RESOURCE", 33, p_scratch);
    list_dialog_scroll_top_pic = LBX_Reload_Next("RESOURCE", 34, p_scratch);
    list_dialog_scroll_bottom_pic = LBX_Reload_Next("RESOURCE", 35, p_scratch);
    list_dialog_up_locked_pic = LBX_Reload_Next("RESOURCE", 36, p_scratch);
    list_dialog_down_locked_pic = LBX_Reload_Next("RESOURCE", 37, p_scratch);
    for (i = 0; i < 5; i++) {
        list_dialog_row_pics[i] = LBX_Reload_Next("RESOURCE", i + 12, p_scratch);
        list_dialog_row_end_pics[i] = LBX_Reload_Next("RESOURCE", 22, p_scratch);
    }
    for (i = 5; i < 10; i++) {
        list_dialog_row_pics[i] = list_dialog_row_pics[i - 5];
        list_dialog_row_end_pics[i] = list_dialog_row_end_pics[i - 5];
    }
    list_dialog_more = more;
    list_dialog_count = count;
    list_dialog_names = names;
    list_dialog_title = title;
    list_dialog_first = 0;
    list_dialog_highlight_row = 0;
    List_Select_Layout(count, list_dialog_names, title);
    UI_Set_Redraw_Fn(List_Select_Dialog_Redraw, 1);
    done = 0;
    up_ctl = UI_NO_CONTROL;
    down_ctl = UI_NO_CONTROL;
    for (i = 0; i < 10; i++)
        list_dialog_row_ctls[i] = UI_NO_CONTROL;
    cancel_ctl = UI_NO_CONTROL;
    while (done == 0) {
        input = UI_Get_Input();
        if (input == cancel_ctl) {
            done = 1;
            picked = -1;
        }
        for (i = 0; i < list_dialog_count && i < list_dialog_rows; i++)
            if (list_dialog_row_ctls[i] == input) {
                done = 1;
                picked = i + list_dialog_first;
            }
        if (input == up_ctl)
            list_dialog_first--;
        if (input == down_ctl)
            list_dialog_first++;
        Set_Draw_Page();
        List_Select_Dialog_Redraw();
        UI_Clear_Controls();
        up_ctl = UI_NO_CONTROL;
        down_ctl = UI_NO_CONTROL;
        for (i = 0; i < 10; i++)
            list_dialog_row_ctls[i] = UI_NO_CONTROL;
        for (i = 0; i < list_dialog_count && i < list_dialog_rows; i++)
            if (list_dialog_more == 1)
                list_dialog_row_ctls[i] =
                    UI_Add_Region(list_dialog_x + Pic_Width(list_dialog_left_pic) + Pic_Width(list_dialog_scroll_pic),
                                   list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i,
                                   list_dialog_x2 - Pic_Width(list_dialog_right_pic),
                                   list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * (i + 1) - 1,
                                   "", -1);
            else
                list_dialog_row_ctls[i] =
                    UI_Add_Region(list_dialog_x + Pic_Width(list_dialog_left_pic),
                                   list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i,
                                   list_dialog_x2 - Pic_Width(list_dialog_right_pic),
                                   list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * (i + 1) - 1,
                                   "", -1);
        if (list_dialog_more == 1) {
            if (list_dialog_first > 0)
                up_ctl = UI_Add_Button(list_dialog_x + Pic_Width(list_dialog_left_pic) + 4,
                                       list_dialog_y + Pic_Height(list_dialog_top_pic) + 3, "", list_dialog_top_arrow_pic, "", -1);
            if (list_dialog_first + count < list_dialog_rows)
                down_ctl = UI_Add_Button(list_dialog_x + Pic_Width(list_dialog_left_pic) + 4,
                                         list_dialog_y2 - Pic_Height(list_dialog_bottom_pic) - 2 - Pic_Height(list_dialog_bottom_arrow_pic),
                                         "", list_dialog_bottom_arrow_pic, "", -1);
        }
        box_ctl = UI_Add_Region(list_dialog_x, list_dialog_y, list_dialog_x2, list_dialog_y2, "", -1);
        cancel_ctl = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        Screen_Present();
    }
    UI_Suspend_Redraw_Fn();
    Font_Restore_Colors();
    Scratch_Restore_From_EMS();
    return picked;
}

/* () the list box: frame pieces, the rows (the one under the mouse highlighted), the scroll column when more = 1,
   the title and the names */
void List_Select_Dialog_Redraw(void)
{
    register int i;
    register int row;

    row = UI_Hit_Test();
    if (row > list_dialog_rows || row > list_dialog_count)
        row = 0;
    if (row > 0)
        list_dialog_highlight_row = row - 1;
    Screen_Restore_A800();
    Set_Clip_Rect(list_dialog_x, list_dialog_y, list_dialog_x2, list_dialog_y2);
    Draw_Pic_Clipped(list_dialog_x, list_dialog_y, list_dialog_left_pic);
    Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic), list_dialog_y, list_dialog_top_pic);
    Set_Clip_Rect(list_dialog_x, list_dialog_y, list_dialog_x2 - 5, list_dialog_y2);
    if (list_dialog_more == 0) {
        for (i = 0; list_dialog_count - 1 > i; i++) {
            if (i == list_dialog_highlight_row) {
                Set_Pic_Frame(list_dialog_row_pics[i], 1);
                Set_Pic_Frame(list_dialog_row_end_pics[i], 1);
            } else {
                Set_Pic_Frame(list_dialog_row_pics[i], 0);
                Set_Pic_Frame(list_dialog_row_end_pics[i], 0);
            }
            Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic),
                             list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i,
                             list_dialog_row_pics[i]);
            Draw_Pic_Clipped(list_dialog_x2 - Pic_Width(list_dialog_right_pic) + 1,
                             list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i,
                             list_dialog_row_end_pics[i]);
        }
        if (list_dialog_count - 1 == list_dialog_highlight_row) {
            Set_Pic_Frame(list_dialog_row_pics[9], 1);
            Set_Pic_Frame(list_dialog_row_end_pics[9], 1);
        } else {
            Set_Pic_Frame(list_dialog_row_pics[9], 0);
            Set_Pic_Frame(list_dialog_row_end_pics[9], 0);
        }
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic),
                         list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * (list_dialog_count - 1),
                         list_dialog_row_pics[9]);
        Draw_Pic_Clipped(list_dialog_x2 - Pic_Width(list_dialog_right_pic) + 1,
                         list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * (list_dialog_count - 1),
                         list_dialog_row_end_pics[9]);
    } else {
        for (i = 0; list_dialog_count - 1 > i; i++) {
            if (i == list_dialog_highlight_row) {
                Set_Pic_Frame(list_dialog_row_pics[i], 1);
                Set_Pic_Frame(list_dialog_row_end_pics[i], 1);
            } else {
                Set_Pic_Frame(list_dialog_row_pics[i], 0);
                Set_Pic_Frame(list_dialog_row_end_pics[i], 0);
            }
            Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic) + Pic_Width(list_dialog_scroll_pic),
                             list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i,
                             list_dialog_row_pics[i]);
            Draw_Pic_Clipped(list_dialog_x2 - Pic_Width(list_dialog_right_pic) + 1,
                             list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i,
                             list_dialog_row_end_pics[i]);
        }
        if (list_dialog_count - 1 == list_dialog_highlight_row) {
            Set_Pic_Frame(list_dialog_row_pics[9], 1);
            Set_Pic_Frame(list_dialog_row_end_pics[9], 1);
        } else {
            Set_Pic_Frame(list_dialog_row_pics[9], 0);
            Set_Pic_Frame(list_dialog_row_end_pics[9], 0);
        }
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic) + Pic_Width(list_dialog_scroll_pic),
                         list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * (list_dialog_count - 1),
                         list_dialog_row_pics[9]);
        Draw_Pic_Clipped(list_dialog_x2 - Pic_Width(list_dialog_right_pic) + 1,
                         list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * (list_dialog_count - 1),
                         list_dialog_row_end_pics[9]);
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic),
                         list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_scroll_top_pic),
                         list_dialog_scroll_pic);
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic) + 4,
                         list_dialog_y + Pic_Height(list_dialog_top_pic) + 3, list_dialog_up_locked_pic);
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic) + 4,
                         list_dialog_y2 - Pic_Height(list_dialog_bottom_pic) - 2 - Pic_Height(list_dialog_down_locked_pic),
                         list_dialog_down_locked_pic);
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic), list_dialog_y + Pic_Height(list_dialog_top_pic),
                         list_dialog_scroll_top_pic);
        Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic), list_dialog_y2 - Pic_Height(list_dialog_bottom_pic),
                         list_dialog_scroll_bottom_pic);
    }
    Set_Clip_Rect(list_dialog_x, list_dialog_y, list_dialog_x2, list_dialog_y2);
    Draw_Pic_Clipped(list_dialog_x + Pic_Width(list_dialog_left_pic), list_dialog_y2 - Pic_Height(list_dialog_bottom_pic) + 1,
                     list_dialog_bottom_pic);
    Draw_Pic_Clipped(list_dialog_x, list_dialog_y2 - Pic_Height(list_dialog_corner_bl_pic) + 1, list_dialog_corner_bl_pic);
    Draw_Pic_Clipped(list_dialog_x2 - Pic_Width(list_dialog_right_pic) + 1, list_dialog_y, list_dialog_right_pic);
    Draw_Pic_Clipped(list_dialog_x2 - Pic_Width(list_dialog_corner_br_pic) + 1,
                     list_dialog_y2 - Pic_Height(list_dialog_corner_br_pic) + 1, list_dialog_corner_br_pic);
    Font_Select_Palette(4, list_title_shadow_colors);
    Set_Font_Color(0xfb);
    Print_Text_Centered((list_dialog_x + list_dialog_x2) / 2 + 1, list_dialog_y + 5, list_dialog_title);
    Font_Select_Palette(4, list_title_colors);
    Set_Font_Color(0x14);
    Print_Text_Centered((list_dialog_x + list_dialog_x2) / 2, list_dialog_y + 4, list_dialog_title);
    Reset_Clip_Rect();
    Font_Select_Shadow(4, 1);
    for (i = 0; i < list_dialog_count && i < list_dialog_rows; i++) {
        if (i == list_dialog_highlight_row) {
            Set_Text_Shadow_Color(0xb2);
            Set_Font_Color(0xb6);
        } else {
            Set_Text_Shadow_Color(0xb6);
            Set_Font_Color(0x35);
        }
        if (list_dialog_more == 1)
            Print_Text(list_dialog_x + Pic_Width(list_dialog_left_pic) + Pic_Width(list_dialog_scroll_pic) + 4,
                       list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i + 2,
                       list_dialog_names[i + list_dialog_first]);
        else
            Print_Text(list_dialog_x + Pic_Width(list_dialog_left_pic) + 4,
                       list_dialog_y + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_row_pics[0]) * i + 2,
                       list_dialog_names[i + list_dialog_first]);
    }
}

/* (count, names, title) counts the names (list_dialog_rows), sizes the box from the widest name / the title and
   count rows, and centres it
   Numbered: BUG #220 (BUGS_AND_QUIRKS.txt) */
void List_Select_Layout(int count, char **names, char *title)
{
    int total_w;
    int total_h;
    int w;
    register int width;
    register int i;

    list_dialog_rows = 0;
    while (*names[list_dialog_rows] != 0)
        list_dialog_rows++;
    Font_Select(4, 0);
    width = 0;
    for (i = 0; i < list_dialog_rows; i++)
        if (Text_Width(names[i]) > width)
            width = Text_Width(names[i]);
    if (list_dialog_more != 0)
        width += Pic_Width(list_dialog_scroll_pic);
    w = Text_Width(title);
    if (w > width)
        width = w;
    width += 8;
    total_w = width + Pic_Width(list_dialog_left_pic) + Pic_Width(list_dialog_right_pic);
    total_h = Pic_Height(list_dialog_row_pics[0]) * count + Pic_Height(list_dialog_top_pic) + Pic_Height(list_dialog_bottom_pic);
    list_dialog_x = (320 - total_w) / 2;
    list_dialog_y = (200 - total_h) / 2;
    list_dialog_x2 = list_dialog_x + total_w - 1;
    list_dialog_y2 = list_dialog_y + total_h - 1;
}

/* (style, pic) saves the scratch block, sets the scratch heap's used mark (the pictures reload from the block's
   start when pic lies beyond its first 0x5dc paragraphs, else after them, so pic survives), loads the frame pictures of the box
   style (0..3) and the five realm gems. RESOURCE.LBX: box / picture panel / gem panel = 46..48 (style 0), 43..45
   (1), 40..42 (2), 56..58 (3); the realm gems 51..55 in GEM_* order */
void Notify_Box_Load(unsigned style, unsigned pic)
{
    int i;

    Scratch_Save_To_EMS();
    Far_Poke_Word(p_scratch, HEAP_HDR_USED, p_scratch + 0x5dc < pic ? 1 : 0x5dc);
    switch (style) {
    case 0:
        notify_box_pics[0] = LBX_Reload_Next("RESOURCE", 46, p_scratch);
        notify_pict_box_pics[0] = LBX_Reload_Next("RESOURCE", 47, p_scratch);
        notify_gem_box_pics[0] = LBX_Reload_Next("RESOURCE", 48, p_scratch);
        break;
    case 1:
        notify_box_pics[1] = LBX_Reload_Next("RESOURCE", 43, p_scratch);
        notify_pict_box_pics[1] = LBX_Reload_Next("RESOURCE", 44, p_scratch);
        notify_gem_box_pics[1] = LBX_Reload_Next("RESOURCE", 45, p_scratch);
        break;
    case 2:
        notify_box_pics[2] = LBX_Reload_Next("RESOURCE", 40, p_scratch);
        notify_pict_box_pics[2] = LBX_Reload_Next("RESOURCE", 41, p_scratch);
        notify_gem_box_pics[2] = LBX_Reload_Next("RESOURCE", 42, p_scratch);
        break;
    case 3:
        notify_box_pics[3] = LBX_Reload_Next("RESOURCE", 56, p_scratch);
        notify_pict_box_pics[3] = LBX_Reload_Next("RESOURCE", 57, p_scratch);
        notify_gem_box_pics[3] = LBX_Reload_Next("RESOURCE", 58, p_scratch);
        break;
    }
    for (i = 0; i < 5; i++)
        notify_realm_pics[i] = LBX_Reload_Next("RESOURCE", i + 51, p_scratch);
}

/* (center_x, y, style, text, use_gem_frame, pic1, pic1_x, pic1_y, pic2, pic2_x, pic2_y, color_cycle, dim) the
   notification box centred on center_x (style 2 with the realm gem of the human's strongest realm: most
   spellbooks, ties to the first of Life, Chaos, Death, Nature, Sorcery); dim = 1 shades the screen first */
void Notify_Picture_Box_Centered(int center_x, int y, register int style, char *text, int use_gem_frame,
                                 register unsigned pic1, int pic1_x, int pic1_y, unsigned pic2, int pic2_x,
                                 int pic2_y, int color_cycle, int dim)
{
    int used;
    int unused;

    Notify_Box_Load(style, pic1);
    notify_realm = REALM_LIFE;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS])
        notify_realm = REALM_CHAOS;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH])
        notify_realm = REALM_DEATH;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE])
        notify_realm = REALM_NATURE;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY])
        notify_realm = REALM_SORCERY;
    switch (notify_realm) {                     /* the realm -> its gem (alphabetical order) */
    case REALM_CHAOS:
        notify_realm = GEM_CHAOS;
        break;
    case REALM_DEATH:
        notify_realm = GEM_DEATH;
        break;
    case REALM_LIFE:
        notify_realm = GEM_LIFE;
        break;
    case REALM_NATURE:
        notify_realm = GEM_NATURE;
        break;
    case REALM_SORCERY:
        notify_realm = GEM_SORCERY;
        break;
    }
    notify_color_cycle = color_cycle;
    notify_color_step = 0;
    Font_Save_Colors();
    if (notify_box_mode == 2)
        Screen_Copy_Front();
    if (dim == 1) {
        Set_Draw_Page();
        Fill_Box_Styled(0, 0, 319, 199, FILL_SHADOW);
    }
    Screen_Save_A800();
    notify_text = text;
    notify_pic1 = pic1;
    notify_pic2 = pic2;
    notify_style = style;
    notify_use_gem_frame = use_gem_frame;
    notify_pic1_dx = pic1_x;
    notify_pic1_dy = pic1_y;
    notify_pic2_dx = pic2_x;
    notify_pic2_dy = pic2_y;
    notify_color_step = 0;
    Font_Select(4, 0);
    used = Paragraph_Used_Width(175, text, PARA_LEFT);
    notify_box_x = center_x - (used + 96) / 2;
    notify_box_y = y;
    UI_Set_Redraw_Fn(Notify_Picture_Box_Center_Redraw, 1);
    Pic_Apply_Palette(notify_box_pics[notify_style]);
    if (pic1 != 0)
        Pic_Apply_Palette(pic1);
    Palette_Mark_Range(0xe0, 0xff);             /* the box's colors E0h .. FFh go up with it; the C6h unmark
                                                   below is a no-op here (C6h is outside that range) */
    Palette_Unmark_Range(PAL_CYCLE_COLOR, PAL_CYCLE_COLOR);
    Remap_Build_Shadow();
    Palette_Upload();
    unused = 0;
    if (notify_box_mode == 1) {
        Set_Draw_Page();
        Notify_Picture_Box_Center_Redraw();
    } else
        UI_Wait_Any_Input();
    Palette_Mark_Range(0, 0xdf);                /* afterwards colors 0 .. DFh again (not C6h) */
    Palette_Unmark_Range(PAL_CYCLE_COLOR, PAL_CYCLE_COLOR);
    Remap_Build_Shadow();
    UI_Suspend_Redraw_Fn();
    Font_Restore_Colors();
    Reset_Clip_Rect();
    Scratch_Restore_From_EMS();
}

/* () redraw of the centred box: color cycling (1: colors F4h..FEh, 2: F7h..FEh), box, gem (style 2), the
   picture panel, the text */
void Notify_Picture_Box_Center_Redraw(void)
{
    register int gem;
    register int used;

    if (notify_style == 2)
        gem = 50;                               /* style 2: the realm gem's 50 pixels */
    else
        gem = 0;
    if (notify_color_cycle == 1) {
        Palette_Rotate_Upload(CYCLE1_FIRST, CYCLE1_COUNT, notify_color_step);
        notify_color_step = (notify_color_step + 1) % CYCLE1_COUNT;
    }
    if (notify_color_cycle == 2) {
        Palette_Rotate_Upload(CYCLE2_FIRST, CYCLE2_COUNT, notify_color_step);
        notify_color_step = (notify_color_step + 1) % CYCLE2_COUNT;
    }
    Screen_Restore_A800();
    Font_Select(4, 0);
    used = Paragraph_Used_Width(175, notify_text, PARA_LEFT);
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, used + notify_box_x + gem + 12, 199);
    Draw_Pic_Clipped(notify_box_x, notify_box_y, notify_box_pics[notify_style]);
    if (notify_style == 2)
        Draw_Pic_Frame(notify_box_x - 23, notify_box_y - 19, notify_realm_pics[notify_realm]);
    Reset_Clip_Rect();
    Fill_Rect(notify_box_x + used + gem + 12, notify_box_y, notify_box_x + used + gem + 62, notify_box_y + 55, 0);
    if (notify_use_gem_frame == 0)
        Draw_Pic_Frame(notify_box_x + used + gem + 12, notify_box_y, notify_pict_box_pics[notify_style]);
    else
        Draw_Pic_Frame(notify_box_x + used + gem + 12, notify_box_y, notify_gem_box_pics[notify_style]);
    if (notify_pic1 != 0) {
        Pic_Decode_Frame_To_Bitmap(notify_pic1, p_pic_work);
        Draw_Bitmap(notify_box_x + used + notify_pic1_dx + gem + 16, notify_box_y + notify_pic1_dy, p_pic_work);
    }
    if (notify_pic2 != 0) {
        Pic_Decode_Frame_To_Bitmap(notify_pic2, p_pic_work);
        Draw_Bitmap(notify_box_x + used + notify_pic2_dx + gem + 16, notify_box_y + notify_pic2_dy, p_pic_work);
    }
    if (notify_style == 3) {
        Font_Select_Palette(4, notify_shadow_colors);
        Font_Select(4, 15, 15);
        Paragraph_Print(notify_box_x + gem + 11, notify_box_y + 9, used + 1, notify_text, PARA_LEFT);
        Paragraph_Print(notify_box_x + gem + 10, notify_box_y + 9, used + 1, notify_text, PARA_LEFT);
        Font_Select(4, 4, 15);
        Set_Font_Color(0x15);
    } else {
        Font_Select_Palette(4, notify_shadow_palettes[notify_style]);
        Font_Select(4, 15, 15);
        Paragraph_Print(notify_box_x + gem + 11, notify_box_y + 9, used + 1, notify_text, PARA_LEFT);
        Paragraph_Print(notify_box_x + gem + 10, notify_box_y + 9, used + 1, notify_text, PARA_LEFT);
        Font_Select_Palette(4, notify_text_palettes[notify_style]);
    }
    Paragraph_Print(notify_box_x + gem + 10, notify_box_y + 8, used + 1, notify_text, PARA_LEFT);
    if (notify_color_cycle == 1) {
        Palette_Rotate_Upload(CYCLE1_FIRST, CYCLE1_COUNT, notify_color_step);
        notify_color_step = (notify_color_step + 1) % CYCLE1_COUNT;
    }
    if (notify_color_cycle == 2) {
        Palette_Rotate_Upload(CYCLE2_FIRST, CYCLE2_COUNT, notify_color_step);
        notify_color_step = (notify_color_step + 1) % CYCLE2_COUNT;
    }
}

/* (x_unused, y, style, text, use_gem_frame, pic1, pic1_x, pic1_y, pic2, pic2_x, pic2_y, color_cycle, dim) the
   notification box at the left edge (x from the realm gem's offset; x is never read). QUIRKS: dim is zeroed on
   entry (the darken branch is dead), and in notify_box_mode 1 it draws with the CENTRED box's redraw
   Numbered: BUG #392 (BUGS_AND_QUIRKS.txt) */
void Notify_Picture_Box(int x_unused, int y, register int style, char *text, int use_gem_frame, register unsigned pic1,
                        int pic1_x, int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim)
{
    int used;
    int unused;

    Notify_Box_Load(style, pic1);
    dim = 0;
    notify_realm = REALM_LIFE;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS])
        notify_realm = REALM_CHAOS;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH])
        notify_realm = REALM_DEATH;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE])
        notify_realm = REALM_NATURE;
    if (wizards[PLAYER_HUMAN].spellbooks[notify_realm] < wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY])
        notify_realm = REALM_SORCERY;
    switch (notify_realm) {                     /* the realm -> its gem (alphabetical order) */
    case REALM_CHAOS:
        notify_realm = GEM_CHAOS;
        break;
    case REALM_DEATH:
        notify_realm = GEM_DEATH;
        break;
    case REALM_LIFE:
        notify_realm = GEM_LIFE;
        break;
    case REALM_NATURE:
        notify_realm = GEM_NATURE;
        break;
    case REALM_SORCERY:
        notify_realm = GEM_SORCERY;
        break;
    }
    notify_color_cycle = color_cycle;
    notify_color_step = 0;
    Font_Save_Colors();
    if (notify_box_mode == 2)
        Screen_Copy_Front();
    if (dim == 1) {
        Set_Draw_Page();
        Fill_Box_Styled(0, 0, 319, 199, FILL_SHADOW);
    }
    Screen_Save_A800();
    notify_text = text;
    notify_pic1 = pic1;
    notify_pic2 = pic2;
    notify_style = style;
    notify_use_gem_frame = use_gem_frame;
    notify_pic1_dx = pic1_x;
    notify_pic1_dy = pic1_y;
    notify_pic2_dx = pic2_x;
    notify_pic2_dy = pic2_y;
    notify_color_step = 0;
    Font_Select(4, 0);
    used = Paragraph_Used_Width(175, text, PARA_LEFT);
    if (notify_gem_dx[notify_realm] < 0)
        notify_box_x = 6 - notify_gem_dx[notify_realm];
    else
        notify_box_x = 6;
    notify_box_y = y;
    UI_Set_Redraw_Fn(Notify_Picture_Box_Redraw, 1);
    Pic_Apply_Palette(notify_box_pics[notify_style]);
    if (pic1 != 0)
        Pic_Apply_Palette(pic1);
    Palette_Upload();
    Palette_Mark_Range(0, 0xf3);                /* all but the cycled colors F4h .. FFh */
    Palette_Unmark_Range(PAL_CYCLE_COLOR, PAL_CYCLE_COLOR);
    Remap_Build_Shadow();
    unused = 0;
    if (notify_box_mode == 1) {
        Set_Draw_Page();
        Notify_Picture_Box_Center_Redraw();
    } else
        UI_Wait_Any_Input();
    Palette_Mark_Range(0, 0xdf);                /* afterwards colors 0 .. DFh again (not C6h) */
    Palette_Unmark_Range(PAL_CYCLE_COLOR, PAL_CYCLE_COLOR);
    Remap_Build_Shadow();
    UI_Suspend_Redraw_Fn();
    Font_Restore_Colors();
    Reset_Clip_Rect();
    Scratch_Restore_From_EMS();
}

/* () redraw of the left-edge box: color cycling (F4h..FEh), box, gem, picture panel, text */
void Notify_Picture_Box_Redraw(void)
{
    register int x2;
    register int used;

    if (notify_color_cycle == 1) {
        Palette_Rotate_Upload(CYCLE1_FIRST, CYCLE1_COUNT, notify_color_step);
        notify_color_step = (notify_color_step + 1) % CYCLE1_COUNT;
    }
    Screen_Restore_A800();
    Font_Select(4, 0);
    used = Paragraph_Used_Width(175, notify_text, PARA_LEFT);
    x2 = used + notify_box_x + notify_text_dx[notify_realm];
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, x2, 199);
    Draw_Pic_Clipped(notify_box_x, notify_box_y, notify_box_pics[notify_style]);
    Draw_Pic_Frame(notify_box_x + notify_gem_dx[notify_realm] - 1, notify_box_y + notify_gem_dy[notify_realm],
                   notify_realm_pics[notify_realm]);
    Reset_Clip_Rect();
    Fill_Rect(x2 + 2, notify_box_y, x2 + 47, notify_box_y + 55, 0);
    if (notify_use_gem_frame == 0)
        Draw_Pic_Frame(x2 + 1, notify_box_y, notify_pict_box_pics[notify_style]);
    else
        Draw_Pic_Frame(x2 + 1, notify_box_y, notify_gem_box_pics[notify_style]);
    if (notify_pic1 != 0) {
        Pic_Decode_Frame_To_Bitmap(notify_pic1, p_pic_work);
        Draw_Bitmap(x2 + notify_pic1_dx + 7, notify_box_y + notify_pic1_dy, p_pic_work);
    }
    if (notify_pic2 != 0) {
        Pic_Decode_Frame_To_Bitmap(notify_pic2, p_pic_work);
        Draw_Bitmap(notify_pic2_dx + x2 + 7, notify_box_y + notify_pic2_dy, p_pic_work);
    }
    Font_Select_Palette(4, notify_shadow_palettes[notify_style]);
    Font_Select(4, 15, 15);
    Paragraph_Print(notify_box_x + notify_text_dx[notify_realm] + 1, notify_box_y + 9, used + 1, notify_text, PARA_LEFT);
    Paragraph_Print(notify_box_x + notify_text_dx[notify_realm], notify_box_y + 9, used + 1, notify_text, PARA_LEFT);
    Font_Select_Palette(4, notify_text_palettes[notify_style]);
    Paragraph_Print(notify_box_x + notify_text_dx[notify_realm], notify_box_y + 8, used + 1, notify_text, PARA_LEFT);
    if (notify_color_cycle == 1) {
        Palette_Rotate_Upload(CYCLE1_FIRST, CYCLE1_COUNT, notify_color_step);
        notify_color_step = (notify_color_step + 1) % CYCLE1_COUNT;
    }
}

/* (x1, y1, x2, y2, src_x1, src_y1, src_x2, src_y2, lines) Window_Zoom_Open with the image where it ends */
void Window_Zoom_Open_In_Place(register int x1, register int y1, int x2, int y2, int src_x1, int src_y1, int src_x2,
                           int src_y2, int lines)
{
    Window_Zoom_Open(x1, y1, x2, y2, x1, y1, x2, y2, src_x1, src_y1, src_x2, src_y2, lines);
}

/* (dst rect, img rect, src rect, lines) grabs the image rectangle and zooms it in 5 steps from the source rectangle
   toward the destination (0 .. 4/5 of the way: the last step is at 80 %, never at the destination) (Pic_Scale_Percent each step), with connector lines when lines != 0; does nothing (returns 0)
   when Expanding Help is off */
int Window_Zoom_Open(int dst_x1, int dst_y1, int dst_x2, int dst_y2, int img_x1, int img_y1, int img_x2, int img_y2,
                     int src_x1, int src_y1, int src_x2, int src_y2, int lines)
{
    int height;
    int width;
    unsigned saved_seg;
    int x_pct;
    int y_pct;
    int x1;
    int y1;
    int x2;
    int y2;
    int bytes;
    unsigned img_w;
    unsigned img_h;
    register int i;
    register unsigned pic;

    zoom_lines = lines;
    zoom_dst_x1 = dst_x1;
    zoom_dst_y1 = dst_y1;
    zoom_dst_x2 = dst_x2;
    zoom_dst_y2 = dst_y2;
    zoom_src_x1 = src_x1;
    zoom_src_y1 = src_y1;
    zoom_src_x2 = src_x2;
    zoom_src_y2 = src_y2;
    if (game_config.expanding_help == 0) {
        Window_Zoom_Skipped_Nop();
        return 0;
    }
    UI_Clear_Controls();
    if (img_x1 > img_x2)
        Swap_Words(&img_x1, &img_x2);
    if (img_y1 > img_y2)
        Swap_Words(&img_y1, &img_y2);
    if (src_x1 > src_x2)
        Swap_Words(&src_x1, &src_x2);
    if (src_y1 > src_y2)
        Swap_Words(&src_y1, &src_y2);
    EMS_Map_Handle_Pages(ems_vgafile_handle, 0);
    saved_seg = ems_frame_seg;
    Heap_Mark(p_scratch);
    pic = Heap_Alloc_Back(p_scratch, 0xc35);    /* 0xc35 paragraphs: the bitmap header + the grabbed pixels
                                                   from pic + 1 */
    bytes = (img_y2 - img_y1) * (img_x2 - img_x1) + 32;   /* BUG #470: (w - 1)(h - 1) + 32, short of the w x h + 16
                                                   bitmap once w + h > 17: each step's restore misses its tail */
    width = img_x2 - img_x1 + 1;
    height = img_y2 - img_y1 + 1;
    Pic_Create_Blank(width, height, pic);
    img_w = img_x2 - img_x1 + 1;
    img_h = img_y2 - img_y1 + 1;
    Set_Draw_Page();
    Screen_Grab_Rect_Core(pic + 1, img_x1, img_y1, img_x2, img_y2);
    Mem_Copy_Far(0, saved_seg, 0, pic, bytes);
    Screen_Copy_Front();
    Screen_Save_A800();
    Window_Zoom_Start_Nop();
    for (i = 0; i < 5; i++) {
        Timer_Mark();
        Screen_Restore_A800();
        x1 = (img_x1 - src_x1) * i / 5 + src_x1;
        y1 = (img_y1 - src_y1) * i / 5 + src_y1;
        x2 = (img_x2 - src_x2) * i / 5 + src_x2;
        y2 = (img_y2 - src_y2) * i / 5 + src_y2;
        if (x1 > x2)
            Swap_Words(&x1, &x2);
        if (y1 > y2)
            Swap_Words(&y1, &y2);
        width = x2 - x1 + 1;
        height = y2 - y1 + 1;
        x_pct = width * 100 / img_w;
        y_pct = height * 100 / img_h;
        Pic_Scale_Percent(pic, x_pct, y_pct);
        Set_Draw_Page();
        x1 = (dst_x1 - src_x1) * i / 5 + src_x1;
        y1 = (dst_y1 - src_y1) * i / 5 + src_y1;
        x2 = (dst_x2 - src_x2) * i / 5 + src_x2;
        y2 = (dst_y2 - src_y2) * i / 5 + src_y2;
        if (x1 > x2)
            Swap_Words(&x1, &x2);
        if (y1 > y2)
            Swap_Words(&y1, &y2);
        width = x2 - x1 + 1;
        height = y2 - y1 + 1;
        if (i != 0 && zoom_lines != 0)
            Window_Zoom_Draw(src_x1, src_y1, src_x2, src_y2, x1, y1, x2, y2, ZOOM_LINE_COLORS);
        Draw_Bitmap(x1, y1, pic);
        UI_Present_Frame();
        if (i != 4)
            Mem_Copy_Far(0, pic, 0, saved_seg, bytes);
        Wait_Ticks(1);
    }
    Heap_Release(p_scratch);
}

/* () the connector lines of the open zoom window again (redraw callbacks), when it has them */
int Window_Zoom_Lines_Redraw(void)
{
    if (zoom_lines == 0)
        return 0;
    Set_Draw_Page();
    Window_Zoom_Draw(zoom_src_x1, zoom_src_y1, zoom_src_x2, zoom_src_y2, zoom_dst_x1, zoom_dst_y1, zoom_dst_x2,
                     zoom_dst_y2, ZOOM_LINE_COLORS);
}

/* (from rect, to rect, color pairs a, b, c) the dotted connector lines between two rectangles, every second pixel
   row / column on the sides that face each other; dash patterns from a fixed random seed (the game's seed is
   saved and restored). QUIRKS: two branches mix the left and right edges (copy-paste) and draw a zero-length line
   or the same line twice; color pair b is built but never drawn
   Numbered: BUG #377, #391 (BUGS_AND_QUIRKS.txt) */
void Window_Zoom_Draw(register int from_x1, int from_y1, int from_x2, int from_y2, int to_x1, int to_y1, int to_x2,
                      int to_y2, int col_a1, int col_a2, int col_b1, int col_b2, int col_c1, int col_c2)
{
    int x_a;
    int y_a;
    int x_b;
    int y_b;
    int row;
    int k;
    int y;
    long seed;
    char pat_c[4][12];
    char pat_b[4][12];
    char pat_a[4][12];
    register int x;

    Set_Draw_Page();
    seed = Random_Get_Seed();
    Random_Seed(0x188edL);
    for (row = 0; row < 4; row++)
        for (k = 0; k < 12; k++) {
            if (Random(2) == 1) {
                Swap_Words(&col_a2, &col_a1);
                Swap_Words(&col_b2, &col_b1);
                Swap_Words(&col_c2, &col_c1);
            }
            if (Random(4) == 1)
                pat_a[row][k] = col_a2;
            else
                pat_a[row][k] = col_a1;
            if (Random(4) == 1)
                pat_b[row][k] = col_b2;
            else
                pat_b[row][k] = col_b1;
            if (Random(4) == 1)
                pat_c[row][k] = col_c2;
            else
                pat_c[row][k] = col_c1;
        }
    if (from_y1 < to_y1) {
        if (from_x1 < to_x1) {
            for (x = from_x1; x < to_x1; x += 2) {
                y_a = (x - from_x1) * (to_y1 - from_y1) / (to_x1 - from_x1) + from_y1;
                y_b = (x - from_x1) * (to_y2 - from_y2) / (to_x1 - from_x1) + from_y2;
                x_b = (to_x2 - from_x2) * (x - from_x1) / (to_x1 - from_x1) + from_x2;
                Draw_Line_Pattern(x_b, y_a, x_b, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_b, x_b, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x, y_b, pat_a[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x_b, y_a, pat_a[Random(4) - 1], 12, 0);
            }
        } else if (from_x2 > to_x2) {
            for (x = from_x2 - 2; x > to_x2; x -= 2) {
                y_a = (x - from_x2) * (to_y1 - from_y1) / (to_x2 - from_x2) + from_y1;
                y_b = (x - from_x2) * (to_y2 - from_y2) / (to_x2 - from_x2) + from_y2;
                x_a = (to_x1 - from_x1) * (x - from_x2) / (to_x2 - from_x2) + from_x1;
                Draw_Line_Pattern(x_a, y_a, x_a, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_a, y_b, x, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_a, y_a, x_a, y_b, pat_a[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x, y_b, pat_a[Random(4) - 1], 12, 0);
            }
        } else {
            for (y = from_y1; y < to_y1; y += 2) {
                x_a = (y - from_y1) * (to_x1 - from_x1) / (to_y1 - from_y1) + from_x1;
                x_b = (y - from_y1) * (to_x2 - from_x2) / (to_y1 - from_y1) + from_x2;
                y_b = (to_y2 - from_y2) * (y - from_y1) / (to_y1 - from_y1) + from_y2;
                Draw_Line_Pattern(x_a, y, x_a, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_b, y, x_b, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_a, y, x_b, y, pat_a[Random(4) - 1], 12, 0);
            }
        }
    } else if (from_y2 > to_y2) {
        if (from_x1 < to_x1) {
            for (x = from_x1; x < to_x1; x += 2) {
                y_a = (x - from_x1) * (to_y1 - from_y1) / (to_x1 - from_x1) + from_y1;
                y_b = (x - from_x1) * (to_y2 - from_y2) / (to_x1 - from_x1) + from_y2;
                x_b = (to_x2 - from_x2) * (x - from_x1) / (to_x1 - from_x1) + from_x2;
                Draw_Line_Pattern(x_b, y_a, x_b, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x_b, y_a, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x, y_b, pat_a[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_b, x_b, y_b, pat_a[Random(4) - 1], 12, 0);
            }
        } else if (from_x2 > to_x2) {
            for (x = from_x2 - 2; x > to_x2; x -= 2) {
                y_a = (x - from_x1) * (to_y1 - from_y1) / (to_x1 - from_x1) + from_y1;
                y_b = (x - from_x1) * (to_y2 - from_y2) / (to_x1 - from_x1) + from_y2;
                x_a = (to_x1 - from_x1) * (x - from_x2) / (to_x2 - from_x2) + from_x1;
                Draw_Line_Pattern(x_a, y_a, x_a, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_a, y_a, x, y_a, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x, y_a, pat_a[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x, y_b, pat_a[Random(4) - 1], 12, 0);
            }
        } else {
            for (y = from_y2 - 2; y > to_y2; y -= 2) {
                x_a = (y - from_y2) * (to_x1 - from_x1) / (to_y2 - from_y2) + from_x1;
                x_b = (y - from_y2) * (to_x2 - from_x2) / (to_y2 - from_y2) + from_x2;
                y_a = (to_y1 - from_y1) * (y - from_y2) / (to_y2 - from_y2) + from_y1;
                Draw_Line_Pattern(x_a, y_a, x_a, y, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_b, y_a, x_b, y, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_a, y, x_b, y, pat_a[Random(4) - 1], 12, 0);
            }
        }
    } else {
        if (from_x1 < to_x1) {
            for (x = from_x1; x < to_x1; x += 2) {
                y_a = (x - from_x1) * (to_y1 - from_y1) / (to_x1 - from_x1) + from_y1;
                y_b = (x - from_x1) * (to_y2 - from_y2) / (to_x1 - from_x1) + from_y2;
                x_b = (to_x2 - from_x2) * (x - from_x1) / (to_x1 - from_x1) + from_x2;
                Draw_Line_Pattern(x, y_a, x_b, y_a, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_b, x_b, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_a, x, y_b, pat_a[Random(4) - 1], 12, 0);
            }
        } else {
            for (x = from_x2 - 2; x > to_x2; x -= 2) {
                y_a = (x - from_x2) * (to_y1 - from_y1) / (to_x2 - from_x2) + from_y1;
                y_b = (x - from_x2) * (to_y2 - from_y2) / (to_x2 - from_x2) + from_y2;
                x_b = (to_x2 - from_x2) * (x - from_x2) / (to_x2 - from_x2) + from_x2;
                Draw_Line_Pattern(x, y_a, x_b, y_a, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x, y_b, x_b, y_b, pat_c[Random(4) - 1], 12, 0);
                Draw_Line_Pattern(x_b, y_a, x_b, y_b, pat_a[Random(4) - 1], 12, 0);
            }
        }
    }
    Random_Seed(seed);
}

/* (cx, cy, pic, steps, &x, &y) the next step of a picture zooming out of (cx, cy): its final place (kept 5 pixels
   from the edges; QUIRK #390: the bottom limit is 315 like the right one, and the left / top clamps set x2 / y2
   to w / 2 + 4 / h / 2 + 4, not x1 + w - 1) and, before the last step, a position and
   scale between; zoom_step counts up
   Numbered: BUG #390 (BUGS_AND_QUIRKS.txt) */
void Zoom_Step_Rect(int cx, int cy, unsigned pic, register int steps, int *x, int *y)
{
    int x1;
    int h;
    int w;
    int y1;
    int x2;
    int y2;
    int pct;

    w = Far_Peek_Word(pic, PIC_WIDTH);
    h = Far_Peek_Word(pic, PIC_HEIGHT);
    x1 = cx - w / 2;
    y1 = cy - h;
    x2 = x1 + w - 1;
    y2 = y1 + h - 1;
    if (x1 < 5) {
        x1 = 5;
        x2 = w / 2 + 4;
    }
    if (y1 < 5) {
        y1 = 5;
        y2 = h / 2 + 4;
    }
    if (x2 > 315) {
        x2 = 315;
        x1 = 315 - w + 1;
    }
    if (y2 > 315) {
        y2 = 315;
        y1 = 315 - h + 1;
    }
    if (zoom_step > steps)
        zoom_step = steps;
    if (zoom_step == steps) {
        *x = x1;
        *y = y1;
    } else {
        *x = (long)zoom_step * (x1 - cx) / steps + cx;
        *y = (long)zoom_step * (y1 - cy) / steps + cy;
        pct = zoom_step * 100 / steps;
        Pic_Scale_Percent(pic, pct, pct);
    }
    zoom_step++;
}

/* () zoom_step back to 1: the next Zoom_Step_Rect starts a new zoom */
void Zoom_Reset(void)
{
    zoom_step = 1;
}

/* () the 64 KB scratch block to the VGAFILEH handle's pages 0-3 (pop-ups reuse it) */
void Scratch_Save_To_EMS(void)
{
    register unsigned seg;

    seg = EMS_Map_Backup_Frame();
    Mem_Copy_Far(0, seg, 0, p_scratch, 32768);
    Mem_Copy_Far(0, seg + 0x800, 0, p_scratch + 0x800, 32768);
}

/* () and back */
void Scratch_Restore_From_EMS(void)
{
    register unsigned seg;

    seg = EMS_Map_Backup_Frame();
    Mem_Copy_Far(0, p_scratch, 0, seg, 32768);
    Mem_Copy_Far(0, p_scratch + 0x800, 0, seg + 0x800, 32768);
}
