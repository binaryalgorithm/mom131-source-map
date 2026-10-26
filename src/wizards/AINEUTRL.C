/* AINEUTRL.C - WIZARDS.EXE overlay code segment f978 (stub 26a5): the neutral player's AI and two wizard-AI planners - neutral
   cities kill their cheapest unit when the garrison outgrows the population, lairs release roaming monster
   groups and neutral cities send raiders (each on a timer kept in the neutral Wizard record), neutral field
   stacks pick a city to attack (or are disbanded), the AI scratch tables are allocated, and a wizard AI picks its
   strategy focus and re-plans its hostility toward the other wizards.
   This module has no data or BSS of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS f978 src/wizards/AINEUTRL.C */

/* the module's functions in their original declaration order */
void AI_Neutral_Trim_Garrison(void);
int AI_Neutral_Lair_Monsters(void);
int AI_Neutral_City_Raiders(void);
void AI_Neutral_Stack_Orders(void);
void AI_Init_Buffers(void);
int AI_Choose_Strategy(int ai);
int AI_Hostility_Replan(int ai);
void AI_Neutral_Stack_Table(int player);

#include "GAME.H"

/* the neutral player's wizard record has no use for these two fields: they are its timers */
#define NEUTRAL_LAIR_TIMER wizards[PLAYER_NEUTRAL].average_unit_cost   /* lair monsters leave their lairs */
#define NEUTRAL_RAID_TIMER wizards[PLAYER_NEUTRAL].casting_cost_initial   /* neutral raids on cities */

extern int personality_hostility_mod[6];        /* DS:1fde  by Wizard.personality: added to hostility chances */
extern int *ai_lm_surplus[2];                   /* DS:90f2 */
extern int *ai_lm_target_value[2];              /* DS:90f6 */
extern int *ai_lm_own_value[2];                 /* DS:90fa */
extern unsigned *ai_lm_enemy_strength[2];       /* DS:90fe */
extern unsigned *ai_lm_own_strength[2];         /* DS:9102 */
extern struct AIEnemyStack *p_ai_enemy_stacks;  /* DS:9106 */
extern struct AIStack *p_ai_own_stacks;             /* DS:9108 */
extern int ai_own_stack_count;                  /* DS:910c */
extern unsigned char *ai_known_spells;          /* DS:911c */
extern unsigned char *p_ai_spell_groups;        /* DS:9120 */
extern int *ai_city_threat;                     /* DS:9122 */
extern int *ai_city_garrison;                   /* DS:9124 */
extern int *ai_city_target_value;               /* DS:9126 */
extern int *ai_city_own_value;                  /* DS:9128 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */

int Random(int n);
int abs(int x);
void EMS_Map_Default(void);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
void Map_Stack_At(int x, int y, int plane, int *count, int *list);
void Map_Units_At(int x, int y, int plane, int *count, int *list);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Unit_Can_Cross_Water(int unit);
int Wizard_Gold_Upkeep(int player);
int Wizard_Mana_Upkeep(int player);
int AI_Adjacent_Tile_Pick(int x, int y, int plane, int *fx, int *fy);
int AI_Roll_Monster_Group(int budget, int race_realm, int *out);
int AI_Stack_Order_Attack_Tile(int stack, int tx, int ty, int player);
int AI_Count_Known_Spells(int player);
int AI_Knows_Spell(int player, int spell);

/* () each neutral city whose units outnumber its population (+2 for a Granary, +2 for a Farmers' Market)
   kills its cheapest unit (UNIT_REMOVE_KILLED) - one per city per turn */
void AI_Neutral_Trim_Garrison(void)
{
    int city;
    int unit;
    int count;
    int cheapest_cost;
    int cheapest;
    int cx;
    int cy;
    int cplane;
    int keep;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == PLAYER_NEUTRAL) {
            cx = p_cities[city].x;
            cy = p_cities[city].y;
            cplane = p_cities[city].plane;
            cheapest_cost = 1000;
            cheapest = -1;
            count = 0;
            for (unit = 0; unit < num_units; unit++) {
                if (p_units[unit].plane == cplane && p_units[unit].x == cx && p_units[unit].y == cy) {
                    count++;
                    if (unit_types[p_units[unit].type].cost < cheapest_cost) {
                        cheapest_cost = unit_types[p_units[unit].type].cost;
                        cheapest = unit;
                    }
                }
            }
            keep = p_cities[city].population;
            if (p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
                keep += 2;
            if (p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_BUILT
                || p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
                keep += 2;
            if (count > keep && cheapest != -1)
                Unit_Kill_Or_Dismiss(cheapest, UNIT_REMOVE_KILLED);
        }
    }
}

