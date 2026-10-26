/* AIPROD.C - WIZARDS.EXE code segment eab0: city production choices - the AI's per-turn pass over its cities
   (re-choose on "auto" or, 1 in 6, on Housing / Trade Goods; else maybe buy), the wizard AI's weighted choice
   (Trade Goods roll, the garrison checklist off the human's home landmass, building weights by category and
   objective, unit weights by cost, settler / engineer / transport / military rules), the neutral cities' choice,
   the AI's buying, and the Grand Vizier's choice for the human's cities.
   Data: one never-referenced word at DS:7114 (owner not fixed by the EXE: it may be AIUNITS.C's); no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS eab0 src/wizards/AIPROD.C */

/* the module's functions in their original declaration order */
void AI_City_Production_Turn(int player);
int AI_Wizard_Choose_Production(int player, int city);
void AI_Neutral_Choose_Production(int city);
void AI_City_Maybe_Buy(int player, int city);
int Vizier_Choose_Production(int city);

#include "GAME.H"

extern char ai_bldg_objective_bonus[8][5];      /* DS:203a */
extern char ai_unit_objective_bonus[5];         /* DS:2062 */
extern char ai_civilian_objective_bonus[5];     /* DS:2067 */
extern int ai_bldg_category_weight[10];         /* DS:206c */
extern int ai_own_transport_count;              /* DS:9094 */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern int game_turn;                           /* DS:bd90 */

int ai_prod_unused = 0;                         /* DS:7114  never referenced */

int Random(int n);
void City_Build_Options(int city, int *n_options, int *n_total, int *n_units, int *options);
int AI_Count_Units_At(int x, int y, int plane);
int Random_Weighted_Pick(int *weights, int count);
int City_Buy_Price(int city);
void City_Buy(int player, int city);

/* (player) each of the player's cities: a new choice when production is "auto" or, 1 in 6, Housing / Trade Goods
   (neutral cities by their own rule); otherwise a wizard may buy the current item */
void AI_City_Production_Turn(int player)
{
    int reroll;
    register int city;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == player) {
            reroll = 0;
            if (p_cities[city].producing == PROD_AUTO_CHOOSE)
                reroll = 1;
            if ((p_cities[city].producing == PROD_HOUSING || p_cities[city].producing == PROD_TRADE_GOODS) &&
                Random(6) == 1)
                reroll = 1;
            if (reroll == 1) {
                if (player == PLAYER_NEUTRAL)
                    AI_Neutral_Choose_Production(city);
                else
                    AI_Wizard_Choose_Production(player, city);
            } else if (p_cities[city].producing != PROD_HOUSING && p_cities[city].producing != PROD_TRADE_GOODS &&
                       player != PLAYER_NEUTRAL)
                AI_City_Maybe_Buy(player, city);
        }
    }
}

/* (player, city) -> 0 when a fixed rule set the production; the weighted pick is only stored in producing (no
   return value on that path): first, off the human's home landmass (or after turn 200), the garrison checklist 1+
   Barracks, 2+ Builders Hall, 3+ Shrine, 4+ Smithy, 5+ Granary; then Trade Goods when Random(100) <= (own cities / 8
   - Trade Goods count) * 10 + 40 (never for the human; BUG #145: counts the chosen city's Trade Goods once per own city);
   else Random_Weighted_Pick over City_Build_Options: building = category weight +
   objective bonus (Housing x2 when population < 5 - BUG #145: tested on city [option count]); unit = (cost / 10)^2;
   units_only (units here < Random(30) on the home landmass, Random(15) elsewhere) zeroes the buildings, else the
   military units; military * (10 + objective bonus) / 10; settlers (population > 2) / engineers only while none
   is on the landmass or in production, x1.5 before turn 50, * (10 + civilian bonus) / 10; transports while fewer
   than turn / 40 (and count <= 8: up to 9), weight * (turn / 40 - count)
   Numbered: BUG #145 (BUGS_AND_QUIRKS.txt) */
