/* MOVE.C - WIZARDS.EXE code segment 5297 (end of the human turn, unit move reset, contacts).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 5297 src/wizards/MOVE.C */
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order; batch 39) */
void Units_Compact(void);
int Stack_Scout_Range(int player, int x, int y, int plane);
void Move_Uncalled_Nop_5297(void);
void Human_End_Turn(void);
void Units_Reset_Moves_NonCurrent(void);
void Units_Reset_Moves(void);
void Wizards_Update_Contacts(void);

extern struct Unit far *p_units;                /* DS:9ec2 */
extern struct City far *p_cities;               /* DS:9cbc */
extern struct Building far *p_building_data;    /* DS:938c */
extern struct Wizard wizards[6];                /* DS:9eca */
extern struct UnitType unit_types[];            /* DS:019c */
extern char *city_size_names[];                 /* DS:0156 */
extern int num_units;                           /* DS:bd92 */
extern int num_players;                         /* DS:bd9c */
extern int current_player;                      /* DS:bd8c */
extern int map_view_x;                          /* DS:bd8a */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_plane;                      /* DS:bd86 */
extern struct GameConfig game_config;           /* DS:bd9e  MAGIC.SET, read / written whole */
extern char city_notice_count;                  /* DS:9af0 */
extern struct CityNotice city_notices[];        /* DS:9af2 */
extern int city_screen_city;                    /* DS:9644 */
extern int current_screen;                      /* DS:9990 */
int building_done_type;                         /* DS:c08c  this module's BSS (owner not fixed by the EXE, b99: any module from MOVE.C to UNITDRAW.C
                                                   gives the same EXE; the 5315 stand-in held it until then) */
extern int no_units_to_move;                    /* DS:9444 */
extern char message_text[];                     /* DS:c7fc */
extern char near_heap[];                        /* DS:d49a  near arena; its first 0x100 bytes are a text scratch */

int Array_Delete();                             /* unprototyped: called with one far pointer for its (offset, segment) words (UTIL.C) */
void far_strcpy(char *dst, int dst_seg, char far *src);
char *Text_Article_A_An(char *word);
void Message_Box(char *text);
void Units_Update_In_Tower(void);
void Units_Set_Draw_Priority(void);
void Map_Build_Unit_Grid(int x, int y, int plane);
void Map_Invalidate(void);
void Next_Turn(void);
void Load_Overland_Screen_Graphics(void);
int Screen_Turn_Report(int mode);
int Player_Stacks_Changed_Nop(int player);
void Map_Center_View(int *view_x, int *view_y, int x, int y, int plane);
void Popup_Building_Completed(int unused_a, int unused_b, int city, int building);
void Screen_City(void);
void Stack_Blink_Off(void);
void Units_Moved_Nop(void);
int Overland_Cast_Spell(int player);
void Next_Unit_Run_GoTos(int player, int *view_x, int *view_y, int *view_plane);
void Stack_Blink_Reset(void);
int Unit_Move_Points(int unit);
void Visibility_Rebuild(void);

/* () deletes every unit record whose owner is outside 0 .. 5 (removed units) and moves the hired heroes'
   unit indexes above it down by one */
void Units_Compact(void)
{
    int player;
    int slot;
    int unit_type;
    int unit;

    unit = 0;
    while (unit < num_units) {
        unit_type = p_units[unit].type;
        if (p_units[unit].owner < 0 || p_units[unit].owner > PLAYER_NEUTRAL) {
            Array_Delete(unit, p_units, sizeof(struct Unit), num_units);
            for (player = 0; player < num_players; player++)
                for (slot = 0; slot < 6; slot++)
                    if (wizards[player].hired_heroes[slot].unit_index != -1 &&
                        wizards[player].hired_heroes[slot].unit_index > unit)
                        wizards[player].hired_heroes[slot].unit_index--;
            num_units--;
        } else
            unit++;
    }
}

/* (player, x, y, plane) -> the best scouting range of the player's units on the square (or in a tower there),
   at least 1 */
int Stack_Scout_Range(int player, int x, int y, int plane)
{
    char best;
    int unit;

    best = 1;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].owner == player &&
            (p_units[unit].plane == plane || p_units[unit].in_tower == 1) &&
            p_units[unit].x == x && p_units[unit].y == y && p_units[unit].scout_range > best)
            best = p_units[unit].scout_range;
    return best;
}

/* () empty; no caller */
void Move_Uncalled_Nop_5297(void)
{
}

/* () the human's End Turn: tidies the unit table, runs Next_Turn, shows the turn report (when the setting is
   on) and each city notice on the city screen - after the "can no longer produce" message (not for Auto
   Choose; the city falls back to Housing) or the completed-building popup - then resets the moves, casts a spell whose cost is paid and runs the GoTo
   orders
   Numbered: BUG #224 (BUGS_AND_QUIRKS.txt) */
