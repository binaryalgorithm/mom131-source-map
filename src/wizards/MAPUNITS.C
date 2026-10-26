/* MAPUNITS.C - WIZARDS.EXE overlay code segment 51b2 (stub 241a): units and places on the overland map - a wizard's
   fortress and summoning-circle cities, the unit lists of a square (own, foreign, all, a city's garrison), the
   settler test of a stack, nodes and towers (a stack entering or leaving a tower), revealing the map, exploring
   around a square, and the plane button (Planar Travel with its destination check); three empty stubs.
   This module's data (DS:2f8c): its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 51b2 src/wizards/MAPUNITS.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Wizard_Fortress_City(int player);
int Wizard_Summoning_City(int player);
void City_Garrison_List(int city, int *count, int *list);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
void Tile_Units_Foreign(int x, int y, int plane, int player, int *count, int *list);
void Map_Stack_At(int x, int y, int plane, int *count, int *list);
void Map_Stack_At_Unlimited(int x, int y, int plane, int *count, int *list);
int Stack_Check_Settle(int count, int *list);
void Units_Moved_Nop(void);
void Map_Units_Uncalled_Nop_05bd(void);
int Node_At(int x, int y, int plane);
int Stack_Tower_Update(int count, int *list, int plane);
void Units_Update_In_Tower(void);
void Unit_Dismiss(int unit);
void Reveal_Map_All(void);
void Map_Units_Uncalled_Nop_093a(void);
int Map_Explore_Radius(int x, int y, int plane, int range);
void Overland_Plane_Button(int player, int *vx, int *vy, int *plane);
int Planar_Travel_Dest_Check(int lead, int dest_plane);
void Units_At_Tile_Plane(int x, int y, int plane, int *count, int *list);

#include "GAME.H"

extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int stack_special_action;                /* DS:c04e */
extern int goto_preview_on;                     /* DS:c068 */

int Tile_Settle_Blocker(int x, int y, int plane);
int Planar_Seal_Active(void);
int Map_Explore_Tile(int x, int y, int plane);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Stack_Can_Planar_Travel(void);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
int Stack_Scout_Range(int player, int x, int y, int plane);
int Move_Scout_Contacts(int player, int x, int y, int plane, int range);
void Stack_Active_Movement_Modes(int *modes);
int Tile_Is_Water(int x, int y, int plane);
int Lair_At(int x, int y, int plane);
int City_At_Copy(int x, int y, int plane);
int Path_Ward_Blocked(int city, int *n, int *units);

/* (player) -> the city on the player's active fortress, -1 (none, or the neutral player) */
int Wizard_Fortress_City(int player)
{
    int city;
    int i;

    if (player == PLAYER_NEUTRAL)
        return -1;
    city = -1;
    for (i = 0; i < num_cities && city == -1; i++) {
        if (p_fortresses[player].active == 1) {
            if (p_fortresses[player].plane == p_cities[i].plane && p_fortresses[player].x == p_cities[i].x
                && p_fortresses[player].y == p_cities[i].y)
                city = i;
        }
    }
    return city;
}

/* (player) -> the city on the player's summoning circle, -1 */
int Wizard_Summoning_City(int player)
{
    int city;
    int i;

    if (player == PLAYER_NEUTRAL)
        return -1;
    city = -1;
    for (i = 0; i < num_cities && city == -1; i++) {
        if (p_cities[i].plane == wizards[player].summoning_circle_xyp[XYP_PLANE]
            && p_cities[i].x == wizards[player].summoning_circle_xyp[XYP_X]
            && p_cities[i].y == wizards[player].summoning_circle_xyp[XYP_Y])
            city = i;
    }
    return city;
}

/* (city, &count, list) up to 9 units of the city's owner on its square (the plane must match; the tower flag is
   not used) */
void City_Garrison_List(int city, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    for (unit = 0; unit < num_units && n < MAX_STACK; unit++) {
        if (p_cities[city].plane == p_units[unit].plane && p_cities[city].x == p_units[unit].x
            && p_cities[city].y == p_units[unit].y && p_units[unit].owner == p_cities[city].owner) {
            list[n] = unit;
            n++;
        }
    }
    *count = n;
}