/* () the lair-monster timer (NEUTRAL_LAIR_TIMER) grows by Random(difficulty + 1); at
   50 - 5 x difficulty, from turn 50, a random live lair (guardians not of RACE_LIFE_CREATURE) on a landmass that holds a
   wizard's city releases a monster group of (2 rolls) x turn / 5 (halved when an AI wizard's fortress shares the
   landmass and the human's does not) next to it; the lair is re-picked while Random(3) > 1 and it is not on the human fortress's plane
   (BUG #32: the re-pick never resets the lair, so it keeps the first one) */
int AI_Neutral_Lair_Monsters(void)
{
    int i;
    int budget;
    int live_lairs;
    int lair;
    int lx;
    int ly;
    int plane;
    unsigned lair_lm;
    int adj_x;
    int adj_y;
    int tries;
    int pick;
    int race_realm;
    int group_count;
    int ai_fortress_here;                       /* an AI wizard's fortress (players 1 ..) is on this landmass */
    int group[10];

    live_lairs = 0;
    for (i = 0; i < NUM_LAIRS; i++)
        if (p_lairs[i].active == 1 && p_lairs[i].guard1_type != 0 && unit_types[p_lairs[i].guard1_type].race != RACE_LIFE_CREATURE)
            live_lairs++;
    if (live_lairs == 0)
        return 0;
    NEUTRAL_LAIR_TIMER += Random(difficulty + 1);
    if (50 - difficulty * 5 > NEUTRAL_LAIR_TIMER)
        return 0;
    NEUTRAL_LAIR_TIMER = 0;
    if (game_turn < 50)
        return 0;
    tries = 0;
    lair = -1;
    do {
        for (; lair == -1 && tries < 1000; tries++) {
            pick = Random(NUM_LAIRS) - 1;
            if (p_lairs[pick].active == 1 && p_lairs[pick].guard1_type != 0
                && unit_types[p_lairs[pick].guard1_type].race != RACE_LIFE_CREATURE) {
                for (i = 0; i < num_cities && lair == -1; i++) {
                    if (p_cities[i].owner != PLAYER_NEUTRAL && p_cities[i].plane == p_lairs[pick].plane
                        && p_landmass_ids[p_cities[i].plane][p_cities[i].y][p_cities[i].x]
                           == p_landmass_ids[p_lairs[pick].plane][p_lairs[pick].y][p_lairs[pick].x])
                        lair = pick;
                }
            }
        }
    } while (Random(3) > 1 && p_fortresses[PLAYER_HUMAN].plane != p_lairs[lair].plane);   /* no lair found: p_lairs[-1] read */
    if (tries >= 1000)
        return 0;
    lx = p_lairs[lair].x;
    ly = p_lairs[lair].y;
    plane = p_lairs[lair].plane;
    lair_lm = p_landmass_ids[plane][ly][lx];
    ai_fortress_here = 0;
    if (p_landmass_ids[p_fortresses[PLAYER_HUMAN].plane][p_fortresses[PLAYER_HUMAN].y][p_fortresses[PLAYER_HUMAN].x] != lair_lm) {
        for (i = 1; i < num_players; i++)
            if (p_landmass_ids[p_fortresses[i].plane][p_fortresses[i].y][p_fortresses[i].x] == lair_lm)
                ai_fortress_here = 1;
    }
    race_realm = -1;
    if (p_lairs[lair].guard1_count > 0) {
        race_realm = unit_types[p_lairs[lair].guard1_type].race;
        if (race_realm < RACE_ARCANE_CREATURE)   /* normal-race guards: the raiders are rolled from the Death creatures */
            race_realm = RACE_DEATH_CREATURE;
    }
    if (race_realm == -1)
        return 0;
    if (AI_Adjacent_Tile_Pick(lx, ly, plane, &adj_x, &adj_y) == 0)
        return 0;
    budget = (Random(difficulty + 1) + Random(difficulty + 1)) * game_turn / 5;
    if (ai_fortress_here == 1)
        budget = budget / 2;
    group_count = AI_Roll_Monster_Group(budget, race_realm, group);
    for (i = 0; i < group_count; i++)
        Unit_Create(group[i], PLAYER_NEUTRAL, adj_x, adj_y, plane, UNIT_SRC_NONE);
}

