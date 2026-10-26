/* CITYCALC.C - WIZARDS.EXE overlay code segment c751 (stub 25f8): city economics - roads on the movement maps, the
   city area, food / production / gold / research / power, growth and maximum population, production cost, unrest,
   the city site value, the area ownership bitmaps, the per-wizard power statistics, outpost growth, mercenary offers,
   and the breakdown lists of the city screen.
   Data DS:6e9e..6eab (the unrest by tax rate).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS c751 src/wizards/CITYCALC.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int City_Area_Tiles(int x, int y, int plane, int *xs, int *ys);
void Road_Build(int x, int y, int plane);
void Road_Enchant(int x, int y, int plane);
void Tile_Hook_Nop(int x, int y, int plane);
void Human_Cities_Area_Nop(void);
int City_Population_Tens(int city);
int City_Terrain_Food_Cap(int city);
int City_Wild_Game_Food(int city);
int City_Calc_Food(int city);
int City_Max_Population(int city);
int City_Calc_Production(int city);
int City_Calc_Gold(int city);
int City_Calc_Research(int city);
int City_Calc_Power(int city);
int City_Growth_Rate(int city);
int City_Production_Cost(int city);
int Fortress_Find_Unit_Tile(int player, int unit_type, int *wx, int *wy, int *wp);
int Mercenary_Offer_Roll(int player, int *x, int *y, int *plane, int *amount, int *type_out, int *cost_out,
                         int *level_out);
void Never_Set_Flag_Tiles_Collect(void);
int City_Min_Farmers(int city);
void Outposts_Grow(void);
void Wizard_Update_Power_Stats(void);
int Wizard_Stat_Magic_Power(int player);
int Wizard_Stat_Army_Strength(int player);
int Wizard_Stat_Spell_Research(int player);
int Wizard_Stat_Population(int player);
int City_Rebels(int city);
void City_Site_Evaluate(int x, int y, int plane, int *max_pop, int *prod_pct, int *gold_pct, int *unit_cost_pct,
                        int *mineral_gold, int *mineral_power, int *nightshade, int *mithril, int *adamantium,
                        int *coastal, int *unexplored);
void City_Area_Bitmaps_Rebuild(void);
int Tile_Is_Shared(int x, int y, int plane);
void Road_Remove(int x, int y, int plane);
int City_Food_Breakdown(int city, int *out);
int City_Gold_Breakdown(int city, int *out);
int City_Production_Breakdown(int city, int *out);
int City_Research_Breakdown(int city, int *out);
int City_Power_Breakdown(int city, int *out);

#include "GAME.H"

int tax_unrest_pct[7] = { 0, 10, 20, 30, 45, 60, 75 };  /* DS:6e9e  unrest % by tax rate */

extern struct RaceInfo race_table[];            /* DS:1d86 */
extern struct DifficultyInfo difficulty_table[];  /* DS:1fea */
extern char never_set_flag_tile_plane[5];               /* DS:941c */
extern char never_set_flag_tile_y[5];                   /* DS:9421 */
extern char never_set_flag_tile_x[5];                   /* DS:9426 */
extern int never_set_flag_tile_count;                   /* DS:942c */
extern struct Events far *p_events;             /* DS:9998 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern char far *p_race_unrest_rows[];         /* DS:9ce0 */
extern unsigned char far *city_area_bits;       /* DS:9c9c */
extern unsigned char far *city_area_shared_bits;   /* DS:9ca0 */
extern unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH];  /* DS:9cac */
extern unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb0 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb8 */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */

void EMS_Map_Default(void);
int Terrain_Food(int x, int y, int plane);
int Bit_Test(int bit, char far *map);
void Bit_Set(int bit, char far *map);
int Terrain_Production_Pct(int x, int y, int plane, int gaia);
int City_Trade_Pct(int x, int y, int plane);
int City_Road_Trade_Pct(int city);
int Tile_Mineral_Gold(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Silver_Gold(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_GoldOre_Gold(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Gem_Gold(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Mineral_Power(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Mithril_Power(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Adamantium_Power(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Quork_Power(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Crysx_Power(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Mineral_Unit_Cost_Pct(int x, int y, int plane, int miners_guild, int dwarf);
int Tile_Is_Water(int x, int y, int plane);
int Tile_Is_Mithril(int x, int y, int plane);
int Tile_Is_Adamantium(int x, int y, int plane);
int Random(int n);
int Wizard_Effective_Fame(int player);
int AI_Accept_Mercenary_Offer(int player, int cost);
int Unit_Gold_Upkeep(int unit);
int Unit_Mana_Upkeep(int unit);

/* (x, y, plane) a road on the square: movement cost 1 for the land move types 0..4, the road flag set and the enchanted
   road flag cleared; on Myrror the road is enchanted at once */
void Road_Build(int x, int y, int plane)
{
    EMS_Map_Default();
    p_movement_cost_maps[plane][MOVE_COST_CAVALRY][y][x] = 1;
    p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] = 1;
    p_movement_cost_maps[plane][MOVE_COST_FORESTER][y][x] = 1;
    p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER][y][x] = 1;
    p_movement_cost_maps[plane][MOVE_COST_SWIMMING][y][x] = 1;
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_ROAD;
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_ENCHANTED_ROAD;
    if (plane == PLANE_MYRROR)
        Road_Enchant(x, y, plane);
}

/* (x, y, plane) a road on the square becomes enchanted: flag 0x10, movement cost 0 for the land move types */
void Road_Enchant(int x, int y, int plane)
{
    EMS_Map_Default();
    if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_ROAD) {
        p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_ENCHANTED_ROAD;
        p_movement_cost_maps[plane][MOVE_COST_CAVALRY][y][x] = 0;
        p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] = 0;
        p_movement_cost_maps[plane][MOVE_COST_FORESTER][y][x] = 0;
        p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER][y][x] = 0;
        p_movement_cost_maps[plane][MOVE_COST_SWIMMING][y][x] = 0;
    }
}

/* (x, y, plane) empty */
void Tile_Hook_Nop(int x, int y, int plane)
{
}

/* calls the empty Tile_Hook_Nop for the 5 x 5 area of every human city (both radius cases are 2) */
void Human_Cities_Area_Nop(void)
{
    int cx;
    int cy;
    int plane;
    int tx;
    int ty;
    int radius;
    int city;
    int wx;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == PLAYER_HUMAN) {
            cx = p_cities[city].x;
            cy = p_cities[city].y;
            plane = p_cities[city].plane;
            if (p_cities[city].population >= 8)
                radius = 2;
            else
                radius = 2;
            for (ty = cy - radius; ty < cy + radius + 1; ty++) {
                if (ty >= 0 && ty < MAP_HEIGHT) {
                    for (tx = cx - radius; tx < cx + radius + 1; tx++) {
                        wx = tx;
                        if (wx < 0)
                            wx += MAP_WIDTH;
                        if (wx >= MAP_WIDTH)
                            wx -= MAP_WIDTH;
                        Tile_Hook_Nop(wx, ty, plane);
                    }
                }
            }
        }
    }
}

/* (city) the growth points toward the next citizen (0 .. 99, shown x 10 people) */
int City_Population_Tens(int city)
{
    return p_cities[city].population_tens;
}

/* (city) the food the area's terrain supports: the sum of Terrain_Food (x 1.5 with Gaia's Blessing) / 4 */
int City_Terrain_Food_Cap(int city)
{
    int area_tiles;
    int plane;
    int xs[25];
    int ys[25];
    int i;
    int food;

    plane = p_cities[city].plane;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    food = 0;
    for (i = 0; i < area_tiles; i++)
        food += Terrain_Food(xs[i], ys[i], plane);
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING] > 0)
        food = food * 3 / 2;
    return food / 4;
}

/* (city) wild game: +2 per area square with mineral flag 0x40, +1 when another city shares it */
int City_Wild_Game_Food(int city)
{
    int area_tiles;
    int plane;
    int cell;
    int xs[25];
    int ys[25];
    int food;
    int i;

    plane = p_cities[city].plane;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    food = 0;
    for (i = 0; i < area_tiles; i++) {
        if (p_mineral_maps[plane][ys[i]][xs[i]] & MINFLAG_WILD_GAME) {
            cell = ys[i] * MAP_WIDTH + xs[i];
            if (Bit_Test(cell, city_area_shared_bits + plane * MAP_SQUARES))
                food++;
            else
                food += 2;
        }
    }
    return food;
}

/* (x, y, plane, xs, ys) the squares of a city area (5 x 5 without the corners, x wraps, y clipped, squares with
   terrain flag 0x20 left out) into xs / ys; returns their count */
