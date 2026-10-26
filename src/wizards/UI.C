/* UI.C - WIZARDS.EXE code segment 2666 (MAGIC.EXE 24da, the same file): the control / input library every screen
   uses: a table of controls (p_ui_controls, 26h-byte UIControl records) built by the UI_Add_* functions, the input
   loops (mouse: UI_Poll_Input; keyboard cursor: UI_Poll_Input_Keyboard), hotkeys and arrow-key navigation, text
   fields, sliders, list dialogs, control drawing, right-click help, the redraw callback and the developer input
   recorder's file functions.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 2666 src/wizards/UI.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     DOS_File_Close (1; DOSFILE.C takes none);
     DOS_File_Read_Far (5; DOSFILE.C takes 4);
     DOS_File_Write_Far (5; DOSFILE.C takes 4);
     Draw_Pic_Frame (4; PIC.C takes 3);
     Font_Select_Shadow (3; TEXT.C takes 4);
     Font_Select_Shadow_UL (3; TEXT.C takes 4);
     UI_Add_Slider (12; its definition below takes 11). */
#include <string.h>
#include <dos.h>
#include "STRUCTS.H"
#include "ENUMS.H"

#define UI_ALT_KEY_OFFSET 0x5f     /* an Alt + letter key code = the upper-case letter + 0x5f (KEY_ALT_A 0xa0) */
#define UI_OFFSCREEN 500          /* the box of a control with no screen area (hotkeys) */
#define UI_CLIPPED_AWAY (-10000)  /* the _Clipped adders' result for a control wholly outside the clip */
#define UI_TEXT_LINE_INDENT 10    /* text lines print this far right of their box */
#define UI_MAX_CONTROLS 150       /* p_ui_controls: 0x165 paragraphs = 150 UIControl records */
#define UI_CURSOR_HOME_X 158      /* the keyboard cursor's start, near the screen centre */
#define UI_CURSOR_HOME_Y 100
#define UI_DEBUG_BOX_COLOR 14     /* the developer outlines: controls */
#define UI_DEBUG_HELP_COLOR 180   /* help areas */
#pragma warn -pro

extern int clip_x1;                             /* DS:76da */
extern int clip_y1;                             /* DS:76dc */
extern int clip_x2;                             /* DS:76de */
extern int clip_y2;                             /* DS:76e0 */
extern int input_record_seg;                    /* DS:8246 */
extern int input_record_base;                   /* DS:8248 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
struct UIControl far *p_ui_controls;            /* DS:e8ac  MAGIC DS:a8a2   */
RedrawFn redraw_fn;                             /* DS:e8a8  MAGIC DS:a89e   */
RedrawFn saved_redraw_fn;                       /* DS:e8a4  MAGIC DS:a89a   */
int ui_control_count;                           /* DS:e8a2  MAGIC DS:a898   */
int ui_saved_control_count;                     /* DS:e8a0  MAGIC DS:a896   */
int mouse_pointer_offset;                       /* DS:e89e  MAGIC DS:a894   */
int ui_cursor_x;                                /* DS:e89c  MAGIC DS:a892   */
int ui_cursor_y;                                /* DS:e89a  MAGIC DS:a890   */
int ui_input_delay;                             /* DS:e898  MAGIC DS:a88e   */
char ui_edit_buf[30];                           /* DS:e87a  MAGIC DS:a870   */
int ui_last_click_x;                            /* DS:e878  MAGIC DS:a86e   */
int ui_last_click_y;                            /* DS:e876  MAGIC DS:a86c   */
int ui_last_click_buttons;                      /* DS:e874  MAGIC DS:a86a   */
int *p_active_help;                             /* DS:e872  MAGIC DS:a868   - the HelpEntry list read as 5 ints
                                                   per entry: [5 i] id, [5 i + 1 .. 4] x1, y1, x2, y2 */

void far *Seg_To_Far_Ptr(unsigned seg);
void UI_Draw_Pressed_Control(int control, int x, int y);
void UI_Draw_Control(int control, int pressed);
void UI_Draw_Controls(void);
void UI_Redraw_Frame_1_Tick(void);
void UI_Redraw_Frame_Wait(void);
void UI_Call_Redraw_Fn(void);
void UI_Text_Edit_Draw(int control, char *str);
void UI_Clear_Controls(void);
void UI_Input_Delay(int delay);
void UI_Present_Frame(void);
void Slider_Step_Up(int control_index);
void Slider_Step_Down(int control_index);
int DOS_File_Read_Far();                       /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
int DOS_File_Write_Far();                      /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
void DOS_File_Close();                         /* no prototype here: called with the handle; DOSFILE.C takes none (it closes the stored one) */
void Draw_Pic_Frame();                         /* no prototype here: called with a 4th word (0) the routine never reads */
void Font_Select_Shadow();                     /* no prototype here: some calls pass 2 or 3 of the 4 words (QUIRK #371) */
void Font_Select_Shadow_UL();                  /* no prototype here: some calls pass 2 or 3 of the 4 words (QUIRK #371) */
int UI_Add_Slider();                           /* no prototype here: UI_Add_Slider_Near passes its near value pointer + segment 0 (= DS) */

int redraw_fn_active = 0;                       /* DS:824a  the redraw callback runs while the input loops wait */
int ui_hotkey_pos = 0;                          /* DS:824c  control of the last hotkey match (UI_Handle_Nav_Key resumes after it) */
int ui_mouse_present = 0;                       /* DS:824e  0: keyboard-cursor mode (UI_Poll_Input_Keyboard) */
int ui_highlight_control = -1;                  /* DS:8250  control drawn pressed / highlighted, -1 none */
int ui_press_dx = 1;                            /* DS:8252  x offset of a pressed button's label (1: one px right) */
int ui_press_dy = -1;                           /* DS:8254  y offset of a pressed button's label (-1: one px up) */
int ui_list_title_help = -1;                   /* DS:8256  the title-button help of both list dialogs */
int ui_list_dialog_active = 0;                         /* DS:8258  1 while a list dialog runs */
int ui_frame_ticks = 2;                         /* DS:825a  ticks per frame of the waiting loops */
struct MouseRegion ui_init_mouse_region = { 0, 0, 0, 319, 199, 0 };    /* DS:825c  UI_Init(1)'s pointer region */
/* ui_init_mouse_region by the field order: x1 0, y1 319, x2 199, y2 0 (MAGIC.C's regions read 0, 0, 319, 199) */
int ui_pressed_control = 0;                     /* DS:8268  control held down right now */
int ui_list_hit_enabled = 1;                    /* DS:826a  0: the list dialogs skip UI_Hit_Test (only the DEAD
                                                   UI_List_Hit_Disable sets 0) */
int ui_list_dialog_scrolling = 0;                           /* DS:826c */
int ui_button_color2 = 0;                               /* DS:826e  the text buttons' (list lines') second Font_Select
                                                   color set (c2), 1..14; c3 is not passed */
int ui_list_scroll_req = 0;                     /* DS:8270  +1 / -1: the keyboard cursor ran off a list end */
int ui_right_click_ids = 0;                     /* DS:8272  1: a right-click returns -(control) (in game) */
int ui_instant_click = 0;                       /* DS:8274  1: controls fire on the press */
char ui_not_in_use[] = "NOT IN USE";            /* DS:8276  no reader in either program (a placeholder name) */
int ui_edit_caret = 0;                          /* DS:8282 */
int ui_edit_blink = 0;                          /* DS:8284 */
int ui_edit_caret_visible = 0;                   /* DS:8286 */
int ui_edit_active = 0;                         /* DS:8288  1 while a type-11 text line is edited */
int ui_show_control_boxes = 0;                  /* DS:828a  developer key `: outline every control box */
int ui_show_help_boxes = 0;                     /* DS:828c  developer key ~: outline every help area */
int ui_edit_control = -1;                       /* DS:828e  the type-11 line being edited, -1 none */
int input_record_mode = 0;                      /* DS:8290 */
int input_record_flag = 0;                      /* DS:8292  Input_Record_Init's 2nd argument, no reader */
int input_record_exists = 0;                    /* DS:8294 */
int ui_help_enabled = 0;                        /* DS:8296  UI_Set_Help called (right-click help on) */
int ui_help_count = 0;                          /* DS:8298 */
int ui_alt_seq_control = 0;                     /* DS:829a  type-8 control whose multi-letter code is half typed */

/* () -> the activated control (0 none, -1 right-click, -(control) a right-click in the in-game mode): the mouse-driver
   input loop under UI_Get_Input - keys (hotkeys, Enter on the control under the pointer, +/- on sliders, typing into
   the edited text line), a click already taken, or a button held down (the control under the pointer is shown
   pressed until the release); the right-click help; the developer keys ` and ~ outline the controls / help areas.
   Simtex name Interpret_Mouse_Input (MoO2 symbols, module fields); clarified using ReMoM repo
   Numbered: BUG #402 (BUGS_AND_QUIRKS.txt) */
int UI_Poll_Input(void)
{
    int mx;
    int my;
    int buttons;
    int printable;
    int x1;
    int y1;
    int x2;
    int y2;
    int unused;
    int control;
    char key;
    int hit;
    int j;

    control = 0;
    ui_highlight_control = -1;
    buttons = 0;
    ui_pressed_control = 0;
    mx = Mouse_X();
    my = Mouse_Y();
    mouse_pointer_offset = Mouse_Get_Pointer_Offset();
    if (Key_Pressed()) {
        key = UI_Handle_Nav_Key(&control);
        if (key == 0) {
            Mouse_Click_Handler_On();
            return 0;
        }
        if (key == '\x91')                /* KEY_F11 (char: compared as -111) */
            Debug_Memory_Report_Key();
        if (key == '\x92') {              /* KEY_F12 */
            Mouse_Hide();
            Mouse_Restore_Under();
            Debug_Screenshot_FLI();
            Mouse_Save_Under(Mouse_X(), Mouse_Y());
            Mouse_Draw_Pointer(Mouse_X(), Mouse_Y());
            Mouse_Set_Position(Mouse_X(), Mouse_Y());
            Mouse_Show();
        }
        if (key == KEY_ESC && ui_right_click_ids == 0)
            return -1;
        if (key == '`' && Check_Release_Version() == 0) {
            if (ui_show_control_boxes == 0)
                ui_show_control_boxes = 1;
            else
                ui_show_control_boxes = 0;
        }
        if (key == '~' && Check_Release_Version() == 0) {
            if (ui_show_help_boxes == 0)
                ui_show_help_boxes = 1;
            else
                ui_show_help_boxes = 0;
        }
        if (p_ui_controls[control].type == UI_ALT_HOTKEY) {
            Mouse_Click_Handler_On();
            return control;
        }
        if (control != 0 && p_ui_controls[control].hotkey == key) {
            if (p_ui_controls[control].type == UI_SLIDER) {
                Mouse_Click_Handler_On();
                return 0;
            }
            if (control != 0) {
                mx = p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2;
                my = p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2;
                UI_Draw_Pressed_Control(control, mx, my);
                Mouse_Click_Handler_On();
                switch (p_ui_controls[control].type) {
                case UI_LATCH:
                    if (*(int *)p_ui_controls[control].param2 == 0) {
                        *(int *)p_ui_controls[control].param2 = 1;
                    } else {
                        UI_Redraw_Frame_1_Tick();
                        ui_highlight_control = -1;
                        return 0;
                    }
                    break;
                case UI_VALUE_REGION:
                    ui_highlight_control = -1;
                    return p_ui_controls[control].param0;
                case UI_TOGGLE:
                    if (*(int *)p_ui_controls[control].param2 == 0)
                        *(int *)p_ui_controls[control].param2 = 1;
                    else
                        *(int *)p_ui_controls[control].param2 = 0;
                    break;
                default:
                    break;
                }
            }
            UI_Redraw_Frame_1_Tick();
            ui_highlight_control = -1;
            return control;
        } else {
            if (key == KEY_ENTER) {
                if (ui_edit_active != 0) {
                    hit = ui_edit_control;
                    strcpy((char *)p_ui_controls[hit].param0, ui_edit_buf);
                    if (ui_instant_click == 0)
                        UI_Redraw_Frame_1_Tick();
                    ui_highlight_control = -1;
                    return hit;
                }
                hit = UI_Control_At_Mouse();
                if (hit == 0)
                    return 0;
                if (hit > 0) {
                    if (p_ui_controls[hit].type == UI_TEXT_LINE) {
                        if (ui_edit_active == 0) {
                            strcpy(ui_edit_buf, (char *)p_ui_controls[hit].param0);
                            ui_edit_blink = 0;
                            ui_edit_caret_visible = 0;
                            ui_edit_active = 1;
                            ui_edit_control = hit;
                        } else if (ui_edit_control != hit) {
                            strcpy(ui_edit_buf, (char *)p_ui_controls[hit].param0);
                            ui_edit_blink = 0;
                            ui_edit_caret_visible = 0;
                            ui_edit_control = hit;
                        } else {
                            for (j = 0; ui_edit_buf[j] != 0 && p_ui_controls[hit].param5 > j; j++)
                                ;
                            if (ui_edit_buf[j - 1] == '_')
                                j--;
                            ui_edit_buf[j] = 0;
                            ui_edit_blink = 0;
                            ui_edit_control = -1;
                            strcpy((char *)p_ui_controls[hit].param0, ui_edit_buf);
                        }
                    } else if (ui_edit_active != 0 && p_ui_controls[hit].type != UI_TEXT_LINE) {
                        for (j = 0; ui_edit_buf[j] != 0 && p_ui_controls[ui_edit_control].param5 > j; j++)
                            ;
                        if (ui_edit_buf[j - 1] == '_')
                            j--;
                        ui_edit_buf[j] = 0;
                        strcpy((char *)p_ui_controls[ui_edit_control].param0, ui_edit_buf);
                        ui_edit_blink = 0;
                        ui_edit_active = 0;
                        ui_edit_control = -1;
                    }
                }
                if (hit != 0) {
                    if (p_ui_controls[hit].type != UI_SLIDER && p_ui_controls[hit].type != UI_TEXT_LINE)
                        UI_Draw_Pressed_Control(hit, mx, my);
                    Mouse_Click_Handler_On();
                    switch (p_ui_controls[hit].type) {
                    case UI_LATCH:
                        if (*(int *)p_ui_controls[hit].param2 == 0) {
                            *(int *)p_ui_controls[hit].param2 = 1;
                        } else {
                            ui_highlight_control = -1;
                            return 0;
                        }
                        break;
                    case UI_VALUE_REGION:
                        ui_highlight_control = -1;
                        return p_ui_controls[hit].param0;
                    case UI_TOGGLE:
                        if (*(int *)p_ui_controls[hit].param2 == 0)
                            *(int *)p_ui_controls[hit].param2 = 1;
                        else
                            *(int *)p_ui_controls[hit].param2 = 0;
                        break;
                    default:
                        break;
                    }
                    if (ui_instant_click == 0)
                        UI_Redraw_Frame_1_Tick();
                    ui_highlight_control = -1;
                    return hit;
                }
                if (ui_list_dialog_active != 0) {
                    for (hit = 1; hit < ui_control_count; hit++) {
                        if (p_ui_controls[hit].type == UI_TEXT_BUTTON &&
                                *(int *)p_ui_controls[hit].param2 == p_ui_controls[hit].param1 &&
                                p_ui_controls[hit].enabled != 0) {
                            ui_highlight_control = -1;
                            return hit;
                        }
                    }
                }
            }
            if (key == '+' || key == '-') {
                hit = UI_Control_At_Mouse();
                if (hit != 0 && p_ui_controls[hit].type == UI_SLIDER) {
                    if (key == '+')
                        Slider_Step_Up(hit);
                    if (key == '-')
                        Slider_Step_Down(hit);
                    Mouse_Click_Handler_On();
                    ui_highlight_control = -1;
                    return hit;
                }
            }
            if (ui_edit_active > 0) {
                if (key != KEY_BACKSPACE) {
                    printable = 0;
                    if ((key > '@' && key < ']') || (key > '-' && key < ';') || key == ' ' || key == '-')   /* A..Z [ \,
                                                   . / 0..9 :, space, -; a..z below */
                        printable = 1;
                    else if (key > 'a' - 1 && key < 'z' + 1)
                        printable = 1;
                    if (printable == 1) {
                        for (hit = 0; ui_edit_buf[hit] != '_' && ui_edit_buf[hit] != 0 &&
                                p_ui_controls[ui_edit_control].param5 - 1 > hit; hit++)
                            ;
                        if (p_ui_controls[ui_edit_control].param5 - 1 > hit) {
                            ui_edit_buf[hit] = key;
                            ui_edit_buf[hit + 1] = 0;
                        } else {
                            ui_edit_buf[hit] = 0;
                        }
                        ui_edit_caret_visible = 0;
                        ui_edit_blink = 0;
                    }
                } else {
                    for (hit = 0; ui_edit_buf[hit] != 0 && ui_edit_buf[hit] != '_' &&
                            p_ui_controls[ui_edit_control].param5 - 1 > hit; hit++)
                        ;
                    if (hit > 0) {
                        ui_edit_buf[hit - 1] = 0;
                        ui_edit_blink = 0;
                    }
                }
            }
            ui_highlight_control = -1;
            Mouse_Click_Handler_On();
            return 0;
        }
    }
    Mouse_Click_Handler_Off();
    if (Mouse_Get_Buttons() == 0) {
        if (Mouse_Take_Click() == 0) {
            Mouse_Click_Handler_On();
            return 0;
        } else {
            buttons = Mouse_Click_Buttons();
            if (buttons == 2) {
                if (ui_help_enabled != 0 && UI_Show_Help_At_Mouse() == 0) {
                    Mouse_Take_Click();
                    Mouse_Take_Event();
                    return 0;
                }
                if (ui_right_click_ids == 0) {
                    Mouse_Take_Click();
                    Mouse_Take_Event();
                    return -1;
                }
            }
            mx = Mouse_Click_X();
            my = Mouse_Click_Y();
            control = 0;
            unused = -1;
            key = 0;
            Mouse_Find_Region(mx, my);
            mouse_pointer_offset = Mouse_Get_Pointer_Offset();
            for (hit = 1; hit < ui_control_count; hit = hit + 1) {
                x1 = p_ui_controls[hit].x1;
                y1 = p_ui_controls[hit].y1;
                x2 = p_ui_controls[hit].x2;
                y2 = p_ui_controls[hit].y2;
                if (mx + mouse_pointer_offset >= x1 && mx + mouse_pointer_offset <= x2 &&
                        my + mouse_pointer_offset >= y1 && my + mouse_pointer_offset <= y2) {
                    control = hit;
                    break;
                }
            }
            if (control != 0 && p_ui_controls[control].type != UI_TEXT_LINE) {
                ui_pressed_control = control;
                UI_Draw_Pressed_Control(control, mx, my);
                UI_Redraw_Frame_1_Tick();
            }
            if (control != 0) {
                if (p_ui_controls[control].type == UI_TEXT_LINE) {
                    if (ui_edit_active == 0) {
                        strcpy(ui_edit_buf, (char *)p_ui_controls[control].param0);
                        ui_edit_blink = 0;
                        ui_edit_caret_visible = 0;
                        ui_edit_active = 1;
                        ui_edit_control = control;
                    } else if (ui_edit_control != control) {
                        strcpy(ui_edit_buf, (char *)p_ui_controls[control].param0);
                        ui_edit_blink = 0;
                        ui_edit_caret_visible = 0;
                        ui_edit_control = control;
                    } else {
                        for (j = 0; ui_edit_buf[j] != 0 && p_ui_controls[control].param5 > j; j++)
                            ;
                        if (ui_edit_buf[j - 1] == '_')
                            j--;
                        ui_edit_buf[j] = 0;
                        ui_edit_blink = 0;
                        ui_edit_control = -1;
                        strcpy((char *)p_ui_controls[control].param0, ui_edit_buf);
                    }
                } else if (ui_edit_active != 0) {
                    for (j = 0; ui_edit_buf[j] != 0 && p_ui_controls[ui_edit_control].param5 > j; j++)
                        ;
                    if (ui_edit_buf[j - 1] == '_')
                        j--;
                    ui_edit_buf[j] = 0;
                    strcpy((char *)p_ui_controls[ui_edit_control].param0, ui_edit_buf);
                    ui_edit_active = 0;
                    ui_edit_control = -1;
                }
            }
            ui_highlight_control = -1;
            if (control != 0) {
                Mouse_Take_Event();
                ui_last_click_x = mx;
                ui_last_click_y = my;
                ui_last_click_buttons = buttons;
            }
            if (control != 0) {
                switch (p_ui_controls[control].type) {
                case UI_LATCH:
                    if (*(int *)p_ui_controls[control].param2 == 0) {
                        *(int *)p_ui_controls[control].param2 = 1;
                    } else {
                        Mouse_Click_Handler_On();
                        ui_highlight_control = -1;
                        return 0;
                    }
                    break;
                case UI_VALUE_REGION:
                    if (buttons == 2) {
                        Mouse_Click_Handler_On();
                        ui_highlight_control = -1;
                        if (ui_right_click_ids != 0)
                            return -p_ui_controls[control].param0;
                        else
                            return -1;
                    } else {
                        Mouse_Click_Handler_On();
                        ui_highlight_control = -1;
                        return p_ui_controls[control].param0;
                    }
                case UI_TOGGLE:
                    if (*(int *)p_ui_controls[control].param2 == 0)
                        *(int *)p_ui_controls[control].param2 = 1;
                    else
                        *(int *)p_ui_controls[control].param2 = 0;
                    break;
                case UI_TEXT_INPUT:
                    UI_Draw_Pressed_Control(control, mx, my);
                    break;
                default:
                    break;
                }
            }
            Mouse_Click_Handler_On();
            ui_pressed_control = 0;
            if (buttons == 2)
                return -control;
            else
                return control;
        }
    } else {
        buttons = Mouse_Get_Buttons();
        if (buttons == 2) {
            if (ui_help_enabled != 0 && UI_Show_Help_At_Mouse() == 0) {
                Mouse_Take_Click();
                Mouse_Take_Event();
                return 0;
            }
            if (ui_right_click_ids == 0) {
                while (Mouse_Get_Buttons() == 2)
                    UI_Redraw_Frame_1_Tick();
                Mouse_Take_Click();
                Mouse_Take_Event();
                return -1;
            }
        }
        while (Mouse_Get_Buttons()) {
            mx = Mouse_X();
            my = Mouse_Y();
            mouse_pointer_offset = Mouse_Get_Pointer_Offset();
            control = 0;
            unused = -1;
            key = 0;
            control = UI_Control_At_Mouse();
            if (control == 0) {
                if (ui_highlight_control != -1) {
                    if (p_ui_controls[ui_highlight_control].type == UI_SLIDER)
                        UI_Call_Redraw_Fn();
                    if (p_ui_controls[ui_highlight_control].type != UI_PIC_VALUE && p_ui_controls[ui_highlight_control].type != UI_TEXT_BUTTON &&
                            p_ui_controls[ui_highlight_control].type != UI_TEXT_LINE) {
                        Mouse_Hide();
                        Mouse_Restore_Under();
                        UI_Draw_Control(ui_highlight_control, 0);
                        Mouse_Save_Under(mx, my);
                        Mouse_Draw_Pointer(mx, my);
                        Mouse_Set_Position(mx, my);
                        Mouse_Show();
                    }
                    ui_highlight_control = -1;
                }
                Mouse_Post_Move(mx, my);
                break;
            }
            if (control != ui_highlight_control && p_ui_controls[control].type != UI_TEXT_INPUT && p_ui_controls[control].type != UI_TEXT_LINE &&
                    (ui_highlight_control == -1 || p_ui_controls[control].type != UI_GRID ||
                     p_ui_controls[ui_highlight_control].type != UI_GRID)) {
                if (p_ui_controls[ui_highlight_control].type == UI_SLIDER)
                    UI_Call_Redraw_Fn();
                UI_Draw_Pressed_Control(control, mx, my);
            }
            if (p_ui_controls[control].type == UI_TEXT_LINE) {
                if (ui_edit_active == 0) {
                    strcpy(ui_edit_buf, (char *)p_ui_controls[control].param0);
                    ui_edit_blink = 0;
                    ui_edit_caret_visible = 0;
                    ui_edit_active = 1;
                    ui_edit_control = control;
                } else if (ui_edit_control != control) {
                    strcpy(ui_edit_buf, (char *)p_ui_controls[control].param0);
                    ui_edit_blink = 0;
                    ui_edit_caret_visible = 0;
                    ui_edit_active = 1;
                    ui_edit_control = control;
                }
            } else if (ui_edit_active != 0) {
                for (j = 0; ui_edit_buf[j] != 0 && p_ui_controls[ui_edit_control].param5 > j; j++)
                    ;
                if (ui_edit_buf[j - 1] == '_')
                    j--;
                ui_edit_buf[j] = 0;
                strcpy((char *)p_ui_controls[ui_edit_control].param0, ui_edit_buf);
                ui_edit_active = 0;
                ui_edit_control = -1;
            }
            ui_pressed_control = control;
            if (ui_instant_click != 0)
                break;
            if (Mouse_Get_Buttons())
                UI_Redraw_Frame_Wait();
        }
        if (p_ui_controls[ui_pressed_control].type == UI_SLIDER)
            UI_Call_Redraw_Fn();
    }
    ui_pressed_control = 0;
    if (control != 0) {
        Mouse_Take_Click();
        Mouse_Take_Event();
        ui_last_click_x = mx;
        ui_last_click_y = my;
        ui_last_click_buttons = buttons;
        switch (p_ui_controls[control].type) {
        case UI_LATCH:
            if (*(int *)p_ui_controls[control].param2 == 0) {
                *(int *)p_ui_controls[control].param2 = 1;
            } else {
                Mouse_Click_Handler_On();
                ui_highlight_control = -1;
                return 0;
            }
            break;
        case UI_VALUE_REGION:
            if (buttons == 2) {
                Mouse_Click_Handler_On();
                ui_highlight_control = -1;
                if (ui_right_click_ids != 0)
                    return -p_ui_controls[control].param0;
                else
                    return -1;
            } else {
                Mouse_Click_Handler_On();
                ui_highlight_control = -1;
                return p_ui_controls[control].param0;
            }
        case UI_TOGGLE:
            if (*(int *)p_ui_controls[control].param2 == 0)
                *(int *)p_ui_controls[control].param2 = 1;
            else
                *(int *)p_ui_controls[control].param2 = 0;
            break;
        case UI_TEXT_INPUT:
            UI_Draw_Pressed_Control(control, mx, my);
            break;
        default:
            break;
        }
    }
    Mouse_Click_Handler_On();
    ui_highlight_control = -1;
    switch (buttons) {
    case 0:
        return 0;
    case 1:
        return control;
    case 2:
        return -control;
    }
}

