/* COMBEND.C - WIZARDS.EXE overlay code segment a4fa (stub 2582): the end of a battle - Combat_End (confused,
   possessed and summoned units sorted out, regeneration, recalled units, Create Undead and Zombie Mastery, the city
   captured / razed / plundered with its collateral damage, lair guardians, items and fame of the dead, the
   survivors' damage, relations and fame), the undead-rise picture, the results scroll (with the raze hotkey) and
   its text, the plunder gold and the monsters turning a city into a lair.
   This module's data (DS:5ea6..6065): its literals; BSS DS:c974..c97f (defined here; c980..c985 is SETTINGS.C's, N214).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS a4fa src/wizards/COMBEND.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Combat_End(int winner, int *n_items, int *items, int end_code);
int Undead_Rise_Creator(int player);
void Combat_Results_Scroll_Redraw(void);
void Combat_Results_Scroll(void);
int Combat_Result_Screen(void);
int City_Plunder_Gold(int city);
int Monsters_Take_City(void);

#include "GAME.H"

#define SCROLL_PIC_CITY_SACKED 9        /* SCROLL.LBX "CMBSCRL1" (rendered: a burning town) */
#define SCROLL_PIC_VICTORY 10           /* "CMBWIN": raised banners */
#define SCROLL_PIC_DEFEAT 11            /* "CMBLOSE": ruins, broken standards */

int combat_result_kind;                         /* DS:c97e  Combat_End's end code */
                                                /* the same value is end_code in Combat_Main, again in COMBAUTO.C */
int combat_pop_lost;                            /* DS:c97c  population (thousands) killed */
int *p_combat_bldgs_lost;                       /* DS:c97a  BUG #442: points at Combat_End's local list */
int combat_bldgs_lost_count;                    /* DS:c978 */
int combat_end_value;                           /* DS:c976  first the fame gained, then the gold looted (the scroll shows the gold) */
int city_razed;                                 /* DS:c974 */

extern struct MouseRegion mouse_region_busy;    /* DS:0106 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286  the city / lair attacked */
extern unsigned combat_results_bottom_pic;      /* DS:9470 */
extern int present_mode;                        /* DS:9c8e */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern int city_damage_counter;                 /* DS:c418 */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int combat_active_unit;                  /* DS:c582  here also the city outcome: CITY_FATE_KEPT / RAIDED / RUINS */
#define combat_city_fate combat_active_unit     /* after the battle: the city's fate (CITY_FATE_*) */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int list_count;                          /* DS:c926  here the human's fame change */
#define combat_fame_change list_count           /* the same word, for this file's use */
extern unsigned results_scroll_pic;             /* DS:c9d0 */
extern int combat_result_min_lines;             /* DS:c9d4 */
extern int scroll_popup_height;                 /* DS:c9d6 */
extern int scroll_text_y;                       /* DS:c9d8 */
extern int scroll_popup_y;                      /* DS:c9dc */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
int AI_Decide_Raze(int player, int city);
int Battle_Unit_Max_Hits(struct BattleUnit far *bu);
void City_Change_Owner(int city, int new_owner);
void City_Collateral_Damage(int city, int pop_lost, int bldg_pct, int *destroyed);
int Combat_Retreat_Resolve(int winner);
int Confirm_Dialog(char *text);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
void Heap_Release(unsigned block);
void Item_Free(int item);
char *itoa(int value, char *string, int radix);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Paragraph_Height();                         /* unprototyped: called here with a third (alignment) argument */
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Print_Text_Centered(int x, int y, char *text);
void Screen_Copy_Front(void);
void Screen_Undead_Rise(int creator);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Space_Width(int width);
void Stack_Limit_Enforce(int unit);
int UI_Add_Hotkey(char *key);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Wizard_Fortress_City(int player);
void Wizard_Add_Gold(int player, int amount);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Screen_Restore_A800(void);

/* (winner, &n_items, items[], end_code) after every battle: the units sorted out (a winner outside 0..5 loses all
   its units; confused units back to their owner or dead; set-aside units kept by the winner, dead for the loser; possessed units gone or
   switched; regenerators healed; recalled units to the summoning circle), Create Undead and Zombie Mastery (up to 9
   units in the stack), a captured city (fame, plunder, the raze choice) and its collateral damage, each unit's end
   (dead: items, the hero's fame, Unit_Kill_Or_Dismiss; alive: its damage), relations and fame, then the results
   scroll and the undead messages for the human. BUG #442: p_combat_bldgs_lost keeps a pointer to a local list
   Numbered: BUG #196, #216, #237, #243, #314 (BUGS_AND_QUIRKS.txt) */
