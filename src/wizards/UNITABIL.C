/* UNITABIL.C - WIZARDS.EXE overlay code segment 679a (stub 248d): the unit view's ability lists - a unit's (rank and
   experience, a hero's item slots, item-granted powers, hero abilities with their bonuses, the caster line, ammunition,
   to-hit, construction, scouting, movement abilities, the ability / attack / ranged-special / curse / mutation /
   enchantment names), a unit type's (the production preview), and an item's power lines.
   This module's data (DS:3a50..49e5): the ability name tables (hero abilities, abilities 1-3, attack flags,
   mutations, ranged specials, enchantments, combat curses, item enchantments, unit / hero rank names, item powers) and
   the literal pool ("" is the tail of "Large Shield", "First Strike" of "Negate First Strike", "Spell" of "Healing
   Spell", "s" of "Holy Bonus", "Undead" of "Create Undead", ")" of " ep)"); no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 679a src/wizards/UNITABIL.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void UnitView_Build_Abilities(int unit, struct UnitAbilityList *list, int *count);
void UnitView_Build_Type_Abilities(int unit_type, struct UnitAbilityList *list, int *count);
void UnitView_Item_Power_Texts(int item, char names[][20], char descs[][40], int *count);

#include "GAME.H"

#define NUM_ABILITY_FLAG_NAMES 15       /* the name tables' lengths */
#define NUM_ABILITY_NAMES 10
#define NUM_ABILITY2_NAMES 8
#define NUM_ATTACK_FLAG_NAMES 11
#define NUM_MUTATION_NAMES 3
#define NUM_RANGED_SPECIALS 6
#define NUM_ENCHANT_NAMES 32
#define NUM_COMBAT_CURSE_NAMES 15
#define NUM_ITEM_ENCHANT_NAMES 21
#define ABIL_LIST_MAX_ROWS 39           /* rows 0 .. 38 of the 40 a UnitAbilityList holds */

extern unsigned item_icon_pics[];               /* DS:913e  ITEMS.LBX 0..115 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern unsigned unitview_slot_icons[];          /* DS:975e  by HiredHero.slot_type - 1 */
extern unsigned item_power_icons[];             /* DS:976e  ITEMISC 0..18 */
extern unsigned special_icons[];                /* DS:97f8  SPECIAL.LBX */
extern unsigned special2_icons[];               /* DS:98d6  SPECIAL2.LBX */

void far_strcpy(char *dst, int dst_seg, char far *src);

/* DS:3a50  the source of the HERO_ABILITY_* names (tools/make_enums.py: each row's HELP.LBX title) */
struct HeroAbility hero_ability_table[23] = {
    {0x1L, 43, 0, 0x203},
    {0x2L, 43, 0, 0x203},
    {0x8L, 45, 0, 0x204},
    {0x10L, 45, 0, 0x204},
    {0x40L, 47, 0, 0x205},
    {0x80L, 47, 0, 0x205},
    {0x200L, 46, 0, 0x206},
    {0x400L, 46, 0, 0x206},
    {0x1000L, 50, 0, 0x207},
    {0x2000L, 50, 0, 0x207},
    {0x8000L, 52, 0, 0x208},
    {0x10000L, 52, 0, 0x208},
    {0x40000L, 54, 0, 0x209},
    {0x80000L, 54, 0, 0x209},
    {0x200000L, 61, 0, 0x20a},
    {0x400000L, 61, 0, 0x20a},
    {0x1000000L, 57, 0, 0x20b},
    {0x2000000L, 57, 0, 0x20b},
    {0x80000000L, 143, 0, 0x273},
    {0x4000000L, 143, 0, 0x273},
    {0x8000000L, 58, 0, 0x20c},
    {0x10000000L, 59, 0, 0x20d},
    {0x20000000L, 60, 0, 0x20e}
};

/* DS:3b08  BattleUnit.ability_flags / UnitType.ability_flags: the source of the AFLAG_* names (tools/make_enums.py) */
struct AbilityName ability_flag_name_table[15] = {
    {"Large Shield", 0x2, 14, 0x18c},
    {"Plane Shift", 0x4, 4, 0x18d},
    {"Wall Crusher", 0x8, 15, 0x18e},
    {"Healer", 0x10, 16, 0x18f},
    {"Create Outpost", 0x20, 17, 0x190},
    {"Invisibility", 0x40, 18, 0x191},
    {"Create Undead", 0x80, 19, 0x192},
    {"Long Range", 0x100, 129, 0x193},
    {"Land Corruption", 0x200, 21, 0x194},
    {"Meld With Node", 0x400, 40, 0x195},
    {"Non Corporeal", 0x800, 22, 0x196},
    {"Wind Walking", 0x1000, 23, 0x197},
    {"Regeneration", 0x2000, 24, 0x198},
    {"Purify", 0x4000, 25, 0x199},
    {"Negate First Strike", 0x8000, 48, 0x19a}
};

/* DS:3b80  UnitType.abilities: the source of the ABILITY_* names (tools/make_enums.py) */
struct AbilityName ability_name_table[10] = {
    {"Lucky", 0x400, 127, 0x19b},
    {"Poison Immunity", 0x80, 5, 0x19c},
    {"Fire Immunity", 0x1, 6, 0x19d},
    {"Stoning Immunity", 0x2, 7, 0x19e},
    {"Weapon Immunity", 0x100, 8, 0x19f},
    {"Missiles Immunity", 0x4, 9, 0x1a0},
    {"Illusions Immunity", 0x8, 10, 0x1a1},
    {"Cold Immunity", 0x10, 11, 0x1a2},
    {"Magic Immunity", 0x20, 12, 0x1a3},
    {"Death Immunity", 0x40, 49, 0x1a4}
};

