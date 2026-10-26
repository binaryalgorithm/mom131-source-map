/* AITURN.C - WIZARDS.EXE overlay code segment d208 (stub 2615): the AI's overland turn - repairing stray unit
   coordinates, then for each AI wizard (during a Time Stop only its caster; while it has a fortress or is casting Spell of Return)
   its diplomacy and strategy, the strength tables (city values, own and enemy stacks, city threats), the landmass
   balance, the overland spell, the magic power split, gold / mana balance and tax, city production, the threat map,
   the landmass plan and the unit orders; then the neutral player. Also the EMS tables of the AI (landmass and coast
   tile lists, the stack tables) and a few helpers.
   This module's data (DS:6f60..6f75): ai_turn_unused_6f60 and two unread tables (owner not fixed by the EXE); no literals, no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS d208 src/wizards/AITURN.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Accept_Mercenary_Offer(int player, int cost);
void AI_Overland_Turn(void);
void AI_Landmass_Balance(int player);
int AI_Magic_Allocation(int player);
int AI_Knows_Spell(int player, int spell);
int AI_Count_Known_Spells(int player);
void AI_Build_Strength_Tables(int player);
void AI_List_Hostile_Wizards(int ai);
int AI_Pick_Min_Landmass_Unused(int *landmass, int *plane);
int AI_Count_Units_At(int x, int y, int plane);
void AI_Balance_Gold_Mana(int player);
void AI_Set_Tax(int player);
void AI_Neutral_City_Farmers(void);
void AI_Disband_Extra_Settlers(int player);
void AI_Disband_Stasis_Units(void);
void AI_Map_EMS_Tables(void);
void AI_Build_Landmass_Tile_Lists(void);
void AI_Build_Coast_Tile_Lists(void);

#include "GAME.H"

int ai_turn_unused_6f60 = 0;                    /* DS:6f60  AI_Overland_Turn clears it, nothing reads it */
char ai_unused_6f62[8] = { 2, 3, 4, 5, 6, 7, 8, 0 };   /* DS:6f62  no code reads it (owner too) */
int ai_unused_6f6a[6] = { 0, 0, 10, 10, 50, 50 };     /* DS:6f6a  as ai_unused_6f62 */

extern unsigned ems_frame_seg;                  /* DS:760c */
extern int *p_ai_circle_timer;                  /* DS:8f98 */
extern int ai_turn_unused_8fc4;                 /* DS:8fc4  cleared each AI turn, never read */
extern int ai_turn_unused_8fc6;                 /* DS:8fc6  as ai_turn_unused_8fc4 */
extern int ai_own_transport_count;              /* DS:9094 */
extern int *ai_lm_surplus[2];                   /* DS:90f2 */
extern int *ai_lm_target_value[2];              /* DS:90f6 */
extern int *ai_lm_own_value[2];                 /* DS:90fa */
extern unsigned *ai_lm_enemy_strength[2];       /* DS:90fe */
extern unsigned *ai_lm_own_strength[2];         /* DS:9102 */
extern struct AIEnemyStack *p_ai_enemy_stacks;  /* DS:9106 */
extern struct AIStack *p_ai_own_stacks;             /* DS:9108 */
extern int ai_enemy_stack_count;                /* DS:910a */
extern int ai_own_stack_count;                  /* DS:910c */
extern int ai_hostile_wizards[6];               /* DS:910e */
extern int ai_hostile_count;                    /* DS:911a */
extern int *ai_city_threat;                     /* DS:9122 */
extern int *ai_city_garrison;                   /* DS:9124 */
extern int *ai_city_target_value;               /* DS:9126 */
extern int *ai_city_own_value;                  /* DS:9128 */
extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern unsigned far *p_threat_maps[2];          /* DS:9c94 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern int far *p_coast_tile_head[2];           /* DS:9d2a */
extern int far *p_coast_tile_next[2];           /* DS:9d32 */
extern char far *p_coast_tile_y[2];             /* DS:9d3a  signed: f48f reads it with cbw */
extern char far *p_coast_tile_x[2];             /* DS:9d42  signed: f48f reads it with cbw */
extern int far *p_ai_square_stack_units[AI_MAX_STACKS];           /* DS:9d4c */
extern char far *p_ai_square_stack_size;               /* DS:9e8c */
extern char far *p_ai_square_stack_class;              /* DS:9e90 */
extern char far *p_ai_square_stack_plane;              /* DS:9e94 */
extern char far *p_ai_square_stack_y;                  /* DS:9e98 */
extern char far *p_ai_square_stack_x;                  /* DS:9e9c */
extern int far *p_landmass_tile_head[2];        /* DS:9ea0 */
extern int far *p_landmass_tile_next[2];        /* DS:9ea8 */
extern char far *p_landmass_tile_y[2];          /* DS:9eb0  signed: f48f reads it with cbw */
extern char far *p_landmass_tile_x[2];          /* DS:9eb8  signed: f48f reads it with cbw */
extern int ems_cont_handle;                     /* DS:9ec0 */
extern int game_turn;                           /* DS:bd90 */
extern int num_cities;                          /* DS:bd94 */
extern int magic_setting;                       /* DS:bd98 */
extern int num_players;                         /* DS:bd9c */
extern int ai_replan;                           /* DS:c9f0  set by OVERCAST.C when a cast changed the map */

int abs(int x);
void far *Seg_To_Far_Ptr(unsigned seg);
void EMS_Map_Pages(int first, int handle);
void EMS_Map_Default(void);
void AI_Init_Buffers(void);
int AI_Hostility_Replan(int ai);
int AI_Choose_Strategy(int ai);
void AI_Choose_Attack_Landmass(int player);
void AI_Choose_Overland_Spell(int player);
void AI_Drop_Bad_Globals(int player);
void AI_City_Production_Turn(int player);
void AI_Build_Threat_Map(int player);
int AI_Landmass_Plan(int player);
void AI_Unit_Dispatch(int ai);
int AI_Execute_Unit_Orders(int player);
void AI_Neutral_Stack_Orders(void);
int AI_Neutral_City_Raiders(void);
int AI_Neutral_Lair_Monsters(void);
void AI_Neutral_Trim_Garrison(void);
int AI_Unit_Strength(int unit);
int Unit_Can_Cross_Water(int unit);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana);
int City_Min_Farmers(int city);
int Tile_Is_Sea_NoWrap(int x, int y, int plane);

