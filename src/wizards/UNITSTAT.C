/* UNITSTAT.C - WIZARDS.EXE overlay code segment 9748 (stub 2553): a unit's effective statistics - the battle record
   built from its unit type, then the modifiers: experience levels, hero abilities and items, weapon mutations,
   unit enchantments, the global and combat enchantments, node auras and curses; and its hit points per figure.
   This module has no data of its own (no literals, no BSS).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 9748 src/wizards/UNITSTAT.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
long Hero_Item_Bonus(int unit, struct BattleUnit far *bu);
void Hero_Item_Enchants(int item, struct BattleUnit far *bu);
void Hero_Item_Attack_Powers(int far *powers, int item);
int Unit_Max_Hits(int unit);
int Battle_Unit_Max_Hits(struct BattleUnit far *bu);
void Battle_Unit_Init(int unit, struct BattleUnit far *bu);
void Unit_Apply_Modifiers(struct BattleUnit far *bu);
void Unit_Enchant_Bonus(struct BattleUnit far *bu, long ench, char mutations);
void Unit_Experience_Bonus(int unit, struct BattleUnit far *bu);
void Hero_Ability_Bonus(int unit, struct BattleUnit far *bu);
void Battle_Unit_Combat_Modifiers(struct BattleUnit far *bu);
int Unit_Has_Spell_Enchant(int unit, int spell);

#include "GAME.H"

extern char far *p_combat_enchants;             /* DS:922e */
extern int num_players;                         /* DS:bd9c */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int combat_node_type;                    /* DS:c7c2 */
extern int far *p_aura_leadership;              /* DS:c896 */
extern int far *p_aura_resistance;              /* DS:c89a */
extern int far *p_aura_holy_bonus;              /* DS:c89e */

int Mem_Copy_Far();                               /* unprototyped (see HELPSCR.C): far pointers */
int Unit_Gold_Upkeep(int unit);
int Unit_Move_Points(int unit);

/* (unit, bu) adds a hero's three items to its battle record: their enchantments, the shield's Large Shield, spell
   skill (casters only), defense, moves and resistance; weapons (types 0-2) and misc items (ITEM_TYPE_MISC) add their attack and
   to-hit to melee, Flaming +3, Giant Strength +1, and their attack powers; a bow to a bow, a staff or wand to magic
   ranged, an axe to a thrown attack and misc items to any ranged attack add the same to ranged (Giant Strength only
   for a thrown axe). Returns the item enchantments (0 for a normal unit) */
long Hero_Item_Bonus(int unit, struct BattleUnit far *bu)
{
    long ench;
    int player;
    int hero_slot;
    int item;
    int i;

    bu->item_enchantments = 0;
    bu->melee_powers = 0;
    bu->ranged_powers = 0;
    player = p_units[unit].owner;
    hero_slot = p_units[unit].hero_slot;
    if (hero_slot == -1)
        return 0;
    ench = 0;
    for (i = 0; i < 3; i++) {
        if (wizards[player].hired_heroes[hero_slot].item_index[i] == -1)
            continue;
        item = wizards[player].hired_heroes[hero_slot].item_index[i];
        Hero_Item_Enchants(item, bu);
        if (p_items[item].item_type == ITEM_TYPE_SHIELD)
            bu->ability_flags = bu->ability_flags | AFLAG_LARGE_SHIELD;
        if (bu->spell_skill > 0)
            bu->spell_skill += p_items[item].spell_skill;
        bu->defense += p_items[item].defense;
        bu->defense_bonus_shown += p_items[item].defense;
        bu->moves += p_items[item].moves;
        bu->resist += p_items[item].resist;
        bu->resist_bonus_shown += p_items[item].resist;
        if (p_items[item].item_type <= ITEM_TYPE_AXE || p_items[item].item_type == ITEM_TYPE_MISC) {
            bu->melee += p_items[item].attack;
            bu->melee_bonus_shown += p_items[item].attack;
            bu->melee_hit_bonus += p_items[item].to_hit;
            if ((p_items[item].powers & ITEM_POWER_FLAMING) > 0) {
                bu->melee = bu->melee + 3;
                bu->melee_bonus_shown = bu->melee_bonus_shown + 3;
            }
            if ((p_items[item].powers & ITEM_POWER_GIANT_STRENGTH) > 0) {
                bu->melee = bu->melee + 1;
                bu->melee_bonus_shown = bu->melee_bonus_shown + 1;
            }
            Hero_Item_Attack_Powers(&bu->melee_powers, item);
        }
        if ((p_items[item].item_type == ITEM_TYPE_BOW && bu->ranged_type / 10 == RANGED_CLASS_MISSILE) ||
            (p_items[item].item_type == ITEM_TYPE_STAFF && bu->ranged_type / 10 == RANGED_CLASS_MAGIC) ||
            (p_items[item].item_type == ITEM_TYPE_WAND && bu->ranged_type / 10 == RANGED_CLASS_MAGIC) ||
            (p_items[item].item_type == ITEM_TYPE_AXE && bu->ranged_type == RANGED_THROWN) || p_items[item].item_type == ITEM_TYPE_MISC) {
            bu->ranged += p_items[item].attack;
            bu->ranged_bonus_shown += p_items[item].attack;
            bu->ranged_hit_bonus += p_items[item].to_hit;
            if ((p_items[item].powers & ITEM_POWER_FLAMING) > 0) {
                bu->ranged = bu->ranged + 3;
                bu->ranged_bonus_shown = bu->ranged_bonus_shown + 3;
            }
            if ((p_items[item].powers & ITEM_POWER_GIANT_STRENGTH) > 0 && p_items[item].item_type == ITEM_TYPE_AXE && bu->ranged_type == RANGED_THROWN) {
                bu->ranged = bu->ranged + 1;
                bu->ranged_bonus_shown = bu->ranged_bonus_shown + 1;
            }
            Hero_Item_Attack_Powers(&bu->ranged_powers, item);
        }
    }
    ench = ench | bu->item_enchantments;
    return ench;
}