int City_Area_Tiles(int x, int y, int plane, int *xs, int *ys)
{
    int n;
    int dx_min;
    int dx_max;
    int dx;
    int ty;
    unsigned char far *row;
    int wx;
    int dy;

    n = 0;
    for (dy = -2; dy <= 2; dy++) {
        ty = y + dy;
        if (ty >= 0 && ty < MAP_HEIGHT) {
            dx_min = -2;
            dx_max = 2;
            if (dy == -2 || dy == 2) {
                dx_min = -1;
                dx_max = 1;
            }
            row = p_terrain_flags_maps[plane][ty];
            for (dx = dx_min; dx <= dx_max; dx++) {
                wx = x + dx;
                if (wx < 0)
                    wx += MAP_WIDTH;
                if (wx >= MAP_WIDTH)
                    wx -= MAP_WIDTH;
                if ((row[wx] & TERRAIN_FLAG_CORRUPTION) == 0) {
                    xs[n] = wx;
                    ys[n] = ty;
                    n++;
                }
            }
        }
    }
    return n;
}

/* (city) the food production: 0 for an empty city; the farmers (cut to the population, written back) x 2, x 3 for
   Halflings or an Animists' Guild; +2 Foresters' Guild; halved by Famine; above the terrain cap only half the excess
   counts; +2 Granary, +3 Farmers' Market; + wild game
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Calc_Food(int city)
{
    int cap;
    int food;

    if (p_cities[city].population == 0)
        return 0;
    cap = City_Terrain_Food_Cap(city);
    if (p_cities[city].farmers > p_cities[city].population)
        p_cities[city].farmers = p_cities[city].population;
    if (p_cities[city].race == RACE_HALFLING || p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_REPLACED)
        food = p_cities[city].farmers * 3;
    else
        food = p_cities[city].farmers * 2;
    if (p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
        food += 2;
    if (p_cities[city].enchants[CITY_ENCH_FAMINE] > 0)
        food = food / 2;
    if (food > cap)
        food = cap + (food - cap) / 2;
    if (p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
        food += 2;
    if (p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
        food += 3;
    food += City_Wild_Game_Food(city);
    return food;
}

/* (city) the maximum population: the terrain food cap (halved by Famine) +2 Granary, +3 Farmers' Market, + wild game */
int City_Max_Population(int city)
{
    int pop;

    pop = City_Terrain_Food_Cap(city);
    if (p_cities[city].enchants[CITY_ENCH_FAMINE] > 0)
        pop = pop / 2;
    if (p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
        pop += 2;
    if (p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
        pop += 3;
    pop += City_Wild_Game_Food(city);
    return pop;
}

/* (city) the production: (workers x 2 (3 for Dwarves and Klackons) + farmers / 2, rounded up) x the percentage of 100
   + the area's terrain bonuses (Gaia's Blessing variant) + Foresters' Guild / Sawmill 25, Miners' / Mechanicians'
   Guild 50, Inspirations 100; halved (rounded up) by Cursed Lands. The owner is read but unused */
int City_Calc_Production(int city)
{
    int area_tiles;
    int i;
    int workers;
    int prod;
    int owner;
    int plane;
    int gaia;
    int per_worker_x2;
    int xs[25];
    int ys[25];
    int pct;

    if (p_cities[city].population == 0)
        return 0;
    owner = p_cities[city].owner;
    plane = p_cities[city].plane;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    if (p_cities[city].farmers > p_cities[city].population)
        p_cities[city].farmers = p_cities[city].population;
    workers = p_cities[city].population - p_cities[city].farmers - City_Rebels(city);
    if (p_cities[city].race == RACE_DWARVEN || p_cities[city].race == RACE_KLACKON)
        per_worker_x2 = 6;
    else
        per_worker_x2 = 4;
    prod = (workers * per_worker_x2 + p_cities[city].farmers + 1) / 2;
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING] > 0)
        gaia = 1;
    else
        gaia = 0;
    pct = 100;
    for (i = 0; i < area_tiles; i++)
        pct += Terrain_Production_Pct(xs[i], ys[i], plane, gaia);
    if (p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
        pct += 25;
    if (p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_REPLACED)
        pct += 25;
    if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
        pct += 50;
    if (p_cities[city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_REPLACED)
        pct += 50;
    if (p_cities[city].enchants[CITY_ENCH_INSPIRATIONS] > 0)
        pct += 100;
    prod = (long)prod * pct / 100;
    if (p_cities[city].enchants[CITY_ENCH_CURSED_LANDS] > 0)
        prod = (prod + 1) / 2;
    return prod;
}

/* (city) the gold income: the taxed citizens (population - rebels, x tax rate / 2; a neutral city 1 gold each, no tax rate;
   doubled for Dwarves) + the
   area's minerals, x (100 + trade %) / 100 - the trade % from the square and roads (+50 Nomads), at most 3 x the
   population, + Merchants' Guild 100, Bank / Marketplace 50, Prosperity 100; + half the production for Trade Goods;
   at most 255
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Calc_Gold(int city)
{
    int gold;
    int area_tiles;
    int k;
    int player;
    int plane;
    int miners_guild;
    int dwarf;
    int trade_pct;
    int xs[25];
    int ys[25];

    if (p_cities[city].population == 0)
        return 0;
    player = p_cities[city].owner;
    plane = p_cities[city].plane;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    trade_pct = City_Road_Trade_Pct(city) + City_Trade_Pct(p_cities[city].x, p_cities[city].y, p_cities[city].plane);
    if (p_cities[city].race == RACE_NOMAD)
        trade_pct += 50;
    if (p_cities[city].population * 3 < trade_pct)
        trade_pct = p_cities[city].population * 3;
    if (p_cities[city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_REPLACED)
        trade_pct += 100;
    if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
        miners_guild = 1;
    else
        miners_guild = 0;
    if (p_cities[city].buildings[BLD_BANK] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_BANK] == BLDSTATE_REPLACED)
        trade_pct += 50;
    if (p_cities[city].buildings[BLD_MARKETPLACE] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MARKETPLACE] == BLDSTATE_REPLACED)
        trade_pct += 50;
    if (p_cities[city].enchants[CITY_ENCH_PROSPERITY] > 0)
        trade_pct += 100;
    if (player == PLAYER_NEUTRAL)
        gold = p_cities[city].population - City_Rebels(city);
    else
        gold = (p_cities[city].population - City_Rebels(city)) * wizards[player].tax_rate / 2;
    if (p_cities[city].race == RACE_DWARVEN) {
        gold += gold;
        dwarf = 1;
    } else
        dwarf = 0;
    for (k = 0; k < area_tiles; k++)
        gold += Tile_Mineral_Gold(xs[k], ys[k], plane, miners_guild, dwarf);
    gold = (long)gold * (trade_pct + 100L) / 100;
    if (p_cities[city].producing == PROD_TRADE_GOODS)
        gold += p_cities[city].production_income / 2;
    if (gold > 255)
        gold = 255;
    return gold;
}

/* (city) research: Library 2, Sages' Guild 3, University 5, Wizards' Guild 8; 0 for empty and neutral cities */
int City_Calc_Research(int city)
{
    int owner;
    int research;

    owner = p_cities[city].owner;
    if (p_cities[city].population == 0 || owner == PLAYER_NEUTRAL)
        return 0;
    research = 0;
    if (p_cities[city].buildings[BLD_LIBRARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_LIBRARY] == BLDSTATE_REPLACED)
        research += 2;
    if (p_cities[city].buildings[BLD_SAGES_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_SAGES_GUILD] == BLDSTATE_REPLACED)
        research += 3;
    if (p_cities[city].buildings[BLD_UNIVERSITY] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_UNIVERSITY] == BLDSTATE_REPLACED)
        research += 5;
    if (p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED)
        research += 8;
    return research;
}

/* (city) the power (mana) income: 0 during the mana short event and for empty or neutral cities; the religious
   buildings 1 + 2 + 3 + 4 (x 1.5 for Divine / Infernal Power, x 2 with Dark Rituals), Alchemists' Guild +3, Wizards'
   Guild -3, the race (Dark Elves = population, High Elves / Beastmen / Draconians = population / 2), the fortress
   city the spellbooks (+5 on Myrror), the minerals; the moons halve or add the religious power by realm
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Calc_Power(int city)
{
    int i;
    int player;
    int religion_pct;
    int religion;
    int area_tiles;
    int plane;
    int dwarf;
    int miners_guild;
    int books;
    int xs[25];
    int ys[25];
    int power;

    player = p_cities[city].owner;
    if (p_events->mana_short == EVENT_STATE_RUNNING)
        return 0;
    if (p_cities[city].population == 0 || player == PLAYER_NEUTRAL)
        return 0;
    religion = 0;
    religion_pct = 100;
    if (wizards[player].retorts[RETORT_DIVINE_POWER] || wizards[player].retorts[RETORT_INFERNAL_POWER])
        religion_pct = 150;
    if (p_cities[city].enchants[CITY_ENCH_DARK_RITUALS])
        religion_pct *= 2;
    if (p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_REPLACED)
        religion++;
    if (p_cities[city].buildings[BLD_TEMPLE] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_TEMPLE] == BLDSTATE_REPLACED)
        religion += 2;
    if (p_cities[city].buildings[BLD_PARTHENON] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_PARTHENON] == BLDSTATE_REPLACED)
        religion += 3;
    if (p_cities[city].buildings[BLD_CATHEDRAL] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_CATHEDRAL] == BLDSTATE_REPLACED)
        religion += 4;
    religion = religion * religion_pct / 100;
    power = religion;
    if (p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_REPLACED)
        power += 3;
    if (p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED)
        power -= 3;
    plane = p_cities[city].plane;
    if (p_cities[city].race == RACE_DARK_ELF)
        power += p_cities[city].population;
    if (p_cities[city].race == RACE_HIGH_ELF || p_cities[city].race == RACE_BEASTMEN ||
        p_cities[city].race == RACE_DRACONIAN)
        power += p_cities[city].population / 2;
    if (p_fortresses[player].x == p_cities[city].x && p_fortresses[player].y == p_cities[city].y &&
        p_fortresses[player].plane == plane) {
        books = wizards[player].spellbooks[REALM_NATURE] + wizards[player].spellbooks[REALM_SORCERY] +
                wizards[player].spellbooks[REALM_CHAOS] + wizards[player].spellbooks[REALM_LIFE] +
                wizards[player].spellbooks[REALM_DEATH];
        if (plane == PLANE_ARCANUS)
            power += books;
        else
            power += books + 5;
    }
    if (p_cities[city].race == RACE_DWARVEN)
        dwarf = 1;
    else
        dwarf = 0;
    if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
        miners_guild = 1;
    else
        miners_guild = 0;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    for (i = 0; i < area_tiles; i++)
        power += Tile_Mineral_Power(xs[i], ys[i], plane, miners_guild, dwarf);
    if (wizards[player].spellbooks[REALM_DEATH] && p_events->good_moon == EVENT_STATE_RUNNING)
        power -= religion / 2;
    if (wizards[player].spellbooks[REALM_LIFE] && p_events->bad_moon == EVENT_STATE_RUNNING)
        power -= religion / 2;
    if (power < 0)
        power = 0;
    if (wizards[player].spellbooks[REALM_LIFE] && p_events->good_moon == EVENT_STATE_RUNNING)
        power += religion;
    if (wizards[player].spellbooks[REALM_DEATH] && p_events->bad_moon == EVENT_STATE_RUNNING)
        power += religion;
    return power;
}

/* (city) the growth per turn in tenths of a citizen: 0 for empty cities and at the maximum (only an EXACT match
   stops it - bug: a city above its maximum keeps growing); -population with a maximum of 0; (food - population) x 5
   when starving; else (max - population + 1) / 2 + the race's growth + Granary 2 + Farmers' Market 3 - but no growth
   at all from MAX_CITY_POPULATION (25) citizens on - doubled by Stream of Life and a population boom in the city (population_boom in
   ANY non-zero state - new, running or disabled - unlike CITYSCR.C's EVENT_STATE_RUNNING test), x (100 + P) / 100 with P = -25 Dark
   Rituals and for Housing +50 (population 1) or the worker percentage + Sawmill 10 + Builders' Hall 15; halved for neutral
   cities (0 at 2 x (difficulty + 1) citizens), x the difficulty's AI growth % for AI cities; at least 0 (the starving branch's
   negative rate is returned as is)
   Numbered: BUG #103 (BUGS_AND_QUIRKS.txt) */
int City_Growth_Rate(int city)
{
    int modifier;
    int max_pop;
    int growth;

    if (p_cities[city].population == 0)
        return 0;
    max_pop = City_Max_Population(city);
    if (p_cities[city].population == max_pop)
        return 0;
    if (max_pop == 0)
        return -p_cities[city].population;
    if (p_cities[city].food_income < p_cities[city].population)
        growth = (p_cities[city].food_income - p_cities[city].population) * 5;
    else {
        growth = (max_pop - p_cities[city].population + 1) / 2 + race_table[p_cities[city].race].growth;
        if (p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
            growth += 2;
        if (p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
            growth += 3;
        if (p_cities[city].population >= MAX_CITY_POPULATION)
            return 0;
        if (p_cities[city].enchants[CITY_ENCH_STREAM_OF_LIFE] > 0)
            growth *= 2;
        if (p_events->population_boom && p_events->boom_city == city)
            growth *= 2;
        modifier = 0;
        if (p_cities[city].enchants[CITY_ENCH_DARK_RITUALS] > 0)
            modifier -= 25;
        if (p_cities[city].producing == PROD_HOUSING) {
            if (p_cities[city].population == 1)
                modifier += 50;
            else
                modifier += (p_cities[city].population - City_Rebels(city) - p_cities[city].farmers) * 100 /
                            p_cities[city].population;
            if (p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_REPLACED)
                modifier += 10;
            if (p_cities[city].buildings[BLD_BUILDERS_HALL] == BLDSTATE_BUILT ||
                p_cities[city].buildings[BLD_BUILDERS_HALL] == BLDSTATE_REPLACED)
                modifier += 15;
            if (p_cities[city].population >= MAX_CITY_POPULATION)
                return 0;
        }
        growth = (long)growth * (modifier + 100) / 100;
        if (p_cities[city].owner == PLAYER_NEUTRAL) {
            growth = growth / 2;
            if (p_cities[city].population >= (difficulty + 1) * 2)
                growth = 0;
        } else if (p_cities[city].owner != PLAYER_HUMAN)
            growth = (long)growth * difficulty_table[difficulty].ai_growth_pct / 100;
        if (growth <= 0)
            growth = 0;
    }
    return growth;
}

/* (city) the full cost of what the city produces: a building's cost, or a unit's cost reduced by the area's
   minerals (not for units with AFLAG_PURIFY, transports, or the Fire Ball / Doom Bolt spell; at most 50% off) */
int City_Production_Cost(int city)
{
    int cost;
    int area_tiles;
    int plane;
    int minerals_apply;
    int dwarven;
    int i;
    int cost_pct;
    int miners;
    int xs[25];
    int ys[25];
    int producing;

    EMS_Map_Default();
    producing = p_cities[city].producing;
    if (p_cities[city].race == RACE_DWARVEN)
        dwarven = 1;
    else
        dwarven = 0;
    if (producing < PROD_UNIT_BASE)
        cost = p_building_data[producing].cost;
    else {
        cost = unit_types[producing - PROD_UNIT_BASE].cost;
        plane = p_cities[city].plane;
        area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
        cost_pct = 100;
        minerals_apply = 1;
        if (unit_types[producing - PROD_UNIT_BASE].ability_flags & AFLAG_PURIFY)
            minerals_apply = 0;
        if (unit_types[producing - PROD_UNIT_BASE].transport_capacity != 0)
            minerals_apply = 0;
        if ((char)unit_types[producing - PROD_UNIT_BASE].abilities2 & ABILITY2_FIRE_BALL_SPELL)   /* the unit spells: low byte */
            minerals_apply = 0;
        if ((char)unit_types[producing - PROD_UNIT_BASE].abilities2 & ABILITY2_DOOMBOLT_SPELL)
            minerals_apply = 0;
        if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
            miners = 1;
        else
            miners = 0;
        if (minerals_apply == 1) {
            for (i = 0; i < area_tiles; i++)
                cost_pct -= Tile_Mineral_Unit_Cost_Pct(xs[i], ys[i], plane, miners, dwarven);
            if (cost_pct < 50)
                cost_pct = 50;
            if (cost_pct != 100)
                cost = (long)cost * cost_pct / 100;
        }
    }
    return cost;
}

/* (player, unit_type, *wx, *wy, *wp) DEAD CODE: where a new unit of the type would go at the player's fortress - the
   fortress city's square while it holds fewer than 9 of the player's units (1), else the last square of the 3 x 3
   with fewer than 9 own and no foreign units that is land (or any square for swimming / sailing / flying types),
   returning 0 */
int Fortress_Find_Unit_Tile(int player, int unit_type, int *wx, int *wy, int *wp)
{
    int i;
    int city;
    int x;
    int y;
    int plane;
    int adx;
    int ady;
    int can_water;
    int grid[3][3];
    register int k;
    register int j;

    if (p_fortresses[player].active == 0)
        return 0;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            grid[i][j] = 0;
    city = -1;
    for (k = 0; k < num_cities; k++)
        if (p_fortresses[player].x == p_cities[k].x && p_fortresses[player].y == p_cities[k].y)
            city = k;
    if (city == -1)
        return 0;
    x = p_cities[city].x;
    y = p_cities[city].y;
    plane = p_cities[city].plane;
    for (k = 0; k < num_units; k++) {
        if (p_units[k].plane == plane && p_units[k].owner != PLAYER_NEUTRAL) {
            adx = p_units[k].x - x;
            ady = p_units[k].y - y;
            if (adx < 0)
                adx = -adx;
            if (ady < 0)
                ady = -ady;
            if (adx <= 1 && ady <= 1) {
                if (p_units[k].owner != player)
                    grid[p_units[k].y - y + 1][p_units[k].x - x + 1] = 1000;
                else
                    grid[p_units[k].y - y + 1][p_units[k].x - x + 1]++;
            }
        }
    }
    if (grid[1][1] < MAX_STACK) {
        *wx = x;
        *wy = y;
        *wp = plane;
        return 1;
    }
    can_water = 0;
    if (unit_types[unit_type].move_flags == MOVE_FLYING || unit_types[unit_type].move_flags == MOVE_SWIMMING || unit_types[unit_type].move_flags == MOVE_SAILING)
        can_water = 1;
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            if (grid[i + 1][j + 1] < MAX_STACK && (Tile_Is_Water(x + j, y + i, plane) == 0 || can_water == 1)) {
                *wx = x + j;
                *wy = y + i;
                *wp = plane;
            }
        }
    }
    return 0;
}

/* (player, x, y, plane, *amount, *type, *cost, *level) a mercenary offer: the level and the count roll Random(100)
   + fame (> 90: 3, > 60: 2, else 1); up to 20 tries (none before turn 30: amount 0, returns 0) of a normal unit type
   (35..197 - Random(198) - 1 -, not 131, no fantastic / settlers / sailing / Draconian engineers) whose race's plane
   - Myrror for Beastmen, Dark Elves, Draconians, Dwarves and Trolls, Arcanus for the rest - holds the player's
   units or cities (a tower counts for both); the limit game_turn / 2 is tested BEFORE the try's cost is computed, so each try is judged by the previous
   try's per-unit cost (cost x (level + 3) / 2, no count) - bug: the first try compares an uninitialised cost; no offer (count 0) if the gold does not exceed the cost (checked before the
   Charismatic halving) or an AI refuses. x / y / plane are unused; no return value at the end
   Numbered: BUG #8 (BUGS_AND_QUIRKS.txt) */
int Mercenary_Offer_Roll(int player, int *x, int *y, int *plane, int *amount, int *type_out, int *cost_out,
                         int *level_out)
{
    int fame;
    int level;
    int roll;
    int cost;
    int count;
    int arcanus;
    int myrror;
    int tries;
    int i;
    int unit_type;

    myrror = 0;
    arcanus = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == player) {
            if (p_units[i].plane == PLANE_ARCANUS)
                arcanus = 1;
            else
                myrror = 1;
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player) {
            if (p_cities[i].plane == PLANE_ARCANUS)
                arcanus = 1;
            else
                myrror = 1;
        }
    }
    for (i = 0; i < NUM_TOWERS; i++) {
        if (p_towers[i].owner == player) {
            arcanus = 1;
            myrror = 1;
        }
    }
    fame = Wizard_Effective_Fame(player);
    roll = Random(100) + fame;
    if (roll > 90)
        level = 3;
    else if (roll > 60)
        level = 2;
    else
        level = 1;
    tries = 0;
