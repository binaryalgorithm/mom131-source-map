/* CITY.C - WIZARDS.EXE overlay code segment 4a3f (stub 23fa): cities - the view of a city you do not own, creating,
   conquering, damaging and removing a city, building removal / selling, production prerequisites, buying, the city
   screen's resource rows, production boxes, enchantment list and citizens row, sold-building clean-up, gold / mana
   gains with their caps, and the resource breakdown scroll.
   This module's data (DS:2c9a..2d70): its literals; BSS (DS:bfbe..bff1): the breakdown scroll and city view state.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 4a3f src/wizards/CITY.C */
#include <string.h>

/* Called here with no prototype in scope (a prototype would reject the argument count):
     UI_Set_Redraw_Fn (Screen_City_Breakdown passes only the function; the other call passes the usual 2). */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_City_View(void);
void CityView_Draw(void);
void CityView_Alloc_Buffers(void);
int CityScr_Draw_Hotspot_Label(int control, int panel_x, int owner);
int Create_City(void);
void City_Change_Owner(int city, int new_owner);
void City_Collateral_Damage(int city, int pop_lost, int bldg_pct, int *destroyed);
void City_Remove(int city);
void City_Draw_Enchant_Names_2Col(int x, int y, int *slots, int *owners, int count);
void City_Clear_Enchantment(int city, int slot);
int City_Nth_Building(int city, int n);
int City_Try_Remove_Building(int bld, int city);
int City_Sell_Building(int bld, int city, int price);
int City_Can_Sell_Building(int bld, int city, int *needed_by);
int City_Production_Independent_Of(int city, int bld);
void City_Check_Production_Prereqs(int city);
void City_Production_Lost_Prereq(int city);
int Building_Sell_Price(int bld);
int Wizard_Can_Afford_Remaining(int player, int city);
void City_Buy(int player, int city);
int City_Buy_Price(int city);
void CityScr_Add_Resource_Controls(int city, int x, int y);
void CityScr_Draw_Resource_Rows(int city, int x, int y);
void Draw_Icon_Row_Fit(int *x, int y, int total, int amount, unsigned pic_ten, unsigned pic_one);
void CityScr_Draw_Production_Boxes(int city);
void City_Get_Enchant_List(int city, int *slots, int *owners, int *count);
void CityScr_Build_Enchant_List(void);
void CityScr_Draw_Enchant_List(int x, int y);
void CityScr_Add_Enchant_Controls(int x, int y);
void CityScr_Draw_Population_Row(int city, int x, int y);
void CityScr_Add_Citizen_Controls(int city, int x, int y);
void CityScr_Unit_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2);
void Turn_Finalize_Removed_Buildings(void);
void Wizard_Add_Gold(int player, int amount);
void Wizard_Add_Mana(int player, int amount);
int City_Enchant_Help_Id(int slot);
void Str_Append_Possessive(char *name);
void Screen_City_Breakdown(int breakdown);
void CityBreakdown_Draw(void);
int Draw_Icon_Row(int x, int y, int amount, unsigned pic_one, unsigned pic_ten);

#include "GAME.H"

#define BREAKDOWN_LINES 15              /* LISTDAT.LBX lines per resource */
/* the blocks: food 0, production 15, gold 30, power 45, research 60 */

int city_view_x;                                /* DS:bff0  the city view panel's left edge (60) */
int city_view_y;                                /* DS:bfee  its top (0) */
int cityview_list_count;                        /* DS:bfec  visible units in the viewed city */
int *cityview_unit_list;                        /* DS:bfea */
int *cityview_unit_ctls;                        /* DS:bfe8  their control ids */
int breakdown_kind;                             /* DS:bfe6  the clicked resource row (Breakdown: 0/1 food, 2 production, 3 upkeep (building maintenance), 4 gold, 5 power, 6 research) */
unsigned breakdown_top_pic;                     /* DS:bfe4  HELP.LBX 0 */
unsigned breakdown_bottom_pic;                  /* DS:bfe2  HELP.LBX 1 */
int breakdown_first;                            /* DS:bfe0  first building listed (0 / 17: the "More" page) */
int city_breakdown_lines;                       /* DS:bfde */
int *p_city_breakdown;                          /* DS:bfdc  the amount of each line (maintenance: the building ids) */
char *breakdown_lines[15];                      /* DS:bfbe  LISTDAT.LBX line texts */

extern char *cityscape_special_names[];         /* DS:0162  Summoning Circle .. Altar of Battle (hotspots 100..106); ReMoM _magic_building_names, clarified using ReMoM repo */
extern struct RaceInfo race_table[];            /* DS:1d86 */
extern char *city_enchant_names[];              /* DS:1ee4 */
extern char help_title_colors[];                /* DS:7118 */
extern char help_text_colors[];                 /* DS:711d */
extern struct CityscapeHotspot far *p_cityscape_hotspots;  /* DS:92b0 */
extern int cityscape_hotspot_count;             /* DS:92b4 */
extern unsigned cityview_bg_pic;                /* DS:949e  RELOAD.LBX 0x1a 'ENEMYCIT': the City View window background (ReMoM _enemy_city_seg; clarified using ReMoM repo) */
extern unsigned resource_x10_pics[];            /* DS:94a0 */
extern unsigned food_deficit_x10_pic;           /* DS:94ac */
extern unsigned city_rebel_pics[];              /* DS:94ae */
extern unsigned city_farmer_pics[];             /* DS:94ca */
extern unsigned resource_x1_pics[];             /* DS:94e6 */
extern unsigned food_deficit_x1_pic;           /* DS:94f0 */
extern unsigned city_worker_pics[];             /* DS:94f2 */
extern int city_hover_control;                  /* DS:9752 */
extern unsigned city_ench_down_arrow_pic;       /* DS:975a */
extern unsigned city_ench_up_arrow_pic;         /* DS:975c */
extern unsigned gold_deficit_x1_pic;                 /* DS:97a2 */
extern unsigned prod_box_filled_pic;                 /* DS:993a */
extern unsigned prod_box_empty_pic;                 /* DS:993c */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int city_breakdown_ctls[7];              /* DS:bf70 */
extern int *p_city_citizen_ctls;                 /* DS:bf7e */
extern int city_ench_down_button;                   /* DS:bf84 */
extern int city_ench_up_button;                   /* DS:bf86 */
extern int cityscape_hotspot_shown;             /* DS:bf8e */
extern int *city_hotspot_controls;              /* DS:bf90 */
extern int city_ench_shown;                  /* DS:bf92 */
extern int city_ench_scroll;                    /* DS:bf96 */
extern int city_ench_list_first;                /* DS:bf98 */
extern int city_ench_count;                     /* DS:bf9a */
extern int *p_city_ench_owners;                 /* DS:bf9c */
extern int *p_city_ench_slots;                  /* DS:bf9e */
extern int *p_city_ench_ctls;                   /* DS:bfa0 */
extern int city_garrison_y;                     /* DS:bfa2 */
extern int city_garrison_x;                     /* DS:bfa4 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
void far_strcpy(char *dst, int dst_seg, char far *src);
void Screen_Copy_Front(void);
void UI_Set_Redraw_Fn();                        /* unprototyped: see the note above */
void Help_Load_City_View(void);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
int Unit_Is_Invisible(int unit);
int Array_Delete_Near(int index, char *array, int record_size, int count);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void City_Recalculate(int city);
void UI_Input_Delay(int delay);
void CityScr_Add_Cityscape_Hotspots(void);
int UI_Add_Hotkey(char *key);
int UI_Hit_Test(void);
int Debug_Print_Labeled_Number(int col, int row, int value, char *label);
int Debug_Print_Number(int col, int row, int value);
void Play_Click_Sound(void);
int Confirm_Box(char *text);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Font_Select_Shadow_UL(int font, int c1, int c2, int c3);
void Set_Space_Width(int width);
void Set_Font_Color(int color);
int Print_Text_Centered(int x, int y, char *text);
void Print_Far_Text_Centered(int x, int y, char far *text);
int Tile_Visible(int x, int y, int plane);
void Draw_Unit_Figure(int x, int y, int unit, int icon_mode);
void Outline_Cycle_Tick(void);
void Cityscape_Draw(int city, int x, int y, int new_obj, int fade_obj);
int Wizard_Fortress_City(int player);
int Wizard_Summoning_City(int player);
void Wizard_Fortress_Captured(int loser, int winner, int city);
int Map_Explore_Radius(int x, int y, int plane, int range);
void Cities_Update_Road_Connections(void);
void Human_Cities_Area_Nop(void);
void City_Area_Bitmaps_Rebuild(void);
void Road_Remove(int x, int y, int plane);
int Array_Delete();                             /* unprototyped: called with one far pointer for its (offset, segment) words (UTIL.C) */
void City_Notices_Remove_City(int city);
int City_Remaining_Cost(int producing, int city);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int City_Production_Cost(int city);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Swap_Words(int *a, int *b);
int City_Min_Farmers(int city);
int City_Rebels(int city);
int City_Food_Breakdown(int city, int *out);
int City_Production_Breakdown(int city, int *out);
int City_Gold_Breakdown(int city, int *out);
int City_Power_Breakdown(int city, int *out);
int City_Research_Breakdown(int city, int *out);