/* (x, y, plane, owner, &count, list) the owner's units on the square (tower units on either plane); no limit of 9 */
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    for (unit = 0; unit < num_units; unit++) {
        if ((p_units[unit].plane == plane || p_units[unit].in_tower == 1) && p_units[unit].x == x
            && p_units[unit].y == y && p_units[unit].owner == owner) {
            list[n] = unit;
            n++;
        }
    }
    *count = n;
}

/* (x, y, plane, player, &count, list) the live units on the square not owned by the player */
void Tile_Units_Foreign(int x, int y, int plane, int player, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    for (unit = 0; unit < num_units; unit++) {
        if ((p_units[unit].plane == plane || p_units[unit].in_tower == 1) && p_units[unit].x == x
            && p_units[unit].y == y && p_units[unit].owner != player && p_units[unit].owner != PLAYER_NONE) {
            list[n] = unit;
            n++;
        }
    }
    *count = n;
}

/* (x, y, plane, &count, list) up to 9 live units on the square (tower units on either plane) */
void Map_Stack_At(int x, int y, int plane, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    for (unit = 0; unit < num_units && n < MAX_STACK; unit++) {
        if ((p_units[unit].plane == plane || p_units[unit].in_tower == 1) && p_units[unit].x == x
            && p_units[unit].y == y && p_units[unit].owner != PLAYER_NONE) {
            list[n] = unit;
            n++;
        }
    }
    *count = n;
}

/* (x, y, plane, &count, list) the live units on the square on exactly that plane (in_tower ignored) */
void Units_At_Tile_Plane(int x, int y, int plane, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].plane == plane && p_units[unit].x == x && p_units[unit].y == y && p_units[unit].owner != PLAYER_NONE) {
            list[n] = unit;
            n++;
        }
    }
    *count = n;
}

/* (x, y, plane, &count, list) Map_Stack_At without the limit of 9 (no callers) */
void Map_Stack_At_Unlimited(int x, int y, int plane, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    for (unit = 0; unit < num_units; unit++) {
        if ((p_units[unit].plane == plane || p_units[unit].in_tower == 1) && p_units[unit].x == x
            && p_units[unit].y == y && p_units[unit].owner != PLAYER_NONE) {
            list[n] = unit;
            n++;
        }
    }
    *count = n;
}

/* (count, list) -> 1 when the stack holds a settler (ability 0x20, not undead - mutation_flags 0x20) and its
   first unit's square can be settled: stack_special_action = 1 */
int Stack_Check_Settle(int count, int *list)
{
    int found;
    int unit_type;
    int i;
    int unit;

    found = 0;
    for (i = 0; i < count && found == 0; i++) {
        unit = list[i];
        unit_type = p_units[unit].type;
        if ((unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) && (p_units[unit].mutation_flags & MUT_UNDEAD) == 0)
            found = 1;
    }
    unit = list[0];
    if (found == 1 && Tile_Settle_Blocker(p_units[unit].x, p_units[unit].y, p_units[unit].plane) == SETTLE_OK) {
        stack_special_action = STACK_SPECIAL_SETTLE;
        return 1;
    }
    return 0;
}

/* () an empty stub */
void Units_Moved_Nop(void)
{
}

/* () an empty stub (no callers) */
void Map_Units_Uncalled_Nop_05bd(void)
{
}

/* (x, y, plane) -> the node there (0..29), -1 */
int Node_At(int x, int y, int plane)
{
    int found;
    int node;

    found = -1;
    for (node = 0; node < NUM_NODES && found == -1; node++) {
        if (p_nodes[node].plane == plane && p_nodes[node].x == x && p_nodes[node].y == y)
            found = node;
    }
    return found;
}

/* (count, list, plane) -> 0 for an empty stack (no return value otherwise): a stack standing on a tower (and no
   Planar Seal) is in the tower - the tower takes the stack's owner, and the current player sees the square on both
   planes; else units leaving a tower are put on the given plane */