/* (player, cost) whether the AI takes a mercenary / hero offer: never before turn 50, else if it costs at most half
   its gold */
int AI_Accept_Mercenary_Offer(int player, int cost)
{
    if (game_turn < 50)
        return 0;
    if (wizards[player].gold / 2 < cost)
        return 0;
    return 1;
}

/* the AI players' overland turn (see the file comment)
   Numbered: BUG #239 (BUGS_AND_QUIRKS.txt) */
void AI_Overland_Turn(void)
{
    int player;
    int i;

    for (i = 0; i < num_units; i++) {
        if (p_units[i].plane < 0 || p_units[i].plane > 1) {
            p_units[i].plane = 0;
            p_units[i].owner = PLAYER_NONE;
        }
        if (p_units[i].x < 0 || p_units[i].x >= MAP_WIDTH) {
            p_units[i].x = 0;
            p_units[i].owner = PLAYER_NONE;
        }
        if (p_units[i].y < 0 || p_units[i].y >= MAP_HEIGHT) {
            p_units[i].y = 0;
            p_units[i].owner = PLAYER_NONE;
        }
    }
    EMS_Map_Default();
    AI_Init_Buffers();
    for (player = 1; player < num_players; player++) {
        if (time_stop_player != 0 && time_stop_player - 1 != player)
            continue;
        if (p_fortresses[player].active != 1 && wizards[player].casting_spell != SPELL_SPELL_OF_RETURN)
            continue;
        p_ai_circle_timer[player]--;
        AI_Hostility_Replan(player);
        AI_Choose_Strategy(player);
        ai_turn_unused_6f60 = 0;
        AI_List_Hostile_Wizards(player);
        AI_Build_Strength_Tables(player);
        AI_Landmass_Balance(player);
        AI_Choose_Attack_Landmass(player);
        if (wizards[player].casting_cost_left <= 0 && wizards[player].casting_spell != SPELL_NONE) {
            Overland_Cast_Spell(player);
            EMS_Map_Default();
            wizards[player].casting_spell = SPELL_NONE;
            if (ai_replan == 1) {
                AI_Init_Buffers();
                AI_List_Hostile_Wizards(player);
                AI_Build_Strength_Tables(player);
            }
            if (wizards[player].casting_spell == SPELL_NONE)   /* always true: cleared just above */
                AI_Choose_Overland_Spell(player);
        }
        if (wizards[player].casting_spell == SPELL_NONE)
            AI_Choose_Overland_Spell(player);
        EMS_Map_Default();
        AI_Magic_Allocation(player);
        AI_Drop_Bad_Globals(player);
        AI_Balance_Gold_Mana(player);
        AI_Set_Tax(player);
        ai_turn_unused_8fc4 = 0;
        ai_turn_unused_8fc6 = 0;
        for (i = 0; i < num_players; i++)       /* Diplo_Tick_Peace_Turns counts them down too: an AI's run at double speed */
            if (wizards[player].peace_turns[i] != 0)
                wizards[player].peace_turns[i] = wizards[player].peace_turns[i] - 1;
        AI_City_Production_Turn(player);
        AI_Build_Threat_Map(player);
        AI_Landmass_Plan(player);
        AI_Unit_Dispatch(player);
        EMS_Map_Default();
        AI_Disband_Extra_Settlers(player);
    }
    EMS_Map_Default();
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner != PLAYER_HUMAN)
            p_units[i].goto_blocked = 0;
    for (player = 1; player < num_players; player++)
        if ((time_stop_player == 0 || time_stop_player - 1 == player) &&
            (p_fortresses[player].active == 1 || wizards[player].casting_spell == SPELL_SPELL_OF_RETURN))
            AI_Execute_Unit_Orders(player);
    EMS_Map_Default();
    AI_City_Production_Turn(PLAYER_NEUTRAL);
    AI_Neutral_City_Farmers();
    AI_Neutral_Stack_Orders();
    AI_Execute_Unit_Orders(PLAYER_NEUTRAL);
    AI_Neutral_City_Raiders();
    AI_Neutral_Lair_Monsters();
    AI_Neutral_Trim_Garrison();
    AI_Disband_Stasis_Units();
}

/* (player) per plane and landmass: own and enemy strength (stacks / 10, lair guardians by cost / 10), own and target
   value (cities, nodes by power and magic setting), and the surplus = own - own value / 10 - enemy, less 25 */
