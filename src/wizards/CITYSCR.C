/* CITYSCR.C - WIZARDS.EXE overlay code segment 47bc (stub 23f2): the city screen - its loop (the breakdown
   controls, the X debug dump, the enchantment scroll arrows and turning off an own enchantment, Change, Buy, OK /
   Esc, selling back a building from the cityscape, the garrison slots and their statistics, the farmers row), its
   redraw (help entries, area map, info, buttons, cityscape, the trade goods / housing paragraph), the info panel
   (name, race, plague / boom, population and growth, turns left, the product picture and name), the controls and
   buttons, the "construction completed" popup, naming the starting city, the sell-back flags, the garrison icons,
   the screen's buffers, the product figure, a building picture's opaque bounds, the Buy button test and the area
   map's corner shading and shared-tile marks.
   This module's data (DS:2a8a..2c99): its literals ("" is the tail of "X", " " of "\x01 by purchasing ", "." of
   the one-building-each-turn message, "0" of "00", ")" of " (Plague)"); BSS DS:bf80..bfbd (defined here). No
   <string.h>: strlen() is undeclared (int), so 'strlen(name) < 1' is a signed test, as in the original.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 47bc src/wizards/CITYSCR.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_City(void);
void CityScr_Redraw(void);
void CityScr_Draw_Info(void);
void CityScr_Add_Controls(void);
void CityScr_Add_Cityscape_Hotspots(void);
void CityScr_Add_Buttons(void);
void CityScr_Draw_Buttons(void);
void Popup_Building_Completed(int unused_a, int unused_b, int city, int building);
void Dialog_Name_Starting_City(int city);
void NameCity_Redraw(void);
void CityScr_Build_Sell_Flags(int city);
int CityScr_Draw_Garrison(int x, int y);
void CityScr_Alloc_Buffers(void);
void CityScr_Load_Minimap_And_Product(void);
void Building_Pic_Opaque_Bounds(int city, int bldg, int *x, int *y, int *w, int *h, unsigned bm);
void CityScr_Update_Buy_Disabled(void);
void CityScr_Draw_Area_Map_Overlay(void);

/* defined ahead of GAME.H, whose externs (p_text_buf2 first) would otherwise order them */
char *p_text_buf;                               /* DS:bfbc  shared text/string buffer */
char *p_text_buf2;                              /* DS:bfba  second text buffer (itoa / strcat source) */

#include "GAME.H"

int cityscr_unused_bfb8;                        /* DS:bfb8  never referenced */
int production_return_screen;                   /* DS:bfb6  where Screen_City_Production goes on exit */
int city_new_building;                          /* DS:bfb4  the just-built building Cityscape_Draw shows; -1 none */
int city_screen_turns_left;                     /* DS:bfb2  City_Turns_To_Complete of the product */
int city_screen_cost_left;                      /* DS:bfb0  City_Remaining_Cost of the product */
char *p_city_screen_text;                       /* DS:bfae  the product's name */
int city_buy_button;                            /* DS:bfac  -1000: none (Buy disabled) */
int city_change_button;                         /* DS:bfaa */
int city_ok_button;                             /* DS:bfa8 */
int city_esc_hotkey;                            /* DS:bfa6 */
int city_garrison_x;                            /* DS:bfa4  215: left of the garrison slots (CityScr_Unit_Slot_Rect) */
int city_garrison_y;                            /* DS:bfa2  102: their top; both also passed, unused, to Draw_Garrison */
int *p_city_ench_ctls;                          /* DS:bfa0  control id of each listed enchantment */
int *p_city_ench_slots;                         /* DS:bf9e  City_Get_Enchant_List enchantment slots */
int *p_city_ench_owners;                        /* DS:bf9c  City_Get_Enchant_List owners */
int city_ench_count;                            /* DS:bf9a  City_Get_Enchant_List count */
int city_ench_list_first;                       /* DS:bf98  first enchantment row shown */
int city_ench_scroll;                           /* DS:bf96  1: the list needs the scroll arrows */
int cityscr_unused_bf94;                        /* DS:bf94  set to 0, never read */
int city_ench_shown;                         /* DS:bf92  enchantments listed on the screen */
int *city_hotspot_controls;                     /* DS:bf90  control id of each cityscape building */
int cityscape_hotspot_shown;                    /* DS:bf8e  hotspots added as controls */
int city_map_view_x;                            /* DS:bf8c  the 5 x 5 area map's view origin (Map_Draw_View) */
int city_map_view_y;                            /* DS:bf8a */
int cityscr_unused_bf88;                        /* DS:bf88  set to -1000, never read */
int city_ench_up_button;                          /* DS:bf86  enchantment scroll-up control */
int city_ench_down_button;                          /* DS:bf84  enchantment scroll-down control */
int *p_cityscr_sell_flags;                    /* DS:bf82  int[36]: can sell back (never read) */
int cityscr_buy_disabled;                       /* DS:bf80  1: no Buy button */

extern char *city_enchant_names[];              /* DS:1ee4 */
extern struct RaceInfo race_table[14];          /* DS:1d86 */
extern struct CityscapeHotspot far *p_cityscape_hotspots;  /* DS:92b0 */
extern int cityscape_hotspot_count;             /* DS:92b4 */
extern unsigned unitview_land_bg_pic;           /* DS:92b6  CITYSCAP.LBX 0x67 */
extern unsigned building_pics[];                /* DS:92b8  CITYSCAP pictures by BuildingId */
extern unsigned cityscape_trade_goods_pic;      /* DS:9300  CITYSCAP.LBX 0x29 */
extern unsigned cityscape_housing_pics[3];      /* DS:9302  CITYSCAP.LBX 0x2a..0x2c, by race house style */
extern unsigned p_music_buf;                    /* DS:9390 */
extern int no_units_to_move;                    /* DS:9444 */
extern unsigned name_entry_box_pic;             /* DS:953c  BACKGRND.LBX 0x21 */
extern unsigned city_shared_tile_pic;           /* DS:95f0  BACKGRND 0: the mark on a map square another city shares */
extern int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];               /* DS:964c */
extern int minimap_control;                     /* DS:973e */
extern int map_grid_control;                    /* DS:9740 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int city_hover_control;                  /* DS:9752  control under the mouse (UI_Hit_Test) */
extern unsigned city_done_backdrop_pic;         /* DS:9758  BACKGRND 10: the grassland behind a completed building */
extern unsigned city_ench_down_arrow_pic;       /* DS:975a  BACKGRND.LBX 0x10 */
extern unsigned city_ench_up_arrow_pic;         /* DS:975c  BACKGRND.LBX 0x0f */
extern unsigned unitview_water_bg_pic;          /* DS:979c  UNITVIEW.LBX 0x22 */
extern unsigned city_screen_bg_pic;             /* DS:9936  BACKGRND 6: the city screen */
extern unsigned city_no_product_pic;            /* DS:9938  BACKGRND 13: blank panel over the product (Trade Goods / Housing) */
extern unsigned city_buy_greyed_pic;            /* DS:993e  BACKGRND 14: Buy greyed out */
extern unsigned city_ok_button_pic;             /* DS:9940  BACKGRND 9: OK */
extern unsigned city_buy_button_pic;            /* DS:9942  BACKGRND 7: Buy */
extern unsigned city_change_button_pic;         /* DS:9944  BACKGRND 8: Change */
extern struct Events far *p_events;             /* DS:9998 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern int present_mode;                        /* DS:9c8e */
extern unsigned big_city_wall_pic;              /* DS:9d18  CITYSCAP 0x72 'big city wall': the completed-walls popup */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int map_view_plane;                      /* DS:bd86 */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int city_breakdown_ctls[7];              /* DS:bf70  the breakdown controls (a click opens Screen_City_Breakdown) */
extern int *p_city_citizen_ctls;                 /* DS:bf7e  control id of each non-rebel citizen (farmers, then workers) */
extern int ui_unit_slot_controls[MAX_STACK];            /* DS:c052 */
extern int goto_preview_on;                     /* DS:c068 */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */

