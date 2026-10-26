/* CITYLIST.C - WIZARDS.EXE overlay code segment 5767: the Cities screen (Main_Loop screen 3) - the current player's
   cities and outposts nine rows at a time with race, population, net gold, production, what is being built and the
   turns left; the highlighted city's name, enchantments and minimap below. Left-click a row: that city's screen (an
   outpost: the map, centred on it); right-click: the production screen (an outpost: the outpost screen).
   This module's data (DS:31f0): its literals; BSS (DS:c094..c0b9).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 5767 src/wizards/CITYLIST.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_Cities(void);
void CityList_Draw_Minimap(void);
void CityList_Load(void);
void CityList_Redraw(void);
void CityList_Build(void);
void CityList_Fill_Visible(void);
void CityList_Add_Controls(void);
void CityList_Calc_Visible_Rows(void);

#include "GAME.H"

int *p_citylist_visible;                        /* DS:c0b8  the city of each visible row */
int *p_citylist_all;                            /* DS:c0b6  every listed city, in city-table order */
int citylist_unused_c0b4;                       /* DS:c0b4  set to 16, never read */
int citylist_unused_c0b2;                       /* DS:c0b2  set to 16, never read */
int citylist_row_ctls[CITYLIST_ROWS];                       /* DS:c0a0  the row controls */
int citylist_row_ctl_count;                     /* DS:c09e */
int citylist_count;                             /* DS:c09c */
int citylist_saved_first_row;                   /* DS:c09a  the first row kept while the production screen is up */
int citylist_highlight;                         /* DS:c098  the row under the mouse */
int citylist_unused_c094[2];                    /* DS:c094  never referenced; owner open (no effect): these 4 BSS bytes sit
                                                   between MAINPANL.C (c092) and this module, either file gives
                                                   the same EXE */

extern struct RaceInfo race_table[14];          /* DS:1d86 */
extern unsigned citylist_ok_btn_pic;            /* DS:950e  RELOAD.LBX 0x16 */
extern unsigned citylist_down_arrow_pic;        /* DS:9510  ARMYLIST.LBX 2 */
extern unsigned citylist_up_arrow_pic;          /* DS:9512  ARMYLIST.LBX 1 */
extern unsigned citylist_background_pic;        /* DS:9514  RELOAD.LBX 0x15 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int city_ench_count;                     /* DS:bf9a */
extern int *p_city_ench_owners;                 /* DS:bf9c */
extern int *p_city_ench_slots;                  /* DS:bf9e */
extern int production_return_screen;            /* DS:bfb6 */
extern int citylist_unused_c0be;                /* DS:c0be */
extern int list_visible_rows;                   /* DS:c0c6  shared with the armies list (ARMYLIST.C) */
extern int list_first_row;                      /* DS:c0c8 */
extern unsigned p_minimap_pic;                  /* DS:c086 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Pic_Apply_Palette(unsigned pic);
void Remap_Build_Range(int first, int last);
void Remap_Build_Shadow(void);
void City_Garrison_List(int city, int *count, int *list);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
void Help_Load_City_List(void);
void Font_Select(int font, int c1, int c2, int c3);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Add_Hotkey(char *key);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
void Screen_Outpost(int is_new_outpost, int unused_zoom_x, int unused_zoom_y);
void Minimap_Draw_Blink(int px, int py, int w, int h, int plane, int cx, int cy);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Set_Space_Width(int width);
void Fill_Box_Styled();                           /* no prototype here: called with 6 arguments (the definition in GRAPHICS.C takes 9, read by style) */
int Print_Far_Text(int x, int y, char far *text);
int City_Remaining_Cost(int producing, int city);
int City_Turns_To_Complete(int cost, int city);
void Str_To_Upper(char *str);
void City_Get_Enchant_List(int city, int *slots, int *owners, int *count);
void City_Draw_Enchant_Names_2Col(int x, int y, int *slots, int *owners, int count);

/* () the Cities screen: OK / Esc leaves to the map; the arrows (U / D) scroll by five rows (down by one near the
   end); a row click opens the city (an outpost: centres the map on it), a right-click its production screen (an
   outpost: the outpost screen, then the list again). The garrison list read on entry is never used
   Numbered: BUG #100, #101 (BUGS_AND_QUIRKS.txt) */
