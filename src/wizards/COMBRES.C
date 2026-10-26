/* COMBRES.C - WIZARDS.EXE overlay code segment a22c (stub 257b): combat resolution - the to-hit, block and
   resistance rolls, a unit's resistance bonus, an attack's damage flags, one exchange of blows (missile, thrown /
   breath / gaze, First Strike, counter-attack, Haste), resolving one attack figure by figure with its special
   attacks, the defender's shields, an attack's magic realm, the auras at the start of each combat turn (Combat_Start_Auras, from Combat_Turn_Start), a unit attacking a
   unit or a wall square, the distance between two units, the AI's score of a target, the missile-attack test and
   the Cloak of Fear count.
   This module's data: its literal ("message"); BSS DS:c970..c973 (defined here).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS a22c src/wizards/COMBRES.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Roll_To_Hit(int points, int to_hit);
int Roll_Blocks(int shields, int to_block);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
int Resist_Bonus(struct BattleUnit bu, int realm);
int Attack_Damage_Flags(int bu, int attack_kind);
void Combat_Exchange(int attacker, int defender, int *dmg_def, int *dmg_att, int ranged, int apply_effects);
int Attack_Resolve(int attacker, int figures, int defender, int attack_kind, int *dmg, int counter, int heal);
int Defender_Shields(int defender, int ranged_type, int damage_flags, int attack_flags, int realm);
int Attack_Realm(int ranged_type, int bu);
void Combat_Start_Auras(int battlefield_kind);
int Combat_Unit_Attack(int attacker, int target, int x, int y);
int Combat_Distance(int bu1, int bu2);
int AI_Target_Score(int attacker, int target, int ranged);
int Has_Shooting_Attack(int bu);
int Cloak_Of_Fear_Count(int attacker, int defender);

#include "GAME.H"

/* the damage immunity of a defender: weapon immunity (shields at least 10), total (50 shields) */
#define IMMUNE_WEAPON 1
#define IMMUNE_TOTAL 2

/* DS:c970..c973: the last BSS before COMBEND.C's; no module of this stretch (NEXTTURN.C .. COMBRES.C) owns it
   otherwise - defined here (owner not fixed by the EXE), highest address first */
int combat_winner;                              /* DS:c972  the winner from Combat_Count_Sides (-1: none yet) */
int combres_unused_c970;                        /* DS:c970  never referenced */

extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct HeroStats far *p_hero_stats[];    /* DS:9232  [player] the hero type table */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_auto;                         /* DS:c432 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int far *p_aura_leadership;              /* DS:c896 */
extern int far *p_aura_resistance;              /* DS:c89a */
extern int far *p_aura_holy_bonus;              /* DS:c89e */
extern int combat_turns_since_ai_acted;                   /* DS:c8b0 */
extern int combat_siege_flags;                  /* DS:c8b4 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
int Random(int n);
void Set_Draw_Page(void);
void Screen_Present(void);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
void Warning_Box(char *text);
void Combat_Draw_Screen(void);
void Battle_Unit_Heal(int bu, int amount, int overflow);
int In_City_Area(int bu);
int Is_City_Wall_Square(int x, int y);
void Wall_Of_Fire_Crossing(int unit);
int Unit_Combat_Moves(int bu);
void Combat_Missile_Anim(int attacker, int target, int hit, int x, int y);
void Combat_Melee_Anim(int attacker, int target, int attacker_hit, int target_hit, int x, int y);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
void Battle_Apply_Damage(int unit, int *damage);
int Ranged_Target_Blocked(int attacker, int target);

/* (points, to-hit bonus) -> hits: each point hits on Random(10) >= 8 - bonus, always on a 10 */
int Roll_To_Hit(int points, int to_hit)
{
    int roll;
    int i;
    int hits;

    hits = 0;
    for (i = 0; i < points; i++) {
        roll = Random(10);
        if (8 - to_hit <= roll || roll == 10)
            hits++;
    }
    return hits;
}

/* (shields, block bonus) -> blocks: each shield blocks on Random(10) >= 8 - bonus */
int Roll_Blocks(int shields, int to_block)
{
    int i;
    int blocks;

    blocks = 0;
    for (i = 0; i < shields; i++) {
        if (Random(10) >= 8 - to_block)
            blocks++;
    }
    return blocks;
}

/* (battle unit by value, modifier, realm) -> the margin by which a resistance roll fails: Random(10) above the
   resistance (with Resist_Bonus) + modifier, else 0 */
int Roll_Resist(struct BattleUnit bu, int modifier, int realm)
{
    int roll;
    int total;

    total = Resist_Bonus(bu, realm) + modifier;
    roll = Random(10);
    if (roll > total)
        return roll - total;
    else
        return 0;
}

/* (battle unit by value, realm or -1) -> its resistance against the realm: + 30 for a Charmed hero
   (HERO_ABILITY_CHARMED), + 30 Magic Immunity (any realm), + 30 Righteousness (chaos, death), Elemental Armor + 10 or Resist
   Elements + 3 (nature, chaos), Bless + 3 (chaos, death), Resist Magic + 5 (any realm)
   Numbered: BUG #257 (BUGS_AND_QUIRKS.txt) */
int Resist_Bonus(struct BattleUnit bu, int realm)
{
    int unit;
    long ench;
    int bonus;

    ench = p_units[bu.unit_index].enchantments | bu.combat_enchantments | bu.item_enchantments;
    bonus = bu.resist;
    unit = bu.unit_index;
    if (p_units[unit].hero_slot > -1 &&
        (p_hero_stats[p_units[unit].owner][p_units[unit].type].abilities & HERO_ABILITY_CHARMED))
        bonus += 30;
    if ((bu.abilities & ABILITY_MAGIC_IMMUNITY) && realm >= 0)
        bonus += 30;
    if ((ench & UNIT_ENCH_RIGHTEOUSNESS) && (realm == REALM_CHAOS || realm == REALM_DEATH))
        bonus += 30;
    if (realm == REALM_CHAOS || realm == REALM_NATURE) {
        if (ench & UNIT_ENCH_ELEMENTAL_ARMOR)
            bonus += 10;
        else if (ench & UNIT_ENCH_RESIST_ELEMENTS)
            bonus += 3;
    }
    if ((ench & UNIT_ENCH_BLESS) && (realm == REALM_CHAOS || realm == REALM_DEATH))
        bonus += 3;
    if ((ench & UNIT_ENCH_RESIST_MAGIC) && realm >= 0)
        bonus += 5;
    return bonus;
}

/* (battle unit, attack_kind: ATTACK_KIND_MELEE, else ranged) -> the attack's damage flags, as the Ability immunity bit
   that stops each (Defender_Shields: abilities & flags): illusion, an ordinary weapon (no magic weapon); ranged:
   missile (type 2x), fire breath (101), magic (3x), an ordinary weapon for a
   boulder or missile attack (ranged_type / 10 < 3: RANGED_NONE too) or a thrown one - BUG #236: the 'thrown' test reads ranged_type / 10 == 100, never true
   Numbered: BUG #236 (BUGS_AND_QUIRKS.txt) */
