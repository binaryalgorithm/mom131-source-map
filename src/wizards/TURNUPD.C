/* TURNUPD.C - WIZARDS.EXE overlay code segment c3f5 (stub 25ea): the end-of-turn updates - city production, gold
   and magic income, upkeep (food, gold, mana) and what a shortfall disbands or dispels, city growth, the overland
   casting, the random offers (heroes, mercenaries, merchants), research and spell learning, healing, global
   enchantment diplomacy, unit experience, volcano decay and two AI helpers.
   This module's data (DS:6e4c..6e9a): its literals; BSS (DS:ca66..ca73): p_unit_heal_divisor and 10 unread bytes.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS c3f5 src/wizards/TURNUPD.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Turn_Report_Clear(void);
void Cities_Recalculate_All(void);
void City_Production_Step(int city);
void Turn_Gold_Income(void);
void Turn_Apply_Magic_Income(void);
void Turn_Pay_Upkeep(void);
void Upkeep_Shortfall_Food(int player, int food, int food_upkeep);
int Upkeep_Shortfall_Gold(int player, int gold_needed);
int Upkeep_Shortfall_Mana(int player, int mana_needed);
int Mana_Shortfall_Summons(int player, int mana_needed);
int Mana_Shortfall_City_Enchants(int player, int mana_needed);
int Mana_Shortfall_Global_Enchants(int player, int mana_needed);
int Mana_Shortfall_Unit_Enchants(int player, int mana_needed);
int Human_Gold_Shortfall(void);
int Human_Mana_Shortfall(void);
void Turn_City_Growth(void);
int Fortress_Hero_Skill_Bonus(int player);
void Turn_Overland_Casting(void);
void Turn_Random_Offers(void);
void Turn_Research_Complete(void);
void Wizard_Learn_Spell(int player, int spell, int show);
void Unit_Heal_Step(int unit, int divisor, int healer);
void Turn_Heal_Units(void);
void Turn_Global_Enchant_Diplomacy(void);
void Turn_Unit_Experience(void);
void Turn_Volcano_Decay(void);
int AI_Disband_Obsolete_Units(void);
void AI_Average_Unit_Cost(void);

#include "GAME.H"

int turnupd_unused_ca6a[5];                     /* DS:ca6a  no code reads these 10 bytes (owner and type unknown) */
char far *p_unit_heal_divisor;                    /* DS:ca66  [unit] this turn's healing divisor (HEAL_MARK_HEALER = healed by a healer) */
#define HEAL_MARK_HEALER 0x42                     /* the divisor slot's mark for a unit a healer has treated */

extern char report_outposts_deserted_count;     /* DS:99c2 */
extern char report_outposts_deserted_names[][20];       /* DS:99c3 */
extern char report_outposts_grown_count;        /* DS:9adb */
extern unsigned char report_outposts_grown[];   /* DS:9adc */
extern char report_buildings_lost_count;              /* DS:9b42 */
extern char report_units_lost_count;            /* DS:9b94 */
extern struct ReportUnitLost report_units_lost[];       /* DS:9b96 */
extern char report_units_killed_count;          /* DS:9be6 */
extern char report_city_growth_count;           /* DS:9c38 */
extern unsigned char report_city_growth[];      /* DS:9c39 */
extern char report_city_deaths_count;           /* DS:9c4d */
extern unsigned char report_city_deaths[];      /* DS:9c4e */
extern char report_unit_spells_lost_count;      /* DS:9c62 */
extern unsigned char report_unit_spells_lost[]; /* DS:9c63 */
extern char report_city_spells_lost_count;      /* DS:9c77 */
extern unsigned char report_city_spells_lost[]; /* DS:9c78 */
extern char report_globals_lost;                /* DS:9c8c */
extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern int grand_vizier;                        /* DS:9d22 */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int game_turn;                           /* DS:bd90 */
extern int hire_name_entry;                     /* DS:c9e0 */
#define merchant_item hire_name_entry          /* here: the merchant's offered item */
extern int list_count;                          /* DS:c926 */
#define disbanded_count list_count               /* here: the units AI_Disband_Obsolete_Units disbands */
extern struct MouseRegion mouse_region_busy;    /* DS:0106 */

void City_Recalculate(int city);
int City_Production_Cost(int city);
int Map_Explore_Radius(int x, int y, int plane, int range);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Stack_Limit_Enforce(int unit);
void City_Garrison_List(int city, int *count, int *list);
void far_strcpy();                              /* unprototyped: (near dst, 0, far src) */
int Vizier_Choose_Production(int city);
void Units_Food_Upkeep_All(int *out);
void Wizard_Magic_Income(int *mana, int *research, int *skill, int player);
int Wizard_Food_Upkeep(int player);
int Wizard_Gold_Upkeep(int player);
int Wizard_Mana_Upkeep(int player);
int Units_Stranded_By_Removal(int unit, int *n, int *list);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Unit_Gold_Upkeep(int unit);
int Unit_Mana_Upkeep(int unit);
int City_Enchant_Mana_Upkeep(int city, int player);
int Global_Enchant_Mana_Upkeep(int player);
void Wizard_Power_Base(void);
int City_Growth_Rate(int city);
int City_Min_Farmers(int city);
void City_Consecration_Turn(int city);
void City_Stream_Of_Life_Turn(int city);
void City_Chaos_Rift_Turn(int city);
void City_Gaias_Blessing_Turn(int city);
void City_Nightshade_Dispel(int city);
void Wizards_Count_Volcanoes(void);
int Item_Create_Prefab(int prefab_kind, int *spellbooks, int max_cost);
void Item_Free(int item);
void Screen_Merchant_Offer(void);
int Mercenary_Offer_Roll(int player, int *x, int *y, int *plane, int *amount, int *type_out, int *cost_out, int *level_out);
int Screen_Hire_Mercenaries(int unit_type, int count, int level, int cost);
int Wizard_Hero_Count(int player);
int Wizard_Free_Hero_Slot(int player);
int Hero_Pick_Random(int player, int mode, int pool);
int Screen_Hire_Hero(int slot, int hero_type, int offer_kind);
int AI_Hire_Hero(int player, int slot, int unit_type);
int Screen_Choose_Research(void);
int Sound_Stop(void);
void Music_Play_Current(void);
int AI_Choose_Research(int player);
int Array_Delete_Near(int index, char *array, int record_size, int count);
int Research_Candidates_Refill(int player);
int Screen_Spell_Learned(int spell, int researched);
int Unit_Max_Hits(int unit);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);
void Map_Units_At(int x, int y, int plane, int *count, int *list);
void Global_Enchant_Relations(int caster, int spell, int divisor);
void Great_Wasting_Turn(int caster);
void Meteor_Storm_Turn(int caster);
void Armageddon_Turn(int caster);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
int Screen_Hero_Level_Up(int unit);
void Volcano_To_Mountain(int x, int y, int plane);

/* City_Recalculate for every city (start of Next_Turn and after the autosave) */
void Cities_Recalculate_All(void)
{
    int city;

    for (city = 0; city < num_cities; city++)
        City_Recalculate(city);
}

/* zero the end-of-turn report lists (shown by Screen_Turn_Report) */
void Turn_Report_Clear(void)
{
    report_units_lost_count = 0;
    report_units_killed_count = 0;
    report_unit_spells_lost_count = 0;
    report_city_spells_lost_count = 0;
    report_globals_lost = 0;
    report_city_growth_count = 0;
    report_city_deaths_count = 0;
    report_buildings_lost_count = 0;
    report_outposts_deserted_count = 0;
    report_outposts_grown_count = 0;
}