retry:
    tries++;
    if (tries > 20 || game_turn < 30) {
        *amount = 0;
        return 0;
    }
    unit_type = Random(NUM_UNIT_TYPES) - 1;
    if (unit_types[unit_type].ability_flags & AFLAG_FANTASTIC)
        goto retry;
    if (unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST)
        goto retry;
    if (unit_types[unit_type].move_flags == MOVE_SAILING)
        goto retry;
    if (unit_type < UNIT_TRIREME)                    /* no heroes */
        goto retry;
    if (unit_type == UNIT_NOMAD_MAGICIANS)
        goto retry;
    if (unit_types[unit_type].race == RACE_DRACONIAN && unit_types[unit_type].construction > 0)
        goto retry;
    if (unit_types[unit_type].race == RACE_BEASTMEN || unit_types[unit_type].race == RACE_DARK_ELF || unit_types[unit_type].race == RACE_DRACONIAN ||
        unit_types[unit_type].race == RACE_DWARVEN || unit_types[unit_type].race == RACE_TROLL) {
        if (myrror == 0)
            goto retry;
    } else {
        if (arcanus == 0)
            goto retry;
    }
    if (game_turn / 2 < cost)
        goto retry;
    cost = unit_types[unit_type].cost * (level + 3) / 2;
    roll = Random(100) + fame;
    if (roll > 90)
        count = 3;
    else if (roll > 60)
        count = 2;
    else
        count = 1;
    cost = cost * count;
    if (wizards[player].gold <= cost)
        count = 0;
    if (wizards[player].retorts[RETORT_CHARISMATIC])
        cost = cost / 2;
    if (player != PLAYER_HUMAN && AI_Accept_Mercenary_Offer(player, cost) == 0)
        count = 0;
    *amount = count;
    *type_out = unit_type;
    *cost_out = cost;
    *level_out = level;
}

