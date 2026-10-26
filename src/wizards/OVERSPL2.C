/* OVERSPL2.C - WIZARDS.EXE overlay code segment acd6 (stub 25a3): overland spells, second part - Cruel Unminding,
   the overland spell damage of one unit / one tile, Black Wind's resist-or-die, Call the Void (also the meteor
   event), the city protection check (Consecration, the wards, Nightshade), Chaos Channels, Warp Node, Stasis,
   Nature's Cures, Move Fortress, Earthquake (also the earthquake event) and Nature's Wrath striking a wizard.
   This module's data (DS:6686..6760): its literals only; no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS acd6 src/wizards/OVERSPL2.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Cruel_Unminding_Apply(int wizard);
void Spell_Damage_Tile(int unit, int spell);
void Unit_Spell_Damage(int unit, int spell, int *n_items, int *items, int strength);
void Stack_Resist_Or_Die(int x, int y, int plane, int spell);
int Call_The_Void_Cast(int player);
int Call_The_Void_Effect(int city);
int City_Spell_Protection_Check(int spell, int city, int caster, int show);
void Chaos_Channels_Apply(int unit);
int Chaos_Channels_Cast(int player);
int Warp_Node_Cast(int player);
int Stasis_Cast(int player);
int Natures_Cures_Cast(int player);
int Move_Fortress_Cast(int player);
int Earthquake_Cast(int player);
int Earthquake_Effect(int city, int *item_count, int *items);
void Natures_Wrath_Strike(int target);

#include "GAME.H"

extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern unsigned music_jingle;                   /* DS:9134 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern unsigned p_music_buf;                    /* DS:9390 */
extern unsigned diplo_mirror_pic;               /* DS:9918 */
extern char report_buildings_lost_count;              /* DS:9b42 */
extern struct ReportBldgLost report_buildings_lost[]; /* DS:9b44 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int award_loc_plane;                     /* DS:c300 */
extern int award_loc_y;                         /* DS:c302 */
extern int award_loc_x;                         /* DS:c304 */
extern int award_at_location;                   /* DS:c306 */
extern int far *p_aura_holy_bonus;              /* DS:c89e */
extern int ai_replan;                           /* DS:c9f0 */

int AI_Overland_Spell_Target(int target_kind, int *x, int *y, int *plane, int spell, int caster);
int AI_Pick_City_Spell_Target(int target_kind, int *city, int spell, int caster);
int AI_Pick_Unit_Enchant_Target(int target_kind, int *unit, int spell, int caster);
int AI_Target_Warp_Node(int *node, int spell);
void Battle_Apply_Damage(int unit, int *damage);
int CitySpell_Cast_Anim(int city, int spell, int wizard);
void CitySpell_View_Loop();                     /* no prototype here: called with 3 arguments (CASTANIM.C takes 2) */
void City_Collateral_Damage(int city, int pop_lost, int bldg_pct, int *destroyed);
void City_Garrison_List(int city, int *count, int *list);
void Font_Select_Outline(int font, int c1, int c2, int c3);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Hero_Items_Redistribute(int player, int count, int *items);
void Hostile_Spell_Relations(int caster, int spell, int city);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Map_Full_Redraw(void);
void Map_Redraw(void);
void Map_Spell_Effect_Anim(int x, int y, int plane);
void Minimap_Alloc_Pic(void);
void Music_Play_Current(void);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
void far *Seg_To_Far_Ptr(unsigned seg);
int Sound_Play(unsigned sound);
int Sound_Stop(void);
int Spell_Load_Anim(int spell);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
int Spell_Dispel_Resistance(int strength, int player, int realm);
int Spell_Fizzle_Message_Overland(int victim, int blocker, int spell, char *blocker_spell);
void Spell_Load_Sound(int spell);
int Spell_Target_Map_Select(int target_type, int *target, int *y, int *plane, int *screen_x, int *screen_y,
                            char *spell_name);
int Tile_Is_Water(int x, int y, int plane);
int Tile_Visible(int x, int y, int plane);
void Transition_Wipe_Left(int x, int y);
void Unit_Kill_Or_Dismiss(int unit, int mode);

/* (wizard): Cruel Unminding - the casting skill drops by Random(10) % (at least 1); the skill points are set to the
   start of the new level; returns the loss */
