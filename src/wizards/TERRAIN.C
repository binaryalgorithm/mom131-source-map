/* TERRAIN.C - WIZARDS.EXE overlay code segment f319 (stub 2684): the terrain rules of a map square - its type predicates (water, sea,
   ocean, shore, land, river, forest, hills, mountain, desert, swamp, tundra, grassland, volcano, node, corruption),
   its food / production / road-building values, the minerals' gold / power / unit-cost bonuses (Miners' Guild,
   dwarves, halved on a square shared with another city), a city's trade bonus (river / sea, road-connected
   cities), weapon grade (Alchemists' Guild + mithril / adamantium), terrain prerequisite code for buildings and
   the Nightshade count of every city. A terrain value is the map word modulo 762; 0xa2..0xb8 are the single
   squares (a switch over 23 values), the higher ranges the blended shores, rivers, hills, mountains and deserts
   (named TILE_*_FIRST / _LAST in ENUMS.H, TerrainRange and TerrainBand).
   No data or BSS of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS f319 src/wizards/TERRAIN.C */

/* the module's functions in their original declaration order */
int Tile_Is_Water(int x, int y, int plane);
int Tile_Is_Node(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Sea_NoWrap(int x, int y, int plane);
int Tile_Is_Hills(int x, int y, int plane);
int Tile_Is_Swamp(int x, int y, int plane);
int Tile_Is_Desert(int x, int y, int plane);
int Tile_Is_Grassland(int x, int y, int plane);
int Terrain_Food(int x, int y, int plane);
int Tile_Is_Forest_Explored(int x, int y, int plane);
void Tile_Purify(int x, int y, int plane);
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
int Tile_Is_Mithril(int x, int y, int plane);
int Tile_Is_Adamantium(int x, int y, int plane);
int Tile_Mineral_Unit_Cost_Pct(int x, int y, int plane, int miners_guild, int dwarf);
int City_Weapon_Grade(int city);
int Road_Build_Turns(int x, int y, int plane);
int Terrain_Weight_Unused(int x, int y, int plane);
int Tile_Is_Ocean(int x, int y, int plane);
int Tile_Is_Shore_Water(int x, int y, int plane);
int Tile_Terrain_Prereq_Code(int x, int y, int plane);
int Tile_Is_Mountain(int x, int y, int plane);
int Tile_Is_Tundra(int x, int y, int plane);
int Tile_Is_Volcano(int x, int y, int plane);
int Tile_Is_Corrupted(int x, int y, int plane);
int Tile_Has_Never_Set_Flag(int x, int y, int plane);
void Cities_Count_Nightshade(void);
int Tile_Is_Land(int x, int y, int plane);
int Tile_Is_Sea(int x, int y, int plane);
int Tile_Water_Bonus_Unused(int x, int y, int plane);

#include "GAME.H"

extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb8 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */

int Tile_Is_Shared(int x, int y, int plane);
int City_Area_Tiles(int x, int y, int plane, int *xs, int *ys);

/* (x, y, plane) clears the square's corruption flag (0x20) */
void Tile_Purify(int x, int y, int plane)
{
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_CORRUPTION;
}

/* (x, y, plane) -> 1 on a node square (0xa8 sorcery, 0xa9 nature, 0xaa chaos) */
int Tile_Is_Node(int x, int y, int plane)
{
    int unused;                                 /* set to 0, never read */
    int tile;

    unused = 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_SORCERY_NODE || tile == TILE_CHAOS_NODE || tile == TILE_NATURE_NODE)
        return 1;
    return 0;
}

/* (x, y, plane) -> the square's food in quarter units (the callers sum the area and divide by 4): the
   half-unit values (grassland 3, forest / hills 1, nodes 4 / 5, river 4 ...) doubled, halved on a square shared with
   another city; 0 for ocean. Swamps give 0 (QUIRK #272; ReMoM notes the manual's 1/2); MAGIC.EXE's world-generation copy
   (Tile_Food_Value) gives the Nature node 4, not 5; clarified using ReMoM repo */
