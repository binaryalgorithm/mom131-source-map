/* GAMEMENU.C - WIZARDS.EXE overlay code segment f294 (stub 2680): the in-game Game menu (the "Game" button of the
   main screen, screen 1) - LOAD.LBX with the 8 save slots (a slot can be loaded only if its SAVE<n>.GAM exists, any
   slot can be saved to; names from game_config.save_names), Quit (Save_Game(8) = the continue save, then back to
   MAGIC.EXE's main menu with the "JENNY" argument), Load, Save, Settings and OK / Esc; and the state rebuild after
   a game is loaded (Game_Rebuild_After_Load).
   This module's data (DS:712c): its literals ("" is the tail of "LOAD.LBX": -d); BSS (DS:d496): game_menu_save_pic.
   The menu's other pictures and controls are SETTINGS.C's (the settings screen shares them).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS f294 src/wizards/GAMEMENU.C */
#include <stdlib.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

unsigned game_menu_save_pic;                    /* DS:d496  LOAD.LBX 3 "Save active" */

extern struct MouseRegion mouse_region_menu;    /* DS:60d4 */
extern int menu_return_mode;                    /* DS:60e0  MENU_RETURN_* */
extern int path_cache_index;                    /* DS:6f76 */
extern int *p_ai_replan_timer;                  /* DS:8faa  per player */
extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern unsigned p_scratch;                      /* DS:998e  segment of the scratch buffer */
extern int current_screen;                      /* DS:9990 */
extern int stack_unit_count;                    /* DS:999c */
extern struct Wizard wizards[6];                /* DS:9eca */
extern int present_mode;                        /* DS:9c8e */
extern struct PathCacheEntry far *p_path_cache; /* DS:9c90 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int current_player;                      /* DS:bd8c */
extern int difficulty;                          /* DS:bd96 */
extern int num_players;                         /* DS:bd9c */
extern struct GameConfig game_config;           /* DS:bd9e  MAGIC.SET, read / written whole */
extern int game_menu_settings_control;          /* DS:c980 */
extern unsigned game_menu_save_off_pic;         /* DS:c99a */
extern int save_slot_list[8];                   /* DS:c99c */
extern int game_menu_save_control;              /* DS:c9ac */
extern int game_menu_quit_control;              /* DS:c9ae */
extern int settings_ok_control;                 /* DS:c9b0 */
extern int game_menu_load_slot;                 /* DS:c9b2 */
extern int game_menu_save_slot;                 /* DS:c9b4 */
extern int game_menu_load_control;              /* DS:c9b6 */
extern int game_menu_first_frame;               /* DS:c9b8 */
extern unsigned game_menu_load_off_pic;         /* DS:c9ba */
extern unsigned game_menu_marker_pic;           /* DS:c9bc */
extern int save_slot_count;                     /* DS:c9be */
extern unsigned game_menu_settings_pic;         /* DS:c9c0 */
extern unsigned game_menu_text_fill_pic;        /* DS:c9c2 */
extern unsigned load_ok_pic;                    /* DS:c9c4 */
extern unsigned game_menu_load_pic;             /* DS:c9c6 */
extern unsigned game_menu_quit_pic;             /* DS:c9c8 */
extern unsigned load_background_pic;            /* DS:c9ca */

void Game_Menu_Redraw(void);
void Screen_Game_Menu(void);
void Help_Load_Game_Menu(void);
void Game_Rebuild_After_Load(void);

