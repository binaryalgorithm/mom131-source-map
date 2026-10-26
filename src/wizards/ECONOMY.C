/* ECONOMY.C - WIZARDS.EXE overlay code segment 9ef8 (stub 2569): the per-turn economy - a city's six resources from
   their calculators (AI cities scaled by the difficulty table), each wizard's magic power base (nodes, conjunctions,
   masteries, warped nodes, city power, volcanoes), gold / food / mana upkeep of units, city and global enchantments,
   the top-bar income summary, power split into mana / skill / research (Mana Focusing, Sage heroes, research
   bonuses), effective fame, unit levels from experience (Heroism, Warlord, Crusade), killing or dismissing a unit
   (the hero's items and hire state) and the nine-units-per-square limit (MAX_STACK; it acts on a tenth).
   No data or BSS of its own; the upkeep tables (DS:1f18, 1f58, 1f8c) and the difficulty table belong to other modules.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 9ef8 src/wizards/ECONOMY.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void City_Recalculate(int city);
void Wizard_Power_Base(void);
int Unit_Gold_Upkeep(int unit);
int Wizard_Gold_Upkeep(int player);
int Wizard_Food_Upkeep(int player);
void Units_Food_Upkeep_All(int *out);
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana);
int Wizard_Mana_Upkeep(int player);
int Unit_Mana_Upkeep(int unit);
int City_Enchant_Mana_Upkeep(int city, int player);
int Global_Enchant_Mana_Upkeep(int player);
int Units_Same_Tile(int a, int b);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void Sort_Bytes_Desc(char *array, int n);
void Map_Units_At(int x, int y, int plane, int *count, int *list);
int Wizard_Religion_Total(int player);
void Wizard_Magic_Split(int *mana, int *skill, int *research, int player);
void Wizard_Magic_Income(int *mana, int *research, int *skill, int player);
int Research_Bonus_Pct(int player, int spell);
int Wizard_Effective_Fame(int player);
int Wizard_Free_Hero_Slot(int player);
int City_Calc_Building_Upkeep(int city);
int Unit_Calc_Level(int unit);
void Unit_Bump_Or_Report(int unit);
void Stack_Limit_Enforce(int unit);
int Unit_Level_From_Experience(int unit);

#include "GAME.H"

extern int unit_ench_upkeep[32];                /* DS:1f18  mana upkeep per unit enchantment bit (ReMoM TBL_UE_Upkeep; clarified using ReMoM repo) */
extern int city_ench_upkeep[26];                /* DS:1f58 */
extern int global_ench_upkeep[24];              /* DS:1f8c */
extern struct DifficultyInfo difficulty_table[]; /* DS:1fea */
extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern struct Events far *p_events;             /* DS:9998 */
extern char report_units_lost_count;            /* DS:9b94 */
extern struct ReportUnitLost report_units_lost[];       /* DS:9b96 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */
extern int difficulty;                          /* DS:bd96 */
extern int magic_setting;                       /* DS:bd98 */

void EMS_Map_Default(void);
int City_Calc_Food(int city);
int City_Calc_Production(int city);
int City_Calc_Gold(int city);
int City_Calc_Research(int city);
int City_Calc_Power(int city);
void Item_Free(int item);
void Unit_Bump_To_Adjacent(int unit);
void far_strcpy();                              /* unprototyped: called here with (far dst, near src, 0) */
int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */

/* (city) recomputes the city's food, production, gold, building upkeep, research and power; an AI wizard's city
   (not the human's or neutral) has each scaled by the difficulty table's percentages
   Numbered: BUG #126 (BUGS_AND_QUIRKS.txt) */
