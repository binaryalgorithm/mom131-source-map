/* AIORDERS.C - WIZARDS.EXE overlay code segment f48f (stub 2693): AI stack orders and landmass planning - an older
   family of stack orders (release, purify, wait, meld, patrol, plane shift, settlers / escorts / combat units /
   engineers / transports go-to, disembark, fill to 9, build road; most of them dead code in 1.31), search helpers
   (nearest coast / water / free land / landing tile, unclaimed landmass, free node, monster groups), the garrison
   rule for leaving a city, the per-landmass plan, the threat map, the attack landmass choice and order cancelling.
   This module has no data of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS f48f src/wizards/AIORDERS.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void AI_Stack_Release(int stack);
void AI_Stack_Order_Purify(int stack);
int AI_Stack_Order_Wait(int stack);
void AI_Stack_Order_Meld_Or_Patrol(int stack);
void AI_Stack_Order_Patrol_All(int stack);
void AI_Stack_Order_Plane_Shift(int stack);
int AI_Stack_Order_Settlers_Go(int stack, int tx, int ty, int n, int m);
int AI_Stack_Order_Send_N(int stack, int tx, int ty, int n, int m);
int AI_Stack_Order_Send_Water_Capable(int stack, int tx, int ty);
int AI_Stack_Order_Settler_Escort(int stack, int tx, int ty);
int AI_Stack_Order_Send_Combat_N(int stack, int tx, int ty, int n);
int AI_Stack_Order_Attack_Tile(int stack, int tx, int ty, int player);
int AI_Stack_Order_Engineers_Go(int stack, int tx, int ty);
void AI_Stack_Order_Disembark(int stack, int tx, int ty);
int AI_Stack_Order_Fill_To_9(int stack, int tx, int ty);
int AI_Stack_Order_Meld_Plus_Escort(int stack, int tx, int ty);
int AI_Stack_Order_Build_Road(int stack, int tx, int ty);
int AI_Stack_Order_Send_NonSettlers_N(int stack, int tx, int ty, int n);
void AI_Order_Transports_Goto(int stack, int tx, int ty);
int AI_Dist_To_Nearest_Human_Unit(int x, int y, int plane);
void AI_Nearest_Coast_Tile(int x, int y, int plane, int *cx, int *cy);
int AI_Nearest_Land_Of_Landmass(int x, int y, int plane, int landmass, int *wx, int *wy);
int AI_Adjacent_Free_Land(int x, int y, int plane, int *fx, int *fy);
int AI_Find_Unclaimed_Landmass_Tile(int player, int x, int y, int plane, int *fx, int *fy);
int AI_Tile_Enemy_Strength(int player, int x, int y, int plane);
int AI_Enemy_Stack_On_Landmass(int landmass);
int AI_Find_City_For_Node_Spirit(int player, int *city_out);
int AI_Landmass_Has_Free_Node(int player, int city);
int AI_Find_Landing_Tile(int stack, int unused, int x, int y, int plane, int *fx, int *fy);
int AI_Stack_Unit_Count(int stack);
int AI_Units_Near_Tile(int x, int y, int plane);
int AI_Adjacent_Tile_Pick(int x, int y, int plane, int *fx, int *fy);
int AI_Roll_Monster_Group(int budget, int race_realm, int *out);
int AI_Stack_Garrison_OK(int stack, int x, int y, int plane);
void AI_Build_Threat_Map(int player);
int AI_Landmass_Plan(int player);
void AI_Choose_Attack_Landmass(int player);
int AI_Cancel_Unit_Orders(int player);

#include "GAME.H"

extern struct LandmassPlan ai_landmass_plan[2][5];  /* DS:8890 */
extern int *p_ai_expand_y[2];                   /* DS:8f9a */
extern int *p_ai_expand_x[2];                   /* DS:8f9e */
extern int *p_ai_replan_timer;                  /* DS:8faa */
extern int *p_ai_expand_landmass[2];            /* DS:8fac */
extern int *p_ai_attack_landmass[2];            /* DS:8fb0 */
extern char ai_landmass_claimed_bits[2][8];     /* DS:8fb4  per plane, a bit per landmass; never written (all 0), read in AIORDERS.C */
extern int *ai_lm_surplus[2];                   /* DS:90f2 */
extern struct AIEnemyStack *p_ai_enemy_stacks;  /* DS:9106 */
extern struct AIStack *p_ai_own_stacks;             /* DS:9108 */
extern int ai_enemy_stack_count;                /* DS:910a */
extern int ai_own_stack_count;                  /* DS:910c */
extern unsigned far *p_threat_maps[2];          /* DS:9c94 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Fortress far *p_fortresses;       /* DS:9cc8 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern int num_players;                         /* DS:bd9c */
extern int far *p_coast_tile_head[2];           /* DS:9d2a */
extern int far *p_coast_tile_next[2];           /* DS:9d32 */
extern char far *p_coast_tile_y[2];             /* DS:9d3a */
extern char far *p_coast_tile_x[2];             /* DS:9d42 */
extern int far *p_landmass_tile_head[2];        /* DS:9ea0 */
extern int far *p_landmass_tile_next[2];        /* DS:9ea8 */
extern char far *p_landmass_tile_y[2];          /* DS:9eb0 */
extern char far *p_landmass_tile_x[2];          /* DS:9eb8 */

int Stack_Can_Plane_Shift(int x, int y, int plane);
int Unit_Can_Fly(int unit);
int Unit_Can_Cross_Water(int unit);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int Tile_Is_Land(int x, int y, int plane);
int abs(int x);
int Bit_Test_Near(int bit, char *map);
int AI_Unit_Strength(int unit);
int UnitType_AI_Strength(int unit_type);
int Random(int n);
void AI_Map_EMS_Tables(void);
int Random_Weighted_Pick(int *weights, int count);
void EMS_Map_Default(void);

/* (stack) DEAD CODE: the units on the stack's square are released (settlers: ORDER_SETTLE), the stack is
   handled (strength -1) */
void AI_Stack_Release(int stack)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            if (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST)
                p_units[unit].order = ORDER_SETTLE;
            else
                p_units[unit].order = ORDER_PATROL;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack) DEAD CODE: the stack's units get ORDER_PURIFY */
void AI_Stack_Order_Purify(int stack)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane)
            p_units[unit].order = ORDER_PURIFY;
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack) DEAD CODE: if the garrison may leave, the stack's units wait (ORDER_AI_WAIT, dest x 10); 0 if it may not */
int AI_Stack_Order_Wait(int stack)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            p_units[unit].order = ORDER_AI_WAIT;
            p_units[unit].dest_x = 10;          /* ORDER_AI_WAIT's countdown (ORDERS.C counts dest_x down) */
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
    p_ai_own_stacks[stack].status = ORDER_AI_WAIT;
}

