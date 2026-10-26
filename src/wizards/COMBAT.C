/* COMBAT.C - WIZARDS.EXE overlay code segment 75eb: Combat_Main, the tactical battle when the human is involved -
   the set-up (sides, battlefield, armies, auras, node, sound effects and music), the turn order (each combat turn
   starts with Combat_Turn_Start, then the defender's side moves: an AI defender at once, a human defender through the
   input loop, then the attacker), the human input loop (Spell / Wait / Done / Info / Auto / Flee buttons, map clicks
   as unit orders, right clicks for the unit view, the enchantment icon pages) and the hand-over to Combat_End.
   This module's data (DS:50ac): combat_enchant_icon_table and its literals; BSS (DS:c406..c5ba; c41e..c5ba owner not fixed by the EXE, see below).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 75eb src/wizards/COMBAT.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Combat_Main(int attacker_player, int defender_player, int *units, int n, int x, int y, int plane, int *n_items,
                int *items);

#include "GAME.H"

struct EnchIcon combat_enchant_icon_table[15] = {
    {0, 600}, {1, 601}, {2, 602}, {3, 603}, {4, 604}, {5, 605}, {6, 606}, {7, 607}, {8, 608}, {9, 609}, {10, 610},
    {11, 611}, {12, 632}, {13, 767}, {14, 797}
};                                              /* DS:50ac  {icon, help} per combat enchantment pair, read only by Combat_Build_Enchant_Icons (7eb0); its place before this module's literals (UNITFIG.C's data ends at 50ab with its literals) makes it this module's data */

/* DS:c41e..c5ba: the shared combat state. Its owner is not fixed by the EXE: the bytes allow this module, COMBMOVE.C, MAPVIS.C,
   ENDGAME.C (the next modules in link order, none with BSS of its own) or a code-less data module; the names are
   combat state used across the combat modules, so it is defined here, after the module's own BSS (b88) */
unsigned combat_small_attack_icons[9];          /* DS:c5a8  COMPIX 29-37 */
unsigned combat_info_pics[14];                  /* DS:c58c  the Info screen's pictures */
int combat_hover_unit;                          /* DS:c58a */
int battle_unit_count;                          /* DS:c588 */
int combat_attacker_player;                     /* DS:c586 */
int combat_defender_player;                     /* DS:c584 */
int combat_active_unit;                         /* DS:c582 */
                                                /* also: the city's fate after a battle - CITY_FATE_KEPT,
                                                   _RAIDED, _RUINS (COMBEND.C) */
unsigned combat_spell_button_pic;               /* DS:c580  COMPIX 1 */
unsigned combat_wait_button_pic;                /* DS:c57e  COMPIX 2 */
unsigned combat_done_button_pic;                /* DS:c57c  COMPIX 3 */
unsigned combat_auto_button_pic;                /* DS:c57a  COMPIX 4 */
unsigned combat_flee_button_pic;                /* DS:c578  COMPIX 21 */
unsigned combat_use_button_pic;                 /* DS:c576  COMPIX 20 */
int combat_unused_c574;                         /* DS:c574  no reader or writer found (never referenced) */
unsigned combat_sprite_pics[COMBAT_SPRITE_SLOTS];                /* DS:c550  18 sprite buffers (Combat_Load_UI) */
char *combat_click_map[22];                     /* DS:c524  per grid row: the battle unit at each square, -1 */
int *p_combat_cursor_region;                    /* DS:c522 */
/* p_combat_cursor_region: one struct MouseRegion (12 bytes, COMBINFO.C) written by index, MREG_* */
int combat_battlefield_kind;                       /* DS:c520 */
/* combat_battlefield_kind: the BATTLEFIELD_* value of Combat_Terrain_Kind, battlefield_kind in Combat_Strategic and
   Combat_Start_Auras (BFIELD.C's 'kind' is a different value: a BF_TERRAIN_*) */