char *itoa(int value, char *string, int radix);
int getch(void);
void far_strcpy();                              /* unprototyped: also called with (far dst, near src, 0) */
void Help_Load_City(void);
void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Map_Center_City_View(int *view_x, int *view_y, int x, int y, int plane);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Stack_Blink_Off(void);
void Stack_Blink_Reset(void);
void Map_Build_Unit_Grid(int x, int y, int plane);
void Map_Invalidate(void);
void CityScr_Build_Enchant_List(void);
int City_Min_Farmers(int city);
int City_Rebels(int city);
void City_Recalculate(int city);
int City_Remaining_Cost(int producing, int city);
int City_Turns_To_Complete(int cost, int city);
int City_Growth_Rate(int city);
void Palette_Cycle_Reset(void);
void UI_Input_Delay(int delay);
void Timer_Mark(void);
void Wait_Ticks(int ticks);
int UI_Add_Hotkey(char *key);
int UI_Get_Input(void);
int UI_Hit_Test(void);
void UI_Clear_Help(void);
void UI_Clear_Controls(void);
int UI_Add_Region(int x1, int y1, int x2, int y2, char *hotkey, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Text_Input_Box(int x, int y, int width, char *buffer, int max_len, int fill, int right_align, int caret_style,
                      int caret_colors, int help);
void Screen_City_Breakdown(int breakdown);
void Screen_City_Production(void);
void Screen_Clear_Both_Pages(void);
int Debug_Print_Labeled_Number(int col, int row, int value, char *label);
int Debug_Print_Number(int col, int row, int value);
void Play_Click_Sound(void);
int City_Buy_Price(int city);
void City_Buy(int player, int city);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
char *Text_Article_A_An(char *word);
int Confirm_Box(char *text);
void Message_Box(char *text);
void City_Clear_Enchantment(int city, int slot);
void City_Get_Enchant_List(int city, int *slots, int *owners, int *count);
int City_Can_Sell_Building(int bld, int city, int *needed_by);
int City_Production_Independent_Of(int city, int bld);
int Building_Sell_Price();                      /* unprototyped: called with (building, city), CITY.C defines it with 1 parameter */
int City_Sell_Building(int bld, int city, int price);
void CityScr_Unit_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2);
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2);
void City_Frame_AI_Nop(void);
void Screen_Present(void);
void Screen_Copy_Front(void);
void Reset_Clip_Rect(void);
void Set_Draw_Page(void);
void Screen_Save_A800(void);
void Screen_Restore_A800(void);
int City_Enchant_Help_Id(int slot);
void Minimap_Set_Size(int width, int height);
void Minimap_Alloc_Pic(void);
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty, int player);
void Map_Full_Redraw(void);
void Map_Draw_Fog(int px, int py, int w, int h, int vx, int vy, int plane);
void Map_Draw_Unit_Grid(int px, int py, int w, int h, int vx, int vy);
int Tile_Is_Shared(int x, int y, int plane);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Set_Pic_Frame(unsigned pic, unsigned frame);
int Pic_Get_Frame(unsigned pic);
int Pic_Frame_Count(unsigned pic);
void Pic_Get_Opaque_Bounds(unsigned pic, int *x, int *y, int *w, int *h);
void Pic_Set_Remap_Flag(unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
void Fill_Box_Styled();                           /* unprototyped: called with 6 arguments */
void Cityscape_Draw(int city, int x, int y, int new_obj, int fade_obj);
void Cityscape_Reveal_Reset(void);
int CityScr_Draw_Hotspot_Label(int control, int panel_x, int owner);
void CityScr_Draw_Production_Boxes(int city);
void CityScr_Draw_Population_Row(int city, int x, int y);
void CityScr_Draw_Resource_Rows(int city, int x, int y);
void CityScr_Draw_Enchant_List(int x, int y);
void CityScr_Add_Enchant_Controls(int x, int y);
void CityScr_Add_Citizen_Controls(int city, int x, int y);
void CityScr_Add_Resource_Controls(int city, int x, int y);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Font_Select_Shadow_UL(int font, int c1, int c2, int c3);
void Set_Text_Shadow_Color(int color);
void Set_Font_Color(int color);
void Set_Line_Spacing(int line_gap);
void Set_Space_Width(int width);
void Font_Select_Palette(int font, char *colors);
int Print_Text(int x, int y, char *text);
int Print_Text_Centered(int x, int y, char *text);
int Print_Text_Right(int x, int y, char *text);
int Print_Number_Right(int x, int y, int value);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Paragraph_Used_Width(int width, char *text, unsigned align);
int Paragraph_Height(int width, char *text);
void Str_Trim(char *text);
void EMS_Map_Default(void);
void Draw_Unit_Figures(int x, int y, int type_or_unit, int is_unit);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void Outline_Cycle_Tick(void);
void Load_Unit_Figure(int unit_or_type, int is_type);
void Heap_Mark(unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Heap_Release(unsigned block);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
unsigned LBX_Reload(char *name, int entry, unsigned block);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Music_Play_Current(void);
void City_Pick_Default_Name(int race, char far *name);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);

/* () the city screen of city_screen_city: centres the area map, selects the city's stack, clamps the farmers
   (at least City_Min_Farmers, at most the non-rebels), defaults the product to housing, then loops until OK / Esc
   or a garrison unit is clicked (back to the main screen, 5); X dumps the city's numbers on the debug screen
   Numbered: BUG #337 (BUGS_AND_QUIRKS.txt) */
void Screen_City(void)
{
    int leave;
    int redraw;
    int i;
    int type;                                   /* reused: a unit type, the needed_by out value, a 'production ceases' flag, a debug row */
    int city_x;
    int city_y;
    int hotkey_x;
    int amount;
    int bldg;
    int x1;
    int y1;
    int x2;
    int y2;
    int min_farmers;
    int rebels;
    int no_units;
    int row;
    int input;

    Help_Load_City();
    if (present_mode != PRESENT_NORMAL && present_mode != PRESENT_DISSOLVE)
        present_mode = PRESENT_NORMAL;
    goto_preview_on = 0;
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(CityScr_Redraw, 1);
    CityScr_Alloc_Buffers();
    city_map_view_x = 0;
    city_map_view_y = 0;
    Map_Center_City_View(&city_map_view_x, &city_map_view_y, p_cities[city_screen_city].x, p_cities[city_screen_city].y,
                         map_view_plane);
    city_garrison_x = 215;
    city_garrison_y = 102;
    no_units = 0;
    if (stack_unit_count == 0)
        no_units = 1;
    city_x = p_cities[city_screen_city].x;
    city_y = p_cities[city_screen_city].y;
    Stack_Select_At(current_player, &city_map_view_x, &city_map_view_y, map_view_plane, city_x, city_y);
    Stack_Blink_Off();
    Map_Build_Unit_Grid(city_map_view_x, city_map_view_y, map_view_plane);
    Map_Invalidate();
    CityScr_Build_Sell_Flags(city_screen_city);
    CityScr_Build_Enchant_List();
    min_farmers = City_Min_Farmers(city_screen_city);
    if (p_cities[city_screen_city].farmers < min_farmers)
        p_cities[city_screen_city].farmers = min_farmers;
    rebels = City_Rebels(city_screen_city);
    if (p_cities[city_screen_city].farmers > p_cities[city_screen_city].population - rebels)
        p_cities[city_screen_city].farmers = p_cities[city_screen_city].population - rebels;
    if (p_cities[city_screen_city].producing == PROD_NONE)
        p_cities[city_screen_city].producing = PROD_HOUSING;
    City_Recalculate(city_screen_city);
    city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing, city_screen_city);
    city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
    if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE) {
        type = p_cities[city_screen_city].producing - PROD_UNIT_BASE;
        strcpy(p_city_screen_text, unit_types[type].name);
    } else
        far_strcpy(p_city_screen_text, 0, p_building_data[p_cities[city_screen_city].producing].name);
    cityscape_hotspot_count = 0;
    CityScr_Update_Buy_Disabled();
    city_hover_control = -1;
    CityScr_Load_Minimap_And_Product();
    City_Recalculate(city_screen_city);
    city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing, city_screen_city);
    city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
    Palette_Cycle_Reset();
    UI_Input_Delay(2);
    redraw = 0;
    leave = 0;
    city_hover_control = -1;
    while (leave == 0) {
        Timer_Mark();
        CityScr_Add_Controls();
        hotkey_x = UI_Add_Hotkey("X");
        input = UI_Get_Input();
        city_hover_control = UI_Hit_Test();
        for (i = 0; i < BREAKDOWN_RESEARCH + 1; i++) {
            if (city_breakdown_ctls[i] == input) {
                Screen_City_Breakdown(i);
                redraw = 1;
                CityScr_Alloc_Buffers();
                UI_Clear_Help();
                Help_Load_City();
                UI_Set_Redraw_Fn(CityScr_Redraw, 1);
                CityScr_Update_Buy_Disabled();
                if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE) {
                    type = p_cities[city_screen_city].producing - PROD_UNIT_BASE;
                    strcpy(p_city_screen_text, unit_types[type].name);
                } else
                    far_strcpy(p_city_screen_text, 0, p_building_data[p_cities[city_screen_city].producing].name);
                CityScr_Load_Minimap_And_Product();
                CityScr_Build_Enchant_List();
                city_hover_control = -1;
                redraw = 1;
                UI_Clear_Controls();
                cityscape_hotspot_count = 0;
                UI_Input_Delay(2);
                break;
            }
        }
        if (input == hotkey_x) {
            Screen_Clear_Both_Pages();
            row = 5;
            row++;
            Debug_Print_Labeled_Number(0, row, p_cities[city_screen_city].population, "population");
            row++;
            Debug_Print_Labeled_Number(0, row, p_cities[city_screen_city].production_income, "production");
            row++;
            for (i = 0; i < 20; i++)
                Debug_Print_Number(i, row, p_cities[city_screen_city].buildings[i]);
            row++;
            for (i = 20; i < NUM_BUILDINGS; i++)
                Debug_Print_Number(i - 20, row, p_cities[city_screen_city].buildings[i]);
            row++;
            Debug_Print_Number(0, row, p_cities[city_screen_city].x);
            Debug_Print_Number(5, row, p_cities[city_screen_city].y);
            Debug_Print_Number(10, row, p_cities[city_screen_city].plane);
            row++;
            Debug_Print_Labeled_Number(0, row, city_screen_city, "city");
            row++;
            Debug_Print_Labeled_Number(0, row, p_cities[city_screen_city].buildings[BLD_CITY_WALLS], "walls");
            for (type = 0; type < 5; type++)
                for (i = 0; i < 5; i++)
                    Debug_Print_Number(i, type, map_unit_grid[type][i]);
            getch();
            Map_Invalidate();
        }
        if (input == city_ench_up_button) {
            Play_Click_Sound();
            city_ench_list_first -= NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first < 0)
                city_ench_list_first = city_ench_count / NUM_CITY_ENCH_ROWS * NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first >= city_ench_count)
                city_ench_list_first = city_ench_count - NUM_CITY_ENCH_ROWS;
            redraw = 1;
        }
        if (input == city_ench_down_button) {
            Play_Click_Sound();
            city_ench_list_first += NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first >= city_ench_count)
                city_ench_list_first = 0;
            redraw = 1;
        }
        if (input == city_change_button) {
            Play_Click_Sound();
            production_return_screen = PROD_RETURN_CITY_SCREEN;
            Screen_City_Production();
            CityScr_Alloc_Buffers();
            UI_Clear_Help();
            Help_Load_City();
            UI_Set_Redraw_Fn(CityScr_Redraw, 1);
            City_Recalculate(city_screen_city);
            city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing, city_screen_city);
            city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
            CityScr_Update_Buy_Disabled();
            if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE) {
                type = p_cities[city_screen_city].producing - PROD_UNIT_BASE;
                strcpy(p_city_screen_text, unit_types[type].name);
            } else
                far_strcpy(p_city_screen_text, 0, p_building_data[p_cities[city_screen_city].producing].name);
            CityScr_Load_Minimap_And_Product();
            production_return_screen = PROD_RETURN_CITY_SCREEN;
            redraw = 1;
        }
        if (input == city_ok_button || input == city_esc_hotkey) {
            stack_unit_count = 0;
            Play_Click_Sound();
            map_scroll_target_x = map_view_x;
            map_scroll_target_y = map_view_y;
            UI_Suspend_Redraw_Fn();
            UI_Clear_Help();
            if (no_units == 1) {
                stack_unit_count = 0;
            } else {
            }
            leave = 1;
            current_screen = SCREEN_MAP;
        }
        if (input == city_buy_button) {
            Play_Click_Sound();
            amount = wizards[current_player].gold;
            if (city_screen_turns_left > 1 && City_Buy_Price(city_screen_city) <= amount) {
                UI_Clear_Help();
                Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                strcpy(p_text_buf, "Do you wish to spend \x02");
                itoa(City_Buy_Price(city_screen_city), p_text_buf2, 10);
                strcat(p_text_buf, p_text_buf2);
                strcat(p_text_buf, " Gold");
                strcat(p_text_buf, "\x01 by purchasing ");
                strcat(p_text_buf, Text_Article_A_An(p_city_screen_text));
                strcat(p_text_buf, " ");
                strcat(p_text_buf, p_city_screen_text);
                strcat(p_text_buf, "?");
                if (Confirm_Box(p_text_buf) == 1)
                    City_Buy(current_player, city_screen_city);
                UI_Suspend_Redraw_Fn();
                UI_Set_Redraw_Fn(CityScr_Redraw, 1);
                City_Recalculate(city_screen_city);
                city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing, city_screen_city);
                city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
                CityScr_Update_Buy_Disabled();
            } else {
            }
            UI_Suspend_Redraw_Fn();
            UI_Set_Redraw_Fn(CityScr_Redraw, 1);
            redraw = 1;
            Map_Invalidate();
            UI_Clear_Help();
            Help_Load_City();
        }
        for (i = 0; i < city_ench_shown; i++) {
            if (p_city_ench_ctls[i] == input && p_city_ench_owners[city_ench_list_first + i] == current_player) {
                Play_Click_Sound();
                UI_Clear_Help();
                strcpy(p_text_buf, "Do you wish to turn off the \x02");
                strcat(p_text_buf, city_enchant_names[p_city_ench_slots[city_ench_list_first + i]]);
                strcat(p_text_buf, "\x01 spell?");
                if (Confirm_Box(p_text_buf) == 1) {
                    City_Clear_Enchantment(city_screen_city, p_city_ench_slots[city_ench_list_first + i]);
                    City_Get_Enchant_List(city_screen_city, p_city_ench_slots, p_city_ench_owners, &city_ench_count);
                    city_ench_scroll = 0;
                    if (city_ench_count > NUM_CITY_ENCH_ROWS)
                        city_ench_scroll = 1;
                    else
                        city_ench_list_first = 0;
                    City_Recalculate(city_screen_city);
                    city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing,
                                                                city_screen_city);
                    city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
                }
                UI_Suspend_Redraw_Fn();
                UI_Set_Redraw_Fn(CityScr_Redraw, 1);
                redraw = 1;
                Map_Invalidate();
                UI_Clear_Help();
                Help_Load_City();
                break;
            }
        }
        for (i = 0; i < cityscape_hotspot_shown; i++) {
            if (city_hotspot_controls[i] == input) {
                Play_Click_Sound();
                bldg = p_cityscape_hotspots[i].building;
                if (bldg <= NUM_BUILDINGS) {     /* a building, not a cityscape special (SCAPE_*: 100 ..) */
                    if (p_cities[city_screen_city].sold_this_turn == 1)
                        Message_Box("You can only sell back one building each turn.");
                    else if (City_Can_Sell_Building(bldg, city_screen_city, &type) == 1) {
                        UI_Clear_Help();
                        type = 0;
                        if (City_Production_Independent_Of(city_screen_city, bldg) == 0) {
                            strcpy(p_text_buf, "Selling back your \x02");
                            far_strcpy(p_text_buf2, 0, p_building_data[bldg].name);
                            strcat(p_text_buf, p_text_buf2);
                            strcat(p_text_buf, "\x01 will cease production of your \x02");
                            if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE)
                                strcat(p_text_buf, unit_types[p_cities[city_screen_city].producing - PROD_UNIT_BASE].name);
                            else {
                                far_strcpy(p_text_buf2, 0, p_building_data[p_cities[city_screen_city].producing].name);
                                strcat(p_text_buf, p_text_buf2);
                            }
                            strcat(p_text_buf, "\x01.");
                            Message_Box(p_text_buf);
                            type = 1;
                        }
                        UI_Clear_Help();
                        amount = Building_Sell_Price(bldg, city_screen_city);
                        strcpy(p_text_buf, "Do you wish to sell back the \x02");
                        far_strcpy(p_text_buf2, 0, p_building_data[bldg].name);
                        strcat(p_text_buf, p_text_buf2);
                        strcat(p_text_buf, "\x01 for ");
                        itoa(amount, p_text_buf2, 10);
                        strcat(p_text_buf, p_text_buf2);
                        strcat(p_text_buf, " gold?");
                        if (Confirm_Box(p_text_buf) == 1) {
                            City_Sell_Building(bldg, city_screen_city, amount);
                            if (type == 1)
                                p_cities[city_screen_city].producing = PROD_HOUSING;
                            CityScr_Build_Sell_Flags(city_screen_city);
                            City_Recalculate(city_screen_city);
                            city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing,
                                                                        city_screen_city);
                            city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
                            CityScr_Update_Buy_Disabled();
                            if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE) {
                                type = p_cities[city_screen_city].producing - PROD_UNIT_BASE;
                                strcpy(p_city_screen_text, unit_types[type].name);
                            } else
                                far_strcpy(p_city_screen_text, 0,
                                           p_building_data[p_cities[city_screen_city].producing].name);
                            p_cities[city_screen_city].sold_this_turn = 1;
                        }
                    } else {
                        UI_Clear_Help();
                        if (type > -1) {
                            strcpy(p_text_buf, "You cannot sell back the ");
                            far_strcpy(p_text_buf2, 0, p_building_data[bldg].name);
                            strcat(p_text_buf, p_text_buf2);
                            strcat(p_text_buf, " because it is required by the ");
                            far_strcpy(p_text_buf2, 0, p_building_data[type].name);
                            strcat(p_text_buf, p_text_buf2);
                            strcat(p_text_buf, ".");
                            Screen_Copy_Front();
                            UI_Clear_Controls();
                            Message_Box(p_text_buf);
                        }
                    }
                }
                UI_Suspend_Redraw_Fn();
                UI_Set_Redraw_Fn(CityScr_Redraw, 1);
                redraw = 1;
                Map_Invalidate();
                UI_Clear_Help();
                Help_Load_City();
                break;
            }
        }
        for (i = 0; i < stack_unit_count; i++) {
            if (ui_unit_slot_controls[i] == input) {
                Play_Click_Sound();
                map_scroll_target_x = map_view_x;
                map_scroll_target_y = map_view_y;
                leave = 1;
                current_screen = SCREEN_MAP;
                if (no_units_to_move == 1)
                    no_units_to_move = 0;
                break;
            }
        }
        for (i = 0; i < stack_unit_count; i++) {
            if (-ui_unit_slot_controls[i] == input) {  /* a right click comes back as the control's negative */
                CityScr_Unit_Slot_Rect(i, &x1, &y1, &x2, &y2);
                Unit_Stats_Popup(stack_units[i].unit, x1 + 1, y1 + 1, x2 + 1, y2 + 1);
                UI_Set_Redraw_Fn(CityScr_Redraw, 1);
                CityScr_Alloc_Buffers();
                CityScr_Build_Sell_Flags(city_screen_city);
                if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE) {
                    type = p_cities[city_screen_city].producing - PROD_UNIT_BASE;
                    strcpy(p_city_screen_text, unit_types[type].name);
                } else
                    far_strcpy(p_city_screen_text, 0, p_building_data[p_cities[city_screen_city].producing].name);
                redraw = 1;
                CityScr_Load_Minimap_And_Product();
                city_x = p_cities[city_screen_city].x;
                city_y = p_cities[city_screen_city].y;
                Stack_Select_At(current_player, &city_map_view_x, &city_map_view_y, map_view_plane, city_x, city_y);
                CityScr_Build_Enchant_List();
                Map_Invalidate();
                UI_Clear_Help();
                Help_Load_City();
                break;
            }
        }
        for (i = 0; i < p_cities[city_screen_city].population - City_Rebels(city_screen_city); i++) {
            if (p_city_citizen_ctls[i] == input) {      /* citizen i -> i + 1 farmers, at least the minimum; with no
                                                   minimum, the first farmer of a farmed city toggles to 0 */
                Play_Click_Sound();
                min_farmers = City_Min_Farmers(city_screen_city);
                rebels = City_Rebels(city_screen_city);   /* never read here; clarified using ReMoM repo */
                if (min_farmers == 0) {
                    if (p_cities[city_screen_city].farmers == 0)
                        p_cities[city_screen_city].farmers = i + 1;
                    else if (i == 0)
                        p_cities[city_screen_city].farmers = 0;
                    else
                        p_cities[city_screen_city].farmers = i + 1;
                } else if (i < min_farmers)
                    p_cities[city_screen_city].farmers = min_farmers;
                else
                    p_cities[city_screen_city].farmers = i + 1;
                City_Recalculate(city_screen_city);
                city_screen_cost_left = City_Remaining_Cost(p_cities[city_screen_city].producing, city_screen_city);
                city_screen_turns_left = City_Turns_To_Complete(city_screen_cost_left, city_screen_city);
                City_Recalculate(city_screen_city);
                CityScr_Update_Buy_Disabled();
                break;
            }
        }
        if (leave == 0 && redraw == 0) {
            CityScr_Redraw();
            City_Frame_AI_Nop();
            Screen_Present();
            Wait_Ticks(1);
        }
        redraw = 0;
    }
    city_new_building = -1;
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    Stack_Blink_Reset();
}

