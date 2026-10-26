/* OVERLAND.C - WIZARDS.EXE overlay code segment 4fb6 (stub 240f): the overland main screen - the loop (hotkeys F1-F9 info,
   F10 quick save, the numpad moves, C centre, Q / U deselect, the debug Alt keys - all gated by
   Check_Release_Version but Alt+P, which re-randomises every AI personality in the release build too), the top-bar buttons (Game, Spells,
   Armies, Cities, Magic, Info, Plane), the order buttons (Done, Patrol, Wait, Build / Purify / Meld), the unit slots,
   left click on the map (move, Go-To preview, Earth Gate), right click (select a stack, the stack / city popups,
   scroll), the minimap, the end of turn (with the food warning) and the debug autoplay; the click regions, the
   order and top-bar buttons, Map_Redraw and a numpad step of the selected stack.
   This module's data (DS:2e10): three words, then the literals (DS:2e16..2f83);
   BSS (DS:c03e..c089): the screen's control ids and stack-panel globals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 4fb6 src/wizards/OVERLAND.C */
#include <string.h>

/* the module's functions in their original declaration order */
void Screen_Overland_Map(void);
void Overland_Add_Controls(void);
void Overland_Add_Order_Buttons(void);
void Overland_Draw_Order_Buttons(void);
void Overland_Add_TopBar_Buttons(void);
void Overland_Draw_TopBar(void);
void Overland_Add_Stack_Icon_Controls(void);
void Map_Redraw(void);
void Overland_Screen_Reset(void);
void Stack_Key_Move_Dir(int dir);

#include "GAME.H"

/* Stack_Key_Move_Dir's directions: the numeric keypad's layout */
#define KEYDIR_SW 1
#define KEYDIR_S 2
#define KEYDIR_SE 3
#define KEYDIR_W 4
#define KEYDIR_E 6
#define KEYDIR_NW 7
#define KEYDIR_N 8
#define KEYDIR_NE 9

int debug_strategic_toggle = 0;                 /* DS:2e10  debug Alt+K flips it with Strategic Combat; never read */
int debug_autoplay = 0;                         /* DS:2e12  debug Alt+A: end the turn on the 8th idle loop (ticks -1 .. 6) */
int start_city_named = 0;                       /* DS:2e14  1 once the turn-0 "name your starting city" dialog ran */

int overland_dirty_never_read;                       /* DS:c088  set to 1, never read */
unsigned p_minimap_pic;                         /* DS:c086  the minimap picture */
int stack_panel_x;                              /* DS:c084 */
int stack_panel_y;                              /* DS:c082 */
int overland_unused_c080;                       /* DS:c080  never referenced */
int topbar_game_control;                        /* DS:c07e */
int topbar_spells_control;                      /* DS:c07c */
int topbar_armies_control;                      /* DS:c07a */
int topbar_cities_control;                      /* DS:c078 */
int topbar_magic_control;                       /* DS:c076 */
int topbar_info_control;                        /* DS:c074 */
int topbar_plane_control;                       /* DS:c072 */
int order_special_control;                      /* DS:c070 */
int order_patrol_control;                       /* DS:c06e */
int order_wait_control;                         /* DS:c06c */
int order_done_control;                         /* DS:c06a */
int goto_preview_on;                            /* DS:c068 */
int goto_preview_len;                           /* DS:c066 */
int goto_stack_moving;                          /* DS:c064 */
int ui_unit_slot_controls[MAX_STACK];                   /* DS:c052 */
int next_turn_control;                          /* DS:c050 */
int stack_special_action;                       /* DS:c04e  STACK_SPECIAL_*: -1 none, 0 road, 1 settle, 2 purify, 9 meld */
int overland_unused_control_d;                  /* DS:c04c */
int overland_unused_control_c;                  /* DS:c04a */
int overland_unused_control_b;                  /* DS:c048 */
int overland_unused_control_a;                  /* DS:c046 */
int first_move_pending;                            /* DS:c044 */
int stack_move_pending;                         /* DS:c042 */
/* stack_move_pending: 1 after a selection / order; Stack_Move_Click clears it with first_move_pending */
int overland_unused_c040;                       /* DS:c040  kept at 1 or cleared, never otherwise read */
int overland_unused_c03e;                       /* DS:c03e  cleared, never read */

