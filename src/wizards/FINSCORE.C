/* FINSCORE.C - WIZARDS.EXE overlay code segment 86ea (stub 2527): the final score screen at the end of a game
   (HALOFAM.LBX) and the entry into the Hall of Fame (10 scores kept in MAGIC.SET), then the Hall of Fame list.
   Score = known spells + population / 2 (thousands, own cities) + 50 per banished wizard + 2 x effective fame
   + (2000 - 2 x turn, when the fortress stands and nobody else cast Mastery) + 250 for the Spell of Mastery,
   scaled by difficulty (Intro 1/2, Easy 3/4, Normal 1, Hard 2, Impossible 3). BUG #109: the turn bonus has no
   floor - after turn 1000 it subtracts.
   This module's data (DS:5a7e): its literals ("+" is the tail of ": +": -d); BSS (DS:c7e8..c7fc).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 86ea src/wizards/FINSCORE.C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Final_Score_Redraw(void);
void Screen_Final_Score(void);

#include "GAME.H"

unsigned final_score_city_pic;                  /* DS:c7fa  HALOFAM 15 + home race: the town icon */
unsigned final_score_book_pic;                  /* DS:c7f8  HALOFAM 30: the spellbook icon */
unsigned final_score_hourglass_pic;             /* DS:c7f6  HALOFAM 29 */
unsigned final_score_rival_pics[4];             /* DS:c7ee  the portraits of the banished wizards (HALOFAM 1 + portrait) */
unsigned final_score_fame_pic;                  /* DS:c7ec  HALOFAM 31 */
unsigned final_score_master_pic;                /* DS:c7ea  HALOFAM 32 */
int final_score;                                /* DS:c7e8 */

extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern int mastery_winner;                      /* DS:ca56  the player who cast the Spell of Mastery, -1 none */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
int Sound_Stop(void);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
void Screen_Hall_Of_Fame_List(void);
void Help_Load_Hall_Of_Fame(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Bit_Set_Near(int bit, char *map);
int Bit_Test_Near(int bit, char *map);
void Set_Font_Color(int color);
void Fill_Box_Styled();                           /* no prototype here: called with 5 and 6 arguments (GRAPHICS.C takes 9, read by style) */

/* the final score screen until a click; a score that makes the top 10 is entered into the Hall of Fame and MAGIC.SET
   is written; then the Hall of Fame list. After a Spell of Mastery win every rival whose fortress still stands
   counts as banished */
void Screen_Final_Score(void)
{
    int done;
    int input;
    int control;
    int k;
    FILE *fp;
    int i;
    int j;

    final_score = 0;
    Sound_Stop();
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Palette_Fade_To_Black();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Set_Draw_Page();
    i = LBX_Reload("HALOFAM", 0, p_scratch);
    Draw_Pic_Frame(0, 0, i);
    Palette_Upload();
    Palette_Unmark_Range(0, 0xff);
    Palette_Mark_Range(0, 0xff);
    Remap_Build_All();
    Screen_Save_A800();
    Screen_Save_AC00();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    final_score_hourglass_pic = LBX_Reload("HALOFAM", 29, p_scratch);
    final_score_book_pic = LBX_Reload_Next("HALOFAM", 30, p_scratch);
    final_score_fame_pic = LBX_Reload_Next("HALOFAM", 31, p_scratch);
    final_score_master_pic = LBX_Reload_Next("HALOFAM", 32, p_scratch);
    final_score_city_pic = LBX_Reload_Next("HALOFAM", wizards[current_player].home_race + 15, p_scratch);
    UI_Set_Redraw_Fn(Final_Score_Redraw, 2);
    if (mastery_winner == current_player)
        for (i = 1; i < num_players; i++)
            if (p_fortresses[i].active == 1)
                Bit_Set_Near(i, wizards[current_player].defeated_wizards);
    k = 0;
    for (i = 0; i < num_players; i++) {
        if (Bit_Test_Near(i, wizards[current_player].defeated_wizards) == 1) {
            final_score_rival_pics[k] = LBX_Reload_Next("HALOFAM", wizards[i].portrait + 1, p_scratch);
            k++;
        }
    }
    UI_Clear_Help();
    Help_Load_Hall_Of_Fame();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (done == 0) {
            Final_Score_Redraw();
            Screen_Present();
            Wait_Ticks(2);
        }
    }
    Sound_Stop();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    for (i = 0; i < 10; i++) {
        if (game_config.hall_of_fame_scores[i] < final_score) {
            for (j = 8; j >= i; j--) {
                strcpy(game_config.hall_of_fame_names[j + 1], game_config.hall_of_fame_names[j]);
                game_config.hall_of_fame_scores[j + 1] = game_config.hall_of_fame_scores[j];
                game_config.hall_of_fame_race[j + 1] = game_config.hall_of_fame_race[j];
            }
            game_config.hall_of_fame_scores[i] = final_score;
            game_config.hall_of_fame_race[i] = wizards[current_player].home_race;
            strcpy(game_config.hall_of_fame_names[i], wizards[current_player].wizard_name);
            fp = fopen("MAGIC.SET", "wb");
            fwrite(&game_config, sizeof(game_config), 1, fp);
            fclose(fp);
            break;
        }
    }
    Screen_Hall_Of_Fame_List();
    Palette_Fade_To_Black();
    Palette_Load(0, -1);
    Palette_Cycle_Reset();
    Palette_Unmark_Range(0, 0xff);
    Palette_Mark_Range(0, 0xdf);
    Remap_Build_All();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Palette_Upload();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
}