void Palette_Fade_To_Black(void);
void Palette_Fade_From_Black(void);
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
void Palette_Mark_Range(int first, int last);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
void Remap_Build_All(void);
void Fill_Rect(int x1, int y1, int x2, int y2, int color);
void Set_Draw_Page(void);
void Set_Draw_Page_Front(void);
void Screen_Copy_Front(void);
void Draw_Pic_Frame(int x, int y, unsigned pic);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
int DOS_Find_File(char *pattern, char *out);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
void UI_Present_Frame(void);
void Font_Select();                             /* no prototype here: this module calls it with 3 arguments (FONT.ASM takes 4) */
void Font_Select_Palette(int font, char *colors);
int Print_Text_Centered(int x, int y, char *text);
void Timer_Mark(void);
void Wait_Ticks(int ticks);
void UI_Clear_Controls(void);
void UI_Clear_Help(void);
void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Set_Help(struct HelpEntry *help, int count);
int UI_Add_Hotkey(char *key);
int UI_Add_Region(int x1, int y1, int x2, int y2, char *hotkey, int help);
int UI_Add_Text_Line();                         /* no prototype here: called with 8 arguments (the definition in UI.C takes 9) */
int UI_Get_Input(void);
int UI_Get_Pressed_Control(void);
void Quit_Hook_Empty(void);
void Save_Game(int slot);
void Load_Game(int slot);
void Exec_Program(char *path, char *arg0, char *arg1, char *arg2);
void Screen_Settings(void);
void City_Area_Bitmaps_Rebuild(void);
void Game_Session_Init(void);
void UnitTypes_Init_Upkeep_Sounds(void);
void Human_Cities_Area_Nop(void);
void Never_Set_Flag_Tiles_Collect(void);
void Cities_Recalc_All_Farmers(void);
void Music_Play_Current(void);
void Load_Overland_Screen_Graphics(void);
void Cities_Update_Road_Connections(void);
void Units_Compact(void);
void Units_Reset_Moves_NonCurrent(void);
void Overland_Resume_View(void);
void AI_Build_Landmass_Tile_Lists(void);
void AI_Build_Coast_Tile_Lists(void);

/* the Game menu. UI_Add_Text_Line is called with 8 of its 9 arguments (no caret colors)
   Numbered: BUG #373 (BUGS_AND_QUIRKS.txt) */
