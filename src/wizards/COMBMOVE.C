/* COMBMOVE.C - WIZARDS.EXE overlay code segment 7719: tactical combat turns and movement - a battle unit's figure
   frames, the start of each combat turn (counters, Call Lightning / Wrack / vortices / auras, then every living unit's
   stats and moves recomputed with Terror, Entangle (-2 moves), Web, Black Sleep, Confusion and Regeneration), the movement
   cost grid of a unit, walking a path square by square (or teleporting / merging), the active unit, the left-click
   order (attack or move), the click map of reachable / attackable squares, the visibility of an invisible unit to
   its enemy side, the city damage tally and the teleport / merge movement types.
   This module has no data or BSS of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7719 src/wizards/COMBMOVE.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Battle_Unit_Load_Figure(int bu);
void Combat_Turn_Start(void);
void Combat_Build_Move_Costs(int bu);
int Combat_Move_Unit(int bu, int x, int y);
void Combat_Set_Active_Unit(int bu);
void Combat_Unit_Order(int bu, int gx, int gy);
void Combat_Build_Click_Map(void);
int Combat_Unit_Visible_To_Enemy(int bu);
int City_Damage_Tally_Turn(void);
int Unit_Special_Move_Type(int bu);

#include "GAME.H"

extern int combat_move_write_only;              /* DS:7082  zeroed before every move (and in AI_Move_Path), never read */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_site;                      /* DS:9284  CombatAt: COMBAT_AT_UNITS / _CITY / _LAIR */
extern int combat_all_units_immobile;                     /* DS:c406  (see COMBAT.C) */
extern int city_damage_counter;                 /* DS:c418 */
extern unsigned combat_silence_sfx;             /* DS:c41a */
extern int combat_defender_sees_invis;          /* DS:c41e */
extern int combat_attacker_sees_invis;                /* DS:c420 */
extern int combat_ai_moved;                     /* DS:c430 */
extern int combat_wizard_can_cast;              /* DS:c438 */
extern int combat_turn_count;                   /* DS:c51c */
extern int combat_battlefield_kind;                /* DS:c520 */
extern char *combat_click_map[];                /* DS:c524  per grid row: a battle unit, CLICK_REACHABLE, CLICK_BLOCKED, CLICK_WALL */
extern int combat_active_unit;                  /* DS:c582 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int combat_hover_unit;                   /* DS:c58a */
extern int far *p_aura_resistance;              /* DS:c89a */
extern int combat_turns_since_ai_acted;                   /* DS:c8b0 */
extern int *p_combat_path_y;                    /* DS:d13e */
extern int *p_combat_path_x;                    /* DS:d140 */
extern int combat_path_len;                     /* DS:d142 */
extern int *p_combat_path_prev;                  /* DS:d144 */
extern unsigned char *p_combat_path_dist;       /* DS:d146 */
extern unsigned char *p_combat_move_cost;       /* DS:d148 */
extern struct CombatVortex far *p_vortices;     /* DS:d14e */
extern int vortex_count;                        /* DS:d152 */

int Load_Unit_Figure_Frames(int unit_type, int slot);
void Mana_Leak_Turn(void);
void Call_Lightning_Turn(int caster_player);
void Wrack_Turn(int caster_player);
void Vortices_Turn(void);
void Combat_Start_Auras(int battlefield_kind);
void Unit_Apply_Modifiers(struct BattleUnit far *bu);
void Battle_Unit_Combat_Modifiers(struct BattleUnit far *bu);
int Unit_Combat_Moves(int bu);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
int Random(int n);
void Battle_Unit_Heal(int bu, int amount, int overflow);
int Unit_Combat_Move_Type(int bu);
int Mem_Copy_Far();                                /* unprototyped (see HELPSCR.C) */
void Combat_Mark_Wall_Obstacles(int unit);
int Combat_Find_Path(int fx, int fy, int tx, int ty);
unsigned Unit_Play_Move_Sound(int unit);
int Sound_Play(unsigned sound);
void Combat_Draw_Screen(void);
void Screen_Present(void);
void Wall_Of_Fire_Crossing(int unit);
void Combat_Teleport_Anim(int unit, int x, int y);
void Combat_Merge_Anim(int unit, int x, int y);
int abs(int x);
int Combat_Unit_Attack(int attacker, int target, int x, int y);
int Can_Attack_Target(int unit, int target);
int Melee_Wall_Allowed(int attacker, int target);
int Unit_Move_Points(int unit);                  /* int here (no mov ah,0 after the call); unsigned char in other sources */
int Is_City_Wall_Square(int x, int y);
void Combat_Calc_Reach(int x, int y, int moves);
int Combat_Distance(int bu1, int bu2);
int In_City_Area(int bu);

