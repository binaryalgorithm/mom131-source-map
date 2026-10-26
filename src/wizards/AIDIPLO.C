/* AIDIPLO.C - WIZARDS.EXE overlay code segment 7442 (stub 24bf): the AI wizards' side of diplomacy with the human -
   the proposals an AI makes (a Wizard Pact, an Alliance or a spell exchange, with a tribute incentive), its reaction
   to an offense (warnings, a broken treaty, war, or a peace offer while at war), the per-turn drift of the treaty,
   trade and peace interests with the clearing of the per-turn message state, and three unused helpers.
   No data of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7442 src/wizards/AIDIPLO.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int AI_Diplo_Propose_To_Human(int player, int ai);
int AI_React_To_Offense(int ai);
void Diplo_Interest_Drift(void);
int Diplo_Alt_Contact_Always_0(int ai);
int Diplo_Army_Ratio_Roll_Unused(int wizard_a, int wizard_b);
int Diplo_Pop_Ratio_Roll_Unused(int wizard_a, int wizard_b);

#include "GAME.H"

extern int personality_treaty_mod[6];           /* DS:1fc8  by personality: Maniacal 0 .. Peaceful 50 */
extern int game_turn;                           /* DS:bd90 */
extern unsigned char xchg_ai_values[10];        /* DS:c399  [0]: the AI proposal's value limit */
extern unsigned char xchg_ai_spells[10];        /* DS:c3a3  [0]: the spell the AI asks for */
extern unsigned char spell_trade_values[10];    /* DS:c3ae */
extern unsigned char spell_trade_list[10];      /* DS:c3b8 */
extern unsigned char spell_trade_count;         /* DS:c3c2 */

int Diplo_List_Spells_To_Trade(int giver, int receiver, int min_value);
int Diplo_Declare_War(int declarer, int target);
int Diplo_Break_Treaty(int breaker, int victim);
int abs(int x);

/* AI_Diplo_Propose_To_Human's proposal: 1 / 2 = the treaty (TREATY_WIZARD_PACT / _ALLIANCE), 5 a spell trade */
#define PROPOSE_NONE 0
#define PROPOSE_SPELLS 5

/* (player 0, AI ai) -> 0 when nothing happens (no other return value): needs mutual contact (the alternative gate
   reads wizards[PLAYER_HUMAN].pending_rel_amount[PLAYER_HUMAN], the human's own slot, never written - always 0). Unless a message is pending (then 1 in 2):
   S = bias + relation + personality_treaty_mod; a pact (no treaty, relation > 15, Random(100) + S + treaty
   interest >= 50), an alliance (pact or the pact roll, relation > 50, a second roll of the same formula >= 100), else a spell
   exchange: the human's first spell the AI lacks against the AI's spells worth at least its price (value x 100 /
   price_divisor, passed as the minimum), kept only when one is worth no more than that price; else a pact when the AI's population is smaller than the human's (relation above 0, no
   treaty). Messages DIPLO_MSG_OFFER_PACT / _OFFER_ALLIANCE / _OFFER_SPELL_TRADE;
   a pact or alliance may carry a tribute (BUG #166: the gold is sized by population and paid from nothing).
   Dead vestige: a join-war gold amount computed into join_gold, never read (#165). player is always 0 (DIPLOTRN.C):
   the body also says PLAYER_HUMAN and current_player for that wizard */