/* () the raid timer (NEUTRAL_RAID_TIMER) grows by Random(difficulty + 1); above 29 a random neutral
   city on a landmass that holds a wizard's city sends garrison x difficulty / 6 raiders (x 2/3 when a rival
   fortress shares the landmass, / 2 on Myrror before turn 200, at least 1) - NEW units of random garrison
   types (only types > 40: BUG #283; no transports, no settlers) next to the city, with experience by turn -
   and created / 3 garrison units are disbanded; no raid in 1000 picks adds 15 to the lair-monster timer */
int AI_Neutral_City_Raiders(void)
{
    int j;
    int k;
    int x;
    int y;
    int plane;
    int unused_a;                               /* set to 0, never read */
    int adj_x;
    int adj_y;
    int level_arg;                              /* Unit_Create's src: < -1 = starting level abs(src) - 1;
                                                   -1 (UNIT_SRC_NONE) = none */
    unsigned city_lm;
    int garrison;
    int raiders;
    int any_neutral_city;
    int done;
    int tries;
    int city;
    int has_target;
    int unit_type;
    int created;
    int ai_fortress_here;                       /* an AI wizard's fortress (players 1 ..) is on this landmass */
    int garrison_units[MAX_STACK];

    any_neutral_city = 0;
    for (k = 0; k < num_cities; k++)
        if (p_cities[k].owner == PLAYER_NEUTRAL)
            any_neutral_city = 1;
    if (any_neutral_city == 0)
        return 0;
    NEUTRAL_RAID_TIMER += Random(difficulty + 1);
    if (NEUTRAL_RAID_TIMER < 30)
        return 0;
    NEUTRAL_RAID_TIMER = 0;
    unused_a = 0;
    done = 0;
    tries = 0;
    for (tries = 0; tries < 1000 && done == 0; tries++) {
        city = Random(num_cities) - 1;
        if (p_cities[city].owner == PLAYER_NEUTRAL) {
            x = p_cities[city].x;
            y = p_cities[city].y;
            plane = p_cities[city].plane;
            city_lm = p_landmass_ids[plane][y][x];
            ai_fortress_here = 0;
            for (k = 1; k < num_players; k++)
                if (p_landmass_ids[p_fortresses[k].plane][p_fortresses[k].y][p_fortresses[k].x] == city_lm)
                    ai_fortress_here = 1;
            has_target = 0;
            for (j = 0; j < num_cities && has_target == 0; j++)
                if (p_cities[j].owner != PLAYER_NEUTRAL
                    && p_landmass_ids[p_cities[j].plane][p_cities[j].y][p_cities[j].x] == city_lm)
                    has_target = 1;
            if (has_target == 1 && AI_Adjacent_Tile_Pick(x, y, plane, &adj_x, &adj_y) == 1) {
                Map_Stack_At(x, y, plane, &garrison, garrison_units);
                if (garrison > 0) {
                    raiders = garrison * difficulty / 6;
                    if (ai_fortress_here == 1)
                        raiders = raiders * 2 / 3;
                    if (plane == PLANE_MYRROR && game_turn < 200)
                        raiders = raiders / 2;
                    if (raiders < 1)
                        raiders = 1;
                    if (game_turn > 250)
                        level_arg = -4;
                    else if (game_turn > 120)
                        level_arg = -3;
                    else if (game_turn > 40)
                        level_arg = -2;
                    else
                        level_arg = -1;
                    created = 0;
                    for (j = 0; j < raiders; j++) {
                        unit_type = p_units[garrison_units[Random(garrison) - 1]].type;
                        if (unit_type > UNIT_BARBARIAN_SWORDSMEN && unit_types[unit_type].transport_capacity == 0
                            && (unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) == 0) {
                            Unit_Create(unit_type, PLAYER_NEUTRAL, adj_x, adj_y, plane, level_arg);
                            created++;
                        }
                    }
                    for (j = 0; j < created / 3; j++)
                        Unit_Kill_Or_Dismiss(garrison_units[j], UNIT_REMOVE_KILLED);
                    done = 1;
                }
            }
        }
    }
    if (done == 0)
        NEUTRAL_LAIR_TIMER += 15;
}

