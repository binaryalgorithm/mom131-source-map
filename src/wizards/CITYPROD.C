/* CITYPROD.C - WIZARDS.EXE overlay code segment 4e5c (stub 2409): the city production screen (Change in the city screen or a
   right click in the cities list): the buildable buildings (left column) and units (right column), the chosen
   item's unit view or BUILDESC description, Ok / Cancel; also the build-option rules - which units (at most 12;
   with more, Spearmen and Swordsmen are dropped) and buildings (prerequisites, the race's forbidden list, the
   terrain prerequisites 101+ such as 110 = sea beside the city) a city may build, what a building allows, and the
   remaining cost / turns of an item.
   This module's data (DS:2d72): its literals; BSS (DS:bff4..c03e, with the stack popup's two controls).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 4e5c src/wizards/CITYPROD.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_City_Production(void);
void CityProd_Alloc_Buffers(void);
void CityProd_Load_Selected_Figure(void);
void CityProd_Add_Controls(void);
void CityProd_Redraw(void);
void CityProd_Draw(void);
void Building_Allows_List(int bldg, int *n, int *out);
void City_Build_Options(int city, int *n_options, int *n_total, int *n_units, int *options);
int City_Terrain_Prereq(int city, int prereq_code);
int City_Remaining_Cost(int producing, int city);
int City_Turns_To_Complete(int cost, int city);
int Building_Race_Reachable(int bldg, int target);

#include "GAME.H"

#define CITYPROD_MAX_UNITS 12           /* City_Build_Options: at most 12 units, 20 choices in all */
#define CITYPROD_MAX_CHOICES 20

int cityprod_panel_x;                           /* DS:c03c  x origin of the panel's controls (0) */
int cityprod_panel_y;                           /* DS:c03a  y origin of the panel's controls (0) */
int cityprod_unused_c02e[6];                    /* DS:c02e  never referenced (inside this module's BSS block) */
int cityprod_selected;                          /* DS:c02c  the selected entry of p_production_choices */
int cityprod_unused_c02a;                       /* DS:c02a  never referenced (inside this module's BSS block) */
int cityprod_row_ctl_width;                     /* DS:c028  width of an option row's control (74); also the unit view's x */
int cityprod_row_ctl_height;                    /* DS:c026  height of an option row's control (11) */
int cityprod_unused_dx1;                        /* DS:c024  90, only feeds a value CityProd_Draw never uses */
int cityprod_unused_dy1;                        /* DS:c022  28 */
int cityprod_unused_dx2;                        /* DS:c020  102 */
int cityprod_unused_dy2;                        /* DS:c01e  55 */
int cityprod_unit_col_x;                        /* DS:c01c  left x of the unit column (240) */
int cityprod_unit_col_y;                        /* DS:c01a  top y of the unit column (4) */
int cityprod_row_height;                        /* DS:c018  row height of both columns (14) */
int cityprod_bldg_col_x;                        /* DS:c016  left x of the building column (0) */
int cityprod_bldg_col_y;                        /* DS:c014  top y of the building column (4) */
int *p_cityprod_option_ctls;                    /* DS:c012  near int[25]: the control of every option row */
int stack_popup_outside_ctl;                    /* DS:c010  (used by STACKPOP.C) a full-screen control behind the stack popup */
int stack_popup_window_ctl;                     /* DS:c00e  (used by STACKPOP.C) the stack popup's window area; -1000 when absent */
int cityprod_unused_c00a[2];                    /* DS:c00a  never referenced (inside this module's BSS block) */
int cityprod_cancel_button;                     /* DS:c008 */
int cityprod_ok_button;                         /* DS:c006  -1000 while being re-added */
int cityprod_unused_flag;                       /* DS:c004  set to 0, never read */
unsigned p_cityprod_item_pic_buf;               /* DS:c002  Heap_Alloc_Front(p_scratch, 30): the chosen unit's figure */
int cityprod_option_count;                      /* DS:c000  entries in p_production_choices */
int cityprod_option_total;                      /* DS:bffe  City_Build_Options' n_total (its second count) */
                                                /* always = cityprod_option_count, so the "not enough population"
                                                   branch is never taken */