void City_Recalculate(int city)
{
    int diff;

    EMS_Map_Default();
    p_cities[city].food_income = City_Calc_Food(city);
    p_cities[city].production_income = City_Calc_Production(city);
    p_cities[city].gold_income = City_Calc_Gold(city);
    p_cities[city].building_upkeep = City_Calc_Building_Upkeep(city);
    p_cities[city].research_income = City_Calc_Research(city);
    p_cities[city].power_income = City_Calc_Power(city);
    if (p_cities[city].owner != PLAYER_HUMAN && p_cities[city].owner != PLAYER_NEUTRAL) {
        diff = difficulty;
        p_cities[city].food_income = (long)p_cities[city].food_income * difficulty_table[diff].food_pct / 100;
        p_cities[city].production_income = (long)p_cities[city].production_income * difficulty_table[diff].production_pct / 100;
        p_cities[city].power_income = (long)p_cities[city].power_income * difficulty_table[diff].power_pct / 100;
        p_cities[city].gold_income = (long)p_cities[city].gold_income * difficulty_table[diff].gold_pct / 100;
        p_cities[city].research_income = (long)p_cities[city].research_income * difficulty_table[diff].research_pct / 100;
        p_cities[city].building_upkeep = (long)p_cities[city].building_upkeep * difficulty_table[diff].upkeep_pct / 100;
    }
}

/* () every wizard's power_base: nothing during Mana Short; else the owned nodes (warped: -5; else power x
   (magic setting + 1) / 2, halved / doubled by the three Conjunctions, doubled for the realm's Mastery and again
   for Node Mastery), the AI wizards' total scaled by the difficulty, + the cities' power, + the volcanoes;
   0 while casting Spell of Return, never below 0 */
void Wizard_Power_Base(void)
{
    int player;
    int power[6];
    int i;
    int node_power;

    for (i = 0; i < num_players; i++) {
        wizards[i].power_base = 0;
        power[i] = 0;
    }
    if (p_events->mana_short != EVENT_STATE_RUNNING) {
        for (i = 0; i < NUM_NODES; i++) {
            if (p_nodes[i].owner > -1) {
                player = p_nodes[i].owner;
                if ((p_nodes[i].meld_flags & NODE_WARPED) > 0) {
                    wizards[player].power_base -= 5;
                } else {
                    node_power = p_nodes[i].power * (magic_setting + 1) / 2;
                    if (p_events->conjunction_chaos == EVENT_STATE_RUNNING) {
                        if (p_nodes[i].realm == NODE_SORCERY) node_power /= 2;
                        if (p_nodes[i].realm == NODE_NATURE) node_power /= 2;
                        if (p_nodes[i].realm == NODE_CHAOS) node_power *= 2;
                    }
                    if (p_events->conjunction_sorcery == EVENT_STATE_RUNNING) {
                        if (p_nodes[i].realm == NODE_SORCERY) node_power *= 2;
                        if (p_nodes[i].realm == NODE_NATURE) node_power /= 2;
                        if (p_nodes[i].realm == NODE_CHAOS) node_power /= 2;
                    }
                    if (p_events->conjunction_nature == EVENT_STATE_RUNNING) {
                        if (p_nodes[i].realm == NODE_SORCERY) node_power /= 2;
                        if (p_nodes[i].realm == NODE_NATURE) node_power *= 2;
                        if (p_nodes[i].realm == NODE_CHAOS) node_power /= 2;
                    }
                    if (p_nodes[i].realm == NODE_SORCERY && wizards[player].retorts[RETORT_SORCERY_MASTERY] > 0) node_power *= 2;
                    if (p_nodes[i].realm == NODE_CHAOS && wizards[player].retorts[RETORT_CHAOS_MASTERY] > 0) node_power *= 2;
                    if (p_nodes[i].realm == NODE_NATURE && wizards[player].retorts[RETORT_NATURE_MASTERY] > 0) node_power *= 2;
                    if (wizards[player].retorts[RETORT_NODE_MASTERY] > 0) node_power *= 2;
                    wizards[player].power_base += node_power;
                }
            }
        }
        for (i = 1; i < num_players; i++) {     /* the AI wizards only: their node power by difficulty */
            wizards[i].power_base = (long)wizards[i].power_base * difficulty_table[difficulty].power_pct / 100;
        }
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner > -1 && p_cities[i].owner != PLAYER_NEUTRAL) {
                wizards[p_cities[i].owner].power_base += p_cities[i].power_income;
            }
        }
        for (i = 0; i < num_players; i++) {
            if (wizards[i].casting_spell == SPELL_SPELL_OF_RETURN) {
                wizards[i].power_base = 0;
            } else {
                wizards[i].power_base += wizards[i].volcanoes;
                if (wizards[i].power_base < 0) {
                    wizards[i].power_base = 0;
                }
            }
        }
    }
}

