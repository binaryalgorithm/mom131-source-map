/* SPELLTGT.C - WIZARDS.EXE overlay code segment 5c50: the overland target selection of a spell - the map screen with
   the "Casting" panel and its prompt ("Select a friendly unit as the target for a <Name> spell."), the cursor map
   (the animated target cursor on every valid square), left clicks that pick a unit / stack / city / node / square
   (a unit pick goes through the stack popup), right clicks that open the stack popup or the city view, the minimap,
   the plane button and Cancel; also the map-square-to-screen helper.
   This module's data (DS:3312): the cursor animation frame and its literals; BSS (DS:c13e..c175).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 5c50 src/wizards/SPELLTGT.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Spell_Target_Map_Select(int target_type, int *target, int *y, int *plane, int *screen_x, int *screen_y,
                            char *spell_name);
void SpellTarget_Add_Controls(void);
void SpellTarget_Redraw(void);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
void SpellTarget_Alloc(void);
void SpellTarget_Build_Prompt(int target_type, char *spell_name);
void SpellTarget_Clamp_Square(int *px, int *py, int x, int y);
void SpellTarget_Clear(void);
void SpellTarget_Set_Friendly_Stack(int stack);
void SpellTarget_Set_City(int city);
void SpellTarget_Set_Enemy_Stack(int stack);
void SpellTarget_Set_Node(int node);
int SpellTarget_Set_Square(int x, int y);
void SpellTarget_Build_Cursor_Map(void);
void SpellTarget_Draw_Panel(void);

#include "GAME.H"

int spelltarget_cursor_frame = 0;               /* DS:3312  0..4, the target cursor's animation frame (image 9 + frame) */

int spelltarget_cancel_button;                  /* DS:c174 */
int spelltarget_anim_step;                      /* DS:c172  0..3 */
int spelltarget_dash_phase;                     /* DS:c170  3..0, counts down */
struct MouseRegion *p_spell_target_cursor_map;  /* DS:c16e  [0] the whole screen, [1..120] the 12 x 10 map squares */
int spelltarget_cursor;                         /* DS:c16c  the cursor image of a valid target square */
int spelltarget_result;                        /* DS:c16a  the unit / city / node or the square's map x; -1 none */
int spelltarget_map_y;                          /* DS:c168  the square's map y; -1 */
int spelltarget_plane;                          /* DS:c166  the square's plane; -1 */
int spelltarget_type;                          /* DS:c164  MAP_TARGET_*: the kind of target the spell takes */
int spelltarget_cur_x;                          /* DS:c162  the picked map-view cell, kept in 1..10 */
int spelltarget_cur_y;                          /* DS:c160  the picked map-view cell, kept in 1..8 */
char spelltarget_unused_c14a[22];               /* DS:c14a  never referenced (inside this module's BSS block) */
char *spelltarget_prompt;                       /* DS:c148  100 bytes from Near_Alloc */
int spelltarget_unused_c146;                    /* DS:c146  never referenced (inside this module's BSS block) */
int spelltarget_unused_c144;                         /* DS:c144  set to 0, read nowhere */
int spelltarget_unused_c142;                         /* DS:c142  set to 1, read nowhere */
int spelltarget_start_plane;                    /* DS:c140  map_view_plane at the start, read nowhere */
int spelltarget_spell;                          /* DS:c13e  the casting spell (Earth Lore may target unexplored squares) */

extern unsigned button_blockout_pic;            /* DS:9522 */
extern unsigned cast_button_border_pic;         /* DS:9524 */
extern unsigned main_cancel_button_pic;         /* DS:9526 */
extern unsigned cast_bg_pic;                    /* DS:9528 */
extern int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];               /* DS:964c  unit, 1000 + city or 1100 + node per view square; -1 (no other negatives: the abs() calls are no-ops) */
extern int minimap_control;                     /* DS:973e */
extern int map_grid_control;                    /* DS:9740 */
extern int minimap_click_y;                     /* DS:9742 */
extern int minimap_click_x;                     /* DS:9744 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int map_cursor_cell_y;                   /* DS:974e */
extern int map_cursor_cell_x;                   /* DS:9750 */
extern unsigned main_screen_bg_pic;             /* DS:9922 */
extern int present_mode;                        /* DS:9c8e */
extern int overland_unused_control_a;           /* DS:c046 */
extern int overland_unused_control_b;           /* DS:c048 */
extern int overland_unused_control_c;           /* DS:c04a */
extern int overland_unused_control_d;           /* DS:c04c */
extern int topbar_plane_control;                /* DS:c072 */
extern unsigned p_minimap_pic;                  /* DS:c086 */

