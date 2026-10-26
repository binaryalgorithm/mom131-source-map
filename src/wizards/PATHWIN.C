/* PATHWIN.C - WIZARDS.EXE code segment db07 (MAGIC.EXE 503f, the same file): a windowed pathfinder that is never
   called in either program (DEAD CODE: Path_Find_Windowed has no caller). It plans over 5 x 5 blocks of the 60 x 40
   map (Path_Win_Refine on the block grid, or a straight Map_Line_Path), then finds each leg inside a window with
   Path_Win_Search - a Bellman-Ford style relaxation over a w x h cost grid (FFh = impassable) using per-region
   neighbour offset tables (corners, edges, interior). Results: x / y byte lists and the per-step costs.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS db07 src/wizards/PATHWIN.C */
#include "ENUMS.H"

#define PATH_WIN_SIZE 15                /* Path_Win_Leg's window: 3 x 3 blocks of REGION_SIZE squares */

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
int Path_Win_Search(int from_x, int from_y, int to_x, int to_y, int w, int h, unsigned char *map, char *out_x, char *out_y);
int Path_Find_Windowed(int from_x, int from_y, int to_x, int to_y, unsigned char (far *map)[60], unsigned char far *block_map, char *out_x, char *out_y, char *out_cost, int budget, int mode);
int Path_Win_Leg(int from_x, int from_y, int to_x, int to_y, char *out_x, char *out_y, unsigned char (far *map)[60]);
int Path_Win_Blocks(int x1, int y1, int x2, int y2, char *out_x, char *out_y, unsigned char far *map);
int Path_Win_Block_Edges(int bx1, int by1, int bx2, int by2, int *edge_x, int *edge_y, unsigned char (far *map)[MAP_WIDTH]);

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int path_win_prev[225];                         /* DS:cdd8  MAGIC DS:92c2   */
unsigned char path_win_cost[226];               /* DS:ccf6  MAGIC DS:91e0   */
                                                /* 15 x 15 = 225 cells used; the extra byte is padding (layout) */
int path_win_list[100];                         /* DS:cc2e  MAGIC DS:9118   */


int path_win_edge_ofs[5] = { 2, 1, 3, 0, 4 };   /* DS:704c  tile offsets along a 5 x 5 block edge, centre first */


/* (from_x, from_y, to_x, to_y, w, h, costs, out_x, out_y) shortest path inside a w x h window over the byte cost grid
   (FFh = blocked; a cell's cost is paid when entering it): relaxes every cell against its neighbours until nothing
   changes, then walks the predecessors back from the target. Returns the steps written to out_x / out_y (the start
   cell not included), 0 if either end is blocked or the target is unreachable */
