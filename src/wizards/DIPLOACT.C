/* DIPLOACT.C - WIZARDS.EXE overlay code segment 7057 (stub 24ae): the diplomatic actions - the greeting an audience
   opens with, the end-of-turn relation drift (treaty bonuses, army envy, the human's city count, the pull toward
   each wizard's base relation, the mirrored table), an AI's demand to drop an ally, the AI's answer score to a
   proposal, treaties made and broken, peace, war, a relation change seen by one wizard (scaled by the relation,
   personality, Charismatic, war and the turn), the tribute and spell-exchange menus, and the curses two wizards
   lift from each other's cities at peace.
   This module's data (DS:4eba..4f39): the three menu prompts and its literals (the menu title "" is the tail of the
   first prompt).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7057 src/wizards/DIPLOACT.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Diplo_Pick_Greeting(void);
void Diplo_Relation_Drift(void);
int Diplo_Demand_Drop_Ally(void);
int Diplo_Proposal_Response(int penalty, int interest_kind);
int Diplo_Min_Interest(void);
void Diplo_Clamp_Treaty_Interest(void);
void Diplo_Give_Spell(int player, int spell);
void Diplo_Break_Conflicting_Allies(void);
void Diplo_Set_Status(int a, int b, int status);
void Diplo_Make_Peace(int a, int b);
void Diplo_Interests_Minus10(int a, int b);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
int Diplo_Declare_War(int declarer, int target);
int Diplo_Break_Treaty(int breaker, int victim);
int Diplo_List_Spells_To_Trade(int giver, int receiver, int min_value);
void Diplo_Offer_Tribute(void);
void Diplo_Exchange_Spells(void);
void Diplo_Remove_Mutual_City_Curses(int a, int b);

#include "GAME.H"

char *diplo_prompt_texts[3] = {                 /* DS:4eba */
    "What do you offer as tribute?",
    "What type of spell interests you?",
    "What will you trade for it?"
};

extern int personality_treaty_mod[];            /* DS:1fc8 */
extern int diplo_test_mode;                     /* DS:4c44  never set: the test branches are dead */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern int land_size;                           /* DS:bd9a */
extern int diplo_last_response_score;           /* DS:c30e */
extern unsigned char xchg_counter_spells[10][10];       /* DS:c320 */
extern unsigned char xchg_counter_count[10];    /* DS:c384 */
extern unsigned char xchg_offer_spell[10];      /* DS:c38e */
extern unsigned char xchg_offer_count;          /* DS:c398 */
extern unsigned char xchg_ai_values[10];        /* DS:c399 */
extern unsigned char xchg_ai_spells[10];        /* DS:c3a3 */
extern unsigned char xchg_ai_count;             /* DS:c3ad */
extern unsigned char spell_trade_values[10];    /* DS:c3ae */
extern unsigned char spell_trade_list[10];      /* DS:c3b8 */
extern unsigned char spell_trade_count;         /* DS:c3c2 */
extern char *p_diplo_text;                      /* DS:c3e4 */
extern int diplo_msg_variant;                   /* DS:c3ea */
extern int diplo_msg_type;                      /* DS:c3ec */
extern int diplo_talk_mode;                     /* DS:c3ee */
extern char *diplo_menu_items[7];               /* DS:c3f0 */
extern int diplo_partner;                       /* DS:c3fe */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
int Diplo_Army_Near_Fortress(int a, int b, long *army_a, long *army_b);
int Screen_Diplomacy_Offer(void);
void Wizard_Learn_Spell(int player, int spell, int show);
void Diplo_Load_Assets(void);
int AI_Cancel_Unit_Orders(int player);
int Diplo_Tradeable_Spells(int receiver, int giver, int min_value, unsigned char *spell_list);
int Spell_Trade_Value(int player, int spell);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Font_Select();                             /* no prototype here: called with 3 arguments (TEXT.C takes 4) */
void Set_Font_Color(int color);
void Set_Line_Spacing(int line_gap);
int UI_Text_List_Select(int x, int y, int width, char *title, char *items, int stride, int *selection, int *enabled,
                        int fill_mode, int fill1, int fill2, int fill3, int fill4, int help);
void Wizard_Add_Gold(int player, int amount);
void Diplo_Show_Reply(int msg, int reply);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Diplo_Menu_Redraw(void);

/* the audience's opening: refused (the score without the relation <= -100), else DIPLO_MSG_AUDIENCE_GREETING when
   the score is above -50, DIPLO_MSG_AUDIENCE_GRANTED otherwise
   Numbered: BUG #322 (BUGS_AND_QUIRKS.txt) */
void Diplo_Pick_Greeting(void)
{
    int min_interest;
    int score;

    min_interest = Diplo_Min_Interest();
    score = wizards[PLAYER_HUMAN].relation_bias[diplo_partner] + wizards[PLAYER_HUMAN].relationship[diplo_partner] + min_interest
          + personality_treaty_mod[wizards[diplo_partner].personality];
    if (score - wizards[PLAYER_HUMAN].relationship[diplo_partner] <= -100) {
        if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] >= TREATY_WAR) {   /* both branches empty in the original */
        } else {
        }
        diplo_msg_type = DIPLO_MSG_GO_AWAY;
        diplo_talk_mode = DIPLO_TALK_REFUSED;
    } else {
        if (score > -50) {
            diplo_msg_type = DIPLO_MSG_AUDIENCE_GREETING;
        } else {
            diplo_msg_type = DIPLO_MSG_AUDIENCE_GRANTED;
        }
        diplo_talk_mode = DIPLO_TALK_GREETING;
    }
}

