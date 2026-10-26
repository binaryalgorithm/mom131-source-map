/* BFGEN.C - WIZARDS.EXE overlay code segment df82 (stub 264b): the combat battlefield generator - the site (city,
   fortress, tower, node, lair), the terrain kind and the neighbouring roads of the map square, the houses and city
   walls, the islands of a sea battle, random terrain patches, a river, roads by random walks, the terrain clean-up
   passes, the tile pictures (auto-tiling), the movement cost layers, the terrain decorations; the grid <-> screen
   projection, Earth to Mud, and saving / restoring the world block in COMBAT.TMP around a battle.
   This module's data (DS:70a2..70c6): the four direction tables (dx / dy, with a fifth zero entry) and its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS df82 src/wizards/BFGEN.C */

/* Called here with no prototype in scope, as in SAVELOAD.C (a prototype would reject these argument counts):
     DOS_File_Read_Far / DOS_File_Write_Far (5 words: buffer offset, segment, size, count, handle; DOSFILE.C takes 4),
     DOS_File_Close (1 argument; the DOSFILE.C definition takes none).
   Shapes the bytes require: the retry loops of Lay_Roads / Scatter_Decor_A are labels with 'if (...) goto' (a
   do-while gives a short backward jcc, the game has jcc-over-jmp), and the decor site test is an empty then-branch
   ('if (site == 0 || site == 4) ; else if ...' - the game's je to a jmp). */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Battlefield_Generate(int x, int y, int plane);
void Battlefield_Generate_Map(int site, int house_style, int *roads, int enchanted_road, int terrain, int *rivers,
                              int flying_fortress, int att_island, int def_island, int walls, int population,
                              int wall_bits);
int Battlefield_Scatter_Decor_A(int terrain, int site);
void Battlefield_Scatter_Decor_B(int terrain, int site);
void Battlefield_Lay_Roads(int site, int *roads);
void Battlefield_Clear_Rough(int site);
void Battlefield_Fill_Variant_Gaps(void);
void Battlefield_Pick_Tiles(int terrain);
void Battlefield_Build_Move_Costs(int site, int walls);
void Battlefield_Random_Patches(int terrain, int count, int len_rand, int len_min);
void Combat_Grid_To_Screen(int gx, int gy, int ox, int oy, int *sx, int *sy);
void Earth_To_Mud_Square(int x, int y);
int Combat_Screen_To_Grid_X(int sx, int sy);
int Combat_Screen_To_Grid_Y(int sx, int sy);
void Combat_Screen_To_Grid_X_Sub(int sx, int sy, int *cell, int *sub);
void Combat_Screen_To_Grid_Y_Sub(int sx, int sy, int *cell, int *sub);
void Combat_Tmp_Restore(void);
void Combat_Tmp_Save(void);
void Battlefield_Lay_River(int *rivers);

#include "GAME.H"

#define BF_DIR_DOWN 0                   /* the road walks' directions (battlefield_dir_dx / _dy) */
#define BF_DIR_LEFT 1
#define BF_DIR_UP 2
#define BF_DIR_RIGHT 3

/* MAGIC's WORLDGEN.C step_dx / walk_dx hold the same table */
int battlefield_dir_dx[5] = { 0, -1, 0, 1, 0 };          /* DS:70a2  direction 0 down, 1 left, 2 up, 3 right */
int battlefield_dir_dy[5] = { 1, 0, -1, 0, 0 };          /* DS:70ac */

extern int wall_set_index;                      /* DS:706e */
extern int battlefield_reuse_map;               /* DS:709c */
extern int combat_wall_rising;                /* DS:9272 */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern int world_block_paras;                   /* DS:9d1a */
extern unsigned p_world_block;                /* DS:9d20 */
extern int num_cities;                          /* DS:bd94 */
extern int num_players;                         /* DS:bd9c */
extern int battlefield_enchanted_road;                   /* DS:cfa0 */
extern unsigned battlefield_decor_b_pics[5];             /* DS:cfda */
extern unsigned battlefield_house_pics[];            /* DS:d11c */
extern int vortex_count;                        /* DS:d152 */
extern int combat_def_floating_island;          /* DS:d380 */
extern int combat_att_floating_island;          /* DS:d382 */
extern struct RaceInfo race_table[14];          /* DS:1d86 */

