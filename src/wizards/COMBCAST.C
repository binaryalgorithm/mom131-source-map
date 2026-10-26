/* COMBCAST.C - WIZARDS.EXE overlay code segment 8a20 (stub 2535): the AI's choice of a target for a combat spell
   (by the spell's effect type, with the special tests for True Light / Darkness, Healing, Warp Wood, Warp Creature,
   Disintegrate, dispels and Holy Word) and casting a combat spell (damage, resistance rolls, curses, enchantments,
   the side's combat enchantments, Disenchant Area, Dispel Magic, walls, the special spells and summons).
   This module has no data of its own (no literals, no BSS).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 8a20 src/wizards/COMBCAST.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Combat_Spell_Target(int spell, int player, int *x, int *y);
int AI_Check_TrueLight_Darkness(int spell);
int AI_Target_Healing(int player);
int AI_Target_Warp_Wood(int player);
int AI_Target_Warp_Creature(int player);
int AI_Target_Disintegrate(int player);
int AI_Target_Dispel(int player);
int AI_Check_Holy_Word(int player);
void Combat_Cast_Spell(int spell, int target, int caster, int x, int y, int mana, int show_anim);

#include "GAME.H"

extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int ai_defender_spell_threat;              /* DS:9294 */
extern int ai_defender_attack_profile[5];       /* DS:9298 */
extern int ai_attacker_attack_profile[5];       /* DS:92a2 */
extern int ai_defender_threat_realms;           /* DS:92ac  realm bits of the defender's castable hostile spells */
extern int ai_attacker_threat_realms;           /* DS:92ae */
extern int present_mode;                        /* DS:9c8e */
extern char *combat_click_map[];                /* DS:c524  per grid row: the battle unit at each square, -1 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */

int Random(int n);
void Set_Draw_Page(void);
void Screen_Present(void);
void Combat_Draw_Screen(void);
int Combat_Unit_Visible_To_Enemy(int bu);
int Unit_Combat_Moves(int bu);
void Unit_Apply_Modifiers(struct BattleUnit far *bu);
void Battle_Unit_Combat_Modifiers(struct BattleUnit far *bu);
void Battle_Unit_Heal(int bu, int amount, int overflow);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Combat_Summon_Add_Unit(int owner, int unit, int x, int y);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
void Battle_Apply_Damage(int unit, int *damage);
int Spell_Resist_Modifier(int spell);
int AI_Target_Earth_To_Mud(int player, int *x, int *y);
int AI_Target_Disrupt(int player, int *x, int *y);
int AI_Target_Recall_Hero(int player);
int AI_Target_Cracks_Call(int player, int *x, int *y);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
int Resist_Bonus(struct BattleUnit bu, int realm);
int Combat_Distance(int bu1, int bu2);
int Combat_Spell_Animate(int x, int y, int spell, int caster_player, int show_anim, int caster);
void Disenchant_Area_Combat(int caster, int strength);
void Dispel_Square(int x, int y, int caster, int strength, int *msgs);
void Cracks_Call_Square(int x, int y);
void Raise_Dead(int caster_player, int caster, int x, int y);
void Animate_Dead(int caster_player, int caster);
void Life_Drain_Apply(int target, int *damage, int caster, int mana);
void Combat_Wall_Create(int spell, int caster);
void Warp_Creature_Apply(int unit);
void Vortex_Create(int caster_player, int x, int y);
void Combat_Summon_Anim(int unit, int x, int y, int spell, int caster);
int Unit_Combat_Value(int bu);
void Earth_To_Mud_Square(int x, int y);

/* (spell, player, &x, &y) -> the AI's target for a combat spell by its effect type: damage (SPELL_FX_DAMAGE /
   _DAMAGE_VAR) the enemy with the best estimated damage (Life Drain: 13 - its death resistance) plus 100 - its hits
   left + its front damage; save-or-die (SPELL_FX_DESTROY / _DESTROY_VAR; _UNIT_CURSE; _NORMAL_UNIT_CURSE against
   normal units) the enemy with the best value x the chance to fail its resistance (Disintegrate: its full value);
   buffs (SPELL_FX_UNIT_ENCH; _NORMAL_UNIT_ENCH normal units only) the own unit with the most figures x 10 + melee
   that lacks the enchantment (Berserk only for a non-hero that reaches its target this turn); SPELL_FX_CURSE_NO_RESIST
   (Web and holds) the best enemy by value (+ 2000 for a flyer under Web); SPELL_FX_COMBAT_ENCH / _VAR TARGET_SQUARE
   unless already active on its side (-> -1), True Light / Darkness and Holy Word by their own tests; SPELL_FX_SPECIAL
   the special targeting functions (Healing, Mass Healing, Warp Wood, Warp Creature, Earth to Mud, Disrupt, Recall
   Hero, Cracks Call, Raise Dead); SPELL_FX_DISPEL dispels; SPELL_FX_SUMMON a free square of the own half near the
   middle (up to 200 tries, fewer than 9 own units and 36 battle units, not at 1000 units) -> TARGET_SQUARE;
   SPELL_FX_DISENCHANT -> TARGET_SQUARE. Enemies with Magic
   Immunity or Righteousness (chaos, death) are skipped; -1 without a target. BUG #440: no SPELL_FX_CITY_ENCH case, so Wall
   of Fire / Wall of Darkness get -1 and the AI's scorer (COMBAI.C) rejects them before its own SPELL_WALL_OF_FIRE
   case: the AI never casts the walls in combat; the summon square is kept after 200 failed tries (occupied: the summon
   lands on a unit). clarified using ReMoM repo
   Numbered: BUG #212, #229, #247 (BUGS_AND_QUIRKS.txt) */
