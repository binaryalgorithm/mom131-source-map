/* PATHFIND.C - WIZARDS.EXE overlay code segment d592 (stub 2624): the overland path finder - the movement cost map
   for a stack's movement modes, the obstacles, a search window around the two ends for the AI (retried without it
   when it finds nothing), the relaxation (PATHRLX.ASM) and the path read back through the predecessors; the AI's
   path cache (140 records of up to 35 steps); a straight-line variant and a stub.
   Data (DS:6f76): path_cache_index (owner as ReMoM's layout, b127); BSS (DS:ca8a..ca8e): the cache hit / miss counters.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS d592 src/wizards/PATHFIND.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Path_Line_Costs(int x1, int y1, int x2, int y2, int plane, unsigned char *xs, unsigned char *ys,
                    unsigned char *costs, int m0, int m1, int m2, int m3, int m4, int m5);
int Path_Find(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying, int x1, int y1,
              int x2, int y2, int plane, unsigned char *path_x, unsigned char *path_y, unsigned char *path_cost,
              int window, int mode, int needs_transport, int stack_size, int player);
void Path_Bounding_Box(int x1, int y1, int x2, int y2, int *xmin, int *ymin, int *xmax, int *ymax);
int Path_Extract(int mode, int x1, int y1, int x2, int y2, char *out_x, char *out_y, char *out_cost);
void Path_Cost_Map_Select(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying,
                          int plane);
int Path_Uncalled_Returns_2(void);
void Path_Cache_Invalidate(void);
int Path_Cache_Store(int player, int x1, int y1, int x2, int y2, int cache_plane, int xs, int ys, int costs);

#include "GAME.H"

int path_cache_misses;                          /* DS:ca8c */
int path_cache_hits;                            /* DS:ca8a */

int path_cache_index = -1;                      /* DS:6f76  the record found / stored last; -1 none (owner: this module, as in ReMoM's layout, where ovr148's initialized data is this word; clarified using ReMoM repo) */
extern struct PathCacheEntry far *p_path_cache; /* DS:9c90  140 records */
extern struct PathWork far *p_path_work;        /* DS:9ca4 */
extern unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH];   /* DS:9cac  per plane: cavalry, walking, forester,
                                                                  mountaineer, swimming, sailing */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4  2400 bytes per plane */

void EMS_Map_Default(void);
int abs(int x);
int Mem_Copy_Far();                                /* unprototyped (see HELPSCR.C): far pointers or offset, segment pairs */
int Mem_Fill_Far();                             /* unprototyped: called with one far pointer for its (offset, segment) words (UTIL.C) */
int Bytes_Min_Far();                            /* unprototyped: called with far pointers for its (offset, segment) word pairs (UTIL.C) */
void Path_Apply_Obstacles(struct PathWork far *work, int needs_transport, int stack_size, int plane, int player,
                          int x2, int y2, int x1, int y1);
void Path_Relax(int x, int y, struct PathWork far *work);
int Map_Line_Path(int x1, int y1, int x2, int y2, char *xs, char *ys, int width);

/* () -> 2: a stub */
int Path_Uncalled_Returns_2(void)
{
    return 2;
}

/* (x1, y1, x2, y2, plane, xs, ys, costs, the six movement modes) -> steps, 0 when the target is impassable: the
   straight line with each square's move cost. DEAD: no callers */
int Path_Line_Costs(int x1, int y1, int x2, int y2, int plane, unsigned char *xs, unsigned char *ys,
                    unsigned char *costs, int m0, int m1, int m2, int m3, int m4, int m5)
{
    int len;
    int i;

    EMS_Map_Default();
    Path_Cost_Map_Select(m0, m1, m2, m3, m4, m5, plane);
    if (p_path_work->cost[y2][x2] == COST_IMPASSABLE)
        return 0;
    len = Map_Line_Path(x1, y1, x2, y2, xs, ys, MAP_WIDTH);
    for (i = 0; i < len; i++)
        costs[i] = p_path_work->cost[ys[i]][xs[i]];
    return len;
}

/* (the six movement modes, x1, y1, x2, y2, plane, path_x, path_y, path_cost, -, -, needs_transport, stack_size,
   player) -> steps, 0 when the target is impassable: the cheapest path into the three byte buffers (start not
   included). An AI (player != 0) first looks in the path cache, then searches inside the bounding box of the two
   ends (Path_Bounding_Box) and again over the whole map when that finds nothing. The window / mode parameters are
   overwritten at once (the callers' 1 and move count are never read); the 'mode 1' cost flattening for the human
   is therefore dead
   Numbered: BUG #203 (BUGS_AND_QUIRKS.txt) */
