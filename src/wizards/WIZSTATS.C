/* WIZSTATS.C - WIZARDS.EXE code segment a0ec (unit creation, casting skill / cost, hero picks, move points,
   the normal-living test, the PWR cheat, AI strength, unit-type upkeep and sounds, and dropping a deleted city's
   notices).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS a0ec src/wizards/WIZSTATS.C */
/* no <stdlib.h>: abs() is called as a function (the header's inline abs would differ) */
#include "STRUCTS.H"
#include "ENUMS.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order; batch 39) */
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
int Wizard_Casting_Skill(int player);
int Spell_Casting_Cost(int player, int spell, int in_combat);
int Spell_Cost_Reduction_Pct(int player, int spell);
int Wizard_Hero_Count(int player);
int Wizard_Dead_Hero_Count(int player);
int Hero_Pick_Random(int player, int mode, int pool);
int Unit_Move_Points(int unit);
int Unit_Is_Normal_Living(int unit);
void Cheat_PWR(void);
int UnitType_AI_Strength(int unit_type);
int AI_Unit_Strength(int unit);
void UnitTypes_Init_Upkeep_Sounds(void);
void City_Notices_Remove_City(int city);

extern struct Unit far *p_units;                /* DS:9ec2 */
extern struct City far *p_cities;               /* DS:9cbc */
extern struct Fortress far *p_fortresses;       /* DS:9cc8 */
extern struct Item far *p_items;                /* DS:9136 */
extern struct Spell far *p_spell_data;          /* DS:912c */
extern struct HeroStats far *p_hero_stats[6];   /* DS:9232 */
extern struct BattleUnit far *p_unit_stats_buf; /* DS:9226 */
extern struct Wizard wizards[6];                /* DS:9eca */
extern struct UnitType unit_types[];            /* DS:019c */
extern int exp_level_thresholds[9];             /* DS:1d74 */
extern int num_units;                           /* DS:bd92 */
extern int num_players;                         /* DS:bd9c */
extern char city_notice_count;                  /* DS:9af0 */
extern struct CityNotice city_notices[];        /* DS:9af2 */
extern char message_text[];                     /* DS:c7fc */

int Unit_Calc_Level(int unit);
int City_Weapon_Grade(int city);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
void Message_Box(char *text);
void City_Remove(int city);
void Chaos_Channels_Apply(int unit);
int Wizard_Effective_Fame(int player);
int Random(int n);
int AI_Toughness_Value(int hits, int defense);
int AI_Missile_Value(int ranged, int figures, int attack_flags);
int AI_Attack_Value(int melee, int ranged, int figures, int attack_flags, int ranged_type);
void Battle_Unit_Init(int unit, struct BattleUnit far *bu);