extern int debug_show_landmass;                 /* DS:7004 */
extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern int stack_blink_state;                   /* DS:9442 */
extern int no_units_to_move;                    /* DS:9444 */
extern int city_screen_city;                    /* DS:9644 */
extern int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];               /* DS:964c  unit, 1000 + city or 1100 + node per view square; -1 */
extern int minimap_control;                     /* DS:973e */
extern int map_grid_control;                    /* DS:9740 */
extern int minimap_click_y;                     /* DS:9742 */
extern int minimap_click_x;                     /* DS:9744 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int map_cursor_cell_y;                   /* DS:974e */
extern int map_cursor_cell_x;                   /* DS:9750 */
extern int debug_alt_t_toggle;                  /* DS:9920  debug Alt+T flips it; never read */
extern unsigned special_order_blank_pic;       /* DS:9954 */
extern unsigned patrol_lock_pic;        /* DS:9958 */
extern unsigned done_lock_pic;          /* DS:995a */
extern unsigned meld_button_pic;                /* DS:995c */
extern unsigned purify_button_pic;              /* DS:995e */
extern unsigned build_button_pic;               /* DS:9962 */
extern unsigned done_button_pic;                /* DS:9964 */
extern unsigned wait_button_pic;          /* DS:9966 */
extern unsigned patrol_button_pic;              /* DS:9968 */
extern unsigned topbar_spells_pic;              /* DS:996a */
extern unsigned topbar_info_pic;                /* DS:996c */
extern unsigned topbar_magic_pic;               /* DS:996e */
extern unsigned topbar_cities_pic;              /* DS:9970 */
extern unsigned topbar_game_pic;                /* DS:9972 */
extern unsigned topbar_armies_pic;              /* DS:9974 */
extern unsigned topbar_plane_pic;               /* DS:9976 */
extern int current_screen;                      /* DS:9990 */
extern int zoom_steps;                          /* DS:9992 */
extern int zoom_from_y;                         /* DS:9994 */
extern int zoom_from_x;                         /* DS:9996 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int map_view_plane;                      /* DS:bd86 */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int game_turn;                           /* DS:bd90 */
extern int citylist_saved_first_row;            /* DS:c09a */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
void Minimap_Alloc_Pic_Copy(void);
void Stack_Clear_Draw_Priority(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Minimap_Build_View(void);
void Mouse_Default_Cursor(void);
void Stack_Update_Special_Action(void);
void UI_Input_Delay(int delay);
void Help_Load_Main_Screen(void);
int UI_Add_Hotkey(char *key);
int UI_Add_Alt_Hotkey(char *code);
void Screen_Info_Menu(int choice);
void Save_Game(int slot);
void Play_Click_Sound(void);
void Play_Click_Sound_Casting(void);
void Stack_Set_Order(int player, int *x, int *y, int *plane, int order, int dest_x, int dest_y);
void Stack_Meld_Button(void);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
int Stack_Settle_Button(void);
void Overland_Plane_Button(int player, int *vx, int *vy, int *plane);
int Stack_Toggle_Unit(int slot);
void Stack_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2);
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2);
int Player_Has_Unordered_Units(void);
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana);
int Confirm_Box(char *text);
int Stack_Selected_Moves(void);
int Stack_Preview_GoTo_Path(int unit);
int Stack_Earth_Gate_Jump(int x, int y, int plane);
int Stack_Can_Planar_Travel(void);
int Stack_Move_Click(int dest_x, int dest_y, int player, int *view_x, int *view_y, int *view_plane);
void Map_Normalize_Scroll_Target(void);
int Screen_Stack_Popup(int unit, int select, int x, int y);
void Screen_Outpost(int is_new_outpost, int unused_zoom_x, int unused_zoom_y);
void Screen_City_View(void);
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
int Debug_Print_Labeled_Number(int col, int row, int value, char *label);
void Dialog_Name_Starting_City(int city);
void Overland_Frame_AI_Nop(void);
void Overland_Frame_Map_Nop(void);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Overland_Screen_Draw(int *vx, int *vy, int plane, int tx, int ty, int player);

/* () the overland main screen until another screen is chosen (current_screen) or the turn ends
   Numbered: BUG #202 (BUGS_AND_QUIRKS.txt) */