/* DEAD (no callers; the developer input recorder): stop the playback
   found because ReMoM's numbering ran ahead of ours (10/17 audit); clarified using ReMoM repo */
void Input_Record_Stop_Playback(void)
{
    if (input_record_exists != 0) {
        Input_Record_Clear();
        Input_Record_Pause();
        input_record_mode = 1;
    }
}

/* DEAD: stop recording */
void Input_Record_Stop(void)
{
    if (input_record_exists != 0) {
        Input_Record_Off();
        Input_Record_Pause();
        input_record_mode = -1;
    }
}

/* DEAD: start recording */
void Input_Record_Start(void)
{
    input_record_mode = 0;
    Input_Record_Close();
}

/* DEAD: (events, flag) allocates the event buffer (4 bytes per event) */
void Input_Record_Init(int events, int flag)
{
    input_record_seg = Heap_Alloc_Block((events << 2) / 16 + 1);
    input_record_exists = 1;
    input_record_flag = flag;
    input_record_base = events;
}

/* DEAD: (file) loads a recording */
void Input_Record_Load(char *name)
{
    int unused;
    int file;

    if (input_record_exists != 0) {
        file = DOS_File_Open(name, "rb");
        DOS_File_Read_Far(0, input_record_seg, input_record_base << 2, 1, file);
        DOS_File_Close(file);
        unused = Input_Record_Seek_End();
        input_record_mode = 0;
    }
}

/* DEAD: (file) saves the recording */
void Input_Record_Save(char *name)
{
    int file;

    if (input_record_exists != 0) {
        file = DOS_File_Open(name, "wb");
        DOS_File_Write_Far(0, input_record_seg, input_record_base << 2, 1, file);
        DOS_File_Close(file);
        input_record_mode = 0;
    }
}

/* a right-click returns -(control id) (the in-game mode).
   Simtex name Disable_Cancel (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Right_Click_Ids_On(void)
{
    ui_right_click_ids = 1;
}

/* a right-click returns -1. Simtex name Enable_Cancel (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Right_Click_Ids_Off(void)
{
    ui_right_click_ids = 0;
}

/* a control fires on the press, not on the release.
   Simtex name Disable_Mouse_Wait (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Instant_Click_On(void)
{
    ui_instant_click = 1;
}

/* () controls fire on the release again, the default (ui_instant_click = 0).
   Simtex name Enable_Mouse_Wait (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Instant_Click_Off(void)
{
    ui_instant_click = 0;
}

/* (help entries, count) right-click help for the screen's controls (HelpEntry records: id, x1, y1, x2, y2).
   Simtex name Set_Help_List (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Set_Help(struct HelpEntry *help, int count)
{
    p_active_help = (int *)help;   /* read as 5 ints per entry */
    ui_help_enabled = 1;
    ui_help_count = count;
}

/* () the screen has no right-click help.
   Simtex name Deactivate_Help_List (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Clear_Help(void)
{
    ui_help_enabled = 0;
    ui_help_count = 0;
    p_active_help = 0;
}

/* () -> 0 when the mouse is over a help rectangle and its help was shown, else 1.
   Simtex name Check_Help_List (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Show_Help_At_Mouse(void)
{
    int shown;
    int mx;
    int my;
    int i;
    int entry;

    shown = 0;
    mx = Mouse_X();
    my = Mouse_Y();
    if (ui_help_count == 0)
        return 1;
    for (i = 0; i < ui_help_count; i++) {
        entry = i * 5;
        if (p_active_help[entry] != -1 && p_active_help[entry + 1] <= mx && p_active_help[entry + 3] >= mx &&
                p_active_help[entry + 2] <= my && p_active_help[entry + 4] >= my) {
            Screen_Help_Popup(p_active_help[entry]);
            i = ui_help_count;                  /* a dead store: the return follows; clarified using ReMoM repo */
            return 0;
        }
    }
    return 1;
}

/* (control) -> width; a negative index counts as its absolute value */
int UI_Control_Width(int control)
{
    int width;

    if (control < 0)
        control = -control;
    if (control >= ui_control_count)
        return 0;
    width = p_ui_controls[control].x2 - p_ui_controls[control].x1 + 1;
    return width;
}

/* (control, either sign) -> its height in pixels, 0 for a control outside the table */
int UI_Control_Height(int control)
{
    int height;

    if (control < 0)
        control = -control;
    if (control >= ui_control_count)
        return 0;
    height = p_ui_controls[control].y2 - p_ui_controls[control].y1 + 1;
    return height;
}

/* () -> the x of the last click */
int UI_Last_Click_X(void)
{
    return ui_last_click_x;
}

/* () -> the y of the last click */
int UI_Last_Click_Y(void)
{
    return ui_last_click_y;
}

/* () -> the mouse buttons of the last click */
int UI_Last_Click_Buttons(void)
{
    return ui_last_click_buttons;
}

/* () -> the control pressed last */
int UI_Get_Pressed_Control(void)
{
    return ui_pressed_control;
}

/* () -> control: the no-mouse input loop - the keys move a software cursor between controls.
   Simtex name Auto_Input (MoO2 symbols, module fields); clarified using ReMoM repo
   Numbered: BUG #338 (BUGS_AND_QUIRKS.txt) */
int UI_Poll_Input_Keyboard(void)
{
    int hover;
    int old_x;
    int old_y;
    int control;
    int printable;
    char key;
    int hit;
    int i;

    old_x = ui_cursor_x;
    old_y = ui_cursor_y;
    Mouse_Restore_Under();
    Mouse_Save_Under(ui_cursor_x, ui_cursor_y);
    Mouse_Draw_Pointer(ui_cursor_x, ui_cursor_y);
    Mouse_Get_Pointer_Image(old_x, old_y);
    mouse_pointer_offset = Mouse_Get_Pointer_Offset();
    hover = 0;
    hover = UI_Control_At_Mouse();   /* never read */
    control = 0;
    key = 0;
    if (Key_Pressed()) {
        key = UI_Handle_Nav_Key(&control);
        if (key == '\x91')                /* KEY_F11 (char: compared as -111) */
            Debug_Memory_Report_Key();
        if (key == '\x92') {              /* KEY_F12 */
            Mouse_Restore_Under();
            Debug_Screenshot_FLI();
            Mouse_Save_Under(Mouse_X(), Mouse_Y());
            Mouse_Draw_Pointer(Mouse_X(), Mouse_Y());
            Mouse_Set_Position(Mouse_X(), Mouse_Y());
        }
        if (key == KEY_ESC)
            return -1;
        if (p_ui_controls[control].hotkey == key) {
            if (key == 0)
                return 0;
            if (control != 0) {
                old_x = ui_cursor_x;
                old_y = ui_cursor_y;
                ui_cursor_x = p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2;
                ui_cursor_y = p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2;
                if (ui_cursor_x < 0 || ui_cursor_x > SCREEN_MAX_X)
                    ui_cursor_x = old_x;
                if (ui_cursor_y < 0 || ui_cursor_y > SCREEN_MAX_Y)
                    ui_cursor_y = old_y;
                UI_Draw_Pressed_Control(control, ui_cursor_x, ui_cursor_y);
                switch (p_ui_controls[control].type) {
                case UI_LATCH:
                    if (*(int *)p_ui_controls[control].param2 == 0) {
                        *(int *)p_ui_controls[control].param2 = 1;
                    } else {
                        ui_highlight_control = -1;
                        return 0;
                    }
                    break;
                case UI_VALUE_REGION:
                    return p_ui_controls[control].param0;
                case UI_TOGGLE:
                    if (*(int *)p_ui_controls[control].param2 == 0)
                        *(int *)p_ui_controls[control].param2 = 1;
                    else
                        *(int *)p_ui_controls[control].param2 = 0;
                    break;
                default:
                    break;
                }
            }
            ui_highlight_control = -1;
            return control;
        } else {
            if (key == KEY_ENTER) {
                old_x = ui_cursor_x;
                old_y = ui_cursor_y;
                hit = UI_Control_At_Mouse();
                if (hit > 0) {
                    if (p_ui_controls[hit].type == UI_TEXT_LINE) {
                        if (ui_edit_active == 0) {
                            strcpy(ui_edit_buf, (char *)p_ui_controls[hit].param0);
                            ui_edit_blink = 0;
                            ui_edit_caret_visible = 0;
                            ui_edit_active = 1;
                            ui_edit_control = hit;
                        } else if (ui_edit_control != hit) {
                            strcpy(ui_edit_buf, (char *)p_ui_controls[hit].param0);
                            ui_edit_blink = 0;
                            ui_edit_caret_visible = 0;
                            ui_edit_control = hit;
                        }
                    } else if (ui_edit_active != 0 && p_ui_controls[hit].type != UI_TEXT_LINE) {
                        /* BUG #338 (original): bounds by control i (the loop counter), not ui_edit_control */
                        for (i = 0; ui_edit_buf[i] != 0 && p_ui_controls[i].param5 > i; i++)
                            ;
                        if (ui_edit_buf[i - 1] == '_')
                            i--;
                        ui_edit_buf[i] = 0;
                        strcpy((char *)p_ui_controls[ui_edit_control].param0, ui_edit_buf);
                        ui_edit_blink = 0;
                        ui_edit_active = 0;
                        ui_edit_control = -1;
                    }
                }
                if (hit != 0) {
                    if (p_ui_controls[hit].type != UI_SLIDER && p_ui_controls[hit].type != UI_TEXT_LINE)
                        UI_Draw_Pressed_Control(hit, ui_cursor_x, ui_cursor_y);
                    switch (p_ui_controls[hit].type) {
                    case UI_LATCH:
                        if (*(int *)p_ui_controls[hit].param2 == 0) {
                            *(int *)p_ui_controls[hit].param2 = 1;
                        } else {
                            ui_highlight_control = -1;
                            return 0;
                        }
                        break;
                    case UI_VALUE_REGION:
                        return p_ui_controls[hit].param0;
                    case UI_TOGGLE:
                        /* BUG #338 (original): tests control's flag but writes hit's */
                        if (*(int *)p_ui_controls[control].param2 == 0)
                            *(int *)p_ui_controls[hit].param2 = 1;
                        else
                            *(int *)p_ui_controls[hit].param2 = 0;
                        break;
                    default:
                        break;
                    }
                    if (ui_instant_click == 0)
                        UI_Redraw_Frame_1_Tick();
                    return hit;
                }
                if (ui_list_dialog_active != 0) {
                    for (hit = 1; hit < ui_control_count; hit++) {
                        if (p_ui_controls[hit].type == UI_TEXT_BUTTON &&
                                *(int *)p_ui_controls[hit].param2 == p_ui_controls[hit].param1 &&
                                p_ui_controls[hit].enabled != 0) {
                            ui_highlight_control = -1;
                            return hit;
                        }
                    }
                }
            }
            if (key == '+' || key == '-') {
                hit = UI_Control_At_Mouse();
                if (hit != 0) {
                    if (p_ui_controls[hit].type == UI_SLIDER) {
                        if (key == '+')
                            Slider_Step_Up(hit);
                        if (key == '-')
                            Slider_Step_Down(hit);
                        return hit;
                    }
                    return 0;
                }
            }
            if (ui_edit_active > 0) {
                if (key != KEY_BACKSPACE) {
                    printable = 0;
                    if ((key > '@' && key < ']') || (key > '-' && key < ';') || key == ' ' || key == '-')   /* A..Z [ \,
                                                   . / 0..9 :, space, -; a..z below */
                        printable = 1;
                    else if (key > 'a' - 1 && key < 'z' + 1)
                        printable = 1;
                    if (printable == 1) {
                        for (hit = 0; ui_edit_buf[hit] != '_' && ui_edit_buf[hit] != 0 &&
                                p_ui_controls[ui_edit_control].param5 - 1 > hit; hit++)
                            ;
                        if (p_ui_controls[ui_edit_control].param5 - 1 > hit) {
                            ui_edit_buf[hit] = key;
                            ui_edit_buf[hit + 1] = 0;
                        } else {
                            ui_edit_buf[hit] = 0;
                        }
                        ui_edit_caret_visible = 0;
                        ui_edit_blink = 0;
                    }
                } else {
                    for (hit = 0; ui_edit_buf[hit] != 0 && ui_edit_buf[hit] != '_' &&
                            p_ui_controls[ui_edit_control].param5 - 1 > hit; hit++)
                        ;
                    if (hit > 0) {
                        ui_edit_buf[hit - 1] = 0;
                        ui_edit_blink = 0;
                    }
                }
            }
        }
    }
    ui_cursor_x = Mouse_X();
    ui_cursor_y = Mouse_Y();
    return 0;
}

