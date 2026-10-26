/* MAPDRAW.C - WIZARDS.EXE overlay code segment 59bd (stub 244d): drawing the overland map view - one scroll step of
   the view toward a target with the minimap, the map layers, the selected stack's blink and the fog; the unit grid
   (which stack, city or node is drawn on each visible square), the draw priority of units, the minimap with its
   view brackets, a small block redraw under a moving figure, and the tower test.
   No data of its own; BSS (DS:c11c..c124): the minimap size and origin.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 59bd src/wizards/MAPDRAW.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty, int player);
void Map_Normalize_Scroll_Target(void);
void Stack_Blink_Off(void);
void Stack_Blink_Hide(void);
void Stack_Blink_Reset(void);
void Stack_Blink_Draw(int vx, int vy, int plane);
void Minimap_Set_Size(int width, int height);
void Minimap_Draw(int x, int y, int w, int h);
void Map_Build_Unit_Grid(int x, int y, int plane);
void Map_Grid_Add_Nodes(int vx, int vy, int plane);
void Map_Redraw_Block(int px, int py, int plane, int vx, int vy);
void Units_Set_Draw_Priority(void);
void Stack_Clear_Draw_Priority(void);
int Tile_Has_Tower(int x, int y);
void Map_Uncalled_Nop_0227(int unused);

#include "GAME.H"

int minimap_origin_x;                           /* DS:c122  map x of the minimap's left column */
int minimap_origin_y;                           /* DS:c120  map y of its top row */
int minimap_w;                                  /* DS:c11e */
int minimap_h;                                  /* DS:c11c */

extern int stack_blink_state;                   /* DS:9442  0..7 blinking, -1 steady, -2 hidden */
extern int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];               /* DS:964c  unit, 1000 + city or 1100 + node per view square; -1 */
extern int map_scroll_px_y;                     /* DS:9746 */
extern int map_scroll_px_x;                     /* DS:9748 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned p_minimap_pic;                  /* DS:c086 */

void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
void Minimap_Build(int ox, int oy, int plane, unsigned pic, int w, int h, int hx, int hy, int hilite);
void Map_Draw_Layers(int px, int py, int cols, int rows, int vx, int vy, int plane);
void Map_Draw_Fog(int px, int py, int w, int h, int vx, int vy, int plane);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void Draw_Bitmap(int x, int y, unsigned pic);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
int Tile_Visible(int x, int y, int plane);
void MapDraw_State_Save(void);
void MapDraw_State_Restore(void);
int Unit_Is_Invisible(int unit);
int Planar_Seal_Active(void);

/* (px, py, w, h, &vx, &vy, plane, tx, ty, player): moves the view (vx, vy) one square toward (tx, ty) - x the short
   way round the wrap, never past the target - and then, when it moved, rebuilds the draw priorities and the unit
   grid (and the current player's map_view); draws the minimap, the map layers, the stack blink (only when it did
   not move) and the fog */
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty, int player)
{
    int dir;
    int wrap;
    int moved;
    int x;
    int y;

    x = *vx;
    y = *vy;
    moved = 0;
    if (tx != x) {
        moved = 1;
        wrap = 0;
        if (tx < x)
            dir = 0;
        if (tx > x)
            dir = 1;
        if (tx > 50 && x < 20) {                /* far apart across the wrap seam: the short way round */
            dir = 0;
            wrap = 1;
        }
        if (x > 50 && tx < 20) {
            dir = 1;
            wrap = 1;
        }
        if (dir == 0) {
            map_scroll_px_x -= MAP_TILE_W;
            x--;
            if (x < 0) {
                x = MAP_WIDTH - 1;
                map_scroll_px_x = x * MAP_TILE_W;
            }
            if (wrap == 0 && x < tx) {
                x = tx;
                map_scroll_px_x = x * MAP_TILE_W;
            }
        } else {
            map_scroll_px_x += MAP_TILE_W;
            x++;
            if (x == MAP_WIDTH) {
                x = 0;
                map_scroll_px_x = 0;
            }
            if (wrap == 0 && x > tx) {
                x = tx;
                map_scroll_px_x = x * MAP_TILE_W;
            }
        }
    }
    if (ty != y) {
        moved = 1;
        if (ty < y) {
            map_scroll_px_y -= MAP_TILE_H;
            y--;
            if (y < ty)
                y = ty;
        } else {
            map_scroll_px_y += MAP_TILE_H;
            y++;
            if (y > ty)
                y = ty;
        }
    }
    if (moved == 1) {
        Units_Set_Draw_Priority();
        Stack_Clear_Draw_Priority();
        Map_Build_Unit_Grid(x, y, plane);
        if (player == current_player) {
            map_view_x = x;
            map_view_y = y;
        }
    }
    Map_Window_Origin(&minimap_origin_x, &minimap_origin_y, (x + MAP_VIEW_COLS / 2) % MAP_WIDTH, y + MAP_VIEW_ROWS / 2, minimap_w, minimap_h);
    Minimap_Build(minimap_origin_x, minimap_origin_y, plane, p_minimap_pic, minimap_w, minimap_h, 0, 0, 0);
    Map_Draw_Layers(px, py, w, h, x, y, plane);
    if (moved == 0)
        Stack_Blink_Draw(x, y, plane);
    Map_Draw_Fog(px, py, w, h, x, y, plane);
    Reset_Clip_Rect();
    *vx = x;
    *vy = y;
}

