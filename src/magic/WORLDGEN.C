/* WORLDGEN.C - MAGIC.EXE code segment 4358 (MAGIC only): building the two worlds for a new game - landmasses,
   heights, terrain and climate, nodes, towers, lairs, the wizards' fortresses and the neutral cities, minerals, roads,
   shorelines, rivers, terrain variants, movement maps, the region bits, the first exploration, the starting units.
   Its data block is DS:3334 .. 34c3 (after NEWGAME.C's).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 4358 src/magic/WORLDGEN.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     Mem_Copy_Far (3, with far pointers; UTIL.C takes (offset, segment) word pairs, 5 words). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

#define NODES_ARCANUS 16                /* nodes 0 .. 15 on Arcanus, 16 .. 29 on Myrror */
#define LAIR_SLOT_NODES 12              /* p_lairs: the nodes' lairs from here (towers 0 .. 11 before) */
#define LAIR_SLOT_NORMAL 42             /* the normal lairs from here (after the 30 node lairs) */

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
void City_Pick_Default_Name(int race, char far *name);
void World_Build(void);
void Lairs_Record_Start_Guards(void);
void Gen_Islets_Around_Sites(int plane);
void Place_Towers(void);
void Place_Fortresses(void);
void Landmass_Clear(int plane);
void Balance_Node_Realms(int plane);
void Gen_Climate(int plane);
void Gen_Height_To_Terrain(int plane);
void Gen_Landmasses(int plane);
void Place_Nodes(void);
void Node_Make_Aura(int power, char far *aura_x, char far *aura_y, int x, int y);
int Node_Site_Reject(int node);
void Node_Pick_Realm(int power, char far *aura_x, char far *aura_y, int plane, char far *realm);
void Landmass_Label(int plane, int x, int y);
void Place_Lairs(void);
#define LAIR_CREATE_TOWER 0          /* Lair_Create's kind: a tower, */
#define LAIR_CREATE_NODE 1           /* a node (1 + its NodeRealm: */
#define LAIR_CREATE_SORCERY_NODE 1   /*   sorcery, */
#define LAIR_CREATE_NATURE_NODE 2    /*   nature, */
#define LAIR_CREATE_CHAOS_NODE 3     /*   chaos), */
#define LAIR_CREATE_RANDOM 4       /* or a random lair type (cave .. fallen temple) */
#define TILE_RIVER_MARKER 1000     /* Gen_River's value for a river square until Gen_River_Pictures draws it */
void Lair_Create(int lair, int plane, int x, int y, int unused_strength, int site_kind, int budget);
int Pick_Creature_Of_Realm(int race_code, int max_upkeep);
void Gen_Desert_Edges(void);
void Gen_Neighbour_Variants(void);
void Gen_Terrain_Variety(void);
int Gen_River(int plane);
void Gen_River_Pictures(int plane);
int Place_Neutral_Cities(int plane);
void Gen_Roads(int plane);
void Place_Minerals(int plane);
int Mineral_Pick_Desert(int plane);
void Build_Region_Bits(int plane);
int Mineral_Pick_Hills(int plane);
int Mineral_Pick_Mountain(int plane);
void Explore_Start(void);
void Explore_Mark(int plane, int x, int y, int bits);
void Build_Movement_Maps(int plane);
int Tile_Is_Shore_High(int x, int y, int plane);
int Progress_Bar(int percent);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int city);
void Gen_Ocean_Variety(void);
int Tile_Is_Water_Gen(int x, int y, int plane);
int Is_Node_At(int x, int y, int plane);
int Tower_At(int x, int y);
/* the generator's own tile tests (_Gen): other tile sets than WIZARDS' TERRAIN.C functions of the base name */
int Tile_Is_River_Gen(int x, int y, int plane);
int Tile_Is_Shore_Variant(int x, int y, int plane);
int Tile_Is_Shore_Any(int x, int y, int plane);
int Tile_Is_Mountain_Gen(int x, int y, int plane);
int Tile_Is_Hills_Gen(int x, int y, int plane);
int Tile_Is_Swamp_Gen(int x, int y, int plane);
int Tile_Is_Desert_Range(int x, int y, int plane);
int Tile_Is_Grassland_Gen(int x, int y, int plane);
int Tile_Food_Value(int x, int y, int plane);
int Site_Food_Value(int x, int y, int plane);
int City_Catchment_List(int x, int y, int plane, int *xs, int *ys);
int Tile_Is_Forest(int x, int y, int plane);
void Gen_Uncalled_Nop(void);
#pragma warn -pro

extern struct UnitType unit_types[];            /* DS:020c */
extern int exp_level_thresholds[9];             /* DS:1de4 */
extern struct RaceInfo race_table[14];           /* DS:1df6 */
extern unsigned char (far *p_terrain_stats)[6];  /* DS:5bac */
extern struct Building far *p_building_data;         /* DS:6712 */
extern struct Unit far *p_units;                 /* DS:68f8 */
extern struct Wizard wizards[6];                /* DS:6900 */
extern int num_units;                           /* DS:87c8 */
extern int num_cities;                          /* DS:87ca */
extern int difficulty;                          /* DS:87cc */
extern int magic_setting;                       /* DS:87ce */
extern int land_size;                           /* DS:87d0 */
extern int num_players;                         /* DS:87d2 */
extern unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH]; /* DS:89c2 */
extern unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89c6 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89ca */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89ce */
extern struct City far *p_cities;               /* DS:89d2 */
extern struct Lair far *p_lairs;                /* DS:89d6 */
extern struct Tower far *p_towers;              /* DS:89da */
extern struct Fortress far *p_fortresses;       /* DS:89de */
extern struct Node far *p_nodes;                /* DS:89e2 */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89e6 */
extern unsigned char (far *p_region_bits_water)[REGION_ROWS][REGION_COLS]; /* DS:89ea */
extern unsigned char (far *p_region_bits_land)[REGION_ROWS][REGION_COLS]; /* DS:89ee */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89f2 */
extern unsigned worldgen_box_pic;               /* DS:8dd6 */
extern int ng_chosen_race;                      /* DS:8ea4 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int landmass_next_id_spare[5];                  /* DS:9042  no code in either program reads it */
int landmass_next_id;                           /* DS:9040 */
int landmass_races[200];                        /* DS:8eb0  the neutral towns' race, pre-rolled per landmass id */

void far *Seg_To_Far_Ptr(unsigned seg);

/* walk_dx below and BFGEN.C's battlefield_dir_dx hold the same values (separate copies in the data) */
int step_dx[5] = { 0, -1, 0, 1, 0 };    /* DS:3334  the 4 directions S, W, N, E (+ stay) */
int step_dy[5] = { 1, 0, -1, 0, 0 };    /* DS:333e */
int step_opposite[4] = { 2, 3, 0, 1 };    /* DS:3348 */
int walk_dx[5] = { 0, -1, 0, 1, 0 };    /* DS:3350  the same values as step_dx: a second table, indexed by walk number */
int walk_dy[5] = { 1, 0, -1, 0, 0 };    /* DS:335a */
struct MouseRegion mouse_region_busy = { 6, 0, 0, 0, 319, 199 };    /* DS:3364 */

/* () a new game's worlds: BUILDDAT, the progress box, the AI wizards and spell picks (Wizards_Setup), then both
   planes step by step (rivers: 10 per plane, each tried up to 2000 times), the exploration and the lairs */
void World_Build(void)
{
    int i;
    int tries;

    p_building_data = Seg_To_Far_Ptr(LBX_Load_Records("BUILDDAT", 0, 0, NUM_BUILDINGS, sizeof(struct Building)));
    Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
    Screen_Copy_Front();
    Set_Draw_Page();
    Draw_Pic_Frame(75, 67, worldgen_box_pic);
    Font_Select(3, 3);
    Print_Text(88, 76, "Building The Worlds...");
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Present_Frame();
    Screen_Copy_Front();
    Wizards_Setup();
    Progress_Bar(5);
    Landmass_Clear(0);
    Progress_Bar(10);
    Landmass_Clear(1);
    Progress_Bar(15);
    Gen_Landmasses(0);
    Progress_Bar(20);
    Gen_Landmasses(1);
    Progress_Bar(25);
    Gen_Height_To_Terrain(0);
    Progress_Bar(30);
    Gen_Height_To_Terrain(1);
    Progress_Bar(35);
    Gen_Climate(0);
    Progress_Bar(40);
    Gen_Climate(1);
    Progress_Bar(45);
    Place_Nodes();
    Progress_Bar(50);
    Balance_Node_Realms(0);
    Balance_Node_Realms(1);
    Progress_Bar(55);
    Place_Towers();
    Progress_Bar(60);
    Gen_Islets_Around_Sites(0);
    Gen_Islets_Around_Sites(1);
    Place_Lairs();
    Progress_Bar(65);
    num_units = 0;
    Place_Fortresses();
    Progress_Bar(70);
    Place_Neutral_Cities(0);
    Place_Neutral_Cities(1);
    Progress_Bar(75);
    Place_Minerals(0);
    Place_Minerals(1);
    Gen_Roads(0);
    Gen_Roads(1);
    Progress_Bar(80);
    Gen_Neighbour_Variants();
    Progress_Bar(85);
    for (i = 0; i < 10; i++) {
        for (tries = 0; Gen_River(0) == 0 && tries < 2000; tries++)
            ;
        for (tries = 0; Gen_River(1) == 0 && tries < 2000; tries++)
            ;
    }
    Gen_River_Pictures(0);
    Gen_River_Pictures(1);
    Gen_Desert_Edges();
    Progress_Bar(85);
    Gen_Terrain_Variety();
    Build_Movement_Maps(0);
    Build_Movement_Maps(1);
    Progress_Bar(90);
    Build_Region_Bits(0);
    Build_Region_Bits(1);
    Progress_Bar(95);
    Explore_Start();
    Gen_Ocean_Variety();
    Lairs_Record_Start_Guards();
}

char *lair_site_names[11] = {    /* DS:3370  the site names, no reader in either program */
    "tower", "chaos node", "nature node", "sorcery node", "mysterious cave", "dungeon", "ancient temple",
    "abandoned keep", "monster lair", "ancient ruins", "fallen temple"
};
int river_variants[16][4] = {    /* DS:3386  river terrain by neighbour mask, 4 variants; row 0 (no water / river near): TILE_GRASSLAND */
    { 162, 162, 162, 162 },
    { 187, 187, 187, 187 },
    { 188, 188, 188, 188 },
    { 189, 193, 233, 189 },
    { 185, 185, 185, 185 },
    { 237, 238, 239, 237 },
    { 191, 195, 235, 191 },
    { 251, 252, 253, 254 },
    { 186, 186, 186, 186 },
    { 192, 196, 236, 192 },
    { 240, 241, 242, 240 },
    { 243, 244, 245, 246 },
    { 190, 194, 234, 190 },
    { 255, 256, 257, 258 },
    { 247, 248, 249, 250 },
    { 468, 469, 470, 471 }
};

/* () every lair's guard counts get their high nibble copied from the low one (count + count * 16) */
void Lairs_Record_Start_Guards(void)
{
    int lair;

    for (lair = 0; lair < NUM_LAIRS; lair++) {
        p_lairs[lair].guard1_count = p_lairs[lair].guard1_count + (p_lairs[lair].guard1_count << 4);
        p_lairs[lair].guard2_count = p_lairs[lair].guard2_count + (p_lairs[lair].guard2_count << 4);
    }
}

/* (plane) a lone node square (TILE_SORCERY / NATURE / CHAOS_NODE) or tower square surrounded by water gets,
   2 times in 3, 2 .. 9 tries at a land square around it (not on a tower or node). The case table (1 NW .. 9 SE, a
   diagonal only when touching land) wants dir = (dx + 1) + (dy + 1) * 3 + 1; BUG #360: the code computes dx + dy * 3 + 1,
   so only the east and south squares are placed, the south-west one with case 3's test, the others never
   Numbered: BUG #360 (BUGS_AND_QUIRKS.txt) */
void Gen_Islets_Around_Sites(int plane)
{
    int terrain;
    int i;
    int on_tower;
    int count;
    int dx;
    int dy;
    int found;
    int dir;
    register int x;
    register int y;

    for (y = 1; y < MAP_HEIGHT - 1; y++) {
        for (x = 1; x < MAP_WIDTH - 1; x++) {
            terrain = p_terrain_maps[plane][y][x];
            on_tower = 0;
            for (i = 0; i < NUM_TOWERS; i++) {
                if (p_towers[i].y == y && p_towers[i].x == x)
                    on_tower = 1;
            }
            if ((terrain == TILE_SORCERY_NODE || terrain == TILE_NATURE_NODE || terrain == TILE_CHAOS_NODE || on_tower == 1) &&
                    Tile_Is_Water_Gen(x, y - 1, plane) == 1 && Tile_Is_Water_Gen(x - 1, y, plane) == 1 &&
                    Tile_Is_Water_Gen(x + 1, y, plane) == 1 && Tile_Is_Water_Gen(x, y + 1, plane) == 1 &&
                    Random(3) > 1) {
                count = Random(8) + 1;
                for (i = 0; i < count; i++) {
                    dx = Random(3) - 2;
                    dy = Random(3) - 2;
                    if ((dx != 0 || dy != 0) && Tower_At(x + dx, y + dy) == 0 &&
                            Is_Node_At(x + dx, y + dy, plane) == 0) {
                        dir = dx + dy * 3 + 1;
                        found = 0;
                        switch (dir) {
                        case 1:
                            if (p_terrain_maps[plane][y + dy + 1][x + dx] != TILE_OCEAN ||
                                    p_terrain_maps[plane][y + dy][x + dx + 1] != TILE_OCEAN)
                                found = 1;
                            else
                                found = 0;
                            break;
                        case 2:
                            found = 1;
                            break;
                        case 3:
                            if (p_terrain_maps[plane][y + dy + 1][x + dx] != TILE_OCEAN ||
                                    p_terrain_maps[plane][y + dy][x + dx - 1] != TILE_OCEAN)
                                found = 1;
                            else
                                found = 0;
                            break;
                        case 4:
                            found = 1;
                            break;
                        case 5:
                            found = 0;
                            break;
                        case 6:
                            found = 1;
                            break;
                        case 7:
                            if (p_terrain_maps[plane][y + dy - 1][x + dx] != TILE_OCEAN ||
                                    p_terrain_maps[plane][y + dy][x + dx + 1] != TILE_OCEAN)
                                found = 1;
                            else
                                found = 0;
                            break;
                        case 8:
                            found = 1;
                            break;
                        case 9:
                            if (p_terrain_maps[plane][y + dy - 1][x + dx] != TILE_OCEAN ||
                                    p_terrain_maps[plane][y + dy][x + dx - 1] != TILE_OCEAN)
                                found = 1;
                            else
                                found = 0;
                            break;
                        }
                        if (found == 1) {
                            Landmass_Label(plane, x + dx, y + dy);
                            p_terrain_maps[plane][y + dy][x + dx] = TILE_GRASSLAND;
                        }
                    }
                }
            }
        }
    }
}

/* () the 6 towers: random squares (3 .. 56, 3 .. 36) that are land on a plane (or 1 time in 40 not), min_dist
   (10, one less every 50 tries past 500) from the other towers and 4 from every node; both planes' squares become
   TILE_GRASSLAND and get their landmass. The try counter is never initialised
   Numbered: BUG #352 (BUGS_AND_QUIRKS.txt) */
void Place_Towers(void)
{
    int x;
    int y;
    int min_dist;
    int tries;
    register int i;
    register int tower;

    min_dist = 10;
    for (tower = 0; tower < NUM_TOWERS; tower++) {
retry:
        tries++;
        if (tries > 500) {
            tries = 450;
            min_dist--;
        }
        x = Random(54) + 2;
        y = Random(34) + 2;
        if (p_terrain_maps[0][y][x] == TILE_OCEAN && p_terrain_maps[1][y][x] == TILE_OCEAN && Random(40) > 1)
            goto retry;
        for (i = 0; i < tower; i++) {
            if (Map_Distance(x, y, p_towers[i].x, p_towers[i].y, MAP_WIDTH) < min_dist)
                goto retry;
        }
        for (i = 0; i < NUM_NODES; i++) {
            if (Map_Distance(x, y, p_nodes[i].x, p_nodes[i].y, MAP_WIDTH) < 4)
                goto retry;
        }
        p_towers[tower].x = x;
        p_towers[tower].y = y;
        p_towers[tower].owner = PLAYER_NONE;
        Landmass_Label(0, x, y);
        Landmass_Label(1, x, y);
        p_terrain_maps[0][y][x] = TILE_GRASSLAND;
        p_terrain_maps[1][y][x] = TILE_GRASSLAND;
    }
}

/* () a fortress for all six wizard slots (n 0..5, whatever num_players is), capitals and starting units only for
   the players in the game: a random land square (Myrran wizards on Myrror) at least min_fort from the
   other fortresses on its plane and min_other / 2 from nodes, towers and lairs (both limits relax each round of
   1000 tries), with food >= 8 - n / 3 (500 misses restart the round - the miss count is not reset, so
   after the 500th every miss restarts it). Capitals: the human's chosen race, an AI's
   by Random (Myrran races with the Myrran retort; 5 races above Normal, 9 at Normal and below) - a High Elf off forest (tested on the square the
   location loop left, BUG #359) sends the search back to a new site (goto retry, BUG: into the fortress loop) -; size 1, 4 pop, smithy, barracks,
   builders' hall, a default name. Starting units: one of each race's two basic types (Dwarves: two of the second) */
