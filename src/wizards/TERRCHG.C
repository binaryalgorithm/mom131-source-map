/* TERRCHG.C - WIZARDS.EXE overlay code segment cbdf (stub 2606): terrain changes - volcanoes (and their owners' count), the
   grassland / forest / desert / mountain / hills conversions with the 3x3 autotiling (desert, tundra, mountain and
   hills blends from TERRTYPE.LBX), the terrain blend classes, the volcano mineral roll (BUG #24: never yields one), a
   new city's default name, the Depletion / New Minerals event sites (BUG #1: their search loops never run), setting
   and clearing a tile's mineral, the cities' road connections and the AI's road paths.
   BUG #484: no conversion here updates p_movement_cost_maps (built only by MAGIC.EXE's world generation, then saved and
   loaded; WIZARDS changes it only for roads and cities), so a square changed by Change Terrain, Raise Volcano, Gaia's
   Blessing or a cooling volcano keeps the old terrain's movement costs for the rest of the game (COMMUNITY-KNOWN:
   ReMoM notes the TERRSTAT patch fixes it). clarified using ReMoM repo
   This module's data (DS:6eac): the grassland / forest / desert tile choices, then its literals; BSS (DS:ca76..ca78).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS cbdf src/wizards/TERRCHG.C */
#include <string.h>

/* the module's functions in their original declaration order */
void Tile_Make_Volcano(int x, int y, int plane, int owner);
void Wizards_Count_Volcanoes(void);
void Tile_Make_Grassland(int x, int y, int plane);
void Tile_Make_Forest(int x, int y, int plane);
void Tile_Make_Desert_Unused(int x, int y, int plane);
void Volcano_To_Mountain(int x, int y, int plane);
void Tile_Make_Hills(int x, int y, int plane);
void Terrain_Retile_3x3(int x, int y, int plane);
int Terrain_Autotile(int x, int y, int plane);
int Terrain_Blend_Class(int x, int y, int plane);
int Volcano_Roll_Mineral(int x, int y, int plane);
void City_Pick_Default_Name(int race, char far *name);
int Event_Find_Depletion_Site(int player, int *x, int *y, int *plane, int *mineral);
int Event_Find_New_Mineral_Site(int player, int *x, int *y, int *plane, int *mineral);
void Tile_Set_Mineral(int x, int y, int plane, int mineral);
void Tile_Clear_Mineral(int x, int y, int plane);
void Cities_Update_Road_Connections(void);
void City_Road_Connections(int x, int y, int plane);
int Road_Path_AI(int x1, int y1, int x2, int y2, int plane, char *xs, char *ys);

#include "GAME.H"

int (*terrtype_table)[256];                     /* DS:ca76  near copy of TERRTYPE.LBX (5 records of 256 words) */

int grassland_tiles[3] = { 0xa2, 0xac, 0xad };  /* DS:6eac  picked by Random(2): 0xad never (BUG #369) */
int forest_tiles[3] = { 0xa3, 0xb7, 0xb8 };     /* DS:6eb2 */
int desert_tiles[3] = { 0xae, 0xaf, 0xaf };     /* DS:6eb8 */

extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];        /* DS:9cb8 */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];        /* DS:9cd0 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];             /* DS:9cdc */

int Random(int n);
void EMS_Map_Default(void);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
extern struct PathWork far *p_path_work;        /* DS:9ca4 */
void Path_Relax(int x, int y, struct PathWork far *work);
void Bit_Set(int bit, char far *map);
void far_strcpy();                              /* no prototype here: called with a far / near destination and a near
                                                   source with segment 0 (4 words each time) */

/* (x, y, plane, owner) the tile becomes a volcano owned by the wizard (owner + 1 or-ed into the flags), the 3x3 is re-tiled and
   the mineral re-rolled */
void Tile_Make_Volcano(int x, int y, int plane, int owner)
{
    p_terrain_maps[plane][y][x] = TILE_VOLCANO;
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | owner + 1;
    Terrain_Retile_3x3(x, y, plane);
    p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] & 0x2f;   /* dead: cleared on the next line */
    p_mineral_maps[plane][y][x] = 0;
    Volcano_Roll_Mineral(x, y, plane);
}

