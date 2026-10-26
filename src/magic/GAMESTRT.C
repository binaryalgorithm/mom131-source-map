/* GAMESTRT.C - MAGIC.EXE code segment 522e (MAGIC only): setting up a new game - BEFORE the world is built
   (Wizards_Setup, from World_Build: the AI wizards' presets, books, retorts, banners, the spell libraries) and AFTER
   it (NewGame_Start_Init: power, relations, realms, personality and objective, heroes, items, events).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 522e src/magic/GAMESTRT.C */
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
void Wizards_Setup(void);
void NewGame_Start_Init(void);
void Items_Reset_Start(void);
void Wizards_Init_Circle_And_Mastery(void);
void Wizards_Pick_Realms_And_Traits(void);
void Wizards_Reset_Diplomacy(void);
void Wizards_Init_Magic_State(void);
void AI_Wizards_Create(void);
void Wizards_Init_Spell_Lists(void);
void Events_Reset(void);
void Heroes_Init_Start(void);
void Turn_Report_Clear(void);
void Hero_Item_Slot_Types(int hero_type, int *slots);
void AI_Wizard_Concentrate_Books(int player);
int Wizard_Casting_Skill(int player);
void Wizards_Init_Relations(void);
int Random_Weighted_Pick(unsigned *weights, int count);
#pragma warn -pro

extern struct WizardPreset wizard_presets[];    /* DS:2ad0 */
extern int default_spells[5][START_SPELL_PICKS];               /* DS:2c6a */
extern struct UnitType unit_types[];            /* DS:020c */
extern char report_outposts_deserted_count;     /* DS:61e2 */
extern struct Events far *p_events;             /* DS:61b8 */
extern char report_outposts_grown_count;        /* DS:62fb */
extern char report_buildings_lost_count;        /* DS:6362 */
extern char report_units_lost_count;            /* DS:63b4 */
extern char report_units_killed_count;          /* DS:6406 */
extern char report_city_growth_count;           /* DS:6458 */
extern char report_city_deaths_count;           /* DS:646d */
extern char report_unit_spells_lost_count;      /* DS:6482 */
extern char report_city_spells_lost_count;      /* DS:6497 */
extern char report_globals_lost;                /* DS:64ac */
extern struct HeroStats far *p_hero_stats[6];   /* DS:65ac */
extern int *p_ai_circle_timer;                  /* DS:5a1a */
extern struct Item far *p_items;                /* DS:64b0 */
extern unsigned char far *p_items_in_game;      /* DS:64b4 */
extern struct Spell far *p_spell_data;          /* DS:662c */
extern int grand_vizier;                        /* DS:6758 */
extern struct Wizard wizards[6];                /* DS:6900 */
extern int difficulty;                          /* DS:87cc */
extern int num_players;                         /* DS:87d2 */
extern struct City far *p_cities;               /* DS:89d2 */
extern struct Fortress far *p_fortresses;       /* DS:89de */
extern int start_spells[][5][START_SPELL_PICKS];               /* DS:8a42 */

