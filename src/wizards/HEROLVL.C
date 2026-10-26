/* HEROLVL.C - WIZARDS.EXE overlay code segment 82d7 (stub 2509): the hero level-up popup - "<name> has made a
   level." with the fixed gains (+1 hit points, resistance, attack; +1 defense on odd levels) and the hero's rank and
   ability lines with their new bonuses (Leadership, Legendary, Blademaster, Armsmaster, Constitution, Might, Arcane
   Power, Sage, Prayermaster, Agility; the super version marked '}'), and the caster line.
   This module's data (DS:576c): its literals; BSS (DS:c7a0..c7aa).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 82d7 src/wizards/HEROLVL.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Screen_Hero_Level_Up(int unit);
void Hero_Level_Up_Redraw(void);
void Hero_Level_Up_Lines(int unit, struct UnitAbilityList *list, int *count);

#include "GAME.H"

int levelup_unit;                               /* DS:c7a8 */
int levelup_box_x;                              /* DS:c7a6 */
int levelup_box_y;                              /* DS:c7a4 */
unsigned levelup_box_pic;                       /* DS:c7a2  RELOAD.LBX 0x17 */
unsigned levelup_bottom_pic;                    /* DS:c7a0  RELOAD.LBX 0x18 */

extern struct HeroAbility hero_ability_table[23];      /* DS:3a50 */
extern char *hero_level_names[9];               /* DS:3f52  Hero, Myrmidon, Captain, ... */
extern unsigned p_music_buf;                    /* DS:9390 */
extern unsigned item_power_bullet_pic;          /* DS:976a */
extern unsigned special_icons[];                /* DS:97f8  SPECIAL.LBX */
extern unsigned special2_icons[];               /* DS:98d6  SPECIAL2.LBX */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int unit_view_abil_count;                /* DS:c190 */
extern struct UnitAbilityList *p_unit_view_list;       /* DS:c192 */
extern int unit_view_row_text_ctl[8];           /* DS:c1c0 */
extern int unit_view_row_icon_ctl[8];           /* DS:c1d0 */
extern unsigned unit_view_portrait_pic;         /* DS:c79e */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Minimap_Alloc_Pic(void);
void Map_Full_Redraw(void);
void Map_Redraw_Dissolve(void);
void Screen_Copy_Front(void);
void Load_Unit_View_Graphics(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Help_Load_Hero_Level(void);
int UI_Add_Picture(int x, int y, int pic, char *hotkey, int help);
void Music_Play_Current(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);

/* (unit) -> 0 for a hero already at the top level (no return value otherwise): the hero gains a level and the
   popup shows it until a click (the music's level-up tune when event music is on) */
int Screen_Hero_Level_Up(int unit)
{
    int done;
    int input;
    int leave;
    int y;
    int bottom;                                 /* written, never read */
    int sound;
    int i;
    int x;

    if (p_units[unit].level > 7)                /* already Demi-God, the last rank */
        return 0;
    Sound_Stop();
    Minimap_Alloc_Pic();
    Map_Full_Redraw();
    Screen_Copy_Front();
    Screen_Save_A800();
    Load_Unit_View_Graphics();
    levelup_unit = unit;
    levelup_box_x = 31;
    levelup_box_y = 41;
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Hero_Level_Up_Redraw, 1);
    p_units[levelup_unit].level = p_units[levelup_unit].level + 1;
    Battle_Unit_Init(unit, p_unit_stats_buf);
    levelup_box_pic = LBX_Reload("RELOAD", 0x17, p_scratch);
    levelup_bottom_pic = LBX_Reload_Next("RELOAD", 0x18, p_scratch);
    unit_view_portrait_pic = LBX_Reload_Next("PORTRAIT", p_unit_stats_buf->hero_portrait, p_scratch);
    if (game_config.event_music == 1) {
        sound = LBX_Reload("MUSIC", MUSIC_FANFARE, p_music_buf);
        Sound_Play(sound);
    }
    p_text_buf = Near_Alloc_Reset(100);
    p_unit_view_list = (struct UnitAbilityList *)Near_Alloc(sizeof(struct UnitAbilityList));
    unit_view_abil_count = 0;
    Hero_Level_Up_Lines(unit, p_unit_view_list, &unit_view_abil_count);
    if (unit_view_abil_count > 8)                /* the popup holds 8 lines (4 rows of 2) */
        unit_view_abil_count = 8;
    UI_Clear_Help();
    Help_Load_Hero_Level();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        x = levelup_box_x + 13;
        y = levelup_box_y + 48;
        for (i = 0; i < unit_view_abil_count; i++) {
            unit_view_row_icon_ctl[i] = UI_Add_Picture(x, y, p_unit_view_list->icon[i], "",
                                                           p_unit_view_list->help[i]);
            help_entries[i].id = p_unit_view_list->help[i];
            unit_view_row_text_ctl[i] = UI_Add_Region(x + 19, y + 5, x + 93, y + 13, "", p_unit_view_list->help[i]);
            x = levelup_box_x + 13;
            if (i != 0) {
                if (i % 2 == 1)
                    y += 17;
                else
                    x += 115;
            } else
                x += 115;
        }
        bottom = levelup_box_y + (unit_view_abil_count + 1) / 2 * 17 + 49;
        leave = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        for (i = 0; i < unit_view_abil_count; i++) {
            if (unit_view_row_icon_ctl[i] == input || unit_view_row_text_ctl[i] == input) {
                done = -1;
                break;
            }
        }
        if (input == leave)
            done = -1;
        if (done == 0) {
            Screen_Restore_A800();
            Hero_Level_Up_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    Load_Overland_Screen_Graphics();
    UI_Clear_Help();
    Reset_Clip_Rect();
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Minimap_Alloc_Pic();
    Map_Redraw_Dissolve();
    Music_Play_Current();
}

/* () the popup: the box, the portrait, "<name> has made a level.", the fixed gains (+1 defense on odd levels) and
   the ability lines */
void Hero_Level_Up_Redraw(void)
{
    char colors[6];
    int owner;
    int y;
    char num_text[6];
    int level;                                  /* written, never read */
    int gain;
    int bottom;
    int i;
    int x;

    owner = p_units[levelup_unit].owner;
    level = p_units[levelup_unit].level;
    Set_Draw_Page();
    bottom = levelup_box_y + (unit_view_abil_count + 1) / 2 * 17 + 49;
    Set_Clip_Rect(levelup_box_x, levelup_box_y, levelup_box_x + 250, bottom);
    Draw_Pic_Clipped(levelup_box_x, levelup_box_y, levelup_box_pic);
    Draw_Pic_Frame(levelup_box_x, bottom, levelup_bottom_pic);
    Draw_Pic_Frame(levelup_box_x + 10, levelup_box_y + 10, unit_view_portrait_pic);
    colors[0] = 0xf0;
    colors[1] = 0xb2;
    colors[2] = 0xb3;
    colors[3] = 0xb4;
    colors[4] = 0xb4;
    Font_Select_Palette(4, colors);
    Font_Select_Shadow(4, 15, 0, 0);
    Set_Text_Shadow_Color(0xf0);
    strcpy(p_text_buf, wizards[owner].hired_heroes[p_units[levelup_unit].hero_slot].name);
    strcat(p_text_buf, " has made a level.");
    Print_Text(levelup_box_x + 48, levelup_box_y + 9, p_text_buf);
    colors[0] = 0xf0;
    colors[1] = 0xb2;
    colors[2] = 0xb2;
    colors[3] = 0xb2;
    colors[4] = 0xb2;
    Font_Select_Palette(2, colors);
    Font_Select_Shadow(2, 15, 0, 0);
    Set_Text_Shadow_Color(0xf0);
    Draw_Pic_Frame(levelup_box_x + 47, levelup_box_y + 25, item_power_bullet_pic);
    Print_Text(levelup_box_x + 55, levelup_box_y + 24, "+1 Hit Points");
    Draw_Pic_Frame(levelup_box_x + 47, levelup_box_y + 35, item_power_bullet_pic);
    Print_Text(levelup_box_x + 55, levelup_box_y + 34, "+1 Resistance");
    Draw_Pic_Frame(levelup_box_x + 139, levelup_box_y + 25, item_power_bullet_pic);
    Print_Text(levelup_box_x + 147, levelup_box_y + 24, "+1 Attack");
    gain = -1;
    switch (p_units[levelup_unit].level) {
    case 1:
    case 3:
    case 5:
    case 7:
        gain = 1;
        break;
    }
    if (gain != -1) {
        Draw_Pic_Frame(levelup_box_x + 139, levelup_box_y + 35, item_power_bullet_pic);
        itoa(gain, num_text, 10);
        strcpy(p_text_buf, "+");
        strcat(p_text_buf, num_text);
        strcat(p_text_buf, " Defense");
        Print_Text(levelup_box_x + 147, levelup_box_y + 34, p_text_buf);
    }
    y = levelup_box_y + 48;
    x = levelup_box_x + 13;
    for (i = 0; i < unit_view_abil_count; i++) {
        Draw_Pic_Frame(x, y, p_unit_view_list->icon[i]);
        Print_Text(x + 19, y + 5, p_unit_view_list->text[i]);
        x = levelup_box_x + 13;
        if (i != 0) {
            if (i % 2 == 1)
                y += 17;
            else
                x += 115;
        } else
            x += 115;
    }
}

/* (unit, list, &count) the level-up lines: the rank name, "+1 To Hit" at levels 2 / 5 / 8, one line per ability of
   the hero whose bonus grows (Leadership, Blademaster and Constitution only on even levels -
   which miss when they grow: Leadership's (level + 1) / 3 at levels 2 / 5 / 8, Blademaster's (level + 1) / 2 at
   the odd levels; '}' marks the super
   ability), then the
   caster line ("Caster <points> (<skill>) mp") */
void Hero_Level_Up_Lines(int unit, struct UnitAbilityList *list, int *count)
{
    long bits;
    int unit_type;
    int ability;
    int owner;
    int bonus;
    int show;
    char num_text[20];
    int n;

    *count = 0;
    n = 0;
    unit_type = p_units[unit].type;
    owner = p_units[unit].owner;
    strcpy(list->text[n], hero_level_names[p_units[unit].level]);
    list->ench_mask[n] = ABIL_ROW_PLAIN;
    list->icon[n] = special_icons[p_units[unit].level + SPECIAL_ICON_RANK_BASE];
    list->help[n] = p_units[unit].level + HELP_HERO_RANK_BASE;
    n++;
    if (p_units[unit].level == 2 || p_units[unit].level == 5 || p_units[unit].level == 8) {
        strcpy(list->text[n], "+");
        bonus = 1;
        itoa(bonus, num_text, 10);
        strcat(list->text[n], num_text);
        strcat(list->text[n], " To Hit");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special2_icons[SPECIAL2_ICON_TO_HIT];
        list->help[n] = HELP_TO_HIT;
        n++;
    }
    for (ability = 0; ability < NUM_HERO_ABILITY_ROWS; ability++) {
        bonus = -1;
        show = 1;
        bits = p_hero_stats[owner][unit_type].abilities & hero_ability_table[ability].mask;
        if (bits != 0) {
            switch (bits) {
            case HERO_ABILITY_LEADERSHIP:
                if (p_units[unit].level % 2 == 0) {
                    strcpy(list->text[n], "Leadership");
                    bonus = (p_units[unit].level + 1) / 3;
                }
                break;
            case HERO_ABILITY_SUPER_LEADERSHIP:
                if (p_units[unit].level % 2 == 0) {
                    strcpy(list->text[n], "Leadership");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) / 2;
                }
                break;
            case HERO_ABILITY_LEGENDARY:
                strcpy(list->text[n], "Legendary");
                bonus = (p_units[unit].level + 1) * 3;
                break;
            case HERO_ABILITY_SUPER_LEGENDARY:
                strcpy(list->text[n], "Legendary");
                strcat(list->text[n], "}");
                bonus = (p_units[unit].level + 1) * 9 / 2;
                break;
            case HERO_ABILITY_BLADEMASTER:
                if (p_units[unit].level % 2 == 0) {
                    strcpy(list->text[n], "Blademaster");
                    bonus = (p_units[unit].level + 1) / 2;
                }
                break;
            case HERO_ABILITY_SUPER_BLADEMASTER:
                if (p_units[unit].level % 2 == 0) {
                    strcpy(list->text[n], "Blademaster");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 4;
                }
                break;
            case HERO_ABILITY_ARMSMASTER:
                strcpy(list->text[n], "Armsmaster");
                bonus = (p_units[unit].level + 1) * 2;
                break;
            case HERO_ABILITY_SUPER_ARMSMASTER:
                strcpy(list->text[n], "Armsmaster");
                strcat(list->text[n], "}");
                bonus = (p_units[unit].level + 1) * 3;
                break;
            case HERO_ABILITY_CONSTITUTION:
                if (p_units[unit].level % 2 == 0) {
                    strcpy(list->text[n], "Constitution");
                    bonus = p_units[unit].level + 1;
                }
                break;
            case HERO_ABILITY_SUPER_CONSTITUTION:
                if (p_units[unit].level % 2 == 0) {
                    strcpy(list->text[n], "Constitution");
                    strcat(list->text[n], "}");
                    bonus = (p_units[unit].level + 1) * 3 / 2;
                }
                break;
            case HERO_ABILITY_MIGHT:
                strcpy(list->text[n], "Might");
                bonus = p_units[unit].level + 1;
                break;
            case HERO_ABILITY_SUPER_MIGHT:
                strcpy(list->text[n], "Might");
                strcat(list->text[n], "}");
                bonus = (p_units[unit].level + 1) * 3 / 2;
                break;
            case HERO_ABILITY_ARCANE_POWER:
                strcpy(list->text[n], "Arcane Power");
                bonus = p_units[unit].level + 1;
                break;
            case HERO_ABILITY_SUPER_ARCANE_POWER:
                strcpy(list->text[n], "Arcane Power");
                strcat(list->text[n], "}");
                bonus = (p_units[unit].level + 1) * 3 / 2;
                break;
            case HERO_ABILITY_SAGE:
                strcpy(list->text[n], "Sage");
                strcat(list->text[n], " ");
                bonus = (p_units[unit].level + 1) * 3;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], num_text);
                strcat(list->text[n], " rp");
                show = 0;
                break;
            case HERO_ABILITY_SUPER_SAGE:
                strcpy(list->text[n], "Sage");
                strcat(list->text[n], "}");
                strcat(list->text[n], " ");
                bonus = (p_units[unit].level + 1) * 9 / 2;
                itoa(bonus, num_text, 10);
                strcat(list->text[n], num_text);
                strcat(list->text[n], " rp");
                show = 0;
                break;
            case HERO_ABILITY_PRAYERMASTER:
                strcpy(list->text[n], "Prayermaster");
                bonus = p_units[unit].level + 1;
                break;
            case HERO_ABILITY_SUPER_PRAYERMASTER:
                strcpy(list->text[n], "Prayermaster");
                strcat(list->text[n], "}");
                bonus = (p_units[unit].level + 1) * 3 / 2;
                break;
            case HERO_ABILITY_AGILE:
                strcpy(list->text[n], "Agility");
                bonus = p_units[unit].level + 1;
                break;
            case HERO_ABILITY_SUPER_AGILE:
                strcpy(list->text[n], "Agility");
                strcat(list->text[n], "}");
                bonus = (p_units[unit].level + 1) * 3 / 2;
                break;
            }
            if (bonus > 0) {
                if (show == 1) {
                    itoa(bonus, num_text, 10);
                    strcat(list->text[n], " +");
                    strcat(list->text[n], num_text);
                }
                list->ench_mask[n] = ABIL_ROW_PLAIN;
                list->icon[n] = special_icons[hero_ability_table[ability].icon];
                list->help[n] = hero_ability_table[ability].help;
                n++;
            }
        }
    }
    if (p_hero_stats[owner][unit_type].caster_level > 0) {
        strcpy(list->text[n], "Caster");
        strcat(list->text[n], " ");
        if (p_unit_stats_buf->spell_skill > p_unit_stats_buf->spell_points) {
            bonus = p_unit_stats_buf->spell_points;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], num_text);
            strcat(list->text[n], " (");
            bonus = p_unit_stats_buf->spell_skill;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], num_text);
            strcat(list->text[n], ")");
        } else {
            bonus = p_unit_stats_buf->spell_skill;
            itoa(bonus, num_text, 10);
            strcat(list->text[n], num_text);
        }
        strcat(list->text[n], " mp");
        list->ench_mask[n] = ABIL_ROW_PLAIN;
        list->icon[n] = special_icons[SPECIAL_ICON_CASTER];
        list->help[n] = HELP_CASTER;
        n++;
    }
    *count = n;
}
