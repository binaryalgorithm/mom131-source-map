/* COMBDISP.C - WIZARDS.EXE overlay code segment b1af (stub 25b2): combat spells, second part - Disenchant Area /
   Disenchant True in combat (the enemy combat enchantments, a melded Warp Node, the enemy vortices and city
   enchantments, then every unit's square), Dispel Magic on one square, Cracks Call on one square, the mass spells
   (Flame Strike, Holy Word, Death Spell, Mass Healing, Call Chaos) with their animation, the combat spell
   animation and realm flash, Raise Dead and Animate Dead.
   This module's data (DS:6844..68a0): its literals only; no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS b1af src/wizards/COMBDISP.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Disenchant_Area_Combat(int caster, int strength);
void Dispel_Square(int x, int y, int caster, int strength, int *msgs);
void Cracks_Call_Square(int x, int y);
int Mass_Spell_Animate(int caster_player, int spell, int show_anim, int caster);
void Flame_Strike_All(int caster_player);
void Holy_Word_All(int caster_player);
void Death_Spell_All(int caster_player);
int Combat_Spell_Animate(int x, int y, int spell, int caster_player, int show_anim, int caster);
void Raise_Dead(int caster_player, int caster, int x, int y);
void Animate_Dead(int caster_player, int caster);

#include "GAME.H"

extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern char *combat_click_map[];                /* DS:c524  per grid row: the battle unit at each square, -1 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int *p_dispel_curse_spells;             /* DS:c8a8  combat_flags bit -> spell */
extern int *p_dispel_ench_spells;                /* DS:c8aa  enchantment bit -> spell */
extern struct CombatVortex far *p_vortices;     /* DS:d14e */
extern int vortex_count;                        /* DS:d152 */

int Array_Delete();                             /* unprototyped: called with one far pointer for its (offset, segment) words (UTIL.C) */
void Battle_Apply_Damage(int unit, int *damage);
int Battle_Free_Gfx_Slot(void);
void Battle_Unit_Combat_Modifiers(struct BattleUnit far *bu);
void Battle_Unit_Heal(int bu, int amount, int overflow);
void Battlefield_Draw_Terrain(void);
void Bolt_Spell_Anim(int x, int y, int spell, int caster);
int Call_Chaos_Cast(int caster, int animate);
int CitySlot_To_Spell(int slot);
void Combat_Cast_Banner(int caster, int spell);
void Combat_Draw_Screen(void);
void Combat_Grid_To_Screen(int gx, int gy, int ox, int oy, int *sx, int *sy);
void Combat_Load_Spell_Sound(int spell);
void Combat_Summon_Anim(int unit, int x, int y, int spell, int caster);
void Cracks_Call_Anim(int x, int y, int caster);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
int Is_City_Wall_Square(int x, int y);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Lightning_Strike_Anim(int x, int y, int caster);
int List_Select_Dialog(int count, char **names, int more, char *title);
int Load_Unit_Figure_Frames(int unit_type, int slot);
void Near_Alloc_Mark(void);
void Near_Alloc_Release(void);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
void Palette_Tint_Upload(int percent, int r, int g, int b);
int Pic_Frame_Count(unsigned pic);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
void Screen_Copy_Front(void);
void Set_Pic_Frame(unsigned pic, unsigned frame);
int Sound_Play(unsigned sound);
void Spell_Anim_On_Square(int x, int y, int big, int caster, int spell);
int Spell_Load_Anim(int spell);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
int Spell_Dispel_Resistance(int strength, int player, int realm);
void Unit_Apply_Modifiers(struct BattleUnit far *bu);

/* (caster: battle unit < 20 or 20 + player, strength) Disenchant Area / True in combat: each enemy combat
   enchantment, a Warp Node melded by the caster's side, each enemy vortex and each city enchantment not the
   caster's rolls (long)strength * 250 / (its resistance + strength) against Random(250); the first four dispelled
   are named, the fifth is "and more"; then Dispel_Square on every living unit's square.
   BUG #195: a unit caster's owner is read with i before i is set. The city enchantments are p_battlefield's copy
   (BFGEN.C), which no code copies back: a city enchantment dispelled here seems to last only for the battle
   (PROVISIONAL, RC5); walls cast in this battle are not in the copy and cannot be dispelled here; slot 25 (the
   Nightshade count) is rolled as an enchantment with spell 0 ("None has been dispelled."). clarified using ReMoM
   repo
   Numbered: BUG #195, #290, #299, #300 (BUGS_AND_QUIRKS.txt) */