/* the view of a city you do not own (from the map or a spell target): the cityscape, the visible units of the
   city (invisible ones dropped), the enchantment list with scroll arrows (your own enchantments can be turned off),
   the cityscape hotspots; 'X' prints debug numbers; any click on a unit / building / outside leaves
   Numbered: BUG #337 (BUGS_AND_QUIRKS.txt) */
void Screen_City_View(void)
{
    int leave;
    int redraw;
    int close;
    int hotkey;
    int ux;
    int uy;
    int k;
    int input;

    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Save_A800();
    Screen_Present();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(CityView_Draw, 1);
    city_view_x = 60;
    city_view_y = 0;
    UI_Clear_Help();
    Help_Load_City_View();
    CityView_Alloc_Buffers();
    Tile_Units_Of_Owner(p_cities[city_screen_city].x, p_cities[city_screen_city].y, p_cities[city_screen_city].plane,
                   p_cities[city_screen_city].owner, &cityview_list_count, cityview_unit_list);
    k = 0;
    while (k < cityview_list_count) {
        if (Unit_Is_Invisible(cityview_unit_list[k]) == 1) {
            Array_Delete_Near(k, (char *)cityview_unit_list, 2, cityview_list_count);
            cityview_list_count--;
        } else
            k++;
    }
    cityview_bg_pic = LBX_Reload("RELOAD", 0x1a, p_scratch);
    CityScr_Build_Enchant_List();
    City_Recalculate(city_screen_city);
    city_hover_control = -1;
    cityscape_hotspot_count = 0;
    UI_Clear_Controls();
    UI_Input_Delay(1);
    redraw = 0;
    leave = 0;
    while (leave == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        CityScr_Add_Enchant_Controls(city_view_x + 0x8b, city_view_y + 0x33);
        CityScr_Add_Cityscape_Hotspots();
        for (k = 0; k < cityview_list_count; k++) {
            ux = city_view_x + k % 6 * 22 + 5;
            uy = city_view_y + k / 6 * 20 + 0x34;
            cityview_unit_ctls[k] = UI_Add_Region(ux, uy, ux + 0x13, uy + 0x12, "", -1);
        }
        close = UI_Add_Region(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, "\x1b", -1);
        hotkey = UI_Add_Hotkey("X");
        input = UI_Get_Input();
        city_hover_control = UI_Hit_Test();
        if (input == hotkey) {
            Debug_Print_Labeled_Number(0, 0, city_screen_city, "city");
            Debug_Print_Labeled_Number(0, 1, p_cities[city_screen_city].x, "X");
            Debug_Print_Labeled_Number(0, 2, p_cities[city_screen_city].y, "Y");
            Debug_Print_Labeled_Number(0, 3, p_cities[city_screen_city].plane, "plane");
            for (k = 0; k < 20; k++)
                Debug_Print_Number(k, 4, p_cities[city_screen_city].buildings[k]);
            for (k = 20; k < NUM_BUILDINGS; k++)
                Debug_Print_Number(k - 20, 5, p_cities[city_screen_city].buildings[k]);
        }
        if (abs(input) == close) {   /* negative: a right click */
            Play_Click_Sound();
            leave = 1;
        }
        for (k = 0; k < cityview_list_count; k++)
            if (cityview_unit_ctls[k] == input) {
                Play_Click_Sound();
                leave = -1;
            }
        for (k = 0; k < cityscape_hotspot_shown; k++)
            if (city_hotspot_controls[k] == input) {
                Play_Click_Sound();
                leave = -1;
            }
        for (k = 0; k < city_ench_shown; k++) {
            if (p_city_ench_ctls[k] == input && p_city_ench_owners[city_ench_list_first + k] == current_player) {
                Play_Click_Sound();
                UI_Clear_Help();
                strcpy(p_text_buf, "Do you wish to turn off the \x02");
                strcat(p_text_buf, city_enchant_names[p_city_ench_slots[city_ench_list_first + k]]);
                strcat(p_text_buf, "\x01 spell?");
                if (Confirm_Box(p_text_buf) == 1) {
                    City_Clear_Enchantment(city_screen_city, p_city_ench_slots[city_ench_list_first + k]);
                    City_Get_Enchant_List(city_screen_city, p_city_ench_slots, p_city_ench_owners, &city_ench_count);
                    city_ench_scroll = 0;
                    if (city_ench_count > NUM_CITY_ENCH_ROWS)
                        city_ench_scroll = 1;
                    else
                        city_ench_list_first = 0;
                }
                UI_Suspend_Redraw_Fn();
                UI_Set_Redraw_Fn(CityView_Draw, 1);
                redraw = 1;
                Map_Invalidate();
                UI_Clear_Help();
                Help_Load_City_View();
                break;
            }
        }
        if (input == city_ench_up_button) {
            Play_Click_Sound();
            city_ench_list_first -= NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first < 0)
                city_ench_list_first = city_ench_count / NUM_CITY_ENCH_ROWS * NUM_CITY_ENCH_ROWS;
            redraw = 1;
        }
        if (input == city_ench_down_button) {
            Play_Click_Sound();
            city_ench_list_first += NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first >= city_ench_count)
                city_ench_list_first = 0;
            redraw = 1;
        }
        if (leave == 0 && redraw == 0) {
            Screen_Restore_A800();
            CityView_Draw();
            Screen_Present();
            Wait_Ticks(1);
        }
        redraw = 0;
    }
    present_mode = PRESENT_DISSOLVE;
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
    UI_Clear_Controls();
}

/* the redraw of Screen_City_View: help entries (race, the hovered building, the enchantments), the frame, "<size>
   of <name>", the race, the citizens, the units (if the square is visible, else "No Report"), the enchantments,
   the cityscape and the hovered hotspot's label */
void CityView_Draw(void)
{
    int unit_type;
    char colors[6];
    int ux;
    int uy;
    int k;

    help_entries[1].id = p_cities[city_screen_city].race + HELP_RACE_TOWNSFOLK;
    help_entries[2].id = HELP_CITY_ENCHANTMENTS;
    help_entries[3].id = HELP_CITYSCAPE_VIEW;
    for (k = 0; k < cityscape_hotspot_shown; k++) {
        if (city_hotspot_controls[k] == city_hover_control) {
            help_entries[3].id = p_cityscape_hotspots[k].building >= SCAPE_SUMMONING_CIRCLE ? p_cityscape_hotspots[k].building + HELP_CITYSCAPE_OBJECT_BASE :
                p_cityscape_hotspots[k].building + HELP_BUILDING;
            break;
        }
    }
    for (k = 0; k < 6; k++)
        help_entries[k + 5].id = -1;
    if (city_ench_shown > 0) {
        help_entries[2].id = -1;
        for (k = 0; k < city_ench_shown; k++)
            help_entries[k + 5].id = City_Enchant_Help_Id(p_city_ench_slots[city_ench_list_first + k]);
    }
    Reset_Clip_Rect();
    Set_Draw_Page();
    Draw_Pic_Frame(city_view_x - 1, city_view_y, cityview_bg_pic);
    Font_Select_Shadow(5, 5, 0, 0);
    Set_Space_Width(1);
    Set_Text_Shadow_Color(0);
    strcpy(p_text_buf, city_size_names[p_cities[city_screen_city].size]);
    strcat(p_text_buf, " of ");
    far_strcpy(p_text_buf2, 0, p_cities[city_screen_city].name);
    strcat(p_text_buf, p_text_buf2);
    Print_Text_Centered(city_view_x + 0x6a, city_view_y + 3, p_text_buf);
    colors[0] = 0xbe;
    colors[1] = 0xb3;
    Font_Select_Palette(0, colors);
    Set_Text_Shadow_Color(0x13);
    Font_Select_Shadow(1, 0xf, 0, 0);
    Print_Text(city_view_x + 6, city_view_y + 0x13, race_table[p_cities[city_screen_city].race].name);
    Print_Text(city_view_x + 6, 0x2b, "Units");
    CityScr_Draw_Population_Row(city_screen_city, city_view_x + 4, city_view_y + 0x1b);
    help_entries[4].id = HELP_CITY_GARRISONS;
    Set_Space_Width(1);
    if (Tile_Visible(p_cities[city_screen_city].x, p_cities[city_screen_city].y, p_cities[city_screen_city].plane) == 1) {
        for (k = 0; k < cityview_list_count; k++)
            if (cityview_unit_ctls[k] == city_hover_control) {
                unit_type = p_units[cityview_unit_list[k]].type;
                Print_Text(city_view_x + 0x24, 0x2b, unit_types[unit_type].name);
            }
        ux = city_view_x + 5;
        uy = city_view_y + 0x34;
        for (k = 0; k < cityview_list_count; k++)
            Draw_Unit_Figure(ux + k % 6 * 22, uy + k / 6 * 20, cityview_unit_list[k], 1);
        Outline_Cycle_Tick();
    } else {
        help_entries[4].id = HELP_NO_REPORT;
        Print_Text(city_view_x + 0x36, city_view_y + 0x40, "No Report");
    }
    CityScr_Draw_Enchant_List(city_view_x + 0x8b, city_view_y + 0x33);
    Cityscape_Draw(city_screen_city, city_view_x + 4, city_view_y + 0x65, 0, -1);
    CityScr_Draw_Hotspot_Label(city_hover_control, city_view_x, p_cities[city_screen_city].owner);
}

/* () the city screens' near buffers: two text buffers, the enchantment slot / owner / control lists (26 each), 72
   hotspot controls, 20 citizen controls, the 9 viewed units and their controls */