/* () the city screen's redraw: the help entries (race, the resource rows present, the listed enchantments, the
   building under the mouse), the 5x5 area map with its overlay, the background, info and buttons, the cityscape
   (with the just-built building), the hovered building's label, and the trade goods / housing paragraph or the
   production boxes */
void CityScr_Redraw(void)
{
    int i;

    help_entries[1].id = p_cities[city_screen_city].race + HELP_RACE_TOWNSFOLK;
    help_entries[2].id = -1;
    if (p_cities[city_screen_city].food_income > 0)
        help_entries[2].id = HELP_FOOD;
    help_entries[3].id = -1;
    if (p_cities[city_screen_city].production_income > 0)
        help_entries[3].id = HELP_PRODUCTION;
    help_entries[4].id = -1;
    if (p_cities[city_screen_city].gold_income > 0 || p_cities[city_screen_city].building_upkeep > 0)
        help_entries[4].id = HELP_GOLD;
    help_entries[5].id = -1;
    if (p_cities[city_screen_city].power_income > 0)
        help_entries[5].id = HELP_MAGICAL_POWER;
    help_entries[6].id = -1;
    if (p_cities[city_screen_city].research_income > 0)
        help_entries[6].id = HELP_SPELL_RESEARCH;
    help_entries[7].id = HELP_CITY_ENCHANTMENTS;
    for (i = 0; i < 6; i++)
        help_entries[16 + i].id = -1;
    if (city_ench_shown > 0) {
        help_entries[7].id = -1;
        for (i = 0; i < city_ench_shown; i++)
            help_entries[16 + i].id = City_Enchant_Help_Id(p_city_ench_slots[city_ench_list_first + i]);
    }
    help_entries[9].id = HELP_CITYSCAPE;
    for (i = 0; i < cityscape_hotspot_shown; i++) {
        if (city_hotspot_controls[i] == city_hover_control) {
            if (p_cityscape_hotspots[i].building >= SCAPE_SUMMONING_CIRCLE)
                help_entries[9].id = p_cityscape_hotspots[i].building + HELP_CITYSCAPE_OBJECT_BASE;
            else
                help_entries[9].id = p_cityscape_hotspots[i].building + HELP_BUILDING;
            break;
        }
    }
    help_entries[14].id = -1;
    Reset_Clip_Rect();
    Set_Draw_Page();
    Map_Invalidate();
    Minimap_Set_Size(MINIMAP_W, MINIMAP_H);
    Map_Draw_View(215, 4, 5, 5, &city_map_view_x, &city_map_view_y, map_view_plane, map_scroll_target_x, map_scroll_target_y,
                  current_player);
    CityScr_Draw_Area_Map_Overlay();
    Reset_Clip_Rect();
    Draw_Pic_Frame(0, 0, city_screen_bg_pic);
    CityScr_Draw_Info();
    CityScr_Draw_Buttons();
    Reset_Clip_Rect();
    if (city_new_building != -1) {             /* empty branches: the original has them too (also above) */
    } else {
    }
    Cityscape_Draw(city_screen_city, 4, 101, city_new_building, city_new_building);
    CityScr_Draw_Hotspot_Label(city_hover_control, 1, p_cities[city_screen_city].owner);
    if (p_cities[city_screen_city].producing == PROD_TRADE_GOODS || p_cities[city_screen_city].producing == PROD_HOUSING) {
        Draw_Pic_Frame(260, 149, city_no_product_pic);
        Font_Select_Shadow_UL(0, 0, 0, 0);
        Set_Text_Shadow_Color(4);
        Set_Font_Color(26);
        Set_Line_Spacing(0);
        if (p_cities[city_screen_city].producing == PROD_TRADE_GOODS)
            Paragraph_Print(262, 159, 50, "Converts production to gold.", PARA_CENTER);
        else
            Paragraph_Print(262, 159, 50, "Increases population growth rate.", PARA_CENTER);
    } else
        CityScr_Draw_Production_Boxes(city_screen_city);
}

