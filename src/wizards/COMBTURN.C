/* COMBTURN.C - WIZARDS.EXE overlay code segment 7eb0 (stub 24f1): the battle's turn flow and its armies - the
   combat enchantment icon lists and their hotspots, a side's turn (its wizard may cast, then the AI moves) and the
   end of the human's turn, the retreat after a battle (fled units go back, die or are pushed onto the squares
   around the site; on water they need transport, flight, swimming or wind walking; the losses are listed), the
   mouse cursor over the battlefield (move, melee, shoot, none), loading both armies into the battle unit table
   (Floating Island, ships in a city), combat summons, the airborne / reach tests and a unit's move sound.
   This module's data (DS:56cc..56e7): its literals ("" and " " are tails of other strings); BSS (DS:c79a..c79f):
   three words, owner not fixed by the EXE.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7eb0 src/wizards/COMBTURN.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Combat_Build_Enchant_Icons(void);
int Unit_Has_Combat_Ability(int bu);
int Combat_Leadership_Side(void);
void Combat_Side_Turn(int player);
void Combat_End_Human_Turn(void);
int Combat_Retreat_Resolve(int winner);
int Player_City_At(int x, int y, int plane, int player);
int Retreat_Place_Units_On_Tile(int x, int y, int plane, int player);
int Unit_Place_If_Room(int x, int y, int plane, int unit, int stack_count);
void Combat_Update_Mouse_Cursor(void);
int Ranged_Cursor_For_Target(int att, int def);
void Combat_Add_Enchant_Icon_Controls(void);
int Unit_Combat_Move_Type(int bu);
void Combat_Summon_Add_Unit(int owner, int unit, int x, int y);
void Battle_Slot_Init(int slot, int owner, int unit, int x, int y);
int Combat_Load_Armies(int count, int *units);
int Unit_Flying_Not_Held(int bu, int unused);
int Unit_Is_Airborne(int bu, int unused);
int Can_Attack_Target(int unit, int target);
unsigned Unit_Play_Move_Sound(int unit);
void Combat_Auto_Off(void);
void Units_List_Text(int count, int *units);

#include "GAME.H"

unsigned unit_view_portrait_pic;                /* DS:c79e  BSS DS:c79a..c79f: owner not fixed by the EXE (any module from COMBTURN.C to
                                                   RESUME.C gives the same EXE; the 7eb0 stand-in held them until b104) */
int combat_redraw_needed;                       /* DS:c79c */
int combat_human_turn;                          /* DS:c79a */

extern struct EnchIcon combat_enchant_icon_table[];     /* DS:50ac  per combat enchantment pair: icon, help */
extern int combat_cursor_anim;                  /* DS:7080 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern int combat_active_marker_y;              /* DS:9278 */
extern int combat_active_marker_x;              /* DS:927a */
extern int combat_active_marker_on;             /* DS:927c */
extern int combat_target_y;                     /* DS:927e */
extern int combat_target_x;                     /* DS:9280 */
extern int combat_cursor_mode;                  /* DS:9282 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern unsigned p_world_block;                /* DS:9d20 */
extern int difficulty;                          /* DS:bd96 */
extern int combat_all_units_immobile;                     /* DS:c406 */
extern unsigned combat_silence_sfx;             /* DS:c41a */
extern int combat_attacker_sees_invis;                /* DS:c420 */
extern int combat_ai_moved;                     /* DS:c430 */
extern int combat_auto;                         /* DS:c432 */
extern int combat_ai_side;                      /* DS:c434 */
extern int combat_human_side;                   /* DS:c436 */
extern int combat_def_icon_first;               /* DS:c43a */
extern int combat_att_icon_first;               /* DS:c43c */
extern int combat_enemy_wizard;                 /* DS:c43e */
extern int combat_def_icon_ctls[15];            /* DS:c440 */
extern int combat_att_icon_ctls[15];            /* DS:c45e */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int combat_att_icon_count;               /* DS:c482 */
extern int combat_def_icon_count;               /* DS:c484 */
extern struct PicHelp combat_def_icons[15];     /* DS:c486 */
extern struct PicHelp combat_att_icons[15];     /* DS:c4c2 */
extern unsigned combat_enchant_pics[15];        /* DS:c4fe  COMPIX 5-15, 41, 60, 79, 80 */
extern int *p_combat_cursor_region;             /* DS:c522  one MouseRegion */
extern char *combat_click_map[];                /* DS:c524  per grid row: a battle unit, CLICK_REACHABLE, CLICK_BLOCKED, CLICK_WALL */
extern int combat_active_unit;                  /* DS:c582 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int combat_hover_unit;                   /* DS:c58a */
extern int move_retreat_y;                         /* DS:c5de */
extern int move_retreat_x;                         /* DS:c5e0 */
extern int far *p_aura_leadership;              /* DS:c896  [player] best Leadership of the side */
extern int combat_def_floating_island;          /* DS:d380 */
extern int combat_att_floating_island;          /* DS:d382 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
int Combat_Spell_Action();                      /* no prototype here: called with 4 arguments (1 elsewhere) */
int Combat_Check_Winner(void);
void AI_Combat_Turn(int player);
void Combat_Turn_Start(void);
int Lair_At(int x, int y, int plane);
int Tile_Is_Water(int x, int y, int plane);
int Unit_Can_Cross_Water(int unit);
int Hero_Item_Water_Walking(int unit);
int Unit_Can_Swim(int unit);
void Map_Stack_At(int x, int y, int plane, int *count, int *list);
int Unit_Type_Can_Sail(int unit);
int Mouse_X(void);
int Mouse_Y(void);
int Combat_Screen_To_Grid_X(int sx, int sy);
int Combat_Screen_To_Grid_Y(int sx, int sy);
int Melee_Wall_Allowed(int attacker, int target);
int UI_Add_Picture(int x, int y, int pic, char *hotkey, int help);
int Battle_Free_Gfx_Slot(void);
int Load_Unit_Figure_Frames(int unit_type, int slot);
void Combat_Deploy_Side(int player);
int Sound_Play(unsigned sound);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
int Unit_Load_Move_Sound(int bu);
void Combat_Select_Next_Unit(int player);
void Combat_Build_Click_Map(void);
void Text_List_Separator(int *count, int total, char *text);

