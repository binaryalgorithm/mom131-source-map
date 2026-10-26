/* STRUCTS.H - the record layouts of both programs. Kept by hand since the module matches proved the types; the first version
   came from re/structs.tsv (tools/make_header.py), whose rows still hold the evidence behind each field.
   Borland C++ 3.1, medium model (-mm), word alignment of the original: every pad is explicit. */
#ifndef STRUCTS_H
#define STRUCTS_H

struct HiredHero;
struct GlobalEnchants;
struct Wizard;
struct Unit;
struct City;
struct Building;
struct UnitType;
struct Spell;
struct Item;
struct BattleUnit;
struct Events;
struct Battlefield;
struct ItemPower;
struct ItemData;
struct Lair;
struct UnitAbilityList;
struct HeroStats;
struct GameConfig;
struct HeroName;
struct SpellbookPage;
struct Node;
struct Fortress;
struct TerrainGfx;
struct Tower;
struct RaceInfo;
struct DifficultyInfo;
struct AbilityName;
struct HeroTemplate;
struct HeroAbility;
struct RangedSpecialName;
struct MutationName;
struct EnchantName;
struct ItemPowerName;
struct HelpEntry;
struct WizardPreset;
struct LandmassPlan;
struct UIControl;
struct PathWork;
struct AIStack;
struct CreditLine;
struct StackSlot;
struct UnitLostRecord;
struct BuildingLostRecord;
struct CityNotice;
struct UnitKilledRecord;
struct EnchMask;
struct EmsLbxEntry;
struct CombatEffectRow;
struct PicHelp;
struct EnchIcon;
struct MouseUnder;
struct CombatSprite;
struct CombatVortex;
struct CombatMissile;
struct AIEnemyStack;
struct PathCacheEntry;
struct CityscapeHotspot;
struct LbxPicHeader;
struct HelpRecord;
struct AILDriverDesc;
struct FogRule;
struct LoseAnimStep;
struct MouseRegion;
struct TimbreEntry;
struct ConfigMom;
struct ReportUnitLost;
struct ReportUnitKilled;
struct ReportBldgLost;

/* a hero in a wizard's service: Wizard.hired_heroes[6] */
struct HiredHero {
    int unit_index;   /* index into p_units; -1 an empty slot */
    char name[14];   /* the hero's name as the player set it */
    int item_index[3];   /* the three equipped items: indexes into p_items, -1 none */
    int slot_type[3];   /* ItemSlot: what each of the three slots takes (by hero class, Hero_Item_Slot_Types) */
};   /* 0x1c */

/* a wizard's own global enchantments, one byte per GlobalEnchant slot: 0 none, else the wizard's player + 1
   (OVERCAST.C) */
struct GlobalEnchants {
    char eternal_night;
    char evil_omens;
    char zombie_mastery;
    char aura_of_majesty;
    char wind_mastery;
    char suppress_magic;
    char time_stop;
    char nature_awareness;
    char nature_s_wrath;
    char herb_mastery;
    char chaos_surge;
    char doom_mastery;
    char great_wasting;
    char meteor_storm;
    char armageddon;
    char tranquility;
    char life_force;
    char crusade;
    char just_cause;
    char holy_arms;
    char planar_seal;
    char charm_of_life;
    char detect_magic;
    char awareness;
};   /* 0x18 */

#define SPELLS_PER_REALM 40   /* SpellId = realm * 40 + slot + 1 (Arcane 201..214); spell_library[realm][slot] */
#define SPELL_SLOT(spell) (((spell) - 1) % SPELLS_PER_REALM)    /* the spell's slot in its realm */
#define SPELL_REALM(spell) (((spell) - 1) / SPELLS_PER_REALM)   /* its realm (Realm) */
#define SPELL_SLOT_UNCOMMON 10   /* a realm's slots: 0..9 common, 10..19 uncommon, 20..29 rare, 30..39 very rare */
#define SPELL_SLOT_RARE 20
#define SPELL_SLOT_VERY_RARE 30