int personality_weights[6][6] = {          /* DS:354c  personality draw weights by primary realm (row 5 unused); ReMoM TBL_AI_Realm_PRS, clarified using ReMoM repo */
    { 0, 1, 1, 1, 5, 2 },
    { 1, 2, 3, 1, 2, 1 },
    { 1, 2, 2, 5, 0, 0 },
    { 0, 1, 2, 0, 2, 5 },
    { 3, 3, 2, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0 }
};
int objective_weights[6][5] = {            /* DS:3594  objective draw weights by primary realm; column 0 (Pragmatist) stays 0: no AI is one; ReMoM TBL_AI_Realm_OBJ, clarified using ReMoM repo */
    { 0, 2, 2, 4, 2 },
    { 0, 1, 4, 2, 3 },
    { 0, 3, 2, 2, 3 },
    { 0, 2, 3, 4, 1 },
    { 0, 4, 1, 1, 4 },
    { 0, 0, 0, 0, 0 }
};
struct HeroTemplate hero_templates[35] = {  /* DS:35d0  one template per hero type (ReMoM _hero_premade_table; clarified using ReMoM repo) */
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_CONSTITUTION, 0, { 0, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_MIGHT, 0, { 0, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_MAGE, HERO_ABILITY_SAGE, 2, { SPELL_DISPEL_MAGIC_TRUE, SPELL_COUNTER_MAGIC, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_NOBLE, 0, { 0, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, 0, 1, { SPELL_RESIST_ELEMENTS, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_LEADERSHIP | HERO_ABILITY_FEMALE, 1, { SPELL_CONFUSION, SPELL_VERTIGO, 0, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, 0, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_MAGE, HERO_ABILITY_FEMALE, 2, { SPELL_HEALING, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_BLADEMASTER | HERO_ABILITY_FEMALE, 0, { 0, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_CHARMED | HERO_ABILITY_FEMALE | HERO_ABILITY_AGILE, 0, { 0, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, 0, 2, { SPELL_ICE_BOLT, SPELL_PETRIFY, SPELL_WEB, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_SUPER_AGILE, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_ANY, 0, 1, { SPELL_ELDRITCH_WEAPON, SPELL_SHATTER, SPELL_FLAME_BLADE, 0 } },
    { 1, HERO_ABIL_PICK_MAGE, HERO_ABILITY_ARCANE_POWER, 3, { SPELL_FIRE_BOLT, SPELL_FIREBALL, SPELL_FIRE_ELEMENTAL, SPELL_FLAME_STRIKE } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_BLADEMASTER, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_MAGE, 0, 2, { SPELL_WORD_OF_RECALL, SPELL_GUARDIAN_WIND, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_MIGHT, 1, { SPELL_RESIST_ELEMENTS, SPELL_STONE_SKIN, 0, 0 } },
    { 2, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_MIGHT, 0, { 0, 0, 0, 0 } },
    { 2, HERO_ABIL_PICK_MAGE, HERO_ABILITY_CHARMED | HERO_ABILITY_FEMALE, 3, { SPELL_DARKNESS, SPELL_MANA_LEAK, SPELL_BLACK_PRAYER, SPELL_POSSESSION } },
    { 2, HERO_ABIL_PICK_ANY, 0, 1, { 0, 0, 0, 0 } },
    { 2, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_BLADEMASTER, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_LEADERSHIP | HERO_ABILITY_SUPER_LEGENDARY, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_BLADEMASTER | HERO_ABILITY_MIGHT | HERO_ABILITY_CHARMED | HERO_ABILITY_FEMALE, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_MAGE, 0, 5, { SPELL_LIGHTNING_BOLT, SPELL_WARP_LIGHTNING, SPELL_DOOM_BOLT, 0 } },
    { 5, HERO_ABIL_PICK_ANY, 0, 1, { 0, 0, 0, 0 } },
    { 2, HERO_ABIL_PICK_MAGE, HERO_ABILITY_FEMALE, 3, { SPELL_PSIONIC_BLAST, SPELL_VERTIGO, SPELL_MIND_STORM, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_LEADERSHIP | HERO_ABILITY_LEGENDARY | HERO_ABILITY_ARMSMASTER | HERO_ABILITY_CONSTITUTION | HERO_ABILITY_MIGHT, 0, { 0, 0, 0, 0 } },
    { 0, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_ARCANE_POWER | HERO_ABILITY_SUPER_PRAYERMASTER | HERO_ABILITY_CHARMED | HERO_ABILITY_NOBLE | HERO_ABILITY_FEMALE, 4, { SPELL_DISPEL_EVIL, SPELL_HEALING, SPELL_PRAYER, SPELL_HOLY_WORD } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_LEGENDARY | HERO_ABILITY_SUPER_MIGHT | HERO_ABILITY_PRAYERMASTER, 0, { 0, 0, 0, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_LEGENDARY | HERO_ABILITY_BLADEMASTER | HERO_ABILITY_CONSTITUTION | HERO_ABILITY_MIGHT, 0, { 0, 0, 0, 0 } },
    { 3, HERO_ABIL_PICK_ANY, HERO_ABILITY_BLADEMASTER | HERO_ABILITY_FEMALE, 1, { SPELL_RESIST_MAGIC, SPELL_FLIGHT, 0, 0 } },
    { 1, HERO_ABIL_PICK_FIGHTER, HERO_ABILITY_SUPER_LEADERSHIP | HERO_ABILITY_SUPER_LEGENDARY | HERO_ABILITY_CONSTITUTION | HERO_ABILITY_NOBLE, 0, { 0, 0, 0, 0 } },
    { 2, HERO_ABIL_PICK_MAGE, HERO_ABILITY_ARCANE_POWER, 4, { SPELL_WEAKNESS, SPELL_BLACK_SLEEP, SPELL_ANIMATE_DEAD, SPELL_WRACK } },
    { 3, HERO_ABIL_PICK_ANY, HERO_ABILITY_CONSTITUTION | HERO_ABILITY_ARCANE_POWER, 3, { 0, 0, 0, 0 } },
    { 2, HERO_ABIL_PICK_ANY, HERO_ABILITY_SUPER_LEADERSHIP | HERO_ABILITY_CONSTITUTION | HERO_ABILITY_SUPER_MIGHT | HERO_ABILITY_PRAYERMASTER, 5, { SPELL_TRUE_LIGHT, SPELL_HEALING, SPELL_HOLY_ARMOR, SPELL_LIONHEART } }
};


/* () the AI wizards, the AI wizards' starting spell picks (the default lists; the human keeps the picks made on
   the spell screen), every wizard's spell library */
void Wizards_Setup(void)
{
    int i;
    int player;

    AI_Wizards_Create();
    for (player = 1; player < num_players; player++) {
        for (i = 0; i < START_SPELL_PICKS; i++) {
            start_spells[player][0][i] = default_spells[0][i];
            start_spells[player][1][i] = default_spells[1][i];
            start_spells[player][2][i] = default_spells[2][i];
            start_spells[player][3][i] = default_spells[3][i];
            start_spells[player][4][i] = default_spells[4][i];
        }
    }
    Wizards_Init_Spell_Lists();
    wizards[PLAYER_NEUTRAL].defeated_wizards[0] = 0;          /* the neutral player's record */
    wizards[PLAYER_NEUTRAL].average_unit_cost = 0;   /* the neutral player's lair timer */
}

/* () after the world is built: items, heroes, power, diplomacy, main realms, homes, the turn report; each wizard's
   home race = its capital's race; starting relations; gold: the human (5 - difficulty) x 25, the AIs 150; AI circle
   timers 81 .. 120 */
void NewGame_Start_Init(void)
{
    int i;

    Progress_Bar(100);
    Items_Reset_Start();
    Heroes_Init_Start();
    Wizards_Init_Magic_State();
    Wizards_Reset_Diplomacy();
    Wizards_Pick_Realms_And_Traits();
    Wizards_Init_Circle_And_Mastery();
    Turn_Report_Clear();
    for (i = 0; i < num_players; i++)
        wizards[i].home_race = p_cities[i].race;
    Wizards_Init_Relations();
    wizards[PLAYER_HUMAN].gold = (5 - difficulty) * 25;
    for (i = 1; i < num_players; i++) {
        wizards[i].gold = 150;
        p_ai_circle_timer[i] = Random(40) + 80;
    }
}

/* () the starting relation of every pair of wizards (both ways, also base_relation; peace timers 0): 2 x the books
   both have in Sorcery / Chaos / Nature / Life, - 2 x the larger Chaos count; + 2 x their Life books - 3 x the larger
   Death count, or - 5 x (Life + Death) when the pair has any Life book (both summed) and any Death book (the larger
   count) - which can be one wizard's; at least -90 */
void Wizards_Init_Relations(void)
{
    int death;
    int life;
    int chaos;
    int common_books;
    int books_i;
    int relation;
    int j;
    int i;

    grand_vizier = 0;                           /* the Grand Vizier option starts off in a new game */
    for (i = 0; i < num_players; i++) {
        for (j = i + 1; j < num_players; j++) {
            relation = 0;
            death = wizards[i].spellbooks[REALM_DEATH];
            if (wizards[j].spellbooks[REALM_DEATH] > death)
                death = wizards[j].spellbooks[REALM_DEATH];
            life = wizards[i].spellbooks[REALM_LIFE];
            life += wizards[j].spellbooks[REALM_LIFE];
            chaos = wizards[i].spellbooks[REALM_CHAOS];
            if (wizards[j].spellbooks[REALM_CHAOS] > chaos)
                chaos = wizards[j].spellbooks[REALM_CHAOS];
            relation = 0;
            if (life > 0 && death > 0) {
                relation -= (life + death) * 5;
            } else {
                relation += life * 2;
                relation -= death * 3;
            }
            relation -= chaos * 2;
            common_books = wizards[j].spellbooks[REALM_SORCERY];
            books_i = wizards[i].spellbooks[REALM_SORCERY];
            if (books_i < common_books)
                common_books = books_i;
            relation += common_books * 2;
            common_books = wizards[j].spellbooks[REALM_CHAOS];
            books_i = wizards[i].spellbooks[REALM_CHAOS];
            if (books_i < common_books)
                common_books = books_i;
            relation += common_books * 2;
            common_books = wizards[j].spellbooks[REALM_NATURE];
            books_i = wizards[i].spellbooks[REALM_NATURE];
            if (books_i < common_books)
                common_books = books_i;
            relation += common_books * 2;
            common_books = wizards[j].spellbooks[REALM_LIFE];
            books_i = wizards[i].spellbooks[REALM_LIFE];
            if (books_i < common_books)
                common_books = books_i;
            relation += common_books * 2;
            if (relation < -90)
                relation = -90;
            wizards[i].relationship[j] = relation;
            wizards[j].relationship[i] = relation;
            wizards[i].base_relation[j] = relation;
            wizards[j].base_relation[i] = relation;
            wizards[i].peace_turns[j] = 0;
            wizards[j].peace_turns[i] = 0;
        }
    }
}

/* () no item is in play yet */
void Items_Reset_Start(void)
{
    int i;

    for (i = 0; i < NUM_ITEMS; i++)
        p_items[i].cost = 0;
    for (i = 0; i < NUM_PREFAB_ITEMS; i++)
        p_items_in_game[i] = 0;
}

/* () each wizard's summoning circle = its fortress; the Spell of Mastery's research cost, 3000 less with 11 books of
   a realm */
void Wizards_Init_Circle_And_Mastery(void)
{
    int i;

    for (i = 0; i < num_players; i++) {
        wizards[i].summoning_circle_xyp[XYP_PLANE] = p_fortresses[i].plane;
        wizards[i].summoning_circle_xyp[XYP_X] = p_fortresses[i].x;
        wizards[i].summoning_circle_xyp[XYP_Y] = p_fortresses[i].y;
        wizards[i].mastery_research_cost = p_spell_data[SPELL_SPELL_OF_MASTERY].research_cost;
        if (wizards[i].spellbooks[REALM_NATURE] == MAX_REALM_BOOKS || wizards[i].spellbooks[REALM_SORCERY] == MAX_REALM_BOOKS || wizards[i].spellbooks[REALM_CHAOS] == MAX_REALM_BOOKS ||
                wizards[i].spellbooks[REALM_LIFE] == MAX_REALM_BOOKS || wizards[i].spellbooks[REALM_DEATH] == MAX_REALM_BOOKS)
            wizards[i].mastery_research_cost -= 3000;
    }
}

/* () every wizard's primary / secondary realm (most / next most books; ties to the earlier realm; no second realm ->
   the primary again); BUG #370: the secondary scan starts from Nature's count, so with a Nature primary the secondary
   is Nature too. Each AI also knows its realm's basic summon, on Hard / Impossible its uncommon summon becomes
   researchable (BUG #362: Death / Chaos / Sorcery test Nature's Basilisk byte, Life
   tests Nature's Path Finding slot; only Nature tests its own), and draws its personality and objective
   by weights per primary realm, shifted by its retorts (a Life primary is never Maniacal, a Death primary never
   Peaceful) */
void Wizards_Pick_Realms_And_Traits(void)
{
    int best;
    int pick_personality;
    int pick_objective;
    int primary;
    int secondary;
    int roll;
    int total;
    int personality_w[6];
    int objective_w[6];
    int realm;
    int player;

    primary = 0;
    best = wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE];
    for (realm = 1; realm < 5; realm++) {
        if (wizards[PLAYER_HUMAN].spellbooks[realm] > best) {
            primary = realm;
            best = wizards[PLAYER_HUMAN].spellbooks[realm];
        }
    }
    secondary = 0;
    best = wizards[PLAYER_HUMAN].spellbooks[REALM_NATURE];
    for (realm = 1; realm < 5; realm++) {
        if (wizards[PLAYER_HUMAN].spellbooks[realm] > best && primary != realm) {
            secondary = realm;
            best = wizards[PLAYER_HUMAN].spellbooks[realm];
        }
    }
    if (wizards[PLAYER_HUMAN].spellbooks[secondary] == 0)
        secondary = primary;
    wizards[PLAYER_HUMAN].primary_realm = primary;
    wizards[PLAYER_HUMAN].secondary_realm = secondary;
    for (player = 1; player < num_players; player++) {
        primary = 0;
        best = wizards[player].spellbooks[REALM_NATURE];
        for (realm = 1; realm < 5; realm++) {
            if (wizards[player].spellbooks[realm] > best) {
                primary = realm;
                best = wizards[player].spellbooks[realm];
            }
        }
        switch (primary) {
        case REALM_NATURE:
            wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_SPRITES)] = SPELLSTATE_KNOWN;
            break;
        case REALM_LIFE:
            wizards[player].spell_library[REALM_LIFE][SPELL_SLOT(SPELL_GUARDIAN_SPIRIT)] = SPELLSTATE_KNOWN;
            break;
        case REALM_DEATH:
            wizards[player].spell_library[REALM_DEATH][SPELL_SLOT(SPELL_GHOULS)] = SPELLSTATE_KNOWN;
            break;
        case REALM_CHAOS:
            wizards[player].spell_library[REALM_CHAOS][SPELL_SLOT(SPELL_HELL_HOUNDS)] = SPELLSTATE_KNOWN;
            break;
        case REALM_SORCERY:
            wizards[player].spell_library[REALM_SORCERY][SPELL_SLOT(SPELL_NAGAS)] = SPELLSTATE_KNOWN;
            break;
        }
        if (difficulty >= DIFFICULTY_HARD) {
            switch (primary) {
            case REALM_NATURE:
                if (wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_BASILISK)] == SPELLSTATE_UNAVAILABLE)
                    wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_BASILISK)] = SPELLSTATE_RESEARCHABLE;
                break;
            case REALM_LIFE:
                if (wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_PATH_FINDING)] == SPELLSTATE_UNAVAILABLE)
                    wizards[player].spell_library[REALM_LIFE][SPELL_SLOT(SPELL_UNICORNS)] = SPELLSTATE_RESEARCHABLE;
                break;
            case REALM_DEATH:
                if (wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_BASILISK)] == SPELLSTATE_UNAVAILABLE)
                    wizards[player].spell_library[REALM_DEATH][SPELL_SLOT(SPELL_SHADOW_DEMONS)] = SPELLSTATE_RESEARCHABLE;
                break;
            case REALM_CHAOS:
                if (wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_BASILISK)] == SPELLSTATE_UNAVAILABLE)
                    wizards[player].spell_library[REALM_CHAOS][SPELL_SLOT(SPELL_CHIMERAS)] = SPELLSTATE_RESEARCHABLE;
                break;
            case REALM_SORCERY:
                if (wizards[player].spell_library[REALM_NATURE][SPELL_SLOT(SPELL_BASILISK)] == SPELLSTATE_UNAVAILABLE)
                    wizards[player].spell_library[REALM_SORCERY][SPELL_SLOT(SPELL_PHANTOM_BEAST)] = SPELLSTATE_RESEARCHABLE;
                break;
            }
        }
        secondary = 0;
        best = wizards[player].spellbooks[REALM_NATURE];
        for (realm = 1; realm < 5; realm++) {
            if (wizards[player].spellbooks[realm] > best && primary != realm) {
                secondary = realm;
                best = wizards[player].spellbooks[realm];
            }
        }
        if (wizards[player].spellbooks[secondary] == 0)
            secondary = primary;
        wizards[player].primary_realm = primary;
        wizards[player].secondary_realm = secondary;
        for (realm = 0; realm < 6; realm++)
            personality_w[realm] = personality_weights[primary][realm];
        for (realm = 0; realm < 5; realm++)
            objective_w[realm] = objective_weights[primary][realm];
        if (wizards[player].retorts[RETORT_WARLORD] == 1) {
            personality_w[PERSONALITY_RUTHLESS] += 2;
            personality_w[PERSONALITY_AGGRESSIVE] += 3;
            objective_w[OBJECTIVE_MILITARIST] += 2;
            objective_w[OBJECTIVE_EXPANSIONIST] += 3;
        }
        if (wizards[player].retorts[RETORT_CHAOS_MASTERY] == 1) {
            personality_w[PERSONALITY_CHAOTIC] += 3;
            objective_w[OBJECTIVE_THEURGIST]++;
        }
        if (wizards[player].retorts[RETORT_NATURE_MASTERY] == 1) {
            personality_w[PERSONALITY_LAWFUL] += 3;
            objective_w[OBJECTIVE_THEURGIST]++;
        }
        if (wizards[player].retorts[RETORT_INFERNAL_POWER] == 1)
            personality_w[PERSONALITY_MANIACAL] += 4;
        if (wizards[player].retorts[RETORT_DIVINE_POWER] == 1)
            personality_w[PERSONALITY_PEACEFUL] += 4;
        if (wizards[player].retorts[RETORT_CHANNELER] == 1)
            personality_w[PERSONALITY_AGGRESSIVE] += 2;
        if (wizards[player].retorts[RETORT_ALCHEMY] == 1)
            objective_w[OBJECTIVE_PERFECTIONIST] += 2;
        if (wizards[player].retorts[RETORT_ARCHMAGE] == 1)
            objective_w[OBJECTIVE_THEURGIST] += 4;
        if (wizards[player].retorts[RETORT_MYRRAN] == 1)
            objective_w[OBJECTIVE_EXPANSIONIST] += 4;
        if (wizards[player].retorts[RETORT_CONJURER] == 1)
            objective_w[OBJECTIVE_MILITARIST] += 3;
        if (primary == REALM_LIFE)
            personality_w[PERSONALITY_MANIACAL] = 0;
        if (primary == REALM_DEATH)
            personality_w[PERSONALITY_PEACEFUL] = 0;
        total = 0;
        for (realm = 0; realm < 6; realm++)
            total += personality_w[realm];
        roll = Random(total);
        pick_personality = 0;
        roll -= personality_w[PERSONALITY_MANIACAL];
        while (roll > 0 && pick_personality < 5) {
            pick_personality++;
            roll -= personality_w[pick_personality];
        }
        total = 0;
        for (realm = 0; realm < 5; realm++)
            total += objective_w[realm];
        roll = Random(total);
        pick_objective = 0;
        roll -= objective_w[OBJECTIVE_PRAGMATIST];
        while (roll > 0 && pick_objective < 4) {
            pick_objective++;
            roll -= objective_w[pick_objective];
        }
        wizards[player].personality = pick_personality;
        wizards[player].objective = pick_objective;
    }
}