/* () each wizard's volcanoes = the tiles whose flags & 7 name it (owner + 1) */
void Wizards_Count_Volcanoes(void)
{
    int i;
    int plane;
    int x;
    int y;
    int owner;
    int counts[10];

    for (i = 0; i < num_players; i++)
        counts[i] = 0;
    for (plane = 0; plane < NUM_PLANES; plane++)
        for (y = 0; y < MAP_HEIGHT; y++)
            for (x = 0; x < MAP_WIDTH; x++) {
                owner = p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_VOLCANO_OWNER;
                if (owner > 0)
                    counts[owner - 1]++;
            }
    for (i = 0; i < num_players; i++)
        wizards[i].volcanoes = counts[i];
}

/* (x, y, plane) grassland (0xa2 or 0xac), re-tiled, volcano owner cleared */
void Tile_Make_Grassland(int x, int y, int plane)
{
    p_terrain_maps[plane][y][x] = grassland_tiles[Random(2) - 1];
    Terrain_Retile_3x3(x, y, plane);
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_VOLCANO_OWNER;
}

/* (x, y, plane) forest, re-tiled, volcano owner cleared */
void Tile_Make_Forest(int x, int y, int plane)
{
    p_terrain_maps[plane][y][x] = forest_tiles[Random(3) - 1];
    Terrain_Retile_3x3(x, y, plane);
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_VOLCANO_OWNER;
}

/* (x, y, plane) desert, re-tiled, volcano owner cleared (never called) */
void Tile_Make_Desert_Unused(int x, int y, int plane)
{
    p_terrain_maps[plane][y][x] = desert_tiles[Random(3) - 1];
    Terrain_Retile_3x3(x, y, plane);
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_VOLCANO_OWNER;
}

/* (x, y, plane) a volcano becomes a mountain, re-tiled, owner cleared */
void Volcano_To_Mountain(int x, int y, int plane)
{
    p_terrain_maps[plane][y][x] = TILE_MOUNTAIN;
    Terrain_Retile_3x3(x, y, plane);
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_VOLCANO_OWNER;
}

/* (x, y, plane) hills, re-tiled, volcano owner cleared */
void Tile_Make_Hills(int x, int y, int plane)
{
    p_terrain_maps[plane][y][x] = TILE_HILLS;
    Terrain_Retile_3x3(x, y, plane);
    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] & ~TERRAIN_FLAG_VOLCANO_OWNER;
}

/* (x, y, plane) loads TERRTYPE.LBX and autotiles the tile and its 8 neighbours */
void Terrain_Retile_3x3(int x, int y, int plane)
{
    terrtype_table = (int (*)[256])Near_Alloc_Reset(0xa00);
    LBX_Load_Records_Near("TERRTYPE", 0, terrtype_table, 0, 5, 0x200);
    Terrain_Autotile(x, y, plane);
    Terrain_Autotile(x - 1, y - 1, plane);
    Terrain_Autotile(x, y - 1, plane);
    Terrain_Autotile(x + 1, y - 1, plane);
    Terrain_Autotile(x - 1, y, plane);
    Terrain_Autotile(x + 1, y, plane);
    Terrain_Autotile(x - 1, y + 1, plane);
    Terrain_Autotile(x, y + 1, plane);
    Terrain_Autotile(x + 1, y + 1, plane);
}

/* (x, y, plane) the blend tile of a desert / tundra (edges against other classes) or mountain / hills (joins with
   the same class) from its 8 neighbours' classes (read with x wrapped; the tile is written at the unwrapped x:
   BUG #131); rows outside 0..39 are left alone */
