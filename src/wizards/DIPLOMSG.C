/* DIPLOMSG.C - WIZARDS.EXE overlay code segment 7269: diplomacy messages - the audience an AI wizard requests (its
   proposal, demand or news), the offer dialog (Accept / Reject, Agree / Forget It, a spell list) with the AI's
   second try sweetened by tribute, the AI-versus-AI diplomacy of each turn, and the message text loader that
   expands DIPLOMSG.LBX's control bytes (names, cities, spells, units, treaties, the year, numbers).
   This module's data (DS:4f3a): its literals; BSS (DS:c400..c405).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7269 src/wizards/DIPLOMSG.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Screen_Diplomacy_Incoming(void);
int AI_Diplo_Turn_AI_Vs_AI(void);
int Screen_Diplomacy_Offer(void);
void Diplo_Offer_Redraw(void);
void Diplo_Load_Message(int msg_type, int wizard);

#include "GAME.H"

/* DIPLOMSG.LBX's text placeholders: the byte in a message and what the switch below writes there */
#define DTXT_HUMAN         0x80   /* the human wizard's name */
#define DTXT_PARTNER       0x81   /* the speaking wizard */
#define DTXT_ARG_WIZARD    0x82   /* the wizard in pending_msg_arg */
#define DTXT_CITY          0x83   /* the pending message's city */
#define DTXT_CITY_SIZE     0x84   /* its size name */
#define DTXT_WAR_TARGET    0x85   /* the war_request_target wizard */
#define DTXT_MSG_WIZARD    0x87   /* the msg_wizard_arg wizard */
#define DTXT_NUMBER        0x88   /* msg_number_arg */
#define DTXT_WANTED_SPELL  0x89   /* the wanted spell */
#define DTXT_GOLD          0x8a   /* msg_number_arg + " gold" */
#define DTXT_BROKEN_TREATY 0x8b   /* the broken treaty's name */
#define DTXT_ARG_SPELL     0x8c   /* the spell in pending_msg_arg */
#define DTXT_ARG_UNIT      0x8d   /* the unit type in pending_msg_arg */
#define DTXT_ARG_UNIT_A    0x8e   /* the same with "a" / "an" */
#define DTXT_TREATY        0x8f   /* the current treaty's name */
#define DTXT_YEAR          0x90   /* the game year */
#define DTXT_SPELL         0x91   /* diplo_msg_spell */
#define DTXT_WIZARD        0x92   /* diplo_msg_wizard */
#define DTXT_THIRD_WIZARD  0x93   /* diplo_third_wizard */
#define DTXT_AMOUNT        0x94   /* diplo_msg_number */

int diplo_msg_variant_count;                         /* DS:c404  DIPLOMSG.LBX 0 record word 2: the text variants */
int diplo_msg_anim;                             /* DS:c402  record word 1: the portrait animation the loaded message asks for */
int diplo_msg_mood;                             /* DS:c400  record word 0: the music mood it asks for (diplo_music_mood: the playing one) */

extern char *city_size_names[];                 /* DS:0156 */
extern int personality_treaty_mod[];            /* DS:1fc8 */
extern unsigned diplo_mirror_pic;               /* DS:9918 */
extern int present_mode;                        /* DS:9c8e */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern unsigned char spell_trade_list[];        /* DS:c3b8 */
extern int diplo_msg_wizard;                    /* DS:c3c4 */
extern int diplo_msg_number;                    /* DS:c3c6 */
extern int diplo_msg_spell;                     /* DS:c3c8 */
extern int diplo_music_mood;                    /* DS:c3d4 */
extern int diplo_anim_prev;                     /* DS:c3de */
extern int diplo_anim_state;                    /* DS:c3e0 */
extern char *p_diplo_msg_cache;                 /* DS:c3e2 */
extern char *p_diplo_text;                      /* DS:c3e4 */
extern int diplo_third_wizard;                  /* DS:c3e6 */
extern int diplo_msg_saved;                     /* DS:c3e8 */
extern int diplo_msg_variant;                   /* DS:c3ea */
extern int diplo_msg_type;                      /* DS:c3ec */
extern int diplo_talk_mode;                     /* DS:c3ee */
extern char *diplo_menu_items[7];               /* DS:c3f0 */
extern int diplo_partner;                       /* DS:c3fe */