void Combat_End(int winner, int *n_items, int *items, int end_code)
{
    long ench;
    int owner;
    int winner_units;
    int kill_value;
    int zombies;
    int risen;
    char num_text[10];
    int hits_bonus;
    int guard2_free;
    int bldg_pct;
    int pop_pct;
    int monsters_take;
    int relation_hit;
    int fame_bonus;
    int destroyed[NUM_BUILDINGS];
    int i;
    int j;

    fame_bonus = 0;
    combat_result_kind = end_code;
    combat_pop_lost = 0;
    combat_bldgs_lost_count = 0;
    combat_end_value = 0;
    if (winner < 0 || winner > 5) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == winner)
                p_battle_units[i].status = BATTLE_UNIT_DESTROYED;
        }
    }
    *n_items = 0;
    kill_value = 0;
    zombies = 0;
    risen = 0;
    guard2_free = 0;
    if (combat_site == COMBAT_AT_LAIR) {
        p_lairs[combat_site_index].guard1_count = p_lairs[combat_site_index].guard1_count & ~LAIR_GUARDS_LEFT_MASK;
        if ((p_lairs[combat_site_index].guard2_count & LAIR_GUARDS_LEFT_MASK) == 0)
            guard2_free = 1;
        p_lairs[combat_site_index].guard2_count = p_lairs[combat_site_index].guard2_count & ~LAIR_GUARDS_LEFT_MASK;
    }
    winner_units = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION) {
            if (p_units[p_battle_units[i].unit_index].owner == winner)
                p_battle_units[i].owner = p_units[p_battle_units[i].unit_index].owner;
            else
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
        }
        if (p_battle_units[i].owner == winner) {
            if (p_battle_units[i].status == BATTLE_UNIT_SET_ASIDE)
                p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
        } else if (p_battle_units[i].status == BATTLE_UNIT_SET_ASIDE) {
            p_battle_units[i].status = BATTLE_UNIT_DEAD;
        }
        ench = p_battle_units[i].combat_enchantments | p_units[p_battle_units[i].unit_index].enchantments |
               p_battle_units[i].item_enchantments;
        if ((p_battle_units[i].combat_flags & COMBAT_FLAG_POSSESSION) || (p_battle_units[i].combat_flags & COMBAT_FLAG_CREATURE_BINDING)) {
            if (p_battle_units[i].owner == winner)
                p_battle_units[i].status = BATTLE_UNIT_DESTROYED;
            else if (p_battle_units[i].owner == combat_attacker_player)
                p_battle_units[i].owner = combat_defender_player;
            else
                p_battle_units[i].owner = combat_attacker_player;
        }
        if ((p_battle_units[i].status <= BATTLE_UNIT_DEAD || p_battle_units[i].status == BATTLE_UNIT_FLED || p_battle_units[i].status == BATTLE_UNIT_RECALLED) &&
            ((ench & UNIT_ENCH_REGENERATION) | (p_battle_units[i].ability_flags & AFLAG_REGENERATION))) {
            if (p_battle_units[i].owner == winner)
                p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
            p_battle_units[i].figures_alive = p_battle_units[i].figures;
            p_battle_units[i].front_damage = 0;
        }
        if (p_battle_units[i].status == BATTLE_UNIT_RECALLED) {
            owner = p_battle_units[i].owner;
            p_units[p_battle_units[i].unit_index].x = wizards[owner].summoning_circle_xyp[XYP_X];
            p_units[p_battle_units[i].unit_index].y = wizards[owner].summoning_circle_xyp[XYP_Y];
            p_units[p_battle_units[i].unit_index].plane = wizards[owner].summoning_circle_xyp[XYP_PLANE];
            p_units[p_battle_units[i].unit_index].finished = 1;
            Stack_Limit_Enforce(p_battle_units[i].unit_index);
            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
            if (owner == winner && owner == combat_attacker_player)
                p_units[p_battle_units[i].unit_index].owner = OWNER_RECALLED;   /* so Overland_Attack, on a
                                                   win, leaves it at the circle and gives the owner back */
        }
        if (p_battle_units[i].owner == winner && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            winner_units++;
    }
    Combat_Retreat_Resolve(winner);
    combat_fame_change = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner != winner && p_battle_units[i].status == BATTLE_UNIT_DRAINED && p_battle_units[i].race != RACE_DEATH_CREATURE &&
            p_units[p_battle_units[i].unit_index].hero_slot == -1 && winner_units < MAX_STACK) {
            p_battle_units[i].figures_alive = p_battle_units[i].figures;
            p_battle_units[i].front_damage = 0;
            p_units[p_battle_units[i].unit_index].x = combat_loc_x;
            p_units[p_battle_units[i].unit_index].y = combat_loc_y;
            p_units[p_battle_units[i].unit_index].plane = combat_loc_plane;
            p_units[p_battle_units[i].unit_index].owner = winner;
            p_units[p_battle_units[i].unit_index].mutation_flags =
                p_units[p_battle_units[i].unit_index].mutation_flags | MUT_UNDEAD;
            p_battle_units[i].owner = winner;
            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
            risen++;
            winner_units++;
            kill_value += 2;
        }
        if (wizards[winner].globals.zombie_mastery > 0 && p_battle_units[i].status == BATTLE_UNIT_DEAD &&
            (char)p_battle_units[i].race < RACE_ARCANE_CREATURE && p_units[p_battle_units[i].unit_index].hero_slot == -1 &&
            winner_units < MAX_STACK) {
            if (p_battle_units[i].owner != winner)
                kill_value += 2;
            zombies++;
            p_units[p_battle_units[i].unit_index].type = UNIT_ZOMBIES;
            p_units[p_battle_units[i].unit_index].owner = winner;
            p_units[p_battle_units[i].unit_index].experience = 0;
            p_units[p_battle_units[i].unit_index].level = 0;
            p_units[p_battle_units[i].unit_index].x = combat_loc_x;
            p_units[p_battle_units[i].unit_index].y = combat_loc_y;
            p_units[p_battle_units[i].unit_index].plane = combat_loc_plane;
            p_battle_units[i].figures_alive = unit_types[UNIT_ZOMBIES].figures;
            p_battle_units[i].figures = unit_types[UNIT_ZOMBIES].figures;
            p_battle_units[i].front_damage = 0;
            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
            winner_units++;
        }
    }
    if (combat_site == COMBAT_AT_CITY) {
        combat_pop_lost = 0;
        combat_bldgs_lost_count = 0;
        if (winner == combat_attacker_player) {
            if (combat_defender_player != PLAYER_NEUTRAL) {
                wizards[combat_defender_player].fame -= p_cities[combat_site_index].size;
                if (wizards[combat_defender_player].fame < 0)
                    wizards[combat_defender_player].fame = 0;
            }
            if (combat_defender_player == PLAYER_HUMAN)
                combat_fame_change -= p_cities[combat_site_index].size;
            if (p_cities[combat_site_index].population == 0) {
                city_razed = 1;
            } else if (winner == PLAYER_HUMAN) {
                if (game_config.raze_city == 1) {
                    i = Confirm_Dialog("Do you wish to completely destroy this city?");
                    if (i == CONFIRM_RAZE)
                        city_razed = 1;
                }
            } else if (winner < num_players) {
                city_razed = AI_Decide_Raze(winner, combat_site_index);
            }
            if (city_razed == 0) {
                combat_end_value = p_cities[combat_site_index].size - 2;
                if (combat_end_value < 0)
                    combat_end_value = 0;
                wizards[winner].fame = wizards[winner].fame + combat_end_value;
                if (winner == PLAYER_HUMAN)
                    combat_fame_change += combat_end_value;
                combat_end_value = 0;               /* reused from here: the gold looted */
                combat_end_value = City_Plunder_Gold(combat_site_index);
                if (winner < num_players)
                    Wizard_Add_Gold(winner, combat_end_value);
                if (p_cities[combat_site_index].owner < num_players)
                    wizards[combat_defender_player].gold -= combat_end_value;
            } else {
                combat_end_value = p_cities[combat_site_index].size;
                if (winner == PLAYER_HUMAN)
                    combat_fame_change -= combat_end_value;
                wizards[winner].fame -= combat_end_value;
                if (wizards[winner].fame < 0)
                    wizards[winner].fame = 0;
                combat_end_value = City_Plunder_Gold(combat_site_index);
                if (p_cities[combat_site_index].owner < num_players)
                    wizards[combat_defender_player].gold -= combat_end_value;
                for (i = BLD_BARRACKS; i < NUM_BUILDINGS; i++) {
                    if (p_cities[combat_site_index].buildings[i] > -1)
                        combat_end_value += p_building_data[i].cost / 10;
                }
                if (winner < num_players)
                    Wizard_Add_Gold(winner, combat_end_value);
            }
        } else {
            combat_end_value = 0;
        }
        if (p_cities[combat_site_index].population != 0) {
            pop_pct = city_damage_counter * 2;
            if (winner == combat_attacker_player) {
                if (winner == PLAYER_NEUTRAL) {
                    for (i = 0; i < battle_unit_count; i++) {
                        if (p_battle_units[i].owner == PLAYER_NEUTRAL) {
                            if (p_battle_units[i].ability_flags & AFLAG_FANTASTIC)
                                monsters_take = 1;
                            else
                                monsters_take = 0;
                            break;
                        }
                    }
                    if (monsters_take == 1) {
                        pop_pct += 50;
                        combat_city_fate = Monsters_Take_City();
                    } else {
                        combat_city_fate = CITY_FATE_RAIDED;
                    }
                } else {
                    pop_pct += 10;
                }
            }
            if (pop_pct > 50)
                pop_pct = 50;
            j = 0;
            for (i = 0; i < p_cities[combat_site_index].population - 1; i++) {
                if (Random(100) <= pop_pct)
                    j++;
            }
            for (i = 0; i < NUM_BUILDINGS; i++)
                destroyed[i] = 0;
            bldg_pct = city_damage_counter;
            if (winner == combat_attacker_player) {
                if (winner == PLAYER_NEUTRAL)
                    bldg_pct += 50;
                else
                    bldg_pct += 10;
            }
            if (bldg_pct > 75)
                bldg_pct = 75;
            if (combat_city_fate != CITY_FATE_RUINS)
                City_Collateral_Damage(combat_site_index, j, bldg_pct, destroyed);
            /* BUG #443: the shift below reuses j (the inhabitants killed), so combat_pop_lost gets its end value when a
               notice was removed; and it moves only .city, not .producing */
            if (winner != combat_defender_player) {
                for (i = 0; i < city_notice_count; i++) {
                    if (city_notices[i].city == combat_site_index) {
                        for (j = i; j < city_notice_count; j++)
                            city_notices[j].city = city_notices[j + 1].city;
                        city_notice_count = city_notice_count - 1;
                    }
                }
            }
            combat_bldgs_lost_count = 0;
            for (i = 0; i < NUM_BUILDINGS; i++) {
                if (destroyed[i] > 0)
                    combat_bldgs_lost_count++;
            }
            combat_pop_lost = j;
            p_combat_bldgs_lost = destroyed;
        }
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (combat_site == COMBAT_AT_LAIR && winner == PLAYER_NEUTRAL && p_battle_units[i].owner == PLAYER_NEUTRAL && p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            if ((int)p_units[p_battle_units[i].unit_index].type == p_lairs[combat_site_index].guard1_type) {
                p_lairs[combat_site_index].guard1_count = p_lairs[combat_site_index].guard1_count + 1;
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
            } else if ((int)p_units[p_battle_units[i].unit_index].type == p_lairs[combat_site_index].guard2_type) {
                p_lairs[combat_site_index].guard2_count = p_lairs[combat_site_index].guard2_count + 1;
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
            } else if (guard2_free == 1) {
                p_lairs[combat_site_index].guard2_type = p_units[p_battle_units[i].unit_index].type;
                p_lairs[combat_site_index].guard2_count = p_lairs[combat_site_index].guard2_count + 0x11;   /* 1 left,
                                                   1 at the start */
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
                guard2_free = 0;
            } else {
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
            }
        }
        relation_hit = 0;                       /* BUG #196: reset per unit - the call after the loop gets the last unit's */
        /* removed: dead units, combat-only summons, and neutral units outside a field battle - all of them at a lair,
           the fantastic ones in a city */
        if (p_battle_units[i].status > 0 || p_units[p_battle_units[i].unit_index].plane == PLANE_COMBAT_ONLY ||
            (p_units[p_battle_units[i].unit_index].owner == PLAYER_NEUTRAL && combat_site != COMBAT_AT_UNITS &&
             (combat_site != COMBAT_AT_CITY || (p_battle_units[i].ability_flags & AFLAG_FANTASTIC)))) {
            if (p_battle_units[i].owner != winner) {
                kill_value += 2;
                if (unit_types[p_units[p_battle_units[i].unit_index].type].cost >= 600)
                    fame_bonus = 1;
            }
            if (p_battle_units[i].owner == combat_defender_player && p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY) {
                if (p_units[p_battle_units[i].unit_index].hero_slot > -1)
                    relation_hit += 20;
                else
                    relation_hit += Random(20);
                if (p_battle_units[i].construction > 0 || (p_battle_units[i].ability_flags & AFLAG_CREATE_OUTPOST))
                    relation_hit += Random(10);
            }
            if (p_units[p_battle_units[i].unit_index].hero_slot > -1) {
                for (j = 0; j < 3; j++) {
                    if (wizards[p_units[p_battle_units[i].unit_index].owner].hired_heroes[p_units[p_battle_units[i].unit_index].hero_slot].item_index[j] > -1) {
                        if (p_battle_units[i].status == BATTLE_UNIT_DESTROYED) {
                            Item_Free(wizards[p_units[p_battle_units[i].unit_index].owner].hired_heroes[p_units[p_battle_units[i].unit_index].hero_slot].item_index[j]);
                        } else if (*n_items < 18) {
                            items[*n_items] = wizards[p_units[p_battle_units[i].unit_index].owner].hired_heroes[p_units[p_battle_units[i].unit_index].hero_slot].item_index[j];
                            *n_items = *n_items + 1;
                        } else {
                            Item_Free(wizards[p_units[p_battle_units[i].unit_index].owner].hired_heroes[p_units[p_battle_units[i].unit_index].hero_slot].item_index[j]);
                        }
                    }
                    wizards[p_units[p_battle_units[i].unit_index].owner].hired_heroes[p_units[p_battle_units[i].unit_index].hero_slot].item_index[j] = -1;
                }
                if (p_battle_units[i].owner != winner) {
                    if (p_battle_units[i].owner == PLAYER_HUMAN)
                        combat_fame_change -= (p_units[p_battle_units[i].unit_index].level + 1) / 2;
                    wizards[p_battle_units[i].owner].fame -= (p_units[p_battle_units[i].unit_index].level + 1) / 2;
                }
            }
            Unit_Kill_Or_Dismiss(p_battle_units[i].unit_index,
                                 (p_battle_units[i].status == BATTLE_UNIT_DESTROYED || (p_units[p_battle_units[i].unit_index].mutation_flags & MUT_UNDEAD)) ? UNIT_REMOVE_GONE : UNIT_REMOVE_KILLED);
        } else {
            if (p_battle_units[i].front_damage < 0)
                p_battle_units[i].front_damage = 0;
            p_battle_units[i].heal_overflow = 0;
            p_battle_units[i].combat_enchantments = 0;
            hits_bonus = Battle_Unit_Max_Hits(&p_battle_units[i]);
            hits_bonus = p_battle_units[i].hits_per_figure - hits_bonus;
            p_battle_units[i].front_damage = p_battle_units[i].front_damage - hits_bonus;
            if (p_battle_units[i].front_damage < 0)
                p_battle_units[i].front_damage = 0;
            p_units[p_battle_units[i].unit_index].damage =
                p_battle_units[i].front_damage + (p_battle_units[i].figures - p_battle_units[i].figures_alive) *
                    (p_battle_units[i].hits_per_figure - hits_bonus);
            p_units[p_battle_units[i].unit_index].moves_left = 0;
        }
    }
    if (combat_defender_player != PLAYER_HUMAN && combat_defender_player != PLAYER_NEUTRAL && combat_attacker_player != PLAYER_NEUTRAL)
        Diplo_Relation_Change(-relation_hit, combat_attacker_player, combat_defender_player, DIPLO_MSG_UNPROVOKED_DESTRUCTION, 0, 0);
    if (combat_site == COMBAT_AT_LAIR && winner == PLAYER_NEUTRAL) {
        if ((p_lairs[combat_site_index].guard1_count & LAIR_GUARDS_LEFT_MASK) == 0 && (p_lairs[combat_site_index].guard2_count & LAIR_GUARDS_LEFT_MASK) > 0) {
            p_lairs[combat_site_index].guard1_count = p_lairs[combat_site_index].guard2_count;
            p_lairs[combat_site_index].guard1_type = p_lairs[combat_site_index].guard2_type;
            p_lairs[combat_site_index].guard2_count = 0;
            p_lairs[combat_site_index].guard2_type = UNIT_GARGOYLES;   /* an empty slot's placeholder (count 0); it
                                                   is a creature type, so the test below hangs on guard1 alone */
        }
        if (risen > 0 && p_lairs[combat_site_index].guard1_type >= UNIT_MAGIC_SPIRIT && p_lairs[combat_site_index].guard2_type >= UNIT_MAGIC_SPIRIT)
            risen = 0;
    }
    if (winner != PLAYER_NEUTRAL && kill_value >= 8) {
        if (winner == PLAYER_HUMAN)
            combat_fame_change++;
        wizards[winner].fame = wizards[winner].fame + 1;
        if (winner == combat_attacker_player) {
            if (wizards[combat_defender_player].fame > 20) {
                if (combat_defender_player == PLAYER_HUMAN)
                    combat_fame_change--;
                wizards[combat_defender_player].fame--;
            }
        } else if (wizards[combat_attacker_player].fame > 20) {
            if (combat_attacker_player == PLAYER_HUMAN)
                combat_fame_change--;
            wizards[combat_attacker_player].fame--;
        }
    }
    if (winner == PLAYER_HUMAN)
        combat_fame_change += fame_bonus;
    wizards[winner].fame += fame_bonus;
    if (combat_attacker_player == PLAYER_HUMAN || combat_defender_player == PLAYER_HUMAN)
        Combat_Results_Scroll();
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == winner &&
            ((char)p_battle_units[i].race < RACE_ARCANE_CREATURE || p_units[p_battle_units[i].unit_index].type == UNIT_HERO_CHOSEN))
            p_units[p_battle_units[i].unit_index].experience += kill_value;
    }
    if (combat_attacker_player == PLAYER_HUMAN || combat_defender_player == PLAYER_HUMAN) {
        if (zombies > 0) {
            itoa(zombies, num_text, 10);
            strcpy(message_text, num_text);
            strcat(message_text, zombies > 1 ? " units have" : " unit has");
            strcat(message_text, " been transformed into zombies.");
            Screen_Undead_Rise(0xae);
        }
        if (risen > 0) {
            itoa(risen, num_text, 10);
            strcpy(message_text, num_text);
            strcat(message_text, risen > 1 ? " units rise from the dead to serve " : " unit rises from the dead to serve ");
            if (winner == PLAYER_HUMAN) {
                strcat(message_text, "you.");
            } else if (winner != PLAYER_NEUTRAL) {
                strcat(message_text, wizards[winner].wizard_name);
                strcat(message_text, ".");
            } else if (risen > 1) {
                strcat(message_text, " their creator.");
            } else {
                strcat(message_text, " its creator.");
            }
            Screen_Undead_Rise(Undead_Rise_Creator(winner));
        }
    }
    if (combat_attacker_player != PLAYER_HUMAN)
        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
}