/* a player: wizards[6] (0 the human, 1 .. 4 the AI wizards, 5 the neutral player); saved with the game */
struct Wizard {
    char portrait;   /* the portrait (0 .. 13 the preset wizards' pictures); signed */
    char wizard_name[20];   /* the wizard's name, NUL-terminated */
    char home_race;   /* RaceId */
    char banner_color;   /* BannerColor */
    unsigned char pad_17;   /* padding after the byte banner_color */
    int personality;   /* Personality */
    int objective;   /* Objective */
    unsigned char unused_1c[6];   /* no reference in either program; 0 in every save */
    unsigned mastery_research_cost;   /* research points for the Spell of Mastery */
    int fame;
    int power_base;
    int volcanoes;   /* volcanoes this wizard owns (each gives volcano_mana) */
    char research_percentage;   /* the power split: research share in percent (the three add up to 100) */
    char mana_percentage;   /* mana share in percent */
    char skill_percentage;   /* casting-skill share in percent */
    unsigned char volcano_mana;   /* mana from volcanoes per turn */
    int summoning_circle_xyp[3];   /* the summoning circle: x, y, plane */
    int research_candidates[8];   /* the spells offered for research (0 none), the one being researched included */
    unsigned char unused_44[4];   /* no reference in either program; 0 in every save */
    int average_unit_cost;   /* the AI's mean unit cost (AI_Average_Unit_Cost, every 25 turns; units below half are
                                disbanded); for the neutral player the lair timer */
    int unused_4a;   /* no reference in either program; 0 in every save */
    int combat_skill_left;   /* casting skill left in the current battle */
    int casting_cost_left;   /* mana still to spend on the spell being cast overland */
    int casting_cost_initial;   /* the full cost of the spell being cast; for the neutral player the raid timer
                                   (AINEUTRL.C NEUTRAL_RAID_TIMER) */
    int casting_spell;   /* SpellId */
    int skill_left_this_turn;   /* casting skill left this turn (Turn_Overland_Casting) */
    int nominal_casting_skill;   /* the casting skill (Wizard_Casting_Skill) */
    int tax_rate;   /* 0 .. 6: the tax, rate / 2 gold per citizen */
    int spellbooks[5];   /* books per realm, in Realm order (Nature, Sorcery, Chaos, Life, Death) */
    char retorts[18];   /* 0 / 1 per Retort; signed */
    struct HiredHero hired_heroes[6];   /* the heroes in service (MAX_HEROES slots) */
    int unused_11e;   /* no reference in either program; 0 in every save */
    int vault_items[4];   /* the item vault: indexes into p_items, -1 empty */
    unsigned char contacted[6];   /* 0 / 1 per wizard: contact made (the wizard's own entry is set every turn) */
    int treaty_interest[6];   /* per wizard: the AI's interest in a treaty (diplomacy scores) */
    int peace_interest[6];   /* per wizard: interest in peace */
    int trade_interest[6];   /* per wizard: interest in spell trades */
    char relationship[6];   /* the current relation to each wizard (-100 .. 100); drifts to base_relation */
    char diplomacy_status[6];   /* per wizard: Treaty (TREATY_WAR and above at war); signed */
    int pending_rel_amount[6];   /* per wizard: the pending relation change (the largest kept), cleared each turn */
    char pending_rel_msg[6];   /* per wizard: the DiploMsg of that pending change; signed */
    int pending_msg_arg[6];   /* per wizard: the message's argument: a wizard, a spell or a unit type, by control
                                 code */
    char pending_msg_city[6];   /* per wizard: the city the message names (control codes 0x83 / 0x84); signed */
    char base_relation[6];   /* the level relationship drifts toward */
    unsigned char met_stage[6];   /* per wizard: MetStage */
    unsigned char broken_treaty_type[6];   /* per wizard: the treaty of the last Diplo_Break_Treaty
                                              (TREATY_WIZARD_PACT / _ALLIANCE) */
    int msg_wizard_arg[6];   /* per wizard: a wizard a message names (control code 0x87); every writer stores 0 */
    char relation_bias[6];   /* a lasting modifier added to the relation (the audience score, treaty breaks) */
    unsigned char demand_drop_ally[6];   /* per wizard: the ally the AI objects to; written, never read (BUG #225) */
    char refusal_reason_msg[6];   /* per wizard: a refusal message type Diplo_Show_Reply would use; every writer
                                     stores 0 */
    int tribute_spell_given[6];   /* SpellId */
    char tribute_spell[6];   /* SpellId8; per wizard: a spell offered as tribute */
    int wanted_spell[6];   /* SpellId; per wizard: the spell the AI wants in a trade */
    int spells_offered[6][6];   /* SpellId; [wizard][n]: the spells the AI offers in a trade */
    char spells_offered_count[6];   /* per wizard: entries in spells_offered; signed */
    int tribute_gold[6];   /* per wizard: gold offered as tribute */
    char war_request_target[6];   /* per wizard: the wizard an ally asks the human to fight (-1 none); signed */
    int unused_230[6];   /* no reference in either program; 0 in every save */
    int msg_number_arg[6];   /* per wizard: a number a message prints (control codes 0x88 / 0x8a); never written */
    unsigned char pay_for_war_pending[6];   /* per wizard: an offer to pay for a war, queued as
                                               DIPLO_MSG_PAY_FOR_WAR; every writer stores 0 */
    char warning_count[6];   /* per wizard: the AI's warnings before it breaks a treaty (AI_React_To_Offense);
                                signed */
    unsigned char border_warning_stage[6];   /* per wizard: 0 .. 2, units of this wizard parked by the other's
                                                cities under a treaty (AI_Border_Tension) */
    int research_left;   /* research points still needed for research_spell */
    int mana_reserve;   /* mana in reserve (capped at 32000) */
    long skill_points;   /* casting skill points: the skill is the smallest n with n (n + 1) >= this; signed */
    int research_spell;   /* SpellId being researched */
    char spell_library[6][40];   /* [Realm][slot]: SpellState; the neutral player's holds the AI planning tables
                                    (GAMEDATA.C Alloc_Game_Data) */
    unsigned char defeated_wizards[2];   /* a bit per wizard this one has banished (Bit_Set_Near / Bit_Test_Near;
                                            the Hall of Fame counts them) */
    int gold;   /* gold in the treasury */
    int unused_358;   /* no reference in either program; 0 in every save */
    int stat_magic_power;   /* the Astrologer's Magic Power bar: max(0, power_base - 10), scaled x 200 / max when
                               the leader passes 200 */
    int stat_spell_research;   /* the Astrologer's Spell Research bar: the sum of (slot + 1) over the known spells /
                                  8, scaled likewise */
    int stat_army_strength;   /* the Astrologer's Army Strength bar: the units' (gold upkeep + 2 x mana upkeep) / 5,
                                 scaled likewise */
    int stat_population;   /* the cities' population / 10; not shown, only part of the history sum */
    unsigned char power_history[288];   /* the Historian's graph, a byte per turn: (the four stats before scaling) /
                                           7, scaled x 160 / max; after 288 turns it scrolls */
    struct GlobalEnchants globals;   /* the wizard's global enchantments (struct GlobalEnchants) */
    int ai_focus;   /* AIFocus (AI_Choose_Strategy; read by AI_Magic_Allocation) */
    int unused_49c;   /* no reference in either program; 0 in every save */
    int hostility[6];   /* per wizard: Hostility (the AI's stance; HOSTILITY_HOSTILE and above make a target) */
    int hostility_replan_timer;   /* turns to the next AI_Hostility_Replan; every relation event resets it to
                                     Random(10) + 15 */
    int ai_strategy_timer;   /* turns to the next AI_Choose_Strategy (Random(10) + 15) */
    int ai_magic_timer;   /* turns to the next AI_Magic_Allocation (Random(10) + 15) */
    char peace_turns[6];   /* per wizard: the peace timer, Random(6) + 7 after a peace; counted down each turn;
                              signed */
    int unused_4b6[2];   /* no reference in either program; -1, -1 in every save */
    int ai_attack_target;   /* a slot of ai_hostile_wizards (AI_Pick_Spell_Category), not a player; -1 none */
    int unused_4bc[4];   /* no reference in either program; -1, 0, 0, 0 in every save */
    int primary_realm;   /* Realm; the AI's main realm */
    int secondary_realm;   /* Realm; the AI's second realm */
};   /* 0x4c8 */

/* an overland unit: p_units[MAX_UNITS]; saved with the game */
struct Unit {
    char x;   /* map square; signed */
    char y;   /* signed */
    char plane;   /* Plane; signed */
    char owner;   /* Player; PLAYER_NONE when the unit is gone */
    char moves_max;   /* movement in half moves (2 = one move); signed */
    unsigned char type;   /* UnitTypeId (below NUM_HERO_TYPES a hero) */
    char hero_slot;   /* its HiredHero slot; -1 not a hero */
    unsigned char finished;   /* 1: done for this turn (moved, fought, disbanded: no longer selectable) */
    char moves_left;   /* half moves left this turn; signed */
    char dest_x;   /* also ORDER_PURIFY's work counter (Purify_Turn marks the purifiers ORDER_PURIFYING while it
                      counts, STACKCMD.C) and ORDER_AI_WAIT's countdown (ORDERS.C) */
    char dest_y;   /* signed */
    char order;   /* UnitOrder; signed */
    char level;   /* experience level: 0 recruit, 1 regular, 2 veteran, 3 elite ..; signed */
    unsigned char unused_0d;   /* no access in either program (stale bytes in saves) */
    int experience;   /* experience points */
    unsigned char goto_blocked;   /* 1: its go-to could not go on this turn */
    char damage;   /* hit points lost (front figure: damage % hits per figure); signed */
    char draw_priority;   /* the stack's top figure on the map: melee + ranged (at least 1), +50 a transport, -1 not
                             drawn; recomputed, signed */
    unsigned char unused_13;   /* no access in either program (stale bytes in saves) */
    int in_tower;   /* 1: standing in a Tower of Wizardry (on both planes) */
    char scout_range;   /* scouting radius 1 .. 3 (Stack_Scout_Range); signed */
    char mutation_flags;   /* MUT_*: weapon grade (bits 0-1), Chaos Channels, created undead, Stasis */
    long enchantments;   /* UnitEnchant bits; signed (the code tests (enchantments & mask) > 0) */
    char road_turns;   /* turns left on the road square being built; -1 none (99 is cleared to -1 after a move;
                          nothing in either program sets 99) */
    char road_start_x;   /* the road being built: the square it started on; signed */
    char road_start_y;   /* signed */
    unsigned char unused_1f;   /* no access in either program */
};   /* 0x20 */

/* a city or outpost: p_cities[MAX_CITIES]; saved with the game */
struct City {
    char name[14];   /* the city's name */
    char race;   /* RaceId */
    char x;   /* map square; signed */
    char y;   /* signed */
    char plane;   /* Plane; signed */
    char owner;   /* Player */
    char size;   /* 0 outpost, else (population + 3) / 4, at most 5 (TURNUPD.C) */
    char population;   /* citizens; signed */
    char farmers;   /* citizens farming; signed */
    unsigned char sold_this_turn;   /* 1: a building was sold this turn (one sale a turn) */
    unsigned char unused_17;   /* alignment pad: the int after it sits on an even offset under -a (ReMoM: pad2B_17h;
                                  clarified using ReMoM repo); no access in either program */
    int population_tens;   /* growth toward the next citizen: in a city 0 .. 99 (x 10 people, 100 adds one); in an
                              outpost (population 0) 0 .. 10 growth houses (x 100 people, 10 makes it a city) */
    unsigned char seen_by;   /* a bit per player (Bit_Set / Bit_Test) */
    unsigned char unused_1b;   /* alignment pad, as unused_17 (ReMoM: pad2B_1Bh) */
    int producing;   /* Production */
    char building_count;   /* buildings ever built (the entries not BLDSTATE_NONE, sold ones too), recomputed by
                              City_Build_Options; signed */
    char buildings[36];   /* BuildingState */
    char enchants[26];   /* [CityEnchant]: 0 none, else the caster player + 1; [CITY_ENCH_NIGHTSHADE] the count of
                            Nightshade squares (Cities_Count_Nightshade) */
    unsigned char production_income;   /* per turn, as the next four */
    int production_progress;   /* production put into the current item; done at its cost */
    unsigned char gold_income;   /* gold per turn before upkeep */
    unsigned char building_upkeep;   /* the buildings' gold upkeep per turn */
    unsigned char power_income;   /* power (mana) per turn */
    unsigned char research_income;   /* research per turn */
    unsigned char food_income;   /* food per turn */
    unsigned char road_connections[13];   /* a bit per city: connected by road (City_Road_Trade_Pct) */
};   /* 0x72 */