/* (unit) -> its gold upkeep: the type's upkeep for a normal unit (heroes: not Noble, not the Chosen), 0 for
   fantastic and undead units; an AI unit pays half on Impossible, 3 / 4 on Hard
   Numbered: BUG #90 (BUGS_AND_QUIRKS.txt) */
int Unit_Gold_Upkeep(int unit)
{
    int owner;
    int upkeep;

    owner = p_units[unit].owner;
    upkeep = 0;
    if ((unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0) {
        if (p_units[unit].type > UNIT_HERO_CHOSEN) {
            upkeep += unit_types[p_units[unit].type].upkeep;
        } else {
            if ((p_hero_stats[owner][p_units[unit].type].abilities & HERO_ABILITY_NOBLE) == 0
             && p_units[unit].type != UNIT_HERO_CHOSEN) {
                upkeep += unit_types[p_units[unit].type].upkeep;
            }
        }
    }
    if (p_units[unit].mutation_flags & MUT_UNDEAD) {
        upkeep = 0;
    }
    if (p_units[unit].owner != PLAYER_HUMAN) {
        if (difficulty == DIFFICULTY_IMPOSSIBLE) {
            upkeep /= 2;
        }
        if (difficulty == DIFFICULTY_HARD) {
            upkeep = upkeep * 3 / 4;
        }
    }
    return upkeep;
}

/* (player) -> the units' gold upkeep less the effective fame, at least 0 */
int Wizard_Gold_Upkeep(int player)
{
    int total;
    int fame;
    int unit;

    total = 0;
    fame = Wizard_Effective_Fame(player);
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner == player) {
            total += Unit_Gold_Upkeep(unit);
        }
    }
    total -= fame;
    if (total < 0) {
        total = 0;
    }
    return total;
}

/* (player) -> the food upkeep: one per normal (not fantastic, not undead) non-hero unit */
int Wizard_Food_Upkeep(int player)
{
    int count;
    int unit;

    count = 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner == player && (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0
         && p_units[unit].type > UNIT_HERO_CHOSEN && (p_units[unit].mutation_flags & MUT_UNDEAD) == 0) {
            count++;
        }
    }
    return count;
}

/* (out[6]) the same count for every player at once - but the owner test is <= num_players, so owner num_players
   and a dead unit (PLAYER_NONE, -1: out[-1] is written) count too
   Numbered: BUG #81 (BUGS_AND_QUIRKS.txt) */
void Units_Food_Upkeep_All(int *out)
{
    int unit;

    for (unit = 0; unit < 6; unit++) {
        out[unit] = 0;
    }
    for (unit = 0; unit < num_units; unit++) {
        if ((unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0 && p_units[unit].type > UNIT_HERO_CHOSEN
         && p_units[unit].owner <= num_players && (p_units[unit].mutation_flags & MUT_UNDEAD) == 0) {
            out[p_units[unit].owner]++;
        }
    }
}

/* (player, &gold, &food, &mana) the net income per turn: mana = magic income - mana upkeep; food = the cities'
   positive surpluses (deficits ignored) - food upkeep; gold = the cities' gold - building upkeep, + 10 per Noble hero, + half the food
   balance, - gold upkeep. AI upkeep is scaled by the difficulty. The player under its own Time Stop: 0 gold,
   0 food, -200 mana. (The first call passes &food_in / &gold_in as the research / skill outputs; both are
   zeroed right after) */
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana)
{
    int gold_upkeep;
    int food_upkeep;
    int mana_upkeep;
    int gold_in;
    int food_in;
    int mana_in;
    int city;                                   /* also the hero-slot loop (0 .. 5) */

    Wizard_Magic_Income(&mana_in, &food_in, &gold_in, player);
    gold_upkeep = 0;
    gold_in = 0;
    food_upkeep = 0;
    food_in = 0;
    mana_upkeep = 0;
    if (time_stop_player - 1 == player) {
        *food = 0;
        *gold = 0;
        *mana = -TIME_STOP_UPKEEP;              /* the Time Stop caster */
    } else {
        mana_upkeep = Wizard_Mana_Upkeep(player);
        gold_upkeep = Wizard_Gold_Upkeep(player);
        food_upkeep = Wizard_Food_Upkeep(player);
        if (player != PLAYER_HUMAN) {
            mana_upkeep = difficulty_table[difficulty].upkeep_pct * mana_upkeep / 100;
            gold_upkeep = difficulty_table[difficulty].upkeep_pct * gold_upkeep / 100;
            food_upkeep = difficulty_table[difficulty].upkeep_pct * food_upkeep / 100;
        }
        for (city = 0; city < num_cities; city++) {
            if (p_cities[city].owner == player) {
                gold_in += p_cities[city].gold_income - p_cities[city].building_upkeep;
                if ((unsigned char)p_cities[city].population < p_cities[city].food_income) {
                    food_in += p_cities[city].food_income - p_cities[city].population;
                }
            }
        }
        for (city = 0; city < 6; city++) {      /* here: the six hero slots */
            if (wizards[player].hired_heroes[city].unit_index > -1
             && (p_hero_stats[player][p_units[wizards[player].hired_heroes[city].unit_index].type].abilities & HERO_ABILITY_NOBLE) != 0) {
                gold_in += 10;
            }
        }
        *food = food_in - food_upkeep;
        gold_in += *food / 2;
        *gold = gold_in - gold_upkeep;
        *mana = mana_in - mana_upkeep;
    }
}