/* (stack) DEAD CODE: melders meld (ORDER_MELD), the others patrol (ORDER_PATROL) */
void AI_Stack_Order_Meld_Or_Patrol(int stack)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            if (unit_types[p_units[unit].type].ability_flags & AFLAG_MELD_WITH_NODE)
                p_units[unit].order = ORDER_MELD;
            else
                p_units[unit].order = ORDER_PATROL;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack) DEAD CODE: the stack's units patrol (ORDER_PATROL); the stack stays unhandled */
void AI_Stack_Order_Patrol_All(int stack)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane)
            p_units[unit].order = ORDER_PATROL;
}

/* (stack) DEAD CODE: a stack able to plane shift moves to the other plane */
void AI_Stack_Order_Plane_Shift(int stack)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (Stack_Can_Plane_Shift(x, y, plane) == 1) {
        for (unit = 0; unit < num_units; unit++) {
            if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
                p_units[unit].order = ORDER_READY;
                p_units[unit].plane = 1 - p_units[unit].plane;
            }
        }
        p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
    }
}

/* (stack, x, y, n, m) DEAD CODE: the settlers go to (x, y) - first all of them, counting n and m down, then those
   (again) while both counts last; the settlers beyond the counts patrol, other units are untouched. With no caller,
   what n and m count is not known: two caps counted down together */
int AI_Stack_Order_Settlers_Go(int stack, int tx, int ty, int n, int m)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST)) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
            n--;
            m--;
        }
    }
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST)) {
            if (n > 0 && m > 0) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
                n--;
                m--;
            } else
                p_units[unit].order = ORDER_PATROL;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y, n, m) DEAD CODE: units go to (x, y) while both counts last, the rest patrol */
int AI_Stack_Order_Send_N(int stack, int tx, int ty, int n, int m)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            if (n > 0 && m > 0) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
                n--;
                m--;
            } else
                p_units[unit].order = ORDER_PATROL;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: units that can cross water or fly (or all, for an AISTACK_WIND_WALKER stack) go to (x, y), the rest
   patrol */
int AI_Stack_Order_Send_Water_Capable(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            if ((p_ai_own_stacks[stack].flags & AISTACK_WIND_WALKER) || Unit_Can_Fly(unit) || Unit_Can_Cross_Water(unit)) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
            } else
                p_units[unit].order = ORDER_PATROL;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: the settlers go to (x, y) with up to a third of the stack (no transports) as escort */
int AI_Stack_Order_Settler_Escort(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int sent;
    int limit;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    limit = AI_Stack_Unit_Count(stack) / 3;
    sent = 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            if (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
            } else if (unit_types[p_units[unit].type].transport_capacity == 0 && sent < limit) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
                sent++;
            }
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y, n) DEAD CODE: up to n combat units (no settlers, engineers, transports or melders) go to (x, y) */
int AI_Stack_Order_Send_Combat_N(int stack, int tx, int ty, int n)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units && n > 0; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 &&
            unit_types[p_units[unit].type].construction == 0 && unit_types[p_units[unit].type].transport_capacity == 0 &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_MELD_WITH_NODE) == 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
            n--;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, tx, ty, player) refuses (0) to attack a square held by a wizard the player has peace / a treaty with, or when the
   garrison may not leave; else all non-settlers go to (x, y) */
int AI_Stack_Order_Attack_Tile(int stack, int tx, int ty, int player)
{
    int x;
    int y;
    int plane;
    int found;
    int owner;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    found = 0;
    for (unit = 0; unit < num_units && found == 0; unit++) {
        if (p_units[unit].x == tx && p_units[unit].y == ty && p_units[unit].plane == plane) {
            found = 1;
            owner = p_units[unit].owner;
        }
    }
    if (found == 1 && owner != player && owner != PLAYER_NEUTRAL &&
        (wizards[player].peace_turns[owner] != 0 || wizards[player].diplomacy_status[owner] == TREATY_WIZARD_PACT ||
         wizards[player].diplomacy_status[owner] == TREATY_ALLIANCE))
        return 0;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) == 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: the engineers go to (x, y) */
int AI_Stack_Order_Engineers_Go(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            unit_types[p_units[unit].type].construction > 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: everything but the transports goes to (x, y) */
void AI_Stack_Order_Disembark(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            unit_types[p_units[unit].type].transport_capacity == 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: non-settlers go to (x, y) until the stacks there hold 9 units (the count loop runs
   over num_units stacks) */
int AI_Stack_Order_Fill_To_9(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int room;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    room = MAX_STACK;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++)
        if (p_ai_own_stacks[unit].x == tx && p_ai_own_stacks[unit].y == ty)
            room -= p_ai_own_stacks[unit].count;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 && room > 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
            room--;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: the melders go to (x, y) with one escort (none from a city stack of fewer than 3) */
int AI_Stack_Order_Meld_Plus_Escort(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int escorted;
    int count;
    int in_city;
    int city;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    count = p_ai_own_stacks[stack].count;
    in_city = 0;
    for (city = 0; city < num_cities; city++)
        if (p_cities[city].x == x && p_cities[city].y == y && p_cities[city].plane == plane)
            in_city = 1;
    escorted = 0;
    if (in_city == 1 && count < 3)
        escorted = 1;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            if (unit_types[p_units[unit].type].ability_flags & AFLAG_MELD_WITH_NODE) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
            } else if (unit_types[p_units[unit].type].transport_capacity == 0 && escorted == 0) {
                p_units[unit].order = ORDER_GOTO;
                p_units[unit].dest_x = tx;
                p_units[unit].dest_y = ty;
                escorted = 1;
            }
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) DEAD CODE: the engineers build a road from the stack's square to (x, y) (ORDER_BUILD_ROAD) */
int AI_Stack_Order_Build_Road(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            unit_types[p_units[unit].type].construction > 0) {
            p_units[unit].order = ORDER_BUILD_ROAD;
            p_units[unit].road_start_x = x;
            p_units[unit].road_start_y = y;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y, n) DEAD CODE: up to n non-settlers go to (x, y) */
int AI_Stack_Order_Send_NonSettlers_N(int stack, int tx, int ty, int n)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    if (AI_Stack_Garrison_OK(stack, x, y, plane) == 0)
        return 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 && n > 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
            n--;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (stack, x, y) the transports on the stack's square go to (x, y); the stack is handled */
void AI_Order_Transports_Goto(int stack, int tx, int ty)
{
    int x;
    int y;
    int plane;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane &&
            unit_types[p_units[unit].type].transport_capacity > 0) {
            p_units[unit].order = ORDER_GOTO;
            p_units[unit].dest_x = tx;
            p_units[unit].dest_y = ty;
        }
    }
    p_ai_own_stacks[stack].strength = AISTACK_ORDERED;
}