/* BUILDDAT.LBX record: p_building_data[NUM_BUILDINGS] */
struct Building {
    char name[20];   /* the building's name */
    int prereq1;   /* BuildingId16 */
    int prereq2;   /* BuildingId16 */
    int replaces;   /* BuildingId16 the building this one upgrades (Armory: Barracks); -1 none */
    int unread_1a;   /* data never read: 1 only for the Fighters Guild */
    int unread_1c;   /* data never read: 1 only for the War College */
    int unread_1e;   /* data never read: 1 only for the Alchemists Guild */
    int gold_upkeep;   /* gold upkeep per turn, stored negative */
    int unread_22;   /* data never read: Granary 2, Farmers Market 5 */
    int zero_24;   /* 0 in every record, never read */
    int unread_26;   /* data never read: Alchemists Guild 3, Wizards Guild 8 */
    int religion_weight;   /* Shrine 2, Temple 5, Parthenon 9, Cathedral 15; read only by Wizard_Religion_Total,
                              which has no caller */
    int unread_2a;   /* data never read: Library 2, Sages Guild 3, Oracle 6, University 7, Wizards Guild 8 */
    int cost;   /* production cost */
    int zero_2e;   /* 0 in every record, never read */
    int cityscape_size;   /* the cityscape lot class 0 .. 3 (-1 not drawn): Cityscape_Layout places by it */
    int category;   /* 0 .. 7, the AI's building class (ai_bldg_category_weight, ai_bldg_objective_bonus) */
};   /* 0x34 */

/* heroes read UnitType.req_building_1 as their PORTRAIT.LBX index and req_building_2 as their item-slot class */
#define hero_portrait req_building_1
#define hero_item_class req_building_2

/* a unit type: unit_types[NUM_UNIT_TYPES] (MAGIC.C / WIZARDS.C data) */
struct UnitType {
    char *name;   /* the name (a near string) */
    char melee;   /* melee strength; signed */
    char ranged;   /* ranged strength; signed */
    char ranged_type;   /* RangedType; signed */
    unsigned char ammo;   /* ranged shots per battle */
    char to_hit;   /* to-hit bonus */
    char defense;   /* signed */
    unsigned char resist;   /* resistance */
    char moves;   /* half moves; signed */
    int cost;   /* production cost */
    char upkeep;   /* gold (or, for creatures, mana) upkeep per turn; signed */
    char race;   /* RaceId: 0 .. 13 a race, 14 generic, 15 .. 20 a fantastic creature's realm (RACE_*_CREATURE) */
    unsigned char req_building_1;   /* heroes: the PORTRAIT.LBX index */
    unsigned char req_building_2;   /* normal units: a second building required; heroes: the item-slot class
                                       (Hero_Item_Slot_Types) */
    unsigned map_figure_pic;   /* the map figure picture */
    char hits;   /* hits per figure; signed */
    char scouting;   /* scouting range; above 1 listed "Scouting <n>"; signed */
    char transport_capacity;   /* units carried (Trireme 2, Galley 5, Warship 3, Floating Island 8); signed */
    char figures;   /* figures in the unit; signed */
    char construction;   /* road-building power (Road_Build_Turns); signed */
    char special_strength;   /* the special attack's strength: poison / touch (+), gaze or life steal as a negative
                                modifier; signed */
    char move_flags;   /* MoveFlag bits; signed */
    unsigned char unused_19;   /* the byte after move_flags; never read */
    unsigned abilities;   /* UnitAbilities bits */
    unsigned abilities2;   /* UnitAbilities2 bits (the unit's own spells, Holy Bonus ...); MAGIC.C's table sets high-byte
                              bits too, WIZARDS reads the low byte only */
    unsigned ability_flags;   /* UnitAbilityFlags bits (AFLAG_*) */
    unsigned attack_flags;   /* AttackFlag bits */
    unsigned char sounds;   /* high nibble the move sound, low nibble the attack sound */
    unsigned char pad_23;   /* 0 in every type, never read */
};   /* 0x24 */

/* SPELLDAT.LBX record: p_spell_data[NUM_SPELLS] (record 0 the empty spell) */
struct Spell {
    char name[18];   /* the spell's name */
    unsigned char pad_12;   /* 0 in every record, never read */
    char ai_group;   /* AIGroup: how the AI groups the spell (research weights, casting; AIG_NONE never researched) */
    unsigned char pad_14;   /* 0 in every record, never read */
    char effect_type;   /* SpellEffect */
    unsigned char book_section;   /* BookSection */
    char realm;   /* Realm8 */
    unsigned char castable;   /* 1 overland only (CASTABLE_OVERLAND_ONLY), 0xff / 0xfe combat only (-1 / -2 read
                                 through (char)), 0 / 2 / 3 / 4 both - Spell_Casting_Cost counts only 0 / 2 / 3 as
                                 combat-capable -, 5 never (CASTABLE_NEVER) */
    unsigned char pad_19;   /* 0 in every record, never read */
    int base_cost;   /* the casting cost before reductions (Spell_Casting_Cost) */
    int research_cost;   /* research points */
    char sound_index;   /* the cast sound: >= 0 SOUNDFX.LBX entry, < 0 NEWSOUND.LBX entry -n */
    unsigned char pad_1f;   /* 0 in every record, never read */
    unsigned long effect_param;   /* per effect_type: unit type, slot, mask... */
};   /* 0x24 */

/* an item: p_items[NUM_ITEMS]; saved with the game */
struct Item {
    char name[30];   /* the item's name */
    unsigned icon;   /* the item's picture, an ITEMS.LBX entry */
    char slot_type;   /* ITEM_SLOT_* (as HiredHero.slot_type) */
    char item_type;   /* ItemType; signed */
    int cost;   /* value; below 1 free (Item_Free writes 0, a broken item -1); sold for cost / 2 */
    char attack;   /* attack bonus (a misc item adds it to melee and ranged) */
    char to_hit;   /* to-hit bonus (BattleUnit melee_hit_bonus / ranged_hit_bonus) */
    char defense;   /* defense bonus (the forge adds 1 for chain mail, 2 for plate) */
    char moves;   /* in half moves (the unit view shows moves / 2) */
    char resist;   /* resistance bonus */
    char spell_skill;   /* spell skill bonus */
    char spell_save;   /* spell save penalty for its targets (shown "-N Spell Save") */
    char spell;   /* SpellId8 */
    char charges;   /* charges of spell (1 .. 4) */
    unsigned char pad_2d;   /* 0 in every saved item, never read */
    long powers;   /* ItemPower bits */
};   /* 0x32 */

/* BattleUnit: the first 36 bytes are a copy of the unit type record from .melee on (UNITSTAT.C) - so
   req_building_2 arrives as figures_alive, map_figure_pic as gfx_slot, hits as hits_per_figure, unused_19 as
   move_flags_hi - then rewritten */
/* a unit's combat stats: p_battle_units[MAX_BATTLE_UNITS]; also p_unit_stats_buf, one unit's derived stats
   (Battle_Unit_Init) */
