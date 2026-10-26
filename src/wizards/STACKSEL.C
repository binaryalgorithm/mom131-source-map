/* STACKSEL.C - WIZARDS.EXE overlay code segment 54e2 (stub 242f): selecting a stack - collecting the player's units
   on a square, sorting them strongest first, its GoTo preview, centring or nudging the map view onto it; the stack
   orders (Ready, Patrol, Build Road, Go-To, Done, Wait, Purify); the Earth Gate jump between two cities; the unit
   statistics popup; two stubs.
   No data of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 54e2 src/wizards/STACKSEL.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Player_Stacks_Changed_Nop(int player);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Stack_Sort_By_Strength(void);
void Stack_Collect_Tile(int player, int plane, int x, int y);
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane);
int Map_View_Nudge_Edge(int *view_x, int *view_y, int x, int y, int width, int height);
int Stack_Select_Hook_Always_0(int count, int *units);
void Stack_Set_Order(int player, int *x, int *y, int *plane, int order, int dest_x, int dest_y);
int Stack_Earth_Gate_Jump(int x, int y, int plane);
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2);

#include "GAME.H"

extern int stack_fresh_selection;               /* DS:9446 */
extern int map_scroll_px_y;                     /* DS:9746 */
extern int map_scroll_px_x;                     /* DS:9748 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int stack_move_pending;                  /* DS:c042 */
extern int first_move_pending;                     /* DS:c044 */
extern int goto_preview_on;                     /* DS:c068 */

int Stack_Preview_GoTo_Path(int unit);
void Stack_Update_Special_Action(void);
void Stack_Get_Active_Units(int *count, int *units);
int Stack_Tower_Update(int count, int *list, int plane);
int Mem_Swap(char *a, char *b, int count);
int City_At(int x, int y, int plane);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
int Stack_Scout_Range(int player, int x, int y, int plane);
int Move_Scout_Contacts(int player, int x, int y, int plane, int range);
int Map_Explore_Radius(int x, int y, int plane, int range);
void Overland_Unit_View(int x, int y, int src_x1, int src_y1, int src_x2, int src_y2, int mode, int unit);

/* (player) -> 0: a stub */
int Player_Stacks_Changed_Nop(int player)
{
    return 0;
}

/* (player, &vx, &vy, plane, x, y) -> 0 when nobody stands there (no return value otherwise): selects the player's
   stack on (x, y), sorts it, previews its GoTo path when every unit is on Go-To (the current player's only),
   brings its top unit into view (Map_View_Ensure_Visible), updates the special action and the tower flag */
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y)
{
    int unit;
    int active_count;
    int active[MAX_STACK];
    int slot;
    int sel;

    stack_fresh_selection = 1;
    Stack_Collect_Tile(player, plane, x, y);
    if (stack_unit_count == 0)
        return 0;
    Stack_Sort_By_Strength();
    goto_preview_on = 0;
    sel = -1;
    if (player == current_player) {
        for (slot = 0; slot < stack_unit_count; slot++) {
            unit = stack_units[slot].unit;
            if (p_units[unit].order == ORDER_GOTO)
                sel = unit;
            else {
                sel = -1;
                break;
            }
        }
    }
    if (sel != -1)
        Stack_Preview_GoTo_Path(sel);
    unit = top_unit_in_stack;
    x = p_units[top_unit_in_stack].x;
    y = p_units[top_unit_in_stack].y;
    Map_View_Ensure_Visible(vx, vy, x, y, plane);
    Stack_Update_Special_Action();
    Stack_Get_Active_Units(&active_count, active);
    Stack_Select_Hook_Always_0(active_count, active);
    Stack_Tower_Update(active_count, active, plane);
    if (stack_unit_count < 1)
        goto_preview_on = 0;
}

/* () bubble-sorts stack_units (with their active flags) by the type's melee + ranged, a transport counting 50,
   strongest first; top_unit_in_stack = the first active unit (else the first) */