/* (x, y, plane) the distance (the larger axis, no wrap) to the nearest human unit on the plane, at most 100 */
int AI_Dist_To_Nearest_Human_Unit(int x, int y, int plane)
{
    int best;
    int dist;
    int dx;
    int dy;
    int unit;

    best = 100;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner == PLAYER_HUMAN && p_units[unit].plane == plane) {
            dx = x - p_units[unit].x;
            if (dx < 0)
                dx = -dx;
            dy = y - p_units[unit].y;
            if (dy < 0)
                dy = -dy;
            if (dx < dy)
                dist = dy;
            else
                dist = dx;
            if (dist < best)
                best = dist;
        }
    }
    return best;
}

/* (x, y, plane, *cx, *cy) the nearest coast tile of (x, y)'s landmass with an unthreatened land square next to it;
   (x, y) itself when there is none */
void AI_Nearest_Coast_Tile(int x, int y, int plane, int *cx, int *cy)
{
    int dx;
    int dy;
    int best_x;
    int best_y;
    int tx;
    int ty;
    int dist;
    int best;
    int tile;
    int landmass;
    int found;

    landmass = p_landmass_ids[plane][y][x];
    best = 2000;
    for (tile = p_coast_tile_head[plane][landmass]; tile != -1; tile = p_coast_tile_next[plane][tile]) {
        tx = p_coast_tile_x[plane][tile];
        ty = p_coast_tile_y[plane][tile];
        found = 0;
        for (dy = -1; dy < 2 && found == 0; dy++)
            for (dx = -1; dx < 2 && found == 0; dx++)
                if (p_landmass_ids[plane][dy + ty][dx + tx] != 0 &&
                    p_threat_maps[plane][(dy + ty) * MAP_WIDTH + dx + tx] == 0)
                    found = 1;
        if (found == 1) {
            dist = Map_Distance(tx, ty, x, y, MAP_WIDTH);
            if (dist < best) {
                best = dist;
                best_x = tx;
                best_y = ty;
            }
        }
    }
    if (best != 2000) {
        *cx = best_x;
        *cy = best_y;
    } else {
        *cx = x;
        *cy = y;
    }
}

/* (x, y, plane, landmass, *wx, *wy) the nearest land square of the landmass within 10 squares: 1 and its position,
   else 0 and (x, y) */
int AI_Nearest_Land_Of_Landmass(int x, int y, int plane, int landmass, int *wx, int *wy)
{
    int best_x;
    int best_y;
    int dist;
    int best;
    int adx;
    int ady;
    int xx;
    int yy;
    int wrap_x;

    best = 2000;
    for (yy = y - 10; yy < y + 11; yy++) {
        if (yy > 0 && yy < MAP_HEIGHT) {
            for (xx = x - 10; xx < x + 11; xx++) {
                wrap_x = xx;
                if (wrap_x < 0)
                    wrap_x += MAP_WIDTH;
                if (wrap_x >= MAP_WIDTH)
                    wrap_x -= MAP_WIDTH;
                if (Tile_Is_Land(wrap_x, yy, plane) == 1 && p_landmass_ids[plane][yy][wrap_x] == landmass) {
                    adx = abs(wrap_x - x);
                    ady = abs(yy - y);
                    dist = adx > ady ? adx : ady;
                    if (dist < best) {
                        best = dist;
                        best_x = wrap_x;
                        best_y = yy;
                    }
                }
            }
        }
    }
    if (best != 2000) {
        *wx = best_x;
        *wy = best_y;
        return 1;
    } else {
        *wx = x;
        *wy = y;
        return 0;
    }
}

/* (x, y, plane, *fx, *fy) the first unthreatened land square around (x, y): 1 and its position, else 0 */
int AI_Adjacent_Free_Land(int x, int y, int plane, int *fx, int *fy)
{
    int xx;
    int yy;
    int wrap_x;

    for (yy = y - 1; yy < y + 2; yy++) {
        if (yy > 0 && yy < MAP_HEIGHT) {
            for (xx = x - 1; xx < x + 2; xx++) {
                wrap_x = xx;
                if (wrap_x < 0)
                    wrap_x += MAP_WIDTH;
                if (wrap_x >= MAP_WIDTH)
                    wrap_x -= MAP_WIDTH;
                if (Tile_Is_Land(wrap_x, yy, plane) == 1 && p_threat_maps[plane][yy * MAP_WIDTH + wrap_x] == 0) {
                    *fx = wrap_x;
                    *fy = yy;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* (player, x, y, plane, *fx, *fy) DEAD CODE: the nearest unthreatened square (2..57, 2..37) of a landmass nobody
   claims - not in the claimed bits, without an own settler stack (AISTACK_SETTLER), an enemy stack or one of the player's cities;
   1 and its position, else 0 and (x, y). Bug: the claimed bits are read at &bits[plane][i] instead of bits[plane] */
int AI_Find_Unclaimed_Landmass_Tile(int player, int x, int y, int plane, int *fx, int *fy)
{
    int landmass;
    int best;
    int best_x;
    int best_y;
    int dist;
    int adx;
    int ady;
    int tx;
    int ty;
    char claimed[MAX_LANDMASSES];
    int i;

    for (i = 0; i < MAX_LANDMASSES; i++) {
        if (Bit_Test_Near(i, &ai_landmass_claimed_bits[plane][i]))
            claimed[i] = 1;
        else
            claimed[i] = 0;
    }
    for (i = 0; i < ai_own_stack_count; i++)
        if ((p_ai_own_stacks[i].flags & AISTACK_SETTLER) && (int)p_ai_own_stacks[i].landmass < MAX_LANDMASSES)
            claimed[p_ai_own_stacks[i].landmass] = 1;
    for (i = 0; i < ai_enemy_stack_count; i++)
        if ((int)p_ai_enemy_stacks[i].landmass < MAX_LANDMASSES)
            claimed[p_ai_enemy_stacks[i].landmass] = 1;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player && p_cities[i].plane == plane) {
            landmass = p_landmass_ids[plane][p_cities[i].y][p_cities[i].x];
            if (landmass < MAX_LANDMASSES)
                claimed[landmass] = 1;
        }
    }
    claimed[0] = 1;
    best = 1000;
    for (ty = 2; ty < MAP_HEIGHT - 2; ty++) {
        for (tx = 2; tx < MAP_WIDTH - 2; tx++) {
            landmass = p_landmass_ids[plane][ty][tx];
            if (claimed[landmass] == 0 && p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0) {
                adx = abs(x - tx);
                ady = abs(y - ty);
                dist = adx > ady ? adx : ady;
                if (dist < best) {
                    best = dist;
                    best_x = tx;
                    best_y = ty;
                }
            }
        }
    }
    if (best != 1000) {
        *fx = best_x;
        *fy = best_y;
        return 1;
    } else {
        *fx = x;
        *fy = y;
        return 0;
    }
}

/* (player, x, y, plane) DEAD CODE: the strength / 10 of the other players' units on the square plus the guardians
   of an active lair there. Bug: AI_Unit_Strength is passed the unit's TYPE as a unit index */
int AI_Tile_Enemy_Strength(int player, int x, int y, int plane)
{
    int total;
    int i;

    total = 0;
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner != player && p_units[i].x == x && p_units[i].y == y && p_units[i].plane == plane)
            total += AI_Unit_Strength(p_units[i].type) / 10;
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].active == 1 && p_lairs[i].x == x && p_lairs[i].y == y && p_lairs[i].plane == plane) {
            total += UnitType_AI_Strength(p_lairs[i].guard1_type) / 10 * (p_lairs[i].guard1_count & LAIR_GUARDS_LEFT_MASK);
            total += UnitType_AI_Strength(p_lairs[i].guard2_type) / 10 * (p_lairs[i].guard2_count & LAIR_GUARDS_LEFT_MASK);
        }
    }
    return total;
}

