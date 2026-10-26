/* STACKPOP.C - WIZARDS.EXE overlay code segment 6a2d (stub 2490): the stack popup - the list of the units on a map
   square (clicking an enemy or own stack on the overland map, and Spell_Target_Map_Select picking one unit of a
   stack): a window that zooms open from the square, titled with the owner's name ("Raiders" / "Rampaging Monsters"
   for the neutral player), one row per unit (icon, name, melee / ranged / defense / hits / moves with their icons
   and help areas).
   This module's data (DS:49e6): its literals ("" is the tail of "X": -d); BSS (DS:c2c0..c2d6).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6a2d src/wizards/STACKPOP.C */
#include <string.h>
#include "GAME.H"

#define STACK_POPUP_ROW_H 19            /* a unit row of the stack popup */

int stack_popup_x;                              /* DS:c2d4  popup left x (45) */
int stack_popup_y;                              /* DS:c2d2  popup top y */
int stack_popup_rows_dx;                        /* DS:c2d0  x offset of the unit rows (8) */
int stack_popup_rows_dy;                        /* DS:c2ce  y offset of the first unit row (24) */
int stack_popup_unit;                           /* DS:c2cc  the unit the popup was opened on */
int *p_stack_popup_units;                       /* DS:c2ca  unit per row */
int *p_stack_popup_controls;                    /* DS:c2c8  control id per row */
int stack_popup_count;                          /* DS:c2c6  units listed (max 9) */
int stack_popup_unused;                         /* DS:c2c4  never used (its place in this module's BSS) */
int stack_popup_owner;                          /* DS:c2c2  owner of the listed units */
int stack_popup_bottom;                         /* DS:c2c0  popup bottom y */

extern unsigned char banner_text_colors[];      /* DS:00c8  a text color per banner (ReMoM COL_Banners2; clarified using ReMoM repo) */
extern unsigned unitlist_bottom_pic;            /* DS:9754 */
extern unsigned unitlist_bg_pic;                /* DS:9756 */
extern unsigned unitview_stat_icons[];          /* DS:97da */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int stack_popup_window_ctl;              /* DS:c00e */
extern int stack_popup_outside_ctl;             /* DS:c010 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
int Screen_Stack_Popup(int unit, int select, int x, int y);
void Stack_Popup_Redraw(void);
void Stack_Popup_Add_Controls(int select);

void Load_Stack_Popup_Art(void);
void Pic_Apply_Palette(unsigned pic);
void Screen_Copy_Front(void);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
int Unit_Is_Invisible(int unit);
void Window_Zoom_Open_In_Place(int x1, int y1, int x2, int y2, int src_x1, int src_y1, int src_x2, int src_y2, int lines);
int Window_Zoom_Lines_Redraw(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
int UI_Add_Hotkey(char *key);
int UI_Hit_Test(void);
void Help_Set_Stack_Popup(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void UnitView_Melee_Icon(int weapon_icon, int *icon);
void UnitView_Ranged_Icon(int ranged_type, int *icon);
void UnitView_Move_Icon(int move_flags, int *icon);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Outline_Cycle_Tick(void);

/* (unit, select, x, y) -> the chosen unit or -1: the popup of the units on unit's square (same owner, same plane or
   in a Tower of Wizardry, at most 9; an enemy's invisible units left out), zooming open from (x, y); with select 1
   every row is a control. BUG #480: 'rebuild' is read before it is first set (the first frame may skip its redraw) */
