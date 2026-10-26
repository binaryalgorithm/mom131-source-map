/* UNITVIEW.C - WIZARDS.EXE overlay code segment 5e1f (stub 246c): the unit statistics window - opened from combat or
   the overland map (Screen_Unit_View: item rows, dispelling an own enchantment, dismissing the unit), drawn by
   Draw_Unit_View (also the city production preview of a unit type or a building: mode 0), with the stat icon rows,
   the upkeep row, the ability list and its paging / help controls.
   This module's data (DS:33c4..347d): its literals only. BSS DS:c176..c1e9: the unit view's state.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 5e1f src/wizards/UNITVIEW.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Combat_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit);
void Overland_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit);
void Screen_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit);
void UnitView_Redraw(void);
void Draw_Unit_View(int x, int y, int mode, int subject, struct UnitAbilityList *list, int count, unsigned pic);
void Draw_Unit_Upkeep(int x, int y, int unit);
int Draw_Stat_Icons(int x, int y, int icon, int value, int per_row, int bonus, int penalty);
int Draw_Hits_Icons(int x, int y, int icon, int hits, int damage, int per_row, int bonus);
void UnitView_Move_Icon(int move_flags, int *icon);
void Draw_Unit_Ability_List(int x, int y, struct UnitAbilityList *list, int count, int first, unsigned pic);
void UnitView_Add_Ability_Controls(int x, int y, struct UnitAbilityList *list, int count, int first);
void UnitView_Set_Ability_Help(struct UnitAbilityList *list, int count, int help_base);
void UnitView_Add_Scroll_Buttons(int x, int y);
void UnitView_Clear_Ability_Controls(void);
void UnitView_Scroll_Abilities(int input, int *rows_out);
void UnitView_Melee_Icon(int weapon_icon, int *icon);
void UnitView_Ranged_Icon(int ranged_type, int *icon);
void UnitView_Init_Hire(int unit, int count);
void UnitView_Init_List(int count);

#include "GAME.H"

/* the ability list: 8 rows a page (2 columns of 4), a hero's first page 4 (the portrait takes the rest) */
#define ABIL_ROWS_PER_PAGE 8
#define ABIL_HERO_FIRST_ROWS 4

int unit_view_x;                                /* DS:c1e8 */
int unit_view_y;                                /* DS:c1e6 */
int unit_view_mode;                             /* DS:c1e4  UNIT_VIEW_PRODUCTION / _OVERLAND / _COMBAT */
int unit_view_unit;                             /* DS:c1e2 */
unsigned p_unit_view_item_pic;                  /* DS:c1e0 */
int unit_view_row_icon_ctl[8];                  /* DS:c1d0 */
int unit_view_row_text_ctl[8];                  /* DS:c1c0 */
int unit_view_item_icon_ctl[3];                 /* DS:c1ba */
int unit_view_item_text_ctl[3];                 /* DS:c1b4 */
long unit_view_row_ench[8];                     /* DS:c194  the enchantment bit of each visible row */
struct UnitAbilityList *p_unit_view_list;       /* DS:c192 */
int unit_view_abil_count;                       /* DS:c190 */
int unit_view_more_pages;                       /* DS:c18e */
int unit_view_abil_first;                       /* DS:c18c */
int unit_view_ctl_window;                       /* DS:c18a */
int unit_view_ctl_panel;                        /* DS:c188 */
int unit_view_ctl_exit;                         /* DS:c186 */
int unit_view_btn_ok;                           /* DS:c184 */
int unit_view_btn_dismiss;                      /* DS:c182 */
int unit_view_btn_up;                           /* DS:c180 */
int unit_view_btn_down;                         /* DS:c17e */
unsigned unit_figure_src_pic;                   /* DS:c17c */
int unit_view_clear_bg;                         /* DS:c17a */
int unit_view_is_hero;                          /* DS:c178 */
int unit_view_from_combat;                      /* DS:c176 */

extern struct RaceInfo race_table[14];          /* DS:1d86 */
extern char *arcane_first_realm_names[6];                    /* DS:1ed8  the names of "races" 15..20 (the realms of summoned units) */
extern unsigned unitview_land_bg_pic;           /* DS:92b6  CITYSCAP 0x67 */
extern unsigned ok_button_pic;                  /* DS:9498 */
extern unsigned resource_x10_pics[];            /* DS:94a0 */
extern unsigned resource_x1_pics[];             /* DS:94e6 */
extern unsigned unitview_button_panel_pic;      /* DS:9540 */
extern unsigned unitview_water_bg_pic;          /* DS:979c */
extern unsigned unitview_portrait_frame_pic;    /* DS:979e */
extern unsigned gold_coin_pic;                  /* DS:97a0 */
extern unsigned unitview_row_frame_pics[];      /* DS:97a4 */
extern unsigned unitview_arrow_down_pic;        /* DS:97b4 */
extern unsigned unitview_arrow_up_pic;          /* DS:97b6 */
extern unsigned unitview_bg_pic;                /* DS:97b8 */
extern unsigned unitview_build_bg_pic;          /* DS:97ba */
extern unsigned unitview_stat_icons_gold[];     /* DS:97bc */
extern unsigned unitview_stat_icons[];          /* DS:97da */
extern unsigned special_icons[];                /* DS:97f8  SPECIAL.LBX */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern char *p_building_desc_text;              /* DS:bff2 */
extern unsigned unit_view_portrait_pic;         /* DS:c79e */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Item_Outline_Cycle_Tick(void);
void Building_Allows_List(int bldg, int *n, int *out);
void Building_Pic_Opaque_Bounds(int city, int bldg, int *x, int *y, int *w, int *h, unsigned bm);
int City_Production_Cost(int city);
int Confirm_Box(char *text);
void Draw_Bitmap(int x, int y, unsigned pic);
void Draw_Icon_Row_Fit(int *x, int y, int total, int amount, unsigned pic_ten, unsigned pic_one);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Draw_Unit_Figures(int x, int y, int type_or_unit, int is_unit);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Fill_Box_Styled();                           /* no prototype here: called with 5 arguments (the definition in GRAPHICS.C takes 9, read by style) */
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Help_Load_Unit_View(int view_mode);
void Item_Build_Icon(int item, unsigned pic);
void Item_View_Popup(int item, int x, int y);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Load_Combat_Graphics(void);
void Load_Unit_Figure(int unit_or_type, int is_type);
void Load_Unit_View_Graphics(void);
void Pic_Apply_Palette(unsigned pic);
void Pic_Set_Remap_Flag(unsigned pic);
void Play_Click_Sound(void);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void Screen_Copy_Front(void);
void Set_Font_Color(int color);
void Set_Line_Spacing(int line_gap);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Set_Space_Width(int width);
void Stack_Clear_Draw_Priority(void);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Text_List_Separator(int *count, int total, char *text);
int Text_Width(char *text);
int Tile_Is_Water(int x, int y, int plane);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Add_Hotkey(char *key);
int UI_Add_Picture(int x, int y, int pic, char *hotkey, int help);
int UI_Hit_Test(void);
void UI_Input_Delay(int delay);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UnitView_Alloc_Buffers(void);
void UnitView_Build_Abilities(int unit, struct UnitAbilityList *list, int *count);
int Unit_Dispel_Enchant_Confirm(int unit, long bits, char *text);
int Unit_Gold_Upkeep(int unit);
void Unit_Dismiss(int unit);
int Unit_Mana_Upkeep(int unit);
int Window_Zoom_Lines_Redraw(void);
int Window_Zoom_Open(int dst_x1, int dst_y1, int dst_x2, int dst_y2, int img_x1, int img_y1, int img_x2, int img_y2,
                    int src_x1, int src_y1, int src_x2, int src_y2, int lines);