void Stack_Sort_By_Strength(void)
{
    int j;
    int unit_a;
    int unit_b;
    int pass;
    int sa;
    int sb;

    for (pass = 0; pass < stack_unit_count; pass++) {
        for (j = 0; j < stack_unit_count - 1; j++) {
            unit_a = stack_units[j].unit;
            unit_b = stack_units[j + 1].unit;
            sa = unit_types[p_units[unit_a].type].melee + unit_types[p_units[unit_a].type].ranged;
            sb = unit_types[p_units[unit_b].type].melee + unit_types[p_units[unit_b].type].ranged;
            if (unit_types[p_units[unit_a].type].transport_capacity > 0)
                sa = 50;
            if (unit_types[p_units[unit_b].type].transport_capacity > 0)
                sb = 50;
            if (sa < sb)
                Mem_Swap((char *)&stack_units[j], (char *)&stack_units[j + 1], sizeof(struct StackSlot));
        }
    }
    top_unit_in_stack = stack_units[0].unit;
    for (j = 0; j < stack_unit_count; j++) {
        if (stack_units[j].active == 1) {
            top_unit_in_stack = stack_units[j].unit;
            break;
        }
    }
}

/* (player, plane, x, y) stack_units = the player's live units on (x, y) on that plane or in a tower (no limit of
   9 here); active = moves left and not finished (a waiting unit, status 5, is woken to 0); purifying units (8)
   start inactive */
void Stack_Collect_Tile(int player, int plane, int x, int y)
{
    int unit;
    int i;
    int n;

    n = 0;
    for (i = 0; i < MAX_STACK; i++)
        stack_units[i].active = 0;
    for (i = 0; i < num_units; i++) {
        unit = i;
        if (p_units[unit].owner == player && p_units[unit].x == x && p_units[unit].y == y && p_units[unit].owner != PLAYER_NONE
            && (p_units[unit].plane == plane || p_units[unit].in_tower == 1)) {
            stack_units[n].unit = unit;
            if (p_units[unit].moves_left > 0 && p_units[unit].finished == 0) {
                stack_units[n].active = 1;
                if (p_units[unit].order == ORDER_WAIT)
                    p_units[unit].order = ORDER_READY;
            }
            if (p_units[unit].order == ORDER_PURIFY)
                stack_units[n].active = 0;
            n++;
        }
    }
    stack_unit_count = n;
}

/* (&view x, &view y, x, y, plane): (x, y) already inside the 12 x 10 view (with the x wrap) -> Map_View_Nudge_Edge
   only; else the view = (x - 6 wrapped, y - 5 at least 0, at most 30). Then the draw priorities and the unit
   grid */
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane)
{
    int tx;
    int ty;
    int found;

    ty = y;
    found = 0;
    if (*view_y <= ty && *view_y + MAP_VIEW_ROWS > ty) {
        tx = x;
        if (*view_x <= tx && *view_x + MAP_VIEW_COLS > tx) {
            tx -= *view_x;
            found = 1;
        } else {
            tx += MAP_WIDTH;
            if (*view_x <= tx && *view_x + MAP_VIEW_COLS > tx) {
                tx -= *view_x;
                found = 1;
            }
        }
    }
    if (found == 0) {
        tx = x - MAP_VIEW_COLS / 2;
        if (tx > 0)
            *view_x = tx;
        else
            *view_x = (tx + MAP_WIDTH) % MAP_WIDTH;
        ty = y - MAP_VIEW_ROWS / 2;
        if (ty > 0)
            *view_y = ty;
        else
            *view_y = 0;
        if (*view_y + MAP_VIEW_ROWS >= MAP_HEIGHT)
            *view_y = MAP_HEIGHT - MAP_VIEW_ROWS;
        map_scroll_target_x = *view_x;
        map_scroll_target_y = *view_y;
        map_scroll_px_x = map_scroll_target_x * MAP_TILE_W;
        map_scroll_px_y = map_scroll_target_y * MAP_TILE_H;
    } else
        Map_View_Nudge_Edge(view_x, view_y, x, y, MAP_VIEW_COLS, MAP_VIEW_ROWS);
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(*view_x, *view_y, plane);
}

/* (&view x, &view y, x, y, width, height) -> 1 moved: scrolls the view one square when (x, y) is on its edge
   column / row (x wraps at 60, y kept within the 40 rows) */
