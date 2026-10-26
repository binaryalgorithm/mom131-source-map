/* COMBAI.C - WIZARDS.EXE overlay code segment c165 (stub 25e6): the combat AI's spell choice - the best-scoring
   castable spell, each combat spell's score (by the battle odds, the enemy's threat realms and attack profile, and
   the units on the field), a unit's combat value, the per-side threat assessment and attack profile, and the list
   of spells a caster can afford.
   This module has no data of its own (no literals, no BSS).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS c165 src/wizards/COMBAI.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Combat_Choose_Spell(int caster);
int AI_Combat_Spell_Score(int player, int spell, int odds);
int Unit_Combat_Value(int bu);
void AI_Combat_Assess_Threats(void);
void AI_Side_Attack_Profile(int player, int *out);
void Combat_Castable_Spell_List(int caster, int *out, int range_mult);

#include "GAME.H"

extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int ai_defender_spell_threat;              /* DS:9294 */
extern int ai_attacker_spell_threat;              /* DS:9296 */
extern int ai_defender_attack_profile[5];       /* DS:9298 */
extern int ai_attacker_attack_profile[5];       /* DS:92a2 */
extern int ai_defender_threat_realms;           /* DS:92ac  realm bits of the defender's castable hostile spells */
extern int ai_attacker_threat_realms;           /* DS:92ae */
extern int combat_turn_count;                   /* DS:c51c */
extern int combat_battlefield_kind;                /* DS:c520 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int list_count;                          /* DS:c926 */

int AI_Combat_Spell_Target(int spell, int player, int *x, int *y);
int Battle_Odds(int player);
void Combat_Build_Click_Map(void);
int Combat_Range_Multiplier(int player);
int Has_Shooting_Attack(int bu);
int Resist_Bonus(struct BattleUnit bu, int realm);
int Spell_Casting_Cost(int player, int spell, int in_combat);

/* (caster: battle unit < 20 or 20 + player) -> the castable spell with the highest score above 0, or 0 */
int AI_Combat_Choose_Spell(int caster)
{
    int s;                                      /* the range multiplier, then each spell's score */
    int best_score;
    int best_spell;
    int i;
    int player;
    int odds;
    int spells[92];                             /* Combat_Castable_Spell_List's capacity */

    player = caster > CASTER_WIZARD - 1 ? caster - CASTER_WIZARD : p_battle_units[caster].owner;
    best_score = 0;
    best_spell = 0;
    if (caster > CASTER_WIZARD - 1)
        s = Combat_Range_Multiplier(caster - CASTER_WIZARD);
    else
        s = 1;
    Combat_Castable_Spell_List(caster, spells, s);
    odds = Battle_Odds(player);
    for (i = 0; i < list_count; i++) {
        s = AI_Combat_Spell_Score(player, spells[i], odds);
        if (s > best_score) {
            best_score = s;
            best_spell = spells[i];
        }
    }
    return best_spell;
}

/* (player, spell, odds 0..5 from Battle_Odds) -> the spell's score: -100 with no enemy left, -1 without a target;
   else the case's value (from the odds tables - damage, defence, buff, curse, summon - and the field) plus half the
   spell's cost reduction plus Random(20). The threat bits are 1 << REALM_* (1 Nature, 2 Sorcery, 4 Chaos, 8 Life,
   16 Death), but the tests use realm NUMBERS as masks - BUG #20: Resist Elements / Elemental Armor test & 0 (REALM_NATURE,
   always 0) and & 2 (REALM_CHAOS as a number, the Sorcery bit), True Sight & 1 (REALM_SORCERY, the Nature bit),
   Bless / Righteousness & 8 / & 2 (the Life / Sorcery bits, for spells that guard against Death and Chaos)
   Numbered: BUG #223 (BUGS_AND_QUIRKS.txt) */
