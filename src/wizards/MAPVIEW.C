/* MAPVIEW.C - WIZARDS.EXE code segment 5a8c: the map view's origin (centring on a tile, the city screen's 5 x 5 window,
   a minimap window), the fog of war (drawing the black / edge quarters, exploring a tile and smoothing the edge
   quarters of the 5 x 5 tiles around it), the main map's city markers and the blinking minimap of the list screens.
   This module's data (DS:3278): Fog_Fill_Corner's initialiser; BSS (DS:c124..c127).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 5a8c src/wizards/MAPVIEW.C */

/* the module's functions in their original declaration order */
void Map_Center_View(int *view_x, int *view_y, int x, int y, int plane);
void Map_Center_City_View(int *view_x, int *view_y, int x, int y, int plane);
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
void Map_Draw_Fog(int px, int py, int w, int h, int vx, int vy, int plane);
void Map_Draw_Cities_Move_Anim(int scr_x, int scr_y, int cols, int rows, int view_x, int view_y, int plane);
int Map_Explore_Tile(int x, int y, int plane);
void Fog_Fill_Corner(int x, int y, int plane);
int Fog_Smooth(int x, int y, int plane);
void Minimap_Draw_Blink(int px, int py, int w, int h, int plane, int cx, int cy);

#include "GAME.H"

#define CITY_MAP_SIZE 5                 /* the city screen's map: 5 x 5 squares round the city */

int minimap_blink_tick;                         /* DS:c126  0..7; the dot shows for 0..3 */
int mapview_unused_c124;                        /* DS:c124  never referenced; owner open (no effect): this word sits between MAPDRAW.C (c11c) and this module, either file gives the same EXE */

extern unsigned char city_marker_colors[][5];   /* DS:7011  the five marker colors (+1) of each banner color */
extern int map_scroll_px_y;                     /* DS:9746 */
extern int map_scroll_px_x;                     /* DS:9748 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern unsigned city_marker_pic;                /* DS:cb60 */
extern unsigned map_city_walled_pic;           /* DS:cbd8 */
extern unsigned map_city_unwalled_pic;           /* DS:cbda */
extern unsigned fog_edge_pics[16];              /* DS:cc02  [1..15] the fog edge quarters by exploration mask (Load_Map_Assets) */

void Stack_Clear_Draw_Priority(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Fill_Rect_Clipped(int x1, int y1, int x2, int y2, int color);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Replace_Color(unsigned pic, int from, int to);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Minimap_Build(int ox, int oy, int plane, unsigned pic, int w, int h, int hx, int hy, int hilite);
void Draw_Bitmap(int x, int y, unsigned pic);

/* (&view_x, &view_y, x, y, plane) the map view's top-left corner for tile (x, y): x - 6 wrapped at 60, y - 5 kept in
   0..30; also the pixel scroll and the unit grid */
void Map_Center_View(int *view_x, int *view_y, int x, int y, int plane)
{
    int left;
    int top;

    if (x >= MAP_WIDTH)
        x = x - MAP_WIDTH;
    if (x < 0)
        x = x + MAP_WIDTH;
    top = y;
    left = x - MAP_VIEW_COLS / 2;
    if (left > 0)
        *view_x = left;
    else
        *view_x = (left + MAP_WIDTH) % MAP_WIDTH;
    top = y - MAP_VIEW_ROWS / 2;
    if (top > 0)
        *view_y = top;
    else
        *view_y = 0;
    if (*view_y + MAP_VIEW_ROWS >= MAP_HEIGHT)
        *view_y = MAP_HEIGHT - MAP_VIEW_ROWS;
    map_scroll_target_x = *view_x;
    map_scroll_target_y = *view_y;
    map_scroll_px_x = map_scroll_target_x * MAP_TILE_W;
    map_scroll_px_y = map_scroll_target_y * MAP_TILE_H;
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(*view_x, *view_y, plane);
}

/* (&view_x, &view_y, x, y, plane) the same for the city screen's 5 x 5 map: x - 2, y - 2 kept in 0..35 */
void Map_Center_City_View(int *view_x, int *view_y, int x, int y, int plane)
{
    int left;
    int top;

    top = y;
    left = x - CITY_MAP_SIZE / 2;
    if (left > 0)
        *view_x = left;
    else
        *view_x = (left + MAP_WIDTH) % MAP_WIDTH;
    top = y - CITY_MAP_SIZE / 2;
    if (top > 0)
        *view_y = top;
    else
        *view_y = 0;
    if (*view_y + CITY_MAP_SIZE >= MAP_HEIGHT)
        *view_y = MAP_HEIGHT - CITY_MAP_SIZE;
    map_scroll_target_x = *view_x;
    map_scroll_target_y = *view_y;
    map_scroll_px_x = map_scroll_target_x * MAP_TILE_W;
    map_scroll_px_y = map_scroll_target_y * MAP_TILE_H;
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(*view_x, *view_y, plane);
}

/* (&ox, &oy, cx, cy, w, h) the top-left of a w x h window centred on (cx, cy); x wraps at 60, y is not clamped */
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h)
{
    int top;
    int left;

    top = cy;
    left = cx - w / 2;
    if (left > 0)
        *ox = left;
    else
        *ox = (left + MAP_WIDTH) % MAP_WIDTH;
    top = cy - h / 2;
    *oy = top;
}

