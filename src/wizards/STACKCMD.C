/* STACKCMD.C - WIZARDS.EXE overlay code segment 5315 (stub 2428): the selected stack's commands - toggling a unit
   in the stack panel, the click move (the move group, Stack_Move, the units left without moves done), a Go-To step
   (its group, units left behind lose the order, the path cache), running the Go-Tos until a stack needs orders
   (Find_Next_Unit: the nearest unfinished unit, waiting ones last, the other plane once), the special button
   (road / settle / purify / meld tests), the move and Go-To groups with their transports, melding a node, the
   purify turn and the idle-unit test for the automatic end of turn.
   This module's data (DS:3020): its literal; BSS (DS:c08e..c091): two words only other modules use, owner not fixed by the EXE
   (any module from STACKCMD.C to UNITDRAW.C gives the same EXE; the 5315 stand-in held them until b99).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 5315 src/wizards/STACKCMD.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Stack_Toggle_Unit(int slot);
int Stack_Move_Click(int dest_x, int dest_y, int player, int *view_x, int *view_y, int *view_plane);
int Stack_Continue_GoTo(int player, int lead_unit, int dest_x, int dest_y, int *view_x, int *view_y, int plane);
void Next_Unit_Run_GoTos(int player, int *view_x, int *view_y, int *view_plane);
int Find_Next_Unit(int player, int *plane);
void Stack_Update_Special_Action(void);
void Stack_Get_Active_Units(int *count, int *units);
void Stack_Build_Move_Group(int *count, int *out);
void GoTo_Build_Group(int *count, int *out, int dest_x, int dest_y, int player, int lead_unit);
int Stack_Can_Build_Road(int n, int *units);
int Stack_Can_Purify(int n, int *units);
int Stack_Can_Meld(int n, int *units);
void Stack_Meld_Button(void);
int Unit_Meld_Node(int n, int *units);
void Purify_Turn(void);
int Player_Has_Unordered_Units(void);

#include "GAME.H"

/* the sum of the purifiers' counters a square needs in one turn (each counter counts its purifier's turns: 0, 1, 2 ...) */
#define PURIFY_WORK_NEEDED 4

int stackcmd_unused_c090;                       /* DS:c090  never referenced */
int stack_moves_cached;                         /* DS:c08e  the last moves value of Stack_Selected_Moves / Stack_Moves_Available (UNITDRAW.C, MOVECHK.C) */

extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern int no_units_to_move;                    /* DS:9444 */
extern int stack_fresh_selection;               /* DS:9446 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int map_view_plane;                      /* DS:bd86 */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int num_units;                           /* DS:bd92 */
extern int stack_move_pending;                  /* DS:c042 */
extern int first_move_pending;                     /* DS:c044 */
extern int stack_special_action;                /* DS:c04e */
extern int goto_stack_moving;                   /* DS:c064 */
extern int goto_preview_on;                     /* DS:c068 */
extern char path_cost[];                        /* DS:c5f0 */
extern char path_y[];                           /* DS:c668 */
extern char path_x[];                           /* DS:c6e0 */

int Stack_Selected_Moves(void);
void First_Move_Nop(void);
int Stack_Move(int player, int dest_x, int dest_y, int mode, int *view_x, int *view_y, int plane, int n, int *units);
int Stack_Moves_WindWalk_Carry(void);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Stack_Clear_Draw_Priority(void);
void EMS_Map_Default(void);
void Path_Cache_Invalidate(void);
int Path_Cache_Store(int player, int x1, int y1, int x2, int y2, int cache_plane, int xs, int ys, int costs);
void Minimap_Alloc_Pic_Copy(void);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int Stack_Check_Settle(int count, int *list);
int City_At_Copy(int x, int y, int plane);
int Stack_Needs_Transport(int n, int *units, int *needing_ride);
int Hero_Item_Water_Walking(int unit);
int Unit_Can_Swim(int unit);
int Tile_Is_Water(int x, int y, int plane);
int Node_At(int x, int y, int plane);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
void Tile_Purify(int x, int y, int plane);

/* (stack slot) -> 0 when the unit is done or out of moves: a click on a unit in the stack panel. The first click on
   a fresh multi-unit selection (unless its order is the never-set ORDER_UNUSED_6) drops all the others when several are active; an
   inactive unit becomes active (a unit with orders loses them: status 0, Go-To preview off, road work cleared);
   an active one inactive; finished units never stay active */
