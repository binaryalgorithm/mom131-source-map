/* MOVECHK.C - WIZARDS.EXE overlay code segment 7dfd (stub 24ec): checking a planned overland move - walking the
   path square by square (stop where the moves run out, at a lair / city / enemy stack to attack, at a city ward,
   before a stack would exceed 9 units, on a square with a transport), the stack's moves, the city ward test and its
   message, and printing half moves ("2.5").
   This module's data (DS:5698): its literals ("" is the tail of "A ": -d); BSS (DS:c794): ward_blocked_flags.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7dfd src/wizards/MOVECHK.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order). City_Ward_Check
   has no parameter list: Path_Ward_Blocked calls it with 5 arguments, its definition takes 3 */
void Path_Walk_Check(int player, char *path_x, char *path_y, int plane, char *path_cost, int moves, int *target,
                     int *attack, int *length, int *incomplete, int *units, int n);
int Foreign_City_At_Unused(int x, int y, int plane, int player);
int City_At_Copy(int x, int y, int plane);
int Stack_Moves_Available(int *units, int n);
int Path_Ward_Blocked(int city, int *n, int *units);
int City_Ward_Check();
void Ward_Blocked_Message(void);
void Print_Half_Moves(int x, int y, int half_moves, int align);

#include "GAME.H"

#define WARD_SLOT(ench) ((ench) - CITY_ENCH_NATURE_WARD)   /* ward_blocked_flags: a ward's slot */

char ward_blocked_flags[5];                     /* DS:c794  the wards that stopped the last move (Nature, Sorcery, Chaos, Life, Death) */

extern char *city_enchant_names[];              /* DS:1ee4 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286 */
extern int stack_moves_cached;                  /* DS:c08e */
extern int stack_move_ward_blocked;             /* DS:c5dc */

int Lair_At(int x, int y, int plane);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
void Tile_Units_Foreign(int x, int y, int plane, int player, int *count, int *list);
int Hero_Item_Water_Walking(int unit);
int Unit_Can_Swim(int unit);
void Text_List_Separator(int *count, int total, char *text);

/* (player, path x / y / cost, plane, moves, &target, &attack, &length, &incomplete, units, n): walks a planned path:
   the walk ends on the square where the summed cost reaches the moves (incomplete), before a warded city, a full
   square (more than 9 units) or an own city that would overflow, on a square with a transport (when the boats can
   carry everybody - otherwise before it), and on a lair (combat_site 5), foreign city (1) or enemy stack (0) to
   attack. Any complete stop (not 'incomplete') clears the current player's Go-To orders; a stack on Go-To
   then never attacks: the attack is dropped and the walk ends one square short. The initial assignments are made twice; the at_* flags are never read; the transport check
   ends with an empty loop */
