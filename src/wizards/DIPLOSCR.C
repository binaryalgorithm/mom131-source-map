/* DIPLOSCR.C - WIZARDS.EXE overlay code segment 6ea7 (stub 24a6): the diplomacy screen the human opens - the audience
   loop, the main menu (Propose Treaty / Threaten-Break Treaty / Offer Tribute / Exchange Spells / Good Bye), the
   treaty proposals with the AI's sweetener request, picking a third wizard, the threats, the reply box, the
   partner's portrait animation, eyes and mood music, loading the screen and its pictures, and the armies near a
   wizard's fortress.
   This module's data (DS:4c44): the test flag, the war-target count, the menu strings and titles, then its literals;
   BSS (DS:c308..c400): the diplomacy state shared with the other diplomacy modules.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6ea7 src/wizards/DIPLOSCR.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order */
int Screen_Diplomacy(void);
void Diplo_Screen_Redraw(void);
int Diplo_Append_Tribute_Text(int spell, int gold);
void Diplo_Menu_Redraw(void);
void Diplo_Main_Menu(void);
void Diplo_Propose_Treaty(void);
int Diplo_Pick_Third_Wizard(int mode);
void Diplo_Show_Reply(int msg, int reply);
int Diplo_AI_Ask_Sweetener(int tier);
void Diplo_Reply_Redraw(void);
void Diplo_Threaten_Menu(void);
int Diplo_Play_Mood_Music(int mood);
void Diplo_Draw_Portrait_Anim(void);
void Diplo_Screen_Load(void);
void Diplo_Load_Assets(void);
void Diplo_Draw_Eyes(void);
void Diplo_Portrait_Fade_In(void);
void Diplo_Uncalled_Nop(void);
int Diplo_Army_Near_Fortress(int a, int b, long *army_a, long *army_b);

#include "GAME.H"

#define TREATY_MENU_PACT 0              /* Diplo_Propose_Treaty's rows (diplo_treaty_menu_strings) */
#define TREATY_MENU_ALLIANCE 1
#define TREATY_MENU_PEACE 2
#define TREATY_MENU_WAR_ON 3
#define TREATY_MENU_BREAK_ALLIANCE 4
#define TREATY_MENU_FORGET_IT 5
#define TREATY_MENU_ROWS 6
#define SWEETENER_DECLINED 1            /* Diplo_AI_Ask_Sweetener: the human said no (DIPLO_REPLY_WANTS_SPELL's value) */

int diplo_partner;                              /* DS:c3fe  the wizard the human talks to */
char *diplo_menu_items[7];                      /* DS:c3f0  100-byte option strings of the list menus */
int diplo_talk_mode;                            /* DS:c3ee  0 greeting, 1 refused audience, 2 AI proposal, 6 a message */
int diplo_msg_type;                             /* DS:c3ec  DIPLOMSG.LBX message type */
int diplo_msg_variant;                          /* DS:c3ea  text variant (-1: pick a new one) */
int diplo_msg_saved;                            /* DS:c3e8  the proposal while the greeting is shown */
int diplo_third_wizard;                         /* DS:c3e6  the third wizard of a message */
char *p_diplo_text;                             /* DS:c3e4  0xdc bytes: the expanded message text */
char *p_diplo_msg_cache;                        /* DS:c3e2  0xdc bytes: the DIPLOMSG record, then */
                                                /* the expanded text (Diplo_Load_Message copies it back on a repeat) */
int diplo_anim_state;                           /* DS:c3e0  the portrait state shown (set from diplo_msg_anim, the message's) */
int diplo_anim_prev;                            /* DS:c3de  the state at the last redraw (-1 at start) */
unsigned diplo_background_pic;                  /* DS:c3dc  DIPLOMAC 0 */
int diplo_unused_c3da;                          /* DS:c3da  never referenced */
unsigned diplo_music_good;                      /* DS:c3d8  MUSIC.LBX 41 + 2 x portrait */
unsigned diplo_music_bad;                       /* DS:c3d6  MUSIC.LBX 42 + 2 x portrait */
int diplo_music_mood;                           /* DS:c3d4  the mood music playing (-1 none) */
unsigned diplo_talk_pic;                        /* DS:c3d2  DIPLOMAC 24 + portrait: the talk animation */
int diplo_unused_c3ce[2];                       /* DS:c3ce  never referenced */
unsigned *p_diplo_eyes_left;                    /* DS:c3cc  DIPLOMAC 2..12 by relation */
unsigned *p_diplo_eyes_right;                   /* DS:c3ca  DIPLOMAC 13..23 */
int diplo_msg_spell;                            /* DS:c3c8  spell named by control code 0x91 */
int diplo_msg_number;                           /* DS:c3c6  number printed by control code 0x94 */
int diplo_msg_wizard;                           /* DS:c3c4  wizard named by control code 0x92 */
unsigned char spell_trade_count;                /* DS:c3c2  entries in spell_trade_list */
unsigned char spell_trade_list[10];             /* DS:c3b8  spells from Diplo_List_Spells_To_Trade */
/* BUG #54: a byte keeps only the low 8 bits of a research cost (in 1.31, 159 of 214 spells cost more than 255), so
   the AI's trade comparisons use value & 0xff */
unsigned char spell_trade_values[10];           /* DS:c3ae  their values (Spell_Trade_Value) */
unsigned char xchg_ai_count;                    /* DS:c3ad  Exchange Spells: partner spells copied */
unsigned char xchg_ai_spells[10];               /* DS:c3a3  AIDIPLO: [0] the spell the AI asks for; Exchange Spells: the partner's spells */
unsigned char xchg_ai_values[10];               /* DS:c399  AIDIPLO: [0] the proposal's value limit; Exchange Spells: their values */
unsigned char xchg_offer_count;                 /* DS:c398  partner spells with a counter offer */
unsigned char xchg_offer_spell[10];             /* DS:c38e  partner spell per offer row */
unsigned char xchg_counter_count[10];           /* DS:c384  the number of human counter spells per row */
unsigned char xchg_counter_spells[10][10];      /* DS:c320  human counter spells per row */
char *diplo_list_unused;                        /* DS:c31e  20 bytes allocated, never used */
char *diplo_str_ptrs_unused[6];                 /* DS:c312  0x32-byte strings allocated, never read */
int diplo_unused_c310;                          /* DS:c310  never referenced */
int diplo_last_response_score;                  /* DS:c30e  written by Diplo_Proposal_Response, never read */
int *p_diplo_war_targets;                       /* DS:c30c  wizards the partner could declare war on */
int *p_diplo_partner_allies;                    /* DS:c30a  the partner's allies */
int diplo_partner_ally_count;                   /* DS:c308 */