int AI_Combat_Spell_Score(int player, int spell, int odds)
{
    int score;
    int n;
    int *profile;
    int realms;
    int threat;
    int enemy_count;
    int target;
    int enemy_curse_field;   /* 500 while the enemy side's Wrack / Mana Leak / Call Lightning is in force; never read */
    int dmg_odds[6];
    int def_odds[6];
    int buff_odds[6];
    int curse_odds[6];
    int summon_odds[6];
    int i;
    int count;

    Combat_Build_Click_Map();
    enemy_count = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            enemy_count++;
    }
    if (enemy_count == 0)
        return -100;
    dmg_odds[0] = 500;
    def_odds[0] = -900;
    buff_odds[0] = -900;
    curse_odds[0] = -900;
    summon_odds[0] = 0;
    dmg_odds[1] = 0;
    def_odds[1] = 30;
    buff_odds[1] = 10;
    curse_odds[1] = 30;
    summon_odds[1] = 10;
    dmg_odds[2] = 10;
    def_odds[2] = 20;
    buff_odds[2] = 20;
    curse_odds[2] = 35;
    summon_odds[2] = 20;
    dmg_odds[3] = 20;
    def_odds[3] = 15;
    buff_odds[3] = 20;
    curse_odds[3] = 15;
    summon_odds[3] = 25;
    dmg_odds[4] = 30;
    def_odds[4] = 10;
    buff_odds[4] = 20;
    curse_odds[4] = 0;
    summon_odds[4] = 10;
    dmg_odds[5] = 40;
    def_odds[5] = 0;
    buff_odds[5] = 10;
    curse_odds[5] = 0;
    summon_odds[5] = -10;
    if (player == combat_attacker_player) {
        realms = ai_defender_threat_realms;
        profile = ai_defender_attack_profile;
        threat = ai_defender_spell_threat;
        if (p_combat_enchants[CENCH_WRACK_DEF] > 0 || p_combat_enchants[CENCH_MANA_LEAK_DEF] > 0 || p_combat_enchants[CENCH_CALL_LIGHTNING_DEF] > 0)
            enemy_curse_field = 500;
        else
            enemy_curse_field = 0;
    } else {
        realms = ai_attacker_threat_realms;
        profile = ai_attacker_attack_profile;
        threat = ai_attacker_spell_threat;
        if (p_combat_enchants[CENCH_WRACK_ATT] > 0 || p_combat_enchants[CENCH_MANA_LEAK_ATT] > 0 || p_combat_enchants[CENCH_CALL_LIGHTNING_ATT] > 0)
            enemy_curse_field = 500;
        else
            enemy_curse_field = 0;
    }
    target = AI_Combat_Spell_Target(spell, player, &score, &score);   /* the x / y outputs are not wanted */
    if (target < 0)
        return -1;
    score = 0;
    switch (spell) {
    case SPELL_RESIST_ELEMENTS:
        score += (profile[REALM_NATURE] + profile[REALM_CHAOS]) / 4;
        if ((realms & 0) || (realms & 2))   /* BUG #20: realm numbers as masks (see the header): 0 tests nothing, 2 Sorcery */
            score += threat;
        if (score != 0)
            score += def_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_STONE_SKIN:
        score += def_odds[odds];
        score += 15;
        break;
    case SPELL_ICE_BOLT:
        score += dmg_odds[odds];
        break;
    case SPELL_PETRIFY:
        score += dmg_odds[odds];
        break;
    case SPELL_WALL_OF_STONE:
        count = 0;
        if (player == combat_defender_player) {
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                    (p_battle_units[i].move_flags & MOVE_FLYING) == 0 && (p_battle_units[i].move_flags & MOVE_TELEPORTING) == 0 &&
                    (p_battle_units[i].move_flags & MOVE_MERGING) == 0)
                    count++;
            }
            score += def_odds[odds] * count;
        } else {
            score = -100;
        }
        break;
    case SPELL_WEB:
        score += curse_odds[odds];
        count = 0;
        for (i = 0; i < battle_unit_count && count == 0; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].move_flags & MOVE_FLYING) != 0)
                count = 1;
        }
        score += count * 10;
        break;
    case SPELL_EARTH_TO_MUD:
        count = 0;
        n = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && Has_Shooting_Attack(i)) {
                if (p_battle_units[i].owner == player)
                    count += p_battle_units[i].ranged * p_battle_units[i].figures_alive;
                else
                    n += p_battle_units[i].ranged * p_battle_units[i].figures_alive;
            }
        }
        if (count * 2 > n * 3)                    /* cnt / n: the own / enemy missile strength (ranged x figures) */
            score += 15;
        else
            score -= 100;
        break;
    case SPELL_CALL_LIGHTNING:
        if (odds > 1)
            score += enemy_count * 4;
        score += dmg_odds[odds];
        break;
    case SPELL_CRACKS_CALL:
        if (combat_battlefield_kind != BATTLEFIELD_WATER) {
            score += dmg_odds[odds];
            if (player == combat_attacker_player && p_battlefield->has_city_walls > 0)
                score += 20;
        } else {
            score = -100;
        }
        break;
    case SPELL_ELEMENTAL_ARMOR:
        score += (profile[REALM_NATURE] + profile[REALM_CHAOS]) / 2;
        if ((realms & 0) || (realms & 2))   /* BUG #20: realm numbers as masks (see the header): 0 tests nothing, 2 Sorcery */
            score += threat * 2;
        if (score > 0)
            score += def_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_IRON_SKIN:
        score += def_odds[odds];
        score += 35;
        break;
    case SPELL_EARTH_ELEMENTAL:
        if (combat_battlefield_kind == BATTLEFIELD_WATER)
            score -= 100;
        else
            score += summon_odds[odds];
        break;
    case SPELL_REGENERATION:
        score += def_odds[odds];
        break;
    case SPELL_RESIST_MAGIC:
        count = 0;
        for (i = 0; i < 5; i++)
            count += profile[i];
        score += count / 4;
        if (realms > 0)
            score += threat;
        if (score > 0)
            score += def_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_DISPEL_MAGIC_TRUE:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                (p_battle_units[i].combat_flags & ~COMBAT_FLAGS_NON_CURSE) > 0)
                count++;
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].combat_enchantments > 0 || (p_battle_units[i].combat_flags & COMBAT_FLAGS_NON_CURSE) > 0 ||
                 p_units[p_battle_units[i].unit_index].enchantments > 0))
                count++;
        }
        if (count > 0)
            score += 35;
        else
            score -= 100;
        break;
    case SPELL_PSIONIC_BLAST:
        score += dmg_odds[odds];
        break;
    case SPELL_WORD_OF_RECALL:
        if (wizards[player].casting_spell == SPELL_SPELL_OF_RETURN) {
            score = -100;
        } else if (player == PLAYER_HUMAN) {
            score = -100;
        } else {
            count = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                    p_battle_units[i].cost >= 200 &&
                    (odds < 2 || p_battle_units[i].front_damage + 3 >= p_battle_units[i].hits_per_figure))
                    count++;
            }
            if (count == 0)
                score -= 100;
            else
                score += 100;
        }
        break;
    case SPELL_PHANTOM_WARRIORS:
        if (combat_battlefield_kind == BATTLEFIELD_WATER)
            score -= 100;
        else
            score += summon_odds[odds];
        if (p_battlefield->city_enchants[CITY_ENCH_WALL_OF_FIRE] > 0 && player == combat_attacker_player)
            score = -100;
        break;
    case SPELL_COUNTER_MAGIC:
        if (realms > 0) {
            score += threat * 4 + 30;
            score += def_odds[odds];
        } else {
            score = -100;
        }
        break;
    case SPELL_VERTIGO:
        score += curse_odds[odds];
        break;
    case SPELL_GUARDIAN_WIND:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].ranged_type / 10 == RANGED_CLASS_MISSILE && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                p_battle_units[i].owner != player)
                count += p_battle_units[i].ranged;
        }
        if (count > 0) {
            score += def_odds[odds];
            score += count;
        }
        break;
    case SPELL_FLIGHT:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if ((p_battle_units[i].move_flags & MOVE_FLYING) != 0 && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                p_battle_units[i].owner != player)
                count++;
        }
        score += def_odds[odds];
        if (count == 0)
            score += 20;
        score += count * 2;
        break;
    case SPELL_BANISH:
        score += dmg_odds[odds];
        break;
    case SPELL_PHANTOM_BEAST:
        score += summon_odds[odds];
        break;
    case SPELL_CONFUSION:
        score += curse_odds[odds];
        break;
    case SPELL_AIR_ELEMENTAL:
        score += summon_odds[odds];
        break;
    case SPELL_MASS_INVISIBILITY:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0) {
                count++;
                if (p_battle_units[i].ranged_type < RANGED_THROWN)
                    count += p_battle_units[i].ranged;
            }
        }
        if (count > 0) {
            score += def_odds[odds];
            score += count;
        } else {
            score -= 100;
        }
        break;
    case SPELL_HASTE:
        score += buff_odds[odds];
        break;
    case SPELL_MIND_STORM:
        score += curse_odds[odds];
        break;
    case SPELL_MAGIC_IMMUNITY:
        count = 0;
        for (i = 0; i < 5; i++)
            count += profile[i];
        score += count / 2;
        if (realms > 0)
            score += threat * 3;
        if (score > 0)
            score += def_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_CREATURE_BINDING:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (char)p_battle_units[i].race >= RACE_ARCANE_CREATURE)
                count++;
        }
        if (count == 1) {
            score += dmg_odds[odds];
        } else {
            score += summon_odds[odds];
            score += dmg_odds[odds];
            score += 20;
        }
        break;
    case SPELL_INVISIBILITY:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].abilities & ABILITY_ILLUSIONS_IMMUNITY) == 0) {
                count++;
                if (p_battle_units[i].ranged_type < RANGED_THROWN)
                    count += p_battle_units[i].ranged;
            }
        }
        if (count > 0) {
            score += def_odds[odds] + 5;
            score += count;
        } else {
            score = -100;
        }
        break;
    case SPELL_FIRE_BOLT:
        score += dmg_odds[odds];
        break;
    case SPELL_FLAME_BLADE:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                ((p_battle_units[i].combat_enchantments & UNIT_ENCH_INVULNERABILITY) != 0 ||
                 (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_INVULNERABILITY) != 0 ||
                 (p_battle_units[i].item_enchantments & UNIT_ENCH_INVULNERABILITY) != 0))
                count++;
        }
        if (count > 0)
            score += 25;
        score += buff_odds[odds];
        break;
    case SPELL_LIGHTNING_BOLT:
        score += dmg_odds[odds];
        break;
    case SPELL_DISRUPT:
        if (player == combat_attacker_player && p_battlefield->has_city_walls > 0)
            score += 20;
        else
            score -= 100;
        break;
    case SPELL_SHATTER:
        score += def_odds[odds];
        break;
    case SPELL_FIREBALL:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                p_battle_units[i].figures_alive > count)
                count = p_battle_units[i].figures_alive;
        }
        score += dmg_odds[odds];
        if (count > 3)
            score += (count - 3) * 5;
        break;
    case SPELL_WALL_OF_FIRE:   /* dead: AI_Combat_Spell_Target returns -1 for the walls (SPELL_FX_CITY_ENCH), so the
                                  scorer has already returned -1; clarified using ReMoM repo */
        count = 0;
        if (player == combat_defender_player) {
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                    (p_battle_units[i].move_flags & MOVE_FLYING) == 0 && (p_battle_units[i].move_flags & MOVE_TELEPORTING) == 0 &&
                    (p_battle_units[i].move_flags & MOVE_MERGING) == 0)
                    count++;
            }
            score += buff_odds[odds] * (count + 1) / 2;
        } else {
            score = -100;
        }
        break;
    case SPELL_FIRE_ELEMENTAL:
        if (combat_battlefield_kind == BATTLEFIELD_WATER)
            score -= 100;
        else
            score += summon_odds[odds];
        break;
    case SPELL_WARP_CREATURE:
        score += curse_odds[odds];
        break;
    case SPELL_ELDRITCH_WEAPON:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                ((p_battle_units[i].combat_enchantments & UNIT_ENCH_INVULNERABILITY) != 0 ||
                 (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_INVULNERABILITY) != 0 ||
                 (p_battle_units[i].item_enchantments & UNIT_ENCH_INVULNERABILITY) != 0))
                count++;
        }
        if (count > 0)
            score += 25;
        score += buff_odds[odds];
        break;
    case SPELL_WARP_WOOD:
        score += curse_odds[odds];
        break;
    case SPELL_IMMOLATION:
        score += buff_odds[odds];
        score += 10;
        break;
    case SPELL_METAL_FIRES:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                (char)p_battle_units[i].race < RACE_ARCANE_CREATURE)
                count++;
        }
        if (count > 0) {
            score += buff_odds[odds];
            score += count * 5;
        } else {
            score -= 100;
        }
        break;
    case SPELL_MAGIC_VORTEX:
        score += dmg_odds[odds];
        break;
    case SPELL_WARP_LIGHTNING:
        score += dmg_odds[odds];
        break;
    case SPELL_DISINTEGRATE:
        score += dmg_odds[odds];
        score += 25;
        break;
    case SPELL_DOOM_BOLT:
        score += dmg_odds[odds];
        break;
    case SPELL_FLAME_STRIKE:
        score += dmg_odds[odds];
        score += enemy_count * 2;
        break;
    case SPELL_WARP_REALITY:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].race == RACE_CHAOS_CREATURE) {
                if (p_battle_units[i].owner == player)
                    count++;
                else
                    count--;
            }
        }
        if (count > 0) {
            score += dmg_odds[odds];
            score += count * 5;
        } else {
            score -= 100;
        }
        break;
    case SPELL_HOLY_WEAPON:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                ((p_battle_units[i].combat_enchantments & UNIT_ENCH_INVULNERABILITY) != 0 ||
                 (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_INVULNERABILITY) != 0 ||
                 (p_battle_units[i].item_enchantments & UNIT_ENCH_INVULNERABILITY) != 0))
                count++;
        }
        if (count > 0)
            score += 25;
        score += buff_odds[odds];
        break;
    case SPELL_BLESS:
        score += (profile[REALM_DEATH] + profile[REALM_CHAOS]) / 4;
        if ((realms & 8) || (realms & 2))       /* BUG #20: the Life / Sorcery bits (see the header) */
            score += threat;
        if (score != 0)
            score += def_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_STAR_FIRES:
        score += dmg_odds[odds];
        break;
    case SPELL_HEALING:
        score += Unit_Combat_Value(target) / 4;
        break;
    case SPELL_HOLY_ARMOR:
        score += def_odds[odds];
        break;
    case SPELL_TRUE_LIGHT:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                ((p_battle_units[i].owner == player && p_battle_units[i].race == RACE_LIFE_CREATURE) ||
                 (p_battle_units[i].owner != player && p_battle_units[i].race == RACE_DEATH_CREATURE)))
                count++;
        }
        if (count == 0)
            score = -100;
        else
            score += count * 3;
        break;
    case SPELL_HEROISM:
        score += buff_odds[odds];
        break;
    case SPELL_TRUE_SIGHT:
        score += profile[REALM_SORCERY] / 2;               /* always 0: AI_Side_Attack_Profile never fills out[1] */
        if (realms & 1)                         /* BUG #20: the Nature bit (see the header) */
            score += threat * 2;
        if (score != 0)
            score += def_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_PRAYER:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player)
                count++;
        }
        score += buff_odds[odds];
        score += count * 2;
        break;
    case SPELL_INVULNERABILITY:
        score += def_odds[odds];
        score += 30;
        break;
    case SPELL_MASS_HEALING:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                (p_battle_units[i].figures_alive > p_battle_units[i].figures || p_battle_units[i].front_damage > 0))
                /* BUG #435: figures_alive > figures is never true (figures is the full count): only front damage counts */
                count++;
        }
        if (count > 0)
            score += (count - 1) * 10 + 5;
        else
            score = -1000;
        break;
    case SPELL_RAISE_DEAD:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status > 0 && p_battle_units[i].owner == player &&
                (char)p_battle_units[i].race < RACE_ARCANE_CREATURE && p_battle_units[i].status != BATTLE_UNIT_DESTROYED &&
                p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY)
                count++;
        }
        if (count > 0)
            score += summon_odds[odds];
        else
            score -= 100;
        break;
    case SPELL_DISPEL_EVIL:
        score += dmg_odds[odds];
        break;
    case SPELL_HOLY_WORD:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                Resist_Bonus(p_battle_units[i], REALM_LIFE) - 2 < 10 && (char)p_battle_units[i].race >= RACE_ARCANE_CREATURE)
                count += p_battle_units[i].figures_alive;
        }
        if (count > 0) {
            score += dmg_odds[odds];
            score += count * 3;
        } else {
            score -= 100;
        }
        break;
    case SPELL_LIONHEART:
        break;
    case SPELL_HIGH_PRAYER:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player)
                count++;
        }
        score += buff_odds[odds];
        score += count * 4;
        break;
    case SPELL_RIGHTEOUSNESS:
        score += profile[REALM_DEATH] + profile[REALM_CHAOS];
        if ((realms & 8) || (realms & 2))       /* BUG #20: the Life / Sorcery bits (see the header) */
            score += threat * 2;
        if (score > 0)
            score += def_odds[odds];
        break;
    case SPELL_WEAKNESS:
        score += curse_odds[odds];
        break;
    case SPELL_CLOAK_OF_FEAR:
        score += def_odds[odds];
        break;
    case SPELL_BLACK_SLEEP:
        score += curse_odds[odds];
        score += dmg_odds[odds];
        break;
    case SPELL_DARKNESS:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                ((p_battle_units[i].owner == player && p_battle_units[i].race == RACE_DEATH_CREATURE) ||
                 (p_battle_units[i].owner != player && p_battle_units[i].race == RACE_LIFE_CREATURE)))
                count++;
        }
        if (count == 0)
            score = -100;
        else
            score += count * 3;
        break;
    case SPELL_BLACK_PRAYER:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player)
                count++;
        }
        score += def_odds[odds];
        score += count * 4;
        break;
    case SPELL_BERSERK:
        score += buff_odds[odds];
        score += 10;
        break;
    case SPELL_LIFE_DRAIN:
        score += dmg_odds[odds];
        score += 10;
        break;
    case SPELL_MANA_LEAK:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].spell_points > 0 || p_battle_units[i].ranged_type / 10 == RANGED_CLASS_MAGIC))
                count += p_battle_units[i].figures_alive;
        }
        if (count > 0)
            score += 10 - combat_turn_count * 2;
        else
            score = -100;
        break;
    case SPELL_POSSESSION:
        score += dmg_odds[odds];
        score += summon_odds[odds];
        break;
    case SPELL_TERROR:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                Resist_Bonus(p_battle_units[i], REALM_DEATH) + 1 < 10 && (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY) == 0)
                count++;
        }
        if (count > 0) {
            score += curse_odds[odds];
            score += count * 3;
        } else {
            score -= 100;
        }
        break;
    case SPELL_WALL_OF_DARKNESS:
        count = 0;
        if (player == combat_defender_player) {
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player && Has_Shooting_Attack(i))
                    count += p_battle_units[i].ranged;
            }
            score += def_odds[odds] + count * 2;
        } else {
            score = -100;
        }
        break;
    case SPELL_WRAITH_FORM:
        score += def_odds[odds];
        break;
    case SPELL_WRACK:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                Resist_Bonus(p_battle_units[i], REALM_DEATH) + 1 < 10 && (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY) == 0)
                count += p_battle_units[i].figures_alive;
        }
        if (count > 0) {
            score += dmg_odds[odds];
            score += count * 5;
        } else {
            score -= 100;
        }
        break;
    case SPELL_WORD_OF_DEATH:
        score += dmg_odds[odds];
        break;
    case SPELL_DEATH_SPELL:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                Resist_Bonus(p_battle_units[i], REALM_DEATH) - 2 < 10 && (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY) == 0)
                count += p_battle_units[i].figures_alive;
        }
        if (count > 0) {
            score += dmg_odds[odds];
            score += count * 3;
        } else {
            score -= 100;
        }
        break;
    case SPELL_ANIMATE_DEAD:            /* cnt is counted and never used */
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status > 0 && p_battle_units[i].status != BATTLE_UNIT_DESTROYED && p_battle_units[i].race != RACE_DEATH_CREATURE &&
                p_units[p_battle_units[i].unit_index].hero_slot == -1 && p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY)
                count++;
        }
        break;
    case SPELL_DISPEL_MAGIC:
        count = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                (p_battle_units[i].combat_flags & ~COMBAT_FLAGS_NON_CURSE) > 0)
                count++;
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].combat_enchantments > 0 || (p_battle_units[i].combat_flags & COMBAT_FLAGS_NON_CURSE) > 0 ||
                 p_units[p_battle_units[i].unit_index].enchantments > 0))
                count++;
        }
        if (count > 0)
            score += 25;
        else
            score -= 100;
        break;
    case SPELL_DISENCHANT_TRUE:
    case SPELL_DISENCHANT_AREA:
        count = 0;
        score = 0;
        n = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if ((p_battle_units[i].combat_enchantments & UNIT_ENCH_INVISIBILITY) == 0 &&
                (p_battle_units[i].item_enchantments & UNIT_ENCH_INVISIBILITY) == 0 && (p_battle_units[i].ability_flags & AFLAG_INVISIBILITY) == 0 &&
                (p_units[p_battle_units[i].unit_index].enchantments & UNIT_ENCH_INVISIBILITY) == 0 && p_battle_units[i].owner != PLAYER_HUMAN)
                n++;
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                (p_battle_units[i].combat_flags & ~COMBAT_FLAGS_NON_CURSE) > 0)
                count++;
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner != player &&
                (p_battle_units[i].combat_enchantments > 0 || (p_battle_units[i].combat_flags & COMBAT_FLAGS_NON_CURSE) > 0 ||
                 p_units[p_battle_units[i].unit_index].enchantments > 0) &&
                (wizards[p_battle_units[i].owner].globals.holy_arms == 0 ||
                 p_battle_units[i].combat_enchantments != UNIT_ENCH_HOLY_WEAPON || (p_battle_units[i].combat_flags & COMBAT_FLAGS_NON_CURSE) > 0 ||
                 p_units[p_battle_units[i].unit_index].enchantments > 0))
                count++;
        }
        score += count * 6;
        if (n == 0)
            score += 150;
        if (player == combat_attacker_player)
            n = 1;
        else
            n = 0;
        count = 0;
        for (i = n; i < NUM_COMBAT_ENCHANTS; i += 2) {   /* 15 pairs; n = the ENEMY's side (1: the defender's, when the
                                                   caster attacks); == 1 misses Counter Magic, whose slot holds its mana */
            if (p_combat_enchants[i] == 1)
                count++;
        }
        score += count * 30;
        if (score == 0)
            score = -100;
        break;
    case SPELL_RECALL_HERO:
        if (wizards[player].casting_spell == SPELL_SPELL_OF_RETURN)
            score = -100;
        if (player == PLAYER_HUMAN) {
            score = -100;
        } else {
            count = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player &&
                    p_units[p_battle_units[i].unit_index].hero_slot > -1 &&
                    (odds < 2 || p_battle_units[i].front_damage + 3 >= p_battle_units[i].hits_per_figure))
                    count++;
            }
            if (count == 0)
                score = -100;
            else
                score += 100;
        }
        break;
    }
    score += Spell_Cost_Reduction_Pct(player, spell) / 2;
    score += Random(20);
    return score;
}