int Terrain_Autotile(int x, int y, int plane)
{
    int mask;

    if (y < 0)
        return 0;
    if (y >= MAP_HEIGHT)
        return 0;
    if (Terrain_Blend_Class(x, y, plane) == BLEND_DESERT) {
        mask = 0;
        if (Terrain_Blend_Class(x - 1, y - 1, plane) != BLEND_DESERT)
            mask++;
        if (Terrain_Blend_Class(x, y - 1, plane) != BLEND_DESERT)
            mask += 2;
        if (Terrain_Blend_Class(x + 1, y - 1, plane) != BLEND_DESERT)
            mask += 4;
        if (Terrain_Blend_Class(x + 1, y, plane) != BLEND_DESERT)
            mask += 8;
        if (Terrain_Blend_Class(x + 1, y + 1, plane) != BLEND_DESERT)
            mask += 16;
        if (Terrain_Blend_Class(x, y + 1, plane) != BLEND_DESERT)
            mask += 32;
        if (Terrain_Blend_Class(x - 1, y + 1, plane) != BLEND_DESERT)
            mask += 64;
        if (Terrain_Blend_Class(x - 1, y, plane) != BLEND_DESERT)
            mask += 128;
        if (mask > 0)
            p_terrain_maps[plane][y][x] = terrtype_table[0][mask] + TILE_DESERT_EDGE_BASE;
        else
            p_terrain_maps[plane][y][x] = TILE_DESERT;
    }
    if (Terrain_Blend_Class(x, y, plane) == BLEND_TUNDRA) {
        mask = 0;
        if (Terrain_Blend_Class(x - 1, y - 1, plane) != BLEND_TUNDRA)
            mask++;
        if (Terrain_Blend_Class(x, y - 1, plane) != BLEND_TUNDRA)
            mask += 2;
        if (Terrain_Blend_Class(x + 1, y - 1, plane) != BLEND_TUNDRA)
            mask += 4;
        if (Terrain_Blend_Class(x + 1, y, plane) != BLEND_TUNDRA)
            mask += 8;
        if (Terrain_Blend_Class(x + 1, y + 1, plane) != BLEND_TUNDRA)
            mask += 16;
        if (Terrain_Blend_Class(x, y + 1, plane) != BLEND_TUNDRA)
            mask += 32;
        if (Terrain_Blend_Class(x - 1, y + 1, plane) != BLEND_TUNDRA)
            mask += 64;
        if (Terrain_Blend_Class(x - 1, y, plane) != BLEND_TUNDRA)
            mask += 128;
        if (mask > 0)
            p_terrain_maps[plane][y][x] = terrtype_table[0][mask] + TILE_TUNDRA_EDGE_BASE;
        else
            p_terrain_maps[plane][y][x] = TILE_TUNDRA_2;
    }
    if (Terrain_Blend_Class(x, y, plane) == BLEND_MOUNTAIN) {
        mask = 0;
        if (Terrain_Blend_Class(x - 1, y - 1, plane) == BLEND_MOUNTAIN)
            mask++;
        if (Terrain_Blend_Class(x, y - 1, plane) == BLEND_MOUNTAIN)
            mask += 2;
        if (Terrain_Blend_Class(x + 1, y - 1, plane) == BLEND_MOUNTAIN)
            mask += 4;
        if (Terrain_Blend_Class(x + 1, y, plane) == BLEND_MOUNTAIN)
            mask += 8;
        if (Terrain_Blend_Class(x + 1, y + 1, plane) == BLEND_MOUNTAIN)
            mask += 16;
        if (Terrain_Blend_Class(x, y + 1, plane) == BLEND_MOUNTAIN)
            mask += 32;
        if (Terrain_Blend_Class(x - 1, y + 1, plane) == BLEND_MOUNTAIN)
            mask += 64;
        if (Terrain_Blend_Class(x - 1, y, plane) == BLEND_MOUNTAIN)
            mask += 128;
        if (mask > 0)
            p_terrain_maps[plane][y][x] = terrtype_table[1][mask];
        else
            p_terrain_maps[plane][y][x] = TILE_MOUNTAIN;
    }
    EMS_Map_Default();
    if (Terrain_Blend_Class(x, y, plane) == BLEND_HILLS) {
        mask = 0;
        if (Terrain_Blend_Class(x - 1, y - 1, plane) == BLEND_HILLS)
            mask++;
        if (Terrain_Blend_Class(x, y - 1, plane) == BLEND_HILLS)
            mask += 2;
        if (Terrain_Blend_Class(x + 1, y - 1, plane) == BLEND_HILLS)
            mask += 4;
        if (Terrain_Blend_Class(x + 1, y, plane) == BLEND_HILLS)
            mask += 8;
        if (Terrain_Blend_Class(x + 1, y + 1, plane) == BLEND_HILLS)
            mask += 16;
        if (Terrain_Blend_Class(x, y + 1, plane) == BLEND_HILLS)
            mask += 32;
        if (Terrain_Blend_Class(x - 1, y + 1, plane) == BLEND_HILLS)
            mask += 64;
        if (Terrain_Blend_Class(x - 1, y, plane) == BLEND_HILLS)
            mask += 128;
        if (mask > 0) {
            if (terrtype_table[1][mask] >= TILE_MOUNTAIN_RANGE_FIRST)
                p_terrain_maps[plane][y][x] = terrtype_table[1][mask] + TILE_HILLS_FROM_MOUNTAIN;
            else
                p_terrain_maps[plane][y][x] = TILE_HILLS;
        } else
            p_terrain_maps[plane][y][x] = TILE_HILLS;
    }
}

