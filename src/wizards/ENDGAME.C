/* ENDGAME.C - WIZARDS.EXE overlay code segment 78ba (stub 24d7): the end of a wizard - a captured fortress (banishment or
   elimination: the loser's cities, units, nodes, enchantments, treasury and treaties), the conquest screen and its
   animation stages, the check for the last rival, the win screen, the lose screen (a spinning, shrinking
   portrait), banishment (the Spell of Return, the human's resign question) and the AI's vortex screen.
   This module's data (DS:511c): lose_anim_path[27], then its literals; BSS (DS:c5ba..c5d6).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 78ba src/wizards/ENDGAME.C */
#include <string.h>
#include <stdlib.h>

/* the module's functions in their original declaration order */
void Wizard_Fortress_Captured(int loser, int winner, int city);
int Screen_Conquest(int loser, int winner);
void Conquest_Redraw(void);
void Conquest_Anim_Approach(void);
int Conquest_Anim_Load_Stage2(void);
int Conquest_Anim_Lightning(int first);
int Conquest_Anim_Shatter(void);
void Conquest_Anim_Neutral_Frame(void);
int All_Rivals_Defeated(void);
void Screen_Win(int player);
void Win_Redraw(void);
void Screen_Lose(int player);
void Lose_Redraw(void);
int Wizard_Banish(int loser, int winner);
void Screen_Banish_Vortex(int wizard);
void Banish_Vortex_Redraw(void);

#include "GAME.H"

unsigned conquest_portrait_pic;                 /* DS:c5d4  the loser's WIZLAB portrait */
unsigned conquest_anim_pic;                     /* DS:c5d2  the winner's CONQUEST picture */
unsigned conquest_stage_pic;                    /* DS:c5d0  CONQUEST 14 / 32; the vortex's CMBTFX 27 */
unsigned conquest_axeboy_pic;                   /* DS:c5ce  CONQUEST 15 */
unsigned conquest_lightning_pic2;               /* DS:c5cc  CONQUEST 16 */
unsigned conquest_lightning_pic;                /* DS:c5ca  CONQUEST 18 + loser portrait; VORTEX 0 */
int endgame_anim_frame;                         /* DS:c5c8  frame counter of the end-game screens */
unsigned conquest_sfx;                          /* DS:c5c6  the loser's scream (SOUNDFX 34 / 35) */
int conquest_loser;                             /* DS:c5c4 */
int conquest_winner;                            /* DS:c5c2  5 = raiders */
unsigned win_bg_pic;                            /* DS:c5c0  WIN 1 */
unsigned win_anim_pic;                          /* DS:c5be  WIN 3 + portrait; LOSE 1 + portrait */
int lose_anim_step;                             /* DS:c5bc  index into lose_anim_path */
unsigned lose_warp_pic;                         /* DS:c5ba  400-paragraph work picture of the lose animation */

/* DS:511c  the lose animation: top-left x, y, rotation, scale % (read as its low byte) per step */
struct LoseAnimStep lose_anim_path[27] = {
    { 0, 90, 0, 100 }, { 12, 73, 10, 100 }, { 25, 57, 20, 100 }, { 32, 45, 30, 100 }, { 40, 29, 40, 100 },
    { 42, 15, 50, 100 }, { 55, 3, 60, 100 }, { 77, -4, 70, 100 }, { 119, -10, 80, 95 }, { 147, -8, 90, 90 },
    { 170, -5, 100, 85 }, { 190, 7, 110, 80 }, { 212, 19, 120, 75 }, { 218, 40, 130, 70 }, { 227, 59, 140, 65 },
    { 217, 79, 150, 60 }, { 212, 97, 160, 55 }, { 196, 106, 170, 50 }, { 179, 113, 180, 45 },
    { 160, 109, 190, 40 }, { 145, 105, 200, 35 }, { 135, 100, 210, 30 }, { 130, 94, 220, 25 },
    { 135, 87, 230, 20 }, { 147, 87, 240, 15 }, { 152, 89, 250, 10 }, { 159, 98, 260, 5 }
};

extern unsigned p_music_buf;                    /* DS:9390 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned p_world_block;                /* DS:9d20 */
extern int current_player;                      /* DS:bd8c */
extern struct GameConfig game_config;           /* DS:bd9e */
extern char *p_text_buf2;                       /* DS:bfba */
extern char *p_text_buf;                        /* DS:bfbc */
extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */

void City_Garrison_List(int city, int *count, int *list);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void Wizard_Add_Gold(int player, int amount);
void Wizard_Add_Mana(int player, int amount);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
int Conquest_Loot_Spells(int winner, int loser);
void Screen_Final_Score(void);
void Quit_Hook_Empty(void);
void Exec_Program(char *path, char *arg0, char *arg1, char *arg2);
void Bit_Set_Near(int bit, char *map);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Str_Append_Possessive(char *name);
void Anim_Load_To_EMS(char *lbx, int entry);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
void Music_Play_Current(void);
void Font_Select_Outline_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
int Pic_Frame_Count(unsigned pic);
int Pic_Get_Frame(unsigned pic);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Anim_EMS_Next_Frame(void);
void Combat_Tmp_Save(void);
void Combat_Tmp_Restore(void);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Scale_Percent(unsigned pic, int x_pct, int y_pct);
void Pic_Draw_Rotated(int x, int y, int angle, unsigned pic);
int Spell_Casting_Cost(int player, int spell, int in_combat);
int Fortress_Hero_Skill_Bonus(int player);
void Minimap_Alloc_Pic(void);
void Map_Full_Redraw(void);
void Screen_Copy_Front(void);
int Confirm_Box(char *text);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);

/* (loser, winner, city) City_Change_Owner, when a fortress city falls. A loser with 2+ cities is banished first
   (Wizard_Banish); with fewer, or when the banishment answers > 0 (resign, or an AI that cannot return), the loser
   is eliminated: the city goes to the winner, the fortress is gone, the winner gains 5 fame; the loser's city
   enchantments are removed everywhere; in its cities normal units go neutral, heroes and fantastic units die, the
   cities go neutral (not when the loser is the current player: its cities stay its own); its nodes are released; the winner gets half
   its gold and mana; its other units die and its global enchantments end; the conquest screen (not after a
   banishment the loser resigned from: answer 1); treaties and contacts with it are erased and the winner loses 40
   relation with the others; the winner loots spells. Then the lose screen or, the last rival gone, the win screen,
   the Hall of Fame and MAGIC.EXE
   Numbered: BUG #226 (BUGS_AND_QUIRKS.txt) */