/* (bu) its figure frames loaded for its unit type */
void Battle_Unit_Load_Figure(int bu)
{
    p_battle_units[bu].gfx_slot = Load_Unit_Figure_Frames(p_units[p_battle_units[bu].unit_index].type, bu);
}

/* () the start of a combat turn: the counters, Mana Leak, Call Lightning / Wrack per side, the vortices (the active
   unit kept), the auras and the city damage tally; then each living unit: modifiers and moves recomputed, Terror
   (resist or no moves), Entangle (CENCH_ENTANGLE_*, -2 moves), Web (broken by its strength or no moves), Black Sleep (COMBAT_FLAG_BLACK_SLEEP),
   Confusion (Random(4): 1 no moves, 2 random move, 3 changes sides) and Regeneration
   Numbered: BUG #15, #246, #257, #286, #349 (BUGS_AND_QUIRKS.txt) */
void Combat_Turn_Start(void)
{
    int r;
    int saved_active;
    register int bu;

    combat_all_units_immobile = 1;
    combat_turns_since_ai_acted++;
    combat_turn_count++;
    combat_wizard_can_cast = 1;
    combat_hover_unit = -1;
    combat_ai_moved = 0;
    Mana_Leak_Turn();
    if (p_combat_enchants[CENCH_CALL_LIGHTNING_ATT] != 0)
        Call_Lightning_Turn(combat_attacker_player);
    if (p_combat_enchants[CENCH_CALL_LIGHTNING_DEF] != 0)
        Call_Lightning_Turn(combat_defender_player);
    if (p_combat_enchants[CENCH_WRACK_ATT] != 0)
        Wrack_Turn(combat_attacker_player);
    if (p_combat_enchants[CENCH_WRACK_DEF] != 0)
        Wrack_Turn(combat_defender_player);
    saved_active = combat_active_unit;
    Vortices_Turn();
    combat_active_unit = saved_active;
    Combat_Start_Auras(combat_battlefield_kind);
    City_Damage_Tally_Turn();
    for (bu = 0; bu < battle_unit_count; bu++) {
        p_battle_units[bu].times_attacked = 0;
        if (p_battle_units[bu].status == BATTLE_UNIT_ACTIVE) {
            Unit_Apply_Modifiers(&p_battle_units[bu]);
            Battle_Unit_Combat_Modifiers(&p_battle_units[bu]);
            p_battle_units[bu].moves = Unit_Combat_Moves(bu);
            p_battle_units[bu].action_state = ACTION_NONE;
            if ((p_combat_enchants[CENCH_TERROR_ATT] != 0 && p_battle_units[bu].owner == combat_defender_player) ||
                (p_combat_enchants[CENCH_TERROR_DEF] != 0 && p_battle_units[bu].owner == combat_attacker_player)) {
                r = Roll_Resist(p_battle_units[bu], p_aura_resistance[p_battle_units[bu].owner] + 1,   /* + 1: Terror's
                                                   modifier (Spell_Resist_Modifier) */
                                p_spell_data[SPELL_TERROR].realm);
                if (r > 0) {
                    p_battle_units[bu].action_state = ACTION_DONE;
                    p_battle_units[bu].moves = 0;
                }
            }
            if ((p_combat_enchants[CENCH_ENTANGLE_ATT] != 0 && p_battle_units[bu].owner == combat_defender_player) ||
                (p_combat_enchants[CENCH_ENTANGLE_DEF] != 0 && p_battle_units[bu].owner == combat_attacker_player)) {
                if (p_battle_units[bu].moves > 0)
                    p_battle_units[bu].moves = p_battle_units[bu].moves - 2;
                if (p_battle_units[bu].moves < 1)
                    p_battle_units[bu].moves = 0;
            }
            if (p_battle_units[bu].web_hp > 0) {
                if (p_battle_units[bu].ranged_type / 10 == RANGED_CLASS_MISSILE)
                    r = p_battle_units[bu].melee > p_battle_units[bu].ranged ? p_battle_units[bu].melee :
                        p_battle_units[bu].ranged;
                else if (p_battle_units[bu].ranged_type > RANGED_THROWN)
                    r = p_battle_units[bu].melee + p_battle_units[bu].ranged;
                else
                    r = p_battle_units[bu].melee;
                p_battle_units[bu].web_hp = p_battle_units[bu].web_hp - r;
                if (p_battle_units[bu].web_hp <= 0)
                    p_battle_units[bu].web_hp = 0;
                else {
                    p_battle_units[bu].action_state = ACTION_DONE;
                    p_battle_units[bu].moves = 0;
                }
            }
            if (p_battle_units[bu].combat_flags & COMBAT_FLAG_BLACK_SLEEP) {
                p_battle_units[bu].action_state = ACTION_DONE;
                p_battle_units[bu].moves = 0;
            }
            p_battle_units[bu].confusion_result = CONFUSION_NONE;
            if (p_battle_units[bu].combat_flags & COMBAT_FLAG_CONFUSION) {
                r = Random(4);
                if (r == 1) {
                    p_battle_units[bu].action_state = ACTION_DONE;
                    p_battle_units[bu].moves = 0;
                }
                if (r == 2)
                    p_battle_units[bu].confusion_result = CONFUSION_RANDOM_MOVE;
                if (r == 3) {
                    p_battle_units[bu].confusion_result = CONFUSION_CHANGED_SIDE;
                    if (p_battle_units[bu].owner == combat_attacker_player)
                        p_battle_units[bu].owner = combat_defender_player;
                    else
                        p_battle_units[bu].owner = combat_attacker_player;
                }
            }
            if (((p_battle_units[bu].combat_enchantments & UNIT_ENCH_REGENERATION) || (p_battle_units[bu].item_enchantments & UNIT_ENCH_REGENERATION) ||
                 (p_units[p_battle_units[bu].unit_index].enchantments & UNIT_ENCH_REGENERATION) ||
                 (p_battle_units[bu].ability_flags & AFLAG_REGENERATION)) &&
                p_battle_units[bu].status == BATTLE_UNIT_ACTIVE)
                Battle_Unit_Heal(bu, 1, 0);
        }
    }
}