void CityView_Alloc_Buffers(void)
{
    int n;

    p_text_buf = Near_Alloc_Reset(0x50);
    p_text_buf2 = Near_Alloc(0x50);
    p_city_ench_slots = (int *)Near_Alloc(0x34);
    p_city_ench_owners = (int *)Near_Alloc(0x34);
    p_city_ench_ctls = (int *)Near_Alloc(0x34);
    n = 72;
    city_hotspot_controls = (int *)Near_Alloc(n * 2);
    p_city_citizen_ctls = (int *)Near_Alloc(0x28);
    cityview_unit_list = (int *)Near_Alloc(0x12);
    cityview_unit_ctls = (int *)Near_Alloc(0x12);
}

/* (control, panel x, owner) the label of the hovered cityscape hotspot: a special object's name (the fortress
   "<wizard>'s Fortress"), a building's name, "Destroyed" over a removed (sold or destroyed) one; always outlined, in font 1
   when the label lies inside the panel, else the small font 0. Returns 0 for no control (-1) */
int CityScr_Draw_Hotspot_Label(int control, int panel_x, int owner)
{
    int lx;
    int px;
    char owner_name[26];
    int k;

    if (control == -1)
        return 0;
    px = 0xaf;                                  /* the label band runs 0x26 .. 0xaf of the panel */
    if (panel_x == 1)               /* 1: the city screen's own panel (absolute label x) */
        lx = 0x26;
    else {
        lx = panel_x + 0x26;
        px = panel_x + 0xaf;
    }
    for (k = 0; k < cityscape_hotspot_shown; k++) {
        if (city_hotspot_controls[k] == control) {
            if (p_cityscape_hotspots[k].building >= SCAPE_SUMMONING_CIRCLE) {
                Font_Select_Outline(1, 0, 0, 0);
                if (p_cityscape_hotspots[k].label_x <= lx || p_cityscape_hotspots[k].label_x > px) {
                    Font_Select_Outline(0, 0, 0, 0);
                    Set_Space_Width(1);
                }
                Set_Font_Color(8);
                Set_Text_Shadow_Color(0);
                if (p_cityscape_hotspots[k].building == SCAPE_FORTRESS) {
                    strcpy(owner_name, wizards[owner].wizard_name);
                    Str_Append_Possessive(owner_name);
                    Print_Text_Centered(p_cityscape_hotspots[k].label_x + 1, p_cityscape_hotspots[k].label_y - 4,
                                        owner_name);
                    Print_Text_Centered(p_cityscape_hotspots[k].label_x + 1, p_cityscape_hotspots[k].label_y + 3,
                                        cityscape_special_names[p_cityscape_hotspots[k].building - SCAPE_SUMMONING_CIRCLE]);
                } else
                    Print_Text_Centered(p_cityscape_hotspots[k].label_x + 1, p_cityscape_hotspots[k].label_y,
                                        cityscape_special_names[p_cityscape_hotspots[k].building - SCAPE_SUMMONING_CIRCLE]);
            } else if (p_cities[city_screen_city].buildings[p_cityscape_hotspots[k].building] == BLDSTATE_REMOVED) {
                if (p_cityscape_hotspots[k].label_x <= lx || p_cityscape_hotspots[k].label_x > px) {
                    Font_Select_Outline(0, 2, 0, 0);
                    Set_Space_Width(1);
                } else
                    Font_Select_Outline(1, 2, 0, 0);
                Set_Font_Color(0xa6);
                Set_Text_Shadow_Color(0);
                Print_Text_Centered(p_cityscape_hotspots[k].label_x + 1, p_cityscape_hotspots[k].label_y, "Destroyed");
                Print_Far_Text_Centered(p_cityscape_hotspots[k].label_x + 1, p_cityscape_hotspots[k].label_y + 6,
                                        p_building_data[p_cityscape_hotspots[k].building].name);
            } else {
                if (p_cityscape_hotspots[k].label_x <= lx || p_cityscape_hotspots[k].label_x > px) {
                    Font_Select_Outline(0, 0, 0, 0);
                    Set_Space_Width(1);
                } else
                    Font_Select_Outline(1, 0, 0, 0);
                Set_Font_Color(8);
                Set_Text_Shadow_Color(0);
                Print_Far_Text_Centered(p_cityscape_hotspots[k].label_x + 1, p_cityscape_hotspots[k].label_y,
                                        p_building_data[p_cityscape_hotspots[k].building].name);
            }
        }
    }
}

/* () -> 1 with record num_cities initialised (owner none, no buildings but state 0 for 'none', Housing, no
   enchantments - Consecration cleared twice; the name, farmers, building upkeep, research income and road
   connections keep the slot's old values) and counted, 0 when 99 cities exist (so at most 99, not 100)
   Numbered: BUG #266 (BUGS_AND_QUIRKS.txt) */
int Create_City(void)
{
    int k;

    if (num_cities == (MAX_CITIES - 1))   /* the city table holds 100; no new city past 99 */
        return 0;
    p_cities[num_cities].race = 0;
    p_cities[num_cities].x = 0;
    p_cities[num_cities].y = 0;
    p_cities[num_cities].plane = 0;
    p_cities[num_cities].owner = PLAYER_NONE;
    p_cities[num_cities].size = 0;
    p_cities[num_cities].population = 0;
    p_cities[num_cities].sold_this_turn = 0;
    p_cities[num_cities].population_tens = 0;
    p_cities[num_cities].seen_by = 0;
    p_cities[num_cities].producing = PROD_HOUSING;
    p_cities[num_cities].building_count = 0;
    for (k = 0; k < NUM_BUILDINGS; k++)
        p_cities[num_cities].buildings[k] = BLDSTATE_NONE;
    p_cities[num_cities].buildings[BLD_NONE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_WALL_OF_FIRE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_CHAOS_RIFT] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_DARK_RITUALS] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_EVIL_PRESENCE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_CURSED_LANDS] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_PESTILENCE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_CLOUD_OF_SHADOW] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_FAMINE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_FLYING_FORTRESS] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_NATURE_WARD] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_SORCERY_WARD] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_CHAOS_WARD] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_LIFE_WARD] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_DEATH_WARD] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_NATURES_EYE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_EARTH_GATE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_STREAM_OF_LIFE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_GAIAS_BLESSING] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_INSPIRATIONS] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_PROSPERITY] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_ASTRAL_GATE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_HEAVENLY_LIGHT] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_CONSECRATION] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_WALL_OF_DARKNESS] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_NIGHTSHADE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_ALTAR_OF_BATTLE] = 0;
    p_cities[num_cities].enchants[CITY_ENCH_CONSECRATION] = 0;
    p_cities[num_cities].production_income = 0;
    p_cities[num_cities].production_progress = 0;
    p_cities[num_cities].gold_income = 0;
    p_cities[num_cities].power_income = 0;
    p_cities[num_cities].food_income = 0;
    num_cities++;
    return 1;
}

/* (city, new owner) the old owner's own enchantments in the city end (not curses cast by others, BUG #127); losing
   the fortress city or the last city -> Wizard_Fortress_Captured (not for a neutral old owner); losing the summoning city moves the circle to the
   fortress city; the current player explores radius 2 (no Oracle, #128); production Trade Goods, progress 0; the
   city, road connections and area bitmaps are recomputed
   Numbered: BUG #128, #227 (BUGS_AND_QUIRKS.txt) */
void City_Change_Owner(int city, int new_owner)
{
    int old_owner;
    char far *enchants;
    int fortress;
    int summoning;
    int cities;
    int k;

    old_owner = p_cities[city].owner;
    enchants = p_cities[city].enchants;
    for (k = 0; k < NUM_CITY_ENCHANTS; k++)
        if (enchants[k] > 0 && enchants[k] - 1 == old_owner)
            enchants[k] = 0;
    fortress = Wizard_Fortress_City(old_owner);
    summoning = Wizard_Summoning_City(old_owner);
    cities = 0;
    for (k = 0; k < num_cities && cities < 2; k++)
        if (p_cities[k].owner == old_owner)
            cities++;
    if (old_owner != PLAYER_NEUTRAL && (city == fortress || cities < 2))
        Wizard_Fortress_Captured(old_owner, new_owner, city);
    if (city == summoning && fortress != -1) {
        wizards[old_owner].summoning_circle_xyp[XYP_PLANE] = p_cities[fortress].plane;
        wizards[old_owner].summoning_circle_xyp[XYP_X] = p_cities[fortress].x;
        wizards[old_owner].summoning_circle_xyp[XYP_Y] = p_cities[fortress].y;
    }
    if (new_owner == current_player)
        Map_Explore_Radius(p_cities[city].x, p_cities[city].y, p_cities[city].plane, 2);
    p_cities[city].owner = new_owner;
    p_cities[city].producing = PROD_TRADE_GOODS;
    p_cities[city].production_progress = 0;
    p_cities[city].sold_this_turn = 0;
    City_Recalculate(city);
    Cities_Update_Road_Connections();
    Human_Cities_Area_Nop();
    City_Area_Bitmaps_Rebuild();
}

/* (city, population lost, building %, out destroyed[]) each building 1..35 in state >= 1 (standing, or already
   removed = 2, BUG #129) falls on Random(100) <= % if no standing or replaced building
   needs it (City_Try_Remove_Building; the caller gets no count: destroyed[] holds the fallen ones); population -= lost; size
   recomputed (max 5); production prerequisites rechecked; no population or size -> City_Remove
   Numbered: BUG #266 (BUGS_AND_QUIRKS.txt) */
