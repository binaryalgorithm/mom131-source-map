/* DIPLOTRN.C - WIZARDS.EXE overlay code segment 736d (stub 24ba): the AI wizards' diplomatic turn toward the human -
   first contact, greetings, reactions to offenses and proposals (AIDIPLO.C), the war checks (Chaotic whims, army
   ratios, the human's wars, the difficulty quota), breaking a treaty or declaring war, an ally's war request, the
   border tension of units parked near an ally's cities; showing the pending messages; the per-turn peace_turns countdown
   and interest regrowth.
   No data of its own.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 736d src/wizards/DIPLOTRN.C */
/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Diplo_Mark_Met(void);
int AI_Diplo_Turn_Vs_Human(void);
int AI_War_Check(int target, int ai);
int AI_Break_Or_Declare(int target, int ai);
void AI_Ally_Pick_War_Request(int ai);
int Diplo_Show_Pending_Messages(void);
int AI_Border_Tension(int city_owner, int unit_owner);
void Diplo_Tick_Peace_Turns(void);
void Diplo_Interest_Regrow(void);

#include "GAME.H"

extern int personality_treaty_mod[6];           /* DS:1fc8 */
extern int diplo_test_mode;                     /* DS:4c44  never set: the test branch of AI_War_Check is dead */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern int diplo_partner;                       /* DS:c3fe */

void Diplo_Clamp_Treaty_Interest(void);
int Diplo_Min_Interest(void);
int AI_React_To_Offense(int ai);
int AI_Diplo_Propose_To_Human(int player, int ai);
int Diplo_Army_Near_Fortress(int a, int b, long *army_a, long *army_b);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
int Diplo_Break_Treaty(int breaker, int victim);
int Diplo_Declare_War(int declarer, int target);
int Screen_Diplomacy_Incoming(void);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int abs(int x);

/* () every AI wizard the current player has contacted but not met: met (stage 1) and no treaty both ways
   Numbered: BUG #297 (BUGS_AND_QUIRKS.txt) */
void Diplo_Mark_Met(void)
{
    int player;

    for (player = 1; player < num_players; player++) {
        if (wizards[current_player].met_stage[player] == MET_NOT_YET && wizards[current_player].contacted[player] == 1) {
            wizards[current_player].met_stage[player] = MET_JUST_MET;
            wizards[current_player].diplomacy_status[player] = TREATY_NONE;
            wizards[player].diplomacy_status[current_player] = TREATY_NONE;
        }
    }
}

/* () -> 0 while the human casts Spell of Return (no return value otherwise): per AI wizard with a fortress: Final
   War clears its message; just met -> greeting (message personality + 15); pay_for_war_pending -> message 2; else (no war
   message pending) score = bias + relation + the lowest interest + personality_treaty_mod + relation again (the
   relation counts twice): >= -100 clears the message; below, an offense pending this turn -> AI_React_To_Offense
   (skipped when Random(75) >= |2 x amount| and no warnings), at war and Random(100) + 30 < peace interest ->
   AI_React_To_Offense, else Random(100) < 2 x amount (+3 without warnings) and 1 in 4 -> a proposal
   (AI_Diplo_Propose_To_Human, or the warnings reset), otherwise AI_Ally_Pick_War_Request; then AI_War_Check and
   AI_Border_Tension while no message is pending
   Numbered: BUG #46, #67 (BUGS_AND_QUIRKS.txt) */
