/* COMBUTIL.C - WIZARDS.EXE overlay code segment a76b (stub 2587): combat helpers - a side's missile strength, the city walls,
   Wall of Fire and the city core as movement obstacles, the battlefield kind, the water-battle sideline, the
   battle odds, unit / spell sound loading, Wall of Fire damage, the hit animation, the city-area and wall-square
   tests, the combat-only summons, a unit's combat moves, a city taken without a battle (fame, plunder, razing),
   the AI's flee and raze decisions and the raze question dialog.
   This module's data (DS:6066): its literals ("" is the tail of "soundfx": -d); no BSS of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS a76b src/wizards/COMBUTIL.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order, which is not the
   order of the definitions) */
int Combat_Terrain_Kind(int x, int y, int plane, int in_city);
int Water_Battle_Sideline(int player);
int Battle_Odds(int player);
int Unit_Load_Move_Sound(int bu);
int Unit_Load_Attack_Sound(int bu);
void Combat_Load_Spell_Sound(int spell);
int Unit_Combat_Moves(int bu);
void Wall_Of_Fire_Crossing(int unit);
void Battle_Unit_Set_Hit_Anim(int unit, int damage);
void Battle_Unit_Clear_Hit_Anim(int bu);
int In_City_Area(int bu);
int Is_City_Wall_Square(int x, int y);
int Is_Combat_Only_Summon(int bu);
void City_Captured(int n, int *units);
int Side_Missile_Strength(int player);
void Combat_Mark_Wall_Obstacles(int unit);
void Confirm_Dialog_Redraw(void);
void Combat_Block_City_Ring(void);
int Confirm_Dialog(char *text);
int AI_Decide_Raze(int player, int city);
int AI_Should_Flee(int player);

#include "GAME.H"

extern char notify_shadow_colors[5];            /* DS:6f78 */
extern char confirm_text_colors[5];             /* DS:6f7d */
extern int confirm_box_dx;                      /* DS:6f96 */
extern int confirm_box_dy;                      /* DS:6f98 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern struct Item far *p_items;                /* DS:9136 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286  the city / lair attacked */
extern struct Building far *p_building_data;    /* DS:938c */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern unsigned p_world_block;                /* DS:9d20 */
extern struct GameConfig game_config;           /* DS:bd9e */
extern unsigned combat_silence_sfx;             /* DS:c41a */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int combat_turn_count;                   /* DS:c51c */
extern int combat_active_unit;                  /* DS:c582 */
extern int battle_unit_count;                   /* DS:c588 */
extern int list_count;                          /* DS:c926  here the human's fame change (as in COMBEND.C) */
#define combat_fame_change list_count
extern int city_razed;                          /* DS:c974 */
extern int combat_end_value;                    /* DS:c976  by turns: the fame gained, the plunder, the total gold */
extern int combat_bldgs_lost_count;             /* DS:c978 */
extern int combat_pop_lost;                     /* DS:c97c */
extern int combat_result_kind;                  /* DS:c97e */
extern unsigned confirm_left_button_pic;          /* DS:cacc  COMPIX 82: the left button, hotkey "R" (raze) */
extern unsigned confirm_right_button_pic;         /* DS:cace  COMPIX 81: the right button, hotkey "N" (no) */
#define raze_button_pic confirm_left_button_pic    /* the confirm box's No / Yes pictures, reloaded here with */
#define keep_button_pic confirm_right_button_pic   /* COMPIX's Raze (left, "R") and Keep (right, "N") buttons */
extern unsigned confirm_box_bottom_pic;         /* DS:cad0 */
extern unsigned confirm_box_top_pic;            /* DS:cad2 */
extern char *notify_text;                       /* DS:cb4e */
extern int notify_box_y;                        /* DS:cb50 */
extern int notify_box_x;                        /* DS:cb52 */
extern unsigned char *p_combat_move_cost;       /* DS:d148 */
extern int hit_frame_counters[20];                 /* DS:d160  per lost figure, two sets of 10: its death-animation frame (COMBSPRT.C; ReMoM m_gibs_frames, clarified using ReMoM repo) */

int Random(int n);
int abs(int x);
int Has_Shooting_Attack(int bu);
int Tile_Is_Water(int x, int y, int plane);
int Unit_Combat_Value(int bu);
int Sound_Play(unsigned sound);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
void Battle_Apply_Damage(int unit, int *damage);
void UnitView_Alloc_Buffers(void);
void Map_Redraw(void);
int City_Plunder_Gold(int city);
void Wizard_Add_Gold(int player, int amount);
int Monsters_Take_City(void);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void Combat_Results_Scroll(void);
void Font_Save_Colors(void);
void Font_Restore_Colors(void);
void Font_Select_Palette(int font, char *colors);
void Scratch_Save_To_EMS(void);
void Scratch_Restore_From_EMS(void);
void Font_Select();                             /* no prototype here: this module calls it with 3 arguments (FONT.ASM takes 4) */
int Paragraph_Height(int width, char *text);   /* COMBTARG.C calls it with a third argument */
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Screen_Copy_Front(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Set_Font_Color(int color);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);

