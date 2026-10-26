/* SETTINGS.C - WIZARDS.EXE code segment a8fc (MAGIC.EXE 3cfc, the same file): the settings screen and MAGIC.SET
   (struct GameConfig, read / written whole: 0x1d2 bytes). The 18 on/off options are toggled through p_settings_words
   as an int array over the start of game_config (+0x00 .. +0x22) - the order on screen is not the order in the file.
   Does NOT include GAME.H: Font_Select_Shadow is called with 3 arguments here and 4 elsewhere (no prototype in the
   original).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS a8fc src/wizards/SETTINGS.C */
#include <stdio.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

/* toggle i is GameConfig field i; 14 the spellbook ordering (resets both books' pages) */
#define SETTING_BOOK_ORDERING 14

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
void Settings_Redraw(void);
void Screen_Settings(void);
void Settings_Defaults(void);
void Help_Load_Settings(void);
void Settings_Load_Or_Create(void);

extern int research_first_page;                 /* DS:9448 */
extern int overland_spellbook_page;                            /* DS:944a */
extern int combat_spellbook_page;               /* DS:944c */
extern unsigned p_scratch;                      /* DS:998e */
extern int current_screen;                      /* DS:9990 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern struct GameConfig game_config;           /* DS:bd9e */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
unsigned load_background_pic;                   /* DS:c9ca  MAGIC DS:8a40   */
unsigned game_menu_quit_pic;                    /* DS:c9c8  MAGIC DS:8a3e  LOAD.LBX 2 "Quit active" (WIZARDS Screen_Game_Menu; batch 40, was a spare) */
unsigned game_menu_load_pic;                    /* DS:c9c6  MAGIC DS:8a3c  LOAD.LBX 1 "Load active" (Screen_Game_Menu) */
unsigned load_ok_pic;                           /* DS:c9c4  MAGIC DS:8a3a   */
unsigned game_menu_text_fill_pic;               /* DS:c9c2  MAGIC DS:8a38  LOAD.LBX 9 "Text Fill" (Screen_Game_Menu) */
unsigned game_menu_settings_pic;                /* DS:c9c0  MAGIC DS:8a36  LOAD.LBX 0xc "Settings button" (Screen_Game_Menu) */
int save_slot_count;                            /* DS:c9be  MAGIC DS:8a34  existing SAVE1..8.GAM files (Screen_Game_Menu) */
unsigned game_menu_marker_pic;                  /* DS:c9bc  MAGIC DS:8a32  LOAD.LBX 10 "selection marker" (Screen_Game_Menu) */
unsigned game_menu_load_off_pic;                /* DS:c9ba  MAGIC DS:8a30  LOAD.LBX 6 (entry name "Quit inactive"): the greyed Load button (Game_Menu_Redraw) */
int game_menu_first_frame;                      /* DS:c9b8  MAGIC DS:8a2e   */
int game_menu_load_control;                     /* DS:c9b6  MAGIC DS:8a2c  control id of the game menu's Load button: Load_Game(game_menu_load_slot) */
int game_menu_save_slot;                        /* DS:c9b4  MAGIC DS:8a2a  slot selected in the game menu (-1 none; Save greyed) */
int game_menu_load_slot;                        /* DS:c9b2  MAGIC DS:8a28  the selected slot if its save exists, else -1 (Load greyed) */
int settings_ok_control;                        /* DS:c9b0  MAGIC DS:8a26  the Ok control of the settings screen and of the game menu */
int game_menu_quit_control;                     /* DS:c9ae  MAGIC DS:8a24  control id of the game menu's Quit button: Save_Game(8) = the continue save, then leave */
int game_menu_save_control;                     /* DS:c9ac  MAGIC DS:8a22  control id of the game menu's Save button: Save_Game(game_menu_save_slot) */
int save_slot_list[8];                         /* DS:c99c  MAGIC DS:8a12   */
unsigned game_menu_save_off_pic;                /* DS:c99a  MAGIC DS:8a10  LOAD.LBX 7 "Save inactive" (Screen_Game_Menu; batch 40, was a spare) */
int *p_settings_words;                          /* DS:c998  MAGIC DS:8a0e   */
unsigned settings_check_on_pics[3];             /* DS:c992  MAGIC DS:8a08   */
unsigned settings_check_off_pics[3];            /* DS:c98c  MAGIC DS:8a02   */
unsigned settings_unused_pics[3];               /* DS:c986  MAGIC DS:89fc   */
                                                /* [0], [1] LOAD.LBX 19 / 20, loaded, never drawn; [2] never set (length
                                                   from the layout) */