int Cruel_Unminding_Apply(int wizard)
{
    int loss;
    int new_skill;
    int k;
    long points;
    int step;
    int skill;

    skill = Wizard_Casting_Skill(wizard);
    loss = Random(10);
    loss = (long)skill * loss / 100;
    if (loss == 0)
        loss = 1;
    new_skill = skill - loss;
    if (wizards[wizard].retorts[RETORT_ARCHMAGE] > 0)
        new_skill -= 10;                        /* Archmage's +10 comes off before the points */
    points = 0;                                 /* skill n needs n * (n - 1) + 1 points */
    step = 0;
    for (k = 0; k < new_skill; k++) {
        points += step;
        step += 2;
    }
    wizards[wizard].skill_points = points + 1;
    return loss;
}

/* (unit, spell): the spell's damage to every unit on that unit's tile; the dead heroes' items are handed out */
void Spell_Damage_Tile(int unit, int spell)
{
    int i;
    int player;
    int tx;
    int ty;
    int tp;
    int item_count;
    int items[18];

    tx = p_units[unit].x;
    ty = p_units[unit].y;
    tp = p_units[unit].plane;
    item_count = 0;
    award_at_location = 1;
    award_loc_x = tx;
    award_loc_y = ty;
    award_loc_plane = tp;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].x == tx && p_units[i].y == ty && p_units[i].plane == tp) {
            player = p_units[i].owner;
            Unit_Spell_Damage(i, spell, &item_count, items, 0);
        }
    }
    Hero_Items_Redistribute(player, item_count, items);
}

/* (unit, spell, &n_items, items, strength): overland spell damage to one unit (Regeneration negates the normal
   damage); a dead hero's items are added to items */
void Unit_Spell_Damage(int unit, int spell, int *n_items, int *items, int strength)
{
    int k;
    int j;
    int dmg[3];
    int total;
    long ench;

    ai_replan = 1;
    for (k = 0; k < 3; k++)
        dmg[k] = 0;
    p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, BATTLE_UNIT_PARAS));
    p_aura_holy_bonus = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 3));
    for (k = 0; k < num_players; k++)
        p_aura_holy_bonus[k] = 0;
    Battle_Unit_Init(unit, p_battle_units);
    Spell_Damage_Roll(spell, 0, dmg, strength);   /* 0: the one battle unit just built at p_battle_units[0] */
    ench = p_battle_units->combat_enchantments | p_units[unit].enchantments | p_battle_units->item_enchantments;
    if ((ench & UNIT_ENCH_REGENERATION) || (p_battle_units->ability_flags & AFLAG_REGENERATION))
        dmg[0] = 0;
    total = 0;
    for (j = 0; j < 3; j++)
        total += dmg[j];
    total += p_battle_units->front_damage;
    p_battle_units->figures_alive = p_battle_units->figures_alive - total / p_battle_units->hits_per_figure;
    p_battle_units->front_damage = total % p_battle_units->hits_per_figure;
    if (p_battle_units->figures_alive <= 0) {
        if (p_units[unit].hero_slot > -1) {
            for (j = 0; j < 3; j++) {
                if (wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[j] > -1) {
                    items[*n_items] = wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[j];
                    *n_items = *n_items + 1;
                }
                wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[j] = 0;   /* BUG #472: 0, not -1 (none) - as Death Wish */
            }
        }
        Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
    } else {
        if (p_battle_units->front_damage < 0)
            p_battle_units->front_damage = 0;
        p_units[unit].damage = p_battle_units->front_damage +
                               (p_battle_units->figures - p_battle_units->figures_alive) * p_battle_units->hits_per_figure;
    }
}

/* (x, y, plane, spell): every figure on the tile resists (Black Wind: at -1) or dies */
void Stack_Resist_Or_Die(int x, int y, int plane, int spell)
{
    int unit;
    int j;
    int resist_roll;
    int player;
    int item_count;
    int modifier;
    int dmg[3];
    int items[18];

    ai_replan = 1;
    Heap_Mark(p_scratch);
    p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, BATTLE_UNIT_PARAS));
    item_count = 0;
    modifier = 0;
    if (spell == SPELL_BLACK_WIND)
        modifier = -1;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].x == x && p_units[unit].y == y && p_units[unit].plane == plane) {
            player = p_units[unit].owner;
            for (j = 0; j < 3; j++)
                dmg[j] = 0;
            Battle_Unit_Init(unit, p_battle_units);
            for (j = 0; j < p_battle_units->figures_alive; j++) {
                resist_roll = Roll_Resist(*p_battle_units, modifier, p_spell_data[spell].realm);
                if (resist_roll > 0)
                    dmg[0] += p_battle_units->hits_per_figure;
            }
            Battle_Apply_Damage(0, dmg);
            p_units[unit].damage = p_units[unit].damage + dmg[0];
            if (p_battle_units->status > 0) {
                if (p_units[unit].hero_slot > -1) {
                    for (j = 0; j < 3; j++) {
                        if (wizards[player].hired_heroes[p_units[unit].hero_slot].item_index[j] > -1) {
                            items[item_count] = wizards[player].hired_heroes[p_units[unit].hero_slot].item_index[j];
                            item_count++;
                        }
                        wizards[player].hired_heroes[p_units[unit].hero_slot].item_index[j] = 0;   /* BUG #472: 0, not -1 (none) - as Death Wish */
                    }
                }
                Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
            }
        }
    }
    Heap_Release(p_scratch);
    award_at_location = 1;
    award_loc_x = x;
    award_loc_y = y;
    award_loc_plane = plane;
    Hero_Items_Redistribute(player, item_count, items);
}