int Terrain_Food(int x, int y, int plane)
{
    int food;
    int tile;

    food = 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_OCEAN)
        return 0;
    if (tile >= TILE_OCEAN_2)
        return 0;
    else if (tile == TILE_PLACEHOLDER_LAND)
        food = 3;
    else if (tile < TILE_LAND_FIRST)
        food = 1;
    else if (tile > TILE_RIVER3_LAST)
        food = 1;
    else if (tile > TILE_SHORE3_LAST)
        food = 4;
    else if (tile > TILE_DESERT_RANGE_LAST)
        food = 1;
    else if (tile > TILE_HILLS_RANGE_LAST)
        return 0;
    else if (tile > TILE_MOUNTAIN_RANGE_LAST)
        food = 1;
    else if (tile > TILE_RIVER2_LAST)
        return 0;
    else if (tile > TILE_SINGLE_LAST)
        food = 4;
    else {
        switch (tile) {
        case TILE_GRASSLAND:
        case TILE_GRASSLAND_2:
        case TILE_GRASSLAND_3:
        case TILE_GRASSLAND_4:
            food = 3;
            break;
        case TILE_FOREST:
        case TILE_FOREST_2:
        case TILE_FOREST_3:
            food = 1;
            break;
        case TILE_MOUNTAIN:
            food = 0;
            break;
        case TILE_DESERT:
        case TILE_DESERT_2:
        case TILE_DESERT_3:
        case TILE_DESERT_4:
            food = 0;
            break;
        case TILE_SWAMP:
        case TILE_SWAMP_2:
        case TILE_SWAMP_3:
            food = 0;
            break;
        case TILE_TUNDRA:
        case TILE_TUNDRA_2:
        case TILE_TUNDRA_3:
            food = 0;
            break;
        case TILE_SORCERY_NODE:
            food = 4;
            break;
        case TILE_NATURE_NODE:
            food = 5;
            break;
        case TILE_CHAOS_NODE:
            food = 0;
            break;
        case TILE_VOLCANO:
            food = 0;
            break;
        case TILE_HILLS:
            food = 1;
            break;
        }
    }
    food = food + food;
    if (Tile_Is_Shared(x, y, plane) != 0)
        food = food / 2;
    return food;
}

/* (x, y, plane, gaia) -> the square's production bonus %: hills / desert 3 (not the plain TILE_DESERT: 0 - BUG #490,
   the Surveyor shows +3% for every desert), mountains and the chaos node 5, forest and the nature node 3 (6 with
   Gaia's Blessing), halved when shared
   Numbered: BUG #490 (BUGS_AND_QUIRKS.txt) */
int Terrain_Production_Pct(int x, int y, int plane, int gaia)
{
    int tile;
    int pct;

    pct = 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile > TILE_DESERT_RANGE_LAST)
        return 0;
    if (tile > TILE_MOUNTAIN_RANGE_LAST)
        pct = 3;
    else if (tile > TILE_RIVER2_LAST)
        pct = 5;
    else if (tile == TILE_MOUNTAIN || tile == TILE_CHAOS_NODE)
        pct = 5;
    else if (tile == TILE_FOREST || tile == TILE_FOREST_2 || tile == TILE_FOREST_3 || tile == TILE_NATURE_NODE) {
        if (gaia == 1)
            pct = 6;
        else
            pct = 3;
    } else if (tile == TILE_HILLS || tile == TILE_DESERT_2 || tile == TILE_DESERT_3 || tile == TILE_DESERT_4)
        pct = 3;
    else
        return 0;
    if (Tile_Is_Shared(x, y, plane) != 0)
        pct = pct / 2;
    return pct;
}

/* (x, y, plane) -> a city's trade bonus %: 20 on a river, 10 beside water, 30 both */
int City_Trade_Pct(int x, int y, int plane)
{
    int pct;
    int river;
    int water;
    register int dy;
    register int dx;

    river = 0;
    water = 0;
    for (dy = -1; dy < 2; dy++) {
        for (dx = -1; dx < 2; dx++) {
            if (dy == 0 && dx == 0) {
                if (Tile_Is_River(x, y, plane) == 1)
                    river = 1;
            } else if (y + dy >= 0 && y + dy < MAP_HEIGHT && Tile_Is_Water(x + dx, y + dy, plane) != 0)
                water = 1;
        }
    }
    pct = 0;
    if (river == 1) {
        if (water == 1)
            pct = 30;
        else
            pct = 20;
    } else if (water == 1)
        pct = 10;
    return pct;
}

/* (city) -> its road trade bonus: the population of every road-connected city (half for the same race), at most
   3 x its own population */