void Disenchant_Area_Combat(int caster, int strength)
{
    int slot;                                   /* the side 0 / 1 into p_combat_enchants, then the city enchantment slot */
    int j;
    int spell;
    unsigned char far *city_ench;
    int shown;
    unsigned pic;
    int owner;
    int enemy;
    int ench_spell[15];
    int i;
    int chance;

    ench_spell[0] = SPELL_TRUE_LIGHT;
    ench_spell[1] = SPELL_DARKNESS;
    ench_spell[2] = SPELL_WARP_REALITY;
    ench_spell[3] = SPELL_BLACK_PRAYER;
    ench_spell[4] = SPELL_WRACK;
    ench_spell[5] = SPELL_METAL_FIRES;
    ench_spell[6] = SPELL_PRAYER;
    ench_spell[7] = SPELL_HIGH_PRAYER;
    ench_spell[8] = SPELL_TERROR;
    ench_spell[9] = SPELL_CALL_LIGHTNING;
    ench_spell[10] = SPELL_COUNTER_MAGIC;
    ench_spell[11] = SPELL_MASS_INVISIBILITY;
    ench_spell[12] = SPELL_ENTANGLE;
    ench_spell[13] = SPELL_MANA_LEAK;
    ench_spell[14] = SPELL_BLUR;
    owner = caster > CASTER_WIZARD - 1 ? caster - CASTER_WIZARD : p_battle_units[i].owner;
    shown = 0;
    Screen_Copy_Front();
    if (owner == combat_attacker_player) {
        slot = 1;
        enemy = combat_defender_player;
    } else {
        slot = 0;
        enemy = combat_attacker_player;
    }
    if (enemy != PLAYER_NEUTRAL) {
        for (i = 0; i < NUM_COMBAT_ENCHANTS; i += 2) {
            if (p_combat_enchants[i + slot] > 0) {
                spell = ench_spell[i / 2];
                chance = Spell_Dispel_Resistance(p_spell_data[spell].base_cost, enemy, p_spell_data[spell].realm) + strength;
                chance = (long)strength * 250 / chance;
                if (Random(250) <= chance) {
                    if (i == CENCH_MASS_INVISIBILITY_ATT) {
                        for (j = 0; j < battle_unit_count; j++) {
                            if (p_battle_units[j].owner == enemy)
                                p_battle_units[j].combat_enchantments = p_battle_units[j].combat_enchantments ^ UNIT_ENCH_INVISIBILITY;   /* XOR: a unit
                                                   without the bit would gain it */
                        }
                    }
                    p_combat_enchants[i + slot] = 0;
                    Heap_Mark(p_scratch);
                    pic = LBX_Reload_Next("SPECFX", 52, p_scratch);   /* the dispel over a combat enchantment icon */
                    if (shown < 5) {
                        if (shown < 4) {
                            far_strcpy(message_text, 0, p_spell_data[spell].name);
                            strcat(message_text, " has been dispelled.");
                        } else {
                            LBX_Load_Records_Near("message", 0, message_text, 45, 1, 150);
                        }
                        Notify_Picture_Box(160, shown * 25 + 40, 3, message_text, 0, pic, 0, 8, 0, 0, 0, 1, 0);
                        shown++;
                    }
                    Heap_Release(p_scratch);
                }
            }
        }
    }
    for (i = 0; i < NUM_NODES; i++) {
        if ((p_nodes[i].meld_flags & NODE_WARPED) > 0 && p_nodes[i].owner == owner && p_nodes[i].x == combat_loc_x &&
            p_nodes[i].y == combat_loc_y && p_nodes[i].plane == combat_loc_plane) {
            chance = Spell_Dispel_Resistance(p_spell_data[SPELL_WARP_NODE].base_cost, PLAYER_NONE, p_spell_data[SPELL_WARP_NODE].realm) +
                strength;
            chance = (long)strength * 250 / chance;
            if (Random(250) <= chance)
                p_nodes[i].meld_flags = p_nodes[i].meld_flags ^ NODE_WARPED;
        }
    }
    for (i = 0; i < vortex_count; i++) {
        if (p_vortices[i].owner != owner) {
            chance = Spell_Dispel_Resistance(p_spell_data[SPELL_MAGIC_VORTEX].base_cost, p_vortices[i].owner,
                                        p_spell_data[SPELL_MAGIC_VORTEX].realm) + strength;
            chance = (long)strength * 250 / chance;
            if (Random(250) <= chance) {
                Array_Delete(i, p_vortices, 12, vortex_count);
                i--;
                vortex_count--;
            }
        }
    }
    city_ench = (unsigned char far *)p_battlefield->city_enchants;
    for (slot = 0; slot < NUM_CITY_ENCHANTS; slot++) {
        if (city_ench[slot] > 0 && city_ench[slot] != owner + 1) {
            spell = CitySlot_To_Spell(slot);
            chance = Spell_Dispel_Resistance(p_spell_data[spell].base_cost, city_ench[slot] - 1, p_spell_data[spell].realm) +
                strength;
            chance = (long)strength * 250 / chance;
            if (Random(250) <= chance) {
                city_ench[slot] = 0;
                Heap_Mark(p_scratch);
                pic = LBX_Reload_Next("specfx", 52, p_scratch);
                if (spell == SPELL_WALL_OF_FIRE)
                    p_battlefield->wall_of_fire = 0;
                if (spell == SPELL_WALL_OF_DARKNESS)
                    p_battlefield->wall_of_darkness = 0;
                if (shown < 5) {
                    if (shown < 4) {
                        far_strcpy(message_text, 0, p_spell_data[spell].name);
                        strcat(message_text, " has been dispelled.");
                    } else {
                        LBX_Load_Records_Near("message", 0, message_text, 46, 1, 150);
                    }
                    Notify_Picture_Box(160, shown * 25 + 40, 3, message_text, 0, pic, 0, 8, 0, 0, 0, 1, 0);
                    shown++;
                }
                Heap_Release(p_scratch);
            }
        }
    }
    shown = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            Dispel_Square(p_battle_units[i].combat_x, p_battle_units[i].combat_y, caster, strength, &shown);
    }
}

/* (x, y, caster, strength, *msgs: messages shown so far) Dispel Magic on one square: the vortices there, then each
   unit there - an enemy unit's Spell Lock (or else its unit enchantments, at five times the cost, and its combat
   enchantments (not the Invisibility bit of a side under Mass Invisibility, not the Holy Weapon bit while the CASTER's owner has
   Holy Arms), Haste and Confusion); an own unit's harmful
   combat flags (curses) roll at +500. BUG #195: a unit caster's owner is read with i before i is set. No status test:
   dead, fled or recalled units still recorded on the square lose enchantments too (the overland ones, kept after the
   battle); a dispelled Flight toggles MOVE_FLYING (^), so a natural flyer that also had Flight stops flying;
   clarified using ReMoM repo
   Numbered: BUG #195, #218, #219, #262, #279, #300, #306 (BUGS_AND_QUIRKS.txt) */
