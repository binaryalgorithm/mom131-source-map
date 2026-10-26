/* ADVISORS.C - WIZARDS.EXE overlay code segment 64e7 (stub 2482): the Info menu and its advisors - the "Select An
   Advisor" list (or F1-F9 directly), the Tax Collector's rate list, the Historian's power graph, the Astrologer's
   army / magic / research bars, the Cartographer's map of either plane (nodes, lairs, towers, units, cities, the
   city name under the mouse, a click centres the view) and the Grand Vizier toggle.
   This module's data (DS:37c0..39d5): its literals; BSS (DS:c296..c2a5): the advisor screens' pictures and offsets.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 64e7 src/wizards/ADVISORS.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_Info_Menu(int choice);
void Screen_Tax_Collector(void);
void Screen_Historian(void);
void Historian_Redraw(void);
void Historian_Draw_Wizard_Line(int wizard, int end, int start);
void Screen_Astrologer(void);
void Astrologer_Redraw(void);
void Screen_Cartographer(void);
int Cartographer_Redraw(void);
void Cartographer_Draw_Plane(int plane);
void Advisor_Grand_Vizier_Toggle(void);

#include "GAME.H"

/* the advisor menu's rows (F1 .. F9) */
#define ADVISOR_SURVEYOR 0
#define ADVISOR_CARTOGRAPHER 1
#define ADVISOR_APPRENTICE 2
#define ADVISOR_HISTORIAN 3
#define ADVISOR_ASTROLOGER 4
#define ADVISOR_CHANCELLOR 5
#define ADVISOR_TAX_COLLECTOR 6
#define ADVISOR_GRAND_VIZIER 7
#define ADVISOR_MIRROR 8

unsigned historian_bg_pic;                      /* DS:c2a4  RELOAD.LBX 0 */
unsigned astrologer_bg_pic;                     /* DS:c2a2  RELOAD.LBX 1 */
int astrologer_x;                               /* DS:c2a0  20 */
int astrologer_y;                               /* DS:c29e  11 */
int cartographer_map_x;                         /* DS:c29c  16 */
int cartographer_map_y;                         /* DS:c29a  27 */
unsigned cartographer_map_buf;                  /* DS:c298  the 240 x 160 map picture */
int cartographer_plane;                         /* DS:c296  the plane shown */

extern unsigned char banner_text_colors[];      /* DS:00c8  a text color per banner (ReMoM COL_Banners2; clarified using ReMoM repo) */
extern int present_mode;                        /* DS:9c8e */
extern unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb0 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int grand_vizier;                        /* DS:9d22 */
extern int current_screen;                      /* DS:9990 */
extern int stack_unit_count;                    /* DS:999c */
extern int map_view_plane;                      /* DS:bd86 */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int num_units;                           /* DS:bd92 */
extern int game_turn;                           /* DS:bd90 */
extern int num_cities;                          /* DS:bd94 */
extern char *p_text_buf2;                       /* DS:bfba */
extern char *p_text_buf;                        /* DS:bfbc */

