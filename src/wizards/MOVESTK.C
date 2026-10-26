/* MOVESTK.C - WIZARDS.EXE overlay code segment 7b88 (stub 24e4): moving a stack on the overland map - the core move
   (the path by Path_Find, a road builder's straight line or AI road path with its build counters, the walk check,
   the animation, lairs and attacks, the moves paid and the Go-To kept or cleared), the animation along the path
   (the view following, the road laid on each square left), the contacts a moving stack makes with other wizards'
   units and cities, the obstacles a path must avoid, the units that need a transport, and putting the passengers
   of a transport with moves on Patrol.
   No data of its own; BSS (DS:c5dc..c794): the move globals and the path arrays.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7b88 src/wizards/MOVESTK.C */

/* the module's functions in their original declaration order */
int Stack_Move(int player, int dest_x, int dest_y, int mode, int *view_x, int *view_y, int plane, int n, int *units);
void Stack_Animate_Path(int player, int plane, int path_len, int *view_x, int *view_y, int *units, int n);
int Move_Scout_Contacts(int player, int x, int y, int plane, int range);
void Path_Apply_Obstacles(struct PathWork far *work, int needs_transport, int stack_size, int plane, int player,
                          int x2, int y2, int x1, int y1);
int Stack_Needs_Transport(int n, int *units, int *needing_ride);
int Stack_Board_Transport(int n, int *units);

#include "GAME.H"

char move_unused_c758[60];                      /* DS:c758  never referenced */
char path_x[120];                               /* DS:c6e0  the path's squares, start excluded */
char path_y[120];                               /* DS:c668 */
char path_cost[120];                            /* DS:c5f0  move cost of each step (road builders: turns) */
int stack_move_attacker;                        /* DS:c5ee  the moving stack's first unit in a fight */
int stack_move_target_unit;                     /* DS:c5ec  the unit it attacks */
char move_unused_c5e2[10];                      /* DS:c5e2  never referenced */
int move_retreat_x;                                /* DS:c5e0  the square before the last one walked */
int move_retreat_y;                                /* DS:c5de */
int stack_move_ward_blocked;                    /* DS:c5dc  1: a Spell Ward stopped the walk */

extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286  the city / lair attacked */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern int first_move_pending;                     /* DS:c044 */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */

void Stack_Blink_Hide(void);
int Stack_Moves_Available(int *units, int n);
void Stack_Movement_Modes(int *modes, int *units, int count);
int Path_Find(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying, int x1, int y1,
              int x2, int y2, int plane, unsigned char *path_x, unsigned char *path_y, unsigned char *path_cost,
              int window, int mode, int needs_transport, int stack_size, int player);
int Map_Line_Path(int x1, int y1, int x2, int y2, char *xs, char *ys, int width);
int Road_Path_AI(int x1, int y1, int x2, int y2, int plane, char *xs, char *ys);
int Unit_Has_Endurance(int unit);
int Road_Build_Turns(int x, int y, int plane);
void Road_Build(int x, int y, int plane);
void City_Road_Connections(int x, int y, int plane);
void Path_Walk_Check(int player, char *path_x, char *path_y, int plane, char *path_cost, int moves, int *target,
                     int *attack, int *length, int *incomplete, int *units, int n);
void Ward_Blocked_Message(void);
int Lair_Enter(int lair, int player);
int Overland_Attack(int attacker, int target, int n, int *units);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
int Stack_Tower_Update(int count, int *list, int plane);
int Tile_Visible(int x, int y, int plane);
int Unit_Is_Invisible(int unit);
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane);
int Map_View_Nudge_Edge(int *view_x, int *view_y, int x, int y, int width, int height);
void Minimap_Build_View(void);
void Overland_Screen_Draw(int *vx, int *vy, int plane, int tx, int ty, int player);
void Screen_Copy_Front(void);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
int Map_Explore_Radius(int x, int y, int plane, int range);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty, int player);
void Map_Redraw_Block(int px, int py, int plane, int vx, int vy);
void Map_Draw_Cities_Move_Anim(int scr_x, int scr_y, int cols, int rows, int view_x, int view_y, int plane);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void Map_Draw_Fog(int px, int py, int w, int h, int vx, int vy, int plane);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int Hero_Item_Water_Walking(int unit);
int Unit_Can_Swim(int unit);
int Unit_Can_Cross_Water(int unit);