void Dispel_Square(int x, int y, int caster, int strength, int *msgs)
{
    int owner;
    int vortex;
    int bit;
    long mask;
    int mask16;
    int chance;
    int enemy;
    unsigned pic;
    int i;

    Near_Alloc_Mark();
    p_dispel_ench_spells = (int *)Near_Alloc(0x40);
    p_dispel_curse_spells = (int *)Near_Alloc(0x1e);
    p_dispel_ench_spells[0] = SPELL_IMMOLATION;
    p_dispel_curse_spells[0] = SPELL_VERTIGO;
    p_dispel_ench_spells[1] = SPELL_GUARDIAN_WIND;
    p_dispel_curse_spells[1] = SPELL_CONFUSION;
    p_dispel_ench_spells[2] = SPELL_BERSERK;
    p_dispel_curse_spells[2] = SPELL_NONE;
    p_dispel_ench_spells[3] = SPELL_CLOAK_OF_FEAR;
    p_dispel_curse_spells[3] = SPELL_MIND_STORM;
    p_dispel_ench_spells[4] = SPELL_BLACK_CHANNELS;
    p_dispel_curse_spells[4] = SPELL_SHATTER;
    p_dispel_ench_spells[5] = SPELL_WRAITH_FORM;
    p_dispel_curse_spells[5] = SPELL_WEAKNESS;
    p_dispel_ench_spells[6] = SPELL_REGENERATION;
    p_dispel_curse_spells[6] = SPELL_BLACK_SLEEP;
    p_dispel_ench_spells[7] = SPELL_PATH_FINDING;
    p_dispel_curse_spells[7] = SPELL_WARP_CREATURE;
    p_dispel_ench_spells[8] = SPELL_WATER_WALKING;
    p_dispel_curse_spells[8] = SPELL_WARP_CREATURE;
    p_dispel_ench_spells[9] = SPELL_RESIST_ELEMENTS;
    p_dispel_curse_spells[9] = SPELL_WARP_CREATURE;
    p_dispel_ench_spells[10] = SPELL_ELEMENTAL_ARMOR;
    p_dispel_curse_spells[10] = SPELL_NONE;
    p_dispel_ench_spells[11] = SPELL_STONE_SKIN;
    p_dispel_curse_spells[11] = SPELL_HASTE;
    p_dispel_ench_spells[12] = SPELL_IRON_SKIN;
    p_dispel_curse_spells[12] = SPELL_WEB;
    p_dispel_ench_spells[13] = SPELL_ENDURANCE;
    p_dispel_curse_spells[13] = SPELL_CREATURE_BINDING;
    p_dispel_ench_spells[14] = SPELL_SPELL_LOCK;
    p_dispel_ench_spells[15] = SPELL_INVISIBILITY;
    p_dispel_ench_spells[16] = SPELL_WIND_WALKING;
    p_dispel_ench_spells[17] = SPELL_FLIGHT;
    p_dispel_ench_spells[18] = SPELL_RESIST_MAGIC;
    p_dispel_ench_spells[19] = SPELL_MAGIC_IMMUNITY;
    p_dispel_ench_spells[20] = SPELL_FLAME_BLADE;
    p_dispel_ench_spells[21] = SPELL_ELDRITCH_WEAPON;
    p_dispel_ench_spells[22] = SPELL_TRUE_SIGHT;
    p_dispel_ench_spells[23] = SPELL_HOLY_WEAPON;
    p_dispel_ench_spells[24] = SPELL_HEROISM;
    p_dispel_ench_spells[25] = SPELL_BLESS;
    p_dispel_ench_spells[26] = SPELL_LIONHEART;
    p_dispel_ench_spells[27] = SPELL_GIANT_STRENGTH;
    p_dispel_ench_spells[28] = SPELL_PLANAR_TRAVEL;
    p_dispel_ench_spells[29] = SPELL_HOLY_ARMOR;
    p_dispel_ench_spells[30] = SPELL_RIGHTEOUSNESS;
    p_dispel_ench_spells[31] = SPELL_INVULNERABILITY;
    Screen_Copy_Front();
    owner = caster > CASTER_WIZARD - 1 ? caster - CASTER_WIZARD : p_battle_units[i].owner;
    if (owner == combat_attacker_player)
        enemy = combat_defender_player;
    else
        enemy = combat_attacker_player;
    for (vortex = 0; vortex < vortex_count; vortex++) {
        if (p_vortices[vortex].x == x && p_vortices[vortex].y == y) {
            chance = Spell_Dispel_Resistance(p_spell_data[SPELL_MAGIC_VORTEX].base_cost, p_vortices[vortex].owner,
                                        p_spell_data[SPELL_MAGIC_VORTEX].realm) + strength;
            chance = (long)strength * 250 / chance;
            if (Random(250) <= chance) {
                Array_Delete(vortex, p_vortices, 12, vortex_count);
                vortex--;
                vortex_count--;
            }
        }
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].combat_x == x && p_battle_units[i].combat_y == y) {
            if (p_battle_units[i].owner != owner) {
                if (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_SPELL_LOCK) {
                    chance = Spell_Dispel_Resistance(150 /* Spell Lock's fixed dispel cost */, p_battle_units[i].owner, p_spell_data[SPELL_SPELL_LOCK].realm) +
                        strength;
                    chance = (long)strength * 250 / chance;
                    if (Random(250) <= chance)
                        p_units[p_battle_units[i].unit_index].enchantments =
                            p_units[p_battle_units[i].unit_index].enchantments ^ UNIT_ENCH_SPELL_LOCK;
                } else {
                    for (bit = 0; bit < 31; bit++) {    /* bit 31, Invulnerability, is never tried (here or below) */
                        mask = 1L << bit;
                        if ((p_units[p_battle_units[i].unit_index].enchantments & mask) > 0) {
                            chance = Spell_Dispel_Resistance(p_spell_data[p_dispel_ench_spells[bit]].base_cost * 5,
                                                        p_battle_units[i].owner,
                                                        p_spell_data[p_dispel_ench_spells[bit]].realm) + strength;
                            chance = (long)strength * 250 / chance;
                            if (Random(250) <= chance) {
                                p_units[p_battle_units[i].unit_index].enchantments =
                                    p_units[p_battle_units[i].unit_index].enchantments ^ mask;
                                if (bit == 24)          /* UNIT_ENCH_HEROISM: the level drops back */
                                    p_units[p_battle_units[i].unit_index].level =
                                        Unit_Calc_Level(p_battle_units[i].unit_index);
                                if (bit == 17)          /* UNIT_ENCH_FLIGHT */
                                    p_battle_units[i].move_flags = p_battle_units[i].move_flags ^ MOVE_FLYING;
                                if (*msgs < 5) {
                                    Heap_Mark(p_scratch);
                                    pic = LBX_Reload_Next("specfx", 51, p_scratch);   /* the dispel over a unit */
                                    if (*msgs < 4) {
                                        far_strcpy(message_text, 0, p_spell_data[p_dispel_ench_spells[bit]].name);
                                        strcat(message_text, " has been dispelled.");
                                    } else {
                                        LBX_Load_Records_Near("message", 0, message_text, 19, 1, 150);
                                    }
                                    Notify_Picture_Box(*msgs * 10 + 160, *msgs * 25 + 20, 3, message_text, 0, pic, 0, 8,
                                                       0, 0, 0, 1, 0);
                                    *msgs = *msgs + 1;
                                    Heap_Release(p_scratch);
                                }
                            }
                        }
                    }
                    for (bit = 0; bit < 31; bit++) {
                        mask = 1L << bit;
                        if (enemy == combat_attacker_player) {
                            if (p_combat_enchants[CENCH_MASS_INVISIBILITY_ATT] > 0 && mask == UNIT_ENCH_INVISIBILITY)
                                continue;
                        } else {
                            if (p_combat_enchants[CENCH_MASS_INVISIBILITY_DEF] > 0 && mask == UNIT_ENCH_INVISIBILITY)
                                continue;
                        }
                        if (mask == UNIT_ENCH_HOLY_WEAPON && wizards[owner].globals.holy_arms > 0)   /* the caster's side */
                            continue;
                        if ((p_battle_units[i].combat_enchantments & mask) > 0) {
                            chance = Spell_Dispel_Resistance(p_spell_data[p_dispel_ench_spells[bit]].base_cost,
                                                        p_battle_units[i].owner,
                                                        p_spell_data[p_dispel_ench_spells[bit]].realm) + strength;
                            chance = (long)strength * 250 / chance;
                            if (Random(250) <= chance) {
                                Heap_Mark(p_scratch);
                                pic = LBX_Reload_Next("specfx", 51, p_scratch);
                                p_battle_units[i].combat_enchantments = p_battle_units[i].combat_enchantments ^ mask;
                                if (bit == 24)          /* UNIT_ENCH_HEROISM: the level drops back */
                                    p_units[p_battle_units[i].unit_index].level = Unit_Calc_Level(p_battle_units[i].unit_index);
                                if (bit == 17)          /* UNIT_ENCH_FLIGHT */
                                    p_battle_units[i].move_flags = p_battle_units[i].move_flags ^ MOVE_FLYING;
                                if (*msgs < 5) {
                                    if (*msgs < 4) {
                                        far_strcpy(message_text, 0, p_spell_data[p_dispel_ench_spells[bit]].name);
                                        strcat(message_text, " has been dispelled.");
                                    } else {
                                        LBX_Load_Records_Near("message", 0, message_text, 19, 1, 150);
                                    }
                                    Notify_Picture_Box(*msgs * 10 + 160, *msgs * 25 + 20, 3, message_text, 0, pic, 0, 8, 0,
                                                       0, 0, 1, 0);
                                    *msgs = *msgs + 1;
                                }
                                Heap_Release(p_scratch);
                            }
                        }
                    }
                }
                if (p_battle_units[i].combat_flags & COMBAT_FLAG_HASTE) {
                    chance = Spell_Dispel_Resistance(p_spell_data[SPELL_HASTE].base_cost, p_battle_units[i].owner,
                                                p_spell_data[SPELL_HASTE].realm) + strength;
                    chance = (long)strength * 250 / chance;
                    if (Random(250) <= chance) {
                        Heap_Mark(p_scratch);
                        pic = LBX_Reload_Next("specfx", 51, p_scratch);
                        p_battle_units[i].combat_flags = p_battle_units[i].combat_flags ^ COMBAT_FLAG_HASTE;
                        if (*msgs < 5) {
                            if (*msgs < 4) {
                                far_strcpy(message_text, 0, p_spell_data[SPELL_HASTE].name);
                                strcat(message_text, " has been dispelled.");
                            } else {
                                LBX_Load_Records_Near("message", 0, message_text, 19, 1, 150);
                            }
                            Notify_Picture_Box(*msgs * 10 + 160, *msgs * 25 + 20, 3, message_text, 0, pic, 0, 8, 0, 0,
                                               0, 1, 0);
                            *msgs = *msgs + 1;
                        }
                        Heap_Release(p_scratch);
                    }
                }
                if ((p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION) && p_battle_units[i].confusion_result == CONFUSION_CHANGED_SIDE) {
                    chance = Spell_Dispel_Resistance(p_spell_data[SPELL_CONFUSION].base_cost, p_battle_units[i].owner,
                                                p_spell_data[SPELL_CONFUSION].realm) + strength;
                    chance = (long)strength * 250 / chance;
                    if (Random(250) <= chance) {
                        Heap_Mark(p_scratch);
                        if (p_battle_units[i].owner == combat_attacker_player)
                            p_battle_units[i].owner = combat_defender_player;
                        else
                            p_battle_units[i].owner = combat_attacker_player;
                        pic = LBX_Reload_Next("specfx", 51, p_scratch);
                        p_battle_units[i].combat_flags = p_battle_units[i].combat_flags ^ COMBAT_FLAG_CONFUSION;
                        if (*msgs < 5) {
                            if (*msgs < 4) {
                                far_strcpy(message_text, 0, p_spell_data[SPELL_CONFUSION].name);
                                strcat(message_text, " has been dispelled.");
                            } else {
                                LBX_Load_Records_Near("message", 0, message_text, 19, 1, 150);
                            }
                            Notify_Picture_Box(*msgs * 10 + 160, *msgs * 25 + 20, 3, message_text, 0, pic, 0, 8, 0, 0,
                                               0, 1, 0);
                            *msgs = *msgs + 1;
                        }
                        Heap_Release(p_scratch);
                    }
                }
            } else {
                for (bit = 0; bit < 14; bit++) {
                    mask16 = 1 << bit;
                    if ((p_battle_units[i].combat_flags & mask16) > 0 && mask16 != COMBAT_FLAG_HASTE &&
                        (mask16 != COMBAT_FLAG_CONFUSION || p_battle_units[i].confusion_result != CONFUSION_CHANGED_SIDE)) {
                        chance = Spell_Dispel_Resistance(p_spell_data[p_dispel_curse_spells[bit]].base_cost, enemy,
                                                    p_spell_data[p_dispel_curse_spells[bit]].realm) + strength;
                        chance = (long)strength * 250 / chance;
                        if (Random(250) <= chance + 500) {   /* always true: an own unit's curse always goes */
                            if (mask16 == COMBAT_FLAG_WEB)
                                p_battle_units[i].web_hp = 0;
                            Heap_Mark(p_scratch);
                            pic = LBX_Reload_Next("specfx", 51, p_scratch);
                            p_battle_units[i].combat_flags = p_battle_units[i].combat_flags ^ mask16;
                            if (*msgs < 5) {
                                if (*msgs < 4) {
                                    far_strcpy(message_text, 0, p_spell_data[p_dispel_curse_spells[bit]].name);
                                    strcat(message_text, " has been dispelled.");
                                } else {
                                    LBX_Load_Records_Near("message", 0, message_text, 19, 1, 150);
                                }
                                chance = *msgs * 10 + 160;
                                Notify_Picture_Box(chance, *msgs * 25 + 20, 3, message_text, 0, pic, 0, 8, 0, 0, 0, 1, 0);
                                *msgs = *msgs + 1;
                            }
                            Heap_Release(p_scratch);
                        }
                    }
                }
            }
        }
    }
    Near_Alloc_Release();
    Battlefield_Draw_Terrain();
}