/* (landmass) DEAD CODE: 1 if an enemy stack stands on the landmass */
int AI_Enemy_Stack_On_Landmass(int landmass)
{
    int i;

    for (i = 0; i < ai_enemy_stack_count; i++)
        if (p_ai_enemy_stacks[i].landmass == landmass)
            return 1;
    return 0;
}

/* (player, *city_out) DEAD CODE: 1 when the summoning circle's city (else the city with the player's index) or any
   of the player's cities has a free node on its landmass; *city_out = the circle's city either way (bug: not the
   city that was found) */
int AI_Find_City_For_Node_Spirit(int player, int *city_out)
{
    int circle_city;
    int city;

    circle_city = player;
    for (city = 0; city < num_cities; city++)
        if (p_cities[city].x == wizards[player].summoning_circle_xyp[XYP_X] &&
            p_cities[city].y == wizards[player].summoning_circle_xyp[XYP_Y] &&
            p_cities[city].plane == wizards[player].summoning_circle_xyp[XYP_PLANE])
            circle_city = city;
    if (AI_Landmass_Has_Free_Node(player, circle_city) == 1) {
found:
        *city_out = circle_city;
        return 1;
    }
    for (city = 0; city < num_cities; city++)
        if (p_cities[city].owner == player && AI_Landmass_Has_Free_Node(player, city) == 1)
            goto found;
    return 0;
}

/* (player, city) DEAD CODE: 1 if a node on the city's landmass belongs to someone else and has no active lair, no
   enemy stack and no melder of the player's on the landmass */
int AI_Landmass_Has_Free_Node(int player, int city)
{
    int node;
    int unused;
    int taken;
    int k;
    int plane;
    int landmass;
    int lair;
    int unit;

    plane = p_cities[city].plane;
    landmass = p_landmass_ids[plane][p_cities[city].y][p_cities[city].x];
    unused = 0;
    for (node = 0; node < NUM_NODES; node++) {
        if (p_nodes[node].plane == plane && p_nodes[node].owner != player &&
            p_landmass_ids[plane][p_nodes[node].y][p_nodes[node].x] == landmass) {
            taken = 0;
            for (lair = 0; lair < NUM_LAIRS; lair++)
                if (p_lairs[lair].x == p_nodes[node].x && p_lairs[lair].y == p_nodes[node].y &&
                    p_lairs[lair].plane == p_nodes[node].plane && p_lairs[lair].active == 1)
                    taken = 1;
            for (k = 0; k < ai_enemy_stack_count && taken == 0; k++)
                if (p_ai_enemy_stacks[k].x == p_nodes[node].x && p_ai_enemy_stacks[k].y == p_nodes[node].y &&
                    p_ai_enemy_stacks[k].plane == p_nodes[node].plane)
                    taken = 1;
            if (taken == 0) {
                for (unit = 0; unit < num_units; unit++)
                    if (p_units[unit].owner == player && p_units[unit].plane == plane &&
                        (unit_types[p_units[unit].type].ability_flags & AFLAG_MELD_WITH_NODE) &&
                        p_landmass_ids[plane][p_units[unit].y][p_units[unit].x] == landmass)
                        taken = 1;
                if (taken == 0)
                    return 1;
            }
        }
    }
    return 0;
}

/* (stack, unused, x, y, plane, *fx, *fy) DEAD CODE: the first unthreatened square around (x, y) on a landmass
   without a surplus (stacks flagged AISTACK_SETTLER: only claimed landmasses); 1 and its position, else 0. Bug: the stack loop
   overwrites the plane parameter with the last own stack's plane */
int AI_Find_Landing_Tile(int stack, int unused, int x, int y, int plane, int *fx, int *fy)
{
    int tx;
    int ty;
    int claimed_only;
    int landmass;
    int tile_landmass;
    char marked[MAX_LANDMASSES];
    int i;

    if (p_ai_own_stacks[stack].flags & AISTACK_SETTLER)
        claimed_only = 1;
    else
        claimed_only = 0;
    for (i = 0; i < MAX_LANDMASSES; i++)
        marked[i] = 0;
    for (i = 0; i < ai_own_stack_count; i++) {
        plane = p_ai_own_stacks[i].plane;
        landmass = p_ai_own_stacks[i].landmass;
        if (landmass < MAX_LANDMASSES && ai_lm_surplus[plane][landmass] > 0)
            marked[landmass] = 1;
    }
    marked[0] = 1;
    for (ty = y - 1; ty < y + 2; ty++) {
        if (ty >= 0 && ty < MAP_HEIGHT) {
            for (tx = x - 1; tx < x + 2; tx++) {
                if (tx >= 0 && tx < MAP_WIDTH) {
                    tile_landmass = p_landmass_ids[plane][ty][tx];
                    if (marked[tile_landmass] == 0 &&
                        (claimed_only == 0 || Bit_Test_Near(tile_landmass, ai_landmass_claimed_bits[plane])) &&
                        p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0) {
                        *fx = tx;
                        *fy = ty;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* (stack) the number of units on the stack's square */
int AI_Stack_Unit_Count(int stack)
{
    int x;
    int y;
    int plane;
    int count;
    int unit;

    x = p_ai_own_stacks[stack].x;
    y = p_ai_own_stacks[stack].y;
    plane = p_ai_own_stacks[stack].plane;
    count = 0;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].plane == plane && p_units[unit].x == x && p_units[unit].y == y)
            count++;
    return count;
}

/* (x, y, plane) DEAD CODE: the units of any owner within one square of (x, y) (no wrap) */
int AI_Units_Near_Tile(int x, int y, int plane)
{
    int count;
    int dx;
    int dy;
    int unit;

    count = 0;
    for (unit = 0; unit < num_units; unit++)
        for (dx = -1; dx < 2; dx++)
            for (dy = -1; dy < 2; dy++)
                if (p_units[unit].plane == plane && p_units[unit].x + dx == x && p_units[unit].y + dy == y)
                    count++;
    return count;
}

/* (x, y, plane, *fx, *fy) (AINEUTRL.C: lair monsters released, neutral raids) the first land square of the 3x3 around (x, y), row by row, that is not the
   centre and holds no unit, active lair or city of the plane; 1 and its position, else 0 (no x wrap, no y bounds) */
int AI_Adjacent_Tile_Pick(int x, int y, int plane, int *fx, int *fy)
{
    int tx;
    int ty;
    int cells[3][3];
    int i;
    int j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (Tile_Is_Land(x + j - 1, y + i - 1, plane) == 0)
                cells[i][j] = 0;
            else
                cells[i][j] = 1;
        }
    }
    cells[1][1] = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].plane == plane) {
            tx = p_units[i].x;
            ty = p_units[i].y;
            if (abs(tx - x) < 2 && abs(ty - y) < 2)
                cells[ty - y + 1][tx - x + 1] = 0;
        }
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].active == 1 && p_lairs[i].plane == plane) {
            tx = p_lairs[i].x;
            ty = p_lairs[i].y;
            if (abs(tx - x) < 2 && abs(ty - y) < 2)
                cells[ty - y + 1][tx - x + 1] = 0;
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].plane == plane) {
            tx = p_cities[i].x;
            ty = p_cities[i].y;
            if (abs(tx - x) < 2 && abs(ty - y) < 2)
                cells[ty - y + 1][tx - x + 1] = 0;
        }
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (cells[i][j] == 1) {
                *fx = x + j - 1;
                *fy = y + i - 1;
                return 1;
            }
        }
    }
    return 0;
}

