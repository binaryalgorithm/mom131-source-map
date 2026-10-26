/* MAGICSCR.C - WIZARDS.EXE overlay code segment 6080 (stub 2474): the magic screen - the three staffs (mana,
   research and casting skill shares of the power base, in steps of 2%; a staff can be locked and moving one moves
   the unlocked others), the income, casting skill, reserve and power base lines, what is being cast, researched
   and where summons go; a gem per rival wizard (left click: diplomacy, right click: the wizard info box) with the
   relation, the treaty icons and, under Detect Magic, the spell being cast; the list of global enchantments in
   play (the player's own can be cancelled); and the Alchemy box (gold <-> mana, 2:1 or 1:1 with the Alchemy
   retort, at most 999 per transaction). Alt-codes RVL (Reveal_Map_All), PWR (Cheat_PWR) and DESTIN (a developer
   message box).
   This module's data (DS:347e..35a3): alchemy_gold_to_mana, then the literals ("" is the tail of "RVL", the
   research slider's hotkey "R" the tail of "PWR", Cancel's "C" the tail of "MAGIC": -d); BSS (DS:c1ea..c253).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6080 src/wizards/MAGICSCR.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_Magic(void);
void MagicScr_Draw(void);
void MagicScr_Add_Controls(void);
void MagicScr_Build_Global_Ench_List(void);
void MagicScr_Load(void);
void Alchemy_Dialog(int x, int y);
void Alchemy_Draw(void);
void Alchemy_Add_Controls(void);

#include "GAME.H"

/* the global-enchantment list: one column up to 9, then two columns of 9 a page */
#define MAGIC_LIST_ROWS 18
#define MAGIC_EXIT_MAP (-1)             /* Screen_Magic's exit_reason (0: stay) */
#define MAGIC_EXIT_DIPLOMACY 2

int alchemy_gold_to_mana = 1;                   /* DS:347e  1 = gold to mana, 0 = mana to gold */

int magic_staff_skill;                          /* DS:c252  staff heights 0..50 (= percentage / 2) */
int magic_staff_research;                       /* DS:c250 */
int magic_staff_mana;                           /* DS:c24e */
int magic_lock_skill_control;                   /* DS:c24c */
int magic_lock_research_control;                /* DS:c24a */
int magic_lock_mana_control;                    /* DS:c248 */
int magic_skill_prev;                           /* DS:c246  the heights at the last redraw */
int magic_research_prev;                        /* DS:c244 */
int magic_mana_prev;                            /* DS:c242 */
int magic_ok_button;                            /* DS:c240 */
int magic_alchemy_button;                       /* DS:c23e */
int *p_magic_enchant_spell;                     /* DS:c23c  the global enchantments in play: spell */
char *p_magic_enchant_owner;                    /* DS:c23a  and its caster */
int magic_enchant_count;                        /* DS:c238 */
int magic_list_scroll;                          /* DS:c236  first row shown */
int magic_list_has_more;                         /* DS:c234  more than 18: the "More..." control */
int magic_list_more_control;                    /* DS:c232 */
int *p_magic_enchant_ctls;                      /* DS:c230 */
int *p_magic_gem_ctls;                          /* DS:c22e */
int magic_enchant_rows_shown;                   /* DS:c22c */
unsigned alchemy_arrow_pic;                     /* DS:c22a  the arrow bar's working copy (mirrored for mana to gold) */
int alchemy_box_x;                              /* DS:c228 */
int alchemy_box_y;                              /* DS:c226 */
int alchemy_amount;                             /* DS:c224 */
int alchemy_slider_pos;                         /* DS:c222  3..53 */
int alchemy_anim_frame;                         /* DS:c220  0..7 */
int alchemy_cancel_button;                      /* DS:c21e */
int alchemy_ok_button;                          /* DS:c21c */
int alchemy_direction_button;                   /* DS:c21a */
int alchemy_box_control;                        /* DS:c218 */
int alchemy_outside_control;                    /* DS:c216 */
int magic_gem_count;                            /* DS:c214  gems shown = num_players - 1 */
int magic_gem_wizard[4];                        /* DS:c20c  -2 fortress gone (dead / banished), -1 not contacted, else the wizard */
                                                /* also: Screen_Wizard_Target's gem control ids (CASTANIM.C) */
int magic_summon_city;                          /* DS:c20a  -1 = none */
int alchemy_ratio;                              /* DS:c208  2, 1 with the Alchemy retort */
unsigned treaty_icon_pics[15];                  /* DS:c1ea  MAGIC.LBX 60..74: 3 per banner color (pact, alliance,
                                                   war) - owner not fixed by the EXE */