/* (x, y) Cracks Call on one square: each non-flying unit there without Non-Corporeal dies to
   200 damage on a 1 in 4; a city wall square there is broken */
void Cracks_Call_Square(int x, int y)
{
    int dmg[3];
    int wx;
    int wy;
    int i;
    int j;

    for (i = 0; i < battle_unit_count; i++) {
        for (j = 0; j < 3; j++)
            dmg[j] = 0;
        if (p_battle_units[i].combat_x == x && p_battle_units[i].combat_y == y && (p_battle_units[i].move_flags & MOVE_FLYING) == 0 &&
            (p_battle_units[i].ability_flags & AFLAG_NON_CORPOREAL) == 0) {
            j = Random(4);
            if (j == 1) {
                dmg[DAMAGE_DESTROY] = 200;
                Battle_Apply_Damage(i, dmg);
            }
        }
    }
    if (Is_City_Wall_Square(x, y) == 1) {
        wx = x - BF_CITY_X1;
        wy = y - BF_CITY_Y1;
        p_battlefield->wall_segments[wy][wx] = WALL_BROKEN;
    }
}

/* (caster_player, spell, show_anim, caster) the mass spells: Call Chaos casts itself; else the spell's animation on
   every target unit (staggered by up to 4 frames) with the effect applied at the middle frame - Death Spell, Holy
   Word, Flame Strike, Mass Healing (5 hits to each own non-undead unit). BUG #13: without the animation the frame and
   frame count tested are uninitialised
   Numbered: BUG #13 (BUGS_AND_QUIRKS.txt) */