void Stack_Blink_Off(void);
void Set_Text_Shadow_Color(int color);
void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Units_Set_Draw_Priority(void);
void Map_Build_Unit_Grid(int x, int y, int plane);
void Minimap_Build_View(void);
void UI_Clear_Help(void);
void Help_Load_Spell_Target(void);
void UI_Input_Delay(int delay);
void Timer_Mark(void);
void UI_Clear_Controls(void);
int UI_Get_Input(void);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
void Overland_Plane_Button(int player, int *vx, int *vy, int *plane);
int Node_At(int x, int y, int plane);
int abs(int x);
void City_Garrison_List(int city, int *count, int *list);
int Screen_Stack_Popup(int unit, int select, int x, int y);
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
void Map_Normalize_Scroll_Target(void);
void Map_Center_View(int *view_x, int *view_y, int x, int y, int plane);
void Screen_City_View(void);
void Screen_Present(void);
void Wait_Ticks(int ticks);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
void Reset_Clip_Rect(void);
void Overland_Add_TopBar_Buttons(void);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void Set_Draw_Page(void);
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty, int player);
void Minimap_Draw(int x, int y, int w, int h);
void Draw_Order_Buttons_Disabled(void);
void Overland_Draw_TopBar(void);
void Draw_Gold_Mana_Reserve(void);
void Set_Pic_Frame(unsigned pic, unsigned frame);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
char *Text_Article_A_An(char *word);
void Map_Grid_Add_Nodes(int vx, int vy, int plane);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);

/* (target_type, &target, &y, &plane, &screen_x, &screen_y, spell name) -> 1 picked / 0 Cancel: the overland target of
   a spell; target_type 0 own unit (through the stack popup) / 1 own stack / 2 enemy unit (popup) / 3 enemy stack /
   4 explored square (any for Earth Lore) / 5 own city / 6 enemy city / 7 node; target = the unit / city / node or the
   square's map x (y / plane only for squares, else -1); a city click on a unit / stack type picks its first garrison
   unit; the screen position is the picked cell's (kept off the view's edges) */
