/* ATTACK.C - WIZARDS.EXE overlay code segment 7d45 (stub 24e8): an overland attack - against a stack, a city or a
   lair (combat_site 0 / 1 / 5): the neutral player never attacks neutral targets, a treaty partner is attacked
   only after the human confirms (or never, by the AI), an empty city is simply taken; then tactical combat
   (Combat_Main, when the human is involved and Strategic Combat is off) or the auto-resolve (Combat_Strategic), the
   winner's hero items, the city's capture / razing and the diplomatic consequences; also the lair guardians, the
   attack on a lair and two unused helpers.
   This module's data (DS:5660): its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7d45 src/wizards/ATTACK.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Overland_Attack(int attacker, int target, int n, int *units);
void Lair_Spawn_Guardians(int lair);
int Lair_Attack(int lair, int player);
void Diplo_After_Attack(int attacker, int defender);
void Attack_At_Unit_Unused(int target, int at_unit, int plane);
void Map_Flash_Combat_Site_Unused(void);

#include "GAME.H"

extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern int stack_unit_count;                    /* DS:999c */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286  the city / lair attacked */
extern int map_view_plane;                      /* DS:bd86 */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int award_loc_plane;                     /* DS:c300 */
extern int award_loc_y;                         /* DS:c302 */
extern int award_loc_x;                         /* DS:c304 */
extern int award_at_location;                   /* DS:c306 */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int move_retreat_y;                         /* DS:c5de */
extern int move_retreat_x;                         /* DS:c5e0 */
extern int city_razed;                          /* DS:c974 */

int Confirm_Box(char *text);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
int Diplo_Break_Treaty(int breaker, int victim);
int Diplo_Declare_War(int declarer, int target);
void City_Captured(int n, int *units);
void City_Change_Owner(int city, int new_owner);
int Wizard_Fortress_City(int player);
int Sound_Stop(void);
int Combat_Main(int attacker_player, int defender_player, int *units, int n, int x, int y, int plane, int *n_items,
                int *items);
int Combat_Strategic(int *units, int n, int x, int y, int plane, int *item_count, int *items);
void Music_Play_Current(void);
void Hero_Items_Redistribute(int player, int count, int *items);
void Combat_End_Nop(void);
void Minimap_Alloc_Pic(void);
void Screen_Copy_Front(void);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane);
void Minimap_Build_View(void);
void Map_Full_Redraw(void);
void Overland_Screen_Draw(int *vx, int *vy, int plane, int tx, int ty, int player);

/* (attacker, target, n, units) -> 1 if the attacker won; target = a unit index (MOVESTK.C) or the defending
   player (Lair_Attack: PLAYER_NEUTRAL; ATTACK.C's other caller: the unit's owner). The stack is first put back on the square it came
   from (move_retreat_x / _y). BUG #172: the treaty break is called with the roles swapped
   Numbered: BUG #172, #227, #281 (BUGS_AND_QUIRKS.txt) */