int Stack_Tower_Update(int count, int *list, int plane)
{
    int unit;
    int found;
    int tower;
    int ux;
    int uy;
    int i;

    if (count < 1)
        return 0;
    ux = p_units[list[0]].x;
    uy = p_units[list[0]].y;
    found = 0;
    for (i = 0; i < NUM_TOWERS && found == 0; i++) {
        if (p_towers[i].x == ux && p_towers[i].y == uy) {
            found = 1;
            tower = i;
        }
    }
    if (found == 1 && Planar_Seal_Active() == 1)
        found = 0;
    if (found == 1) {
        for (i = 0; i < count; i++) {
            unit = list[i];
            p_units[unit].in_tower = 1;
        }
        unit = list[0];
        p_towers[tower].owner = p_units[unit].owner;
        if (p_units[unit].owner == current_player) {
            Map_Explore_Tile(p_towers[tower].x, p_towers[tower].y, 0);
            Map_Explore_Tile(p_towers[tower].x, p_towers[tower].y, 1);
        }
    } else {
        for (i = 0; i < count; i++) {
            unit = list[i];
            if (p_units[unit].in_tower == 1) {
                p_units[unit].plane = plane;
                p_units[unit].in_tower = 0;
            }
        }
    }
}

/* () every unit on a tower square is in the tower (in_tower 1) - unless Planar Seal is in effect */
void Units_Update_In_Tower(void)
{
    int seal;
    int unit;
    int tower;

    seal = Planar_Seal_Active();
    for (unit = 0; unit < num_units; unit++) {
        p_units[unit].in_tower = 0;
        for (tower = 0; tower < NUM_TOWERS; tower++) {
            if (p_units[unit].x == p_towers[tower].x && p_units[unit].y == p_towers[tower].y) {
                if (seal == 1)
                    p_units[unit].in_tower = 0;
                else
                    p_units[unit].in_tower = 1;
            }
        }
    }
}

/* (unit) Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED): removed as dismissed, not killed */
void Unit_Dismiss(int unit)
{
    Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED);
}

/* () the whole map explored on both planes, every wizard in contact with every other, Nature Awareness on for the
   current player (spell 0x22; also the magic screen's "RVL" cheat) */
void Reveal_Map_All(void)
{
    int x;
    int y;

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            p_exploration_maps[0][y][x] = EXPLORED_ALL;
            p_exploration_maps[1][y][x] = EXPLORED_ALL;
        }
    }
    for (x = 0; x < num_players; x++) {
        for (y = 0; y < num_players; y++) {
            wizards[y].contacted[x] = 1;
            wizards[x].contacted[y] = 1;
        }
    }
    wizards[current_player].globals.nature_awareness = 1;
    Visibility_Rebuild();
    Map_Invalidate();
}

/* () an empty stub (no callers) */
void Map_Units_Uncalled_Nop_093a(void)
{
}

/* (x, y, plane, range) -> 0 for range 0 or 1 (no return value otherwise): explores the (2 x range - 1) squares
   wide square around (x, y) (x wraps, rows past 39 skipped) */
int Map_Explore_Radius(int x, int y, int plane, int range)
{
    int ty;
    int wx;
    int x0;
    int y0;
    int tx;

    if (range == 0)
        return 0;
    if (range == 1) {
        Map_Explore_Tile(x, y, plane);
        return 0;
    }
    range = range - 1;
    y0 = y - range;
    if (y0 < 0)
        y0 = 0;
    x0 = x - range;
    if (x0 < 0)
        x0 = x0 + MAP_WIDTH;
    range = range * 2 + 1;
    for (ty = y0; y0 + range > ty; ty++) {
        if (ty < MAP_HEIGHT) {
            for (tx = x0; x0 + range > tx; tx++) {
                if (tx >= MAP_WIDTH)
                    wx = tx - MAP_WIDTH;
                else
                    wx = tx;
                Map_Explore_Tile(wx, ty, plane);
            }
        }
    }
}

/* (player, &vx, &vy, &plane) the plane button: the view changes plane; a selected stack that can Planar Travel
   goes with it (its active units) unless Planar Seal (message, no change) or the destination blocks (message);
   otherwise the view changes plane and the selection stays (the drop under *plane == old is dead: the plane was
   already flipped) - except a stack standing in a tower under Planar Seal keeps the plane */