int Spell_Target_Map_Select(int target_type, int *target, int *y, int *plane, int *screen_x, int *screen_y,
                            char *spell_name)
{
    int done;
    int input;
    int hit_test;
    int px;
    int py;
    int redrawn;                                /* read before its first write on the first pass (original) */
    int result;
    int ctrl_c;
    int ctrl_b;
    int ctrl_a;                                 /* ctrl_a .. ctrl_c: set to -1000 (no control), never read */
    int garrison_count;
    int garrison[MAX_STACK];
    register int t;                             /* the grid entry, then the picked unit / city / node */
    register int found;

    present_mode = PRESENT_NORMAL;
    spelltarget_spell = wizards[current_player].casting_spell;
    spelltarget_dash_phase = 0;
    spelltarget_anim_step = 0;
    spelltarget_type = target_type;
    result = 1;
    spelltarget_result = -1;
    spelltarget_map_y = -1;
    spelltarget_plane = -1;
    spelltarget_unused_c144 = 0;
    spelltarget_unused_c142 = 1;
    spelltarget_start_plane = map_view_plane;
    Stack_Blink_Off();
    Set_Text_Shadow_Color(0);
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
    SpellTarget_Alloc();
    SpellTarget_Build_Prompt(spelltarget_type, spell_name);
    Units_Set_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    SpellTarget_Build_Cursor_Map();
    Map_Invalidate();
    Minimap_Build_View();
    UI_Clear_Help();
    Help_Load_Spell_Target();
    UI_Input_Delay(4);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        SpellTarget_Add_Controls();
        ctrl_c = UI_NO_CONTROL;
        ctrl_b = UI_NO_CONTROL;
        ctrl_a = UI_NO_CONTROL;
        input = UI_Get_Input();
        hit_test = UI_Hit_Test();
        if (input == spelltarget_cancel_button) {
            Play_Click_Sound();
            result = 0;
            done = -1;
        }
        if (input == topbar_plane_control) {
            Play_Click_Sound();
            Overland_Plane_Button(current_player, &map_view_x, &map_view_y, &map_view_plane);
            Map_Invalidate();
            Minimap_Build_View();
            UI_Suspend_Redraw_Fn();
            UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
            SpellTarget_Build_Cursor_Map();
        }
        if (input == map_grid_control) {
            found = 0;
            if (spelltarget_type == MAP_TARGET_NODE) {
                t = Node_At((map_view_x + map_cursor_cell_x) % MAP_WIDTH, (map_view_y + map_cursor_cell_y) % MAP_HEIGHT,
                            map_view_plane);
                if (t != -1)
                    t += GRID_NODE_BASE;
            } else
                t = map_unit_grid[map_cursor_cell_y][map_cursor_cell_x];
            if (t != -1) {
                Play_Click_Sound();
                t = abs(t);
                if (spelltarget_type == MAP_TARGET_SQUARE)
                    done = SpellTarget_Set_Square(map_cursor_cell_x, map_cursor_cell_y);
                else {
                    if (t < GRID_CITY_BASE) {
                        if (p_units[t].owner == current_player) {
                            if (spelltarget_type == MAP_TARGET_OWN_UNIT || spelltarget_type == MAP_TARGET_OWN_STACK || spelltarget_type == MAP_TARGET_SQUARE)   /* the
                                                   SQUARE terms here and below are dead: handled above */
                                found = 1;
                        } else if (spelltarget_type == MAP_TARGET_ENEMY_STACK || spelltarget_type == MAP_TARGET_ENEMY_UNIT || spelltarget_type == MAP_TARGET_SQUARE)
                            found = 1;
                    } else if (t < GRID_NODE_BASE) {
                        t = t - GRID_CITY_BASE;
                        switch (spelltarget_type) {
                        case MAP_TARGET_OWN_CITY:
                            if (p_cities[t].owner == current_player)
                                found = 1;
                            break;
                        case MAP_TARGET_ENEMY_CITY:
                            if (p_cities[t].owner != current_player)
                                found = 1;
                            break;
                        case MAP_TARGET_OWN_UNIT:
                        case MAP_TARGET_OWN_STACK:
                            if (p_cities[t].owner == current_player) {
                                City_Garrison_List(t, &garrison_count, garrison);
                                if (garrison_count > 0) {
                                    t = garrison[0];
                                    found = 1;
                                }
                            }
                            break;
                        case MAP_TARGET_ENEMY_UNIT:
                        case MAP_TARGET_ENEMY_STACK:
                            if (p_cities[t].owner != current_player) {
                                City_Garrison_List(t, &garrison_count, garrison);
                                if (garrison_count > 0) {
                                    t = garrison[0];
                                    found = 1;
                                }
                            }
                            break;
                        }
                    } else if (spelltarget_type == MAP_TARGET_NODE) {
                        t = t - GRID_NODE_BASE;
                        found = 1;
                    }
                    if (found == 1) {
                        spelltarget_cur_x = map_cursor_cell_x;
                        spelltarget_cur_y = map_cursor_cell_y;
                        if (spelltarget_cur_x == 0)         /* inside the 12 x 10 view: as the (dead)
                                                   SpellTarget_Clamp_Square */
                            spelltarget_cur_x++;
                        if (spelltarget_cur_x == 11)
                            spelltarget_cur_x--;
                        if (spelltarget_cur_y == 0)
                            spelltarget_cur_y++;
                        if (spelltarget_cur_y == 9)
                            spelltarget_cur_y--;
                        switch (spelltarget_type) {
                        case MAP_TARGET_SQUARE:
                            SpellTarget_Set_Square(map_cursor_cell_x, map_cursor_cell_y);
                            done = 1;
                            break;
                        case MAP_TARGET_OWN_CITY:
                            SpellTarget_Set_City(t);
                            done = 1;
                            break;
                        case MAP_TARGET_ENEMY_CITY:
                            SpellTarget_Set_City(t);
                            done = 1;
                            break;
                        case MAP_TARGET_OWN_UNIT:
                            px = map_cursor_cell_x * MAP_TILE_W;
                            py = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                            spelltarget_result = Screen_Stack_Popup(t, 1, px, py);
                            if (spelltarget_result != -1)
                                done = 1;
                            else {
                                UI_Clear_Help();
                                SpellTarget_Clear();
                                SpellTarget_Alloc();
                                SpellTarget_Build_Prompt(spelltarget_type, spell_name);
                                UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
                                SpellTarget_Build_Cursor_Map();
                                UI_Clear_Help();
                                Help_Load_Spell_Target();
                                Map_Invalidate();
                                Minimap_Build_View();
                                redrawn = 1;
                            }
                            break;
                        case MAP_TARGET_OWN_STACK:
                            SpellTarget_Set_Friendly_Stack(t);
                            done = 1;
                            break;
                        case MAP_TARGET_ENEMY_UNIT:
                            px = map_cursor_cell_x * MAP_TILE_W;
                            py = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                            spelltarget_result = Screen_Stack_Popup(t, 1, px, py);
                            if (spelltarget_result != -1)
                                done = 1;
                            else {
                                UI_Clear_Help();
                                SpellTarget_Clear();
                                SpellTarget_Alloc();
                                SpellTarget_Build_Prompt(spelltarget_type, spell_name);
                                UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
                                SpellTarget_Build_Cursor_Map();
                                Map_Invalidate();
                                Minimap_Build_View();
                                UI_Clear_Help();
                                Help_Load_Spell_Target();
                                redrawn = 1;
                            }
                            break;
                        case MAP_TARGET_ENEMY_STACK:
                            SpellTarget_Set_Enemy_Stack(t);
                            done = 1;
                            break;
                        case MAP_TARGET_NODE:
                            SpellTarget_Set_Node(t);
                            done = 1;
                            break;
                        }
                    }
                }
            } else if (spelltarget_type == MAP_TARGET_SQUARE)
                done = SpellTarget_Set_Square(map_cursor_cell_x, map_cursor_cell_y);
        }
        if (abs(input) == minimap_control) {
            Play_Click_Sound();
            Map_Window_Origin(&px, &py, (map_view_x + MAP_VIEW_COLS / 2) % MAP_WIDTH, map_view_y + MAP_VIEW_ROWS / 2, MINIMAP_W, MINIMAP_H);
            map_scroll_target_x = minimap_click_x + px;
            map_scroll_target_y = minimap_click_y + py;
            Map_Normalize_Scroll_Target();
            Map_Center_View(&map_view_x, &map_view_y, map_scroll_target_x, map_scroll_target_y, map_view_plane);
            Minimap_Build_View();
            SpellTarget_Build_Cursor_Map();
            Map_Invalidate();
            redrawn = 1;
        }
        if (-map_grid_control == input) {
            t = map_unit_grid[map_cursor_cell_y][map_cursor_cell_x];
            if (t != -1) {
                Play_Click_Sound();
                t = abs(t);
                if (t < GRID_CITY_BASE) {
                    if (p_units[t].owner == current_player) {
                        if (spelltarget_type == MAP_TARGET_OWN_UNIT || spelltarget_type == MAP_TARGET_OWN_STACK) {
                            px = map_cursor_cell_x * MAP_TILE_W;
                            py = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                            UI_Clear_Help();
                            Screen_Stack_Popup(t, 0, px, py);
                            UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
                            SpellTarget_Alloc();
                            SpellTarget_Build_Prompt(spelltarget_type, spell_name);
                            SpellTarget_Build_Cursor_Map();
                            Map_Invalidate();
                            Minimap_Build_View();
                            UI_Clear_Help();
                            Help_Load_Spell_Target();
                            redrawn = 1;
                        }
                    } else if (spelltarget_type == MAP_TARGET_ENEMY_UNIT || spelltarget_type == MAP_TARGET_ENEMY_STACK) {
                        px = map_cursor_cell_x * MAP_TILE_W;
                        py = map_cursor_cell_y * MAP_TILE_H + MAP_VIEW_TOP;
                        UI_Clear_Help();
                        Screen_Stack_Popup(t, 0, px, py);
                        UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
                        SpellTarget_Alloc();
                        SpellTarget_Build_Prompt(spelltarget_type, spell_name);
                        SpellTarget_Build_Cursor_Map();
                        Map_Invalidate();
                        Minimap_Build_View();
                        UI_Clear_Help();
                        Help_Load_Spell_Target();
                        redrawn = 1;
                    }
                } else if (t < GRID_NODE_BASE) {
                    found = 0;
                    t = t - GRID_CITY_BASE;
                    switch (spelltarget_type) {
                    case MAP_TARGET_OWN_CITY:
                        if (p_cities[t].owner == current_player)
                            found = 1;
                        break;
                    case MAP_TARGET_ENEMY_CITY:
                        if (p_cities[t].owner != current_player)
                            found = 1;
                        break;
                    case MAP_TARGET_OWN_UNIT:
                    case MAP_TARGET_OWN_STACK:
                        if (p_cities[t].owner == current_player)
                            found = 1;
                        break;
                    case MAP_TARGET_ENEMY_UNIT:
                    case MAP_TARGET_ENEMY_STACK:
                        if (p_cities[t].owner != current_player)
                            found = 1;
                        break;
                    }
                    if (found == 1) {
                        city_screen_city = t;
                        UI_Clear_Help();
                        Screen_City_View();
                        UI_Set_Redraw_Fn(SpellTarget_Redraw, 1);
                        SpellTarget_Alloc();
                        SpellTarget_Build_Prompt(spelltarget_type, spell_name);
                        SpellTarget_Build_Cursor_Map();
                        Map_Invalidate();
                        Minimap_Build_View();
                        UI_Clear_Help();
                        Help_Load_Spell_Target();
                        redrawn = 1;
                    }
                }
            } else {
                map_scroll_target_x += map_cursor_cell_x - MAP_VIEW_COLS / 2;
                map_scroll_target_y += map_cursor_cell_y - MAP_VIEW_ROWS / 2;
                Map_Normalize_Scroll_Target();
            }
        }
        if (done == 0 && redrawn == 0) {
            SpellTarget_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
        redrawn = 0;
    }
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    Map_Invalidate();
    *target = spelltarget_result;
    *y = spelltarget_map_y;
    *plane = spelltarget_plane;
    *screen_x = spelltarget_cur_x * MAP_TILE_W;
    *screen_y = spelltarget_cur_y * MAP_TILE_H + MAP_VIEW_TOP;
    return result;
}