/* (item, bu) the unit enchantments an item grants, added to the battle record's item enchantments; Holy
   Avenger (ITEM_POWER_HOLY_AVENGER) also grants Bless, as its item text says ("as bless & dispel evil")
   Numbered: BUG #218 (BUGS_AND_QUIRKS.txt) */
void Hero_Item_Enchants(int item, struct BattleUnit far *bu)
{
    long ench;

    ench = 0;
    if ((p_items[item].powers & ITEM_POWER_CLOAK_OF_FEAR) > 0)
        ench = ench | UNIT_ENCH_CLOAK_OF_FEAR;
    if ((p_items[item].powers & ITEM_POWER_WRAITHFORM) > 0)
        ench = ench | UNIT_ENCH_WRAITH_FORM;
    if ((p_items[item].powers & ITEM_POWER_REGENERATION) > 0)
        ench = ench | UNIT_ENCH_REGENERATION;
    if ((p_items[item].powers & ITEM_POWER_PATHFINDING) > 0)
        ench = ench | UNIT_ENCH_PATH_FINDING;
    if ((p_items[item].powers & ITEM_POWER_WATER_WALKING) > 0)
        ench = ench | UNIT_ENCH_WATER_WALKING;
    if ((p_items[item].powers & ITEM_POWER_RESIST_ELEMENTS) > 0)
        ench = ench | UNIT_ENCH_RESIST_ELEMENTS;
    if ((p_items[item].powers & ITEM_POWER_ELEMENTAL_ARMOR) > 0)
        ench = ench | UNIT_ENCH_ELEMENTAL_ARMOR;
    if ((p_items[item].powers & ITEM_POWER_ENDURANCE) > 0)
        ench = ench | UNIT_ENCH_ENDURANCE;
    if ((p_items[item].powers & ITEM_POWER_INVISIBILITY) > 0)
        ench = ench | UNIT_ENCH_INVISIBILITY;
    if ((p_items[item].powers & ITEM_POWER_FLIGHT) > 0)
        ench = ench | UNIT_ENCH_FLIGHT;
    if ((p_items[item].powers & ITEM_POWER_RESIST_MAGIC) > 0)
        ench = ench | UNIT_ENCH_RESIST_MAGIC;
    if ((p_items[item].powers & ITEM_POWER_GUARDIAN_WIND) > 0)
        ench = ench | UNIT_ENCH_GUARDIAN_WIND;
    if ((p_items[item].powers & ITEM_POWER_MAGIC_IMMUNITY) > 0)
        ench = ench | UNIT_ENCH_MAGIC_IMMUNITY;
    if ((p_items[item].powers & ITEM_POWER_TRUE_SIGHT) > 0)
        ench = ench | UNIT_ENCH_TRUE_SIGHT;
    if ((p_items[item].powers & ITEM_POWER_BLESS) > 0)
        ench = ench | UNIT_ENCH_BLESS;
    if ((p_items[item].powers & ITEM_POWER_LION_HEART) > 0)
        ench = ench | UNIT_ENCH_LIONHEART;
    if ((p_items[item].powers & ITEM_POWER_PLANAR_TRAVEL) > 0)
        ench = ench | UNIT_ENCH_PLANAR_TRAVEL;
    if ((p_items[item].powers & ITEM_POWER_RIGHTEOUSNESS) > 0)
        ench = ench | UNIT_ENCH_RIGHTEOUSNESS;
    if ((p_items[item].powers & ITEM_POWER_INVULNERABILITY) > 0)
        ench = ench | UNIT_ENCH_INVULNERABILITY;
    if ((p_items[item].powers & ITEM_POWER_HOLY_AVENGER) > 0)
        ench = ench | UNIT_ENCH_BLESS;
    bu->item_enchantments = bu->item_enchantments | ench;
}

/* (&powers, item) the attack powers an item grants, added to a melee / ranged power set */
void Hero_Item_Attack_Powers(int far *powers, int item)
{
    int attack_bits;

    attack_bits = *powers;
    if ((p_items[item].powers & ITEM_POWER_VAMPIRIC) > 0)
        attack_bits = attack_bits | ATTACK_LIFE_STEAL;
    if ((p_items[item].powers & ITEM_POWER_LIGHTNING) > 0)
        attack_bits = attack_bits | ATTACK_ARMOR_PIERCING;
    if ((p_items[item].powers & ITEM_POWER_DESTRUCTION) > 0)
        attack_bits = attack_bits | ATTACK_DESTRUCTION;
    if ((p_items[item].powers & ITEM_POWER_DOOM) > 0)
        attack_bits = attack_bits | ATTACK_AUTOMATIC_DAMAGE;
    if ((p_items[item].powers & ITEM_POWER_DEATH) > 0)
        attack_bits = attack_bits | ATTACK_DEATH_TOUCH;
    if ((p_items[item].powers & ITEM_POWER_POWER_DRAIN) > 0)
        attack_bits = attack_bits | ATTACK_POWER_DRAIN;
    if ((p_items[item].powers & ITEM_POWER_HOLY_AVENGER) > 0)
        attack_bits = attack_bits | ATTACK_DISPEL_EVIL;
    if ((p_items[item].powers & ITEM_POWER_PHANTASMAL) > 0)
        attack_bits = attack_bits | ATTACK_ILLUSION;
    if ((p_items[item].powers & ITEM_POWER_STONING) > 0)
        attack_bits = attack_bits | ATTACK_STONING_TOUCH;
    *powers = attack_bits;
}

/* (unit) a unit's hit points per figure: its type's, Black Channels +1, Lionheart +3 (Heroism counts as level 3 at
   least); a hero +1 at levels above 0, 1 and 3 and its hit-point abilities (HERO_ABILITY_CONSTITUTION: level + 1,
   _SUPER_CONSTITUTION: (level + 1) * 3 / 2); every unit +1 at levels above 2, 4, 5, 6 and 7 (no step above 3);
   then Charm of Life's +25% (at least 1)
   Numbered: BUG #273 (BUGS_AND_QUIRKS.txt) */