void AI_Landmass_Balance(int player)
{
    int lm;
    int plane;
    int surplus;
    int i;

    for (i = 0; i < MAX_LANDMASSES; i++) {
        ai_lm_own_strength[0][i] = 0;
        ai_lm_own_strength[1][i] = 0;
        ai_lm_enemy_strength[0][i] = 0;
        ai_lm_enemy_strength[1][i] = 0;
        ai_lm_own_value[0][i] = 0;
        ai_lm_own_value[1][i] = 0;
        ai_lm_target_value[0][i] = 0;
        ai_lm_target_value[1][i] = 0;
    }
    for (i = 0; i < ai_own_stack_count; i++) {
        plane = p_ai_own_stacks[i].plane;
        lm = p_landmass_ids[plane][p_ai_own_stacks[i].y][p_ai_own_stacks[i].x];
        if (lm < MAX_LANDMASSES)
            ai_lm_own_strength[plane][lm] += p_ai_own_stacks[i].strength / 10;
    }
    for (i = 0; i < ai_enemy_stack_count; i++) {
        plane = p_ai_enemy_stacks[i].plane;
        lm = p_landmass_ids[plane][p_ai_enemy_stacks[i].y][p_ai_enemy_stacks[i].x];
        if (lm < MAX_LANDMASSES)
            ai_lm_enemy_strength[plane][lm] += p_ai_enemy_stacks[i].strength / 10;
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].active != 1)
            continue;
        plane = p_lairs[i].plane;
        lm = p_landmass_ids[plane][p_lairs[i].y][p_lairs[i].x];
        if (lm >= MAX_LANDMASSES)
            continue;
        ai_lm_enemy_strength[plane][lm] +=
            (p_lairs[i].guard1_count & LAIR_GUARDS_LEFT_MASK) * (unit_types[p_lairs[i].guard1_type].cost / 10);
        ai_lm_enemy_strength[plane][lm] +=
            (p_lairs[i].guard2_count & LAIR_GUARDS_LEFT_MASK) * (unit_types[p_lairs[i].guard2_type].cost / 10);
    }
    for (i = 0; i < num_cities; i++) {
        plane = p_cities[i].plane;
        lm = p_landmass_ids[plane][p_cities[i].y][p_cities[i].x];
        if (lm < MAX_LANDMASSES) {
            ai_lm_own_value[plane][lm] += ai_city_own_value[i];
            ai_lm_target_value[plane][lm] += ai_city_target_value[i];
        }
    }
    for (i = 0; i < NUM_NODES; i++) {
        plane = p_nodes[i].plane;
        lm = p_landmass_ids[plane][p_nodes[i].y][p_nodes[i].x];
        if (lm < MAX_LANDMASSES) {
            if (p_nodes[i].owner == player)
                ai_lm_own_value[plane][lm] += p_nodes[i].power * (magic_setting + 1) * 2;
            else
                ai_lm_target_value[plane][lm] += p_nodes[i].power * (magic_setting + 1) * 2;
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++)
        for (lm = 0; lm < MAX_LANDMASSES; lm++) {
            surplus = ai_lm_own_strength[plane][lm] - ai_lm_own_value[plane][lm] / 10 - ai_lm_enemy_strength[plane][lm];
            if (surplus > 0) {
                surplus -= 25;
                if (surplus < 0)
                    surplus = 0;
            }
            ai_lm_surplus[plane][lm] = surplus;
        }
}

/* (player) every Random(10) + 15 turns re-splits the power: research / mana / skill 10 / 30 / 10 plus five +10
   picks biased by the objective, casting skill, Alchemy, the AI focus and the mana reserve; researching Fire
   Elemental (SPELL_FIRE_ELEMENTAL, bug #154) 70 / 20 / 10, casting Spell of Mastery 0 / 90 / 10, before turn 30 35 / 40 / 25;
   a wizard with no spell
   to research gets a skill share of half its research share (replacing the old skill share) and no research. Mana takes the rest. The result is meaningless (0 when it is
   not yet time; no return statement otherwise)
   Numbered: BUG #324 (BUGS_AND_QUIRKS.txt) */
int AI_Magic_Allocation(int player)
{
    int realm;
    int slot;
    int found;
    int k;
    int casting_skill;
    int reserve;
    int choice;                                 /* 0 mana, 1 skill, 2 research (the switch below) */

    casting_skill = Wizard_Casting_Skill(player);
    reserve = wizards[player].mana_reserve;
    wizards[player].ai_magic_timer--;
    if (wizards[player].ai_magic_timer > 0)
        return 0;
    wizards[player].ai_magic_timer = Random(10) + 15;
    found = 0;
    for (realm = 0; realm < NUM_REALMS && found == 0; realm++)
        for (slot = 0; slot < SPELLS_PER_REALM && found == 0; slot++)
            if (wizards[player].spell_library[realm][slot] == SPELLSTATE_RESEARCHABLE)
                found++;
    /* BUG #324: the candidates (SPELLSTATE_OFFERED) are not counted, so once every spell left to learn is a candidate the
       AI stops researching (research 0) with spells still to research; clarified using ReMoM repo */
    if (found == 0) {
        wizards[player].skill_percentage = wizards[player].research_percentage / 2;
        wizards[player].research_percentage = 0;
    } else {
        wizards[player].research_percentage = 10;
        wizards[player].skill_percentage = 10;
        wizards[player].mana_percentage = 30;
        for (k = 0; k < 5; k++) {
            choice = Random(3) - 1;
            if (wizards[player].objective == OBJECTIVE_THEURGIST && Random(3) == 1)
                choice = 2;
            if (wizards[player].objective == OBJECTIVE_MILITARIST && Random(3) == 1)
                choice = 1;
            if (wizards[player].objective == OBJECTIVE_PERFECTIONIST && Random(3) == 1)
                choice = 0;
            if (Wizard_Casting_Skill(player) > 100 && Random(3) == 1)
                choice = 2;
            if (wizards[player].retorts[RETORT_ALCHEMY] != 0 && Random(3) == 1)
                choice = 0;
            if (wizards[player].ai_focus == 6 && Random(3) == 1)   /* no AI focus is 6 (AIFocus ends at 5) */
                choice = 0;
            if (wizards[player].ai_focus == AI_FOCUS_SPELLS && Random(3) == 1)
                choice = 2;
            if (wizards[player].ai_focus == AI_FOCUS_SKILL && Random(3) == 1)
                choice = 1;
            if (wizards[player].mana_reserve < game_turn * 3 && Random(2) == 1)
                choice = 0;
            if (wizards[player].mana_reserve > 1000) {
                if (Random(2) == 1)
                    choice = 2;
                else
                    choice = 1;
            }
            if (choice == 1 && reserve * 4 < casting_skill)
                choice = 0;
            switch (choice) {
            case 1:
                wizards[player].skill_percentage = wizards[player].skill_percentage + 10;
                break;
            case 0:
                wizards[player].mana_percentage = wizards[player].mana_percentage + 10;
                break;
            case 2:
                wizards[player].research_percentage = wizards[player].research_percentage + 10;
                break;
            }
        }
        if (wizards[player].research_spell == SPELL_FIRE_ELEMENTAL) {
            wizards[player].research_percentage = 70;
            wizards[player].skill_percentage = 10;
            wizards[player].mana_percentage = 20;
        }
        if (wizards[player].casting_spell == SPELL_SPELL_OF_MASTERY) {
            wizards[player].research_percentage = 0;
            wizards[player].skill_percentage = 10;
            wizards[player].mana_percentage = 90;
        }
        if (game_turn < 30) {
            wizards[player].research_percentage = 35;
            wizards[player].skill_percentage = 25;
            wizards[player].mana_percentage = 40;
        }
    }
    wizards[player].mana_percentage = 100 - wizards[player].skill_percentage - wizards[player].research_percentage;
}