/* () the controls: the top bar's buttons, the map grid and the minimap (only while the view is drawn where it
   stands) and Cancel (key C) */
void SpellTarget_Add_Controls(void)
{
    map_grid_control = UI_NO_CONTROL;
    minimap_control = UI_NO_CONTROL;
    overland_unused_control_d = UI_NO_CONTROL;
    overland_unused_control_c = UI_NO_CONTROL;
    overland_unused_control_b = UI_NO_CONTROL;
    overland_unused_control_a = UI_NO_CONTROL;
    Overland_Add_TopBar_Buttons();
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        map_grid_control = UI_Add_Grid_Control(0, MAP_VIEW_TOP, MAP_TILE_W, MAP_TILE_H, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_cursor_cell_x, &map_cursor_cell_y, -1);
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        minimap_control = UI_Add_Grid_Control(MINIMAP_X, MINIMAP_Y, 1, 1, MINIMAP_W, MINIMAP_H, &minimap_click_x, &minimap_click_y, -1);
    spelltarget_cancel_button = UI_Add_Button(263, 181, "", main_cancel_button_pic, "C", -1);
}

/* () the screen: the map view, the frame, the minimap, the greyed order buttons, the top bar, the reserve, the
   Casting panel, the cursor map and the Cancel button */
void SpellTarget_Redraw(void)
{
    Reset_Clip_Rect();
    Set_Draw_Page();
    Map_Invalidate();
    Map_Draw_View(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_view_x, &map_view_y, map_view_plane, map_scroll_target_x, map_scroll_target_y, current_player);
    Draw_Pic_Frame(0, 0, main_screen_bg_pic);
    Minimap_Draw(MINIMAP_X, MINIMAP_Y, MINIMAP_W, MINIMAP_H);
    Draw_Order_Buttons_Disabled();
    Overland_Draw_TopBar();
    Draw_Gold_Mana_Reserve();
    SpellTarget_Draw_Panel();
    SpellTarget_Build_Cursor_Map();
    Set_Pic_Frame(main_cancel_button_pic, 0);
    Draw_Pic_Frame(263, 181, main_cancel_button_pic);
}