/* Font_Select and Font_Select_Shadow have no prototype here: this module calls them with 3 / 2 arguments */
void Font_Select();
void Font_Select_Shadow();
void far_strcpy(char *dst, int dst_seg, char far *src);
void Diplo_Clamp_Treaty_Interest(void);
void Diplo_Pick_Greeting(void);
void Diplo_Screen_Load(void);
void Diplo_Screen_Redraw(void);
void Diplo_Main_Menu(void);
int Diplo_Demand_Drop_Ally(void);
void Diplo_Set_Status(int a, int b, int status);
void Diplo_Make_Peace(int a, int b);
void Diplo_Interests_Minus10(int a, int b);
int Diplo_Declare_War(int declarer, int target);
int Diplo_Break_Treaty(int breaker, int victim);
void Diplo_Give_Spell(int player, int spell);
void Diplo_Show_Reply(int msg, int reply);
int Diplo_Append_Tribute_Text(int spell, int gold);
void Diplo_Draw_Portrait_Anim(void);
void Diplo_Draw_Eyes(void);
int Diplo_Play_Mood_Music(int mood);
void Diplo_Break_Conflicting_Allies(void);
int Diplo_Tradeable_Spells(int receiver, int giver, int min_value, unsigned char *spell_list);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
int AI_War_Check(int target, int ai);
int AI_Border_Tension(int city_owner, int unit_owner);
void Wizard_Add_Gold(int player, int amount);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
int UI_Text_List_Select(int x, int y, int width, char *title, char *items, int stride, int *selection, int *enabled,
                        int fill_mode, int fill1, int fill2, int fill3, int fill4, int help);
int Sound_Stop(void);
void Combat_Tmp_Restore(void);
void Music_Play_Current(void);
void Set_Font_Color(int color);
void Set_Line_Spacing(int line_gap);
int Paragraph_Height(int width, char *text);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
char *Text_Article_A_An(char *word);

/* () an AI wizard's audience (wizards[0].pending_rel_msg[diplo_partner]): DIPLO_MSG_TRIBUTE_THANKS and
   DIPLO_MSG_ACCEPT_FOR_SPELL are skipped (-> 0); a proposal (DIPLO_MSG_OFFER_PACT .. DIPLO_MSG_PLEAD_FOR_PEACE,
   DIPLO_MSG_PAY_FOR_WAR, DIPLO_MSG_DEMAND_BREAK_TREATY) first greets (DIPLO_MSG_AUDIENCE_GREETING), demands the drop of an ally, then shows Screen_Diplomacy_Offer
   and applies the answer with the AI's gold / spell sweetener; none pending (DIPLO_MSG_NONE) greets (Diplo_Pick_Greeting) and a click opens
   the main menu; anything else is shown as a plain message. No return value otherwise. (DIPLOSCR.C's twin takes
   the proposals only up to DIPLO_MSG_OFFER_PEACE.)
   Numbered: BUG #55, #56 (BUGS_AND_QUIRKS.txt) */