int AI_Combat_Spell_Target(int spell, int player, int *x, int *y)
{
    int best_score;
    int score;
    int best;
    int resist;
    long ench;
    int resist_mod;
    int zero_bonus;                                  /* always 0, added into every damage score */
    int resist_term;
    int dmg;
    int i;

    best_score = -1;
    best = -1;
    switch (p_spell_data[spell].effect_type) {
    case SPELL_FX_DAMAGE:
    case SPELL_FX_DAMAGE_VAR:
        for (i = 0; i < battle_unit_count; i++) {
            dmg = 0;
            resist_term = 0;
            zero_bonus = 0;
            if ((p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY) == 0) {
                if (spell == SPELL_STAR_FIRES && p_battle_units[i].race != RACE_DEATH_CREATURE && p_battle_units[i].race != RACE_CHAOS_CREATURE)
                    continue;
                if (spell == SPELL_PSIONIC_BLAST && (p_battle_units[i].abilities & ABILITY_ILLUSIONS_IMMUNITY))
                    continue;
                if (spell == SPELL_LIFE_DRAIN && (p_battle_units[i].race == RACE_DEATH_CREATURE || (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY)))
                    continue;
                ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                       p_battle_units[i].item_enchantments;
                if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH))
                    continue;
                if (p_battle_units[i].owner == player)
                    continue;
                if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                    continue;
                if (!Combat_Unit_Visible_To_Enemy(i))
                    continue;
                if (spell == SPELL_LIFE_DRAIN) {
                    resist_term = 13 - Resist_Bonus(p_battle_units[i], REALM_DEATH);
                    if (resist_term < 0)
                        resist_term = 0;
                } else
                    Spell_Damage_Roll(spell, i, &dmg, 25);
                score = dmg + resist_term + zero_bonus;
                if (score > 0)
                    score += 100 - p_battle_units[i].figures_alive * p_battle_units[i].hits_per_figure +
                             p_battle_units[i].front_damage;
                if (score > best_score) {
                    best_score = score;
                    best = i;
                }
            }
        }
        break;
    case SPELL_FX_DESTROY:
    case SPELL_FX_DESTROY_VAR:
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == player)
                continue;
            if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                continue;
            if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
                continue;
            if ((p_battle_units[i].abilities & ABILITY_STONING_IMMUNITY) && spell == SPELL_PETRIFY)
                continue;
            if (spell == SPELL_DISPEL_EVIL && p_battle_units[i].race != RACE_DEATH_CREATURE && p_battle_units[i].race != RACE_CHAOS_CREATURE)
                continue;
            if (spell == SPELL_BANISH && (char)p_battle_units[i].race < RACE_ARCANE_CREATURE)
                continue;
            ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                   p_battle_units[i].item_enchantments;
            if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH))
                continue;
            if (!Combat_Unit_Visible_To_Enemy(i))
                continue;
            resist_mod = Spell_Resist_Modifier(spell);
            resist = Resist_Bonus(p_battle_units[i], p_spell_data[spell].realm);
            resist += resist_mod;
            if (resist >= RESIST_ALWAYS)
                continue;
            if (spell == SPELL_DISINTEGRATE)
                score = Unit_Combat_Value(i);
            else
                score = (Unit_Combat_Value(i) * (10 - resist) + 9) / 10;
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
        break;
    case SPELL_FX_UNIT_ENCH:
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner != player)
                continue;
            if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                continue;
            ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                   p_battle_units[i].item_enchantments;
            if (ench & p_spell_data[spell].effect_param)
                continue;
            if (spell == SPELL_STONE_SKIN && (ench & UNIT_ENCH_IRON_SKIN))
                continue;
            if (spell == SPELL_RESIST_ELEMENTS && (ench & UNIT_ENCH_ELEMENTAL_ARMOR))
                continue;
            if (spell == SPELL_RESIST_MAGIC && (ench & UNIT_ENCH_WRAITH_FORM))
                continue;
            if (spell == SPELL_FLIGHT && (p_battle_units[i].move_flags & MOVE_FLYING))
                continue;
            if (spell == SPELL_BERSERK &&
                (p_battle_units[i].target_unit == -1 || p_battle_units[i].target_unit == i ||
                 Combat_Distance(i, p_battle_units[i].target_unit) > p_battle_units[i].moves ||
                 p_units[p_battle_units[i].unit_index].hero_slot > -1))
                continue;
            score = p_battle_units[i].figures_alive * 10 + p_battle_units[i].melee;
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
        break;
    case SPELL_FX_NORMAL_UNIT_ENCH:
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner != player)
                continue;
            if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                continue;
            if ((char)p_battle_units[i].race >= RACE_ARCANE_CREATURE)
                continue;
            ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                   p_battle_units[i].item_enchantments;
            if (ench & p_spell_data[spell].effect_param)
                continue;
            if (spell == SPELL_HEROISM && p_units[p_battle_units[i].unit_index].level > 2)
                continue;
            score = p_battle_units[i].figures_alive * 10 + p_battle_units[i].melee;
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
        break;
    case SPELL_FX_UNIT_CURSE:
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == player)
                continue;
            if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                continue;
            if (p_battle_units[i].combat_flags & (int)p_spell_data[spell].effect_param)
                continue;
            if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
                continue;
            if (p_spell_data[spell].realm == REALM_SORCERY && (p_battle_units[i].abilities & ABILITY_ILLUSIONS_IMMUNITY))
                continue;
            if (p_spell_data[spell].realm == REALM_DEATH && (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY))
                continue;
            ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                   p_battle_units[i].item_enchantments;
            if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH))
                continue;
            if (spell == SPELL_CREATURE_BINDING && (char)p_battle_units[i].race < RACE_ARCANE_CREATURE)
                continue;
            if (spell == SPELL_SHATTER && p_battle_units[i].melee <= 1 && p_battle_units[i].ranged <= 1)
                continue;
            if (!Combat_Unit_Visible_To_Enemy(i))
                continue;
            resist_mod = Spell_Resist_Modifier(spell);
            resist = Resist_Bonus(p_battle_units[i], p_spell_data[spell].realm);
            resist += resist_mod;
            if (resist >= RESIST_ALWAYS)
                continue;
            score = (Unit_Combat_Value(i) * (10 - resist) + 9) / 10;
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
        break;
    case SPELL_FX_NORMAL_UNIT_CURSE:
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
                continue;
            if (p_battle_units[i].combat_flags & p_spell_data[spell].effect_param)
                continue;
            if (p_spell_data[spell].realm == REALM_SORCERY && (p_battle_units[i].abilities & ABILITY_ILLUSIONS_IMMUNITY))
                continue;
            ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                   p_battle_units[i].item_enchantments;
            if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH))
                continue;
            if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                (char)p_battle_units[i].race < RACE_ARCANE_CREATURE) {
                if (!Combat_Unit_Visible_To_Enemy(i))
                    continue;
                resist = Resist_Bonus(p_battle_units[i], p_spell_data[spell].realm);
                if (resist >= RESIST_ALWAYS)
                    continue;
                score = (Unit_Combat_Value(i) * (10 - resist) + 9) / 10;
                if (score > best_score) {
                    best_score = score;
                    best = i;
                }
            }
        }
        break;
    case SPELL_FX_CURSE_NO_RESIST:
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == player)
                continue;
            if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
                continue;
            if (spell == SPELL_WEB) {
                if (p_battle_units[i].ability_flags & AFLAG_NON_CORPOREAL)
                    continue;
                if (p_battle_units[i].web_hp > 0)
                    continue;
            } else {
                if (p_battle_units[i].combat_flags & p_spell_data[spell].effect_param)
                    continue;
                if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
                    continue;
                if (p_spell_data[spell].realm == REALM_SORCERY && (p_battle_units[i].abilities & ABILITY_ILLUSIONS_IMMUNITY))
                    continue;
            }
            ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
                   p_battle_units[i].item_enchantments;
            if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH))
                continue;
            if (!Combat_Unit_Visible_To_Enemy(i))
                continue;
            score = Unit_Combat_Value(i);
            if (spell == SPELL_WEB && (p_battle_units[i].move_flags & MOVE_FLYING))
                score += 2000;
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
        break;
    case SPELL_FX_COMBAT_ENCH:
    case SPELL_FX_COMBAT_ENCH_VAR:
        if (player == combat_attacker_player)   /* value: the side's offset in the enchantment pair */
            resist = 0;
        else
            resist = 1;
        if (p_combat_enchants[(int)p_spell_data[spell].effect_param + resist] > 0)
            return -1;
        if (spell == SPELL_TRUE_LIGHT || spell == SPELL_DARKNESS)
            best = AI_Check_TrueLight_Darkness(spell);
        else if (spell == SPELL_HOLY_WORD)
            best = AI_Check_Holy_Word(player);
        else
            best = 99;
        break;
    case SPELL_FX_SPECIAL:
        if (spell == SPELL_HEALING)
            best = AI_Target_Healing(player);
        if (spell == SPELL_MASS_HEALING) {
            best = AI_Target_Healing(player);
            if (best > -1)
                best = 99;
        }
        if (spell == SPELL_WARP_WOOD)
            best = AI_Target_Warp_Wood(player);
        if (spell == SPELL_WARP_CREATURE)
            best = AI_Target_Warp_Creature(player);
        if (spell == SPELL_EARTH_TO_MUD)
            best = AI_Target_Earth_To_Mud(player, x, y);
        if (spell == SPELL_DISRUPT)
            best = AI_Target_Disrupt(player, x, y);
        if (spell == SPELL_RECALL_HERO)
            best = AI_Target_Recall_Hero(player);
        if (spell == SPELL_CRACKS_CALL)
            best = AI_Target_Cracks_Call(player, x, y);
        if (spell == SPELL_RAISE_DEAD) {
            best = 99;
            *x = 0;
            *y = 0;
        }
        break;
    case SPELL_FX_DISPEL:
        best = AI_Target_Dispel(player);
        break;
    case SPELL_FX_SUMMON:
        if (num_units != MAX_UNITS) {
            resist = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
                    resist++;
            }
            if (resist < MAX_STACK && battle_unit_count < MAX_BATTLE_UNITS) {
                resist = 0;
                do {
                    if (player == combat_attacker_player)   /* the summon square: x 11 .. 13 (attacker) or
                                                   8 .. 10, y 9 .. 11 */
                        *x = 14 - Random(3);
                    else
                        *x = Random(3) + 7;
                    *y = Random(3) + 8;
                    resist++;
                } while (combat_click_map[*y][*x] >= 0 && resist < 200);
                best = 99;
            }
        }
        break;
    case SPELL_FX_DISENCHANT:
        best = 99;
        break;
    default:
        break;
    }
    return best;
}