int diplo_test_mode = 0;                        /* DS:4c44  never set: the test branches of the AI diplomacy are dead */
int diplo_war_target_count = 0;                 /* DS:4c46  entries in p_diplo_war_targets */
char *diplo_main_menu_strings[7] = {            /* DS:4c48 */
    "[ Propose Treaty", "[ Threaten/Break Treaty", "[ Offer Tribute", "[ Exchange Spells", "[ Good Bye", "", ""
};
char *diplo_treaty_menu_strings[7] = {          /* DS:4c56 */
    "[ Wizard Pact", "[ Alliance", "[ Peace Treaty", "[ Declaration of War on Another Wizard",
    "[ Break Alliance With Another Wizard", "[ Forget It", ""
};
char *diplo_threaten_menu_strings[6] = {        /* DS:4c64 */
    "[ Break Wizard Pact", "[ Break Alliance", "[ Threaten To Attack", "[ Forget It", ""
};
char diplo_agree_items[4][12] = { "[ Agree    ", "[ Forget It" };  /* DS:4c70  the sweetener's answers */
char *diplo_menu_titles[3] = { "How may I serve you:", "You propose a treaty: ", "Your actions: " };  /* DS:4ca0 */

extern int personality_treaty_mod[];            /* DS:1fc8 */
extern unsigned diplo_face_pic;                 /* DS:979a  MOODWIZ entry = portrait: frame 0 good, 1 bad, 2 neutral */
extern unsigned diplo_mirror_pic;               /* DS:9918 */
extern int present_mode;                        /* DS:9c8e */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern unsigned p_world_block;                /* DS:9d20 */
extern int game_turn;                           /* DS:bd90 */

/* Font_Select, Font_Select_Shadow and Paragraph_Height have no prototype here: this module calls them with 2 and 3
   arguments */
void Font_Select();
void Font_Select_Shadow();
int Paragraph_Height();
void far_strcpy(char *dst, int dst_seg, char far *src);
void Diplo_Clamp_Treaty_Interest(void);
void Diplo_Pick_Greeting(void);
void Diplo_Load_Message(int msg_type, int wizard);
int Diplo_List_Spells_To_Trade(int giver, int receiver, int min_value);
int Diplo_Min_Interest(void);
void Diplo_Offer_Tribute(void);
void Diplo_Exchange_Spells(void);
int Diplo_Proposal_Response(int penalty, int interest_kind);
void Diplo_Set_Status(int a, int b, int status);
void Diplo_Make_Peace(int a, int b);
void Diplo_Interests_Minus10(int a, int b);
int Diplo_Declare_War(int declarer, int target);
int Diplo_Break_Treaty(int breaker, int victim);
void Diplo_Give_Spell(int player, int spell);
void Wizard_Add_Gold(int player, int amount);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
int UI_Text_List_Select(int x, int y, int width, char *title, char *items, int stride, int *selection, int *enabled,
                        int fill_mode, int fill1, int fill2, int fill3, int fill4, int help);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Combat_Tmp_Save(void);
void Combat_Tmp_Restore(void);
void Research_Recheck_Known(int fade);
void Music_Play_Current(void);
void Load_Diplomacy_Art(void);
void Set_Font_Color(int color);
void Set_Line_Spacing(int line_gap);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void Set_Pic_Frame(unsigned pic, unsigned frame);
int Pic_Get_Frame(unsigned pic);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Screen_Copy_Front(void);

/* () the diplomacy screen the human opens (Main_Loop screen 15): DIPLO_MSG_TRIBUTE_THANKS / _ACCEPT_FOR_SPELL
   pending -> 0 at once; no pending
   message greets (mode 0: a click opens the main menu, Esc leaves; mode 1 refused: any click leaves); a proposal
   (DIPLO_MSG_OFFER_PACT .. _OFFER_PEACE, _PAY_FOR_WAR, _DEMAND_BREAK_TREATY) sets mode 2, which this loop never handles; any other message is shown and the screen left.
   Exit: the Magic screen, COMBAT.TMP restored, the research list rechecked, the overland graphics and music back
   Numbered: BUG #65 (BUGS_AND_QUIRKS.txt) */