/* (player): Call the Void on a city - target, protection check, the effect, the relation penalty */
int Call_The_Void_Cast(int player)
{
    int city;
    int ty;
    int tp;
    int ok;
    int sx;
    int sy;

    ai_replan = 1;
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    if (player == PLAYER_HUMAN)
        ok = Spell_Target_Map_Select(MAP_TARGET_ENEMY_CITY, &city, &ty, &tp, &sx, &sy, "Call the Void");
    else
        ok = AI_Pick_City_Spell_Target(MAP_TARGET_ENEMY_CITY, &city, SPELL_CALL_THE_VOID, player);
    if (ok == 1)
        ok = City_Spell_Protection_Check(SPELL_CALL_THE_VOID, city, player, 1);
    if (ok == 1) {
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0) {
            ai_replan = 1;
            Spell_Load_Sound(SPELL_CALL_THE_VOID);
            Spell_Load_Anim(SPELL_CALL_THE_VOID);
            Map_Spell_Effect_Anim(p_cities[city].x, p_cities[city].y, p_cities[city].plane);
        }
        Heap_Release(p_scratch);
        Call_The_Void_Effect(city);
        Map_Full_Redraw();
        Hostile_Spell_Relations(player, SPELL_CALL_THE_VOID, city);
    }
    return ok;
}

/* (city): Call the Void's effect (also the meteor event) - 5x5 land corruption at 50% per square, the garrison's
   tile takes the spell's damage, buildings 50% collateral damage and about half the citizens (1 in 2 for each beyond the first); returns the
   buildings lost
   Numbered: BUG #35 (BUGS_AND_QUIRKS.txt) */
int Call_The_Void_Effect(int city)
{
    int i;
    int j;
    int gcount;
    int lost;
    int x;
    int garrison[MAX_STACK];
    int destroyed[NUM_BUILDINGS];

    for (i = p_cities[city].x - 2; i <= p_cities[city].x + 2; i++) {
        if (i < 0)
            x = i + MAP_WIDTH;
        else if (i > MAP_WIDTH)   /* BUG #35: not >=, so a city at x 58 / 59 reaches column 60, off the map */
            x = i - MAP_WIDTH;
        else
            x = i;
        for (j = p_cities[city].y - 2; j <= p_cities[city].y + 2; j++) {
            if (j < 0 || j > MAP_HEIGHT) {   /* BUG #35: not >=, so row 40 (off the map) gets through */
            } else if (Random(2) == 1 && Tile_Is_Water(x, j, p_cities[city].plane) != 1) {
                p_terrain_flags_maps[p_cities[city].plane][j][x] = p_terrain_flags_maps[p_cities[city].plane][j][x] | TERRAIN_FLAG_CORRUPTION;
            }
        }
    }
    City_Garrison_List(city, &gcount, garrison);
    if (gcount > 0)
        Spell_Damage_Tile(garrison[0], SPELL_CALL_THE_VOID);
    for (i = 0; i < NUM_BUILDINGS; i++)
        destroyed[i] = 0;
    j = 0;
    for (i = 0; i < p_cities[city].population - 1; i++) {
        if (Random(2) == 1)
            j++;
    }
    City_Collateral_Damage(city, j, 50, destroyed);
    lost = 0;
    for (i = 0; i < NUM_BUILDINGS; i++) {
        if (destroyed[i] > 0)
            lost++;
    }
    if (p_cities[city].owner == PLAYER_HUMAN) {
        for (j = 0; j < NUM_BUILDINGS; j++) {
            if (destroyed[j] > 0 && report_buildings_lost_count < MAX_REPORT_ENTRIES) {
                report_buildings_lost[report_buildings_lost_count].city = city;
                report_buildings_lost[report_buildings_lost_count].building = destroyed[j];
                report_buildings_lost_count++;
            }
        }
    }
    return lost;
}