int Stack_Toggle_Unit(int slot)
{
    int active;
    int i;
    int can_wake;                               /* always 1 */
    int unit;

    unit = stack_units[slot].unit;
    if (p_units[unit].order == ORDER_DONE || p_units[unit].moves_left < 1)
        return 0;
    if (stack_fresh_selection == 1 && stack_unit_count > 1 && p_units[unit].order != ORDER_UNUSED_6) {
        active = 0;
        for (i = 0; i < stack_unit_count; i++)
            if (stack_units[i].active == 1)
                active++;
        if (active > 1)
            for (i = 0; i < stack_unit_count; i++)
                stack_units[i].active = 0;
    }
    stack_fresh_selection = 0;
    unit = stack_units[slot].unit;
    if (stack_units[slot].active == 0) {
        if (p_units[unit].order > 0) {
            if (p_units[unit].moves_left > 0) {
                can_wake = 1;
                if (can_wake == 1) {
                    stack_units[slot].active = 1;
                    if (p_units[unit].order == ORDER_GOTO)
                        goto_preview_on = 0;
                    p_units[unit].order = ORDER_READY;
                    p_units[unit].finished = 0;
                    p_units[unit].road_turns = -1;
                    if (p_units[unit].moves_left < 1) {
                        stack_units[slot].active = 0;
                        p_units[unit].finished = 1;
                    } else {
                        no_units_to_move = 0;
                        Stack_Blink_Reset();
                    }
                }
            }
        } else {
            if (p_units[unit].moves_left > 0)
                stack_units[slot].active = 1;
            else
                stack_units[slot].active = 0;
            p_units[unit].order = ORDER_READY;
        }
    } else
        stack_units[slot].active = 0;
    if (p_units[unit].finished == 1)
        stack_units[slot].active = 0;
    Stack_Selected_Moves();
    Stack_Update_Special_Action();
}

/* (dest x, y, player, &view x, &view y, &view plane) -> 0 for the top / bottom row: the human's click move - the move
   group, Stack_Move, the stack reselected at the top unit's square; out of moves: the group's ready units are done
   (status 4) and the Go-Tos run
   Numbered: BUG #203 (BUGS_AND_QUIRKS.txt) */
int Stack_Move_Click(int dest_x, int dest_y, int player, int *view_x, int *view_y, int *view_plane)
{
    int top_unit;
    int move_mode;
    int moves;
    int x;
    int y;
    int plane;
    int count;
    int group[MAX_STACK];
    int i;

    move_mode = STACK_MOVE_NORMAL;
    if (dest_y == 0 || dest_y == MAP_HEIGHT - 1)
        return 0;
    Stack_Build_Move_Group(&count, group);
    top_unit_in_stack = group[0];
    if (stack_move_pending == 1) {
        stack_move_pending = 0;
        first_move_pending = 0;
    }
    if (first_move_pending == 0)
        First_Move_Nop();
    Stack_Move(player, dest_x, dest_y, move_mode, view_x, view_y, *view_plane, count, group);
    top_unit = top_unit_in_stack;
    x = p_units[top_unit].x;
    y = p_units[top_unit].y;
    plane = p_units[top_unit].plane;
    Player_Stacks_Changed_Nop(player);
    moves = Stack_Moves_WindWalk_Carry();      /* not read: the call refreshes the cache */
    Player_Stacks_Changed_Nop(player);
    Stack_Select_At(player, view_x, view_y, *view_plane, x, y);
    moves = Stack_Moves_WindWalk_Carry();
    if (moves < 1) {
        for (i = 0; i < count; i++) {
            if (p_units[group[i]].order == ORDER_READY) {
                p_units[group[i]].finished = 1;
                p_units[group[i]].order = ORDER_DONE;
            }
        }
        Next_Unit_Run_GoTos(player, view_x, view_y, view_plane);
    }
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(*view_x, *view_y, *view_plane);
    Units_Moved_Nop();
    Stack_Blink_Reset();
}

/* (player, lead unit, dest x, y, &view x, &view y, plane) -> Stack_Move's result, 0 without a group: one Go-To step
   (mode 2 when every unit is building a road); units still on the start square without road work lose the Go-To
   (goto_blocked); the path cache follows the first live unit without road work */