/* () the info panel: "<size> of <name>", the race with (Plague) / (Pop. Boom), "Population: n,nnn (+g)", the
   citizens and resource rows, the enchantment list, the garrison, the turns left, the product's picture (a unit's
   figures on land or water, a building centred by its opaque bounds) and its name (smaller font when it does not
   fit, a paragraph above the line when it wraps) */
void CityScr_Draw_Info(void)
{
    int prod;
    unsigned bm;
    int x1;
    int y1;
    int w;
    int h;
    int pic_x;
    int pic_y;
    char colors[6];
    int pop;
    int type;

    strcpy(p_text_buf, city_size_names[p_cities[city_screen_city].size]);
    strcat(p_text_buf, " of ");
    far_strcpy(p_text_buf2, 0, p_cities[city_screen_city].name);
    strcat(p_text_buf, p_text_buf2);
    Font_Select_Shadow(5, 5, 0, 0);
    Set_Font_Color(26);
    Set_Text_Shadow_Color(0);
    Print_Text_Centered(105, 3, p_text_buf);
    colors[0] = 0xbe;
    colors[1] = 0xb3;
    Font_Select_Palette(0, colors);
    Set_Text_Shadow_Color(19);
    Font_Select_Shadow(1, 15, 0, 0);
    strcpy(p_text_buf, race_table[p_cities[city_screen_city].race].name);
    if (p_events->plague == EVENT_STATE_RUNNING && p_events->plague_city == city_screen_city) {
        help_entries[14].id = HELP_PLAGUE;
        strcat(p_text_buf, " (Plague)");
    }
    if (p_events->population_boom == EVENT_STATE_RUNNING && p_events->boom_city == city_screen_city) {
        help_entries[14].id = HELP_POPULATION_BOOM;
        strcat(p_text_buf, " (Pop. Boom)");
    }
    Print_Text(6, 19, p_text_buf);
    pop = p_cities[city_screen_city].population * 1000 + p_cities[city_screen_city].population_tens * 10;
    strcpy(p_text_buf, "Population: ");
    if (pop > 999) {
        itoa(pop / 1000, p_text_buf2, 10);
        strcat(p_text_buf, p_text_buf2);
        strcat(p_text_buf, ",");
        pop = pop - pop / 1000 * 1000;
        itoa(pop, p_text_buf2, 10);
        if (pop < 10)
            strcat(p_text_buf, "00");
        else if (pop < 100)
            strcat(p_text_buf, "0");
    } else
        itoa(pop, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, " (");
    EMS_Map_Default();
    pop = City_Growth_Rate(city_screen_city) * 10;
    itoa(pop, p_text_buf2, 10);
    if (pop > -1)
        strcat(p_text_buf, "+");
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, ")");
    Set_Space_Width(1);
    Print_Text_Right(208, 19, p_text_buf);
    CityScr_Draw_Population_Row(city_screen_city, 4, 27);
    CityScr_Draw_Resource_Rows(city_screen_city, 6, 52);
    CityScr_Draw_Enchant_List(139, 51);
    CityScr_Draw_Garrison(city_garrison_x, city_garrison_y);
    colors[0] = 0xbe;
    colors[1] = 0xb3;
    Font_Select_Palette(0, colors);
    Set_Text_Shadow_Color(19);
    Font_Select_Shadow(1, 15, 0, 0);
    if (city_screen_turns_left > 0 && p_cities[city_screen_city].producing != PROD_HOUSING
        && p_cities[city_screen_city].producing != PROD_TRADE_GOODS) {
        Print_Number_Right(286, 140, city_screen_turns_left);
        if (city_screen_turns_left > 1)
            strcpy(p_text_buf, "Turns");
        else
            strcpy(p_text_buf, "Turn");
        Print_Text(291, 140, p_text_buf);
    }
    prod = p_cities[city_screen_city].producing;
    if (prod >= PROD_UNIT_BASE) {
        type = prod - PROD_UNIT_BASE;
        if (unit_types[type].transport_capacity > 0)
            Draw_Pic_Frame(219, 157, unitview_water_bg_pic);
        else {
            Set_Pic_Frame(unitview_land_bg_pic, 0);
            Draw_Pic_Frame(218, 144, unitview_land_bg_pic);
        }
        Draw_Unit_Figures(224, 136, type, 0);
    } else {
        pic_x = 215;
        pic_y = 147;
        Heap_Mark(p_scratch);
        bm = Heap_Alloc_Back(p_scratch, 500);
        Heap_Release(p_scratch);               /* bm is used after this release: nothing below allocates first */
        Building_Pic_Opaque_Bounds(city_screen_city, prod, &x1, &y1, &w, &h, bm);
        pic_x = pic_x + ((44 - w) / 2 - x1);
        pic_y = pic_y + ((31 - h) / 2 - y1);
        Pic_Set_Remap_Flag(bm);
        Draw_Bitmap(pic_x, pic_y, bm);
    }
    Font_Select_Shadow_UL(1, 0, 0, 0);
    Set_Line_Spacing(0);
    Set_Font_Color(26);
    Set_Space_Width(1);
    type = Paragraph_Used_Width(44, p_city_screen_text, PARA_LEFT);
    if (type > 42 || type <= 0) {
        Font_Select_Shadow_UL(0, 0, 0, 0);
        Set_Line_Spacing(0);
        Set_Space_Width(1);
    }
    if (Paragraph_Height(44, p_city_screen_text) > 6) {
        Font_Select_Shadow_UL(0, 0, 0, 0);
        Set_Line_Spacing(0);
        Set_Space_Width(1);
    }
    Set_Font_Color(26);
    Set_Text_Shadow_Color(4);
    type = Paragraph_Height(44, p_city_screen_text);
    if (type <= 0)
        Print_Text(216, 179, p_city_screen_text);
    else {
        type = 179 - (type - 6);
        Paragraph_Print(216, type, 44, p_city_screen_text, PARA_CENTER);
    }
    Set_Text_Shadow_Color(0);
}