void Place_Fortresses(void)
{
    int n;
    int per_plane[2];
    int plane;
    int x;
    int y;
    int tries;
    int bad;
    int k;
    int min_fort;
    int min_other;
    int fails;
    register int j;
    register int unit_type;

    min_fort = 16;
    min_other = 8;
    do {
        min_fort--;
        min_other--;
        if (min_other < 1)
            min_other = 1;
        if (min_fort < 10)
            min_fort = 10;
        tries = 0;
        per_plane[0] = 0;
        per_plane[1] = 0;
        fails = 0;
restart:
        for (n = 0; n < 6 && tries < 1000; n++) {
retry:
            tries++;
            plane = 0;
            if (wizards[n].retorts[RETORT_MYRRAN] != 0)
                plane = 1;
            x = Random(54) + 2;
            y = Random(34) + 2;
            bad = 0;
            if (p_terrain_maps[plane][y][x] == TILE_OCEAN)
                bad = 1;
            for (j = 0; j < n; j++) {
                if (p_fortresses[j].plane == plane && Map_Distance(x, y, p_fortresses[j].x, p_fortresses[j].y, MAP_WIDTH) < min_fort)
                    bad = 1;
            }
            for (j = 0; j < NUM_NODES; j++) {
                if (p_nodes[j].plane == plane && Map_Distance(x, y, p_nodes[j].x, p_nodes[j].y, MAP_WIDTH) < min_other / 2)
                    bad = 1;
            }
            for (j = 0; j < NUM_TOWERS; j++) {
                if (Dist_Max_Plus_Half_Min(x, y, p_towers[j].x, p_towers[j].y) < min_other / 2)   /* towers: this metric;
                                                   nodes / lairs: Map_Distance */
                    bad = 1;
            }
            for (j = 0; j < NUM_LAIRS; j++) {
                if (p_lairs[j].plane == plane && Map_Distance(x, y, p_lairs[j].x, p_lairs[j].y, MAP_WIDTH) < min_other / 2)
                    bad = 1;
            }
            if (tries < 1000 && bad == 1)
                goto retry;
            if (Site_Food_Value(x, y, plane) < 8 - n / 3) {
                fails++;
                if (fails > 500)
                    goto restart;
                else
                    goto retry;
            }
            p_fortresses[n].x = x;
            p_fortresses[n].y = y;
            p_fortresses[n].plane = plane;
            p_fortresses[n].active = 1;
            per_plane[plane]++;
        }
    } while (tries >= 1000);
    num_cities = 0;
    for (k = 0; k < num_players; k++) {
        if (k == 0)
            p_cities[num_cities].race = ng_chosen_race;
        else {
            /* form not fixed by the bytes: the jump-table pad shows one of the two race tests was a goto (the bytes
               match either way) */
            if (wizards[k].retorts[RETORT_MYRRAN] == 0)
                goto not_myrran;
            switch (Random(5) - 1) {
            case 0:
                p_cities[num_cities].race = RACE_BEASTMEN;
                break;
            case 1:
                p_cities[num_cities].race = RACE_DARK_ELF;
                break;
            case 2:
                p_cities[num_cities].race = RACE_DRACONIAN;
                break;
            case 3:
                p_cities[num_cities].race = RACE_DWARVEN;
                break;
            case 4:
                p_cities[num_cities].race = RACE_TROLL;
                break;
            }
            goto picked;
not_myrran:
            if (difficulty > DIFFICULTY_NORMAL) {
                switch (Random(5) - 1) {
                case 0:
                    p_cities[num_cities].race = RACE_KLACKON;
                    break;
                case 1:
                    p_cities[num_cities].race = RACE_HIGH_ELF;
                    break;
                case 2:
                    p_cities[num_cities].race = RACE_HALFLING;
                    break;
                case 3:
                    p_cities[num_cities].race = RACE_HIGH_MEN;
                    break;
                case 4:
                    p_cities[num_cities].race = RACE_NOMAD;
                    break;
                }
            } else {
                switch (Random(9) - 1) {
                case 0:
                    p_cities[num_cities].race = RACE_BARBARIAN;
                    break;
                case 1:
                    p_cities[num_cities].race = RACE_GNOLL;
                    break;
                case 2:
                    p_cities[num_cities].race = RACE_HALFLING;
                    break;
                case 3:
                    p_cities[num_cities].race = RACE_HIGH_ELF;
                    break;
                case 4:
                    p_cities[num_cities].race = RACE_HIGH_MEN;
                    break;
                case 5:
                    p_cities[num_cities].race = RACE_KLACKON;
                    break;
                case 6:
                    p_cities[num_cities].race = RACE_LIZARDMAN;
                    break;
                case 7:
                    p_cities[num_cities].race = RACE_NOMAD;
                    break;
                case 8:
                    p_cities[num_cities].race = RACE_ORC;
                    break;
                }
            }
picked:
            if (p_cities[num_cities].race == RACE_HIGH_ELF && Tile_Is_Forest(x, y, plane) == 0)
                goto retry;
        }
        p_cities[num_cities].x = p_fortresses[k].x;
        p_cities[num_cities].y = p_fortresses[k].y;
        p_cities[num_cities].plane = p_fortresses[k].plane;
        p_cities[num_cities].owner = k;
        p_cities[num_cities].size = 1;
        p_cities[num_cities].population = 4;
        p_cities[num_cities].building_count = 0;
        p_cities[num_cities].production_progress = 0;
        p_cities[num_cities].population_tens = 0;
        if (k == 0)
            p_cities[num_cities].producing = PROD_HOUSING;
        else
            p_cities[num_cities].producing = PROD_AUTO_CHOOSE;
        p_cities[num_cities].enchants[CITY_ENCH_WALL_OF_FIRE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_WALL_OF_DARKNESS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CHAOS_RIFT] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_DARK_RITUALS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_EVIL_PRESENCE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CURSED_LANDS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_PESTILENCE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CLOUD_OF_SHADOW] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_ALTAR_OF_BATTLE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_FAMINE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_FLYING_FORTRESS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_DEATH_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CHAOS_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_LIFE_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_SORCERY_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_NATURE_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_NATURES_EYE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_EARTH_GATE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_STREAM_OF_LIFE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_GAIAS_BLESSING] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_INSPIRATIONS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_PROSPERITY] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_ASTRAL_GATE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_HEAVENLY_LIGHT] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CONSECRATION] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_NIGHTSHADE] = 0;
        p_cities[num_cities].farmers = 3;
        for (j = 0; j < NUM_BUILDINGS; j++)
            p_cities[num_cities].buildings[j] = BLDSTATE_NONE;
        p_cities[num_cities].buildings[BLD_NONE] = 0;
        p_cities[num_cities].buildings[BLD_SMITHY] = BLDSTATE_BUILT;
        p_cities[num_cities].buildings[BLD_BARRACKS] = BLDSTATE_BUILT;
        p_cities[num_cities].buildings[BLD_BUILDERS_HALL] = BLDSTATE_BUILT;
        City_Pick_Default_Name(p_cities[num_cities].race, p_cities[num_cities].name);
        num_cities++;
    }
    for (k = 0; k < num_players; k++) {
        switch (p_cities[k].race) {
        case RACE_BARBARIAN:
            unit_type = UNIT_BARBARIAN_SPEARMEN;
            break;
        case RACE_BEASTMEN:
            unit_type = UNIT_BEASTMEN_SPEARMEN;
            break;
        case RACE_DARK_ELF:
            unit_type = UNIT_DARK_ELF_SPEARMEN;
            break;
        case RACE_DRACONIAN:
            unit_type = UNIT_DRACONIAN_SPEARMEN;
            break;
        case RACE_DWARVEN:
            unit_type = UNIT_DWARF_SWORDSMEN;
            break;
        case RACE_GNOLL:
            unit_type = UNIT_GNOLL_SPEARMEN;
            break;
        case RACE_HALFLING:
            unit_type = UNIT_HALFLING_SPEARMEN;
            break;
        case RACE_HIGH_ELF:
            unit_type = UNIT_HIGH_ELF_SPEARMEN;
            break;
        case RACE_HIGH_MEN:
            unit_type = UNIT_HIGH_MEN_SPEARMEN;
            break;
        case RACE_KLACKON:
            unit_type = UNIT_KLACKON_SPEARMEN;
            break;
        case RACE_LIZARDMAN:
            unit_type = UNIT_LIZARDMAN_SPEARMEN;
            break;
        case RACE_NOMAD:
            unit_type = UNIT_NOMAD_SPEARMEN;
            break;
        case RACE_ORC:
            unit_type = UNIT_ORC_SPEARMEN;
            break;
        case RACE_TROLL:
            unit_type = UNIT_TROLL_SPEARMEN;
            break;
        }
        if (p_cities[k].race != RACE_DWARVEN)
            Unit_Create(unit_type, k, p_cities[k].x, p_cities[k].y, p_cities[k].plane, k);
    }
    for (k = 0; k < num_players; k++) {
        switch (p_cities[k].race) {
        case RACE_BARBARIAN:
            unit_type = UNIT_BARBARIAN_SWORDSMEN;
            break;
        case RACE_BEASTMEN:
            unit_type = UNIT_BEASTMEN_SWORDSMEN;
            break;
        case RACE_DARK_ELF:
            unit_type = UNIT_DARK_ELF_SWORDSMEN;
            break;
        case RACE_DRACONIAN:
            unit_type = UNIT_DRACONIAN_SWORDSMEN;
            break;
        case RACE_DWARVEN:
            unit_type = UNIT_DWARF_SWORDSMEN;
            break;
        case RACE_GNOLL:
            unit_type = UNIT_GNOLL_SWORDSMEN;
            break;
        case RACE_HALFLING:
            unit_type = UNIT_HALFLING_SWORDSMEN;
            break;
        case RACE_HIGH_ELF:
            unit_type = UNIT_HIGH_ELF_SWORDSMEN;
            break;
        case RACE_HIGH_MEN:
            unit_type = UNIT_HIGH_MEN_SWORDSMEN;
            break;
        case RACE_KLACKON:
            unit_type = UNIT_KLACKON_SWORDSMEN;
            break;
        case RACE_LIZARDMAN:
            unit_type = UNIT_LIZARDMAN_SWORDSMEN;
            break;
        case RACE_NOMAD:
            unit_type = UNIT_NOMAD_SWORDSMEN;
            break;
        case RACE_ORC:
            unit_type = UNIT_ORC_SWORDSMEN;
            break;
        case RACE_TROLL:
            unit_type = UNIT_TROLL_SWORDSMEN;
            break;
        }
        if (p_cities[k].race == RACE_DWARVEN)
            Unit_Create(unit_type, k, p_cities[k].x, p_cities[k].y, p_cities[k].plane, k);
        Unit_Create(unit_type, k, p_cities[k].x, p_cities[k].y, p_cities[k].plane, k);
    }
}

/* (plane) no landmasses yet: every square's id 0, the next id 1 */
void Landmass_Clear(int plane)
{
    int x;
    int y;

    landmass_next_id = 1;
    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++)
            p_landmass_ids[plane][y][x] = 0;
    }
}

/* (plane) too many Sorcery nodes: Chaos and Nature are topped up to 6 (Arcanus; Myrror 3) by turning random Sorcery
   nodes of the plane. BUG #358: the excess over 9 (4) only gates the loops, it is never counted down
   Numbered: BUG #358 (BUGS_AND_QUIRKS.txt) */
void Balance_Node_Realms(int plane)
{
    int chaos;
    int sorcery;
    int nature;
    int node;
    int to_chaos;
    int to_nature;
    int excess;
    int n;

    chaos = 0;
    sorcery = 0;
    nature = 0;
    for (node = 0; node < NUM_NODES; node++) {
        if (p_nodes[node].plane == plane) {
            switch (p_nodes[node].realm) {
            case NODE_SORCERY:
                sorcery++;
                break;
            case NODE_NATURE:
                nature++;
                break;
            case NODE_CHAOS:
                chaos++;
                break;
            }
        }
    }
    to_chaos = 0;
    to_nature = 0;
    if (plane == PLANE_ARCANUS) {
        if (sorcery > 9)
            excess = sorcery - 9;
        else
            excess = 0;
        while (chaos < 6 && excess > 0) {
            chaos++;
            to_chaos++;
        }
        while (nature < 6 && excess > 0) {
            nature++;
            to_nature++;
        }
    } else {
        if (sorcery > 4)
            excess = sorcery - 4;
        else
            excess = 0;
        while (chaos < 3 && excess > 0) {
            chaos++;
            to_chaos++;
        }
        while (nature < 3 && excess > 0) {
            nature++;
            to_nature++;
        }
    }
    while (to_chaos > 0) {
        n = Random(NUM_NODES) - 1;
        if (p_nodes[n].plane == plane && p_nodes[n].realm == NODE_SORCERY) {
            p_nodes[n].realm = NODE_CHAOS;
            p_terrain_maps[plane][p_nodes[n].y][p_nodes[n].x] = TILE_CHAOS_NODE;
            Landmass_Label(plane, p_nodes[n].x, p_nodes[n].y);
            to_chaos--;
        }
    }
    while (to_nature > 0) {
        n = Random(NUM_NODES) - 1;
        if (p_nodes[n].plane == plane && p_nodes[n].realm == NODE_SORCERY) {
            p_nodes[n].realm = NODE_NATURE;
            p_terrain_maps[plane][p_nodes[n].y][p_nodes[n].x] = TILE_NATURE_NODE;
            Landmass_Label(plane, p_nodes[n].x, p_nodes[n].y);
            to_nature--;
        }
    }
}

/* (plane) climate: TILE_GRASSLAND / TILE_FOREST in the 6 rows next to each pole turns TILE_TUNDRA more often
   nearer the pole; 8 deserts (TILE_DESERT) and 8 swamp patches (TILE_SWAMP; the "not on rows 35 .. 45" test is dead:
   the centres are rows 11 .. 30) as 5 short random walks from a
   centre (wrapping at the map edges) */
void Gen_Climate(int plane)
{
    int k;
    int center_x;
    int center_y;
    int patch;
    int walk_x;
    int walk_y;
    int dir;
    int step_dir;
    int len;
    int unused;
    register int y;
    register int x;

    for (y = 2; y < 8; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            if ((p_terrain_maps[plane][y][x] == TILE_GRASSLAND || p_terrain_maps[plane][y][x] == TILE_FOREST) && Random(8) + 2 >= y)
                p_terrain_maps[plane][y][x] = TILE_TUNDRA;
            if ((p_terrain_maps[plane][MAP_HEIGHT - 1 - y][x] == TILE_GRASSLAND || p_terrain_maps[plane][MAP_HEIGHT - 1 - y][x] == TILE_FOREST) &&
                    Random(8) + 2 >= y)
                p_terrain_maps[plane][MAP_HEIGHT - 1 - y][x] = TILE_TUNDRA;
        }
    }
    for (patch = 0; patch < 8; patch++) {
        center_x = Random(54) + 2;
        center_y = Random(24) + 8;
        if (p_terrain_maps[plane][center_y][center_x] == TILE_FOREST)
            p_terrain_maps[plane][center_y][center_x] = TILE_DESERT;
        for (dir = 0; dir < 5; dir++) {
            walk_x = walk_dx[dir] + center_x;
            walk_y = walk_dy[dir] + center_y;
            unused = -1;
            len = Random(6) + 4;
            for (k = 0; k < len; k++) {
                step_dir = Random(4) - 1;
                x = walk_x + walk_dx[step_dir];
                y = walk_y + walk_dy[step_dir];
                if (x >= MAP_WIDTH)
                    x = x - MAP_WIDTH;
                if (y >= MAP_HEIGHT)
                    y = y - MAP_HEIGHT;
                if (x < 0)
                    x = x + MAP_WIDTH;
                if (y < 0)
                    y = y + MAP_HEIGHT;
                walk_x = x;
                walk_y = y;
                if (p_terrain_maps[plane][walk_y][walk_x] != TILE_OCEAN)
                    p_terrain_maps[plane][walk_y][walk_x] = TILE_DESERT;
            }
        }
    }
    for (patch = 0; patch < 8; patch++) {
again:
        center_x = Random(56) + 1;
        center_y = Random(20) + 10;
        if (center_y >= 35 && center_y <= 45)  /* never true: center_y is 11 .. 30 */
            goto again;
        if (p_terrain_maps[plane][center_y][center_x] == TILE_FOREST)
            p_terrain_maps[plane][center_y][center_x] = TILE_SWAMP;
        for (dir = 0; dir < 5; dir++) {
            walk_x = walk_dx[dir] + center_x;
            walk_y = walk_dy[dir] + center_y;
            unused = -1;
            len = Random(3) + 2;
            for (k = 0; k < len; k++) {
                step_dir = Random(4) - 1;
                x = walk_x + walk_dx[step_dir];
                y = walk_y + walk_dy[step_dir];
                if (x >= MAP_WIDTH)
                    x = x - MAP_WIDTH;
                if (y >= MAP_HEIGHT)
                    y = y - MAP_HEIGHT;
                if (x < 0)
                    x = x + MAP_WIDTH;
                if (y < 0)
                    y = y + MAP_HEIGHT;
                walk_x = x;
                walk_y = y;
                if (p_terrain_maps[plane][walk_y][walk_x] == TILE_FOREST)
                    p_terrain_maps[plane][walk_y][walk_x] = TILE_SWAMP;
            }
        }
    }
}

/* (plane) heights to terrain: 1 grassland (1 in 4 forest), 2 .. 3 forest, 4 .. 5 hills, higher mountains; the polar
   rows tundra, with random tundra runs (1 .. 4 squares) on the next rows */