int Mass_Spell_Animate(int caster_player, int spell, int show_anim, int caster)
{
    int frame;
    int nframes;
    int sx;
    int sy;
    int big;
    int delays[36];
    int i;

    if (spell == SPELL_CALL_CHAOS) {
        Call_Chaos_Cast(caster, show_anim);
        return;                                 /* no value: callers ignore the result here */
    }
    if (show_anim == 0) {
        if (spell == SPELL_DEATH_SPELL && (nframes + 4) / 2 == frame)
            Death_Spell_All(caster_player);
        if (spell == SPELL_HOLY_WORD && (nframes + 4) / 2 == frame)
            Holy_Word_All(caster_player);
        if (spell == SPELL_FLAME_STRIKE && (nframes + 4) / 2 == frame)
            Flame_Strike_All(caster_player);
        if (spell == SPELL_MASS_HEALING && (nframes + 4) / 2 == frame) {
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].owner == caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                    p_battle_units[i].race != RACE_DEATH_CREATURE)
                    Battle_Unit_Heal(i, 5, 0);
            }
        }
        return 0;
    }
    Heap_Mark(p_scratch);
    big = Spell_Load_Anim(spell);
    Combat_Load_Spell_Sound(spell);
    nframes = Pic_Frame_Count(p_anim_pic);
    if (p_sfx_buf != -1)
        Sound_Play(p_sfx_buf);
    for (i = 0; i < battle_unit_count; i++) {
        if (((p_battle_units[i].owner != caster_player && spell != SPELL_MASS_HEALING) ||
             (p_battle_units[i].owner == caster_player && spell == SPELL_MASS_HEALING)) &&
            p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            delays[i] = Random(5) - 1;
        else
            delays[i] = -1;
    }
    for (frame = 0; frame < nframes + 4; frame++) {
        Timer_Mark();
        if ((nframes + 4) / 2 == frame) {
            if (spell == SPELL_DEATH_SPELL)
                Death_Spell_All(caster_player);
            if (spell == SPELL_HOLY_WORD)
                Holy_Word_All(caster_player);
            if (spell == SPELL_FLAME_STRIKE)
                Flame_Strike_All(caster_player);
            if (spell == SPELL_MASS_HEALING) {
                for (i = 0; i < battle_unit_count; i++) {
                    if (p_battle_units[i].owner == caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                        p_battle_units[i].race != RACE_DEATH_CREATURE)
                        Battle_Unit_Heal(i, 5, 0);
                }
            }
        }
        Combat_Draw_Screen();
        Combat_Cast_Banner(caster, spell);
        for (i = 0; i < battle_unit_count; i++) {
            if (delays[i] > -1) {
                Combat_Grid_To_Screen(p_battle_units[i].combat_x, p_battle_units[i].combat_y, 4, 4, &sx, &sy);
                if (big == 0) {
                    sx -= 14;
                    sy -= 21;
                } else {
                    sx -= 28;
                    sy -= 30;
                }
                if (frame - delays[i] < nframes && frame - delays[i] >= 0) {
                    Set_Pic_Frame(p_anim_pic, frame - delays[i]);
                    Draw_Pic_Clipped(sx, sy, p_anim_pic);
                }
            }
        }
        Screen_Present();
        Wait_Ticks(2);
    }
    Heap_Release(p_scratch);
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
}