/* the first 5 squares (Arcanus then Myrror, row by row) with terrain flag 0x40 into never_set_flag_tile_x / _y / _plane;
   vestigial - nothing reads them */
void Never_Set_Flag_Tiles_Collect(void)
{
    int x;
    int y;
    register int plane;

    never_set_flag_tile_count = 0;
    for (plane = 0; plane < 2 && never_set_flag_tile_count < 5; plane++) {
        for (y = 0; y < MAP_HEIGHT && never_set_flag_tile_count < 5; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if ((p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_NEVER_SET) && never_set_flag_tile_count < 5) {
                    never_set_flag_tile_x[never_set_flag_tile_count] = x;
                    never_set_flag_tile_y[never_set_flag_tile_count] = y;
                    never_set_flag_tile_plane[never_set_flag_tile_count] = plane;
                    never_set_flag_tile_count++;
                }
            }
        }
    }
}

/* (city) the fewest farmers that feed the population (after wild game, Granary 2 and Farmers' Market 3), with
   City_Calc_Food's farmer rules */
int City_Min_Farmers(int city)
{
    int farmers;
    int need;
    int population;
    int cap;
    int per_farmer;
    int food;

    EMS_Map_Default();
    population = p_cities[city].population;
    need = population - City_Wild_Game_Food(city);
    if (p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
        need -= 2;
    if (p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
        need -= 3;
    if (need <= 0)
        return 0;
    cap = City_Terrain_Food_Cap(city);
    if (p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_REPLACED || p_cities[city].race == RACE_HALFLING)
        per_farmer = 3;
    else
        per_farmer = 2;
    farmers = 0;
    food = 0;
    while (food < need && farmers < population) {
        farmers++;
        food = farmers * per_farmer;
        if (p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
            food += 2;
        if (p_cities[city].enchants[CITY_ENCH_FAMINE] > 0)
            food = food / 2;
        if (food > cap)
            food = cap + (food - cap) / 2;
    }
    return farmers;
}

/* the growth of the outposts (population 0): grow % = the terrain food cap + the race's outpost growth + Gaia's
   Blessing 20 + Stream of Life 10 + 5 / 10 per mineral (bug: the mineral is read at the area index of the CITY
   number, not the square's), x the difficulty's outpost growth % (the human player too); Random(100) <= grow % adds
   Random(3) tens; the shrink chance 5 (+5 Evil Presence, +10 each Pestilence / Famine / Chaos Rift) takes Random(2)
   Numbered: BUG #5, #121, #122 (BUGS_AND_QUIRKS.txt) */
void Outposts_Grow(void)
{
    int chance;
    int area_tiles;
    int mineral;
    int plane;
    int k;
    int xs[25];
    int ys[25];
    int city;
    int grow;

    EMS_Map_Default();
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].population == 0) {
            grow = City_Terrain_Food_Cap(city);
            grow += race_table[p_cities[city].race].outpost_growth;
            if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING])
                grow += 20;
            if (p_cities[city].enchants[CITY_ENCH_STREAM_OF_LIFE])
                grow += 10;
            plane = p_cities[city].plane;
            area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
            for (k = 0; k < area_tiles; k++) {
                mineral = p_mineral_maps[plane][ys[city]][xs[city]] & MINERAL_KIND_MASK;
                if (mineral != 0) {
                    if (mineral == MINERAL_IRON_ORE || mineral == MINERAL_SILVER)
                        grow += 5;
                    else
                        grow += 10;
                }
            }
            chance = 5;
            if (p_cities[city].enchants[CITY_ENCH_EVIL_PRESENCE])
                chance += 5;
            if (p_cities[city].enchants[CITY_ENCH_PESTILENCE])
                chance += 10;
            if (p_cities[city].enchants[CITY_ENCH_FAMINE])
                chance += 10;
            if (p_cities[city].enchants[CITY_ENCH_CHAOS_RIFT])
                chance += 10;
            grow = (long)grow * difficulty_table[difficulty].outpost_growth_pct / 100;
            if (Random(100) <= grow)
                p_cities[city].population_tens += Random(3);
            if (Random(100) <= chance)
                p_cities[city].population_tens -= Random(2);
        }
    }
}

/* each turn: the four power-graph statistics of every wizard - magic power, army strength and research scaled so
   the best stays at most 200, population unscaled - and the sum / 7
   (scaled to at most 160) appended to the power history (shifted left after turn 288)
   Numbered: BUG #96 (BUGS_AND_QUIRKS.txt) */