/* Fill_Box_Styled has no prototype here: called with 5 of its 9 arguments */
void Fill_Box_Styled();
void Help_Load_Info_Menu(void);
void Help_Load_Tax_Collector(void);
void Help_Load_Historian(void);
void Help_Load_Astrologer(void);
void Help_Load_Cartographer(void);
int List_Select_Dialog(int count, char **names, int more, char *title);
void Screen_Surveyor(void);
void Screen_Apprentice(void);
void Screen_Wizard_Info(int wizard, int from_x1, int from_y1, int from_x2, int from_y2);
void Cities_Recalculate_All(void);
void Pic_Apply_Palette(unsigned pic);
void Remap_Build_Shadow(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Play_Click_Sound(void);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
void Set_Space_Width(int width);
int Print_Text_Centered(int x, int y, char *text);
void Print_Far_Text_Centered(int x, int y, char far *text);
int Print_Number(int x, int y, int value);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Put_Pixel(int x, int y, int color);
void Screen_Copy_Front(void);
void Screen_Restore_AC00(void);
int Mouse_X(void);
int Mouse_Y(void);
int UI_Add_Hotkey(char *key);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
void Stack_Clear_Draw_Priority(void);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Cartographer_Build_Map(int plane, unsigned buf);
void Draw_Bitmap(int x, int y, unsigned pic);
int Tile_Visible(int x, int y, int plane);
int Unit_Is_Invisible(int unit);
int Confirm_Box(char *text);

/* (choice: -1 the Info button's list, 0..8 F1..F9) an advisor: Surveyor, Cartographer, Apprentice, Historian,
   Astrologer, Chancellor (the turn report), Tax Collector, Grand Vizier ("*" when on), Mirror; back to the overland */
void Screen_Info_Menu(int choice)
{
    int pick;
    char fkey[4];
    char *names[10];
    char rows[10][26];
    int i;

    strcpy(fkey, " (F");
    if (choice == -1) {
        /* "\035" is TEXT_GOTO_X: the next byte is the x of the F-key column ('Q' 81, ';' 59, '6' 54, '4' 52) */
        strcpy(rows[0], "Surveyor");
        strcat(rows[0], "\035Q");
        strcat(rows[0], fkey);
        strcat(rows[0], "1)");
        strcpy(rows[1], "Cartographer");
        strcat(rows[1], "\035Q");
        strcat(rows[1], fkey);
        strcat(rows[1], "2)");
        strcpy(rows[2], "Apprentice");
        strcat(rows[2], "\035Q");
        strcat(rows[2], fkey);
        strcat(rows[2], "3)");
        strcpy(rows[3], "Historian");
        strcat(rows[3], "\035Q");
        strcat(rows[3], fkey);
        strcat(rows[3], "4)");
        strcpy(rows[4], "Astrologer");
        strcat(rows[4], "\035Q");
        strcat(rows[4], fkey);
        strcat(rows[4], "5)");
        strcpy(rows[5], "Chancellor");
        strcat(rows[5], "\035Q");
        strcat(rows[5], fkey);
        strcat(rows[5], "6)");
        strcpy(rows[6], "Tax Collector");
        strcat(rows[6], "\035Q");
        strcat(rows[6], fkey);
        strcat(rows[6], "7)");
        strcpy(rows[7], "Grand Vizier");
        if (grand_vizier == 1)
            strcat(rows[7], "*");
        strcat(rows[7], "\035Q");
        strcat(rows[7], fkey);
        strcat(rows[7], "8)");
        strcpy(rows[8], "Mirror");
        strcat(rows[8], "\035Q");
        strcat(rows[8], fkey);
        strcat(rows[8], "9)");
        strcpy(rows[9], "");
        for (i = 0; i < 10; i++)
            names[i] = rows[i];
        Help_Load_Info_Menu();
        pick = List_Select_Dialog(9, names, 0, "Select An Advisor");
        UI_Clear_Help();
    } else
        pick = choice;
    switch (pick) {
    case ADVISOR_SURVEYOR:
        Screen_Surveyor();
        break;
    case ADVISOR_CARTOGRAPHER:
        Screen_Cartographer();
        break;
    case ADVISOR_APPRENTICE:
        Screen_Apprentice();
        break;
    case ADVISOR_HISTORIAN:
        Screen_Historian();
        break;
    case ADVISOR_ASTROLOGER:
        Screen_Astrologer();
        break;
    case ADVISOR_CHANCELLOR:
        Screen_Turn_Report(1);
        break;
    case ADVISOR_TAX_COLLECTOR:
        Screen_Tax_Collector();
        break;
    case ADVISOR_GRAND_VIZIER:
        Advisor_Grand_Vizier_Toggle();
        break;
    case ADVISOR_MIRROR:
        Screen_Wizard_Info(0, 150, 60, 180, 90);
        break;
    }
    present_mode = PRESENT_DISSOLVE;
    current_screen = SCREEN_MAP;
}

/* () "Tax Per Population": 0 .. 3 gold per citizen (0 .. 75% unrest), "*" on the current rate; a pick sets it */
void Screen_Tax_Collector(void)
{
    int choice;
    char gold_suffix[10];
    char unrest_suffix[10];
    char *names[13];
    char rows[13][30];
    int i;
    int rate;

    strcpy(gold_suffix, " gold,");
    strcpy(unrest_suffix, "% unrest");
    rate = wizards[current_player].tax_rate;
    strcpy(rows[0], "0");
    strcat(rows[0], gold_suffix);
    strcat(rows[0], "\035;");
    strcat(rows[0], "0");
    strcat(rows[0], unrest_suffix);
    strcpy(rows[1], ".5");
    strcat(rows[1], gold_suffix);
    strcat(rows[1], "\0356");
    strcat(rows[1], "10");
    strcat(rows[1], unrest_suffix);
    strcpy(rows[2], "1");
    strcat(rows[2], gold_suffix);
    strcat(rows[2], "\0354");
    strcat(rows[2], "20");
    strcat(rows[2], unrest_suffix);
    strcpy(rows[3], "1.5");
    strcat(rows[3], gold_suffix);
    strcat(rows[3], "\0354");
    strcat(rows[3], "30");
    strcat(rows[3], unrest_suffix);
    strcpy(rows[4], "2");
    strcat(rows[4], gold_suffix);
    strcat(rows[4], "\0354");
    strcat(rows[4], "45");
    strcat(rows[4], unrest_suffix);
    strcpy(rows[5], "2.5");
    strcat(rows[5], gold_suffix);
    strcat(rows[5], "\0354");
    strcat(rows[5], "60");
    strcat(rows[5], unrest_suffix);
    strcpy(rows[6], "3");
    strcat(rows[6], gold_suffix);
    strcat(rows[6], "\0354");
    strcat(rows[6], "75");
    strcat(rows[6], unrest_suffix);
    if (rate > 6)
        rate = 6;
    for (i = 0; i < 7; i++) {
        if (i == rate)
            strcat(rows[i], "*");
        names[i] = rows[i];
    }
    strcpy(rows[7], "");
    names[7] = rows[7];
    Help_Load_Tax_Collector();
    choice = List_Select_Dialog(7, names, 0, " Tax Per Population ");
    UI_Clear_Help();
    if (choice != -1)
        wizards[current_player].tax_rate = choice;
    Cities_Recalculate_All();
}

/* () the Historian (RELOAD.LBX 0) until a click or Esc */
void Screen_Historian(void)
{
    int control;
    int input;
    int done;

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    historian_bg_pic = LBX_Reload("RELOAD", 0, p_scratch);
    Set_Draw_Page();
    Pic_Apply_Palette(historian_bg_pic);
    Palette_Mark_Range(0, 255);
    Remap_Build_Shadow();
    UI_Set_Redraw_Fn(Historian_Redraw, 1);
    UI_Clear_Help();
    Help_Load_Historian();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control) {
            Play_Click_Sound();
            done = 1;
        }
        if (done == 0) {
            Historian_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
    Reset_Clip_Rect();
    UI_Clear_Controls();
}

/* () "History Of Wizards Power": the date, the year axis (a tick a year, a label every 5 years), a line per known
   wizard with a fortress and the human's, the names in banner colors. The window is the last min(turn, 287)
   samples; after 24 years it starts 11 - month (23 - month before a multiple-of-5 year) samples in
   Numbered: BUG #94 (BUGS_AND_QUIRKS.txt) */
void Historian_Redraw(void)
{
    char colors[6];
    char banner_colors[6];
    int month;
    int year;
    int i;
    int end;
    int tick_x;
    int tick_y;
    int start;
    char space[4];
    char months[12][12];
    int first_year;
    int k;

    strcpy(space, " ");
    strcpy(months[0], "January");
    strcpy(months[1], "February");
    strcpy(months[2], "March");
    strcpy(months[3], "April");
    strcpy(months[4], "May");
    strcpy(months[5], "June");
    strcpy(months[6], "July");
    strcpy(months[7], "August");
    strcpy(months[8], "September");
    strcpy(months[9], "October");
    strcpy(months[10], "November");
    strcpy(months[11], "December");
    banner_colors[0] = 0xac;                    /* by BannerColor: blue, green, purple, red, yellow - the same literals in
                                                   the two graphs below; the Cartographer's list (banner_text_colors) differs in
                                                   purple (123) and yellow (179) */
    banner_colors[1] = 0xd8;
    banner_colors[2] = 0x7c;
    banner_colors[3] = 0xc9;
    banner_colors[4] = 0xd3;
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, historian_bg_pic);
    for (i = 1; i < 6; i++)
        colors[i] = i + 0xb1;
    Font_Select_Palette(4, colors);
    Font_Select_Shadow(4, 15, 0, 0);
    Set_Font_Color(0xf6);
    Set_Text_Shadow_Color(0xfe);
    Print_Text_Centered(160, 10, "History Of Wizards Power");
    year = game_turn / TURNS_PER_YEAR;
    month = game_turn % TURNS_PER_YEAR;
    strcpy(p_text_buf, months[month]);
    itoa(year + START_YEAR, p_text_buf2, 10);
    strcat(p_text_buf, space);
    strcat(p_text_buf, p_text_buf2);
    Font_Select_Shadow(2, 15, 0, 0);
    Set_Text_Shadow_Color(0xfe);
    Print_Text_Right(309, 11, p_text_buf);
    Draw_Line(16, 183, 303, 183, 0xb4);
    Fill_Box_Styled(16, 184, 303, 184, FILL_SHADOW);
    first_year = 0;
    start = 0;
    if (year > 23) {
        first_year = year - 23;
        start = 11 - month;
    }
    if ((first_year + 1) % 5 == 0) {
        first_year++;
        start = 23 - month;
    }
    Font_Select_Shadow(1, 0, 0, 0);
    Set_Text_Shadow_Color(0xfe);
    Print_Number(10, 186, first_year + START_YEAR);
    k = 1;
    if (game_turn < POWER_HISTORY_TURNS)
        end = game_turn;
    else
        end = POWER_HISTORY_TURNS - 1;
    for (i = 0; i < end; i++) {
        if (k % 12 == 0) {
            tick_x = k / 12 * 12 + 16;
            tick_y = 182;
            Draw_Line(tick_x, tick_y, tick_x, tick_y + 2, 0xb4);
            if ((first_year + k / 12) % 5 == 0)
                Print_Number(k / 12 * 12 + 10, 186, first_year + k / 12 + START_YEAR);
        }
        k++;
    }
    for (i = 1; i < num_players; i++)
        if (p_fortresses[i].active == 1 && wizards[current_player].contacted[i] == 1)
            Historian_Draw_Wizard_Line(i, end, start);
    Historian_Draw_Wizard_Line(current_player, end, start);
    colors[0] = banner_colors[wizards[PLAYER_HUMAN].banner_color];
    colors[1] = banner_colors[wizards[PLAYER_HUMAN].banner_color];
    colors[2] = banner_colors[wizards[PLAYER_HUMAN].banner_color];
    colors[3] = banner_colors[wizards[PLAYER_HUMAN].banner_color];
    colors[4] = banner_colors[wizards[PLAYER_HUMAN].banner_color];
    colors[5] = banner_colors[wizards[PLAYER_HUMAN].banner_color];
    Font_Select_Palette(3, colors);
    Font_Select_Shadow(3, 15, 0, 0);
    Set_Text_Shadow_Color(0xfe);
    Print_Text(10, 23, wizards[PLAYER_HUMAN].wizard_name);
    for (i = 1; i < num_players; i++) {
        if (wizards[current_player].contacted[i] == 1) {
            colors[0] = banner_colors[wizards[i].banner_color];
            colors[1] = banner_colors[wizards[i].banner_color];
            colors[2] = banner_colors[wizards[i].banner_color];
            colors[3] = banner_colors[wizards[i].banner_color];
            colors[4] = banner_colors[wizards[i].banner_color];
            colors[5] = banner_colors[wizards[i].banner_color];
            Font_Select_Palette(3, colors);
            Font_Select_Shadow(3, 15, 0, 0);
            Set_Text_Shadow_Color(0xfe);
            Print_Text(10, i * 9 + 23, wizards[i].wizard_name);
        }
    }
}