/* DS:3bd0  abilities2: the source of the ABILITY2_* names (tools/make_enums.py) */
struct AbilityName ability2_name_table[8] = {
    {"Healing Spell", 0x1, 38, 0x1a5},
    {"Fire Ball Spell", 0x2, 39, 0x1a6},
    {"Doombolt Spell", 0x4, 41, 0x1a7},
    {"Immolation", 0x8, 32, 0x1a8},
    {"Cause Fear Spell", 0x20, 132, 0x1a9},
    {"Web Spell", 0x10, 131, 0x1aa},
    {"Resistance To All", 0x40, 33, 0x1ab},
    {"Holy Bonus", 0x80, 34, 0x1ac}
};

/* DS:3c10  attack_flags: the source of the ATTACK_* names (tools/make_enums.py) */
struct AbilityName attack_flag_name_table[11] = {
    {"Armor Piercing", 0x1, 28, 0x1ad},
    {"First Strike", 0x2, 29, 0x1ae},
    {"Poison", 0x4, 30, 0x1af},
    {"Life Steal", 0x8, 31, 0x1b0},
    {"Automatic Damage", 0x10, 140, 0x1b1},
    {"Destruction", 0x20, 116, 0x1b2},
    {"Illusion", 0x40, 35, 0x1b3},
    {"Stoning Touch", 0x80, 27, 0x1b4},
    {"Death Touch", 0x200, 141, 0x1b5},
    {"Power Drain", 0x400, 142, 0x1b6},
    {"Dispel Evil", 0x800, 133, 0x1b7}
};

/* DS:3c68 */
struct MutationName mutation_name_table[3] = {
    {"Chaos Channel", MUT_CHAOS_DEMON_SKIN, 62, 0x1b8},
    {"Chaos Channel", MUT_CHAOS_DEMON_WINGS, 63, 0x1b9},
    {"Chaos Channel", MUT_CHAOS_FIRE_BREATH, 64, 0x1ba}
};

/* DS:3c7a */
struct RangedSpecialName ranged_special_table[6] = {
    {"Thrown", RANGED_THROWN, 130, 0x1bb},
    {"Fire Breath", RANGED_FIRE_BREATH, 138, 0x1bc},
    {"Lightning", RANGED_LIGHTNING, 137, 0x1bd},
    {"Stoning Gaze", RANGED_STONING_GAZE, 26, 0x1be},
    {"Doom Gaze", RANGED_DOOM_GAZE, 134, 0x1bf},
    {"Death Gaze", RANGED_DEATH_GAZE, 135, 0x1c0}
};

/* DS:3c9e */
struct EnchantName enchant_name_table[32] = {
    {"Immolation", UNIT_ENCH_IMMOLATION, 117, 0x1c1},
    {"Guardian Wind", UNIT_ENCH_GUARDIAN_WIND, 118, 0x1c2},
    {"Berserk", UNIT_ENCH_BERSERK, 128, 0x1c3},
    {"Cloak Of Fear", UNIT_ENCH_CLOAK_OF_FEAR, 97, 0x1c4},
    {"Black Channels", UNIT_ENCH_BLACK_CHANNELS, 67, 0x1c5},
    {"Wraith Form", UNIT_ENCH_WRAITH_FORM, 68, 0x1c6},
    {"Regenerate", UNIT_ENCH_REGENERATION, 69, 0x1c7},
    {"Path Finding", UNIT_ENCH_PATH_FINDING, 70, 0x1c8},
    {"Water Walking", UNIT_ENCH_WATER_WALKING, 71, 0x1c9},
    {"Elemental Armor", UNIT_ENCH_ELEMENTAL_ARMOR, 73, 0x1ca},
    {"Resist Elements", UNIT_ENCH_RESIST_ELEMENTS, 72, 0x1cb},
    {"Stone Skin", UNIT_ENCH_STONE_SKIN, 74, 0x1cc},
    {"Iron Skin", UNIT_ENCH_IRON_SKIN, 75, 0x1cd},
    {"Endurance", UNIT_ENCH_ENDURANCE, 76, 0x1ce},
    {"Spell Lock", UNIT_ENCH_SPELL_LOCK, 119, 0x1cf},
    {"Invisibility", UNIT_ENCH_INVISIBILITY, 78, 0x1d0},
    {"Wind Walking", UNIT_ENCH_WIND_WALKING, 79, 0x1d1},
    {"Flight", UNIT_ENCH_FLIGHT, 80, 0x1d2},
    {"Resist Magic", UNIT_ENCH_RESIST_MAGIC, 81, 0x1d3},
    {"Magic Immunity", UNIT_ENCH_MAGIC_IMMUNITY, 82, 0x1d4},
    {"Flame Blade", UNIT_ENCH_FLAME_BLADE, 83, 0x1d5},
    {"Eldritch Weapon", UNIT_ENCH_ELDRITCH_WEAPON, 84, 0x1d6},
    {"True Sight", UNIT_ENCH_TRUE_SIGHT, 85, 0x1d7},
    {"Holy Weapon", UNIT_ENCH_HOLY_WEAPON, 86, 0x1d8},
    {"Heroism", UNIT_ENCH_HEROISM, 87, 0x1d9},
    {"Bless", UNIT_ENCH_BLESS, 88, 0x1da},
    {"Lion Heart", UNIT_ENCH_LIONHEART, 89, 0x1db},
    {"Giant Strength", UNIT_ENCH_GIANT_STRENGTH, 65, 0x1dc},
    {"Planar Travel", UNIT_ENCH_PLANAR_TRAVEL, 91, 0x1dd},
    {"Holy Armor", UNIT_ENCH_HOLY_ARMOR, 92, 0x1de},
    {"Righteousness", UNIT_ENCH_RIGHTEOUSNESS, 93, 0x1df},
    {"Invulnerability", UNIT_ENCH_INVULNERABILITY, 94, 0x1e0}
};