/* () per wizard: the AI timers 0, peace timers with everyone -1 (Wizards_Init_Relations then sets every pair to
   0, so only the wizard's own entry keeps it), attack target and the neighbouring unused words -1 */
void Wizards_Reset_Diplomacy(void)
{
    int i;
    int j;

    for (i = 0; i < num_players; i++) {
        wizards[i].hostility_replan_timer = 0;
        wizards[i].ai_strategy_timer = 0;
        wizards[i].ai_magic_timer = 0;
        for (j = 0; j < num_players; j++)
            wizards[i].peace_turns[j] = -1;
        wizards[i].unused_4b6[0] = -1;
        wizards[i].unused_4b6[1] = -1;
        wizards[i].ai_attack_target = -1;
        wizards[i].unused_4bc[0] = -1;
    }
}

/* () per wizard: fame 10 with Famous; casting skill raised a point at a time until it reaches 2 x (books, + 5 with
   Archmage); no research or casting, mana 0, sliders 34 / 33 / 33 %, the Spell of Mastery entry 3, 21 of the 24
   global enchantments cleared (Planar Seal, Charm of Life and Awareness are not touched), tax 2, empty vault
   and no heroes */
void Wizards_Init_Magic_State(void)
{
    int k;
    int i;
    int target_skill;

    for (i = 0; i < num_players; i++) {
        if (wizards[i].retorts[RETORT_FAMOUS] == 1)
            wizards[i].fame = 10;
        else
            wizards[i].fame = 0;
        target_skill = wizards[i].spellbooks[REALM_NATURE] + wizards[i].spellbooks[REALM_SORCERY] + wizards[i].spellbooks[REALM_CHAOS] +
               wizards[i].spellbooks[REALM_LIFE] + wizards[i].spellbooks[REALM_DEATH];
        if (wizards[i].retorts[RETORT_ARCHMAGE] != 0)
            target_skill += 5;
        target_skill += target_skill;
        wizards[i].nominal_casting_skill = 0;
        wizards[i].skill_points = 0;
        while (wizards[i].nominal_casting_skill < target_skill) {
            wizards[i].skill_points++;
            wizards[i].nominal_casting_skill = Wizard_Casting_Skill(i);
        }
        wizards[i].research_spell = 0;
        wizards[i].mana_reserve = 0;
        wizards[i].research_left = 0;
        wizards[i].skill_percentage = 33;
        wizards[i].mana_percentage = 33;
        wizards[i].research_percentage = 34;
        wizards[i].spell_library[REALM_ARCANE][SPELL_SLOT(SPELL_SPELL_OF_MASTERY)] = SPELLSTATE_OFFERED;
        wizards[i].casting_spell = 0;
        wizards[i].casting_cost_left = 0;
        wizards[i].globals.eternal_night = 0;
        wizards[i].globals.evil_omens = 0;
        wizards[i].globals.zombie_mastery = 0;
        wizards[i].globals.aura_of_majesty = 0;
        wizards[i].globals.wind_mastery = 0;
        wizards[i].globals.suppress_magic = 0;
        wizards[i].globals.time_stop = 0;
        wizards[i].globals.nature_awareness = 0;
        wizards[i].globals.nature_s_wrath = 0;
        wizards[i].globals.herb_mastery = 0;
        wizards[i].globals.chaos_surge = 0;
        wizards[i].globals.doom_mastery = 0;
        wizards[i].globals.great_wasting = 0;
        wizards[i].globals.meteor_storm = 0;
        wizards[i].globals.armageddon = 0;
        wizards[i].globals.tranquility = 0;
        wizards[i].globals.life_force = 0;
        wizards[i].globals.crusade = 0;
        wizards[i].globals.just_cause = 0;
        wizards[i].globals.holy_arms = 0;
        wizards[i].volcanoes = 0;
        wizards[i].globals.detect_magic = 0;
        wizards[i].tax_rate = 2;
        wizards[i].vault_items[0] = -1;      /* -1: an empty vault slot */
        wizards[i].vault_items[1] = -1;
        wizards[i].vault_items[2] = -1;
        wizards[i].vault_items[3] = -1;
        for (k = 0; k < 6; k++)
            wizards[i].hired_heroes[k].unit_index = -1;
    }
}