int Path_Find(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying, int x1, int y1,
              int x2, int y2, int plane, unsigned char *path_x, unsigned char *path_y, unsigned char *path_cost,
              int window, int mode, int needs_transport, int stack_size, int player)
{
    int x;
    int y;
    int tile;
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    int i;
    int n;

    EMS_Map_Default();
    window = 1;
    mode = 8;
    path_cache_index = -1;
    if (player == PLAYER_HUMAN)
        window = 0;
    else {
        for (i = 0; i < PATH_CACHE_SIZE && p_path_cache[i].x1 != -1; i++) {
            if (p_path_cache[i].x1 == x1 && p_path_cache[i].y1 == y1 && p_path_cache[i].x2 == x2
                && p_path_cache[i].y2 == y2 && p_path_cache[i].plane == plane)
                path_cache_index = i;
        }
        if (path_cache_index != -1) {
            Mem_Copy_Far(path_x, _DS, p_path_cache[path_cache_index].path_x, p_path_cache[path_cache_index].length);
            Mem_Copy_Far(path_y, _DS, p_path_cache[path_cache_index].path_y, p_path_cache[path_cache_index].length);
            Mem_Copy_Far(path_cost, _DS, p_path_cache[path_cache_index].path_cost, p_path_cache[path_cache_index].length);
            path_cache_hits++;
            return p_path_cache[path_cache_index].length;
        } else
            path_cache_misses++;
    }
again:
    Path_Cost_Map_Select(cavalry, forester, mountaineer, swimming, sailing, flying, plane);
    if (player == PLAYER_HUMAN && mode == 1) {
        for (y = 0; y < MAP_HEIGHT; y++)
            for (x = 0; x < MAP_WIDTH; x++)
                if (p_path_work->cost[y][x] != COST_IMPASSABLE && p_path_work->cost[y][x] > 2)
                    p_path_work->cost[y][x] = 2;
    }
    Path_Apply_Obstacles(p_path_work, needs_transport, stack_size, plane, player, x2, y2, x1, y1);
    if (p_path_work->cost[y2][x2] == COST_IMPASSABLE)
        return 0;
    if (window == 1) {
        Path_Bounding_Box(x1, y1, x2, y2, &xmin, &ymin, &xmax, &ymax);
        for (y = 0; y < ymin; y++)
            for (x = 0; x < MAP_WIDTH; x++)
                p_path_work->cost[y][x] = COST_IMPASSABLE;
        for (y = ymax + 1; y < MAP_HEIGHT; y++)
            for (x = 0; x < MAP_WIDTH; x++)
                p_path_work->cost[y][x] = COST_IMPASSABLE;
        if (xmin < xmax) {
            for (y = ymin; y <= ymax; y++) {
                for (x = 0; x < xmin; x++)
                    p_path_work->cost[y][x] = COST_IMPASSABLE;
                for (x = xmax + 1; x < MAP_WIDTH; x++)
                    p_path_work->cost[y][x] = COST_IMPASSABLE;
            }
        } else {
            for (y = ymin; y <= ymax; y++)
                for (x = xmax + 1; x < xmin; x++)
                    p_path_work->cost[y][x] = COST_IMPASSABLE;
        }
        Path_Relax(x1, y1, p_path_work);
        n = 0;
        tile = x2 + y2 * MAP_WIDTH;
        while (p_path_work->prev[tile] != tile) {
            p_path_work->path[n] = tile;
            tile = p_path_work->prev[tile];
            n++;
        }
        for (i = 0; i < n; i++) {
            x = p_path_work->path[n - 1 - i] % MAP_WIDTH;
            y = p_path_work->path[n - 1 - i] / MAP_WIDTH;
            path_x[i] = x;
            path_y[i] = y;
            path_cost[i] = p_path_work->cost[y][x];
        }
        if (n == 0) {
            window = 0;
            goto again;
        }
    }
    if (window == 0) {
        Path_Relax(x1, y1, p_path_work);
        n = 0;
        tile = x2 + y2 * MAP_WIDTH;
        while (p_path_work->prev[tile] != tile) {
            p_path_work->path[n] = tile;
            tile = p_path_work->prev[tile];
            n++;
        }
        for (i = 0; i < n; i++) {
            x = p_path_work->path[n - 1 - i] % MAP_WIDTH;
            y = p_path_work->path[n - 1 - i] / MAP_WIDTH;
            path_x[i] = x;
            path_y[i] = y;
            path_cost[i] = p_path_work->cost[y][x];
        }
    }
    return n;
}