int City_Road_Trade_Pct(int city)
{
    int byte_i;
    int bit;
    int other_city;
    register int total;

    total = 0;
    for (byte_i = 0; byte_i < 13; byte_i++) {
        if (p_cities[city].road_connections[byte_i] != 0) {
            for (bit = 0; bit < 8; bit++) {
                other_city = byte_i * 8 + bit;
                if (other_city < num_cities && other_city != city) {
                    if (Bit_Test(other_city, p_cities[city].road_connections) != 0) {
                        if (p_cities[city].race == p_cities[other_city].race)
                            total += p_cities[other_city].population / 2;
                        else
                            total += p_cities[other_city].population;
                    }
                }
            }
        }
    }
    if (p_cities[city].population * 3 < total)
        total = p_cities[city].population * 3;
    return total;
}

/* (x, y, plane) -> 1 on a river square (0xb9..0xc4, 0xe9..0x102, 0x1d4..0x1d8); x wraps, y off the map -> 0 */
int Tile_Is_River(int x, int y, int plane)
{
    int tile;

    if (x < 0)
        x += MAP_WIDTH;
    if (x >= MAP_WIDTH)
        x -= MAP_WIDTH;
    if (y < 0)
        return 0;
    if (y >= MAP_HEIGHT)
        return 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile > TILE_RIVER3_LAST)
        return 0;
    if (tile > TILE_SHORE3_LAST)
        return 1;
    if (tile > TILE_RIVER2_LAST)
        return 0;
    if (tile > TILE_SHORE2_LAST)
        return 1;
    if (tile > TILE_RIVER_LAST)
        return 0;
    if (tile > TILE_SINGLE_LAST)
        return 1;
    return 0;
}

/* (x, y, plane, miners_guild, dwarf) -> silver 2 / gold 3 / gems 5 gold, x2 for dwarves, x1.5 with a Miners'
   Guild, halved when shared */
int Tile_Mineral_Gold(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int gold;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    gold = 0;
    if (mineral == MINERAL_SILVER)
        gold = 2;
    if (mineral == MINERAL_GOLD)
        gold = 3;
    if (mineral == MINERAL_GEMS)
        gold = 5;
    if (dwarf != 0)
        gold *= 2;
    if (miners_guild != 0)
        gold = gold * 3 / 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        gold = gold / 2;
    return gold;
}

/* (x, y, plane, miners_guild, dwarf) -> silver's 2 gold (same modifiers) */
int Tile_Silver_Gold(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int gold;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    gold = 0;
    if (mineral == MINERAL_SILVER)
        gold = 2;
    if (dwarf != 0)
        gold *= 2;
    if (miners_guild != 0)
        gold = gold * 3 / 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        gold = gold / 2;
    return gold;
}

/* (x, y, plane, miners_guild, dwarf) -> gold ore's 3 gold (same modifiers) */
int Tile_GoldOre_Gold(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int gold;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    gold = 0;
    if (mineral == MINERAL_GOLD)
        gold = 3;
    if (dwarf != 0)
        gold *= 2;
    if (miners_guild != 0)
        gold = gold * 3 / 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        gold = gold / 2;
    return gold;
}

/* (x, y, plane, miners_guild, dwarf) -> gems' 5 gold (same modifiers) */
int Tile_Gem_Gold(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int gold;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    gold = 0;
    if (mineral == MINERAL_GEMS)
        gold = 5;
    if (dwarf != 0)
        gold *= 2;
    if (miners_guild != 0)
        gold = gold * 3 / 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        gold = gold / 2;
    return gold;
}

/* (x, y, plane, miners_guild, dwarf) -> mithril 1 / adamantium 2 / quork 3 / crysx 5 power, x2 for dwarves, x1.5
   with a Miners' Guild, halved when shared */
int Tile_Mineral_Power(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int power;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    power = 0;
    if (mineral == MINERAL_MITHRIL)
        power = 1;
    if (mineral == MINERAL_ADAMANTIUM)
        power = 2;
    if (mineral == MINERAL_QUORK)
        power = 3;
    if (mineral == MINERAL_CRYSX)
        power = 5;
    if (dwarf != 0)
        power *= 2;
    if (miners_guild != 0)
        power = power * 3 / 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        power = power / 2;
    return power;
}