/* () the AI wizards: distinct random presets (portrait, name, books, retort; Hard+: 1 in 5 get their books
   concentrated), a difficulty-dependent number of random reductions (a book less, or the retorts cleared - BUG #363:
   that clears one byte, retorts[k] with k the reduction counter, 18 times), then picks added up to 11 / 13 (Hard) / 15
   (Impossible) - Myrran costs 3, Warlord / Infernal / Divine / Channeler / Famous 2, others 1: 2 .. 5 books of a realm
   (Life only without Death and back, at most 11 per realm, 12 in all) or a retort (a realm mastery weighted by that
   realm's books); restarts unless exactly 1 or 2 AIs have Myrran (Hard+, 4+ players); then the banner colors */
void AI_Wizards_Create(void)
{
    int j;
    int taken;
    int preset;
    int count;
    int pick;
    int picks;
    int n;                                      /* books to add, then (from the retort picks on) the retort index */
    int color;
    int books;
    char *retorts;
    int banner_taken[5];
    int target;
    int myrran;
    int realm_weights[5];
    int realm;
    int sum;
    int k;
    int i;

    for (k = 0; k < 5; k++)
        banner_taken[k] = 0;
    banner_taken[wizards[PLAYER_HUMAN].banner_color] = 1;
again:
    for (i = 1; i < num_players; i++) {
retry:
        taken = 0;
        preset = Random(NUM_WIZARD_PORTRAITS) - 1;
        for (j = 0; j < i; j++) {
            if (wizards[j].portrait == preset)
                taken = 1;
        }
        if (taken == 1)
            goto retry;
        wizards[i].portrait = preset;
    }
    for (i = 1; i < num_players; i++)
        strcpy(wizards[i].wizard_name, wizard_presets[wizards[i].portrait].name);
    for (i = 1; i < num_players; i++) {
        wizards[i].spellbooks[REALM_LIFE] = wizard_presets[wizards[i].portrait].books_life;
        wizards[i].spellbooks[REALM_DEATH] = wizard_presets[wizards[i].portrait].books_death;
        wizards[i].spellbooks[REALM_CHAOS] = wizard_presets[wizards[i].portrait].books_chaos;
        wizards[i].spellbooks[REALM_NATURE] = wizard_presets[wizards[i].portrait].books_nature;
        wizards[i].spellbooks[REALM_SORCERY] = wizard_presets[wizards[i].portrait].books_sorcery;
        if (Random(5) == 1 && difficulty > DIFFICULTY_NORMAL)
            AI_Wizard_Concentrate_Books(i);
        retorts = wizards[i].retorts;
        for (k = 0; k < 18; k++)
            retorts[k] = 0;
        if (wizard_presets[wizards[i].portrait].retort != -1)
            retorts[wizard_presets[wizards[i].portrait].retort] = 1;
    }
    for (i = 1; i < num_players; i++) {
        switch (difficulty) {
        case DIFFICULTY_INTRO:
            count = 0;
            break;
        case DIFFICULTY_EASY:
            count = 0;
            break;
        case DIFFICULTY_NORMAL:
            count = Random(3);
            break;
        case DIFFICULTY_HARD:
            count = Random(3) + Random(3);
            break;
        case DIFFICULTY_IMPOSSIBLE:
            count = Random(5) + Random(5);
            break;
        case 5:   /* no such setting (the menu stops at DIFFICULTY_IMPOSSIBLE): never taken */
            count = Random(8) + Random(8);
            break;
        }
        retorts = wizards[i].retorts;
        for (k = 0; k < count; k++) {
            pick = Random(6);
            switch (pick) {
            case 1:
                if (wizards[i].spellbooks[REALM_LIFE] > 1)
                    wizards[i].spellbooks[REALM_LIFE]--;
                break;
            case 2:
                if (wizards[i].spellbooks[REALM_DEATH] > 1)
                    wizards[i].spellbooks[REALM_DEATH]--;
                break;
            case 3:
                if (wizards[i].spellbooks[REALM_CHAOS] > 1)
                    wizards[i].spellbooks[REALM_CHAOS]--;
                break;
            case 4:
                if (wizards[i].spellbooks[REALM_NATURE] > 1)
                    wizards[i].spellbooks[REALM_NATURE]--;
                break;
            case 5:
                if (wizards[i].spellbooks[REALM_SORCERY] > 1)
                    wizards[i].spellbooks[REALM_SORCERY]--;
                break;
            case 6:
                retorts = wizards[i].retorts;
                for (j = 0; j < 18; j++)
                    retorts[k] = 0;
                break;
            }
        }
    }
    for (i = 1; i < num_players; i++) {
        retorts = wizards[i].retorts;
        picks = wizards[i].spellbooks[REALM_LIFE] + wizards[i].spellbooks[REALM_DEATH] + wizards[i].spellbooks[REALM_NATURE] +
                wizards[i].spellbooks[REALM_CHAOS] + wizards[i].spellbooks[REALM_SORCERY];
        books = picks;
        for (k = 0; k < 18; k++) {
            if (retorts[k] != 0) {
                if (k == RETORT_MYRRAN)
                    picks += 3;
                else if (k == RETORT_WARLORD || k == RETORT_INFERNAL_POWER || k == RETORT_DIVINE_POWER ||
                         k == RETORT_FAMOUS || k == RETORT_CHANNELER)
                    picks += 2;
                else
                    picks++;
            }
        }
        target = 11;
        if (difficulty == DIFFICULTY_HARD)
            target = 13;
        if (difficulty == DIFFICULTY_IMPOSSIBLE)
            target = 15;
        while (picks < target) {
            pick = Random(8);   /* 1 .. 5 a book (Life, Death, Nature, Sorcery, Chaos), 6 .. 8 a retort */
            n = Random(4) + 1;
            if (n + picks > target)
                n = target - picks;
            if (n + books > 12)
                n = 12 - books;
            if (n <= 0)
                pick = 6;
            if (pick == 6)
                n = 1;
            switch (pick) {
            case 1:
                if (wizards[i].spellbooks[REALM_LIFE] < MAX_REALM_BOOKS && wizards[i].spellbooks[REALM_DEATH] == 0)
                    wizards[i].spellbooks[REALM_LIFE] += n;
                if (wizards[i].spellbooks[REALM_LIFE] > MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_LIFE] = MAX_REALM_BOOKS;
                break;
            case 2:
                if (wizards[i].spellbooks[REALM_DEATH] < MAX_REALM_BOOKS && wizards[i].spellbooks[REALM_LIFE] == 0)
                    wizards[i].spellbooks[REALM_DEATH] += n;
                if (wizards[i].spellbooks[REALM_DEATH] > MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_DEATH] = MAX_REALM_BOOKS;
                break;
            case 3:
                if (wizards[i].spellbooks[REALM_NATURE] < MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_NATURE] += n;
                if (wizards[i].spellbooks[REALM_NATURE] > MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_NATURE] = MAX_REALM_BOOKS;
                break;
            case 4:
                if (wizards[i].spellbooks[REALM_SORCERY] < MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_SORCERY] += n;
                if (wizards[i].spellbooks[REALM_SORCERY] > MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_SORCERY] = MAX_REALM_BOOKS;
                break;
            case 5:
                if (wizards[i].spellbooks[REALM_CHAOS] < MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_CHAOS] += n;
                if (wizards[i].spellbooks[REALM_CHAOS] > MAX_REALM_BOOKS)
                    wizards[i].spellbooks[REALM_CHAOS] = MAX_REALM_BOOKS;
                break;
            case 6:
            case 7:
            case 8:
                n = Random(18) - 1;             /* n now the retort index */
                if (n >= RETORT_CHAOS_MASTERY && n <= RETORT_DIVINE_POWER) {
                    for (realm = 0; realm <= 4; realm++)
                        realm_weights[realm] = wizards[i].spellbooks[realm];
                    if (wizards[i].retorts[RETORT_CHAOS_MASTERY] > 0)
                        realm_weights[REALM_CHAOS] = 0;
                    if (wizards[i].retorts[RETORT_NATURE_MASTERY] > 0)
                        realm_weights[REALM_NATURE] = 0;
                    if (wizards[i].retorts[RETORT_SORCERY_MASTERY] > 0)
                        realm_weights[REALM_SORCERY] = 0;
                    if (wizards[i].retorts[RETORT_DIVINE_POWER] > 0 || target - 2 < picks)
                        realm_weights[REALM_LIFE] = 0;
                    if (wizards[i].retorts[RETORT_INFERNAL_POWER] > 0 || target - 2 < picks)
                        realm_weights[REALM_DEATH] = 0;
                    sum = 0;
                    for (realm = 0; realm <= 4; realm++)
                        sum += realm_weights[realm];
                    if (sum > 0) {
                        n = Random_Weighted_Pick(realm_weights, 5) + 2;   /* realm + 2 -> that realm's retort */
                        if (n == REALM_NATURE + 2)
                            n = RETORT_NATURE_MASTERY;
                        else if (n == REALM_SORCERY + 2)
                            n = RETORT_SORCERY_MASTERY;
                        else if (n == REALM_CHAOS + 2)
                            n = RETORT_CHAOS_MASTERY;
                        else if (n == REALM_LIFE + 2)
                            n = RETORT_DIVINE_POWER;
                        else if (n == REALM_DEATH + 2)
                            n = RETORT_INFERNAL_POWER;
                        retorts[n] = 1;
                    }
                } else if (n == RETORT_WARLORD || n == RETORT_CHANNELER || n == RETORT_FAMOUS) {
                    if (target - 1 > picks)
                        retorts[n] = 1;
                } else if (n == RETORT_MYRRAN) {
                    if (target - 2 > picks)
                        retorts[n] = 1;
                } else {
                    retorts[n] = 1;
                }
                break;
            }
            picks = wizards[i].spellbooks[REALM_LIFE] + wizards[i].spellbooks[REALM_DEATH] + wizards[i].spellbooks[REALM_NATURE] +
                    wizards[i].spellbooks[REALM_CHAOS] + wizards[i].spellbooks[REALM_SORCERY];
            books = picks;
            for (k = 0; k < 18; k++) {
                if (retorts[k] != 0) {
                    if (k == RETORT_MYRRAN)
                        picks += 3;
                    else if (k == RETORT_WARLORD || k == RETORT_INFERNAL_POWER || k == RETORT_DIVINE_POWER ||
                             k == RETORT_FAMOUS || k == RETORT_CHANNELER)
                        picks += 2;
                    else
                        picks++;
                }
            }
        }
    }
    myrran = 0;
    for (k = 1; k < num_players; k++) {
        if (wizards[k].retorts[RETORT_MYRRAN] != 0)
            myrran++;
    }
    if ((myrran < 1 || myrran > 2) && difficulty > DIFFICULTY_NORMAL && num_players > 3) {
        goto again;
        /* never reached (after the goto): */
        wizard_presets[7].retort = -1;
        wizard_presets[7].books_chaos = 7;
    }
    for (i = 1; i < num_players; i++) {
        if (wizards[i].spellbooks[REALM_NATURE] > 3 && banner_taken[BANNER_GREEN] == 0) {
            wizards[i].banner_color = BANNER_GREEN;
            banner_taken[BANNER_GREEN] = 1;
        } else if (wizards[i].spellbooks[REALM_CHAOS] > 3 && banner_taken[BANNER_RED] == 0) {
            wizards[i].banner_color = BANNER_RED;
            banner_taken[BANNER_RED] = 1;
        } else if (wizards[i].spellbooks[REALM_SORCERY] > 3 && banner_taken[BANNER_BLUE] == 0) {
            wizards[i].banner_color = BANNER_BLUE;
            banner_taken[BANNER_BLUE] = 1;
        } else if (wizards[i].spellbooks[REALM_DEATH] > 3 && banner_taken[BANNER_PURPLE] == 0) {
            wizards[i].banner_color = BANNER_PURPLE;
            banner_taken[BANNER_PURPLE] = 1;
        } else {
pick_color:
            color = Random(5) - 1;
            if (banner_taken[color] == 1)
                goto pick_color;
            banner_taken[color] = 1;
            wizards[i].banner_color = color;
        }
    }
}