int Unit_Max_Hits(int unit)
{
    int bonus;
    int level;
    int hits;

    level = p_units[unit].level;
    if ((p_units[unit].enchantments & UNIT_ENCH_HEROISM) && level < 3)
        level = 3;
    hits = unit_types[p_units[unit].type].hits;
    if (p_units[unit].enchantments & UNIT_ENCH_BLACK_CHANNELS)
        hits++;
    if (p_units[unit].enchantments & UNIT_ENCH_LIONHEART)
        hits += 3;
    if (p_units[unit].hero_slot > -1) {
        if (level > 0)
            hits++;
        if (p_hero_stats[p_units[unit].owner][p_units[unit].type].abilities & HERO_ABILITY_CONSTITUTION)
            hits += level + 1;
        if (p_hero_stats[p_units[unit].owner][p_units[unit].type].abilities & HERO_ABILITY_SUPER_CONSTITUTION)
            hits += (level + 1) * 3 / 2;
        if (level > 1)
            hits++;
        if (level > 3)
            hits++;
    }
    if (level > 2)
        hits++;
    if (level > 4)
        hits++;
    if (level > 5)
        hits++;
    if (level > 6)
        hits++;
    if (level > 7)
        hits++;
    if (wizards[p_units[unit].owner].globals.charm_of_life > 0) {
        bonus = hits / 4;
        if (bonus < 1)
            bonus = 1;
        hits += bonus;
    }
    return hits;
}

/* (bu) as Unit_Max_Hits for a battle record (with its combat and item enchantments, Crusade's level, the shown
   bonuses and the healing overflow)
   Numbered: BUG #89 (BUGS_AND_QUIRKS.txt) */
int Battle_Unit_Max_Hits(struct BattleUnit far *bu)
{
    int unit;
    long ench;
    int level;
    int bonus;
    int hits;

    unit = bu->unit_index;
    level = p_units[unit].level;
    ench = p_units[unit].enchantments | bu->combat_enchantments | bu->item_enchantments;
    if ((ench & UNIT_ENCH_HEROISM) && level < 3)
        level = 3;
    if (wizards[bu->owner].globals.crusade > 0 && (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) == 0)
        level++;
    hits = unit_types[p_units[unit].type].hits;
    if (ench & UNIT_ENCH_BLACK_CHANNELS) {
        hits++;
        bu->hits_bonus_shown = bu->hits_bonus_shown + 1;
    }
    if (ench & UNIT_ENCH_LIONHEART) {
        hits += 3;
        bu->hits_bonus_shown = bu->hits_bonus_shown + 3;
    }
    if (p_units[unit].hero_slot > -1) {
        if (level > 0)
            hits++;
        if (p_hero_stats[p_units[unit].owner][p_units[unit].type].abilities & HERO_ABILITY_CONSTITUTION)
            hits += level + 1;
        if (p_hero_stats[p_units[unit].owner][p_units[unit].type].abilities & HERO_ABILITY_SUPER_CONSTITUTION)
            hits += (level + 1) * 3 / 2;
        if (level > 1)
            hits++;
        if (level > 3)
            hits++;
    }
    if (level > 2)
        hits++;
    if (level > 4)
        hits++;
    if (level > 5)
        hits++;
    if (level > 6)
        hits++;
    if (level > 7)
        hits++;
    if (wizards[p_units[unit].owner].globals.charm_of_life > 0) {
        bonus = hits / 4;
        if (bonus < 1)
            bonus = 1;
        hits += bonus;
    }
    hits += bu->heal_overflow;
    bu->hits_bonus_shown += bu->heal_overflow;
    return hits;
}

/* (unit, bu) builds a unit's battle record: the unit type's statistics, cleared combat state, the upkeep, a touch
   attack's strength, the modifiers, the damage taken and a hero's first item's charges. The 0x24-byte copy from the
   type's melee takes 2 bytes of the next type too (as in Battle_Unit_Init_From_Type): harmless, combat_flags is
   cleared next
   Numbered: BUG #233 (BUGS_AND_QUIRKS.txt) */
void Battle_Unit_Init(int unit, struct BattleUnit far *bu)
{
    int charges;
    int i;

    Mem_Copy_Far(bu, (void far *)&unit_types[p_units[unit].type].melee, 36);
    bu->combat_flags = 0;
    bu->melee_hit_bonus = 0;
    bu->ranged_hit_bonus = 0;
    bu->to_hit = 0;
    bu->block_bonus = 0;
    bu->weapon_icon = 0;
    bu->melee_powers = 0;
    bu->ranged_powers = 0;
    bu->item_enchantments = 0;
    bu->heal_overflow = 0;
    bu->unit_index = unit;
    bu->web_hp = 0;
    bu->melee_bonus_shown = 0;
    bu->ranged_bonus_shown = 0;
    bu->defense_bonus_shown = 0;
    bu->resist_bonus_shown = 0;
    bu->hits_bonus_shown = 0;
    bu->melee_penalty_shown = 0;
    bu->ranged_penalty_shown = 0;
    bu->defense_penalty_shown = 0;
    bu->resist_penalty_shown = 0;
    bu->gfx_slot = -1;
    bu->status = BATTLE_UNIT_ACTIVE;
    bu->owner = p_units[unit].owner;
    for (i = 0; i < 3; i++)
        bu->damage[i] = 0;
    bu->combat_enchantments = 0;
    bu->times_attacked = 0;
    bu->spell_skill = 0;
    bu->item_charges = 0;
    bu->target_unit = -1;
    bu->touch_strength = 0;
    bu->upkeep = Unit_Gold_Upkeep(unit);
    if (bu->attack_flags & ATTACK_POISON) {
        bu->touch_strength = bu->special_strength;
        if (bu->ranged_type != RANGED_DOOM_GAZE)
            bu->special_strength = 0;
    }
    Unit_Apply_Modifiers(bu);
    bu->spell_points = bu->spell_skill;
    bu->damage[DAMAGE_NORMAL] = p_units[unit].damage;
    bu->front_damage = p_units[unit].damage % bu->hits_per_figure;
    bu->figures_alive = bu->figures - p_units[unit].damage / bu->hits_per_figure;
    bu->hit_anim_kills = 0;
    bu->confusion_result = CONFUSION_NONE;
    bu->hit_anim_intensity = 0;
    bu->zeroed_5a = 0;
    bu->zeroed_5a = 0;
    bu->attack_anim_role = 0;
    bu->aura_color = 0;
    bu->move_anim_step = 0;
    bu->moving = 0;
    bu->action_state = ACTION_NONE;
    bu->always_animate = 0;
    bu->draw_mode = DRAW_MODE_NORMAL;
    bu->anim_ability = 0;
    if (p_units[unit].hero_slot > -1) {
        if (wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[0] > -1)
            charges = p_items[wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[0]].charges;
        else
            charges = 0;
        if (charges > 0)
            bu->item_charges = charges;
    }
}