/* (caster_player) Flame Strike: the spell's damage to every living enemy unit without Wraith Form or
   Righteousness
   Numbered: BUG #247 (BUGS_AND_QUIRKS.txt) */
void Flame_Strike_All(int caster_player)
{
    int dmg[3];
    long ench;
    int i;

    for (i = 0; i < battle_unit_count; i++) {
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (p_battle_units[i].owner != caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (ench & UNIT_ENCH_WRAITH_FORM) == 0 &&
            (ench & UNIT_ENCH_RIGHTEOUSNESS) == 0) {
            Spell_Damage_Roll(SPELL_FLAME_STRIKE, i, dmg, 0);
            Battle_Apply_Damage(i, dmg);
        }
    }
}

/* (caster_player) Holy Word: each figure of every living enemy fantastic unit (race 15 and up) without
   Wraith Form or Spell Lock resists at -2 (-7 when undead) or dies
   Numbered: BUG #247 (BUGS_AND_QUIRKS.txt) */
void Holy_Word_All(int caster_player)
{
    int n;
    long ench;
    int modifier;
    int dmg[3];
    register int i;
    register int j;

    dmg[DAMAGE_NORMAL] = 0;
    dmg[DAMAGE_DRAIN] = 0;
    dmg[DAMAGE_DESTROY] = 0;
    for (i = 0; i < battle_unit_count; i++) {
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (p_battle_units[i].owner != caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (ench & UNIT_ENCH_WRAITH_FORM) == 0 &&
            (ench & UNIT_ENCH_SPELL_LOCK) == 0 && (char)p_battle_units[i].race >= RACE_ARCANE_CREATURE) {
            modifier = -2;
            if (p_units[p_battle_units[i].unit_index].mutation_flags & MUT_UNDEAD)
                modifier += -5;
            n = p_battle_units[i].figures_alive;
            for (j = 0; j < n; j++) {
                dmg[DAMAGE_DESTROY] = 0;
                if (Roll_Resist(p_battle_units[i], modifier, REALM_LIFE) > 0) {
                    dmg[DAMAGE_DESTROY] = p_battle_units[i].hits_per_figure;
                    Battle_Apply_Damage(i, dmg);
                }
            }
        }
    }
}

/* (caster_player) Death Spell: each figure of every living enemy unit without Wraith Form or Death Immunity
   resists at -2 or dies
   Numbered: BUG #247 (BUGS_AND_QUIRKS.txt) */