/* (wizard, end, start) the wizard's power_history[start .. end - 1] as a line from x 16, y 181 - value (at most up
   to y 22), in the banner color (BUG #94: the first point reads power_history[0], not [start])
   Numbered: BUG #94 (BUGS_AND_QUIRKS.txt) */
void Historian_Draw_Wizard_Line(int wizard, int end, int start)
{
    int i;
    int y1;
    int x1;
    int x2;
    int y2;
    int last;
    int x;
    unsigned char colors[6];

    colors[0] = 0xac;
    colors[1] = 0xd8;
    colors[2] = 0x7c;
    colors[3] = 0xc9;
    colors[4] = 0xd3;
    last = end;
    x = 0;
    for (i = start; i < last; i++) {
        x2 = x + 16;
        y2 = 181 - wizards[wizard].power_history[i];
        if (y2 < 22)
            y2 = 22;
        if (i > start) {
            Draw_Line(x1, y1, x2, y2, colors[wizards[wizard].banner_color]);
            x1 = x2;
            y1 = y2;
        } else {
            x1 = 16;
            y1 = 181 - wizards[wizard].power_history[0];
            if (y1 < 22)
                y1 = 22;
        }
        x++;
    }
}

/* () the Astrologer (RELOAD.LBX 1 at 20, 11) until a click or Esc */
void Screen_Astrologer(void)
{
    int control;
    int input;
    int done;

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    astrologer_bg_pic = LBX_Reload("RELOAD", 1, p_scratch);
    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Save_A800();
    Screen_Present();
    Set_Draw_Page();
    Pic_Apply_Palette(astrologer_bg_pic);
    Palette_Mark_Range(0, 255);
    Remap_Build_Shadow();
    Screen_Restore_A800();
    UI_Set_Redraw_Fn(Astrologer_Redraw, 1);
    UI_Clear_Help();
    Help_Load_Astrologer();
    astrologer_x = 20;
    astrologer_y = 11;
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control) {
            Play_Click_Sound();
            done = 1;
        }
        if (done == 0) {
            Astrologer_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
    UI_Clear_Controls();
}

