/* MASTERY.C - WIZARDS.EXE overlay code segment c06a: the Spell of Mastery sequences - "<wizard> has started casting
   the Spell of Mastery", the human's win (the rival portraits struck one by one, then Screen_Win, the Hall of Fame
   and MAGIC.EXE) and the loss to an AI caster (the portrait sheared, shrunk and dissolved) - and, sharing the
   overlay, the post-combat "rise from the dead" picture.
   This module's data (DS:6dca): its literals; BSS (DS:ca58..ca65).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS c06a src/wizards/MASTERY.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Undead_Rise_Load(int creator);
void Undead_Rise_Draw(void);
void Screen_Undead_Rise(int creator);
void Mastery_Lose_Load(int portrait);
void Mastery_Lose_Draw(void);
void Screen_Mastery_Lose(void);
void Mastery_Started_Draw(void);
void Screen_Mastery_Started(int player);
void Mastery_Win_Load(void);
void Mastery_Win_Draw(void);
void Spell_Of_Mastery_Complete(int caster);

#include "GAME.H"

#define MASTERY_RIVAL_FRAMES 60         /* Spell of Mastery: 60 frames per rival shown, 6 before and after */
#define MASTERY_LEAD_FRAMES 6

unsigned undead_rise_bg_pic;                    /* DS:ca64  CMBTFX.LBX 0x1b */
unsigned undead_rise_rocks_pic;                    /* DS:ca62  CMBTFX.LBX 0x1d (UNDEADMS): the rocks, drawn over the figures */
unsigned undead_rise_back_pic;                    /* DS:ca60  CMBTFX.LBX 0x1e (UNDEADRM): the figures behind */
unsigned undead_rise_front_pic;                    /* DS:ca5e  CMBTFX.LBX 0x1f (UNDEADFM): the figures in front */
unsigned undead_rise_unit_pic;                  /* DS:ca5c  MONSTER.LBX: the risen unit */
unsigned mastery_lose_pic;                      /* DS:ca5a  SPELLOSE.LBX 0x1d */
unsigned mastery_lose_portrait_pic;             /* DS:ca58  SPELLOSE.LBX <portrait> */

extern struct MouseRegion mouse_region_busy;    /* DS:0106 */
extern unsigned music_jingle;                   /* DS:9134 */
extern unsigned p_music_buf;                    /* DS:9390 */
extern int present_mode;                        /* DS:9c8e */
extern unsigned p_world_block;                /* DS:9d20 */
extern int combat_loc_x;                        /* DS:c480  reused here as the frame counter */
extern unsigned wizlab_portrait_pic;            /* DS:c968 */
extern unsigned wizlab_familiar_pic;            /* DS:c96a */
extern unsigned wizlab_top_pic;                 /* DS:c96c */
extern unsigned mastery_lab_pic;                /* DS:c9f4 */
extern int mastery_caster;                      /* DS:c9f6 */
extern int cast_anim_frame;                     /* DS:ca14 */
extern unsigned mastery_twinkle_pic;            /* DS:ca24 */
extern unsigned castui_frame_pic;               /* DS:ca26 */
extern int disjunction_choice;                  /* DS:ca2c */
#define mastery_anim_entry disjunction_choice    /* here: the SPELLSCR.LBX animation entry loaded */
extern unsigned ui_temp_pic_a;                  /* DS:ca34 */
extern unsigned summon_anim_pic;                /* DS:ca54 */
extern int mastery_winner;                      /* DS:ca56  the player who cast the Spell of Mastery */