/* (unit_type, owner, x, y, plane, src) -> 1 created / 0 at the unit cap, or for a settler that would empty a
   wizard's fortress city (the citizen given back; message 11 for the human): src = the city that built it (0 .. 1999: its
   Fighters' Guild / War College / Altar of Battle experience and weapon grade), -1 none, below -1 a starting level
   abs(src) - 1 (AINEUTRL.C), 2000 ignores the unit caps (950 for the AI, 980 for all) */
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src)
{
    int player;

    if (owner != PLAYER_HUMAN && num_units > UNIT_CAP_AI && src != UNIT_SRC_NO_CAP)
        return 0;
    if (src != UNIT_SRC_NO_CAP && num_units > UNIT_CAP)
        return 0;
    if (num_units == MAX_UNITS)
        return 0;
    {
        p_units[num_units].x = x;
        p_units[num_units].y = y;
        p_units[num_units].plane = plane;
        p_units[num_units].owner = owner;
        p_units[num_units].moves_max = unit_types[unit_type].moves;
        p_units[num_units].type = unit_type;
        p_units[num_units].hero_slot = -1;
        p_units[num_units].in_tower = 0;
        p_units[num_units].finished = 1;
        p_units[num_units].moves_left = 0;
        p_units[num_units].scout_range = unit_types[unit_type].scouting;
        p_units[num_units].dest_x = 0;
        p_units[num_units].dest_y = 0;
        p_units[num_units].order = ORDER_READY;
        p_units[num_units].level = 0;
        p_units[num_units].experience = 0;
        p_units[num_units].damage = 0;
        p_units[num_units].draw_priority = 0;
        p_units[num_units].enchantments = 0;
        p_units[num_units].mutation_flags = WEAPON_NORMAL;   /* the whole byte: no mutations, weapon grade normal */
        p_units[num_units].goto_blocked = 0;
        p_units[num_units].road_turns = -1;
        if (src >= 0 && src < UNIT_SRC_NO_CAP) {
            if (p_cities[src].buildings[BLD_FIGHTERS_GUILD] == BLDSTATE_BUILT ||
                p_cities[src].buildings[BLD_FIGHTERS_GUILD] == BLDSTATE_REPLACED)
                p_units[num_units].experience = exp_level_thresholds[1];
            if (p_cities[src].buildings[BLD_WAR_COLLEGE] == BLDSTATE_BUILT ||
                p_cities[src].buildings[BLD_WAR_COLLEGE] == BLDSTATE_REPLACED)
                p_units[num_units].experience = exp_level_thresholds[2];
            if (p_cities[src].enchants[CITY_ENCH_ALTAR_OF_BATTLE] > 0)
                p_units[num_units].experience = exp_level_thresholds[3];
            p_units[num_units].mutation_flags = City_Weapon_Grade(src);
            if ((unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) != 0) {
                p_cities[src].population = p_cities[src].population - 1;
                if (p_cities[src].population == 0) {
                    p_cities[src].population_tens = 3;
                    if (p_cities[src].owner < num_players) {
                        for (player = 0; player < num_players; player++) {
                            if (p_cities[src].x == p_fortresses[player].x && p_cities[src].y == p_fortresses[player].y &&
                                p_cities[src].plane == p_fortresses[player].plane) {
                                p_cities[src].population = p_cities[src].population + 1;
                                if (player == PLAYER_HUMAN) {
                                    LBX_Load_Records_Near("message", 0, message_text, 11, 1, 150);
                                    Message_Box(message_text);
                                }
                                return 0;
                            }
                        }
                    }
                    City_Remove(src);
                }
            }
            if (wizards[owner].retorts[RETORT_ALCHEMY] > 0)
                if (p_units[num_units].mutation_flags == 0)
                    p_units[num_units].mutation_flags = WEAPON_MAGIC;
            if (wizards[owner].globals.doom_mastery > 0 && (unit_types[unit_type].ability_flags & AFLAG_FANTASTIC) == 0)
                Chaos_Channels_Apply(num_units);
            p_units[num_units].level = Unit_Calc_Level(num_units);
        } else if (src < -1) {
            src = abs(src) - 1;
            p_units[num_units].experience = exp_level_thresholds[src];
            p_units[num_units].level = Unit_Calc_Level(num_units);
        }
        num_units++;
        return 1;
    }
}

/* (player) -> the casting skill: the n for which 2 + 4 + .. + 2n (= n (n + 1)) first reaches the skill points;
   Archmage +10 */
int Wizard_Casting_Skill(int player)
{
    long sum;
    int step;
    int skill;

    step = 0;
    skill = 0;
    sum = 0;
    while (wizards[player].skill_points > sum) {
        skill++;
        step += 2;
        sum += step;
    }
    if (wizards[player].retorts[RETORT_ARCHMAGE] > 0)
        skill += 10;
    return skill;
}

/* (player, spell, in_combat) -> the cost: the base cost, x 3 / 2 for Nature and Life spells while anyone has
   Evil Omens, x 5 for a combat-capable spell (castable 0 / 2 / 3) cast overland, less the reduction %
   Numbered: BUG #41, #306 (BUGS_AND_QUIRKS.txt) */
int Spell_Casting_Cost(int player, int spell, int in_combat)
{
    int i;
    int omens;
    int reduction;
    int cost;

    reduction = Spell_Cost_Reduction_Pct(player, spell);
    cost = p_spell_data[spell].base_cost;
    if (p_spell_data[spell].realm == REALM_NATURE || p_spell_data[spell].realm == REALM_LIFE) {
        omens = 0;
        for (i = 0; i < num_players; i++)
            if (wizards[i].globals.evil_omens > 0)
                omens = 1;
        if (omens == 1)
            cost = cost * 3 / 2;
    }
    /* castable 0 / 2 / 3 (castable both ways; not 4): five times the cost overland */
    if ((p_spell_data[spell].castable == 0 || p_spell_data[spell].castable == 2 || p_spell_data[spell].castable == 3) &&
        in_combat == 0)
        cost = cost * 5;
    cost = cost - (int)((long)cost * reduction / 100);
    return cost;
}

/* (player, spell) -> the casting cost reduction %: 10 per book of the realm above 7, the realm's Mastery 15
   (Nature, Sorcery, Chaos), Artificer 50 for item creation, Conjurer 25 for summons, Runemaster 25 for Arcane */