/* () "Current Status Of Wizards": Army Strength, Magic Power and Spell Research of every known wizard (the human
   too), a bar of up to 200 pixels each in the banner color
   Numbered: BUG #95 (BUGS_AND_QUIRKS.txt) */
void Astrologer_Redraw(void)
{
    char pal[6];
    int dy;
    int player;
    int stats[3];
    int row;
    unsigned char colors[6];
    char space[4];
    int i;
    int y;

    strcpy(space, " ");
    colors[0] = 0xac;
    colors[1] = 0xd8;
    colors[2] = 0x7c;
    colors[3] = 0xc9;
    colors[4] = 0xd3;
    Set_Draw_Page();
    Draw_Pic_Frame(astrologer_x, astrologer_y, astrologer_bg_pic);
    for (player = 1; player < 6; player++)
        pal[player] = player + 0xb1;
    Font_Select_Palette(4, pal);
    Font_Select_Shadow(4, 15, 0, 0);
    Set_Font_Color(0xf6);
    Set_Text_Shadow_Color(0xfe);
    Print_Text_Centered(astrologer_x + 138, astrologer_y + 10, "Current Status Of Wizards");
    Font_Select_Shadow(2, 3, 0, 0);
    Set_Font_Color(0xf6);
    Set_Text_Shadow_Color(0xfe);
    strcpy(p_text_buf, "Army");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Strength");
    Print_Text_Centered(astrologer_x + 138, astrologer_y + 23, p_text_buf);
    strcpy(p_text_buf, "Magic");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Power");
    Print_Text_Centered(astrologer_x + 138, astrologer_y + 72, p_text_buf);
    strcpy(p_text_buf, "Spell");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Research");
    Print_Text_Centered(astrologer_x + 138, astrologer_y + 121, p_text_buf);
    Font_Select_Shadow(0, 15, 0, 0);
    Set_Font_Color(0xf6);
    Set_Text_Shadow_Color(0xfe);
    row = 0;
    for (player = 0; player < num_players; player++) {
        if (wizards[current_player].contacted[player] == 1) {
            dy = 35;
            stats[0] = wizards[player].stat_army_strength;
            stats[1] = wizards[player].stat_magic_power;
            stats[2] = wizards[player].stat_spell_research;
            for (i = 0; i < 3; i++) {
                if (stats[i] > 200)
                    stats[i] = 200;
                y = astrologer_y + dy + row * 7;
                Print_Text(astrologer_x + 14, y, wizards[player].wizard_name);
                Fill_Box_Styled(astrologer_x + 64, y + 2, astrologer_x + stats[i] + 65, y + 3, FILL_SHADOW);
                Fill_Rect(astrologer_x + 64, y + 1, astrologer_x + stats[i] + 64, y + 2,
                          colors[wizards[player].banner_color]);
                dy += 49;
            }
            row++;
        }
    }
}