/* Print_Text_Centered has no prototype here: this module calls it with a fourth argument */
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Screen_Copy_Front(void);
void Combat_Tmp_Save(void);
void Combat_Tmp_Restore(void);
void Screen_Lose(int player);
void Screen_Win(int player);
void Screen_Final_Score(void);
void Quit_Hook_Empty(void);
void Exec_Program(char *path, char *arg0, char *arg1, char *arg2);
int Anim_Get_Frame(void);
void Anim_Set_Frame(int frame);
void Anim_EMS_Next_Frame(void);
void Anim_Load_To_EMS(char *lbx, int entry);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
void Palette_Fade_From_Black(void);
void Music_Play_Current(void);
void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
void Set_Draw_Page_Front(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Pic_Apply_Palette(unsigned pic);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Reset_Frame(unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Get_Opaque_Bounds(unsigned pic, int *x, int *y, int *w, int *h);
void Pic_Shear_Columns(int x1, int dy1, int x2, int dy2, unsigned pic);
void Pic_Scale_Percent(unsigned pic, int x_pct, int y_pct);
void Pic_Dissolve(unsigned pic, int amount);
void Draw_Bitmap(int x, int y, unsigned pic);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Set_Font_Color(int color);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Print_Text_Centered();                     /* no prototype here: called with a 4th word the routine never reads */

/* (portrait) the loss: SPELLOSE.LBX 0x1c background (and palette), the human's familiar (0x1e + realm), "<caster>
   has cast the", the spell picture (0x1d) and the human's portrait in two versions (portrait, portrait + 14) */
void Mastery_Lose_Load(int portrait)
{
    castui_frame_pic = LBX_Reload("spellose", 0x1c, p_scratch);   /* the SPELLOSE background in CASTUI's frame word */
    wizlab_familiar_pic = LBX_Reload_Next("spellose", wizards[PLAYER_HUMAN].primary_realm + 0x1e, p_scratch);
    Pic_Apply_Palette(castui_frame_pic);
    Palette_Upload();
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, castui_frame_pic);
    Screen_Save_A800();
    strcpy(message_text, wizards[mastery_winner].wizard_name);
    strcat(message_text, " has cast the");
    mastery_lose_pic = LBX_Reload("spellose", 0x1d, p_scratch);
    mastery_lose_portrait_pic = LBX_Reload_Next("spellose", portrait, p_scratch);
    wizlab_portrait_pic = LBX_Reload_Next("spellose", portrait + 14, p_scratch);
    ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 900);
}

/* () a frame of the loss: frames 0..11 the portrait with the spell rising, 12..23 the portrait sheared, shrunk and
   slid off while the other picture dissolves in, 24..35 that picture's own frames; the familiar below */
void Mastery_Lose_Draw(void)
{
    int x1;
    int y1;
    int w;
    int h;
    int slide_x;
    int spell_x;
    char colors[16];
    int dy;
    int i;

    for (i = 0; i < 16; i++)
        colors[i] = 5;
    Screen_Restore_A800();
    Pic_Reset_Frame(wizlab_portrait_pic);
    Pic_Decode_Frame_To_Bitmap(wizlab_portrait_pic, ui_temp_pic_a);
    Pic_Get_Opaque_Bounds(ui_temp_pic_a, &x1, &y1, &w, &h);
    spell_x = x1 + w / 2 + 35;
    Pic_Reset_Frame(wizlab_portrait_pic);
    Font_Select_Palette(5, colors);
    Set_Text_Shadow_Color(0);
    Font_Select_Outline(5, 15, 0, 0);
    Print_Text_Centered(160, 5, message_text, 2);
    Print_Text_Centered(160, 25, "Spell of Mastery");
    if (cast_anim_frame < 12) {
        Draw_Pic_Frame(61, 68, wizlab_portrait_pic);
        Set_Pic_Frame(mastery_lose_pic, cast_anim_frame / 2);
        Draw_Pic_Frame(spell_x, 34, mastery_lose_pic);
    } else if (cast_anim_frame < 24) {
        Set_Pic_Frame(mastery_lose_pic, 5);
        Draw_Pic_Frame(spell_x, 34, mastery_lose_pic);
        slide_x = 61;
        dy = -y1;
        if ((cast_anim_frame - 12) / 2 < 2) {
            Pic_Shear_Columns(x1, 0, x1 + w / 4, dy / 6, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w / 4 + 1, dy / 6, x1 + w / 2, dy / 3, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w / 2 + 1, dy / 3, x1 + w * 3 / 4, dy / 6, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w * 3 / 4 + 1, dy / 6, x1 + w, 0, ui_temp_pic_a);
        } else if ((cast_anim_frame - 12) / 2 < 4) {
            Pic_Shear_Columns(x1, 0, x1 + w / 4, dy / 4, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w / 4 + 1, dy / 4, x1 + w / 2, dy / 2, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w / 2 + 1, dy / 2, x1 + w * 3 / 4, dy / 4, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w * 3 / 4 + 1, dy / 4, x1 + w, 0, ui_temp_pic_a);
        } else {
            Pic_Shear_Columns(x1, 0, x1 + w / 4, dy / 2, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w / 2 + 1, dy / 2, x1 + w / 2, dy, ui_temp_pic_a);   /* starts past its end (the other
                                                   stages: w / 4 + 1 .. w / 2); Pic_Shear_Columns swaps it, so only
                                                   columns w / 2 .. w / 2 + 1 move */
            Pic_Shear_Columns(x1 + w / 2 + 1, dy, x1 + w * 3 / 4, dy / 2, ui_temp_pic_a);
            Pic_Shear_Columns(x1 + w * 3 / 4 + 1, dy / 2, x1 + w, 0, ui_temp_pic_a);
        }
        Pic_Scale_Percent(ui_temp_pic_a, 100 - (cast_anim_frame - 12) * 6, 100);
        slide_x = (cast_anim_frame - 12) * 714 / 200 + 61;
        Draw_Bitmap(slide_x, 68, ui_temp_pic_a);
        Pic_Reset_Frame(mastery_lose_portrait_pic);
        Pic_Decode_Frame_To_Bitmap(mastery_lose_portrait_pic, ui_temp_pic_a);
        Pic_Dissolve(ui_temp_pic_a, (cast_anim_frame - 12) * 8);
        Draw_Bitmap(spell_x, 34, ui_temp_pic_a);
    } else if (cast_anim_frame < 36) {
        Set_Pic_Frame(mastery_lose_portrait_pic, (cast_anim_frame - 24) / 2);
        Draw_Pic_Frame(spell_x, 34, mastery_lose_portrait_pic);
    }
    Draw_Pic_Frame(190, 160, wizlab_familiar_pic);
}