/* (view_x, view_y, &x, &y) -> 1 when map square (x, y) is in the 12 x 10 view (across the x wrap): x / y become its
   screen corner (col * 20, row * 18 + 20), else 0 / 0 */
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y)
{
    int ry;
    int visible;
    int rx;

    visible = 0;
    ry = *y;
    if (ry >= view_y && ry < view_y + MAP_VIEW_ROWS) {
        ry = ry - view_y;
        rx = *x;
        if (rx >= view_x && rx < view_x + MAP_VIEW_COLS) {
            rx -= view_x;
            visible = 1;
        } else {
            rx += MAP_WIDTH;
            if (rx >= view_x && rx < view_x + MAP_VIEW_COLS) {
                rx -= view_x;
                visible = 1;
            }
        }
    }
    if (visible == 1) {
        *x = rx * MAP_TILE_W;
        *y = ry * MAP_TILE_H + MAP_VIEW_TOP;
    } else {
        *x = 0;
        *y = 0;
    }
    return visible;
}

/* () the buffers: the minimap picture (0x12f paragraphs in front of the scratch block), the cursor map (0x618 bytes)
   and the prompt (100) */
void SpellTarget_Alloc(void)
{
    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS);
    p_spell_target_cursor_map = (struct MouseRegion *)Near_Alloc_Reset(0x618);
    spelltarget_prompt = Near_Alloc(100);
}