extern char *relation_names[];                  /* DS:35a4 */
extern int time_stop_player;                    /* DS:912a  Time Stop caster + 1 (0 = none) */
extern int magic_lock_mana;                     /* DS:947e */
extern int magic_lock_research;                 /* DS:9480 */
extern int magic_lock_skill;                    /* DS:9482 */
extern unsigned alchemy_pow2gold_pic;           /* DS:9484  MAGIC.LBX 59 */
extern unsigned alchemy_arrow_bar_pic;          /* DS:9486  MAGIC.LBX 57 */
extern unsigned alchemy_right_arrow_pic;        /* DS:9488  MAGIC.LBX 55 */
extern unsigned alchemy_left_arrow_pic;         /* DS:948a  MAGIC.LBX 56 */
extern unsigned alchemy_cancel_pic;             /* DS:948c  MAGIC.LBX 53 */
extern unsigned alchemy_ok_pic;                 /* DS:948e  MAGIC.LBX 54 */
extern unsigned alchemy_star_pic;               /* DS:9494  MAGIC.LBX 58 */
extern unsigned alchemy_bg_pic;                 /* DS:9496  MAGIC.LBX 52 */
extern unsigned magic_gem_pics[];               /* DS:958e */
extern unsigned staff_skill_locked_pic;         /* DS:95aa  MAGIC.LBX 17 */
extern unsigned staff_research_locked_pic;      /* DS:95ac  MAGIC.LBX 16 */
extern unsigned staff_mana_locked_pic;          /* DS:95ae  MAGIC.LBX 15 */
extern unsigned magic_ok_button_pic;            /* DS:95b0  MAGIC.LBX 14 */
extern unsigned magic_alchemy_button_pic;       /* DS:95b2  MAGIC.LBX 13 */
extern unsigned staff_skill_full_pic;           /* DS:95b4  MAGIC.LBX 12 */
extern unsigned staff_skill_empty_pic;          /* DS:95b6  MAGIC.LBX 11 */
extern unsigned staff_research_full_pic;        /* DS:95b8  MAGIC.LBX 10 */
extern unsigned staff_research_empty_pic;       /* DS:95ba  MAGIC.LBX 9 */
extern unsigned staff_mana_full_pic;            /* DS:95bc  MAGIC.LBX 8 */
extern unsigned staff_mana_empty_pic;           /* DS:95be  MAGIC.LBX 7 */
extern unsigned magic_gem_frame_pic;            /* DS:95c0  MAGIC.LBX 6 */
extern unsigned magic_overlay_pic;              /* DS:95c2  MAGIC.LBX 51 */
extern unsigned magicscr_bg_pic;                /* DS:95ce  MAGIC.LBX 0 */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int diplo_partner;                       /* DS:c3fe */
extern int spend_per_turn;                      /* DS:c90a */
extern int net_mana_per_turn;                   /* DS:c90e */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Cheat_PWR(void);
int Confirm_Box(char *text);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Fortress_Hero_Skill_Bonus(int player);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Help_Load_Alchemy(void);
void Help_Load_Magic_Screen(void);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Pic_Mirror(unsigned pic);
void Play_Click_Sound(void);
int Print_Far_Text(int x, int y, char far *text);
void Print_Far_Text_Centered(int x, int y, char far *text);
int Print_Text_Centered(int x, int y, char *text);
void Reveal_Map_All(void);
void Screen_Copy_Front(void);
void Screen_Wizard_Info(int wizard, int from_x1, int from_y1, int from_x2, int from_y2);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Font_Color(int color);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Set_Space_Width(int width);
int UI_Add_Alt_Hotkey(char *code);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Add_Hotkey(char *key);
int UI_Add_Slider_Near(int x, int y, int scale_min, int scale_max, int min, int max, int width, int height,
                       int *value, char *hotkey, int help);
void UI_Input_Delay(int delay);
void UI_Set_Control_Count(int n);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Warning_Box(char *text);
void Wizard_Add_Gold(int player, int amount);
void Wizard_Add_Mana(int player, int amount);
void Wizard_Magic_Income(int *mana, int *research, int *skill, int player);
void Wizard_Power_Base(void);
int Wizard_Summoning_City(int player);

/* the magic screen (main-loop screen 6) until OK or Escape (back to the map) or a gem's left click (diplomacy)
   Numbered: BUG #42 (BUGS_AND_QUIRKS.txt) */