void Gen_Height_To_Terrain(int plane)
{
    int y;
    int k;
    int len;
    int x;

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            if (p_terrain_maps[plane][y][x] != TILE_OCEAN) {
                if (p_terrain_maps[plane][y][x] > 3) {
                    if (p_terrain_maps[plane][y][x] == 4 || p_terrain_maps[plane][y][x] == 5)
                        p_terrain_maps[plane][y][x] = TILE_HILLS;
                    else
                        p_terrain_maps[plane][y][x] = TILE_MOUNTAIN;
                } else {
                    if (p_terrain_maps[plane][y][x] > 1)
                        p_terrain_maps[plane][y][x] = TILE_FOREST;
                    else if (Random(4) == 1)
                        p_terrain_maps[plane][y][x] = TILE_FOREST;
                    else
                        p_terrain_maps[plane][y][x] = TILE_GRASSLAND;
                }
            }
        }
    }
    for (x = 0; x < MAP_WIDTH; x++) {
        p_terrain_maps[plane][0][x] = TILE_TUNDRA;
        p_terrain_maps[plane][MAP_HEIGHT - 1][x] = TILE_TUNDRA;
        if (Random(4) == 1) {
            len = Random(4);
            for (k = 0; k < len && x + k < MAP_WIDTH; k++)
                p_terrain_maps[plane][1][x + k] = TILE_TUNDRA;
        }
        if (Random(4) == 1) {
            len = Random(4);
            for (k = 0; k < len && x + k < MAP_WIDTH; k++)
                p_terrain_maps[plane][MAP_HEIGHT - 2][x + k] = TILE_TUNDRA;
        }
    }
}

/* (plane) landmasses as heights: until the land target (small 360, medium 480, large 720 squares) is met, a seed
   (the first 8 in different cells of a 5 x 5 grid) starts 3 .. 5 random walks (6 .. 15 / 11 .. 20 / 21 .. 30 steps
   by land size) that raise each square they cross; a walk repeats its last direction with falling odds and turns
   back from the map edge (x 2 .. 57, y 4 .. 35) */
void Gen_Landmasses(int plane)
{
    int land;
    int target;
    int x;
    int y;
    int nx;
    int ny;
    int len;
    int step;
    int dir;
    int last_dir;
    int straight;
    int seeds_left;
    int cell_w;
    int cell_h;
    int walks;
    int w;
    int sx;
    int sy;
    int grid[5][5];
    register int j;
    register int i;

    for (i = 0; i < MAP_HEIGHT; i++) {
        for (j = 0; j < MAP_WIDTH; j++)
            p_terrain_maps[plane][i][j] = TILE_OCEAN;
    }
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++)
            grid[i][j] = 0;
    }
    seeds_left = 8;
    cell_w = 16;
    cell_h = 11;
    land = 0;
    switch (land_size) {
    case 0:
        target = 360;
        break;
    case 1:
        target = 480;
        break;
    case 2:
        target = 720;
        break;
    }
    while (land <= target) {
        last_dir = -1;
        straight = 1;
pick:
        sx = Random(46) + 6;
        sy = Random(26) + 6;
        if (seeds_left > 0) {
            if (grid[sy / cell_h][sx / cell_w] == 1)
                goto pick;
            grid[sy / cell_h][sx / cell_w] = 1;
            seeds_left--;
        }
        walks = Random(3) + 2;
        for (w = 0; w < walks; w++) {
            switch (land_size) {
            case 0:
                len = Random(10) + 5;
                break;
            case 1:
                len = Random(10) + 10;
                break;
            case 2:
                len = Random(10) + 20;
                break;
            }
            x = sx + walk_dx[w];                /* the w-th walk starts a step S / W / N / E / here */
            y = sy + walk_dy[w];
            for (step = 0; step < len && land <= target; step++) {
                if (p_terrain_maps[plane][y][x] == TILE_OCEAN)
                    land++;
                p_terrain_maps[plane][y][x]++;
                Landmass_Label(plane, x, y);
turn:
                dir = Random(4) - 1;
                if (dir == last_dir) {
                    if (Random(straight * 2) != 1)
                        goto turn;
                    straight++;
                } else
                    straight = 1;
                last_dir = dir;
                nx = x + step_dx[dir];
                ny = y + step_dy[dir];
                if (nx < 2)
                    goto edge;
                if (ny < 4)
                    goto edge;
                if (nx >= 58)
                    goto edge;
                if (ny >= 36) {
edge:
                    last_dir = -1;
                    goto turn;
                }
                x = nx;
                y = ny;
            }
        }
    }
}

/* () the 30 nodes: 16 on Arcanus (power 5 .. 10), 14 on Myrror (power 10 .. 20), each in its cell of a 5-wide grid
   (+ up to 24 / 20 / 40 random), 3 .. 56 x 2 .. 36, on land (water 1 time in 40 / 25), 3+ squares from the other
   nodes (Myrror: of its plane); then the aura and the realm. BUG #413: the Myrror nodes test Arcanus' land */
void Place_Nodes(void)
{
    int ox;
    int oy;
    int x;
    int y;
    register int n;
    register int j;

    for (n = 0; n < NODES_ARCANUS; n++) {
retry_arcanus:
        ox = (n % 5) * 12;
        oy = (n / 5) * 10;
        x = Random(24) + ox - 1;
        y = Random(20) + oy - 1;
        if (x >= MAP_WIDTH)
            x = x - MAP_WIDTH;
        if (y >= MAP_HEIGHT)
            y = y - MAP_HEIGHT;
        if (x < 0)
            x = x + MAP_WIDTH;
        if (y < 0)
            y = y + MAP_HEIGHT;
        if (x < 3)
            goto retry_arcanus;
        if (y < 2)
            goto retry_arcanus;
        if (x >= 57)
            goto retry_arcanus;
        if (y >= 37)
            goto retry_arcanus;
        if (p_terrain_maps[0][y][x] == TILE_OCEAN && Random(40) != 1)
            goto retry_arcanus;
        for (j = 0; j < n; j++) {
            if (Map_Distance(x, y, p_nodes[j].x, p_nodes[j].y, MAP_WIDTH) < 3)
                goto retry_arcanus;
        }
        p_nodes[n].x = x;
        p_nodes[n].y = y;
        p_nodes[n].plane = PLANE_ARCANUS;
        p_nodes[n].meld_flags = 0;
        p_nodes[n].owner = PLAYER_NONE;
        p_nodes[n].power = Random(6) + 4;
        Node_Make_Aura(p_nodes[n].power, p_nodes[n].aura_x, p_nodes[n].aura_y, x, y);
        if (Node_Site_Reject(n) == 1)
            goto retry_arcanus;
        Node_Pick_Realm(p_nodes[n].power, p_nodes[n].aura_x, p_nodes[n].aura_y, p_nodes[n].plane, &p_nodes[n].realm);
    }
    for (n = NODES_ARCANUS; n < NUM_NODES; n++) {
retry_myrror:
        ox = ((n - 20) % 5) * 12;               /* n - 20, not n - 16: nodes 16 .. 19 get cells -4 .. -1 (wrapped below) */
        oy = ((n - 20) / 5) * 20;
        x = Random(24) + ox - 1;
        y = Random(40) + oy - 1;
        if (x >= MAP_WIDTH)
            x = x - MAP_WIDTH;
        if (y >= MAP_HEIGHT)
            y = y - MAP_HEIGHT;
        if (x < 0)
            x = x + MAP_WIDTH;
        if (y < 0)
            y = y + MAP_HEIGHT;
        if (x < 3)
            goto retry_myrror;
        if (y < 2)
            goto retry_myrror;
        if (x >= 57)
            goto retry_myrror;
        if (y >= 37)
            goto retry_myrror;
        if (p_terrain_maps[0][y][x] == TILE_OCEAN && Random(25) != 1)
            goto retry_myrror;
        for (j = 0; j < n; j++) {
            if (p_nodes[j].plane == PLANE_MYRROR && Map_Distance(x, y, p_nodes[j].x, p_nodes[j].y, MAP_WIDTH) < 3)
                goto retry_myrror;
        }
        p_nodes[n].x = x;
        p_nodes[n].y = y;
        p_nodes[n].plane = PLANE_MYRROR;
        p_nodes[n].meld_flags = 0;
        p_nodes[n].owner = PLAYER_NONE;
        p_nodes[n].power = Random(11) + 9;
        Node_Make_Aura(p_nodes[n].power, p_nodes[n].aura_x, p_nodes[n].aura_y, x, y);
        if (Node_Site_Reject(n) == 1)
            goto retry_myrror;
        Node_Pick_Realm(p_nodes[n].power, p_nodes[n].aura_x, p_nodes[n].aura_y, p_nodes[n].plane, &p_nodes[n].realm);
    }
}

/* (power, aura x / y, x, y) a node's aura: the node square, then up to 8 distinct squares a step from the node, the
   rest (power over 9) within two steps in each axis (Random(2): 1 or 2) (never off the map edge) */
void Node_Make_Aura(int power, char far *aura_x, char far *aura_y, int x, int y)
{
    int nx;
    int ny;
    int dir_x;
    int dir_y;
    int dup;
    int k;
    int j;

    aura_x[0] = x;
    aura_y[0] = y;
    for (k = 1; k < power && k < 9; k++) {
        do {
            dir_x = Random(4) - 1;
            dir_y = Random(4) - 1;
            nx = step_dx[dir_x] + x;
            ny = step_dy[dir_y] + y;
            dup = 0;
            for (j = 0; j < k; j++) {
                if (aura_x[j] == (char)nx && aura_y[j] == (char)ny)
                    dup = 1;
            }
            if (nx <= 0 || ny <= 0 || nx >= MAP_WIDTH - 1 || ny >= MAP_HEIGHT - 1)
                dup = 1;
        } while (dup == 1);
        aura_x[k] = nx;
        aura_y[k] = ny;
    }
    for (k = 9; k < power; k++) {
        do {
            dir_x = Random(4) - 1;
            dir_y = Random(4) - 1;
            nx = Random(2) * step_dx[dir_x] + x;
            ny = Random(2) * step_dy[dir_y] + y;
            dup = 0;
            for (j = 0; j < k; j++) {
                if (aura_x[j] == (char)nx && aura_y[j] == (char)ny)
                    dup = 1;
            }
            if (nx <= 0 || ny <= 0 || (char)nx >= MAP_WIDTH - 1 || (char)ny >= MAP_HEIGHT - 1)
                dup = 1;
        } while (dup == 1);
        aura_x[k] = nx;
        aura_y[k] = ny;
    }
}

/* (node) 1 when the node's aura shares a square with an earlier node's aura (same x / y, either plane), else 0 */
int Node_Site_Reject(int node)
{
    int power;
    int xs[20];
    int ys[20];
    int i;
    int aura;
    int square;

    power = p_nodes[node].power;
    for (i = 0; i < power; i++) {
        xs[i] = p_nodes[node].aura_x[i];
        ys[i] = p_nodes[node].aura_y[i];
    }
    for (i = 0; i < node; i++) {
        for (aura = 0; aura < p_nodes[i].power; aura++) {
            for (square = 0; square < power; square++) {
                if ((char)xs[square] == p_nodes[i].aura_x[aura] && (char)ys[square] == p_nodes[i].aura_y[aura])
                    return 1;
            }
        }
    }
    return 0;
}

/* (power, aura x / y, plane, realm) the node's realm: chaos (TILE_CHAOS_NODE) when Random(15) beats both Random(25) and
   another Random(15), else sorcery (TILE_SORCERY_NODE) or nature (TILE_NATURE_NODE). BUG #414: the aura's water / hill / grass counts are overwritten by the
   rolls and never used */
void Node_Pick_Realm(int power, char far *aura_x, char far *aura_y, int plane, char far *realm)
{
    int sorcery;
    int chaos;
    int nature;
    int i;

    sorcery = 0;
    chaos = 0;
    nature = 0;
    for (i = 0; i < power; i++) {
        if (p_terrain_maps[plane][aura_y[i]][aura_x[i]] == TILE_OCEAN)
            sorcery++;
        if (p_terrain_maps[plane][aura_y[i]][aura_x[i]] == TILE_MOUNTAIN || p_terrain_maps[plane][aura_y[i]][aura_x[i]] == TILE_DESERT)
            chaos++;
        if (p_terrain_maps[plane][aura_y[i]][aura_x[i]] == TILE_GRASSLAND || p_terrain_maps[plane][aura_y[i]][aura_x[i]] == TILE_FOREST)
            nature++;
    }
    sorcery = Random(25);
    chaos = Random(15);
    nature = Random(15);
    if (chaos > sorcery && chaos > nature) {
        p_terrain_maps[plane][aura_y[0]][aura_x[0]] = TILE_CHAOS_NODE;
        *realm = NODE_CHAOS;
        Landmass_Label(plane, aura_x[0], aura_y[0]);
    } else if (sorcery > nature) {
        p_terrain_maps[plane][aura_y[0]][aura_x[0]] = TILE_SORCERY_NODE;
        *realm = NODE_SORCERY;
        Landmass_Label(plane, aura_x[0], aura_y[0]);
    } else {
        p_terrain_maps[plane][aura_y[0]][aura_x[0]] = TILE_NATURE_NODE;
        *realm = NODE_NATURE;
        Landmass_Label(plane, aura_x[0], aura_y[0]);
    }
}

/* (plane, x, y) the square joins the landmass of its 8 neighbours: a new id when none has one, else the first
   neighbour id, every other id met being renamed to it all over the plane */
void Landmass_Label(int plane, int x, int y)
{
    int first;
    int i;
    int j;
    int x2;
    int y2;
    int old;
    int found[10];
    int count;

    first = 0;
    count = 0;
    if (p_landmass_ids[plane][y][x] != 0) {
        found[count] = p_landmass_ids[plane][y][x];
        count++;
    }
    if (p_landmass_ids[plane][y - 1][x - 1] != 0) {
        found[count] = p_landmass_ids[plane][y - 1][x - 1];
        count++;
    }
    if (p_landmass_ids[plane][y - 1][x] != 0) {
        found[count] = p_landmass_ids[plane][y - 1][x];
        count++;
    }
    if (p_landmass_ids[plane][y - 1][x + 1] != 0) {
        found[count] = p_landmass_ids[plane][y - 1][x + 1];
        count++;
    }
    if (p_landmass_ids[plane][y][x - 1] != 0) {
        found[count] = p_landmass_ids[plane][y][x - 1];
        count++;
    }
    if (p_landmass_ids[plane][y][x + 1] != 0) {
        found[count] = p_landmass_ids[plane][y][x + 1];
        count++;
    }
    if (p_landmass_ids[plane][y + 1][x - 1] != 0) {
        found[count] = p_landmass_ids[plane][y + 1][x - 1];
        count++;
    }
    if (p_landmass_ids[plane][y + 1][x] != 0) {
        found[count] = p_landmass_ids[plane][y + 1][x];
        count++;
    }
    if (p_landmass_ids[plane][y + 1][x + 1] != 0) {
        found[count] = p_landmass_ids[plane][y + 1][x + 1];
        count++;
    }
    if (count == 0) {
        p_landmass_ids[plane][y][x] = landmass_next_id;
        landmass_next_id++;
    } else {
        first = found[0];
        for (i = 1; i < count; i++) {
            if (found[i] != first) {
                old = found[i];
                for (j = i; j < count; j++) {
                    if (found[j] == old)
                        found[j] = first;
                }
                for (y2 = 0; y2 < MAP_HEIGHT; y2++) {
                    for (x2 = 0; x2 < MAP_WIDTH; x2++) {
                        if (p_landmass_ids[plane][y2][x2] == old)
                            p_landmass_ids[plane][y2][x2] = first;
                    }
                }
            }
        }
        p_landmass_ids[plane][y][x] = first;
    }
}

/* () the lairs: 0 .. 5 the towers (budget 700 .. 1200; copied to Myrror as 6 .. 11), 12 .. 41 the nodes (strength
   from power x magic, budget power^2 x (5 .. 15) x (magic + 1) / 2; the strength is not read by
   Lair_Create), then 25 normal and 32 weak lairs on random land
   squares 2+ from the lairs so far (Myrror ones richer); the last 3 slots inactive */
void Place_Lairs(void)
{
    int normal_lairs;
    int plane;
    int k;
    int x;
    int y;
    register int i;
    register int strength;

    normal_lairs = 25;
    for (i = 0; i < NUM_TOWERS; i++)
        Lair_Create(i, 0, p_towers[i].x, p_towers[i].y, 3, LAIR_CREATE_TOWER, Random(11) * 50 + 650);
    for (i = NUM_TOWERS; i < LAIR_SLOT_NODES; i++) {   /* slots: towers 0 .. 5 Arcanus, 6 .. 11 their Myrror
                                                   copies, 12 .. 41 the nodes, 42 .. the normal lairs, then the weak ones */
        Mem_Copy_Far(&p_lairs[i], &p_lairs[i - NUM_TOWERS], sizeof(struct Lair));
        p_lairs[i].plane = PLANE_MYRROR;
    }
    for (i = 0; i < NUM_NODES; i++) {
        strength = (p_nodes[i].power - 1) * magic_setting / 2 / 6 + 1;
        if (strength > 4)
            strength = 4;
        if (strength < 1)
            strength = 1;
        Lair_Create(i + LAIR_SLOT_NODES, p_nodes[i].plane, p_nodes[i].x, p_nodes[i].y, strength, p_nodes[i].realm + LAIR_CREATE_NODE,
                    p_nodes[i].power * p_nodes[i].power * (Random(11) + 4) * (magic_setting + 1) / 2);
    }
    for (i = 0; i < normal_lairs; i++) {
normal:
        plane = Random(2) - 1;
        x = Random(54) + 2;
        y = Random(34) + 2;
        if (Tile_Is_Water_Gen(x, y, plane) == 1)
            goto normal;
        for (k = 0; k < i + LAIR_SLOT_NORMAL; k++) {
            if (Map_Distance(x, y, p_lairs[k].x, p_lairs[k].y, MAP_WIDTH) < 2)
                goto normal;
        }
        if (plane == PLANE_ARCANUS)
            strength = Random(29) * 50 + 50;
        else
            strength = Random(24) * 100 + 100;
        Lair_Create(i + LAIR_SLOT_NORMAL, plane, x, y, strength, LAIR_CREATE_RANDOM, strength);
    }
    for (i = 0; i < 32; i++) {
weak:
        plane = Random(2) - 1;
        x = Random(54) + 2;
        y = Random(34) + 2;
        if (Tile_Is_Water_Gen(x, y, plane) == 1)
            goto weak;
        for (k = 0; k < normal_lairs + i + 42; k++) {
            if (Map_Distance(x, y, p_lairs[k].x, p_lairs[k].y, MAP_WIDTH) < 2)
                goto weak;
        }
        strength = Random(plane == PLANE_ARCANUS ? 10 : 20) * 10;
        Lair_Create(i + normal_lairs + 42, plane, x, y, strength, LAIR_CREATE_RANDOM, strength);
    }
    for (i = normal_lairs + 74; i < normal_lairs + 77; i++)
        p_lairs[i].active = 0;
}