int Screen_Diplomacy(void)
{
    int input;
    int click;
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
    if ((diplo_msg_type >= DIPLO_MSG_OFFER_PACT && diplo_msg_type <= DIPLO_MSG_OFFER_PEACE) || diplo_msg_type == DIPLO_MSG_PAY_FOR_WAR || diplo_msg_type == DIPLO_MSG_DEMAND_BREAK_TREATY) {
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
        case DIPLO_TALK_REFUSED:
            if (input == -1 || input == click)
                done = 1;
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
    Palette_Fade_To_Black();
    present_mode = PRESENT_FADE_IN;
    Combat_Tmp_Restore();
    Research_Recheck_Known(1);
    Load_Overland_Screen_Graphics();
    Music_Play_Current();
}

/* () redraw of the audience: the portrait animation (not when the wizard refuses to talk), the mirror, the eyes,
   and in modes 0 / 1 / 2 the message text */
void Diplo_Screen_Redraw(void)
{
    Set_Draw_Page();
    Screen_Restore_A800();
    if (diplo_talk_mode != DIPLO_TALK_REFUSED)
        Diplo_Draw_Portrait_Anim();
    Draw_Pic_Frame(95, 1, diplo_mirror_pic);
    Diplo_Draw_Eyes();
    switch (diplo_talk_mode) {
    case DIPLO_TALK_GREETING:
    case DIPLO_TALK_REFUSED:
    case DIPLO_TALK_PROPOSAL:
        Font_Select_Shadow(4, 4);
        Set_Text_Shadow_Color(0);
        Set_Font_Color(0xbb);
        Set_Line_Spacing(1);
        Diplo_Load_Message(diplo_msg_type, diplo_partner);
        Paragraph_Print(38, 140, 245, p_diplo_text, PARA_LEFT);
        break;
    }
}

/* (spell, gold) appends "<spell> spell", or "<gold> gold" when there is no spell, to the message; nothing at all when
   gold is 0, even with a spell (BUG #64)
   Numbered: BUG #64 (BUGS_AND_QUIRKS.txt) */
int Diplo_Append_Tribute_Text(int spell, int gold)
{
    char text[30];

    if (gold == 0)
        return 0;
    if (spell != SPELL_NONE) {
        far_strcpy(text, 0, p_spell_data[spell].name);
        strcat(p_diplo_text, text);
        strcat(p_diplo_text, " spell");
    } else {
        itoa(gold, text, 10);
        strcat(p_diplo_text, text);
        strcat(p_diplo_text, " gold");
    }
}

/* () redraw of the menus: portrait, mirror, eyes and the prompt (line spacing 0 when it is taller than 39 pixels) */
void Diplo_Menu_Redraw(void)
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
    if (height > 39) {
        Set_Line_Spacing(0);
        height = Paragraph_Height(245, p_diplo_text);
    }
    Paragraph_Print(38, 138, 245, p_diplo_text, PARA_LEFT);
}

/* () "How may I serve you:" until Good Bye / Esc: at war Threaten and Exchange Spells are off, Offer Tribute too
   without 25 gold or a spell the partner lacks; a partner with ANY of its treaty / trade / peace interests below -100 is tired of talks
   Numbered: BUG #322 (BUGS_AND_QUIRKS.txt) */
void Diplo_Main_Menu(void)
{
    int choice;
    int enabled[10];
    int i;
    int done;

    done = 0;
    while (done == 0) {
        choice = 0;
        for (i = 0; i < 6; i++)
            enabled[i] = 1;
        if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] >= TREATY_WAR) {
            enabled[DIPLO_MENU_THREATEN] = 0;
            enabled[DIPLO_MENU_EXCHANGE] = 0;
        }
        Diplo_List_Spells_To_Trade(current_player, diplo_partner, 0);
        if (wizards[PLAYER_HUMAN].gold < 25 && spell_trade_count == 0)
            enabled[DIPLO_MENU_TRIBUTE] = 0;
        if (Diplo_Min_Interest() < -100) {
            Diplo_Show_Reply(DIPLO_MSG_END_TALKS, 3);
            done = 1;
        } else {
            UI_Set_Redraw_Fn(Diplo_Menu_Redraw, 3);
            UI_Input_Delay(1);
            strcpy(p_diplo_text, diplo_menu_titles[0]);
            for (i = 0; i < 7; i++)
                strcpy(diplo_menu_items[i], diplo_main_menu_strings[i]);
            Font_Select(1, 4, 3);
            Set_Font_Color(0xbb);
            choice = UI_Text_List_Select(38, 142, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, enabled, 15, 11, 0, 0,
                                         0, -1);
            switch (choice) {
            case -1:
            case DIPLO_MENU_GOODBYE:
                done = 1;
                break;
            case DIPLO_MENU_PROPOSE:
                Diplo_Propose_Treaty();
                break;
            case DIPLO_MENU_THREATEN:
                Diplo_Threaten_Menu();
                break;
            case DIPLO_MENU_TRIBUTE:
                Diplo_Offer_Tribute();
                if (diplo_msg_type == DIPLO_MSG_END_TALKS)
                    done = 1;
                break;
            case DIPLO_MENU_EXCHANGE:
                Diplo_Exchange_Spells();
                break;
            }
        }
    }
    UI_Set_Redraw_Fn(Diplo_Screen_Redraw, 3);
    UI_Input_Delay(1);
}

/* () "You propose a treaty:" Wizard Pact (no treaty yet; refused at relation 10 or less), Alliance (below an
   alliance; relation over 50), Peace Treaty (at war), Declaration of War on Another Wizard (a partner allied with the
   human agrees 1 in 2 when the human is at war with the target), Break Alliance With Another Wizard (refused at a relation
   of 24 or less) or Forget It; an undecided answer brings the AI's sweetener request; the reply is shown unless
   cancelled or the sweetener declined
   Numbered: BUG #61, #155 (BUGS_AND_QUIRKS.txt) */