/* () per wizard, the spell library (0 not in the books, 1 researchable, 2 known): all 240 cleared; Arcane: the first
   12 researchable, Spell of Return known; per realm with books - 11: all 40 researchable, the 10 commons and the
   start picks' 2 uncommons and rare known; fewer: the first (books - 1) start picks known, commons researchable at
   random until (books -> 3 5 6 7 8 9 10 ..) are in the library, uncommons (1 2 3 4 5 6 8 10 ..; 8+ books all),
   rares (0 1 2 3 4 5 6 7 9; 10+ all) and very rares (0 0 1 2 3 4 5 6 7; 10+ all) researchable at random. Then
   Magic Spirit known, and with Artificer Enchant Item and Create Artifact */
void Wizards_Init_Spell_Lists(void)
{
    int player;
    int realm;
    int k;
    int count;
    int pick;
    int j;
    int have;
    int taken[10];
    int i;

    for (player = 0; player < num_players; player++) {
        for (realm = 0; realm < NUM_REALMS; realm++)
            for (k = 0; k < SPELLS_PER_REALM; k++)
                wizards[player].spell_library[realm][k] = SPELLSTATE_UNAVAILABLE;
        for (realm = 0; realm < 6; realm++) {
            if (realm == REALM_ARCANE) {
                wizards[player].spell_library[realm][0] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][1] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][2] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][3] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][4] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][5] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][6] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][7] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][8] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][9] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][10] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][11] = SPELLSTATE_RESEARCHABLE;
                wizards[player].spell_library[realm][SPELL_SLOT(SPELL_SPELL_OF_RETURN)] = SPELLSTATE_KNOWN;
            } else if (wizards[player].spellbooks[realm] != 0) {
                if (wizards[player].spellbooks[realm] == MAX_REALM_BOOKS) {
                    for (i = 0; i < SPELLS_PER_REALM; i++)
                        wizards[player].spell_library[realm][i] = SPELLSTATE_RESEARCHABLE;
                    switch (realm) {
                    case REALM_NATURE:
                        for (j = 0; j < 10; j++)
                            wizards[player].spell_library[realm][j] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_NATURE][10])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_NATURE][11])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_NATURE][12])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_DEATH:
                        for (j = 0; j < 10; j++)
                            wizards[player].spell_library[realm][j] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_DEATH][10])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_DEATH][11])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_DEATH][12])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_CHAOS:
                        for (j = 0; j < 10; j++)
                            wizards[player].spell_library[realm][j] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_CHAOS][10])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_CHAOS][11])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_CHAOS][12])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_LIFE:
                        for (j = 0; j < 10; j++)
                            wizards[player].spell_library[realm][j] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_LIFE][10])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_LIFE][11])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_LIFE][12])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_SORCERY:
                        for (j = 0; j < 10; j++)
                            wizards[player].spell_library[realm][j] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_SORCERY][10])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_SORCERY][11])] = SPELLSTATE_KNOWN;
                        wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_SORCERY][12])] = SPELLSTATE_KNOWN;
                        break;
                    }
                } else {
                    for (i = 0; i < 10; i++)
                        taken[i] = 0;
                    switch (wizards[player].spellbooks[realm]) {
                    case 1:
                        count = 3;
                        break;
                    case 2:
                        count = 5;
                        break;
                    case 3:
                        count = 6;
                        break;
                    case 4:
                        count = 7;
                        break;
                    case 5:
                        count = 8;
                        break;
                    case 6:
                        count = 9;
                        break;
                    case 7:
                        count = 10;
                        break;
                    case 8:
                        count = 10;
                        break;
                    case 9:
                        count = 10;
                        break;
                    case 10:
                        count = 10;
                        break;
                    }
                    switch (realm) {
                    case REALM_NATURE:
                        for (i = 0; i < wizards[player].spellbooks[realm] - 1; i++)
                            wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_NATURE][i])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_LIFE:
                        for (i = 0; i < wizards[player].spellbooks[realm] - 1; i++)
                            wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_LIFE][i])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_DEATH:
                        for (i = 0; i < wizards[player].spellbooks[realm] - 1; i++)
                            wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_DEATH][i])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_CHAOS:
                        for (i = 0; i < wizards[player].spellbooks[realm] - 1; i++)
                            wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_CHAOS][i])] = SPELLSTATE_KNOWN;
                        break;
                    case REALM_SORCERY:
                        for (i = 0; i < wizards[player].spellbooks[realm] - 1; i++)
                            wizards[player].spell_library[realm][SPELL_SLOT(start_spells[player][REALM_SORCERY][i])] = SPELLSTATE_KNOWN;
                        break;
                    }
                    have = wizards[player].spellbooks[realm] - 1;
                    while (have < count) {
                        i = Random(10) - 1;
                        if (wizards[player].spell_library[realm][i] == SPELLSTATE_UNAVAILABLE)
                            wizards[player].spell_library[realm][i] = SPELLSTATE_RESEARCHABLE;
                        have = 0;
                        for (i = 0; i < 10; i++) {
                            if (wizards[player].spell_library[realm][i] != SPELLSTATE_UNAVAILABLE)
                                have++;
                        }
                    }
                    if (wizards[player].spellbooks[realm] > 7) {
                        for (i = 0; i < 10; i++)
                            wizards[player].spell_library[realm][i + SPELL_SLOT_UNCOMMON] = SPELLSTATE_RESEARCHABLE;
                    } else {
                        for (i = 0; i < 10; i++)
                            taken[i] = 0;
                        switch (wizards[player].spellbooks[realm]) {
                        case 1:
                            count = 1;
                            break;
                        case 2:
                            count = 2;
                            break;
                        case 3:
                            count = 3;
                            break;
                        case 4:
                            count = 4;
                            break;
                        case 5:
                            count = 5;
                            break;
                        case 6:
                            count = 6;
                            break;
                        case 7:
                            count = 8;
                            break;
                        case 8:
                            count = 10;
                            break;
                        case 9:
                            count = 10;
                            break;
                        case 10:
                            count = 10;
                            break;
                        }
                        for (i = 0; i < count; i++) {
pick_uncommon:
                            pick = Random(10) - 1;
                            if (taken[pick] == 1)
                                goto pick_uncommon;
                            taken[pick] = 1;
                        }
                        for (i = 0; i < 10; i++) {
                            if (taken[i] == 1)
                                wizards[player].spell_library[realm][i + SPELL_SLOT_UNCOMMON] = SPELLSTATE_RESEARCHABLE;
                        }
                    }
                    if (wizards[player].spellbooks[realm] > 9) {
                        for (i = 0; i < 10; i++)
                            wizards[player].spell_library[realm][i + SPELL_SLOT_RARE] = SPELLSTATE_RESEARCHABLE;
                    } else {
                        for (i = 0; i < 10; i++)
                            taken[i] = 0;
                        switch (wizards[player].spellbooks[realm]) {
                        case 1:
                            count = 0;
                            break;
                        case 2:
                            count = 1;
                            break;
                        case 3:
                            count = 2;
                            break;
                        case 4:
                            count = 3;
                            break;
                        case 5:
                            count = 4;
                            break;
                        case 6:
                            count = 5;
                            break;
                        case 7:
                            count = 6;
                            break;
                        case 8:
                            count = 7;
                            break;
                        case 9:
                            count = 9;
                            break;
                        }
                        for (i = 0; i < count; i++) {
pick_rare:
                            pick = Random(10) - 1;
                            if (taken[pick] == 1)
                                goto pick_rare;
                            taken[pick] = 1;
                        }
                        for (i = 0; i < 10; i++) {
                            if (taken[i] == 1)
                                wizards[player].spell_library[realm][i + SPELL_SLOT_RARE] = SPELLSTATE_RESEARCHABLE;
                        }
                    }
                    if (wizards[player].spellbooks[realm] > 9) {
                        for (i = 0; i < 10; i++)
                            wizards[player].spell_library[realm][i + SPELL_SLOT_VERY_RARE] = SPELLSTATE_RESEARCHABLE;
                    } else {
                        for (i = 0; i < 10; i++)
                            taken[i] = 0;
                        switch (wizards[player].spellbooks[realm]) {
                        case 1:
                            count = 0;
                            break;
                        case 2:
                            count = 0;
                            break;
                        case 3:
                            count = 1;
                            break;
                        case 4:
                            count = 2;
                            break;
                        case 5:
                            count = 3;
                            break;
                        case 6:
                            count = 4;
                            break;
                        case 7:
                            count = 5;
                            break;
                        case 8:
                            count = 6;
                            break;
                        case 9:
                            count = 7;
                            break;
                        }
                        for (i = 0; i < count; i++) {
pick_very_rare:
                            pick = Random(10) - 1;
                            if (taken[pick] == 1)
                                goto pick_very_rare;
                            taken[pick] = 1;
                        }
                        for (i = 0; i < 10; i++) {
                            if (taken[i] == 1)
                                wizards[player].spell_library[realm][i + SPELL_SLOT_VERY_RARE] = SPELLSTATE_RESEARCHABLE;
                        }
                    }
                }
            }
        }
        wizards[player].spell_library[REALM_ARCANE][SPELL_SLOT(SPELL_MAGIC_SPIRIT)] = SPELLSTATE_KNOWN;
        if (wizards[player].retorts[RETORT_ARTIFICER] != 0) {
            wizards[player].spell_library[REALM_ARCANE][SPELL_SLOT(SPELL_ENCHANT_ITEM)] = SPELLSTATE_KNOWN;
            wizards[player].spell_library[REALM_ARCANE][SPELL_SLOT(SPELL_CREATE_ARTIFACT)] = SPELLSTATE_KNOWN;
        }
    }
}

