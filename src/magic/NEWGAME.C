/* NEWGAME.C - MAGIC.EXE code segment 3da3 (MAGIC only): the load-game screen and the new-game screens - options,
   wizard / portrait / name / banner / race, custom picks, spell picks, the presets - and the hand-off to WIZARDS.EXE.
   Its data block is DS:2abc .. 3333 (after SETTINGS.C's, before WorldGen's).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 3da3 src/magic/NEWGAME.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     Font_Select_Shadow, Font_Select_Shadow_UL (2; TEXT.C takes 4);
     Palette_Load (2; PALETTE.C takes 3). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
void Text_List_Separator(int *count, int total, char *text);
void LoadGame_Redraw(void);
void NewGameOptions_Redraw(void);
void SelectWizard_Redraw(void);
void WizardName_Redraw(void);
void CustomPicks_Redraw(void);
void SelectRace_Redraw(void);
void SelectBanner_Redraw(void);
void SelectSpells_Redraw(void);
int NewGame_Run(void);
int Screen_NewGame_Options(void);
int Screen_Select_Wizard(void);
int Screen_Select_Portrait(void);
int Screen_Wizard_Name(void);
int Screen_Custom_Picks(void);
int Screen_Select_Spells(void);
int Screen_Select_Race(void);
int Screen_Select_Banner(void);
void Launch_Wizards(int slot);
void Palette_Fade_To_Black(void);
void Screen_Load_Game(void);
void Help_Load_LoadGame(void);
void Palette_Fade_From_Black(void);
void Help_Load_NewGame_Options(void);
void NewGame_Roll_Book_Looks(void);
void Help_Load_Select_Wizard(int custom);
void Wizard_Apply_Preset(int preset);
void NewGame_Draw_Spellbooks(int preset);
void Help_Load_Select_Portrait(void);
void NewGame_Draw_Retort_Text(void);
void Help_Load_Select_Banner(void);
void Help_Load_Select_Race(void);
void Help_Load_Custom_Picks(void);
void SelectSpells_Add_Controls(void);
void SelectSpells_Draw_List(void);
void Window_Zoom_Start_Nop(void);
void Window_Zoom_Skipped_Nop(void);
void Screen_Present(void);
#pragma warn -pro

extern struct RaceInfo race_table[14];           /* DS:1df6 */
extern char *retort_names[18];                  /* DS:1f96 */
extern int menu_return_mode;                    /* DS:290a */
extern int ctl_load;                            /* DS:52b6 */
extern unsigned screen_background_pic;          /* DS:52b8 */
extern int ctl_cancel;                          /* DS:52c0 */
extern int menu_selection;                      /* DS:52c4 */
extern unsigned portrait_pics[14];              /* DS:52c6 */
extern unsigned load_btn_load_off_pic;          /* DS:52f8 */
extern int menu_forced_choice;                  /* DS:52fc */
extern int save_list_count;                     /* DS:52fe */
extern unsigned ng_ok_pic;                      /* DS:5cb0 */
extern unsigned p_scratch;                      /* DS:61ae */
extern int screen_fade_in;                      /* DS:64ae */
extern struct Spell far *p_spell_data;          /* DS:662c */
extern struct Wizard wizards[6];                /* DS:6900 */
extern struct HelpEntry help_entries[];         /* DS:85b0 */
extern int difficulty;                          /* DS:87cc */
extern int magic_setting;                       /* DS:87ce */
extern int land_size;                           /* DS:87d0 */
extern int num_players;                         /* DS:87d2 */
extern struct GameConfig game_config;           /* DS:87d4 */
extern int save_slot_list[8];                   /* DS:8a12  the load list's slots (SETTINGS.C); MAGIC.C's menu_save_slots is the main menu's own list */
extern unsigned msgbox_bottom_pic;              /* DS:90c6 */
extern unsigned msgbox_top_pic;                 /* DS:90c8 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
unsigned load_btn_save_off_pic;                 /* DS:8eae */
unsigned ok_button_pic;                         /* DS:8eac */
int ng_picks_done;                              /* DS:8eaa */
int ng_chosen_race_spare[2];                    /* DS:8ea6  no code in either program reads it */
int ng_chosen_race;                             /* DS:8ea4 */
unsigned load_slot_bar_pic;                     /* DS:8ea2 */
unsigned load_btn_settings_pic;                 /* DS:8ea0 */
int ctl_settings;                               /* DS:8e9e */
int select_spells_realm;                        /* DS:8e9c */
int ng_selected_spare;                          /* DS:8e9a  no code in either program reads it */
int ng_selected;                                /* DS:8e98  the load screen: the chosen save slot */
/* ng_last_pressed: on the custom-picks screen ng_selected holds the control pressed last frame (no repeated
   "no picks left" message) */
#define ng_last_pressed ng_selected
unsigned select_mark_pic;                       /* DS:8e96 */
int ng_picks_left;                          /* DS:8e94  the picks still to make; -1 only inside one CustomPicks_Redraw
                                               (a book row re-set to its own value with none left: no message), then 0 */
int ctl_ok;                                     /* DS:8e92 */
unsigned load_btn_load_pic;                     /* DS:8e90 */
unsigned ng_cancel_pic;                         /* DS:8e8e */
int ctl_retorts[18];                            /* DS:8e6a */
unsigned ng_panel_pic;                          /* DS:8e68 */
unsigned ng_button_pics[15];                    /* DS:8e4a */
int ng_shown_preset;                            /* DS:8e48 */
int ng_controls[30];                            /* DS:8e0c */
int ctl_difficulty;                             /* DS:8e0a */
int ctl_opponents;                              /* DS:8e08 */
int ctl_magic_power;                            /* DS:8e06 */
int ctl_land_size;                              /* DS:8e04 */
unsigned ng_book_pics[15];                      /* DS:8de6 */
int ng_right_rows_spare;                        /* DS:8de4  no code in either program reads it */
int ng_right_rows;                              /* DS:8de2  the right-hand list's rows: 8 Select Wizard, 7 Select
                                                   Picture - the shared redraw also tells the two screens apart by it */
#define NG_LIST_WIZARDS 8     /* ng_right_rows: the Select Wizard list (its 8 rows) */
#define NG_LIST_PORTRAITS 7   /* the Select Picture list */
int ng_life_slider;                             /* DS:8de0 */
int ng_death_slider;                            /* DS:8dde */
int ng_chaos_slider;                            /* DS:8ddc */
int ng_nature_slider;                           /* DS:8dda */
int ng_sorcery_slider;                          /* DS:8dd8 */
unsigned worldgen_box_pic;                      /* DS:8dd6 */
int ng_book_looks[6][11];                       /* DS:8d52  each book's look 0..2 (Random(3) - 1); rows: 0 the
                                                   summary strip (every realm), 1 Death, 2 Life, 3 Nature,
                                                   4 Sorcery, 5 Chaos (the custom-picks columns) */
#define NG_BOOK_LOOKS 3            /* ng_book_pics: 3 looks per realm, in groups Life +0, Sorcery +3, ... */
#define NG_BOOK_PICS_SORCERY 3
#define NG_BOOK_PICS_NATURE 6
#define NG_BOOK_PICS_DEATH 9
#define NG_BOOK_PICS_CHAOS 12
#define NG_BOOK_STRIP_X 36         /* the picked books' strip under the portrait: from (36, 135), 8 pixels a book */
#define NG_BOOK_STRIP_Y 135
#define NG_BOOK_STEP 8
int ng_message;                                 /* DS:8d50  the message box to show next (0 none): */
#define NG_MSG_PICKS_LEFT 1        /* both screens: "You need to make all your picks before you can continue" */
#define NG_MSG_MAX_RETORTS 2       /* custom picks: "You may not select more than 6 special abilities" */
#define NG_MSG_RETORT_NEEDS 3      /* "To select <ng_message_retort> you need: ..." */
#define NG_MSG_LIFE_AND_DEATH 4    /* "You can not select both Life and Death magic" */
#define NG_MSG_NO_PICKS_LEFT 5     /* "You have already made all your picks" */
#define NG_MSG_TWO_PICKS 6         /* "... not enough picks left ... You need 2 picks" */
#define NG_MSG_REALM_FULL 2        /* select spells: "You have no picks left in this area, ..." */
int ng_message_retort;                          /* DS:8d4e */
/* the HLPENTRY.LBX entry counts of the screens (Help_Load_* and the re-sets after a message box) */
#define NG_HELP_LOADGAME 4
#define NG_HELP_OPTIONS 6
#define NG_HELP_BANNER 1
#define NG_HELP_PORTRAIT 1
#define NG_HELP_WIZARD 15
#define NG_HELP_CUSTOM_PICKS 25
#define NG_HELP_RACE 14
#define NG_HELP_SPELLS 30
int start_spells[6][5][START_SPELL_PICKS];                     /* DS:8a42 */

void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
long DOS_File_Size(char *name);