void Path_Walk_Check(int player, char *path_x, char *path_y, int plane, char *path_cost, int moves, int *target,
                     int *attack, int *length, int *incomplete, int *units, int n)
{
    int tgt;
    int atk;
    int len;
    int stop;
    int unit;
    int count;
    int city;
    int at_units;
    int at_city;
    int at_lair;
    int lair;
    int cost_sum;
    int inc;
    int dropped;
    int length_in;
    int ward;
    int capacity;
    int there_need;
    int stack_need;
    int tile_units[MAX_STACK];
    int i;
    int k;

    tgt = -1;
    atk = *attack;
    len = *length;
    inc = *incomplete;
    length_in = *length;
    tgt = -1;
    atk = *attack;
    inc = *incomplete;
    len = length_in;
    cost_sum = 0;
    stop = 0;
    for (i = 0; i < len && stop == 0; i++) {
        cost_sum += path_cost[i];
        if (cost_sum >= moves) {
            len = i + 1;
            inc = 1;
            atk = 0;
            stop = 1;
        }
        lair = Lair_At(path_x[i], path_y[i], plane);
        if (lair != -1) {
            stop = 1;
            at_lair = 1;
            inc = 0;
            len = i;
            len = i + 1;
            atk = 1;
            combat_site = COMBAT_AT_LAIR;
            combat_site_index = lair;
            tgt = PLAYER_NEUTRAL;   /* a lair's defenders */
            inc = 0;
        } else {
            city = City_At_Copy(path_x[i], path_y[i], plane);
            if (city != -1) {
                ward = Path_Ward_Blocked(city, &n, units);
                if (ward == 1) {
                    stop = 1;
                    len = i;
                    at_city = 1;
                    inc = 0;
                } else if (p_cities[city].owner != player) {
                    stop = 1;
                    len = i;
                    at_city = 1;
                    inc = 0;
                    len = i + 1;
                    atk = 1;
                    tgt = p_cities[city].owner;
                    combat_site_index = city;
                    combat_site = COMBAT_AT_CITY;
                    inc = 0;
                } else {
                    Tile_Units_Of_Owner(path_x[i], path_y[i], plane, player, &count, tile_units);
                    if (n + count > MAX_STACK) {
                        stop = 1;
                        len = i;
                        inc = 0;
                    }
                }
            } else {
                Tile_Units_Foreign(path_x[i], path_y[i], plane, player, &count, tile_units);
                if (count > 0) {
                    stop = 1;
                    at_units = 1;
                    inc = 0;
                    len = i;
                    len = i + 1;
                    inc = 0;
                    atk = 1;
                    tgt = tile_units[0];
                    combat_site = COMBAT_AT_UNITS;
                } else {
                    Tile_Units_Of_Owner(path_x[i], path_y[i], plane, player, &count, tile_units);
                    if (n + count > MAX_STACK) {
                        stop = 1;
                        len = i;
                        inc = 0;
                    } else if (count > 0) {
                        capacity = 0;
                        for (k = 0; k < count; k++) {
                            if (unit_types[p_units[tile_units[k]].type].transport_capacity > 0) {
                                capacity += unit_types[p_units[tile_units[k]].type].transport_capacity;
                                stop = 1;
                                len = i + 1;
                                inc = 0;
                            }
                        }
                        for (k = 0; k < n; k++) {
                            if (unit_types[p_units[units[k]].type].transport_capacity > 0) {
                                capacity += unit_types[p_units[units[k]].type].transport_capacity;
                                stop = 1;
                                len = i + 1;
                                inc = 0;
                            }
                        }
                        if (stop == 1 && capacity > 0) {
                            there_need = count;
                            for (k = 0; k < count; k++)
                                if (p_units[tile_units[k]].type <= UNIT_HERO_CHOSEN || Hero_Item_Flight(tile_units[k]) == 1
                                    || Hero_Item_Water_Walking(tile_units[k]) == 1
                                    || Unit_Can_Swim(tile_units[k]) == 1
                                    || Unit_Has_Wind_Walking(tile_units[k]) == 1
                                    || Unit_Can_Fly(tile_units[k]) == 1)
                                    there_need--;
                            stack_need = n;
                            for (k = 0; k < n; k++)
                                if (p_units[units[k]].type <= UNIT_HERO_CHOSEN || Hero_Item_Flight(units[k]) == 1
                                    || Hero_Item_Water_Walking(units[k]) == 1 || Unit_Can_Swim(units[k]) == 1
                                    || Unit_Has_Wind_Walking(units[k]) == 1 || Unit_Can_Fly(units[k]) == 1)
                                    stack_need--;
                            if (there_need + stack_need - 1 <= capacity) {
                                for (k = 0; k < n; k++)
                                    if (unit_types[p_units[units[k]].type].transport_capacity < 1)
                                        ;
                            } else
                                len = i;
                        }
                    }
                }
            }
        }
    }
    if (inc == 0 && stop == 1) {
        dropped = 0;
        for (k = 0; k < n; k++) {
            unit = units[k];
            if (p_units[unit].order == ORDER_GOTO && player == current_player) {
                if (atk == 1) {
                    atk = 0;
                    dropped = 1;
                }
                p_units[unit].order = ORDER_READY;
                p_units[unit].dest_x = 0;
                p_units[unit].dest_y = 0;
            }
        }
        if (dropped == 1)
            len--;
    }
    *target = tgt;
    *attack = atk;
    *length = len;
    *incomplete = inc;
}