void Diplo_Propose_Treaty(void)
{
    int choice;
    int msg;
    int threshold;                              /* set, never used */
    int enabled[10];
    int i;
    int reply;

    choice = 0;
    for (i = 0; i < TREATY_MENU_ROWS; i++)
        enabled[i] = 1;
    strcpy(p_diplo_text, diplo_menu_titles[1]);
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] != TREATY_NONE)
        enabled[TREATY_MENU_PACT] = 0;
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] >= TREATY_ALLIANCE)
        enabled[TREATY_MENU_ALLIANCE] = 0;
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] < TREATY_WAR)
        enabled[TREATY_MENU_PEACE] = 0;
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] >= TREATY_WAR)
        enabled[TREATY_MENU_WAR_ON] = 0;
    diplo_war_target_count = 0;
    for (i = 1; i < num_players; i++) {
        if (wizards[diplo_partner].contacted[i] == 1 && wizards[PLAYER_HUMAN].contacted[i] == 1
            && wizards[diplo_partner].diplomacy_status[i] < TREATY_WAR && i != diplo_partner) {
            p_diplo_war_targets[diplo_war_target_count] = i;
            diplo_war_target_count++;
        }
    }
    if (diplo_war_target_count == 0)
        enabled[TREATY_MENU_WAR_ON] = 0;
    diplo_partner_ally_count = 0;
    for (i = 1; i < num_players; i++) {
        if (wizards[diplo_partner].contacted[i] == 1 && wizards[diplo_partner].diplomacy_status[i] == TREATY_ALLIANCE
            && i != diplo_partner) {
            p_diplo_partner_allies[diplo_partner_ally_count] = i;
            diplo_partner_ally_count++;
        }
    }
    if (diplo_partner_ally_count == 0)
        enabled[TREATY_MENU_BREAK_ALLIANCE] = 0;
    for (i = 0; i < TREATY_MENU_ROWS; i++)
        strcpy(diplo_menu_items[i], diplo_treaty_menu_strings[i]);
    Font_Select(1, 4, 3);
    Set_Font_Color(0xbb);
    Set_Line_Spacing(2);
    choice = UI_Text_List_Select(38, 142, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, enabled, 15, 11, 0, 0, 0, -1);
    switch (choice) {
    case TREATY_MENU_PACT:
        if (wizards[PLAYER_HUMAN].relationship[diplo_partner] > 10)
            reply = Diplo_Proposal_Response(75, INTEREST_TREATY);
        else
            reply = DIPLO_REPLY_REFUSE;
        if (reply == DIPLO_REPLY_WANTS_SPELL || reply == DIPLO_REPLY_WANTS_GOLD)
            reply = Diplo_AI_Ask_Sweetener(reply);
        if (reply == DIPLO_REPLY_ACCEPT)
            Diplo_Set_Status(PLAYER_HUMAN, diplo_partner, TREATY_WIZARD_PACT);
        msg = DIPLO_MSG_PACT_SIGNED;
        break;
    case TREATY_MENU_ALLIANCE:
        if (wizards[PLAYER_HUMAN].relationship[diplo_partner] > 50)
            reply = Diplo_Proposal_Response(125, INTEREST_TREATY);
        else
            reply = DIPLO_REPLY_REFUSE;
        if (reply == DIPLO_REPLY_WANTS_SPELL || reply == DIPLO_REPLY_WANTS_GOLD)
            reply = Diplo_AI_Ask_Sweetener(reply);
        if (reply == DIPLO_REPLY_ACCEPT)
            Diplo_Set_Status(PLAYER_HUMAN, diplo_partner, TREATY_ALLIANCE);
        msg = DIPLO_MSG_ALLIANCE_SIGNED;
        break;
    case TREATY_MENU_PEACE:
        reply = Diplo_Proposal_Response(60, INTEREST_TREATY);   /* peace on the treaty interest, not INTEREST_PEACE */
        if (reply == DIPLO_REPLY_WANTS_SPELL || reply == DIPLO_REPLY_WANTS_GOLD)
            reply = Diplo_AI_Ask_Sweetener(reply);
        if (reply == DIPLO_REPLY_ACCEPT)
            Diplo_Make_Peace(PLAYER_HUMAN, diplo_partner);
        Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
        Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
        msg = DIPLO_MSG_PEACE_SIGNED;
        break;
    case TREATY_MENU_WAR_ON:
        diplo_msg_wizard = Diplo_Pick_Third_Wizard(0);
        reply = DIPLO_REPLY_WANTS_GOLD;
        msg = DIPLO_MSG_AGREE_JOIN_WAR;
        if (diplo_msg_wizard != 0) {
            if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] == TREATY_ALLIANCE && wizards[PLAYER_HUMAN].diplomacy_status[diplo_msg_wizard] == TREATY_WAR) {
                if (Random(8) < 5)
                    reply = DIPLO_REPLY_ACCEPT;
                else
                    reply = DIPLO_REPLY_WANTS_GOLD;
            } else {
                threshold = wizards[diplo_partner].diplomacy_status[diplo_msg_wizard] * 30 + 150;
                reply = Diplo_Proposal_Response(wizards[diplo_partner].relationship[diplo_msg_wizard] + 150, INTEREST_TREATY);
            }
            if (reply == DIPLO_REPLY_WANTS_SPELL || reply == DIPLO_REPLY_WANTS_GOLD)
                reply = Diplo_AI_Ask_Sweetener(reply);
            if (reply == DIPLO_REPLY_ACCEPT)
                Diplo_Declare_War(diplo_partner, diplo_msg_wizard);
            msg = DIPLO_MSG_AGREE_JOIN_WAR;
        } else
            choice = -1;
        break;
    case TREATY_MENU_BREAK_ALLIANCE:
        diplo_third_wizard = Diplo_Pick_Third_Wizard(1);
        if (diplo_third_wizard != 0) {
            if (wizards[diplo_partner].relationship[PLAYER_HUMAN] > 24) {
                reply = Diplo_Proposal_Response(wizards[diplo_partner].relationship[diplo_third_wizard] + 175, INTEREST_TREATY);
                if (reply == DIPLO_REPLY_WANTS_SPELL || reply == DIPLO_REPLY_WANTS_GOLD)
                    reply = Diplo_AI_Ask_Sweetener(reply);
                if (reply == DIPLO_REPLY_ACCEPT)
                    Diplo_Break_Treaty(diplo_partner, diplo_third_wizard);
            } else
                reply = DIPLO_REPLY_WANTS_GOLD;
            msg = DIPLO_MSG_AGREE_BREAK_ALLIANCE;
        } else
            choice = -1;
        break;
    case TREATY_MENU_FORGET_IT:
        choice = -1;
        break;
    }
    if (choice != -1 && reply != SWEETENER_DECLINED)
        Diplo_Show_Reply(msg, reply);
}

/* (mode) -> the wizard picked (0 = cancelled): mode 0 "Who should I declare war on?" from the war targets, mode 1
   "Who should I break my treaty with?" from the partner's allies, then Forget It */