/* (bu: a battle unit) p_combat_move_cost from the battlefield's grid for the unit's movement type (0 / 1 walking, 2 / 6 / 7
   flying, 3, 4 sailing, 5 the cheaper of walking and sailing) */
void Combat_Build_Move_Costs(int bu)
{
    int move_type;
    register int i;
    register int j;

    move_type = Unit_Combat_Move_Type(bu);
    i = Unit_Special_Move_Type(bu);
    if (i > 0)
        move_type = i;
    switch (move_type) {
    case COMBAT_MOVE_NONE:
    case COMBAT_MOVE_WALK:
        Mem_Copy_Far((unsigned char far *)p_combat_move_cost, p_battlefield->cost_ground, BATTLEFIELD_CELLS);
        break;
    case COMBAT_MOVE_FLY:
        Mem_Copy_Far((unsigned char far *)p_combat_move_cost, p_battlefield->cost_flying, BATTLEFIELD_CELLS);
        break;
    case COMBAT_MOVE_TYPE_3:
        Mem_Copy_Far((unsigned char far *)p_combat_move_cost, p_battlefield->cost_mode3, BATTLEFIELD_CELLS);
        break;
    case COMBAT_MOVE_SAIL:
        Mem_Copy_Far((unsigned char far *)p_combat_move_cost, p_battlefield->cost_sailing, BATTLEFIELD_CELLS);
        break;
    case COMBAT_MOVE_SWIM:
        for (i = 0; i < BATTLEFIELD_H; i++)
            for (j = 0; j < BATTLEFIELD_W; j++)
                p_combat_move_cost[i * BATTLEFIELD_W + j] = p_battlefield->cost_sailing[i][j] >
                    p_battlefield->cost_ground[i][j] ? p_battlefield->cost_ground[i][j] :
                    p_battlefield->cost_sailing[i][j];
        break;
    case COMBAT_MOVE_TELEPORT:
    case COMBAT_MOVE_MERGE:
        Mem_Copy_Far((unsigned char far *)p_combat_move_cost, p_battlefield->cost_flying, BATTLEFIELD_CELLS);
        break;
    }
}