/* DEAD (no callers): (x, y, plane, player) -> a city of another player there, or -1 */
int Foreign_City_At_Unused(int x, int y, int plane, int player)
{
    int city;
    int result;

    result = -1;
    for (city = 0; city < num_cities && result == -1; city++)
        if (p_cities[city].owner != player && p_cities[city].plane == plane && p_cities[city].x == x
            && p_cities[city].y == y)
            result = city;
    return result;
}

/* (x, y, plane) -> the city there or -1 (the same as City_At) */
int City_At_Copy(int x, int y, int plane)
{
    int city;
    int result;

    result = -1;
    for (city = 0; city < num_cities && result == -1; city++)
        if (p_cities[city].plane == plane && p_cities[city].x == x && p_cities[city].y == y)
            result = city;
    return result;
}

/* (units, n) -> the stack's moves: the smallest moves left, or the moves of the first transport / Wind Walker found
   (0 for an empty stack); also into stack_moves_cached
   Numbered: BUG #208 (BUGS_AND_QUIRKS.txt) */
int Stack_Moves_Available(int *units, int n)
{
    int k;
    int unit;
    int moves;

    moves = MIN_NOT_FOUND;
    for (k = 0; k < n; k++) {
        unit = units[k];
        if (p_units[unit].moves_left < moves)
            moves = p_units[unit].moves_left;
        if (unit_types[p_units[unit].type].transport_capacity > 0) {
            moves = p_units[unit].moves_left;
            break;
        }
        if (Unit_Has_Wind_Walking(unit) == 1) {
            moves = p_units[unit].moves_left;
            break;
        }
    }
    if (moves == MIN_NOT_FOUND)
        moves = 0;
    stack_moves_cached = moves;
    return stack_moves_cached;
}

/* (city, &n, units) -> 1 if a ward of the city stops the stack (stack_move_ward_blocked set for the move's message).
   City_Ward_Check gets two more arguments it does not take */
int Path_Ward_Blocked(int city, int *n, int *units)
{
    int owner;
    int n_copy;
    int unused;
    int result;
    int ward;
    int list[MAX_STACK];

    result = 0;
    owner = p_units[*units].owner;
    n_copy = *n;
    ward = City_Ward_Check(city, &n_copy, units, &unused, list);
    if (ward == 1) {
        stack_move_ward_blocked = 1;
        result = 1;
    }
    return result;
}

/* (city, &n, units) -> 1 if one of the city's wards stops a unit: Nature / Sorcery / Life ward the creatures of
   race 0x10 / 0x11 / 0x13; Chaos ward race 0x12 and Chaos-Channels mutations (0x1c), Death ward race 0x14 and
   Undead (0x20); sets ward_blocked_flags. BUG #173: the Chaos / Death creatures are overwritten with -1 in the
   caller's unit list */