int Screen_Diplomacy_Incoming(void)
{
    int click;
    int reply;
    int input;                                  /* also the answer of Screen_Diplomacy_Offer */
    int done;

    Diplo_Clamp_Treaty_Interest();
    diplo_music_mood = -1;
    diplo_anim_prev = -1;
    diplo_anim_state = DIPLO_ANIM_TALK_GOOD;
    Palette_Upload();
    done = 0;
    UI_Set_Redraw_Fn(Diplo_Screen_Redraw, 3);
    UI_Input_Delay(1);
    UI_Clear_Controls();
    diplo_talk_mode = DIPLO_TALK_MESSAGE;
    diplo_msg_type = wizards[PLAYER_HUMAN].pending_rel_msg[diplo_partner];
    present_mode = PRESENT_NORMAL;
    if (diplo_msg_type == DIPLO_MSG_ACCEPT_FOR_SPELL || diplo_msg_type == DIPLO_MSG_TRIBUTE_THANKS)
        return 0;
    if (diplo_msg_type == DIPLO_MSG_NONE) {
        diplo_talk_mode = DIPLO_TALK_GREETING;
        Diplo_Pick_Greeting();
    }
    Diplo_Screen_Load();
    diplo_msg_variant = -1;
    if ((diplo_msg_type >= DIPLO_MSG_OFFER_PACT && diplo_msg_type <= DIPLO_MSG_PLEAD_FOR_PEACE) || diplo_msg_type == DIPLO_MSG_PAY_FOR_WAR || diplo_msg_type == DIPLO_MSG_DEMAND_BREAK_TREATY) {
        diplo_msg_saved = diplo_msg_type;
        diplo_msg_type = DIPLO_MSG_AUDIENCE_GREETING;
        diplo_talk_mode = DIPLO_TALK_PROPOSAL;
        diplo_third_wizard = wizards[PLAYER_HUMAN].war_request_target[diplo_partner];
    }
    click = UI_NO_CONTROL;
    while (done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        switch (diplo_talk_mode) {
        case DIPLO_TALK_PROPOSAL:
            if (input == -1 || input == click) {
                reply = Diplo_Demand_Drop_Ally();
                if (reply != 0) {
                    diplo_msg_type = diplo_msg_saved;
                    diplo_msg_variant = -1;
                    input = Screen_Diplomacy_Offer();
                    if (input == 0) {
                        if (diplo_msg_type == DIPLO_MSG_OFFER_PACT)
                            Diplo_Set_Status(PLAYER_HUMAN, diplo_partner, TREATY_WIZARD_PACT);
                        if (diplo_msg_type == DIPLO_MSG_OFFER_ALLIANCE)
                            Diplo_Set_Status(PLAYER_HUMAN, diplo_partner, TREATY_ALLIANCE);
                        if (diplo_msg_type == DIPLO_MSG_OFFER_PEACE) {
                            Diplo_Make_Peace(PLAYER_HUMAN, diplo_partner);
                            if (wizards[PLAYER_HUMAN].relationship[diplo_partner] < 80)
                                wizards[PLAYER_HUMAN].relationship[diplo_partner] = wizards[PLAYER_HUMAN].relationship[diplo_partner] + 20;
                            wizards[diplo_partner].relationship[PLAYER_HUMAN] = wizards[PLAYER_HUMAN].relationship[diplo_partner];
                            Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
                            Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
                        }
                        if (diplo_msg_type == DIPLO_MSG_DEMAND_BREAK_TREATY)   /* agreeing declares war outright; the text
                                                   asks only for the treaty to end (DIPLOACT.C breaks it) */
                            Diplo_Declare_War(PLAYER_HUMAN, diplo_third_wizard);
                        if (diplo_msg_type == DIPLO_MSG_OFFER_PACT || diplo_msg_type == DIPLO_MSG_OFFER_ALLIANCE || diplo_msg_type == DIPLO_MSG_OFFER_PEACE) {
                            Wizard_Add_Gold(current_player, wizards[PLAYER_HUMAN].tribute_gold[diplo_partner]);
                            if (wizards[PLAYER_HUMAN].tribute_spell[diplo_partner] != 0)
                                Diplo_Give_Spell(PLAYER_HUMAN, wizards[PLAYER_HUMAN].tribute_spell[diplo_partner]);
                        }
                    } else if (diplo_msg_type == DIPLO_MSG_DEMAND_BREAK_TREATY) {
                        Diplo_Break_Treaty(diplo_partner, 0);
                    }
                    if (input > -1 && diplo_msg_type == DIPLO_MSG_OFFER_SPELL_TRADE)
                        Diplo_Give_Spell(PLAYER_HUMAN, wizards[PLAYER_HUMAN].spells_offered[diplo_partner][input]);
                }
                Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
                if (diplo_msg_type == DIPLO_MSG_OFFER_PACT || diplo_msg_type == DIPLO_MSG_OFFER_ALLIANCE)
                    wizards[PLAYER_HUMAN].treaty_interest[diplo_partner] -= Random(30) + 20;
                if (diplo_msg_type == DIPLO_MSG_OFFER_PEACE)
                    wizards[PLAYER_HUMAN].peace_interest[diplo_partner] -= Random(50) + 50;
                if (diplo_msg_type == DIPLO_MSG_OFFER_SPELL_TRADE)
                    wizards[PLAYER_HUMAN].trade_interest[diplo_partner] -= Random(30) + 20;
                done = 1;
            }
            break;
        case DIPLO_TALK_MESSAGE:
            Diplo_Show_Reply(diplo_msg_type, 3);
            done = 1;
            break;
        case DIPLO_TALK_GREETING:
            if (input == -1)
                done = 1;
            if (input == click) {
                Diplo_Main_Menu();
                done = 1;
            }
            break;
        }
        UI_Clear_Controls();
        click = UI_NO_CONTROL;
        switch (diplo_talk_mode) {
        case DIPLO_TALK_GREETING:
        case DIPLO_TALK_REFUSED:
        case DIPLO_TALK_PROPOSAL:
            click = UI_Add_Region(0, 0, 319, 199, "", -1);
            break;
        }
        if (done == 0) {
            Diplo_Screen_Redraw();
            Screen_Present();
            Wait_Ticks(3);
        }
    }
    Sound_Stop();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    current_screen = SCREEN_MAGIC;
    Combat_Tmp_Restore();
    Load_Overland_Screen_Graphics();
    Music_Play_Current();
}