/* (x, y, plane, miners_guild (unused), dwarf) -> mithril's 1 power, x2 for dwarves, halved when shared */
int Tile_Mithril_Power(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int power;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    power = 0;
    if (mineral == MINERAL_MITHRIL)
        power = 1;
    if (dwarf != 0)
        power *= 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        power = power / 2;
    return power;
}

/* (x, y, plane, miners_guild (unused), dwarf) -> adamantium's 2 power (same modifiers) */
int Tile_Adamantium_Power(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int power;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    power = 0;
    if (mineral == MINERAL_ADAMANTIUM)
        power = 2;
    if (dwarf != 0)
        power *= 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        power = power / 2;
    return power;
}

/* (x, y, plane, miners_guild (unused), dwarf) -> quork crystals' 3 power (same modifiers) */
int Tile_Quork_Power(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int power;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    power = 0;
    if (mineral == MINERAL_QUORK)
        power = 3;
    if (dwarf != 0)
        power *= 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        power = power / 2;
    return power;
}

/* (x, y, plane, miners_guild (unused), dwarf) -> crysx crystals' 5 power (same modifiers) */
int Tile_Crysx_Power(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int power;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    power = 0;
    if (mineral == MINERAL_CRYSX)
        power = 5;
    if (dwarf != 0)
        power *= 2;
    if (Tile_Is_Shared(x, y, plane) != 0)
        power = power / 2;
    return power;
}

/* (x, y, plane) -> 1 on mithril ore */
int Tile_Is_Mithril(int x, int y, int plane)
{
    int mineral;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    if (mineral == MINERAL_MITHRIL)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on adamantium ore */
int Tile_Is_Adamantium(int x, int y, int plane)
{
    int mineral;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    if (mineral == MINERAL_ADAMANTIUM)
        return 1;
    return 0;
}

/* (x, y, plane, miners_guild, dwarf) -> the unit-cost cut % of iron ore 5 (10 with the guild) or coal 10 (20),
   x2 for dwarves, halved when shared. BUG #353: the guild doubles the cut instead of +50% */
int Tile_Mineral_Unit_Cost_Pct(int x, int y, int plane, int miners_guild, int dwarf)
{
    int mineral;
    int pct;

    mineral = p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK;
    pct = 0;
    if (mineral == MINERAL_IRON_ORE) {
        if (miners_guild == 1)
            pct = 10;
        else
            pct = 5;
    }
    if (mineral == MINERAL_COAL) {
        if (miners_guild == 1)
            pct = 20;
        else
            pct = 10;
    }
    if (dwarf == 1)
        pct = pct << 1;
    if (Tile_Is_Shared(x, y, plane) != 0)
        pct = pct / 2;
    return pct;
}

/* (city) -> its units' weapon grade: 1 (magic) with an Alchemists' Guild (built, or in state 0), 2 with mithril in
   the city area, 3 with adamantium */
int City_Weapon_Grade(int city)
{
    int i;
    int mineral;
    int area_tiles;
    int plane;
    int grade;
    int xs[25];
    int ys[25];

    grade = 0;
    if (p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_BUILT || p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] == BLDSTATE_REPLACED) {
        grade = 1;
        plane = p_cities[city].plane;
        area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane, xs, ys);
        for (i = 0; i < area_tiles; i++) {
            mineral = p_mineral_maps[plane][ys[i]][xs[i]] & MINERAL_KIND_MASK;
            if (mineral == MINERAL_ADAMANTIUM)
                grade = 3;
            if (mineral == MINERAL_MITHRIL && grade != 3)
                grade = 2;
        }
    }
    return grade;
}

/* (x, y, plane) -> turns of work to build a road on the square (0 when it already has one, -1 on water): grassland
   3, forest / tundra 6, desert 4, swamp / mountain / volcano 8, hills 6, river 5, nodes 4 / 5 / 8 */