/* (bu: a battle unit, x, y) -> 0 when no path: the move costs (occupied squares, vortices, the backdrop square, walls) and the
   path, then the walk square by square (sound, animation, Wall of Fire; stopped when the unit dies) or the teleport /
   merge; the unit faces on along its last step; moves - the path cost (teleport / merge: 2)
   Numbered: BUG #295 (BUGS_AND_QUIRKS.txt) */
int Combat_Move_Unit(int bu, int x, int y)
{
    int step;
    int ddx;
    int ddy;
    char owner;
    int prev_x;
    int prev_y;
    int last_x;
    int last_y;
    int special;
    int teleport;
    unsigned sound;
    int k;

    owner = p_battle_units[bu].owner;
    Combat_Build_Move_Costs(bu);
    for (k = 0; k < battle_unit_count; k++)
        if (p_battle_units[k].status == BATTLE_UNIT_ACTIVE)
            p_combat_move_cost[p_battle_units[k].combat_y * BATTLEFIELD_W + p_battle_units[k].combat_x] = COST_IMPASSABLE;
    for (k = 0; k < vortex_count; k++)
        p_combat_move_cost[p_vortices[k].y * BATTLEFIELD_W + p_vortices[k].x] = COST_IMPASSABLE;
    if (p_battlefield->backdrop != BACKDROP_FIELD)
        p_combat_move_cost[BATTLEFIELD_CELL(BF_FEATURE_X, BF_FEATURE_Y)] = COST_IMPASSABLE;
    if (p_battlefield->has_city_walls == 1 || p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] > 0)
        Combat_Mark_Wall_Obstacles(bu);
    teleport = 0;
    special = Unit_Special_Move_Type(bu);
    if (special == COMBAT_MOVE_TELEPORT || special == COMBAT_MOVE_MERGE) {
        combat_path_len = 0;
        teleport = 1;
    } else {
        Combat_Find_Path(p_battle_units[bu].combat_x, p_battle_units[bu].combat_y, x, y);
        if (combat_path_len == 0)
            return 0;
    }
    prev_x = p_battle_units[bu].combat_x;
    prev_y = p_battle_units[bu].combat_y;
    combat_move_write_only = 0;
    p_battle_units[bu].moving = 1;
    sound = Unit_Play_Move_Sound(bu);
    if (teleport == 0) {
        p_battle_units[bu].dest_x = x;
        p_battle_units[bu].dest_y = y;
        for (step = 0; step < combat_path_len; step++) {
            p_battle_units[bu].dest_x = p_combat_path_x[step];
            p_battle_units[bu].dest_y = p_combat_path_y[step];
            if (game_config.sound_effects == 1)
                Sound_Play(sound);
            if (game_config.movement_animations == 1) {
                for (k = 0; k < 8; k++) {             /* 8 animation steps a square (0 .. 7; 7 without animations) */
                    p_battle_units[bu].move_anim_step = p_battle_units[bu].move_anim_step + 1;
                    Combat_Draw_Screen();
                    Screen_Present();
                }
            } else {
                p_battle_units[bu].move_anim_step = 7;
                Combat_Draw_Screen();
                Screen_Present();
            }
            if (game_config.sound_effects == 1)
                Sound_Play(combat_silence_sfx);
            Wall_Of_Fire_Crossing(bu);
            if (p_battle_units[bu].status > 0)  /* no longer BATTLE_UNIT_ACTIVE: it died on the way */
                break;
            p_battle_units[bu].move_anim_step = 0;
            p_battle_units[bu].combat_x = p_combat_path_x[step];
            p_battle_units[bu].combat_y = p_combat_path_y[step];
        }
    } else {
        if (special == COMBAT_MOVE_TELEPORT)
            Combat_Teleport_Anim(bu, x, y);
        else
            Combat_Merge_Anim(bu, x, y);
    }
    p_battle_units[bu].move_anim_step = 0;
    p_battle_units[bu].moving = 0;
    if (combat_path_len > 1) {
        prev_x = p_combat_path_x[combat_path_len - 2];
        prev_y = p_combat_path_y[combat_path_len - 2];
        last_x = p_combat_path_x[combat_path_len - 1];
        last_y = p_combat_path_y[combat_path_len - 1];
    } else {
        last_x = x;
        last_y = y;
    }
    ddx = last_x - prev_x;
    ddy = last_y - prev_y;
    if (teleport == 0) {
        p_battle_units[bu].combat_x = p_battle_units[bu].dest_x;
        p_battle_units[bu].combat_y = p_battle_units[bu].dest_y;
        p_battle_units[bu].dest_x = last_x + ddx;
        p_battle_units[bu].dest_y = last_y + ddy;
        p_battle_units[bu].moves = p_battle_units[bu].moves -
            p_combat_path_dist[p_battle_units[bu].combat_y * BATTLEFIELD_W + p_battle_units[bu].combat_x];
    } else {
        if (p_battle_units[bu].moves > 2)
            p_battle_units[bu].moves = p_battle_units[bu].moves - 2;
        else
            p_battle_units[bu].moves = 0;
    }
    if (p_battle_units[bu].moves < 0)
        p_battle_units[bu].moves = 0;
}