/* (lair, plane, x, y, unused_strength (never read), kind, budget) one lair: budget x (difficulty + 1) / 4; the type by kind (0 tower,
   1 .. 3 sorcery / nature / chaos node, 4 Random(7) of cave .. fallen temple); the guardians' realm (unit race 16 nature
   .. 20 death): nodes their own, towers death 2 / 6 else one each, temples / ruins / fallen temples death 3 / 4 else
   life, the other four always nature (BUG #357: the switch has no breaks); the strongest creature of that realm (unit types
   150 .. 197, no transports) under budget / Random(4), then a second kind, at most 9 in all; the treasure from the
   guardians' cost (gold, mana, items, a prisoner, spells, picks)
   Numbered: BUG #357 (BUGS_AND_QUIRKS.txt) */
void Lair_Create(int lair, int plane, int x, int y, int unused_strength, int site_kind, int budget)
{
    int i;
    int guard_race;
    int roll;
    int best;
    int best_cost;
    int limit;
    int tries;
    int treasure;

    budget = (long)budget * (difficulty + 1) / 4;
    p_lairs[lair].x = x;
    p_lairs[lair].y = y;
    p_lairs[lair].plane = plane;
    p_lairs[lair].active = 1;
    switch (site_kind) {
    case LAIR_CREATE_TOWER:
        p_lairs[lair].type = LAIR_TOWER;
        break;
    case LAIR_CREATE_CHAOS_NODE:
        p_lairs[lair].type = LAIR_CHAOS_NODE;
        break;
    case LAIR_CREATE_NATURE_NODE:
        p_lairs[lair].type = LAIR_NATURE_NODE;
        break;
    case LAIR_CREATE_SORCERY_NODE:
        p_lairs[lair].type = LAIR_SORCERY_NODE;
        break;
    case LAIR_CREATE_RANDOM:
        p_lairs[lair].type = Random(7) + 3;   /* 4 .. 10: LAIR_MYSTERIOUS_CAVE .. LAIR_FALLEN_TEMPLE */
        break;
    }
    p_lairs[lair].gold = 0;
    p_lairs[lair].mana = 0;
    p_lairs[lair].special = 0;
    p_lairs[lair].item_count = 0;
    p_lairs[lair].flags = 0;
    p_lairs[lair].guard1_type = 0;
    p_lairs[lair].guard2_type = 0;
    p_lairs[lair].guard1_count = 0;
    p_lairs[lair].guard2_count = 0;
    if (site_kind == LAIR_CREATE_SORCERY_NODE)
        guard_race = RACE_SORCERY_CREATURE;
    else if (site_kind == LAIR_CREATE_CHAOS_NODE)
        guard_race = RACE_CHAOS_CREATURE;
    else if (site_kind == LAIR_CREATE_NATURE_NODE)
        guard_race = RACE_NATURE_CREATURE;
    else if (site_kind == LAIR_CREATE_TOWER) {
        switch (Random(6)) {
        case 1:
            guard_race = RACE_DEATH_CREATURE;
            break;
        case 2:
            guard_race = RACE_DEATH_CREATURE;
            break;
        case 3:
            guard_race = RACE_CHAOS_CREATURE;
            break;
        case 4:
            guard_race = RACE_LIFE_CREATURE;
            break;
        case 5:
            guard_race = RACE_NATURE_CREATURE;
            break;
        case 6:
            guard_race = RACE_SORCERY_CREATURE;
            break;
        }
    } else if (p_lairs[lair].type == LAIR_ANCIENT_TEMPLE || p_lairs[lair].type == LAIR_RUINS || p_lairs[lair].type == LAIR_FALLEN_TEMPLE) {
        switch (Random(4)) {
        case 1:
            guard_race = RACE_DEATH_CREATURE;
            break;
        case 2:
            guard_race = RACE_DEATH_CREATURE;
            break;
        case 3:
            guard_race = RACE_DEATH_CREATURE;
            break;
        case 4:
            guard_race = RACE_LIFE_CREATURE;
            break;
        }
    } else {
        switch (Random(5)) {
        case 1:
            guard_race = RACE_DEATH_CREATURE;
        case 2:
            guard_race = RACE_DEATH_CREATURE;
        case 3:
            guard_race = RACE_CHAOS_CREATURE;
        case 4:
            guard_race = RACE_CHAOS_CREATURE;
        case 5:
            guard_race = RACE_NATURE_CREATURE;
        }
    }
    tries = 0;
    do {
        limit = budget / Random(4);
        best_cost = 0;
        best = -1;
        for (i = 150; i < NUM_UNIT_TYPES; i++) {   /* from 150; guard_race leaves the summons (UNIT_MAGIC_SPIRIT ..) */
            if (unit_types[i].race == guard_race && unit_types[i].transport_capacity == 0 && unit_types[i].cost > best_cost &&
                unit_types[i].cost < limit) {
                best_cost = unit_types[i].cost;
                best = i;
            }
        }
        tries++;
    } while (best == -1 && tries < 200);
    if (tries < 200) {
        p_lairs[lair].guard1_type = best;
        p_lairs[lair].guard1_count = budget / unit_types[p_lairs[lair].guard1_type].cost;
        if (p_lairs[lair].guard1_count > 1 && Random(2) == 1)
            p_lairs[lair].guard1_count = p_lairs[lair].guard1_count - 1;
        if (p_lairs[lair].guard1_count > 8)
            p_lairs[lair].guard1_count = 8;
        budget -= unit_types[p_lairs[lair].guard1_type].cost * p_lairs[lair].guard1_count;
    }
    tries = 0;
    if (p_lairs[lair].guard1_count != MAX_STACK && p_lairs[lair].guard1_count != 0) {
        do {
            limit = budget / Random(10 - p_lairs[lair].guard1_count);
            best_cost = 0;
            best = -1;
            for (i = 150; i < NUM_UNIT_TYPES; i++) {   /* from 150; guard_race leaves the summons (UNIT_MAGIC_SPIRIT ..) */
                if (unit_types[i].race == guard_race && unit_types[i].transport_capacity == 0 &&
                    unit_types[i].cost > best_cost && unit_types[i].cost < limit && p_lairs[lair].guard1_type != i) {
                    best_cost = unit_types[i].cost;
                    best = i;
                }
            }
            tries++;
        } while (best == -1 && tries < 200);
        if (tries < 200) {
            p_lairs[lair].guard2_type = best;
            p_lairs[lair].guard2_count = budget / unit_types[p_lairs[lair].guard2_type].cost;
            if (p_lairs[lair].guard1_count + p_lairs[lair].guard2_count > MAX_STACK)
                p_lairs[lair].guard2_count = MAX_STACK - p_lairs[lair].guard1_count;
        }
    }
    if (p_lairs[lair].guard1_type == 0)
        p_lairs[lair].guard1_count = 0;
    if (p_lairs[lair].guard2_type == 0)
        p_lairs[lair].guard2_count = 0;
    treasure = unit_types[p_lairs[lair].guard1_type].cost * p_lairs[lair].guard1_count +
               unit_types[p_lairs[lair].guard2_type].cost * p_lairs[lair].guard2_count / 2;
    if (difficulty != DIFFICULTY_IMPOSSIBLE)
        treasure = (long)treasure * 4 / (difficulty + 1);
    if (plane == PLANE_ARCANUS)
        treasure = (long)treasure * (Random(76) + 49) / 100;
    else
        treasure = (long)treasure * (Random(100) + 75) / 100;
    if (treasure < 50)
        treasure = 50;
    if (site_kind == LAIR_CREATE_TOWER) {
        roll = Random(4);
        treasure -= roll * roll * 50 - 100;
        p_lairs[lair].special = roll;
    }
    while (treasure >= 50) {
        switch (Random(15)) {
        case 1:
        case 2:
            roll = Random(20) * 10;
            if (roll > treasure)
                roll = treasure;
            roll = roll / 10 * 10;
            treasure -= 200;
            p_lairs[lair].gold += roll;
            break;
        case 3:
        case 4:
            roll = Random(20) * 10;
            if (roll > treasure)
                roll = treasure;
            roll = roll / 10 * 10;
            treasure -= 200;
            p_lairs[lair].mana += roll;
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            if (p_lairs[lair].item_count < 3 && treasure >= 300) {
                roll = Random(27) * 100 + 300;
                if (roll > treasure)
                    roll = treasure;
                roll = roll / 10 * 10;
                treasure -= roll;
                p_lairs[lair].item_values[p_lairs[lair].item_count] = roll;
                p_lairs[lair].item_count = p_lairs[lair].item_count + 1;
            }
            break;
        case 10:
            if (treasure >= 400 && p_lairs[lair].flags == 0) {   /* needs 400, costs 1000: the treasure can go negative */
                p_lairs[lair].flags = 1 << LAIR_BIT_PRISONER;
                treasure -= 1000;
            }
            break;
        case 11:
        case 12:
        case 13:
            roll = Random(4);
            if (roll * roll * 50 <= treasure && p_lairs[lair].special < 4) {
                treasure -= roll * roll * 50;
                p_lairs[lair].special = p_lairs[lair].special + roll;
                if (p_lairs[lair].special > 4)
                    p_lairs[lair].special = 4;
            }
            break;
        case 14:
        case 15:
            roll = Random(4);
            if (roll == 1 && treasure >= 3000) {
                p_lairs[lair].special = LAIR_SPECIAL_TWO_BOOKS;
                treasure -= 3000;
            } else if (treasure >= 2000) {
                p_lairs[lair].special = LAIR_SPECIAL_TWO_BOOKS;
                treasure -= 3000;               /* 3000 here and below, whatever the branch's threshold */
            } else if (treasure >= 1000) {
                p_lairs[lair].special = LAIR_SPECIAL_BOOK;
                treasure -= 3000;
            }
            break;
        }
    }
    if (p_lairs[lair].special > 4) {           /* a book reward (LAIR_SPECIAL_BOOK / _TWO_BOOKS) replaces the rest */
        p_lairs[lair].gold = 0;
        p_lairs[lair].mana = 0;
        p_lairs[lair].item_count = 0;
        p_lairs[lair].flags = 0;
    }
}

/* (race_code, max_upkeep) a random unit type (Random(197)) of that race code (15 .. 20: a realm) with an upkeep byte at most max_upkeep and
   a melee attack; after 50 tries max_upkeep is lowered every 10th try; 0 after 500 tries. No caller in MAGIC */
int Pick_Creature_Of_Realm(int race_code, int max_upkeep)
{
    int unit_type;
    int tries;

    for (tries = 0; tries < 500; tries++) {
        unit_type = Random(197);
        if (unit_types[unit_type].race == race_code && unit_types[unit_type].upkeep <= max_upkeep && unit_types[unit_type].melee != 0)
            return unit_type;
        if (tries > 50 && tries % 10 == 0 && max_upkeep > 0)
            max_upkeep--;
    }
    return 0;
}

/* () desert edges, after Gen_River_Pictures: (1) a TILE_LAKE_NO_RIVER square (0x134, the river-less lake picture, inside the
   desert-edge band) takes the landmass id of a land neighbour (8
   neighbours, not clipped at the map edge - a latent read off the map on rows 0 / 39); (2) every desert square
   (TILE_DESERT .. _4, TILE_LAKE_NO_RIVER) gets the variant TERRTYPE.LBX 0 table[mask] + TILE_DESERT_EDGE_BASE for its
   non-desert neighbours
   (8-bit mask from the NW clockwise, clipped), or plain TILE_DESERT when all neighbours are desert */
void Gen_Desert_Edges(void)
{
    int plane;
    int mask;
    int dx;
    int dy;
    int *variants;
    int x;
    int y;

    variants = (int *)Near_Alloc_Reset(2560);
    LBX_Load_Records_Near("TERRTYPE", 0, variants, 0, 5, 512);
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_LAKE_NO_RIVER) {
                    for (dy = -1; dy < 2; dy++) {
                        for (dx = -1; dx < 2; dx++) {
                            if (p_landmass_ids[plane][y + dy][dx + x] != 0)
                                p_landmass_ids[plane][y][x] = p_landmass_ids[plane][y + dy][dx + x];
                        }
                    }
                }
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_DESERT || p_terrain_maps[plane][y][x] == TILE_DESERT_2 ||
                    p_terrain_maps[plane][y][x] == TILE_DESERT_3 || p_terrain_maps[plane][y][x] == TILE_DESERT_4 ||
                    p_terrain_maps[plane][y][x] == TILE_LAKE_NO_RIVER) {
                    mask = 0;
                    if (Tile_Is_Desert_Range(x - 1, y - 1, plane) == 0 && x - 1 >= 0 && y - 1 >= 0)
                        mask++;
                    if (Tile_Is_Desert_Range(x, y - 1, plane) == 0 && y - 1 >= 0)
                        mask += 2;
                    if (Tile_Is_Desert_Range(x + 1, y - 1, plane) == 0 && x + 1 < MAP_WIDTH && y - 1 >= 0)
                        mask += 4;
                    if (Tile_Is_Desert_Range(x + 1, y, plane) == 0 && x + 1 < MAP_WIDTH)
                        mask += 8;
                    if (Tile_Is_Desert_Range(x + 1, y + 1, plane) == 0 && x + 1 < MAP_WIDTH && y + 1 < MAP_HEIGHT)
                        mask += 16;
                    if (Tile_Is_Desert_Range(x, y + 1, plane) == 0 && y + 1 < MAP_HEIGHT)
                        mask += 32;
                    if (Tile_Is_Desert_Range(x - 1, y + 1, plane) == 0 && x - 1 >= 0 && y + 1 < MAP_HEIGHT)
                        mask += 64;
                    if (Tile_Is_Desert_Range(x - 1, y, plane) == 0 && x - 1 >= 0)
                        mask += 128;
                    if (mask > 0)
                        p_terrain_maps[plane][y][x] = variants[mask] + TILE_DESERT_EDGE_BASE;
                    else
                        p_terrain_maps[plane][y][x] = TILE_DESERT;
                }
            }
        }
    }
}

/* () neighbour-mask picture variants (masks from the NW clockwise: 1 NW, 2 N, 4 NE, 8 E, 16 SE, 32 S, 64 SW, 128 W;
   TERRTYPE.LBX 0, five 256-word tables loaded, two used - table 1 / 2 = variants[0] / [1]): (1) ocean squares get table 1 [mask of land neighbours TILE_LAND_FIRST .. TILE_HILLS]
   (columns wrap); (2) TILE_MOUNTAIN get table 2 [mask of mountain neighbours 0xa4 / 0x103 .. 0x112]; (3) TILE_TUNDRA
   gets table 1 [mask of non-tundra neighbours] + 0x258 (columns wrap, but the east neighbour also needs x + 1 <
   MAP_WIDTH: clipped); (4) TILE_HILLS get table 2 [mask of hill
   neighbours 0xab / 0x113 .. 0x123] + 0x10. BUG #415: the south tests are y < 40, not y + 1 < 40 (row 40 read off the map) */
