/* COMBTACT.C - WIZARDS.EXE overlay code segment 9218 (stub 2547): the combat AI's unit tactics - the side's stance
   (melee units hold back or missile units advance by the battle odds and the magic balance), each unit's action
   (approach and attack, shoot, innate and item spells, casting, summoning), the side's rally point, the turn
   driver, the target choice and scan, and moving a unit along a path toward a square or a target.
   This module's data: no literals; BSS DS:c8ae..c8b5 (defined here).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 9218 src/wizards/COMBTACT.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Combat_Stance(int player);
void AI_Unit_Execute(int unit, int rally_x, int rally_y);
void AI_Sort_By_Moves(int *list, int n);
int AI_Rally_Point(int unit, int *x, int *y);
void AI_Combat_Turn(int player);
int AI_Choose_Target(int unit, int mode);
int AI_Target_Scan(int unit, int *action, int ranged);
void AI_Move_Toward(int unit, int x, int y, int target, int rally_x, int rally_y);
int AI_Move_Path(int unit, int x, int y, int target, int rally_x, int rally_y);

#include "GAME.H"

int combat_siege_flags;                         /* DS:c8b4  1 city walls, 2 Wall of Fire, 4 Wall of Darkness (0: the
                                                   defender gave up the city) */
int combat_defender_holds_city;                 /* DS:c8b2 */
int combat_turns_since_ai_acted;                          /* DS:c8b0  + 1 a turn; -1 when a non-human unit moved, attacked or cast */
int ai_combat_late_pass;                        /* DS:c8ae  1 in the heroes' pass when no AI unit has acted this turn */

extern int combat_move_write_only;                  /* DS:7082 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern unsigned p_world_block;                /* DS:9d20 */
extern unsigned combat_silence_sfx;             /* DS:c41a */
extern int combat_defender_sees_invis;          /* DS:c41e */
extern int combat_attacker_sees_invis;                /* DS:c420 */
extern int combat_auto;                         /* DS:c432 */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int combat_active_unit;                  /* DS:c582 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int combat_winner;                       /* DS:c972 */
extern int *p_combat_path_y;                    /* DS:d13e */
extern int *p_combat_path_x;                    /* DS:d140 */
extern int combat_path_len;                     /* DS:d142 */
extern unsigned char *p_combat_path_dist;       /* DS:d146 */
extern unsigned char *p_combat_move_cost;       /* DS:d148 */
extern struct CombatVortex far *p_vortices;     /* DS:d14e */
extern int vortex_count;                        /* DS:d152 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
int Random(int n);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
int Sound_Play(unsigned sound);
void Screen_Present(void);
void Combat_Draw_Screen(void);
void Combat_Set_Active_Unit(int bu);
void Combat_Build_Move_Costs(int unit);
void Combat_Build_Click_Map(void);
int Combat_Unit_Visible_To_Enemy(int bu);
int Combat_Find_Path(int fx, int fy, int tx, int ty);
void Combat_Mark_Wall_Obstacles(int unit);
void Combat_Block_City_Ring(void);
int In_City_Area(int bu);
int Side_Missile_Strength(int player);
int Battle_Odds(int player);
int Unit_Load_Move_Sound(int bu);
int Has_Shooting_Attack(int bu);
int AI_Target_Score(int attacker, int target, int ranged);
int Combat_Distance(int bu1, int bu2);
int Combat_Unit_Attack(int attacker, int target, int x, int y);
int Defender_Shields(int defender, int ranged_type, int damage_flags, int attack_flags, int realm);
int Unit_Combat_Value(int bu);
void Demon_Lord_Summon(int unit);
void Combat_Cast_Spell(int spell, int target, int caster, int x, int y, int mana, int show_anim);
int AI_Combat_Spell_Target(int spell, int player, int *x, int *y);
int Combat_Spell_Action();                      /* no prototype here: called with 4 arguments (1 elsewhere) */
void Combat_Merge_Anim(int unit, int x, int y);
void Combat_Teleport_Anim(int unit, int x, int y);

/* (player) the side's stance: action_state AI_ACTION_MELEE / AI_ACTION_MISSILE per living unit; the magic balance = own
   (spell points / 4 + missile strength) - the enemy's; by Battle_Odds: 0 / 1 - melee units hold back (AI_ACTION_HOLD_BACK) when the
   balance beats a margin of Random(3) - 4 and the side has magic; 2 - the same when the enemy's value is at least
   three times the own melee units' (integer division: > 2) (no test of a zero divisor), missile units advance (AI_ACTION_ADVANCE) when it is under three times, the
   balance is not negative and the side is not defending a siege; 3 - melee units hold back on a positive balance,
   missile units advance on a negative one; above - each missile unit advances on Random(2) == 1.
   -> 0 when either side has no value (the value is unused) */
int AI_Combat_Stance(int player)
{
    int enemy_magic;
    int odds;
    int balance;
    int melee_value;
    int own_value;
    int enemy_value;
    int margin;
    register int i;
    register int own_magic;

    own_magic = 0;
    enemy_magic = 0;
    own_value = 0;
    enemy_value = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            if (p_battle_units[i].owner == player) {
                if (Has_Shooting_Attack(i))
                    p_battle_units[i].action_state = AI_ACTION_MISSILE;
                else
                    p_battle_units[i].action_state = AI_ACTION_MELEE;
                own_value += Unit_Combat_Value(i);
                own_magic += p_battle_units[i].spell_points / 4;
            } else {
                enemy_value += Unit_Combat_Value(i);
                enemy_magic += p_battle_units[i].spell_points / 4;
            }
        }
    }
    own_magic += Side_Missile_Strength(player);
    if (player == combat_attacker_player)
        enemy_magic += Side_Missile_Strength(combat_defender_player);
    else
        enemy_magic += Side_Missile_Strength(combat_attacker_player);
    if (own_value == 0 || enemy_value == 0)
        return 0;
    odds = Battle_Odds(player);
    balance = own_magic - enemy_magic;
    melee_value = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player && p_battle_units[i].action_state == AI_ACTION_MELEE)
            melee_value += Unit_Combat_Value(i);
    }
    margin = Random(3) - 4;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player) {
            switch (odds) {
            case 0:
            case 1:
                if (p_battle_units[i].action_state == AI_ACTION_MELEE && balance > margin && own_magic > 0)
                    p_battle_units[i].action_state = AI_ACTION_HOLD_BACK;
                break;
            case 2:
                if (p_battle_units[i].action_state == AI_ACTION_MELEE && enemy_value / melee_value > 2 && balance > margin &&
                    own_magic > 0)
                    p_battle_units[i].action_state = AI_ACTION_HOLD_BACK;
                if (p_battle_units[i].action_state == AI_ACTION_MISSILE && melee_value != 0 && enemy_value / melee_value <= 2 &&
                    balance >= 0 && (combat_siege_flags == 0 || p_battle_units[i].owner == combat_attacker_player))
                    p_battle_units[i].action_state = AI_ACTION_ADVANCE;
                break;
            case 3:
                if (p_battle_units[i].action_state == AI_ACTION_MELEE) {
                    if (balance > 0)
                        p_battle_units[i].action_state = AI_ACTION_HOLD_BACK;
                } else if (balance < 0)
                    p_battle_units[i].action_state = AI_ACTION_ADVANCE;
                break;
            default:
                if (p_battle_units[i].action_state == AI_ACTION_MISSILE && Random(2) == 1)
                    p_battle_units[i].action_state = AI_ACTION_ADVANCE;
                break;
            }
        }
    }
}