void Screen_Overland_Map(void)
{
    int done;
    int i;
    int answer;
    int unit;
    int px;                                     /* a map square at the go-to click, screen pixels elsewhere */
    int py;
    int slot_x2;
    int slot_y2;
    int sel_x;                                  /* set only by the right-click select: the Meld / Settle
                                                   buttons pass it before any assignment (original) */
    int sel_y;
    int key_x;
    int key_none;                               /* set to -1000, never used */
    int key_excl;
    int key_at;
    int key_c;
    int key_q;
    int entry;
    int on_view;
    int unit_cell_x;                            /* the unit's map x, then its column in the map view */
    int unit_y;                                 /* the unit's map y (a map row throughout) */
    int selected_moves;
    int key_alt_t;
    int key_f10;
    int key_alt_k;
    int key_hash;
    int key_alt_n;
    int food;
    int gold;
    int mana;
    int key_n;
    int key_nw;
    int key_ne;
    int key_w;
    int key_e;
    int key_sw;
    int key_s;
    int key_se;
    int key_alt_a;
    int autoplay_ticks;
    int food_refused;
    int key_f1;
    int key_f2;
    int key_f3;
    int key_f4;
    int key_f5;
    int key_f6;
    int key_f7;
    int key_f8;
    int key_f9;
    char turns_text[10];
    int key_u;
    int key_alt_p;
    int input;
    int no_redraw;

    Reset_Clip_Rect();
    UI_Clear_Controls();
    overland_dirty_never_read = 1;
    Minimap_Alloc_Pic_Copy();
    Stack_Blink_Reset();
    Set_Text_Shadow_Color(0);
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Map_Redraw, 1);
    stack_panel_x = STACK_PANEL_HOME_X;
    stack_panel_y = STACK_PANEL_HOME_Y;
    citylist_saved_first_row = 0;
    overland_unused_c03e = 0;
    first_move_pending = 0;
    stack_move_pending = 1;
    Map_Invalidate();
    Minimap_Build_View();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Mouse_Default_Cursor();
    Stack_Update_Special_Action();
    if (goto_stack_moving != 1)
        goto_stack_moving = 0;
    if (overland_unused_c040 != 1)
        overland_unused_c040 = 0;
    no_redraw = 0;
    food_refused = 0;
    UI_Input_Delay(1);
    Palette_Cycle_Reset();
    UI_Clear_Help();
    Help_Load_Main_Screen();
    autoplay_ticks = -1;
    Map_Redraw();
    Screen_Present();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        Overland_Add_Controls();
        key_x = UI_Add_Hotkey("X");
        key_none = UI_NO_CONTROL;
        key_excl = UI_Add_Hotkey("!");
        key_at = UI_Add_Hotkey("@");
        key_hash = UI_Add_Hotkey("#");
        key_c = UI_Add_Hotkey("C");
        key_alt_t = UI_Add_Alt_Hotkey("T");
        key_alt_n = UI_Add_Alt_Hotkey("N");
        key_alt_p = UI_Add_Alt_Hotkey("P");
        key_q = UI_Add_Hotkey("Q");
        key_u = UI_Add_Hotkey("U");
        key_nw = UI_Add_Hotkey("\x1a");
        key_n = UI_Add_Hotkey("\x03");
        key_ne = UI_Add_Hotkey("\x19");
        key_w = UI_Add_Hotkey("\x01");
        key_e = UI_Add_Hotkey("\x02");
        key_sw = UI_Add_Hotkey("\x1c");
        key_s = UI_Add_Hotkey("\x04");
        key_se = UI_Add_Hotkey("\x1d");
        key_f10 = UI_Add_Hotkey("\x17");
        key_f1 = UI_Add_Hotkey("\x0e");
        key_f2 = UI_Add_Hotkey("\x0f");
        key_f3 = UI_Add_Hotkey("\x10");
        key_f4 = UI_Add_Hotkey("\x11");
        key_f5 = UI_Add_Hotkey("\x12");
        key_f6 = UI_Add_Hotkey("\x13");
        key_f7 = UI_Add_Hotkey("\x14");
        key_f8 = UI_Add_Hotkey("\x15");
        key_f9 = UI_Add_Hotkey("\x16");
        key_alt_k = UI_Add_Alt_Hotkey("K");
        key_alt_a = UI_Add_Alt_Hotkey("A");
        input = UI_Get_Input();
        if (input == key_f1) {
            Screen_Info_Menu(0);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f2) {
            Screen_Info_Menu(1);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f3) {
            Screen_Info_Menu(2);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f4) {
            Screen_Info_Menu(3);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f5) {
            Screen_Info_Menu(4);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f6) {
            Screen_Info_Menu(5);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f7) {
            Screen_Info_Menu(6);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f8) {
            Screen_Info_Menu(7);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_f9) {
            Screen_Info_Menu(8);
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Overland_Screen_Reset();
        }
        if (input == key_alt_a && Check_Release_Version() == 0)
            debug_autoplay = 1 - debug_autoplay;
        if (input == key_alt_p) {               /* not gated: works in the release build */
            for (i = 1; i < num_players; i++)
                wizards[i].personality = Random(6) - 1;
        }
        if (input == key_alt_k && Check_Release_Version() == 0) {
            game_config.strategic_combat = 1 - game_config.strategic_combat;
            debug_strategic_toggle = 1 - debug_strategic_toggle;
        }
        if (input == key_hash && Check_Release_Version() == 0) {
        }
        if (input == key_alt_t && Check_Release_Version() == 0)
            debug_alt_t_toggle = 1 - debug_alt_t_toggle;
        if (input == key_alt_n && Check_Release_Version() == 0)
            debug_show_landmass = 1 - debug_show_landmass;
        if (input == key_f10)
            Save_Game(SAVE_SLOT_CONTINUE);
        if (input == key_x) {
        }
        if (input == key_c && stack_unit_count > 0) {
            unit = stack_units[0].unit;
            map_view_plane = p_units[unit].plane;
            Map_Center_View(&map_view_x, &map_view_y, p_units[unit].x, p_units[unit].y, p_units[unit].plane);
            Minimap_Build_View();
            Mouse_Default_Cursor();
            Map_Invalidate();
        }
        if (input == key_q || input == key_u) {
            stack_unit_count = 0;
            Stack_Blink_Off();
            Units_Set_Draw_Priority();
            Stack_Clear_Draw_Priority();
            Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
            Map_Invalidate();
            no_redraw = 1;
        }
        if (input == topbar_info_control) {
            Play_Click_Sound();
            current_screen = SCREEN_INFO;
            done = 1;
        }
        if (input == topbar_game_control) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_GAME_MENU;
        }
        if (input == topbar_cities_control) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_CITIES;
        }
        if (input == topbar_magic_control) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_MAGIC;
        }
        if (input == topbar_armies_control) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_ARMIES;
        }
        if (input == topbar_spells_control) {
            Play_Click_Sound();
            if (wizards[current_player].casting_spell == SPELL_SPELL_OF_RETURN) {
                i = wizards[current_player].casting_cost_left / wizards[current_player].nominal_casting_skill;
                itoa(i, turns_text, 10);
                strcpy(message_text, "You may not throw any spells while you are banished.  There are at least ");
                strcat(message_text, turns_text);
                strcat(message_text, " turns remaining until you may return.");
                Message_Box(message_text);
                UI_Set_Redraw_Fn(Map_Redraw, 1);
            } else {
                Stack_Blink_Off();
                Units_Set_Draw_Priority();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                done = 1;
                current_screen = SCREEN_SPELLBOOK;
            }
        }
        if (input == order_patrol_control) {
            Play_Click_Sound();
            Stack_Blink_Reset();
            Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_PATROL, 0, 0);
            Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
            Overland_Screen_Reset();
            if (no_units_to_move == 1) {
                Units_Set_Draw_Priority();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                no_redraw = 1;
            }
        }
        if (input == order_special_control) {
            Play_Click_Sound();
            switch (stack_special_action) {
            case STACK_SPECIAL_MELD:
                Stack_Meld_Button();
                Player_Stacks_Changed_Nop(current_player);
                Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, sel_x, sel_y);
                Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
                Overland_Screen_Reset();
                break;
            case STACK_SPECIAL_PURIFY:
                Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_PURIFY, 0, 0);
                Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
                Overland_Screen_Reset();
                if (no_units_to_move == 1)
                    no_redraw = 1;
                break;
            case STACK_SPECIAL_ROAD:
                done = -1;
                current_screen = SCREEN_ROAD_BUILDING;
                break;
            case STACK_SPECIAL_SETTLE:
                if (Stack_Settle_Button() == 1) {
                    Player_Stacks_Changed_Nop(current_player);
                    Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, sel_x, sel_y);
                    Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
                    Overland_Screen_Reset();
                    no_redraw = 1;
                }
                UI_Set_Redraw_Fn(Map_Redraw, 1);
                break;
            }
        }
        if (input == order_done_control) {
            Play_Click_Sound();
            Stack_Blink_Reset();
            Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_DONE, 0, 0);
            Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
            Overland_Screen_Reset();
            if (no_units_to_move == 1) {
                no_redraw = 1;
                Units_Set_Draw_Priority();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
            }
        }
        if (input == order_wait_control) {
            Play_Click_Sound();
            Stack_Blink_Reset();
            Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_WAIT, 0, 0);
            Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
            Overland_Screen_Reset();
            if (no_units_to_move == 1) {
                Units_Set_Draw_Priority();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                no_redraw = 1;
            }
        }
        if (input == topbar_plane_control) {
            Play_Click_Sound();
            Overland_Plane_Button(current_player, &map_view_x, &map_view_y, &map_view_plane);
            Overland_Screen_Reset();
            Map_Invalidate();
            Minimap_Build_View();
            UI_Suspend_Redraw_Fn();
            UI_Set_Redraw_Fn(Map_Redraw, 1);
            Mouse_Default_Cursor();
        }
        for (i = 0; i < stack_unit_count; i++) {
            if (ui_unit_slot_controls[i] == input) {
                Play_Click_Sound_Casting();
                Stack_Toggle_Unit(i);
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                overland_dirty_never_read = 1;
                break;
            }
        }
        for (i = 0; i < stack_unit_count; i++) {
            if (-ui_unit_slot_controls[i] == input) {   /* UI_Get_Input returns -id for a right click */
                Stack_Blink_Off();
                Units_Set_Draw_Priority();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                Map_Redraw();
                Screen_Present();
                Stack_Slot_Rect(i, &px, &py, &slot_x2, &slot_y2);
                Unit_Stats_Popup(stack_units[i].unit, px, py, px + 18, py + 18);
                UI_Set_Redraw_Fn(Map_Redraw, 1);
                Minimap_Alloc_Pic_Copy();
                Mouse_Default_Cursor();
                Stack_Blink_Reset();
                Units_Set_Draw_Priority();
                Stack_Clear_Draw_Priority();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                Map_Invalidate();
                Minimap_Build_View();
                no_redraw = 1;
                UI_Clear_Help();
                Help_Load_Main_Screen();
                break;
            }
        }
        if (input == next_turn_control || (time_stop_player > 0 && current_player + 1 != time_stop_player)
            || (no_units_to_move == 1 && food_refused == 0 && game_config.end_turn_wait == 0
                && Player_Has_Unordered_Units() == 1)) {
            Play_Click_Sound();
            Wizard_Income_Summary(current_player, &gold, &food, &mana);
            no_redraw = 1;
            if (food < 0) {
                if (input == next_turn_control || food_refused == 0) {
                    if (time_stop_player > 0 && current_player + 1 != time_stop_player)
                        answer = 1;
                    else if (food < 0)
                        answer = Confirm_Box("Some units do not have enough food and will die unless you allocate more farmers in a city.  Do you wish to allow them to die?");
                    if (answer == 1) {
                        done = -1;
                        current_screen = SCREEN_ENDING_TURN;
                        Map_Redraw();
                        UI_Clear_Controls();
                        Screen_Present();
                        current_screen = SCREEN_END_TURN;
                        food_refused = 0;
                    } else
                        food_refused = 1;
                } else
                    food_refused = 1;
            } else {
                current_screen = SCREEN_ENDING_TURN;
                Map_Redraw();
                UI_Clear_Controls();
                Screen_Present();
                current_screen = SCREEN_END_TURN;
                food_refused = 0;
                done = -1;
            }
            UI_Set_Redraw_Fn(Map_Redraw, 1);
        }
        if (input == key_n) {
            Stack_Key_Move_Dir(KEYDIR_N);
            no_redraw = 1;
        }
        if (input == key_nw) {
            Stack_Key_Move_Dir(KEYDIR_NW);
            no_redraw = 1;
        }
        if (input == key_ne) {
            Stack_Key_Move_Dir(KEYDIR_NE);
            no_redraw = 1;
        }
        if (input == key_w) {
            Stack_Key_Move_Dir(KEYDIR_W);
            no_redraw = 1;
        }
        if (input == key_e) {
            Stack_Key_Move_Dir(KEYDIR_E);
            no_redraw = 1;
        }
        if (input == key_sw) {
            Stack_Key_Move_Dir(KEYDIR_SW);
            no_redraw = 1;
        }
        if (input == key_s) {
            Stack_Key_Move_Dir(KEYDIR_S);
            no_redraw = 1;
        }
        if (input == key_se) {
            Stack_Key_Move_Dir(KEYDIR_SE);
            no_redraw = 1;
        }
        if (input == map_grid_control && stack_unit_count != 0) {
            selected_moves = Stack_Selected_Moves();
            if (selected_moves < 1) {
                Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_GOTO,
                                (map_view_x + map_cursor_cell_x) % MAP_WIDTH, (map_view_y + map_cursor_cell_y) % MAP_HEIGHT);
                unit = stack_units[0].unit;
                Stack_Preview_GoTo_Path(unit);
                if (goto_preview_len < 1)
                    Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_READY,
                                    (map_view_x + map_cursor_cell_x) % MAP_WIDTH, (map_view_y + map_cursor_cell_y) % MAP_HEIGHT);
                Mouse_Default_Cursor();
                Map_Invalidate();
                overland_dirty_never_read = 1;
            }
            if (goto_preview_on == 1) {
                overland_dirty_never_read = 1;
                unit = stack_units[0].unit;
                if (p_units[unit].plane == map_view_plane) {
                    unit_y = p_units[unit].y;
                    if (unit_y >= map_view_y && unit_y < map_view_y + MAP_VIEW_ROWS) {
                        on_view = 0;
                        unit_cell_x = p_units[unit].x;
                        if (unit_cell_x >= map_view_x) {
                            unit_cell_x -= map_view_x;
                            on_view = 1;
                        } else {
                            unit_cell_x += MAP_WIDTH;
                            if (unit_cell_x >= map_view_x && unit_cell_x < map_view_x + MAP_VIEW_COLS) {
                                unit_cell_x -= map_view_x;
                                on_view = 1;
                            }
                        }
                        if (on_view == 1 && map_cursor_cell_x == unit_cell_x && unit_y - map_view_y == map_cursor_cell_y) {
                            Stack_Set_Order(current_player, &map_view_x, &map_view_y, &map_view_plane, ORDER_READY, 0, 0);
                            goto_stack_moving = 0;
                            goto_preview_on = 0;
                        }
                    }
                }
            } else {
                overland_dirty_never_read = 1;
                if (no_units_to_move == 0) {
                    px = (map_view_x + map_cursor_cell_x) % MAP_WIDTH;
                    py = map_view_y + map_cursor_cell_y;
                    if (Stack_Earth_Gate_Jump(px, py, map_view_plane) == 0) {
                        if (Stack_Can_Planar_Travel() == 0) {
                            if (p_units[stack_units[0].unit].plane == map_view_plane
                                || p_units[stack_units[0].unit].in_tower == 1)
                                Stack_Move_Click(px, py, current_player, &map_view_x, &map_view_y, &map_view_plane);
                            else
                                Message_Box("The selected units cannot move on this plane.");
                        } else {
                            if (p_units[stack_units[0].unit].plane == map_view_plane
                                || p_units[stack_units[0].unit].in_tower == 1)
                                Stack_Move_Click(px, py, current_player, &map_view_x, &map_view_y, &map_view_plane);
                            else
                                Message_Box("The selected units cannot move on this plane.");
                        }
                    }
                    Overland_Screen_Reset();
                    no_redraw = 1;
                }
            }
        }
        if (-map_grid_control == input) {
            entry = map_unit_grid[map_cursor_cell_y][map_cursor_cell_x];
            if (entry != -1) {
                Play_Click_Sound_Casting();
                entry = abs(entry);
                if (entry < GRID_CITY_BASE) {
                    if (p_units[entry].owner == current_player) {
                        top_unit_in_stack = entry;
                        unit = top_unit_in_stack;
                        sel_x = p_units[unit].x;
                        sel_y = p_units[unit].y;
                        next_unit_ref_x = p_units[unit].x;
                        next_unit_ref_y = p_units[unit].y;
                        Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, sel_x, sel_y);
                        if (no_units_to_move == 1)
                            no_units_to_move = 0;
                        Stack_Blink_Reset();
                        stack_blink_state = 0;
                        Units_Set_Draw_Priority();
                        Stack_Clear_Draw_Priority();
                        Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                        Mouse_Default_Cursor();
                        Map_Invalidate();
                    } else {
                        px = map_cursor_cell_x * MAP_TILE_W;
                        py = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                        UI_Clear_Help();
                        Screen_Stack_Popup(entry, 0, px, py);
                        UI_Set_Redraw_Fn(Map_Redraw, 1);
                        Minimap_Alloc_Pic_Copy();
                        Mouse_Default_Cursor();
                        Map_Invalidate();
                        Minimap_Build_View();
                        UI_Clear_Help();
                        Help_Load_Main_Screen();
                    }
                    no_redraw = 1;
                    overland_dirty_never_read = 1;
                } else if (entry < GRID_NODE_BASE) {
                    city_screen_city = entry - GRID_CITY_BASE;
                    if (p_cities[city_screen_city].owner == current_player) {
                        if (p_cities[city_screen_city].size == 0) {
                            present_mode = PRESENT_ZOOM;
                            zoom_from_x = map_cursor_cell_x * MAP_TILE_W;
                            zoom_from_y = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                            zoom_steps = 8;
                            UI_Clear_Help();
                            px = map_cursor_cell_x * MAP_TILE_W;
                            py = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                            Screen_Outpost(0, px, py);
                            UI_Set_Redraw_Fn(Map_Redraw, 1);
                            Minimap_Alloc_Pic_Copy();
                            Mouse_Default_Cursor();
                            Map_Invalidate();
                            Minimap_Build_View();
                            no_redraw = 1;
                            UI_Clear_Help();
                            Help_Load_Main_Screen();
                            overland_dirty_never_read = 1;
                        } else {
                            present_mode = PRESENT_ZOOM;
                            zoom_from_x = map_cursor_cell_x * MAP_TILE_W;
                            zoom_from_y = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                            zoom_steps = 8;
                            current_screen = SCREEN_CITY;
                            done = 1;
                        }
                    } else {
                        UI_Clear_Help();
                        Screen_City_View();
                        UI_Set_Redraw_Fn(Map_Redraw, 1);
                        Minimap_Alloc_Pic_Copy();
                        Mouse_Default_Cursor();
                        Map_Invalidate();
                        Minimap_Build_View();
                        no_redraw = 1;
                        UI_Clear_Help();
                        Help_Load_Main_Screen();
                        overland_dirty_never_read = 1;
                    }
                } else {
                    /* 1100 + node: nothing */
                }
            } else {
                map_scroll_target_x += map_cursor_cell_x - MAP_VIEW_COLS / 2;
                map_scroll_target_y += map_cursor_cell_y - MAP_VIEW_ROWS / 2;
                Map_Normalize_Scroll_Target();
            }
        }
        if (abs(input) == minimap_control) {
            Play_Click_Sound();
            Map_Window_Origin(&px, &py, (map_view_x + MAP_VIEW_COLS / 2) % MAP_WIDTH, map_view_y + MAP_VIEW_ROWS / 2, MINIMAP_W, MINIMAP_H);
            map_scroll_target_x = minimap_click_x + px;
            map_scroll_target_y = minimap_click_y + py;
            map_view_x = map_scroll_target_x;
            map_view_y = map_scroll_target_y;
            Map_Center_View(&map_view_x, &map_view_y, map_scroll_target_x, map_scroll_target_y, map_view_plane);
            Minimap_Build_View();
            Mouse_Default_Cursor();
            Map_Invalidate();
            no_redraw = 1;
        }
        if (debug_autoplay == 1 && input == 0) {
            if (autoplay_ticks > 5) {
                done = -1;
                current_screen = SCREEN_END_TURN;
                autoplay_ticks = -1;
                input = 0;
            } else
                autoplay_ticks++;
            Debug_Print_Labeled_Number(25, 0, game_turn, "turn");
        }
        if (done == 0 && no_redraw == 0) {
            Map_Redraw();
            if (game_turn == 0) {
                if (start_city_named == 0) {
                    Dialog_Name_Starting_City(0);
                    UI_Set_Redraw_Fn(Map_Redraw, 1);
                    start_city_named = 1;
                }
            } else
                start_city_named = 1;
            Overland_Frame_AI_Nop();
            Overland_Frame_Map_Nop();
            Screen_Present();
            Wait_Ticks(1);
        }
        no_redraw = 0;
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
}