void Wizard_Update_Power_Stats(void)
{
    int max_power;
    int max_army;
    int max_research;
    int max_pop;
    int k;
    int score[6];                               /* each wizard's total of the four stats (then / 7 into the history) */
    int player;
    int max_total;

    for (player = 0; player < num_players; player++) {
        wizards[player].stat_magic_power = Wizard_Stat_Magic_Power(player);
        wizards[player].stat_army_strength = Wizard_Stat_Army_Strength(player);
        wizards[player].stat_spell_research = Wizard_Stat_Spell_Research(player);
        wizards[player].stat_population = Wizard_Stat_Population(player);
        score[player] = wizards[player].stat_magic_power + wizards[player].stat_army_strength +
                        wizards[player].stat_spell_research + wizards[player].stat_population;
    }
    max_power = 0;
    max_army = 0;
    max_research = 0;
    max_pop = 0;
    max_total = 0;
    for (player = 0; player < num_players; player++) {
        if (wizards[player].stat_magic_power > max_power)
            max_power = wizards[player].stat_magic_power;
        if (wizards[player].stat_army_strength > max_army)
            max_army = wizards[player].stat_army_strength;
        if (wizards[player].stat_spell_research > max_research)
            max_research = wizards[player].stat_spell_research;
        if (wizards[player].stat_population > max_pop)
            max_pop = wizards[player].stat_population;
        if (score[player] > max_total)
            max_total = score[player];
    }
    if (max_power > 200)
        for (player = 0; player < num_players; player++)
            wizards[player].stat_magic_power = (long)wizards[player].stat_magic_power * 200 / max_power;
    if (max_army > 200)
        for (player = 0; player < num_players; player++)
            wizards[player].stat_army_strength = (long)wizards[player].stat_army_strength * 200 / max_army;
    if (max_research > 200)
        for (player = 0; player < num_players; player++)
            wizards[player].stat_spell_research = (long)wizards[player].stat_spell_research * 200 / max_research;
    for (player = 0; player < num_players; player++)
        score[player] = (long)score[player] / 7;
    max_total = (long)max_total / 7;
    if (max_total > 160)
        for (player = 0; player < num_players; player++)
            score[player] = (long)score[player] * 160 / max_total;
    if (game_turn < POWER_HISTORY_TURNS) {
        for (player = 0; player < num_players; player++)
            wizards[player].power_history[game_turn] = score[player];
    } else {
        for (player = 0; player < num_players; player++) {
            for (k = 0; k < POWER_HISTORY_TURNS - 1; k++)
                wizards[player].power_history[k] = wizards[player].power_history[k + 1];
            wizards[player].power_history[POWER_HISTORY_TURNS - 1] = score[player];
        }
    }
}

/* (player) the magic power statistic: the power base - 10, at least 0 */
int Wizard_Stat_Magic_Power(int player)
{
    int power;

    power = wizards[player].power_base - 10;
    if (power < 0)
        power = 0;
    return power;
}

/* (player) the army strength statistic: the gold upkeep + 2 x the mana upkeep of the player's units, / 5 */
int Wizard_Stat_Army_Strength(int player)
{
    long sum;
    int unit;

    sum = 0;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].owner == player)
            sum += Unit_Gold_Upkeep(unit) + Unit_Mana_Upkeep(unit) * 2;
    return sum / 5;
}

/* (player) the spell research statistic: the sum of (slot + 1) over the known spells of every realm, / 8 */
int Wizard_Stat_Spell_Research(int player)
{
    int realm;
    int total;
    int slot;

    total = 0;
    for (realm = 0; realm < NUM_REALMS; realm++)
        for (slot = 0; slot < SPELLS_PER_REALM; slot++)
            if (wizards[player].spell_library[realm][slot] == SPELLSTATE_KNOWN)
                total += slot + 1;
    return total / 8;
}

/* (player) the population statistic: the population of the player's cities / 10 */
int Wizard_Stat_Population(int player)
{
    int total;
    int city;

    total = 0;
    for (city = 0; city < num_cities; city++)
        if (p_cities[city].owner == player)
            total += p_cities[city].population;
    return total / 10;
}

/* (city) the rebels: population x (the race table (TERRSTAT.LBX entry 1) x 10 by the owner's home race and the
   city's race + the tax unrest % (+25 Famine)) / 100, less the calming - the religious buildings (x 1.5 with Divine
   / Infernal Power; none under Evil Presence unless the owner has Death books), Oracle 2, Animists' Guild 1 - replaced
   by -1 under another wizard's Great Wasting or -2 under Armageddon - then -1 Dark Rituals, -2 Pestilence, -1 Cursed
   Lands, Just Cause 1, Gaia's Blessing 2; when that is not enough half the normal units in the city; 0 with Stream of Life, at
   most the population; trims the farmers to fit */
int City_Rebels(int city)
{
    int rebels;
    int player;
    int i;
    int calm;
    int x;
    int y;
    int unrest_pct;
    int religion;
    int plane;
    int garrison;

    player = p_cities[city].owner;
    x = p_cities[city].x;
    y = p_cities[city].y;
    plane = p_cities[city].plane;
    unrest_pct = tax_unrest_pct[wizards[player].tax_rate] +
                 p_race_unrest_rows[wizards[player].home_race][p_cities[city].race] * 10;
    if (p_cities[city].enchants[CITY_ENCH_FAMINE])
        unrest_pct += 25;
    rebels = (long)p_cities[city].population * unrest_pct / 100;
    calm = 0;
    if (p_cities[city].enchants[CITY_ENCH_EVIL_PRESENCE] == 0 || wizards[player].spellbooks[REALM_DEATH] != 0) {
        religion = 0;
        if (p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_REPLACED)
            religion++;
        if (p_cities[city].buildings[BLD_TEMPLE] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_TEMPLE] == BLDSTATE_REPLACED)
            religion++;
        if (p_cities[city].buildings[BLD_PARTHENON] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_PARTHENON] == BLDSTATE_REPLACED)
            religion++;
        if (p_cities[city].buildings[BLD_CATHEDRAL] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_CATHEDRAL] == BLDSTATE_REPLACED)
            religion++;
        if (wizards[player].retorts[RETORT_INFERNAL_POWER] || wizards[player].retorts[RETORT_DIVINE_POWER])
            religion += religion / 2;
        calm += religion;
    }
    if (p_cities[city].buildings[BLD_ORACLE] == BLDSTATE_BUILT)
        calm += 2;
    if (p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_BUILT)
        calm++;
    for (i = 0; i < num_players; i++)
        if (i != player && wizards[i].globals.great_wasting)
            calm = -1;
    for (i = 0; i < num_players; i++)
        if (i != player && wizards[i].globals.armageddon)
            calm = -2;
    if (p_cities[city].enchants[CITY_ENCH_DARK_RITUALS])
        calm--;
    if (p_cities[city].enchants[CITY_ENCH_PESTILENCE])
        calm -= 2;
    if (p_cities[city].enchants[CITY_ENCH_CURSED_LANDS])
        calm--;
    if (wizards[player].globals.just_cause)
        calm++;
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING])
        calm += 2;
    if (calm < rebels) {
        garrison = 0;
        for (i = 0; i < num_units; i++)
            if (p_units[i].owner == player && p_units[i].x == x && p_units[i].y == y && p_units[i].plane == plane &&
                p_units[i].type < UNIT_MAGIC_SPIRIT)
                garrison++;
        calm += garrison / 2;
    }
    rebels -= calm;
    if (rebels < 0)
        rebels = 0;
    if (p_cities[city].enchants[CITY_ENCH_STREAM_OF_LIFE])
        rebels = 0;
    if (p_cities[city].population < rebels)
        rebels = p_cities[city].population;
    if (p_cities[city].farmers + rebels > p_cities[city].population)
        p_cities[city].farmers = p_cities[city].population - rebels;
    if (p_cities[city].farmers < 0)
        p_cities[city].farmers = 0;
    return rebels;
}

/* (x, y, plane, *max_pop, *prod_pct, *gold_pct, *unit_cost_pct, *mineral_gold, *mineral_power, *nightshade,
   *mithril, *adamantium, *coastal, *unexplored) the Surveyor's City Resources and the AI's settler site data over
   the 21 squares of a city area (rows 0 and 40+ skipped): an unexplored square (the human player's exploration map)
   sets *unexplored; a square in another city's area counts half (food remainders carried); food in quarters / 4,
   at most 25 (Wild Game +2 quarters - BUG #123: half a food, where a city gets 2 from it (City_Wild_Game_Food), so
   the Surveyor and the AI's site choice undervalue it); the unit cost % at most 50; adamantium clears mithril. On an existing city:
   City_Max_Population, the production buildings and Inspirations, the trade % (Nomads, roads, at most 3 x the
   population) and the gold buildings and Prosperity. Coastal = explored water in the 3 x 3
   Numbered: BUG #26, #124 (BUGS_AND_QUIRKS.txt) */
