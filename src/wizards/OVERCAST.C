/* OVERCAST.C - WIZARDS.EXE overlay code segment ba2c (stub 25cf): casting a finished overland spell - the counter
   checks (Suppress Magic, Tranquility, Life Force), then by effect type: summoning, unit damage, unit and city
   enchantments with their refusal messages, item creation, city curses, global enchantments, the special spells,
   wizard-targeted spells (Spell Blast, Cruel Unminding, Drain Power, Subversion), Disenchant Area / True, Disjunction
   and Nature's Wrath; plus the city / global enchantment slot to spell maps, the dispel resistance multiplier,
   Disenchant Area's overland effect and its target test.
   This module's data (DS:6a42..6b49): its literals only; BSS DS:c9f0..c9f9 (owner not fixed by the EXE, see below).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS ba2c src/wizards/OVERCAST.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int CitySlot_To_Spell(int slot);
int Spell_Dispel_Resistance(int strength, int player, int realm);
void Disenchant_Area_Overland(int x, int y, int plane, int player, int strength);
int Tile_Has_Dispel_Target(int x, int y, int plane);
int Overland_Cast_Spell(int player);
int GlobalSlot_To_Spell(int slot);

#include "GAME.H"

/* BSS DS:c9f0..c9f9 (owner not fixed by the EXE: the gap between HIRE.C and CASTUI.C, all of whose other modules have no
   BSS; this module is the main writer of ai_replan and starts the Spell of Mastery) */
int overcast_unused_c9f8;                       /* DS:c9f8  never referenced */
int mastery_caster;                             /* DS:c9f6  the Spell of Mastery's caster again */
                                                /* the same as mastery_winner, stored only on the human's path, for the
                                                   win animation (Mastery_Win_Load) */
unsigned mastery_lab_pic;                       /* DS:c9f4  WIZLAB 0x13 */
int overcast_unused_c9f2;                       /* DS:c9f2  never referenced */
int ai_replan;                                  /* DS:c9f0  1: a spell / event changed the world (AI re-plans) */

extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern char message_text[];                     /* DS:c7fc */
extern int ui_spell;                            /* DS:ca16 */

/* Screen_Disjunction (CASTUI.C), Spell_Ward_Cast, Spell_Binding_Cast, Great_Unsummoning_Cast and Death_Wish_Cast
   (OVERSPL3.C) are int functions with no return value, called here undeclared: their result is whatever AX
   they leave (BUG #408) */
void far_strcpy(char *dst, int dst_seg, char far *src);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Random(int n);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
void Message_Box(char *text);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Minimap_Alloc_Pic(void);
void Map_Full_Redraw(void);
int Spell_Fizzle_Message_Overland(int victim, int blocker, int spell, char *blocker_spell);
void Spell_Of_Mastery_Complete(int caster);
void Floating_Island_Cast(int player);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Stack_Limit_Enforce(int unit);
void Summon_Complete_Anim(int summoned, int music_tier, int wizard);
void Select_Stack_Of_Unit();                    /* no prototype here: called with no argument (MAINSCR.C takes 1) */
int Spell_Target_Map_Select(int target_type, int *target, int *y, int *plane, int *screen_x, int *screen_y,
                            char *spell_name);
int AI_Overland_Spell_Target(int target_kind, int *x, int *y, int *plane, int spell, int caster);
int AI_Pick_Unit_Enchant_Target(int target_kind, int *unit, int spell, int caster);
int AI_Pick_City_Spell_Target(int target_kind, int *city, int spell, int caster);
int AI_Pick_Wizard_Target(int *target, int spell, int caster);
int AI_Pick_Global_Enchant_Target(int *wizard, int *slot, int spell, int caster);
int City_Spell_Protection_Check(int spell, int city, int caster, int show);
int Tile_Visible(int x, int y, int plane);
int Spell_Load_Anim(int spell);
void Spell_Load_Sound(int spell);
void Map_Spell_Effect_Anim(int x, int y, int plane);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
void Spell_Damage_Tile(int unit, int spell);
int Unit_Is_Normal_Living(int unit);
int Unit_Calc_Level(int unit);
int CitySpell_Cast_Anim(int city, int spell, int wizard);
void CitySpell_View_Loop();                     /* no prototype here: called with 3 arguments (CASTANIM.C takes 2) */
int Map_Explore_Radius(int x, int y, int plane, int range);
void City_Consecration_Turn(int city);
int Item_Alloc_Copy(int src_item);
void Items_Screen_Award(int count, int *items);
void Hero_Items_Redistribute(int player, int count, int *items);
void Hostile_Spell_Relations(int caster, int spell, int city);
void GlobalEnch_Complete_Anim(int wizard, int spell);
void Nature_Awareness_Reveal(int player);
void Holy_Arms_Clear_Holy_Weapon(int player);
void Wizard_Reveal_All_Cities(int player);
void Global_Enchant_Relations(int caster, int spell, int divisor);
int Spell_Of_Return_Cast(int player);
int Corruption_Cast(int player);
int Earth_Lore_Cast(int player);
int Change_Terrain_Cast(int player);
int Stasis_Cast(int player);
int Natures_Cures_Cast(int player);
int Move_Fortress_Cast(int player);
int Transmute_Cast(int player);
int Wall_Of_Stone_Cast(int player);
int Earthquake_Cast(int player);
int Word_Of_Recall_Cast(int player);
int Enchant_Road_Cast(int player);
int Chaos_Channels_Cast(int player);
int Warp_Node_Cast(int player);
int Raise_Volcano_Cast(int player);
int Call_The_Void_Cast(int player);
int Plane_Shift_Cast(int player);
int Resurrection_Cast(int player);
int Incarnation_Cast(int player);
int Black_Wind_Cast(int player);
int Lycanthropy_Cast(int player);
int Summon_Hero_Cast(int player, int mode);
int Screen_Wizard_Target(int spell);
int Spell_Casting_Cost(int player, int spell, int in_combat);
int Cruel_Unminding_Apply(int wizard);
int Drain_Power_Apply(int player);
void Subversion_Apply(int victim);
void Natures_Wrath_Strike(int target);

