/* CASTANIM.C - WIZARDS.EXE overlay code segment be86 (stub 25d9): the spell-completion screens - the summoning circle
   ("<unit> Summoned", hero / champion / item), a city spell over the zoomed cityscape (Earthquake shakes it; the
   hotspots name the buildings), a global enchantment taking hold over the caster's portrait, and the wizard-target
   screen (Spell Blast, Cruel Unminding, Drain Power, Subversion: pick a rival, the effect, its animation).
   This module's data (DS:6c4a..6dc9): its literals; BSS (DS:ca32..ca57): the pictures of these screens and the
   dialog words the casting screens share (cast_ui_done .. cast_result_shown, mastery_winner; ca46..ca4b
   never referenced; owner not fixed by the EXE).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS be86 src/wizards/CASTANIM.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Summon_Anim_Load(int summoned, int wizard);
void Summon_Anim_Redraw(void);
void Summon_Complete_Anim(int summoned, int music_tier, int wizard);
void CitySpell_Anim_Load(int spell, int wizard);
void CitySpell_Cityscape_Redraw(void);
int CitySpell_Cast_Anim(int city, int spell, int wizard);
void CitySpell_View_Loop(int city, int spell);
void GlobalEnch_Anim_Load(int spell, int wizard);
void GlobalEnch_Anim_Redraw(void);
void GlobalEnch_Complete_Anim(int wizard, int spell);
void WizTarget_Screen_Load(int spell);
void WizTarget_Screen_Redraw(void);
int Screen_Wizard_Target(int spell);

#include "GAME.H"

int mastery_winner;                             /* DS:ca56  the player who cast the Spell of Mastery, -1 none (MASTERY.C) */
unsigned summon_anim_pic;                       /* DS:ca54  SPELLSCR.LBX portrait + 0x2e (SUMMON00 .. 13): the caster */
int cast_result_shown;                          /* DS:ca52  CASTUI.C: 1 while the Disjunction / Binding result shows */
                                                /* here: city_spell_scape_object, the cityscape object the city spell shows */
int cast_ui_slot;                               /* DS:ca50  the dialog's slot, city or wizard */
                                                /* also: the success flag of Spell_Binding_Resolve /
                                                   Disjunction_Resolve */
int cast_screen_live;                            /* DS:ca4e */
                                                /* 1 while a casting screen's live part is drawn: the wizard-target
                                                   and Disjunction choice, CitySpell_Cast_Anim's scene */
int cast_ui_done;                          /* DS:ca4c */
                                                /* the done flag of every casting screen: power dialog, summoning,
                                                   global enchantment, wizard target, Disjunction */
int castanim_unused_ca46[3];                    /* DS:ca46  never referenced */
unsigned globalench_anim_pic;                   /* DS:ca44  MOODWIZ.LBX portrait */
unsigned forge_icon_prev_pic;                   /* DS:ca42 */
unsigned forge_icon_next_pic;                   /* DS:ca40 */
unsigned summon_flame_pic;                      /* DS:ca3e  SPELLSCR.LBX 10 */
unsigned summon_bg_pic;                         /* DS:ca3c  SPELLSCR.LBX 9 */
unsigned summon_flame_frame_pic;                /* DS:ca3a  SPELLSCR.LBX 11 */
unsigned summon_monster_pic;                    /* DS:ca38  MONSTER.LBX */
unsigned ui_temp_pic_b;                         /* DS:ca36 */
unsigned ui_temp_pic_a;                         /* DS:ca34 */
unsigned wiztarget_bg_pic;                      /* DS:ca32  SPELLSCR.LBX 0 (0x48 for Spell Blast) */

extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */
extern struct MouseRegion mouse_region_busy;    /* DS:0106 */
extern char *city_size_names[];                 /* DS:0156 */
extern char *cityscape_special_names[];         /* DS:0162  Summoning Circle .. Altar of Battle (hotspots 100..106); ReMoM _magic_building_names, clarified using ReMoM repo */
extern char *relation_names[];                  /* DS:35a4 */
extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern unsigned music_jingle;                   /* DS:9134 */
extern struct CityscapeHotspot far *p_cityscape_hotspots;  /* DS:92b0 */
extern int cityscape_hotspot_count;             /* DS:92b4 */
extern struct Building far *p_building_data;    /* DS:938c */
extern unsigned p_music_buf;                    /* DS:9390 */
extern unsigned cityspell_bg_pic;               /* DS:946c */
extern unsigned magic_gem_pics[];               /* DS:958e */
extern unsigned magic_gem_frame_pic;            /* DS:95c0 */
extern unsigned magic_overlay_pic;              /* DS:95c2 */
extern unsigned diplo_mirror_pic;               /* DS:9918 */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int cityscape_hotspot_shown;             /* DS:bf8e */
extern int *city_hotspot_controls;              /* DS:bf90 */
extern int magic_gem_wizard[];                  /* DS:c20c */
extern int cast_power_total;                    /* DS:ca12 */
                                                /* here a frame counter (GlobalEnch_Complete_Anim); CASTUI.C: the extra
                                                   power, then the total cost */
extern int cast_anim_frame;                     /* DS:ca14 */
extern int ui_spell;                            /* DS:ca16 */
extern unsigned castui_frame_pic;               /* DS:ca26 */
extern int cast_ui_caster;                      /* DS:ca28 */
                                                /* the caster in CASTUI.C; here a picture handle (the alias below) */
extern unsigned wiztarget_button_pic;           /* DS:ca2a */
extern int disjunction_choice;                  /* DS:ca2c */
                                                /* one word, three roles here (the aliases below); MASTERY.C: the
                                                   SPELLSCR animation entry */
