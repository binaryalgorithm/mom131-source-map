/* CITYSCAP.C - WIZARDS.EXE overlay code segment cd8b (stub 260e): the cityscape of the city screens - the layout
   grid of buildings and specials, the roads between the blocks, the objects with their fade-in and animation, the
   wards and walls, the background by terrain and plane, the building hotspots, and the outpost scene.
   Data DS:6ed0..6f5f (animation counters, the reveal level, the block column / row positions, the hotspot shapes),
   BSS DS:ca78..ca89.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS cd8b src/wizards/CITYSCAP.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Cityscape_Reveal_Reset(void);
void Cityscape_Draw(int city, int x, int y, int new_obj, int fade_obj);
void Cityscape_Draw_Roads(int x, int y);
void Cityscape_Erase_Unused_Roads(unsigned bitmap);
void Pic_Cut_Capsule12(int x, int y, unsigned bitmap);
void Pic_Cut_Capsule10(int x, int y, unsigned bitmap);
void Pic_Cut_RoadDiag(int x, int y, unsigned bitmap);
void Cityscape_Draw_Objects(int city, int x, int y, int new_obj);
void Cityscape_Layout(int city, int new_obj);
void Cityscape_Draw_Wards_Walls(int city, int x, int y);
void Cityscape_Draw_Background(int city, int x, int y, int map_x, int map_y, int plane);
void Cityscape_Add_Hotspot(int x, int y, int bld, int shape);
void Outpost_Draw_Scene(int city, int x, int y);
void Outpost_Draw_Landscape(int city, int x, int y);

#include "GAME.H"

int cityscape_anim_frame = 0;                   /* DS:6ed0  0..8 (outposts: 0..9) */
#define CITYSCAPE_SEED_SALT 0x2895             /* added to map x * y: the layout's random seed */
int cityscape_anim_tick = 0;                    /* DS:6ed2  0..11 */
int cityscape_wall_anim_frame = 0;              /* DS:6ed4  0..3 */
int cityscape_reveal = 100;                     /* DS:6ed6  0 -> 100 in steps of 10: the fade-in */
int cityscape_col_pos[4] = { 4, 9, 13, 18 };    /* DS:6ed8  grid columns of the vertical roads */
int cityscape_row_pos[4] = { 0, 5, 9, 14 };     /* DS:6ee0  grid rows of the horizontal roads */
int cityscape_hotspot_shapes[5][3][4] = {       /* DS:6ee8  per shape three rectangles (x1, y1, x2, y2) */
    { { 0, -12, 18, 0 }, { 5, -20, 24, -13 }, { 19, -13, 24, -7 } },
    { { 0, -14, 29, 0 }, { 4, -17, 31, -4 }, { 7, -21, 35, -6 } },
    { { 0, -15, 19, 0 }, { 6, -20, 23, -6 }, { 10, -25, 30, -11 } },
    { { 0, -14, 29, 0 }, { 6, -19, 34, -5 }, { 11, -25, 39, -11 } },
    { { 6, -41, 30, -3 }, { 11, -47, 30, -42 }, { 25, -41, 30, -9 } }
};

unsigned cityscape_bitmap;                      /* DS:ca88  0x4e2 paragraphs: the roads are built in it */
int cityscape_has_fortress;                     /* DS:ca86 */
int cityscape_has_summon_circle;                /* DS:ca84 */
int cityscape_ground_frame;                     /* DS:ca82  roads / background frame by the city's state */
unsigned cityscape_hmask_pic;                   /* DS:ca80  CITYSCAP 0x6d: the capsule a road row is cut with */
unsigned cityscape_vmask_pic;                   /* DS:ca7e  CITYSCAP 0x6e: the slant of a road column */
                                                /* hmask: decoded into p_pic_work by every Cityscape_Draw_Roads, unused */
                                                /* vmask: unused; the Pic_Cut_* functions draw the shapes themselves */
int (far *p_cityscape_grid)[23];                /* DS:ca7a  15 rows x 23 cells: objects (> 0), roads (-5) */
int cityscape_fade_obj;                         /* DS:ca78  the object being faded in */

extern struct RaceInfo race_table[];            /* DS:1d86 */
extern struct CityscapeHotspot far *p_cityscape_hotspots;  /* DS:92b0 */
extern int cityscape_hotspot_count;             /* DS:92b4 */
extern unsigned building_pics[];                /* DS:92b8  CITYSCAP pictures by BuildingId */
extern unsigned cityscape_fortress_pic;         /* DS:9308 */
extern unsigned cityscape_city_walls_pic;       /* DS:930a */
extern unsigned cityscape_altar_pic;            /* DS:930c */
extern unsigned cityscape_evil_presence_pic;    /* DS:930e */
extern unsigned cityscape_dark_rituals_pic;     /* DS:9310 */
extern unsigned cityscape_astral_gate_pic;      /* DS:9312 */
extern unsigned cityscape_stream_of_life_pic;   /* DS:9314 */
extern unsigned cityscape_earth_gate_pic;       /* DS:9316 */
extern unsigned cityscape_summon_circle_pic;    /* DS:9318 */
extern unsigned cityscape_rocks_pics[];         /* DS:931a */
extern unsigned cityscape_tree_pics[];          /* DS:9320 */
extern unsigned cityscape_house_pics[][5];      /* DS:9326  by house style */
extern unsigned cityscape_roads_pic;            /* DS:9344 */
extern unsigned cityscape_wall_of_darkness_pic;      /* DS:9346  drawn for Wall of Darkness */
extern unsigned cityscape_wall_of_fire_pic;     /* DS:934a */
extern unsigned cityscape_consecration_pic;     /* DS:934c */
extern unsigned cityscape_prosperity_pic;       /* DS:934e */
extern unsigned cityscape_inspirations_pic;     /* DS:9350 */
extern unsigned cityscape_natures_eye_pic;      /* DS:9352 */
extern unsigned cityscape_sorcery_ward_pic;     /* DS:9354 */
extern unsigned cityscape_life_ward_pic;        /* DS:9356 */
extern unsigned cityscape_nature_ward_pic;      /* DS:9358 */
extern unsigned cityscape_chaos_ward_pic;       /* DS:935a */
extern unsigned cityscape_death_ward_pic;       /* DS:935c */
extern unsigned cityscape_famine_pic;           /* DS:935e */
extern unsigned cityscape_far_chaos_rift_pic;   /* DS:9360 */
extern unsigned cityscape_far_heavenly_light_pic;        /* DS:9362 */
extern unsigned cityscape_far_dark_cloud_pic;   /* DS:9364 */
extern unsigned cityscape_mountain_bg_pic;       /* DS:9366 */
extern unsigned cityscape_hills_bg_pic;         /* DS:9368 */
extern unsigned cityscape_ocean_pic;            /* DS:936a */
extern unsigned cityscape_river_pic;            /* DS:936c */
extern unsigned cityscape_cursed_land_pic;      /* DS:936e */
extern unsigned cityscape_gaia_pic;             /* DS:9372 */
extern unsigned cityscape_flying_fortress_pic;  /* DS:9374 */
extern unsigned cityscape_arcanus_ground_pic;              /* DS:9376 */
extern unsigned cityscape_myrror_river_pic;      /* DS:9378 */
extern unsigned cityscape_myrror_ocean_pic;      /* DS:937a */
extern unsigned cityscape_plain_bg_pic;         /* DS:937c */
extern unsigned cityscape_myrror_chaos_rift_pic;       /* DS:937e */
extern unsigned cityscape_myrror_dark_cloud_pic; /* DS:9380 */
extern unsigned cityscape_myrror_heavenly_light_pic;      /* DS:9382 */
extern unsigned cityscape_myrror_plains_pic;     /* DS:9384 */
extern unsigned cityscape_myrror_mountain_pic;   /* DS:9386 */
extern unsigned cityscape_myrror_hills_pic;      /* DS:9388 */
extern unsigned cityscape_myrror_ground_pic;     /* DS:938a */
extern unsigned cityscape_rubble_pics[];        /* DS:943a */
extern unsigned p_pic_work;                     /* DS:998c */
extern unsigned p_scratch;                      /* DS:998e */
extern struct Fortress far *p_fortresses;       /* DS:9cc8 */
extern int num_players;                         /* DS:bd9c */