/* the attacker's (even slots) and defender's (odd) active combat enchantments; 20 / 21 count when above 0 */
void Combat_Build_Enchant_Icons(void)
{
    int i;
    int half;
    int on;

    combat_att_icon_count = 0;
    combat_def_icon_count = 0;
    for (i = 0; i < NUM_COMBAT_ENCHANTS; i++) {
        on = 0;
        if (p_combat_enchants[i] == CENCH_CAST) {   /* not CENCH_FROM_CITY / _FROM_ETERNAL_NIGHT: no icon */
            on = 1;
        }
        if (i == CENCH_COUNTER_MAGIC_ATT || i == CENCH_COUNTER_MAGIC_DEF) {   /* its strength */
            if (p_combat_enchants[i] > 0) {
                on = 1;
            } else {
                on = 0;
            }
        }
        if (on == 1) {
            half = i / 2;
            if (i % 2 == 0) {
                combat_att_icons[combat_att_icon_count].pic = combat_enchant_pics[combat_enchant_icon_table[half].icon];
                combat_att_icons[combat_att_icon_count].help = combat_enchant_icon_table[half].help;
                combat_att_icon_count++;
            } else {
                combat_def_icons[combat_def_icon_count].pic = combat_enchant_pics[combat_enchant_icon_table[half].icon];
                combat_def_icons[combat_def_icon_count].help = combat_enchant_icon_table[half].help;
                combat_def_icon_count++;
            }
        }
    }
}

/* the Healing, Fire Ball, Doom Bolt or Web spell, Summon Demons or a 40-point caster (ABILITY_CASTER_MP40; a 20-point
   caster fails) (the game tests abilities2 as a byte)
   Numbered: BUG #293 (BUGS_AND_QUIRKS.txt) */
int Unit_Has_Combat_Ability(int bu)
{
    if ((char)p_battle_units[bu].abilities2 & ABILITY2_HEALING_SPELL) {
        return 1;
    }
    if ((char)p_battle_units[bu].abilities2 & ABILITY2_FIRE_BALL_SPELL) {
        return 1;
    }
    if ((char)p_battle_units[bu].abilities2 & ABILITY2_DOOMBOLT_SPELL) {
        return 1;
    }
    if ((char)p_battle_units[bu].abilities2 & ABILITY2_WEB_SPELL) {
        return 1;
    }
    if (p_battle_units[bu].abilities & ABILITY_SUMMON_DEMONS) {
        return 1;
    }
    if (p_battle_units[bu].abilities & ABILITY_CASTER_MP40) {
        return 1;
    }
    return 0;
}

/* no callers: the side with the larger Leadership aura */
int Combat_Leadership_Side(void)
{
    if (p_aura_leadership[combat_attacker_player] > p_aura_leadership[combat_defender_player]) {
        return combat_attacker_player;
    }
    return combat_defender_player;
}

/* (player) a computer-driven side's turn (the AI, or the human's side in auto combat): its wizard may cast first
   (not for the human side), then its units act
   while no side has won */
void Combat_Side_Turn(int player)
{
    int winner;

    if (player == combat_human_side) {
    } else {
        Combat_Spell_Action(player + CASTER_WIZARD, combat_loc_x, combat_loc_y, combat_loc_plane);
    }
    winner = Combat_Check_Winner();
    if (winner == -1) {
        Combat_Build_Enchant_Icons();
        AI_Combat_Turn(player);
    }
}

/* () ends the human's combat turn: hover, marker and cursor mode cleared, the arrow pointer; then the AI side
   moves (unless it already has), and without a winner the next combat turn starts (Combat_Turn_Start), an AI
   defender moves first and, in auto combat, the human's side is played too */
void Combat_End_Human_Turn(void)
{
    int winner;

    combat_hover_unit = -1;
    combat_active_marker_on = 0;
    combat_cursor_mode = 0;
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    combat_human_turn = 0;
    if (combat_ai_moved == 0) {
        combat_active_marker_on = 0;
        combat_cursor_mode = 0;
        Combat_Side_Turn(combat_ai_side);
    }
    winner = Combat_Check_Winner();
    if (winner == -1) {
        Combat_Turn_Start();
        if (combat_defender_player == combat_ai_side) {
            Combat_Side_Turn(combat_defender_player);
            combat_ai_moved = 1;
        }
        combat_human_turn = 1;
        if (combat_auto == 1) {
            Combat_Side_Turn(combat_human_side);
        }
    }
}