/* (city) a populated city's production: a building (producing 3..99; Trade Goods / Housing: the AI switches to
   auto) gains the production (half for the neutral player); at its cost it is built (a ghost state + 1 if it was
   not 'none', BUG #132), replaces its predecessor, the Oracle explores radius 6 for the human, a notice is queued
   for the human and the city goes to Housing (Vizier: auto). A unit gains the full production; at its cost it is
   created (fewer than 1000 units) or the human is told to disband some (Trade Goods). AI / Vizier: auto (-4) */
void City_Production_Step(int city)
{
    int garrison_count;
    int garrison[MAX_STACK];
    char city_name[20];
    int production_cost;

    production_cost = City_Production_Cost(city);
    if (p_cities[city].population > 0) {
        if (p_cities[city].producing < PROD_UNIT_BASE) {
            if (p_cities[city].producing < BLD_BARRACKS) {   /* none, Trade Goods, Housing */
                if (p_cities[city].owner != PLAYER_HUMAN)
                    p_cities[city].producing = PROD_AUTO_CHOOSE;
            } else {
                p_cities[city].production_progress = p_cities[city].production_progress +
                    (p_cities[city].owner == PLAYER_NEUTRAL ? p_cities[city].production_income / 2 : p_cities[city].production_income);
                if (p_cities[city].production_progress >= production_cost) {
                    if (p_cities[city].buildings[p_cities[city].producing] < 0) {
                        p_cities[city].buildings[p_cities[city].producing] = BLDSTATE_BUILT;
                        if (p_building_data[p_cities[city].producing].replaces != -1)
                            p_cities[city].buildings[p_building_data[p_cities[city].producing].replaces] = BLDSTATE_REPLACED;
                    } else
                        p_cities[city].buildings[p_cities[city].producing] =
                            p_cities[city].buildings[p_cities[city].producing] + 1;
                    if (p_cities[city].producing == PROD_ORACLE && p_cities[city].owner == PLAYER_HUMAN)
                        Map_Explore_Radius(p_cities[city].x, p_cities[city].y, p_cities[city].plane, 6);
                    p_cities[city].building_count++;
                    p_cities[city].production_progress = 0;
                    if (p_cities[city].owner == PLAYER_HUMAN) {
                        if (city_notice_count < MAX_REPORT_ENTRIES) {
                            city_notices[city_notice_count].city = city;
                            city_notices[city_notice_count].producing = p_cities[city].producing;
                            city_notice_count++;
                        }
                        if (grand_vizier == 1)
                            p_cities[city].producing = PROD_AUTO_CHOOSE;
                        else
                            p_cities[city].producing = PROD_HOUSING;
                    } else
                        p_cities[city].producing = PROD_AUTO_CHOOSE;
                }
            }
        } else {
            p_cities[city].production_progress = p_cities[city].production_progress + p_cities[city].production_income;
            if (p_cities[city].production_progress >= production_cost) {
                if (num_units + 1 < MAX_UNITS) {
                    Unit_Create(p_cities[city].producing - PROD_UNIT_BASE, p_cities[city].owner, p_cities[city].x,
                                p_cities[city].y, p_cities[city].plane, city);
                    Stack_Limit_Enforce(num_units - 1);
                    City_Garrison_List(city, &garrison_count, garrison);
                    if (p_cities[city].owner != PLAYER_HUMAN || grand_vizier == 1)
                        p_cities[city].producing = PROD_AUTO_CHOOSE;
                } else if (p_cities[city].owner == PLAYER_HUMAN) {
                    LBX_Load_Records_Near("message", 0, message_text, 0x42, 1, 0x96);
                    far_strcpy(city_name, 0, p_cities[city].name);
                    strcat(message_text, city_name);
                    strcat(message_text, ". You must disband some units if you wish to build or summon any more.");
                    Message_Box(message_text);
                    if (p_cities[city].owner == PLAYER_HUMAN && grand_vizier == 0)
                        p_cities[city].producing = PROD_TRADE_GOODS;
                    else
                        p_cities[city].producing = PROD_AUTO_CHOOSE;
                } else
                    p_cities[city].producing = PROD_AUTO_CHOOSE;
                p_cities[city].production_progress = 0;
            }
        }
        if (p_cities[city].owner == PLAYER_HUMAN && grand_vizier == 1 && p_cities[city].producing == PROD_AUTO_CHOOSE)
            Vizier_Choose_Production(city);
    }
}

/* gold income: +10 per Noble hero, the cities' gold minus building upkeep, half the food surplus over the units'
   food upkeep; capped at INCOME_CAP (32000; the end of the turn caps at RESOURCE_CAP, 30000), at least 0 */
void Turn_Gold_Income(void)
{
    int surplus;
    int gold[6];
    int food_gold[6];
    int food_upkeep[6];
    int i;
    int j;

    for (i = 0; i < num_players; i++) {
        gold[i] = 0;
        food_gold[i] = 0;
        for (j = 0; j < 6; j++)
            if (wizards[i].hired_heroes[j].unit_index > -1 &&
                (p_hero_stats[i][p_units[wizards[i].hired_heroes[j].unit_index].type].abilities & HERO_ABILITY_NOBLE) > 0)
                gold[i] += 10;
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner > -1) {
            gold[p_cities[i].owner] = gold[p_cities[i].owner] + p_cities[i].gold_income - p_cities[i].building_upkeep;
            surplus = p_cities[i].food_income - p_cities[i].population;
            if (surplus < 0)
                surplus = 0;
            food_gold[p_cities[i].owner] = food_gold[p_cities[i].owner] + surplus;
        }
    }
    Units_Food_Upkeep_All(food_upkeep);
    for (i = 0; i < num_players; i++) {
        food_gold[i] = (food_gold[i] - food_upkeep[i]) / 2;
        if (food_gold[i] < 0)
            food_gold[i] = 0;
        if (gold[i] + food_gold[i] > INCOME_CAP - wizards[i].gold)
            wizards[i].gold = INCOME_CAP;
        else
            wizards[i].gold += gold[i] + food_gold[i];
        if (wizards[i].gold < 0)
            wizards[i].gold = 0;
    }
}

/* research, mana and casting skill income (not while casting Spell of Return): research left -= RP, reserve +=
   MP (cap INCOME_CAP, 32000), skill points += SP (x 3 / 2 for Archmage) */
void Turn_Apply_Magic_Income(void)
{
    int skill;
    int mana;
    unsigned research;
    int player;

    for (player = 0; player < num_players; player++) {
        if (wizards[player].casting_spell != SPELL_SPELL_OF_RETURN) {
            Wizard_Magic_Income(&mana, &research, &skill, player);
            if (wizards[player].research_left > research)
                wizards[player].research_left = wizards[player].research_left - research;
            else
                wizards[player].research_left = 0;
            if (INCOME_CAP - wizards[player].mana_reserve < mana)
                wizards[player].mana_reserve = INCOME_CAP;
            else
                wizards[player].mana_reserve += mana;
            wizards[player].skill_points = wizards[player].skill_points +
                (wizards[player].retorts[RETORT_ARCHMAGE] > 0 ? (long)skill * 3 / 2 : skill);
        }
    }
}