/* (unit, rally x, y or 0, 0 for the target's square) carries out the unit's action_state: AI_ACTION_MELEE - approach and
   attack while the moves fall (behind city walls a unit without a target walks toward the gate (BF_GATE_X, BF_GATE_Y) instead -
   BUG #451: the search for an enemy in the gate tests for a target inside the no-target branch, so it never runs;
   raiders (PLAYER_NEUTRAL) attacking a city without walls and without a target walk into the city core (the nearest reachable of 5..8 x 10..13),
   then toward (0, 0)); AI_ACTION_ADVANCE - one step closer, then shoot while it has moves; AI_ACTION_HOLD_BACK
   - one move toward the target; AI_ACTION_MISSILE - shoot (or walk when out of ammunition); AI_ACTION_DOOM_BOLT,
   _FIREBALL, _WEB (each innate spell used up), _SUMMON_DEMONS; AI_ACTION_ITEM_SPELL / AI_ACTION_CAST (when Combat_Spell_Action
   returns 1 the unit chooses a target with mode 1 and fights, else its moves end). Afterwards action_state ACTION_DONE (unchanged after the
   innate spells and the summoning), and the moves never stay negative. BUG #452: no case for AI_ACTION_HEALING - the
   innate Healing AI_Target_Scan used up is never cast
   Numbered: BUG #198 (BUGS_AND_QUIRKS.txt) */