void Death_Spell_All(int caster_player)
{
    int n;
    long ench;
    int dmg[3];
    int i;
    int j;

    dmg[DAMAGE_NORMAL] = 0;
    dmg[DAMAGE_DRAIN] = 0;
    dmg[DAMAGE_DESTROY] = 0;
    for (i = 0; i < battle_unit_count; i++) {
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (p_battle_units[i].owner != caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (ench & UNIT_ENCH_WRAITH_FORM) == 0 &&
            (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY) == 0) {
            n = p_battle_units[i].figures_alive;
            for (j = 0; j < n; j++) {
                dmg[DAMAGE_NORMAL] = 0;
                if (Roll_Resist(p_battle_units[i], -2, REALM_DEATH) > 0) {
                    dmg[DAMAGE_NORMAL] = p_battle_units[i].hits_per_figure;
                    Battle_Apply_Damage(i, dmg);
                }
            }
        }
    }
}

/* (x, y, spell, caster_player, show_anim, caster) -> 0 when not animated: a targeted spell's animation (bolts,
   Lightning, Cracks Call, the square animation); the mass spells go to Mass_Spell_Animate; the rest (global
   combat enchantments, Disenchant Area / True, Raise / Animate Dead) flash the screen in the realm's color */
int Combat_Spell_Animate(int x, int y, int spell, int caster_player, int show_anim, int caster)
{
    int big;                                    /* Spell_Load_Anim: the big animation flag (as in Mass_Spell_Animate) */
    int red;
    int green;
    int blue;
    int i;

    if (p_spell_data[spell].effect_type != SPELL_FX_COMBAT_ENCH && p_spell_data[spell].effect_type != SPELL_FX_COMBAT_ENCH_VAR &&
        p_spell_data[spell].effect_type != SPELL_FX_DISENCHANT && spell != SPELL_RAISE_DEAD && spell != SPELL_ANIMATE_DEAD) {
        if (show_anim == 0)
            return 0;
        Combat_Load_Spell_Sound(spell);
        if (spell == SPELL_FIREBALL || spell == SPELL_FIRE_BOLT || spell == SPELL_ICE_BOLT || spell == SPELL_DOOM_BOLT) {
            Bolt_Spell_Anim(x, y, spell, caster);
        } else if (spell == SPELL_LIGHTNING_BOLT) {
            Spell_Load_Anim(SPELL_CALL_LIGHTNING);
            Lightning_Strike_Anim(x, y, caster);
        } else {
            Heap_Mark(p_scratch);
            big = Spell_Load_Anim(spell);
            Heap_Release(p_scratch);
            if (spell == SPELL_CRACKS_CALL)
                Cracks_Call_Anim(x, y, caster);
            else
                Spell_Anim_On_Square(x, y, big, caster, spell);
        }
        Heap_Release(p_scratch);                /* no matching mark on the bolt / lightning paths; a second release on the rest */
    } else {
        if (spell == SPELL_FLAME_STRIKE || spell == SPELL_HOLY_WORD || spell == SPELL_DEATH_SPELL ||
            spell == SPELL_CALL_CHAOS || spell == SPELL_MASS_HEALING) {
            Mass_Spell_Animate(caster_player, spell, show_anim, caster);
        } else {
            if (show_anim == 0)
                return 0;
            switch (p_spell_data[spell].realm) {
            case REALM_NATURE:
                red = 0;
                green = 63;
                blue = 0;
                break;
            case REALM_CHAOS:
                red = 63;
                green = 0;
                blue = 0;
                break;
            case REALM_SORCERY:
                red = 0;
                green = 0;
                blue = 63;
                break;
            case REALM_LIFE:
                red = 63;
                green = 63;
                blue = 63;
                break;
            case REALM_DEATH:
                red = 0;
                green = 0;
                blue = 0;
                break;
            case REALM_ARCANE:
                red = 63;
                green = 63;
                blue = 63;
                break;
            }
            Set_Draw_Page();
            Combat_Draw_Screen();
            Combat_Cast_Banner(caster, spell);
            Screen_Present();
            for (i = 0; i < 20; i++) {
                Palette_Mark_Range(0, 255);
                Palette_Tint_Upload(i * 2, red, green, blue);
            }
            for (i = 20; i > -1; i--) {
                Palette_Mark_Range(0, 255);
                Palette_Tint_Upload(i * 2, red, green, blue);
            }
        }
    }
}

/* (caster_player, caster, x, y) Raise Dead: the caster picks one of its own normal units out of the battle (any status
   but ACTIVE / DESTROYED: dead, but also fled, recalled or set aside; not one in the plane-9 limbo); the AI takes the first and a random free square on its side. It returns with half its figures
   (a single figure: half its hits), its enchantments wiped. BUG #220: names[] are uninitialised pointers that strcpy
   writes through
   Numbered: BUG #194, #220, #231 (BUGS_AND_QUIRKS.txt) */