void Gen_Neighbour_Variants(void)
{
    int x;
    int plane;
    int mask;
    int wx;
    int (*variants)[256];
    int y;

    variants = (int (*)[256])Near_Alloc_Reset(2560);
    LBX_Load_Records_Near("TERRTYPE", 0, variants, 0, 5, 512);
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_OCEAN) {
                    mask = 0;
                    wx = x - 1;
                    if (wx < 0)
                        wx += MAP_WIDTH;
                    if (p_terrain_maps[plane][y - 1][wx] >= TILE_LAND_FIRST && p_terrain_maps[plane][y - 1][wx] <= TILE_LAND_LAST && y - 1 >= 0)
                        mask++;
                    if (p_terrain_maps[plane][y - 1][x] >= TILE_LAND_FIRST && p_terrain_maps[plane][y - 1][x] <= TILE_LAND_LAST && y - 1 >= 0)
                        mask += 2;
                    wx = x + 1;
                    if (wx >= MAP_WIDTH)
                        wx -= MAP_WIDTH;
                    if (p_terrain_maps[plane][y - 1][wx] >= TILE_LAND_FIRST && p_terrain_maps[plane][y - 1][wx] <= TILE_LAND_LAST && y - 1 >= 0)
                        mask += 4;
                    wx = x + 1;
                    if (wx >= MAP_WIDTH)
                        wx -= MAP_WIDTH;
                    if (p_terrain_maps[plane][y][wx] >= TILE_LAND_FIRST && p_terrain_maps[plane][y][wx] <= TILE_LAND_LAST)
                        mask += 8;
                    wx = x + 1;
                    if (wx >= MAP_WIDTH)
                        wx -= MAP_WIDTH;
                    if (p_terrain_maps[plane][y + 1][wx] >= TILE_LAND_FIRST && p_terrain_maps[plane][y + 1][wx] <= TILE_LAND_LAST && y < MAP_HEIGHT)
                        mask += 16;
                    if (p_terrain_maps[plane][y + 1][x] >= TILE_LAND_FIRST && p_terrain_maps[plane][y + 1][x] <= TILE_LAND_LAST && y < MAP_HEIGHT)
                        mask += 32;
                    wx = x - 1;
                    if (wx < 0)
                        wx += MAP_WIDTH;
                    if (p_terrain_maps[plane][y + 1][wx] >= TILE_LAND_FIRST && p_terrain_maps[plane][y + 1][wx] <= TILE_LAND_LAST && y < MAP_HEIGHT)
                        mask += 64;
                    wx = x - 1;
                    if (wx < 0)
                        wx += MAP_WIDTH;
                    if (p_terrain_maps[plane][y][wx] >= TILE_LAND_FIRST && p_terrain_maps[plane][y][wx] <= TILE_LAND_LAST)
                        mask += 128;
                    if (mask > 0)
                        p_terrain_maps[plane][y][x] = variants[0][mask];
                }
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_MOUNTAIN) {
                    mask = 0;
                    if ((p_terrain_maps[plane][y - 1][x - 1] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y - 1][x - 1] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y - 1][x - 1] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        x - 1 >= 0 && y - 1 >= 0)
                        mask++;
                    if ((p_terrain_maps[plane][y - 1][x] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y - 1][x] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y - 1][x] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        y - 1 >= 0)
                        mask += 2;
                    if ((p_terrain_maps[plane][y - 1][x + 1] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y - 1][x + 1] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y - 1][x + 1] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        x + 1 < MAP_WIDTH && y - 1 >= 0)
                        mask += 4;
                    if ((p_terrain_maps[plane][y][x + 1] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y][x + 1] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y][x + 1] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        x + 1 < MAP_WIDTH)
                        mask += 8;
                    if ((p_terrain_maps[plane][y + 1][x + 1] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y + 1][x + 1] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y + 1][x + 1] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        x + 1 < MAP_WIDTH && y < MAP_HEIGHT)
                        mask += 16;
                    if ((p_terrain_maps[plane][y + 1][x] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y + 1][x] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y + 1][x] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        y < MAP_HEIGHT)
                        mask += 32;
                    if ((p_terrain_maps[plane][y + 1][x - 1] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y + 1][x - 1] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y + 1][x - 1] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        x - 1 >= 0 && y < MAP_HEIGHT)
                        mask += 64;
                    if ((p_terrain_maps[plane][y][x - 1] == TILE_MOUNTAIN ||
                         (p_terrain_maps[plane][y][x - 1] >= TILE_MOUNTAIN_RANGE_FIRST && p_terrain_maps[plane][y][x - 1] <= TILE_MOUNTAIN_RANGE_LAST)) &&
                        x - 1 >= 0)
                        mask += 128;
                    if (mask > 0)
                        p_terrain_maps[plane][y][x] = variants[1][mask];
                }
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_TUNDRA) {
                    mask = 0;
                    wx = x - 1;
                    if (wx < 0)
                        wx += MAP_WIDTH;
                    if (p_terrain_maps[plane][y - 1][wx] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y - 1][wx] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y - 1][wx] > TERRAIN_IDS_PER_PLANE) && y - 1 >= 0)
                        mask++;
                    if (p_terrain_maps[plane][y - 1][x] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y - 1][x] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y - 1][x] > TERRAIN_IDS_PER_PLANE) && y - 1 >= 0)
                        mask += 2;
                    wx = x + 1;
                    if (wx >= MAP_WIDTH)
                        wx -= MAP_WIDTH;
                    if (p_terrain_maps[plane][y - 1][wx] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y - 1][wx] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y - 1][wx] > TERRAIN_IDS_PER_PLANE) && y - 1 >= 0)
                        mask += 4;
                    wx = x + 1;
                    if (wx >= MAP_WIDTH)
                        wx -= MAP_WIDTH;
                    if (p_terrain_maps[plane][y][wx] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y][wx] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y][wx] > TERRAIN_IDS_PER_PLANE) && x + 1 < MAP_WIDTH)
                        mask += 8;
                    wx = x + 1;
                    if (wx >= MAP_WIDTH)
                        wx -= MAP_WIDTH;
                    if (p_terrain_maps[plane][y + 1][wx] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y + 1][wx] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y + 1][wx] > TERRAIN_IDS_PER_PLANE) && y < MAP_HEIGHT)
                        mask += 16;
                    if (p_terrain_maps[plane][y + 1][x] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y + 1][x] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y + 1][x] > TERRAIN_IDS_PER_PLANE) && y < MAP_HEIGHT)
                        mask += 32;
                    wx = x - 1;
                    if (wx < 0)
                        wx += MAP_WIDTH;
                    if (p_terrain_maps[plane][y + 1][wx] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y + 1][wx] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y + 1][wx] > TERRAIN_IDS_PER_PLANE) && y < MAP_HEIGHT)
                        mask += 64;
                    wx = x - 1;
                    if (wx < 0)
                        wx += MAP_WIDTH;
                    if (p_terrain_maps[plane][y][wx] != TILE_TUNDRA &&
                        (p_terrain_maps[plane][y][wx] < TILE_TUNDRA_RANGE_FIRST || p_terrain_maps[plane][y][wx] > TERRAIN_IDS_PER_PLANE))
                        mask += 128;
                    if (mask > 0)
                        p_terrain_maps[plane][y][x] = variants[0][mask] + TILE_TUNDRA_EDGE_BASE;
                }
            }
        }
    }
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_HILLS) {
                    mask = 0;
                    if ((p_terrain_maps[plane][y - 1][x - 1] == TILE_HILLS ||
                         (p_terrain_maps[plane][y - 1][x - 1] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y - 1][x - 1] <= TILE_HILLS_RANGE_LAST)) &&
                        x - 1 >= 0 && y - 1 >= 0)
                        mask++;
                    if ((p_terrain_maps[plane][y - 1][x] == TILE_HILLS ||
                         (p_terrain_maps[plane][y - 1][x] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y - 1][x] <= TILE_HILLS_RANGE_LAST)) &&
                        y - 1 >= 0)
                        mask += 2;
                    if ((p_terrain_maps[plane][y - 1][x + 1] == TILE_HILLS ||
                         (p_terrain_maps[plane][y - 1][x + 1] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y - 1][x + 1] <= TILE_HILLS_RANGE_LAST)) &&
                        x + 1 < MAP_WIDTH && y - 1 >= 0)
                        mask += 4;
                    if ((p_terrain_maps[plane][y][x + 1] == TILE_HILLS ||
                         (p_terrain_maps[plane][y][x + 1] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y][x + 1] <= TILE_HILLS_RANGE_LAST)) &&
                        x + 1 < MAP_WIDTH)
                        mask += 8;
                    if ((p_terrain_maps[plane][y + 1][x + 1] == TILE_HILLS ||
                         (p_terrain_maps[plane][y + 1][x + 1] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y + 1][x + 1] <= TILE_HILLS_RANGE_LAST)) &&
                        x + 1 < MAP_WIDTH && y < MAP_HEIGHT)
                        mask += 16;
                    if ((p_terrain_maps[plane][y + 1][x] == TILE_HILLS ||
                         (p_terrain_maps[plane][y + 1][x] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y + 1][x] <= TILE_HILLS_RANGE_LAST)) &&
                        y < MAP_HEIGHT)
                        mask += 32;
                    if ((p_terrain_maps[plane][y + 1][x - 1] == TILE_HILLS ||
                         (p_terrain_maps[plane][y + 1][x - 1] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y + 1][x - 1] <= TILE_HILLS_RANGE_LAST)) &&
                        x - 1 >= 0 && y < MAP_HEIGHT)
                        mask += 64;
                    if ((p_terrain_maps[plane][y][x - 1] == TILE_HILLS ||
                         (p_terrain_maps[plane][y][x - 1] >= TILE_HILLS_RANGE_FIRST && p_terrain_maps[plane][y][x - 1] <= TILE_HILLS_RANGE_LAST)) &&
                        x - 1 >= 0)
                        mask += 128;
                    if (mask > 0)
                        p_terrain_maps[plane][y][x] = variants[1][mask] + TILE_HILLS_FROM_MOUNTAIN;
                }
            }
        }
    }
}

/* () picture variety on both planes: TILE_GRASSLAND -> itself / _4 / _2 / _3 (Random(4)), TILE_FOREST -> itself /
   _2 / _3 (Random(3)), TILE_DESERT -> itself / _2 .. _4, TILE_SWAMP -> itself / _2 / _3, TILE_TUNDRA -> itself / _2;
   loads TERRTYPE records 0 .. 4 into a near buffer it never reads. BUG #365: the tundra cases are 1 / 3 / 4 of
   Random(3), so two thirds of the tundra stay plain (case 1 and the missing 2) and TILE_TUNDRA_3 is never placed */
void Gen_Terrain_Variety(void)
{
    int plane;
    int roll;
    int *variants;
    register int x;
    register int y;

    variants = (int *)Near_Alloc_Reset(2560);
    LBX_Load_Records_Near("TERRTYPE", 0, variants, 0, 5, 512);
    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_GRASSLAND) {
                    roll = Random(4);
                    switch (roll) {
                    case 1:
                        p_terrain_maps[plane][y][x] = TILE_GRASSLAND;
                        break;
                    case 2:
                        p_terrain_maps[plane][y][x] = TILE_GRASSLAND_4;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = TILE_GRASSLAND_2;
                        break;
                    case 4:
                        p_terrain_maps[plane][y][x] = TILE_GRASSLAND_3;
                        break;
                    }
                }
                if (p_terrain_maps[plane][y][x] == TILE_FOREST) {
                    roll = Random(3);
                    switch (roll) {
                    case 1:
                        p_terrain_maps[plane][y][x] = TILE_FOREST;
                        break;
                    case 2:
                        p_terrain_maps[plane][y][x] = TILE_FOREST_2;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = TILE_FOREST_3;
                        break;
                    }
                }
                if (p_terrain_maps[plane][y][x] == TILE_DESERT) {
                    roll = Random(4);
                    switch (roll) {
                    case 1:
                        p_terrain_maps[plane][y][x] = TILE_DESERT;
                        break;
                    case 2:
                        p_terrain_maps[plane][y][x] = TILE_DESERT_2;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = TILE_DESERT_3;
                        break;
                    case 4:
                        p_terrain_maps[plane][y][x] = TILE_DESERT_4;
                        break;
                    }
                }
                if (p_terrain_maps[plane][y][x] == TILE_SWAMP) {
                    roll = Random(3);
                    switch (roll) {
                    case 1:
                        p_terrain_maps[plane][y][x] = TILE_SWAMP;
                        break;
                    case 2:
                        p_terrain_maps[plane][y][x] = TILE_SWAMP_2;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = TILE_SWAMP_3;
                        break;
                    }
                }
                if (p_terrain_maps[plane][y][x] == TILE_TUNDRA) {
                    roll = Random(3);
                    switch (roll) {
                    case 1:
                        p_terrain_maps[plane][y][x] = TILE_TUNDRA;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = TILE_TUNDRA_2;
                        break;
                    case 4:
                        p_terrain_maps[plane][y][x] = TILE_TUNDRA_3;
                        break;
                    }
                }
            }
        }
    }
}

/* (plane) one river: a start 5 .. 56 x 5 .. 36 with no mineral, no water in its 3 x 3, not mountain / hills / node /
   river; then up to 30 steps (half the time the first direction, else any direction but the opposite of the FIRST
   one) over squares without a mineral, mountain, hills, node or desert, until it joins a river or touches water (N /
   W / here / E / S); 4 .. 28 squares and the mouth not beside a shore: its squares become terrain 1000 (the marker
   Gen_River_Pictures turns into river pictures). 1 = placed, 0 = rejected */
int Gen_River(int plane)
{
    int count;
    int x;
    int y;
    int first_dir;
    int prev_dir;
    int i;
    int dir;
    int done;
    int steps;
    int nx;
    int ny;
    int last_dir;                               /* written, never read */
    int end_x;
    int end_y;
    int xs[30];
    int ys[30];
    int dirs[30];

    x = Random(52) + 4;
    y = Random(32) + 4;
    if (p_mineral_maps[plane][y][x] != 0)
        return 0;
    if (Tile_Is_Water_Gen(x - 1, y - 1, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x - 1, y, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x - 1, y + 1, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x, y - 1, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x, y, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x, y + 1, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x + 1, y - 1, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x + 1, y, plane) == 1)
        return 0;
    if (Tile_Is_Water_Gen(x + 1, y + 1, plane) == 1)
        return 0;
    if (Tile_Is_Mountain_Gen(x, y, plane) == 1)
        return 0;
    if (Tile_Is_Hills_Gen(x, y, plane) == 1)
        return 0;
    if (Is_Node_At(x, y, plane) == 1)
        return 0;
    if (Tile_Is_River_Gen(x, y, plane) == 1)
        return 0;
    xs[0] = x;
    ys[0] = y;
    count = 1;
    done = 0;
    steps = 0;
    first_dir = Random(4) - 1;
    prev_dir = -1;
    dirs[0] = first_dir;
    while (done == 0) {
retry:
        if (Random(2) > 1 || prev_dir == -1)
            dir = first_dir;
        else {
pick:
            dir = Random(4) - 1;
            if (step_opposite[first_dir] == dir)
                goto pick;
        }
        dirs[count] = dir;
        prev_dir = dir;
        nx = xs[count - 1] + step_dx[dir];
        ny = ys[count - 1] + step_dy[dir];
        last_dir = dir;
        steps++;
        if (steps > 30)
            return 0;
        if (p_mineral_maps[plane][ny][nx] != 0)
            goto retry;
        if (Tile_Is_Mountain_Gen(nx, ny, plane) == 1)
            goto retry;
        if (Tile_Is_Hills_Gen(nx, ny, plane) == 1)
            goto retry;
        if (Is_Node_At(nx, ny, plane) == 1)
            goto retry;
        if (Tile_Is_Desert_Range(nx, ny, plane) == 1)
            goto retry;
        if (Tile_Is_River_Gen(nx, ny, plane) == 1)
            done = 1;
        xs[count] = nx;
        ys[count] = ny;
        count++;
        if (Tile_Is_Water_Gen(nx, ny - 1, plane) == 1)
            done = 1;
        if (Tile_Is_Water_Gen(nx - 1, ny, plane) == 1)
            done = 1;
        if (Tile_Is_Water_Gen(nx, ny, plane) == 1)
            done = 1;
        if (Tile_Is_Water_Gen(nx + 1, ny, plane) == 1)
            done = 1;
        if (Tile_Is_Water_Gen(nx, ny + 1, plane) == 1)
            done = 1;
        if (count > 28)
            return 0;
        prev_dir = dir;
    }
    if (count < 4)
        return 0;
    end_x = xs[count - 1];
    end_y = ys[count - 1];
    if (Tile_Is_Shore_High(end_x, end_y - 1, plane) == 1)
        return 0;
    if (Tile_Is_Shore_High(end_x - 1, end_y, plane) == 1)
        return 0;
    if (Tile_Is_Shore_High(end_x, end_y, plane) == 1)
        return 0;
    if (Tile_Is_Shore_High(end_x + 1, end_y, plane) == 1)
        return 0;
    if (Tile_Is_Shore_High(end_x, end_y + 1, plane) == 1)
        return 0;
    for (i = 0; i < count; i++)
        p_terrain_maps[plane][ys[i]][xs[i]] = TILE_RIVER_MARKER;
    return 1;
}

/* (plane) rivers and river mouths: every river square (Tile_Is_River_Gen, incl. Gen_River's marker 1000) gets
   river_variants[N 1 / E 2 / S 4 / W 8 mask of water or river][Random(4) - 1]; then the shore squares beside a river
   (mask of river neighbours) get their river-mouth picture by shore code (0x12 lakes, 3 .. 30, 0x22 .. 0x32, then the
   0x32 .. 0x61 families); where a mouth needs it, a neighbour becomes grassland 0xa2 and x / y step back so the scan
   passes those squares again */