void Screen_Cities(void)
{
    int done;
    int hit;
    int up_left;
    int down_left;
    int up_right;
    int down_right;
    int key_up;
    int key_down;
    int ok;
    int redraw;
    int key_esc;
    int garrison_count;
    int garrison[MAX_STACK];
    int i;
    int input;

    CityList_Load();
    Pic_Apply_Palette(citylist_background_pic);
    Palette_Upload();
    Palette_Unmark_Range(0, 0xf3);
    Palette_Mark_Range(0, 0xff);
    Remap_Build_Range(0, 1);
    citylist_unused_c0b4 = 16;
    citylist_unused_c0b2 = 16;
    CityList_Build();
    list_first_row = citylist_saved_first_row;
    CityList_Fill_Visible();
    City_Garrison_List(list_first_row, &garrison_count, garrison);   /* a row, not a city: the result is unused */
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(CityList_Redraw, 1);
    UI_Input_Delay(1);
    CityList_Draw_Minimap();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    CityList_Calc_Visible_Rows();
    citylist_highlight = 0;
    Help_Load_City_List();
    UI_Input_Delay(2);
    redraw = 0;
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        CityList_Add_Controls();
        Font_Select(0, 0, 6, 15);
        ok = UI_Add_Button(239, 182, "", citylist_ok_btn_pic, "O", -1);
        key_esc = UI_Add_Hotkey("\x1b");
        if (list_first_row == 0) {
            help_entries[0].id = -1;
            help_entries[2].id = -1;
            key_up = UI_NO_CONTROL;
            up_left = UI_NO_CONTROL;
            up_right = UI_NO_CONTROL;
        } else {
            help_entries[0].id = HELP_UP_ARROW;
            help_entries[2].id = HELP_UP_ARROW;
            key_up = UI_Add_Hotkey("U");
            up_left = UI_Add_Button(11, 26, "", citylist_up_arrow_pic, "", -1);
            up_right = UI_Add_Button(299, 26, "", citylist_up_arrow_pic, "", -1);
        }
        if (list_first_row + CITYLIST_ROWS == citylist_count || list_visible_rows == citylist_count) {
            help_entries[1].id = -1;
            help_entries[3].id = -1;
            key_down = UI_NO_CONTROL;
            down_left = UI_NO_CONTROL;
            down_right = UI_NO_CONTROL;
        } else {
            help_entries[1].id = HELP_DOWN_ARROW;
            help_entries[3].id = HELP_DOWN_ARROW;
            key_down = UI_Add_Hotkey("D");
            down_left = UI_Add_Button(11, 139, "", citylist_down_arrow_pic, "", -1);
            down_right = UI_Add_Button(299, 139, "", citylist_down_arrow_pic, "", -1);
        }
        input = UI_Get_Input();
        hit = UI_Hit_Test();
        if (input == -1 || input == ok || input == key_esc) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_MAP;
        }
        if (input == up_left || input == up_right || input == key_up) {
            Play_Click_Sound();
            if (list_first_row != 0) {
                if (list_first_row - CITYLIST_SCROLL >= 0)
                    list_first_row -= CITYLIST_SCROLL;
                else
                    list_first_row = 0;
                CityList_Fill_Visible();
                redraw = 1;
            }
        }
        if (input == down_left || input == down_right || input == key_down) {
            Play_Click_Sound();
            if (list_first_row + CITYLIST_ROWS < citylist_count) {
                if (list_first_row + (CITYLIST_ROWS + CITYLIST_SCROLL) < citylist_count)
                    list_first_row += CITYLIST_SCROLL;
                else
                    list_first_row++;
                CityList_Fill_Visible();
                redraw = 1;
            }
        }
        for (i = 0; i < citylist_row_ctl_count; i++) {
            if (citylist_row_ctls[i] == input) {
                Play_Click_Sound();
                city_screen_city = p_citylist_visible[i];
                if (p_cities[city_screen_city].size == 0) {
                    map_view_plane = p_cities[city_screen_city].plane;
                    Map_Center_View(&map_view_x, &map_view_y, p_cities[city_screen_city].x,
                                    p_cities[city_screen_city].y, p_cities[city_screen_city].plane);
                    current_screen = SCREEN_MAP;
                } else {
                    map_view_plane = p_cities[city_screen_city].plane;
                    Map_Center_View(&map_view_x, &map_view_y, p_cities[city_screen_city].x,
                                    p_cities[city_screen_city].y, p_cities[city_screen_city].plane);
                    current_screen = SCREEN_CITY;
                }
                done = 1;
                break;
            }
        }
        for (i = 0; i < citylist_row_ctl_count; i++) {
            if (-citylist_row_ctls[i] == input) {
                Play_Click_Sound();
                citylist_saved_first_row = list_first_row;
                city_screen_city = p_citylist_visible[i];
                if (p_cities[city_screen_city].size == 0) {
                    Screen_Outpost(0, -1, -1);
                    UI_Set_Redraw_Fn(CityList_Redraw, 1);
                    CityList_Load();
                    CityList_Build();
                    CityList_Fill_Visible();
                    City_Garrison_List(list_first_row, &garrison_count, garrison);   /* a row, not a city: the result is unused */
                    CityList_Draw_Minimap();
                    redraw = 1;
                } else {
                    current_screen = SCREEN_CITY_PRODUCTION;
                    production_return_screen = PROD_RETURN_CITY_LIST;
                    done = 1;
                }
                break;
            }
        }
        for (i = 0; i < citylist_row_ctl_count; i++) {
            if (citylist_row_ctls[i] == hit) {
                citylist_highlight = i;
                break;
            }
        }
        if (done == 0 && redraw == 0) {
            CityList_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
        redraw = 0;
    }
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    Palette_Unmark_Range(0, 0xff);
    Palette_Mark_Range(0, 0xdf);
    Remap_Build_Shadow();
}