long Random_Get_Seed(void);
void Random_Seed(long seed);
int Tile_Is_Forest_Explored(int x, int y, int plane);
int Tile_Is_Hills(int x, int y, int plane);
int Tile_Is_Mountain(int x, int y, int plane);
int Tile_Is_Desert(int x, int y, int plane);
int Tile_Is_Swamp(int x, int y, int plane);
int Tile_Is_Grassland(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Tundra(int x, int y, int plane);
int Load_Combat_Terrain_Tiles(int bf_terrain, int plane);
void Battlefield_Draw_Terrain(void);
int DOS_File_Open(char *path, char *mode);
int DOS_File_Read_Far();                       /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
int DOS_File_Write_Far();                      /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
void DOS_File_Close();                         /* no prototype here: called with the handle; DOSFILE.C takes none (it closes the stored one) */

/* (x, y, plane) generates the battlefield of a map square: the site (0 open, 1 outpost, 2 city, 3 fortress city,
   4 tower, 5..7 node by realm, 8.. lair by type), the city's house style, walls and wall enchantments, the
   neighbouring roads, the terrain kind; the map is generated from a seed of the square (the random sequence is
   restored afterwards) and the terrain drawn
   Numbered: BUG #23 (BUGS_AND_QUIRKS.txt) */
void Battlefield_Generate(int x, int y, int plane)
{
    int player;
    int tx;
    int ty;
    long seed;
    int site;
    int population;
    int house_style;
    int enchanted_road;
    int terrain;
    int flying_fortress;
    int fortress_here;                          /* set, never read */
    int walls;
    int wall_bits;
    char far *src;                              /* first the bytes to zero (city_enchants: 30 bytes, 4 past the 26-entry array), then the copy source */
    char far *dst;
    int roads[9];                               /* the 3 x 3 squares around: (dx + 1) + (dy + 1) * 3 -
                                                   0 NW, 1 N, 2 NE, 3 W, 4 here, 5 E, 6 SW, 7 S, 8 SE */
    int rivers[9];
    int i;

    combat_wall_rising = 0;
    wall_bits = 0;
    src = p_battlefield->city_enchants;
    for (i = 0; i < 30; i++)                    /* 30, not the 26 city_enchants: the last 4 land in unused_1598 (no
                                                   effect); clarified using ReMoM repo */
        src[i] = 0;
    site = BF_SITE_OPEN;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x != x)
            continue;
        if (p_cities[i].y != y)
            continue;
        if (p_cities[i].plane != plane)
            continue;
        population = p_cities[i].population;
        if (population == 0)
            site = BF_SITE_OUTPOST;
        else {
            site = BF_SITE_CITY;
            for (player = 0; player < num_players; player++)
                if (p_fortresses[player].active == 1 && p_fortresses[player].x == x && p_fortresses[player].y == y &&
                    p_fortresses[player].plane == plane) {
                    site = BF_SITE_FORTRESS;
                    fortress_here = 1;
                }
            house_style = race_table[p_cities[i].race].house_style;   /* styles 1 and 2 swap here (CITYSCAP.C
                                                   uses the race's style as it is) */
            if (house_style == HOUSE_STYLE_TREE)
                house_style = HOUSE_STYLE_HUT;
            else if (house_style == HOUSE_STYLE_HUT)
                house_style = HOUSE_STYLE_TREE;
            if (p_cities[i].enchants[CITY_ENCH_FLYING_FORTRESS] != 0)
                flying_fortress = 1;
            else
                flying_fortress = 0;
            wall_bits = 0;
            if (p_cities[i].enchants[CITY_ENCH_WALL_OF_FIRE] != 0)
                wall_bits = 1;
            if (p_cities[i].enchants[CITY_ENCH_WALL_OF_DARKNESS] != 0)
                wall_bits += 2;
            if (p_cities[i].buildings[BLD_CITY_WALLS] >= 0)
                walls = 1;
            else
                walls = 0;
        }
        for (src = p_cities[i].enchants, dst = p_battlefield->city_enchants; src <= &p_cities[i].enchants[CITY_ENCH_NIGHTSHADE];
             src++, dst++)
            *dst = *src;
    }
    p_battlefield->plane = plane;
    for (i = 0; i < NUM_TOWERS; i++)
        if (p_towers[i].x == x && p_towers[i].y == y)
            site = BF_SITE_TOWER;
    for (i = 0; i < NUM_NODES; i++)
        if (p_nodes[i].x == x && p_nodes[i].y == y && p_nodes[i].plane == plane)
            site = BF_SITE_SORCERY_NODE - p_nodes[i].realm;   /* NODE_SORCERY 0 .. NODE_CHAOS 2 */
    for (i = 0; i < NUM_LAIRS; i++)
        if (p_lairs[i].x == x && p_lairs[i].y == y && p_lairs[i].plane == plane && p_lairs[i].active == 1)
            site = p_lairs[i].type + BF_SITE_LAIR_BASE;   /* LAIR_MYSTERIOUS_CAVE 4 -> 8 .. */
    for (i = 0; i < 9; i++)
        roads[i] = 0;
    for (i = 0; i < 9; i++)
        rivers[i] = 0;
    for (ty = y - 1; ty < y + 2; ty++)
        for (tx = x - 1; tx < x + 2; tx++)
            if (ty >= 0 && ty < MAP_HEIGHT && tx >= 0 && tx < MAP_WIDTH && (p_terrain_flags_maps[plane][ty][tx] & TERRAIN_FLAG_ROAD))
                roads[(tx - x) + (ty - y + 1) * 3 + 1] = 1;
    if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_ENCHANTED_ROAD)
        enchanted_road = 1;
    else
        enchanted_road = 0;
    terrain = BF_TERRAIN_OCEAN;
    if (Tile_Is_Forest_Explored(x, y, plane) == 1)
        terrain = BF_TERRAIN_FOREST;
    else if (Tile_Is_Hills(x, y, plane) == 1)
        terrain = BF_TERRAIN_HILLS;
    else if (Tile_Is_Mountain(x, y, plane) == 1) {
        terrain = BF_TERRAIN_MOUNTAIN;
        wall_set_index = 0;
    } else if (Tile_Is_Desert(x, y, plane) == 1) {
        terrain = BF_TERRAIN_DESERT;
        wall_set_index = 0;
    } else if (Tile_Is_Swamp(x, y, plane) == 1 || Tile_Is_Grassland(x, y, plane) == 1)
        terrain = BF_TERRAIN_GRASSLAND;
    else if (Tile_Is_River(x, y, plane) == 1)
        terrain = BF_TERRAIN_GRASSLAND;
    else if (Tile_Is_Tundra(x, y, plane) == 1)
        terrain = BF_TERRAIN_TUNDRA;
    else
        terrain = BF_TERRAIN_OCEAN;
    if (site == BF_SITE_NATURE_NODE)
        terrain = BF_TERRAIN_FOREST;
    if (plane == PLANE_MYRROR)                  /* CITYWALL sets: 0 light stone, 1 dark stone (Myrror), 2 green-marked */
        wall_set_index = 1;                     /* (never chosen); other terrains keep the previous battle's set */
    seed = Random_Get_Seed();
    Random_Seed((long)x * y * (plane + 5) + 0x2737);
    Load_Combat_Terrain_Tiles(terrain, plane);
    if (battlefield_reuse_map == 0)
        Battlefield_Generate_Map(site, house_style, roads, enchanted_road, terrain, rivers, flying_fortress,
                                 combat_att_floating_island, combat_def_floating_island, walls, population, wall_bits);
    vortex_count = 0;
    Random_Seed(seed);
    Battlefield_Draw_Terrain();
}

/* (site, house style, roads, enchanted road, terrain, rivers, flying_fortress (unused), attacker / defender island, walls, population,
   wall bits) the battlefield map: the backdrop of the site, a city's houses (a full block from 12 people, 11 for a
   fortress city, else one random house per person) and its wall segments, the wall enchantments; a sea battle's
   floating islands, or the land terrain: random patches, the river, roads and the clean-up passes; then the tile
   pictures, movement costs and decorations
   Numbered: BUG #326 (BUGS_AND_QUIRKS.txt) */