#define summon_kind disjunction_choice           /* here: the summoning screen's kind (> 0 a unit type, -1 an
                                                   item, > -10 a hero, else a champion) */
#define cast_hotspot disjunction_choice          /* and the hotspot under the mouse (the city-spell and
                                                   wizard-target screens) */
#define global_ench_anim_pic cast_ui_caster      /* here: the global-enchantment animation picture */
#define global_ench_frame cast_power_total       /* and its frame (0 .. 119) */
#define city_spell_scape_object cast_result_shown   /* here: the cityscape object the city spell shows */

/* Heap_Release has no prototype here: Screen_Wizard_Target calls it without its argument */
void Heap_Release();
int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Heap_Mark(unsigned block);
void Heap_Reset(unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Near_Alloc_Mark(void);
void Near_Alloc_Release(void);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Pic_Apply_Palette(unsigned pic);
void Pic_Reset_Frame(unsigned pic);
int Pic_Cut_To_Mask(int x, int y, unsigned dst, unsigned mask);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Draw_Page_Front(void);
void Screen_Copy_Front(void);
void Screen_Restore_AC00(void);
void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
void Map_Redraw_Dissolve(void);
void Map_Full_Redraw(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
int UI_Hit_Test(void);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void UI_Set_Help(struct HelpEntry *help, int count);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
void Set_Line_Spacing(int line_gap);
int Print_Text_Centered(int x, int y, char *text);
void Print_Far_Text_Centered(int x, int y, char far *text);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Music_Play_Current(void);
void Music_Load_Realm(int spell, int base);
void Spell_Load_Sound(int spell);
int Spell_Load_Anim(int spell);
void Play_Click_Sound(void);
void Warning_Box(char *text);
void Cityscape_Draw(int city, int x, int y, int new_obj, int fade_obj);
void Cityscape_Reveal_Reset(void);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
void Window_Zoom_Open_In_Place(int x1, int y1, int x2, int y2, int src_x1, int src_y1, int src_x2, int src_y2, int lines);
void Transition_Wipe_Left(int x, int y);
void Screen_Wizard_Info(int wizard, int from_x1, int from_y1, int from_x2, int from_y2);
int Spell_Casting_Cost(int player, int spell, int in_combat);
int Cruel_Unminding_Apply(int wizard);
int Drain_Power_Apply(int player);
void Subversion_Apply(int victim);

/* (summoned: unit type > 0, or SUMMON_*: -1 item, -2 / -3 male / female hero, -20 / -30 male / female champion; wizard) the summoning circle's pictures: the
   circle over the map (palette of the summoned unit's race / realm), the flames, the summoned figure and the caster */
void Summon_Anim_Load(int summoned, int wizard)
{
    unsigned pic;
    int entry;

    if (summoned < 0)
        entry = 15;                             /* an item / hero / champion: SPELLSCR.LBX 0x3d */
    else
        entry = unit_types[summoned].race;
    entry = entry + 0x2e;                       /* SPELLSCR.LBX 0x2e + race: the realm races 15 .. 20 reach the coloured
                                                   circles 0x3d .. 0x42 (SUMMGREY .. SUMMPURP); 0x2e .. 0x3b (SUMMON00 .. 13)
                                                   are the 14 wizard portraits, summon_anim_pic's */
    Heap_Mark(p_scratch);
    summon_bg_pic = LBX_Reload_Next("Spellscr", 9, p_scratch);
    pic = LBX_Reload_Next("Spellscr", entry, p_scratch);
    Pic_Apply_Palette(pic);
    Set_Draw_Page();
    Map_Redraw();
    Draw_Pic_Frame(30, 42, summon_bg_pic);
    Screen_Save_A800();
    Heap_Release(p_scratch);
    Heap_Mark(p_scratch);
    summon_flame_pic = LBX_Reload_Next("Spellscr", 10, p_scratch);
    summon_flame_frame_pic = LBX_Reload_Next("Spellscr", 11, p_scratch);
    if (summoned > 0)
        summon_monster_pic = LBX_Reload_Next("Monster", summoned - (int)p_spell_data[SPELL_MAGIC_SPIRIT].effect_param,
                                             p_scratch);
    else {
        if (summoned == SUMMON_ITEM)
            summon_monster_pic = LBX_Reload_Next("Monster", MONSTER_PIC_ITEM, p_scratch);
        if (summoned == SUMMON_HERO_MALE || summoned == SUMMON_CHAMPION_MALE)
            summon_monster_pic = LBX_Reload_Next("Monster", MONSTER_PIC_MALE, p_scratch);
        if (summoned == SUMMON_HERO_FEMALE || summoned == SUMMON_CHAMPION_FEMALE)
            summon_monster_pic = LBX_Reload_Next("Monster", MONSTER_PIC_FEMALE, p_scratch);
    }
    summon_anim_pic = LBX_Reload_Next("Spellscr", wizards[wizard].portrait + 0x2e, p_scratch);
    pic = LBX_Reload_Next("Spellscr", entry, p_scratch);
    Pic_Apply_Palette(pic);
    Palette_Upload();
}

/* () a frame of the summoning: the caster, the flames, the figure rising out of the circle (with spell
   animations, and only for the human's summons: cast_ui_slot 0),
   the title */
void Summon_Anim_Redraw(void)
{
    int x;
    int y;

    x = 30;
    y = 42;
    Screen_Restore_A800();
    Set_Clip_Rect(x + 7, y + 8, 319, 199);
    Draw_Pic_Clipped(x + 8, y + 3, summon_anim_pic);
    Draw_Pic_Frame(x + 0x37, y + 0x36, summon_flame_pic);
    Set_Clip_Rect(0, 0, 319, y + 0x69);
    if (cast_anim_frame < 30 && game_config.spell_animations == 1 && cast_ui_slot == 0)
        Draw_Pic_Clipped(x + 0x4c, y + 0x6e - cast_anim_frame * 3, summon_monster_pic);
    else
        Draw_Pic_Clipped(x + 0x4c, y + 0x12, summon_monster_pic);
    Draw_Pic_Frame(x + 0x40, y + 0x50, summon_flame_frame_pic);
    Set_Text_Shadow_Color(16);
    Font_Select_Shadow(4, 4, 0, 0);
    Set_Font_Color(0xbe);
    if (summon_kind > 0) {
        strcpy(message_text, unit_types[(int)p_spell_data[wizards[cast_ui_slot].casting_spell].effect_param].name);
        strcat(message_text, " Summoned");
    } else if (summon_kind == SUMMON_ITEM)
        strcpy(message_text, "Item Enchanted");
    else if (summon_kind > -10)                 /* the heroes; -20 / -30 the champions */
        strcpy(message_text, "Hero Summoned");
    else
        strcpy(message_text, "Champion Summoned");
    Print_Text_Centered(x + 0x5a, y + 0x75, message_text);
}

/* (summoned, music tier, wizard) the summoning screen: its jingle, 130 frames with spell animations (40 without) or a
   click; the busy or arrow cursor afterwards */
void Summon_Complete_Anim(int summoned, int music_tier, int wizard)
{
    int control;
    int input;
    int frames;

    Sound_Stop();
    music_jingle = LBX_Reload("Music", music_tier + MUSIC_SUMMON_BASE, p_music_buf);   /* MUSIC 13 .. 16: the summoning jingle
                                                   by tier (the spell's rarity; 3 for heroes and items) */
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    cast_anim_frame = 0;
    cast_ui_slot = wizard;
    summon_kind = summoned;
    Minimap_Alloc_Pic();
    Map_Full_Redraw();
    Summon_Anim_Load(summoned, wizard);
    UI_Set_Redraw_Fn(Summon_Anim_Redraw, 2);
    present_mode = PRESENT_DISSOLVE;
    UI_Clear_Controls();
    control = UI_Add_Region(0, 0, 319, 199, "", -1);
    if (game_config.spell_animations == 1)
        frames = 130;
    else
        frames = 40;
    cast_ui_done = 0;
    cast_anim_frame = 0;
    while (cast_anim_frame < frames && cast_ui_done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input == control)
            cast_ui_done = 1;
        Set_Draw_Page();
        Summon_Anim_Redraw();
        Screen_Present();
        Wait_Ticks(2);
        present_mode = PRESENT_NORMAL;
        cast_anim_frame++;
    }
    UI_Suspend_Redraw_Fn();
    Map_Redraw_Dissolve();
    Sound_Stop();
    Heap_Release(p_scratch);
    Music_Play_Current();
    Mouse_Set_Cursor_Regions(1, wizard == 0 ? &mouse_region_arrow : &mouse_region_busy);
}

/* (spell, wizard) the city-spell frame (SPELLSCR.LBX 0x49): "<size> of <city>", "You cast <spell>" / "<name> casts
   <spell>"; the hotspot control list */
void CitySpell_Anim_Load(int spell, int wizard)
{
    char name[26];

    Screen_Copy_Front();
    cityspell_bg_pic = LBX_Reload("spellscr", 0x49, p_scratch);
    Set_Draw_Page();
    Draw_Pic_Frame(0x35, 0x26, cityspell_bg_pic);
    Set_Text_Shadow_Color(17);
    Font_Select_Shadow(5, 5, 0, 0);
    strcpy(message_text, city_size_names[p_cities[cast_ui_slot].size]);
    strcat(message_text, " of ");
    far_strcpy(name, 0, p_cities[cast_ui_slot].name);
    strcat(message_text, name);
    Print_Text_Centered(160, 44, message_text);
    Set_Text_Shadow_Color(17);
    Font_Select_Shadow(4, 4, 0, 0);
    Set_Font_Color(24);
    if (wizard == 0)
        strcpy(message_text, "You cast ");
    else {
        strcpy(message_text, wizards[wizard].wizard_name);
        strcat(message_text, " casts ");
    }
    far_strcpy(name, 0, p_spell_data[spell].name);
    strcat(message_text, name);
    Print_Text_Centered(160, 164, message_text);
    Screen_Save_A800();
    Near_Alloc_Mark();
    city_hotspot_controls = (int *)Near_Alloc_Reset(0x90);
}

/* () a frame of the city spell: the cityscape and the name of the hotspot under the mouse (a destroyed building gets
   "Destroyed" above its name) */
void CitySpell_Cityscape_Redraw(void)
{
    int x;
    int y;
    int i;

    x = 0x35;
    y = 0x26;
    Screen_Restore_A800();
    Cityscape_Draw(cast_ui_slot, x + 5, y + 0x1b, city_spell_scape_object, city_spell_scape_object);
    for (i = 0; i < cityscape_hotspot_shown; i++) {
        if (city_hotspot_controls[i] == cast_hotspot) {
            Font_Select_Outline(1, 0, 0, 0);
            Set_Font_Color(8);
            if (p_cityscape_hotspots[i].building >= SCAPE_SUMMONING_CIRCLE)
                Print_Text_Centered(p_cityscape_hotspots[i].label_x, p_cityscape_hotspots[i].label_y,
                                    cityscape_special_names[p_cityscape_hotspots[i].building - 100]);
            else if (p_cities[cast_ui_slot].buildings[p_cityscape_hotspots[i].building] == BLDSTATE_REMOVED) {
                Font_Select_Outline(1, 2, 0, 0);
                Set_Font_Color(0xa6);
                Print_Text_Centered(p_cityscape_hotspots[i].label_x, p_cityscape_hotspots[i].label_y, "Destroyed");
                Print_Far_Text_Centered(p_cityscape_hotspots[i].label_x, p_cityscape_hotspots[i].label_y + 6,
                                        p_building_data[p_cityscape_hotspots[i].building].name);
            } else
                Print_Far_Text_Centered(p_cityscape_hotspots[i].label_x, p_cityscape_hotspots[i].label_y,
                                        p_building_data[p_cityscape_hotspots[i].building].name);
        }
    }
}

/* (city, spell, wizard) -> 0 without spell animations: a city spell - its realm jingle, the cityscape effect of the
   spell, the frame, then the zoom into the cityscape and (Earthquake) the shaking */
int CitySpell_Cast_Anim(int city, int spell, int wizard)
{
    int x;
    int y;
    int music;
    int i;

    Sound_Stop();
    if (p_spell_data[spell].effect_type == SPELL_FX_CITY_CURSE)   /* Music_Load_Realm's bases: MUSIC 6 .. 11 the
                                                   curses, 0 .. 5 the enchantments (+ 0 arcane, 1 chaos, 2 death, 3 life,
                                                   4 nature, 5 sorcery) */
        music = 6;
    else
        music = 0;
    Music_Load_Realm(spell, music);
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    cast_screen_live = 1;
    cast_ui_slot = city;
    cast_hotspot = UI_NO_CONTROL;
    switch (spell) {
    case SPELL_MOVE_FORTRESS:
    case SPELL_SPELL_OF_RETURN:
        city_spell_scape_object = SCAPE_FORTRESS;
        break;
    case SPELL_EARTH_GATE:
        city_spell_scape_object = SCAPE_EARTH_GATE;
        break;
    case SPELL_STREAM_OF_LIFE:
        city_spell_scape_object = SCAPE_STREAM_OF_LIFE;
        break;
    case SPELL_ASTRAL_GATE:
        city_spell_scape_object = SCAPE_ASTRAL_GATE;
        break;
    case SPELL_SUMMONING_CIRCLE:
        city_spell_scape_object = SCAPE_SUMMONING_CIRCLE;
        break;
    case SPELL_DARK_RITUALS:
        city_spell_scape_object = SCAPE_DARK_RITUALS;
        break;
    case SPELL_ALTAR_OF_BATTLE:
        city_spell_scape_object = SCAPE_ALTAR_OF_BATTLE;
        break;
    default:
        city_spell_scape_object = 0;
        break;
    }
    CitySpell_Anim_Load(spell, wizard);
    Heap_Reset(p_scratch);
    UI_Set_Redraw_Fn(CitySpell_Cityscape_Redraw, 2);
    cast_screen_live = 0;
    Set_Draw_Page();
    CitySpell_Cityscape_Redraw();
    Screen_Save_AC00();
    cast_screen_live = 1;
    Set_Draw_Page();
    CitySpell_Cityscape_Redraw();
    cast_screen_live = 0;
    if (game_config.spell_animations == 0)
        return 0;
    else {
        x = p_cities[city].x;
        y = p_cities[city].y;
        Map_Square_To_Screen(map_view_x, map_view_y, &x, &y);
        Window_Zoom_Open_In_Place(0x35, 0x26, 0x10c, 0xa2, x, y, x + 20, y + 18, 0);
        Set_Draw_Page();
        Screen_Restore_AC00();
        Screen_Save_A800();
        Screen_Present();
        if (spell == SPELL_EARTHQUAKE) {
            Spell_Load_Sound(spell);
            Set_Draw_Page();
            CitySpell_Cityscape_Redraw();
            Screen_Save_A800();
            if (p_sfx_buf != -1)
                Sound_Play(p_sfx_buf);
            for (i = 35; i > 5; i--) {
                Transition_Wipe_Left(Random(i) / 5, Random(i) / 5);
                Screen_Present();
            }
            Set_Draw_Page();
            CitySpell_Cityscape_Redraw();
            Screen_Present();
            p_sfx_buf = -1;
        } else {
            for (i = 0; i < 8; i++) {
                Set_Draw_Page();
                CitySpell_Cityscape_Redraw();
                Screen_Present();
            }
        }
    }
    Near_Alloc_Release();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Cityscape_Reveal_Reset();
}

/* (city, spell) the cityscape after a city spell for up to 250 frames or a click, naming the hotspot under the mouse */
void CitySpell_View_Loop(int city, int spell)
{
    int control;
    int x1;
    int x2;
    int y1;
    int y2;
    int i;

    cast_ui_slot = city;
    if (spell != SPELL_EARTHQUAKE) {
        Heap_Reset(p_scratch);
        Spell_Load_Sound(spell);
    }
    city_spell_scape_object = 0;
    present_mode = PRESENT_DISSOLVE;
    Set_Draw_Page();
    CitySpell_Cityscape_Redraw();
    Screen_Present();
    if (spell != SPELL_EARTHQUAKE && (int)p_sfx_buf > -1)
        Sound_Play(p_sfx_buf);
    UI_Set_Redraw_Fn(CitySpell_Cityscape_Redraw, 2);
    UI_Clear_Controls();
    cityscape_hotspot_shown = 0;
    for (i = 0; i < cityscape_hotspot_count; i++) {
        x1 = p_cityscape_hotspots[i].x1;
        y1 = p_cityscape_hotspots[i].y1;
        x2 = p_cityscape_hotspots[i].x2;
        y2 = p_cityscape_hotspots[i].y2;
        city_hotspot_controls[cityscape_hotspot_shown] = UI_Add_Region(x1, y1, x2, y2, "", -1);
        cityscape_hotspot_shown++;
    }
    control = UI_Add_Region(0, 0, 319, 199, "", -1);
    for (i = 0; i < 250 && UI_Get_Input() == 0; i++) {
        cast_hotspot = UI_Hit_Test();
        Timer_Mark();
        Set_Draw_Page();
        CitySpell_Cityscape_Redraw();
        Screen_Present();
        Wait_Ticks(2);
    }
    Near_Alloc_Release();
    Heap_Release(p_scratch);
    UI_Clear_Controls();
    Sound_Stop();
    Music_Play_Current();
    UI_Suspend_Redraw_Fn();
    Minimap_Alloc_Pic();
    Map_Redraw_Dissolve();
}

/* (spell, wizard) the global enchantment's pictures for the current stage (global_ench_frame): the caster's mood
   portrait, the mirror, the effect (SPECFX.LBX) and its mask, or the DIPLOMAC.LBX background */
void GlobalEnch_Anim_Load(int spell, int wizard)
{
    int entry;

    globalench_anim_pic = LBX_Reload_Next("moodwiz", wizards[wizard].portrait, p_scratch);
    diplo_mirror_pic = LBX_Reload_Next("BACKGRND", 18, p_scratch);
    if (spell == SPELL_DEATH_WISH)              /* the global's index (effect_param); Death Wish and Great Unsummoning
                                                   have none, Awareness (23) is moved to 41 because Death Wish takes 23 */
        entry = 23;
    else if (spell == SPELL_GREAT_UNSUMMONING)
        entry = 24;
    else if (spell == SPELL_AWARENESS)
        entry = 41;
    else
        entry = (int)p_spell_data[spell].effect_param;
    ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 0x302);
    ui_temp_pic_b = Heap_Alloc_Back(p_scratch, 0x302);
    if (global_ench_frame == 20 || game_config.spell_animations == 0) {
        p_anim_pic = LBX_Reload_Next("Specfx", entry + 15, p_scratch);   /* the globals' pictures from SPECFX 15 */
        global_ench_anim_pic = LBX_Reload_Next("Specfx", 0x36, p_scratch);
        Pic_Reset_Frame(global_ench_anim_pic);
    } else if (global_ench_frame == 30) {
        p_anim_pic = LBX_Reload_Next("Specfx", entry + 15, p_scratch);   /* the globals' pictures from SPECFX 15 */
        global_ench_anim_pic = LBX_Reload_Next("Specfx", 0x37, p_scratch);
        Pic_Reset_Frame(global_ench_anim_pic);
    } else if (global_ench_frame < 10) {
        castui_frame_pic = LBX_Reload_Next("Diplomac", 1, p_scratch);
        global_ench_anim_pic = LBX_Reload_Next("Specfx", 0x36, p_scratch);
    } else if (global_ench_frame < 20) {
        castui_frame_pic = LBX_Reload_Next("Diplomac", 1, p_scratch);
        global_ench_anim_pic = LBX_Reload_Next("Specfx", 0x37, p_scratch);
        Pic_Reset_Frame(global_ench_anim_pic);
    }
}