void Screen_Game_Menu(void)
{
    int done;
    int k;
    int hotkey;
    int input;
    int x;
    int shown;
    int loaded;
    int load_controls[8];
    char pattern[16];
    char num_text[16];
    char find_buf[20];
    int i;
    int y;

    loaded = 0;
    game_menu_first_frame = 1;
    if (menu_return_mode != MENU_RETURN_NO_FADE) {
        Palette_Fade_To_Black();
        Fill_Rect(0, 0, 319, 199, 0);
        Set_Draw_Page_Front();
        Fill_Rect(0, 0, 319, 199, 0);
        Set_Draw_Page();
        Palette_Load(1, -1);
    } else if (menu_return_mode != MENU_RETURN_NO_FADE)           /* never true */
        menu_return_mode = MENU_RETURN_MAP;
    load_background_pic = LBX_Reload("LOAD.LBX", 0, p_scratch);
    game_menu_quit_pic = LBX_Reload_Next("LOAD.LBX", 2, p_scratch);
    game_menu_load_pic = LBX_Reload_Next("LOAD.LBX", 1, p_scratch);
    game_menu_save_pic = LBX_Reload_Next("LOAD.LBX", 3, p_scratch);
    load_ok_pic = LBX_Reload_Next("LOAD.LBX", 4, p_scratch);
    game_menu_load_off_pic = LBX_Reload_Next("LOAD.LBX", 6, p_scratch);
    game_menu_save_off_pic = LBX_Reload_Next("LOAD.LBX", 7, p_scratch);
    game_menu_text_fill_pic = LBX_Reload_Next("LOAD.LBX", 9, p_scratch);
    game_menu_marker_pic = LBX_Reload_Next("LOAD.LBX", 10, p_scratch);
    game_menu_settings_pic = LBX_Reload_Next("LOAD.LBX", 12, p_scratch);
    save_slot_count = 0;
    for (i = 1; i < 9; i++) {
        strcpy(pattern, "SAVE");
        itoa(i, num_text, 10);
        strcat(pattern, num_text);
        strcat(pattern, ".GAM");
        if (DOS_Find_File(pattern, find_buf) != 0) {
            save_slot_list[save_slot_count] = i - 1;
            save_slot_count++;
        } else
            save_slot_list[save_slot_count] = -1;
    }
    done = 0;
    game_menu_load_slot = -1;
    game_menu_save_slot = -1;
    shown = 0;
    x = 43;
    y = 171;
    Mouse_Set_Cursor_Regions(1, &mouse_region_menu);
    UI_Clear_Controls();
    hotkey = UI_Add_Hotkey("\x1b");
    game_menu_quit_control = UI_Add_Region(x, y, x + 39, y + 13, "", -1);
    game_menu_load_control = UI_Add_Region(x + 40, y, x + 78, y + 13, "", -1);
    game_menu_save_control = UI_Add_Region(122, y, 159, y + 13, "", -1);
    settings_ok_control = UI_Add_Region(231, y, 270, y + 13, "", -1);
    game_menu_settings_control = UI_Add_Region(172, y, 229, y + 13, "", -1);
    Font_Select(3, 1, 3);
    for (i = 0; i < 8; i++)
        load_controls[i] = UI_Add_Text_Line(x, i * 15 + 47, 260, game_config.save_names[i], 19, 0,
                                            game_menu_marker_pic, -1);
    game_menu_load_slot = -1;
    done = 0;
    UI_Set_Redraw_Fn(Game_Menu_Redraw, 1);
    Help_Load_Game_Menu();
    while (done == 0) {
        input = UI_Get_Input();
        Timer_Mark();
        for (i = 0; i < 8; i++) {
            if (load_controls[i] == input) {
                game_menu_save_slot = i;
                game_menu_load_slot = -1;
                for (k = 0; k < save_slot_count; k++)
                    if (save_slot_list[k] == i)
                        game_menu_load_slot = i;
            }
        }
        if (input == game_menu_quit_control) {
            menu_return_mode = MENU_RETURN_MAP;
            Quit_Hook_Empty();
            Save_Game(SAVE_SLOT_CONTINUE);
            Exec_Program("MAGIC.EXE", "JENNY", "", "");   /* JENNY: MAGIC.EXE skips the intro */
            done = 1;
        }
        if (input == settings_ok_control || input == hotkey) {
            menu_return_mode = MENU_RETURN_MAP;
            done = 1;
        }
        if (input == game_menu_load_control && game_menu_load_slot >= 0) {
            menu_return_mode = MENU_RETURN_MAP;
            UI_Clear_Help();
            Load_Game(game_menu_load_slot);
            loaded = 1;
            done = 1;
        }
        if (input == game_menu_save_control && game_menu_save_slot >= 0) {
            menu_return_mode = MENU_RETURN_MAP;
            Save_Game(game_menu_save_slot);
            done = 1;
        }
        if (input == game_menu_settings_control) {
            menu_return_mode = MENU_RETURN_FROM_MENU;
            Screen_Settings();
            done = 1;
        }
        if (done == 0) {
            Game_Menu_Redraw();
            UI_Present_Frame();
            if (game_menu_first_frame != 0 && shown == 0) {
                Screen_Copy_Front();
                if (menu_return_mode != MENU_RETURN_NO_FADE)
                    Palette_Fade_From_Black();
                shown = 1;
            }
            Wait_Ticks(1);
        }
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    if (menu_return_mode == MENU_RETURN_MAP)
        current_screen = SCREEN_MAP;
    if (current_screen == SCREEN_MAP) {
        present_mode = PRESENT_FADE_IN;
        Palette_Fade_To_Black();
        Fill_Rect(0, 0, 319, 199, 0);
        Set_Draw_Page_Front();
        Fill_Rect(0, 0, 319, 199, 0);
        Set_Draw_Page();
        Palette_Load(0, -1);
        Palette_Mark_Range(0, 255);
        Remap_Build_All();
        Palette_Cycle_Color(PAL_CYCLE_COLOR, 40, 0, 0, 63, 0, 0, 1);
    }
    if (loaded == 1)
        Game_Rebuild_After_Load();
}

/* redraw callback of the Game menu: background, the slot markers of existing saves, the buttons (pressed: one
   pixel down-right; Load / Save greyed while no usable slot is selected) and the copyright line */
void Game_Menu_Redraw(void)
{
    int x;
    int y;
    char colors[2];
    int pressed;
    int i;

    colors[0] = 34;
    colors[1] = 43;
    pressed = UI_Get_Pressed_Control();
    x = 43;
    y = 171;                                    /* never used */
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    Draw_Pic_Frame(0, 0, load_background_pic);
    Fill_Rect(42, 170, 160, 184, 0);
    Fill_Rect(171, 170, 271, 184, 0);
    for (i = 0; i < save_slot_count; i++)
        Draw_Pic_Frame(x, save_slot_list[i] * 15 + 47, game_menu_text_fill_pic);
    if (pressed == game_menu_quit_control)
        Draw_Pic_Frame(44, 172, game_menu_quit_pic);
    else
        Draw_Pic_Frame(43, 171, game_menu_quit_pic);
    if (pressed == game_menu_settings_control)
        Draw_Pic_Frame(173, 172, game_menu_settings_pic);
    else
        Draw_Pic_Frame(172, 171, game_menu_settings_pic);
    if (game_menu_load_slot >= 0) {
        if (pressed == game_menu_load_control)
            Draw_Pic_Frame(84, 172, game_menu_load_pic);
        else
            Draw_Pic_Frame(83, 171, game_menu_load_pic);
    } else
        Draw_Pic_Frame(83, 171, game_menu_load_off_pic);
    if (game_menu_save_slot >= 0) {
        if (pressed == game_menu_save_control)
            Draw_Pic_Frame(123, 172, game_menu_save_pic);
        else
            Draw_Pic_Frame(122, 171, game_menu_save_pic);
    } else
        Draw_Pic_Frame(122, 171, game_menu_save_off_pic);
    if (pressed == settings_ok_control)
        Draw_Pic_Frame(232, 172, load_ok_pic);
    else
        Draw_Pic_Frame(231, 171, load_ok_pic);
    Font_Select_Palette(0, colors);
    Print_Text_Centered(160, 193, "Copyright  Simtex Software, 1995   V1.31");
}

/* the Game menu's right-click help: HLPENTRY.LBX entry 27 (6 records) */
void Help_Load_Game_Menu(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 27, help_entries, 0, 6, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 27);              /* BUG #462: 27 (the LBX entry), not 6: entries 6..26 keep the previous
                                                   screen's help rectangles */
}