/* (player, dest_x, dest_y, mode, &view_x, &view_y, plane, n, units) -> 1 moved, 0 no moves / no path: the core
   overland move. Modes 0 / 3: Path_Find with the stack's movement modes; mode 2 (road building): the human's straight
   line or the AI's road path from the road start, cut after the stack's square, each step costing Road_Build_Turns /
   the engineers' power (Endurance +1, at least 1); an engineer whose counter ran out builds here at the path's end
   and stops, else restarts its counter and steps; a running counter counts down (BUG #469: the path length is set per
   unit, so the last unit decides). Then the walk check, the animation (roads laid), the ward message, a lair entered
   (0: back a square, 1: the survivors, 99: all back) or an attack; every unfinished unit not on Patrol pays the moves,
   a human Go-To is dropped after a fight, an incomplete walk gives every moving unit a Go-To to
   (dest_x, dest_y) - a click move becomes a Go-To; not over a Go-To-enemy order -, an arrival makes the unit ready again
   Numbered: BUG #176, #317 (BUGS_AND_QUIRKS.txt) */
int Stack_Move(int player, int dest_x, int dest_y, int mode, int *view_x, int *view_y, int plane, int n, int *units)
{
    int attacker;
    int path_len;
    int sx;
    int sy;
    int road_x;
    int road_y;
    int start_idx;
    int fight;
    int target_unit;
    int moves_available;
    int incomplete;
    int moves_used;
    int start_plane;                            /* set, never used */
    int lair_flag;                              /* never set: always 0 */
    int needs_transport;
    int last_engineer;                          /* set, never used */
    int eng_power;
    int modes[9];                               /* 6 used (MOVE_MODE_*), 8 cleared */
    int boat_list[MAX_STACK];
    int i;
    int unit;

    unit = units[0];
    sx = p_units[unit].x;
    sy = p_units[unit].y;
    start_plane = p_units[unit].plane;
    lair_flag = 0;
    Stack_Blink_Off();
    Map_Invalidate();
    moves_available = Stack_Moves_Available(units, n);
    if (moves_available < 1) {
        Stack_Blink_Reset();
        return 0;
    }
    for (i = 0; i < 8; i++)
        modes[i] = 0;
    Stack_Movement_Modes(modes, units, n);
    needs_transport = Stack_Needs_Transport(n, units, boat_list);
    switch (mode) {
    case STACK_MOVE_NORMAL:
    case STACK_MOVE_UNUSED_3:
        path_len = Path_Find(modes[MOVE_MODE_CAVALRY], modes[MOVE_MODE_FORESTER], modes[MOVE_MODE_MOUNTAINEER], modes[MOVE_MODE_SWIMMING], modes[MOVE_MODE_SAILING], modes[MOVE_MODE_FLYING], sx, sy, dest_x, dest_y, plane,
                             path_x, path_y, path_cost, 1, moves_available, needs_transport, n, player);
        break;
    case STACK_MOVE_ROAD:
        road_x = p_units[unit].road_start_x;
        road_y = p_units[unit].road_start_y;
        if (player == current_player)
            path_len = Map_Line_Path(road_x, road_y, dest_x, dest_y, path_x, path_y, MAP_WIDTH);
        else
            path_len = Road_Path_AI(road_x, road_y, dest_x, dest_y, plane, path_x, path_y);
        start_idx = -1;
        for (i = 0; i < path_len; i++) {
            if (path_x[i] == sx && path_y[i] == sy) {
                start_idx = i;
                break;
            }
        }
        if (start_idx != -1) {
            path_len = path_len - start_idx - 1;
            for (i = 0; i < path_len; i++) {
                path_x[i] = path_x[start_idx + i + 1];
                path_y[i] = path_y[start_idx + i + 1];
            }
        }
        eng_power = 0;
        for (i = 0; i < n; i++) {
            unit = units[i];
            if (unit_types[p_units[unit].type].construction > 0) {
                eng_power = eng_power + unit_types[p_units[unit].type].construction;
                last_engineer = unit;
                if (Unit_Has_Endurance(unit) == 1) {
                    last_engineer = unit;
                    eng_power++;
                }
            }
        }
        for (i = 0; i < path_len; i++) {
            path_cost[i] = Road_Build_Turns(path_x[i], path_y[i], plane);
            if (eng_power > 0)
                path_cost[i] = path_cost[i] / eng_power;
            if (path_cost[i] == 0)
                path_cost[i] = 1;
        }
        start_idx = 1;
        for (i = 0; i < n; i++) {
            if (p_units[units[i]].road_turns > 0) {
                start_idx = 0;
                p_units[units[i]].finished = 1;
                p_units[units[i]].road_turns = p_units[units[i]].road_turns - 1;
            } else if (path_len == 0) {
                start_idx = 0;
                p_units[units[i]].road_turns = -1;
                p_units[units[i]].order = ORDER_READY;
                Road_Build(p_units[units[i]].x, p_units[units[i]].y, plane);
                City_Road_Connections(p_units[units[i]].x, p_units[units[i]].y, plane);
            } else {
                p_units[units[i]].road_turns = path_cost[0];
                start_idx = 1;
            }
            if (start_idx == 1)
                path_len = 1;
            else
                path_len = 0;
        }
        break;
    }
    if (path_len < 1) {
        Stack_Blink_Reset();
        return 0;
    }
    incomplete = 0;
    fight = 0;
    stack_move_ward_blocked = 0;
    Path_Walk_Check(player, path_x, path_y, plane, path_cost, moves_available, &target_unit, &fight, &path_len,
                    &incomplete, units, n);
    moves_used = 0;
    for (i = 0; i < path_len; i++)
        moves_used = moves_used + path_cost[i];
    if (path_len > 1) {
        move_retreat_x = path_x[path_len - 2];
        move_retreat_y = path_y[path_len - 2];
    } else {
        move_retreat_x = sx;
        move_retreat_y = sy;
    }
    if (path_len > 0)
        Stack_Animate_Path(player, plane, path_len, view_x, view_y, units, n);
    else {
        moves_used = 0;
        incomplete = 0;
    }
    if (stack_move_ward_blocked == 1 && player == current_player)
        Ward_Blocked_Message();
    if (fight == 1 || lair_flag == 1) {
        attacker = unit;
        first_move_pending = 0;
        stack_move_attacker = attacker;
        stack_move_target_unit = target_unit;
        if (combat_site == COMBAT_AT_LAIR) {
            switch (Lair_Enter(combat_site_index, player)) {
            case LAIR_ENTER_LOST:
                for (i = 0; i < n; i++) {
                    if (p_units[units[i]].x == combat_loc_x && p_units[units[i]].y == combat_loc_y) {
                        p_units[units[i]].x = move_retreat_x;
                        p_units[units[i]].y = move_retreat_y;
                    }
                }
                break;
            case LAIR_ENTER_WON:
                Tile_Units_Of_Owner(combat_loc_x, combat_loc_y, combat_loc_plane, player, &n, units);
                break;
            case LAIR_ENTER_DECLINED:
                for (i = 0; i < n; i++) {
                    p_units[units[i]].x = move_retreat_x;
                    p_units[units[i]].y = move_retreat_y;
                }
                break;
            }
        } else if (Overland_Attack(player, target_unit, n, units) == 1)
            Tile_Units_Of_Owner(combat_loc_x, combat_loc_y, combat_loc_plane, player, &n, units);
    }
    combat_loc_x = -1;
    combat_loc_y = -1;
    Stack_Tower_Update(n, units, plane);
    for (i = 0; i < n; i++) {
        unit = units[i];
        if (p_units[unit].road_turns == 99)
            p_units[unit].road_turns = -1;
        if (p_units[unit].finished == 0 && p_units[unit].order != ORDER_PATROL) {
            p_units[unit].moves_left = p_units[unit].moves_left - moves_used;
            if (p_units[unit].moves_left < 1) {
                p_units[unit].finished = 1;
                p_units[unit].moves_left = 0;
            }
            if (p_units[unit].order == ORDER_GOTO && fight == 1 && p_units[unit].owner == current_player) {
                p_units[unit].order = ORDER_READY;
                p_units[unit].dest_x = 0;
                p_units[unit].dest_y = 0;
                incomplete = 0;
            }
            if (incomplete == 1 && p_units[unit].order != ORDER_GOTO_ENEMY) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = dest_x;
                p_units[unit].dest_y = dest_y;
                p_units[unit].finished = 1;
            }
            if (p_units[unit].order == ORDER_GOTO && p_units[unit].x == p_units[unit].dest_x
                && p_units[unit].y == p_units[unit].dest_y) {
                if (p_units[unit].road_turns == -1) {
                    p_units[unit].order = ORDER_READY;
                    p_units[unit].dest_x = 0;
                    p_units[unit].dest_y = 0;
                    if (p_units[unit].moves_left > 0)
                        p_units[unit].finished = 0;
                }
                incomplete = 0;
            }
            if (p_units[unit].order == ORDER_GOTO)
                p_units[unit].finished = 1;
        }
    }
    Stack_Board_Transport(n, units);
    Stack_Blink_Reset();
    if (player == current_player)
        Visibility_Rebuild();
    return 1;
}

