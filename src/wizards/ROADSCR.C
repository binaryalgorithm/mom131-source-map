/* ROADSCR.C - WIZARDS.EXE overlay code segment 5b8b (stub 2459): the Road Building screen (main-loop screen 7): the
   map with the planned road drawn as an animated dashed line from the stack to the clicked square (a straight
   Map_Line_Path of up to 10 squares), the panel with the turns it takes ("Blocked." across water), Cancel / OK; OK
   orders the stack's engineers to build it (Units_Order_Build_Road) and sends the stack on its way.
   This module's data (DS:327e): its literals; BSS (DS:c128..c13e).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 5b8b src/wizards/ROADSCR.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Screen_Road_Building(void);
void RoadScr_Add_Controls(void);
void RoadScr_Draw(void);
void RoadScr_Draw_Panel(void);
void RoadScr_Draw_Path(void);
int RoadScr_Plan(int x, int y);
int Stack_Min_Max_Moves(void);
void RoadScr_Collect_Engineers(int *n, int *list);
int Units_Order_Build_Road(int n, int *units, int dest_x, int dest_y);

#include "GAME.H"

#define ROAD_MAX_SQUARES 10             /* the road planner: at most 10 squares, 120 turns of work */
#define ROAD_MAX_TURNS 120

int road_start_x;                               /* DS:c13c  the stack's square, where the road starts */
int road_start_y;                               /* DS:c13a */
int road_end_x;                                 /* DS:c138  the planned road's last square */
int road_end_y;                                 /* DS:c136 */
int road_build_turns;                           /* DS:c134  turns to build it; -1 blocked (water) */
int road_path_len;                              /* DS:c132  squares in path_x / path_y (at most 10) */
int map_tool_cancel_control;                        /* DS:c130  the Cancel button (also the surveyor screen's) */
int road_dash_phase;                            /* DS:c12e  the dashed line's phase, counts 3 .. 0 */
int road_ok_button;                             /* DS:c12c */
int road_engineer_unit;                         /* DS:c12a  the last engineer counted (written, never read) */
int road_engineer_power;                        /* DS:c128  the stack's summed road power (+1 per Endurance) */

extern unsigned road_button_border_pic;         /* DS:951a */
extern unsigned road_bg_pic;                    /* DS:951c */
extern unsigned road_ok_pic;                    /* DS:951e */
extern unsigned button_blockout_pic;            /* DS:9522 */
extern unsigned main_cancel_button_pic;         /* DS:9526 */
extern int minimap_control;                     /* DS:973e */
extern int map_grid_control;                    /* DS:9740 */
extern int minimap_click_y;                     /* DS:9742 */
extern int minimap_click_x;                     /* DS:9744 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int map_cursor_cell_y;                   /* DS:974e */
extern int map_cursor_cell_x;                   /* DS:9750 */
extern unsigned main_screen_bg_pic;             /* DS:9922 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern int overland_unused_control_a;           /* DS:c046 */
extern int overland_unused_control_b;           /* DS:c048 */
extern int overland_unused_control_c;           /* DS:c04a */
extern int overland_unused_control_d;           /* DS:c04c */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern char path_y[];                           /* DS:c668 */
extern char path_x[];                           /* DS:c6e0 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
int Tile_Is_Water(int x, int y, int plane);
int Road_Build_Turns(int x, int y, int plane);
int Unit_Has_Endurance(int unit);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Minimap_Build_View(void);
void Help_Load_Road_Building(void);
void UI_Input_Delay(int delay);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
int Stack_Continue_GoTo(int player, int lead_unit, int dest_x, int dest_y, int *view_x, int *view_y, int plane);
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
void Mouse_Default_Cursor(void);
void Map_Normalize_Scroll_Target(void);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty,
                   int player);
void Minimap_Draw(int x, int y, int w, int h);
void Overland_Draw_TopBar(void);
void Draw_Gold_Mana_Reserve(void);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Draw_Line_Dashed(int x1, int y1, int x2, int y2, char *pattern, int length, int phase);
int Map_Line_Path(int x1, int y1, int x2, int y2, char *xs, char *ys, int width);

/* () -> 0 when the stack stands on water (no return value otherwise): the road-building screen for the selected
   stack until Cancel or OK; OK with a buildable road orders the engineers (Units_Order_Build_Road) and moves the
   stack (Stack_Continue_GoTo / Next_Unit_Run_GoTos); both return to the main screen (5)
   Numbered: BUG #176 (BUGS_AND_QUIRKS.txt) */