int AI_Diplo_Turn_Vs_Human(void)
{
    int interest;
    int score;
    int ai;
    int chance;

    if (wizards[current_player].casting_spell == SPELL_SPELL_OF_RETURN)
        return 0;
    Diplo_Clamp_Treaty_Interest();
    for (ai = 1; ai < num_players; ai++) {
        if (p_fortresses[ai].active == 0) {
            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
            continue;
        }
        if (wizards[PLAYER_HUMAN].diplomacy_status[ai] == TREATY_FINAL_WAR)
            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
        else if (wizards[PLAYER_HUMAN].met_stage[ai] == MET_JUST_MET) {
            wizards[PLAYER_HUMAN].met_stage[ai] = MET_GREETED;
            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = wizards[ai].personality + DIPLO_MSG_GREETING_BASE;
        } else if (wizards[PLAYER_HUMAN].pay_for_war_pending[ai] != 0)
            wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_PAY_FOR_WAR;
        else if (wizards[PLAYER_HUMAN].pending_rel_msg[ai] == DIPLO_MSG_WAR_BEGUN || wizards[PLAYER_HUMAN].pending_rel_msg[ai] == DIPLO_MSG_TREATY_INCONVENIENT
                 || wizards[PLAYER_HUMAN].pending_rel_msg[ai] == DIPLO_MSG_WAR_BORED || wizards[PLAYER_HUMAN].pending_rel_msg[ai] == DIPLO_MSG_WAR_TIRED_OF_GAMES) {
        } else {
            diplo_partner = ai;
            interest = Diplo_Min_Interest();
            score = wizards[PLAYER_HUMAN].relation_bias[diplo_partner] + wizards[PLAYER_HUMAN].relationship[diplo_partner] + interest
                    + personality_treaty_mod[wizards[diplo_partner].personality];
            if (score + wizards[PLAYER_HUMAN].relationship[diplo_partner] >= -100)
                wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
            else {
                chance = wizards[PLAYER_HUMAN].pending_rel_amount[ai] * 2;
                if (wizards[PLAYER_HUMAN].pending_rel_amount[ai] < 0) {
                    if (wizards[PLAYER_HUMAN].warning_count[ai] > 0 || Random(75) < abs(chance))
                        AI_React_To_Offense(ai);
                    else
                        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
                } else if (wizards[PLAYER_HUMAN].diplomacy_status[ai] == TREATY_WAR && Random(100) + 30 < wizards[PLAYER_HUMAN].peace_interest[ai])
                    AI_React_To_Offense(ai);
                else {
                    if (wizards[PLAYER_HUMAN].warning_count[ai] == 0)
                        chance += 3;
                    if (Random(100) < chance && Random(4) == 1) {
                        if (wizards[PLAYER_HUMAN].warning_count[ai] > 0)
                            wizards[PLAYER_HUMAN].warning_count[ai] = 0;
                        else
                            AI_Diplo_Propose_To_Human(0, ai);
                    } else
                        AI_Ally_Pick_War_Request(ai);
                }
            }
        }
        if (wizards[PLAYER_HUMAN].pending_rel_msg[ai] == DIPLO_MSG_NONE)
            AI_War_Check(0, ai);
        if (wizards[PLAYER_HUMAN].pending_rel_msg[ai] == DIPLO_MSG_NONE)
            AI_Border_Tension(ai, 0);
    }
}

/* (target, AI ai) -> 0 when skipped (no return value otherwise): not before turn 100 against the current player,
   only without war, with contact and the target's fortress standing. A Chaotic AI: Random(300) <= difficulty (and
   no peace timer toward the human) -> DIPLO_MSG_WAR_BORED (amount 2000) and war. Otherwise two 1-in-20 rolls (the first
   only for a Militarist or Expansionist that is not Lawful): score = relation + army term (the armies on the
   landmass of the AI's fortress, Diplo_Army_Near_Fortress: the AI's larger -> -50 - 100 x ai / target, else 100 x target / ai - 50;
   +-100 for an empty side) + personality_treaty_mod + bias <= -150 -> AI_Break_Or_Declare. Then the two rules of
   #164 that read the human's wars (an ally of the AI at war with the human, 1 in 10; the difficulty quota of
   wars with the human: relation < -30, no peace timer, Random(15) <= -relation / 10)
   Numbered: BUG #164 (BUGS_AND_QUIRKS.txt) */