int AI_Wizard_Choose_Production(int player, int city)
{
    int option_total;
    int unit_count;
    int option_count;
    int pick;
    int units_only;
    int want_settler;
    unsigned city_lm;
    int units_at;
    int ut;
    int want_engineer;
    int prod;
    int trade_chance;
    int plane;
    int x;
    int y;
    int own_cities;
    int trade_goods;
    unsigned human_home_lm;                     /* the human's home landmass; -1 (no match) after turn 200 */
    int options[50];
    int weights[50];
    register int i;

    trade_goods = 0;
    own_cities = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player) {
            if (p_cities[city].producing == PROD_TRADE_GOODS)
                trade_goods++;
            own_cities++;
        }
    }
    trade_chance = (own_cities / 8 - trade_goods) * 10 + 40;
    if (player == PLAYER_HUMAN)
        trade_chance = 0;
    if (game_turn > 200)
        human_home_lm = -1;
    else
        human_home_lm = p_landmass_ids[p_fortresses->plane][p_fortresses->y][p_fortresses->x];   /* the human's fortress */
    plane = p_cities[city].plane;
    x = p_cities[city].x;
    y = p_cities[city].y;
    City_Build_Options(city, &option_count, &option_total, &unit_count, options);
    city_lm = p_landmass_ids[plane][y][x];
    want_settler = 1;
    want_engineer = 1;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == player && p_units[i].plane == plane &&
            p_landmass_ids[p_units[i].plane][p_units[i].y][p_units[i].x] == city_lm) {
            if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST)
                want_settler = 0;
            if (unit_types[p_units[i].type].construction > 0)
                want_engineer = 0;
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player && i != city) {
            prod = p_cities[i].producing;
            if (prod >= PROD_UNIT_BASE) {
                prod = prod - PROD_UNIT_BASE;
                if (unit_types[prod].ability_flags & AFLAG_CREATE_OUTPOST)
                    want_settler = 0;
                if (unit_types[prod].construction > 0)
                    want_engineer = 0;
            }
        }
    }
    units_at = AI_Count_Units_At(x, y, plane);
    if (city_lm == human_home_lm) {
        if (Random(30) > units_at)
            units_only = 1;
        else
            units_only = 0;
    } else {
        if (Random(15) > units_at)
            units_only = 1;
        else
            units_only = 0;
        if (units_at > 0 && p_cities[city].buildings[BLD_BARRACKS] == BLDSTATE_NONE) {
            p_cities[city].producing = PROD_BARRACKS;
            return 0;
        }
        if (units_at > 1 && p_cities[city].buildings[BLD_BUILDERS_HALL] == BLDSTATE_NONE) {
            p_cities[city].producing = PROD_BUILDERS_HALL;
            return 0;
        }
        if (units_at > 2 && p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_NONE) {
            p_cities[city].producing = PROD_SHRINE;
            return 0;
        }
        if (units_at > 3 && p_cities[city].buildings[BLD_SMITHY] == BLDSTATE_NONE) {
            p_cities[city].producing = PROD_SMITHY;
            return 0;
        }
        if (units_at > 4 && p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_NONE) {
            p_cities[city].producing = PROD_GRANARY;
            return 0;
        }
    }
    if (Random(100) <= trade_chance) {
        p_cities[city].producing = PROD_TRADE_GOODS;
        return 0;
    }
    for (i = 0; i < option_count; i++) {
        if (options[i] >= PROD_UNIT_BASE) {
            weights[i] = unit_types[options[i] - PROD_UNIT_BASE].cost / 10;
            weights[i] *= weights[i];
        } else
            weights[i] = ai_bldg_category_weight[p_building_data[options[i]].category] +
                         ai_bldg_objective_bonus[p_building_data[options[i]].category][wizards[player].objective];
    }
    if (p_cities[i].population < 5)
        for (i = 0; i < option_count; i++)
            if (options[i] == PROD_HOUSING)
                weights[i] *= 2;
    for (i = 0; i < option_count; i++) {
        if (options[i] >= PROD_UNIT_BASE) {
            ut = options[i] - PROD_UNIT_BASE;
            if (unit_types[ut].ability_flags & AFLAG_CREATE_OUTPOST) {
                if (want_settler == 1 && p_cities[city].population > 2) {
                    if (game_turn < 50)
                        weights[i] = weights[i] * 3 / 2;
                    weights[i] = ((ai_civilian_objective_bonus[wizards[player].objective] + 10) * weights[i]) / 10;
                } else
                    weights[i] = 0;
            } else if (unit_types[ut].construction > 0) {
                if (want_engineer == 1) {
                    if (game_turn < 50)
                        weights[i] = weights[i] * 3 / 2;
                    weights[i] = ((ai_civilian_objective_bonus[wizards[player].objective] + 10) * weights[i]) / 10;
                } else
                    weights[i] = 0;
            } else if (unit_types[ut].transport_capacity > 0) {
                if (game_turn / 40 >= ai_own_transport_count && ai_own_transport_count <= 8)
                    weights[i] = weights[i] * (game_turn / 40 - ai_own_transport_count);
                else
                    weights[i] = 0;
            } else if (units_only == 0)
                weights[i] = 0;
            else
                weights[i] = ((ai_unit_objective_bonus[wizards[player].objective] + 10) * weights[i]) / 10;
        } else if (units_only == 1)
            weights[i] = 0;
    }
    pick = Random_Weighted_Pick(weights, option_count);
    p_cities[city].producing = options[pick];
}