/* (player, spell) whether the wizard knows the spell (library state SPELLSTATE_KNOWN) */
int AI_Knows_Spell(int player, int spell)
{
    if (spell == SPELL_NONE)
        return 0;
    if (wizards[player].spell_library[SPELL_REALM(spell)][SPELL_SLOT(spell)] == SPELLSTATE_KNOWN)
        return 1;
    return 0;
}

/* (player) the number of known spells (library state SPELLSTATE_KNOWN) */
int AI_Count_Known_Spells(int player)
{
    int count;
    int realm;
    int slot;

    count = 0;
    for (realm = 0; realm < 6; realm++)
        for (slot = 0; slot < SPELLS_PER_REALM; slot++)
            if (wizards[player].spell_library[realm][slot] == SPELLSTATE_KNOWN)
                count++;
    return count;
}

/* (player) the AI's strength tables: each city's own value (population, doubled for Beastmen, Dark Elves,
   Draconians, Dwarves and race 14 (RACE_GENERIC), + 2 per building,
   + 2 x turn for the fortress city) or target value (hostile or neutral: the same, + 200 for a fortress); the own
   stacks (up to 80: flags, strength, transport capacity, unit count, landmass, status) and their city garrisons; every
   other unit not standing in a city into the enemy stacks (up to 240) and the city threats (on the city in full, within 2 squares half).
   BUGS #21: the human-city doubling tests p_cities[i] with a stale i (the unit loop has not run yet); the threat doubling runs over the enemy
   stack count with the last city owner; the Wind Walking pass sets AISTACK_CROSSES_WATER on the stack of the stale index k;
   enemy units of allied wizards and dead units count too. For the neutral player a new stack is kept only when
   its unit is a settler (AFLAG_CREATE_OUTPOST) outside a city; fantastic units joining a kept stack flag it
   AISTACK_SETTLER too
   Numbered: BUG #21, #137, #351 (BUGS_AND_QUIRKS.txt) */