/* after a battle with fled units (status 2): the winner's go back to the stack's tile; the loser's die at random
   (1 in 2, a hero 1 in 4; none of the current player's at Intro / Easy, attacking or defending), held / confused / webbed ones always; a defender's survivors
   are pushed onto the 3 x 3 around the site; on water an attacker's need transport, flight, swimming or wind
   walking. The lost units are listed in message_text. BUG #456: at the top row y0 is clamped to 0, so rows 0 .. 2 are
   tried and a survivor can land 2 rows away (x wraps correctly); the water tests ignore Non-Corporeal and test
   Unit_Can_Cross_Water in one branch, Unit_Can_Swim in the other (#321). clarified using ReMoM repo
   Numbered: BUG #321 (BUGS_AND_QUIRKS.txt) */
int Combat_Retreat_Resolve(int winner)
{
    int i;
    int player;
    int att_fled;
    int def_fled;
    int y;
    int x0;
    int y0;
    int span;
    int x;
    int unit;
    int lost_count;
    int transport_space;
    int wind_walker;
    int units[MAX_STACK];

    lost_count = 0;
    if (winner == combat_attacker_player) {
        player = combat_defender_player;
    } else {
        player = combat_attacker_player;
    }
    att_fled = 0;
    def_fled = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_units[p_battle_units[i].unit_index].plane == PLANE_COMBAT_ONLY) {
            p_battle_units[i].status = BATTLE_UNIT_DEAD;
        }
        if (p_battle_units[i].status == BATTLE_UNIT_FLED) {
            if (p_battle_units[i].owner == combat_attacker_player) {
                att_fled++;
            } else if (p_battle_units[i].owner == combat_defender_player) {
                def_fled++;
            }
        }
    }
    if (att_fled == 0 && def_fled == 0) {
        return 0;
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_SET_ASIDE) {
            p_battle_units[i].status = BATTLE_UNIT_FLED;
        }
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == winner) {
            unit = p_battle_units[i].unit_index;
            p_units[unit].x = move_retreat_x;
            p_units[unit].y = move_retreat_y;
            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
        }
    }
    if (player == combat_attacker_player) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_FLED) {
                if (p_units[p_battle_units[i].unit_index].road_turns > -1) {
                    p_units[p_battle_units[i].unit_index].order = ORDER_READY;
                    p_units[p_battle_units[i].unit_index].road_turns = -1;
                }
                if (difficulty > DIFFICULTY_EASY || player != current_player) {
                    if (p_units[p_battle_units[i].unit_index].hero_slot == -1) {
                        if (Random(2) == 1) {
                            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                        }
                    } else {
                        if (Random(4) != 1) {
                            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                        }
                    }
                } else {
                    p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                }
                if ((p_battle_units[i].combat_flags & COMBAT_FLAG_BLACK_SLEEP) || (p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION)
                 || ((p_battle_units[i].combat_flags & COMBAT_FLAG_WEB) && p_battle_units[i].web_hp > 0)) {
                    p_battle_units[i].status = BATTLE_UNIT_FLED;
                }
            }
        }
        if (Tile_Is_Water(combat_loc_x, combat_loc_y, combat_loc_plane) == 1) {
            transport_space = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player
                 && p_battle_units[i].transport_capacity > 0) {
                    transport_space = transport_space + p_battle_units[i].transport_capacity;
                }
            }
            if (transport_space == 0) {
                wind_walker = 0;
                for (i = 0; i < battle_unit_count; i++) {
                    if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player) {
                        unit = p_battle_units[i].unit_index;
                        if (Unit_Has_Wind_Walking(unit) == 1) {
                            wind_walker = 1;
                        }
                    }
                }
                if (wind_walker != 1) {
                    for (i = 0; i < battle_unit_count; i++) {
                        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player) {
                            unit = p_battle_units[i].unit_index;
                            if (Unit_Can_Fly(unit) == 1 || Unit_Can_Cross_Water(unit) == 1 || Hero_Item_Flight(unit) == 1
                             || Hero_Item_Water_Walking(unit) == 1) {
                                p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                            } else {
                                p_battle_units[i].status = BATTLE_UNIT_FLED;
                            }
                        }
                    }
                }
            } else {
                wind_walker = 0;
                for (i = 0; i < battle_unit_count; i++) {
                    if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player) {
                        unit = p_battle_units[i].unit_index;
                        if (Unit_Has_Wind_Walking(unit) == 1) {
                            wind_walker = 1;
                        }
                    }
                }
                if (wind_walker != 1) {
                    x = 0;                      /* x: here the passengers counted */
                    for (i = 0; i < battle_unit_count; i++) {
                        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player
                         && p_battle_units[i].transport_capacity < 1) {
                            if (p_units[p_battle_units[i].unit_index].type <= UNIT_HERO_CHOSEN   /* heroes never need a ride */
                             || Hero_Item_Flight(p_battle_units[i].unit_index) == 1
                             || Hero_Item_Water_Walking(p_battle_units[i].unit_index) == 1
                             || Unit_Can_Swim(p_battle_units[i].unit_index) == 1
                             || Unit_Has_Wind_Walking(p_battle_units[i].unit_index) == 1
                             || Unit_Can_Fly(p_battle_units[i].unit_index) == 1) {
                            } else {
                                x++;
                                if (x > transport_space) {
                                    p_battle_units[i].status = BATTLE_UNIT_FLED;
                                }
                            }
                        }
                    }
                }
            }
        }
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
                units[lost_count] = p_units[p_battle_units[i].unit_index].type;
                lost_count++;
            }
        }
        Units_List_Text(lost_count, units);
        return 0;
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_FLED) {
            if (player != current_player) {
                p_units[p_battle_units[i].unit_index].order = ORDER_READY;
                p_units[p_battle_units[i].unit_index].road_turns = -1;
            }
            if (p_units[p_battle_units[i].unit_index].road_turns > -1) {
                p_units[p_battle_units[i].unit_index].order = ORDER_READY;
                p_units[p_battle_units[i].unit_index].road_turns = -1;
            }
            if (difficulty > DIFFICULTY_EASY || player != current_player) {
                if (p_units[p_battle_units[i].unit_index].hero_slot == -1) {
                    if (Random(2) == 1) {
                        p_battle_units[i].status = BATTLE_UNIT_DEAD;
                    }
                } else {
                    if (Random(4) == 1) {
                        p_battle_units[i].status = BATTLE_UNIT_DEAD;
                    }
                }
            }
            if ((p_battle_units[i].combat_flags & COMBAT_FLAG_BLACK_SLEEP) || (p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION)
             || ((p_battle_units[i].combat_flags & COMBAT_FLAG_WEB) && p_battle_units[i].web_hp > 0)) {
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
            }
            if (p_battle_units[i].status == BATTLE_UNIT_DEAD) {
                units[lost_count] = p_units[p_battle_units[i].unit_index].type;
                lost_count++;
            }
        }
    }
    y0 = combat_loc_y - 1;
    if (y0 < 0) {
        y0 = 0;
    }
    x0 = combat_loc_x - 1;
    if (x0 < 0) {
        x0 = x0 + MAP_WIDTH;
    }
    span = 3;
    for (y = y0; y < y0 + span; y++) {
        if (y < MAP_HEIGHT) {
            for (i = x0; i < x0 + span; i++) {
                if (i >= MAP_WIDTH) {
                    x = i - MAP_WIDTH;
                } else {
                    x = i;
                }
                if (x == combat_loc_x && y == combat_loc_y) {
                } else {
                    if (Lair_At(x, y, combat_loc_plane) == -1 && Player_City_At(x, y, combat_loc_plane, player) == -1) {
                        Retreat_Place_Units_On_Tile(x, y, combat_loc_plane, player);
                    }
                }
            }
        }
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
            p_battle_units[i].status = BATTLE_UNIT_DEAD;
            units[lost_count] = p_units[p_battle_units[i].unit_index].type;
            lost_count++;
        }
    }
    Units_List_Text(lost_count, units);
}