/* (x, y, source rect, mode 2, unit): the unit view from combat - no zoom animation; the battlefield art is
   reloaded afterwards */
void Combat_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit)
{
    Load_Unit_View_Graphics();
    Heap_Mark(p_scratch);
    UnitView_Alloc_Buffers();
    if (p_units[unit].type > UNIT_HERO_CHOSEN)
        Load_Unit_Figure(unit, 0);
    else
        unit_view_portrait_pic = LBX_Reload_Next("PORTRAIT", unit_types[p_units[unit].type].hero_portrait, p_scratch);
    unit_view_from_combat = 1;
    Screen_Unit_View(x, y, src_x1, src_y1, src_x2, src_y2, mode, unit);
    Heap_Release(p_scratch);
    Load_Combat_Graphics();
}

/* (x, y, source rect, mode 1, unit): the overland unit view - zooms open from the clicked rectangle */
void Overland_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit)
{
    unsigned spare;

    Load_Unit_View_Graphics();
    spare = Heap_Alloc_Front(p_scratch, 1);
    Heap_Mark(p_scratch);
    UnitView_Alloc_Buffers();
    unit_view_from_combat = 0;
    if (p_units[unit].type > UNIT_HERO_CHOSEN)
        Load_Unit_Figure(unit, 0);
    else
        unit_view_portrait_pic = LBX_Reload_Next("PORTRAIT", unit_types[p_units[unit].type].hero_portrait, p_scratch);
    Screen_Unit_View(x, y, src_x1, src_y1, src_x2, src_y2, mode, unit);
    Heap_Release(p_scratch);
    Load_Overland_Screen_Graphics();
}

/* (x, y, source rect, mode 1 overland / 2 combat, unit): the unit view's input loop - Ok / Esc / a click outside
   leave; Dismiss (overland) asks and kills the unit; a right click on a hero item row (a negated control id) opens the
   item view; an enchantment row (not in
   combat) offers to dispel it; the X hotkey does nothing
   Numbered: BUG #82 (BUGS_AND_QUIRKS.txt) */