/* DS:3dde */
struct EnchantName combat_curse_name_table[15] = {
    {"Vertigo", COMBAT_FLAG_VERTIGO, 101, 0x1e1},
    {"Confusion", COMBAT_FLAG_CONFUSION, 111, 0x1e2},
    {"Whirlwind", COMBAT_FLAG_WHIRLWIND, 112, 0x1e3},
    {"Mind Storm", COMBAT_FLAG_MIND_STORM, 113, 0x1e4},
    {"Shatter", COMBAT_FLAG_SHATTER, 95, 0x1e5},
    {"Weakness", COMBAT_FLAG_WEAKNESS, 96, 0x1e6},
    {"Black Sleep", COMBAT_FLAG_BLACK_SLEEP, 114, 0x1e7},
    {"Warp Creature", COMBAT_FLAG_WARP_MELEE, 122, 0x1e8},
    {"Warp Creature", COMBAT_FLAG_WARP_DEFENSE, 123, 0x1e9},
    {"Warp Creature", COMBAT_FLAG_WARP_RESIST, 124, 0x1ea},
    {"Mind Twist", COMBAT_FLAG_MIND_TWIST, 121, 0x1eb},
    {"Haste", COMBAT_FLAG_HASTE, 77, 0x1ec},
    {"Web", COMBAT_FLAG_WEB, 99, 0x1ed},
    {"Creature Binding", COMBAT_FLAG_CREATURE_BINDING, 100, 0x1ee},
    {"Posession", COMBAT_FLAG_POSSESSION, 98, 0x1ef}
};

/* the masks are UNIT_ENCH_* bits of BattleUnit.item_enchantments (Hero_Item_Enchants): "Haste" 0x4000 is
   UNIT_ENCH_SPELL_LOCK and "Merging" 0x20000000 UNIT_ENCH_HOLY_ARMOR, bits no item sets, so those rows never show */
/* DS:3e74 */
struct EnchantName item_enchant_name_table[21] = {
    {"Magic Immunity", UNIT_ENCH_MAGIC_IMMUNITY, 0, 0x22c},
    {"Regenerate", UNIT_ENCH_REGENERATION, 1, 0x22d},
    {"Haste", UNIT_ENCH_SPELL_LOCK, 2, 0x22e},
    {"True Sight", UNIT_ENCH_TRUE_SIGHT, 3, 0x22f},
    {"Path Finding", UNIT_ENCH_PATH_FINDING, 4, 0x230},
    {"Water Walking", UNIT_ENCH_WATER_WALKING, 5, 0x231},
    {"Bless", UNIT_ENCH_BLESS, 6, 0x232},
    {"Lion Heart", UNIT_ENCH_LIONHEART, 7, 0x233},
    {"Invisibility", UNIT_ENCH_INVISIBILITY, 8, 0x234},
    {"Planar Travel", UNIT_ENCH_PLANAR_TRAVEL, 9, 0x235},
    {"Endurance", UNIT_ENCH_ENDURANCE, 10, 0x236},
    {"Righteousness", UNIT_ENCH_RIGHTEOUSNESS, 11, 0x237},
    {"Invulnerability", UNIT_ENCH_INVULNERABILITY, 12, 0x238},
    {"Resist Elements", UNIT_ENCH_RESIST_ELEMENTS, 13, 0x239},
    {"Elemental Armor", UNIT_ENCH_ELEMENTAL_ARMOR, 14, 0x23a},
    {"Cloak Of Fear", UNIT_ENCH_CLOAK_OF_FEAR, 15, 0x23b},
    {"Flight", UNIT_ENCH_FLIGHT, 16, 0x23c},
    {"Resist Magic", UNIT_ENCH_RESIST_MAGIC, 17, 0x23d},
    {"Guardian Wind", UNIT_ENCH_GUARDIAN_WIND, 21, 0x23e},
    {"Giant Strength", UNIT_ENCH_GIANT_STRENGTH, 20, 0x23f},
    {"Merging", UNIT_ENCH_HOLY_ARMOR, 18, 0x240}
};

/* DS:3f46 */
char *unit_level_names[6] = {
    "Recruit",
    "Regular",
    "Veteran",
    "Elite",
    "Ultra Elite",
    "Champion"
};

/* DS:3f52 */
char *hero_level_names[9] = {
    "Hero",
    "Myrmidon",
    "Captain",
    "Commander",
    "Champion",
    "Lord",
    "Grand Lord",
    "Super Hero",
    "Demi-God"
};

/* (unit, list, &count) the unit view's ability lines of a unit (at most 39 in the table loops): its rank and
   experience (normal units and heroes, not Undead), a hero's three item slots (an empty one shows the slot type),
   the item-granted powers, the hero abilities with their level bonus ('}' marks the super ability; Sage shows its
   research points, Lucky / Charmed / Noble no bonus), a hero caster's spell points, the ammunition (none for a
   caster), to-hit, Construction, Scouting, the movement abilities, the ability tables, Summon Demons, a caster
   unit's spell points, the attack flags (with the poison / stoning strength), the ranged specials (Doom Gaze also
   lists Death Gaze and Stoning Gaze), the combat curses, the Chaos Channels, Undead, Stasis and the enchantments.
   Reads p_unit_stats_buf (ranged type, ammunition, to-hit, movement, abilities, spell points) without building it:
   the caller fills it (Battle_Unit_Init). BUG #87: the ammunition test reads p_battle_units by unit TYPE; a hero ability line would be counted even when the switch
   matched no case (latent: every one of the 23 single-bit masks has a case)
   Numbered: BUG #87 (BUGS_AND_QUIRKS.txt) */