void Wizard_Fortress_Captured(int loser, int winner, int city)
{
    int i;
    int k;
    int garrison_count;
    char *globals;
    char far *enchants;
    int loser_cities;
    int banish_answer;
    int garrison[MAX_STACK];

    loser_cities = 0;
    for (i = 0; i < num_cities && loser_cities < 2; i++)
        if (p_cities[i].owner == loser)
            loser_cities++;
    banish_answer = 0;
    if (loser_cities > 1)
        banish_answer = Wizard_Banish(loser, winner);
    if (loser_cities < 2 || banish_answer >= 1) {
        if (city != -1)
            p_cities[city].owner = winner;
        p_fortresses[loser].active = 0;
        wizards[winner].fame += 5;
        for (i = 0; i < num_cities; i++) {
            enchants = p_cities[i].enchants;
            for (k = 0; k < NUM_CITY_ENCHANTS; k++)
                if (enchants[k] > 0 && enchants[k] - 1 == loser)
                    enchants[k] = 0;
            if (p_cities[i].owner == loser) {
                City_Garrison_List(i, &garrison_count, garrison);
                for (k = 0; k < garrison_count; k++) {
                    if (p_units[garrison[k]].owner == loser) {
                        if (unit_types[p_units[garrison[k]].type].ability_flags & AFLAG_FANTASTIC)
                            Unit_Kill_Or_Dismiss(garrison[k], UNIT_REMOVE_DISMISSED);
                        else if (p_units[garrison[k]].hero_slot != -1)
                            Unit_Kill_Or_Dismiss(garrison[k], UNIT_REMOVE_DISMISSED);
                        else
                            p_units[garrison[k]].owner = PLAYER_NEUTRAL;
                    }
                }
                if (p_cities[i].owner != current_player)
                    p_cities[i].owner = PLAYER_NEUTRAL;
            }
        }
        for (i = 0; i < NUM_NODES; i++) {
            if (p_nodes[i].owner == loser) {
                p_nodes[i].owner = PLAYER_NONE;
                p_nodes[i].meld_flags = 0;
            }
        }
        Wizard_Add_Gold(winner, wizards[loser].gold / 2);
        Wizard_Add_Mana(winner, wizards[loser].mana_reserve / 2);
        for (i = 0; i < num_units; i++)
            if (p_units[i].owner == loser)
                Unit_Kill_Or_Dismiss(i, UNIT_REMOVE_DISMISSED);
        globals = (char *)&wizards[loser].globals;
        for (i = 0; i < NUM_GLOBAL_ENCHANTS; i++)
            globals[i] = 0;
        if (banish_answer != 1)
            Screen_Conquest(loser, winner);
        for (i = 0; i < num_players; i++) {
            wizards[i].diplomacy_status[loser] = TREATY_NONE;
            wizards[loser].diplomacy_status[i] = TREATY_NONE;
            wizards[i].contacted[loser] = 0;
            wizards[loser].contacted[i] = 0;
            if (i != loser && i != winner)
                Diplo_Relation_Change(-40, winner, i, DIPLO_MSG_TROOPS_NEAR_CITIES, 0, 0);   /* the conquest's message: "troops near
                                                   our cities" */
        }
        Conquest_Loot_Spells(winner, loser);
        if (loser == current_player) {
            Screen_Lose(loser);
            Screen_Final_Score();
            Quit_Hook_Empty();
            Exec_Program("MAGIC.EXE", "JENNY", "", "");   /* JENNY: MAGIC.EXE skips the intro */
        } else if (All_Rivals_Defeated() == 1) {
            Screen_Win(current_player);
            Screen_Final_Score();
            Quit_Hook_Empty();
            Exec_Program("MAGIC.EXE", "JENNY", "", "");   /* JENNY: MAGIC.EXE skips the intro */
        }
    }
}

/* (loser, winner) the conquest screen: "<winner> banishes / defeats <loser>" or "Raiders destroy <loser>['s
   fortress]." over the WIZLAB background, the CONQUEST animation (Conquest_Redraw, set at 6 ticks; the loop waits 5 a frame; a click or
   frame 31 ends it) and its music. A neutral loser shows nothing unless the raiders are the winner. The human
   winner records the loser in its defeated-wizards bits */
int Screen_Conquest(int loser, int winner)
{
    int i;                                      /* loop counter, then picture, then music entry */
    int done;
    int input;
    int control;
    unsigned music;
    int loser_cities;
    char space[4];

    strcpy(space, " ");
    if (winner != PLAYER_NEUTRAL && loser == PLAYER_NEUTRAL)
        return 0;
    if (winner == current_player)
        Bit_Set_Near(loser, wizards[current_player].defeated_wizards);
    conquest_loser = loser;
    conquest_winner = winner;
    Sound_Stop();
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    UI_Clear_Controls();
    Palette_Fade_To_Black();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    loser_cities = 0;
    for (i = 0; i < num_cities && loser_cities < 2; i++)
        if (p_cities[i].owner == conquest_loser)
            loser_cities++;
    Palette_Load(3, -1);
    Palette_Upload();
    Set_Draw_Page();
    i = LBX_Reload("WIZLAB", 19, p_scratch);
    Draw_Pic_Frame(0, 0, i);
    Screen_Save_A800();
    Screen_Save_AC00();
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    if (conquest_winner != PLAYER_NEUTRAL) {
        strcpy(p_text_buf, wizards[winner].wizard_name);
        strcat(p_text_buf, space);
        strcat(p_text_buf, loser_cities > 1 ? "banishes" : "defeats");
        strcat(p_text_buf, space);
        strcat(p_text_buf, wizards[loser].wizard_name);
    } else {
        strcpy(p_text_buf, "Raiders");
        strcat(p_text_buf, space);
        strcat(p_text_buf, "destroy");
        strcat(p_text_buf, space);
        strcpy(p_text_buf2, wizards[loser].wizard_name);
        if (loser_cities > 1) {
            Str_Append_Possessive(p_text_buf2);
            strcat(p_text_buf, p_text_buf2);
            strcat(p_text_buf, space);
            strcat(p_text_buf, "fortress");
        } else
            strcat(p_text_buf, p_text_buf2);
        strcat(p_text_buf, ".");
    }
    conquest_portrait_pic = LBX_Reload("WIZLAB", wizards[loser].portrait, p_scratch);
    conquest_anim_pic = LBX_Reload_Next("CONQUEST", wizards[winner].portrait, p_scratch);
    conquest_stage_pic = LBX_Reload_Next("CONQUEST", 14, p_scratch);
    conquest_axeboy_pic = LBX_Reload_Next("CONQUEST", 15, p_scratch);
    Anim_Load_To_EMS("CONQUEST", 17);
    if (game_config.music == 1) {
        i = MUSIC_VICTORY;
        if (loser == current_player)
            i = MUSIC_DEFEAT;
        music = LBX_Reload("MUSIC", i, p_music_buf);
        Sound_Play(music);
    }
    UI_Set_Redraw_Fn(Conquest_Redraw, 6);
    endgame_anim_frame = 0;
    done = 0;
    for (i = 0; i < 3; i++) {
        Timer_Mark();
        Conquest_Redraw();
        endgame_anim_frame = 0;
        Screen_Present();
        Wait_Ticks(5);
    }
    endgame_anim_frame = 0;
    UI_Input_Delay(3);
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (endgame_anim_frame > 30)
            done = -1;
        if (done == 0) {
            Conquest_Redraw();
            Screen_Present();
            Wait_Ticks(5);
        }
    }
    Sound_Stop();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Palette_Fade_To_Black();
    Palette_Load(0, -1);
    Palette_Cycle_Reset();
    Palette_Unmark_Range(0, 255);
    Palette_Mark_Range(0, 223);
    Remap_Build_All();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Music_Play_Current();
}