int Attack_Damage_Flags(int bu, int attack_kind)
{
    int flags;

    flags = 0;
    if ((p_battle_units[bu].attack_flags & ATTACK_ILLUSION) > 0)
        flags = flags | ABILITY_ILLUSIONS_IMMUNITY;
    if (attack_kind > 0) {
        if ((p_battle_units[bu].ranged_powers & ATTACK_ILLUSION) > 0)
            flags = flags | ABILITY_ILLUSIONS_IMMUNITY;
        if (p_battle_units[bu].ranged_type / 10 == RANGED_CLASS_MISSILE)
            flags = flags | ABILITY_MISSILES_IMMUNITY;
        if (p_battle_units[bu].ranged_type == RANGED_FIRE_BREATH)
            flags = flags | ABILITY_FIRE_IMMUNITY;
        if (p_battle_units[bu].ranged_type / 10 == RANGED_CLASS_MAGIC)
            flags = flags | ABILITY_MAGIC_IMMUNITY;
        if ((p_battle_units[bu].ranged_type / 10 < RANGED_CLASS_MAGIC || p_battle_units[bu].ranged_type / 10 == RANGED_THROWN) &&
            p_battle_units[bu].weapon_icon == WEAPON_NORMAL)
            flags = flags | ABILITY_WEAPON_IMMUNITY;
    } else {
        if (p_battle_units[bu].weapon_icon == WEAPON_NORMAL)
            flags = flags | ABILITY_WEAPON_IMMUNITY;
        if ((p_battle_units[bu].melee_powers & ATTACK_ILLUSION) > 0)
            flags = flags | ABILITY_ILLUSIONS_IMMUNITY;
    }
    return flags;
}

/* (attacker, defender, damage to the defender[3], damage to the attacker[3], ranged, apply_effects: 1 from
   Combat_Unit_Attack - gates the Wall of Fire crossing and the Cloak of Fear rolls, and goes on as
   Attack_Resolve's heal: Life Steal heals) one exchange.
   Ranged: a shot (no ranged strength or a blocked target: -5 per kind); Haste shoots again for 3 spell points (when
   more than 6) or 1 ammunition (when more than 1) - BUG #185: the magic test reads p_battle_units[3] (the loop index),
   not the attacker. Melee: the attacker's thrown / breath / gaze attack first (ranged_type 100
   and up; twice with Haste below 103); a defender's gaze (103 and
   up, not held) strikes with its surviving figures; Wall of Fire; then with First Strike (not negated) the attacker
   strikes with its figures less the Cloak of Fear and gaze losses before the defender's counter, else the counter
   (twice with Haste) and the attacker's blow are simultaneous: the counter's kills do not reduce the attacker's
   figures; Haste strikes again
   Numbered: BUG #185, #191, #192, #295 (BUGS_AND_QUIRKS.txt) */
void Combat_Exchange(int attacker, int defender, int *dmg_def, int *dmg_att, int ranged, int apply_effects)
{
    int dmg[3];
    int def_total;
    int figures;
    int blocked;
    int feared;                                 /* the Cloak of Fear count, plus the figures lost to the gaze (and,
                                                   without First Strike, front_damage) */
    int att_total;
    int again;
    int i;

    feared = 0;
    for (i = 0; i < 3; i++) {
        dmg_def[i] = 0;
        dmg_att[i] = 0;
    }
    att_total = 0;
    blocked = Ranged_Target_Blocked(attacker, defender);
    if (ranged == 1) {
        if (p_battle_units[attacker].ranged > 0 && blocked == 0) {
            Attack_Resolve(attacker, p_battle_units[attacker].figures_alive, defender, ATTACK_KIND_MISSILE, dmg, 0, apply_effects);
            for (i = 0; i < 3; i++)
                dmg_def[i] += dmg[i];
            if (p_battle_units[attacker].combat_flags & COMBAT_FLAG_HASTE) {
                again = 0;
                if (p_battle_units[i].ranged_type / 10 == RANGED_CLASS_MAGIC || (p_battle_units[attacker].abilities & ABILITY_CASTER)) {
                    if (p_battle_units[attacker].spell_points > 6) {
                        p_battle_units[attacker].spell_points = p_battle_units[attacker].spell_points - 3;
                        again = 1;
                    }
                } else if (p_battle_units[attacker].ammo > 1) {
                    again = 1;
                    p_battle_units[attacker].ammo = p_battle_units[attacker].ammo - 1;
                }
                if (again == 1) {
                    Attack_Resolve(attacker, p_battle_units[attacker].figures_alive, defender, ATTACK_KIND_MISSILE, dmg, 0,
                                   apply_effects);
                    for (i = 0; i < 3; i++)
                        dmg_def[i] += dmg[i];
                }
            }
        } else {
            for (i = 0; i < 3; i++)
                dmg_def[i] += -5;
        }
    } else {
        if (p_battle_units[attacker].ranged_type >= RANGED_THROWN) {
            Attack_Resolve(attacker, p_battle_units[attacker].figures_alive, defender, ATTACK_KIND_THROWN, dmg, 0, apply_effects);
            for (i = 0; i < 3; i++)
                dmg_def[i] += dmg[i];
            if ((p_battle_units[attacker].combat_flags & COMBAT_FLAG_HASTE) && p_battle_units[attacker].ranged_type < RANGED_STONING_GAZE) {
                Attack_Resolve(attacker, p_battle_units[attacker].figures_alive, defender, ATTACK_KIND_THROWN, dmg, 0, apply_effects);
                for (i = 0; i < 3; i++)
                    dmg_def[i] += dmg[i];
            }
        }
        if ((p_battle_units[defender].combat_flags & COMBAT_FLAG_BLACK_SLEEP) == 0 && p_battle_units[defender].ranged_type >= RANGED_STONING_GAZE) {
            def_total = 0;
            for (i = 0; i < 3; i++)
                def_total += dmg_def[i];
            def_total += p_battle_units[defender].front_damage;
            if (def_total > 0)
                figures = p_battle_units[defender].figures_alive - def_total / p_battle_units[defender].hits_per_figure;
            else
                figures = p_battle_units[defender].figures_alive;
            Attack_Resolve(defender, figures, attacker, ATTACK_KIND_THROWN, dmg, 1, apply_effects);
            for (i = 0; i < 3; i++) {
                dmg_att[i] += dmg[i];
                att_total += dmg[i];
            }
        }
        if (apply_effects != 0)
            Wall_Of_Fire_Crossing(attacker);
        if (p_battle_units[attacker].status == BATTLE_UNIT_ACTIVE) {
            if ((p_battle_units[attacker].attack_flags & ATTACK_FIRST_STRIKE) && (p_battle_units[defender].ability_flags & AFLAG_NEGATE_FIRST_STRIKE) == 0) {
                if (apply_effects == 1)
                    feared = Cloak_Of_Fear_Count(attacker, defender);
                for (i = 0; i < 3; i++)
                    feared += dmg_att[i] / p_battle_units[attacker].hits_per_figure;
                Attack_Resolve(attacker, p_battle_units[attacker].figures_alive - feared, defender, ATTACK_KIND_MELEE, dmg, 0,
                               apply_effects);
                for (i = 0; i < 3; i++)
                    dmg_def[i] += dmg[i];
            }
            if ((p_battle_units[defender].combat_flags & COMBAT_FLAG_BLACK_SLEEP) == 0) {
                def_total = 0;
                for (i = 0; i < 3; i++)
                    def_total += dmg_def[i];
                def_total += p_battle_units[defender].front_damage;
                if (def_total > 0)
                    figures = p_battle_units[defender].figures_alive - def_total /
                              p_battle_units[defender].hits_per_figure;
                else
                    figures = p_battle_units[defender].figures_alive;
                if (apply_effects == 1)
                    figures -= Cloak_Of_Fear_Count(attacker, defender);
                if (figures > 0) {
                    Attack_Resolve(defender, figures, attacker, ATTACK_KIND_MELEE, dmg, 1, apply_effects);
                    for (i = 0; i < 3; i++)
                        dmg_att[i] += dmg[i];
                    if (p_battle_units[defender].combat_flags & COMBAT_FLAG_HASTE) {
                        Attack_Resolve(defender, figures, attacker, ATTACK_KIND_MELEE, dmg, 1, apply_effects);
                        for (i = 0; i < 3; i++)
                            dmg_att[i] += dmg[i];
                    }
                }
            }
            if ((p_battle_units[attacker].attack_flags & ATTACK_FIRST_STRIKE) == 0 || (p_battle_units[defender].ability_flags & AFLAG_NEGATE_FIRST_STRIKE)) {
                if (apply_effects == 1)
                    feared = Cloak_Of_Fear_Count(attacker, defender);
                att_total += p_battle_units[attacker].front_damage;
                feared += att_total / p_battle_units[attacker].hits_per_figure;
                Attack_Resolve(attacker, p_battle_units[attacker].figures_alive - feared, defender, ATTACK_KIND_MELEE, dmg, 0,
                               apply_effects);
                for (i = 0; i < 3; i++)
                    dmg_def[i] += dmg[i];
            }
            if (p_battle_units[attacker].combat_flags & COMBAT_FLAG_HASTE) {
                Attack_Resolve(attacker, p_battle_units[attacker].figures_alive - feared, defender, ATTACK_KIND_MELEE, dmg, 0,
                               apply_effects);
                for (i = 0; i < 3; i++)
                    dmg_def[i] += dmg[i];
            }
        }
    }
}