/* (player) -> the unit type pictured raising the undead: the player's most expensive (upkeep) non-hero unit with
   Life Steal (ATTACK_LIFE_STEAL) or AFLAG_CREATE_UNDEAD; else UNDEAD_FROM_ITEM (10; also for a hero with an
   ITEM_POWER_VAMPIRIC item) */
int Undead_Rise_Creator(int player)
{
    int creator_type;
    int best;
    int score;
    int item;
    register int i;
    register int j;

    best = -1;
    creator_type = UNDEAD_FROM_ITEM;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner == player &&
            ((p_battle_units[i].attack_flags & ATTACK_LIFE_STEAL) || (p_battle_units[i].ability_flags & AFLAG_CREATE_UNDEAD))) {
            score = (char)p_battle_units[i].upkeep + 100;
            if (score > best && p_units[p_battle_units[i].unit_index].type > UNIT_HERO_CHOSEN) {
                best = score;
                creator_type = p_units[p_battle_units[i].unit_index].type;
            }
        } else if (best < 10 && player < 6 && p_units[p_battle_units[i].unit_index].hero_slot > -1) {
            for (j = 0; j < 3; j++) {
                item = wizards[player].hired_heroes[p_units[p_battle_units[i].unit_index].hero_slot].item_index[j];
                if (item > -1 && (p_items[item].powers & ITEM_POWER_VAMPIRIC) != 0) {
                    best = 10;
                    creator_type = UNDEAD_FROM_ITEM;
                }
            }
        }
    }
    return creator_type;
}