/* (city) a neutral city: Barracks first; units only while the garrison is under Random(15) + 5, never settlers,
   engineers or ships; no Ship Wrights' / Maritime Guild */
void AI_Neutral_Choose_Production(int city)
{
    int option_total;
    int unit_count;
    int option_count;
    int pick;
    int want_units;
    unsigned city_lm;
    int options[50];
    int weights[50];
    register int i;

    City_Build_Options(city, &option_count, &option_total, &unit_count, options);
    city_lm = p_landmass_ids[p_cities[city].plane][p_cities[city].y][p_cities[city].x];   /* never read */
    if (AI_Count_Units_At(p_cities[city].x, p_cities[city].y, p_cities[city].plane) < Random(15) + 5)
        want_units = 1;
    else
        want_units = 0;
    for (i = 0; i < option_count; i++) {
        if (options[i] >= PROD_UNIT_BASE) {
            weights[i] = unit_types[options[i] - PROD_UNIT_BASE].cost / 10;
            weights[i] *= weights[i];
        } else
            weights[i] = ai_bldg_category_weight[p_building_data[options[i]].category];
    }
    for (i = 0; i < option_count; i++) {
        if (options[i] >= PROD_UNIT_BASE) {
            if ((unit_types[options[i] - PROD_UNIT_BASE].ability_flags & AFLAG_CREATE_OUTPOST) || unit_types[options[i] - PROD_UNIT_BASE].transport_capacity != 0 ||
                unit_types[options[i] - PROD_UNIT_BASE].construction != 0)
                weights[i] = 0;
            else if (want_units == 0)
                weights[i] = 0;
        } else if (want_units == 1)
            weights[i] = 0;
    }
    for (i = 0; i < option_count; i++) {
        if (options[i] == PROD_SHIP_WRIGHTS_GUILD)
            weights[i] = 0;
        if (options[i] == PROD_MARITIME_GUILD)
            weights[i] = 0;
    }
    if (p_cities[city].buildings[BLD_BARRACKS] == BLDSTATE_NONE)
        p_cities[city].producing = PROD_BARRACKS;
    else {
        pick = Random_Weighted_Pick(weights, option_count);
        p_cities[city].producing = options[pick];
    }
}

/* (player, city) buys the current item when its buy price times a random factor (building 2..5, settler 1..4, other
   unit 4..8) is below the player's gold */
void AI_City_Maybe_Buy(int player, int city)
{
    int cost;

    if (p_cities[city].producing >= PROD_UNIT_BASE) {
        if (unit_types[p_cities[city].producing - PROD_UNIT_BASE].ability_flags & AFLAG_CREATE_OUTPOST)
            cost = Random(4) * City_Buy_Price(city);
        else
            cost = (Random(5) + 3) * City_Buy_Price(city);
    } else
        cost = (Random(4) + 1) * City_Buy_Price(city);
    if (wizards[player].gold > cost)
        City_Buy(player, city);
}

/* (city) the Grand Vizier's choice for a human city, stored in producing (-> 0 when a fixed rule set it, or an
   empty city; no return value on the weighted path): the
   AI_Wizard_Choose_Production weighting as player 0 with its own constants - the garrison checklist always applies,
   no Trade Goods roll; units_only = Random(15) > units here, never before turn 20 once 2+ are here; transports
   while fewer than turn / 30 (and count <= 10: up to 11), counted here among the units on the landmass; BUG #4: the Housing x2 test reads
   city [option count]
   Numbered: BUG #4 (BUGS_AND_QUIRKS.txt) */