void City_Site_Evaluate(int x, int y, int plane, int *max_pop, int *prod_pct, int *gold_pct, int *unit_cost_pct,
                        int *mineral_gold, int *mineral_power, int *nightshade, int *mithril, int *adamantium,
                        int *coastal, int *unexplored)
{
    int tx;
    int bit;
    int half;
    int food;
    int found;
    int i;
    int city;
    int wx;
    int ty;

    EMS_Map_Default();
    found = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane) {
            found = 1;
            city = i;
        }
    }
    *max_pop = 0;
    *prod_pct = 0;
    *gold_pct = 0;
    *unit_cost_pct = 0;
    *mineral_gold = 0;
    *mineral_power = 0;
    *nightshade = 0;
    *mithril = 0;
    *adamantium = 0;
    *coastal = 0;
    *unexplored = 0;
    half = 0;
    *gold_pct = City_Trade_Pct(x, y, plane);
    for (ty = y - 2; ty < y + 3; ty++) {
        if (ty > 0 && ty < MAP_HEIGHT) {
            for (tx = x - 2; tx < x + 3; tx++) {
                if ((y - 2 == ty && (x - 2 == tx || x + 2 == tx)) || (y + 2 == ty && (x - 2 == tx || x + 2 == tx)))
                    continue;
                wx = tx;
                if (wx < 0)
                    wx += MAP_WIDTH;
                if (wx >= MAP_WIDTH)
                    wx -= MAP_WIDTH;
                if (p_exploration_maps[plane][ty][wx] != 0) {
                    bit = ty * MAP_WIDTH + wx;
                    if (found == 0 && Bit_Test(bit, city_area_bits + plane * MAP_SQUARES) != 0) {
                        food = Terrain_Food(wx, ty, plane);
                        if (p_mineral_maps[plane][ty][wx] & MINFLAG_WILD_GAME)
                            food += 2;
                        *max_pop += (food + half) / 2;
                        half = (food + half) % 2;
                        *prod_pct += Terrain_Production_Pct(wx, ty, plane, 0) / 2;
                        *unit_cost_pct += Tile_Mineral_Unit_Cost_Pct(wx, ty, plane, 0, 0) / 2;
                        if (*unit_cost_pct > 50)
                            *unit_cost_pct = 50;
                        *mineral_gold += Tile_Mineral_Gold(wx, ty, plane, 0, 0) / 2;
                        *mineral_power += Tile_Mineral_Power(wx, ty, plane, 0, 0) / 2;
                    } else {
                        *max_pop += Terrain_Food(wx, ty, plane);
                        if (p_mineral_maps[plane][ty][wx] & MINFLAG_WILD_GAME)
                            *max_pop += 2;
                        *prod_pct += Terrain_Production_Pct(wx, ty, plane, 0);
                        *unit_cost_pct += Tile_Mineral_Unit_Cost_Pct(wx, ty, plane, 0, 0);
                        if (*unit_cost_pct > 50)
                            *unit_cost_pct = 50;
                        *mineral_gold += Tile_Mineral_Gold(wx, ty, plane, 0, 0);
                        *mineral_power += Tile_Mineral_Power(wx, ty, plane, 0, 0);
                    }
                    if (p_mineral_maps[plane][ty][wx] & MINFLAG_NIGHTSHADE)
                        *nightshade = 1;
                    if (Tile_Is_Mithril(wx, ty, plane) == 1)
                        *mithril = 1;
                    if (Tile_Is_Adamantium(wx, ty, plane) == 1) {
                        *adamantium = 1;
                        *mithril = 0;
                    }
                } else
                    *unexplored = 1;
            }
        }
    }
    *max_pop = *max_pop / 4;
    if (found == 1) {
        *max_pop = City_Max_Population(city);
        if (p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
            *prod_pct += 25;
        if (p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_REPLACED)
            *prod_pct += 25;
        if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
            *prod_pct += 50;
        if (p_cities[city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_REPLACED)
            *prod_pct += 50;
        if (p_cities[city].enchants[CITY_ENCH_INSPIRATIONS] > 0)
            *prod_pct += 100;
        if (p_cities[city].race == RACE_NOMAD)
            *gold_pct += 50;
        *gold_pct += City_Road_Trade_Pct(city);
        if (p_cities[city].population * 3 < *gold_pct)
            *gold_pct = p_cities[city].population * 3;
        if (p_cities[city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_REPLACED)
            *gold_pct += 100;
        if (p_cities[city].buildings[BLD_BANK] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_BANK] == BLDSTATE_REPLACED)
            *gold_pct += 50;
        if (p_cities[city].buildings[BLD_MARKETPLACE] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_MARKETPLACE] == BLDSTATE_REPLACED)
            *gold_pct += 50;
        if (p_cities[city].enchants[CITY_ENCH_PROSPERITY] > 0)
            *gold_pct += 100;
    }
    if (*max_pop > MAX_CITY_POPULATION)
        *max_pop = MAX_CITY_POPULATION;
    for (ty = y - 1; ty < y + 2; ty++) {
        if (ty > 0 && ty < MAP_HEIGHT) {
            for (tx = x - 1; tx < x + 2; tx++) {
                wx = tx;
                if (wx < 0)
                    wx += MAP_WIDTH;
                if (wx >= MAP_WIDTH)
                    wx -= MAP_WIDTH;
                if (p_exploration_maps[plane][ty][wx] != 0 && Tile_Is_Water(wx, ty, plane) != 0)
                    *coastal = 1;
            }
        }
    }
}

/* rebuilds the city area bitmaps: city_area_bits marks every square in some city's 21-square area,
   city_area_shared_bits the squares in two or more areas (bit y x 60 + x, one map per plane at 2400 bytes apart).
   BUG #27: x is clipped to 0 .. 59 here while City_Area_Tiles wraps it, so area squares across the map's seam are never
   marked or shared */
void City_Area_Bitmaps_Rebuild(void)
{
    int plane;
    int dy;
    int city_plane;
    int city_x;
    int city_y;
    int tx;
    int ty;
    int bit;
    int i;
    int dx;

    EMS_Map_Default();
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (i = 0; i < MAP_SQUARES / 8; i++) {
            *(city_area_bits + plane * MAP_SQUARES + i) = 0;
            *(city_area_shared_bits + plane * MAP_SQUARES + i) = 0;
        }
    }
    for (i = 0; i < num_cities; i++) {
        city_x = p_cities[i].x;
        city_y = p_cities[i].y;
        city_plane = p_cities[i].plane;
        for (dy = -2; dy < 3; dy++) {
            ty = city_y + dy;
            if (ty >= 0 && ty < MAP_HEIGHT) {
                for (dx = -2; dx < 3; dx++) {
                    tx = city_x + dx;
                    if (tx >= 0 && tx < MAP_WIDTH && (dy != -2 || (dx != -2 && dx != 2)) &&
                        (dy != 2 || (dx != -2 && dx != 2))) {
                        bit = ty * MAP_WIDTH + tx;
                        if (Bit_Test(bit, city_area_bits + city_plane * MAP_SQUARES))
                            Bit_Set(bit, city_area_shared_bits + city_plane * MAP_SQUARES);
                        Bit_Set(bit, city_area_bits + city_plane * MAP_SQUARES);
                    }
                }
            }
        }
    }
}

/* (x, y, plane) 1 if the square lies in two or more city areas */
int Tile_Is_Shared(int x, int y, int plane)
{
    int bit;

    bit = y * MAP_WIDTH + x;
    if (Bit_Test(bit, city_area_shared_bits + plane * MAP_SQUARES))
        return 1;
    return 0;
}

/* (x, y, plane) removes the road: movement cost 2 for the land move types 0..4, the road and enchanted road flags
   cleared
   Numbered: BUG #125 (BUGS_AND_QUIRKS.txt) */
void Road_Remove(int x, int y, int plane)
{
    EMS_Map_Default();
    p_movement_cost_maps[plane][MOVE_COST_CAVALRY][y][x] = 2;
    p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] = 2;
    p_movement_cost_maps[plane][MOVE_COST_FORESTER][y][x] = 2;
    p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER][y][x] = 2;
    p_movement_cost_maps[plane][MOVE_COST_SWIMMING][y][x] = 2;
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_ROAD;
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_ENCHANTED_ROAD;
}