int Road_Build_Turns(int x, int y, int plane)
{
    int tile;

    if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_ROAD)
        return 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 6;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 2;
    if (tile < TILE_LAND_FIRST)
        return -1;
    if (tile > TILE_RIVER3_LAST)
        return -1;
    if (tile > TILE_SHORE3_LAST)
        return 5;
    if (tile > TILE_DESERT_RANGE_LAST)
        return -1;
    if (tile > TILE_HILLS_RANGE_LAST)
        return 4;
    if (tile > TILE_MOUNTAIN_RANGE_LAST)
        return 6;
    if (tile > TILE_RIVER2_LAST)
        return 8;
    if (tile > TILE_SHORE2_LAST)
        return 5;
    if (tile > TILE_RIVER_LAST)
        return -1;
    if (tile > TILE_SINGLE_LAST)
        return 5;
    switch (tile) {
    case TILE_GRASSLAND:
    case TILE_GRASSLAND_2:
    case TILE_GRASSLAND_3:
    case TILE_GRASSLAND_4:
        return 3;
    case TILE_FOREST:
    case TILE_FOREST_2:
    case TILE_FOREST_3:
        return 6;
    case TILE_MOUNTAIN:
        return 8;
    case TILE_DESERT:
    case TILE_DESERT_2:
    case TILE_DESERT_3:
    case TILE_DESERT_4:
        return 4;
    case TILE_SWAMP:
    case TILE_SWAMP_2:
    case TILE_SWAMP_3:
        return 8;
    case TILE_TUNDRA:
    case TILE_TUNDRA_2:
    case TILE_TUNDRA_3:
        return 6;
    case TILE_SORCERY_NODE:
        return 4;
    case TILE_NATURE_NODE:
        return 5;
    case TILE_CHAOS_NODE:
        return 8;
    case TILE_VOLCANO:
        return 8;
    case TILE_HILLS:
        return 6;
    }
    return -1;
}

/* (x, y, plane) -> a per-terrain weight (4 / 2 / 6 / 12 / 0); never called */
int Terrain_Weight_Unused(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 4;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 0;
    if (tile < TILE_LAND_FIRST)
        return 0;
    if (tile > TILE_RIVER3_LAST)
        return 0;
    if (tile > TILE_SHORE3_LAST)
        return 2;
    if (tile > TILE_DESERT_RANGE_LAST)
        return 0;
    if (tile > TILE_HILLS_RANGE_LAST)
        return 2;
    if (tile > TILE_MOUNTAIN_RANGE_LAST)
        return 6;
    if (tile > TILE_RIVER2_LAST)
        return 12;
    if (tile > TILE_SINGLE_LAST)
        return 2;
    switch (tile) {
    case TILE_GRASSLAND:
    case TILE_GRASSLAND_2:
        return 0;
    case TILE_GRASSLAND_3:
    case TILE_GRASSLAND_4:
        return 0;
    case TILE_FOREST:
    case TILE_FOREST_2:
    case TILE_FOREST_3:
        return 4;
    case TILE_MOUNTAIN:
        return 12;
    case TILE_DESERT:
    case TILE_DESERT_2:
    case TILE_DESERT_3:
    case TILE_DESERT_4:
        return 2;
    case TILE_SWAMP:
    case TILE_SWAMP_2:
    case TILE_SWAMP_3:
        return 4;
    case TILE_TUNDRA:
    case TILE_TUNDRA_2:
    case TILE_TUNDRA_3:
        return 4;
    case TILE_SORCERY_NODE:
        return 0;
    case TILE_NATURE_NODE:
        return 4;
    case TILE_CHAOS_NODE:
        return 12;
    case TILE_VOLCANO:
        return 12;
    case TILE_HILLS:
        return 6;
    }
    return 0;
}

/* (x, y, plane) -> Tile_Is_Sea's test without the x wrap and y bounds */
int Tile_Is_Sea_NoWrap(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 0;
    if (tile >= TILE_SHORE4_FIRST)
        return 1;
    if (tile >= TILE_RIVER3_FIRST)
        return 0;
    if (tile >= TILE_SHORE3_FIRST)
        return 1;
    if (tile >= TILE_RIVER2_FIRST)
        return 0;
    if (tile >= TILE_SHORE2_FIRST)
        return 1;
    if (tile >= TILE_LAND_FIRST)
        return 0;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 0;
    if (tile == TILE_LAKE)
        return 0;
    return 1;
}