/* upkeep: the food surplus, gold and mana; the human's shortfalls disband units / dispel spells */
void Turn_Pay_Upkeep(void)
{
    int need;
    int gold_upkeep[6];
    int mana_upkeep[6];
    int food_surplus[6];
    int i;

    for (i = 0; i < 6; i++)
        food_surplus[i] = 0;
    for (i = 0; i < num_cities; i++)
        if (p_cities[i].owner > -1 && p_cities[i].food_income > p_cities[i].population)
            food_surplus[p_cities[i].owner] += p_cities[i].food_income - p_cities[i].population;
    for (i = 0; i < num_players; i++) {
        need = Wizard_Food_Upkeep(i);
        if (food_surplus[i] < need && i == PLAYER_HUMAN)
            Upkeep_Shortfall_Food(i, food_surplus[i], need);
        gold_upkeep[i] = Wizard_Gold_Upkeep(i);
        if (gold_upkeep[i] > wizards[i].gold && i == PLAYER_HUMAN)
            gold_upkeep[i] = Upkeep_Shortfall_Gold(i, gold_upkeep[i]);
        wizards[i].gold = wizards[i].gold - gold_upkeep[i];
        if (wizards[i].gold < 0)
            wizards[i].gold = 0;
        mana_upkeep[i] = Wizard_Mana_Upkeep(i);
        if (mana_upkeep[i] > wizards[i].mana_reserve && i == PLAYER_HUMAN)
            mana_upkeep[i] = Upkeep_Shortfall_Mana(i, mana_upkeep[i]);
        wizards[i].mana_reserve = wizards[i].mana_reserve - mana_upkeep[i];
        if (wizards[i].mana_reserve < 0)
            wizards[i].mana_reserve = 0;
    }
}

/* (player, food, food upkeep) disbands the player's normal units (not fantastic, hero or transport) from the top
   index down until the food covers the upkeep; report reasons UNIT_LOST_FOOD / UNIT_LOST_DROWNED (the latter the units a removed ship strands at sea). BUG #6: the stranded units are
   tested by the carrier's type
   Numbered: BUG #6 (BUGS_AND_QUIRKS.txt) */
void Upkeep_Shortfall_Food(int player, int food, int food_upkeep)
{
    int n_stranded;
    int k;
    int stranded[MAX_STACK];
    int unit;

    for (unit = num_units - 1; unit > -1 && food_upkeep > food; unit--) {
        if (p_units[unit].owner == player && (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0 &&
            p_units[unit].type > UNIT_HERO_CHOSEN && unit_types[p_units[unit].type].transport_capacity == 0) {
            food_upkeep--;
            Units_Stranded_By_Removal(unit, &n_stranded, stranded);
            if (n_stranded > 0) {
                for (k = 0; k < n_stranded; k++) {
                    if ((unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0 && p_units[unit].type > UNIT_HERO_CHOSEN)
                        food_upkeep--;
                    if (report_units_lost_count < MAX_REPORT_ENTRIES && player == PLAYER_HUMAN) {
                        report_units_lost[report_units_lost_count].unit_type = p_units[unit].type;
                        report_units_lost[report_units_lost_count].reason = UNIT_LOST_DROWNED;
                        report_units_lost_count++;
                    }
                    Unit_Kill_Or_Dismiss(stranded[k], UNIT_REMOVE_KILLED);
                }
            }
            if (report_units_lost_count < MAX_REPORT_ENTRIES && player == PLAYER_HUMAN) {
                report_units_lost[report_units_lost_count].unit_type = p_units[unit].type;
                report_units_lost[report_units_lost_count].reason = UNIT_LOST_FOOD;
                report_units_lost_count++;
            }
            Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED);
        }
    }
}

/* (player, gold needed) -> the gold still needed: disbands non-fantastic, non-transport units with a gold upkeep
   (heroes included) from the top index down while the treasury is short; report reasons UNIT_LOST_GOLD / _DROWNED */
int Upkeep_Shortfall_Gold(int player, int gold_needed)
{
    int upkeep;
    int n_stranded;
    int k;
    int stranded[MAX_STACK];
    int unit;

    for (unit = num_units - 1; unit > -1 && wizards[player].gold < gold_needed; unit--) {
        if (p_units[unit].owner == player && (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0 &&
            unit_types[p_units[unit].type].transport_capacity == 0) {
            upkeep = Unit_Gold_Upkeep(unit);
            if (upkeep <= 0)
                continue;
            gold_needed -= upkeep;
            Units_Stranded_By_Removal(unit, &n_stranded, stranded);
            if (n_stranded > 0) {
                for (k = 0; k < n_stranded; k++) {
                    gold_needed -= Unit_Gold_Upkeep(stranded[k]);
                    if (report_units_lost_count < MAX_REPORT_ENTRIES && player == PLAYER_HUMAN) {
                        report_units_lost[report_units_lost_count].unit_type = p_units[stranded[k]].type;
                        report_units_lost[report_units_lost_count].reason = UNIT_LOST_DROWNED;
                        report_units_lost_count++;
                    }
                    Unit_Kill_Or_Dismiss(stranded[k], UNIT_REMOVE_KILLED);
                }
            }
            if (report_units_lost_count < MAX_REPORT_ENTRIES && p_units[unit].owner == PLAYER_HUMAN) {
                report_units_lost[report_units_lost_count].unit_type = p_units[unit].type;
                report_units_lost[report_units_lost_count].reason = UNIT_LOST_GOLD;
                report_units_lost_count++;
            }
            Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED);
        }
    }
    return gold_needed;
}

/* (player, mana needed) -> the mana still needed: up to four rounds, each from a random category on to the next
   untried one - unit enchantments, city enchantments, global enchantments, summoned units */
int Upkeep_Shortfall_Mana(int player, int mana_needed)
{
    int k;
    int tried[4];
    int rounds;
    int category;

    rounds = 0;
    for (k = 0; k < 4; k++)
        tried[k] = 0;
    while (wizards[player].mana_reserve < mana_needed && rounds < 4) {
        rounds++;
        for (category = Random(4) - 1; tried[category] > 0; category = (category + 1) % 4)
            ;
        switch (category) {
        case 0:
            mana_needed = Mana_Shortfall_Unit_Enchants(player, mana_needed);
            tried[0] = 1;
            break;
        case 1:
            mana_needed = Mana_Shortfall_City_Enchants(player, mana_needed);
            tried[1] = 1;
            break;
        case 2:
            mana_needed = Mana_Shortfall_Global_Enchants(player, mana_needed);
            tried[2] = 1;
            break;
        case 3:
            mana_needed = Mana_Shortfall_Summons(player, mana_needed);
            tried[3] = 1;
            break;
        }
    }
    return mana_needed;
}

/* (player, mana needed) -> the mana still needed: disbands the player's fantastic units, each saving its upkeep
   (half for a Channeler); report reasons UNIT_LOST_MANA / _DROWNED. BUG #6: starts one past the last unit (index num_units)
   Numbered: BUG #6 (BUGS_AND_QUIRKS.txt) */
int Mana_Shortfall_Summons(int player, int mana_needed)
{
    int n_stranded;
    int k;
    int divisor;
    int stranded[MAX_STACK];
    int unit;

    if (wizards[player].retorts[RETORT_CHANNELER] > 0)
        divisor = 2;
    else
        divisor = 1;
    for (unit = num_units; unit > -1 && wizards[player].mana_reserve < mana_needed; unit--) {
        if (p_units[unit].owner == player && (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) != 0) {
            mana_needed -= Unit_Mana_Upkeep(unit) / divisor;
            Units_Stranded_By_Removal(unit, &n_stranded, stranded);
            if (n_stranded > 0) {
                for (k = 0; k < n_stranded; k++) {
                    mana_needed -= Unit_Mana_Upkeep(stranded[k]) / divisor;
                    if (report_units_lost_count < MAX_REPORT_ENTRIES && player == PLAYER_HUMAN) {
                        report_units_lost[report_units_lost_count].unit_type = p_units[stranded[k]].type;
                        report_units_lost[report_units_lost_count].reason = UNIT_LOST_DROWNED;
                        report_units_lost_count++;
                    }
                    Unit_Kill_Or_Dismiss(stranded[k], UNIT_REMOVE_KILLED);
                }
            }
            if (report_units_lost_count < MAX_REPORT_ENTRIES && player == PLAYER_HUMAN) {
                report_units_lost[report_units_lost_count].unit_type = p_units[unit].type;
                report_units_lost[report_units_lost_count].reason = UNIT_LOST_MANA;
                report_units_lost_count++;
            }
            Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_DISMISSED);
        }
    }
    return mana_needed;
}