/* (player, plane, path_len, &view_x, &view_y, units, n) moves the stack along the path: shown when it is the current
   player's or (with 'enemy moves' on) when a step is visible and not every unit invisible - the view centred, then
   each step slid in 5 / 6 frames (2 diagonally) with the view nudged at the edge; the units land on the last square
   at once, a road is laid on the start square; every step makes the scouting contacts (range: the best scout range,
   at least 2 for a flier, Wind Walker or flight item; the water-walker test is
   Unit_Water_Move_Always_0, always 0) */
void Stack_Animate_Path(int player, int plane, int path_len, int *view_x, int *view_y, int *units, int n)
{
    int unit;
    int dx_px;
    int dy_px;
    int end_x;                                  /* first the count of invisible units */
    int end_y;
    int k;
    int cx;
    int cy;
    int sx;
    int sy;
    int nx;
    int ny;
    int road_on;
    int scout;
    int from_sx;
    int from_sy;
    int to_sx;
    int to_sy;
    int frames;
    int prev_x;
    int prev_y;
    int best;
    int visible;
    int i;

    unit = units[0];
    visible = 0;
    if (player == current_player)
        visible = 1;
    if (player != current_player && game_config.enemy_moves == 1) {
        for (i = 0; i < path_len && visible == 0; i++)
            if (Tile_Visible(path_x[i], path_y[i], plane) == 1)
                visible = 1;
        if (visible == 1) {
            end_x = 0;
            for (i = 0; i < n; i++)
                if (Unit_Is_Invisible(units[i]) == 1)
                    end_x++;
            if (end_x == n)
                visible = 0;
        }
    }
    if (visible == 1) {
        Map_View_Ensure_Visible(view_x, view_y, p_units[unit].x, p_units[unit].y, plane);
        map_view_x = *view_x;
        map_view_y = *view_y;
        Units_Set_Draw_Priority();
        Map_Build_Unit_Grid(*view_x, *view_y, plane);
        Map_Invalidate();
        Minimap_Build_View();
        Overland_Screen_Draw(view_x, view_y, plane, *view_x, *view_y, player);
        Screen_Present();
        Screen_Copy_Front();
        Map_Invalidate();
    }
    scout = 1;
    for (i = 0; i < n; i++)
        if (p_units[units[i]].scout_range > scout)
            scout = p_units[units[i]].scout_range;
    if (scout < 2) {
        for (i = 0; i < n; i++) {
            if (Unit_Can_Fly(units[i]) == 1 || Unit_Has_Wind_Walking(units[i]) == 1
                || Unit_Water_Move_Always_0(units[i]) == 1 || Hero_Item_Flight(units[i]) == 1) {
                scout = 2;
                break;
            }
        }
    }
    best = p_units[units[0]].draw_priority;
    k = 0;
    p_units[units[0]].draw_priority = 0;
    for (i = 1; i < n; i++) {
        if (p_units[units[i]].draw_priority > best) {
            best = p_units[units[i]].draw_priority;
            k = i;
        }
        p_units[units[i]].draw_priority = 0;
    }
    unit = units[k];
    cx = p_units[unit].x;
    cy = p_units[unit].y;
    end_x = path_x[path_len - 1];               /* end_x: from here the destination x */
    end_y = path_y[path_len - 1];
    for (i = 0; i < n; i++) {
        p_units[units[i]].x = end_x;
        p_units[units[i]].y = end_y;
    }
    road_on = 0;
    if (p_units[unit].road_turns != -1) {
        if (p_units[unit].road_turns == 99)
            p_units[unit].road_turns = -1;
        road_on = 1;
    }
    Reset_Clip_Rect();
    Set_Draw_Page();
    if (visible == 1) {
        Stack_Blink_Hide();
        Map_Build_Unit_Grid(*view_x, *view_y, plane);
    }
    prev_x = cx;
    prev_y = cy;
    if (road_on == 1) {
        Road_Build(cx, cy, plane);
        City_Road_Connections(cx, cy, plane);
    }
    for (i = 0; i < path_len; i++) {
        nx = path_x[i];
        ny = path_y[i];
        dx_px = 0;
        if (nx > cx) {
            if (cx == 0 && nx == MAP_WIDTH - 1) {
                dx_px = -4;
                cx--;
                if (cx < 0)
                    cx = MAP_WIDTH - 1;
            } else {
                dx_px = 4;
                cx = cx + 1;
                if (cx >= MAP_WIDTH)
                    cx = 0;
            }
        } else if (nx < cx) {
            if (cx == MAP_WIDTH - 1 && nx == 0) {
                dx_px = 4;
                cx = cx + 1;
                if (cx >= MAP_WIDTH)
                    cx = 0;
            } else {
                dx_px = -4;
                cx--;
                if (cx < 0)
                    cx = MAP_WIDTH - 1;
            }
        }
        dy_px = 0;
        if (ny > cy) {
            dy_px = 3;
            cy++;
        } else if (ny < cy) {
            dy_px = -3;
            cy--;
        }
        frames = 0;                             /* a 20 x 18 square: 6 x 3 px vertically, 5 x 4 px across, 2 x (10, 9)
                                                   diagonally */
        if (dx_px == 0)
            frames = 6;
        if (dy_px == 0)
            frames = 5;
        if (frames == 0) {
            if (dy_px > 0)
                dy_px = 9;
            else
                dy_px = -9;
            if (dx_px > 0)
                dx_px = 10;
            else
                dx_px = -10;
            frames = 2;
        }
        if (visible == 1) {
            if (Map_View_Nudge_Edge(view_x, view_y, cx, cy, MAP_VIEW_COLS, MAP_VIEW_ROWS) == 1) {
                Map_Invalidate();
                if (player != current_player) {
                    map_view_x = *view_x;
                    map_view_y = *view_y;
                }
                Map_Build_Unit_Grid(*view_x, *view_y, plane);
                sx = prev_x;
                sy = prev_y;
                Map_Square_To_Screen(*view_x, *view_y, &sx, &sy);
                if (player == current_player)
                    Map_Explore_Radius(cx, cy, plane, scout);
            } else {
                sx = prev_x;
                sy = prev_y;
                Map_Square_To_Screen(*view_x, *view_y, &sx, &sy);
            }
            from_sx = prev_x;
            from_sy = prev_y;
            Map_Square_To_Screen(*view_x, *view_y, &from_sx, &from_sy);
            to_sx = nx;
            to_sy = ny;
            Map_Square_To_Screen(*view_x, *view_y, &to_sx, &to_sy);
            Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
            for (k = 0; k < frames; k++) {
                Set_Draw_Page();
                sx = sx + dx_px;
                sy = sy + dy_px;
                if (k == 1 && player == current_player) {
                    Map_Invalidate();
                    Map_Explore_Radius(cx, cy, plane, scout);
                    Minimap_Build_View();
                }
                Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
                Map_Draw_View(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, view_x, view_y, plane, *view_x, *view_y, player);
                Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
                Map_Redraw_Block(from_sx, from_sy, plane, *view_x, *view_y);
                Map_Redraw_Block(to_sx, to_sy, plane, *view_x, *view_y);
                Map_Draw_Cities_Move_Anim(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, *view_x, *view_y, plane);
                Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
                Draw_Unit_Icon(sx, sy, unit, UNIT_ICON_MAP);
                Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
                Map_Draw_Fog(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, *view_x, *view_y, plane);
                Screen_Present();
            }
        }
        Move_Scout_Contacts(player, cx, cy, plane, scout);
        prev_x = path_x[i];
        prev_y = path_y[i];
    }
    Reset_Clip_Rect();
    Map_Invalidate();
}