/* end of turn: treaty bonuses; army envy (-10, 1 in 20, only while the human's army is under twice a wizard's:
   Random(100) must reach army x 50 / theirs - a bigger army causes less envy); the human's city count (every other
   turn when over (land size + 1) x 3: a penalty with every wizard i as the actor - AIs halved - toward each contacted
   non-ally); the drift toward the base relations (AI rows; Random(2) when Random(105) beats |relation|: up
   every turn while below the base, down only every tenth turn); the mirror
   Numbered: BUG #156, #157 (BUGS_AND_QUIRKS.txt) */
void Diplo_Relation_Drift(void)
{
    int i;
    int j;
    int amount;
    int base;
    int count;
    int drift;
    int army;
    int threshold;

    for (i = 0; i < num_players; i++) {
        for (j = i + 1; j < num_players; j++) {
            if (Random(2) == 1) {
                if (wizards[i].diplomacy_status[j] == TREATY_WIZARD_PACT) {
                    Diplo_Relation_Change(Random(3), i, j, 0, 0, 0);
                }
                if (wizards[i].diplomacy_status[j] == TREATY_ALLIANCE) {
                    Diplo_Relation_Change(Random(6), i, j, 0, 0, 0);
                }
            }
        }
    }
    army = wizards[PLAYER_HUMAN].stat_army_strength;
    if (army > 0) {
        for (i = 0; i < num_players; i++) {
            if (wizards[i].stat_army_strength < army && wizards[i].stat_army_strength > 0) {
                threshold = (long)army * 50 / wizards[i].stat_army_strength;
                if (Random(100) >= threshold && Random(20) == 1) {
                    Diplo_Relation_Change(-10, PLAYER_HUMAN, i, DIPLO_MSG_TROOPS_NEAR_CITIES, 0, 0);
                }
            }
        }
    }
    if (game_turn % 2 == 0) {
        for (i = 0; i < num_players; i++) {
            count = 0;
            for (j = 0; j < num_cities; j++) {
                if (p_cities[j].owner == PLAYER_HUMAN) {
                    count++;
                }
            }
            if ((land_size + 1) * 3 < count && Random(4) == 1) {
                amount = Random(4) * (-(count - (land_size + 1) * 3) / (land_size + 6 - difficulty)) / 3;
                amount = amount * 4;
                if (i != 0) {
                    amount /= 2;
                }
                if (amount < -15) {
                    amount = -15;
                }
                for (j = 1; j < num_players; j++) {
                    if (wizards[i].contacted[j] == 1 && wizards[i].diplomacy_status[j] != TREATY_ALLIANCE) {
                        Diplo_Relation_Change(amount, i, j, DIPLO_MSG_TOO_POWERFUL, 0, 0);
                    }
                }
            }
        }
    }
    for (i = 1; i < num_players; i++) {
        for (j = 0; j < num_players; j++) {
            if (Random(105) > abs(wizards[i].relationship[j])) {
                drift = Random(2);
            } else {
                drift = 0;
            }
            if (j != i && wizards[i].pending_rel_amount[j] == 0) {
                base = wizards[i].base_relation[j];
                if (wizards[i].relationship[j] < base) {
                    wizards[i].relationship[j] += drift;
                    if (wizards[i].relationship[j] > base) {
                        wizards[i].relationship[j] = base;
                    }
                } else if (game_turn % 10 == 0) {
                    wizards[i].relationship[j] -= drift;
                    if (wizards[i].relationship[j] < base) {
                        wizards[i].relationship[j] = base;
                    }
                }
                if (wizards[i].relationship[j] < -100) {
                    wizards[i].relationship[j] = -100;
                }
                if (wizards[i].relationship[j] > 100) {
                    wizards[i].relationship[j] = 100;
                }
            }
        }
    }
    for (i = 0; i < num_players; i++) {
        for (j = 0; j < num_players; j++) {
            if (i == j) {
                wizards[i].relationship[j] = 0;
            } else {
                wizards[i].relationship[j] = wizards[j].relationship[i];
            }
        }
    }
}

/* before an AI proposal: drop your ally that the partner is at war with? 0 refused (relations -100), 1 */
int Diplo_Demand_Drop_Ally(void)
{
    int found;
    int i;
    int ally;
    int answer;

    found = 0;
    for (i = 1; i < num_players; i++) {
        if (i != diplo_partner && wizards[diplo_partner].diplomacy_status[i] >= TREATY_WAR && wizards[PLAYER_HUMAN].diplomacy_status[i] == TREATY_ALLIANCE) {
            found = i;
        }
    }
    ally = found;
    if (ally == 0) {
        return 1;
    }
    diplo_msg_type = DIPLO_MSG_DEMAND_BREAK_TREATY;
    wizards[PLAYER_HUMAN].demand_drop_ally[diplo_partner] = ally;
    diplo_msg_variant = -1;
    answer = Screen_Diplomacy_Offer();
    if (answer != 0) {
        i = Random(6) + 6;
        wizards[PLAYER_HUMAN].relationship[diplo_partner] -= i;
        if (wizards[PLAYER_HUMAN].relationship[diplo_partner] < -100) {
            wizards[PLAYER_HUMAN].relationship[diplo_partner] = -100;
        }
        wizards[diplo_partner].relationship[PLAYER_HUMAN] = wizards[PLAYER_HUMAN].relationship[diplo_partner] = -100;
        return 0;
    } else {
        Diplo_Break_Treaty(PLAYER_HUMAN, ally);
    }
    return 1;
}