/* (spell: True Light or Darkness) -> 99 when a living unit of race 19 or 20 (the life and death realms' creatures)
   is on the field, else -1 (the light / dark flag is computed and not used) */
int AI_Check_TrueLight_Darkness(int spell)
{
    int light;
    int result;
    int i;

    result = -1;
    if (spell == SPELL_TRUE_LIGHT)
        light = 1;
    else
        light = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if ((p_battle_units[i].race == RACE_LIFE_CREATURE || p_battle_units[i].race == RACE_DEATH_CREATURE) && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            result = 99;
    }
    return result;
}

/* (player) -> the own living, non-undead unit that lost the most: (figures lost x hits + front damage, when above 2)
   x (melee + ranged); -1 when none lost more than 2 */
int AI_Target_Healing(int player)
{
    int best_value;
    int heal_value;
    int best;
    int i;

    best_value = 0;
    best = -1;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner != player)
            continue;
        if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
            continue;
        if (p_battle_units[i].race == RACE_DEATH_CREATURE)
            continue;
        heal_value = (p_battle_units[i].figures - p_battle_units[i].figures_alive) * p_battle_units[i].hits_per_figure +
                p_battle_units[i].front_damage;
        if (heal_value > 2)
            heal_value *= p_battle_units[i].melee + p_battle_units[i].ranged;
        else
            heal_value = 0;
        if (heal_value > best_value) {
            best_value = heal_value;
            best = i;
        }
    }
    return best;
}