/* () a frame of the global enchantment: "You have completed casting..." / "<name> has cast...", the portrait dissolving
   into the effect in the mirror, then the spell's name */
void GlobalEnch_Anim_Redraw(void)
{
    int x;
    int y;

    x = 58;
    y = 28;
    if (cast_ui_done != 1) {
        Map_Redraw();
        Set_Text_Shadow_Color(0);
        Font_Select_Outline(5, 0, 0, 0);
        if (game_config.spell_animations == 1) {
            if (global_ench_frame < 34) {
                if (cast_ui_slot == 0)
                    strcpy(message_text, "You have completed casting...");
                else {
                    strcpy(message_text, wizards[cast_ui_slot].wizard_name);
                    strcat(message_text, " has cast...");
                }
                Print_Text_Centered(x + 62, y + 133, message_text);
            }
            if (global_ench_frame < 14) {
                Pic_Reset_Frame(globalench_anim_pic);
                Draw_Pic_Frame(x + 12, y + 12, globalench_anim_pic);
                Pic_Decode_Frame_To_Bitmap(global_ench_anim_pic, ui_temp_pic_a);
                Pic_Decode_Frame_To_Bitmap(castui_frame_pic, ui_temp_pic_b);
                Pic_Cut_To_Mask(0, 0, ui_temp_pic_b, ui_temp_pic_a);
                Draw_Bitmap(x + 12, y + 12, ui_temp_pic_b);
                Draw_Pic_Frame(x, y, diplo_mirror_pic);
            } else if (global_ench_frame < 20) {
                Draw_Pic_Frame(x + 12, y + 12, castui_frame_pic);
                Pic_Reset_Frame(globalench_anim_pic);
                Draw_Pic_Frame(x + 12, y + 12, globalench_anim_pic);
                Draw_Pic_Frame(x, y, diplo_mirror_pic);
            } else if (global_ench_frame < 34) {
                Draw_Pic_Frame(x + 10, y + 8, p_anim_pic);
                Pic_Decode_Frame_To_Bitmap(global_ench_anim_pic, ui_temp_pic_a);
                Pic_Reset_Frame(globalench_anim_pic);
                Pic_Decode_Frame_To_Bitmap(globalench_anim_pic, ui_temp_pic_b);
                Pic_Cut_To_Mask(0, 0, ui_temp_pic_b, ui_temp_pic_a);
                Draw_Bitmap(x + 12, y + 12, ui_temp_pic_b);
                Draw_Pic_Frame(x, y, diplo_mirror_pic);
            } else {
                Draw_Pic_Frame(x, y, diplo_mirror_pic);
                Draw_Pic_Frame(x + 10, y + 8, p_anim_pic);
                Print_Far_Text_Centered(x + 62, y + 133, p_spell_data[ui_spell].name);
            }
        } else {
            if (cast_ui_slot == 0)
                strcpy(message_text, "You have completed casting...");
            else {
                strcpy(message_text, wizards[cast_ui_slot].wizard_name);
                strcat(message_text, " has cast...");
            }
            Print_Text_Centered(x + 62, y + 133, message_text);
            Draw_Pic_Frame(x, y, diplo_mirror_pic);
            Draw_Pic_Frame(x + 10, y + 8, p_anim_pic);
            Print_Far_Text_Centered(x + 62, y + 153, p_spell_data[ui_spell].name);
        }
    }
}