/* the first city of player on the square, -1 when none */
int Player_City_At(int x, int y, int plane, int player)
{
    int found;
    int city;

    found = -1;
    for (city = 0; city < num_cities && found == -1; city++) {
        if (p_cities[city].plane == plane && p_cities[city].owner == player && p_cities[city].y == y
         && p_cities[city].x == x) {
            found = city;
        }
    }
    return found;
}

/* a retreat square: empty or under 9 of player's units, no tower. On land it is refused (0) when any fled unit can
   sail, else every fled unit that fits is placed; on water, with a Wind Walker (fled or there) every one that fits,
   else fliers / water crossers / flight or water-walking items, and the rest as passengers up to the fled
   transports' capacity. -> 1 once the square is accepted, even when nothing fits */
int Retreat_Place_Units_On_Tile(int x, int y, int plane, int player)
{
    int i;
    int unit;
    int stack_count;
    int wind_walker;
    int found;
    int capacity;
    int passengers;
    int stack[MAX_STACK];

    Map_Stack_At(x, y, plane, &stack_count, stack);
    if (stack_count > 0) {
        if (p_units[stack[0]].owner != player) {
            return 0;
        }
        if (stack_count >= MAX_STACK) {
            return 0;
        }
    }
    found = 0;
    for (i = 0; i < NUM_TOWERS && found == 0; i++) {
        if (p_towers[i].x == x && p_towers[i].y == y) {
            return 0;
        }
    }
    wind_walker = 0;
    if (Tile_Is_Water(x, y, plane) == 1) {
        found = 0;
        for (i = 0; i < battle_unit_count && found == 0; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
                unit = p_battle_units[i].unit_index;
                if (unit != -1 && Unit_Has_Wind_Walking(unit) == 1) {
                    wind_walker = 1;
                    found = 1;
                }
            }
        }
        if (found == 0) {
            found = 0;
            for (i = 0; i < stack_count && found == 0; i++) {
                unit = stack[i];
                if (unit != -1 && Unit_Has_Wind_Walking(unit) == 1) {
                    wind_walker = 1;
                    found = 1;
                }
            }
        }
        if (wind_walker == 0) {
            capacity = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
                    unit = p_battle_units[i].unit_index;
                    if (unit_types[p_units[unit].type].transport_capacity > 0) {
                        capacity = capacity + unit_types[p_units[unit].type].transport_capacity;
                    }
                }
            }
            found = 0;
            passengers = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
                    unit = p_battle_units[i].unit_index;
                    if (Unit_Can_Fly(unit) == 1 || Unit_Can_Cross_Water(unit) == 1 || Hero_Item_Flight(unit) == 1
                     || Hero_Item_Water_Walking(unit) == 1) {
                        if (Unit_Place_If_Room(x, y, plane, unit, stack_count) == 1) {
                            stack_count++;
                            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                        }
                    } else {
                        passengers++;
                        if (passengers <= capacity && Unit_Place_If_Room(x, y, plane, unit, stack_count) == 1) {
                            stack_count++;
                            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
                unit = p_battle_units[i].unit_index;
                if (Unit_Type_Can_Sail(unit) == 1) {
                    return 0;
                }
            }
        }
        wind_walker = 1;
    }
    if (wind_walker == 1) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_FLED && p_battle_units[i].owner == player) {
                unit = p_battle_units[i].unit_index;
                if (Unit_Place_If_Room(x, y, plane, unit, stack_count) == 1) {
                    stack_count++;
                    p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
                }
            }
        }
    }
    return 1;
}