/* the new-game screens': the same full-screen arrow as MAGIC.C's mouse_region_arrow */
struct MouseRegion mouse_region_new_game = { 1, 0, 0, 0, 319, 199 };    /* DS:2abc */
char name_box_colors[8] = { 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18 };    /* DS:2ac8  the name edit box colors (ReMoM wziards_name_colors; clarified using ReMoM repo) */
struct WizardPreset wizard_presets[15] = {    /* DS:2ad0  name, books Life / Sorcery / Nature / Death / Chaos, retort */
    { "Merlin", 5, 0, 5, 0, 0, RETORT_SAGE_MASTER },
    { "Raven", 0, 6, 5, 0, 0, RETORT_NONE },
    { "Sharee", 0, 0, 0, 5, 5, RETORT_CONJURER },
    { "Lo Pan", 0, 5, 0, 0, 5, RETORT_CHANNELER },
    { "Jafar", 0, 10, 0, 0, 0, RETORT_ALCHEMY },
    { "Oberic", 0, 0, 5, 0, 5, RETORT_MANA_FOCUSING },
    { "Rjak", 0, 0, 0, 9, 0, RETORT_INFERNAL_POWER },
    { "Sss'ra", 4, 0, 0, 0, 4, RETORT_MYRRAN },
    { "Tauron", 0, 0, 0, 0, 10, RETORT_CHAOS_MASTERY },
    { "Freya", 0, 0, 10, 0, 0, RETORT_NATURE_MASTERY },
    { "Horus", 5, 5, 0, 0, 0, RETORT_ARCHMAGE },
    { "Ariel", 10, 0, 0, 0, 0, RETORT_CHARISMATIC },
    { "Tlaloc", 0, 0, 4, 5, 0, RETORT_WARLORD },
    { "Kali", 0, 5, 0, 5, 0, RETORT_ARTIFICER },
    { "Custom", 0, 0, 0, 0, 0, RETORT_NONE }
};
int newgame_unused_2c1a[10] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };    /* DS:2c1a  2 .. 11 beside the start_spells_* tables (by books); no reader found */
int start_spells_common[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };    /* DS:2c2e  commons to pick: [books - 2] on the spell screen, [books - 1] in Wizard_Apply_Preset (BUG #354) */
int start_spells_uncommon[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 };    /* DS:2c42 */
int start_spells_rare[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };    /* DS:2c56 */
int default_spells[5][START_SPELL_PICKS] = {    /* DS:2c6a  the default picks per realm (Nature, Sorcery, Chaos, Life, Death) */
    { 6, 7, 9, 8, 4, 5, 1, 3, 2, 10, 17, 15, 27 },
    { 49, 50, 45, 43, 46, 48, 47, 42, 41, 44, 56, 60, 65 },
    { 83, 90, 86, 84, 85, 89, 88, 87, 82, 81, 91, 97, 106 },
    { 130, 129, 126, 127, 125, 124, 122, 123, 128, 121, 133, 136, 147 },
    { 167, 166, 162, 163, 165, 169, 168, 161, 170, 164, 174, 175, 184 }
};
int retort_requirements[18][7] = {    /* DS:2cec  per retort - picks in any realm, how many realms, picks in Life / Death / Chaos / Nature / Sorcery; ReMoM's special-ability requirements, clarified using ReMoM repo */
    { 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 0, 4, 0, 0 },
    { 0, 1, 0, 0, 0, 4, 0 },
    { 0, 1, 0, 0, 0, 0, 4 },
    { 0, 1, 0, 4, 0, 0, 0 },
    { 0, 1, 4, 0, 0, 0, 0 },
    { 1, 2, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { 4, 1, 0, 0, 0, 0, 0 },
    { 4, 1, 0, 0, 0, 0, 0 },
    { 0, 3, 0, 0, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { 2, 3, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
void Font_Select_Shadow();                     /* no prototype here: some calls pass 2 or 3 of the 4 words (QUIRK #371) */
void Font_Select_Shadow_UL();                  /* no prototype here: some calls pass 2 or 3 of the 4 words (QUIRK #371) */
int custom_retort_slots[18] = { 0, 1, 8, 10, 17, 15, 7, 9, 6, 13, 14, 16, 2, 3, 4, 5, 11, 12 };    /* DS:2de8  the retort of each Custom Picks grid slot */

/* () the new-game screens as a state machine: 0 options, 1 wizard (Custom -> the custom path), 2 portrait, 3 name,
   4 custom picks, 5 spells, 6 race, 7 banner, 99 done, -1 back to the main menu (returns 0). Done: the world is
   built, events and the game start initialised, saved as SAVE9.GAM, and WIZARDS.EXE started */
int NewGame_Run(void)
{
    int done;
    int state;
    int custom;

    land_size = 1;
    magic_setting = 1;
    num_players = 1;
    difficulty = 1;
    state = NG_STATE_OPTIONS;
    done = 0;
    while (done == 0) {
        UI_Clear_Help();
        switch (state) {
        case NG_STATE_BACK:
            return 0;
        case NG_STATE_OPTIONS:
            state = Screen_NewGame_Options();
            break;
        case NG_STATE_WIZARD:
            state = Screen_Select_Wizard();
            if (state == NG_STATE_PORTRAIT)
                custom = 1;
            else
                custom = 0;
            break;
        case NG_STATE_PORTRAIT:
            state = Screen_Select_Portrait();
            break;
        case NG_STATE_NAME:
            state = Screen_Wizard_Name();
            if (custom != 0) {
                if (state == NG_STATE_BACK)
                    state = NG_STATE_PORTRAIT;
                else
                    state = NG_STATE_CUSTOM_PICKS;
            } else {
                if (state == NG_STATE_BACK)
                    state = NG_STATE_WIZARD;
                else
                    state = NG_STATE_RACE;
            }
            break;
        case NG_STATE_CUSTOM_PICKS:
            state = Screen_Custom_Picks();
            break;
        case NG_STATE_SPELLS:
            state = Screen_Select_Spells();
            break;
        case NG_STATE_RACE:
            state = Screen_Select_Race();
            if (state == NG_STATE_BACK) {
                if (custom != 0)
                    state = NG_STATE_SPELLS;
                else
                    state = NG_STATE_NAME;
            }
            break;
        case NG_STATE_BANNER:
            state = Screen_Select_Banner();
            break;
        case NG_STATE_DONE:
            done = -1;
            break;
        }
    }
    World_Build();
    Events_Reset();
    NewGame_Start_Init();
    Save_Game(SAVE_SLOT_CONTINUE);
    Launch_Wizards(SAVE_SLOT_CONTINUE);
}

/* (slot) fade out, stop the music and sound; a loaded slot (not SAVE_SLOT_CONTINUE, SAVE9.GAM itself) is copied
   through SAVE9.GAM too; then WIZARDS.EXE */
void Launch_Wizards(int slot)
{
    Palette_Fade_To_Black();
    Music_Release_Sequence();
    Sound_Shutdown();
    if (slot != SAVE_SLOT_CONTINUE) {
        Load_Game(slot);
        Save_Game(SAVE_SLOT_CONTINUE);
    }
    Exec_Program("WIZARDS.EXE", "JENNY", "JENNY", "");
}

/* () the load-game screen: the saves SAVE1 .. SAVE8.GAM that exist, Load starts WIZARDS.EXE with the selected one,
   Settings opens the settings screen (then the main menu reopens this screen:
   menu_forced_choice = MAIN_MENU_LOAD), Cancel / Esc back to the main menu */
void Screen_Load_Game(void)
{
    int k;
    int x1;
    int y1;
    int hk_esc;
    int input;
    int faded;
    int slot_controls[8];
    char pattern[16];
    char slot_text[16];
    char find_buf[20];
    register int i;
    register int done;

    screen_background_pic = LBX_Reload("LOAD.LBX", 0, p_scratch);
    ng_cancel_pic = LBX_Reload_Next("LOAD.LBX", 2, p_scratch);
    load_btn_load_pic = LBX_Reload_Next("LOAD.LBX", 1, p_scratch);
    load_btn_load_off_pic = LBX_Reload_Next("LOAD.LBX", 5, p_scratch);
    load_btn_save_off_pic = LBX_Reload_Next("LOAD.LBX", 7, p_scratch);
    ok_button_pic = LBX_Reload_Next("LOAD.LBX", 8, p_scratch);
    load_slot_bar_pic = LBX_Reload_Next("LOAD.LBX", 9, p_scratch);
    load_btn_settings_pic = LBX_Reload_Next("LOAD.LBX", 12, p_scratch);
    select_mark_pic = LBX_Reload_Next("LOAD.LBX", 10, p_scratch);
    screen_fade_in = 0;
    done = 0;
    faded = 0;
    if (menu_return_mode != MENU_RETURN_NO_FADE) {
        screen_fade_in = 1;
        Palette_Fade_To_Black();
        Fill_Rect(0, 0, 319, 199, 0);
        Set_Draw_Page_Front();
        Fill_Rect(0, 0, 319, 199, 0);
        Set_Draw_Page();
        Palette_Load(1, -1);
    }
    x1 = 43;
    y1 = 171;
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    hk_esc = UI_Add_Hotkey("\x1b");
    ctl_cancel = UI_Add_Region(x1, y1, x1 + 39, y1 + 13, "\x1b", -1);
    ctl_load = UI_Add_Region(x1 + 40, y1, x1 + 78, y1 + 13, "L", -1);
    ctl_ok = UI_NO_CONTROL;
    save_list_count = 0;
    for (i = 1; i < 9; i++) {
        strcpy(pattern, "SAVE");
        itoa(i, slot_text, 10);
        strcat(pattern, slot_text);
        strcat(pattern, ".GAM");
        if (DOS_Find_File(pattern, find_buf) != 0) {
            save_slot_list[save_list_count] = i - 1;
            save_list_count++;
        } else
            save_slot_list[save_list_count] = -1;
    }
    for (i = 0; i < 8; i++)
        slot_controls[i] = UI_NO_CONTROL;
    for (i = 0; i < save_list_count; i++)
        slot_controls[save_slot_list[i]] = UI_Add_Region(x1, save_slot_list[i] * 15 + 47, x1 + 220,
                                                           save_slot_list[i] * 15 + 56, "", -1);
    ctl_settings = UI_Add_Region(172, y1, 229, y1 + 13, "", -1);
    ng_selected = -1;
    done = 0;
    UI_Set_Redraw_Fn(LoadGame_Redraw, 1);
    Help_Load_LoadGame();
    while (!done) {
        input = UI_Get_Input();
        Timer_Mark();
        for (i = 0; i < 8; i++) {
            if (slot_controls[i] == input) {
                for (k = 0; k < save_list_count; k++) {
                    if (save_slot_list[k] == i)
                        ng_selected = i;
                }
            }
        }
        if (input == ctl_cancel || input == hk_esc) {
            done = 1;
            Play_Message_Sound();
        }
        if (input == ctl_load && ng_selected >= 0) {
            UI_Clear_Help();
            Launch_Wizards(ng_selected);
            done = 1;
        }
        if (input == ctl_settings) {
            UI_Clear_Help();
            Screen_Settings();
            menu_forced_choice = MAIN_MENU_LOAD;
            done = 1;
        }
        if (!done) {
            LoadGame_Redraw();
            UI_Present_Frame();
            if (screen_fade_in != 0 && faded == 0) {
                Screen_Copy_Front();
                Palette_Fade_From_Black();
                faded = 1;
            }
            Wait_Ticks(2);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
}

/* () redraw of the load-game screen: the save slots (the selected one dark blue with the marker), the buttons
   (pressed: one pixel right / down; Load greyed until a save is selected) and the copyright line */
void LoadGame_Redraw(void)
{
    char selected_colors[4] = { 0x66, 0x66, 0x66, 0x66 };
    char slot_colors[4] = { 0x60, 0x61, 0x60, 0x61 };
    int y1;
    char copyright_colors[2];
    int i;
    int x1;

    copyright_colors[0] = 0x22;
    copyright_colors[1] = 0x2b;
    menu_selection = UI_Get_Pressed_Control();   /* reused here: the pressed control */
    x1 = 43;
    y1 = 171;
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    Draw_Pic_Frame(0, 0, screen_background_pic);
    Fill_Rect(42, 170, 123, 184, 0);
    Fill_Rect(171, 170, 271, 184, 0);
    for (i = 0; i < save_list_count; i++) {
        Draw_Pic_Frame(x1, save_slot_list[i] * 15 + 47, load_slot_bar_pic);
        if (save_slot_list[i] == ng_selected) {
            Font_Select_Palette(3, selected_colors);
            Font_Select_Shadow_UL(3, 15);
            Draw_Pic_Frame(x1, save_slot_list[i] * 15 + 46, select_mark_pic);
        } else {
            Font_Select_Palette(3, slot_colors);
            Font_Select_Shadow_UL(3, 15);
        }
        Print_Text(x1 + 10, save_slot_list[i] * 15 + 47, game_config.save_names[save_slot_list[i]]);
    }
    if (menu_selection == ctl_cancel)
        Draw_Pic_Frame(44, 172, ng_cancel_pic);
    else
        Draw_Pic_Frame(43, 171, ng_cancel_pic);
    if (menu_selection == ctl_settings)
        Draw_Pic_Frame(173, 172, load_btn_settings_pic);
    else
        Draw_Pic_Frame(172, 171, load_btn_settings_pic);
    if (ng_selected >= 0) {
        if (menu_selection == ctl_load)
            Draw_Pic_Frame(84, 172, load_btn_load_pic);
        else
            Draw_Pic_Frame(83, 171, load_btn_load_pic);
    } else
        Draw_Pic_Frame(83, 171, load_btn_load_off_pic);
    Draw_Pic_Frame(231, 171, ok_button_pic);
    Draw_Pic_Frame(122, 171, load_btn_save_off_pic);
    Font_Select_Palette(0, copyright_colors);
    Print_Text_Centered(160, 193, "Copyright  Simtex Software, 1995   V1.31");
}

/* () the game options screen (difficulty, opponents, land size, magic), started from MAGIC.SET's last choices (or
   the defaults when it is missing or not 0x1d2 bytes; out-of-range values reset); clicking an option cycles it. OK
   writes MAGIC.SET and sets the game options -> 1; Cancel -> -1 */
int Screen_NewGame_Options(void)
{
    int faded;
    int done;
    char find_buf[30];
    int input;
    FILE *fp;

    screen_fade_in = 1;
    if (screen_fade_in != 0)                    /* always true: as compiled */
        Palette_Fade_To_Black();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page_Front();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page();
    Palette_Load(0, -1);
    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 1, p_scratch);
    ng_ok_pic = LBX_Reload_Next("NEWGAME.LBX", 2, p_scratch);
    ng_cancel_pic = LBX_Reload_Next("NEWGAME.LBX", 3, p_scratch);
    ng_button_pics[0] = LBX_Reload_Next("NEWGAME.LBX", 4, p_scratch);
    ng_button_pics[1] = LBX_Reload_Next("NEWGAME.LBX", 5, p_scratch);
    ng_button_pics[2] = LBX_Reload_Next("NEWGAME.LBX", 6, p_scratch);
    ng_button_pics[3] = LBX_Reload_Next("NEWGAME.LBX", 7, p_scratch);
    if (DOS_Find_File("MAGIC.SET", find_buf) == 0 || DOS_File_Size("MAGIC.SET") != sizeof(struct GameConfig))
        Settings_Defaults();
    else {
        fp = fopen("MAGIC.SET", "rb");
        fread(&game_config, sizeof(struct GameConfig), 1, fp);
        fclose(fp);
    }
    if (game_config.newgame_difficulty < 0 || game_config.newgame_difficulty > 4)
        game_config.newgame_difficulty = 0;
    if (game_config.newgame_opponents < 1 || game_config.newgame_opponents > 4)
        game_config.newgame_opponents = 1;
    if (game_config.newgame_land_size < 0 || game_config.newgame_land_size > 2)
        game_config.newgame_land_size = 0;
    if (game_config.newgame_magic_power < 0 || game_config.newgame_magic_power > 2)
        game_config.newgame_magic_power = 0;
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    Set_Draw_Page_Front();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    ctl_ok = UI_Add_Button(252, 179, "", ng_ok_pic, "", -1);
    ctl_cancel = UI_Add_Button(171, 179, "", ng_cancel_pic, "\x1b", -1);
    ctl_difficulty = UI_Add_Region(250, 38, 315, 54, "", -1);
    ctl_opponents = UI_Add_Region(250, 65, 315, 81, "", -1);
    ctl_land_size = UI_Add_Region(250, 92, 315, 108, "", -1);
    ctl_magic_power = UI_Add_Region(250, 119, 315, 135, "", -1);
    done = 0;
    faded = 0;
    UI_Set_Redraw_Fn(NewGameOptions_Redraw, 1);
    Help_Load_NewGame_Options();
    while (done == 0) {
        input = UI_Get_Input();
        Timer_Mark();
        if (input == ctl_cancel) {
            UI_Suspend_Redraw_Fn();
            UI_Clear_Help();
            return NG_STATE_BACK;
        }
        if (input == ctl_ok) {
            fp = fopen("MAGIC.SET", "wb");
            fwrite(&game_config, sizeof(struct GameConfig), 1, fp);
            fclose(fp);
            UI_Suspend_Redraw_Fn();
            UI_Clear_Help();
            NewGame_Roll_Book_Looks();
            land_size = game_config.newgame_land_size;
            magic_setting = game_config.newgame_magic_power;
            num_players = game_config.newgame_opponents + 1;
            difficulty = game_config.newgame_difficulty;
            return NG_STATE_WIZARD;
        }
        if (input == ctl_difficulty) {
            game_config.newgame_difficulty++;
            if (game_config.newgame_difficulty > 4)
                game_config.newgame_difficulty = 0;
        }
        if (input == ctl_opponents) {
            game_config.newgame_opponents++;
            if (game_config.newgame_opponents > 4)
                game_config.newgame_opponents = 1;
        }
        if (input == ctl_land_size) {
            game_config.newgame_land_size++;
            if (game_config.newgame_land_size > 2)
                game_config.newgame_land_size = 0;
        }
        if (input == ctl_magic_power) {
            game_config.newgame_magic_power++;
            if (game_config.newgame_magic_power > 2)
                game_config.newgame_magic_power = 0;
        }
        if (done == 0) {
            NewGameOptions_Redraw();
            UI_Present_Frame();
            if (screen_fade_in != 0 && faded == 0) {
                Screen_Copy_Front();
                Palette_Fade_From_Black();
                faded = 1;
            }
            Wait_Ticks(2);
        }
    }
}

/* () redraw of the options screen: each option's button (pressed: one pixel right / down) with its current value
   in dark brown over a light tan copy one pixel right / down (a light shadow) */
void NewGameOptions_Redraw(void)
{
    char light[4] = { 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[4] = { 0xbb, 0xbb, 0xbb, 0xbb };
    char difficulty_names[5][11] = { "Intro", "Easy", "Normal", "Hard", "Impossible" };
    char land_size_names[3][7] = { "Small", "Medium", "Large" };
    char magic_power_names[3][9] = { "Weak", "Normal", "Powerful" };
    char opponent_names[4][8] = { "One", "Two", "Three", "Four" };

    Draw_Pic_Frame(0, 0, screen_background_pic);
    Fill_Rect(250, 38, 315, 54, 0);
    Fill_Rect(250, 65, 315, 82, 0);
    Fill_Rect(250, 92, 315, 107, 0);
    Fill_Rect(250, 119, 315, 134, 0);
    Fill_Rect(251, 178, 315, 193, 0);
    Fill_Rect(170, 178, 234, 193, 0);
    Draw_Pic_Frame(165, 0, ng_panel_pic);
    menu_selection = UI_Get_Pressed_Control();   /* reused here: the pressed control */
    Font_Select(3, 15);
    if (menu_selection != ctl_difficulty) {
        Draw_Pic_Frame(251, 39, ng_button_pics[0]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(283, 43, difficulty_names[game_config.newgame_difficulty]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(282, 42, difficulty_names[game_config.newgame_difficulty]);
    } else {
        Draw_Pic_Frame(252, 40, ng_button_pics[0]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(284, 44, difficulty_names[game_config.newgame_difficulty]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(283, 43, difficulty_names[game_config.newgame_difficulty]);
    }
    if (menu_selection != ctl_opponents) {
        Draw_Pic_Frame(251, 66, ng_button_pics[1]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(283, 71, opponent_names[game_config.newgame_opponents - 1]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(282, 70, opponent_names[game_config.newgame_opponents - 1]);
    } else {
        Draw_Pic_Frame(252, 67, ng_button_pics[1]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(284, 72, opponent_names[game_config.newgame_opponents - 1]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(283, 71, opponent_names[game_config.newgame_opponents - 1]);
    }
    if (menu_selection != ctl_land_size) {
        Draw_Pic_Frame(251, 93, ng_button_pics[2]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(283, 97, land_size_names[game_config.newgame_land_size]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(282, 96, land_size_names[game_config.newgame_land_size]);
    } else {
        Draw_Pic_Frame(252, 94, ng_button_pics[2]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(284, 98, land_size_names[game_config.newgame_land_size]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(283, 97, land_size_names[game_config.newgame_land_size]);
    }
    if (menu_selection != ctl_magic_power) {
        Draw_Pic_Frame(251, 120, ng_button_pics[3]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(283, 124, magic_power_names[game_config.newgame_magic_power]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(282, 123, magic_power_names[game_config.newgame_magic_power]);
    } else {
        Draw_Pic_Frame(252, 121, ng_button_pics[3]);
        Font_Select_Palette(3, light);
        Print_Text_Centered(284, 125, magic_power_names[game_config.newgame_magic_power]);
        Font_Select_Palette(3, dark);
        Print_Text_Centered(283, 124, magic_power_names[game_config.newgame_magic_power]);
    }
}

/* () a seed from the options (computed, never used) and the 6 x 11 random book looks (0 .. 2, Random is 1-based) of the spell-book
   pictures */
void NewGame_Roll_Book_Looks(void)
{
    long seed;
    long i;

    seed = game_config.newgame_magic_power * game_config.newgame_opponents *
           (game_config.newgame_land_size * game_config.newgame_difficulty) * 123 + 1234;
    for (i = 0; i < 11; i++) {
        ng_book_looks[5][i] = Random(3) - 1;
        ng_book_looks[4][i] = Random(3) - 1;
        ng_book_looks[3][i] = Random(3) - 1;
        ng_book_looks[1][i] = Random(3) - 1;
        ng_book_looks[2][i] = Random(3) - 1;
        ng_book_looks[0][i] = Random(3) - 1;
    }
}

/* () the wizard selection: the 14 presets (7 + 7) and Custom (the 15th button, not on Intro) -> 3 (a preset taken),
   2 (Custom), 0 (Esc, back to the options) */
int Screen_Select_Wizard(void)
{
    int faded;
    int done;
    int input;
    int hotkey;
    int i;
    int rows;                                   /* the right column's buttons: 7 on Intro, 8 with Custom */

    rows = 0;
    if (game_config.newgame_difficulty < 1)
        rows = 7;
    else
        rows = 8;
    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    for (i = 0; i < 15; i++)
        ng_button_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 9, p_scratch);
    if (rows == 0)                              /* never: rows is 7 or 8 */
        ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 8, p_scratch);
    else
        ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 39, p_scratch);
    for (i = 0; i < 15; i++)
        ng_book_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 24, p_scratch);
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    done = 0;
    faded = 0;
    ng_shown_preset = 0;
    ng_right_rows = NG_LIST_WIZARDS;
    ng_controls[14] = UI_NO_CONTROL;
    for (i = 0; i < 7; i++)
        ng_controls[i] = UI_Add_Region(168, i * 22 + 26, 237, i * 22 + 42, "", -1);
    for (i = 0; i < rows; i++)
        ng_controls[i + 7] = UI_Add_Region(244, i * 22 + 26, 313, i * 22 + 42, "", -1);
    hotkey = UI_Add_Hotkey("\x1b");
    UI_Set_Redraw_Fn(SelectWizard_Redraw, 1);
    Help_Load_Select_Wizard(game_config.newgame_difficulty < 1 ? 0 : 1);
    UI_Input_Delay(2);
    while (done == 0) {
        input = UI_Get_Input();
        Timer_Mark();
        if (input == hotkey)
            return NG_STATE_OPTIONS;
        for (i = 0; i < 14; i++) {
            if (ng_controls[i] == input) {
                UI_Suspend_Redraw_Fn();
                UI_Clear_Help();
                Wizard_Apply_Preset(i);
                return NG_STATE_NAME;
            }
        }
        if (input == ng_controls[14]) {
            UI_Suspend_Redraw_Fn();
            UI_Clear_Help();
            return NG_STATE_PORTRAIT;
        }
        if (done == 0) {
            SelectWizard_Redraw();
            Palette_Upload();
            UI_Present_Frame();
            if (faded == 0) {
                faded = 1;
                Screen_Copy_Front();
            }
            Wait_Ticks(2);
        }
    }
}

/* () redraw of the wizard (and portrait) selection: the preset under the mouse is shown - portrait, name, retort,
   spellbooks; the 15 buttons (pressed: one pixel right / down) */
void SelectWizard_Redraw(void)
{
    int pressed;
    int count;
    char light[8] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[8] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    char gold[2] = { 0xb7, 0xb2 };
    char text[30];
    int i;

    pressed = UI_Get_Pressed_Control();
    if (UI_Hit_Test() != 0)
        ng_shown_preset = UI_Hit_Test() - 1;   /* control ids are 1-based: the preset regions were added first */
    Draw_Pic_Frame(0, 0, screen_background_pic);
    Draw_Pic_Frame(165, 17, ng_panel_pic);
    Font_Select_Shadow(5, 5);
    Print_Text_Centered(242, 1, ng_right_rows == NG_LIST_WIZARDS ? "Select Wizard" : "Select Picture");
    Fill_Rect(168, 27, 237, 42, 0);
    Fill_Rect(168, 48, 237, 64, 0);
    Fill_Rect(168, 70, 237, 86, 0);
    Fill_Rect(168, 92, 237, 108, 0);
    Fill_Rect(168, 114, 237, 130, 0);
    Fill_Rect(168, 136, 237, 152, 0);
    Fill_Rect(168, 158, 237, 174, 0);
    Fill_Rect(244, 26, 313, 42, 0);
    Fill_Rect(244, 48, 313, 64, 0);
    Fill_Rect(244, 70, 313, 86, 0);
    Fill_Rect(244, 92, 313, 108, 0);
    Fill_Rect(244, 114, 313, 130, 0);
    Fill_Rect(244, 136, 313, 152, 0);
    Fill_Rect(244, 158, 313, 174, 0);
    if (ng_right_rows == NG_LIST_WIZARDS && game_config.newgame_difficulty > 0)
        Fill_Rect(244, 180, 313, 196, 0);
    if (ng_shown_preset < 14)
        Draw_Pic_Frame(24, 10, portrait_pics[ng_shown_preset]);
    Font_Select(3, 15);
    for (i = 0; i < 7; i++) {
        if (ng_controls[i] != pressed) {
            Draw_Pic_Frame(169, i * 22 + 27, ng_button_pics[i]);
            Font_Select_Palette(3, light);
            Print_Text_Centered(203, i * 22 + 31, wizard_presets[i].name);
            Font_Select_Palette(3, dark);
            Print_Text_Centered(202, i * 22 + 30, wizard_presets[i].name);
        } else {
            Draw_Pic_Frame(170, i * 22 + 28, ng_button_pics[i]);
            Font_Select_Palette(3, light);
            Print_Text_Centered(204, i * 22 + 32, wizard_presets[i].name);
            Font_Select_Palette(3, dark);
            Print_Text_Centered(203, i * 22 + 31, wizard_presets[i].name);
        }
    }
    count = ng_right_rows;
    if (game_config.newgame_difficulty < 1)
        count = 7;
    for (i = 0; i < count; i++) {
        if (ng_controls[i + 7] != pressed) {
            Draw_Pic_Frame(245, i * 22 + 27, ng_button_pics[i + 7]);
            Font_Select_Palette(3, light);
            Print_Text_Centered(279, i * 22 + 31, wizard_presets[i + 7].name);
            Font_Select_Palette(3, dark);
            Print_Text_Centered(278, i * 22 + 30, wizard_presets[i + 7].name);
        } else {
            Draw_Pic_Frame(246, i * 22 + 28, ng_button_pics[i + 7]);
            Font_Select_Palette(3, light);
            Print_Text_Centered(280, i * 22 + 32, wizard_presets[i + 7].name);
            Font_Select_Palette(3, dark);
            Print_Text_Centered(279, i * 22 + 31, wizard_presets[i + 7].name);
        }
    }
    if (wizard_presets[ng_shown_preset].retort != -1 && ng_right_rows == NG_LIST_WIZARDS && ng_shown_preset < 14) {
        Font_Select(0, 15);
        Font_Select_Palette(0, dark);
        strcpy(text, retort_names[wizard_presets[ng_shown_preset].retort]);
        strcat(text, ".");
        Print_Text(13, 181, text);
        Font_Select_Palette(0, gold);
        Print_Text(12, 180, text);
    }
    if (ng_right_rows == NG_LIST_WIZARDS)
        NewGame_Draw_Spellbooks(ng_shown_preset);
    if (ng_shown_preset < 14) {
        Font_Select(4, 15);
        Font_Select_Palette(4, light);
        Print_Text_Centered(78, 120, wizard_presets[ng_shown_preset].name);
        Print_Text_Centered(77, 120, wizard_presets[ng_shown_preset].name);
        Font_Select_Palette(4, dark);
        Print_Text_Centered(77, 119, wizard_presets[ng_shown_preset].name);
    }
}

/* () the custom wizard's portrait: one of the 14 preset pictures (also takes that preset's name) -> 3; Esc -> 1 */
int Screen_Select_Portrait(void)
{
    int input;
    int i;
    int hotkey;
    register int done;
    register int faded;

    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    for (i = 0; i < 15; i++)
        ng_button_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 9, p_scratch);
    ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 39, p_scratch);
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    done = 0;
    faded = 0;
    ng_shown_preset = 0;
    ng_right_rows = NG_LIST_PORTRAITS;
    for (i = 0; i < 7; i++)
        ng_controls[i] = UI_Add_Region(168, i * 22 + 26, 237, i * 22 + 42, "", -1);
    for (i = 0; i < 7; i++)
        ng_controls[i + 7] = UI_Add_Region(244, i * 22 + 26, 313, i * 22 + 42, "", -1);
    hotkey = UI_Add_Hotkey("\x1b");
    UI_Set_Redraw_Fn(SelectWizard_Redraw, 1);
    Help_Load_Select_Portrait();
    UI_Input_Delay(2);
    while (!done) {
        input = UI_Get_Input();
        Timer_Mark();
        if (input == hotkey)
            return NG_STATE_WIZARD;
        for (i = 0; i < 14; i++) {
            if (ng_controls[i] == input) {
                UI_Suspend_Redraw_Fn();
                UI_Clear_Help();
                wizards[PLAYER_HUMAN].portrait = i;
                strcpy(wizards[PLAYER_HUMAN].wizard_name, wizard_presets[i].name);
                done = 1;
            }
        }
        if (!done) {
            SelectWizard_Redraw();
            Palette_Upload();
            UI_Present_Frame();
            if (!faded) {
                faded = 1;
                Screen_Copy_Front();
            }
            Wait_Ticks(2);
        }
    }
    return NG_STATE_NAME;
}

/* () the wizard's name: a 10-character edit box (blank keeps the old name) -> 0; Esc -> -1 */
int Screen_Wizard_Name(void)
{
    int input_result;
    char name[16];

    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 40, p_scratch);
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    UI_Set_Redraw_Fn(WizardName_Redraw, 1);
    WizardName_Redraw();
    Palette_Upload();
    UI_Present_Frame();
    Font_Select(3, 8, 8);
    strcpy(name, wizards[PLAYER_HUMAN].wizard_name);
    input_result = UI_Text_Input_Box(195, 36, 80, name, 10, 0, 0, 1, name_box_colors, -1);
    Str_Trim(name);
    if ((int)strlen(name) < 1)
        strcpy(name, wizards[PLAYER_HUMAN].wizard_name);
    strcpy(wizards[PLAYER_HUMAN].wizard_name, name);
    UI_Suspend_Redraw_Fn();
    if (input_result == -1)
        return NG_STATE_BACK;
    else
        return 0;                               /* done: NewGame_Run tests only NG_STATE_BACK */
}

/* () redraw of the name screen: portrait, name; a preset wizard (not the custom path) also shows books and retorts */
void WizardName_Redraw(void)
{
    char light[8] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[8] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };

    Draw_Pic_Frame(0, 0, screen_background_pic);
    Draw_Pic_Frame(181, 17, ng_panel_pic);
    Font_Select_Shadow(5, 5);
    Print_Text_Centered(242, 1, "Wizard's Name");
    Draw_Pic_Frame(24, 10, portrait_pics[wizards[PLAYER_HUMAN].portrait]);
    Font_Select(4, 15);
    Font_Select_Palette(4, light);
    Print_Text_Centered(78, 120, wizards[PLAYER_HUMAN].wizard_name);
    Print_Text_Centered(77, 120, wizards[PLAYER_HUMAN].wizard_name);
    Font_Select_Palette(4, dark);
    Print_Text_Centered(77, 119, wizards[PLAYER_HUMAN].wizard_name);
    if (ng_right_rows != NG_LIST_PORTRAITS) {   /* the custom path (Select Picture) has no books / retorts yet */
        ng_right_rows = 0;                      /* neither list from here on */
        NewGame_Draw_Spellbooks(wizards[PLAYER_HUMAN].portrait);
        NewGame_Draw_Retort_Text();
    }
}

/* () the banner color: green, blue, red, purple, yellow -> 99 (done); Esc -> 6 (back to the race) */
int Screen_Select_Banner(void)
{
    int faded;
    int input;
    char unused_colors[8] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    int hotkey;
    int i;
    int done;

    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 46, p_scratch);
    for (i = 0; i < 15; i++)
        ng_book_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 24, p_scratch);
    worldgen_box_pic = LBX_Reload_Next("NEWGAME.LBX", 53, p_scratch);
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    done = 0;
    faded = 0;
    ng_controls[0] = UI_Add_Region(175, 21, 313, 55, "", -1);
    ng_controls[1] = UI_Add_Region(175, 56, 313, 93, "", -1);
    ng_controls[2] = UI_Add_Region(175, 94, 313, 128, "", -1);
    ng_controls[3] = UI_Add_Region(175, 129, 313, 162, "", -1);
    ng_controls[4] = UI_Add_Region(175, 163, 313, 199, "", -1);
    hotkey = UI_Add_Hotkey("\x1b");
    UI_Set_Redraw_Fn(SelectBanner_Redraw, 1);
    Help_Load_Select_Banner();
    while (!done) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input == hotkey)
            return NG_STATE_RACE;
        for (i = 0; i < 5; i++) {
            if (ng_controls[i] == input) {
                switch (i) {
                case 0:
                    wizards[PLAYER_HUMAN].banner_color = BANNER_GREEN;
                    break;
                case 1:
                    wizards[PLAYER_HUMAN].banner_color = BANNER_BLUE;
                    break;
                case 2:
                    wizards[PLAYER_HUMAN].banner_color = BANNER_RED;
                    break;
                case 3:
                    wizards[PLAYER_HUMAN].banner_color = BANNER_PURPLE;
                    break;
                case 4:
                    wizards[PLAYER_HUMAN].banner_color = BANNER_YELLOW;
                    break;
                }
                done = 1;
            }
        }
        if (!done) {
            SelectBanner_Redraw();
            Palette_Upload();
            UI_Present_Frame();
            if (faded == 0) {
                faded = 1;
                Screen_Copy_Front();
            }
            Wait_Ticks(2);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    return NG_STATE_DONE;
}

/* () redraw of the banner screen: portrait, name, the spellbooks (Life, Sorcery, Nature, Death, Chaos; book pictures
   by ng_book_looks[0] - the same row for every realm), retorts */
void SelectBanner_Redraw(void)
{
    char light[5] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[5] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    int i;
    int x;

    Draw_Pic_Frame(0, 0, screen_background_pic);
    Draw_Pic_Frame(158, 0, ng_panel_pic);
    Font_Select_Shadow(5, 5);
    Print_Text_Centered(242, 1, "Select Banner");
    Draw_Pic_Frame(24, 10, portrait_pics[wizards[PLAYER_HUMAN].portrait]);
    Font_Select(4, 15);
    Font_Select_Palette(4, light);
    Print_Text_Centered(78, 120, wizards[PLAYER_HUMAN].wizard_name);
    Print_Text_Centered(77, 120, wizards[PLAYER_HUMAN].wizard_name);
    Font_Select_Palette(4, dark);
    Print_Text_Centered(77, 119, wizards[PLAYER_HUMAN].wizard_name);
    x = NG_BOOK_STRIP_X;
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i]]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_SORCERY]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_NATURE]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_DEATH]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_CHAOS]);
        x += NG_BOOK_STEP;
    }
    NewGame_Draw_Retort_Text();
}

/* () the race: 9 Arcanian races and 5 Myrran ones (only with the Myrran retort, else a message); the choice flashes
   4 times -> 7 (the banner); Esc -> -1 */
int Screen_Select_Race(void)
{
    int faded;
    int done;
    int input;
    int arcanus[9] = { RACE_BARBARIAN, RACE_GNOLL, RACE_HALFLING, RACE_HIGH_ELF, RACE_HIGH_MEN, RACE_KLACKON, RACE_LIZARDMAN, RACE_NOMAD, RACE_ORC };
    int myrran[5] = { RACE_BEASTMEN, RACE_DARK_ELF, RACE_DRACONIAN, RACE_DWARVEN, RACE_TROLL };
    char dark[8] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    char highlight[2] = { 0x14, 0xb0 };         /* SelectRace_Redraw's highlight / normal colours: the chosen race
                                                   blinks, an Arcanian one normal first, a Myrran one highlight first */
    char normal[2] = { 0x19, 0x32 };
    char shadow[2] = { 0x02, 0x02 };
    char *retorts;
    int hotkey;
    int i;
    int j;

    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 54, p_scratch);
    ng_button_pics[0] = LBX_Reload_Next("NEWGAME.LBX", 55, p_scratch);
    msgbox_top_pic = LBX_Reload_Next("NEWGAME.LBX", 44, p_scratch);
    msgbox_bottom_pic = LBX_Reload_Next("NEWGAME.LBX", 45, p_scratch);
    for (i = 0; i < 15; i++)
        ng_book_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 24, p_scratch);
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    done = 0;
    faded = 0;
    retorts = wizards[PLAYER_HUMAN].retorts;
    for (i = 0; i < 9; i++)
        ng_controls[i] = UI_Add_Region(211, i * 10 + 38, 270, i * 10 + 45, "", -1);
    for (i = 0; i < 5; i++)
        ng_controls[i + 9] = UI_Add_Region(211, i * 10 + 147, 270, i * 10 + 154, "", -1);
    hotkey = UI_Add_Hotkey("\x1b");
    UI_Set_Redraw_Fn(SelectRace_Redraw, 1);
    Help_Load_Select_Race();
    while (done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input == hotkey)
            return NG_STATE_BACK;
        for (i = 0; i < 9; i++) {
            if (ng_controls[i] == input) {
                ng_chosen_race = arcanus[i];
                done = 1;
            }
        }
        for (i = 9; i < 14; i++) {
            if (retorts[RETORT_MYRRAN] == 1) {
                if (ng_controls[i] == input) {
                    ng_chosen_race = myrran[i - 9];
                    done = 1;
                }
            } else if (ng_controls[i] == input) {
                UI_Suspend_Redraw_Fn();
                UI_Clear_Help();
                Message_Box("You can not select a Myrran race unless you have the Myrran special.");
                UI_Set_Redraw_Fn(SelectRace_Redraw, 1);
                UI_Set_Help(help_entries, NG_HELP_RACE);
            }
        }
        if (done == 0) {
            SelectRace_Redraw();
            Palette_Upload();
            UI_Present_Frame();
            if (faded == 0) {
                faded = 1;
                Screen_Copy_Front();
            }
            Wait_Ticks(2);
        }
    }
    for (i = 0; i < 4; i++) {
        Timer_Mark();
        for (j = 0; j < 9; j++) {
            if (arcanus[j] == ng_chosen_race) {
                Font_Select_Palette(2, shadow);
                Print_Text(221, j * 10 + 39, race_table[arcanus[j]].name);
                Font_Select_Palette(2, i == 0 || i == 2 ? normal : highlight);
                Print_Text(220, j * 10 + 38, race_table[arcanus[j]].name);
                j = 9;
            }
        }
        for (j = 9; j < 14; j++) {
            if (myrran[j - 9] == ng_chosen_race) {
                Font_Select_Palette(2, shadow);
                Print_Text(221, (j - 9) * 10 + 148, race_table[myrran[j - 9]].name);
                Font_Select_Palette(2, i == 0 || i == 2 ? highlight : normal);
                Print_Text(220, (j - 9) * 10 + 147, race_table[myrran[j - 9]].name);
                j = 14;
            }
        }
        Palette_Upload();
        UI_Present_Frame();
        if (faded == 0) {
            faded = 1;
            Screen_Copy_Front();
        }
        Wait_Ticks(1);
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    return NG_STATE_BANNER;
}