/* () redraw of the conquest screen: frame 0 the portrait; 1-6 the approach; 7 the second stage loads; 7-10
   lightning; 11-18 the shatter (with the scream at 11); 19-22 lightning again; under raiders a static frame
   from 7 on; then the caption */
void Conquest_Redraw(void)
{
    Set_Draw_Page();
    Reset_Clip_Rect();
    Screen_Restore_A800();
    if (endgame_anim_frame == 0)
        Draw_Pic_Frame(69, 75, conquest_portrait_pic);
    if (endgame_anim_frame > 0) {
        if (endgame_anim_frame < 7)
            Conquest_Anim_Approach();
        else {
            if (endgame_anim_frame == 7)
                Conquest_Anim_Load_Stage2();
            if (endgame_anim_frame < 11)
                Conquest_Anim_Lightning(1);
            else if (endgame_anim_frame < 19) {
                if (conquest_winner != PLAYER_NEUTRAL && endgame_anim_frame == 11 && game_config.sound_effects == 1)
                    Sound_Play(conquest_sfx);
                Conquest_Anim_Shatter();
            } else if (endgame_anim_frame < 23)
                Conquest_Anim_Lightning(0);
        }
    }
    if (endgame_anim_frame > 6 && conquest_winner == PLAYER_NEUTRAL) {
        Draw_Pic_Frame(69, 75, conquest_portrait_pic);
        Conquest_Anim_Neutral_Frame();
    }
    Font_Select_Outline_Shadow(5, 5, 0, 0);
    Set_Text_Shadow_Color(1);
    Print_Text_Centered(160, 6, p_text_buf);
    endgame_anim_frame++;
}

/* () frames 1-6: the axe man (conquest_axeboy_pic, CONQUEST.LBX 'AXEBOY'), the winner (not for raiders) and the stage slide in */
void Conquest_Anim_Approach(void)
{
    int x;
    int y;

    Draw_Pic_Frame(69, 75, conquest_portrait_pic);
    Set_Clip_Rect(0, 0, 319, 199);
    x = 66 - (endgame_anim_frame - 1) * 11;
    y = -(endgame_anim_frame + 5);
    Draw_Pic_Clipped(x + 252, y + 88, conquest_axeboy_pic);
    if (conquest_winner != PLAYER_NEUTRAL) {
        x = 90 - (endgame_anim_frame - 1) * 15;
        y = 78 - (endgame_anim_frame - 1) * 13;
        Draw_Pic_Clipped(x + 208, y + 60, conquest_anim_pic);
    }
    x = 30 - (endgame_anim_frame - 1) * 5;
    y = 60 - (endgame_anim_frame - 1) * 10;
    Draw_Pic_Clipped(x + 100, y + 120, conquest_stage_pic);
}