void Battlefield_Generate_Map(int site, int house_style, int *roads, int enchanted_road, int terrain, int *rivers,
                              int flying_fortress, int att_island, int def_island, int walls, int population,
                              int wall_bits)
{
    int patches;
    int patches2;
    int house;
    int blocked;
    int rx;
    int ry;
    long seed;
    register int x;
    register int y;

    p_battlefield->house_count = 0;
    for (y = 0; y <= 3; y++)
        for (x = 0; x <= 3; x++)
            p_battlefield->wall_segments[y][x] = WALL_NONE;
    seed = Random_Get_Seed();
    switch (site) {
    case BF_SITE_OPEN:
        p_battlefield->backdrop = BACKDROP_FIELD;
        break;
    case BF_SITE_OUTPOST:                      /* walls not read: an outpost battle never has walls (BUG #271:
                                                   Battlefield_Generate does not compute them for an outpost either);
                                                   clarified using ReMoM repo */
        p_battlefield->backdrop = BACKDROP_OUTPOST;
        break;
    case BF_SITE_CITY:
        p_battlefield->backdrop = BACKDROP_CITY;
        if (population >= 12) {
            house = 0;
            for (y = 0; y <= 3; y++)
                for (x = 0; x <= 3; x++)
                    if (((x != 0 || y != 0) && (x != 3 || y != 0) && (x != 0 || y != 3) && (x != 3 || y != 3)) ||
                        walls == 0) {
                        p_battlefield->house_x[house] = x + 5;
                        p_battlefield->house_y[house] = y + 10;
                        p_battlefield->house_pic[house] = battlefield_house_pics[house_style * 5 + Random(5) - 1];
                        house++;
                    }
            p_battlefield->house_count = 12;
        } else {
            p_battlefield->house_count = 0;
            while (p_battlefield->house_count < population) {
                rx = Random(4) - 1;
                ry = Random(4) - 1;
                x = rx + 5;
                y = ry + 10;
                blocked = 0;
                if (walls == 1) {
                    if (rx == 0 && ry == 0)
                        blocked = 1;
                    if (rx == 0 && ry == 3)
                        blocked = 1;
                    if (rx == 3 && ry == 0)
                        blocked = 1;
                    if (rx == 3 && ry == 3)
                        blocked = 1;
                }
                for (house = 0; house < p_battlefield->house_count; house++)
                    if (p_battlefield->house_x[house] == x && p_battlefield->house_y[house] == y)
                        blocked = 1;
                if (blocked == 0) {
                    p_battlefield->house_x[p_battlefield->house_count] = x;
                    p_battlefield->house_y[p_battlefield->house_count] = y;
                    p_battlefield->house_pic[p_battlefield->house_count] =
                        battlefield_house_pics[house_style * 5 + Random(5) - 1];
                    p_battlefield->house_count++;
                }
            }
        }
        p_battlefield->has_city_walls = walls;
        if (walls == 1) {
            for (y = 0; y <= 3; y++)
                for (x = 0; x <= 3; x++)
                    p_battlefield->wall_segments[y][x] = WALL_STANDING;
            p_battlefield->wall_segments[1][1] = WALL_NONE;
            p_battlefield->wall_segments[2][1] = WALL_NONE;
            p_battlefield->wall_segments[1][2] = WALL_NONE;
            p_battlefield->wall_segments[2][2] = WALL_NONE;
        }
        break;
    case BF_SITE_FORTRESS:
        p_battlefield->backdrop = BACKDROP_FORTRESS;
        if (population >= 11) {
            house = 0;
            for (y = 0; y <= 3; y++)
                for (x = 0; x <= 3; x++)
                    if ((((x != 0 || y != 0) && (x != 3 || y != 0) && (x != 0 || y != 3) && (x != 3 || y != 3)) ||
                         walls == 0) && (x != 1 || y != 1)) {
                        p_battlefield->house_x[house] = x + 5;
                        p_battlefield->house_y[house] = y + 10;
                        p_battlefield->house_pic[house] = battlefield_house_pics[house_style * 5 + Random(5) - 1];
                        house++;
                    }
            p_battlefield->house_count = 11;
        } else {
            p_battlefield->house_count = 0;
            while (p_battlefield->house_count < population) {
                rx = Random(4) - 1;
                ry = Random(4) - 1;
                x = rx + 5;
                y = ry + 10;
                blocked = 0;
                if (rx == 1 && ry == 1)
                    blocked = 1;
                if (walls == 1) {
                    if (rx == 0 && ry == 0)
                        blocked = 1;
                    if (rx == 0 && ry == 3)
                        blocked = 1;
                    if (rx == 3 && ry == 0)
                        blocked = 1;
                    if (rx == 3 && ry == 3)
                        blocked = 1;
                }
                for (house = 0; house < p_battlefield->house_count; house++)
                    if (p_battlefield->house_x[house] == x && p_battlefield->house_y[house] == y)
                        blocked = 1;
                if (blocked == 0) {
                    p_battlefield->house_x[p_battlefield->house_count] = x;
                    p_battlefield->house_y[p_battlefield->house_count] = y;
                    p_battlefield->house_pic[p_battlefield->house_count] =
                        battlefield_house_pics[house_style * 5 + Random(5) - 1];
                    p_battlefield->house_count++;
                }
            }
        }
        p_battlefield->has_city_walls = walls;
        if (walls == 1) {
            for (y = 0; y <= 3; y++)
                for (x = 0; x <= 3; x++)
                    p_battlefield->wall_segments[y][x] = WALL_STANDING;
            p_battlefield->wall_segments[1][1] = WALL_NONE;
            p_battlefield->wall_segments[2][1] = WALL_NONE;
            p_battlefield->wall_segments[1][2] = WALL_NONE;
            p_battlefield->wall_segments[2][2] = WALL_NONE;
        }
        break;
    case BF_SITE_TOWER:
        p_battlefield->backdrop = BACKDROP_TOWER;
        break;
    case BF_SITE_CHAOS_NODE:
        p_battlefield->backdrop = BACKDROP_CHAOS_NODE;
        break;
    case BF_SITE_NATURE_NODE:
        p_battlefield->backdrop = BACKDROP_NATURE_NODE;
        break;
    case BF_SITE_SORCERY_NODE:
        p_battlefield->backdrop = BACKDROP_SORCERY_NODE;
        break;
    case BF_SITE_CAVE:
        p_battlefield->backdrop = BACKDROP_CAVE_LAIR;
        break;
    case BF_SITE_DUNGEON:
        p_battlefield->backdrop = BACKDROP_DUNGEON_RUINS;
        break;
    case BF_SITE_ANCIENT_TEMPLE:
        p_battlefield->backdrop = BACKDROP_ANCIENT_TEMPLE;
        break;
    case BF_SITE_KEEP:
        p_battlefield->backdrop = BACKDROP_KEEP;
        break;
    case BF_SITE_MONSTER_LAIR:
        p_battlefield->backdrop = BACKDROP_CAVE_LAIR;
        break;
    case BF_SITE_RUINS:
        p_battlefield->backdrop = BACKDROP_DUNGEON_RUINS;
        break;
    case BF_SITE_FALLEN_TEMPLE:
        p_battlefield->backdrop = BACKDROP_FALLEN_TEMPLE;
        break;
    }
    Random_Seed(seed);
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++)
            p_battlefield->mud[y][x] = 0;
    if (wall_bits & 1)
        p_battlefield->wall_of_fire = 1;
    else
        p_battlefield->wall_of_fire = 0;
    if (wall_bits & 2)
        p_battlefield->wall_of_darkness = 1;
    else
        p_battlefield->wall_of_darkness = 0;
    battlefield_enchanted_road = enchanted_road;
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++)
            p_battlefield->road[y][x] = 0;
    if (terrain == BF_TERRAIN_OCEAN) {
        for (y = 0; y < BATTLEFIELD_H; y++)
            for (x = 0; x < BATTLEFIELD_W; x++)
                p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_OCEAN;
        if (def_island == 1) {
            p_battlefield->road[12][5] = BF_ROAD_ISLAND;
            p_battlefield->road[12][6] = BF_ROAD_ISLAND;
            p_battlefield->road[12][7] = BF_ROAD_ISLAND;
            p_battlefield->road[11][5] = BF_ROAD_ISLAND;
            p_battlefield->road[11][6] = BF_ROAD_ISLAND;
            p_battlefield->road[11][7] = BF_ROAD_ISLAND;
            p_battlefield->road[13][5] = BF_ROAD_ISLAND;
            p_battlefield->road[13][6] = BF_ROAD_ISLAND;
            p_battlefield->road[13][7] = BF_ROAD_ISLAND;
            p_battlefield->road[13][8] = BF_ROAD_ISLAND_PIC;
        }
        if (att_island == 1) {
            if (def_island == 1) {
                p_battlefield->road[12][8] = BF_ROAD_ISLAND;
                p_battlefield->road[12][9] = BF_ROAD_ISLAND;
                p_battlefield->road[12][10] = BF_ROAD_ISLAND;
                p_battlefield->road[12][11] = BF_ROAD_ISLAND;
                p_battlefield->road[11][9] = BF_ROAD_ISLAND;
                p_battlefield->road[11][10] = BF_ROAD_ISLAND;
                p_battlefield->road[11][11] = BF_ROAD_ISLAND;
                p_battlefield->road[13][9] = BF_ROAD_ISLAND;
                p_battlefield->road[13][10] = BF_ROAD_ISLAND;
                p_battlefield->road[13][11] = BF_ROAD_ISLAND;
                p_battlefield->road[13][12] = BF_ROAD_ISLAND_PIC;
            } else {
                p_battlefield->road[11][16] = BF_ROAD_ISLAND;
                p_battlefield->road[11][17] = BF_ROAD_ISLAND;
                p_battlefield->road[11][18] = BF_ROAD_ISLAND;
                p_battlefield->road[10][16] = BF_ROAD_ISLAND;
                p_battlefield->road[10][17] = BF_ROAD_ISLAND;
                p_battlefield->road[10][18] = BF_ROAD_ISLAND;
                p_battlefield->road[12][16] = BF_ROAD_ISLAND;
                p_battlefield->road[12][17] = BF_ROAD_ISLAND;
                p_battlefield->road[12][18] = BF_ROAD_ISLAND;
                p_battlefield->road[12][19] = BF_ROAD_ISLAND_PIC;
            }
        }
    } else {
        for (y = 0; y < BATTLEFIELD_H; y++)
            for (x = 0; x < BATTLEFIELD_W; x++)
                p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_PLAIN;
        patches = 0;
        patches2 = 8;
        if (terrain == BF_TERRAIN_GRASSLAND)
            patches = 5;
        if (terrain == BF_TERRAIN_HILLS)
            patches = 20;
        if (terrain == BF_TERRAIN_DESERT)
            patches = 10;
        if (terrain == BF_TERRAIN_MOUNTAIN)
            patches = 30;
        if (terrain == BF_TERRAIN_TUNDRA)
            patches = 10;
        if (terrain == BF_TERRAIN_FOREST)
            patches = 5;
        Battlefield_Random_Patches(BATTLE_TERRAIN_ROUGH, patches, 8, 3);
        Battlefield_Random_Patches(BATTLE_TERRAIN_VARIANT, patches2, 4, 0);
        if (terrain == BF_TERRAIN_GRASSLAND)
            Battlefield_Lay_River(rivers);
        Battlefield_Lay_Roads(site, roads);
        Battlefield_Clear_Rough(site);
        Battlefield_Fill_Variant_Gaps();
    }
    Battlefield_Pick_Tiles(terrain);
    Battlefield_Build_Move_Costs(site, walls);
    if (terrain == BF_TERRAIN_OCEAN) {
        p_battlefield->decor_a_count = 0;
        p_battlefield->decor_b_count = 0;
    } else {
        p_battlefield->decor_a_count = 0;
        p_battlefield->decor_b_count = 0;
        Battlefield_Scatter_Decor_A(terrain, site);
        Battlefield_Scatter_Decor_B(terrain, site);
    }
}

