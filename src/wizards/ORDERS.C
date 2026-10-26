/* ORDERS.C - WIZARDS.EXE overlay code segment 81df (stub 24ff): carrying out unit orders - the AI's standing orders
   per unit (build road then go, go-to, meld, settle, ferry wait), the settle button and founding an outpost from a
   stack, room on a square for a unit (water, towers, sailing types), pushing a unit off an overfull square, and the
   plane-shift test of a stack; one unused helper pair.
   No data of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 81df src/wizards/ORDERS.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Execute_Unit_Orders(int player);
int AI_Order_Meld_Exec(int unit);
int Unit_Settle_Exec(int unit);
int Stack_Settle_Button(void);
int Stack_Try_Outpost(int n, int *units);
int AI_Order_GoTo_Exec(int unit);
int Tile_Find_Free_Near_Unused(int x, int y, int plane, int player, int *out_x, int *out_y);
int Tile_Free_Slots(int x, int y, int plane, int player);
void Unit_Bump_To_Adjacent(int unit);
int Tile_Room_For_Unit(int x, int y, int plane, int player, int unit);
int AI_Order_Ferry_Wait_Exec(int unit);
int Road_Order_Start(int unit);
int Stack_Can_Plane_Shift(int x, int y, int plane);

#include "GAME.H"

extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern char path_y[];                           /* DS:c668 */
extern char path_x[];                           /* DS:c6e0 */

void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
void Map_Stack_At(int x, int y, int plane, int *count, int *list);
void Units_At_Tile_Plane(int x, int y, int plane, int *count, int *list);
int Unit_Meld_Node(int n, int *units);
void Stack_Get_Active_Units(int *count, int *units);
int Settler_Found_City(int x, int y, int plane, int race, int owner, int settler);
void Minimap_Alloc_Pic(void);
void Minimap_Build_View(void);
int Stack_Continue_GoTo(int player, int lead_unit, int dest_x, int dest_y, int *view_x, int *view_y, int plane);
int Lair_At(int x, int y, int plane);
int Player_City_At(int x, int y, int plane, int player);
int Tile_Is_Water(int x, int y, int plane);
int Stack_Tower_Update(int count, int *list, int plane);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Unit_Can_Cross_Water(int unit);
int Unit_Can_Swim(int unit);
int Hero_Item_Water_Walking(int unit);
int Road_Path_AI(int x1, int y1, int x2, int y2, int plane, char *xs, char *ys);
int Road_Build_Turns(int x, int y, int plane);
int Unit_Has_Endurance(int unit);
int Planar_Seal_Active(void);

/* (player) -> 0 under another wizard's Time Stop (no return value otherwise): every unit of the player carries out
   its standing order: 2 build road (then go), 3 / 16 go to, 9 meld, 10 settle, 11 ferry wait; the view is restored */
int AI_Execute_Unit_Orders(int player)
{
    int view_x;
    int view_y;
    int view_plane;
    int unit;

    view_x = map_view_x;
    view_y = map_view_y;
    view_plane = map_view_plane;
    if (time_stop_player > 0 && player + 1 != time_stop_player)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner == player) {
            switch (p_units[unit].order) {
            case ORDER_MELD:
                AI_Order_Meld_Exec(unit);
                break;
            case ORDER_PURIFY:
                break;
            case ORDER_SETTLE:
                Unit_Settle_Exec(unit);
                break;
            case ORDER_BUILD_ROAD:
                Road_Order_Start(unit);
                AI_Order_GoTo_Exec(unit);
                break;
            case ORDER_GOTO:
                AI_Order_GoTo_Exec(unit);
                break;
            case ORDER_AI_WAIT:
                AI_Order_Ferry_Wait_Exec(unit);
                break;
            case ORDER_GOTO_ENEMY:
                AI_Order_GoTo_Exec(unit);
                break;
            }
        }
    }
    map_view_x = view_x;
    map_view_y = view_y;
    map_scroll_target_x = map_view_x;
    map_scroll_target_y = map_view_y;
    map_view_plane = view_plane;
}