/* (target type, spell name) "Select a friendly unit as the target for a <Name> spell." (name highlighted) */
void SpellTarget_Build_Prompt(int target_type, char *spell_name)
{
    strcpy(spelltarget_prompt, "Select a");
    switch (target_type) {
    case MAP_TARGET_OWN_UNIT:
        strcat(spelltarget_prompt, " friendly unit ");
        break;
    case MAP_TARGET_OWN_STACK:
        strcat(spelltarget_prompt, " friendly group ");
        break;
    case MAP_TARGET_ENEMY_UNIT:
        strcat(spelltarget_prompt, "n enemy unit ");
        break;
    case MAP_TARGET_ENEMY_STACK:
        strcat(spelltarget_prompt, "n enemy group ");
        break;
    case MAP_TARGET_SQUARE:
        strcat(spelltarget_prompt, " space ");
        break;
    case MAP_TARGET_OWN_CITY:
        strcat(spelltarget_prompt, " friendly city ");
        break;
    case MAP_TARGET_ENEMY_CITY:
        strcat(spelltarget_prompt, "n enemy city ");
        break;
    case MAP_TARGET_NODE:
        strcat(spelltarget_prompt, " magic node ");
        break;
    }
    strcat(spelltarget_prompt, "as the target for ");
    strcat(spelltarget_prompt, Text_Article_A_An(spell_name));
    strcat(spelltarget_prompt, " \002");
    strcat(spelltarget_prompt, spell_name);
    strcat(spelltarget_prompt, "\001 spell.");
}

/* DEAD (no callers): (&x, &y, x, y) a map-view cell kept in x 1..10, y 1..8 */
void SpellTarget_Clamp_Square(int *px, int *py, int x, int y)
{
    *px = x;
    *py = y;
    if (*px == 0)
        (*px)++;
    if (*px == 11)
        (*px)--;
    if (*py == 0)
        (*py)++;
    if (*py == 9)
        (*py)--;
}

/* () no target: result / y / plane -1 */
void SpellTarget_Clear(void)
{
    spelltarget_result = -1;
    spelltarget_map_y = -1;
    spelltarget_plane = -1;
}

/* (stack) target type 1 */
void SpellTarget_Set_Friendly_Stack(int stack)
{
    spelltarget_result = stack;
    spelltarget_map_y = -1;
    spelltarget_plane = -1;
}