/* (x1, y1, x2, y2, &xmin, &ymin, &xmax, &ymax) Path_Find's search window: the two ends widened by a quarter of
   their distance (5 squares around the wrap side when the ends are more than 30 columns apart); x wraps; ymin is
   kept in 0..39 but ymax never is (BUG #368) */
void Path_Bounding_Box(int x1, int y1, int x2, int y2, int *xmin, int *ymin, int *xmax, int *ymax)
{
    int margin;
    int my;
    int dx;

    my = abs(y1 - y2) / 4;
    if (y1 <= y2) {
        *ymin = y1 - my;
        *ymax = y2 + my;
    } else {
        *ymin = y2 - my;
        *ymax = y1 + my;
    }
    if (*ymin < 0)
        *ymin = 0;
    if (*ymin >= MAP_HEIGHT)
        *ymin = MAP_HEIGHT - 1;
    dx = abs(x1 - x2);
    if (dx > MAP_WIDTH / 2) {
        margin = 5;
        if (x1 <= x2) {
            *xmin = x2 - margin;
            *xmax = x1 + margin;
        } else {
            *xmin = x1 - margin;
            *xmax = x2 + margin;
        }
    } else {
        margin = dx / 4;
        if (x1 <= x2) {
            *xmin = x1 - margin;
            *xmax = x2 + margin;
        } else {
            *xmin = x2 - margin;
            *xmax = x1 + margin;
        }
    }
    if (*xmin < 0)
        *xmin += MAP_WIDTH;
    if (*xmax >= MAP_WIDTH)
        *xmax -= MAP_WIDTH;
}

/* (mode, x1, y1, x2, y2, out x, out y, out cost) -> steps: mode 0 reads the last search's path back through the
   predecessors from (x2, y2); else the straight line with cost 2 per step */
int Path_Extract(int mode, int x1, int y1, int x2, int y2, char *out_x, char *out_y, char *out_cost)
{
    int tile;
    int len;
    int x;
    int y;
    int i;
    int n;

    EMS_Map_Default();
    n = 0;
    if (mode != 0) {
        len = Map_Line_Path(x1, y1, x2, y2, out_x, out_y, MAP_WIDTH);
        for (i = 0; i < len; i++)
            out_cost[i] = 2;
        return len;
    }
    tile = x2 + y2 * MAP_WIDTH;
    while (p_path_work->prev[tile] != tile) {
        p_path_work->path[n] = tile;
        tile = p_path_work->prev[tile];
        n++;
    }
    for (i = 0; i < n; i++) {
        x = p_path_work->path[n - 1 - i] % MAP_WIDTH;
        y = p_path_work->path[n - 1 - i] / MAP_WIDTH;
        out_x[i] = x;
        out_y[i] = y;
        out_cost[i] = p_path_work->cost[y][x];
    }
    return n;
}

/* (cavalry, forester, mountaineer, swimming, sailing, flying, plane) the cost map for the movement modes into
   p_path_work: flying 2 everywhere (0 on squares with terrain flags 18h); sailing the sailing map; forester and
   mountaineer 1 per square (0 / FFh kept from the walking map; 1 everywhere with swimming); else the mode's map
   (cavalry, forester, mountaineer, swimming or walking) and the per-square minimum with the other modes held */