void Remap_Build_Range(int first, int last);
long Random_Get_Seed(void);
void Random_Seed(long seed);
void Heap_Mark(unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Release(unsigned block);
void far *Seg_To_Far_Ptr(unsigned seg);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Reset_Frame();   /* unprototyped: Cityscape_Draw_Roads and the Myrror ocean of
                             Cityscape_Draw_Background pass (pic, 0) - it takes the picture only */
void Draw_Bitmap(int x, int y, unsigned pic);
unsigned Far_Peek_Word(unsigned seg, unsigned off);
int Random(int n);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Pic_Set_Remap_Flag(unsigned pic);
void Pic_Dissolve(unsigned pic, int amount);
int Tile_Is_Forest_Explored(int x, int y, int plane);
int Tile_Is_Mountain(int x, int y, int plane);
int Tile_Is_Hills(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Sea(int x, int y, int plane);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Reset_Clip_Rect(void);
int Draw_Pic_Clipped();   /* unprototyped: Outpost_Draw_Landscape passes a 4th argument 0 for Famine and Cursed Lands -
                             it takes (x, y, pic) */

/* resets the fade-in of a newly built object */
void Cityscape_Reveal_Reset(void)
{
    cityscape_reveal = 0;
}

/* (city, x, y, new_obj, fade_obj) draws the city's cityscape at (x, y): the ground frame from the city's state
   (Flying Fortress 1, Cursed Lands 0, Famine 2, Gaia's Blessing 3, else 4), the random generator seeded from the
   map square, then background, layout, roads, objects and wards; advances the animation and the fade-in */
void Cityscape_Draw(int city, int x, int y, int new_obj, int fade_obj)
{
    int map_x;
    int map_y;
    int plane;
    int player;
    long seed;

    Remap_Build_Range(7, 7);
    cityscape_fade_obj = fade_obj;
    if (p_cities[city].enchants[CITY_ENCH_FLYING_FORTRESS])
        cityscape_ground_frame = 1;
    else if (p_cities[city].enchants[CITY_ENCH_CURSED_LANDS])
        cityscape_ground_frame = 0;
    else if (p_cities[city].enchants[CITY_ENCH_FAMINE])
        cityscape_ground_frame = 2;
    else if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING])
        cityscape_ground_frame = 3;
    else
        cityscape_ground_frame = 4;
    if (new_obj < BLD_BARRACKS)                 /* only Barracks .. Miners' Guild and the SCAPE_* specials fade in */
        new_obj = 0;
    if (new_obj > BLD_MINERS_GUILD && new_obj < SCAPE_SUMMONING_CIRCLE)
        new_obj = 0;
    seed = Random_Get_Seed();
    Heap_Mark(p_scratch);
    cityscape_bitmap = Heap_Alloc_Back(p_scratch, 0x4e2);
    p_cityscape_hotspots = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 0xb0));
    p_cityscape_grid = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 0x2c));
    map_x = p_cities[city].x;
    map_y = p_cities[city].y;
    plane = p_cities[city].plane;
    Random_Seed((long)map_x * map_y + CITYSCAPE_SEED_SALT);
    cityscape_has_fortress = 0;
    cityscape_has_summon_circle = 0;
    for (player = 0; player < num_players; player++) {
        if (p_fortresses[player].active == 1) {
            if (p_fortresses[player].x == map_x && p_fortresses[player].y == map_y &&
                p_fortresses[player].plane == plane)
                cityscape_has_fortress = 1;
            if (p_cities[city].x == wizards[player].summoning_circle_xyp[XYP_X] &&
                p_cities[city].y == wizards[player].summoning_circle_xyp[XYP_Y] &&
                p_cities[city].plane == wizards[player].summoning_circle_xyp[XYP_PLANE])
                cityscape_has_summon_circle = 1;
        }
    }
    Cityscape_Draw_Background(city, x, y, map_x, map_y, plane);
    Cityscape_Layout(city, new_obj);
    Cityscape_Draw_Roads(x, y);
    Cityscape_Draw_Objects(city, x, y, new_obj);
    Cityscape_Draw_Wards_Walls(city, x, y);
    cityscape_anim_frame = (cityscape_anim_frame + 1) % 9;
    cityscape_anim_tick = (cityscape_anim_tick + 1) % 12;
    cityscape_wall_anim_frame = (cityscape_wall_anim_frame + 1) % 4;
    Random_Seed(seed);
    Draw_Line(x, y, x + 204, y, 0);
    Draw_Line(x, y + 95, x + 204, y + 95, 0);
    Draw_Line(x, y, x, y + 95, 0);
    Draw_Line(x + 204, y, x + 204, y + 95, 0);
    if (cityscape_reveal < 100)
        cityscape_reveal += 10;
    Heap_Release(p_scratch);
}

/* (x, y) the cityscape roads: grid cells next to occupied blocks become road (-5) along the rows 0 / 5 / 9 / 14
   and the columns 4 / 9 / 13 / 18 (bug: the row-0 road over columns 14..17 is written into row 5), road ends are
   joined to the next crossing road, crossings with two road neighbours are filled; then the road picture of the
   ground frame is drawn into cityscape_bitmap, the unused cells cut out, and shown at (x + 1, y + 17)
   Numbered: BUG #31 (BUGS_AND_QUIRKS.txt) */