int AI_War_Check(int target, int ai)
{
    int score;
    int at_war;
    int player;
    int k;
    int army;
    long target_army;
    long ai_army;

    if (diplo_test_mode != 0) {
        for (player = 1; player < num_players; player++)
            for (k = 1; k < num_players; k++)
                if (player != k)
                    wizards[player].diplomacy_status[k] = TREATY_ALLIANCE;
        return 0;
    }
    if (target == current_player && game_turn < 100)
        return 0;
    if (wizards[target].diplomacy_status[ai] >= TREATY_WAR)
        return 0;
    /* the war test again, as in the original */
    if (wizards[target].diplomacy_status[ai] >= TREATY_WAR || wizards[target].contacted[ai] != 1
        || p_fortresses[target].active == 0)
        return 0;
    if (wizards[ai].personality == PERSONALITY_CHAOTIC && Random(300) <= difficulty
        && (target != 0 || wizards[target].peace_turns[ai] < 1)) {
        wizards[target].pending_rel_msg[ai] = DIPLO_MSG_WAR_BORED;
        wizards[target].pending_rel_amount[ai] = 2000;
        Diplo_Declare_War(ai, target);
    } else {
        if (Random(20) == 1 && (wizards[ai].objective == OBJECTIVE_MILITARIST || wizards[ai].objective == OBJECTIVE_EXPANSIONIST)
            && wizards[ai].personality != PERSONALITY_LAWFUL && (target != 0 || wizards[target].peace_turns[ai] < 1)) {
            target_army = 0;
            target_army = 0;                    /* sic: ai_army is not cleared */
            Diplo_Army_Near_Fortress(target, ai, &target_army, &ai_army);
            if (ai_army > target_army) {
                if (target_army > 0)
                    army = -(ai_army * 100 / target_army) - 50;
                else
                    army = -100;
            } else {
                if (ai_army > 0)
                    army = target_army * 100 / ai_army - 50;
                else
                    army = 100;
            }
            score = wizards[target].relationship[ai] + army + personality_treaty_mod[wizards[ai].personality]
                    + wizards[target].relation_bias[ai];
            if (score <= -150)
                AI_Break_Or_Declare(target, ai);
        }
        if (Random(20) == 1 && (target != 0 || wizards[target].peace_turns[ai] < 1)) {
            target_army = 0;
            target_army = 0;                    /* sic: ai_army is not cleared */
            Diplo_Army_Near_Fortress(target, ai, &target_army, &ai_army);
            if (ai_army > target_army) {
                if (target_army > 0)
                    army = -(ai_army * 100 / target_army) - 50;
                else
                    army = -100;
            } else {
                if (ai_army > 0)
                    army = target_army * 100 / ai_army - 50;
                else
                    army = 100;
            }
            score = wizards[target].relationship[ai] + army + personality_treaty_mod[wizards[ai].personality]
                    + wizards[target].relation_bias[ai];
            if (score <= -150)
                AI_Break_Or_Declare(target, ai);
        }
    }
    for (player = 1; player < num_players; player++) {
        if (wizards[PLAYER_HUMAN].diplomacy_status[player] == TREATY_WAR && wizards[ai].diplomacy_status[player] == TREATY_ALLIANCE
            && wizards[target].peace_turns[ai] < 1 && Random(10) == 1 && wizards[ai].diplomacy_status[target] != TREATY_ALLIANCE)
            AI_Break_Or_Declare(target, ai);
    }
    if (difficulty < DIFFICULTY_NORMAL)
        return 0;
    at_war = 0;
    for (player = 1; player < num_players; player++) {
        if (wizards[PLAYER_HUMAN].diplomacy_status[player] >= TREATY_WAR)
            at_war++;
    }
    if (at_war >= difficulty)
        return 0;
    if (wizards[target].relationship[ai] < -30 && wizards[target].peace_turns[ai] < 1) {
        score = -wizards[target].relationship[ai] / 10;
        if (Random(15) <= score)
            AI_Break_Or_Declare(target, ai);
    }
}

/* (target, AI ai) -> 0 when skipped (no return value otherwise): with a treaty - never below difficulty 2 - an
   alliance 1 in 4 (5 in 8 for an Expansionist), a pact 1 in 2 (always for an Expansionist) is broken
   (DIPLO_MSG_TREATY_INCONVENIENT - queued only when the target is the human -, Diplo_Break_Treaty, relations capped at 30 both ways); without one and no peace timer:
   war (DIPLO_MSG_WAR_BEGUN when the relation is negative, else DIPLO_MSG_WAR_TIRED_OF_GAMES) */