/* (player, x, y, plane, range) -> the last foreign unit in range (-1 none; 0 at range 0): other wizards' units on the
   plane (or in a tower) within range, and other wizards' cities on the plane (or plane 2) within range or 3 squares,
   which also get the mover in their 'seen by' bits; an AI mover and an AI owner contact each other */
int Move_Scout_Contacts(int player, int x, int y, int plane, int range)
{
    int radius;
    int found;
    int dist;
    int i;

    radius = range;
    if (radius == 0)
        return 0;
    found = -1;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner != player && p_units[i].owner != PLAYER_NONE && p_units[i].owner != PLAYER_NEUTRAL
            && (p_units[i].plane == plane || p_units[i].in_tower == 1)) {
            dist = Map_Distance(x, y, p_units[i].x, p_units[i].y, MAP_WIDTH);
            if (dist <= radius) {
                if (player != current_player && p_units[i].owner != current_player) {
                    wizards[player].contacted[p_units[i].owner] = 1;
                    wizards[p_units[i].owner].contacted[player] = 1;
                }
                found = i;
            }
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner != player && p_cities[i].owner != PLAYER_NEUTRAL
            && (p_cities[i].plane == plane || p_cities[i].plane == PLANE_BOTH)) {
            dist = Map_Distance(x, y, p_cities[i].x, p_cities[i].y, MAP_WIDTH);
            if (dist <= radius || dist <= 3) {
                Bit_Set(player, (char far *)&p_cities[i].seen_by);
                if (player != current_player && p_cities[i].owner != current_player) {
                    wizards[player].contacted[p_cities[i].owner] = 1;
                    wizards[p_cities[i].owner].contacted[player] = 1;
                }
            }
        }
    }
    return found;
}