int Diplo_Pick_Third_Wizard(int mode)
{
    int choice;
    int count;
    int i;

    count = mode == 0 ? diplo_war_target_count : diplo_partner_ally_count;
    for (i = 0; i < count; i++) {
        strcpy(diplo_menu_items[i], "[ ");
        if (mode == 0)
            strcat(diplo_menu_items[i], wizards[p_diplo_war_targets[i]].wizard_name);
        else
            strcat(diplo_menu_items[i], wizards[p_diplo_partner_allies[i]].wizard_name);
    }
    strcpy(diplo_menu_items[i], "[ Forget It");
    diplo_menu_items[i + 1][0] = '\0';
    strcpy(p_diplo_text, mode == 0 ? "Who should I declare war on?" : "Who should I break my treaty with?");
    Font_Select(1, 4, 3);
    Set_Font_Color(0xbb);
    choice = UI_Text_List_Select(38, 142, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, 0, 15, 11, 0, 0, 0, -1);
    if (choice == -1)
        return 0;
    if (choice == count)
        return 0;
    if (mode == 0)
        return p_diplo_war_targets[choice];
    else
        return p_diplo_partner_allies[choice];
}

/* (msg, reply) shows a message and waits for a click. The answer to a proposal (0x33..0x42) is replaced: tiers 0..2
   by the stored refusal reason (1 in 2, then cleared), else "you broke treaties" (1 in 4 when one is stored, then
   cleared), else 0x39; tier 3 by "grateful for the spell" (1 in 4 when a gift spell is stored, then cleared) */
void Diplo_Show_Reply(int msg, int reply)
{
    int done;
    int click;
    int is_answer;

    is_answer = 0;
    if (msg == DIPLO_MSG_NOT_INTERESTED)
        is_answer = 1;
    if (msg >= DIPLO_MSG_REFUSE_TREATY_BREAKER && msg <= DIPLO_MSG_REFUSE_EXPANSION)   /* the refusal reasons */
        is_answer = 1;
    if (msg >= DIPLO_MSG_PACT_SIGNED && msg <= DIPLO_MSG_AGREE_JOIN_WAR)   /* the agreements */
        is_answer = 1;
    diplo_msg_type = msg;
    if (is_answer == 1) {
        switch (reply) {
        case DIPLO_REPLY_REFUSE:
        case DIPLO_REPLY_WANTS_SPELL:
        case DIPLO_REPLY_WANTS_GOLD:
            if (wizards[PLAYER_HUMAN].refusal_reason_msg[diplo_partner] != 0 && Random(2) == 1) {
                diplo_msg_type = wizards[PLAYER_HUMAN].refusal_reason_msg[diplo_partner];
                wizards[PLAYER_HUMAN].refusal_reason_msg[diplo_partner] = 0;
            } else if (wizards[PLAYER_HUMAN].broken_treaty_type[diplo_partner] != 0 && Random(4) == 1) {
                wizards[PLAYER_HUMAN].broken_treaty_type[diplo_partner] = 0;
                diplo_msg_type = DIPLO_MSG_REFUSE_TREATY_BREAKER;
            } else
                diplo_msg_type = DIPLO_MSG_NOT_INTERESTED;
            break;
        case DIPLO_REPLY_ACCEPT:
            if (Random(4) == 1 && wizards[PLAYER_HUMAN].tribute_spell_given[diplo_partner] != 0) {
                diplo_msg_spell = wizards[PLAYER_HUMAN].tribute_spell_given[diplo_partner];
                wizards[PLAYER_HUMAN].tribute_spell_given[diplo_partner] = 0;
                diplo_msg_type = DIPLO_MSG_ACCEPT_FOR_SPELL;
            } else
                diplo_msg_type = msg;
            break;
        }
    } else
        diplo_msg_type = msg;
    UI_Set_Redraw_Fn(Diplo_Reply_Redraw, 3);
    UI_Input_Delay(1);
    diplo_msg_variant = -1;
    Diplo_Load_Message(diplo_msg_type, diplo_partner);
    done = 0;
    UI_Clear_Controls();
    click = UI_Add_Region(0, 0, 319, 199, "", -1);
    while (done == 0) {
        Timer_Mark();
        if (UI_Get_Input() != 0)
            done = 1;
        if (done == 0) {
            Diplo_Reply_Redraw();
            Screen_Present();
            Wait_Ticks(3);
        }
    }
    UI_Clear_Controls();
}

/* (tier 1 / 2) -> DIPLO_REPLY_REFUSE nothing to ask, SWEETENER_DECLINED, DIPLO_REPLY_ACCEPT paid: the AI asks for gold ((Random(8) + Random(8)) x turn / 25 x
   25, dropped when the human has less) or for the first spell the human could give it (tier 1, or no gold); Agree
   pays it. BUG #62: 'has_spell' is only ever cleared - with a spell to trade it keeps whatever DI held
   Numbered: BUG #62 (BUGS_AND_QUIRKS.txt) */
int Diplo_AI_Ask_Sweetener(int tier)
{
    int has_spell;
    int spell;
    int choice;
    int unused;
    int is_gold;
    int height;
    char offer_text[80];
    char spell_name[30];
    int gold;

    unused = 0;
    gold = (Random(8) + Random(8)) * game_turn / 25 * 25;
    if (gold > wizards[PLAYER_HUMAN].gold)
        gold = 0;
    Diplo_List_Spells_To_Trade(PLAYER_HUMAN, diplo_partner, game_turn);
    if (spell_trade_count > 0)
        spell = spell_trade_list[0];
    else
        has_spell = 0;
    if (gold == 0 && has_spell == 0)
        return DIPLO_REPLY_REFUSE;
    if ((has_spell != 0 && tier == 1) || gold == 0) {
        far_strcpy(spell_name, 0, p_spell_data[spell].name);
        strcpy(offer_text, spell_name);
        strcat(offer_text, " spell");
        is_gold = 0;
    } else {
        itoa(gold, offer_text, 10);
        strcat(offer_text, " gold");
        is_gold = 1;
    }
    if (Random(2) == 1) {
        strcpy(p_diplo_text, "Perhaps if you were to throw in ");
        strcat(p_diplo_text, offer_text);
        strcat(p_diplo_text, " we could deal.");
    } else {
        strcpy(p_diplo_text, "If you also offer me ");
        strcat(p_diplo_text, offer_text);
        strcat(p_diplo_text, " I would accept.");
    }
    UI_Set_Redraw_Fn(Diplo_Reply_Redraw, 3);
    UI_Input_Delay(1);
    choice = 0;
    Font_Select(4, 4);                          /* plain Font_Select here (the other boxes: Font_Select_Shadow) */
    Set_Text_Shadow_Color(0);
    Set_Font_Color(0xbb);
    Set_Line_Spacing(1);
    height = Paragraph_Height(245, p_diplo_text, 0);
    Font_Select(1, 4, 3);
    Set_Font_Color(0xbb);
    choice = UI_Text_List_Select(38, height + 142, 245, "", diplo_agree_items[0], 12, &choice, 0, 15, 11, 0, 0, 0, -1);
    if (choice == -1 || choice == 1)
        return SWEETENER_DECLINED;
    if (is_gold == 0)
        Diplo_Give_Spell(diplo_partner, spell);
    else {
        wizards[PLAYER_HUMAN].gold -= gold;
        Wizard_Add_Gold(diplo_partner, gold);
    }
    return DIPLO_REPLY_ACCEPT;
}