/* (spell, city, caster, show) -> 1: the spell goes through; Consecration blocks Chaos and Death, each ward its realm,
   Nightshade dispels on a roll (show: the fizzle message)
   Numbered: BUG #230 (BUGS_AND_QUIRKS.txt) */
int City_Spell_Protection_Check(int spell, int city, int caster, int show)
{
    int allowed;
    int chance;

    allowed = 1;
    if (p_cities[city].enchants[CITY_ENCH_CONSECRATION] > 0 &&
        (p_spell_data[spell].realm == REALM_DEATH || p_spell_data[spell].realm == REALM_CHAOS)) {
        if (show)
            Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Consecration");
        allowed = 0;
    }
    if (p_cities[city].enchants[CITY_ENCH_DEATH_WARD] > 0 && p_spell_data[spell].realm == REALM_DEATH) {
        if (show)
            Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Death Ward");
        allowed = 0;
    }
    if (p_cities[city].enchants[CITY_ENCH_LIFE_WARD] > 0 && p_spell_data[spell].realm == REALM_LIFE) {
        if (show)
            Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Life Ward");
        allowed = 0;
    }
    if (p_cities[city].enchants[CITY_ENCH_NATURE_WARD] > 0 && p_spell_data[spell].realm == REALM_NATURE) {
        if (show)
            Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Nature Ward");
        allowed = 0;
    }
    if (p_cities[city].enchants[CITY_ENCH_CHAOS_WARD] > 0 && p_spell_data[spell].realm == REALM_CHAOS) {
        if (show)
            Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Chaos Ward");
        allowed = 0;
    }
    if (p_cities[city].enchants[CITY_ENCH_SORCERY_WARD] > 0 && p_spell_data[spell].realm == REALM_SORCERY) {
        if (show)
            Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Sorcery Ward");
        allowed = 0;
    }
    if (p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] > 0) {
        chance = Spell_Dispel_Resistance(wizards[caster].casting_cost_initial, caster, p_spell_data[spell].realm) +
                 p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] * 100;
        chance = (long)p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] * 25000 / chance;
        if (Random(250) <= chance) {
            if (show)
                Spell_Fizzle_Message_Overland(caster, p_cities[city].owner, spell, "Nightshade");
            allowed = 0;
        }
    }
    return allowed;
}

/* (unit): Chaos Channels - a random wings / fire breath / armour mutation, re-rolled when it does not apply */
void Chaos_Channels_Apply(int unit)
{
    int effect;
    int unit_type;

    effect = -1;
    unit_type = p_units[unit].type;
    while (effect == -1) {
        effect = Random(3) - 1;
        if (effect == 0 && ((unit_types[unit_type].move_flags & MOVE_FLYING) || (unit_types[unit_type].move_flags & MOVE_SAILING)))
            effect = -1;
        if (effect == 1 && (unit_types[unit_type].ranged > 3 ||
                       (unit_types[unit_type].ranged_type != RANGED_NONE && unit_types[unit_type].ranged_type != RANGED_THROWN)))
            effect = -1;
    }
    switch (effect) {
    case 0:
        p_units[unit].mutation_flags = p_units[unit].mutation_flags | MUT_CHAOS_DEMON_WINGS;
        break;
    case 1:
        p_units[unit].mutation_flags = p_units[unit].mutation_flags | MUT_CHAOS_FIRE_BREATH;
        break;
    case 2:
        p_units[unit].mutation_flags = p_units[unit].mutation_flags | MUT_CHAOS_DEMON_SKIN;
        break;
    }
}

/* (player): Chaos Channels on a unit; the human is refused fantastic or already mutated units (message 29) */
int Chaos_Channels_Cast(int player)
{
    int picked;
    int retry;
    int unit;
    int ty;
    int tp;
    int sx;
    int sy;

    if (player == PLAYER_HUMAN) {
        retry = 0;
        do {
            if (retry == 1) {
                Map_Full_Redraw();
                LBX_Load_Records_Near("message", 0, message_text, 29, 1, 150);
                Message_Box(message_text);
            }
            retry = 1;
            picked = Spell_Target_Map_Select(MAP_TARGET_OWN_UNIT, &unit, &ty, &tp, &sx, &sy, "Chaos Channels");
        } while (picked == 1 && ((unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) ||
                            (p_units[unit].mutation_flags & (MUT_CHAOS_DEMON_SKIN | MUT_CHAOS_DEMON_WINGS | MUT_CHAOS_FIRE_BREATH | MUT_UNDEAD))));
    } else {
        picked = AI_Pick_Unit_Enchant_Target(MAP_TARGET_OWN_UNIT, &unit, SPELL_CHAOS_CHANNELS, player);
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN) {
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            ai_replan = 1;
            Spell_Load_Sound(SPELL_CHAOS_CHANNELS);
            Spell_Load_Anim(SPELL_CHAOS_CHANNELS);
            Map_Spell_Effect_Anim(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
            Map_Full_Redraw();
            Heap_Release(p_scratch);
        }
        Chaos_Channels_Apply(unit);
    }
    return picked;
}