/* (attacker, attacking figures, defender, kind 0 melee / 1 thrown, breath or gaze / 2 missile, damage[3] out,
   counter, heal) one attack, figure by figure -> 0 without figures. To-hit: the attacker's, melee + its hit bonus -
   the defender's block bonus, missile + its ranged hit bonus - 1 per 3 squares for non-magic shots (Long Range: at
   most 1); -1 against an invisible defender without Illusions Immunity; a counter-attack - times attacked / 2. A
   walking melee attacker cannot reach a flyer (counters and thrown / breath attacks can). Before the figures: an
   Immolation, the stoning (103, 104) and death (104, 105) gazes per defending figure; a held defender is hit
   automatically. Per figure: Dispel Evil against chaos and death units (-4, undead -9), Stoning Touch, Death
   Touch, Life Steal (heals the attacker), Destruction; the to-hit roll (automatic hits: full points for a held target or a Doom gaze, half when
   the automatic bit comes from the melee / ranged powers), Blur, the
   blocks per defending figure (Invulnerability -2); touch attacks roll the touch strength against resistance.
   Damage kinds: 0 normal, 1 drain (a Create Undead attacker, Life Steal), 2
   irrecoverable. No value otherwise
   Numbered: BUG #186, #187, #188, #214, #258 (BUGS_AND_QUIRKS.txt) */