/* (&control) -> key: reads a key; a control whose hotkey matches (type-8 controls cycle through a multi-letter code)
   -> *control and the pointer moves to its centre; the 8 arrow / diagonal keys pick the neighbour */
int UI_Handle_Nav_Key(int *control)
{
    int result;
    int j;
    int reset_alt_seqs;
    char key;
    char orig_key;
    char text[30];
    int i;

    key = Key_Read_Translated();
    if (ui_hotkey_pos >= ui_control_count)
        ui_hotkey_pos = 0;
    i = ui_hotkey_pos + 1;
    orig_key = key;
    if (key > 'a' - 1 && key < 'z' + 1)
        key = key - ('a' - 'A');
    if (ui_alt_seq_control != 0) {
        i = ui_alt_seq_control;
        key = key - UI_ALT_KEY_OFFSET;
        if (p_ui_controls[i].hotkey == key) {
            key = key + UI_ALT_KEY_OFFSET;
        } else {
            key = key + UI_ALT_KEY_OFFSET;
            i = ui_control_count;
        }
    } else {
        i = ui_control_count;
    }
    if (i == ui_control_count) {
        ui_alt_seq_control = 0;
        i = ui_hotkey_pos + 1;
        while ((p_ui_controls[i].hotkey != key || p_ui_controls[i].type == UI_ALT_HOTKEY) && i != ui_control_count) {
            if (p_ui_controls[i].type == UI_ALT_HOTKEY) {
                key = key - UI_ALT_KEY_OFFSET;
                if (p_ui_controls[i].hotkey == key) {
                    key = key + UI_ALT_KEY_OFFSET;
                    break;
                } else {
                    key = key + UI_ALT_KEY_OFFSET;
                    i++;
                }
            } else {
                i++;
            }
        }
        if (i == ui_control_count) {
            i = 1;
            while ((p_ui_controls[i].hotkey != key || p_ui_controls[i].type == UI_ALT_HOTKEY) && i != ui_control_count) {
                if (p_ui_controls[i].type == UI_ALT_HOTKEY) {
                    key = key - UI_ALT_KEY_OFFSET;
                    if (p_ui_controls[i].hotkey == key) {
                        key = key + UI_ALT_KEY_OFFSET;
                        break;
                    } else {
                        key = key + UI_ALT_KEY_OFFSET;
                        i++;
                    }
                } else {
                    i++;
                }
            }
        }
    }
    ui_hotkey_pos = i;
    reset_alt_seqs = 1;
    if (i < ui_control_count) {
        *control = i;
        if (p_ui_controls[i].x1 < SCREEN_W && p_ui_controls[i].y1 < SCREEN_H) {
            Mouse_Hide();
            ui_cursor_x = p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2;
            ui_cursor_y = p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2;
            if (ui_cursor_x < SCREEN_W && ui_cursor_y < SCREEN_H) {
                Mouse_Find_Region(ui_cursor_x, ui_cursor_y);
                mouse_pointer_offset = Mouse_Get_Pointer_Offset();
                ui_cursor_x = ui_cursor_x - mouse_pointer_offset;
                ui_cursor_y = ui_cursor_y - mouse_pointer_offset;
                Mouse_Set_Position(ui_cursor_x, ui_cursor_y);
                Mouse_Restore_Under();
                Mouse_Save_Under(ui_cursor_x, ui_cursor_y);
                Mouse_Draw_Pointer(ui_cursor_x, ui_cursor_y);
            }
            Mouse_Show();
        }
        if (p_ui_controls[i].type == UI_ALT_HOTKEY) {
            strcpy(text, (char *)p_ui_controls[i].param0);
            p_ui_controls[i].param1++;
            if (p_ui_controls[i].param1 >= p_ui_controls[i].param2) {
                p_ui_controls[i].param1 = 0;
                p_ui_controls[i].hotkey = text[0];
                ui_alt_seq_control = 0;
            } else {
                p_ui_controls[i].hotkey = text[p_ui_controls[i].param1];
                ui_alt_seq_control = i;
                *control = 0;
                key = 0;
            }
            reset_alt_seqs = 0;
        }
    } else {
        result = *control;
        switch (key) {
        case KEY_LEFT:
            result = UI_Nav_Move('L');
            break;
        case KEY_RIGHT:
            result = UI_Nav_Move('R');
            break;
        case KEY_UP:
            result = UI_Nav_Move('U');
            break;
        case KEY_DOWN:
            result = UI_Nav_Move('D');
            break;
        case KEY_UP_LEFT:
            result = UI_Nav_Move('W');
            break;
        case KEY_UP_RIGHT:
            result = UI_Nav_Move('X');
            break;
        case KEY_DOWN_LEFT:
            result = UI_Nav_Move('Y');
            break;
        case KEY_DOWN_RIGHT:
            result = UI_Nav_Move('Z');
            break;
        default:
            break;
        }
        *control = result;
    }
    if (reset_alt_seqs != 0) {
        for (j = 0; j < ui_control_count; j++) {
            if (p_ui_controls[j].type == UI_ALT_HOTKEY) {
                strcpy(text, (char *)p_ui_controls[j].param0);
                p_ui_controls[j].param1 = 0;
                p_ui_controls[j].hotkey = text[0];
            }
        }
    }
    if (p_ui_controls[*control].hotkey != key)
        key = orig_key;
    return key;
}

/* (dx, dy, current, x, y) -> the control an arrow key moves to: candidates are controls 1.. of type < UI_GRID without a
   hotkey letter (type 10 only when enabled); a straight move first takes the nearest control whose centre is within
   5 px (-6 < d < 6) of the cursor's column / row, else the nearest (squared distance) inside a cone (100 * side / ahead < 44); a
   diagonal takes the nearest with 34 <= 100 * short / long <= 105; keeps 'current' when nothing qualifies, 0 when there are no
   candidates at all */