/* (player): Warp Node on an enemy-owned node (messages 30 already warped, 31 unowned, 32 own) */
int Warp_Node_Cast(int player)
{
    int picked;
    int node;
    int ty;
    int tp;
    int sx;
    int sy;
    int valid;

    if (player == PLAYER_HUMAN) {
        Minimap_Alloc_Pic();
        Heap_Mark(p_scratch);
        valid = 0;
        picked = 1;
        while (valid == 0 && picked == 1) {
            picked = Spell_Target_Map_Select(MAP_TARGET_NODE, &node, &ty, &tp, &sx, &sy, "Warp Node");
            if (picked == 1) {
                if (p_nodes[node].meld_flags & NODE_WARPED) {
                    LBX_Load_Records_Near("message", 0, message_text, 30, 1, 150);
                    Message_Box(message_text);
                } else if (p_nodes[node].owner == PLAYER_NONE) {
                    LBX_Load_Records_Near("message", 0, message_text, 31, 1, 150);
                    Message_Box(message_text);
                } else if (p_nodes[node].owner == PLAYER_HUMAN) {
                    LBX_Load_Records_Near("message", 0, message_text, 32, 1, 150);
                    Message_Box(message_text);
                } else {
                    valid = 1;
                }
            }
        }
    } else {
        picked = AI_Target_Warp_Node(&node, SPELL_WARP_NODE);
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || p_nodes[node].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_nodes[node].plane][p_nodes[node].y][p_nodes[node].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            Spell_Load_Sound(SPELL_WARP_NODE);
            Spell_Load_Anim(SPELL_WARP_NODE);
            Map_Spell_Effect_Anim(p_nodes[node].x, p_nodes[node].y, p_nodes[node].plane);
            Heap_Release(p_scratch);
        }
        p_nodes[node].meld_flags = p_nodes[node].meld_flags | NODE_WARPED;
        Map_Full_Redraw();
    }
    return picked;
}

/* (player): Stasis on every unit of a tile (the human must pick a stack not all in Stasis); a city there gets its
   protection check
   Numbered: BUG #34 (BUGS_AND_QUIRKS.txt) */
int Stasis_Cast(int player)
{
    int picked;
    int i;
    int tx;
    int ty;
    int tp;
    int sx;
    int sy;
    int has_human;                              /* set when a human unit is hit, never read */
    int valid;
    int found;

    has_human = 0;
    if (player == PLAYER_HUMAN) {
        valid = 0;
        picked = 1;
        while (picked == 1 && valid == 0) {
            picked = Spell_Target_Map_Select(MAP_TARGET_ENEMY_STACK, &tx, &ty, &tp, &sx, &sy, "Stasis");
            if (picked == 1) {
                found = 0;
                if ((p_units[tx].mutation_flags & (MUT_STASIS_NEW | MUT_STASIS)) == 0) {
                    valid = 1;
                } else {
                    for (i = 0; i < num_units && found == 0; i++) {
                        if (p_units[i].x == p_units[tx].x && p_units[i].y == p_units[tx].y &&
                            p_units[i].plane == p_units[tx].plane) {
                            if ((p_units[i].mutation_flags & (MUT_STASIS_NEW | MUT_STASIS)) == 0)
                                valid = 1;
                            found = 1;
                        }
                    }
                }
            }
        }
        ty = p_units[tx].y;
        tp = p_units[tx].plane;
        tx = p_units[tx].x;
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_ENEMY_STACK, &tx, &ty, &tp, SPELL_STASIS, player);
    }
    if (picked == 1) {
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].x == tx && p_cities[i].y == ty && p_cities[i].plane == tp)
                picked = City_Spell_Protection_Check(SPELL_STASIS, i, player, 1);
        }
        if (picked == 1) {
            Minimap_Alloc_Pic();
            ai_replan = 1;
            Heap_Mark(p_scratch);
            p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, BATTLE_UNIT_PARAS));
            for (i = 0; i < num_units; i++) {
                if (p_units[i].x == tx && p_units[i].y == ty && p_units[i].plane == tp) {
                    if (p_units[i].owner == PLAYER_HUMAN)
                        has_human = 1;
                    p_units[i].mutation_flags = p_units[i].mutation_flags | MUT_STASIS_NEW;
                    p_units[i].moves_left = 0;
                    p_units[i].finished = 1;
                }
            }
            Heap_Release(p_scratch);
        }
        /* BUG #34: tx is the map x by now (p_units[tx] is some other unit; has_human is set but not used), and the
           animation plays when the square is NOT visible - the other spells test Tile_Visible() true. clarified
           using ReMoM repo */
        if (player == PLAYER_HUMAN || p_units[tx].owner == PLAYER_HUMAN || !Tile_Visible(tx, ty, tp)) {
            Spell_Load_Sound(SPELL_STASIS);
            Spell_Load_Anim(SPELL_STASIS);
            Map_Spell_Effect_Anim(tx, ty, tp);
            Map_Full_Redraw();
        }
    }
    return picked;
}