int Attack_Resolve(int attacker, int figures, int defender, int attack_kind, int *dmg, int counter, int heal)
{
    int f;
    int j;
    int k;
    int hits;
    int range_pen;
    int shields;
    int ranged_type;
    int realm;
    int points;
    int attack_flags;
    int powers;
    int damage_flags;
    int to_hit;                                 /* the attacker's to-hit after the modifiers: Roll_To_Hit's */
    int unused;
    int block_bonus;
    int total;
    int front;
    int pre_dmg[3];
    long ench;
    int modifier;

    front = p_battle_units[defender].front_damage;
    ench = p_units[p_battle_units[defender].unit_index].enchantments | p_battle_units[defender].combat_enchantments |
           p_battle_units[defender].item_enchantments;
    unused = 0;
    points = 0;
    hits = 0;
    to_hit = p_battle_units[attacker].to_hit;
    for (f = 0; f < 3; f++)
        dmg[f] = 0;
    if (figures <= 0)
        return 0;
    attack_flags = p_battle_units[attacker].attack_flags;
    block_bonus = p_battle_units[defender].block_bonus;
    damage_flags = Attack_Damage_Flags(attacker, attack_kind);
    if (attack_kind > 0) {
        attack_flags = attack_flags | p_battle_units[attacker].ranged_powers;
        powers = p_battle_units[attacker].ranged_powers;
        points = p_battle_units[attacker].ranged;
        realm = Attack_Realm(p_battle_units[attacker].ranged_type, attacker);
        ranged_type = p_battle_units[attacker].ranged_type;
        if (ranged_type == RANGED_LIGHTNING)
            attack_flags = attack_flags | ATTACK_ARMOR_PIERCING;
        if (ranged_type == RANGED_DOOM_GAZE)
            attack_flags = attack_flags | ATTACK_AUTOMATIC_DAMAGE;
        if (attack_kind == 2) {
            if (Has_Shooting_Attack(attacker)) {
                to_hit += p_battle_units[attacker].ranged_hit_bonus;
                if (p_battle_units[attacker].ranged_type / 10 != RANGED_CLASS_MAGIC) {
                    range_pen = Combat_Distance(attacker, defender) / 3;
                    if ((p_battle_units[attacker].ability_flags & AFLAG_LONG_RANGE) && range_pen > 0)
                        range_pen = 1;
                    to_hit -= range_pen;
                }
            } else
                points = 0;
        }
    } else {
        attack_flags = attack_flags | p_battle_units[attacker].melee_powers;
        powers = p_battle_units[attacker].melee_powers;
        to_hit = to_hit + (p_battle_units[attacker].melee_hit_bonus - p_battle_units[defender].block_bonus);
        points = p_battle_units[attacker].melee;
        realm = Attack_Realm(0, attacker);
        ranged_type = 0;
    }
    for (f = 0; f < 3; f++)
        pre_dmg[f] = 0;
    if (((ench & UNIT_ENCH_INVISIBILITY) || (p_battle_units[defender].ability_flags & AFLAG_INVISIBILITY)) &&
        (p_battle_units[attacker].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0)
        to_hit--;
    if (counter == 1)
        to_hit -= p_battle_units[attacker].times_attacked / 2;
    if ((p_battle_units[defender].move_flags & MOVE_FLYING) == 0 || attack_kind != 0 || (p_battle_units[attacker].move_flags & MOVE_FLYING) ||
        counter == 1 || p_battle_units[attacker].ranged_type >= RANGED_THROWN) {
        if ((p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) == 0) {
            if (p_battle_units[attacker].abilities2 & ABILITY2_IMMOLATION)
                Spell_Damage_Roll(SPELL_FIREBALL, defender, pre_dmg, 4);
            if ((ranged_type == RANGED_STONING_GAZE || ranged_type == RANGED_DOOM_GAZE) && (p_battle_units[defender].abilities & ABILITY_STONING_IMMUNITY) == 0) {
                for (f = 0; f < p_battle_units[defender].figures_alive; f++) {
                    if (Roll_Resist(p_battle_units[defender], -abs(p_battle_units[attacker].special_strength), REALM_NATURE) > 0)
                        pre_dmg[DAMAGE_DESTROY] += p_battle_units[defender].hits_per_figure;
                }
            }
            if ((ranged_type == RANGED_DOOM_GAZE || ranged_type == RANGED_DEATH_GAZE) && (p_battle_units[defender].abilities & ABILITY_DEATH_IMMUNITY) == 0) {
                for (f = 0; f < p_battle_units[defender].figures_alive; f++) {
                    if (Roll_Resist(p_battle_units[defender], -abs(p_battle_units[attacker].special_strength), REALM_DEATH) > 0)
                        pre_dmg[DAMAGE_NORMAL] += p_battle_units[defender].hits_per_figure;
                }
            }
        }
        if (p_battle_units[defender].combat_flags & COMBAT_FLAG_BLACK_SLEEP)
            attack_flags = attack_flags | ATTACK_AUTOMATIC_DAMAGE;
        if (points > 0) {
            shields = Defender_Shields(defender, ranged_type, damage_flags, attack_flags, realm);
            if (attack_flags & ATTACK_ELDRITCH_WEAPON)
                block_bonus--;
            if (In_City_Area(defender) == 1 && In_City_Area(attacker) == 0 && p_battlefield->has_city_walls != 0) {
                if (Is_City_Wall_Square(p_battle_units[defender].combat_x, p_battle_units[defender].combat_y))
                    shields += 3;
                else
                    shields++;
            }
            for (f = 0; f < figures; f++) {
                if ((p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) == 0) {
                    if ((attack_flags & ATTACK_DISPEL_EVIL) &&
                        (p_battle_units[defender].race == RACE_CHAOS_CREATURE || p_battle_units[defender].race == RACE_DEATH_CREATURE)) {
                        modifier = -4;
                        if (p_units[p_battle_units[defender].unit_index].mutation_flags & MUT_UNDEAD)
                            modifier -= 5;
                        if (Roll_Resist(p_battle_units[defender], modifier, REALM_LIFE) > 0)
                            pre_dmg[DAMAGE_DESTROY] += p_battle_units[defender].hits_per_figure;
                    }
                    if ((attack_flags & ATTACK_STONING_TOUCH) && (p_battle_units[defender].abilities & ABILITY_STONING_IMMUNITY) == 0) {
                        modifier = -abs(p_battle_units[attacker].special_strength);
                        if (powers & ATTACK_STONING_TOUCH)
                            modifier--;
                        if (Roll_Resist(p_battle_units[defender], modifier, REALM_NATURE) > 0)
                            pre_dmg[DAMAGE_DESTROY] += p_battle_units[defender].hits_per_figure;
                    }
                    if ((attack_flags & ATTACK_DEATH_TOUCH) && (p_battle_units[defender].abilities & ABILITY_DEATH_IMMUNITY) == 0) {
                        modifier = -abs(p_battle_units[attacker].special_strength);
                        if (powers & ATTACK_DEATH_TOUCH)
                            modifier -= 3;
                        if (Roll_Resist(p_battle_units[defender], modifier, REALM_DEATH) > 0)
                            pre_dmg[DAMAGE_NORMAL] += p_battle_units[defender].hits_per_figure;
                    }
                    if ((attack_flags & ATTACK_LIFE_STEAL) && (p_battle_units[defender].abilities & ABILITY_DEATH_IMMUNITY) == 0) {
                        modifier = -abs(p_battle_units[attacker].special_strength);
                        j = Roll_Resist(p_battle_units[defender], modifier, REALM_DEATH);
                        pre_dmg[DAMAGE_DRAIN] += j;
                        if (heal != 0)
                            Battle_Unit_Heal(attacker, j, 1);
                    }
                    if (attack_flags & ATTACK_DESTRUCTION) {
                        if (Roll_Resist(p_battle_units[defender], 0, REALM_CHAOS) > 0)
                            pre_dmg[DAMAGE_DESTROY] += p_battle_units[defender].hits_per_figure;
                    }
                }
                if (attack_flags & ATTACK_AUTOMATIC_DAMAGE) {
                    if (powers & ATTACK_AUTOMATIC_DAMAGE)
                        hits = points / 2;
                    else
                        hits = points;
                } else {
                    hits += Roll_To_Hit(points, to_hit);
                    if ((p_battle_units[defender].owner == combat_attacker_player && p_combat_enchants[CENCH_BLUR_ATT] > 0 &&
                         (p_battle_units[defender].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0) ||
                        (p_battle_units[defender].owner == combat_defender_player && p_combat_enchants[CENCH_BLUR_DEF] > 0 &&
                         (p_battle_units[defender].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0)) {
                        for (k = 0; k < hits; k++) {
                            if (Random(10) == 10)
                                hits--;
                        }
                    }
                    hits -= Roll_Blocks(shields, block_bonus);
                    if (ench & UNIT_ENCH_INVULNERABILITY)
                        hits -= 2;
                    if (hits < 0)
                        hits = 0;
                }
                if ((p_battle_units[attacker].attack_flags & ATTACK_POISON) && (p_battle_units[defender].abilities & ABILITY_POISON_IMMUNITY) == 0) {
                    for (j = 0; j < p_battle_units[attacker].touch_strength; j++) {
                        if (Roll_Resist(p_battle_units[defender], 0, REALM_NONE) > 0) {
                            if ((p_battle_units[attacker].ability_flags & AFLAG_CREATE_UNDEAD) &&
                                (p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) == 0)
                                pre_dmg[DAMAGE_DRAIN]++;
                            else
                                pre_dmg[DAMAGE_NORMAL]++;
                        }
                    }
                }
                total = 0;
                for (j = 0; j < 3; j++)
                    total += pre_dmg[j];
                total += front;
                while (p_battle_units[defender].hits_per_figure <= hits + total) {
                    hits -= p_battle_units[defender].hits_per_figure;
                    if (hits < 0) {
                        if ((p_battle_units[attacker].ability_flags & AFLAG_CREATE_UNDEAD) &&
                            (p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) == 0)
                            dmg[DAMAGE_DRAIN] += p_battle_units[defender].hits_per_figure + hits;
                        else
                            dmg[DAMAGE_NORMAL] += p_battle_units[defender].hits_per_figure + hits;
                        total += hits;
                        hits = 0;
                    } else {
                        if ((p_battle_units[attacker].ability_flags & AFLAG_CREATE_UNDEAD) &&
                            (p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) == 0)
                            dmg[DAMAGE_DRAIN] += p_battle_units[defender].hits_per_figure;
                        else
                            dmg[DAMAGE_NORMAL] += p_battle_units[defender].hits_per_figure;
                        if ((attack_flags & ATTACK_AUTOMATIC_DAMAGE) == 0) {
                            hits -= Roll_Blocks(shields, block_bonus);
                            if (ench & UNIT_ENCH_INVULNERABILITY)
                                hits -= 2;
                        }
                        if (hits < 0)
                            hits = 0;
                    }
                }
                if ((p_battle_units[attacker].ability_flags & AFLAG_CREATE_UNDEAD) && (p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) == 0)
                    dmg[DAMAGE_DRAIN] += hits;
                else
                    dmg[DAMAGE_NORMAL] += hits;
                front = total + hits;
                hits = 0;
                unused = 0;
                for (j = 0; j < 3; j++) {
                    dmg[j] += pre_dmg[j];
                    pre_dmg[j] = 0;
                }
            }
        }
    }
}

/* (defender, ranged type or 0 for melee, damage flags, attack flags, realm or -1) -> the defender's shields
   against this attack: 0 against an illusion it cannot see through; Large Shield + 2 against ranged attacks;
   weapon immunities (damage flags it is immune to) and Magic Immunity -> 50 against ranged attacks, Weapon
   Immunity at least 10; Bless + 3 and Righteousness 50 (chaos, death; Righteousness
   against ranged attacks only and from unit or combat enchantments only); Elemental Armor + 10 or Resist Elements + 3
   (nature, chaos, ranged only); armour piercing (attack flag 1) halves them
   Numbered: BUG #189, #215 (BUGS_AND_QUIRKS.txt) */
int Defender_Shields(int defender, int ranged_type, int damage_flags, int attack_flags, int realm)
{
    long ench;
    int immune;                                 /* 1 weapon immunity (shields at least 10), 2 total (50) */
    int shields;

    immune = 0;
    ench = p_units[p_battle_units[defender].unit_index].enchantments | p_battle_units[defender].combat_enchantments |
           p_battle_units[defender].item_enchantments;
    shields = p_battle_units[defender].defense;
    if (damage_flags & p_battle_units[defender].abilities & ABILITY_ILLUSIONS_IMMUNITY)
        damage_flags = damage_flags ^ ABILITY_ILLUSIONS_IMMUNITY;
    if (damage_flags & ABILITY_ILLUSIONS_IMMUNITY)   /* an illusion: no shields */
        return 0;
    if ((p_battle_units[defender].ability_flags & AFLAG_LARGE_SHIELD) > 0 && ranged_type != 0)
        shields += 2;
    if ((p_battle_units[defender].abilities & damage_flags) && ranged_type != 0)
        immune = IMMUNE_TOTAL;
    if (damage_flags & p_battle_units[defender].abilities & ABILITY_WEAPON_IMMUNITY)
        immune = IMMUNE_WEAPON;
    if ((p_battle_units[defender].abilities & ABILITY_MAGIC_IMMUNITY) && realm > -1 && ranged_type != 0)
        immune = IMMUNE_TOTAL;
    if (realm == REALM_CHAOS || realm == REALM_DEATH) {
        if (ench & UNIT_ENCH_BLESS)
            shields += 3;
        if (((p_units[p_battle_units[defender].unit_index].enchantments & UNIT_ENCH_RIGHTEOUSNESS) ||
             (p_battle_units[defender].combat_enchantments & UNIT_ENCH_RIGHTEOUSNESS)) && ranged_type != 0)
            immune = IMMUNE_TOTAL;
    }
    if ((realm == REALM_CHAOS || realm == REALM_NATURE) && ranged_type != 0) {
        if (ench & UNIT_ENCH_ELEMENTAL_ARMOR)
            shields += 10;
        else if (ench & UNIT_ENCH_RESIST_ELEMENTS)
            shields += 3;
    }
    if (attack_flags & ATTACK_ARMOR_PIERCING)
        shields = shields / 2;
    if (immune == IMMUNE_WEAPON && shields < 10)
        shields = 10;
    if (immune == IMMUNE_TOTAL)
        return 50;
    return shields;
}

/* (ranged type or 0 for melee, battle unit) -> the attack's realm: melee by the unit's race (normal races -1,
   race 15 -> 5, fantastic races 16.. -> race - 16), magic missiles 30 / 31 / 33 / 36 chaos 2, 32 sorcery 1,
   34 / 35 / 37 / 38 nature 0, breath 101 / 102 chaos, gazes 103 nature 0, 104 chaos 2, 105 death 4; -1 else */
int Attack_Realm(int ranged_type, int bu)
{
    int realm;

    realm = REALM_NONE;
    switch (ranged_type) {
    case 0:
        if ((char)p_battle_units[bu].race < RACE_ARCANE_CREATURE)
            realm = REALM_NONE;
        else if (p_battle_units[bu].race == RACE_ARCANE_CREATURE)
            realm = REALM_ARCANE;
        else
            realm = (char)p_battle_units[bu].race - RACE_NATURE_CREATURE;
        break;
    case RANGED_BOULDER:
        realm = REALM_NONE;
        break;
    case RANGED_UNUSED_11:
        realm = REALM_NONE;
        break;
    case RANGED_ARROW:
        realm = REALM_NONE;
        break;
    case RANGED_SLING:
        realm = REALM_NONE;
        break;
    case 22:
        realm = REALM_NONE;
        break;
    case RANGED_MAGIC_LIGHTNING:
        realm = REALM_CHAOS;
        break;
    case RANGED_MAGIC_FIRE:
        realm = REALM_CHAOS;
        break;
    case RANGED_MAGIC_ILLUSION:
        realm = REALM_SORCERY;
        break;
    case RANGED_MAGIC_DEATH:
        realm = REALM_CHAOS;
        break;
    case RANGED_MAGIC_WIND:
        realm = REALM_NATURE;
        break;
    case RANGED_MAGIC_HOLY:
        realm = REALM_NATURE;
        break;
    case RANGED_MAGIC_DARK:
        realm = REALM_CHAOS;
        break;
    case RANGED_MAGIC_SPRITE:
        realm = REALM_NATURE;
        break;
    case RANGED_MAGIC_NATURE:
        realm = REALM_NATURE;
        break;
    case RANGED_THROWN:
        realm = REALM_NONE;
        break;
    case RANGED_FIRE_BREATH:
        realm = REALM_CHAOS;
        break;
    case RANGED_LIGHTNING:
        realm = REALM_CHAOS;
        break;
    case RANGED_STONING_GAZE:
        realm = REALM_NATURE;
        break;
    case RANGED_DOOM_GAZE:
        realm = REALM_CHAOS;
        break;
    case RANGED_DEATH_GAZE:
        realm = REALM_DEATH;
        break;
    }
    return realm;
}

/* (battlefield kind: 1 only from Combat_Strategic's auto-resolved city battles) the city's Cloud of Shadow (6) is recorded
   as the defender's darkness, Heavenly Light (21) lights it; a wizard's Eternal Night is recorded in the
   _ATT / _DEF darkness slot of the side that owns it (both for a third party), unless the defender has Heavenly Light. Kind 1 gives every defender + 3
   defense, walls or not (ReMoM reads it as a slip for a City Walls test; clarified using ReMoM repo). Per side the best Holy Bonus and resistance aura (abilities2 0x80 / 0x40: special strength; hero
   abilities 0x1000000: level + 1, 0x2000000: (level + 1) x 3 / 2) and Leadership (hero 1: (level + 1) / 3, 2:
   (level + 1) / 2); the resistance aura includes the Holy Bonus
   Numbered: BUG #261, #289 (BUGS_AND_QUIRKS.txt) */
void Combat_Start_Auras(int battlefield_kind)
{
    int resist;
    int leadership;
    int i;
    int owner;

    if (p_battlefield->city_enchants[CITY_ENCH_CLOUD_OF_SHADOW] > 0)
        p_combat_enchants[CENCH_DARKNESS_DEF] = CENCH_FROM_CITY;
    if (p_battlefield->city_enchants[CITY_ENCH_HEAVENLY_LIGHT] > 0)
        p_combat_enchants[CENCH_TRUE_LIGHT_DEF] = CENCH_FROM_CITY;
    for (i = 0; i < num_players; i++) {
        if (wizards[i].globals.eternal_night > 0 && p_combat_enchants[CENCH_TRUE_LIGHT_DEF] != CENCH_FROM_CITY) {
            if (i == combat_attacker_player)
                p_combat_enchants[CENCH_DARKNESS_ATT] = CENCH_FROM_ETERNAL_NIGHT;
            else if (i == combat_defender_player)
                p_combat_enchants[CENCH_DARKNESS_DEF] = CENCH_FROM_ETERNAL_NIGHT;
            else {
                p_combat_enchants[CENCH_DARKNESS_DEF] = CENCH_FROM_ETERNAL_NIGHT;
                p_combat_enchants[CENCH_DARKNESS_ATT] = CENCH_FROM_ETERNAL_NIGHT;
            }
        }
    }
    for (i = 0; i < 7; i++) {                   /* the aura slots: players 0 .. 6 (PLAYER_NO_WIZARD too) */
        p_aura_holy_bonus[i] = 0;
        p_aura_resistance[i] = 0;
        p_aura_leadership[i] = 0;
    }
    for (i = 0; i < battle_unit_count; i++) {
        leadership = 0;
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            if (battlefield_kind == BATTLEFIELD_CITY && p_battle_units[i].owner == combat_defender_player)
                p_battle_units[i].defense = p_battle_units[i].defense + 3;
            owner = p_battle_units[i].owner;
            if ((p_battle_units[i].abilities2 & ABILITY2_HOLY_BONUS) && p_battle_units[i].special_strength > p_aura_holy_bonus[owner])
                p_aura_holy_bonus[owner] = p_battle_units[i].special_strength;
            if ((p_battle_units[i].abilities2 & ABILITY2_RESISTANCE_TO_ALL) && p_battle_units[i].special_strength > p_aura_resistance[owner])
                p_aura_resistance[owner] = p_battle_units[i].special_strength;
            if (p_units[p_battle_units[i].unit_index].hero_slot >= 0) {
                if (p_hero_stats[owner][p_units[p_battle_units[i].unit_index].type].abilities & HERO_ABILITY_PRAYERMASTER) {
                    resist = p_units[p_battle_units[i].unit_index].level + 1;
                    if (p_aura_resistance[owner] < resist)
                        p_aura_resistance[owner] = resist;
                }
                if (p_hero_stats[owner][p_units[p_battle_units[i].unit_index].type].abilities & HERO_ABILITY_SUPER_PRAYERMASTER) {
                    resist = (p_units[p_battle_units[i].unit_index].level + 1) * 3 / 2;
                    if (p_aura_resistance[owner] < resist)
                        p_aura_resistance[owner] = resist;
                }
                if (p_hero_stats[owner][p_units[p_battle_units[i].unit_index].type].abilities & HERO_ABILITY_LEADERSHIP)
                    leadership = (p_units[p_battle_units[i].unit_index].level + 1) / 3;
                if (p_hero_stats[owner][p_units[p_battle_units[i].unit_index].type].abilities & HERO_ABILITY_SUPER_LEADERSHIP)
                    leadership = (p_units[p_battle_units[i].unit_index].level + 1) / 2;
                if (p_aura_leadership[owner] < leadership)
                    p_aura_leadership[owner] = leadership;
            }
        }
    }
    for (i = 0; i < num_players; i++)
        p_aura_resistance[i] += p_aura_holy_bonus[i];
}

/* (attacker, target unit or CLICK_WALL (99) for a wall square, x, y) a unit's attack: at range a target it cannot see
   (Ranged_Target_Blocked 1) or one behind a Wall of Darkness (2) is refused (a message for the human) -> 0;
   adjacent targets are shot only when ranged > melee / 2, the unit has a missile attack and the target is not
   Missile (2x) or Magic (3x) Immune. Combat_Exchange; the target's times_attacked + 1; a shot costs 20 moves and 3
   spell points for heroes and casters with magic shots (none left below 3) or 1 ammunition, a melee attack half the
   unit's moves; both sides take their damage. Wall Crusher (ability_flags 8, not the defender) on a wall square
   breaches it on Random(100) <= 25 at range, <= 50 adjacent (attacking the wall square itself plays the attack and
   pays for it - BUG #447: magic shots there stop below 5 spell points, and the hit shown is uninitialised)
   Numbered: BUG #190, #284, #348 (BUGS_AND_QUIRKS.txt) */
int Combat_Unit_Attack(int attacker, int target, int x, int y)
{
    int k;
    int att_total;
    int def_total;
    int def_dmg[3];
    int att_dmg[3];
    int ranged;                                 /* the distance first, then reused as the 0 / 1 shoot flag */
    int blocked;
    int breach;

    if (p_battle_units[attacker].owner != PLAYER_HUMAN)
        combat_turns_since_ai_acted = -1;
    if (target != CLICK_WALL) {
        x = p_battle_units[target].combat_x;
        y = p_battle_units[target].combat_y;
        blocked = Ranged_Target_Blocked(attacker, target);
        ranged = Combat_Distance(attacker, target);
        if (ranged > 1) {
            if (blocked != 0) {
                if (blocked == 1) {
                    if (p_battle_units[attacker].owner == PLAYER_HUMAN && combat_auto == 0) {
                        LBX_Load_Records_Near("message", 0, message_text, 3, 1, 150);   /* MESSAGE.LBX record 3 */
                        Warning_Box(message_text);
                    }
                    return 0;
                }
                if (blocked == 2) {
                    if (p_battle_units[attacker].owner == PLAYER_HUMAN && combat_auto == 0) {
                        LBX_Load_Records_Near("message", 0, message_text, 4, 1, 150);
                        Warning_Box(message_text);
                    }
                    return 0;
                }
            } else
                ranged = 1;
        } else {
            if (p_battle_units[attacker].ranged > p_battle_units[attacker].melee / 2 && Has_Shooting_Attack(attacker)) {
                if ((p_battle_units[attacker].ranged_type / 10 == RANGED_CLASS_MISSILE && (p_battle_units[target].abilities & ABILITY_MISSILES_IMMUNITY)) ||
                    (p_battle_units[attacker].ranged_type / 10 == RANGED_CLASS_MAGIC && (p_battle_units[target].abilities & ABILITY_MAGIC_IMMUNITY)))
                    ranged = 0;
                else
                    ranged = 1;
            } else
                ranged = 0;
        }
        if (blocked > 0)
            ranged = 0;
        p_battle_units[attacker].dest_x = p_battle_units[target].combat_x;
        p_battle_units[attacker].dest_y = p_battle_units[target].combat_y;
        Combat_Exchange(attacker, target, def_dmg, att_dmg, ranged, 1);
        p_battle_units[target].times_attacked = p_battle_units[target].times_attacked + 1;
        att_total = 0;
        def_total = 0;
        for (k = 0; k < 3; k++) {
            att_total += att_dmg[k];
            def_total += def_dmg[k];
        }
        if (ranged == 1) {
            Combat_Missile_Anim(attacker, target, def_total, x, y);
            p_battle_units[attacker].moves = p_battle_units[attacker].moves - 20;   /* 10 moves: a shot ends the turn */
            if (p_battle_units[attacker].moves < 0)
                p_battle_units[attacker].moves = 0;
            if ((p_units[p_battle_units[attacker].unit_index].hero_slot > -1 ||
                 (p_battle_units[attacker].abilities & ABILITY_CASTER)) && p_battle_units[attacker].ranged_type / 10 == RANGED_CLASS_MAGIC) {
                if (p_battle_units[attacker].spell_points > 3)
                    p_battle_units[attacker].spell_points = p_battle_units[attacker].spell_points - 3;
                else
                    p_battle_units[attacker].spell_points = 0;
                if (p_battle_units[attacker].spell_points < 3) {
                    p_battle_units[attacker].ranged_type = RANGED_NONE;
                    p_battle_units[attacker].ranged = 0;
                }
            } else {
                p_battle_units[attacker].ammo = p_battle_units[attacker].ammo - 1;
                if (p_battle_units[attacker].ammo <= 0) {
                    p_battle_units[attacker].ranged_type = RANGED_NONE;
                    p_battle_units[attacker].ranged = 0;
                }
            }
        } else {
            Combat_Melee_Anim(attacker, target, att_total, def_total, p_battle_units[target].combat_x,
                              p_battle_units[target].combat_y);
            p_battle_units[attacker].moves = p_battle_units[attacker].moves - (Unit_Combat_Moves(attacker) + 1) / 2;
            if (p_battle_units[attacker].moves < 0)
                p_battle_units[attacker].moves = 0;
        }
        Battle_Apply_Damage(target, def_dmg);
        Battle_Apply_Damage(attacker, att_dmg);
    }
    breach = 0;
    if ((p_battle_units[attacker].ability_flags & AFLAG_WALL_CRUSHER) && p_battle_units[attacker].owner != combat_defender_player &&
        Is_City_Wall_Square(x, y)) {
        if (target == CLICK_WALL) {
            if (abs(p_battle_units[attacker].combat_x - x) > 1 || abs(p_battle_units[attacker].combat_y - y) > 1) {
                if (Random(100) <= 25)
                    breach = 1;
                Combat_Missile_Anim(attacker, target, def_total, x, y);
                p_battle_units[attacker].moves = p_battle_units[attacker].moves - 20;   /* 10 moves: a shot ends the turn */
                if (p_units[p_battle_units[attacker].unit_index].hero_slot > -1 &&
                    p_battle_units[attacker].ranged_type / 10 == RANGED_CLASS_MAGIC) {
                    if (p_battle_units[attacker].spell_points > 3)
                        p_battle_units[attacker].spell_points = p_battle_units[attacker].spell_points - 3;
                    else
                        p_battle_units[attacker].spell_points = 0;
                    if (p_battle_units[attacker].spell_points < 5) {
                        p_battle_units[attacker].ranged_type = RANGED_NONE;
                        p_battle_units[attacker].ranged = 0;
                    }
                } else {
                    p_battle_units[attacker].ammo = p_battle_units[attacker].ammo - 1;
                    if (p_battle_units[attacker].ammo <= 0) {
                        p_battle_units[attacker].ranged_type = RANGED_NONE;
                        p_battle_units[attacker].ranged = 0;
                    }
                }
            } else {
                if (Random(100) <= 50)
                    breach = 1;
                Combat_Melee_Anim(attacker, target, att_total, def_total, x, y);
                p_battle_units[attacker].moves = p_battle_units[attacker].moves -
                                                 (Unit_Combat_Moves(attacker) + 1) / 2;
            }
        } else {
            if (abs(p_battle_units[attacker].combat_x - x) > 1 || abs(p_battle_units[attacker].combat_y - y) > 1) {
                if (Random(100) <= 25)
                    breach = 1;
            } else {
                if (Random(100) <= 50)
                    breach = 1;
            }
        }
        if (breach != 0)
            p_battlefield->wall_segments[y - BF_CITY_Y1][x - BF_CITY_X1] = WALL_BROKEN;
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
}

/* (bu1, bu2) -> the distance between two battle units: the larger of the x and y differences */
int Combat_Distance(int bu1, int bu2)
{
    int dx;
    int dy;

    dx = abs(p_battle_units[bu1].combat_x - p_battle_units[bu2].combat_x);
    dy = abs(p_battle_units[bu1].combat_y - p_battle_units[bu2].combat_y);
    if (dx > dy)
        return dx;
    else
        return dy;
}

/* (attacker, target, ranged) -> the AI's score for attacking the target: -200 for a flyer a walking unit with no
   ranged attack at all (RANGED_NONE) cannot reach; else the attack strength - the target's shields, + the target's ranged / 3 + 2
   when it shoots, - 10 when it is confused (combat_flags 2); a defender in a node battle (backdrop 9+; the attacker < 40
   test is always true: 36 battle units) + 3 against targets with ability_flags 0x400, else - 10 when its defense / 2 beats the target's attack;
   + 10 for its current target; attacking walking units of a siege: a wall square: score = -20 (the gate (8, 12) + 5), the
   squares (6-7, 11-12): score = -20; behind a Wall of Fire: score = -20 when a Fireball-strength hit would do more than 1 damage;
   + 20 for a held target at range; a melee target within its moves (distance x 2 <= moves: half moves) + 25, + 20
   more when held
   Numbered: BUG #313 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Score(int attacker, int target, int ranged)
{
    int points;
    int shields;
    int score;
    int target_attack;
    int range_pen;
    int ranged_type;
    int realm;
    int powers;
    int attack_flags;
    int damage_flags;
    int to_hit;                                 /* computed, never used (nor powers) */
    int wall_dmg[3];

    score = -100;
    if ((p_battle_units[target].move_flags & MOVE_FLYING) && (p_battle_units[attacker].move_flags & MOVE_FLYING) == 0 &&
        p_battle_units[attacker].ranged_type == RANGED_NONE)
        return -200;
    target_attack = p_battle_units[target].ranged > p_battle_units[target].melee ? p_battle_units[target].ranged :
                    p_battle_units[target].melee;
    attack_flags = p_battle_units[attacker].attack_flags;
    to_hit = p_battle_units[attacker].to_hit;
    if (ranged != 0) {
        damage_flags = Attack_Damage_Flags(attacker, ranged);
        attack_flags = attack_flags | p_battle_units[attacker].ranged_powers;
        points = p_battle_units[attacker].ranged;
        realm = Attack_Realm(p_battle_units[attacker].ranged_type, attacker);
        ranged_type = p_battle_units[attacker].ranged_type;
        to_hit += p_battle_units[attacker].ranged_hit_bonus;
        if (p_battle_units[attacker].ranged_type / 10 != RANGED_CLASS_MAGIC) {
            range_pen = Combat_Distance(attacker, target) / 3;
            if ((p_battle_units[attacker].ability_flags & AFLAG_LONG_RANGE) && range_pen > 0)
                range_pen = 1;
            to_hit -= range_pen;
        }
    } else {
        damage_flags = Attack_Damage_Flags(attacker, ranged);
        attack_flags = attack_flags | p_battle_units[attacker].melee_powers;
        powers = p_battle_units[attacker].melee_powers;
        to_hit += p_battle_units[attacker].melee_hit_bonus - p_battle_units[target].block_bonus;
        points = p_battle_units[attacker].melee;
        realm = Attack_Realm(0, attacker);
        ranged_type = 0;
    }
    shields = Defender_Shields(target, ranged_type, damage_flags, attack_flags, realm);
    score = points - shields;
    if (Has_Shooting_Attack(target))
        score += p_battle_units[target].ranged / 3 + 2;
    if ((p_battle_units[target].combat_flags & COMBAT_FLAG_CONFUSION) > 0)
        score -= 10;
    if (p_battlefield->backdrop >= BACKDROP_SORCERY_NODE && score > 0 && attacker < 40 &&
        p_battle_units[attacker].owner == combat_defender_player && (p_battle_units[target].ability_flags & AFLAG_MELD_WITH_NODE))
        score += 3;
    else if (score > 0 && attacker < 40 && p_battle_units[attacker].defense / 2 > target_attack)
        score -= 10;
    if (p_battle_units[attacker].target_unit == target)
        score += 10;
    if ((combat_siege_flags & SIEGE_CITY_WALLS) && ranged == 0 && p_battle_units[attacker].owner == combat_attacker_player &&
        (p_battle_units[attacker].move_flags & MOVE_FLYING) == 0 && (p_battle_units[attacker].ability_flags & AFLAG_NON_CORPOREAL) == 0 &&
        (p_battle_units[attacker].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[attacker].move_flags & MOVE_MERGING) == 0) {
        if (Is_City_Wall_Square(p_battle_units[target].combat_x, p_battle_units[target].combat_y) > 0) {
            if (p_battle_units[target].combat_x != BF_GATE_X || p_battle_units[target].combat_y != BF_GATE_Y)
                score = -20;
            else
                score += 5;
        }
        if ((p_battle_units[target].combat_x == 6 || p_battle_units[target].combat_x == 7) &&   /* the four centre squares */
            (p_battle_units[target].combat_y == 11 || p_battle_units[target].combat_y == 12))
            score = -20;
    }
    if ((combat_siege_flags & SIEGE_WALL_OF_FIRE) && ranged == 0 && p_battle_units[attacker].owner == combat_attacker_player &&
        (p_battle_units[attacker].move_flags & MOVE_FLYING) == 0 && (p_battle_units[attacker].move_flags & MOVE_TELEPORTING) == 0 &&
        (p_battle_units[attacker].move_flags & MOVE_MERGING) == 0 && In_City_Area(target) == 1 &&
        In_City_Area(attacker) == 0) {
        Spell_Damage_Roll(SPELL_FIREBALL, attacker, wall_dmg, 0);
        if (wall_dmg[0] > 1)
            score = -20;
    }
    if (ranged == 1 && (p_battle_units[target].combat_flags & COMBAT_FLAG_BLACK_SLEEP))
        score += 20;
    if (ranged == 0 && Combat_Distance(attacker, target) * 2 <= p_battle_units[attacker].moves) {
        if (p_battle_units[target].combat_flags & COMBAT_FLAG_BLACK_SLEEP)
            score += 20;
        score += 25;
    }
    return score;
}

/* (battle unit) -> 1 when its ranged type is a shot of any class below 100: boulder, missile or magic (1-99) */
int Has_Shooting_Attack(int bu)
{
    int result;

    result = 0;
    if (p_battle_units[bu].ranged_type > 0 && p_battle_units[bu].ranged_type < RANGED_THROWN)
        result = 1;
    return result;
}

/* (attacker, defender) -> the defending figures that fail a death-realm resistance roll when the attacker has
   Cause Fear (ABILITY2_CAUSE_FEAR_SPELL) or Cloak of Fear (UNIT_ENCH_CLOAK_OF_FEAR); none against death immunity
   Numbered: BUG #191 (BUGS_AND_QUIRKS.txt) */
int Cloak_Of_Fear_Count(int attacker, int defender)
{
    int count;
    int k;

    count = 0;
    if (((p_battle_units[attacker].abilities2 & ABILITY2_CAUSE_FEAR_SPELL) || (p_battle_units[attacker].combat_enchantments & UNIT_ENCH_CLOAK_OF_FEAR) ||
         (p_battle_units[attacker].item_enchantments & UNIT_ENCH_CLOAK_OF_FEAR) ||
         (p_units[p_battle_units[attacker].unit_index].enchantments & UNIT_ENCH_CLOAK_OF_FEAR)) &&
        (p_battle_units[defender].abilities & ABILITY_DEATH_IMMUNITY) == 0) {
        for (k = 0; k < p_battle_units[defender].figures_alive; k++) {
            if (Roll_Resist(p_battle_units[defender], 0, REALM_DEATH) > 0)
                count++;
        }
    }
    return count;
}