void City_Collateral_Damage(int city, int pop_lost, int bldg_pct, int *destroyed)
{
    int count;
    int roll;
    int bld;

    if (bldg_pct > 0) {
        count = 0;
        for (bld = 1; bld < NUM_BUILDINGS; bld++) {
            if (p_cities[city].buildings[bld] >= 1) {
                roll = Random(100);
                if (roll <= bldg_pct && City_Try_Remove_Building(bld, city) == 1) {
                    destroyed[count] = bld;
                    count++;
                }
            }
        }
    }
    if (pop_lost > 0)
        p_cities[city].population = p_cities[city].population - pop_lost;
    p_cities[city].size = (p_cities[city].population + 3) / 4;
    if (p_cities[city].size > 5)
        p_cities[city].size = 5;
    City_Check_Production_Prereqs(city);
    if (p_cities[city].population < 1 || p_cities[city].size < 1) {
        p_cities[city].size = 0;
        City_Remove(city);
    }
}

/* (city) its road square removed (#125), the record deleted (later cities shift down), its notices dropped, the
   area bitmaps and road connections rebuilt */
void City_Remove(int city)
{
    int done;
    int city_index;

    done = 0;
    city_index = 0;
    Road_Remove(p_cities[city].x, p_cities[city].y, p_cities[city].plane);
    while (city_index < num_cities && done == 0) {
        if (city_index == city) {
            Array_Delete(city_index, p_cities, sizeof(struct City), num_cities);
            City_Notices_Remove_City(city);
            num_cities--;
            done = 1;
            City_Area_Bitmaps_Rebuild();
            Cities_Update_Road_Connections();
        } else
            city_index++;
    }
}

/* (x, y, slots, owners, count) enchantment names in two columns of 4, colored by the caster's banner (Nightshade
   owner 10: color 6) */
void City_Draw_Enchant_Names_2Col(int x, int y, int *slots, int *owners, int count)
{
    int unused;
    char palette[6];
    char colors[6];
    int lx;
    int ly;
    unsigned char banner;
    int j;
    int k;

    colors[0] = 0xdd;
    colors[1] = 0xda;
    colors[2] = 0x7d;
    colors[3] = 0xc9;
    colors[4] = 0xd3;
    colors[5] = 0x32;
    unused = 0;
    lx = x;
    for (k = 0; k < count; k++) {
        ly = y + k * 7;
        if (k > 3) {
            lx = x + 0x3f;
            ly = y + (k - 4) * 7;
        }
        if (owners[k] == ENCH_OWNER_NIGHTSHADE)
            for (j = 0; j < 6; j++)
                palette[j] = 6;
        else {
            banner = wizards[owners[k]].banner_color;
            for (j = 0; j < 6; j++)
                palette[j] = colors[banner];
        }
        Font_Select_Palette(1, palette);
        if (count > 4)
            Font_Select(0, 0xf, 0, 0);
        else
            Font_Select(1, 0xf, 0, 0);
        Set_Space_Width(1);
        Set_Font_Color(2);
        Print_Text(lx, ly, city_enchant_names[slots[k]]);
    }
    Set_Text_Shadow_Color(0);
}

/* (city, slot) the enchantment ends */
void City_Clear_Enchantment(int city, int slot)
{
    char far *enchants;

    enchants = p_cities[city].enchants;
    enchants[slot] = 0;
}

/* (city, n) -> the n-th building (from 0) in state 1 or more - built, or removed this turn - 0 if none */
int City_Nth_Building(int city, int n)
{
    int found;
    int count;
    int bld;

    found = 0;
    count = 0;
    for (bld = 1; bld < NUM_BUILDINGS; bld++) {
        if (p_cities[city].buildings[bld] >= 1) {
            if (count == n) {
                found = bld;
                break;
            }
            count++;
        }
    }
    return found;
}

/* (building, city) -> 0 when a building of the city (standing or replaced) needs it, else 1 with the building
   marked removed (2) */
int City_Try_Remove_Building(int bld, int city)
{
    int needed;
    int k;

    needed = 0;
    for (k = 1; k < NUM_BUILDINGS && needed == 0; k++)
        if (p_cities[city].buildings[k] == BLDSTATE_BUILT || p_cities[city].buildings[k] == BLDSTATE_REPLACED)
            if (p_building_data[k].prereq1 == bld || p_building_data[k].prereq2 == bld)
                needed = 1;
    if (needed == 1)
        return 0;
    p_cities[city].buildings[bld] = BLDSTATE_REMOVED;
    return 1;
}

/* (building, city, price) -> 1 with the building removed and the price paid to the owner, 0 if another needs it */
int City_Sell_Building(int bld, int city, int price)
{
    if (City_Try_Remove_Building(bld, city) == 1) {
        Wizard_Add_Gold(p_cities[city].owner, price);
        return 1;
    }
    return 0;
}

/* (building, city, out needed_by) -> 1 if the building can be sold; 0 if already sold (needed_by -1) or a building
   of the city needs it (needed_by = the first building from that one on that is not 'replaced') */
int City_Can_Sell_Building(int bld, int city, int *needed_by)
{
    register int j;
    register int ok;
    register int k;

    if (p_cities[city].buildings[bld] == BLDSTATE_REMOVED) {
        *needed_by = -1;
        ok = 0;
        return ok;
    }
    ok = 1;
    for (k = 1; k < NUM_BUILDINGS && ok == 1; k++) {
        if (p_cities[city].buildings[k] == BLDSTATE_BUILT || p_cities[city].buildings[k] == BLDSTATE_REPLACED) {
            if (p_building_data[k].prereq1 == bld || p_building_data[k].prereq2 == bld) {
                for (j = k; j < NUM_BUILDINGS && p_cities[city].buildings[j] == BLDSTATE_REPLACED; j++)
                    ;
                if (j < NUM_BUILDINGS) {
                    *needed_by = j;
                    ok = 0;
                }
            }
        }
    }
    return ok;
}

/* (city, building) -> 0 if the city's current production requires the building, else 1 */
int City_Production_Independent_Of(int city, int bld)
{
    int prod;

    prod = p_cities[city].producing;
    if (prod >= PROD_UNIT_BASE) {
        prod -= PROD_UNIT_BASE;
        if (unit_types[prod].req_building_1 == bld || unit_types[prod].req_building_2 == bld)
            return 0;
    } else if (p_building_data[prod].prereq1 == bld || p_building_data[prod].prereq2 == bld)
        return 0;
    return 1;
}

/* (city) the current production's required buildings (unit: both; building: prerequisite 2 alone when
   prerequisite 1 is above 100, else both) must stand or be replaced, else City_Production_Lost_Prereq */
void City_Check_Production_Prereqs(int city)
{
    int prod;

    prod = p_cities[city].producing;
    if (prod >= PROD_UNIT_BASE) {
        prod -= PROD_UNIT_BASE;
        if ((p_cities[city].buildings[unit_types[prod].req_building_1] == BLDSTATE_BUILT ||
             p_cities[city].buildings[unit_types[prod].req_building_1] == BLDSTATE_REPLACED) &&
            (p_cities[city].buildings[unit_types[prod].req_building_2] == BLDSTATE_BUILT ||
             p_cities[city].buildings[unit_types[prod].req_building_2] == BLDSTATE_REPLACED))
            ;
        else
            City_Production_Lost_Prereq(city);
    } else if (p_building_data[prod].prereq1 > BLD_PREREQ_TERRAIN_BASE) {
        if (p_cities[city].buildings[p_building_data[prod].prereq2] == BLDSTATE_BUILT ||
            p_cities[city].buildings[p_building_data[prod].prereq2] == BLDSTATE_REPLACED)
            ;
        else
            City_Production_Lost_Prereq(city);
    } else {
        if ((p_cities[city].buildings[p_building_data[prod].prereq1] == BLDSTATE_BUILT ||
             p_cities[city].buildings[p_building_data[prod].prereq1] == BLDSTATE_REPLACED) &&
            (p_cities[city].buildings[p_building_data[prod].prereq2] == BLDSTATE_BUILT ||
             p_cities[city].buildings[p_building_data[prod].prereq2] == BLDSTATE_REPLACED))
            ;
        else
            City_Production_Lost_Prereq(city);
    }
}

/* (city) the current player's city queues a notice (production negated: "can no longer produce"), Housing when
   the queue is full; any other city goes to auto-choose */
void City_Production_Lost_Prereq(int city)
{
    if (p_cities[city].owner == current_player) {
        if (city_notice_count < MAX_REPORT_ENTRIES) {
            city_notices[city_notice_count].city = city;
            city_notices[city_notice_count].producing = -p_cities[city].producing;
            city_notice_count = city_notice_count + 1;
        } else
            p_cities[city].producing = PROD_HOUSING;
    } else
        p_cities[city].producing = PROD_AUTO_CHOOSE;
}

/* (building) -> its sale price: cost / 3 (City Walls / 6), at least 1 */
int Building_Sell_Price(int bld)
{
    int price;

    price = p_building_data[bld].cost / 3;
    if (bld == BLD_CITY_WALLS)
        price = price / 2;
    if (price < 1)
        price = 1;
    return price;
}