void AI_Unit_Execute(int unit, int rally_x, int rally_y)
{
    int k;
    int result;
    int proceed;

    if (rally_x == 0 && rally_y == 0) {
        rally_x = p_battle_units[p_battle_units[unit].target_unit].combat_x;
        rally_y = p_battle_units[p_battle_units[unit].target_unit].combat_y;
    }
    Combat_Set_Active_Unit(unit);
    combat_active_unit = unit;
    switch (p_battle_units[unit].action_state) {
    case AI_ACTION_MELEE:
        if (p_battle_units[unit].melee != 0) {
            if ((combat_siege_flags & SIEGE_CITY_WALLS) && p_battle_units[unit].target_unit == -1) {
                proceed = 0;
                if (p_battle_units[unit].owner == combat_attacker_player && p_battle_units[unit].target_unit != -1) {
                    for (k = 0; k < battle_unit_count; k++) {
                        if (p_battle_units[k].combat_x == BF_GATE_X && p_battle_units[k].combat_y == BF_GATE_Y &&
                            p_battle_units[unit].owner != p_battle_units[k].owner && p_battle_units[k].status == BATTLE_UNIT_ACTIVE) {
                            p_battle_units[unit].target_unit = k;
                            proceed = 1;
                        }
                    }
                    if (proceed == 0)
                        AI_Move_Toward(unit, BF_GATE_X, BF_GATE_Y, AI_MOVE_NO_TARGET, BF_GATE_X, BF_GATE_Y);
                } else
                    AI_Move_Toward(unit, BF_GATE_X, BF_GATE_Y, AI_MOVE_NO_TARGET, BF_GATE_X, BF_GATE_Y);
            } else
                proceed = 1;
            if (proceed == 1) {
                k = 220;                        /* the previous moves: above any possible value */
                if (p_battle_units[unit].target_unit == -1 && p_battle_units[unit].owner == PLAYER_NEUTRAL &&
                    p_battle_units[unit].owner == combat_attacker_player && combat_site == COMBAT_AT_CITY)
                    AI_Move_Toward(unit, 0, 0, -1, 0, 0);
                else
                    while (p_battle_units[unit].moves < k && p_battle_units[unit].target_unit > -1 &&
                           p_battle_units[unit].moves > 0 && p_battle_units[unit].status == BATTLE_UNIT_ACTIVE) {
                        k = p_battle_units[unit].moves;
                        if (p_battle_units[unit].target_unit >= 0)
                            AI_Move_Toward(unit, p_battle_units[p_battle_units[unit].target_unit].combat_x,
                                           p_battle_units[p_battle_units[unit].target_unit].combat_y,
                                           p_battle_units[unit].target_unit, rally_x, rally_y);
                        if (p_battle_units[p_battle_units[unit].target_unit].status != BATTLE_UNIT_ACTIVE)
                            AI_Choose_Target(unit, 0);
                    }
            }
        }
        p_battle_units[unit].action_state = ACTION_DONE;
        break;
    case AI_ACTION_ADVANCE:
        if (p_battle_units[unit].target_unit >= 0) {
            if (Combat_Distance(unit, p_battle_units[unit].target_unit) != 1) {
                k = p_battle_units[unit].moves;
                p_battle_units[unit].moves = 1;
                AI_Move_Toward(unit, p_battle_units[p_battle_units[unit].target_unit].combat_x,
                               p_battle_units[p_battle_units[unit].target_unit].combat_y,
                               p_battle_units[unit].target_unit, rally_x, rally_y);
                p_battle_units[unit].moves = k + p_battle_units[unit].moves - 1;
            }
            while (p_battle_units[unit].moves > 0 && p_battle_units[unit].target_unit > -1) {
                Combat_Unit_Attack(unit, p_battle_units[unit].target_unit, 0, 0);
                if (p_battle_units[p_battle_units[unit].target_unit].status != BATTLE_UNIT_ACTIVE)
                    AI_Choose_Target(unit, 0);
            }
        }
        p_battle_units[unit].action_state = ACTION_DONE;
        break;
    case AI_ACTION_HOLD_BACK:
        if (p_battle_units[unit].target_unit >= 0) {
            AI_Move_Toward(unit, p_battle_units[p_battle_units[unit].target_unit].combat_x,
                           p_battle_units[p_battle_units[unit].target_unit].combat_y,
                           p_battle_units[unit].target_unit, rally_x, rally_y);
            if (p_battle_units[p_battle_units[unit].target_unit].status != BATTLE_UNIT_ACTIVE)
                AI_Choose_Target(unit, 0);
        }
        p_battle_units[unit].action_state = ACTION_DONE;
        break;
    case AI_ACTION_MISSILE:
        if (p_battle_units[unit].target_unit >= 0) {
            k = 220;
            while (p_battle_units[unit].moves < k && p_battle_units[unit].target_unit > -1 &&
                   p_battle_units[unit].moves > 0 && p_battle_units[unit].status == BATTLE_UNIT_ACTIVE) {
                k = p_battle_units[unit].moves;
                if (p_battle_units[unit].ranged_type == RANGED_NONE)
                    AI_Move_Toward(unit, p_battle_units[p_battle_units[unit].target_unit].combat_x,
                                   p_battle_units[p_battle_units[unit].target_unit].combat_y,
                                   p_battle_units[unit].target_unit, rally_x, rally_y);
                else
                    Combat_Unit_Attack(unit, p_battle_units[unit].target_unit, 0, 0);
                if (p_battle_units[p_battle_units[unit].target_unit].status != BATTLE_UNIT_ACTIVE)
                    AI_Choose_Target(unit, 0);
            }
        }
        p_battle_units[unit].action_state = ACTION_DONE;
        break;
    case AI_ACTION_DOOM_BOLT:
        p_battle_units[unit].abilities2 = p_battle_units[unit].abilities2 ^ ABILITY2_DOOMBOLT_SPELL;
        Combat_Cast_Spell(SPELL_DOOM_BOLT, p_battle_units[unit].target_unit, unit,
                          p_battle_units[p_battle_units[unit].target_unit].combat_x,
                          p_battle_units[p_battle_units[unit].target_unit].combat_y, 0, 1);
        p_battle_units[unit].moves = 0;
        break;
    case AI_ACTION_FIREBALL:
        p_battle_units[unit].abilities2 = p_battle_units[unit].abilities2 ^ ABILITY2_FIRE_BALL_SPELL;
        Combat_Cast_Spell(SPELL_FIREBALL, p_battle_units[unit].target_unit, unit,
                          p_battle_units[p_battle_units[unit].target_unit].combat_x,
                          p_battle_units[p_battle_units[unit].target_unit].combat_y, 0, 1);
        p_battle_units[unit].moves = 0;
        break;
    case AI_ACTION_SUMMON_DEMONS:
        Demon_Lord_Summon(unit);
        p_battle_units[unit].moves = 0;
        break;
    case AI_ACTION_WEB:
        p_battle_units[unit].abilities2 = p_battle_units[unit].abilities2 ^ ABILITY2_WEB_SPELL;
        Combat_Cast_Spell(SPELL_WEB, p_battle_units[unit].target_unit, unit,
                          p_battle_units[p_battle_units[unit].target_unit].combat_x,
                          p_battle_units[p_battle_units[unit].target_unit].combat_y, 0, 1);
        p_battle_units[unit].moves = 0;
        break;
    case AI_ACTION_ITEM_SPELL:
    case AI_ACTION_CAST:
        result = Combat_Spell_Action(unit, combat_loc_x, combat_loc_y, combat_loc_plane);
        if (result == CAST_RESULT_NONE) {
            AI_Choose_Target(unit, 1);
            if (p_battle_units[unit].target_unit >= 0) {
                k = 220;
                while (p_battle_units[unit].moves < k && p_battle_units[unit].target_unit > -1 &&
                       p_battle_units[unit].moves > 0) {
                    k = p_battle_units[unit].moves;
                    if (p_battle_units[unit].ranged_type == RANGED_NONE)
                        AI_Move_Toward(unit, p_battle_units[p_battle_units[unit].target_unit].combat_x,
                                       p_battle_units[p_battle_units[unit].target_unit].combat_y,
                                       p_battle_units[unit].target_unit, rally_x, rally_y);
                    else
                        Combat_Unit_Attack(unit, p_battle_units[unit].target_unit, 0, 0);
                    if (p_battle_units[p_battle_units[unit].target_unit].status != BATTLE_UNIT_ACTIVE)
                        AI_Choose_Target(unit, 0);
                }
            }
        } else
            p_battle_units[unit].moves = 0;
        p_battle_units[unit].action_state = ACTION_DONE;
        break;
    }
    if (p_battle_units[unit].moves < 0)
        p_battle_units[unit].moves = 0;
}

/* (list of battle units, n) sorts the list by the units' moves, fewest first (insertion sort on char indices) */
void AI_Sort_By_Moves(int *list, int n)
{
    char bu;
    int i;
    char cur_moves;
    char moves;
    char j;

    for (i = 1; i < n; i++) {
        bu = list[i];
        j = i - 1;
        cur_moves = p_battle_units[list[j]].moves;
        moves = p_battle_units[bu].moves;
        while (j > -1 && cur_moves > moves) {
            list[j + 1] = list[j];
            j--;
            if (j > -1)
                cur_moves = p_battle_units[list[j]].moves;
        }
        list[j + 1] = bu;
    }
}

/* (unit, &x, &y) the side's rally point: an AI_ACTION_HOLD_BACK or AI_ACTION_MISSILE unit's own square (-> 0); else the
   farthest square of the path toward its target that it reaches with its moves (other units' squares blocked, the
   target's costing 2, vortices blocked) - BUG #330: an empty path (target unreachable) reads path[-1]; a missing target
   (-1) is not tested. No value then
   Numbered: BUG #330 (BUGS_AND_QUIRKS.txt) */