int Overland_Attack(int attacker, int target, int n, int *units)
{
    int won;
    int att;
    int defenders;
    int item_count;
    int destroyed;
    int items[18];
    int i;
    int def;

    destroyed = 0;
    city_razed = 0;
    stack_unit_count = 0;
    att = attacker;
    def = target;                               /* def: the target unit (COMBAT_AT_UNITS), then its owner below */
    for (i = 0; i < n; i++) {
        p_units[units[i]].x = move_retreat_x;
        p_units[units[i]].y = move_retreat_y;
    }
    if (att == PLAYER_NEUTRAL) {
        switch (combat_site) {
        case COMBAT_AT_UNITS:
            if (p_units[def].owner == PLAYER_NEUTRAL) {
                for (i = 0; i < n; i++)
                    p_units[units[i]].order = ORDER_READY;
                combat_site_index = -1;
                combat_site = COMBAT_AT_NONE;
                return 0;
            }
            break;
        case COMBAT_AT_CITY:
            if (p_cities[combat_site_index].owner == PLAYER_NEUTRAL) {
                for (i = 0; i < n; i++)
                    p_units[units[i]].order = ORDER_READY;
                combat_site_index = -1;
                combat_site = COMBAT_AT_NONE;
                return 0;
            }
            break;
        case COMBAT_AT_LAIR:
            for (i = 0; i < n; i++)
                p_units[units[i]].order = ORDER_READY;
            combat_site_index = -1;
            combat_site = COMBAT_AT_NONE;
            return 0;
            break;
        }
    }
    switch (combat_site) {
    case COMBAT_AT_UNITS:
        combat_loc_x = p_units[def].x;
        combat_loc_y = p_units[def].y;
        combat_loc_plane = p_units[def].plane;
        def = p_units[def].owner;               /* from here the defending player */
        break;
    case COMBAT_AT_CITY:
        wizards[PLAYER_NEUTRAL].banner_color = BANNER_NEUTRAL;
        combat_loc_x = p_cities[combat_site_index].x;
        combat_loc_y = p_cities[combat_site_index].y;
        combat_loc_plane = p_cities[combat_site_index].plane;
        defenders = 0;
        for (i = 0; i < num_units; i++)
            if (p_units[i].x == combat_loc_x && p_units[i].y == combat_loc_y && p_units[i].plane == combat_loc_plane
                && p_units[i].owner == def)
                defenders++;
        break;
    case COMBAT_AT_LAIR:
        wizards[PLAYER_NEUTRAL].banner_color = BANNER_NEUTRAL;
        Lair_Spawn_Guardians(combat_site_index);
        combat_loc_x = p_lairs[combat_site_index].x;
        combat_loc_y = p_lairs[combat_site_index].y;
        combat_loc_plane = p_lairs[combat_site_index].plane;
        break;
    }
    if (att < PLAYER_NEUTRAL && def < PLAYER_NEUTRAL && (combat_site == COMBAT_AT_CITY || combat_site == COMBAT_AT_UNITS)
        && (wizards[def].diplomacy_status[att] == TREATY_ALLIANCE || wizards[def].diplomacy_status[att] == TREATY_WIZARD_PACT)) {
        if (att == current_player) {
            strcpy(message_text, "You have a treaty with ");
            strcat(message_text, wizards[def].wizard_name);
            strcat(message_text, ".  Do you still wish to attack?");
            if (Confirm_Box(message_text) == 0) {
                combat_site_index = -1;
                combat_site = COMBAT_AT_NONE;
                return 0;
            } else if (att < MAX_PLAYERS && def < MAX_PLAYERS) {   /* always true: owners are 0 .. 5 (here and below) */
                Diplo_Relation_Change(-40, att, def, DIPLO_MSG_REMEMBER_THIS_DAY, 0, 0);
                Diplo_Break_Treaty(def, att);
            }
        } else {
            for (i = 0; i < n; i++)
                p_units[units[i]].order = ORDER_READY;
            combat_site_index = -1;
            combat_site = COMBAT_AT_NONE;
            return 0;
        }
    }
    if (combat_site == COMBAT_AT_CITY && defenders < 1) {
        won = 1;
        item_count = 0;
        City_Captured(n, units);
    } else if ((att == current_player || def == current_player) && game_config.strategic_combat == 0) {
        Sound_Stop();
        won = Combat_Main(att, def, units, n, combat_loc_x, combat_loc_y, combat_loc_plane, &item_count, items);
        Music_Play_Current();
    } else {
        won = Combat_Strategic(units, n, combat_loc_x, combat_loc_y, combat_loc_plane, &item_count, items);
        item_count = 0;                         /* auto-resolved: the dead heroes' items are not redistributed */
    }
    if (won == 1) {
        for (i = 0; i < n; i++) {
            if (p_units[units[i]].owner == OWNER_RECALLED) {
                p_units[units[i]].owner = att;
                p_units[units[i]].finished = 1;
            } else {
                p_units[units[i]].x = combat_loc_x;
                p_units[units[i]].y = combat_loc_y;
                p_units[units[i]].plane = combat_loc_plane;
            }
        }
        award_at_location = 1;
        award_loc_x = combat_loc_x;
        award_loc_y = combat_loc_y;
        award_loc_plane = combat_loc_plane;
        Hero_Items_Redistribute(att, item_count, items);
        award_at_location = 0;
        if (combat_site == COMBAT_AT_CITY) {
            if (p_cities[combat_site_index].owner != att) {
                if (p_cities[combat_site_index].size != 0) {
                    if (att < MAX_PLAYERS && def < MAX_PLAYERS)
                        Diplo_Relation_Change(-40, att, def, DIPLO_MSG_UNPROVOKED_ATTACK, combat_site_index, 0);
                    if (att == PLAYER_NEUTRAL && (unit_types[p_units[units[0]].type].ability_flags & AFLAG_FANTASTIC) != 0) {
                        /* neutral raiders (ability bit 1) leave the city as it is */
                    } else {
                        if (att < MAX_PLAYERS && def < MAX_PLAYERS)
                            Diplo_Declare_War(att, def);
                        if (city_razed == 0) {
                            City_Change_Owner(combat_site_index, att);
                            if (att != current_player)
                                p_cities[combat_site_index].producing = PROD_AUTO_CHOOSE;
                        }
                    }
                } else if (att == PLAYER_NEUTRAL && (unit_types[p_units[units[0]].type].ability_flags & AFLAG_FANTASTIC) != 0) {
                    /* raiders do not raze a size-0 city */
                } else
                    destroyed = 1;
            }
        }
    } else {
        Hero_Items_Redistribute(def, item_count, items);   /* BUG #428: award_at_location is not set to 1 first (the
                                                   attacker's branch does): a winning defender's loot counts as from
                                                   the vault - free only for heroes at the fortress, not at the battle
                                                   site; clarified using ReMoM repo */
        award_at_location = 0;
        if (combat_site == COMBAT_AT_CITY) {
            if (p_cities[combat_site_index].owner != def) {
                if (p_cities[combat_site_index].size != 0) {
                    if (city_razed == 0)
                        City_Change_Owner(combat_site_index, def);
                } else
                    destroyed = 1;
            } else if (att < MAX_PLAYERS && def < MAX_PLAYERS && p_cities[combat_site_index].size != 0) {
                if (Wizard_Fortress_City(def) == combat_site_index)
                    Diplo_Relation_Change(-60, att, def, DIPLO_MSG_UNPROVOKED_ATTACK, combat_site_index, 0);
                else
                    Diplo_Relation_Change(-20, att, def, DIPLO_MSG_UNPROVOKED_ATTACK, combat_site_index, 0);
            }
        }
    }
    Diplo_After_Attack(att, def);
    if (destroyed == 1) {
        City_Change_Owner(combat_site_index, att);
        City_Remove(combat_site_index);
        city_razed = 0;
    }
    if (city_razed == 1) {
        City_Change_Owner(combat_site_index, att);
        City_Remove(combat_site_index);
        city_razed = 0;
    }
    combat_site_index = -1;
    combat_site = COMBAT_AT_NONE;
    Combat_End_Nop();
    Minimap_Alloc_Pic();
    if (att == current_player || def == current_player) {
        Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
        Map_Full_Redraw();
        Screen_Copy_Front();
    }
    return won;
}