/* (unit) -> 0 when finished (no return value otherwise): the meld order (status 9) - the owner's units on the
   square meld with the node (Unit_Meld_Node), then all of them are ready (status 0) */
int AI_Order_Meld_Exec(int unit)
{
    int count;
    int x;
    int y;
    int plane;
    int owner;
    int units[MAX_STACK];
    int i;

    if (p_units[unit].finished == 1)
        return 0;
    x = p_units[unit].x;
    y = p_units[unit].y;
    plane = p_units[unit].plane;
    owner = p_units[unit].owner;
    Tile_Units_Of_Owner(x, y, plane, owner, &count, units);
    Unit_Meld_Node(count, units);
    for (i = 0; i < count; i++)
        p_units[units[i]].order = ORDER_READY;
}

/* (unit) -> 0 when finished (no return value otherwise): the settle order (status 10) - the owner's units on the
   square try to found an outpost (Stack_Try_Outpost), then all of them are ready (status 0) */
int Unit_Settle_Exec(int unit)
{
    int count;
    int x;
    int y;
    int plane;
    int owner;
    int units[MAX_STACK];
    int i;

    if (p_units[unit].finished == 1)
        return 0;
    x = p_units[unit].x;
    y = p_units[unit].y;
    plane = p_units[unit].plane;
    owner = p_units[unit].owner;
    Tile_Units_Of_Owner(x, y, plane, owner, &count, units);
    Stack_Try_Outpost(count, units);
    for (i = 0; i < count; i++)
        p_units[units[i]].order = ORDER_READY;
}

/* () -> Stack_Try_Outpost of the selected stack's active units (the human's Build button) */
int Stack_Settle_Button(void)
{
    int count;
    int units[MAX_STACK];

    Stack_Get_Active_Units(&count, units);
    return Stack_Try_Outpost(count, units);
}

/* (n, units) -> 1 when the first unit that can found an outpost (type ability 0x20) and is not undead (mutation
   0x20) founds a city (Settler_Found_City with its race and owner) */
int Stack_Try_Outpost(int n, int *units)
{
    int unit_type;
    int found;
    int x;
    int y;
    int plane;
    int race;
    int owner;
    int unit;
    int i;

    found = 0;
    for (i = 0; i < n && found == 0; i++) {
        unit = units[i];
        unit_type = p_units[unit].type;
        if ((unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) && (p_units[unit].mutation_flags & MUT_UNDEAD) == 0)
            found = 1;
    }
    if (found == 1) {
        x = p_units[unit].x;
        y = p_units[unit].y;
        plane = p_units[unit].plane;
        owner = p_units[unit].owner;
        race = unit_types[p_units[unit].type].race;
        if (Settler_Found_City(x, y, plane, race, owner, unit) == 1)
            return 1;
        else
            return 0;
    }
    return 0;
}

/* (unit) -> 0 when finished (no return value otherwise): the go-to order (status 3 / 16) - Stack_Continue_GoTo
   toward the unit's destination */
int AI_Order_GoTo_Exec(int unit)
{
    int dest_x;
    int dest_y;
    int view_x;
    int view_y;
    int plane;
    int owner;
    int ux;                                     /* written, never read */
    int uy;                                     /* written, never read */
    int result;                                 /* written, never read */

    if (p_units[unit].finished == 1)
        return 0;
    Minimap_Alloc_Pic();
    Minimap_Build_View();
    dest_x = p_units[unit].dest_x;
    dest_y = p_units[unit].dest_y;
    plane = p_units[unit].plane;
    owner = p_units[unit].owner;
    ux = p_units[unit].x;
    uy = p_units[unit].y;
    view_x = map_view_x;
    view_y = map_view_y;
    map_view_x = view_x;
    map_view_y = view_y;
    result = Stack_Continue_GoTo(owner, unit, dest_x, dest_y, &view_x, &view_y, plane);
}

/* (x, y, plane, player, &out x, &out y) -> the free slots of the first square of the 3 x 3 around (x, y) without a
   lair or the player's city that has room. DEAD: no callers */