/* () the screen's controls: clears them, drops the map controls, adds the garrison slots (-100 when empty), the
   enchantment controls, the cityscape buildings, the buttons, the citizens and the resource rows */
void CityScr_Add_Controls(void)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int i;

    UI_Clear_Controls();
    cityscape_hotspot_shown = 0;
    map_grid_control = UI_NO_CONTROL;
    minimap_control = UI_NO_CONTROL;
    cityscr_unused_bf94 = 0;
    for (i = 0; i < MAX_STACK; i++)
        ui_unit_slot_controls[i] = UI_NO_SLOT;
    city_buy_button = UI_NO_CONTROL;
    city_change_button = UI_NO_CONTROL;
    city_ok_button = UI_NO_CONTROL;
    cityscr_unused_bf88 = UI_NO_CONTROL;
    for (i = 0; i < stack_unit_count; i++) {
        CityScr_Unit_Slot_Rect(i, &x1, &y1, &x2, &y2);
        ui_unit_slot_controls[i] = UI_Add_Region(x1 + 1, y1 + 1, x2 + 1, y2 + 1, "", -1);
    }
    CityScr_Add_Enchant_Controls(139, 51);
    CityScr_Add_Cityscape_Hotspots();
    CityScr_Add_Buttons();
    CityScr_Add_Citizen_Controls(city_screen_city, 4, 27);
    CityScr_Add_Resource_Controls(city_screen_city, 6, 52);
}