/* () each neutral field stack (AI_Neutral_Stack_Table) next to a wizard's city weighs it against its current
   target - score 10 - units there - distance, +5 at difficulty > 2 and +5 more above 3 for a human city - and
   attacks the better; a stack not attacking scores every wizard city on its landmass (anywhere on the plane when
   it flies or crosses water; at most 30) the same way and attacks the best; with none, the stack is disbanded */
void AI_Neutral_Stack_Orders(void)
{
    int j;
    int city;
    int sx;
    int sy;
    unsigned stack_lm;
    int plane;
    int candidates;
    int adj_city;
    int best_score;
    int chosen;                                 /* set, never read */
    int stack;
    int target;
    int score;
    int unit_count;
    int dest_x;
    int dest_y;
    int units[18];

    EMS_Map_Default();
    AI_Neutral_Stack_Table(PLAYER_NEUTRAL);
    for (stack = 0; stack < ai_own_stack_count; stack++) {
        sx = p_ai_own_stacks[stack].x;
        sy = p_ai_own_stacks[stack].y;
        plane = p_ai_own_stacks[stack].plane;
        stack_lm = p_landmass_ids[plane][sy][sx];
        adj_city = -1;
        for (j = 0; j < num_cities && adj_city == -1; j++) {
            if (p_cities[j].owner != PLAYER_NEUTRAL && p_cities[j].plane == plane && abs(sx - p_cities[j].x) < 2
                && abs(sy - p_cities[j].y) < 2)
                adj_city = j;
        }
        if (adj_city > -1 && p_ai_own_stacks[stack].status == ORDER_GOTO) {
            Map_Units_At(sx, sy, plane, &unit_count, units);
            dest_x = p_units[units[0]].dest_x;
            dest_y = p_units[units[0]].dest_y;
            target = -1;
            for (j = 0; j < num_cities && target == -1; j++)
                if (p_cities[j].x == dest_x && p_cities[j].y == dest_y && p_cities[j].plane == plane)
                    target = j;
            if (target != adj_city) {
                Map_Units_At(p_cities[adj_city].x, p_cities[adj_city].y, p_cities[adj_city].plane, &unit_count, units);
                best_score = 10 - unit_count - Map_Distance(p_cities[adj_city].x, p_cities[adj_city].y, sx, sy, MAP_WIDTH);
                if (difficulty > DIFFICULTY_NORMAL && p_cities[adj_city].owner == PLAYER_HUMAN)
                    best_score += 5;
                if (difficulty > DIFFICULTY_HARD && p_cities[adj_city].owner == PLAYER_HUMAN)
                    best_score += 5;
                Map_Units_At(dest_x, dest_y, plane, &unit_count, units);
                score = 10 - unit_count - Map_Distance(dest_x, dest_y, sx, sy, MAP_WIDTH);
                if (difficulty > DIFFICULTY_NORMAL && p_cities[target].owner == PLAYER_HUMAN)
                    score += 5;
                if (difficulty > DIFFICULTY_HARD && p_cities[target].owner == PLAYER_HUMAN)
                    score += 5;
                if (best_score > score) {
                    AI_Stack_Order_Attack_Tile(stack, p_cities[adj_city].x, p_cities[adj_city].y, PLAYER_NEUTRAL);
                    p_ai_own_stacks[stack].status = ORDER_GOTO;
                }
            }
        }
        if (p_ai_own_stacks[stack].status != ORDER_GOTO) {
            best_score = -1000;
            target = -1;
            candidates = 0;
            for (city = 0; city < num_cities; city++) {
                if (p_cities[city].owner != PLAYER_NEUTRAL && candidates < 30 && p_cities[city].plane == plane
                    && ((p_ai_own_stacks[stack].flags & AISTACK_CROSSES_WATER) != 0
                        || p_landmass_ids[p_cities[city].plane][p_cities[city].y][p_cities[city].x] == stack_lm)) {
                    Map_Units_At(p_cities[city].x, p_cities[city].y, p_cities[city].plane, &unit_count, units);
                    score = 10 - unit_count - Map_Distance(p_cities[city].x, p_cities[city].y, sx, sy, MAP_WIDTH);
                    if (difficulty > DIFFICULTY_NORMAL && p_cities[city].owner == PLAYER_HUMAN)
                        score += 5;
                    if (difficulty > DIFFICULTY_HARD && p_cities[city].owner == PLAYER_HUMAN)
                        score += 5;
                    if (score > best_score) {
                        target = city;
                        best_score = score;
                    }
                    candidates++;
                }
            }
            if (candidates > 0) {
                chosen = target;
                AI_Stack_Order_Attack_Tile(stack, p_cities[target].x, p_cities[target].y, PLAYER_NEUTRAL);
            } else {
                for (j = 0; j < num_units; j++)
                    if (p_units[j].x == sx && p_units[j].y == sy && p_units[j].plane == plane)
                        Unit_Kill_Or_Dismiss(j, UNIT_REMOVE_KILLED);
            }
        }
    }
}

