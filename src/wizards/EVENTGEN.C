/* EVENTGEN.C - WIZARDS.EXE overlay code segment 6ad6 (stub 2493): random events - the per-turn roll (chance from the turns since
   the last event, scaled by difficulty; up to 5 tries of a random event against a victim wizard picked by the
   event's good / bad class, rejected when disabled or running, by the global-event, gold, turn, city, garrison,
   site and enchantment rules; a fired event stores its data in p_events), the per-turn processing of every
   active event (its effect, its announcement, and the end of the lasting ones), and the Disjunction gate.
   This module's BSS (DS:c2d6..c2ec): the announcement's parameters and results.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6ad6 src/wizards/EVENTGEN.C */

/* the module's functions in their original declaration order */
int Event_Roll(void);
void Events_Process(void);
int Any_Global_Enchant_Active(void);

#include "GAME.H"

int events_unused_flag;                         /* DS:c2ea  cleared by Events_Process, never read */
int event_msg_variant;                          /* DS:c2e8  0 the victim is the current player, 4 another wizard,
                                                   3 a global event, 1 an event ends */
int event_msg_event;                            /* DS:c2e6  the event announced (1..18) */
int event_city;                                 /* DS:c2e4  the city (or mineral site) of the event */
int event_msg_arg;                           /* DS:c2e2  item / city / gold / mineral of the event */
int event_msg_unused;                           /* DS:c2e0  set to 0, never read */
int event_victim;                            /* DS:c2de  the victim wizard (99 for a global event) */
int event_buildings_lost;                         /* DS:c2dc  buildings lost (Meteor, Earthquake) */
int event_units_lost;                         /* DS:c2da  units lost */
int event_pop_lost;                           /* DS:c2d8  population lost */
int events_unused_c2d6;                         /* DS:c2d6  no reader or writer found (never referenced) */

extern int random_events_enabled;                   /* DS:0154  1 in the EXE, never written */
extern int event_is_good[19];                   /* DS:4a0c */
extern int event_cooldown;                      /* DS:4a32 */
extern struct Events far *p_events;             /* DS:9998 */
extern int current_player;                      /* DS:bd8c */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */

int Random(int n);
int Event_Announce(void);
int Event_Pick_Wizard(int good);
int Event_Pick_City(int player);
int Wizard_Fortress_City(int player);
void City_Garrison_List(int city, int *count, int *list);
int Event_Find_Depletion_Site(int player, int *x, int *y, int *plane, int *mineral);
int Event_Find_New_Mineral_Site(int player, int *x, int *y, int *plane, int *mineral);
int Item_Create_Prefab(int prefab_kind, int *spellbooks, int max_cost);
int Call_The_Void_Effect(int city);
int Earthquake_Effect(int city, int *item_count, int *items);
void Hero_Items_Redistribute(int player, int count, int *items);
void Minimap_Alloc_Pic(void);
void Palette_Unmark_Range(int first, int last);
void Palette_Mark_Range(int first, int last);
void Remap_Build_Shadow(void);
void Set_Draw_Page(void);
void Map_Redraw(void);
void Fill_Box_Styled();                           /* no prototype here: called with 5 arguments (the definition in GRAPHICS.C takes 9, read by style) */
void Screen_Present(void);
void Screen_Copy_Front(void);
void Items_Screen_Award(int count, int *items);
void Map_Stack_At(int x, int y, int plane, int *count, int *list);
int Map_Explore_Radius(int x, int y, int plane, int range);
void Human_Cities_Area_Nop(void);
void City_Recalculate(int city);
void City_Area_Bitmaps_Rebuild(void);
void Cities_Update_Road_Connections(void);
void Unit_Dismiss(int unit);
void City_Change_Owner(int city, int new_owner);
void Tile_Clear_Mineral(int x, int y, int plane);
void Tile_Set_Mineral(int x, int y, int plane, int mineral);