/* (budget, race_realm, out[9]) up to 9 random fantastic creatures (types 151..197) of the realm that cost at most the
   remaining budget and are no transports, while 25 or more of the budget is left (2000 tries); returns the count */
int AI_Roll_Monster_Group(int budget, int race_realm, int *out)
{
    int count;
    int tries;
    int unit_type;

    if (budget < 0)
        budget = 0;
    count = 0;
    for (tries = 0; budget >= 25 && tries < 2000; tries++) {
        unit_type = Random(UNIT_NAGAS - UNIT_TROLL_SHAMAN) + UNIT_TROLL_SHAMAN;   /* 151 .. 197, Troll Settlers to the last
                                                   type; the race test keeps only the realm's creatures */
        if (unit_types[unit_type].race == race_realm && unit_types[unit_type].cost <= budget &&
            unit_types[unit_type].transport_capacity == 0 && count < MAX_STACK) {
            budget -= unit_types[unit_type].cost;
            out[count] = unit_type;
            count++;
        }
    }
    return count;
}

/* (stack, x, y, plane) 1 if units may leave (x, y): always outside a city; in a city 1 / 2 / 3 regular units
   (difficulty < 2 / 2 / higher) must be there (it does not count what stays after the move; stack is unused), and after turn 120 a wizard's fortress city keeps all of them (the
   owner is the last unit seen on the square) */
int AI_Stack_Garrison_OK(int stack, int x, int y, int plane)
{
    int in_city;
    int unit_type;
    int owner;
    int i;
    int need;

    in_city = 0;
    for (i = 0; i < num_cities && in_city == 0; i++)
        if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane)
            in_city = 1;
    if (in_city == 0)
        return 1;
    if (difficulty < DIFFICULTY_NORMAL)
        need = 1;
    else if (difficulty == DIFFICULTY_NORMAL)
        need = 2;
    else
        need = 3;
    for (i = 0; i < num_units && need > 0; i++) {
        if (p_units[i].x == x && p_units[i].y == y && p_units[i].plane == plane) {
            owner = p_units[i].owner;
            unit_type = p_units[i].type;
            if ((unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 && unit_types[unit_type].construction == 0 &&
                unit_types[unit_type].transport_capacity == 0)
                need--;
        }
    }
    if (owner != PLAYER_NEUTRAL && p_fortresses[owner].plane == plane && p_fortresses[owner].x == x &&
        p_fortresses[owner].y == y && game_turn > 120)
        return 0;
    if (need > 0)
        return 0;
    return 1;
}

/* (player) the per-landmass plan. A LANDMASS_NO_CITY landmass whose rally point is threatened forces a re-plan; otherwise
   the plan is redone every Random(10) + Random(10) + 25 turns (0 when not due). Units without engineering power
   lose their orders when their destination is unthreatened - bug: read from the threat map of 'plane' 2, past the
   end of p_threat_maps. Per landmass: the rally point is the centroid of the own cities; the status is 3 without an
   own city (4 when a city of a wizard at TREATY_ALLIANCE stands there), 1 when the own unit cost is over 10x
   the foreign, else 2 (bug: status[] holds the own-city count at that test, so a landmass with exactly 6 own cities and no
   foreign units keeps the value 6 = LANDMASS_NO_TARGETS); landmass 0 is 4. Per plane the
   expansion target is the unthreatened square of a status-3 landmass without foreign units nearest to (cx, cy) -
   bug: cx / cy are uninitialised there, and after a forced replan threat is still 1, so no target is searched at
   all. Rally points of status 1 / 2 / 4 on threatened or foreign squares move to
   the nearest unthreatened square of the landmass (none: status 4); status-3 rally points move to a coast square;
   with an attack landmass, status-1 rally points move to the coast square nearest the attack landmass's coast
   Numbered: BUG #152, #153, #323 (BUGS_AND_QUIRKS.txt) */