/* (wizard, spell) a global enchantment taking hold: its jingle, 120 frames (the pictures reloaded at 10, 20, 30) or a
   click */
void GlobalEnch_Complete_Anim(int wizard, int spell)
{
    int input;
    int control;
    int slot;

    Sound_Stop();
    if (spell == SPELL_DEATH_WISH)              /* the jingle: MUSIC 19 + the global's index, the generic 40 past 21 */
        slot = 23;
    else if (spell == SPELL_GREAT_UNSUMMONING)
        slot = 24;
    else
        slot = (int)p_spell_data[spell].effect_param;
    if (slot > 21)
        music_jingle = LBX_Reload("Music", MUSIC_NEW_SPELL, p_music_buf);
    else
        music_jingle = LBX_Reload("Music", slot + MUSIC_GLOBAL_ENCH_BASE, p_music_buf);
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    global_ench_frame = 0;
    cast_ui_slot = wizard;
    ui_spell = spell;
    Heap_Reset(p_scratch);
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    GlobalEnch_Anim_Load(spell, wizard);
    UI_Clear_Controls();
    control = UI_Add_Region(0, 0, 319, 199, "", -1);
    UI_Set_Redraw_Fn(GlobalEnch_Anim_Redraw, 2);
    cast_ui_done = 0;
    global_ench_frame = 0;
    while (global_ench_frame < 120 && cast_ui_done == 0) {
        input = abs(UI_Get_Input());
        if (input == control)
            cast_ui_done = 1;
        if (cast_ui_done != 1) {
            if (global_ench_frame == 10) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                GlobalEnch_Anim_Load(spell, wizard);
            } else if (global_ench_frame == 20) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                GlobalEnch_Anim_Load(spell, wizard);
            } else if (global_ench_frame == 30) {
                Heap_Release(p_scratch);
                Heap_Mark(p_scratch);
                GlobalEnch_Anim_Load(spell, wizard);
            }
            if (global_ench_frame == 34)
                present_mode = PRESENT_DISSOLVE;
            Set_Draw_Page();
            GlobalEnch_Anim_Redraw();
            Screen_Present();
        }
        global_ench_frame++;
    }
    Heap_Release(p_scratch);
    Sound_Stop();
    Music_Play_Current();
    Map_Redraw_Dissolve();
}