int cityprod_unit_option_count;                 /* DS:bffc  units in p_production_choices (= the first building's index) */
int cityprod_garrison_full;                     /* DS:bffa  1 when the city holds 9 units: unit choices are refused */
int *p_production_choices;                      /* DS:bff8  near int[20]: units as type + 100, then buildings */
struct UnitAbilityList *p_cityprod_abil_list;   /* DS:bff6  the chosen unit's ability list */
char *p_cityprod_spare_buf;                     /* DS:bff4  110 bytes, never used */
char *p_building_desc_text;                     /* DS:bff2  BUILDESC.LBX text buffer (also UNITVIEW.C); owner: this module, where ReMoM's layout starts ovr056's BSS (b130: held by the 4a3f stand-in until then; CITY.C never uses it); clarified using ReMoM repo */

extern struct RaceInfo race_table[14];          /* DS:1d86 */
extern unsigned ok_button_pic;                  /* DS:9498 */
extern unsigned cityprod_right_tab_pic;         /* DS:949a */
extern unsigned cityprod_left_tab_pic;          /* DS:949c */
extern unsigned unitview_build_bg_pic;          /* DS:97ba */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int production_return_screen;            /* DS:bfb6 */
extern int unit_view_abil_first;                /* DS:c18c */
extern int unit_view_more_pages;                /* DS:c18e */
extern int unit_view_abil_count;                /* DS:c190 */