/* (slot) -> the city enchantment spell of a city enchantment slot: the first spell of effect type SPELL_FX_CITY_ENCH
   or _CITY_CURSE whose effect value is the slot (slots 9-13: SPELL_SPELL_WARD, never searched) */
int CitySlot_To_Spell(int slot)
{
    int spell;
    int i;

    spell = SPELL_NONE;
    if (slot >= 9 && slot <= 13)
        spell = SPELL_SPELL_WARD;
    for (i = 1; i < NUM_SPELLS && spell == SPELL_NONE; i++) {
        if ((p_spell_data[i].effect_type == SPELL_FX_CITY_ENCH || p_spell_data[i].effect_type == SPELL_FX_CITY_CURSE) &&
            (int)p_spell_data[i].effect_param == slot)
            spell = i;
    }
    return spell;
}

/* (strength, player, realm) -> the dispel resistance of a spell: strength times 1, +1 with the realm's mastery
   retort, +1 with Archmage (none for player -1) */
int Spell_Dispel_Resistance(int strength, int player, int realm)
{
    int mult;

    mult = 1;
    if (player != PLAYER_NONE) {
        switch (realm) {
        case REALM_CHAOS:
            if (wizards[player].retorts[RETORT_CHAOS_MASTERY] > 0)
                mult++;
            break;
        case REALM_NATURE:
            if (wizards[player].retorts[RETORT_NATURE_MASTERY] > 0)
                mult++;
            break;
        case REALM_SORCERY:
            if (wizards[player].retorts[RETORT_SORCERY_MASTERY] > 0)
                mult++;
            break;
        default:
            break;
        }
        if (wizards[player].retorts[RETORT_ARCHMAGE] > 0)
            mult++;
    }
    return strength * mult;
}

/* (x, y, plane, player, strength) Disenchant Area / True overland: a node under Warp Node (NODE_WARPED)
   on the square, each city
   enchantment not the caster's, each enemy unit's enchantment (bits 0..30 - never Invulnerability, bit 31 -; Spell Lock first: while it
   holds, nothing else) and
   the Stasis of the caster's own units roll (long)strength * 250 / (resistance + strength) against Random(250) (the resistance from the spell's
   base cost - x 5 for a unit enchantment); the
   human sees the first four dispelled named, the fifth as "and more"
   Numbered: BUG #33, #279, #306 (BUGS_AND_QUIRKS.txt) */