void Screen_Magic(void)
{
    int i;
    int input;
    int exit_reason;
    char *globals;
    int skip;
    int hk_x;
    int x1;
    int y1;
    int x2;
    int y2;
    int esc;
    int destin;
    int rvl;
    int pwr;

    UI_Clear_Help();
    Help_Load_Magic_Screen();
    skip = 0;
    for (i = 0; i < 4; i++) {
        help_entries[i].id = HELP_GRAY_GEM;
        help_entries[i + 10].id = -1;
    }
    magic_gem_count = 0;
    for (i = 1; i < num_players; i++) {
        magic_gem_wizard[magic_gem_count] = -2;
        help_entries[magic_gem_count].id = HELP_SHATTERED_GEM;
        help_entries[magic_gem_count + 10].id = -1;
        if (p_fortresses[i].active == 1) {
            magic_gem_wizard[magic_gem_count] = -1;
            help_entries[magic_gem_count].id = HELP_GRAY_GEM;
        }
        if (p_fortresses[i].active == 1 && wizards[current_player].contacted[i] == 1) {
            magic_gem_wizard[magic_gem_count] = i;
            help_entries[magic_gem_count].id = -1;
            help_entries[magic_gem_count + 10].id = HELP_RELATIONS;
        }
        magic_gem_count++;
    }
    MagicScr_Load();
    magic_staff_skill = wizards[PLAYER_HUMAN].skill_percentage / 2;
    magic_staff_research = wizards[PLAYER_HUMAN].research_percentage / 2;
    magic_staff_mana = 50 - magic_staff_skill - magic_staff_research;
    magic_skill_prev = magic_staff_skill;
    magic_research_prev = magic_staff_research;
    magic_mana_prev = magic_staff_mana;
    Wizard_Power_Base();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(MagicScr_Draw, 1);
    magic_list_scroll = 0;
    MagicScr_Build_Global_Ench_List();
    magic_list_has_more = 0;
    if (magic_enchant_count > MAGIC_LIST_ROWS) {
        magic_list_has_more = 1;
    }
    magic_summon_city = Wizard_Summoning_City(current_player);
    UI_Input_Delay(1);
    exit_reason = 0;
    rvl = UI_Add_Alt_Hotkey("RVL");
    pwr = UI_Add_Alt_Hotkey("PWR");
    destin = UI_Add_Alt_Hotkey("DESTIN");
    while (exit_reason == 0) {
        Timer_Mark();
        UI_Set_Control_Count(destin);
        MagicScr_Add_Controls();
        hk_x = UI_Add_Hotkey("X");
        esc = UI_Add_Hotkey("\x1b");
        input = UI_Get_Input();
        if (input == rvl) {
            Reveal_Map_All();
            skip = 1;
        }
        if (input == pwr) {
            Cheat_PWR();
            skip = 1;
        }
        if (input == destin) {
            Warning_Box("Write it up, give me a save game.");
            UI_Set_Redraw_Fn(MagicScr_Draw, 1);
            UI_Clear_Help();
            Help_Load_Magic_Screen();
            skip = 1;
        }
        if (input == magic_lock_mana_control) {
            Play_Click_Sound();
            magic_lock_mana = !magic_lock_mana;
        }
        if (input == magic_lock_skill_control) {
            Play_Click_Sound();
            magic_lock_skill = !magic_lock_skill;
        }
        if (input == magic_lock_research_control) {
            Play_Click_Sound();
            magic_lock_research = !magic_lock_research;
        }
        if (input == magic_ok_button || input == esc) {
            Play_Click_Sound();
            exit_reason = MAGIC_EXIT_MAP;
        }
        for (i = 0; i < magic_enchant_rows_shown; i++) {
            if (p_magic_enchant_ctls[i] == input && p_magic_enchant_owner[i + magic_list_scroll] == current_player) {
                Play_Click_Sound();
                strcpy(p_text_buf, "Do you wish to cancel your \x02");
                far_strcpy(p_text_buf2, 0, p_spell_data[p_magic_enchant_spell[i + magic_list_scroll]].name);
                strcat(p_text_buf, p_text_buf2);
                strcat(p_text_buf, "\x01 spell.");   /* a question ending with a period; clarified using ReMoM repo */
                if (Confirm_Box(p_text_buf) == 1) {
                    globals = (char *)&wizards[PLAYER_HUMAN].globals;
                    globals[(int)p_spell_data[p_magic_enchant_spell[i + magic_list_scroll]].effect_param] = 0;
                    if ((int)p_spell_data[p_magic_enchant_spell[i + magic_list_scroll]].effect_param == GLOBAL_ENCH_NATURE_AWARENESS) {
                        Visibility_Rebuild();
                    }
                    if ((int)p_spell_data[p_magic_enchant_spell[i + magic_list_scroll]].effect_param == GLOBAL_ENCH_TIME_STOP) {
                        time_stop_player = 0;
                    }
                    magic_list_scroll = 0;
                    MagicScr_Build_Global_Ench_List();
                    magic_list_has_more = 0;
                    if (magic_enchant_count > MAGIC_LIST_ROWS) {
                        magic_list_has_more = 1;
                    }
                }
                UI_Set_Redraw_Fn(MagicScr_Draw, 1);
                UI_Clear_Help();
                Help_Load_Magic_Screen();
                skip = 1;
                UI_Clear_Controls();
                rvl = UI_Add_Alt_Hotkey("RVL");
                pwr = UI_Add_Alt_Hotkey("PWR");
                destin = UI_Add_Alt_Hotkey("DESTIN");
                break;
            }
        }
        if (input == magic_alchemy_button) {
            Play_Click_Sound();
            Alchemy_Dialog(80, 60);
            UI_Set_Redraw_Fn(MagicScr_Draw, 1);
            MagicScr_Load();
            MagicScr_Build_Global_Ench_List();
            skip = 1;
            if (net_mana_per_turn + wizards[PLAYER_HUMAN].mana_reserve > wizards[PLAYER_HUMAN].nominal_casting_skill) {
                spend_per_turn = wizards[PLAYER_HUMAN].nominal_casting_skill;
            } else if (net_mana_per_turn + wizards[PLAYER_HUMAN].mana_reserve > 0) {
                spend_per_turn = net_mana_per_turn + wizards[PLAYER_HUMAN].mana_reserve;
            } else {
                spend_per_turn = 0;
            }
            UI_Clear_Help();
            Help_Load_Magic_Screen();
            present_mode = PRESENT_DISSOLVE;
            UI_Clear_Controls();
            rvl = UI_Add_Alt_Hotkey("RVL");
            pwr = UI_Add_Alt_Hotkey("PWR");
            destin = UI_Add_Alt_Hotkey("DESTIN");
            skip = 1;
        }
        for (i = 0; i < magic_gem_count; i++) {
            if (-p_magic_gem_ctls[i] == input && magic_gem_wizard[i] > 0) {   /* -id: a right click */
                Play_Click_Sound();
                x1 = i * 77 + 29;
                y1 = 10;
                x2 = x1 + 30;
                y2 = y1 + 30;
                Screen_Wizard_Info(magic_gem_wizard[i], x1, y1, x2, y2);
                MagicScr_Load();
                MagicScr_Build_Global_Ench_List();
                UI_Set_Redraw_Fn(MagicScr_Draw, 1);
                UI_Clear_Help();
                Help_Load_Magic_Screen();
                UI_Clear_Controls();
                rvl = UI_Add_Alt_Hotkey("RVL");
                pwr = UI_Add_Alt_Hotkey("PWR");
                destin = UI_Add_Alt_Hotkey("DESTIN");
                skip = 1;
                break;
            }
        }
        for (i = 0; i < magic_gem_count; i++) {
            if (p_magic_gem_ctls[i] == input && magic_gem_wizard[i] > 0) {
                if (wizards[current_player].casting_spell == SPELL_SPELL_OF_RETURN) {
                    Warning_Box("You may not contact other wizards while you are banished.");
                } else {
                    Play_Click_Sound();
                    diplo_partner = magic_gem_wizard[i];
                    current_screen = SCREEN_DIPLOMACY;
                    exit_reason = MAGIC_EXIT_DIPLOMACY;
                }
                break;
            }
        }
        if (abs(input) == magic_list_more_control) {
            Play_Click_Sound();
            magic_list_scroll += MAGIC_LIST_ROWS;
            if (magic_list_scroll >= magic_enchant_count) {
                magic_list_scroll = 0;
            }
            skip = 1;
        }
        if (exit_reason == 0 && skip == 0) {
            MagicScr_Draw();
            Screen_Present();
            Wait_Ticks(1);
        }
        skip = 0;
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    current_screen = SCREEN_MAP;
    if (exit_reason == MAGIC_EXIT_DIPLOMACY) {
        current_screen = SCREEN_DIPLOMACY;
    }
}

/* redraw callback: rebalances the staffs after a slider moved (the change goes to the unlocked other staffs, half
   each, a remainder stays), draws them, the income and status lines, the gems and the enchantment list
   Numbered: BUG #264 (BUGS_AND_QUIRKS.txt) */
void MagicScr_Draw(void)
{
    int i;
    int delta;
    int half;
    int mana;
    int skill;
    int research;
    int j;
    int rel;
    char colors[6];
    char banner_colors[6];
    int gem;
    int status;
    int gem_x;
    int y;

    banner_colors[0] = 0xac;
    banner_colors[1] = 0xd8;
    banner_colors[2] = 0x7b;
    banner_colors[3] = 0xc9;
    banner_colors[4] = 0xd3;
    for (i = 0; i < 4; i++) {
        help_entries[i].id = HELP_GRAY_GEM;
        help_entries[i + 10].id = -1;
        help_entries[i + 43].id = -1;
    }
    gem = 0;
    for (i = 1; i < num_players; i++) {
        help_entries[gem].id = HELP_SHATTERED_GEM;
        help_entries[gem + 10].id = -1;
        if (p_fortresses[i].active == 1) {
            help_entries[gem].id = HELP_GRAY_GEM;
        }
        if (p_fortresses[i].active == 1 && wizards[current_player].contacted[i] == 1) {
            help_entries[gem].id = -1;
            help_entries[gem + 10].id = HELP_RELATIONS;
        }
        gem++;
    }
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, magicscr_bg_pic);
    Set_Pic_Frame(magic_ok_button_pic, 0);
    Draw_Pic_Frame(291, 181, magic_ok_button_pic);
    Set_Pic_Frame(magic_alchemy_button_pic, 0);
    Draw_Pic_Frame(232, 181, magic_alchemy_button_pic);
    if (magic_skill_prev != magic_staff_skill && magic_lock_skill != 0) {
        magic_staff_skill = magic_skill_prev;
    }
    if (magic_research_prev != magic_staff_research && magic_lock_research != 0) {
        magic_staff_research = magic_research_prev;
    }
    if (magic_mana_prev != magic_staff_mana && magic_lock_mana != 0) {
        magic_staff_mana = magic_mana_prev;
    }
    if (magic_skill_prev != magic_staff_skill) {
        delta = magic_skill_prev - magic_staff_skill;
        if (magic_lock_research + magic_lock_mana == 2) {
            magic_staff_skill = magic_skill_prev;
        } else if (magic_lock_research != 0 && magic_lock_mana == 0) {
            if (magic_staff_mana + delta < 0) {
                magic_staff_skill = magic_skill_prev + magic_staff_mana;
                magic_staff_mana = 0;
            } else {
                magic_staff_mana += delta;
            }
        } else if (magic_lock_mana != 0 && magic_lock_research == 0) {
            if (magic_staff_research + delta < 0) {
                magic_staff_skill = magic_skill_prev + magic_staff_research;
                magic_staff_research = 0;
            } else {
                magic_staff_research += delta;
            }
        } else {
            magic_staff_skill += delta % 2;
            half = delta / 2;
            delta = half;
            if (magic_staff_research + half < 0) {
                delta += magic_staff_research + half;
                magic_staff_research = 0;
            } else {
                magic_staff_research += half;
            }
            if (magic_staff_mana + delta < 0) {
                magic_staff_research += magic_staff_mana + delta;
                magic_staff_mana = 0;
            } else {
                magic_staff_mana += delta;
            }
        }
    } else if (magic_research_prev != magic_staff_research) {
        delta = magic_research_prev - magic_staff_research;
        if (magic_lock_skill + magic_lock_mana == 2) {
            magic_staff_research = magic_research_prev;
        } else if (magic_lock_skill != 0 && magic_lock_mana == 0) {
            if (magic_staff_mana + delta < 0) {
                magic_staff_research = magic_research_prev + magic_staff_mana;
                magic_staff_mana = 0;
            } else {
                magic_staff_mana += delta;
            }
        } else if (magic_lock_mana != 0 && magic_lock_skill == 0) {
            if (magic_staff_skill + delta < 0) {
                magic_staff_research = magic_research_prev + magic_staff_skill;
                magic_staff_skill = 0;
            } else {
                magic_staff_skill += delta;
            }
        } else {
            magic_staff_research += delta % 2;
            half = delta / 2;
            delta = half;
            if (magic_staff_skill + half < 0) {
                delta += magic_staff_skill + half;
                magic_staff_skill = 0;
            } else {
                magic_staff_skill += half;
            }
            if (magic_staff_mana + delta < 0) {
                magic_staff_skill += magic_staff_mana + delta;
                magic_staff_mana = 0;
            } else {
                magic_staff_mana += delta;
            }
        }
    } else if (magic_mana_prev != magic_staff_mana) {
        delta = magic_mana_prev - magic_staff_mana;
        if (magic_lock_research + magic_lock_skill == 2) {
            magic_staff_mana = magic_mana_prev;
        } else if (magic_lock_research != 0 && magic_lock_skill == 0) {
            if (magic_staff_skill + delta < 0) {
                magic_staff_mana = magic_mana_prev + magic_staff_skill;
                magic_staff_skill = 0;
            } else {
                magic_staff_skill += delta;
            }
        } else if (magic_lock_skill != 0 && magic_lock_research == 0) {
            if (magic_staff_research + delta < 0) {
                magic_staff_mana = magic_mana_prev + magic_staff_research;
                magic_staff_research = 0;
            } else {
                magic_staff_research += delta;
            }
        } else {
            magic_staff_mana += delta % 2;
            half = delta / 2;
            delta = half;
            if (magic_staff_research + half < 0) {
                delta += magic_staff_research + half;
                magic_staff_research = 0;
            } else {
                magic_staff_research += half;
            }
            if (magic_staff_skill + delta < 0) {
                magic_staff_research += magic_staff_skill + delta;
                magic_staff_skill = 0;
            } else {
                magic_staff_skill += delta;
            }
        }
    }
    magic_mana_prev = magic_staff_mana;
    magic_research_prev = magic_staff_research;
    magic_skill_prev = magic_staff_skill;
    if (magic_mana_prev < 0) {
        magic_mana_prev = 0;
    }
    if (magic_research_prev < 0) {
        magic_research_prev = 0;
    }
    if (magic_skill_prev < 0) {
        magic_skill_prev = 0;
    }
    Set_Clip_Rect(0, 79, 319, 152 - magic_staff_mana);
    Draw_Pic_Clipped(29, 83, staff_mana_empty_pic);
    Set_Clip_Rect(0, 152 - magic_staff_mana, 319, 166);
    if (magic_lock_mana != 0) {
        Draw_Pic_Frame(27, 81, staff_mana_locked_pic);
    }
    Draw_Pic_Clipped(32, 102, staff_mana_full_pic);
    Set_Clip_Rect(0, 79, 319, 152 - magic_staff_research);
    Draw_Pic_Clipped(75, 85, staff_research_empty_pic);
    Set_Clip_Rect(0, 152 - magic_staff_research, 319, 166);
    if (magic_lock_research != 0) {
        Draw_Pic_Frame(74, 81, staff_research_locked_pic);
    }
    Draw_Pic_Clipped(79, 102, staff_research_full_pic);
    Set_Clip_Rect(0, 79, 319, 152 - magic_staff_skill);
    Draw_Pic_Clipped(122, 83, staff_skill_empty_pic);
    Set_Clip_Rect(0, 152 - magic_staff_skill, 319, 166);
    if (magic_lock_skill != 0) {
        Draw_Pic_Frame(121, 81, staff_skill_locked_pic);
    }
    Draw_Pic_Clipped(126, 102, staff_skill_full_pic);
    Reset_Clip_Rect();
    wizards[PLAYER_HUMAN].mana_percentage = magic_staff_mana * 2;
    wizards[PLAYER_HUMAN].research_percentage = magic_staff_research * 2;
    wizards[PLAYER_HUMAN].skill_percentage = magic_staff_skill * 2;
    Wizard_Magic_Income(&mana, &research, &skill, 0);
    Font_Select_Shadow(2, 3, 0, 0);
    Set_Text_Shadow_Color(0x13);
    Set_Font_Color(0xb9);
    itoa(mana, p_text_buf, 10);
    strcat(p_text_buf, " MP");
    Print_Text_Right(54, 160, p_text_buf);
    if (wizards[current_player].research_spell == SPELL_NONE) {
        Print_Text(67, 160, "No Spell");
    } else {
        itoa(research, p_text_buf, 10);
        strcat(p_text_buf, " RP");
        Print_Text_Right(101, 160, p_text_buf);
    }
    itoa(skill, p_text_buf, 10);
    strcat(p_text_buf, " SP");
    Print_Text_Right(148, 160, p_text_buf);
    Font_Select_Shadow(1, 3, 0, 0);
    Set_Text_Shadow_Color(0xf0);
    Set_Font_Color(0xe4);
    Set_Space_Width(1);
    Print_Text(5, 177, "Casting Skill:");
    itoa(Wizard_Casting_Skill(current_player) + Fortress_Hero_Skill_Bonus(current_player), p_text_buf, 10);
    strcat(p_text_buf, "(");
    itoa(Wizard_Casting_Skill(current_player), p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, ")");
    Print_Text_Right(90, 177, p_text_buf);
    Print_Text(5, 185, "Magic Reserve:");
    if (wizards[PLAYER_HUMAN].mana_reserve > 19999) {   /* 20000 and more: the narrow font */
        Font_Select_Shadow(0, 3, 0, 0);
        Set_Text_Shadow_Color(0xf0);
        Set_Font_Color(0xe4);
        Set_Space_Width(1);
    }
    Print_Number_Right(90, 185, wizards[PLAYER_HUMAN].mana_reserve);
    Font_Select_Shadow(1, 3, 0, 0);
    Set_Text_Shadow_Color(0xf0);
    Set_Font_Color(0xe4);
    Set_Space_Width(1);
    Print_Text(5, 193, "Power Base:");
    Print_Number_Right(90, 193, wizards[PLAYER_HUMAN].power_base);
    Print_Text(100, 177, "Casting:");
    Print_Far_Text(156, 177, p_spell_data[wizards[current_player].casting_spell].name);
    Print_Text(100, 185, "Researching:");
    Print_Far_Text(156, 185, p_spell_data[wizards[current_player].research_spell].name);
    Print_Text(100, 193, "Summon To:");
    if (magic_summon_city == -1) {
        Print_Text(156, 193, "None");
    } else {
        Print_Far_Text(156, 193, p_cities[magic_summon_city].name);
    }
    Set_Text_Shadow_Color(0);
    for (i = 0; i < 4; i++) {
        Draw_Pic_Frame(i * 77 + 24, 4, magic_gem_frame_pic);
    }
    gem_x = 68;
    for (i = 0; i < magic_gem_count; i++) {
        y = 2;
        Font_Select(0, 3, 0, 0);
        Set_Font_Color(0xe4);
        switch (magic_gem_wizard[i]) {
        case -2:
            Draw_Pic_Frame(i * 77 + 23, 3, magic_overlay_pic);
            break;
        case -1:
            Draw_Pic_Frame(i * 77 + 24, 4, magic_gem_frame_pic);
            break;
        default:
            Draw_Pic_Frame(i * 77 + 24, 4, magic_gem_pics[i]);
            rel = (wizards[magic_gem_wizard[i]].relationship[current_player] + 100) / 20;
            Print_Text_Centered(i * 77 + 45, 53, relation_names[rel]);
            status = wizards[magic_gem_wizard[i]].diplomacy_status[current_player];
            if (status > 0) {
                if (status > 2) {   /* pact, alliance, war and worse: the three treaty icons */
                    status = 3;
                }
                status--;
                Draw_Pic_Frame(gem_x, y, treaty_icon_pics[wizards[current_player].banner_color * 3 + status]);
                help_entries[i + 43].id = HELP_TREATIES;
                y += 12;
            }
            for (j = 0; j < magic_gem_count; j++) {
                if (j != i && magic_gem_wizard[j] > -1) {
                    status = wizards[magic_gem_wizard[i]].diplomacy_status[magic_gem_wizard[j]];
                    if (status > 0) {
                        if (status > 2) {   /* pact, alliance, war and worse: the three treaty icons */
                            status = 3;
                        }
                        status--;
                        Draw_Pic_Frame(gem_x, y,
                                       treaty_icon_pics[wizards[magic_gem_wizard[j]].banner_color * 3 + status]);
                        help_entries[i + 43].id = HELP_TREATIES;
                        y += 12;
                    }
                }
            }
            if (wizards[current_player].globals.detect_magic != 0) {
                colors[0] = 0xb6;
                colors[1] = 0xb1;
                Font_Select_Palette(0, colors);
                Font_Select_Outline(0, 15, 0, 0);
                Set_Font_Color(0xb6);
                Set_Text_Shadow_Color(1);
                Print_Far_Text_Centered(i * 77 + 45, 4,
                                        p_spell_data[wizards[magic_gem_wizard[i]].casting_spell].name);
            }
            break;
        }
        gem_x += 77;
    }
    help_entries[9].id = HELP_ENCHANTMENTS;
    if (magic_enchant_count > 0) {
        help_entries[9].id = -1;
    }
    if (magic_enchant_count < 10) {             /* up to 9: one column */
        for (i = 0; i < magic_enchant_count; i++) {
            help_entries[i + 16].id = -1;
            colors[0] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
            colors[1] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
            colors[2] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
            colors[3] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
            colors[4] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
            colors[5] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
            Font_Select_Palette(2, colors);
            Font_Select_Shadow(2, 15, 0, 0);
            Set_Text_Shadow_Color(0x13);
            Set_Font_Color(0xb9);
            Print_Far_Text(171, i * 10 + 80, p_spell_data[p_magic_enchant_spell[i + magic_list_scroll]].name);
            help_entries[i + 16].id = p_magic_enchant_spell[i + magic_list_scroll];
        }
    } else {
        for (i = 0; i < MAGIC_LIST_ROWS; i++) {
            help_entries[i + 25].id = -1;
        }
        for (i = 0; i < magic_enchant_rows_shown; i++) {
            if (p_magic_enchant_spell[i + magic_list_scroll] > 0) {
                colors[0] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
                colors[1] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
                colors[2] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
                colors[3] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
                colors[4] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
                colors[5] = banner_colors[wizards[p_magic_enchant_owner[i + magic_list_scroll]].banner_color];
                Font_Select_Palette(0, colors);
                Font_Select_Shadow(0, 15, 0, 0);
                Set_Text_Shadow_Color(0x13);
                Set_Font_Color(0xb9);
                Print_Far_Text(i % 2 * 69 + 171, i / 2 * 9 + 81,
                               p_spell_data[p_magic_enchant_spell[i + magic_list_scroll]].name);
                help_entries[i + 25].id = p_magic_enchant_spell[i + magic_list_scroll];
            }
        }
    }
    Font_Select(0, 13, 0, 0);
    Set_Font_Color(0xb9);
    if (magic_list_has_more == 1) {
        Print_Text(286, 163, "More...");
    }
}