/* () the random-events record: no event before turn 50, every event inactive */
void Events_Reset(void)
{
    p_events->last_event_turn = 50;
    p_events->meteor = EVENT_STATE_NONE;
    p_events->gift = EVENT_STATE_NONE;
    p_events->disjunction = EVENT_STATE_NONE;
    p_events->marriage = EVENT_STATE_NONE;
    p_events->earthquake = EVENT_STATE_NONE;
    p_events->pirates = EVENT_STATE_NONE;
    p_events->plague = EVENT_STATE_NONE;
    p_events->rebellion = EVENT_STATE_NONE;
    p_events->donation = EVENT_STATE_NONE;
    p_events->depletion = EVENT_STATE_NONE;
    p_events->new_minerals = EVENT_STATE_NONE;
    p_events->population_boom = EVENT_STATE_NONE;
    p_events->good_moon = EVENT_STATE_NONE;
    p_events->bad_moon = EVENT_STATE_NONE;
    p_events->conjunction_chaos = EVENT_STATE_NONE;
    p_events->conjunction_nature = EVENT_STATE_NONE;
    p_events->conjunction_sorcery = EVENT_STATE_NONE;
    p_events->mana_short = EVENT_STATE_NONE;
}

/* () every player's 35 hero records (the neutral player has none): not hired, then the hero's template with its
   random picks rolled - Random(14) picks an ability, a second pick of it makes it Super. Fighter picks: Leadership,
   Legendary, Armsmaster, Blademaster, Might, Constitution, Agility; mage picks: Arcane Power (never for the Knight,
   UNIT_HERO_KNIGHT; Super only with a magic ranged attack), Prayermaster, Sage, a caster level; either: Noble (not the Chosen, UNIT_HERO_CHOSEN),
   Charmed (not a unit type with Magic Immunity), Lucky. Every pick taken counts against both counters. BUG #364: Lucky
   has no break and also rolls Agility */