/* () wraps map_scroll_target_x into 0..59 and keeps map_scroll_target_y in 0..30; a view at x 0 with the drawn x past 49 becomes
   x 60 */
void Map_Normalize_Scroll_Target(void)
{
    if (map_scroll_target_y + MAP_VIEW_ROWS >= MAP_HEIGHT)
        map_scroll_target_y = MAP_HEIGHT - MAP_VIEW_ROWS;
    if (map_scroll_target_x >= MAP_WIDTH)
        map_scroll_target_x = map_scroll_target_x - MAP_WIDTH;
    if (map_scroll_target_x < 0)
        map_scroll_target_x = map_scroll_target_x + MAP_WIDTH;
    if (map_scroll_target_y < 0)
        map_scroll_target_y = 0;
    if (map_view_x == 0 && map_scroll_target_x > MAP_WIDTH - MAP_VIEW_COLS + 1)
        map_view_x = MAP_WIDTH;
}

/* (unused) an empty stub */
void Map_Uncalled_Nop_0227(int unused)
{
    int copy;

    copy = unused;
}

/* () the stack blink steady (-1) */
void Stack_Blink_Off(void)
{
    stack_blink_state = -1;
}

/* () the stack blink hidden (-2) */
void Stack_Blink_Hide(void)
{
    stack_blink_state = -2;
}

/* () restarts the selected stack's blink when a stack is selected and units can still move; invalidates the map */
void Stack_Blink_Reset(void)
{
    if (no_units_to_move == 0 && stack_unit_count > 0)
        stack_blink_state = 0;
    Map_Invalidate();
}

/* (vx, vy, plane) the selected stack's blink: steps the phase (mod 8); in the lit half (or steady) draws its first
   active unit and the minimap with the stack's dot. BUG #28 (59bd:032a): the steady case looks the square up in
   map_unit_grid with the PIXEL coordinates Map_Square_To_Screen returned
   Numbered: BUG #28 (BUGS_AND_QUIRKS.txt) */
void Stack_Blink_Draw(int vx, int vy, int plane)
{
    int x;
    int y;
    int found;
    int visible;
    int unit;
    int i;

    if (stack_blink_state == -2)
        return;
    if (p_units[stack_units[0].unit].owner == PLAYER_NONE)
        return;
    unit = stack_units[0].unit;
    if (p_units[unit].plane != plane && p_units[unit].in_tower != 1)
        return;
    x = p_units[unit].x;
    y = p_units[unit].y;
    visible = Map_Square_To_Screen(vx, vy, &x, &y);
    if (stack_blink_state != -1)
        stack_blink_state = (stack_blink_state + 1) % 8;
    if ((stack_blink_state == -1 || stack_blink_state > 3)
        && ((stack_blink_state == -1 && map_unit_grid[y - vy][x] == -1) || stack_blink_state != -1)) {
        if (visible == 1) {
            found = 0;
            for (i = 0; i < stack_unit_count; i++) {
                if (stack_units[i].active == 1) {
                    found = i;
                    break;
                }
            }
            unit = stack_units[found].unit;
            Draw_Unit_Icon(x, y, unit, UNIT_ICON_MAP);
        }
        unit = stack_units[0].unit;
        Minimap_Build(minimap_origin_x, minimap_origin_y, plane, p_minimap_pic, minimap_w, minimap_h, p_units[unit].x,
                      p_units[unit].y, 1);
    }
}