/* (bu) recomputes a battle record's modifiable statistics from its unit type: spell skill, experience, Lucky,
   hero abilities and items, weapon mutations (magic / mithril / adamantium), Chaos Surge, Holy Arms / Holy Weapon,
   the unit enchantments, moves and hit points
   Numbered: BUG #84, #234, #235, #248 (BUGS_AND_QUIRKS.txt) */
void Unit_Apply_Modifiers(struct BattleUnit far *bu)
{
    int chaos_surge;
    long ench;
    char mutations;
    int weapon;
    int unit;
    int i;

    unit = bu->unit_index;
    mutations = p_units[unit].mutation_flags;
    bu->to_hit = unit_types[p_units[bu->unit_index].type].to_hit;
    bu->melee_hit_bonus = 0;
    bu->ranged_hit_bonus = 0;
    bu->block_bonus = 0;
    bu->melee_bonus_shown = 0;
    bu->ranged_bonus_shown = 0;
    bu->defense_bonus_shown = 0;
    bu->resist_bonus_shown = 0;
    bu->hits_bonus_shown = 0;
    bu->melee_penalty_shown = 0;
    bu->ranged_penalty_shown = 0;
    bu->defense_penalty_shown = 0;
    bu->resist_penalty_shown = 0;
    bu->resist = unit_types[p_units[unit].type].resist;
    bu->defense = unit_types[p_units[unit].type].defense;
    bu->melee = unit_types[p_units[unit].type].melee;
    bu->ranged = unit_types[p_units[unit].type].ranged;
    if (bu->abilities & ABILITY_CASTER_MP40)
        bu->spell_skill = 40;
    if (bu->abilities & ABILITY_CASTER_MP20)
        bu->spell_skill = 20;
    Unit_Experience_Bonus(unit, bu);
    if (bu->abilities & ABILITY_LUCKY) {
        bu->to_hit = bu->to_hit + 1;
        bu->block_bonus = bu->block_bonus + 1;
        bu->resist = bu->resist + 1;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
    }
    if (p_units[unit].hero_slot > -1) {
        if (bu->weapon_icon == 0)
            bu->weapon_icon = 1;
        Hero_Ability_Bonus(unit, bu);
        Hero_Item_Bonus(unit, bu);
    }
    ench = p_units[unit].enchantments | bu->item_enchantments;
    weapon = p_units[unit].mutation_flags & MUT_WEAPON_GRADE;
    if (weapon > 0) {
        if (bu->melee > 0) {
            bu->melee = weapon + bu->melee - 1;
            bu->melee_bonus_shown = weapon + bu->melee_bonus_shown - 1;
            bu->melee_hit_bonus = bu->melee_hit_bonus + 1;
        }
        if (bu->ranged_type / 10 == RANGED_CLASS_MISSILE || bu->ranged_type / 10 == RANGED_CLASS_BOULDER || bu->ranged_type == RANGED_THROWN) {
            bu->ranged = weapon + bu->ranged - 1;
            bu->ranged_bonus_shown = weapon + bu->ranged_bonus_shown - 1;
            bu->ranged_hit_bonus = bu->ranged_hit_bonus + 1;
        }
        bu->defense = weapon + bu->defense - 1;
        bu->defense_bonus_shown = weapon + bu->defense_bonus_shown - 1;
        bu->weapon_icon = weapon + 1;
    }
    chaos_surge = 0;
    for (i = 0; i < num_players; i++)
        if (wizards[PLAYER_HUMAN].globals.chaos_surge > 0)   /* BUG #84: the human's every pass, not wizards[i]'s */
            chaos_surge = 1;
    if (chaos_surge == 1 && bu->race == RACE_CHAOS_CREATURE) {
        if (bu->ranged > 0) {
            bu->ranged = bu->ranged + 2;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 2;
        }
        if (bu->melee > 0) {
            bu->melee = bu->melee + 2;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 2;
        }
    }
    if ((wizards[p_units[unit].owner].globals.holy_arms > 0 && (bu->ability_flags & AFLAG_FANTASTIC) == 0 && (mutations & MUT_UNDEAD) == 0) ||
        (ench & UNIT_ENCH_HOLY_WEAPON)) {
        bu->combat_enchantments = bu->combat_enchantments | UNIT_ENCH_HOLY_WEAPON;
        bu->melee_hit_bonus = bu->melee_hit_bonus + 1;
        if (bu->ranged_type == RANGED_THROWN || bu->ranged_type / 10 == RANGED_CLASS_BOULDER || bu->ranged_type / 10 == RANGED_CLASS_MISSILE)
            bu->ranged_hit_bonus = bu->ranged_hit_bonus + 1;
        if (bu->weapon_icon == 0)
            bu->weapon_icon = 1;
    }
    /* race > RACE_TROLL takes in RACE_GENERIC (14: the catapult and the ships) too: their attacks count as magic
       weapons and get through Weapon Immunity (a bug per the patch notes ReMoM quotes); clarified using ReMoM repo */
    if (((bu->ability_flags & AFLAG_FANTASTIC) || (char)bu->race > RACE_TROLL) && bu->weapon_icon == WEAPON_NORMAL)
        bu->weapon_icon = 1;
    if (bu->ability_flags & AFLAG_NON_CORPOREAL)
        bu->move_flags = bu->move_flags | MOVE_SWIMMING;
    Unit_Enchant_Bonus(bu, ench, mutations);
    bu->moves = Unit_Move_Points(unit);
    ench = p_units[unit].enchantments | bu->item_enchantments;
    if (bu->moves < 3 && (ench & UNIT_ENCH_FLIGHT))
        bu->moves = 6;                          /* half moves: Flight gives at least 3 */
    bu->hits_per_figure = Unit_Max_Hits(unit);
}