/* (player) -> the ranged strength of the player's LAST living missile unit (BUG #18: not a sum)
   Numbered: BUG #18 (BUGS_AND_QUIRKS.txt) */
int Side_Missile_Strength(int player)
{
    int bu;
    int ranged;

    ranged = 0;
    for (bu = 0; bu < battle_unit_count; bu++)
        if (p_battle_units[bu].status == BATTLE_UNIT_ACTIVE && p_battle_units[bu].owner == player)
            if (Has_Shooting_Attack(bu) != 0)
                ranged = p_battle_units[bu].ranged;
    return ranged;
}

/* (unit) the movement-cost map's obstacles: standing wall corners; the city centre unless the battlefield is a
   city one; for a ground unit (not flying, non-corporeal or teleporting, no Merging) with walls or Flying Fortress:
   outside the city the standing wall squares (every wall square under Flying Fortress; the gate 8,12 stays
   open without it), inside the ring around them; then the dead test (ability_flags & 0: never true) for the core
   Numbered: BUG #251, #350 (BUGS_AND_QUIRKS.txt) */
void Combat_Mark_Wall_Obstacles(int unit)
{
    int row;
    int col;

    if (p_battlefield->has_city_walls == 1) {
        if (p_battlefield->wall_segments[0][0] == WALL_STANDING)
            p_combat_move_cost[BATTLEFIELD_CELL(5, 10)] = COST_IMPASSABLE;
        if (p_battlefield->wall_segments[0][3] == WALL_STANDING)
            p_combat_move_cost[BATTLEFIELD_CELL(8, 10)] = COST_IMPASSABLE;
        if (p_battlefield->wall_segments[3][0] == WALL_STANDING)
            p_combat_move_cost[BATTLEFIELD_CELL(5, 13)] = COST_IMPASSABLE;
        if (p_battlefield->wall_segments[3][3] == WALL_STANDING)
            p_combat_move_cost[BATTLEFIELD_CELL(8, 13)] = COST_IMPASSABLE;
    }
    if (p_battlefield->backdrop != BACKDROP_CITY)
        p_combat_move_cost[BATTLEFIELD_CELL(BF_FEATURE_X, BF_FEATURE_Y)] = COST_IMPASSABLE;
    if ((p_battle_units[unit].move_flags & MOVE_FLYING) == 0 && (p_battle_units[unit].move_flags & MOVE_TELEPORTING) == 0
        && (p_battle_units[unit].move_flags & MOVE_MERGING) == 0 && (p_battle_units[unit].ability_flags & AFLAG_NON_CORPOREAL) == 0) {
        if (p_battlefield->has_city_walls == 1 || p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] != 0) {
            if (In_City_Area(unit) != 0) {
                for (col = 0; col < 4; col++) {
                    for (row = 0; row < 4; row++) {
                        if (col == 3 && row == 2 && p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] == 0)
                            continue;
                        if (p_battlefield->wall_segments[row][col] == WALL_STANDING || p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] > 0) {
                            if (col == 0) {
                                p_combat_move_cost[(row + 10) * BATTLEFIELD_W + 4] = COST_IMPASSABLE;
                                if (row == 0)
                                    p_combat_move_cost[BATTLEFIELD_CELL(4, 9)] = COST_IMPASSABLE;
                                if (row == 3)
                                    p_combat_move_cost[BATTLEFIELD_CELL(4, 14)] = COST_IMPASSABLE;
                            }
                            if (row == 0)
                                p_combat_move_cost[col + BATTLEFIELD_CELL(5, 9)] = COST_IMPASSABLE;
                            if (col == 3) {
                                p_combat_move_cost[(row + 10) * BATTLEFIELD_W + 9] = COST_IMPASSABLE;
                                if (row == 0)
                                    p_combat_move_cost[BATTLEFIELD_CELL(9, 9)] = COST_IMPASSABLE;
                                if (row == 3)
                                    p_combat_move_cost[BATTLEFIELD_CELL(9, 14)] = COST_IMPASSABLE;
                            }
                            if (row == 3)
                                p_combat_move_cost[col + BATTLEFIELD_CELL(5, 14)] = COST_IMPASSABLE;
                        }
                    }
                }
            } else {
                for (col = 0; col < 4; col++) {
                    for (row = 0; row < 4; row++) {
                        if (col == 3 && row == 2 && p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] == 0)
                            continue;
                        if (p_battlefield->wall_segments[row][col] == WALL_STANDING || p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] > 0)
                            p_combat_move_cost[(row + 10) * BATTLEFIELD_W + col + 5] = COST_IMPASSABLE;
                    }
                }
            }
        }
    }
    if ((p_battle_units[unit].ability_flags & 0) != 0 && p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] != 0
        && (p_battle_units[unit].move_flags & MOVE_FLYING) == 0 && (p_battle_units[unit].move_flags & MOVE_TELEPORTING) == 0
        && In_City_Area(unit) == 0) {
        for (col = 0; col < 4; col++)
            for (row = 0; row < 4; row++)
                p_combat_move_cost[(row + 10) * BATTLEFIELD_W + col + 5] = COST_IMPASSABLE;
    }
}