/* (player, mana needed) -> the mana still needed: clears all of the player's enchantments in a city at a time
   (upkeep halved for a Channeler); logs the city for the human */
int Mana_Shortfall_City_Enchants(int player, int mana_needed)
{
    int city;
    int upkeep;
    char far *enchants;
    int divisor;
    int k;

    if (wizards[player].retorts[RETORT_CHANNELER] > 0)
        divisor = 2;
    else
        divisor = 1;
    for (city = 0; city < num_cities && wizards[player].mana_reserve < mana_needed; city++) {
        upkeep = City_Enchant_Mana_Upkeep(city, player) / divisor;
        if (upkeep > 0) {
            mana_needed -= upkeep;
            enchants = p_cities[city].enchants;
            for (k = 0; k < NUM_CITY_ENCHANTS; k++)
                if (enchants[k] == player + 1)
                    enchants[k] = 0;
            if (player == PLAYER_HUMAN && report_city_spells_lost_count < MAX_REPORT_ENTRIES) {
                report_city_spells_lost[report_city_spells_lost_count] = city;
                report_city_spells_lost_count = report_city_spells_lost_count + 1;
            }
        }
    }
    return mana_needed;
}

/* (player, mana needed) -> the mana still needed: clears all 24 of the player's global enchantments (upkeep halved
   for a Channeler); report_globals_lost for the human */
int Mana_Shortfall_Global_Enchants(int player, int mana_needed)
{
    int upkeep;
    char *globals;
    int divisor;
    int k;

    if (wizards[player].retorts[RETORT_CHANNELER] > 0)
        divisor = 2;
    else
        divisor = 1;
    upkeep = Global_Enchant_Mana_Upkeep(player) / divisor;
    if (upkeep > 0) {
        mana_needed -= upkeep;
        globals = (char *)&wizards[player].globals;
        for (k = 0; k < NUM_GLOBAL_ENCHANTS; k++)
            if ((int)globals[k] > 0)
                globals[k] = 0;
        if (player == PLAYER_HUMAN)
            report_globals_lost = 1;
    }
    return mana_needed;
}

/* (player, mana needed) -> the mana still needed: strips all enchantments of each own unit whose enchantment upkeep
   (unit upkeep minus a fantastic unit's base upkeep, halved for a Channeler) is positive; logs the UNIT INDEX for
   the human (printed as a type name, BUG #130); the mutation_flags & 0x4000 test is never true (the field is
   a char: dead code) */
int Mana_Shortfall_Unit_Enchants(int player, int mana_needed)
{
    int unit;
    int upkeep;
    unsigned unused;
    int divisor;

    if (wizards[player].retorts[RETORT_CHANNELER] > 0)
        divisor = 2;
    else
        divisor = 1;
    unused = 0x95d0;   /* a dead store of 0x95d0 (a constant or DS:95d0 - the bytes cannot tell) */
    for (unit = 0; unit < num_units && wizards[player].mana_reserve < mana_needed; unit++) {
        if (p_units[unit].owner == player) {
            upkeep = Unit_Mana_Upkeep(unit) / divisor;
            if (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC)
                upkeep -= unit_types[p_units[unit].type].upkeep / divisor;
            if (upkeep > 0) {
                mana_needed -= upkeep;
                p_units[unit].enchantments = 0;
                if (p_units[unit].mutation_flags & 0x4000)
                    p_units[unit].mutation_flags = p_units[unit].mutation_flags - 0x4000;
                if (player == PLAYER_HUMAN && report_unit_spells_lost_count < MAX_REPORT_ENTRIES) {
                    report_unit_spells_lost[report_unit_spells_lost_count] = unit;
                    report_unit_spells_lost_count = report_unit_spells_lost_count + 1;
                }
            }
        }
    }
    return mana_needed;
}

/* () -> the gold the human is short: the gold upkeep minus (treasury + the cities' gold), 0 if covered */
int Human_Gold_Shortfall(void)
{
    int upkeep;
    int city;
    int gold;

    gold = 0;
    for (city = 0; city < num_cities; city++)
        if (p_cities[city].owner == PLAYER_HUMAN)
            gold += p_cities[city].gold_income;
    upkeep = Wizard_Gold_Upkeep(PLAYER_HUMAN);
    if (gold + wizards[PLAYER_HUMAN].gold < upkeep)
        return upkeep - gold - wizards[PLAYER_HUMAN].gold;
    else
        return 0;
}

/* () -> the mana the human is short: the mana upkeep minus (power x mana% + reserve), 0 if covered; it first runs
   Wizard_Power_Base, recomputing every wizard's power_base */
int Human_Mana_Shortfall(void)
{
    int mana;
    int upkeep;

    Wizard_Power_Base();
    mana = wizards[PLAYER_HUMAN].mana_percentage * wizards[PLAYER_HUMAN].power_base / 100;
    upkeep = Wizard_Mana_Upkeep(PLAYER_HUMAN);
    if (mana + wizards[PLAYER_HUMAN].mana_reserve < upkeep)
        return upkeep - mana - wizards[PLAYER_HUMAN].mana_reserve;
    else
        return 0;
}

/* per city: an outpost (population 0) with no growth points left is deserted and removed (BUG #2: no 'continue' -
   the city shifted into the slot is then tested too), one with 10+ grows into a hamlet; a city adds its growth rate,
   at 100+ grows by one (below 25; the farmers keep their surplus over the minimum; neutral max 8), below 0 shrinks
   (min 1); Pestilence kills one if Random(10) < population; then its production. Then the city enchantment effects
   (Consecration, Stream of Life, Chaos Rift, Gaia's Blessing, Nightshade) and the volcano count
   Numbered: BUG #104, #274 (BUGS_AND_QUIRKS.txt) */