/* () the Cartographer: both planes drawn once into store pages, N toggles the plane, C / Esc leaves, a click on the
   map centres the overland view there and leaves */
void Screen_Cartographer(void)
{
    int close_control;
    int plane_control;
    int x;
    int y;
    int grid_control;
    int hotkey;
    int done;
    int input;

    p_text_buf = Near_Alloc_Reset(100);
    UI_Set_Redraw_Fn((void (far *)(void))Cartographer_Redraw, 1);
    UI_Clear_Help();
    cartographer_plane = map_view_plane;
    cartographer_map_x = 16;
    cartographer_map_y = 27;
    Help_Load_Cartographer();
    Cartographer_Draw_Plane(0);
    Cartographer_Draw_Plane(1);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        plane_control = UI_Add_Region(285, 180, 314, 199, "N", -1);
        close_control = UI_Add_Region(265, 143, 299, 169, "C", -1);
        hotkey = UI_Add_Hotkey("\x1b");
        grid_control = UI_Add_Grid_Control(cartographer_map_x, cartographer_map_y, 4, 4, 60, 40, &x, &y, -1);
        input = UI_Get_Input();
        if (input == plane_control) {
            Play_Click_Sound();
            cartographer_plane = 1 - cartographer_plane;
        }
        if (input == close_control || input == hotkey) {
            Play_Click_Sound();
            done = 1;
        }
        if (input == grid_control) {
            done = 1;
            map_view_plane = cartographer_plane;
            map_view_x = x;
            map_view_y = y;
            stack_unit_count = 0;
            Map_Center_View(&map_view_x, &map_view_y, x, y, map_view_plane);
            Stack_Blink_Off();
            Units_Set_Draw_Priority();
            Stack_Clear_Draw_Priority();
            Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
            Map_Invalidate();
        }
        if (done == 0) {
            Cartographer_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    present_mode = PRESENT_FADE_IN;
}

/* () -> 0 when the mouse is off the map: the plane's saved picture, and the name of an explored city under the mouse
   above it in its owner's color */
int Cartographer_Redraw(void)
{
    int x;
    int y;
    char name_pal[4];
    int text_x;
    int text_y;
    int banner_colors[6];
    int city;

    banner_colors[0] = 0xac;                    /* 0 and 3 literal, the rest from banner_text_colors (as recovered) */
    banner_colors[1] = banner_text_colors[1];
    banner_colors[2] = banner_text_colors[2];
    banner_colors[3] = 0xc9;
    banner_colors[4] = banner_text_colors[4];
    banner_colors[5] = banner_text_colors[5];
    Set_Draw_Page();
    switch (cartographer_plane) {
    case 0:
        Screen_Restore_A800();
        break;
    case 1:
        Screen_Restore_AC00();
        break;
    }
    x = Mouse_X();
    y = Mouse_Y();
    if (x < cartographer_map_x)
        return 0;
    if (y < cartographer_map_y)
        return 0;
    if (cartographer_map_x + 240 < x)
        return 0;
    if (cartographer_map_y + 160 < y)
        return 0;
    x = (x - cartographer_map_x) / 4;
    y = (y - cartographer_map_y) / 4;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].plane == cartographer_plane && p_cities[city].x == x && p_cities[city].y == y
            && p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0) {
            text_x = cartographer_map_x + x * 4;
            text_y = cartographer_map_y + y * 4;
            name_pal[0] = 1;
            name_pal[1] = banner_colors[wizards[p_cities[city].owner].banner_color];
            Font_Select_Palette(0, name_pal);
            Set_Text_Shadow_Color(1);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Far_Text_Centered(text_x + 2, text_y - 6, p_cities[city].name);
            break;
        }
    }
}