void Load_Unit_View_Graphics(void);
void UI_Clear_Help(void);
void Help_Load_City_Production(void);
void City_Garrison_List(int city, int *count, int *list);
void Battle_Unit_Init_From_Type(int unit_type, struct BattleUnit far *bu);
void UnitView_Build_Type_Abilities(int unit_type, struct UnitAbilityList *list, int *count);
void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Clear_Controls(void);
void Screen_Copy_Front(void);
void Pic_Apply_Palette(unsigned pic);
void Remap_Build_Range(int first, int last);
void Fill_Box_Styled();                           /* no prototype here: called with 5 arguments (GRAPHICS.C takes 9, read by style) */
void UI_Input_Delay(int delay);
void Timer_Mark(void);
int UI_Add_Hotkey(char *key);
int UI_Get_Input(void);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
void Warning_Box(char *text);
void City_Recalculate(int city);
void UnitView_Scroll_Abilities(int input, int *rows_out);
void Reset_Clip_Rect(void);
void Remap_Build_Shadow(void);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Load_Unit_Figure(int unit_or_type, int is_type);
void UnitView_Clear_Ability_Controls(void);
void UnitView_Add_Ability_Controls(int x, int y, struct UnitAbilityList *list, int count, int first);
void Font_Select(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void UnitView_Init_List(int count);
void Draw_Unit_View(int x, int y, int mode, int subject, struct UnitAbilityList *list, int count, unsigned pic);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Text_Shadow_Color(int color);
void Font_Select_Palette(int font, char *colors);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Space_Width(int width);
int Print_Text(int x, int y, char *text);
int Print_Far_Text(int x, int y, char far *text);
int Tile_Is_Sea(int x, int y, int plane);
int City_Area_Tiles(int x, int y, int plane, int *xs, int *ys);
int Tile_Terrain_Prereq_Code(int x, int y, int plane);
int City_Production_Cost(int city);

/* () the production screen of city_screen_city: a click on an option selects it (a second click, or Ok, sets the
   city producing it); a unit while the garrison is full gives a warning (the population warning is never
   reached: cityprod_option_total always equals the count); Cancel / Esc leaves it unchanged; returns to the city screen or the cities list
   (production_return_screen)
   Numbered: BUG #45 (BUGS_AND_QUIRKS.txt) */
void Screen_City_Production(void)
{
    int hit_test;                               /* read, never used */
    int input;
    int skip_frame;
    int unit_type;                              /* first the city's producing value (a Production id) for the search, then a unit type */
    int choice;
    int scroll;
    int first;                                  /* written, never read */
    int hotkey;
    int garrison_count;
    int garrison[MAX_STACK];
    register int i;
    register int done;

    Load_Unit_View_Graphics();
    UI_Clear_Help();
    cityprod_panel_x = 0;
    cityprod_panel_y = 0;
    cityprod_row_ctl_width = 74;
    cityprod_row_ctl_height = 11;
    cityprod_row_height = 14;
    cityprod_unused_dx1 = 90;
    cityprod_unused_dy1 = 28;
    cityprod_unused_dx2 = 102;
    cityprod_unused_dy2 = 55;
    cityprod_unit_col_x = cityprod_panel_x + 240;
    cityprod_unit_col_y = cityprod_panel_y + 4;
    cityprod_bldg_col_x = cityprod_panel_x;
    cityprod_bldg_col_y = cityprod_panel_y + 4;
    Help_Load_City_Production();
    CityProd_Alloc_Buffers();
    City_Build_Options(city_screen_city, &cityprod_option_count, &cityprod_option_total, &cityprod_unit_option_count,
                       p_production_choices);
    City_Garrison_List(city_screen_city, &garrison_count, garrison);
    cityprod_garrison_full = 0;
    if (garrison_count == MAX_STACK)
        cityprod_garrison_full = 1;
    unit_type = p_cities[city_screen_city].producing;
    for (i = 0; i < cityprod_option_count; i++) {
        if (p_production_choices[i] == unit_type) {
            cityprod_selected = i;
            break;
        }
    }
    choice = p_production_choices[cityprod_selected];
    unit_view_abil_count = 0;
    if (p_production_choices[cityprod_selected] >= PROD_UNIT_BASE) {
        unit_type = p_production_choices[cityprod_selected] - PROD_UNIT_BASE;
        Battle_Unit_Init_From_Type(unit_type, p_unit_stats_buf);
        UnitView_Build_Type_Abilities(unit_type, p_cityprod_abil_list, &unit_view_abil_count);
    } else
        LBX_Load_Records_Near("BUILDESC", 0, p_building_desc_text, p_production_choices[cityprod_selected], 1, 200);
    unit_view_abil_first = 0;
    unit_view_more_pages = 0;
    if (unit_view_abil_count > 9)
        unit_view_more_pages = 1;
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(CityProd_Redraw, 1);
    CityProd_Load_Selected_Figure();
    cityprod_unused_flag = 0;
    skip_frame = 0;
    first = 1;
    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Pic_Apply_Palette(unitview_build_bg_pic);
    Palette_Upload();
    Palette_Unmark_Range(0, 255);
    Palette_Mark_Range(0, 255);
    Remap_Build_Range(0, 1);
    Fill_Box_Styled(0, 0, 319, 199, FILL_SHADOW);
    Screen_Save_A800();
    UI_Input_Delay(1);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        CityProd_Add_Controls();
        hotkey = UI_Add_Hotkey("\x1b");
        input = UI_Get_Input();
        hit_test = UI_Hit_Test();
        if (input == cityprod_cancel_button || input == hotkey) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_CITY;
        }
        for (i = 0; i < cityprod_option_count; i++) {
            if (p_cityprod_option_ctls[i] == input) {
                Play_Click_Sound();
                UI_Clear_Help();
                if (i < cityprod_option_total) {
                    if (cityprod_garrison_full == 1 && i < cityprod_unit_option_count) {
                        strcpy(p_text_buf, "There are already too many units currently in the city.");
                        Warning_Box(p_text_buf);
                        UI_Set_Redraw_Fn(CityProd_Redraw, 1);
                        skip_frame = 1;
                    } else if (cityprod_selected == i) {
                        p_cities[city_screen_city].producing = p_production_choices[cityprod_selected];
                        done = 1;
                        current_screen = SCREEN_CITY;
                        City_Recalculate(city_screen_city);
                    } else {
                        cityprod_selected = i;
                        choice = p_production_choices[cityprod_selected];
                        skip_frame = 1;
                        CityProd_Load_Selected_Figure();
                        if (choice < PROD_UNIT_BASE)
                            LBX_Load_Records_Near("BUILDESC", 0, p_building_desc_text,
                                                  p_production_choices[cityprod_selected], 1, 200);
                    }
                } else {                        /* dead: cityprod_option_total always equals the count */
                    strcpy(p_text_buf, "There is not enough population to support new construction.");
                    Warning_Box(p_text_buf);
                    UI_Set_Redraw_Fn(CityProd_Redraw, 1);
                    skip_frame = 1;
                }
                Help_Load_City_Production();
                break;
            }
        }
        if (input == cityprod_ok_button) {
            Play_Click_Sound();
            p_cities[city_screen_city].producing = p_production_choices[cityprod_selected];
            done = 1;
            current_screen = SCREEN_CITY;
            City_Recalculate(city_screen_city);
        }
        UnitView_Scroll_Abilities(input, &scroll);
        if (done == 0 && skip_frame == 0) {
            Screen_Restore_A800();
            CityProd_Redraw();
            Screen_Present();
            first = 0;
            Wait_Ticks(2);
        }
        skip_frame = 0;
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    if (production_return_screen == PROD_RETURN_CITY_LIST)
        current_screen = SCREEN_CITIES;
    if (production_return_screen == PROD_RETURN_CITY_SCREEN)
        current_screen = SCREEN_CITY;
    present_mode = PRESENT_DISSOLVE;
    Palette_Unmark_Range(0, 255);
    Palette_Mark_Range(0, 223);
    Remap_Build_Shadow();
    Load_Overland_Screen_Graphics();
}