/* () the minimap (49 x 33 at 42,162) centred on the highlighted city
   Numbered: BUG #101 (BUGS_AND_QUIRKS.txt) */
void CityList_Draw_Minimap(void)
{
    int x;
    int y;
    int plane;

    x = p_cities[p_citylist_visible[citylist_highlight]].x;
    y = p_cities[p_citylist_visible[citylist_highlight]].y;
    plane = p_cities[p_citylist_visible[citylist_highlight]].plane;
    Minimap_Draw_Blink(42, 162, 49, 33, plane, x, y);
}

/* () near buffers (the city lists, text, the enchantment list) and the pictures: RELOAD.LBX 0x15 background (also
   the palette) and 0x16 OK button, ARMYLIST.LBX 1 / 2 arrows */
void CityList_Load(void)
{
    p_citylist_all = (int *)Near_Alloc_Reset(200);
    p_citylist_visible = (int *)Near_Alloc(18);
    p_text_buf = Near_Alloc(80);
    p_city_ench_slots = (int *)Near_Alloc(52);
    p_city_ench_owners = (int *)Near_Alloc(52);
    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS_SMALL);
    citylist_background_pic = LBX_Reload_Next("RELOAD", 0x15, p_scratch);
    citylist_ok_btn_pic = LBX_Reload_Next("RELOAD", 0x16, p_scratch);
    citylist_up_arrow_pic = LBX_Reload_Next("ARMYLIST", 1, p_scratch);
    citylist_down_arrow_pic = LBX_Reload_Next("ARMYLIST", 2, p_scratch);
}

/* () redraw callback of Screen_Cities: the arrows (when more than 9 cities; frame 1 at either end), "The Cities Of
   <wizard>", the headers, the treasury, a row per visible city - name, race, population, gold net of building
   upkeep, production, what is being built and the turns left - with the highlighted row over gradient boxes, and
   the highlighted city's name, enchantments (up to 8) and minimap */