/* (bu) -> the AI's strength value of a battle unit: attack value + toughness (+ missile value)
   Numbered: BUG #318 (BUGS_AND_QUIRKS.txt) */
int Unit_Combat_Value(int bu)
{
    int ranged;
    int value;

    /* ranged is set inside the fifth argument: BCC pushes the arguments right to left, so it is ready when the
       second is read */
    value = AI_Attack_Value(p_battle_units[bu].melee, ranged, p_battle_units[bu].figures_alive,
                            p_battle_units[bu].attack_flags | p_battle_units[bu].melee_powers,
                            p_battle_units[bu].ranged_type >= RANGED_THROWN ? (ranged = p_battle_units[bu].ranged,
                                                                     p_battle_units[bu].ranged_type)
                                                                  : (ranged = 0, -1));
    value += AI_Toughness_Value(p_battle_units[bu].hits_per_figure * p_battle_units[bu].figures_alive -
                                    p_battle_units[bu].front_damage,
                                p_battle_units[bu].defense);
    if (Has_Shooting_Attack(bu))
        value += AI_Missile_Value(p_battle_units[bu].ranged, p_battle_units[bu].figures_alive,
                                  p_battle_units[bu].attack_flags | p_battle_units[bu].ranged_powers);
    return value;
}

/* () each side's threat realms - 1 << realm of each castable damage, destroy and curse spell and Creature Binding
   (Chaos Channels too, for the defender only), OR'ed into ai_*_threat_realms, which nothing clears (BUG #331) - and
   each side's attack profile. ai_*_spell_threat are not set here (nor anywhere)
   Numbered: BUG #20, #312 (BUGS_AND_QUIRKS.txt) */