/* the magic screen's controls: the three staff sliders (0..50, 5 wide, 50 high), OK, Alchemy, the three lock areas, a gem
   per rival, the enchantment rows (one column of up to 9, else two of up to 18 from magic_list_scroll) and
   "More..." */
void MagicScr_Add_Controls(void)
{
    int i;
    int x;
    int y;

    UI_Add_Slider_Near(32, 102, 0, 50, 0, 50, 5, 50, &magic_staff_mana, "M", -1);
    UI_Add_Slider_Near(79, 102, 0, 50, 0, 50, 5, 50, &magic_staff_research, "R", -1);
    UI_Add_Slider_Near(126, 102, 0, 50, 0, 50, 5, 50, &magic_staff_skill, "S", -1);
    magic_ok_button = UI_Add_Button(291, 181, "", magic_ok_button_pic, "O", -1);
    magic_alchemy_button = UI_Add_Button(232, 181, "", magic_alchemy_button_pic, "A", -1);
    magic_lock_mana_control = UI_Add_Region(28, 82, 41, 101, "", -1);
    magic_lock_research_control = UI_Add_Region(75, 82, 88, 101, "", -1);
    magic_lock_skill_control = UI_Add_Region(122, 82, 135, 101, "", -1);
    for (i = 0; i < magic_gem_count; i++) {
        x = i * 77 + 29;
        y = 10;
        p_magic_gem_ctls[i] = UI_Add_Region(x, y, x + 30, y + 32, "", -1);
    }
    magic_enchant_rows_shown = 0;
    x = 169;
    y = 80;
    if (magic_enchant_count < 10) {             /* up to 9: one column */
        magic_enchant_rows_shown = magic_enchant_count;
        for (i = 0; i < magic_enchant_count; i++) {
            p_magic_enchant_ctls[i] = UI_Add_Region(x, i * 10 + y, x + 120, i * 10 + y + 9, "", -1);
        }
    } else {
        magic_enchant_rows_shown = magic_enchant_count - magic_list_scroll;
        if (magic_enchant_rows_shown > MAGIC_LIST_ROWS) {
            magic_enchant_rows_shown = MAGIC_LIST_ROWS;
        }
        for (i = 0; i < magic_enchant_rows_shown; i++) {
            p_magic_enchant_ctls[i] = UI_Add_Region(i % 2 * 70 + x, i / 2 * 9 + y, i % 2 * 70 + x + 63,
                                                     i / 2 * 9 + y + 6, "", -1);
        }
    }
    magic_list_more_control = UI_NO_CONTROL;
    if (magic_list_has_more == 1) {
        magic_list_more_control = UI_Add_Region(286, 163, 307, 169, "", -1);
    }
}