/* () frame 7 (not under raiders): the last approach frame becomes the background; the loser's portrait, the
   winner's second CONQUEST picture, the loser's lightning, the bolt and the second stage load, and the scream:
   SOUNDFX 34 for portraits 2, 9, 11, 13, else 35
   Numbered: BUG #108 (BUGS_AND_QUIRKS.txt) */
int Conquest_Anim_Load_Stage2(void)
{
    int x;
    int y;
    int sfx;

    if (conquest_winner == PLAYER_NEUTRAL)
        return 0;
    Set_Clip_Rect(0, 0, 319, 199);
    x = 66 - (endgame_anim_frame - 1) * 11;
    y = -(endgame_anim_frame + 5);
    Draw_Pic_Clipped(x + 252, y + 88, conquest_axeboy_pic);
    x = 90 - (endgame_anim_frame - 1) * 15;
    y = 78 - (endgame_anim_frame - 1) * 13;
    Draw_Pic_Clipped(x + 208, y + 60, conquest_anim_pic);
    x = 30 - (endgame_anim_frame - 1) * 5;
    y = 60 - (endgame_anim_frame - 1) * 10;
    Draw_Pic_Clipped(x + 100, y + 120, conquest_stage_pic);
    Screen_Save_A800();
    conquest_portrait_pic = LBX_Reload("WIZLAB", wizards[conquest_loser].portrait, p_scratch);
    conquest_anim_pic = LBX_Reload_Next("CONQUEST", wizards[conquest_winner].portrait + 33, p_scratch);
    conquest_lightning_pic = LBX_Reload_Next("CONQUEST", wizards[conquest_loser].portrait + 18, p_scratch);
    conquest_lightning_pic2 = LBX_Reload_Next("CONQUEST", 16, p_scratch);
    conquest_stage_pic = LBX_Reload_Next("CONQUEST", 32, p_scratch);
    if (game_config.sound_effects == 1) {
        switch (wizards[conquest_loser].portrait) {   /* the female wizards scream differently */
        case WIZ_PORTRAIT_SHAREE:
        case WIZ_PORTRAIT_FREYA:
        case WIZ_PORTRAIT_ARIEL:
        case WIZ_PORTRAIT_KALI:
            sfx = 34;
            break;
        default:
            sfx = 35;
            break;
        }
        conquest_sfx = LBX_Reload_Next("SOUNDFX", sfx, p_scratch);
    }
}

/* (first) lightning frames: the bolt; after the first, the bolt steps back a frame and the loser's lightning is
   drawn as often as its frame says (each draw advances it) */
int Conquest_Anim_Lightning(int first)
{
    int frame;
    int count;                                  /* frame count of the bolt, never read */

    if (conquest_winner == PLAYER_NEUTRAL)
        return 0;
    count = Pic_Frame_Count(conquest_lightning_pic2);
    frame = Pic_Get_Frame(conquest_lightning_pic2);
    if (first == 1) {
        Draw_Pic_Frame(69, 75, conquest_portrait_pic);
        Draw_Pic_Frame(177, 62, conquest_lightning_pic2);
    } else {
        Draw_Pic_Frame(177, 62, conquest_lightning_pic2);
        Set_Pic_Frame(conquest_lightning_pic2, frame - 1);
        frame = Pic_Get_Frame(conquest_lightning_pic);
        switch (frame) {
        case 0:
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            break;
        case 1:
            Set_Pic_Frame(conquest_lightning_pic, 0);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            break;
        case 2:
            Set_Pic_Frame(conquest_lightning_pic, 0);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            break;
        case 3:
            Set_Pic_Frame(conquest_lightning_pic, 0);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            Draw_Pic_Frame(66, 16, conquest_lightning_pic);
            break;
        }
    }
    Draw_Pic_Clipped(208, 60, conquest_anim_pic);
    Draw_Pic_Clipped(100, 120, conquest_stage_pic);
}