int Screen_Stack_Popup(int unit, int select, int x, int y)
{
    int input;
    int hit;
    int rebuild;
    int picked;
    unsigned seg;
    int hotkey;
    int i;
    int done;

    Load_Stack_Popup_Art();
    Pic_Apply_Palette(unitlist_bg_pic);
    Palette_Upload();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Save_A800();
    picked = -1;
    stack_popup_unit = unit;
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    p_stack_popup_units = (int *)Near_Alloc(MAX_STACK * sizeof(int));
    p_stack_popup_controls = (int *)Near_Alloc(MAX_STACK * sizeof(int));
    stack_popup_owner = p_units[stack_popup_unit].owner;
    stack_popup_count = 0;
    for (i = 0; i < num_units && stack_popup_count < MAX_STACK; i++) {
        if (p_units[i].owner == p_units[stack_popup_unit].owner
            && (p_units[i].plane == p_units[stack_popup_unit].plane || p_units[i].in_tower == 1)
            && p_units[i].x == p_units[stack_popup_unit].x
            && p_units[i].y == p_units[stack_popup_unit].y) {
            if (stack_popup_owner != current_player && Unit_Is_Invisible(i) == 1)
                continue;
            p_stack_popup_units[stack_popup_count] = i;
            stack_popup_count++;
        }
    }
    stack_popup_x = 45;
    stack_popup_y = 0;
    stack_popup_rows_dx = 8;
    stack_popup_rows_dy = 24;
    stack_popup_bottom = stack_popup_rows_dy + stack_popup_count * STACK_POPUP_ROW_H;
    stack_popup_y = 97 - stack_popup_bottom / 2;   /* centred on y 97 */
    stack_popup_bottom = stack_popup_y + stack_popup_bottom;
    seg = Heap_Alloc_Front(p_scratch, 1);
    Set_Draw_Page();
    Stack_Popup_Redraw();
    Window_Zoom_Open_In_Place(stack_popup_x, stack_popup_y, stack_popup_x + 238, stack_popup_bottom + 5, x, y, x + 19,
                          y + 16, 1);
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Stack_Popup_Redraw, 1);
    UI_Input_Delay(1);
    UI_Clear_Help();
    Help_Set_Stack_Popup();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        Stack_Popup_Add_Controls(select);
        hotkey = UI_Add_Hotkey("X");
        input = UI_Get_Input();
        hit = UI_Hit_Test();
        if (input == stack_popup_outside_ctl)
            done = 1;
        for (i = 0; i < stack_popup_count; i++) {
            if (p_stack_popup_controls[i] == input) {
                picked = p_stack_popup_units[i];
                done = -1;
            }
        }
        if (done == 0 && rebuild == 0) {
            Screen_Restore_A800();
            Stack_Popup_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
        rebuild = 0;
    }
    present_mode = PRESENT_DISSOLVE;
    UI_Suspend_Redraw_Fn();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Clear_Help();
    Load_Overland_Screen_Graphics();
    return picked;
}

/* redraw callback of the stack popup: the window in the owner's banner text color, the title, one row per unit
   (icon, name, the five stats with their icons - a stat of 0 is left out - and their help areas), and the help
   area of the whole window */