int AI_Diplo_Propose_To_Human(int player, int ai)
{
    int proposal;                               /* PROPOSE_*: none, pact, alliance, spells */
    int score;
    int roll;
    int war_target;
    int join_gold;
    int price_divisor;
    int found;
    int offer_price;                            /* the human's spell priced for the AI: the AI's spells must be worth this
                                                   much or more (Diplo_List_Spells_To_Trade's minimum) */
    int unused_term;
    int trade;
    int i;

    if ((wizards[current_player].contacted[ai] == 0 || wizards[ai].contacted[current_player] == 0)
        && (wizards[PLAYER_HUMAN].pending_rel_amount[PLAYER_HUMAN] == 0 || wizards[PLAYER_HUMAN].diplomacy_status[ai] >= TREATY_WAR || Diplo_Alt_Contact_Always_0(ai) == 1)) {
        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
        return 0;
    }
    if (wizards[player].pending_rel_msg[ai] != DIPLO_MSG_NONE && Random(2) == 1)
        return 0;
    proposal = PROPOSE_NONE;
    unused_term = 0;
    score = wizards[player].relation_bias[ai] + wizards[player].relationship[ai] + unused_term
        + personality_treaty_mod[wizards[ai].personality];
    if (wizards[player].diplomacy_status[ai] == TREATY_NONE && wizards[player].relationship[ai] > 15) {
        roll = score + Random(100) + wizards[player].treaty_interest[ai];
        if (roll >= 50)
            proposal = TREATY_WIZARD_PACT;
    }
    if ((wizards[player].diplomacy_status[ai] == TREATY_WIZARD_PACT || proposal == TREATY_WIZARD_PACT) && wizards[player].relationship[ai] > 50) {
        roll = score + Random(100) + wizards[player].treaty_interest[ai];
        if (roll >= 100)
            proposal = TREATY_ALLIANCE;
    }
    if (proposal == PROPOSE_NONE) {
        war_target = 0;
        for (i = 1; i < num_players; i++) {
            if (i != ai && wizards[ai].diplomacy_status[i] >= TREATY_WAR && wizards[current_player].contacted[i] != 1)
                war_target = i;
        }
        if (war_target != 0 && Random(10) == 1 && wizards[player].msg_wizard_arg[ai] == 0) {
            Diplo_List_Spells_To_Trade(ai, 0, 0);
            join_gold = 0;
            if (spell_trade_count == 0)
                join_gold = (Random(3) + 1) * game_turn / 50 * 50;
        }
    }
    if (proposal == PROPOSE_NONE) {
        roll = score + Random(100);
        if (roll >= 25) {
            unused_term = 0;
            trade = wizards[player].trade_interest[ai];
            if (trade > 0)
                trade = trade / 5;
            if (trade > 30)
                trade = 30;
            if (wizards[ai].diplomacy_status[player] == TREATY_ALLIANCE)
                trade += 25;
            score = wizards[player].relation_bias[ai] + wizards[player].relationship[ai] / 2 + unused_term
                + personality_treaty_mod[wizards[ai].personality] + trade - 125;
            roll = score + Random(100) + trade - 125;   /* trade and - 125 a second time: score holds them already */
            if (roll < 0)
                price_divisor = abs(roll) + 100;
            else
                price_divisor = 20000 / (roll + 200);
            if (price_divisor < 50)
                price_divisor = 50;
            Diplo_List_Spells_To_Trade(0, ai, 0);
            if (spell_trade_count > 0) {
                xchg_ai_spells[0] = spell_trade_list[0];
                xchg_ai_values[0] = (long)spell_trade_values[0] * 100 / price_divisor;
                offer_price = xchg_ai_values[0];
                Diplo_List_Spells_To_Trade(ai, 0, offer_price);
                found = 0;
                for (i = 0; i < spell_trade_count; i++) {
                    if (spell_trade_values[i] <= xchg_ai_values[0])
                        found = 1;
                }
                if (found != 0)
                    proposal = PROPOSE_SPELLS;
            }
        }
    }
    if (proposal == PROPOSE_NONE && wizards[PLAYER_HUMAN].relationship[ai] > 0 && wizards[ai].stat_population < wizards[PLAYER_HUMAN].stat_population
        && wizards[PLAYER_HUMAN].diplomacy_status[ai] == TREATY_NONE)
        proposal = TREATY_WIZARD_PACT;
    switch (proposal) {
    case TREATY_WIZARD_PACT:
        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_OFFER_PACT;
        break;
    case TREATY_ALLIANCE:
        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_OFFER_ALLIANCE;
        break;
    case PROPOSE_SPELLS:
        if (spell_trade_count > 0) {
            wizards[PLAYER_HUMAN].spells_offered_count[ai] = 0;
            for (i = 0; i < spell_trade_count && i < 6; i++) {
                if (spell_trade_values[i] <= xchg_ai_values[0]) {
                    wizards[PLAYER_HUMAN].spells_offered[ai][wizards[PLAYER_HUMAN].spells_offered_count[ai]] = spell_trade_list[i];
                    wizards[PLAYER_HUMAN].spells_offered_count[ai] = wizards[PLAYER_HUMAN].spells_offered_count[ai] + 1;
                }
            }
            if (wizards[PLAYER_HUMAN].spells_offered_count[ai] > 0) {
                wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_OFFER_SPELL_TRADE;
                wizards[PLAYER_HUMAN].wanted_spell[ai] = xchg_ai_spells[0];
            }
            wizards[ai].trade_interest[PLAYER_HUMAN] = 0;
            wizards[PLAYER_HUMAN].trade_interest[ai] = 0;
        }
        break;
    }
    if (wizards[ai].stat_population != 0 && (proposal == TREATY_WIZARD_PACT || proposal == TREATY_ALLIANCE) && wizards[ai].personality > PERSONALITY_RUTHLESS
        && Random(100) <= wizards[PLAYER_HUMAN].stat_population * 30 / wizards[ai].stat_population && Random(8) == 1) {
        if (Random(4) > 1) {
            /* BUG #166: the tribute gold is sized by the AI's population */
            wizards[PLAYER_HUMAN].tribute_gold[ai] = (wizards[ai].stat_population / (Random(3) + 1) + game_turn) / 25 * 25;
        } else {
            Diplo_List_Spells_To_Trade(ai, 0, 0);
            if (spell_trade_count > 0)
                wizards[PLAYER_HUMAN].tribute_spell[ai] = spell_trade_list[0];
        }
    }
}