int Path_Win_Search(int from_x, int from_y, int to_x, int to_y, int w, int h, unsigned char *map, char *out_x,
                    char *out_y)
{
    int i;
    int changed;
    int n;
    int old_prev;
    int row;
    int col;
    int w2;
    int ncost;
    int h2;
    int cost;
    int wh;
    unsigned char newcost;
    int path_len;
    int nb_all[8];
    int nb_left[8];
    int nb_right[8];
    int nb_top_left[8];
    int nb_top_right[8];
    int nb_bottom_left[8];
    int nb_bottom_right[8];
    int nb_top[8];
    int nb_bottom[8];
    int cell;
    int k;

    nb_all[0] = -w - 1;
    nb_all[1] = -w;
    nb_all[2] = -w + 1;
    nb_all[3] = -1;
    nb_all[4] = 1;
    nb_all[5] = w - 1;
    nb_all[6] = w;
    nb_all[7] = w + 1;
    nb_left[0] = -w;
    nb_left[1] = -w + 1;
    nb_left[2] = 1;
    nb_left[3] = w;
    nb_left[4] = w + 1;
    nb_right[0] = -w - 1;
    nb_right[1] = -w;
    nb_right[2] = -1;
    nb_right[3] = w;
    nb_right[4] = w - 1;
    nb_top_left[0] = 1;
    nb_top_left[1] = w;
    nb_top_left[2] = w + 1;
    nb_top_right[0] = -1;
    nb_top_right[1] = w;
    nb_top_right[2] = w - 1;
    nb_bottom_left[0] = 1;
    nb_bottom_left[1] = -w;
    nb_bottom_left[2] = -w + 1;
    nb_bottom_right[0] = -1;
    nb_bottom_right[1] = -w;
    nb_bottom_right[2] = -w - 1;
    nb_top[0] = -1;
    nb_top[1] = 1;
    nb_top[2] = w;
    nb_top[3] = w - 1;
    nb_top[4] = w + 1;
    nb_bottom[0] = -1;
    nb_bottom[1] = 1;
    nb_bottom[2] = -w;
    nb_bottom[3] = -w - 1;
    nb_bottom[4] = -w + 1;
    path_len = 0;
    if (map[to_y * w + to_x] == COST_IMPASSABLE)
        return 0;
    if (map[from_y * w + from_x] == COST_IMPASSABLE)
        return 0;
    wh = w * h;
    for (i = 0; i < wh; i++)
        path_win_prev[i] = i;
    for (i = 0; i < wh; i++)
        path_win_cost[i] = COST_IMPASSABLE;
    path_win_cost[from_x + from_y * w] = 0;
    changed = 1;
    while (changed == 1) {
        changed = 0;
        w2 = w - 2;
        h2 = h - 2;
        cell = 0;
        cost = map[cell];
        if (cost != COST_IMPASSABLE) {
            old_prev = path_win_prev[cell];
            for (k = 0; k < 3; k++) {
                n = cell + nb_top_left[k];
                ncost = path_win_cost[n];
                if (ncost != COST_IMPASSABLE) {
                    newcost = ncost + cost;
                    if (path_win_cost[cell] > newcost) {
                        path_win_prev[cell] = n;
                        path_win_cost[cell] = newcost;
                        if (path_win_prev[cell] != old_prev)
                            changed = 1;
                    }
                }
            }
        }
        cell++;
        for (col = 0; col < w2; col++) {
            cost = map[cell];
            if (cost != COST_IMPASSABLE) {
                old_prev = path_win_prev[cell];
                for (k = 0; k < 5; k++) {
                    n = cell + nb_top[k];
                    ncost = path_win_cost[n];
                    if (ncost != COST_IMPASSABLE) {
                        newcost = ncost + cost;
                        if (path_win_cost[cell] > newcost) {
                            path_win_prev[cell] = n;
                            path_win_cost[cell] = newcost;
                            if (path_win_prev[cell] != old_prev)
                                changed = 1;
                        }
                    }
                }
            }
            cell++;
        }
        cost = map[cell];
        if (cost != COST_IMPASSABLE) {
            old_prev = path_win_prev[cell];
            for (k = 0; k < 3; k++) {
                n = cell + nb_top_right[k];
                ncost = path_win_cost[n];
                if (ncost != COST_IMPASSABLE) {
                    newcost = ncost + cost;
                    if (path_win_cost[cell] > newcost) {
                        path_win_prev[cell] = n;
                        path_win_cost[cell] = newcost;
                        if (path_win_prev[cell] != old_prev)
                            changed = 1;
                    }
                }
            }
        }
        cell++;
        for (row = 0; row < h2; row++) {
            cost = map[cell];
            if (cost != COST_IMPASSABLE) {
                old_prev = path_win_prev[cell];
                for (k = 0; k < 5; k++) {
                    n = cell + nb_left[k];
                    ncost = path_win_cost[n];
                    if (ncost != COST_IMPASSABLE) {
                        newcost = ncost + cost;
                        if (path_win_cost[cell] > newcost) {
                            path_win_prev[cell] = n;
                            path_win_cost[cell] = newcost;
                            if (path_win_prev[cell] != old_prev)
                                changed = 1;
                        }
                    }
                }
            }
            cell++;
            for (col = 0; col < w2; col++) {
                cost = map[cell];
                if (cost != COST_IMPASSABLE) {
                    old_prev = path_win_prev[cell];
                    for (k = 0; k < 8; k++) {
                        n = cell + nb_all[k];
                        ncost = path_win_cost[n];
                        if (ncost != COST_IMPASSABLE) {
                            newcost = ncost + cost;
                            if (path_win_cost[cell] > newcost) {
                                path_win_prev[cell] = n;
                                path_win_cost[cell] = newcost;
                                if (path_win_prev[cell] != old_prev)
                                    changed = 1;
                            }
                        }
                    }
                }
                cell++;
            }
            cost = map[cell];
            if (cost != COST_IMPASSABLE) {
                old_prev = path_win_prev[cell];
                for (k = 0; k < 5; k++) {
                    n = cell + nb_right[k];
                    ncost = path_win_cost[n];
                    if (ncost != COST_IMPASSABLE) {
                        newcost = ncost + cost;
                        if (path_win_cost[cell] > newcost) {
                            path_win_prev[cell] = n;
                            path_win_cost[cell] = newcost;
                            if (path_win_prev[cell] != old_prev)
                                changed = 1;
                        }
                    }
                }
            }
            cell++;
        }
        cost = map[cell];
        if (cost != COST_IMPASSABLE) {
            old_prev = path_win_prev[cell];
            for (k = 0; k < 3; k++) {
                n = cell + nb_bottom_left[k];
                ncost = path_win_cost[n];
                if (ncost != COST_IMPASSABLE) {
                    newcost = ncost + cost;
                    if (path_win_cost[cell] > newcost) {
                        path_win_prev[cell] = n;
                        path_win_cost[cell] = newcost;
                        if (path_win_prev[cell] != old_prev)
                            changed = 1;
                    }
                }
            }
        }
        cell++;
        for (col = 0; col < w2; col++) {
            cost = map[cell];
            if (cost != COST_IMPASSABLE) {
                old_prev = path_win_prev[cell];
                for (k = 0; k < 5; k++) {
                    n = cell + nb_bottom[k];
                    ncost = path_win_cost[n];
                    if (ncost != COST_IMPASSABLE) {
                        newcost = ncost + cost;
                        if (path_win_cost[cell] > newcost) {
                            path_win_prev[cell] = n;
                            path_win_cost[cell] = newcost;
                            if (path_win_prev[cell] != old_prev)
                                changed = 1;
                        }
                    }
                }
            }
            cell++;
        }
        cost = map[cell];
        if (cost != COST_IMPASSABLE) {
            old_prev = path_win_prev[cell];
            for (k = 0; k < 3; k++) {
                n = cell + nb_bottom_right[k];
                ncost = path_win_cost[n];
                if (ncost != COST_IMPASSABLE) {
                    newcost = ncost + cost;
                    if (path_win_cost[cell] > newcost) {
                        path_win_prev[cell] = n;
                        path_win_cost[cell] = newcost;
                        if (path_win_prev[cell] != old_prev)
                            changed = 1;
                    }
                }
            }
        }
        cell++;
    }
    path_len = 0;
    cell = to_x + to_y * w;
    while (path_win_prev[cell] != cell) {
        path_win_list[path_len] = cell;
        cell = path_win_prev[cell];
        path_len++;
    }
    if (from_x + from_y * w != cell)
        return 0;
    for (i = 0; i < path_len; i++) {
        out_x[i] = path_win_list[path_len - 1 - i] % w;
        out_y[i] = path_win_list[path_len - 1 - i] / w;
    }
    return path_len;
}