/* () redraw of the race screen: portrait, name, books, retorts, and the two race lists (the one under the mouse
   highlighted; the Myrran races greyed without the Myrran retort) */
void SelectRace_Redraw(void)
{
    int x;
    int hit;
    int arcanus[9] = { RACE_BARBARIAN, RACE_GNOLL, RACE_HALFLING, RACE_HIGH_ELF, RACE_HIGH_MEN, RACE_KLACKON, RACE_LIZARDMAN, RACE_NOMAD, RACE_ORC };
    int myrran[5] = { RACE_BEASTMEN, RACE_DARK_ELF, RACE_DRACONIAN, RACE_DWARVEN, RACE_TROLL };
    char light[5] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[5] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    char orange[5] = { 0xb4, 0xb4, 0xb4, 0xb4, 0xb4 };
    char highlight[2] = { 0x14, 0xb0 };
    char normal[2] = { 0x19, 0x32 };
    char grey[2] = { 0x19, 0x16 };
    char shadow[2] = { 0x02, 0x02 };
    char *retorts;
    int i;
    int j;

    Draw_Pic_Frame(0, 0, screen_background_pic);
    Draw_Pic_Frame(164, 18, ng_panel_pic);
    Draw_Pic_Frame(208, 34, ng_button_pics[0]);
    Font_Select_Shadow(5, 5);
    Print_Text_Centered(242, 1, "Select Race");
    Draw_Pic_Frame(24, 10, portrait_pics[wizards[PLAYER_HUMAN].portrait]);
    Font_Select(4, 15);
    Font_Select_Palette(4, light);
    Print_Text_Centered(78, 120, wizards[PLAYER_HUMAN].wizard_name);
    Print_Text_Centered(77, 120, wizards[PLAYER_HUMAN].wizard_name);
    Font_Select_Palette(4, dark);
    Print_Text_Centered(77, 119, wizards[PLAYER_HUMAN].wizard_name);
    hit = UI_Hit_Test();
    retorts = wizards[PLAYER_HUMAN].retorts;
    x = NG_BOOK_STRIP_X;
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i]]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_SORCERY]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_NATURE]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_DEATH]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_CHAOS]);
        x += NG_BOOK_STEP;
    }
    NewGame_Draw_Retort_Text();
    Font_Select_Palette(3, orange);
    Font_Select_Shadow(3, 15);
    Print_Text(198, 25, "Arcanian Races:");
    for (j = 0; j < 9; j++) {
        Font_Select_Palette(2, shadow);
        Print_Text(221, j * 10 + 39, race_table[arcanus[j]].name);
        if (ng_controls[j] == hit)
            Font_Select_Palette(2, highlight);
        else
            Font_Select_Palette(2, normal);
        Print_Text(220, j * 10 + 38, race_table[arcanus[j]].name);
    }
    Font_Select_Palette(3, orange);
    Font_Select_Shadow(3, 15);
    Print_Text(202, 134, "Myrran Races:");
    for (j = 0; j < 5; j++) {
        Font_Select_Palette(2, shadow);
        Print_Text(221, j * 10 + 148, race_table[myrran[j]].name);
        if (retorts[RETORT_MYRRAN] == 1) {
            if (ng_controls[j + 9] == hit)
                Font_Select_Palette(2, highlight);
            else
                Font_Select_Palette(2, normal);
        } else
            Font_Select_Palette(2, grey);
        Print_Text(220, j * 10 + 147, race_table[myrran[j]].name);
    }
}