/* (width, height) of the minimap in squares */
void Minimap_Set_Size(int width, int height)
{
    minimap_w = width;
    minimap_h = height;
}

/* (x, y, w, h) the minimap picture with the corner brackets of the 12 x 10 view (color 13) */
void Minimap_Draw(int x, int y, int w, int h)
{
    int color;
    int cx;
    int cy;

    color = 13;
    Reset_Clip_Rect();
    Draw_Bitmap(x, y, p_minimap_pic);
    cx = w / 2;
    cy = h / 2;
    cx -= 6;
    cy -= 5;
    cx = x + cx;
    cy = y + cy;
    Draw_Line(cx - 1, cy - 1, cx + 1, cy - 1, color);
    Draw_Line(cx - 1, cy - 1, cx - 1, cy + 1, color);
    Draw_Line(cx + 10, cy - 1, cx + 12, cy - 1, color);
    Draw_Line(cx + 12, cy - 1, cx + 12, cy + 1, color);
    Draw_Line(cx - 1, cy + 10, cx - 1, cy + 8, color);
    Draw_Line(cx - 1, cy + 10, cx + 1, cy + 10, color);
    Draw_Line(cx + 12, cy + 10, cx + 12, cy + 8, color);
    Draw_Line(cx + 12, cy + 10, cx + 10, cy + 10, color);
}

/* (x, y, plane) map_unit_grid for the 12 x 10 view at (x, y): per square the visible unit with the highest draw
   priority (priority 0 - the selected stack - only while its blink is steady); an explored city (1000 + city) then
   overwrites its square, units or not */
void Map_Build_Unit_Grid(int x, int y, int plane)
{
    int row;
    int ux;
    int uy;
    int found;
    int occupant;
    int i;

    for (row = 0; row < MAP_VIEW_ROWS; row++)
        for (i = 0; i < MAP_VIEW_COLS; i++)
            map_unit_grid[row][i] = -1;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner != PLAYER_NONE && Tile_Visible(p_units[i].x, p_units[i].y, plane) == 1
            && (p_units[i].plane == plane || p_units[i].in_tower == 1)) {
            uy = p_units[i].y;
            if (uy >= y && y + MAP_VIEW_ROWS > uy) {
                ux = p_units[i].x;
                if (ux >= x && x + MAP_VIEW_COLS > ux) {
                    occupant = map_unit_grid[uy - y][ux - x];
                    if (occupant != -1) {
                        if (p_units[i].draw_priority > p_units[occupant].draw_priority)
                            map_unit_grid[uy - y][ux - x] = i;
                    } else if (p_units[i].draw_priority > 0)
                        map_unit_grid[uy - y][ux - x] = i;
                    else if (p_units[i].draw_priority == 0 && stack_blink_state == -1)
                        map_unit_grid[uy - y][ux - x] = i;
                } else {
                    ux += MAP_WIDTH;
                    if (ux >= x && x + MAP_VIEW_COLS > ux) {
                        occupant = map_unit_grid[uy - y][ux - x];
                        if (occupant != -1) {
                            if (p_units[i].draw_priority > p_units[occupant].draw_priority)
                                map_unit_grid[uy - y][ux - x] = i;
                        } else if (p_units[i].draw_priority > 0)
                            map_unit_grid[uy - y][ux - x] = i;
                        else if (p_units[i].draw_priority == 0 && stack_blink_state == -1)
                            map_unit_grid[uy - y][ux - x] = i;
                    }
                }
            }
        }
    }
    for (i = 0; i < num_cities; i++) {
        if ((p_cities[i].plane == plane || p_cities[i].plane == PLANE_BOTH)
            && p_exploration_maps[p_cities[i].plane][p_cities[i].y][p_cities[i].x] != 0) {
            uy = p_cities[i].y;
            if (uy >= y && y + MAP_VIEW_ROWS > uy) {
                found = 0;
                uy = p_cities[i].y - y;
                ux = p_cities[i].x;
                if (ux >= x && x + MAP_VIEW_COLS > ux) {
                    ux -= x;
                    found = 1;
                } else {
                    ux += MAP_WIDTH;
                    if (ux >= x && x + MAP_VIEW_COLS > ux) {
                        ux -= x;
                        found = 1;
                    }
                }
                if (found == 1)
                    map_unit_grid[uy][ux] = i + GRID_CITY_BASE;
            }
        }
    }
}