/* (city) target types 5 / 6 */
void SpellTarget_Set_City(int city)
{
    spelltarget_result = city;
    spelltarget_map_y = -1;
    spelltarget_plane = -1;
}

/* (stack) target type 3 */
void SpellTarget_Set_Enemy_Stack(int stack)
{
    spelltarget_result = stack;
    spelltarget_map_y = -1;
    spelltarget_plane = -1;
}

/* (node) target type 7 */
void SpellTarget_Set_Node(int node)
{
    spelltarget_result = node;
    spelltarget_map_y = -1;
    spelltarget_plane = -1;
}

/* (cell x, cell y) -> 1 for an explored square (any square for Earth Lore): target type 4, the square's map x / y /
   plane; else all -1 */
int SpellTarget_Set_Square(int x, int y)
{
    int ok;

    ok = 0;
    spelltarget_cur_x = x;
    spelltarget_cur_y = y;
    if (spelltarget_cur_x == 0)
        spelltarget_cur_x++;
    if (spelltarget_cur_x == 11)
        spelltarget_cur_x--;
    if (spelltarget_cur_y == 0)
        spelltarget_cur_y++;
    if (spelltarget_cur_y == 9)
        spelltarget_cur_y--;
    spelltarget_result = map_view_x + x;
    if (spelltarget_result >= MAP_WIDTH)
        spelltarget_result = spelltarget_result - MAP_WIDTH;
    spelltarget_map_y = map_view_y + y;
    spelltarget_plane = map_view_plane;
    if (p_exploration_maps[spelltarget_plane][spelltarget_map_y][spelltarget_result] == 0 &&
        spelltarget_spell != SPELL_EARTH_LORE) {
        spelltarget_result = -1;
        spelltarget_map_y = -1;
        spelltarget_plane = -1;
    } else
        ok = 1;
    return ok;
}

/* () the mouse cursor map: the arrow on the whole screen, then per view square the animated target cursor where the
   square is a valid target (else image 3); also steps the cursor's animation frame */