/* (bu, ench, mutations) the effects of a unit's enchantments and of the Chaos Channels / undead mutations on its
   battle record. The undead mutation sets only the race (death creature) and the fantastic flag - no Cold, Poison or
   Illusions Immunity (ReMoM reads those as missing; clarified using ReMoM repo)
   Numbered: BUG #234 (BUGS_AND_QUIRKS.txt) */
void Unit_Enchant_Bonus(struct BattleUnit far *bu, long ench, char mutations)
{
    if (ench & UNIT_ENCH_WATER_WALKING)
        bu->move_flags = bu->move_flags | MOVE_SWIMMING;
    if (ench & UNIT_ENCH_TRUE_SIGHT)
        bu->abilities = bu->abilities | ABILITY_ILLUSIONS_IMMUNITY;
    if (ench & UNIT_ENCH_INVULNERABILITY)
        bu->abilities = bu->abilities | ABILITY_WEAPON_IMMUNITY;
    if (ench & UNIT_ENCH_FLIGHT)
        bu->move_flags = bu->move_flags | MOVE_FLYING;
    if (ench & UNIT_ENCH_WRAITH_FORM) {
        bu->ability_flags = bu->ability_flags | AFLAG_NON_CORPOREAL;
        bu->abilities = bu->abilities | ABILITY_WEAPON_IMMUNITY;
        bu->move_flags = bu->move_flags | MOVE_SWIMMING;
    }
    if (mutations & MUT_UNDEAD) {
        bu->race = RACE_DEATH_CREATURE;
        bu->ability_flags = bu->ability_flags | AFLAG_FANTASTIC;
    }
    if (ench & UNIT_ENCH_BLACK_CHANNELS) {
        if (bu->melee > 0) {
            bu->melee = bu->melee + 2;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 2;
        }
        if (bu->ranged_type != RANGED_NONE) {
            bu->ranged = bu->ranged + 1;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 1;
        }
        bu->defense = bu->defense + 1;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 1;
        bu->resist = bu->resist + 1;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
        bu->race = RACE_DEATH_CREATURE;
        bu->ability_flags = bu->ability_flags | AFLAG_FANTASTIC;
        bu->abilities = bu->abilities | (ABILITY_POISON_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_ILLUSIONS_IMMUNITY);
    }
    if (ench & UNIT_ENCH_IRON_SKIN) {
        bu->defense = bu->defense + 5;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 5;
    } else if (ench & UNIT_ENCH_STONE_SKIN) {
        bu->defense = bu->defense + 1;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 1;
    }
    if (ench & UNIT_ENCH_GUARDIAN_WIND)
        bu->abilities = bu->abilities | ABILITY_MISSILES_IMMUNITY;
    if (ench & UNIT_ENCH_MAGIC_IMMUNITY)
        bu->abilities = bu->abilities | ABILITY_MAGIC_IMMUNITY;
    if (ench & UNIT_ENCH_FLAME_BLADE) {
        if (bu->melee > 0) {
            bu->melee = bu->melee + 2;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 2;
        }
        if (bu->ranged_type == RANGED_THROWN || bu->ranged_type / 10 == RANGED_CLASS_MISSILE) {
            bu->ranged = bu->ranged + 2;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 2;
        }
        if (bu->weapon_icon == 0)
            bu->weapon_icon = 1;
    }
    if (ench & UNIT_ENCH_GIANT_STRENGTH) {
        if (bu->melee > 0) {
            bu->melee = bu->melee + 1;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 1;
        }
        if (bu->ranged_type == RANGED_THROWN) {
            bu->ranged = bu->ranged + 1;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 1;
        }
    }
    if (ench & UNIT_ENCH_IMMOLATION)
        bu->abilities2 = bu->abilities2 | ABILITY2_IMMOLATION;
    if (ench & UNIT_ENCH_ELDRITCH_WEAPON) {
        bu->melee_powers = bu->melee_powers | ATTACK_ELDRITCH_WEAPON;
        if (bu->ranged_type / 10 == RANGED_CLASS_MISSILE || bu->ranged_type == RANGED_THROWN)
            bu->ranged_powers = bu->ranged_powers | ATTACK_ELDRITCH_WEAPON;
        if (bu->weapon_icon == 0)
            bu->weapon_icon = 1;
    }
    if (mutations & MUT_CHAOS_DEMON_SKIN) {
        bu->defense = bu->defense + 3;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 3;
        bu->race = RACE_CHAOS_CREATURE;
    }
    if (mutations & MUT_CHAOS_DEMON_WINGS) {
        bu->move_flags = bu->move_flags | MOVE_FLYING;
        bu->race = RACE_CHAOS_CREATURE;
    }
    if (mutations & MUT_CHAOS_FIRE_BREATH) {
        bu->ranged = 2;
        bu->ranged_type = RANGED_FIRE_BREATH;
        bu->race = RACE_CHAOS_CREATURE;
    }
    if (ench & UNIT_ENCH_LIONHEART) {
        if (bu->melee > 0) {
            bu->melee = bu->melee + 3;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 3;
        }
        if (bu->ranged_type / 10 == RANGED_CLASS_MISSILE || bu->ranged_type / 10 == RANGED_CLASS_BOULDER || bu->ranged_type == RANGED_THROWN) {
            bu->ranged = bu->ranged + 3;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 3;
        }
        bu->resist = bu->resist + 3;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 3;
    }
    if (ench & UNIT_ENCH_HOLY_ARMOR) {
        bu->defense = bu->defense + 2;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 2;
    }
    if (bu->race == RACE_DEATH_CREATURE)
        bu->abilities = bu->abilities | ABILITY_DEATH_IMMUNITY;
    if (ench & UNIT_ENCH_BERSERK) {
        if (bu->melee > 0) {
            bu->melee_bonus_shown += bu->melee;
            bu->melee = bu->melee * 2;
        }
        bu->defense_penalty_shown = bu->defense;
        bu->defense = -20;
    }
}