/* moves the unit there when the stack has room (under 9) */
int Unit_Place_If_Room(int x, int y, int plane, int unit, int stack_count)
{
    int room;

    room = MAX_STACK - stack_count;
    if (room < 1) {
        return 0;
    }
    p_units[unit].x = x;
    p_units[unit].y = y;
    p_units[unit].plane = plane;
    return 1;
}

/* the cursor over the battlefield: 1 normal, 3 none, 4 shoot, 5 melee, 7 move; combat_hover_unit */
void Combat_Update_Mouse_Cursor(void)
{
    int unit;
    int gx;
    int ddx;
    int ddy;
    int mx;
    int my;
    int gy;
    int rt;

    combat_hover_unit = -1;
    combat_active_marker_on = 0;
    if (combat_active_unit > -1) {
        combat_active_marker_on = 1;
        combat_active_marker_x = p_battle_units[combat_active_unit].combat_x;
        combat_active_marker_y = p_battle_units[combat_active_unit].combat_y;
        if (combat_all_units_immobile == 1 && combat_auto == 0) {
            if (p_battle_units[combat_active_unit].owner != combat_human_side) {
            } else {
                combat_active_marker_on = 0;
            }
        }
    }
    p_combat_cursor_region[MREG_IMAGE] = POINTER_HAND;
    mx = Mouse_X() + 4;
    my = Mouse_Y() + 4;
    combat_cursor_mode = 0;
    if (my < 168) {
        gx = Combat_Screen_To_Grid_X(mx, my);
        gy = Combat_Screen_To_Grid_Y(mx, my);
        switch (combat_click_map[gy][gx]) {
        case CLICK_BLOCKED:
            p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
            break;
        case CLICK_REACHABLE:
            combat_cursor_mode = 1;
            combat_target_x = gx;
            combat_target_y = gy;
            p_combat_cursor_region[MREG_IMAGE] = POINTER_MOVE;
            break;
        case CLICK_WALL:
            combat_cursor_mode = 1;
            combat_target_x = gx;
            combat_target_y = gy;
            combat_cursor_anim = (combat_cursor_anim + 1) % 3;
            unit = combat_click_map[gy][gx];
            ddx = abs(gx - p_battle_units[combat_active_unit].combat_x);
            ddy = abs(gy - p_battle_units[combat_active_unit].combat_y);
            p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
            if (p_battle_units[combat_active_unit].moves > 0) {
                if (ddx <= 1 && ddy <= 1) {
                    p_combat_cursor_region[MREG_IMAGE] = POINTER_MELEE;
                } else {
                    rt = p_battle_units[combat_active_unit].ranged_type / 10;
                    if (rt == RANGED_CLASS_BOULDER || rt == RANGED_CLASS_MISSILE || rt == RANGED_CLASS_MAGIC) {
                        p_combat_cursor_region[MREG_IMAGE] = POINTER_MISSILE;
                    }
                }
            }
            break;
        default:
            combat_cursor_mode = 1;
            combat_target_x = gx;
            combat_target_y = gy;
            combat_cursor_anim = (combat_cursor_anim + 1) % 3;
            unit = combat_click_map[gy][gx];
            combat_hover_unit = unit;
            if ((combat_all_units_immobile != 1 || combat_auto != 0) && p_battle_units[unit].owner != current_player) {
                ddx = abs(p_battle_units[unit].combat_x - p_battle_units[combat_active_unit].combat_x);
                ddy = abs(p_battle_units[unit].combat_y - p_battle_units[combat_active_unit].combat_y);
                p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
                if (p_battle_units[combat_active_unit].moves > 0 && Can_Attack_Target(combat_active_unit, unit) == 1) {
                    if (ddx <= 1 && ddy <= 1) {
                        if (Melee_Wall_Allowed(combat_active_unit, unit) == 1) {
                            p_combat_cursor_region[MREG_IMAGE] = POINTER_MELEE;
                        } else {
                            p_combat_cursor_region[MREG_IMAGE] = Ranged_Cursor_For_Target(combat_active_unit, unit);
                        }
                    } else {
                        p_combat_cursor_region[MREG_IMAGE] = Ranged_Cursor_For_Target(combat_active_unit, unit);
                    }
                }
            }
            break;
        }
    }
    if (combat_auto == 1) {
        combat_active_marker_on = 0;
        combat_cursor_mode = 0;
    }
    p_combat_cursor_region[MREG_POINTER_OFS] = 0;
    p_combat_cursor_region[MREG_X1] = 0;
    p_combat_cursor_region[MREG_Y1] = 0;
    p_combat_cursor_region[MREG_X2] = 319;
    p_combat_cursor_region[MREG_Y2] = 199;
    Mouse_Set_Cursor_Regions(1, (struct MouseRegion *)p_combat_cursor_region);
}