void Gen_River_Pictures(int plane)
{
    int mask;
    int terrain;
    register int x;
    register int y;

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            terrain = p_terrain_maps[plane][y][x];
            if (Tile_Is_River_Gen(x, y, plane) == 1)
                terrain = TILE_RIVER_MARKER;
            if (terrain == TILE_RIVER_MARKER) {
                mask = 0;
                if (Tile_Is_Water_Gen(x, y - 1, plane) == 1 || Tile_Is_River_Gen(x, y - 1, plane) == 1)
                    mask++;
                if (Tile_Is_Water_Gen(x + 1, y, plane) == 1 || Tile_Is_River_Gen(x + 1, y, plane) == 1)
                    mask += 2;
                if (Tile_Is_Water_Gen(x, y + 1, plane) == 1 || Tile_Is_River_Gen(x, y + 1, plane) == 1)
                    mask += 4;
                if (Tile_Is_Water_Gen(x - 1, y, plane) == 1 || Tile_Is_River_Gen(x - 1, y, plane) == 1)
                    mask += 8;
                p_terrain_maps[plane][y][x] = river_variants[mask][Random(4) - 1];
            }
            mask = 0;
            if (Tile_Is_River_Gen(x, y - 1, plane) == 1)
                mask++;
            if (Tile_Is_River_Gen(x + 1, y, plane) == 1)
                mask += 2;
            if (Tile_Is_River_Gen(x, y + 1, plane) == 1)
                mask += 4;
            if (Tile_Is_River_Gen(x - 1, y, plane) == 1)
                mask += 8;
            if (terrain == TILE_LAKE) {         /* a one-square lake: by its river neighbours (N 1, E 2, S 4, W 8) */
                switch (mask) {
                case 0:                         /* no river: TILE_LAKE_NO_RIVER (in the desert-edge band; Gen_Desert_Edges
                                                   then treats it as desert and gives it a land neighbour's landmass) */
                    p_terrain_maps[plane][y][x] = TILE_LAKE_NO_RIVER;
                    break;
                case 1:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    break;
                case 2:
                    p_terrain_maps[plane][y][x] = 0xc7;
                    break;
                case 3:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    x--;
                    y--;
                    break;
                case 4:
                    p_terrain_maps[plane][y][x] = 0xc8;
                    break;
                case 5:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                    x--;
                    y--;
                    break;
                case 6:
                    p_terrain_maps[plane][y][x] = 0xc8;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    x--;
                    y--;
                    break;
                case 7:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    x--;
                    y--;
                    break;
                case 8:
                    p_terrain_maps[plane][y][x] = 0xc5;
                    break;
                case 9:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                    x -= 2;
                    y--;
                    break;
                case 10:
                    p_terrain_maps[plane][y][x] = 0xc5;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    x--;
                    y--;
                    break;
                case 11:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                    x -= 2;
                    y--;
                    break;
                case 12:
                    p_terrain_maps[plane][y][x] = 0xc5;
                    p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                    x -= 2;
                    y--;
                    break;
                case 13:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                    p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                    x -= 2;
                    y--;
                    break;
                case 14:
                    p_terrain_maps[plane][y][x] = 0xc5;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                    x -= 2;
                    y--;
                    break;
                case 15:
                    p_terrain_maps[plane][y][x] = 0xc6;
                    p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                    p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                    p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                    x -= 2;
                    y--;
                    break;
                }
            } else if (terrain < 0x1f) {        /* the shore codes (TERRTYPE.LBX 0's shore table) by family: each
                                                   case below picks the TERRAIN.LBX river-mouth picture for this
                                                   shore shape and river mask; the picture ids have no names */
                switch (terrain) {
                case 3:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0xda;
                    break;
                case 4:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0xd9;
                    break;
                case 5:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0xdb;
                    break;
                case 10:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0xe7;
                    break;
                case 11:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0xdc;
                    break;
                case 12:
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0xde;
                    break;
                case 16:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0xe5;
                    break;
                case 17:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0xe8;
                    break;
                case 19:
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0xe0;
                    break;
                case 20:
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0xdd;
                    break;
                case 21:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0xe6;
                    break;
                case 22:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0xe4;
                    break;
                case 23:
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0xdf;
                    break;
                case 28:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0xe3;
                    break;
                case 29:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0xe1;
                    break;
                case 30:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0xe2;
                    break;
                }
            } else if (terrain >= 0x22 && terrain <= 0x32) {
                switch (terrain) {
                case 0x22:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1e9;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1d9;
                    if (mask == 9)
                        p_terrain_maps[plane][y][x] = 0xc9;
                    break;
                case 0x23:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1ea;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1da;
                    if (mask == 9)
                        p_terrain_maps[plane][y][x] = 0xca;
                    break;
                case 0x24:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1eb;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1db;
                    if (mask == 9)
                        p_terrain_maps[plane][y][x] = 0xcb;
                    break;
                case 0x25:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1ec;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1dc;
                    if (mask == 9)
                        p_terrain_maps[plane][y][x] = 0xcc;
                    break;
                case 0x26:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1ed;
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1dd;
                    if (mask == 6)
                        p_terrain_maps[plane][y][x] = 0xcd;
                    break;
                case 0x27:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1ee;
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1de;
                    if (mask == 6)
                        p_terrain_maps[plane][y][x] = 0xce;
                    break;
                case 0x28:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1ef;
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1df;
                    if (mask == 6)
                        p_terrain_maps[plane][y][x] = 0xcf;
                    break;
                case 0x29:
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1f0;
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1e0;
                    if (mask == 6)
                        p_terrain_maps[plane][y][x] = 0xd0;
                    break;
                case 0x2a:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1e1;
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1f1;
                    if (mask == 3)
                        p_terrain_maps[plane][y][x] = 0xd1;
                    break;
                case 0x2b:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1e2;
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1f2;
                    if (mask == 3)
                        p_terrain_maps[plane][y][x] = 0xd2;
                    break;
                case 0x2c:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1e3;
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1f3;
                    if (mask == 3)
                        p_terrain_maps[plane][y][x] = 0xd3;
                    break;
                case 0x2d:
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = 0x1e4;
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = 0x1f4;
                    if (mask == 3)
                        p_terrain_maps[plane][y][x] = 0xd4;
                    break;
                case 0x2e:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1e5;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1f5;
                    if (mask == 12)
                        p_terrain_maps[plane][y][x] = 0xd5;
                    break;
                case 0x2f:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1e6;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1f6;
                    if (mask == 12)
                        p_terrain_maps[plane][y][x] = 0xd6;
                    break;
                case 0x30:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1e7;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1f7;
                    if (mask == 12)
                        p_terrain_maps[plane][y][x] = 0xd7;
                    break;
                case 0x31:
                    if (mask == 4)
                        p_terrain_maps[plane][y][x] = 0x1e8;
                    if (mask == 8)
                        p_terrain_maps[plane][y][x] = 0x1f8;
                    if (mask == 12)
                        p_terrain_maps[plane][y][x] = 0xd8;
                    break;
                }
            } else {
                /* the straight shores 0x32 .. 0x51 and the corner families 0x52 ..: by the river mask, shore code +
                   0x172 / 0x1a7 / 0x1b7 / 0x1e7 / 0x1f7 / 0x207 picks one of the TERRAIN.LBX river-mouth sets (unnamed
                   pictures); where the mouth needs a second square, the neighbour becomes grassland and the scan steps back */
                if (terrain >= 0x32 && terrain <= 0x41) {
                    if (mask == 1)
                        p_terrain_maps[plane][y][x] = terrain + 0x1e7;
                    else if (mask == 4)
                        p_terrain_maps[plane][y][x] = terrain + 0x1f7;
                    else if (mask == 5) {
                        p_terrain_maps[plane][y][x] = terrain + 0x1e7;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                    }
                }
                if (terrain >= 0x42 && terrain <= 0x51) {
                    if (mask == 2)
                        p_terrain_maps[plane][y][x] = terrain + 0x207;
                    else if (mask == 8)
                        p_terrain_maps[plane][y][x] = terrain + 0x1f7;
                    else if (mask == 10) {
                        p_terrain_maps[plane][y][x] = terrain + 0x1f7;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                    }
                }
                if (terrain == 0x52 || terrain == 0x56 || terrain == 0x5a || terrain == 0x5e) {
                    switch (mask) {
                    case 2:
                        p_terrain_maps[plane][y][x] = terrain + 0x1b7;
                        break;
                    case 4:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        break;
                    case 8:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        break;
                    case 6:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 10:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 12:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 14:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    }
                }
                if (terrain == 0x53 || terrain == 0x57 || terrain == 0x5b || terrain == 0x5f) {
                    switch (mask) {
                    case 1:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        break;
                    case 4:
                        p_terrain_maps[plane][y][x] = terrain + 0x1b7;
                        break;
                    case 8:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        break;
                    case 5:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 9:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                        x -= 2;
                        y--;
                        break;
                    case 12:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 13:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x -= 2;
                        y--;
                        break;
                    }
                }
                if (terrain == 0x54 || terrain == 0x58 || terrain == 0x5c || terrain == 0x60) {
                    switch (mask) {
                    case 1:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        break;
                    case 2:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        break;
                    case 8:
                        p_terrain_maps[plane][y][x] = terrain + 0x1b7;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 9:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                        x -= 2;
                        y--;
                        break;
                    case 10:
                        p_terrain_maps[plane][y][x] = terrain + 0x1b7;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 11:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        p_terrain_maps[plane][y][x - 1] = TILE_GRASSLAND;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x -= 2;
                        y--;
                        break;
                    }
                }
                if (terrain == 0x55 || terrain == 0x59 || terrain == 0x5d || terrain == 0x61) {
                    switch (mask) {
                    case 1:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        break;
                    case 2:
                        p_terrain_maps[plane][y][x] = terrain + 0x172;
                        break;
                    case 4:
                        p_terrain_maps[plane][y][x] = terrain + 0x1b7;
                        break;
                    case 3:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 5:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 6:
                        p_terrain_maps[plane][y][x] = terrain + 0x1b7;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    case 7:
                        p_terrain_maps[plane][y][x] = terrain + 0x1a7;
                        p_terrain_maps[plane][y][x + 1] = TILE_GRASSLAND;
                        p_terrain_maps[plane][y + 1][x] = TILE_GRASSLAND;
                        x--;
                        y--;
                        break;
                    }
                }
            }
        }
    }
}

/* (plane) the neutral towns: landmass_races[1 .. 199] pre-rolled (Arcanus Random(9): barbarian, gnoll, halfling,
   high men, high men, klackon, lizardmen, nomad, orc - the switch has 13 cases, 9 .. 12 dead; Myrror Random(5):
   beastmen, dark elf, draconian, dwarven, troll); up to 15 towns in 1000 tries on land (not 2 .. 0xa1) >= 5 from the
   fortresses, >= 4 from the towns / nodes of the plane, the towers and the lairs of the plane; race the same roll
   (High Elf on a forest square, else High Men) replaced 3 times in 4 by its landmass's; population Random(4) +
   (difficulty + 1) / 3 + 1 (Hard / Impossible 1 in 5: Random(10) + (difficulty + 1) / 3), size population / 4;
   buildings by population (a switch without breaks: 2 barracks, 3 smithy, 4 builders' hall, 5 armory, 7 granary,
   8 stables, 10 walls, 12 fighters' guild, 13 armorers' guild, 14+ shrine), minus the race's forbidden ones and any
   whose prerequisite is missing, replaced ones set to 0; garrison population / 4 (at most 9) of the race's last
   melee unit type buildable here and as many of its last ranged one (none: melee again); the default name.
   QUIRK #412: every Dark Elf unit but the Settlers and the Nightblades has a magic ranged attack, and the
   Nightblades need a Fighters' Guild (population 12+, more than this generator gives: at most 11), so a Dark Elf
   town finds no melee type (0: Unit_Create makes nothing) and gets only the ranged half of its garrison; clarified
   using ReMoM repo.
   Returns 0 when the tries run out (else no value). Form: the unit race tests compare as ints (both sides
   CBW), written here as an (int) cast on the town's race */
int Place_Neutral_Cities(int plane)
{
    int x;
    int y;
    int bad;
    int k;
    int melee;
    int ranged;
    int tries;
    int forest;
    register int j;
    register int n;

    tries = 0;
    for (n = 1; n < 200; n++) {
        if (plane == PLANE_ARCANUS) {
            switch (Random(9) - 1) {
            case 0:
                landmass_races[n] = RACE_BARBARIAN;
                break;
            case 1:
                landmass_races[n] = RACE_GNOLL;
                break;
            case 2:
                landmass_races[n] = RACE_HALFLING;
                break;
            case 3:
                landmass_races[n] = RACE_HIGH_MEN;
                break;
            case 4:
                landmass_races[n] = RACE_HIGH_MEN;
                break;
            case 5:
                landmass_races[n] = RACE_KLACKON;
                break;
            case 6:
                landmass_races[n] = RACE_LIZARDMAN;
                break;
            case 7:
                landmass_races[n] = RACE_NOMAD;
                break;
            case 8:
                landmass_races[n] = RACE_ORC;
                break;
            case 9:
                landmass_races[n] = RACE_BARBARIAN;
                break;
            case 10:
                landmass_races[n] = RACE_HIGH_MEN;
                break;
            case 11:
                landmass_races[n] = RACE_NOMAD;
                break;
            case 12:
                landmass_races[n] = RACE_HIGH_MEN;
                break;
            }
        } else {
            switch (Random(5) - 1) {
            case 0:
                landmass_races[n] = RACE_BEASTMEN;
                break;
            case 1:
                landmass_races[n] = RACE_DARK_ELF;
                break;
            case 2:
                landmass_races[n] = RACE_DRACONIAN;
                break;
            case 3:
                landmass_races[n] = RACE_DWARVEN;
                break;
            case 4:
                landmass_races[n] = RACE_TROLL;
                break;
            }
        }
    }
    for (n = 0; n < 15 && tries < 1000; n++) {
retry:
        tries++;
        x = Random(54) + 3;
        y = Random(34) + 3;
        bad = 0;
        if (p_terrain_maps[plane][y][x] == TILE_OCEAN)
            bad = 1;
        if (p_terrain_maps[plane][y][x] >= TILE_SHORE_FIRST && p_terrain_maps[plane][y][x] <= TILE_SHORE_LAST)
            bad = 1;
        for (j = 0; j < num_players; j++) {
            if (Dist_Max_Plus_Half_Min(x, y, p_fortresses[j].x, p_fortresses[j].y) < 5)
                bad = 1;
        }
        for (j = 0; j < num_cities; j++) {
            if (p_cities[j].plane == plane && Map_Distance(x, y, p_cities[j].x, p_cities[j].y, MAP_WIDTH) < 4)
                bad = 1;
        }
        for (j = 0; j < NUM_NODES; j++) {
            if (p_nodes[j].plane == plane && Dist_Max_Plus_Half_Min(x, y, p_nodes[j].x, p_nodes[j].y) < 4)
                bad = 1;
        }
        for (j = 0; j < NUM_TOWERS; j++) {
            if (Dist_Max_Plus_Half_Min(x, y, p_towers[j].x, p_towers[j].y) < 4)
                bad = 1;
        }
        for (j = 0; j < NUM_LAIRS; j++) {
            if (p_lairs[j].plane == plane && Dist_Max_Plus_Half_Min(x, y, p_lairs[j].x, p_lairs[j].y) < 4)
                bad = 1;
        }
        if (tries < 1000 && bad == 1)
            goto retry;
        if (tries >= 1000)
            return 0;
        forest = Tile_Is_Forest(x, y, plane);
        if (plane == PLANE_ARCANUS) {
            switch (Random(9) - 1) {
            case 0:
                p_cities[num_cities].race = RACE_BARBARIAN;
                break;
            case 1:
                p_cities[num_cities].race = RACE_GNOLL;
                break;
            case 2:
                p_cities[num_cities].race = RACE_HALFLING;
                break;
            case 3:
                if (forest == 1)
                    p_cities[num_cities].race = RACE_HIGH_ELF;
                else
                    p_cities[num_cities].race = RACE_HIGH_MEN;
                break;
            case 4:
                p_cities[num_cities].race = RACE_HIGH_MEN;
                break;
            case 5:
                p_cities[num_cities].race = RACE_KLACKON;
                break;
            case 6:
                p_cities[num_cities].race = RACE_LIZARDMAN;
                break;
            case 7:
                p_cities[num_cities].race = RACE_NOMAD;
                break;
            case 8:
                p_cities[num_cities].race = RACE_ORC;
                break;
            case 9:
                p_cities[num_cities].race = RACE_BARBARIAN;
                break;
            case 10:
                p_cities[num_cities].race = RACE_HIGH_MEN;
                break;
            case 11:
                p_cities[num_cities].race = RACE_NOMAD;
                break;
            case 12:
                p_cities[num_cities].race = RACE_HIGH_MEN;
                break;
            }
        } else {
            switch (Random(5) - 1) {
            case 0:
                p_cities[num_cities].race = RACE_BEASTMEN;
                break;
            case 1:
                p_cities[num_cities].race = RACE_DARK_ELF;
                break;
            case 2:
                p_cities[num_cities].race = RACE_DRACONIAN;
                break;
            case 3:
                p_cities[num_cities].race = RACE_DWARVEN;
                break;
            case 4:
                p_cities[num_cities].race = RACE_TROLL;
                break;
            }
        }
        if (Random(4) > 1)
            p_cities[num_cities].race = landmass_races[p_landmass_ids[plane][y][x]];
        p_cities[num_cities].x = x;
        p_cities[num_cities].y = y;
        p_cities[num_cities].plane = plane;
        p_cities[num_cities].owner = PLAYER_NEUTRAL;
        p_cities[num_cities].population = Random(4) + (difficulty + 1) / 3 + 1;
        if (difficulty > DIFFICULTY_NORMAL && Random(5) == 1)
            p_cities[num_cities].population = Random(10) + (difficulty + 1) / 3;
        p_cities[num_cities].size = p_cities[num_cities].population / 4;
        p_cities[num_cities].building_count = 0;
        p_cities[num_cities].enchants[CITY_ENCH_WALL_OF_DARKNESS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_WALL_OF_FIRE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CHAOS_RIFT] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_DARK_RITUALS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_EVIL_PRESENCE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CURSED_LANDS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_PESTILENCE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CLOUD_OF_SHADOW] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_ALTAR_OF_BATTLE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_FAMINE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_FLYING_FORTRESS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_DEATH_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CHAOS_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_LIFE_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_SORCERY_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_NATURE_WARD] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_NATURES_EYE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_EARTH_GATE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_STREAM_OF_LIFE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_GAIAS_BLESSING] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_INSPIRATIONS] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_PROSPERITY] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_ASTRAL_GATE] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_HEAVENLY_LIGHT] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_CONSECRATION] = 0;
        p_cities[num_cities].enchants[CITY_ENCH_NIGHTSHADE] = 0;
        p_cities[num_cities].producing = PROD_AUTO_CHOOSE;
        for (j = 0; j < NUM_BUILDINGS; j++)
            p_cities[num_cities].buildings[j] = BLDSTATE_NONE;
        p_cities[num_cities].buildings[BLD_NONE] = 0;
        switch (p_cities[num_cities].population) {
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            p_cities[num_cities].buildings[BLD_SHRINE] = BLDSTATE_BUILT;
        case 13:
            p_cities[num_cities].buildings[BLD_ARMORERS_GUILD] = BLDSTATE_BUILT;
        case 12:
            p_cities[num_cities].buildings[BLD_FIGHTERS_GUILD] = BLDSTATE_BUILT;
        case 10:
        case 11:
            p_cities[num_cities].buildings[BLD_CITY_WALLS] = BLDSTATE_BUILT;
        case 8:
        case 9:
            p_cities[num_cities].buildings[BLD_STABLES] = BLDSTATE_BUILT;
        case 7:
            p_cities[num_cities].buildings[BLD_GRANARY] = BLDSTATE_BUILT;
        case 5:
        case 6:
            p_cities[num_cities].buildings[BLD_ARMORY] = BLDSTATE_BUILT;
        case 4:
            p_cities[num_cities].buildings[BLD_BUILDERS_HALL] = BLDSTATE_BUILT;
        case 3:
            p_cities[num_cities].buildings[BLD_SMITHY] = BLDSTATE_BUILT;
        case 2:
            p_cities[num_cities].buildings[BLD_BARRACKS] = BLDSTATE_BUILT;
        }
        for (k = 0; k < race_table[p_cities[num_cities].race].forbidden_count; k++)
            p_cities[num_cities].buildings[race_table[p_cities[num_cities].race].forbidden_buildings[k]] = BLDSTATE_NONE;
        /* BUG #416 (latent: none of the buildings placed above has one): a terrain prereq1 (> BLD_PREREQ_TERRAIN_BASE)
           indexes buildings[] far past its 36 entries */
        for (j = 1; j <= (NUM_BUILDINGS - 1); j++) {
            if (p_cities[num_cities].buildings[j] == BLDSTATE_BUILT) {
                if (p_building_data[j].prereq1 > BLD_PREREQ_TERRAIN_BASE) {
                    if (p_cities[num_cities].buildings[p_building_data[j].prereq1] == BLDSTATE_NONE)
                        p_cities[num_cities].buildings[j] = BLDSTATE_NONE;
                } else {
                    if (p_cities[num_cities].buildings[p_building_data[j].prereq1] == BLDSTATE_NONE ||
                        p_cities[num_cities].buildings[p_building_data[j].prereq2] == BLDSTATE_NONE)
                        p_cities[num_cities].buildings[j] = BLDSTATE_NONE;
                }
            }
        }
        for (j = 1; j <= 35; j++) {
            if (p_building_data[j].replaces > 0 && p_cities[num_cities].buildings[j] == BLDSTATE_BUILT &&
                p_cities[num_cities].buildings[p_building_data[j].replaces] == BLDSTATE_BUILT)
                p_cities[num_cities].buildings[p_building_data[j].replaces] = BLDSTATE_REPLACED;
        }
        melee = 0;
        for (j = 40; j < 154; j++) {            /* the race units 40 .. 153 (Barbarian Spearmen, 39, left out) */
            if (unit_types[j].race == (int)p_cities[num_cities].race &&
                (unit_types[j].req_building_1 == 0 ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_1] == BLDSTATE_BUILT ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_1] == BLDSTATE_REPLACED) &&
                (unit_types[j].req_building_2 == 0 ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_2] == BLDSTATE_BUILT ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_2] == BLDSTATE_REPLACED) &&
                (unit_types[j].ability_flags & AFLAG_CREATE_OUTPOST) == 0 && unit_types[j].construction == 0 &&
                unit_types[j].transport_capacity == 0 &&
                (unit_types[j].ranged_type == RANGED_NONE || unit_types[j].ranged_type >= RANGED_THROWN))
                melee = j;
        }
        ranged = 0;
        for (j = 40; j < 154; j++) {            /* the race units 40 .. 153 (Barbarian Spearmen, 39, left out) */
            if (unit_types[j].race == (int)p_cities[num_cities].race &&
                (unit_types[j].req_building_1 == 0 ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_1] == BLDSTATE_BUILT ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_1] == BLDSTATE_REPLACED) &&
                (unit_types[j].req_building_2 == 0 ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_2] == BLDSTATE_BUILT ||
                 p_cities[num_cities].buildings[unit_types[j].req_building_2] == BLDSTATE_REPLACED) &&
                (unit_types[j].ability_flags & AFLAG_CREATE_OUTPOST) == 0 && unit_types[j].construction == 0 &&
                unit_types[j].transport_capacity == 0 && unit_types[j].ranged_type > 0 &&
                unit_types[j].ranged_type < RANGED_THROWN)
                ranged = j;
        }
        for (j = 0; j < p_cities[num_cities].population / 4 && j < MAX_STACK; j++)
            Unit_Create(melee, PLAYER_NEUTRAL, p_cities[num_cities].x, p_cities[num_cities].y, p_cities[num_cities].plane, num_cities);
        if (ranged == 0) {
            for (j = 0; j < p_cities[num_cities].population / 4 && j < MAX_STACK; j++)
                Unit_Create(melee, PLAYER_NEUTRAL, p_cities[num_cities].x, p_cities[num_cities].y, p_cities[num_cities].plane,
                            num_cities);
        } else {
            for (j = 0; j < p_cities[num_cities].population / 4 && j < MAX_STACK; j++)
                Unit_Create(ranged, PLAYER_NEUTRAL, p_cities[num_cities].x, p_cities[num_cities].y, p_cities[num_cities].plane,
                            num_cities);
        }
        City_Pick_Default_Name(p_cities[num_cities].race, p_cities[num_cities].name);
        num_cities++;
    }
}