/* () the ring of squares around the 4x4 city core is blocked */
void Combat_Block_City_Ring(void)
{
    int col;
    int row;

    for (col = 0; col < 4; col++) {
        for (row = 0; row < 4; row++) {
            if (col == 0) {
                p_combat_move_cost[(row + 10) * BATTLEFIELD_W + 4] = COST_IMPASSABLE;
                if (row == 0)
                    p_combat_move_cost[BATTLEFIELD_CELL(4, 9)] = COST_IMPASSABLE;
                if (row == 3)
                    p_combat_move_cost[BATTLEFIELD_CELL(4, 14)] = COST_IMPASSABLE;
            }
            if (row == 0)
                p_combat_move_cost[col + BATTLEFIELD_CELL(5, 9)] = COST_IMPASSABLE;
            if (col == 3) {
                p_combat_move_cost[(row + 10) * BATTLEFIELD_W + 9] = COST_IMPASSABLE;
                if (row == 0)
                    p_combat_move_cost[BATTLEFIELD_CELL(9, 9)] = COST_IMPASSABLE;
                if (row == 3)
                    p_combat_move_cost[BATTLEFIELD_CELL(9, 14)] = COST_IMPASSABLE;
            }
            if (row == 3)
                p_combat_move_cost[col + BATTLEFIELD_CELL(5, 14)] = COST_IMPASSABLE;
        }
    }
}

/* (x, y, plane, in_city) -> the battlefield kind: 6 water, 1 a city (only when in_city: Combat_Strategic passes 1,
   the tactical Combat_Main 0 - so a tactical battle is never kind 1 and Combat_Start_Auras' +3 city defense
   never applies there), 2 / 3 / 4 for the special
   terrains 0xa8 / 0xa9 / 0xaa; a city's Heavenly Light / Cloud of Shadow become combat enchantments */
int Combat_Terrain_Kind(int x, int y, int plane, int in_city)
{
    int kind;
    unsigned terrain;

    kind = BATTLEFIELD_LAND;
    if (Tile_Is_Water(x, y, plane) != 0)
        kind = BATTLEFIELD_WATER;
    if (combat_site == COMBAT_AT_CITY) {
        if (p_cities[combat_site_index].enchants[CITY_ENCH_HEAVENLY_LIGHT] > 0)
            p_combat_enchants[CENCH_TRUE_LIGHT_DEF] = CENCH_FROM_CITY;
        if (p_cities[combat_site_index].enchants[CITY_ENCH_CLOUD_OF_SHADOW] > 0)
            p_combat_enchants[CENCH_DARKNESS_DEF] = CENCH_FROM_CITY;
        if (in_city == 1)
            kind = BATTLEFIELD_CITY;
    }
    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
    if (terrain == TILE_SORCERY_NODE)
        kind = BATTLEFIELD_SORCERY_NODE;
    if (terrain == TILE_NATURE_NODE)
        kind = BATTLEFIELD_NATURE_NODE;
    if (terrain == TILE_CHAOS_NODE)
        kind = BATTLEFIELD_CHAOS_NODE;
    return kind;
}

/* (player) a battle on water: unless one of the player's units has Wind Walking (the ability or the enchantment)
   or is a Floating Island, its units that neither carry others nor fly or swim are set aside (status 3) -> how many */
