/* MAPVIS.C - WIZARDS.EXE code segment 7851 (map visibility), rebuilt from the game's machine code.
   MATCHED: Borland C++ 3.1, BCC -c -mm -a (no other options) compiles this file to the game's bytes exactly
   (all 8 functions, 1,666 bytes; check: python -P tools/match_build.py WIZARDS 7851 src/wizards/MAPVIS.C).
   Structs: the shared src/include/STRUCTS.H (P1: this file had its own partial copies).
   Things the match forced (not style choices):
     - function order = order in the segment; local declaration order sets stack slots and which locals get SI / DI;
     - 'if (p) f(a, b1); else f(a, b2);' (Borland merges the two calls) - a ?: argument compiles differently;
     - 'if (a && b) {...}' versus an early 'return' compile differently;
     - p_exploration_maps is a far pointer to char[40][60] (indexing order shows in the code).
   Visibility_Mark_Square has no final return (the original too: BCC warns; the callers ignore the value). */

/* the module's functions in the order the original declared them (= its overlay stub's thunk order; batch 39) */
void Visibility_Clear(int plane);
int Tile_Visible(int x, int y, int plane);
void Tile_Set_Visible(int x, int y, int plane);
void Tile_Clear_Visible(int x, int y, int plane);
void Visibility_Rebuild(void);
int Visibility_Mark_Square(int x, int y, int plane, int range);
void Unit_Check_Contact(int unit, int plane);
int Tile_Explored(int x, int y, int plane);
#include "STRUCTS.H"
#include "ENUMS.H"

extern unsigned char far *vis_bitmap_myrror;    /* DS:9392 */
extern unsigned char far *vis_bitmap_arcanus;   /* DS:9396 */
extern unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb0 */
extern struct Unit far *p_units;                /* DS:9ec2 */
extern struct Wizard wizards[6];                /* DS:9eca */
extern struct City far *p_cities;               /* DS:9cbc */
extern int current_player;                      /* DS:bd8c */
extern int num_units;                           /* DS:bd92 */
extern int num_cities;                          /* DS:bd94 */
extern int num_players;                         /* DS:bd9c */

int Unit_Can_Fly(int unit);
int Unit_Has_Wind_Walking(int unit);
int Unit_Water_Move_Always_0(int unit);
int Hero_Item_Flight(int unit);

int Bit_Test(int bit, char far *map);
void Bit_Set(int bit, char far *map);
void Bit_Clear(int bit, char far *map);

/* (plane) clears the plane's visibility bitmap (2400 squares, one bit each) */
void Visibility_Clear(int plane)
{
    int i;
    int size;

    size = MAP_SQUARES;
    size = size / 8;
    if (plane == PLANE_ARCANUS) {
        for (i = 0; i < size; i++)
            vis_bitmap_arcanus[i] = 0;
    } else {
        for (i = 0; i < size; i++)
            vis_bitmap_myrror[i] = 0;
    }
}

/* (x, y, plane) -> 1 when the square is in sight this turn: its visibility bit is set and it is explored */
int Tile_Visible(int x, int y, int plane)
{
    int test;
    int explored;
    int bit;

    bit = y * MAP_WIDTH + x;
    if (plane == PLANE_ARCANUS)
        test = Bit_Test(bit, vis_bitmap_arcanus);
    else
        test = Bit_Test(bit, vis_bitmap_myrror);
    explored = Tile_Explored(x, y, plane);
    if (test != 0 && explored == 1)
        return 1;
    else
        return 0;
}

/* (x, y, plane) sets the square's visibility bit (bit y * 60 + x of the plane's bitmap) */
void Tile_Set_Visible(int x, int y, int plane)
{
    int bit;

    bit = y * MAP_WIDTH + x;
    if (plane == PLANE_ARCANUS)
        Bit_Set(bit, vis_bitmap_arcanus);
    else
        Bit_Set(bit, vis_bitmap_myrror);
}

/* (x, y, plane) clears the square's visibility bit */
void Tile_Clear_Visible(int x, int y, int plane)
{
    int bit;

    bit = y * MAP_WIDTH + x;
    if (plane == PLANE_ARCANUS)
        Bit_Clear(bit, vis_bitmap_arcanus);
    else
        Bit_Clear(bit, vis_bitmap_myrror);
}

/* () the current player's sight, both planes: with Nature Awareness every explored square is visible and
   every wizard is in contact; otherwise each own unit sees its scouting range (at least 2 for fliers, Wind
   Walking and flight items; else +1 on patrol), each own city 2 (3 with City Walls, 5 with Nature's Eye, 4 with
   an Oracle - tested after the Eye, so an Oracle city sees 4, not 5), and a visible foreign wizard's city or
   visible non-invisible unit puts the two wizards in contact. A city with plane 2 counts on both planes
   Numbered: BUG #25, #297 (BUGS_AND_QUIRKS.txt) */