/* the global enchantments in play, all wizards: the spell (the one whose effect_param is the enchantment's index)
   and its caster */
void MagicScr_Build_Global_Ench_List(void)
{
    int i;
    int spell;
    int player;
    unsigned char *globals;

    magic_enchant_count = 0;
    for (player = 0; player < MAX_PLAYERS; player++) {
        globals = (unsigned char *)&wizards[player].globals;
        for (i = 0; i < NUM_GLOBAL_ENCHANTS; i++) {
            if ((int)globals[i] > 0) {
                for (spell = 0; spell < NUM_SPELLS; spell++) {
                    if ((int)p_spell_data[spell].effect_param == i && p_spell_data[spell].effect_type == SPELL_FX_GLOBAL_ENCH) {
                        p_magic_enchant_spell[magic_enchant_count] = spell;
                        break;
                    }
                }
                p_magic_enchant_owner[magic_enchant_count] = player;
                magic_enchant_count++;
            }
        }
    }
}

/* the magic screen's buffers (in the near pool) and pictures (MAGIC.LBX, a LILWIZ portrait per contacted rival) */
void MagicScr_Load(void)
{
    int i;

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(30);
    p_magic_enchant_spell = (int *)Near_Alloc(0xc0);
    p_magic_enchant_owner = Near_Alloc(0x60);
    p_magic_enchant_ctls = (int *)Near_Alloc(0x28);
    p_magic_gem_ctls = (int *)Near_Alloc(0xc);
    for (i = 0; i < 0x60; i++) {
        p_magic_enchant_spell[i] = 0;
        p_magic_enchant_owner[i] = PLAYER_NONE;
    }
    magicscr_bg_pic = LBX_Reload("MAGIC", 0, p_scratch);
    magic_alchemy_button_pic = LBX_Reload_Next("MAGIC", 13, p_scratch);
    magic_ok_button_pic = LBX_Reload_Next("MAGIC", 14, p_scratch);
    for (i = 0; i < magic_gem_count; i++) {
        magic_gem_pics[i] = 0;
        if (magic_gem_wizard[i] > -1) {
            magic_gem_pics[i] = LBX_Reload_Next("LILWIZ", LILWIZ_ENTRY(wizards[magic_gem_wizard[i]].portrait,
                                                wizards[magic_gem_wizard[i]].banner_color), p_scratch);   /* by gem slot here */
        }
    }
    magic_gem_frame_pic = LBX_Reload_Next("MAGIC", 6, p_scratch);
    magic_overlay_pic = LBX_Reload_Next("MAGIC", 51, p_scratch);
    staff_mana_empty_pic = LBX_Reload_Next("MAGIC", 7, p_scratch);
    staff_mana_full_pic = LBX_Reload_Next("MAGIC", 8, p_scratch);
    staff_research_empty_pic = LBX_Reload_Next("MAGIC", 9, p_scratch);
    staff_research_full_pic = LBX_Reload_Next("MAGIC", 10, p_scratch);
    staff_skill_empty_pic = LBX_Reload_Next("MAGIC", 11, p_scratch);
    staff_skill_full_pic = LBX_Reload_Next("MAGIC", 12, p_scratch);
    staff_mana_locked_pic = LBX_Reload_Next("MAGIC", 15, p_scratch);
    staff_research_locked_pic = LBX_Reload_Next("MAGIC", 16, p_scratch);
    staff_skill_locked_pic = LBX_Reload_Next("MAGIC", 17, p_scratch);
    for (i = 0; i < 15; i++) {
        treaty_icon_pics[i] = LBX_Reload_Next("MAGIC", i + 60, p_scratch);
    }
}