int Map_View_Nudge_Edge(int *view_x, int *view_y, int x, int y, int width, int height)
{
    int moved;

    moved = 0;
    if (*view_x == x) {
        (*view_x)--;
        if (*view_x < 0)
            *view_x = *view_x + MAP_WIDTH;
        moved = 1;
    }
    if ((*view_x + width - 1) % MAP_WIDTH == x) {
        *view_x = (*view_x + 1) % MAP_WIDTH;
        moved = 1;
    }
    if (*view_y + height - 1 == y && *view_y + height + 1 < 40) {
        *view_y = *view_y + 1;
        moved = 1;
    }
    if (*view_y == y && *view_y != 0) {
        *view_y = *view_y - 1;
        moved = 1;
    }
    if (moved == 1) {
        map_scroll_target_x = *view_x;
        map_scroll_target_y = *view_y;
        map_scroll_px_x = map_scroll_target_x * MAP_TILE_W;
        map_scroll_px_y = map_scroll_target_y * MAP_TILE_H;
    }
    return moved;
}

/* (count, units) -> 0: a stub */
int Stack_Select_Hook_Always_0(int count, int *units)
{
    if (count < 1)
        return 0;
    return 0;
}

/* (player, &x, &y, &plane, order, dest x, dest y) the order for the stack: 1 / 2 (Patrol / Build Road) -> the
   active units with moves get the status, finished, inactive; 4 Done -> the active units not on Go-To or
   purifying: done, finished, no moves, no road; 5 Wait -> the active units with moves: status 5, inactive; 0 Ready
   -> EVERY unit of the stack: ready (active) if it has moves, else done; 3 Go-To -> EVERY unit that is ready, done
   or on Go-To gets the destination (#169); 8 Purify -> the active purifiers (ability 0x4000). Then the selection
   blink, the draw priorities and the unit grid
   Numbered: BUG #169 (BUGS_AND_QUIRKS.txt) */
void Stack_Set_Order(int player, int *x, int *y, int *plane, int order, int dest_x, int dest_y)
{
    int count;                                  /* counted, never read */
    int unit_type;
    int slot;
    int unit;

    count = 0;
    stack_move_pending = 1;
    first_move_pending = 0;
    switch (order) {
    case ORDER_PATROL:
    case ORDER_BUILD_ROAD:
        for (slot = 0; slot < stack_unit_count; slot++) {
            unit = stack_units[slot].unit;
            if (stack_units[slot].active != 0 && p_units[unit].moves_left > 0 && p_units[unit].finished == 0) {
                p_units[unit].order = order;
                p_units[unit].finished = 1;
                stack_units[slot].active = 0;
                p_units[unit].dest_x = 0;
                p_units[unit].dest_y = 0;
                count++;
            }
        }
        break;
    case ORDER_DONE:
        for (slot = 0; slot < stack_unit_count; slot++) {
            if (stack_units[slot].active == 1 && p_units[stack_units[slot].unit].order != ORDER_GOTO
                && p_units[stack_units[slot].unit].order != ORDER_PURIFY) {
                unit = stack_units[slot].unit;
                stack_units[slot].active = 0;
                p_units[unit].finished = 1;
                p_units[unit].order = ORDER_DONE;
                p_units[unit].road_turns = -1;
                p_units[unit].moves_left = 0;
                p_units[unit].dest_x = 0;
                p_units[unit].dest_y = 0;
            }
        }
        break;
    case ORDER_WAIT:
        for (slot = 0; slot < stack_unit_count; slot++) {
            unit = stack_units[slot].unit;
            if (stack_units[slot].active != 0 && p_units[unit].moves_left > 0 && p_units[unit].finished == 0) {
                p_units[unit].order = ORDER_WAIT;
                stack_units[slot].active = 0;
                p_units[unit].dest_x = 0;
                p_units[unit].dest_y = 0;
            }
        }
        break;
    case ORDER_READY:
        for (slot = 0; slot < stack_unit_count; slot++) {
            unit = stack_units[slot].unit;
            p_units[unit].order = ORDER_READY;
            p_units[unit].road_turns = -1;
            if (p_units[unit].moves_left > 0) {
                stack_units[slot].active = 1;
                p_units[unit].finished = 0;
            } else {
                p_units[unit].order = ORDER_DONE;
                p_units[unit].finished = 1;
            }
            p_units[unit].dest_x = 0;
            p_units[unit].dest_y = 0;
        }
        break;
    case ORDER_GOTO:
        for (slot = 0; slot < stack_unit_count; slot++) {
            unit = stack_units[slot].unit;
            if (p_units[unit].order == ORDER_DONE || p_units[unit].order == ORDER_GOTO || p_units[unit].order == ORDER_READY) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = dest_x;
                p_units[unit].dest_y = dest_y;
                p_units[unit].road_turns = -1;
            }
        }
        break;
    case ORDER_PURIFY:
        for (slot = 0; slot < stack_unit_count; slot++) {
            if (stack_units[slot].active == 1) {
                unit = stack_units[slot].unit;
                unit_type = p_units[unit].type;
                if (unit_types[unit_type].ability_flags & AFLAG_PURIFY) {
                    stack_units[slot].active = 0;
                    p_units[unit].finished = 1;
                    p_units[unit].order = ORDER_PURIFY;
                    p_units[unit].moves_left = 0;
                    p_units[unit].dest_x = 0;
                    p_units[unit].dest_y = 0;
                    p_units[unit].road_turns = -1;
                }
            }
        }
        break;
    }
    Stack_Blink_Reset();
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(*x, *y, *plane);
}