/* () the screen's buffers: text (80), the ability list, a spare 110 bytes, the choices (20 words), their controls
   (25) and the building description (200) */
void CityProd_Alloc_Buffers(void)
{
    p_text_buf = Near_Alloc_Reset(80);
    p_cityprod_abil_list = (struct UnitAbilityList *)Near_Alloc(0x5f0);
    p_cityprod_spare_buf = Near_Alloc(110);
    p_production_choices = (int *)Near_Alloc(40);
    p_cityprod_option_ctls = (int *)Near_Alloc(50);
    p_building_desc_text = Near_Alloc(200);
}

/* () a 30-paragraph block for the chosen item's figure; loads the unit type's figure when it is a unit */
void CityProd_Load_Selected_Figure(void)
{
    int unit_type;

    p_cityprod_item_pic_buf = Heap_Alloc_Front(p_scratch, 30);
    if (p_production_choices[cityprod_selected] >= PROD_UNIT_BASE) {
        unit_type = p_production_choices[cityprod_selected] - PROD_UNIT_BASE;
        Load_Unit_Figure(unit_type, 1);
    }
}

/* () one control per option row (buildings in the left column, help 374; units in the right, help 375), the
   chosen unit's ability controls and their help, Cancel and Ok */
void CityProd_Add_Controls(void)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int unused;                                 /* set to 0, never read */
    int unit_row;
    register int i;
    register int bldg_row;

    unused = 0;
    bldg_row = 0;
    unit_row = 0;
    for (i = 0; i < cityprod_option_count; i++) {
        if (p_production_choices[i] < PROD_UNIT_BASE) {
            x1 = cityprod_bldg_col_x;
            y1 = cityprod_bldg_col_y + bldg_row * cityprod_row_height;
            help_entries[bldg_row].id = HELP_BUILDING_OPTIONS;
            bldg_row++;
        } else {
            x1 = cityprod_unit_col_x;
            y1 = cityprod_unit_col_y + unit_row * cityprod_row_height;
            help_entries[unit_row + 13].id = HELP_UNIT_OPTIONS;   /* help slots: 0 .. the buildings, 13 .. the units */
            unit_row++;
        }
        x2 = cityprod_row_ctl_width + x1 - 1;
        y2 = cityprod_row_ctl_height + y1 - 1;
        p_cityprod_option_ctls[i] = UI_Add_Region(x1, y1, x2, y2, "", -1);
    }
    UnitView_Clear_Ability_Controls();
    if (p_production_choices[cityprod_selected] >= PROD_UNIT_BASE) {
        UnitView_Add_Ability_Controls(cityprod_panel_x + 85, cityprod_panel_y + 108, p_cityprod_abil_list,
                                      unit_view_abil_count, unit_view_abil_first);
        help_entries[38].id = -1;
        help_entries[39].id = -1;
        help_entries[40].id = -1;
        help_entries[41].id = -1;
        for (i = 0; i < unit_view_abil_count; i++)
            help_entries[i + 38].id = p_cityprod_abil_list->help[i];
    }
    Font_Select(4, 4, 0, 0);
    Set_Font_Color(203);
    cityprod_cancel_button = UI_Add_Button(cityprod_panel_x + 99, cityprod_panel_y + 181, "Cancel", ok_button_pic, "C",
                                           -1);
    cityprod_ok_button = UI_NO_CONTROL;
    cityprod_ok_button = UI_Add_Button(cityprod_panel_x + 172, cityprod_panel_y + 181, "Ok", ok_button_pic, "O", -1);
}