int UI_Find_Control_In_Direction(int dx, int dy, int current, int x, int y)
{
    int dist_y;
    int best;
    int candidates;
    int valid;
    long ratio;
    long best_dist;
    long dist;
    int i;
    int dist_x;

    dist_x = 500;
    dist_y = 500;
    ratio = 500;
    best_dist = 200000L;
    best = current;
    i = 1;
    candidates = 0;
    while (i < ui_control_count) {
        if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
            if (p_ui_controls[i].type != UI_TEXT_BUTTON)
                candidates++;
            else if (p_ui_controls[i].enabled != 0)
                candidates++;
        }
        i++;
    }
    if (candidates == 0)
        return 0;
    if (dy != 0 && dx == 0) {
        for (i = 1; i < ui_control_count; i++) {
            if (i == current) {
                if (ui_control_count - 1 == i)
                    break;
                else
                    i = i + 1;
            }
            valid = 0;
            if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
                if (p_ui_controls[i].type == UI_TEXT_BUTTON) {
                    if (p_ui_controls[i].enabled != 0)
                        valid = 1;
                } else {
                    valid = 1;
                }
            }
            if (valid != 0) {
                if (dy < 0)
                    dist_y = y - p_ui_controls[i].y1 - (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2;
                else
                    dist_y = (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + p_ui_controls[i].y1 - y;
                dist_x = (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 + p_ui_controls[i].x1 - x;
                if (p_ui_controls[i].x1 <= x && p_ui_controls[i].x2 >= x && (dist_x < -6 || dist_x > 6))
                    dist_x = 6;
                if (dist_x > -6 && dist_x < 6 && dist_y > 0 && dist_y < best_dist) {
                    best_dist = dist_y;
                    best = i;
                }
            }
        }
        if (best == current) {
            for (i = 1; i < ui_control_count; i++) {
                if (i == current) {
                    if (ui_control_count - 1 == i)
                        break;
                    else
                        i = i + 1;
                }
                valid = 0;
                if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
                    if (p_ui_controls[i].type == UI_TEXT_BUTTON) {
                        if (p_ui_controls[i].enabled != 0)
                            valid = 1;
                    } else {
                        valid = 1;
                    }
                }
                if (valid != 0) {
                    if (dy < 0)
                        dist_y = y - p_ui_controls[i].y1 - (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2;
                    else
                        dist_y = (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + p_ui_controls[i].y1 - y;
                    dist_x = (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 + p_ui_controls[i].x1 - x;
                    if (p_ui_controls[i].x1 <= x && p_ui_controls[i].x2 >= x && (dist_x < -6 || dist_x > 6))
                        dist_x = 6;
                    if (dist_x < 0)
                        dist_x = -dist_x;
                    if (dist_y < 0)
                        dist_x = 500;
                    if (dist_y == 0)
                        dist_y = 1;
                    ratio = 100L * dist_x / dist_y;
                    if (ratio >= 0 && ratio < 44) {
                        dist = (long)dist_x * dist_x + (long)dist_y * dist_y;
                        if (dist < best_dist) {
                            best_dist = dist;
                            best = i;
                        }
                    }
                }
            }
        }
    }
    if (dx != 0 && dy == 0) {
        for (i = 1; i < ui_control_count; i++) {
            if (i == current) {
                if (ui_control_count - 1 == i)
                    break;
                else
                    i = i + 1;
            }
            valid = 0;
            if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
                if (p_ui_controls[i].type == UI_TEXT_BUTTON) {
                    if (p_ui_controls[i].enabled != 0)
                        valid = 1;
                } else {
                    valid = 1;
                }
            }
            if (valid != 0) {
                if (dx < 0)
                    dist_x = x - p_ui_controls[i].x1 - (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2;
                else
                    dist_x = p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 - x;
                dist_y = (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + p_ui_controls[i].y1 - y;
                if (p_ui_controls[i].y1 <= y && p_ui_controls[i].y2 >= y && (dist_y < -6 || dist_y > 6))
                    dist_y = 6;
                if (dist_y > -6 && dist_y < 6 && dist_x > 0 && dist_x < best_dist) {
                    best_dist = dist_x;
                    best = i;
                }
            }
        }
        if (best == current) {
            for (i = 1; i < ui_control_count; i++) {
                if (i == current) {
                    if (ui_control_count - 1 == i)
                        break;
                    else
                        i = i + 1;
                }
                valid = 0;
                if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
                    if (p_ui_controls[i].type == UI_TEXT_BUTTON) {
                        if (p_ui_controls[i].enabled != 0)
                            valid = 1;
                    } else {
                        valid = 1;
                    }
                }
                if (valid != 0) {
                    if (dx < 0)
                        dist_x = x - p_ui_controls[i].x1 - (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2;
                    else
                        dist_x = p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 - x;
                    dist_y = (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + p_ui_controls[i].y1 - y;
                    if (p_ui_controls[i].y1 <= y && p_ui_controls[i].y2 >= y && (dist_y < -6 || dist_y > 6))
                        dist_y = 6;
                    if (dist_y < 0)
                        dist_y = -dist_y;
                    if (dist_x < 0)
                        dist_y = 500;
                    if (dist_x == 0)
                        dist_x = 1;
                    ratio = 100L * dist_y / dist_x;
                    if (ratio >= 0 && ratio < 44) {
                        dist = (long)dist_x * dist_x + (long)dist_y * dist_y;
                        if (dist < best_dist) {
                            best_dist = dist;
                            best = i;
                        }
                    }
                }
            }
        }
    }
    if (dx != 0 && dy < 0) {
        for (i = 1; i < ui_control_count; i++) {
            if (i == current) {
                if (ui_control_count - 1 == i)
                    break;
                else
                    i = i + 1;
            }
            valid = 0;
            if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
                if (p_ui_controls[i].type == UI_TEXT_BUTTON) {
                    if (p_ui_controls[i].enabled != 0)
                        valid = 1;
                } else {
                    valid = 1;
                }
            }
            if (valid != 0) {
                if (dx > 0)
                    dist_x = p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 - x;
                else
                    dist_x = x - p_ui_controls[i].x1 - (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2;
                dist_y = y - (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 - p_ui_controls[i].y1;
                if (dist_x < 0 || dist_y < 0) {
                    ratio = 500;
                } else {
                    if (dist_x >= dist_y && dist_y != 0)
                        ratio = 100L * dist_y / dist_x;
                    if (dist_y > dist_x && dist_x != 0)
                        ratio = 100L * dist_x / dist_y;
                    if (dist_x == 0 || dist_y == 0)
                        ratio = 500;
                    if (ratio >= 34 && ratio <= 105) {
                        dist = (long)dist_x * dist_x + (long)dist_y * dist_y;
                        if (dist < best_dist) {
                            best_dist = dist;
                            best = i;
                        }
                    }
                }
            }
        }
    }
    if (dx != 0 && dy > 0) {
        for (i = 1; i < ui_control_count; i++) {
            if (i == current) {
                if (ui_control_count - 1 == i)
                    break;
                else
                    i = i + 1;
            }
            valid = 0;
            if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].hotkey == 0) {
                if (p_ui_controls[i].type == UI_TEXT_BUTTON) {
                    if (p_ui_controls[i].enabled != 0)
                        valid = 1;
                } else {
                    valid = 1;
                }
            }
            if (valid != 0) {
                if (dx > 0)
                    dist_x = p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 - x;
                else
                    dist_x = x - p_ui_controls[i].x1 - (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2;
                dist_y = (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + p_ui_controls[i].y1 - y;
                if (dist_x < 0 || dist_y < 0) {
                    ratio = 500;
                } else {
                    if (dist_x >= dist_y && dist_y != 0)
                        ratio = 100L * dist_y / dist_x;
                    if (dist_y > dist_x && dist_x != 0)
                        ratio = 100L * dist_x / dist_y;
                    if (dist_x == 0 || dist_y == 0)
                        ratio = 500;
                    if (ratio >= 34 && ratio <= 105) {
                        dist = (long)dist_x * dist_x + (long)dist_y * dist_y;
                        if (dist < best_dist) {
                            best_dist = dist;
                            best = i;
                        }
                    }
                }
            }
        }
    }
    return best;
}

/* (control) moves the keyboard cursor (no-mouse mode) to the control's centre */
void UI_Warp_Mouse_To_Control(int control)
{
    int x;
    int y;

    if (ui_mouse_present == 0) {
        x = p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2;
        y = p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2;
        if (y >= 0 && y < SCREEN_H && x >= 0 && x < SCREEN_W) {
            ui_cursor_x = x;
            ui_cursor_y = y;
            Mouse_Find_Region(ui_cursor_x, ui_cursor_y);
            mouse_pointer_offset = Mouse_Get_Pointer_Offset();
            ui_cursor_x = ui_cursor_x - mouse_pointer_offset;
            ui_cursor_y = ui_cursor_y - mouse_pointer_offset;
            Mouse_Set_Position(ui_cursor_x, ui_cursor_y);
            Mouse_Restore_Under();
            Mouse_Save_Under(ui_cursor_x, ui_cursor_y);
            Mouse_Draw_Pointer(ui_cursor_x, ui_cursor_y);
        }
    }
}

/* () -> the control under the mouse (1..), 0 none.
   Simtex name Scan_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Control_At_Mouse(void)
{
    int mx;
    int my;
    int found;
    int x1;
    int y1;
    int x2;
    int y2;
    int i;

    mx = Mouse_X();
    my = Mouse_Y();
    found = 0;
    Mouse_Find_Region(mx, my);
    mouse_pointer_offset = Mouse_Get_Pointer_Offset();
    for (i = 1; i < ui_control_count; i = i + 1) {
        x1 = p_ui_controls[i].x1;
        y1 = p_ui_controls[i].y1;
        x2 = p_ui_controls[i].x2;
        y2 = p_ui_controls[i].y2;
        if (mx + mouse_pointer_offset >= x1 && mx + mouse_pointer_offset <= x2 &&
                my + mouse_pointer_offset >= y1 && my + mouse_pointer_offset <= y2) {
            found = i;
            break;
        }
    }
    return found;
}

/* () -> the control under the mouse: a UI_VALUE_REGION stores its value; a disabled UI_TEXT_BUTTON -> 0; a UI_GRID
   stores the column / row under the mouse (unless the highlighted control is a grid).
   Simtex name Scan_Input (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Hit_Test(void)
{
    int mx;
    int my;
    int control;

    mx = Mouse_X();
    my = Mouse_Y();
    control = 0;
    Mouse_Find_Region(mx, my);
    mouse_pointer_offset = Mouse_Get_Pointer_Offset();
    control = UI_Control_At_Mouse();
    if (p_ui_controls[control].type == UI_VALUE_REGION) {
        *(int *)p_ui_controls[control].param2 = p_ui_controls[control].param1;
        return p_ui_controls[control].param0;
    } else {
        if (p_ui_controls[control].type == UI_TEXT_BUTTON && p_ui_controls[control].enabled == 0)
            return 0;
        if (p_ui_controls[control].type == UI_GRID) {
            if (ui_highlight_control == -1 || p_ui_controls[ui_highlight_control].type != UI_GRID) {
                *(int *)p_ui_controls[control].param3 = (Mouse_X() - p_ui_controls[control].x1) / p_ui_controls[control].param1;
                *(int *)p_ui_controls[control].param4 = (Mouse_Y() - p_ui_controls[control].y1) / p_ui_controls[control].param2;
            }
        }
        return control;
    }
}

/* () waits for a key and returns it.
   Simtex name Read_Character (MoO2 symbols, module fields); clarified using ReMoM repo */
int Wait_For_Key(void)
{
    while (Key_Pressed() == 0)
        ;
    return Key_Read_Translated();
}

/* () empties the key buffer, waits for the mouse buttons to be released and drops the click.
   Simtex name Reset_Wait_For_Input (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Flush_Input(void)
{
    ui_pressed_control = 0;
    while (Key_Pressed())
        Key_Read_Translated();
    if (ui_mouse_present != 0) {
        while (Mouse_Get_Buttons())
            UI_Redraw_Frame_Wait();
        Mouse_Take_Click();
        Mouse_Take_Event();
    }
}

/* () -> -1 after a key or a click (no controls active meanwhile; the redraw callback keeps running).
   Simtex name Wait_For_Input (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Wait_Any_Input(void)
{
    char key;
    int done;
    int clicked;

    UI_Flush_Input();
    done = 0;
    clicked = 0;
    ui_saved_control_count = ui_control_count;
    ui_control_count = 1;
    while (done == 0) {
        if (ui_mouse_present != 0) {
            Mouse_Click_Handler_Off();
            if (Mouse_Get_Buttons() != 0 || Mouse_Take_Click() != 0) {
                clicked = 1;
                done = 1;
            }
            if (Key_Pressed()) {
                key = Key_Read_Translated();
                switch (key) {
                case '\x91':                         /* F11 / F12: the developer keys */
                    Debug_Memory_Report_Key();
                    break;
                case '\x92':
                    Mouse_Hide();
                    Mouse_Restore_Under();
                    Debug_Screenshot_FLI();
                    Mouse_Save_Under(Mouse_X(), Mouse_Y());
                    Mouse_Draw_Pointer(Mouse_X(), Mouse_Y());
                    Mouse_Set_Position(Mouse_X(), Mouse_Y());
                    Mouse_Show();
                    break;
                default:
                    Mouse_Take_Click();
                    done = 1;
                }
            }
        } else {
            if (Key_Pressed()) {
                key = Key_Read_Translated();
                switch (key) {
                case '\x91':                         /* F11 / F12: the developer keys */
                    Debug_Memory_Report_Key();
                    break;
                case '\x92':
                    Mouse_Hide();
                    Mouse_Restore_Under();
                    Debug_Screenshot_FLI();
                    Mouse_Save_Under(Mouse_X(), Mouse_Y());
                    Mouse_Draw_Pointer(Mouse_X(), Mouse_Y());
                    Mouse_Set_Position(Mouse_X(), Mouse_Y());
                    Mouse_Show();
                    break;
                default:
                    done = 1;
                }
            }
        }
        Mouse_Click_Handler_On();
        UI_Redraw_Frame_Wait();
    }
    if (clicked != 0) {
        Mouse_Click_Handler_Off();
        while (Mouse_Get_Buttons())
            UI_Redraw_Frame_Wait();
    }
    ui_control_count = ui_saved_control_count;
    Mouse_Click_Handler_On();
    Mouse_Take_Click();
    Mouse_Take_Event();
    return -1;
}

/* (direction) -> control: L / R / U / D, W / X / Y / Z = up-left / up-right / down-left / down-right; finds the
   control under the cursor and moves to its neighbour (UI_Find_Control_In_Direction); list dialogs scroll instead */
int UI_Nav_Move(char direction)
{
    int i;
    int x;
    int y;
    int current;
    int dx;
    int dy;
    int result;

    switch (direction) {
    case 'L':
        dx = -1;
        dy = 0;
        break;
    case 'R':
        dx = 1;
        dy = 0;
        break;
    case 'U':
        dx = 0;
        dy = -1;
        break;
    case 'D':
        dx = 0;
        dy = 1;
        break;
    case 'W':
        dx = -1;
        dy = -1;
        break;
    case 'X':
        dx = 1;
        dy = -1;
        break;
    case 'Y':
        dx = -1;
        dy = 1;
        break;
    case 'Z':
        dx = 1;
        dy = 1;
        break;
    default:
        dx = 0;
        dy = 0;
        break;
    }
    if (ui_list_dialog_active != 0 && dy != 0) {
        result = 0;
        result = UI_List_Step_Selection(dy);
        return result;
    }
    if (ui_mouse_present != 0) {
        x = Mouse_X();
        y = Mouse_Y();
    } else {
        x = ui_cursor_x;
        y = ui_cursor_y;
    }
    current = 0;
    for (i = ui_control_count - 1; i > 0; i--) {
        if (p_ui_controls[i].type < UI_GRID && p_ui_controls[i].y1 <= y + mouse_pointer_offset &&
                p_ui_controls[i].y2 >= y + mouse_pointer_offset && p_ui_controls[i].x1 <= x + mouse_pointer_offset &&
                p_ui_controls[i].x2 >= x + mouse_pointer_offset) {
            if (p_ui_controls[i].type != UI_TEXT_BUTTON)
                current = i;
            else if (p_ui_controls[i].enabled != 0)
                current = i;
        }
    }
    result = current;
    if (dx != 0 || dy != 0) {
        result = UI_Find_Control_In_Direction(dx, dy, current, x, y);
        if (result != current && result != 0) {
            ui_cursor_x = p_ui_controls[result].x1 + (p_ui_controls[result].x2 - p_ui_controls[result].x1) / 2;
            ui_cursor_y = p_ui_controls[result].y1 + (p_ui_controls[result].y2 - p_ui_controls[result].y1) / 2;
            Mouse_Hide();
            if (ui_cursor_x > 0 && ui_cursor_x < SCREEN_W && ui_cursor_y > 0 && ui_cursor_y < SCREEN_H) {
                Mouse_Find_Region(ui_cursor_x, ui_cursor_y);
                mouse_pointer_offset = Mouse_Get_Pointer_Offset();
                ui_cursor_x = ui_cursor_x - mouse_pointer_offset;
                ui_cursor_y = ui_cursor_y - mouse_pointer_offset;
                Mouse_Restore_Under();
                Mouse_Save_Under(ui_cursor_x, ui_cursor_y);
                Mouse_Draw_Pointer(ui_cursor_x, ui_cursor_y);
                Mouse_Set_Position(ui_cursor_x, ui_cursor_y);
            } else {
                result = current;
                ui_cursor_x = x;
                ui_cursor_y = y;
            }
            Mouse_Show();
        }
    }
    return result;
}

/* (control) the key loop of a type-4 text field: Enter, Esc or a click ends it; Backspace / Delete / Left / Right
   edit at the caret; letters, [ and \, digits . - / : and space are typed (the first Backspace clears the default text).
   Simtex name Input_Box_Popup (MoO2 symbols, module NAMESTAR); clarified using ReMoM repo
   Numbered: BUG #401 (BUGS_AND_QUIRKS.txt) */
void UI_Text_Edit_Run(int control)
{
    int printable;
    int max_len;
    int typed;
    int width;
    int font_h;
    int done;
    int idle;
    char key;
    char text[62];
    int len;
    int j;

    idle = 4;                                   /* 4 key polls end the edit, counted only while input_record_mode is -1:
                                                   only the dead Input_Record_Stop sets it - never in 1.31 */
    if (ui_mouse_present != 0) {
        while (Mouse_Get_Buttons())
            UI_Call_Redraw_Fn();
    }
    ui_edit_caret = 0;
    ui_edit_blink = 0;
    max_len = p_ui_controls[control].param5;
    width = p_ui_controls[control].x2 - p_ui_controls[control].x1;
    typed = 0;
    Mouse_Hide();
    len = 0;
    key = 0;
    Font_Select(p_ui_controls[control].font, p_ui_controls[control].color, p_ui_controls[control].color2);
    strcpy(text, (char *)p_ui_controls[control].param0);
    while (text[len] != 0)
        len++;
    while (Text_Width(text) > width && len > 0) {
        len -= len;         /* = 0 (as compiled): a default wider than the box is emptied, not trimmed */
        text[len] = 0;
    }
    ui_edit_caret = len;
    if (ui_edit_caret >= max_len)
        ui_edit_caret = max_len;
    strcpy((char *)p_ui_controls[control].param0, text);
    font_h = Font_Get_Height();
    UI_Text_Edit_Draw(control, text);
    done = 0;
    while (key != KEY_ENTER && done == 0) {
        while (!Key_Pressed() && done == 0) {
            if (input_record_mode == -1)
                idle--;
            if (idle == 0)
                break;
            if (ui_mouse_present != 0 && (Mouse_Get_Buttons() || Mouse_Take_Click())) {
                done = 1;
                break;
            }
            ui_edit_blink++;
            if (ui_edit_blink > font_h * 2 - 1)
                ui_edit_blink = 0;
            UI_Text_Edit_Draw(control, text);
        }
        if (idle == 0)
            break;
        if (done != 0)
            break;
        key = Key_Read_Translated();
        switch (key) {
        case KEY_BACKSPACE:
            if (typed == 0) {
                text[0] = 0;
                len = 0;
                ui_edit_caret = 0;
                ui_edit_blink = 0;
                typed = 1;
            } else if (len > 0) {
                if (ui_edit_caret >= len) {
                    len = len - 1;
                    text[len] = 0;
                    ui_edit_caret--;
                    ui_edit_blink = 0;
                } else if (ui_edit_caret > 0) {
                    for (j = ui_edit_caret; j < len; j++)
                        text[j - 1] = text[j];
                    len--;
                    ui_edit_caret--;
                    ui_edit_blink = 0;
                    text[len] = 0;
                }
            }
            break;
        case KEY_DELETE:
            if (len > 0 && ui_edit_caret < len) {
                for (j = ui_edit_caret; j < len; j++)
                    text[j] = text[j + 1];
                len--;
                ui_edit_blink = 0;
                text[len] = 0;
            }
            break;
        case KEY_LEFT:
            typed = 1;
            if (ui_edit_caret > 0) {
                ui_edit_caret--;
                ui_edit_blink = 0;
            }
            break;
        case KEY_RIGHT:
            if (ui_edit_caret < max_len && ui_edit_caret < len) {
                ui_edit_caret++;
                ui_edit_blink = 0;
                if (ui_edit_caret >= len) {
                    text[len] = ' ';
                    text[len + 1] = 0;
                    if (ui_edit_caret >= max_len || Text_Width(text) > width)
                        ui_edit_caret--;
                    text[len] = 0;
                }
            }
            break;
        default:
            printable = 0;
            if (key == KEY_ESC) {
                done = 2;   /* Esc (1: a click; Enter ends the loop with done 0) */
                printable = 0;
            } else if ((key > '@' && key < ']') || (key > '-' && key < ';') || key == ' ' || key == '-' ||
                    (key > 'a' - 1 && key < 'z' + 1)) {
                printable = 1;
            }
            if (printable == 1) {
                typed = 1;
                text[len] = key;
                text[len + 1] = 0;
                if (len < max_len && Text_Width(text) <= width) {
                    text[len] = 0;
                    if (ui_edit_caret < len) {
                        for (j = len; j > ui_edit_caret; j--)
                            text[j] = text[j - 1];
                        len++;
                        text[ui_edit_caret] = key;
                        ui_edit_caret++;
                    } else {
                        text[len] = key;
                        len = len + 1;
                        text[len] = ' ';
                        text[len + 1] = 0;
                        if (len < max_len && Text_Width(text) <= width)
                            ui_edit_caret++;
                    }
                    text[len] = 0;
                    ui_edit_blink = 0;
                } else {
                    text[len] = 0;
                }
            }
            break;
        }
        UI_Text_Edit_Draw(control, text);
    }
    strcpy((char *)p_ui_controls[control].param0, text);
    if (done != 0 && ui_mouse_present != 0) {
        while (Mouse_Get_Buttons())
            ;
    }
    Mouse_Restore_Under();
    Mouse_Show();
    ui_highlight_control = -1;
    ui_edit_caret = 0;
    ui_edit_blink = 0;
}

/* (x, y, width, buffer, max_len, fill color, right_align, caret style, caret colors, help) -> 0 (Enter / click) or -1
   (Esc): a one-shot text entry box with its own key loop (UI_Text_Edit_Run's, on a temporary control).
   Simtex name Setup_Input_Box_Popup (MoO2 symbols, module NAMESTAR); clarified using ReMoM repo
   Numbered: BUG #401 (BUGS_AND_QUIRKS.txt) */
int UI_Text_Input_Box(int x, int y, int width, char *buffer, int max_len, int fill, int right_align, int caret_style,
                      int caret_colors, int help)
{
    int printable;
    int typed;
    int font_h;
    int idle;
    int done;
    int control;
    char key;
    char text[62];
    int len;
    int j;

    idle = 4;                                   /* as in UI_Text_Edit_Run: never counted down in 1.31 */
    UI_Clear_Controls();
    if (ui_mouse_present != 0) {
        while (Mouse_Get_Buttons())
            ;
        Mouse_Take_Click();
        Mouse_Take_Event();
    }
    UI_Input_Delay(1);
    control = UI_Add_Text_Input(x, y, width, buffer, max_len, fill, right_align, caret_style, caret_colors, "", help);
    ui_highlight_control = control;
    ui_edit_caret = 0;
    ui_edit_blink = 0;
    ui_edit_caret_visible = 0;
    typed = 0;
    len = 0;
    key = 0;
    font_h = Font_Get_Height();
    strcpy(text, buffer);
    while (text[len] != 0)
        len++;
    while (Text_Width(text) > width && len > 0) {
        len -= len;         /* = 0 (as compiled): a default wider than the box is emptied, not trimmed */
        text[len] = 0;
    }
    ui_edit_caret = len;
    if (ui_edit_caret >= max_len)
        ui_edit_caret = max_len;
    UI_Text_Edit_Draw(control, text);
    done = 0;
    while (key != KEY_ENTER && done == 0) {
        while (!Key_Pressed() && done == 0) {
            if (input_record_mode == -1)
                idle--;
            if (idle == 0)
                break;
            if (ui_mouse_present != 0 && (Mouse_Get_Buttons() || Mouse_Take_Click())) {
                done = 1;
                break;
            }
            ui_edit_blink++;
            if (ui_edit_blink > font_h * 2 - 1)
                ui_edit_blink = 0;
            UI_Text_Edit_Draw(control, text);
        }
        if (idle == 0)
            break;
        if (done != 0)
            break;
        key = Key_Read_Translated();
        switch (key) {
        case KEY_BACKSPACE:
            if (typed == 0) {
                text[0] = 0;
                len = 0;
                ui_edit_caret = 0;
                ui_edit_blink = 0;
                typed = 1;
            } else if (len > 0) {
                if (ui_edit_caret >= len) {
                    len = len - 1;
                    text[len] = 0;
                    ui_edit_caret--;
                    ui_edit_blink = 0;
                } else if (ui_edit_caret > 0) {
                    for (j = ui_edit_caret; j < len; j++)
                        text[j - 1] = text[j];
                    len--;
                    ui_edit_caret--;
                    ui_edit_blink = 0;
                    text[len] = 0;
                }
            }
            break;
        case KEY_DELETE:
            if (len > 0 && ui_edit_caret < len) {
                for (j = ui_edit_caret; j < len; j++)
                    text[j] = text[j + 1];
                len--;
                ui_edit_blink = 0;
                text[len] = 0;
            }
            break;
        case KEY_LEFT:
            typed = 1;
            if (ui_edit_caret > 0) {
                ui_edit_caret--;
                ui_edit_blink = 0;
            }
            break;
        case KEY_RIGHT:
            if (ui_edit_caret < max_len && ui_edit_caret < len) {
                ui_edit_caret++;
                ui_edit_blink = 0;
                if (ui_edit_caret == len) {
                    text[len] = ' ';
                    text[len + 1] = 0;
                    if (Text_Width(text) > width)
                        ui_edit_caret--;
                    text[len] = 0;
                }
            }
            break;
        default:
            printable = 0;
            if (key == KEY_ESC) {
                done = 2;   /* Esc (1: a click; Enter ends the loop with done 0) */
                printable = 0;
            } else if ((key > '@' && key < ']') || (key > '-' && key < ';') || key == ' ' || key == '-' ||
                    (key > 'a' - 1 && key < 'z' + 1)) {
                printable = 1;
            }
            if (printable == 1) {
                typed = 1;
                text[len] = key;
                text[len + 1] = 0;
                if (len < max_len && Text_Width(text) <= width) {
                    text[len] = 0;
                    if (ui_edit_caret < len) {
                        for (j = len; j > ui_edit_caret; j--)
                            text[j] = text[j - 1];
                        len++;
                        text[ui_edit_caret] = key;
                        ui_edit_caret++;
                    } else {
                        text[len] = key;
                        len = len + 1;
                        text[len] = ' ';
                        text[len + 1] = 0;
                        if (len < max_len && Text_Width(text) <= width)
                            ui_edit_caret++;
                    }
                    text[len] = 0;
                    ui_edit_blink = 0;
                } else {
                    text[len] = 0;
                }
            }
            break;
        }
        UI_Text_Edit_Draw(control, text);
    }
    strcpy(buffer, text);
    if (done != 0 && ui_mouse_present != 0) {
        while (Mouse_Get_Buttons())
            ;
    }
    ui_highlight_control = -1;
    UI_Clear_Controls();
    Mouse_Take_Click();
    Mouse_Take_Event();
    ui_edit_caret = 0;
    ui_edit_blink = 0;
    if (done == 2)
        return -1;
    return 0;
}

/* (control, text) draws a text field with its caret: style 0 a block rising / falling with the blink counter in the
   caret colors (one per font row), style 1 a blinking underline.
   Simtex name Draw_Input_Box_Popup (MoO2 symbols, module NAMESTAR); clarified using ReMoM repo */
void UI_Text_Edit_Draw(int control, char *str)
{
    char one[2];
    int k;
    int char_w;
    int caret_x;
    int h;
    int base_y;
    int width;
    char text[62];
    unsigned char colors[16];
    int ctl;
    int j;

    Timer_Mark();
    Set_Draw_Page();
    Mem_Copy(colors, (char *)p_ui_controls[control].param6, Font_Get_Height());   /* param6 of a text control = its caret colors (near pointer kept in the int field) */
    strcpy(text, str);
    UI_Call_Redraw_Fn();
    width = p_ui_controls[control].x2 - p_ui_controls[control].x1;
    ctl = control;
    Font_Select(p_ui_controls[ctl].font, p_ui_controls[ctl].color, p_ui_controls[ctl].color2);
    h = Font_Get_Height() - 1;
    if (p_ui_controls[ctl].param2 != 0)
        Fill_Rect(p_ui_controls[ctl].x1 - 1, p_ui_controls[ctl].y1, p_ui_controls[ctl].x2, p_ui_controls[ctl].y2,
                  p_ui_controls[ctl].param2);
    one[1] = 0;
    switch (p_ui_controls[ctl].param4) {
    case 0:
        one[0] = text[ui_edit_caret];
        text[ui_edit_caret] = 0;
        caret_x = p_ui_controls[ctl].x1 + Text_Width(text);
        text[ui_edit_caret] = one[0];
        if (one[0] == 0)
            one[0] = ' ';
        char_w = Text_Width(one);
        if (ui_edit_blink > 0 && ui_edit_blink <= h) {
            j = ui_edit_blink;
            base_y = p_ui_controls[ctl].y1 + h;
            for (k = 0; j > 0; k++, j--)
                Draw_Line(caret_x, base_y - j + 1, caret_x + char_w + 1, base_y - j + 1, colors[k]);
        } else if (ui_edit_blink != 0) {
            j = h - (ui_edit_blink - h);
            for (k = 0; j > 0; k++, j--)
                Draw_Line(caret_x, p_ui_controls[ctl].y1 + j - 1, caret_x + char_w + 1, p_ui_controls[ctl].y1 + j - 1,
                          colors[h - k - 1]);
        }
        break;
    case 1:
        one[0] = text[ui_edit_caret];
        text[ui_edit_caret] = 0;
        caret_x = p_ui_controls[ctl].x1 + Text_Width(text);
        text[ui_edit_caret] = one[0];
        if (one[0] == 0)
            one[0] = ' ';
        char_w = Text_Width(one);
        j = ui_edit_blink % 3;
        if (j == 0 && ui_edit_caret_visible == 0)
            ui_edit_caret_visible = 1;
        else if (j == 0 && ui_edit_caret_visible == 1)
            ui_edit_caret_visible = 0;
        base_y = p_ui_controls[ctl].y1 + h;
        if (ui_edit_caret_visible != 0)
            Draw_Line(caret_x, base_y + 1, caret_x + char_w + 1, base_y + 1, colors[0]);
        break;
    default:
        break;
    }
    Font_Select(p_ui_controls[ctl].font, p_ui_controls[ctl].color, p_ui_controls[ctl].color2);
    Text_Toggle_Highlight();
    Print_Text(p_ui_controls[control].x1, p_ui_controls[control].y1, text);
    Palette_Upload();
    UI_Present_Frame();
    Wait_Ticks(ui_frame_ticks);
}

/* (x, y, width, buffer, max_len, fill color, right_align, caret style, caret colors, hotkey, help) -> control: a type-4
   text field (box x .. x + width, y .. y + font height) in the current font and colors.
   Simtex name Add_Input_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Add_Text_Input(int x, int y, int width, char *buffer, int max_len, int fill, int right_align, int caret_style,
                      int caret_colors, char *hotkey, int help)
{
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y;
    p_ui_controls[ui_control_count].font = Font_Get_Current();
    p_ui_controls[ui_control_count].color = Font_Get_Color1();
    p_ui_controls[ui_control_count].color2 = Font_Get_Color2();
    p_ui_controls[ui_control_count].x2 = x + width;
    p_ui_controls[ui_control_count].y2 = y + Font_Get_Height();
    p_ui_controls[ui_control_count].param5 = max_len;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].param0 = (int)buffer;
    p_ui_controls[ui_control_count].param2 = fill;
    p_ui_controls[ui_control_count].param3 = right_align;
    p_ui_controls[ui_control_count].param4 = caret_style;
    p_ui_controls[ui_control_count].param6 = caret_colors;
    p_ui_controls[ui_control_count].type = UI_TEXT_INPUT;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, width, text, max_len, fill color, edit_pic, help, shadow_style) -> control: a type-11 editable text line
   (the save-game names).
   Simtex name Add_Continuous_String_Input_Field (MoO2 symbols, module fields); clarified using ReMoM repo
   Numbered: BUG #373 (BUGS_AND_QUIRKS.txt) */
int UI_Add_Text_Line(int x, int y, int width, char *text, int max_len, int fill, int edit_pic, int help,
                     int shadow_style)
{
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y;
    p_ui_controls[ui_control_count].font = Font_Get_Current();
    p_ui_controls[ui_control_count].color = Font_Get_Color1();
    p_ui_controls[ui_control_count].color2 = Font_Get_Color2();
    p_ui_controls[ui_control_count].x2 = x + width;
    p_ui_controls[ui_control_count].y2 = y + Font_Get_Height();
    p_ui_controls[ui_control_count].param5 = max_len;
    p_ui_controls[ui_control_count].hotkey = 0;
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].param0 = (int)text;
    p_ui_controls[ui_control_count].param1 = edit_pic;
    p_ui_controls[ui_control_count].param2 = fill;
    p_ui_controls[ui_control_count].param6 = shadow_style;
    p_ui_controls[ui_control_count].type = UI_TEXT_LINE;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (control) the box one pixel shorter (its bottom edge one up) */
void UI_Control_Shrink_Bottom(int control)
{
    p_ui_controls[control].y2 = p_ui_controls[control].y2 - 1;
}

/* (x, y, width, text, enabled, &current, value, fill mode, fill 1..4, hotkey, help) -> control: a type-10 text button
   (a list line: selected when *current == value).
   Simtex name Add_String_List_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Add_Text_Button(int x, int y, int width, char *text, int enabled, int *current, int value, int fill_mode,
                       int fill1, int fill2, int fill3, int fill4, char *hotkey, int help)
{
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y - 1;
    p_ui_controls[ui_control_count].font = Font_Get_Current();
    p_ui_controls[ui_control_count].color = Font_Get_Color1();
    p_ui_controls[ui_control_count].color2 = Font_Get_Color2();
    p_ui_controls[ui_control_count].x2 = x + width;
    p_ui_controls[ui_control_count].y2 = y + Font_Get_Height() + 1;
    p_ui_controls[ui_control_count].param1 = value;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    p_ui_controls[ui_control_count].enabled = enabled;
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].param0 = (int)text;
    p_ui_controls[ui_control_count].param2 = (int)current;
    p_ui_controls[ui_control_count].fill_mode = fill_mode;
    p_ui_controls[ui_control_count].param3 = fill1;
    p_ui_controls[ui_control_count].param4 = fill2;
    p_ui_controls[ui_control_count].param5 = fill3;
    p_ui_controls[ui_control_count].param6 = fill4;
    p_ui_controls[ui_control_count].type = UI_TEXT_BUTTON;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (code) -> control: an off-screen type-8 control for an Alt+letter code (several letters are typed in turn).
   Simtex name Add_Multi_Hot_Key_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Add_Alt_Hotkey(char *code)
{
    char text[30];
    int len;

    strcpy(text, code);
    for (len = 0; text[len] != 0 && len < 30; len++)
        ;
    p_ui_controls[ui_control_count].x1 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].y1 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].x2 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].y2 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].type = UI_ALT_HOTKEY;
    p_ui_controls[ui_control_count].help = -1;
    p_ui_controls[ui_control_count].param0 = (int)code;
    p_ui_controls[ui_control_count].param1 = 0;
    p_ui_controls[ui_control_count].param2 = len;
    p_ui_controls[ui_control_count].hotkey = text[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, pic, hotkey, help) -> control: a type-5 area the size of the picture */
int UI_Add_Picture(int x, int y, int pic, char *hotkey, int help)
{
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y;
    p_ui_controls[ui_control_count].x2 = p_ui_controls[ui_control_count].x1 + Pic_Width(pic);
    p_ui_controls[ui_control_count].y2 = p_ui_controls[ui_control_count].y1 + Pic_Height(pic);
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].param5 = pic;
    p_ui_controls[ui_control_count].type = UI_PICTURE;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, pic, hotkey, help) -> control: the same clipped to the clip rectangle; UI_CLIPPED_AWAY when fully outside */