/* (player) -> the mana upkeep of its units, city enchantments and global enchantments; halved for a
   Channeler */
int Wizard_Mana_Upkeep(int player)
{
    int total;
    int i;

    total = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == player) {
            total += Unit_Mana_Upkeep(i);
        }
    }
    for (i = 0; i < num_cities; i++) {
        total += City_Enchant_Mana_Upkeep(i, player);
    }
    total += Global_Enchant_Mana_Upkeep(player);
    if (wizards[player].retorts[RETORT_CHANNELER] > 0) {
        total /= 2;
    }
    return total;
}

/* (unit) -> its mana upkeep: a fantastic unit's upkeep (x 3 / 2 when undead) or the Chosen's, 3 / 4 (rounded
   up) for a Conjurer, + unit_ench_upkeep of every enchantment bit */
int Unit_Mana_Upkeep(int unit)
{
    int bit;
    long ench;
    int set;
    int upkeep;

    upkeep = 0;
    ench = p_units[unit].enchantments;
    if (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) {
        if (p_units[unit].mutation_flags & MUT_UNDEAD) {
            upkeep += unit_types[p_units[unit].type].upkeep * 3 / 2;
        } else {
            upkeep += unit_types[p_units[unit].type].upkeep;
        }
    }
    if (p_units[unit].type == UNIT_HERO_CHOSEN) {
        upkeep += unit_types[p_units[unit].type].upkeep;
    }
    if (p_units[unit].owner < num_players && wizards[p_units[unit].owner].retorts[RETORT_CONJURER] > 0) {
        upkeep = (upkeep * 3 + 3) / 4;
    }
    for (bit = 0; bit < 32; bit++) {
        set = (int)ench & 1;
        if (set == 1) {
            upkeep += unit_ench_upkeep[bit];
        }
        ench >>= 1;
    }
    return upkeep;
}

/* (city, player) -> the upkeep of the city enchantments the player cast there (enchants[i] = player + 1) */
int City_Enchant_Mana_Upkeep(int city, int player)
{
    char far *ench;
    int total;
    int i;

    total = 0;
    ench = p_cities[city].enchants;
    for (i = 0; i < NUM_CITY_ENCHANTS; i++) {
        if (ench[i] == player + 1) {
            total += city_ench_upkeep[i];
        }
    }
    return total;
}

/* (player) -> the upkeep of the player's active global enchantments (the 24 bytes of Wizard.globals) */
int Global_Enchant_Mana_Upkeep(int player)
{
    unsigned char *globals;
    int total;
    int i;

    total = 0;
    globals = (unsigned char *)&wizards[player].globals;
    for (i = 0; i < NUM_GLOBAL_ENCHANTS; i++) {
        if (globals[i] > 0) {
            total += global_ench_upkeep[i];
        }
    }
    return total;
}