/* () all the overland click regions: the unit slots (or the end-turn area when no unit is left to move), the map grid
   and the minimap (only when the view is not scrolling), the top bar and the order buttons */
void Overland_Add_Controls(void)
{
    int i;

    overland_unused_control_d = UI_NO_CONTROL;
    overland_unused_control_c = UI_NO_CONTROL;
    overland_unused_control_b = UI_NO_CONTROL;
    overland_unused_control_a = UI_NO_CONTROL;
    map_grid_control = UI_NO_CONTROL;
    minimap_control = UI_NO_CONTROL;
    next_turn_control = UI_NO_CONTROL;
    order_special_control = UI_NO_CONTROL;
    order_patrol_control = UI_NO_CONTROL;
    order_wait_control = UI_NO_CONTROL;
    order_done_control = UI_NO_CONTROL;
    topbar_game_control = UI_NO_CONTROL;
    topbar_spells_control = UI_NO_CONTROL;
    topbar_armies_control = UI_NO_CONTROL;
    topbar_cities_control = UI_NO_CONTROL;
    topbar_magic_control = UI_NO_CONTROL;
    topbar_info_control = UI_NO_CONTROL;
    topbar_plane_control = UI_NO_CONTROL;
    for (i = 0; i < MAX_STACK; i++)
        ui_unit_slot_controls[i] = UI_NO_CONTROL;
    if (stack_unit_count > 0)
        Overland_Add_Stack_Icon_Controls();
    else if (no_units_to_move == 1)
        next_turn_control = UI_Add_Region(248, 175, 313, 199, "N", -1);
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        map_grid_control = UI_Add_Grid_Control(0, MAP_VIEW_TOP, MAP_TILE_W, MAP_TILE_H, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_cursor_cell_x, &map_cursor_cell_y, -1);
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        minimap_control = UI_Add_Grid_Control(MINIMAP_X, MINIMAP_Y, 1, 1, MINIMAP_W, MINIMAP_H, &minimap_click_x, &minimap_click_y, -1);
    Overland_Add_TopBar_Buttons();
    Overland_Add_Order_Buttons();
}