/* (player): Nature's Cures - heals every unit on the tile but the Death creatures (RACE_DEATH_CREATURE) and the
   undead (MUT_UNDEAD)
   Numbered: BUG #34 (BUGS_AND_QUIRKS.txt) */
int Natures_Cures_Cast(int player)
{
    int i;
    int tx;
    int ty;
    int tp;
    int sx;
    int sy;
    int picked;

    if (player == PLAYER_HUMAN)
        picked = Spell_Target_Map_Select(MAP_TARGET_OWN_STACK, &tx, &ty, &tp, &sx, &sy, "Nature's Cures");
    else
        picked = AI_Overland_Spell_Target(MAP_TARGET_OWN_STACK, &tx, &ty, &tp, SPELL_NATURES_CURES, player);
    /* BUG #34: also for the AI, whose tx is already a map x (Plane_Shift_Cast converts only the human's); the
       visibility test below then indexes p_units with the converted map x */
    ty = p_units[tx].y;
    tp = p_units[tx].plane;
    tx = p_units[tx].x;
    if (picked == 1) {
        if (player == PLAYER_HUMAN || Tile_Visible(p_units[tx].x, p_units[tx].y, p_units[tx].plane)) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            Spell_Load_Sound(SPELL_NATURES_CURES);
            Spell_Load_Anim(SPELL_NATURES_CURES);
            Map_Spell_Effect_Anim(tx, ty, tp);
            Map_Full_Redraw();
            Heap_Release(p_scratch);
        }
        for (i = 0; i < num_units; i++) {
            if (p_units[i].x == tx && p_units[i].y == ty && p_units[i].plane == tp &&
                unit_types[p_units[i].type].race != RACE_DEATH_CREATURE && (p_units[i].mutation_flags & MUT_UNDEAD) == 0)
                p_units[i].damage = 0;
        }
    }
    return picked;
}

/* (player): Move Fortress to an own city (message 33: the fortress is there); the summoning circle follows when it
   was at the fortress; the home race becomes the city's */
int Move_Fortress_Cast(int player)
{
    int city;
    int ty;
    int tp;
    int sx;
    int sy;
    int ok;
    int valid;

    if (player == PLAYER_HUMAN) {
        valid = 0;
        ok = 1;
        while (valid == 0 && ok == 1) {
            ok = Spell_Target_Map_Select(MAP_TARGET_OWN_CITY, &city, &ty, &tp, &sx, &sy, "Move Fortress");
            if (ok == 1) {
                if (p_cities[city].x == p_fortresses->x && p_fortresses->y == p_cities[city].y &&   /* p_fortresses[0]: the
                                                   human's, the only caster here */
                    p_fortresses->plane == p_cities[city].plane) {
                    LBX_Load_Records_Near("message", 0, message_text, 33, 1, 150);
                    Message_Box(message_text);
                } else {
                    valid = 1;
                }
            }
        }
    } else {
        ok = AI_Pick_City_Spell_Target(MAP_TARGET_OWN_CITY, &city, SPELL_MOVE_FORTRESS, player);
    }
    if (ok == 1) {
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            ai_replan = 1;
            CitySpell_Cast_Anim(city, SPELL_MOVE_FORTRESS, player);
        }
        if (p_fortresses[player].x == wizards[player].summoning_circle_xyp[XYP_X] &&
            p_fortresses[player].y == wizards[player].summoning_circle_xyp[XYP_Y] &&
            p_fortresses[player].plane == wizards[player].summoning_circle_xyp[XYP_PLANE]) {
            wizards[player].summoning_circle_xyp[XYP_X] = p_cities[city].x;
            wizards[player].summoning_circle_xyp[XYP_Y] = p_cities[city].y;
            wizards[player].summoning_circle_xyp[XYP_PLANE] = p_cities[city].plane;
        }
        p_fortresses[player].x = p_cities[city].x;
        p_fortresses[player].y = p_cities[city].y;
        p_fortresses[player].plane = p_cities[city].plane;
        wizards[player].home_race = p_cities[city].race;
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            CitySpell_View_Loop(city, SPELL_MOVE_FORTRESS, player);
            Heap_Release(p_scratch);
        }
    }
    return ok;
}