/* (x, y, plane) -> the blend class of a tile (x wrapped): 0 none, 1 water / shore, 2 land, 3 desert, 4 hills,
   5 mountain, 6 tundra */
int Terrain_Blend_Class(int x, int y, int plane)
{
    int tile;

    if (x >= MAP_WIDTH)
        x -= MAP_WIDTH;
    if (x < 0)
        x += MAP_WIDTH;
    if (y < 0)
        return BLEND_NONE;
    if (y >= MAP_HEIGHT)
        return BLEND_NONE;
    tile = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (tile >= TILE_TUNDRA_RANGE_FIRST)
        return BLEND_TUNDRA;
    if (tile == TILE_PLACEHOLDER_LAND)
        return BLEND_LAND;
    if (tile >= TILE_SHORE4_FIRST)
        return BLEND_WATER;
    if (tile >= TILE_RIVER3_FIRST)
        return BLEND_LAND;
    if (tile >= TILE_SHORE3_FIRST)
        return BLEND_WATER;
    if (tile >= TILE_DESERT_RANGE_FIRST)
        return BLEND_DESERT;
    if (tile >= TILE_HILLS_RANGE_FIRST)
        return BLEND_HILLS;
    if (tile >= TILE_MOUNTAIN_RANGE_FIRST)
        return BLEND_MOUNTAIN;
    if (tile >= TILE_RIVER2_FIRST)
        return BLEND_LAND;
    if (tile >= TILE_SHORE2_FIRST)
        return BLEND_WATER;
    if (tile >= TILE_RIVER_FIRST)
        return BLEND_LAND;
    if (tile < TILE_LAND_FIRST)
        return BLEND_WATER;
    switch (tile) {
    case TILE_GRASSLAND:
    case TILE_GRASSLAND_2:
    case TILE_GRASSLAND_3:
    case TILE_GRASSLAND_4:
        return BLEND_LAND;
    case TILE_FOREST:
    case TILE_FOREST_2:
    case TILE_FOREST_3:
        return BLEND_LAND;
    case TILE_MOUNTAIN:
        return BLEND_MOUNTAIN;
    case TILE_DESERT:
    case TILE_DESERT_2:
    case TILE_DESERT_3:
    case TILE_DESERT_4:
        return BLEND_DESERT;
    case TILE_SWAMP:
    case TILE_SWAMP_2:
    case TILE_SWAMP_3:
        return BLEND_LAND;
    case TILE_TUNDRA:
    case TILE_TUNDRA_2:
    case TILE_TUNDRA_3:
        return BLEND_TUNDRA;
    case TILE_SORCERY_NODE:
        return BLEND_NONE;
    case TILE_NATURE_NODE:
        return BLEND_NONE;
    case TILE_CHAOS_NODE:
        return BLEND_NONE;
    case TILE_VOLCANO:
        return BLEND_NONE;
    case TILE_HILLS:
        return BLEND_HILLS;
    default:
        return BLEND_NONE;
    }
}

/* (x, y, plane) 1 in 5: a mineral by Random(20) (iron 5, coal 3, silver 3, gold 3, gems 2, mithril 2, adamantium
   2) - BUG #24: added to the zeroed byte and masked with 0xe0, so nothing is ever set
   Numbered: BUG #24 (BUGS_AND_QUIRKS.txt) */
int Volcano_Roll_Mineral(int x, int y, int plane)
{
    int mineral;
    int roll;

    if (Random(5) > 1)
        return 0;
    roll = Random(20);
    mineral = 0;
    switch (roll) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        mineral = MINERAL_IRON_ORE;
        break;
    case 6:
    case 7:
    case 8:
        mineral = MINERAL_COAL;
        break;
    case 9:
    case 10:
    case 11:
        mineral = MINERAL_SILVER;
        break;
    case 12:
    case 13:
    case 14:
        mineral = MINERAL_GOLD;
        break;
    case 15:
    case 16:
        mineral = MINERAL_GEMS;
        break;
    case 17:
    case 18:
        mineral = MINERAL_MITHRIL;
        break;
    case 19:
    case 20:
        mineral = MINERAL_ADAMANTIUM;
        break;
    }
    p_mineral_maps[plane][y][x] = mineral + p_mineral_maps[plane][y][x] & 0xe0;
}