/* () allocates the AI scratch tables in the near heap; the landmass surplus rows alias the enemy-strength rows */
void AI_Init_Buffers(void)
{
    ai_city_own_value = (int *)Near_Alloc_Reset(MAX_CITIES * sizeof(int));
    ai_city_target_value = (int *)Near_Alloc(MAX_CITIES * sizeof(int));
    ai_city_garrison = (int *)Near_Alloc(MAX_CITIES * sizeof(int));
    ai_city_threat = (int *)Near_Alloc(MAX_CITIES * sizeof(int));
    p_ai_own_stacks = (struct AIStack *)Near_Alloc(AI_MAX_STACKS * sizeof(struct AIStack));
    p_ai_enemy_stacks = (struct AIEnemyStack *)Near_Alloc(AI_MAX_ENEMY_STACKS * sizeof(struct AIEnemyStack));
    p_ai_spell_groups = Near_Alloc(92);         /* AI groups 0 .. AIG_LAST (90), and one more */
    ai_known_spells = Near_Alloc(50);           /* BUG #417: 50 spells, filled with no bound test (AI_Count_Known_Spells) */
    ai_lm_own_strength[0] = (unsigned *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_own_strength[1] = (unsigned *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_enemy_strength[0] = (unsigned *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_enemy_strength[1] = (unsigned *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_own_value[0] = (int *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_own_value[1] = (int *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_target_value[0] = (int *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_target_value[1] = (int *)Near_Alloc(MAX_LANDMASSES * sizeof(int));
    ai_lm_surplus[0] = (int *)ai_lm_enemy_strength[0];
    ai_lm_surplus[1] = (int *)ai_lm_enemy_strength[1];
}

/* (ai) every Random(10) + 15 turns: the strategy focus - AI_FOCUS_ECONOMY when its upkeep (gold + 2 x mana) is
   below the human's, AI_FOCUS_SKILL when its casting skill is under 1.5 x the human's or 20, AI_FOCUS_SPELLS with
   few spells, AI_FOCUS_WAR when at HOSTILITY_WAR or worse to anyone, AI_FOCUS_STOP_MASTERY when the human casts the Spell of Mastery - and also when the AI, hostile to the human, is
   itself casting something at least as costly as Cruel Unminding (no Mastery involved) - when it knows Spell Blast, Drain Power or Cruel Unminding.
   A second focus (from few spells, the own city count, the lower upkeep and the gold) is computed and never used */
int AI_Choose_Strategy(int ai)
{
    int i;
    int at_war;
    int focus;
    int unused_focus;                           /* computed, never read */
    int lower_upkeep;
    int own_cities;
    int few_spells;

    wizards[ai].ai_strategy_timer--;
    if (wizards[ai].ai_strategy_timer > 0)
        return 0;
    wizards[ai].ai_strategy_timer = Random(10) + 15;
    at_war = 0;
    for (i = 0; i < num_players; i++)
        if (wizards[ai].hostility[i] >= HOSTILITY_WAR)
            at_war = 1;
    if (Wizard_Gold_Upkeep(ai) + Wizard_Mana_Upkeep(ai) * 2 < Wizard_Gold_Upkeep(0) + Wizard_Mana_Upkeep(0) * 2)
        lower_upkeep = 1;
    else
        lower_upkeep = 0;
    own_cities = 0;
    for (i = 0; i < num_cities; i++)
        if (p_cities[i].owner == ai)
            own_cities++;
    if (AI_Count_Known_Spells(ai) < 20 || AI_Count_Known_Spells(ai) * 3 / 4 > AI_Count_Known_Spells(ai))   /* the second test is never true */
        few_spells = 1;
    else
        few_spells = 0;
    unused_focus = 0;
    if (few_spells == 1)
        unused_focus = 5;
    if (own_cities < 4)
        unused_focus = 4;
    if (own_cities < 10)
        unused_focus = 3;
    if (lower_upkeep == 1)
        unused_focus = 1;
    if (Wizard_Gold_Upkeep(ai) * 2 > wizards[ai].gold)
        unused_focus = 2;
    focus = AI_FOCUS_NONE;
    if (lower_upkeep == 1)
        focus = AI_FOCUS_ECONOMY;
    if (Wizard_Casting_Skill(ai) < Wizard_Casting_Skill(0) * 3 / 2 || Wizard_Casting_Skill(ai) < 20)
        focus = AI_FOCUS_SKILL;
    if (few_spells == 1)
        focus = AI_FOCUS_SPELLS;
    if (at_war == 1)
        focus = AI_FOCUS_WAR;
    if (AI_Knows_Spell(ai, SPELL_SPELL_BLAST) == 1 || AI_Knows_Spell(ai, SPELL_DRAIN_POWER) == 1
        || AI_Knows_Spell(ai, SPELL_CRUEL_UNMINDING) == 1) {
        if (wizards[PLAYER_HUMAN].casting_spell == SPELL_SPELL_OF_MASTERY)
            focus = AI_FOCUS_STOP_MASTERY;
        if (wizards[ai].hostility[PLAYER_HUMAN] >= HOSTILITY_HOSTILE
            && p_spell_data[wizards[ai].casting_spell].base_cost >= p_spell_data[SPELL_CRUEL_UNMINDING].base_cost)
            focus = AI_FOCUS_STOP_MASTERY;
    }
    wizards[ai].ai_focus = focus;
}

/* (ai) from turn 100, every Random(10) + 15 turns: HOSTILITY_WAR to the neutral player; toward each contacted
   wizard HOSTILITY_NONE, or HOSTILITY_WAR at war (TREATY_WAR only: Final War goes to the
   relationship-chance branch below; or while casting Fire Elemental: BUG #154, never
   true - it is combat-only) raised to HOSTILITY_DESTROY at 40% (Maniacal), 20% (Ruthless), 10% (Chaotic,
   Lawful, Peaceful), never for Aggressive; without a treaty
   or peace turns, or against a Spell of Mastery caster, a chance from the relationship, the personality table
   (Lawful with a negative bias subtracts the Lawful entry) and the bias (+50 against Mastery) gives HOSTILITY_HOSTILE, and two
   more rolls HOSTILITY_WAR
   Numbered: BUG #253, #305 (BUGS_AND_QUIRKS.txt) */
int AI_Hostility_Replan(int ai)
{
    int player;
    int chance;

    if (game_turn < 100)
        return 0;
    wizards[ai].hostility_replan_timer--;
    if (wizards[ai].hostility_replan_timer > 0)
        return 0;
    wizards[ai].hostility_replan_timer = Random(10) + 15;
    wizards[ai].hostility[PLAYER_NEUTRAL] = HOSTILITY_WAR;
    for (player = 0; player < num_players; player++) {
        wizards[ai].hostility[player] = HOSTILITY_NONE;
        if (player == ai)
            continue;
        if (wizards[ai].contacted[player] != 1)
            continue;
        if (wizards[ai].diplomacy_status[player] == TREATY_WAR || wizards[ai].casting_spell == SPELL_FIRE_ELEMENTAL) {
            wizards[ai].hostility[player] = HOSTILITY_WAR;
            if (wizards[ai].personality == PERSONALITY_MANIACAL) {
                if (Random(10) <= 4)
                    wizards[ai].hostility[player] = HOSTILITY_DESTROY;
            } else if (wizards[ai].personality == PERSONALITY_RUTHLESS) {
                if (Random(10) <= 2)
                    wizards[ai].hostility[player] = HOSTILITY_DESTROY;
            } else if (wizards[ai].personality >= PERSONALITY_CHAOTIC && Random(10) == 1)
                wizards[ai].hostility[player] = HOSTILITY_DESTROY;
        } else if ((wizards[ai].diplomacy_status[player] != TREATY_ALLIANCE && wizards[ai].diplomacy_status[player] != TREATY_WIZARD_PACT
                    && wizards[ai].peace_turns[player] == 0)
                   || wizards[player].casting_spell == SPELL_SPELL_OF_MASTERY) {
            chance = wizards[ai].relationship[player] < 0 ? -wizards[ai].relationship[player] / 2 + 50
                                                          : (100 - wizards[ai].relationship[player]) / 4;
            if (wizards[ai].personality == PERSONALITY_LAWFUL && wizards[ai].relation_bias[player] < 0)
                chance -= personality_hostility_mod[PERSONALITY_LAWFUL];
            else
                chance += personality_hostility_mod[wizards[ai].personality];
            chance += wizards[ai].relation_bias[player];
            if (wizards[player].casting_spell == SPELL_SPELL_OF_MASTERY)
                chance += 50;
            if (Random(100) < chance) {
                wizards[ai].hostility[player] = HOSTILITY_HOSTILE;
                if (Random(100) < chance && Random(100) < chance)
                    wizards[ai].hostility[player] = HOSTILITY_WAR;
            }
        }
    }
}

/* (player: always 5, unused) rebuilds p_ai_own_stacks with the neutral stacks outside cities (at most 80): tile,
   unit status, AISTACK_CROSSES_WATER when a unit flies or crosses water (a stack seen again is cleared to 0 by a unit that
   does neither) */
void AI_Neutral_Stack_Table(int player)
{
    int i;
    int unit;
    int found;
    int in_city;
    int x;
    int y;
    int plane;

    ai_own_stack_count = 0;
    for (unit = 0; unit < num_units && ai_own_stack_count < AI_MAX_STACKS; unit++) {
        if (p_units[unit].owner == PLAYER_NEUTRAL) {
            x = p_units[unit].x;
            y = p_units[unit].y;
            plane = p_units[unit].plane;
            found = 0;
            for (i = 0; i < ai_own_stack_count; i++) {
                if (p_ai_own_stacks[i].x == x && p_ai_own_stacks[i].y == y && p_ai_own_stacks[i].plane == plane) {
                    found = 1;
                    if (Unit_Can_Fly(unit) == 0 && Unit_Can_Cross_Water(unit) == 0)
                        p_ai_own_stacks[i].flags = 0;
                }
            }
            if (found == 0) {
                in_city = 0;
                for (i = 0; i < num_cities; i++)
                    if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane)
                        in_city = 1;
                if (in_city == 0) {
                    p_ai_own_stacks[ai_own_stack_count].x = x;
                    p_ai_own_stacks[ai_own_stack_count].y = y;
                    p_ai_own_stacks[ai_own_stack_count].plane = plane;
                    p_ai_own_stacks[ai_own_stack_count].status = p_units[unit].order;
                    if (Unit_Can_Fly(unit) != 0 || Unit_Can_Cross_Water(unit) != 0)
                        p_ai_own_stacks[ai_own_stack_count].flags = AISTACK_CROSSES_WATER;
                    else
                        p_ai_own_stacks[ai_own_stack_count].flags = 0;
                    ai_own_stack_count++;
                }
            }
        }
    }
}