/* () an AI completed the Spell of Mastery: the loss sequence, then Screen_Lose, the Hall of Fame and MAGIC.EXE */
void Screen_Mastery_Lose(void)
{
    UI_Clear_Controls();
    Sound_Stop();
    music_jingle = LBX_Reload("music", MUSIC_MASTERY_LOST, p_music_buf);
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    Palette_Fade_To_Black();
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Screen_Copy_Front();
    Mastery_Lose_Load(wizards[PLAYER_HUMAN].portrait);
    UI_Set_Redraw_Fn(Mastery_Lose_Draw, 2);
    cast_anim_frame = 0;
    present_mode = PRESENT_DISSOLVE;
    Set_Draw_Page();
    Mastery_Lose_Draw();
    Screen_Present();
    for (cast_anim_frame = 0; cast_anim_frame < 43; cast_anim_frame++) {
        Timer_Mark();
        Set_Draw_Page();
        Mastery_Lose_Draw();
        Screen_Present();
        Wait_Ticks(2);
    }
    Combat_Tmp_Restore();
    Screen_Lose(0);
    Screen_Final_Score();
    Quit_Hook_Empty();
    Exec_Program("MAGIC.EXE", "JENNY", "", "");
}

/* () a frame of "has started casting": the EMS animation replayed up to its next frame, the caster line and "Spell
   of Mastery"; combat_loc_x counts the frames */
void Mastery_Started_Draw(void)
{
    int i;
    int frames;

    frames = Anim_Get_Frame();
    frames = frames + 1;
    Anim_Set_Frame(0);
    Set_Draw_Page();
    for (i = 0; i < frames; i++)
        Anim_EMS_Next_Frame();
    Set_Text_Shadow_Color(0);
    Font_Select_Outline(5, 5, 0, 0);
    Print_Text_Centered(160, 160, message_text, 2);
    Print_Text_Centered(160, 180, "Spell of Mastery");
    combat_loc_x++;
}

/* (player) "<wizard> has started casting the Spell of Mastery": SPELLSCR.LBX 0x43..0x46 animated for 200 frames or
   until a click, then back to the map */
void Screen_Mastery_Started(int player)
{
    int control;

    UI_Suspend_Redraw_Fn();
    Sound_Stop();
    strcpy(message_text, wizards[player].wizard_name);
    strcat(message_text, " has started casting the");
    music_jingle = LBX_Reload("music", MUSIC_MASTERY, p_music_buf);
    Anim_Load_To_EMS("spellscr", 0x43);
    mastery_anim_entry = 0x43;
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    Palette_Fade_To_Black();
    Set_Draw_Page_Front();
    Fill_Rect(0, 0, 319, 199, 0);
    Screen_Copy_Front();
    UI_Present_Frame();
    Palette_Load(4, -1);
    Set_Draw_Page();
    Mastery_Started_Draw();
    UI_Present_Frame();
    Palette_Fade_From_Black();
    UI_Set_Redraw_Fn(Mastery_Started_Draw, 2);
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    UI_Clear_Controls();
    control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    combat_loc_x = 0;
    while (combat_loc_x < 200 && UI_Get_Input() != control) {
        if (combat_loc_x == 17)
            Anim_Load_To_EMS("spellscr", 0x44);
        else if (combat_loc_x == 33)
            Anim_Load_To_EMS("spellscr", 0x45);
        else if (combat_loc_x == 49)
            Anim_Load_To_EMS("spellscr", 0x46);
        Timer_Mark();
        Set_Draw_Page();
        Mastery_Started_Draw();
        UI_Present_Frame();
        Wait_Ticks(3);
    }
    Sound_Stop();
    Music_Play_Current();
    Palette_Fade_To_Black();
    Palette_Load(0, -1);
    Minimap_Alloc_Pic();
    UI_Set_Redraw_Fn(Map_Redraw, 2);
    Mouse_Set_Cursor_Regions(1, player != PLAYER_HUMAN ? &mouse_region_busy : &mouse_region_arrow);
    Set_Draw_Page();
    Map_Redraw();
    UI_Present_Frame();
    Screen_Copy_Front();
    Palette_Fade_From_Black();
}