void AI_Combat_Assess_Threats(void)
{
    int range_mult;
    int spells[92];
    int i;
    int effect;

    range_mult = Combat_Range_Multiplier(combat_attacker_player);
    Combat_Castable_Spell_List(combat_attacker_player + CASTER_WIZARD, spells, range_mult);
    for (i = 0; i < list_count; i++) {
        effect = p_spell_data[spells[i]].effect_type;
        if (effect == SPELL_FX_DAMAGE || effect == SPELL_FX_DAMAGE_VAR || effect == SPELL_FX_DESTROY || effect == SPELL_FX_UNIT_CURSE || effect == SPELL_FX_NORMAL_UNIT_CURSE || spells[i] == SPELL_CREATURE_BINDING)
            ai_attacker_threat_realms = ai_attacker_threat_realms | (1 << p_spell_data[spells[i]].realm);
    }
    range_mult = Combat_Range_Multiplier(combat_defender_player);
    Combat_Castable_Spell_List(combat_defender_player + CASTER_WIZARD, spells, range_mult);
    for (i = 0; i < list_count; i++) {
        effect = p_spell_data[spells[i]].effect_type;
        if (effect == SPELL_FX_DAMAGE || effect == SPELL_FX_DESTROY || effect == SPELL_FX_DAMAGE_VAR || effect == SPELL_FX_UNIT_CURSE || effect == SPELL_FX_NORMAL_UNIT_CURSE || spells[i] == SPELL_CREATURE_BINDING ||
            spells[i] == SPELL_CHAOS_CHANNELS)
            ai_defender_threat_realms = ai_defender_threat_realms | (1 << p_spell_data[spells[i]].realm);
    }
    AI_Side_Attack_Profile(combat_attacker_player, ai_attacker_attack_profile);
    AI_Side_Attack_Profile(combat_defender_player, ai_defender_attack_profile);
}