void Heroes_Init_Start(void)
{
    int player;
    int hero;
    int level;
    int mage;
    int any;
    unsigned long abilities;
    int fighter;

    for (player = 0; player < PLAYER_NEUTRAL; player++) {
        for (hero = 0; hero < NUM_HERO_TYPES; hero++) {
            p_hero_stats[player][hero].status = 0;
            fighter = 0;
            mage = 0;
            any = 0;
            switch (hero_templates[hero].pick_kind) {
            case HERO_ABIL_PICK_FIGHTER:
                fighter = hero_templates[hero].picks;
                break;
            case HERO_ABIL_PICK_MAGE:
                mage = hero_templates[hero].picks;
                break;
            case HERO_ABIL_PICK_ANY:
                any = hero_templates[hero].picks;
                break;
            }
            abilities = hero_templates[hero].abilities;
            level = hero_templates[hero].caster_level;
            if (any > 0) {
                fighter = any;
                mage = any;
            }
            while (fighter > 0 || mage > 0) {
                switch (Random(14) - 1) {   /* an ability to add or raise to its super form; 8 = caster level + 1 */
                case 0:
                    if ((abilities & HERO_ABILITY_SUPER_LEADERSHIP) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_LEADERSHIP) != 0) {
                            abilities = abilities ^ HERO_ABILITY_LEADERSHIP;
                            abilities = abilities | HERO_ABILITY_SUPER_LEADERSHIP;
                        } else
                            abilities = abilities | HERO_ABILITY_LEADERSHIP;
                        fighter--;
                        mage--;
                    }
                    break;
                case 1:
                    if ((abilities & HERO_ABILITY_SUPER_LEGENDARY) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_LEGENDARY) != 0) {
                            abilities = abilities ^ HERO_ABILITY_LEGENDARY;
                            abilities = abilities | HERO_ABILITY_SUPER_LEGENDARY;
                        } else
                            abilities = abilities | HERO_ABILITY_LEGENDARY;
                        fighter--;
                        mage--;
                    }
                    break;
                case 2:
                    if ((abilities & HERO_ABILITY_SUPER_ARMSMASTER) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_ARMSMASTER) != 0) {
                            abilities = abilities ^ HERO_ABILITY_ARMSMASTER;
                            abilities = abilities | HERO_ABILITY_SUPER_ARMSMASTER;
                        } else
                            abilities = abilities | HERO_ABILITY_ARMSMASTER;
                        fighter--;
                        mage--;
                    }
                    break;
                case 3:
                    if ((abilities & HERO_ABILITY_SUPER_BLADEMASTER) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_BLADEMASTER) != 0) {
                            abilities = abilities ^ HERO_ABILITY_BLADEMASTER;
                            abilities = abilities | HERO_ABILITY_SUPER_BLADEMASTER;
                        } else
                            abilities = abilities | HERO_ABILITY_BLADEMASTER;
                        fighter--;
                        mage--;
                    }
                    break;
                case 4:
                    if ((abilities & HERO_ABILITY_SUPER_MIGHT) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_MIGHT) != 0) {
                            abilities = abilities ^ HERO_ABILITY_MIGHT;
                            abilities = abilities | HERO_ABILITY_SUPER_MIGHT;
                        } else
                            abilities = abilities | HERO_ABILITY_MIGHT;
                        fighter--;
                        mage--;
                    }
                    break;
                case 5:
                    if ((abilities & HERO_ABILITY_SUPER_CONSTITUTION) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_CONSTITUTION) != 0) {
                            abilities = abilities ^ HERO_ABILITY_CONSTITUTION;
                            abilities = abilities | HERO_ABILITY_SUPER_CONSTITUTION;
                        } else
                            abilities = abilities | HERO_ABILITY_CONSTITUTION;
                        fighter--;
                        mage--;
                    }
                    break;
                case 6:
                    if ((abilities & HERO_ABILITY_SUPER_ARCANE_POWER) == 0 && mage > 0 && hero != UNIT_HERO_KNIGHT) {
                        if ((abilities & HERO_ABILITY_ARCANE_POWER) != 0 && unit_types[hero].ranged_type >= RANGED_MAGIC_FIRE &&   /* a magic attack, not RANGED_MAGIC_LIGHTNING */
                                unit_types[hero].ranged_type <= RANGED_MAGIC_NATURE) {
                            abilities = abilities ^ HERO_ABILITY_ARCANE_POWER;
                            abilities = abilities | HERO_ABILITY_SUPER_ARCANE_POWER;
                        } else                  /* a second pick without a magic attack is spent for nothing */
                            abilities = abilities | HERO_ABILITY_ARCANE_POWER;
                        fighter--;
                        mage--;
                    }
                    break;
                case 7:
                    if ((abilities & HERO_ABILITY_SUPER_PRAYERMASTER) == 0 && mage > 0) {
                        if ((abilities & HERO_ABILITY_PRAYERMASTER) != 0) {
                            abilities = abilities ^ HERO_ABILITY_PRAYERMASTER;
                            abilities = abilities | HERO_ABILITY_SUPER_PRAYERMASTER;
                        } else
                            abilities = abilities | HERO_ABILITY_PRAYERMASTER;
                        fighter--;
                        mage--;
                    }
                    break;
                case 8:
                    if (mage > 0) {
                        level++;
                        fighter--;
                        mage--;
                    }
                    break;
                case 9:
                    if ((abilities & HERO_ABILITY_NOBLE) == 0 && hero != UNIT_HERO_CHOSEN) {
                        abilities = abilities | HERO_ABILITY_NOBLE;
                        fighter--;
                        mage--;
                    }
                    break;
                case 10:
                    if ((abilities & HERO_ABILITY_CHARMED) == 0 && (unit_types[hero].abilities & ABILITY_MAGIC_IMMUNITY) == 0) {
                        abilities = abilities | HERO_ABILITY_CHARMED;
                        fighter--;
                        mage--;
                    }
                    break;
                case 11:
                    if ((abilities & HERO_ABILITY_LUCKY) == 0) {
                        abilities = abilities | HERO_ABILITY_LUCKY;
                        fighter--;
                        mage--;
                    }
                case 12:
                    if ((abilities & HERO_ABILITY_SUPER_AGILE) == 0 && fighter > 0) {
                        if ((abilities & HERO_ABILITY_AGILE) != 0) {
                            abilities = abilities ^ HERO_ABILITY_AGILE;
                            abilities = abilities | HERO_ABILITY_SUPER_AGILE;
                        } else
                            abilities = abilities | HERO_ABILITY_AGILE;
                        fighter--;
                        mage--;
                    }
                    break;
                case 13:
                    if ((abilities & HERO_ABILITY_SUPER_SAGE) == 0 && mage > 0) {
                        if ((abilities & HERO_ABILITY_SAGE) != 0) {
                            abilities = abilities ^ HERO_ABILITY_SAGE;
                            abilities = abilities | HERO_ABILITY_SUPER_SAGE;
                        } else
                            abilities = abilities | HERO_ABILITY_SAGE;
                        fighter--;
                        mage--;
                    }
                    break;
                }
            }
            p_hero_stats[player][hero].abilities = abilities;
            p_hero_stats[player][hero].caster_level = level;
            p_hero_stats[player][hero].spells[0] = hero_templates[hero].spells[0];
            p_hero_stats[player][hero].spells[1] = hero_templates[hero].spells[1];
            p_hero_stats[player][hero].spells[2] = hero_templates[hero].spells[2];
            p_hero_stats[player][hero].spells[3] = hero_templates[hero].spells[3];
        }
    }
}

