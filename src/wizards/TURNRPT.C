/* TURNRPT.C - WIZARDS.EXE overlay code segment a9a3: the turn report - the outposts deserted / grown messages, then
   the parchment scroll (SCROLL.LBX) unrolling with this turn's random events (info-menu mode only), disbanded and
   killed units, city growth and deaths, destroyed buildings and dissipated spells, scrollable with U / D.
   This module's data (DS:6292): its literals; BSS (DS:c9cc..c9dd).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS a9a3 src/wizards/TURNRPT.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void TurnReport_Redraw(void);
void TurnReport_Load_Graphics(void);
int Screen_Turn_Report(int mode);
void TurnReport_Draw_Text(int mode);

#include "GAME.H"

/* this block also serves COMBEND.C's combat result scroll (scroll_popup_*, scroll_text_y, results_scroll_pic,
   combat_result_min_lines) */
int scroll_popup_y;                             /* DS:c9dc  the text's scroll offset, 127 - height .. 0 */
int turnreport_height;                          /* DS:c9da  the whole report's height in pixels */
int scroll_text_y;                              /* DS:c9d8  the text's top line */
int scroll_popup_height;                        /* DS:c9d6  the visible height, at most 127 */
int combat_result_min_lines;                    /* DS:c9d4  not used by this module */
int turnreport_anim_frame;                      /* DS:c9d2  0..6 while scrolling down */
unsigned results_scroll_pic;                    /* DS:c9d0  SCROLL.LBX 5 */
unsigned turnreport_up_arrow_pic;               /* DS:c9ce  SCROLL.LBX 0 */
unsigned turnreport_down_arrow_pic;             /* DS:c9cc  SCROLL.LBX 1 */

extern struct Events far *p_events;             /* DS:9998 */
extern char report_outposts_deserted_count;     /* DS:99c2 */
extern char report_outposts_deserted_names[][20];       /* DS:99c3 */
extern char report_outposts_grown_count;        /* DS:9adb */
extern unsigned char report_outposts_grown[];   /* DS:9adc */
extern char report_buildings_lost_count;              /* DS:9b42 */
extern struct ReportBldgLost report_buildings_lost[]; /* DS:9b44 */
extern char report_units_lost_count;            /* DS:9b94 */
extern struct ReportUnitLost report_units_lost[];       /* DS:9b96 */
extern char report_units_killed_count;          /* DS:9be6 */
extern struct ReportUnitKilled report_units_killed[];   /* DS:9be8 */
extern char report_city_growth_count;           /* DS:9c38 */
extern unsigned char report_city_growth[];      /* DS:9c39 */
extern char report_city_deaths_count;           /* DS:9c4d */
extern unsigned char report_city_deaths[];      /* DS:9c4e */
extern char report_unit_spells_lost_count;      /* DS:9c62 */
extern unsigned char report_unit_spells_lost[]; /* DS:9c63 */
extern char report_city_spells_lost_count;      /* DS:9c77 */
extern unsigned char report_city_spells_lost[]; /* DS:9c78 */
extern char report_globals_lost;                /* DS:9c8c */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */

