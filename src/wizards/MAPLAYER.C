/* MAPLAYER.C - WIZARDS.EXE overlay code segment d95b (stub 2632): the layers of the overland map view - the draw
   cache (which view was last drawn, how many more full redraws), the animated terrain (TERRAIN.LBX through EMS; a
   debug overlay prints the landmass ids), the animation counters, the explored-edge pictures, the city markers,
   towers, lairs, the node auras (owned nodes in the owner's color, warped nodes with a dissolving, waving aura),
   the corruption / wild game / hunter's lodge / nightshade specials, the minerals, the roads (enchanted roads
   animated), the units of the view, the minimap picture, the cartographer's whole-plane map and a water test.
   This module's data (DS:6ff4..704b): the animation counters, the draw cache, the minimap / marker colors, the node
   aura wave phases and their saved random seed; BSS (DS:cb58..cc2e): the map pictures loaded by Load_Map_Assets.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS d95b src/wizards/MAPLAYER.C */

/* the module's functions in their original declaration order */
void Map_Invalidate(void);
void MapDraw_State_Save(void);
void MapDraw_State_Restore(void);
void Map_Draw_Layers(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Terrain(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Anim_Tick(void);
void Map_Draw_Fog_Edges(int px, int py, int w, int h, int vx, int vy, int plane);
void Map_Draw_Cities(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Towers(int px, int py, int w, int h, int vx, int vy, int plane);
void Map_Draw_Lairs(int px, int py, int w, int h, int vx, int vy, int plane);
void Map_Draw_Node_Auras(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Specials(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Minerals(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Roads(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Unit_Grid(int px, int py, int w, int h, int vx, int vy);
void Minimap_Build(int ox, int oy, int plane, unsigned pic, int w, int h, int hx, int hy, int hilite);
void Cartographer_Build_Map(int plane, unsigned buf);
int Tile_Is_Water_Gen(int x, int y, int plane);

#include "GAME.H"

int mapdraw_unused_cc2a[2];                     /* DS:cc2a  never referenced; owner open (no effect): between this module and PATHWIN.C (cc2e), either file gives the same EXE */
unsigned terrain_ems_handle;                    /* DS:cc28  EMS handle of TERRAIN.LBX */
unsigned map_tile_gfx_index;                    /* DS:cc26  graphic of the square being drawn: terrain_gfx_table byte + animation frame */
struct TerrainGfx far *terrain_gfx_table;       /* DS:cc22  TERRAIN.LBX 1: EMS page and graphic of every terrain type */
unsigned fog_edge_pics[16];                     /* DS:cc02  [1..15] the fog edge quarters by exploration mask */
unsigned mapdraw_unused_cbdc[19];               /* DS:cbdc  never referenced */
unsigned map_city_unwalled_pic;                  /* DS:cbda  MAPBACK.LBX 21 */
unsigned map_city_walled_pic;                  /* DS:cbd8  MAPBACK.LBX 20 */
unsigned map_tower_owned_pic;                   /* DS:cbd6  MAPBACK 0x46, recolored to the owner */
unsigned map_tower_unowned_pic;                 /* DS:cbd4  MAPBACK 0x45 */
unsigned mapdraw_unused_cbd2;                   /* DS:cbd2  never referenced */
unsigned map_lair_pics[7];                      /* DS:cbc4  [lair type - 4]: MAPBACK 47 4a 48 49 47 4a 4b (cave .. fallen temple) */
unsigned map_road_lone_pic;                        /* DS:cbc2  MAPBACK 0x2d, a lone road dot (before the road set 0x2e..0x35); loaded, never drawn */
unsigned map_road_pics[8];                      /* DS:cbb2  MAPBACK 0x2e..0x35: top, top right, right, .. top left */
unsigned map_eroad_lone_pic;                        /* DS:cbb0  MAPBACK 0x36, the animated lone dot before the enchanted set; never drawn */
unsigned map_eroad_pics[8];                     /* DS:cba0  MAPBACK 0x37..0x3e: the enchanted roads, animated */
unsigned mapdraw_unused_cb9e;                   /* DS:cb9e  never referenced */
unsigned unit_banner_back_pics[6];              /* DS:cb92  MAPBACK.LBX 14..19, a unit's backdrop by banner color */
unsigned node_aura_pic;                         /* DS:cb90  20 x 18 work bitmap of a warped node's aura square */
unsigned map_site_lumber_camp_pic;              /* DS:cb8e  MAPBACK 0x58 'lumber camp' (SITES): a black square in 1.31; loaded, never drawn */
unsigned map_node_aura_pics[6];                 /* DS:cb82  MAPBACK 0x3f..0x43 by banner color ([5] never loaded) */
unsigned map_mineral_pics[10];                  /* DS:cb6e  [1..9] MAPBACK 0x4e..0x56 by mineral */
unsigned map_site_mud_pic;                      /* DS:cb6c  MAPBACK 0x4c 'mud' (SITES): a brown earth texture; loaded, never drawn */
unsigned map_corruption_pic;                    /* DS:cb6a  MAPBACK 0x4d */
unsigned map_site_mine_pic;                     /* DS:cb68  MAPBACK 0x57 'mine' (SITES): a black square in 1.31; loaded, never drawn */
unsigned map_wild_game_pic;                     /* DS:cb66  MAPBACK 0x5c */
unsigned map_nightshade_pic;                    /* DS:cb64  MAPBACK 0x5b */
unsigned map_hunters_lodge_pic;                 /* DS:cb62  MAPBACK 0x5a */
unsigned city_marker_pic;                       /* DS:cb60  work bitmap: a city / tower marker recolored to its owner, the warped-node mask */
unsigned map_warped_mask_pic;                   /* DS:cb5e  MAPBACK 0x5d */
int map_draw_terrain_now;                       /* DS:cb5c  1 = this draw repaints the terrain (and every fog edge) */
int mapdraw_saved_redraws;                      /* DS:cb5a */
int mapdraw_saved_view_x;                       /* DS:cb58 */

int map_anim_frame = 0;                         /* DS:6ff4  0..3: animated terrain frame */
int map_road_anim_frame = 0;                    /* DS:6ff6  0..5: enchanted road frame */
int map_anim_counter = 0;                       /* DS:6ff8  0..29999: node aura frames */
int mapdraw_unused_6ffa = 0;                    /* DS:6ffa  never referenced */
int map_anim_toggle = 0;                        /* DS:6ffc  the counters advance every second draw */
int map_drawn_view_x = -1;                       /* DS:6ffe  view x of the last draw (-1: repaint) */
int map_drawn_view_y = -1;                     /* DS:7000  view y of the last draw */
int map_full_redraws_left = 2;                  /* DS:7002  redraws of an unmoved view that still repaint the terrain */
int debug_show_landmass = 0;                    /* DS:7004  1 prints the landmass id and column on every square */
int mapdraw_unused_7006 = 0;                    /* DS:7006  never referenced */
int mapdraw_unused_7008 = 0;                    /* DS:7008  never referenced */
unsigned char minimap_banner_colors[6] = { 0xab, 0xd8, 0xcd, 0xc9, 0xd2, 0x32 };  /* DS:700a  minimap city color by banner */
unsigned char minimap_neutral_color = 0x32;     /* DS:7010  minimap color of a neutral city */
unsigned char city_marker_colors[7][5] = {      /* DS:7011  the five marker colors (+1) of each banner color */
    { 0x60, 0x61, 0x62, 0x63, 0x64 }, { 0x48, 0x49, 0x4a, 0x4b, 0x4c }, { 0xcd, 0xce, 0xcf, 0xd0, 0xd1 },
    { 0xc9, 0xa5, 0xcb, 0xa6, 0x2d }, { 0xd1, 0xd2, 0xd3, 0xd4, 0xd5 }, { 0x1d, 0x1d, 0x1c, 0x1c, 0x1b },
    { 0x1d, 0x1d, 0x1c, 0x1c, 0x1b }
};
char node_wave_phase[20] = { 0 };               /* DS:7034  -1 / 0 / +1 per row and column of a warped node's aura */
long node_aura_saved_seed = 0x000f9f9bL;        /* DS:7048  the random seed the aura dissolves with (re-captured each draw) */

extern int mapdraw_saved_view_y;              /* DS:cb56 */
extern unsigned ems_frame_seg;                  /* DS:760c */
extern int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];               /* DS:964c  unit, 1000 + city or 1100 + node per view square; -1 */
extern unsigned char far *p_terrain_minimap_colors;        /* DS:9ca8 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];        /* DS:9cb8 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];        /* DS:9cd0 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];             /* DS:9cdc */
extern unsigned p_palette;                      /* DS:e7e8 */

void EMS_Map_Pages(int first, int handle);
void Draw_Bitmap(int x, int y, unsigned pic);
void AI_Map_EMS_Tables(void);
void Font_Select();                             /* no prototype here: this module calls it with 2 arguments (FONT.ASM takes 4) */
int Print_Number(int x, int y, int value);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Replace_Color(unsigned pic, int from, int to);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Screen_Grab_Rect(int x1, int y1, int x2, int y2, unsigned pic);
long Random_Get_Seed(void);
void Random_Seed(long seed);
void Pic_Dissolve(unsigned pic, int amount);
void Pic_Shift_Rows_H(char *shifts, unsigned pic);
void Pic_Shift_Columns_V(char *shifts, unsigned pic);
int Pic_Cut_To_Mask(int x, int y, unsigned dst, unsigned mask);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void far *Seg_To_Far_Ptr(unsigned seg);
void Pic_Create(int width, int height, unsigned pic, int color);
void Pic_Create_Blank(int width, int height, unsigned pic);

/* () the next map draws repaint everything */
void Map_Invalidate(void)
{
    map_drawn_view_x = -1;
    map_drawn_view_y = -1;
}

/* () saves the draw cache and forces a full redraw */
void MapDraw_State_Save(void)
{
    mapdraw_saved_view_x = map_drawn_view_x;
    mapdraw_saved_view_y = map_drawn_view_y;
    mapdraw_saved_redraws = map_full_redraws_left;
    map_drawn_view_x = -1;
}

/* () restores what MapDraw_State_Save saved */
void MapDraw_State_Restore(void)
{
    map_drawn_view_x = mapdraw_saved_view_x;
    map_drawn_view_y = mapdraw_saved_view_y;
    map_full_redraws_left = mapdraw_saved_redraws;
}

/* (px, py, cols, rows, vx, vy, plane) every layer of the map view at screen (px, py); the terrain is repainted when
   the view moved, and for map_full_redraws_left more draws of the same view (a static view only animates) */
void Map_Draw_Layers(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    if (vx != map_drawn_view_x || vy != map_drawn_view_y) {
        map_draw_terrain_now = 1;
        map_full_redraws_left = 1;
    } else {
        map_full_redraws_left--;
        if (map_full_redraws_left < 0)
            map_draw_terrain_now = 0;
        else
            map_draw_terrain_now = 1;
    }
    map_drawn_view_x = vx;
    map_drawn_view_y = vy;
    Map_Draw_Terrain(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Minerals(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Specials(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Roads(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Cities(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Towers(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Lairs(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Unit_Grid(px, py, cols, rows, vx, vy);
    Map_Draw_Node_Auras(px, py, cols, rows, vx, vy, plane);
    Map_Draw_Fog_Edges(px, py, cols, rows, vx, vy, plane);
    Map_Anim_Tick();
}

/* (px, py, cols, rows, vx, vy, plane) the terrain squares from TERRAIN.LBX in EMS (Myrror's terrain types follow
   Arcanus' 762); unexplored squares black; the animated ones every draw, the rest only when map_draw_terrain_now.
   last_page is never updated, so the EMS pages are mapped for every square */
void Map_Draw_Terrain(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    int page;
    int terrain;
    int last_page;
    int wx;
    unsigned gfx;
    int x;
    int y;
    int plane_base;
    register int sy;
    register int sx;

    if (plane == PLANE_ARCANUS)
        plane_base = 0;
    else
        plane_base = TERRAIN_IDS_PER_PLANE;
    sy = py;
    last_page = -1;
    for (y = vy; y < vy + rows; y++) {
        sx = px;
        for (x = vx; x < vx + cols; sx += MAP_TILE_W, x++) {
            wx = x >= MAP_WIDTH ? x - MAP_WIDTH : x;
            if (p_exploration_maps[plane][y][wx] != 0) {
                terrain = p_terrain_maps[plane][y][wx] + plane_base;
                page = 0;
                page = terrain_gfx_table[terrain].ems_page;
                map_tile_gfx_index = terrain_gfx_table[terrain].gfx;
                if (page & TERRAIN_GFX_ANIMATED) {
                    map_tile_gfx_index += map_anim_frame;
                    page = page & TERRAIN_GFX_PAGE;
                } else if (map_draw_terrain_now == 0)
                    continue;
                asm mov ax, map_tile_gfx_index          /* gfx = index * 24 + ems_frame_seg: a terrain picture
                                                   is 24 paragraphs in the EMS frame */
                asm mov bx, ax
                asm shl ax, 1
                asm add ax, bx
                asm shl ax, 1
                asm shl ax, 1
                asm shl ax, 1
                asm add ax, ems_frame_seg
                asm mov gfx, ax
                if (last_page != page)
                    EMS_Map_Pages(page, terrain_ems_handle);
                Draw_Bitmap(sx, sy, gfx);
            } else
                Fill_Rect(sx, sy, sx + MAP_TILE_W - 1, sy + MAP_TILE_H - 1, 0);
            if (debug_show_landmass != 0) {
                AI_Map_EMS_Tables();
                Font_Select(0, 0);
                Print_Number(sx, sy + 12, p_landmass_ids[map_view_plane][y][wx]);
                Font_Select(0, 2);
                Print_Number(sx, sy, wx);
            }
        }
        sy += MAP_TILE_H;
    }
}

/* () every second call: the terrain frame (0..3), the node aura counter and the enchanted road frame (0..5) */
void Map_Anim_Tick(void)
{
    map_anim_toggle = 1 - map_anim_toggle;
    if (map_anim_toggle == 0) {
        map_anim_frame = (map_anim_frame + 1) % 4;
        map_anim_counter = (map_anim_counter + 1) % 30000;
        map_road_anim_frame = (map_road_anim_frame + 1) % 6;
    }
}

/* (px, py, w, h, vx, vy, plane) the edge quarters of the partly explored squares (mask 1..14), on the squares whose
   terrain was repainted this draw */
void Map_Draw_Fog_Edges(int px, int py, int w, int h, int vx, int vy, int plane)
{
    int mask;
    int wx;
    int terrain;
    int sy;
    int sx;
    register int x;
    register int y;

    sy = py;
    for (y = vy; y < vy + h; y++) {
        sx = px;
        for (x = vx; x < vx + w; x++) {
            if (x >= MAP_WIDTH)
                wx = x - MAP_WIDTH;
            else
                wx = x;
            terrain = p_terrain_maps[plane][y][wx];
            if (map_draw_terrain_now == 1 || (terrain_gfx_table[terrain].ems_page & TERRAIN_GFX_ANIMATED) != 0) {
                mask = p_exploration_maps[plane][y][wx];
                if (mask != 0 && mask != EXPLORED_ALL)
                    Draw_Pic_Frame(sx, sy, fog_edge_pics[mask]);
            }
            sx += MAP_TILE_W;
        }
        sy += MAP_TILE_H;
    }
}

/* (px, py, cols, rows, vx, vy, plane) the explored cities' markers (clipped to the map area of the main screen or
   of the other screens): walled or not, sized by population, in the owner's banner colors (neutral: fixed)
   Numbered: BUG #29 (BUGS_AND_QUIRKS.txt) */
void Map_Draw_Cities(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    int x;
    int y;
    int owner;
    int pic;
    int draw_x;
    int draw_y;
    int frame;
    int city;
    int i;

    if (current_screen == SCREEN_CITY)
        Set_Clip_Rect(215, 4, 454, 183);       /* larger than the city screen's 5 x 5 map (x2 314, y2 93): a city one
                                                   square below the view (drawn too, see the test below) can spill
                                                   under the map - whether the screen paints over it is not traced;
                                                   clarified using ReMoM repo */
    else
        Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].plane == plane) {
            x = p_cities[city].x;
            y = p_cities[city].y;
            if (p_exploration_maps[plane][y][x] != 0) {
                x = x - vx;
                if (x < 0)
                    x += MAP_WIDTH;
                y = y - vy;
                if (y + 1 >= 0 && y - 1 < rows && x + 1 >= 0 && x - 1 < cols) {
                    owner = p_cities[city].owner;
                    draw_x = x * MAP_TILE_W + px - 6;
                    draw_y = y * MAP_TILE_H + py - 6;
                    if (owner == PLAYER_NEUTRAL) {
                        pic = p_cities[city].buildings[BLD_CITY_WALLS] != BLDSTATE_REPLACED ? map_city_walled_pic   /* walls -1 (none) and 1 both pick MAPBACK 20, the
                                                   walled picture; only 0 picks 21 (RUNTIME_CHECKS RC1) */
                                                                                      : map_city_unwalled_pic;
                        frame = (p_cities[city].population - 1) / 4;
                        if (frame > 4)
                            frame = 4;
                        Set_Pic_Frame(pic, frame);
                        Pic_Decode_Frame_To_Bitmap(pic, city_marker_pic);
                        for (i = 0; i < 5; i++)
                            Pic_Replace_Color(city_marker_pic, i + CITY_MARKER_PAL_BASE, i + NEUTRAL_MARKER_COLOR_BASE);
                    } else {
                        pic = p_cities[city].buildings[BLD_CITY_WALLS] != BLDSTATE_REPLACED ? map_city_walled_pic
                                                                                      : map_city_unwalled_pic;
                        frame = (p_cities[city].population - 1) / 4;
                        if (frame > 4)
                            frame = 4;
                        Set_Pic_Frame(pic, frame);
                        Pic_Decode_Frame_To_Bitmap(pic, city_marker_pic);
                        for (i = 0; i < 5; i++)
                            Pic_Replace_Color(city_marker_pic, i + CITY_MARKER_PAL_BASE,
                                              city_marker_colors[wizards[owner].banner_color][i] - 1);
                    }
                    Draw_Bitmap_Clipped(draw_x, draw_y, city_marker_pic);
                }
            }
        }
    }
}

/* (px, py, w, h, vx, vy, plane) the explored towers in the view: unowned, or the owned picture with its five shades
   all set to the color NUMBER banner * 5 + 0xaa instead of a banner table entry (BUG #344) */
void Map_Draw_Towers(int px, int py, int w, int h, int vx, int vy, int plane)
{
    int y;
    int owner;
    int k;
    int draw_x;
    int draw_y;
    register int x;
    register int tower;

    for (tower = 0; tower < NUM_TOWERS; tower++) {
        x = p_towers[tower].x;
        y = p_towers[tower].y;
        if (p_exploration_maps[plane][y][x] != 0) {
            x = x - vx;
            if (x < 0)
                x += MAP_WIDTH;
            y = y - vy;
            if (y >= 0 && y < h && x >= 0 && x < w) {
                owner = p_towers[tower].owner;
                draw_x = x * MAP_TILE_W + px;
                draw_y = y * MAP_TILE_H + py;
                if (owner == PLAYER_NONE)
                    Pic_Decode_Frame_To_Bitmap(map_tower_unowned_pic, city_marker_pic);
                else {
                    Pic_Decode_Frame_To_Bitmap(map_tower_owned_pic, city_marker_pic);
                    for (k = 0; k < 5; k++)
                        Pic_Replace_Color(city_marker_pic, k + 0xe0, wizards[owner].banner_color * 5 + 0xaa);
                }
                Draw_Bitmap(draw_x, draw_y, city_marker_pic);
            }
        }
    }
}

/* (px, py, w, h, vx, vy, plane) the explored, uncleared lairs (types 4..10) of the plane in the view */
void Map_Draw_Lairs(int px, int py, int w, int h, int vx, int vy, int plane)
{
    int y;
    int draw_x;
    int draw_y;
    register int lair;
    register int x;

    for (lair = 0; lair < NUM_LAIRS; lair++) {
        if (p_lairs[lair].plane == plane && p_lairs[lair].active == 1 && p_lairs[lair].type > LAIR_SORCERY_NODE) {
            x = p_lairs[lair].x;
            y = p_lairs[lair].y;
            if (p_exploration_maps[plane][y][x] != 0) {
                x = x - vx;
                if (x < 0)
                    x += MAP_WIDTH;
                y = y - vy;
                if (y >= 0 && y < h && x >= 0 && x < w) {
                    draw_x = x * MAP_TILE_W + px;
                    draw_y = y * MAP_TILE_H + py;
                    Draw_Pic_Frame(draw_x, draw_y, map_lair_pics[p_lairs[lair].type - LAIR_MYSTERIOUS_CAVE]);
                }
            }
        }
    }
}

/* (px, py, cols, rows, vx, vy, plane) with the 'show node owners' option: each owned node's explored aura squares in
   the owner's color; a warped node's own square gets the warped mask over a copy of the screen that is dissolved
   (with node_aura_saved_seed - re-captured every draw, the frame test being
   always true, so the pattern changes each draw) and waved by the row / column phases, which drift every draw */
void Map_Draw_Node_Auras(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    int node;
    int owner;
    int k;
    int power;                                  /* set, never used */
    unsigned pic;
    int i;
    int draw_x;
    int draw_y;
    int phase;
    long seed;
    register int x;
    register int y;

    for (node = 0; node < NUM_NODES; node++) {
        if (p_nodes[node].plane == plane) {
            owner = p_nodes[node].owner;
            if (owner != PLAYER_NONE && game_config.show_node_owners == 1) {
                pic = map_node_aura_pics[wizards[owner].banner_color];
                power = p_nodes[node].power;
                for (k = 0; k < p_nodes[node].power; k++) {
                    x = p_nodes[node].aura_x[k];
                    y = p_nodes[node].aura_y[k];
                    if (p_exploration_maps[plane][y][x] != 0) {
                        x = x - vx;
                        if (x < 0)
                            x += MAP_WIDTH;
                        y = y - vy;
                        if (y >= 0 && y < rows && x >= 0 && x < cols) {
                            draw_x = x * MAP_TILE_W + px;
                            draw_y = y * MAP_TILE_H + py;
                            Set_Pic_Frame(pic, (map_anim_counter + k) % 6);
                            Draw_Pic_Frame(draw_x, draw_y, pic);
                        }
                    }
                }
                if ((p_nodes[node].meld_flags & NODE_WARPED) > 0) {
                    x = p_nodes[node].aura_x[0];
                    y = p_nodes[node].aura_y[0];
                    if (p_exploration_maps[plane][y][x] != 0) {
                        x = x - vx;
                        if (x < 0)
                            x += MAP_WIDTH;
                        y = y - vy;
                        if (y >= 0 && y < rows && x >= 0 && x < cols) {
                            draw_x = x * MAP_TILE_W + px;
                            draw_y = y * MAP_TILE_H + py;
                            Set_Pic_Frame(map_warped_mask_pic, 0);
                            Pic_Decode_Frame_To_Bitmap(map_warped_mask_pic, city_marker_pic);
                            Set_Pic_Frame(map_warped_mask_pic, 0);
                            Screen_Grab_Rect(draw_x, draw_y, draw_x + MAP_TILE_W - 1, draw_y + MAP_TILE_H - 1, node_aura_pic);
                            if (map_anim_frame >= 0) {   /* always true (0 .. 3) */
                                node_aura_saved_seed = Random_Get_Seed();
                                for (i = 0; i < 20; i++) {
                                    phase = node_wave_phase[i];
                                    if (Random(4) == 1)
                                        phase++;
                                    if (Random(4) == 1)
                                        phase--;
                                    if (phase < -1)
                                        phase = -1;
                                    if (phase > 1)
                                        phase = 1;
                                    node_wave_phase[i] = phase;
                                }
                            }
                            seed = Random_Get_Seed();
                            Random_Seed(node_aura_saved_seed);
                            Pic_Dissolve(node_aura_pic, 50);
                            Random_Seed(seed);
                            Pic_Shift_Rows_H(node_wave_phase, node_aura_pic);
                            Pic_Shift_Columns_V(node_wave_phase, node_aura_pic);
                            Pic_Cut_To_Mask(0, 0, node_aura_pic, city_marker_pic);
                            Draw_Bitmap(draw_x, draw_y, node_aura_pic);
                        }
                    }
                }
            }
        }
    }
}

/* (px, py, cols, rows, vx, vy, plane) on explored squares: corruption; wild game (with the hunter's lodge) unless a
   city stands there; nightshade */
void Map_Draw_Specials(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    int wx;
    int x;
    int sy;
    int sx;
    int found;
    unsigned char terrain_flags;
    unsigned char mineral;
    register int y;
    register int city;

    sy = py;
    for (y = vy; y < vy + rows; y++) {
        sx = px;
        for (x = vx; x < vx + cols; x++) {
            wx = x >= MAP_WIDTH ? x - MAP_WIDTH : x;
            if (p_exploration_maps[plane][y][wx] != 0) {
                terrain_flags = p_terrain_flags_maps[plane][y][wx];
                if (terrain_flags & TERRAIN_FLAG_CORRUPTION)
                    Draw_Pic_Frame(sx, sy, map_corruption_pic);
                mineral = p_mineral_maps[plane][y][wx];
                if (mineral & MINFLAG_WILD_GAME) {
                    found = 0;
                    for (city = 0; city < num_cities; city++)
                        if (p_cities[city].x == wx && p_cities[city].y == y && p_cities[city].plane == plane)
                            found = 1;
                    if (found == 0) {
                        Draw_Pic_Frame(sx, sy, map_wild_game_pic);
                        if (mineral & MINFLAG_HUNTERS_LODGE)
                            Draw_Pic_Frame(sx, sy, map_hunters_lodge_pic);
                    }
                }
                if (mineral & MINFLAG_NIGHTSHADE)
                    Draw_Pic_Frame(sx, sy, map_nightshade_pic);
            }
            sx += MAP_TILE_W;
        }
        sy += MAP_TILE_H;
    }
}

/* (px, py, cols, rows, vx, vy, plane) the minerals (1..9) of the explored squares without a city */
void Map_Draw_Minerals(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    int wx;
    int found;
    int y;
    int sy;
    int sx;
    int mineral;
    unsigned char far *mineral_row;
    unsigned char far *explored_row;
    register int city;
    register int x;

    sy = py;
    for (y = vy; y < vy + rows; y++) {
        sx = px;
        mineral_row = p_mineral_maps[plane][y];
        explored_row = p_exploration_maps[plane][y];
        for (x = vx; x < vx + cols; x++) {
            if (x >= MAP_WIDTH)
                wx = x - MAP_WIDTH;
            else
                wx = x;
            if (explored_row[wx] != 0) {
                mineral = mineral_row[wx] & MINERAL_KIND_MASK;
                if (mineral != 0) {
                    found = 0;
                    for (city = 0; city < num_cities; city++)
                        if (p_cities[city].x == wx && p_cities[city].y == y && p_cities[city].plane == plane)
                            found = 1;
                    if (found == 0)
                        Draw_Pic_Frame(sx, sy, map_mineral_pics[mineral]);
                }
            }
            sx += MAP_TILE_W;
        }
        sy += MAP_TILE_H;
    }
}

/* (px, py, cols, rows, vx, vy, plane) on each explored road square, a road piece toward every neighbour with a road
   (enchanted roads animated); the neighbours are read unwrapped (BUG #27: column 0 sees the previous row's east end, column 59
   the next row's west end) */
void Map_Draw_Roads(int px, int py, int cols, int rows, int vx, int vy, int plane)
{
    int wx;
    int x;
    int y;
    unsigned char road;
    unsigned char enchanted;
    unsigned char far *square;
    register int sy;
    register int sx;

    sy = py;
    for (y = vy; y < vy + rows; y++) {
        sx = px;
        for (x = vx; x < vx + cols; x++) {
            wx = x >= MAP_WIDTH ? x - MAP_WIDTH : x;
            if (p_exploration_maps[plane][y][wx] != 0) {
                square = &p_terrain_flags_maps[plane][y - 1][wx - 1];   /* the 3 x 3 block in the 60-wide map (NB_*) */
                road = square[NB_SELF] & TERRAIN_FLAG_ROAD;
                if (road != 0) {
                    enchanted = square[NB_SELF] & TERRAIN_FLAG_ENCHANTED_ROAD;
                    if (enchanted == 0) {
                        if (square[NB_N] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[0]);
                        if (square[NB_NE] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[1]);
                        if (square[NB_E] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[2]);
                        if (square[NB_SE] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[3]);
                        if (square[NB_S] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[4]);
                        if (square[NB_SW] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[5]);
                        if (square[NB_W] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[6]);
                        if (square[NB_NW] & TERRAIN_FLAG_ROAD)
                            Draw_Pic_Frame(sx, sy, map_road_pics[7]);
                    } else {
                        if (square[NB_N] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[0], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[0]);
                        }
                        if (square[NB_NE] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[1], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[1]);
                        }
                        if (square[NB_E] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[2], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[2]);
                        }
                        if (square[NB_SE] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[3], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[3]);
                        }
                        if (square[NB_S] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[4], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[4]);
                        }
                        if (square[NB_SW] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[5], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[5]);
                        }
                        if (square[NB_W] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[6], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[6]);
                        }
                        if (square[NB_NW] & TERRAIN_FLAG_ROAD) {
                            Set_Pic_Frame(map_eroad_pics[7], map_road_anim_frame);
                            Draw_Pic_Frame(sx, sy, map_eroad_pics[7]);
                        }
                    }
                }
            }
            sx += MAP_TILE_W;
        }
        sy += MAP_TILE_H;
    }
}

/* (px, py, w, h, vx, vy) the unit (not city / node) of each square of map_unit_grid; a 3-wide window reads the grid
   at its offset from the main view */
void Map_Draw_Unit_Grid(int px, int py, int w, int h, int vx, int vy)
{
    int row;
    int sx;
    int sy;
    int unit;
    int row0;
    register int col0;
    register int col;

    if (w == 3) {                               /* Map_Redraw_Block's 3-wide window */
        col0 = vx - map_view_x;
        if (col0 < 0)
            col0 += MAP_WIDTH;
        if (col0 >= MAP_WIDTH)
            col0 -= MAP_WIDTH;
        row0 = vy - map_view_y;
    } else {
        col0 = 0;
        row0 = 0;
    }
    sy = py;
    for (row = 0; row < h; row++) {
        sx = px;
        for (col = 0; col < w; col++) {
            unit = map_unit_grid[row + row0][col + col0];
            if (unit != -1 && unit < GRID_CITY_BASE)
                Draw_Unit_Icon(sx, sy, unit, UNIT_ICON_MAP);
            else
                ;
            sx += MAP_TILE_W;
        }
        sy += MAP_TILE_H;
    }
}

/* (ox, oy, plane, pic, w, h, hx, hy, hilite) the w x h minimap picture of the map from (ox, oy): the explored
   squares' terrain colors, the cities in their owner's color, and with hilite a white dot at (hx, hy) (BUG #467: a y
   past the bottom is corrected on x) */
void Minimap_Build(int ox, int oy, int plane, unsigned pic, int w, int h, int hx, int hy, int hilite)
{
    int i;
    int j;
    int mx;
    int terrain;
    int my;
    int city;
    int owner;
    int plane_base;
    unsigned char (far *explored)[MAP_WIDTH];
    unsigned (far *terrain_map)[MAP_WIDTH];
    unsigned char far *dest;
    register int cx;
    register int cy;

    if (plane == PLANE_ARCANUS)
        plane_base = 0;
    else
        plane_base = TERRAIN_IDS_PER_PLANE;
    explored = p_exploration_maps[plane];
    terrain_map = p_terrain_maps[plane];
    dest = Seg_To_Far_Ptr(pic + 1);
    Pic_Create(w, h, pic, 1);
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            my = j + oy;
            mx = i + ox;
            if (mx < 0)
                mx += MAP_WIDTH;
            if (mx >= MAP_WIDTH)
                mx -= MAP_WIDTH;
            if (my >= 0 && my < MAP_HEIGHT && explored[my][mx] != 0) {
                terrain = terrain_map[my][mx] + plane_base;
                dest[i * h + j] = p_terrain_minimap_colors[terrain];
            }
        }
    }
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].plane == plane) {
            cx = p_cities[city].x;
            cy = p_cities[city].y;
            if (explored[cy][cx] != 0) {
                cx = cx - ox;
                if (cx < 0)
                    cx += MAP_WIDTH;
                cy = cy - oy;
                if (cy >= 0 && cy < h && cx >= 0 && cx < w) {
                    owner = p_cities[city].owner;
                    if (owner == PLAYER_NEUTRAL)
                        dest[cx * h + cy] = minimap_neutral_color;
                    else
                        dest[cx * h + cy] = minimap_banner_colors[wizards[owner].banner_color];
                }
            }
        }
    }
    if (hilite != 0) {
        cx = hx - ox;
        cy = hy - oy;
        if (cx < 0)
            cx += MAP_WIDTH;
        if (cx >= MAP_WIDTH)
            cx -= MAP_WIDTH;
        if (cy < 0)
            cy += MAP_HEIGHT;
        if (cy >= MAP_HEIGHT)
            cx -= MAP_HEIGHT;                   /* the BUG #467 of the header: x, not y */
        if (cy >= 0 && cy < h && cx >= 0 && cx < w)
            dest[cx * h + cy] = 15;             /* white */
    }
}