int UI_Add_Picture_Clipped(int x, int y, int pic, char *hotkey, int help)
{
    int x2;
    int y2;

    x2 = x + Pic_Width(pic);
    y2 = y + Pic_Height(pic);
    if (x2 < clip_x1 || x > clip_x2)
        return UI_CLIPPED_AWAY;
    if (y2 < clip_y1 || y > clip_y2)
        return UI_CLIPPED_AWAY;
    if (x >= clip_x1)
        p_ui_controls[ui_control_count].x1 = x;
    else
        p_ui_controls[ui_control_count].x1 = clip_x1;
    if (x2 <= clip_x2)
        p_ui_controls[ui_control_count].x2 = x2;
    else
        p_ui_controls[ui_control_count].x2 = clip_x2;
    if (y >= clip_y1)
        p_ui_controls[ui_control_count].y1 = y;
    else
        p_ui_controls[ui_control_count].y1 = clip_y1;
    if (y2 <= clip_y2)
        p_ui_controls[ui_control_count].y2 = y2;
    else
        p_ui_controls[ui_control_count].y2 = clip_y2;
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].param5 = pic;
    p_ui_controls[ui_control_count].type = UI_PICTURE;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (dx, dy) how far a pressed button's label moves (default 1, -1).
   Simtex name Set_Button_Down_Offsets (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Set_Press_Offset(int dx, int dy)
{
    ui_press_dx = dx;
    ui_press_dy = dy;
}

/* (x, y, label, pic, hotkey, help) fills the next record for a picture button (box = the picture).
   Simtex name Add_Button_Info (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Init_Pic_Control(int x, int y, char *label, int pic, char *hotkey, int help)
{
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y;
    p_ui_controls[ui_control_count].param0 = (int)label;
    p_ui_controls[ui_control_count].x2 = p_ui_controls[ui_control_count].x1 + Pic_Width(pic) - 1;
    p_ui_controls[ui_control_count].y2 = p_ui_controls[ui_control_count].y1 + Pic_Height(pic) - 1;
    p_ui_controls[ui_control_count].font = Font_Get_Current();
    p_ui_controls[ui_control_count].color = Font_Get_Color1();
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].param5 = pic;
    p_ui_controls[ui_control_count].param3 = Pic_Full_Frames_Flag(pic);
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
}

/* (x, y, label, pic, hotkey, help) -> control: a type-0 picture button.
   Simtex name Add_Button_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help)
{
    UI_Init_Pic_Control(x, y, label, pic, hotkey, help);
    p_ui_controls[ui_control_count].type = UI_BUTTON;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, label, pic, &flag, hotkey, help) -> control: a type-1 picture button that flips *flag */
int UI_Add_Toggle_Button(int x, int y, char *label, int pic, int *flag, char *hotkey, int help)
{
    UI_Init_Pic_Control(x, y, label, pic, hotkey, help);
    p_ui_controls[ui_control_count].type = UI_TOGGLE;
    p_ui_controls[ui_control_count].param2 = (int)flag;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, label, pic, &flag, hotkey, help) -> control: a type-2 picture button that sets *flag = 1 while it is 0 */
int UI_Add_Latch_Button(int x, int y, char *label, int pic, int *flag, char *hotkey, int help)
{
    UI_Init_Pic_Control(x, y, label, pic, hotkey, help);
    p_ui_controls[ui_control_count].type = UI_LATCH;
    p_ui_controls[ui_control_count].param2 = (int)flag;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* DEAD: (x, y, label, pic, &current, value, hotkey, help) -> control: a type-3 picture control (UI_Draw_Controls
   compares *current with value, as for a text button) */
int UI_Add_Pic_Value_Control(int x, int y, char *label, int pic, int current, int value, char *hotkey, int help)
{
    UI_Init_Pic_Control(x, y, label, pic, hotkey, help);
    p_ui_controls[ui_control_count].type = UI_PIC_VALUE;
    p_ui_controls[ui_control_count].param2 = current;
    p_ui_controls[ui_control_count].param1 = value;
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x1, y1, x2, y2, hotkey, help) -> control: a type-7 rectangle.
   Simtex name Add_Hidden_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Add_Region(int x1, int y1, int x2, int y2, char *hotkey, int help)
{
    p_ui_controls[ui_control_count].x1 = x1;
    p_ui_controls[ui_control_count].y1 = y1;
    p_ui_controls[ui_control_count].x2 = x2;
    p_ui_controls[ui_control_count].y2 = y2;
    p_ui_controls[ui_control_count].type = UI_REGION;
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x1, y1, x2, y2, hotkey, help) -> control: the same clipped to the clip rectangle; UI_CLIPPED_AWAY when fully outside */
int UI_Add_Region_Clipped(int x1, int y1, int x2, int y2, char *hotkey, int help)
{
    if (x2 < clip_x1 || x1 > clip_x2)
        return UI_CLIPPED_AWAY;
    if (y2 < clip_y1 || y1 > clip_y2)
        return UI_CLIPPED_AWAY;
    if (x1 >= clip_x1)
        p_ui_controls[ui_control_count].x1 = x1;
    else
        p_ui_controls[ui_control_count].x1 = clip_x1;
    if (x2 <= clip_x2)
        p_ui_controls[ui_control_count].x2 = x2;
    else
        p_ui_controls[ui_control_count].x2 = clip_x2;
    if (y1 >= clip_y1)
        p_ui_controls[ui_control_count].y1 = y1;
    else
        p_ui_controls[ui_control_count].y1 = clip_y1;
    if (y2 <= clip_y2)
        p_ui_controls[ui_control_count].y2 = y2;
    else
        p_ui_controls[ui_control_count].y2 = clip_y2;
    p_ui_controls[ui_control_count].type = UI_REGION;
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (key) -> control: an off-screen type-7 control matched by the key alone */
int UI_Add_Hotkey(char *key)
{
    p_ui_controls[ui_control_count].x1 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].y1 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].x2 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].y2 = UI_OFFSCREEN;
    p_ui_controls[ui_control_count].type = UI_REGION;
    p_ui_controls[ui_control_count].help = -1;
    p_ui_controls[ui_control_count].hotkey = key[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (value, &out, stored_value, x1, y1, x2, y2, hotkey, help) -> control: a type-9 rectangle; a hit stores stored_value in *out and
   returns value (UI_Hit_Test) */
int UI_Add_Value_Region(int value, int *out, int stored_value, int x1, int y1, int x2, int y2, char *hotkey, int help)
{
    p_ui_controls[ui_control_count].x1 = x1;
    p_ui_controls[ui_control_count].y1 = y1;
    p_ui_controls[ui_control_count].x2 = x2;
    p_ui_controls[ui_control_count].y2 = y2;
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].type = UI_VALUE_REGION;
    p_ui_controls[ui_control_count].param1 = stored_value;
    p_ui_controls[ui_control_count].param0 = value;
    p_ui_controls[ui_control_count].param2 = (int)out;
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, cell_w, cell_h, cols, rows, &col, &row, help) -> control: a type-12 grid of cols x rows cells; a hit
   stores the cell under the mouse in *col / *row */
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help)
{
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y;
    p_ui_controls[ui_control_count].x2 = x + cell_w * cols - 1;
    p_ui_controls[ui_control_count].y2 = y + cell_h * rows - 1;
    p_ui_controls[ui_control_count].help = help;
    p_ui_controls[ui_control_count].type = UI_GRID;
    p_ui_controls[ui_control_count].param1 = cell_w;
    p_ui_controls[ui_control_count].param2 = cell_h;
    p_ui_controls[ui_control_count].hotkey = 0;
    p_ui_controls[ui_control_count].param3 = (int)col;
    p_ui_controls[ui_control_count].param4 = (int)row;
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (x, y, scale_min, scale_max, min, max, width, height, &value, hotkey, help) a slider whose value is in DGROUP;
   int with no return statement: UI_Add_Slider's control number is left in AX */
int UI_Add_Slider_Near(int x, int y, int scale_min, int scale_max, int min, int max, int width, int height,
                       int *value, char *hotkey, int help)
{
    UI_Add_Slider(x, y, scale_min, scale_max, min, max, width, height, value, 0, hotkey, help);
}

/* (x, y, scale_min, scale_max, min, max, width, height, far &value (segment 0 = DGROUP), hotkey, help) -> control: a
   type-6 slider, vertical when height >= width */
int UI_Add_Slider(int x, int y, int scale_min, int scale_max, int min, int max, int width, int height,
                  int far *value, char *hotkey, int help)
{
    if (FP_SEG(value) == 0) {
        _AX = _DS;
        *((unsigned *)&value + 1) = _AX;
    }
    p_ui_controls[ui_control_count].x1 = x;
    p_ui_controls[ui_control_count].y1 = y;
    p_ui_controls[ui_control_count].x2 = p_ui_controls[ui_control_count].x1 + width;
    p_ui_controls[ui_control_count].y2 = p_ui_controls[ui_control_count].y1 + height;
    p_ui_controls[ui_control_count].param0 = min;
    p_ui_controls[ui_control_count].param1 = max;
    p_ui_controls[ui_control_count].param3 = scale_min;
    p_ui_controls[ui_control_count].param4 = scale_max;
    p_ui_controls[ui_control_count].help = help;
    if (width > height)
        p_ui_controls[ui_control_count].param5 = 0;
    else
        p_ui_controls[ui_control_count].param5 = 1;
    p_ui_controls[ui_control_count].type = UI_SLIDER;
    p_ui_controls[ui_control_count].param2 = FP_SEG(value);
    p_ui_controls[ui_control_count].param6 = FP_OFF(value);
    p_ui_controls[ui_control_count].hotkey = hotkey[0];
    if (p_ui_controls[ui_control_count].hotkey > 'a' - 1 && p_ui_controls[ui_control_count].hotkey < 'z' + 1)
        p_ui_controls[ui_control_count].hotkey = p_ui_controls[ui_control_count].hotkey - ('a' - 'A');
    ui_control_count = ui_control_count + 1;
    return ui_control_count - 1;
}

/* (control_index: the slider control) the slider value from the mouse position, clamped to min .. max and stored through the far value
   pointer; the vertical branch leaves out '+ scale_min' (every vertical slider has scale_min 0)
   Numbered: BUG #403 (BUGS_AND_QUIRKS.txt) */
void Slider_Set_From_Mouse(int control_index)
{
    int range;
    int value;
    int value_seg;
    int value_off;
    long scaled;
    register int control = control_index;
    register int pos;

    if (p_ui_controls[control].param5 == 0) {
        range = p_ui_controls[control].x2 - p_ui_controls[control].x1;
        pos = Mouse_X() + mouse_pointer_offset;
        scaled = (long)(pos - p_ui_controls[control].x1) * (p_ui_controls[control].param4 - p_ui_controls[control].param3) / range;
        if (p_ui_controls[control].x2 <= pos)
            value = p_ui_controls[control].param4;
        else if (p_ui_controls[control].x1 >= pos)
            value = p_ui_controls[control].param3;
        else
            value = p_ui_controls[control].param3 + (int)scaled;
    } else {
        range = p_ui_controls[control].y2 - p_ui_controls[control].y1;
        pos = Mouse_Y() + mouse_pointer_offset;
        scaled = (long)(p_ui_controls[control].y2 - pos) * (p_ui_controls[control].param4 - p_ui_controls[control].param3) / range;
        if (p_ui_controls[control].y2 <= pos)
            value = p_ui_controls[control].param3;
        else if (p_ui_controls[control].y1 >= pos)
            value = p_ui_controls[control].param4;
        else
            value = (int)scaled;
    }
    if (p_ui_controls[control].param0 > value)
        value = p_ui_controls[control].param0;
    if (p_ui_controls[control].param1 < value)
        value = p_ui_controls[control].param1;
    value_seg = p_ui_controls[control].param2;
    value_off = p_ui_controls[control].param6;
    asm push es
    asm mov ax, value
    asm mov cx, value_seg
    asm mov bx, value_off
    asm mov es, cx
    asm mov es:[bx], ax
    asm pop es
}

/* (control) the + key on a slider: 5 % of the scale up, clamped to max */
void Slider_Step_Up(int control_index)
{
    int range;
    int value;
    int value_seg;
    int value_off;
    long scaled;
    register int control = control_index;
    register int percent;

    value_seg = p_ui_controls[control].param2;
    value_off = p_ui_controls[control].param6;
    asm push es
    asm mov cx, value_seg
    asm mov bx, value_off
    asm mov es, cx
    asm mov ax, es:[bx]
    asm mov value, ax
    asm pop es
    range = p_ui_controls[control].param4 - p_ui_controls[control].param3;
    percent = 100L * (value - p_ui_controls[control].param3) / range;
    percent += 5;
    if (percent > 100) {
        value = p_ui_controls[control].param1;
    } else {
        scaled = (long)percent * range / 100;
        value = p_ui_controls[control].param3 + (int)scaled;
    }
    if (p_ui_controls[control].param1 < value)
        value = p_ui_controls[control].param1;
    asm push es
    asm mov ax, value
    asm mov cx, value_seg
    asm mov bx, value_off
    asm mov es, cx
    asm mov es:[bx], ax
    asm pop es
}

/* (control) the - key on a slider: 5 % of the scale down, clamped to min */
void Slider_Step_Down(int control_index)
{
    int range;
    int value;
    int value_seg;
    int value_off;
    long scaled;
    register int control = control_index;
    register int percent;

    value_seg = p_ui_controls[control].param2;
    value_off = p_ui_controls[control].param6;
    asm push es
    asm mov cx, value_seg
    asm mov bx, value_off
    asm mov es, cx
    asm mov ax, es:[bx]
    asm mov value, ax
    asm pop es
    range = p_ui_controls[control].param4 - p_ui_controls[control].param3;
    percent = 100L * (value - p_ui_controls[control].param3) / range;
    percent -= 5;
    if (percent < 0) {
        value = p_ui_controls[control].param0;
    } else {
        scaled = (long)percent * range / 100;
        value = p_ui_controls[control].param3 + (int)scaled;
    }
    if (p_ui_controls[control].param0 > value)
        value = p_ui_controls[control].param0;
    asm push es
    asm mov ax, value
    asm mov cx, value_seg
    asm mov bx, value_off
    asm mov es, cx
    asm mov es:[bx], ax
    asm pop es
}

/* () removes every control (control 0 stays reserved).
   Simtex name Clear_Fields (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Clear_Controls(void)
{
    ui_control_count = 1;
    ui_highlight_control = -1;
    ui_pressed_control = 0;
    ui_edit_active = 0;
    ui_edit_control = -1;
}

/* (style) the text buttons' second Font_Select color set (ui_button_color2) 1 .. 14, else 0 */
void UI_Set_Style(int style)
{
    if (style > 0 && style < 15)
        ui_button_color2 = style;
    else
        ui_button_color2 = 0;
}

/* DEAD (no callers): (x, y, width, title, items, stride, &selection, enabled[] or 0, visible rows, up button x, y,
   pic, down button x, y, pic, fill mode, fill 1..4, help) -> chosen item or -1: a scrolling list dialog - one text
   button per visible item (the items end at an empty string), latch buttons and the list keys scroll */
int UI_Text_List_Select_Scrolling(int x, int y, int width, char *title, char *items, int stride, int *selection, int *enabled,
                  int rows, int up_x, int up_y, int up_pic, int down_x, int down_y, int down_pic, int fill_mode,
                  int fill1, int fill2, int fill3, int fill4, int help)
{
    int done;
    int n;                                      /* first a found flag, then the item count */
    int line_h;
    int font_h;
    int row_y;
    int top;
    int title_control;
    char *item;
    int input;                                  /* first the first enabled item, then UI_Get_Input's result */
    int copied;
    int hit;
    int title_state;
    int right;
    int row;
    int up_control;
    int down_control;
    int color3;
    int en;
    int color2;
    int old_selection;
    int shown;
    int up_flag;
    int down_flag;
    char first[6];
    int i;

    ui_list_dialog_active = 1;
    ui_list_dialog_scrolling = 1;
    ui_list_scroll_req = 0;
    color3 = Font_Get_Color3();
    color2 = Font_Get_Color2();
    UI_Input_Delay(1);
    UI_Clear_Controls();
    right = x + width;
    font_h = Font_Get_Height();
    line_h = font_h + Get_Text_Spacing();
    done = 0;
    i = 0;
    n = 0;
    item = items;
    row_y = y + line_h;
    input = 0;
    row = 0;
    while (done == 0) {
        Mem_Copy(first, item, 2);
        if (first[0] != 0) {
            if (n == 0 && (enabled[i] != 0 || enabled == 0)) {
                input = i;
                n = 1;
            }
            i++;
        } else {
            done = 1;
        }
        item += stride;
    }
    n = i;
    i = 0;
    if (*selection >= 0 && *selection < n)
        i = *selection;
    if (i + rows > n) {
        i = n - rows;
        if (i < 0)
            i = 0;
    }
    top = i;
    item = items + i * stride;
    row = 0;
    for (; i < n && row < rows; i++, row++) {
        if (enabled == 0)
            en = 1;
        else
            en = enabled[i];
        UI_Add_Text_Button(x, row_y, width, item, en, selection, i, fill_mode, fill1, fill2, fill3, fill4, "", help);
        row_y += line_h;
        item += stride;
    }
    shown = row;
    if (*selection < 0 || *selection >= n) {
        if (input > 0 && input < n)
            *selection = top;
        else
            *selection = -1;
    }
    Font_Select(Font_Get_Current(), Font_Get_Color1(), color3);
    title_state = 0;
    title_control = UI_Add_Text_Button(x, y, width, title, 0, &title_state, 1, 0, 0, 0, 0, 0, "", ui_list_title_help);
    if (top == 0)
        up_flag = 1;
    else
        up_flag = 0;
    if (i >= n)
        down_flag = 1;
    else
        down_flag = 0;
    up_control = UI_Add_Latch_Button(up_x, up_y, "", up_pic, &up_flag, "\x05", -1);
    down_control = UI_Add_Latch_Button(down_x, down_y, "", down_pic, &down_flag, "\x06", -1);
    done = 0;
    copied = 0;
    input = 0;
    old_selection = *selection;
    while (done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input < 0 || (input > 0 && input < up_control))
            done = 1;
        if ((input > 0 && input < up_control && p_ui_controls[input].enabled == 0) || input == title_control)
            done = 0;
        if (input == up_control) {
            UI_Clear_Controls();
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color2);
            i = top - rows;
            if (i < 0)
                i = 0;
            top = i;
            *selection = -1;
            if (enabled[i] != 0 || enabled == 0) {
                *selection = top;
            } else {
                for (; top + rows > i; i++) {
                    if (enabled[i] != 0 || enabled == 0) {
                        *selection = i;
                        break;
                    }
                }
            }
            i = top;
            item = items + i * stride;
            row = 0;
            row_y = y + line_h;
            for (; i < n && row < rows; i++, row++) {
                if (enabled == 0)
                    en = 1;
                else
                    en = enabled[i];
                UI_Add_Text_Button(x, row_y, width, item, en, selection, i, fill_mode, fill1, fill2, fill3, fill4, "", help);
                row_y += line_h;
                item += stride;
            }
            shown = row;
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color3);
            title_state = 0;
            UI_Add_Text_Button(x, y, width, title, 0, &title_state, 1, 0, 0, 0, 0, 0, "", ui_list_title_help);
            if (top == 0)
                up_flag = 1;
            else
                up_flag = 0;
            if (i >= n)
                down_flag = 1;
            else
                down_flag = 0;
            up_control = UI_Add_Latch_Button(up_x, up_y, "", up_pic, &up_flag, "\x05", -1);
            down_control = UI_Add_Latch_Button(down_x, down_y, "", down_pic, &down_flag, "\x06", -1);
        } else if (input == down_control) {
            UI_Clear_Controls();
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color2);
            i = top + rows;
            if (i + rows > n) {
                i = n - rows;
                if (i < 0)
                    i = 0;
            }
            if (i >= n)
                i = top;
            top = i;
            *selection = -1;
            if (enabled[top] != 0 || enabled == 0) {
                *selection = top;
            } else {
                for (; top + rows > i; i++) {
                    if (enabled[i] != 0 || enabled == 0) {
                        *selection = i;
                        break;
                    }
                }
            }
            row = 0;
            i = top;
            item = items + i * stride;
            row_y = y + line_h;
            for (; i < n && row < rows; i++, row++) {
                if (enabled == 0)
                    en = 1;
                else
                    en = enabled[i];
                UI_Add_Text_Button(x, row_y, width, item, en, selection, i, fill_mode, fill1, fill2, fill3, fill4, "", help);
                row_y += line_h;
                item += stride;
            }
            shown = row;
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color3);
            title_state = 0;
            UI_Add_Text_Button(x, y, width, title, 0, &title_state, 1, 0, 0, 0, 0, 0, "", ui_list_title_help);
            if (top == 0)
                up_flag = 1;
            else
                up_flag = 0;
            if (i >= n)
                down_flag = 1;
            else
                down_flag = 0;
            up_control = UI_Add_Latch_Button(up_x, up_y, "", up_pic, &up_flag, "\x05", -1);
            down_control = UI_Add_Latch_Button(down_x, down_y, "", down_pic, &down_flag, "\x06", -1);
        }
        if (ui_list_hit_enabled != 0) {
            hit = UI_Hit_Test();
            if (hit > 0)
                *selection = p_ui_controls[hit].param1;
        }
        if (ui_list_scroll_req == 1) {
            UI_Clear_Controls();
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color2);
            i = top + 1;                        /* shown: here the scan index */
            for (shown = top + rows; (enabled[shown] == 0 && shown < n) || enabled == 0; shown++)   /* QUIRK #485: never ends
                                                   when enabled == 0 (DEAD code) */
                i++;
            if (shown >= n)
                i = top;
            if (i + rows > n) {
                i = n - rows;
                if (i < 0)
                    i = 0;
            }
            top = i;
            item = items + i * stride;
            row = 0;
            row_y = y + line_h;
            for (; i < n && row < rows; i++, row++) {
                if (enabled == 0)
                    en = 1;
                else
                    en = enabled[i];
                UI_Add_Text_Button(x, row_y, width, item, en, selection, i, fill_mode, fill1, fill2, fill3, fill4, "", help);
                row_y += line_h;
                item += stride;
            }
            shown = row;
            if (enabled[i - 1] != 0 || enabled == 0)
                *(int *)p_ui_controls[shown].param2 = p_ui_controls[shown].param1;
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color3);
            title_state = 0;
            UI_Add_Text_Button(x, y, width, title, 0, &title_state, 1, 0, 0, 0, 0, 0, "", ui_list_title_help);
            if (top == 0)
                up_flag = 1;
            else
                up_flag = 0;
            if (i >= n)
                down_flag = 1;
            else
                down_flag = 0;
            up_control = UI_Add_Latch_Button(up_x, up_y, "", up_pic, &up_flag, "\x05", -1);
            down_control = UI_Add_Latch_Button(down_x, down_y, "", down_pic, &down_flag, "\x06", -1);
        }
        if (ui_list_scroll_req == -1) {
            UI_Clear_Controls();
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color2);
            i = top - 1;
            if (i < 0)
                i = 0;
            while ((enabled[i] == 0 && i > 0) || enabled == 0)
                i--;
            top = i;
            item = items + i * stride;
            row = 0;
            row_y = y + line_h;
            for (; i < n && row < rows; i++, row++) {
                if (enabled == 0)
                    en = 1;
                else
                    en = enabled[i];
                UI_Add_Text_Button(x, row_y, width, item, en, selection, i, fill_mode, fill1, fill2, fill3, fill4, "", help);
                row_y += line_h;
                item += stride;
            }
            shown = row;
            *selection = p_ui_controls[1].param1;
            Font_Select(Font_Get_Current(), Font_Get_Color1(), color3);
            title_state = 0;
            UI_Add_Text_Button(x, y, width, title, 0, &title_state, 1, 0, 0, 0, 0, 0, "", ui_list_title_help);
            if (top == 0)
                up_flag = 1;
            else
                up_flag = 0;
            if (i >= n)
                down_flag = 1;
            else
                down_flag = 0;
            up_control = UI_Add_Latch_Button(up_x, up_y, "", up_pic, &up_flag, "\x05", -1);
            down_control = UI_Add_Latch_Button(down_x, down_y, "", down_pic, &down_flag, "\x06", -1);
        }
        ui_list_scroll_req = 0;
        UI_Call_Redraw_Fn();
        Palette_Upload();
        UI_Present_Frame();
        if (copied == 0) {
            Screen_Copy_Front();
            copied = 1;
        }
        Wait_Ticks(ui_frame_ticks);
    }
    UI_Clear_Controls();
    ui_list_dialog_active = 0;
    ui_list_hit_enabled = 1;
    ui_list_dialog_scrolling = 0;
    Mouse_Take_Click();
    Mouse_Take_Event();
    if (input < 0) {
        *selection = old_selection;
        return -1;
    }
    return input + top - 1;
}