void AI_Build_Strength_Tables(int player)
{
    int city;
    int owner;
    int k;
    int found;
    int strength;
    int can_fly;
    int wind_walking;
    int is_hero_or_ship;                        /* type < UNIT_BARBARIAN_SWORDSMEN: also the Catapult and Barbarian Spearmen */
    int can_meld;
    int can_cross;
    int stack_city;
    int i;

    for (city = 0; city < num_cities; city++) {
        ai_city_threat[city] = 0;
        ai_city_garrison[city] = 0;
        ai_city_target_value[city] = 0;
        ai_city_own_value[city] = 0;
        owner = p_cities[city].owner;
        if (owner == player) {
            ai_city_own_value[city] = p_cities[city].population;
            if (p_cities[city].race == RACE_BEASTMEN || p_cities[city].race == RACE_DARK_ELF ||
                p_cities[city].race == RACE_DRACONIAN || p_cities[city].race == RACE_DWARVEN ||
                p_cities[city].race == RACE_GENERIC)
                ai_city_own_value[city] *= 2;
            ai_city_own_value[city] += p_cities[city].building_count * 2;
            if (p_fortresses[player].x == p_cities[city].x && p_fortresses[player].y == p_cities[city].y &&
                p_fortresses[player].plane == p_cities[city].plane)
                ai_city_own_value[city] += game_turn * 2;
        } else if ((wizards[player].hostility[owner] >= HOSTILITY_HOSTILE && wizards[player].peace_turns[owner] == 0) ||
                   owner == PLAYER_NEUTRAL) {
            ai_city_target_value[city] = p_cities[city].population;
            if (p_cities[city].race == RACE_BEASTMEN || p_cities[city].race == RACE_DARK_ELF ||
                p_cities[city].race == RACE_DRACONIAN || p_cities[city].race == RACE_DWARVEN ||
                p_cities[city].race == RACE_GENERIC)
                ai_city_target_value[city] *= 2;
            ai_city_target_value[city] += p_cities[city].building_count * 2;
            if (p_fortresses[owner].x == p_cities[city].x && p_fortresses[owner].y == p_cities[city].y &&
                p_fortresses[owner].plane == p_cities[city].plane && owner != PLAYER_NEUTRAL)
                ai_city_target_value[city] += 200;
            if (p_cities[i].owner == PLAYER_HUMAN)
                ai_city_target_value[city] *= 2;
        }
    }
    ai_own_stack_count = 0;
    ai_enemy_stack_count = 0;
    for (i = 0; i < num_units; i++) {
        strength = AI_Unit_Strength(i);
        can_fly = Unit_Can_Fly(i);
        wind_walking = Unit_Has_Wind_Walking(i);
        can_cross = Unit_Can_Cross_Water(i);
        if (p_units[i].type < UNIT_BARBARIAN_SWORDSMEN)
            is_hero_or_ship = 1;
        else
            is_hero_or_ship = 0;
        if (unit_types[p_units[i].type].ability_flags & AFLAG_MELD_WITH_NODE)
            can_meld = 1;
        else
            can_meld = 0;
        if (p_units[i].owner == player) {
            if (ai_own_stack_count < AI_MAX_STACKS) {
                found = 0;
                for (k = 0; k < ai_own_stack_count && found == 0; k++) {
                    if (p_units[i].x == p_ai_own_stacks[k].x && p_units[i].y == p_ai_own_stacks[k].y &&
                        p_units[i].plane == p_ai_own_stacks[k].plane) {
                        p_ai_own_stacks[k].count = p_ai_own_stacks[k].count + 1;
                        if (unit_types[p_units[i].type].transport_capacity > 0) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_TRANSPORT;
                            p_ai_own_stacks[k].spare_transport += unit_types[p_units[i].type].transport_capacity;
                            strength = 0;
                        } else if (can_cross == 0 && can_fly == 0)
                            p_ai_own_stacks[k].spare_transport = p_ai_own_stacks[k].spare_transport - 1;
                        if (unit_types[p_units[i].type].construction > 0) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_ENGINEER;
                            strength = 0;
                        }
                        if (unit_types[p_units[i].type].ability_flags & AFLAG_PURIFY)
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_PURIFIER;
                        if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_SETTLER;
                            strength = 0;
                        }
                        if (player == PLAYER_NEUTRAL && (unit_types[p_units[i].type].ability_flags & AFLAG_FANTASTIC)) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_SETTLER;
                            strength = 0;
                        }
                        if (unit_types[p_units[i].type].transport_capacity > 0) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_TRANSPORT;
                            strength = 0;
                        }
                        if (can_fly == 0 && can_cross == 0)
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags & ~AISTACK_CROSSES_WATER;
                        if (wind_walking == 1) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_WIND_WALKER;
                            p_ai_own_stacks[k].spare_transport = 8;
                        }
                        if (is_hero_or_ship == 1) {
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_HERO;
                            strength = strength * 2 / 3;
                        }
                        if (can_meld == 1)
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_MELDER;
                        if (unit_types[p_units[i].type].construction > 0) {
                            strength = 0;
                            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_ENGINEER;
                        }
                        p_ai_own_stacks[k].strength += strength;
                        if (p_units[i].order == ORDER_BUILD_ROAD)
                            p_ai_own_stacks[k].status = ORDER_BUILD_ROAD;
                        else if (p_ai_own_stacks[k].status != ORDER_BUILD_ROAD && p_units[i].order == ORDER_GOTO)
                            p_ai_own_stacks[k].status = ORDER_GOTO;
                        found = 1;
                    }
                }
                if (found == 0) {
                    p_ai_own_stacks[ai_own_stack_count].x = p_units[i].x;
                    p_ai_own_stacks[ai_own_stack_count].y = p_units[i].y;
                    p_ai_own_stacks[ai_own_stack_count].plane = p_units[i].plane;
                    p_ai_own_stacks[ai_own_stack_count].flags = AISTACK_CROSSES_WATER;
                    p_ai_own_stacks[ai_own_stack_count].strength = 0;
                    p_ai_own_stacks[ai_own_stack_count].spare_transport = 0;
                    p_ai_own_stacks[ai_own_stack_count].count = 1;
                    p_ai_own_stacks[ai_own_stack_count].landmass = 0;
                    p_ai_own_stacks[ai_own_stack_count].landmass = p_landmass_ids[p_units[i].plane][p_units[i].y][p_units[i].x];
                    if (unit_types[p_units[i].type].transport_capacity > 0) {
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_TRANSPORT;
                        p_ai_own_stacks[ai_own_stack_count].spare_transport += unit_types[p_units[i].type].transport_capacity;
                        strength = 0;
                    } else if (can_cross == 0 && can_fly == 0)
                        p_ai_own_stacks[ai_own_stack_count].spare_transport =
                            p_ai_own_stacks[ai_own_stack_count].spare_transport - 1;
                    if (unit_types[p_units[i].type].construction > 0) {
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_ENGINEER;
                        strength = 0;
                    }
                    if (unit_types[p_units[i].type].ability_flags & AFLAG_PURIFY)
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_PURIFIER;
                    if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST) {
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_SETTLER;
                        strength = 0;
                    }
                    if (can_fly == 0 && can_cross == 0)
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags & ~AISTACK_CROSSES_WATER;
                    if (wind_walking == 1) {
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_WIND_WALKER;
                        p_ai_own_stacks[ai_own_stack_count].spare_transport = 8;
                    }
                    if (is_hero_or_ship == 1)           /* no two-thirds strength cut for a stack's first unit */
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_HERO;
                    if (can_meld == 1)
                        p_ai_own_stacks[ai_own_stack_count].flags = p_ai_own_stacks[ai_own_stack_count].flags | AISTACK_MELDER;
                    p_ai_own_stacks[ai_own_stack_count].strength = strength;
                    p_ai_own_stacks[ai_own_stack_count].status = p_units[i].order;
                    if (p_units[i].order == ORDER_BUILD_ROAD)
                        p_ai_own_stacks[ai_own_stack_count].status = ORDER_BUILD_ROAD;
                    else if (p_ai_own_stacks[ai_own_stack_count].status != ORDER_BUILD_ROAD && p_units[i].order == ORDER_GOTO)
                        p_ai_own_stacks[ai_own_stack_count].status = ORDER_GOTO;
                    if (player == PLAYER_NEUTRAL) {
                        found = 0;
                        if ((p_ai_own_stacks[ai_own_stack_count].flags & AISTACK_SETTLER) == 0)
                            found = 1;
                        for (stack_city = 0; stack_city < num_cities && found == 0; stack_city++)
                            if (p_cities[stack_city].x == p_ai_own_stacks[ai_own_stack_count].x &&
                                p_cities[stack_city].y == p_ai_own_stacks[ai_own_stack_count].y &&
                                p_cities[stack_city].plane == p_ai_own_stacks[ai_own_stack_count].plane)
                                found = 1;
                        if (found == 0)
                            ai_own_stack_count++;
                    } else
                        ai_own_stack_count++;
                }
                for (k = 0; k < num_cities; k++)
                    if (p_units[i].x == p_cities[k].x && p_units[i].y == p_cities[k].y &&
                        p_units[i].plane == p_cities[k].plane)
                        ai_city_garrison[k] += strength;
            }
        } else {
            if (ai_enemy_stack_count >= AI_MAX_ENEMY_STACKS)
                continue;
            found = 0;
            for (k = 0; k < num_cities && found == 0; k++)
                if (p_units[i].x == p_cities[k].x && p_units[i].y == p_cities[k].y &&
                    p_units[i].plane == p_cities[k].plane)
                    found = 1;
            for (k = 0; k < ai_enemy_stack_count && found == 0; k++) {
                if (p_units[i].x == p_ai_enemy_stacks[k].x && p_units[i].y == p_ai_enemy_stacks[k].y &&
                    p_units[i].plane == p_ai_enemy_stacks[k].plane) {
                    if (unit_types[p_units[i].type].transport_capacity > 0)
                        strength = 0;
                    if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST)
                        strength = 0;
                    if (unit_types[p_units[i].type].construction > 0)
                        strength = 0;
                    p_ai_enemy_stacks[k].strength += strength;
                    found = 1;
                }
            }
            if (found == 0) {
                p_ai_enemy_stacks[ai_enemy_stack_count].x = p_units[i].x;
                p_ai_enemy_stacks[ai_enemy_stack_count].y = p_units[i].y;
                p_ai_enemy_stacks[ai_enemy_stack_count].plane = p_units[i].plane;
                if (unit_types[p_units[i].type].transport_capacity > 0)
                    strength = 0;
                if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST)
                    strength = 0;
                if (unit_types[p_units[i].type].construction > 0)
                    strength = 0;
                p_ai_enemy_stacks[ai_enemy_stack_count].strength = strength;
                ai_enemy_stack_count++;
            }
            for (k = 0; k < num_cities; k++) {
                if (p_units[i].plane != p_cities[k].plane)
                    continue;
                if (p_units[i].x == p_cities[k].x && p_units[i].y == p_cities[k].y)
                    ai_city_threat[k] += strength;
                else if (abs(p_units[i].x - p_cities[k].x) < 3 && abs(p_units[i].y - p_cities[k].y) < 3)
                    ai_city_threat[k] += strength / 2;
            }
        }
    }
    for (i = 0; i < ai_enemy_stack_count; i++)
        if (wizards[player].hostility[owner] == HOSTILITY_HOSTILE)
            ai_city_threat[i] *= 2;
    for (i = 0; i < ai_own_stack_count; i++)
        if (p_ai_own_stacks[i].flags & AISTACK_WIND_WALKER)
            p_ai_own_stacks[k].flags = p_ai_own_stacks[k].flags | AISTACK_CROSSES_WATER;
    ai_own_transport_count = 0;
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner == player && unit_types[p_units[i].type].transport_capacity > 0)
            ai_own_transport_count++;
}