/* () each turn, unless the human is casting Spell of Return (-> 0): every ordered pair of living AI wizards that have
   met, 1 in 15 - 2 x difficulty: score = relation bias + relationship + the other's personality modifier + Random(100)
   + (Random(5) + 3) x difficulty (+10 pact, +20 alliance); score + treaty interest > 150 makes the pair allied
   (already allied: a pact), else score > trade interest + 80 trades a spell each way; 1 in 20 relations +6 .. 10 (DIPLO_MSG_TRIBUTE_THANKS); at
   war, diplo_msg_variant (the diplomacy screen's last text variant, stale here) + peace interest > 100 makes peace; interests -30, conflicting alliances broken, the war
   check and border tension. Then every ally of a wizard at war with a third joins that war
   Numbered: BUG #9, #57, #162 (BUGS_AND_QUIRKS.txt) */
int AI_Diplo_Turn_AI_Vs_AI(void)
{
    int base;
    int k;
    int spell_a;
    int spell_b;
    int count_a;
    int count_b;
    int score;
    int i;
    int j;

    if (wizards[current_player].casting_spell == SPELL_SPELL_OF_RETURN)
        return 0;
    for (i = 1; i < num_players; i++) {
        for (j = 1; j < num_players; j++) {
            if (p_fortresses[i].active == 1 && p_fortresses[j].active == 1 && i != j
                    && Random(15 - difficulty * 2) == 1 && wizards[i].contacted[j] == 1) {
                base = wizards[i].relation_bias[j] + wizards[i].relationship[j]
                       + personality_treaty_mod[wizards[j].personality];
                score = base + Random(100);
                score = score + (Random(5) + 3) * difficulty;
                if (wizards[i].diplomacy_status[j] == TREATY_WIZARD_PACT)
                    score += 10;
                if (wizards[i].diplomacy_status[j] == TREATY_ALLIANCE)
                    score += 20;
                if (score + wizards[i].treaty_interest[j] > 150 && wizards[i].diplomacy_status[j] != TREATY_ALLIANCE) {
                    Diplo_Set_Status(i, j, TREATY_ALLIANCE);
                } else if (score + wizards[i].treaty_interest[j] > 150 && wizards[i].diplomacy_status[j] != TREATY_WIZARD_PACT) {
                    Diplo_Set_Status(i, j, TREATY_WIZARD_PACT);
                } else if (wizards[i].trade_interest[j] + 80 < score) {
                    count_a = Diplo_Tradeable_Spells(i, j, 1, spell_trade_list);
                    spell_a = spell_trade_list[0];
                    count_b = Diplo_Tradeable_Spells(j, i, 1, spell_trade_list);
                    spell_b = spell_trade_list[0];
                    if (count_a > 0 && count_b > 0) {   /* BUG #458: spell_a is one j knows and i lacks (receiver i,
                                                   giver j), yet j is given it - and i gets spell_b, which i
                                                   knows: the trade changes nothing */
                        Diplo_Give_Spell(j, spell_a);
                        Diplo_Give_Spell(i, spell_b);
                    }
                }
                if (Random(20) == 1)
                    Diplo_Relation_Change(Random(5) + 5, i, j, DIPLO_MSG_TRIBUTE_THANKS, 0, 0);
                if (wizards[i].diplomacy_status[j] == TREATY_WAR && diplo_msg_variant + wizards[i].peace_interest[j] > 100)
                    Diplo_Make_Peace(i, j);
                Diplo_Interests_Minus10(i, j);
                Diplo_Interests_Minus10(i, j);
                Diplo_Interests_Minus10(i, j);
                Diplo_Break_Conflicting_Allies();
                AI_War_Check(i, j);
                if (wizards[i].pending_rel_msg[j] == DIPLO_MSG_NONE)
                    AI_Border_Tension(j, i);
            }
        }
    }
    for (i = 1; i < num_players; i++) {
        for (j = 1; j < num_players; j++) {
            if (p_fortresses[i].active == 1 && p_fortresses[j].active == 1 && wizards[i].diplomacy_status[j] == TREATY_ALLIANCE) {
                for (k = 1; k < num_players; k++) {
                    if (p_fortresses[k].active == 1 && k != i && k != j && wizards[k].diplomacy_status[i] == TREATY_WAR
                            && wizards[k].diplomacy_status[j] != TREATY_WAR)
                        Diplo_Declare_War(j, k);
                }
            }
        }
    }
}