/* (work, needs_transport - never read -, stack_size, plane, player, x2, y2, x1, y1) marks the squares a path may not
   enter (0xff):
   neutral units except at the destination (a neutral mover: except its start), the player's own units when the
   stack would grow past 9, other wizards' units except at the destination when the relation is above the lowest
   three steps, or else when they outnumber the stack; uncleared lairs and foreign cities except the destination.
   Own units on a water destination whose transports can take everyone get cost 2
   Numbered: BUG #256, #301 (BUGS_AND_QUIRKS.txt) */
void Path_Apply_Obstacles(struct PathWork far *work, int needs_transport, int stack_size, int plane, int player,
                          int x2, int y2, int x1, int y1)
{
    int unit;
    int k;
    int capacity;
    int ux;
    int uy;
    int here_count;
    int dest_count;
    int need_boat;
    int here_units[MAX_STACK];
    int dest_units[MAX_STACK];

    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].plane == plane && p_units[unit].owner != PLAYER_NONE) {
            ux = p_units[unit].x;
            uy = p_units[unit].y;
            if (p_units[unit].owner == PLAYER_NEUTRAL) {
                if (player == PLAYER_NEUTRAL) {
                    if (p_units[unit].x == x1 && p_units[unit].y == y1)
                        ;
                    else
                        work->cost[uy][ux] = COST_IMPASSABLE;
                } else {
                    if (ux == x2 && uy == y2)
                        ;
                    else
                        work->cost[uy][ux] = COST_IMPASSABLE;
                }
            } else if (p_units[unit].owner == player) {
                if (p_units[unit].x == x1 && p_units[unit].y == y1 && p_landmass_ids[plane][y2][x2] == 0) {
                    capacity = 0;
                    Tile_Units_Of_Owner(x2, y2, plane, player, &dest_count, dest_units);
                    for (k = 0; k < dest_count; k++)
                        capacity += unit_types[p_units[dest_units[k]].type].transport_capacity;
                    if (capacity > 0) {
                        Tile_Units_Of_Owner(ux, uy, plane, player, &here_count, here_units);
                        need_boat = here_count;
                        for (k = 0; k < here_count; k++)
                            if (p_units[here_units[k]].type <= UNIT_HERO_CHOSEN || Hero_Item_Flight(here_units[k]) == 1
                                || Hero_Item_Water_Walking(here_units[k]) == 1 || Unit_Can_Swim(here_units[k]) == 1
                                || Unit_Has_Wind_Walking(here_units[k]) == 1 || Unit_Can_Fly(here_units[k]) == 1)
                                need_boat--;
                        if (need_boat - 1 <= capacity && need_boat <= MAX_STACK)
                            work->cost[uy][ux] = 2;   /* one move: the transport's square stays enterable */
                    }
                } else {
                    if (ux == x1 && uy == y1)
                        here_count = 0;
                    else
                        Tile_Units_Of_Owner(ux, uy, plane, player, &here_count, here_units);
                    if (here_count + stack_size > MAX_STACK)
                        work->cost[uy][ux] = COST_IMPASSABLE;
                    else if (p_landmass_ids[plane][y2][x2] == 0) {
                        capacity = 0;
                        Tile_Units_Of_Owner(x2, y2, plane, player, &dest_count, dest_units);
                        for (k = 0; k < dest_count; k++)
                            capacity += unit_types[p_units[dest_units[k]].type].transport_capacity;
                        if (capacity > 0) {
                            need_boat = here_count;
                            for (k = 0; k < here_count; k++)
                                if (p_units[here_units[k]].type <= UNIT_HERO_CHOSEN || Hero_Item_Flight(here_units[k]) == 1
                                    || Hero_Item_Water_Walking(here_units[k]) == 1
                                    || Unit_Can_Swim(here_units[k]) == 1
                                    || Unit_Has_Wind_Walking(here_units[k]) == 1 || Unit_Can_Fly(here_units[k]) == 1)
                                    need_boat--;
                            if (need_boat - 1 <= capacity && need_boat <= MAX_STACK)
                                work->cost[uy][ux] = 2;
                        }
                    }
                }
            } else {
                if (ux == x2 && uy == y2)
                    ;
                else if (work->cost[uy][ux] != COST_IMPASSABLE) {
                    k = (wizards[p_units[unit].owner].relationship[player] + 100) / 20;   /* steps of 20 from -100: */
                    if (k > 2)                  /* the owner's relation -40 or better: no path through */
                        work->cost[uy][ux] = COST_IMPASSABLE;
                    else {
                        Tile_Units_Of_Owner(ux, uy, plane, p_units[unit].owner, &here_count, here_units);
                        if (here_count > stack_size)
                            work->cost[uy][ux] = COST_IMPASSABLE;
                    }
                }
            }
        }
    }
    for (unit = 0; unit < NUM_LAIRS; unit++) {
        if (p_lairs[unit].plane == plane && p_lairs[unit].active == 1) {
            if (p_lairs[unit].y == y2 && p_lairs[unit].x == x2)
                ;
            else
                work->cost[p_lairs[unit].y][p_lairs[unit].x] = COST_IMPASSABLE;
        }
    }
    for (unit = 0; unit < num_cities; unit++) {
        if (p_cities[unit].plane == plane && p_cities[unit].owner != player) {
            if (p_cities[unit].y == y2 && p_cities[unit].x == x2)
                ;
            else
                work->cost[p_cities[unit].y][p_cities[unit].x] = COST_IMPASSABLE;
        }
    }
}