int Screen_Road_Building(void)
{
    int done;
    int hit;                                    /* read, never used */
    int slot;
    int cx;
    int cy;
    int engineers;
    int turns;
    int units[MAX_STACK];
    register int unit;
    register int input;

    road_path_len = 0;
    road_build_turns = 0;
    road_dash_phase = 0;
    road_engineer_power = 0;
    unit = stack_units[0].unit;
    road_start_x = p_units[unit].x;
    road_start_y = p_units[unit].y;
    if (Tile_Is_Water(road_start_x, road_start_y, p_units[unit].plane) == 1)
        return 0;
    road_engineer_unit = unit;
    for (slot = 0; slot < stack_unit_count; slot++) {
        unit = stack_units[slot].unit;
        if (unit_types[p_units[unit].type].construction > 0) {
            road_engineer_power = road_engineer_power + unit_types[p_units[unit].type].construction;
            road_engineer_unit = unit;
            if (Unit_Has_Endurance(unit) == 1) {
                road_engineer_power++;
                road_engineer_unit = unit;
            }
        }
    }
    turns = Road_Build_Turns(road_start_x, road_start_y, map_view_plane);
    if (road_engineer_power > 0) {
        turns = turns / road_engineer_power;
        if (turns < 1)
            turns = 1;
    }
    road_build_turns = turns;
    road_end_x = road_start_x;
    road_end_y = road_start_y;
    Stack_Blink_Reset();
    Set_Text_Shadow_Color(0);
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(RoadScr_Draw, 1);
    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS_SMALL);
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Map_Invalidate();
    Minimap_Build_View();
    UI_Clear_Help();
    Help_Load_Road_Building();
    UI_Input_Delay(1);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        RoadScr_Add_Controls();
        input = UI_Get_Input();
        hit = UI_Hit_Test();
        if (input == map_tool_cancel_control) {
            Play_Click_Sound();
            current_screen = SCREEN_MAP;
            done = -1;
        }
        if (input == road_ok_button) {
            Play_Click_Sound();
            if (road_build_turns > 0) {
                UI_Clear_Controls();
                UI_Suspend_Redraw_Fn();
                RoadScr_Collect_Engineers(&engineers, units);
                if (engineers > 0) {
                    Units_Order_Build_Road(engineers, units, road_end_x, road_end_y);
                    Stack_Continue_GoTo(current_player, units[0], road_end_x, road_end_y, &map_view_x, &map_view_y,
                                        map_view_plane);
                    Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
                }
            }
            current_screen = SCREEN_MAP;
            done = -1;
        }
        if (input == map_grid_control) {
            Play_Click_Sound();
            cx = (map_view_x + map_cursor_cell_x) % MAP_WIDTH;
            cy = map_view_y + map_cursor_cell_y;
            Map_Invalidate();
            RoadScr_Plan(cx, cy);
        }
        if (input == minimap_control) {
            Play_Click_Sound();
            Map_Window_Origin(&cx, &cy, (map_view_x + MAP_VIEW_COLS / 2) % MAP_WIDTH, map_view_y + MAP_VIEW_ROWS / 2, MINIMAP_W, MINIMAP_H);
            map_scroll_target_x = minimap_click_x + cx;
            map_scroll_target_y = minimap_click_y + cy;
            map_view_x = map_scroll_target_x;
            map_view_y = map_scroll_target_y;
            Map_Center_View(&map_view_x, &map_view_y, map_scroll_target_x, map_scroll_target_y, map_view_plane);
            Minimap_Build_View();
            Mouse_Default_Cursor();
            Map_Invalidate();
        }
        if (input == -map_grid_control) {
            map_scroll_target_x += map_cursor_cell_x - MAP_VIEW_COLS / 2;
            map_scroll_target_y += map_cursor_cell_y - MAP_VIEW_ROWS / 2;
            Map_Normalize_Scroll_Target();
        }
        if (done == 0) {
            RoadScr_Draw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
}