/* () the human wizard's retorts as one sentence ("A, B and C.") under the portrait, gold over dark brown; two pixels
   higher (y 178, not 180) with more than 5 */
void NewGame_Draw_Retort_Text(void)
{
    char dark[4] = { 0xbb, 0xbb, 0xbb, 0xbb };
    char gold[2] = { 0xb7, 0xb2 };
    char *retorts;
    int unused;
    int n;
    int y;
    char text[126];
    int unused_ff[6] = { -1, -1, -1, -1, -1, -1 };
    int i;
    int total;

    unused = 0;
    n = 0;
    text[0] = 0;
    retorts = wizards[PLAYER_HUMAN].retorts;
    total = 0;
    for (i = 0; i < 18; i++) {
        if (retorts[i] == 1)
            total++;
    }
    strcpy(text, "");
    n = 0;
    for (i = 0; i < 18; i++) {
        if (retorts[i] == 1) {
            Text_List_Separator(&n, total, text);
            strcat(text, retort_names[i]);
        }
    }
    if (total > 0)
        strcat(text, ".");
    if (total > 5)
        y = 178;
    else
        y = 180;
    Font_Select(0, 15);
    Font_Select_Palette(0, dark);
    Set_Line_Spacing(1);
    Paragraph_Print(13, y + 1, 138, text, PARA_LEFT);
    Font_Select_Palette(0, gold);
    Set_Line_Spacing(1);
    Paragraph_Print(12, y, 138, text, PARA_LEFT);
}

