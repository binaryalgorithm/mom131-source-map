/* WIZINFO.C - WIZARDS.EXE overlay code segment 62a9 (stub 2479): the wizard info box (a portrait clicked on the magic
   screen): portrait gem and name; for another wizard the relation, treaty, personality and objective lines with
   help areas; gold and mana; one book icon per spellbook; fame; the retorts as a wrapped "a, b and c." list with
   a help area each; and the hired heroes ("<name> The <unit type>").
   This module's data (DS:35a4): relation_names, treaty_names, their strings, then the literals ("" is the tail
   of "Hate": -d); BSS (DS:c254..c25e).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 62a9 src/wizards/WIZINFO.C */
#include <string.h>
#include "GAME.H"

char *relation_names[11] = { "Hate", "Troubled", "Tense", "Restless", "Unease", "Neutral", "Relaxed", "Calm",
                             "Peaceful", "Friendly", "Harmony" };              /* DS:35a4  (relation + 100) / 20 */
char *treaty_names[5] = { "No Treaty", "Wizard Pact", "Alliance", "War", "Final War" };   /* DS:35ba */

int wizinfo_icon_count;                         /* DS:c25c  book icons in p_wizinfo_icon_pics */
int wizinfo_box_x;                              /* DS:c25a */
int wizinfo_box_y;                              /* DS:c258 */
unsigned *p_wizinfo_icon_pics;                  /* DS:c256  one book picture per spellbook */
int wizinfo_wizard;                             /* DS:c254 */

extern char *retort_names[18];                  /* DS:0130  "Alchemy" .. "Artificer" */
extern char *personality_names[];               /* DS:1fbc */
extern char *objective_names[];                 /* DS:1fd4 */
extern unsigned wizinfo_book_pics[18];          /* DS:954a  MAGIC.LBX 33..50: 3 per realm */
extern unsigned wizinfo_bg_pic;                 /* DS:956e */
extern unsigned magic_gem_pics[];               /* DS:958e */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern unsigned treaty_icon_pics[15];           /* DS:c1ea  MAGIC.LBX 60..74 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
void Screen_Wizard_Info(int wizard, int from_x1, int from_y1, int from_x2, int from_y2);
void Wizard_Info_Draw(void);
void Wizard_Info_Load(void);

void Screen_Copy_Front(void);
void Play_Click_Sound(void);
void Wizard_Power_Base(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
void Help_Load_Wizard_Info(void);
void Window_Zoom_Open_In_Place(int x1, int y1, int x2, int y2, int src_x1, int src_y1, int src_x2, int src_y2, int lines);
int Window_Zoom_Lines_Redraw(void);
int Wizard_Effective_Fame(int player);
void Set_Space_Width(int width);
int Text_Width(char *text);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);

/* (wizard, from x1, y1, x2, y2): the info box of a wizard (zooming open from the portrait for another wizard)
   until a click */
void Screen_Wizard_Info(int wizard, int from_x1, int from_y1, int from_x2, int from_y2)
{
    int input;
    int control;
    int done;

    wizinfo_wizard = wizard;
    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Save_A800();
    Screen_Present();
    Wizard_Info_Load();
    UI_Clear_Help();
    Wizard_Power_Base();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Wizard_Info_Draw, 1);
    Help_Load_Wizard_Info();
    wizinfo_box_x = 75;
    wizinfo_box_y = 2;
    if (wizinfo_wizard != current_player) {
        Set_Draw_Page();
        Wizard_Info_Draw();
        Window_Zoom_Open_In_Place(wizinfo_box_x, wizinfo_box_y, wizinfo_box_x + 179, wizinfo_box_y + 195, from_x1,
                              from_y1, from_x2, from_y2, 1);
    }
    UI_Input_Delay(1);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control) {
            Play_Click_Sound();
            done = -1;
        }
        if (done == 0) {
            Screen_Restore_A800();
            Wizard_Info_Draw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Clear_Help();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    present_mode = PRESENT_DISSOLVE;
}

