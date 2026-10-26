/* INTRO.C - MAGIC.EXE code segment 557e (MAGIC only): the intro animation - 13 scenes from INTRO.LBX played through
   EMS, the copyright line, the scene sound effects; a click or Esc skips it.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 557e src/magic/INTRO.C */
#include "STRUCTS.H"
#include "ENUMS.H"

/* the intro's scenes: NUM_INTRO_SCENES (ENUMS.H) */
#define MUSIC_INTRO 0x70                /* MUSIC.LBX "MOM49" */
#define SFX_INTRO_TORAN1 0x73           /* SOUNDFX.LBX "TORAN1" */
#define SFX_INTRO_MERLIN 4              /* "MERLIN1", intro speech */
#define SFX_INTRO_SCENE6 0x74           /* past SOUNDFX.LBX's name table: no label */
#define SFX_INTRO_TORAN2 1              /* "TORAN2M", intro speech */
#define SFX_LIGHTNING_BOLT 0x1b         /* "LITBOLT" */
#define SFX_INTRO_SCENE9 0x76           /* no label */
#define SFX_INTRO_TORAN3 3              /* "TORAN3M1", intro speech */

/* the module's functions in the order the original declared them (= its overlay stub's thunk order;
   link test, batch 36) */
void Intro_Play(void);
void Intro_Fade_Out_2_Steps(void);
void Intro_Fade_Out_20_Steps(void);
void Intro_Fade_Out_10_Steps(void);
void Intro_Fade_In_3_Steps(void);
void Intro_Fade_In_51_Steps(void);
#pragma warn -pro

extern struct MouseRegion mouse_region_hidden;  /* DS:1f60 */
extern unsigned p_scratch;                      /* DS:61ae */
extern struct GameConfig game_config;           /* DS:87d4 */


int intro_frames[NUM_INTRO_SCENES] = { 15, 12, 30, 18, 25, 32, 24, 32, 30, 37, 30, 11, 32 };     /* DS:385e  frames per scene */

/* () plays the intro (MAGIC.EXE skips it when started back from the game) */
void Intro_Play(void)
{
    int click_control;
    int input;
    int skip;
    int music;
    int sfx_scene8_10;
    int scene_sfx;                              /* the current scene's sound: each load names its SFX_INTRO_* entry */
    int sfx_scene6;
    int esc_control;
    unsigned char colors[2];
    int scene;
    int frame;

    colors[0] = 0x4f;                           /* the copyright line's colour slot: entries 0 and 1, palette indices of
                                                   the intro's palette (Font_Select_Palette copies 16 entries, so the
                                                   other 14 come from the stack beyond colors[]) */
    colors[1] = 0xa3;
    music = LBX_Reload("music.lbx", MUSIC_INTRO, p_scratch);
    Heap_Mark(p_scratch);
    scene_sfx = LBX_Reload_Next("soundfx.lbx", SFX_INTRO_TORAN1, p_scratch);
    if (game_config.music == 1)
        Sound_Play(music);
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
    UI_Clear_Controls();
    click_control = UI_Add_Region(0, 0, 319, 199, "", -1);
    esc_control = UI_Add_Hotkey("\x1b");
    for (scene = 0; scene < NUM_INTRO_SCENES; scene++) {
        Anim_Load_To_EMS("INTRO.LBX", scene);
        skip = 0;
        if (scene == 0 || scene > 3) {
            Intro_Fade_Out_2_Steps();
        } else {
            if (scene == 3)
                Intro_Fade_Out_20_Steps();
            if (scene == 1)
                Intro_Fade_Out_20_Steps();
            if (scene == 2)
                Intro_Fade_Out_10_Steps();
        }
        for (frame = 0; intro_frames[scene] > frame; frame++) {
            Timer_Mark();
            input = UI_Get_Input();
            if (input == click_control || input == esc_control)
                skip = 1;
            Set_Draw_Page();
            if (frame == 0)
                Fill_Rect(0, 0, 319, 199, 0);
            Anim_EMS_Next_Frame();
            if (scene == 2 && frame > 6) {
                Font_Select_Palette(0, colors);
                Print_Text_Centered(160, 192, "Copyright  Simtex Software, 1995   V1.31");
            }
            if (scene == 4 && frame == 0 && game_config.sound_effects == 1)
                Sound_Play(scene_sfx);
            if (scene == 5 && frame == 10) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                scene_sfx = LBX_Reload_Next("soundfx.lbx", SFX_INTRO_MERLIN, p_scratch);
                sfx_scene6 = LBX_Reload_Next("soundfx.lbx", SFX_INTRO_SCENE6, p_scratch);
                if (game_config.sound_effects == 1)
                    Sound_Play(scene_sfx);
            }
            if (scene == 6 && frame == 0 && game_config.sound_effects == 1)
                Sound_Play(sfx_scene6);
            if (scene == 7 && frame == 2) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                scene_sfx = LBX_Reload_Next("soundfx.lbx", SFX_INTRO_TORAN2, p_scratch);
                sfx_scene8_10 = LBX_Reload_Next("soundfx.lbx", SFX_LIGHTNING_BOLT, p_scratch);
                if (game_config.sound_effects == 1)
                    Sound_Play(scene_sfx);
            }
            if (scene == 9 && frame == 0) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                scene_sfx = LBX_Reload_Next("soundfx.lbx", SFX_INTRO_SCENE9, p_scratch);
                sfx_scene8_10 = LBX_Reload_Next("soundfx.lbx", SFX_LIGHTNING_BOLT, p_scratch);
                if (game_config.sound_effects == 1)
                    Sound_Play(scene_sfx);
            }
            if (scene == 11 && frame == 2) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                scene_sfx = LBX_Reload_Next("soundfx.lbx", SFX_INTRO_TORAN3, p_scratch);
                if (game_config.sound_effects == 1)
                    Sound_Play(scene_sfx);
            }
            if (scene == 8 && frame == 3 && game_config.sound_effects == 1)
                Sound_Play(sfx_scene8_10);
            if (scene == 8 && frame == 9 && game_config.sound_effects == 1)
                Sound_Play(sfx_scene8_10);
            if (scene == 10 && frame == 2 && game_config.sound_effects == 1)
                Sound_Play(sfx_scene8_10);
            UI_Present_Frame();
            if (frame == 0 && scene > 2)
                Intro_Fade_In_3_Steps();
            else if (frame == 0 && (scene == 2 || scene == 0 || scene == 1))
                Intro_Fade_In_51_Steps();
            Screen_Copy_Front();
            Wait_Ticks(3);
            if (skip == 1)
                frame = intro_frames[scene];
        }
        if (skip == 1)
            scene = NUM_INTRO_SCENES;
    }
}

/* () the screen to black in 2 steps (a local copy of Palette_Fade_To_Black) */
void Intro_Fade_Out_2_Steps(void)
{
    int percent;

    for (percent = 50; percent < 101; percent += 50) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () the same in 10 steps */
void Intro_Fade_Out_10_Steps(void)
{
    int percent;

    for (percent = 10; percent < 101; percent += 10) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () the same in 20 steps */
void Intro_Fade_Out_20_Steps(void)
{
    int percent;

    for (percent = 5; percent < 101; percent += 5) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () from black in 3 steps (a local copy of Palette_Fade_From_Black) */
void Intro_Fade_In_3_Steps(void)
{
    int percent;

    for (percent = 100; percent >= 0; percent -= 50) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () from black in 51 steps */
void Intro_Fade_In_51_Steps(void)
{
    int percent;

    for (percent = 100; percent >= 0; percent -= 2) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}