/* () a control for each cityscape hotspot (its rectangle), ids in city_hotspot_controls */
void CityScr_Add_Cityscape_Hotspots(void)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int i;

    cityscape_hotspot_shown = 0;
    for (i = 0; i < cityscape_hotspot_count; i++) {
        x1 = p_cityscape_hotspots[i].x1;
        y1 = p_cityscape_hotspots[i].y1;
        x2 = p_cityscape_hotspots[i].x2;
        y2 = p_cityscape_hotspots[i].y2;
        city_hotspot_controls[cityscape_hotspot_shown] = UI_Add_Region(x1, y1, x2, y2, "", -1);
        cityscape_hotspot_shown++;
    }
}

/* () Buy (unless disabled), Change and OK buttons, the Esc hotkey */
void CityScr_Add_Buttons(void)
{
    city_buy_button = UI_NO_CONTROL;
    if (cityscr_buy_disabled == 0)
        city_buy_button = UI_Add_Button(214, 188, "", city_buy_button_pic, "B", -1);
    city_change_button = UI_Add_Button(247, 188, "", city_change_button_pic, "C", -1);
    city_ok_button = UI_Add_Button(286, 188, "", city_ok_button_pic, "O", -1);
    city_esc_hotkey = UI_Add_Hotkey("\x1b");
}