/* (x, y, plane) -> 1 when the square is explored and forest (0xa3 / 0xb7 / 0xb8) or the nature node (0xa9) */
int Tile_Is_Forest_Explored(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (p_exploration_maps[plane][y][x] == 0)
        return 0;
    if (tile == TILE_FOREST || tile == TILE_FOREST_2 || tile == TILE_FOREST_3 || tile == TILE_NATURE_NODE)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on water (ocean, shore, lake, river mouth ...); x does not wrap */
int Tile_Is_Water(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 0;
    if (tile > TILE_RIVER3_LAST)
        return 1;
    if (tile > TILE_SHORE3_LAST)
        return 0;
    if (tile > TILE_DESERT_RANGE_LAST)
        return 1;
    if (tile > TILE_SHORE2_LAST)
        return 0;
    if (tile > TILE_RIVER_LAST)
        return 1;
    if (tile > TILE_SHORE_LAST)
        return 0;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 0;
    return 1;
}

/* (x, y, plane) -> 1 on open ocean (0, or 601) */
int Tile_Is_Ocean(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_OCEAN || tile == TILE_OCEAN_2)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on 0, 2-0x11, 0x13-0x31, 0x62-0xa1, 0xd9-0xe8, 0x1d9-0x1f7: a narrower water test than
   Tile_Is_Sea. BUG #308: misses 182 shore variants and ocean 601 */
int Tile_Is_Shore_Water(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 0;
    if (tile == TILE_LAKE)
        return 0;
    if (tile < 0x32)
        return 1;
    if (tile < 0x62)
        return 0;
    if (tile < TILE_LAND_FIRST)
        return 1;
    if (tile < 0xd9)
        return 0;
    if (tile < TILE_RIVER2_FIRST)
        return 1;
    if (tile < TILE_SHORE4_FIRST)
        return 0;
    if (tile < 0x1f8)
        return 1;
    return 0;
}

/* (x, y, plane) -> the building terrain-prerequisite code of the square: 101 forest, 104 grassland / river,
   110 water, 200 hills / mountain / volcano, 0 other */
int Tile_Terrain_Prereq_Code(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 0;
    if (tile == TILE_PLACEHOLDER_LAND)
        return BLD_PREREQ_GRASSLAND;
    if (tile < TILE_LAND_FIRST)
        return BLD_PREREQ_SEA;
    if (tile > TILE_RIVER3_LAST)
        return BLD_PREREQ_SEA;
    if (tile > TILE_SHORE3_LAST)
        return BLD_PREREQ_GRASSLAND;
    if (tile > TILE_DESERT_RANGE_LAST)
        return BLD_PREREQ_SEA;
    if (tile > TILE_HILLS_RANGE_LAST)
        return 0;
    if (tile > TILE_RIVER2_LAST)
        return BLD_PREREQ_HILLS;
    if (tile > TILE_SINGLE_LAST)
        return BLD_PREREQ_GRASSLAND;
    switch (tile) {
    case TILE_GRASSLAND:
    case TILE_GRASSLAND_2:
    case TILE_GRASSLAND_3:
    case TILE_GRASSLAND_4:
        return BLD_PREREQ_GRASSLAND;
    case TILE_FOREST:
    case TILE_FOREST_2:
    case TILE_FOREST_3:
        return BLD_PREREQ_FOREST;
    case TILE_MOUNTAIN:
        return BLD_PREREQ_HILLS;
    case TILE_DESERT:
    case TILE_DESERT_2:
    case TILE_DESERT_3:
    case TILE_DESERT_4:
        return 0;
    case TILE_SWAMP:
    case TILE_SWAMP_2:
    case TILE_SWAMP_3:
        return 0;
    case TILE_TUNDRA:
    case TILE_TUNDRA_2:
    case TILE_TUNDRA_3:
        return 0;
    case TILE_SORCERY_NODE:
        return BLD_PREREQ_GRASSLAND;
    case TILE_NATURE_NODE:
        return BLD_PREREQ_FOREST;
    case TILE_CHAOS_NODE:
        return BLD_PREREQ_HILLS;
    case TILE_VOLCANO:
        return BLD_PREREQ_HILLS;
    case TILE_HILLS:
        return BLD_PREREQ_HILLS;
    }
    return 0;
}

/* (x, y, plane) -> 1 on hills (0xab, 0x113..0x123) */
int Tile_Is_Hills(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_HILLS)
        return 1;
    if (tile > TILE_MOUNTAIN_RANGE_LAST && tile < TILE_DESERT_RANGE_FIRST)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on mountains (0xa4, the chaos node 0xaa, the volcano 0xb3, 0x103..0x112) */
int Tile_Is_Mountain(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_MOUNTAIN || tile == TILE_CHAOS_NODE || tile == TILE_VOLCANO)
        return 1;
    if (tile > TILE_RIVER2_LAST && tile < TILE_HILLS_RANGE_FIRST)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on desert (0xa5, 0xae..0xb0, 0x124..0x1c3) */
int Tile_Is_Desert(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile > TILE_HILLS_RANGE_LAST && tile < TILE_SHORE3_FIRST)
        return 1;
    if (tile == TILE_DESERT || tile == TILE_DESERT_2 || tile == TILE_DESERT_3 || tile == TILE_DESERT_4)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on tundra (0xa7, 0xb5, 0xb6, above 602) */
int Tile_Is_Tundra(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_TUNDRA || tile == TILE_TUNDRA_2 || tile == TILE_TUNDRA_3)
        return 1;
    if (tile > TILE_TUNDRA_RANGE_FIRST)            /* BUG #410: > where every other test has >=, so 602 itself is not
                                                   tundra here; MAGIC's Gen_Neighbour_Variants makes 602 for a tundra square whose
                                                   only non-tundra neighbour is the SE one (TERRTYPE.LBX 0 table[16] = 2) */
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on swamp (0xa6, 0xb1, 0xb2) */
int Tile_Is_Swamp(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_SWAMP || tile == TILE_SWAMP_2 || tile == TILE_SWAMP_3)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on grassland (0xa2, 0xac, 0xad, 0xb4) or the sorcery node (0xa8) */
int Tile_Is_Grassland(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_GRASSLAND || tile == TILE_GRASSLAND_2 || tile == TILE_GRASSLAND_3 || tile == TILE_GRASSLAND_4 || tile == TILE_SORCERY_NODE)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on a volcano (0xb3) or the chaos node (0xaa). BUG #259: what changes volcanoes (Gaia's
   Blessing, Change Terrain) also hits chaos nodes */