void far_strcpy(char *dst, int dst_seg, char far *src);
void Screen_Copy_Front(void);
void Minimap_Alloc_Pic(void);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Map_Full_Redraw(void);
void Map_Redraw(void);
void Message_Box(char *text);
int Array_Delete_Near(int index, char *array, int record_size, int count);
void Anim_Load_To_EMS(char *lbx, int entry);
void Anim_EMS_Next_Frame(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void VGA_Present_Dissolve(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Play_Click_Sound_Casting(void);
void Heap_Release(unsigned block);
void Set_Space_Width(int width);
int Print_Text_Clipped(int x, int y, char *text);
int Print_Far_Text_Clipped(int x, int y, char far *text);
int Print_Text_Clipped_Centered(int x, int y, char *text);
int Print_Number_Clipped(int x, int y, int value);

/* () redraw callback: the scroll arrows while there is more above / below */
void TurnReport_Redraw(void)
{
    Screen_Copy_Front();
    if (scroll_popup_y < 0)
        Draw_Pic_Frame(156, 13, turnreport_up_arrow_pic);
    if (-turnreport_height + SCROLL_VIEW_HEIGHT < scroll_popup_y)
        Draw_Pic_Frame(156, 172, turnreport_down_arrow_pic);
}

/* () SCROLL.LBX 5 (the parchment), 0 / 1 (the arrows) */
void TurnReport_Load_Graphics(void)
{
    Minimap_Alloc_Pic();
    results_scroll_pic = LBX_Reload_Next("Scroll", 5, p_scratch);
    turnreport_up_arrow_pic = LBX_Reload_Next("Scroll", 0, p_scratch);
    turnreport_down_arrow_pic = LBX_Reload_Next("Scroll", 1, p_scratch);
}

/* (mode) -> 1 when there is nothing to report in mode 0 (end of turn), no return value otherwise: message boxes for
   deserted / grown outposts, then the report scroll (mode 1, from the info menu, always shows and adds the random
   events); a city that both grew and shrank is dropped from the deaths */
int Screen_Turn_Report(int mode)
{
    int j;
    int entries;
    int input;
    int done;
    int control;
    int up;
    int down;
    int lines;
    char city_name[20];
    int i;
    int headers;

    headers = 0;
    entries = 0;
    lines = 0;
    scroll_popup_y = 0;
    turnreport_anim_frame = 0;
    TurnReport_Load_Graphics();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Map_Full_Redraw();
    Screen_Copy_Front();
    for (i = 0; i < report_outposts_deserted_count; i++) {
        strcpy(message_text, "The outpost of ");
        strcat(message_text, report_outposts_deserted_names[i]);
        strcat(message_text, " has been deserted.");
        Message_Box(message_text);
    }
    for (i = 0; i < report_outposts_grown_count; i++) {
        strcpy(message_text, "The outpost of ");
        far_strcpy(city_name, 0, p_cities[report_outposts_grown[i]].name);
        strcat(message_text, city_name);
        strcat(message_text, " has grown into a hamlet.");
        Message_Box(message_text);
    }
    for (i = 0; i < report_city_growth_count; i++) {
        for (j = 0; j < report_city_deaths_count; j++) {
            if (report_city_growth[i] == report_city_deaths[j]) {
                Array_Delete_Near(j, report_city_deaths, 1, report_city_deaths_count);
                report_city_deaths_count = report_city_deaths_count - 1;
                j--;
            }
        }
    }
    entries = report_units_lost_count + report_city_growth_count + report_buildings_lost_count
              + report_unit_spells_lost_count + report_city_spells_lost_count + report_units_killed_count
              + report_globals_lost + report_city_deaths_count;
    if (mode == 1) {
        if (p_events->plague == EVENT_STATE_RUNNING && p_events->plague_player == PLAYER_HUMAN)
            lines++;
        if (p_events->population_boom == EVENT_STATE_RUNNING && p_events->boom_player == PLAYER_HUMAN)
            lines++;
        if (p_events->good_moon == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->bad_moon == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->conjunction_chaos == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->conjunction_nature == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->conjunction_sorcery == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->mana_short == EVENT_STATE_RUNNING)
            lines++;
        entries += lines;
    }
    if (entries == 0) {
        if (mode == 1)
            headers++;
        else
            return 1;
    }
    if (lines > 0)
        headers++;
    if (report_units_lost_count > 0)
        headers++;
    if (report_units_killed_count > 0)
        headers++;
    if (report_city_growth_count > 0)
        headers++;
    if (report_city_deaths_count > 0)
        headers++;
    if (report_buildings_lost_count > 0)
        headers++;
    if (report_unit_spells_lost_count > 0)
        headers++;
    if (report_city_spells_lost_count > 0)
        headers++;
    if (report_globals_lost > 0)
        headers++;
    scroll_popup_height = entries * 7 + headers * 12;
    turnreport_height = scroll_popup_height;
    if (scroll_popup_height > SCROLL_VIEW_HEIGHT)
        scroll_popup_height = SCROLL_VIEW_HEIGHT;
    scroll_text_y = 99 - scroll_popup_height / 2 + 2;   /* 99: the screen's middle row */
    UI_Clear_Controls();
    Anim_Load_To_EMS("scroll", 2);
    for (i = 0; i <= (scroll_popup_height + 7) / 8; i++) {
        Set_Draw_Page();
        Map_Invalidate();
        Map_Redraw();
        Set_Clip_Rect(0, 88 - i * 4, 319, i * 4 + 108);   /* unrolls from the middle: 4 px up and down a frame */
        Draw_Pic_Clipped(62, 29, results_scroll_pic);
        TurnReport_Draw_Text(mode);
        Reset_Clip_Rect();
        Anim_EMS_Next_Frame();
        if (i == 0) {
            Palette_Upload();
            VGA_Present_Dissolve();
        } else {
            Screen_Present();
        }
    }
    Screen_Copy_Front();
    UI_Set_Redraw_Fn(TurnReport_Redraw, 1);
    done = 0;
    if (turnreport_height > SCROLL_VIEW_HEIGHT) {
        up = UI_Add_Region(28, 9, 288, 25, "U", -1);
        down = UI_Add_Region(28, 170, 288, 190, "D", -1);
    } else {
        up = UI_NO_CONTROL;
        down = UI_NO_CONTROL;
    }
    control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    while (done == 0) {
        input = UI_Get_Input();
        if (input == control)
            done = 1;
        if (input == up && scroll_popup_y < 0) {
            Play_Click_Sound_Casting();
            Anim_Load_To_EMS("Scroll", 3);
            for (i = 0; i < 5 && scroll_popup_y < 0; i++) {
                Timer_Mark();
                scroll_popup_y = scroll_popup_y + 5;
                if (scroll_popup_y > 0)
                    scroll_popup_y = 0;
                Set_Draw_Page();
                Map_Invalidate();
                Map_Redraw();
                Set_Clip_Rect(0, 29, 319, scroll_popup_y + 156);
                Draw_Pic_Clipped(62, scroll_popup_y + 29, results_scroll_pic);
                Set_Clip_Rect(0, scroll_popup_y + 156, 319, 177);
                Draw_Pic_Clipped(62, scroll_popup_y + 156, results_scroll_pic);
                Set_Clip_Rect(0, 22, 319, 176);
                TurnReport_Draw_Text(mode);
                Reset_Clip_Rect();
                Anim_EMS_Next_Frame();
                if (scroll_popup_y < 0)
                    Draw_Pic_Frame(156, 13, turnreport_up_arrow_pic);
                if (-turnreport_height + SCROLL_VIEW_HEIGHT < scroll_popup_y)
                    Draw_Pic_Frame(156, 172, turnreport_down_arrow_pic);
                Screen_Present();
                Wait_Ticks(2);
            }
            Screen_Copy_Front();
        }
        if (input == down && -turnreport_height + SCROLL_VIEW_HEIGHT < scroll_popup_y) {
            Play_Click_Sound_Casting();
            Anim_Load_To_EMS("Scroll", 4);
            for (i = 0; i < 5 && -turnreport_height + SCROLL_VIEW_HEIGHT < scroll_popup_y; i++) {
                Timer_Mark();
                scroll_popup_y = scroll_popup_y - 6;
                if (-turnreport_height + SCROLL_VIEW_HEIGHT > scroll_popup_y)
                    scroll_popup_y = -turnreport_height + SCROLL_VIEW_HEIGHT;
                else
                    turnreport_anim_frame = (turnreport_anim_frame + 1) % 7;
                Set_Draw_Page();
                Map_Invalidate();
                Map_Redraw();
                Set_Clip_Rect(0, 29, 319, scroll_popup_y + 156);
                Draw_Pic_Clipped(62, scroll_popup_y + 29, results_scroll_pic);
                Set_Clip_Rect(0, scroll_popup_y + 156, 319, 177);
                Draw_Pic_Clipped(62, scroll_popup_y + 156, results_scroll_pic);
                Set_Clip_Rect(0, 22, 319, 176);
                TurnReport_Draw_Text(mode);
                Reset_Clip_Rect();
                Anim_EMS_Next_Frame();
                if (scroll_popup_y < 0)
                    Draw_Pic_Frame(156, 13, turnreport_up_arrow_pic);
                if (-turnreport_height + SCROLL_VIEW_HEIGHT < scroll_popup_y)
                    Draw_Pic_Frame(156, 172, turnreport_down_arrow_pic);
                Screen_Present();
                Wait_Ticks(2);
            }
            Screen_Copy_Front();
        }
        if (done == 0) {
            Set_Draw_Page();
            TurnReport_Redraw();
            Screen_Present();
        }
    }
    UI_Suspend_Redraw_Fn();
    Heap_Release(p_scratch);
    Map_Full_Redraw();
    Screen_Copy_Front();
}

/* (mode) the report text at scroll_text_y + scroll_popup_y: the random events (mode 1), then each non-empty section
   with its heading; "NO EVENTS THIS MONTH" when the info-menu report is empty */
void TurnReport_Draw_Text(int mode)
{
    int tx;
    char pal_a[6];
    char pal_b[6];
    int x;
    int lines;
    char name[20];
    int y;
    int i;

    lines = 0;
    if (mode != 0) {
        if (p_events->plague == EVENT_STATE_RUNNING && p_events->plague_player == PLAYER_HUMAN)
            lines++;
        if (p_events->population_boom == EVENT_STATE_RUNNING && p_events->boom_player == PLAYER_HUMAN)
            lines++;
        if (p_events->good_moon == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->bad_moon == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->conjunction_chaos == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->conjunction_nature == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->conjunction_sorcery == EVENT_STATE_RUNNING)
            lines++;
        if (p_events->mana_short == EVENT_STATE_RUNNING)
            lines++;
    }
    for (i = 0; i < 5; i++)
        pal_a[i] = 0x2f;
    for (i = 0; i < 5; i++)
        pal_b[i] = 0x37;
    pal_b[0] = 0xf7;
    pal_a[0] = 0xf0;
    y = 0;
    if (lines > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(165, scroll_text_y + scroll_popup_y + y, "RANDOM EVENTS");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        if (p_events->plague == EVENT_STATE_RUNNING && p_events->plague_player == PLAYER_HUMAN) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Plague in ");
            Print_Far_Text_Clipped(x, scroll_text_y + scroll_popup_y + y, p_cities[p_events->plague_city].name);
            y += 7;
        }
        if (p_events->population_boom == EVENT_STATE_RUNNING && p_events->boom_player == PLAYER_HUMAN) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Population Boom in ");
            Print_Far_Text_Clipped(x, scroll_text_y + scroll_popup_y + y, p_cities[p_events->boom_city].name);
            y += 7;
        }
        if (p_events->good_moon == EVENT_STATE_RUNNING) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Good Moon");
            y += 7;
        }
        if (p_events->bad_moon == EVENT_STATE_RUNNING) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Bad Moon");
            y += 7;
        }
        if (p_events->conjunction_chaos == EVENT_STATE_RUNNING) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Red Conjunction");
            y += 7;
        }
        if (p_events->conjunction_nature == EVENT_STATE_RUNNING) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Green Conjunction");
            y += 7;
        }
        if (p_events->conjunction_sorcery == EVENT_STATE_RUNNING) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Blue Conjunction");
            y += 7;
        }
        if (p_events->mana_short == EVENT_STATE_RUNNING) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, "Mana Short");
            y += 7;
        }
        y += 2;
    }
    if (report_units_lost_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(165, scroll_text_y + scroll_popup_y + y, "UNITS DISBANDED");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        for (i = 0; i < report_units_lost_count; i++) {
            x = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, unit_types[report_units_lost[i].unit_type].name);
            switch (report_units_lost[i].reason) {
            case UNIT_LOST_FOOD:
                Print_Text_Clipped(x, scroll_text_y + scroll_popup_y + y, " - deserted: lack of food.");
                break;
            case UNIT_LOST_GOLD:
                Print_Text_Clipped(x, scroll_text_y + scroll_popup_y + y, " - deserted: lack of gold.");
                break;
            case UNIT_LOST_MANA:
                Print_Text_Clipped(x, scroll_text_y + scroll_popup_y + y, " - dispelled: lack of mana.");
                break;
            case UNIT_LOST_ROOM:
                Print_Text_Clipped(x, scroll_text_y + scroll_popup_y + y, " deserted to make room for new units");
                break;
            case UNIT_LOST_DROWNED:
                Print_Text_Clipped(x, scroll_text_y + scroll_popup_y + y,
                                   " drowned because unit carrying it was destroyed.");
                break;
            }
            y += 7;
        }
        y += 2;
    }
    if (report_units_killed_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(165, scroll_text_y + scroll_popup_y + y, "UNITS KILLED");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        for (i = 0; i < report_units_killed_count; i++) {
            if (report_units_killed[i].unit_type <= UNIT_HERO_CHOSEN)
                far_strcpy(message_text, 0, p_hero_names[report_units_killed[i].unit_type].name);
            else
                strcpy(message_text, unit_types[report_units_killed[i].unit_type].name);
            strcat(message_text, " killed by ");
            far_strcpy(name, 0, p_spell_data[report_units_killed[i].spell].name);
            strcat(message_text, name);
            if (report_units_killed[i].city >= 0) {
                strcat(message_text, " in ");
                far_strcpy(name, 0, p_cities[report_units_killed[i].city].name);
                strcat(message_text, name);
            }
            Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, message_text);
            y += 7;
        }
        y += 2;
    }
    if (report_city_growth_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "CITY GROWTH");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(1);
        for (i = 0; i < report_city_growth_count; i++) {
            tx = Print_Far_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, p_cities[report_city_growth[i]].name);
            tx = Print_Text_Clipped(tx + 1, scroll_text_y + scroll_popup_y + y, " has grown to a population of ");
            Print_Number_Clipped(tx + 1, scroll_text_y + scroll_popup_y + y, p_cities[report_city_growth[i]].population);
            y += 7;
        }
        y += 2;
    }
    if (report_city_deaths_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "CITY DEATHS");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        for (i = 0; i < report_city_deaths_count; i++) {
            tx = Print_Far_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, p_cities[report_city_deaths[i]].name);
            tx = Print_Text_Clipped(tx + 1, scroll_text_y + scroll_popup_y + y, " now has a population of ");
            Print_Number_Clipped(tx + 1, scroll_text_y + scroll_popup_y + y, p_cities[report_city_deaths[i]].population);
            y += 7;
        }
        y += 2;
    }
    if (report_buildings_lost_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "Destroyed Buildings");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        for (i = 0; i < report_buildings_lost_count; i++) {
            tx = Print_Far_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, p_cities[report_buildings_lost[i].city].name);
            tx = Print_Text_Clipped(tx + 1, scroll_text_y + scroll_popup_y + y, " has lost its ");
            tx = Print_Far_Text_Clipped(tx + 1, scroll_text_y + scroll_popup_y + y,
                                        p_building_data[report_buildings_lost[i].building].name);
            y += 7;
        }
        y += 2;
    }
    if (report_unit_spells_lost_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "UNIT SPELLS DISSIPATED");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        for (i = 0; i < report_unit_spells_lost_count; i++) {
            tx = Print_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, unit_types[report_unit_spells_lost[i]].name);   /* BUG #130: a unit index */
            y += 7;
        }
        y += 2;
    }
    if (report_city_spells_lost_count > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "CITY SPELLS DISSIPATED");
        y += 11;
        Font_Select_Palette(1, pal_b);
        Set_Space_Width(2);
        for (i = 0; i < report_city_spells_lost_count; i++) {
            tx = Print_Far_Text_Clipped(70, scroll_text_y + scroll_popup_y + y, p_cities[report_city_spells_lost[i]].name);
            y += 7;
        }
        y += 2;
    }
    if (report_globals_lost > 0) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "GLOBAL SPELLS DISSIPATED");
        y += 13;
    }
    if (y == 0 && mode == 1) {
        Font_Select_Palette(4, pal_a);
        Print_Text_Clipped_Centered(158, scroll_text_y + scroll_popup_y + y, "NO EVENTS THIS MONTH");
        y += 11;
    }
    Reset_Clip_Rect();
}