/* () the redraw callback */
void CityProd_Redraw(void)
{
    Set_Draw_Page();
    CityProd_Draw();
}

/* () the screen: the chosen item's unit view (unit stats and abilities, or the building's picture and
   description) with its help, then both option columns - the chosen row highlighted, refused rows in red / grey */
void CityProd_Draw(void)
{
    int y;
    int unused_x1;
    int unused_y1;
    int unused_x2;
    int unused_y2;
    int choice;
    int unit_type;
    int unit_row;
    int bldg_row;
    char colors[6];
    register int i;
    register int x;

    x = cityprod_panel_x;
    y = cityprod_panel_y;
    cityprod_unused_flag = 0;
    unused_x1 = x + cityprod_unused_dx1;
    unused_y1 = y + cityprod_unused_dy1;
    Font_Select(2, 1, 0, 0);
    unused_x2 = x + cityprod_unused_dx2;
    unused_y2 = y + cityprod_unused_dy2;
    choice = p_production_choices[cityprod_selected];
    if (choice >= PROD_UNIT_BASE) {
        help_entries[28].id = -1;
        help_entries[29].id = -1;
        help_entries[30].id = -1;
        help_entries[31].id = HELP_UNIT_MOVES;
        help_entries[33].id = HELP_MELEE;
        help_entries[34].id = HELP_RANGE;
        help_entries[35].id = HELP_ARMOR;
        help_entries[36].id = HELP_RESISTANCE;
        help_entries[37].id = HELP_HITS;
        help_entries[38].id = -1;
        help_entries[39].id = -1;
        help_entries[40].id = -1;
        help_entries[41].id = -1;
        help_entries[42].id = HELP_UPKEEP;
        unit_type = choice - PROD_UNIT_BASE;
        Battle_Unit_Init_From_Type(unit_type, p_unit_stats_buf);
        UnitView_Build_Type_Abilities(unit_type, p_cityprod_abil_list, &unit_view_abil_count);
        UnitView_Init_List(unit_view_abil_count);
    } else {
        help_entries[28].id = HELP_BUILDING_DESCRIPTION;
        help_entries[29].id = HELP_ALLOWS;
        help_entries[30].id = HELP_MAINTENANCE;
        help_entries[31].id = -1;
        help_entries[33].id = -1;
        help_entries[34].id = -1;
        help_entries[35].id = -1;
        help_entries[36].id = -1;
        help_entries[37].id = -1;
        help_entries[38].id = -1;
        help_entries[39].id = -1;
        help_entries[40].id = -1;
        help_entries[41].id = -1;
        help_entries[42].id = -1;
        UnitView_Init_List(0);
    }
    Draw_Unit_View(cityprod_panel_x + cityprod_row_ctl_width, cityprod_panel_y, UNIT_VIEW_PRODUCTION, choice, p_cityprod_abil_list,
                   unit_view_abil_count, p_cityprod_item_pic_buf);
    unit_row = 0;
    bldg_row = 0;
    for (i = 0; i < cityprod_option_count; i++) {
        if (p_production_choices[i] < PROD_UNIT_BASE) {
            x = cityprod_bldg_col_x;
            y = cityprod_bldg_col_y + bldg_row * cityprod_row_height;
            Draw_Pic_Frame(x, y, cityprod_left_tab_pic);
            bldg_row++;
        } else {
            x = cityprod_unit_col_x;
            y = cityprod_unit_col_y + unit_row * cityprod_row_height;
            Draw_Pic_Frame(x, y, cityprod_right_tab_pic);
            unit_row++;
        }
        if (i < cityprod_option_total) {
            colors[0] = 251;
            colors[1] = 1;
            Set_Text_Shadow_Color(244);
            if (i == cityprod_selected) {
                colors[0] = 244;
                colors[1] = 255;
                Set_Text_Shadow_Color(248);
            }
            if (cityprod_garrison_full == 1 && i < cityprod_unit_option_count) {
                colors[0] = 4;
                colors[1] = 5;
            }
        } else {
            colors[0] = 251;
            colors[1] = 5;
            Set_Text_Shadow_Color(244);
        }
        Font_Select_Palette(2, colors);
        Font_Select_Shadow(2, 15, 0, 0);
        Set_Space_Width(1);
        if (p_production_choices[i] >= PROD_UNIT_BASE) {
            unit_type = p_production_choices[i] - PROD_UNIT_BASE;
            Print_Text(x + 3, y + 1, unit_types[unit_type].name);
        } else
            Print_Far_Text(x + 2, y + 1, p_building_data[p_production_choices[i]].name);
    }
}