/* (x, y, plane) -> 1 jumped: the clicked square is a city where the current player has at most 8 units, the stack
   stands in a city of the same plane and owner, both with an Earth Gate cast by player 0 (enchants == 1: the caster
   player + 1), and the movers + the units there <= 9: every
   active unfinished unit with moves goes to the target city for 2 half-moves (finished at <= 0). Then scouting,
   contacts and exploration from the target, the next-unit reference there, Next_Unit_Run_GoTos, the visibility */
int Stack_Earth_Gate_Jump(int x, int y, int plane)
{
    int at;
    int jumped;
    int city;
    int player;
    int scout;
    int dest_count;
    int movers;
    int dest_units[MAX_STACK];
    int unit;
    int slot;

    if (stack_unit_count < 1)
        return 0;
    city = City_At(x, y, plane);
    if (city == -1)
        return 0;
    Tile_Units_Of_Owner(x, y, plane, current_player, &dest_count, dest_units);
    if (dest_count >= MAX_STACK)
        return 0;
    movers = 0;
    jumped = 0;
    unit = stack_units[0].unit;
    at = City_At(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
    if (at != -1 && p_cities[at].plane == p_cities[city].plane && p_cities[at].owner == p_cities[city].owner
        && p_cities[at].enchants[CITY_ENCH_EARTH_GATE] == 1 && p_cities[city].enchants[CITY_ENCH_EARTH_GATE] == 1) {
        for (slot = 0; slot < stack_unit_count; slot++) {
            unit = stack_units[slot].unit;
            if (p_units[unit].finished == 0 && stack_units[slot].active == 1 && p_units[unit].moves_left > 0)
                movers++;
        }
        if (movers + dest_count <= MAX_STACK) {
            for (slot = 0; slot < stack_unit_count; slot++) {
                unit = stack_units[slot].unit;
                if (p_units[unit].finished == 0 && stack_units[slot].active == 1 && p_units[unit].moves_left > 0) {
                    p_units[unit].x = p_cities[city].x;
                    p_units[unit].y = p_cities[city].y;
                    p_units[unit].moves_left = p_units[unit].moves_left - 2;   /* the jump: one move (2 half moves) */
                    if (p_units[unit].moves_left < 1)
                        p_units[unit].finished = 1;
                }
            }
            player = p_cities[at].owner;
            scout = Stack_Scout_Range(player, x, y, plane);
            Move_Scout_Contacts(player, x, y, plane, scout);
            Map_Explore_Radius(x, y, plane, scout);
            next_unit_ref_x = p_cities[city].x;
            next_unit_ref_y = p_cities[city].y;
            Next_Unit_Run_GoTos(player, &map_view_x, &map_view_y, &plane);
            jumped = 1;
            Visibility_Rebuild();
        }
    }
    return jumped;
}

/* (unit, from x1, y1, x2, y2) the unit's statistics window, zooming from the given rectangle */
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2)
{
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Battle_Unit_Init(unit, p_unit_stats_buf);
    Overland_Unit_View(31, 6, from_x1, from_y1, from_x2, from_y2, UNIT_VIEW_OVERLAND, unit);   /* the window at (31, 6) */
}