int Stack_Continue_GoTo(int player, int lead_unit, int dest_x, int dest_y, int *view_x, int *view_y, int plane)
{
    int unit;
    int move_mode;
    int count;
    int moved;
    int x;
    int y;
    int road_units;
    int units[MAX_STACK];
    int i;

    GoTo_Build_Group(&count, units, dest_x, dest_y, player, lead_unit);
    if (count < 1)
        return 0;
    road_units = 0;
    move_mode = STACK_MOVE_NORMAL;
    for (i = 0; i < count; i++) {
        unit = units[i];
        if (p_units[unit].road_turns != -1)
            road_units++;
    }
    if (road_units == count)
        move_mode = STACK_MOVE_ROAD;
    x = p_units[lead_unit].x;
    y = p_units[lead_unit].y;
    moved = Stack_Move(player, dest_x, dest_y, move_mode, view_x, view_y, plane, count, units);
    for (i = 0; i < count; i++) {
        if (p_units[units[i]].x == x && p_units[units[i]].y == y && p_units[units[i]].road_turns == -1
            && (p_units[units[i]].order == ORDER_GOTO || p_units[units[i]].order == ORDER_GOTO_ENEMY) && p_units[units[i]].finished == 0) {
            p_units[units[i]].goto_blocked = 1;
            p_units[units[i]].order = ORDER_READY;
            p_units[units[i]].dest_x = 0;
            p_units[units[i]].dest_y = 0;
        }
    }
    for (i = 0; i < count; i++) {
        if (p_units[units[i]].owner != PLAYER_NONE && p_units[units[i]].road_turns == -1) {
            if (p_units[units[i]].order == ORDER_READY) {
                EMS_Map_Default();
                Path_Cache_Invalidate();
                break;
            } else {
                EMS_Map_Default();
                Path_Cache_Store(player, p_units[units[i]].x, p_units[units[i]].y, p_units[units[i]].dest_x,
                                 p_units[units[i]].dest_y, plane, (int)path_x, (int)path_y, (int)path_cost);
                break;
            }
        }
    }
    if (player == current_player) {
        Player_Stacks_Changed_Nop(player);
        Units_Set_Draw_Priority();
        Stack_Clear_Draw_Priority();
        Map_Build_Unit_Grid(*view_x, *view_y, plane);
        Stack_Blink_Reset();
    }
    return moved;
}

/* (player, &view x, &view y, &view plane) selects the player's next stacks in Find_Next_Unit order, moving each one on
   a Go-To (or readying it when it has arrived and has no road work) until one needs orders or none is left (then
   no_units_to_move = 1, the stack emptied) */
void Next_Unit_Run_GoTos(int player, int *view_x, int *view_y, int *view_plane)
{
    int unit;
    int done;
    int none_left;
    int x;
    int y;
    int dest_x;
    int dest_y;
    int unused;

    unused = 0;
    none_left = 0;
    done = 0;
    Map_Invalidate();
    Player_Stacks_Changed_Nop(player);
    while (done == 0) {
        goto_stack_moving = 0;
        none_left = Find_Next_Unit(player, view_plane);
        if (none_left == 1) {
            no_units_to_move = 1;
            goto_preview_on = 0;
            done = 1;
        } else {
            unit = top_unit_in_stack;
            x = p_units[unit].x;
            y = p_units[unit].y;
            Stack_Select_At(player, view_x, view_y, *view_plane, x, y);
        }
        if (done == 0) {
            done = 1;
            switch (p_units[top_unit_in_stack].order) {
            case ORDER_GOTO:
                dest_x = p_units[top_unit_in_stack].dest_x;
                dest_y = p_units[top_unit_in_stack].dest_y;
                goto_stack_moving = 1;
                if (p_units[top_unit_in_stack].x == dest_x && p_units[top_unit_in_stack].y == dest_y
                    && p_units[top_unit_in_stack].road_turns == -1)
                    p_units[top_unit_in_stack].order = ORDER_READY;
                else {
                    Minimap_Alloc_Pic_Copy();
                    Stack_Continue_GoTo(player, top_unit_in_stack, dest_x, dest_y, view_x, view_y, *view_plane);
                    Minimap_Alloc_Pic_Copy();
                }
                done = 0;
                break;
            }
        }
    }
    if (no_units_to_move == 0) {
        unit = top_unit_in_stack;
        x = p_units[unit].x;
        y = p_units[unit].y;
        Stack_Select_At(player, view_x, view_y, *view_plane, x, y);
    } else {
        stack_unit_count = 0;
        Stack_Blink_Off();
        Map_Build_Unit_Grid(*view_x, *view_y, *view_plane);
    }
    stack_move_pending = 1;
}