int combat_end_turn_pending;                      /* DS:c51e */
int combat_turn_count;                          /* DS:c51c */
unsigned combat_enchant_pics[15];               /* DS:c4fe  COMPIX 5-15, 41, 60, 79, 80 */
struct PicHelp combat_att_icons[15];            /* DS:c4c2 */
struct PicHelp combat_def_icons[15];            /* DS:c486 */
int combat_def_icon_count;                      /* DS:c484 */
int combat_att_icon_count;                      /* DS:c482 */
int combat_loc_x;                               /* DS:c480  overland tile being fought over */
int combat_loc_y;                               /* DS:c47e */
int combat_loc_plane;                           /* DS:c47c */
int combat_att_icon_ctls[15];                   /* DS:c45e */
int combat_def_icon_ctls[15];                   /* DS:c440 */
int combat_enemy_wizard;                        /* DS:c43e */
int combat_att_icon_first;                      /* DS:c43c */
int combat_def_icon_first;                      /* DS:c43a */
int combat_wizard_can_cast;                     /* DS:c438 */
int combat_human_side;                          /* DS:c436 */
int combat_ai_side;                             /* DS:c434 */
int combat_auto;                                /* DS:c432 */
int combat_ai_moved;                            /* DS:c430 */
unsigned combat_spell_lock_pic;                 /* DS:c42e  COMPIX 23 */
unsigned combat_wait_lock_pic;                  /* DS:c42c  COMPIX 24 */
unsigned combat_done_lock_pic;                  /* DS:c42a  COMPIX 28 */
unsigned combat_auto_lock_pic;                  /* DS:c428  COMPIX 26 */
unsigned combat_flee_lock_pic;                  /* DS:c426  COMPIX 27 */
unsigned combat_use_lock_pic;                   /* DS:c424  COMPIX 25 */
unsigned combat_cancel_button_pic;              /* DS:c422  COMPIX 22 */
int combat_attacker_sees_invis;                /* DS:c420  1: an attacking unit sees invisible - defenders show */
int combat_defender_sees_invis;                /* DS:c41e  1: a defending unit sees invisible - attackers show */
unsigned combat_music_data;                     /* DS:c41c  the battle music (MUSIC.LBX) */
unsigned combat_silence_sfx;                    /* DS:c41a  SFX_SILENCE: played to cut a sound off */
int city_damage_counter;                        /* DS:c418  +1 per attacker in the city area a turn, +5 per vortex move in */
                                                /* COMBEND: population lost % = 2 x it, buildings lost % = it */
char combat_city_name[14];                      /* DS:c40a  the attacked city's name */
int combat_city_defended;                       /* DS:c408  1: a city held by a wizard is attacked */
int combat_all_units_immobile;                            /* DS:c406  1 from each turn start; 0 after any button or dialog, or once a human unit */
                                                /* with moves is active (COMBINFO.C). 1 with a human unit active and auto combat off: no unit marker, no
                                                   attack cursor (COMBTURN.C), and on the human's turn "All units are immobilized.
                                                   Select an action." (COMBDRAW.C); 0 lets a pending end of turn run.
                                                   ReMoM _human_handle_immobile; clarified using ReMoM repo */

extern struct BattleUnit far *p_unit_stats_buf; /* DS:9226 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern int combat_ai_fled;                      /* DS:924a */
extern int combat_active_marker_on;             /* DS:927c */
extern int combat_target_y;                     /* DS:927e */
extern int combat_target_x;                     /* DS:9280 */
extern int combat_cursor_mode;                  /* DS:9282 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286  the city / lair attacked */
extern unsigned combat_border_pic;              /* DS:973c */
extern int present_mode;                        /* DS:9c8e */
extern unsigned p_world_block;                /* DS:9d20 */
extern int combat_human_turn;                   /* DS:c79a */
extern int combat_redraw_needed;                /* DS:c79c */
extern int combat_info_count;                   /* DS:c7c6 */
extern int combat_turns_since_ai_acted;                   /* DS:c8b0 */
extern int combat_defender_holds_city;          /* DS:c8b2 */