/* (a, b) -> 1 when the two units stand on the same square of the same plane */
int Units_Same_Tile(int a, int b)
{
    if (p_units[a].x == p_units[b].x && p_units[a].y == p_units[b].y && p_units[a].plane == p_units[b].plane) {
        return 1;
    } else {
        return 0;
    }
}

/* (unit, mode) mode 0 killed (a hero resurrectable: status -(level + 1)), 1 dismissed (items freed, status = the
   level: hireable again), 2 gone (HERO_STATUS_UNAVAILABLE). The Chosen always takes the dismissed path; a killed or
   gone hero's items are only unlinked (item_index -1), not Item_Free'd */
void Unit_Kill_Or_Dismiss(int unit, int mode)
{
    int owner;
    int k;

    owner = p_units[unit].owner;
    p_units[unit].level = Unit_Level_From_Experience(unit);
    if (mode == UNIT_REMOVE_DISMISSED || p_units[unit].type == UNIT_HERO_CHOSEN) {
        p_units[unit].finished = 1;
        if (p_units[unit].hero_slot > -1) {
            for (k = 0; k < 3; k++) {
                if (wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[k] > -1) {
                    Item_Free(wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[k]);
                }
                wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[k] = -1;
            }
            wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].unit_index = -1;
            p_hero_stats[p_units[unit].owner][p_units[unit].type].status = p_units[unit].level;   /* HeroStats.status
                                                   >= 0: the level, offered again */
            if (p_units[unit].owner == PLAYER_HUMAN) {
                p_hero_names[p_units[unit].type].experience = p_units[unit].experience;
                far_strcpy(p_hero_names[p_units[unit].type].name,
                           wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].name, 0);
            }
        }
    } else {
        p_units[unit].finished = 1;
        if (p_units[unit].hero_slot > -1) {
            for (k = 0; k < 3; k++) {
                wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[k] = -1;
            }
            wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].unit_index = -1;
            if (mode != UNIT_REMOVE_GONE) {
                if (p_units[unit].owner == PLAYER_HUMAN) {
                    p_hero_names[p_units[unit].type].experience = p_units[unit].experience;
                    far_strcpy(p_hero_names[p_units[unit].type].name,
                               wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].name, 0);
                }
                p_hero_stats[p_units[unit].owner][p_units[unit].type].status = -(p_units[unit].level + 1);   /* dead:
                                                   -(level + 1), can be resurrected */
            } else {
                p_hero_stats[p_units[unit].owner][p_units[unit].type].status = HERO_STATUS_UNAVAILABLE;
            }
        }
    }
    p_units[unit].owner = PLAYER_NONE;
    p_units[unit].plane = -1;
}

/* insertion sort, largest first; no callers */
void Sort_Bytes_Desc(char *array, int n)
{
    char key;
    int i;
    char cur;
    char key2;
    char j;

    for (i = 1; i < n; i++) {
        key = array[i];
        j = i - 1;
        cur = array[j];
        key2 = key;
        while (j > -1 && cur < key2) {
            array[j + 1] = array[j];
            j--;
            if (j > -1) {
                cur = array[j];
            }
        }
        array[j + 1] = key;
    }
}

/* (x, y, plane, &count, list) the living units on the square (no tower rule, no limit) */
void Map_Units_At(int x, int y, int plane, int *count, int *list)
{
    int n;
    int unit;

    n = 0;
    *count = 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].owner != PLAYER_NONE && p_units[unit].plane == plane) {
            list[n] = unit;
            n = n + 1;
        }
    }
    *count = n;
}

/* (player) no callers: the sum over its cities of building state (1 built; 2 removed - counted double) x
   Building.religion_weight */
int Wizard_Religion_Total(int player)
{
    int total;
    int city;
    int bld;

    total = 0;
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == player) {
            for (bld = 0; bld < NUM_BUILDINGS; bld++) {
                if (p_cities[city].buildings[bld] >= 1) {
                    total += p_cities[city].buildings[bld] * p_building_data[bld].religion_weight;
                }
            }
        }
    }
    return total;
}