/* (race, name) a town name: a random one of the race's 20 (CITYNAME.LBX entry 0, record race) that no town has yet
   (case-insensitive); after 200 tries a clash loads a random race's list instead (Random(14) - 1). far_strcpy is
   called unprototyped here (dst far, then the near src with segment 0 = DS) */
void City_Pick_Default_Name(int race, char far *name)
{
    int tries;
    int pick;
    char existing[20];
    int i;
    char (*names)[CITY_NAME_LEN];

    names = (char (*)[CITY_NAME_LEN])Near_Alloc_Reset(CITY_NAMES_PER_RACE * CITY_NAME_LEN);
    LBX_Load_Records_Near("CITYNAME", 0, names, race, 1, CITY_NAMES_PER_RACE * CITY_NAME_LEN);
    tries = 0;
again:
    tries++;
    pick = Random(CITY_NAMES_PER_RACE) - 1;
    for (i = 0; i < num_cities; i++) {
        far_strcpy(existing, 0, p_cities[i].name);
        if (stricmp(existing, names[pick]) == 0) {
            if (tries < 200)
                goto again;
            else
                LBX_Load_Records_Near("CITYNAME", 0, names, Random(14) - 1, 1, CITY_NAMES_PER_RACE * CITY_NAME_LEN);
        }
    }
    far_strcpy(name, names[pick], 0);
}

/* (plane) roads: the plane's terrain flags cleared; neutral towns of the plane on one landmass less than 11 apart
   (Dist_Max_Plus_Half_Min) are joined (flag 8, Myrror also 0x10) when the straight line (Map_Line_Path) crosses no
   water / shore / river square (2 .. 0xa1, 0, 0xd9 .. 0xe8, 0x1c4 .. 0x259); then flag 8 on every town square (both
   planes, every call). BUG #356: the Myrror 0x10 of the town squares goes to the road loop's leftover x / y */
void Gen_Roads(int plane)
{
    int city_a;
    int city_b;
    int blocked;
    int len;
    int k;
    int x;
    int y;
    int city;
    int ax1;
    int ay1;
    int bx1;
    int by1;
    char line_x[70];
    char line_y[70];

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++)
            p_terrain_flags_maps[plane][y][x] = 0;
    }
    for (city_a = 0; city_a < num_cities; city_a++) {
        ax1 = p_cities[city_a].x;
        ay1 = p_cities[city_a].y;
        for (city_b = city_a + 1; city_b < num_cities; city_b++) {
            bx1 = p_cities[city_b].x;
            by1 = p_cities[city_b].y;
            if (p_cities[city_a].plane == plane && p_cities[city_b].plane == plane && p_cities[city_a].owner == PLAYER_NEUTRAL &&
                p_cities[city_b].owner == PLAYER_NEUTRAL && p_landmass_ids[plane][ay1][ax1] == p_landmass_ids[plane][by1][bx1] &&
                Dist_Max_Plus_Half_Min(ax1, ay1, bx1, by1) < 11) {
                len = Map_Line_Path(ax1, ay1, bx1, by1, line_x, line_y, MAP_WIDTH);
                blocked = 0;
                for (k = 0; k < len - 1 && blocked == 0; k++) {
                    x = line_x[k];
                    y = line_y[k];
                    if (p_terrain_maps[plane][y][x] >= TILE_SHORE_FIRST && p_terrain_maps[plane][y][x] <= TILE_SHORE_LAST)
                        blocked = 1;
                    if (p_terrain_maps[plane][y][x] == TILE_OCEAN)
                        blocked = 1;
                    if (p_terrain_maps[plane][y][x] >= 0xd9 && p_terrain_maps[plane][y][x] <= TILE_SHORE2_LAST)
                        blocked = 1;
                    if (p_terrain_maps[plane][y][x] >= TILE_SHORE3_FIRST && p_terrain_maps[plane][y][x] <= TILE_OCEAN_2)
                        blocked = 1;
                }
                if (blocked == 0) {
                    for (k = 0; k < len - 1; k++) {
                        x = line_x[k];
                        y = line_y[k];
                        p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_ROAD;
                        if (plane == PLANE_MYRROR)
                            p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_ENCHANTED_ROAD;
                    }
                }
            }
        }
    }
    for (city = 0; city < num_cities; city++) {
        p_terrain_flags_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] =
            p_terrain_flags_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] | TERRAIN_FLAG_ROAD;
        if (plane == PLANE_MYRROR)
            p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_ENCHANTED_ROAD;
    }
}

/* (plane) Block_Edge_Bits_Build of movement layer 1 (land) -> p_region_bits_land[plane] and layer 5 (water only) ->
   p_region_bits_water[plane]: block-crossing bits for a coarse pathfinder WIZARDS never uses (only saved / loaded) */
void Build_Region_Bits(int plane)
{
    EMS_Map_Default();
    Block_Edge_Bits_Build(p_movement_cost_maps[plane][MOVE_COST_WALKING], p_region_bits_land[plane]);
    Block_Edge_Bits_Build(p_movement_cost_maps[plane][MOVE_COST_SAILING], p_region_bits_water[plane]);
}

/* (plane) minerals: the plane's mineral and flag maps cleared; one site per 4 x 4 (Arcanus) / 3 x 3 (Myrror) cell at
   cell + Random(2 x cell), skipped when it has a mineral, a lair / tower or a town of the plane. BUG #355: the edge
   test is y <= 2 AND y >= 38 AND x <= 2 AND x >= 58 (never true), so sites reach x 64 / y 45 off the map; a grassland
   site (and half the forest ones) becomes mountain 3 / 7, hills 3 / 7, swamp 1 / 7; then the kind by terrain: forest
   0x40 (wild game), mountain Mineral_Pick_Mountain, hills Mineral_Pick_Hills, swamp 0x80 (nightshade), desert Mineral_Pick_Desert */
void Place_Minerals(int plane)
{
    int k;
    int cx;
    int cy;
    int lair_here;
    int city_here;
    int cell;
    register int x;
    register int y;

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            p_mineral_maps[plane][y][x] = 0;
            p_terrain_flags_maps[plane][y][x] = 0;
        }
    }
    if (plane == PLANE_ARCANUS)
        cell = 4;
    else
        cell = 3;
    for (cy = 0; cy < MAP_HEIGHT; cy += cell) {
        for (cx = 0; cx < MAP_WIDTH; cx += cell) {
            y = cy + Random(cell * 2);
            x = cx + Random(cell * 2);
            city_here = 0;
            lair_here = 0;
            for (k = 0; k < NUM_LAIRS; k++) {
                if (p_lairs[k].plane == plane && p_lairs[k].x == x && p_lairs[k].y == y)
                    lair_here = 1;
            }
            for (k = 0; k < NUM_TOWERS; k++) {
                if (p_towers[k].x == x && p_towers[k].y == y)
                    lair_here = 1;
            }
            for (k = 0; k < num_cities; k++) {
                if (p_cities[k].plane == plane && p_cities[k].x == x && p_cities[k].y == y)
                    city_here = 1;
            }
            if (p_mineral_maps[plane][y][x] != 0)
                continue;
            if (city_here != 0)
                continue;
            if (lair_here != 0)
                continue;
            if (y <= 2 && y >= MAP_HEIGHT - 2 && x <= 2 && x >= MAP_WIDTH - 2)
                continue;
            if (Tile_Is_Grassland_Gen(x, y, plane) == 1 || (Tile_Is_Forest(x, y, plane) == 1 && Random(2) == 1)) {
                switch (Random(7) - 1) {
                case 0:
                    p_terrain_maps[plane][y][x] = TILE_MOUNTAIN;
                    break;
                case 1:
                    p_terrain_maps[plane][y][x] = TILE_MOUNTAIN;
                    break;
                case 2:
                    p_terrain_maps[plane][y][x] = TILE_MOUNTAIN;
                    break;
                case 3:
                    p_terrain_maps[plane][y][x] = TILE_HILLS;
                    break;
                case 4:
                    p_terrain_maps[plane][y][x] = TILE_HILLS;
                    break;
                case 5:
                    p_terrain_maps[plane][y][x] = TILE_HILLS;
                    break;
                case 6:
                    p_terrain_maps[plane][y][x] = TILE_SWAMP;
                    break;
                }
            }
            if (Tile_Is_Forest(x, y, plane) == 1)
                p_mineral_maps[plane][y][x] = MINFLAG_WILD_GAME;
            if (Tile_Is_Mountain_Gen(x, y, plane) == 1)
                p_mineral_maps[plane][y][x] = Mineral_Pick_Mountain(plane);
            if (Tile_Is_Hills_Gen(x, y, plane) == 1)
                p_mineral_maps[plane][y][x] = Mineral_Pick_Hills(plane);
            if (Tile_Is_Swamp_Gen(x, y, plane) == 1)
                p_mineral_maps[plane][y][x] = MINFLAG_NIGHTSHADE;
            if (Tile_Is_Desert_Range(x, y, plane) == 1)
                p_mineral_maps[plane][y][x] = Mineral_Pick_Desert(plane);
        }
    }
}

/* (plane) a desert mineral: Arcanus Random(9), Myrror Random(5) + 5 -> gems 5 (1 .. 6), quork 8 (7 .. 9), crysx 9 (10) */
int Mineral_Pick_Desert(int plane)
{
    int mineral;
    int roll;

    mineral = 0;
    if (plane == PLANE_ARCANUS)
        roll = Random(9);
    else
        roll = Random(5) + 5;
    switch (roll) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        mineral = MINERAL_GEMS;
        break;
    case 7:
    case 8:
    case 9:
        mineral = MINERAL_QUORK;
        break;
    case 10:
        mineral = MINERAL_CRYSX;
        break;
    }
    return mineral;
}

/* (plane) a hills mineral (MINERAL_*): Arcanus Random(18) - iron 6, silver 4, coal 3, gold 4, mithril 1 in 18; Myrror
   Random(10) + 10 - iron, coal, silver 1 each, gold 4, mithril 2, adamantium 1 in 10 */
int Mineral_Pick_Hills(int plane)
{
    int mineral;
    int roll;

    mineral = 0;
    if (plane == PLANE_ARCANUS)
        roll = Random(18);
    else
        roll = Random(10) + 10;
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
        mineral = MINERAL_SILVER;
        break;
    case 9:
    case 10:
        mineral = MINERAL_COAL;
        break;
    case 11:
        mineral = MINERAL_IRON_ORE;
        break;
    case 12:
        mineral = MINERAL_COAL;
        break;
    case 13:
        mineral = MINERAL_SILVER;
        break;
    case 14:
    case 15:
    case 16:
    case 17:
        mineral = MINERAL_GOLD;
        break;
    case 18:
    case 19:
        mineral = MINERAL_MITHRIL;
        break;
    case 20:
        mineral = MINERAL_ADAMANTIUM;
        break;
    }
    return mineral;
}

/* (plane) a mountain mineral (MINERAL_*): Arcanus Random(18) - iron 4, silver 3, coal 5, gold 3, mithril 3 in 18;
   Myrror Random(10) + 10 - iron, silver, coal 1 each, gold 2, mithril 3, adamantium 2 in 10 */
int Mineral_Pick_Mountain(int plane)
{
    int mineral;
    int roll;

    mineral = 0;
    if (plane == PLANE_ARCANUS)
        roll = Random(18);
    else
        roll = Random(10) + 10;
    switch (roll) {
    case 1:
    case 2:
    case 3:
        mineral = MINERAL_IRON_ORE;
        break;
    case 4:
    case 5:
        mineral = MINERAL_SILVER;
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        mineral = MINERAL_COAL;
        break;
    case 10:
        mineral = MINERAL_GOLD;
        break;
    case 11:
        mineral = MINERAL_IRON_ORE;
        break;
    case 12:
        mineral = MINERAL_SILVER;
        break;
    case 13:
        mineral = MINERAL_COAL;
        break;
    case 14:
    case 15:
        mineral = MINERAL_GOLD;
        break;
    case 16:
    case 17:
    case 18:
        mineral = MINERAL_MITHRIL;
        break;
    case 19:
    case 20:
        mineral = MINERAL_ADAMANTIUM;
        break;
    }
    return mineral;
}

/* () exploration: both planes unexplored, then the 5 x 5 around the human wizard's fortress revealed (the inner
   3 x 3 fully, then its corner squares without their outer corner quarter; the outer ring gets the quarters that
   face the fortress - EXPLORED_SW / NW / NE / SE = the square's corner quarters 1 / 2 / 4 / 8) */