struct BattleUnit {
    char melee;   /* from UnitType, then the modifiers (Unit_Apply_Modifiers) */
    char ranged;   /* as melee */
    char ranged_type;   /* RangedType; signed */
    char ammo;   /* ranged shots left */
    char to_hit;   /* to-hit bonus */
    char defense;   /* as melee */
    char resist;   /* as melee */
    char moves;   /* half moves */
    int cost;   /* the type's cost */
    unsigned char upkeep;   /* gold upkeep (Unit_Gold_Upkeep) */
    unsigned char race;   /* RaceId, incl. the realm creatures 15 .. 20; UNITSTAT.C makes an undead unit
                             RACE_DEATH_CREATURE and a Chaos-Channels one RACE_CHAOS_CREATURE */
    unsigned char req_building_1;   /* UnitType.req_building_1: heroes' portrait */
    char figures_alive;   /* figures standing */
    int gfx_slot;   /* the figure-graphics slot (EMS cache); -1 none */
    char hits_per_figure;   /* Unit_Max_Hits */
    char scouting;   /* scouting range; above 1 listed "Scouting <n>"; signed */
    char transport_capacity;   /* from UnitType */
    char figures;   /* figures in the unit */
    char construction;   /* from UnitType (road building); > 0 listed "Construction" */
    char special_strength;   /* from UnitType; moved to touch_strength for a poison attack */
    char move_flags;   /* MoveFlag bits (the low byte of UnitType.move_flags, with the spells' changes) */
    unsigned char move_flags_hi;   /* UnitType.unused_19, arriving with the copy; never read */
    unsigned abilities;   /* UnitAbilities bits, with the spells' and items' additions */
    char abilities2;   /* UnitAbilities2 bits: the low byte of UnitType.abilities2 (the unit's own spells); signed */
    unsigned char abilities2_hi;   /* the high byte of UnitType.abilities2, arriving with the copy (MAGIC's table sets bits
                                      there); never read */
    int ability_flags;   /* UnitAbilityFlags bits (AFLAG_*), with the spells' changes; signed */
    int attack_flags;   /* AttackFlag bits; signed */
    unsigned char sounds_copy;   /* UnitType.sounds copied; never read (the sound loaders read UnitType) */
    unsigned char pad_21;   /* UnitType.pad_23 copied (always 0); never read */
    int combat_flags;   /* CombatFlag bits: the curses and combat states on the unit; signed */
    char melee_hit_bonus;   /* to-hit bonus of melee attacks (weapon grade, Holy Weapon / Arms, items) */
    char ranged_hit_bonus;   /* the same for ranged attacks */
    char block_bonus;   /* defense roll bonus (Lucky +1) */
    char weapon_icon;   /* the weapon grade + 1 (WEAPON_* + 1; 0 none), not an icon index */
    int melee_powers;   /* the melee attack's AttackFlag bits from items (+ Eldritch Weapon); signed */
    int ranged_powers;   /* the ranged attack's AttackFlag bits from items; signed */
    unsigned long item_enchantments;   /* the hero's items' enchantments, in Unit.enchantments' layout (UnitEnchant) */
    int unit_index;   /* index into p_units */
    char heal_overflow;   /* extra hits per figure (Battle_Unit_Max_Hits adds it) */
    char web_hp;   /* Web's strength left (12 when cast; the unit's strength is taken off each turn); stuck while >
                      0 */
    char status;   /* BattleStatus; signed */
    char owner;   /* Player; signed */
    unsigned char damage[3];   /* [DamageKind] */
    char front_damage;   /* damage on the front figure */
    long combat_enchantments;   /* the enchantments cast in this battle, in Unit.enchantments' layout (UnitEnchant); signed */
    char times_attacked;   /* attacks received this turn: a counter-attack loses times_attacked / 2 to-hit */
    unsigned char spell_skill;   /* the mana pool (Battle_Unit_Init) */
    unsigned char spell_points;   /* the mana left: starts as spell_skill */
    char item_charges;   /* charges of the hero's spell item left */
    char touch_strength;   /* the poison / touch attack's strength */
    char target_unit;   /* its target: a battle-unit index, -1 none */
    int combat_x;   /* battlefield square */
    int combat_y;   /* battlefield square */
    int dest_x;   /* the square it moves or attacks toward (the figures are drawn between by move_anim_step / 8) */
    int dest_y;   /* as dest_x */
    int move_anim_step;   /* 0 .. 8 during a move animation */
    int hit_anim_kills;   /* figures killed, for the hit animation */
    int aura_color;   /* the enchantment outline's realm row (0 Chaos, 1 Death, 2 Life, 3 Nature, 4 Sorcery), -1
                         none */
    int moving;   /* 1 while it moves */
    int action_state;   /* ACTION_* (both sides: done, wait, fled) and AI_ACTION_* (the AI's plan) */
    unsigned char confusion_result;   /* ConfusionResult this turn */
    unsigned char pad_57;   /* never accessed */
    int hit_anim_intensity;   /* 1 .. 5, the hit animation's strength */
    int zeroed_5a;   /* zeroed twice in Battle_Unit_Init, never read */
    int always_animate;   /* 1: its figures animate even standing (Unit_Set_Anim_Flags) */
    int attack_anim_role;   /* AnimRole */
    int draw_mode;   /* DrawMode */
    unsigned anim_ability;   /* 1: the unit has ABILITY_FLYING_ANIM (Unit_Set_Anim_Flags) */
    char melee_bonus_shown;   /* the unit view's gold boxes: points gained on the melee row */
    char ranged_bonus_shown;   /* as melee_bonus_shown, ranged row */
    char defense_bonus_shown;   /* as melee_bonus_shown, defense row */
    char resist_bonus_shown;   /* as melee_bonus_shown, resistance row */
    char hits_bonus_shown;   /* as melee_bonus_shown, hits row (Battle_Unit_Max_Hits) */
    char melee_penalty_shown;   /* points lost, drawn on the melee row */
    char ranged_penalty_shown;   /* as melee_penalty_shown, ranged row */
    char defense_penalty_shown;   /* as melee_penalty_shown, defense row (Berserk stores the whole old defense) */
    char resist_penalty_shown;   /* as melee_penalty_shown, resistance row */
    unsigned char pad_6d;   /* pads the record to 0x6e */
};   /* 0x6e */

/* each event-named field (meteor, gift, ... mana_short) holds an EventState; the _player / _city / _gold /
   _item / _site / _turns fields beside it are that event's data */
/* the random events' state: *p_events; each event a status (EventState) and its fields; saved with the game */
struct Events {
    int last_event_turn;   /* the turn of the last event */
    int meteor;   /* EventState */
    int meteor_player;
    int meteor_city;
    int gift;   /* EventState */
    int gift_player;
    int gift_item;   /* the item given */
    int disjunction;   /* EventState */
    int marriage;   /* EventState */
    int marriage_player;
    int marriage_city;   /* the neutral city that joins */
    int marriage_own_city;   /* the wizard's city it joins */
    int earthquake;   /* EventState */
    int quake_player;
    int quake_city;
    int pirates;   /* EventState */
    int pirates_player;
    int pirates_gold;   /* gold taken */
    int plague;   /* EventState */
    int plague_player;
    int plague_city;
    int plague_turns;   /* turns left */
    int rebellion;   /* EventState */
    int rebellion_player;
    int rebellion_city;
    int donation;   /* EventState */
    int donation_player;
    int donation_gold;
    int depletion;   /* EventState */
    int depletion_player;
    int depletion_site;   /* the mineral site that runs out */
    int new_minerals;   /* EventState */
    int minerals_site;   /* the new mineral site */
    int minerals_player;
    int population_boom;   /* EventState */
    int boom_city;
    int boom_player;
    int boom_turns;   /* turns left */
    int good_moon;   /* EventState */
    int good_moon_turns;   /* turns left */
    int bad_moon;   /* EventState */
    int bad_moon_turns;   /* turns left */
    int conjunction_chaos;   /* EventState */
    int conj_chaos_turns;   /* turns left */
    int conjunction_nature;   /* EventState */
    int conj_nature_turns;   /* turns left */
    int conjunction_sorcery;   /* EventState */
    int conj_sorcery_turns;   /* turns left */
    int mana_short;   /* EventState */
    int mana_short_turns;   /* turns left */
};   /* 0x64 */

#define BATTLEFIELD_CELL(x, y) ((y) * BATTLEFIELD_W + (x))   /* a cell of the 21 x 22 battlefield grids (BATTLEFIELD_W, ENUMS.H) */