/* (player, city) -> 1 if the treasury covers the rest of the city's production cost (unused) */
int Wizard_Can_Afford_Remaining(int player, int city)
{
    int cost;
    int gold;
    int ok;

    cost = City_Remaining_Cost(p_cities[city].producing, city);
    gold = wizards[player].gold;
    if (cost <= gold)
        ok = 1;
    else
        ok = 0;
    return ok;
}

/* (player, city) pays the buy price and completes the production */
void City_Buy(int player, int city)
{
    int price;

    price = City_Buy_Price(city);
    wizards[player].gold -= price;
    p_cities[city].production_progress += City_Remaining_Cost(p_cities[city].producing, city);
}

/* (city) -> the buy price: the remaining cost x 4 with no progress, x 2 from half the remaining cost on, else x 3 */
int City_Buy_Price(int city)
{
    int price;

    price = City_Remaining_Cost(p_cities[city].producing, city);
    if (p_cities[city].production_progress < 1)
        price = price * 4;
    else if (p_cities[city].production_progress >= price / 2)
        price = price * 2;
    else
        price = price * 3;
    return price;
}

/* (city, x, y) the city screen's resource rows (food, production, gold, power, research) as icon rows, each with
   its click controls (city_breakdown_ctls: food needed / surplus, production, upkeep / net gold, power, research) */
void CityScr_Add_Resource_Controls(int city, int x, int y)
{
    int x1;                                     /* also the city_breakdown_ctls index (2 / 5 / 6) in the default branch */
    int food_net;
    int gold_net;                   /* computed, never read */
    int totals[5];
    int x2;
    int amount;
    int k;

    food_net = abs(p_cities[city].food_income - p_cities[city].population);
    gold_net = abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
    totals[0] = p_cities[city].food_income + 1;   /* + 1: one more than the food row's parts sum to (the gold total has none) */
    totals[1] = p_cities[city].production_income;
    totals[3] = p_cities[city].power_income;
    totals[4] = p_cities[city].research_income;
    if (p_cities[city].gold_income >= p_cities[city].building_upkeep)
        totals[2] = p_cities[city].building_upkeep + abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
    else
        totals[2] = p_cities[city].gold_income + abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
    for (k = 0; k < 5; k++) {
        switch (k) {
        case 0:
            if (p_cities[city].population >= p_cities[city].food_income)
                amount = p_cities[city].food_income;
            else
                amount = p_cities[city].population;
            x2 = x;
            Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[k], resource_x1_pics[k]);
            if (amount > 0)
                x2 = x2 + 6;
            city_breakdown_ctls[BREAKDOWN_FOOD_EATEN] = UI_Add_Region(x, y + k * 8, x2, y + k * 8 + 7, "", -1);
            x1 = x2;
            if (p_cities[city].food_income >= p_cities[city].population)
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], food_net, resource_x10_pics[k], resource_x1_pics[k]);
            else
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], food_net, food_deficit_x10_pic, food_deficit_x1_pic);
            city_breakdown_ctls[BREAKDOWN_FOOD_SURPLUS] = UI_Add_Region(x1, y + k * 8, 0x84, y + k * 8 + 7, "", -1);
            break;
        case 2:
            x2 = x;
            amount = p_cities[city].gold_income >= p_cities[city].building_upkeep ? p_cities[city].building_upkeep :
                p_cities[city].gold_income;
            Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[k], resource_x1_pics[k]);
            if (amount > 0)
                x2 = x2 + 5;
            city_breakdown_ctls[BREAKDOWN_UPKEEP] = UI_Add_Region(x, y + k * 8, x2, y + k * 8 + 7, "", -1);
            amount = abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
            x1 = x2;
            if (p_cities[city].gold_income >= p_cities[city].building_upkeep)
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[k], resource_x1_pics[k]);
            else
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[5], gold_deficit_x1_pic);   /* [5]: the
                                                   gold-deficit x10 picture (the food one is food_deficit_x10_pic) */
            city_breakdown_ctls[BREAKDOWN_GOLD_SURPLUS] = UI_Add_Region(x1, y + k * 8, 0x84, y + k * 8 + 7, "", -1);
            break;
        default:
            x2 = x;
            Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], totals[k], resource_x10_pics[k], resource_x1_pics[k]);
            switch (k) {
            case 1:
                x1 = BREAKDOWN_PRODUCTION;
                break;
            case 3:
                x1 = BREAKDOWN_POWER;
                break;
            case 4:
                x1 = BREAKDOWN_RESEARCH;
                break;
            }
            city_breakdown_ctls[x1] = UI_Add_Region(x, y + k * 8, 0x84, y + k * 8 + 7, "", -1);
            break;
        }
    }
}

/* (city, x, y) the resource rows of CityScr_Add_Resource_Controls without the controls */
void CityScr_Draw_Resource_Rows(int city, int x, int y)
{
    int food_net;
    int gold_net;                   /* computed, never read */
    int totals[5];
    int x2;
    int amount;
    int k;

    food_net = abs(p_cities[city].food_income - p_cities[city].population);
    gold_net = abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
    totals[0] = p_cities[city].food_income + 1;   /* + 1: one more than the food row's parts sum to (the gold total has none) */
    totals[1] = p_cities[city].production_income;
    totals[3] = p_cities[city].power_income;
    totals[4] = p_cities[city].research_income;
    if (p_cities[city].gold_income >= p_cities[city].building_upkeep)
        totals[2] = p_cities[city].building_upkeep + abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
    else
        totals[2] = p_cities[city].gold_income + abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
    for (k = 0; k < 5; k++) {
        switch (k) {
        case 0:
            if (p_cities[city].population >= p_cities[city].food_income)
                amount = p_cities[city].food_income;
            else
                amount = p_cities[city].population;
            x2 = x;
            Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[k], resource_x1_pics[k]);
            if (amount > 0)
                x2 = x2 + 6;
            if (p_cities[city].food_income >= p_cities[city].population)
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], food_net, resource_x10_pics[k], resource_x1_pics[k]);
            else
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], food_net, food_deficit_x10_pic, food_deficit_x1_pic);
            break;
        case 2:
            x2 = x;
            amount = p_cities[city].gold_income >= p_cities[city].building_upkeep ? p_cities[city].building_upkeep :
                p_cities[city].gold_income;
            Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[k], resource_x1_pics[k]);
            if (amount > 0)
                x2 = x2 + 5;
            amount = abs(p_cities[city].gold_income - p_cities[city].building_upkeep);
            if (p_cities[city].gold_income >= p_cities[city].building_upkeep)
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[k], resource_x1_pics[k]);
            else
                Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], amount, resource_x10_pics[5], gold_deficit_x1_pic);   /* [5]: the
                                                   gold-deficit x10 picture (the food one is food_deficit_x10_pic) */
            break;
        default:
            x2 = x;
            Draw_Icon_Row_Fit(&x2, y + k * 8, totals[k], totals[k], resource_x10_pics[k], resource_x1_pics[k]);
            break;
        }
    }
}

/* (&x, y, total, amount, ten picture, one picture - the tens first, unlike Draw_Icon_Row) draws amount as tens and ones, the spacing shrunk so that the
   row of 'total' fits (tens: 80 px, ones: 125 px less the tens); x moves past the row */
void Draw_Icon_Row_Fit(int *x, int y, int total, int amount, unsigned pic_ten, unsigned pic_one)
{
    int ix;
    int done;
    int n;
    int k;
    int used;
    int pad;
    int spacing;

    ix = *x;
    pad = 0;
    used = 0;
    if (total / 10 > 0) {
        spacing = 14;
        done = 0;
        n = total / 10;
        while (done == 0) {
            if (n * spacing > 79)
                spacing--;
            else
                done = 1;
            if (spacing < 4)
                done = 1;
        }
        for (k = 0; k < amount / 10; k++)
            Draw_Pic_Frame(ix + k * spacing, y, pic_ten);
        if (amount / 10 > 0) {
            ix = ix + amount / 10 * spacing + 1;
            pad = 0;
        }
        n = amount / 10 * 10;
        total = total - n;
        amount = amount - n;
        used = ix - *x;
    }
    if (total > 0) {
        spacing = 8;
        done = 0;
        n = total;
        while (done == 0) {
            if (n * spacing > 0x7d - used)
                spacing--;
            else
                done = 1;
            if (spacing < 3)
                done = 1;
        }
        for (k = 0; k < amount; k++)
            Draw_Pic_Frame(ix + k * spacing, y, pic_one);
        if (amount > 0) {
            ix = ix + amount * spacing;
            pad = 8 - spacing;
        }
    }
    *x = ix + pad;
}

/* (city) the production boxes: cost / 10 boxes (10 a row, 20 narrow ones above 50), progress / 10 filled, the
   next one half-filled by the rest */
void CityScr_Draw_Production_Boxes(int city)
{
    int x0;
    int y0;
    int box_w;
    int y;
    int boxes;
    int filled;
    int partial;
    int points_per_box;
    int per_row;
    register int k;
    register int x;

    x0 = 0x106;
    y0 = 0x97;
    box_w = 5;
    points_per_box = 10;
    per_row = 10;
    filled = p_cities[city].production_progress / points_per_box;
    partial = p_cities[city].production_progress % points_per_box;
    boxes = City_Production_Cost(city) / points_per_box;
    if (boxes == 0)
        boxes = 1;
    if (boxes > 50) {
        box_w = 2;
        per_row = 20;
    }
    for (k = 0; k < boxes; k++) {
        x = x0 + k % per_row * box_w;
        y = y0 + k / per_row * 6;
        if (k < filled)
            Draw_Pic_Frame(x, y, prod_box_filled_pic);
        else {
            Draw_Pic_Frame(x, y, prod_box_empty_pic);
            if (k == filled && partial > 0) {
                partial = partial / 2;
                Set_Clip_Rect(0, 0, x + partial, SCREEN_MAX_Y);
                Draw_Pic_Clipped(x, y, prod_box_filled_pic);
                Reset_Clip_Rect();
            }
        }
    }
}