int AI_Landmass_Plan(int player)
{
    int plane;
    int lm;
    int i;
    int tx;
    int ty;
    int n;
    int tile;
    int cx;
    int cy;
    int unit_type;
    int best_dist;
    int rx;
    int best_x;
    int best_y;
    int dist;
    int best_lm;
    int ry;
    int threat;                                 /* first a flag: a status-3 rally point is threatened (forces the replan) */
    int cell;   /* a threat-map cell; also the attack landmass's coast-square count */
    unsigned own_acc[2][MAX_LANDMASSES];   /* [plane][MAX_LANDMASSES]: the own cities' x sums, then the own units' costs */
    unsigned foreign_acc[2][MAX_LANDMASSES];   /* [plane][MAX_LANDMASSES]: the own cities' y sums, then the foreign units' costs */

    AI_Map_EMS_Tables();
    threat = 0;
    for (plane = 0; plane < NUM_PLANES; plane++)
        for (lm = 0; lm < MAX_LANDMASSES && threat == 0; lm++)
            if (ai_landmass_plan[plane][player].status[lm] == LANDMASS_NO_CITY &&
                p_threat_maps[plane][ai_landmass_plan[plane][player].rally_y[lm] * MAP_WIDTH +
                                     ai_landmass_plan[plane][player].rally_x[lm]] != 0)
                threat = 1;
    if (threat == 1)
        p_ai_replan_timer[player] = 0;
    p_ai_replan_timer[player]--;
    if (p_ai_replan_timer[player] > 0)
        return 0;
    p_ai_replan_timer[player] = Random(10) + Random(10) + 25;
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner == player && unit_types[p_units[i].type].construction == 0 &&
            p_threat_maps[plane][p_units[i].dest_y * MAP_WIDTH + p_units[i].dest_x] == 0)
            p_units[i].order = ORDER_READY;
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (lm = 0; lm < MAX_LANDMASSES; lm++) {
            ai_landmass_plan[plane][player].status[lm] = LANDMASS_UNPLANNED;
            own_acc[plane][lm] = 0;
            foreign_acc[plane][lm] = 0;
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player) {
            plane = p_cities[i].plane;
            lm = p_landmass_ids[plane][p_cities[i].y][p_cities[i].x];
            if (lm < MAX_LANDMASSES) {
                own_acc[plane][lm] += p_cities[i].x;
                foreign_acc[plane][lm] += p_cities[i].y;
                ai_landmass_plan[plane][player].status[lm] = ai_landmass_plan[plane][player].status[lm] + 1;
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (lm = 0; lm < MAX_LANDMASSES; lm++) {
            n = ai_landmass_plan[plane][player].status[lm];
            if (n > 0) {
                ai_landmass_plan[plane][player].rally_x[lm] = own_acc[plane][lm] / n;
                ai_landmass_plan[plane][player].rally_y[lm] = foreign_acc[plane][lm] / n;
            }
        }
    }
    for (i = 0; i < MAX_LANDMASSES; i++) {
        own_acc[0][i] = 0;
        own_acc[1][i] = 0;
        foreign_acc[0][i] = 0;
        foreign_acc[1][i] = 0;
    }
    for (i = 0; i < num_units; i++) {
        unit_type = p_units[i].type;
        if ((unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) == 0) {
            plane = p_units[i].plane;
            lm = p_landmass_ids[plane][p_units[i].y][p_units[i].x];
            if (lm < MAX_LANDMASSES) {
                if (p_units[i].owner == player)
                    own_acc[plane][lm] += unit_types[unit_type].cost;
                else
                    foreign_acc[plane][lm] += unit_types[unit_type].cost;
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (i = 0; i < MAX_LANDMASSES; i++) {
            /* status[] still holds the own-city count here: 6 cities read as LANDMASS_NO_TARGETS (the header's bug) */
            if (ai_landmass_plan[plane][player].status[i] != LANDMASS_NO_TARGETS || foreign_acc[plane][i] != 0) {
                if (ai_landmass_plan[plane][player].status[i] > 0) {
                    if (own_acc[plane][i] > foreign_acc[plane][i] * 10)
                        ai_landmass_plan[plane][player].status[i] = LANDMASS_DOMINANT;
                    else
                        ai_landmass_plan[plane][player].status[i] = LANDMASS_CONTESTED;
                } else
                    ai_landmass_plan[plane][player].status[i] = LANDMASS_NO_CITY;
            }
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner != player && p_cities[i].owner != PLAYER_NEUTRAL &&
            wizards[player].diplomacy_status[p_cities[i].owner] == TREATY_ALLIANCE) {
            plane = p_cities[i].plane;
            lm = p_landmass_ids[plane][p_cities[i].y][p_cities[i].x];
            if (ai_landmass_plan[plane][player].status[lm] == LANDMASS_NO_CITY)
                ai_landmass_plan[plane][player].status[lm] = LANDMASS_IGNORE;
        }
    }
    ai_landmass_plan[0][player].status[0] = LANDMASS_IGNORE;
    ai_landmass_plan[1][player].status[0] = LANDMASS_IGNORE;
    for (plane = 0; plane < NUM_PLANES; plane++) {
        best_dist = 1000;
        for (lm = 1; lm < MAX_LANDMASSES; lm++) {
            if (ai_landmass_plan[plane][player].status[lm] == LANDMASS_NO_CITY && foreign_acc[plane][lm] == 0) {
                for (tile = p_landmass_tile_head[plane][lm]; tile != -1 && threat == 0;
                     tile = p_landmass_tile_next[plane][tile]) {
                    tx = p_landmass_tile_x[plane][tile];
                    ty = p_landmass_tile_y[plane][tile];
                    cell = ty * MAP_WIDTH + tx;
                    if (p_threat_maps[plane][cell] == 0) {
                        dist = Map_Distance(tx, ty, cx, cy, MAP_WIDTH) + Random(20);
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_x = p_landmass_tile_x[plane][tile];
                            best_y = p_landmass_tile_y[plane][tile];
                            best_lm = lm;
                        }
                    }
                }
            }
        }
        if (best_dist < 1000) {
            p_ai_expand_landmass[plane][player] = best_lm;
            p_ai_expand_x[plane][player] = best_x;
            p_ai_expand_y[plane][player] = best_y;
        } else
            p_ai_expand_landmass[plane][player] = 0;
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (lm = 1; lm < MAX_LANDMASSES; lm++) {
            if (ai_landmass_plan[plane][player].status[lm] == LANDMASS_CONTESTED || ai_landmass_plan[plane][player].status[lm] == LANDMASS_DOMINANT ||
                ai_landmass_plan[plane][player].status[lm] == LANDMASS_IGNORE) {
                rx = ai_landmass_plan[plane][player].rally_x[lm];
                ry = ai_landmass_plan[plane][player].rally_y[lm];
                if (p_threat_maps[plane][ry * MAP_WIDTH + rx] != 0 || p_landmass_ids[plane][ry][rx] != lm) {
                    best_dist = 1000;
                    for (tile = p_landmass_tile_head[plane][lm]; tile != -1; tile = p_landmass_tile_next[plane][tile]) {
                        dist = Map_Distance(p_landmass_tile_x[plane][tile], p_landmass_tile_y[plane][tile], rx, ry, MAP_WIDTH);
                        if (dist < best_dist) {
                            tx = p_landmass_tile_x[plane][tile];
                            ty = p_landmass_tile_y[plane][tile];
                            threat = p_threat_maps[plane][ty * MAP_WIDTH + tx];
                            if (threat == 0) {
                                best_dist = dist;
                                best_x = p_landmass_tile_x[plane][tile];
                                best_y = p_landmass_tile_y[plane][tile];
                                best_lm = lm;
                            }
                        }
                    }
                    if (best_dist < 1000) {
                        ai_landmass_plan[plane][player].rally_x[lm] = best_x;
                        ai_landmass_plan[plane][player].rally_y[lm] = best_y;
                    } else {
                        ai_landmass_plan[plane][player].rally_x[lm] = 0;
                        ai_landmass_plan[plane][player].rally_y[lm] = 0;
                        ai_landmass_plan[plane][player].status[lm] = LANDMASS_IGNORE;
                    }
                }
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (lm = 1; lm < MAX_LANDMASSES; lm++) {
            if (ai_landmass_plan[plane][player].status[lm] == LANDMASS_NO_CITY) {
                rx = ai_landmass_plan[plane][player].rally_x[lm];
                ry = ai_landmass_plan[plane][player].rally_y[lm];
                best_dist = 1000;
                for (tile = p_coast_tile_head[plane][lm]; tile != -1; tile = p_coast_tile_next[plane][tile]) {
                    tx = p_coast_tile_x[plane][tile];
                    ty = p_coast_tile_y[plane][tile];
                    if (p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0) {
                        dist = Map_Distance(tx, ty, rx, ry, MAP_WIDTH) + Random(20);
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_x = p_coast_tile_x[plane][tile];
                            best_y = p_coast_tile_y[plane][tile];
                            best_lm = lm;
                        }
                    }
                }
                if (best_dist < 1000) {
                    ai_landmass_plan[plane][player].rally_x[lm] = best_x;
                    ai_landmass_plan[plane][player].rally_y[lm] = best_y;
                } else {
                    ai_landmass_plan[plane][player].rally_x[lm] = 0;
                    ai_landmass_plan[plane][player].rally_y[lm] = 0;
                    ai_landmass_plan[plane][player].status[lm] = LANDMASS_IGNORE;
                }
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        if (p_ai_attack_landmass[plane][player] != 0) {
            for (lm = 1; lm < MAX_LANDMASSES; lm++) {
                if (ai_landmass_plan[plane][player].status[lm] == LANDMASS_DOMINANT) {
                    cx = 0;
                    cy = 0;
                    cell = 0;
                    for (tile = p_coast_tile_head[plane][p_ai_attack_landmass[plane][player]]; tile != -1;
                         tile = p_coast_tile_next[plane][tile]) {
                        cx += p_coast_tile_x[plane][tile];
                        cy += p_coast_tile_y[plane][tile];
                        cell++;
                    }
                    cx = cx / cell;
                    cy = cy / cell;
                    best_dist = 1000;
                    for (tile = p_coast_tile_head[plane][lm]; tile != -1; tile = p_coast_tile_next[plane][tile]) {
                        dist = Map_Distance(p_coast_tile_x[plane][tile], p_coast_tile_y[plane][tile], cx, cy, MAP_WIDTH);
                        if (dist < best_dist) {
                            tx = p_coast_tile_x[plane][tile];
                            ty = p_coast_tile_y[plane][tile];
                            if (p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0) {
                                best_dist = dist;
                                best_x = tx;
                                best_y = ty;
                            }
                        }
                    }
                    if (best_dist < 1000) {
                        ai_landmass_plan[plane][player].rally_x[lm] = best_x;
                        ai_landmass_plan[plane][player].rally_y[lm] = best_y;
                    }
                }
            }
        }
    }
    EMS_Map_Default();
}

/* (player) rebuilds both planes' threat maps: the strength / 10 of every other player's unit and of the guardians
   of active lairs; THREAT_AT_PEACE marks units of wizards the player has a pact / alliance / peace with or is not hostile
   to; THREAT_SITE marks cities (+1 for foreign ones), lairs and nodes
   Numbered: BUG #137, #320 (BUGS_AND_QUIRKS.txt) */
void AI_Build_Threat_Map(int player)
{
    int map_cells;
    int owner;
    int cell;
    int at_peace[MAX_PLAYERS];
    int i;
    int plane;

    AI_Map_EMS_Tables();
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (wizards[player].diplomacy_status[i] == TREATY_WIZARD_PACT || wizards[player].diplomacy_status[i] == TREATY_ALLIANCE ||
            wizards[player].peace_turns[i] > 0 || wizards[player].hostility[i] < HOSTILITY_HOSTILE)
            at_peace[i] = 1;
        else
            at_peace[i] = 0;
    }
    map_cells = MAP_SQUARES;
    for (plane = 0; plane < NUM_PLANES; plane++)
        for (i = 0; i < map_cells; i++)
            p_threat_maps[plane][i] = 0;
    /* BUG #418: no test for a dead unit not yet compacted (owner PLAYER_NONE: the phantom threat and at_peace[-1] of
       #137). AI_Overland_Turn resets a plane of -1 to 0 only once, before its player loop; a unit killed later in that
       pass (Unit_Kill_Or_Dismiss: plane -1 - e.g. by an earlier AI wizard's overland spell) adds its strength through
       p_threat_maps[-1], the far pointer before it (p_path_cache, DS:9c90) - into the AI path cache. clarified using
       ReMoM repo */
    for (i = 0; i < num_units; i++) {
        owner = p_units[i].owner;
        if (owner != player && p_units[i].owner != player) {
            plane = p_units[i].plane;
            cell = p_units[i].y * MAP_WIDTH + p_units[i].x;
            p_threat_maps[plane][cell] += AI_Unit_Strength(i) / 10;
            if (at_peace[owner] == 1)
                p_threat_maps[plane][cell] = p_threat_maps[plane][cell] | THREAT_AT_PEACE;
        }
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].active == 1 && p_lairs[i].guard1_count > 0) {
            plane = p_lairs[i].plane;
            cell = p_lairs[i].y * MAP_WIDTH + p_lairs[i].x;
            p_threat_maps[plane][cell] += UnitType_AI_Strength(p_lairs[i].guard1_type) / 10 *
                                          (p_lairs[i].guard1_count & LAIR_GUARDS_LEFT_MASK);
            p_threat_maps[plane][cell] += UnitType_AI_Strength(p_lairs[i].guard2_type) / 10 *
                                          (p_lairs[i].guard2_count & LAIR_GUARDS_LEFT_MASK);
        }
    }
    for (i = 0; i < num_cities; i++) {
        plane = p_cities[i].plane;
        cell = p_cities[i].y * MAP_WIDTH + p_cities[i].x;
        p_threat_maps[plane][cell] = p_threat_maps[plane][cell] | THREAT_SITE;
        if (p_cities[i].owner != player)
            p_threat_maps[plane][cell]++;
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].active != 0) {
            plane = p_lairs[i].plane;
            cell = p_lairs[i].y * MAP_WIDTH + p_lairs[i].x;
            p_threat_maps[plane][cell] = p_threat_maps[plane][cell] | THREAT_SITE;
        }
    }
    for (i = 0; i < NUM_NODES; i++) {
        plane = p_nodes[i].plane;
        cell = p_nodes[i].y * MAP_WIDTH + p_nodes[i].x;
        p_threat_maps[plane][cell] = p_threat_maps[plane][cell] | THREAT_SITE;
    }
}