int Tile_Is_Volcano(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile == TILE_CHAOS_NODE || tile == TILE_VOLCANO)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 when the square is corrupted (flag 0x20) */
int Tile_Is_Corrupted(int x, int y, int plane)
{
    if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_CORRUPTION)
        return 1;
    else
        return 0;
}

/* (x, y, plane) -> 1 when the square has terrain flag 0x40, which nothing sets; never called */
int Tile_Has_Never_Set_Flag(int x, int y, int plane)
{
    if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_NEVER_SET)
        return 1;
    else
        return 0;
}

/* () every populated city with any of buildings 10, 17-19, 21-25 (22 tested twice, any state but -1) gets
   enchants[25] = the Nightshade squares (mineral flag 0x80) of its area. BUG #482: a city that stops qualifying keeps
   its old count
   Numbered: BUG #379 (BUGS_AND_QUIRKS.txt) */
void Cities_Count_Nightshade(void)
{
    int x;                                      /* read, never used */
    int y;                                      /* read, never used */
    int count;
    int unused;                                 /* set to 2, never read */
    int population;
    int plane;                                  /* read, never used */
    int area_tiles;
    int plane_b;
    int xs[25];
    int ys[25];
    register int city;
    register int i;

    for (city = 0; city < num_cities; city++) {
        x = p_cities[city].x;
        y = p_cities[city].y;
        plane = p_cities[city].plane;
        population = p_cities[city].population;
        unused = 2;
        count = 0;
        if (population != 0
            && (p_cities[city].buildings[BLD_SHRINE] != BLDSTATE_NONE || p_cities[city].buildings[BLD_SHRINE] != BLDSTATE_NONE
                || p_cities[city].buildings[BLD_TEMPLE] != BLDSTATE_NONE || p_cities[city].buildings[BLD_PARTHENON] != BLDSTATE_NONE
                || p_cities[city].buildings[BLD_CATHEDRAL] != BLDSTATE_NONE || p_cities[city].buildings[BLD_SAGES_GUILD] != BLDSTATE_NONE
                || p_cities[city].buildings[BLD_ORACLE] != BLDSTATE_NONE || p_cities[city].buildings[BLD_ALCHEMISTS_GUILD] != BLDSTATE_NONE
                || p_cities[city].buildings[BLD_WIZARDS_GUILD] != BLDSTATE_NONE || p_cities[city].buildings[BLD_ANIMISTS_GUILD] != BLDSTATE_NONE)) {
            plane_b = p_cities[city].plane;
            area_tiles = City_Area_Tiles(p_cities[city].x, p_cities[city].y, plane_b, xs, ys);
            for (i = 0; i < area_tiles; i++) {
                if (p_mineral_maps[plane_b][ys[i]][xs[i]] & MINFLAG_NIGHTSHADE)
                    count++;
            }
            p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] = count;
        }
    }
}