/* (player, &plane) -> 1 none left, 0 found (top_unit_in_stack, next_unit_ref x / y): the unfinished unit of the
   player nearest to the last one, on *plane or in a tower, not purifying (8) or 100; a waiting unit (5) only when no
   ready one is on this plane - then every waiting unit becomes ready; nothing here -> the other plane once
   Numbered: BUG #285 (BUGS_AND_QUIRKS.txt) */
int Find_Next_Unit(int player, int *plane)
{
    int done;
    int none_left;
    int distance;
    int i;
    int wait_dist;
    int best_waiting;
    int best_dist;
    int best;
    int k;
    int switched;
    int unused;
    int unit;
    int cur_plane;

    wait_dist = MIN_NOT_FOUND;
    best_waiting = -1;
    best_dist = MIN_NOT_FOUND;
    best = -1;
    switched = 0;
    cur_plane = *plane;
    unused = -1;
    none_left = 0;
    done = 0;
    i = 0;
    while (done == 0) {
        unit = i;
        if (p_units[unit].owner == player && (p_units[unit].plane == cur_plane || p_units[unit].in_tower == 1)
            && p_units[unit].owner != PLAYER_NONE && p_units[unit].finished == 0) {
            distance = Map_Distance(next_unit_ref_x, next_unit_ref_y, p_units[unit].x, p_units[unit].y, MAP_WIDTH);
            switch (p_units[unit].order) {
            case ORDER_PURIFY:
                break;
            case ORDER_WAIT:
                if (wait_dist > distance) {
                    wait_dist = distance;
                    best_waiting = unit;
                }
                break;
            case 100:                           /* no 1.31 code stores order 100 */
                break;
            default:
                if (best_dist > distance) {
                    best_dist = distance;
                    best = unit;
                }
                break;
            }
        }
        i++;
        if (i == num_units) {
            if (best == -1) {
                if (best_waiting == -1) {
                    if (switched == 1) {
                        done = 1;
                        none_left = 1;
                    } else {
                        switched = 1;
                        cur_plane = (cur_plane + 1) % 2;
                        wait_dist = MIN_NOT_FOUND;
                        best_waiting = -1;
                        best_dist = MIN_NOT_FOUND;
                        best = -1;
                        unused = -1;
                        i = 0;
                    }
                } else {
                    if (cur_plane == PLANE_BOTH)
                        cur_plane = 0;
                    *plane = cur_plane;
                    top_unit_in_stack = best_waiting;
                    done = 1;
                    next_unit_ref_x = p_units[top_unit_in_stack].x;
                    next_unit_ref_y = p_units[top_unit_in_stack].y;
                    for (k = 0; k < num_units; k++)
                        if (p_units[k].owner == player && p_units[k].order == ORDER_WAIT)
                            p_units[k].order = ORDER_READY;
                }
            } else {
                done = 1;
                if (cur_plane == PLANE_BOTH)
                    cur_plane = 0;
                *plane = cur_plane;
                top_unit_in_stack = best;
                next_unit_ref_x = p_units[top_unit_in_stack].x;
                next_unit_ref_y = p_units[top_unit_in_stack].y;
            }
        }
    }
    return none_left;
}

/* () the special button for the active units: none (-1), road 0, settle 1, purify 2, meld 9 (the last test that
   passes wins) */
void Stack_Update_Special_Action(void)
{
    int count;
    int units[MAX_STACK];

    stack_special_action = STACK_SPECIAL_NONE;
    Stack_Get_Active_Units(&count, units);
    Stack_Can_Build_Road(count, units);
    Stack_Check_Settle(count, units);
    Stack_Can_Purify(count, units);
    Stack_Can_Meld(count, units);
}