/* () the win: WIZLAB.LBX 0x13 on both pages, SPLMASTR.LBX 0x1d, the caster's portrait (14 + portrait) and 0x1c */
void Mastery_Win_Load(void)
{
    Set_Draw_Page();
    mastery_lab_pic = LBX_Reload("wizlab", 0x13, p_scratch);
    Draw_Pic_Frame(0, 0, mastery_lab_pic);
    Screen_Save_A800();
    Set_Draw_Page_Front();
    wizlab_top_pic = LBX_Reload("splmastr", 0x1d, p_scratch);
    summon_anim_pic = LBX_Reload("splmastr", wizards[mastery_caster].portrait + 14, p_world_block);
    mastery_twinkle_pic = LBX_Reload_Next("splmastr", 0x1c, p_world_block);
}

/* () a frame of the win: the caster's casting loop (60 frames per rival) and the EMS animation, with a twinkle for the
   first 9 frames of each loop */
void Mastery_Win_Draw(void)
{
    Screen_Restore_A800();
    Draw_Pic_Frame(90, 0, wizlab_top_pic);
    if (cast_anim_frame > 6) {
        if ((cast_anim_frame - 6) % 60 < 10)
            Set_Pic_Frame(summon_anim_pic, (cast_anim_frame - 6) % 60 / 2);
        else if ((cast_anim_frame - 6) % 60 > 50)
            Set_Pic_Frame(summon_anim_pic, (60 - (cast_anim_frame - 6) % 60) / 2);
        else
            Set_Pic_Frame(summon_anim_pic, 4);
        Draw_Pic_Frame(69, 75, summon_anim_pic);
        if (cast_anim_frame < (num_players - 1) * 60 + 6) {
            Anim_EMS_Next_Frame();
            if ((cast_anim_frame - 6) % 60 < 9)
                Draw_Pic_Frame(95, 55, mastery_twinkle_pic);
        }
    } else {
        Pic_Reset_Frame(summon_anim_pic);
        Draw_Pic_Frame(69, 75, summon_anim_pic);
    }
}

/* (caster) the game ends at once: an AI caster -> Screen_Mastery_Lose (which never returns); the human -> each
   rival's portrait struck in turn (those casting Spell of Return are skipped), then Screen_Win, the Hall of Fame and
   MAGIC.EXE */
void Spell_Of_Mastery_Complete(int caster)
{
    int control;
    int i;
    int end_frame;
    register int wizard;

    mastery_winner = caster;
    Combat_Tmp_Save();
    UI_Suspend_Redraw_Fn();
    if (caster != PLAYER_HUMAN)
        Screen_Mastery_Lose();
    cast_anim_frame = 0;
    mastery_caster = caster;
    Sound_Stop();
    music_jingle = LBX_Reload("music", MUSIC_MASTERY, p_music_buf);
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    Palette_Fade_To_Black();
    Mastery_Win_Load();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Screen_Copy_Front();
    Set_Draw_Page();
    Mastery_Win_Draw();
    UI_Present_Frame();
    Palette_Fade_From_Black();
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    UI_Clear_Controls();
    control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    end_frame = (num_players - 1) * MASTERY_RIVAL_FRAMES + 2 * MASTERY_LEAD_FRAMES;
    for (i = 1; i < num_players; i++) {
        if (wizards[i].casting_spell == SPELL_SPELL_OF_RETURN)
            end_frame -= MASTERY_RIVAL_FRAMES;
    }
    wizard = 0;
    for (cast_anim_frame = 0; cast_anim_frame < end_frame && UI_Get_Input() != control; cast_anim_frame++) {
        if ((cast_anim_frame - MASTERY_LEAD_FRAMES) % MASTERY_RIVAL_FRAMES == 0) {
            while (wizards[++wizard].casting_spell == SPELL_SPELL_OF_RETURN)
                ;
            Anim_Load_To_EMS("splmastr", wizards[wizard].portrait);
            Pic_Reset_Frame(mastery_twinkle_pic);
        }
        if (end_frame - MASTERY_LEAD_FRAMES == cast_anim_frame) {
            Heap_Release(p_scratch);
            Heap_Mark(p_scratch);
            wizlab_top_pic = LBX_Reload("splmastr", 0x1f, p_scratch);
        }
        if (cast_anim_frame == 7)
            wizlab_top_pic = LBX_Reload("splmastr", 0x1e, p_scratch);
        Timer_Mark();
        Set_Draw_Page();
        Mastery_Win_Draw();
        UI_Present_Frame();
        Wait_Ticks(3);
    }
    Sound_Stop();
    Music_Play_Current();
    Palette_Fade_To_Black();
    Palette_Load(0, -1);
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    UI_Present_Frame();
    Screen_Copy_Front();
    Palette_Fade_From_Black();
    Combat_Tmp_Restore();
    Screen_Win(0);
    Screen_Final_Score();
    Quit_Hook_Empty();
    Exec_Program("MAGIC.EXE", "JENNY", "", "");
}