void Explore_Start(void)
{
    int plane;
    int dx;
    register int x;
    register int y;

    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            for (x = 0; x < MAP_WIDTH; x++)
                p_exploration_maps[plane][y][x] = 0;
        }
    }
    plane = p_fortresses[PLAYER_HUMAN].plane;
    x = p_fortresses[PLAYER_HUMAN].x;
    y = p_fortresses[PLAYER_HUMAN].y;
    for (dx = -1; dx <= 1; dx++)
        Explore_Mark(plane, x + dx, y, EXPLORED_ALL);
    for (dx = -1; dx <= 1; dx++)
        Explore_Mark(plane, x + dx, y - 1, EXPLORED_ALL);
    for (dx = -1; dx <= 1; dx++)
        Explore_Mark(plane, x + dx, y + 1, EXPLORED_ALL);
    Explore_Mark(plane, x - 1, y - 2, EXPLORED_SE);
    Explore_Mark(plane, x, y - 2, EXPLORED_SE | EXPLORED_SW);
    Explore_Mark(plane, x + 1, y - 2, EXPLORED_SW);
    Explore_Mark(plane, x - 1, y + 2, EXPLORED_NE);
    Explore_Mark(plane, x, y + 2, EXPLORED_NE | EXPLORED_NW);
    Explore_Mark(plane, x + 1, y + 2, EXPLORED_NW);
    Explore_Mark(plane, x - 2, y - 1, EXPLORED_SE);
    Explore_Mark(plane, x - 2, y, EXPLORED_SE | EXPLORED_NE);
    Explore_Mark(plane, x - 2, y + 1, EXPLORED_NE);
    Explore_Mark(plane, x + 2, y - 1, EXPLORED_SW);
    Explore_Mark(plane, x + 2, y + 1, EXPLORED_NW);
    Explore_Mark(plane, x + 2, y, EXPLORED_NW | EXPLORED_SW);
    Explore_Mark(plane, x - 1, y - 1, EXPLORED_SE | EXPLORED_NE | EXPLORED_SW);
    Explore_Mark(plane, x - 1, y + 1, EXPLORED_SE | EXPLORED_NE | EXPLORED_NW);
    Explore_Mark(plane, x + 1, y + 1, EXPLORED_NE | EXPLORED_NW | EXPLORED_SW);
    Explore_Mark(plane, x + 1, y - 1, EXPLORED_SE | EXPLORED_NW | EXPLORED_SW);
}

/* (plane, x, y, bits) one square's exploration bits, ignored off the map */
void Explore_Mark(int plane, int x, int y, int bits)
{
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
        p_exploration_maps[plane][y][x] = bits;
}

/* (plane) the 6 movement-cost layers from the terrain's TERRSTAT costs; a road square (flag 8) costs 1 in layers
   0 .. 4 on Arcanus (enchanted roads on Myrror: 0) and 0xff in layer 5 */
void Build_Movement_Maps(int plane)
{
    int x;
    int y;
    int terrain;

    EMS_Map_Default();
    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            terrain = p_terrain_maps[plane][y][x];
            p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] = p_terrain_stats[terrain][MOVE_COST_WALKING];
            p_movement_cost_maps[plane][MOVE_COST_CAVALRY][y][x] = p_terrain_stats[terrain][MOVE_COST_CAVALRY];
            p_movement_cost_maps[plane][MOVE_COST_FORESTER][y][x] = p_terrain_stats[terrain][MOVE_COST_FORESTER];
            p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER][y][x] = p_terrain_stats[terrain][MOVE_COST_MOUNTAINEER];
            p_movement_cost_maps[plane][MOVE_COST_SWIMMING][y][x] = p_terrain_stats[terrain][MOVE_COST_SWIMMING];
            p_movement_cost_maps[plane][MOVE_COST_SAILING][y][x] = p_terrain_stats[terrain][MOVE_COST_SAILING];
            if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_ROAD) {
                if (plane == PLANE_ARCANUS) {
                    p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] = 1;
                    p_movement_cost_maps[plane][MOVE_COST_CAVALRY][y][x] = 1;
                    p_movement_cost_maps[plane][MOVE_COST_FORESTER][y][x] = 1;
                    p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER][y][x] = 1;
                    p_movement_cost_maps[plane][MOVE_COST_SWIMMING][y][x] = 1;
                } else {
                    p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] = 0;
                    p_movement_cost_maps[plane][MOVE_COST_CAVALRY][y][x] = 0;
                    p_movement_cost_maps[plane][MOVE_COST_FORESTER][y][x] = 0;
                    p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER][y][x] = 0;
                    p_movement_cost_maps[plane][MOVE_COST_SWIMMING][y][x] = 0;
                }
                p_movement_cost_maps[plane][MOVE_COST_SAILING][y][x] = COST_IMPASSABLE;
            }
        }
    }
}

/* (x, y, plane) 1 when the square is a shore picture (terrain mod 0x2fa in 0x62 .. 0xa1) */
int Tile_Is_Shore_High(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain >= 0x62 && terrain <= TILE_SHORE_LAST)   /* the second half of the shore band */
        return 1;
    return 0;
}

/* (percent) the "Building The Worlds..." bar: x 90 .. 90 + 136 x percent / 100, y 89 .. 94, color 0xac; 0 does
   nothing (returns 0, else no value) */
int Progress_Bar(int percent)
{
    int width;

    Set_Draw_Page();
    if (percent == 0)
        return 0;
    if (percent >= 100)
        Fill_Rect(90, 89, 225, 94, 0xac);
    else {
        width = percent * 136 / 100;
        Fill_Rect(90, 89, width + 90, 94, 0xac);
    }
    UI_Present_Frame();
}

/* (unit_type, owner, x, y, plane, city) MAGIC's own unit constructor for the starting units and the neutral
   towns' garrisons (Place_Neutral_Cities): the record from unit_types
   (moves, scouting), no hero slot, no road; Alchemy -> mutation_flags 1; city >= 0: Fighters' Guild level 1 / War
   College level 2 (experience from exp_level_thresholds), Warlord +1 level. BUG #361 (LATENT): the town it reads is an
   uninitialised local, not the city parameter. Type 0 returns 0 (else no value) */
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int city)
{
    int town;

    if (unit_type == 0)
        return 0;
    p_units[num_units].x = x;
    p_units[num_units].y = y;
    p_units[num_units].plane = plane;
    p_units[num_units].owner = owner;
    p_units[num_units].moves_max = unit_types[unit_type].moves;
    p_units[num_units].type = unit_type;
    p_units[num_units].hero_slot = -1;
    p_units[num_units].finished = 0;
    p_units[num_units].moves_left = unit_types[unit_type].moves;
    p_units[num_units].scout_range = unit_types[unit_type].scouting;
    p_units[num_units].dest_x = 0;
    p_units[num_units].dest_y = 0;
    p_units[num_units].order = ORDER_READY;
    p_units[num_units].level = 0;
    p_units[num_units].experience = 0;
    p_units[num_units].damage = 0;
    p_units[num_units].draw_priority = 0;
    p_units[num_units].enchantments = 0;
    p_units[num_units].goto_blocked = 0;
    p_units[num_units].road_turns = -1;
    p_units[num_units].mutation_flags = WEAPON_NORMAL;
    if (wizards[owner].retorts[RETORT_ALCHEMY] != 0)
        p_units[num_units].mutation_flags = WEAPON_MAGIC;
    if (city >= 0) {
        if (p_cities[town].buildings[BLD_FIGHTERS_GUILD] > 0) {
            p_units[num_units].experience = exp_level_thresholds[1];
            p_units[num_units].level = 1;
        }
        if (p_cities[town].buildings[BLD_WAR_COLLEGE] > 0) {
            p_units[num_units].experience = exp_level_thresholds[2];
            p_units[num_units].level = 2;
        }
        if (p_units[num_units].level > 3) {     /* never true: the guilds give at most 2 */
            p_units[num_units].experience = exp_level_thresholds[3];
            p_units[num_units].level = 3;
        }
        if (wizards[owner].retorts[RETORT_WARLORD] > 0)
            p_units[num_units].level = p_units[num_units].level + 1;
    }
    num_units++;
}

/* () plain ocean 0 becomes the other ocean picture 0x259 1 time in 5 (0x2fa -> 0x553 is dead: no square holds 0x2fa).
   BUG #366: y 0 .. 40 and x 0 .. 60, one row and column past the map */
void Gen_Ocean_Variety(void)
{
    int x;
    register int plane;
    register int y;

    for (plane = 0; plane < NUM_PLANES; plane++) {
        for (y = 0; y <= MAP_HEIGHT; y++) {
            for (x = 0; x <= MAP_WIDTH; x++) {
                if (p_terrain_maps[plane][y][x] == TILE_OCEAN && Random(5) == 1)
                    p_terrain_maps[plane][y][x] = TILE_OCEAN_2;
                if (p_terrain_maps[plane][y][x] == TILE_OCEAN + TERRAIN_IDS_PER_PLANE && Random(5) == 1)
                    p_terrain_maps[plane][y][x] = TILE_OCEAN_2 + TERRAIN_IDS_PER_PLANE;
            }
        }
    }
}

/* (x, y, plane) 1 for water (terrain mod 0x2fa: ocean and shores 0, 2 .. 0xa1 and the shore variants 0xc5 .. 0xe8,
   0x1c4 .. 0x1d3, 0x1d9 .. 0x259), 0 for terrain 1 and land */
int Tile_Is_Water_Gen(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_PLACEHOLDER_LAND)
        return 0;
    if (terrain < TILE_LAND_FIRST)
        return 1;
    if (terrain > TILE_SHORE2_FIRST - 1 && terrain < TILE_SHORE2_LAST + 1)
        return 1;
    if (terrain > TILE_SHORE3_FIRST - 1 && terrain < TILE_SHORE3_LAST + 1)
        return 1;
    if (terrain > TILE_SHORE4_FIRST - 1 && terrain < TILE_TUNDRA_RANGE_FIRST)
        return 1;
    return 0;
}

/* (x, y, plane) 1 when one of the 30 nodes is on that square */
int Is_Node_At(int x, int y, int plane)
{
    int i;

    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].x == x && p_nodes[i].y == y && p_nodes[i].plane == plane)
            return 1;
    }
    return 0;
}

/* (x, y) 1 when one of the 6 towers is on that square (either plane) */
int Tower_At(int x, int y)
{
    int i;

    for (i = 0; i < NUM_TOWERS; i++) {
        if (p_towers[i].x == x && p_towers[i].y == y)
            return 1;
    }
    return 0;
}

/* (x, y, plane) forest 0xa3 / 0xb7 / 0xb8 (terrain mod 0x2fa) */
int Tile_Is_Forest(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_FOREST || terrain == TILE_FOREST_2 || terrain == TILE_FOREST_3)
        return 1;
    return 0;
}

/* (x, y, plane) a river: terrain mod 0x2fa in 0xb9 .. 0xc5, 0xe9 .. 0x102, 0x1d4 .. 0x1d8. The test for Gen_River's
   marker 1000 is dead (the value was taken mod 762); the marker passes because 1000 % 762 = 0xee is in 0xe9 .. 0x102 */
int Tile_Is_River_Gen(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain > TILE_SINGLE_LAST && terrain < TILE_SHORE2_FIRST + 1)   /* TILE_RIVER_* and one more: SHORE2_FIRST is water
                                                   in Tile_Is_Water_Gen too */
        return 1;
    if (terrain > TILE_RIVER2_FIRST - 1 && terrain < TILE_RIVER2_LAST + 1)
        return 1;
    if (terrain > TILE_RIVER3_FIRST - 1 && terrain < TILE_RIVER3_LAST + 1)
        return 1;
    if (terrain == TILE_RIVER_MARKER)
        return 1;
    return 0;
}

/* (x, y, plane) a square of the later shore tile sets: terrain mod 0x2fa in 0xc9 .. 0xe8, 0x1c4 .. 0x1d3,
   0x1d9 .. 0x258 (ReMoM's tt_Shore2F / tt_Shore2 / tt_Shore3 - its MAPGEN.c calls this function
   Square_Is_River_Mouth_NewGame; the Surveyor shows these squares as Shore; clarified using ReMoM repo) */
int Tile_Is_Shore_Variant(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain > 0xc8 && terrain < 0xe9)
        return 1;
    if (terrain > 0x1c3 && terrain < 0x1d4)
        return 1;
    if (terrain > 0x1d8 && terrain < 0x259)
        return 1;
    return 0;
}

/* (x, y, plane) any shore: plain 2 .. 0xa1 or a Tile_Is_Shore_Variant range */
int Tile_Is_Shore_Any(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain > TILE_SHORE_FIRST - 1 && terrain < TILE_SHORE_LAST + 1)
        return 1;
    if (terrain > 0xc8 && terrain < 0xe9)
        return 1;
    if (terrain > 0x1c3 && terrain < 0x1d4)
        return 1;
    if (terrain > 0x1d8 && terrain < 0x259)
        return 1;
    return 0;
}

/* (x, y, plane) mountain 0xa4 or a mountain range picture 0x103 .. 0x112 */
int Tile_Is_Mountain_Gen(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_MOUNTAIN || (terrain > TILE_MOUNTAIN_RANGE_FIRST - 1 && terrain < TILE_MOUNTAIN_RANGE_LAST + 1))
        return 1;
    return 0;
}

/* (x, y, plane) hills 0xab or a hills range picture 0x113 .. 0x123 */
int Tile_Is_Hills_Gen(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_HILLS || (terrain > TILE_HILLS_RANGE_FIRST - 1 && terrain < TILE_HILLS_RANGE_LAST + 1))
        return 1;
    return 0;
}

/* (x, y, plane) swamp 0xa6 / 0xb1 / 0xb2 */
int Tile_Is_Swamp_Gen(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_SWAMP_2 || terrain == TILE_SWAMP_3 || terrain == TILE_SWAMP)
        return 1;
    return 0;
}

/* (x, y, plane) desert 0xa5 / 0xae / 0xaf / 0xb0 or a desert edge picture 0x124 .. 0x1c3 */
int Tile_Is_Desert_Range(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_DESERT || terrain == TILE_DESERT_2 || terrain == TILE_DESERT_3 || terrain == TILE_DESERT_4 || (terrain > TILE_DESERT_RANGE_FIRST - 1 && terrain < TILE_DESERT_RANGE_LAST + 1))
        return 1;
    return 0;
}

/* (x, y, plane) grassland 0xa2 / 0xac / 0xad / 0xb4 or terrain 1 */
int Tile_Is_Grassland_Gen(int x, int y, int plane)
{
    int terrain;

    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_GRASSLAND || terrain == TILE_GRASSLAND_2 || terrain == TILE_GRASSLAND_3 || terrain == TILE_GRASSLAND_4 || terrain == TILE_PLACEHOLDER_LAND)
        return 1;
    return 0;
}

/* (x, y, plane) twice the food of one square, for the fortress sites: ocean 0 and 0x259+ none; terrain 1 3; water
   and shores 1; rivers 4; desert / mountain range pictures none, hills range 1; river-like 0xb9 .. 0x102 4; then by
   base terrain: grassland 3, forest 1, hills 1, Sorcery / Nature nodes 4, mountains / deserts / swamps / tundra /
   Chaos nodes 0 (WIZARDS.EXE's Terrain_Food gives the Nature node 5; clarified using ReMoM repo) */
int Tile_Food_Value(int x, int y, int plane)
{
    int food;
    int terrain;

    food = 0;
    terrain = p_terrain_maps[plane][y][x];
    if (terrain == TILE_OCEAN)
        return 0;
    if (terrain >= 0x259)
        return 0;
    else if (terrain == TILE_PLACEHOLDER_LAND)
        food = 3;
    else if (terrain < 0xa2)
        food = 1;
    else if (terrain > 0x1d8)
        food = 1;
    else if (terrain > 0x1d3)
        food = 4;
    else if (terrain > 0x1c3)
        food = 1;
    else if (terrain > 0x123)
        return 0;
    else if (terrain > 0x112)
        food = 1;
    else if (terrain > 0x102)
        return 0;
    else if (terrain > 0xb8)
        food = 4;
    else {
        switch (terrain) {
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
            food = 4;
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
    return food;
}

/* (x, y, plane) the food of a site's catchment: the Tile_Food_Value sum / 2 */
int Site_Food_Value(int x, int y, int plane)
{
    int count;
    int xs[25];
    int ys[25];
    int total;
    int i;

    count = City_Catchment_List(x, y, plane, xs, ys);
    total = 0;
    for (i = 0; i < count; i++)
        total += Tile_Food_Value(xs[i], ys[i], plane);
    return total / 2;
}

/* (x, y, plane, xs, ys) the catchment squares of a town at x, y: the 5 x 5 without its corners, rows inside the map,
   columns wrapping; returns the count */
int City_Catchment_List(int x, int y, int plane, int *xs, int *ys)
{
    int from;
    int to;
    int ny;
    int count;
    int dx;
    int dy;
    int nx;

    count = 0;
    for (dy = -2; dy <= 2; dy++) {
        ny = y + dy;
        if (ny >= 0 && ny < MAP_HEIGHT) {
            from = -2;
            to = 2;
            if (dy == -2 || dy == 2) {
                from = -1;
                to = 1;
            }
            for (dx = from; dx <= to; dx++) {
                nx = x + dx;
                if (nx < 0)
                    nx += MAP_WIDTH;
                if (nx >= MAP_WIDTH)
                    nx -= MAP_WIDTH;
                xs[count] = nx;
                ys[count] = ny;
                count++;
            }
        }
    }
    return count;
}

/* () empty */
void Gen_Uncalled_Nop(void)
{
}