/* (race, name) a random one of the race's 20 CITYNAME.LBX names not used by another city (after 200 tries any of
   a random race's) */
void City_Pick_Default_Name(int race, char far *name)
{
    int city;
    char *names;
    int tries;
    int pick;
    char existing[20];

    names = Near_Alloc_Reset(CITY_NAMES_PER_RACE * CITY_NAME_LEN);
    LBX_Load_Records_Near("CITYNAME", 0, names, race, 1, CITY_NAMES_PER_RACE * CITY_NAME_LEN);
    tries = 0;
retry:
    tries++;
    pick = Random(CITY_NAMES_PER_RACE) - 1;
    for (city = 0; city < num_cities; city++) {
        far_strcpy(existing, 0, p_cities[city].name);
        if (stricmp(existing, names + pick * CITY_NAME_LEN) == 0) {
            if (tries < 200)
                goto retry;
            else
                LBX_Load_Records_Near("CITYNAME", 0, names, Random(14) - 1, 1, CITY_NAMES_PER_RACE * CITY_NAME_LEN);
        }
    }
    far_strcpy(name, names + pick * CITY_NAME_LEN, 0);
}

/* (player, &x, &y, &plane, &mineral) -> the city whose radius holds a mineral (1..7) to deplete, -1. BUG #1: the
   loop runs while found == 0 but found starts at -1, so it never runs and the Depletion event never fires
   Numbered: BUG #1 (BUGS_AND_QUIRKS.txt) */
int Event_Find_Depletion_Site(int player, int *x, int *y, int *plane, int *mineral)
{
    int rounds;
    int found;
    int city;
    int tries;

    found = -1;
    for (rounds = 0; rounds < 100 && found == 0; rounds++) {
        city = Random(num_cities) - 1;
        if (p_cities[city].owner == player) {
            for (tries = 0; tries < 50 && found == -1; tries++) {
                *x = Random(5) - 3;
                *y = Random(5) - 3;
                if ((*x == -2 && *y == -2) || (*x == 2 && *y == -2) || (*x == -2 && *y == 2) || (*x == 2 && *y == 2))
                    continue;
                *x += p_cities[city].x;
                *y += p_cities[city].y;
                if (*x < 0)
                    *x += MAP_WIDTH;
                if (*x >= MAP_WIDTH)
                    *x -= MAP_WIDTH;
                if (*y > 0 && *y < MAP_HEIGHT) {
                    *plane = p_cities[city].plane;
                    if ((p_mineral_maps[*plane][*y][*x] & MINERAL_KIND_MASK) != MINERAL_NONE && (p_mineral_maps[*plane][*y][*x] & MINERAL_KIND_MASK) < MINERAL_QUORK) {
                        found = city;
                        *mineral = p_mineral_maps[*plane][*y][*x] & MINERAL_KIND_MASK;
                    }
                }
            }
        }
    }
    return found;
}

/* (player, &x, &y, &plane, &mineral) -> the city whose radius has hills or mountains without a mineral (mineral 2,
   or 4..7: gold, gems, mithril, adamantium), -1. Same BUG #1: never runs */
int Event_Find_New_Mineral_Site(int player, int *x, int *y, int *plane, int *mineral)
{
    int rounds;
    int found;
    int city;
    int tries;

    found = -1;
    for (rounds = 0; rounds < 100 && found == 0; rounds++) {
        city = Random(num_cities) - 1;
        if (p_cities[city].owner == player) {
            for (tries = 0; tries < 50 && found == -1; tries++) {
                *x = Random(5) - 3;
                *y = Random(5) - 3;
                if ((*x == -2 && *y == -2) || (*x == 2 && *y == -2) || (*x == -2 && *y == 2) || (*x == 2 && *y == 2))
                    continue;
                *x += p_cities[city].x;
                *y += p_cities[city].y;
                if (*x < 0)
                    *x += MAP_WIDTH;
                if (*x >= MAP_WIDTH)
                    *x -= MAP_WIDTH;
                if (*y > 0 && *y < MAP_HEIGHT) {
                    *plane = p_cities[city].plane;
                    if ((Terrain_Blend_Class(*x, *y, *plane) == BLEND_HILLS || Terrain_Blend_Class(*x, *y, *plane) == BLEND_MOUNTAIN)
                        && (p_mineral_maps[*plane][*y][*x] & MINERAL_KIND_MASK) == MINERAL_NONE) {
                        found = city;
                        if (Random(4) == 1)
                            *mineral = MINERAL_COAL;
                        else
                            *mineral = Random(4) + 3;   /* MINERAL_GOLD .. MINERAL_ADAMANTIUM */
                    }
                }
            }
        }
    }
    return found;
}