/* (creator) the "rise from the dead" pictures: CMBTFX.LBX 0x1b / 0x1d / 0x1e / 0x1f and the risen unit's MONSTER.LBX
   picture (UNDEAD_FROM_ITEM: the item picture, below it: 0x2f, else type - the Magic Spirit's unit type) */
void Undead_Rise_Load(int creator)
{
    Heap_Mark(p_scratch);
    undead_rise_bg_pic = LBX_Reload_Next("cmbtfx", 0x1b, p_scratch);
    undead_rise_rocks_pic = LBX_Reload_Next("cmbtfx", 0x1d, p_scratch);
    undead_rise_back_pic = LBX_Reload_Next("cmbtfx", 0x1e, p_scratch);
    undead_rise_front_pic = LBX_Reload_Next("cmbtfx", 0x1f, p_scratch);
    if (creator > UNDEAD_FROM_ITEM) {
        undead_rise_unit_pic = LBX_Reload_Next("Monster", creator - (int)p_spell_data[SPELL_MAGIC_SPIRIT].effect_param,
                                               p_scratch);
    } else if (creator == UNDEAD_FROM_ITEM) {
        undead_rise_unit_pic = LBX_Reload_Next("Monster", MONSTER_PIC_ITEM, p_scratch);
    } else {
        undead_rise_unit_pic = LBX_Reload_Next("Monster", 0x2f, p_scratch);
    }
}

/* () a frame of the rise: the pictures slide up over 40 frames (clipped at y 123), then stay; the text below */
void Undead_Rise_Draw(void)
{
    int frames;
    int x;
    int y;

    x = 70;
    y = 14;
    frames = 40;
    Screen_Restore_A800();
    Draw_Pic_Frame(x, y, undead_rise_bg_pic);
    Set_Clip_Rect(0, 0, 319, y + 109);
    if (cast_anim_frame < frames) {
        Draw_Pic_Clipped(x, y + 17 - cast_anim_frame * 17 / frames, undead_rise_back_pic);
        Draw_Pic_Clipped(x, y + 40 - cast_anim_frame * 40 / frames, undead_rise_front_pic);
        Draw_Pic_Frame(x, y, undead_rise_rocks_pic);
        Draw_Pic_Clipped(x + 78, y + 110 - cast_anim_frame * 80 / frames, undead_rise_unit_pic);
    } else {
        Draw_Pic_Frame(x, y, undead_rise_back_pic);
        Draw_Pic_Frame(x, y, undead_rise_front_pic);
        Draw_Pic_Frame(x + 78, y + 30, undead_rise_unit_pic);
    }
    Reset_Clip_Rect();
    Set_Text_Shadow_Color(16);
    Font_Select_Shadow(4, 4, 0, 0);
    Set_Font_Color(190);
    Paragraph_Print(x + 7, y + 114, 165, message_text, PARA_CENTER);
}

/* (creator) Combat_End's "rise from the dead" screen: 100 frames or until a click */
void Screen_Undead_Rise(int creator)
{
    int input;
    int control;
    int done;

    Undead_Rise_Load(creator);
    cast_anim_frame = 0;
    UI_Set_Redraw_Fn(Undead_Rise_Draw, 2);
    UI_Clear_Controls();
    control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    done = 0;
    for (cast_anim_frame = 0; cast_anim_frame < 100 && done == 0; cast_anim_frame++) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input == control)
            done = 1;
        if (done == 0) {
            Set_Draw_Page();
            Undead_Rise_Draw();
            Screen_Present();
        }
        Wait_Ticks(2);
    }
    Heap_Release(p_scratch);
}