/* (city, out[15]) the Food scroll lines: 0 the farmers (2 each, 3 for Halflings; half over the terrain cap, the cap
   without Gaia's Blessing), 1 the Animists' Guild extra, 2 Granary 2, 3 Farmers' Market 3, 4 Foresters' Guild (2, or
   1 over the cap), 5 Wild Game, 6 Gaia's Blessing, 7 the Famine loss - not quite City_Calc_Food's sums (the Foresters'
   2 enter Gaia's line; without Halflings or the guild Gaia's line restarts from 2 per farmer). Returns 0 for an
   empty city, else no value
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Food_Breakdown(int city, int *out)
{
    int food;
    int k;
    int cap;
    int total;

    EMS_Map_Default();
    for (k = 0; k < 15; k++)
        out[k] = 0;
    if (p_cities[city].population == 0)
        return 0;
    cap = City_Terrain_Food_Cap(city);
    if (p_cities[city].farmers > p_cities[city].population)
        p_cities[city].farmers = p_cities[city].population;
    if (p_cities[city].race == RACE_HALFLING || p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_REPLACED)
        food = p_cities[city].farmers * 3;
    else
        food = p_cities[city].farmers * 2;
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING])
        cap = cap * 2 / 3;
    if (food > cap)
        food = cap + (food - cap) / 2;
    total = food;
    if (p_cities[city].race == RACE_HALFLING)
        food = p_cities[city].farmers * 3;
    else
        food = p_cities[city].farmers * 2;
    if (food > cap)
        food = cap + (food - cap) / 2;
    out[FOODLINE_FARMERS] = food;
    out[FOODLINE_ANIMISTS] = total - food;
    if (p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED) {
        if (food > cap)
            out[FOODLINE_FORESTERS_GUILD] = 1;
        else
            out[FOODLINE_FORESTERS_GUILD] = 2;
        food += 2;
    }
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING]) {
        cap = City_Terrain_Food_Cap(city);
        if (p_cities[city].race == RACE_HALFLING || p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_BUILT ||
            p_cities[city].buildings[BLD_ANIMISTS_GUILD] == BLDSTATE_REPLACED)
            food += p_cities[city].farmers * 3;
        else
            food = p_cities[city].farmers * 2;
        if (food > cap)
            food = cap + (food - cap) / 2;
        out[FOODLINE_GAIAS_BLESSING] = food - out[FOODLINE_FARMERS] - out[FOODLINE_ANIMISTS];
    }
    if (p_cities[city].enchants[CITY_ENCH_FAMINE] > 0)
        out[FOODLINE_FAMINE] = food / 2;
    if (p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_GRANARY] == BLDSTATE_REPLACED)
        out[FOODLINE_GRANARY] = 2;
    if (p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FARMERS_MARKET] == BLDSTATE_REPLACED)
        out[FOODLINE_FARMERS_MARKET] = 3;
    out[FOODLINE_WILD_GAME] = City_Wild_Game_Food(city);
}

/* (city, out[15]) the Gold scroll lines: 0 the taxes (doubled for Dwarves after the base is taken), 1 Trade Goods
   (production / 2), 2..4 the silver / gold / gem mines, 5 the Miners' Guild half, 6 the trade % (square, roads,
   Nomads; at most 3 x the population) of the base, 7 Marketplace and 8 Bank 50 % of the base, 9 Merchants' Guild (=
   the taxes line), 10 Prosperity - not City_Calc_Gold's sums. Returns 0 for an empty city, else no value
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Gold_Breakdown(int city, int *out)
{
    int area_tiles;
    int gold;
    int player;
    int plane;
    int miners_guild;
    int dwarf;
    int trade_pct;
    int sum;
    int xs[25];
    int ys[25];
    int k;

    EMS_Map_Default();
    for (k = 0; k < 15; k++)
        out[k] = 0;
    if (p_cities[city].population == 0)
        return 0;
    player = p_cities[city].owner;
    plane = p_cities[city].plane;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    if (player == PLAYER_NEUTRAL)
        gold = p_cities[city].population - City_Rebels(city);
    else
        gold = (p_cities[city].population - City_Rebels(city)) * wizards[player].tax_rate / 2;
    out[GOLDLINE_TAXES] = gold;
    if (p_cities[city].race == RACE_DWARVEN)
        dwarf = 1;
    else
        dwarf = 0;
    sum = 0;
    for (k = 0; k < area_tiles; k++)
        sum += Tile_Silver_Gold(xs[k], ys[k], plane, 0, dwarf);
    out[GOLDLINE_SILVER] = sum;
    sum = 0;
    for (k = 0; k < area_tiles; k++)
        sum += Tile_GoldOre_Gold(xs[k], ys[k], plane, 0, dwarf);
    out[GOLDLINE_GOLD] = sum;
    sum = 0;
    for (k = 0; k < area_tiles; k++)
        sum += Tile_Gem_Gold(xs[k], ys[k], plane, 0, dwarf);
    out[GOLDLINE_GEMS] = sum;
    if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
        miners_guild = 1;
    else
        miners_guild = 0;
    if (miners_guild == 1)
        out[GOLDLINE_MINERS_GUILD] = (out[GOLDLINE_SILVER] + out[GOLDLINE_GOLD] + out[GOLDLINE_GEMS]) / 2;
    for (k = 0; k < area_tiles; k++) {
        gold += Tile_Silver_Gold(xs[k], ys[k], plane, miners_guild, dwarf);
        gold += Tile_GoldOre_Gold(xs[k], ys[k], plane, miners_guild, dwarf);
        gold += Tile_Gem_Gold(xs[k], ys[k], plane, miners_guild, dwarf);
    }
    if (p_cities[city].race == RACE_DWARVEN)
        out[GOLDLINE_TAXES] += out[GOLDLINE_TAXES];
    trade_pct = City_Road_Trade_Pct(city) + City_Trade_Pct(p_cities[city].x, p_cities[city].y, p_cities[city].plane);
    if (p_cities[city].race == RACE_NOMAD)
        trade_pct += 50;
    if (p_cities[city].population * 3 < trade_pct)
        trade_pct = p_cities[city].population * 3;
    out[GOLDLINE_TRADE] = (long)gold * trade_pct / 100;
    trade_pct = 0;
    if (p_cities[city].buildings[BLD_MARKETPLACE] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MARKETPLACE] == BLDSTATE_REPLACED)
        out[GOLDLINE_MARKETPLACE] = (long)gold * 50 / 100;
    if (p_cities[city].buildings[BLD_BANK] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_BANK] == BLDSTATE_REPLACED)
        out[GOLDLINE_BANK] = (long)gold * 50 / 100;
    if (p_cities[city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MERCHANTS_GUILD] == BLDSTATE_REPLACED)
        out[GOLDLINE_MERCHANTS_GUILD] = out[GOLDLINE_TAXES];
    if (p_cities[city].enchants[CITY_ENCH_PROSPERITY] > 0) {
        out[GOLDLINE_PROSPERITY] = out[GOLDLINE_TAXES] + out[GOLDLINE_SILVER] + out[GOLDLINE_GOLD] + out[GOLDLINE_GEMS] + out[GOLDLINE_MINERS_GUILD];
        if (p_cities[city].race == RACE_DWARVEN)
            out[GOLDLINE_PROSPERITY] += out[GOLDLINE_TAXES];
    }
    if (p_cities[city].producing == PROD_TRADE_GOODS) {
        gold += p_cities[city].production_income / 2;
        out[GOLDLINE_TRADE_GOODS] = p_cities[city].production_income / 2;
    }
}

/* (city, out[15]) the Production scroll lines: 0 the workers (2 each, 3 for Dwarves / Klackons), 1 the farmers'
   half, 2 the terrain % of the base, 3 Sawmill and 4 Foresters' Guild 25 %, 5 Miners' Guild and 6 Mechanicians'
   Guild 50 %, 7 Gaia's Blessing's terrain extra, 8 Inspirations (= lines 0 + 1), 9 Cursed Lands (half the sum, a
   loss); each line rounded down on its own. Returns the base x (terrain % + Inspirations 100) / 100, 0 for an empty
   city
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Production_Breakdown(int city, int *out)
{
    int area_tiles;
    int terrain_pct;
    int i;
    int workers;
    int base;
    int owner;
    int plane;
    int gaia;
    int per_worker_x2;
    int sum;
    int xs[25];
    int ys[25];

    EMS_Map_Default();
    for (i = 0; i < 15; i++)
        out[i] = 0;
    if (p_cities[city].population == 0)
        return 0;
    owner = p_cities[city].owner;
    plane = p_cities[city].plane;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    if (p_cities[city].farmers > p_cities[city].population)
        p_cities[city].farmers = p_cities[city].population;
    workers = p_cities[city].population - p_cities[city].farmers - City_Rebels(city);
    if (p_cities[city].race == RACE_DWARVEN || p_cities[city].race == RACE_KLACKON)
        per_worker_x2 = 6;
    else
        per_worker_x2 = 4;
    base = (workers * per_worker_x2 + p_cities[city].farmers + 1) / 2;
    out[PRODLINE_WORKERS] = workers * per_worker_x2 / 2;
    out[PRODLINE_FARMERS] = base - out[PRODLINE_WORKERS];
    gaia = 0;
    terrain_pct = 0;
    for (i = 0; i < area_tiles; i++)
        terrain_pct += Terrain_Production_Pct(xs[i], ys[i], plane, gaia);
    out[PRODLINE_TERRAIN] = (long)base * terrain_pct / 100;
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING] > 0)
        gaia = 1;
    else
        gaia = 0;
    sum = 0;
    for (i = 0; i < area_tiles; i++)
        sum += Terrain_Production_Pct(xs[i], ys[i], plane, gaia);
    out[PRODLINE_GAIAS_BLESSING] = (long)base * sum / 100 - out[PRODLINE_TERRAIN];
    if (p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_FORESTERS_GUILD] == BLDSTATE_REPLACED)
        out[PRODLINE_FORESTERS_GUILD] = (long)base * 25 / 100;
    if (p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SAWMILL] == BLDSTATE_REPLACED)
        out[PRODLINE_SAWMILL] = (long)base * 25 / 100;
    if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
        out[PRODLINE_MINERS_GUILD] = (long)base * 50 / 100;
    if (p_cities[city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MECHANICIANS_GUILD] == BLDSTATE_REPLACED)
        out[PRODLINE_MECHANICIANS_GUILD] = (long)base * 50 / 100;
    if (p_cities[city].enchants[CITY_ENCH_INSPIRATIONS] > 0) {
        terrain_pct += 100;
        out[PRODLINE_INSPIRATIONS] = out[PRODLINE_WORKERS] + out[PRODLINE_FARMERS];
    }
    base = (long)base * terrain_pct / 100;
    if (p_cities[city].enchants[CITY_ENCH_CURSED_LANDS] > 0)
        out[PRODLINE_CURSED_LANDS] = (out[PRODLINE_WORKERS] + out[PRODLINE_FARMERS] + out[PRODLINE_TERRAIN] + out[PRODLINE_SAWMILL] + out[PRODLINE_FORESTERS_GUILD] + out[PRODLINE_MINERS_GUILD] + out[PRODLINE_MECHANICIANS_GUILD] + out[PRODLINE_GAIAS_BLESSING] + out[PRODLINE_INSPIRATIONS]) / 2;
    return base;
}

/* (city, out[15]) the Research scroll lines: 0 Library 2, 1 Sages' Guild 3, 2 University 5, 3 Wizards' Guild 8 (as
   City_Calc_Research). Returns 0 for an empty or neutral city, else no value */