void Turn_City_Growth(void)
{
    int city;
    int rate;
    int min_farmers;
    int spare_farmers;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].population == 0) {
            if (p_cities[city].population_tens <= 0) {
                if (p_cities[city].owner == PLAYER_HUMAN && report_outposts_deserted_count < MAX_REPORT_ENTRIES) {
                    far_strcpy(report_outposts_deserted_names[report_outposts_deserted_count], 0, p_cities[city].name);
                    report_outposts_deserted_count = report_outposts_deserted_count + 1;
                }
                City_Remove(city);
            }
            if (p_cities[city].population_tens >= 10) {
                p_cities[city].population = 1;
                p_cities[city].size = 1;
                p_cities[city].farmers = City_Min_Farmers(city);
                City_Recalculate(city);
                if (p_cities[city].owner == PLAYER_HUMAN && report_outposts_grown_count < MAX_REPORT_ENTRIES) {
                    report_outposts_grown[report_outposts_grown_count] = city;
                    report_outposts_grown_count = report_outposts_grown_count + 1;
                }
            }
        } else {
            rate = City_Growth_Rate(city);
            p_cities[city].population_tens += rate;
            if (p_cities[city].population_tens >= POP_TENS_PER_CITIZEN && p_cities[city].population < MAX_CITY_POPULATION) {
                spare_farmers = City_Min_Farmers(city);
                spare_farmers = p_cities[city].farmers - spare_farmers;
                p_cities[city].population = p_cities[city].population + 1;
                p_cities[city].population_tens = 0;
                min_farmers = City_Min_Farmers(city);
                p_cities[city].farmers = min_farmers + spare_farmers;
                if (p_cities[city].farmers > p_cities[city].population)
                    p_cities[city].farmers = p_cities[city].population;
                p_cities[city].size = (p_cities[city].population + 3) / 4;
                if (p_cities[city].size > 5)
                    p_cities[city].size = 5;
                if (p_cities[city].owner == PLAYER_NEUTRAL && p_cities[city].population > 8)
                    p_cities[city].population = 8;
                if (p_cities[city].owner == PLAYER_HUMAN && report_city_growth_count < MAX_REPORT_ENTRIES) {
                    report_city_growth[report_city_growth_count] = city;
                    report_city_growth_count = report_city_growth_count + 1;
                }
            }
            if (p_cities[city].population_tens < 0) {
                if (p_cities[city].population > 1) {
                    p_cities[city].population = p_cities[city].population - 1;
                    p_cities[city].size = (p_cities[city].population + 3) / 4;
                    if (p_cities[city].owner == PLAYER_HUMAN && report_city_deaths_count < MAX_REPORT_ENTRIES) {
                        report_city_deaths[report_city_deaths_count] = city;
                        report_city_deaths_count++;
                    }
                    p_cities[city].population_tens = p_cities[city].population_tens + 100;
                } else
                    p_cities[city].population_tens = 5;
            }
            if (p_cities[city].enchants[CITY_ENCH_PESTILENCE] > 0 && p_cities[city].population > Random(10)) {
                p_cities[city].population = p_cities[city].population - 1;
                if (p_cities[city].owner == PLAYER_HUMAN && report_city_deaths_count < MAX_REPORT_ENTRIES) {
                    report_city_deaths[report_city_deaths_count] = city;
                    report_city_deaths_count++;
                }
            }
            City_Production_Step(city);
        }
        if (p_cities[city].enchants[CITY_ENCH_CONSECRATION] > 0)
            City_Consecration_Turn(city);
        if (p_cities[city].enchants[CITY_ENCH_STREAM_OF_LIFE] > 0)
            City_Stream_Of_Life_Turn(city);
        if (p_cities[city].enchants[CITY_ENCH_CHAOS_RIFT] > 0)
            City_Chaos_Rift_Turn(city);
        if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING] > 0)
            City_Gaias_Blessing_Turn(city);
        if (p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] > 0)
            City_Nightshade_Dispel(city);
    }
    Wizards_Count_Volcanoes();
}

/* (player) -> half the spell skill of each of the player's heroes standing in its fortress city, summed */
int Fortress_Hero_Skill_Bonus(int player)
{
    int half;
    int bonus;
    int k;

    half = 0;
    bonus = 0;
    for (k = 0; k < 6; k++) {
        if (wizards[player].hired_heroes[k].unit_index > -1 &&
            p_units[wizards[player].hired_heroes[k].unit_index].x == p_fortresses[player].x &&
            p_units[wizards[player].hired_heroes[k].unit_index].y == p_fortresses[player].y &&
            p_units[wizards[player].hired_heroes[k].unit_index].plane == p_fortresses[player].plane) {
            Battle_Unit_Init(wizards[player].hired_heroes[k].unit_index, p_unit_stats_buf);
            half = p_unit_stats_buf->spell_skill / 2;
            bonus += half;
        }
    }
    return bonus;
}

/* per wizard (only the Time Stop caster while it runs): this turn's skill += the fortress heroes' bonus; the nominal
   skill refreshed; a spell in progress gets min(mana, skill left, cost left) - an AI casting Spell of Return pays
   its nominal skill with the mana floored at 0; if the rest then fits in min(mana, nominal skill) it is finished now
   and borrowed from next turn's skill; next turn's skill = nominal (minus what was borrowed)
   Numbered: BUG #226 (BUGS_AND_QUIRKS.txt) */
void Turn_Overland_Casting(void)
{
    int player;
    int amount;

    for (player = 0; player < num_players; player++) {
        if (time_stop_player == 0 || player + 1 == time_stop_player) {
            wizards[player].skill_left_this_turn += Fortress_Hero_Skill_Bonus(player);
            wizards[player].nominal_casting_skill = Wizard_Casting_Skill(player);
            if (wizards[player].casting_spell > 0) {
                if (wizards[player].skill_left_this_turn > wizards[player].mana_reserve)
                    amount = wizards[player].mana_reserve;
                else
                    amount = wizards[player].skill_left_this_turn;
                if (wizards[player].casting_cost_left < amount)
                    amount = wizards[player].casting_cost_left;
                if (player == PLAYER_HUMAN || wizards[player].casting_spell != SPELL_SPELL_OF_RETURN) {
                    wizards[player].casting_cost_left -= amount;
                    wizards[player].mana_reserve -= amount;
                } else {
                    wizards[player].casting_cost_left -= wizards[player].nominal_casting_skill;
                    if (wizards[player].casting_cost_left < 0)
                        wizards[player].casting_cost_left = 0;
                    wizards[player].mana_reserve -= wizards[player].nominal_casting_skill;
                    if (wizards[player].mana_reserve < 0)
                        wizards[player].mana_reserve = 0;
                }
                amount = 0;
                wizards[player].skill_left_this_turn = 0;
                if (wizards[player].casting_cost_left > 0) {
                    if (wizards[player].nominal_casting_skill > wizards[player].mana_reserve)
                        amount = wizards[player].mana_reserve;
                    else
                        amount = wizards[player].nominal_casting_skill;
                    if (wizards[player].casting_cost_left < amount)
                        amount = wizards[player].casting_cost_left;
                    if (wizards[player].casting_cost_left - amount == 0) {
                        wizards[player].casting_cost_left -= amount;
                        wizards[player].mana_reserve -= amount;
                        wizards[player].skill_left_this_turn = -amount;
                    }
                }
            }
            if (wizards[player].skill_left_this_turn < 0)
                wizards[player].skill_left_this_turn += wizards[player].nominal_casting_skill;
            else
                wizards[player].skill_left_this_turn = wizards[player].nominal_casting_skill;
        }
    }
}

/* per wizard with a fortress, not casting Spell of Return: a merchant (human only; fame / 25 + 2 %, x2 Famous, max
   10) offers a prefab item at 3x its cost if the treasury holds the FULL price (the Charismatic halving of
   item_price has no effect: Screen_Merchant_Offer never receives it);
   mercenaries and heroes only with fewer than 947 units: mercenaries (fame / 20 + 1 %, x2 Famous, max 10, AI +10)
   appear at the fortress; a hero (fame / 25 + 3 %, x2 Famous, max 10, AI +10, divided by (heroes + 1) / 2 + 1). BUG #3: fame is read for the
   human only, the AI wizards use the human's */