/* () Done (D), Patrol, Wait (W) and the special button (Build B, Purify, Meld); only Wait when no stack is
   selected and units are left to move */
void Overland_Add_Order_Buttons(void)
{
    if (stack_unit_count > 0) {
        order_done_control = UI_Add_Button(246, 176, "", done_button_pic, "D", -1);
        order_patrol_control = UI_Add_Button(280, 176, "", patrol_button_pic, "", -1);
        order_wait_control = UI_Add_Button(246, 186, "", wait_button_pic, "W", -1);
        order_special_control = UI_NO_CONTROL;
        if (stack_special_action != STACK_SPECIAL_NONE) {
            switch (stack_special_action) {
            case STACK_SPECIAL_PURIFY:
                order_special_control = UI_Add_Button(280, 186, "", purify_button_pic, "", -1);
                break;
            case STACK_SPECIAL_MELD:
                order_special_control = UI_Add_Button(280, 186, "", meld_button_pic, "", -1);
                break;
            default:
                order_special_control = UI_Add_Button(280, 186, "", build_button_pic, "B", -1);
                break;
            }
        }
    }
    if (stack_unit_count < 1) {
        order_done_control = UI_NO_CONTROL;
        order_patrol_control = UI_NO_CONTROL;
        order_wait_control = UI_NO_CONTROL;
        order_special_control = UI_NO_CONTROL;
        if (no_units_to_move == 0)
            order_wait_control = UI_Add_Button(246, 186, "", wait_button_pic, "W", -1);
    }
}