/* the battle map: *p_battlefield (BATTLEFIELD_H rows of BATTLEFIELD_W squares, [row][column]) */
struct Battlefield {
    int tile[22][21];   /* the terrain picture of each square */
    unsigned char terrain_type[22][21];   /* BattleTerrain per square */
    unsigned char road[22][21];   /* per square: 1 a road (+ connection bits); 0x80 / 0x81 Floating Island squares */
    unsigned char cost_ground[22][21];   /* half moves to enter on foot; 0xff impassable */
    unsigned char cost_flying[22][21];   /* the same flying (always 2) */
    unsigned char cost_mode3[22][21];   /* COMBAT_MOVE_TYPE_3's map: no unit gets that move type */
    unsigned char cost_sailing[22][21];   /* the same sailing (water squares only) */
    int decor_a_count;   /* the scattered decorations of set A (Battlefield_Scatter_Decor_A) */
    int decor_a_x[100];
    int decor_a_y[100];
    int decor_a_index[100];   /* an index 0..4 into battlefield_decor_a_pics */
    int decor_b_count;   /* the decorations of set B (Battlefield_Scatter_Decor_B) */
    int decor_b_x[100];
    int decor_b_y[100];
    int decor_b_pic[100];   /* a picture handle (battlefield_decor_b_pics[]), not an index */
    unsigned char mud[22][21];   /* 1: Earth to Mud on the square */
    int backdrop;   /* Backdrop */
    int house_count;   /* a city's houses drawn on the field */
    int house_x[16];
    int house_y[16];
    int house_pic[16];   /* each house's picture */
    int has_city_walls;   /* 1: city walls stand */
    unsigned wall_segments[4][4];   /* the 4 x 4 wall ring around the city core, [row][column]: WallSegment
                                       (WALL_NONE / _STANDING / _BROKEN) */
    int wall_of_fire;   /* 1: Wall of Fire around the core */
    int wall_of_darkness;   /* 1: Wall of Darkness around the core */
    int plane;   /* Plane of the battle */
    char city_enchants[26];   /* the city's enchantments copied (as City.enchants); signed */
    unsigned char unused_1598[40];   /* no valid access (the BUG #278 wall reads land here) */
};   /* 0x15c0 */

/* ITEMPOW.LBX record: p_item_powers, the forge's powers */
struct ItemPower {
    char name[18];   /* the power's name (stat powers "+N Stat": byte 3 is cut to print the number) */
    int type_mask;   /* tested signed ((mask & m) > 0 -> test / jg) in 94df - so int (ReMoM: int16_t too) */
    int cost;   /* added to the item's cost (x 2 on misc items for the stat powers); also the per-power cap */
    char group;   /* 0 .. 6 the stat groups (Strength .. Protection: one power per group), 9 none (any number) */
    char realm;   /* Realm8; Realm of the books it needs */
    char books_required;   /* special powers: the books of realm needed; stat powers hold their bonus here (not
                              tested) */
    unsigned char pad_19;   /* 0 in every record */
    unsigned long powers;   /* ItemPower bits it adds to Item.powers (0 for the stat powers) */
};   /* 0x1e */

/* ITEMDATA.LBX record: a prefabricated item (Item_Create_Prefab) */
struct ItemData {
    struct Item item;   /* the item (moves doubled when copied) */
    unsigned char books_required[5];   /* books needed per realm, Realm order */
    unsigned char is_artifact;   /* 0 an Enchant Item item, 1 a Create Artifact / gift-event item */
};   /* 0x38 */

/* a lair, node site or tower guard record: p_lairs[NUM_LAIRS]; saved with the game */
struct Lair {
    char x;
    char y;
    char plane;   /* Plane */
    unsigned char active;   /* 1 until cleared (Lair_Clear sets 0) */
    char type;   /* LairType */
    unsigned char guard1_type;   /* the first guardians' UnitTypeId */
    unsigned char guard1_count;   /* low nibble the guards left (LAIR_GUARDS_LEFT_MASK), high nibble the
                                     count at the start (WORLDGEN.C: count + (count << 4)) */
    unsigned char guard2_type;   /* the second guardians' UnitTypeId (0 with guard2_count 0: none) */
    unsigned char guard2_count;   /* the same */
    unsigned char unused_09;   /* no access in either program (stale bytes in saves) */
    int gold;   /* treasure gold */
    int mana;   /* treasure mana */
    char special;   /* 1 .. 4 a spell of that rarity, 5 / 6 one / two spellbooks or retorts (LAIR.C) */
    unsigned char flags;   /* LairFlagBit bits (Bit_Test / Bit_Set) */
    char item_count;   /* treasure items 0 .. 3 */
    unsigned char unused_11;   /* no access in either program */
    int item_values[3];   /* a value per treasure item; ignored by Treasure_Roll (BUG #74) */
};   /* 0x18 */

/* the unit view's ability rows (40 at most): p_unit_view_list, p_cityprod_abil_list */
struct UnitAbilityList {
    char text[40][30];   /* each row's text (ability, item name, "<level> (<n> ep)") */
    long ench_mask[40];   /* each row's UnitEnchant bits; -1 a plain ability, -99 an item-slot row */
    unsigned icon[40];   /* each row's picture; -1 none */
    int help[40];   /* each row's help id; item-slot rows: the item (>= 0) or -(slot type + 0x240) when empty */
};   /* 0x5f0 */

/* a hero type's state for one player: p_hero_stats[player][hero type]; saved with the game */
struct HeroStats {
    int status;   /* >= 0 the level (available, or dismissed); -(level + 1) dead; HERO_STATUS_UNAVAILABLE in
                    service or never to be offered */
    long abilities;   /* HeroAbility bits; signed */
    char caster_level;   /* > 0 a caster: spell skill (v + 1) x (level + 1) x 5 / 2 (Hero_Ability_Bonus) */
    char spells[4];   /* SpellId8; the hero's own spells (SpellId8, 0 none) */
    unsigned char unused_0b;   /* the alignment pad that ends the 0xc record (ReMoM reads it so too; clarified using ReMoM repo);
                                  no access through p_hero_stats, yet the value changes between saves (writer not located) */
};   /* 0xc */

/* MAGIC.SET: the settings and the new-game choices, read and written whole: game_config */
struct GameConfig {
    int sound_effects;   /* the settings' option 0 "Sound Effects" (0 off / 1 on) */
    int music;   /* option 1 "Background Music" */
    int event_music;   /* option 2 "Event Music" */
    int city_spell_events;   /* option 3 "City Spell Events" */
    int overland_spell_events;   /* option 4 "Overland Spell Events" */
    int summoning_events;   /* option 5 "Summoning Events" */
    int end_turn_summary;   /* option 6 "End of Turn Summary" */
    int raze_city;   /* option 7 "Raze City" */
    int random_events;   /* option 8 "Random Events" */
    int end_turn_wait;   /* option 9 "End of Turn Wait" */
    int strategic_combat;   /* option 10 "Strategic Combat Only" */
    int auto_unit_info;   /* option 11 "Auto Unit Information" */
    int enemy_moves;   /* option 12 "Enemy Moves" */
    int enemy_spells;   /* option 13 "Enemy Spells" */
    int book_ordering;   /* option 14 "Spell Book Ordering" */
    int spell_animations;   /* option 15 "Spell Animations" */
    int show_node_owners;   /* option 16 "Show Node Owners" */
    int expanding_help;   /* option 17 "Expanding Help" */
    int sound_channels;   /* 0 .. 2; the start-up forces 2 */
    int input_type;   /* Engine_Init's input mode; the start-up forces 1 (the mouse) */
    int newgame_difficulty;   /* Difficulty */
    int newgame_opponents;   /* the new-game screen: opponents (MAGIC: num_players = this + 1) */
    int newgame_land_size;   /* the new-game screen: land size 0 .. 2 (360 / 480 / 720 land squares) */
    int newgame_magic_power;   /* the new-game screen: magic 0 .. 2 (node guardian budget x (this + 1)) */
    int save_slot_used[8];   /* 1: SAVEn.GAM holds a game (cleared at start-up when the file is gone) */
    char save_names[8][20];   /* the save descriptions */
    int movement_animations;   /* the "Movement Animations" option (combat moves, missiles, AI paths, page turns) */
    char hall_of_fame_names[10][20];   /* the Hall of Fame's ten names */
    int hall_of_fame_scores[10];   /* their scores */
    int hall_of_fame_race[10];   /* RaceId16 */
};   /* 0x1d2 */