void Turn_Random_Offers(void)
{
    int i;
    int fame;
    int unit_type;
    int free_slot;
    int x;
    int y;
    int plane;
    int count;
    int cost;
    int level;
    int accept;
    int item_price;
    int books[5];
    int player;
    int chance;

    for (i = 0; i < 5; i++)
        books[i] = MAX_REALM_BOOKS + 1;          /* more than any wizard can have: no book limit */
    for (player = 0; player < num_players; player++) {
        if (wizards[player].casting_spell == SPELL_SPELL_OF_RETURN || p_fortresses[player].active == 0)
            continue;
        if (player == PLAYER_HUMAN) {
            fame = Wizard_Effective_Fame(player);
            chance = fame / 25 + 2;
            if (wizards[player].retorts[RETORT_FAMOUS] > 0)
                chance *= 2;
            if (chance > 10)
                chance = 10;
            if (Random(100) <= chance) {
                merchant_item = Item_Create_Prefab(PREFAB_ANY, books, 0);
                item_price = p_items[merchant_item].cost * 3;
                if (wizards[PLAYER_HUMAN].gold >= item_price) {
                    if (wizards[PLAYER_HUMAN].retorts[RETORT_CHARISMATIC] > 0)
                        item_price = item_price / 2;
                    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                    Screen_Merchant_Offer();
                    Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
                } else
                    Item_Free(merchant_item);
            }
        }
        if (num_units < UNIT_CAP_AI - 3) {     /* room for the largest band (3, Mercenary_Offer_Roll) under UNIT_CAP_AI */
            chance = fame / 20 + 1;
            if (wizards[player].retorts[RETORT_FAMOUS] > 0)
                chance *= 2;
            if (chance > 10)
                chance = 10;
            if (player > 0)
                chance += 10;
            if (Random(100) <= chance) {
                unit_type = 0;
                Mercenary_Offer_Roll(player, &x, &y, &plane, &count, &unit_type, &cost, &level);
                x = p_fortresses[player].x;
                y = p_fortresses[player].y;
                plane = p_fortresses[player].plane;
                if (count > 0 && num_units + count < MAX_UNITS && wizards[player].gold >= cost) {
                    accept = 1;
                    if (player == PLAYER_HUMAN) {
                        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                        accept = Screen_Hire_Mercenaries(unit_type, count, level, cost);
                        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
                    }
                    if (accept == 1) {
                        wizards[player].gold -= cost;
                        for (i = 0; i < count; i++) {
                            Unit_Create(unit_type, player, x, y, plane, UNIT_SRC_NONE);
                            p_units[num_units - 1].level = level;
                            p_units[num_units - 1].experience = exp_level_thresholds[level];
                            p_units[num_units - 1].finished = 0;
                            p_units[num_units - 1].moves_left = p_units[num_units - 1].moves_max;
                            Stack_Limit_Enforce(num_units - 1);
                        }
                        if (player == PLAYER_HUMAN) {
                            next_unit_ref_x = x;
                            next_unit_ref_y = y;
                            map_view_plane = plane;
                            Player_Stacks_Changed_Nop(player);
                        }
                    }
                }
            }
            chance = fame / 25 + 3;
            if (wizards[player].retorts[RETORT_FAMOUS] > 0)
                chance *= 2;
            if (chance > 10)
                chance = 10;
            if (player > 0)
                chance += 10;
            chance = chance / ((Wizard_Hero_Count(player) + 1) / 2 + 1);
            if (Random(100) <= chance && num_units + 1 < MAX_UNITS) {
                free_slot = Wizard_Free_Hero_Slot(player);
                unit_type = Hero_Pick_Random(player, HPICK_OFFER, HPICK_POOL_ALL);
                if (free_slot > -1 && unit_type > -1) {
                    if (player == PLAYER_HUMAN) {
                        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                        Screen_Hire_Hero(free_slot, unit_type, HIRE_OFFER_HIRE);
                        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
                    } else
                        AI_Hire_Hero(player, free_slot, unit_type);
                }
            }
        }
    }
}

/* per wizard: a finished research (left 0, spell > 0) is learned; with no research set (spell < 0, or none before
   turn 2) the human picks one (music restarted after a learned spell), an AI by AI_Choose_Research; research left =
   the new spell's cost (Spell of Mastery: the wizard's mastery cost) */
void Turn_Research_Complete(void)
{
    int player;
    int learned;

    learned = 0;
    for (player = 0; player < num_players; player++) {
        if (wizards[player].research_left == 0 && wizards[player].research_spell > 0) {
            Wizard_Learn_Spell(player, wizards[player].research_spell, 1);
            learned = 1;
        }
        if (wizards[player].research_spell < 0 || (wizards[player].research_spell == SPELL_NONE && game_turn < 2)) {
            if (player == PLAYER_HUMAN) {
                Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                Screen_Choose_Research();
                Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
                if (learned == 1) {
                    Sound_Stop();
                    Music_Play_Current();
                }
            } else
                AI_Choose_Research(player);
            wizards[player].research_left = wizards[player].research_spell == SPELL_SPELL_OF_MASTERY ?
                wizards[player].mastery_research_cost : p_spell_data[wizards[player].research_spell].research_cost;
        }
    }
}

/* (player, spell, show) the spell goes into the library (state 2) and the mastery cost drops by half its research
   cost; if it was the research target the candidate list is fixed and refilled (no candidates: research none,
   else the negated spell asks for a new pick), else it is just dropped from the candidates; the human sees the
   spell (show 1), an AI that finished its research picks the next one
   Numbered: BUG #264 (BUGS_AND_QUIRKS.txt) */
void Wizard_Learn_Spell(int player, int spell, int show)
{
    int row;
    int col;
    int researched;
    int refilled;
    int k;

    researched = 0;
    if (wizards[player].mastery_research_cost > (unsigned)p_spell_data[spell].research_cost / 2)
        wizards[player].mastery_research_cost -= (unsigned)p_spell_data[spell].research_cost / 2;
    else
        wizards[player].mastery_research_cost = 0;
    row = SPELL_REALM(spell);
    col = SPELL_SLOT(spell);
    wizards[player].spell_library[row][col] = SPELLSTATE_KNOWN;
    if (wizards[player].research_spell == spell) {
        researched = 1;
        for (k = 0; k < 8; k++) {
            if (wizards[player].research_candidates[k] == wizards[player].research_spell) {
                Array_Delete_Near(k, (char *)wizards[player].research_candidates, 2, 8);
                wizards[player].research_candidates[7] = 0;
            }
        }
        refilled = Research_Candidates_Refill(player);
        wizards[player].research_left = 0;
        if (refilled == 0)
            wizards[player].research_spell = 0;
        else
            wizards[player].research_spell = -wizards[player].research_spell;
    } else {
        for (k = 0; k < 8; k++) {
            if (wizards[player].research_candidates[k] == spell) {
                Array_Delete_Near(k, (char *)wizards[player].research_candidates, 2, 8);
                wizards[player].research_candidates[7] = 0;
                Research_Candidates_Refill(player);
            }
        }
    }
    if (player == PLAYER_HUMAN && show == 1) {
        Screen_Spell_Learned(spell, researched);
        if (researched == 0) {
            Sound_Stop();
            Music_Play_Current();
        }
    } else if (researched == 1 && player != PLAYER_HUMAN) {
        AI_Choose_Research(player);
        wizards[player].research_left = wizards[player].research_spell == SPELL_SPELL_OF_MASTERY ?
            wizards[player].mastery_research_cost : p_spell_data[wizards[player].research_spell].research_cost;
    }
}

/* (unit, divisor, healer) total = max hits x figures; a healer's touch (healer != 0) heals total / 5 and marks the
   unit HEAL_MARK_HEALER; natural healing (0 < damage < total) heals one point per full divisor in total, one more on
   Random(divisor) <= the rest; damage floored at 0
   Numbered: BUG #298 (BUGS_AND_QUIRKS.txt) */
void Unit_Heal_Step(int unit, int divisor, int healer)
{
    int total;

    total = Unit_Max_Hits(unit) * unit_types[p_units[unit].type].figures;
    if (healer) {
        p_units[unit].damage = p_units[unit].damage - total / 5;
        if (p_units[unit].damage < 0)
            p_units[unit].damage = 0;
        p_unit_heal_divisor[unit] = HEAL_MARK_HEALER;
    } else if (p_units[unit].damage < total && p_units[unit].damage > 0) {
        while (total > divisor) {
            p_units[unit].damage--;
            total -= divisor;
        }
        if (Random(divisor) <= total)
            p_units[unit].damage--;
        if (p_units[unit].damage < 0)
            p_units[unit].damage = 0;
    }
}