/* () the map grid (12 x 10 squares of 20 x 18 at 0,20) and the minimap (58 x 30 at 251,21) - only while the view
   is drawn where it stands -, Cancel (280,181, key C) and OK (246,181, key O) */
void RoadScr_Add_Controls(void)
{
    map_grid_control = UI_NO_CONTROL;
    minimap_control = UI_NO_CONTROL;
    road_ok_button = UI_NO_CONTROL;
    overland_unused_control_d = UI_NO_CONTROL;
    overland_unused_control_c = UI_NO_CONTROL;
    overland_unused_control_b = UI_NO_CONTROL;
    overland_unused_control_a = UI_NO_CONTROL;
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        map_grid_control = UI_Add_Grid_Control(0, MAP_VIEW_TOP, MAP_TILE_W, MAP_TILE_H, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_cursor_cell_x, &map_cursor_cell_y, -1);
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        minimap_control = UI_Add_Grid_Control(MINIMAP_X, MINIMAP_Y, 1, 1, MINIMAP_W, MINIMAP_H, &minimap_click_x, &minimap_click_y, -1);
    map_tool_cancel_control = UI_Add_Button(280, 181, "", main_cancel_button_pic, "C", -1);
    road_ok_button = UI_Add_Button(246, 181, "", road_ok_pic, "O", -1);
}

/* () the redraw: map view, main background, minimap, top bar, gold / mana, the panel and the planned road */
void RoadScr_Draw(void)
{
    Reset_Clip_Rect();
    Set_Draw_Page();
    Map_Invalidate();
    Map_Draw_View(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_view_x, &map_view_y, map_view_plane, map_scroll_target_x, map_scroll_target_y, current_player);
    Draw_Pic_Frame(0, 0, main_screen_bg_pic);
    Minimap_Draw(MINIMAP_X, MINIMAP_Y, MINIMAP_W, MINIMAP_H);
    Overland_Draw_TopBar();
    Draw_Gold_Mana_Reserve();
    RoadScr_Draw_Panel();
    RoadScr_Draw_Path();
}

/* () the side panel: "Road Building" and "Blocked. ..." or "It will take N turn(s) to complete ..." */
void RoadScr_Draw_Panel(void)
{
    char colors[4];

    Draw_Pic_Frame(242, 173, button_blockout_pic);
    Draw_Pic_Frame(240, 76, road_bg_pic);
    Draw_Pic_Frame(240, 173, road_button_border_pic);
    Font_Select_Shadow(1, 0, 0, 0);
    Set_Text_Shadow_Color(0xa6);
    colors[0] = 8;
    colors[1] = 0xa1;
    colors[2] = 0xa1;
    Font_Select_Palette(1, colors);
    Font_Select_Shadow(4, 0, 0, 0);             /* replaces the colour set just loaded: "Road Building" prints in set
                                                   0; clarified using ReMoM repo */
    Set_Text_Shadow_Color(0x94);
    Print_Text_Centered(280, 80, "Road");
    Print_Text_Centered(280, 90, "Building");
    colors[0] = 8;
    colors[1] = 0xa1;
    colors[2] = 0xa1;
    Font_Select_Palette(1, colors);
    Font_Select_Shadow(1, 15, 0, 0);
    Set_Text_Shadow_Color(0x94);
    strcpy(p_text_buf, "");
    if (road_build_turns == -1)
        strcpy(p_text_buf, "Blocked.  Roads can not be built across large bodies of water.");
    else if (road_build_turns > 0) {
        strcpy(p_text_buf, "It will take ");
        itoa(road_build_turns, p_text_buf2, 10);
        strcat(p_text_buf, p_text_buf2);
        strcat(p_text_buf, " turn");
        if (road_build_turns > 1)
            strcat(p_text_buf, "s");
        strcat(p_text_buf, " to complete the construction of this road.");
    }
    Paragraph_Print(250, 106, 60, p_text_buf, PARA_LEFT);
}