void Stack_Popup_Redraw(void)
{
    int slot;
    int row_x;
    int row_y;
    int unit_type;
    char colors[6];
    unsigned pic;
    int col_x;
    int text_y;
    int stats[5];
    int saved_type;
    int i;
    int k;

    if (stack_popup_owner == PLAYER_NEUTRAL)
        wizards[stack_popup_owner].banner_color = BANNER_NEUTRAL;
    colors[0] = banner_text_colors[wizards[stack_popup_owner].banner_color];
    colors[1] = banner_text_colors[wizards[stack_popup_owner].banner_color];
    colors[2] = banner_text_colors[wizards[stack_popup_owner].banner_color];
    colors[3] = banner_text_colors[wizards[stack_popup_owner].banner_color];
    colors[4] = banner_text_colors[wizards[stack_popup_owner].banner_color];
    colors[5] = banner_text_colors[wizards[stack_popup_owner].banner_color];
    Font_Select_Palette(4, colors);
    Font_Select_Shadow(4, 15, 0, 0);
    Set_Text_Shadow_Color(0);
    for (i = 0; i < 50; i++)
        help_entries[i].id = -1;
    Set_Draw_Page();
    Window_Zoom_Lines_Redraw();
    Set_Clip_Rect(0, 0, 319, stack_popup_bottom);
    Draw_Pic_Clipped(stack_popup_x, stack_popup_y, unitlist_bg_pic);
    Reset_Clip_Rect();
    Draw_Pic_Frame(stack_popup_x, stack_popup_bottom, unitlist_bottom_pic);
    if (stack_popup_owner == PLAYER_NEUTRAL) {
        if (unit_types[p_units[p_stack_popup_units[0]].type].ability_flags & AFLAG_FANTASTIC)
            strcpy(p_text_buf, "Rampaging Monsters");
        else
            strcpy(p_text_buf, "Raiders");
    } else {
        strcpy(p_text_buf, wizards[stack_popup_owner].wizard_name);
        strcat(p_text_buf, " Units");
    }
    Font_Select_Outline(4, 15, 0, 0);
    Set_Text_Shadow_Color(252);
    Print_Text_Centered(stack_popup_x + 118, stack_popup_y + 7, p_text_buf);
    row_x = stack_popup_x + stack_popup_rows_dx;
    row_y = stack_popup_y + stack_popup_rows_dy;
    for (i = 0; i < stack_popup_count; i++) {
        unit_type = p_units[p_stack_popup_units[i]].type;
        Draw_Unit_Icon(row_x, row_y, p_stack_popup_units[i],
                       p_units[p_stack_popup_units[i]].owner == current_player ? UNIT_ICON_ACTIVE : UNIT_ICON_OTHER);
        slot = p_units[p_stack_popup_units[i]].hero_slot;
        if (slot != -1)
            strcpy(p_text_buf, wizards[p_units[p_stack_popup_units[i]].owner].hired_heroes[slot].name);
        else
            strcpy(p_text_buf, unit_types[unit_type].name);
        colors[0] = 197;
        colors[1] = 255;
        colors[2] = 80;
        colors[3] = 255;
        Font_Select_Palette(2, colors);
        Font_Select_Outline(2, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        Print_Text(row_x + 24, row_y + 5, p_text_buf);
        saved_type = p_units[p_stack_popup_units[i]].type;
        Battle_Unit_Init(p_stack_popup_units[i], p_unit_stats_buf);
        p_units[p_stack_popup_units[i]].type = saved_type;   /* Battle_Unit_Init does not write it: changes nothing */
        colors[0] = 250;
        colors[1] = 252;
        Font_Select_Palette(1, colors);
        Font_Select(1, 15, 0, 0);
        Set_Text_Shadow_Color(252);
        col_x = stack_popup_x + 140;
        text_y = row_y + 7;
        stats[0] = p_unit_stats_buf->melee;
        if (p_unit_stats_buf->ranged_type != RANGED_NONE
            && (p_unit_stats_buf->ranged_type / 10 == RANGED_CLASS_BOULDER || p_unit_stats_buf->ranged_type / 10 == RANGED_CLASS_MISSILE
                || p_unit_stats_buf->ranged_type / 10 == RANGED_CLASS_MAGIC))
            stats[1] = p_unit_stats_buf->ranged;
        else
            stats[1] = 0;
        stats[2] = p_unit_stats_buf->defense;
        stats[3] = p_unit_stats_buf->hits_per_figure;
        stats[4] = p_unit_stats_buf->moves / 2;
        for (k = 0; k < 5; k++) {
            if (stats[k] > 0) {
                Print_Number_Right(col_x, text_y, stats[k]);
                help_entries[i * 5 + k].x1 = col_x - 9;
                help_entries[i * 5 + k].y1 = text_y - 2;
                help_entries[i * 5 + k].x2 = col_x + 9;
                help_entries[i * 5 + k].y2 = text_y + 5;
                switch (k) {
                case 0:
                    help_entries[i * 5 + k].id = HELP_MELEE;
                    UnitView_Melee_Icon(p_unit_stats_buf->weapon_icon, &slot);
                    pic = unitview_stat_icons[slot];
                    break;
                case 1:
                    help_entries[i * 5 + k].id = HELP_RANGE;
                    UnitView_Ranged_Icon(p_unit_stats_buf->ranged_type, &slot);
                    pic = unitview_stat_icons[slot];
                    break;
                case 2:
                    help_entries[i * 5 + k].id = HELP_ARMOR;
                    pic = unitview_stat_icons[STAT_ICON_SHIELD];
                    break;
                case 3:
                    help_entries[i * 5 + k].id = HELP_HITS;
                    pic = unitview_stat_icons[STAT_ICON_HEART];
                    break;
                case 4:
                    help_entries[i * 5 + k].id = HELP_UNIT_MOVES;
                    UnitView_Move_Icon(p_unit_stats_buf->move_flags, &slot);
                    pic = unitview_stat_icons[slot];
                    break;
                }
                Draw_Pic_Frame(col_x + 1, text_y - 2, pic);
                Draw_Line(col_x + 1, text_y - 1, col_x + 1, text_y + 5, 197);
                Draw_Line(col_x + 1, text_y + 5, col_x + 9, text_y + 5, 246);
            }
            col_x += 20;
        }
        row_y += STACK_POPUP_ROW_H;
    }
    help_entries[stack_popup_count * 5].x1 = stack_popup_x;
    help_entries[stack_popup_count * 5].y1 = stack_popup_y;
    help_entries[stack_popup_count * 5].x2 = stack_popup_x + 237;
    help_entries[stack_popup_count * 5].y2 = stack_popup_bottom + 4;
    help_entries[stack_popup_count * 5].id = HELP_UNIT_LIST;
    Outline_Cycle_Tick();
}

/* (select): the popup's controls - with select 1 one per unit row and one for the window; always a full-screen
   control behind it (a click outside closes the popup) */
void Stack_Popup_Add_Controls(int select)
{
    int x;
    int y;
    int i;

    for (i = 0; i < MAX_STACK; i++)
        p_stack_popup_controls[i] = UI_NO_CONTROL;
    stack_popup_window_ctl = UI_NO_CONTROL;
    if (select == 1) {
        x = stack_popup_x + stack_popup_rows_dx;
        y = stack_popup_y + stack_popup_rows_dy;
        for (i = 0; i < stack_popup_count; i++) {
            p_stack_popup_controls[i] = UI_Add_Region(x, y, x + 222, y + 18, "", -1);
            y += STACK_POPUP_ROW_H;
        }
        stack_popup_window_ctl = UI_Add_Region(stack_popup_x, stack_popup_y, stack_popup_x + 237,
                                                stack_popup_bottom + 4, "", -1);
    }
    stack_popup_outside_ctl = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
}