/* (from_x, from_y, to_x, to_y, map, block_map, out_x, out_y, out_cost, budget, mode) DEAD (no caller): a path over
   the 60 x 40 move-cost map (FFh = impassable) in legs of at most a 15 x 15 window: plans the 5 x 5 blocks to cross
   (mode 1: the straight Map_Line_Path, else Path_Win_Blocks over block_map), then for each block boundary searches a
   leg from the current tile to one of the passable edge tiles; stops when the budget (+ 6) is spent. Returns the
   tiles written to out_x / out_y / out_cost (0 if the target is impassable or a leg fails) */
int Path_Find_Windowed(int from_x, int from_y, int to_x, int to_y, unsigned char (far *map)[60],
                       unsigned char far *block_map, char *out_x, char *out_y, char *out_cost, int budget, int mode)
{
    int blocks;
    int bx1;
    int by1;
    int bx2;
    int by2;
    int cur_x;
    int cur_y;
    int steps;
    int n;
    int step;
    int edge_x[5];
    int edge_y[5];
    int edge_count;
    unsigned char block_x[35];
    unsigned char block_y[35];
    register int i;

    budget += 6;
    if (map[to_y][to_x] == COST_IMPASSABLE)
        return 0;
    bx1 = from_x / 5;
    by1 = from_y / 5;
    bx2 = to_x / 5;
    by2 = to_y / 5;
    if (bx1 == bx2 && by1 == by2) {
        n = Path_Win_Leg(from_x, from_y, to_x, to_y, out_x, out_y, map);
        if (n != 0) {
            for (i = 0; i < n; i++) {
                out_cost[i] = map[out_y[i]][out_x[i]];
                budget -= out_cost[i];
            }
            return n;
        }
    }
    if (mode == 1)
        blocks = Map_Line_Path(bx1, by1, bx2, by2, block_x, block_y, 12);
    else
        blocks = Path_Win_Blocks(bx1, by1, bx2, by2, block_x, block_y, block_map);
    if (blocks == 0)
        return 0;
    cur_x = from_x;
    cur_y = from_y;
    steps = 0;
    for (step = 0; budget > 0 && step <= blocks && (cur_x != to_x || cur_y != to_y); step++) {
        if (blocks - 2 <= step) {
            edge_x[0] = to_x;
            edge_y[0] = to_y;
            edge_count = 1;
        } else
            edge_count = Path_Win_Block_Edges(block_x[step + 1], block_y[step + 1], block_x[step + 2], block_y[step + 2], edge_x,
                                              edge_y, map);
        if (edge_count == 0)
            return 0;
        n = 0;
        for (i = 0; i < edge_count && n == 0; i++)
            n = Path_Win_Leg(cur_x, cur_y, edge_x[i], edge_y[i], out_x + steps, out_y + steps, map);
        if (n == 0)
            return 0;
        for (i = 0; i < n; i++) {
            out_cost[steps + i] = map[out_y[steps + i]][out_x[steps + i]];
            budget -= out_cost[steps + i];
        }
        if (blocks - 2 <= step) {
            cur_x = out_x[steps + n - 1];
            cur_y = out_y[steps + n - 1];
            steps += n;
        } else {
            cur_x = out_x[steps];
            cur_y = out_y[steps];
            steps++;
            for (i = 0; i < n && (block_x[step + 1] != cur_x / 5 || block_y[step + 1] != cur_y / 5); i++) {
                cur_x = out_x[steps];
                cur_y = out_y[steps];
                steps++;
            }
        }
    }
    return steps;
}

