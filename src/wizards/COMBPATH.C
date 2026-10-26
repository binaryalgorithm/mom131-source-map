/* COMBPATH.C - WIZARDS.EXE overlay code segment e29d (stub 2653): combat path finding on the 21 x 22 battlefield
   grid. A Bellman-Ford style relaxation over p_combat_move_cost (FFh = impassable): every cell takes its cheapest
   neighbour, sweeping row by row until nothing changes, with three neighbour-offset tables for the left column, the
   inner columns and the right column (-1000 = no neighbour there). BUG #446: the sweep covers rows 0..19 only - the last
   two rows of the 22-row grid are never relaxed (the same copy-paste shape as PATHWIN.C's overland search).
   This module's data (DS:70c8): the three offset tables; BSS (DS:d384): p_combat_path_cells.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS e29d src/wizards/COMBPATH.C */
#include "ENUMS.H"

/* neighbour offsets on the 21-wide grid: the four diagonals (1 more in Combat_Find_Path) first, then the sides */
int combat_path_nbr_ofs[8] = { -20, 20, 22, -22, -21, 21, -1, 1 };                /* DS:70c8 */
int combat_path_nbr_ofs_x0[8] = { -20, -1000, 22, -1000, -21, 21, -1000, 1 };   /* DS:70d8  left column (-1000: no neighbour, dropped by the nb >= 0 test) */
int combat_path_nbr_ofs_x20[8] = { -1000, 20, -1000, -22, -21, 21, -1, -1000 }; /* DS:70e8  right column */

int *p_combat_path_cells;                       /* DS:d384  the path's cells, last first */
                                                /* aliases p_combat_move_cost: writing the path overwrites the costs */

extern int *p_combat_path_y;                    /* DS:d13e */
extern int *p_combat_path_x;                    /* DS:d140 */
extern int combat_path_len;                     /* DS:d142 */
extern int *p_combat_path_prev;                  /* DS:d144  per cell: predecessor (Find_Path) / reach flag */
extern unsigned char *p_combat_path_dist;       /* DS:d146 */
extern unsigned char *p_combat_move_cost;       /* DS:d148 */

/* (from x, y, to x, y): the cheapest path - its steps (the start not included) in p_combat_path_x / _y, the
   count in combat_path_len; a diagonal step costs 1 more than a side step. Returns 0 if the target square is
   blocked; otherwise the result is meaningless (no return statement: the last loop counter)
   Numbered: BUG #200 (BUGS_AND_QUIRKS.txt) */