void Human_End_Turn(void)
{
    int saved_plane;
    char word[20];
    int i;
    int prod;

    Units_Compact();
    Units_Update_In_Tower();
    Units_Set_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Map_Invalidate();
    Next_Turn();
    Load_Overland_Screen_Graphics();
    Units_Compact();
    Units_Update_In_Tower();
    if (game_config.end_turn_summary == 1)
        Screen_Turn_Report(0);
    if (city_notice_count > 0) {
        Player_Stacks_Changed_Nop(current_player);
        for (i = 0; i < city_notice_count; i++) {
            if (city_notices[i].city != -1) {
                city_screen_city = city_notices[i].city;
                saved_plane = map_view_plane;
                map_view_plane = p_cities[city_screen_city].plane;
                if (city_notices[i].producing < 0) {
                    if (p_cities[city_screen_city].producing != PROD_AUTO_CHOOSE) {
                        strcpy(message_text, "The ");
                        strcat(message_text, city_size_names[p_cities[city_screen_city].size]);
                        strcat(message_text, " of ");
                        far_strcpy(near_heap, 0, p_cities[city_screen_city].name);
                        strcat(message_text, near_heap);
                        strcat(message_text, " can no longer produce ");
                        prod = p_cities[city_screen_city].producing;
                        if (prod >= PROD_UNIT_BASE) {
                            prod -= PROD_UNIT_BASE;
                            strcat(message_text, unit_types[prod].name);
                        } else {
                            far_strcpy(word, 0, p_building_data[prod].name);
                            strcat(message_text, Text_Article_A_An(word));
                            strcat(message_text, " ");
                            strcat(message_text, word);
                        }
                        strcat(message_text, ".");
                        Message_Box(message_text);
                    }
                    p_cities[city_screen_city].producing = PROD_HOUSING;
                } else {
                    building_done_type = city_notices[i].producing;
                    Map_Center_View(&map_view_x, &map_view_y, p_cities[city_screen_city].x,
                                    p_cities[city_screen_city].y, map_view_plane);
                    Popup_Building_Completed(5, 0x65, city_screen_city, building_done_type);   /* 5, 0x65: unused by the callee */
                }
                current_screen = SCREEN_CITY;
                Screen_City();
                Stack_Blink_Off();
                map_view_plane = saved_plane;
                city_notices[i].city = -1;
            }
        }
    }
    current_screen = SCREEN_MAP;
    city_notice_count = 0;
    Units_Reset_Moves();
    Player_Stacks_Changed_Nop(current_player);
    Units_Moved_Nop();
    if (wizards[current_player].casting_cost_left <= 0 && wizards[current_player].casting_spell > 0)
        Overland_Cast_Spell(current_player);
    no_units_to_move = 0;
    Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
    Stack_Blink_Reset();
}

/* () new moves for every unit not of the current player: patrol, road-building and ORDER_UNUSED_6 units
   stay finished, Wait / Done become Ready, units in Stasis keep 0 moves; then the contacts and visibility
   Numbered: BUG #224 (BUGS_AND_QUIRKS.txt) */
void Units_Reset_Moves_NonCurrent(void)
{
    int unit;

    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner != current_player) {
            p_units[unit].finished = 0;
            if (p_units[unit].order == ORDER_PATROL || p_units[unit].order == ORDER_BUILD_ROAD || p_units[unit].order == ORDER_UNUSED_6)
                p_units[unit].finished = 1;
            if (p_units[unit].order == ORDER_WAIT || p_units[unit].order == ORDER_DONE)
                p_units[unit].order = ORDER_READY;
            p_units[unit].moves_max = Unit_Move_Points(unit);
            p_units[unit].moves_left = p_units[unit].moves_max;
            if ((p_units[unit].mutation_flags & (MUT_STASIS_NEW | MUT_STASIS)) != 0) {
                p_units[unit].moves_left = 0;
                p_units[unit].finished = 1;
            }
        }
    }
    Wizards_Update_Contacts();
}

/* () the same for every unit
   Numbered: BUG #224 (BUGS_AND_QUIRKS.txt) */
void Units_Reset_Moves(void)
{
    int unit;

    for (unit = 0; unit < num_units; unit++) {
        p_units[unit].finished = 0;
        if (p_units[unit].order == ORDER_PATROL || p_units[unit].order == ORDER_BUILD_ROAD || p_units[unit].order == ORDER_UNUSED_6)
            p_units[unit].finished = 1;
        if (p_units[unit].order == ORDER_WAIT || p_units[unit].order == ORDER_DONE)
            p_units[unit].order = ORDER_READY;
        p_units[unit].moves_max = Unit_Move_Points(unit);
        p_units[unit].moves_left = p_units[unit].moves_max;
        if ((p_units[unit].mutation_flags & (MUT_STASIS_NEW | MUT_STASIS)) != 0) {
            p_units[unit].moves_left = 0;
            p_units[unit].finished = 1;
        }
    }
    Wizards_Update_Contacts();
}

/* () each wizard is in contact with itself; a wizard with Nature Awareness meets every wizard owning a visible
   (not invisible) unit; then the current player's visibility is rebuilt. Only the Invisibility enchantment is tested
   (not Unit_Is_Invisible): units invisible by ability or by an item still make contact; clarified using ReMoM repo */
void Wizards_Update_Contacts(void)
{
    int player;
    int unit;

    for (player = 0; player < num_players; player++) {
        wizards[player].contacted[player] = 1;
        if (wizards[player].globals.nature_awareness != 0) {
            for (unit = 0; unit < num_units; unit++) {
                if (p_units[unit].owner != PLAYER_NONE && p_units[unit].owner != player &&
                    p_units[unit].owner != PLAYER_NEUTRAL && (p_units[unit].enchantments & UNIT_ENCH_INVISIBILITY) == 0) {
                    wizards[player].contacted[p_units[unit].owner] = 1;
                    wizards[p_units[unit].owner].contacted[player] = 1;
                }
            }
        }
    }
    Visibility_Rebuild();
}