/* (player) -> the enemy missile unit (ranged type 2x) with the most ranged x ammunition x figures, else any
   targetable enemy (score -10); Magic Immunity and Righteousness skipped */
int AI_Target_Warp_Wood(int player)
{
    int best_score;
    int best;
    long ench;
    int i;
    int score;

    best = -1;
    best_score = -1;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
            continue;
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (ench & UNIT_ENCH_RIGHTEOUSNESS)
            continue;
        if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            if (!Combat_Unit_Visible_To_Enemy(i))
                continue;
            if (p_battle_units[i].ranged_type / 10 == RANGED_CLASS_MISSILE)
                score = p_battle_units[i].ranged * p_battle_units[i].ammo * p_battle_units[i].figures_alive;
            else
                score = -10;
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
    }
    return best;
}

/* (player) -> the normal enemy unit (not yet warped: combat_flags 0x80 / 0x100 / 0x200) with the best (melee +
   ranged) x figures x the chance to fail its resistance against Warp Creature's realm */
int AI_Target_Warp_Creature(int player)
{
    int score;
    int best_score;
    int best;
    long ench;
    register int i;
    register int resist;

    best = -1;
    best_score = -1;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
            continue;
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (ench & UNIT_ENCH_RIGHTEOUSNESS)
            continue;
        if ((p_battle_units[i].combat_flags & COMBAT_FLAG_WARP_MELEE) || (p_battle_units[i].combat_flags & COMBAT_FLAG_WARP_DEFENSE) ||
            (p_battle_units[i].combat_flags & COMBAT_FLAG_WARP_RESIST))
            continue;
        if (p_battle_units[i].owner == player)
            continue;
        if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
            continue;
        if ((char)p_battle_units[i].race >= RACE_ARCANE_CREATURE)
            continue;
        if (!Combat_Unit_Visible_To_Enemy(i))
            continue;
        resist = Resist_Bonus(p_battle_units[i], p_spell_data[SPELL_WARP_CREATURE].realm);
        if (resist >= RESIST_ALWAYS)
            continue;
        score = ((p_battle_units[i].melee + p_battle_units[i].ranged) * p_battle_units[i].figures_alive * (10 - resist) +
                 9) / 10;
        if (score > best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

/* (player) -> the enemy unit with the most (melee + ranged) x figures whose resistance against Disintegrate's realm
   is at most 10 */
int AI_Target_Disintegrate(int player)
{
    long ench;
    int resist;
    int best_score;
    int best;
    register int i;
    register int score;

    best = -1;
    best_score = -1;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY)
            continue;
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (ench & UNIT_ENCH_RIGHTEOUSNESS)
            continue;
        if (p_battle_units[i].owner == player)
            continue;
        if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
            continue;
        if (!Combat_Unit_Visible_To_Enemy(i))
            continue;
        resist = Resist_Bonus(p_battle_units[i], p_spell_data[SPELL_DISINTEGRATE].realm);
        if (resist > 10)
            continue;
        score = (p_battle_units[i].melee + p_battle_units[i].ranged) * p_battle_units[i].figures_alive;
        if (score > best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

/* (player) -> the unit a dispel helps most: an own unit under curses (COMBAT_FLAGS_CURSES) by its value plus a
   weight per curse; an enemy with unit enchantments (or combat_flags 0x6800, or confused) by its value plus a weight
   per enchantment, some only when the own side's threat realms meet them; -1 for none above 0. BUG #312: the threat
   realms and attack profiles are taken the same way for both sides (the defender gets the attacker's), the
   profiles and ai_defender_spell_threat (copied to threat) are never used, and one test reads 'threats & 0'
   Numbered: BUG #20, #312 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Dispel(int player)
{
    int best;
    int best_score;
    long ench;
    int *enemy_profile;
    int *own_profile;
    int own_threats;
    int enemy_threats;
    int threat;
    int score;
    int i;

    best_score = 0;
    best = -1;
    if (player == combat_attacker_player) {
        enemy_threats = ai_defender_threat_realms;
        enemy_profile = ai_defender_attack_profile;
        own_threats = ai_attacker_threat_realms;
        own_profile = ai_attacker_attack_profile;
    } else {
        own_threats = ai_attacker_threat_realms;
        own_profile = ai_attacker_attack_profile;
        enemy_threats = ai_defender_threat_realms;
        enemy_profile = ai_defender_attack_profile;
    }
    threat = ai_defender_spell_threat;
    for (i = 0; i < battle_unit_count; i++) {
        score = -1;
        ench = p_battle_units[i].combat_enchantments | p_units[p_battle_units[i].unit_index].enchantments;
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
            (p_battle_units[i].combat_flags & COMBAT_FLAGS_CURSES)) {
            score = Unit_Combat_Value(i);
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_VERTIGO)
                score += 20;
            if ((p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION) && p_battle_units[i].confusion_result != CONFUSION_CHANGED_SIDE)
                score += 30;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_MIND_STORM)
                score += 40;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_SHATTER)
                score += 10;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_WEAKNESS)
                score += 10;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_BLACK_SLEEP)
                score += 40;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_WARP_MELEE)
                score += 15;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_WARP_DEFENSE)
                score += 10;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_WARP_RESIST)
                score += 10;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_MIND_TWIST)
                score += 10;
        }
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
            (ench > 0 || (p_battle_units[i].combat_flags & COMBAT_FLAGS_NON_CURSE) ||
             ((p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION) && p_battle_units[i].confusion_result == CONFUSION_CHANGED_SIDE))) {
            if (!Combat_Unit_Visible_To_Enemy(i))
                continue;
            score = Unit_Combat_Value(i);
            if (ench & UNIT_ENCH_IMMOLATION)
                score += 20;
            if ((p_battle_units[i].combat_flags & COMBAT_FLAG_CONFUSION) && p_battle_units[i].confusion_result != CONFUSION_CHANGED_SIDE)
                score += 30;
            if (ench & UNIT_ENCH_GUARDIAN_WIND)
                score += 10;
            if (ench & UNIT_ENCH_CLOAK_OF_FEAR)
                score += 10;
            if (ench & UNIT_ENCH_WRAITH_FORM)
                score += 10;
            /* the threat tests use realm numbers as masks, as in COMBAI.C (see AI_Combat_Spell_Score) */
            if ((ench & UNIT_ENCH_ELEMENTAL_ARMOR) && ((own_threats & 2) || (own_threats & 0)))
                score += 20;
            if (ench & UNIT_ENCH_STONE_SKIN)
                score += 10;
            if (ench & UNIT_ENCH_IRON_SKIN)
                score += 20;
            if (ench & UNIT_ENCH_SPELL_LOCK)
                score -= 10;
            if (ench & UNIT_ENCH_INVISIBILITY)
                score += 10;
            if (ench & UNIT_ENCH_MAGIC_IMMUNITY)
                score += 25;
            if ((ench & UNIT_ENCH_TRUE_SIGHT) && (own_threats & 1))
                score += 30;
            if (ench & UNIT_ENCH_LIONHEART)
                score += 30;
            if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && ((own_threats & 2) || (own_threats & 8)))
                score += 30;
            if (ench & UNIT_ENCH_INVULNERABILITY)
                score += 40;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_CREATURE_BINDING)
                score += 70;
            if (p_battle_units[i].combat_flags & COMBAT_FLAG_POSSESSION)
                score += 70;
        }
        if (score > best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

/* (spell, target battle unit, caster: battle unit < 20 or 20 + player, x, y, mana spent, show the animation) casts
   a combat spell. The resistance modifier drops by the spell-save bonus of a hero caster's items. By effect type:
   4 direct damage; 22 the same with Life Drain, the bolts boosted 1:1 by extra mana (Psionic Blast 1/2, Fireball
   1/3); 12 / 23 Disintegrate (resistance + modifier below 10 -> 200 irrecoverable damage) or a resistance roll per
   figure (Dispel Evil -5 against undead, Banish - extra mana / 15; Petrify not against stoning immunity), Banish and
   Dispel Evil not against Spell Lock (enchantment 0x4000); 13 / 16 curses on a failed roll (Possession and Creature Binding switch
   sides, Black Sleep ends the turn); 14 unresisted effects (Web: 12 strength, ends the turn) not against Magic
   Immunity (except Web), Righteousness (chaos, death) or Illusions Immunity (sorcery); 1 / 15 unit enchantments
   (Haste: combat_flags 0x800); 10 / 21 the side's combat enchantment (Counter Magic: its strength = the mana; not
   Flame Strike, Holy Word, Death Spell, Call Chaos, Mass Healing, which work elsewhere); 19 Disenchant Area and 18
   Dispel Magic (Runemaster x2, the True versions x3); 2 walls; 5 the special spells; 0 summons (Unit_Create on plane
   9). The unit statistics are recomputed after each, keeping the moves already spent. BUG #193: the target's combat / item
   enchantments are read with an uninitialised battle unit index (i); every normal-unit enchantment's recomputation
   clears the 'full moves' flag first, so the moves never refill. The Runemaster test is written '(&wizards[..].retorts[..])[0]': the game adds the retort's
   offset (0x72) to the wizard's address at run time, which a plain 'wizards[caster - 20].retorts[..]' folds into
   one displacement (the original probably named the retort as a member)
   Numbered: BUG #193, #211, #216, #230, #268, #280 (BUGS_AND_QUIRKS.txt) */
void Combat_Cast_Spell(int spell, int target, int caster, int x, int y, int mana, int show_anim)
{
    int resist;                                 /* also the p_combat_enchants index (the spell's CENCH_* slot from
                                                   effect_param, +1 for the defender) */
    int i;
    int caster_player;
    int figures;                                /* also Unit_Create's success flag */
    int modifier;
    long ench;
    int dmg[3];
    int value;
    int moves_were_full;

    caster_player = caster >= CASTER_WIZARD ? caster - CASTER_WIZARD : p_battle_units[caster].owner;
    ench = p_units[p_battle_units[target].unit_index].enchantments | p_battle_units[i].combat_enchantments |
           p_battle_units[i].item_enchantments;
    modifier = Spell_Resist_Modifier(spell);
    /* BUG #436: < 19, not < CASTER_WIZARD (20): battle unit 19 gets no item bonus (as in COMBAI.C) */
    if (caster < 19 && p_units[p_battle_units[caster].unit_index].hero_slot > -1) {
        for (i = 0; i < 3; i++) {
            if (wizards[caster_player].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[i] > -1)
                modifier -= p_items[wizards[caster_player].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[i]].spell_save;
        }
    }
    for (i = 0; i < 3; i++)
        dmg[i] = 0;
    switch (p_spell_data[spell].effect_type) {
    case SPELL_FX_DAMAGE:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        Spell_Damage_Roll(spell, target, dmg, 0);
        Battle_Apply_Damage(target, dmg);
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
        break;
    case SPELL_FX_DAMAGE_VAR:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        if (spell == SPELL_LIFE_DRAIN)
            Life_Drain_Apply(target, dmg, caster, mana);
        if (spell == SPELL_FIRE_BOLT || spell == SPELL_ICE_BOLT || spell == SPELL_LIGHTNING_BOLT) {
            value = (unsigned char)p_spell_data[spell].effect_param;   /* its low byte: the base strength */
            if (mana > 0)
                value += mana - p_spell_data[spell].base_cost;
            Spell_Damage_Roll(spell, target, dmg, value);
        }
        if (spell == SPELL_PSIONIC_BLAST) {
            value = (unsigned char)p_spell_data[spell].effect_param;   /* its low byte: the base strength */
            if (mana > 0)
                value += (mana - p_spell_data[spell].base_cost) / 2;
            Spell_Damage_Roll(spell, target, dmg, value);
        }
        if (spell == SPELL_FIREBALL) {
            value = (unsigned char)p_spell_data[spell].effect_param;   /* its low byte: the base strength */
            if (mana > 0)
                value += (mana - p_spell_data[spell].base_cost) / 3;
            Spell_Damage_Roll(spell, target, dmg, value);
        }
        Battle_Apply_Damage(target, dmg);
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
        break;
    case SPELL_FX_DESTROY:
    case SPELL_FX_DESTROY_VAR:
        if ((spell != SPELL_BANISH && spell != SPELL_DISPEL_EVIL) || (ench & UNIT_ENCH_SPELL_LOCK) == 0) {
            Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
            if (spell == SPELL_DISINTEGRATE) {
                if (Resist_Bonus(p_battle_units[target], REALM_CHAOS) + modifier < 10) {   /* Disintegrate's realm,
                                                   hard-coded */
                    dmg[DAMAGE_DESTROY] = 200;
                    Battle_Apply_Damage(target, dmg);
                }
                Set_Draw_Page();
                Combat_Draw_Screen();
                Screen_Present();
            } else {
                if (spell == SPELL_DISPEL_EVIL && (p_units[p_battle_units[target].unit_index].mutation_flags & MUT_UNDEAD))
                    modifier -= 5;
                if (spell == SPELL_BANISH)
                    modifier -= (mana - p_spell_data[SPELL_BANISH].base_cost) / 15;   /* -1 per 15 extra mana */
                if (spell != SPELL_PETRIFY || (p_battle_units[target].abilities & ABILITY_STONING_IMMUNITY) == 0) {
                    figures = p_battle_units[target].figures_alive;
                    for (i = 0; i < figures; i++) {
                        resist = Roll_Resist(p_battle_units[target], modifier, p_spell_data[spell].realm);
                        if (resist > 0) {
                            dmg[DAMAGE_DESTROY] = p_battle_units[target].hits_per_figure;
                            Battle_Apply_Damage(target, dmg);
                        }
                    }
                }
                Set_Draw_Page();
                Combat_Draw_Screen();
                Screen_Present();
            }
        }
        break;
    case SPELL_FX_UNIT_CURSE:
    case SPELL_FX_NORMAL_UNIT_CURSE:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        resist = Roll_Resist(p_battle_units[target], modifier, p_spell_data[spell].realm);
        if (resist > 0) {
            p_battle_units[target].combat_flags = p_battle_units[target].combat_flags |
                                                  (int)p_spell_data[spell].effect_param;
            if (spell == SPELL_POSSESSION || spell == SPELL_CREATURE_BINDING) {
                if (p_battle_units[target].owner == combat_attacker_player)
                    p_battle_units[target].owner = combat_defender_player;
                else
                    p_battle_units[target].owner = combat_attacker_player;
                if (p_battle_units[target].owner == PLAYER_HUMAN)
                    p_battle_units[target].action_state = ACTION_NONE;
            }
            if (spell == SPELL_BLACK_SLEEP) {
                p_battle_units[target].action_state = ACTION_DONE;
                p_battle_units[target].moves = 0;
            }
            Set_Draw_Page();
            Combat_Draw_Screen();
            Screen_Present();
            value = Unit_Combat_Moves(target);
            moves_were_full = 0;
            if (p_battle_units[target].moves == value)
                moves_were_full = 1;
            else
                value = p_battle_units[target].moves;
            Unit_Apply_Modifiers(&p_battle_units[target]);
            Battle_Unit_Combat_Modifiers(&p_battle_units[target]);
            if (moves_were_full == 1)
                p_battle_units[target].moves = Unit_Combat_Moves(target);
            else
                p_battle_units[target].moves = value;
        } else {
            Set_Draw_Page();
            Combat_Draw_Screen();
            Screen_Present();
        }
        break;
    case SPELL_FX_CURSE_NO_RESIST:
        if (((p_battle_units[target].abilities & ABILITY_MAGIC_IMMUNITY) == 0 || spell == SPELL_WEB) &&
            ((p_spell_data[spell].realm != REALM_CHAOS && p_spell_data[spell].realm != REALM_DEATH) || (ench & UNIT_ENCH_RIGHTEOUSNESS) == 0) &&
            (p_spell_data[spell].realm != REALM_SORCERY || (p_battle_units[target].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0)) {
            Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
            p_battle_units[target].combat_flags = p_battle_units[target].combat_flags |
                                                  (int)p_spell_data[spell].effect_param;
            if (spell == SPELL_WEB) {
                p_battle_units[target].web_hp = 12;   /* Web's strength */
                p_battle_units[target].action_state = ACTION_DONE;
                p_battle_units[target].moves = 0;
            }
            Set_Draw_Page();
            Combat_Draw_Screen();
            Screen_Present();
            value = Unit_Combat_Moves(target);
            moves_were_full = 0;
            if (p_battle_units[target].moves == value)
                moves_were_full = 1;
            else
                value = p_battle_units[target].moves;
            Unit_Apply_Modifiers(&p_battle_units[target]);
            Battle_Unit_Combat_Modifiers(&p_battle_units[target]);
            if (moves_were_full == 1)
                p_battle_units[target].moves = Unit_Combat_Moves(target);
            else
                p_battle_units[target].moves = value;
        } else {
            Set_Draw_Page();
            Combat_Draw_Screen();
            Screen_Present();
        }
        break;
    case SPELL_FX_UNIT_ENCH:
    case SPELL_FX_NORMAL_UNIT_ENCH:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        if (spell == SPELL_HASTE)
            p_battle_units[target].combat_flags = p_battle_units[target].combat_flags | COMBAT_FLAG_HASTE;
        else
            p_battle_units[target].combat_enchantments = p_battle_units[target].combat_enchantments |
                                                         p_spell_data[spell].effect_param;
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
        value = Unit_Combat_Moves(target);
        moves_were_full = 0;
        if (p_battle_units[target].moves == value)
            moves_were_full = 1;
        else
            value = p_battle_units[target].moves;
        Unit_Apply_Modifiers(&p_battle_units[target]);
        Battle_Unit_Combat_Modifiers(&p_battle_units[target]);
        moves_were_full = 0;                               /* BUG #268: cleared before the test below (see the header) */
        if (moves_were_full == 1)
            p_battle_units[target].moves = Unit_Combat_Moves(target);
        else
            p_battle_units[target].moves = value;
        break;
    case SPELL_FX_COMBAT_ENCH:
    case SPELL_FX_COMBAT_ENCH_VAR:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        if (spell != SPELL_FLAME_STRIKE && spell != SPELL_HOLY_WORD && spell != SPELL_DEATH_SPELL &&
            spell != SPELL_CALL_CHAOS && spell != SPELL_MASS_HEALING) {
            resist = p_spell_data[spell].effect_param;
            if (caster_player != combat_attacker_player)
                resist++;
            if (spell == SPELL_COUNTER_MAGIC)
                p_combat_enchants[resist] = mana;
            else
                p_combat_enchants[resist] = 1;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
                    moves_were_full = 0;
                    value = Unit_Combat_Moves(i);
                    if (p_battle_units[i].moves == value)
                        moves_were_full = 1;
                    else
                        value = p_battle_units[i].moves;
                    Unit_Apply_Modifiers(&p_battle_units[i]);
                    Battle_Unit_Combat_Modifiers(&p_battle_units[i]);
                    if (moves_were_full == 1)
                        p_battle_units[i].moves = Unit_Combat_Moves(i);
                    else
                        p_battle_units[i].moves = value;
                }
            }
        }
        break;
    case SPELL_FX_DISENCHANT:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        present_mode = PRESENT_DISSOLVE;
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
        present_mode = PRESENT_NORMAL;
        if (caster > CASTER_WIZARD - 1 && (&wizards[caster - CASTER_WIZARD].retorts[RETORT_RUNEMASTER])[0] > 0)
            mana *= 2;
        if (spell == SPELL_DISENCHANT_TRUE)
            mana *= 3;
        Disenchant_Area_Combat(caster, mana);
        for (i = 0; i < battle_unit_count; i++) {
            moves_were_full = 0;
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
                value = Unit_Combat_Moves(i);
                if (p_battle_units[i].moves == value)
                    moves_were_full = 1;
                else
                    value = p_battle_units[i].moves;
                Unit_Apply_Modifiers(&p_battle_units[i]);
                Battle_Unit_Combat_Modifiers(&p_battle_units[i]);
                if (moves_were_full == 1)
                    p_battle_units[i].moves = Unit_Combat_Moves(i);
                else
                    p_battle_units[i].moves = value;
            }
        }
        break;
    case SPELL_FX_DISPEL:
        Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        present_mode = PRESENT_DISSOLVE;
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
        present_mode = PRESENT_NORMAL;
        if (caster > CASTER_WIZARD - 1 && (&wizards[caster - CASTER_WIZARD].retorts[RETORT_RUNEMASTER])[0] > 0)
            mana *= 2;
        if (spell == SPELL_DISPEL_MAGIC_TRUE)
            mana *= 3;
        modifier = 0;
        Dispel_Square(x, y, caster, mana, &modifier);
        value = Unit_Combat_Moves(target);
        moves_were_full = 0;
        if (p_battle_units[target].moves == value)
            moves_were_full = 1;
        else
            value = p_battle_units[target].moves;
        Unit_Apply_Modifiers(&p_battle_units[target]);
        Battle_Unit_Combat_Modifiers(&p_battle_units[target]);
        if (moves_were_full == 1)
            p_battle_units[target].moves = Unit_Combat_Moves(target);
        else
            p_battle_units[target].moves = value;
        break;
    case SPELL_FX_CITY_ENCH:
        Combat_Wall_Create(spell, caster);
        break;
    case SPELL_FX_SPECIAL:
        if (spell != SPELL_WALL_OF_STONE && spell != SPELL_RAISE_DEAD && spell != SPELL_ANIMATE_DEAD)
            Combat_Spell_Animate(x, y, spell, caster_player, show_anim, caster);
        if (spell == SPELL_WALL_OF_STONE)
            Combat_Wall_Create(SPELL_WALL_OF_STONE, caster);
        if (spell == SPELL_WARP_WOOD) {
            p_battle_units[target].ammo = 0;
            p_battle_units[target].ranged_type = RANGED_NONE;
            value = Unit_Combat_Moves(target);
            moves_were_full = 0;
            if (p_battle_units[target].moves == value)
                moves_were_full = 1;
            else
                value = p_battle_units[target].moves;
            Unit_Apply_Modifiers(&p_battle_units[target]);
            Battle_Unit_Combat_Modifiers(&p_battle_units[target]);
            if (moves_were_full == 1)
                p_battle_units[target].moves = Unit_Combat_Moves(target);
            else
                p_battle_units[target].moves = value;
        }
        if (spell == SPELL_HEALING)
            Battle_Unit_Heal(target, 5, 0);
        if (spell == SPELL_CREATURE_BINDING) {
            resist = Roll_Resist(p_battle_units[target], modifier, p_spell_data[spell].realm);
            if (resist > 0) {
                p_battle_units[target].combat_flags = p_battle_units[target].combat_flags | COMBAT_FLAG_CREATURE_BINDING;
                if (p_battle_units[target].owner == combat_attacker_player)
                    p_battle_units[target].owner = combat_defender_player;
                else
                    p_battle_units[target].owner = combat_attacker_player;
            }
        }
        if (spell == SPELL_WARP_CREATURE) {
            resist = Roll_Resist(p_battle_units[target], modifier, p_spell_data[spell].realm);
            if (resist > 0)
                Warp_Creature_Apply(target);
            value = Unit_Combat_Moves(target);
            moves_were_full = 0;
            if (p_battle_units[target].moves == value)
                moves_were_full = 1;
            else
                value = p_battle_units[target].moves;
            Unit_Apply_Modifiers(&p_battle_units[target]);
            Battle_Unit_Combat_Modifiers(&p_battle_units[target]);
            if (moves_were_full == 1)
                p_battle_units[target].moves = Unit_Combat_Moves(target);
            else
                p_battle_units[target].moves = value;
        }
        if (spell == SPELL_RECALL_HERO || spell == SPELL_WORD_OF_RECALL)
            p_battle_units[target].status = BATTLE_UNIT_RECALLED;
        if (spell == SPELL_MAGIC_VORTEX)
            Vortex_Create(caster_player, x, y);
        if (spell == SPELL_EARTH_TO_MUD)
            Earth_To_Mud_Square(x, y);
        if (spell == SPELL_CRACKS_CALL)
            Cracks_Call_Square(x, y);
        if (spell == SPELL_DISRUPT)
            p_battlefield->wall_segments[y - BF_CITY_Y1][x - BF_CITY_X1] = WALL_BROKEN;
        if (spell == SPELL_RAISE_DEAD)
            Raise_Dead(caster_player, caster, x, y);
        if (spell == SPELL_ANIMATE_DEAD)
            Animate_Dead(caster_player, caster);
        break;
    case SPELL_FX_SUMMON:
        figures = Unit_Create(p_spell_data[spell].effect_param, caster_player, 0, 0, PLANE_COMBAT_ONLY, UNIT_SRC_NO_CAP);
        if (figures == 1) {
            Combat_Summon_Add_Unit(caster_player, num_units - 1, x, y);
            Combat_Summon_Anim(battle_unit_count - 1, x, y, spell, caster);
        }
        break;
    }
    Combat_Draw_Screen();
    Screen_Present();
}

/* (player) Holy Word's test: a fantastic enemy unit (race 15 and up) without Spell Lock - BUG #12: among units
   whose status is above 0 (not alive), and the result (99) is never returned (no return statement)
   Numbered: BUG #12 (BUGS_AND_QUIRKS.txt) */
int AI_Check_Holy_Word(int player)
{
    int i;
    int result;

    result = 0;
    for (i = 0; i < battle_unit_count && result == 0; i++) {
        if (p_battle_units[i].status > 0 && p_battle_units[i].owner != player &&
            (char)p_battle_units[i].race >= RACE_ARCANE_CREATURE && (p_battle_units[i].combat_enchantments & UNIT_ENCH_SPELL_LOCK) == 0 &&
            (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_SPELL_LOCK) == 0)
            result = 99;
    }
}