/* (bldg, &n, out[]) the buildings (by id) and units (type + 100) that bldg is a prerequisite of, for the city
   screen's race: a building not forbidden to the race, reachable through its prerequisite chain
   (Building_Race_Reachable), or with a terrain prerequisite the city meets and its other prerequisite not yet
   built; a unit of the race (or any race, 14) with one of its buildings present */
void Building_Allows_List(int bldg, int *n, int *out)
{
    int k;
    int race;
    int forbidden;
    int prereq;
    int reach;
    register int i;

    *n = 0;
    race = p_cities[city_screen_city].race;
    for (i = 0; i < NUM_BUILDINGS; i++) {
        forbidden = 0;
        if (p_building_data[i].prereq1 == bldg || p_building_data[i].prereq2 == bldg) {
            for (k = 0; k < race_table[race].forbidden_count; k++) {
                if (race_table[race].forbidden_buildings[k] == i) {
                    forbidden = 1;
                    break;
                }
            }
            if (forbidden == 0) {
                if (p_building_data[i].prereq1 > BLD_PREREQ_TERRAIN_BASE) {
                    prereq = p_building_data[i].prereq1;
                    if (City_Terrain_Prereq(city_screen_city, prereq) == 1
                        && p_cities[city_screen_city].buildings[p_building_data[i].prereq2] != BLDSTATE_BUILT) {
                        out[*n] = i;
                        *n = *n + 1;
                    }
                } else {
                    reach = Building_Race_Reachable(bldg, i);
                    if (reach != -1) {
                        out[*n] = reach;
                        *n = *n + 1;
                    }
                }
            }
        }
    }
    for (i = NUM_HERO_TYPES; i < NUM_UNIT_TYPES; i++) {
        if ((unit_types[i].race == race || unit_types[i].race == RACE_GENERIC)
            && (unit_types[i].req_building_1 == bldg || unit_types[i].req_building_2 == bldg)) {
            if (p_cities[city_screen_city].buildings[unit_types[i].req_building_1] != BLDSTATE_NONE
                || p_cities[city_screen_city].buildings[unit_types[i].req_building_2] != BLDSTATE_NONE) {
                out[*n] = i + PROD_UNIT_BASE;
                *n = *n + 1;
            }
        }
    }
}

/* (city, &n_options, &n_total, &n_units, options[]) the city's build list: the units of its race (or any race)
   whose two buildings stand (not sold), at most 12 - with 12 or more the list is rebuilt without Spearmen and
   Swordsmen; then the buildings not built (or sold) and not forbidden to the race whose prerequisites stand
   (prereq1 101+ is a terrain prerequisite, City_Terrain_Prereq), up to 20 entries, with at most 12 buildings;
   n_units = the units; n_total gets the units, then +2, then the total. Also recounts City.building_count
   (every building slot that is not -1, sold ones included). BUG #432: req_building_1 / 2 (unsigned char) are compared
   with -1
   Numbered: BUG #44 (BUGS_AND_QUIRKS.txt) */