int Spell_Cost_Reduction_Pct(int player, int spell)
{
    int excess;
    int books;
    int pct;

    pct = 0;
    if (p_spell_data[spell].realm != REALM_ARCANE)
        books = wizards[player].spellbooks[p_spell_data[spell].realm];
    else
        books = 0;
    if (books > 7) {
        excess = books - 7;
        pct += excess * 10;
    }
    if (p_spell_data[spell].realm == REALM_NATURE && wizards[player].retorts[RETORT_NATURE_MASTERY] > 0)
        pct += 15;
    if (p_spell_data[spell].realm == REALM_SORCERY && wizards[player].retorts[RETORT_SORCERY_MASTERY] > 0)
        pct += 15;
    if (p_spell_data[spell].realm == REALM_CHAOS && wizards[player].retorts[RETORT_CHAOS_MASTERY] > 0)
        pct += 15;
    if (wizards[player].retorts[RETORT_ARTIFICER] > 0 && p_spell_data[spell].effect_type == SPELL_FX_CREATE_ITEM)
        pct += 50;
    if (wizards[player].retorts[RETORT_CONJURER] > 0 && p_spell_data[spell].effect_type == SPELL_FX_SUMMON)
        pct += 25;
    if (p_spell_data[spell].realm == REALM_ARCANE && wizards[player].retorts[RETORT_RUNEMASTER] > 0)
        pct += 25;
    return pct;
}

/* (player) -> how many hired-hero slots are filled */
int Wizard_Hero_Count(int player)
{
    int slot;
    int count;

    count = 0;
    for (slot = 0; slot < 6; slot++)
        if (wizards[player].hired_heroes[slot].unit_index != -1)
            count++;
    return count;
}

/* (player) -> how many of hero types 0 .. 33 are dead (status < 0, not HERO_STATUS_UNAVAILABLE) */
int Wizard_Dead_Hero_Count(int player)
{
    int hero;
    int count;

    count = 0;
    for (hero = 0; hero < UNIT_HERO_CHOSEN; hero++)
        if (p_hero_stats[player][hero].status < 0 && p_hero_stats[player][hero].status != HERO_STATUS_UNAVAILABLE)
            count++;
    return count;
}

/* (player, mode, pool) -> a random hero type the player can get, or -1: still available, types 27 / 28 only with
   Life books, 29 / 32 only with Death books, and (cost - 100) / 10 within the effective fame (mode 1: fame
   200, any). Pool 0 all 34 types, 1 the first 24, 2 types 24 .. 33 (retried as pool 0 when none fits - BUG #245: tries is not reset, so the retry gets one try); 10
   tries (100 when mode > 0); nothing when the unit table is full
   Numbered: BUG #245 (BUGS_AND_QUIRKS.txt) */
int Hero_Pick_Random(int player, int mode, int pool)
{
    int found;
    int fame;
    int tries;
    int max_tries;
    int hero;

    tries = 0;
    if (mode > HPICK_OFFER)
        max_tries = 100;
    else
        max_tries = 10;
    fame = Wizard_Effective_Fame(player);
    if (mode == HPICK_SUMMONED)
        fame = 200;
    found = 0;
retry:
    if (num_units + 1 < MAX_UNITS) {
        do {
            tries++;
            if (pool == HPICK_POOL_ALL)
                hero = Random(UNIT_HERO_CHOSEN) - 1;    /* 0 .. 33: every hero but the Chosen */
            else if (pool == HPICK_POOL_BASIC)
                hero = Random(UNIT_HERO_WARLOCK + 1) - 1;   /* the basic heroes 0 .. 23 */
            else
                hero = Random(UNIT_HERO_CHOSEN - 1 - UNIT_HERO_WARLOCK) + UNIT_HERO_WARLOCK;   /* the champions 24 .. 33 */
            if (p_hero_stats[player][hero].status >= 0 &&
                ((hero != UNIT_HERO_PRIESTESS && hero != UNIT_HERO_PALADIN) || wizards[player].spellbooks[REALM_LIFE] > 0) &&
                ((hero != UNIT_HERO_BLACK_KNIGHT && hero != UNIT_HERO_NECROMANCER) || wizards[player].spellbooks[REALM_DEATH] > 0) &&
                (unit_types[hero].cost - 100) / 10 <= fame)
                found = 1;
        } while (found == 0 && tries < max_tries);
    }
    if (pool == HPICK_POOL_CHAMPIONS && found == 0) {
        pool = HPICK_POOL_ALL;
        goto retry;
    }
    if (found == 0)
        return -1;
    else
        return hero;
}

/* (unit) -> the moves per turn: the type's, at least 6 with flight (item or enchantment), at least 4 with
   Chaos Channels' demon wings, +2 with Endurance, + the items' move bonuses; a transport moves x 3 / 2 when
   only its owner's Wind Mastery counts, / 2 against more enemy Wind Masteries than its own
   Numbered: BUG #235, #316 (BUGS_AND_QUIRKS.txt) */