int City_Ward_Check(int city, int *n, int *units)
{
    int count;
    int blocked;
    int mutations;
    int i;

    for (i = 0; i < 5; i++)
        ward_blocked_flags[i] = 0;
    count = *n;
    blocked = 0;
    if (p_cities[city].enchants[CITY_ENCH_NATURE_WARD] > 0)
        for (i = 0; i < count; i++)
            if (units[i] != -1 && unit_types[p_units[units[i]].type].race == RACE_NATURE_CREATURE) {
                ward_blocked_flags[WARD_SLOT(CITY_ENCH_NATURE_WARD)] = 1;
                blocked = 1;
            }
    if (p_cities[city].enchants[CITY_ENCH_SORCERY_WARD] > 0)
        for (i = 0; i < count; i++)
            if (units[i] != -1 && unit_types[p_units[units[i]].type].race == RACE_SORCERY_CREATURE) {
                ward_blocked_flags[WARD_SLOT(CITY_ENCH_SORCERY_WARD)] = 1;
                blocked = 1;
            }
    if (p_cities[city].enchants[CITY_ENCH_CHAOS_WARD] > 0)
        for (i = 0; i < count; i++)
            if (units[i] != -1) {
                if (unit_types[p_units[units[i]].type].race == RACE_CHAOS_CREATURE) {
                    units[i] = -1;
                    ward_blocked_flags[WARD_SLOT(CITY_ENCH_CHAOS_WARD)] = 1;
                    blocked = 1;
                } else {
                    mutations = p_units[units[i]].mutation_flags;
                    if (mutations & (MUT_CHAOS_DEMON_SKIN | MUT_CHAOS_DEMON_WINGS | MUT_CHAOS_FIRE_BREATH)) {
                        ward_blocked_flags[WARD_SLOT(CITY_ENCH_CHAOS_WARD)] = 1;
                        blocked = 1;
                    }
                }
            }
    if (p_cities[city].enchants[CITY_ENCH_LIFE_WARD] > 0)
        for (i = 0; i < count; i++)
            if (units[i] != -1 && unit_types[p_units[units[i]].type].race == RACE_LIFE_CREATURE) {
                ward_blocked_flags[WARD_SLOT(CITY_ENCH_LIFE_WARD)] = 1;
                blocked = 1;
            }
    if (p_cities[city].enchants[CITY_ENCH_DEATH_WARD] > 0)
        for (i = 0; i < count; i++)
            if (units[i] != -1) {
                if (unit_types[p_units[units[i]].type].race == RACE_DEATH_CREATURE) {
                    units[i] = -1;
                    ward_blocked_flags[WARD_SLOT(CITY_ENCH_DEATH_WARD)] = 1;
                    blocked = 1;
                } else {
                    mutations = p_units[units[i]].mutation_flags;
                    if (mutations & MUT_UNDEAD) {
                        ward_blocked_flags[WARD_SLOT(CITY_ENCH_DEATH_WARD)] = 1;
                        blocked = 1;
                    }
                }
            }
    return blocked;
}

/* "A Nature Ward, Chaos Ward and Death Ward prevents your troops from entering the city." */
void Ward_Blocked_Message(void)
{
    int state;
    int total;
    int i;

    total = 0;
    for (i = 0; i < 5; i++)
        if (ward_blocked_flags[i] != 0)
            total++;
    strcpy(message_text, "A ");
    state = 0;
    for (i = 0; i < 5; i++) {
        if (ward_blocked_flags[i] != 0) {
            Text_List_Separator(&state, total, message_text);
            strcat(message_text, city_enchant_names[CITY_ENCH_NATURE_WARD + i]);
        }
    }
    strcat(message_text, " prevents your troops from entering the city.");
    Message_Box(message_text);
}

/* (x, y, half_moves, align): moves kept as halves - the whole part (none when 0: 1 prints ".5"), ".5" when odd;
   align 0 left, 1 right, others print nothing */
void Print_Half_Moves(int x, int y, int half_moves, int align)
{
    char text[8];
    char num_text[8];
    int whole;

    if (half_moves >= 0)
        whole = half_moves / 2;
    else
        whole = 0;
    strcpy(text, "");
    if (whole > 0) {
        itoa(whole, num_text, 10);
        strcat(text, num_text);
    }
    if (half_moves % 2 != 0)
        strcat(text, ".5");
    switch (align) {
    case 0:
        Print_Text(x, y, text);
        break;
    case 1:
        Print_Text_Right(x, y, text);
        break;
    }
}