/* (from_x, from_y, to_x, to_y, out_x, out_y, map) one leg: copies the 15 x 15 tiles around the start's 5 x 5 block
   (wrapping east-west; outside the map at the top / bottom = blocked) and runs Path_Win_Search in it; the result is
   moved back to map coordinates. Returns the steps */
int Path_Win_Leg(int from_x, int from_y, int to_x, int to_y, char *out_x, char *out_y, unsigned char (far *map)[60])
{
    int size;
    int bx;
    int by;
    int ox;
    int oy;
    int n;
    int i;
    int wfx;
    int wtx;
    int wfy;
    int wty;
    unsigned char window[PATH_WIN_SIZE * PATH_WIN_SIZE];
    register int col;
    register int row;

    bx = from_x / REGION_SIZE;
    by = from_y / REGION_SIZE;
    ox = (bx - 1) * REGION_SIZE;
    oy = (by - 1) * REGION_SIZE;
    wfx = from_x - ox;
    wfy = from_y - oy;
    wtx = to_x - ox;
    wty = to_y - oy;
    if (wfx > PATH_WIN_SIZE)
        wfx = from_x - MAP_WIDTH - ox;
    if (wtx > PATH_WIN_SIZE)
        wtx = to_x - MAP_WIDTH - ox;
    if (wfx < 0)
        wfx = from_x + MAP_WIDTH - ox;
    if (wtx < 0)
        wtx = to_x + MAP_WIDTH - ox;
    size = PATH_WIN_SIZE;
    for (row = 0; row < size; row++)
        for (col = 0; col < size; col++)
            window[row * PATH_WIN_SIZE + col] = map[row + oy][col + ox];
    if (bx == 0)
        for (row = 0; row < size; row++)
            for (col = 0; col < REGION_SIZE; col++)
                window[row * PATH_WIN_SIZE + col] = map[row + oy][col + (MAP_WIDTH - REGION_SIZE)];
    if (bx == REGION_COLS - 1)
        for (row = 0; row < size; row++)
            for (col = 0; col < REGION_SIZE; col++)
                window[row * PATH_WIN_SIZE + col + 2 * REGION_SIZE] = map[row + oy][col];
    if (by == 0)
        for (row = 0; row < REGION_SIZE; row++)
            for (col = 0; col < size; col++)
                window[row * PATH_WIN_SIZE + col] = COST_IMPASSABLE;
    if (by == REGION_ROWS - 1)
        for (row = 0; row < REGION_SIZE; row++)
            for (col = 0; col < size; col++)
                window[row * PATH_WIN_SIZE + col + 2 * REGION_SIZE * PATH_WIN_SIZE] = COST_IMPASSABLE;
    n = Path_Win_Search(wfx, wfy, wtx, wty, size, size, window, out_x, out_y);
    for (i = 0; i < n; i++) {
        out_x[i] = out_x[i] + ox;
        if (out_x[i] < 0)
            out_x[i] = out_x[i] + MAP_WIDTH;
        if (out_x[i] >= MAP_WIDTH)
            out_x[i] = out_x[i] - MAP_WIDTH;
        out_y[i] = out_y[i] + oy;
        if (out_y[i] < 0)
            out_y[i] = out_y[i] + MAP_HEIGHT;
        if (out_y[i] >= MAP_HEIGHT)
            out_y[i] = out_y[i] - MAP_HEIGHT;
    }
    return n;
}