/* (x, y): the Alchemy box until OK, Cancel, Escape or a click outside it */
void Alchemy_Dialog(int x, int y)
{
    int esc;
    int input;
    int done;

    alchemy_box_x = x;
    alchemy_box_y = y;
    alchemy_anim_frame = 0;
    alchemy_amount = 0;
    alchemy_slider_pos = 3;
    UI_Clear_Help();
    UI_Set_Redraw_Fn(Alchemy_Draw, 1);
    Help_Load_Alchemy();
    alchemy_arrow_pic = Heap_Alloc_Front(p_scratch, 40);
    alchemy_bg_pic = LBX_Reload_Next("MAGIC", 52, p_scratch);
    alchemy_cancel_pic = LBX_Reload_Next("MAGIC", 53, p_scratch);
    alchemy_ok_pic = LBX_Reload_Next("MAGIC", 54, p_scratch);
    alchemy_left_arrow_pic = LBX_Reload_Next("MAGIC", 56, p_scratch);
    alchemy_right_arrow_pic = LBX_Reload_Next("MAGIC", 55, p_scratch);
    alchemy_arrow_bar_pic = LBX_Reload_Next("MAGIC", 57, p_scratch);
    alchemy_star_pic = LBX_Reload_Next("MAGIC", 58, p_scratch);
    alchemy_pow2gold_pic = LBX_Reload_Next("MAGIC", 59, p_scratch);
    alchemy_ratio = 2;
    if (wizards[current_player].retorts[RETORT_ALCHEMY] == 1) {
        alchemy_ratio = 1;
    }
    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Save_A800();
    Screen_Present();
    UI_Input_Delay(1);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        Alchemy_Add_Controls();
        esc = UI_Add_Hotkey("\x1b");
        input = UI_Get_Input();
        if (input == alchemy_cancel_button || input == alchemy_outside_control || input == esc) {
            Play_Click_Sound();
            done = 1;
        }
        if (input == alchemy_ok_button) {
            Play_Click_Sound();
            done = 1;
            if (alchemy_gold_to_mana == 1) {
                wizards[PLAYER_HUMAN].gold = wizards[PLAYER_HUMAN].gold - alchemy_amount;
                Wizard_Add_Mana(current_player, alchemy_amount / alchemy_ratio);
            } else {
                Wizard_Add_Gold(current_player, alchemy_amount / alchemy_ratio);
                wizards[PLAYER_HUMAN].mana_reserve = wizards[PLAYER_HUMAN].mana_reserve - alchemy_amount;
            }
        }
        if (input == alchemy_direction_button) {
            Play_Click_Sound();
            alchemy_gold_to_mana = !alchemy_gold_to_mana;
        }
        if (done == 0) {
            Screen_Restore_A800();
            Alchemy_Draw();
            Screen_Present();
            Wait_Ticks(2);
        }
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
}