/* (&mana, &skill, &research, player) power_base split by the player's percentages (rounded; research gets the
   rest - moved to skill, else mana, when the research share is 0); Mana Focusing +25 % mana; research + the
   cities' research + Sage heroes (3 x (level + 1), Super Sage 4.5 x). The values may be negative */
void Wizard_Magic_Split(int *mana, int *skill, int *research, int player)
{
    int power;
    int mana_v;
    int skill_v;
    int research_v;
    int city_research;
    int i;

    power = wizards[player].power_base;
    mana_v = ((long)wizards[player].mana_percentage * power + 50) / 100;
    skill_v = ((long)wizards[player].skill_percentage * power + 50) / 100;
    research_v = power - mana_v - skill_v;
    if (wizards[player].research_percentage == 0 && research_v > 0) {
        if (wizards[player].skill_percentage != 0) {
            skill_v += research_v;
        } else {
            mana_v += research_v;
        }
        research_v = 0;
    }
    if (wizards[player].retorts[RETORT_MANA_FOCUSING] > 0) {
        mana_v += mana_v / 4;
    }
    city_research = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player) {
            city_research += p_cities[i].research_income;
        }
    }
    research_v += city_research;
    for (i = 0; i < 6; i++) {
        if (wizards[player].hired_heroes[i].unit_index > -1) {
            if ((p_hero_stats[player][p_units[wizards[player].hired_heroes[i].unit_index].type].abilities & HERO_ABILITY_SAGE) != 0) {
                research_v += (p_units[wizards[player].hired_heroes[i].unit_index].level + 1) * 3;
            }
            if ((p_hero_stats[player][p_units[wizards[player].hired_heroes[i].unit_index].type].abilities & HERO_ABILITY_SUPER_SAGE) != 0) {
                research_v += (p_units[wizards[player].hired_heroes[i].unit_index].level + 1) * 9 / 2;
            }
        }
    }
    *mana = mana_v;
    *skill = skill_v;
    *research = research_v;
}

/* (&mana, &research, &skill, player) Wizard_Magic_Split floored at 0, research + Research_Bonus_Pct for the
   spell being researched; all 0 while casting Spell of Return. NOTE the parameter order differs from
   Wizard_Magic_Split's (research and skill swapped) */
void Wizard_Magic_Income(int *mana, int *research, int *skill, int player)
{
    int bonus;
    int mana_v;
    int research_v;
    int skill_v;

    if (wizards[player].casting_spell != SPELL_SPELL_OF_RETURN) {
        Wizard_Magic_Split(&mana_v, &skill_v, &research_v, player);
        bonus = Research_Bonus_Pct(player, wizards[player].research_spell);
        if (research_v < 0) {
            research_v = 0;
        }
        if (mana_v < 0) {
            mana_v = 0;
        }
        if (skill_v < 0) {
            skill_v = 0;
        }
        research_v += (long)research_v * bonus / 100;
        *mana = mana_v;
        *research = research_v;
        *skill = skill_v;
    } else {
        *mana = 0;
        *research = 0;
        *skill = 0;
    }
}

/* (player, spell) -> the research bonus %: Sage Master 25, Conjurer 25 for summons, the realm's Mastery 15
   (Chaos, Nature, Sorcery), 10 per book of the realm above 7, Runemaster 25 for Arcane */