/* (n, units, needing_ride) -> how many units need a transport, listed in needing_ride: none at all when one unit has
   Wind Walking (or the always-0 water stub); else the units that cannot fly, cross water or use a flight / water
   walking item */
int Stack_Needs_Transport(int n, int *units, int *needing_ride)
{
    int count;
    int i;

    count = 0;
    for (i = 0; i < n; i++) {
        if (Unit_Has_Wind_Walking(units[i]) == 1 || Unit_Water_Move_Always_0(units[i]) == 1)
            return 0;
        if (Unit_Can_Fly(units[i]) == 1 || Unit_Can_Cross_Water(units[i]) == 1 || Hero_Item_Flight(units[i]) == 1
            || Hero_Item_Water_Walking(units[i]) == 1)
            ;
        else {
            needing_ride[count] = units[i];
            count++;
        }
    }
    return count;
}

/* (n, units) -> 0 when no owner is found: the owner is the first unit's (the search may read units[n], one past the
   end); when a transport with moves left stands on the square, every other unit there of that owner goes on Patrol
   and is finished - on land too (BUG #171) */
int Stack_Board_Transport(int n, int *units)
{
    int found;
    int tile_count;
    int x;
    int y;
    int plane;
    int owner;
    int tile_units[MAX_STACK];
    int i;

    x = p_units[units[0]].x;
    y = p_units[units[0]].y;
    plane = p_units[units[0]].plane;
    owner = PLAYER_NONE;
    for (i = 0; owner == PLAYER_NONE; i++) {
        if (i > n)
            break;
        owner = p_units[units[i]].owner;
    }
    if (owner == PLAYER_NONE)
        return 0;
    Tile_Units_Of_Owner(x, y, plane, owner, &tile_count, tile_units);
    found = 0;
    for (i = 0; i < tile_count; i++)
        if (unit_types[p_units[tile_units[i]].type].transport_capacity > 0 && p_units[tile_units[i]].moves_left > 0)
            found = 1;
    if (found == 1) {
        for (i = 0; i < tile_count; i++) {
            if (unit_types[p_units[tile_units[i]].type].transport_capacity < 1) {
                p_units[tile_units[i]].order = ORDER_PATROL;
                p_units[tile_units[i]].finished = 1;
            }
        }
    } else {
        for (i = 0; i < tile_count; i++)
            if (p_units[tile_units[i]].order == ORDER_PATROL)
                ;
    }
}