/* () draws the order buttons with their help ids; with no stack selected Done / Patrol / the special are greyed and
   Wait is drawn normally, all with help -1 */
void Overland_Draw_Order_Buttons(void)
{
    if (stack_unit_count > 0) {
        Set_Pic_Frame(done_button_pic, 0);
        Draw_Pic_Frame(246, 176, done_button_pic);
        help_entries[MAIN_HELP_DONE].id = HELP_DONE;
        Set_Pic_Frame(patrol_button_pic, 0);
        Draw_Pic_Frame(280, 176, patrol_button_pic);
        help_entries[MAIN_HELP_PATROL].id = HELP_PATROL;
        Set_Pic_Frame(wait_button_pic, 0);
        Draw_Pic_Frame(246, 186, wait_button_pic);
        help_entries[MAIN_HELP_WAIT].id = HELP_WAIT;
        if (stack_special_action != STACK_SPECIAL_NONE) {
            switch (stack_special_action) {
            case STACK_SPECIAL_PURIFY:
                Set_Pic_Frame(purify_button_pic, 0);
                Draw_Pic_Frame(280, 186, purify_button_pic);
                help_entries[MAIN_HELP_SPECIAL].id = HELP_PURIFY;
                break;
            case STACK_SPECIAL_MELD:
                Set_Pic_Frame(meld_button_pic, 0);
                Draw_Pic_Frame(280, 186, meld_button_pic);
                help_entries[MAIN_HELP_SPECIAL].id = HELP_MELD;
                break;
            default:
                Set_Pic_Frame(build_button_pic, 0);
                Draw_Pic_Frame(280, 186, build_button_pic);
                help_entries[MAIN_HELP_SPECIAL].id = HELP_BUILD;
                break;
            }
        } else {
            Set_Pic_Frame(special_order_blank_pic, 0);
            Draw_Pic_Frame(280, 186, special_order_blank_pic);
            help_entries[MAIN_HELP_SPECIAL].id = -1;
        }
    } else {
        Set_Pic_Frame(done_lock_pic, 0);
        Draw_Pic_Frame(246, 176, done_lock_pic);
        help_entries[MAIN_HELP_DONE].id = -1;
        Set_Pic_Frame(patrol_lock_pic, 0);
        Draw_Pic_Frame(280, 176, patrol_lock_pic);
        help_entries[MAIN_HELP_PATROL].id = -1;
        Set_Pic_Frame(special_order_blank_pic, 0);
        Draw_Pic_Frame(280, 186, special_order_blank_pic);
        help_entries[MAIN_HELP_SPECIAL].id = -1;
        Set_Pic_Frame(wait_button_pic, 0);
        Draw_Pic_Frame(246, 186, wait_button_pic);
        help_entries[MAIN_HELP_WAIT].id = -1;
    }
}