/* (player): Earthquake on a city - the effect, the relation penalty; the city view shows the old population until
   it closes */
int Earthquake_Cast(int player)
{
    int city;
    int ty;
    int tp;
    int sx;
    int sy;
    int ok;
    int item_count;
    int pop_before;
    int pop_after;
    int items[18];

    if (player == PLAYER_HUMAN)
        ok = Spell_Target_Map_Select(MAP_TARGET_ENEMY_CITY, &city, &ty, &tp, &sx, &sy, "Earthquake");
    else
        ok = AI_Pick_City_Spell_Target(MAP_TARGET_ENEMY_CITY, &city, SPELL_EARTHQUAKE, player);
    if (ok == 1) {
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            CitySpell_Cast_Anim(city, SPELL_EARTHQUAKE, player);
        }
        pop_before = p_cities[city].population;
        Earthquake_Effect(city, &item_count, items);
        pop_after = p_cities[city].population;
        p_cities[city].population = pop_before;
        Hostile_Spell_Relations(player, SPELL_EARTHQUAKE, city);
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            CitySpell_View_Loop(city, SPELL_EARTHQUAKE, player);
            Heap_Release(p_scratch);
        }
        p_cities[city].population = pop_after;
        award_at_location = 1;
        award_loc_x = p_cities[city].x;
        award_loc_y = p_cities[city].y;
        award_loc_plane = p_cities[city].plane;
        Hero_Items_Redistribute(p_cities[city].owner, item_count, items);
    }
    return ok;
}

/* (city, &item_count, items): Earthquake's effect (also the earthquake event) - each garrison unit that neither flies nor
   is non-corporeal dies on Random(4) == 1 (the dead heroes' items go to items), buildings 15% collateral damage;
   returns the buildings lost */
int Earthquake_Effect(int city, int *item_count, int *items)
{
    int i;
    int j;
    int unit;
    int gcount;
    int lost;
    int garrison[MAX_STACK];
    int destroyed[NUM_BUILDINGS];

    ai_replan = 1;
    Heap_Mark(p_scratch);
    p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, BATTLE_UNIT_PARAS));
    City_Garrison_List(city, &gcount, garrison);
    *item_count = 0;
    for (i = 0; i < gcount; i++) {
        Battle_Unit_Init(garrison[i], p_battle_units);
        if ((p_battle_units->move_flags & MOVE_FLYING) || (p_battle_units->ability_flags & AFLAG_NON_CORPOREAL)) {
        } else if (Random(4) == 1) {
            unit = garrison[i];
            if (p_units[unit].hero_slot > -1) {
                for (j = 0; j < 3; j++) {
                    if (wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[j] > -1) {
                        items[*item_count] = wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[j];
                        *item_count = *item_count + 1;
                    }
                    wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[j] = -1;
                }
            }
            Unit_Kill_Or_Dismiss(garrison[i], UNIT_REMOVE_KILLED);
        }
    }
    Heap_Release(p_scratch);
    for (i = 0; i < NUM_BUILDINGS; i++)
        destroyed[i] = 0;
    City_Collateral_Damage(city, 0, 15, destroyed);
    lost = 0;
    for (i = 0; i < NUM_BUILDINGS; i++) {
        if (destroyed[i] > 0)
            lost++;
    }
    if (p_cities[city].owner == PLAYER_HUMAN) {
        for (j = 0; j < NUM_BUILDINGS; j++) {
            if (destroyed[j] > 0 && report_buildings_lost_count < MAX_REPORT_ENTRIES) {
                report_buildings_lost[report_buildings_lost_count].city = city;
                report_buildings_lost[report_buildings_lost_count].building = destroyed[j];
                report_buildings_lost_count++;
            }
        }
    }
    return lost;
}

/* (target): Nature's Wrath strikes the caster of a Chaos or Death spell - the announcement with its music, then in
   every city of the target each garrison unit that neither flies nor is non-corporeal dies at 15% (Random(100) <= 15), buildings 5%
   collateral damage; the dead heroes' items are handed out per city
   role name corrected to ReMoM's reading (10/17 audit); clarified using ReMoM repo */