/* (bu) the old active unit stops moving; bu becomes the active unit */
void Combat_Set_Active_Unit(int bu)
{
    p_battle_units[combat_active_unit].moving = 0;
    combat_active_unit = bu;
}

/* (bu, gx, gy) a left click on the battlefield: an enemy unit or an attackable wall square (CLICK_WALL) is attacked -
   in melee when adjacent (a wall must allow it) or by a shooter / caster (ranged type 1x..3x); an empty reachable
   square (CLICK_REACHABLE) is a move */
void Combat_Unit_Order(int bu, int gx, int gy)
{
    int adx;
    int ady;
    int unused;
    int rt;
    int tx;
    int ty;
    int clicked;

    clicked = combat_click_map[gy][gx];
    if (clicked == CLICK_WALL || (clicked >= 0 && p_battle_units[clicked].owner != p_battle_units[bu].owner && p_battle_units[clicked].status == BATTLE_UNIT_ACTIVE)) {
        if (clicked == CLICK_WALL) {
            tx = gx;
            ty = gy;
        } else {
            tx = p_battle_units[clicked].combat_x;
            ty = p_battle_units[clicked].combat_y;
        }
        adx = abs(tx - p_battle_units[bu].combat_x);
        ady = abs(ty - p_battle_units[bu].combat_y);
        unused = -2;
        if (p_battle_units[bu].moves > 0) {
            if (clicked == CLICK_WALL) {
                if (adx <= 1 && ady <= 1)
                    Combat_Unit_Attack(bu, clicked, tx, ty);
                else {
                    rt = p_battle_units[bu].ranged_type / 10;
                    if (rt == RANGED_CLASS_BOULDER || rt == RANGED_CLASS_MISSILE || rt == RANGED_CLASS_MAGIC)
                        Combat_Unit_Attack(bu, clicked, tx, ty);
                }
            } else if (Can_Attack_Target(bu, clicked) == 1) {
                if (adx <= 1 && ady <= 1) {
                    if (Melee_Wall_Allowed(bu, clicked) == 1)
                        Combat_Unit_Attack(bu, clicked, tx, ty);
                    else {
                        rt = p_battle_units[bu].ranged_type / 10;
                        if (rt == RANGED_CLASS_BOULDER || rt == RANGED_CLASS_MISSILE || rt == RANGED_CLASS_MAGIC)
                            Combat_Unit_Attack(bu, clicked, tx, ty);
                    }
                } else {
                    rt = p_battle_units[bu].ranged_type / 10;
                    if (rt == RANGED_CLASS_BOULDER || rt == RANGED_CLASS_MISSILE || rt == RANGED_CLASS_MAGIC)
                        Combat_Unit_Attack(bu, clicked, tx, ty);
                }
            }
        }
    } else if (clicked == CLICK_REACHABLE && p_battle_units[bu].moves > 0)
        Combat_Move_Unit(bu, gx, gy);
}