/* healing: every unit's divisor is 20, 10 in a city, 6 with an Animists' Guild (only the first 9 of the owner's
   units there: City_Garrison_List); the removal of other players' units garrisoned there (UNIT_REMOVE_GONE) never
   fires - City_Garrison_List lists only the city owner's units; clarified using ReMoM repo. Units outside a Time
   Stop heal naturally (not race 20, not undead); a healer heals its stack (in a stack of 10+, neutral units are
   removed (UNIT_REMOVE_GONE), and all but the human's if the human has a unit there - a 10-unit stack is rare but
   possible, e.g. after a prisoner hero joins a 9-unit stack at a lair, Lair_Enter)
   Numbered: BUG #298 (BUGS_AND_QUIRKS.txt) */
void Turn_Heal_Units(void)
{
    int count;
    int found;
    int list[18];
    int k;
    int i;

    p_unit_heal_divisor = Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, (unsigned)num_units / 16 + 4));
    for (i = 0; i < num_units; i++)
        p_unit_heal_divisor[i] = 20;
    for (i = 0; i < num_cities; i++) {
        City_Garrison_List(i, &count, list);
        for (k = 0; k < count; k++)
            if (p_units[list[k]].owner != p_cities[i].owner)
                Unit_Kill_Or_Dismiss(list[k], UNIT_REMOVE_GONE);
        for (k = 0; k < count; k++)
            p_unit_heal_divisor[list[k]] = 10;
        if (p_cities[i].buildings[BLD_ANIMISTS_GUILD] > 0)
            for (k = 0; k < count; k++)
                p_unit_heal_divisor[list[k]] = 6;
    }
    for (i = 0; i < num_units; i++) {
        if (time_stop_player == 0 || p_units[i].owner == time_stop_player - 1) {
            if (unit_types[p_units[i].type].race != RACE_DEATH_CREATURE && (p_units[i].mutation_flags & MUT_UNDEAD) == 0)
                Unit_Heal_Step(i, p_unit_heal_divisor[i], 0);
            if (unit_types[p_units[i].type].ability_flags & AFLAG_HEALER) {
                Map_Units_At(p_units[i].x, p_units[i].y, p_units[i].plane, &count, list);
                if (count > MAX_STACK) {
                    found = 0;
                    for (k = 0; k < count; k++) {
                        if (p_units[list[k]].owner == PLAYER_NEUTRAL)
                            Unit_Kill_Or_Dismiss(list[k], UNIT_REMOVE_GONE);
                        if (p_units[list[k]].owner == PLAYER_HUMAN)
                            found = 1;
                    }
                    if (found == 1)
                        for (k = 0; k < count; k++)
                            if (p_units[list[k]].owner != PLAYER_HUMAN)
                                Unit_Kill_Or_Dismiss(list[k], UNIT_REMOVE_GONE);
                    Map_Units_At(p_units[i].x, p_units[i].y, p_units[i].plane, &count, list);
                }
                for (k = 0; k < count; k++)
                    if (unit_types[p_units[list[k]].type].race != RACE_DEATH_CREATURE && (p_units[list[k]].mutation_flags & MUT_UNDEAD) == 0 &&
                        p_unit_heal_divisor[list[k]] != HEAL_MARK_HEALER)
                        Unit_Heal_Step(list[k], 0, 1);
            }
        }
    }
}

/* per wizard: no spell -> the initial casting cost cleared; casting Spell of Mastery and each active global
   enchantment change the other wizards' relations (Global_Enchant_Relations); Great Wasting, Meteor Storm and
   Armageddon also take their turn effect */
void Turn_Global_Enchant_Diplomacy(void)
{
    int player;

    for (player = 0; player < num_players; player++) {
        if (wizards[player].casting_spell == SPELL_NONE)
            wizards[player].casting_cost_initial = 0;
        if (wizards[player].casting_spell == SPELL_SPELL_OF_MASTERY)
            Global_Enchant_Relations(player, SPELL_SPELL_OF_MASTERY, 2);
        if (wizards[player].globals.eternal_night > 0)
            Global_Enchant_Relations(player, SPELL_ETERNAL_NIGHT, 10);
        if (wizards[player].globals.evil_omens > 0)
            Global_Enchant_Relations(player, SPELL_EVIL_OMENS, 10);
        if (wizards[player].globals.zombie_mastery > 0)
            Global_Enchant_Relations(player, SPELL_ZOMBIE_MASTERY, 10);
        if (wizards[player].globals.aura_of_majesty > 0)
            Global_Enchant_Relations(player, SPELL_AURA_OF_MAJESTY, 10);
        if (wizards[player].globals.wind_mastery > 0)
            Global_Enchant_Relations(player, SPELL_WIND_MASTERY, 10);
        if (wizards[player].globals.suppress_magic > 0)
            Global_Enchant_Relations(player, SPELL_SUPPRESS_MAGIC, 10);
        if (wizards[player].globals.time_stop > 0)
            Global_Enchant_Relations(player, SPELL_TIME_STOP, 10);
        if (wizards[player].globals.nature_awareness > 0)
            Global_Enchant_Relations(player, SPELL_NATURE_AWARENESS, 10);
        if (wizards[player].globals.nature_s_wrath > 0)
            Global_Enchant_Relations(player, SPELL_NATURES_WRATH, 10);
        if (wizards[player].globals.herb_mastery > 0)
            Global_Enchant_Relations(player, SPELL_HERB_MASTERY, 10);
        if (wizards[player].globals.chaos_surge > 0)
            Global_Enchant_Relations(player, SPELL_CHAOS_SURGE, 10);
        if (wizards[player].globals.doom_mastery > 0)
            Global_Enchant_Relations(player, SPELL_DOOM_MASTERY, 10);
        if (wizards[player].globals.great_wasting > 0) {
            Great_Wasting_Turn(player);
            Global_Enchant_Relations(player, SPELL_GREAT_WASTING, 10);
        }
        if (wizards[player].globals.meteor_storm > 0) {
            Meteor_Storm_Turn(player);
            Global_Enchant_Relations(player, SPELL_METEOR_STORM, 10);
        }
        if (wizards[player].globals.armageddon > 0) {
            Armageddon_Turn(player);
            Global_Enchant_Relations(player, SPELL_ARMAGEDDON, 10);
        }
        if (wizards[player].globals.tranquility > 0)
            Global_Enchant_Relations(player, SPELL_TRANQUILITY, 10);
        if (wizards[player].globals.life_force > 0)
            Global_Enchant_Relations(player, SPELL_LIFE_FORCE, 10);
        if (wizards[player].globals.crusade > 0)
            Global_Enchant_Relations(player, SPELL_CRUSADE, 10);
        if (wizards[player].globals.just_cause > 0)
            Global_Enchant_Relations(player, SPELL_JUST_CAUSE, 10);
        if (wizards[player].globals.holy_arms > 0)
            Global_Enchant_Relations(player, SPELL_HOLY_ARMS, 10);
    }
}

/* per unit: Herb Mastery heals its owner's units fully (not race 20, not undead); a stasis unit (MUT_STASIS) leaves it when
   Roll_Resist(-5) returns 0, a new stasis (MUT_STASIS_NEW) becomes MUT_STASIS; non-fantastic, non-undead units gain 1 experience
   (Time Stop: the caster's only) and their level (the human's heroes see the level-up screen). Per wizard: each hired
   hero's stack (once) gets the best bonus of its heroes - Armsmaster 2 x (level + 1), else 3 x (level + 1) for
   Super Armsmaster (tested in that order) - added to every non-hero there. (A non-hero marks hero_done[-1]: a word inside bu, harmless) */