void Path_Cost_Map_Select(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying,
                          int plane)
{
    int x;
    int y;
    int cell;
    char mask;
    int count;
    unsigned char far *terrain_flags;
    unsigned char far *dst;

    EMS_Map_Default();
    if (flying == 1) {
        Mem_Fill_Far(p_path_work, 0x2760, 2);   /* not the whole PathWork (0x2780): the last 0x20 bytes kept */
        terrain_flags = (unsigned char far *)p_terrain_flags_maps[plane];
        dst = (unsigned char far *)p_path_work;
        count = MAP_SQUARES;
        mask = TERRAIN_FLAG_ROAD | TERRAIN_FLAG_ENCHANTED_ROAD;
        for (cell = 0; cell < count; cell++)
            if ((terrain_flags[cell] & mask) == mask)
                dst[cell] = 0;
    } else if (sailing == 1)
        Mem_Copy_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_SAILING], MAP_SQUARES);
    else if (forester == 1 && mountaineer == 1) {
        if (swimming == 0) {
            for (y = 0; y < MAP_HEIGHT; y++) {
                for (x = 0; x < MAP_WIDTH; x++) {
                    if (p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] == 0)
                        p_path_work->cost[y][x] = 0;
                    else if (p_movement_cost_maps[plane][MOVE_COST_WALKING][y][x] != COST_IMPASSABLE)
                        p_path_work->cost[y][x] = 1;
                    else
                        p_path_work->cost[y][x] = COST_IMPASSABLE;
                }
            }
        } else {
            for (y = 0; y < MAP_HEIGHT; y++)
                for (x = 0; x < MAP_WIDTH; x++)
                    p_path_work->cost[y][x] = 1;
        }
    } else if (cavalry == 1) {
        Mem_Copy_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_CAVALRY], MAP_SQUARES);
        if (swimming == 1)
            Bytes_Min_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_SWIMMING], MAP_SQUARES);
        if (forester == 1)
            Bytes_Min_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_FORESTER], MAP_SQUARES);
        if (mountaineer == 1)
            Bytes_Min_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER], MAP_SQUARES);
    } else if (forester == 1) {
        Mem_Copy_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_FORESTER], MAP_SQUARES);
        if (swimming == 1)
            Bytes_Min_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_SWIMMING], MAP_SQUARES);
        if (mountaineer == 1)
            Bytes_Min_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER], MAP_SQUARES);
    } else if (mountaineer == 1) {
        Mem_Copy_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_MOUNTAINEER], MAP_SQUARES);
        if (swimming == 1)
            Bytes_Min_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_SWIMMING], MAP_SQUARES);
    } else {
        if (swimming == 1)
            Mem_Copy_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_SWIMMING], MAP_SQUARES);
        else
            Mem_Copy_Far(p_path_work, p_movement_cost_maps[plane][MOVE_COST_WALKING], MAP_SQUARES);
    }
}

/* () frees the cache record path_cache_index - only 1..139: record 0 is never invalidated (BUG #138) */
void Path_Cache_Invalidate(void)
{
    if (path_cache_index > 0 && path_cache_index < PATH_CACHE_SIZE)
        p_path_cache[path_cache_index].x1 = -1;
}

/* (player, x1, y1, x2, y2, cache_plane, xs, ys, costs) -> 0 when nothing is stored (no return value otherwise): finds the
   two waypoints in the path (up to 35 steps) and, unless that from / to pair is cached already, stores the part
   between them in a free record of the path cache */
int Path_Cache_Store(int player, int x1, int y1, int x2, int y2, int cache_plane, int xs, int ys, int costs)
{
    int to;
    int found;
    int i;
    int from;

    if (player == PLAYER_HUMAN)
        return 0;
    from = -1;
    to = -1;
    for (i = 0; i < PATH_CACHE_MAX_STEPS && (from == -1 || to == -1); i++) {
        if (((char *)xs)[i] == x1 && ((char *)ys)[i] == y1)
            from = i;
        if (((char *)xs)[i] == x2 && ((char *)ys)[i] == y2)
            to = i;
    }
    if (from == -1 || to == -1 || to <= from)
        return 0;
    for (i = 0; i < PATH_CACHE_SIZE && path_cache_index == -1; i++)
        if (p_path_cache[i].x1 == -1)
            path_cache_index = i;
    if (path_cache_index == -1)
        return 0;
    found = 0;
    for (i = 0; i < PATH_CACHE_SIZE && found == 0; i++) {   /* no plane test (the lookup has one): the same ends on
                                                               the other plane are never cached; clarified using
                                                               ReMoM repo */
        if (p_path_cache[i].x1 == x1 && p_path_cache[i].y1 == y1 && p_path_cache[i].x2 == x2
            && p_path_cache[i].y2 == y2)
            found = 1;
    }
    if (found == 1)
        return 0;
    p_path_cache[path_cache_index].x1 = x1;
    p_path_cache[path_cache_index].y1 = y1;
    p_path_cache[path_cache_index].x2 = x2;
    p_path_cache[path_cache_index].y2 = y2;
    p_path_cache[path_cache_index].plane = cache_plane;
    p_path_cache[path_cache_index].length = to - from;
    Mem_Copy_Far(p_path_cache[path_cache_index].path_x, from + xs + 1, _DS, p_path_cache[path_cache_index].length);
    Mem_Copy_Far(p_path_cache[path_cache_index].path_y, from + ys + 1, _DS, p_path_cache[path_cache_index].length);
    Mem_Copy_Far(p_path_cache[path_cache_index].path_cost, from + costs + 1, _DS, p_path_cache[path_cache_index].length);
}