/* () the top bar: Game (G), Spells (S), Armies (A), Cities, Magic (M), Info (I), Plane (P) */
void Overland_Add_TopBar_Buttons(void)
{
    topbar_game_control = UI_Add_Button(7, 4, "", topbar_game_pic, "G", -1);
    topbar_spells_control = UI_Add_Button(0x2f, 4, "", topbar_spells_pic, "S", -1);
    topbar_armies_control = UI_Add_Button(0x59, 4, "", topbar_armies_pic, "A", -1);
    topbar_cities_control = UI_Add_Button(0x8c, 4, "", topbar_cities_pic, "", -1);
    topbar_magic_control = UI_Add_Button(0xb8, 4, "", topbar_magic_pic, "M", -1);
    topbar_info_control = UI_Add_Button(0xe2, 4, "", topbar_info_pic, "I", -1);
    topbar_plane_control = UI_Add_Button(0x10e, 4, "", topbar_plane_pic, "P", -1);
}

/* () draws the 7 top-bar buttons at their resting frame */
void Overland_Draw_TopBar(void)
{
    Set_Pic_Frame(topbar_game_pic, 0);
    Draw_Pic_Frame(7, 4, topbar_game_pic);
    Set_Pic_Frame(topbar_spells_pic, 0);
    Draw_Pic_Frame(0x2f, 4, topbar_spells_pic);
    Set_Pic_Frame(topbar_armies_pic, 0);
    Draw_Pic_Frame(0x59, 4, topbar_armies_pic);
    Set_Pic_Frame(topbar_cities_pic, 0);
    Draw_Pic_Frame(0x8c, 4, topbar_cities_pic);
    Set_Pic_Frame(topbar_magic_pic, 0);
    Draw_Pic_Frame(0xb8, 4, topbar_magic_pic);
    Set_Pic_Frame(topbar_info_pic, 0);
    Draw_Pic_Frame(0xe2, 4, topbar_info_pic);
    Set_Pic_Frame(topbar_plane_pic, 0);
    Draw_Pic_Frame(0x10e, 4, topbar_plane_pic);
}