/* (plane, buf) the cartographer's 240 x 160 map of a whole plane (4 x 4 pixels per square) in the picture at
   buf + 0x10: a 256-entry remap table at buf:0 first maps the palette onto the ~32 shades 0xe0.. (0xe0 + (r + g + b / 2) / 5
   of the 6-bit values; 0x66 / 0x67 to 0), every explored square samples 4 x 4 pixels of its terrain graphic through that table, then the partly
   explored squares get black edges by their exploration mask */
void Cartographer_Build_Map(int plane, unsigned buf)
{
    int page;
    int terrain;
    int last_page;
    unsigned k;
    int pix;
    unsigned y;
    unsigned gfx;
    int sum;
    int mask;
    int i;
    unsigned block;
    char far *palette;
    char far *remap;
    register unsigned x;

    block = buf + 0x10;
    Pic_Create_Blank(240, 160, block);
    palette = Seg_To_Far_Ptr(p_palette);
    remap = Seg_To_Far_Ptr(buf);
    i = 0;
    for (k = 0; k < 0x300; k += 3) {
        if (i == 0x66)
            remap[i] = 0;
        else if (i == 0x67)
            remap[i] = 0;
        else {
            sum = palette[k] + palette[k + 1] + (palette[k + 2] >> 1);
            remap[i] = sum / 5 + 0xe0;
        }
        i++;
    }
    last_page = -1;
    pix = 0x110;
    for (x = 0; x < MAP_WIDTH; x++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            if (p_exploration_maps[plane][y][x] != 0) {
                terrain = p_terrain_maps[plane][y][x];
                page = 0;
                page = terrain_gfx_table[terrain].ems_page;
                map_tile_gfx_index = terrain_gfx_table[terrain].gfx;
                if (page & TERRAIN_GFX_ANIMATED) {
                    map_tile_gfx_index += map_anim_frame;
                    page = page & TERRAIN_GFX_PAGE;
                }
                asm mov ax, map_tile_gfx_index          /* gfx = index * 24 + ems_frame_seg: a terrain picture
                                                   is 24 paragraphs in the EMS frame */
                asm mov bx, ax
                asm shl ax, 1
                asm add ax, bx
                asm shl ax, 1
                asm shl ax, 1
                asm shl ax, 1
                asm add ax, ems_frame_seg
                asm mov gfx, ax
                if (last_page != page)
                    EMS_Map_Pages(page, terrain_ems_handle);
                asm push es
                asm push ds
                asm push di
                asm mov di, pix
                asm mov ax, gfx
                asm mov bx, buf
                asm inc ax
                asm mov ds, ax
                asm mov es, bx
                asm mov bx, 0
                asm mov bl, ds:[0]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[5]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[10]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[15]
                asm mov al, es:[bx]
                asm stosb
                asm add di, 156
                asm mov bl, ds:[90]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[95]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[100]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[105]
                asm mov al, es:[bx]
                asm stosb
                asm add di, 156
                asm mov bl, ds:[180]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[185]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[190]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[195]
                asm mov al, es:[bx]
                asm stosb
                asm add di, 156
                asm mov bl, ds:[270]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[275]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[280]
                asm mov al, es:[bx]
                asm stosb
                asm mov bl, ds:[285]
                asm mov al, es:[bx]
                asm stosb
                asm pop di
                asm pop ds
                asm pop es
            }
            pix += 4;
        }
        pix += 0x1e0;
    }
    pix = 0;
    for (x = 0; x < MAP_WIDTH; x++) {
        for (y = 0; y < MAP_HEIGHT; y++) {
            mask = p_exploration_maps[plane][y][x];
            if (mask != 0 && mask != EXPLORED_ALL) {
                switch (mask) {
                case EXPLORED_SW:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm mov es:[di+0B0h], al
                    asm mov es:[di+150h], al
                    asm mov es:[di+1F0h], al
                    asm mov es:[di+1F1h], al
                    asm mov es:[di+1F2h], al
                    asm mov es:[di+1F3h], al
                    asm mov es:[di+151h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_NW:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+13h], al
                    asm mov es:[di+0B3h], al
                    asm mov es:[di+153h], al
                    asm mov es:[di+1F0h], al
                    asm mov es:[di+1F1h], al
                    asm mov es:[di+1F2h], al
                    asm mov es:[di+1F3h], al
                    asm mov es:[di+152h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SW | EXPLORED_NW:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+1F0h], al
                    asm mov es:[di+1F1h], al
                    asm mov es:[di+1F2h], al
                    asm mov es:[di+1F3h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_NE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm mov es:[di+11h], al
                    asm mov es:[di+12h], al
                    asm mov es:[di+13h], al
                    asm mov es:[di+0B3h], al
                    asm mov es:[di+153h], al
                    asm mov es:[di+1F3h], al
                    asm mov es:[di+0B2h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SW | EXPLORED_NE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+13h], al
                    asm mov es:[di+1F0h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_NW | EXPLORED_NE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+13h], al
                    asm mov es:[di+0B3h], al
                    asm mov es:[di+153h], al
                    asm mov es:[di+1F3h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SW | EXPLORED_NW | EXPLORED_NE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+1F3h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm mov es:[di+11h], al
                    asm mov es:[di+12h], al
                    asm mov es:[di+13h], al
                    asm mov es:[di+0B0h], al
                    asm mov es:[di+150h], al
                    asm mov es:[di+1F0h], al
                    asm mov es:[di+0B1h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SW | EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm mov es:[di+0B0h], al
                    asm mov es:[di+150h], al
                    asm mov es:[di+1F0h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_NW | EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm mov es:[di+1F3h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SW | EXPLORED_NW | EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+13h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_NE | EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm mov es:[di+11h], al
                    asm mov es:[di+12h], al
                    asm mov es:[di+13h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_SW | EXPLORED_NE | EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+10h], al
                    asm pop di
                    asm pop es
                    break;
                case EXPLORED_NW | EXPLORED_NE | EXPLORED_SE:
                    asm push es
                    asm push di
                    asm mov al, 0
                    asm mov di, pix
                    asm mov bx, block
                    asm mov es, bx
                    asm mov es:[di+1F0h], al
                    asm pop di
                    asm pop es
                    break;
                }
            }
            pix += 4;
        }
        pix += 0x1e0;
    }
}

/* (x, y, plane) 1 for water (terrain mod 0x2fa: ocean and shores 0, 2 .. 0xa1 and the shore variants 0xc5 .. 0xe8,
   0x1c4 .. 0x1d3, 0x1d9 .. 0x259), 0 for terrain 1 and land; no callers in WIZARDS (MAGIC's world generator has
   the same function) */
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