void City_Build_Options(int city, int *n_options, int *n_total, int *n_units, int *options)
{
    int k;
    int prereq;
    int forbidden;
    int race;
    int built;                                  /* counted, never read */
    int units;
    register int i;
    register int count;

    race = p_cities[city].race;
    count = 0;
    p_cities[city].building_count = 0;
    built = 0;
    for (i = 1; i < NUM_BUILDINGS; i++) {
        if (p_cities[city].buildings[i] != BLDSTATE_NONE)
            p_cities[city].building_count = p_cities[city].building_count + 1;
        if (p_cities[city].buildings[i] == BLDSTATE_BUILT)
            built++;
    }
    for (i = NUM_HERO_TYPES; i < NUM_UNIT_TYPES && count < CITYPROD_MAX_UNITS; i++) {
        if ((unit_types[i].race == race || unit_types[i].race == RACE_GENERIC) && unit_types[i].req_building_1 != BLD_NO_BUILDING
            && unit_types[i].req_building_2 != BLD_NO_BUILDING
            && p_cities[city].buildings[unit_types[i].req_building_1] != BLDSTATE_NONE
            && p_cities[city].buildings[unit_types[i].req_building_1] != BLDSTATE_REMOVED
            && p_cities[city].buildings[unit_types[i].req_building_2] != BLDSTATE_NONE
            && p_cities[city].buildings[unit_types[i].req_building_2] != BLDSTATE_REMOVED) {
            options[count] = i + PROD_UNIT_BASE;
            count = count + 1;
        }
    }
    if (count > 11) {                           /* twelve or more */
        for (i = 0; i < count; i++)
            options[i] = 0;
        count = 0;
        for (i = NUM_HERO_TYPES; i < NUM_UNIT_TYPES && count < CITYPROD_MAX_UNITS; i++) {
            if ((unit_types[i].race == race || unit_types[i].race == RACE_GENERIC) && unit_types[i].req_building_1 != BLD_NO_BUILDING
                && unit_types[i].req_building_2 != BLD_NO_BUILDING
                && p_cities[city].buildings[unit_types[i].req_building_1] != BLDSTATE_NONE
                && p_cities[city].buildings[unit_types[i].req_building_1] != BLDSTATE_REMOVED
                && p_cities[city].buildings[unit_types[i].req_building_2] != BLDSTATE_NONE
                && p_cities[city].buildings[unit_types[i].req_building_2] != BLDSTATE_REMOVED) {
                if (stricmp(unit_types[i].name, "Spearmen") == 0 || stricmp(unit_types[i].name, "Swordsmen") == 0)
                    continue;
                options[count] = i + PROD_UNIT_BASE;
                count = count + 1;
            }
        }
    }
    *n_total = count;
    *n_units = count;
    units = count;
    for (i = 1; i < NUM_BUILDINGS && count < CITYPROD_MAX_CHOICES; i++) {
        forbidden = 0;
        if (p_cities[city].buildings[i] == BLDSTATE_NONE || p_cities[city].buildings[i] == BLDSTATE_REMOVED) {
            for (k = 0; k < race_table[race].forbidden_count; k++) {
                if (race_table[race].forbidden_buildings[k] == i) {
                    forbidden = 1;
                    break;
                }
            }
            if (forbidden == 0) {
                if (p_building_data[i].prereq1 > BLD_PREREQ_TERRAIN_BASE) {
                    prereq = p_building_data[i].prereq1;
                    if (City_Terrain_Prereq(city, prereq) == 1
                        && p_cities[city].buildings[p_building_data[i].prereq2] != BLDSTATE_NONE
                        && p_cities[city].buildings[p_building_data[i].prereq2] != BLDSTATE_REMOVED) {
                        options[count] = i;
                        count = count + 1;
                    }
                } else if (p_cities[city].buildings[p_building_data[i].prereq1] != BLDSTATE_NONE
                           && p_cities[city].buildings[p_building_data[i].prereq1] != BLDSTATE_REMOVED
                           && p_cities[city].buildings[p_building_data[i].prereq2] != BLDSTATE_NONE
                           && p_cities[city].buildings[p_building_data[i].prereq2] != BLDSTATE_REMOVED) {
                    options[count] = i;
                    count = count + 1;
                }
            }
        }
    }
    *n_total = *n_total + 2;
    units = count - units;                      /* units: from here the buildings counted */
    if (units > 12) {
        units = units - 12;
        count = count - units;
    }
    *n_total = count;
    *n_options = count;
}