/* the region grid of Path_Win_Blocks (REGION_*: 12 x 8 blocks of 5 x 5 squares, as REGIONS.C); a block's open
   edges (EDGE_*, ENUMS.H), and the crossings that exist (EDGE_*_CROSSING) - the target block's open its own edges
   for the search */

/* (x1, y1, x2, y2, out_x, out_y, map) shortest route over the 12 x 8 grid of 5 x 5 blocks (wrapping east-west): map
   holds per block the open edges (1 north, 2 east, 4 south, 8 west); the target's 10h / 20h / 40h / 80h bits open its
   own edges for the search and are restored after. Every step costs 1. Returns the blocks written to out_x / out_y
   including the start (1 when start = target, 0 if unreachable) */
int Path_Win_Blocks(int x1, int y1, int x2, int y2, char *out_x, char *out_y, unsigned char far *map)
{
    int path_len;
    int newcost;
    int target_cell;
    int n;
    int t1;
    int t4;
    int t2;
    int t8;
    int edges;
    int i;
    int col;
    int cols;
    int rows;
    int row;
    int changed;
    int old_prev;
    int tflags;
    int *list;
    int *prev;
    unsigned char *cost;
    int cell;

    path_len = 0;
    prev = path_win_prev;
    cost = path_win_cost;
    list = path_win_list;
    if (x1 == x2 && y1 == y2) {
        *out_x = x1;
        *out_y = y1;
        return 1;
    }
    for (i = 0; i < REGION_CELLS; i++)
        prev[i] = i;
    for (i = 0; i < REGION_CELLS; i++)
        cost[i] = COST_IMPASSABLE;
    cost[x1 + y1 * REGION_COLS] = 0;
    target_cell = y2 * REGION_COLS + x2;
    tflags = map[target_cell];
    t1 = tflags & EDGE_N;
    t4 = tflags & EDGE_S;
    t2 = tflags & EDGE_E;
    t8 = tflags & EDGE_W;
    if (tflags & EDGE_N_CROSSING)
        tflags = tflags | EDGE_N;
    if (tflags & EDGE_S_CROSSING)
        tflags = tflags | EDGE_S;
    if (tflags & EDGE_W_CROSSING)
        tflags = tflags | EDGE_W;
    if (tflags & EDGE_E_CROSSING)
        tflags = tflags | EDGE_E;
    map[target_cell] = tflags;
    changed = 1;
    while (changed == 1) {
        changed = 0;
        cols = REGION_COLS - 2;                 /* the inner columns: the first and the last are done apart */
        rows = REGION_ROWS;
        cell = 0;
        for (row = 0; row < rows; row++) {
            edges = map[cell];
            if (edges & EDGE_N) {
                n = cell - REGION_COLS;
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            if (edges & EDGE_S) {
                n = cell + REGION_COLS;
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            if (edges & EDGE_W) {
                n = cell + (REGION_COLS - 1);
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            if (edges & EDGE_E) {
                n = cell + 1;
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            cell++;
            for (col = 0; col < cols; col++) {
                edges = map[cell];
                if (edges & EDGE_N) {
                    n = cell - REGION_COLS;
                    newcost = cost[n] + 1;
                    if (cost[cell] > newcost) {
                        old_prev = prev[cell];
                        prev[cell] = n;
                        cost[cell] = newcost;
                        if (prev[cell] != old_prev)
                            changed = 1;
                    }
                }
                if (edges & EDGE_S) {
                    n = cell + REGION_COLS;
                    newcost = cost[n] + 1;
                    if (cost[cell] > newcost) {
                        old_prev = prev[cell];
                        prev[cell] = n;
                        cost[cell] = newcost;
                        if (prev[cell] != old_prev)
                            changed = 1;
                    }
                }
                if (edges & EDGE_W) {
                    n = cell - 1;
                    newcost = cost[n] + 1;
                    if (cost[cell] > newcost) {
                        old_prev = prev[cell];
                        prev[cell] = n;
                        cost[cell] = newcost;
                        if (prev[cell] != old_prev)
                            changed = 1;
                    }
                }
                if (edges & EDGE_E) {
                    n = cell + 1;
                    newcost = cost[n] + 1;
                    if (cost[cell] > newcost) {
                        old_prev = prev[cell];
                        prev[cell] = n;
                        cost[cell] = newcost;
                        if (prev[cell] != old_prev)
                            changed = 1;
                    }
                }
                cell++;
            }
            edges = map[cell];
            if (edges & EDGE_N) {
                n = cell - REGION_COLS;
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            if (edges & EDGE_S) {
                n = cell + REGION_COLS;
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            if (edges & EDGE_W) {
                n = cell - 1;
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            if (edges & EDGE_E) {
                n = cell - (REGION_COLS - 1);
                newcost = cost[n] + 1;
                if (cost[cell] > newcost) {
                    old_prev = prev[cell];
                    prev[cell] = n;
                    cost[cell] = newcost;
                    if (prev[cell] != old_prev)
                        changed = 1;
                }
            }
            cell++;
        }
    }
    map[target_cell] = (map[target_cell] & (0xff ^ EDGE_N)) + t1;
    map[target_cell] = (map[target_cell] & (0xff ^ EDGE_S)) + t4;
    map[target_cell] = (map[target_cell] & (0xff ^ EDGE_E)) + t2;
    map[target_cell] = (map[target_cell] & (0xff ^ EDGE_W)) + t8;
    path_len = 0;
    cell = x2 + y2 * REGION_COLS;
    while (prev[cell] != cell) {
        list[path_len] = cell;
        cell = prev[cell];
        path_len++;
    }
    if (path_len == 0)
        return 0;
    out_x[0] = x1;
    out_y[0] = y1;
    for (i = 0; i < path_len; i++) {
        out_x[i + 1] = list[path_len - 1 - i] % REGION_COLS;
        if (out_x[i + 1] < 0)
            out_x[i + 1] = out_x[i + 1] + REGION_COLS;
        out_y[i + 1] = list[path_len - 1 - i] / REGION_COLS;
    }
    return path_len + 1;
}

/* (bx1, by1, bx2, by2, edge_x, edge_y, map) the passable tiles (map != FFh) on the side of block (bx1, by1) that
   faces the neighbouring block (bx2, by2), centre first; returns their count */
int Path_Win_Block_Edges(int bx1, int by1, int bx2, int by2, int *edge_x, int *edge_y, unsigned char (far *map)[MAP_WIDTH])
{
    int x0;
    int y0;
    int count;
    int j;
    int i;

    x0 = bx1 * 5;
    y0 = by1 * 5;
    count = 0;
    if (by2 == by1) {
        if (bx2 < bx1 || (bx1 == 0 && bx2 == 11)) {
            for (i = 0; i < 5; i++)
                if (map[path_win_edge_ofs[i] + y0][x0] != COST_IMPASSABLE) {
                    edge_x[count] = x0;
                    edge_y[count] = y0 + path_win_edge_ofs[i];
                    count++;
                }
        } else {
            for (i = 0; i < 5; i++)
                if (map[path_win_edge_ofs[i] + y0][x0 + 4] != COST_IMPASSABLE) {
                    edge_x[count] = x0 + 4;
                    edge_y[count] = path_win_edge_ofs[i] + y0;
                    count++;
                }
        }
    } else if (by2 > by1) {
        for (j = 0; j < 5; j++)
            if (map[y0 + 4][path_win_edge_ofs[j] + x0] != COST_IMPASSABLE) {
                edge_x[count] = path_win_edge_ofs[j] + x0;
                edge_y[count] = y0 + 4;
                count++;
            }
    } else {
        for (j = 0; j < 5; j++)
            if (map[y0][path_win_edge_ofs[j] + x0] != COST_IMPASSABLE) {
                edge_x[count] = path_win_edge_ofs[j] + x0;
                edge_y[count] = y0;
                count++;
            }
    }
    return count;
}