/* () Buy (or its greyed picture), Change, OK and, with 7 or more enchantments, the scroll arrows */
void CityScr_Draw_Buttons(void)
{
    if (cityscr_buy_disabled == 0) {
        Set_Pic_Frame(city_buy_button_pic, 0);
        Draw_Pic_Frame(214, 188, city_buy_button_pic);
    } else
        Draw_Pic_Frame(214, 188, city_buy_greyed_pic);
    Set_Pic_Frame(city_change_button_pic, 0);
    Draw_Pic_Frame(247, 188, city_change_button_pic);
    Set_Pic_Frame(city_ok_button_pic, 0);
    Draw_Pic_Frame(286, 188, city_ok_button_pic);
    if (city_ench_scroll == 1) {
        Set_Pic_Frame(city_ench_up_arrow_pic, 0);
        Draw_Pic_Frame(201, 50, city_ench_up_arrow_pic);
        Set_Pic_Frame(city_ench_down_arrow_pic, 0);
        Draw_Pic_Frame(201, 85, city_ench_down_arrow_pic);
    }
}

/* (unused_a, unused_b, city, building) the "The <size> of <name> has completed the construction of a <building>." picture
   box (a, b and city unused: city_screen_city is used; walls drop their last letter); the event music; the
   cityscape then shows the building as new. The text names building_done_type, the picture is 'building' (the
   one caller, MOVE.C, passes building_done_type for it) */
void Popup_Building_Completed(int unused_a, int unused_b, int city, int building)
{
    int x1;
    int y1;
    int h;
    int w;
    int sound;
    int len;
    unsigned bm;
    int x;
    int y;

    Sound_Stop();
    Minimap_Alloc_Pic();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Map_Full_Redraw();
    Screen_Copy_Front();
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    bm = Heap_Alloc_Front(p_scratch, 500);
    strcpy(p_text_buf, "The ");
    strcat(p_text_buf, city_size_names[p_cities[city_screen_city].size]);
    strcat(p_text_buf, " of ");
    far_strcpy(p_text_buf2, 0, p_cities[city_screen_city].name);
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, " has completed the construction of ");
    far_strcpy(p_text_buf2, 0, p_building_data[building_done_type].name);
    strcat(p_text_buf, Text_Article_A_An(p_text_buf2));
    strcat(p_text_buf, " ");
    if (building_done_type == BLD_CITY_WALLS) {
        len = strlen(p_text_buf2);
        p_text_buf2[len - 1] = 0;
    }
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, ".");
    if (game_config.event_music == 1) {
        sound = LBX_Reload("MUSIC", MUSIC_BUILDING_FINISHED, p_music_buf);
        Sound_Play(sound);
    }
    x = 0;
    y = 8;
    if (building_done_type == BLD_CITY_WALLS)
        Pic_Decode_Frame_To_Bitmap(big_city_wall_pic, bm);
    else
        Pic_Decode_Frame_To_Bitmap(building_pics[building], bm);
    Pic_Get_Opaque_Bounds(bm, &x1, &y1, &w, &h);
    x = x + ((41 - w) / 2 - x1);
    y = y + ((43 - h) / 2 - y1);
    Screen_Copy_Front();
    if (building_done_type == BLD_CITY_WALLS)
        Notify_Picture_Box(160, 60, 2, p_text_buf, 0, city_done_backdrop_pic, 0, 8, big_city_wall_pic, x, y, 0, 0);
    else
        Notify_Picture_Box(160, 60, 2, p_text_buf, 0, city_done_backdrop_pic, 0, 8, building_pics[building], x, y, 0, 0);
    Music_Play_Current();
    Cityscape_Reveal_Reset();
    city_new_building = building;
}

/* (city) the "Name Starting City" entry: a default name for the city's race, edited in the box (an empty name
   keeps the default) */
void Dialog_Name_Starting_City(int city)
{
    int box;                                    /* written, never read */
    int box_x;
    int box_y;
    char name[16];
    char default_name[16];
    char colors[14];

    present_mode = PRESENT_NORMAL;
    colors[0] = 0x61;
    colors[1] = 0x61;
    colors[2] = 0x62;
    colors[3] = 0x62;
    colors[4] = 0x63;
    colors[5] = 0x63;
    colors[6] = 0x64;
    colors[7] = 0x64;
    colors[8] = 0x65;
    colors[9] = 0x65;
    colors[10] = 0x66;
    colors[11] = 0x66;
    colors[12] = 0x67;
    colors[13] = 0x67;
    box_x = 60;
    box_y = 30;
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(NameCity_Redraw, 1);
    Screen_Save_A800();
    City_Pick_Default_Name(p_cities[city].race, p_cities[city].name);
    far_strcpy(name, 0, p_cities[city].name);
    strcpy(default_name, name);
    UI_Clear_Controls();
    Font_Select(4, 4, 3, 3);
    Set_Space_Width(1);
    Set_Font_Color(0x67);
    box = UI_Text_Input_Box(box_x + 16, box_y + 21, 75, name, 12, 0, 0, 0, (int)colors, -1);   /* the colour array's address through an int parameter */
    Str_Trim(name);
    if (strlen(name) < 1)
        strcpy(name, default_name);
    far_strcpy(p_cities[city].name, name, 0);
    present_mode = PRESENT_DISSOLVE;
}

/* () the name entry's box and its "Name Starting City" title */
void NameCity_Redraw(void)
{
    int x;
    int y;

    x = 60;
    y = 30;
    Set_Draw_Page();
    Screen_Restore_A800();
    Draw_Pic_Frame(x, y, name_entry_box_pic);
    Font_Select(4, 4, 0, 0);
    Print_Text(x + 10, y + 6, "Name Starting City");
}

/* (city) the can-sell-back flag of buildings 1..35: 1 unless the city has it and City_Can_Sell_Building says no */
void CityScr_Build_Sell_Flags(int city)
{
    int required;
    int i;

    for (i = 1; i < NUM_BUILDINGS; i++) {
        p_cityscr_sell_flags[i] = 1;
        if (p_cities[city].buildings[i] >= 1 && City_Can_Sell_Building(i, city, &required) == 0)
            p_cityscr_sell_flags[i] = 0;
    }
}

/* (x, y unused) the garrison's unit icons in their slots (none: 0), then the outline color cycle */
int CityScr_Draw_Garrison(int x, int y)
{
    int unit;
    int x1;
    int y1;
    int x2;
    int y2;
    int i;

    if (top_unit_in_stack == -1)
        return 0;
    for (i = 0; i < stack_unit_count; i++) {
        unit = stack_units[i].unit;
        CityScr_Unit_Slot_Rect(i, &x1, &y1, &x2, &y2);
        Draw_Unit_Icon(x1, y1, unit, UNIT_ICON_ACTIVE);
    }
    Outline_Cycle_Tick();
}