void Turn_Unit_Experience(void)
{
    int slot;
    int bonus;
    int best;
    int tile_count;
    int unit;
    int level;
    int tile_units[MAX_STACK];
    int hero_done[6];
    struct BattleUnit bu;
    int i;
    int k;

    for (i = 0; i < num_units; i++) {
        if (wizards[p_units[i].owner].globals.herb_mastery > 0 && unit_types[p_units[i].type].race != RACE_DEATH_CREATURE &&
            (p_units[i].mutation_flags & MUT_UNDEAD) == 0)
            p_units[i].damage = 0;
        if (p_units[i].mutation_flags & MUT_STASIS) {
            Battle_Unit_Init(i, &bu);
            if (Roll_Resist(bu, -5, REALM_SORCERY) == 0)
                p_units[i].mutation_flags = p_units[i].mutation_flags ^ MUT_STASIS;
        }
        if (p_units[i].mutation_flags & MUT_STASIS_NEW) {
            p_units[i].mutation_flags = p_units[i].mutation_flags | MUT_STASIS;
            p_units[i].mutation_flags = p_units[i].mutation_flags ^ MUT_STASIS_NEW;
        }
        if ((unit_types[p_units[i].type].ability_flags & AFLAG_FANTASTIC) == 0 && (p_units[i].mutation_flags & MUT_UNDEAD) == 0 &&
            (time_stop_player == 0 || p_units[i].owner == time_stop_player - 1)) {
            p_units[i].experience++;
            level = Unit_Calc_Level(i);
            if (p_units[i].hero_slot > -1 && p_units[i].owner == PLAYER_HUMAN && p_units[i].level < level)
                Screen_Hero_Level_Up(i);
            p_units[i].level = Unit_Calc_Level(i);
        }
    }
    for (i = 0; i < num_players; i++) {
        for (slot = 0; slot < 6; slot++)
            hero_done[slot] = 0;
        if (time_stop_player == 0 || time_stop_player - 1 == i) {
            for (slot = 0; slot < 6; slot++) {
                if (wizards[i].hired_heroes[slot].unit_index > -1 && hero_done[slot] == 0) {
                    unit = wizards[i].hired_heroes[slot].unit_index;
                    best = 0;
                    Map_Units_At(p_units[unit].x, p_units[unit].y, p_units[unit].plane, &tile_count, tile_units);
                    hero_done[slot] = 1;
                    for (k = 0; k < tile_count; k++) {
                        bonus = 0;
                        hero_done[p_units[tile_units[k]].hero_slot] = 1;
                        if (p_units[tile_units[k]].hero_slot > -1) {
                            if (p_hero_stats[i][p_units[tile_units[k]].type].abilities & HERO_ABILITY_ARMSMASTER)
                                bonus = (p_units[tile_units[k]].level + 1) * 2;
                            else if (p_hero_stats[i][p_units[tile_units[k]].type].abilities & HERO_ABILITY_SUPER_ARMSMASTER)
                                bonus = (p_units[tile_units[k]].level + 1) * 3;
                            if (bonus > best)
                                best = bonus;
                        }
                    }
                    if (best > 0) {
                        for (k = 0; k < tile_count; k++) {
                            if (p_units[tile_units[k]].hero_slot == -1) {
                                p_units[tile_units[k]].experience += best;
                                p_units[tile_units[k]].level = Unit_Calc_Level(tile_units[k]);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* each volcano turns into a mountain with 2% a turn (Random(100) < 3: 1 or 2); then the volcano count */
void Turn_Volcano_Decay(void)
{
    int x;
    int y;
    int plane;
    unsigned tile;

    for (plane = 0; plane < NUM_PLANES; plane++)
        for (x = 0; x < MAP_WIDTH; x++)
            for (y = 0; y < MAP_HEIGHT; y++) {
                tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
                if (tile == TILE_VOLCANO && Random(100) < 3)
                    Volcano_To_Mountain(x, y, plane);
            }
    Wizards_Count_Volcanoes();
}

/* from turn 100: AI units worth less than half their wizard's average unit cost are disbanded - every 25 turns (the
   average recomputed) the first such unit of each AI city's garrison, and every turn any such unit outside a city
   (not outpost builders, node melders, road builders, transports, heroes or the Trireme: type above UNIT_TRIREME) */
int AI_Disband_Obsolete_Units(void)
{
    int unit_type;
    int owner;
    char *seen;
    int garrison_count;
    int done;
    int garrison[MAX_STACK];
    int i;
    int k;

    if (game_turn < 100)
        return 0;
    seen = Near_Alloc_Reset(num_units);
    for (i = 0; i < num_units; i++)
        seen[i] = 0;
    if (game_turn % 25 == 0) {
        AI_Average_Unit_Cost();
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner != PLAYER_NEUTRAL && p_cities[i].owner != PLAYER_HUMAN) {
                City_Garrison_List(i, &garrison_count, garrison);
                for (k = 0; k < garrison_count; k++)
                    seen[garrison[k]] = 1;
                done = 0;
                for (k = 0; k < garrison_count && done == 0; k++) {
                    if (unit_types[p_units[garrison[k]].type].cost <
                        wizards[p_cities[i].owner].average_unit_cost / 2) {
                        Unit_Kill_Or_Dismiss(garrison[k], UNIT_REMOVE_KILLED);
                        done = 1;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner != PLAYER_NEUTRAL && p_cities[i].owner != PLAYER_HUMAN) {
                City_Garrison_List(i, &garrison_count, garrison);
                for (k = 0; k < garrison_count; k++)
                    seen[garrison[k]] = 1;
            }
        }
    }
    disbanded_count = 0;
    for (i = 0; i < num_units; i++) {
        owner = p_units[i].owner;
        unit_type = p_units[i].type;
        if (owner > PLAYER_HUMAN && owner < PLAYER_NEUTRAL && (unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 &&
            (unit_types[unit_type].ability_flags & AFLAG_MELD_WITH_NODE) == 0 && unit_types[unit_type].construction == 0 &&
            unit_types[unit_type].transport_capacity == 0 && unit_type > UNIT_TRIREME &&
            unit_types[unit_type].cost < wizards[owner].average_unit_cost / 2 && seen[i] == 0) {
            Unit_Kill_Or_Dismiss(i, UNIT_REMOVE_KILLED);
            disbanded_count++;
        }
    }
}

/* per AI wizard: the average cost of its units (not outpost builders, node melders, road builders or transports) */
void AI_Average_Unit_Cost(void)
{
    int owner;
    long cost_sum[6];
    int count[6];
    int i;
    int unit_type;

    for (i = 0; i < 6; i++) {
        cost_sum[i] = 0;
        count[i] = 0;
    }
    for (i = 0; i < num_units; i++) {
        owner = p_units[i].owner;
        unit_type = p_units[i].type;
        if (owner > PLAYER_HUMAN && owner < PLAYER_NEUTRAL && (unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) == 0 &&
            (unit_types[unit_type].ability_flags & AFLAG_MELD_WITH_NODE) == 0 && unit_types[unit_type].construction == 0 &&
            unit_types[unit_type].transport_capacity == 0) {
            cost_sum[owner] += unit_types[unit_type].cost;
            count[owner]++;
        }
    }
    for (i = 1; i < num_players; i++)
        if (count[i] != 0)
            wizards[i].average_unit_cost = cost_sum[i] / count[i];
}