int City_Research_Breakdown(int city, int *out)
{
    int k;
    int owner;

    EMS_Map_Default();
    for (k = 0; k < 15; k++)
        out[k] = 0;
    owner = p_cities[city].owner;
    if (p_cities[city].population == 0 || owner == PLAYER_NEUTRAL)
        return 0;
    if (p_cities[city].buildings[BLD_LIBRARY] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_LIBRARY] == BLDSTATE_REPLACED)
        out[RESLINE_LIBRARY] = 2;
    if (p_cities[city].buildings[BLD_SAGES_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_SAGES_GUILD] == BLDSTATE_REPLACED)
        out[RESLINE_SAGES_GUILD] = 3;
    if (p_cities[city].buildings[BLD_UNIVERSITY] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_UNIVERSITY] == BLDSTATE_REPLACED)
        out[RESLINE_UNIVERSITY] = 5;
    if (p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED)
        out[RESLINE_WIZARDS_GUILD] = 8;
}

/* (city, out[15]) the Power scroll lines: 0 the fortress spellbooks (+5 on Myrror), 1 the race (Dark Elves the
   population, High Elves / Beastmen / Draconians half), 2..5 Shrine 1 (never scaled) / Temple / Parthenon /
   Cathedral (x 1.5 for Divine / Infernal Power), 6 Alchemists' Guild 3, 7 Wizards' Guild 3 (stored positive; the scroll prints it with a "-", as a cost),
   8..11 mithril / adamantium / quork / crysx, 12 the Miners' Guild extra, 13 Dark Rituals, 14 Evil Presence (only for
   owners with Death books); the moons halve (rounding up) or double lines 2..5 and 13. Returns 0 during the mana
   short event or for an empty or neutral city, else no value
   Numbered: BUG #43 (BUGS_AND_QUIRKS.txt) */
int City_Power_Breakdown(int city, int *out)
{
    int i;
    int player;
    int power;
    int religion_pct;
    int religion;
    int area_tiles;
    int plane;
    int dwarven;
    int miners_guild;
    int mineral_power;
    int books;
    int xs[25];
    int ys[25];

    EMS_Map_Default();
    for (i = 0; i < 15; i++)
        out[i] = 0;
    player = p_cities[city].owner;
    if (p_events->mana_short == EVENT_STATE_RUNNING)
        return 0;
    if (p_cities[city].population == 0 || player == PLAYER_NEUTRAL)
        return 0;
    religion = 0;
    religion_pct = 100;
    if (wizards[player].retorts[RETORT_DIVINE_POWER] || wizards[player].retorts[RETORT_INFERNAL_POWER])
        religion_pct = 150;
    if (p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_SHRINE] == BLDSTATE_REPLACED) {
        out[POWLINE_SHRINE] = 1;
        religion++;
    }
    if (p_cities[city].buildings[BLD_TEMPLE] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_TEMPLE] == BLDSTATE_REPLACED) {
        out[POWLINE_TEMPLE] = religion_pct * 2 / 100;
        religion += 2;
    }
    if (p_cities[city].buildings[BLD_PARTHENON] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_PARTHENON] == BLDSTATE_REPLACED) {
        out[POWLINE_PARTHENON] = religion_pct * 3 / 100;
        religion += 3;
    }
    if (p_cities[city].buildings[BLD_CATHEDRAL] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_CATHEDRAL] == BLDSTATE_REPLACED) {
        out[POWLINE_CATHEDRAL] = religion_pct * 4 / 100;
        religion += 4;
    }
    religion = religion * religion_pct / 100;
    if (p_cities[city].enchants[CITY_ENCH_DARK_RITUALS]) {
        out[POWLINE_DARK_RITUALS] = religion;
        religion *= 2;
    }
    if ((wizards[player].spellbooks[REALM_DEATH] && p_events->good_moon == EVENT_STATE_RUNNING) ||
        (wizards[player].spellbooks[REALM_LIFE] && p_events->bad_moon == EVENT_STATE_RUNNING)) {
        out[POWLINE_SHRINE] = (out[POWLINE_SHRINE] + 1) / 2;
        out[POWLINE_TEMPLE] = (out[POWLINE_TEMPLE] + 1) / 2;
        out[POWLINE_PARTHENON] = (out[POWLINE_PARTHENON] + 1) / 2;
        out[POWLINE_CATHEDRAL] = (out[POWLINE_CATHEDRAL] + 1) / 2;
        out[POWLINE_DARK_RITUALS] = (out[POWLINE_DARK_RITUALS] + 1) / 2;
    }
    if ((wizards[player].spellbooks[REALM_LIFE] && p_events->good_moon == EVENT_STATE_RUNNING) ||
        (wizards[player].spellbooks[REALM_DEATH] && p_events->bad_moon == EVENT_STATE_RUNNING)) {
        out[POWLINE_SHRINE] += out[POWLINE_SHRINE];
        out[POWLINE_TEMPLE] += out[POWLINE_TEMPLE];
        out[POWLINE_PARTHENON] += out[POWLINE_PARTHENON];
        out[POWLINE_CATHEDRAL] += out[POWLINE_CATHEDRAL];
        out[POWLINE_DARK_RITUALS] += out[POWLINE_DARK_RITUALS];
    }
    if (p_cities[city].enchants[CITY_ENCH_EVIL_PRESENCE] && wizards[player].spellbooks[REALM_DEATH])
        out[POWLINE_EVIL_PRESENCE] = out[POWLINE_SHRINE] + out[POWLINE_TEMPLE] + out[POWLINE_PARTHENON] + out[POWLINE_CATHEDRAL] + out[POWLINE_DARK_RITUALS];
    if (p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_REPLACED)
        out[POWLINE_ALCHEMISTS_GUILD] = 3;
    if (p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_WIZARDS_GUILD] == BLDSTATE_REPLACED)
        out[POWLINE_WIZARDS_GUILD] = 3;
    plane = p_cities[city].plane;
    power = 0;
    if (p_cities[city].race == RACE_DARK_ELF)
        power += p_cities[city].population;
    if (p_cities[city].race == RACE_HIGH_ELF || p_cities[city].race == RACE_BEASTMEN ||
        p_cities[city].race == RACE_DRACONIAN)
        power += p_cities[city].population / 2;
    out[POWLINE_RACE] = power;
    if (p_fortresses[player].x == p_cities[city].x && p_fortresses[player].y == p_cities[city].y &&
        p_fortresses[player].plane == plane) {
        books = wizards[player].spellbooks[REALM_NATURE] + wizards[player].spellbooks[REALM_SORCERY] +
                wizards[player].spellbooks[REALM_CHAOS] + wizards[player].spellbooks[REALM_LIFE] +
                wizards[player].spellbooks[REALM_DEATH];
        out[POWLINE_FORTRESS] = plane == PLANE_ARCANUS ? books : books + 5;
    }
    if (p_cities[city].race == RACE_DWARVEN)
        dwarven = 1;
    else
        dwarven = 0;
    area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
    for (i = 0; i < area_tiles; i++) {
        out[POWLINE_MITHRIL] += Tile_Mithril_Power(xs[i], ys[i], plane, 0, dwarven);
        out[POWLINE_ADAMANTIUM] += Tile_Adamantium_Power(xs[i], ys[i], plane, 0, dwarven);
        out[POWLINE_QUORK] += Tile_Quork_Power(xs[i], ys[i], plane, 0, dwarven);
        out[POWLINE_CRYSX] += Tile_Crysx_Power(xs[i], ys[i], plane, 0, dwarven);
    }
    mineral_power = 0;
    for (i = 0; i < area_tiles; i++)
        mineral_power += Tile_Mineral_Power(xs[i], ys[i], plane, 0, dwarven);
    if (p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_BUILT ||
        p_cities[city].buildings[BLD_MINERS_GUILD] == BLDSTATE_REPLACED)
        miners_guild = 1;
    else
        miners_guild = 0;
    power = 0;
    for (i = 0; i < area_tiles; i++)
        power += Tile_Mineral_Power(xs[i], ys[i], plane, miners_guild, dwarven);
    out[POWLINE_MINERS_GUILD] = power - mineral_power;
}