/* a hero type's name and experience for the human: p_hero_names[NUM_HERO_TYPES]; saved with the game */
struct HeroName {
    char name[14];   /* the name the human gave (kept when the hero dies or is dismissed) */
    int experience;   /* experience restored by Hero_Spawn / Resurrection */
};   /* 0x10 */

/* a page of the spellbook: p_spellbook_pages (Spellbook_Build_Pages) */
struct SpellbookPage {
    char title[16];   /* the page's section title */
    int spells[6];   /* SpellId */
    char count;
    unsigned char pad_1d;   /* pads the record to 0x1e */
};   /* 0x1e */

/* a magic node: p_nodes[NUM_NODES]; saved with the game */
struct Node {
    char x;
    char y;
    char plane;   /* Plane */
    char owner;   /* Player */
    char power;   /* squares in its aura (power income x (the magic setting + 1) / 2) */
    char aura_x[20];   /* the aura squares' x */
    char aura_y[20];   /* their y */
    char realm;   /* NodeRealm */
    char meld_flags;   /* NodeMeldFlag bits (warped; melded by a Guardian Spirit) */
    unsigned char unused_2f;   /* no access in either program (stale bytes in saves) */
};   /* 0x30 */

/* a wizard's fortress: p_fortresses[MAX_PLAYERS]; saved with the game */
struct Fortress {
    char x;
    char y;
    char plane;   /* Plane */
    unsigned char active;   /* 1 while the wizard has a fortress (banished wizards drop out) */
};   /* 0x4 */

/* a terrain picture's place in EMS: terrain_gfx_table (TERRAIN.LBX entry 1), per terrain id */
struct TerrainGfx {
    unsigned char ems_page;   /* EMS logical page; 0x80 animated (4 frames), 0xff never mapped */
    unsigned char gfx;   /* the graphic within the mapped pages (24 paragraphs each) */
};   /* 0x2 */
#define TERRAIN_GFX_ANIMATED 0x80   /* TerrainGfx.ems_page: an animated terrain (its frames follow), */
#define TERRAIN_GFX_PAGE 0x7f       /* the EMS page */

/* a Tower of Wizardry: p_towers[NUM_TOWERS]; saved with the game */
struct Tower {
    char x;
    char y;
    char owner;   /* Player: the owner of the last unit to enter (Stack_Tower_Update) */
    unsigned char pad_03;   /* never accessed */
};   /* 0x4 */

/* a race's row: race_table[14] (MAGIC.C / WIZARDS.C data) */
struct RaceInfo {
    char *name;   /* the race's name */
    int forbidden_count;   /* entries used in forbidden_buildings */
    int forbidden_buildings[7];   /* BuildingId16; the buildings the race cannot build */
    int outpost_growth;   /* outpost growth chance */
    int growth;   /* population growth, tens per turn (City_Growth_Rate) */
    int house_style;   /* HOUSE_STYLE_*: roofed, tree houses, huts */
};   /* 0x18 */

/* the per-difficulty factors in percent for the AI: difficulty_table[5] */
struct DifficultyInfo {
    int ai_growth_pct;   /* city growth (City_Growth_Rate) */
    int outpost_growth_pct;   /* outpost growth */
    int production_pct;   /* production (City_Recalculate) */
    int gold_pct;   /* gold */
    int power_pct;   /* power (City_Recalculate, Wizard_Power_Base) */
    int research_pct;   /* research */
    int food_pct;   /* food */
    int upkeep_pct;   /* building upkeep */
};   /* 0x10 */

struct AbilityName {   /* a row of the unit view's ability name tables (UNITABIL.C) */
    char *name;
    unsigned mask;
    unsigned char icon;   /* SPECIAL.LBX index (read as a byte) */
    int help;
};   /* 0x8 */

#define LILWIZ_ENTRY(portrait, banner) ((portrait) * 5 + (banner))   /* LILWIZ.LBX: a wizard's gem, 5 banner
                                                                        colours per portrait */
#define UI_NO_HOTKEY ((char *)-1)   /* UI_Add_*: no key meant; hotkey[0] then reads the byte at DS:ffff */
#define MONSTER_PIC_FEMALE 0x2c     /* MONSTER.LBX: the summoning figure of a female hero or champion, */
#define MONSTER_PIC_MALE 0x2d       /* a male one */
#define MONSTER_PIC_ITEM 0x2e       /* and an item (a sword) */
#define UNDEAD_FROM_ITEM 10         /* Screen_Undead_Rise's type when a Vampiric item raised them (not a unit type) */

#define HERO_ABIL_PICK_FIGHTER 0   /* HeroTemplate.pick_kind: the random picks go to fighter abilities, */
#define HERO_ABIL_PICK_MAGE 1      /* to mage abilities (and caster level), */
#define HERO_ABIL_PICK_ANY 2       /* or to either */

/* a hero type's starting abilities: hero_templates[NUM_HERO_TYPES] (GAMESTRT.C Heroes_Init_Start) */
struct HeroTemplate {
    int picks;   /* random ability picks */
    int pick_kind;   /* HeroPick: fighter, mage or either */
    unsigned long abilities;   /* starting HeroStats.abilities (HeroAbility bits) */
    int caster_level;   /* starting HeroStats.caster_level */
    int spells[4];   /* the hero's own spells (stored as bytes in HeroStats.spells) */
};   /* 0x12 */

/* a hero ability row of the unit view: hero_ability_table[NUM_HERO_ABILITY_ROWS] */
struct HeroAbility {
    unsigned long mask;   /* its HeroAbility bit in HeroStats.abilities */
    unsigned char icon;   /* its icon frame */
    unsigned char pad_05;   /* 0 in every row */
    int help;   /* its help id */
};   /* 0x8 */

struct RangedSpecialName {   /* ranged_special_table: a ranged_type 100..105 and its line */
    char *name;
    char key;   /* ranged_type (compared as a signed byte) */
    unsigned char icon;
    int help;
};   /* 0x6 */

struct MutationName {   /* mutation_name_table: a Unit.mutation_flags bit */
    char *name;
    unsigned char mask;
    unsigned char icon;
    int help;
};   /* 0x6 */

struct EnchantName {   /* the enchantment / combat curse / item enchantment name tables */
    char *name;
    long mask;
    unsigned char icon;
    int help;
};   /* 0xa */

struct ItemPowerName {   /* item_power_name_table: an Item.powers bit, its name and card description */
    char *name;
    char *description;
    long mask;
};   /* 0x8 */

/* a screen's help area (HLPENTRY.LBX records): help_entries[50] */
struct HelpEntry {
    int id;   /* the HELP.LBX entry; -1 none */
    int x1;   /* the area: x1 <= mouse x <= x2, y1 <= mouse y <= y2 */
    int y1;
    int x2;
    int y2;
};   /* 0xa */

/* a preset wizard of the new-game screen: wizard_presets[15] (NEWGAME.C) */
struct WizardPreset {
    char name[10];   /* "Merlin" .. "Kali", "Custom" */
    int books_life;   /* the starting books in the order Life, Sorcery, Nature, Death, Chaos (not Realm order) */
    int books_sorcery;
    int books_nature;
    int books_death;
    int books_chaos;
    int retort;   /* Retort16 */
};   /* 0x16 */

/* the AI's plan per landmass for one plane and player: ai_landmass_plan[plane][player] */
struct LandmassPlan {
    char rally_x[60];   /* per landmass: the rally point */
    char rally_y[60];   /* per landmass: the rally point */
    char status[60];   /* per landmass: LandmassStatus; signed */
};   /* 0xb4 */