/* (city, out slots, out owners, out count) the city's enchantments with their casters (Nightshade: 10), sorted by
   caster, highest first */
void City_Get_Enchant_List(int city, int *slots, int *owners, int *count)
{
    int pass;
    int owner;
    char far *enchants;
    int n;
    int k;

    owner = p_cities[city].owner;               /* never read */
    enchants = p_cities[city].enchants;
    n = 0;
    for (k = 0; k < NUM_CITY_ENCHANTS; k++) {
        if (enchants[k] > 0) {
            slots[n] = k;
            if (stricmp(city_enchant_names[k], "Nightshade") == 0)   /* = CITY_ENCH_NIGHTSHADE */
                owners[n] = ENCH_OWNER_NIGHTSHADE;
            else
                owners[n] = enchants[k] - 1;
            n++;
        }
    }
    *count = n;
    for (pass = 0; pass < n; pass++)
        for (k = 0; k < n - 1; k++)
            if (owners[k] < owners[k + 1]) {
                Swap_Words(&owners[k], &owners[k + 1]);
                Swap_Words(&slots[k], &slots[k + 1]);
            }
}

/* () the enchantment list of the screen's city, scrolled to the top (arrows from 7 on) */
void CityScr_Build_Enchant_List(void)
{
    City_Get_Enchant_List(city_screen_city, p_city_ench_slots, p_city_ench_owners, &city_ench_count);
    city_ench_list_first = 0;
    city_ench_scroll = 0;
    if (city_ench_count > NUM_CITY_ENCH_ROWS)
        city_ench_scroll = 1;
}

/* (x, y) the shown enchantments, colored by the caster's banner (Nightshade: color 1); it also sets
   wizards[PLAYER_NEUTRAL].banner_color to BANNER_NEUTRAL (game state written by a draw) */
void CityScr_Draw_Enchant_List(int x, int y)
{
    char palette[6];
    char colors[6];
    unsigned char banner;
    int j;
    int k;

    colors[0] = 0xde;
    colors[1] = 0x4f;
    colors[2] = 0x7f;
    colors[3] = 0xc9;
    colors[4] = 0xd3;
    colors[5] = 0x32;
    for (k = 0; k < city_ench_shown; k++) {
        if (p_city_ench_owners[k + city_ench_list_first] == ENCH_OWNER_NIGHTSHADE)
            for (j = 0; j < 6; j++)
                palette[j] = 1;
        else {
            if (p_city_ench_owners[k + city_ench_list_first] == PLAYER_NEUTRAL)
                wizards[PLAYER_NEUTRAL].banner_color = BANNER_NEUTRAL;
            banner = wizards[p_city_ench_owners[k + city_ench_list_first]].banner_color;
            for (j = 0; j < 6; j++)
                palette[j] = colors[banner];
        }
        Font_Select_Palette(0, palette);
        Font_Select_Shadow_UL(1, 0xf, 0, 0);
        Set_Text_Shadow_Color(6);
        Set_Font_Color(7);
        Set_Space_Width(1);
        Print_Text(x, y + k * 7, city_enchant_names[p_city_ench_slots[k + city_ench_list_first]]);
    }
}

/* (x, y) the scroll arrows (when the list scrolls) and a control per shown enchantment (at most 6)
   Numbered: BUG #337 (BUGS_AND_QUIRKS.txt) */
void CityScr_Add_Enchant_Controls(int x, int y)
{
    int k;

    city_ench_up_button = UI_NO_CONTROL;
    city_ench_down_button = UI_NO_CONTROL;
    if (city_ench_scroll == 1) {
        city_ench_up_button = UI_Add_Button(x + 0x3e, y - 1, "", city_ench_up_arrow_pic, "", -1);
        city_ench_down_button = UI_Add_Button(x + 0x3e, y + 0x22, "", city_ench_down_arrow_pic, "", -1);
    }
    city_ench_shown = 0;
    city_ench_shown = city_ench_count - city_ench_list_first;
    if (city_ench_shown > NUM_CITY_ENCH_ROWS)
        city_ench_shown = NUM_CITY_ENCH_ROWS;
    if (city_ench_shown <= 0) {
        city_ench_list_first = 0;
        city_ench_shown = city_ench_count - city_ench_list_first;
        if (city_ench_shown > NUM_CITY_ENCH_ROWS)
            city_ench_shown = NUM_CITY_ENCH_ROWS;
    }
    for (k = 0; k < city_ench_shown; k++)
        p_city_ench_ctls[k] = UI_Add_Region(x, y + k * 7, x + 0x3c, k * 7 + y + 6, "", -1);
}

/* (city, x, y) the farmers (clamped to the minimum .. population - rebels, written back), workers and rebels, with
   a 4 px gap after the minimum farmers (or after the farmers; none for a one-citizen city) and before the rebels
   (drawn 2 px higher); 9 px apart, 8 from 20 citizens */
void CityScr_Draw_Population_Row(int city, int x, int y)
{
    int k;
    int farmers;
    int min_farmers;
    int step;
    int gapped;
    int rebels;
    int px;

    min_farmers = City_Min_Farmers(city);
    rebels = City_Rebels(city);
    if (p_cities[city].farmers < min_farmers)
        p_cities[city].farmers = min_farmers;
    if (p_cities[city].farmers > p_cities[city].population - rebels)
        p_cities[city].farmers = p_cities[city].population - rebels;
    farmers = p_cities[city].farmers;
    step = 9;
    if (p_cities[city].population > 19)
        step = 8;
    px = 0;
    gapped = 0;
    if (p_cities[city].population == 1)
        gapped = 1;
    for (k = 0; k < farmers; k++) {
        if (k == min_farmers && gapped == 0) {
            px += 4;
            gapped = 1;
        }
        Draw_Pic_Frame(x + px, y, city_farmer_pics[p_cities[city].race]);
        px += step;
    }
    if (gapped == 0)
        px += 4;
    for (k = 0; k < p_cities[city].population - farmers - rebels; k++) {
        Draw_Pic_Frame(x + px, y, city_worker_pics[p_cities[city].race]);
        px += step;
    }
    px += 4;
    for (k = 0; k < rebels; k++) {
        Draw_Pic_Frame(x + px, y - 2, city_rebel_pics[p_cities[city].race]);
        px += step;
    }
}

/* (city, x, y) clamps the farmers as CityScr_Draw_Population_Row does, then a 10 x 15 control per non-rebel
   citizen with the same spacing */
void CityScr_Add_Citizen_Controls(int city, int x, int y)
{
    int k;
    int x1;
    int y1;
    int x2;
    int y2;
    int min_farmers;
    int step;
    int gapped;
    int farmers;
    int rebels;
    int px;

    min_farmers = City_Min_Farmers(city);
    rebels = City_Rebels(city);
    if (p_cities[city].farmers < min_farmers)
        p_cities[city].farmers = min_farmers;
    if (p_cities[city].farmers > p_cities[city].population - rebels)
        p_cities[city].farmers = p_cities[city].population - rebels;
    step = 9;
    if (p_cities[city].population > 19)
        step = 8;
    px = 0;
    farmers = p_cities[city].farmers;
    gapped = 0;
    if (p_cities[city].population == 1)
        gapped = 1;
    for (k = 0; k < p_cities[city].population - rebels; k++) {
        if (k == min_farmers && gapped == 0) {
            px += 4;
            gapped = 1;
        }
        if (k == farmers && gapped == 0) {
            px += 4;
            gapped = 1;
        }
        x1 = x + px;
        y1 = y;
        x2 = x1 + 9;
        y2 = y1 + 0xe;
        p_city_citizen_ctls[k] = UI_Add_Region(x1, y1, x2, y2, "", -1);
        px += step;
    }
}

/* (i, &x1, &y1, &x2, &y2) the garrison slot rectangle: 5 a row, 20 x 17 apart */
void CityScr_Unit_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2)
{
    *x1 = city_garrison_x + i % 5 * 20;
    *y1 = city_garrison_y + i / 5 * 17;
    *x2 = *x1 + 0x11;
    *y2 = *y1 + 0xf;
}

/* end of turn: buildings removed this turn (2: sold, or destroyed by City_Collateral_Damage) become 'none' and their replaced predecessor stands again; the current
   player's cities may sell again */
void Turn_Finalize_Removed_Buildings(void)
{
    int city;
    int bld;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == current_player)
            p_cities[city].sold_this_turn = 0;
        for (bld = 0; bld < NUM_BUILDINGS; bld++) {
            if (p_cities[city].buildings[bld] == BLDSTATE_REMOVED) {
                if (p_building_data[bld].replaces != BLD_NO_BUILDING)
                    p_cities[city].buildings[p_building_data[bld].replaces] = BLDSTATE_BUILT;
                p_cities[city].buildings[bld] = BLDSTATE_NONE;
            }
        }
    }
}