/* (player, out[5]) the side's attack strength by the realm of the attack kind, as percentages of its total:
   [REALM_NATURE] stoning gaze / touch, [REALM_CHAOS] fire / lightning breath, Fireball / Doom Bolt, Immolation,
   destruction, [REALM_LIFE] dispel evil, [REALM_DEATH] life steal (special strength), fear, death touch;
   [REALM_SORCERY] is never filled */
void AI_Side_Attack_Profile(int player, int *out)
{
    int sum;
    long ench;
    int i;

    sum = 0;
    for (i = 0; i < 5; i++)
        out[i] = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            ench = p_battle_units[i].combat_enchantments | p_battle_units[i].item_enchantments |
                   p_units[p_battle_units[i].unit_index].enchantments;
            sum += p_battle_units[i].melee * p_battle_units[i].figures_alive;
            sum += p_battle_units[i].ranged * p_battle_units[i].figures_alive;
            if (p_battle_units[i].ranged_type == RANGED_FIRE_BREATH || p_battle_units[i].ranged_type == RANGED_LIGHTNING)
                out[REALM_CHAOS] += p_battle_units[i].ranged * p_battle_units[i].figures_alive;
            if (p_battle_units[i].ranged_type == RANGED_STONING_GAZE)
                out[REALM_NATURE] += p_battle_units[i].ranged * p_battle_units[i].figures_alive;
            if ((p_battle_units[i].abilities2 & ABILITY2_FIRE_BALL_SPELL) != 0 || (p_battle_units[i].abilities2 & ABILITY2_DOOMBOLT_SPELL) != 0) {
                sum += 10;
                out[REALM_CHAOS] += 10;
            }
            if ((p_battle_units[i].abilities2 & ABILITY2_IMMOLATION) != 0 || (ench & UNIT_ENCH_IMMOLATION) != 0) {
                sum += 20;
                out[REALM_CHAOS] += 20;
            }
            if ((p_battle_units[i].attack_flags & ATTACK_LIFE_STEAL) != 0 || (p_battle_units[i].melee_powers & ATTACK_LIFE_STEAL) != 0 ||
                (p_battle_units[i].ranged_powers & ATTACK_LIFE_STEAL) != 0) {
                sum += (p_battle_units[i].special_strength + 3) * p_battle_units[i].figures_alive;
                out[REALM_DEATH] += (p_battle_units[i].special_strength + 3) * p_battle_units[i].figures_alive;
            }
            if ((p_battle_units[i].attack_flags & ATTACK_DESTRUCTION) != 0 || (p_battle_units[i].melee_powers & ATTACK_DESTRUCTION) != 0 ||
                (p_battle_units[i].ranged_powers & ATTACK_DESTRUCTION) != 0) {
                sum += p_battle_units[i].figures_alive * 5;
                out[REALM_CHAOS] += p_battle_units[i].figures_alive * 5;
            }
            if ((p_battle_units[i].attack_flags & ATTACK_STONING_TOUCH) != 0 || (p_battle_units[i].melee_powers & ATTACK_STONING_TOUCH) != 0 ||
                (p_battle_units[i].ranged_powers & ATTACK_STONING_TOUCH) != 0) {
                sum += p_battle_units[i].figures_alive * 5;
                out[REALM_NATURE] += p_battle_units[i].figures_alive * 5;
            }
            if ((p_battle_units[i].abilities2 & ABILITY2_CAUSE_FEAR_SPELL) != 0 || (ench & UNIT_ENCH_CLOAK_OF_FEAR) != 0) {
                sum += p_battle_units[i].figures_alive;
                out[REALM_DEATH] += p_battle_units[i].figures_alive;
            }
            if ((p_battle_units[i].attack_flags & ATTACK_DEATH_TOUCH) != 0 || (p_battle_units[i].melee_powers & ATTACK_DEATH_TOUCH) != 0 ||
                (p_battle_units[i].ranged_powers & ATTACK_DEATH_TOUCH) != 0) {
                sum += p_battle_units[i].figures_alive * 5;
                out[REALM_DEATH] += p_battle_units[i].figures_alive * 5;
            }
            if ((p_battle_units[i].attack_flags & ATTACK_DISPEL_EVIL) != 0 || (p_battle_units[i].melee_powers & ATTACK_DISPEL_EVIL) != 0 ||
                (p_battle_units[i].ranged_powers & ATTACK_DISPEL_EVIL) != 0) {
                sum += p_battle_units[i].figures_alive * 5;
                out[REALM_LIFE] += p_battle_units[i].figures_alive * 5;
            }
        }
    }
    if (sum != 0) {
        for (i = 0; i < 5; i++)
            out[i] = (long)out[i] * 100 / sum;
    }
}