/* () -> 0 accept, 1 reject (also "Forget It" of Agree / Forget It), a spell index, or -1 ("Forget It" of
   the spell list only), for diplo_msg_type: Accept / Reject (pact, alliance,
   peace), Agree / Forget It (DIPLO_MSG_DEMAND_BREAK_TREATY), up to 4 offered spells + Forget It
   (DIPLO_MSG_OFFER_SPELL_TRADE, after a click); DIPLO_MSG_PAY_FOR_WAR is a click only (1). A refusal while the AI holds tribute (spell or gold) brings one more try with it offered */
int Screen_Diplomacy_Offer(void)
{
    int choice;
    int cancel;
    int control;                                /* set, never used */
    int forget;
    int phrase;
    char spell_name[80];
    int i;
    int height;

    Diplo_Load_Message(diplo_msg_type, diplo_partner);
    forget = UI_NO_CONTROL;
    for (i = 0; i < 7; i++)
        strcpy(diplo_menu_items[i], "[ ");
    switch (diplo_msg_type) {
    case DIPLO_MSG_OFFER_PACT:
    case DIPLO_MSG_OFFER_ALLIANCE:
    case DIPLO_MSG_OFFER_PEACE:
        strcat(diplo_menu_items[0], "Accept");
        strcat(diplo_menu_items[1], "Reject");
        diplo_menu_items[2][0] = 0;
        cancel = 1;
        break;
    case DIPLO_MSG_DEMAND_BREAK_TREATY:
        strcat(diplo_menu_items[0], "Agree");
        strcat(diplo_menu_items[1], "Forget It");
        diplo_menu_items[2][0] = 0;
        cancel = 1;
        break;
    case DIPLO_MSG_OFFER_SPELL_TRADE:
        i = 0;
        for (i = 0; i < wizards[PLAYER_HUMAN].spells_offered_count[diplo_partner] && i < 4; i++) {
            far_strcpy(spell_name, 0, p_spell_data[wizards[PLAYER_HUMAN].spells_offered[diplo_partner][i]].name);
            strcat(diplo_menu_items[i], spell_name);
        }
        forget = i;
        strcat(diplo_menu_items[forget], "Forget It");
        diplo_menu_items[forget + 1][0] = 0;
        cancel = forget;
        Diplo_Load_Message(diplo_msg_type, diplo_partner);
        break;
    }
    UI_Set_Redraw_Fn(Diplo_Offer_Redraw, 3);
    UI_Input_Delay(1);
    choice = cancel;
    Font_Select_Shadow(4, 4);
    Set_Text_Shadow_Color(0);
    Set_Font_Color(0xbb);
    Set_Line_Spacing(1);
    height = Paragraph_Height(245, p_diplo_text);
    if (height > 39) {
        Set_Line_Spacing(0);
        height = Paragraph_Height(245, p_diplo_text);
    }
    if (diplo_msg_type == DIPLO_MSG_PAY_FOR_WAR || diplo_msg_type == DIPLO_MSG_OFFER_SPELL_TRADE) {
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "", -1);
        while (UI_Get_Input() == 0) {
            Timer_Mark();
            Diplo_Offer_Redraw();
            Screen_Present();
            Wait_Ticks(3);
        }
        choice = 1;
        if (diplo_msg_type == DIPLO_MSG_OFFER_SPELL_TRADE) {
            strcpy(p_diplo_text, "In exchange you will receive:");
            Font_Select(1, 4, 3);
            Set_Font_Color(0xbb);
            choice = UI_Text_List_Select(38, 147, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0, 0, -1);
        }
    } else {
        Font_Select(1, 4, 3);
        Set_Font_Color(0xbb);
        choice = UI_Text_List_Select(38, height + 133, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0, 0,
                                     -1);
        if ((choice == 1 || choice == -1)
                && (wizards[PLAYER_HUMAN].tribute_spell[diplo_partner] != 0 || wizards[PLAYER_HUMAN].tribute_gold[diplo_partner] != 0)) {
            phrase = Random(2);
            strcpy(p_diplo_text, phrase == 1 ? "What if we were to also offer "
                                             : "Perhaps you would reconsider if we also provided ");
            Diplo_Append_Tribute_Text(wizards[PLAYER_HUMAN].tribute_spell[diplo_partner], wizards[PLAYER_HUMAN].tribute_gold[diplo_partner]);
            if (phrase == 1)
                strcat(p_diplo_text, " as an incentive");
            strcat(p_diplo_text, "?");
            Font_Select_Shadow(4, 4);
            Set_Text_Shadow_Color(0);
            Set_Font_Color(0xbb);
            Set_Line_Spacing(1);
            height = Paragraph_Height(245, p_diplo_text);
            if (height > 39) {
                Set_Line_Spacing(0);
                height = Paragraph_Height(245, p_diplo_text);
            }
            Font_Select(1, 4, 3);
            Set_Font_Color(0xbb);
            if (diplo_msg_type == DIPLO_MSG_DEMAND_BREAK_TREATY)
                choice = 0;
            else
                choice = 1;
            choice = UI_Text_List_Select(38, height + 133, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0,
                                         0, -1);
        }
    }
    if (choice == -1)
        choice = cancel;
    if (choice == forget)
        choice = -1;
    UI_Set_Redraw_Fn(Diplo_Screen_Redraw, 3);
    UI_Input_Delay(1);
    return choice;
}