void UnitView_Build_Abilities(int unit, struct UnitAbilityList *list, int *count)
{
    int bonus;
    int k;
    int player;
    int hero_slot;
    int item;
    long ench;
    long bits;
    int utype;
    unsigned char mutations;
    int ranged_class;
    char num_text[20];
    char roman[5][5];
    int n;

    strcpy(roman[0], "");
    strcpy(roman[1], "I");
    strcpy(roman[2], "II");
    strcpy(roman[3], "III");
    strcpy(roman[4], "IV");
    utype = p_units[unit].type;
    *count = 0;
    n = 0;
    if (p_units[unit].level >= 0 && utype < UNIT_MAGIC_SPIRIT && (p_units[unit].mutation_flags & MUT_UNDEAD) == 0) {
        if (p_units[unit].hero_slot != -1)
            strcpy(list->text[n], hero_level_names[p_units[unit].level]);
        else
            strcpy(list->text[n], unit_level_names[p_units[unit].level]);
        strcat(list->text[n], " (");
        itoa(p_units[unit].experience, num_text, 10);
        strcat(list->text[n], num_text);
        strcat(list->text[n], " ep)");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[p_units[unit].level + SPECIAL_ICON_RANK_BASE];
        if (p_units[unit].hero_slot != -1)
            list->help[n] = p_units[unit].level + HELP_HERO_RANK_BASE;
        else
            list->help[n] = p_units[unit].level + HELP_UNIT_RANK_BASE;
        n++;
    }
    if (p_units[unit].hero_slot != -1) {
        player = p_units[unit].owner;
        hero_slot = p_units[unit].hero_slot;
        for (k = 0; k < 3; k++) {
            item = wizards[player].hired_heroes[hero_slot].item_index[k];
            if (item > -1) {
                far_strcpy(list->text[n], 0, p_items[item].name);
                list->ench_mask[n] = ABIL_ROW_ITEM;
                list->icon[n] = item_icon_pics[p_items[item].icon];
                list->help[n] = item;
            } else {
                strcpy(list->text[n], "");
                list->ench_mask[n] = ABIL_ROW_ITEM;
                list->icon[n] = unitview_slot_icons[wizards[player].hired_heroes[hero_slot].slot_type[k] - 1];
                list->help[n] = -(wizards[player].hired_heroes[hero_slot].slot_type[k] + HELP_UNIT_VIEW_SLOT);   /* negative: a slot-type help id */
            }
            n++;
        }
        for (k = 0; k < NUM_ITEM_ENCHANT_NAMES; k++) {
            if (p_unit_stats_buf->item_enchantments & item_enchant_name_table[k].mask) {
                strcpy(list->text[n], item_enchant_name_table[k].name);
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = item_power_icons[item_enchant_name_table[k].icon];
                list->help[n] = item_enchant_name_table[k].help;
                n++;
            }
        }
        for (k = 0; k < NUM_HERO_ABILITY_ROWS; k++) {
            bits = p_hero_stats[player][utype].abilities & hero_ability_table[k].mask;
            if (bits != 0) {
                switch (bits) {
                case HERO_ABILITY_LEADERSHIP:
                    strcpy(list->text[n], "Leadership");
                    bonus = (p_units[unit].level + 1) / 3;
                    break;
                case HERO_ABILITY_SUPER_LEADERSHIP:
                    strcpy(list->text[n], "Leadership");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) / 2;
                    break;
                case HERO_ABILITY_LEGENDARY:
                    strcpy(list->text[n], "Legendary");
                    bonus = (p_units[unit].level + 1) * 3;
                    break;
                case HERO_ABILITY_SUPER_LEGENDARY:
                    strcpy(list->text[n], "Legendary");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 9 / 2;
                    break;
                case HERO_ABILITY_BLADEMASTER:
                    strcpy(list->text[n], "Blademaster");
                    bonus = (p_units[unit].level + 1) / 2;
                    break;
                case HERO_ABILITY_SUPER_BLADEMASTER:
                    strcpy(list->text[n], "Blademaster");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 4;
                    break;
                case HERO_ABILITY_ARMSMASTER:
                    strcpy(list->text[n], "Armsmaster");
                    bonus = (p_units[unit].level + 1) * 2;
                    break;
                case HERO_ABILITY_SUPER_ARMSMASTER:
                    strcpy(list->text[n], "Armsmaster");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3;
                    break;
                case HERO_ABILITY_CONSTITUTION:
                    strcpy(list->text[n], "Constitution");
                    bonus = p_units[unit].level + 1;
                    break;
                case HERO_ABILITY_SUPER_CONSTITUTION:
                    strcpy(list->text[n], "Constitution");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 2;
                    break;
                case HERO_ABILITY_MIGHT:
                    strcpy(list->text[n], "Might");
                    bonus = p_units[unit].level + 1;
                    break;
                case HERO_ABILITY_SUPER_MIGHT:
                    strcpy(list->text[n], "Might");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 2;
                    break;
                case HERO_ABILITY_ARCANE_POWER:
                    strcpy(list->text[n], "Arcane Power");
                    bonus = p_units[unit].level + 1;
                    break;
                case HERO_ABILITY_SUPER_ARCANE_POWER:
                    strcpy(list->text[n], "Arcane Power");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 2;
                    break;
                case HERO_ABILITY_SAGE:
                    strcpy(list->text[n], "Sage");
                    strcat(list->text[n], " ");
                    bonus = (p_units[unit].level + 1) * 3;
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], num_text);
                    strcat(list->text[n], " rp");
                    bonus = -1;
                    break;
                case HERO_ABILITY_SUPER_SAGE:
                    strcpy(list->text[n], "Sage");
                    strcat(list->text[n], "}");
                    strcat(list->text[n], " ");
                    bonus = (p_units[unit].level + 1) * 9 / 2;
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], num_text);
                    strcat(list->text[n], " rp");
                    bonus = -1;
                    break;
                case HERO_ABILITY_PRAYERMASTER:
                    strcpy(list->text[n], "Prayermaster");
                    bonus = p_units[unit].level + 1;
                    break;
                case HERO_ABILITY_SUPER_PRAYERMASTER:
                    strcpy(list->text[n], "Prayermaster");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 2;
                    break;
                case HERO_ABILITY_AGILE:
                    strcpy(list->text[n], "Agility");
                    bonus = p_units[unit].level + 1;
                    break;
                case HERO_ABILITY_SUPER_AGILE:
                    strcpy(list->text[n], "Agility");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 2;
                    break;
                case HERO_ABILITY_LUCKY:
                    strcpy(list->text[n], "Lucky");
                    bonus = -1;
                    break;
                case HERO_ABILITY_CHARMED:
                    strcpy(list->text[n], "Charmed");
                    bonus = -1;
                    break;
                case HERO_ABILITY_NOBLE:
                    strcpy(list->text[n], "Noble");
                    bonus = -1;
                    break;
                }
                if (p_units[unit].level >= 0 && bonus > 0) {
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], " +");
                    strcat(list->text[n], num_text);
                }
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = special_icons[hero_ability_table[k].icon];
                list->help[n] = hero_ability_table[k].help;
                n++;
            }
        }
        if (p_hero_stats[player][utype].caster_level > 0) {
            strcpy(list->text[n], "Caster");
            if (p_units[unit].level >= 0) {
                strcat(list->text[n], " ");
                if (p_unit_stats_buf->spell_skill > p_unit_stats_buf->spell_points) {
                    bonus = p_unit_stats_buf->spell_points;
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], num_text);
                    strcat(list->text[n], " (");
                    bonus = p_unit_stats_buf->spell_skill;
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], num_text);
                    strcat(list->text[n], ")");
                } else {
                    bonus = p_unit_stats_buf->spell_skill;
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], num_text);
                }
                strcat(list->text[n], " mp");
            }
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[SPECIAL_ICON_CASTER];
            list->help[n] = HELP_CASTER;
            n++;
        }
    }
    ranged_class = p_unit_stats_buf->ranged_type / 10;
    if (ranged_class == RANGED_CLASS_BOULDER || ranged_class == RANGED_CLASS_MISSILE || ranged_class == RANGED_CLASS_MAGIC) {
        if ((ranged_class == RANGED_CLASS_MAGIC && (p_battle_units[utype].abilities & ABILITY_CASTER) != 0) ||
            (p_units[unit].hero_slot != -1 && p_hero_stats[p_units[unit].owner][utype].caster_level > 0)) {
        } else {
            if (ranged_class == RANGED_CLASS_BOULDER)
                strcpy(list->text[n], "Rock");
            if (ranged_class == RANGED_CLASS_MISSILE) {
                if (p_unit_stats_buf->ranged_type == RANGED_ARROW)
                    strcpy(list->text[n], "Arrow");
                else
                    strcpy(list->text[n], "Bullet");
            }
            if (ranged_class == RANGED_CLASS_MAGIC)
                strcpy(list->text[n], "Spell");
            bonus = p_unit_stats_buf->ammo;
            itoa(bonus, num_text, 10);
            if (bonus > 1)
                strcat(list->text[n], "s");
            strcat(list->text[n], " x");
            strcat(list->text[n], num_text);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special2_icons[SPECIAL2_ICON_AMMO];
            list->help[n] = HELP_AMMO;
            n++;
        }
    }
    if (p_unit_stats_buf->to_hit != 0) {
        bonus = p_unit_stats_buf->to_hit;
        itoa(bonus, num_text, 10);
        if (bonus > 0) {
            strcpy(list->text[n], "+");
            list->icon[n] = special2_icons[SPECIAL2_ICON_TO_HIT];
            list->help[n] = HELP_TO_HIT;
        } else {
            strcpy(list->text[n], "");
            list->icon[n] = special2_icons[SPECIAL2_ICON_TO_HIT_MINUS];
            list->help[n] = HELP_TO_HIT_MINUS;
        }
        strcat(list->text[n], num_text);
        strcat(list->text[n], " To Hit");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        n++;
    }
    if (p_unit_stats_buf->construction > 0) {
        strcpy(list->text[n], "Construction");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_CONSTRUCTION];
        list->help[n] = HELP_CONSTRUCTION;
        n++;
    }
    if (p_unit_stats_buf->scouting > 1) {
        strcpy(list->text[n], "Scouting");
        bonus = p_unit_stats_buf->scouting;
        strcpy(num_text, roman[bonus]);
        strcat(list->text[n], " ");
        strcat(list->text[n], num_text);
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_SCOUTING];
        list->help[n] = HELP_SCOUTING;
        n++;
    }
    if (p_unit_stats_buf->move_flags & MOVE_MERGING) {
        strcpy(list->text[n], "Merging");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = item_power_icons[18];   /* the Merging icon (item_enchant_name_table's Merging row) */
        list->help[n] = HELP_MERGING;
        n++;
    }
    if (p_unit_stats_buf->move_flags & MOVE_TELEPORTING) {
        strcpy(list->text[n], "Teleporting");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_TELEPORTING];
        list->help[n] = HELP_TELEPORTING;
        n++;
    }
    if ((p_unit_stats_buf->move_flags & MOVE_FORESTER) && (p_unit_stats_buf->move_flags & MOVE_MOUNTAINEER)) {
        strcpy(list->text[n], "Pathfinding");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_PATHFINDING];
        list->help[n] = HELP_PATHFINDING;
        n++;
    } else {
        if (p_unit_stats_buf->move_flags & MOVE_MOUNTAINEER) {
            strcpy(list->text[n], "Mountaineer");
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[SPECIAL_ICON_MOUNTAINEER];
            list->help[n] = HELP_MOUNTAINEER;
            n++;
        }
        if (p_unit_stats_buf->move_flags & MOVE_FORESTER) {
            strcpy(list->text[n], "Forester");
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[SPECIAL_ICON_FORESTER];
            list->help[n] = HELP_FORESTER;
            n++;
        }
    }
    for (k = 0; k < NUM_ABILITY_FLAG_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (p_unit_stats_buf->ability_flags & ability_flag_name_table[k].mask) {
            strcpy(list->text[n], ability_flag_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ability_flag_name_table[k].icon];
            list->help[n] = ability_flag_name_table[k].help;
            n++;
        }
    }
    if (unit_types[utype].abilities & ABILITY_SUMMON_DEMONS) {   /* the unit TYPE's count: in combat the line
                                                   keeps the full count after summons are used; clarified using ReMoM
                                                   repo */
        strcpy(list->text[n], "Summon Demons");
        bonus = 0;
        if (unit_types[utype].abilities & ABILITY_SUMMON_DEMONS_2)
            bonus = 2;
        if (unit_types[utype].abilities & ABILITY_SUMMON_DEMONS_1)
            bonus++;
        itoa(bonus, num_text, 10);
        strcat(list->text[n], " ");
        strcat(list->text[n], num_text);
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special2_icons[SPECIAL2_ICON_SUMMON_DEMONS];
        list->help[n] = HELP_SUMMON_DEMONS;
        n++;
    }
    if (unit_types[utype].abilities & ABILITY_CASTER) {
        strcpy(list->text[n], "Caster");
        strcat(list->text[n], " ");
        if (p_unit_stats_buf->spell_skill > p_unit_stats_buf->spell_points) {
            bonus = p_unit_stats_buf->spell_points;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], num_text);
            strcat(list->text[n], " (");
            bonus = p_unit_stats_buf->spell_skill;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], num_text);
            strcat(list->text[n], ")");
        } else {
            bonus = p_unit_stats_buf->spell_skill;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], num_text);
        }
        strcat(list->text[n], " mp");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_CASTER];
        list->help[n] = HELP_CASTER;
        n++;
    }
    for (k = 0; k < NUM_ABILITY_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (unit_types[utype].abilities & ability_name_table[k].mask) {
            strcpy(list->text[n], ability_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ability_name_table[k].icon];
            list->help[n] = ability_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_ABILITY2_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (p_unit_stats_buf->abilities2 & ability2_name_table[k].mask) {
            strcpy(list->text[n], ability2_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ability2_name_table[k].icon];
            list->help[n] = ability2_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_ATTACK_FLAG_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (p_unit_stats_buf->attack_flags & attack_flag_name_table[k].mask) {
            strcpy(list->text[n], attack_flag_name_table[k].name);
            if (attack_flag_name_table[k].mask == ATTACK_POISON) {
                bonus = p_unit_stats_buf->touch_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
            }
            if (attack_flag_name_table[k].mask == ATTACK_STONING_TOUCH) {
                bonus = p_unit_stats_buf->special_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
            }
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[attack_flag_name_table[k].icon];
            list->help[n] = attack_flag_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_RANGED_SPECIALS && n < ABIL_LIST_MAX_ROWS; k++) {
        if (p_unit_stats_buf->ranged_type == ranged_special_table[k].key) {
            strcpy(list->text[n], ranged_special_table[k].name);
            bonus = p_unit_stats_buf->ranged;
            if (ranged_special_table[k].key == RANGED_STONING_GAZE || ranged_special_table[k].key == RANGED_DEATH_GAZE)
                bonus = p_unit_stats_buf->special_strength;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], " ");
            strcat(list->text[n], num_text);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ranged_special_table[k].icon];
            list->help[n] = ranged_special_table[k].help;
            n++;
            if (ranged_special_table[k].key == RANGED_DOOM_GAZE) {
                strcpy(list->text[n], ranged_special_table[k + 1].name);
                bonus = p_unit_stats_buf->special_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = special_icons[ranged_special_table[k + 1].icon];
                list->help[n] = ranged_special_table[k + 1].help;
                n++;
                strcpy(list->text[n], ranged_special_table[k - 1].name);
                bonus = p_unit_stats_buf->special_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = special_icons[ranged_special_table[k - 1].icon];
                list->help[n] = ranged_special_table[k - 1].help;
                n++;
            }
        }
    }
    for (k = 0; k < NUM_COMBAT_CURSE_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (p_unit_stats_buf->combat_flags & combat_curse_name_table[k].mask) {
            strcpy(list->text[n], combat_curse_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[combat_curse_name_table[k].icon];
            list->help[n] = combat_curse_name_table[k].help;
            n++;
        }
    }
    mutations = p_units[unit].mutation_flags;
    for (k = 0; k < NUM_MUTATION_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (mutation_name_table[k].mask & mutations) {
            strcpy(list->text[n], mutation_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[mutation_name_table[k].icon];
            list->help[n] = mutation_name_table[k].help;
            n++;
        }
    }
    mutations = p_units[unit].mutation_flags;
    if (mutations & MUT_UNDEAD) {
        strcpy(list->text[n], "Undead");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special2_icons[SPECIAL2_ICON_UNDEAD];
        list->help[n] = HELP_UNDEAD;
        n++;
    }
    mutations = p_units[unit].mutation_flags;
    if (mutations & (MUT_STASIS_NEW | MUT_STASIS)) {
        strcpy(list->text[n], "Stasis");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special2_icons[SPECIAL2_ICON_STASIS];
        list->help[n] = HELP_STASIS;
        n++;
    }
    ench = p_units[unit].enchantments;
    for (k = 0; k < NUM_ENCHANT_NAMES && n < ABIL_LIST_MAX_ROWS; k++) {
        if (ench & enchant_name_table[k].mask) {
            strcpy(list->text[n], enchant_name_table[k].name);
            list->ench_mask[n] = enchant_name_table[k].mask;
            list->icon[n] = special_icons[enchant_name_table[k].icon];
            list->help[n] = enchant_name_table[k].help;
            n++;
        }
    }
    *count = n;
}


/* (unit_type, list, &count) the ability lines of a unit type (the city production preview): Construction, Scouting,
   Teleporting / Forester / Mountaineer, the ability tables, the attack flags and the ranged specials. BUG #486: the
   poison / stoning / gaze strengths and the ranged test come from p_unit_stats_buf (the last unit's statistics),
   not from the type */
void UnitView_Build_Type_Abilities(int unit_type, struct UnitAbilityList *list, int *count)
{
    int k;
    char num_text[10];
    int bonus;
    char roman[5][5];
    int n;

    strcpy(roman[0], "");
    strcpy(roman[1], "I");
    strcpy(roman[2], "II");
    strcpy(roman[3], "III");
    strcpy(roman[4], "IV");
    *count = 0;
    n = 0;
    if (unit_types[unit_type].construction > 0) {
        strcpy(list->text[n], "Construction");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_CONSTRUCTION];
        list->help[n] = HELP_CONSTRUCTION;
        n++;
    }
    if (unit_types[unit_type].scouting > 1) {
        strcpy(list->text[n], "Scouting");
        bonus = unit_types[unit_type].scouting;
        strcpy(num_text, roman[bonus]);
        strcat(list->text[n], " ");
        strcat(list->text[n], num_text);
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_SCOUTING];
        list->help[n] = HELP_SCOUTING;
        n++;
    }
    if (unit_types[unit_type].move_flags & MOVE_TELEPORTING) {
        strcpy(list->text[n], "Teleporting");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_TELEPORTING];
        list->help[n] = HELP_TELEPORTING;
        n++;
    }
    if (unit_types[unit_type].move_flags & MOVE_FORESTER) {
        strcpy(list->text[n], "Forester");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_FORESTER];
        list->help[n] = HELP_FORESTER;
        n++;
    }
    if (unit_types[unit_type].move_flags & MOVE_MOUNTAINEER) {
        strcpy(list->text[n], "Mountaineer");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_MOUNTAINEER];
        list->help[n] = HELP_MOUNTAINEER;
        n++;
    }
    for (k = 0; k < NUM_ABILITY_FLAG_NAMES; k++) {
        if (unit_types[unit_type].ability_flags & ability_flag_name_table[k].mask) {
            strcpy(list->text[n], ability_flag_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ability_flag_name_table[k].icon];
            list->help[n] = ability_flag_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_ABILITY_NAMES; k++) {
        if (unit_types[unit_type].abilities & ability_name_table[k].mask) {
            strcpy(list->text[n], ability_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ability_name_table[k].icon];
            list->help[n] = ability_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_ABILITY2_NAMES; k++) {
        if ((char)unit_types[unit_type].abilities2 & ability2_name_table[k].mask) {
            strcpy(list->text[n], ability2_name_table[k].name);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ability2_name_table[k].icon];
            list->help[n] = ability2_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_ATTACK_FLAG_NAMES; k++) {
        if (unit_types[unit_type].attack_flags & attack_flag_name_table[k].mask) {
            strcpy(list->text[n], attack_flag_name_table[k].name);
            if (attack_flag_name_table[k].mask == ATTACK_POISON) {
                bonus = p_unit_stats_buf->touch_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
            }
            if (attack_flag_name_table[k].mask == ATTACK_STONING_TOUCH) {
                bonus = p_unit_stats_buf->special_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
            }
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[attack_flag_name_table[k].icon];
            list->help[n] = attack_flag_name_table[k].help;
            n++;
        }
    }
    for (k = 0; k < NUM_RANGED_SPECIALS && n < ABIL_LIST_MAX_ROWS; k++) {
        if (p_unit_stats_buf->ranged_type == ranged_special_table[k].key) {
            strcpy(list->text[n], ranged_special_table[k].name);
            bonus = p_unit_stats_buf->ranged;
            if (ranged_special_table[k].key == RANGED_STONING_GAZE || ranged_special_table[k].key == RANGED_DEATH_GAZE)
                bonus = p_unit_stats_buf->special_strength;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], " ");
            strcat(list->text[n], num_text);
            list->ench_mask[n] = ABIL_ROW_PLAIN;
            list->icon[n] = special_icons[ranged_special_table[k].icon];
            list->help[n] = ranged_special_table[k].help;
            n++;
            if (ranged_special_table[k].key == RANGED_DOOM_GAZE) {
                strcpy(list->text[n], ranged_special_table[k + 1].name);
                bonus = p_unit_stats_buf->special_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = special_icons[ranged_special_table[k + 1].icon];
                list->help[n] = ranged_special_table[k + 1].help;
                n++;
                strcpy(list->text[n], ranged_special_table[k - 1].name);
                bonus = p_unit_stats_buf->special_strength;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], " ");
                strcat(list->text[n], num_text);
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = special_icons[ranged_special_table[k - 1].icon];
                list->help[n] = ranged_special_table[k - 1].help;
                n++;
            }
        }
    }
    *count = n;
}