void CityList_Redraw(void)
{
    int city;
    int n;
    int producing;
    int cost;
    int turns;
    char pal_b[6];
    char pal_a[6];
    int i;
    int y;

    CityList_Calc_Visible_Rows();
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, citylist_background_pic);
    if (list_first_row == 0 && citylist_count > CITYLIST_ROWS) {
        Set_Pic_Frame(citylist_up_arrow_pic, 1);
        Draw_Pic_Frame(11, 26, citylist_up_arrow_pic);
        Set_Pic_Frame(citylist_up_arrow_pic, 1);
        Draw_Pic_Frame(299, 26, citylist_up_arrow_pic);
    } else if (citylist_count > CITYLIST_ROWS) {
        Set_Pic_Frame(citylist_up_arrow_pic, 0);
        Draw_Pic_Frame(11, 26, citylist_up_arrow_pic);
        Set_Pic_Frame(citylist_up_arrow_pic, 0);
        Draw_Pic_Frame(299, 26, citylist_up_arrow_pic);
    }
    if (citylist_count > CITYLIST_ROWS && (list_first_row + CITYLIST_ROWS == citylist_count || list_visible_rows == citylist_count)) {
        Set_Pic_Frame(citylist_down_arrow_pic, 1);
        Draw_Pic_Frame(11, 139, citylist_down_arrow_pic);
        Set_Pic_Frame(citylist_down_arrow_pic, 1);
        Draw_Pic_Frame(299, 139, citylist_down_arrow_pic);
    } else if (citylist_count > CITYLIST_ROWS) {
        Set_Pic_Frame(citylist_down_arrow_pic, 0);
        Draw_Pic_Frame(11, 139, citylist_down_arrow_pic);
        Set_Pic_Frame(citylist_down_arrow_pic, 0);
        Draw_Pic_Frame(299, 139, citylist_down_arrow_pic);
    }
    strcpy(p_text_buf, "The Cities Of ");
    strcat(p_text_buf, wizards[current_player].wizard_name);
    for (i = 0; i < 5; i++)
        pal_a[i] = 0xed;
    Font_Select_Palette(4, pal_a);
    Print_Text_Centered(162, 5, p_text_buf);
    Print_Text_Centered(161, 5, p_text_buf);
    Print_Text_Centered(162, 4, p_text_buf);
    for (i = 0; i < 5; i++)
        pal_a[i] = 0xee;
    Font_Select_Palette(4, pal_a);
    Print_Text_Centered(161, 3, p_text_buf);
    Print_Text_Centered(160, 4, p_text_buf);
    for (i = 0; i < 5; i++)
        pal_a[i] = 0xef;
    Font_Select_Palette(4, pal_a);
    Print_Text_Centered(161, 4, p_text_buf);
    pal_b[0] = 0xec;
    pal_b[1] = 0xb1;
    pal_b[2] = 0xb2;
    pal_b[3] = 0xb1;
    pal_b[4] = 0xb0;
    Font_Select_Palette(4, pal_b);
    Print_Text_Centered(160, 3, p_text_buf);
    Set_Text_Shadow_Color(0xe7);
    pal_a[0] = 0xec;
    pal_a[1] = 0x81;
    pal_a[2] = 0x81;
    Font_Select_Palette(1, pal_a);
    Font_Select_Shadow(1, 15, 0, 0);
    Print_Text(32, 17, "Name");
    Print_Text(88, 17, "Race");
    Print_Text(136, 17, "Pop");
    Print_Text(154, 17, "Gold");
    Print_Text(176, 17, "Prd");
    Print_Text(197, 17, "Producing");
    Print_Text(272, 17, "Time");
    itoa(wizards[current_player].gold, p_text_buf, 10);
    strcat(p_text_buf, "GP");
    Print_Text_Centered(242, 172, p_text_buf);
    itoa(wizards[current_player].mana_reserve, p_text_buf, 10);
    strcat(p_text_buf, "MP");
    Print_Text_Centered(276, 172, p_text_buf);
    Set_Space_Width(1);
    for (i = 0; i < list_visible_rows; i++) {
        city = p_citylist_visible[i];
        if (i == citylist_highlight) {          /* empty branches: the original has them too */
        } else {
        }
        Set_Text_Shadow_Color(0xe7);
        if (i == citylist_highlight) {
            citylist_unused_c0be = 0;
            y = i * 14 + 25;
            Fill_Box_Styled(30, y + 1, 81, y + 9, FILL_REMAP, 1);
            Fill_Box_Styled(86, y + 1, 130, y + 9, FILL_REMAP, 1);
            Fill_Box_Styled(135, y + 1, 150, y + 9, FILL_REMAP, 1);
            Fill_Box_Styled(155, y + 1, 170, y + 9, FILL_REMAP, 1);
            Fill_Box_Styled(175, y + 1, 190, y + 9, FILL_REMAP, 1);
            Fill_Box_Styled(195, y + 1, 270, y + 9, FILL_REMAP, 1);
            Fill_Box_Styled(275, y + 1, 289, y + 9, FILL_REMAP, 1);
        }
        Print_Far_Text(31, i * 14 + 27, p_cities[city].name);
        Print_Text(87, i * 14 + 27, race_table[p_cities[city].race].name);
        Print_Number_Right(148, i * 14 + 27, p_cities[city].population);
        Print_Number_Right(168, i * 14 + 27, p_cities[city].gold_income - p_cities[city].building_upkeep);
        Print_Number_Right(188, i * 14 + 27, p_cities[city].production_income);
        producing = p_cities[city].producing;
        if (producing >= PROD_UNIT_BASE) {
            n = producing - PROD_UNIT_BASE;
            strcpy(p_text_buf, unit_types[n].name);
        } else {
            far_strcpy(p_text_buf, 0, p_building_data[producing].name);
        }
        Print_Text(196, i * 14 + 27, p_text_buf);
        cost = City_Remaining_Cost(producing, city);
        turns = City_Turns_To_Complete(cost, city);
        Print_Number_Right(287, i * 14 + 27, turns);
    }
    if (citylist_count > 0) {
        city = p_citylist_visible[citylist_highlight];
        Set_Text_Shadow_Color(0xe7);
        pal_a[0] = 0xec;
        pal_a[1] = 0x81;
        pal_a[2] = 0x81;
        Font_Select_Palette(1, pal_a);
        Font_Select_Shadow(1, 15, 0, 0);
        far_strcpy(p_text_buf, 0, p_cities[city].name);
        Str_To_Upper(p_text_buf);
        Print_Text(99, 158, p_text_buf);
        City_Get_Enchant_List(city, p_city_ench_slots, p_city_ench_owners, &city_ench_count);
        n = city_ench_count;
        if (n > 8)                              /* two columns of 4 */
            n = 8;
        City_Draw_Enchant_Names_2Col(100, 168, p_city_ench_slots, p_city_ench_owners, n);
        CityList_Draw_Minimap();
    }
}