/* redraw callback: the box, "<n> GP" and "<n> PP" for the slider's share (the whole treasury or reserve at the
   right end, capped at 999), the moving arrows and the slider's star */
void Alchemy_Draw(void)
{
    char colors[6];
    long scale;

    colors[0] = 0xc0;
    colors[1] = 0xb3;
    Set_Draw_Page();
    if (alchemy_gold_to_mana == 1) {
        alchemy_anim_frame = (alchemy_anim_frame + 1) % 8;
    } else {
        alchemy_anim_frame = (alchemy_anim_frame + 7) % 8;   /* one frame back */
    }
    Draw_Pic_Frame(alchemy_box_x - 5, alchemy_box_y, alchemy_bg_pic);
    Font_Select_Palette(1, colors);
    Set_Text_Shadow_Color(0);
    Font_Select_Shadow(3, 15, 0, 0);
    if (alchemy_gold_to_mana == 1) {
        if (wizards[PLAYER_HUMAN].gold <= 1) {
            Font_Select_Shadow(4, 2, 0, 0);
            alchemy_slider_pos = 3;
        }
    } else {
        Draw_Pic_Frame(alchemy_box_x + 7, alchemy_box_y + 10, alchemy_pow2gold_pic);
        if (wizards[PLAYER_HUMAN].mana_reserve <= 1) {
            Font_Select_Shadow(4, 2, 0, 0);
            alchemy_slider_pos = 3;
        }
    }
    Font_Select(1, 15, 0, 0);
    if (alchemy_gold_to_mana == 1) {
        scale = 50;
        if ((int)((long)wizards[PLAYER_HUMAN].gold * 50 / scale) > 999) {
            scale = (int)((long)wizards[PLAYER_HUMAN].gold * 50 / 999L);
        }
        alchemy_amount = ((long)alchemy_slider_pos - 3) * wizards[PLAYER_HUMAN].gold / scale;
        if (alchemy_amount > 999) {
            alchemy_amount = 999;
        }
        Print_Number_Right(alchemy_box_x + 28, alchemy_box_y + 26, alchemy_amount);
        Print_Text_Right(alchemy_box_x + 40, alchemy_box_y + 26, "GP");
        Print_Number_Right(alchemy_box_x + 130, alchemy_box_y + 26, alchemy_amount / alchemy_ratio);
    } else {
        scale = 50;
        if ((int)((long)wizards[PLAYER_HUMAN].mana_reserve * 50 / scale) > 999) {
            scale = (int)((long)wizards[PLAYER_HUMAN].mana_reserve * 50 / 999L);
        }
        alchemy_amount = ((long)alchemy_slider_pos - 3) * wizards[PLAYER_HUMAN].mana_reserve / scale;
        if (alchemy_amount > 999) {
            alchemy_amount = 999;
        }
        Print_Number_Right(alchemy_box_x + 28, alchemy_box_y + 26, alchemy_amount / alchemy_ratio);
        Print_Text_Right(alchemy_box_x + 40, alchemy_box_y + 26, "GP");
        Print_Number_Right(alchemy_box_x + 130, alchemy_box_y + 26, alchemy_amount);
    }
    Print_Text_Right(alchemy_box_x + 142, alchemy_box_y + 26, "PP");
    Pic_Decode_Frame_To_Bitmap(alchemy_arrow_bar_pic, alchemy_arrow_pic);
    if (alchemy_gold_to_mana == 0) {
        Pic_Mirror(alchemy_arrow_pic);
    }
    Set_Clip_Rect(alchemy_box_x + 50, 0, alchemy_box_x + alchemy_slider_pos + 50, 199);
    Draw_Bitmap_Clipped(alchemy_box_x + alchemy_anim_frame + 50, alchemy_box_y + 25, alchemy_arrow_pic);
    Draw_Bitmap_Clipped(alchemy_box_x + alchemy_anim_frame + 2, alchemy_box_y + 25, alchemy_arrow_pic);
    Reset_Clip_Rect();
    Draw_Pic_Frame(alchemy_box_x + alchemy_slider_pos + 47, alchemy_box_y + 25, alchemy_star_pic);
}