void Disenchant_Area_Overland(int x, int y, int plane, int player, int strength)
{
    int k;
    int spell;
    unsigned char far *ench;
    int chance;
    int shown;
    unsigned pic_unit;
    unsigned pic_city;
    long bit;
    int ench_spell[32];                         /* [UNIT_ENCH_* bit number] -> the spell that casts it (SpellId;
                                                   tools/make_enums.py reads this table) */
    int i;

    shown = 0;
    pic_unit = LBX_Reload_Next("SpEcFx", 51, p_scratch);
    pic_city = LBX_Reload_Next("SpEcFx", 52, p_scratch);
    ench_spell[0] = SPELL_IMMOLATION;
    ench_spell[1] = SPELL_GUARDIAN_WIND;
    ench_spell[2] = SPELL_BERSERK;
    ench_spell[3] = SPELL_CLOAK_OF_FEAR;
    ench_spell[4] = SPELL_BLACK_CHANNELS;
    ench_spell[5] = SPELL_WRAITH_FORM;
    ench_spell[6] = SPELL_REGENERATION;
    ench_spell[7] = SPELL_PATH_FINDING;
    ench_spell[8] = SPELL_WATER_WALKING;
    ench_spell[9] = SPELL_RESIST_ELEMENTS;
    ench_spell[10] = SPELL_ELEMENTAL_ARMOR;
    ench_spell[11] = SPELL_STONE_SKIN;
    ench_spell[12] = SPELL_IRON_SKIN;
    ench_spell[13] = SPELL_ENDURANCE;
    ench_spell[14] = SPELL_SPELL_LOCK;
    ench_spell[15] = SPELL_INVISIBILITY;
    ench_spell[16] = SPELL_WIND_WALKING;
    ench_spell[17] = SPELL_FLIGHT;
    ench_spell[18] = SPELL_RESIST_MAGIC;
    ench_spell[19] = SPELL_MAGIC_IMMUNITY;
    ench_spell[20] = SPELL_FLAME_BLADE;
    ench_spell[21] = SPELL_ELDRITCH_WEAPON;
    ench_spell[22] = SPELL_TRUE_SIGHT;
    ench_spell[23] = SPELL_HOLY_WEAPON;
    ench_spell[24] = SPELL_HEROISM;
    ench_spell[25] = SPELL_BLESS;
    ench_spell[26] = SPELL_LIONHEART;
    ench_spell[27] = SPELL_GIANT_STRENGTH;
    ench_spell[28] = SPELL_PLANAR_TRAVEL;
    ench_spell[29] = SPELL_HOLY_ARMOR;
    ench_spell[30] = SPELL_RIGHTEOUSNESS;
    ench_spell[31] = SPELL_INVULNERABILITY;
    for (i = 0; i < NUM_NODES; i++) {
        if ((p_nodes[i].meld_flags & NODE_WARPED) > 0 && p_nodes[i].x == x && p_nodes[i].y == y && p_nodes[i].plane == plane) {
            chance = Spell_Dispel_Resistance(p_spell_data[SPELL_WARP_NODE].base_cost, PLAYER_NONE, REALM_CHAOS) + strength;
            chance = (long)strength * 250 / chance;
            if (Random(250) <= chance)
                p_nodes[i].meld_flags = p_nodes[i].meld_flags ^ NODE_WARPED;
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane) {
            ench = (unsigned char far *)p_cities[i].enchants;
            for (k = 0; k < NUM_CITY_ENCHANTS; k++) {
                if (ench[k] > 0 && ench[k] != player + 1) {
                    spell = CitySlot_To_Spell(k);
                    chance = Spell_Dispel_Resistance(p_spell_data[spell].base_cost, ench[k] - 1,
                                                     p_spell_data[spell].realm) + strength;
                    chance = (long)strength * 250 / chance;
                    if (Random(250) <= chance) {
                        ench[k] = 0;
                        if (player == PLAYER_HUMAN && shown < 5) {
                            if (shown < 4) {
                                far_strcpy(message_text, 0, p_spell_data[spell].name);
                                strcat(message_text, " has been dispelled.");
                            } else {
                                LBX_Load_Records_Near("message", 0, message_text, 18, 1, 150);
                            }
                            Notify_Picture_Box(160, shown * 25 + 40, 2, message_text, 0, pic_city, 0, 8, 0, 0, 0, 1, 0);
                            shown++;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < num_units; i++) {
        if (p_units[i].x == x && p_units[i].y == y && p_units[i].plane == plane) {
            if (p_units[i].owner != player) {
                if (p_units[i].enchantments & UNIT_ENCH_SPELL_LOCK) {
                    /* 150: Spell Lock's fixed dispel cost */
                    chance = Spell_Dispel_Resistance(150, p_units[i].owner, REALM_SORCERY) + strength;
                    chance = (long)strength * 250 / chance;
                    if (Random(250) <= chance)
                        p_units[i].enchantments = p_units[i].enchantments ^ UNIT_ENCH_SPELL_LOCK;
                } else {
                    for (k = 0; k < 31; k++) {
                        bit = 1L << k;
                        if ((p_units[i].enchantments & bit) > 0) {
                            chance = Spell_Dispel_Resistance(p_spell_data[ench_spell[k]].base_cost * 5, p_units[i].owner,
                                                             p_spell_data[ench_spell[k]].realm) + strength;
                            chance = (long)strength * 250 / chance;
                            if (Random(250) <= chance) {
                                p_units[i].enchantments = p_units[i].enchantments ^ bit;
                                if (player == PLAYER_HUMAN && shown < 5) {
                                    if (shown < 4) {
                                        far_strcpy(message_text, 0, p_spell_data[ench_spell[k]].name);
                                        strcat(message_text, " has been dispelled.");
                                    } else {
                                        LBX_Load_Records_Near("message", 0, message_text, 19, 1, 150);
                                    }
                                    Notify_Picture_Box(160, shown * 25 + 40, 2, message_text, 0, pic_unit, 4, 8, 0, 0, 0, 1,
                                                       0);
                                    shown++;
                                }
                            }
                        }
                    }
                }
            } else {
                if (p_units[i].mutation_flags & (MUT_STASIS_NEW | MUT_STASIS)) {
                    chance = Spell_Dispel_Resistance(p_spell_data[SPELL_STASIS].base_cost, p_units[i].owner,
                                                     p_spell_data[SPELL_STASIS].realm) + strength;
                    chance = (long)strength * 250 / chance;
                    if (Random(250) <= chance) {
                        if (p_units[i].mutation_flags == MUT_STASIS_NEW)
                            p_units[i].mutation_flags = p_units[i].mutation_flags ^ MUT_STASIS_NEW;
                        else
                            p_units[i].mutation_flags = p_units[i].mutation_flags ^ MUT_STASIS;
                        if (player == PLAYER_HUMAN) {
                            LBX_Load_Records_Near("message", 0, message_text, 20, 1, 150);
                            Notify_Picture_Box(160, shown * 25 + 40, 2, message_text, 0, pic_unit, 4, 8, 0, 0, 0, 1, 0);
                        }
                    }
                }
            }
        }
    }
}

/* (x, y, plane) -> 1 when a city, a warped node or a unit is on the square (Disenchant Area's target) */
int Tile_Has_Dispel_Target(int x, int y, int plane)
{
    int i;
    int found;

    found = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        for (i = 0; i < NUM_NODES; i++) {
            if ((p_nodes[i].meld_flags & NODE_WARPED) > 0 && p_nodes[i].x == x && p_nodes[i].y == y && p_nodes[i].plane == plane) {
                found = 1;
                break;
            }
        }
    }
    if (found == 0) {
        for (i = 0; i < num_units; i++) {
            if (p_units[i].x == x && p_units[i].y == y && p_units[i].plane == plane) {
                found = 1;
                break;
            }
        }
    }
    return found;
}

/* (player) -> 0 when nothing is being cast; casts the player's finished spell (wizards[player].casting_spell):
   a Suppress Magic of another wizard (and Tranquility for chaos, Life Force for death spells) may counter it
   (Random(250) <= 125000 / (resistance + 500)); otherwise by effect type - the human picks targets on the map
   (refused targets explain why and ask again), the AI through its target pickers. Nature's Wrath of another
   wizard strikes back at chaos and death spells. The casting fields are cleared at the end (no value returned)
   Numbered: BUG #158, #213, #247 (BUGS_AND_QUIRKS.txt) */
int Overland_Cast_Spell(int player)
{
    unsigned char far *p_ench;
    int value;                                  /* by turns: the counter chance, an enchantment slot, the disjoined spell */
    int i;
    int target;                                 /* per spell: a unit, a city, a wizard, the map x, or a found flag */
    int target_y;                               /* target_y / _plane: the target square (target its x); a global
                                                   enchantment's slot in target_y */
    int target_plane;
    int screen_y;
    int screen_x;
    int ok;
    int flag;                                   /* by turns: the summon music tier, the target-retry flag, the dispel-target flag,
                                                   the human-watches flag, the Nature's Wrath wizard (-1 none) */
    int n;                                      /* the forged item, later the disjunction chance */
    int cost;
    int is_human;
    char name[20];
    int spell;

    if (player == PLAYER_HUMAN)
        is_human = 1;
    else
        is_human = 0;
    ok = 1;
    ai_replan = 0;
    spell = wizards[player].casting_spell;
    if (spell == SPELL_NONE)
        return 0;
    far_strcpy(name, 0, p_spell_data[spell].name);
    for (i = 0; i < num_players && ok == 1; i++) {
        if (wizards[i].globals.suppress_magic > 0 && i != player) {
            value = Spell_Dispel_Resistance(wizards[player].casting_cost_initial, player, p_spell_data[spell].realm) +
                    500;
            value = 125000L / value;
            if (Random(250) <= value) {
                Spell_Fizzle_Message_Overland(player, i, spell, "Suppress Magic");
                ok = 0;
            }
        }
    }
    if (p_spell_data[spell].realm == REALM_CHAOS && ok == 1) {
        for (i = 0; i < num_players && ok == 1; i++) {
            if (wizards[i].globals.tranquility > 0 && i != player) {
                value = Spell_Dispel_Resistance(wizards[player].casting_cost_initial, player,
                                                p_spell_data[spell].realm) + 500;
                value = 125000L / value;
                if (Random(250) <= value) {
                    Spell_Fizzle_Message_Overland(player, i, spell, "Tranquility");
                    ok = 0;
                }
            }
        }
    }
    if (p_spell_data[spell].realm == REALM_DEATH && ok == 1) {
        for (i = 0; i < num_players && ok == 1; i++) {
            if (wizards[i].globals.life_force > 0 && i != player) {
                value = Spell_Dispel_Resistance(wizards[player].casting_cost_initial, player,
                                                p_spell_data[spell].realm) + 500;
                value = 125000L / value;
                if (Random(250) <= value) {
                    Spell_Fizzle_Message_Overland(player, i, spell, "Life Force");
                    ok = 0;
                }
            }
        }
    }
    if (ok == 1) {
        ui_spell = spell;
        if (spell == SPELL_SPELL_OF_MASTERY) {
            Spell_Of_Mastery_Complete(player);
            ai_replan = 1;
        } else {
            switch (p_spell_data[spell].effect_type) {
            case SPELL_FX_SUMMON:
                if (num_units < UNIT_CAP && (player == PLAYER_HUMAN || num_units < UNIT_CAP_AI)) {
                    if (spell == SPELL_FLOATING_ISLAND) {
                        ai_replan = 1;
                        Floating_Island_Cast(player);
                    } else {
                        Unit_Create((int)p_spell_data[spell].effect_param, player, wizards[player].summoning_circle_xyp[XYP_X],
                                    wizards[player].summoning_circle_xyp[XYP_Y], wizards[player].summoning_circle_xyp[XYP_PLANE], UNIT_SRC_NONE);
                        Stack_Limit_Enforce(num_units - 1);
                    }
                    flag = SPELL_SLOT(spell);
                    flag = flag / SPELL_SLOT_UNCOMMON;   /* 10 slots a rarity: the jingle tier 0 .. 3 */
                    if (spell != SPELL_FLOATING_ISLAND && (player == PLAYER_HUMAN || (game_config.enemy_spells == 1 &&
                        wizards[PLAYER_HUMAN].contacted[player] == 1 && wizards[PLAYER_HUMAN].globals.detect_magic > 0))) {
                        Summon_Complete_Anim((int)p_spell_data[spell].effect_param, flag, player);
                        ai_replan = 1;
                    }
                    p_units[num_units - 1].finished = 0;
                    p_units[num_units - 1].moves_left = p_units[num_units - 1].moves_max;
                    if (player == PLAYER_HUMAN) {
                        next_unit_ref_x = p_units[num_units - 1].x;
                        next_unit_ref_y = p_units[num_units - 1].y;
                        map_view_plane = p_units[num_units - 1].plane;
                        Select_Stack_Of_Unit();
                    }
                    ok = 1;
                } else {
                    if (player == PLAYER_HUMAN) {
                        LBX_Load_Records_Near("message", 0, message_text, 21, 1, 150);
                        Message_Box(message_text);
                    }
                    ok = 0;
                }
                break;
            case SPELL_FX_DAMAGE:
                if (player == PLAYER_HUMAN)
                    ok = Spell_Target_Map_Select(MAP_TARGET_ENEMY_STACK, &target, &target_y, &target_plane, &screen_x, &screen_y, name);
                else
                    ok = AI_Overland_Spell_Target(MAP_TARGET_ENEMY_STACK, &target, &target_y, &target_plane, spell, player);
                if (ok == 1) {
                    for (i = 0; i < num_cities; i++) {
                        if (p_cities[i].x == p_units[target].x && p_cities[i].y == p_units[target].y &&
                            p_cities[i].plane == p_units[target].plane)
                            ok = City_Spell_Protection_Check(spell, i, player, is_human);
                    }
                }
                if (ok == 1) {
                    if (player == PLAYER_HUMAN || p_units[target].owner == PLAYER_HUMAN ||
                        (Tile_Visible(p_units[target].x, p_units[target].y, p_units[target].plane) &&
                         game_config.enemy_spells == 1)) {
                        ai_replan = 1;
                        Minimap_Alloc_Pic();
                        Spell_Load_Anim(spell);
                        Spell_Load_Sound(spell);
                        Map_Spell_Effect_Anim(p_units[target].x, p_units[target].y, p_units[target].plane);
                        Map_Full_Redraw();
                    }
                    Diplo_Relation_Change(-3, player, p_units[target].owner, DIPLO_MSG_UNPROVOKED_DESTRUCTION, 0, target);   /* the victim: -3 */
                    Spell_Damage_Tile(target, spell);
                }
                break;
            case SPELL_FX_UNIT_ENCH:
            case SPELL_FX_NORMAL_UNIT_ENCH:
                if (player == PLAYER_HUMAN) {
                    flag = 0;
                    ok = 1;
                    while (flag == 0 && ok == 1) {
                        flag = 1;
                        ok = Spell_Target_Map_Select(MAP_TARGET_OWN_UNIT, &target, &target_y, &target_plane, &screen_x, &screen_y, name);
                        if (ok == 1) {
                            if (p_units[target].enchantments & p_spell_data[spell].effect_param) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                strcpy(message_text, "That unit already has ");
                                strcat(message_text, name);
                                strcat(message_text, " cast on it");
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_STONE_SKIN && (p_units[target].enchantments & UNIT_ENCH_IRON_SKIN)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 22, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_RESIST_ELEMENTS && (p_units[target].enchantments & UNIT_ENCH_ELEMENTAL_ARMOR)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 23, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_RESIST_MAGIC && (p_units[target].enchantments & UNIT_ENCH_WRAITH_FORM)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 24, 1, 150);
                                Message_Box(message_text);
                            }
                            if ((p_spell_data[spell].effect_type == SPELL_FX_NORMAL_UNIT_ENCH || spell == SPELL_CHAOS_CHANNELS) &&
                                Unit_Is_Normal_Living(target) == 0) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                strcpy(message_text, "Only normal units may have ");
                                strcat(message_text, name);
                                strcat(message_text, " cast on them");
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_IMMOLATION && ((char)unit_types[p_units[target].type].abilities2 & ABILITY2_IMMOLATION)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_CLOAK_OF_FEAR && ((char)unit_types[p_units[target].type].abilities2 & ABILITY2_CAUSE_FEAR_SPELL)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_TRUE_SIGHT && (unit_types[p_units[target].type].abilities & ABILITY_ILLUSIONS_IMMUNITY)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_MAGIC_IMMUNITY && (unit_types[p_units[target].type].abilities & ABILITY_MAGIC_IMMUNITY)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_PLANAR_TRAVEL && (unit_types[p_units[target].type].ability_flags & AFLAG_PLANE_SHIFT)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_INVISIBILITY && (unit_types[p_units[target].type].ability_flags & AFLAG_INVISIBILITY)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_WRAITH_FORM && (unit_types[p_units[target].type].ability_flags & AFLAG_NON_CORPOREAL)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_WIND_WALKING && (unit_types[p_units[target].type].ability_flags & AFLAG_WIND_WALKING)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_REGENERATION && (unit_types[p_units[target].type].ability_flags & AFLAG_REGENERATION)) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 87, 1, 150);
                                Message_Box(message_text);
                            }
                            if (spell == SPELL_BLACK_CHANNELS && Unit_Is_Normal_Living(target) == 0 &&
                                (p_units[target].mutation_flags & MUT_UNDEAD) == 0) {
                                flag = 0;
                                Minimap_Alloc_Pic();
                                Map_Full_Redraw();
                                strcpy(message_text, "Only normal units and created undead may have ");
                                strcat(message_text, name);
                                strcat(message_text, " cast on them");
                                Message_Box(message_text);
                            }
                        }
                    }
                } else
                    ok = AI_Pick_Unit_Enchant_Target(MAP_TARGET_OWN_UNIT, &target, spell, player);
                if (ok == 1) {
                    if (player == PLAYER_HUMAN) {
                        Minimap_Alloc_Pic();
                        Spell_Load_Sound(spell);
                        Spell_Load_Anim(spell);
                        Map_Spell_Effect_Anim(p_units[target].x, p_units[target].y, p_units[target].plane);
                        Map_Full_Redraw();
                    }
                    p_units[target].enchantments = p_units[target].enchantments | p_spell_data[spell].effect_param;
                    if (spell == SPELL_HEROISM)
                        p_units[target].level = Unit_Calc_Level(target);
                    if (spell == SPELL_BLACK_CHANNELS)
                        p_units[target].mutation_flags = p_units[target].mutation_flags | MUT_UNDEAD;
                }
                break;
            case SPELL_FX_CITY_ENCH:
                if (player == PLAYER_HUMAN) {
                    flag = 0;
                    ok = 1;
                    while (flag == 0 && ok == 1) {
                        flag = 1;
                        ok = Spell_Target_Map_Select(MAP_TARGET_OWN_CITY, &target, &target_y, &target_plane, &screen_x, &screen_y, name);
                        if (ok == 1) {
                            if (spell == SPELL_SUMMONING_CIRCLE) {
                                if (p_cities[target].x == wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_X] &&
                                    p_cities[target].y == wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_Y] &&
                                    p_cities[target].plane == wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_PLANE]) {
                                    flag = 0;
                                    Map_Full_Redraw();
                                    LBX_Load_Records_Near("message", 0, message_text, 25, 1, 150);
                                    Message_Box(message_text);
                                }
                            } else {
                                p_ench = (unsigned char far *)p_cities[target].enchants;
                                if (p_ench[(int)p_spell_data[spell].effect_param] > 0) {
                                    flag = 0;
                                    Map_Full_Redraw();
                                    strcpy(message_text, "That city already has ");
                                    strcat(message_text, name);
                                    strcat(message_text, " cast on it");
                                    Message_Box(message_text);
                                }
                            }
                        }
                    }
                } else
                    ok = AI_Pick_City_Spell_Target(MAP_TARGET_OWN_CITY, &target, spell, player);
                if (ok == 1) {
                    if (player == PLAYER_HUMAN || p_cities[target].owner == PLAYER_HUMAN ||
                        (game_config.enemy_spells == 1 &&
                         p_exploration_maps[p_cities[target].plane][p_cities[target].y][p_cities[target].x] != 0 &&
                         wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
                        ai_replan = 1;
                        CitySpell_Cast_Anim(target, spell, player);
                    }
                    if (spell == SPELL_SUMMONING_CIRCLE) {
                        wizards[player].summoning_circle_xyp[XYP_X] = p_cities[target].x;
                        wizards[player].summoning_circle_xyp[XYP_Y] = p_cities[target].y;
                        wizards[player].summoning_circle_xyp[XYP_PLANE] = p_cities[target].plane;
                    } else {
                        p_ench = (unsigned char far *)p_cities[target].enchants;
                        value = (int)p_spell_data[spell].effect_param;
                        p_ench[value] = player + 1;
                    }
                    if (player == PLAYER_HUMAN || p_cities[target].owner == PLAYER_HUMAN ||
                        (game_config.enemy_spells == 1 &&
                         p_exploration_maps[p_cities[target].plane][p_cities[target].y][p_cities[target].x] != 0 &&
                         wizards[PLAYER_HUMAN].globals.detect_magic > 0))
                        CitySpell_View_Loop(target, spell, player);
                    if (player == PLAYER_HUMAN && spell == SPELL_NATURES_EYE)
                        Map_Explore_Radius(p_cities[target].x, p_cities[target].y, p_cities[target].plane, 5);
                    if (spell == SPELL_CONSECRATION)
                        City_Consecration_Turn(target);
                }
                break;
            case SPELL_FX_CREATE_ITEM:
                if (player == PLAYER_HUMAN) {
                    Summon_Complete_Anim(SUMMON_ITEM, MUSIC_TIER_VERY_RARE, player);
                    n = Item_Alloc_Copy(ITEM_REC_FORGE);
                    Items_Screen_Award(1, &n);
                } else {
                    n = Item_Alloc_Copy(ITEM_REC_WORK - player);   /* the AI's own work record, 135..132 */
                    Hero_Items_Redistribute(player, 1, &n);
                }
                current_screen = SCREEN_MAP;
                ok = 1;
                break;
            case SPELL_FX_CITY_CURSE:
                if (player == PLAYER_HUMAN) {
                    flag = 0;
                    ok = 1;
                    while (flag == 0 && ok == 1) {
                        ok = Spell_Target_Map_Select(MAP_TARGET_ENEMY_CITY, &target, &target_y, &target_plane, &screen_x, &screen_y, name);
                        if (ok == 1) {
                            p_ench = (unsigned char far *)p_cities[target].enchants;
                            if (p_ench[(int)p_spell_data[spell].effect_param] > 0) {
                                Map_Full_Redraw();
                                strcpy(message_text, "That city already has ");
                                strcat(message_text, name);
                                strcat(message_text, " cast on it");
                                Message_Box(message_text);
                            } else if (spell == SPELL_EVIL_PRESENCE && p_cities[target].owner < num_players &&
                                       wizards[p_cities[target].owner].spellbooks[REALM_DEATH] > 0) {
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 26, 1, 150);
                                Message_Box(message_text);
                            } else
                                flag = 1;
                        }
                    }
                } else
                    ok = AI_Pick_City_Spell_Target(MAP_TARGET_ENEMY_CITY, &target, spell, player);
                if (ok == 1)
                    ok = City_Spell_Protection_Check(spell, target, player, is_human);
                if (ok == 1) {
                    if (player == PLAYER_HUMAN || p_cities[target].owner == PLAYER_HUMAN ||
                        (game_config.enemy_spells == 1 &&
                         p_exploration_maps[p_cities[target].plane][p_cities[target].y][p_cities[target].x] != 0 &&
                         wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
                        ai_replan = 1;
                        Minimap_Alloc_Pic();
                        Heap_Mark(p_scratch);
                        CitySpell_Cast_Anim(target, spell, player);
                    }
                    p_ench = (unsigned char far *)p_cities[target].enchants;
                    value = (int)p_spell_data[spell].effect_param;
                    p_ench[value] = player + 1;
                    if (player == PLAYER_HUMAN || p_cities[target].owner == PLAYER_HUMAN ||
                        (game_config.enemy_spells == 1 &&
                         p_exploration_maps[p_cities[target].plane][p_cities[target].y][p_cities[target].x] != 0 &&
                         wizards[PLAYER_HUMAN].globals.detect_magic > 0))
                        CitySpell_View_Loop(target, spell, player);
                }
                Hostile_Spell_Relations(player, spell, target);
                break;
            case SPELL_FX_GLOBAL_ENCH:
                ai_replan = 1;
                GlobalEnch_Complete_Anim(player, spell);
                ok = 1;
                p_ench = (unsigned char far *)&wizards[player].globals;
                value = (int)p_spell_data[spell].effect_param;
                p_ench[value] = player + 1;
                if (spell == SPELL_TIME_STOP)
                    time_stop_player = player + 1;
                if (spell == SPELL_NATURE_AWARENESS)
                    Nature_Awareness_Reveal(player);
                if (spell == SPELL_HOLY_ARMS)
                    Holy_Arms_Clear_Holy_Weapon(player);
                if (spell == SPELL_CRUSADE) {
                    for (i = 0; i < num_units; i++) {
                        if (p_units[i].owner == player)
                            p_units[i].level = Unit_Calc_Level(i);
                    }
                }
                if (spell == SPELL_AWARENESS)
                    Wizard_Reveal_All_Cities(player);
                Global_Enchant_Relations(player, spell, 1);
                break;
            case SPELL_FX_SPECIAL:
                ai_replan = 1;
                if (spell == SPELL_SPELL_OF_RETURN)
                    Spell_Of_Return_Cast(player);
                if (spell == SPELL_CORRUPTION)
                    ok = Corruption_Cast(player);
                if (spell == SPELL_EARTH_LORE)
                    ok = Earth_Lore_Cast(player);
                if (spell == SPELL_CHANGE_TERRAIN)
                    ok = Change_Terrain_Cast(player);
                if (spell == SPELL_STASIS)
                    ok = Stasis_Cast(player);
                if (spell == SPELL_NATURES_CURES)
                    ok = Natures_Cures_Cast(player);
                if (spell == SPELL_MOVE_FORTRESS)
                    ok = Move_Fortress_Cast(player);
                if (spell == SPELL_TRANSMUTE)
                    ok = Transmute_Cast(player);
                if (spell == SPELL_WALL_OF_STONE)
                    ok = Wall_Of_Stone_Cast(player);
                if (spell == SPELL_EARTHQUAKE)
                    ok = Earthquake_Cast(player);
                if (spell == SPELL_WORD_OF_RECALL)
                    ok = Word_Of_Recall_Cast(player);
                if (spell == SPELL_ENCHANT_ROAD)
                    ok = Enchant_Road_Cast(player);
                if (spell == SPELL_SPELL_WARD)
                    ok = Spell_Ward_Cast(player);
                if (spell == SPELL_SPELL_BINDING)
                    ok = Spell_Binding_Cast(player);
                if (spell == SPELL_CHAOS_CHANNELS)
                    ok = Chaos_Channels_Cast(player);
                if (spell == SPELL_WARP_NODE)
                    ok = Warp_Node_Cast(player);
                if (spell == SPELL_RAISE_VOLCANO)
                    ok = Raise_Volcano_Cast(player);
                if (spell == SPELL_CALL_THE_VOID)
                    ok = Call_The_Void_Cast(player);
                if (spell == SPELL_PLANE_SHIFT)
                    ok = Plane_Shift_Cast(player);
                if (spell == SPELL_RESURRECTION)
                    ok = Resurrection_Cast(player);
                if (spell == SPELL_INCARNATION)
                    ok = Incarnation_Cast(player);
                if (spell == SPELL_GREAT_UNSUMMONING)
                    ok = Great_Unsummoning_Cast(player);
                if (spell == SPELL_DEATH_WISH)
                    ok = Death_Wish_Cast(player);
                if (spell == SPELL_BLACK_WIND)
                    ok = Black_Wind_Cast(player);
                if (spell == SPELL_LYCANTHROPY)
                    ok = Lycanthropy_Cast(player);
                if (spell == SPELL_SUMMON_HERO)
                    ok = Summon_Hero_Cast(player, HPICK_POOL_BASIC);
                if (spell == SPELL_SUMMON_CHAMPION)
                    ok = Summon_Hero_Cast(player, HPICK_POOL_CHAMPIONS);
                break;
            case SPELL_FX_VS_WIZARD:
            case SPELL_FX_VS_WIZARD_VAR:
                if (player == PLAYER_HUMAN) {
                    target = 0;
                    for (i = 1; i < num_players; i++) {
                        if (wizards[PLAYER_HUMAN].contacted[i] == 1 && p_fortresses[i].active == 1)
                            target = 1;
                    }
                    if (target == 1) {
                        ok = 1;
                        target = Screen_Wizard_Target(spell);
                    } else {
                        ok = 0;
                        LBX_Load_Records_Near("message", 0, message_text, 27, 1, 150);
                        far_strcpy(name, 0, p_spell_data[spell].name);
                        strcat(message_text, name);
                        strcat(message_text, " spell has failed.");
                        Message_Box(message_text);
                    }
                } else {
                    ok = AI_Pick_Wizard_Target(&target, spell, player);
                    if (ok == 1) {
                        switch ((int)p_spell_data[spell].effect_param) {
                        case WIZSPELL_SPELL_BLAST:
                            cost = p_spell_data[wizards[target].casting_spell].base_cost -   /* the human's Spell Blast
                                                   (CASTANIM.C) charges casting_cost_initial - cost_left instead */
                                   wizards[target].casting_cost_left;
                            if (wizards[player].mana_reserve >= cost) {
                                if (target == PLAYER_HUMAN)
                                    Spell_Fizzle_Message_Overland(PLAYER_HUMAN, player, wizards[PLAYER_HUMAN].casting_spell, "Spell Blast");
                                wizards[player].mana_reserve -= cost;
                                if (wizards[target].casting_spell != SPELL_SPELL_OF_RETURN) {
                                    wizards[target].casting_spell = 0;
                                    wizards[target].casting_cost_left = 0;
                                    wizards[target].casting_cost_initial = 0;
                                } else
                                    wizards[target].casting_cost_left = Spell_Casting_Cost(target, SPELL_SPELL_OF_RETURN, 0);
                            }
                            break;
                        case WIZSPELL_CRUEL_UNMINDING:
                            Cruel_Unminding_Apply(target);
                            break;
                        case WIZSPELL_DRAIN_POWER:
                            Drain_Power_Apply(target);
                            break;
                        case WIZSPELL_SUBVERSION:
                            Subversion_Apply(target);
                            break;
                        }
                    }
                }
                break;
            case SPELL_FX_DISENCHANT:
                if (player == PLAYER_HUMAN) {
                    flag = 0;
                    ok = 1;
                    Minimap_Alloc_Pic();
                    Heap_Mark(p_scratch);
                    while (flag == 0 && ok == 1) {
                        ok = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &target, &target_y, &target_plane, &screen_x, &screen_y, name);
                        if (ok == 1) {
                            flag = Tile_Has_Dispel_Target(target, target_y, target_plane);
                            if (flag == 0) {
                                Map_Full_Redraw();
                                LBX_Load_Records_Near("message", 0, message_text, 28, 1, 150);
                                Message_Box(message_text);
                            }
                        }
                    }
                } else
                    ok = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &target, &target_y, &target_plane, spell, player);
                if (ok == 1) {
                    flag = 0;
                    if (player == PLAYER_HUMAN)
                        flag = 1;
                    else {
                        for (i = 0; i < num_cities && flag == 0; i++) {
                            if (p_cities[i].x == target && p_cities[i].y == target_y && p_cities[i].plane == target_plane &&
                                (p_cities[i].owner == PLAYER_HUMAN ||
                                 (game_config.enemy_spells == 1 &&
                                  p_exploration_maps[p_cities[i].plane][p_cities[i].y][p_cities[i].x] != 0 &&
                                  wizards[PLAYER_HUMAN].globals.detect_magic > 0))) {
                                ai_replan = 1;
                                flag = 1;
                                Minimap_Alloc_Pic();
                                Heap_Mark(p_scratch);
                            }
                        }
                    }
                    for (i = 0; i < num_units && flag == 0; i++) {
                        if (p_units[i].x == target && p_units[i].y == target_y && p_units[i].plane == target_plane &&
                            (p_units[i].owner == PLAYER_HUMAN ||
                             (Tile_Visible(p_units[i].x, p_units[i].y, p_units[i].plane) &&
                              game_config.enemy_spells == 1 && wizards[PLAYER_HUMAN].globals.detect_magic > 0))) {
                            flag = 1;
                            ai_replan = 1;
                            Minimap_Alloc_Pic();
                            Heap_Mark(p_scratch);
                        }
                    }
                    if (flag == 1) {
                        ai_replan = 1;
                        Spell_Load_Sound(spell);
                        Spell_Load_Anim(spell);
                        Map_Spell_Effect_Anim(target, target_y, target_plane);
                        Map_Full_Redraw();
                        Heap_Release(p_scratch);
                    }
                    if (wizards[player].retorts[RETORT_RUNEMASTER] > 0)   /* the dispel strength: the mana paid, x 2
                                                   Runemaster, x 3 for the True version */
                        wizards[player].casting_cost_initial *= 2;
                    if (spell == SPELL_DISENCHANT_TRUE)
                        wizards[player].casting_cost_initial *= 3;
                    Disenchant_Area_Overland(target, target_y, target_plane, player, wizards[player].casting_cost_initial);
                }
                break;
            case SPELL_FX_DISJUNCTION:
                if (player == PLAYER_HUMAN)
                    ok = Screen_Disjunction(spell, player);
                else {
                    ok = AI_Pick_Global_Enchant_Target(&target, &target_y, spell, player);
                    if (ok == 1) {
                        if (wizards[player].retorts[RETORT_RUNEMASTER] > 0)
                            wizards[player].casting_cost_initial *= 2;
                        if (spell == SPELL_DISJUNCTION_TRUE)
                            wizards[player].casting_cost_initial *= 3;
                        value = GlobalSlot_To_Spell(target_y);
                        n = Spell_Dispel_Resistance(p_spell_data[value].base_cost, target, p_spell_data[value].realm) +
                            wizards[player].casting_cost_initial;
                        n = (long)wizards[player].casting_cost_initial * 250 / n;
                        if (Random(250) <= n) {
                            p_ench = (unsigned char far *)&wizards[target].globals;
                            p_ench[(int)p_spell_data[value].effect_param] = 0;
                            Spell_Fizzle_Message_Overland(target, player, value, "Disjunction");
                        }
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    if (ok == 1) {
        flag = -1;                              /* now the Nature's Wrath wizard, -1 none */
        for (i = 0; i < num_players; i++) {
            if (wizards[i].globals.nature_s_wrath > 0 && i != player)
                flag = i;
        }
        if (flag >= 0 && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH)) {
            ai_replan = 1;
            Natures_Wrath_Strike(player);
        }
    }
    wizards[player].casting_cost_left = 0;
    wizards[player].casting_cost_initial = 0;
    wizards[player].casting_spell = 0;
}