/* (&count, units) the active units of the selected stack */
void Stack_Get_Active_Units(int *count, int *units)
{
    int i;
    int n;

    n = 0;
    for (i = 0; i < stack_unit_count; i++) {
        if (stack_units[i].active == 1) {
            units[n] = stack_units[i].unit;
            n++;
        }
    }
    *count = n;
}

/* (&count, out) the units that move with a click: without an active transport the active units not on a Go-To;
   with one in a city the active transports only; with one outside a city the active transports, then - nobody needing a ride - every other unit not on a
   Go-To, or else the riders in stack order while carried < capacity (heroes ride free; the matching stops at the
   first unit left over) and every unit that moves by itself (flight / water walking item, swimming, Wind Walking,
   flying) not on a Go-To (BUG #167) */
void Stack_Build_Move_Group(int *count, int *out)
{
    int n_all;
    int n_out;
    int has_ship;
    int needs_transport;
    int capacity;
    int k;
    int carried;
    int to_carry[MAX_STACK];
    int all_units[MAX_STACK];
    int i;

    has_ship = 0;
    capacity = 0;
    for (i = 0; i < stack_unit_count; i++) {
        if (stack_units[i].active == 1 && p_units[stack_units[i].unit].order != ORDER_GOTO
            && unit_types[p_units[stack_units[i].unit].type].transport_capacity > 0) {
            has_ship = 1;
            capacity += unit_types[p_units[stack_units[i].unit].type].transport_capacity;
        }
    }
    if (has_ship == 1) {
        n_all = 0;
        n_out = 0;
        for (i = 0; i < stack_unit_count; i++) {
            if (stack_units[i].active == 1 && p_units[stack_units[i].unit].order != ORDER_GOTO
                && unit_types[p_units[stack_units[i].unit].type].transport_capacity > 0) {
                out[n_out] = stack_units[i].unit;
                n_out++;
            }
            all_units[n_all] = stack_units[i].unit;
            n_all++;
        }
        n_all = n_out;
        if (City_At_Copy(p_units[stack_units[0].unit].x, p_units[stack_units[0].unit].y,
                      p_units[stack_units[0].unit].plane) == -1) {
            for (i = 0; i < MAX_STACK; i++)
                to_carry[i] = -1;
            needs_transport = Stack_Needs_Transport(stack_unit_count, all_units, to_carry);
            if (needs_transport > 0) {
                n_all = n_out;
                k = 0;
                carried = 0;
                for (i = 0; i < stack_unit_count; i++) {
                    if (to_carry[k] == stack_units[i].unit
                        && (carried < capacity || p_units[stack_units[i].unit].type <= UNIT_HERO_CHOSEN)) {
                        out[n_all] = stack_units[i].unit;
                        n_all++;
                        k++;
                        if (p_units[stack_units[i].unit].type > UNIT_HERO_CHOSEN)
                            carried++;
                    }
                }
                for (i = 0; i < stack_unit_count; i++) {
                    if ((Hero_Item_Flight(stack_units[i].unit) == 1 || Hero_Item_Water_Walking(stack_units[i].unit) == 1
                         || Unit_Can_Swim(stack_units[i].unit) == 1 || Unit_Has_Wind_Walking(stack_units[i].unit) == 1
                         || Unit_Can_Fly(stack_units[i].unit) == 1)
                        && p_units[stack_units[i].unit].order != ORDER_GOTO) {
                        out[n_all] = stack_units[i].unit;
                        n_all++;
                    }
                }
            } else {
                if (n_all < stack_unit_count) {
                    for (i = 0; i < stack_unit_count; i++) {
                        if (p_units[stack_units[i].unit].order != ORDER_GOTO
                            && unit_types[p_units[stack_units[i].unit].type].transport_capacity < 1) {
                            out[n_all] = stack_units[i].unit;
                            n_all++;
                        }
                    }
                }
            }
        }
    } else {
        n_all = 0;
        for (i = 0; i < stack_unit_count; i++) {
            if (stack_units[i].active == 1 && p_units[stack_units[i].unit].order != ORDER_GOTO) {
                out[n_all] = stack_units[i].unit;
                n_all++;
            }
        }
    }
    *count = n_all;
}