int Water_Battle_Sideline(int player)
{
    int bu;
    int count;

    count = 0;
    if (Tile_Is_Water(combat_loc_x, combat_loc_y, combat_loc_plane) == 1) {
        for (bu = 0; bu < battle_unit_count; bu++) {
            if (p_battle_units[bu].owner == player) {
                if ((p_battle_units[bu].ability_flags & AFLAG_WIND_WALKING) != 0
                    || (p_units[p_battle_units[bu].unit_index].enchantments & UNIT_ENCH_WIND_WALKING) != 0
                    || p_units[p_battle_units[bu].unit_index].type == (int)p_spell_data[SPELL_FLOATING_ISLAND].effect_param)
                    return 0;
            }
        }
        for (bu = 0; bu < battle_unit_count; bu++) {
            if (p_battle_units[bu].owner == player && p_battle_units[bu].transport_capacity == 0
                && (p_battle_units[bu].move_flags & MOVE_FLYING) == 0 && (p_battle_units[bu].move_flags & MOVE_SWIMMING) == 0) {
                p_battle_units[bu].status = BATTLE_UNIT_SET_ASIDE;
                count++;
            }
        }
    }
    return count;
}

/* (player) -> the odds 0..5 from the combat values of the living units (after turn 1 only those with a target,
   and the human's): 0 when the player has none, 5 when the enemy has none; else by the value ratio */
int Battle_Odds(int player)
{
    int bu;
    int own_value;
    int enemy_value;
    int odds;
    int unit_value;
    int own_strength;
    int enemy_strength;

    own_value = 0;
    enemy_value = 0;
    own_strength = 0;
    enemy_strength = 0;
    for (bu = 0; bu < battle_unit_count; bu++) {
        if (p_battle_units[bu].status == BATTLE_UNIT_ACTIVE
            && (combat_turn_count < 2 || p_battle_units[bu].target_unit != -1 || p_battle_units[bu].owner == PLAYER_HUMAN)) {
            unit_value = Unit_Combat_Value(bu);
            if (p_battle_units[bu].owner == player) {
                own_value += unit_value;
                own_strength += p_battle_units[bu].melee + p_battle_units[bu].ranged;
            } else {
                enemy_value += unit_value;
                enemy_strength += p_battle_units[bu].melee + p_battle_units[bu].ranged;
            }
        }
    }
    if (own_value == 0 || own_strength == 0)
        return 0;
    if (enemy_value == 0 || enemy_strength == 0)
        return 5;
    if (enemy_value / own_value > 3)
        odds = 0;
    else if (enemy_value / own_value > 2)
        odds = 1;
    else if (enemy_value * 2 >= own_value * 3)
        odds = 2;
    else if (own_value / enemy_value > 3)
        odds = 5;
    else if (own_value / enemy_value > 2)
        odds = 4;
    else
        odds = 3;
    return odds;
}

/* (bu) -> the unit's movement sound: UnitType.sounds high nibble = SOUNDFX entry (0: entry 106; 13: NEWSOUND 0) */
int Unit_Load_Move_Sound(int bu)
{
    int entry;                                  /* the LBX entry, then the loaded sound returned */

    entry = unit_types[p_units[p_battle_units[bu].unit_index].type].sounds / 16;
    if (entry == 0)
        entry = LBX_Reload_Next("soundfx", SFX_SILENCE, p_world_block);
    else if (entry == 13)
        entry = LBX_Reload_Next("newsound", 0, p_world_block);
    else
        entry = LBX_Reload_Next("soundfx", entry, p_world_block);
    return entry;
}

/* (bu) -> the unit's attack sound: UnitType.sounds low nibble = CMBTSND entry (8: SFX_SILENCE, none) */
int Unit_Load_Attack_Sound(int bu)
{
    int entry;

    entry = unit_types[p_units[p_battle_units[bu].unit_index].type].sounds & 15;
    if (entry == 8)
        entry = LBX_Reload_Next("soundfx", SFX_SILENCE, p_world_block);
    else
        entry = LBX_Reload_Next("cmbtsnd", entry, p_world_block);
    return entry;
}

/* (spell) plays the silence (combat_silence_sfx: cuts the sound off) and loads the spell's sound into p_sfx_buf (SOUNDFX, or NEWSOUND for a negative
   index); -1 with sound effects off
   Numbered: BUG #260 (BUGS_AND_QUIRKS.txt) */
void Combat_Load_Spell_Sound(int spell)
{
    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        Heap_Mark(p_world_block);
        if (p_spell_data[spell].sound_index > -1)
            p_sfx_buf = LBX_Reload_Next("SOUNDFX", p_spell_data[spell].sound_index, p_world_block);
        else
            p_sfx_buf = LBX_Reload_Next("NEWSOUND", abs(p_spell_data[spell].sound_index), p_world_block);
        Heap_Release(p_world_block);
    } else
        p_sfx_buf = 0xffff;
}