/* (preset) the preset's books in its two realms (Life, Sorcery, Nature, Death, Chaos order) under the portrait */
void NewGame_Draw_Spellbooks(int preset)
{
    int first_pic_base;                         /* realm x NG_BOOK_LOOKS: the first / second realm's picture group */
    int second_pic_base;
    int *books;
    int first_count;
    int second_count;
    int first_realm;
    int second_realm;
    int i;
    int x;

    first_pic_base = -1;
    second_pic_base = -1;
    first_count = 0;
    second_count = 0;
    x = NG_BOOK_STRIP_X;
    first_realm = -1;                           /* first_realm / second_realm: set, never read */
    second_realm = -1;
    books = &wizard_presets[preset].books_life;
    for (i = 0; i < 5; i++) {
        if (books[i] > 0) {
            if (first_pic_base == -1) {
                first_pic_base = i * NG_BOOK_LOOKS;
                first_count += books[i];
                first_realm = i;
            } else {
                second_pic_base = i * NG_BOOK_LOOKS;
                second_count += books[i];
                second_realm = i;
            }
        }
    }
    if (first_pic_base != -1) {
        for (i = 0; i < first_count; i++) {
            Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[first_pic_base + ng_book_looks[0][i]]);
            x += NG_BOOK_STEP;
        }
    }
    if (second_pic_base != -1) {
        for (i = 0; i < second_count; i++) {
            Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[second_pic_base + ng_book_looks[0][i]]);
            x += NG_BOOK_STEP;
        }
    }
}

/* () the custom wizard's picks: 11 picks shared by the five realm sliders (books) and the 18 retort buttons (five
   cost 2 - Warlord, Channeler, Divine Power, Famous, Infernal Power at grid slots 1 2 8 9 15 -, Myrran 3, its refusal
   showing the "You need 2 picks" text; at most 6 retorts; Life excludes Death). A refused click explains itself in
   a message box (ng_message 1 .. 6; 3 lists the retort's book requirements). OK after all picks -> 5 (spells);
   Esc -> 3 */
int Screen_Custom_Picks(void)
{
    int faded;
    int done;                                   /* inside the retort loop: a 'handled' flag */
    int input;
    int count;
    int *req_flat;
    int n;
    char *retorts;
    char num_text[6];
    char text[150];
    char pick_realm_names[5][10] = { "Life", "Death", "Chaos", "Nature", "Sorcery" };   /* retort_requirements' column order */
    int two_pick_slots[5] = { 1, 2, 8, 9, 15 };
    int hotkey;
    int i;
    int j;

    screen_background_pic = LBX_Reload("NEWGAME.LBX", 41, p_scratch);
    ng_ok_pic = LBX_Reload_Next("NEWGAME.LBX", 42, p_scratch);
    ok_button_pic = LBX_Reload_Next("NEWGAME.LBX", 43, p_scratch);
    for (i = 0; i < 15; i++)
        ng_book_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 24, p_scratch);
    msgbox_top_pic = LBX_Reload_Next("NEWGAME.LBX", 44, p_scratch);
    msgbox_bottom_pic = LBX_Reload_Next("NEWGAME.LBX", 45, p_scratch);
    select_mark_pic = LBX_Reload_Next("NEWGAME.LBX", 52, p_scratch);
    Mouse_Set_Cursor_Regions(1, &mouse_region_new_game);
    UI_Clear_Controls();
    UI_Right_Click_Ids_On();
    retorts = wizards[PLAYER_HUMAN].retorts;
    done = 0;
    faded = 0;
    ng_shown_preset = 0;
    ng_picks_left = NG_TOTAL_PICKS;
    ng_picks_done = 0;
    ctl_ok = UI_Add_Region(251, 181, 282, 196, "", -1);
    ng_life_slider = 0;
    ng_death_slider = 0;
    ng_chaos_slider = 0;
    ng_nature_slider = 0;
    ng_sorcery_slider = 0;
    ng_message = 0;
    ng_last_pressed = 0;
    req_flat = retort_requirements[0];   /* [retort * 7 + k]: 0 picks in any realm, 1 realms, 2 + realm (Life ..) */
    for (i = 0; i < 18; i++)
        retorts[i] = 0;
    ng_controls[0] = UI_Add_Slider_Near(190, 47, 0, 96, 0, 95, 96, 22, &ng_life_slider, "", -1);
    ng_controls[1] = UI_Add_Slider_Near(190, 73, 0, 96, 0, 95, 96, 22, &ng_death_slider, "", -1);
    ng_controls[2] = UI_Add_Slider_Near(190, 99, 0, 96, 0, 95, 96, 22, &ng_chaos_slider, "", -1);
    ng_controls[3] = UI_Add_Slider_Near(190, 125, 0, 96, 0, 95, 96, 22, &ng_nature_slider, "", -1);
    ng_controls[4] = UI_Add_Slider_Near(190, 151, 0, 96, 0, 95, 96, 22, &ng_sorcery_slider, "", -1);
    for (i = 0; i < 6; i++) {
        ctl_retorts[i] = UI_Add_Region(164, i * 7 + 5, 202, i * 7 + 10, "", -1);
        ctl_retorts[i + 6] = UI_Add_Region(203, i * 7 + 5, 252, i * 7 + 10, "", -1);
        ctl_retorts[i + 12] = UI_Add_Region(253, i * 7 + 5, 319, i * 7 + 10, "", -1);
    }
    hotkey = UI_Add_Hotkey("\x1b");
    wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE] = 0;
    wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY] = 0;
    wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS] = 0;
    wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] = 0;
    wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] = 0;
    UI_Set_Redraw_Fn(CustomPicks_Redraw, 1);
    Help_Load_Custom_Picks();
    while (done == 0) {
        input = UI_Get_Input();
        Timer_Mark();
        if (input == hotkey)
            return NG_STATE_NAME;
        if (input == ctl_ok && ng_picks_done != 0)
            done = 1;
        count = 0;
        for (i = 0; i < 18; i++) {
            if (retorts[i] == 1)
                count++;
        }
        if ((input == ng_controls[0] && wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] != 0) ||
                (input == ng_controls[1] && wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] != 0))
            ng_message = NG_MSG_LIFE_AND_DEATH;
        if (ng_message > 0) {
            switch (ng_message) {
            case NG_MSG_PICKS_LEFT:
                strcpy(text, "You need to make all your picks before you can continue");
                break;
            case NG_MSG_MAX_RETORTS:
                strcpy(text, "You may not select more than 6 special abilities");
                break;
            case NG_MSG_RETORT_NEEDS:
                strcpy(text, "To select ");
                strcat(text, retort_names[ng_message_retort]);
                strcat(text, " you need:   ");
                if (retort_requirements[ng_message_retort][1] == 1 && retort_requirements[ng_message_retort][0] == 0) {
                    for (i = 0; i < 5; i++) {
                        if (req_flat[ng_message_retort * 7 + i + 2] > 0) {
                            itoa(req_flat[ng_message_retort * 7 + i + 2], num_text, 10);
                            strcat(text, num_text);
                            strcat(text, " picks in ");
                            strcat(text, pick_realm_names[i]);
                            strcat(text, " Magic");
                        }
                    }
                } else if (retort_requirements[ng_message_retort][0] > 0) {
                    itoa(req_flat[ng_message_retort * 7], num_text, 10);
                    strcat(text, num_text);
                    strcat(text, " pick");
                    if (req_flat[ng_message_retort * 7] > 1)
                        strcat(text, "s in any ");
                    else
                        strcat(text, " in any ");
                    if (req_flat[ng_message_retort * 7 + 1] > 1) {
                        itoa(req_flat[ng_message_retort * 7 + 1], num_text, 10);
                        strcat(text, num_text);
                        strcat(text, " Realms of Magic");
                    } else
                        strcat(text, "Realm of Magic");
                } else {
                    n = 0;
                    for (j = 0; j < 5; j++) {
                        if (req_flat[ng_message_retort * 7 + j + 2] > 0) {
                            if (n > 0)
                                strcat(text, ", ");
                            itoa(req_flat[ng_message_retort * 7 + j + 2], num_text, 10);
                            strcat(text, num_text);
                            strcat(text, " pick in ");
                            strcat(text, pick_realm_names[j]);
                            strcat(text, " Magic");
                            n++;
                        }
                    }
                }
                break;
            case NG_MSG_LIFE_AND_DEATH:
                strcpy(text, "You can not select both Life and Death magic");
                break;
            case NG_MSG_NO_PICKS_LEFT:
                strcpy(text, "You have already made all your picks");
                break;
            case NG_MSG_TWO_PICKS:
                strcpy(text, "You don't have enough picks left to make this selection. You need 2 picks");
                break;
            default:
                break;
            }
            UI_Suspend_Redraw_Fn();
            UI_Clear_Help();
            Message_Box(text);
            UI_Set_Redraw_Fn(CustomPicks_Redraw, 1);
            UI_Set_Help(help_entries, NG_HELP_CUSTOM_PICKS);
            ng_message = 0;
        }
        for (i = 0; i < 18; i++) {
            if (ctl_retorts[i] == input) {
                if (retorts[custom_retort_slots[i]] == 1)
                    retorts[custom_retort_slots[i]] = 0;
                else if (ng_picks_left > 0) {
                    if (count > 5)
                        ng_message = NG_MSG_MAX_RETORTS;
                    else {
                        done = 0;
                        for (j = 0; j < 5; j++) {
                            if (two_pick_slots[j] == i) {
                                if (ng_picks_left < 2)
                                    ng_message = NG_MSG_TWO_PICKS;
                                else
                                    retorts[custom_retort_slots[i]] = 1;
                                done = 1;
                            }
                        }
                        if (done == 0 && i == 7) {      /* grid slot 7 = Myrran (custom_retort_slots[7]): 3 picks */
                            if (ng_picks_left < 3)
                                ng_message = NG_MSG_TWO_PICKS;
                            else
                                retorts[RETORT_MYRRAN] = 1;
                            done = 1;
                        }
                        if (done == 0)
                            retorts[custom_retort_slots[i]] = 1;
                        done = 0;
                    }
                } else
                    ng_message = NG_MSG_NO_PICKS_LEFT;
            }
        }
        if (done == 0) {
            CustomPicks_Redraw();
            Palette_Upload();
            UI_Present_Frame();
            if (faded == 0) {
                faded = 1;
                Screen_Copy_Front();
            }
            Wait_Ticks(2);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    return NG_STATE_SPELLS;
}

/* () redraw of the custom picks screen; it also applies the sliders: picks left = 11 - retorts (Warlord, Channeler,
   Divine / Infernal Power, Famous cost 2, Myrran 3) - books; a pressed realm slider takes books one at a time while
   picks last (Life and Death exclude each other). Retorts become available by books (a realm Mastery or Divine /
   Infernal Power with 4 books there, Archmage / Mana Focusing with 4 in any realm, Sage Master with 2 realms,
   Runemaster with 3 realms of 2+ books, Node Mastery with Chaos, Sorcery and Nature); a taken retort that loses its
   requirement is dropped with message 3. The third retort column reads its names through custom_retort_slots[i +
   j * 6] with j left at 2 by the loop before (= i + 12) */