/* () redraw of the reply box: portrait, eyes, mirror and the text */
void Diplo_Reply_Redraw(void)
{
    Set_Draw_Page();
    Screen_Restore_A800();
    Diplo_Draw_Portrait_Anim();
    Diplo_Draw_Eyes();
    Draw_Pic_Frame(95, 1, diplo_mirror_pic);
    Font_Select_Shadow(4, 4);
    Set_Text_Shadow_Color(0);
    Set_Font_Color(0xbb);
    Set_Line_Spacing(1);
    Paragraph_Print(38, 140, 245, p_diplo_text, PARA_LEFT);
}

/* () "Your actions:" Break Wizard Pact (pact only) / Break Alliance (alliance only) -> the treaty broken (DIPLO_MSG_REMEMBER_THIS_DAY);
   Threaten To Attack: the armies near the partner's fortress give a term - partner stronger:
   -(partner x 100 / human) - 50, below -150 (-100 with no human army); else human x 100 / partner - 50, 50 or more
   (100 with no partner army) -, + the personality and
   relation bias + Random(100) + Random(100); every threat also costs the relation Random(15), the pending relation
   change Random(5), adds a warning and sets the treaty interest to -120. Below 0 war (0x27; Diplo_Declare_War with
   the human as the declarer), from 200 it gives a spell (0x46), from 90 gold (0x45 - BUG #49: the
   human gains it but the partner pays nothing), else 0x43 "ignored". Every exit cuts the interests 10 times
   Numbered: BUG #49, #50, #51, #52, #53 (BUGS_AND_QUIRKS.txt) */
void Diplo_Threaten_Menu(void)
{
    int choice;
    int tier;
    int roll;
    int tribute;
    long partner_army;
    long human_army;
    int army_term;
    int enabled[10];
    register int i;
    register int msg;

    choice = 0;
    for (i = 0; i < 6; i++)
        enabled[i] = 1;
    strcpy(p_diplo_text, diplo_menu_titles[2]);
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] != TREATY_WIZARD_PACT)
        enabled[0] = 0;
    if (wizards[PLAYER_HUMAN].diplomacy_status[diplo_partner] != TREATY_ALLIANCE)
        enabled[1] = 0;
    for (i = 0; i < 5; i++)
        strcpy(diplo_menu_items[i], diplo_threaten_menu_strings[i]);
    *diplo_menu_items[5] = '\0';
    Font_Select(1, 4, 3);
    Set_Font_Color(0xbb);
    choice = UI_Text_List_Select(38, 142, 245, "", diplo_menu_items[0], DIPLO_MENU_ITEM_LEN, &choice, enabled, 15, 11, 0, 0, 0, -1);
    tier = 3;
    switch (choice) {
    case 0:
    case 1:
        Diplo_Break_Treaty(PLAYER_HUMAN, diplo_partner);
        msg = DIPLO_MSG_REMEMBER_THIS_DAY;
        choice = 0;
        break;
    case 2:
        choice = 0;
        if (Diplo_Army_Near_Fortress(current_player, diplo_partner, &human_army, &partner_army) == 0)
            break;
        if (partner_army > human_army) {
            if (human_army > 0)
                army_term = -(partner_army * 100 / human_army) - 50;
            else
                army_term = -100;
        } else {
            if (partner_army > 0)
                army_term = human_army * 100 / partner_army - 50;
            else
                army_term = 100;
        }
        roll = personality_treaty_mod[wizards[diplo_partner].personality] + wizards[PLAYER_HUMAN].relation_bias[diplo_partner]
               + army_term + Random(100) + Random(100);
        wizards[PLAYER_HUMAN].pending_rel_amount[diplo_partner] -= Random(5);
        wizards[PLAYER_HUMAN].warning_count[diplo_partner] = wizards[PLAYER_HUMAN].warning_count[diplo_partner] + 1;
        wizards[PLAYER_HUMAN].relationship[diplo_partner] = wizards[PLAYER_HUMAN].relationship[diplo_partner] - Random(15);
        wizards[PLAYER_HUMAN].treaty_interest[diplo_partner] = -120;
        if (wizards[PLAYER_HUMAN].relationship[diplo_partner] < -100)
            wizards[PLAYER_HUMAN].relationship[diplo_partner] = -100;
        wizards[diplo_partner].relationship[PLAYER_HUMAN] = wizards[PLAYER_HUMAN].relationship[diplo_partner];
        if (roll < 90) {
            if (roll < 0) {
                Diplo_Declare_War(PLAYER_HUMAN, diplo_partner);
                msg = DIPLO_MSG_WAR_BEGUN;
            } else
                msg = DIPLO_MSG_IGNORE_THREATS;
        } else {
            msg = DIPLO_MSG_IGNORE_THREATS;
            if (roll >= 200) {
                Diplo_List_Spells_To_Trade(diplo_partner, 0, 0);
                if (spell_trade_count > 0) {
                    wizards[PLAYER_HUMAN].peace_turns[diplo_partner] = Random(15) + 5;
                    msg = DIPLO_MSG_APPEASE_WITH_SPELL;
                    diplo_msg_spell = spell_trade_list[0];
                    Diplo_Give_Spell(PLAYER_HUMAN, diplo_msg_spell);
                }
            } else if (roll >= 90) {
                tribute = 25 * ((long)(Random(8) + 2) * game_turn / 25);
                if (wizards[diplo_partner].gold < tribute) {
                    if (wizards[diplo_partner].gold > 0)
                        tribute = wizards[diplo_partner].gold / 4;
                    else
                        tribute = 0;
                }
                if (tribute != 0) {
                    wizards[PLAYER_HUMAN].peace_turns[diplo_partner] = Random(15) + 5;
                    Wizard_Add_Gold(current_player, tribute);
                    msg = DIPLO_MSG_STOP_ATTACKS_GOLD;
                    diplo_msg_number = tribute;
                }
            }
        }
        break;
    }
    for (i = 0; i < 10; i++)
        Diplo_Interests_Minus10(PLAYER_HUMAN, diplo_partner);
    if (choice != -1 && tier != 1 && choice != 3)   /* tier != 1: always true (never set to 1 here) */
        Diplo_Show_Reply(msg, tier);
}