/* UIControl.param0 .. param6 by control type (UI.C's UI_Add_* functions):
   pictured buttons (button, toggle, latch, pic value): param0 label, param3 the picture's full-frames flag,
     param5 picture; toggle / latch param2 the int * flag; pic value param2 / param1 its two values
   picture: param5 picture        text input: param0 buffer, param2 fill, param3 right-align, param4 caret
     style, param5 max length, param6 caret colors
   text line: param0 text, param1 edit picture, param2 fill, param5 max length, param6 shadow style
   text button: param0 text, param1 its value, param2 the int * current value, param3 .. param6 four fills
   alt hotkey: param0 the code string, param2 its length     value region: param0 value, param1 stored
     value, param2 the int * out
   grid: param1 / param2 cell width / height, param3 / param4 the int * column / row
   slider: param0 / param1 min / max, param3 / param4 scale min / max, param5 0 horizontal / 1 vertical,
     param2 / param6 the value's segment / offset */
/* a control of the current screen: p_ui_controls[ui_control_count] (UI.C) */
struct UIControl {
    int x1;   /* the box: left */
    int y1;   /* top */
    int x2;   /* right (text input: x + width) */
    int y2;   /* bottom (text input: y + the font height) */
    int type;   /* UIControlType */
    int help;   /* its help id; -1 none */
    int font;   /* the font at creation (text controls) */
    int color;   /* the color set at creation */
    int fill_mode;   /* type 10: the highlighted line's fill (FillStyle) */
    int enabled;   /* type 10: selectable when not 0 */
    int color2;   /* the highlight color set at creation */
    int param0;   /* by type: 0 .. 3 the label, 4 the buffer, 8 the hotkey string, 9 value a, 10 / 11 the text, 6
                     the minimum */
    int param1;   /* by type: 3 value b, 6 the maximum, 8 the letters of a multi-letter hotkey matched, 9 value c,
                     10 the line's value, 12 the cell width */
    int param2;   /* by type: 1 / 2 a flag pointer, 3 value a, 4 / 11 the fill color, 6 the value's segment, 8 the
                     string length, 9 / 10 a result pointer, 12 the cell height */
    int param3;   /* by type: 0 .. 3 the picture's full-frames flag, 6 the scale minimum, 10 fill parameter 1, 12 a
                     column pointer */
    int param4;   /* by type: 4 the caret style (0 rising block, 1 blinking underline), 6 the scale maximum, 10 fill
                     parameter 2, 12 a row pointer */
    int param5;   /* by type: 0 .. 3 / 5 the picture, 4 / 11 the maximum length, 6 1 vertical, 10 fill parameter 3 */
    int param6;   /* by type: 4 / 11 the caret's color table, 6 the value's offset, 10 fill parameter 4 */
    char hotkey;   /* the hotkey letter, upper-cased; 0 none */
    unsigned char pad_25;   /* pads the record */
};   /* 0x26 */

/* the overland path finder's work area: *p_path_work (PATHFIND.C) */
struct PathWork {
    unsigned char cost[40][60];   /* move cost per square [y][x]; 0xff impassable */
    unsigned char dist[40][60];   /* the best cost found per square (rows past 40 of cost run into it: BUG #368) */
    int prev[2400];   /* the predecessor square (y x MAP_WIDTH + x) */
    int path[256];   /* the path found, as square indexes */
};   /* 0x2780 */

/* an AI player's own stack: p_ai_own_stacks (AI_Build_Strength_Tables) */
struct AIStack {
    char x;   /* the stack's square; signed */
    char y;
    char plane;
    unsigned char flags;   /* AIStackFlag bits */
    int strength;           /* the stack's strength; AISTACK_ORDERED (-1) once it has its orders */
    char spare_transport;   /* transports' capacity less one per unit that needs a ride; 8 with Wind Walking */
    char count;   /* units in the stack */
    char landmass;   /* the landmass of its square */
    char status;   /* UnitOrder */
};   /* 0xa */

/* a credits line: role and name (no source uses the struct: CREDITS.C keeps char *credits[][2]) */
struct CreditLine {
    char *role;   /* "Game Designer" ..; "" a continuation line */
    char *name;
};   /* 0x4 */

/* a unit of the selected stack: stack_units[MAX_STACK] */
struct StackSlot {
    int unit;   /* index into p_units */
    int active;   /* 1: moves with the stack */
};   /* 0x4 */

/* a turn report line: a unit lost (MAGIC's view; WIZARDS: ReportUnitLost) */
struct UnitLostRecord {
    unsigned unit_type;   /* the lost unit's UnitTypeId */
    unsigned char reason;   /* UnitLostReason */
    unsigned char pad_03;   /* never accessed */
};   /* 0x4 */

/* a turn report line: a building lost (MAGIC's view; WIZARDS: ReportBldgLost) */
struct BuildingLostRecord {
    unsigned char city;   /* the city */
    unsigned char pad_01;   /* never accessed */
    int building;   /* the BuildingId */
};   /* 0x4 */

/* a turn report line: what a city finished or can no longer build: city_notices */
struct CityNotice {
    char city;   /* the city; 0xff cleared */
    unsigned char pad_01;   /* never accessed */
    int producing;   /* Production; negated: the city can no longer build it (its prerequisite was lost) */
};   /* 0x4 */

/* a turn report line: a unit killed by a spell (MAGIC's view; WIZARDS: ReportUnitKilled) */
struct UnitKilledRecord {
    unsigned unit_type;   /* UnitTypeId (below NUM_HERO_TYPES a hero, printed by name) */
    char city;   /* the city it was in; -1 none */
    unsigned char spell;   /* the SpellId that killed it */
};   /* 0x4 */

/* Unit.enchantments as two words (no source uses the struct) */
struct EnchMask {
    unsigned lo;   /* the low word */
    unsigned hi;   /* the high word */
};   /* 0x4 */

/* an LBX file cached in EMS: ems_lbx_table[EMS_LBX_TABLE_SIZE] */
struct EmsLbxEntry {
    char name[9];   /* the LBX file's name */
    char in_reserved_pages;   /* 1: the handle lives in the reserved EMS pages */
    unsigned handle;   /* its EMS handle */
};   /* 0xc */

/* a row of the combat enchantment list: combat_effect_rows */
struct CombatEffectRow {
    int pic;   /* the picture; -1 an empty row */
    int help;   /* the help id */
    char name[26];   /* the text */
};   /* 0x1e */

/* a picture and its help id (the combat info icons) */
struct PicHelp {
    int pic;
    int help;
};   /* 0x4 */

/* an enchantment icon and its help id: combat_enchant_icon_table */
struct EnchIcon {
    int icon;   /* index into combat_enchant_pics */
    int help;
};   /* 0x4 */

/* the screen strip saved under the mouse pointer */
struct MouseUnder {
    unsigned screen_ofs;   /* the strip's offset in the VGA page (x / 4 + y x 80) */
    unsigned width;   /* its width in words (3, less at the right edge) */
    unsigned rows;   /* rows saved (16, less at the bottom edge) */
    unsigned data[597];   /* the pixels: 4 planes x width x rows words */
};   /* 0x4b0 */

/* a figure or effect to draw this battlefield frame: p_sprite_list (sorted by depth) */
struct CombatSprite {
    int x;   /* screen x */
    int y;   /* screen y */
    int pic;   /* the picture; SPRITE_FIGURE: an index into combat_sprite_pics */
    int frame;   /* the picture's frame (SPRITE_PIC) */
    int y_ofs;   /* taken off y when drawn */
    int x_ofs;   /* taken off x when drawn */
    int kind;   /* SpriteKind */
    int unread_arg8;   /* written from the caller's 8th argument, never read */
    int unread_arg9;   /* written from the 9th argument, never read */
    int unread_arg10;   /* written from the 10th argument, never read */
    int unread_arg12;   /* written from the 12th argument, never read */
    int overlay_frame;   /* SPRITE_FIGURE: the overlay's frame; 3 none drawn, < 0 no overlay */
    int overlay;   /* SPRITE_FIGURE: 0 none, else wall_rise_pics[13 + overlay] at (x - 3, y - 11) */
    long depth;   /* the draw order: grid y x 8000 + grid x x 320 + sub y x 16 + sub x (Sprite_List_Insert_Sorted); signed */
};   /* 0x1e */