/* the AI's answer to a proposal: DIPLO_REPLY_* (kind INTEREST_TREATY / _PEACE / _TRADE picks the interest; every caller
   passes INTEREST_TREATY - kind 1 would leave 'interest' unset)
   Numbered: BUG #51 (BUGS_AND_QUIRKS.txt) */
int Diplo_Proposal_Response(int penalty, int interest_kind)
{
    int unused;
    int score_base;
    int interest;
    long human_army;
    long partner_army;
    int term;
    int score;

    unused = 0;
    switch (interest_kind) {
    case INTEREST_TREATY:
        interest = wizards[PLAYER_HUMAN].treaty_interest[diplo_partner];
        break;
    case INTEREST_PEACE:
        interest = wizards[PLAYER_HUMAN].peace_interest[diplo_partner];
        break;
    case INTEREST_TRADE:
        interest = wizards[PLAYER_HUMAN].trade_interest[diplo_partner];
        break;
    }
    score_base = wizards[PLAYER_HUMAN].relation_bias[diplo_partner] + wizards[PLAYER_HUMAN].relationship[diplo_partner]
         + personality_treaty_mod[wizards[diplo_partner].personality] + interest;
    human_army = 0;
    partner_army = 0;
    Diplo_Army_Near_Fortress(current_player, diplo_partner, &human_army, &partner_army);
    if (partner_army > human_army) {
        if (human_army > 0) {
            term = -(partner_army * 100 / human_army) - 50;
        } else {
            term = -50;
        }
    } else {
        if (partner_army > 0) {
            term = human_army * 100 / partner_army - 50;
        } else {
            term = 50;
        }
    }
    score = Random(100) + score_base - penalty + term;
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] == TREATY_ALLIANCE) {
        score += 40;
    }
    Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
    if (interest_kind == INTEREST_TREATY) {
        wizards[PLAYER_HUMAN].treaty_interest[diplo_partner] -= Random(30) + 20;
    }
    if (interest_kind == INTEREST_PEACE) {
        wizards[PLAYER_HUMAN].peace_interest[diplo_partner] -= Random(50) + 50;
    }
    if (interest_kind == INTEREST_TRADE) {
        wizards[PLAYER_HUMAN].trade_interest[diplo_partner] -= Random(50) + 20;
    }
    diplo_last_response_score = score;
    if (score < -75) {
        return DIPLO_REPLY_REFUSE;
    }
    if (score < -50) {
        return DIPLO_REPLY_WANTS_SPELL;
    }
    if (score < 0) {
        return DIPLO_REPLY_WANTS_GOLD;
    }
    return DIPLO_REPLY_ACCEPT;
}

/* the lowest of the human's treaty, trade and peace interest toward the partner. The diplo_test_mode 3 branch (dead:
   the mode is never set) would set min = -1000 and then return 0; clarified using ReMoM repo */
int Diplo_Min_Interest(void)
{
    int min;

    if (diplo_test_mode == 3) {
        min = -1000;
        return 0;
    }
    min = wizards[PLAYER_HUMAN].treaty_interest[diplo_partner];
    if (wizards[PLAYER_HUMAN].trade_interest[diplo_partner] < min) {
        min = wizards[PLAYER_HUMAN].trade_interest[diplo_partner];
    }
    if (wizards[PLAYER_HUMAN].peace_interest[diplo_partner] < min) {
        min = wizards[PLAYER_HUMAN].peace_interest[diplo_partner];
    }
    return min;
}

/* () every treaty interest kept within -200 .. 120 */
void Diplo_Clamp_Treaty_Interest(void)
{
    int i;
    int j;

    for (i = 0; i < num_players; i++) {
        for (j = 0; j < num_players; j++) {
            if (i != j) {
                if (wizards[i].treaty_interest[j] > 120) {
                    wizards[i].treaty_interest[j] = 120;
                }
                if (wizards[i].treaty_interest[j] < -200) {
                    wizards[i].treaty_interest[j] = -200;
                }
            }
        }
    }
}

/* (player, spell) the player learns the spell (a trade); the diplomacy pictures reload when it is the
   current player */
void Diplo_Give_Spell(int player, int spell)
{
    Wizard_Learn_Spell(player, spell, 0);
    if (player == current_player) {
        Diplo_Load_Assets();
    }
}

/* an AI allied to two wizards at war with each other breaks both treaties
   Numbered: BUG #163 (BUGS_AND_QUIRKS.txt) */
void Diplo_Break_Conflicting_Allies(void)
{
    int i;
    int j;
    int k;

    for (i = 1; i < num_players; i++) {
        for (j = i + 1; j < num_players; j++) {
            for (k = i + 1; k < num_players; k++) {
                if (wizards[j].diplomacy_status[k] == TREATY_WAR && wizards[i].diplomacy_status[j] == TREATY_ALLIANCE
                 && wizards[i].diplomacy_status[k] == TREATY_ALLIANCE) {
                    Diplo_Break_Treaty(i, j);
                    Diplo_Break_Treaty(i, k);
                }
            }
        }
    }
}