/* (terrain, site) the first decoration set in clusters: a random count by terrain, clusters of count / clusters
   around a free random square, kept off the city / feature squares; each raises its square's ground and mode-3 move cost by 1 (to
   at most 4). BUG #430: the bounds test checks the cluster centre,
   not the placed square. The result is meaningless (0 for no decorations; no value otherwise) */
int Battlefield_Scatter_Decor_A(int terrain, int site)
{
    int count;
    int blocked;
    int k;
    int ox;
    int oy;
    int sx;
    int sy;
    int clusters;
    int cx;
    int cy;
    int cluster;
    int tries;
    int per;
    register int x;
    register int y;

    if (terrain == BF_TERRAIN_GRASSLAND || terrain == BF_TERRAIN_HILLS)
        count = Random(20);
    else if (terrain == BF_TERRAIN_DESERT)
        count = Random(10);
    else if (terrain == BF_TERRAIN_MOUNTAIN)
        count = Random(40);
    else if (terrain == BF_TERRAIN_TUNDRA)
        count = Random(20);
    else
        count = Random(30) + 30;
    p_battlefield->decor_a_count = 0;
    if (count == 0)
        return 0;
    clusters = Random(4) + count / 10;
    if (count < clusters)
        return 0;
    per = count / clusters;
    if (per < 1)
        per = 1;
    tries = 0;
    for (cluster = 0; cluster < clusters; cluster++) {
    pick:
        if (tries > 500)
            return;
        cx = Random(21) - 1;
        cy = Random(22) - 1;
        if (p_battlefield->road[cy][cx] != 0)
            goto pick;
        if (p_battlefield->terrain_type[cy][cx] != BATTLE_TERRAIN_PLAIN)
            goto pick;
        if (p_battlefield->tile[cy][cx] == 0xff)   /* no code in the sources writes tile 0xff */
            goto pick;
        for (k = 0; k < per; k++) {
            x = Random(7) + cx - 4;
            y = Random(7) + cy - 4;
            ox = Random(6);
            oy = Random(6);
            blocked = 0;
            if (cx < 0)
                blocked = 1;
            if (cy < 0)
                blocked = 1;
            if (cx >= 21)
                blocked = 1;
            if (cy >= 22)
                blocked = 1;
            if (site == BF_SITE_OPEN || site == BF_SITE_TOWER)
                ;
            else if (y >= 11 && y <= 13 && x >= 5 && x <= 7)
                blocked = 1;
            if (p_battlefield->road[y][x] != 0)
                blocked = 1;
            if (p_battlefield->terrain_type[y][x] != BATTLE_TERRAIN_PLAIN)
                blocked = 1;
            if (p_battlefield->tile[y][x] == 0xff)
                blocked = 1;
            if (p_battlefield->backdrop != BACKDROP_FIELD && p_battlefield->backdrop != BACKDROP_NATURE_NODE && x >= 5 && x <= 7 && y >= 10 &&
                y <= 12)
                blocked = 1;
            if ((p_battlefield->backdrop == BACKDROP_CITY || p_battlefield->backdrop == BACKDROP_FORTRESS) &&
                ((x == 8 && y == 10) || (x == 8 && y == 11) || (x == 8 && y == 12) || (x == 8 && y == 13) ||
                 (x == 5 && y == 13) || (x == 6 && y == 13) || (x == 7 && y == 13) || (x == 8 && y == 13)))
                blocked = 1;
            Combat_Grid_To_Screen(x, y, ox, oy, &sx, &sy);
            if (blocked == 0 && sx > 0 && sx < 320 && sy > 0 && sy < 180) {
                p_battlefield->decor_a_x[p_battlefield->decor_a_count] = sx - 5;
                p_battlefield->decor_a_y[p_battlefield->decor_a_count] = sy;
                if (p_battlefield->cost_ground[y][x] < 4)
                    p_battlefield->cost_ground[y][x] = p_battlefield->cost_ground[y][x] + 1;
                if (p_battlefield->cost_mode3[y][x] < 4)
                    p_battlefield->cost_mode3[y][x] = p_battlefield->cost_mode3[y][x] + 1;
                p_battlefield->decor_a_index[p_battlefield->decor_a_count] = Random(5) - 1;
                p_battlefield->decor_a_count++;
            } else {
                tries++;
                if (tries < 500)
                    k--;
            }
        }
    }
}

/* (terrain, site) the second decoration set, one by one on free squares (each raising its square's ground and
   mode-3 move cost by 1, to at most 4). BUG #23: an operator precedence slip makes
   the city-gate exclusion apply to some squares on every battlefield
   Numbered: BUG #23 (BUGS_AND_QUIRKS.txt) */