/* (unit) a ground unit (not flying, teleporting or merging) outside the city core moving into it through
   Wall of Fire takes Fireball damage
   Numbered: BUG #295 (BUGS_AND_QUIRKS.txt) */
void Wall_Of_Fire_Crossing(int unit)
{
    int damage[3];

    if (p_battlefield->wall_of_fire > 0 && (p_battle_units[unit].move_flags & MOVE_FLYING) == 0
        && (p_battle_units[unit].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[unit].move_flags & MOVE_MERGING) == 0
        && In_City_Area(unit) == 0 && p_battle_units[unit].dest_x >= BF_CITY_X1 && p_battle_units[unit].dest_x <= BF_CITY_X2
        && p_battle_units[unit].dest_y >= BF_CITY_Y1 && p_battle_units[unit].dest_y <= BF_CITY_Y2) {
        Spell_Damage_Roll(SPELL_FIREBALL, unit, damage, 0);
        Battle_Apply_Damage(unit, damage);
    }
}

/* (unit, damage) the hit animation: figures killed = ceil(damage / hits per figure), at most the living ones;
   intensity = (damage + figures / 2) / (2 x figures), 1..5 (3 with no figures); the 20 hit-frame counters
   (hit_frame_counters, COMBSPRT.C) restart at Random(4) - 1 = 0..3 */
void Battle_Unit_Set_Hit_Anim(int unit, int damage)
{
    int intensity;
    int kills;
    int k;

    kills = 0;
    if (p_battle_units[unit].figures_alive == 0)
        intensity = 3;
    else
        intensity = (damage + p_battle_units[unit].figures_alive / 2) / (p_battle_units[unit].figures_alive * 2);
    if (intensity > 5)
        intensity = 5;
    if (damage > 0 && intensity == 0)
        intensity = 1;
    if (damage > 0) {
        kills = (damage + p_battle_units[unit].hits_per_figure - 1) / p_battle_units[unit].hits_per_figure;
        if (p_battle_units[unit].figures_alive < kills)
            kills = p_battle_units[unit].figures_alive;
    }
    for (k = 0; k < 20; k++)
        hit_frame_counters[k] = Random(4) - 1;
    p_battle_units[unit].hit_anim_kills = kills;
    p_battle_units[unit].hit_anim_intensity = intensity;
}

/* (bu) the hit animation off */
void Battle_Unit_Clear_Hit_Anim(int bu)
{
    p_battle_units[bu].hit_anim_kills = 0;
    p_battle_units[bu].hit_anim_intensity = 0;
}

/* (bu) -> 1 inside the city core (combat x 5..8, y 10..13) */
int In_City_Area(int bu)
{
    if (p_battle_units[bu].combat_x >= BF_CITY_X1 && p_battle_units[bu].combat_y >= BF_CITY_Y1 && p_battle_units[bu].combat_x <= BF_CITY_X2
        && p_battle_units[bu].combat_y <= BF_CITY_Y2)
        return 1;
    else
        return 0;
}

/* (x, y) -> 1 on a standing wall square (walls present, the edge of the 4 x 4 core - not its 2 x 2 interior 6-7 x 11-12) */
int Is_City_Wall_Square(int x, int y)
{
    int result;

    result = 0;
    if (p_battlefield->has_city_walls == 1 && x >= BF_CITY_X1 && x <= BF_CITY_X2 && y >= BF_CITY_Y1 && y <= BF_CITY_Y2
        && ((x != 6 && x != 7) || (y != 11 && y != 12)) && p_battlefield->wall_segments[y - BF_CITY_Y1][x - BF_CITY_X1] == WALL_STANDING)
        result = 1;
    return result;
}

/* (bu) -> 1 for the combat-only summons: the Fire, Earth and Air Elementals, Phantom Beast / Warriors, Demon */
int Is_Combat_Only_Summon(int bu)
{
    int unit_type;

    unit_type = p_units[p_battle_units[bu].unit_index].type;
    if ((int)p_spell_data[SPELL_FIRE_ELEMENTAL].effect_param == unit_type)
        return 1;
    if ((int)p_spell_data[SPELL_EARTH_ELEMENTAL].effect_param == unit_type)
        return 1;
    if ((int)p_spell_data[SPELL_PHANTOM_BEAST].effect_param == unit_type)
        return 1;
    if ((int)p_spell_data[SPELL_PHANTOM_WARRIORS].effect_param == unit_type)
        return 1;
    if ((int)p_spell_data[SPELL_AIR_ELEMENTAL].effect_param == unit_type)
        return 1;
    if (unit_type == UNIT_DEMON)
        return 1;
    return 0;
}