/* () every city of current_player, in city-table order. Side effect: a listed city producing nothing (BUILDDAT 0)
   is set to Housing
   Numbered: BUG #98 (BUGS_AND_QUIRKS.txt) */
void CityList_Build(void)
{
    int city;

    citylist_count = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == current_player) {
            p_citylist_all[citylist_count] = city;
            citylist_count++;
            if (p_cities[city].producing == PROD_NONE)
                p_cities[city].producing = PROD_HOUSING;
        }
    }
}

/* () the visible rows' cities from the first row on */
void CityList_Fill_Visible(void)
{
    int i;

    CityList_Calc_Visible_Rows();
    for (i = 0; i < list_visible_rows; i++)
        p_citylist_visible[i] = p_citylist_all[i + list_first_row];
}

/* () one control per visible row (30, 26 + 14r) - (289, 34 + 14r) */
void CityList_Add_Controls(void)
{
    int i;
    int x1;
    int y1;
    int x2;
    int y2;

    citylist_row_ctl_count = 0;
    CityList_Calc_Visible_Rows();
    for (i = 0; i < list_visible_rows; i++) {
        x1 = 30;
        y1 = i * 14 + 26;
        x2 = x1 + 259;
        y2 = y1 + 8;
        citylist_row_ctls[citylist_row_ctl_count] = UI_Add_Region(x1, y1, x2, y2, "", -1);
        citylist_row_ctl_count++;
    }
}

/* () visible rows = min(first + 9, 9, count): first >= 0, so min(9, count) */
void CityList_Calc_Visible_Rows(void)
{
    list_visible_rows = list_first_row + CITYLIST_ROWS;
    if (list_visible_rows > CITYLIST_ROWS)
        list_visible_rows = CITYLIST_ROWS;
    if (list_visible_rows > citylist_count)
        list_visible_rows = citylist_count;
}