void Battlefield_Scatter_Decor_B(int terrain, int site)
{
    int count;
    int blocked;
    int k;
    int ox;
    int oy;
    int sx;
    int sy;
    register int x;
    register int y;

    if (terrain == BF_TERRAIN_GRASSLAND || terrain == BF_TERRAIN_HILLS)
        count = Random(5) - 1;
    else if (terrain == BF_TERRAIN_DESERT)
        count = Random(8) - 1;
    else if (terrain == BF_TERRAIN_MOUNTAIN)
        count = Random(12);
    else if (terrain == BF_TERRAIN_TUNDRA)
        count = Random(12) + 4;
    else
        count = Random(8);
    p_battlefield->decor_b_count = 0;
    for (k = 0; k < count; k++) {
        x = Random(21) - 1;
        y = Random(22) - 1;
        ox = Random(2) + 3;
        oy = Random(2) + 3;
        blocked = 0;
        if (site == BF_SITE_OPEN || site == BF_SITE_TOWER)
            ;
        else if (y >= 11 && y <= 13 && x >= 5 && x <= 7)
            blocked = 1;
        if (p_battlefield->road[y][x] != 0)
            blocked = 1;
        if (p_battlefield->terrain_type[y][x] != BATTLE_TERRAIN_PLAIN)
            blocked = 1;
        if (p_battlefield->tile[y][x] == 0xff)
            blocked = 1;
        if (p_battlefield->backdrop != BACKDROP_FIELD && p_battlefield->backdrop != BACKDROP_NATURE_NODE && x >= 5 && x <= 7 && y >= 10 && y <= 12)
            blocked = 1;
        if ((p_battlefield->backdrop == BACKDROP_CITY || p_battlefield->backdrop == BACKDROP_FORTRESS) && x == 8 && y == 10 || x == 8 && y == 11 ||
            x == 8 && y == 12 || x == 8 && y == 13 || x == 5 && y == 13 || x == 6 && y == 13 || x == 7 && y == 13 ||
            x == 8 && y == 13)
            blocked = 1;
        Combat_Grid_To_Screen(x, y, ox, oy, &sx, &sy);
        if (blocked == 0 && sx > 0 && sx < 320 && sy > 0 && sy < 180) {
            p_battlefield->decor_b_x[p_battlefield->decor_b_count] = sx - 5;
            p_battlefield->decor_b_y[p_battlefield->decor_b_count] = sy;
            if (p_battlefield->cost_ground[y][x] < 4)
                p_battlefield->cost_ground[y][x] = p_battlefield->cost_ground[y][x] + 1;
            if (p_battlefield->cost_mode3[y][x] < 4)
                p_battlefield->cost_mode3[y][x] = p_battlefield->cost_mode3[y][x] + 1;
            p_battlefield->decor_b_pic[p_battlefield->decor_b_count] = battlefield_decor_b_pics[Random(5) - 1];
            p_battlefield->decor_b_count++;
        } else
            k--;
    }
}

/* (site, roads) roads by random walks from the centre (for a city or fortress from inside the
   city core: (7, 12), (7, 11) or (6, 12) by direction - the gate is (8, 12)) toward each side with a road on
   the map; then each road square's connection bits from its neighbours, and half the squares lose bit 1 */
void Battlefield_Lay_Roads(int site, int *roads)
{
    int start_x;
    int start_y;
    int dir;
    int nx;
    int ny;
    int straight;
    int n0;
    int n1;
    int n2;
    int prev_dir;
    int n3;
    int n4;
    int n5;
    int n6;
    int n7;
    int n8;
    register int x;
    register int y;

    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++)
            p_battlefield->road[y][x] = 0;
    if (roads[4] == 1) {
        start_x = 10;
        start_y = 10;
        if (roads[5] == 1 || roads[2] == 1) {
            x = start_x;
            y = start_y;
            if (site == BF_SITE_CITY || site == BF_SITE_FORTRESS) {
                x = 7;
                y = 12;
            }
            p_battlefield->road[y][x] = 1;
            prev_dir = BF_DIR_RIGHT;
            dir = BF_DIR_RIGHT;
            straight = 0;
            while (x < 20) {
                if (straight < 2) {
                    dir = prev_dir;
                    straight++;
                } else {
                    retry1:
                    if (Random(4) != 1)
                        dir = BF_DIR_RIGHT;
                    else
                        dir = Random(4) - 1;
                    if (dir == BF_DIR_LEFT)
                        goto retry1;
                    straight = 1;
                }
                prev_dir = dir;
                nx = x + battlefield_dir_dx[dir];
                ny = y + battlefield_dir_dy[dir];
                if (nx >= 0 && nx < 21 && ny >= 0 && dir != BF_DIR_LEFT && ny < 22) {
                    x = nx;
                    y = ny;
                    p_battlefield->road[y][x] = 1;
                }
            }
        }
        if (roads[1] == 1 || roads[0] == 1) {
            x = start_x;
            y = start_y;
            if (site == BF_SITE_CITY || site == BF_SITE_FORTRESS) {
                x = 7;
                y = 11;
            }
            p_battlefield->road[y][x] = 1;
            prev_dir = BF_DIR_UP;
            straight = 0;
            dir = BF_DIR_UP;
            while (y > 0) {
                if (straight < 2) {
                    dir = prev_dir;
                    straight++;
                } else {
                    retry2:
                    if (Random(3) != 1)
                        dir = BF_DIR_UP;
                    else
                        dir = Random(4) - 1;
                    if (dir == BF_DIR_DOWN)
                        goto retry2;
                    if (dir != prev_dir)
                        straight = 1;
                }
                prev_dir = dir;
                nx = x + battlefield_dir_dx[dir];
                ny = y + battlefield_dir_dy[dir];
                if (nx >= 0 && nx < 21 && ny >= 0 && ny < 22) {
                    x = nx;
                    y = ny;
                    p_battlefield->road[y][x] = 1;
                }
            }
        }
        if (roads[3] == 1 || roads[6] == 1) {
            x = start_x;
            y = start_y;
            if (site == BF_SITE_CITY || site == BF_SITE_FORTRESS) {
                x = 6;
                y = 12;
            }
            p_battlefield->road[y][x] = 1;
            prev_dir = BF_DIR_LEFT;
            dir = BF_DIR_DOWN;
            straight = 0;
            while (x > 0) {
                if (straight < 2) {
                    dir = prev_dir;
                    straight++;
                } else {
                    retry3:
                    if (Random(4) != 1)
                        dir = BF_DIR_LEFT;
                    else
                        dir = Random(4) - 1;
                    if (dir == BF_DIR_RIGHT)
                        goto retry3;
                    straight = 1;
                }
                prev_dir = dir;
                nx = x + battlefield_dir_dx[dir];
                ny = y + battlefield_dir_dy[dir];
                if (nx >= 0 && nx < 21 && ny >= 0 && ny < 22) {
                    x = nx;
                    y = ny;
                    p_battlefield->road[y][x] = 1;
                }
            }
        }
        if (roads[7] == 1 || roads[8] == 1) {
            x = start_x;
            y = start_y;
            if (site == BF_SITE_CITY || site == BF_SITE_FORTRESS) {
                x = 7;
                y = 12;
            }
            p_battlefield->road[y][x] = 1;
            prev_dir = BF_DIR_DOWN;
            dir = BF_DIR_DOWN;
            straight = 0;
            while (y < 21) {
                if (straight < 2) {
                    dir = prev_dir;
                    straight++;
                } else {
                    retry4:
                    if (Random(4) != 1)
                        dir = BF_DIR_DOWN;
                    else
                        dir = Random(4) - 1;
                    if (dir == BF_DIR_UP)
                        goto retry4;
                    straight = 1;
                }
                prev_dir = dir;
                nx = x + battlefield_dir_dx[dir];
                ny = y + battlefield_dir_dy[dir];
                if (nx >= 0 && nx < 21 && ny >= 0 && ny < 22) {
                    x = nx;
                    y = ny;
                    p_battlefield->road[y][x] = 1;
                }
            }
        }
    }
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++) {
            if (p_battlefield->road[y][x] == 0)
                continue;
            n0 = p_battlefield->road[y - 1][x - 1];
            n1 = p_battlefield->road[y - 1][x];
            n2 = p_battlefield->road[y - 1][x + 1];
            n3 = p_battlefield->road[y][x - 1];
            n4 = p_battlefield->road[y][x];
            n5 = p_battlefield->road[y][x + 1];
            n6 = p_battlefield->road[y + 1][x - 1];
            n7 = p_battlefield->road[y + 1][x];
            n8 = p_battlefield->road[y + 1][x + 1];
            if (y == 0) {
                n0 = 1;
                n1 = 1;
                n2 = 1;
            }
            if (y == 21) {
                n6 = 1;
                n7 = 1;
                n8 = 1;
            }
            if (x == 0) {
                n0 = 1;
                n3 = 1;
                n6 = 1;
            }
            if (x == 20) {
                n2 = 1;
                n5 = 1;
                n8 = 1;
            }
            if (n3 != 0 && n5 != 0)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] + 2;
            if (n1 != 0 && n7 != 0)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] + 4;
            if (n1 != 0 && n3 != 0)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] + 8;
            if (n5 != 0 && n1 != 0)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] + 0x10;
            if (n7 != 0 && n5 != 0)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] + 0x20;
            if (n3 != 0 && n7 != 0)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] + 0x40;
        }
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++)
            if (p_battlefield->road[y][x] != 0 && Random(2) == 1)
                p_battlefield->road[y][x] = p_battlefield->road[y][x] & 0xfe;   /* the value-1 bit: COMBSPRT.C's
                                                   road picture set */
}