/* () combat_click_map for the active unit: CLICK_BLOCKED unreachable, CLICK_REACHABLE reachable (Combat_Calc_Reach
   over its moves, 20 for teleport / merge), CLICK_WALL an attackable wall square (attacker with wall crushing), else the battle unit there */
void Combat_Build_Click_Map(void)
{
    int x;
    int row;
    int ucx;
    int ucy;
    int count;
    int mp;                                     /* computed, never used */
    int moves;
    int special;
    int teleport;
    register int y;
    register int reach;

    Combat_Build_Move_Costs(combat_active_unit);
    for (x = 0; x < battle_unit_count; x++)
        if (p_battle_units[x].status == BATTLE_UNIT_ACTIVE)
            p_combat_move_cost[p_battle_units[x].combat_y * BATTLEFIELD_W + p_battle_units[x].combat_x] = COST_IMPASSABLE;
    for (x = 0; x < vortex_count; x++)
        p_combat_move_cost[p_vortices[x].y * BATTLEFIELD_W + p_vortices[x].x] = COST_IMPASSABLE;
    if (p_battlefield->backdrop != BACKDROP_FIELD)
        p_combat_move_cost[BATTLEFIELD_CELL(BF_FEATURE_X, BF_FEATURE_Y)] = COST_IMPASSABLE;
    if (p_battlefield->has_city_walls == 1 || p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] > 0)
        Combat_Mark_Wall_Obstacles(combat_active_unit);
    teleport = 0;
    special = Unit_Special_Move_Type(combat_active_unit);
    if (special == COMBAT_MOVE_TELEPORT || special == COMBAT_MOVE_MERGE)
        teleport = 1;
    moves = p_battle_units[combat_active_unit].moves;
    mp = Unit_Move_Points(p_battle_units[combat_active_unit].unit_index);
    ucx = p_battle_units[combat_active_unit].combat_x;
    ucy = p_battle_units[combat_active_unit].combat_y;
    for (y = 0; y < BATTLEFIELD_H; y++)
        for (x = 0; x < BATTLEFIELD_W; x++)
            combat_click_map[y][x] = CLICK_BLOCKED;
    if (p_battlefield->has_city_walls == 1 && p_battle_units[combat_active_unit].owner == combat_attacker_player &&
        (p_battle_units[combat_active_unit].ability_flags & AFLAG_WALL_CRUSHER)) {
        for (y = 0; y < BATTLEFIELD_H; y++)
            for (x = 0; x < BATTLEFIELD_W; x++)
                if (Is_City_Wall_Square(x, y) == 1)
                    combat_click_map[y][x] = CLICK_WALL;
    }
    for (x = 0; x < battle_unit_count; x++)
        if (p_battle_units[x].status == BATTLE_UNIT_ACTIVE && p_battle_units[x].draw_mode != DRAW_MODE_HIDDEN)
            combat_click_map[p_battle_units[x].combat_y][p_battle_units[x].combat_x] = x;
    reach = 0;
    if (teleport == 0)
        reach = p_battle_units[combat_active_unit].moves;
    else if (p_battle_units[combat_active_unit].moves > 0)
        reach = 20;
    count = 0;
    Combat_Calc_Reach(p_battle_units[combat_active_unit].combat_x, p_battle_units[combat_active_unit].combat_y, reach);
    for (y = 0; y < BATTLEFIELD_H; y++) {
        row = y * BATTLEFIELD_W;
        for (x = 0; x < BATTLEFIELD_W; x++) {
            if (p_combat_path_prev[row + x] == 1) {   /* after Combat_Calc_Reach: 1 = reachable */
                combat_click_map[y][x] = CLICK_REACHABLE;
                count++;
            }
        }
    }
    combat_click_map[p_battle_units[combat_active_unit].combat_y][p_battle_units[combat_active_unit].combat_x] =
        combat_active_unit;
}