void Visibility_Rebuild(void)
{
    int y;
    int pass;
    int range;
    int i;
    int plane;

    plane = 0;
    for (pass = 0; pass < 2; pass++) {
        Visibility_Clear(plane);
        if (wizards[current_player].globals.nature_awareness != 0) {
            for (y = 0; y < MAP_HEIGHT; y++)
                for (i = 0; i < MAP_WIDTH; i++)
                    if (Tile_Explored(i, y, plane) == 1)
                        Tile_Set_Visible(i, y, plane);
            for (i = 0; i < num_players; i++) {
                wizards[current_player].contacted[i] = 1;
                wizards[i].contacted[current_player] = 1;
            }
        } else {
            for (i = 0; i < num_units; i++) {
                if (p_units[i].owner == current_player &&
                    (p_units[i].plane == plane || p_units[i].in_tower == 1)) {
                    range = p_units[i].scout_range;
                    if (Unit_Can_Fly(i) == 1 || Unit_Has_Wind_Walking(i) == 1 ||
                        Unit_Water_Move_Always_0(i) == 1 || Hero_Item_Flight(i) == 1) {
                        if (range < 2)
                            range = 2;
                    } else if (p_units[i].order == ORDER_PATROL)
                        range++;
                    Visibility_Mark_Square(p_units[i].x, p_units[i].y, plane, range);
                }
            }
            for (i = 0; i < num_cities; i++) {
                if (p_cities[i].owner == current_player &&
                    (p_cities[i].plane == plane || p_cities[i].plane == PLANE_BOTH)) {
                    range = 2;
                    if (p_cities[i].buildings[BLD_CITY_WALLS] != BLDSTATE_NONE)   /* walls in any state (the map
                                                   picture tests != 0 instead: RUNTIME_CHECKS RC1) */
                        range = 3;
                    if (p_cities[i].enchants[CITY_ENCH_NATURES_EYE] != 0)
                        range = 5;
                    if (p_cities[i].buildings[BLD_ORACLE] != BLDSTATE_NONE)
                        range = 4;
                    Visibility_Mark_Square(p_cities[i].x, p_cities[i].y, plane, range);
                }
            }
            for (i = 0; i < num_cities; i++) {
                if (p_cities[i].owner != current_player && p_cities[i].owner != PLAYER_NEUTRAL &&
                    (p_cities[i].plane == plane || p_cities[i].plane == PLANE_BOTH) &&
                    Tile_Visible(p_cities[i].x, p_cities[i].y, plane) == 1 && p_cities[i].owner < PLAYER_NEUTRAL) {
                    wizards[current_player].contacted[p_cities[i].owner] = 1;
                    wizards[p_cities[i].owner].contacted[current_player] = 1;
                }
            }
            for (i = 0; i < num_units; i++)
                Unit_Check_Contact(i, plane);
        }
        plane = 1;
    }
}

/* (x, y, plane, range) marks the explored squares of the (2 range + 1) square around (x, y) visible (x wraps,
   y is clipped); range 0 marks nothing. No return value after the loop */
int Visibility_Mark_Square(int x, int y, int plane, int range)
{
    int yy;
    int xx;
    int wx;
    int x0;
    int y0;

    if (range == 0)
        return 0;
    y0 = y - range;
    if (y0 < 0)
        y0 = 0;
    x0 = x - range;
    if (x0 < 0)
        x0 = x0 + MAP_WIDTH;
    range = range * 2 + 1;
    for (yy = y0; yy < y0 + range; yy++) {
        if (yy < MAP_HEIGHT) {
            for (xx = x0; xx < x0 + range; xx++) {
                wx = xx >= MAP_WIDTH ? xx - MAP_WIDTH : xx;
                if (Tile_Explored(wx, yy, plane) == 1)
                    Tile_Set_Visible(wx, yy, plane);
            }
        }
    }
}

/* (unit, plane) a wizard's unit (not the current player's, not neutral or dead) on this plane or in a tower,
   not invisible and on a visible square, puts its owner and the current player in contact
   Numbered: BUG #297 (BUGS_AND_QUIRKS.txt) */
void Unit_Check_Contact(int unit, int plane)
{
    if (p_units[unit].owner == current_player)
        return;
    if (p_units[unit].owner == PLAYER_NONE)
        return;
    if (p_units[unit].owner == PLAYER_NEUTRAL)
        return;
    if (p_units[unit].plane != plane && p_units[unit].in_tower != 1)
        return;
    if ((p_units[unit].enchantments & UNIT_ENCH_INVISIBILITY) == 0 &&
        Tile_Visible(p_units[unit].x, p_units[unit].y, plane) == 1) {
        wizards[current_player].contacted[p_units[unit].owner] = 1;
        wizards[p_units[unit].owner].contacted[current_player] = 1;
    }
}

/* (x, y, plane) -> 1 when the square has any explored quarter */
int Tile_Explored(int x, int y, int plane)
{
    if (p_exploration_maps[plane][y][x] != 0)
        return 1;
    else
        return 0;
}