/* status both ways (TREATY_WIZARD_PACT, TREATY_ALLIANCE, TREATY_NONE ...), hostility cleared; when the current player
   is one of the two, both sides' unit orders are cancelled (AI_Cancel_Unit_Orders) */
void Diplo_Set_Status(int a, int b, int status)
{
    wizards[a].diplomacy_status[b] = status;
    wizards[b].diplomacy_status[a] = status;
    wizards[a].hostility[b] = HOSTILITY_NONE;
    wizards[b].hostility[a] = HOSTILITY_NONE;
    if (a == current_player || b == current_player) {
        AI_Cancel_Unit_Orders(a);
        AI_Cancel_Unit_Orders(b);
    }
}

/* (a, b) peace: no treaty, a's relation to b +40 (at most 100) copied to b's, a peace timer (peace_turns) of
   8 .. 13 turns (Random(6) + 7) on the human's side, the mutual city curses removed, hostility cleared, the
   hostility replans in 16 .. 25 turns; the AI units' orders are cancelled when the current player is one
   of them
   Numbered: BUG #58 (BUGS_AND_QUIRKS.txt) */
void Diplo_Make_Peace(int a, int b)
{
    Diplo_Set_Status(a, b, TREATY_NONE);
    wizards[a].relationship[b] += 40;
    if (wizards[a].relationship[b] > 100) {
        wizards[a].relationship[b] = 100;
    }
    wizards[b].relationship[a] = wizards[a].relationship[b];
    if (a == PLAYER_HUMAN) {
        wizards[a].peace_turns[b] = Random(6) + 7;
    }
    if (b == PLAYER_HUMAN) {
        wizards[b].peace_turns[a] = Random(6) + 7;
    }
    Diplo_Remove_Mutual_City_Curses(a, b);
    wizards[a].hostility[b] = HOSTILITY_NONE;
    wizards[b].hostility[a] = HOSTILITY_NONE;
    wizards[a].hostility_replan_timer = Random(10) + 15;
    wizards[b].hostility_replan_timer = Random(10) + 15;
    if (a == current_player || b == current_player) {
        AI_Cancel_Unit_Orders(a);
        AI_Cancel_Unit_Orders(b);
    }
}

/* (a, b) a's treaty, trade and peace interest toward b each -10 */
void Diplo_Interests_Minus10(int a, int b)
{
    wizards[a].treaty_interest[b] -= 10;
    wizards[a].trade_interest[b] -= 10;
    wizards[a].peace_interest[b] -= 10;
}