void Screen_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit)
{
    int i;
    int done;
    int died;
    int input;
    int hit;
    int can_dispel;
    int rows;
    int redraw;
    int item;
    int esc_key;
    int x_key;

    unit_view_x = x;
    unit_view_y = y;
    unit_view_mode = mode;
    unit_view_unit = unit;
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(UnitView_Redraw, 1);
    UI_Clear_Help();
    Help_Load_Unit_View(mode);
    unit_view_abil_count = 0;
    switch (mode) {
    case 1:
    case 2:
        UnitView_Build_Abilities(unit_view_unit, p_unit_view_list, &unit_view_abil_count);
        break;
    }
    unit_view_abil_first = 0;
    unit_view_more_pages = 0;
    unit_view_is_hero = 0;
    if (p_units[unit_view_unit].hero_slot != -1)
        unit_view_is_hero = 1;
    if (unit_view_is_hero == 1 && unit_view_abil_count > ABIL_HERO_FIRST_ROWS)
        unit_view_more_pages = 1;
    if (unit_view_abil_count > ABIL_ROWS_PER_PAGE)
        unit_view_more_pages = 1;
    can_dispel = 1;
    if (mode == UNIT_VIEW_COMBAT)
        can_dispel = 0;
    Pic_Apply_Palette(unitview_bg_pic);
    Palette_Upload();
    Palette_Mark_Range(244, 255);
    Remap_Build_All();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Save_A800();
    UI_Input_Delay(4);
    unit_view_clear_bg = 1;
    Set_Draw_Page();
    UnitView_Redraw();
    if (unit_view_from_combat == 0)
        Window_Zoom_Open(unit_view_x, unit_view_y, unit_view_x + 217, unit_view_y + 184, unit_view_x, unit_view_y,
                         unit_view_x + 282, unit_view_y + 184, src_x1, src_y1, src_x2, src_y2, 1);
    unit_view_clear_bg = 0;
    UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        UnitView_Add_Ability_Controls(unit_view_x + 8, unit_view_y + 108, p_unit_view_list, unit_view_abil_count,
                                      unit_view_abil_first);
        Font_Select(4, 4, 0, 0);
        Set_Font_Color(203);
        unit_view_btn_ok = UI_NO_CONTROL;
        if (mode != UNIT_VIEW_COMBAT)
            unit_view_btn_ok = UI_Add_Button(unit_view_x + 222, unit_view_y + 162, "Ok", ok_button_pic, "O", -1);
        unit_view_btn_dismiss = UI_NO_CONTROL;
        if (mode == UNIT_VIEW_OVERLAND)
            unit_view_btn_dismiss = UI_Add_Button(unit_view_x + 222, unit_view_y + 143, "Dismiss", ok_button_pic, "D",
                                                  -1);
        unit_view_ctl_window = UI_Add_Region(unit_view_x, unit_view_y, unit_view_x + 217, unit_view_y + 184, "", -1);
        unit_view_ctl_panel = UI_NO_CONTROL;
        if (mode != UNIT_VIEW_COMBAT)
            unit_view_ctl_panel = UI_Add_Region(unit_view_x + 213, unit_view_y + 133, unit_view_x + 280,
                                                 unit_view_y + 183, "", -1);
        unit_view_ctl_exit = UI_Add_Region(0, 0, 319, 199, "", -1);
        esc_key = UI_Add_Hotkey("\x1b");
        x_key = UI_Add_Hotkey("X");
        UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
        input = UI_Get_Input();
        hit = UI_Hit_Test();
        if (input == x_key) {
        }
        UnitView_Scroll_Abilities(input, &rows);
        for (i = 0; i < 3; i++) {
            if (-unit_view_item_icon_ctl[i] == input || -unit_view_item_text_ctl[i] == input) {   /* a right click */
                Play_Click_Sound();
                item = wizards[p_units[unit_view_unit].owner].hired_heroes[p_units[unit_view_unit].hero_slot].item_index[i];
                if (item != -1) {
                    UI_Clear_Help();
                    Item_View_Popup(item, unit_view_x + 16, unit_view_y + (i + 1) * 17 + 116);
                    UI_Suspend_Redraw_Fn();
                    UI_Set_Redraw_Fn(UnitView_Redraw, 1);
                    UI_Input_Delay(2);
                    redraw = 1;
                    Heap_Release(p_scratch);
                    Heap_Mark(p_scratch);
                    UnitView_Alloc_Buffers();
                    if (p_units[unit_view_unit].type > UNIT_HERO_CHOSEN)
                        Load_Unit_Figure(unit_view_unit, 0);
                    else
                        unit_view_portrait_pic = LBX_Reload_Next("PORTRAIT",
                            unit_types[p_units[unit_view_unit].type].hero_portrait, p_scratch);
                    UnitView_Build_Abilities(unit_view_unit, p_unit_view_list, &unit_view_abil_count);
                    if (unit_view_abil_first >= unit_view_abil_count)
                        unit_view_abil_first -= ABIL_ROWS_PER_PAGE;
                    unit_view_more_pages = 0;
                    if (unit_view_is_hero == 1 && unit_view_abil_count > ABIL_HERO_FIRST_ROWS)
                        unit_view_more_pages = 1;
                    if (unit_view_abil_count > ABIL_ROWS_PER_PAGE)
                        unit_view_more_pages = 1;
                    Help_Load_Unit_View(mode);
                    UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
                }
                break;
            }
        }
        for (i = 0; i < rows && can_dispel == 1; i++) {
            if ((unit_view_row_icon_ctl[i] == input || unit_view_row_text_ctl[i] == input) &&
                unit_view_row_ench[i] != ABIL_ROW_PLAIN && unit_view_row_ench[i] != ABIL_ROW_ITEM) {
                Play_Click_Sound();
                UI_Clear_Help();
                died = Unit_Dispel_Enchant_Confirm(unit_view_unit, unit_view_row_ench[i], p_text_buf);
                if (died == 1) {
                    done = -1;
                    Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
                } else {
                    UI_Suspend_Redraw_Fn();
                    UI_Set_Redraw_Fn(UnitView_Redraw, 1);
                    redraw = 1;
                    Heap_Release(p_scratch);
                    Heap_Mark(p_scratch);
                    UnitView_Alloc_Buffers();
                    if (p_units[unit_view_unit].type > UNIT_HERO_CHOSEN)
                        Load_Unit_Figure(unit_view_unit, 0);
                    else
                        unit_view_portrait_pic = LBX_Reload_Next("PORTRAIT",
                            unit_types[p_units[unit_view_unit].type].hero_portrait, p_scratch);
                    Battle_Unit_Init(unit_view_unit, p_unit_stats_buf);
                    UnitView_Build_Abilities(unit_view_unit, p_unit_view_list, &unit_view_abil_count);
                    if (unit_view_abil_first >= unit_view_abil_count)
                        unit_view_abil_first -= ABIL_ROWS_PER_PAGE;
                    unit_view_more_pages = 0;
                    if (unit_view_is_hero == 1 && unit_view_abil_count > ABIL_HERO_FIRST_ROWS)
                        unit_view_more_pages = 1;
                    if (unit_view_abil_count > ABIL_ROWS_PER_PAGE)
                        unit_view_more_pages = 1;
                    Help_Load_Unit_View(mode);
                    UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
                }
                break;
            }
        }
        if (input == unit_view_btn_dismiss) {
            Play_Click_Sound();
            if (p_units[unit_view_unit].hero_slot == -1) {
                strcpy(p_text_buf, "Do you wish to disband the unit of \x02");
                strcat(p_text_buf, unit_types[p_units[unit_view_unit].type].name);
                strcat(p_text_buf, "\x01?");
            } else {
                strcpy(p_text_buf, "Do you wish to dismiss \x02");
                strcat(p_text_buf, wizards[current_player].hired_heroes[p_units[unit_view_unit].hero_slot].name);
                strcat(p_text_buf, "\x01?");
            }
            UI_Clear_Help();
            if (Confirm_Box(p_text_buf) == 1) {
                done = 1;
                Unit_Dismiss(unit_view_unit);
                Player_Stacks_Changed_Nop(current_player);
                Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, p_units[unit_view_unit].x,
                                p_units[unit_view_unit].y);
                Units_Set_Draw_Priority();
                Stack_Clear_Draw_Priority();
            }
            UI_Suspend_Redraw_Fn();
            UI_Set_Redraw_Fn(UnitView_Redraw, 1);
            Help_Load_Unit_View(mode);
            UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
            redraw = 1;
        }
        if (input == unit_view_ctl_exit || input == unit_view_btn_ok || input == esc_key) {
            Play_Click_Sound();
            done = -1;
        }
        if (done == 0 && redraw == 0) {    /* redraw is uninitialised on the first pass (original) */
            Screen_Restore_A800();
            UnitView_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
        redraw = 0;
    }
    present_mode = PRESENT_DISSOLVE;
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
}

/* redraw callback of the unit view: the zoom lines (overland), the window, Ok / Dismiss and the paging arrows */
void UnitView_Redraw(void)
{
    if (unit_view_from_combat == 0)
        Window_Zoom_Lines_Redraw();
    Draw_Unit_View(unit_view_x, unit_view_y, unit_view_mode, unit_view_unit, p_unit_view_list, unit_view_abil_count,
                   p_unit_view_item_pic);
    if (unit_view_mode != UNIT_VIEW_COMBAT) {
        Set_Pic_Frame(ok_button_pic, 0);
        Draw_Pic_Frame(unit_view_x + 222, unit_view_y + 162, ok_button_pic);
        Set_Pic_Frame(ok_button_pic, 0);
        Draw_Pic_Frame(unit_view_x + 222, unit_view_y + 143, ok_button_pic);
    }
    if (unit_view_more_pages == 1) {
        Set_Pic_Frame(unitview_arrow_up_pic, 0);
        Draw_Pic_Frame(unit_view_x + 204, unit_view_y + 105, unitview_arrow_up_pic);
        Set_Pic_Frame(unitview_arrow_down_pic, 0);
        Draw_Pic_Frame(unit_view_x + 204, unit_view_y + 168, unitview_arrow_down_pic);
    }
}

/* (x, y, mode, subject, list, count, item pic): the whole window. mode 0 = city production preview: subject < 100 a
   building (picture, name, maintenance, Allows list, description), subject >= 100 the unit type subject - 100 ("Cost N(M)");
   mode 1 overland, mode 2 combat (subject = the unit). Hero: portrait, name, "The <type>"; else the figures on land or
   water and the name (race / realm prefix with ABILITY_RACE_PREFIX). Rows: Moves, Heads (Hydra), Upkeep or Damage
   (combat), Melee, Range, Armor, Resist, Hits, then the ability list */