/* (ai) lists the wizards it is hostile to (hostility >= HOSTILITY_HOSTILE) and not at peace with */
void AI_List_Hostile_Wizards(int ai)
{
    int player;

    ai_hostile_count = 0;
    for (player = 0; player < num_players; player++)
        if (wizards[ai].hostility[player] >= HOSTILITY_HOSTILE && wizards[ai].peace_turns[player] == 0 && ai != player) {
            ai_hostile_wizards[ai_hostile_count] = player;
            ai_hostile_count++;
        }
}

/* (&landmass, &plane) the valued landmass with the lowest surplus; whether one was found. No callers */
int AI_Pick_Min_Landmass_Unused(int *landmass, int *plane)
{
    int best_landmass;
    int best;
    int best_plane;
    int try_plane;
    int lm;

    best_landmass = -1;
    best_plane = -1;
    best = 10000;
    for (try_plane = 0; try_plane < NUM_PLANES; try_plane++)
        for (lm = 0; lm < MAX_LANDMASSES; lm++)
            if (ai_lm_own_value[try_plane][lm] != 0 && ai_lm_surplus[try_plane][lm] < best) {
                best_landmass = lm;
                best_plane = try_plane;
                best = ai_lm_surplus[try_plane][lm];
            }
    *landmass = best_landmass;
    *plane = best_plane;
    if (best_plane != -1)
        return 1;
    return 0;
}

/* (x, y, plane) the number of units of any owner on the square */
int AI_Count_Units_At(int x, int y, int plane)
{
    int count;
    int unit;

    count = 0;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane)
            count++;
    return count;
}

/* (player) balances gold and mana: while casting Spell of Return the gold (half without Alchemy) becomes the mana
   reserve (BUG #275: it replaces the reserve, = not +=, so the mana already saved is lost);
   with Alchemy gold + mana is re-split 90 / 10 before turn 150, then 30 / 70 (each capped at 30000);
   otherwise an empty reserve takes a quarter of the gold (and half the gold goes), and on Random(10) == 1: mana below gold / 5
   (gold over 100) -> half the gold goes, a quarter of it to the mana; gold below mana / 4 (mana over 100) -> the
   reverse; else before turn 100 with over 50 mana -> half the mana goes, a quarter to the gold */
void AI_Balance_Gold_Mana(int player)
{
    long total;
    long new_gold;
    long new_mana;

    if (wizards[player].casting_spell == SPELL_SPELL_OF_RETURN) {
        wizards[player].mana_reserve = wizards[player].retorts[RETORT_ALCHEMY] == 1 ? wizards[player].gold :
                                                                                     wizards[player].gold / 2;
        wizards[player].gold = 0;
    } else if (wizards[player].retorts[RETORT_ALCHEMY] == 1) {
        if (game_turn < 150) {
            total = (long)wizards[player].gold + wizards[player].mana_reserve;
            new_gold = total * 9 / 10;
            if (new_gold > RESOURCE_CAP)
                new_gold = RESOURCE_CAP;
            new_mana = total - new_gold;
            if (new_mana > RESOURCE_CAP)
                new_mana = RESOURCE_CAP;
        } else {
            total = (long)wizards[player].gold + wizards[player].mana_reserve;
            new_mana = total * 7 / 10;
            if (new_mana > RESOURCE_CAP)
                new_mana = RESOURCE_CAP;
            new_gold = total - new_mana;
            if (new_gold > RESOURCE_CAP)
                new_gold = RESOURCE_CAP;
        }
        wizards[player].gold = new_gold;
        wizards[player].mana_reserve = new_mana;
    } else {
        if (wizards[player].mana_reserve == 0) {
            wizards[player].mana_reserve = wizards[player].gold / 4;
            wizards[player].gold = wizards[player].gold / 2;
        }
        if (Random(10) == 1) {
            if (wizards[player].mana_reserve < wizards[player].gold / 5) {
                if (wizards[player].gold > 100) {
                    new_gold = wizards[player].gold;
                    new_mana = wizards[player].mana_reserve;
                    new_mana += new_gold / 4;
                    new_gold -= new_gold / 2;
                    if (new_mana > RESOURCE_CAP)
                        new_mana = RESOURCE_CAP;
                    if (new_gold > RESOURCE_CAP)
                        new_gold = RESOURCE_CAP;
                    wizards[player].gold = new_gold;
                    wizards[player].mana_reserve = new_mana;
                }
            } else if (wizards[player].gold < wizards[player].mana_reserve / 4) {
                if (wizards[player].mana_reserve > 100) {
                    new_gold = wizards[player].gold;
                    new_mana = wizards[player].mana_reserve;
                    new_gold += new_mana / 4;
                    new_mana -= new_mana / 2;
                    if (new_mana > RESOURCE_CAP)
                        new_mana = RESOURCE_CAP;
                    if (new_gold > RESOURCE_CAP)
                        new_gold = RESOURCE_CAP;
                    wizards[player].gold = new_gold;
                    wizards[player].mana_reserve = new_mana;
                }
            } else if (game_turn < 100 && wizards[player].mana_reserve > 50) {
                new_gold = wizards[player].gold;
                new_mana = wizards[player].mana_reserve;
                new_gold += new_mana / 4;
                new_mana -= new_mana / 2;
                if (new_mana > RESOURCE_CAP)
                    new_mana = RESOURCE_CAP;
                if (new_gold > RESOURCE_CAP)
                    new_gold = RESOURCE_CAP;
                wizards[player].gold = new_gold;
                wizards[player].mana_reserve = new_mana;
            }
        }
    }
}