/* (city, prereq_code) -> 1 when the city meets a terrain prerequisite: 110 = a sea square in the 3 x 3 around it
   (rows from y - 1 clamped to 0, so a row-0 city tests rows 0-2; x wraps); any other code = a square of the city
   area whose Tile_Terrain_Prereq_Code matches. BUG #433: the 110 search's break leaves only the column loop */
int City_Terrain_Prereq(int city, int prereq_code)
{
    int y;
    int code;
    int x;
    int found;
    int tx;
    int ty;
    int area_tiles;
    int x0;
    int y0;
    int xs[25];
    int ys[25];
    register int i;

    found = 0;
    if (prereq_code == BLD_PREREQ_SEA) {
        y0 = p_cities[city].y - 1;
        if (y0 < 0)
            y0 = 0;
        x0 = p_cities[city].x - 1;
        if (x0 < 0)
            x0 = x0 + MAP_WIDTH;
        for (y = y0; y < y0 + 3; y++) {
            if (y < MAP_HEIGHT) {
                for (i = x0; i < x0 + 3; i++) {
                    if (i >= MAP_WIDTH)
                        x = i - MAP_WIDTH;
                    else
                        x = i;
                    if (Tile_Is_Sea(x, y, p_cities[city].plane) == 1) {
                        found = 1;
                        break;
                    }
                }
            }
        }
    } else {
        area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, p_cities[city].plane, xs, ys);
        for (i = 0; i < area_tiles && found == 0; i++) {
            tx = xs[i];
            ty = ys[i];
            code = Tile_Terrain_Prereq_Code(tx, ty, p_cities[city].plane);
            if (code == prereq_code)
                found = 1;
        }
    }
    return found;
}

/* (item, city) -> the production still needed for item (its cost minus the city's progress, at least 0): the
   city's producing is set to item for City_Production_Cost and restored */
int City_Remaining_Cost(int producing, int city)
{
    int saved;
    int cost;

    cost = 0;
    saved = p_cities[city].producing;
    p_cities[city].producing = producing;
    cost = City_Production_Cost(city);
    cost = cost - p_cities[city].production_progress;
    if (cost < 0)
        cost = 0;
    p_cities[city].producing = saved;
    return cost;
}

/* (cost, city) -> turns to produce cost at the city's production, rounded up, at least 1; with no production 1
   when the city's CURRENT product needs nothing more (the cost argument is not used then), else 999 */
int City_Turns_To_Complete(int cost, int city)
{
    int turns;

    turns = 0;
    if (p_cities[city].production_income > 0) {
        turns = cost / p_cities[city].production_income;
        if (cost % p_cities[city].production_income != 0)
            turns++;
        if (turns == 0)
            turns = 1;
    } else if (City_Remaining_Cost(p_cities[city].producing, city) == 0)
        turns = 1;
    else
        turns = 999;
    return turns;
}

/* (bldg, target) -> target, or -1: whether the city screen's race can ever build target - every building starts
   allowed, the race's forbidden ones are not, and 50 passes spread that to the buildings needing them; -1 also
   when both of target's prerequisites are built. bldg is not read */
int Building_Race_Reachable(int bldg, int target)
{
    int found;
    int race;
    int allowed[36];
    int i;
    int j;

    for (i = 0; i < NUM_BUILDINGS; i++)
        allowed[i] = 1;
    race = p_cities[city_screen_city].race;
    found = -1;
    if (p_cities[city_screen_city].buildings[p_building_data[target].prereq1] != BLDSTATE_BUILT
        || p_cities[city_screen_city].buildings[p_building_data[target].prereq2] != BLDSTATE_BUILT) {
        for (j = 0; j < race_table[race].forbidden_count; j++)
            allowed[race_table[race].forbidden_buildings[j]] = 0;
        for (i = 0; i < 50; i++) {
            for (j = 0; j < NUM_BUILDINGS; j++) {
                if (p_building_data[j].prereq1 > BLD_PREREQ_TERRAIN_BASE) {
                    if (allowed[p_building_data[j].prereq2] == 0)
                        allowed[j] = 0;
                } else {
                    if (allowed[p_building_data[j].prereq1] == 0)
                        allowed[j] = 0;
                    if (allowed[p_building_data[j].prereq2] == 0)
                        allowed[j] = 0;
                }
            }
        }
        if (allowed[target] == 1)
            found = target;
    }
    return found;
}