/* () Next_Turn. With random_events_enabled 0 (never: nothing writes it; the option is game_config.random_events)
   every event is disabled (3). Chance = turns since the
   last event (0 for the 5 turns after one - event_cooldown is not saved, so also after every load), x1/2, 2/3,
   3/4, 4/5 by difficulty; fires when Random(512) <= chance. Up to 5 tries of Random(18): the victim by the
   event's good / bad class, with an active fortress; rejected when the event is disabled (3) or running (2), a
   global event while another runs, Pirates under 100 gold, Meteor / Marriage before turn 150, a city event
   without a city (Marriage also needs a neutral one), Rebellion in the fortress city, with a hero or with more
   fantastic than normal units, Depletion / New Minerals without a site, Disjunction without a global
   enchantment, Good vs Bad Moon, Plague vs Population Boom on the same city. Fired: state 1 and its data
   (Pirates take 30..50% of the gold rounded down to 10; Donation 100 + 5 x Random(100)). turn / (20, 18, 15,
   13, 10 by difficulty) is computed and never used
   Numbered: BUG #287 (BUGS_AND_QUIRKS.txt) */
int Event_Roll(void)
{
    int event;
    int player;
    int chance;
    int tries;
    int k;
    int escalation;                             /* computed, never read */
    int site_x;
    int site_y;
    int city;
    int neutral_city;
    int site_plane;
    int mineral;
    int garrison_count;
    int fantastic_units;
    int normal_units;
    int n;                                      /* the running global events; then the Pirates' take */
    int garrison[MAX_STACK];

    if (random_events_enabled == 0) {
        p_events->meteor = EVENT_STATE_DISABLED;
        p_events->gift = EVENT_STATE_DISABLED;
        p_events->disjunction = EVENT_STATE_DISABLED;
        p_events->marriage = EVENT_STATE_DISABLED;
        p_events->earthquake = EVENT_STATE_DISABLED;
        p_events->pirates = EVENT_STATE_DISABLED;
        p_events->plague = EVENT_STATE_DISABLED;
        p_events->plague_turns = 0;
        p_events->rebellion = EVENT_STATE_DISABLED;
        p_events->donation = EVENT_STATE_DISABLED;
        p_events->depletion = EVENT_STATE_DISABLED;
        p_events->new_minerals = EVENT_STATE_DISABLED;
        p_events->population_boom = EVENT_STATE_DISABLED;
        p_events->boom_turns = 0;
        p_events->good_moon = EVENT_STATE_DISABLED;
        p_events->good_moon_turns = 0;
        p_events->bad_moon = EVENT_STATE_DISABLED;
        p_events->bad_moon_turns = 0;
        p_events->conjunction_chaos = EVENT_STATE_DISABLED;
        p_events->conj_chaos_turns = 0;
        p_events->conjunction_nature = EVENT_STATE_DISABLED;
        p_events->conj_nature_turns = 0;
        p_events->conjunction_sorcery = EVENT_STATE_DISABLED;
        p_events->conj_sorcery_turns = 0;
        p_events->mana_short = EVENT_STATE_DISABLED;
        p_events->mana_short_turns = 0;
        return 0;
    }
    if (p_events->last_event_turn > game_turn)
        chance = 0;
    else {
        chance = game_turn - p_events->last_event_turn;
        if (chance < 0)
            chance = 0;
    }
    if (event_cooldown < 5) {
        event_cooldown++;
        chance = 0;
    }
    switch (difficulty) {
    case DIFFICULTY_INTRO:
        chance = chance / 2;
        break;
    case DIFFICULTY_EASY:
        chance = chance * 2 / 3;
        break;
    case DIFFICULTY_NORMAL:
        chance = chance * 3 / 4;
        break;
    case DIFFICULTY_HARD:
        chance = chance * 4 / 5;
        break;
    }
    if (Random(512) <= chance) {
        event = -1;
        for (tries = 0; tries < 5 && event == -1; tries++) {
            event = Random(NUM_EVENT_TYPES);
            player = Event_Pick_Wizard(event_is_good[event]);
            if (player == PLAYER_NONE)
                event = -1;
            if (p_fortresses[player].active == 0)   /* read even for PLAYER_NONE: p_fortresses[-1] (original) */
                event = -1;
            if (event == EVENT_METEOR && (p_events->meteor == EVENT_STATE_DISABLED || p_events->meteor == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_GIFT && (p_events->gift == EVENT_STATE_DISABLED || p_events->gift == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_DISJUNCTION && (p_events->disjunction == EVENT_STATE_DISABLED || p_events->disjunction == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_MARRIAGE && (p_events->marriage == EVENT_STATE_DISABLED || p_events->marriage == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_EARTHQUAKE && (p_events->earthquake == EVENT_STATE_DISABLED || p_events->earthquake == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_PIRATES && (p_events->pirates == EVENT_STATE_DISABLED || p_events->pirates == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_PLAGUE && (p_events->plague == EVENT_STATE_DISABLED || p_events->plague == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_DONATION && (p_events->donation == EVENT_STATE_DISABLED || p_events->donation == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_REBELLION && (p_events->rebellion == EVENT_STATE_DISABLED || p_events->rebellion == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_DEPLETION && (p_events->depletion == EVENT_STATE_DISABLED || p_events->depletion == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_NEW_MINERALS && (p_events->new_minerals == EVENT_STATE_DISABLED || p_events->new_minerals == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_POPULATION_BOOM && (p_events->population_boom == EVENT_STATE_DISABLED || p_events->population_boom == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_GOOD_MOON && (p_events->good_moon == EVENT_STATE_DISABLED || p_events->good_moon == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_BAD_MOON && (p_events->bad_moon == EVENT_STATE_DISABLED || p_events->bad_moon == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_CONJUNCTION_CHAOS && (p_events->conjunction_chaos == EVENT_STATE_DISABLED || p_events->conjunction_chaos == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_CONJUNCTION_NATURE && (p_events->conjunction_nature == EVENT_STATE_DISABLED || p_events->conjunction_nature == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_CONJUNCTION_SORCERY && (p_events->conjunction_sorcery == EVENT_STATE_DISABLED || p_events->conjunction_sorcery == EVENT_STATE_RUNNING))
                event = -1;
            if (event == EVENT_MANA_SHORT && (p_events->mana_short == EVENT_STATE_DISABLED || p_events->mana_short == EVENT_STATE_RUNNING))
                event = -1;
            n = 0;
            if (p_events->good_moon == EVENT_STATE_RUNNING)
                n++;
            if (p_events->bad_moon == EVENT_STATE_RUNNING)
                n++;
            if (p_events->conjunction_chaos == EVENT_STATE_RUNNING)
                n++;
            if (p_events->conjunction_nature == EVENT_STATE_RUNNING)
                n++;
            if (p_events->conjunction_sorcery == EVENT_STATE_RUNNING)
                n++;
            if (p_events->mana_short == EVENT_STATE_RUNNING)
                n++;
            if (n >= 1 && (event == EVENT_GOOD_MOON || event == EVENT_BAD_MOON || event == EVENT_CONJUNCTION_CHAOS || event == EVENT_CONJUNCTION_NATURE || event == EVENT_CONJUNCTION_SORCERY || event == EVENT_MANA_SHORT))
                event = -1;
            if (event == EVENT_PIRATES && wizards[player].gold < 100)
                event = -1;
            if (event == EVENT_METEOR || event == EVENT_MARRIAGE || event == EVENT_EARTHQUAKE || event == EVENT_PLAGUE || event == EVENT_REBELLION || event == EVENT_POPULATION_BOOM) {
                city = Event_Pick_City(player);
                if (city == -1)
                    event = -1;
                if (event == EVENT_MARRIAGE) {
                    neutral_city = Event_Pick_City(PLAYER_NEUTRAL);
                    if (neutral_city == -1)
                        event = -1;
                }
            }
            if (event == EVENT_MARRIAGE && game_turn < 150)
                event = -1;
            if (event == EVENT_METEOR && game_turn < 150)
                event = -1;
            if (event == EVENT_REBELLION) {
                fantastic_units = 0;
                normal_units = 0;
                if (Wizard_Fortress_City(player) == city)
                    event = -1;
                else {
                    City_Garrison_List(city, &garrison_count, garrison);
                    for (k = 0; k < garrison_count && event == EVENT_REBELLION; k++) {
                        if (unit_types[p_units[garrison[k]].type].race >= RACE_ARCANE_CREATURE)
                            fantastic_units++;
                        else
                            normal_units++;
                        if (p_units[garrison[k]].hero_slot > -1)
                            event = -1;
                    }
                    if (fantastic_units > normal_units)
                        event = -1;
                }
            }
            if (event == EVENT_DEPLETION && Event_Find_Depletion_Site(player, &site_x, &site_y, &site_plane, &mineral) == -1)
                event = -1;
            if (event == EVENT_NEW_MINERALS && Event_Find_New_Mineral_Site(player, &site_x, &site_y, &site_plane, &mineral) == -1)
                event = -1;
            if (event == EVENT_DISJUNCTION && Any_Global_Enchant_Active() == 0)
                event = -1;
            if (event == EVENT_GOOD_MOON && p_events->bad_moon == EVENT_STATE_RUNNING)
                event = -1;
            if (event == EVENT_BAD_MOON && p_events->good_moon == EVENT_STATE_RUNNING)
                event = -1;
            if (event == EVENT_PLAGUE && p_events->population_boom == EVENT_STATE_RUNNING && p_events->boom_city == city)
                event = -1;
            if (event == EVENT_POPULATION_BOOM && p_events->plague == EVENT_STATE_RUNNING && p_events->plague_city == city)
                event = -1;
        }
        if (event != -1) {
            p_events->last_event_turn = game_turn;
            event_cooldown = 0;
            switch (difficulty) {
            case DIFFICULTY_INTRO:
                escalation = game_turn / 20;
                break;
            case DIFFICULTY_EASY:
                escalation = game_turn / 18;
                break;
            case DIFFICULTY_NORMAL:
                escalation = game_turn / 15;
                break;
            case DIFFICULTY_HARD:
                escalation = game_turn / 13;
                break;
            case DIFFICULTY_IMPOSSIBLE:
                escalation = game_turn / 10;
                break;
            }
            switch (event) {
            case EVENT_METEOR:
                p_events->meteor = EVENT_STATE_NEW;
                p_events->meteor_city = city;
                p_events->meteor_player = player;
                break;
            case EVENT_GIFT:
                p_events->gift = EVENT_STATE_NEW;
                p_events->gift_player = player;
                p_events->gift_item = Item_Create_Prefab(PREFAB_ARTIFACT, wizards[player].spellbooks, 0);
                break;
            case EVENT_DISJUNCTION:
                p_events->disjunction = EVENT_STATE_NEW;
                break;
            case EVENT_MARRIAGE:
                p_events->marriage = EVENT_STATE_NEW;
                p_events->marriage_city = neutral_city;
                p_events->marriage_own_city = city;
                p_events->marriage_player = player;
                break;
            case EVENT_EARTHQUAKE:
                p_events->earthquake = EVENT_STATE_NEW;
                p_events->quake_player = player;
                p_events->quake_city = city;
                break;
            case EVENT_PIRATES:
                p_events->pirates = EVENT_STATE_NEW;
                p_events->pirates_player = player;
                n = wizards[player].gold * (Random(21) + 29L) / 100;
                n -= n % 10;
                p_events->pirates_gold = n;
                break;
            case EVENT_PLAGUE:
                p_events->plague = EVENT_STATE_NEW;
                p_events->plague_city = city;
                p_events->plague_player = player;
                break;
            case EVENT_REBELLION:
                p_events->rebellion = EVENT_STATE_NEW;
                p_events->rebellion_city = city;
                p_events->rebellion_player = player;
                break;
            case EVENT_DONATION:
                p_events->donation = EVENT_STATE_NEW;
                p_events->donation_player = player;
                p_events->donation_gold = Random(100) * 5 + 100;
                break;
            case EVENT_DEPLETION:
                p_events->depletion = EVENT_STATE_NEW;
                p_events->depletion_player = player;
                break;
            case EVENT_NEW_MINERALS:
                p_events->new_minerals = EVENT_STATE_NEW;
                p_events->minerals_player = player;
                break;
            case EVENT_POPULATION_BOOM:
                p_events->population_boom = EVENT_STATE_NEW;
                p_events->boom_city = city;
                p_events->boom_player = player;
                break;
            case EVENT_GOOD_MOON:
                p_events->good_moon = EVENT_STATE_NEW;
                break;
            case EVENT_BAD_MOON:
                p_events->bad_moon = EVENT_STATE_NEW;
                break;
            case EVENT_CONJUNCTION_CHAOS:
                p_events->conjunction_chaos = EVENT_STATE_NEW;
                break;
            case EVENT_CONJUNCTION_NATURE:
                p_events->conjunction_nature = EVENT_STATE_NEW;
                break;
            case EVENT_CONJUNCTION_SORCERY:
                p_events->conjunction_sorcery = EVENT_STATE_NEW;
                break;
            case EVENT_MANA_SHORT:
                p_events->mana_short = EVENT_STATE_NEW;
                break;
            }
        }
    }
}

/* () Next_Turn, also with random events off: each event in state 1 / 2 takes effect and is announced - Meteor
   (Call the Void on the city, if it is still the victim's), Gift (the human gets the item screen; an AI gets
   nothing: BUG #177), Disjunction (every wizard's global enchantments cleared), Marriage (the neutral city and
   its units join the wizard), Earthquake (announced, and the lost hero items redistributed, only
   when something was lost), Pirates (gold, floor 0), Plague (lasting: -1 population at Random(10) < population, above 2; ends
   1 in 20 after 4 turns or when the population falls below 2, which is reset to 2: BUG #178), Rebellion (normal units go neutral,
   fantastic ones die, the city goes neutral), Donation, Depletion / New Minerals (the site found again now),
   Population Boom (lasting) and the six global events (lasting; end at Random(20) <= turns - 2 after 4 turns);
   the end of a lasting event is announced to the human (variant 1) */
void Events_Process(void)
{
    int i;
    int city;
    int k;
    int player;
    unsigned char *ench;
    int garrison_after;
    int x;
    int y;
    int plane;
    int mineral;
    int pop_before;
    int garrison_before;
    int item_count;
    int garrison[MAX_STACK];
    int items[18];

    events_unused_flag = 0;
    if (p_events->meteor == EVENT_STATE_NEW || p_events->meteor == EVENT_STATE_RUNNING) {
        if (p_cities[p_events->meteor_city].owner == p_events->meteor_player) {
            player = p_events->meteor_player;
            event_msg_arg = 0;
            event_msg_unused = 0;
            event_msg_event = EVENT_METEOR;
            event_city = p_events->meteor_city;
            event_victim = player;
            if (player == current_player)
                event_msg_variant = EVENT_MSG_YOURS;
            else
                event_msg_variant = EVENT_MSG_OTHER_WIZARD;
            p_events->meteor = EVENT_STATE_NONE;
            City_Garrison_List(event_city, &garrison_before, garrison);
            pop_before = p_cities[event_city].population;
            event_buildings_lost = Call_The_Void_Effect(event_city);
            event_pop_lost = pop_before - p_cities[event_city].population;
            City_Garrison_List(event_city, &garrison_after, garrison);
            event_units_lost = garrison_before - garrison_after;
            Event_Announce();
        } else
            p_events->meteor = EVENT_STATE_NONE;
    }
    if (p_events->gift == EVENT_STATE_NEW || p_events->gift == EVENT_STATE_RUNNING) {
        event_msg_arg = p_events->gift_item;
        event_msg_unused = 0;
        event_msg_event = EVENT_GIFT;
        event_city = 0;
        event_victim = p_events->gift_player;
        p_events->gift = EVENT_STATE_NONE;
        if (p_events->gift_player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        Event_Announce();
        if (event_msg_variant == EVENT_MSG_YOURS) {
            Minimap_Alloc_Pic();
            Palette_Unmark_Range(0, 255);
            Palette_Mark_Range(0, 223);
            Remap_Build_Shadow();
            Set_Draw_Page();
            Map_Redraw();
            Fill_Box_Styled(0, 0, 319, 199, FILL_SHADOW);
            Screen_Present();
            Screen_Copy_Front();
            garrison[0] = p_events->gift_item;   /* garrison reused: the one-item list */
            Items_Screen_Award(1, garrison);
        }
    }
    if (p_events->disjunction == EVENT_STATE_NEW || p_events->disjunction == EVENT_STATE_RUNNING) {
        event_msg_arg = 0;
        event_msg_unused = 0;
        event_msg_event = EVENT_DISJUNCTION;
        event_city = 0;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        p_events->disjunction = EVENT_STATE_NONE;
        for (i = 0; i < MAX_PLAYERS; i++) {
            ench = (unsigned char *)&wizards[i].globals;
            for (k = 0; k < NUM_GLOBAL_ENCHANTS; k++)
                if ((int)ench[k] > 0)
                    ench[k] = 0;
        }
        Event_Announce();
    }
    if (p_events->marriage == EVENT_STATE_NEW || p_events->marriage == EVENT_STATE_RUNNING) {
        player = p_events->marriage_player;
        event_city = p_events->marriage_city;
        event_msg_arg = p_events->marriage_city;
        event_msg_event = EVENT_MARRIAGE;
        event_victim = p_events->marriage_player;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        city = p_events->marriage_city;
        Map_Stack_At(p_cities[city].x, p_cities[city].y, p_cities[city].plane, &garrison_after, garrison);   /* garrison_after: here
                                                   the Marriage city's stack count */
        for (i = 0; i < garrison_after; i++)
            if (p_units[garrison[i]].owner != PLAYER_NONE)
                p_units[garrison[i]].owner = player;
        p_cities[city].owner = player;
        p_cities[city].producing = PROD_TRADE_GOODS;
        p_cities[city].production_progress = 0;
        p_cities[city].sold_this_turn = 0;
        if (player == current_player)
            Map_Explore_Radius(p_cities[city].x, p_cities[city].y, p_cities[city].plane, 3);
        Human_Cities_Area_Nop();
        City_Recalculate(city);
        City_Area_Bitmaps_Rebuild();
        Cities_Update_Road_Connections();
        Event_Announce();
        p_events->marriage = EVENT_STATE_NONE;
    }
    if (p_events->earthquake == EVENT_STATE_NEW || p_events->earthquake == EVENT_STATE_RUNNING) {
        player = p_events->quake_player;
        event_msg_event = EVENT_EARTHQUAKE;
        event_victim = p_events->quake_player;
        event_msg_arg = 0;
        event_city = p_events->quake_city;
        p_events->earthquake = EVENT_STATE_NONE;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        City_Garrison_List(event_city, &garrison_before, garrison);
        pop_before = p_cities[event_city].population;
        event_buildings_lost = Earthquake_Effect(event_city, &item_count, items);
        event_pop_lost = pop_before - p_cities[event_city].population;
        City_Garrison_List(event_city, &garrison_after, garrison);
        event_units_lost = garrison_before - garrison_after;
        if (event_units_lost == 0 && event_pop_lost == 0 && event_buildings_lost == 0) {
            /* nothing lost: no announcement (the empty branch is in the code: a jmp over the else) */
        } else {
            Event_Announce();
            Hero_Items_Redistribute(p_cities[event_city].owner, item_count, items);
        }
    }
    if (p_events->pirates == EVENT_STATE_NEW || p_events->pirates == EVENT_STATE_RUNNING) {
        player = p_events->pirates_player;
        event_msg_event = EVENT_PIRATES;
        event_victim = p_events->pirates_player;
        event_msg_arg = p_events->pirates_gold;
        p_events->pirates = EVENT_STATE_NONE;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        Event_Announce();
        wizards[player].gold = wizards[player].gold - p_events->pirates_gold;
        if (wizards[player].gold < 0)
            wizards[player].gold = 0;
    }
    if (p_events->plague == EVENT_STATE_NEW || p_events->plague == EVENT_STATE_RUNNING) {
        player = p_events->plague_player;
        event_msg_event = EVENT_PLAGUE;
        event_city = p_events->plague_city;
        event_victim = p_events->plague_player;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        if (Random(10) < p_cities[p_events->plague_city].population && p_cities[p_events->plague_city].population > 2)
            p_cities[p_events->plague_city].population = p_cities[p_events->plague_city].population - 1;
        if (p_events->plague == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->plague = EVENT_STATE_RUNNING;
            p_events->plague_turns = 0;
        } else {
            if (Random(20) == 1 && p_events->plague_turns > 3)
                p_events->plague = EVENT_STATE_NONE;
            p_events->plague_turns++;
            if (p_cities[p_events->plague_city].population < 2) {
                p_events->plague = EVENT_STATE_NONE;
                p_cities[p_events->plague_city].population = 2;
            }
            if (player == current_player && p_events->plague == EVENT_STATE_NONE) {
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
        }
    }
    if (p_events->rebellion == EVENT_STATE_NEW || p_events->rebellion == EVENT_STATE_RUNNING) {
        player = p_events->rebellion_player;
        event_city = p_events->rebellion_city;
        event_victim = p_events->rebellion_player;
        event_msg_event = EVENT_REBELLION;
        p_events->rebellion = EVENT_STATE_NONE;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        City_Garrison_List(event_city, &garrison_after, garrison);
        for (i = 0; i < garrison_after; i++) {
            if (unit_types[p_units[garrison[i]].type].race >= RACE_ARCANE_CREATURE)
                Unit_Dismiss(garrison[i]);
            else
                p_units[garrison[i]].owner = PLAYER_NEUTRAL;
        }
        City_Change_Owner(event_city, PLAYER_NEUTRAL);
        Event_Announce();
    }
    if (p_events->donation == EVENT_STATE_NEW || p_events->donation == EVENT_STATE_RUNNING) {
        player = p_events->donation_player;
        event_msg_event = EVENT_DONATION;
        event_victim = p_events->donation_player;
        event_msg_arg = p_events->donation_gold;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        p_events->donation = EVENT_STATE_NONE;
        wizards[player].gold += event_msg_arg;
        Event_Announce();
    }
    if (p_events->depletion == EVENT_STATE_NEW || p_events->depletion == EVENT_STATE_RUNNING) {
        player = p_events->depletion_player;
        event_msg_event = EVENT_DEPLETION;
        event_victim = p_events->depletion_player;
        i = Event_Find_Depletion_Site(player, &x, &y, &plane, &mineral);   /* i reused: the site */
        p_events->depletion_site = i;
        event_city = p_events->depletion_site;
        Tile_Clear_Mineral(x, y, plane);
        event_msg_arg = mineral;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        p_events->depletion = EVENT_STATE_NONE;
        Event_Announce();
    }
    if (p_events->new_minerals == EVENT_STATE_NEW || p_events->new_minerals == EVENT_STATE_RUNNING) {
        player = p_events->minerals_player;
        event_msg_event = EVENT_NEW_MINERALS;
        event_victim = p_events->minerals_player;
        i = Event_Find_New_Mineral_Site(player, &x, &y, &plane, &mineral);
        p_events->minerals_site = i;
        event_city = i;
        Tile_Set_Mineral(x, y, plane, mineral);
        event_msg_arg = mineral;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        p_events->new_minerals = EVENT_STATE_NONE;
        Event_Announce();
    }
    if (p_events->population_boom == EVENT_STATE_NEW || p_events->population_boom == EVENT_STATE_RUNNING) {
        player = p_events->boom_player;
        event_msg_event = EVENT_POPULATION_BOOM;
        event_victim = p_events->boom_player;
        event_city = p_events->boom_city;
        if (player == current_player)
            event_msg_variant = EVENT_MSG_YOURS;
        else
            event_msg_variant = EVENT_MSG_OTHER_WIZARD;
        if (p_events->population_boom == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->population_boom = EVENT_STATE_RUNNING;
            p_events->boom_turns = 0;
        } else {
            if (Random(20) == 1 && p_events->boom_turns > 3) {
                p_events->population_boom = EVENT_STATE_NONE;
                if (player == current_player) {
                    event_msg_variant = EVENT_MSG_ENDS;
                    Event_Announce();
                }
            }
            p_events->boom_turns++;
        }
    }
    if (p_events->good_moon == EVENT_STATE_NEW || p_events->good_moon == EVENT_STATE_RUNNING) {
        event_msg_event = EVENT_GOOD_MOON;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        if (p_events->good_moon == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->good_moon = EVENT_STATE_RUNNING;
            p_events->good_moon_turns = 0;
        } else {
            if (Random(20) <= p_events->good_moon_turns - 2 && p_events->good_moon_turns > 3) {
                p_events->good_moon = EVENT_STATE_NONE;
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
            p_events->good_moon_turns++;
        }
    }
    if (p_events->bad_moon == EVENT_STATE_NEW || p_events->bad_moon == EVENT_STATE_RUNNING) {
        event_msg_event = EVENT_BAD_MOON;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        if (p_events->bad_moon == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->bad_moon = EVENT_STATE_RUNNING;
            p_events->bad_moon_turns = 0;
        } else {
            if (Random(20) <= p_events->bad_moon_turns - 2 && p_events->bad_moon_turns > 3) {
                p_events->bad_moon = EVENT_STATE_NONE;
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
            p_events->bad_moon_turns++;
        }
    }
    if (p_events->conjunction_chaos == EVENT_STATE_NEW || p_events->conjunction_chaos == EVENT_STATE_RUNNING) {
        event_msg_event = EVENT_CONJUNCTION_CHAOS;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        if (p_events->conjunction_chaos == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->conjunction_chaos = EVENT_STATE_RUNNING;
            p_events->conj_chaos_turns = 0;
        } else {
            if (Random(20) <= p_events->conj_chaos_turns - 2 && p_events->conj_chaos_turns > 3) {
                p_events->conjunction_chaos = EVENT_STATE_NONE;
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
            p_events->conj_chaos_turns++;
        }
    }
    if (p_events->conjunction_nature == EVENT_STATE_NEW || p_events->conjunction_nature == EVENT_STATE_RUNNING) {
        event_msg_event = EVENT_CONJUNCTION_NATURE;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        if (p_events->conjunction_nature == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->conjunction_nature = EVENT_STATE_RUNNING;
            p_events->conj_nature_turns = 0;
        } else {
            if (Random(20) <= p_events->conj_nature_turns - 2 && p_events->conj_nature_turns > 3) {
                p_events->conjunction_nature = EVENT_STATE_NONE;
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
            p_events->conj_nature_turns++;
        }
    }
    if (p_events->conjunction_sorcery == EVENT_STATE_NEW || p_events->conjunction_sorcery == EVENT_STATE_RUNNING) {
        event_msg_event = EVENT_CONJUNCTION_SORCERY;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        if (p_events->conjunction_sorcery == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->conjunction_sorcery = EVENT_STATE_RUNNING;
            p_events->conj_sorcery_turns = 0;
        } else {
            if (Random(20) <= p_events->conj_sorcery_turns - 2 && p_events->conj_sorcery_turns > 3) {
                p_events->conjunction_sorcery = EVENT_STATE_NONE;
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
            p_events->conj_sorcery_turns++;
        }
    }
    if (p_events->mana_short == EVENT_STATE_NEW || p_events->mana_short == EVENT_STATE_RUNNING) {
        event_msg_event = EVENT_MANA_SHORT;
        event_victim = EVENT_NO_VICTIM;
        event_msg_variant = EVENT_MSG_GLOBAL;
        if (p_events->mana_short == EVENT_STATE_NEW) {
            Event_Announce();
            p_events->mana_short = EVENT_STATE_RUNNING;
            p_events->mana_short_turns = 0;
        } else {
            if (Random(20) <= p_events->mana_short_turns - 2 && p_events->mana_short_turns > 3) {
                p_events->mana_short = EVENT_STATE_NONE;
                event_msg_variant = EVENT_MSG_ENDS;
                Event_Announce();
            }
            p_events->mana_short_turns++;
        }
    }
}

/* () -> 1 when any wizard has any of the 24 global enchantments (the Disjunction event's gate) */
int Any_Global_Enchant_Active(void)
{
    int k;
    int player;
    int found;
    unsigned char *ench;

    found = 0;
    for (player = 0; player < MAX_PLAYERS && found == 0; player++) {
        ench = (unsigned char *)&wizards[player].globals;
        for (k = 0; k < NUM_GLOBAL_ENCHANTS && found == 0; k++)
            if ((int)ench[k] > 0)
                found = 1;
    }
    return found;
}