/* POINTER_MISSILE when att can shoot def, else POINTER_REFUSED (an invisible target needs True Sight or the attacking
   side's combat_attacker_sees_invis - read even when att is on the defending side) */
int Ranged_Cursor_For_Target(int att, int def)
{
    int cursor;
    int rt;
    unsigned long att_ench;
    unsigned long def_ench;

    cursor = POINTER_REFUSED;
    rt = p_battle_units[att].ranged_type / 10;
    if (rt == RANGED_CLASS_BOULDER || rt == RANGED_CLASS_MISSILE || rt == RANGED_CLASS_MAGIC) {
        cursor = POINTER_MISSILE;
        def_ench = p_battle_units[def].combat_enchantments | p_battle_units[def].item_enchantments
                 | p_units[p_battle_units[def].unit_index].enchantments;
        att_ench = p_battle_units[att].combat_enchantments | p_battle_units[att].item_enchantments
                 | p_units[p_battle_units[att].unit_index].enchantments;
        if ((att_ench & UNIT_ENCH_TRUE_SIGHT) != 0) {
            cursor = POINTER_MISSILE;
        } else if ((def_ench & UNIT_ENCH_INVISIBILITY) != 0 || (p_battle_units[def].ability_flags & AFLAG_INVISIBILITY)) {
            if (combat_attacker_sees_invis == 1) {
                cursor = POINTER_MISSILE;
            } else {
                cursor = POINTER_REFUSED;
            }
        }
    } else {
        cursor = POINTER_REFUSED;
    }
    return cursor;
}

/* up to 4 icon hotspots per side from its first shown icon, 17 px apart at y 179 (x 247 for the human) */
void Combat_Add_Enchant_Icon_Controls(void)
{
    int i;
    int x;

    if (combat_attacker_player == current_player) {
        x = 247;
    } else {
        x = 8;
    }
    for (i = 0; i + combat_att_icon_first < combat_att_icon_count && i < COMBAT_ICONS_PER_PAGE; i++) {
        combat_att_icon_ctls[i] = UI_Add_Picture(x + i * 17, 179, combat_att_icons[i + combat_att_icon_first].pic, "",
                                                     combat_att_icons[i + combat_att_icon_first].help);
    }
    if (combat_defender_player == current_player) {
        x = 247;
    } else {
        x = 8;
    }
    for (i = 0; i + combat_def_icon_first < combat_def_icon_count && i < COMBAT_ICONS_PER_PAGE; i++) {
        combat_def_icon_ctls[i] = UI_Add_Picture(x + i * 17, 179, combat_def_icons[i + combat_def_icon_first].pic, "",
                                                     combat_def_icons[i + combat_def_icon_first].help);
    }
}

/* (bu) -> its CombatMoveType: flying, swimming, sailing, else walking */
int Unit_Combat_Move_Type(int bu)
{
    if (p_battle_units[bu].move_flags & MOVE_FLYING) {
        return COMBAT_MOVE_FLY;
    } else if (p_battle_units[bu].move_flags & MOVE_SWIMMING) {
        return COMBAT_MOVE_SWIM;
    } else if (p_battle_units[bu].move_flags & MOVE_FORESTER) {
        return COMBAT_MOVE_WALK;
    } else if (p_battle_units[bu].move_flags & MOVE_MOUNTAINEER) {
        return COMBAT_MOVE_WALK;
    } else if (p_battle_units[bu].move_flags & MOVE_SAILING) {
        return COMBAT_MOVE_SAIL;
    }
    return COMBAT_MOVE_WALK;
}

/* a combat summon (on plane 9) joins as a new battle unit, or takes a dead summon's slot (none free: lost)
   Numbered: BUG #180, #291 (BUGS_AND_QUIRKS.txt) */
void Combat_Summon_Add_Unit(int owner, int unit, int x, int y)
{
    int slot;

    if (battle_unit_count < MAX_BATTLE_UNITS) {
        Battle_Slot_Init(battle_unit_count, owner, unit, x, y);
        battle_unit_count++;
    } else {
        for (slot = 0; slot < battle_unit_count; slot++) {
            if (p_battle_units[slot].status > 0 && p_units[p_battle_units[slot].unit_index].plane == PLANE_COMBAT_ONLY) {
                Battle_Slot_Init(slot, owner, unit, x, y);
                return;
            }
        }
    }
}

/* (slot, owner, unit, x, y) a battle unit from the unit at (x, y): its record, a figure graphics slot, the
   animation fields cleared
   Numbered: BUG #291 (BUGS_AND_QUIRKS.txt) */
