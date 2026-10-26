/* AISPELL.C - WIZARDS.EXE overlay code segment e318 (stub 2656): the AI's spell choices - the research pick, the
   overland spell for the turn (category weights, summons, unit / city / global enchantments, attack spells, the
   anti-realm globals, disenchants), and the targets of each overland spell (cities, map squares, units, wizards,
   nodes), plus the Summoning Circle placement.
   Data DS:70f8..7113 (the one string literal); BSS DS:d386..d3eb (ai_spell_pick_weights,
   ai_foreign_node_on_circle_plane).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS e318 src/wizards/AISPELL.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Choose_Research(int player);
void AI_Choose_Overland_Spell(int player);
int AI_Pick_Spell_Category(int player);
void AI_Build_Known_Spell_Groups(int player);
int AI_Pick_Summon(int player);
int AI_Pick_Unit_Enchant(int player);
int AI_Pick_Anti_Realm_Global(int player);
int AI_Pick_Global_Enchant(int player);
int AI_Pick_Attack_Spell(int player);
int AI_Pick_City_Spell(int player);
int AI_Life_Casting_Penalized(void);
int AI_Chaos_Casting_Penalized(void);
int AI_Death_Casting_Penalized(void);
int AI_Nature_Casting_Penalized(void);
int AI_Pick_Disenchant(int player);
int AI_Pick_Disjunction(int player);
int AI_Target_Wall_Of_Stone(int caster, int *city);
int AI_Target_Transmute(int caster, int *x, int *y, int *plane);
int AI_Target_Change_Terrain(int caster, int *x, int *y, int *plane);
int AI_Target_Flying_Fortress(int caster, int *city);
int AI_Target_Move_Fortress(int caster, int *city);
int AI_Target_Earth_Gate(int caster, int *city);
int AI_Target_Gaias_Blessing(int caster, int *city);
int AI_Target_Wall_Of_Fire(int caster, int *city);
int AI_Target_Wall_Of_Darkness(int caster, int *city);
int AI_Target_Heavenly_Light(int caster, int *city);
int AI_Target_Altar_Of_Battle(int caster, int *city);
int AI_Target_Stream_Of_Life(int caster, int *city);
int AI_Target_Inspirations(int caster, int *city);
int AI_Target_Prosperity(int caster, int *city);
int AI_Target_Astral_Gate(int caster, int *city);
int AI_Target_Dark_Rituals(int caster, int *city);
int AI_Target_Cloud_Of_Shadow(int caster, int *city);
int AI_Target_Spell_Ward(int caster, int *city, int *realm);
int AI_Target_Consecration(int caster, int *city);
void Debug_List_Known_Spells(int player);
int AI_Target_Summoning_Circle(int caster, int *city);
int AI_Fatal_No_Spell_Target(int spell);
void AI_Drop_Bad_Globals(int player);
int AI_Pick_City_Spell_Target(int target_kind, int *city, int spell, int caster);
int AI_Has_Seen_Any_City(int player);
int AI_Pick_Unit_Enchant_Target(int target_kind, int *unit, int spell, int caster);
int AI_Pick_Unit_For_Enchant(int caster, long enchants, int *unit, int normal_only, int summons_only, int no_basic,
                             int movement_spell, int unused);
int AI_Target_Warp_Node(int *node, int spell);
int AI_Pick_Global_Enchant_Target(int *wizard, int *slot, int spell, int caster);
int AI_Overland_Spell_Target(int target_kind, int *x, int *y, int *plane, int spell, int caster);
int AI_Target_Area_Damage(int caster, int *x, int *y, int *plane);
int AI_Target_Floating_Island(int caster, int *x, int *y, int *plane);
int AI_Target_Enchant_Road(int caster, int *x, int *y, int *plane);
int AI_Target_Disenchant(int caster, int *x, int *y, int *plane);
int AI_Target_Corruption_Volcano(int caster, int *x, int *y, int *plane, int spell);
int AI_Target_Raise_Volcano_Old(int caster, int *x, int *y, int *plane);
int AI_Target_Plane_Shift(int stack, int *x, int *y, int *plane);
int AI_Target_Natures_Cures(int caster, int *x, int *y, int *plane);
int AI_Pick_Wizard_Target(int *target, int spell, int caster);
int AI_Target_Spell_Ward_Call(int *city, int *realm, int caster);
int AI_More_Myrror_Cities_Than_Human(int player);
int AI_Tile_Floating_Island_OK(int x, int y, int plane);
int AI_Circle_At_Fortress(int player);

#include <string.h>
#include "GAME.H"

/* BSS DS:d386..d3eb, highest address first */
int ai_foreign_node_on_circle_plane;                    /* DS:d3ea  the foreign-node flag of AI_Pick_Spell_Category */
                                                /* set only when the player has no meld unit on the circle's plane */
int ai_spell_pick_weights[50];                    /* DS:d386 */

extern int personality_hostility_mod[6];        /* DS:1fde */
extern int *p_ai_circle_timer;                  /* DS:8f98 */
extern int ai_own_transport_count;             /* DS:9094 */
extern int *ai_city_garrison;                   /* DS:9124 */
extern int *ai_city_own_value;                  /* DS:9128 */
extern int *ai_city_target_value;               /* DS:9126 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb8 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern struct AIEnemyStack *p_ai_enemy_stacks;  /* DS:9106 */
extern struct AIStack *p_ai_own_stacks;             /* DS:9108 */
extern int ai_enemy_stack_count;                /* DS:910a */
extern int ai_own_stack_count;                  /* DS:910c */
extern int ai_hostile_wizards[6];               /* DS:910e */
extern int ai_hostile_count;                    /* DS:911a */
extern unsigned char *ai_known_spells;          /* DS:911c */
extern int ai_known_spell_count;                /* DS:911e */
extern unsigned char *p_ai_spell_groups;        /* DS:9120 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern struct Fortress far *p_fortresses;       /* DS:9cc8 */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */

extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */

int AI_Magic_Allocation(int player);
void Wizard_Magic_Income(int *mana, int *research, int *skill, int player);
int Research_Bonus_Pct(int player, int spell);
int Random_Weighted_Pick32(long *weights, int count);
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana);
int Wizard_Begin_Casting(int player, int spell, int page_slot);
int Wizard_Hero_Count(int player);
int Wizard_Dead_Hero_Count(int player);
int Random_Weighted_Pick(int *weights, int count);
int Random(int n);                             /* -> 1 .. n */
int Tile_Is_Desert(int x, int y, int plane);
int Tile_Is_Swamp(int x, int y, int plane);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Debug_Print(int col, int row, char *text);
void Screen_Copy_Front(void);
int UI_Wait_Any_Input(void);
int Bit_Test(int bit, char far *map);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int Tile_Is_Land(int x, int y, int plane);
int Tile_Is_Volcano(int x, int y, int plane);
int Tile_Is_Corrupted(int x, int y, int plane);
int Tile_Is_Mountain(int x, int y, int plane);
int Tile_Is_Hills(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Node(int x, int y, int plane);
int Tile_Is_Water(int x, int y, int plane);
int Tile_Is_Sea(int x, int y, int plane);

/* (player) the research pick among the 8 research candidates, a weighted random choice: w = the research cost x 100
   / (100 + the realm's research bonus) (Spell of Mastery: the mastery cost); x 4 for an AI group already known; for
   unknown groups / 10 (groups 16 / 35 / 59 / 81) and / 5 (a wider list); militarists / 2 for those four groups;
   unknown 16 / 10, 35 / 7, 59 / 5, 81 / 3; Consecration while the HUMAN's primary realm is Death or Chaos, and
   Spell Ward / 2; Disenchant Area, Disjunction / 2; Disenchant True, Disjunction True, Summon Hero / 5; Summoning
   Circle x 2; a known group x 3 again; research income below the effective cost / 20: x 2, below it / 50: x 5 (the
   unmodified eff_cost, not the running w); at least 1 (when the
   cost is positive); AI group 0: never. The final weight is 52 - 50 x w / the largest w (at least 1) - a SMALL w is
   likely, so the dividing factors favour a spell and the multiplying ones disfavour it. No candidates: research
   none, research 0 % / mana 50 % / skill 50 %, returns 0; else no value
   Numbered: BUG #139 (BUGS_AND_QUIRKS.txt) */
int AI_Choose_Research(int player)
{
    int *cands;
    int *books;
    int pick;
    int base_cost;
    int cand_count;
    int bonus_pct;
    int slot;
    unsigned research;
    int mana;
    int skill;
    int ai_group;
    long total;                                 /* summed, never used */
    long max;
    unsigned eff_cost[8];
    long weight[8];
    int group_known[92];
    int i;

    cands = wizards[player].research_candidates;
    books = wizards[player].spellbooks;
    cand_count = 0;
    for (i = 0; i < MAX_RESEARCH_CANDIDATES; i++)
        if (cands[i] != 0)
            cand_count++;
    if (cand_count == 0) {
        wizards[player].research_spell = 0;
        wizards[player].research_percentage = 0;
        wizards[player].mana_percentage = 50;
        wizards[player].skill_percentage = 50;
        return 0;
    }
    wizards[player].research_spell = SPELL_EARTH_TO_MUD;   /* a stand-in: Wizard_Magic_Income's research
                                                              estimate reads research_spell; the pick replaces it */
    wizards[player].ai_magic_timer = 0;
    AI_Magic_Allocation(player);
    Wizard_Magic_Income(&mana, &research, &skill, player);
    for (i = 0; i < cand_count; i++) {
        if (cands[i] == SPELL_SPELL_OF_MASTERY)
            eff_cost[i] = wizards[player].mastery_research_cost;
        else
            eff_cost[i] = p_spell_data[cands[i]].research_cost;
        bonus_pct = Research_Bonus_Pct(player, p_spell_data[cands[i]].realm);   /* BUG #139: a realm where it takes
                                                   a spell - it reads p_spell_data[realm].realm, spell 0..5's */
        eff_cost[i] = (long)eff_cost[i] * 100 / (bonus_pct + 100L);
    }
    for (i = 0; i <= AIG_LAST; i++)
        group_known[i] = 0;
    for (i = 0; i < NUM_REALMS; i++)
        for (slot = 0; slot < SPELLS_PER_REALM; slot++)
            if (wizards[player].spell_library[i][slot] == SPELLSTATE_KNOWN)
                group_known[p_spell_data[i * SPELLS_PER_REALM + slot + 1].ai_group] = 1;
    max = 0;
    for (i = 0; i < cand_count; i++) {
        ai_group = p_spell_data[cands[i]].ai_group;
        weight[i] = eff_cost[i];
        base_cost = weight[i];
        if (group_known[ai_group] == 1)
            weight[i] = weight[i] << 2;
        else {
            if (group_known[ai_group] == 0 && (ai_group == AIG_COMBAT_SUMMON_1 || ai_group == AIG_COMBAT_SUMMON_2 || ai_group == AIG_COMBAT_SUMMON_3 || ai_group == AIG_COMBAT_SUMMON_4))
                weight[i] = weight[i] / 10;
            if (group_known[ai_group] == 0 &&
                (ai_group == AIG_DAMAGE_1 || ai_group == AIG_DAMAGE_2 || ai_group == AIG_DAMAGE_3 || ai_group == AIG_DAMAGE_4 || ai_group == AIG_BATTLEFIELD_1 ||
                 ai_group == AIG_BATTLEFIELD_2 || ai_group == AIG_BATTLEFIELD_3 || ai_group == AIG_BATTLEFIELD_4 || ai_group == AIG_COMBAT_AID_1 || ai_group == AIG_COMBAT_AID_2 ||
                 ai_group == AIG_COMBAT_AID_3 || ai_group == AIG_COMBAT_AID_4 || ai_group == AIG_COMBAT_SUMMON_1 || ai_group == AIG_COMBAT_SUMMON_2 || ai_group == AIG_COMBAT_SUMMON_3 ||
                 ai_group == AIG_COMBAT_SUMMON_4))
                weight[i] = weight[i] / 5;
        }
        if (wizards[player].objective == OBJECTIVE_MILITARIST &&
            (ai_group == AIG_COMBAT_SUMMON_1 || ai_group == AIG_COMBAT_SUMMON_2 || ai_group == AIG_COMBAT_SUMMON_3 || ai_group == AIG_COMBAT_SUMMON_4))
            weight[i] = weight[i] / 2;
        if (group_known[ai_group] == 0 && ai_group == AIG_COMBAT_SUMMON_1)
            weight[i] = weight[i] / 10;
        if (group_known[ai_group] == 0 && ai_group == AIG_COMBAT_SUMMON_2)
            weight[i] = weight[i] / 7;
        if (group_known[ai_group] == 0 && ai_group == AIG_COMBAT_SUMMON_3)
            weight[i] = weight[i] / 5;
        if (group_known[ai_group] == 0 && ai_group == AIG_COMBAT_SUMMON_4)
            weight[i] = weight[i] / 3;
        if (((wizards[PLAYER_HUMAN].primary_realm == REALM_DEATH || wizards[PLAYER_HUMAN].primary_realm == REALM_CHAOS) &&
             cands[i] == SPELL_CONSECRATION) || cands[i] == SPELL_SPELL_WARD)
            weight[i] = weight[i] / 2;
        if (cands[i] == SPELL_DISENCHANT_AREA)
            weight[i] = weight[i] / 2;
        if (cands[i] == SPELL_DISJUNCTION)
            weight[i] = weight[i] / 2;
        if (cands[i] == SPELL_DISENCHANT_TRUE)
            weight[i] = weight[i] / 5;
        if (cands[i] == SPELL_DISJUNCTION_TRUE)
            weight[i] = weight[i] / 5;
        if (cands[i] == SPELL_SUMMON_HERO)
            weight[i] = weight[i] / 5;
        if (cands[i] == SPELL_SUMMONING_CIRCLE)
            weight[i] = weight[i] << 1;
        if (group_known[p_spell_data[cands[i]].ai_group] == 1)
            weight[i] = weight[i] * 3;
        if (eff_cost[i] / 20 > research)
            weight[i] *= 2;
        if (eff_cost[i] / 50 > research)
            weight[i] = weight[i] * 5;
        if (weight[i] < 1 && base_cost > 0)
            weight[i] = 1;
        if (p_spell_data[cands[i]].ai_group == AIG_NONE)
            weight[i] = 0;
        if (weight[i] > max)
            max = weight[i];
    }
    total = 0;
    for (i = 0; i < cand_count; i++) {
        if (weight[i] != 0) {
            weight[i] = 52 - weight[i] * 50 / max;
            if (weight[i] < 0)
                weight[i] = 1;
        }
        total += weight[i];
    }
    pick = Random_Weighted_Pick32(weight, cand_count);
    wizards[player].research_spell = cands[pick];
}

/* (player) the AI's overland spell for the turn: the known spell groups, a category (AI_Pick_Spell_Category), the
   category's spell; none when the spell's base cost / 50 exceeds the mana reserve / 10 + the mana income; then
   Wizard_Begin_Casting (with an uninitialised page slot)
   Numbered: BUG #240 (BUGS_AND_QUIRKS.txt) */
void AI_Choose_Overland_Spell(int player)
{
    int category;
    int page_slot;
    int budget;
    char *known;                                /* set, never read */
    int gold;
    int food;
    int mana;
    int spell;

    known = wizards[player].spell_library[0] - 1;
    AI_Build_Known_Spell_Groups(player);
    category = AI_Pick_Spell_Category(player);
    switch (category) {
    case AI_CAT_NONE:
        spell = SPELL_NONE;
        break;
    case AI_CAT_SUMMON:
        spell = AI_Pick_Summon(player);
        break;
    case AI_CAT_UNIT_ENCHANT:
        spell = AI_Pick_Unit_Enchant(player);
        break;
    case AI_CAT_CITY:
        spell = AI_Pick_City_Spell(player);
        break;
    case AI_CAT_DISENCHANT:
        spell = AI_Pick_Disenchant(player);
        break;
    case AI_CAT_DISJUNCTION:
        spell = AI_Pick_Disjunction(player);
        break;
    case AI_CAT_SUMMONING_CIRCLE:
        spell = SPELL_SUMMONING_CIRCLE;
        break;
    case AI_CAT_ATTACK:
        spell = AI_Pick_Attack_Spell(player);
        break;
    case AI_CAT_ANTI_REALM:
        spell = AI_Pick_Anti_Realm_Global(player);
        break;
    case AI_CAT_GLOBAL:
        spell = AI_Pick_Global_Enchant(player);
        break;
    case AI_CAT_MASTERY:
        spell = SPELL_SPELL_OF_MASTERY;
        break;
    }
    Wizard_Income_Summary(player, &gold, &food, &mana);
    budget = wizards[player].mana_reserve / 10 + mana;
    if (p_spell_data[spell].base_cost / 50 > budget)
        spell = SPELL_NONE;
    Wizard_Begin_Casting(player, spell, page_slot);
}

/* (player) the spell category, a weighted random pick of 11: 0 none (always 1: the 50 - turn set before turn 50 is overwritten just before the
   pick), 1 summon
   (100 + 10 per free hero slot, +30 for AI_FOCUS_ECONOMY, +30 when no meld unit and a foreign node on the circle's plane,
   x 1.5 Conjurer / Channeler, x 5 when the circle is not at the fortress), 2 unit enchantments (by turn: 0 / 100 /
   200, + (250 - turn) / 2 between turns 21 and 199, else + 25), 3 city spells (50, or 5 for a player the human has
   not contacted; 150 for some groups), 4 disenchants (40 per curse on an own city, 300 per human Flying Fortress,
   100 per own warped node, 5 per enchanted human unit on the landmass of the caster's OWN fortress), 5 disjunction (per rival
   global), 6 Summoning Circle (100 with an own city and the circle timer below 0), 7 attack ((the attack target's
   weight + the personality's hostility) / 4 when an attack group is known - bug: one test reads power_history[83]
   instead of a spell group), 8 anti-realm globals, 9 globals (at most 300 above 400; both 0 under 200 mana), 10
   Spell of Mastery 500. The attack target is a weighted pick of the hostile wizards: (100 - relation) - 2 x bias,
   +50 at war, x (difficulty + 1) for the human. The army strength sums, circle_lm and circle_city are computed and never
   used
   Numbered: BUG #140, #305 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Spell_Category(int player)
{
    int i;
    int circle_lm;
    int plane;
    int circle_city;
    int has_meld_unit;
    int fortress_lm;
    int own_cities;
    long own_str;
    long enemy_str;
    char *known;
    int hostile_weight[6];
    int cat_weight[12];                         /* 11 used */

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i <= 10; i++)
        cat_weight[i] = 0;
    if (game_turn < 50)
        cat_weight[AI_CAT_NONE] = 50 - game_turn;
    plane = wizards[player].summoning_circle_xyp[XYP_PLANE];
    circle_lm = p_landmass_ids[plane][wizards[player].summoning_circle_xyp[XYP_Y]][wizards[player].summoning_circle_xyp[XYP_X]];
    circle_city = 0;
    for (i = 0; i < num_cities; i++)
        if (p_cities[i].x == wizards[player].summoning_circle_xyp[XYP_X] &&
            p_cities[i].y == wizards[player].summoning_circle_xyp[XYP_Y] &&
            p_cities[i].plane == wizards[player].summoning_circle_xyp[XYP_PLANE])
            circle_city = i;
    cat_weight[AI_CAT_SUMMON] = 100;
    cat_weight[AI_CAT_SUMMON] += (6 - Wizard_Hero_Count(player)) * 10;
    if (wizards[player].ai_focus == AI_FOCUS_ECONOMY)
        cat_weight[AI_CAT_SUMMON] += 30;
    has_meld_unit = 0;
    ai_foreign_node_on_circle_plane = 0;
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner == player && p_units[i].plane == plane &&
            (unit_types[p_units[i].type].ability_flags & AFLAG_MELD_WITH_NODE))
            has_meld_unit = 1;
    if (has_meld_unit == 0) {
        for (i = 0; i < NUM_NODES && ai_foreign_node_on_circle_plane == 0; i++)
            if (p_nodes[i].plane == plane && p_nodes[i].owner != player)
                ai_foreign_node_on_circle_plane = 1;
        if (ai_foreign_node_on_circle_plane == 1)
            cat_weight[AI_CAT_SUMMON] += 30;
    }
    own_str = 0;
    for (i = 0; i < ai_own_stack_count; i++)
        own_str += p_ai_own_stacks[i].strength;
    enemy_str = 0;
    for (i = 0; i < ai_enemy_stack_count; i++)
        enemy_str += p_ai_enemy_stacks[i].strength;
    if (wizards[player].retorts[RETORT_CONJURER] || wizards[player].retorts[RETORT_CHANNELER])
        cat_weight[AI_CAT_SUMMON] = cat_weight[AI_CAT_SUMMON] * 3 / 2;
    if (AI_Circle_At_Fortress(player) == 0)
        cat_weight[AI_CAT_SUMMON] *= 5;
    if (known[SPELL_DISENCHANT_AREA] == SPELLSTATE_KNOWN || known[SPELL_DISENCHANT_TRUE] == SPELLSTATE_KNOWN) {
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner == player) {
                if (p_cities[i].enchants[CITY_ENCH_CHAOS_RIFT])
                    cat_weight[AI_CAT_DISENCHANT] += 40;
                if (p_cities[i].enchants[CITY_ENCH_EVIL_PRESENCE])
                    cat_weight[AI_CAT_DISENCHANT] += 40;
                if (p_cities[i].enchants[CITY_ENCH_CURSED_LANDS])
                    cat_weight[AI_CAT_DISENCHANT] += 40;
                if (p_cities[i].enchants[CITY_ENCH_PESTILENCE])   /* weighed, but AI_Target_Disenchant never picks it */
                    cat_weight[AI_CAT_DISENCHANT] += 40;
                if (p_cities[i].enchants[CITY_ENCH_FAMINE])
                    cat_weight[AI_CAT_DISENCHANT] += 40;
            }
            if (p_cities[i].owner == PLAYER_HUMAN && p_cities[i].enchants[CITY_ENCH_FLYING_FORTRESS])
                cat_weight[AI_CAT_DISENCHANT] += 300;
        }
        for (i = 0; i < NUM_NODES; i++)
            if (p_nodes[i].owner == player && (p_nodes[i].meld_flags & NODE_WARPED) > 0)
                cat_weight[AI_CAT_DISENCHANT] += 100;
        fortress_lm = p_landmass_ids[p_fortresses[player].plane][p_fortresses[player].y][p_fortresses[player].x];
        for (i = 0; i < num_units; i++)
            if (p_units[i].owner == PLAYER_HUMAN && p_units[i].enchantments != 0 &&
                p_landmass_ids[p_units[i].plane][p_units[i].y][p_units[i].x] == fortress_lm)
                cat_weight[AI_CAT_DISENCHANT] += 5;
    }
    if (known[SPELL_DISJUNCTION] == SPELLSTATE_KNOWN || known[SPELL_DISJUNCTION_TRUE] == SPELLSTATE_KNOWN) {
        for (i = 0; i < num_players; i++) {
            if (i != player) {
                if (wizards[player].primary_realm != REALM_DEATH && wizards[i].globals.eternal_night)
                    cat_weight[AI_CAT_DISJUNCTION] += 100;
                if (wizards[i].globals.evil_omens &&
                    (wizards[i].primary_realm == REALM_LIFE || wizards[i].primary_realm == REALM_NATURE))
                    cat_weight[AI_CAT_DISJUNCTION] += 300;
                if (wizards[i].globals.evil_omens &&
                    (wizards[i].secondary_realm == REALM_LIFE || wizards[i].secondary_realm == REALM_NATURE))
                    cat_weight[AI_CAT_DISJUNCTION] += 150;
                if (wizards[i].globals.zombie_mastery)
                    cat_weight[AI_CAT_DISJUNCTION] += 100;
                if (wizards[i].globals.aura_of_majesty)
                    cat_weight[AI_CAT_DISJUNCTION] += 25;
                if (wizards[i].globals.wind_mastery)
                    cat_weight[AI_CAT_DISJUNCTION] += 25;
                if (wizards[i].globals.suppress_magic)
                    cat_weight[AI_CAT_DISJUNCTION] += 2000;
                if (wizards[i].globals.aura_of_majesty)   /* a second time: + 50 in all */
                    cat_weight[AI_CAT_DISJUNCTION] += 25;
                if (wizards[i].globals.nature_s_wrath &&
                    (wizards[i].primary_realm == REALM_DEATH || wizards[i].primary_realm == REALM_CHAOS))
                    cat_weight[AI_CAT_DISJUNCTION] += 400;
                if (wizards[i].globals.nature_s_wrath &&
                    (wizards[i].secondary_realm == REALM_DEATH || wizards[i].secondary_realm == REALM_CHAOS))
                    cat_weight[AI_CAT_DISJUNCTION] += 200;
                if (wizards[i].globals.herb_mastery)
                    cat_weight[AI_CAT_DISJUNCTION] += 50;
                if (wizards[i].globals.chaos_surge && wizards[i].primary_realm != REALM_CHAOS &&
                    wizards[i].secondary_realm != REALM_CHAOS)
                    cat_weight[AI_CAT_DISJUNCTION] += 100;
                if (wizards[i].globals.doom_mastery)
                    cat_weight[AI_CAT_DISJUNCTION] += 200;
                if (wizards[i].globals.great_wasting)
                    cat_weight[AI_CAT_DISJUNCTION] += 50;
                if (wizards[i].globals.meteor_storm)
                    cat_weight[AI_CAT_DISJUNCTION] += 100;
                if (wizards[i].globals.armageddon)
                    cat_weight[AI_CAT_DISJUNCTION] += 200;
                if (wizards[i].globals.tranquility && wizards[i].primary_realm == REALM_CHAOS)
                    cat_weight[AI_CAT_DISJUNCTION] += 500;
                if (wizards[i].globals.tranquility && wizards[i].secondary_realm == REALM_CHAOS)
                    cat_weight[AI_CAT_DISJUNCTION] += 250;
                if (wizards[i].globals.life_force && wizards[i].primary_realm == REALM_DEATH)
                    cat_weight[AI_CAT_DISJUNCTION] += 500;
                if (wizards[i].globals.life_force && wizards[i].secondary_realm == REALM_DEATH)
                    cat_weight[AI_CAT_DISJUNCTION] += 250;
                if (wizards[i].globals.crusade)
                    cat_weight[AI_CAT_DISJUNCTION] += 100;
                if (wizards[i].globals.just_cause)
                    cat_weight[AI_CAT_DISJUNCTION] += 10;
                if (wizards[i].globals.holy_arms)
                    cat_weight[AI_CAT_DISJUNCTION] += 100;
                if (wizards[i].globals.planar_seal && (i == 0 || Random(5) == 1))
                    cat_weight[AI_CAT_DISJUNCTION] += 200;
                if (wizards[i].globals.planar_seal)
                    cat_weight[AI_CAT_DISJUNCTION] += 25;
                if (wizards[i].globals.charm_of_life)
                    cat_weight[AI_CAT_DISJUNCTION] += 200;
            }
        }
    }
    if (known[SPELL_SPELL_OF_MASTERY] == SPELLSTATE_KNOWN)
        cat_weight[AI_CAT_MASTERY] = 500;
    if (ai_hostile_count > 0) {
        for (i = 0; i < 6; i++)
            hostile_weight[i] = 0;
        for (i = 0; i < ai_hostile_count; i++) {
            hostile_weight[i] = 100 - wizards[player].relationship[ai_hostile_wizards[i]] -
                         wizards[player].relation_bias[ai_hostile_wizards[i]] * 2;
            if (hostile_weight[i] < 0)
                hostile_weight[i] = 0;
            if (wizards[player].diplomacy_status[ai_hostile_wizards[i]] >= TREATY_WAR)
                hostile_weight[i] += 50;
            if (ai_hostile_wizards[i] == PLAYER_HUMAN)
                hostile_weight[i] *= difficulty + 1;
        }
        wizards[player].ai_attack_target = Random_Weighted_Pick(hostile_weight, ai_hostile_count);   /* a slot of
                                                   ai_hostile_wizards, not a player */
    } else
        wizards[player].ai_attack_target = -1;   /* no attack target */
    if (wizards[player].ai_attack_target != -1 &&
        (p_ai_spell_groups[AIG_CITY_CURSE_1] == 1 || p_ai_spell_groups[AIG_CITY_CURSE_2] == 1 || p_ai_spell_groups[AIG_CITY_CURSE_3] == 1 ||
         p_ai_spell_groups[AIG_CITY_CURSE_4] == 1 || p_ai_spell_groups[AIG_WARP_NODE] == 1 || p_ai_spell_groups[AIG_STASIS] == 1 ||
         p_ai_spell_groups[AIG_STORM_1] == 1 || p_ai_spell_groups[AIG_STORM_2] == 1 || p_ai_spell_groups[AIG_STORM_3] == 1 ||
         p_ai_spell_groups[AIG_STORM_4] == 1 || p_ai_spell_groups[AIG_TIME_STOP] == 1 || wizards[player].power_history[83] == 1 /* BUG #140: a group test was meant */ ||
         p_ai_spell_groups[AIG_DEATH_WISH] == 1 || p_ai_spell_groups[AIG_WIZARD_ATTACK] == 1))
        cat_weight[AI_CAT_ATTACK] = (hostile_weight[wizards[player].ai_attack_target] +
                         personality_hostility_mod[wizards[player].personality]) / 4;
    if (cat_weight[AI_CAT_ATTACK] < 0)
        cat_weight[AI_CAT_ATTACK] = 0;
    if (p_ai_spell_groups[AIG_CITY_BOOST_1] == 1 || p_ai_spell_groups[AIG_CITY_BOOST_2] == 1) {
        if (wizards[PLAYER_HUMAN].contacted[player] == 1)
            cat_weight[AI_CAT_CITY] = 50;
        else
            cat_weight[AI_CAT_CITY] = 5;
    }
    if (p_ai_spell_groups[AIG_CITY_BOOST_3] == 1 || p_ai_spell_groups[AIG_CITY_BOOST_4] == 1 || p_ai_spell_groups[AIG_TERRAIN] == 1 ||
        p_ai_spell_groups[AIG_CITY_WARD] == 1)
        cat_weight[AI_CAT_CITY] = 150;
    if (p_ai_spell_groups[AIG_MOVEMENT_1] == 1 || p_ai_spell_groups[AIG_MOVEMENT_2] == 1 || p_ai_spell_groups[AIG_MOVEMENT_3] == 1 ||
        p_ai_spell_groups[AIG_MOVEMENT_4] == 1) {
        if (game_turn < 50)
            cat_weight[AI_CAT_UNIT_ENCHANT] = 0;
        else if (game_turn < 200)
            cat_weight[AI_CAT_UNIT_ENCHANT] = 100;
        else
            cat_weight[AI_CAT_UNIT_ENCHANT] = 200;
    }
    if (p_ai_spell_groups[AIG_WEAPON_1] == 1 || p_ai_spell_groups[AIG_WEAPON_2] == 1 || p_ai_spell_groups[AIG_WEAPON_3] == 1 ||
        p_ai_spell_groups[AIG_WEAPON_4] == 1 || p_ai_spell_groups[AIG_ARMOR_1] == 1 || p_ai_spell_groups[AIG_ARMOR_2] == 1 ||
        p_ai_spell_groups[AIG_ARMOR_3] == 1 || p_ai_spell_groups[AIG_ARMOR_4] == 1 || p_ai_spell_groups[AIG_HEALING] == 1) {
        if (game_turn > 20 && game_turn < 200)
            cat_weight[AI_CAT_UNIT_ENCHANT] += (250 - game_turn) / 2;
        else
            cat_weight[AI_CAT_UNIT_ENCHANT] += 25;
    }
    if (known[SPELL_HERB_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.herb_mastery == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_NATURES_WRATH] == SPELLSTATE_KNOWN && wizards[player].globals.nature_s_wrath == 0 &&
        (wizards[PLAYER_HUMAN].primary_realm == REALM_DEATH || wizards[PLAYER_HUMAN].secondary_realm == REALM_DEATH ||
         wizards[PLAYER_HUMAN].primary_realm == REALM_CHAOS || wizards[PLAYER_HUMAN].secondary_realm == REALM_CHAOS))
        cat_weight[AI_CAT_ANTI_REALM] += 100;
    if (known[SPELL_WIND_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.wind_mastery == 0)
        cat_weight[AI_CAT_GLOBAL] += 75;
    if (known[SPELL_AWARENESS] == SPELLSTATE_KNOWN && wizards[player].globals.awareness == 0)
        cat_weight[AI_CAT_GLOBAL] += 500;
    if (known[SPELL_AURA_OF_MAJESTY] == SPELLSTATE_KNOWN && wizards[player].globals.aura_of_majesty == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    /* BUG #419: into category 8 (AI_Pick_Anti_Realm_Global), which has no Suppress Magic slot; only category 9's
       AI_Pick_Global_Enchant casts it */
    if (known[SPELL_SUPPRESS_MAGIC] == SPELLSTATE_KNOWN && wizards[player].globals.suppress_magic == 0)
        cat_weight[AI_CAT_ANTI_REALM] += 100;
    if (known[SPELL_SPELL_BINDING] == SPELLSTATE_KNOWN)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_GREAT_WASTING] == SPELLSTATE_KNOWN && wizards[player].globals.great_wasting == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_CHAOS_SURGE] == SPELLSTATE_KNOWN && wizards[player].globals.chaos_surge == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_DOOM_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.doom_mastery == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_METEOR_STORM] == SPELLSTATE_KNOWN && wizards[player].globals.meteor_storm == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_ARMAGEDDON] == SPELLSTATE_KNOWN && wizards[player].globals.armageddon == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_JUST_CAUSE] == SPELLSTATE_KNOWN && wizards[player].globals.just_cause == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_HOLY_ARMS] == SPELLSTATE_KNOWN && wizards[player].globals.holy_arms == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_CHARM_OF_LIFE] == SPELLSTATE_KNOWN && wizards[player].globals.charm_of_life == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_CRUSADE] == SPELLSTATE_KNOWN && wizards[player].globals.crusade == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_GREAT_UNSUMMONING] == SPELLSTATE_KNOWN)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_DEATH_WISH] == SPELLSTATE_KNOWN)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_LIFE_FORCE] == SPELLSTATE_KNOWN && wizards[player].globals.life_force == 0 &&
        (wizards[PLAYER_HUMAN].primary_realm == REALM_DEATH || wizards[PLAYER_HUMAN].secondary_realm == REALM_DEATH))
        cat_weight[AI_CAT_ANTI_REALM] += 100;
    if (known[SPELL_TRANQUILITY] == SPELLSTATE_KNOWN && wizards[player].globals.tranquility == 0 &&
        (wizards[PLAYER_HUMAN].primary_realm == REALM_CHAOS || wizards[PLAYER_HUMAN].secondary_realm == REALM_CHAOS))
        cat_weight[AI_CAT_ANTI_REALM] += 100;
    if (known[SPELL_ETERNAL_NIGHT] == SPELLSTATE_KNOWN && wizards[player].globals.eternal_night == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (known[SPELL_EVIL_OMENS] == SPELLSTATE_KNOWN && wizards[player].globals.evil_omens == 0 &&
        (wizards[PLAYER_HUMAN].primary_realm == REALM_LIFE || wizards[PLAYER_HUMAN].secondary_realm == REALM_LIFE ||
         wizards[PLAYER_HUMAN].primary_realm == REALM_NATURE || wizards[PLAYER_HUMAN].secondary_realm == REALM_NATURE))
        cat_weight[AI_CAT_ANTI_REALM] += 100;
    if (known[SPELL_ZOMBIE_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.zombie_mastery == 0)
        cat_weight[AI_CAT_GLOBAL] += 100;
    if (cat_weight[AI_CAT_GLOBAL] > 400)
        cat_weight[AI_CAT_GLOBAL] = 300;
    if (known[SPELL_PLANAR_SEAL] == SPELLSTATE_KNOWN && wizards[player].globals.planar_seal == 0 &&
        AI_More_Myrror_Cities_Than_Human(player) == 1)
        cat_weight[AI_CAT_GLOBAL] += 50;
    if (wizards[player].mana_reserve < 200) {
        cat_weight[AI_CAT_GLOBAL] = 0;
        cat_weight[AI_CAT_ANTI_REALM] = 0;
    }
    if (known[SPELL_SUMMONING_CIRCLE] == SPELLSTATE_KNOWN && p_ai_circle_timer[player] < 0) {
        own_cities = 0;
        for (i = 0; i < num_cities; i++)
            if (p_cities[i].owner == player)
                own_cities++;
        if (own_cities > 0)
            cat_weight[AI_CAT_SUMMONING_CIRCLE] = 100;
    }
    cat_weight[AI_CAT_NONE] = 1;
    return Random_Weighted_Pick(cat_weight, 11);
}

/* (player) the AI's known spells: every spell the player knows (library 2) outside the 21 combat-only AI groups
   (AIG_DAMAGE_*, _COMBAT_AID_*, _COMBAT_SUMMON_*, _BATTLEFIELD_*, _DISPEL_*, AIG_SPELL_LOCK) and AIG_NONE goes to ai_known_spells and flags its group
   in p_ai_spell_groups */
void AI_Build_Known_Spell_Groups(int player)
{
    int realm;
    int slot;
    int ai_group;
    int k;
    int spell;

    ai_known_spell_count = 0;
    for (k = 0; k <= AIG_LAST; k++)
        p_ai_spell_groups[k] = 0;
    for (realm = 0; realm < NUM_REALMS; realm++) {
        for (slot = 0; slot < SPELLS_PER_REALM; slot++) {
            if (wizards[player].spell_library[realm][slot] == SPELLSTATE_KNOWN) {
                spell = realm * SPELLS_PER_REALM + slot + 1;
                ai_group = p_spell_data[spell].ai_group;
                if (ai_group != AIG_DAMAGE_1 && ai_group != AIG_DAMAGE_2 && ai_group != AIG_DAMAGE_3 && ai_group != AIG_DAMAGE_4 && ai_group != AIG_COMBAT_AID_1 &&
                    ai_group != AIG_COMBAT_AID_2 && ai_group != AIG_COMBAT_AID_3 && ai_group != AIG_COMBAT_AID_4 && ai_group != AIG_COMBAT_SUMMON_1 && ai_group != AIG_COMBAT_SUMMON_2 &&
                    ai_group != AIG_COMBAT_SUMMON_3 && ai_group != AIG_COMBAT_SUMMON_4 && ai_group != AIG_BATTLEFIELD_1 && ai_group != AIG_BATTLEFIELD_2 && ai_group != AIG_BATTLEFIELD_3 &&
                    ai_group != AIG_BATTLEFIELD_4 && ai_group != AIG_DISPEL_1 && ai_group != AIG_DISPEL_2 && ai_group != AIG_DISPEL_3 && ai_group != AIG_SPELL_LOCK &&
                    ai_group != AIG_NONE) {
                    ai_known_spells[ai_known_spell_count] = spell;
                    p_ai_spell_groups[ai_group] = 1;
                    ai_known_spell_count++;
                }
            }
        }
    }
}

/* (player) the summon pick, a weighted random choice over 50 slots: each known creature summon (base cost / 10)
   squared (slot 11, the 65 summon, is not squared; slot 26 is 0), the transport summon (43) only while the player
   has fewer than 10 transports and fewer than turn / 60 (base cost / 5); Resurrection 100 (fewer than 6 heroes and a
   dead one); the node spirits (129 / 201) 200 / 100 when a foreign node exists on the circle's plane and no own meld unit is
   there (Magic Spirit only without Guardian Spirit known); the item
   spells 20 / 30 (30 / 40 with Artificer); with fewer than 6 heroes Incarnation 500 (only while the player has
   Barbarian Spearmen), else Summon Champion 400, else Summon Hero 200; the 17 summon base cost / 5. Evil Omens cuts the Life and Nature
   summons to 2 / 3, the Chaos and Death penalties cut those realms' summons (0 below 20, / 3 below 50, else / 2).
   Returns the spell, 0 for an empty slot
   Numbered: BUG #141 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Summon(int player)
{
    char *known;
    int i;
    int has_spearmen;
    int pick;

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i < 50; i++)
        ai_spell_pick_weights[i] = 0;
    if (known[SPELL_WAR_BEARS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[1] = p_spell_data[SPELL_WAR_BEARS].base_cost / 10;
    if (known[SPELL_SPRITES] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[2] = p_spell_data[SPELL_SPRITES].base_cost / 10;
    if (known[SPELL_GIANT_SPIDERS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[3] = p_spell_data[SPELL_GIANT_SPIDERS].base_cost / 10;
    if (known[SPELL_BASILISK] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[4] = p_spell_data[SPELL_BASILISK].base_cost / 10;
    if (known[SPELL_STONE_GIANT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[5] = p_spell_data[SPELL_STONE_GIANT].base_cost / 10;
    if (known[SPELL_GORGONS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[6] = p_spell_data[SPELL_GORGONS].base_cost / 10;
    if (known[SPELL_BEHEMOTH] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[7] = p_spell_data[SPELL_BEHEMOTH].base_cost / 10;
    if (known[SPELL_COLOSSUS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[8] = p_spell_data[SPELL_COLOSSUS].base_cost / 10;
    if (known[SPELL_GREAT_WYRM] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[9] = p_spell_data[SPELL_GREAT_WYRM].base_cost / 10;
    if (known[SPELL_FLOATING_ISLAND] == SPELLSTATE_KNOWN && ai_own_transport_count < 10 && game_turn / 60 > ai_own_transport_count)
        ai_spell_pick_weights[10] = p_spell_data[SPELL_FLOATING_ISLAND].base_cost / 5;
    if (known[SPELL_STORM_GIANT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[11] = p_spell_data[SPELL_STORM_GIANT].base_cost / 10;
    if (known[SPELL_DJINN] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[12] = p_spell_data[SPELL_DJINN].base_cost / 10;
    if (known[SPELL_SKY_DRAKE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[13] = p_spell_data[SPELL_SKY_DRAKE].base_cost / 10;
    if (known[SPELL_HELL_HOUNDS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[14] = p_spell_data[SPELL_HELL_HOUNDS].base_cost / 10;
    if (known[SPELL_GARGOYLES] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[15] = p_spell_data[SPELL_GARGOYLES].base_cost / 10;
    if (known[SPELL_DOOM_BAT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[16] = p_spell_data[SPELL_DOOM_BAT].base_cost / 10;
    if (known[SPELL_CHAOS_SPAWN] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[17] = p_spell_data[SPELL_CHAOS_SPAWN].base_cost / 10;
    if (known[SPELL_FIRE_GIANT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[18] = p_spell_data[SPELL_FIRE_GIANT].base_cost / 10;
    if (known[SPELL_CHIMERAS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[19] = p_spell_data[SPELL_CHIMERAS].base_cost / 10;
    if (known[SPELL_EFREET] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[20] = p_spell_data[SPELL_EFREET].base_cost / 10;
    if (known[SPELL_HYDRA] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[21] = p_spell_data[SPELL_HYDRA].base_cost / 10;
    if (known[SPELL_GREAT_DRAKE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[22] = p_spell_data[SPELL_GREAT_DRAKE].base_cost / 10;
    if (known[SPELL_UNICORNS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[23] = p_spell_data[SPELL_UNICORNS].base_cost / 10;
    if (known[SPELL_ANGEL] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[24] = p_spell_data[SPELL_ANGEL].base_cost / 10;
    if (known[SPELL_ARCH_ANGEL] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[25] = p_spell_data[SPELL_ARCH_ANGEL].base_cost / 10;
    ai_spell_pick_weights[26] = 0;
    if (known[SPELL_SKELETONS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[27] = p_spell_data[SPELL_SKELETONS].base_cost / 10;
    if (known[SPELL_GHOULS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[28] = p_spell_data[SPELL_GHOULS].base_cost / 10;
    if (known[SPELL_NIGHT_STALKER] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[29] = p_spell_data[SPELL_NIGHT_STALKER].base_cost / 10;
    if (known[SPELL_WRAITHS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[31] = p_spell_data[SPELL_WRAITHS].base_cost / 10;
    if (known[SPELL_SHADOW_DEMONS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[32] = p_spell_data[SPELL_SHADOW_DEMONS].base_cost / 10;
    if (known[SPELL_DEATH_KNIGHTS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[33] = p_spell_data[SPELL_DEATH_KNIGHTS].base_cost / 10;
    if (known[SPELL_DEMON_LORD] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[34] = p_spell_data[SPELL_DEMON_LORD].base_cost / 10;
    ai_spell_pick_weights[1] = ai_spell_pick_weights[1] * ai_spell_pick_weights[1];
    ai_spell_pick_weights[2] = ai_spell_pick_weights[2] * ai_spell_pick_weights[2];
    ai_spell_pick_weights[3] = ai_spell_pick_weights[3] * ai_spell_pick_weights[3];
    ai_spell_pick_weights[4] = ai_spell_pick_weights[4] * ai_spell_pick_weights[4];
    ai_spell_pick_weights[5] = ai_spell_pick_weights[5] * ai_spell_pick_weights[5];
    ai_spell_pick_weights[6] = ai_spell_pick_weights[6] * ai_spell_pick_weights[6];
    ai_spell_pick_weights[7] = ai_spell_pick_weights[7] * ai_spell_pick_weights[7];
    ai_spell_pick_weights[8] = ai_spell_pick_weights[8] * ai_spell_pick_weights[8];
    ai_spell_pick_weights[9] = ai_spell_pick_weights[9] * ai_spell_pick_weights[9];
    ai_spell_pick_weights[10] = ai_spell_pick_weights[10] * ai_spell_pick_weights[10];
    ai_spell_pick_weights[12] = ai_spell_pick_weights[12] * ai_spell_pick_weights[12];
    ai_spell_pick_weights[13] = ai_spell_pick_weights[13] * ai_spell_pick_weights[13];
    ai_spell_pick_weights[14] = ai_spell_pick_weights[14] * ai_spell_pick_weights[14];
    ai_spell_pick_weights[15] = ai_spell_pick_weights[15] * ai_spell_pick_weights[15];
    ai_spell_pick_weights[16] = ai_spell_pick_weights[16] * ai_spell_pick_weights[16];
    ai_spell_pick_weights[17] = ai_spell_pick_weights[17] * ai_spell_pick_weights[17];
    ai_spell_pick_weights[18] = ai_spell_pick_weights[18] * ai_spell_pick_weights[18];
    ai_spell_pick_weights[19] = ai_spell_pick_weights[19] * ai_spell_pick_weights[19];
    ai_spell_pick_weights[20] = ai_spell_pick_weights[20] * ai_spell_pick_weights[20];
    ai_spell_pick_weights[21] = ai_spell_pick_weights[21] * ai_spell_pick_weights[21];
    ai_spell_pick_weights[22] = ai_spell_pick_weights[22] * ai_spell_pick_weights[22];
    ai_spell_pick_weights[23] = ai_spell_pick_weights[23] * ai_spell_pick_weights[23];
    ai_spell_pick_weights[24] = ai_spell_pick_weights[24] * ai_spell_pick_weights[24];
    ai_spell_pick_weights[25] = ai_spell_pick_weights[25] * ai_spell_pick_weights[25];
    ai_spell_pick_weights[26] = ai_spell_pick_weights[26] * ai_spell_pick_weights[26];
    ai_spell_pick_weights[27] = ai_spell_pick_weights[27] * ai_spell_pick_weights[27];
    ai_spell_pick_weights[28] = ai_spell_pick_weights[28] * ai_spell_pick_weights[28];
    ai_spell_pick_weights[29] = ai_spell_pick_weights[29] * ai_spell_pick_weights[29];
    ai_spell_pick_weights[31] = ai_spell_pick_weights[31] * ai_spell_pick_weights[31];
    ai_spell_pick_weights[32] = ai_spell_pick_weights[32] * ai_spell_pick_weights[32];
    ai_spell_pick_weights[33] = ai_spell_pick_weights[33] * ai_spell_pick_weights[33];
    ai_spell_pick_weights[34] = ai_spell_pick_weights[34] * ai_spell_pick_weights[34];
    if (known[SPELL_RESURRECTION] == SPELLSTATE_KNOWN && Wizard_Hero_Count(player) < 6 && Wizard_Dead_Hero_Count(player) > 0)
        ai_spell_pick_weights[35] = 100;
    ai_spell_pick_weights[36] = 0;
    ai_spell_pick_weights[37] = 0;
    if (known[SPELL_GUARDIAN_SPIRIT] == SPELLSTATE_KNOWN) {
        if (ai_foreign_node_on_circle_plane == 1)
            ai_spell_pick_weights[36] = 200;
        else
            ai_spell_pick_weights[36] = 0;
    } else if (known[SPELL_MAGIC_SPIRIT] == SPELLSTATE_KNOWN) {
        if (ai_foreign_node_on_circle_plane == 1)
            ai_spell_pick_weights[37] = 100;
        else
            ai_spell_pick_weights[37] = 0;
    }
    if (known[SPELL_ENCHANT_ITEM] == SPELLSTATE_KNOWN) {
        if (wizards[player].retorts[RETORT_ARTIFICER])
            ai_spell_pick_weights[38] = 30;
        else
            ai_spell_pick_weights[38] = 20;
    }
    if (known[SPELL_CREATE_ARTIFACT] == SPELLSTATE_KNOWN) {
        if (wizards[player].retorts[RETORT_ARTIFICER])
            ai_spell_pick_weights[39] = 40;
        else
            ai_spell_pick_weights[39] = 30;
    }
    /* BUG #420: Incarnation's test reads type 39 (Barbarian Spearmen) where Torin the Chosen is 34, and requires one (it
       should require none): an AI owning Spearmen weighs Incarnation 500 even after Torin came - Incarnation_Cast
       then refuses - and the else-if keeps Summon Champion / Summon Hero out; clarified using ReMoM repo */
    has_spearmen = 0;
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner == player && p_units[i].type == UNIT_BARBARIAN_SPEARMEN)
            has_spearmen = 1;
    if (Wizard_Hero_Count(player) < 6) {
        if (known[SPELL_INCARNATION] == SPELLSTATE_KNOWN && has_spearmen == 1)
            ai_spell_pick_weights[40] = 500;
        else if (known[SPELL_SUMMON_CHAMPION] == SPELLSTATE_KNOWN)
            ai_spell_pick_weights[41] = 400;
        else if (known[SPELL_SUMMON_HERO] == SPELLSTATE_KNOWN)
            ai_spell_pick_weights[42] = 200;
    }
    if (known[SPELL_COCKATRICES] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[43] = p_spell_data[SPELL_COCKATRICES].base_cost / 5;
    if (AI_Life_Casting_Penalized() == 1) {
        ai_spell_pick_weights[23] = ai_spell_pick_weights[23] * 2 / 3;
        ai_spell_pick_weights[24] = ai_spell_pick_weights[24] * 2 / 3;
        ai_spell_pick_weights[25] = ai_spell_pick_weights[25] * 2 / 3;
        ai_spell_pick_weights[26] = ai_spell_pick_weights[26] * 2 / 3;
        ai_spell_pick_weights[35] = ai_spell_pick_weights[35] * 2 / 3;
        ai_spell_pick_weights[36] = ai_spell_pick_weights[36] * 2 / 3;
        ai_spell_pick_weights[40] = ai_spell_pick_weights[40] * 2 / 3;
    }
    if (AI_Nature_Casting_Penalized() == 1) {
        ai_spell_pick_weights[1] = ai_spell_pick_weights[1] * 2 / 3;
        ai_spell_pick_weights[2] = ai_spell_pick_weights[2] * 2 / 3;
        ai_spell_pick_weights[3] = ai_spell_pick_weights[3] * 2 / 3;
        ai_spell_pick_weights[4] = ai_spell_pick_weights[4] * 2 / 3;
        ai_spell_pick_weights[5] = ai_spell_pick_weights[5] * 2 / 3;
        ai_spell_pick_weights[6] = ai_spell_pick_weights[6] * 2 / 3;
        ai_spell_pick_weights[7] = ai_spell_pick_weights[7] * 2 / 3;
        ai_spell_pick_weights[8] = ai_spell_pick_weights[8] * 2 / 3;
        ai_spell_pick_weights[9] = ai_spell_pick_weights[9] * 2 / 3;
        ai_spell_pick_weights[10] = ai_spell_pick_weights[10] * 2 / 3;
        ai_spell_pick_weights[43] = ai_spell_pick_weights[43] * 2 / 3;
    }
    if (AI_Death_Casting_Penalized() == 1) {
        for (i = 27; i <= 34; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    if (AI_Chaos_Casting_Penalized() == 1) {
        for (i = 14; i <= 22; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    pick = Random_Weighted_Pick(ai_spell_pick_weights, 50);
    switch (pick) {
    case 1:
        return SPELL_WAR_BEARS;
    case 2:
        return SPELL_SPRITES;
    case 3:
        return SPELL_GIANT_SPIDERS;
    case 4:
        return SPELL_BASILISK;
    case 5:
        return SPELL_STONE_GIANT;
    case 6:
        return SPELL_GORGONS;
    case 7:
        return SPELL_BEHEMOTH;
    case 8:
        return SPELL_COLOSSUS;
    case 9:
        return SPELL_GREAT_WYRM;
    case 10:
        return SPELL_FLOATING_ISLAND;
    case 11:
        return SPELL_STORM_GIANT;
    case 12:
        return SPELL_DJINN;
    case 13:
        return SPELL_SKY_DRAKE;
    case 14:
        return SPELL_HELL_HOUNDS;
    case 15:
        return SPELL_GARGOYLES;
    case 16:
        return SPELL_DOOM_BAT;
    case 17:
        return SPELL_CHAOS_SPAWN;
    case 18:
        return SPELL_FIRE_GIANT;
    case 19:
        return SPELL_CHIMERAS;
    case 20:
        return SPELL_EFREET;
    case 21:
        return SPELL_HYDRA;
    case 22:
        return SPELL_GREAT_DRAKE;
    case 23:
        return SPELL_UNICORNS;
    case 24:
        return SPELL_ANGEL;
    case 25:
        return SPELL_ARCH_ANGEL;
    case 26:
    case 27:
        return SPELL_SKELETONS;
    case 28:
        return SPELL_GHOULS;
    case 29:
        return SPELL_NIGHT_STALKER;
    case 31:
        return SPELL_WRAITHS;
    case 32:
        return SPELL_SHADOW_DEMONS;
    case 33:
        return SPELL_DEATH_KNIGHTS;
    case 34:
        return SPELL_DEMON_LORD;
    case 35:
        return SPELL_RESURRECTION;
    case 36:
        return SPELL_GUARDIAN_SPIRIT;
    case 37:
        return SPELL_MAGIC_SPIRIT;
    case 38:
        return SPELL_ENCHANT_ITEM;
    case 39:
        return SPELL_CREATE_ARTIFACT;
    case 40:
        return SPELL_INCARNATION;
    case 41:
        return SPELL_SUMMON_CHAMPION;
    case 42:
        return SPELL_SUMMON_HERO;
    case 43:
        return SPELL_COCKATRICES;
    }
    return SPELL_NONE;
}

/* (player) the unit enchantment pick, a weighted random choice over 50 slots: a fixed weight for each known unit
   enchantment (Plane Shift only while none of wizards 0..3 has Planar Seal; the two mastery-gated slots only while
   the player lacks Doom / Zombie Mastery). Evil Omens cuts the Life and Nature entries to 2 / 3 (one Nature entry to
   1 / 2), the Death and Chaos penalties cut those realms' entries (0 below 20, / 3 below 50, else / 2).
   Returns the spell, 0 for an empty slot */
int AI_Pick_Unit_Enchant(int player)
{
    char *known;
    int i;
    int pick;

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i < 50; i++)
        ai_spell_pick_weights[i] = 0;
    if (known[SPELL_RESIST_ELEMENTS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[1] = 5;
    if (known[SPELL_STONE_SKIN] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[2] = 15;
    if (known[SPELL_ELEMENTAL_ARMOR] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[3] = 5;
    if (known[SPELL_IRON_SKIN] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[4] = 30;
    if (known[SPELL_REGENERATION] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[5] = 10;
    if (known[SPELL_RESIST_MAGIC] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[6] = 5;
    if (known[SPELL_BLUR] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[7] = 0;
    if (known[SPELL_MAGIC_IMMUNITY] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[9] = 20;
    if (known[SPELL_FLAME_BLADE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[10] = 30;
    if (known[SPELL_ELDRITCH_WEAPON] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[11] = 15;
    if (known[SPELL_IMMOLATION] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[12] = 10;
    if (known[SPELL_HOLY_WEAPON] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[13] = 10;
    if (known[SPELL_BLESS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[14] = 5;
    if (known[SPELL_HOLY_ARMOR] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[15] = 10;
    if (known[SPELL_HEROISM] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[16] = 10;
    if (known[SPELL_TRUE_SIGHT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[17] = 5;
    if (known[SPELL_INVULNERABILITY] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[18] = 20;
    if (known[SPELL_LIONHEART] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[19] = 30;
    if (known[SPELL_RIGHTEOUSNESS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[20] = 5;
    if (known[SPELL_CLOAK_OF_FEAR] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[21] = 10;
    if (known[SPELL_WRAITH_FORM] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[23] = 5;
    if (known[SPELL_WATER_WALKING] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[24] = 30;
    if (known[SPELL_PATH_FINDING] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[25] = 5;
    if (known[SPELL_ENTANGLE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[26] = 0;
    if (known[SPELL_FLIGHT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[27] = 25;
    if (known[SPELL_WIND_WALKING] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[28] = 50;
    if (known[SPELL_ENDURANCE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[29] = 10;
    if (known[SPELL_PLANE_SHIFT] == SPELLSTATE_KNOWN && wizards[PLAYER_HUMAN].globals.planar_seal == 0 && wizards[1].globals.planar_seal == 0 &&
        wizards[2].globals.planar_seal == 0 && wizards[3].globals.planar_seal == 0)
        ai_spell_pick_weights[30] = 20;
    if (known[SPELL_PLANAR_TRAVEL] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[31] = 5;
    if (known[SPELL_NATURES_CURES] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[32] = 5;
    if (known[SPELL_ENCHANT_ROAD] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[33] = 10;
    if (known[SPELL_SPELL_LOCK] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[34] = 10;
    if (known[SPELL_INVISIBILITY] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[35] = 40;
    if (known[SPELL_CHAOS_CHANNELS] == SPELLSTATE_KNOWN && wizards[player].globals.doom_mastery == 0)
        ai_spell_pick_weights[36] = 25;
    if (known[SPELL_LYCANTHROPY] == SPELLSTATE_KNOWN && wizards[player].globals.zombie_mastery == 0)
        ai_spell_pick_weights[37] = 25;
    if (known[SPELL_BLACK_CHANNELS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[38] = 25;
    if (known[SPELL_GUARDIAN_WIND] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[39] = 10;
    if (known[SPELL_GIANT_STRENGTH] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[40] = 10;
    if (AI_Life_Casting_Penalized() == 1) {
        for (i = 13; i <= 20; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
        ai_spell_pick_weights[29] = ai_spell_pick_weights[29] * 2 / 3;
        ai_spell_pick_weights[30] = ai_spell_pick_weights[30] * 2 / 3;
        ai_spell_pick_weights[31] = ai_spell_pick_weights[31] * 2 / 3;
    }
    if (AI_Nature_Casting_Penalized() == 1) {
        for (i = 1; i <= 5; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
        ai_spell_pick_weights[24] = ai_spell_pick_weights[24] * 2 / 3;
        ai_spell_pick_weights[25] = ai_spell_pick_weights[25] * 2 / 3;
        ai_spell_pick_weights[26] = ai_spell_pick_weights[26] * 2 / 3;
        ai_spell_pick_weights[32] = ai_spell_pick_weights[32] * 2 / 3;
        ai_spell_pick_weights[40] = ai_spell_pick_weights[40] / 2;
    }
    if (AI_Death_Casting_Penalized() == 1) {
        for (i = 21; i <= 23; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
        i = 37;
        if (ai_spell_pick_weights[i] < 20)
            ai_spell_pick_weights[i] = 0;
        else
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        i = 38;
        if (ai_spell_pick_weights[i] < 20)
            ai_spell_pick_weights[i] = 0;
        else
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
    }
    if (AI_Chaos_Casting_Penalized() == 1) {
        for (i = 10; i <= 12; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
        i = 36;
        if (ai_spell_pick_weights[i] < 20)
            ai_spell_pick_weights[i] = 0;
        else
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
    }
    pick = Random_Weighted_Pick(ai_spell_pick_weights, 50);
    switch (pick) {
    case 1:
        return SPELL_RESIST_ELEMENTS;
    case 2:
        return SPELL_STONE_SKIN;
    case 3:
        return SPELL_ELEMENTAL_ARMOR;
    case 4:
        return SPELL_IRON_SKIN;
    case 5:
        return SPELL_REGENERATION;
    case 6:
        return SPELL_RESIST_MAGIC;
    case 7:
        return SPELL_NONE;
    case 9:
        return SPELL_MAGIC_IMMUNITY;
    case 10:
        return SPELL_FLAME_BLADE;
    case 11:
        return SPELL_ELDRITCH_WEAPON;
    case 12:
        return SPELL_IMMOLATION;
    case 13:
        return SPELL_HOLY_WEAPON;
    case 14:
        return SPELL_BLESS;
    case 15:
        return SPELL_HOLY_ARMOR;
    case 16:
        return SPELL_HEROISM;
    case 17:
        return SPELL_TRUE_SIGHT;
    case 18:
        return SPELL_INVULNERABILITY;
    case 19:
        return SPELL_LIONHEART;
    case 20:
        return SPELL_RIGHTEOUSNESS;
    case 21:
        return SPELL_CLOAK_OF_FEAR;
    case 23:
        return SPELL_WRAITH_FORM;
    case 24:
        return SPELL_WATER_WALKING;
    case 25:
        return SPELL_PATH_FINDING;
    case 26:
        return SPELL_ENTANGLE;
    case 27:
        return SPELL_FLIGHT;
    case 28:
        return SPELL_WIND_WALKING;
    case 29:
        return SPELL_ENDURANCE;
    case 30:
        return SPELL_PLANE_SHIFT;
    case 31:
        return SPELL_PLANAR_TRAVEL;
    case 32:
        return SPELL_NATURES_CURES;
    case 33:
        return SPELL_ENCHANT_ROAD;
    case 34:
        return SPELL_SPELL_LOCK;
    case 35:
        return SPELL_INVISIBILITY;
    case 36:
        return SPELL_CHAOS_CHANNELS;
    case 37:
        return SPELL_LYCANTHROPY;
    case 38:
        return SPELL_BLACK_CHANNELS;
    case 39:
        return SPELL_GUARDIAN_WIND;
    case 40:
        return SPELL_GIANT_STRENGTH;
    }
    return SPELL_NONE;
}

/* (player) the anti-realm global pick: Nature's Wrath (40), Life Force (156), Tranquility (157) and Evil Omens
   (198) when known, weighted base cost / 10; Evil Omens cuts the Life and Nature ones to 2 / 3, the Death penalty
   cuts Evil Omens (0 below 20, / 3 below 50, else / 2). Returns the spell or 0
   Numbered: BUG #240 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Anti_Realm_Global(int player)
{
    int i;
    char *known;
    int pick;

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i < 50; i++)
        ai_spell_pick_weights[i] = 0;
    if (known[SPELL_NATURES_WRATH] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[1] = p_spell_data[SPELL_NATURES_WRATH].base_cost / 10;
    if (known[SPELL_LIFE_FORCE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[2] = p_spell_data[SPELL_LIFE_FORCE].base_cost / 10;
    if (known[SPELL_TRANQUILITY] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[3] = p_spell_data[SPELL_TRANQUILITY].base_cost / 10;
    if (known[SPELL_EVIL_OMENS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[4] = p_spell_data[SPELL_EVIL_OMENS].base_cost / 10;
    if (AI_Life_Casting_Penalized() == 1) {
        for (i = 2; i <= 3; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
    }
    if (AI_Nature_Casting_Penalized() == 1) {
        for (i = 1; i <= 1; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
    }
    if (AI_Death_Casting_Penalized() == 1) {
        for (i = 4; i <= 4; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    pick = Random_Weighted_Pick(ai_spell_pick_weights, 50);
    switch (pick) {
    case 1:
        return SPELL_NATURES_WRATH;
    case 2:
        return SPELL_LIFE_FORCE;
    case 3:
        return SPELL_TRANQUILITY;
    case 4:
        return SPELL_EVIL_OMENS;
    }
    return 0;
}

/* (player) the global enchantment pick: 20 known overland globals weighted base cost / 10, the lasting ones only
   while the player does not have them active (Planar Seal base cost / 20 and only with more Myrror cities than the
   human); Evil Omens cuts the Life and Nature ones to 2 / 3, the Death and Chaos penalties cut those realms' ones.
   Returns the spell or 0 */
int AI_Pick_Global_Enchant(int player)
{
    char *known;
    int i;
    int pick;

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i < 50; i++)
        ai_spell_pick_weights[i] = 0;
    if (known[SPELL_HERB_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.herb_mastery == 0)
        ai_spell_pick_weights[1] = p_spell_data[SPELL_HERB_MASTERY].base_cost / 10;
    if (known[SPELL_WIND_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.wind_mastery == 0)
        ai_spell_pick_weights[2] = p_spell_data[SPELL_WIND_MASTERY].base_cost / 10;
    if (known[SPELL_AURA_OF_MAJESTY] == SPELLSTATE_KNOWN && wizards[player].globals.aura_of_majesty == 0)
        ai_spell_pick_weights[3] = p_spell_data[SPELL_AURA_OF_MAJESTY].base_cost / 10;
    if (known[SPELL_GREAT_UNSUMMONING] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[4] = p_spell_data[SPELL_GREAT_UNSUMMONING].base_cost / 10;
    if (known[SPELL_SUPPRESS_MAGIC] == SPELLSTATE_KNOWN && wizards[player].globals.suppress_magic == 0)
        ai_spell_pick_weights[5] = p_spell_data[SPELL_SUPPRESS_MAGIC].base_cost / 10;
    if (known[SPELL_SPELL_BINDING] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[6] = p_spell_data[SPELL_SPELL_BINDING].base_cost / 10;
    if (known[SPELL_GREAT_WASTING] == SPELLSTATE_KNOWN && wizards[player].globals.great_wasting == 0)
        ai_spell_pick_weights[7] = p_spell_data[SPELL_GREAT_WASTING].base_cost / 10;
    if (known[SPELL_CHAOS_SURGE] == SPELLSTATE_KNOWN && wizards[player].globals.chaos_surge == 0)
        ai_spell_pick_weights[8] = p_spell_data[SPELL_CHAOS_SURGE].base_cost / 10;
    if (known[SPELL_DOOM_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.doom_mastery == 0)
        ai_spell_pick_weights[9] = p_spell_data[SPELL_DOOM_MASTERY].base_cost / 10;
    if (known[SPELL_METEOR_STORM] == SPELLSTATE_KNOWN && wizards[player].globals.meteor_storm == 0)
        ai_spell_pick_weights[10] = p_spell_data[SPELL_METEOR_STORM].base_cost / 10;
    if (known[SPELL_ARMAGEDDON] == SPELLSTATE_KNOWN && wizards[player].globals.armageddon == 0)
        ai_spell_pick_weights[11] = p_spell_data[SPELL_ARMAGEDDON].base_cost / 10;
    if (known[SPELL_JUST_CAUSE] == SPELLSTATE_KNOWN && wizards[player].globals.just_cause == 0)
        ai_spell_pick_weights[12] = p_spell_data[SPELL_JUST_CAUSE].base_cost / 10;
    if (known[SPELL_PLANAR_SEAL] == SPELLSTATE_KNOWN && wizards[player].globals.planar_seal == 0 && AI_More_Myrror_Cities_Than_Human(player))
        ai_spell_pick_weights[13] = p_spell_data[SPELL_PLANAR_SEAL].base_cost / 20;
    if (known[SPELL_HOLY_ARMS] == SPELLSTATE_KNOWN && wizards[player].globals.holy_arms == 0)
        ai_spell_pick_weights[14] = p_spell_data[SPELL_HOLY_ARMS].base_cost / 10;
    if (known[SPELL_CHARM_OF_LIFE] == SPELLSTATE_KNOWN && wizards[player].globals.charm_of_life == 0)
        ai_spell_pick_weights[15] = p_spell_data[SPELL_CHARM_OF_LIFE].base_cost / 10;
    if (known[SPELL_CRUSADE] == SPELLSTATE_KNOWN && wizards[player].globals.crusade == 0)
        ai_spell_pick_weights[16] = p_spell_data[SPELL_CRUSADE].base_cost / 10;
    if (known[SPELL_ETERNAL_NIGHT] == SPELLSTATE_KNOWN && wizards[player].globals.eternal_night == 0)
        ai_spell_pick_weights[17] = p_spell_data[SPELL_ETERNAL_NIGHT].base_cost / 10;
    if (known[SPELL_DEATH_WISH] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[18] = p_spell_data[SPELL_DEATH_WISH].base_cost / 10;
    if (known[SPELL_ZOMBIE_MASTERY] == SPELLSTATE_KNOWN && wizards[player].globals.zombie_mastery == 0)
        ai_spell_pick_weights[19] = p_spell_data[SPELL_ZOMBIE_MASTERY].base_cost / 10;
    if (known[SPELL_AWARENESS] == SPELLSTATE_KNOWN && wizards[player].globals.awareness == 0)
        ai_spell_pick_weights[20] = p_spell_data[SPELL_AWARENESS].base_cost / 10;
    if (AI_Life_Casting_Penalized() == 1) {
        for (i = 12; i <= 16; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
    }
    if (AI_Nature_Casting_Penalized() == 1) {
        for (i = 1; i <= 1; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
    }
    if (AI_Death_Casting_Penalized() == 1) {
        for (i = 17; i <= 19; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    if (AI_Chaos_Casting_Penalized() == 1) {
        for (i = 7; i <= 11; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    pick = Random_Weighted_Pick(ai_spell_pick_weights, 50);
    switch (pick) {
    case 1:
        return SPELL_HERB_MASTERY;
    case 2:
        return SPELL_WIND_MASTERY;
    case 3:
        return SPELL_AURA_OF_MAJESTY;
    case 4:
        return SPELL_GREAT_UNSUMMONING;
    case 5:
        return SPELL_SUPPRESS_MAGIC;
    case 6:
        return SPELL_SPELL_BINDING;
    case 7:
        return SPELL_GREAT_WASTING;
    case 8:
        return SPELL_CHAOS_SURGE;
    case 9:
        return SPELL_DOOM_MASTERY;
    case 10:
        return SPELL_METEOR_STORM;
    case 11:
        return SPELL_ARMAGEDDON;
    case 12:
        return SPELL_JUST_CAUSE;
    case 13:
        return SPELL_PLANAR_SEAL;
    case 14:
        return SPELL_HOLY_ARMS;
    case 15:
        return SPELL_CHARM_OF_LIFE;
    case 16:
        return SPELL_CRUSADE;
    case 17:
        return SPELL_ETERNAL_NIGHT;
    case 18:
        return SPELL_DEATH_WISH;
    case 19:
        return SPELL_ZOMBIE_MASTERY;
    case 20:
        return SPELL_AWARENESS;
    }
    return SPELL_NONE;
}

/* (player) the overland attack spell pick: 19 known ones weighted base cost / 2 .. / 10 (slot 5, Time Stop, only
   with a mana reserve of 3000+; slot 8, Raise Volcano, not under the player's own Armageddon; slot 9 unused); the
   ones aimed at a city, a map square (Corruption, Raise Volcano) or a node (Warp Node) are zeroed while the player
   has seen no city; Evil Omens cuts slots 1..2 to 2 / 3, the Death penalty cuts slots 12..20, the Chaos one 7..11
   (Corruption, slot 6, escapes it; Warp Node, slot 13, falls under the Death cut). Returns the spell or 0
   Numbered: BUG #241 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Attack_Spell(int player)
{
    char *known;
    int i;
    int pick;

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i < 50; i++)
        ai_spell_pick_weights[i] = 0;
    if (known[SPELL_ICE_STORM] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[1] = p_spell_data[SPELL_ICE_STORM].base_cost / 10;
    if (known[SPELL_EARTHQUAKE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[2] = p_spell_data[SPELL_EARTHQUAKE].base_cost / 10;
    if (known[SPELL_SPELL_BLAST] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[3] = p_spell_data[SPELL_SPELL_BLAST].base_cost / 2;
    if (known[SPELL_STASIS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[4] = p_spell_data[SPELL_STASIS].base_cost / 5;
    if (known[SPELL_TIME_STOP] == SPELLSTATE_KNOWN && wizards[player].mana_reserve >= 3000)
        ai_spell_pick_weights[5] = p_spell_data[SPELL_TIME_STOP].base_cost / 10;
    if (known[SPELL_CORRUPTION] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[6] = p_spell_data[SPELL_CORRUPTION].base_cost / 5;
    if (known[SPELL_FIRE_STORM] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[7] = p_spell_data[SPELL_FIRE_STORM].base_cost / 10;
    if (known[SPELL_RAISE_VOLCANO] == SPELLSTATE_KNOWN && wizards[player].globals.armageddon == 0)
        ai_spell_pick_weights[8] = p_spell_data[SPELL_RAISE_VOLCANO].base_cost / 10;
    if (known[SPELL_CHAOS_RIFT] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[10] = p_spell_data[SPELL_CHAOS_RIFT].base_cost / 3;
    if (known[SPELL_CALL_THE_VOID] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[11] = p_spell_data[SPELL_CALL_THE_VOID].base_cost / 2;
    if (known[SPELL_FAMINE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[12] = p_spell_data[SPELL_FAMINE].base_cost / 5;
    if (known[SPELL_WARP_NODE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[13] = p_spell_data[SPELL_WARP_NODE].base_cost / 3;
    if (known[SPELL_BLACK_WIND] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[14] = p_spell_data[SPELL_BLACK_WIND].base_cost / 10;
    if (known[SPELL_DRAIN_POWER] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[15] = p_spell_data[SPELL_DRAIN_POWER].base_cost / 5;
    if (known[SPELL_EVIL_PRESENCE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[16] = p_spell_data[SPELL_EVIL_PRESENCE].base_cost / 5;
    if (known[SPELL_SUBVERSION] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[17] = p_spell_data[SPELL_SUBVERSION].base_cost / 10;
    if (known[SPELL_PESTILENCE] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[18] = p_spell_data[SPELL_PESTILENCE].base_cost / 2;
    if (known[SPELL_CURSED_LANDS] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[19] = p_spell_data[SPELL_CURSED_LANDS].base_cost / 5;
    if (known[SPELL_CRUEL_UNMINDING] == SPELLSTATE_KNOWN)
        ai_spell_pick_weights[20] = p_spell_data[SPELL_CRUEL_UNMINDING].base_cost / 5;
    if (AI_Has_Seen_Any_City(player) == 0) {
        ai_spell_pick_weights[16] = 0;
        ai_spell_pick_weights[19] = 0;
        ai_spell_pick_weights[18] = 0;
        ai_spell_pick_weights[12] = 0;
        ai_spell_pick_weights[2] = 0;
        ai_spell_pick_weights[10] = 0;
        ai_spell_pick_weights[6] = 0;
        ai_spell_pick_weights[8] = 0;
        ai_spell_pick_weights[11] = 0;
        ai_spell_pick_weights[13] = 0;
    }
    if (AI_Nature_Casting_Penalized() == 1) {
        for (i = 1; i <= 2; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
    }
    if (AI_Death_Casting_Penalized() == 1) {
        for (i = 12; i <= 20; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    if (AI_Chaos_Casting_Penalized() == 1) {
        for (i = 7; i <= 11; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
    }
    pick = Random_Weighted_Pick(ai_spell_pick_weights, 50);
    switch (pick) {
    case 1:
        return SPELL_ICE_STORM;
    case 2:
        return SPELL_EARTHQUAKE;
    case 3:
        return SPELL_SPELL_BLAST;
    case 4:
        return SPELL_STASIS;
    case 5:
        return SPELL_TIME_STOP;
    case 6:
        return SPELL_CORRUPTION;
    case 7:
        return SPELL_FIRE_STORM;
    case 8:
        return SPELL_RAISE_VOLCANO;
    case 10:
        return SPELL_CHAOS_RIFT;
    case 11:
        return SPELL_CALL_THE_VOID;
    case 12:
        return SPELL_FAMINE;
    case 13:
        return SPELL_WARP_NODE;
    case 14:
        return SPELL_BLACK_WIND;
    case 15:
        return SPELL_DRAIN_POWER;
    case 16:
        return SPELL_EVIL_PRESENCE;
    case 17:
        return SPELL_SUBVERSION;
    case 18:
        return SPELL_PESTILENCE;
    case 19:
        return SPELL_CURSED_LANDS;
    case 20:
        return SPELL_CRUEL_UNMINDING;
    }
    return SPELL_NONE;
}

/* (player) the city spell pick: each known city spell whose target picker finds a city (the city itself is
   discarded) gets a weight: a fixed 50 .. 300 or game_turn / 6 .. / 25 (Astral Gate 0); Evil Omens cuts the Life
   and Nature ones to 2 / 3, the Death and Chaos penalties cut those realms' ones. Returns the spell or 0
   Numbered: BUG #304 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_City_Spell(int player)
{
    int i;
    char *known;
    int x;
    int y;
    int city;
    int plane;
    int ward_realm;
    int pick;

    known = wizards[player].spell_library[0] - 1;
    for (i = 0; i < 50; i++)
        ai_spell_pick_weights[i] = 0;
    if (known[SPELL_WALL_OF_STONE] == SPELLSTATE_KNOWN && AI_Target_Wall_Of_Stone(player, &city) == 1)
        ai_spell_pick_weights[1] = game_turn / 6;
    if (known[SPELL_TRANSMUTE] == SPELLSTATE_KNOWN && AI_Target_Transmute(player, &x, &y, &plane) == 1)
        ai_spell_pick_weights[2] = 200;
    if (known[SPELL_CHANGE_TERRAIN] == SPELLSTATE_KNOWN && AI_Target_Change_Terrain(player, &x, &y, &plane) == 1)
        ai_spell_pick_weights[3] = 50;
    if (known[SPELL_MOVE_FORTRESS] == SPELLSTATE_KNOWN && AI_Target_Move_Fortress(player, &city) == 1)
        ai_spell_pick_weights[5] = game_turn / 15;
    if (known[SPELL_EARTH_GATE] == SPELLSTATE_KNOWN && AI_Target_Earth_Gate(player, &city) == 1)
        ai_spell_pick_weights[6] = game_turn / 15;
    if (known[SPELL_GAIAS_BLESSING] == SPELLSTATE_KNOWN && AI_Target_Gaias_Blessing(player, &city) == 1)
        ai_spell_pick_weights[7] = 300;
    if (known[SPELL_FLYING_FORTRESS] == SPELLSTATE_KNOWN && AI_Target_Flying_Fortress(player, &city) == 1)
        ai_spell_pick_weights[8] = 100;
    if (known[SPELL_WALL_OF_FIRE] == SPELLSTATE_KNOWN && AI_Target_Wall_Of_Fire(player, &city) == 1)
        ai_spell_pick_weights[9] = game_turn / 20;
    if (known[SPELL_HEAVENLY_LIGHT] == SPELLSTATE_KNOWN && AI_Target_Heavenly_Light(player, &city) == 1)
        ai_spell_pick_weights[10] = game_turn / 25;
    if (known[SPELL_STREAM_OF_LIFE] == SPELLSTATE_KNOWN && AI_Target_Stream_Of_Life(player, &city) == 1)
        ai_spell_pick_weights[11] = game_turn / 10;
    if (known[SPELL_INSPIRATIONS] == SPELLSTATE_KNOWN && AI_Target_Inspirations(player, &city) == 1)
        ai_spell_pick_weights[12] = 300;
    if (known[SPELL_PROSPERITY] == SPELLSTATE_KNOWN && AI_Target_Prosperity(player, &city) == 1)
        ai_spell_pick_weights[13] = 300;
    if (known[SPELL_ASTRAL_GATE] == SPELLSTATE_KNOWN && AI_Target_Astral_Gate(player, &city) == 1)
        ai_spell_pick_weights[14] = 0;
    if (known[SPELL_DARK_RITUALS] == SPELLSTATE_KNOWN && AI_Target_Dark_Rituals(player, &city) == 1)
        ai_spell_pick_weights[15] = 100;
    if (known[SPELL_CLOUD_OF_SHADOW] == SPELLSTATE_KNOWN && AI_Target_Cloud_Of_Shadow(player, &city) == 1)
        ai_spell_pick_weights[16] = game_turn / 20;
    if (known[SPELL_SPELL_WARD] == SPELLSTATE_KNOWN && AI_Target_Spell_Ward(player, &city, &ward_realm) == 1)
        ai_spell_pick_weights[17] = 100;
    if (known[SPELL_CONSECRATION] == SPELLSTATE_KNOWN && AI_Target_Consecration(player, &city) == 1)
        ai_spell_pick_weights[18] = 100;
    if (known[SPELL_WALL_OF_DARKNESS] == SPELLSTATE_KNOWN && AI_Target_Wall_Of_Darkness(player, &city) == 1)
        ai_spell_pick_weights[19] = game_turn / 20;
    if (known[SPELL_ALTAR_OF_BATTLE] == SPELLSTATE_KNOWN && AI_Target_Altar_Of_Battle(player, &city) == 1)
        ai_spell_pick_weights[20] = 50;
    if (AI_Life_Casting_Penalized() == 1) {
        for (i = 10; i <= 14; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
        ai_spell_pick_weights[18] = ai_spell_pick_weights[18] * 2 / 3;
        ai_spell_pick_weights[20] = ai_spell_pick_weights[20] * 2 / 3;
    }
    if (AI_Nature_Casting_Penalized() == 1) {
        for (i = 1; i <= 7; i++)
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] * 2 / 3;
    }
    if (AI_Death_Casting_Penalized() == 1) {
        for (i = 15; i <= 16; i++) {
            if (ai_spell_pick_weights[i] < 20)
                ai_spell_pick_weights[i] = 0;
            else
                ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
        }
        i = 19;
        if (ai_spell_pick_weights[i] < 20)
            ai_spell_pick_weights[i] = 0;
        else
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
    }
    if (AI_Chaos_Casting_Penalized() == 1) {
        i = 9;
        if (ai_spell_pick_weights[i] < 20)
            ai_spell_pick_weights[i] = 0;
        else
            ai_spell_pick_weights[i] = ai_spell_pick_weights[i] < 50 ? ai_spell_pick_weights[i] / 3 : ai_spell_pick_weights[i] / 2;
    }
    pick = Random_Weighted_Pick(ai_spell_pick_weights, 50);
    switch (pick) {
    case 1:
        return SPELL_WALL_OF_STONE;
    case 2:
        return SPELL_TRANSMUTE;
    case 3:
        return SPELL_CHANGE_TERRAIN;
    case 5:
        return SPELL_MOVE_FORTRESS;
    case 6:
        return SPELL_EARTH_GATE;
    case 7:
        return SPELL_GAIAS_BLESSING;
    case 8:
        return SPELL_FLYING_FORTRESS;
    case 9:
        return SPELL_WALL_OF_FIRE;
    case 10:
        return SPELL_HEAVENLY_LIGHT;
    case 11:
        return SPELL_STREAM_OF_LIFE;
    case 12:
        return SPELL_INSPIRATIONS;
    case 13:
        return SPELL_PROSPERITY;
    case 14:
        return SPELL_ASTRAL_GATE;
    case 15:
        return SPELL_DARK_RITUALS;
    case 16:
        return SPELL_CLOUD_OF_SHADOW;
    case 17:
        return SPELL_SPELL_WARD;
    case 18:
        return SPELL_CONSECRATION;
    case 19:
        return SPELL_WALL_OF_DARKNESS;
    case 20:
        return SPELL_ALTAR_OF_BATTLE;
    }
    return SPELL_NONE;
}

/* 1 while any wizard has Evil Omens active (cuts the AI's Life weights); AI_Nature_Casting_Penalized is its twin */
int AI_Life_Casting_Penalized(void)
{
    int i;

    for (i = 0; i < num_players; i++)
        if (wizards[i].globals.evil_omens)
            return 1;
    return 0;
}

/* 1 while any wizard has Tranquility or Nature's Wrath active (cuts the AI's Chaos weights) */
int AI_Chaos_Casting_Penalized(void)
{
    int i;

    for (i = 0; i < num_players; i++) {
        if (wizards[i].globals.tranquility)
            return 1;
        if (wizards[i].globals.nature_s_wrath)
            return 1;
    }
    return 0;
}

/* 1 while any wizard has Life Force or Nature's Wrath active (cuts the AI's Death weights) */
int AI_Death_Casting_Penalized(void)
{
    int i;

    for (i = 0; i < num_players; i++) {
        if (wizards[i].globals.life_force)
            return 1;
        if (wizards[i].globals.nature_s_wrath)
            return 1;
    }
    return 0;
}

/* 1 while any wizard has Evil Omens active (cuts the AI's Nature weights); the same test as
   AI_Life_Casting_Penalized */
int AI_Nature_Casting_Penalized(void)
{
    int i;

    for (i = 0; i < num_players; i++)
        if (wizards[i].globals.evil_omens)
            return 1;
    return 0;
}

/* (player) Disenchant True if known, else Disenchant Area, else 0 */
int AI_Pick_Disenchant(int player)
{
    char *known;

    known = wizards[player].spell_library[0] - 1;
    if (known[SPELL_DISENCHANT_TRUE] == SPELLSTATE_KNOWN)
        return SPELL_DISENCHANT_TRUE;
    if (known[SPELL_DISENCHANT_AREA] == SPELLSTATE_KNOWN)
        return SPELL_DISENCHANT_AREA;
    return 0;
}

/* (player) Disjunction True if known, else Disjunction, else 0 */
int AI_Pick_Disjunction(int player)
{
    char *known;

    known = wizards[player].spell_library[0] - 1;
    if (known[SPELL_DISJUNCTION_TRUE] == SPELLSTATE_KNOWN)
        return SPELL_DISJUNCTION_TRUE;
    if (known[SPELL_DISJUNCTION] == SPELLSTATE_KNOWN)
        return SPELL_DISJUNCTION;
    return 0;
}

/* (caster, &city) the caster's most valuable city (ai_city_own_value) without City Walls; 0 when none */
int AI_Target_Wall_Of_Stone(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].buildings[BLD_CITY_WALLS] == BLDSTATE_NONE &&
            ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &x, &y, &plane) a Transmute tile: the FIRST tile (scan dy, dx from -2), in the 5x5 square of the caster's most valuable city
   (ai_city_own_value), whose mineral bits 0 or 1 are set (the & 3 test repeats the first two; any mineral number
   with those bits qualifies, gems and mithril included); 0 when none
   Numbered: BUG #142 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Transmute(int caster, int *x, int *y, int *plane)
{
    int city;
    int wx;
    int wy;
    int best_city;
    int best_value;
    int dx;
    int dy;
    int city_plane;
    int tx;
    int ty;
    int tp;

    best_city = -1;
    best_value = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == caster) {
            city_plane = p_cities[city].plane;
            for (dy = -2; dy < 3; dy++) {
                wy = dy + p_cities[city].y;
                if (wy >= 0 && wy < MAP_HEIGHT) {
                    for (dx = -2; dx < 3; dx++) {
                        wx = p_cities[city].x + dx;
                        if (wx < 0)
                            wx += MAP_WIDTH;
                        if (wx >= MAP_WIDTH)
                            wx -= MAP_WIDTH;
                        /* & 2 || & 1 || & 3: any mineral number with bit 0 or 1 set (1 .. 3, 5 .. 7, 9, ...) */
                        if ((p_mineral_maps[city_plane][wy][wx] & 2 || p_mineral_maps[city_plane][wy][wx] & 1 ||
                             p_mineral_maps[city_plane][wy][wx] & 3) && ai_city_own_value[city] > best_value) {
                            best_city = city;
                            best_value = ai_city_own_value[city];
                            tx = wx;
                            ty = wy;
                            tp = city_plane;
                        }
                    }
                }
            }
        }
    }
    if (best_city == -1)
        return 0;
    *x = tx;
    *y = ty;
    *plane = tp;
    return 1;
}

/* (caster, &x, &y, &plane) a Change Terrain tile: the FIRST desert or swamp tile (scan order) in the 5x5 square of the caster's
   most valuable city (ai_city_own_value); 0 when none */
int AI_Target_Change_Terrain(int caster, int *x, int *y, int *plane)
{
    int city;
    int wx;
    int best_city;
    int best_value;
    int dx;
    int dy;
    int city_plane;
    int wy;
    int tx;
    int ty;
    int tp;

    best_city = -1;
    best_value = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == caster) {
            city_plane = p_cities[city].plane;
            for (dy = -2; dy < 3; dy++) {
                wy = dy + p_cities[city].y;
                if (wy >= 0 && wy < MAP_HEIGHT) {
                    for (dx = -2; dx < 3; dx++) {
                        wx = p_cities[city].x + dx;
                        if (wx < 0)
                            wx += MAP_WIDTH;
                        if (wx >= MAP_WIDTH)
                            wx -= MAP_WIDTH;
                        if ((Tile_Is_Desert(wx, wy, city_plane) == 1 || Tile_Is_Swamp(wx, wy, city_plane) == 1) &&
                            ai_city_own_value[city] > best_value) {
                            best_city = city;
                            best_value = ai_city_own_value[city];
                            tx = wx;
                            ty = wy;
                            tp = city_plane;
                        }
                    }
                }
            }
        }
    }
    if (best_city == -1)
        return 0;
    *x = tx;
    *y = ty;
    *plane = tp;
    return 1;
}

/* (caster, &city) a Move Fortress target: the caster's city, other than the fortress city, with the strongest
   garrison (ai_city_garrison) above the fortress city's; 0 when none
   Numbered: BUG #304 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Move_Fortress(int caster, int *city)
{
    int i;
    int best;
    int best_garrison;
    int fortress_city;                          /* left unset when no city stands on the fortress */

    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x == p_fortresses[caster].x && p_cities[i].y == p_fortresses[caster].y &&
            p_cities[i].plane == p_fortresses[caster].plane)
            fortress_city = i;
    }
    best = -1;
    best_garrison = ai_city_garrison[fortress_city];
    for (i = 0; i < num_cities; i++) {
        if (i != fortress_city && p_cities[i].owner == caster && ai_city_garrison[i] > best_garrison) {
            best = i;
            best_garrison = ai_city_garrison[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Flying Fortress target: the fortress city, but only when it ALREADY has Flying Fortress, so
   the AI never casts the spell fresh (BUG #36); 0 otherwise
   Numbered: BUG #36 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Flying_Fortress(int caster, int *city)
{
    int i;
    int best;
    int fortress_city;                          /* left unset when no city stands on the fortress */

    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x == p_fortresses[caster].x && p_cities[i].y == p_fortresses[caster].y &&
            p_cities[i].plane == p_fortresses[caster].plane)
            fortress_city = i;
    }
    if (p_cities[fortress_city].enchants[CITY_ENCH_FLYING_FORTRESS] == 0)
        best = -1;
    else
        best = fortress_city;
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Earth Gate target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Earth_Gate(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_EARTH_GATE] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Gaia's Blessing target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Gaias_Blessing(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_GAIAS_BLESSING] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Wall of Fire target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Wall_Of_Fire(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_WALL_OF_FIRE] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Wall of Darkness target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Wall_Of_Darkness(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_WALL_OF_DARKNESS] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Heavenly Light target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Heavenly_Light(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_HEAVENLY_LIGHT] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Altar of Battle target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Altar_Of_Battle(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_ALTAR_OF_BATTLE] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Stream of Life target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Stream_Of_Life(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_STREAM_OF_LIFE] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Inspirations target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Inspirations(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_INSPIRATIONS] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Prosperity target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Prosperity(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_PROSPERITY] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Astral Gate target: the caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Astral_Gate(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_ASTRAL_GATE] == 0 && ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Dark Rituals target: the caster's most valuable city (ai_city_own_value) with a Temple and
   without Dark Rituals; 0 when none */
int AI_Target_Dark_Rituals(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_DARK_RITUALS] == 0 &&
            (p_cities[i].buildings[BLD_TEMPLE] == BLDSTATE_BUILT || p_cities[i].buildings[BLD_TEMPLE] == BLDSTATE_REPLACED) &&
            ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city) the Cloud of Shadow target: the caster's most valuable city (ai_city_own_value) without it; 0 when
   none */
int AI_Target_Cloud_Of_Shadow(int caster, int *city)
{
    int i;
    int best;
    int best_value;

    best = -1;
    best_value = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && p_cities[i].enchants[CITY_ENCH_CLOUD_OF_SHADOW] == 0 &&
            ai_city_own_value[i] > best_value) {
            best = i;
            best_value = ai_city_own_value[i];
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (caster, &city, &realm) the Spell Ward target: the caster's most valuable city (ai_city_own_value) lacking the
   ward against the HUMAN's primary realm, else against the secondary realm; for a city with both, 1 in 10 a random
   realm Random(5) - 1 (it may be a ward the city already has); 0 when none */
int AI_Target_Spell_Ward(int caster, int *city, int *realm)
{
    int i;
    int best;
    int best_value;
    int primary;
    int secondary;
    int ward_realm;

    best = -1;
    best_value = 0;
    primary = wizards[PLAYER_HUMAN].primary_realm;
    secondary = wizards[PLAYER_HUMAN].secondary_realm;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster && ai_city_own_value[i] > best_value) {
            if (primary == REALM_NATURE && p_cities[i].enchants[CITY_ENCH_NATURE_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_NATURE;
            } else if (primary == REALM_CHAOS && p_cities[i].enchants[CITY_ENCH_CHAOS_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_CHAOS;
            } else if (primary == REALM_SORCERY && p_cities[i].enchants[CITY_ENCH_SORCERY_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_SORCERY;
            } else if (primary == REALM_DEATH && p_cities[i].enchants[CITY_ENCH_DEATH_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_DEATH;
            } else if (primary == REALM_LIFE && p_cities[i].enchants[CITY_ENCH_LIFE_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_LIFE;
            } else if (secondary == REALM_NATURE && p_cities[i].enchants[CITY_ENCH_NATURE_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_NATURE;
            } else if (secondary == REALM_CHAOS && p_cities[i].enchants[CITY_ENCH_CHAOS_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_CHAOS;
            } else if (secondary == REALM_SORCERY && p_cities[i].enchants[CITY_ENCH_SORCERY_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_SORCERY;
            } else if (secondary == REALM_DEATH && p_cities[i].enchants[CITY_ENCH_DEATH_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_DEATH;
            } else if (secondary == REALM_LIFE && p_cities[i].enchants[CITY_ENCH_LIFE_WARD] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = REALM_LIFE;
            } else if (Random(10) == 1) {
                best = i;
                best_value = ai_city_own_value[i];
                ward_realm = Random(5) - 1;
            }
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    *realm = ward_realm;
    return 1;
}

/* (caster, &city) the Consecration target: only while the HUMAN's realms include Death or Chaos (else 1 in 10), the
   caster's most valuable city (ai_city_own_value) without it; 0 when none */
int AI_Target_Consecration(int caster, int *city)
{
    int i;
    int best;
    int best_value;
    int primary;
    int secondary;

    best = -1;
    best_value = 0;
    primary = wizards[PLAYER_HUMAN].primary_realm;
    secondary = wizards[PLAYER_HUMAN].secondary_realm;
    if (primary == REALM_DEATH || secondary == REALM_DEATH || primary == REALM_CHAOS || secondary == REALM_CHAOS ||
        Random(10) == 1) {
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner == caster && ai_city_own_value[i] > best_value &&
                p_cities[i].enchants[CITY_ENCH_CONSECRATION] == 0) {
                best = i;
                best_value = ai_city_own_value[i];
            }
        }
    }
    if (best == -1)
        return 0;
    *city = best;
    return 1;
}

/* (player) DEBUG leftover, no callers: prints every spell the player knows with Debug_Print, then waits for a key */
void Debug_List_Known_Spells(int player)
{
    int spell;
    int row;
    char *known;

    known = wizards[player].spell_library[0] - 1;
    row = 0;
    for (spell = 0; spell < SPELL_CREATE_ARTIFACT; spell++) {   /* not 215: ids 211 .. 214 (Create Artifact .. Spell of Return) are
                                                 never listed */
        if (known[spell] == SPELLSTATE_KNOWN) {
            far_strcpy(near_heap, 0, p_spell_data[spell].name);
            Debug_Print(20, row, near_heap);
            row++;
        }
    }
    Screen_Copy_Front();
    UI_Suspend_Redraw_Fn();
    UI_Wait_Any_Input();
}

/* (spell) "<spell> could not be found for CP.": a fatal error unless this is the release build (then 0) */
int AI_Fatal_No_Spell_Target(int spell)
{
    if (Check_Release_Version() == 1)
        return 0;
    far_strcpy(near_heap, 0, p_spell_data[spell].name);
    strcat(near_heap, " could not be found for CP.");
    Fatal_Exit(near_heap);
}

/* (player) drops the player's own self-harming globals: Eternal Night when neither the primary nor the secondary
   realm is Death, Chaos Surge when neither is Chaos, Evil Omens when either is Life or Nature, Tranquility when
   either is Chaos, Life Force when either is Death (a third realm's books do not count); all of them when
   the mana reserve is 0
   Numbered: BUG #143, #242 (BUGS_AND_QUIRKS.txt) */
void AI_Drop_Bad_Globals(int player)
{
    if (wizards[player].globals.eternal_night && wizards[player].primary_realm != REALM_DEATH && wizards[player].secondary_realm != REALM_DEATH)
        wizards[player].globals.eternal_night = 0;
    if (wizards[player].globals.evil_omens && (wizards[player].primary_realm == REALM_LIFE || wizards[player].secondary_realm == REALM_LIFE ||
        wizards[player].primary_realm == REALM_NATURE || wizards[player].secondary_realm == REALM_NATURE))
        wizards[player].globals.evil_omens = 0;
    if (wizards[player].globals.chaos_surge && wizards[player].primary_realm != REALM_CHAOS && wizards[player].secondary_realm != REALM_CHAOS)
        wizards[player].globals.chaos_surge = 0;
    if (wizards[player].globals.tranquility && (wizards[player].primary_realm == REALM_CHAOS || wizards[player].secondary_realm == REALM_CHAOS))
        wizards[player].globals.tranquility = 0;
    if (wizards[player].globals.life_force && (wizards[player].primary_realm == REALM_DEATH || wizards[player].secondary_realm == REALM_DEATH))
        wizards[player].globals.life_force = 0;
    if (wizards[player].mana_reserve == 0) {
        wizards[player].globals.eternal_night = 0;
        wizards[player].globals.evil_omens = 0;
        wizards[player].globals.zombie_mastery = 0;
        wizards[player].globals.aura_of_majesty = 0;
        wizards[player].globals.wind_mastery = 0;
        wizards[player].globals.suppress_magic = 0;
        wizards[player].globals.time_stop = 0;
        wizards[player].globals.nature_awareness = 0;
        wizards[player].globals.nature_s_wrath = 0;
        wizards[player].globals.herb_mastery = 0;
        wizards[player].globals.chaos_surge = 0;
        wizards[player].globals.doom_mastery = 0;
        wizards[player].globals.great_wasting = 0;
        wizards[player].globals.meteor_storm = 0;
        wizards[player].globals.armageddon = 0;
        wizards[player].globals.tranquility = 0;
        wizards[player].globals.life_force = 0;
        wizards[player].globals.crusade = 0;
        wizards[player].globals.just_cause = 0;
        wizards[player].globals.holy_arms = 0;
        wizards[player].globals.planar_seal = 0;
        wizards[player].globals.charm_of_life = 0;
        wizards[player].globals.detect_magic = 0;
        wizards[player].globals.awareness = 0;
    }
}

/* (target_kind, &city, spell, caster) the city target of a city spell: target_kind MAP_TARGET_OWN_CITY = an own-city enchantment, dispatched to its
   AI_Target_* picker; kind MAP_TARGET_ENEMY_CITY = a curse on an enemy city: the highest ai_city_target_value among non-neutral cities
   the caster has seen, without Consecration, without the Chaos Ward (Earthquake, Chaos Rift, Call the Void - the
   Earthquake test is for Chaos, not Nature) or the Death Ward (Evil Presence, Cursed Lands, Pestilence, Famine),
   Evil Presence only at Death wizards, and not already cursed by the same spell (no owner test: the caster's
   own cities and those of wizards at peace with it score 0 in ai_city_target_value). Returns 1 when found, 0 when not;
   an unknown spell or kind is a fatal error outside the release build
   Numbered: BUG #112, #113, #304 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_City_Spell_Target(int target_kind, int *city, int spell, int caster)
{
    int death_curse;
    int already_cursed;
    int i;
    int best;
    int best_value;

    if (target_kind == MAP_TARGET_OWN_CITY) {
        switch (spell) {
        case SPELL_WALL_OF_DARKNESS:
            return AI_Target_Wall_Of_Darkness(caster, city);
        case SPELL_WALL_OF_STONE:
            return AI_Target_Wall_Of_Stone(caster, city);
        case SPELL_MOVE_FORTRESS:
            return AI_Target_Move_Fortress(caster, city);
        case SPELL_EARTH_GATE:
            return AI_Target_Earth_Gate(caster, city);
        case SPELL_FLYING_FORTRESS:
            return AI_Target_Flying_Fortress(caster, city);
        case SPELL_WALL_OF_FIRE:
            return AI_Target_Wall_Of_Fire(caster, city);
        case SPELL_HEAVENLY_LIGHT:
            return AI_Target_Heavenly_Light(caster, city);
        case SPELL_ALTAR_OF_BATTLE:
            return AI_Target_Altar_Of_Battle(caster, city);
        case SPELL_INSPIRATIONS:
            return AI_Target_Inspirations(caster, city);
        case SPELL_STREAM_OF_LIFE:
            return AI_Target_Stream_Of_Life(caster, city);
        case SPELL_ASTRAL_GATE:
            return AI_Target_Astral_Gate(caster, city);
        case SPELL_PROSPERITY:
            return AI_Target_Prosperity(caster, city);
        case SPELL_CONSECRATION:
            return AI_Target_Consecration(caster, city);
        case SPELL_CLOUD_OF_SHADOW:
            return AI_Target_Cloud_Of_Shadow(caster, city);
        case SPELL_SUMMONING_CIRCLE:
            return AI_Target_Summoning_Circle(caster, city);
        case SPELL_DARK_RITUALS:
            return AI_Target_Dark_Rituals(caster, city);
        case SPELL_GAIAS_BLESSING:
            return AI_Target_Gaias_Blessing(caster, city);
        }
        AI_Fatal_No_Spell_Target(spell);
    } else if (target_kind == MAP_TARGET_ENEMY_CITY) {
        if (spell == SPELL_EARTHQUAKE || spell == SPELL_CHAOS_RIFT || spell == SPELL_CALL_THE_VOID)
            death_curse = 0;
        else if (spell == SPELL_EVIL_PRESENCE || spell == SPELL_CURSED_LANDS || spell == SPELL_PESTILENCE || spell == SPELL_FAMINE)
            death_curse = 1;
        else
            AI_Fatal_No_Spell_Target(spell);
        best = -1;
        best_value = 0;
        for (i = 0; i < num_cities; i++) {
            if (ai_city_target_value[i] > best_value && p_cities[i].owner != PLAYER_NEUTRAL &&
                p_cities[i].enchants[CITY_ENCH_CONSECRATION] == 0 &&
                ((death_curse == 0 && p_cities[i].enchants[CITY_ENCH_CHAOS_WARD] == 0) ||
                 (death_curse == 1 && p_cities[i].enchants[CITY_ENCH_DEATH_WARD] == 0)) &&
                (spell != SPELL_EVIL_PRESENCE || wizards[p_cities[i].owner].spellbooks[REALM_DEATH] != 0) &&
                Bit_Test(caster, &p_cities[i].seen_by)) {
                already_cursed = 0;
                switch (spell) {
                case SPELL_EARTHQUAKE:
                    break;
                case SPELL_CHAOS_RIFT:
                    if (p_cities[i].enchants[CITY_ENCH_CHAOS_RIFT])
                        already_cursed = 1;
                    break;
                case SPELL_CALL_THE_VOID:
                    break;
                case SPELL_EVIL_PRESENCE:
                    if (p_cities[i].enchants[CITY_ENCH_EVIL_PRESENCE])
                        already_cursed = 1;
                    break;
                case SPELL_CURSED_LANDS:
                    if (p_cities[i].enchants[CITY_ENCH_CURSED_LANDS])
                        already_cursed = 1;
                    break;
                case SPELL_PESTILENCE:
                    if (p_cities[i].enchants[CITY_ENCH_PESTILENCE])
                        already_cursed = 1;
                    break;
                case SPELL_FAMINE:
                    if (p_cities[i].enchants[CITY_ENCH_FAMINE])
                        already_cursed = 1;
                    break;
                }
                if (already_cursed == 0) {
                    best_value = ai_city_target_value[i];
                    best = i;
                }
            }
        }
        if (best == -1)
            return 0;
        *city = best;
        return 1;
    } else {
        AI_Fatal_No_Spell_Target(spell);
    }
}

/* (player) 1 when any city's seen_by has the player's bit (else AI_Pick_Attack_Spell zeroes its city attacks) */
int AI_Has_Seen_Any_City(int player)
{
    int city;

    for (city = 0; city < num_cities; city++)
        if (Bit_Test(player, &p_cities[city].seen_by))
            return 1;
    return 0;
}

/* (target_kind, &unit, spell, caster) the unit target of a unit enchantment (target_kind MAP_TARGET_OWN_UNIT only): per spell, the Unit.enchantments
   bits that must not be set yet plus the filters (normal units only / summons only / not the basic types / the
   flight mode) for AI_Pick_Unit_For_Enchant; spell 51 never (returns 0); an unknown spell or kind is a fatal
   error outside the release build (Word of Recall, a known unit enchantment, is not in the list)
   Numbered: BUG #115 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Unit_Enchant_Target(int target_kind, int *unit, int spell, int caster)
{
    if (target_kind == MAP_TARGET_OWN_UNIT) {
        switch (spell) {                        /* the flags: normal_only, summons_only, no_basic, movement_spell, unused */
        case SPELL_RESIST_ELEMENTS:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_RESIST_ELEMENTS, unit, 0, 0, 0, 0, 0);
        case SPELL_STONE_SKIN:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_STONE_SKIN, unit, 0, 0, 0, 0, 0);
        case SPELL_WATER_WALKING:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_WATER_WALKING, unit, 0, 0, 0, 1, 0);
        case SPELL_PATH_FINDING:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_PATH_FINDING, unit, 0, 0, 0, 0, 0);
        case SPELL_ELEMENTAL_ARMOR:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_ELEMENTAL_ARMOR, unit, 0, 0, 0, 0, 0);
        case SPELL_IRON_SKIN:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_IRON_SKIN, unit, 0, 0, 0, 0, 0);
        case SPELL_REGENERATION:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_REGENERATION, unit, 0, 0, 0, 0, 0);
        case SPELL_RESIST_MAGIC:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_RESIST_MAGIC, unit, 0, 0, 0, 0, 0);
        case SPELL_BLUR:
            return 0;
        case SPELL_FLIGHT:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_FLIGHT, unit, 0, 0, 0, 1, 0);
        case SPELL_SPELL_LOCK:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_SPELL_LOCK, unit, 0, 1, 0, 0, 0);
        case SPELL_WIND_WALKING:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_WIND_WALKING, unit, 0, 0, 0, 0, 0);
        case SPELL_MAGIC_IMMUNITY:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_MAGIC_IMMUNITY, unit, 0, 0, 0, 0, 0);
        case SPELL_INVISIBILITY:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_INVISIBILITY, unit, 0, 0, 0, 0, 0);
        case SPELL_CHAOS_CHANNELS:
            return AI_Pick_Unit_For_Enchant(caster, 0x0L, unit, 1, 0, 0, 0, 0);
        case SPELL_ELDRITCH_WEAPON:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_ELDRITCH_WEAPON, unit, 1, 0, 0, 0, 0);
        case SPELL_IMMOLATION:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_IMMOLATION, unit, 0, 0, 0, 0, 0);
        case SPELL_HOLY_WEAPON:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_HOLY_WEAPON, unit, 1, 0, 0, 0, 0);
        case SPELL_GIANT_STRENGTH:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_GIANT_STRENGTH, unit, 1, 0, 0, 0, 0);
        case SPELL_BLESS:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_BLESS, unit, 0, 0, 0, 0, 0);
        case SPELL_ENDURANCE:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_ENDURANCE, unit, 0, 0, 0, 0, 0);
        case SPELL_HOLY_ARMOR:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_HOLY_ARMOR, unit, 0, 0, 0, 0, 0);
        case SPELL_HEROISM:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_HEROISM, unit, 0, 0, 0, 0, 0);
        case SPELL_TRUE_SIGHT:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_TRUE_SIGHT, unit, 0, 0, 0, 0, 0);
        case SPELL_INVULNERABILITY:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_INVULNERABILITY, unit, 0, 0, 0, 0, 0);
        case SPELL_PLANAR_TRAVEL:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_PLANAR_TRAVEL, unit, 0, 0, 0, 1, 0);
        case SPELL_LIONHEART:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_LIONHEART, unit, 0, 0, 0, 0, 0);
        case SPELL_RIGHTEOUSNESS:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_RIGHTEOUSNESS, unit, 0, 0, 0, 0, 0);
        case SPELL_CLOAK_OF_FEAR:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_CLOAK_OF_FEAR, unit, 0, 0, 0, 0, 0);
        case SPELL_LYCANTHROPY:
            return AI_Pick_Unit_For_Enchant(caster, 0x0L, unit, 1, 0, 1, 0, 0);
        case SPELL_BLACK_CHANNELS:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_BLACK_CHANNELS, unit, 0, 0, 0, 0, 0);
        case SPELL_WRAITH_FORM:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_WRAITH_FORM, unit, 0, 0, 0, 0, 0);
        case SPELL_FLAME_BLADE:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_FLAME_BLADE, unit, 1, 0, 0, 0, 0);
        case SPELL_GUARDIAN_WIND:
            return AI_Pick_Unit_For_Enchant(caster, UNIT_ENCH_GUARDIAN_WIND, unit, 0, 0, 0, 0, 0);
        default:
            AI_Fatal_No_Spell_Target(spell);
        }
    } else {
        AI_Fatal_No_Spell_Target(spell);
    }
    return 0;
}

/* (caster, enchants, &unit, normal_only, summons_only, no_basic, movement_spell, unused) the unit for a unit
   enchantment: the caster's strongest stack outside its cities (else the stack at its fortress - BUG #421:
   with none there either p_ai_own_stacks[-1] is read), then the unit there
   with the highest upkeep that is not a transport, settler (AFLAG_CREATE_OUTPOST) or melder (AFLAG_MELD_WITH_NODE),
   lacks all of the enchants
   bits, and passes the filters (normal_only: below the summons; summons_only: the summons; no_basic: types above
   40; Floating Islands only with movement_spell). With movement_spell flying / swimming units weigh 0, and the x 20 for
   settlers is dead code (they are filtered out first). The last parameter is never read. Returns 1 when found */
int AI_Pick_Unit_For_Enchant(int caster, long enchants, int *unit, int normal_only, int summons_only, int no_basic,
                             int movement_spell, int unused)
{
    int i;
    int city;
    int upkeep;
    int best_unit;
    int unit_plane;
    int best_value;
    int in_city;
    int best_stack;

    best_stack = -1;
    best_value = 0;
    for (i = 0; i < ai_own_stack_count; i++) {
        in_city = 0;
        for (city = 0; city < num_cities && in_city == 0; city++) {
            if ((int)p_ai_own_stacks[i].x == p_cities[city].x && (int)p_ai_own_stacks[i].y == p_cities[city].y &&
                (int)p_ai_own_stacks[i].plane == p_cities[city].plane)
                in_city = 1;
        }
        if (in_city == 0 && p_ai_own_stacks[i].strength > best_value) {
            best_stack = i;
            best_value = p_ai_own_stacks[i].strength;
        }
    }
    if (best_stack == -1) {
        for (i = 0; i < ai_own_stack_count; i++) {
            if ((int)p_ai_own_stacks[i].x == p_fortresses[caster].x && (int)p_ai_own_stacks[i].y == p_fortresses[caster].y &&
                (int)p_ai_own_stacks[i].plane == p_fortresses[caster].plane)
                best_stack = i;
        }
    }
    best_unit = -1;
    best_value = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].x == p_ai_own_stacks[best_stack].x && p_units[i].y == p_ai_own_stacks[best_stack].y &&
            p_units[i].plane == p_ai_own_stacks[best_stack].plane &&
            (normal_only == 0 || p_units[i].type < UNIT_MAGIC_SPIRIT) &&
            (summons_only == 0 || p_units[i].type >= UNIT_MAGIC_SPIRIT) &&
            unit_types[p_units[i].type].transport_capacity == 0 &&
            (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 &&
            (unit_types[p_units[i].type].ability_flags & AFLAG_MELD_WITH_NODE) == 0 &&
            (no_basic == 0 || p_units[i].type > UNIT_BARBARIAN_SWORDSMEN) &&
            (p_units[i].enchantments & enchants) == 0 &&
            (movement_spell == 1 || p_units[i].type != UNIT_FLOATING_ISLAND)) {
            upkeep = unit_types[p_units[i].type].upkeep;
            unit_plane = p_units[i].plane;     /* never read */
            if (movement_spell == 1) {
                if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST)
                    upkeep *= 20;
                if (unit_types[p_units[i].type].move_flags & MOVE_FLYING || unit_types[p_units[i].type].move_flags & MOVE_SWIMMING)
                    upkeep = 0;
            }
            if (upkeep > best_value) {
                best_unit = i;
                best_value = upkeep;
            }
        }
    }
    if (best_unit == -1)
        return 0;
    *unit = best_unit;
    return 1;
}

/* (&node, spell) the Warp Node target (spell 186 only, and only with a hostile wizard): the victim is the human
   half the time, else Random(ai_hostile_count) used as a wizard number (not as an index into ai_hostile_wizards);
   then the victim's most powerful node that is ALREADY warped (meld_flags bit 0) - so the AI never warps a node
   fresh (BUG #114). Returns 1 when found
   Numbered: BUG #114 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Warp_Node(int *node, int spell)
{
    int i;
    int best;
    int victim;
    int best_power;

    if (spell != SPELL_WARP_NODE || ai_hostile_count == 0)
        return 0;
    if (Random(2) == 1)
        victim = PLAYER_HUMAN;
    else
        victim = Random(ai_hostile_count);
    best = -1;
    best_power = 0;
    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].owner == victim && p_nodes[i].power > best_power && (p_nodes[i].meld_flags & NODE_WARPED) == 1) {
            best_power = p_nodes[i].power;
            best = i;
        }
    }
    if (best == -1)
        return 0;
    *node = best;
    return 1;
}

/* (&wizard, &slot, spell, caster) the global enchantment to remove with Spell Binding (76) or Disjunction /
   Disjunction True (210 / 61): a priority per global slot of every other wizard (higher for the ones that hurt the
   caster's realms), the highest wins. The priorities 200 / 250 are stored in signed chars (-56 / -6) and lose every
   comparison (BUG #116). Returns 1 when found; any other spell is a fatal error outside the release build
   Numbered: BUG #116 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Global_Enchant_Target(int *wizard, int *slot, int spell, int caster)
{
    int wiz;
    int i;
    int k;
    int best_slot;
    int best_wizard;
    int best;
    char priority[MAX_PLAYERS][NUM_GLOBAL_ENCHANTS + 1];   /* a row per wizard; the 25th slot stays 0 */

    for (i = 0; i < MAX_PLAYERS; i++)
        for (k = 0; k < NUM_GLOBAL_ENCHANTS + 1; k++)
            priority[i][k] = 0;
    wiz = 0;
    if (spell == SPELL_SPELL_BINDING) {
        for (i = 0; i < num_players; i++) {
            if (i != caster) {
                wiz = i;
                if (wizards[wiz].globals.eternal_night) {
                    if (wizards[caster].primary_realm == REALM_DEATH || wizards[caster].secondary_realm == REALM_DEATH)
                        priority[wiz][GLOBAL_ENCH_ETERNAL_NIGHT] = 30;
                    else
                        priority[wiz][GLOBAL_ENCH_ETERNAL_NIGHT] = 100;
                }
                if (wizards[wiz].globals.evil_omens)
                    priority[wiz][GLOBAL_ENCH_EVIL_OMENS] = 10;
                if (wizards[wiz].globals.zombie_mastery)
                    priority[wiz][GLOBAL_ENCH_ZOMBIE_MASTERY] = 50;
                if (wizards[wiz].globals.aura_of_majesty)
                    priority[wiz][GLOBAL_ENCH_AURA_OF_MAJESTY] = 20;
                if (wizards[wiz].globals.wind_mastery)
                    priority[wiz][GLOBAL_ENCH_WIND_MASTERY] = 15;
                if (wizards[wiz].globals.suppress_magic)
                    priority[wiz][GLOBAL_ENCH_SUPPRESS_MAGIC] = 100;
                if (wizards[wiz].globals.time_stop)
                    priority[wiz][GLOBAL_ENCH_TIME_STOP] = 10;
                if (wizards[wiz].globals.nature_awareness)
                    priority[wiz][GLOBAL_ENCH_NATURE_AWARENESS] = 10;
                if (wizards[wiz].globals.nature_s_wrath)
                    priority[wiz][GLOBAL_ENCH_NATURES_WRATH] = 20;
                if (wizards[wiz].globals.herb_mastery)
                    priority[wiz][GLOBAL_ENCH_HERB_MASTERY] = 20;
                if (wizards[wiz].globals.chaos_surge) {
                    if (wizards[caster].primary_realm == REALM_CHAOS || wizards[caster].secondary_realm == REALM_CHAOS)
                        priority[wiz][GLOBAL_ENCH_CHAOS_SURGE] = 30;
                    else
                        priority[wiz][GLOBAL_ENCH_CHAOS_SURGE] = 100;
                }
                if (wizards[wiz].globals.doom_mastery)
                    priority[wiz][GLOBAL_ENCH_DOOM_MASTERY] = 50;
                if (wizards[wiz].globals.great_wasting)
                    priority[wiz][GLOBAL_ENCH_GREAT_WASTING] = 20;
                if (wizards[wiz].globals.meteor_storm)
                    priority[wiz][GLOBAL_ENCH_METEOR_STORM] = 30;
                if (wizards[wiz].globals.armageddon)
                    priority[wiz][GLOBAL_ENCH_ARMAGEDDON] = 40;
                if (wizards[wiz].globals.tranquility)
                    priority[wiz][GLOBAL_ENCH_TRANQUILITY] = 30;
                if (wizards[wiz].globals.life_force)
                    priority[wiz][GLOBAL_ENCH_LIFE_FORCE] = 30;
                if (wizards[wiz].globals.crusade)
                    priority[wiz][GLOBAL_ENCH_CRUSADE] = 20;
                if (wizards[wiz].globals.just_cause)
                    priority[wiz][GLOBAL_ENCH_JUST_CAUSE] = 15;
                if (wizards[wiz].globals.holy_arms)
                    priority[wiz][GLOBAL_ENCH_HOLY_ARMS] = 30;
                if (wizards[wiz].globals.planar_seal)
                    priority[wiz][GLOBAL_ENCH_PLANAR_SEAL] = 10;
                if (wizards[wiz].globals.charm_of_life)
                    priority[wiz][GLOBAL_ENCH_CHARM_OF_LIFE] = 60;
                if (wizards[wiz].globals.detect_magic)
                    priority[wiz][GLOBAL_ENCH_DETECT_MAGIC] = 1;
                if (wizards[wiz].globals.awareness)
                    priority[wiz][GLOBAL_ENCH_AWARENESS] = 1;
            }
        }
        best_slot = -1;
        best_wizard = -1;
        best = 0;
        for (wiz = 0; wiz < MAX_PLAYERS; wiz++) {   /* all six; the Disjunction branch scans num_players */
            for (i = 0; i < NUM_GLOBAL_ENCHANTS + 1; i++) {
                if (priority[wiz][i] > best) {
                    best_wizard = wiz;
                    best = priority[wiz][i];
                    best_slot = i;
                }
            }
        }
        if (best_slot == -1)
            return 0;
        *wizard = best_wizard;
        *slot = best_slot;
        return 1;
    } else if (spell == SPELL_DISJUNCTION || spell == SPELL_DISJUNCTION_TRUE) {
        for (i = 0; i < num_players; i++) {
            if (i != caster) {
                wiz = i;
                if (wizards[wiz].globals.eternal_night) {
                    if (wizards[caster].primary_realm == REALM_LIFE || wizards[caster].secondary_realm == REALM_LIFE)
                        priority[wiz][GLOBAL_ENCH_ETERNAL_NIGHT] = 200;
                    else
                        priority[wiz][GLOBAL_ENCH_ETERNAL_NIGHT] = 50;
                }
                if (wizards[wiz].globals.evil_omens) {
                    if (wizards[caster].primary_realm == REALM_LIFE || wizards[caster].secondary_realm == REALM_LIFE || wizards[caster].primary_realm == REALM_NATURE || wizards[caster].secondary_realm == REALM_NATURE)
                        priority[wiz][GLOBAL_ENCH_EVIL_OMENS] = 200;
                    else
                        priority[wiz][GLOBAL_ENCH_EVIL_OMENS] = 0;
                }
                if (wizards[wiz].globals.zombie_mastery)
                    priority[wiz][GLOBAL_ENCH_ZOMBIE_MASTERY] = 50;
                if (wizards[wiz].globals.aura_of_majesty)
                    priority[wiz][GLOBAL_ENCH_AURA_OF_MAJESTY] = 20;
                if (wizards[wiz].globals.wind_mastery)
                    priority[wiz][GLOBAL_ENCH_WIND_MASTERY] = 15;
                if (wizards[wiz].globals.suppress_magic)
                    priority[wiz][GLOBAL_ENCH_SUPPRESS_MAGIC] = 250;
                if (wizards[wiz].globals.time_stop)
                    priority[wiz][GLOBAL_ENCH_TIME_STOP] = 10;
                if (wizards[wiz].globals.nature_awareness)
                    priority[wiz][GLOBAL_ENCH_NATURE_AWARENESS] = 10;
                if (wizards[wiz].globals.nature_s_wrath) {
                    if (wizards[caster].primary_realm == REALM_DEATH || wizards[caster].secondary_realm == REALM_DEATH || wizards[caster].primary_realm == REALM_CHAOS || wizards[caster].secondary_realm == REALM_CHAOS)
                        priority[wiz][GLOBAL_ENCH_NATURES_WRATH] = 200;
                    else
                        priority[wiz][GLOBAL_ENCH_NATURES_WRATH] = 0;
                }
                if (wizards[wiz].globals.herb_mastery) {
                    if (wizards[caster].primary_realm == REALM_DEATH)
                        priority[wiz][GLOBAL_ENCH_HERB_MASTERY] = 0;
                    else
                        priority[wiz][GLOBAL_ENCH_HERB_MASTERY] = 50;
                }
                if (wizards[wiz].globals.chaos_surge) {
                    if (wizards[caster].primary_realm == REALM_CHAOS || wizards[caster].secondary_realm == REALM_CHAOS)
                        priority[wiz][GLOBAL_ENCH_CHAOS_SURGE] = 0;
                    else
                        priority[wiz][GLOBAL_ENCH_CHAOS_SURGE] = 100;
                }
                if (wizards[wiz].globals.doom_mastery)
                    priority[wiz][GLOBAL_ENCH_DOOM_MASTERY] = 50;
                if (wizards[wiz].globals.great_wasting)
                    priority[wiz][GLOBAL_ENCH_GREAT_WASTING] = 20;
                if (wizards[wiz].globals.meteor_storm)
                    priority[wiz][GLOBAL_ENCH_METEOR_STORM] = 30;
                if (wizards[wiz].globals.armageddon)
                    priority[wiz][GLOBAL_ENCH_ARMAGEDDON] = 40;
                if (wizards[wiz].globals.tranquility) {
                    if (wizards[caster].primary_realm == REALM_CHAOS || wizards[caster].secondary_realm == REALM_CHAOS)
                        priority[wiz][GLOBAL_ENCH_TRANQUILITY] = 200;
                    else
                        priority[wiz][GLOBAL_ENCH_TRANQUILITY] = 0;
                }
                if (wizards[wiz].globals.life_force) {
                    if (wizards[caster].primary_realm == REALM_CHAOS || wizards[caster].secondary_realm == REALM_CHAOS)
                        priority[wiz][GLOBAL_ENCH_LIFE_FORCE] = 200;
                    else
                        priority[wiz][GLOBAL_ENCH_LIFE_FORCE] = 0;
                }
                if (wizards[wiz].globals.crusade)
                    priority[wiz][GLOBAL_ENCH_CRUSADE] = 20;
                if (wizards[wiz].globals.just_cause)
                    priority[wiz][GLOBAL_ENCH_JUST_CAUSE] = 15;
                if (wizards[wiz].globals.holy_arms)
                    priority[wiz][GLOBAL_ENCH_HOLY_ARMS] = 30;
                if (wizards[wiz].globals.planar_seal)
                    priority[wiz][GLOBAL_ENCH_PLANAR_SEAL] = 10;
                if (wizards[wiz].globals.charm_of_life)
                    priority[wiz][GLOBAL_ENCH_CHARM_OF_LIFE] = 60;
                if (wizards[wiz].globals.detect_magic)
                    priority[wiz][GLOBAL_ENCH_DETECT_MAGIC] = 1;
                if (wizards[wiz].globals.awareness)
                    priority[wiz][GLOBAL_ENCH_AWARENESS] = 1;
            }
        }
        best_slot = -1;
        best_wizard = -1;
        best = 0;
        for (wiz = 0; wiz < num_players; wiz++) {
            for (i = 0; i < 25; i++) {
                if (priority[wiz][i] > best) {
                    best_wizard = wiz;
                    best = priority[wiz][i];
                    best_slot = i;
                }
            }
        }
        if (best_slot == -1)
            return 0;
        *wizard = best_wizard;
        *slot = best_slot;
        return 1;
    }
    AI_Fatal_No_Spell_Target(spell);
}

/* (target_kind, &x, &y, &plane, spell, caster) the map square target of an overland spell, dispatched by spell to its
   AI_Target_* picker (Transmute, Change Terrain, Ice Storm / Fire Storm / Black Wind / Stasis, Floating Island,
   Enchant Road, Disenchant True / Area, Corruption / Raise Volcano, Plane Shift, Nature's Cures); returns its found
   flag. Any other spell is a fatal error outside the release build. kind is unused (only copied to a local) */
int AI_Overland_Spell_Target(int target_kind, int *x, int *y, int *plane, int spell, int caster)
{
    int unused_kind;

    if (spell == SPELL_TRANSMUTE)
        return AI_Target_Transmute(caster, x, y, plane);
    else if (spell == SPELL_CHANGE_TERRAIN)
        return AI_Target_Change_Terrain(caster, x, y, plane);
    else if (spell == SPELL_ICE_STORM || spell == SPELL_FIRE_STORM || spell == SPELL_BLACK_WIND || spell == SPELL_STASIS)
        return AI_Target_Area_Damage(caster, x, y, plane);
    else if (spell == SPELL_FLOATING_ISLAND)
        return AI_Target_Floating_Island(caster, x, y, plane);
    else if (spell == SPELL_ENCHANT_ROAD)
        return AI_Target_Enchant_Road(caster, x, y, plane);
    else if (spell == SPELL_DISENCHANT_TRUE || spell == SPELL_DISENCHANT_AREA)
        return AI_Target_Disenchant(caster, x, y, plane);
    else if (spell == SPELL_CORRUPTION || spell == SPELL_RAISE_VOLCANO)
        return AI_Target_Corruption_Volcano(caster, x, y, plane, spell);
    else if (spell == SPELL_PLANE_SHIFT)
        return AI_Target_Plane_Shift(caster, x, y, plane);
    else if (spell == SPELL_NATURES_CURES)
        return AI_Target_Natures_Cures(caster, x, y, plane);
    else
        AI_Fatal_No_Spell_Target(spell);
    unused_kind = target_kind;
}

/* (caster, &x, &y, &plane) the area damage target (Ice Storm / Fire Storm / Black Wind / Stasis): the strongest enemy
   stack within 2 of one of the caster's cities (3 with City Walls, 5 with an Oracle) or within 2 of one of its
   units. BUG #110: the distance is measured to city / unit number STACK (the stack index) instead of the loop's city /
   unit. Returns 1 when found
   Numbered: BUG #110 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Area_Damage(int caster, int *x, int *y, int *plane)
{
    int stack;
    int j;
    int best_x;
    int best_y;
    int best_plane;
    unsigned best_strength;
    int radius;
    int found;
    int sx;
    int sy;
    int sp;

    best_x = -1;
    best_strength = 0;
    for (stack = 0; stack < ai_enemy_stack_count; stack++) {
        if (p_ai_enemy_stacks[stack].strength > best_strength) {
            found = 0;
            sx = p_ai_enemy_stacks[stack].x;
            sy = p_ai_enemy_stacks[stack].y;
            sp = p_ai_enemy_stacks[stack].plane;
            for (j = 0; j < num_cities && found == 0; j++) {
                if (p_cities[j].owner == caster && p_cities[j].plane == sp) {
                    radius = 2;
                    if (p_cities[j].buildings[BLD_CITY_WALLS] == BLDSTATE_BUILT)
                        radius = 3;
                    if (p_cities[j].buildings[BLD_ORACLE] == BLDSTATE_BUILT)
                        radius = 5;
                    if (Map_Distance(sx, sy, p_cities[stack].x, p_cities[stack].y, MAP_WIDTH) <= radius) {
                        best_strength = p_ai_enemy_stacks[stack].strength;
                        best_x = sx;
                        best_y = sy;
                        best_plane = sp;
                        found = 1;
                    }
                }
            }
            if (found == 0) {
                radius = 2;
                for (j = 0; j < num_units && found == 0; j++) {
                    if (p_units[j].owner == caster && p_units[j].plane == sp &&
                        Map_Distance(sx, sy, p_units[stack].x, p_units[stack].y, MAP_WIDTH) <= radius) {
                        best_strength = p_ai_enemy_stacks[stack].strength;
                        best_x = sx;
                        best_y = sy;
                        best_plane = sp;
                        found = 1;
                    }
                }
            }
        }
    }
    if (best_x == -1)
        return 0;
    *x = best_x;
    *y = best_y;
    *plane = best_plane;
    return 1;
}

/* (caster, &x, &y, &plane) the Floating Island target: a random city of the caster (200 tries), then the nearest
   square that passes AI_Tile_Floating_Island_OK, tried in the order west (-d, 0), north (0, -d), then the distance d
   grows, east (+d, 0), south (0, +d), west, north, ... up to d = 9 (the first two at d = 0 are the city square).
   Returns 1 when found */
int AI_Target_Floating_Island(int caster, int *x, int *y, int *plane)
{
    int dist;
    int found;
    int cx;
    int cy;
    int west_north;                             /* 1: the west / north side (-d) */
    int cp;
    int second;                                 /* 0 probes along x, 1 along y */
    int best_x;
    int best_y;
    int player;
    int tries;
    int city;

    player = caster;                            /* never read */
    found = 0;
    for (tries = 0; tries < 200 && found == 0; tries++) {
        city = Random(num_cities) - 1;
        if (p_cities[city].owner == caster) {
            found = 1;
            cp = p_cities[city].plane;
            cx = p_cities[city].x;
            cy = p_cities[city].y;
        }
    }
    if (found == 0)
        return 0;
    dist = 0;
    west_north = 1;
    second = 0;
    found = 0;
    while (found == 0 && dist < 10) {
        if (west_north == 0) {
            if (second == 0) {
                if (AI_Tile_Floating_Island_OK(cx + dist, cy, cp) == 1) {
                    found = 1;
                    best_x = cx + dist;
                    best_y = cy;
                }
                second = 1;
            } else {
                if (AI_Tile_Floating_Island_OK(cx, cy + dist, cp) == 1) {
                    found = 1;
                    best_x = cx;
                    best_y = cy + dist;
                }
                second = 0;
                west_north = 1;
            }
        } else {
            if (second == 0) {
                if (AI_Tile_Floating_Island_OK(cx - dist, cy, cp) == 1) {
                    found = 1;
                    best_x = cx - dist;
                    best_y = cy;
                }
                second = 1;
            } else {
                if (AI_Tile_Floating_Island_OK(cx, cy - dist, cp) == 1) {
                    found = 1;
                    best_x = cx;
                    best_y = cy - dist;
                }
                second = 0;
                dist++;
                west_north = 0;
            }
        }
    }
    if (found == 0)
        return 0;
    *x = best_x;
    *y = best_y;
    *plane = cp;
    return 1;
}

/* (caster, &x, &y, &plane) the Enchant Road target: the city with the largest garrison (ai_city_garrison: only
   the caster's own units count, so only its own cities, though there is no owner test) whose
   area has a road (terrain flag 8) and no enchanted road (0x10). BUG #111: the 5x5 loop never adds its offsets, so only
   the city square itself is tested (24 times). Returns 1 when found
   Numbered: BUG #111 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Enchant_Road(int caster, int *x, int *y, int *plane)
{
    int city;
    int best;
    int dx;
    int dy;
    int garrison;
    int best_value;
    int has_road;
    int enchanted;
    int player;

    player = caster;                            /* never read */
    best = -1;
    best_value = 0;
    for (city = 0; city < num_cities; city++) {
        garrison = ai_city_garrison[city];
        if (garrison > best_value) {
            has_road = 0;
            enchanted = 0;
            for (dy = -2; dy < 3; dy++) {
                for (dx = -2; dx < 3; dx++) {
                    if (dx != 0 || dy != 0) {
                        if (p_terrain_flags_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] & TERRAIN_FLAG_ROAD) {
                            has_road = 1;
                            if (p_terrain_flags_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] & TERRAIN_FLAG_ENCHANTED_ROAD)
                                enchanted = 1;
                        }
                    }
                }
            }
            if (has_road == 1 && enchanted == 0) {
                best = city;
                best_value = garrison;
            }
        }
    }
    if (best == -1)
        return 0;
    *x = p_cities[best].x;
    *y = p_cities[best].y;
    *plane = p_cities[best].plane;
    return 1;
}

/* (caster, &x, &y, &plane) the Disenchant True / Area target: a human city with Flying Fortress (each one 1 in 2
   while scanning), else the caster's most powerful warped node, else its most valuable city with Cursed Lands,
   Famine, Chaos Rift or Evil Presence, else the most expensive enchanted human unit on the landmass of the caster's
   fortress - whose square is then read from p_units[city] with city = -1 (BUG #144). Returns 1 when found
   Numbered: BUG #144 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Disenchant(int caster, int *x, int *y, int *plane)
{
    int i;
    int best;
    int node;
    int city;
    int best_unit;
    int landmass;

    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == PLAYER_HUMAN && p_cities[i].enchants[CITY_ENCH_FLYING_FORTRESS] && Random(2) == 1) {
            *x = p_cities[i].x;
            *y = p_cities[i].y;
            *plane = p_cities[i].plane;
            return 1;
        }
    }
    node = -1;
    best = 0;
    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].owner == caster && (p_nodes[i].meld_flags & NODE_WARPED) > 0 && p_nodes[i].power > best) {
            node = i;
            best = p_nodes[i].power;
        }
    }
    if (node != -1) {
        *x = p_nodes[node].x;
        *y = p_nodes[node].y;
        *plane = p_nodes[node].plane;
        return 1;
    }
    city = -1;
    best = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == caster &&
            (p_cities[i].enchants[CITY_ENCH_CURSED_LANDS] || p_cities[i].enchants[CITY_ENCH_FAMINE] ||
             p_cities[i].enchants[CITY_ENCH_CHAOS_RIFT] || p_cities[i].enchants[CITY_ENCH_EVIL_PRESENCE]) &&
            ai_city_own_value[i] > best) {
            city = i;
            best = ai_city_own_value[i];
        }
    }
    if (city != -1) {
        *x = p_cities[city].x;
        *y = p_cities[city].y;
        *plane = p_cities[city].plane;
        return 1;
    }
    landmass = p_landmass_ids[p_fortresses[caster].plane][p_fortresses[caster].y][p_fortresses[caster].x];
    best_unit = -1;
    best = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == PLAYER_HUMAN && p_units[i].enchantments != 0 &&
            p_landmass_ids[p_units[i].plane][p_units[i].y][p_units[i].x] == landmass &&
            unit_types[p_units[i].type].cost > best) {
            best_unit = i;
            best = unit_types[p_units[i].type].cost;
        }
    }
    if (best_unit != -1) {
        *x = p_units[city].x;
        *y = p_units[city].y;
        *plane = p_units[city].plane;
        return 1;
    }
    return 0;
}

/* (caster, &x, &y, &plane, spell) the Corruption (85) / Raise Volcano target: the seen enemy (non-neutral) city with
   the highest ai_city_target_value that has, in 25 random tries, a square at offset Random(5) - 3 (-3 .. +1, BUG #422)
   that is not a 5x5 corner, inside the map (x / y > 0), land, no volcano, not corrupted, and for Raise Volcano not
   mountain / hills / river / node. *plane is the plane of the LAST city examined, not the chosen one (BUG #423). Returns 1
   when found */
int AI_Target_Corruption_Volcano(int caster, int *x, int *y, int *plane, int spell)
{
    int city;
    int wx;
    int best_value;
    int best_city;
    int dx;
    int dy;
    int found;
    int tries;
    int best_x;
    int best_y;
    int wy;
    int best_plane;
    int tp;
    int corner;

    best_city = -1;
    best_value = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner != caster && p_cities[city].owner != PLAYER_NEUTRAL && ai_city_target_value[city] > best_value &&
            Bit_Test(caster, &p_cities[city].seen_by)) {
            tp = p_cities[city].plane;
            tries = 0;
            found = 0;
            for (; tries < 25 && found == 0; tries++) {
                dx = Random(5) - 3;
                dy = Random(5) - 3;
                if ((dx == -2 && (dy == -2 || dy == 2)) || (dx == 2 && (dy == -2 || dy == 2)))
                    corner = 1;
                else
                    corner = 0;
                wx = dx + p_cities[city].x;
                wy = dy + p_cities[city].y;
                if (corner == 0 && wx > 0 && wx < MAP_WIDTH && wy > 0 && wy < MAP_HEIGHT && Tile_Is_Land(wx, wy, tp) == 1 &&
                    Tile_Is_Volcano(wx, wy, tp) == 0 && Tile_Is_Corrupted(wx, wy, tp) == 0 &&
                    (spell == SPELL_CORRUPTION || (Tile_Is_Mountain(wx, wy, tp) == 0 && Tile_Is_Hills(wx, wy, tp) == 0 &&
                                     Tile_Is_River(wx, wy, tp) == 0 && Tile_Is_Node(wx, wy, tp) == 0))) {
                    best_city = city;
                    best_value = ai_city_target_value[city];
                    best_x = wx;
                    best_y = wy;
                    best_plane = tp;
                    found = 1;
                }
            }
        }
    }
    if (best_city != -1) {
        *x = best_x;
        *y = best_y;
        *plane = tp;
        return 1;
    }
    return 0;
}

/* (caster, &x, &y, &plane) DEAD CODE (no callers): an older Raise Volcano target - the same search as
   AI_Target_Corruption_Volcano with the seen test before the value test, hills / mountain / node excluded (rivers
   allowed), and the same last-city plane BUG #423. Returns 1 when found */
int AI_Target_Raise_Volcano_Old(int caster, int *x, int *y, int *plane)
{
    int city;
    int wx;
    int best_value;
    int best_city;
    int dx;
    int dy;
    int found;
    int tries;
    int best_x;
    int best_y;
    int wy;
    int best_plane;
    int tp;
    int corner;

    best_city = -1;
    best_value = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner != caster && p_cities[city].owner != PLAYER_NEUTRAL && Bit_Test(caster, &p_cities[city].seen_by) &&
            ai_city_target_value[city] > best_value) {
            tp = p_cities[city].plane;
            tries = 0;
            found = 0;
            for (; tries < 25 && found == 0; tries++) {
                dx = Random(5) - 3;
                dy = Random(5) - 3;
                if ((dx == -2 && (dy == -2 || dy == 2)) || (dx == 2 && (dy == -2 || dy == 2)))
                    corner = 1;
                else
                    corner = 0;
                wx = dx + p_cities[city].x;
                wy = dy + p_cities[city].y;
                if (corner == 0 && wx > 0 && wx < MAP_WIDTH && wy > 0 && wy < MAP_HEIGHT && Tile_Is_Land(wx, wy, tp) == 1 &&
                    Tile_Is_Volcano(wx, wy, tp) == 0 && Tile_Is_Corrupted(wx, wy, tp) == 0 &&
                    Tile_Is_Hills(wx, wy, tp) == 0 && Tile_Is_Mountain(wx, wy, tp) == 0 && Tile_Is_Node(wx, wy, tp) == 0) {
                    best_city = city;
                    best_value = ai_city_target_value[city];
                    best_x = wx;
                    best_y = wy;
                    best_plane = tp;
                    found = 1;
                }
            }
        }
    }
    if (best_city != -1) {
        *x = best_x;
        *y = best_y;
        *plane = tp;
        return 1;
    }
    return 0;
}

/* (stack, &x, &y, &plane) the Plane Shift target: the caster's strongest Arcanus stack carrying a settler (AISTACK_SETTLER), not on a city
   square (a city at the same x / y on either plane counts), whose square on Myrror suits it (water for a ship stack, AISTACK_TRANSPORT; not sea otherwise). The first parameter
   (the caster) is only used as the stack loop counter. Returns 1 when found */
int AI_Target_Plane_Shift(int stack, int *x, int *y, int *plane)
{
    int best_stack;
    int best;
    int strength;
    int city;

    best_stack = -1;
    best = 0;
    for (stack = 0; stack < ai_own_stack_count; stack++) {
        strength = p_ai_own_stacks[stack].strength;
        if ((p_ai_own_stacks[stack].flags & AISTACK_SETTLER) == 0)
            strength = 0;
        for (city = 0; city < num_cities; city++) {
            if (p_ai_own_stacks[stack].x == p_cities[city].x && p_ai_own_stacks[stack].y == p_cities[city].y)
                strength = 0;
        }
        if (p_ai_own_stacks[stack].plane == PLANE_ARCANUS && strength > best) {
            if (p_ai_own_stacks[stack].flags & AISTACK_TRANSPORT) {
                if (Tile_Is_Water(p_ai_own_stacks[stack].x, p_ai_own_stacks[stack].y, PLANE_MYRROR) == 1) {
                    best_stack = stack;
                    best = strength;
                }
            } else {
                if (Tile_Is_Sea(p_ai_own_stacks[stack].x, p_ai_own_stacks[stack].y, PLANE_MYRROR) == 0) {
                    best_stack = stack;
                    best = strength;
                }
            }
        }
    }
    if (best_stack == -1)
        return 0;
    *x = p_ai_own_stacks[best_stack].x;
    *y = p_ai_own_stacks[best_stack].y;
    *plane = p_ai_own_stacks[best_stack].plane;
    return 1;
}

/* (caster, &x, &y, &plane) the Nature's Cures target: the caster's most damaged unit; returns its square (the cast
   code then uses x as a unit index - BUG #34). Returns 1 when found */
int AI_Target_Natures_Cures(int caster, int *x, int *y, int *plane)
{
    int i;
    int best;
    int best_damage;
    int damage;

    best = -1;
    best_damage = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == caster) {
            damage = p_units[i].damage;
            if (damage > best_damage) {
                best = i;
                best_damage = damage;
            }
        }
    }
    if (best == -1)
        return 0;
    *x = p_units[best].x;
    *y = p_units[best].y;
    *plane = p_units[best].plane;
    return 1;
}

/* (&target, spell, caster) the wizard target of Drain Power (171), Subversion (177), Cruel Unminding (191) and Spell
   Blast (58): the first hostile wizard half the time, else a random one; then the Spell Blast condition (the target
   is casting and its progress is at most the caster's mana reserve - 50) is required for all four (BUG #102). Returns 1
   when the target is accepted; any other spell is a fatal error outside the release build
   Numbered: BUG #102 (BUGS_AND_QUIRKS.txt) */
int AI_Pick_Wizard_Target(int *target, int spell, int caster)
{
    if (ai_hostile_count == 0)
        return 0;
    if (spell == SPELL_DRAIN_POWER || spell == SPELL_SUBVERSION || spell == SPELL_CRUEL_UNMINDING || spell == SPELL_SPELL_BLAST) {
        *target = Random(2) == 1 ? ai_hostile_wizards[0] : ai_hostile_wizards[Random(ai_hostile_count) - 1];
        if (wizards[*target].casting_spell == SPELL_NONE)
            return 0;
        if (p_spell_data[wizards[*target].casting_spell].base_cost - wizards[*target].casting_cost_left >
            wizards[caster].mana_reserve - 50)
            return 0;
        return 1;
    } else
        AI_Fatal_No_Spell_Target(spell);
}

/* (&city, &realm, caster) AI_Target_Spell_Ward with its arguments in the caller's order */
int AI_Target_Spell_Ward_Call(int *city, int *realm, int caster)
{
    return AI_Target_Spell_Ward(caster, city, realm);
}

/* (player) 1 when the player has more cities on Myrror than the human; the human's counters are never initialised
   (BUG #424). Gates Planar Seal in AI_Pick_Global_Enchant and its category-9 weight in
   AI_Pick_Spell_Category */
int AI_More_Myrror_Cities_Than_Human(int player)
{
    int city;
    int ai_myrror;
    int ai_arcanus;
    int human_arcanus;
    int human_myrror;

    ai_arcanus = 0;
    ai_myrror = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == PLAYER_HUMAN) {
            if (p_cities[city].plane == PLANE_ARCANUS)
                human_arcanus++;
            else
                human_myrror++;
        } else if (p_cities[city].owner == player) {
            if (p_cities[city].plane == PLANE_ARCANUS)
                ai_arcanus++;
            else
                ai_myrror++;
        }
    }
    if (ai_myrror > human_myrror)
        return 1;
    return 0;
}

/* (x, y, plane) 1 when the square's terrain type (mod 762) is in the ranges a Floating Island may be
   placed on (below 162 except 1 and 18, 201 .. 232, 452 .. 467, 473 .. 601):
   the ocean and the shore tile sets, not the lake (18) or tile 1 (ReMoM's tt_ ranges; clarified using ReMoM repo) */
int AI_Tile_Floating_Island_OK(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain >= TILE_TUNDRA_RANGE_FIRST)
        return 0;
    if (terrain > TILE_RIVER3_LAST)
        return 1;
    if (terrain > TILE_SHORE3_LAST)
        return 0;
    if (terrain > TILE_DESERT_RANGE_LAST)
        return 1;
    if (terrain > TILE_SHORE2_LAST)
        return 0;
    if (terrain > 200)                          /* not a tile boundary: shore 197..200 is refused */
        return 1;
    if (terrain > TILE_SHORE_LAST)
        return 0;
    if (terrain == TILE_PLACEHOLDER_LAND)
        return 0;
    if (terrain == TILE_LAKE)
        return 0;
    return 1;
}

/* (caster, &city) the Summoning Circle target: while the circle is not at the fortress, the fortress city (the AI
   circle timer becomes Random(50) + Random(50)); when it already is there, the timer becomes Random(30) + 20 and the
   target is the caster's most populous non-fortress city on the plane + landmass with the most (other - own)
   cities. Returns 1 when found */
int AI_Target_Summoning_Circle(int caster, int *city)
{
    int i;
    int best_city;
    int best_diff;                              /* in the second loop the best population */
    int best_lm;
    int j;
    int fortress_city;
    int circle_city;
    int lm;
    int others;
    int own;
    int best_plane;
    int plane;
    int diff;

    fortress_city = -1;
    circle_city = -1;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x == p_fortresses[caster].x && p_cities[i].y == p_fortresses[caster].y &&
            p_cities[i].plane == p_fortresses[caster].plane)
            fortress_city = i;
        if (p_cities[i].x == wizards[caster].summoning_circle_xyp[XYP_X] &&
            p_cities[i].y == wizards[caster].summoning_circle_xyp[XYP_Y] &&
            p_cities[i].plane == wizards[caster].summoning_circle_xyp[XYP_PLANE])
            circle_city = i;
    }
    if (fortress_city == -1)
        return 0;
    if (circle_city == -1 || fortress_city != circle_city) {
        p_ai_circle_timer[caster] = Random(50) + Random(50);
        *city = fortress_city;
        return 1;
    } else {
        p_ai_circle_timer[caster] = Random(30) + 20;
        best_diff = -100;
        best_lm = -1;
        best_plane = 0;
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner == caster) {
                others = 0;
                own = 0;
                plane = p_cities[i].plane;
                lm = p_landmass_ids[plane][p_cities[i].y][p_cities[i].x];
                if (lm != best_lm) {
                    for (j = 0; j < num_cities; j++) {
                        if (p_cities[j].plane == plane &&
                            p_landmass_ids[plane][p_cities[j].y][p_cities[j].x] == lm) {
                            if (p_cities[j].owner == caster)
                                own++;
                            else
                                others++;
                        }
                    }
                    diff = others - own;
                    if (diff > best_diff) {
                        best_diff = diff;
                        best_lm = lm;
                        best_plane = plane;
                    }
                }
            }
        }
        if (best_lm == -1)
            return 0;
        best_city = -1;
        best_diff = 0;
        for (i = 0; i < num_cities; i++) {
            if (i != fortress_city &&
                p_landmass_ids[p_cities[i].plane][p_cities[i].y][p_cities[i].x] == best_lm &&
                p_cities[i].plane == best_plane && p_cities[i].owner == caster &&
                p_cities[i].population > best_diff) {
                best_diff = p_cities[i].population;
                best_city = i;
            }
        }
        if (best_city == -1)
            return 0;
        *city = best_city;
        return 1;
    }
}

/* (player) 1 when the player's Summoning Circle is in its fortress city (when not, AI_Pick_Spell_Category multiplies
   category 1's weight by 5) */
int AI_Circle_At_Fortress(int player)
{
    int city;
    int fortress_city;
    int circle_city;

    fortress_city = -1;
    circle_city = -1;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].x == p_fortresses[player].x && p_cities[city].y == p_fortresses[player].y &&
            p_cities[city].plane == p_fortresses[player].plane)
            fortress_city = city;
        if (p_cities[city].x == wizards[player].summoning_circle_xyp[XYP_X] &&
            p_cities[city].y == wizards[player].summoning_circle_xyp[XYP_Y] &&
            p_cities[city].plane == wizards[player].summoning_circle_xyp[XYP_PLANE])
            circle_city = city;
    }
    if (fortress_city == -1 || circle_city == -1 || circle_city != fortress_city)
        return 0;
    else
        return 1;
}