/* (player) the tax rate: +1 while the gold income is below 75 per 50 turns played (and the rate below 6), -1 above
   200 income (not below 1), at most 4; a food deficit moves random own cities' workers to farming (2 food each, up to
   50 tries) */
void AI_Set_Tax(int player)
{
    int unused_a;
    int unused_b;
    int unused_c;
    int tries;
    int gold;
    int mana;
    int food;
    int city;

    unused_a = 0;
    unused_c = 0;
    unused_b = 0;
    Wizard_Income_Summary(player, &gold, &food, &mana);
    if (game_turn / 50 * 75 > gold && wizards[player].tax_rate < 6)
        wizards[player].tax_rate++;
    Wizard_Income_Summary(player, &gold, &food, &mana);
    if (gold > 200 && wizards[player].tax_rate > 1)
        wizards[player].tax_rate--;
    if (wizards[player].tax_rate > 4)
        wizards[player].tax_rate = 4;
    tries = 0;
    if (food < 0) {
        food = -food;
        for (; food > 0 && tries < 50; tries++) {
            city = Random(num_cities) - 1;
            if (p_cities[city].owner == player && p_cities[city].farmers < p_cities[city].population) {
                p_cities[city].farmers = p_cities[city].farmers + 1;
                food -= 2;
            }
        }
    }
}

/* the neutral cities' farmers: (minimum farmers + units in the city) / 3 for Halflings or with an Animists' Guild,
   else / 2, rounded up, at most the population */
void AI_Neutral_City_Farmers(void)
{
    int min_farmers;
    register int city;
    register int units_at;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner != PLAYER_NEUTRAL)
            continue;
        min_farmers = City_Min_Farmers(city);
        units_at = AI_Count_Units_At(p_cities[city].x, p_cities[city].y, p_cities[city].plane);
        if (p_cities[city].race == RACE_HALFLING || p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_BUILT) {
            p_cities[city].farmers = (min_farmers + units_at) / 3;
            if ((min_farmers + units_at) % 3 != 0)
                p_cities[city].farmers = p_cities[city].farmers + 1;
        } else {
            p_cities[city].farmers = (min_farmers + units_at) / 2;
            if ((min_farmers + units_at) % 2 != 0)
                p_cities[city].farmers = p_cities[city].farmers + 1;
        }
        if (p_cities[city].farmers > p_cities[city].population)
            p_cities[city].farmers = p_cities[city].population;
    }
}

/* (player) disbands every own settler past the first per landmass, and after turn 200 every road builder past the
   first */
void AI_Disband_Extra_Settlers(int player)
{
    int lm;
    int settlers[MAX_LANDMASSES];
    int engineers[MAX_LANDMASSES];
    int unit;

    for (unit = 0; unit < MAX_LANDMASSES; unit++) {
        settlers[unit] = 0;
        engineers[unit] = 0;
    }
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner != player)
            continue;
        if (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) {
            lm = p_landmass_ids[p_units[unit].plane][p_units[unit].y][p_units[unit].x];
            if (lm != 0 && lm < MAX_LANDMASSES) {
                settlers[lm]++;
                if (settlers[lm] > 1)
                    Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
            }
        }
        if (unit_types[p_units[unit].type].construction > 0 && game_turn > 200) {
            lm = p_landmass_ids[p_units[unit].plane][p_units[unit].y][p_units[unit].x];
            if (lm != 0 && lm < MAX_LANDMASSES) {
                engineers[lm]++;
                if (engineers[lm] > 1)
                    Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
            }
        }
    }
}

/* disbands the computer players' units held by MUT_STASIS_NEW / MUT_STASIS whose type resists below 7 */
void AI_Disband_Stasis_Units(void)
{
    int unit;

    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].owner != PLAYER_HUMAN && (p_units[unit].mutation_flags & (MUT_STASIS_NEW | MUT_STASIS)) &&
            (char)unit_types[p_units[unit].type].resist < 7)
            Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
}

/* maps the AI's EMS page and points the AI tables into it, at these paragraph offsets: the landmass tile lists x 0 /
   100, y 200 / 300, next 400 / 600, heads 800 / 825 (per plane); the square stacks' x / y / plane / class / size
   850 .. 890 (10 each); the threat maps 900 / 1500 (600: MAP_SQUARES words); the coast tile lists x 2100 / 2200,
   y 2300 / 2400, next 2500 / 2700, heads 2900 / 3000; the 80 stack unit lists from 0xc80, 2 paragraphs each */