/* (slot) -> the global enchantment spell of a GlobalEnchants slot (Eternal Night .. Awareness). The switch has no
   default: a slot past the last would return an uninitialised value (latent: the one caller passes the AI's picked
   slot); clarified using ReMoM repo */
int GlobalSlot_To_Spell(int slot)
{
    int spell;

    switch (slot) {
    case GLOBAL_ENCH_ETERNAL_NIGHT:
        spell = SPELL_ETERNAL_NIGHT;
        break;
    case GLOBAL_ENCH_EVIL_OMENS:
        spell = SPELL_EVIL_OMENS;
        break;
    case GLOBAL_ENCH_ZOMBIE_MASTERY:
        spell = SPELL_ZOMBIE_MASTERY;
        break;
    case GLOBAL_ENCH_AURA_OF_MAJESTY:
        spell = SPELL_AURA_OF_MAJESTY;
        break;
    case GLOBAL_ENCH_WIND_MASTERY:
        spell = SPELL_WIND_MASTERY;
        break;
    case GLOBAL_ENCH_SUPPRESS_MAGIC:
        spell = SPELL_SUPPRESS_MAGIC;
        break;
    case GLOBAL_ENCH_TIME_STOP:
        spell = SPELL_TIME_STOP;
        break;
    case GLOBAL_ENCH_NATURE_AWARENESS:
        spell = SPELL_NATURE_AWARENESS;
        break;
    case GLOBAL_ENCH_NATURES_WRATH:
        spell = SPELL_NATURES_WRATH;
        break;
    case GLOBAL_ENCH_HERB_MASTERY:
        spell = SPELL_HERB_MASTERY;
        break;
    case GLOBAL_ENCH_CHAOS_SURGE:
        spell = SPELL_CHAOS_SURGE;
        break;
    case GLOBAL_ENCH_DOOM_MASTERY:
        spell = SPELL_DOOM_MASTERY;
        break;
    case GLOBAL_ENCH_GREAT_WASTING:
        spell = SPELL_GREAT_WASTING;
        break;
    case GLOBAL_ENCH_METEOR_STORM:
        spell = SPELL_METEOR_STORM;
        break;
    case GLOBAL_ENCH_ARMAGEDDON:
        spell = SPELL_ARMAGEDDON;
        break;
    case GLOBAL_ENCH_TRANQUILITY:
        spell = SPELL_TRANQUILITY;
        break;
    case GLOBAL_ENCH_LIFE_FORCE:
        spell = SPELL_LIFE_FORCE;
        break;
    case GLOBAL_ENCH_CRUSADE:
        spell = SPELL_CRUSADE;
        break;
    case GLOBAL_ENCH_JUST_CAUSE:
        spell = SPELL_JUST_CAUSE;
        break;
    case GLOBAL_ENCH_HOLY_ARMS:
        spell = SPELL_HOLY_ARMS;
        break;
    case GLOBAL_ENCH_PLANAR_SEAL:
        spell = SPELL_PLANAR_SEAL;
        break;
    case GLOBAL_ENCH_CHARM_OF_LIFE:
        spell = SPELL_CHARM_OF_LIFE;
        break;
    case GLOBAL_ENCH_DETECT_MAGIC:
        spell = SPELL_DETECT_MAGIC;
        break;
    case GLOBAL_ENCH_AWARENESS:
        spell = SPELL_AWARENESS;
        break;
    }
    return spell;
}