/* () the planned road as an animated dashed line (colors 200.. when blocked, else 108..) from the start square
   through path_x / path_y, square centres (x * 20 + 10, y * 18 + 29) with the x wrap; no segment is drawn to
   column 29; the dash phase steps 3 -> 0 */
void RoadScr_Draw_Path(void)
{
    int k;
    int y2;
    int x1;
    int y1;
    int dy;
    int dx0;
    int dy0;
    char pattern[8];
    int count;
    int color_base;
    int ox;
    int oy;
    int skip;
    register int x;
    register int x2;

    Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
    if (road_build_turns == -1)
        color_base = 200;
    else
        color_base = 108;
    count = 4;
    for (k = 0; k < count; k++)
        pattern[k] = color_base + k;
    ox = 10;
    oy = 9;
    dx0 = road_start_x - map_view_x;
    if (dx0 < -(MAP_WIDTH / 2))
        dx0 = dx0 + MAP_WIDTH;
    dy0 = road_start_y - map_view_y;
    x1 = dx0 * MAP_TILE_W;
    y1 = dy0 * MAP_TILE_H + MAP_VIEW_TOP;
    x1 += ox;
    y1 += oy;
    for (k = 0; k < road_path_len; k++) {
        x = path_x[k];
        dy = path_y[k];
        x -= map_view_x;
        if (x < -(MAP_WIDTH / 2))
            x = x + MAP_WIDTH;
        skip = 0;
        if (x == 29)                            /* half the map away: the square across the wrap seam,
                                                   no line to it: a step from -30 to -31 lands here after the
                                                   + MAP_WIDTH above, so the line would cross the view */
            skip = 1;
        dy -= map_view_y;
        x2 = x * MAP_TILE_W;
        y2 = dy * MAP_TILE_H + MAP_VIEW_TOP;
        x2 += ox;
        y2 += oy;
        if (skip == 0)
            Draw_Line_Dashed(x1, y1, x2, y2, pattern, count, road_dash_phase);
        x1 = x2;
        y1 = y2;
    }
    road_dash_phase--;
    if (road_dash_phase < 0)
        road_dash_phase = count - 1;
    Reset_Clip_Rect();
}

/* (x, y) -> 0 on an unexplored square (no return value otherwise): plans the straight road to (x, y) - at most 10
   squares, its end square, and the turns: the start square's, then each square's (each divided by the engineers'
   power, at least 1); a square that cannot take a road ends the path there and blocks it (-1); the path stops
   before the total would pass 120
   Numbered: BUG #30 (BUGS_AND_QUIRKS.txt) */