/* (vx, vy, plane) the explored nodes of the plane into map_unit_grid as 1100 + node (spell targeting) */
void Map_Grid_Add_Nodes(int vx, int vy, int plane)
{
    int ny;
    int found;
    int node;
    int nx;

    for (node = 0; node < NUM_NODES; node++) {
        if (p_nodes[node].plane == plane
            && p_exploration_maps[p_nodes[node].plane][p_nodes[node].y][p_nodes[node].x] != 0) {
            ny = p_nodes[node].y;
            if (ny >= vy && vy + 10 > ny) {
                found = 0;
                ny = p_nodes[node].y - vy;
                nx = p_nodes[node].x;
                if (nx >= vx && vx + 12 > nx) {
                    nx -= vx;
                    found = 1;
                } else {
                    nx += MAP_WIDTH;
                    if (nx >= vx && vx + 12 > nx) {
                        nx -= vx;
                        found = 1;
                    }
                }
                if (found == 1)
                    map_unit_grid[ny][nx] = node + GRID_NODE_BASE;
            }
        }
    }
}

/* (px, py, plane, vx, vy) redraws the 3 x 3 squares (fewer at the map's bottom edge or the view's) around the pixel
   (px, py) of the view at (vx, vy) - under a moving figure */
void Map_Redraw_Block(int px, int py, int plane, int vx, int vy)
{
    int x0;
    int y0;
    int col;
    int w;
    int h;
    int row;

    w = 3;
    h = 3;
    x0 = px - MAP_TILE_W;
    y0 = py - MAP_TILE_H;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    col = (vx + x0 / MAP_TILE_W) % MAP_WIDTH;
    row = vy + (y0 - MAP_VIEW_TOP) / MAP_TILE_H;
    if (row + h >= MAP_HEIGHT - 1)
        h--;
    if (row - vy + h > 10)
        h--;
    if (row + h >= MAP_HEIGHT - 1)
        row = MAP_HEIGHT - (h + 1);
    MapDraw_State_Save();
    Map_Invalidate();
    if (row - map_view_y < 0) {                 /* the global view, not vy: right only while the caller's view is it */
        row++;
        y0 += 18;
    }
    Map_Draw_Layers(x0, y0, w, h, col, row, plane);
    MapDraw_State_Restore();
}

/* () every unit's draw priority: its type's melee + ranged (at least 1), +50 for a transport, -1 when dead or an
   invisible unit of another player */
void Units_Set_Draw_Priority(void)
{
    int strength;
    int unit;

    for (unit = 0; unit < num_units; unit++) {
        strength = unit_types[p_units[unit].type].melee + unit_types[p_units[unit].type].ranged;
        if (strength == 0)
            strength = 1;
        if (unit_types[p_units[unit].type].transport_capacity > 0)
            strength += 50;
        if (p_units[unit].owner == PLAYER_NONE)
            strength = -1;
        if (p_units[unit].owner != current_player && Unit_Is_Invisible(unit) == 1)
            strength = -1;
        p_units[unit].draw_priority = strength;
    }
}

/* () draw priority 0 for the selected stack (drawn by the blink) */
void Stack_Clear_Draw_Priority(void)
{
    int unit;
    int slot;

    for (slot = 0; slot < stack_unit_count; slot++) {
        unit = stack_units[slot].unit;
        p_units[unit].draw_priority = 0;
    }
}

/* (x, y) -> 1 when a tower stands there and Planar Seal is not in effect */
int Tile_Has_Tower(int x, int y)
{
    int i;
    int found;

    found = 0;
    for (i = 0; i < NUM_TOWERS && found == 0; i++) {
        if (p_towers[i].x == x && p_towers[i].y == y)
            found = 1;
    }
    if (found == 1 && Planar_Seal_Active() == 1)
        found = 0;
    return found;
}