void Raise_Dead(int caster_player, int caster, int x, int y)
{
    int count;
    int choice;
    int candidates[8];
    char *names[8];
    int i;
    int raised;

    Battlefield_Draw_Terrain();
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
    Screen_Copy_Front();
    Screen_Save_A800();
    count = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status > 0 && p_battle_units[i].owner == caster_player &&
            (char)p_battle_units[i].race < RACE_ARCANE_CREATURE && p_battle_units[i].status != BATTLE_UNIT_DESTROYED &&
            p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY) {
            candidates[count] = i;
            strcpy(names[count], unit_types[p_units[p_battle_units[i].unit_index].type].name);
            count++;
        }
    }
    choice = -1;
    if (count > 0) {
        if (caster_player != PLAYER_HUMAN) {
            choice = 0;
        } else if (count == 1) {
            choice = 0;
        } else if (count < 7) {
            choice = List_Select_Dialog(count, names, 0, "Select a unit to Raise");
        } else {
            choice = List_Select_Dialog(6, names, 1, "Select a unit to Raise");
        }
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
    if (choice != -1) {
        if (caster_player != PLAYER_HUMAN) {
            do {
                if (caster_player == combat_attacker_player)
                    x = 14 - Random(3);
                else
                    x = Random(3) + 7;
                y = Random(3) + 8;
            } while (combat_click_map[y][x] >= 0);
        }
        raised = candidates[choice];
        if (p_battle_units[raised].figures > 1) {
            p_battle_units[raised].figures_alive = p_battle_units[raised].figures / 2;
            p_battle_units[raised].front_damage = 0;
        } else {
            p_battle_units[raised].figures_alive = p_battle_units[raised].figures;
            p_battle_units[raised].front_damage = p_battle_units[raised].hits_per_figure / 2;
        }
        p_battle_units[raised].combat_flags = 0;
        p_battle_units[raised].combat_enchantments = 0;
        p_units[p_battle_units[raised].unit_index].enchantments = 0;
        p_battle_units[raised].combat_x = x;
        p_battle_units[raised].combat_y = y;
        p_battle_units[raised].dest_x = x;
        p_battle_units[raised].dest_y = y;
        p_battle_units[raised].move_anim_step = 0;
        p_battle_units[raised].aura_color = 0;
        p_battle_units[raised].hit_anim_kills = 0;
        p_battle_units[raised].moving = 0;
        p_battle_units[raised].action_state = ACTION_NONE;
        p_battle_units[raised].gfx_slot = Load_Unit_Figure_Frames(p_units[p_battle_units[raised].unit_index].type,
                                                               Battle_Free_Gfx_Slot());
        p_battle_units[raised].status = BATTLE_UNIT_ACTIVE;
        Unit_Apply_Modifiers(&p_battle_units[raised]);
        Battle_Unit_Combat_Modifiers(&p_battle_units[raised]);
        Combat_Summon_Anim(raised, x, y, SPELL_RAISE_DEAD, caster);
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
}

/* (caster_player, caster) Animate Dead: the human picks a non-hero unit out of the battle (any status but ACTIVE /
   DESTROYED: dead, but also fled, recalled or set aside - BUG #231; any side's, but a Magic Immune one only its own;
   not undead, not in limbo) that rises at full figures on a random free square of the caster's
   side as the caster's undead (MUT_UNDEAD). BUG #220: names[] are uninitialised pointers that strcpy writes
   through; the AI shows the dialog too
   Numbered: BUG #194, #220, #221, #231 (BUGS_AND_QUIRKS.txt) */
void Animate_Dead(int caster_player, int caster)
{
    int count;
    int choice;
    int x;
    int y;
    int candidates[17];
    char *names[17];
    int i;
    int raised;

    count = 0;
    Battlefield_Draw_Terrain();
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
    Screen_Copy_Front();
    Screen_Save_A800();
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status > 0 && p_battle_units[i].status != BATTLE_UNIT_DESTROYED && p_battle_units[i].race != RACE_DEATH_CREATURE &&
            ((p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY) == 0 || p_battle_units[i].owner == caster_player) &&
            p_units[p_battle_units[i].unit_index].hero_slot == -1 && p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY) {
            candidates[count] = i;
            strcpy(names[count], unit_types[p_units[p_battle_units[i].unit_index].type].name);
            count++;
        }
    }
    if (count < 7)
        choice = List_Select_Dialog(count, names, 0, "Select a unit to Animate");
    else
        choice = List_Select_Dialog(6, names, 1, "Select a unit to Animate");
    if (choice != -1) {
        do {
            if (caster_player == combat_attacker_player)
                x = 14 - Random(3);
            else
                x = Random(3) + 7;
            y = Random(3) + 8;
        } while (combat_click_map[y][x] >= 0);
        raised = candidates[choice];
        p_battle_units[raised].owner = caster_player;
        p_battle_units[raised].figures_alive = p_battle_units[raised].figures;
        p_battle_units[raised].front_damage = 0;
        p_battle_units[raised].combat_flags = 0;
        p_battle_units[raised].combat_enchantments = 0;
        p_battle_units[raised].move_anim_step = 0;
        p_battle_units[raised].aura_color = 0;
        p_battle_units[raised].hit_anim_kills = 0;
        p_battle_units[raised].moving = 0;
        p_battle_units[raised].action_state = ACTION_NONE;
        p_units[p_battle_units[raised].unit_index].enchantments = 0;
        p_units[p_battle_units[raised].unit_index].mutation_flags =
            p_units[p_battle_units[raised].unit_index].mutation_flags | MUT_UNDEAD;
        p_battle_units[raised].combat_x = x;
        p_battle_units[raised].combat_y = y;
        p_battle_units[raised].gfx_slot = Load_Unit_Figure_Frames(p_units[p_battle_units[raised].unit_index].type,
                                                               Battle_Free_Gfx_Slot());
        p_battle_units[raised].status = BATTLE_UNIT_ACTIVE;
        Unit_Apply_Modifiers(&p_battle_units[raised]);
        Battle_Unit_Combat_Modifiers(&p_battle_units[raised]);
        Combat_Summon_Anim(raised, x, y, SPELL_ANIMATE_DEAD, caster);
    }
}