int Research_Bonus_Pct(int player, int spell)
{
    int bonus;
    int realm;

    bonus = 0;
    realm = p_spell_data[spell].realm;
    if (wizards[player].retorts[RETORT_SAGE_MASTER] > 0) {
        bonus += 25;
    }
    if (wizards[player].retorts[RETORT_CONJURER] > 0 && p_spell_data[spell].effect_type == SPELL_FX_SUMMON) {
        bonus += 25;
    }
    switch (realm) {
    case REALM_CHAOS:
        if (wizards[player].retorts[RETORT_CHAOS_MASTERY] > 0) {
            bonus += 15;
        }
        if (wizards[player].spellbooks[REALM_CHAOS] > 7) {
            bonus += (wizards[player].spellbooks[REALM_CHAOS] - 7) * 10;
        }
        break;
    case REALM_NATURE:
        if (wizards[player].retorts[RETORT_NATURE_MASTERY] > 0) {
            bonus += 15;
        }
        if (wizards[player].spellbooks[REALM_NATURE] > 7) {
            bonus += (wizards[player].spellbooks[REALM_NATURE] - 7) * 10;
        }
        break;
    case REALM_SORCERY:
        if (wizards[player].retorts[RETORT_SORCERY_MASTERY] > 0) {
            bonus += 15;
        }
        if (wizards[player].spellbooks[REALM_SORCERY] > 7) {
            bonus += (wizards[player].spellbooks[REALM_SORCERY] - 7) * 10;
        }
        break;
    case REALM_LIFE:
        if (wizards[player].spellbooks[REALM_LIFE] > 7) {
            bonus += (wizards[player].spellbooks[REALM_LIFE] - 7) * 10;
        }
        break;
    case REALM_DEATH:
        if (wizards[player].spellbooks[REALM_DEATH] > 7) {
            bonus += (wizards[player].spellbooks[REALM_DEATH] - 7) * 10;
        }
        break;
    case REALM_ARCANE:
        if (wizards[player].retorts[RETORT_RUNEMASTER] > 0) {
            bonus += 25;
        }
        break;
    default:
        break;
    }
    return bonus;
}

/* fame + Legendary heroes + Just Cause: the fame of gold upkeep, merchants, heroes, mercenaries and the score */
int Wizard_Effective_Fame(int player)
{
    int total;
    int slot;
    int unit;

    total = 0;
    for (slot = 0; slot < 6; slot++) {
        if (wizards[player].hired_heroes[slot].unit_index > -1) {
            unit = wizards[player].hired_heroes[slot].unit_index;
            if ((p_hero_stats[player][p_units[unit].type].abilities & HERO_ABILITY_LEGENDARY) != 0) {
                total += (p_units[unit].level + 1) * 3;
            }
            if ((p_hero_stats[player][p_units[unit].type].abilities & HERO_ABILITY_SUPER_LEGENDARY) != 0) {
                total += (p_units[unit].level + 1) * 9 / 2;
            }
        }
    }
    if (wizards[player].globals.just_cause > 0) {
        total += 10;
    }
    total += wizards[player].fame;
    return total;
}

/* (player) -> the first empty hired-hero slot, or -1 */
int Wizard_Free_Hero_Slot(int player)
{
    int slot;
    int i;

    slot = -1;
    for (i = 0; i < 6; i++) {
        if (wizards[player].hired_heroes[i].unit_index == -1) {
            slot = i;
            break;
        }
    }
    return slot;
}

/* (city) -> the gold upkeep of the city's standing buildings (Building.gold_upkeep is stored negative) */
int City_Calc_Building_Upkeep(int city)
{
    int total;
    int bld;

    total = 0;
    for (bld = 0; bld < NUM_BUILDINGS; bld++) {
        if ((p_cities[city].buildings[bld] == BLDSTATE_BUILT || p_cities[city].buildings[bld] == BLDSTATE_REPLACED)
         && p_building_data[bld].gold_upkeep < 0) {
            total += abs(p_building_data[bld].gold_upkeep);
        }
    }
    return total;
}

/* (unit) -> the experience level (normal units to 3, heroes to 8; caps the experience), with Heroism, Warlord,
   Crusade. Side effect: a unit whose own level reached 3 loses its (now useless) Heroism enchantment */