/* (lair): creates the lair's guardians as neutral units on its square - first group, then the second, at most 9;
   the normal (not summoned) types, below UNIT_MAGIC_SPIRIT, get the Undead mutation (MUT_UNDEAD) */
void Lair_Spawn_Guardians(int lair)
{
    int placed;
    int count;
    int i;

    count = p_lairs[lair].guard1_count & LAIR_GUARDS_LEFT_MASK;
    placed = 0;
    for (i = 0; i < count && placed < MAX_STACK; i++) {
        if (Unit_Create(p_lairs[lair].guard1_type, PLAYER_NEUTRAL, p_lairs[lair].x, p_lairs[lair].y, p_lairs[lair].plane,
                        UNIT_SRC_NO_CAP) == 1) {
            if (p_lairs[lair].guard1_type < UNIT_MAGIC_SPIRIT)
                p_units[num_units - 1].mutation_flags = p_units[num_units - 1].mutation_flags | MUT_UNDEAD;
            placed++;
        }
    }
    count = p_lairs[lair].guard2_count & LAIR_GUARDS_LEFT_MASK;
    for (i = 0; i < count && placed < MAX_STACK; i++) {
        if (Unit_Create(p_lairs[lair].guard2_type, PLAYER_NEUTRAL, p_lairs[lair].x, p_lairs[lair].y, p_lairs[lair].plane,
                        UNIT_SRC_NO_CAP) == 1) {
            if (p_lairs[lair].guard2_type < UNIT_MAGIC_SPIRIT)
                p_units[num_units - 1].mutation_flags = p_units[num_units - 1].mutation_flags | MUT_UNDEAD;
            placed++;
        }
    }
}