/* redraw callback of the info box */
void Wizard_Info_Draw(void)
{
    int k;
    int unused;
    int icon_x;
    int unit;
    char *retort_flags;
    int fame;
    char colors[6];
    int relation;
    int y;
    int width;
    int x0;
    char space[4];
    char retorts[18];
    int help_ids[18];
    register int n;
    register int x;

    strcpy(space, " ");
    help_ids[0] = 693;                          /* HELP.LBX ids of the 18 retorts, by RETORT_* */
    help_ids[1] = 694;
    help_ids[2] = 705;
    help_ids[3] = 706;
    help_ids[4] = 707;
    help_ids[5] = 708;
    help_ids[6] = 701;
    help_ids[7] = 699;
    help_ids[8] = 695;
    help_ids[9] = 700;
    help_ids[10] = 696;
    help_ids[11] = 709;
    help_ids[12] = 710;
    help_ids[13] = 702;
    help_ids[14] = 703;
    help_ids[15] = 698;
    help_ids[16] = 704;
    help_ids[17] = 697;
    if (wizinfo_wizard != current_player)
        Window_Zoom_Lines_Redraw();
    Set_Draw_Page();
    Draw_Pic_Frame(wizinfo_box_x, wizinfo_box_y, wizinfo_bg_pic);
    Draw_Pic_Frame(wizinfo_box_x + 11, wizinfo_box_y + 11, magic_gem_pics[0]);
    Set_Text_Shadow_Color(234);
    colors[0] = 150;
    for (k = 1; k < 6; k++)
        colors[k] = k + 177;
    Font_Select_Palette(4, colors);
    Font_Select_Shadow(4, 15, 0, 0);
    Print_Text_Centered(wizinfo_box_x + 114, wizinfo_box_y + 11, wizards[wizinfo_wizard].wizard_name);
    help_entries[14].id = -1;                   /* help slots: 7 fame, 8-13 retorts, 14-17 the relation lines (BUG #488: the
                                                   retorts go to k + 8 later, so a 7th+ retort overwrites them) */
    help_entries[15].id = -1;
    help_entries[16].id = -1;
    help_entries[17].id = -1;
    if (wizinfo_wizard != current_player) {
        relation = (wizards[wizinfo_wizard].relationship[current_player] + 100) / 20;
        Font_Select_Shadow(1, 15, 0, 0);
        strcpy(p_text_buf, "Relations:");
        strcat(p_text_buf, space);
        strcat(p_text_buf, relation_names[relation]);
        Print_Text(wizinfo_box_x + 59, wizinfo_box_y + 25, p_text_buf);
        help_entries[14].id = HELP_RELATIONS;
        help_entries[14].x1 = wizinfo_box_x + 59;
        help_entries[14].y1 = wizinfo_box_y + 25;
        help_entries[14].x2 = wizinfo_box_x + 159;
        help_entries[14].y2 = wizinfo_box_y + 32;
        strcpy(p_text_buf, "Treaties:");
        strcat(p_text_buf, space);
        strcat(p_text_buf, treaty_names[wizards[wizinfo_wizard].diplomacy_status[current_player]]);
        Print_Text(wizinfo_box_x + 59, wizinfo_box_y + 33, p_text_buf);
        help_entries[15].id = -1;
        if (wizards[wizinfo_wizard].diplomacy_status[current_player] == TREATY_WIZARD_PACT)
            help_entries[15].id = HELP_WIZARD_PACT;
        if (wizards[wizinfo_wizard].diplomacy_status[current_player] == TREATY_ALLIANCE)
            help_entries[15].id = HELP_ALLIANCE;
        help_entries[15].x1 = wizinfo_box_x + 59;
        help_entries[15].y1 = wizinfo_box_y + 33;
        help_entries[15].x2 = wizinfo_box_x + 159;
        help_entries[15].y2 = wizinfo_box_y + 40;
        strcpy(p_text_buf, "Personality:");
        strcat(p_text_buf, space);
        strcat(p_text_buf, personality_names[wizards[wizinfo_wizard].personality]);
        Print_Text(wizinfo_box_x + 59, wizinfo_box_y + 41, p_text_buf);
        help_entries[16].id = wizards[wizinfo_wizard].personality + HELP_PERSONALITY;
        help_entries[16].x1 = wizinfo_box_x + 59;
        help_entries[16].y1 = wizinfo_box_y + 41;
        help_entries[16].x2 = wizinfo_box_x + 159;
        help_entries[16].y2 = wizinfo_box_y + 48;
        strcpy(p_text_buf, "Objective:");
        strcat(p_text_buf, space);
        strcat(p_text_buf, objective_names[wizards[wizinfo_wizard].objective]);
        Print_Text(wizinfo_box_x + 59, wizinfo_box_y + 49, p_text_buf);
        help_entries[17].id = wizards[wizinfo_wizard].objective + HELP_OBJECTIVE;
        help_entries[17].x1 = wizinfo_box_x + 59;
        help_entries[17].y1 = wizinfo_box_y + 49;
        help_entries[17].x2 = wizinfo_box_x + 159;
        help_entries[17].y2 = wizinfo_box_y + 56;
    }
    Font_Select_Shadow(1, 15, 0, 0);
    Print_Number_Right(wizinfo_box_x + 31, wizinfo_box_y + 75, wizards[wizinfo_wizard].gold);
    Print_Number_Right(wizinfo_box_x + 159, wizinfo_box_y + 75, wizards[wizinfo_wizard].mana_reserve);
    Font_Select_Shadow(0, 15, 0, 0);
    Print_Text(wizinfo_box_x + 34, wizinfo_box_y + 75, "GP");
    Print_Text(wizinfo_box_x + 162, wizinfo_box_y + 75, "MP");
    k = wizinfo_icon_count * 8 / 2;
    icon_x = wizinfo_box_x + (90 - k);
    for (k = 0; k < wizinfo_icon_count; k++)
        Draw_Pic_Frame(icon_x + k * 8, wizinfo_box_y + 66, p_wizinfo_icon_pics[k]);
    help_entries[7].id = -1;
    fame = Wizard_Effective_Fame(wizinfo_wizard);
    if (fame > 0) {
        help_entries[7].id = HELP_FAMOUS;
        Font_Select_Shadow(2, 15, 0, 0);
        Print_Number_Right(wizinfo_box_x + 79, wizinfo_box_y + 96, fame);
        Print_Text(wizinfo_box_x + 85, wizinfo_box_y + 96, "Fame");
    }
    help_entries[8].id = -1;
    help_entries[9].id = -1;
    help_entries[10].id = -1;
    help_entries[11].id = -1;
    help_entries[12].id = -1;
    help_entries[13].id = -1;
    strcpy(p_text_buf, "");
    n = 0;
    retort_flags = wizards[wizinfo_wizard].retorts;
    for (k = 0; k < 18; k++) {
        if (retort_flags[k] == 1) {
            retorts[n] = k;
            n++;
        }
    }
    if (n > 0) {
        Set_Text_Shadow_Color(234);
        Font_Select_Shadow(1, 15, 0, 0);
        Set_Space_Width(1);
        strcpy(p_text_buf, "");
        x0 = wizinfo_box_x + 13;
        x = x0;
        y = wizinfo_box_y + 111;
        if (n > 5)
            y = y - 2;
        unused = 0;
        for (k = 0; k < n; k++) {
            strcpy(p_text_buf, retort_names[retorts[k]]);
            if (k + 1 != n) {
                if (k + 2 == n) {
                    strcat(p_text_buf, space);
                    strcat(p_text_buf, "and");
                } else
                    strcat(p_text_buf, ",");
                strcat(p_text_buf, space);
            }
            width = Text_Width(p_text_buf);
            if (x + width > x0 + 155) {
                x = x0;
                y += 7;
            }
            Print_Text(x, y, p_text_buf);
            help_entries[k + 8].x1 = x;
            help_entries[k + 8].y1 = y;
            help_entries[k + 8].x2 = x + width;
            help_entries[k + 8].y2 = y + 5;
            help_entries[k + 8].id = help_ids[retorts[k]];
            x = x + width + 1;
            strcpy(p_text_buf, "");
        }
        Print_Text(x, y, ".");
    }
    Font_Select_Shadow(2, 15, 0, 0);
    Print_Text(wizinfo_box_x + 74, wizinfo_box_y + 131, "Heroes");
    Font_Select_Shadow(1, 15, 0, 0);
    n = 0;
    for (k = 0; k < 6; k++) {
        unit = wizards[wizinfo_wizard].hired_heroes[k].unit_index;
        if (unit > -1 && p_units[unit].owner != PLAYER_NONE) {
            strcpy(p_text_buf, wizards[wizinfo_wizard].hired_heroes[k].name);
            strcat(p_text_buf, space);
            strcat(p_text_buf, "The");
            strcat(p_text_buf, space);
            strcat(p_text_buf, unit_types[p_units[unit].type].name);
            Print_Text(wizinfo_box_x + 13, wizinfo_box_y + n * 8 + 142, p_text_buf);
            n++;
        }
    }
}