/* (spell) the wizard-target screen: the box (SPELLSCR.LBX 0, 0x48 for Spell Blast) over the map and a gem per rival
   (known and alive; else the empty frame or the dead overlay); the help records */
void WizTarget_Screen_Load(int spell)
{
    int box_x;
    int box_y;
    int x;
    int y;
    int k;
    int i;

    box_x = 50;
    box_y = 20;
    Minimap_Alloc_Pic();
    if (spell != SPELL_SPELL_BLAST)
        wiztarget_bg_pic = LBX_Reload_Next("SPELLSCR", 0, p_scratch);
    else
        wiztarget_bg_pic = LBX_Reload_Next("SPELLSCR", 0x48, p_scratch);
    wiztarget_button_pic = LBX_Reload_Next("Spellscr", 0x47, p_scratch);
    magic_gem_frame_pic = LBX_Reload_Next("MAGIC", 6, p_scratch);
    magic_overlay_pic = LBX_Reload_Next("MAGIC", 0x33, p_scratch);
    for (i = 1; i < num_players; i++)
        magic_gem_pics[i] = LBX_Reload_Next("Lilwiz", LILWIZ_ENTRY(wizards[i].portrait, wizards[i].banner_color), p_scratch);
    Set_Draw_Page();
    Map_Redraw();
    Draw_Pic_Frame(box_x - 5, box_y - 10, wiztarget_bg_pic);
    Font_Select(0, 3, 0, 0);
    Set_Font_Color(0xf9);
    for (k = 0; k < 4; k++) {
        x = box_x + k % 2 * 77 + 19;              /* the 2 x 2 gem grid: columns 77 apart, rows 61 */
        y = box_y + k / 2 * 61 + 27;
        if (wizards[PLAYER_HUMAN].contacted[k + 1] == 1 && p_fortresses[k + 1].active == 1)
            Draw_Pic_Frame(x, y, magic_gem_pics[k + 1]);
        else if (p_fortresses[k + 1].active == 1)
            Draw_Pic_Frame(x, y, magic_gem_frame_pic);
        else
            Draw_Pic_Frame(x, y - 1, magic_overlay_pic);
    }
    Screen_Save_A800();
    Set_Draw_Page_Front();
    LBX_Load_Records_Near("hlpentry", HLPENTRY_TARGET_WIZARD, help_entries, 0, 7, sizeof(struct HelpEntry));
}