/* a Magic Vortex on the battlefield: p_vortices */
struct CombatVortex {
    int x;   /* battlefield square */
    int y;   /* battlefield square */
    int dest_x;   /* the square it moves to (= x while it stands) */
    int dest_y;   /* as dest_x */
    int anim_step;   /* 0 .. 7, the move animation */
    int owner;   /* Player16 */
};   /* 0xc */

/* a missile in flight: p_missile_list */
struct CombatMissile {
    int x1;   /* start (screen) */
    int y1;   /* start (screen) */
    int x2;   /* end (screen) */
    int y2;   /* end (screen) */
    int kind;   /* MissileKind (PROJECTILE_*) */
    int progress;   /* percent of the way, from 10 to 100 */
    int direction;   /* 0 .. 7 (Atan2_Deg) */
};   /* 0xe */

/* an enemy stack the AI sees: p_ai_enemy_stacks (AI_Build_Strength_Tables) */
struct AIEnemyStack {
    char x;   /* map square */
    char y;
    char plane;   /* Plane */
    char landmass;   /* the landmass of its square */
    unsigned strength;   /* the summed unit strength */
};   /* 0x6 */

/* a remembered overland path: p_path_cache (Path_Cache_Store / _Lookup) */
struct PathCacheEntry {
    int x1;   /* the start */
    int y1;   /* the start */
    int x2;   /* the target */
    int y2;   /* the target */
    int plane;   /* Plane */
    int length;   /* steps stored */
    unsigned char path_x[35];   /* each step's x */
    unsigned char path_y[35];   /* each step's y */
    unsigned char path_cost[35];   /* each step's move cost */
    unsigned char pad_75;   /* never accessed */
};   /* 0x76 */

/* a building's click area on the city picture: p_cityscape_hotspots */
struct CityscapeHotspot {
    int building;   /* BuildingId16 */
    int x1;   /* the area */
    int y1;
    int x2;
    int y2;
    int label_x;   /* where its name is printed */
    int label_y;
};   /* 0xe */

/* the header of an LBX picture (Draw_Pic_Frame, PIC.C) */
struct LbxPicHeader {
    int width;
    int height;
    int frame;   /* the frame to draw next (every draw advances it); signed */
    int frame_count;   /* frames in the picture */
    unsigned loop_frame;   /* the frame an animation wraps back to */
    char ems_handle;   /* EMS handle; 0 in conventional memory */
    char ems_page;   /* its first EMS logical page */
    int ems_offset;   /* its offset inside those pages */
    int palette_offset;   /* an embedded palette's offset; 0 none (Pic_Load_Palette) */
    char full_frames;   /* 0: each frame draws over the previous one (decoding frame n decodes 0 .. n) */
    char remap;   /* not 0: colors 0xe8 and up are translucent through the remap tables */
};   /* 0x12 */
/* the picture header (struct LbxPicHeader) by offset; a decoded bitmap shares the first four words (its frame
   word holds PIC_BITMAP_MARKER) and keeps its remap flag at +8, its pixels from +0x10 (segment + 1) */
#define PIC_WIDTH 0
#define PIC_HEIGHT 2
#define PIC_FRAME 4
#define PIC_FRAME_COUNT 6
#define PIC_LOOP_FRAME 8          /* an LBX picture */
#define BITMAP_REMAP 8            /* a bitmap */
#define PIC_EMS_HANDLE 10
#define PIC_EMS_PAGE 11
#define PIC_EMS_OFFSET 12
#define PIC_PALETTE 14
#define PIC_FULL_FRAMES 16
#define PIC_FRAME_TABLE 0x12      /* 4 bytes per frame */
#define PIC_PIXEL_SEG(seg) ((seg) + 1)   /* the pixels: one paragraph past the 16-byte header */
#define PIC_HEADER_SIZE 16        /* a bitmap's header (its pixels follow); an LBX picture's fields before PIC_FULL_FRAMES */

/* a HELP.LBX record: *p_help_record */
struct HelpRecord {
    char title[30];   /* the title line */
    char pic_lbx[14];   /* the picture's LBX file; "" none */
    int pic_index;   /* its entry */
    int next;   /* the next record: -1 the following one, 0 the end of the chain */
    char text[1000];   /* the paragraph (Paragraph_Print, width 180) */
};   /* 0x418 */

/* a Miles AIL driver's description (music_drvr_desc, digi_drvr_desc) */
struct AILDriverDesc {
    unsigned version;   /* not read by the game */
    unsigned drvr_type;   /* AilDriverType */
    char file_suffix[4];   /* the timbre file's suffix ("FAT." + suffix) */
    unsigned long unk_08;   /* not read by either program */
    int io;   /* the port: CONFIG's value, restored when AIL_detect_device fails */
    int irq;   /* as io */
    int dma;   /* as io */
    int drq;   /* AIL_init_driver's 5th argument */
    unsigned unk_14;   /* not read */
    unsigned unk_16;   /* not read */
};   /* 0x18 */

/* a fog-of-war rule: the exploration bits a square and a neighbour get (Map_Draw_Fog's table) */
struct FogRule {
    unsigned mask;   /* the square's exploration bits (Explored) */
    unsigned neighbour_mask;   /* the neighbour's */
    unsigned add_mask;   /* ORed into the square (low byte used) */
    unsigned neighbour_add_mask;   /* ORed into the neighbour */
    int dir;   /* FOG_DIR_*: the neighbour right, below, left */
};   /* 0xa */

/* a step of the losing wizard's flight: lose_anim_path[27] */
struct LoseAnimStep {
    int x;   /* top left (Pic_Draw_Along_Line) */
    int y;   /* top left; negative in steps 7 .. 10 */
    int angle;   /* rotation, 0 .. 260 in steps of 10 */
    int scale;   /* percent for Pic_Scale_Percent (its low byte), 100 down to 5 */
};   /* 0x8 */

/* a screen area and the pointer shown over it (Mouse_Set_Cursor_Regions) */
struct MouseRegion {
    int image;   /* PointerImage */
    int pointer_offset;   /* added to the mouse x / y for hit tests (Mouse_Get_Pointer_Offset) */
    int x1;   /* the area */
    int y1;
    int x2;
    int y2;
};   /* 0xc */

/* an entry of the timbre file's index (Sound_Init) */
struct TimbreEntry {
    char patch;   /* the patch number */
    char bank;   /* the bank number; -1 the end of the index */
    long offset;   /* file offset of the timbre (a word length, then the data) */
};   /* 0x6 */

/* CONFIG.MOM, written by INSTALL: the sound hardware */
struct ConfigMom {
    int music_io;   /* Engine_Init's music port (-1 for most drivers) */
    int music_driver;   /* INSTALL's music driver id (mapped to SoundDriver) */
    int music_irq;   /* Engine_Init's music IRQ */
    int digi_io;   /* Engine_Init's digital port */
    int digi_driver;   /* INSTALL's digital driver id */
    int digi_irq;   /* Engine_Init's digital IRQ */
    int digi_dma;   /* Engine_Init's digital DMA channel */
    int unused_0e;   /* never read */
    int unused_10;   /* never read */
};   /* 0x12 */

/* a turn report line: a unit lost: report_units_lost (WIZARDS; MAGIC's UnitLostRecord) */
struct ReportUnitLost {
    int unit_type;   /* UnitTypeId */
    char reason;   /* UnitLostReason */
    unsigned char pad_03;   /* never read */
};   /* 0x4 */

/* a turn report line: a unit killed by a spell: report_units_killed */
struct ReportUnitKilled {
    int unit_type;   /* UnitTypeId; below NUM_HERO_TYPES a hero (named from p_hero_names) */
    char city;   /* the city it was in; -1 none */
    unsigned char spell;   /* the SpellId that killed it */
};   /* 0x4 */

/* a turn report line: a building lost: report_buildings_lost */
struct ReportBldgLost {
    char city;   /* the city */
    unsigned char pad_01;   /* never read */
    int building;   /* the BuildingId */
};   /* 0x4 */

typedef void (far *RedrawFn)(void);   /* the UI redraw callback (UI.C redraw_fn / saved_redraw_fn) */

#endif