/* (player, amount) gold += amount, at most 30000 (from 30000 on only a loss applies) */
void Wizard_Add_Gold(int player, int amount)
{
    if (wizards[player].gold >= RESOURCE_CAP) {
        if (amount < 0)
            wizards[player].gold += amount;
        else
            wizards[player].gold = RESOURCE_CAP;
    } else if (RESOURCE_CAP - wizards[player].gold < amount)
        wizards[player].gold = RESOURCE_CAP;
    else
        wizards[player].gold += amount;
}

/* (player, amount) mana += amount, at most 30000 (from 30000 on only a loss applies) */
void Wizard_Add_Mana(int player, int amount)
{
    if (wizards[player].mana_reserve >= RESOURCE_CAP) {
        if (amount < 0)
            wizards[player].mana_reserve += amount;
        else
            wizards[player].mana_reserve = RESOURCE_CAP;
    } else if (RESOURCE_CAP - wizards[player].mana_reserve < amount)
        wizards[player].mana_reserve = RESOURCE_CAP;
    else
        wizards[player].mana_reserve += amount;
}

/* (slot) -> the help entry of a city enchantment, -1 for none */
int City_Enchant_Help_Id(int slot)
{
    switch (slot) {
    case CITY_ENCH_WALL_OF_FIRE:
        return HELP_WALL_OF_FIRE;
    case CITY_ENCH_CHAOS_RIFT:
        return HELP_CHAOS_RIFT;
    case CITY_ENCH_DARK_RITUALS:
        return HELP_DARK_RITUALS;
    case CITY_ENCH_EVIL_PRESENCE:
        return HELP_EVIL_PRESENCE;
    case CITY_ENCH_CURSED_LANDS:
        return HELP_CURSED_LANDS;
    case CITY_ENCH_PESTILENCE:
        return HELP_PESTILENCE;
    case CITY_ENCH_CLOUD_OF_SHADOW:
        return HELP_CLOUD_OF_SHADOW;
    case CITY_ENCH_FAMINE:
        return HELP_FAMINE;
    case CITY_ENCH_FLYING_FORTRESS:
        return HELP_FLYING_FORTRESS;
    case CITY_ENCH_NATURE_WARD:
        return HELP_SPELL_WARD;
    case CITY_ENCH_SORCERY_WARD:
        return HELP_SPELL_WARD;
    case CITY_ENCH_CHAOS_WARD:
        return HELP_SPELL_WARD;
    case CITY_ENCH_LIFE_WARD:
        return HELP_SPELL_WARD;
    case CITY_ENCH_DEATH_WARD:
        return HELP_SPELL_WARD;
    case CITY_ENCH_NATURES_EYE:
        return HELP_NATURES_EYE;
    case CITY_ENCH_EARTH_GATE:
        return HELP_EARTH_GATE;
    case CITY_ENCH_STREAM_OF_LIFE:
        return HELP_STREAM_OF_LIFE;
    case CITY_ENCH_GAIAS_BLESSING:
        return HELP_GAIAS_BLESSING;
    case CITY_ENCH_INSPIRATIONS:
        return HELP_INSPIRATIONS;
    case CITY_ENCH_PROSPERITY:
        return HELP_PROSPERITY;
    case CITY_ENCH_ASTRAL_GATE:
        return HELP_ASTRAL_GATE;
    case CITY_ENCH_HEAVENLY_LIGHT:
        return HELP_HEAVENLY_LIGHT;
    case CITY_ENCH_CONSECRATION:
        return HELP_CONSECRATION;
    case CITY_ENCH_WALL_OF_DARKNESS:
        return HELP_WALL_OF_DARKNESS;
    case CITY_ENCH_ALTAR_OF_BATTLE:
        return HELP_ALTAR_OF_BATTLE;
    case CITY_ENCH_NIGHTSHADE:
        return HELP_NIGHTSHADE;
    default:
        return -1;
    }
}

/* (name) appends "'" to a name ending in s / S, else "'s" */
void Str_Append_Possessive(char *name)
{
    int len;

    len = strlen(name);
    strcat(name, name[len - 1] == 's' || name[len - 1] == 'S' ? "'" : "'s");
}

/* (clicked resource row) the scroll itemising a city resource: food (0, 1), production (2), building maintenance
   (3, with a "More" page from 18 buildings on), gold (4), power (5), research (6)
   Numbered: BUG #372 (BUGS_AND_QUIRKS.txt) */
void Screen_City_Breakdown(int breakdown)
{
    int done;
    int input;
    int close;
    int more_ctl;
    int more;
    int k;
    int first;

    breakdown_kind = breakdown;
    p_text_buf = Near_Alloc_Reset(0x50);
    p_text_buf2 = Near_Alloc(0x50);
    p_city_breakdown = (int *)Near_Alloc(100);
    for (k = 0; k < BREAKDOWN_LINES; k++)
        breakdown_lines[k] = Near_Alloc(0x32);
    breakdown_top_pic = LBX_Reload("HELP", 0, p_scratch);
    breakdown_bottom_pic = LBX_Reload_Next("HELP", 1, p_scratch);
    UI_Clear_Controls();
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(CityBreakdown_Draw);
    Screen_Copy_Front();
    Screen_Save_A800();
    more = 0;
    breakdown_first = 0;
    city_breakdown_lines = 0;
    switch (breakdown_kind) {
    case BREAKDOWN_FOOD_EATEN:
    case BREAKDOWN_FOOD_SURPLUS:
        first = 0;
        for (k = 0; k < BREAKDOWN_LINES; k++) {
            LBX_Load_Records_Near("LISTDAT", 0, p_text_buf, first + k, 1, 200);
            strcpy(breakdown_lines[k], p_text_buf);
        }
        City_Food_Breakdown(city_screen_city, p_city_breakdown);
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0)
                city_breakdown_lines++;
        break;
    case BREAKDOWN_PRODUCTION:
        first = BREAKDOWN_LINES;
        for (k = 0; k < BREAKDOWN_LINES; k++) {
            LBX_Load_Records_Near("LISTDAT", 0, p_text_buf, first + k, 1, 200);
            strcpy(breakdown_lines[k], p_text_buf);
        }
        City_Production_Breakdown(city_screen_city, p_city_breakdown);
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0)
                city_breakdown_lines++;
        break;
    case BREAKDOWN_GOLD_SURPLUS:
        first = 2 * BREAKDOWN_LINES;
        for (k = 0; k < BREAKDOWN_LINES; k++) {
            LBX_Load_Records_Near("LISTDAT", 0, p_text_buf, first + k, 1, 200);
            strcpy(breakdown_lines[k], p_text_buf);
        }
        City_Gold_Breakdown(city_screen_city, p_city_breakdown);
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0)
                city_breakdown_lines++;
        break;
    case BREAKDOWN_POWER:
        first = 3 * BREAKDOWN_LINES;
        for (k = 0; k < BREAKDOWN_LINES; k++) {
            LBX_Load_Records_Near("LISTDAT", 0, p_text_buf, first + k, 1, 200);
            strcpy(breakdown_lines[k], p_text_buf);
        }
        City_Power_Breakdown(city_screen_city, p_city_breakdown);
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0)
                city_breakdown_lines++;
        break;
    case BREAKDOWN_RESEARCH:
        first = 4 * BREAKDOWN_LINES;
        for (k = 0; k < BREAKDOWN_LINES; k++) {
            LBX_Load_Records_Near("LISTDAT", 0, p_text_buf, first + k, 1, 200);
            strcpy(breakdown_lines[k], p_text_buf);
        }
        City_Research_Breakdown(city_screen_city, p_city_breakdown);
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0)
                city_breakdown_lines++;
        break;
    }
    if (breakdown_kind == BREAKDOWN_UPKEEP) {
        city_breakdown_lines = 0;
        for (k = 1; k < NUM_BUILDINGS; k++)
            if (p_cities[city_screen_city].buildings[k] == BLDSTATE_BUILT || p_cities[city_screen_city].buildings[k] == BLDSTATE_REPLACED) {
                p_city_breakdown[city_breakdown_lines] = k;
                city_breakdown_lines++;
            }
        if (city_breakdown_lines > BREAKDOWN_PAGE_LINES)
            more = 1;
    }
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        more_ctl = UI_NO_CONTROL;
        if (more == 1)
            more_ctl = UI_Add_Region(0xd9, 0xa6, 0xf0, 0xaf, "M", -1);
        close = UI_Add_Region(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, "\x1b", -1);
        input = UI_Get_Input();
        if (input == close)
            done = 1;
        if (input == more_ctl) {
            if (breakdown_first == 0)
                breakdown_first = BREAKDOWN_PAGE_LINES;
            else
                breakdown_first = 0;
        }
        if (done == 0) {
            CityBreakdown_Draw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Clear_Controls();
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
}

/* the redraw of Screen_City_Breakdown: the scroll sized to the lines (at most 17), the title, a row of icons per
   line, then the line texts right of the widest row (" (Replaced)" after a line whose building is replaced) */