/* (mood 0..2) -> 0 when it is the mood already playing: with music on, the partner's Good (0 / 1) or Bad (2)
   track; remembers the mood */
int Diplo_Play_Mood_Music(int mood)
{
    if (mood == diplo_music_mood)
        return 0;
    if (game_config.music == 1) {
        switch (mood) {
        case 0:
            Sound_Play(diplo_music_good);
            break;
        case 1:
            Sound_Play(diplo_music_good);
            break;
        case 2:
            Sound_Play(diplo_music_bad);
            break;
        }
    }
    diplo_music_mood = mood;
}

/* () the partner's portrait at (107, 13): states 0 / 1 play the talk animation (24 frames) then the good face (3),
   2 the talk then the bad face (4); 3 good, 4 bad, 7 neutral face; 5 / 6 nothing. A changed state restarts its
   picture */
void Diplo_Draw_Portrait_Anim(void)
{
    int frame;
    int k;
    register int x;
    register int y;

    x = 107;
    y = 13;
    if (diplo_anim_state != diplo_anim_prev) {
        switch (diplo_anim_state) {
        case DIPLO_ANIM_TALK_GOOD:
            Set_Pic_Frame(diplo_talk_pic, 0);
            break;
        case DIPLO_ANIM_TALK_GOOD_B:
            Set_Pic_Frame(diplo_talk_pic, 0);
            break;
        case DIPLO_ANIM_TALK_BAD:
            Set_Pic_Frame(diplo_talk_pic, 0);
            break;
        case DIPLO_ANIM_GOOD:
            Set_Pic_Frame(diplo_face_pic, 0);
            break;
        case DIPLO_ANIM_BAD:
            Set_Pic_Frame(diplo_face_pic, 1);
            break;
        case DIPLO_ANIM_NONE:
            Set_Pic_Frame(diplo_talk_pic, 0);
            break;
        case DIPLO_ANIM_NONE_B:
            Set_Pic_Frame(diplo_talk_pic, 0);
            break;
        case DIPLO_ANIM_NEUTRAL:
            Set_Pic_Frame(diplo_face_pic, 2);
            break;
        }
    }
    frame = Pic_Get_Frame(diplo_talk_pic);
    Set_Draw_Page();
    switch (diplo_anim_state) {
    case DIPLO_ANIM_TALK_GOOD:
    case DIPLO_ANIM_TALK_GOOD_B:
        if (frame < 24) {
            Set_Pic_Frame(diplo_talk_pic, 0);
            for (k = 0; k <= frame; k++)
                Draw_Pic_Frame(x, y, diplo_talk_pic);
        } else {
            Set_Pic_Frame(diplo_face_pic, 0);
            Draw_Pic_Frame(x, y, diplo_face_pic);
            diplo_anim_state = DIPLO_ANIM_GOOD;
        }
        break;
    case DIPLO_ANIM_TALK_BAD:
        if (frame < 24) {
            Set_Pic_Frame(diplo_talk_pic, 0);
            for (k = 0; k <= frame; k++)
                Draw_Pic_Frame(x, y, diplo_talk_pic);
        } else {
            diplo_anim_state = DIPLO_ANIM_BAD;
            Set_Pic_Frame(diplo_face_pic, 0);
            Draw_Pic_Frame(x, y, diplo_face_pic);
            Draw_Pic_Frame(x, y, diplo_face_pic);
        }
        break;
    case DIPLO_ANIM_GOOD:
        Set_Pic_Frame(diplo_face_pic, 0);
        Draw_Pic_Frame(x, y, diplo_face_pic);
        break;
    case DIPLO_ANIM_BAD:
        Set_Pic_Frame(diplo_face_pic, 0);
        Draw_Pic_Frame(x, y, diplo_face_pic);
        Draw_Pic_Frame(x, y, diplo_face_pic);
        break;
    case DIPLO_ANIM_NEUTRAL:
        Set_Pic_Frame(diplo_face_pic, 0);
        Draw_Pic_Frame(x, y, diplo_face_pic);
        Draw_Pic_Frame(x, y, diplo_face_pic);
        Draw_Pic_Frame(x, y, diplo_face_pic);
        break;
    case DIPLO_ANIM_NONE:
        break;
    }
    diplo_anim_prev = diplo_anim_state;
}

/* () loads the screen: COMBAT.TMP saved and the cities moved to the LBX block, the partner's Good / Bad music, the
   background and the eyes; draws background and eyes on both pages, fades the portrait in (unless it refuses to talk)
   and loads the rest (Diplo_Load_Assets). The music overwrites the map layers in the world block until
   Combat_Tmp_Restore (BUG #491: Diplo_Army_Near_Fortress reads them meanwhile) */