/* DS:3f64 */
struct ItemPowerName item_power_name_table[32] = {
    {"Vampiric", ", life draining", ITEM_POWER_VAMPIRIC},
    {"Lightning", ", armor piercing", ITEM_POWER_LIGHTNING},
    {"Cloak of Fear", ", as cloak of fear spell", ITEM_POWER_CLOAK_OF_FEAR},
    {"Destruction", ", resist or be disintegrated", ITEM_POWER_DESTRUCTION},
    {"Wraithform", ", as wraithform spell", ITEM_POWER_WRAITHFORM},
    {"Regeneration", ", as regeneration spell", ITEM_POWER_REGENERATION},
    {"Pathfinding", ", as pathfinding spell", ITEM_POWER_PATHFINDING},
    {"Water Walking", ", as water walking spell", ITEM_POWER_WATER_WALKING},
    {"Resist Elements", ", as resist elements spell", ITEM_POWER_RESIST_ELEMENTS},
    {"Elemental Armor", ", as elemental armor spell", ITEM_POWER_ELEMENTAL_ARMOR},
    {"Doom", ", ignores armor", ITEM_POWER_DOOM},
    {"Endurance", ", as swiftness spell", ITEM_POWER_ENDURANCE},
    {"Haste", ", doubles attacks", ITEM_POWER_HASTE},
    {"Invisibility", ", as the invisibility spell", ITEM_POWER_INVISIBILITY},
    {"Death", ", save at -3 or die", ITEM_POWER_DEATH},
    {"Flight", ", as flight spell", ITEM_POWER_FLIGHT},
    {"Resist Magic", ", as resist magic spell", ITEM_POWER_RESIST_MAGIC},
    {"Magic Immunity", ", as magic immunity spell", ITEM_POWER_MAGIC_IMMUNITY},
    {"Power Drain", ", target loses 2-20 mana", ITEM_POWER_POWER_DRAIN},
    {"Holy Avenger", ", as bless & dispel evil spell", ITEM_POWER_HOLY_AVENGER},
    {"True Sight", ", as true sight spell", ITEM_POWER_TRUE_SIGHT},
    {"Phantasmal", ", ignores armor & illusion attacks", ITEM_POWER_PHANTASMAL},
    {"Flaming", ", +3 attack strength", ITEM_POWER_FLAMING},
    {"Bless", ", as bless spell", ITEM_POWER_BLESS},
    {"Lion Heart", ", as lion heart spell", ITEM_POWER_LION_HEART},
    {"Stoning", ", resist at -1 or be stoned", ITEM_POWER_STONING},
    {"Planar Travel", ", as planar travel spell", ITEM_POWER_PLANAR_TRAVEL},
    {"Merging", ", as merging spell", ITEM_POWER_MERGING},
    {"Righteousness", ", as righteousness spell", ITEM_POWER_RIGHTEOUSNESS},
    {"Invulnerability", ", as invulnerability spell", ITEM_POWER_INVULNERABILITY},
    {"Guardian Wind", ", as guardian wind spell", ITEM_POWER_GUARDIAN_WIND},
    {"Giant Strength", ", as giant strength spell", ITEM_POWER_GIANT_STRENGTH}
};