int Tile_Find_Free_Near_Unused(int x, int y, int plane, int player, int *out_x, int *out_y)
{
    int y0;
    int x0;
    int span;
    int wx;
    int found;
    int free;
    int fx;
    int fy;
    int ty;
    int tx;

    y0 = y - 1;
    if (y0 < 0)
        y0 = 0;
    x0 = x - 1;
    if (x0 < 0)
        x0 = x0 + MAP_WIDTH;
    span = 3;
    found = 0;
    free = 0;
    for (ty = y0; y0 + span > ty && found == 0; ty++) {
        if (ty < MAP_HEIGHT) {
            for (tx = x0; x0 + span > tx && found == 0; tx++) {
                free = 0;
                if (tx >= MAP_WIDTH)
                    wx = tx - MAP_WIDTH;
                else
                    wx = tx;
                if (Lair_At(wx, ty, plane) == -1 && Player_City_At(wx, ty, plane, player) == -1) {
                    free = Tile_Free_Slots(wx, ty, plane, player);
                    if (free > 0) {
                        found = 1;
                        fx = wx;
                        fy = ty;
                    }
                }
            }
        }
    }
    *out_x = fx;
    *out_y = fy;
    return free;
}

/* (x, y, plane, player) -> 9 - the units there when the square is empty or holds fewer than 9 of the player's units
   and is neither water nor a tower; else 0. Only Tile_Find_Free_Near_Unused calls it (dead) */
int Tile_Free_Slots(int x, int y, int plane, int player)
{
    int count;
    int slots;
    int found;
    int units[MAX_STACK];
    int i;
    int ok;

    Map_Stack_At(x, y, plane, &count, units);
    if (count > 0) {
        if (p_units[units[0]].owner != player)
            return 0;
        if (count >= MAX_STACK)
            return 0;
    }
    ok = 0;
    slots = 0;
    if (Tile_Is_Water(x, y, plane) == 1)
        ok = 0;
    else
        ok = 1;
    found = 0;
    for (i = 0; i < NUM_TOWERS && found == 0; i++) {
        if (p_towers[i].x == x && p_towers[i].y == y)
            found = 1;
    }
    if (found == 1)
        ok = 0;
    if (ok == 1)
        slots = MAX_STACK - count;
    return slots;
}

/* (unit) pushes a unit off an overfull square: the first square of the 3 x 3 around it (row by row, x wrapping)
   that is not the combat site, not a lair, not one of the owner's cities and has room (Tile_Room_For_Unit); none ->
   the unit is lost. Edge case (READ): foreign cities are not excluded (Player_City_At tests the owner's), so an EMPTY foreign city next door can
   receive the unit without a fight. BUG #456: at the top row y0 is clamped to 0, which shifts the 3 x 3 down - rows 0 .. 2
   are tried, so the unit can land 2 rows away (x wraps correctly); clarified using ReMoM repo */
void Unit_Bump_To_Adjacent(int unit)
{
    int tx;
    int y0;
    int x0;
    int span;
    int wx;
    int found;
    int room;
    int ux;
    int uy;
    int plane;
    int player;
    int nx;
    int ny;
    int one[2];
    int ty;

    ux = p_units[unit].x;
    uy = p_units[unit].y;
    plane = p_units[unit].plane;
    player = p_units[unit].owner;
    y0 = uy - 1;
    if (y0 < 0)
        y0 = 0;
    x0 = ux - 1;
    if (x0 < 0)
        x0 = x0 + MAP_WIDTH;
    span = 3;
    found = 0;
    room = 0;
    for (ty = y0; y0 + span > ty && found == 0; ty++) {
        if (ty < MAP_HEIGHT) {
            for (tx = x0; x0 + span > tx && found == 0; tx++) {
                if (tx >= MAP_WIDTH)
                    wx = tx - MAP_WIDTH;
                else
                    wx = tx;
                if (wx == combat_loc_x && ty == combat_loc_y)
                    continue;
                if (Lair_At(wx, ty, plane) == -1 && Player_City_At(wx, ty, plane, player) == -1) {
                    room = Tile_Room_For_Unit(wx, ty, plane, player, unit);
                    if (room > 0) {
                        found = 1;
                        nx = wx;
                        ny = ty;
                    }
                }
            }
        }
    }
    if (room > 0) {
        p_units[unit].x = nx;
        p_units[unit].y = ny;
        one[0] = unit;
        Stack_Tower_Update(1, one, p_units[unit].plane);
    } else
        Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED);
}