/* (bu) -> combat moves: 0 when webbed; else the type's moves + the hero's item moves, x1.5 / x0.5 for a ship by the
   balance of Wind Mastery casters, at least 6 with Flight, 4 with Chaos wings, +2 with Endurance (BUG #197: the
   item flag is never initialised), x2 with Haste
   Numbered: BUG #289, #316 (BUGS_AND_QUIRKS.txt) */
int Unit_Combat_Moves(int bu)
{
    int moves;
    int endurance_item;                         /* BUG #197: only ever set to 1 */
    int i;
    int item_moves;
    int wind;
    int *slots;
    long enchants;

    if (p_battle_units[bu].web_hp > 0)
        return 0;
    enchants = p_units[p_battle_units[bu].unit_index].enchantments | p_battle_units[bu].combat_enchantments
               | p_battle_units[bu].item_enchantments;
    item_moves = 0;
    if (p_units[p_battle_units[bu].unit_index].hero_slot > -1) {
        slots = wizards[p_battle_units[bu].owner].hired_heroes[p_units[p_battle_units[bu].unit_index].hero_slot].item_index;
        for (i = 0; i < 3; i++) {
            if (slots[i] > -1) {
                if ((p_items[slots[i]].powers & ITEM_POWER_ENDURANCE) > 0)
                    endurance_item = 1;
                item_moves += p_items[slots[i]].moves;
            }
        }
    }
    moves = unit_types[p_units[p_battle_units[bu].unit_index].type].moves;
    moves += item_moves;
    if (unit_types[p_units[p_battle_units[bu].unit_index].type].transport_capacity > 0) {
        wind = 0;
        for (i = 0; i < num_players; i++) {
            if (wizards[i].globals.wind_mastery > 0) {
                if (p_battle_units[bu].owner == i)
                    wind++;
                else
                    wind--;
            }
        }
        if (wind > 0)
            moves = moves * 3 / 2;
        if (wind < 0)
            moves = moves / 2;
    }
    if (moves < 6 && (enchants & UNIT_ENCH_FLIGHT) != 0)
        moves = 6;
    if (moves < 4 && (p_units[p_battle_units[bu].unit_index].mutation_flags & MUT_CHAOS_DEMON_WINGS) != 0)
        moves = 4;
    if ((enchants & UNIT_ENCH_ENDURANCE) > 0 || endurance_item == 1)
        moves += 2;
    if ((p_battle_units[bu].combat_flags & COMBAT_FLAG_HASTE) != 0)
        moves *= 2;
    return moves;
}

/* (n, units) a stack entered a city without a battle: the old owner loses fame (the city's size); the city is
   razed when empty, by the human's answer (with the raze option on) or by AI_Decide_Raze. Kept: the winner gains
   fame size - 2 (at least 0) and the plunder, which the old owner loses. Razed: the winner loses fame (the size),
   the old owner loses the plunder, the winner gets the plunder + a tenth of the cost of each building not
   BLDSTATE_NONE (replaced and removed ones too).
   Neutral fantastic attackers take the city as monsters and disappear; the human gets the result scroll */