/* () a frame of the wizard-target screen: per known rival its relation (Spell Blast: the spell it is casting, and the
   mana it would cost under the mouse); the title or the result */
void WizTarget_Screen_Redraw(void)
{
    int i;
    int gem_x;
    int gem_y;
    char colors[6];
    char num_text[6];
    int relation;
    char text[20];
    int box_x;
    int box_y;

    box_x = 50;
    box_y = 20;
    colors[0] = 0xf9;
    for (i = 1; i < 6; i++)
        colors[i] = i + 0xb1;
    Screen_Restore_A800();
    Font_Select(0, 3, 0, 0);
    Set_Font_Color(0xf9);
    for (i = 0; i < 4; i++) {
        gem_x = box_x + i % 2 * 77 + 19;
        gem_y = box_y + i / 2 * 61 + 27;
        if (wizards[PLAYER_HUMAN].contacted[i + 1] == 1 && p_fortresses[i + 1].active == 1) {
            if (ui_spell != SPELL_SPELL_BLAST) {
                relation = (wizards[i + 1].relationship[current_player] + 100) / 20;
                Print_Text_Centered(gem_x + 21, gem_y + 49, relation_names[relation]);
            } else {
                if (magic_gem_wizard[i] == cast_hotspot) {
                    Set_Text_Shadow_Color(0xfe);
                    Font_Select_Palette(4, colors);
                    itoa(wizards[i + 1].casting_cost_initial - wizards[i + 1].casting_cost_left, text, 10);
                    strcat(text, " MP");
                    Print_Text_Centered(box_x + 39, box_y + 149, text);
                }
                Font_Select(0, 3, 0, 0);
                Set_Font_Color(0xf9);
                Print_Far_Text_Centered(gem_x + 21, gem_y + 49, p_spell_data[wizards[i + 1].casting_spell].name);
            }
        }
    }
    Set_Text_Shadow_Color(0x97);
    Font_Select_Palette(4, colors);
    Font_Select_Shadow(4, 15, 0, 0);
    Set_Line_Spacing(1);
    if (cast_screen_live == 1) {
        strcpy(message_text, "Choose target for a ");
        far_strcpy(text, 0, p_spell_data[ui_spell].name);
        strcat(message_text, text);
        strcat(message_text, " spell");
    } else {
        strcpy(message_text, wizards[cast_ui_slot].wizard_name);
        switch ((int)p_spell_data[ui_spell].effect_param) {
        case WIZSPELL_SPELL_BLAST:
            strcat(message_text, " has been spell blasted");
            break;
        case WIZSPELL_CRUEL_UNMINDING:
            strcat(message_text, " loses ");
            itoa(cast_power_total, num_text, 10);
            strcat(message_text, num_text);
            strcat(message_text, " points of casting ability");
            break;
        case WIZSPELL_DRAIN_POWER:
            strcat(message_text, " loses ");
            itoa(cast_power_total, num_text, 10);
            strcat(message_text, num_text);
            strcat(message_text, " points of mana");
            break;
        case WIZSPELL_SUBVERSION:
            strcat(message_text, message_text[(int)strlen(message_text) - 1] == 's' ? "' " : "'s ");
            strcat(message_text, " relations have been subverted");
            break;
        }
    }
    Paragraph_Print(box_x + 5, box_y - 1, 144, message_text, PARA_CENTER);
}