int AI_Rally_Point(int unit, int *x, int *y)
{
    int i;

    if (p_battle_units[unit].action_state == AI_ACTION_HOLD_BACK || p_battle_units[unit].action_state == AI_ACTION_MISSILE) {
        *x = p_battle_units[unit].combat_x;
        *y = p_battle_units[unit].combat_y;
        return 0;
    }
    Combat_Build_Move_Costs(unit);
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[unit].target_unit == i)
            p_combat_move_cost[p_battle_units[i].combat_y * BATTLEFIELD_W + p_battle_units[i].combat_x] = 2;
        else if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            p_combat_move_cost[p_battle_units[i].combat_y * BATTLEFIELD_W + p_battle_units[i].combat_x] = COST_IMPASSABLE;
    }
    for (i = 0; i < vortex_count; i++)
        p_combat_move_cost[p_vortices[i].y * BATTLEFIELD_W + p_vortices[i].x] = COST_IMPASSABLE;
    Combat_Find_Path(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y,
                     p_battle_units[p_battle_units[unit].target_unit].combat_x,
                     p_battle_units[p_battle_units[unit].target_unit].combat_y);
    for (i = 0; i < combat_path_len && (i == 0 || p_battle_units[unit].moves -
                                        p_combat_path_dist[p_combat_path_y[i - 1] * BATTLEFIELD_W + p_combat_path_x[i - 1]] > 0);
         i++)
        ;
    *x = p_combat_path_x[i - 1];
    *y = p_combat_path_y[i - 1];
}

/* (player) the AI side's combat turn: attacking raiders (5) give up (status 4) after more than 3 idle turns; the siege flags
   (walls, Wall of Fire, Wall of Darkness), dropped when the defender gives up the city (enemy vortices, combat
   enchantments 8 / 0x12 / 0x1a, or a strong enemy missile force against none of its own); the stance; the rally
   point (the middle melee unit's, when the own missile strength is not below the enemy's); then every living unit
   with moves (combat_flags 0x40 clear) chooses a target and acts - a defender outside the city during a siege
   rallies on its target - and once more the heroes with moves left. Stops when a winner is decided.
   BUGS #18: own_missile and enemy_missile are added to uninitialised; with no melee unit the rally point reads
   melee_units[0], never set
   Numbered: BUG #244, #303, #330 (BUGS_AND_QUIRKS.txt) */
void AI_Combat_Turn(int player)
{
    int melee_count;
    int side_rally_x;
    int side_rally_y;
    int own_missile;
    int enemy_missile;
    int enemy_vortices;
    int melee_units[MAX_BATTLE_UNITS];
    int i;

    if (player == PLAYER_NEUTRAL && player == combat_attacker_player && combat_turns_since_ai_acted > 3) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == PLAYER_NEUTRAL)
                p_battle_units[i].status = BATTLE_UNIT_DEAD;
        }
    }
    own_missile += Side_Missile_Strength(player);
    if (player == combat_attacker_player)
        enemy_missile += Side_Missile_Strength(combat_defender_player);
    else
        enemy_missile += Side_Missile_Strength(combat_attacker_player);
    combat_siege_flags = 0;
    if (p_battlefield->has_city_walls == 1)
        combat_siege_flags = combat_siege_flags | SIEGE_CITY_WALLS;
    if (p_battlefield->wall_of_darkness == 1)
        combat_siege_flags = combat_siege_flags | SIEGE_WALL_OF_DARKNESS;
    if (p_battlefield->wall_of_fire == 1)
        combat_siege_flags = combat_siege_flags | SIEGE_WALL_OF_FIRE;
    if (player == combat_defender_player) {
        if (combat_defender_holds_city == 1) {
            enemy_vortices = 0;
            if (vortex_count > 0) {
                for (i = 0; i < vortex_count; i++) {
                    if (p_vortices[i].owner == combat_attacker_player)
                        enemy_vortices++;
                }
            }
            if (enemy_missile > 30 && own_missile == 0) {
                combat_siege_flags = 0;
                combat_defender_holds_city = 0;
            }
            if (enemy_vortices > 0 || p_combat_enchants[CENCH_WRACK_ATT] > 0 || p_combat_enchants[CENCH_CALL_LIGHTNING_ATT] > 0 ||
                (p_combat_enchants[CENCH_MANA_LEAK_ATT] > 0 && player < num_players)) {
                combat_siege_flags = 0;
                combat_defender_holds_city = 0;
            }
        } else
            combat_siege_flags = 0;
    }
    combat_winner = -1;
    AI_Combat_Stance(player);
    if (own_missile >= enemy_missile) {
        melee_count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                (p_battle_units[i].action_state == AI_ACTION_MELEE || p_battle_units[i].action_state == AI_ACTION_HOLD_BACK)) {
                melee_units[melee_count] = i;
                melee_count++;
            }
        }
        AI_Sort_By_Moves(melee_units, melee_count);
        AI_Rally_Point(melee_units[(melee_count - 1) / 2], &side_rally_x, &side_rally_y);
    } else {
        side_rally_x = 0;
        side_rally_y = 0;
    }
    ai_combat_late_pass = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].moves > 0 &&
            (p_battle_units[i].combat_flags & COMBAT_FLAG_BLACK_SLEEP) == 0) {
            Combat_Set_Active_Unit(i);
            Combat_Build_Click_Map();
            AI_Choose_Target(i, 0);
            if (p_battle_units[i].owner == combat_defender_player && combat_siege_flags > 0 && In_City_Area(i) == 0)
                AI_Unit_Execute(i, 0, 0);
            else
                AI_Unit_Execute(i, side_rally_x, side_rally_y);
        }
        if (combat_winner != -1)
            break;
    }
    if (combat_turns_since_ai_acted != -1)
        ai_combat_late_pass = 1;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].moves > 0 &&
            p_units[p_battle_units[i].unit_index].hero_slot > -1 && (p_battle_units[i].combat_flags & COMBAT_FLAG_BLACK_SLEEP) == 0) {
            Combat_Set_Active_Unit(i);
            Combat_Build_Click_Map();
            AI_Choose_Target(i, 0);
            AI_Unit_Execute(i, side_rally_x, side_rally_y);
        }
        if (combat_winner != -1)
            break;
    }
}