int Combat_Find_Path(int fx, int fy, int tx, int ty)
{
    int i;
    int changed;
    int old;
    int row;
    int col;
    int inner_cols;
    int nb_dist;
    int rows_swept;
    unsigned char nd;
    unsigned char cost;
    int cell;
    int nb;
    int k;

    combat_path_len = 0;
    if (p_combat_move_cost[ty * BATTLEFIELD_W + tx] == COST_IMPASSABLE)
        return 0;
    for (i = 0; i < BATTLEFIELD_CELLS; i++)
        p_combat_path_prev[i] = i;
    for (i = 0; i < BATTLEFIELD_CELLS; i++)
        p_combat_path_dist[i] = COST_IMPASSABLE;
    p_combat_path_dist[fx + fy * BATTLEFIELD_W] = 0;
    changed = 1;
    while (changed == 1) {
        changed = 0;
        inner_cols = (BATTLEFIELD_W - 2);
        rows_swept = 20;
        cell = 0;
        for (row = 0; row < rows_swept; row++) {
            cost = p_combat_move_cost[cell];
            if (cost != COST_IMPASSABLE) {
                old = p_combat_path_prev[cell];
                for (k = 0; k < 4; k++) {
                    nb = cell + combat_path_nbr_ofs_x0[k];
                    if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                        nb_dist = p_combat_path_dist[nb];
                        if (nb_dist != COST_IMPASSABLE) {
                            nd = nb_dist + cost + 1;
                            if (p_combat_path_dist[cell] > nd) {
                                p_combat_path_prev[cell] = nb;
                                p_combat_path_dist[cell] = nd;
                                if (p_combat_path_prev[cell] != old)
                                    changed = 1;
                            }
                        }
                    }
                }
                for (k = 4; k < 8; k++) {
                    nb = cell + combat_path_nbr_ofs_x0[k];
                    if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                        nb_dist = p_combat_path_dist[nb];
                        if (nb_dist != COST_IMPASSABLE) {
                            nd = nb_dist + cost;
                            if (p_combat_path_dist[cell] > nd) {
                                p_combat_path_prev[cell] = nb;
                                p_combat_path_dist[cell] = nd;
                                if (p_combat_path_prev[cell] != old)
                                    changed = 1;
                            }
                        }
                    }
                }
            }
            cell++;
            for (col = 0; col < inner_cols; col++) {
                cost = p_combat_move_cost[cell];
                if (cost != COST_IMPASSABLE) {
                    old = p_combat_path_prev[cell];
                    for (k = 0; k < 4; k++) {
                        nb = cell + combat_path_nbr_ofs[k];
                        if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                            nb_dist = p_combat_path_dist[nb];
                            if (nb_dist != COST_IMPASSABLE) {
                                nd = nb_dist + cost + 1;
                                if (p_combat_path_dist[cell] > nd) {
                                    p_combat_path_prev[cell] = nb;
                                    p_combat_path_dist[cell] = nd;
                                    if (p_combat_path_prev[cell] != old)
                                        changed = 1;
                                }
                            }
                        }
                    }
                    for (k = 4; k < 8; k++) {
                        nb = cell + combat_path_nbr_ofs[k];
                        if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                            nb_dist = p_combat_path_dist[nb];
                            if (nb_dist != COST_IMPASSABLE) {
                                nd = nb_dist + cost;
                                if (p_combat_path_dist[cell] > nd) {
                                    p_combat_path_prev[cell] = nb;
                                    p_combat_path_dist[cell] = nd;
                                    if (p_combat_path_prev[cell] != old)
                                        changed = 1;
                                }
                            }
                        }
                    }
                }
                cell++;
            }
            cost = p_combat_move_cost[cell];
            if (cost != COST_IMPASSABLE) {
                old = p_combat_path_prev[cell];
                for (k = 0; k < 4; k++) {
                    nb = cell + combat_path_nbr_ofs_x20[k];
                    if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                        nb_dist = p_combat_path_dist[nb];
                        if (nb_dist != COST_IMPASSABLE) {
                            nd = nb_dist + cost + 1;
                            if (p_combat_path_dist[cell] > nd) {
                                p_combat_path_prev[cell] = nb;
                                p_combat_path_dist[cell] = nd;
                                if (p_combat_path_prev[cell] != old)
                                    changed = 1;
                            }
                        }
                    }
                }
                for (k = 4; k < 8; k++) {
                    nb = cell + combat_path_nbr_ofs_x20[k];
                    if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                        nb_dist = p_combat_path_dist[nb];
                        if (nb_dist != COST_IMPASSABLE) {
                            nd = nb_dist + cost;
                            if (p_combat_path_dist[cell] > nd) {
                                p_combat_path_prev[cell] = nb;
                                p_combat_path_dist[cell] = nd;
                                if (p_combat_path_prev[cell] != old)
                                    changed = 1;
                            }
                        }
                    }
                }
            }
            cell++;
        }
    }
    p_combat_path_cells = (int *)p_combat_move_cost;
    combat_path_len = 0;
    for (cell = tx + ty * BATTLEFIELD_W; p_combat_path_prev[cell] != cell; combat_path_len++) {
        p_combat_path_cells[combat_path_len] = cell;
        cell = p_combat_path_prev[cell];
    }
    for (i = 0; i < combat_path_len; i++) {
        p_combat_path_x[i] = p_combat_path_cells[combat_path_len - 1 - i] % BATTLEFIELD_W;
        p_combat_path_y[i] = p_combat_path_cells[combat_path_len - 1 - i] / BATTLEFIELD_W;
    }
}