/* (px, py, w, h, vx, vy, plane) the fog over a w x h tile window at screen (px, py): black where unexplored, the
   edge quarters where partly explored (drawn last, over everything) */
void Map_Draw_Fog(int px, int py, int w, int h, int vx, int vy, int plane)
{
    int mask;
    int wx;
    int terrain;                                /* read, never used */
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
            mask = p_exploration_maps[plane][y][wx];
            if (mask != 0 && mask != EXPLORED_ALL)
                Draw_Pic_Frame(sx, sy, fog_edge_pics[mask]);
            if (mask == 0)
                Fill_Rect_Clipped(sx, sy, sx + 19, sy + 17, 0);
            sx += 20;
        }
        sy += 18;
    }
}

/* (scr_x, scr_y, cols, rows, view_x, view_y, plane) the explored cities' markers on the main map (clipped to the map
   area; MAPLAYER.C's Map_Draw_Cities with that fixed clip, for the move animation): the walled or unwalled marker, sized by population, in the owner's banner colors (neutral: fixed) */
void Map_Draw_Cities_Move_Anim(int scr_x, int scr_y, int cols, int rows, int view_x, int view_y, int plane)
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

    Set_Clip_Rect(0, MAP_VIEW_TOP, MAP_VIEW_RIGHT, SCREEN_MAX_Y);
    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].plane == plane) {
            x = p_cities[city].x;
            y = p_cities[city].y;
            if (p_exploration_maps[plane][y][x] != 0) {
                x = x - view_x;
                if (x < 0)
                    x += MAP_WIDTH;
                y = y - view_y;
                if (y + 1 >= 0 && y - 1 < rows && x + 1 >= 0 && x - 1 < cols) {
                    owner = p_cities[city].owner;
                    draw_x = x * MAP_TILE_W + scr_x - 6;
                    draw_y = y * MAP_TILE_H + scr_y - 6;
                    if (owner == PLAYER_NEUTRAL) {
                        pic = p_cities[city].buildings[BLD_CITY_WALLS] != BLDSTATE_REPLACED ? map_city_walled_pic   /* as in MAPLAYER.C (RUNTIME_CHECKS RC1) */
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

/* (x, y, plane) -> 0 on row 0 (only the tile itself is marked), else no return value (AX = Fog_Smooth's): the tile becomes fully explored and its eight
   neighbours gain the facing quarters, then the corners around them are filled and the edges smoothed. BUG #27: at x 0, 1 and 59 x_start wraps (58 /
   59 / 57) but x_end does not, so the two row loops do not run there: no corner fill beside the map's seam */
int Map_Explore_Tile(int x, int y, int plane)
{
    int right;
    int left;
    int below;
    int above;
    int x_end;
    int x_start;
    int y_bottom;                               /* set, never used */
    int y_top;
    int cx;
    unsigned char (far *map)[60];

    map = p_exploration_maps[plane];
    if (y == 0) {
        map[y][x] = EXPLORED_ALL;
        return 0;
    }
    if (y == 0) {                               /* never: y 0 returned above */
        above = 0;
        y_top = 0;
    } else {
        above = y - 1;
        y_top = y - 2;
        if (y_top < 0)
            y_top = 0;
    }
    if (y == MAP_HEIGHT - 1) {
        below = MAP_HEIGHT - 1;
        y_bottom = MAP_HEIGHT - 2;
    } else {
        below = y + 1;
        y_bottom = y + 2;
        if (y_bottom >= MAP_HEIGHT)
            y_bottom = MAP_HEIGHT - 1;
    }
    if (x == 0) {
        left = MAP_WIDTH - 1;
        x_start = MAP_WIDTH - 2;
    } else {
        left = x - 1;
        x_start = x - 2;
        if (x_start < 0)
            x_start = MAP_WIDTH - 1;
    }
    if (x == MAP_WIDTH - 1) {
        right = 0;
        x_end = 1;
    } else {
        right = x + 1;
        x_end = x + 2;
    }
    map[above][left] = map[above][left] | EXPLORED_SE;
    map[above][x] = map[above][x] | (EXPLORED_SW | EXPLORED_SE);
    map[above][right] = map[above][right] | EXPLORED_SW;
    map[y][left] = map[y][left] | (EXPLORED_NE | EXPLORED_SE);
    map[y][x] = EXPLORED_ALL;
    map[y][right] = map[y][right] | (EXPLORED_SW | EXPLORED_NW);
    map[below][left] = map[below][left] | EXPLORED_NE;
    map[below][x] = map[below][x] | (EXPLORED_NW | EXPLORED_NE);
    map[below][right] = map[below][right] | EXPLORED_NW;
    Fog_Smooth(x, y, plane);
    for (cx = x_start; cx < x_end; cx++)
        Fog_Fill_Corner(cx, y_top, plane);
    Fog_Fill_Corner(right, above, plane);
    Fog_Fill_Corner(right, y, plane);
    for (cx = x_start; cx < x_end; cx++)
        Fog_Fill_Corner(cx, below, plane);
    Fog_Fill_Corner(x_start, above, plane);
    Fog_Fill_Corner(x_start, y, plane);
    Fog_Smooth(x, y, plane);
}

/* (x, y, plane) when all four tiles of the 2 x 2 block at (x, y) are partly explored in one of the corner patterns
   5 / 7 / 10 / 11 / 13 / 14, their inner quarters are filled. BUG #468: right = x + 1 and down = y + 1 are not wrapped:
   column 60 is the next row's column 0, row 40 is Myrror's row 0 (from Arcanus) or past the maps (from Myrror) -
   read, and written when all four match; clarified using ReMoM repo */
void Fog_Fill_Corner(int x, int y, int plane)
{
    char corner_values[6] = { 5, 7, 10, 11, 13, 14 };
    int right;
    int down;
    unsigned char (far *map)[60];
    int count;
    int i;

    map = p_exploration_maps[plane];
    right = x + 1;
    down = y + 1;
    count = 0;
    for (i = 0; i < 6; i++) {
        if (map[y][x] == corner_values[i]) {
            count++;
            break;
        }
    }
    if (count != 0) {
        for (i = 0; i < 6; i++) {
            if (map[y][right] == corner_values[i]) {
                count++;
                break;
            }
        }
        if (count != 1) {
            for (i = 0; i < 6; i++) {
                if (map[down][x] == corner_values[i]) {
                    count++;
                    break;
                }
            }
            if (count != 2) {
                for (i = 0; i < 6; i++) {
                    if (map[down][right] == corner_values[i]) {
                        count++;
                        break;
                    }
                }
                if (count == 4) {
                    map[y][x] = map[y][x] | EXPLORED_SE;
                    map[y][right] = map[y][right] | EXPLORED_SW;
                    map[down][x] = map[down][x] | EXPLORED_NE;
                    map[down][right] = map[down][right] | EXPLORED_NW;
                }
            }
        }
    }
}

/* (x, y, plane) the edge quarters of the 5 x 5 tiles around (x, y) made consistent with their right / lower / left
   neighbour by a table of 38 rules: a tile whose mask is next to (dir 0 right, 1 down, 2 left) one with neighbour_mask
   gains add_mask, the neighbour neighbour_add_mask; an int function with no return value (Map_Explore_Tile passes on whatever is in AX) */
int Fog_Smooth(int x, int y, int plane)
{
    int k;
    int col;
    int x_start;
    int ny;
    int rule_count;
    int row;
    int mask;
    int mask2;
    unsigned char (far *map)[60];
    struct FogRule rules[38];
    int nx;
    int wx;
    int wy;

    rule_count = 38;
    rules[0].mask = 1; rules[0].neighbour_mask = 8; rules[0].add_mask = 9; rules[0].neighbour_add_mask = 9; rules[0].dir = FOG_DIR_RIGHT;
    rules[1].mask = 11; rules[1].neighbour_mask = 13; rules[1].add_mask = 15; rules[1].neighbour_add_mask = 15; rules[1].dir = FOG_DIR_RIGHT;
    rules[2].mask = 7; rules[2].neighbour_mask = 14; rules[2].add_mask = 15; rules[2].neighbour_add_mask = 15; rules[2].dir = FOG_DIR_RIGHT;
    rules[3].mask = 2; rules[3].neighbour_mask = 12; rules[3].add_mask = 6; rules[3].neighbour_add_mask = 15; rules[3].dir = FOG_DIR_RIGHT;
    rules[4].mask = 7; rules[4].neighbour_mask = 11; rules[4].add_mask = 15; rules[4].neighbour_add_mask = 15; rules[4].dir = FOG_DIR_DOWN;
    rules[5].mask = 2; rules[5].neighbour_mask = 9; rules[5].add_mask = 3; rules[5].neighbour_add_mask = 11; rules[5].dir = FOG_DIR_DOWN;
    rules[6].mask = 6; rules[6].neighbour_mask = 9; rules[6].add_mask = 15; rules[6].neighbour_add_mask = 15; rules[6].dir = FOG_DIR_DOWN;
    rules[7].mask = 15; rules[7].neighbour_mask = 13; rules[7].add_mask = 15; rules[7].neighbour_add_mask = 15; rules[7].dir = FOG_DIR_DOWN;
    rules[8].mask = 3; rules[8].neighbour_mask = 8; rules[8].add_mask = 11; rules[8].neighbour_add_mask = 9; rules[8].dir = FOG_DIR_RIGHT;
    rules[9].mask = 2; rules[9].neighbour_mask = 8; rules[9].add_mask = 6; rules[9].neighbour_add_mask = 9; rules[9].dir = FOG_DIR_RIGHT;
    rules[10].mask = 7; rules[10].neighbour_mask = 3; rules[10].add_mask = 15; rules[10].neighbour_add_mask = 3; rules[10].dir = FOG_DIR_RIGHT;
    rules[11].mask = 15; rules[11].neighbour_mask = 14; rules[11].add_mask = 15; rules[11].neighbour_add_mask = 15; rules[11].dir = FOG_DIR_RIGHT;
    rules[12].mask = 15; rules[12].neighbour_mask = 13; rules[12].add_mask = 15; rules[12].neighbour_add_mask = 15; rules[12].dir = FOG_DIR_RIGHT;
    rules[13].mask = 11; rules[13].neighbour_mask = 11; rules[13].add_mask = 11; rules[13].neighbour_add_mask = 15; rules[13].dir = FOG_DIR_DOWN;
    rules[14].mask = 9; rules[14].neighbour_mask = 13; rules[14].add_mask = 9; rules[14].neighbour_add_mask = 15; rules[14].dir = FOG_DIR_DOWN;
    rules[15].mask = 3; rules[15].neighbour_mask = 15; rules[15].add_mask = 15; rules[15].neighbour_add_mask = 15; rules[15].dir = FOG_DIR_RIGHT;
    rules[16].mask = 4; rules[16].neighbour_mask = 9; rules[16].add_mask = 15; rules[16].neighbour_add_mask = 15; rules[16].dir = FOG_DIR_DOWN;
    rules[17].mask = 15; rules[17].neighbour_mask = 7; rules[17].add_mask = 15; rules[17].neighbour_add_mask = 15; rules[17].dir = FOG_DIR_LEFT;
    rules[18].mask = 15; rules[18].neighbour_mask = 13; rules[18].add_mask = 15; rules[18].neighbour_add_mask = 15; rules[18].dir = FOG_DIR_DOWN;
    rules[19].mask = 15; rules[19].neighbour_mask = 11; rules[19].add_mask = 15; rules[19].neighbour_add_mask = 15; rules[19].dir = FOG_DIR_LEFT;
    rules[20].mask = 15; rules[20].neighbour_mask = 3; rules[20].add_mask = 15; rules[20].neighbour_add_mask = 15; rules[20].dir = FOG_DIR_LEFT;
    rules[21].mask = 14; rules[21].neighbour_mask = 15; rules[21].add_mask = 15; rules[21].neighbour_add_mask = 15; rules[21].dir = FOG_DIR_DOWN;
    rules[22].mask = 7; rules[22].neighbour_mask = 15; rules[22].add_mask = 15; rules[22].neighbour_add_mask = 15; rules[22].dir = FOG_DIR_DOWN;
    rules[23].mask = 2; rules[23].neighbour_mask = 1; rules[23].add_mask = 3; rules[23].neighbour_add_mask = 3; rules[23].dir = FOG_DIR_DOWN;
    rules[24].mask = 6; rules[24].neighbour_mask = 1; rules[24].add_mask = 15; rules[24].neighbour_add_mask = 3; rules[24].dir = FOG_DIR_DOWN;
    rules[25].mask = 11; rules[25].neighbour_mask = 3; rules[25].add_mask = 15; rules[25].neighbour_add_mask = 3; rules[25].dir = FOG_DIR_RIGHT;
    rules[26].mask = 2; rules[26].neighbour_mask = 4; rules[26].add_mask = 6; rules[26].neighbour_add_mask = 6; rules[26].dir = FOG_DIR_RIGHT;
    rules[27].mask = 6; rules[27].neighbour_mask = 8; rules[27].add_mask = 14; rules[27].neighbour_add_mask = 12; rules[27].dir = FOG_DIR_DOWN;
    rules[28].mask = 4; rules[28].neighbour_mask = 8; rules[28].add_mask = 12; rules[28].neighbour_add_mask = 12; rules[28].dir = FOG_DIR_DOWN;
    rules[29].mask = 12; rules[29].neighbour_mask = 14; rules[29].add_mask = 12; rules[29].neighbour_add_mask = 15; rules[29].dir = FOG_DIR_RIGHT;
    rules[30].mask = 12; rules[30].neighbour_mask = 13; rules[30].add_mask = 12; rules[30].neighbour_add_mask = 15; rules[30].dir = FOG_DIR_RIGHT;
    rules[31].mask = 3; rules[31].neighbour_mask = 4; rules[31].add_mask = 7; rules[31].neighbour_add_mask = 6; rules[31].dir = FOG_DIR_RIGHT;
    rules[32].mask = 1; rules[32].neighbour_mask = 12; rules[32].add_mask = 9; rules[32].neighbour_add_mask = 15; rules[32].dir = FOG_DIR_RIGHT;
    rules[33].mask = 6; rules[33].neighbour_mask = 15; rules[33].add_mask = 15; rules[33].neighbour_add_mask = 15; rules[33].dir = FOG_DIR_DOWN;
    rules[34].mask = 9; rules[34].neighbour_mask = 12; rules[34].add_mask = 9; rules[34].neighbour_add_mask = 15; rules[34].dir = FOG_DIR_DOWN;
    rules[35].mask = 15; rules[35].neighbour_mask = 12; rules[35].add_mask = 15; rules[35].neighbour_add_mask = 15; rules[35].dir = FOG_DIR_DOWN;
    rules[36].mask = 15; rules[36].neighbour_mask = 2; rules[36].add_mask = 15; rules[36].neighbour_add_mask = 6; rules[36].dir = FOG_DIR_DOWN;
    rules[37].mask = 15; rules[37].neighbour_mask = 5; rules[37].add_mask = 15; rules[37].neighbour_add_mask = 15; rules[37].dir = FOG_DIR_DOWN;
    map = p_exploration_maps[plane];
    x_start = x - 2;
    if (x_start < 0)
        x_start = x_start + MAP_WIDTH;
    wx = x_start;
    wy = y - 2;
    if (wy < 1)
        wy = 1;
    for (row = 0; row < 5; row++) {
        for (col = 0; col < 5; col++) {
            wx = x_start + col;
            if (wx >= MAP_WIDTH)
                wx = wx - MAP_WIDTH;
            mask = map[wy][wx];
            for (k = 0; k < rule_count; k++) {
                if (rules[k].mask == mask) {
                    ny = wy;
                    nx = wx;
                    switch (rules[k].dir) {
                    case FOG_DIR_RIGHT:
                        nx = wx + 1;
                        if (nx >= MAP_WIDTH)
                            nx = nx - MAP_WIDTH;
                        break;
                    case FOG_DIR_DOWN:
                        ny++;
                        if (ny >= MAP_HEIGHT)
                            ny = MAP_HEIGHT - 1;
                        break;
                    case FOG_DIR_LEFT:
                        nx = wx - 1;
                        if (nx < 0)
                            nx = MAP_WIDTH - 1;
                        break;
                    }
                    mask2 = map[ny][nx];
                    if (rules[k].neighbour_mask == mask2) {
                        map[wy][wx] = mask | rules[k].add_mask;
                        map[ny][nx] = mask2 | rules[k].neighbour_add_mask;
                    }
                }
            }
        }
        wy++;
        if (wy >= MAP_HEIGHT)
            wy = MAP_HEIGHT - 1;
    }
}

/* (px, py, w, h, plane, cx, cy) the list screens' minimap: a w x h window centred on (cx, cy) with the centre dot
   blinking (4 frames on, 4 off) */
void Minimap_Draw_Blink(int px, int py, int w, int h, int plane, int cx, int cy)
{
    int ox;
    int oy;
    int dot;

    minimap_blink_tick = (minimap_blink_tick + 1) % 8;
    dot = 0;
    if (minimap_blink_tick < 4)
        dot = 1;
    Map_Window_Origin(&ox, &oy, cx, cy, w, h);
    Minimap_Build(ox, oy, plane, p_minimap_pic, w, h, cx, cy, dot);
    Reset_Clip_Rect();
    Draw_Bitmap(px, py, p_minimap_pic);
}