/* () redraw callback of the results scroll */
void Combat_Results_Scroll_Redraw(void)
{
    Screen_Restore_A800();
    Set_Clip_Rect(0, 0, 319, scroll_popup_height + scroll_popup_y);
    Draw_Pic_Clipped(54, scroll_popup_y, results_scroll_pic);
    Reset_Clip_Rect();
    Draw_Pic_Frame(54, scroll_popup_y + scroll_popup_height, combat_results_bottom_pic);
    Combat_Result_Screen();
}

/* () the battle results scroll for the human: the scroll picture by the outcome, up to 400 frames or a click; 'r'
   razes a city the human captured (fame - size - 1, the buildings' tenth of cost as gold). BUG #314: the raze loop
   reuses the frame counter
   Numbered: BUG #314 (BUGS_AND_QUIRKS.txt) */
void Combat_Results_Scroll(void)
{
    int confirm;
    int done;
    int control;
    int hotkey_r;
    int hotkey_R;
    int can_raze;
    register int i;
    register int input;

    can_raze = 0;
    if (combat_result_kind == COMBAT_RESULT_EXHAUSTED) {
        results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_DEFEAT, p_scratch);
    } else if (combat_site == COMBAT_AT_CITY) {
        if (combat_result_kind == COMBAT_RESULT_WON || combat_result_kind == COMBAT_RESULT_ENEMY_FLED || combat_result_kind == COMBAT_RESULT_AUTO_WON) {
            if (combat_attacker_player == PLAYER_HUMAN) {
                results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_CITY_SACKED, p_scratch);
                can_raze = 1;
            } else {
                results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_VICTORY, p_scratch);
            }
        } else if (combat_attacker_player == PLAYER_HUMAN) {   /* QUIRK #444: the human's lost city attack shows the
                                                   victory picture */
            results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_VICTORY, p_scratch);
        } else {
            results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_CITY_SACKED, p_scratch);
        }
    } else if (combat_result_kind == COMBAT_RESULT_WON || combat_result_kind == COMBAT_RESULT_ENEMY_FLED || combat_result_kind == COMBAT_RESULT_AUTO_WON) {
        results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_VICTORY, p_scratch);
    } else {
        results_scroll_pic = LBX_Reload_Next("scroll", SCROLL_PIC_DEFEAT, p_scratch);
    }
    combat_results_bottom_pic = LBX_Reload_Next("help", 1, p_scratch);
    Set_Draw_Page();
    scroll_popup_height = Combat_Result_Screen();
    scroll_popup_y = (199 - (scroll_popup_height + 22)) / 2;
    UI_Clear_Controls();
    Set_Draw_Page();
    Combat_Results_Scroll_Redraw();
    present_mode = PRESENT_DISSOLVE;
    Screen_Present();
    present_mode = PRESENT_NORMAL;
    Screen_Copy_Front();
    UI_Set_Redraw_Fn(Combat_Results_Scroll_Redraw, 1);
    done = 0;
    hotkey_r = UI_Add_Hotkey("r");
    hotkey_R = UI_Add_Hotkey("R");
    control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    for (i = 0; i < 400 && done == 0; i++) {
        input = UI_Get_Input();
        if (input == control)
            done = 1;
        if ((input == hotkey_r || input == hotkey_R) && city_razed == 0 && can_raze == 1) {
            confirm = Confirm_Dialog("Do you wish to completely destroy this city?");
            if (confirm == CONFIRM_RAZE) {
                city_razed = 1;
                wizards[PLAYER_HUMAN].fame -= p_cities[combat_site_index].size + 1;
                combat_fame_change -= p_cities[combat_site_index].size + 1;
                for (i = BLD_BARRACKS; i < NUM_BUILDINGS; i++) {
                    if (p_cities[combat_site_index].buildings[i] > -1)
                        combat_end_value += p_building_data[i].cost / 10;
                }
                Wizard_Add_Gold(0, combat_end_value);
                scroll_popup_height = Combat_Result_Screen();
                scroll_popup_y = (199 - (scroll_popup_height + 22)) / 2;
            }
            UI_Clear_Controls();
            control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        }
        if (done == 0) {
            Set_Draw_Page();
            Combat_Results_Scroll_Redraw();
            Screen_Present();
        }
    }
    UI_Suspend_Redraw_Fn();
    Heap_Release(p_scratch);
}