/* () redraw callback of the offer dialog: portrait, mirror, the eyes (Diplo_Draw_Eyes) and the message */
void Diplo_Offer_Redraw(void)
{
    int height;

    Set_Draw_Page();
    Screen_Restore_A800();
    Diplo_Draw_Portrait_Anim();
    Draw_Pic_Frame(95, 1, diplo_mirror_pic);
    Diplo_Draw_Eyes();
    Font_Select_Shadow(4, 4);
    Set_Text_Shadow_Color(0);
    Set_Font_Color(0xbb);
    Set_Line_Spacing(1);
    height = Paragraph_Height(245, p_diplo_text);
    if (height > 39)
        Set_Line_Spacing(0);
    Paragraph_Print(38, 140, 245, p_diplo_text, PARA_LEFT);
}

/* (msg_type, wizard) the message text into p_diplo_text: a variant of DIPLOMSG.LBX entry 1 record type * 15 (variant =
   Random(count) - 1, -2 in 2-player games, at least 0; kept while diplo_msg_variant != -1, when the cached text is
   reused) with the control bytes 0x80..0x94 expanded; also the record's mood music and animation
   Numbered: BUG #60 (BUGS_AND_QUIRKS.txt) */
void Diplo_Load_Message(int msg_type, int wizard)
{
    int k;
    int unused;                                 /* set, never used */
    int msg;
    char text[30];
    int len;

    len = 0;
    unused = 0;
    msg = msg_type;
    if (diplo_msg_variant == -1) {
        LBX_Load_Records_Near("DIPLOMSG", 0, &diplo_msg_mood, msg_type, 1, 6);   /* 3 words: diplo_msg_mood, _anim and
                                                   _variants, which follow it in DS */
        if (num_players == 2)
            diplo_msg_variant = Random(diplo_msg_variant_count) - 2;
        else
            diplo_msg_variant = Random(diplo_msg_variant_count) - 1;
        if (diplo_msg_variant < 0)
            diplo_msg_variant = 0;
        LBX_Load_Records_Near("DIPLOMSG", 1, p_diplo_msg_cache, diplo_msg_variant + msg * NUM_DIPLO_MSG_VARIANTS, 1, 200);
        for (k = 0; k < 200 && p_diplo_msg_cache[k] != 0; k++) {
            p_diplo_text[len] = p_diplo_msg_cache[k];
            switch ((unsigned char)p_diplo_text[len]) {
            case DTXT_HUMAN:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[PLAYER_HUMAN].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_PARTNER:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[wizard].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_ARG_WIZARD:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[wizards[PLAYER_HUMAN].pending_msg_arg[wizard]].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_CITY:
                far_strcpy(p_diplo_text + len, 0, p_cities[wizards[PLAYER_HUMAN].pending_msg_city[wizard]].name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_CITY_SIZE:
                strcpy(p_diplo_text + len, city_size_names[p_cities[wizards[PLAYER_HUMAN].pending_msg_city[wizard]].size]);
                len = strlen(p_diplo_text);
                break;
            case DTXT_WAR_TARGET:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[wizards[PLAYER_HUMAN].war_request_target[wizard]].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_MSG_WIZARD:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[wizards[PLAYER_HUMAN].msg_wizard_arg[wizard]].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_NUMBER:
                p_diplo_text[len] = 0;
                itoa(wizards[PLAYER_HUMAN].msg_number_arg[wizard], text, 10);
                strcat(p_diplo_text, text);
                len = strlen(p_diplo_text);
                break;
            case DTXT_WANTED_SPELL:
                p_diplo_text[len] = 0;
                far_strcpy(text, 0, p_spell_data[wizards[PLAYER_HUMAN].wanted_spell[wizard]].name);
                strcat(p_diplo_text, text);
                len = strlen(p_diplo_text);
                break;
            case DTXT_GOLD:
                p_diplo_text[len] = 0;
                itoa(wizards[PLAYER_HUMAN].msg_number_arg[wizard], text, 10);
                strcat(p_diplo_text, text);
                strcat(p_diplo_text, " gold");
                len = strlen(p_diplo_text);
                break;
            case DTXT_BROKEN_TREATY:
                p_diplo_text[len] = 0;
                if (wizards[PLAYER_HUMAN].broken_treaty_type[wizard] == TREATY_ALLIANCE)
                    strcat(p_diplo_text, "Alliance");
                else if (wizards[PLAYER_HUMAN].broken_treaty_type[wizard] == TREATY_WIZARD_PACT)
                    strcat(p_diplo_text, "Wizard Pact");
                else
                    strcat(p_diplo_text, "treaty");
                len = strlen(p_diplo_text);
                break;
            case DTXT_ARG_SPELL:
                p_diplo_text[len] = 0;
                far_strcpy(text, 0, p_spell_data[wizards[PLAYER_HUMAN].pending_msg_arg[wizard]].name);
                strcat(p_diplo_text, text);
                len = strlen(p_diplo_text);
                break;
            case DTXT_ARG_UNIT:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, unit_types[wizards[PLAYER_HUMAN].pending_msg_arg[wizard]].name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_ARG_UNIT_A:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, Text_Article_A_An(unit_types[wizards[PLAYER_HUMAN].pending_msg_arg[wizard]].name));
                len = strlen(p_diplo_text);
                break;
            case DTXT_TREATY:
                p_diplo_text[len] = 0;
                if (wizards[PLAYER_HUMAN].diplomacy_status[wizard] == TREATY_ALLIANCE)
                    strcat(p_diplo_text, "Alliance");
                else if (wizards[PLAYER_HUMAN].diplomacy_status[wizard] == TREATY_WIZARD_PACT)
                    strcat(p_diplo_text, "Wizard Pact");
                else
                    strcat(p_diplo_text, "treaty");
                len = strlen(p_diplo_text);
                break;
            case DTXT_YEAR:
                p_diplo_text[len] = 0;
                itoa(game_turn / TURNS_PER_YEAR + START_YEAR, text, 10);
                strcat(p_diplo_text, text);
                len = strlen(p_diplo_text);
                break;
            case DTXT_SPELL:
                p_diplo_text[len] = 0;
                far_strcpy(text, 0, p_spell_data[diplo_msg_spell].name);
                strcat(p_diplo_text, text);
                len = strlen(p_diplo_text);
                break;
            case DTXT_WIZARD:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[diplo_msg_wizard].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_THIRD_WIZARD:
                p_diplo_text[len] = 0;
                strcat(p_diplo_text, wizards[diplo_third_wizard].wizard_name);
                len = strlen(p_diplo_text);
                break;
            case DTXT_AMOUNT:
                p_diplo_text[len] = 0;
                itoa(diplo_msg_number, text, 10);
                strcat(p_diplo_text, text);
                len = strlen(p_diplo_text);
                break;
            default:
                len++;
                break;
            }
        }
        p_diplo_text[len] = 0;
        strcpy(p_diplo_msg_cache, p_diplo_text);
        Diplo_Play_Mood_Music(diplo_msg_mood);
        diplo_anim_state = diplo_msg_anim;
    } else {
        strcpy(p_diplo_text, p_diplo_msg_cache);
    }
}