/* () frames 11-18: the portrait and the next shatter frame (EMS animation) */
int Conquest_Anim_Shatter(void)
{
    if (conquest_winner == PLAYER_NEUTRAL)
        return 0;
    Draw_Pic_Frame(69, 75, conquest_portrait_pic);
    Anim_EMS_Next_Frame();
    Draw_Pic_Clipped(208, 60, conquest_anim_pic);
    Draw_Pic_Clipped(100, 120, conquest_stage_pic);
}

/* () the static frame under raiders */
void Conquest_Anim_Neutral_Frame(void)
{
    Draw_Pic_Clipped(252, 88, conquest_axeboy_pic);
    if (conquest_winner != PLAYER_NEUTRAL)
        Draw_Pic_Clipped(208, 60, conquest_anim_pic);
    Draw_Pic_Clipped(100, 120, conquest_stage_pic);
}

/* () -> 1 when no rival (players 1..) has an active fortress or a city (a banished rival with a city keeps the
   game going) */
int All_Rivals_Defeated(void)
{
    int player;
    int alive;
    int city;
    int cities;

    alive = 0;
    for (player = 1; player < num_players; player++) {
        if (p_fortresses[player].active == 1)
            alive++;
        else {
            cities = 0;
            for (city = 0; city < num_cities && cities < 2; city++)
                if (p_cities[city].owner == player)
                    cities++;
            if (cities > 0)
                alive++;
        }
    }
    if (alive == 0)
        return 1;
    else
        return 0;
}

/* (player) the win screen: WIN 0 and the player's figure (WIN 17..22 by portrait only), the winner's
   animation (Win_Redraw, 4 ticks a frame; a click or frame 91 ends it) and its music. p_cities is re-pointed at a
   fresh block of p_world_block (nothing is copied) while the combat temp area is saved */
void Screen_Win(int player)
{
    int entry;
    int n;                                      /* picture, then music entry */
    int done;
    int input;
    int control;
    unsigned music;

    Combat_Tmp_Save();
    p_cities = (struct City far *)Seg_To_Far_Ptr(Heap_Alloc_Front(p_world_block, 714));   /* MAX_CITIES x 0x72 bytes */
    Sound_Stop();
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    UI_Clear_Controls();
    Palette_Fade_To_Black();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Palette_Load(5, -1);
    Palette_Upload();
    Set_Draw_Page();
    n = LBX_Reload("WIN", 0, p_scratch);
    Draw_Pic_Frame(0, 0, n);
    switch (wizards[player].portrait) {
    case WIZ_PORTRAIT_RAVEN:
        entry = 20;
        break;
    case WIZ_PORTRAIT_SHAREE:
        entry = 19;
        break;
    case WIZ_PORTRAIT_OBERIC:
        entry = 21;
        break;
    case WIZ_PORTRAIT_SSSRA:
        entry = 22;
        break;
    case WIZ_PORTRAIT_FREYA:
    case WIZ_PORTRAIT_ARIEL:
    case WIZ_PORTRAIT_KALI:
        entry = 18;
        break;
    default:
        entry = 17;
        break;
    }
    n = LBX_Reload("WIN", entry, p_scratch);
    Draw_Pic_Frame(22, 143, n);
    Screen_Save_A800();
    Screen_Save_AC00();
    win_anim_pic = LBX_Reload("WIN", wizards[player].portrait + 3, p_scratch);
    win_bg_pic = LBX_Reload("WIN", 1, p_world_block);
    Anim_Load_To_EMS("WIN", 2);
    if (game_config.music == 1) {
        n = MUSIC_VICTORY;
        music = LBX_Reload("MUSIC", n, p_music_buf);
        Sound_Play(music);
    }
    UI_Set_Redraw_Fn(Win_Redraw, 4);
    done = 0;
    endgame_anim_frame = 0;
    UI_Input_Delay(3);
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (endgame_anim_frame > 90)
            done = -1;
        if (done == 0) {
            Win_Redraw();
            Screen_Present();
            Wait_Ticks(4);
        }
    }
    Sound_Stop();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Palette_Fade_To_Black();
    Palette_Load(0, -1);
    Palette_Cycle_Reset();
    Palette_Unmark_Range(0, 255);
    Palette_Mark_Range(0, 223);
    Remap_Build_All();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Music_Play_Current();
    Combat_Tmp_Restore();
}