/* (player) the war target (once) is the first wizard at TREATY_WAR or worse toward the player, or (bug #154,
   never true: Fire Elemental is combat-only) any wizard casting SPELL_FIRE_ELEMENTAL. Per plane, the attack landmass is kept at
   LANDMASS_CONTESTED / _NO_CITY, dropped at LANDMASS_NO_TARGETS and, at LANDMASS_DOMINANT / _IGNORE /
   _STAGING, dropped when it holds no city of the war target (without one: no neutral city and no city of a wizard the player is hostile
   to). Re-picking weighs each landmass by its target cities (war target 10, neutral 1) and the nearest unthreatened
   coast square to the centroid of the own cities (else the fortress): (2000 - 10 n) / dist with n the weight (a war-target city
   lowers it by 100, a neutral one by 10), at least 10 (fewer cities weigh MORE), +1000 for the war target's fortress landmass at HOSTILITY_DESTROY; without a war target
   n * 1000 / dist. Bug: a threatened coast square reuses the previous distance. All zero: landmasses not at
   LANDMASS_DOMINANT / _NO_TARGETS and not the fortress's weigh their square count (0 below 10). Random_Weighted_Pick, else 0
   Numbered: BUG #376 (BUGS_AND_QUIRKS.txt) */
void AI_Choose_Attack_Landmass(int player)
{
    int sum_x;
    int sum_y;
    int plane;
    int best_dist;
    int lm;
    int tile;                                   /* a list cursor, later the weight total */
    int tx;
    int ty;
    int dist;                                   /* first the own city count, then a distance */
    int attack_lm;
    int repick;
    int war_target;
    int owner;
    int count[MAX_LANDMASSES];                              /* per owner: the cities, later per landmass: the target weight */
    int weights[MAX_LANDMASSES];
    int i;

    war_target = -1;
    AI_Map_EMS_Tables();
    for (i = 0; i < num_players && war_target == -1; i++)
        if (wizards[player].diplomacy_status[i] >= TREATY_WAR || wizards[i].casting_spell == SPELL_FIRE_ELEMENTAL)
            war_target = i;
    for (plane = 0; plane < NUM_PLANES; plane++) {
        repick = 0;
        attack_lm = p_ai_attack_landmass[plane][player];
        if (attack_lm == 0)
            repick = 1;
        else {
            switch (ai_landmass_plan[plane][player].status[attack_lm]) {
            case LANDMASS_CONTESTED:
            case LANDMASS_NO_CITY:
                repick = 0;
                break;
            case LANDMASS_NO_TARGETS:
                repick = 1;
                break;
            case LANDMASS_DOMINANT:
            case LANDMASS_IGNORE:
            case LANDMASS_STAGING:
                for (i = 0; i < MAX_PLAYERS; i++)
                    count[i] = 0;
                for (i = 0; i < num_cities; i++) {
                    if (p_landmass_ids[plane][p_cities[i].y][p_cities[i].x] == attack_lm &&
                        p_cities[i].owner != player) {
                        owner = p_cities[i].owner;
                        count[owner]++;
                    }
                }
                if (war_target > -1) {
                    if (count[war_target] == 0)
                        repick = 1;
                    else
                        repick = 0;
                } else {
                    repick = 1;
                    if (count[PLAYER_NEUTRAL] > 0)
                        repick = 0;
                    else
                        for (i = 0; i < num_players && repick == 1; i++)
                            if (wizards[player].hostility[i] >= HOSTILITY_HOSTILE && count[i] > 0)
                                repick = 0;
                }
                break;
            }
        }
        if (repick == 1) {
            for (i = 0; i < MAX_LANDMASSES; i++) {
                weights[i] = 0;
                count[i] = 0;
            }
            sum_x = 0;
            sum_y = 0;
            dist = 0;                           /* dist: here the own cities counted */
            for (i = 0; i < num_cities; i++) {
                if (p_cities[i].plane == plane) {
                    owner = p_cities[i].owner;
                    if (owner == player) {
                        sum_x += p_cities[i].x;
                        sum_y += p_cities[i].y;
                        dist++;
                    } else if (war_target > -1) {
                        if (owner == war_target)
                            count[p_landmass_ids[plane][p_cities[i].y][p_cities[i].x]] += 10;
                        else if (owner == PLAYER_NEUTRAL)
                            count[p_landmass_ids[plane][p_cities[i].y][p_cities[i].x]]++;
                    } else if (owner == PLAYER_NEUTRAL)
                        count[p_landmass_ids[plane][p_cities[i].y][p_cities[i].x]]++;
                }
            }
            if (dist == 0) {
                sum_x = p_fortresses[player].x;
                sum_y = p_fortresses[player].y;
            } else {
                sum_x = sum_x / dist;
                sum_y = sum_y / dist;
            }
            for (lm = 1; lm < MAX_LANDMASSES; lm++) {
                tile = p_coast_tile_head[plane][lm];
                best_dist = 1000;
                for (; tile != -1; tile = p_coast_tile_next[plane][tile]) {
                    tx = p_coast_tile_x[plane][tile];
                    ty = p_coast_tile_y[plane][tile];
                    if (p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0)
                        dist = Map_Distance(tx, ty, sum_x, sum_y, MAP_WIDTH);
                    if (dist < best_dist)
                        best_dist = dist;
                }
                if (best_dist == 0)
                    best_dist = 1;
                if (war_target > -1) {
                    if (count[lm] > 0) {
                        weights[lm] = (2000 - count[lm] * 10) / best_dist;
                        if (weights[lm] < 10)
                            weights[lm] = 10;
                    }
                    if (wizards[player].hostility[war_target] == HOSTILITY_DESTROY && p_fortresses[war_target].plane == plane &&
                        p_landmass_ids[plane][p_fortresses[war_target].y][p_fortresses[war_target].x] == lm)
                        weights[lm] += 1000;
                } else {
                    weights[lm] = count[lm] * 1000 / best_dist;
                    if (count[lm] > 0 && weights[lm] < 10)
                        weights[lm] = 10;
                }
            }
            tile = 0;                           /* tile: here the weight total */
            for (i = 0; i < MAX_LANDMASSES; i++)
                tile += weights[i];
            if (tile == 0) {
                for (i = 0; i < MAX_LANDMASSES; i++) {
                    if (ai_landmass_plan[plane][player].status[i] != LANDMASS_DOMINANT &&
                        ai_landmass_plan[plane][player].status[i] != LANDMASS_NO_TARGETS) {
                        if (p_fortresses[player].active == 0 ||
                            p_landmass_ids[plane][p_fortresses[player].y][p_fortresses[player].x] != i) {
                            for (tile = p_landmass_tile_head[plane][i]; tile != -1;
                                 tile = p_landmass_tile_next[plane][tile])
                                weights[i]++;
                            if (weights[i] < 10)
                                weights[i] = 0;
                        }
                    }
                }
            }
            tile = 0;                           /* tile: here the weight total */
            for (i = 0; i < MAX_LANDMASSES; i++)
                tile += weights[i];
            if (tile == 0)
                p_ai_attack_landmass[plane][player] = 0;
            else
                p_ai_attack_landmass[plane][player] = Random_Weighted_Pick(weights, MAX_LANDMASSES);
        }
    }
}

/* (player) the player's units except engineers lose their orders (ORDER_READY); 0 for the human player (no return
   value otherwise) */
int AI_Cancel_Unit_Orders(int player)
{
    int unit;

    if (player == PLAYER_HUMAN)
        return 0;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].owner == player && unit_types[p_units[unit].type].construction == 0)
            p_units[unit].order = ORDER_READY;
}