/* (&count, out, dest x, y, player, lead unit) the units that move with a Go-To: the player's units on the lead's
   square; transports on a Go-To (3 / 16) to the same destination - in a city only they go; outside one, nobody
   needing a ride -> every non-transport, else the riders while carried <= capacity (one too many, BUG #168) and
   every unit that moves by itself; no transport: the units on a Go-To to the same destination with moves left and
   the lead's road work. The self-mover test has no Non-Corporeal, and heroes (not counted) are left behind once the
   count is full, the capacity test coming first; clarified using ReMoM repo */
void GoTo_Build_Group(int *count, int *out, int dest_x, int dest_y, int player, int lead_unit)
{
    int n;
    int ships;
    int tile_count;
    int x;
    int y;
    int plane;
    int has_ship;
    int needs_transport;
    int capacity;
    int boat_i;
    int carried;
    int unused;
    int road_turns;
    int tile_units[MAX_STACK];
    int boat_list[MAX_STACK];
    int all_units[MAX_STACK];
    int i;
    int unit;

    x = p_units[lead_unit].x;
    y = p_units[lead_unit].y;
    plane = p_units[lead_unit].plane;
    road_turns = p_units[lead_unit].road_turns;
    tile_count = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].plane == plane && p_units[i].x == x && p_units[i].y == y && p_units[i].owner == player) {
            tile_units[tile_count] = i;
            tile_count++;
        }
    }
    n = 0;
    has_ship = 0;
    capacity = 0;
    for (i = 0; i < tile_count; i++) {
        unit = tile_units[i];
        if ((p_units[unit].order == ORDER_GOTO || p_units[unit].order == ORDER_GOTO_ENEMY) && p_units[unit].dest_x == dest_x
            && p_units[unit].dest_y == dest_y && unit_types[p_units[unit].type].transport_capacity > 0) {
            has_ship = 1;
            capacity += unit_types[p_units[unit].type].transport_capacity;
        }
    }
    if (has_ship == 1) {
        n = 0;
        ships = 0;
        for (i = 0; i < tile_count; i++) {
            unit = tile_units[i];
            if ((p_units[unit].order == ORDER_GOTO || p_units[unit].order == ORDER_GOTO_ENEMY) && p_units[unit].dest_x == dest_x
                && p_units[unit].dest_y == dest_y && unit_types[p_units[unit].type].transport_capacity > 0) {
                out[ships] = unit;
                ships++;
            }
            all_units[n] = unit;
            n++;
        }
        n = ships;
        if (City_At_Copy(x, y, plane) == -1) {
            for (i = 0; i < MAX_STACK; i++)
                boat_list[i] = -1;
            needs_transport = Stack_Needs_Transport(tile_count, all_units, boat_list);
            unused = 0;
            if (needs_transport > 0) {
                n = ships;
                boat_i = 0;
                carried = 0;
                for (i = 0; i < tile_count; i++) {
                    unit = tile_units[i];
                    if (boat_list[boat_i] == unit && carried <= capacity) {
                        out[n] = unit;
                        n++;
                        boat_i++;
                        if (p_units[unit].type > UNIT_HERO_CHOSEN)
                            carried++;
                    }
                }
                for (i = 0; i < tile_count; i++) {
                    if (Hero_Item_Flight(tile_units[i]) == 1 || Hero_Item_Water_Walking(tile_units[i]) == 1
                        || Unit_Can_Swim(tile_units[i]) == 1 || Unit_Has_Wind_Walking(tile_units[i]) == 1
                        || Unit_Can_Fly(tile_units[i]) == 1) {
                        out[n] = tile_units[i];
                        n++;
                    }
                }
            } else {
                if (n < tile_count) {
                    for (i = 0; i < tile_count; i++) {
                        if (unit_types[p_units[tile_units[i]].type].transport_capacity < 1) {
                            out[n] = tile_units[i];
                            n++;
                        }
                    }
                }
            }
        }
    } else {
        n = 0;
        for (i = 0; i < tile_count; i++) {
            unit = tile_units[i];
            if ((p_units[unit].order == ORDER_GOTO || p_units[unit].order == ORDER_GOTO_ENEMY) && p_units[unit].dest_x == dest_x
                && p_units[unit].dest_y == dest_y && p_units[unit].moves_left > 0
                && p_units[unit].road_turns == road_turns) {
                out[n] = unit;
                n++;
            }
        }
    }
    *count = n;
}

/* (n, units) -> 1, stack_special_action 0: the road button - the first unit's square is land and one of the units
   can build roads */