int AI_Break_Or_Declare(int target, int ai)
{
    if (wizards[target].diplomacy_status[ai] == TREATY_ALLIANCE || wizards[target].diplomacy_status[ai] == TREATY_WIZARD_PACT) {
        if (difficulty < DIFFICULTY_NORMAL)
            return 0;
        if ((wizards[target].diplomacy_status[ai] == TREATY_ALLIANCE
             && (Random(4) == 1 || (Random(2) == 1 && wizards[ai].objective == OBJECTIVE_EXPANSIONIST)))
            || (wizards[target].diplomacy_status[ai] == TREATY_WIZARD_PACT
                && (Random(2) == 1 || wizards[ai].objective == OBJECTIVE_EXPANSIONIST))) {
            Diplo_Relation_Change(RELATION_MESSAGE_ONLY, target, ai, DIPLO_MSG_TREATY_INCONVENIENT, 0, 0);
            Diplo_Break_Treaty(ai, target);
            if (wizards[target].relationship[ai] > 30) {
                wizards[target].relationship[ai] = 30;
                wizards[ai].relationship[target] = 30;
            }
        }
    } else if (wizards[target].peace_turns[ai] < 1) {
        if (wizards[target].relationship[ai] < 0)
            Diplo_Relation_Change(RELATION_MESSAGE_ONLY, target, ai, DIPLO_MSG_WAR_BEGUN, 0, 0);
        else
            Diplo_Relation_Change(RELATION_MESSAGE_ONLY, target, ai, DIPLO_MSG_WAR_TIRED_OF_GAMES, 0, 0);
        Diplo_Declare_War(ai, target);
    }
}

/* (AI ai) allied with the human, 1 in 10: war_request_target = the last wizard (not the human, not ai) at war with
   ai, met by the human and not at war with him (-1 none). Always clears the pending message - DIPLO_MSG_DEMAND_BREAK_TREATY is
   never queued, so the request is never shown (#165)
   Numbered: BUG #165 (BUGS_AND_QUIRKS.txt) */
void AI_Ally_Pick_War_Request(int ai)
{
    int human;
    int ally;
    int target;
    int i;

    human = PLAYER_HUMAN;
    ally = ai;                                  /* human / ally: copies of PLAYER_HUMAN and ai (the original's locals) */
    if (wizards[human].diplomacy_status[ally] == TREATY_ALLIANCE && Random(10) == 1) {
        target = -1;
        for (i = 0; i < num_players; i++) {
            if (i != human && i != ally && wizards[i].diplomacy_status[ally] == TREATY_WAR && wizards[human].contacted[i] == 1
                && wizards[human].diplomacy_status[i] != TREATY_WAR)
                target = i;
        }
        wizards[PLAYER_HUMAN].war_request_target[ally] = target;
        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
    } else
        wizards[PLAYER_HUMAN].pending_rel_msg[ai] = DIPLO_MSG_NONE;
}

/* () -> 0 while the human casts Spell of Return (no return value otherwise): the incoming diplomacy screen for every
   wizard with a pending message; every message cleared */
int Diplo_Show_Pending_Messages(void)
{
    int i;

    if (wizards[current_player].casting_spell == SPELL_SPELL_OF_RETURN)
        return 0;
    for (i = 1; i < num_players; i++) {
        if (wizards[PLAYER_HUMAN].pending_rel_msg[i] != DIPLO_MSG_NONE) {
            diplo_partner = i;
            Screen_Diplomacy_Incoming();
        }
        wizards[PLAYER_HUMAN].pending_rel_msg[i] = DIPLO_MSG_NONE;
    }
}

/* (city owner, unit owner) -> 0 at war (TREATY_WAR only: Final War goes on as no treaty; no return
   value otherwise): the first unit of the unit owner (not a settler -
   AFLAG_CREATE_OUTPOST - and no engineer) within distance 2 of a city of the city owner, on its plane or in a
   tower. None -> border_warning_stage 0. With a treaty: stage 0 -> Diplo_Relation_Change(-Random(5),
   DIPLO_MSG_TOO_CLOSE_TO_CITY, the city) and stage 1; stage 1 -> 2; stage 2 -> 0, Relation_Change(-5, DIPLO_MSG_REMEMBER_THIS_DAY) and Diplo_Break_Treaty
   both ways (the second call runs on the already-cleared status). Without one: stage 0 and 5 in 100 (Random(100) < 6) ->
   Relation_Change(-Random(5)), DIPLO_MSG_TOO_CLOSE_TO_CITY one time in 5. So a unit parked next to an ally's city costs the
   treaty on the third consecutive check */