/* rebuilds the run-time state after Load_Game: area bitmaps, the view, unit-type upkeep / sounds, the unit grid, road
   connections, unit list, moves; random events off on Intro; the AI path cache invalidated (the first 100 of its
   PATH_CACHE_SIZE records), the
   landmass / coast tile lists, the AI re-plan timers, and time_stop_player (the last player with Time Stop, + 1).
   The same work as WIZARDS.C Game_Start_Init, but for city_razed */
void Game_Rebuild_After_Load(void)
{
    int i;

    current_player = 0;
    present_mode = PRESENT_NORMAL;
    City_Area_Bitmaps_Rebuild();
    Game_Session_Init();
    UnitTypes_Init_Upkeep_Sounds();
    Human_Cities_Area_Nop();
    Never_Set_Flag_Tiles_Collect();
    Cities_Recalc_All_Farmers();
    stack_unit_count = 0;
    Music_Play_Current();
    Load_Overland_Screen_Graphics();
    Cities_Update_Road_Connections();
    Units_Compact();
    Units_Reset_Moves_NonCurrent();
    Overland_Resume_View();
    if (difficulty == DIFFICULTY_INTRO)
        game_config.random_events = 0;
    for (i = 0; i < 100; i++)                   /* the first 100 of the 140 records */
        p_path_cache[i].x1 = -1;
    path_cache_index = -1;
    AI_Build_Landmass_Tile_Lists();
    AI_Build_Coast_Tile_Lists();
    for (i = 0; i < num_players; i++)
        p_ai_replan_timer[i] = 0;
    time_stop_player = 0;
    for (i = 0; i < num_players; i++)
        if (wizards[i].globals.time_stop > 0)
            time_stop_player = i + 1;
}
