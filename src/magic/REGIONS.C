/* REGIONS.C - MAGIC.EXE code segment 518e (MAGIC only, one function): the block-connectivity map the world generator
   builds for each plane - per 5 x 5 block of the 60 x 40 map, which of its four edges can be crossed (the block edge
   bits here are WORLDGEN.C's region bits: Build_Region_Bits, p_region_bits_land / _water). WIZARDS only
   saves / loads these maps (its block pathfinder in PATHWIN.C is dead code).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 518e src/magic/REGIONS.C */
#include "ENUMS.H"
#pragma warn -pro

int regions_unused = 1;                         /* DS:354a  no reader; the module's only data (link test, batch 36) */

/* (cost layer, out[8][12]) per block: edge bits N 1 / E 2 / S 4 / W 8 = an open square on the edge with an open
   neighbour across it (copies kept in 10h / 20h / 40h / 80h), the low bit cleared again when Path_Win_Search finds
   no path from one square in from the far side (the window's row / column 4; 2 for S / E) across that edge; then every low bit is mirrored onto the neighbour.
   BUG #367: the E check starts from x0 + 1 / column x0 (a copy of the W check), not x0 + 5 / x0 + 4. BUG (not in BUGS_AND_QUIRKS.txt; #366 is the
   ocean-variety overrun): the mirror pass tests 'row < 11' for the east neighbour, so column 11 writes into the next row (row 7: past the map) */