/* (x, y, plane, player, unit) -> the room for the unit there (9 - the units) or 0: an empty square or fewer than 9
   of the player's units, never a tower; water needs Wind Walking (the unit or one there), flight or water crossing;
   on land a sailing type (move_flags 2) must fly
   Numbered: BUG #310, #321 (BUGS_AND_QUIRKS.txt) */
int Tile_Room_For_Unit(int x, int y, int plane, int player, int unit)
{
    int count;
    int room;
    int found;
    int stack_unit;
    int stack[MAX_STACK];
    int i;
    int ok;

    Map_Stack_At(x, y, plane, &count, stack);
    if (count > 0) {
        if (p_units[stack[0]].owner != player)
            return 0;
        if (count >= MAX_STACK)
            return 0;
    }
    found = 0;
    for (i = 0; i < NUM_TOWERS && found == 0; i++) {
        if (p_towers[i].x == x && p_towers[i].y == y)
            return 0;
    }
    ok = 0;
    room = 0;
    if (Tile_Is_Water(x, y, plane) == 1) {
        found = 0;
        if (Unit_Has_Wind_Walking(unit) == 1) {
            ok = 1;
            found = 1;
        }
        if (found == 0) {
            found = 0;
            for (i = 0; i < count && found == 0; i++) {
                stack_unit = stack[i];
                if (stack_unit != -1 && Unit_Has_Wind_Walking(stack_unit) == 1) {
                    ok = 1;
                    found = 1;
                }
            }
        }
        if (ok == 0 && (Unit_Can_Fly(unit) == 1 || Unit_Can_Cross_Water(unit) == 1))
            ok = 1;
    } else {
        if (unit_types[p_units[unit].type].move_flags & MOVE_SAILING) {
            if (Unit_Can_Fly(unit) == 1)
                ok = 1;
            else
                ok = 0;
        } else
            ok = 1;
    }
    if (ok == 1)
        room = MAX_STACK - count;
    return room;
}

/* (unit) -> 0 when finished (no return value otherwise): the ferry wait (status 11) - the countdown in dest_x;
   ready again (status 0) at 0 or when other units join it
   role name corrected to ReMoM's reading (10/17 audit); clarified using ReMoM repo */
int AI_Order_Ferry_Wait_Exec(int unit)
{
    int count;
    int x;
    int y;
    int plane;
    int owner;
    int units[MAX_STACK];

    if (p_units[unit].finished == 1)
        return 0;
    x = p_units[unit].x;
    y = p_units[unit].y;
    plane = p_units[unit].plane;
    owner = p_units[unit].owner;
    Tile_Units_Of_Owner(x, y, plane, owner, &count, units);
    p_units[unit].dest_x = p_units[unit].dest_x - 1;
    if (p_units[unit].dest_x < 1 || count > 1) {
        p_units[unit].order = ORDER_READY;
        p_units[unit].dest_x = 0;
    }
}

/* (unit) -> 0 when finished (no return value otherwise): the AI's road order - every unit of the owner on the square
   with status 2 goes to its destination (status 3) with road_turns = Road_Build_Turns of the first square of
   Road_Path_AI's path / the summed road power of those units (+1 per Endurance; a group with no engineer would
   divide by 0) and its road start here
   Numbered: BUG #311 (BUGS_AND_QUIRKS.txt) */