void CustomPicks_Redraw(void)
{
    int pressed;
    int old_left;
    int x;
    int count;
    int realms_2;
    int realms;
    int used;
    int two_pick_slots[5] = { 1, 2, 8, 9, 15 };
    char light[5] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[5] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    char normal[2] = { 0x19, 0x32 };
    char grey[2] = { 0x19, 0x16 };
    char chosen[2] = { 0x16, 0xb2 };
    char shadow[2] = { 0x10, 0x10 };
    char *retorts;
    char text[20];
    char available[18] = { 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0 };   /* by grid slot (custom_retort_slots) */
    int i;
    int j;

    retorts = wizards[PLAYER_HUMAN].retorts;
    pressed = UI_Get_Pressed_Control();
    x = NG_BOOK_STRIP_X;
    Draw_Pic_Frame(0, 0, screen_background_pic);
    Draw_Pic_Frame(24, 10, portrait_pics[wizards[PLAYER_HUMAN].portrait]);
    Fill_Rect(251, 181, 282, 196, 0);
    count = 0;
    for (i = 0; i < 18; i++) {
        if (retorts[i] == 1)
            count++;
    }
    used = count;
    for (i = 0; i < 5; i++) {
        if (retorts[custom_retort_slots[two_pick_slots[i]]] == 1)
            used++;
    }
    if (retorts[RETORT_MYRRAN] == 1)
        used += 2;
    ng_picks_left = NG_TOTAL_PICKS - used - wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] - wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] - wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS] -
                        wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE] - wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY];
    old_left = ng_picks_left;
    for (i = 0; i < 5; i++) {
        if (ng_controls[i] == pressed) {
            switch (i) {
            case 0:
                ng_picks_left = ng_picks_left + wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE];
                for (j = 0; j < ng_life_slider / 8; j++) {
                    if (wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] != 0)
                        break;
                    if (ng_picks_left == 0)
                        break;
                    ng_picks_left--;
                }
                wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] = j;
                if (ng_life_slider / 8 == j && ng_picks_left == old_left && ng_picks_left == 0)
                    ng_picks_left = -1;
                break;
            case 1:
                ng_picks_left = ng_picks_left + wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH];
                for (j = 0; j < ng_death_slider / 8; j++) {
                    if (wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] != 0)
                        break;
                    if (ng_picks_left == 0)
                        break;
                    ng_picks_left--;
                }
                wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] = j;
                if (ng_death_slider / 8 == j && ng_picks_left == old_left && ng_picks_left == 0)
                    ng_picks_left = -1;
                break;
            case 2:
                ng_picks_left = ng_picks_left + wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS];
                for (j = 0; j < ng_chaos_slider / 8; j++) {
                    if (ng_picks_left == 0)
                        break;
                    ng_picks_left--;
                }
                wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS] = j;
                if (ng_chaos_slider / 8 == j && ng_picks_left == old_left && ng_picks_left == 0)
                    ng_picks_left = -1;
                break;
            case 3:
                ng_picks_left = ng_picks_left + wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE];
                for (j = 0; j < ng_nature_slider / 8; j++) {
                    if (ng_picks_left == 0)
                        break;
                    ng_picks_left--;
                }
                wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE] = j;
                if (ng_nature_slider / 8 == j && ng_picks_left == old_left && ng_picks_left == 0)
                    ng_picks_left = -1;
                break;
            case 4:
                ng_picks_left = ng_picks_left + wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY];
                for (j = 0; j < ng_sorcery_slider / 8; j++) {
                    if (ng_picks_left == 0)
                        break;
                    ng_picks_left--;
                }
                wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY] = j;
                if (ng_sorcery_slider / 8 == j && ng_picks_left == old_left && ng_picks_left == 0)
                    ng_picks_left = -1;
                break;
            }
            if (ng_picks_left == 0 && old_left == ng_picks_left && ng_last_pressed != pressed)
                ng_message = NG_MSG_NO_PICKS_LEFT;
            if (ng_picks_left < 0)
                ng_picks_left = 0;
        }
    }
    ng_last_pressed = pressed;
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE]; i++) {
        Draw_Pic_Frame(i * 8 + 197, 49, ng_book_pics[ng_book_looks[2][i]]);
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i]]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH]; i++) {
        Draw_Pic_Frame(i * 8 + 197, 75, ng_book_pics[ng_book_looks[1][i] + NG_BOOK_PICS_DEATH]);
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_DEATH]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS]; i++) {
        Draw_Pic_Frame(i * 8 + 197, 101, ng_book_pics[ng_book_looks[5][i] + NG_BOOK_PICS_CHAOS]);
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_CHAOS]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE]; i++) {
        Draw_Pic_Frame(i * 8 + 197, 127, ng_book_pics[ng_book_looks[3][i] + NG_BOOK_PICS_NATURE]);
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_NATURE]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY]; i++) {
        Draw_Pic_Frame(i * 8 + 197, 153, ng_book_pics[ng_book_looks[4][i] + NG_BOOK_PICS_SORCERY]);
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_SORCERY]);
        x += NG_BOOK_STEP;
    }
    if (ng_picks_done != 0) {
        if (pressed == ctl_ok)
            Draw_Pic_Frame(253, 183, ng_ok_pic);
        else
            Draw_Pic_Frame(252, 182, ng_ok_pic);
    } else {
        Draw_Pic_Frame(252, 182, ok_button_pic);
        if (pressed == ctl_ok)
            ng_message = NG_MSG_PICKS_LEFT;
    }
    if (wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE] > 3) {
        available[13] = 1;
        available[3] = 1;
        available[16] = 1;
    }
    if (wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY] > 3) {
        available[14] = 1;
        available[3] = 1;
        available[16] = 1;
    }
    if (wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS] > 3) {
        available[12] = 1;
        available[3] = 1;
        available[16] = 1;
    }
    if (wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] > 3) {
        available[15] = 1;
        available[3] = 1;
        available[16] = 1;
    }
    if (wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] > 3) {
        available[8] = 1;
        available[3] = 1;
        available[16] = 1;
    }
    realms = 0;
    realms_2 = 0;
    for (i = 0; i < 5; i++) {
        if (wizards[PLAYER_HUMAN].spellbooks[i] > 0)
            realms++;
        if (wizards[PLAYER_HUMAN].spellbooks[i] > 1)
            realms_2++;
    }
    if (realms > 1)
        available[6] = 1;
    if (realms_2 > 2)
        available[10] = 1;
    for (i = 0; i < 5; i++) {
        if (retorts[custom_retort_slots[two_pick_slots[i]]] != 1 && ng_picks_left < 2)
            available[two_pick_slots[i]] = 0;
    }
    if (retorts[RETORT_MYRRAN] != 1 && ng_picks_left < 3)
        available[7] = 0;
    if (wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS] > 0 && wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY] > 0 && wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE] > 0)
        available[17] = 1;
    Font_Select(0, 15);
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 6; i++) {
            if (retorts[custom_retort_slots[i + j * 6]] == 1 && available[i + j * 6] == 1 && ng_picks_left >= 0) {
                Font_Select_Palette(0, shadow);
                Print_Text(j * 39 + 172, i * 7 + 6, retort_names[custom_retort_slots[i + j * 6]]);
                Font_Select_Palette(0, chosen);
                Print_Text(j * 39 + 171, i * 7 + 5, retort_names[custom_retort_slots[i + j * 6]]);
                Draw_Pic_Frame(j * 39 + 165, i * 7 + 6, select_mark_pic);
            } else {
                if (retorts[custom_retort_slots[i + j * 6]] == 1) {
                    if (available[i + j * 6] == 0) {
                        ng_message = NG_MSG_RETORT_NEEDS;
                        ng_message_retort = custom_retort_slots[i + j * 6];
                    }
                    retorts[custom_retort_slots[i + j * 6]] = 0;
                }
                if (available[i + j * 6] == 1 && count < 6 && ng_picks_left > 0) {
                    Font_Select_Palette(0, shadow);
                    Print_Text(j * 39 + 172, i * 7 + 6, retort_names[custom_retort_slots[i + j * 6]]);
                    Font_Select_Palette(0, normal);
                    Print_Text(j * 39 + 171, i * 7 + 5, retort_names[custom_retort_slots[i + j * 6]]);
                } else {
                    if (available[i + j * 6] == 1)
                        available[i + j * 6] = 0;
                    Font_Select_Palette(0, shadow);
                    Print_Text(j * 39 + 172, i * 7 + 6, retort_names[custom_retort_slots[i + j * 6]]);
                    Font_Select_Palette(0, grey);
                    Print_Text(j * 39 + 171, i * 7 + 5, retort_names[custom_retort_slots[i + j * 6]]);
                }
            }
        }
    }
    for (i = 0; i < 6; i++) {
        if (retorts[custom_retort_slots[i + 12]] == 1 && available[i + 12] == 1 && ng_picks_left >= 0) {
            Font_Select_Palette(0, shadow);
            Print_Text(261, i * 7 + 6, retort_names[custom_retort_slots[i + j * 6]]);
            Font_Select_Palette(0, chosen);
            Print_Text(260, i * 7 + 5, retort_names[custom_retort_slots[i + 12]]);
            Draw_Pic_Frame(254, i * 7 + 6, select_mark_pic);
        } else {
            if (retorts[custom_retort_slots[i + 12]] == 1) {
                if (available[i + 12] == 0) {
                    ng_message = NG_MSG_RETORT_NEEDS;
                    ng_message_retort = custom_retort_slots[i + 12];
                }
                retorts[custom_retort_slots[i + 12]] = 0;
            }
            if (available[i + 12] == 1 && count < 6 && ng_picks_left > 0) {
                Font_Select_Palette(0, shadow);
                Print_Text(261, i * 7 + 6, retort_names[custom_retort_slots[i + j * 6]]);
                Font_Select_Palette(0, normal);
                Print_Text(260, i * 7 + 5, retort_names[custom_retort_slots[i + 12]]);
            } else {
                if (available[i + 12] == 1)
                    available[i + 12] = 0;
                Font_Select_Palette(0, shadow);
                Print_Text(261, i * 7 + 6, retort_names[custom_retort_slots[i + j * 6]]);
                Font_Select_Palette(0, grey);
                Print_Text(260, i * 7 + 5, retort_names[custom_retort_slots[i + 12]]);
            }
        }
    }
    itoa(ng_picks_left, text, 10);
    strcat(text, " picks");
    if (ng_picks_left == 0)
        ng_picks_done = 1;
    else
        ng_picks_done = 0;
    Font_Select(3, 15);
    Font_Select_Palette(3, dark);
    Print_Text_Centered(222, 186, text);
    Font_Select_Palette(3, light);
    Print_Text_Centered(221, 185, text);
    Font_Select(4, 15);
    Font_Select_Palette(4, light);
    Print_Text_Centered(78, 120, wizards[PLAYER_HUMAN].wizard_name);
    Print_Text_Centered(77, 120, wizards[PLAYER_HUMAN].wizard_name);
    Font_Select_Palette(4, dark);
    Print_Text_Centered(77, 119, wizards[PLAYER_HUMAN].wizard_name);
    NewGame_Draw_Retort_Text();
}

/* () the starting spell picks, realm by realm in the order Life, Death, Chaos, Nature, Sorcery (realms with 2+
   books): the default picks are filled in, then clicking a spell of the realm toggles it while picks remain in its
   group (commons, uncommons, rares by start_spells_common / _uncommon / _rare[books - 2]); OK when all are made
   -> next realm; done -> 6 (race); Esc -> 4 (back to the custom picks). QUIRK #411: with no realm of 2+ books
   the screen returns 6 at once, and Esc on the race screen comes back here (custom wizard), so the custom picks
   cannot be reached again; clarified using ReMoM repo (drake178's note) */