/* (caster, out[], range_mult) fills out[] (list_count entries) with the spells the caster can afford: a wizard
   from its library (cost by Spell_Casting_Cost, budget min(skill left, mana * 10 / range)); a unit with the
   caster abilities (0x6000) its realm's 40 spells; other units (heroes) the owner's known spells at base cost, then
   (BUG #436: < 19, not < 20) their hero spells
   Numbered: BUG #332 (BUGS_AND_QUIRKS.txt) */
void Combat_Castable_Spell_List(int caster, int *out, int range_mult)
{
    int player;
    int realm;
    int slot;
    int cost;
    int k;
    int budget;
    int spell;

    list_count = 0;
    if (caster > CASTER_WIZARD - 1) {
        player = caster - CASTER_WIZARD;
        budget = wizards[player].combat_skill_left;
        /* BUG #332: at the own fortress (range_mult 5) a reserve over 16383 makes the int negative: no spell fits (see
           Combat_Spellbook_Build); clarified using ReMoM repo */
        if ((int)((long)wizards[player].mana_reserve * 10 / range_mult) < budget)
            budget = (long)wizards[player].mana_reserve * 10 / range_mult;
    } else {
        player = p_battle_units[caster].owner;
        budget = p_battle_units[caster].spell_points;
    }
    if (caster < CASTER_WIZARD && (p_battle_units[caster].abilities & ABILITY_CASTER) != 0) {
        for (slot = 0; slot < SPELLS_PER_REALM; slot++) {
            spell = ((char)p_battle_units[caster].race - RACE_NATURE_CREATURE) * SPELLS_PER_REALM + slot + 1;
            if (p_spell_data[spell].castable != CASTABLE_OVERLAND_ONLY) {
                cost = p_spell_data[spell].base_cost;
                if (cost <= budget) {
                    out[list_count] = spell;
                    list_count++;
                }
            }
        }
    } else {
        for (realm = 0; realm < NUM_REALMS; realm++) {
            for (slot = 0; slot < SPELLS_PER_REALM; slot++) {
                spell = realm * SPELLS_PER_REALM + slot + 1;
                if (wizards[player].spell_library[realm][slot] == SPELLSTATE_KNOWN && p_spell_data[spell].castable != CASTABLE_OVERLAND_ONLY) {
                    if (caster > CASTER_WIZARD - 1)
                        cost = Spell_Casting_Cost(player, spell, 1);
                    else
                        cost = p_spell_data[spell].base_cost;
                    if (cost <= budget) {
                        out[list_count] = spell;
                        list_count++;
                    }
                }
            }
        }
        if (caster < 19) {              /* BUG #436: 19, not CASTER_WIZARD */
            for (k = 0; k < 4; k++) {
                spell = p_hero_stats[player][p_units[p_battle_units[caster].unit_index].type].spells[k];
                if (spell > 0 && p_spell_data[spell].castable != CASTABLE_OVERLAND_ONLY && p_spell_data[spell].base_cost <= budget) {
                    out[list_count] = spell;
                    list_count++;
                }
            }
        }
    }
}