/* (x, y, width, title, items, stride, &selection, enabled[] or 0, fill mode, fill 1..4, help) -> chosen item or -1:
   one text button per item (the items end at an empty string; disabled items cannot be chosen), own input loop.
   Simtex name Get_List_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Text_List_Select(int x, int y, int width, char *title, char *items, int stride, int *selection, int *enabled,
                        int fill_mode, int fill1, int fill2, int fill3, int fill4, int help)
{
    int done;
    int n;
    int found;
    int line_h;
    int font_h;
    int row_y;
    int right;
    int title_control;
    char *item;
    int copied;
    int hit;
    int title_state;
    int en;
    char first[6];
    int input;                                  /* first the first enabled item, then UI_Get_Input's result */

    ui_list_dialog_active = 1;
    UI_Input_Delay(1);
    UI_Clear_Controls();
    right = x + width;
    font_h = Font_Get_Height();
    line_h = font_h + Get_Text_Spacing();
    done = 0;
    n = 0;
    found = 0;
    item = items;
    row_y = y;
    input = -1;
    while (done == 0) {
        Mem_Copy(first, item, 2);
        if (first[0] != 0) {
            if (found == 0 && (enabled[n] != 0 || enabled == 0)) {
                input = n;
                found = 1;
            }
            row_y += line_h;
            if (enabled == 0)
                en = 1;
            else
                en = enabled[n];
            UI_Add_Text_Button(x, row_y, width, item, en, selection, n, fill_mode, fill1, fill2, fill3, fill4, "", help);
            n++;
        } else {
            done = 1;
        }
        item += stride;
    }
    found = n;   /* from here: the item count */
    Font_Select(Font_Get_Current(), Font_Get_Color1(), Font_Get_Color3());
    title_state = 0;
    title_control = UI_Add_Text_Button(x, y, width, title, 0, &title_state, 1, 0, 0, 0, 0, 0, "", ui_list_title_help);
    if (*selection < 0 || *selection >= found || *selection < input) {
        if (input >= 0 && input < found)
            *selection = p_ui_controls[input + 1].param1;
        else
            *selection = -1;
    }
    done = 0;
    copied = 0;
    input = 0;
    while (done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input != 0)
            done = 1;
        if ((input > 0 && enabled[input - 1] == 0 && enabled != 0) || input == title_control)
            done = 0;
        if (done != 0)
            break;
        UI_Call_Redraw_Fn();
        if (ui_list_hit_enabled != 0) {
            hit = UI_Hit_Test();
            if (hit > 0)
                *selection = p_ui_controls[hit].param1;
        }
        Palette_Upload();
        UI_Present_Frame();
        if (copied == 0) {
            Screen_Copy_Front();
            copied = 1;
        }
        Wait_Ticks(ui_frame_ticks);
    }
    UI_Clear_Controls();
    ui_list_dialog_active = 0;
    ui_list_hit_enabled = 1;
    Mouse_Take_Click();
    Mouse_Take_Event();
    if (input < 0)
        return -1;
    return input - 1;
}