/* the Alchemy box's controls: the slider (key A), Cancel (C), OK (O), the direction button (space; its arrow
   shows the direction), the box itself and the whole screen (a click outside closes the box) */
void Alchemy_Add_Controls(void)
{
    UI_Add_Slider_Near(alchemy_box_x + 50, alchemy_box_y + 25, 0, 55, 3, 53, 55, 7, &alchemy_slider_pos, "A", -1);
    alchemy_cancel_button = UI_Add_Button(alchemy_box_x + 13, alchemy_box_y + 39, "", alchemy_cancel_pic, "C", -1);
    alchemy_ok_button = UI_Add_Button(alchemy_box_x + 96, alchemy_box_y + 39, "", alchemy_ok_pic, "O", -1);
    if (alchemy_gold_to_mana == 1) {
        alchemy_direction_button = UI_Add_Button(alchemy_box_x + 66, alchemy_box_y + 39, "", alchemy_left_arrow_pic,
                                                 " ", -1);
    } else {
        alchemy_direction_button = UI_Add_Button(alchemy_box_x + 66, alchemy_box_y + 39, "", alchemy_right_arrow_pic,
                                                 " ", -1);
    }
    alchemy_box_control = UI_Add_Region(alchemy_box_x, alchemy_box_y, alchemy_box_x + 157, alchemy_box_y + 65, "", -1);
    alchemy_outside_control = UI_Add_Region(0, 0, 319, 199, "", -1);
}