int Stack_Can_Build_Road(int n, int *units)
{
    int unit;
    int unit_type;
    int found;
    int i;

    if (Tile_Is_Water(p_units[*units].x, p_units[*units].y, p_units[*units].plane) == 1)
        return 0;
    found = 0;
    for (i = 0; i < n && found == 0; i++) {
        unit = units[i];
        unit_type = p_units[unit].type;
        if (unit_types[unit_type].construction > 0)
            found = 1;
    }
    if (found == 1) {
        stack_special_action = STACK_SPECIAL_ROAD;
        return 1;
    }
    return 0;
}

/* (n, units) -> 1, stack_special_action 2: the purify button - a purifier (ability 0x4000) and the first unit's
   square corrupted (terrain flag 0x20), read on the VIEW plane */
int Stack_Can_Purify(int n, int *units)
{
    int unit;
    int unit_type;
    int found;
    int x;
    int y;
    int i;

    found = 0;
    for (i = 0; i < n && found == 0; i++) {
        unit = units[i];
        unit_type = p_units[unit].type;
        if (unit_types[unit_type].ability_flags & AFLAG_PURIFY)
            found = 1;
    }
    if (found == 1) {
        x = p_units[*units].x;
        y = p_units[*units].y;
        if ((p_terrain_flags_maps[map_view_plane][y][x] & TERRAIN_FLAG_CORRUPTION) == TERRAIN_FLAG_CORRUPTION) {
            stack_special_action = STACK_SPECIAL_PURIFY;
            return 1;
        }
    }
    return 0;
}

/* (n, units) -> 1, stack_special_action 9: the meld button - on a node with a melder (ability 0x400); the owner's own
   node only for a Guardian Spirit and not already guarded (meld_flags bit 1), any other node unless warped (bit 0) */
int Stack_Can_Meld(int n, int *units)
{
    int unit;
    int unit_type;
    int owner;
    int has_melder;
    int i;
    int node;
    int guardian;

    node = Node_At(p_units[*units].x, p_units[*units].y, p_units[*units].plane);
    owner = p_units[*units].owner;
    has_melder = 0;
    guardian = 0;
    if (node != -1) {
        for (i = 0; i < n; i++) {
            unit = units[i];
            unit_type = p_units[unit].type;
            if (unit_types[unit_type].ability_flags & AFLAG_MELD_WITH_NODE) {
                if (p_units[unit].type == (int)p_spell_data[SPELL_GUARDIAN_SPIRIT].effect_param) {
                    guardian = 1;
                    has_melder = 1;
                    break;
                } else
                    has_melder = 1;
            }
        }
        if (has_melder == 1) {
            if (p_nodes[node].owner != owner) {
                if (p_nodes[node].meld_flags & NODE_WARPED)
                    return 0;
                else
                    stack_special_action = STACK_SPECIAL_MELD;
                return 1;
            } else {
                if (p_nodes[node].meld_flags & NODE_GUARDIAN_SPIRIT)
                    return 0;
                else if (guardian == 1) {
                    stack_special_action = STACK_SPECIAL_MELD;
                    return 1;
                } else
                    return 0;
            }
        }
    }
    return 0;
}

/* () the meld button: the active units meld the node */
void Stack_Meld_Button(void)
{
    int count;
    int units[MAX_STACK];

    Stack_Get_Active_Units(&count, units);
    Unit_Meld_Node(count, units);
}

/* (n, units) melds the node under the units with the first Guardian Spirit, else the last melder; nothing on a warped
   node. Unowned or own node, or no Guardian there: melded; a guarded enemy node: 25% - Random(100) <= 25 - (the Guardian is removed),
   else "The Guardian Spirit occupying this node kills your spirit." for the human. The unit is used up either way */