/* () -> the height drawn: the results scroll's text - the outcome line (MESSAGE.LBX records 5..9 or "<city> has
   been conquered"), the city's fate (kept, ruins, raided), the fame change, the loot, and the city's losses
   (destroyed, inhabitants killed, the destroyed buildings in two columns) */
int Combat_Result_Screen(void)
{
    int r;
    char title_colors[5];
    char text_colors[5];
    char num_text[20];
    char text[150];
    int y;
    int j;

    combat_result_min_lines = 0;
    for (j = 0; j < 5; j++)
        title_colors[j] = 0x2f;
    for (j = 0; j < 5; j++)
        text_colors[j] = 0x37;
    text_colors[0] = 0xf7;
    title_colors[0] = 0xf5;
    y = 92;
    Font_Select_Palette(4, title_colors);
    switch (combat_result_kind) {
    case COMBAT_RESULT_WON:
        LBX_Load_Records_Near("message", 0, text, 5, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        break;
    case COMBAT_RESULT_LOST:
        LBX_Load_Records_Near("message", 0, text, 6, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        break;
    case COMBAT_RESULT_RETREATED:
        LBX_Load_Records_Near("message", 0, text, 7, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        if (message_text[0] != 0) {
            Font_Select_Palette(1, text_colors);
            Set_Space_Width(2);
            Paragraph_Print(75, y + scroll_popup_y, 175, message_text, PARA_LEFT);
            y += Paragraph_Height(175, message_text, 0) + 2;
        }
        break;
    case COMBAT_RESULT_EXHAUSTED:
        LBX_Load_Records_Near("message", 0, text, 8, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        break;
    case COMBAT_RESULT_ENEMY_FLED:
        LBX_Load_Records_Near("message", 0, text, 9, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        break;
    case COMBAT_RESULT_CITY_CONQUERED:
        far_strcpy(message_text, 0, p_cities[combat_site_index].name);
        strcat(message_text, " has been conquered");
        Print_Text_Centered(160, scroll_popup_y + 25, message_text);
        break;
    case COMBAT_RESULT_AUTO_WON:
        LBX_Load_Records_Near("message", 0, text, 5, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        break;
    case COMBAT_RESULT_AUTO_LOST:
        LBX_Load_Records_Near("message", 0, text, 6, 1, 150);
        Print_Text_Centered(160, scroll_popup_y + 25, text);
        break;
    }
    if (combat_city_fate == CITY_FATE_KEPT) {
        Font_Select_Palette(1, text_colors);
        LBX_Load_Records_Near("message", 0, text, 10, 1, 150);
        Paragraph_Print(75, y + scroll_popup_y, 175, text, PARA_CENTER);
        y += Paragraph_Height(175, text, 2) + 2;
    }
    if (combat_city_fate == CITY_FATE_RUINS) {
        Font_Select_Palette(1, text_colors);
        far_strcpy(text, 0, p_cities[combat_site_index].name);
        strcat(text, " has been reduced to ruins");
        Paragraph_Print(75, y + scroll_popup_y, 175, text, PARA_CENTER);
        y += Paragraph_Height(175, text, 2) + 2;
    }
    if (combat_city_fate == CITY_FATE_RAIDED) {
        far_strcpy(text, 0, p_cities[combat_site_index].name);
        strcat(text, " has fallen to raiders");
        Paragraph_Print(75, y + scroll_popup_y, 175, text, PARA_CENTER);
        y += Paragraph_Height(175, text, 2) + 2;
    }
    if (combat_fame_change != 0) {
        Font_Select_Palette(1, text_colors);
        strcpy(text, "You have ");
        strcat(text, combat_fame_change < 0 ? "lost " : "gained ");
        itoa(abs(combat_fame_change), num_text, 10);
        strcat(text, num_text);
        strcat(text, " fame.");
        Print_Text_Centered(160, y + scroll_popup_y, text);
        y += 9;
    }
    if (combat_end_value > 0) {
        Font_Select_Palette(1, text_colors);
        Set_Space_Width(2);
        itoa(combat_end_value, num_text, 10);
        strcpy(text, num_text);
        strcat(text, " gold pieces were looted.");
        Print_Text_Centered(160, y + scroll_popup_y, text);
        y += 9;
    }
    if (city_razed == 1) {
        Font_Select_Palette(1, text_colors);
        Set_Space_Width(2);
        Print_Text_Centered(160, y + scroll_popup_y, "The city has been completly destroyed.");
        y += 7;
        y += 2;
    } else {
        if (combat_pop_lost > 0) {
            Font_Select_Palette(1, text_colors);
            Set_Space_Width(2);
            itoa(combat_pop_lost, num_text, 10);
            strcpy(text, num_text);
            strcat(text, " thousand inhabitants killed.");
            Print_Text_Centered(160, y + scroll_popup_y, text);
            y += 7;
            y += 2;
        }
        if (combat_bldgs_lost_count > 0) {
            Font_Select_Palette(4, title_colors);
            Print_Text_Centered(160, y + scroll_popup_y, "Destroyed Buildings");
            y += 11;
            Font_Select_Palette(1, text_colors);
            Set_Space_Width(2);
            scroll_text_y = y;
            for (j = 0; j < combat_bldgs_lost_count; j++) {
                if (p_combat_bldgs_lost[j] > 0)
                    r = Print_Far_Text((j % 2) * 90 + 75, y + scroll_popup_y,
                                       p_building_data[p_combat_bldgs_lost[j]].name);
                if (j % 2 == 1)
                    y += 7;
            }
            y += 2;
        }
        if (combat_result_min_lines > y)
            y = combat_result_min_lines;
    }
    return y;
}

/* (city) -> the gold plundered: a neutral city Random(10) per population; else the owner's gold times the city's
   share of its population */
int City_Plunder_Gold(int city)
{
    int gold;
    int pop_total;
    int i;

    gold = 0;
    if (p_cities[city].owner == PLAYER_NEUTRAL) {
        for (i = 0; i < p_cities[city].population; i++)
            gold += Random(10);
    } else {
        pop_total = 0;
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].owner == p_cities[city].owner)
                pop_total += p_cities[i].population;
        }
        gold = (long)p_cities[city].population * wizards[p_cities[city].owner].gold / pop_total;
    }
    return gold;
}

/* () -> CITY_FATE_KEPT (the city stays) on a 1 in 2, with no free lair slot or for the owner's fortress city; else CITY_FATE_RUINS:
   the city is removed and a lair (type 9) takes its place, guarded by the two most expensive surviving monster
   types (counts x 0x11), its gold the loot */
int Monsters_Take_City(void)
{
    int count_a;
    int count_b;
    int type_a;
    int type_b;
    int kinds;
    int unit_type;
    int slot;
    int i;

    slot = -1;
    for (i = 0; i < NUM_LAIRS && slot == -1; i++) {
        if (p_lairs[i].active == 0)
            slot = i;
    }
    type_a = 0;
    type_b = 0;
    count_a = 0;
    count_b = 0;
    if (Random(2) == 1 || slot == -1 || Wizard_Fortress_City(p_cities[combat_site_index].owner) == combat_site_index) {
        return CITY_FATE_KEPT;
    } else {
        City_Change_Owner(combat_site_index, 5);
        City_Remove(combat_site_index);
        kinds = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == PLAYER_NEUTRAL) {
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
                unit_type = p_units[p_battle_units[i].unit_index].type;
                if (type_a == unit_type) {
                    count_a++;
                } else if (type_b == unit_type) {
                    count_b++;
                } else if (kinds == 0) {
                    type_a = unit_type;
                    count_a = 1;
                    kinds = 1;
                } else if (kinds == 1) {
                    kinds = 2;
                    if (unit_types[unit_type].cost < unit_types[type_a].cost) {
                        type_b = unit_type;
                        count_b = 1;
                    } else {
                        type_b = type_a;
                        count_b = count_a;
                        type_a = unit_type;
                        count_a = 1;
                    }
                } else if (unit_types[unit_type].cost > unit_types[type_a].cost) {
                    type_b = type_a;
                    count_b = count_a;
                    type_a = unit_type;
                    count_a = 1;
                } else if (unit_types[unit_type].cost > unit_types[type_b].cost) {
                    type_b = unit_type;
                    count_b = 1;
                }
            }
        }
        p_lairs[slot].guard1_type = type_a;
        p_lairs[slot].guard2_type = type_b;
        p_lairs[slot].guard1_count = count_a * 0x11;
        p_lairs[slot].guard2_count = count_b * 0x11;
        p_lairs[slot].x = combat_loc_x;
        p_lairs[slot].y = combat_loc_y;
        p_lairs[slot].plane = combat_loc_plane;
        p_lairs[slot].active = 1;
        p_lairs[slot].type = LAIR_RUINS;
        p_lairs[slot].gold = combat_end_value;
        p_lairs[slot].mana = 0;
        p_lairs[slot].special = 0;
        p_lairs[slot].flags = 0;
        p_lairs[slot].item_count = 0;
        p_lairs[slot].item_values[0] = 0;
        p_lairs[slot].item_values[1] = 0;
        p_lairs[slot].item_values[2] = 0;
    }
    return CITY_FATE_RUINS;
}