/* how observer takes something actor did (amount RELATION_MESSAGE_ONLY: a message only); falls off returning the observer's hostility_replan_timer
   Numbered: BUG #47, #66, #158, #159 (BUGS_AND_QUIRKS.txt) */
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg)
{
    int orig;
    int divisor;

    if (wizards[actor].contacted[observer] == 0) {
        return 0;
    }
    if (diplo_test_mode == 3) {
        if (actor != PLAYER_HUMAN && amount < 0 && observer == PLAYER_HUMAN) {
            if (Random(100) <= abs(amount) / 10) {
                wizards[PLAYER_HUMAN].pending_rel_amount[PLAYER_HUMAN] = 100;
                wizards[PLAYER_HUMAN].pending_rel_msg[PLAYER_HUMAN] = DIPLO_MSG_WAR_BEGUN;
                wizards[PLAYER_HUMAN].pending_msg_arg[PLAYER_HUMAN] = msg_arg;
                wizards[PLAYER_HUMAN].pending_msg_city[PLAYER_HUMAN] = city;
            }
        }
        return 0;
    }
    if (actor >= MAX_PLAYERS || observer >= MAX_PLAYERS) {
        return 0;
    }
    if (actor == observer || amount == 0 || observer == PLAYER_HUMAN || wizards[actor].diplomacy_status[observer] == TREATY_FINAL_WAR
     || observer > num_players || actor > num_players) {
        return 0;
    }
    if (amount == RELATION_MESSAGE_ONLY) {
        if (actor == PLAYER_HUMAN && msg != 0) {
            wizards[PLAYER_HUMAN].pending_rel_amount[observer] = 10000;
            wizards[PLAYER_HUMAN].pending_rel_msg[observer] = msg;
            wizards[PLAYER_HUMAN].pending_msg_arg[observer] = msg_arg;
            wizards[PLAYER_HUMAN].pending_msg_city[observer] = city;
        }
        return 0;
    }
    if (amount > 0) {
        if (wizards[observer].relationship[actor] < 0) {
            amount = amount * 2;
            if (wizards[observer].relationship[actor] + amount > 10) {
                amount = 10 - wizards[observer].relationship[actor];
            }
        } else {
            divisor = wizards[observer].relationship[actor] / 25 + 1;
            if (divisor == 0) {
                divisor = 1;
            }
            amount = amount / divisor;
        }
    } else {
        if (wizards[observer].relationship[actor] > 0) {
            amount = amount * 2;
        } else {
            divisor = wizards[observer].relationship[actor] / -25 + 1;
            if (divisor == 0) {
                divisor = 1;
            }
            amount = amount / divisor;
        }
    }
    if (wizards[observer].personality == PERSONALITY_LAWFUL && amount < 0) {
        amount = amount * 2;
    }
    if (wizards[observer].personality == PERSONALITY_MANIACAL && amount < 0) {
        amount = amount * 3 / 2;
    }
    if (wizards[observer].personality == PERSONALITY_MANIACAL && amount > 0) {
        amount = amount * 3 / 4;
    }
    orig = amount;
    if (wizards[actor].retorts[RETORT_CHARISMATIC] == 1 && amount > 0) {
        amount = amount * 2;
    }
    if (wizards[actor].retorts[RETORT_CHARISMATIC] == 1 && amount < 0) {
        amount = amount / 2;
    }
    if (wizards[observer].diplomacy_status[actor] == TREATY_WAR && msg <= DIPLO_MSG_TOO_POWERFUL && amount < 0) {   /* the offense messages */
        if (wizards[observer].diplomacy_status[actor] == TREATY_WAR && actor == PLAYER_HUMAN) {
            wizards[observer].peace_interest[actor] += amount / 4;
            wizards[actor].peace_interest[observer] -= amount / 4;
        }
        amount = 0;
    }
    if (actor != 0 && observer != 0 && amount > 0 && game_turn > 200) {
        amount = amount * 2;
    }
    if (actor != 0 && observer != 0 && amount < 0 && game_turn > 200) {
        amount = amount / (difficulty / 2 + 1);
    }
    if (amount != 0) {
        wizards[observer].relationship[actor] += amount;
        if (wizards[observer].relationship[actor] < -100) {
            wizards[observer].relationship[actor] = -100;
        }
        if (wizards[observer].relationship[actor] > 100) {
            wizards[observer].relationship[actor] = 100;
        }
        wizards[observer].treaty_interest[actor] += amount;
        wizards[observer].trade_interest[actor] += amount;
        wizards[actor].treaty_interest[observer] = wizards[observer].treaty_interest[actor];
        wizards[actor].trade_interest[observer] = wizards[observer].trade_interest[actor];
        if (orig < 0) {
            wizards[actor].peace_interest[observer] -= orig / 4;
            wizards[observer].peace_interest[actor] += orig / 4;
        } else {
            wizards[observer].peace_interest[actor] += orig;
        }
        if (msg_arg == 10 && Random(100) < 11) {   /* arg is a spell from most callers, a unit index from the
                                                   unit-destruction ones (OVERCAST.C, OVERSPL3.C); no caller passes
                                                   spell 10 (Earth Lore, no city or global spell), so only destroying
                                                   unit #10 rolls this war - a stale number (ReMoM reads it as Spell of
                                                   Mastery); clarified using ReMoM repo */
            Diplo_Declare_War(actor, observer);
        }
        if (actor == PLAYER_HUMAN && msg != 0) {
            if (abs(amount) > abs(wizards[PLAYER_HUMAN].pending_rel_amount[observer])) {
                wizards[PLAYER_HUMAN].pending_rel_amount[observer] = amount;
                wizards[PLAYER_HUMAN].pending_rel_msg[observer] = msg;
                wizards[PLAYER_HUMAN].pending_msg_arg[observer] = msg_arg;
                wizards[PLAYER_HUMAN].pending_msg_city[observer] = city;
            }
        }
    }
    if (wizards[observer].diplomacy_status[actor] >= TREATY_WAR && wizards[observer].relationship[actor] > -25) {
        wizards[observer].relationship[actor] = -25;
    }
    if (wizards[observer].diplomacy_status[actor] != TREATY_ALLIANCE && wizards[observer].relationship[actor] > 65) {
        wizards[observer].relationship[actor] = 65;
    }
    wizards[actor].relationship[observer] = wizards[observer].relationship[actor];
    if (wizards[observer].diplomacy_status[actor] >= TREATY_WAR) {
        wizards[observer].hostility[actor] = HOSTILITY_WAR;
    }
    if (wizards[observer].diplomacy_status[actor] == TREATY_ALLIANCE) {
        wizards[observer].hostility[actor] = HOSTILITY_NONE;
    }
    wizards[observer].hostility_replan_timer = Random(10) + 15;
}

/* declarer goes to war with target; falls off returning the declarer's hostility_replan_timer
   Numbered: BUG #161, #297 (BUGS_AND_QUIRKS.txt) */
int Diplo_Declare_War(int declarer, int target)
{
    if (declarer == 0 && wizards[declarer].met_stage[target] == MET_NOT_YET) {
        wizards[declarer].met_stage[target] = MET_JUST_MET;
    }
    if (target == 0 && wizards[target].met_stage[declarer] == MET_NOT_YET) {
        wizards[target].met_stage[declarer] = MET_JUST_MET;
    }
    if (declarer > 5 || target > 5) {
        return 0;
    }
    if (wizards[declarer].diplomacy_status[target] >= TREATY_WAR) {
        return 0;
    }
    Diplo_Break_Treaty(declarer, target);
    wizards[declarer].base_relation[target] -= 5;
    wizards[target].base_relation[declarer] -= 5;
    wizards[declarer].relationship[target] = -75 - Random(25);
    wizards[target].relationship[declarer] = wizards[declarer].relationship[target];
    wizards[declarer].diplomacy_status[target] = TREATY_WAR;
    wizards[target].diplomacy_status[declarer] = TREATY_WAR;
    wizards[declarer].treaty_interest[target] = -200;
    wizards[target].treaty_interest[declarer] = -200;
    wizards[declarer].trade_interest[target] = -200;
    wizards[target].trade_interest[declarer] = -200;
    wizards[declarer].peace_interest[target] = -130;
    wizards[target].peace_interest[declarer] = -130;
    wizards[target].hostility[declarer] = HOSTILITY_WAR;
    wizards[declarer].hostility[target] = HOSTILITY_WAR;
    wizards[target].hostility_replan_timer = Random(10) + 15;
    wizards[declarer].hostility_replan_timer = Random(10) + 15;
}