int Unit_Meld_Node(int n, int *units)
{
    int k;
    int unit;
    int guardian;
    int owner;
    int node_owner;
    int melded;
    int roll;
    int stack_unit;
    int node;

    if (n < 1)
        return 0;
    stack_unit = *units;
    node = Node_At(p_units[stack_unit].x, p_units[stack_unit].y, p_units[stack_unit].plane);
    if (p_nodes[node].meld_flags & NODE_WARPED)
        return 0;
    owner = p_units[stack_unit].owner;
    node_owner = p_nodes[node].owner;
    guardian = 0;
    unit = -1;
    for (k = 0; k < n; k++) {
        stack_unit = units[k];
        if (unit_types[p_units[stack_unit].type].ability_flags & AFLAG_MELD_WITH_NODE) {
            if (p_units[stack_unit].type == (int)p_spell_data[SPELL_GUARDIAN_SPIRIT].effect_param) {
                unit = stack_unit;
                guardian = 1;
                break;
            } else
                unit = stack_unit;
        }
    }
    if (unit == -1)
        return 0;
    melded = 0;
    if (node_owner == PLAYER_NONE || node_owner == owner)
        melded = 1;
    else if (p_nodes[node].meld_flags & NODE_GUARDIAN_SPIRIT) {
        roll = Random(100);
        if (roll <= 25) {
            melded = 1;
            p_nodes[node].meld_flags = p_nodes[node].meld_flags ^ NODE_GUARDIAN_SPIRIT;
        } else if (owner == current_player)
            Message_Box("The Guardian Spirit occupying this node kills your spirit.");
    } else
        melded = 1;
    if (melded == 1) {
        p_nodes[node].owner = owner;
        if (guardian == 1)
            p_nodes[node].meld_flags = p_nodes[node].meld_flags | NODE_GUARDIAN_SPIRIT;
    }
    Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED);
}

/* () Next_Turn: the purify orders (8), unless frozen by Time Stop - non-purifiers drop the order, purifiers count:
   per square, every purifier of the owner there adds its counter (dest_x) and steps it; a total of 4 or more purifies
   the square and stops them (so one purifier takes 5 turns, two or three 3, four or more 2) */
void Purify_Turn(void)
{
    int x;
    int y;
    int plane;
    int tile_count;
    int progress;
    int owner;
    int tile_units[MAX_STACK];
    int unit;
    int i;                                      /* first the Time Stop flag, then the square's index */

    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner != PLAYER_NONE) {
            i = 0;
            if (time_stop_player > 0 && p_units[unit].owner + 1 != time_stop_player)
                i = 1;
            if (i == 0 && p_units[unit].order == ORDER_PURIFY) {
                if (unit_types[p_units[unit].type].ability_flags & AFLAG_PURIFY)
                    p_units[unit].order = ORDER_PURIFYING;
                else
                    p_units[unit].order = ORDER_READY;
            }
        }
    }
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner != PLAYER_NONE && p_units[unit].order == ORDER_PURIFYING) {
            x = p_units[unit].x;
            y = p_units[unit].y;
            plane = p_units[unit].plane;
            owner = p_units[unit].owner;
            Tile_Units_Of_Owner(x, y, plane, owner, &tile_count, tile_units);
            progress = 0;
            for (i = 0; i < tile_count; i++) {
                if (p_units[tile_units[i]].order == ORDER_PURIFYING) {
                    progress += p_units[tile_units[i]].dest_x;
                    p_units[tile_units[i]].dest_x = p_units[tile_units[i]].dest_x + 1;
                    p_units[tile_units[i]].order = ORDER_PURIFY;
                }
            }
            if (progress >= PURIFY_WORK_NEEDED) {
                Tile_Purify(p_units[tile_units[0]].x, p_units[tile_units[0]].y, p_units[tile_units[0]].plane);
                for (i = 0; i < tile_count; i++) {
                    if (p_units[tile_units[i]].order == ORDER_PURIFY) {
                        p_units[tile_units[i]].dest_x = 0;
                        p_units[tile_units[i]].order = ORDER_READY;
                        p_units[tile_units[i]].finished = 1;
                    }
                }
            }
        }
    }
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].owner != PLAYER_NONE && p_units[unit].order == ORDER_PURIFYING)
            p_units[unit].order = ORDER_PURIFY;
}

/* () -> 1 when the current player has a unit not on Patrol (1), Go-To (3) or purify (8) - done and finished units count too, so "idle" is not the test */
int Player_Has_Unordered_Units(void)
{
    int unit;
    int count;

    count = 0;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].owner == current_player && p_units[unit].order != ORDER_PATROL && p_units[unit].order != ORDER_GOTO
            && p_units[unit].order != ORDER_PURIFY)
            count++;
    if (count > 0)
        return 1;
    else
        return 0;
}