/* () redraw of the win screen: the winner's animation replayed to its frame, the background, the next EMS frame
   and the four caption lines, 15 frames each */
void Win_Redraw(void)
{
    int i;
    int frame;
    char space[4];

    strcpy(space, " ");
    Set_Draw_Page();
    Reset_Clip_Rect();
    Screen_Restore_A800();
    frame = Pic_Get_Frame(win_anim_pic);
    Set_Pic_Frame(win_anim_pic, 0);
    for (i = 0; i <= frame; i++)
        Draw_Pic_Frame(93, 2, win_anim_pic);
    Draw_Pic_Frame(0, 0, win_bg_pic);
    Anim_EMS_Next_Frame();
    Font_Select_Outline_Shadow(5, 5, 0, 0);
    Set_Text_Shadow_Color(0);
    if (endgame_anim_frame < 15)
        strcpy(p_text_buf, "Having conquered both the");
    else if (endgame_anim_frame < 30)
        strcpy(p_text_buf, "world of Arcanus and Myrror,");
    else if (endgame_anim_frame < 45)
        strcpy(p_text_buf, "I and only I, remain the one");
    else if (endgame_anim_frame < 60)
        strcpy(p_text_buf, "and true Master of Magic.");
    Print_Text_Centered(160, 180, p_text_buf);
    endgame_anim_frame++;
}

/* (player) the lose screen: the player's LOSE portrait spinning away along lose_anim_path (Lose_Redraw, 4 ticks
   a frame; a click or frame 46 ends it) and its music */
void Screen_Lose(int player)
{
    int done;
    int input;
    int control;
    unsigned music;

    Sound_Stop();
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    UI_Clear_Controls();
    Palette_Fade_To_Black();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Palette_Load(6, -1);
    Palette_Upload();
    Set_Draw_Page();
    lose_anim_step = 0;
    win_anim_pic = LBX_Reload("LOSE", wizards[player].portrait + 1, p_scratch);
    lose_warp_pic = Heap_Alloc_Back(p_scratch, 400);
    Anim_Load_To_EMS("LOSE", 0);
    if (game_config.music == 1) {
        music = LBX_Reload("MUSIC", MUSIC_DEFEAT, p_music_buf);
        Sound_Play(music);
    }
    UI_Set_Redraw_Fn(Lose_Redraw, 4);
    done = 0;
    endgame_anim_frame = 0;
    UI_Input_Delay(3);
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (endgame_anim_frame > 45)
            done = -1;
        if (done == 0) {
            Lose_Redraw();
            Screen_Present();
            Wait_Ticks(4);
        }
    }
    Sound_Stop();
    UI_Clear_Controls();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Palette_Fade_To_Black();
    Palette_Load(0, -1);
    Palette_Cycle_Reset();
    Palette_Unmark_Range(0, 255);
    Palette_Mark_Range(0, 223);
    Remap_Build_All();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Music_Play_Current();
}

/* () redraw of the lose screen: the next EMS frame and, for the 27 path steps, the portrait scaled and drawn
   rotated at the step's place */
void Lose_Redraw(void)
{
    int x;
    int y;

    Set_Draw_Page();
    Anim_EMS_Next_Frame();
    if (lose_anim_step < 27) {
        x = lose_anim_path[lose_anim_step].x;
        y = lose_anim_path[lose_anim_step].y;
        Pic_Decode_Frame_To_Bitmap(win_anim_pic, lose_warp_pic);
        Pic_Scale_Percent(lose_warp_pic, (char)lose_anim_path[lose_anim_step].scale,
                          (char)lose_anim_path[lose_anim_step].scale);
        Pic_Draw_Rotated(x, y, lose_anim_path[lose_anim_step].angle, lose_warp_pic);
    }
    endgame_anim_frame++;
    lose_anim_step++;
}

/* (loser, winner) -> 0 banished, > 0 eliminate instead (99: an AI whose casting skill + fortress heroes' bonus is
   under 40 or that has no mana; 1: the human resigns). The loser starts casting the Spell of Return at full cost;
   its fortress is gone, the winner gains 5 fame, the loser keeps half its mana and the winner gains that much
   (the half kept, rounded down: one point under the half lost when odd), the conquest screen, the winner loses 20 relation with the others.
   The human is asked "There are at least N turns remaining until you may return. Do you wish to resign?"; an AI
   gets the vortex screen and the winner loots its spells
   Numbered: BUG #265 (BUGS_AND_QUIRKS.txt) */