void AI_Map_EMS_Tables(void)
{
    int i;

    EMS_Map_Pages(0, ems_cont_handle);
    p_landmass_tile_x[0] = Seg_To_Far_Ptr(ems_frame_seg);
    p_landmass_tile_x[1] = Seg_To_Far_Ptr(ems_frame_seg + 100);
    p_landmass_tile_y[0] = Seg_To_Far_Ptr(ems_frame_seg + 200);
    p_landmass_tile_y[1] = Seg_To_Far_Ptr(ems_frame_seg + 300);
    p_landmass_tile_next[0] = Seg_To_Far_Ptr(ems_frame_seg + 400);
    p_landmass_tile_next[1] = Seg_To_Far_Ptr(ems_frame_seg + 600);
    p_landmass_tile_head[0] = Seg_To_Far_Ptr(ems_frame_seg + 800);
    p_landmass_tile_head[1] = Seg_To_Far_Ptr(ems_frame_seg + 825);
    p_ai_square_stack_x = Seg_To_Far_Ptr(ems_frame_seg + 850);
    p_ai_square_stack_y = Seg_To_Far_Ptr(ems_frame_seg + 860);
    p_ai_square_stack_plane = Seg_To_Far_Ptr(ems_frame_seg + 870);
    p_ai_square_stack_class = Seg_To_Far_Ptr(ems_frame_seg + 880);
    p_ai_square_stack_size = Seg_To_Far_Ptr(ems_frame_seg + 890);
    p_threat_maps[0] = Seg_To_Far_Ptr(ems_frame_seg + 900);
    p_threat_maps[1] = Seg_To_Far_Ptr(ems_frame_seg + 1500);
    p_coast_tile_x[0] = Seg_To_Far_Ptr(ems_frame_seg + 2100);
    p_coast_tile_x[1] = Seg_To_Far_Ptr(ems_frame_seg + 2200);
    p_coast_tile_y[0] = Seg_To_Far_Ptr(ems_frame_seg + 2300);
    p_coast_tile_y[1] = Seg_To_Far_Ptr(ems_frame_seg + 2400);
    p_coast_tile_next[0] = Seg_To_Far_Ptr(ems_frame_seg + 2500);
    p_coast_tile_next[1] = Seg_To_Far_Ptr(ems_frame_seg + 2700);
    p_coast_tile_head[0] = Seg_To_Far_Ptr(ems_frame_seg + 2900);
    p_coast_tile_head[1] = Seg_To_Far_Ptr(ems_frame_seg + 3000);
    for (i = 0; i < AI_MAX_STACKS; i++)
        p_ai_square_stack_units[i] = Seg_To_Far_Ptr(ems_frame_seg + i * 2 + 3200);
}

/* per plane, a linked list of each landmass's squares (head per landmass, next / x / y per square) */
void AI_Build_Landmass_Tile_Lists(void)
{
    int x;
    int y;
    int k;
    int count[2];
    int lm;
    int last;
    int plane;

    AI_Map_EMS_Tables();
    for (k = 0; k < MAX_LANDMASSES; k++) {
        p_landmass_tile_head[0][k] = -1;
        p_landmass_tile_head[1][k] = -1;
    }
    for (k = 0; k < 1600; k++) {                /* the tile list's capacity per plane */
        p_landmass_tile_next[0][k] = -1;
        p_landmass_tile_next[1][k] = -1;
    }
    count[0] = 0;
    count[1] = 0;
    for (plane = 0; plane < NUM_PLANES; plane++)
        for (y = 0; y < MAP_HEIGHT; y++)
            for (x = 0; x < MAP_WIDTH; x++) {
                lm = p_landmass_ids[plane][y][x];
                if (lm == 0)
                    continue;
                if (p_landmass_tile_head[plane][lm] == -1)
                    p_landmass_tile_head[plane][lm] = count[plane];
                else {
                    k = p_landmass_tile_head[plane][lm];
                    last = k;
                    while (k != -1) {
                        last = k;
                        k = p_landmass_tile_next[plane][k];
                    }
                    p_landmass_tile_next[plane][last] = count[plane];
                }
                p_landmass_tile_x[plane][count[plane]] = x;
                p_landmass_tile_y[plane][count[plane]] = y;
                p_landmass_tile_next[plane][count[plane]] = -1;
                count[plane]++;
            }
    EMS_Map_Default();
}

/* the same lists for the land squares next to the sea */
void AI_Build_Coast_Tile_Lists(void)
{
    int x;
    int y;
    int k;
    int count[2];
    int lm;
    int last;
    int dx;
    int dy;
    int found;
    int plane;

    AI_Map_EMS_Tables();
    for (k = 0; k < MAX_LANDMASSES; k++) {
        p_coast_tile_head[0][k] = -1;
        p_coast_tile_head[1][k] = -1;
    }
    count[0] = 0;
    count[1] = 0;
    for (plane = 0; plane < NUM_PLANES; plane++)
        for (y = 0; y < MAP_HEIGHT; y++)
            for (x = 0; x < MAP_WIDTH; x++) {
                lm = p_landmass_ids[plane][y][x];
                if (lm == 0)
                    continue;
                found = 0;
                for (dx = -1; dx < 2 && found == 0; dx++)   /* no bounds: at the map's edges the neighbours read the
                                                               row before / after (x) or outside the plane (y);
                                                               clarified using ReMoM repo */
                    for (dy = -1; dy < 2 && found == 0; dy++)
                        if (Tile_Is_Sea_NoWrap(x + dx, y + dy, plane) == 1)
                            found = 1;
                if (found != 1)
                    continue;
                if (p_coast_tile_head[plane][lm] == -1)
                    p_coast_tile_head[plane][lm] = count[plane];
                else {
                    k = p_coast_tile_head[plane][lm];
                    last = k;
                    while (k != -1) {
                        last = k;
                        k = p_coast_tile_next[plane][k];
                    }
                    p_coast_tile_next[plane][last] = count[plane];
                }
                p_coast_tile_x[plane][count[plane]] = x;
                p_coast_tile_y[plane][count[plane]] = y;
                p_coast_tile_next[plane][count[plane]] = -1;
                count[plane]++;
            }
    EMS_Map_Default();
}