void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Clear_Controls(void);
void Palette_Fade_To_Black(void);
void Set_Draw_Page(void);
void Fill_Rect(int x1, int y1, int x2, int y2, int color);
void UI_Present_Frame(void);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Battlefield_Build(void);
void Combat_Load_UI(void);
void Load_Combat_Graphics(void);
void Battlefield_Generate(int x, int y, int plane);
int Combat_Load_Armies(int count, int *units);
void Combat_Draw_Screen(void);
int Combat_Terrain_Kind(int x, int y, int plane, int in_city);
void Combat_Build_Enchant_Icons(void);
void Combat_Find_Node(void);
int Combat_Info_Count(void);
void Combat_Start_Auras(int battlefield_kind);
void Combat_Tmp_Save(void);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Random(int n);
int Sound_Play(unsigned sound);
void Palette_Cycle_Reset(void);
void Pic_Apply_Palette(unsigned pic);
void Palette_Upload(void);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
void Palette_Mark_Range(int first, int last);
void Palette_Unmark_Range(int first, int last);
void Remap_Build_All(void);
void Combat_Set_Active_Unit(int bu);
void Screen_Present(void);
void Combat_Turn_Start(void);
void Combat_Side_Turn(int player);
int Combat_Check_Winner(void);
void Combat_Select_Next_Unit(int player);
void Combat_Build_Click_Map(void);
void UI_Input_Delay(int delay);
void UI_Clear_Help(void);
void Help_Load_Combat(void);
void Timer_Mark(void);
int UI_Add_Region(int x1, int y1, int x2, int y2, char *hotkey, int help);
int UI_Get_Input(void);
void Play_Click_Sound(void);
void Combat_End_Human_Turn(void);
void Combat_Auto_Off(void);
int Unit_Has_Combat_Ability(int bu);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void Combat_Add_Enchant_Icon_Controls(void);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int UI_Add_Hotkey(char *key);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
void Confused_Unit_Random_Move(int unit);
int Confirm_Box(char *text);
int Combat_Screen_To_Grid_X(int sx, int sy);
int Combat_Screen_To_Grid_Y(int sx, int sy);
void Combat_Unit_Order(int unit, int gx, int gy);
void Unit_Update_Visibility(int bu);
int Unit_Combat_Moves(int bu);
int Mem_Copy_Far();                                /* unprototyped (see HELPSCR.C) */
void Combat_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit);
void CombatUI_Alloc_Buffers(void);
void Battlefield_Draw_Terrain(void);
void Combat_Who_Will_Cast(int unit);
int Combat_Spell_Action();                      /* no prototype here: called with 4 arguments (1 elsewhere) */
void Combat_Cast_Refusal_Msg(int reason);
void Screen_Combat_Info(void);
void Combat_Update_Mouse_Cursor(void);
void Wait_Ticks(int ticks);
void Screen_Copy_Front(void);
void Screen_Save_AC00(void);
void Screen_Restore_AC00(void);
void Screen_Save_A800(void);
int Sound_Stop(void);
void Combat_Tmp_Restore(void);
void Load_Overland_Screen_Graphics(void);
void Music_Play_Current(void);
void Combat_End(int winner, int *n_items, int *items, int end_code);

/* (attacker player, defender player, units, n, x, y, plane, &n_items, items) -> 1 when the attacker won: the
   tactical battle. A confused unit (combat_flags 2, confusion_result 2) changes sides when its turn ends; Flee
   ('Do you wish to flee?') gives every human unit ACTION_FLEE (the active / set-aside ones also status FLED)
   and the battle to the AI side. End codes to
   Combat_End: 1 the human won, 2 lost, 3 fled, 4 turn limit (more than 50 turns), 5 the AI fled
   Numbered: BUG #209, #243, #244, #349 (BUGS_AND_QUIRKS.txt) */