/* breaker ends its treaty with victim (the penalty raises its own relation_bias - a sign slip; an alliance break
   raises both base_relations by the same slip); every third wizard's relation_bias toward the VICTIM (not the
   breaker) -5; falls off returning the victim's hostility_replan_timer
   Numbered: BUG #160 (BUGS_AND_QUIRKS.txt) */
int Diplo_Break_Treaty(int breaker, int victim)
{
    int penalty;
    int i;

    if (victim > 5 || breaker > 5) {
        return 0;
    }
    if (wizards[victim].diplomacy_status[breaker] >= TREATY_WAR) {
        return 0;
    }
    penalty = 0;
    if (wizards[breaker].diplomacy_status[victim] == TREATY_WIZARD_PACT) {
        penalty = -10;
    }
    if (wizards[breaker].diplomacy_status[victim] == TREATY_ALLIANCE) {
        penalty = -20;
    }
    if (wizards[breaker].personality == PERSONALITY_LAWFUL) {
        penalty = penalty * 2;
    }
    wizards[breaker].relation_bias[victim] -= penalty;
    for (i = 0; i < num_players; i++) {
        if (i != breaker && i != victim) {
            wizards[i].relation_bias[victim] -= 5;
        }
    }
    if (wizards[breaker].diplomacy_status[victim] == TREATY_ALLIANCE) {
        wizards[breaker].base_relation[victim] -= penalty;
        if (wizards[breaker].base_relation[victim] < -100) {
            wizards[breaker].base_relation[victim] = -100;
        }
        wizards[victim].base_relation[breaker] = wizards[breaker].base_relation[victim];
    }
    if (penalty != 0) {
        wizards[breaker].broken_treaty_type[victim] = wizards[breaker].diplomacy_status[victim];
        wizards[victim].broken_treaty_type[breaker] = wizards[breaker].diplomacy_status[victim];
        wizards[breaker].relationship[victim] -= Random(20);
    }
    wizards[breaker].diplomacy_status[victim] = TREATY_NONE;
    wizards[victim].diplomacy_status[breaker] = TREATY_NONE;
    wizards[breaker].msg_wizard_arg[victim] = 0;
    wizards[victim].msg_wizard_arg[breaker] = 0;
    if (wizards[breaker].relationship[victim] < -100) {
        wizards[breaker].relationship[victim] = -100;
    }
    wizards[victim].relationship[breaker] = wizards[breaker].relationship[victim];
    wizards[breaker].treaty_interest[victim] = -200;
    wizards[breaker].trade_interest[victim] = -200;
    wizards[breaker].peace_interest[victim] = -200;
    wizards[victim].treaty_interest[breaker] = -200;
    wizards[victim].trade_interest[breaker] = -200;
    wizards[victim].peace_interest[breaker] = -200;
    wizards[breaker].hostility_replan_timer = Random(10) + 15;
    wizards[victim].hostility_replan_timer = Random(10) + 15;
}

/* the spells giver could hand receiver (worth min_value or more) into spell_trade_list / _values; falls off
   returning the count */
int Diplo_List_Spells_To_Trade(int giver, int receiver, int min_value)
{
    int n;
    int i;

    spell_trade_count = 0;
    n = 0;
    n = Diplo_Tradeable_Spells(receiver, giver, min_value, spell_trade_list);
    if (n < 1) {
        spell_trade_count = 0;
        return 0;
    }
    spell_trade_count = n;
    for (i = 0; i < spell_trade_count; i++) {
        spell_trade_values[i] = Spell_Trade_Value(receiver, spell_trade_list[i]);
    }
}

/* "Offer Tribute": up to four gold amounts sized by the partner's gold, or one of your spells
   Numbered: BUG #48, #63, #67 (BUGS_AND_QUIRKS.txt) */