/* (AI ai) -> 0 when nothing happens (no other return value): needs contact and a pending relation amount.
   S = bias + relation + personality_treaty_mod (human row). Not at war: relation < -74 -> the AI declares war
   (DIPLO_MSG_WAR_BEGUN); else S - Random(100) + treaty interest < -99 -> a warning (count 0 -> 1, then +1); from the
   second warning a treaty is broken (Diplo_Break_Treaty) while the count is <= 2 and a treaty exists, otherwise
   Random(100) < -(pending amount + pmod / 2) -> war (DIPLO_MSG_WAR_BEGUN), count 1. At war: S + Random(100) + peace interest
   >= 50 and 1 in 8 -> a peace offer (DIPLO_MSG_OFFER_PEACE) with an incentive when Random(100) <= 30 x the AI's population /
   the human's: 1 in 4 its first spell the human lacks, else gold (BUG #166) */
int AI_React_To_Offense(int ai)
{
    int score;
    int unused_term;
    int roll;

    unused_term = 0;
    if (wizards[current_player].contacted[ai] != 1 || wizards[PLAYER_HUMAN].pending_rel_amount[ai] == 0) {
        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
        return 0;
    }
    score = wizards[PLAYER_HUMAN].relation_bias[ai] + wizards[PLAYER_HUMAN].relationship[ai] + unused_term
        + personality_treaty_mod[wizards[ai].personality];
    if (wizards[PLAYER_HUMAN].diplomacy_status[ai] < TREATY_WAR) {
        if (wizards[PLAYER_HUMAN].relationship[ai] <= RELATION_WAR_THRESHOLD) {
            Diplo_Declare_War(ai, 0);
            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_WAR_BEGUN;
        } else {
            roll = score - Random(100) + wizards[PLAYER_HUMAN].treaty_interest[ai];
            if (roll <= -100) {
                if (wizards[PLAYER_HUMAN].warning_count[ai] > 0) {
                    wizards[PLAYER_HUMAN].warning_count[ai] = wizards[PLAYER_HUMAN].warning_count[ai] + 1;
                    if (wizards[PLAYER_HUMAN].warning_count[ai] > 2 || wizards[PLAYER_HUMAN].diplomacy_status[ai] == TREATY_NONE) {
                        if (Random(100) < -(wizards[PLAYER_HUMAN].pending_rel_amount[ai]
                                            + personality_treaty_mod[wizards[ai].personality] / 2)) {
                            Diplo_Declare_War(ai, 0);
                            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_WAR_BEGUN;
                            wizards[PLAYER_HUMAN].warning_count[ai] = 1;
                        }
                    } else {
                        Diplo_Break_Treaty(ai, 0);
                    }
                } else {
                    wizards[PLAYER_HUMAN].warning_count[ai] = 1;
                }
            }
        }
    } else {
        roll = score + Random(100) + wizards[PLAYER_HUMAN].peace_interest[ai];
        if (roll >= 50 && Random(8) == 1) {
            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_OFFER_PEACE;
            if (Random(100) <= wizards[ai].stat_population * 30 / wizards[PLAYER_HUMAN].stat_population) {
                if (Random(4) > 1) {
                    /* BUG #166: the tribute gold is sized by the AI's population */
                    wizards[PLAYER_HUMAN].tribute_gold[ai] = (wizards[ai].stat_population / (Random(3) + 1) + game_turn) / 25 * 25;
                } else {
                    Diplo_List_Spells_To_Trade(ai, 0, 0);
                    if (spell_trade_count > 0)
                        wizards[PLAYER_HUMAN].tribute_spell[ai] = spell_trade_list[0];
                }
            }
        }
    }
}