int settings_unused_c982[2];                    /* DS:c982  MAGIC DS:89f8  never referenced */
int game_menu_settings_control;                 /* DS:c980  MAGIC DS:89f6  the game menu's Settings control id (GAMEMENU.C) */
                                                /* owner: this module by the better reading, not a proof (N214; was COMBEND.C, and MAGIC.C in MAGIC): both programs
                                                   have the same 6 bytes just below this block, read in WIZARDS only by GAMEMENU.C
                                                   and never in MAGIC, and ReMoM's layout starts ovr125's BSS at c980; any owner
                                                   gives the same bytes. clarified using ReMoM repo */

/* the first data of this module (link test, batch 36: they open its block in both programs) */
/* the settings and game menu screens': the same full-screen arrow as mouse_region_arrow */
struct MouseRegion mouse_region_menu = { 1, 0, 0, 0, 319, 199 };    /* DS:60d4 */
int menu_return_mode = MENU_RETURN_MAP;                      /* DS:60e0 */

long DOS_File_Size(char *name);
void Font_Select_Shadow();                     /* no prototype here: some calls pass 2 or 3 of the 4 words (QUIRK #371) */

/* () the settings screen: OK writes MAGIC.SET, Esc leaves without writing (the changes stay in memory); clicking
   Spell Book Ordering also resets the spellbook pages; back to the game menu when opened from it
   Numbered: BUG #106 (BUGS_AND_QUIRKS.txt) */