void Cityscape_Draw_Roads(int x, int y)
{
    int r2;
    int col;
    int row;
    int cx;
    int ry;
    int nc;
    int nr;
    int n;
    int i;

    for (i = 0; i < 4; i++) {
        if (p_cityscape_grid[1][i] > 0) {
            for (col = 0; col < 4; col++)
                p_cityscape_grid[0][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 5; i < 9; i++) {
        if (p_cityscape_grid[1][i] > 0) {
            for (col = 5; col < 9; col++)
                p_cityscape_grid[0][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 10; i < 13; i++) {
        if (p_cityscape_grid[1][i] > 0) {
            for (col = 10; col < 13; col++)
                p_cityscape_grid[0][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 14; i < 18; i++) {
        if (p_cityscape_grid[1][i] > 0) {
            for (col = 14; col < 18; col++)
                p_cityscape_grid[5][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 1; i < 4; i++) {
        if (p_cityscape_grid[4][i] > 0 || p_cityscape_grid[6][i] > 0) {
            for (col = 0; col < 4; col++)
                p_cityscape_grid[5][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 5; i < 9; i++) {
        if (p_cityscape_grid[4][i] > 0 || p_cityscape_grid[6][i] > 0) {
            for (col = 5; col < 9; col++)
                p_cityscape_grid[5][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 10; i < 13; i++) {
        if (p_cityscape_grid[4][i] > 0 || p_cityscape_grid[6][i] > 0) {
            for (col = 10; col < 13; col++)
                p_cityscape_grid[5][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 19; i < 23; i++) {
        if (p_cityscape_grid[4][i] > 0 || p_cityscape_grid[6][i] > 0) {
            for (col = 19; col < 23; col++)
                p_cityscape_grid[5][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 2; i < 4; i++) {
        if (p_cityscape_grid[8][i] > 0 || p_cityscape_grid[10][i] > 0) {
            for (col = 2; col < 4; col++)
                p_cityscape_grid[9][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 5; i < 9; i++) {
        if (p_cityscape_grid[8][i] > 0 || p_cityscape_grid[10][i] > 0) {
            for (col = 5; col < 9; col++)
                p_cityscape_grid[9][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 10; i < 13; i++) {
        if (p_cityscape_grid[8][i] > 0 || p_cityscape_grid[10][i] > 0) {
            for (col = 10; col < 13; col++)
                p_cityscape_grid[9][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 14; i < 18; i++) {
        if (p_cityscape_grid[8][i] > 0 || p_cityscape_grid[10][i] > 0) {
            for (col = 14; col < 18; col++)
                p_cityscape_grid[9][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 19; i < 23; i++) {
        if (p_cityscape_grid[8][i] > 0 || p_cityscape_grid[10][i] > 0) {
            for (col = 19; col < 23; col++)
                p_cityscape_grid[9][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 5; i < 9; i++) {
        if (p_cityscape_grid[13][i] > 0) {
            for (col = 5; col < 9; col++)
                p_cityscape_grid[14][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 10; i < 13; i++) {
        if (p_cityscape_grid[13][i] > 0) {
            for (col = 10; col < 13; col++)
                p_cityscape_grid[14][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 14; i < 18; i++) {
        if (p_cityscape_grid[13][i] > 0) {
            for (col = 14; col < 18; col++)
                p_cityscape_grid[14][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (i = 19; i < 23; i++) {
        if (p_cityscape_grid[13][i] > 0) {
            for (col = 19; col < 23; col++)
                p_cityscape_grid[14][col] = CELL_ROAD;
            i = 1000;
        }
    }
    for (col = 0; col < 4; col++) {
        switch (col) {
        case 0:
            i = 4;
            break;
        case 1:
            i = 9;
            break;
        case 2:
            i = 13;
            break;
        case 3:
            i = 18;
            break;
        }
        for (r2 = 1; r2 < 5; r2++) {
            if (p_cityscape_grid[r2][i - 1] > 0 || p_cityscape_grid[r2][i + 1] > 0) {
                for (row = 1; row < 5; row++)
                    p_cityscape_grid[row][i] = CELL_ROAD;
                r2 = 1000;
            }
        }
        for (r2 = 6; r2 < 9; r2++) {
            if (p_cityscape_grid[r2][i - 1] > 0 || p_cityscape_grid[r2][i + 1] > 0) {
                for (row = 6; row < 9; row++)
                    p_cityscape_grid[row][i] = CELL_ROAD;
                r2 = 1000;
            }
        }
        for (r2 = 10; r2 < 14; r2++) {
            if (p_cityscape_grid[r2][i - 1] > 0 || p_cityscape_grid[r2][i + 1] > 0) {
                for (row = 10; row < 14; row++)
                    p_cityscape_grid[row][i] = CELL_ROAD;
                r2 = 1000;
            }
        }
    }
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            cx = cityscape_col_pos[col];
            ry = cityscape_row_pos[row];
            n = 0;
            if (p_cityscape_grid[ry - 1][cx] == CELL_ROAD && ry > 0)
                n++;
            if (p_cityscape_grid[ry + 1][cx] == CELL_ROAD && ry < 14)
                n++;
            if (p_cityscape_grid[ry][cx - 1] == CELL_ROAD && cx > 0)
                n++;
            if (p_cityscape_grid[ry][cx + 1] == CELL_ROAD && cx < 22)
                n++;
            if (n > 0) {
                if (col > 0 && p_cityscape_grid[ry][cx - 1] != CELL_ROAD) {
                    nc = cityscape_col_pos[col - 1];
                    nr = ry;
                    n = 0;
                    if (p_cityscape_grid[nr - 1][nc] == CELL_ROAD && nr > 0)
                        n++;
                    if (p_cityscape_grid[nr + 1][nc] == CELL_ROAD && nr < 14)
                        n++;
                    if (p_cityscape_grid[nr][nc - 1] == CELL_ROAD && nc > 0)
                        n++;
                    if (p_cityscape_grid[nr][nc + 1] == CELL_ROAD && nc < 22)
                        n++;
                    if (n > 0)
                        for (i = nc + 1; i < cx; i++)
                            p_cityscape_grid[ry][i] = CELL_ROAD;
                }
                if (col < 3 && p_cityscape_grid[ry][cx + 1] != CELL_ROAD) {
                    nc = cityscape_col_pos[col + 1];
                    nr = ry;
                    n = 0;
                    if (p_cityscape_grid[nr - 1][nc] == CELL_ROAD && nr > 0)
                        n++;
                    if (p_cityscape_grid[nr + 1][nc] == CELL_ROAD && nr < 14)
                        n++;
                    if (p_cityscape_grid[nr][nc - 1] == CELL_ROAD && nc > 0)
                        n++;
                    if (p_cityscape_grid[nr][nc + 1] == CELL_ROAD && nc < 22)
                        n++;
                    if (n > 0)
                        for (i = cx + 1; i < nc; i++)
                            p_cityscape_grid[ry][i] = CELL_ROAD;
                }
                if (row > 0 && p_cityscape_grid[ry - 1][cx] != CELL_ROAD) {
                    nc = cx;
                    nr = cityscape_row_pos[row - 1];
                    n = 0;
                    if (p_cityscape_grid[nr - 1][nc] == CELL_ROAD && nr > 0)
                        n++;
                    if (p_cityscape_grid[nr + 1][nc] == CELL_ROAD && nr < 14)
                        n++;
                    if (p_cityscape_grid[nr][nc - 1] == CELL_ROAD && nc > 0)
                        n++;
                    if (p_cityscape_grid[nr][nc + 1] == CELL_ROAD && nc < 22)
                        n++;
                    if (n > 0)
                        for (r2 = nr + 1; r2 < ry; r2++)
                            p_cityscape_grid[r2][cx] = CELL_ROAD;
                }
                if (row < 3 && p_cityscape_grid[ry + 1][cx] != CELL_ROAD) {
                    nc = cx;
                    nr = cityscape_row_pos[row + 1];
                    n = 0;
                    if (p_cityscape_grid[nr - 1][nc] == CELL_ROAD && nr > 0)
                        n++;
                    if (p_cityscape_grid[nr + 1][nc] == CELL_ROAD && nr < 14)
                        n++;
                    if (p_cityscape_grid[nr][nc - 1] == CELL_ROAD && nc > 0)
                        n++;
                    if (p_cityscape_grid[nr][nc + 1] == CELL_ROAD && nc < 22)
                        n++;
                    if (n > 0)
                        for (r2 = ry + 1; r2 < nr; r2++)
                            p_cityscape_grid[r2][cx] = CELL_ROAD;
                }
            }
        }
    }
    n = 0;
    if (p_cityscape_grid[0][3] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[0][5] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[1][4] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[0][4] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[0][8] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[0][10] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[1][9] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[0][9] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[0][12] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[0][14] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[1][13] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[0][13] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[5][3] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[5][5] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[6][4] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[4][4] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[5][4] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[5][8] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[5][10] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[6][9] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[4][9] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[5][9] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[5][12] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[5][14] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[6][13] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[4][13] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[5][13] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[5][17] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[5][19] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[6][18] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[4][18] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[5][18] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[9][3] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[9][5] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[10][4] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[8][4] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[9][4] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[9][8] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[9][10] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[10][9] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[8][9] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[9][9] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[9][12] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[9][14] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[10][13] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[8][13] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[9][13] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[9][17] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[9][19] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[10][18] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[8][18] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[9][18] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[14][3] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[14][5] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[13][4] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[14][4] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[14][8] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[14][10] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[13][9] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[14][9] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[14][12] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[14][14] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[13][13] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[14][13] = CELL_ROAD;
    n = 0;
    if (p_cityscape_grid[14][17] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[14][19] == CELL_ROAD)
        n++;
    if (p_cityscape_grid[13][18] == CELL_ROAD)
        n++;
    if (n > 1)
        p_cityscape_grid[14][18] = CELL_ROAD;
    Set_Pic_Frame(cityscape_roads_pic, cityscape_ground_frame);
    Pic_Decode_Frame_To_Bitmap(cityscape_roads_pic, cityscape_bitmap);
    Pic_Reset_Frame(cityscape_hmask_pic, 0);
    Pic_Decode_Frame_To_Bitmap(cityscape_hmask_pic, p_pic_work);
    Cityscape_Erase_Unused_Roads(cityscape_bitmap);
    Draw_Bitmap(x + 1, y + 17, cityscape_bitmap);
}

/* (bitmap) cuts every road cell left unused (-2) out of the road picture: capsule shapes along the road rows,
   slanted pieces along the road columns. The four road rows are cut at y 4 / 27 / 45 / 68; along a row a cell is
   10 px, each run between road columns 2 - 3 px further left, and each lower row starts further left still - the
   slant of Cityscape_Draw_Objects' layout, with the offsets written out by hand */
void Cityscape_Erase_Unused_Roads(unsigned bitmap)
{
    int col;
    int row;

    for (col = 0; col < 4; col++)
        if (p_cityscape_grid[0][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 29, 4, bitmap);
    if (p_cityscape_grid[0][4] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(69, 4, bitmap);
        if (p_cityscape_grid[0][3] == CELL_ROAD_LATTICE || p_cityscape_grid[0][5] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule10(72, 1, bitmap);
    }
    for (col = 5; col < 9; col++)
        if (p_cityscape_grid[0][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 27, 4, bitmap);
    if (p_cityscape_grid[0][9] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(117, 4, bitmap);
        if (p_cityscape_grid[0][8] == CELL_ROAD_LATTICE || p_cityscape_grid[0][10] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule10(120, 1, bitmap);
    }
    for (col = 10; col < 13; col++)
        if (p_cityscape_grid[0][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 25, 4, bitmap);
    if (p_cityscape_grid[0][13] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(154, 4, bitmap);
        if (p_cityscape_grid[0][12] == CELL_ROAD_LATTICE || p_cityscape_grid[0][14] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule10(157, 1, bitmap);
    }
    for (col = 14; col < 18; col++)
        if (p_cityscape_grid[0][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 22, 4, bitmap);
    for (col = 1; col < 4; col++)
        if (p_cityscape_grid[5][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 6, 27, bitmap);
    if (p_cityscape_grid[5][4] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(46, 27, bitmap);
    for (col = 5; col < 9; col++)
        if (p_cityscape_grid[5][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 4, 27, bitmap);
    if (p_cityscape_grid[5][9] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(94, 27, bitmap);
    for (col = 10; col < 13; col++)
        if (p_cityscape_grid[5][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 + 2, 27, bitmap);
    if (p_cityscape_grid[5][13] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(131, 27, bitmap);
    for (col = 14; col < 18; col++)
        if (p_cityscape_grid[5][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10, 27, bitmap);
    if (p_cityscape_grid[5][18] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(180, 27, bitmap);
    if (p_cityscape_grid[5][19] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule12(188, 27, bitmap);
    for (col = 3; col < 4; col++)
        if (p_cityscape_grid[9][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 12, 45, bitmap);
    if (p_cityscape_grid[9][4] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(28, 45, bitmap);
    for (col = 5; col < 9; col++)
        if (p_cityscape_grid[9][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 14, 45, bitmap);
    if (p_cityscape_grid[9][9] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(76, 45, bitmap);
    for (col = 10; col < 13; col++)
        if (p_cityscape_grid[9][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 16, 45, bitmap);
    if (p_cityscape_grid[9][13] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(113, 45, bitmap);
    for (col = 14; col < 18; col++)
        if (p_cityscape_grid[9][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 19, 45, bitmap);
    if (p_cityscape_grid[9][18] == CELL_ROAD_LATTICE)
        Pic_Cut_Capsule10(161, 45, bitmap);
    for (col = 19; col < 22; col++)
        if (p_cityscape_grid[9][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 21, 45, bitmap);
    if (p_cityscape_grid[14][4] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(5, 68, bitmap);
        Pic_Cut_Capsule10(0, 68, bitmap);
        Pic_Cut_Capsule10(2, 71, bitmap);
    }
    for (col = 5; col < 9; col++)
        if (p_cityscape_grid[14][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 37, 68, bitmap);
    if (p_cityscape_grid[14][9] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(53, 68, bitmap);
        if (p_cityscape_grid[14][8] == CELL_ROAD_LATTICE || p_cityscape_grid[14][10] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule10(52, 71, bitmap);
    }
    for (col = 10; col < 13; col++)
        if (p_cityscape_grid[14][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 39, 68, bitmap);
    if (p_cityscape_grid[14][13] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(91, 68, bitmap);
        if (p_cityscape_grid[14][12] == CELL_ROAD_LATTICE || p_cityscape_grid[14][14] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule10(89, 71, bitmap);
    }
    for (col = 14; col < 18; col++)
        if (p_cityscape_grid[14][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 41, 68, bitmap);
    if (p_cityscape_grid[14][18] == CELL_ROAD_LATTICE) {
        Pic_Cut_Capsule10(139, 68, bitmap);
        if (p_cityscape_grid[14][17] == CELL_ROAD_LATTICE || p_cityscape_grid[14][19] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule10(137, 71, bitmap);
    }
    for (col = 19; col < 23; col++)
        if (p_cityscape_grid[14][col] == CELL_ROAD_LATTICE)
            Pic_Cut_Capsule12(col * 10 - 44, 68, bitmap);
    for (row = 1; row < 5; row++)
        if (p_cityscape_grid[row][4] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(70 - row * 5, row * 5 - 1, bitmap);
    for (row = 6; row < 9; row++)
        if (p_cityscape_grid[row][4] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(72 - row * 5, row * 5 - 3, bitmap);
    for (row = 10; row < 14; row++)
        if (p_cityscape_grid[row][4] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(74 - row * 5, row * 5 - 5, bitmap);
    for (row = 1; row < 5; row++)
        if (p_cityscape_grid[row][9] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(118 - row * 5, row * 5 - 1, bitmap);
    for (row = 6; row < 9; row++)
        if (p_cityscape_grid[row][9] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(120 - row * 5, row * 5 - 3, bitmap);
    for (row = 10; row < 14; row++)
        if (p_cityscape_grid[row][9] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(122 - row * 5, row * 5 - 5, bitmap);
    for (row = 1; row < 5; row++)
        if (p_cityscape_grid[row][13] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(156 - row * 5, row * 5 - 1, bitmap);
    for (row = 6; row < 9; row++)
        if (p_cityscape_grid[row][13] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(158 - row * 5, row * 5 - 3, bitmap);
    for (row = 10; row < 14; row++)
        if (p_cityscape_grid[row][13] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(160 - row * 5, row * 5 - 5, bitmap);
    for (row = 2; row < 5; row++)
        if (p_cityscape_grid[row][18] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(204 - row * 5, row * 5 - 1, bitmap);
    for (row = 6; row < 9; row++)
        if (p_cityscape_grid[row][18] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(206 - row * 5, row * 5 - 3, bitmap);
    for (row = 10; row < 14; row++)
        if (p_cityscape_grid[row][18] == CELL_ROAD_LATTICE)
            Pic_Cut_RoadDiag(208 - row * 5, row * 5 - 5, bitmap);
}

/* (x, y, bitmap) clears a rounded road piece 12 columns long and 3 rows high at (x, y) of the column-major
   bitmap (1, 2, 8 x 3, 2, 1 pixels per column) */
void Pic_Cut_Capsule12(int x, int y, unsigned bitmap)
{
    int k;
    char far *pixels;
    int height;
    int pos;

    height = Far_Peek_Word(bitmap, 2);
    pixels = Seg_To_Far_Ptr(bitmap);
    pos = height * x + y + 18;                 /* 16: the bitmap header, + 2 rows */
    pixels[pos] = 0;
    pos += height - 1;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pos += height - 1;
    for (k = 0; k < 8; k++) {
        pixels[pos] = 0;
        pixels[pos + 1] = 0;
        pixels[pos + 2] = 0;
        pos += height;
    }
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pos += height;
    pixels[pos] = 0;
}

/* (x, y, bitmap) clears a rounded road piece 10 columns long and 3 rows high at (x, y) of the column-major
   bitmap (1, 2, 6 x 3, 2, 1 pixels per column) */
void Pic_Cut_Capsule10(int x, int y, unsigned bitmap)
{
    int k;
    char far *pixels;
    int height;
    int pos;

    height = Far_Peek_Word(bitmap, 2);
    pixels = Seg_To_Far_Ptr(bitmap);
    pos = height * x + y + 18;                 /* 16: the bitmap header, + 2 rows */
    pixels[pos] = 0;
    pos += height - 1;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pos += height - 1;
    for (k = 0; k < 6; k++) {
        pixels[pos] = 0;
        pixels[pos + 1] = 0;
        pixels[pos + 2] = 0;
        pos += height;
    }
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pos += height;
    pixels[pos] = 0;
}

/* (x, y, bitmap) clears a slanted 10-column road piece at (x, y) of the column-major bitmap */
void Pic_Cut_RoadDiag(int x, int y, unsigned bitmap)
{
    char far *pixels;
    int height;
    int pos;

    height = Far_Peek_Word(bitmap, 2);
    pixels = Seg_To_Far_Ptr(bitmap);
    pos = height * x + y + 23;                 /* 16: the bitmap header, + 7 rows */
    pixels[pos] = 0;
    pos += height - 1;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pos += height - 1;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pixels[pos + 2] = 0;
    pos += height - 1;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pixels[pos + 2] = 0;
    pixels[pos + 3] = 0;
    pos += height - 1;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pixels[pos + 2] = 0;
    pixels[pos + 3] = 0;
    pixels[pos + 4] = 0;
    pos += height;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pixels[pos + 2] = 0;
    pixels[pos + 3] = 0;
    pixels[pos + 4] = 0;
    pos += height;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pixels[pos + 2] = 0;
    pixels[pos + 3] = 0;
    pos += height;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pixels[pos + 2] = 0;
    pos += height;
    pixels[pos] = 0;
    pixels[pos + 1] = 0;
    pos += height;
    pixels[pos] = 0;
}

/* (city, x, y, new_obj) draws the layout grid row by row (painter's order): houses of the race's style (2), trees
   (-3), rocks (-4), the buildings (3..34; rubble when removed - sold or destroyed -, Evil Presence over the temples 22..24) and the specials
   (100..106, except the one being added) with their hotspots, the faded-in object dissolved; then the city walls.
   Bug: the walls fade with the last grid cell's object, not with City Walls
   Numbered: BUG #31 (BUGS_AND_QUIRKS.txt) */
void Cityscape_Draw_Objects(int city, int x, int y, int new_obj)
{
    int col;
    int row;
    int house_style;
    int rnd;
    int bld;
    unsigned pic;
    register int sx;
    register int sy;

    cityscape_hotspot_count = 0;
    house_style = race_table[p_cities[city].race].house_style;
    for (row = 1; row < 15; row++) {
        for (col = 0; col < 23; col++) {
            sy = (row - 1) * 5;
            if (row > 4)
                sy -= 2;
            if (row > 8)
                sy -= 2;
            if (row > 13)
                sy -= 2;
            sy += y + 27;
            sx = col * 10;
            if (col > 3)
                sx -= 2;
            if (col > 8)
                sx -= 2;
            if (col > 13)
                sx -= 2;
            if (col > 18)
                sx -= 2;
            sx += x + 27 - (sy - y - 27);
            bld = p_cityscape_grid[row][col];
            if (bld == CELL_HOUSE && x + 195 > sx)
                Draw_Pic_Frame(sx, sy - 19, cityscape_house_pics[house_style][Random(5) - 1]);
            if (bld == CELL_TREE) {
                rnd = Random(3) - 1;
                if (x + 195 > sx + rnd)
                    Draw_Pic_Frame(sx + rnd, sy - 6 - rnd, cityscape_tree_pics[Random(3) - 1]);
            }
            if (bld == CELL_ROCK) {
                rnd = Random(3) - 1;
                if (x + 195 > sx + rnd)
                    Draw_Pic_Frame(sx + rnd, sy - 6, cityscape_rocks_pics[Random(3) - 1]);
            }
            if (bld >= BLD_BARRACKS && bld <= BLD_MINERS_GUILD) {
                Cityscape_Add_Hotspot(sx, sy, bld, p_building_data[bld].cityscape_size);
                if (p_cities[city].buildings[bld] == BLDSTATE_REMOVED) {
                    pic = cityscape_rubble_pics[p_building_data[bld].cityscape_size];
                    Set_Pic_Frame(pic, cityscape_anim_frame);
                    Pic_Set_Remap_Flag(p_pic_work);
                    Pic_Decode_Frame_To_Bitmap(pic, p_pic_work);
                } else {
                    Set_Pic_Frame(building_pics[bld], cityscape_anim_frame);
                    Pic_Decode_Frame_To_Bitmap(building_pics[bld], p_pic_work);
                    if (bld == cityscape_fade_obj)
                        Pic_Dissolve(p_pic_work, cityscape_reveal);
                    Pic_Set_Remap_Flag(p_pic_work);
                }
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                if (bld >= BLD_SHRINE && bld <= BLD_PARTHENON && p_cities[city].enchants[CITY_ENCH_EVIL_PRESENCE]) {
                    Set_Pic_Frame(cityscape_evil_presence_pic, cityscape_anim_frame);
                    Pic_Decode_Frame_To_Bitmap(cityscape_evil_presence_pic, p_pic_work);
                    if (bld == cityscape_fade_obj)
                        Pic_Dissolve(p_pic_work, cityscape_reveal);
                    Pic_Set_Remap_Flag(p_pic_work);
                    Draw_Bitmap(sx, sy - 32, p_pic_work);
                }
            }
            if (bld == SCAPE_DARK_RITUALS && new_obj != SCAPE_DARK_RITUALS) {
                Set_Pic_Frame(cityscape_dark_rituals_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_dark_rituals_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_DARK_RITUALS, 1);
            }
            if (bld == SCAPE_SUMMONING_CIRCLE && new_obj != SCAPE_SUMMONING_CIRCLE) {
                Set_Pic_Frame(cityscape_summon_circle_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_summon_circle_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_SUMMONING_CIRCLE, 1);
            }
            if (bld == SCAPE_EARTH_GATE && new_obj != SCAPE_EARTH_GATE) {
                Set_Pic_Frame(cityscape_earth_gate_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_earth_gate_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_EARTH_GATE, 0);
            }
            if (bld == SCAPE_STREAM_OF_LIFE && new_obj != SCAPE_STREAM_OF_LIFE) {
                Set_Pic_Frame(cityscape_stream_of_life_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_stream_of_life_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_STREAM_OF_LIFE, 0);
            }
            if (bld == SCAPE_ASTRAL_GATE && new_obj != SCAPE_ASTRAL_GATE) {
                Set_Pic_Frame(cityscape_astral_gate_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_astral_gate_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_ASTRAL_GATE, 0);
            }
            if (bld == SCAPE_FORTRESS && new_obj != SCAPE_FORTRESS) {
                Set_Pic_Frame(cityscape_fortress_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_fortress_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 47, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_FORTRESS, 4);
            }
            if (bld == SCAPE_ALTAR_OF_BATTLE && new_obj != SCAPE_ALTAR_OF_BATTLE) {
                Set_Pic_Frame(cityscape_altar_pic, cityscape_anim_frame);
                Pic_Decode_Frame_To_Bitmap(cityscape_altar_pic, p_pic_work);
                if (bld == cityscape_fade_obj)
                    Pic_Dissolve(p_pic_work, cityscape_reveal);
                Pic_Set_Remap_Flag(p_pic_work);
                Draw_Bitmap(sx, sy - 32, p_pic_work);
                Cityscape_Add_Hotspot(sx, sy, SCAPE_ALTAR_OF_BATTLE, 1);
            }
        }
    }
    if (p_cities[city].buildings[BLD_CITY_WALLS] == BLDSTATE_BUILT) {
        Set_Pic_Frame(cityscape_city_walls_pic, 0);
        Pic_Decode_Frame_To_Bitmap(cityscape_city_walls_pic, p_pic_work);
        if (bld == cityscape_fade_obj)
            Pic_Dissolve(p_pic_work, cityscape_reveal);
        Pic_Set_Remap_Flag(p_pic_work);
        Draw_Bitmap(x, y + 82, p_pic_work);
        p_cityscape_hotspots[cityscape_hotspot_count].building = BLD_CITY_WALLS;
        p_cityscape_hotspots[cityscape_hotspot_count].x1 = x;
        p_cityscape_hotspots[cityscape_hotspot_count].y1 = y + 82;
        p_cityscape_hotspots[cityscape_hotspot_count].x2 = x + 204;
        p_cityscape_hotspots[cityscape_hotspot_count].y2 = y + 93;
        p_cityscape_hotspots[cityscape_hotspot_count].label_x = x + 102;
        p_cityscape_hotspots[cityscape_hotspot_count].label_y = y + 85;
        cityscape_hotspot_count++;
    }
}

/* (city, new_obj) lays out the 15 x 23 cityscape grid: blocked corners (-1), the road lattice (-2), the fortress
   block, lots for the buildings by footprint (3x3, 3 wide x 2, 2 wide x 3, 2x2; 500 failed tries drop a lot) spread
   randomly around the centre, houses (population x 8), trees (many in a forest or for house style 1) and rocks (many
   on mountains / hills), then the buildings and specials into their lots, the fortress at [8][10] and the ship
   building (Ship Wrights' Guild .. Maritime Guild, 12..14) at [8][2] in the harbour lot. Row 0 is not cleared, but the lattice and
   blocked loops write every cell of it
   Numbered: BUG #345 (BUGS_AND_QUIRKS.txt) */
void Cityscape_Layout(int city, int new_obj)
{
    int i;
    int j;
    int bld;                                    /* also the counter of the lot / house / tree / rock loops */
    int n_lot0;                                 /* lot class k (n_lot<k>, used<k>, lot<k>_x / _y) = Building.cityscape_size:
                                                   0 2 x 2, 1 3 wide x 2, 2 2 wide x 3, 3 3 x 3 squares */
    int n_lot2;
    int n_lot1;
    int n_lot3;
    int used0;
    int used2;
    int used1;
    int used3;
    int tries;
    int col;
    int row;
    int spread;
    int trees;
    int rocks;
    int bld_state;
    int houses;
    int lot0_x[14];
    int lot0_y[14];
    int lot2_x[8];
    int lot2_y[8];
    int lot1_x[8];
    int lot1_y[8];
    int lot3_x[8];
    int lot3_y[8];

    for (i = 1; i < 15; i++)
        for (j = 0; j < 23; j++)
            p_cityscape_grid[i][j] = CELL_EMPTY;
    for (i = 4; i < 15; i++)
        p_cityscape_grid[i][0] = CELL_BLOCKED;
    for (i = 7; i < 15; i++)
        p_cityscape_grid[i][1] = CELL_BLOCKED;
    for (i = 10; i < 15; i++)
        p_cityscape_grid[i][2] = CELL_BLOCKED;
    for (i = 12; i < 15; i++)
        p_cityscape_grid[i][3] = CELL_BLOCKED;
    for (j = 19; j < 23; j++)
        for (i = 0; i < 5; i++)
            p_cityscape_grid[i][j] = CELL_BLOCKED;
    p_cityscape_grid[6][21] = CELL_BLOCKED;
    p_cityscape_grid[6][22] = CELL_BLOCKED;
    p_cityscape_grid[7][22] = CELL_BLOCKED;
    p_cityscape_grid[0][18] = CELL_BLOCKED;
    if (cityscape_has_fortress == 1 || new_obj == SCAPE_FORTRESS) {
        for (j = 10; j < 13; j++)
            for (i = 6; i < 9; i++)
                p_cityscape_grid[i][j] = CELL_RESERVED;
        for (i = 1; i <= 2; i++)
            p_cityscape_grid[i][7] = CELL_BLOCKED;
        for (i = 1; i <= 2; i++)
            p_cityscape_grid[i][8] = CELL_BLOCKED;
        for (i = 3; i <= 4; i++)
            p_cityscape_grid[i][10] = CELL_BLOCKED;
    }
    p_cityscape_grid[7][22] = CELL_BLOCKED;
    p_cityscape_grid[8][22] = CELL_BLOCKED;
    p_cityscape_grid[7][21] = CELL_BLOCKED;
    p_cityscape_grid[8][21] = CELL_BLOCKED;
    for (j = 0; j < 18; j++)
        p_cityscape_grid[0][j] = CELL_ROAD_LATTICE;
    for (j = 1; j < 23; j++)
        p_cityscape_grid[5][j] = CELL_ROAD_LATTICE;
    for (j = 3; j < 23; j++)
        p_cityscape_grid[9][j] = CELL_ROAD_LATTICE;
    for (j = 4; j < 23; j++)
        p_cityscape_grid[14][j] = CELL_ROAD_LATTICE;
    for (i = 0; i < 15; i++)
        p_cityscape_grid[i][4] = CELL_ROAD_LATTICE;
    for (i = 0; i < 15; i++)
        p_cityscape_grid[i][9] = CELL_ROAD_LATTICE;
    for (i = 0; i < 15; i++)
        p_cityscape_grid[i][13] = CELL_ROAD_LATTICE;
    for (i = 1; i < 15; i++)
        p_cityscape_grid[i][18] = CELL_ROAD_LATTICE;
    n_lot0 = 0;
    n_lot2 = 0;
    n_lot1 = 0;
    n_lot3 = 0;
    for (bld = BLD_BARRACKS; bld <= BLD_MINERS_GUILD; bld++) {
        bld_state = p_cities[city].buildings[bld];
        if ((bld_state >= 1 || bld == new_obj) && bld != BLD_SHIP_YARD && bld != BLD_SHIP_WRIGHTS_GUILD && bld != BLD_MARITIME_GUILD) {
            switch (p_building_data[bld].cityscape_size) {
            case 0:
                n_lot0++;
                break;
            case 1:
                n_lot1++;
                break;
            case 2:
                n_lot2++;
                break;
            case 3:
                n_lot3++;
                break;
            }
        }
    }
    if (cityscape_has_summon_circle || new_obj == SCAPE_SUMMONING_CIRCLE)
        n_lot1++;
    if (p_cities[city].enchants[CITY_ENCH_EARTH_GATE] || new_obj == SCAPE_EARTH_GATE)
        n_lot0++;
    if (p_cities[city].enchants[CITY_ENCH_STREAM_OF_LIFE] || new_obj == SCAPE_STREAM_OF_LIFE)
        n_lot0++;
    if (p_cities[city].enchants[CITY_ENCH_ASTRAL_GATE] || new_obj == SCAPE_ASTRAL_GATE)
        n_lot0++;
    if (p_cities[city].enchants[CITY_ENCH_DARK_RITUALS] || new_obj == SCAPE_DARK_RITUALS)
        n_lot1++;
    if (p_cities[city].enchants[CITY_ENCH_ALTAR_OF_BATTLE] || new_obj == SCAPE_ALTAR_OF_BATTLE)
        n_lot1++;
    for (bld = BLD_SHIP_WRIGHTS_GUILD; bld <= BLD_MARITIME_GUILD; bld++)
        if (p_cities[city].buildings[bld] >= 1 || bld == new_obj)
            for (i = 6; i < 9; i++)
                for (j = 2; j < 4; j++)
                    p_cityscape_grid[i][j] = CELL_RESERVED;
    spread = 3;
    bld = 0;
    tries = 0;
    while (bld < n_lot3) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 20 && row >= 0 && row < 12) {
            if (p_cityscape_grid[row][col] == CELL_EMPTY && p_cityscape_grid[row][col + 1] == CELL_EMPTY &&
                p_cityscape_grid[row][col + 2] == CELL_EMPTY && p_cityscape_grid[row + 1][col] == CELL_EMPTY &&
                p_cityscape_grid[row + 1][col + 1] == CELL_EMPTY && p_cityscape_grid[row + 1][col + 2] == CELL_EMPTY &&
                p_cityscape_grid[row + 2][col] == CELL_EMPTY && p_cityscape_grid[row + 2][col + 1] == CELL_EMPTY &&
                p_cityscape_grid[row + 2][col + 2] == CELL_EMPTY) {
                tries = 0;
                lot3_x[bld] = col;
                lot3_y[bld] = row;
                bld++;
                for (i = 0; i < 3; i++)
                    for (j = 0; j < 3; j++)
                        p_cityscape_grid[row + i][col + j] = CELL_RESERVED;
            }
            tries++;
            if (tries > 500) {
                tries = 0;
                n_lot3--;
            }
        }
        if (spread < 30)
            spread++;
    }
    spread = 3;
    bld = 0;
    tries = 0;
    while (bld < n_lot1) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 20 && row >= 0 && row < 13) {
            if (p_cityscape_grid[row][col] == CELL_EMPTY && p_cityscape_grid[row][col + 1] == CELL_EMPTY &&
                p_cityscape_grid[row][col + 2] == CELL_EMPTY && p_cityscape_grid[row + 1][col] == CELL_EMPTY &&
                p_cityscape_grid[row + 1][col + 1] == CELL_EMPTY && p_cityscape_grid[row + 1][col + 2] == CELL_EMPTY) {
                tries = 0;
                lot1_x[bld] = col;
                lot1_y[bld] = row;
                bld++;
                for (i = 0; i < 2; i++)
                    for (j = 0; j < 3; j++)
                        p_cityscape_grid[row + i][col + j] = CELL_RESERVED;
            }
            tries++;
            if (tries > 500) {
                tries = 0;
                n_lot1--;
            }
        }
        if (spread < 30 && Random(3) == 1)
            spread++;
    }
    spread = 3;
    bld = 0;
    tries = 0;
    while (bld < n_lot2) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 21 && row >= 0 && row < 12) {
            if (p_cityscape_grid[row][col] == CELL_EMPTY && p_cityscape_grid[row][col + 1] == CELL_EMPTY &&
                p_cityscape_grid[row + 1][col] == CELL_EMPTY && p_cityscape_grid[row + 1][col + 1] == CELL_EMPTY &&
                p_cityscape_grid[row + 2][col] == CELL_EMPTY && p_cityscape_grid[row + 2][col + 1] == CELL_EMPTY) {
                tries = 0;
                lot2_x[bld] = col;
                lot2_y[bld] = row;
                bld++;
                for (i = 0; i < 3; i++)
                    for (j = 0; j < 2; j++)
                        p_cityscape_grid[row + i][col + j] = CELL_RESERVED;
            }
            tries++;
            if (tries > 500) {
                tries = 0;
                n_lot2--;
            }
        }
        if (spread < 30 && Random(3) == 1)
            spread++;
    }
    spread = 3;
    bld = 0;
    tries = 0;
    while (bld < n_lot0) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 21 && row >= 0 && row < 13) {
            if (p_cityscape_grid[row][col] == CELL_EMPTY && p_cityscape_grid[row][col + 1] == CELL_EMPTY &&
                p_cityscape_grid[row + 1][col] == CELL_EMPTY && p_cityscape_grid[row + 1][col + 1] == CELL_EMPTY) {
                tries = 0;
                lot0_x[bld] = col;
                lot0_y[bld] = row;
                bld++;
                for (i = 0; i < 2; i++)
                    for (j = 0; j < 2; j++)
                        p_cityscape_grid[row + i][col + j] = CELL_RESERVED;
            }
            tries++;
            if (tries > 500) {
                tries = 0;
                n_lot0--;
            }
        }
        if (spread < 30 && Random(3) == 1)
            spread++;
    }
    houses = p_cities[city].population << 3;
    if (houses == 0)
        houses = 1;
    spread = 3;
    for (bld = 0; bld < houses; bld++) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 23 && row >= 0 && row < 15 && p_cityscape_grid[row][col] == CELL_EMPTY)
            p_cityscape_grid[row][col] = CELL_HOUSE;
        else {
            if (spread < 30 && Random(4) == 1)
                spread++;
            if (Random(4) != 1)
                bld--;
        }
    }
    if (Tile_Is_Forest_Explored(p_cities[city].x, p_cities[city].y, p_cities[city].plane) == 1 ||
        race_table[p_cities[city].race].house_style == HOUSE_STYLE_TREE)
        trees = Random(20) + 80;
    else
        trees = Random(8) + Random(8) + 5;
    if (Tile_Is_Mountain(p_cities[city].x, p_cities[city].y, p_cities[city].plane) == 1 ||
        Tile_Is_Hills(p_cities[city].x, p_cities[city].y, p_cities[city].plane) == 1)
        rocks = Random(10) + 20;
    else
        rocks = Random(8) + Random(8);
    spread = 6;
    for (bld = 0; bld < trees; bld++) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 23 && row >= 0 && row < 15 && p_cityscape_grid[row][col] == CELL_EMPTY)
            p_cityscape_grid[row][col] = CELL_TREE;
        else {
            if (spread < 30 && Random(4) == 1)
                spread++;
            if (Random(2) == 1)
                bld--;
        }
    }
    spread = 6;
    for (bld = 0; bld < rocks; bld++) {
        col = Random(spread + 1) + 10 - spread / 2;
        row = Random(spread + 1) + 6 - spread / 2;
        if (col >= 0 && col < 23 && row >= 0 && row < 15 && p_cityscape_grid[row][col] == CELL_EMPTY)
            p_cityscape_grid[row][col] = CELL_ROCK;
        else {
            if (spread < 30 && Random(4) == 1)
                spread++;
            if (Random(2) == 1)
                bld--;
        }
    }
    cityscape_hotspot_count = 0;
    used0 = 0;
    used2 = 0;
    used1 = 0;
    used3 = 0;
    if (cityscape_has_summon_circle || new_obj == SCAPE_SUMMONING_CIRCLE) {
        if (used1 < n_lot1)
            p_cityscape_grid[lot1_y[used1] + 1][lot1_x[used1]] = SCAPE_SUMMONING_CIRCLE;
        used1++;
    }
    if (p_cities[city].enchants[CITY_ENCH_EARTH_GATE] || new_obj == SCAPE_EARTH_GATE) {
        if (used0 < n_lot0)
            p_cityscape_grid[lot0_y[used0] + 1][lot0_x[used0]] = SCAPE_EARTH_GATE;
        used0++;
    }
    if (p_cities[city].enchants[CITY_ENCH_STREAM_OF_LIFE] || new_obj == SCAPE_STREAM_OF_LIFE) {
        if (used0 < n_lot0)
            p_cityscape_grid[lot0_y[used0] + 1][lot0_x[used0]] = SCAPE_STREAM_OF_LIFE;
        used0++;
    }
    if (p_cities[city].enchants[CITY_ENCH_ASTRAL_GATE] || new_obj == SCAPE_ASTRAL_GATE) {
        if (used0 < n_lot0)
            p_cityscape_grid[lot0_y[used0] + 1][lot0_x[used0]] = SCAPE_ASTRAL_GATE;
        used0++;
    }
    if (p_cities[city].enchants[CITY_ENCH_ALTAR_OF_BATTLE] || new_obj == SCAPE_ALTAR_OF_BATTLE) {
        if (used1 < n_lot1)
            p_cityscape_grid[lot1_y[used1] + 1][lot1_x[used1]] = SCAPE_ALTAR_OF_BATTLE;
        used1++;
    }
    if (p_cities[city].enchants[CITY_ENCH_DARK_RITUALS] || new_obj == SCAPE_DARK_RITUALS) {
        if (used1 < n_lot1)
            p_cityscape_grid[lot1_y[used1] + 1][lot1_x[used1]] = SCAPE_DARK_RITUALS;
        used1++;
    }
    for (bld = BLD_BARRACKS; bld <= BLD_MINERS_GUILD; bld++) {
        if ((p_cities[city].buildings[bld] >= 1 || bld == new_obj) && bld != BLD_SHIP_YARD && bld != BLD_SHIP_WRIGHTS_GUILD && bld != BLD_MARITIME_GUILD) {
            switch (p_building_data[bld].cityscape_size) {
            case 0:
                if (used0 < n_lot0 && p_cities[city].buildings[bld] >= 1) {
                    p_cityscape_grid[lot0_y[used0] + 1][lot0_x[used0]] = bld;
                    used0++;
                }
                break;
            case 2:
                if (used2 < n_lot2 && p_cities[city].buildings[bld] >= 1) {
                    p_cityscape_grid[lot2_y[used2] + 2][lot2_x[used2]] = bld;
                    used2++;
                }
                break;
            case 1:
                if (used1 < n_lot1 && p_cities[city].buildings[bld] >= 1) {
                    p_cityscape_grid[lot1_y[used1] + 1][lot1_x[used1]] = bld;
                    used1++;
                }
                break;
            case 3:
                if (used3 < n_lot3 && p_cities[city].buildings[bld] >= 1) {
                    p_cityscape_grid[lot3_y[used3] + 2][lot3_x[used3]] = bld;
                    used3++;
                }
                break;
            }
        }
    }
    if (cityscape_has_fortress)
        p_cityscape_grid[8][10] = SCAPE_FORTRESS;
    for (bld = BLD_SHIP_WRIGHTS_GUILD; bld <= BLD_MARITIME_GUILD; bld++)
        if (p_cities[city].buildings[bld] >= 1 || bld == new_obj)
            p_cityscape_grid[8][2] = bld;
}

/* (city, x, y) the row of city-enchantment icons at y + 83, 21 pixels apart (the five wards, Nature's Eye,
   Inspirations, Prosperity, Consecration), then the animated Wall of Darkness and Wall of Fire at y + 84 */
void Cityscape_Draw_Wards_Walls(int city, int x, int y)
{
    if (p_cities[city].enchants[CITY_ENCH_DEATH_WARD])
        Draw_Pic_Frame(x + 8, y + 83, cityscape_death_ward_pic);
    if (p_cities[city].enchants[CITY_ENCH_CHAOS_WARD])
        Draw_Pic_Frame(x + 29, y + 83, cityscape_chaos_ward_pic);
    if (p_cities[city].enchants[CITY_ENCH_NATURE_WARD])
        Draw_Pic_Frame(x + 50, y + 83, cityscape_nature_ward_pic);
    if (p_cities[city].enchants[CITY_ENCH_LIFE_WARD])
        Draw_Pic_Frame(x + 71, y + 83, cityscape_life_ward_pic);
    if (p_cities[city].enchants[CITY_ENCH_SORCERY_WARD])
        Draw_Pic_Frame(x + 92, y + 83, cityscape_sorcery_ward_pic);
    if (p_cities[city].enchants[CITY_ENCH_NATURES_EYE])
        Draw_Pic_Frame(x + 113, y + 83, cityscape_natures_eye_pic);
    if (p_cities[city].enchants[CITY_ENCH_INSPIRATIONS])
        Draw_Pic_Frame(x + 134, y + 83, cityscape_inspirations_pic);
    if (p_cities[city].enchants[CITY_ENCH_PROSPERITY])
        Draw_Pic_Frame(x + 155, y + 83, cityscape_prosperity_pic);
    if (p_cities[city].enchants[CITY_ENCH_CONSECRATION])
        Draw_Pic_Frame(x + 176, y + 83, cityscape_consecration_pic);
    if (p_cities[city].enchants[CITY_ENCH_WALL_OF_DARKNESS]) {
        Set_Pic_Frame(cityscape_wall_of_darkness_pic, cityscape_wall_anim_frame);
        Pic_Decode_Frame_To_Bitmap(cityscape_wall_of_darkness_pic, cityscape_bitmap);
        Draw_Bitmap(x, y + 84, cityscape_bitmap);
    }
    if (p_cities[city].enchants[CITY_ENCH_WALL_OF_FIRE]) {
        Set_Pic_Frame(cityscape_wall_of_fire_pic, cityscape_wall_anim_frame);
        Pic_Decode_Frame_To_Bitmap(cityscape_wall_of_fire_pic, cityscape_bitmap);
        Draw_Bitmap(x, y + 84, cityscape_bitmap);
    }
}

/* (city, x, y, map_x, map_y, plane) the cityscape background: the ground of the plane in the ground frame, a river
   or else an ocean when one touches the city's square (animated up to anim_tick / 2), then the far strip: nothing
   under Flying Fortress, else Cloud of Shadow, Heavenly Light, Chaos Rift, mountains or hills north of the city,
   or plains - each in its Arcanus or Myrror version */
void Cityscape_Draw_Background(int city, int x, int y, int map_x, int map_y, int plane)
{
    int k;

    if (p_cities[city].plane == PLANE_ARCANUS) {
        Set_Pic_Frame(cityscape_arcanus_ground_pic, cityscape_ground_frame);
        Draw_Pic_Frame(x, y, cityscape_arcanus_ground_pic);
    } else {
        Set_Pic_Frame(cityscape_myrror_ground_pic, cityscape_ground_frame);
        Draw_Pic_Frame(x, y, cityscape_myrror_ground_pic);
    }
    if (Tile_Is_River(map_x - 1, map_y - 1, plane) == 1 || Tile_Is_River(map_x - 1, map_y, plane) == 1 ||
        Tile_Is_River(map_x - 1, map_y + 1, plane) == 1 || Tile_Is_River(map_x, map_y - 1, plane) == 1 ||
        Tile_Is_River(map_x, map_y, plane) == 1 || Tile_Is_River(map_x, map_y + 1, plane) == 1 ||
        Tile_Is_River(map_x + 1, map_y - 1, plane) == 1 || Tile_Is_River(map_x + 1, map_y, plane) == 1 ||
        Tile_Is_River(map_x + 1, map_y + 1, plane) == 1) {
        if (plane == PLANE_ARCANUS) {
            Set_Pic_Frame(cityscape_river_pic, 0);
            for (k = 0; k <= cityscape_anim_tick / 2; k++)
                Draw_Pic_Frame(x, y, cityscape_river_pic);
        } else {
            Set_Pic_Frame(cityscape_myrror_river_pic, 0);
            for (k = 0; k <= cityscape_anim_tick / 2; k++)
                Draw_Pic_Frame(x, y, cityscape_myrror_river_pic);
        }
    } else if (Tile_Is_Sea(map_x - 1, map_y - 1, plane) == 1 || Tile_Is_Sea(map_x - 1, map_y, plane) == 1 ||
        Tile_Is_Sea(map_x - 1, map_y + 1, plane) == 1 || Tile_Is_Sea(map_x, map_y - 1, plane) == 1 ||
        Tile_Is_Sea(map_x, map_y, plane) == 1 || Tile_Is_Sea(map_x, map_y + 1, plane) == 1 ||
        Tile_Is_Sea(map_x + 1, map_y - 1, plane) == 1 || Tile_Is_Sea(map_x + 1, map_y, plane) == 1 ||
        Tile_Is_Sea(map_x + 1, map_y + 1, plane) == 1) {
        if (plane == PLANE_ARCANUS) {
            Pic_Reset_Frame(cityscape_ocean_pic);
            for (k = 0; k <= cityscape_anim_tick / 2; k++)
                Draw_Pic_Frame(x, y, cityscape_ocean_pic);
        } else {
            Pic_Reset_Frame(cityscape_myrror_ocean_pic, 0);
            for (k = 0; k <= cityscape_anim_tick / 2; k++)
                Draw_Pic_Frame(x, y, cityscape_myrror_ocean_pic);
        }
    }
    if (p_cities[city].enchants[CITY_ENCH_FLYING_FORTRESS] == 0) {
        if (p_cities[city].enchants[CITY_ENCH_CLOUD_OF_SHADOW]) {
            if (p_cities[city].plane == PLANE_ARCANUS)
                Draw_Pic_Frame(x, y, cityscape_far_dark_cloud_pic);
            else
                Draw_Pic_Frame(x, y, cityscape_myrror_dark_cloud_pic);
        } else if (p_cities[city].enchants[CITY_ENCH_HEAVENLY_LIGHT]) {
            if (p_cities[city].plane == PLANE_ARCANUS)
                Draw_Pic_Frame(x, y, cityscape_far_heavenly_light_pic);
            else
                Draw_Pic_Frame(x, y, cityscape_myrror_heavenly_light_pic);
        } else if (p_cities[city].enchants[CITY_ENCH_CHAOS_RIFT]) {
            if (p_cities[city].plane == PLANE_ARCANUS)
                Draw_Pic_Frame(x, y, cityscape_far_chaos_rift_pic);
            else
                Draw_Pic_Frame(x, y, cityscape_myrror_chaos_rift_pic);
        } else if (Tile_Is_Mountain(map_x - 1, map_y - 1, plane) == 1 || Tile_Is_Mountain(map_x, map_y - 1, plane) == 1 ||
            Tile_Is_Mountain(map_x + 1, map_y - 1, plane) == 1 || Tile_Is_Mountain(map_x, map_y, plane) == 1) {
            if (p_cities[city].plane == PLANE_ARCANUS) {
                Pic_Reset_Frame(cityscape_mountain_bg_pic);
                Draw_Pic_Frame(x, y, cityscape_mountain_bg_pic);
            } else {
                Pic_Reset_Frame(cityscape_myrror_mountain_pic);
                Draw_Pic_Frame(x, y, cityscape_myrror_mountain_pic);
            }
        } else if (Tile_Is_Hills(map_x - 1, map_y - 1, plane) == 1 || Tile_Is_Hills(map_x, map_y - 1, plane) == 1 ||
            Tile_Is_Hills(map_x + 1, map_y - 1, plane) == 1 || Tile_Is_Hills(map_x, map_y, plane) == 1) {
            if (p_cities[city].plane == PLANE_ARCANUS) {
                Pic_Reset_Frame(cityscape_hills_bg_pic);
                Draw_Pic_Frame(x, y, cityscape_hills_bg_pic);
            } else {
                Pic_Reset_Frame(cityscape_myrror_hills_pic);
                Draw_Pic_Frame(x, y, cityscape_myrror_hills_pic);
            }
        } else {
            if (p_cities[city].plane == PLANE_ARCANUS)
                Draw_Pic_Frame(x, y, cityscape_plain_bg_pic);
            else
                Draw_Pic_Frame(x, y, cityscape_myrror_plains_pic);
        }
    }
}

/* (x, y, bld, shape) adds three hotspot rectangles for a building or special from the shape table; their label is at
   y + 1, centred over the three (minus 3) */
void Cityscape_Add_Hotspot(int x, int y, int bld, int shape)
{
    int max_x;
    int min_x;
    int i;

    max_x = 0;
    min_x = 1000;
    for (i = 0; i < 3; i++) {
        p_cityscape_hotspots[cityscape_hotspot_count].building = bld;
        p_cityscape_hotspots[cityscape_hotspot_count].x1 = x + cityscape_hotspot_shapes[shape][i][0];
        p_cityscape_hotspots[cityscape_hotspot_count].y1 = y + cityscape_hotspot_shapes[shape][i][1];
        p_cityscape_hotspots[cityscape_hotspot_count].x2 = x + cityscape_hotspot_shapes[shape][i][2];
        p_cityscape_hotspots[cityscape_hotspot_count].y2 = y + cityscape_hotspot_shapes[shape][i][3];
        p_cityscape_hotspots[cityscape_hotspot_count].label_y = y + 1;
        if (p_cityscape_hotspots[cityscape_hotspot_count].x1 < min_x)
            min_x = p_cityscape_hotspots[cityscape_hotspot_count].x1;
        if (p_cityscape_hotspots[cityscape_hotspot_count].x2 > max_x)
            max_x = p_cityscape_hotspots[cityscape_hotspot_count].x2;
        cityscape_hotspot_count++;
    }
    for (i = cityscape_hotspot_count - 3; i < cityscape_hotspot_count; i++)
        p_cityscape_hotspots[i].label_x = (min_x + max_x) / 2 - 3;
}

/* (city, x, y) the outpost scene (72 x 66 with a black frame): the landscape with the random generator seeded from
   the city's square (the previous seed restored), the animation frame stepped 0..9
   Numbered: BUG #207 (BUGS_AND_QUIRKS.txt) */
void Outpost_Draw_Scene(int city, int x, int y)
{
    int map_x;
    int map_y;
    int plane;
    long seed;

    seed = Random_Get_Seed();
    map_x = p_cities[city].x;
    map_y = p_cities[city].y;
    plane = p_cities[city].plane;
    Random_Seed((long)map_x * map_y + CITYSCAPE_SEED_SALT);
    Outpost_Draw_Landscape(city, x, y);
    cityscape_anim_frame = (cityscape_anim_frame + 1) % 10;
    Random_Seed(seed);
    Draw_Line(x, y, x + 71, y, 0);
    Draw_Line(x, y + 65, x + 71, y + 65, 0);
    Draw_Line(x, y, x, y + 65, 0);
    Draw_Line(x + 71, y, x + 71, y + 65, 0);
}

/* (city, x, y) the outpost landscape, clipped to the scene: the ground (Gaia's Blessing / Flying Fortress / plain),
   Famine and Cursed Lands, mountains or hills north of the outpost (not under Flying Fortress), Cloud of Shadow,
   Heavenly Light and Chaos Rift (drawn anim_frame + 1 times), and the first house of the race's style. Slip: the
   hills case sets the frame of the MOUNTAIN picture and draws the hills picture */
void Outpost_Draw_Landscape(int city, int x, int y)
{
    int map_x;
    int map_y;
    int plane;
    int k;
    int house_style;

    map_x = p_cities[city].x;
    map_y = p_cities[city].y;
    plane = p_cities[city].plane;
    Set_Clip_Rect(x, y, x + 71, y + 65);
    if (p_cities[city].enchants[CITY_ENCH_GAIAS_BLESSING]) {
        Set_Pic_Frame(cityscape_gaia_pic, 0);
        Draw_Pic_Clipped(x - 65, y, cityscape_gaia_pic);
    } else {
        if (p_cities[city].enchants[CITY_ENCH_FLYING_FORTRESS]) {
            Set_Pic_Frame(cityscape_flying_fortress_pic, 0);
            Draw_Pic_Clipped(x - 65, y, cityscape_flying_fortress_pic);
        } else {
            Set_Pic_Frame(cityscape_arcanus_ground_pic, 0);
            Draw_Pic_Clipped(x - 65, y, cityscape_arcanus_ground_pic);
        }
    }
    if (p_cities[city].enchants[CITY_ENCH_FAMINE]) {
        Set_Pic_Frame(cityscape_famine_pic, 0);
        Draw_Pic_Clipped(x - 65, y, cityscape_famine_pic, 0);
    }
    if (p_cities[city].enchants[CITY_ENCH_CURSED_LANDS]) {
        Set_Pic_Frame(cityscape_cursed_land_pic, 0);
        Draw_Pic_Clipped(x - 65, y, cityscape_cursed_land_pic, 0);
    }
    if (p_cities[city].enchants[CITY_ENCH_FLYING_FORTRESS] == 0) {
        if (Tile_Is_Mountain(map_x - 1, map_y - 1, plane) == 1 || Tile_Is_Mountain(map_x, map_y - 1, plane) == 1 ||
            Tile_Is_Mountain(map_x + 1, map_y - 1, plane) == 1 || Tile_Is_Mountain(map_x, map_y, plane) == 1) {
            Set_Pic_Frame(cityscape_mountain_bg_pic, 0);
            Draw_Pic_Clipped(x - 65, y, cityscape_mountain_bg_pic);
        } else if (Tile_Is_Hills(map_x - 1, map_y - 1, plane) == 1 || Tile_Is_Hills(map_x, map_y - 1, plane) == 1 ||
                   Tile_Is_Hills(map_x + 1, map_y - 1, plane) == 1 || Tile_Is_Hills(map_x, map_y, plane) == 1) {
            Set_Pic_Frame(cityscape_mountain_bg_pic, 0);
            Draw_Pic_Clipped(x - 65, y, cityscape_hills_bg_pic);
        }
    }
    if (p_cities[city].enchants[CITY_ENCH_CLOUD_OF_SHADOW]) {
        Set_Pic_Frame(cityscape_far_dark_cloud_pic, 0);
        for (k = 0; k <= cityscape_anim_frame; k++)
            Draw_Pic_Clipped(x - 65, y, cityscape_far_dark_cloud_pic);
    }
    if (p_cities[city].enchants[CITY_ENCH_HEAVENLY_LIGHT]) {
        Set_Pic_Frame(cityscape_far_heavenly_light_pic, 0);
        for (k = 0; k <= cityscape_anim_frame; k++)
            Draw_Pic_Clipped(x - 65, y, cityscape_far_heavenly_light_pic);
    }
    if (p_cities[city].enchants[CITY_ENCH_CHAOS_RIFT]) {
        Set_Pic_Frame(cityscape_far_chaos_rift_pic, 0);
        for (k = 0; k <= cityscape_anim_frame; k++)
            Draw_Pic_Clipped(x - 65, y, cityscape_far_chaos_rift_pic);
    }
    house_style = race_table[p_cities[city].race].house_style;
    Set_Pic_Frame(cityscape_house_pics[house_style][0], 0);
    Draw_Pic_Clipped(x + 30, y + 30, cityscape_house_pics[house_style][0]);
    Reset_Clip_Rect();
}