int Wizard_Banish(int loser, int winner)
{
    int player;
    int answer;
    int turns;
    char space[4];
    char num_text[50];

    strcpy(space, " ");
    wizards[loser].casting_spell = SPELL_SPELL_OF_RETURN;
    wizards[loser].casting_cost_left = Spell_Casting_Cost(loser, SPELL_SPELL_OF_RETURN, 0);
    wizards[loser].casting_cost_initial = Spell_Casting_Cost(loser, SPELL_SPELL_OF_RETURN, 0);
    turns = wizards[loser].casting_cost_left / wizards[loser].nominal_casting_skill;   /* no test for a zero skill */
    if (loser != current_player) {
        if (Wizard_Casting_Skill(loser) + Fortress_Hero_Skill_Bonus(loser) < 40 || wizards[loser].mana_reserve < 1)
            return 99;                          /* an AI that cannot cast Spell of Return: eliminated */
    }
    p_fortresses[loser].active = 0;
    wizards[winner].fame += 5;
    wizards[loser].mana_reserve = wizards[loser].mana_reserve / 2;
    Wizard_Add_Mana(winner, wizards[loser].mana_reserve);
    Screen_Conquest(loser, winner);
    for (player = 0; player < num_players; player++)
        if (player != loser && player != winner)
            Diplo_Relation_Change(-20, winner, player, DIPLO_MSG_TROOPS_NEAR_CITIES, 0, 0);
    answer = 0;
    Minimap_Alloc_Pic();
    Map_Full_Redraw();
    Screen_Copy_Front();
    if (loser == current_player) {
        itoa(turns, num_text, 10);
        strcpy(message_text, "There are at least ");
        strcat(message_text, num_text);
        strcat(message_text, " turns remaining until you may return. Do you wish to resign?");
        answer = Confirm_Box(message_text);
    } else if (loser != PLAYER_NEUTRAL) {
        Screen_Save_A800();
        Screen_Banish_Vortex(loser);
        Conquest_Loot_Spells(winner, loser);
    }
    return answer;
}

/* (wizard) "<name> begins casting the Spell Of Return." over the VORTEX animation (Banish_Vortex_Redraw, set at 6
   ticks; the loop waits 5 a frame; a click or frame 41 ends it) */
void Screen_Banish_Vortex(int wizard)
{
    int done;
    int input;
    int control;
    char space[4];

    strcpy(space, " ");
    p_text_buf = Near_Alloc_Reset(100);
    strcpy(p_text_buf, wizards[wizard].wizard_name);
    strcat(p_text_buf, " begins casting the Spell Of Return.");
    conquest_lightning_pic = LBX_Reload("VORTEX", 0, p_scratch);
    conquest_stage_pic = LBX_Reload_Next("CMBTFX", 27, p_scratch);
    UI_Set_Redraw_Fn(Banish_Vortex_Redraw, 6);
    endgame_anim_frame = 0;
    done = 0;
    endgame_anim_frame = 0;
    UI_Input_Delay(3);
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (endgame_anim_frame > 40)
            done = -1;
        if (done == 0) {
            Banish_Vortex_Redraw();
            Screen_Present();
            Wait_Ticks(5);
        }
    }
    UI_Clear_Controls();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
}

/* () redraw of the vortex screen: the vortex, the casting wizard's lightning and the caption paragraph */
void Banish_Vortex_Redraw(void)
{
    int x;
    int y;

    x = 51;
    y = 28;
    Set_Draw_Page();
    Screen_Restore_A800();
    Draw_Pic_Frame(x, y, conquest_stage_pic);
    Draw_Pic_Frame(x + 8, y + 8, conquest_lightning_pic);
    Set_Text_Shadow_Color(16);
    Font_Select_Shadow(4, 4, 0, 0);
    Set_Font_Color(190);
    Paragraph_Print(x + 7, y + 114, 165, p_text_buf, PARA_CENTER);
    endgame_anim_frame++;
}