/* (lair, player): the player's units on the lair's square attack it */
int Lair_Attack(int lair, int player)
{
    int result;
    int count;
    int units[MAX_STACK];

    Tile_Units_Of_Owner(p_lairs[lair].x, p_lairs[lair].y, p_lairs[lair].plane, player, &count, units);
    result = Overland_Attack(player, PLAYER_NEUTRAL, count, units);
    return result;
}

/* DEAD (no callers): the current player's units on at_unit's square attack - BUG #429: it passes the target unit's OWNER,
   while combat_site COMBAT_AT_UNITS makes Overland_Attack read it as a unit index */
void Attack_At_Unit_Unused(int target, int at_unit, int plane)
{
    int count;
    int x;
    int y;
    int plane_copy;
    int owner;
    int units[MAX_STACK];

    x = p_units[at_unit].x;
    y = p_units[at_unit].y;
    plane_copy = plane;
    owner = p_units[target].owner;
    move_retreat_x = x;
    move_retreat_y = y;
    combat_site = COMBAT_AT_UNITS;
    Tile_Units_Of_Owner(x, y, plane_copy, current_player, &count, units);
    Overland_Attack(current_player, owner, count, units);
}

/* (attacker, defender) after an overland attack: the defender's hostility toward the attacker = 2 (3 at war, 0
   when allied), and the defender re-plans its stance in 16..25 turns */
void Diplo_After_Attack(int attacker, int defender)
{
    wizards[defender].hostility[attacker] = HOSTILITY_HOSTILE;
    if (wizards[defender].diplomacy_status[attacker] >= TREATY_WAR)
        wizards[defender].hostility[attacker] = HOSTILITY_WAR;
    if (wizards[defender].diplomacy_status[attacker] == TREATY_ALLIANCE)
        wizards[defender].hostility[attacker] = HOSTILITY_NONE;
    wizards[defender].hostility_replan_timer = Random(10) + 15;
}

/* DEAD (no callers): centres the map on the combat site, draws it for 6 ticks, restores the view */
void Map_Flash_Combat_Site_Unused(void)
{
    int saved_x;
    int saved_y;
    int x;
    int y;

    saved_x = map_view_x;
    saved_y = map_view_y;
    Map_View_Ensure_Visible(&x, &y, combat_loc_x, combat_loc_y, combat_loc_plane);
    map_view_x = x;
    map_view_y = y;
    Units_Set_Draw_Priority();
    Map_Build_Unit_Grid(x, y, combat_loc_plane);
    Map_Invalidate();
    Minimap_Build_View();
    Overland_Screen_Draw(&x, &y, combat_loc_plane, x, y, current_player);
    Screen_Present();
    Screen_Copy_Front();
    Map_Invalidate();
    Timer_Mark();
    Wait_Ticks(6);
    map_view_x = saved_x;
    map_view_y = saved_y;
}