void Diplo_Offer_Tribute(void)
{
    int i;
    int rounded;
    int choice;
    int n_gold;
    int scale;
    int gain;
    long gold;
    int enabled[10];
    int amounts[10];
    char num_text[30];

    for (i = 0; i < 10; i++) {
        enabled[i] = 1;
    }
    gold = wizards[diplo_partner].gold;
    if (gold > 32000) {
        gold = 32000;
    }
    rounded = (int)gold / 25 * 25;
    if (rounded < 100) {
        n_gold = rounded / 25;
        amounts[0] = 25;
        amounts[1] = 50;
        amounts[2] = 75;
        amounts[3] = 100;
    } else {
        n_gold = 4;
        amounts[0] = rounded / 4 / 25 * 25;
        amounts[1] = rounded / 2 / 25 * 25;
        amounts[2] = (int)((long)rounded * 3 / 4) / 25 * 25;
        amounts[3] = rounded / 25 * 25;
    }
    choice = 0;
    strcpy(p_diplo_text, diplo_prompt_texts[0]);
    for (i = 0; i < n_gold; i++) {
        itoa(amounts[i], num_text, 10);
        strcpy(diplo_menu_items[i], "[ ");
        strcat(diplo_menu_items[i], num_text);
        strcat(diplo_menu_items[i], " gold");
        if (amounts[i] > wizards[current_player].gold) {
            enabled[i] = 0;
        }
    }
    strcpy(diplo_menu_items[i], "[ Spells");
    strcpy(diplo_menu_items[i + 1], "[ Forget It");
    diplo_menu_items[i + 2][0] = '\0';
    Font_Select(1, 4, 3);
    Set_Font_Color(0xbb);
    Set_Line_Spacing(2);
    choice = UI_Text_List_Select(0x26, 0x8e, 0xf5, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, enabled, 15, 11, 0, 0, 0, -1);
    if (choice != -1 && n_gold + 1 != choice) {
        if (choice < n_gold) {
            if (wizards[diplo_partner].stat_army_strength != 0) {
                scale = (choice + 1) * 4;
            } else {
                scale = 8;
            }
            i = Random(3);                      /* i reused: the roll */
            gain = (long)(choice + 1) * i * scale / 10;
            wizards[PLAYER_HUMAN].gold -= amounts[choice];
            Wizard_Add_Gold(diplo_partner, amounts[choice]);
            if (gain > 100) {
                gain = 100;
            }
            if (wizards[PLAYER_HUMAN].relationship[diplo_partner] + gain > 100) {
                wizards[PLAYER_HUMAN].relationship[diplo_partner] = 100;
            } else {
                wizards[PLAYER_HUMAN].relationship[diplo_partner] += gain;
            }
            if (wizards[PLAYER_HUMAN].relationship[diplo_partner] > 100) {
                wizards[PLAYER_HUMAN].relationship[diplo_partner] = 100;
            }
            wizards[diplo_partner].relationship[PLAYER_HUMAN] = wizards[PLAYER_HUMAN].relationship[diplo_partner];
            Diplo_Show_Reply(DIPLO_MSG_TRIBUTE_THANKS, 3);
        } else {
            Diplo_List_Spells_To_Trade(PLAYER_HUMAN, diplo_partner, xchg_ai_values[i]);   /* i is n_gold here (left
                                                   by the menu loop): a stale entry as the minimum value */
            if (spell_trade_count > 0) {
                choice = 0;
                for (i = 0; i < spell_trade_count && i < 4; i++) {
                    strcpy(diplo_menu_items[i], "[ ");
                    far_strcpy(num_text, 0, p_spell_data[spell_trade_list[i]].name);
                    strcat(diplo_menu_items[i], num_text);
                }
                strcpy(diplo_menu_items[i], "[ Forget It");
                diplo_menu_items[i + 1][0] = '\0';
                strcpy(p_diplo_text, diplo_prompt_texts[0]);
                Font_Select(1, 4, 3);
                Set_Font_Color(0xbb);
                choice = UI_Text_List_Select(0x26, 0x8e, 0xf5, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0, 0, -1);
                if (choice != -1 && choice < spell_trade_count && choice < 4) {
                    Diplo_Give_Spell(diplo_partner, spell_trade_list[choice]);
                    if (wizards[PLAYER_HUMAN].relationship[diplo_partner] < 0) {
                        scale = 20;
                    } else {
                        scale = (100 - wizards[PLAYER_HUMAN].relationship[diplo_partner]) / 10;
                    }
                    gain = (Random(8) + Random(8)) * scale / 10;
                    wizards[PLAYER_HUMAN].relationship[diplo_partner] += gain;
                    if (wizards[PLAYER_HUMAN].relationship[diplo_partner] > 100) {
                        wizards[PLAYER_HUMAN].relationship[diplo_partner] = 100;
                    }
                    wizards[diplo_partner].relationship[PLAYER_HUMAN] = wizards[PLAYER_HUMAN].relationship[diplo_partner];
                    wizards[PLAYER_HUMAN].peace_interest[diplo_partner] += gain;
                    if (wizards[PLAYER_HUMAN].peace_interest[diplo_partner] > 200) {
                        wizards[PLAYER_HUMAN].peace_interest[diplo_partner] = 200;
                    }
                    wizards[PLAYER_HUMAN].relation_bias[diplo_partner] = Random(8) + wizards[PLAYER_HUMAN].relation_bias[diplo_partner] + 2;
                    if (wizards[PLAYER_HUMAN].relation_bias[diplo_partner] > 30) {
                        wizards[PLAYER_HUMAN].relation_bias[diplo_partner] = 30;
                    }
                    wizards[PLAYER_HUMAN].tribute_spell_given[diplo_partner] = spell_trade_list[choice];
                    Diplo_Show_Reply(DIPLO_MSG_TRIBUTE_THANKS, 3);
                }
            } else {
                Diplo_Show_Reply(DIPLO_MSG_END_TALKS, 3);
            }
        }
    }
}

/* "Exchange Spells": one of the partner's spells for one of yours worth at least as
   much (the accept score is unused)
   Numbered: BUG #47 (BUGS_AND_QUIRKS.txt) */