int Road_Order_Start(int unit)
{
    int path_len;                               /* written, never read */
    int x2;
    int y2;
    int x;
    int y;
    int plane;
    int owner;
    int count;
    int power;
    int units[MAX_STACK];
    int i;

    if (p_units[unit].finished == 1)
        return 0;
    x = p_units[unit].x;
    y = p_units[unit].y;
    plane = p_units[unit].plane;
    x2 = p_units[unit].dest_x;
    y2 = p_units[unit].dest_y;
    owner = p_units[unit].owner;
    Tile_Units_Of_Owner(x, y, plane, owner, &count, units);
    path_len = Road_Path_AI(x, y, x2, y2, plane, path_x, path_y);
    power = 0;
    for (i = 0; i < count; i++) {
        if (p_units[units[i]].order == ORDER_BUILD_ROAD) {
            power = power + unit_types[p_units[units[i]].type].construction;
            if (Unit_Has_Endurance(units[i]) == 1)
                power++;
        }
    }
    for (i = 0; i < count; i++) {
        if (p_units[units[i]].order == ORDER_BUILD_ROAD) {
            p_units[units[i]].order = ORDER_GOTO;
            p_units[units[i]].road_turns = Road_Build_Turns(path_x[0], path_y[0], plane);
            p_units[units[i]].road_turns = p_units[units[i]].road_turns / power;
            p_units[units[i]].road_start_x = x;
            p_units[units[i]].road_start_y = y;
        }
    }
}

/* (x, y, plane) -> 1 when the stack there can shift to the other plane: no Planar Seal; 1..8 units (9 refused -
   #174); the other plane's square empty or holding the owner's units with the total at most 9, no lair, not the
   owner's city. Land there: sailing types must fly. Water there: a Wind Walker on either side, else every unit must
   fly, swim or have a flight / water-walking item. BUG #471: Non-Corporeal and Wraith Form are not tested, though
   overland movement lets them cross water (UNITMOVE.C) - a non-corporeal stack cannot shift onto water;
   clarified using ReMoM repo
   Numbered: BUG #174 (BUGS_AND_QUIRKS.txt) */
int Stack_Can_Plane_Shift(int x, int y, int plane)
{
    int count;
    int dest_count;
    int unused;
    int ww;
    int unit;
    int player;
    int other_plane;
    int units[MAX_STACK];
    int dest_units[MAX_STACK];
    int i;
    int ok;

    if (Planar_Seal_Active() == 1)
        return 0;
    Units_At_Tile_Plane(x, y, plane, &count, units);
    if (count < 1)
        return 0;
    player = p_units[units[0]].owner;
    if (count > 0) {
        if (p_units[units[0]].owner != player)
            return 0;
        if (count >= MAX_STACK)
            return 0;
    }
    other_plane = 1 - plane;
    Units_At_Tile_Plane(x, y, other_plane, &dest_count, dest_units);
    if (dest_count > 0) {
        if (p_units[dest_units[0]].owner != player)
            return 0;
        if (dest_count > MAX_STACK)
            return 0;
        if (dest_count + count > MAX_STACK)
            return 0;
    }
    if (Lair_At(x, y, other_plane) != -1)
        return 0;
    if (Player_City_At(x, y, other_plane, player) != -1)
        return 0;
    ok = 0;
    unused = 0;
    if (Tile_Is_Water(x, y, other_plane) == 1) {
        ww = 0;
        for (i = 0; i < count; i++) {
            if (Unit_Has_Wind_Walking(units[i]) == 1) {
                ok = 1;
                ww = 1;
            }
        }
        if (ww == 0) {
            ww = 0;
            for (i = 0; i < dest_count && ww == 0; i++) {
                unit = dest_units[i];
                if (unit != -1 && Unit_Has_Wind_Walking(unit) == 1) {
                    ok = 1;
                    ww = 1;
                }
            }
            if (ww == 0) {
                for (i = 0; i < count; i++) {
                    if (Unit_Can_Fly(units[i]) == 1 || Hero_Item_Flight(units[i]) == 1 || Unit_Can_Swim(units[i]) == 1
                        || Hero_Item_Water_Walking(units[i]) == 1)
                        ok = 1;
                    else
                        return 0;
                }
            }
        }
    } else {
        ok = 1;
        for (i = 0; i < count && ok == 1; i++) {
            if (unit_types[p_units[units[i]].type].move_flags & MOVE_SAILING) {
                if (Unit_Can_Fly(units[i]) == 1)
                    ok = 1;
                else
                    ok = 0;
            } else
                ok = 1;
        }
    }
    return ok;
}