/* (unit, bu) the experience-level bonuses (Heroism counts as level 3): heroes by their own table */
void Unit_Experience_Bonus(int unit, struct BattleUnit far *bu)
{
    int level;

    level = p_units[unit].level;
    if (((p_units[unit].enchantments & UNIT_ENCH_HEROISM) || (bu->combat_enchantments & UNIT_ENCH_HEROISM)) && level < 3)
        level = 3;
    if (p_units[unit].hero_slot > -1) {
        if (level > 0) {
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
            bu->defense++;
        }
        if (level > 1) {
            bu->to_hit++;
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
        if (level > 2) {
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
            bu->defense++;
        }
        if (level > 3) {
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
        if (level > 4) {
            bu->to_hit++;
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
            bu->defense++;
        }
        if (level > 5) {
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
        if (level > 6) {
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
            bu->defense++;
        }
        if (level > 7) {
            bu->to_hit++;
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
    } else {
        if (level > 0) {
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
        if (level > 1) {
            bu->resist++;
            bu->defense++;
        }
        if (level > 2) {
            bu->to_hit++;
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
        if (level > 3) {
            bu->to_hit++;
            bu->resist++;
            bu->defense++;
        }
        if (level > 4) {
            bu->to_hit++;
            bu->resist++;
            if (bu->ranged > 0)
                bu->ranged++;
            if (bu->melee > 0)
                bu->melee++;
        }
    }
}

/* (unit, bu) a hero's abilities (p_hero_stats bits): no upkeep with HERO_ABILITY_NOBLE or for a Famous wizard's hero;
   level-based defense, to-hit, melee and magic-ranged bonuses; caster level -> spell skill; HERO_ABILITY_LUCKY
   Numbered: BUG #92 (BUGS_AND_QUIRKS.txt) */
void Hero_Ability_Bonus(int unit, struct BattleUnit far *bu)
{
    int owner;
    int hero_type;

    owner = p_units[unit].owner;
    hero_type = p_units[unit].type;
    if ((p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_NOBLE) || wizards[owner].retorts[RETORT_FAMOUS] > 0)
        bu->upkeep = 0;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_AGILE)
        bu->defense = p_units[unit].level + bu->defense + 1;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_SUPER_AGILE)
        bu->defense = bu->defense + (p_units[unit].level + 1) * 3 / 2;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_BLADEMASTER)
        bu->to_hit = bu->to_hit + (p_units[unit].level + 1) / 2;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_SUPER_BLADEMASTER)
        bu->to_hit = bu->to_hit + (p_units[unit].level + 1) * 3 / 4;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_MIGHT)
        bu->melee = p_units[unit].level + bu->melee + 1;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_SUPER_MIGHT)
        bu->melee = bu->melee + (p_units[unit].level + 1) * 3 / 2;
    if ((p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_ARCANE_POWER) && bu->ranged_type / 10 == RANGED_CLASS_MAGIC)
        bu->ranged = p_units[unit].level + bu->ranged + 1;
    if ((p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_SUPER_ARCANE_POWER) && bu->ranged_type / 10 == RANGED_CLASS_MAGIC)
        bu->ranged = bu->ranged + (p_units[unit].level + 1) * 3 / 2;
    if (p_hero_stats[owner][hero_type].caster_level > 0)
        bu->spell_skill = (p_hero_stats[owner][hero_type].caster_level + 1) * (p_units[unit].level + 1) * 5 / 2;
    else
        bu->spell_skill = 0;
    if (p_hero_stats[owner][hero_type].abilities & HERO_ABILITY_LUCKY) {
        bu->to_hit = bu->to_hit + 1;
        bu->block_bonus = bu->block_bonus + 1;
        bu->resist = bu->resist + 1;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
    }
}

/* (bu) the combat-only modifiers on top of Unit_Apply_Modifiers: a node's +2 for its own realm's creatures, the
   Leadership / Holy Bonus / Resistance auras, Haste items, High Prayer / Prayer, True Light / Darkness for Life and
   Death creatures, Metal Fires, Mass Invisibility, Warp Reality, Black Prayer, the curses, the combat-cast
   enchantments, Weakness (-2 melee, -2 missile / thrown), Shatter (attacks capped at 1), Warp Creature's halving, Mind Twist, clamping at 0, Web, hit points.
   BUGS #85, #88: Mind Twist (COMBAT_FLAG_MIND_TWIST) counts in the bonus column; Weakness (COMBAT_FLAG_WEAKNESS) tests
   ranged_type / 10 == RANGED_THROWN (100); Shatter's shown penalties are
   written after the attack is set to 1, so they are always 0
   Numbered: BUG #85, #86, #88, #91, #246, #291 (BUGS_AND_QUIRKS.txt) */