/* (spell) -> the rival picked: Spell Blast (only when the human has the mana it costs), Cruel Unminding, Drain Power
   or Subversion; right click shows the rival; the effect, then 45 frames of its animation over the rival's gem
   Numbered: BUG #136, #406 (BUGS_AND_QUIRKS.txt) */
int Screen_Wizard_Target(int spell)
{
    int box_x;
    int box_y;
    int input;
    int anim_x;
    int anim_y;
    int ok_button;                              /* the Cancel button (Esc; cancelled = 1), later the full-screen click region */
    int cost;
    int cancelled;
    int i;
    int target;

    cancelled = 0;
    box_x = 50;
    box_y = 20;
    ui_spell = spell;
    cast_screen_live = 1;
    WizTarget_Screen_Load(spell);
    UI_Set_Redraw_Fn(WizTarget_Screen_Redraw, 2);
    UI_Clear_Controls();
    for (i = 0; i < 4; i++)
        magic_gem_wizard[i] = UI_Add_Region(box_x + i % 2 * 77 + 17, box_y + i / 2 * 61 + 27,
                                             box_x + i % 2 * 77 + 62, box_y + i / 2 * 61 + 72, "", -1);
    if (spell != SPELL_SPELL_BLAST) {
        ok_button = UI_Add_Button(box_x + 40, box_y + 145, "", wiztarget_button_pic, "\x1b", -1);
        help_entries[1].id = -1;
        help_entries[2].id = -1;
        for (i = 0; i < 4; i++)
            help_entries[i + 3].id = HELP_RELATIONS;
    } else {
        ok_button = UI_Add_Button(box_x + 78, box_y + 145, "", wiztarget_button_pic, "\x1b", -1);
        help_entries[0].id = -1;
        for (i = 0; i < 4; i++) {
            if (wizards[i + 1].casting_spell == SPELL_NONE)
                help_entries[i + 3].id = HELP_CASTING_NONE;
            else
                help_entries[i + 3].id = wizards[i + 1].casting_spell;
        }
    }
    UI_Set_Help(help_entries, 7);
    cast_ui_done = 0;
    while (cast_ui_done == 0) {
        input = UI_Get_Input();
        cast_hotspot = UI_Hit_Test();
        if (input == ok_button) {
            cast_ui_done = 1;
            cancelled = 1;
        }
        for (i = 0; i < 4; i++) {
            if (magic_gem_wizard[i] == input && wizards[PLAYER_HUMAN].contacted[i + 1] == 1 && p_fortresses[i + 1].active == 1) {
                Play_Click_Sound();
                target = i + 1;
                if (spell == SPELL_SPELL_BLAST) {
                    cost = wizards[target].casting_cost_initial - wizards[target].casting_cost_left;
                    if (cost > wizards[PLAYER_HUMAN].mana_reserve) {
                        LBX_Load_Records_Near("message", 0, message_text, 68, 1, 150);
                        Warning_Box(message_text);
                    } else
                        cast_ui_done = 1;
                } else
                    cast_ui_done = 1;
            }
            if (-magic_gem_wizard[i] == input && wizards[PLAYER_HUMAN].contacted[i + 1] == 1 && p_fortresses[i + 1].active == 1) {
                Play_Click_Sound();
                target = i + 1;
                Screen_Wizard_Info(target, box_x + i % 2 * 77 + 17, box_y + i / 2 * 61 + 27, box_x + i % 2 * 77 + 62,
                                   box_y + i / 2 * 61 + 72);
                WizTarget_Screen_Load(spell);
                UI_Set_Redraw_Fn(WizTarget_Screen_Redraw, 2);
                UI_Clear_Controls();
                for (i = 0; i < 4; i++)
                    magic_gem_wizard[i] = UI_Add_Region(box_x + i % 2 * 77 + 17, box_y + i / 2 * 61 + 27,
                                                         box_x + i % 2 * 77 + 62, box_y + i / 2 * 61 + 72, "", -1);
                if (spell != SPELL_SPELL_BLAST) {
                    ok_button = UI_Add_Button(box_x + 40, box_y + 145, "", wiztarget_button_pic, "\x1b", -1);
                    help_entries[1].id = -1;
                    help_entries[2].id = -1;
                    for (i = 0; i < 4; i++)
                        help_entries[i + 3].id = HELP_RELATIONS;
                } else {
                    ok_button = UI_Add_Button(box_x + 78, box_y + 145, "", wiztarget_button_pic, "\x1b", -1);
                    help_entries[0].id = -1;
                    for (i = 0; i < 4; i++) {
                        if (wizards[i + 1].casting_spell == SPELL_NONE)
                            help_entries[i + 3].id = HELP_CASTING_NONE;
                        else
                            help_entries[i + 3].id = wizards[i + 1].casting_spell;
                    }
                }
                UI_Set_Help(help_entries, 7);
            }
        }
        if (cast_ui_done == 0) {
            Set_Draw_Page();
            WizTarget_Screen_Redraw();
            Screen_Present();
        }
    }
    if (cancelled == 0) {
        cast_ui_slot = target;
        anim_x = box_x + (target - 1) % 2 * 77 + 10;
        anim_y = box_y + (target - 1) / 2 * 61 + 21;
        Heap_Reset(p_scratch);
        Spell_Load_Anim(spell);
        Spell_Load_Sound(spell);
        if (p_sfx_buf != -1)
            Sound_Play(p_sfx_buf);
        UI_Clear_Controls();
        ok_button = UI_Add_Region(0, 0, 319, 199, "", -1);
        switch ((int)p_spell_data[spell].effect_param) {
        case WIZSPELL_SPELL_BLAST:
            cost = wizards[target].casting_cost_initial - wizards[target].casting_cost_left;
            if (cost <= wizards[PLAYER_HUMAN].mana_reserve) {
                wizards[PLAYER_HUMAN].mana_reserve -= cost;
                if (wizards[target].casting_spell != SPELL_SPELL_OF_RETURN) {
                    wizards[target].casting_spell = 0;
                    wizards[target].casting_cost_left = 0;
                    wizards[target].casting_cost_initial = 0;
                } else
                    wizards[target].casting_cost_left = Spell_Casting_Cost(target, SPELL_SPELL_OF_RETURN, 0);
            }
            break;
        case WIZSPELL_CRUEL_UNMINDING:
            cast_power_total = Cruel_Unminding_Apply(target);
            break;
        case WIZSPELL_DRAIN_POWER:
            cast_power_total = Drain_Power_Apply(target);
            break;
        case WIZSPELL_SUBVERSION:
            Subversion_Apply(target);
            break;
        }
        for (i = 0; i < 45 && UI_Get_Input() != ok_button; i++) {
            Timer_Mark();
            Set_Draw_Page();
            if (i == 12)
                cast_screen_live = 0;
            WizTarget_Screen_Redraw();
            if (i < 36) {
                if (i % 6 == 0)
                    Pic_Reset_Frame(p_anim_pic);
                Draw_Pic_Frame(anim_x, anim_y, p_anim_pic);
            }
            Screen_Present();
            Wait_Ticks(2);
        }
    }
    Map_Redraw_Dissolve();
    Heap_Release();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    return target;                              /* uninitialised when the screen was cancelled */
}