void Overland_Plane_Button(int player, int *vx, int *vy, int *plane)
{
    int old;
    int i;
    int unit;
    int result;
    int ux;
    int uy;
    int scout;
    int check;
    int found;

    goto_preview_on = 0;
    old = *plane;
    *plane = (*plane + 1) % 2;
    result = 0;
    if (no_units_to_move == 0 && stack_unit_count > 0) {
        result = Stack_Can_Planar_Travel();
        if (result == 1 && Planar_Seal_Active() == 1) {
            Message_Box("Planar Seal prevents your units from changing planes.");
            *plane = old;
            result = PLANAR_SEALED;
        }
    }
    if (result == 1) {
        check = Planar_Travel_Dest_Check(stack_units[0].unit, *plane);
        switch (check) {
        case PLANAR_BLOCKED:
            *plane = old;
            Message_Box("The selected units cannot Planar Travel at this location.");
            Map_Invalidate();
            break;
        case PLANAR_GO:
            for (i = 0; i < stack_unit_count; i++) {
                if (stack_units[i].active == 1) {
                    unit = stack_units[i].unit;
                    p_units[unit].plane = *plane;
                }
            }
            unit = stack_units[0].unit;
            ux = p_units[unit].x;
            uy = p_units[unit].y;
            Stack_Select_At(player, vx, vy, *plane, ux, uy);
            scout = Stack_Scout_Range(player, ux, uy, *plane);
            Map_Explore_Radius(ux, uy, *plane, scout);
            Move_Scout_Contacts(player, ux, uy, *plane, scout);
            Visibility_Rebuild();
            Map_Invalidate();
            break;
        }
    } else if (result != PLANAR_SEALED) {
        if (*plane == old) {                    /* never true: flipped above */
            *plane = (*plane + 1) % 2;
            stack_unit_count = 0;
        }
        if (stack_unit_count > 0 && Planar_Seal_Active() == 1) {
            found = 0;
            unit = stack_units[0].unit;
            ux = p_units[unit].x;
            uy = p_units[unit].y;
            for (i = 0; i < NUM_TOWERS && found == 0; i++) {
                if (p_towers[i].x == ux && p_towers[i].y == uy)
                    found = 1;
            }
            if (found == 1) {
                *plane = old;
                Message_Box("Planar Seal prevents your units from changing planes.");
            }
        }
    }
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(*vx, *vy, *plane);
}

/* (lead unit, destination plane) -> -1 the stack may go, -2 blocked: water there needs a flying, swimming or
   sailing mode among the active units; a lair blocks; a city blocks when warded against the stack (BUG #175:
   checked on a count and list never filled), when foreign, or when the units there and the stack would pass 9;
   an open square blocks with foreign units on it, or past 9 own units
   Numbered: BUG #277 (BUGS_AND_QUIRKS.txt) */
int Planar_Travel_Dest_Check(int lead, int dest_plane)
{
    int count;
    int x;
    int y;
    int site;                                   /* a lair (Lair_At), then a city */
    int ward;
    int units[MAX_STACK];
    int modes[6];
    int result;

    result = PLANAR_GO;
    Stack_Active_Movement_Modes(modes);
    if (Tile_Is_Water(p_units[lead].x, p_units[lead].y, dest_plane) == 1) {
        result = PLANAR_BLOCKED;
        if (modes[MOVE_MODE_SWIMMING] > 0 || modes[MOVE_MODE_SAILING] > 0 || modes[MOVE_MODE_FLYING] > 0)
            result = PLANAR_GO;
    }
    if (result == PLANAR_GO) {
        x = p_units[lead].x;
        y = p_units[lead].y;
        site = Lair_At(x, y, dest_plane);
        if (site != -1)
            result = PLANAR_BLOCKED;
        else {
            site = City_At_Copy(x, y, dest_plane);
            if (site != -1) {
                ward = Path_Ward_Blocked(site, &count, units);
                if (ward == 1)
                    result = PLANAR_BLOCKED;
                else if (p_cities[site].owner != p_units[lead].owner)
                    result = PLANAR_BLOCKED;
                else {
                    Tile_Units_Of_Owner(x, y, dest_plane, p_units[lead].owner, &count, units);
                    if (stack_unit_count + count > MAX_STACK)
                        result = PLANAR_BLOCKED;
                }
            } else {
                count = 0;
                Tile_Units_Foreign(x, y, dest_plane, p_units[lead].owner, &count, units);
                if (count > 0)
                    result = PLANAR_BLOCKED;
                else {
                    count = 0;
                    Tile_Units_Of_Owner(p_units[lead].x, p_units[lead].y, dest_plane, p_units[lead].owner, &count, units);
                    if (stack_unit_count + count > MAX_STACK)
                        result = PLANAR_BLOCKED;
                }
            }
        }
    }
    return result;
}