void Block_Edge_Bits_Build(unsigned char (far *cost)[MAP_WIDTH], unsigned char (far *out)[REGION_COLS])
{
    int block_row;
    int block_col;
    int x0;
    int col;
    int row;
    int scan_y;
    int scan_x;
    int sx;
    int sy;
    int wrow;
    int found;
    int x_across;
    int ex;
    int ey;
    unsigned char edges;
    unsigned char window_ns[35];
    unsigned char window_we[35];
    char path_x[16];
    char path_y[16];
    register int y0;
    register int i;

    y0 = 0;
    for (block_row = 0; block_row < REGION_ROWS; block_row++) {
        x0 = 0;
        for (block_col = 0; block_col < REGION_COLS; block_col++) {
            edges = 0;
            if (block_row != 0) {
                if (cost[y0][x0] != COST_IMPASSABLE && (cost[y0 - 1][x0] != COST_IMPASSABLE || cost[y0 - 1][x0 + 1] != COST_IMPASSABLE))
                    edges = edges | EDGE_N;
                for (scan_x = x0 + 1; x0 + 4 > scan_x && (edges & EDGE_N) == 0; scan_x++) {
                    if (cost[y0][scan_x] != COST_IMPASSABLE && (cost[y0 - 1][scan_x - 1] != COST_IMPASSABLE ||
                            cost[y0 - 1][scan_x] != COST_IMPASSABLE || cost[y0 - 1][scan_x + 1] != COST_IMPASSABLE))
                        edges = edges | EDGE_N;
                }
                if (cost[y0][x0 + 4] != COST_IMPASSABLE && (edges & EDGE_N) == 0 && (cost[y0 - 1][x0 + 4] != COST_IMPASSABLE ||
                        cost[y0 - 1][x0 + 3] != COST_IMPASSABLE))
                    edges = edges | EDGE_N;
                if (edges & EDGE_N)
                    edges = edges | EDGE_N_CROSSING;
                found = 0;
                for (wrow = 0; wrow < 7; wrow++)
                    for (i = 0; i < 5; i++)
                        window_ns[wrow * 5 + i] = cost[y0 + wrow - 1][x0 + i];
                if (block_row == REGION_ROWS - 1)
                    for (i = 0; i < 5; i++)
                        window_ns[30 + i] = COST_IMPASSABLE;
                for (sx = 0; sx < 5 && found == 0; sx++)
                    for (ex = 0; ex < 5 && found == 0; ex++)
                        found += Path_Win_Search(sx, 4, ex, 0, 5, 7, window_ns, path_x, path_y);   /* row 4 to row 0 of the 5 x 7 window */
                if (found == 0)
                    edges = edges & (0xff ^ EDGE_N);
            }
            if (block_row != REGION_ROWS - 1) {
                if (cost[y0 + 4][x0] != COST_IMPASSABLE && (cost[y0 + 5][x0] != COST_IMPASSABLE || cost[y0 + 5][x0 + 1] != COST_IMPASSABLE))
                    edges = edges | EDGE_S;
                for (scan_x = x0 + 1; x0 + 4 > scan_x && (edges & EDGE_S) == 0; scan_x++) {
                    if (cost[y0 + 4][scan_x] != COST_IMPASSABLE && (cost[y0 + 5][scan_x - 1] != COST_IMPASSABLE ||
                            cost[y0 + 5][scan_x] != COST_IMPASSABLE || cost[y0 + 5][scan_x + 1] != COST_IMPASSABLE))
                        edges = edges | EDGE_S;
                }
                if (cost[y0 + 4][x0 + 4] != COST_IMPASSABLE && (edges & EDGE_S) == 0 && (cost[y0 + 5][x0 + 4] != COST_IMPASSABLE ||
                        cost[y0 + 5][x0 + 3] != COST_IMPASSABLE))
                    edges = edges | EDGE_S;
                if (edges & EDGE_S)
                    edges = edges | EDGE_S_CROSSING;
                found = 0;
                for (wrow = 0; wrow < 7; wrow++)
                    for (i = 0; i < 5; i++)
                        window_ns[wrow * 5 + i] = cost[y0 + wrow - 1][x0 + i];
                if (block_row == 0)
                    for (i = 0; i < 5; i++)
                        window_ns[i] = COST_IMPASSABLE;
                for (sx = 0; sx < 5 && found == 0; sx++)
                    for (ex = 0; ex < 5 && found == 0; ex++)
                        found += Path_Win_Search(sx, 2, ex, 6, 5, 7, window_ns, path_x, path_y);   /* row 2 to row 6 */
                if (found == 0)
                    edges = edges & (0xff ^ EDGE_S);
            }
            x_across = x0 - 1;
            if (x_across < 0)
                x_across += MAP_WIDTH;
            if (cost[y0][x0] != COST_IMPASSABLE && (cost[y0][x_across] != COST_IMPASSABLE || cost[y0 + 1][x_across] != COST_IMPASSABLE))
                edges = edges | EDGE_W;
            for (scan_y = y0 + 1; y0 + 4 > scan_y && (edges & EDGE_W) == 0; scan_y++) {
                if (cost[scan_y][x0] != COST_IMPASSABLE && (cost[scan_y - 1][x_across] != COST_IMPASSABLE ||
                        cost[scan_y][x_across] != COST_IMPASSABLE || cost[scan_y + 1][x_across] != COST_IMPASSABLE))
                    edges = edges | EDGE_W;
            }
            if (cost[y0 + 4][x0] != COST_IMPASSABLE && (edges & EDGE_W) == 0 && (cost[y0 + 4][x_across] != COST_IMPASSABLE ||
                    cost[y0 + 3][x_across] != COST_IMPASSABLE))
                edges = edges | EDGE_W;
            if (edges & EDGE_W)
                edges = edges | EDGE_W_CROSSING;
            found = 0;
            for (wrow = 0; wrow < 5; wrow++)
                for (i = 0; i < 7; i++)
                    window_we[wrow * 7 + i] = cost[y0 + wrow][x0 + i - 1];
            if (block_col == 0)
                for (wrow = 0; wrow < 5; wrow++)
                    window_we[wrow * 7] = cost[y0 + wrow][x0 + MAP_WIDTH - 1];
            for (sy = 0; sy < 5 && found == 0; sy++)
                for (ey = 0; ey < 5 && found == 0; ey++)
                    found += Path_Win_Search(4, sy, 0, ey, 7, 5, window_we, path_x, path_y);   /* column 4 to 0 of the 7 x 5 window */
            if (found == 0)
                edges = edges & (0xff ^ EDGE_W);
            x_across = x0 + 1;
            if (x_across >= MAP_WIDTH)
                x_across -= MAP_WIDTH;
            if (cost[y0][x0] != COST_IMPASSABLE && (cost[y0][x_across] != COST_IMPASSABLE || cost[y0 + 1][x_across] != COST_IMPASSABLE))
                edges = edges | EDGE_E;
            for (scan_y = y0 + 1; y0 + 4 > scan_y && (edges & EDGE_E) == 0; scan_y++) {
                if (cost[scan_y][x0] != COST_IMPASSABLE && (cost[scan_y - 1][x_across] != COST_IMPASSABLE ||
                        cost[scan_y][x_across] != COST_IMPASSABLE || cost[scan_y + 1][x_across] != COST_IMPASSABLE))
                    edges = edges | EDGE_E;
            }
            if (cost[y0 + 4][x0] != COST_IMPASSABLE && (edges & EDGE_E) == 0 && (cost[y0 + 4][x_across] != COST_IMPASSABLE ||
                    cost[y0 + 3][x_across] != COST_IMPASSABLE))
                edges = edges | EDGE_E;
            if (edges & EDGE_E)
                edges = edges | EDGE_E_CROSSING;
            found = 0;
            for (wrow = 0; wrow < 5; wrow++)
                for (i = 0; i < 7; i++)
                    window_we[wrow * 7 + i] = cost[y0 + wrow][x0 + i - 1];
            if (block_col == REGION_COLS - 1)
                for (wrow = 0; wrow < 5; wrow++)
                    window_we[wrow * 7 + 6] = cost[y0 + wrow][0];
            for (sy = 0; sy < 5 && found == 0; sy++)
                for (ey = 0; ey < 5 && found == 0; ey++)
                    found += Path_Win_Search(2, sy, 6, ey, 7, 5, window_we, path_x, path_y);   /* column 2 to 6 */
            if (found == 0)
                edges = edges & (0xff ^ EDGE_E);
            out[block_row][block_col] = edges;
            x0 += REGION_SIZE;
        }
        y0 += REGION_SIZE;
    }
    for (row = 0; row < REGION_ROWS; row++) {
        for (col = 0; col < REGION_COLS; col++) {
            if ((out[row][col] & 1) != 0 && row != 0)
                out[row - 1][col] = out[row - 1][col] | 4;
            if ((out[row][col] & 4) != 0 && row < REGION_ROWS - 1)
                out[row + 1][col] = out[row + 1][col] | 1;
            if ((out[row][col] & 8) != 0 && col != 0)
                out[row][col - 1] = out[row][col - 1] | 2;
            if ((out[row][col] & 2) != 0 && row < REGION_COLS - 1)
                out[row][col + 1] = out[row][col + 1] | 8;
        }
    }
}