/* (bu) -> 1 when the unit can be seen: not invisible, or its enemy side sees invisible, or an enemy unit is adjacent.
   Invisibility is read from the combat and unit enchantments and AFLAG_INVISIBILITY, not item_enchantments: an
   item's Invisibility does not hide the unit here (#198; COMMUNITY-KNOWN IW045); clarified
   using ReMoM repo (Target_Is_Visible) */
int Combat_Unit_Visible_To_Enemy(int bu)
{
    unsigned long ench;
    int owner;
    int dist;
    register int other;
    register int visible;

    owner = p_battle_units[bu].owner;
    ench = p_battle_units[bu].combat_enchantments | p_units[p_battle_units[bu].unit_index].enchantments;
    visible = 1;
    if ((ench & UNIT_ENCH_INVISIBILITY) || (p_battle_units[bu].ability_flags & AFLAG_INVISIBILITY)) {
        visible = 0;
        if (owner == combat_defender_player && combat_attacker_sees_invis == 1)
            visible = 1;
        if (owner == combat_attacker_player && combat_defender_sees_invis == 1)
            visible = 1;
        if (visible == 0) {
            for (other = 0; other < battle_unit_count; other++) {
                if (p_battle_units[other].status == BATTLE_UNIT_ACTIVE && p_battle_units[other].owner != owner) {
                    dist = Combat_Distance(bu, other);
                    if (dist < 2)
                        visible = 1;
                }
            }
        }
    }
    return visible;
}

/* () in a city battle, +1 to the city damage counter per living attacking unit inside the city area; returns 0
   only on the early exit, no value otherwise */
int City_Damage_Tally_Turn(void)
{
    register int bu;

    if (combat_site != COMBAT_AT_CITY)
        return 0;
    for (bu = 0; bu < battle_unit_count; bu++)
        if (p_battle_units[bu].status == BATTLE_UNIT_ACTIVE && p_battle_units[bu].owner == combat_attacker_player &&
            In_City_Area(bu) == 1)
            city_damage_counter++;
}

/* (unit) -> 6 teleporting, 7 merging, else 0 */
int Unit_Special_Move_Type(int bu)
{
    if (p_battle_units[bu].move_flags & MOVE_TELEPORTING)
        return COMBAT_MOVE_TELEPORT;
    if (p_battle_units[bu].move_flags & MOVE_MERGING)
        return COMBAT_MOVE_MERGE;
    return COMBAT_MOVE_NONE;
}