/* (rivers) a river (terrain type 3) by a random walk: from the bottom edge northward when the map's neighbours carry
   it east / north / west, else from a random bottom column. Dead in 1.31: the caller passes an all-zero table */
void Battlefield_Lay_River(register int *rivers)
{
    int x;
    int y;
    int nx;
    int ny;
    int alternate;
    int ok;
    int dir;

    alternate = 0;
    if (rivers[4] != 1)
        return;
    if (rivers[5] == 1 || rivers[2] == 1 || rivers[3] == 1 || rivers[6] == 1) {
        x = 7;
        y = 20;
        while (x < 20 && y > 0) {
            if (Random(2) != 1) {
                if (alternate == 0)
                    dir = 3;
                else
                    dir = 2;
            } else
                dir = Random(4) - 1;
            alternate = 1 - alternate;
            nx = x + battlefield_dir_dx[dir];
            ny = y + battlefield_dir_dy[dir];
            ok = 1;
            if (dir == 1)
                ok = 0;
            if (dir == 0)
                ok = 0;
            if (nx >= 0 && nx < 21 && ny >= 0 && ok == 1 && ny < 22) {
                x = nx;
                y = ny;
                p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_RIVER;
            }
        }
        return;
    }
    if (rivers[0] == 1 || rivers[1] == 1 || rivers[7] == 1 || rivers[8] == 1) {
        x = Random(8) + 4;
        y = 20;
        while (x < 20 && y < 21) {
            if (Random(2) != 1) {
                if (alternate == 0)
                    dir = 3;
                else
                    dir = 0;
            } else
                dir = Random(4) - 1;
            alternate = 1 - alternate;
            nx = x + battlefield_dir_dx[dir];
            ny = y + battlefield_dir_dy[dir];
            ok = 1;
            if (dir == 1)
                ok = 0;
            if (dir == 2)
                ok = 0;
            if (nx >= 0 && nx < 21 && ny >= 0 && ok == 1 && ny < 22) {
                x = nx;
                y = ny;
                p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_RIVER;
            }
        }
    }
}

/* 10 passes: a plain square (type 0) between two variant squares (type 2) left / right or above / below becomes
   variant */