/* DEAD: list dialogs ignore mouse hits */
void UI_List_Hit_Disable(void)
{
    ui_list_hit_enabled = 0;
}

/* (direction) list dialogs: Up / Down to the previous / next enabled list line (1 = next), wrapping around - or,
   when the list can scroll, asking the dialog to scroll (ui_list_scroll_req); moves the cursor there and selects
   the line -> the control */
int UI_List_Step_Selection(int direction)
{
    int j;
    int control;
    int start;

    start = UI_Hit_Test();
    control = start;
    if (start != 0) {
        if (direction == 1) {
            for (control++; ui_control_count - 1 > control && p_ui_controls[control].type == UI_TEXT_BUTTON; control++) {
                if (p_ui_controls[control].enabled != 0)
                    break;
            }
            if (ui_control_count - 1 <= control || p_ui_controls[control].type != UI_TEXT_BUTTON) {
                if (ui_list_dialog_scrolling == 0) {
                    for (control = 1; p_ui_controls[control].enabled == 0 && ui_control_count - 1 > control; control++)
                        ;
                    if (ui_control_count - 1 <= control)
                        control = start;
                } else {
                    control = start;
                    ui_list_scroll_req = 1;
                }
            }
        } else {
            if (ui_list_dialog_scrolling != 0 && control == 1) {
                ui_list_scroll_req = -1;
                control = 1;
            } else {
                if (control > 1)
                    control--;
                else
                    control = ui_control_count - 2;
                while (p_ui_controls[control].type != UI_TEXT_BUTTON && control > 0)
                    control--;
                while (control > 0) {
                    if (p_ui_controls[control].enabled != 0)
                        break;
                    control--;
                }
                if (control <= 0) {
                    if (ui_list_dialog_scrolling != 0) {
                        ui_list_scroll_req = -1;
                        control = 1;
                    } else {
                        control = ui_control_count - 2;
                        while (p_ui_controls[control].type != UI_TEXT_BUTTON && control > 0)
                            control--;
                        while (p_ui_controls[control].enabled == 0 && control > 0)
                            control--;
                        if (control == 0)
                            control = start;
                    }
                }
            }
        }
    } else {
        if (*(int *)p_ui_controls[1].param2 >= 0) {
            start = *(int *)p_ui_controls[1].param2 + 1;
            if (start < 0 || ui_control_count - 1 < start)
                start = 0;
            control = start;
            if (direction == 1) {
                for (control++; ui_control_count - 1 > control && p_ui_controls[control].type == UI_TEXT_BUTTON; control++) {
                    if (p_ui_controls[control].enabled != 0)
                        break;
                }
                if ((control >= ui_control_count || p_ui_controls[control].type != UI_TEXT_BUTTON) && ui_list_dialog_scrolling != 0) {
                    ui_list_scroll_req = 1;
                } else if (ui_control_count - 1 <= control) {
                    for (control = 1; p_ui_controls[control].enabled == 0 && ui_control_count - 1 > control; control++)
                        ;
                    if (ui_control_count - 1 <= control)
                        control = start;
                }
            } else {
                if (control == 1 && ui_list_dialog_scrolling != 0) {
                    ui_list_scroll_req = -1;
                } else {
                    if (control > 1)
                        control--;
                    else
                        control = ui_control_count - 2;
                    while (control > 0) {
                        if (p_ui_controls[control].enabled != 0)
                            break;
                        control--;
                    }
                    if (control == 0) {
                        control = ui_control_count - 2;
                        while (p_ui_controls[control].enabled == 0 && control > 0)
                            control--;
                        if (control == 0)
                            control = start;
                    }
                }
            }
        } else {
            for (j = 1; j < ui_control_count; j++) {
                if (p_ui_controls[j].enabled != 0) {
                    control = j;
                    break;
                }
            }
            if (j >= ui_control_count)
                control = 0;
        }
    }
    if (control < 0 || ui_control_count - 1 < control)
        control = 0;
    if (control > 0) {
        ui_cursor_x = p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2;
        ui_cursor_y = p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2;
        if (Mouse_X() != ui_cursor_x || Mouse_Y() != ui_cursor_y) {
            Mouse_Hide();
            Mouse_Find_Region(ui_cursor_x, ui_cursor_y);
            mouse_pointer_offset = Mouse_Get_Pointer_Offset();
            ui_cursor_x = ui_cursor_x - mouse_pointer_offset;
            ui_cursor_y = ui_cursor_y - mouse_pointer_offset;
            Mouse_Restore_Under();
            Mouse_Save_Under(ui_cursor_x, ui_cursor_y);
            Mouse_Draw_Pointer(ui_cursor_x, ui_cursor_y);
            Mouse_Set_Position(ui_cursor_x, ui_cursor_y);
            *(int *)p_ui_controls[control].param2 = p_ui_controls[control].param1;
            Mouse_Show();
        }
    }
    return control;
}

/* (n) n controls (+ the reserved control 0), no highlight.
   Simtex name Clear_Fields_Above (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Set_Control_Count(int n)
{
    ui_control_count = n + 1;
    ui_highlight_control = -1;
}

/* () -> the number of controls on the screen.
   Simtex name Get_Field_Counter (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Get_Control_Count(void)
{
    return ui_control_count;
}

/* (mode) allocates the control table; 0 / 2 keyboard cursor, 1 mouse when a driver answers.
   Simtex name Init_Mouse_Keyboard (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Init(int mode)
{
    p_ui_controls = Seg_To_Far_Ptr(Heap_Alloc_Block(0x165));   /* paragraphs: UI_MAX_CONTROLS (150) x 0x26 bytes */
    switch (mode) {
    case 0:
        Mouse_Enable();
        ui_mouse_present = 0;
        ui_cursor_x = UI_CURSOR_HOME_X;
        ui_cursor_y = UI_CURSOR_HOME_Y;
        Mouse_Set_Position(UI_CURSOR_HOME_X, UI_CURSOR_HOME_Y);
        break;
    case 1:
        Mouse_Set_Cursor_Regions(1, &ui_init_mouse_region);
        ui_mouse_present = Mouse_Init();
        if (ui_mouse_present == 0) {
            Mouse_Enable();
            ui_cursor_x = UI_CURSOR_HOME_X;
            ui_cursor_y = UI_CURSOR_HOME_Y;
            Mouse_Set_Position(UI_CURSOR_HOME_X, UI_CURSOR_HOME_Y);
        } else {
            ui_mouse_present = 1;
        }
        break;
    case 2:
        Mouse_Enable();
        ui_mouse_present = 0;
        ui_cursor_x = UI_CURSOR_HOME_X;
        ui_cursor_y = UI_CURSOR_HOME_Y;
        Mouse_Set_Position(UI_CURSOR_HOME_X, UI_CURSOR_HOME_Y);
        break;
    default:
        break;
    }
    Mouse_Cursor_Setup(UI_CURSOR_HOME_X, UI_CURSOR_HOME_Y);
    ui_input_delay = 0;
    ui_highlight_control = -1;
    ui_right_click_ids = 0;
    UI_Clear_Controls();
}

/* () -> the activated control (0 none; -1 / -(id) right-click): skips the polls of a pending input delay.
   Simtex name Get_Input (MoO2 symbols, module fields); clarified using ReMoM repo */
int UI_Get_Input(void)
{
    int result;

    if (ui_input_delay > 0) {
        ui_input_delay--;
        return 0;
    } else if (ui_input_delay < 0) {
        ui_input_delay = 0;
    }
    if (ui_control_count <= 1)
        return 0;
    if (ui_mouse_present != 0)
        result = UI_Poll_Input();
    else
        result = UI_Poll_Input_Keyboard();
    Set_Draw_Page();
    return result;
}

/* (n) the next n UI_Get_Input calls return 0; drops a pending click.
   Simtex name Set_Input_Delay (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Input_Delay(int delay)
{
    ui_input_delay = delay;
    Mouse_Take_Click();
    Mouse_Take_Event();
}

/* DEAD */
int UI_Get_Input_Delay(void)
{
    return ui_input_delay;
}

/* DEAD: (control) a list line's enabled flag = 0 */
void UI_Text_Button_Clear_State(int control)
{
    if (p_ui_controls[control].type == UI_TEXT_BUTTON)
        p_ui_controls[control].enabled = 0;
}

/* DEAD: (control) a list line's enabled flag = 1 */
void UI_Text_Button_Set_State(int control)
{
    if (p_ui_controls[control].type == UI_TEXT_BUTTON)
        p_ui_controls[control].enabled = 1;
}

/* () draws every control in its current state (the highlighted one pressed, via UI_Draw_Control); the two debug
   switches outline every control box / help rectangle */