void Draw_Unit_View(int x, int y, int mode, int subject, struct UnitAbilityList *list, int count, unsigned pic)
{
    int offset;
    int i;
    int sep;
    int icon;
    int tmp;                                    /* by turns: the saved production, a text width, the Moves icon, a unit type */
    int allows_x;
    int allows_y;
    int production_cost;
    int owner;
    int unit;
    int bld;
    int total;
    int shows_unit;
    int race;
    int coins;
    int pic_x;
    int pic_y;
    char colors[6];
    int per_row;
    int utype;
    int tmp_x;                                  /* also a unit type and a bound x */
    int dy;                                     /* also a bound y */
    int pic_w;
    int pic_h;
    unsigned back;
    int allows[9];
    char name_buf[20];

    unit_view_x = x;
    unit_view_y = y;
    unit = subject;
    bld = subject;
    Set_Draw_Page();
    if (unit_view_clear_bg == 1)
        Fill_Rect(unit_view_x, unit_view_y, unit_view_x + 281, unit_view_y + 183, 0);
    if (mode == UNIT_VIEW_PRODUCTION) {
        Draw_Pic_Frame(unit_view_x, unit_view_y, unitview_build_bg_pic);
    } else {
        Draw_Pic_Frame(unit_view_x, unit_view_y, unitview_bg_pic);
        if (mode != UNIT_VIEW_COMBAT)
            Draw_Pic_Frame(unit_view_x + 213, unit_view_y + 133, unitview_button_panel_pic);
    }
    shows_unit = mode;
    if (mode == UNIT_VIEW_PRODUCTION) {
        if (unit >= PROD_UNIT_BASE) {
            unit = unit - PROD_UNIT_BASE;
            utype = unit;
            shows_unit = 1;
        } else {
            shows_unit = 0;
        }
    } else {
        utype = p_units[unit].type;
    }
    unit_view_is_hero = 0;
    if (mode != UNIT_VIEW_PRODUCTION && p_units[unit].hero_slot != -1)
        unit_view_is_hero = 1;
    colors[0] = 0xc5;
    colors[1] = 0xff;
    Font_Select_Palette(1, colors);
    Font_Select_Outline(1, 15, 0, 0);
    Set_Text_Shadow_Color(251);
    offset = 0;
    if (mode == UNIT_VIEW_PRODUCTION) {
        tmp = p_cities[city_screen_city].producing;
        p_cities[city_screen_city].producing = subject;
        production_cost = City_Production_Cost(city_screen_city);
        p_cities[city_screen_city].producing = tmp;
        offset = 4;
        Print_Text(offset + unit_view_x + 51, unit_view_y + 34, "Cost");
        itoa(production_cost, message_text, 10);
        if (shows_unit == 1) {
            strcat(message_text, "(");
            itoa(unit_types[utype].cost, name_buf, 10);
            strcat(message_text, name_buf);
            strcat(message_text, ")");
        }
        Print_Text(offset + unit_view_x + 73, unit_view_y + 34, message_text);
    }
    switch (shows_unit) {
    case 1:
    case 2:
        colors[0] = 0xc5;
        colors[1] = 0x0e;
        colors[2] = 0xff;
        colors[3] = 0x50;
        colors[4] = 0xff;
        Font_Select_Palette(4, colors);
        Font_Select_Outline(4, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        if (mode != UNIT_VIEW_PRODUCTION && p_units[unit].hero_slot != -1) {
            owner = p_units[unit].owner;
            Draw_Pic_Frame(unit_view_x + 9, unit_view_y + 7, unit_view_portrait_pic);
            Draw_Pic_Frame(unit_view_x + 8, unit_view_y + 6, unitview_portrait_frame_pic);
            Print_Text(unit_view_x + 51, unit_view_y + 6, wizards[owner].hired_heroes[p_units[unit].hero_slot].name);
            Print_Text(unit_view_x + 51, unit_view_y + 17, "The");
            Print_Text(unit_view_x + 75, unit_view_y + 17, unit_types[utype].name);
        } else {
            if (p_unit_stats_buf->transport_capacity > 0 ||
                (mode == UNIT_VIEW_OVERLAND && Tile_Is_Water(p_units[unit].x, p_units[unit].y, p_units[unit].plane) == 1)) {
                Draw_Pic_Frame(offset + unit_view_x + 6, unit_view_y + 18, unitview_water_bg_pic);
            } else {
                Set_Pic_Frame(unitview_land_bg_pic, 0);
                Draw_Pic_Frame(offset + unit_view_x + 5, unit_view_y + 5, unitview_land_bg_pic);
            }
            Draw_Unit_Figures(offset + unit_view_x + 11, unit_view_y - 3, unit, mode);
            if (mode != UNIT_VIEW_PRODUCTION && (p_unit_stats_buf->abilities & ABILITY_RACE_PREFIX)) {
                race = (char)p_unit_stats_buf->race;
                tmp_x = utype;
                if (race >= RACE_ARCANE_CREATURE) {
                    Print_Text(offset + unit_view_x + 51, unit_view_y + 11, arcane_first_realm_names[race - RACE_ARCANE_CREATURE]);
                    tmp = Text_Width(arcane_first_realm_names[race - RACE_ARCANE_CREATURE]);
                } else {
                    Print_Text(offset + unit_view_x + 51, unit_view_y + 11, race_table[race].name);
                    tmp = Text_Width(race_table[race].name);
                }
                Print_Text(offset + unit_view_x + tmp + 56, unit_view_y + 11, unit_types[tmp_x].name);
            } else {
                tmp = utype;
                if (mode != UNIT_VIEW_PRODUCTION) {
                }
                dy = 0;
                if (mode == UNIT_VIEW_PRODUCTION)
                    dy = -4;
                Print_Text(offset + unit_view_x + 51, unit_view_y + dy + 11, unit_types[tmp].name);
            }
        }
        colors[0] = 0xc5;
        colors[1] = 0xff;
        Font_Select_Palette(1, colors);
        Font_Select_Outline(1, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        dy = 0;
        if (mode == UNIT_VIEW_PRODUCTION)
            dy = -7;
        Print_Text(offset + unit_view_x + 51, unit_view_y + dy + 27, "Moves");
        UnitView_Move_Icon(p_unit_stats_buf->move_flags, &tmp);
        tmp_x = 0;
        for (i = 0; i < p_unit_stats_buf->moves / 2; i++) {
            Draw_Pic_Frame(offset + unit_view_x + tmp_x + 84, unit_view_y + dy + 25, unitview_stat_icons[tmp]);
            tmp_x += 8;
        }
        if (mode != UNIT_VIEW_PRODUCTION) {
            tmp = utype;
            if ((int)p_spell_data[SPELL_HYDRA].effect_param == tmp) {
                Print_Text(offset + unit_view_x + 121, unit_view_y + 27, "Heads");
                Print_Number_Right(offset + unit_view_x + 153, unit_view_y + 27, p_unit_stats_buf->figures_alive);
            }
        }
        switch (mode) {
        case 2:
            Print_Text(unit_view_x + 51, unit_view_y + 34, "Damage");
            Print_Number_Right(unit_view_x + 90, unit_view_y + 34, p_unit_stats_buf->front_damage);
            break;
        case 0:
        case 1:
            dy = 0;
            if (mode == UNIT_VIEW_PRODUCTION)
                dy = -7;
            Print_Text(offset + unit_view_x + 51, unit_view_y + dy + 34, "Upkeep");
            if (mode == UNIT_VIEW_PRODUCTION)
                Draw_Unit_Upkeep(offset + unit_view_x + 85, unit_view_y + dy + 33, -utype);
            else
                Draw_Unit_Upkeep(offset + unit_view_x + 85, unit_view_y + dy + 33, unit);
            break;
        }
        colors[0] = 0xc5;
        colors[1] = 0x0e;
        colors[2] = 0xff;
        colors[3] = 0x50;
        colors[4] = 0xff;
        Font_Select_Palette(4, colors);
        Font_Select_Outline(4, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        Print_Text(offset + unit_view_x + 8, unit_view_y + 47, "Melee");
        UnitView_Melee_Icon(p_unit_stats_buf->weapon_icon, &icon);
        per_row = 15;
        if (mode == UNIT_VIEW_PRODUCTION)
            per_row = 10;
        Draw_Stat_Icons(offset + unit_view_x + 49, unit_view_y + 47, icon, p_unit_stats_buf->melee, per_row,
                        p_unit_stats_buf->melee_bonus_shown, p_unit_stats_buf->melee_penalty_shown);
        Print_Text(offset + unit_view_x + 8, unit_view_y + 58, "Range");
        if (p_unit_stats_buf->ranged_type != RANGED_NONE &&
            (p_unit_stats_buf->ranged_type / 10 == RANGED_CLASS_BOULDER || p_unit_stats_buf->ranged_type / 10 == RANGED_CLASS_MISSILE ||
             p_unit_stats_buf->ranged_type / 10 == RANGED_CLASS_MAGIC)) {
            UnitView_Ranged_Icon(p_unit_stats_buf->ranged_type, &icon);
            Draw_Stat_Icons(offset + unit_view_x + 49, unit_view_y + 58, icon, p_unit_stats_buf->ranged, per_row,
                            p_unit_stats_buf->ranged_bonus_shown, p_unit_stats_buf->ranged_penalty_shown);
        }
        Print_Text(offset + unit_view_x + 8, unit_view_y + 68, "Armor");
        Draw_Stat_Icons(offset + unit_view_x + 49, unit_view_y + 68, STAT_ICON_SHIELD, p_unit_stats_buf->defense, per_row,
                        p_unit_stats_buf->defense_bonus_shown, p_unit_stats_buf->defense_penalty_shown);
        Print_Text(offset + unit_view_x + 8, unit_view_y + 79, "Resist");
        Draw_Stat_Icons(offset + unit_view_x + 49, unit_view_y + 79, STAT_ICON_RESIST, p_unit_stats_buf->resist, per_row,
                        p_unit_stats_buf->resist_bonus_shown, p_unit_stats_buf->resist_penalty_shown);
        Print_Text(offset + unit_view_x + 8, unit_view_y + 90, "Hits");
        Draw_Hits_Icons(offset + unit_view_x + 49, unit_view_y + 90, STAT_ICON_HEART, p_unit_stats_buf->hits_per_figure,
                        p_unit_stats_buf->front_damage, per_row, p_unit_stats_buf->hits_bonus_shown);
        Draw_Unit_Ability_List(offset + unit_view_x + 8, unit_view_y + 108, list, count, unit_view_abil_first, pic);
        break;
    case 0:
        pic_x = unit_view_x + 6;
        pic_y = unit_view_y + 4;
        Heap_Mark(p_scratch);
        back = Heap_Alloc_Back(p_scratch, 500);
        Heap_Release(p_scratch);
        Building_Pic_Opaque_Bounds(city_screen_city, bld, &tmp_x, &dy, &pic_w, &pic_h, back);
        pic_x = pic_x + ((44 - pic_w) / 2 - tmp_x);
        pic_y = pic_y + ((36 - pic_h) / 2 - dy);
        Pic_Set_Remap_Flag(back);
        Draw_Bitmap(pic_x, pic_y, back);
        colors[0] = 0xc5;
        colors[1] = 0x0e;
        colors[2] = 0xff;
        colors[3] = 0x50;
        colors[4] = 0xff;
        Font_Select_Palette(4, colors);
        Font_Select_Outline(4, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        Set_Space_Width(1);
        far_strcpy(message_text, 0, p_building_data[bld].name);
        if (Text_Width(message_text) > 100)
            Paragraph_Print(offset + unit_view_x + 51, unit_view_y + 7, 100, message_text, PARA_LEFT);
        else
            Print_Text(offset + unit_view_x + 51, unit_view_y + 11, message_text);
        Print_Text(offset + unit_view_x + 8, unit_view_y + 47, "Maintenance");
        colors[0] = 0xc5;
        colors[1] = 0xff;
        Font_Select_Palette(2, colors);
        Font_Select_Outline(2, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        if (p_building_data[bld].gold_upkeep == 0) {
            Print_Number_Right(offset + unit_view_x + 92, unit_view_y + 48, p_building_data[bld].gold_upkeep);
        } else {
            coins = abs(p_building_data[bld].gold_upkeep);
            for (i = 0; i < coins; i++)
                Draw_Pic_Frame(offset + unit_view_x + i * 9 + 89, unit_view_y + 49, gold_coin_pic);
        }
        Building_Allows_List(bld, &total, allows);
        colors[0] = 0xc5;
        colors[1] = 0x0e;
        colors[2] = 0xff;
        colors[3] = 0x50;
        colors[4] = 0xff;
        Font_Select_Palette(4, colors);
        Font_Select_Outline(4, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        Print_Text(offset + unit_view_x + 8, unit_view_y + 58, "Allows");
        allows_x = offset + unit_view_x + 60;
        allows_y = unit_view_y + 59;
        colors[0] = 0xc5;
        colors[1] = 0xff;
        Font_Select_Palette(2, colors);
        Font_Select_Outline(2, 15, 0, 0);
        Set_Space_Width(1);
        Set_Line_Spacing(1);
        strcpy(message_text, "");
        sep = 0;
        for (i = 0; i < total; i++) {
            Text_List_Separator(&sep, total, message_text);
            if (allows[i] < PROD_UNIT_BASE) {
                far_strcpy(name_buf, 0, p_building_data[allows[i]].name);
                strcat(message_text, name_buf);
            } else {
                strcat(message_text, unit_types[allows[i] - PROD_UNIT_BASE].name);
            }
        }
        if (total > 0)
            strcat(message_text, ".");
        Paragraph_Print(allows_x, allows_y, 100, message_text, PARA_LEFT);
        colors[0] = 0xc5;
        colors[1] = 0x0e;
        colors[2] = 0xff;
        colors[3] = 0x50;
        colors[4] = 0xff;
        Font_Select_Palette(4, colors);
        Font_Select_Outline(4, 15, 0, 0);
        Set_Text_Shadow_Color(251);
        Set_Space_Width(2);
        Paragraph_Print(offset + unit_view_x + 9, unit_view_y + 107, 147, p_building_desc_text, PARA_LEFT);
        break;
    }
}

/* (x, y, unit or -type): the upkeep icons - gold, mana, food (1 for a normal unit; -type: the city preview).
   BUG #81: the real food upkeep counts types > 34, this row types > 35, so a Trireme shows no food here
   Numbered: BUG #81 (BUGS_AND_QUIRKS.txt) */
void Draw_Unit_Upkeep(int x, int y, int unit)
{
    int food;
    int cx;
    int cy;
    int total;
    int gold;
    int mana;

    if (unit > -1) {
        gold = Unit_Gold_Upkeep(unit);
        mana = Unit_Mana_Upkeep(unit);
        food = 0;
        if (!(unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) && p_units[unit].type > UNIT_TRIREME)
            food = 1;
        if (p_units[unit].mutation_flags & MUT_UNDEAD)
            food = 0;
    } else {
        unit = abs(unit);
        gold = unit_types[unit].upkeep;
        mana = 0;
        food = 1;
    }
    total = gold + mana + food;
    cy = y;
    cx = x;
    Draw_Icon_Row_Fit(&cx, cy, total, gold, resource_x10_pics[2], resource_x1_pics[2]);
    Draw_Icon_Row_Fit(&cx, cy, total, mana, resource_x10_pics[3], resource_x1_pics[3]);
    Draw_Icon_Row_Fit(&cx, cy, total, food, resource_x10_pics[0], resource_x1_pics[0]);
}

/* (x, y, icon, value, per_row, bonus, penalty): one stat row of value + lost icons (at most 2 rows, the second offset
   by (3, 2)); the last `bonus` from the gold set, the last `penalty` darkened; 2 px more after every 5th. -> 0 when nothing is drawn, no value otherwise */
int Draw_Stat_Icons(int x, int y, int icon, int value, int per_row, int bonus, int penalty)
{
    int cur_x;
    int i;
    int cur_y;
    int row2;
    int total;

    total = value + penalty;
    if (total < 1)
        return 0;
    if (total > per_row * 2) {
        if (value > per_row * 2) {
            total = per_row * 2;
            penalty = 0;
        } else {
            penalty = per_row * 2 - value;
            total = value + penalty;
        }
    }
    bonus = total - bonus;                      /* from here: the first gold index */
    penalty = total - penalty;                  /* the first darkened index */
    cur_x = x;
    cur_y = y;
    if (total > per_row) {
        row2 = total - per_row;
        for (i = 0; i < per_row; i++) {
            if (i >= bonus)
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
            else
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
            if (i >= penalty)
                Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
            cur_x += 10;
            if ((i + 1) % 5 == 0)
                cur_x += 2;
        }
        cur_x = x + 3;
        cur_y = y + 2;
        for (i = 0; i < row2; i++) {
            if (i + per_row >= bonus)
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
            else
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
            if (i + per_row >= penalty)
                Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
            cur_x += 10;
            if ((i + 1) % 5 == 0)
                cur_x += 2;
        }
    } else {
        for (i = 0; i < total; i++) {
            if (i >= bonus)
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
            else
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
            if (i >= penalty)
                Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
            cur_x += 10;
            if ((i + 1) % 5 == 0)
                cur_x += 2;
        }
    }
}

/* (x, y, icon, hits per figure, front damage, per_row, bonus): the Hits row - the damage taken darkened from the
   end (on 2 rows: the back row first); -> 0 when nothing is drawn, no value otherwise */
int Draw_Hits_Icons(int x, int y, int icon, int hits, int damage, int per_row, int bonus)
{
    int cur_x;
    int i;
    int cur_y;
    int left;
    int row1;
    int row2;

    bonus = hits - bonus;
    if (hits < 1)
        return 0;
    if (hits > per_row * 2)
        hits = per_row * 2;
    cur_x = x;
    cur_y = y;
    if (hits > per_row) {
        row1 = per_row;
        row2 = hits - per_row;
        if (damage < row2) {
            for (i = 0; i < per_row; i++) {
                if (i >= bonus)
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
                else
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
                cur_x += 10;
                if ((i + 1) % 5 == 0)
                    cur_x += 2;
            }
            cur_x = x + 3;
            cur_y = y + 2;
            left = row2 - damage;
            for (i = 0; i < row2; i++) {
                if (i >= bonus - per_row)
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
                else
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
                if (i >= left)
                    Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
                cur_x += 10;
                if ((i + 1) % 5 == 0)
                    cur_x += 2;
            }
        } else {
            for (i = 0; i < row2; i++) {
                if (i >= bonus - per_row)
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
                else
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
                Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
                cur_x += 10;
                if ((i + 1) % 5 == 0)
                    cur_x += 2;
            }
            cur_x = x + 3;
            cur_y = y + 2;
            left = row1 - (damage - row2);
            for (i = 0; i < row1; i++) {
                if (i >= bonus)
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
                else
                    Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
                if (i >= left)
                    Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
                cur_x += 10;
                if ((i + 1) % 5 == 0)
                    cur_x += 2;
            }
        }
    } else {
        left = hits - damage;
        for (i = 0; i < hits; i++) {
            if (i >= bonus)
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons_gold[icon]);
            else
                Draw_Pic_Frame(cur_x, cur_y, unitview_stat_icons[icon]);
            if (i >= left)
                Fill_Box_Styled(cur_x, cur_y, cur_x + 8, cur_y + 7, FILL_SHADOW);
            cur_x += 10;
            if ((i + 1) % 5 == 0)
                cur_x += 2;
        }
    }
}

/* (move_flags, &icon): the Moves row's stat icon, the first bit found deciding: (& 0, never) and cavalry the boot,
   sailing the wave, flying the wings, swimming the wave, else the boot - so a cavalry unit shows the boot even when
   it flies or swims, and sailing beats flying
   Numbered: BUG #83 (BUGS_AND_QUIRKS.txt) */
void UnitView_Move_Icon(int move_flags, int *icon)
{
    int move_icon;

    move_icon = STAT_ICON_BOOT;
    if (move_flags & 0)
        move_icon = STAT_ICON_BOOT;
    else if (move_flags & MOVE_CAVALRY)
        move_icon = STAT_ICON_BOOT;
    else if (move_flags & MOVE_SAILING)
        move_icon = STAT_ICON_WAVE;
    else if (move_flags & MOVE_FLYING)
        move_icon = STAT_ICON_WINGS;
    else if (move_flags & MOVE_SWIMMING)
        move_icon = STAT_ICON_WAVE;
    else if (move_flags & MOVE_TELEPORTING)
        move_icon = STAT_ICON_BOOT;
    else if (move_flags & MOVE_FORESTER)
        move_icon = STAT_ICON_BOOT;
    else if (move_flags & MOVE_MOUNTAINEER)
        move_icon = STAT_ICON_BOOT;
    else if (move_flags & MOVE_MERGING)
        move_icon = STAT_ICON_BOOT;
    *icon = move_icon;
}

/* (x, y, list, count, first, item pic): the visible ability rows (8, or 4 on a hero's first page) in 2 columns of 4,
   17 px apart; item rows on the SPECIAL 3 backdrop with the item's icon
   Numbered: BUG #83 (BUGS_AND_QUIRKS.txt) */
void Draw_Unit_Ability_List(int x, int y, struct UnitAbilityList *list, int count, int first, unsigned pic)
{
    int i;
    int rows;
    int col_x;
    int row_y;
    char colors[6];

    rows = ABIL_ROWS_PER_PAGE;
    if (first + rows > count)
        rows = count - first;
    if (first == 0 && unit_view_is_hero == 1)
        rows = ABIL_HERO_FIRST_ROWS;
    colors[0] = 0xc5;
    colors[1] = 0xff;
    colors[2] = 0x50;
    colors[3] = 0xff;
    Font_Select_Palette(2, colors);
    Font_Select_Outline(2, 15, 0, 0);
    Set_Text_Shadow_Color(251);
    Set_Space_Width(1);
    row_y = y;
    for (i = 0; i < rows; i++) {
        col_x = x;
        if (rows > 4 && i > 3) {
            col_x += 107;
            if (i == 4)
                row_y = y;
        }
        if (list->icon[i + first] != -1) {
            Draw_Pic_Frame(col_x - 1, row_y - 1, unitview_row_frame_pics[i]);
            if (list->ench_mask[i + first] == ABIL_ROW_ITEM) {
                Draw_Pic_Frame(col_x, row_y, special_icons[SPECIAL_ICON_ITEM]);
                if (list->help[i + first] > 0) {   /* help holds the item: item 0 is drawn like an empty slot */
                    Item_Build_Icon(list->help[i + first], pic);
                    Draw_Bitmap_Clipped(col_x - 2, row_y - 2, pic);
                } else {
                    Draw_Pic_Frame(col_x, row_y, list->icon[i + first]);
                }
            } else {
                Draw_Pic_Frame(col_x, row_y, list->icon[i + first]);
            }
        }
        Print_Text(col_x + 19, row_y + 5, list->text[i + first]);
        row_y += 17;
    }
    Item_Outline_Cycle_Tick();
}

/* (x, y, list, count, first): the row controls - item rows go to unit_view_item_icon/text_ctl[k], the others to
   unit_view_row_icon/text_ctl[row] with the row's enchantment bit */
void UnitView_Add_Ability_Controls(int x, int y, struct UnitAbilityList *list, int count, int first)
{
    int i;
    int row_y;
    int rows;
    int col_x;
    int k;

    UnitView_Clear_Ability_Controls();
    rows = ABIL_ROWS_PER_PAGE;
    if (first + rows > count)
        rows = count - first;
    if (unit_view_abil_first == 0 && unit_view_is_hero == 1)   /* the global: every caller passes it as first */
        rows = ABIL_HERO_FIRST_ROWS;
    UnitView_Add_Scroll_Buttons(x + 196, y - 3);
    k = 0;
    row_y = y;
    for (i = 0; i < rows; i++) {
        col_x = x;
        if (rows > 4 && i > 3) {
            col_x += 107;
            if (i == 4)
                row_y = y;
        }
        if (list->ench_mask[i + first] == ABIL_ROW_ITEM) {
            unit_view_item_icon_ctl[k] = UI_Add_Picture(col_x, row_y, list->icon[i + first], "", -1);
            unit_view_item_text_ctl[k] = UI_Add_Region(col_x + 19, row_y + 5, col_x + 93, row_y + 13, "", -1);
            k++;
        } else {
            unit_view_row_icon_ctl[i] = UI_Add_Picture(col_x, row_y, list->icon[i + first], "", list->help[i + first]);
            unit_view_row_text_ctl[i] = UI_Add_Region(col_x + 19, row_y + 5, col_x + 93, row_y + 13, "",
                                                       list->help[i + first]);
            unit_view_row_ench[i] = list->ench_mask[i + first];
        }
        row_y += 17;
    }
}

/* (list, count, base): the right-click help of the rows - a hero's first page and the item rows (negative help
   only) at base+11+i, every other row at base+15+i - and of the paging arrows (base+7, base+8: cleared without more pages) */
void UnitView_Set_Ability_Help(struct UnitAbilityList *list, int count, int help_base)
{
    int i;
    int rows;

    if (unit_view_more_pages != 1) {
        help_entries[help_base + 7].id = -1;
        help_entries[help_base + 8].id = -1;
    }
    rows = ABIL_ROWS_PER_PAGE;
    if (unit_view_abil_first + rows > count)
        rows = count - unit_view_abil_first;
    if (unit_view_abil_first == 0 && unit_view_is_hero == 1)
        rows = ABIL_HERO_FIRST_ROWS;
    for (i = 0; i < 12; i++)
        help_entries[help_base + i + 11].id = -1;
    for (i = 0; i < rows; i++) {
        if (list->ench_mask[i + unit_view_abil_first] == ABIL_ROW_ITEM) {
            if (list->help[i + unit_view_abil_first] < 0)
                help_entries[help_base + i + 11].id = abs(list->help[i + unit_view_abil_first]);
        } else if (unit_view_abil_first == 0 && unit_view_is_hero == 1) {
            help_entries[help_base + i + 11].id = list->help[i + unit_view_abil_first];
        } else {
            help_entries[help_base + i + 15].id = list->help[i + unit_view_abil_first];
        }
    }
}

/* (x, y): the paging arrows (hotkeys U / D) when there are more pages */
void UnitView_Add_Scroll_Buttons(int x, int y)
{
    unit_view_btn_up = UI_NO_CONTROL;
    unit_view_btn_down = UI_NO_CONTROL;
    if (unit_view_more_pages == 1) {
        unit_view_btn_up = UI_Add_Button(x, y, "", unitview_arrow_up_pic, "U", -1);
        unit_view_btn_down = UI_Add_Button(x, y + 63, "", unitview_arrow_down_pic, "D", -1);
    }
}

/* clears the row and item control ids (-1000) and the rows' enchantment bits */
void UnitView_Clear_Ability_Controls(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        unit_view_row_icon_ctl[i] = UI_NO_CONTROL;
        unit_view_row_text_ctl[i] = UI_NO_CONTROL;
        unit_view_row_ench[i] = ABIL_ROW_NONE;
    }
    unit_view_btn_up = UI_NO_CONTROL;
    unit_view_btn_down = UI_NO_CONTROL;
    for (i = 0; i < 3; i++) {
        unit_view_item_icon_ctl[i] = UI_NO_CONTROL;
        unit_view_item_text_ctl[i] = UI_NO_CONTROL;
    }
}

/* (input, &rows): the paging arrows - down: +8 (a hero's first page +4), wrapping to 0; up: back a page, wrapping
   to the last; rows = the visible rows of the new page
   Numbered: BUG #83 (BUGS_AND_QUIRKS.txt) */
void UnitView_Scroll_Abilities(int input, int *rows_out)
{
    if (input == unit_view_btn_down) {
        Play_Click_Sound();
        if (unit_view_abil_first == 0 && unit_view_is_hero == 1)
            unit_view_abil_first += ABIL_HERO_FIRST_ROWS;
        else
            unit_view_abil_first += ABIL_ROWS_PER_PAGE;
        if (unit_view_abil_first >= unit_view_abil_count)
            unit_view_abil_first = 0;
    }
    if (input == unit_view_btn_up) {
        Play_Click_Sound();
        if (unit_view_abil_first == 0 && unit_view_is_hero == 1) {
            unit_view_abil_first = (unit_view_abil_count - ABIL_HERO_FIRST_ROWS) / ABIL_ROWS_PER_PAGE * ABIL_ROWS_PER_PAGE + ABIL_HERO_FIRST_ROWS;
            if (unit_view_abil_first != ABIL_HERO_FIRST_ROWS && unit_view_abil_first == unit_view_abil_count)
                unit_view_abil_first -= ABIL_ROWS_PER_PAGE;
        } else {
            unit_view_abil_first -= ABIL_ROWS_PER_PAGE;
        }
        if (unit_view_abil_first < 0) {
            if (unit_view_abil_first == -ABIL_HERO_FIRST_ROWS && unit_view_is_hero == 1)
                unit_view_abil_first = 0;
            else
                unit_view_abil_first = unit_view_abil_count / ABIL_ROWS_PER_PAGE * ABIL_ROWS_PER_PAGE;
        }
    }
    *rows_out = ABIL_ROWS_PER_PAGE;
    if (unit_view_abil_first + *rows_out > unit_view_abil_count)
        *rows_out = unit_view_abil_count - unit_view_abil_first;
    if (unit_view_abil_first == 0 && unit_view_is_hero == 1)
        *rows_out = ABIL_HERO_FIRST_ROWS;
}

/* (weapon_icon: 0 none, else WEAPON_* + 1, Unit_Apply_Modifiers; &icon): the Melee row's stat icon by weapon quality */
void UnitView_Melee_Icon(int weapon_icon, int *icon)
{
    int w;

    w = weapon_icon;
    if (w > 0) {
        switch (w) {
        case WEAPON_MAGIC + 1:
            *icon = STAT_ICON_MAGIC_SWORD;
            break;
        case WEAPON_MITHRIL + 1:
            *icon = STAT_ICON_MITHRIL;
            break;
        case WEAPON_ADAMANTIUM + 1:
            *icon = STAT_ICON_ADAMANTIUM;
            break;
        default:
            *icon = STAT_ICON_SWORD;
            break;
        }
    } else {
        *icon = STAT_ICON_SWORD;
    }
}

/* (ranged_type, &icon): the Range row's stat icon - 8 for 100 (thrown), 1 for 30..38 (magic: RANGED_CLASS_MAGIC),
   7 for 101..103 (breath / gaze), 6 for 10, 11, 21, 22 (boulders and two of the missiles), else 5 */
void UnitView_Ranged_Icon(int ranged_type, int *icon)
{
    switch (ranged_type) {
    case RANGED_THROWN:
        *icon = STAT_ICON_THROWN;
        break;
    case RANGED_MAGIC_LIGHTNING:
    case RANGED_MAGIC_FIRE:
    case RANGED_MAGIC_ILLUSION:
    case RANGED_MAGIC_DEATH:
    case RANGED_MAGIC_WIND:
    case RANGED_MAGIC_HOLY:
    case RANGED_MAGIC_DARK:
    case RANGED_MAGIC_SPRITE:
    case RANGED_MAGIC_NATURE:
        *icon = STAT_ICON_MAGIC_RANGED;
        break;
    case RANGED_FIRE_BREATH:
    case RANGED_LIGHTNING:
    case RANGED_STONING_GAZE:
        *icon = STAT_ICON_BREATH;
        break;
    case RANGED_BOULDER:
    case RANGED_UNUSED_11:
    case RANGED_SLING:
    case RANGED_UNUSED_22:
        *icon = STAT_ICON_BOULDER;
        break;
    default:
        *icon = STAT_ICON_BOW;
        break;
    }
}

/* (unit, count): the list paging of the hire screens (a hero starts at row 4, his portrait loaded) */
void UnitView_Init_Hire(int unit, int count)
{
    unit_view_abil_first = 0;
    unit_view_more_pages = 0;
    unit_view_abil_count = count;
    unit_view_is_hero = 0;
    if (p_units[unit].hero_slot != -1) {
        unit_view_is_hero = 1;
        unit_view_abil_first = ABIL_HERO_FIRST_ROWS;
        unit_view_portrait_pic = LBX_Reload_Next("PORTRAIT", unit_types[p_units[unit].type].hero_portrait, p_scratch);
    }
    if (unit_view_is_hero == 1 && unit_view_abil_count > ABIL_HERO_FIRST_ROWS)
        unit_view_more_pages = 1;
    if (unit_view_abil_count > ABIL_ROWS_PER_PAGE)
        unit_view_more_pages = 1;
    unit_view_clear_bg = 0;
}

/* (count): resets the list paging for a non-hero list (the city production preview) */
void UnitView_Init_List(int count)
{
    unit_view_abil_first = 0;
    unit_view_more_pages = 0;
    unit_view_abil_count = count;
    unit_view_is_hero = 0;
    unit_view_clear_bg = 0;
}