int Unit_Calc_Level(int unit)
{
    int level;
    int k;

    level = p_units[unit].level;
    if (p_units[unit].type < (int)p_spell_data[SPELL_MAGIC_SPIRIT].effect_param) {   /* a normal unit: below the first
                                                                                     fantastic type (Magic Spirit's: UNIT_MAGIC_SPIRIT) */
        for (k = 0; k < 4; k++) {
            if (p_units[unit].experience >= exp_level_thresholds[k]) {
                level = k;
            }
        }
        if (p_units[unit].hero_slot > -1) {
            for (k = 4; k < 9; k++) {
                if (p_units[unit].experience >= exp_level_thresholds[k]) {
                    level = k;
                }
            }
            if (p_units[unit].experience >= exp_level_thresholds[8]) {
                p_units[unit].experience = exp_level_thresholds[8];
            }
        } else {
            if (p_units[unit].experience >= exp_level_thresholds[3]) {
                p_units[unit].experience = exp_level_thresholds[3];
            }
        }
        if (level >= 3 && (p_units[unit].enchantments & UNIT_ENCH_HEROISM) != 0) {
            p_units[unit].enchantments = p_units[unit].enchantments ^ UNIT_ENCH_HEROISM;
        }
        if ((p_units[unit].enchantments & UNIT_ENCH_HEROISM) != 0 && level < 3) {
            level = 3;
        }
        if (wizards[p_units[unit].owner].retorts[RETORT_WARLORD] > 0) {
            level++;
        }
        if (wizards[p_units[unit].owner].globals.crusade > 0 && p_units[unit].type < UNIT_MAGIC_SPIRIT
         && (p_units[unit].mutation_flags & MUT_UNDEAD) == 0) {
            level++;
        }
    }
    if (level > 8) {
        level = 8;
    }
    return level;
}

/* the experience level alone (as Unit_Calc_Level, without the enchantments and retorts) */
int Unit_Level_From_Experience(int unit)
{
    int level;
    int k;

    level = p_units[unit].level;
    if (p_units[unit].type < (int)p_spell_data[SPELL_MAGIC_SPIRIT].effect_param) {   /* a normal unit: below the first
                                                                                     fantastic type (Magic Spirit's: UNIT_MAGIC_SPIRIT) */
        for (k = 0; k < 4; k++) {
            if (p_units[unit].experience >= exp_level_thresholds[k]) {
                level = k;
            }
        }
        if (p_units[unit].hero_slot > -1) {
            for (k = 4; k < 9; k++) {
                if (p_units[unit].experience >= exp_level_thresholds[k]) {
                    level = k;
                }
            }
            if (p_units[unit].experience >= exp_level_thresholds[8]) {
                p_units[unit].experience = exp_level_thresholds[8];
            }
        } else {
            if (p_units[unit].experience >= exp_level_thresholds[3]) {
                p_units[unit].experience = exp_level_thresholds[3];
            }
        }
    }
    if (level > 8) {
        level = 8;
    }
    return level;
}

/* moves the unit (any owner) to a free adjacent square (Unit_Bump_To_Adjacent); a human unit that had none and was
   removed is listed in the turn report (UNIT_LOST_ROOM) */
void Unit_Bump_Or_Report(int unit)
{
    int human;

    if (p_units[unit].owner == PLAYER_HUMAN) {
        human = 1;
    } else {
        human = 0;
    }
    Unit_Bump_To_Adjacent(unit);
    if (p_units[unit].owner == PLAYER_NONE && human == 1 && report_units_lost_count < MAX_REPORT_ENTRIES) {
        report_units_lost[report_units_lost_count].unit_type = p_units[unit].type;
        report_units_lost[report_units_lost_count].reason = UNIT_LOST_ROOM;
        report_units_lost_count++;
    }
}

/* ten or more units on the unit's tile: the cheapest (gold + mana upkeep + level) leaves */
void Stack_Limit_Enforce(int unit)
{
    int i;
    int best_cost;
    int cost;
    int victim;
    int count;
    int x;
    int y;
    int plane;
    int units[10];

    victim = -999;
    best_cost = 999;
    x = p_units[unit].x;
    y = p_units[unit].y;
    plane = p_units[unit].plane;
    Map_Units_At(p_units[unit].x, p_units[unit].y, p_units[unit].plane, &count, units);
    if (count > MAX_STACK) {
        for (i = 0; i < count; i++) {
            cost = Unit_Gold_Upkeep(units[i]) + Unit_Mana_Upkeep(units[i]);
            cost += p_units[units[i]].level;
            if (cost <= best_cost) {
                best_cost = cost;
                victim = units[i];
            }
        }
        if (p_units[victim].owner == PLAYER_NEUTRAL) {
            Unit_Kill_Or_Dismiss(victim, UNIT_REMOVE_DISMISSED);
        } else {
            Unit_Bump_Or_Report(victim);
        }
    }
}