/* (item, names, descs, &count) an item's power lines, at most 5: "+n Attack", "+n To Hit", "+n Defense", "+n
   Movement" (half the moves), "+n Resistance", "+n Spell Skill", "-n Spell Save", "<spell> x <charges>", then each
   power's name with its description */
void UnitView_Item_Power_Texts(int item, char names[][20], char descs[][40], int *count)
{
    int k;
    char num_text[30];
    int n;

    *count = 0;
    n = 0;
    for (k = 0; k < 5; k++)
        strcpy(descs[k], "");
    if (p_items[item].attack > 0) {
        strcpy(names[n], "+");
        itoa(p_items[item].attack, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " Attack");
        n++;
    }
    if (p_items[item].to_hit > 0 && n < 5) {
        strcpy(names[n], "+");
        itoa(p_items[item].to_hit, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " To Hit");
        n++;
    }
    if (p_items[item].defense > 0 && n < 5) {
        strcpy(names[n], "+");
        itoa(p_items[item].defense, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " Defense");
        n++;
    }
    if (p_items[item].moves > 0 && n < 5) {
        strcpy(names[n], "+");
        itoa(p_items[item].moves / 2, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " Movement");
        n++;
    }
    if (p_items[item].resist > 0 && n < 5) {
        strcpy(names[n], "+");
        itoa(p_items[item].resist, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " Resistance");
        n++;
    }
    if (p_items[item].spell_skill > 0 && n < 5) {
        strcpy(names[n], "+");
        itoa(p_items[item].spell_skill, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " Spell Skill");
        n++;
    }
    if (p_items[item].spell_save > 0 && n < 5) {
        strcpy(names[n], "-");
        itoa(p_items[item].spell_save, num_text, 10);
        strcat(names[n], num_text);
        strcat(names[n], " Spell Save");
        n++;
    }
    if ((unsigned char)p_items[item].spell > 0 && n < 5) {
        far_strcpy(num_text, 0, p_spell_data[(unsigned char)p_items[item].spell].name);
        strcpy(names[n], num_text);
        strcat(names[n], " x");
        itoa(p_items[item].charges, num_text, 10);
        strcat(names[n], " ");
        strcat(names[n], num_text);
        n++;
    }
    for (k = 0; k < 32 && n < 5; k++) {
        if (p_items[item].powers & item_power_name_table[k].mask) {
            strcpy(names[n], item_power_name_table[k].name);
            strcpy(descs[n], item_power_name_table[k].description);
            n++;
        }
    }
    if (n > 5)
        n = 5;
    *count = n;
}