/* (x, y, plane, mineral) the tile's mineral nibble set */
void Tile_Set_Mineral(int x, int y, int plane, int mineral)
{
    p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] & ~MINERAL_KIND_MASK | mineral;
}

/* (x, y, plane) the tile's mineral nibble cleared */
void Tile_Clear_Mineral(int x, int y, int plane)
{
    p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] & ~MINERAL_KIND_MASK;
}

/* () rebuilds every city's road connections: a city with a road on one of its 8 neighbours (read unwrapped: BUG #27
   #27) is connected (both ways) to each later city on its plane and landmass that its road network reaches */
void Cities_Update_Road_Connections(void)
{
    int i;
    int other_city;
    unsigned city_lm;
    int plane;
    int cx;
    int cy;
    int dx;
    int dy;
    int map_cells;                              /* 2400, never read */
    int n_targets;
    int k;
    int cell;
    int has_road;
    int unused;                                 /* set to 0, never read */
    int targets[100];

    EMS_Map_Default();
    unused = 0;
    map_cells = MAP_SQUARES;
    for (i = 0; i < num_cities; i++)
        for (k = 0; k < num_cities / 8 + 1; k++)   /* the bytes in use (one bit per city); all 13 below */
            p_cities[i].road_connections[k] = 0;
    for (i = 0; i < num_cities; i++) {
        plane = p_cities[i].plane;
        cx = p_cities[i].x;
        cy = p_cities[i].y;
        city_lm = p_landmass_ids[plane][cy][cx];
        has_road = 0;
        for (dy = -1; dy < 2; dy++)
            for (dx = -1; dx < 2; dx++)
                if ((dx != 0 || dy != 0) && (p_terrain_flags_maps[plane][dy + cy][dx + cx] & TERRAIN_FLAG_ROAD) != 0)
                    has_road = 1;
        if (has_road == 1) {
            n_targets = 0;
            for (other_city = i + 1; other_city < num_cities; other_city++) {
                if (p_cities[i].plane == p_cities[other_city].plane
                    && p_landmass_ids[p_cities[other_city].plane][p_cities[other_city].y][p_cities[other_city].x] == city_lm) {
                    targets[n_targets] = other_city;
                    n_targets++;
                }
            }
            if (n_targets > 0) {
                for (dy = 0; dy < MAP_HEIGHT; dy++) {
                    for (dx = 0; dx < MAP_WIDTH; dx++) {
                        if ((p_terrain_flags_maps[plane][dy][dx] & TERRAIN_FLAG_ROAD) != 0) {
                            if (p_landmass_ids[plane][dy][dx] == city_lm)
                                p_path_work->cost[dy][dx] = 1;
                        } else
                            p_path_work->cost[dy][dx] = COST_IMPASSABLE;
                    }
                }
                Path_Relax(cx, cy, p_path_work);
                for (k = 0; k < n_targets; k++) {
                    cell = p_cities[targets[k]].x + p_cities[targets[k]].y * MAP_WIDTH;
                    if (p_path_work->prev[cell] != cell) {
                        Bit_Set(targets[k], p_cities[i].road_connections);
                        Bit_Set(i, p_cities[targets[k]].road_connections);
                    }
                }
            }
        }
    }
}