/* (plane) one plane's map page: RELOAD.LBX 2 background, the terrain (Cartographer_Build_Map), the legend (a flag and
   name per known wizard, then Neutral), "Arcanus Plane" / "Myrror Plane", explored nodes / lairs / towers, units on
   visible squares (others' only if not neutral or invisible), explored cities as 3 x 3 boxes; saved to a store page
   Numbered: BUG #97 (BUGS_AND_QUIRKS.txt) */
void Cartographer_Draw_Plane(int plane)
{
    unsigned pic;
    int px;
    int py;
    int row;                                    /* also set to 1 as a 'neutral city seen' flag, never read */
    char colors[14];
    unsigned banner_pics[6];
    int banner_colors[6];
    int i;

    banner_colors[0] = 0xac;                    /* 0 and 3 literal, the rest from banner_text_colors (as recovered) */
    banner_colors[1] = banner_text_colors[1];
    banner_colors[2] = banner_text_colors[2];
    banner_colors[3] = 0xc9;
    banner_colors[4] = banner_text_colors[4];
    banner_colors[5] = banner_text_colors[5];
    Set_Draw_Page();
    pic = LBX_Reload("RELOAD", 2, p_scratch);
    Draw_Pic_Frame(0, 0, pic);
    cartographer_map_buf = Heap_Alloc_Front(p_scratch, 0x9c4);
    for (i = 0; i < 6; i++)
        banner_pics[i] = LBX_Reload_Next("RELOAD", i + 3, p_scratch);
    Cartographer_Build_Map(plane, cartographer_map_buf);
    Draw_Bitmap(cartographer_map_x, cartographer_map_y, cartographer_map_buf + 16);
    Font_Select(1, 1, 0, 0);
    Set_Font_Color(0xee);
    Set_Text_Shadow_Color(0xfe);
    Set_Space_Width(1);
    row = 0;
    for (i = 0; i < num_players; i++) {
        if (wizards[current_player].contacted[i] == 1) {
            Draw_Pic_Frame(260, row * 9 + 80, banner_pics[wizards[i].banner_color]);
            Print_Text(269, row * 9 + 82, wizards[i].wizard_name);
            row++;
        }
    }
    Draw_Pic_Frame(260, row * 9 + 80, banner_pics[5]);
    Print_Text(269, row * 9 + 82, "Neutral");
    for (i = 1; i < 13; i++)
        colors[i] = 1;
    Font_Select_Palette(0, colors);
    Set_Font_Color(25);
    Font_Select_Outline(5, 15, 0, 0);
    Set_Font_Color(25);
    Set_Text_Shadow_Color(0xef);
    strcpy(p_text_buf, plane == PLANE_ARCANUS ? "Arcanus" : "Myrror");
    strcat(p_text_buf, " Plane");
    Print_Text_Centered(160, 12, p_text_buf);
    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].plane == plane && p_exploration_maps[p_nodes[i].plane][p_nodes[i].y][p_nodes[i].x] != 0) {
            px = cartographer_map_x + p_nodes[i].x * 4 + 2;
            py = cartographer_map_y + p_nodes[i].y * 4 + 1;
            Put_Pixel(px, py, 7);
        }
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].plane == plane && p_lairs[i].active == 1
            && p_exploration_maps[p_lairs[i].plane][p_lairs[i].y][p_lairs[i].x] != 0) {
            px = cartographer_map_x + p_lairs[i].x * 4 + 2;
            py = cartographer_map_y + p_lairs[i].y * 4 + 1;
            Put_Pixel(px, py, 7);
        }
    }
    for (i = 0; i < NUM_TOWERS; i++) {
        if (p_exploration_maps[plane][p_towers[i].y][p_towers[i].x] != 0) {
            px = cartographer_map_x + p_towers[i].x * 4 + 2;
            py = cartographer_map_y + p_towers[i].y * 4 + 1;
            Put_Pixel(px, py, 0x8c);            /* towers; nodes and lairs 7, units their banner colour */
        }
    }
    for (i = 0; i < num_units; i++) {
        if ((p_units[i].plane == plane || p_units[i].in_tower == 1)
            && Tile_Visible(p_units[i].x, p_units[i].y, p_units[i].plane) == 1) {
            px = cartographer_map_x + p_units[i].x * 4 + 2;
            py = cartographer_map_y + p_units[i].y * 4 + 1;
            if (p_units[i].owner != PLAYER_NONE) {
                if (p_units[i].owner == current_player)
                    Put_Pixel(px, py, banner_colors[wizards[p_units[i].owner].banner_color]);
                else if (p_units[i].owner == PLAYER_NEUTRAL) {
                    /* neutral units are not shown */
                } else if (Unit_Is_Invisible(i) != 1)
                    Put_Pixel(px, py, banner_colors[wizards[p_units[i].owner].banner_color]);
            }
        }
    }
    row = 0;
    for (i = 0; i < num_cities; i++) {
        if ((p_cities[i].plane == plane || p_cities[i].plane == PLANE_BOTH)
            && p_exploration_maps[p_cities[i].plane][p_cities[i].y][p_cities[i].x] != 0) {
            px = cartographer_map_x + p_cities[i].x * 4;
            py = cartographer_map_y + p_cities[i].y * 4;
            Fill_Rect(px + 1, py + 1, px + 3, py + 3, 1);   /* cities: color 1 */
            if (p_cities[i].owner != PLAYER_NEUTRAL)
                Fill_Rect(px, py, px + 2, py + 2, banner_colors[wizards[p_cities[i].owner].banner_color]);
            else {
                Fill_Rect(px, py, px + 2, py + 2, 16);
                row = 1;
            }
        }
    }
    switch (plane) {
    case 0:
        Screen_Save_A800();
        break;
    case 1:
        Screen_Save_AC00();
        break;
    }
}

/* () the Grand Vizier: off -> "Do you wish to allow the Grand Vizier to select what buildings your cities create?"
   (Yes turns him on); on -> off without asking */
void Advisor_Grand_Vizier_Toggle(void)
{
    if (grand_vizier == 0) {
        if (Confirm_Box("Do you wish to allow the Grand Vizier to select what buildings your cities create?") == 1)
            grand_vizier = 1;
    } else
        grand_vizier = 0;
}