void Diplo_Screen_Load(void)
{
    int level;                                  /* set, never used */
    int i;

    Sound_Stop();
    Load_Diplomacy_Art();
    Combat_Tmp_Save();
    p_cities = Seg_To_Far_Ptr(Heap_Alloc_Front(p_world_block, 0x2ca));   /* MAX_CITIES x 0x72 bytes */
    diplo_music_good = LBX_Reload_Next("MUSIC", wizards[diplo_partner].portrait * 2 + 41, p_world_block);
    diplo_music_bad = LBX_Reload_Next("MUSIC", wizards[diplo_partner].portrait * 2 + 42, p_world_block);
    level = (wizards[PLAYER_HUMAN].relationship[diplo_partner] + 100) / 20;
    p_diplo_eyes_left = (unsigned *)Near_Alloc_Reset(22);
    p_diplo_eyes_right = (unsigned *)Near_Alloc(22);
    diplo_background_pic = LBX_Reload("DIPLOMAC", 0, p_scratch);
    for (i = 0; i < 11; i++)
        p_diplo_eyes_left[i] = LBX_Reload_Next("DIPLOMAC", i + 2, p_scratch);
    for (i = 0; i < 11; i++)
        p_diplo_eyes_right[i] = LBX_Reload_Next("DIPLOMAC", i + 13, p_scratch);
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    Draw_Pic_Frame(0, 0, diplo_background_pic);
    Diplo_Draw_Eyes();
    UI_Clear_Controls();
    Palette_Upload();
    UI_Present_Frame();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    Draw_Pic_Frame(0, 0, diplo_background_pic);
    Diplo_Draw_Eyes();
    Screen_Copy_Front();
    Screen_Save_A800();
    if (diplo_talk_mode != DIPLO_TALK_REFUSED)
        Diplo_Portrait_Fade_In();
    Diplo_Load_Assets();
}

/* () the near arena: the message text and cache, the menu strings, unused strings and lists, the two target lists,
   the eye arrays; MOODWIZ face, the BACKGRND mirror, the talk animation and the eyes (again) */
void Diplo_Load_Assets(void)
{
    int talk;
    int level;                                  /* set, never used */
    int i;

    talk = wizards[diplo_partner].portrait + 24;
    level = (wizards[PLAYER_HUMAN].relationship[diplo_partner] + 100) / 20;
    p_diplo_text = Near_Alloc_Reset(0xdc);
    p_diplo_msg_cache = Near_Alloc(0xdc);
    for (i = 0; i < 7; i++)
        diplo_menu_items[i] = Near_Alloc(DIPLO_MENU_ITEM_LEN);
    for (i = 0; i < 6; i++)
        diplo_str_ptrs_unused[i] = Near_Alloc(50);
    diplo_list_unused = Near_Alloc(20);
    p_diplo_war_targets = (int *)Near_Alloc(20);
    p_diplo_partner_allies = (int *)Near_Alloc(20);
    p_diplo_eyes_left = (unsigned *)Near_Alloc(22);
    p_diplo_eyes_right = (unsigned *)Near_Alloc(22);
    diplo_face_pic = LBX_Reload("MOODWIZ", wizards[diplo_partner].portrait, p_scratch);
    diplo_mirror_pic = LBX_Reload_Next("BACKGRND", 18, p_scratch);
    diplo_talk_pic = LBX_Reload_Next("DIPLOMAC", talk, p_scratch);
    for (i = 0; i < 11; i++)
        p_diplo_eyes_left[i] = LBX_Reload_Next("DIPLOMAC", i + 2, p_scratch);
    for (i = 0; i < 11; i++)
        p_diplo_eyes_right[i] = LBX_Reload_Next("DIPLOMAC", i + 13, p_scratch);
}

/* () the two "eyes" by the partner's relation to the human ((relation + 100) / 20 = 0..10) */
void Diplo_Draw_Eyes(void)
{
    int level;

    level = (wizards[diplo_partner].relationship[PLAYER_HUMAN] + 100) / 20;
    Draw_Pic_Frame(64, 58, p_diplo_eyes_left[level]);
    Draw_Pic_Frame(233, 58, p_diplo_eyes_right[level]);
}

/* () the portrait fading in: DIPLOMAC 38 + portrait (5 frames) under the mirror, 3 ticks each after 6 */
void Diplo_Portrait_Fade_In(void)
{
    int entry;
    unsigned pic;
    int i;

    entry = wizards[diplo_partner].portrait + 38;
    pic = LBX_Reload("DIPLOMAC", entry, p_scratch);
    diplo_mirror_pic = LBX_Reload_Next("BACKGRND", 18, p_scratch);
    Timer_Mark();
    Wait_Ticks(6);
    for (i = 0; i < 5; i++) {
        Timer_Mark();
        Set_Draw_Page();
        Draw_Pic_Frame(107, 13, pic);
        Draw_Pic_Frame(95, 1, diplo_mirror_pic);
        Screen_Present();
        Wait_Ticks(3);
    }
}

/* () empty; no caller */
void Diplo_Uncalled_Nop(void)
{
}

/* (a, b, &army_a, &army_b) -> 0 when a or b is the neutral player, else 1 with the summed unit costs of a's and b's
   units on the plane and landmass of b's fortress. BUG #491: inside an audience p_landmass_ids holds the partner's
   music (Diplo_Screen_Load), so the threat and proposal callers count arbitrary squares
   Numbered: BUG #491 (BUGS_AND_QUIRKS.txt) */
int Diplo_Army_Near_Fortress(int a, int b, long *army_a, long *army_b)
{
    int landmass;
    int plane;
    int unit;

    *army_a = 0;
    *army_b = 0;
    if (b == PLAYER_NEUTRAL || a == PLAYER_NEUTRAL)
        return 0;
    landmass = p_landmass_ids[p_fortresses[b].plane][p_fortresses[b].y][p_fortresses[b].x];
    plane = p_fortresses[b].plane;
    *army_a = 0;
    *army_b = 0;
    for (unit = 0; unit < num_units; unit++) {
        if ((p_units[unit].owner == a || p_units[unit].owner == b) && p_units[unit].plane == plane
            && p_landmass_ids[p_units[unit].plane][p_units[unit].y][p_units[unit].x] == landmass) {
            if (p_units[unit].owner == a)
                *army_a = *army_a + unit_types[p_units[unit].type].cost;
            else
                *army_b = *army_b + unit_types[p_units[unit].type].cost;
        }
    }
    return 1;
}