/* () a click region per selected unit's slot (left click toggles it, right click the stats popup) */
void Overland_Add_Stack_Icon_Controls(void)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int i;

    for (i = 0; i < stack_unit_count; i++) {
        Stack_Slot_Rect(i, &x1, &y1, &x2, &y2);
        ui_unit_slot_controls[i] = UI_Add_Region(x1 - 1, y1 - 1, x2 - 2, y2 - 2, "", -1);
    }
}

/* () the overland screen at the current view (also the screens' redraw callback) */
void Map_Redraw(void)
{
    Reset_Clip_Rect();
    Set_Draw_Page();
    Overland_Screen_Draw(&map_view_x, &map_view_y, map_view_plane, map_scroll_target_x, map_scroll_target_y, current_player);
}

/* () back on the overland screen after another one: pictures, controls, blinking, draw order, unit grid, cursor,
   minimap, redraw callback and help */
void Overland_Screen_Reset(void)
{
    Minimap_Alloc_Pic_Copy();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Stack_Blink_Reset();
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Mouse_Default_Cursor();
    Minimap_Build_View();
    UI_Set_Redraw_Fn(Map_Redraw, 1);
    Map_Invalidate();
    UI_Clear_Help();
    Help_Load_Main_Screen();
    overland_dirty_never_read = 1;
}

/* (numpad direction 1-9) moves the selected stack one square: the first unit's neighbour, y clamped to 0..39 (BUG #203:
   x - 1 at x = 0 gives -1, no wrap; 5 leaves x / y unset); without Planar Travel no plane check (BUG #204)
   Numbered: BUG #203, #204 (BUGS_AND_QUIRKS.txt) */
void Stack_Key_Move_Dir(int dir)
{
    int moves;
    int x;
    int y;

    if (stack_unit_count != 0) {
        moves = Stack_Selected_Moves();
        if (moves > 0) {
            if (no_units_to_move == 0) {
                switch (dir) {
                case KEYDIR_SW:
                    x = (p_units[stack_units[0].unit].x - 1) % MAP_WIDTH;
                    y = p_units[stack_units[0].unit].y + 1;
                    break;
                case KEYDIR_S:
                    x = p_units[stack_units[0].unit].x;
                    y = p_units[stack_units[0].unit].y + 1;
                    break;
                case KEYDIR_SE:
                    x = (p_units[stack_units[0].unit].x + 1) % MAP_WIDTH;
                    y = p_units[stack_units[0].unit].y + 1;
                    break;
                case KEYDIR_W:
                    x = (p_units[stack_units[0].unit].x - 1) % MAP_WIDTH;
                    y = p_units[stack_units[0].unit].y;
                    break;
                case KEYDIR_E:
                    x = (p_units[stack_units[0].unit].x + 1) % MAP_WIDTH;
                    y = p_units[stack_units[0].unit].y;
                    break;
                case KEYDIR_NW:
                    x = (p_units[stack_units[0].unit].x - 1) % MAP_WIDTH;
                    y = p_units[stack_units[0].unit].y - 1;
                    break;
                case KEYDIR_N:
                    x = p_units[stack_units[0].unit].x;
                    y = p_units[stack_units[0].unit].y - 1;
                    break;
                case KEYDIR_NE:
                    x = (p_units[stack_units[0].unit].x + 1) % MAP_WIDTH;
                    y = p_units[stack_units[0].unit].y - 1;
                    break;
                }
                if (y < 0)
                    y = 0;
                if (y >= MAP_HEIGHT)
                    y = MAP_HEIGHT - 1;
                if (Stack_Can_Planar_Travel() == 0)
                    Stack_Move_Click(x, y, current_player, &map_view_x, &map_view_y, &map_view_plane);
                else if (p_units[stack_units[0].unit].plane == map_view_plane || p_units[stack_units[0].unit].in_tower == 1)
                    Stack_Move_Click(x, y, current_player, &map_view_x, &map_view_y, &map_view_plane);
                else
                    Message_Box("The selected units cannot move on this plane.");
                Overland_Screen_Reset();
            }
        }
    }
}