void Battle_Slot_Init(int slot, int owner, int unit, int x, int y)
{
    int gfx;

    Battle_Unit_Init(unit, &p_battle_units[slot]);
    gfx = Battle_Free_Gfx_Slot();
    p_battle_units[slot].gfx_slot = Load_Unit_Figure_Frames(p_units[unit].type, gfx);
    p_battle_units[slot].owner = owner;
    p_battle_units[slot].combat_x = x;
    p_battle_units[slot].combat_y = y;
    p_battle_units[slot].dest_x = x;
    p_battle_units[slot].dest_y = y;
    p_battle_units[slot].move_anim_step = 0;
    p_battle_units[slot].aura_color = 0;
    p_battle_units[slot].hit_anim_kills = 0;
    p_battle_units[slot].moving = 0;
    p_battle_units[slot].action_state = ACTION_NONE;
}

/* the attacker's units from the list, then the defenders at the site -> the defenders' count
   Numbered: BUG #270 (BUGS_AND_QUIRKS.txt) */
int Combat_Load_Armies(int count, int *units)
{
    int i;
    int def_count;

    for (i = 0; i < MAX_BATTLE_UNITS; i++) {
        p_battle_units[i].status = BATTLE_UNIT_DESTROYED;
        p_battle_units[i].gfx_slot = -1;
    }
    battle_unit_count = 0;
    for (i = 0; i < count; i++) {
        Battle_Unit_Init(units[i], &p_battle_units[battle_unit_count]);
        if (p_units[p_battle_units[battle_unit_count].unit_index].type == (int)p_spell_data[SPELL_FLOATING_ISLAND].effect_param) {
            combat_att_floating_island = 1;
            p_battle_units[battle_unit_count].status = BATTLE_UNIT_SET_ASIDE;
        }
        p_battle_units[battle_unit_count].gfx_slot = Load_Unit_Figure_Frames(p_units[units[i]].type, i);
        p_battle_units[battle_unit_count].owner = combat_attacker_player;
        if (p_battle_units[battle_unit_count].owner != current_player) {
            p_units[p_battle_units[battle_unit_count].unit_index].order = ORDER_READY;
            p_units[p_battle_units[battle_unit_count].unit_index].road_turns = -1;
        }
        battle_unit_count++;
    }
    if (combat_attacker_player != current_player) {
        combat_enemy_wizard = combat_attacker_player;
    }
    def_count = 0;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].x == combat_loc_x && p_units[i].y == combat_loc_y && p_units[i].plane == combat_loc_plane
         && p_units[i].owner == combat_defender_player) {
            if (p_units[i].owner != current_player) {
                p_units[i].order = ORDER_READY;
            }
            Battle_Unit_Init(i, &p_battle_units[battle_unit_count]);
            if (p_units[p_battle_units[battle_unit_count].unit_index].type == (int)p_spell_data[SPELL_FLOATING_ISLAND].effect_param) {
                combat_def_floating_island = 1;
                p_battle_units[battle_unit_count].status = BATTLE_UNIT_SET_ASIDE;
            }
            p_battle_units[battle_unit_count].gfx_slot = Load_Unit_Figure_Frames(p_units[i].type, battle_unit_count);
            p_battle_units[battle_unit_count].owner = combat_defender_player;
            def_count++;
            battle_unit_count++;
        }
    }
    if (combat_defender_player == PLAYER_NONE) {
        combat_defender_player = PLAYER_NO_WIZARD;
    }
    if (combat_attacker_player == PLAYER_NONE) {
        combat_attacker_player = PLAYER_NO_WIZARD;
    }
    if (combat_defender_player != current_player) {
        combat_enemy_wizard = combat_defender_player;
    }
    if (combat_enemy_wizard == -1) {
        combat_enemy_wizard = PLAYER_NO_WIZARD;
    }
    if (combat_attacker_player == current_player) {
        combat_human_side = combat_attacker_player;
        combat_ai_side = combat_defender_player;
    } else {
        combat_human_side = combat_defender_player;
        combat_ai_side = combat_attacker_player;
    }
    for (i = 0; i < battle_unit_count; i++) {
        p_battle_units[i].combat_x = 0;
        p_battle_units[i].combat_y = 0;
        p_battle_units[i].dest_x = 0;
        p_battle_units[i].dest_y = 0;
        p_battle_units[i].move_anim_step = 0;
        p_battle_units[i].aura_color = 0;
        p_battle_units[i].hit_anim_kills = 0;
        p_battle_units[i].moving = 0;
        p_battle_units[i].action_state = ACTION_NONE;
        p_battle_units[i].hit_anim_intensity = 0;
        p_battle_units[i].zeroed_5a = 0;
        p_battle_units[i].always_animate = 0;
        p_battle_units[i].attack_anim_role = 0;
        p_battle_units[i].draw_mode = DRAW_MODE_NORMAL;
        p_battle_units[i].anim_ability = 0;
        if (combat_site == COMBAT_AT_CITY) {
            if (unit_types[p_units[p_battle_units[i].unit_index].type].transport_capacity > 0
             && (p_battle_units[i].move_flags & MOVE_SAILING) && (p_battle_units[i].move_flags & MOVE_FLYING) == 0) {
                p_battle_units[i].status = BATTLE_UNIT_SET_ASIDE;
            }
        }
    }
    Combat_Deploy_Side(combat_attacker_player);
    Combat_Deploy_Side(combat_defender_player);
    if (wizards[combat_attacker_player].casting_spell == SPELL_SPELL_OF_RETURN) {
        wizards[combat_attacker_player].combat_skill_left = 0;
    } else {
        wizards[combat_attacker_player].combat_skill_left = wizards[combat_attacker_player].nominal_casting_skill;
    }
    if (combat_defender_player != PLAYER_NO_WIZARD) {
        if (wizards[combat_defender_player].casting_spell == SPELL_SPELL_OF_RETURN) {
            wizards[combat_defender_player].combat_skill_left = 0;
        } else {
            wizards[combat_defender_player].combat_skill_left = wizards[combat_defender_player].nominal_casting_skill;
        }
    } else {
        wizards[combat_defender_player].combat_skill_left = 0;   /* BUG #457: PLAYER_NO_WIZARD (6) is one record past the
                                                   table (wizards DS:9eca + 6 x 0x4c8 = DS:bb7a, help_entries): this
                                                   writes help_entries[7].x2 (DS:bbc6); clarified using ReMoM repo */
    }
    return def_count;
}