void Battlefield_Fill_Variant_Gaps(void)
{
    int pass;
    int x;
    int y;

    for (pass = 0; pass < 10; pass++)
        for (y = 0; y < BATTLEFIELD_H; y++)
            for (x = 0; x < BATTLEFIELD_W; x++) {
                if (p_battlefield->terrain_type[y][x] != BATTLE_TERRAIN_PLAIN)
                    continue;
                if (p_battlefield->terrain_type[y][x + 1] == BATTLE_TERRAIN_VARIANT && p_battlefield->terrain_type[y][x - 1] == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_VARIANT;
                if (p_battlefield->terrain_type[y - 1][x] == BATTLE_TERRAIN_VARIANT &&
                    p_battlefield->terrain_type[y + 1][x] == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_VARIANT;
            }
}

/* (site) 8 passes: a rough square (type 1) next to a variant square (any of 8) or under a road becomes plain; for any
   site but open ground (BF_SITE_OPEN) also the rough squares of the city block (rows 10..13, columns 5..8) */
void Battlefield_Clear_Rough(int site)
{
    int pass;
    int x;
    int y;

    for (pass = 0; pass < 8; pass++)
        for (y = 0; y < BATTLEFIELD_H; y++)
            for (x = 0; x < BATTLEFIELD_W; x++) {
                if (p_battlefield->terrain_type[y][x] != BATTLE_TERRAIN_ROUGH)
                    continue;
                if (p_battlefield->terrain_type[y - 1][x - 1] == BATTLE_TERRAIN_VARIANT ||
                    p_battlefield->terrain_type[y - 1][x] == BATTLE_TERRAIN_VARIANT ||
                    p_battlefield->terrain_type[y - 1][x + 1] == BATTLE_TERRAIN_VARIANT ||
                    p_battlefield->terrain_type[y][x - 1] == BATTLE_TERRAIN_VARIANT || p_battlefield->terrain_type[y][x + 1] == BATTLE_TERRAIN_VARIANT ||
                    p_battlefield->terrain_type[y + 1][x - 1] == BATTLE_TERRAIN_VARIANT ||
                    p_battlefield->terrain_type[y + 1][x] == BATTLE_TERRAIN_VARIANT ||
                    p_battlefield->terrain_type[y + 1][x + 1] == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_PLAIN;
                if (p_battlefield->road[y][x] != 0)
                    p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_PLAIN;
            }
    if (site != BF_SITE_OPEN)
        for (y = 10; y <= 13; y++)
            for (x = 5; x <= 8; x++)
                if (p_battlefield->terrain_type[y][x] == BATTLE_TERRAIN_ROUGH)
                    p_battlefield->terrain_type[y][x] = BATTLE_TERRAIN_PLAIN;
}

/* (-) the tile pictures from the terrain types (auto-tiling against the 8 neighbours): plain squares by their
   variant neighbours, river squares by their river neighbours, variant (2), ocean (4, the animated water) and type 5
   squares (no BFGEN code writes 5) at random, rough squares (1) by their four rough neighbours */
void Battlefield_Pick_Tiles(int terrain)
{
    int n0;
    int n1;
    int n2;
    int n3;
    int terrain_kind;
    int n5;
    int n6;
    int n7;
    int n8;
    int code;
    int o_plain;
    int o_variant;
    int o_edge;
    int o_unused;
    int o_rough;
    register int x;
    register int y;

    o_plain = 0;
    o_variant = 0;
    o_edge = 0;
    o_unused = 0;
    o_rough = 0;
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++) {
            n0 = p_battlefield->terrain_type[y - 1][x - 1];
            n1 = p_battlefield->terrain_type[y - 1][x];
            n2 = p_battlefield->terrain_type[y - 1][x + 1];
            n3 = p_battlefield->terrain_type[y][x - 1];
            terrain_kind = p_battlefield->terrain_type[y][x];
            n5 = p_battlefield->terrain_type[y][x + 1];
            n6 = p_battlefield->terrain_type[y + 1][x - 1];
            n7 = p_battlefield->terrain_type[y + 1][x];
            n8 = p_battlefield->terrain_type[y + 1][x + 1];
            if (y == 0) {
                n0 = 0;
                n1 = 0;
                n2 = 0;
            }
            if (y == 21) {
                n6 = 0;
                n7 = 0;
                n8 = 0;
            }
            if (x == 0) {
                n0 = 0;
                n3 = 0;
                n6 = 0;
            }
            if (x == 20) {
                n2 = 0;
                n5 = 0;
                n8 = 0;
            }
            if (terrain_kind == BATTLE_TERRAIN_PLAIN) {
                if (n5 == BATTLE_TERRAIN_VARIANT) {
                    if (n1 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 27;
                    else if (n7 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 11;
                    else
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 29;
                } else if (n1 == BATTLE_TERRAIN_VARIANT) {
                    if (n3 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 21;
                    else
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 23;
                } else if (n3 == BATTLE_TERRAIN_VARIANT) {
                    if (n1 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 21;
                    else if (n7 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 15;
                    else
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 17;
                } else if (n7 == BATTLE_TERRAIN_VARIANT) {
                    if (n5 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 11;
                    else if (n3 == BATTLE_TERRAIN_VARIANT)
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 15;
                    else
                        p_battlefield->tile[y][x] = Random(2) + o_edge + 9;
                } else if (n8 == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->tile[y][x] = Random(2) + o_edge + 7;
                else if (n2 == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->tile[y][x] = Random(2) + o_edge + 25;
                else if (n0 == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->tile[y][x] = Random(2) + o_edge + 19;
                else if (n6 == BATTLE_TERRAIN_VARIANT)
                    p_battlefield->tile[y][x] = Random(2) + o_edge + 13;
                else
                    p_battlefield->tile[y][x] = Random(4) + o_plain - 1;
            }
            if (p_battlefield->terrain_type[y][x] == BATTLE_TERRAIN_RIVER) {
                if (y == 0) {
                    n0 = 3;
                    n1 = 3;
                    n2 = 3;
                }
                if (y == 21) {
                    n6 = 3;
                    n7 = 3;
                    n8 = 3;
                }
                if (x == 0) {
                    n0 = 3;
                    n3 = 3;
                    n6 = 3;
                }
                if (x == 20) {
                    n2 = 3;
                    n5 = 3;
                    n8 = 3;
                }
                if (n3 == BATTLE_TERRAIN_RIVER && n5 == BATTLE_TERRAIN_RIVER)
                    p_battlefield->tile[y][x] = (Random(2) - 1) * 6 + 56;
                if (n1 == BATTLE_TERRAIN_RIVER && n7 == BATTLE_TERRAIN_RIVER)
                    p_battlefield->tile[y][x] = (Random(2) - 1) * 6 + 57;
                if (n1 == BATTLE_TERRAIN_RIVER && n3 == BATTLE_TERRAIN_RIVER)
                    p_battlefield->tile[y][x] = (Random(2) - 1) * 6 + 58;
                if (n5 == BATTLE_TERRAIN_RIVER && n1 == BATTLE_TERRAIN_RIVER)
                    p_battlefield->tile[y][x] = (Random(2) - 1) * 6 + 59;
                if (n7 == BATTLE_TERRAIN_RIVER && n5 == BATTLE_TERRAIN_RIVER)
                    p_battlefield->tile[y][x] = (Random(2) - 1) * 6 + 60;
                if (n3 == BATTLE_TERRAIN_RIVER && n7 == BATTLE_TERRAIN_RIVER)
                    p_battlefield->tile[y][x] = (Random(2) - 1) * 6 + 61;
                if (y == 0) {
                    n0 = 0;
                    n1 = 0;
                    n2 = 0;
                }
                if (y == 21) {
                    n6 = 0;
                    n7 = 0;
                    n8 = 0;
                }
                if (x == 0) {
                    n0 = 0;
                    n3 = 0;
                    n6 = 0;
                }
                if (x == 20) {
                    n2 = 0;
                    n5 = 0;
                    n8 = 0;
                }
            }
            if (terrain_kind == BATTLE_TERRAIN_VARIANT)
                p_battlefield->tile[y][x] = Random(4) + o_variant + 3;
            if (terrain_kind == BATTLE_TERRAIN_OCEAN)
                p_battlefield->tile[y][x] = Random(4) + 47;
            if (terrain_kind == 5)
                p_battlefield->tile[y][x] = Random(4) + 51;
            if (terrain_kind == BATTLE_TERRAIN_ROUGH) {
                if (n1 == BATTLE_TERRAIN_ROUGH) {
                    if (n5 == BATTLE_TERRAIN_ROUGH) {
                        if (n7 == BATTLE_TERRAIN_ROUGH) {
                            if (n3 == BATTLE_TERRAIN_ROUGH)
                                code = 4;
                            else
                                code = 10;
                        } else if (n3 == BATTLE_TERRAIN_ROUGH)
                            code = 13;
                        else
                            code = 12;
                    } else if (n7 == BATTLE_TERRAIN_ROUGH) {
                        if (n3 == BATTLE_TERRAIN_ROUGH)
                            code = 11;
                        else
                            code = 1;
                    } else if (n3 == BATTLE_TERRAIN_ROUGH)
                        code = 14;
                    else
                        code = 5;
                } else if (n5 == BATTLE_TERRAIN_ROUGH) {
                    if (n7 == BATTLE_TERRAIN_ROUGH) {
                        if (n3 == BATTLE_TERRAIN_ROUGH)
                            code = 8;
                        else
                            code = 7;
                    } else if (n3 == BATTLE_TERRAIN_ROUGH)
                        code = 3;
                    else
                        code = 2;
                } else if (n7 == BATTLE_TERRAIN_ROUGH) {
                    if (n3 == BATTLE_TERRAIN_ROUGH)
                        code = 9;
                    else
                        code = 0;
                } else if (n3 == BATTLE_TERRAIN_ROUGH)
                    code = 6;
                else
                    code = 15;
                p_battlefield->tile[y][x] = code + o_rough + 32;
            }
        }
}

/* (-, -) the four movement cost layers (ground, flying, mode 3, sailing) from the terrain types and the roads; the
   four corners outside the isometric diamond are impassable, and so is the centre square (6, 11) of a backdrop other than the
   open field and a city
   Numbered: BUG #251, #255, #302 (BUGS_AND_QUIRKS.txt) */
void Battlefield_Build_Move_Costs(int site, int walls)
{
    int y;
    int x;
    int t;

    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++) {
            t = p_battlefield->terrain_type[y][x];
            switch (t) {
            case BATTLE_TERRAIN_PLAIN:
                p_battlefield->cost_ground[y][x] = 2;
                p_battlefield->cost_flying[y][x] = 2;
                p_battlefield->cost_mode3[y][x] = 2;
                p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
                break;
            case BATTLE_TERRAIN_ROUGH:
                p_battlefield->cost_ground[y][x] = 4;
                p_battlefield->cost_flying[y][x] = 2;
                p_battlefield->cost_mode3[y][x] = 4;
                p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
                break;
            case BATTLE_TERRAIN_VARIANT:
                p_battlefield->cost_ground[y][x] = 2;
                p_battlefield->cost_flying[y][x] = 2;
                p_battlefield->cost_mode3[y][x] = 2;
                p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
                break;
            case BATTLE_TERRAIN_RIVER:
                p_battlefield->cost_ground[y][x] = 4;
                p_battlefield->cost_flying[y][x] = 2;
                p_battlefield->cost_mode3[y][x] = 4;
                p_battlefield->cost_sailing[y][x] = 2;
                break;
            case BATTLE_TERRAIN_OCEAN:
                p_battlefield->cost_ground[y][x] = COST_IMPASSABLE;
                p_battlefield->cost_flying[y][x] = 2;
                p_battlefield->cost_mode3[y][x] = COST_IMPASSABLE;
                p_battlefield->cost_sailing[y][x] = 2;
                break;
            }
        }
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++) {
            t = p_battlefield->road[y][x];
            if (t != 0 && t != BF_ROAD_ISLAND_PIC) {   /* roads and island squares walkable, not sailable;
                                                          the island picture's square keeps the ocean costs */
                p_battlefield->cost_ground[y][x] = 1;
                p_battlefield->cost_flying[y][x] = 2;
                p_battlefield->cost_mode3[y][x] = 1;
                p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
            }
        }
    for (y = 0; y < 11; y++)
        for (x = 0; x < 11 - y; x++) {
            p_battlefield->cost_ground[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_flying[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_mode3[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
        }
    for (y = 10; y < BATTLEFIELD_H; y++)
        for (x = 0; x < y - 9; x++) {
            p_battlefield->cost_ground[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_flying[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_mode3[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
        }
    for (y = 0; y < 12; y++)
        for (x = y + 10; x < BATTLEFIELD_W; x++) {
            p_battlefield->cost_ground[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_flying[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_mode3[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
        }
    for (y = 10; y < BATTLEFIELD_H; y++)
        for (x = 32 - y; x < BATTLEFIELD_W; x++) {
            p_battlefield->cost_ground[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_flying[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_mode3[y][x] = COST_IMPASSABLE;
            p_battlefield->cost_sailing[y][x] = COST_IMPASSABLE;
        }
    if (p_battlefield->backdrop != BACKDROP_FIELD && p_battlefield->backdrop != BACKDROP_CITY) {
        p_battlefield->cost_ground[BF_FEATURE_Y][BF_FEATURE_X] = COST_IMPASSABLE;
        p_battlefield->cost_flying[BF_FEATURE_Y][BF_FEATURE_X] = COST_IMPASSABLE;
        p_battlefield->cost_mode3[BF_FEATURE_Y][BF_FEATURE_X] = COST_IMPASSABLE;
        p_battlefield->cost_sailing[BF_FEATURE_Y][BF_FEATURE_X] = COST_IMPASSABLE;
    }
}

/* (terrain, count, len_rand, len_min) count random walks of len_min + Random(len_rand) steps writing the terrain type */
void Battlefield_Random_Patches(int terrain, int count, int len_rand, int len_min)
{
    int patch;
    int rx;
    int ry;
    int x;
    int y;
    int dir;
    int unused;
    int len;
    int step;
    register int nx;
    register int ny;

    for (patch = 0; patch < count; patch++) {
        rx = Random(21) - 1;
        ry = Random(22) - 1;
        x = battlefield_dir_dx[Random(4) - 1] + rx;
        y = battlefield_dir_dy[Random(4) - 1] + ry;
        unused = -1;
        len = Random(len_rand) + len_min;
        for (step = 0; step < len; step++) {
            dir = Random(4) - 1;
            nx = x + battlefield_dir_dx[dir];
            ny = y + battlefield_dir_dy[dir];
            if (nx >= 21)
                nx = 20;
            if (ny >= 22)
                ny = 21;
            if (nx < 0)
                nx = 0;
            if (ny < 0)
                ny = 0;
            x = nx;
            y = ny;
            p_battlefield->terrain_type[ny][nx] = terrain;
        }
    }
}

/* (gx, gy, ox, oy, &sx, &sy) the isometric projection of a grid square (with a sub-square offset) to the screen */
void Combat_Grid_To_Screen(int gx, int gy, int ox, int oy, int *sx, int *sy)
{
    int x;
    int y;

    x = (gx - gy) * 16 + 158;                   /* half a tile: 16 x 8 pixels; grid (0, 0) at (158, -80) */
    y = (gx + gy) * 8 - 80;
    x += (ox - oy) * 2;
    y += ox + oy;
    *sx = x;
    *sy = y;
}

/* (x, y) Earth to Mud: the plain and variant squares of the 5 x 5 block become mud (ground, mode 3 and sailing
   cost 12)
   Numbered: BUG #199 (BUGS_AND_QUIRKS.txt) */
void Earth_To_Mud_Square(register int x, int y)
{
    register int dx;
    register int dy;

    for (dy = -2; dy < 3; dy++)
        for (dx = -2; dx < 3; dx++)
            if (p_battlefield->terrain_type[y + dy][x + dx] == BATTLE_TERRAIN_PLAIN ||
                p_battlefield->terrain_type[y + dy][x + dx] == BATTLE_TERRAIN_VARIANT) {
                p_battlefield->mud[y + dy][x + dx] = 1;
                p_battlefield->cost_ground[y + dy][x + dx] = 12;
                p_battlefield->cost_mode3[y + dy][x + dx] = 12;
                p_battlefield->cost_sailing[y + dy][x + dx] = 12;
            }
}

/* (sx, sy) the grid column of a screen point */
int Combat_Screen_To_Grid_X(int sx, int sy)
{
    int grid_x;

    grid_x = ((sx - 158) / 2 + sy + 80) / 16;
    return grid_x;
}

/* (sx, sy) the grid row of a screen point */
int Combat_Screen_To_Grid_Y(int sx, int sy)
{
    int grid_y;

    grid_y = (sy + 80 - (sx - 158) / 2) / 16;
    return grid_y;
}

/* (sx, sy, &cell, &sub) the grid column of a screen point and the position inside it (0..15) */
void Combat_Screen_To_Grid_X_Sub(int sx, int sy, int *cell, int *sub)
{
    int fine_x;

    fine_x = (sx - 158) / 2 + sy + 80;
    *cell = fine_x / 16;
    *sub = fine_x % 16;
}

/* (sx, sy, &cell, &sub) the grid row of a screen point and the position inside it (0..15) */
void Combat_Screen_To_Grid_Y_Sub(int sx, int sy, int *cell, int *sub)
{
    int fine_y;

    fine_y = sy + 80 - (sx - 158) / 2;
    *cell = fine_y / 16;
    *sub = fine_y % 16;
}

/* reads the world block back from COMBAT.TMP (in two parts - 0x7ff paragraphs and the rest - only above 0x800
   paragraphs: a block of exactly 0x800 is read whole) */
void Combat_Tmp_Restore(void)
{
    int rest;
    int first;
    int handle;

    first = world_block_paras;
    if (first > 0x800) {
        first = 0x7ff;
        rest = world_block_paras - 0x7ff;
    } else
        rest = 0;
    handle = DOS_File_Open("COMBAT.TMP", "rb");
    DOS_File_Read_Far(0, p_world_block, first << 4, 1, handle);
    DOS_File_Read_Far(first << 4, p_world_block, rest << 4, 1, handle);
    DOS_File_Close(handle);
}

/* writes the world block to COMBAT.TMP (in two parts - 0x7ff paragraphs and the rest - only above 0x800
   paragraphs: a block of exactly 0x800 is written whole). BUG #334: the open is not checked */
void Combat_Tmp_Save(void)
{
    int rest;
    int first;
    int handle;

    first = world_block_paras;
    if (first > 0x800) {
        first = 0x7ff;
        rest = world_block_paras - 0x7ff;
    } else
        rest = 0;
    handle = DOS_File_Open("COMBAT.TMP", "wb");
    DOS_File_Write_Far(0, p_world_block, first << 4, 1, handle);
    DOS_File_Write_Far(first << 4, p_world_block, rest << 4, 1, handle);
    DOS_File_Close(handle);
}