/* (): for every pair (wizard, AI other, status not Final War) the treaty, trade and peace interests each grow by
   Random(5) when below 0 and again when below 50; peace interest clamped to -200..200. Then for every pair the
   per-turn message state is cleared (Next_Turn) */
void Diplo_Interest_Drift(void)
{
    int msg;                                    /* read, never used */
    int other;
    int i;
    int j;

    for (i = 0; i < num_players; i++) {
        for (j = 1; j < num_players; j++) {
            if (j != i) {
                msg = wizards[i].pending_rel_msg[j];
                if (wizards[i].diplomacy_status[j] != TREATY_FINAL_WAR) {
                    if (wizards[i].treaty_interest[j] < 0)
                        wizards[i].treaty_interest[j] += Random(5);
                    if (wizards[i].treaty_interest[j] < 50)
                        wizards[i].treaty_interest[j] += Random(5);
                    if (wizards[i].trade_interest[j] < 0)
                        wizards[i].trade_interest[j] += Random(5);
                    if (wizards[i].trade_interest[j] < 50)
                        wizards[i].trade_interest[j] += Random(5);
                    if (wizards[i].peace_interest[j] < 0)
                        wizards[i].peace_interest[j] += Random(5);
                    if (wizards[i].peace_interest[j] < 50)
                        wizards[i].peace_interest[j] += Random(5);
                    if (wizards[i].peace_interest[j] < -200)
                        wizards[i].peace_interest[j] = -200;
                    if (wizards[i].peace_interest[j] > 200)
                        wizards[i].peace_interest[j] = 200;
                }
            }
        }
    }
    for (other = 0; other < num_players; other++) {
        for (j = 0; j < num_players; j++) {
            wizards[j].pending_rel_amount[other] = 0;
            wizards[j].pending_rel_msg[other] = DIPLO_MSG_NONE;
            if (wizards[j].pay_for_war_pending[other] != 0) {
                wizards[j].msg_wizard_arg[other] = 0;
                wizards[j].pay_for_war_pending[other] = 0;
            }
            wizards[j].spells_offered_count[other] = 0;
            wizards[j].tribute_spell[other] = SPELL_NONE;
            wizards[j].wanted_spell[other] = SPELL_NONE;
            wizards[j].tribute_gold[other] = 0;
        }
    }
}

/* (ai) -> 0: a stub (AI_Diplo_Propose_To_Human's alternative contact gate) */
int Diplo_Alt_Contact_Always_0(int ai)
{
    return 0;
}

/* (wizard_a, wizard_b) -> +-Random(|r|), 0 when r = 0: r = (army a - army b) x 100 / army b (each at most 3250000 and at least
   1), clamped to +-300. No caller or code pointer in the EXE */
int Diplo_Army_Ratio_Roll_Unused(int wizard_a, int wizard_b)
{
    long va;
    long vb;
    long ratio;
    int bound;

    va = wizards[wizard_a].stat_army_strength;
    vb = wizards[wizard_b].stat_army_strength;
    if (va > 3250000L)
        va = 3250000L;
    if (va == 0)
        va = 1;
    if (vb > 3250000L)
        vb = 3250000L;
    if (vb == 0)
        vb = 1;
    ratio = (va - vb) * 100 / vb;
    if (ratio > 300)
        ratio = 300;
    if (ratio < -300)
        ratio = -300;
    bound = ratio;
    if (bound == 0)
        return 0;
    else if (bound < 0)
        return -Random(-bound);
    else
        return Random(bound);
}

/* (wizard_a, wizard_b): the same with the population (only b's is clamped). No caller or code pointer in the EXE */
int Diplo_Pop_Ratio_Roll_Unused(int wizard_a, int wizard_b)
{
    long va;
    long vb;
    long ratio;
    int bound;

    va = wizards[wizard_a].stat_population;
    vb = wizards[wizard_b].stat_population;
    if (vb > 3250000L)
        vb = 3250000L;
    if (vb == 0)
        vb = 1;
    ratio = (va - vb) * 100 / vb;
    if (ratio > 300)
        ratio = 300;
    if (ratio < -300)
        ratio = -300;
    bound = ratio;
    if (bound == 0)
        return 0;
    else if (bound < 0)
        return -Random(-bound);
    else
        return Random(bound);
}