/* redraw callback of the final score screen: computes and prints every part of the score with its icons, the
   total and its bar (40 points per pixel, at most 200); keeps the total in final_score */
void Final_Score_Redraw(void)
{
    int i;
    int realm;
    int ix;
    int y;
    int defeated;                               /* the defeated_wizards bits: banished, eliminated or struck by Mastery */
    int x0;
    char unused_colors[4];
    char sep[6];
    char space[2];
    int value;
    int total;

    strcpy(sep, ": +");
    strcpy(space, " ");
    for (i = 0; i < 4; i++)
        unused_colors[i] = i + 117;
    unused_colors[0] = 240;
    x0 = 47;
    Set_Draw_Page();
    Reset_Clip_Rect();
    Screen_Restore_A800();
    Font_Select_Shadow(3, 4, 0, 0);
    Set_Text_Shadow_Color(250);
    Set_Font_Color(246);
    Print_Text_Centered(160, 3, wizards[current_player].wizard_name);
    total = 0;
    value = 0;
    for (realm = 0; realm < NUM_REALMS; realm++)
        for (i = 0; i < SPELLS_PER_REALM; i++)
            if (wizards[current_player].spell_library[realm][i] == SPELLSTATE_KNOWN)
                value++;
    total += value;
    Font_Select_Shadow(2, 4, 0, 0);
    Set_Text_Shadow_Color(250);
    Set_Font_Color(246);
    itoa(value, p_text_buf2, 10);
    strcpy(p_text_buf, p_text_buf2);
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Spells");
    strcat(p_text_buf, sep);
    strcat(p_text_buf, p_text_buf2);
    Print_Text(x0, 15, p_text_buf);
    ix = x0;
    y = 25;
    for (i = 0; i < value && i < 50; i++) {
        Draw_Pic_Frame(ix, y, final_score_book_pic);
        ix += 8;
        if (i == 24) {
            ix = x0 + 4;
            y += 3;
        }
    }
    value = 0;
    for (i = 0; i < num_cities; i++)
        if (p_cities[i].owner == current_player)
            value = value + p_cities[i].population;
    itoa(value, p_text_buf2, 10);
    strcpy(p_text_buf, p_text_buf2);
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Townsfolk");
    strcat(p_text_buf, sep);
    value = value / 2;
    itoa(value, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    Print_Text(x0, 37, p_text_buf);
    total += value;
    ix = x0;
    y = 46;
    for (i = 0; i < value && i < 100; i++) {
        Draw_Pic_Frame(ix, y, final_score_city_pic);
        ix += 9;
        if (i > 0 && (i + 1) % 25 == 0) {
            ix = ((i + 1) / 25 == 1 || (i + 1) / 25 == 3) ? x0 + 4 : x0;
            y += 5;
        }
    }
    value = 0;
    defeated = 0;
    for (i = 0; i < num_players; i++)
        if (Bit_Test_Near(i, wizards[current_player].defeated_wizards) == 1)
            defeated++;
    value = defeated;
    if (value == 0)
        strcpy(p_text_buf, "No");
    else {
        itoa(value, p_text_buf2, 10);
        strcpy(p_text_buf, p_text_buf2);
    }
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Banished");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Wizards");
    strcat(p_text_buf, sep);
    value = value * 50;
    itoa(value, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    Print_Text(x0, 78, p_text_buf);
    total += value;
    ix = x0;
    y = 87;
    for (i = 0; i < defeated; i++) {
        Draw_Pic_Frame(ix, y, final_score_rival_pics[i]);
        ix += 35;
    }
    value = Wizard_Effective_Fame(current_player);
    itoa(value, p_text_buf2, 10);
    strcpy(p_text_buf, p_text_buf2);
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Fame");
    strcat(p_text_buf, sep);
    value = value * 2;
    itoa(value, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    Draw_Pic_Frame(x0, 123, final_score_fame_pic);
    Print_Text(x0 + 18, 128, p_text_buf);
    total += value;
    value = game_turn / TURNS_PER_YEAR + START_YEAR;
    strcpy(p_text_buf, "Year");
    strcat(p_text_buf, space);
    itoa(value, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, ":");
    strcat(p_text_buf, space);
    value = 0;
    if (p_fortresses[current_player].active == 1 && (mastery_winner == PLAYER_NONE || mastery_winner == current_player))
        value = 2000 - game_turn * 2;
    if (value >= 0)
        strcat(p_text_buf, "+");
    else
        ;
    itoa(value, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    Draw_Pic_Frame(x0, 142, final_score_hourglass_pic);
    Print_Text(x0 + 18, 147, p_text_buf);
    total = total + value;
    strcpy(p_text_buf, "Spell");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Of");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Mastery");
    strcat(p_text_buf, sep);
    value = 0;
    if (mastery_winner == current_player)
        value = 250;
    itoa(value, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    Draw_Pic_Frame(x0, 161, final_score_master_pic);
    Print_Text(x0 + 18, 166, p_text_buf);
    total += value;
    switch (difficulty) {
    case DIFFICULTY_INTRO:
        total = total / 2;
        break;
    case DIFFICULTY_EASY:
        total = total * 3 / 4;
        break;
    case DIFFICULTY_NORMAL:
        break;
    case DIFFICULTY_HARD:
        total = total * 2;
        break;
    case DIFFICULTY_IMPOSSIBLE:
        total = total * 3;
        break;
    }
    strcpy(p_text_buf, "Final Score:");
    strcat(p_text_buf, space);
    itoa(total, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    Print_Text(x0, 180, p_text_buf);
    value = total / 40;
    if (value > 200)
        value = 200;
    Fill_Box_Styled(47, 189, 251, 189, FILL_SHADOW);
    Fill_Box_Styled(47, 190, 47, 197, FILL_SHADOW);
    Fill_Box_Styled(48, 197, 251, 197, FILL_REMAP, 1);
    Fill_Box_Styled(251, 190, 251, 196, FILL_REMAP, 1);
    Fill_Rect(48, 190, 250, 196, 0);
    Fill_Rect(49, 191, value + 49, 195, 77);
    final_score = total;
}
