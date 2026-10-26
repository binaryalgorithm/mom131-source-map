/* HALLFAME.C - WIZARDS.EXE code segment 86a6 (Hall of Fame list screen).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 86a6 src/wizards/HALLFAME.C */
#include <string.h>
#include <stdlib.h>
#include "STRUCTS.H"

extern struct MouseRegion mouse_region_hidden;  /* DS:00fa */
extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */
extern unsigned p_scratch;                      /* DS:998e  segment of the scratch buffer */
extern char *p_text_buf;                        /* DS:bfbc */
extern char *p_text_buf2;                       /* DS:bfba */
extern struct GameConfig game_config;           /* DS:bd9e  MAGIC.SET, read / written whole */

/* the module's first data (link test, batch 36): the table and its strings open its block in both programs */
char *race_plural_names[14] = {                 /* DS:59b4 */
    "Barbarians", "Beastmen", "Dark Elves", "Draconians", "Dwarves", "Gnolls", "Halflings", "High Elves",
    "High Men", "Klackons", "Lizardmen", "Nomads", "Orcs", "Trolls"
};

void UI_Suspend_Redraw_Fn(void);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
void UI_Clear_Controls(void);
void Palette_Fade_To_Black(void);
void Set_Draw_Page(void);
void Fill_Rect(int x1, int y1, int x2, int y2, int color);
void UI_Present_Frame(void);
unsigned LBX_Reload(char *name, int entry, unsigned block);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Palette_Upload(void);
void Screen_Save_A800(void);
void Screen_Restore_A800(void);
void Screen_Save_AC00(void);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
void UI_Clear_Help(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Timer_Mark(void);
int UI_Add_Region(int x1, int y1, int x2, int y2, char *hotkey, int help);
int UI_Get_Input(void);
void Screen_Present(void);
void Wait_Ticks(int ticks);
void Reset_Clip_Rect(void);
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
void Palette_Cycle_Reset(void);
void Palette_Unmark_Range(int first, int last);
void Palette_Mark_Range(int first, int last);
void Remap_Build_All(void);
void Font_Select_Palette(int font, char *colors);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Text_Shadow_Color(int color);
int Print_Text_Centered(int x, int y, char *text);
int Print_Text_Right(int x, int y, char *text);
int Print_Text(int x, int y, char *text);
int Print_Number_Right(int x, int y, int value);
void Hall_Of_Fame_List_Redraw(void);

/* () the Hall of Fame screen (HALOFAM.LBX 0) until any click or Esc; then the normal palette comes back */
void Screen_Hall_Of_Fame_List(void)
{
    unsigned pic;
    int input;
    int control;
    int done;

    UI_Suspend_Redraw_Fn();
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    UI_Clear_Controls();
    Palette_Fade_To_Black();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Set_Draw_Page();
    pic = LBX_Reload("HALOFAM", 0, p_scratch);
    Draw_Pic_Frame(0, 0, pic);
    Palette_Upload();
    Screen_Save_A800();
    Screen_Save_AC00();
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    UI_Clear_Help();
    UI_Set_Redraw_Fn(Hall_Of_Fame_List_Redraw, 2);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (done == 0) {
            Hall_Of_Fame_List_Redraw();
            Screen_Present();
            Wait_Ticks(2);
        }
    }
    UI_Clear_Controls();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
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

#define HOF_FULL_SCORE 8000L                    /* the score shown as 100 % */

/* () its ten entries: rank, wizard name and race, score and the score as a percentage of 8000 */
void Hall_Of_Fame_List_Redraw(void)
{
    int x;
    char space[2];
    int score;
    char colors[16];
    int i;
    int y;

    strcpy(space, " ");
    colors[0] = 0xf6;
    colors[1] = 0x31;
    colors[2] = 0xb0;
    colors[3] = 0xa2;
    colors[4] = 0xb2;
    colors[5] = 0xb3;
    colors[6] = 0x33;
    colors[7] = 0xb4;
    colors[8] = 0xb8;
    colors[9] = 0xb9;
    colors[10] = 0xd5;
    colors[11] = 0xba;
    colors[12] = 0xf6;
    colors[13] = 0xf6;
    colors[14] = 0xf6;
    colors[15] = 0xf6;
    Set_Draw_Page();
    Reset_Clip_Rect();
    Screen_Restore_A800();
    Font_Select_Palette(2, colors);
    Font_Select_Shadow(5, 15, 0, 0);
    Set_Text_Shadow_Color(0xfa);
    strcpy(p_text_buf, "Hall");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Of");
    strcat(p_text_buf, space);
    strcat(p_text_buf, "Fame");
    Print_Text_Centered(160, 7, p_text_buf);
    for (i = 0; i < 16; i++)
        colors[i] = 0xb3;
    colors[0] = 0xf6;
    Font_Select_Palette(2, colors);
    Font_Select_Shadow(2, 15, 0, 0);
    Set_Text_Shadow_Color(0xfa);
    x = 72;
    y = 42;
    for (i = 0; i < 10; i++) {
        if (game_config.hall_of_fame_scores[i] > 0) {
            if (i == 0)
                Print_Text_Right(x, y, "Master");
            else {
                itoa(i + 1, p_text_buf2, 10);
                strcpy(p_text_buf, p_text_buf2);
                strcat(p_text_buf, ")");
                Print_Text_Right(x, y, p_text_buf);
            }
            strcpy(p_text_buf, game_config.hall_of_fame_names[i]);
            strcat(p_text_buf, " of the ");
            strcat(p_text_buf, race_plural_names[game_config.hall_of_fame_race[i]]);
            Print_Text(x + 9, y, p_text_buf);
            Print_Number_Right(x + 170, y, game_config.hall_of_fame_scores[i]);
            score = (long)game_config.hall_of_fame_scores[i] * 100 / HOF_FULL_SCORE;
            itoa(score, p_text_buf2, 10);
            strcpy(p_text_buf, "(");
            strcat(p_text_buf, p_text_buf2);
            strcat(p_text_buf, "%)");
            Print_Text_Right(x + 198, y, p_text_buf);
            y += 13;
        }
    }
}