int RoadScr_Plan(int x, int y)
{
    int status;                                 /* written, never read */
    int moves;                                  /* written, never read */
    int moves2;                                 /* written, never read */
    int turns;
    int i;

    if (p_exploration_maps[map_view_plane][y][x] == 0)
        return 0;
    road_path_len = Map_Line_Path(road_start_x, road_start_y, x, y, path_x, path_y, MAP_WIDTH);
    if (road_path_len > ROAD_MAX_SQUARES)
        road_path_len = ROAD_MAX_SQUARES;
    if (road_path_len > 0) {
        road_end_x = path_x[road_path_len - 1];
        road_end_y = path_y[road_path_len - 1];
    } else {
        road_end_x = road_start_x;
        road_end_y = road_start_y;
    }
    moves = Stack_Min_Max_Moves();
    moves2 = moves;
    status = 0;
    road_build_turns = 0;
    turns = Road_Build_Turns(road_start_x, road_start_y, map_view_plane);
    if (road_engineer_power > 0) {
        turns = turns / road_engineer_power;
        if (turns < 1)
            turns = 1;
    }
    road_build_turns = turns;
    for (i = 0; i < road_path_len; i++) {
        turns = Road_Build_Turns(path_x[i], path_y[i], map_view_plane);
        if (turns < 0) {
            road_path_len = i + 1;
            status = -1;
            road_build_turns = -1;
            return;
        }
        if (road_engineer_power > 0) {
            turns = turns / road_engineer_power;
            if (turns < 1)
                turns = 1;
        }
        if (road_build_turns + turns > ROAD_MAX_TURNS) {
            road_path_len = i;
            return;
        } else
            road_build_turns += turns;
    }
}

/* () -> the lowest moves_max among the stack's active units that have not finished; 0 when there are none */
int Stack_Min_Max_Moves(void)
{
    int slot;
    int min;
    int unit;

    min = MIN_NOT_FOUND;
    for (slot = 0; slot < stack_unit_count; slot++) {
        unit = stack_units[slot].unit;
        if (stack_units[slot].active == 1 && p_units[unit].finished == 0 && p_units[unit].moves_max < min)
            min = p_units[unit].moves_max;
    }
    if (min == MIN_NOT_FOUND)
        min = 0;
    return min;
}

/* (&n, list) the stack's active engineers that can still move */
void RoadScr_Collect_Engineers(int *n, int *list)
{
    int count;
    register int slot;
    register int unit;

    count = 0;
    for (slot = 0; slot < stack_unit_count; slot++) {
        unit = stack_units[slot].unit;
        if (stack_units[slot].active != 0 && p_units[unit].moves_left > 0 && p_units[unit].finished == 0
            && unit_types[p_units[unit].type].construction > 0) {
            list[count] = unit;
            count++;
        }
    }
    *n = count;
}

/* (n, units, dest x, dest y) -> 0 when the first unit stands on water (no return value otherwise): every unit goes
   to the destination (status 3, Go-To); each engineer also gets road_turns = Road_Build_Turns of its own square /
   road_engineer_power (the whole stack's power - #176), at least 1, its road start = its square and plane =
   map_view_plane (moves a tower stack to the viewed plane)
   Numbered: BUG #311 (BUGS_AND_QUIRKS.txt) */
int Units_Order_Build_Road(int n, int *units, int dest_x, int dest_y)
{
    int k;
    int len;                                    /* written, never read */
    int x;
    int y;
    int unit;

    x = p_units[units[0]].x;
    y = p_units[units[0]].y;
    len = Map_Line_Path(x, y, dest_x, dest_y, path_x, path_y, MAP_WIDTH);
    if (Tile_Is_Water(x, y, map_view_plane) == 1)
        return 0;
    for (k = 0; k < n; k++) {
        unit = units[k];
        p_units[unit].order = ORDER_GOTO;
        p_units[unit].dest_x = dest_x;
        p_units[unit].dest_y = dest_y;
        if (unit_types[p_units[unit].type].construction > 0) {
            p_units[unit].road_turns = Road_Build_Turns(p_units[unit].x, p_units[unit].y, map_view_plane);
            if (road_engineer_power > 0)
                p_units[unit].road_turns = p_units[unit].road_turns / road_engineer_power;
            if (p_units[unit].road_turns < 1)
                p_units[unit].road_turns = 1;
            p_units[unit].road_start_x = p_units[unit].x;
            p_units[unit].road_start_y = p_units[unit].y;
            p_units[unit].plane = map_view_plane;
        }
    }
}