int Screen_Select_Spells(void)
{
    int faded;
    int done;
    int input;
    int k;
    int n;
    int group;
    int books_minus_1;
    int books_row;
    int realm;
    int id;
    int *picks;
    int found;
    int *defaults;
    int realm_order[5] = { REALM_LIFE, REALM_DEATH, REALM_CHAOS, REALM_NATURE, REALM_SORCERY };
    char text[150];
    int i;
    int pick;

    screen_background_pic = LBX_Reload("NEWGAME.LBX", 0, p_scratch);
    ng_panel_pic = LBX_Reload_Next("NEWGAME.LBX", 47, p_scratch);
    ng_button_pics[0] = LBX_Reload_Next("NEWGAME.LBX", 48, p_scratch);
    ng_button_pics[1] = LBX_Reload_Next("NEWGAME.LBX", 49, p_scratch);
    ng_button_pics[2] = LBX_Reload_Next("NEWGAME.LBX", 50, p_scratch);
    ng_button_pics[3] = LBX_Reload_Next("NEWGAME.LBX", 51, p_scratch);
    ng_ok_pic = LBX_Reload_Next("NEWGAME.LBX", 42, p_scratch);
    ok_button_pic = LBX_Reload_Next("NEWGAME.LBX", 43, p_scratch);
    for (i = 0; i < 15; i++)
        ng_book_pics[i] = LBX_Reload_Next("NEWGAME.LBX", i + 24, p_scratch);
    msgbox_top_pic = LBX_Reload_Next("NEWGAME.LBX", 44, p_scratch);
    msgbox_bottom_pic = LBX_Reload_Next("NEWGAME.LBX", 45, p_scratch);
    select_mark_pic = LBX_Reload_Next("NEWGAME.LBX", 52, p_scratch);
    ng_picks_left = 0;
    ng_message = 0;
    UI_Set_Redraw_Fn(SelectSpells_Redraw, 1);
    for (i = 0; i < START_SPELL_PICKS; i++) {
        start_spells[PLAYER_HUMAN][REALM_CHAOS][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_NATURE][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_SORCERY][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_LIFE][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_DEATH][i] = 0;
    }
    LBX_Load_Records_Near("hlpentry.lbx", 36, help_entries, 0, NG_HELP_SPELLS, sizeof(struct HelpEntry));
    for (n = 0; n < 5; n++) {
        done = 1;
        select_spells_realm = realm_order[n];
        ng_picks_done = 0;
        UI_Clear_Controls();
        ctl_cancel = UI_Add_Hotkey("\x1b");
        ctl_ok = UI_Add_Region(251, 181, 282, 196, "", -1);
        for (i = 0; i < 30; i++)
            ng_controls[i] = UI_NO_CONTROL;
        if (wizards[PLAYER_HUMAN].spellbooks[select_spells_realm] > 1) {
            SelectSpells_Add_Controls();
            done = 0;
            faded = 0;
            if (select_spells_realm == REALM_CHAOS) {
                picks = start_spells[PLAYER_HUMAN][REALM_CHAOS];
                defaults = default_spells[REALM_CHAOS];
                realm = REALM_CHAOS;
            }
            if (select_spells_realm == REALM_SORCERY) {
                picks = start_spells[PLAYER_HUMAN][REALM_SORCERY];
                defaults = default_spells[REALM_SORCERY];
                realm = REALM_SORCERY;
            }
            if (select_spells_realm == REALM_NATURE) {
                picks = start_spells[PLAYER_HUMAN][REALM_NATURE];
                defaults = default_spells[REALM_NATURE];
                realm = REALM_NATURE;
            }
            if (select_spells_realm == REALM_LIFE) {
                picks = start_spells[PLAYER_HUMAN][REALM_LIFE];
                defaults = default_spells[REALM_LIFE];
                realm = REALM_LIFE;
            }
            if (select_spells_realm == REALM_DEATH) {
                picks = start_spells[PLAYER_HUMAN][REALM_DEATH];
                defaults = default_spells[REALM_DEATH];
                realm = REALM_DEATH;
            }
            books_minus_1 = wizards[PLAYER_HUMAN].spellbooks[select_spells_realm] - 1;
            books_row = books_minus_1 - 1;
            for (i = 0; i < start_spells_common[books_row]; i++)
                picks[i] = defaults[i];
            for (i = 0; i < start_spells_uncommon[books_row]; i++)
                picks[i + 10] = defaults[i + 10];
            for (i = 0; i < start_spells_rare[books_row]; i++)
                picks[i + 12] = defaults[i + 12];
        }
        while (done == 0) {
            Timer_Mark();
            input = UI_Get_Input();
            if (input == ctl_cancel)
                return NG_STATE_CUSTOM_PICKS;
            if (input == ctl_ok && ng_picks_done != 0)
                done = 1;
            if (books_minus_1 > 0 && faded != 0 && input != 0) {
                group = 0;
                books_row = books_minus_1 - 1;
                if (start_spells_common[books_row] > 0 && start_spells_common[books_row] < 10) {
                    for (i = 0; i < 2; i++) {
                        for (k = 0; k < 5; k++) {
                            id = realm * SPELLS_PER_REALM + k + i * 5 + 1;
                            if (ng_controls[k + i * 5] == input) {
                                found = 0;
                                for (pick = 0; pick < start_spells_common[books_row]; pick++) {
                                    if (picks[pick] == id && found == 0) {
                                        picks[pick] = 0;
                                        found = 1;
                                    }
                                }
                                if (found == 0) {
                                    for (pick = 0; pick < start_spells_common[books_row]; pick++) {
                                        if ((picks[pick] == 0 && found == 0) || start_spells_common[books_row] == 1) {   /* a one-pick group: replaced */
                                            picks[pick] = id;
                                            found = 1;
                                        }
                                    }
                                }
                                if (found == 0)
                                    ng_message = NG_MSG_REALM_FULL;
                            }
                        }
                    }
                    group++;
                }
                if (start_spells_uncommon[books_row] > 0) {
                    for (i = 0; i < 2; i++) {
                        for (k = 0; k < 5; k++) {
                            id = realm * SPELLS_PER_REALM + k + i * 5 + 11;
                            if (ng_controls[k + i * 5 + group * 10] == input) {
                                found = 0;
                                for (pick = 0; pick < start_spells_uncommon[books_row]; pick++) {
                                    if (picks[pick + 10] == id && found == 0) {
                                        picks[pick + 10] = 0;
                                        found = 1;
                                    }
                                }
                                if (found == 0) {
                                    for (pick = 0; pick < start_spells_uncommon[books_row]; pick++) {
                                        if ((picks[pick + 10] == 0 && found == 0) || start_spells_uncommon[books_row] == 1) {
                                            picks[pick + 10] = id;
                                            found = 1;
                                        }
                                    }
                                }
                                if (found == 0)
                                    ng_message = NG_MSG_REALM_FULL;
                            }
                        }
                    }
                    group++;
                }
                if (start_spells_rare[books_row] > 0) {
                    for (i = 0; i < 2; i++) {
                        for (k = 0; k < 5; k++) {
                            id = realm * SPELLS_PER_REALM + k + i * 5 + 21;
                            if (ng_controls[k + i * 5 + group * 10] == input) {
                                found = 0;
                                for (pick = 0; pick < start_spells_rare[books_row]; pick++) {
                                    if (picks[pick + 12] == id && found == 0) {
                                        picks[pick + 12] = 0;
                                        found = 1;
                                    }
                                }
                                if (found == 0) {
                                    for (pick = 0; pick < start_spells_rare[books_row]; pick++) {
                                        if ((picks[pick + 12] == 0 && found == 0) || start_spells_rare[books_row] == 1) {
                                            picks[pick + 12] = id;
                                            found = 1;
                                        }
                                    }
                                }
                                if (found == 0)
                                    ng_message = NG_MSG_REALM_FULL;
                            }
                        }
                    }
                    group++;
                }
            }
            if (ng_message > 0) {
                switch (ng_message) {
                case NG_MSG_PICKS_LEFT:
                    strcpy(text, "You need to make all your picks before you can continue");
                    break;
                case NG_MSG_REALM_FULL:
                    strcpy(text, "You have no picks left in this area, to deselect click on a selected item");
                    break;
                default:
                    break;
                }
                UI_Suspend_Redraw_Fn();
                Message_Box(text);
                UI_Set_Redraw_Fn(SelectSpells_Redraw, 1);
                ng_message = 0;
            }
            if (done == 0) {
                SelectSpells_Redraw();
                Palette_Upload();
                UI_Present_Frame();
                if (faded == 0) {
                    faded = 1;
                    Screen_Copy_Front();
                }
                Wait_Ticks(2);
            }
        }
    }
    UI_Suspend_Redraw_Fn();
    return NG_STATE_RACE;
}

/* () the spell screen's controls: 10 per spell group shown (commons when 1 .. 9 are to pick, uncommons, rares; 51
   pixels apart) and their help entries (the spell ids) */
void SelectSpells_Add_Controls(void)
{
    int books_minus_1;
    int page;
    int books_row;
    int spell_realm;
    int top;
    int count;
    int unused_title_y[3] = { 26, 77, 128 };
    int unused_button_y[3] = { 37, 88, 139 };
    int i;
    int j;

    if (select_spells_realm == REALM_CHAOS)
        spell_realm = REALM_CHAOS;
    if (select_spells_realm == REALM_SORCERY)
        spell_realm = REALM_SORCERY;
    if (select_spells_realm == REALM_NATURE)
        spell_realm = REALM_NATURE;
    if (select_spells_realm == REALM_LIFE)
        spell_realm = REALM_LIFE;
    if (select_spells_realm == REALM_DEATH)
        spell_realm = REALM_DEATH;
    books_minus_1 = wizards[PLAYER_HUMAN].spellbooks[select_spells_realm] - 1;
    page = 0;
    count = 0;
    top = 39;
    for (i = 0; i < 30; i++)
        help_entries[i].id = -1;
    if (books_minus_1 > 0) {
        books_row = books_minus_1 - 1;
        if (start_spells_common[books_row] > 0 && start_spells_common[books_row] < 10) {
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 5; j++) {
                    ng_controls[count] = UI_Add_Region(i * 74 + 170, top + j * 7, i * 78 + 240, top + j * 7 + 5, "", -1);
                    count++;
                }
            }
            for (i = 0; i < 10; i++)
                help_entries[i + page * 10].id = spell_realm * SPELLS_PER_REALM + i + 1;
            page++;
        }
        if (start_spells_uncommon[books_row] > 0) {
            top = page * 51 + 39;
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 5; j++) {
                    ng_controls[count] = UI_Add_Region(i * 74 + 170, top + j * 7, i * 78 + 240, top + j * 7 + 5, "", -1);
                    count++;
                }
            }
            for (i = 0; i < 10; i++)
                help_entries[i + page * 10].id = spell_realm * SPELLS_PER_REALM + i + 11;
            page++;
        }
        if (start_spells_rare[books_row] > 0) {
            top = page * 51 + 39;
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 5; j++) {
                    ng_controls[count] = UI_Add_Region(i * 74 + 170, top + j * 7, i * 78 + 240, top + j * 7 + 5, "", -1);
                    count++;
                }
            }
            for (i = 0; i < 10; i++)
                help_entries[i + page * 10].id = spell_realm * SPELLS_PER_REALM + i + 21;
            page++;
        }
    }
    UI_Set_Help(help_entries, NG_HELP_SPELLS);
}

/* () redraw of the spell screen: name, books, retorts, OK, the picks left, and the spell list (the picks left and
   ng_picks_done are shown before SelectSpells_Draw_List recomputes them at the end: a frame late) */
void SelectSpells_Redraw(void)
{
    char light[5] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
    char dark[5] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    char text[10];
    int i;
    int x;

    menu_selection = UI_Get_Pressed_Control();   /* reused here: the pressed control */
    Draw_Pic_Frame(0, 0, screen_background_pic);
    Draw_Pic_Frame(181, 18, ng_panel_pic);
    Draw_Pic_Frame(196, 180, ng_button_pics[3]);
    Fill_Rect(251, 181, 282, 196, 0);
    Font_Select(4, 15);
    Font_Select_Palette(4, light);
    Print_Text_Centered(78, 120, wizards[PLAYER_HUMAN].wizard_name);
    Print_Text_Centered(77, 120, wizards[PLAYER_HUMAN].wizard_name);
    Font_Select_Palette(4, dark);
    Print_Text_Centered(77, 119, wizards[PLAYER_HUMAN].wizard_name);
    x = NG_BOOK_STRIP_X;
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i]]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_DEATH]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_CHAOS]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_NATURE]);
        x += NG_BOOK_STEP;
    }
    for (i = 0; i < wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY]; i++) {
        Draw_Pic_Frame(x, NG_BOOK_STRIP_Y, ng_book_pics[ng_book_looks[0][i] + NG_BOOK_PICS_SORCERY]);
        x += NG_BOOK_STEP;
    }
    if (ng_picks_done != 0) {
        if (menu_selection == ctl_ok)
            Draw_Pic_Frame(253, 183, ng_ok_pic);
        else
            Draw_Pic_Frame(252, 182, ng_ok_pic);
    } else {
        Draw_Pic_Frame(252, 182, ok_button_pic);
        if (menu_selection == ctl_ok)
            ng_message = NG_MSG_PICKS_LEFT;
    }
    itoa(ng_picks_left, text, 10);
    strcat(text, " picks");
    if (ng_picks_left == 0)
        ng_picks_done = 1;
    else
        ng_picks_done = 0;
    Font_Select(3, 15);
    Font_Select_Palette(3, dark);
    Print_Text_Centered(222, 186, text);
    Font_Select_Palette(3, light);
    Print_Text_Centered(221, 185, text);
    NewGame_Draw_Retort_Text();
    SelectSpells_Draw_List();
}

/* () the spell list of the realm being picked: title "Select <realm> Spells", then per group (commons when 1 .. 9
   are to pick, uncommons, rares) its heading with the count and 10 spell names, the picked ones marked; the picks
   still open add up in ng_picks_left. A pick slot past the group's count is cleared first */