/* (x, y, plane) -> 1 on land (also the generator's placeholder 1 and anything above 601), 0 on water */
int Tile_Is_Land(int x, int y, int plane)
{
    int tile;

    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 1;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 1;
    if (tile >= TILE_RIVER2_FIRST && tile <= TILE_DESERT_RANGE_LAST)
        return 1;
    if (tile >= TILE_RIVER3_FIRST && tile <= TILE_RIVER3_LAST)
        return 1;
    if (tile >= TILE_LAND_FIRST && tile <= TILE_RIVER_LAST)
        return 1;
    return 0;
}

/* (x, y, plane) -> 1 on sea (ocean and shore ranges, not 1 or 0x12); x wraps, y off the map -> 0 */
int Tile_Is_Sea(int x, int y, int plane)
{
    int tile;

    if (x < 0)
        x += MAP_WIDTH;
    if (x >= MAP_WIDTH)
        x -= MAP_WIDTH;
    if (y < 0)
        return 0;
    if (y >= MAP_HEIGHT)
        return 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return 0;
    if (tile > TILE_RIVER3_LAST)
        return 1;
    if (tile > TILE_SHORE3_LAST)
        return 0;
    if (tile > TILE_DESERT_RANGE_LAST)
        return 1;
    if (tile > TILE_SHORE2_LAST)
        return 0;
    if (tile > TILE_RIVER_LAST)
        return 1;
    if (tile > TILE_SHORE_LAST)
        return 0;
    if (tile == TILE_PLACEHOLDER_LAND)
        return 0;
    if (tile == TILE_LAKE)
        return 0;
    return 1;
}

/* (x, y, plane) -> a water bonus (10 water, 25 river), halved when shared; never called. BUG #483: the single
   squares 0xa2..0xb8 all reset it to 0 */
int Tile_Water_Bonus_Unused(int x, int y, int plane)
{
    int bonus;
    int tile;

    bonus = 0;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        bonus = 0;
    if (tile == TILE_PLACEHOLDER_LAND)
        bonus = 0;
    if (tile < TILE_LAND_FIRST)
        bonus = 10;
    if (tile > TILE_RIVER3_LAST)
        bonus = 10;
    if (tile > TILE_SHORE3_LAST)
        bonus = 25;
    if (tile > TILE_DESERT_RANGE_LAST)
        bonus = 10;
    if (tile > TILE_HILLS_RANGE_LAST)
        bonus = 0;
    if (tile > TILE_MOUNTAIN_RANGE_LAST)
        bonus = 0;
    if (tile > TILE_RIVER2_LAST)
        bonus = 0;
    if (tile > TILE_SINGLE_LAST)
        bonus = 25;
    switch (tile) {
    case TILE_GRASSLAND:
    case TILE_GRASSLAND_2:
    case TILE_GRASSLAND_3:
    case TILE_GRASSLAND_4:
        bonus = 0;
        break;
    case TILE_FOREST:
    case TILE_FOREST_2:
    case TILE_FOREST_3:
        bonus = 0;
        break;
    case TILE_MOUNTAIN:
        bonus = 0;
        break;
    case TILE_DESERT:
    case TILE_DESERT_2:
    case TILE_DESERT_3:
    case TILE_DESERT_4:
        bonus = 0;
        break;
    case TILE_SWAMP:
    case TILE_SWAMP_2:
    case TILE_SWAMP_3:
        bonus = 0;
        break;
    case TILE_TUNDRA:
    case TILE_TUNDRA_2:
    case TILE_TUNDRA_3:
        bonus = 0;
        break;
    case TILE_SORCERY_NODE:
        bonus = 0;
        break;
    case TILE_NATURE_NODE:
        bonus = 0;
        break;
    case TILE_CHAOS_NODE:
        bonus = 0;
        break;
    case TILE_VOLCANO:
        bonus = 0;
        break;
    case TILE_HILLS:
        bonus = 0;
        break;
    }
    if (Tile_Is_Shared(x, y, plane) != 0)
        bonus = bonus / 2;
    return bonus;
}