/* (unit, mode 1: after Combat_Spell_Action returned 1 - the scan skips the random cast) a living unit's target: the
   AI_Target_Scan winner (its action becomes action_state, 0 / AI_ACTION_NO_CAST -> AI_ACTION_MELEE or _MISSILE); a bow or sling unit (ranged type
   20-29) drops a Missile Immune target unless its ranged x 2 beats its melee x 3; a missile unit without a target
   retries as melee; melee and missile units of a defender (or with no siege) then switch to the nearest enemy of
   the chosen one's type and defense, and melee units join an earlier ally's target of that type and defense (focus
   fire - BUG #453: an ally without a target reads p_battle_units[-1]). Stores the target (a byte); -1 for a dead unit,
   else no value */
int AI_Choose_Target(int unit, int mode)
{
    int target;
    int action;
    int ranged;
    int owner;
    int other;

    if (p_battle_units[unit].status != BATTLE_UNIT_ACTIVE)
        return -1;
    if (mode == 1)
        action = AI_ACTION_NO_CAST;
    else
        action = 0;
    owner = p_battle_units[unit].owner;
    target = -1;
    ranged = Has_Shooting_Attack(unit);
    target = AI_Target_Scan(unit, &action, ranged);
    if (target > -1 && ranged == 1) {
        if (p_battle_units[unit].ranged_type / 10 == RANGED_CLASS_MISSILE && (p_battle_units[target].abilities & ABILITY_MISSILES_IMMUNITY) &&
            p_battle_units[unit].melee * 3 >= p_battle_units[unit].ranged * 2)
            target = -1;
    }
    if (ranged == 1 && target == -1 && (action == 0 || action == AI_ACTION_NO_CAST || action == AI_ACTION_MISSILE || action == AI_ACTION_ADVANCE)) {
        ranged = 0;
        target = AI_Target_Scan(unit, &action, ranged);
        if (action == AI_ACTION_MISSILE || action == AI_ACTION_ADVANCE)
            action = AI_ACTION_MELEE;
    }
    if (action == 0 || action == AI_ACTION_NO_CAST) {
        if (ranged == 1)
            p_battle_units[unit].action_state = AI_ACTION_MISSILE;
        else
            p_battle_units[unit].action_state = AI_ACTION_MELEE;
    } else
        p_battle_units[unit].action_state = action;
    if ((p_battle_units[unit].action_state == AI_ACTION_MISSILE || p_battle_units[unit].action_state == AI_ACTION_MELEE) &&
        (p_battle_units[unit].owner == combat_defender_player || combat_siege_flags == 0)) {
        if (target > -1 && target < MAX_BATTLE_UNITS) {   /* a battle unit, not TARGET_SQUARE */
            for (other = 0; other < battle_unit_count; other++) {
                if (p_battle_units[other].status == BATTLE_UNIT_ACTIVE && p_battle_units[other].owner != p_battle_units[unit].owner &&
                    p_units[p_battle_units[target].unit_index].type == p_units[p_battle_units[other].unit_index].type &&
                    p_battle_units[target].defense == p_battle_units[other].defense &&
                    Combat_Distance(unit, other) < Combat_Distance(unit, target))
                    target = other;
            }
        }
        if (ranged == 0) {
            for (other = 0; other < unit; other++) {
                if (p_battle_units[other].status == BATTLE_UNIT_ACTIVE && p_battle_units[other].owner == p_battle_units[unit].owner &&
                    p_units[p_battle_units[p_battle_units[other].target_unit].unit_index].type ==
                    p_units[p_battle_units[target].unit_index].type &&
                    p_battle_units[p_battle_units[other].target_unit].status == BATTLE_UNIT_ACTIVE &&
                    p_battle_units[p_battle_units[other].target_unit].defense == p_battle_units[target].defense)
                    target = p_battle_units[other].target_unit;
            }
        }
    }
    p_battle_units[unit].target_unit = target;
}