void SpellTarget_Build_Cursor_Map(void)
{
    int col;
    int row;
    int x;
    int y;
    int x1;
    int y1;
    int x2;
    int y2;
    int garrison_count;
    int garrison[MAX_STACK];
    register int region;
    register int grid_value;

    if (spelltarget_type == MAP_TARGET_NODE)
        Map_Grid_Add_Nodes(map_view_x, map_view_y, map_view_plane);
    p_spell_target_cursor_map->image = POINTER_HAND;
    p_spell_target_cursor_map->pointer_offset = 0;
    p_spell_target_cursor_map->x1 = 0;
    p_spell_target_cursor_map->y1 = 0;
    p_spell_target_cursor_map->x2 = 319;
    p_spell_target_cursor_map->y2 = 199;
    spelltarget_cursor = spelltarget_cursor_frame + POINTER_SPELL_ANIM;   /* the wand's sparkle, 9 .. 13 */
    region = 1;
    for (row = 0; row < 10; row++) {
        for (col = 0; col < 12; col++) {
            x1 = col * MAP_TILE_W;
            y1 = row * MAP_TILE_H + MAP_VIEW_TOP;
            x2 = x1 + (MAP_TILE_W - 1);
            y2 = y1 + (MAP_TILE_H - 1);
            p_spell_target_cursor_map[region].pointer_offset = 0;
            p_spell_target_cursor_map[region].x1 = x1;
            p_spell_target_cursor_map[region].y1 = y1;
            p_spell_target_cursor_map[region].x2 = x2;
            p_spell_target_cursor_map[region].y2 = y2;
            if (spelltarget_type == MAP_TARGET_SQUARE) {
                p_spell_target_cursor_map[region].image = spelltarget_cursor;
                x = map_view_x + col;
                if (x >= MAP_WIDTH)
                    x = x - MAP_WIDTH;
                y = map_view_y + row;
                if (p_exploration_maps[map_view_plane][y][x] == 0 && spelltarget_spell != SPELL_EARTH_LORE)
                    p_spell_target_cursor_map[region].image = POINTER_REFUSED;
            } else {
                p_spell_target_cursor_map[region].image = POINTER_REFUSED;
                grid_value = map_unit_grid[row][col];
                if (grid_value != -1) {
                    grid_value = abs(grid_value);
                    if (grid_value < GRID_CITY_BASE) {
                        switch (spelltarget_type) {
                        case MAP_TARGET_OWN_UNIT:
                        case MAP_TARGET_OWN_STACK:
                            if (p_units[grid_value].owner == current_player)
                                p_spell_target_cursor_map[region].image = spelltarget_cursor;
                            else
                                p_spell_target_cursor_map[region].image = POINTER_REFUSED;
                            break;
                        case MAP_TARGET_ENEMY_UNIT:
                        case MAP_TARGET_ENEMY_STACK:
                            if (p_units[grid_value].owner != current_player)
                                p_spell_target_cursor_map[region].image = spelltarget_cursor;
                            else
                                p_spell_target_cursor_map[region].image = POINTER_REFUSED;
                            break;
                        }
                    } else if (grid_value < GRID_NODE_BASE) {
                        grid_value = grid_value - GRID_CITY_BASE;
                        switch (spelltarget_type) {
                        case MAP_TARGET_OWN_CITY:
                            if (p_cities[grid_value].owner == current_player)
                                p_spell_target_cursor_map[region].image = spelltarget_cursor;
                            break;
                        case MAP_TARGET_ENEMY_CITY:
                            if (p_cities[grid_value].owner != current_player)
                                p_spell_target_cursor_map[region].image = spelltarget_cursor;
                            break;
                        case MAP_TARGET_OWN_UNIT:
                        case MAP_TARGET_OWN_STACK:
                            if (p_cities[grid_value].owner == current_player) {
                                City_Garrison_List(grid_value, &garrison_count, garrison);
                                if (garrison_count > 0)
                                    p_spell_target_cursor_map[region].image = spelltarget_cursor;
                            }
                            break;
                        case MAP_TARGET_ENEMY_UNIT:
                        case MAP_TARGET_ENEMY_STACK:
                            if (p_cities[grid_value].owner != current_player) {
                                City_Garrison_List(grid_value, &garrison_count, garrison);
                                if (garrison_count > 0)
                                    p_spell_target_cursor_map[region].image = spelltarget_cursor;
                            }
                        }
                    } else if (spelltarget_type == MAP_TARGET_NODE)
                        p_spell_target_cursor_map[region].image = spelltarget_cursor;
                }
            }
            region++;
        }
    }
    Mouse_Set_Cursor_Regions(region, p_spell_target_cursor_map);
    if (spelltarget_type == MAP_TARGET_NODE)
        Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    spelltarget_cursor_frame = (spelltarget_cursor_frame + 1) % 5;
}

/* () the Casting panel with the prompt; also steps the panel's two animation counters */
void SpellTarget_Draw_Panel(void)
{
    char glyphs[8];                             /* filled, never used */
    int glyph0;
    int k;
    char colors[4];
    register int frames;

    Draw_Pic_Frame(242, 173, button_blockout_pic);
    glyph0 = '|';
    frames = 4;
    for (k = 0; k < frames; k++)
        glyphs[k] = glyph0 + k;
    Draw_Pic_Frame(240, 76, cast_bg_pic);
    Draw_Pic_Frame(240, 173, cast_button_border_pic);
    colors[0] = 8;
    colors[1] = 0xa1;
    colors[2] = 0xa1;
    Font_Select_Palette(1, colors);
    Font_Select_Shadow(4, 0, 0, 0);
    Set_Text_Shadow_Color(0x94);
    Print_Text_Centered(280, 80, "Casting");
    Font_Select_Shadow(1, 0, 15, 15);
    Set_Text_Shadow_Color(0x94);
    spelltarget_dash_phase--;
    if (spelltarget_dash_phase < 0)
        spelltarget_dash_phase = frames - 1;
    spelltarget_anim_step++;
    if (spelltarget_anim_step == frames)
        spelltarget_anim_step = 0;
    Paragraph_Print(249, 114, 60, spelltarget_prompt, PARA_LEFT);
    Set_Text_Shadow_Color(0);
}