/* () the turn report's counters */
void Turn_Report_Clear(void)
{
    report_units_lost_count = 0;
    report_units_killed_count = 0;
    report_unit_spells_lost_count = 0;
    report_city_spells_lost_count = 0;
    report_globals_lost = 0;
    report_city_growth_count = 0;
    report_city_deaths_count = 0;
    report_buildings_lost_count = 0;
    report_outposts_deserted_count = 0;
    report_outposts_grown_count = 0;
}

/* (hero_type, int slots[3]) the hero's three item-slot types by its item class (UnitType.hero_item_class, i.e. req_building_2):
   warrior-mage: melee or magic, armor, misc; mage: magic, misc, misc; bowman: bow, armor, misc; else melee, armor,
   misc */
void Hero_Item_Slot_Types(int hero_type, int *slots)
{
    if (unit_types[hero_type].hero_item_class == HERO_ITEMS_WARRIOR_MAGE) {
        slots[0] = ITEM_SLOT_MELEE_OR_MAGIC;
        slots[1] = ITEM_SLOT_ARMOR;
        slots[2] = ITEM_SLOT_MISC;
    } else if (unit_types[hero_type].hero_item_class == HERO_ITEMS_MAGE) {
        slots[0] = ITEM_SLOT_MAGIC;
        slots[1] = ITEM_SLOT_MISC;
        slots[2] = ITEM_SLOT_MISC;
    } else if (unit_types[hero_type].hero_item_class == HERO_ITEMS_BOWMAN) {
        slots[0] = ITEM_SLOT_BOW;
        slots[1] = ITEM_SLOT_ARMOR;
        slots[2] = ITEM_SLOT_MISC;
    } else {
        slots[0] = ITEM_SLOT_MELEE;
        slots[1] = ITEM_SLOT_ARMOR;
        slots[2] = ITEM_SLOT_MISC;
    }
}

/* (player) concentrates an AI's books: for each pair of realms that both have books - Death-Chaos, Death-Nature,
   Death-Sorcery, Life-Chaos, Life-Nature, Life-Sorcery, Sorcery-Chaos, Sorcery-Nature - Random(2) moves one realm's
   books into the other (Chaos-Nature is never merged) */
void AI_Wizard_Concentrate_Books(int player)
{
    if (wizards[player].spellbooks[REALM_DEATH] != 0 && wizards[player].spellbooks[REALM_CHAOS] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_DEATH] += wizards[player].spellbooks[REALM_CHAOS];
            wizards[player].spellbooks[REALM_CHAOS] = 0;
        } else {
            wizards[player].spellbooks[REALM_CHAOS] += wizards[player].spellbooks[REALM_DEATH];
            wizards[player].spellbooks[REALM_DEATH] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_DEATH] != 0 && wizards[player].spellbooks[REALM_NATURE] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_DEATH] += wizards[player].spellbooks[REALM_NATURE];
            wizards[player].spellbooks[REALM_NATURE] = 0;
        } else {
            wizards[player].spellbooks[REALM_NATURE] += wizards[player].spellbooks[REALM_DEATH];
            wizards[player].spellbooks[REALM_DEATH] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_DEATH] != 0 && wizards[player].spellbooks[REALM_SORCERY] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_DEATH] += wizards[player].spellbooks[REALM_SORCERY];
            wizards[player].spellbooks[REALM_SORCERY] = 0;
        } else {
            wizards[player].spellbooks[REALM_SORCERY] += wizards[player].spellbooks[REALM_DEATH];
            wizards[player].spellbooks[REALM_DEATH] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_LIFE] != 0 && wizards[player].spellbooks[REALM_CHAOS] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_LIFE] += wizards[player].spellbooks[REALM_CHAOS];
            wizards[player].spellbooks[REALM_CHAOS] = 0;
        } else {
            wizards[player].spellbooks[REALM_CHAOS] += wizards[player].spellbooks[REALM_LIFE];
            wizards[player].spellbooks[REALM_LIFE] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_LIFE] != 0 && wizards[player].spellbooks[REALM_NATURE] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_LIFE] += wizards[player].spellbooks[REALM_NATURE];
            wizards[player].spellbooks[REALM_NATURE] = 0;
        } else {
            wizards[player].spellbooks[REALM_NATURE] += wizards[player].spellbooks[REALM_LIFE];
            wizards[player].spellbooks[REALM_LIFE] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_LIFE] != 0 && wizards[player].spellbooks[REALM_SORCERY] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_LIFE] += wizards[player].spellbooks[REALM_SORCERY];
            wizards[player].spellbooks[REALM_SORCERY] = 0;
        } else {
            wizards[player].spellbooks[REALM_SORCERY] += wizards[player].spellbooks[REALM_LIFE];
            wizards[player].spellbooks[REALM_LIFE] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_SORCERY] != 0 && wizards[player].spellbooks[REALM_CHAOS] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_SORCERY] += wizards[player].spellbooks[REALM_CHAOS];
            wizards[player].spellbooks[REALM_CHAOS] = 0;
        } else {
            wizards[player].spellbooks[REALM_CHAOS] += wizards[player].spellbooks[REALM_SORCERY];
            wizards[player].spellbooks[REALM_SORCERY] = 0;
        }
    }
    if (wizards[player].spellbooks[REALM_SORCERY] != 0 && wizards[player].spellbooks[REALM_NATURE] != 0) {
        if (Random(2) == 1) {
            wizards[player].spellbooks[REALM_SORCERY] += wizards[player].spellbooks[REALM_NATURE];
            wizards[player].spellbooks[REALM_NATURE] = 0;
        } else {
            wizards[player].spellbooks[REALM_NATURE] += wizards[player].spellbooks[REALM_SORCERY];
            wizards[player].spellbooks[REALM_SORCERY] = 0;
        }
    }
}

/* (player) -> the casting skill (MAGIC's copy of WIZSTATS.C's): the n for which 2 + 4 + .. + 2n first
   reaches the skill points; Archmage +10 */
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

/* (weights, count) a random index, each with chance weight / total; while the running total reaches 512 every weight
   is halved IN the caller's array (Random's range); all zero -> 0 */
int Random_Weighted_Pick(unsigned *weights, int count)
{
    unsigned total;
    int i;
    int roll;
    int pick;
    int j;

again:
    total = 0;
    for (i = 0; i < count; i++) {
        total += weights[i];
        if (total >= 512) {
            for (j = 0; j < count; j++)
                weights[j] = weights[j] >> 1;
            goto again;
        }
    }
    if (total == 0)
        return 0;
    roll = Random(total) - weights[0];
    pick = 0;
    while (roll > 0 && pick < count - 1) {
        pick++;
        roll -= weights[pick];
    }
    return pick;
}