int Combat_Main(int attacker_player, int defender_player, int *units, int n, int x, int y, int plane, int *n_items,
                int *items)
{
    int input;
    int battle_over;
    int skip_redraw;                            /* 1: no Combat_Draw_Screen this pass; read before its first assignment on
                                                   the first pass (original) */
    int esc_control;
    int mouse_x;
    int mouse_y;
    int grid_control;
    int gx;
    int gy;
    int spell_button;
    int wait_button;
    int done_button;
    int auto_button;
    int flee_button;
    int info_button;
    int hotkey;
    int panel_control;
    unsigned long saved_ench;
    int saved_moves;
    int spell_result;
    int auto_control;
    int winner;
    int fled;
    int n_loaded;                               /* Combat_Load_Armies: the first side's units (battle_unit_count - n_loaded = the AI side's first) */
    int end_code;
    register int i;
    register int bu;

    combat_turns_since_ai_acted = 0;
    combat_city_defended = 0;
    if (combat_site == COMBAT_AT_CITY && defender_player != PLAYER_NEUTRAL)
        combat_city_defended = 1;
    combat_ai_fled = 0;
    fled = 0;
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Palette_Fade_To_Black();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    present_mode = PRESENT_NORMAL;
    if (combat_site == COMBAT_AT_CITY)
        far_strcpy(combat_city_name, 0, p_cities[combat_site_index].name);
    Battlefield_Build();
    Combat_Load_UI();
    combat_loc_x = x;
    combat_loc_y = y;
    combat_loc_plane = plane;
    combat_attacker_player = attacker_player;
    combat_defender_player = defender_player;
    Load_Combat_Graphics();
    Battlefield_Generate(x, y, plane);
    n_loaded = Combat_Load_Armies(n, units);
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
    combat_att_icon_first = 0;
    combat_def_icon_first = 0;
    combat_battlefield_kind = Combat_Terrain_Kind(x, y, plane, 0);
    Combat_Build_Enchant_Icons();
    Combat_Find_Node();
    combat_end_turn_pending = 0;
    combat_turn_count = 0;
    combat_wizard_can_cast = 1;
    combat_info_count = Combat_Info_Count();
    Combat_Start_Auras(combat_battlefield_kind);
    Combat_Tmp_Save();
    combat_silence_sfx = LBX_Reload("SOUNDFX", SFX_SILENCE, p_world_block);
    if (combat_enemy_wizard != PLAYER_NEUTRAL)  /* Random(2) is 1..2: the tracks are BASE + 2 x portrait + 1 / + 2 */
        combat_music_data = LBX_Reload_Next("MUSIC", wizards[combat_enemy_wizard].portrait * 2 + Random(2) + MUSIC_BATTLE_BASE,
                                            p_world_block);
    else {
        i = Random(2) + MUSIC_BATTLE_NEUTRAL;
        combat_music_data = LBX_Reload_Next("MUSIC", i, p_world_block);
    }
    if (game_config.music == 1)
        Sound_Play(combat_music_data);
    combat_all_units_immobile = 0;
    combat_defender_holds_city = 1;
    combat_hover_unit = -1;
    Palette_Cycle_Reset();
    Pic_Apply_Palette(combat_border_pic);
    Palette_Upload();
    Palette_Cycle_Color(PAL_CYCLE_COLOR, 0, 0, 0, 0x37, 0, 0, 8);
    Palette_Mark_Range(0, 0xf3);
    Remap_Build_All();
    Combat_Set_Active_Unit(combat_attacker_player == current_player ? 0 : battle_unit_count - n_loaded);
    combat_active_marker_on = 0;
    combat_cursor_mode = 0;
    combat_redraw_needed = 1;
    Combat_Draw_Screen();
    Screen_Present();
    city_damage_counter = 0;
    battle_over = 0;
    combat_human_turn = 1;
    combat_auto = 0;
    Combat_Turn_Start();
    combat_all_units_immobile = 0;
    Combat_Set_Active_Unit(combat_attacker_player == current_player ? 0 : battle_unit_count - n_loaded);
    combat_active_marker_on = 0;
    combat_cursor_mode = 0;
    combat_redraw_needed = 1;
    Combat_Draw_Screen();
    Screen_Present();
    combat_ai_moved = 0;
    if (combat_defender_player == combat_ai_side) {
        Combat_Side_Turn(combat_defender_player);
        Combat_Turn_Start();
        combat_ai_moved = 1;
    }
    winner = Combat_Check_Winner();
    if (winner != -1)
        battle_over = -1;
    combat_end_turn_pending = 0;
    if (combat_attacker_player == current_player)
        Combat_Select_Next_Unit(current_player);
    else
        Combat_Set_Active_Unit(battle_unit_count - n_loaded);
    Combat_Draw_Screen();
    Screen_Present();
    Combat_Build_Click_Map();
    UI_Input_Delay(3);
    Combat_Build_Enchant_Icons();
    UI_Clear_Help();
    Help_Load_Combat();
    combat_redraw_needed = 1;
    while (battle_over == 0) {
        UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
        Timer_Mark();
        UI_Clear_Controls();
        if (combat_auto == 1) {
            auto_control = UI_Add_Region(0, 0, 319, 199, "\033", -1);
            input = UI_Get_Input();
            if (input == auto_control) {
                Play_Click_Sound();
                combat_auto = 0;
                combat_ai_moved = 0;
                combat_redraw_needed = 1;
                Combat_End_Human_Turn();
                Combat_Auto_Off();
                combat_redraw_needed = 1;
                combat_all_units_immobile = 0;
            } else
                Combat_End_Human_Turn();
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = -1;
            }
            continue;
        }
        if ((p_battle_units[combat_active_unit].moves > 0 &&
             (p_battle_units[combat_active_unit].spell_points >= UNIT_MIN_CAST_MANA || Unit_Has_Combat_Ability(combat_active_unit) == 1 ||
              p_battle_units[combat_active_unit].item_charges > 0)) ||
            combat_wizard_can_cast == 1)
            spell_button = UI_Add_Button(144, 168, "", combat_spell_button_pic, "S", -1);
        else
            spell_button = UI_NO_CONTROL;
        wait_button = UI_Add_Button(170, 168, "", combat_wait_button_pic, "W", -1);
        done_button = UI_Add_Button(170, 188, "", combat_done_button_pic, "D", -1);
        if (combat_info_count > 0)
            info_button = UI_Add_Button(144, 178, "", combat_use_button_pic, "U", -1);   /* the Info button: COMPIX 20, named "use button" in the LBX */
        else
            info_button = UI_NO_CONTROL;
        auto_button = UI_Add_Button(170, 178, "", combat_auto_button_pic, "A", -1);
        flee_button = UI_Add_Button(144, 188, "", combat_flee_button_pic, "F", -1);
        panel_control = UI_Add_Region(83, 173, 116, 198, "", -1);
        Combat_Add_Enchant_Icon_Controls();
        esc_control = UI_Add_Region(0, 164, 319, 199, "\033", -1);
        grid_control = UI_Add_Grid_Control(0, 0, 1, 1, 319, 164, &mouse_x, &mouse_y, -1);
        hotkey = UI_Add_Hotkey(" ");
        input = UI_Get_Input();
        if (input == auto_button) {
            Play_Click_Sound();
            Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
            combat_active_marker_on = 0;
            combat_cursor_mode = 0;
            combat_auto = 1 - combat_auto;
            combat_end_turn_pending = 0;
            UI_Clear_Controls();
            input = 0;
            Combat_Side_Turn(combat_human_side);
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        if ((p_battle_units[combat_active_unit].moves < 1 && p_battle_units[combat_active_unit].action_state != ACTION_DONE) ||
            p_battle_units[combat_active_unit].status > 0) {   /* any status but ACTIVE: out of the battle */
            p_battle_units[combat_active_unit].moving = 0;
            p_battle_units[combat_active_unit].moves = 0;
            p_battle_units[combat_active_unit].action_state = ACTION_DONE;
            if ((p_battle_units[combat_active_unit].combat_flags & COMBAT_FLAG_CONFUSION) &&
                p_battle_units[combat_active_unit].confusion_result == CONFUSION_CHANGED_SIDE) {
                if (p_battle_units[combat_active_unit].owner == combat_attacker_player)
                    p_battle_units[combat_active_unit].owner = combat_defender_player;
                else
                    p_battle_units[combat_active_unit].owner = combat_attacker_player;
            }
            Combat_Select_Next_Unit(current_player);
            Combat_Build_Click_Map();
            input = -1;
            skip_redraw = 1;
            combat_redraw_needed = 1;
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        if (p_battle_units[combat_active_unit].confusion_result == CONFUSION_RANDOM_MOVE && p_battle_units[combat_active_unit].moves > 0) {
            Confused_Unit_Random_Move(combat_active_unit);
            Combat_Select_Next_Unit(current_player);
            Combat_Build_Click_Map();
            input = -1;
            skip_redraw = 1;
            combat_redraw_needed = 1;
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        for (i = 0; i < combat_att_icon_count && i < COMBAT_ICONS_PER_PAGE && combat_att_icon_count > COMBAT_ICONS_PER_PAGE; i++) {
            if (combat_att_icon_ctls[i] == input) {
                Play_Click_Sound();
                combat_att_icon_first += COMBAT_ICONS_PER_PAGE;   /* pages 0 / 4 / 8: QUIRK #437 - icons 12 .. 14
                                                   of a side with 13+ enchantments are never shown */
                if (combat_att_icon_first > 8)
                    combat_att_icon_first = 0;
                combat_redraw_needed = 1;
                break;
            }
        }
        for (i = 0; i < combat_def_icon_count && i < COMBAT_ICONS_PER_PAGE && combat_def_icon_count > COMBAT_ICONS_PER_PAGE; i++) {
            if (combat_def_icon_ctls[i] == input) {
                Play_Click_Sound();
                combat_def_icon_first += COMBAT_ICONS_PER_PAGE;
                if (combat_def_icon_first > 8)
                    combat_def_icon_first = 0;
                combat_redraw_needed = 1;
                break;
            }
        }
        if (input == flee_button) {
            combat_all_units_immobile = 0;
            Play_Click_Sound();
            Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
            if (Confirm_Box("Do you wish to flee?") == 1) {
                for (i = 0; i < battle_unit_count; i++) {
                    if (p_battle_units[i].owner == combat_human_side) {
                        p_battle_units[i].action_state = ACTION_FLEE;
                        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE || p_battle_units[i].status == BATTLE_UNIT_SET_ASIDE)
                            p_battle_units[i].status = BATTLE_UNIT_FLED;
                    }
                }
                winner = combat_ai_side;
                fled = 1;
                battle_over = -1;
            }
            Combat_Build_Click_Map();
            skip_redraw = 1;
        }
        if (input == grid_control) {
            combat_all_units_immobile = 0;
            combat_target_x = Combat_Screen_To_Grid_X(mouse_x + 4, mouse_y + 4);
            combat_target_y = Combat_Screen_To_Grid_Y(mouse_x + 4, mouse_y + 4);
            Combat_Unit_Order(combat_active_unit, combat_target_x, combat_target_y);
            for (i = 0; i < battle_unit_count; i++)
                Unit_Update_Visibility(i);
            Combat_Build_Click_Map();
            combat_redraw_needed = 1;
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        if (-grid_control == input) {           /* UI_Get_Input: -id = a right click on it */
            gx = Combat_Screen_To_Grid_X(mouse_x + 4, mouse_y + 4);
            gy = Combat_Screen_To_Grid_Y(mouse_x + 4, mouse_y + 4);
            bu = combat_click_map[gy][gx];
            if (bu >= 0) {
                if (p_battle_units[bu].status == BATTLE_UNIT_ACTIVE) {
                    if (p_battle_units[bu].owner == current_player) {
                        if (combat_active_unit == bu) {
                            Play_Click_Sound();
                            UI_Clear_Help();
                            Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                            saved_moves = p_battle_units[bu].moves;
                            p_battle_units[bu].moves = Unit_Combat_Moves(bu);
                            saved_ench = p_units[p_battle_units[bu].unit_index].enchantments;
                            p_units[p_battle_units[bu].unit_index].enchantments =
                                p_battle_units[bu].combat_enchantments | p_units[p_battle_units[bu].unit_index].enchantments;
                            Mem_Copy_Far(p_unit_stats_buf, &p_battle_units[bu], sizeof(struct BattleUnit));
                            Combat_Unit_View(61, 6, 89, 174, 117, 194, UNIT_VIEW_COMBAT, p_battle_units[bu].unit_index);   /* the window (61, 6) */
                            p_battle_units[bu].moves = saved_moves;
                            p_units[p_battle_units[bu].unit_index].enchantments = saved_ench;
                            CombatUI_Alloc_Buffers();
                            Combat_Build_Click_Map();
                            UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
                            Combat_Build_Enchant_Icons();
                            Battlefield_Draw_Terrain();
                            UI_Clear_Help();
                            Help_Load_Combat();
                            combat_redraw_needed = 1;
                        } else {
                            Combat_Set_Active_Unit(bu);
                            Combat_Build_Click_Map();
                            combat_redraw_needed = 1;
                        }
                    } else {
                        Play_Click_Sound();
                        UI_Clear_Help();
                        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                        saved_moves = p_battle_units[bu].moves;
                        p_battle_units[bu].moves = Unit_Combat_Moves(bu);
                        saved_ench = p_units[p_battle_units[bu].unit_index].enchantments;
                        p_units[p_battle_units[bu].unit_index].enchantments =
                            p_battle_units[bu].combat_enchantments | p_units[p_battle_units[bu].unit_index].enchantments;
                        Mem_Copy_Far(p_unit_stats_buf, &p_battle_units[bu], sizeof(struct BattleUnit));
                        Combat_Unit_View(61, 6, 89, 174, 117, 194, UNIT_VIEW_COMBAT, p_battle_units[bu].unit_index);   /* the window (61, 6) */
                        p_battle_units[bu].moves = saved_moves;
                        p_units[p_battle_units[bu].unit_index].enchantments = saved_ench;
                        CombatUI_Alloc_Buffers();
                        Combat_Build_Click_Map();
                        UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
                        Combat_Build_Enchant_Icons();
                        Battlefield_Draw_Terrain();
                        UI_Clear_Help();
                        Help_Load_Combat();
                        combat_redraw_needed = 1;
                    }
                }
            }
        }
        if (input == spell_button) {
            Play_Click_Sound();
            skip_redraw = 1;
            UI_Clear_Help();
            if (p_battle_units[combat_active_unit].moves > 0 &&
                (p_battle_units[combat_active_unit].spell_points >= UNIT_MIN_CAST_MANA || Unit_Has_Combat_Ability(combat_active_unit) == 1 ||
                 p_battle_units[combat_active_unit].item_charges > 0))
                Combat_Who_Will_Cast(combat_active_unit);
            else if (combat_wizard_can_cast == 1) {
                spell_result = Combat_Spell_Action(current_player + CASTER_WIZARD, combat_loc_x, combat_loc_y, combat_loc_plane);
                switch (spell_result) {
                case CAST_RESULT_UNABLE:
                    Combat_Cast_Refusal_Msg(CAST_REFUSED_UNABLE);
                    break;
                case CAST_RESULT_NONE:
                    break;
                case CAST_RESULT_CAST:
                    combat_wizard_can_cast = 0;
                    combat_all_units_immobile = 0;
                    break;
                }
            } else
                Combat_Cast_Refusal_Msg(CAST_REFUSED_ONCE_PER_TURN);
            skip_redraw = 1;
            UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
            Combat_Build_Click_Map();
            Combat_Build_Enchant_Icons();
            Battlefield_Draw_Terrain();
            UI_Clear_Help();
            Help_Load_Combat();
            combat_redraw_needed = 1;
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        if (input == wait_button) {
            combat_all_units_immobile = 0;
            Play_Click_Sound();
            p_battle_units[combat_active_unit].action_state = ACTION_WAIT;
            Combat_Select_Next_Unit(current_player);
            Combat_Build_Click_Map();
            combat_redraw_needed = 1;
        }
        if (input == info_button) {
            combat_all_units_immobile = 0;
            Play_Click_Sound();
            UI_Clear_Help();
            Screen_Combat_Info();
            CombatUI_Alloc_Buffers();
            UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
            Combat_Build_Click_Map();
            Combat_Build_Enchant_Icons();
            Battlefield_Draw_Terrain();
            UI_Clear_Help();
            Help_Load_Combat();
            combat_redraw_needed = 1;
        }
        if (input == done_button || input == hotkey) {
            combat_all_units_immobile = 0;
            Play_Click_Sound();
            p_battle_units[combat_active_unit].action_state = ACTION_DONE;
            p_battle_units[combat_active_unit].moves = 0;
            Combat_Select_Next_Unit(current_player);
            Combat_Build_Click_Map();
            if (combat_end_turn_pending == 1)
                skip_redraw = 1;
            combat_redraw_needed = 1;
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        if (-panel_control == input) {          /* a right click */
            Play_Click_Sound();
            UI_Clear_Help();
            saved_moves = p_battle_units[combat_active_unit].moves;
            p_battle_units[combat_active_unit].moves = Unit_Combat_Moves(combat_active_unit);
            saved_ench = p_units[p_battle_units[combat_active_unit].unit_index].enchantments;
            p_units[p_battle_units[combat_active_unit].unit_index].enchantments =
                p_battle_units[combat_active_unit].combat_enchantments |
                p_units[p_battle_units[combat_active_unit].unit_index].enchantments;
            Mem_Copy_Far(p_unit_stats_buf, &p_battle_units[combat_active_unit], sizeof(struct BattleUnit));
            Combat_Unit_View(61, 6, 89, 174, 117, 194, UNIT_VIEW_COMBAT, p_battle_units[combat_active_unit].unit_index);   /* the window (61, 6) */
            p_battle_units[combat_active_unit].moves = saved_moves;
            p_units[p_battle_units[combat_active_unit].unit_index].enchantments = saved_ench;
            CombatUI_Alloc_Buffers();
            Combat_Build_Click_Map();
            UI_Set_Redraw_Fn(Combat_Draw_Screen, 1);
            Battlefield_Draw_Terrain();
            Combat_Build_Enchant_Icons();
            UI_Clear_Help();
            Help_Load_Combat();
            combat_redraw_needed = 1;
        }
        if (p_battle_units[combat_active_unit].owner != combat_human_side) {
            combat_end_turn_pending = 1;
            combat_all_units_immobile = 0;
        }
        if (battle_over == 0 && combat_end_turn_pending == 1 && combat_all_units_immobile == 0) {
            combat_end_turn_pending = 0;
            Combat_End_Human_Turn();
            Combat_Select_Next_Unit(current_player);
            Combat_Build_Click_Map();
            input = -1;
            skip_redraw = 1;
            combat_redraw_needed = 1;
            winner = Combat_Check_Winner();
            if (winner != -1) {
                battle_over = -1;
                input = 0;
            }
        }
        if (battle_over == 0 && skip_redraw == 0) {
            Combat_Draw_Screen();
            Combat_Update_Mouse_Cursor();
            Screen_Present();
            Wait_Ticks(1);
        }
        skip_redraw = 0;
    }
    UI_Clear_Help();
    Palette_Unmark_Range(0, 0xff);
    Palette_Mark_Range(0, 0xdf);
    Remap_Build_All();
    Palette_Cycle_Reset();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Screen_Copy_Front();
    Screen_Save_AC00();
    if (combat_ai_fled == 1)
        end_code = COMBAT_RESULT_ENEMY_FLED;
    else if (winner == current_player)
        end_code = COMBAT_RESULT_WON;
    else if (fled == 1)
        end_code = COMBAT_RESULT_RETREATED;
    else if (combat_turn_count > COMBAT_TURN_LIMIT)
        end_code = COMBAT_RESULT_EXHAUSTED;
    else
        end_code = COMBAT_RESULT_LOST;
    Screen_Restore_AC00();
    Screen_Save_A800();
    Sound_Stop();
    Combat_Tmp_Restore();
    Load_Overland_Screen_Graphics();
    Timer_Mark();
    Wait_Ticks(1);
    Music_Play_Current();
    Combat_End(winner, n_items, items, end_code);
    if (winner == combat_attacker_player)
        return 1;
    else
        return 0;
}