/* () the screen's near buffers (texts, sell flags, control ids, enchantment lists, citizens row) */
void CityScr_Alloc_Buffers(void)
{
    int n;

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    p_city_screen_text = Near_Alloc(40);
    p_cityscr_sell_flags = (int *)Near_Alloc(72);
    n = 72;
    city_hotspot_controls = (int *)Near_Alloc(n * 2);
    p_city_ench_ctls = (int *)Near_Alloc(52);
    p_city_ench_slots = (int *)Near_Alloc(52);
    p_city_ench_owners = (int *)Near_Alloc(52);
    p_city_citizen_ctls = (int *)Near_Alloc(40);
}

/* () the minimap picture's buffer, and the product's unit figures when it is a unit */
void CityScr_Load_Minimap_And_Product(void)
{
    int unit_type;

    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS);
    if (p_cities[city_screen_city].producing >= PROD_UNIT_BASE) {
        unit_type = p_cities[city_screen_city].producing - PROD_UNIT_BASE;
        Load_Unit_Figure(unit_type, 1);
    }
}

/* (city, bldg, &x, &y, &w, &h, bm) the opaque bounds (Pic_Get_Opaque_Bounds: x, y, width, height) of a building's
   picture over all its frames (trade goods,
   the race's housing, walls' completion picture, else the cityscape picture), drawn into bm; it ends drawn at the
   frame it was on. BUG #434: a frame wins when its width or its height exceeds the best width (the height is compared with the width) */
void Building_Pic_Opaque_Bounds(int city, int bldg, int *x, int *y, int *w, int *h, unsigned bm)
{
    int frame;
    int frames;
    int best_x;
    int best_y;
    int best_w;
    int best_h;
    int fx;
    int fy;
    int fh;
    int fw;
    int frame_no;

    switch (bldg) {
    case BLD_TRADE_GOODS:
        frame = Pic_Get_Frame(cityscape_trade_goods_pic);
        frames = Pic_Frame_Count(cityscape_trade_goods_pic);
        break;
    case BLD_HOUSING:
        frame = Pic_Get_Frame(cityscape_housing_pics[race_table[p_cities[city].race].house_style]);
        frames = Pic_Frame_Count(cityscape_housing_pics[race_table[p_cities[city].race].house_style]);
        break;
    case BLD_CITY_WALLS:
        frame = Pic_Get_Frame(big_city_wall_pic);
        frames = Pic_Frame_Count(big_city_wall_pic);
        break;
    default:
        frame = Pic_Get_Frame(building_pics[bldg]);
        frames = Pic_Frame_Count(building_pics[bldg]);
        break;
    }
    best_x = 0;
    best_y = 0;
    best_h = 0;
    best_w = 0;
    for (frame_no = 0; frame_no < frames; frame_no++) {
        switch (bldg) {
        case BLD_TRADE_GOODS:
            Set_Pic_Frame(cityscape_trade_goods_pic, frame_no);
            Pic_Decode_Frame_To_Bitmap(cityscape_trade_goods_pic, bm);
            break;
        case BLD_HOUSING:
            Set_Pic_Frame(cityscape_housing_pics[race_table[p_cities[city].race].house_style], frame_no);
            Pic_Decode_Frame_To_Bitmap(cityscape_housing_pics[race_table[p_cities[city].race].house_style], bm);
            break;
        case BLD_CITY_WALLS:
            Set_Pic_Frame(big_city_wall_pic, frame_no);
            Pic_Decode_Frame_To_Bitmap(big_city_wall_pic, bm);
            break;
        default:
            Set_Pic_Frame(building_pics[bldg], frame_no);
            Pic_Decode_Frame_To_Bitmap(building_pics[bldg], bm);
            break;
        }
        Pic_Get_Opaque_Bounds(bm, &fx, &fy, &fw, &fh);
        if (fw > best_w || fh > best_w) {
            best_x = fx;
            best_y = fy;
            best_w = fw;
            best_h = fh;
        }
    }
    *x = best_x;
    *y = best_y;
    *h = best_h;
    *w = best_w;
    switch (bldg) {
    case BLD_TRADE_GOODS:
        Set_Pic_Frame(cityscape_trade_goods_pic, frame);
        Pic_Decode_Frame_To_Bitmap(cityscape_trade_goods_pic, bm);
        break;
    case BLD_HOUSING:
        Set_Pic_Frame(cityscape_housing_pics[race_table[p_cities[city].race].house_style], frame);
        Pic_Decode_Frame_To_Bitmap(cityscape_housing_pics[race_table[p_cities[city].race].house_style], bm);
        break;
    case BLD_CITY_WALLS:
        Set_Pic_Frame(big_city_wall_pic, frame);
        Pic_Decode_Frame_To_Bitmap(big_city_wall_pic, bm);
        break;
    default:
        Set_Pic_Frame(building_pics[bldg], frame);
        Pic_Decode_Frame_To_Bitmap(building_pics[bldg], bm);
        break;
    }
}

/* () Buy is disabled with under 2 turns left, for trade goods / housing, when the price exceeds the gold, or
   when nothing is left to pay */
void CityScr_Update_Buy_Disabled(void)
{
    cityscr_buy_disabled = 0;
    if (city_screen_turns_left < 2)
        cityscr_buy_disabled = 1;
    if (p_cities[city_screen_city].producing == PROD_HOUSING || p_cities[city_screen_city].producing == PROD_TRADE_GOODS)
        cityscr_buy_disabled = 1;
    else if (City_Buy_Price(city_screen_city) > wizards[current_player].gold)
        cityscr_buy_disabled = 1;
    if (city_screen_cost_left == 0)
        cityscr_buy_disabled = 1;
}

/* () over the 5x5 area map: the four corner squares shaded, a mark on each square shared with another city; then
   the fog and the unit grid */
void CityScr_Draw_Area_Map_Overlay(void)
{
    int sx;
    int sy;
    int mx;
    int my;
    int plane;
    int done;
    register int col;
    register int row;

    plane = map_view_plane;
    my = city_map_view_y;
    sy = 4;
    for (row = 0; row < 5; row++) {
        mx = city_map_view_x;
        sx = 215;
        for (col = 0; col < 5; col++) {
            done = 0;
            if ((row == 0 || row == 4) && (col == 0 || col == 4)) {
                Fill_Box_Styled(sx, sy, sx + 19, sy + 17, FILL_REMAP, 7);
                done = 1;
            }
            if (done == 0) {
                EMS_Map_Default();
                if (Tile_Is_Shared(mx, my, plane) == 1)
                    Draw_Pic_Frame(sx + 7, sy + 4, city_shared_tile_pic);
            }
            sx += 20;
            mx++;
            if (mx >= MAP_WIDTH)
                mx = 0;
        }
        sy += 18;
        my++;
    }
    Map_Draw_Fog(215, 4, 5, 5, city_map_view_x, city_map_view_y, map_view_plane);
    Map_Draw_Unit_Grid(215, 4, 5, 5, city_map_view_x, city_map_view_y);
}