void City_Captured(int n, int *units)
{
    int i;
    int player;
    int owner;

    UnitView_Alloc_Buffers();
    Set_Draw_Page();
    Map_Redraw();
    Screen_Save_A800();
    player = p_units[units[0]].owner;
    owner = p_cities[combat_site_index].owner;
    combat_fame_change = 0;
    if (owner < num_players) {
        wizards[owner].fame -= p_cities[combat_site_index].size;
        if (owner == PLAYER_HUMAN)
            combat_fame_change -= p_cities[combat_site_index].size;
        if (wizards[owner].fame < 0)
            wizards[owner].fame = 0;
    }
    if (p_cities[combat_site_index].population == 0)
        city_razed = 1;
    else {
        city_razed = 0;
        if (player == PLAYER_HUMAN) {
            if (game_config.raze_city == 1) {
                i = Confirm_Dialog("Do you wish to completely destroy this city?");
                if (i == CONFIRM_RAZE)
                    city_razed = 1;
            }
        } else if (player < num_players)
            city_razed = AI_Decide_Raze(player, combat_site_index);
    }
    if (city_razed == 0) {
        combat_end_value = p_cities[combat_site_index].size - 2;
        if (combat_end_value < 0)
            combat_end_value = 0;
        wizards[player].fame = wizards[player].fame + combat_end_value;
        if (player == PLAYER_HUMAN)
            combat_fame_change += combat_end_value;
        combat_end_value = 0;
        combat_end_value = City_Plunder_Gold(combat_site_index);
        if (player < num_players)
            Wizard_Add_Gold(player, combat_end_value);
        if (owner < num_players) {
            wizards[owner].gold -= combat_end_value;
            if (wizards[owner].gold < 0)
                wizards[owner].gold = 0;
        }
    } else {
        combat_end_value = p_cities[combat_site_index].size;
        if (player == PLAYER_HUMAN)
            combat_fame_change -= combat_end_value;
        if (player < num_players) {
            wizards[player].fame -= combat_end_value;
            if (wizards[player].fame < 0)
                wizards[player].fame = 0;
        }
        combat_end_value = City_Plunder_Gold(combat_site_index);
        if (owner < num_players) {
            wizards[owner].gold -= combat_end_value;
            if (wizards[owner].gold < 0)
                wizards[owner].gold = 0;
        }
        for (i = BLD_BARRACKS; i < NUM_BUILDINGS; i++)
            if (p_cities[combat_site_index].buildings[i] > -1)
                combat_end_value += p_building_data[i].cost / 10;
        if (player < num_players)
            Wizard_Add_Gold(player, combat_end_value);
    }
    p_battle_units = (struct BattleUnit far *)Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, 63));
    if (p_units[units[0]].owner == PLAYER_NEUTRAL && (unit_types[p_units[units[0]].type].ability_flags & AFLAG_FANTASTIC) != 0) {
        for (i = 0; i < n; i++) {
            p_battle_units[i].owner = PLAYER_NEUTRAL;
            p_battle_units[i].status = BATTLE_UNIT_ACTIVE;
            p_battle_units[i].unit_index = units[i];
        }
        battle_unit_count = n;
        combat_active_unit = Monsters_Take_City();
        for (i = 0; i < n; i++)
            Unit_Kill_Or_Dismiss(units[i], UNIT_REMOVE_KILLED);
    }
    combat_pop_lost = 0;
    if (player == PLAYER_HUMAN)
        combat_result_kind = COMBAT_RESULT_WON;
    if (owner == PLAYER_HUMAN)
        combat_result_kind = COMBAT_RESULT_CITY_CONQUERED;
    combat_pop_lost = 0;
    combat_bldgs_lost_count = 0;
    if (player == PLAYER_HUMAN || owner == PLAYER_HUMAN)
        Combat_Results_Scroll();
}

/* (player) -> 1 when the odds are hopeless (0) and the player has a hero there, or only settlers / builders */
int AI_Should_Flee(int player)
{
    int bu;
    int heroes;
    int odds;
    int noncombat;
    int units;

    heroes = 0;
    noncombat = 0;
    units = 0;
    odds = Battle_Odds(player);
    if (odds == 0) {
        for (bu = 0; bu < battle_unit_count; bu++) {
            if ((p_battle_units[bu].status == BATTLE_UNIT_ACTIVE || p_battle_units[bu].status == BATTLE_UNIT_SET_ASIDE) && p_battle_units[bu].owner == player) {
                units++;
                if (p_units[p_battle_units[bu].unit_index].hero_slot > -1)
                    heroes++;
                if ((p_battle_units[bu].ability_flags & AFLAG_CREATE_OUTPOST) != 0 || p_battle_units[bu].construction > 0)
                    noncombat++;
            }
        }
        if (heroes > 0 || units == noncombat)
            return 1;
    }
    return 0;
}

/* (player, city) -> 1 to raze: never a neutral city; chance = by personality (Maniacal 25, Ruthless 10, Lawful 25
   or -200 with no relation bias toward the owner, Peaceful -200), +10 with fewer than 3 living battle units, +
   own units x 50 / owner's units on the city's landmass - 50 (-200 when the owner has none there); Chaotic at
   least 10; raze when Random(100) <= chance
   Numbered: BUG #217, #276 (BUGS_AND_QUIRKS.txt) */