void Screen_Settings(void)
{
    int done;
    int input;
    int esc_control;
    int drawn;
    FILE *fp;
    int anim_control;
    int toggles[18];
    int row_y[13] = { 53, 64, 75, 102, 113, 124, 135, 153, 146, 157, 167, 163, 174 };   /* not in y order; rows 7
                                                   (153) and 10 (167) serve no control */
    char find_buf[30];
    int i;
    int j;

    load_background_pic = LBX_Reload("LOAD.LBX", 11, p_scratch);   /* LOAD.LBX: 11 the settings background, 4 Ok,
                                                                       13-15 the checks on, 16-18 off, 19-20 spare */
    load_ok_pic = LBX_Reload_Next("LOAD.LBX", 4, p_scratch);
    for (i = 0; i < 3; i++)
        settings_check_on_pics[i] = LBX_Reload_Next("LOAD.LBX", i + 13, p_scratch);
    for (i = 0; i < 3; i++)
        settings_check_off_pics[i] = LBX_Reload_Next("LOAD.LBX", i + 16, p_scratch);
    for (i = 0; i < 2; i++)
        settings_unused_pics[i] = LBX_Reload_Next("LOAD.LBX", i + 19, p_scratch);
    if (DOS_Find_File("MAGIC.SET", find_buf) == 0)
        Settings_Defaults();
    else {
        fp = fopen("MAGIC.SET", "rb");
        fread(&game_config, sizeof(game_config), 1, fp);
        fclose(fp);
    }
    p_settings_words = (int *)&game_config;    /* toggle i = GameConfig field i: 0 sound_effects .. 17 expanding_help */
    Mouse_Set_Cursor_Regions(1, &mouse_region_menu);
    UI_Clear_Controls();
    esc_control = UI_Add_Hotkey("\x1b");
    settings_ok_control = UI_Add_Region(265, 175, 306, 189, "O", -1);
    /* the left column: toggles 0 .. 9 (sound effects .. end-of-turn wait) on rows 0 .. 6, 8, 11, 12; the right
       column: toggles 10 .. 17 (strategic combat .. expanding help) on rows 0, 1, 3 .. 6, 8, 9 - the Movement
       Animations control (y 74) takes the right column's row 2 */
    for (i = 0; i < 8; i++) {
        j = i;
        if (j == 7)
            j = 8;
        toggles[i] = UI_Add_Region(31, row_y[j], 175, row_y[j] + 9, "", -1);
    }
    toggles[8] = UI_Add_Region(31, row_y[11], 175, row_y[11] + 9, "", -1);
    toggles[9] = UI_Add_Region(31, row_y[12], 175, row_y[12] + 9, "", -1);
    for (i = 0; i < 8; i++) {
        j = i;
        if (i > 5)
            j++;
        if (i > 1)
            j++;
        toggles[i + 10] = UI_Add_Region(192, row_y[j], 310, row_y[j] + 9, "", -1);
    }
    anim_control = UI_Add_Region(192, 74, 310, 83, "", -1);
    done = 0;
    drawn = 0;
    game_menu_first_frame = 0;
    UI_Set_Redraw_Fn(Settings_Redraw, 1);
    Help_Load_Settings();
    while (done == 0) {
        input = UI_Get_Input();
        Timer_Mark();
        if (input == esc_control)
            done = 1;
        if (input == settings_ok_control) {
            fp = fopen("MAGIC.SET", "wb");
            fwrite(&game_config, sizeof(game_config), 1, fp);
            fclose(fp);
            done = 1;
        }
        for (i = 0; i < 18; i++) {
            if (toggles[i] == input) {
                if (p_settings_words[i] == 0)
                    p_settings_words[i] = 1;
                else
                    p_settings_words[i] = 0;
            }
            if (input == toggles[SETTING_BOOK_ORDERING]) {
                combat_spellbook_page = 0;
                overland_spellbook_page = 0;
                research_first_page = 0;
            }
        }
        if (input == anim_control)
            game_config.movement_animations = 1 - game_config.movement_animations;
        if (done == 0) {
            Settings_Redraw();
            UI_Present_Frame();
            if (drawn == 0) {
                Screen_Copy_Front();
                drawn = 1;
            }
            Wait_Ticks(1);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    if (menu_return_mode == MENU_RETURN_FROM_MENU)
        current_screen = SCREEN_GAME_MENU;
    menu_return_mode = MENU_RETURN_NO_FADE;
}

/* () redraw callback: a check box per option (the 3 box pictures rotate down the columns), the labels and the group
   titles */
void Settings_Redraw(void)
{
    int pressed;
    char colors[4] = { 0x62, 0x62, 0x62, 0x62 };
    register int i;
    register int k;

    pressed = UI_Get_Pressed_Control();
    Draw_Pic_Frame(0, 0, load_background_pic);
    Fill_Rect(265, 175, 306, 189, 0);
    Font_Select_Shadow(2, 2, 45);
    k = 0;
    for (i = 0; i < 3; i++) {
        if (p_settings_words[i] == 0)
            Draw_Pic_Frame(31, i * 11 + 52, settings_check_off_pics[k % 3]);
        else
            Draw_Pic_Frame(31, i * 11 + 52, settings_check_on_pics[k % 3]);
        k++;
    }
    for (i = 0; i < 5; i++) {
        if (p_settings_words[i + 3] == 0)
            Draw_Pic_Frame(31, i * 11 + 101, settings_check_off_pics[k % 3]);
        else
            Draw_Pic_Frame(31, i * 11 + 101, settings_check_on_pics[k % 3]);
        k++;
    }
    if (p_settings_words[8] == 0)
        Draw_Pic_Frame(31, 163, settings_check_off_pics[k % 3]);
    else
        Draw_Pic_Frame(31, 163, settings_check_on_pics[k % 3]);
    k++;
    if (p_settings_words[9] == 0)
        Draw_Pic_Frame(31, 174, settings_check_off_pics[k % 3]);
    else
        Draw_Pic_Frame(31, 174, settings_check_on_pics[k % 3]);
    k++;
    for (i = 0; i < 2; i++) {
        if (p_settings_words[i + 10] == 0)
            Draw_Pic_Frame(192, i * 11 + 52, settings_check_off_pics[k % 3]);
        else
            Draw_Pic_Frame(192, i * 11 + 52, settings_check_on_pics[k % 3]);
        k++;
    }
    for (i = 0; i < 6; i++) {
        if (p_settings_words[i + 12] == 0)
            Draw_Pic_Frame(192, i * 11 + 101, settings_check_off_pics[k % 3]);
        else
            Draw_Pic_Frame(192, i * 11 + 101, settings_check_on_pics[k % 3]);
        k++;
    }
    if (game_config.movement_animations == 0)
        Draw_Pic_Frame(192, 74, settings_check_off_pics[0]);
    else
        Draw_Pic_Frame(192, 74, settings_check_on_pics[0]);
    if (pressed == settings_ok_control)
        Draw_Pic_Frame(267, 177, load_ok_pic);
    else
        Draw_Pic_Frame(266, 176, load_ok_pic);
    Print_Text(47, 54, "Sound Effects");
    Print_Text(47, 65, "Background Music");
    Print_Text(47, 76, "Event Music");
    Print_Text(47, 103, "City Spell Events");
    Print_Text(47, 114, "Overland Spell Events");
    Print_Text(47, 125, "Summoning Events");
    Print_Text(47, 136, "End of Turn Summary");
    Print_Text(47, 147, "Raze City");
    Print_Text(47, 165, "Random Events");
    Print_Text(47, 176, "End of Turn Wait");
    Print_Text(208, 54, "Strategic Combat Only");
    Print_Text(208, 65, "Auto Unit Information");
    Print_Text(208, 76, "Movement Animations");
    Print_Text(208, 103, "Enemy Moves");
    Print_Text(208, 114, "Enemy Spells");
    Print_Text(208, 125, "Spell Book Ordering");
    Print_Text(208, 136, "Spell Animations");
    Print_Text(208, 147, "Show Node Owners");
    Print_Text(208, 158, "Expanding Help");
    Font_Select_Palette(3, colors);
    Font_Select_Shadow(3, 15, 45);
    Print_Text(31, 42, "Sound and Music");
    Print_Text(31, 92, "Messages");
    Print_Text(192, 42, "Combat");
    Print_Text(192, 92, "Display");
}

/* () at start-up: MAGIC.SET is read if it exists with exactly 0x1d2 bytes, else defaults are written; Movement
   Animations and Raze City outside 0 / 1 become 1; an empty Hall of Fame (first score 0) is cleared and saved */
void Settings_Load_Or_Create(void)
{
    char find_buf[30];
    FILE *fp;
    int i;

    if (DOS_Find_File("MAGIC.SET", find_buf) == 0 || DOS_File_Size("MAGIC.SET") != 0x1d2) {
        Settings_Defaults();
        fp = fopen("MAGIC.SET", "wb");
        fwrite(&game_config, sizeof(game_config), 1, fp);
    } else {
        fp = fopen("MAGIC.SET", "rb");
        fread(&game_config, sizeof(game_config), 1, fp);
    }
    fclose(fp);
    if (game_config.movement_animations > 1)
        game_config.movement_animations = 1;
    if (game_config.movement_animations < 0)
        game_config.movement_animations = 1;
    if (game_config.raze_city > 1)
        game_config.raze_city = 1;
    if (game_config.raze_city < 0)
        game_config.raze_city = 1;
    if (game_config.hall_of_fame_scores[0] == 0) {
        for (i = 0; i < 10; i++) {
            game_config.hall_of_fame_scores[i] = 0;
            game_config.hall_of_fame_race[i] = RACE_BARBARIAN;
            strcpy(game_config.hall_of_fame_names[i], "");
        }
        fp = fopen("MAGIC.SET", "wb");
        fwrite(&game_config, sizeof(game_config), 1, fp);
        fclose(fp);
    }
}

/* () every option on except Strategic Combat Only; new game: Intro, 4 opponents, normal land and magic; input type 1,
   2 sound channels; no saves; Hall of Fame empty */
void Settings_Defaults(void)
{
    int i;

    game_config.sound_effects = 1;
    game_config.music = 1;
    game_config.event_music = 1;
    game_config.enemy_moves = 1;
    game_config.enemy_spells = 1;
    game_config.random_events = 1;
    game_config.end_turn_wait = 1;
    game_config.end_turn_summary = 1;
    game_config.raze_city = 1;
    game_config.book_ordering = 1;
    game_config.strategic_combat = 0;
    game_config.spell_animations = 1;
    game_config.city_spell_events = 1;
    game_config.overland_spell_events = 1;
    game_config.summoning_events = 1;
    game_config.auto_unit_info = 1;
    game_config.show_node_owners = 1;
    game_config.expanding_help = 1;
    game_config.movement_animations = 1;
    game_config.newgame_difficulty = DIFFICULTY_INTRO;
    game_config.newgame_opponents = 4;
    game_config.newgame_magic_power = 1;
    game_config.newgame_land_size = 1;
    game_config.input_type = 1;
    game_config.sound_channels = 2;
    for (i = 0; i < 8; i++)
        game_config.save_slot_used[i] = 0;
    for (i = 0; i < 10; i++) {
        game_config.hall_of_fame_scores[i] = 0;
        game_config.hall_of_fame_race[i] = RACE_BARBARIAN;
        strcpy(game_config.hall_of_fame_names[i], "");
    }
}

/* () HLPENTRY.LBX entry 34: the 21 help areas of the settings screen. A slip in the data: record 18, the OK button (265, 175 - 306, 189), shows HELP.LBX 633 "LOAD BUTTON"; records 7 and
   20 are the same (806, Raze City); clarified using ReMoM repo */
void Help_Load_Settings(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 34, help_entries, 0, 21, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 21);
}