void Natures_Wrath_Strike(int target)
{
    int i;
    int j;
    int city;                                   /* also the announcement's frame counter */
    int gcount;
    int item_count;
    int garrison[MAX_STACK];
    int destroyed[NUM_BUILDINGS];
    int items[18];

    ai_replan = 1;
    Sound_Stop();
    music_jingle = LBX_Reload("music", MUSIC_GLOBAL_ENCH_BASE + GLOBAL_ENCH_NATURES_WRATH, p_music_buf);
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    strcpy(message_text, "Nature's Wrath strikes ");
    strcat(message_text, target == PLAYER_HUMAN ? "you" : wizards[target].wizard_name);
    Minimap_Alloc_Pic();
    p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, BATTLE_UNIT_PARAS));
    p_anim_pic = LBX_Reload_Next("Specfx", 23, p_scratch);
    diplo_mirror_pic = LBX_Reload_Next("BACKGRND", 18, p_scratch);
    Spell_Load_Sound(SPELL_EARTHQUAKE);
    if (p_sfx_buf != -1)
        Sound_Play(p_sfx_buf);
    Set_Draw_Page();
    Map_Invalidate();
    Map_Redraw();
    Draw_Pic_Frame(58, 33, diplo_mirror_pic);
    Draw_Pic_Frame(68, 41, p_anim_pic);
    Set_Text_Shadow_Color(0);
    Font_Select_Outline(5, 0, 0, 0);
    Paragraph_Print(45, 166, 152, message_text, PARA_CENTER);
    Screen_Save_A800();
    Screen_Present();
    UI_Clear_Controls();
    i = UI_Add_Region(0, 0, SCREEN_MAX_X, SCREEN_MAX_Y, "", -1);   /* the full-screen click; its id is never read */
    for (city = 0; city < 120 && UI_Get_Input() == 0; city++) {   /* 120 frames, the first 16 the wipe */
        if (city < 16) {
            Transition_Wipe_Left(Random(4) + 1, Random(3) + 1);
        } else {
            Set_Draw_Page();
            Map_Invalidate();
            Map_Redraw();
            Draw_Pic_Frame(58, 33, diplo_mirror_pic);
            Draw_Pic_Frame(68, 41, p_anim_pic);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(5, 0, 0, 0);
            Paragraph_Print(45, 166, 152, message_text, PARA_CENTER);
        }
        Screen_Present();
    }
    for (city = 0; city < num_cities; city++) {
        item_count = 0;
        if (p_cities[city].owner == target) {
            City_Garrison_List(city, &gcount, garrison);
            for (i = 0; i < gcount; i++) {
                Battle_Unit_Init(garrison[i], p_battle_units);
                if ((p_battle_units->move_flags & MOVE_FLYING) || (p_battle_units->ability_flags & AFLAG_NON_CORPOREAL)) {
                } else if (Random(100) <= 15) {
                    if (p_units[garrison[i]].hero_slot > -1) {
                        for (j = 0; j < 3; j++) {
                            if (wizards[p_units[garrison[i]].owner].hired_heroes[p_units[garrison[i]].hero_slot].item_index[j] > -1) {
                                items[item_count] =
                                    wizards[p_units[garrison[i]].owner].hired_heroes[p_units[garrison[i]].hero_slot].item_index[j];
                                item_count++;
                            }
                            wizards[p_units[garrison[i]].owner].hired_heroes[p_units[garrison[i]].hero_slot].item_index[j] = 0;   /* BUG #472: 0, not -1 (none) - as Death Wish */
                        }
                    }
                    Unit_Kill_Or_Dismiss(garrison[i], UNIT_REMOVE_KILLED);
                }
            }
            for (i = 0; i < NUM_BUILDINGS; i++)
                destroyed[i] = 0;
            City_Collateral_Damage(city, 0, 5, destroyed);
            if (p_cities[city].owner == PLAYER_HUMAN) {
                for (j = 0; j < NUM_BUILDINGS; j++) {
                    if (destroyed[j] > 0 && report_buildings_lost_count < MAX_REPORT_ENTRIES) {
                        report_buildings_lost[report_buildings_lost_count].city = city;
                        report_buildings_lost[report_buildings_lost_count].building = destroyed[j];
                        report_buildings_lost_count++;
                    }
                }
            }
        }
        award_at_location = 1;
        award_loc_x = p_cities[city].x;
        award_loc_y = p_cities[city].y;
        award_loc_plane = p_cities[city].plane;
        Hero_Items_Redistribute(target, item_count, items);
    }
    Sound_Stop();
    Music_Play_Current();
}