/* (unit, &action, ranged) -> the unit's target, with *action set: a Demon Lord with summons left and room (fewer
   than 36 battle units, 1000 units, 9 own units) summons (AI_ACTION_SUMMON_DEMONS, target TARGET_SQUARE); innate Doom Bolt (AI_ACTION_DOOM_BOLT, score 30),
   Fireball (AI_ACTION_FIREBALL, score 16 - the target's shields), Healing (AI_ACTION_HEALING, used up) and Web
   (AI_ACTION_WEB) when they find a target; a hero's item spell with charges (AI_ACTION_ITEM_SPELL); a caster with
   more than 2 spell points casts (AI_ACTION_CAST) when ranged - 5 <= Random(15) (not when *action is
   AI_ACTION_NO_CAST). Else the best AI_Target_Score among the enemies it may attack
   (*action 0): a missile unit skips invisible targets and, behind a Wall of Darkness, targets in the city; heroes
   (in melee) skip targets they cannot hurt or that outclass them (until the late pass); a besieged defender inside the city
   does not chase targets outside it. Missile units take, among scores within 3, the target with fewer hits left;
   melee units, among equal scores, the one with more. -1 once the battle is won
   Numbered: BUG #296 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Scan(int unit, int *action, int ranged)
{
    int hp;                                     /* also the hero slot */
    int best_hp;
    int best_score;
    int target;
    int value;
    int chosen;
    int spell;
    int spell_x;
    int spell_y;
    int i;

    best_score = -100;
    target = -1;
    chosen = 0;
    if (combat_winner != -1)
        return -1;
    if ((p_battle_units[unit].abilities & ABILITY_SUMMON_DEMONS) && battle_unit_count < MAX_BATTLE_UNITS && num_units < MAX_UNITS) {
        value = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == p_battle_units[unit].owner && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
                value++;
        }
        if (value < MAX_STACK) {
            *action = AI_ACTION_SUMMON_DEMONS;
            chosen = 1;
            target = TARGET_SQUARE;
        }
    }
    if ((p_battle_units[unit].abilities2 & ABILITY2_DOOMBOLT_SPELL) && chosen == 0) {
        spell = SPELL_DOOM_BOLT;
        target = AI_Combat_Spell_Target(spell, p_battle_units[unit].owner, &spell_x, &spell_y);
        if (target > -1) {
            best_score = 30;
            *action = AI_ACTION_DOOM_BOLT;
        }
    }
    if ((p_battle_units[unit].abilities2 & ABILITY2_FIRE_BALL_SPELL) && chosen == 0) {
        spell = SPELL_FIREBALL;
        target = AI_Combat_Spell_Target(spell, p_battle_units[unit].owner, &spell_x, &spell_y);
        if (target > -1) {
            best_score = 16 - Defender_Shields(target, RANGED_MAGIC_FIRE, ABILITY_FIRE_IMMUNITY | ABILITY_MAGIC_IMMUNITY, 0, REALM_CHAOS);   /* Fireball */
            *action = AI_ACTION_FIREBALL;
        }
    }
    if ((p_battle_units[unit].abilities2 & ABILITY2_HEALING_SPELL) && chosen == 0) {
        spell = SPELL_HEALING;
        target = AI_Combat_Spell_Target(spell, p_battle_units[unit].owner, &spell_x, &spell_y);
        if (target > -1) {
            chosen = 1;
            p_battle_units[unit].abilities2 = p_battle_units[unit].abilities2 ^ ABILITY2_HEALING_SPELL;
            *action = AI_ACTION_HEALING;
        }
    }
    if ((p_battle_units[unit].abilities2 & ABILITY2_WEB_SPELL) && chosen == 0) {
        spell = SPELL_WEB;
        target = AI_Combat_Spell_Target(spell, p_battle_units[unit].owner, &spell_x, &spell_y);
        if (target > -1) {
            chosen = 1;
            *action = AI_ACTION_WEB;
        }
    }
    if (p_units[p_battle_units[unit].unit_index].hero_slot > -1 && chosen == 0) {
        hp = p_units[p_battle_units[unit].unit_index].hero_slot;   /* hp reused: the hero slot */
        value = (unsigned char)p_items[wizards[p_battle_units[unit].owner].hired_heroes[hp].item_index[0]].spell;   /* by the battle owner (#289) */
        if (value > 0 && p_battle_units[unit].item_charges > 0) {
            spell = value;
            target = AI_Combat_Spell_Target(spell, p_battle_units[unit].owner, &spell_x, &spell_y);
            if (target > -1) {
                chosen = 1;
                *action = AI_ACTION_ITEM_SPELL;
            }
        }
    }
    if (p_battle_units[unit].spell_points > 2 && chosen == 0 && *action != AI_ACTION_NO_CAST) {
        if (p_battle_units[unit].ranged - 5 <= Random(15)) {
            *action = AI_ACTION_CAST;
            chosen = 1;
        }
    }
    if (chosen == 0) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[unit].owner == p_battle_units[i].owner)
                continue;
            if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                continue;
            if (ranged == 1 && ((p_battle_units[i].combat_enchantments & UNIT_ENCH_INVISIBILITY) != 0 ||
                                (p_battle_units[i].item_enchantments & UNIT_ENCH_INVISIBILITY) != 0 ||
                                (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_INVISIBILITY) != 0 ||
                                (p_battle_units[i].ability_flags & AFLAG_INVISIBILITY) != 0))
                continue;
            /* the city's own Wall of Darkness only: one cast in this battle sets p_battlefield->wall_of_darkness alone, so
               the AI still aims at targets behind it (Ranged_Target_Blocked then refuses; #198, COMMUNITY-KNOWN W455AI) */
            if (ranged == 1 && p_battlefield->city_enchants[CITY_ENCH_WALL_OF_DARKNESS] > 0 && (p_battle_units[unit].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0 &&
                In_City_Area(unit) == 0 && In_City_Area(i) == 1)
                continue;
            if (Combat_Unit_Visible_To_Enemy(i) == 0 && (p_battle_units[i].combat_x != BF_GATE_X || p_battle_units[i].combat_y != BF_GATE_Y))
                continue;                       /* invisible targets are skipped, except in the gate */
            if (p_units[p_battle_units[unit].unit_index].hero_slot > -1 && ranged == 0 && ai_combat_late_pass != 1 &&
                (p_battle_units[unit].melee < p_battle_units[i].defense ||
                 p_battle_units[unit].defense < p_battle_units[i].melee * 2 / 3 ||
                 (p_battle_units[i].ranged_type >= RANGED_THROWN &&
                  p_battle_units[unit].defense < p_battle_units[i].ranged * 2 / 3)))
                continue;
            if (combat_siege_flags > 0 && ranged == 0 && Combat_Distance(unit, i) != 1 &&
                p_battle_units[unit].owner == combat_defender_player && In_City_Area(unit) == 1 &&
                ((p_battle_units[unit].combat_x == BF_GATE_X && p_battle_units[unit].combat_y == BF_GATE_Y &&
                  (combat_siege_flags & SIEGE_CITY_WALLS)) || In_City_Area(i) == 0))
                continue;
            value = AI_Target_Score(unit, i, ranged);
            if (ranged == 1) {
                if (best_score - 3 <= value && value - 3 <= best_score && target > -1) {
                    hp = p_battle_units[i].figures_alive * p_battle_units[i].hits_per_figure -
                         p_battle_units[i].front_damage;
                    best_hp = p_battle_units[target].figures_alive * p_battle_units[target].hits_per_figure -
                              p_battle_units[target].front_damage;
                    if (hp < best_hp) {
                        if (best_score < value)
                            best_score = value;
                        target = i;
                        *action = 0;
                    }
                } else if (best_score < value) {
                    best_score = value;
                    target = i;
                    *action = 0;
                }
            } else if (value >= best_score) {
                if (value == best_score) {
                    hp = p_battle_units[i].figures_alive * p_battle_units[i].hits_per_figure -
                         p_battle_units[i].front_damage;
                    best_hp = p_battle_units[target].figures_alive * p_battle_units[target].hits_per_figure -
                              p_battle_units[target].front_damage;
                    if (hp > best_hp) {
                        best_score = value;
                        target = i;
                        *action = 0;
                    }
                } else {
                    best_score = value;
                    target = i;
                    *action = 0;
                }
            }
        }
    }
    return target;
}