/* flying and neither held nor webbed (the second argument unused) */
int Unit_Flying_Not_Held(int bu, int unused)
{
    int result;

    result = 0;
    if (p_battle_units[bu].move_flags & MOVE_FLYING) {
        result = 1;
    }
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_WEB) {
        result = 0;
    }
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_BLACK_SLEEP) {
        result = 0;
    }
    return result;
}

/* airborne: Wind Walking or Flight enchantment, a flying type or move flag, unless held or webbed (arg unused)
   Numbered: BUG #228 (BUGS_AND_QUIRKS.txt) */
int Unit_Is_Airborne(int bu, int unused)
{
    int airborne;
    unsigned long ench;

    airborne = 0;
    ench = p_battle_units[bu].combat_enchantments | p_units[p_battle_units[bu].unit_index].enchantments
         | p_battle_units[bu].item_enchantments;
    if ((ench & UNIT_ENCH_WIND_WALKING) != 0) {
        airborne = 1;
    }
    if ((ench & UNIT_ENCH_FLIGHT) != 0) {
        airborne = 1;
    }
    if (unit_types[p_units[p_battle_units[bu].unit_index].type].move_flags & MOVE_FLYING) {
        airborne = 1;
    }
    if (p_battle_units[bu].move_flags & MOVE_FLYING) {
        airborne = 1;
    }
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_WEB) {
        airborne = 0;
    }
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_BLACK_SLEEP) {
        airborne = 0;
    }
    return airborne;
}

/* an airborne attacker reaches anything; an airborne target needs a ranged attack */
int Can_Attack_Target(int unit, int target)
{
    int result;
    int unit_air;
    int target_air;

    result = 0;
    unit_air = Unit_Is_Airborne(unit, 1);
    target_air = Unit_Is_Airborne(target, 1);
    if (unit_air == 1) {
        result = 1;
    } else if (target_air == 1) {
        if (p_battle_units[unit].ranged_type > 0) {
            result = 1;
        } else {
            result = 0;
        }
    } else {
        result = 1;
    }
    return result;
}

/* plays the stored move sound, then loads the unit's */
unsigned Unit_Play_Move_Sound(int unit)
{
    unsigned sound;

    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
    }
    Heap_Mark(p_world_block);
    sound = Unit_Load_Move_Sound(unit);
    Heap_Release(p_world_block);
    return sound;
}

/* () Auto combat off: the human's next unit is selected and the click map rebuilt */
void Combat_Auto_Off(void)
{
    combat_auto = 0;
    Combat_Select_Next_Unit(current_player);
    Combat_Build_Click_Map();
}

/* "While fleeing you lost: <n> <type>, ..." into message_text (a single unit drops the plural s) */
void Units_List_Text(int count, int *units)
{
    int i;
    int k;
    int found;
    int total;
    char num_text[6];
    int last;
    int type_counts[MAX_STACK][2];

    if (count > 0) {
        for (i = 0; i < MAX_STACK; i++) {
            type_counts[i][1] = 0;
        }
        total = 0;
        for (i = 0; i < count; i++) {
            found = 0;
            for (k = 0; k < total; k++) {
                if (type_counts[k][0] == units[i]) {
                    type_counts[k][1] = type_counts[k][1] + 1;
                    found = 1;
                }
            }
            if (found == 0) {
                type_counts[total][0] = units[i];
                type_counts[total][1] = type_counts[total][1] + 1;
                total++;
            }
        }
        strcpy(message_text, "While fleeing you lost: ");
        found = 0;
        for (i = 0; i < total; i++) {
            Text_List_Separator(&found, total, message_text);
            itoa(type_counts[i][1], num_text, 10);
            strcat(message_text, num_text);
            strcat(message_text, " ");
            strcat(message_text, unit_types[type_counts[i][0]].name);
            if (type_counts[i][1] == 1) {
                if (type_counts[i][0] == UNIT_HERO_HUNTRESS || type_counts[i][0] == UNIT_HERO_PRIESTESS) {   /* singular
                                                   names ending in 's': kept whole */
                } else {
                    last = strlen(message_text) - 1;
                    if (message_text[last] == 's') {
                        message_text[last] = 0;
                    }
                }
            }
        }
        strcat(message_text, ".");
    } else {
        strcpy(message_text, "");
    }
}