int Unit_Move_Points(int unit)
{
    int endurance_item;
    int item_moves;
    int wind;
    int *items;
    long powers;
    int k;
    int moves;

    item_moves = 0;
    endurance_item = 0;
    powers = 0;
    if (p_units[unit].hero_slot > -1) {
        items = wizards[p_units[unit].owner].hired_heroes[p_units[unit].hero_slot].item_index;
        for (k = 0; k < 3; k++) {
            if (items[k] > -1) {
                if ((p_items[items[k]].powers & ITEM_POWER_ENDURANCE) > 0)
                    endurance_item = 1;
                powers = powers | p_items[items[k]].powers;
                item_moves += p_items[items[k]].moves;
            }
        }
    }
    moves = unit_types[p_units[unit].type].moves;
    powers = powers | p_units[unit].enchantments;   /* item powers and unit enchantments share the two bits
                                                   tested here (Flight 0x20000, Endurance 0x2000), not the rest */
    if (moves < 6 && (powers & ITEM_POWER_FLIGHT) != 0)
        moves = 6;
    if ((p_units[unit].mutation_flags & MUT_CHAOS_DEMON_WINGS) != 0 && moves < 4)
        moves = 4;
    if ((powers & ITEM_POWER_ENDURANCE) > 0 || endurance_item == 1)
        moves += 2;
    moves += item_moves;
    if (unit_types[p_units[unit].type].transport_capacity > 0) {
        wind = 0;
        for (k = 0; k < num_players; k++) {
            if (wizards[k].globals.wind_mastery > 0) {
                if (p_units[unit].owner == k)
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
    return moves;
}

/* (unit) -> 1 unless the unit is fantastic or undead
   Numbered: BUG #213 (BUGS_AND_QUIRKS.txt) */
int Unit_Is_Normal_Living(int unit)
{
    int result;

    result = 1;
    if ((unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) != 0 || (p_units[unit].mutation_flags & MUT_UNDEAD) != 0)
        result = 0;
    return result;
}

/* () the magic screen's "PWR" cheat: every wizard gets 10000 mana, gold and skill points and knows every spell
   it could research; the human gets Detect Magic, and its research candidates and spell are cleared */
void Cheat_PWR(void)
{
    int player;
    int book;
    int slot;

    for (player = 0; player < 8; player++)      /* player here: the candidate index */
        wizards[PLAYER_HUMAN].research_candidates[player] = 0;
    for (player = 0; player < num_players; player++) {
        wizards[player].mana_reserve = 10000;
        wizards[player].gold = 10000;
        wizards[player].skill_points = 10000;
        wizards[player].nominal_casting_skill = Wizard_Casting_Skill(player);
        wizards[player].skill_left_this_turn = wizards[player].nominal_casting_skill;
        for (book = 0; book <= 5; book++)
            for (slot = 0; slot < 40; slot++)
                if (wizards[player].spell_library[book][slot] % 2 == 1)   /* RESEARCHABLE (1) or OFFERED (3) */
                    wizards[player].spell_library[book][slot] = SPELLSTATE_KNOWN;
    }
    wizards[PLAYER_HUMAN].globals.detect_magic = 1;
    wizards[PLAYER_HUMAN].research_spell = SPELL_NONE;
}

/* (unit_type) -> the AI's strength estimate of a unit type: toughness (hits x figures against defense), twice the
   missile value for a ranged attack up to the magic class, and the melee value (with a thrown / breath /
   gaze attack's strength)
   Numbered: BUG #318 (BUGS_AND_QUIRKS.txt) */
int UnitType_AI_Strength(int unit_type)
{
    int hp_total;                               /* reassigned inside the call (comma expression): the ranged strength or 0 */
    int strength;

    strength = 0;
    hp_total = unit_types[unit_type].hits * unit_types[unit_type].figures;
    strength += AI_Toughness_Value(hp_total, unit_types[unit_type].defense);
    if (unit_types[unit_type].ranged_type / 10 <= RANGED_CLASS_MAGIC)
        strength += AI_Missile_Value(unit_types[unit_type].ranged, unit_types[unit_type].figures, unit_types[unit_type].attack_flags) * 2;
    strength += AI_Attack_Value(unit_types[unit_type].melee, hp_total, unit_types[unit_type].figures, unit_types[unit_type].attack_flags,
                                unit_types[unit_type].ranged_type >= RANGED_THROWN ? (hp_total = unit_types[unit_type].ranged, unit_types[unit_type].ranged_type)
                                                                    : (hp_total = 0, -1));
    return strength;
}

/* (unit) -> the same estimate for an actual unit, from its battle record (hits left, items' powers); 0 for an
   index outside the table (the test lets num_units itself through)
   Numbered: BUG #318 (BUGS_AND_QUIRKS.txt) */
int AI_Unit_Strength(int unit)
{
    int strength;
    int hp_total;                               /* reassigned inside the call (comma expression): the ranged strength or 0 */

    if (unit < 0 || unit > num_units)
        return 0;
    strength = 0;
    Battle_Unit_Init(unit, p_unit_stats_buf);
    hp_total = p_unit_stats_buf->figures_alive * p_unit_stats_buf->hits_per_figure - p_unit_stats_buf->front_damage;
    strength += AI_Toughness_Value(hp_total, p_unit_stats_buf->defense);
    if (p_unit_stats_buf->ranged_type / 10 <= RANGED_CLASS_MAGIC && p_unit_stats_buf->ranged_type != RANGED_NONE)   /* the type-level
                                                   twin above leaves RANGED_NONE in */
        strength += AI_Missile_Value(p_unit_stats_buf->ranged, p_unit_stats_buf->figures_alive,
                                     p_unit_stats_buf->attack_flags | p_unit_stats_buf->ranged_powers) * 2;
    strength += AI_Attack_Value(p_unit_stats_buf->melee, hp_total, p_unit_stats_buf->figures_alive,
                                p_unit_stats_buf->attack_flags | p_unit_stats_buf->melee_powers,
                                p_unit_stats_buf->ranged_type >= RANGED_THROWN ? (hp_total = p_unit_stats_buf->ranged, p_unit_stats_buf->ranged_type)
                                                                     : (hp_total = 0, -1));
    return strength;
}

/* () unit types 0 .. 153: upkeep (cost - 1) / 50 + 1, 0 for each race's first spearmen (Dwarf: swordsmen); then
   every type's sound effect */
void UnitTypes_Init_Upkeep_Sounds(void)
{
    int unit_type;

    for (unit_type = 0; unit_type < UNIT_MAGIC_SPIRIT; unit_type++) {
        switch (unit_type) {
        case UNIT_BARBARIAN_SPEARMEN:
        case UNIT_BEASTMEN_SPEARMEN:
        case UNIT_DARK_ELF_SPEARMEN:
        case UNIT_DRACONIAN_SPEARMEN:
        case UNIT_DWARF_SWORDSMEN:
        case UNIT_GNOLL_SPEARMEN:
        case UNIT_HALFLING_SPEARMEN:
        case UNIT_HIGH_ELF_SPEARMEN:
        case UNIT_HIGH_MEN_SPEARMEN:
        case UNIT_KLACKON_SPEARMEN:
        case UNIT_LIZARDMAN_SPEARMEN:
        case UNIT_NOMAD_SPEARMEN:
        case UNIT_ORC_SPEARMEN:
        case UNIT_TROLL_SPEARMEN:
            unit_types[unit_type].upkeep = 0;
            break;
        default:
            unit_types[unit_type].upkeep = (unit_types[unit_type].cost - 1) / 50 + 1;
        }
    }
    /* the unit sounds: high nibble the movement sound (SOUNDFX entry), low nibble the attack sound (CMBTSND entry);
       COMBUTIL.C reads them */
    unit_types[UNIT_HERO_DWARF].sounds = 0x66;
    unit_types[UNIT_HERO_BARBARIAN].sounds = 0x66;
    unit_types[UNIT_HERO_SAGE].sounds = 0x67;
    unit_types[UNIT_HERO_DERVISH].sounds = 0x66;
    unit_types[UNIT_HERO_BEASTMASTER].sounds = 0x66;
    unit_types[UNIT_HERO_BARD].sounds = 0x66;
    unit_types[UNIT_HERO_ORC_WARRIOR].sounds = 0x66;
    unit_types[UNIT_HERO_HEALER].sounds = 0x66;
    unit_types[UNIT_HERO_HUNTRESS].sounds = 0x66;
    unit_types[UNIT_HERO_THIEF].sounds = 0x66;
    unit_types[UNIT_HERO_DRUID].sounds = 0x67;
    unit_types[UNIT_HERO_WAR_MONK].sounds = 0x66;
    unit_types[UNIT_HERO_WARRIOR_MAGE].sounds = 0x66;
    unit_types[UNIT_HERO_MAGICIAN].sounds = 0x67;
    unit_types[UNIT_HERO_ASSASSIN].sounds = 0x66;
    unit_types[UNIT_HERO_WIND_MAGE].sounds = 0x67;
    unit_types[UNIT_HERO_RANGER].sounds = 0x66;
    unit_types[UNIT_HERO_DRACONIAN].sounds = 0x86;
    unit_types[UNIT_HERO_WITCH].sounds = 0x67;
    unit_types[UNIT_HERO_GOLDEN_ONE].sounds = 0x66;
    unit_types[UNIT_HERO_NINJA].sounds = 0x66;
    unit_types[UNIT_HERO_ROGUE].sounds = 0x66;
    unit_types[UNIT_HERO_AMAZON].sounds = 0x66;
    unit_types[UNIT_HERO_WARLOCK].sounds = 0x67;
    unit_types[UNIT_HERO_UNKNOWN].sounds = 0x66;
    unit_types[UNIT_HERO_ILLUSIONIST].sounds = 0x67;
    unit_types[UNIT_HERO_SWORDSMAN].sounds = 0x66;
    unit_types[UNIT_HERO_PRIESTESS].sounds = 0x67;
    unit_types[UNIT_HERO_PALADIN].sounds = 0x66;
    unit_types[UNIT_HERO_BLACK_KNIGHT].sounds = 0x66;
    unit_types[UNIT_HERO_ELVEN_ARCHER].sounds = 0x66;
    unit_types[UNIT_HERO_KNIGHT].sounds = 0x66;
    unit_types[UNIT_HERO_NECROMANCER].sounds = 0x67;
    unit_types[UNIT_HERO_CHAOS_WARRIOR].sounds = 0x66;
    unit_types[UNIT_HERO_CHOSEN].sounds = 0x66;
    unit_types[UNIT_TRIREME].sounds = 0x86;
    unit_types[UNIT_GALLEY].sounds = 0x86;
    unit_types[UNIT_CATAPULT].sounds = 0x86;
    unit_types[UNIT_WARSHIP].sounds = 0x86;
    unit_types[UNIT_BARBARIAN_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_BARBARIAN_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_BARBARIAN_BOWMEN].sounds = 0x56;
    unit_types[UNIT_BARBARIAN_CAVALRY].sounds = 0x66;
    unit_types[UNIT_BARBARIAN_SHAMAN].sounds = 0x97;
    unit_types[UNIT_BARBARIAN_SETTLERS].sounds = 0x96;
    unit_types[UNIT_BARBARIAN_BERSERKERS].sounds = 0x56;
    unit_types[UNIT_BEASTMEN_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_BEASTMEN_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_BEASTMEN_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_BEASTMEN_BOWMEN].sounds = 0x56;
    unit_types[UNIT_BEASTMEN_PRIESTS].sounds = 0x97;
    unit_types[UNIT_BEASTMEN_MAGICIANS].sounds = 0x97;
    unit_types[UNIT_BEASTMEN_ENGINEERS].sounds = 0x56;
    unit_types[UNIT_BEASTMEN_SETTLERS].sounds = 0x96;
    unit_types[UNIT_BEASTMEN_CENTAURS].sounds = 0x66;
    unit_types[UNIT_BEASTMEN_MANTICORES].sounds = 0x83;
    unit_types[UNIT_BEASTMEN_MINOTAURS].sounds = 0x55;
    unit_types[UNIT_DARK_ELF_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_DARK_ELF_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_DARK_ELF_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_DARK_ELF_CAVALRY].sounds = 0x66;
    unit_types[UNIT_DARK_ELF_PRIESTS].sounds = 0x97;
    unit_types[UNIT_DARK_ELF_SETTLERS].sounds = 0x96;
    unit_types[UNIT_DARK_ELF_NIGHTBLADES].sounds = 0x06;
    unit_types[UNIT_DARK_ELF_WARLOCKS].sounds = 0x97;
    unit_types[UNIT_DARK_ELF_NIGHTMARES].sounds = 0x86;
    unit_types[UNIT_DRACONIAN_SPEARMEN].sounds = 0x86;
    unit_types[UNIT_DRACONIAN_SWORDSMEN].sounds = 0x86;
    unit_types[UNIT_DRACONIAN_HALBERDIERS].sounds = 0x86;
    unit_types[UNIT_DRACONIAN_BOWMEN].sounds = 0x86;
    unit_types[UNIT_DRACONIAN_SHAMAN].sounds = 0x87;
    unit_types[UNIT_DRACONIAN_MAGICIANS].sounds = 0x87;
    unit_types[UNIT_DRACONIAN_ENGINEERS].sounds = 0x86;
    unit_types[UNIT_DRACONIAN_SETTLERS].sounds = 0x96;
    unit_types[UNIT_DRACONIAN_DOOM_DRAKES].sounds = 0x74;
    unit_types[UNIT_DRACONIAN_AIR_SHIP].sounds = 0x86;
    unit_types[UNIT_DWARF_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_DWARF_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_DWARF_ENGINEERS].sounds = 0x56;
    unit_types[UNIT_DWARF_HAMMERHANDS].sounds = 0x56;
    unit_types[UNIT_DWARF_STEAM_CANNON].sounds = 0x96;
    unit_types[UNIT_DWARF_GOLEM].sounds = 0xd6;
    unit_types[UNIT_DWARF_SETTLERS].sounds = 0x96;
    unit_types[UNIT_GNOLL_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_GNOLL_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_GNOLL_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_GNOLL_BOWMEN].sounds = 0x56;
    unit_types[UNIT_GNOLL_SETTLERS].sounds = 0x96;
    unit_types[UNIT_GNOLL_WOLF_RIDERS].sounds = 0x96;
    unit_types[UNIT_HALFLING_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_HALFLING_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_HALFLING_BOWMEN].sounds = 0x56;
    unit_types[UNIT_HALFLING_SHAMAN].sounds = 0x97;
    unit_types[UNIT_HALFLING_SETTLERS].sounds = 0x96;
    unit_types[UNIT_HALFLING_SLINGERS].sounds = 0x96;
    unit_types[UNIT_HIGH_ELF_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_ELF_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_ELF_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_HIGH_ELF_CAVALRY].sounds = 0x66;
    unit_types[UNIT_HIGH_ELF_MAGICIANS].sounds = 0x97;
    unit_types[UNIT_HIGH_ELF_SETTLERS].sounds = 0x96;
    unit_types[UNIT_HIGH_ELF_LONGBOWMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_ELF_ELVEN_LORDS].sounds = 0x66;
    unit_types[UNIT_HIGH_ELF_PEGASAI].sounds = 0x86;
    unit_types[UNIT_HIGH_MEN_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_MEN_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_MEN_BOWMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_MEN_CAVALRY].sounds = 0x66;
    unit_types[UNIT_HIGH_MEN_PRIESTS].sounds = 0x97;
    unit_types[UNIT_HIGH_MEN_MAGICIANS].sounds = 0x97;
    unit_types[UNIT_HIGH_MEN_ENGINEERS].sounds = 0x56;
    unit_types[UNIT_HIGH_MEN_SETTLERS].sounds = 0x96;
    unit_types[UNIT_HIGH_MEN_PIKEMEN].sounds = 0x56;
    unit_types[UNIT_HIGH_MEN_PALADINS].sounds = 0x66;
    unit_types[UNIT_KLACKON_SPEARMEN].sounds = 0x96;
    unit_types[UNIT_KLACKON_SWORDSMEN].sounds = 0x96;
    unit_types[UNIT_KLACKON_HALBERDIERS].sounds = 0x96;
    unit_types[UNIT_KLACKON_ENGINEERS].sounds = 0x96;
    unit_types[UNIT_KLACKON_SETTLERS].sounds = 0x96;
    unit_types[UNIT_KLACKON_STAG_BEETLE].sounds = 0x96;
    unit_types[UNIT_LIZARDMAN_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_LIZARDMAN_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_LIZARDMAN_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_LIZARDMAN_JAVELINEERS].sounds = 0x56;
    unit_types[UNIT_LIZARDMAN_SHAMAN].sounds = 0x97;
    unit_types[UNIT_LIZARDMAN_SETTLERS].sounds = 0x96;
    unit_types[UNIT_LIZARDMAN_DRAGON_TURTLE].sounds = 0xd6;
    unit_types[UNIT_NOMAD_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_NOMAD_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_NOMAD_BOWMEN].sounds = 0x56;
    unit_types[UNIT_NOMAD_PRIESTS].sounds = 0x97;
    unit_types[UNIT_NOMAD_MAGICIANS].sounds = 0x97;
    unit_types[UNIT_NOMAD_SETTLERS].sounds = 0x96;
    unit_types[UNIT_NOMAD_HORSEBOWMEN].sounds = 0x66;
    unit_types[UNIT_NOMAD_PIKEMEN].sounds = 0x56;
    unit_types[UNIT_NOMAD_RANGERS].sounds = 0x56;
    unit_types[UNIT_NOMAD_GRIFFINS].sounds = 0x84;
    unit_types[UNIT_ORC_SPEARMEN].sounds = 0x56;
    unit_types[UNIT_ORC_SWORDSMEN].sounds = 0x56;
    unit_types[UNIT_ORC_HALBERDIERS].sounds = 0x56;
    unit_types[UNIT_ORC_BOWMEN].sounds = 0x56;
    unit_types[UNIT_ORC_CAVALRY].sounds = 0x66;
    unit_types[UNIT_ORC_SHAMAN].sounds = 0x97;
    unit_types[UNIT_ORC_MAGICIANS].sounds = 0x97;
    unit_types[UNIT_ORC_ENGINEERS].sounds = 0x56;
    unit_types[UNIT_ORC_SETTLERS].sounds = 0x96;
    unit_types[UNIT_ORC_WYVERN_RIDERS].sounds = 0x74;
    unit_types[UNIT_TROLL_SPEARMEN].sounds = 0x55;
    unit_types[UNIT_TROLL_SWORDSMEN].sounds = 0x55;
    unit_types[UNIT_TROLL_HALBERDIERS].sounds = 0x55;
    unit_types[UNIT_TROLL_SHAMAN].sounds = 0x97;
    unit_types[UNIT_TROLL_SETTLERS].sounds = 0x95;
    unit_types[UNIT_TROLL_WAR_TROLLS].sounds = 0x55;
    unit_types[UNIT_TROLL_WAR_MAMMOTHS].sounds = 0xa2;
    unit_types[UNIT_MAGIC_SPIRIT].sounds = 0x93;
    unit_types[UNIT_HELL_HOUNDS].sounds = 0x93;
    unit_types[UNIT_GARGOYLES].sounds = 0x82;
    unit_types[UNIT_FIRE_GIANT].sounds = 0xd5;
    unit_types[UNIT_FIRE_ELEMENTAL].sounds = 0x83;
    unit_types[UNIT_CHAOS_SPAWN].sounds = 0x82;
    unit_types[UNIT_CHIMERA].sounds = 0x82;
    unit_types[UNIT_DOOM_BAT].sounds = 0x82;
    unit_types[UNIT_EFREET].sounds = 0x86;
    unit_types[UNIT_HYDRA].sounds = 0xa1;
    unit_types[UNIT_GREAT_DRAKE].sounds = 0x70;
    unit_types[UNIT_SKELETONS].sounds = 0x96;
    unit_types[UNIT_GHOULS].sounds = 0x96;
    unit_types[UNIT_NIGHT_STALKER].sounds = 0x93;
    unit_types[UNIT_WEREWOLVES].sounds = 0x93;
    unit_types[UNIT_DEMON].sounds = 0x82;
    unit_types[UNIT_WRAITHS].sounds = 0x82;
    unit_types[UNIT_SHADOW_DEMONS].sounds = 0x82;
    unit_types[UNIT_DEATH_KNIGHTS].sounds = 0x66;
    unit_types[UNIT_DEMON_LORD].sounds = 0x71;
    unit_types[UNIT_ZOMBIES].sounds = 0x96;
    unit_types[UNIT_UNICORNS].sounds = 0xb3;
    unit_types[UNIT_GUARDIAN_SPIRIT].sounds = 0x83;
    unit_types[UNIT_ANGEL].sounds = 0x86;
    unit_types[UNIT_ARCH_ANGEL].sounds = 0x86;
    unit_types[UNIT_WAR_BEARS].sounds = 0x93;
    unit_types[UNIT_SPRITES].sounds = 0x87;
    unit_types[UNIT_COCKATRICES].sounds = 0x87;
    unit_types[UNIT_BASILISK].sounds = 0xd2;
    unit_types[UNIT_GIANT_SPIDERS].sounds = 0x93;
    unit_types[UNIT_STONE_GIANT].sounds = 0xd5;
    unit_types[UNIT_COLOSSUS].sounds = 0xa5;
    unit_types[UNIT_GORGONS].sounds = 0x72;
    unit_types[UNIT_EARTH_ELEMENTAL].sounds = 0xa1;
    unit_types[UNIT_BEHEMOTH].sounds = 0xa1;
    unit_types[UNIT_GREAT_WYRM].sounds = 0xc0;
    unit_types[UNIT_FLOATING_ISLAND].sounds = 0x08;
    unit_types[UNIT_PHANTOM_BEAST].sounds = 0x08;
    unit_types[UNIT_PHANTOM_WARRIORS].sounds = 0x08;
    unit_types[UNIT_STORM_GIANT].sounds = 0xa5;
    unit_types[UNIT_AIR_ELEMENTAL].sounds = 0x82;
    unit_types[UNIT_DJINN].sounds = 0xb6;
    unit_types[UNIT_SKY_DRAKE].sounds = 0x70;
    unit_types[UNIT_NAGAS].sounds = 0x92;
}

/* (city) a deleted city: notices of later cities move down one index; the city's own notice is removed
   (QUIRK #336: only the city field of the later notices is shifted, and the entry moved into slot i is not
   re-checked)
   Numbered: BUG #336 (BUGS_AND_QUIRKS.txt) */
void City_Notices_Remove_City(int city)
{
    int i;
    int j;

    for (i = 0; i < city_notice_count; i++) {
        if (city_notices[i].city > city)
            city_notices[i].city = city_notices[i].city - 1;
        else if (city_notices[i].city == city) {
            for (j = i; j < city_notice_count; j++)
                city_notices[j].city = city_notices[j + 1].city;
            city_notice_count = city_notice_count - 1;
        }
    }
}