void Diplo_Exchange_Spells(void)
{
    int i;
    int trade_term;
    int choice;
    int k;
    int forget_row;
    int offer_spell;
    int counter_spell;
    int n_counter;
    int row;
    int unused;
    int accept_base;
    int accept_roll;
    char spell_name[30];

    unused = 0;
    trade_term = wizards[PLAYER_HUMAN].trade_interest[diplo_partner];
    if (trade_term > 0) {
        trade_term = trade_term / 5;
    }
    if (trade_term > 30) {
        trade_term = 30;
    }
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] == TREATY_ALLIANCE) {
        trade_term += 25;
    }
    accept_base = wizards[PLAYER_HUMAN].relation_bias[diplo_partner] + wizards[PLAYER_HUMAN].relationship[diplo_partner] / 2 + unused
                + personality_treaty_mod[wizards[diplo_partner].personality] * 2 + trade_term - 125;
    accept_roll = Random(100) + accept_base;
    Diplo_List_Spells_To_Trade(diplo_partner, 0, 0);
    if (spell_trade_count > 0) {
        xchg_ai_count = spell_trade_count;
        for (i = 0; i < spell_trade_count; i++) {
            xchg_ai_spells[i] = spell_trade_list[i];
            xchg_ai_values[i] = spell_trade_values[i];
        }
        xchg_offer_count = 0;
        for (i = 0; i < xchg_ai_count; i++) {
            Diplo_List_Spells_To_Trade(PLAYER_HUMAN, diplo_partner, xchg_ai_values[i]);
            if (spell_trade_count > 0) {
                xchg_counter_count[xchg_offer_count] = spell_trade_count;
                for (k = 0; k < spell_trade_count; k++) {
                    xchg_counter_spells[xchg_offer_count][k] = spell_trade_list[k];
                }
                xchg_offer_spell[xchg_offer_count] = xchg_ai_spells[i];
                xchg_offer_count = xchg_offer_count + 1;
            }
        }
        UI_Set_Redraw_Fn(Diplo_Menu_Redraw, 3);
        if (xchg_offer_count > 0) {
            Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
            wizards[PLAYER_HUMAN].trade_interest[diplo_partner] -= Random(50) + 20;
            choice = 0;
            for (i = 0; i < xchg_offer_count && i < 5; i++) {
                strcpy(diplo_menu_items[i], "[ ");
                far_strcpy(spell_name, 0, p_spell_data[xchg_offer_spell[i]].name);
                strcat(diplo_menu_items[i], spell_name);
            }
            diplo_menu_items[i][0] = '\0';
            strcpy(p_diplo_text, diplo_prompt_texts[1]);
            Font_Select(1, 4, 3);
            Set_Font_Color(0xbb);
            choice = UI_Text_List_Select(0x26, 0x8e, 0xf5, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0, 0, -1);
            if (choice != -1) {
                offer_spell = xchg_offer_spell[choice];
                n_counter = xchg_counter_count[choice];
                row = choice;
                for (i = 0; i < n_counter && i < 4; i++) {
                    strcpy(diplo_menu_items[i], "[ ");
                    far_strcpy(spell_name, 0, p_spell_data[xchg_counter_spells[row][i]].name);
                    strcat(diplo_menu_items[i], spell_name);
                }
                strcpy(diplo_menu_items[i], "[ Forget It");
                forget_row = i;
                diplo_menu_items[forget_row + 1][0] = '\0';
                choice = 0;
                strcpy(p_diplo_text, diplo_prompt_texts[2]);
                Font_Select(1, 4, 3);
                Set_Font_Color(0xbb);
                choice = UI_Text_List_Select(0x26, 0x8e, 0xf5, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0, 0, -1);
                if (choice != -1 && choice != forget_row) {
                    counter_spell = xchg_counter_spells[row][choice];
                    Diplo_Give_Spell(current_player, offer_spell);
                    Diplo_Give_Spell(diplo_partner, counter_spell);
                    if ((unsigned)p_spell_data[offer_spell].research_cost < p_spell_data[counter_spell].research_cost) {
                        Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
                        Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
                        Diplo_Relation_Change(5, PLAYER_HUMAN, diplo_partner, 0, 0, 0);
                    }
                }
            }
        } else {
            Diplo_Show_Reply(DIPLO_MSG_NO_SPELL_WANTED, 3);
        }
    } else {
        Diplo_Show_Reply(DIPLO_MSG_NO_SPELL_WANTED, 3);
    }
    Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
    Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
    Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
    if (wizards[PLAYER_HUMAN].trade_interest[diplo_partner] > 50) {
        wizards[PLAYER_HUMAN].trade_interest[diplo_partner] = 50;
    }
}

/* at peace: each lifts its curses from the other's cities (an enchantment slot holds caster + 1)
   Numbered: BUG #59 (BUGS_AND_QUIRKS.txt) */
void Diplo_Remove_Mutual_City_Curses(int a, int b)
{
    int city;
    char far *ench;
    int i;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == a || p_cities[city].owner == b) {
            ench = p_cities[city].enchants;
            for (i = 0; i < NUM_CITY_ENCHANTS; i++) {
                if (ench[i] > 0) {
                    if (p_cities[city].owner == a) {
                        if (ench[i] - 1 == b) {
                            ench[i] = 0;
                        }
                    } else {
                        if (ench[i] - 1 == a) {
                            ench[i] = 0;
                        }
                    }
                }
            }
        }
    }
}