void Battle_Unit_Combat_Modifiers(struct BattleUnit far *bu)
{
    int unit;
    long ench;
    char mutations;
    int item;
    int node_bonus;
    register int i;

    unit = bu->unit_index;
    node_bonus = 0;
    if (combat_node_type == NODE_SORCERY && bu->race == RACE_SORCERY_CREATURE)
        node_bonus = 1;
    else if (combat_node_type == NODE_CHAOS && bu->race == RACE_CHAOS_CREATURE)
        node_bonus = 1;
    else if (combat_node_type == NODE_NATURE && bu->race == RACE_NATURE_CREATURE)
        node_bonus = 1;
    if (node_bonus != 0) {
        bu->melee = bu->melee + 2;
        bu->melee_bonus_shown = bu->melee_bonus_shown + 2;
        if (bu->ranged > 0) {
            bu->ranged = bu->ranged + 2;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 2;
        }
        bu->resist = bu->resist + 2;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 2;
        bu->defense = bu->defense + 2;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 2;
    }
    if (bu->melee > 0 && (char)bu->race < RACE_ARCANE_CREATURE) {
        bu->melee = bu->melee + p_aura_leadership[bu->owner];
        bu->melee_bonus_shown = bu->melee_bonus_shown + p_aura_leadership[bu->owner];
    }
    if (bu->ranged > 0 && (char)bu->race < RACE_ARCANE_CREATURE && bu->ranged_type / 10 != RANGED_CLASS_MAGIC) {
        bu->ranged = bu->ranged + p_aura_leadership[bu->owner] / 2;
        bu->ranged_bonus_shown = bu->ranged_bonus_shown + p_aura_leadership[bu->owner] / 2;
    }
    if (bu->melee > 0) {
        bu->melee = bu->melee + p_aura_holy_bonus[bu->owner];
        bu->melee_bonus_shown = bu->melee_bonus_shown + p_aura_holy_bonus[bu->owner];
    }
    bu->resist = bu->resist + p_aura_resistance[bu->owner];
    bu->resist_bonus_shown = bu->resist_bonus_shown + p_aura_resistance[bu->owner];
    bu->defense = bu->defense + p_aura_holy_bonus[bu->owner];
    bu->defense_bonus_shown = bu->defense_bonus_shown + p_aura_holy_bonus[bu->owner];
    if (p_units[unit].hero_slot > -1) {
        for (i = 0; i < 3; i++) {
            item = wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index[i];
            if (item > -1 && (p_items[item].powers & ITEM_POWER_HASTE))
                bu->combat_flags = bu->combat_flags | COMBAT_FLAG_HASTE;
        }
    }
    mutations = p_units[bu->unit_index].mutation_flags;
    if ((p_combat_enchants[CENCH_HIGH_PRAYER_ATT] == 1 && bu->owner == combat_attacker_player) ||
        (p_combat_enchants[CENCH_HIGH_PRAYER_DEF] == 1 && bu->owner == combat_defender_player)) {
        bu->to_hit = bu->to_hit + 1;
        bu->block_bonus = bu->block_bonus + 1;
        bu->resist = bu->resist + 1;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
        if (bu->melee > 0) {
            bu->melee = bu->melee + 2;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 2;
        }
        bu->resist = bu->resist + 2;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 2;
        bu->defense = bu->defense + 2;
        bu->defense_bonus_shown = bu->defense_bonus_shown + 2;
    } else if ((p_combat_enchants[CENCH_PRAYER_ATT] == 1 && bu->owner == combat_attacker_player) ||
               (p_combat_enchants[CENCH_PRAYER_DEF] == 1 && bu->owner == combat_defender_player)) {
        bu->to_hit = bu->to_hit + 1;
        bu->block_bonus = bu->block_bonus + 1;
        bu->resist = bu->resist + 1;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
    }
    if (bu->race == RACE_LIFE_CREATURE) {
        if (p_combat_enchants[CENCH_TRUE_LIGHT_ATT] != 0 || p_combat_enchants[CENCH_TRUE_LIGHT_DEF] != 0) {
            if (bu->melee > 0) {
                bu->melee = bu->melee + 1;
                bu->melee_bonus_shown = bu->melee_bonus_shown + 1;
            }
            if (bu->ranged > 0) {
                bu->ranged = bu->ranged + 1;
                bu->ranged_bonus_shown = bu->ranged_bonus_shown + 1;
            }
            bu->resist = bu->resist + 1;
            bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
            bu->defense = bu->defense + 1;
            bu->defense_bonus_shown = bu->defense_bonus_shown + 1;
        }
        if (p_combat_enchants[CENCH_DARKNESS_ATT] != 0 || p_combat_enchants[CENCH_DARKNESS_DEF] != 0) {
            bu->melee = bu->melee - 1;
            bu->melee_penalty_shown = bu->melee_penalty_shown + 1;
            bu->ranged = bu->ranged - 1;
            bu->ranged_penalty_shown = bu->ranged_penalty_shown + 1;
            bu->resist = bu->resist - 1;
            bu->resist_penalty_shown = bu->resist_penalty_shown + 1;
            bu->defense = bu->defense - 1;
            bu->defense_penalty_shown = bu->defense_penalty_shown + 1;
        }
    }
    if (bu->race == RACE_DEATH_CREATURE) {
        if (p_combat_enchants[CENCH_DARKNESS_ATT] != 0 || p_combat_enchants[CENCH_DARKNESS_DEF] != 0) {
            if (bu->melee > 0) {
                bu->melee = bu->melee + 1;
                bu->melee_bonus_shown = bu->melee_bonus_shown + 1;
            }
            if (bu->ranged > 0) {
                bu->ranged = bu->ranged + 1;
                bu->ranged_bonus_shown = bu->ranged_bonus_shown + 1;
            }
            bu->resist = bu->resist + 1;
            bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
            bu->defense = bu->defense + 1;
            bu->defense_bonus_shown = bu->defense_bonus_shown + 1;
        }
        if (p_combat_enchants[CENCH_TRUE_LIGHT_ATT] != 0 || p_combat_enchants[CENCH_TRUE_LIGHT_DEF] != 0) {
            bu->melee = bu->melee - 1;
            bu->melee_penalty_shown = bu->melee_penalty_shown + 1;
            bu->ranged = bu->ranged - 1;
            bu->ranged_penalty_shown = bu->ranged_penalty_shown + 1;
            bu->resist = bu->resist - 1;
            bu->resist_penalty_shown = bu->resist_penalty_shown + 1;
            bu->defense = bu->defense - 1;
            bu->defense_penalty_shown = bu->defense_penalty_shown + 1;
        }
    }
    ench = p_units[bu->unit_index].enchantments | bu->combat_enchantments;
    if (((p_combat_enchants[CENCH_METAL_FIRES_ATT] != 0 && bu->owner == combat_attacker_player) ||
         (p_combat_enchants[CENCH_METAL_FIRES_DEF] != 0 && bu->owner != combat_attacker_player)) &&
        (bu->ability_flags & AFLAG_FANTASTIC) == 0 && (ench & UNIT_ENCH_FLAME_BLADE) == 0) {
        if (bu->melee > 0) {
            bu->melee = bu->melee + 1;
            bu->melee_bonus_shown = bu->melee_bonus_shown + 1;
        }
        if (bu->ranged_type / 10 == RANGED_CLASS_MISSILE || bu->ranged_type == RANGED_THROWN) {
            bu->ranged = bu->ranged + 1;
            bu->ranged_bonus_shown = bu->ranged_bonus_shown + 1;
        }
        if (bu->weapon_icon == 0)
            bu->weapon_icon = 1;
    }
    if ((p_combat_enchants[CENCH_MASS_INVISIBILITY_ATT] != 0 && bu->owner == combat_attacker_player) ||
        (p_combat_enchants[CENCH_MASS_INVISIBILITY_DEF] != 0 && bu->owner != combat_attacker_player))
        bu->combat_enchantments = bu->combat_enchantments | UNIT_ENCH_INVISIBILITY;
    if ((p_combat_enchants[CENCH_WARP_REALITY_ATT] != 0 || p_combat_enchants[CENCH_WARP_REALITY_DEF] != 0) && bu->race != RACE_CHAOS_CREATURE)
        bu->to_hit = bu->to_hit - 2;
    if ((p_combat_enchants[CENCH_BLACK_PRAYER_ATT] != 0 && bu->owner != combat_attacker_player) ||
        (p_combat_enchants[CENCH_BLACK_PRAYER_DEF] != 0 && bu->owner == combat_attacker_player)) {
        bu->melee = bu->melee - 1;
        bu->ranged = bu->ranged - 1;
        bu->resist = bu->resist - 2;
        bu->melee_penalty_shown = bu->melee_penalty_shown + 1;
        bu->ranged_penalty_shown = bu->ranged_penalty_shown + 1;
        bu->resist_penalty_shown = bu->resist_penalty_shown + 2;
        bu->defense = bu->defense - 1;
        bu->defense_penalty_shown = bu->defense_penalty_shown + 1;
    }
    if (bu->combat_flags & COMBAT_FLAG_MIND_TWIST) {
        bu->to_hit = bu->to_hit - 1;
        bu->resist = bu->resist - 1;
        bu->resist_bonus_shown = bu->resist_bonus_shown + 1;
    }
    if (bu->combat_flags & COMBAT_FLAG_VERTIGO) {
        bu->to_hit = bu->to_hit - 2;
        bu->defense = bu->defense - 1;
        bu->defense_penalty_shown = bu->defense_penalty_shown + 1;
    }
    if (bu->combat_flags & COMBAT_FLAG_WEAKNESS) {
        bu->melee = bu->melee - 2;
        bu->melee_penalty_shown = bu->melee_penalty_shown + 2;
        if (bu->ranged_type / 10 == RANGED_CLASS_MISSILE || bu->ranged_type / 10 == RANGED_THROWN) {
            bu->ranged = bu->ranged - 2;
            bu->ranged_penalty_shown = bu->ranged_penalty_shown + 2;
        }
    }
    if (bu->combat_flags & COMBAT_FLAG_MIND_STORM) {
        bu->melee = bu->melee - 5;
        bu->ranged = bu->ranged - 5;
        bu->resist = bu->resist - 5;
        bu->defense = bu->defense - 5;
        bu->defense_penalty_shown = bu->defense_penalty_shown + 5;
        bu->melee_penalty_shown = bu->melee_penalty_shown + 5;
        bu->ranged_penalty_shown = bu->ranged_penalty_shown + 5;
        bu->resist_penalty_shown = bu->resist_penalty_shown + 5;
    }
    ench = p_units[bu->unit_index].enchantments;
    ench = (ench ^ bu->combat_enchantments) & bu->combat_enchantments;   /* the combat-cast ones the unit lacks */
    Unit_Enchant_Bonus(bu, ench, mutations);
    if (bu->combat_flags & COMBAT_FLAG_WARP_MELEE) {
        bu->melee_penalty_shown = bu->melee_penalty_shown + (bu->melee + 1) / 2;
        bu->melee = bu->melee - (bu->melee + 1) / 2;
    }
    if (bu->combat_flags & COMBAT_FLAG_WARP_DEFENSE) {
        bu->defense_penalty_shown = bu->defense_penalty_shown + (bu->defense + 1) / 2;
        bu->defense = bu->defense - (bu->defense + 1) / 2;
    }
    if (bu->combat_flags & COMBAT_FLAG_WARP_RESIST) {
        bu->resist_penalty_shown += bu->resist;
        bu->resist = 0;
    }
    if (bu->combat_flags & COMBAT_FLAG_SHATTER) {
        if (bu->melee > 1) {
            bu->melee = 1;
            bu->melee_penalty_shown = bu->melee - 1;
        }
        if (bu->ranged > 1) {
            bu->ranged = 1;
            bu->ranged_penalty_shown = bu->ranged - 1;
        }
    }
    if (bu->melee < 0)
        bu->melee = 0;
    if (bu->ranged < 0)
        bu->ranged = 0;
    if (bu->defense < 0)
        bu->defense = 0;
    if ((bu->combat_flags & COMBAT_FLAG_WEB) && (bu->move_flags & MOVE_FLYING))
        bu->move_flags = bu->move_flags ^ MOVE_FLYING;
    bu->hits_per_figure = Battle_Unit_Max_Hits(bu);
}

/* (unit, spell) whether a unit has the enchantment a spell casts (its own or from an item); fills the shared
   p_unit_stats_buf (Battle_Unit_Init), overwriting what the unit view reads */
int Unit_Has_Spell_Enchant(int unit, int spell)
{
    long ench;
    int found;

    Battle_Unit_Init(unit, p_unit_stats_buf);
    ench = p_units[unit].enchantments | p_unit_stats_buf->item_enchantments;
    if (ench & p_spell_data[spell].effect_param)
        found = 1;
    else
        found = 0;
    return found;
}