int AI_Decide_Raze(int player, int city)
{
    int chance;
    int k;
    int alive;
    unsigned city_lm;
    int own_units;
    int owner_units;

    chance = 0;
    if (p_cities[city].owner == PLAYER_NEUTRAL)
        return 0;
    switch (wizards[player].personality) {
    case PERSONALITY_MANIACAL:
        chance += 25;
        break;
    case PERSONALITY_RUTHLESS:
        chance += 10;
        break;
    case PERSONALITY_AGGRESSIVE:
        while (0)                               /* the game has a lone jmp to the next case here: an empty statement
                                                   that compiles to one (the original statement is unknown: the bytes fix only the jmp) */
            ;
    case PERSONALITY_CHAOTIC:
        break;
    case PERSONALITY_LAWFUL:
        if (wizards[player].relation_bias[p_cities[city].owner] != 0)
            chance += 25;
        else
            chance -= 200;
        break;
    case PERSONALITY_PEACEFUL:
        chance -= 200;
        break;
    }
    alive = 0;
    for (k = 0; k < battle_unit_count; k++)
        if (p_battle_units[k].status == BATTLE_UNIT_ACTIVE)
            alive++;
    if (alive < 3)
        chance += 10;
    city_lm = p_landmass_ids[p_cities[city].plane][p_cities[city].y][p_cities[city].x];
    own_units = 0;
    owner_units = 0;
    for (k = 0; k < num_units; k++) {
        if (p_landmass_ids[p_units[k].plane][p_units[k].y][p_units[k].x] == city_lm) {
            if (p_units[k].owner == player)
                own_units++;
            else if (p_units[k].owner == p_cities[city].owner)
                owner_units++;
        }
    }
    if (owner_units == 0)
        chance -= 200;
    else
        chance += own_units * 50 / owner_units - 50;
    if (wizards[player].personality == PERSONALITY_CHAOTIC && chance < 10)
        chance = 10;
    if (Random(100) <= chance)
        return 1;
    else
        return 0;
}

/* (text) -> 0 raze (left button, "R") or 1 keep (right button, "N"): the question in the notify box over the
   map; Esc and clicks outside come back as screen_control, which the loop ignores */
int Confirm_Dialog(char *text)
{
    int height;
    int done;
    int box_height;
    int input;
    int keep_button;
    int raze_button;
    int screen_control;
    int box_control;
    int answer;

    Font_Save_Colors();
    Font_Select_Palette(0, confirm_text_colors);
    Scratch_Save_To_EMS();
    confirm_box_top_pic = LBX_Reload("RESOURCE", 0, p_scratch);
    confirm_box_bottom_pic = LBX_Reload_Next("RESOURCE", 1, p_scratch);
    keep_button_pic = LBX_Reload_Next("compix", 81, p_scratch);
    raze_button_pic = LBX_Reload_Next("compix", 82, p_scratch);
    Screen_Copy_Front();
    notify_text = text;
    Font_Select(4, 4, 15);
    height = Paragraph_Height(166, text);
    box_height = height + 34;
    notify_box_x = confirm_box_dx + 68;
    notify_box_y = (200 - box_height) / 2 + confirm_box_dy;
    UI_Clear_Controls();
    keep_button = UI_Add_Button(notify_box_x + 101, notify_box_y + height + 15, "", keep_button_pic, "N", -1);
    raze_button = UI_Add_Button(notify_box_x + 18, notify_box_y + height + 15, "", raze_button_pic, "R", -1);
    /* the shared yes / no picture slots hold COMPIX 81 Keep / 82 Raze here: Raze sits in the "no" (left) slot */
    box_control = UI_Add_Region(notify_box_x, notify_box_y, notify_box_x + 185, notify_box_y + 63, "", -1);
    screen_control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    UI_Set_Redraw_Fn(Confirm_Dialog_Redraw, 1);
    done = 0;
    while (done == 0) {
        input = abs(UI_Get_Input());            /* never -1 after abs: Esc comes through screen_control */
        if (input == -1 || input == raze_button) {
            done = 1;
            answer = 0;
        }
        if (input == keep_button) {
            done = 1;
            answer = 1;
        }
        Confirm_Dialog_Redraw();
        Screen_Present();
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Font_Restore_Colors();
    Reset_Clip_Rect();
    Scratch_Restore_From_EMS();
    return answer;
}

/* () redraw of the question: the box clipped to the text height, its bottom edge and the shadowed paragraph */
void Confirm_Dialog_Redraw(void)
{
    int height;

    Font_Select(4, 4, 15);
    height = Paragraph_Height(166, notify_text);
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, 319, notify_box_y + height + 12);
    Draw_Pic_Clipped(notify_box_x, notify_box_y, confirm_box_top_pic);
    Reset_Clip_Rect();
    Draw_Pic_Frame(notify_box_x, notify_box_y + height + 10, confirm_box_bottom_pic);
    Font_Select_Palette(4, notify_shadow_colors);
    Font_Select(4, 15, 15);
    Paragraph_Print(notify_box_x + 10, notify_box_y + 10, 166, notify_text, PARA_LEFT);
    Paragraph_Print(notify_box_x + 9, notify_box_y + 10, 166, notify_text, PARA_LEFT);
    Set_Font_Color(18);
    Font_Select_Palette(4, confirm_text_colors);
    Font_Select(4, 4, 15);
    Paragraph_Print(notify_box_x + 9, notify_box_y + 9, 166, notify_text, PARA_LEFT);
}