void UI_Draw_Controls(void)
{
    int half;                                   /* half the font height - but the whole height in the UI_TEXT_BUTTON cases */
    int gap;
    int value;
    int value_seg;
    int value_off;
    int changed;
    int saved_x1;
    int saved_x2;
    int saved_y1;
    int saved_y2;
    register int i;
    register int j;

    saved_x1 = clip_x1;
    saved_x2 = clip_x2;
    saved_y1 = clip_y1;
    saved_y2 = clip_y2;
    Reset_Clip_Rect();
    Set_Draw_Page();
    for (i = 1; i < ui_control_count; i++) {
        if (ui_highlight_control == i && p_ui_controls[i].type != UI_TEXT_INPUT && p_ui_controls[i].type != UI_GRID) {
            UI_Draw_Control(i, 1);
        } else {
            switch (p_ui_controls[i].type) {
            case UI_BUTTON:
                Pic_Reset_Frame(p_ui_controls[i].param5);
                Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5, 0);
                Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                half = (Font_Get_Height() - 1) / 2;
                Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2,
                                    p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 - half,
                                    (char *)p_ui_controls[i].param0);
                break;
            case UI_TOGGLE:
                if (*(int *)p_ui_controls[i].param2 == 0) {
                    Pic_Reset_Frame(p_ui_controls[i].param5);
                    Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5, 0);
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                    half = (Font_Get_Height() - 1) / 2;
                    Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2,
                                        p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 - half,
                                        (char *)p_ui_controls[i].param0);
                } else {
                    if (p_ui_controls[i].param3 == 0) {
                        Pic_Reset_Frame(p_ui_controls[i].param5);
                        Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5, 0);
                    } else {
                        Set_Pic_Frame(p_ui_controls[i].param5, 1);
                    }
                    Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5);
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                    half = (Font_Get_Height() - 1) / 2;
                    Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 + ui_press_dx,
                                        p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + ui_press_dy - half,
                                        (char *)p_ui_controls[i].param0);
                }
                break;
            case UI_LATCH:
                if (*(int *)p_ui_controls[i].param2 == 0) {
                    Pic_Reset_Frame(p_ui_controls[i].param5);
                    Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5, 0);
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                    half = (Font_Get_Height() - 1) / 2;
                    Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2,
                                        p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 - half,
                                        (char *)p_ui_controls[i].param0);
                } else {
                    if (p_ui_controls[i].param3 == 0) {
                        Pic_Reset_Frame(p_ui_controls[i].param5);
                        Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5, 0);
                    } else {
                        Set_Pic_Frame(p_ui_controls[i].param5, 1);
                    }
                    Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5);
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                    half = (Font_Get_Height() - 1) / 2;
                    Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 + ui_press_dx,
                                        p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + ui_press_dy - half,
                                        (char *)p_ui_controls[i].param0);
                }
                break;
            case UI_PIC_VALUE:
                if (*(int *)p_ui_controls[i].param2 != p_ui_controls[i].param1) {
                    Pic_Reset_Frame(p_ui_controls[i].param5);
                    Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5);
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                    half = (Font_Get_Height() - 1) / 2;
                    Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2,
                                        p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 - half,
                                        (char *)p_ui_controls[i].param0);
                } else {
                    if (p_ui_controls[i].param3 == 0) {
                        Pic_Reset_Frame(p_ui_controls[i].param5);
                        Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5, 0);
                    } else {
                        Set_Pic_Frame(p_ui_controls[i].param5, 1);
                    }
                    Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param5);
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color);
                    half = (Font_Get_Height() - 1) / 2;
                    Print_Text_Centered(p_ui_controls[i].x1 + (p_ui_controls[i].x2 - p_ui_controls[i].x1) / 2 + ui_press_dx,
                                        p_ui_controls[i].y1 + (p_ui_controls[i].y2 - p_ui_controls[i].y1) / 2 + ui_press_dy - half,
                                        (char *)p_ui_controls[i].param0);
                }
                break;
            case UI_TEXT_BUTTON:
                Font_Select(p_ui_controls[i].font, p_ui_controls[i].color, ui_button_color2);
                gap = Get_Text_Spacing() / 2;
                if (gap == 0)
                    gap = 1;
                half = Font_Get_Height();   /* here: the whole font height (the fill box) */
                if (*(int *)p_ui_controls[i].param2 != p_ui_controls[i].param1) {
                    if (p_ui_controls[i].enabled != 0) {
                        Print_Text(p_ui_controls[i].x1, p_ui_controls[i].y1 + 1, (char *)p_ui_controls[i].param0);
                    } else {
                        Font_Select(p_ui_controls[i].font, p_ui_controls[i].color2, ui_button_color2);
                        Print_Text(p_ui_controls[i].x1, p_ui_controls[i].y1 + 1, (char *)p_ui_controls[i].param0);
                    }
                } else {
                    Fill_Box_Styled(p_ui_controls[i].x1 - 1, p_ui_controls[i].y1 - gap + 1, p_ui_controls[i].x2,
                                  p_ui_controls[i].y1 + half + 1, p_ui_controls[i].fill_mode, p_ui_controls[i].param3,
                                  p_ui_controls[i].param4, p_ui_controls[i].param5, p_ui_controls[i].param6);
                    Print_Text(p_ui_controls[i].x1, p_ui_controls[i].y1 + 1, (char *)p_ui_controls[i].param0);
                }
                break;
            case UI_TEXT_INPUT:
                if (ui_highlight_control != i) {
                    Font_Select(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                    if (p_ui_controls[i].param2 != 0)
                        Fill_Rect(p_ui_controls[i].x1 - 1, p_ui_controls[i].y1, p_ui_controls[i].x2, p_ui_controls[i].y2,
                                  p_ui_controls[i].param2);
                    if (p_ui_controls[i].param3 == 0)
                        Print_Text(p_ui_controls[i].x1, p_ui_controls[i].y1, (char *)p_ui_controls[i].param0);
                    else
                        Print_Text_Right(p_ui_controls[i].x2, p_ui_controls[i].y1, (char *)p_ui_controls[i].param0);
                }
                break;
            case UI_SLIDER:
                value_seg = p_ui_controls[i].param2;
                value_off = p_ui_controls[i].param6;
                asm push es
                asm mov ax, value_seg
                asm mov bx, value_off
                asm mov es, ax
                asm mov ax, es:[bx]
                asm mov value, ax
                asm pop es
                if (p_ui_controls[i].param0 > value)
                    value = p_ui_controls[i].param0;
                if (p_ui_controls[i].param1 < value)
                    value = p_ui_controls[i].param1;
                asm push es
                asm mov ax, value
                asm mov cx, value_seg
                asm mov bx, value_off
                asm mov es, cx
                asm mov es:[bx], ax
                asm pop es
                break;
            case UI_TEXT_LINE:
                if (ui_edit_control != i) {
                    switch (p_ui_controls[i].param6) {
                    case 0:
                        Font_Select(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    case 1:
                        Font_Select_Shadow_UL(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    case -1:
                        Font_Select_Shadow(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    default:
                        Font_Select(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    }
                    if (p_ui_controls[i].param2 != 0)
                        Fill_Rect(p_ui_controls[i].x1 - 1, p_ui_controls[i].y1, p_ui_controls[i].x2, p_ui_controls[i].y2,
                                  p_ui_controls[i].param2);
                    Print_Text(p_ui_controls[i].x1 + UI_TEXT_LINE_INDENT, p_ui_controls[i].y1, (char *)p_ui_controls[i].param0);
                } else {
                    switch (p_ui_controls[i].param6) {
                    case 0:
                        Font_Select(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    case 1:
                        Font_Select_Shadow_UL(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    case -1:
                        Font_Select_Shadow(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    default:
                        Font_Select(p_ui_controls[i].font, p_ui_controls[i].color, p_ui_controls[i].color2);
                        break;
                    }
                    Text_Toggle_Highlight();
                    if (p_ui_controls[i].param2 != 0)
                        Fill_Rect(p_ui_controls[i].x1 - 1, p_ui_controls[i].y1, p_ui_controls[i].x2, p_ui_controls[i].y2,
                                  p_ui_controls[i].param2);
                    changed = 0;
                    if (ui_edit_blink / 2 != ui_edit_caret_visible)
                        changed = 1;
                    ui_edit_caret_visible = ui_edit_blink / 2;
                    ui_edit_blink++;
                    if (ui_edit_blink > 3)
                        ui_edit_blink = 0;
                    if (changed != 0) {
                        if (ui_edit_caret_visible == 0) {
                            for (j = 0; ui_edit_buf[j] != '_' && p_ui_controls[i].param5 > j; j++)
                                ;
                            ui_edit_buf[j] = 0;
                        } else {
                            for (j = 0; ui_edit_buf[j] != 0 && p_ui_controls[i].param5 - 1 > j; j++)
                                ;
                            if (p_ui_controls[i].param5 - 1 > j)
                                strcat(ui_edit_buf, "_");
                        }
                    }
                    if (p_ui_controls[i].param1 != 0)
                        Draw_Pic_Frame(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].param1);
                    Print_Text(p_ui_controls[i].x1 + UI_TEXT_LINE_INDENT, p_ui_controls[i].y1, ui_edit_buf);
                    Text_Color_Normal();
                }
                break;
            default:
                break;
            }
            if (ui_show_control_boxes != 0 && p_ui_controls[i].type != UI_ALT_HOTKEY) {
                Draw_Line_Clipped(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].x2, p_ui_controls[i].y1, UI_DEBUG_BOX_COLOR);
                Draw_Line_Clipped(p_ui_controls[i].x1, p_ui_controls[i].y1, p_ui_controls[i].x1, p_ui_controls[i].y2, UI_DEBUG_BOX_COLOR);
                Draw_Line_Clipped(p_ui_controls[i].x1, p_ui_controls[i].y2, p_ui_controls[i].x2, p_ui_controls[i].y2, UI_DEBUG_BOX_COLOR);
                Draw_Line_Clipped(p_ui_controls[i].x2, p_ui_controls[i].y1, p_ui_controls[i].x2, p_ui_controls[i].y2, UI_DEBUG_BOX_COLOR);
            }
        }
    }
    if (ui_show_help_boxes != 0 && ui_help_count != 0) {
        for (i = 0; i < ui_help_count; i++) {
            j = i * 5;
            if (p_active_help[j] != -1) {
                Draw_Line_Clipped(p_active_help[j + 1], p_active_help[j + 2], p_active_help[j + 3], p_active_help[j + 2], UI_DEBUG_HELP_COLOR);
                Draw_Line_Clipped(p_active_help[j + 1], p_active_help[j + 2], p_active_help[j + 1], p_active_help[j + 4], UI_DEBUG_HELP_COLOR);
                Draw_Line_Clipped(p_active_help[j + 1], p_active_help[j + 4], p_active_help[j + 3], p_active_help[j + 4], UI_DEBUG_HELP_COLOR);
                Draw_Line_Clipped(p_active_help[j + 3], p_active_help[j + 2], p_active_help[j + 3], p_active_help[j + 4], UI_DEBUG_HELP_COLOR);
            }
        }
    }
    Set_Clip_Rect(saved_x1, saved_y1, saved_x2, saved_y2);
}

/* (control, pressed) draws one control normal or pressed; pressing also does the control's action: a value / list
   line control stores its value, a grid the cell under the mouse, a slider follows the mouse, a text field runs its
   key loop (pressed or not); drawn unpressed, a text button / pic-value control RESETS its *current (0 / -1000).
   Simtex name Draw_Field (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Draw_Control(int control, int pressed)
{
    int half;
    int gap;
    int c;

    switch (p_ui_controls[control].type) {
    case UI_BUTTON:
        if (pressed == 0) {
            Pic_Reset_Frame(p_ui_controls[control].param5);
            Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            Font_Select(p_ui_controls[control].font, p_ui_controls[control].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2,
                                p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2 - half,
                                (char *)p_ui_controls[control].param0);
        } else {
            if (p_ui_controls[control].param3 == 0) {
                Pic_Reset_Frame(p_ui_controls[control].param5);
                Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            } else {
                Set_Pic_Frame(p_ui_controls[control].param5, 1);
            }
            Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            Font_Select(p_ui_controls[control].font, p_ui_controls[control].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2 + ui_press_dx,
                                p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2 + ui_press_dy - half,
                                (char *)p_ui_controls[control].param0);
        }
        break;
    case UI_TOGGLE:
        if (pressed == 0 || *(int *)p_ui_controls[control].param2 == 1) {
            Pic_Reset_Frame(p_ui_controls[control].param5);
            Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            Font_Select(p_ui_controls[control].font, p_ui_controls[control].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2,
                                p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2 - half,
                                (char *)p_ui_controls[control].param0);
        } else {
            if (p_ui_controls[control].param3 == 0) {
                Pic_Reset_Frame(p_ui_controls[control].param5);
                Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            } else {
                Set_Pic_Frame(p_ui_controls[control].param5, 1);
            }
            Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            Font_Select(p_ui_controls[control].font, p_ui_controls[control].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2 + ui_press_dx,
                                p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2 + ui_press_dy - half,
                                (char *)p_ui_controls[control].param0);
        }
        break;
    case UI_LATCH:
        if (*(int *)p_ui_controls[control].param2 == 0 && pressed != 1) {
            Pic_Reset_Frame(p_ui_controls[control].param5);
            Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            Font_Select(p_ui_controls[control].font, p_ui_controls[control].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2,
                                p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2 - half,
                                (char *)p_ui_controls[control].param0);
        } else {
            if (p_ui_controls[control].param3 == 0) {
                Pic_Reset_Frame(p_ui_controls[control].param5);
                Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            } else {
                Set_Pic_Frame(p_ui_controls[control].param5, 1);
            }
            Draw_Pic_Frame(p_ui_controls[control].x1, p_ui_controls[control].y1, p_ui_controls[control].param5, 0);
            Font_Select(p_ui_controls[control].font, p_ui_controls[control].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[control].x1 + (p_ui_controls[control].x2 - p_ui_controls[control].x1) / 2 + ui_press_dx,
                                p_ui_controls[control].y1 + (p_ui_controls[control].y2 - p_ui_controls[control].y1) / 2 + ui_press_dy - half,
                                (char *)p_ui_controls[control].param0);
        }
        break;
    case UI_PIC_VALUE:
        if (pressed == 0)
            *(int *)p_ui_controls[control].param2 = UI_NO_CONTROL;   /* not pressed */
        else
            *(int *)p_ui_controls[control].param2 = p_ui_controls[control].param1;
        c = control;
        if (*(int *)p_ui_controls[c].param2 != p_ui_controls[c].param1) {
            Pic_Reset_Frame(p_ui_controls[c].param5);
            Draw_Pic_Frame(p_ui_controls[c].x1, p_ui_controls[c].y1, p_ui_controls[c].param5);
            Font_Select(p_ui_controls[c].font, p_ui_controls[c].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[c].x1 + (p_ui_controls[c].x2 - p_ui_controls[c].x1) / 2,
                                p_ui_controls[c].y1 + (p_ui_controls[c].y2 - p_ui_controls[c].y1) / 2 - half,
                                (char *)p_ui_controls[c].param0);
        } else {
            if (p_ui_controls[c].param3 == 0) {
                Pic_Reset_Frame(p_ui_controls[c].param5);
                Draw_Pic_Frame(p_ui_controls[c].x1, p_ui_controls[c].y1, p_ui_controls[c].param5, 0);
            } else {
                Set_Pic_Frame(p_ui_controls[c].param5, 1);
            }
            Draw_Pic_Frame(p_ui_controls[c].x1, p_ui_controls[c].y1, p_ui_controls[c].param5);
            Font_Select(p_ui_controls[c].font, p_ui_controls[c].color);
            half = (Font_Get_Height() - 1) / 2;
            Print_Text_Centered(p_ui_controls[c].x1 + (p_ui_controls[c].x2 - p_ui_controls[c].x1) / 2 + ui_press_dx,
                                p_ui_controls[c].y1 + (p_ui_controls[c].y2 - p_ui_controls[c].y1) / 2 + ui_press_dy - half,
                                (char *)p_ui_controls[c].param0);
        }
        break;
    case UI_TEXT_BUTTON:
        if (pressed == 0)
            *(int *)p_ui_controls[control].param2 = 0;
        else if (p_ui_controls[control].enabled != 0)
            *(int *)p_ui_controls[control].param2 = p_ui_controls[control].param1;
        c = control;
        Font_Select(p_ui_controls[c].font, p_ui_controls[c].color, ui_button_color2);
        gap = Get_Text_Spacing() / 2;
        if (gap == 0)
            gap = 1;
        half = Font_Get_Height();   /* here: the whole font height (the fill box) */
        if (*(int *)p_ui_controls[c].param2 != p_ui_controls[c].param1) {
            if (p_ui_controls[c].enabled != 0) {
                Print_Text(p_ui_controls[c].x1, p_ui_controls[c].y1 + 1, (char *)p_ui_controls[c].param0);
            } else {
                Font_Select(p_ui_controls[c].font, p_ui_controls[c].color2, ui_button_color2);
                Print_Text(p_ui_controls[c].x1, p_ui_controls[c].y1 + 1, (char *)p_ui_controls[c].param0);
                Text_Color_Normal();
            }
        } else {
            Fill_Box_Styled(p_ui_controls[c].x1 - 1, p_ui_controls[c].y1 - gap + 1, p_ui_controls[c].x2,
                          p_ui_controls[c].y1 + half + 1, p_ui_controls[c].fill_mode, p_ui_controls[c].param3,
                          p_ui_controls[c].param4, p_ui_controls[c].param5, p_ui_controls[c].param6);
            Print_Text(p_ui_controls[c].x1, p_ui_controls[c].y1 + 1, (char *)p_ui_controls[c].param0);
        }
        break;
    case UI_GRID:
        if (pressed == 1) {
            *(int *)p_ui_controls[control].param3 = (Mouse_X() - p_ui_controls[control].x1) / p_ui_controls[control].param1;
            *(int *)p_ui_controls[control].param4 = (Mouse_Y() - p_ui_controls[control].y1) / p_ui_controls[control].param2;
            if ((p_ui_controls[control].x2 - p_ui_controls[control].x1) / p_ui_controls[control].param1 <
                    *(int *)p_ui_controls[control].param3)
                *(int *)p_ui_controls[control].param3 =
                    (p_ui_controls[control].x2 - p_ui_controls[control].x1) / p_ui_controls[control].param1;
            if ((p_ui_controls[control].y2 - p_ui_controls[control].y1) / p_ui_controls[control].param2 <
                    *(int *)p_ui_controls[control].param4)
                *(int *)p_ui_controls[control].param4 =
                    (p_ui_controls[control].y2 - p_ui_controls[control].y1) / p_ui_controls[control].param2;
            if (*(int *)p_ui_controls[control].param3 < 0)
                *(int *)p_ui_controls[control].param3 = 0;
            if (*(int *)p_ui_controls[control].param4 < 0)
                *(int *)p_ui_controls[control].param4 = 0;
        }
        break;
    case UI_SLIDER:
        if (pressed == 1)
            Slider_Set_From_Mouse(control);
        break;
    case UI_VALUE_REGION:
        if (pressed == 1)
            *(int *)p_ui_controls[control].param2 = p_ui_controls[control].param1;
        break;
    case UI_TEXT_INPUT:
        UI_Text_Edit_Run(control);
        break;
    default:
        break;
    }
}

/* (control, x, y) shows the control pressed (drawn highlighted) under the pointer at (x, y).
   Simtex name Push_Field_Down (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Draw_Pressed_Control(int control, int x, int y)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        if (ui_mouse_present != 0) {
            Mouse_Hide();
            if (control != ui_highlight_control) {
                if (ui_highlight_control == -1) {
                    Mouse_Restore_Under();
                    ui_highlight_control = control;
                    UI_Draw_Control(control, 1);
                    if (p_ui_controls[control].type == UI_TEXT_INPUT) {
                        x = Mouse_X();
                        y = Mouse_Y();
                    }
                    Mouse_Save_Under(x, y);
                    Mouse_Draw_Pointer(x, y);
                    Mouse_Set_Position(x, y);
                } else if (ui_highlight_control != control) {
                    Mouse_Restore_Under();
                    if (p_ui_controls[ui_highlight_control].type != UI_PIC_VALUE || p_ui_controls[control].type == UI_PIC_VALUE) {
                        if (p_ui_controls[ui_highlight_control].type == UI_TEXT_BUTTON) {
                            if (p_ui_controls[control].type == UI_TEXT_BUTTON && p_ui_controls[control].enabled != 0)
                                UI_Draw_Control(ui_highlight_control, 0);
                        } else {
                            UI_Draw_Control(ui_highlight_control, 0);
                        }
                    }
                    ui_highlight_control = control;
                    UI_Draw_Control(control, 1);
                    if (p_ui_controls[control].type == UI_TEXT_INPUT) {
                        x = Mouse_X();
                        y = Mouse_Y();
                    }
                    Mouse_Save_Under(x, y);
                    Mouse_Draw_Pointer(x, y);
                    Mouse_Set_Position(x, y);
                }
            }
            Mouse_Show();
        } else {
            Mouse_Restore_Under();
            Set_Draw_Page_Front();
            ui_highlight_control = control;
            UI_Draw_Control(control, 1);
            ui_highlight_control = -1;
            Set_Draw_Page();
            Mouse_Save_Under(x, y);
            Mouse_Draw_Pointer(x, y);
            Mouse_Set_Position(x, y);
        }
    }
}

/* DEAD: (value) sets the list dialogs' title-button help (ui_list_title_help) */
void UI_Set_List_Title_Help(int value)
{
    ui_list_title_help = value;
}

/* DEAD */
void UI_Clear_List_Title_Help(void)
{
    ui_list_title_help = -1;
}

/* (fn, ticks) the screen's redraw callback, run while the input loops wait; ticks per frame 1..9 (else 2).
   Simtex name Assign_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo
   Numbered: BUG #372 (BUGS_AND_QUIRKS.txt) */
void UI_Set_Redraw_Fn(RedrawFn fn, int ticks)
{
    redraw_fn = fn;
    redraw_fn_active = 1;
    if (ticks > 0 && ticks < 10)
        ui_frame_ticks = ticks;
    else
        ui_frame_ticks = 2;
}

/* () keeps the callback for UI_Restore_Redraw_Fn.
   Simtex name Save_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Save_Redraw_Fn(void)
{
    saved_redraw_fn = redraw_fn;
}

/* () the redraw callback saved by UI_Save_Redraw_Fn, enabled.
   Simtex name Restore_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Restore_Redraw_Fn(void)
{
    redraw_fn = saved_redraw_fn;
    redraw_fn_active = 1;
}

/* () the frame loop stops calling the redraw callback.
   Simtex name Deactivate_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Suspend_Redraw_Fn(void)
{
    redraw_fn_active = 0;
}

/* () it calls it again */
void UI_Enable_Redraw_Fn(void)
{
    redraw_fn_active = 1;
}

/* () calls the redraw callback when enabled.
   Simtex name Invoke_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Call_Redraw_Fn(void)
{
    if (redraw_fn_active != 0)
        (*redraw_fn)();
}

/* () one frame of 1 tick: the callback, palette, controls and pointer.
   Simtex name Quick_Call_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Redraw_Frame_1_Tick(void)
{
    if (redraw_fn_active != 0) {
        Timer_Mark();
        Set_Draw_Page();
        UI_Call_Redraw_Fn();
        Palette_Upload();
        UI_Present_Frame();
        Wait_Ticks(1);
    } else {
        Palette_Upload();
        UI_Present_Frame();
    }
}

/* () one frame of ui_frame_ticks ticks.
   Simtex name Call_Auto_Function (MoO2 symbols, module fields); clarified using ReMoM repo */
void UI_Redraw_Frame_Wait(void)
{
    if (redraw_fn_active != 0) {
        Timer_Mark();
        Set_Draw_Page();
        UI_Call_Redraw_Fn();
        Palette_Upload();
        UI_Present_Frame();
        Wait_Ticks(ui_frame_ticks);
    } else {
        Palette_Upload();
        UI_Present_Frame();
    }
}

/* () no controls, skip the next poll */
void UI_Reset(void)
{
    UI_Clear_Controls();
    UI_Input_Delay(1);
}

/* () draws the controls and the pointer on the back page and flips: one full frame.
   Simtex name TOGGLE_PAGES (MoO2 symbols, module MOX2); clarified using ReMoM repo */
void UI_Present_Frame(void)
{
    int mx;
    int my;

    Mouse_Hide();
    mx = Mouse_X();
    my = Mouse_Y();
    UI_Draw_Controls();
    Mouse_Find_Region(mx, my);
    Mouse_Save_Under_Back(mx, my);
    Mouse_Draw_Pointer_Back(mx, my);
    Page_Flip();
    Mouse_Restore_Under_Back();
    Mouse_Copy_Save_Buffer();
    Mouse_Show();
}

/* @@NEXT@@ */