void SelectSpells_Draw_List(void)
{
    int k;
    int pick;
    int books_minus_1;
    int page;
    int books_row;
    int realm;
    int top;
    int spell;
    int *picks;
    int count;
    int found;
    int title_y[3] = { 26, 77, 128 };
    int button_y[3] = { 37, 88, 139 };
    char normal[2] = { 0x19, 0x32 };
    char chosen[2] = { 0x16, 0xb2 };
    char shadow[2] = { 0x10, 0x10 };
    char num_text[4];
    char title_colors[5];
    char color;
    int picked[10];
    char rarity_names[4][10] = { "Common", "Uncommon", "Rare", "Very Rare" };
    char realm_names[5][10] = { "Nature", "Sorcery", "Chaos", "Life", "Death" };   /* REALM_* order; shadows MAGIC.C's */
    char text[30];
    int i;

    if (select_spells_realm == 2) {
        picks = start_spells[0][2];
        color = 0xc9;
        realm = 2;
    }
    if (select_spells_realm == 1) {
        picks = start_spells[0][1];
        color = 0xac;
        realm = 1;
    }
    if (select_spells_realm == 0) {
        picks = start_spells[0][0];
        color = 0xd8;
        realm = 0;
    }
    if (select_spells_realm == 3) {
        picks = start_spells[0][3];
        color = 0x0d;
        realm = 3;
    }
    if (select_spells_realm == 4) {
        picks = start_spells[0][4];
        color = 0x72;
        realm = 4;
    }
    for (i = 0; i < 5; i++)
        title_colors[i] = color;
    strcpy(text, "Select ");
    strcat(text, realm_names[realm]);
    strcat(text, " Spells");
    Font_Select_Palette(4, title_colors);
    Font_Select_Shadow(4, 15);
    Print_Text_Centered(241, 6, text);
    Draw_Pic_Frame(24, 10, portrait_pics[wizards[PLAYER_HUMAN].portrait]);
    books_minus_1 = wizards[PLAYER_HUMAN].spellbooks[select_spells_realm] - 1;
    page = 0;
    top = 39;
    ng_picks_left = 0;
    if (books_minus_1 > 0) {
        books_row = books_minus_1 - 1;
        if (start_spells_common[books_row] < 10)
            picks[start_spells_common[books_row]] = 0;
        for (i = 0; i < 10; i++) {
            picked[i] = picks[i];
            if (picked[i] == 0)
                picked[i] = -1;
        }
        if (start_spells_common[books_row] > 0 && start_spells_common[books_row] < 10) {
            ng_picks_left += start_spells_common[books_row];
            strcpy(text, rarity_names[0]);
            strcat(text, ": ");
            itoa(start_spells_common[books_row], num_text, 10);
            strcat(text, num_text);
            Font_Select_Palette(3, title_colors);
            Font_Select_Shadow(3, 15);
            Print_Text(167, title_y[page], text);
            Draw_Pic_Frame(167, button_y[page], ng_button_pics[page]);
            count = 0;
            for (i = 0; i < 2; i++) {
                for (k = 0; k < 5; k++) {
                    spell = realm * SPELLS_PER_REALM + k + i * 5 + 1;
                    Font_Select_Palette(0, shadow);
                    Print_Far_Text(i * 74 + 176, top + k * 7 + 1, p_spell_data[spell].name);
                    found = -1;
                    for (pick = 0; pick < start_spells_common[books_row]; pick++) {
                        if (picked[pick] == spell) {
                            count++;
                            found = 1;
                            picked[pick] = -1;
                            pick = start_spells_common[books_row];
                        }
                    }
                    if (found != -1) {
                        Font_Select_Palette(0, chosen);
                        Draw_Pic_Frame(i * 74 + 169, top + k * 7 + 1, select_mark_pic);
                        ng_picks_left--;
                    } else
                        Font_Select_Palette(0, normal);
                    Print_Far_Text(i * 74 + 175, top + k * 7, p_spell_data[spell].name);
                }
            }
            page++;
        }
        if (start_spells_uncommon[books_row] < 2)
            picks[start_spells_uncommon[books_row] + 10] = 0;
        for (i = 0; i < 2; i++) {
            picked[i] = picks[i + 10];
            if (picked[i] == 0)
                picked[i] = -1;
        }
        if (start_spells_uncommon[books_row] > 0) {
            ng_picks_left += start_spells_uncommon[books_row];
            top = page * 51 + 39;
            strcpy(text, rarity_names[1]);
            strcat(text, ": ");
            itoa(start_spells_uncommon[books_row], num_text, 10);
            strcat(text, num_text);
            Font_Select_Palette(3, title_colors);
            Font_Select_Shadow(3, 15);
            Print_Text(167, title_y[page], text);
            Draw_Pic_Frame(167, button_y[page], ng_button_pics[page]);
            count = 0;
            for (i = 0; i < 2; i++) {
                for (k = 0; k < 5; k++) {
                    spell = realm * SPELLS_PER_REALM + k + i * 5 + 11;
                    Font_Select_Palette(0, shadow);
                    Print_Far_Text(i * 74 + 176, top + k * 7 + 1, p_spell_data[spell].name);
                    found = -1;
                    for (pick = 0; pick < start_spells_uncommon[books_row]; pick++) {
                        if (picked[pick] == spell) {
                            count++;
                            found = 1;
                            picked[pick] = -1;
                            pick = start_spells_uncommon[books_row];
                        }
                    }
                    if (found != -1) {
                        Font_Select_Palette(0, chosen);
                        Draw_Pic_Frame(i * 74 + 169, top + k * 7 + 1, select_mark_pic);
                        ng_picks_left--;
                    } else
                        Font_Select_Palette(0, normal);
                    Print_Far_Text(i * 74 + 175, top + k * 7, p_spell_data[spell].name);
                }
            }
            page++;
        }
        if (start_spells_rare[books_row] < 1)
            picks[start_spells_rare[books_row] + 12] = 0;
        for (i = 0; i < 1; i++) {
            picked[i] = picks[i + 12];
            if (picked[i] == 0)
                picked[i] = -1;
        }
        if (start_spells_rare[books_row] > 0) {
            ng_picks_left += start_spells_rare[books_row];
            top = page * 51 + 39;
            strcpy(text, rarity_names[2]);
            strcat(text, ": ");
            itoa(start_spells_rare[books_row], num_text, 10);
            strcat(text, num_text);
            Font_Select_Palette(3, title_colors);
            Font_Select_Shadow(3, 15);
            Print_Text(167, title_y[page], text);
            Draw_Pic_Frame(167, button_y[page], ng_button_pics[page]);
            count = 0;
            for (i = 0; i < 2; i++) {
                for (k = 0; k < 5; k++) {
                    spell = realm * SPELLS_PER_REALM + k + i * 5 + 21;
                    Font_Select_Palette(0, shadow);
                    Print_Far_Text(i * 74 + 176, top + k * 7 + 1, p_spell_data[spell].name);
                    found = -1;
                    for (pick = 0; pick < start_spells_rare[books_row]; pick++) {
                        if (picked[pick] == spell) {
                            count++;
                            found = 1;
                            picked[pick] = -1;
                            pick = start_spells_rare[books_row];
                        }
                    }
                    if (found != -1) {
                        Font_Select_Palette(0, chosen);
                        Draw_Pic_Frame(i * 74 + 169, top + k * 7 + 1, select_mark_pic);
                        ng_picks_left--;
                    } else
                        Font_Select_Palette(0, normal);
                    Print_Far_Text(i * 74 + 175, top + k * 7, p_spell_data[spell].name);
                }
            }
            page++;
        }
    }
}

/* (preset) the human wizard takes a preset: portrait, books, retort, the default spell picks and the name - BUG #354: the
   pick counts are read one entry late (start_spells_common[books - 1], not [books - 2]): one common too many, and
   the full 13 at 10 books */
void Wizard_Apply_Preset(int preset)
{
    char *retorts;
    int i;
    int books;

    wizards[PLAYER_HUMAN].portrait = preset;
    wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE] = wizard_presets[preset].books_nature;
    wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY] = wizard_presets[preset].books_sorcery;
    wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS] = wizard_presets[preset].books_chaos;
    wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] = wizard_presets[preset].books_life;
    wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] = wizard_presets[preset].books_death;
    retorts = wizards[PLAYER_HUMAN].retorts;
    for (i = 0; i < 18; i++) {
        if (wizard_presets[preset].retort == i)
            retorts[i] = 1;
        else
            retorts[i] = 0;
    }
    for (i = 0; i < START_SPELL_PICKS; i++) {
        start_spells[PLAYER_HUMAN][REALM_NATURE][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_SORCERY][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_CHAOS][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_LIFE][i] = 0;
        start_spells[PLAYER_HUMAN][REALM_DEATH][i] = 0;
    }
    books = wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE];
    if (books > 1) {
        for (i = 0; i < start_spells_common[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_NATURE][i] = default_spells[REALM_NATURE][i];
        for (i = 0; i < start_spells_uncommon[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_NATURE][i + 10] = default_spells[REALM_NATURE][i + 10];
        if (start_spells_rare[books - 1] > 0)
            start_spells[PLAYER_HUMAN][REALM_NATURE][12] = default_spells[REALM_NATURE][12];
    }
    books = wizards[PLAYER_HUMAN].spellbooks[REALM_SORCERY];
    if (books > 1) {
        for (i = 0; i < start_spells_common[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_SORCERY][i] = default_spells[REALM_SORCERY][i];
        for (i = 0; i < start_spells_uncommon[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_SORCERY][i + 10] = default_spells[REALM_SORCERY][i + 10];
        if (start_spells_rare[books - 1] > 0)
            start_spells[PLAYER_HUMAN][REALM_SORCERY][12] = default_spells[REALM_SORCERY][12];
    }
    books = wizards[PLAYER_HUMAN].spellbooks[REALM_CHAOS];
    if (books > 1) {
        for (i = 0; i < start_spells_common[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_CHAOS][i] = default_spells[REALM_CHAOS][i];
        for (i = 0; i < start_spells_uncommon[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_CHAOS][i + 10] = default_spells[REALM_CHAOS][i + 10];
        if (start_spells_rare[books - 1] > 0)
            start_spells[PLAYER_HUMAN][REALM_CHAOS][12] = default_spells[REALM_CHAOS][12];
    }
    books = wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE];
    if (books > 1) {
        for (i = 0; i < start_spells_common[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_LIFE][i] = default_spells[REALM_LIFE][i];
        for (i = 0; i < start_spells_uncommon[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_LIFE][i + 10] = default_spells[REALM_LIFE][i + 10];
        if (start_spells_rare[books - 1] > 0)
            start_spells[PLAYER_HUMAN][REALM_LIFE][12] = default_spells[REALM_LIFE][12];
    }
    books = wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH];
    if (books > 1) {
        for (i = 0; i < start_spells_common[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_DEATH][i] = default_spells[REALM_DEATH][i];
        for (i = 0; i < start_spells_uncommon[books - 1]; i++)
            start_spells[PLAYER_HUMAN][REALM_DEATH][i + 10] = default_spells[REALM_DEATH][i + 10];
        if (start_spells_rare[books - 1] > 0)
            start_spells[PLAYER_HUMAN][REALM_DEATH][12] = default_spells[REALM_DEATH][12];
    }
    strcpy(wizards[PLAYER_HUMAN].wizard_name, wizard_presets[preset].name);
}

/* () fade the palette to black in 4 steps */
void Palette_Fade_To_Black(void)
{
    int percent;

    for (percent = 25; percent < 101; percent += 25) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () fade the palette back from black in 4 steps */
void Palette_Fade_From_Black(void)
{
    int percent;

    for (percent = 75; percent >= 0; percent -= 25) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () the help entries of the screen (HLPENTRY.LBX 26, 4 entries) */
void Help_Load_LoadGame(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 26, help_entries, 0, NG_HELP_LOADGAME, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, NG_HELP_LOADGAME);
}

/* () the help entries of the screen (HLPENTRY.LBX 29, 6 entries) */
void Help_Load_NewGame_Options(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 29, help_entries, 0, NG_HELP_OPTIONS, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, NG_HELP_OPTIONS);
}

/* () the help entries of the screen (HLPENTRY.LBX 30, 1 entry) */
void Help_Load_Select_Banner(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 30, help_entries, 0, NG_HELP_BANNER, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, NG_HELP_BANNER);
}

/* () the help entries of the screen (HLPENTRY.LBX 31, 1 entry) */
void Help_Load_Select_Portrait(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 31, help_entries, 0, NG_HELP_PORTRAIT, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, NG_HELP_PORTRAIT);
}

/* (custom allowed) the wizard screen's help (HLPENTRY.LBX 32, 15 entries); with Custom shown its entry gets help 673 */
void Help_Load_Select_Wizard(int custom)
{
    LBX_Load_Records_Near("hlpentry.lbx", 32, help_entries, 0, NG_HELP_WIZARD, sizeof(struct HelpEntry));
    if (custom != 0)
        help_entries[14].id = HELP_CUSTOM;
    UI_Set_Help(help_entries, NG_HELP_WIZARD);
}

/* () the help entries of the screen (HLPENTRY.LBX 33, 25 entries) */
void Help_Load_Custom_Picks(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 33, help_entries, 0, NG_HELP_CUSTOM_PICKS, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, NG_HELP_CUSTOM_PICKS);
}

/* () the help entries of the screen (HLPENTRY.LBX 35, 14 entries) */
void Help_Load_Select_Race(void)
{
    LBX_Load_Records_Near("hlpentry.lbx", 35, help_entries, 0, NG_HELP_RACE, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, NG_HELP_RACE);
}

/* (count, total, text) counts one more list item and appends its separator: " and " before the last, ", " between */
void Text_List_Separator(int *count, int total, char *text)
{
    *count = *count + 1;
    if (*count == total) {
        if (*count > 1)
            strcat(text, " and ");
    } else if (*count > 1)
        strcat(text, ", ");
}

/* () empty hooks, named for their WIZARDS call sites in NOTIFY.C's window zoom (ReMoM has no names for them) */
void Window_Zoom_Start_Nop(void)
{
}

/* () empty (MAGIC's copy of the window-zoom hook) */
void Window_Zoom_Skipped_Nop(void)
{
}

/* () show the frame: the mouse cursor redrawn */
void Screen_Present(void)
{
    UI_Present_Frame();
}