void CityBreakdown_Draw(void)
{
    int x;
    int x0;
    int top;
    int h;
    int clip;
    int w;
    int y_first;
    int max_w;
    int lines;                                  /* the shown line count (the scroll size), then each row's icon amount */
    int amount;
    int k;
    int y;

    lines = city_breakdown_lines;
    if (lines > BREAKDOWN_PAGE_LINES)
        lines = BREAKDOWN_PAGE_LINES;
    h = lines * 8 + 15;
    x0 = 0x32;
    top = 100 - (h + 0x30) / 2;
    clip = top + h + 0x17;
    Set_Draw_Page();
    Screen_Restore_A800();
    Set_Clip_Rect(0, 0, SCREEN_MAX_X, clip);
    Draw_Pic_Clipped(x0, top, breakdown_top_pic);
    Reset_Clip_Rect();
    Draw_Pic_Clipped(x0, top + h + 0x18, breakdown_bottom_pic);
    y = top + 0x1a;
    Font_Select_Palette(4, help_title_colors);
    switch (breakdown_kind) {
    case BREAKDOWN_FOOD_EATEN:
    case BREAKDOWN_FOOD_SURPLUS:
        Print_Text_Centered(x0 + 0x67, y, "Food");
        y += 12;
        Font_Select_Palette(1, help_text_colors);
        w = 0;
        max_w = 0;
        y_first = y;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = x0 + 0x14;
                lines = abs(p_city_breakdown[k]);
                if (p_city_breakdown[k] < 0)
                    w = Draw_Icon_Row(x, y - 1, lines, food_deficit_x1_pic, food_deficit_x10_pic);
                else
                    w = Draw_Icon_Row(x, y - 1, lines, resource_x1_pics[0], resource_x10_pics[0]);
                if (w > max_w)
                    max_w = w;
                y += 8;
            }
        y = y_first;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = max_w;
                strcpy(p_text_buf, breakdown_lines[k]);
                if (k == FOODLINE_ANIMISTS && p_cities[city_screen_city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == FOODLINE_GRANARY && p_cities[city_screen_city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == FOODLINE_FARMERS_MARKET && p_cities[city_screen_city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == FOODLINE_FORESTERS_GUILD && p_cities[city_screen_city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                Print_Text(x, y, p_text_buf);
                y += 8;
            }
        break;
    case BREAKDOWN_PRODUCTION:
        Print_Text_Centered(x0 + 0x67, y, "Production");
        y += 12;
        Font_Select_Palette(1, help_text_colors);
        w = 0;
        max_w = 0;
        y_first = y;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = x0 + 0x14;
                lines = abs(p_city_breakdown[k]);
                w = Draw_Icon_Row(x, y - 1, lines, resource_x1_pics[1], resource_x10_pics[1]);
                if (w > max_w)
                    max_w = w;
                y += 8;
            }
        y = y_first;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = max_w;
                strcpy(p_text_buf, breakdown_lines[k]);
                if (k == PRODLINE_SAWMILL && p_cities[city_screen_city].buildings[BLD_SAWMILL] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == PRODLINE_FORESTERS_GUILD && p_cities[city_screen_city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == PRODLINE_MINERS_GUILD && p_cities[city_screen_city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == PRODLINE_MECHANICIANS_GUILD && p_cities[city_screen_city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                Print_Text(x, y, p_text_buf);
                y += 8;
            }
        break;
    case BREAKDOWN_UPKEEP:
        Print_Text_Centered(x0 + 0x67, y, "Building Maintenance");
        y += 12;
        Font_Select_Palette(1, help_text_colors);
        lines = city_breakdown_lines;
        if (lines > BREAKDOWN_PAGE_LINES) {
            Print_Text(0xd9, 0xa7, "More");
            lines = BREAKDOWN_PAGE_LINES;
        }
        w = 0;
        max_w = 0;
        y_first = y;
        for (k = 0; k < lines && breakdown_first + k < city_breakdown_lines; k++) {
            x = x0 + 0x14;
            amount = abs(p_building_data[p_city_breakdown[breakdown_first + k]].gold_upkeep);
            w = Draw_Icon_Row(x, y - 1, amount, resource_x1_pics[2], resource_x10_pics[2]);
            if (w > max_w)
                max_w = w;
            y += 8;
        }
        y = y_first;
        for (k = 0; k < lines && breakdown_first + k < city_breakdown_lines; k++) {
            x = max_w;
            far_strcpy(p_text_buf2, 0, p_building_data[p_city_breakdown[breakdown_first + k]].name);
            strcpy(p_text_buf, p_text_buf2);
            if (p_cities[city_screen_city].buildings[p_city_breakdown[breakdown_first + k]] == BLDSTATE_REPLACED)
                strcat(p_text_buf, " (Replaced)");
            Print_Text(x, y, p_text_buf);
            y += 8;
        }
        break;
    case BREAKDOWN_GOLD_SURPLUS:
        Print_Text_Centered(x0 + 0x67, y, "Gold");
        y += 12;
        Font_Select_Palette(1, help_text_colors);
        w = 0;
        max_w = 0;
        y_first = y;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = x0 + 0x14;
                lines = abs(p_city_breakdown[k]);
                w = Draw_Icon_Row(x, y - 1, lines, resource_x1_pics[2], resource_x10_pics[2]);
                if (w > max_w)
                    max_w = w;
                y += 8;
            }
        y = y_first;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = max_w;
                strcpy(p_text_buf, breakdown_lines[k]);
                if (k == GOLDLINE_MINERS_GUILD && p_cities[city_screen_city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == GOLDLINE_MARKETPLACE && p_cities[city_screen_city].buildings[BLD_MARKETPLACE] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == GOLDLINE_BANK && p_cities[city_screen_city].buildings[BLD_BANK] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == GOLDLINE_MERCHANTS_GUILD && p_cities[city_screen_city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                Print_Text(x, y, p_text_buf);
                y += 8;
            }
        break;
    case BREAKDOWN_POWER:
        Print_Text_Centered(x0 + 0x67, y, "Magical Power");
        y += 12;
        Font_Select_Palette(1, help_text_colors);
        w = 0;
        max_w = 0;
        y_first = y;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = x0 + 0x14;
                if (k == POWLINE_WIZARDS_GUILD && (p_cities[city_screen_city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED ||
                               p_cities[city_screen_city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_BUILT))
                    Print_Text(x - 4, y, "-");      /* the Guild costs 3 power: City_Power_Breakdown stores it positive */
                lines = abs(p_city_breakdown[k]);
                w = Draw_Icon_Row(x, y - 1, lines, resource_x1_pics[3], resource_x10_pics[3]);
                if (w > max_w)
                    max_w = w;
                y += 8;
            }
        y = y_first;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = max_w;
                strcpy(p_text_buf, breakdown_lines[k]);
                if (k == POWLINE_SHRINE && p_cities[city_screen_city].buildings[BLD_SHRINE] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == POWLINE_TEMPLE && p_cities[city_screen_city].buildings[BLD_TEMPLE] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == POWLINE_PARTHENON && p_cities[city_screen_city].buildings[BLD_PARTHENON] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == POWLINE_CATHEDRAL && p_cities[city_screen_city].buildings[BLD_CATHEDRAL] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == POWLINE_ALCHEMISTS_GUILD && p_cities[city_screen_city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == POWLINE_WIZARDS_GUILD && p_cities[city_screen_city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == POWLINE_MINERS_GUILD && p_cities[city_screen_city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                Print_Text(x, y, p_text_buf);
                y += 8;
            }
        break;
    case BREAKDOWN_RESEARCH:
        Print_Text_Centered(x0 + 0x67, y, "Spell Research");
        y += 12;
        Font_Select_Palette(1, help_text_colors);
        w = 0;
        max_w = 0;
        y_first = y;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = x0 + 0x14;
                lines = abs(p_city_breakdown[k]);
                w = Draw_Icon_Row(x, y - 1, lines, resource_x1_pics[4], resource_x10_pics[4]);
                if (w > max_w)
                    max_w = w;
                y += 8;
            }
        y = y_first;
        for (k = 0; k < BREAKDOWN_LINES; k++)
            if (p_city_breakdown[k] != 0) {
                x = max_w;
                strcpy(p_text_buf, breakdown_lines[k]);
                if (k == RESLINE_LIBRARY && p_cities[city_screen_city].buildings[BLD_LIBRARY] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == RESLINE_SAGES_GUILD && p_cities[city_screen_city].buildings[BLD_SAGES_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == RESLINE_UNIVERSITY && p_cities[city_screen_city].buildings[BLD_UNIVERSITY] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                if (k == RESLINE_WIZARDS_GUILD && p_cities[city_screen_city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED)
                    strcat(p_text_buf, " (Replaced)");
                Print_Text(x, y, p_text_buf);
                y += 8;
            }
        break;
    }
}

/* (x, y, amount, one picture, ten picture) -> the x after the row: tens 8 px apart, then ones (8 px apart up to 3,
   else 6). The ones picture comes first here, the tens picture first in Draw_Icon_Row_Fit */
int Draw_Icon_Row(int x, int y, int amount, unsigned pic_one, unsigned pic_ten)
{
    int step;
    int tens;
    int k;

    tens = amount / 10;
    if (tens > 0) {
        step = 8;
        for (k = 0; k < tens; k++)
            Draw_Pic_Frame(x + k * step, y, pic_ten);
        x = x + tens * step + 6;
        amount = amount - tens * 10;
    }
    if (amount > 0) {
        if (amount > 3)
            step = 6;
        else
            step = 8;
        for (k = 0; k < amount; k++)
            Draw_Pic_Frame(x + k * step, y, pic_one);
        x = x + amount * step + 4;
    }
    return x;
}