/* loads the box's pictures (the portrait gem, MAGIC.LBX 33..50 book icons, 60..74 treaty icons) and builds the icon
   row: per realm one icon per spellbook, cycling through the realm's 3 pictures */
void Wizard_Info_Load(void)
{
    int k;
    int base;
    int i;
    int realm;

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(20);
    p_wizinfo_icon_pics = (unsigned *)Near_Alloc(40);
    magic_gem_pics[0] = LBX_Reload("LILWIZ", LILWIZ_ENTRY(wizards[wizinfo_wizard].portrait,
                                   wizards[wizinfo_wizard].banner_color), p_scratch);
    for (i = 0; i < 18; i++)
        wizinfo_book_pics[i] = LBX_Reload_Next("MAGIC", i + 33, p_scratch);
    for (i = 0; i < 15; i++)
        treaty_icon_pics[i] = LBX_Reload_Next("MAGIC", i + 60, p_scratch);
    wizinfo_icon_count = 0;
    for (realm = 0; realm < 5; realm++) {
        base = realm * 3;
        k = 0;
        for (i = 0; i < wizards[wizinfo_wizard].spellbooks[realm]; i++) {
            p_wizinfo_icon_pics[wizinfo_icon_count] = wizinfo_book_pics[base + k];
            k = (k + 1) % 3;
            wizinfo_icon_count++;
        }
    }
}