int Vizier_Choose_Production(int city)
{
    int option_total;
    int unit_count;
    int option_count;
    int pick;
    int units_only;
    int want_settler;
    unsigned city_lm;
    int units_at;
    int ut;
    int want_engineer;
    int prod;
    int plane;
    int x;
    int y;
    int player;
    int options[50];
    int weights[50];
    register int i;

    if (p_cities[city].population == 0)
        return 0;
    player = PLAYER_HUMAN;
    plane = p_cities[city].plane;
    x = p_cities[city].x;
    y = p_cities[city].y;
    City_Build_Options(city, &option_count, &option_total, &unit_count, options);
    city_lm = p_landmass_ids[plane][y][x];
    want_settler = 1;
    want_engineer = 1;
    ai_own_transport_count = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == player && p_units[i].plane == plane &&
            p_landmass_ids[p_units[i].plane][p_units[i].y][p_units[i].x] == city_lm) {
            if (unit_types[p_units[i].type].ability_flags & AFLAG_CREATE_OUTPOST)
                want_settler = 0;
            if (unit_types[p_units[i].type].construction > 0)
                want_engineer = 0;
            if (unit_types[p_units[i].type].transport_capacity > 0)
                ai_own_transport_count++;
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player && i != city) {
            prod = p_cities[i].producing;
            if (prod >= PROD_UNIT_BASE) {
                prod = prod - PROD_UNIT_BASE;
                if (unit_types[prod].ability_flags & AFLAG_CREATE_OUTPOST)
                    want_settler = 0;
                if (unit_types[prod].construction > 0)
                    want_engineer = 0;
            }
        }
    }
    units_at = AI_Count_Units_At(x, y, plane);
    if (Random(15) > units_at)
        units_only = 1;
    else
        units_only = 0;
    if (game_turn < 20 && units_at > 1)
        units_only = 0;
    if (units_at > 0 && p_cities[city].buildings[BLD_BARRACKS] == BLDSTATE_NONE) {
        p_cities[city].producing = PROD_BARRACKS;
        return 0;
    }
    if (units_at > 1 && p_cities[city].buildings[BLD_BUILDERS_HALL] == BLDSTATE_NONE) {
        p_cities[city].producing = PROD_BUILDERS_HALL;
        return 0;
    }
    if (units_at > 2 && p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_NONE) {
        p_cities[city].producing = PROD_SHRINE;
        return 0;
    }
    if (units_at > 3 && p_cities[city].buildings[BLD_SMITHY] == BLDSTATE_NONE) {
        p_cities[city].producing = PROD_SMITHY;
        return 0;
    }
    if (units_at > 4 && p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_NONE) {
        p_cities[city].producing = PROD_GRANARY;
        return 0;
    }
    for (i = 0; i < option_count; i++) {
        if (options[i] >= PROD_UNIT_BASE) {
            weights[i] = unit_types[options[i] - PROD_UNIT_BASE].cost / 10;
            weights[i] *= weights[i];
        } else
            weights[i] = ai_bldg_category_weight[p_building_data[options[i]].category] +
                         ai_bldg_objective_bonus[p_building_data[options[i]].category][wizards[player].objective];
    }
    if (p_cities[i].population < 5)
        for (i = 0; i < option_count; i++)
            if (options[i] == PROD_HOUSING)
                weights[i] *= 2;
    for (i = 0; i < option_count; i++) {
        if (options[i] >= PROD_UNIT_BASE) {
            ut = options[i] - PROD_UNIT_BASE;
            if (unit_types[ut].ability_flags & AFLAG_CREATE_OUTPOST) {
                if (want_settler == 1 && p_cities[city].population > 2) {
                    if (game_turn < 50)
                        weights[i] = weights[i] * 3 / 2;
                    weights[i] = ((ai_civilian_objective_bonus[wizards[player].objective] + 10) * weights[i]) / 10;
                } else
                    weights[i] = 0;
            } else if (unit_types[ut].construction > 0) {
                if (want_engineer == 1) {
                    if (game_turn < 50)
                        weights[i] = weights[i] * 3 / 2;
                    weights[i] = ((ai_civilian_objective_bonus[wizards[player].objective] + 10) * weights[i]) / 10;
                } else
                    weights[i] = 0;
            } else if (unit_types[ut].transport_capacity > 0) {
                if (game_turn / 30 >= ai_own_transport_count && ai_own_transport_count <= 10)
                    weights[i] = weights[i] * (game_turn / 30 - ai_own_transport_count);
                else
                    weights[i] = 0;
            } else if (units_only == 0)
                weights[i] = 0;
            else
                weights[i] = ((ai_unit_objective_bonus[wizards[player].objective] + 10) * weights[i]) / 10;
        } else if (units_only == 1)
            weights[i] = 0;
    }
    pick = Random_Weighted_Pick(weights, option_count);
    p_cities[city].producing = options[pick];
}