int AI_Border_Tension(int city_owner, int unit_owner)
{
    int unit;
    int city;
    int dist;
    int found;
    int border_city;
    int amount;

    if (wizards[unit_owner].diplomacy_status[city_owner] == TREATY_WAR)
        return 0;
    border_city = -1;
    found = 0;
    for (unit = 0; unit < num_units && found == 0; unit++) {
        if (p_units[unit].owner == unit_owner && p_units[unit].owner != PLAYER_NONE && p_units[unit].owner != PLAYER_NEUTRAL
            && (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST) == 0
            && unit_types[p_units[unit].type].construction < 1) {
            for (city = 0; city < num_cities && found == 0; city++) {
                if (p_cities[city].owner == city_owner) {
                    if (p_units[unit].plane == p_cities[city].plane || p_units[unit].in_tower == 1) {
                        dist = Map_Distance(p_units[unit].x, p_units[unit].y, p_cities[city].x, p_cities[city].y, MAP_WIDTH);
                        if (dist < 3) {
                            border_city = city;
                            found = 1;
                        }
                    }
                }
            }
        }
    }
    if (border_city != -1) {
        if (wizards[unit_owner].diplomacy_status[city_owner] == TREATY_ALLIANCE || wizards[unit_owner].diplomacy_status[city_owner] == TREATY_WIZARD_PACT) {
            if (wizards[unit_owner].border_warning_stage[city_owner] == 0) {
                amount = Random(5);
                Diplo_Relation_Change(-amount, unit_owner, city_owner, DIPLO_MSG_TOO_CLOSE_TO_CITY, border_city, 0);
                wizards[unit_owner].border_warning_stage[city_owner] = 1;
            } else if (wizards[unit_owner].border_warning_stage[city_owner] == 1)
                wizards[unit_owner].border_warning_stage[city_owner] = 2;
            else {
                wizards[unit_owner].border_warning_stage[city_owner] = 0;
                Diplo_Relation_Change(-5, unit_owner, city_owner, DIPLO_MSG_REMEMBER_THIS_DAY, border_city, 0);
                Diplo_Break_Treaty(city_owner, unit_owner);
                Diplo_Break_Treaty(unit_owner, city_owner);
            }
        } else {
            wizards[unit_owner].border_warning_stage[city_owner] = 0;
            if (Random(100) < 6) {
                amount = Random(5);
                if (Random(5) == 1)
                    Diplo_Relation_Change(-amount, unit_owner, city_owner, DIPLO_MSG_TOO_CLOSE_TO_CITY, border_city, 0);
                else
                    Diplo_Relation_Change(-amount, unit_owner, city_owner, 0, border_city, 0);
            }
        }
    } else
        wizards[unit_owner].border_warning_stage[city_owner] = 0;
}

/* () every peace timer (wizard toward wizard) -1, floor 0 (Next_Turn)
   Numbered: BUG #239 (BUGS_AND_QUIRKS.txt) */
void Diplo_Tick_Peace_Turns(void)
{
    int i;
    int j;

    for (i = 0; i < num_players; i++) {
        for (j = 0; j < num_players; j++) {
            if (i != j) {
                wizards[i].peace_turns[j] = wizards[i].peace_turns[j] - 1;
                if (wizards[i].peace_turns[j] < 0)
                    wizards[i].peace_turns[j] = 0;
            }
        }
    }
}

/* () the treaty, trade and peace interests (wizard toward wizard) +10 while below 100 - they settle at 100..109
   (Next_Turn) */
void Diplo_Interest_Regrow(void)
{
    int i;
    int j;

    for (i = 0; i < num_players; i++) {
        for (j = 0; j < num_players; j++) {
            if (i != j) {
                if (wizards[i].treaty_interest[j] < 100)
                    wizards[i].treaty_interest[j] += 10;
                if (wizards[i].trade_interest[j] < 100)
                    wizards[i].trade_interest[j] += 10;
                if (wizards[i].peace_interest[j] < 100)
                    wizards[i].peace_interest[j] += 10;
            }
        }
    }
}