/* (x, y, moves): which squares a unit with that many half-moves left can reach from (x, y) - the same
   relaxation (without Combat_Find_Path's + 1 diagonal surcharge, which the moves do pay), then p_combat_path_prev[cell] = 1 where the cost to get there, not
   counting the square's own cost, is below moves */
void Combat_Calc_Reach(int x, int y, int moves)
{
    int gx;
    int gy;
    int i;
    int changed;
    int old;
    int row;
    int col;
    int inner_cols;
    int nb_dist;
    int rows_swept;
    int diff;
    unsigned char nd;
    unsigned char cost;
    int cell;
    int nb;
    int k;

    combat_path_len = 0;
    for (i = 0; i < BATTLEFIELD_CELLS; i++)
        p_combat_path_prev[i] = i;
    for (i = 0; i < BATTLEFIELD_CELLS; i++)
        p_combat_path_dist[i] = COST_IMPASSABLE;
    p_combat_path_dist[x + y * BATTLEFIELD_W] = 0;
    changed = 1;
    while (changed == 1) {
        changed = 0;
        inner_cols = (BATTLEFIELD_W - 2);
        rows_swept = 20;
        cell = 0;
        for (row = 0; row < rows_swept; row++) {
            cost = p_combat_move_cost[cell];
            if (cost != COST_IMPASSABLE) {
                old = p_combat_path_prev[cell];
                for (k = 0; k < 8; k++) {
                    nb = cell + combat_path_nbr_ofs_x0[k];
                    if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                        nb_dist = p_combat_path_dist[nb];
                        if (nb_dist != COST_IMPASSABLE) {
                            nd = nb_dist + cost;
                            if (p_combat_path_dist[cell] > nd) {
                                p_combat_path_prev[cell] = nb;
                                p_combat_path_dist[cell] = nd;
                                if (p_combat_path_prev[cell] != old)
                                    changed = 1;
                            }
                        }
                    }
                }
            }
            cell++;
            for (col = 0; col < inner_cols; col++) {
                cost = p_combat_move_cost[cell];
                if (cost != COST_IMPASSABLE) {
                    old = p_combat_path_prev[cell];
                    for (k = 0; k < 8; k++) {
                        nb = cell + combat_path_nbr_ofs[k];
                        if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                            nb_dist = p_combat_path_dist[nb];
                            if (nb_dist != COST_IMPASSABLE) {
                                nd = nb_dist + cost;
                                if (p_combat_path_dist[cell] > nd) {
                                    p_combat_path_prev[cell] = nb;
                                    p_combat_path_dist[cell] = nd;
                                    if (p_combat_path_prev[cell] != old)
                                        changed = 1;
                                }
                            }
                        }
                    }
                }
                cell++;
            }
            cost = p_combat_move_cost[cell];
            if (cost != COST_IMPASSABLE) {
                old = p_combat_path_prev[cell];
                for (k = 0; k < 8; k++) {
                    nb = cell + combat_path_nbr_ofs_x20[k];
                    if (nb >= 0 && nb < BATTLEFIELD_CELLS) {
                        nb_dist = p_combat_path_dist[nb];
                        if (nb_dist != COST_IMPASSABLE) {
                            nd = nb_dist + cost;
                            if (p_combat_path_dist[cell] > nd) {
                                p_combat_path_prev[cell] = nb;
                                p_combat_path_dist[cell] = nd;
                                if (p_combat_path_prev[cell] != old)
                                    changed = 1;
                            }
                        }
                    }
                }
            }
            cell++;
        }
    }
    for (gy = 0; gy < BATTLEFIELD_H; gy++)
        for (gx = 0; gx < BATTLEFIELD_W; gx++) {
            cell = gy * BATTLEFIELD_W + gx;
            p_combat_path_prev[cell] = 0;
            if (p_combat_path_dist[cell] != COST_IMPASSABLE) {
                cost = p_combat_move_cost[cell];
                diff = p_combat_path_dist[cell] - cost;
                if (moves > diff)
                    p_combat_path_prev[cell] = 1;
            }
        }
}