/* (unit, x, y, target or -1 / -666, rally x, y) the cost map with the other living units, the walls, the city ring
   (a defender holding the city inside a Wall of Fire or Darkness) and the vortices blocked; an attacker whose
   target square is blocked (or without a target) first walks to the nearest reachable square of the city core
   (x 5-8, y 10-13 - also in a battle without a city), then AI_Move_Path
   Numbered: BUG #200 (BUGS_AND_QUIRKS.txt) */
void AI_Move_Toward(int unit, int x, int y, int target, int rally_x, int rally_y)
{
    int tx;
    int ty;
    int best_len;
    int best_x;
    int best_y;
    int i;

    Combat_Build_Move_Costs(unit);
    for (i = 0; i < battle_unit_count; i++) {
        if (i != target && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            p_combat_move_cost[p_battle_units[i].combat_y * BATTLEFIELD_W + p_battle_units[i].combat_x] = COST_IMPASSABLE;
    }
    Combat_Mark_Wall_Obstacles(unit);
    if (p_battle_units[unit].owner == combat_defender_player && combat_defender_holds_city == 1 &&
        (p_battlefield->wall_of_fire > 0 || p_battlefield->wall_of_darkness > 0) && In_City_Area(unit) == 1)
        Combat_Block_City_Ring();
    for (i = 0; i < vortex_count; i++)
        p_combat_move_cost[p_vortices[i].y * BATTLEFIELD_W + p_vortices[i].x] = COST_IMPASSABLE;
    Combat_Find_Path(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y, x, y);
    if (((target > -1 && p_combat_move_cost[y * BATTLEFIELD_W + x] == COST_IMPASSABLE) || target == -1) &&
        p_battle_units[unit].owner == combat_attacker_player) {
        best_len = 999;
        for (tx = BF_CITY_X1; tx < BF_CITY_X2 + 1; tx++) {
            for (ty = BF_CITY_Y1; ty < BF_CITY_Y2 + 1; ty++) {
                Combat_Find_Path(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y, tx, ty);
                if (combat_path_len > 0 && combat_path_len < best_len) {
                    best_x = tx;
                    best_y = ty;
                    best_len = combat_path_len;
                }
            }
        }
        if (best_len < 999)
            AI_Move_Path(unit, best_x, best_y, -1, best_x, best_y);
    }
    AI_Move_Path(unit, x, y, target, rally_x, rally_y);
}

/* (unit, x, y, target or -1, rally x, y) -> 0 without a path, else 1: walks the path (same blocked cost map) inside
   a box from the rally point toward the target's side (the whole field when the rally point is the target), step
   by step while the moves last, animating visible units (teleporters and mergers jump straight to the end); on
   reaching the target it attacks (a unit with no figures left loses its moves); a defender holding a city with walls only
   (no Wall of Fire / Darkness: SIEGE_CITY_WALLS exactly) does not step out of the city core. Afterwards the unit faces along its last step and pays the path's cost
   (teleport / merge: 2). A move by any side but player 0 sets combat_turns_since_ai_acted to -1. BUG #454: without a target (-1) the attack test
   reads p_battle_units[-1]
   Numbered: BUG #295, #347, #348 (BUGS_AND_QUIRKS.txt) */
int AI_Move_Path(int unit, int x, int y, int target, int rally_x, int rally_y)
{
    int i;
    int dir_x;
    int dir_y;
    int prev_x;
    int prev_y;
    int last_x;
    int last_y;
    int attack;
    int first;
    int cur_x;
    int cur_y;
    int box_x1;
    int box_y1;
    int dist_x;
    int dist_y;
    int visible;
    int sound;
    int start;
    int anim_inc;
    int step;

    if (combat_auto == 1)
        anim_inc = 2;
    else
        anim_inc = 1;
    Combat_Build_Move_Costs(unit);
    for (i = 0; i < battle_unit_count; i++) {
        if (i != target && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            p_combat_move_cost[p_battle_units[i].combat_y * BATTLEFIELD_W + p_battle_units[i].combat_x] = COST_IMPASSABLE;
    }
    Combat_Mark_Wall_Obstacles(unit);
    if (p_battle_units[unit].owner == combat_defender_player && combat_defender_holds_city == 1 &&
        (p_battlefield->wall_of_fire > 0 || p_battlefield->wall_of_darkness > 0) && In_City_Area(unit) == 1)
        Combat_Block_City_Ring();
    for (i = 0; i < vortex_count; i++)
        p_combat_move_cost[p_vortices[i].y * BATTLEFIELD_W + p_vortices[i].x] = COST_IMPASSABLE;
    Combat_Find_Path(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y, x, y);
    if (combat_path_len == 0)
        return 0;
    if (p_battle_units[unit].owner != PLAYER_HUMAN)
        combat_turns_since_ai_acted = -1;
    prev_x = p_battle_units[unit].combat_x;
    prev_y = p_battle_units[unit].combat_y;
    p_battle_units[unit].dest_x = x;
    p_battle_units[unit].dest_y = y;
    combat_move_write_only = 0;
    p_battle_units[unit].moving = 1;
    cur_x = p_battle_units[unit].combat_x;
    cur_y = p_battle_units[unit].combat_y;
    if (target > -1) {
        x = p_battle_units[target].combat_x;
        y = p_battle_units[target].combat_y;
    }
    if (rally_x == x && rally_y == y) {         /* from here rally_x / rally_y are the box's far corner */
        box_x1 = 0;
        rally_x = BATTLEFIELD_W;
        box_y1 = 0;
        rally_y = BATTLEFIELD_H;
    } else {
        dist_x = abs(cur_x - x);
        dist_y = abs(cur_y - y);
        if (dist_x < dist_y) {
            box_x1 = 0;
            rally_x = BATTLEFIELD_W;
            if (cur_y < y)
                box_y1 = 0;
            else {
                box_y1 = rally_y;
                rally_y = BATTLEFIELD_H;
            }
        }
        if (dist_x > dist_y) {
            box_y1 = 0;
            rally_y = BATTLEFIELD_H;
            if (cur_x < x)
                box_x1 = 0;
            else {
                box_x1 = rally_x;
                rally_x = BATTLEFIELD_W;
            }
        }
        if (dist_x == dist_y) {
            if (cur_x < x)
                box_x1 = 0;
            else {
                box_x1 = rally_x;
                rally_x = BATTLEFIELD_W;
            }
            if (cur_y < y)
                box_y1 = 0;
            else {
                box_y1 = rally_y;
                rally_y = BATTLEFIELD_H;
            }
        }
    }
    first = 1;
    if (((p_battle_units[unit].combat_enchantments & UNIT_ENCH_INVISIBILITY) != 0 ||
         (p_battle_units[unit].item_enchantments & UNIT_ENCH_INVISIBILITY) != 0 || (p_battle_units[unit].ability_flags & AFLAG_INVISIBILITY) != 0 ||
         (p_units[p_battle_units[unit].unit_index].enchantments & UNIT_ENCH_INVISIBILITY) != 0) && p_battle_units[unit].owner != PLAYER_HUMAN) {
        if ((p_battle_units[unit].owner == combat_attacker_player || combat_attacker_sees_invis == 1) &&
            (p_battle_units[unit].owner == combat_defender_player || combat_defender_sees_invis == 1))
            visible = 1;
        else
            visible = 0;
    } else
        visible = 1;
    start = 0;
    if ((p_battle_units[unit].move_flags & MOVE_TELEPORTING) || (p_battle_units[unit].move_flags & MOVE_MERGING)) {
        if (target > -1 && p_combat_path_x[combat_path_len - 1] == p_battle_units[target].combat_x &&
            p_combat_path_y[combat_path_len - 1] == p_battle_units[target].combat_y) {
            if (combat_path_len > 1)
                start = combat_path_len - 2;
        } else
            start = combat_path_len - 1;
    }
    for (step = start; step < combat_path_len && (first == 1 || p_battle_units[unit].moves -
                        p_combat_path_dist[p_combat_path_y[step - 1] * BATTLEFIELD_W + p_combat_path_x[step - 1]] > 0); step++) {
        if ((p_combat_path_x[step] > rally_x || p_combat_path_x[step] < box_x1 || p_combat_path_y[step] > rally_y ||
             p_combat_path_y[step] < box_y1) && (p_combat_path_x[step] != x || p_combat_path_y[step] != y))
            break;
        first = 0;
        p_battle_units[unit].dest_x = p_combat_path_x[step];
        p_battle_units[unit].dest_y = p_combat_path_y[step];
        attack = 0;
        if (p_battle_units[unit].dest_x == p_battle_units[target].combat_x &&
            p_battle_units[unit].dest_y == p_battle_units[target].combat_y && p_battle_units[target].status == BATTLE_UNIT_ACTIVE)
            attack = 1;
        if (attack == 1) {
            if (p_battle_units[unit].figures_alive <= 0)
                p_battle_units[unit].moves = -2;   /* no figures left: moves spent */
            else
                Combat_Unit_Attack(unit, target, 0, 0);
            step--;
        } else {
            if (p_battle_units[unit].owner == combat_defender_player && combat_siege_flags == SIEGE_CITY_WALLS &&
                (p_combat_path_x[step] < BF_CITY_X1 || p_combat_path_x[step] > BF_CITY_X2 || p_combat_path_y[step] < BF_CITY_Y1 ||
                 p_combat_path_y[step] > BF_CITY_Y2) && In_City_Area(unit) == 1)
                break;
            if (visible == 1) {
                if ((p_battle_units[unit].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[unit].move_flags & MOVE_MERGING) == 0) {
                    if (game_config.sound_effects == 1) {
                        Sound_Play(combat_silence_sfx);
                        Heap_Mark(p_world_block);
                        sound = Unit_Load_Move_Sound(unit);
                        Heap_Release(p_world_block);
                    } else
                        sound = -1;
                    p_battle_units[unit].move_anim_step = 1;
                    if (sound != -1)
                        Sound_Play(sound);
                    if (game_config.movement_animations == 1) {
                        for (i = 0; i < 8; i += anim_inc) {
                            p_battle_units[unit].move_anim_step = p_battle_units[unit].move_anim_step + anim_inc;
                            Combat_Draw_Screen();
                            Screen_Present();
                        }
                    } else {
                        p_battle_units[unit].move_anim_step = 7;
                        Combat_Draw_Screen();
                        Screen_Present();
                    }
                    if (game_config.sound_effects == 1)
                        Sound_Play(combat_silence_sfx);
                } else if (p_battle_units[unit].move_flags & MOVE_TELEPORTING)
                    Combat_Teleport_Anim(unit, p_combat_path_x[step], p_combat_path_y[step]);
                else
                    Combat_Merge_Anim(unit, p_combat_path_x[step], p_combat_path_y[step]);
            }
            p_battle_units[unit].move_anim_step = 0;
            p_battle_units[unit].combat_x = p_combat_path_x[step];
            p_battle_units[unit].combat_y = p_combat_path_y[step];
        }
    }
    p_battle_units[unit].move_anim_step = 0;
    p_battle_units[unit].moving = 0;
    if (step > 1) {
        prev_x = p_combat_path_x[step - 2];
        prev_y = p_combat_path_y[step - 2];
        last_x = p_combat_path_x[step - 1];
        last_y = p_combat_path_y[step - 1];
    } else {
        last_x = x;
        last_y = y;
    }
    dir_x = last_x - prev_x;
    dir_y = last_y - prev_y;
    if (step != 0) {
        p_battle_units[unit].combat_x = p_combat_path_x[step - 1];
        p_battle_units[unit].combat_y = p_combat_path_y[step - 1];
        p_battle_units[unit].dest_x = last_x + dir_x;
        p_battle_units[unit].dest_y = last_y + dir_y;
    }
    if ((p_battle_units[unit].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[unit].move_flags & MOVE_MERGING) == 0)
        p_battle_units[unit].moves = p_battle_units[unit].moves -
                                     p_combat_path_dist[p_battle_units[unit].combat_y * BATTLEFIELD_W +
                                                        p_battle_units[unit].combat_x];
    else
        p_battle_units[unit].moves = p_battle_units[unit].moves - 2;
    return 1;
}