/* (x, y, plane) the same rebuild for the cities on the landmass of (x, y) only (after a road was built) */
void City_Road_Connections(int x, int y, int plane)
{
    int i;
    int other_city;
    unsigned city_lm;
    int cp;
    int cx;
    int cy;
    int dx;
    int dy;
    int map_cells;                              /* 2400, never read */
    int n_targets;
    int k;
    int cell;
    int has_road;
    unsigned lm;
    int targets[100];

    EMS_Map_Default();
    map_cells = MAP_SQUARES;
    lm = p_landmass_ids[plane][y][x];
    for (i = 0; i < num_cities; i++) {
        cp = p_cities[i].plane;
        cx = p_cities[i].x;
        cy = p_cities[i].y;
        city_lm = p_landmass_ids[cp][cy][cx];
        if (city_lm == lm)
            for (k = 0; k < 13; k++)
                p_cities[i].road_connections[k] = 0;
    }
    for (i = 0; i < num_cities; i++) {
        cp = p_cities[i].plane;
        cx = p_cities[i].x;
        cy = p_cities[i].y;
        city_lm = p_landmass_ids[cp][cy][cx];
        if (city_lm == lm) {
            has_road = 0;
            for (dy = -1; dy < 2; dy++)
                for (dx = -1; dx < 2; dx++)
                    if ((dx != 0 || dy != 0) && (p_terrain_flags_maps[cp][dy + cy][dx + cx] & TERRAIN_FLAG_ROAD) != 0)
                        has_road = 1;
            if (has_road == 1) {
                n_targets = 0;
                for (other_city = i + 1; other_city < num_cities; other_city++) {
                    if (p_cities[i].plane == p_cities[other_city].plane
                        && p_landmass_ids[p_cities[other_city].plane][p_cities[other_city].y][p_cities[other_city].x] == city_lm) {
                        targets[n_targets] = other_city;
                        n_targets++;
                    }
                }
                if (n_targets > 0) {
                    for (dy = 0; dy < MAP_HEIGHT; dy++) {
                        for (dx = 0; dx < MAP_WIDTH; dx++) {
                            if ((p_terrain_flags_maps[cp][dy][dx] & TERRAIN_FLAG_ROAD) != 0) {
                                if (p_landmass_ids[cp][dy][dx] == city_lm)
                                    p_path_work->cost[dy][dx] = 1;
                            } else
                                p_path_work->cost[dy][dx] = COST_IMPASSABLE;
                        }
                    }
                    Path_Relax(cx, cy, p_path_work);
                    for (k = 0; k < n_targets; k++) {
                        cell = p_cities[targets[k]].x + p_cities[targets[k]].y * MAP_WIDTH;
                        if (p_path_work->prev[cell] != cell) {
                            Bit_Set(targets[k], p_cities[i].road_connections);
                            Bit_Set(i, p_cities[targets[k]].road_connections);
                        }
                    }
                }
            }
        }
    }
}

/* (x1, y1, x2, y2, plane, xs, ys) -> the steps of the AI's road path from (x1, y1) to (x2, y2) over the start's
   landmass (road 1, other land 2), written start-exclusive into xs / ys; 0 from water or across landmasses
   Numbered: BUG #311 (BUGS_AND_QUIRKS.txt) */
int Road_Path_AI(int x1, int y1, int x2, int y2, int plane, char *xs, char *ys)
{
    int i;
    int steps;
    unsigned lm;
    int x;
    int y;
    int cell;

    EMS_Map_Default();
    lm = p_landmass_ids[plane][y1][x1];
    if (lm == 0)
        return 0;
    if (p_landmass_ids[plane][y2][x2] != lm)
        return 0;
    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            if (p_landmass_ids[plane][y][x] == lm) {
                if ((p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_ROAD) != 0)
                    p_path_work->cost[y][x] = 1;
                else
                    p_path_work->cost[y][x] = 2;
            } else
                p_path_work->cost[y][x] = COST_IMPASSABLE;
        }
    }
    if (p_path_work->cost[y2][x2] == COST_IMPASSABLE)
        return 0;
    Path_Relax(x1, y1, p_path_work);
    steps = 0;
    cell = x2 + y2 * MAP_WIDTH;
    while (p_path_work->prev[cell] != cell) {
        p_path_work->path[steps] = cell;
        cell = p_path_work->prev[cell];
        steps++;
    }
    for (i = 0; i < steps; i++) {
        x = p_path_work->path[steps - 1 - i] % MAP_WIDTH;
        y = p_path_work->path[steps - 1 - i] / MAP_WIDTH;
        xs[i] = x;
        ys[i] = y;
    }
    return steps;
}
