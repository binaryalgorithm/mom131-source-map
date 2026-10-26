/* CASTUI.C - WIZARDS.EXE overlay code segment bcc7 (stub 25d3): casting dialogs - the Resurrection hero list, the
   extra-power slider of the variable-cost spells (overland: the casting cost with the realm reduction; combat: the
   most a unit's spell points or a wizard's skill left, mana and range allow), and the Disjunction / Spell Binding
   target screen (the global enchantments of the other wizards in one column each, three rows at a time, the dispel
   roll and its animation; a bound spell moves to the caster).
   This module's data (DS:6b4a): its literals; BSS (DS:c9fa..ca31): the dialog globals (owner not fixed by the EXE for the
   words only other overlays read: mastery_twinkle_pic, wiztarget_button_pic).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS bcc7 src/wizards/CASTUI.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Resurrection_Pick_Hero(int n, int *hero_types);
void OverlandPower_Load(void);
void OverlandPower_Redraw(void);
void Overland_Extra_Power_Dialog(int spell, int slot);
void CastUI_Power_Load(void);
void CastUI_Power_Redraw(void);
int Combat_Extra_Power_Dialog(int spell, int slot, int caster);
void Disjunction_Screen_Load(int spell);
void Disjunction_Screen_Redraw(void);
int Screen_Disjunction(int spell, int caster);
void Spell_Binding_Resolve(int row, int caster, int spell, int target);
void Disjunction_Resolve(int row, int caster, int spell, int target);

#include "GAME.H"

/* the extra-power slider: power_bar_width from 3 (no extra) to 0x67 on a 0x6a-pixel bar */
#define POWER_BAR_MIN 3
#define POWER_BAR_MAX 0x67
#define POWER_BAR_WIDTH 0x6a
/* the item forge's two icon words hold the power bar's pictures here: SPELLSCR 3 the bar's end, 4 its scrolling fill */
#define power_bar_end_pic forge_icon_prev_pic
#define power_bar_fill_pic forge_icon_next_pic
#define disjunction_box_pic castui_button_pic   /* Disjunction_Screen_Load: the box background (SPELLSCR 1) */

unsigned castui_button_pic;                     /* DS:ca30  SPELLSCR.LBX 0x2a: the extra-power button; SPELLSCR 1: the Disjunction box */
unsigned disj_highlight_pic;                    /* DS:ca2e  SPELLSCR.LBX 7: the selected row of the Disjunction screen */
int disjunction_choice;                         /* DS:ca2c  the row under the mouse (1..), 0 none */
                                                /* also: the summon kind (CASTANIM.C), the hotspot under the mouse,
                                                   MASTERY.C's running SPELLSCR animation */
unsigned wiztarget_button_pic;                  /* DS:ca2a  SPELLSCR.LBX 0x47 (read by the wizard-target screen only) */
int cast_ui_caster;                             /* DS:ca28  the combat caster (> 19 = a wizard) */
                                                /* also: a picture handle (GlobalEnch_Anim_Load) */
unsigned castui_frame_pic;                      /* DS:ca26  SPELLSCR.LBX 5 */
                                                /* also: MASTERY.C's SPELLOSE / SPLMASTR background */
unsigned mastery_twinkle_pic;                   /* DS:ca24  SPLMASTR 0x1c (read by the Spell of Mastery screens only) */
int disj_list_first[5];                         /* DS:ca1a  first row shown of each opponent's column (5, one per possible opponent as ReMoM sizes it; 4 used; clarified using ReMoM repo) */
int power_bar_width;                            /* DS:ca18  the slider value, POWER_BAR_MIN .. POWER_BAR_MAX */
int ui_spell;                                   /* DS:ca16  the spell of the dialog */
int cast_anim_frame;                            /* DS:ca14  0..7 the slider bar's scroll; also the frame counter of the summoning screen (CASTANIM.C) and of the Spell of Mastery and undead-rise screens (MASTERY.C) */
int cast_power_total;                           /* DS:ca12  the extra power, then the total cost */
                                                /* also: GlobalEnch_Complete_Anim's frame counter; the drained skill
                                                   / mana WizTarget_Screen_Redraw shows */
int cast_power_extra;                           /* DS:ca10  combat: the most extra power allowed */
int *disj_owner_lists[5];                       /* DS:ca06  each opponent's run in p_magic_enchant_spell (5, as disj_list_first; 4 used) */
int disj_owner_counts[6];                       /* DS:c9fa  global enchantments per owner (6, one per player as ReMoM sizes it; 5 used) */

extern unsigned spellbook_realm_icon_src_pics[];      /* DS:9454 */
extern unsigned magic_gem_pics[];               /* DS:958e */
extern unsigned magic_gem_frame_pic;            /* DS:95c0 */
extern unsigned magic_overlay_pic;              /* DS:95c2 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern unsigned p_anim_pic;                     /* DS:9130 */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */
extern int magic_enchant_count;                 /* DS:c238 */
extern char *p_magic_enchant_owner;             /* DS:c23a */
extern int *p_magic_enchant_spell;              /* DS:c23c */
extern unsigned spellbook_realm_icon_pics[];          /* DS:c8fe */
extern unsigned ui_temp_pic_a;                  /* DS:ca34 */
extern unsigned ui_temp_pic_b;                  /* DS:ca36 */
extern unsigned forge_icon_next_pic;            /* DS:ca40 */
extern unsigned forge_icon_prev_pic;            /* DS:ca42 */
extern int cast_ui_done;                   /* DS:ca4c */
extern int cast_screen_live;                    /* DS:ca4e */
extern int cast_ui_slot;                        /* DS:ca50 */
extern int cast_result_shown;                  /* DS:ca52 */
extern unsigned list_dialog_bottom_arrow_pic;   /* DS:cb12 */
extern unsigned list_dialog_top_arrow_pic;      /* DS:cb14 */

/* Heap_Release has no prototype here: Overland_Extra_Power_Dialog calls it without its argument */
void Heap_Release();
void far_strcpy(char *dst, int dst_seg, char far *src);
int List_Select_Dialog(int count, char **names, int more, char *title);
void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
void Map_Redraw_Dissolve(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Near_Alloc_Reset_All(void);
void Near_Alloc_Mark(void);
void Near_Alloc_Release(void);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
void Pic_Create_Blank(int width, int height, unsigned pic);
void Pic_Scale_Percent(unsigned pic, int x_pct, int y_pct);
void Pic_Dissolve(unsigned pic, int amount);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Draw_Page_Front(void);
void Screen_Copy_Front(void);
void Spellbook_Overland_Redraw(int x, int y);
void Spellbook_Combat_Redraw(int x, int y, int caster);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Font_Select_Shadow_UL(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
int Print_Text_Centered(int x, int y, char *text);
int Print_Text_To_Pic(int x, int y, char *text, unsigned pic);
int UI_Add_Slider_Near(int x, int y, int scale_min, int scale_max, int min, int max, int width, int height,
                       int *value, char *hotkey, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Hit_Test(void);
int UI_Wait_Any_Input(void);
void Play_Click_Sound(void);
void Play_Click_Sound_Casting(void);
int Spell_Load_Anim(int spell);
void MagicScr_Build_Global_Ench_List(void);
int Spell_Dispel_Resistance(int strength, int player, int realm);
int Spell_Casting_Cost(int player, int spell, int in_combat);
int Combat_Range_Multiplier(int player);
void Battlefield_Draw_Terrain(void);
void Remap_Build_Range(int first, int last);

/* (n, hero types) -> the hero type picked: "<name> the <type>" for each dead hero in a list dialog (6 rows, more
   with a scroll) over the map */
int Resurrection_Pick_Hero(int n, int *hero_types)
{
    int choice;
    int shown;
    int more;
    char *names[31];
    char *name_text;
    int i;

    name_text = Near_Alloc_Reset(0x4d8);              /* 31 names x 40 bytes */
    for (i = 0; i < n; i++) {
        far_strcpy(name_text + i * 40, 0, p_hero_names[hero_types[i]].name);
        strcat(name_text + i * 40, " the ");
        strcat(name_text + i * 40, unit_types[hero_types[i]].name);
        names[i] = name_text + i * 40;
    }
    strcpy(name_text + i * 40, "");
    names[i] = name_text + i * 40;
    if (n > 6) {
        shown = 6;
        more = 1;
    } else {
        shown = n;
        more = 0;
    }
    choice = List_Select_Dialog(shown, names, more, "Select a hero to Ressurect");
    UI_Clear_Controls();
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Redraw();
    Screen_Present();
    UI_Set_Redraw_Fn(Map_Redraw, 2);
    return hero_types[choice];
}

/* () the overland extra-power dialog's pictures: SPELLSCR.LBX 5 frame, 3 / 4 bar ends, 0x2a button */
void OverlandPower_Load(void)
{
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    ui_temp_pic_b = Heap_Alloc_Back(p_scratch, 0x28);
    castui_frame_pic = LBX_Reload_Next("SPELLSCR", 5, p_scratch);
    power_bar_end_pic = LBX_Reload_Next("SPELLSCR", 3, p_scratch);
    power_bar_fill_pic = LBX_Reload_Next("SPELLSCR", 4, p_scratch);
    castui_button_pic = LBX_Reload_Next("SPELLSCR", 0x2a, p_scratch);
    Palette_Cycle_Reset();
}

/* () a frame of the overland dialog: the spellbook, "Additional Power: +N" (N = base cost x 4 x (bar - 3) / 100),
   the casting cost and its reduced cost left, the scrolling bar */
void OverlandPower_Redraw(void)
{
    char text[10];
    char num_text[10];
    int x;
    int y;

    if (cast_ui_slot > 5)                      /* the dialog over the other page; the slot's row */
        x = 5;
    else
        x = 165;
    y = cast_ui_slot % 6 * 22 + 21;
    cast_anim_frame = (cast_anim_frame + 1) % 8;
    Screen_Restore_A800();
    Spellbook_Overland_Redraw(16, 10);
    Draw_Pic_Frame(x - 4, y + 1, castui_frame_pic);
    cast_power_total = ((long)power_bar_width - POWER_BAR_MIN) * ((long)p_spell_data[ui_spell].base_cost << 2) / 100;
    strcpy(text, "+");
    itoa(cast_power_total, num_text, 10);
    strcat(text, num_text);
    Set_Text_Shadow_Color(0xf6);
    Font_Select_Shadow_UL(4, 4, 4, 4);
    Set_Font_Color(6);
    Print_Text(x + 4, y + 8, "Additional Power:");
    Print_Text_Right(x + 0x90, y + 8, text);
    Set_Text_Shadow_Color(0xfb);
    Font_Select_Shadow(4, 4, 4, 4);
    Set_Font_Color(6);
    Print_Text(x + 4, y + 8, "Additional Power:");
    Print_Text_Right(x + 0x90, y + 8, text);
    cast_power_total += p_spell_data[ui_spell].base_cost;
    wizards[PLAYER_HUMAN].casting_cost_initial = cast_power_total;
    wizards[PLAYER_HUMAN].casting_cost_left = cast_power_total
        - (int)((long)cast_power_total * Spell_Cost_Reduction_Pct(0, ui_spell) / 100);
    Pic_Decode_Frame_To_Bitmap(power_bar_fill_pic, ui_temp_pic_b);
    Set_Clip_Rect(x + 8, 0, x + power_bar_width + 8, 199);
    Draw_Bitmap_Clipped(x + cast_anim_frame + 8, y + 0x17, ui_temp_pic_b);
    Draw_Bitmap_Clipped(x + cast_anim_frame - 40, y + 0x17, ui_temp_pic_b);
    Draw_Pic_Frame(x + power_bar_width + 5, y + 0x16, power_bar_end_pic);
    Reset_Clip_Rect();
}

/* (spell, slot) the overland extra-power dialog of a variable-cost spell: the slider until the button; sets the
   human's casting cost (up to 5x the base cost). BUG #431: the cost is base_cost + the extra, less the retort reduction -
   never Spell_Casting_Cost's x 3 / 2 under Evil Omens, so a variable Life / Nature spell escapes it (the combat
   dialog too); clarified using ReMoM repo
   Numbered: BUG #406 (BUGS_AND_QUIRKS.txt) */
void Overland_Extra_Power_Dialog(int spell, int slot)
{
    int input;
    int button;
    int y;
    int x;

    cast_ui_slot = slot;
    if (slot > 5)
        x = 5;
    else
        x = 165;
    y = slot % 6 * 22 + 21;
    OverlandPower_Load();
    cast_anim_frame = 0;
    cast_power_total = p_spell_data[spell].base_cost;
    power_bar_width = POWER_BAR_MIN;
    ui_spell = spell;
    UI_Clear_Controls();
    UI_Add_Slider_Near(x + 12, y + 22, 0, POWER_BAR_WIDTH, POWER_BAR_MIN, POWER_BAR_MAX, POWER_BAR_WIDTH, 7, &power_bar_width, "", -1);
    button = UI_Add_Button(x + 0x7b, y + 0x12, "", castui_button_pic, "O", -1);
    UI_Set_Redraw_Fn(OverlandPower_Redraw, 2);
    cast_ui_done = 0;
    cast_power_total = ((long)power_bar_width - POWER_BAR_MIN) * ((long)p_spell_data[ui_spell].base_cost << 2) / 100;
    cast_power_total += p_spell_data[ui_spell].base_cost;
    wizards[PLAYER_HUMAN].casting_cost_initial = cast_power_total;
    wizards[PLAYER_HUMAN].casting_cost_left = cast_power_total
        - (int)((long)cast_power_total * Spell_Cost_Reduction_Pct(0, ui_spell) / 100);
    while (cast_ui_done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input == button)
            cast_ui_done = 1;
        if (cast_ui_done == 0) {
            Set_Draw_Page();
            OverlandPower_Redraw();
            Screen_Present();
        }
        Wait_Ticks(2);
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Heap_Release();
}

/* () the combat extra-power dialog's pictures: as OverlandPower_Load plus a 0x442-paragraph work picture and the
   six realm icons decoded (spellbook_realm_icon_pics) */
void CastUI_Power_Load(void)
{
    int i;

    Heap_Mark(p_scratch);
    ui_temp_pic_b = Heap_Alloc_Back(p_scratch, 0x28);
    castui_frame_pic = LBX_Reload_Next("SPELLSCR", 5, p_scratch);
    power_bar_end_pic = LBX_Reload_Next("SPELLSCR", 3, p_scratch);
    power_bar_fill_pic = LBX_Reload_Next("SPELLSCR", 4, p_scratch);
    castui_button_pic = LBX_Reload_Next("SPELLSCR", 0x2a, p_scratch);
    ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 0x442);
    for (i = 0; i < 6; i++) {
        spellbook_realm_icon_pics[i] = Heap_Alloc_Back(p_scratch, 4);
        Pic_Decode_Frame_To_Bitmap(spellbook_realm_icon_src_pics[i], spellbook_realm_icon_pics[i]);
    }
    Palette_Cycle_Reset();
}

/* () a frame of the combat dialog: as OverlandPower_Redraw, N = the allowed extra power x (bar - 3) / 100; only a
   wizard (caster > 19) gets the cost reduction */
void CastUI_Power_Redraw(void)
{
    char text[10];
    char num_text[10];
    int x;                                      /* the dialog's screen x */
    int y;

    if (cast_ui_slot > 5)                      /* the dialog over the other page; the slot's row */
        x = 5;
    else
        x = 165;
    y = cast_ui_slot % 6 * 22 + 21;
    cast_anim_frame = (cast_anim_frame + 1) % 8;
    Screen_Restore_A800();
    Spellbook_Combat_Redraw(16, 10, cast_ui_caster);
    Draw_Pic_Frame(x - 4, y + 1, castui_frame_pic);
    cast_power_total = ((long)power_bar_width - POWER_BAR_MIN) * cast_power_extra / 100;
    strcpy(text, "+");
    itoa(cast_power_total, num_text, 10);
    strcat(text, num_text);
    Set_Text_Shadow_Color(0xf6);
    Font_Select_Shadow_UL(4, 4, 4, 4);
    Set_Font_Color(6);
    Print_Text(x + 4, y + 8, "Additional Power:");
    Print_Text_Right(x + 0x90, y + 8, text);
    Set_Text_Shadow_Color(0xfb);
    Font_Select_Shadow(4, 4, 4, 4);
    Set_Font_Color(6);
    Print_Text(x + 4, y + 8, "Additional Power:");
    Print_Text_Right(x + 0x90, y + 8, text);
    cast_power_total += p_spell_data[ui_spell].base_cost;
    if (cast_ui_caster > (CASTER_WIZARD - 1))
        wizards[PLAYER_HUMAN].casting_cost_left = cast_power_total
            - (int)((long)cast_power_total * Spell_Cost_Reduction_Pct(0, ui_spell) / 100);
    else
        wizards[PLAYER_HUMAN].casting_cost_left = cast_power_total;
    Pic_Decode_Frame_To_Bitmap(power_bar_fill_pic, ui_temp_pic_b);
    Set_Clip_Rect(x + 8, 0, x + power_bar_width + 8, 199);
    Draw_Bitmap_Clipped(x + cast_anim_frame + 8, y + 0x17, ui_temp_pic_b);
    Draw_Bitmap_Clipped(x + cast_anim_frame - 40, y + 0x17, ui_temp_pic_b);
    Draw_Pic_Frame(x + power_bar_width + 5, y + 0x16, power_bar_end_pic);
    Reset_Clip_Rect();
}

/* (spell, slot, caster) -> the spell's total cost: the combat extra-power dialog. A unit may add up to its spell
   points less the base cost (at most 4x base); a wizard up to 4x base, limited by the skill left (with the cost
   reduction) and by the mana left at the range multiplier. BUG #431: base_cost throughout (no Evil Omens, as the
   overland dialog), and the mana limit overflows at the own fortress with over 16383 mana (see
   Combat_Spellbook_Build); clarified using ReMoM repo */
int Combat_Extra_Power_Dialog(int spell, int slot, int caster)
{
    int input;
    int button;
    int y;
    int x;                                      /* first the cost reduction, then the range multiplier */

    cast_ui_slot = slot;
    cast_ui_caster = caster;
    ui_spell = spell;
    Screen_Copy_Front();
    Screen_Save_A800();
    if (caster > CASTER_WIZARD - 1) {
        x = Spell_Cost_Reduction_Pct(0, ui_spell);
        cast_power_extra = p_spell_data[ui_spell].base_cost << 2;
        if ((int)((long)wizards[PLAYER_HUMAN].combat_skill_left * (x + 100L) / 100) < cast_power_extra)
            cast_power_extra = (int)((long)wizards[PLAYER_HUMAN].combat_skill_left * (x + 100L) / 100);
        while (cast_power_extra + p_spell_data[spell].base_cost
               - (int)(((long)cast_power_extra + p_spell_data[spell].base_cost) * x / 100)
               > wizards[PLAYER_HUMAN].combat_skill_left)
            cast_power_extra--;
        x = Combat_Range_Multiplier(0);
        if ((int)((long)wizards[PLAYER_HUMAN].mana_reserve * 10 / x) - Spell_Casting_Cost(0, spell, 1) < cast_power_extra)
            cast_power_extra = (int)((long)wizards[PLAYER_HUMAN].mana_reserve * 10 / x) - Spell_Casting_Cost(0, spell, 1);
    } else {
        cast_power_extra = p_battle_units[caster].spell_points - p_spell_data[ui_spell].base_cost;
        if (p_spell_data[ui_spell].base_cost << 2 < cast_power_extra)
            cast_power_extra = p_spell_data[ui_spell].base_cost << 2;
    }
    if (slot > 5)
        x = 5;
    else
        x = 165;
    y = slot % 6 * 22 + 21;
    CastUI_Power_Load();
    cast_anim_frame = 0;
    cast_power_total = p_spell_data[spell].base_cost;
    power_bar_width = POWER_BAR_MIN;
    UI_Clear_Controls();
    UI_Add_Slider_Near(x + 12, y + 22, 0, POWER_BAR_WIDTH, POWER_BAR_MIN, POWER_BAR_MAX, POWER_BAR_WIDTH, 7, &power_bar_width, "", -1);
    button = UI_Add_Button(x + 0x7b, y + 0x12, "", castui_button_pic, "O", -1);
    UI_Set_Redraw_Fn(CastUI_Power_Redraw, 2);
    cast_ui_done = 0;
    while (cast_ui_done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input == button)
            cast_ui_done = 1;
        if (cast_ui_done == 0) {
            Set_Draw_Page();
            CastUI_Power_Redraw();
            Screen_Present();
        }
        Wait_Ticks(2);
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Heap_Release(p_scratch);
    Battlefield_Draw_Terrain();
    return cast_power_total;
}

/* (spell) the Disjunction / Spell Binding screen: the box (SPELLSCR.LBX 1) sized to the opponents over the map, the
   spell animation, the opponents' gems (LILWIZ), MAGIC.LBX 6 / 0x33, the RESOURCE.LBX scroll arrows; the global
   enchantments counted per owner and split into one run per opponent */
void Disjunction_Screen_Load(int spell)
{
    int player;
    int box_x;
    int box_y;
    int i;

    Near_Alloc_Reset_All();
    Minimap_Alloc_Pic();
    Near_Alloc_Mark();
    p_magic_enchant_spell = (int *)Near_Alloc(0x120);   /* 144 = 6 wizards x 24 globals, as ints */
    p_magic_enchant_owner = Near_Alloc(0x90);
    Heap_Mark(p_scratch);
    disjunction_box_pic = LBX_Reload_Next("SPELLSCR", 1, p_scratch);
    box_x = 50;
    box_y = (5 - num_players) * 23;            /* half a 46-pixel row per missing wizard: the rows stay centred */
    Set_Draw_Page();
    Map_Redraw();
    Set_Clip_Rect(0, 0, 319, box_y + (num_players - 2) * 46 + 56);
    Draw_Pic_Clipped(box_x, box_y, disjunction_box_pic);
    Set_Clip_Rect(0, box_y + (num_players - 2) * 46 + 56, 319, 199);
    Draw_Pic_Clipped(box_x, box_y + (num_players - 2) * 46 - 138, disjunction_box_pic);
    if (box_y > 0) {
        Draw_Line(box_x, box_y - 1, box_x + 208, box_y - 1, 1);
        Draw_Line(box_x, box_y + (num_players - 2) * 46 + 62, box_x + 208, box_y + (num_players - 2) * 46 + 62, 1);
    }
    Screen_Save_A800();
    Set_Draw_Page_Front();
    Heap_Release(p_scratch);
    Heap_Mark(p_scratch);
    disj_highlight_pic = LBX_Reload_Next("SPELLSCR", 7, p_scratch);
    Spell_Load_Anim(spell);
    for (player = 1; player < num_players; player++)
        magic_gem_pics[player] = LBX_Reload_Next("Lilwiz", LILWIZ_ENTRY(wizards[player].portrait, wizards[player].banner_color),
                                                 p_scratch);
    magic_gem_frame_pic = LBX_Reload_Next("MAGIC", 6, p_scratch);
    magic_overlay_pic = LBX_Reload_Next("MAGIC", 0x33, p_scratch);
    ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 100);
    ui_temp_pic_b = Heap_Alloc_Back(p_scratch, 100);
    list_dialog_top_arrow_pic = LBX_Reload_Next("RESOURCE", 0x20, p_scratch);
    list_dialog_bottom_arrow_pic = LBX_Reload_Next("RESOURCE", 0x21, p_scratch);
    for (i = 0; i < num_players; i++)
        disj_owner_counts[i] = 0;
    for (i = 0; i < 0x90; i++)
        p_magic_enchant_spell[i] = -1;
    MagicScr_Build_Global_Ench_List();
    for (i = 0; i < magic_enchant_count; i++)
        disj_owner_counts[p_magic_enchant_owner[i]]++;
    disj_owner_lists[0] = p_magic_enchant_spell + disj_owner_counts[0];
    disj_owner_lists[1] = p_magic_enchant_spell + (disj_owner_counts[0] + disj_owner_counts[1]);
    disj_owner_lists[2] = p_magic_enchant_spell + (disj_owner_counts[0] + disj_owner_counts[1] + disj_owner_counts[2]);
    disj_owner_lists[3] = p_magic_enchant_spell
        + (disj_owner_counts[0] + disj_owner_counts[1] + disj_owner_counts[2] + disj_owner_counts[3]);
    for (i = 0; i < num_players - 1; i++)
        disj_list_first[i] = 0;
}

/* () a frame of the Disjunction screen: the selected row, each opponent's gem (known and alive, or with spells to
   show; else the empty frame or the dead overlay) and up to three of its spells, the title ("Select a spell to
   disjunct." / "bind.") or the result */
void Disjunction_Screen_Redraw(void)
{
    int box_x;
    int box_y;
    int i;
    int x;
    int y;
    char colors[6];
    char text[20];
    int player;

    box_x = 50;
    box_y = (5 - num_players) * 23;            /* half a 46-pixel row per missing wizard: the rows stay centred */
    colors[0] = 0xf6;
    for (i = 1; i < 6; i++)
        colors[i] = i + 0xb1;
    Screen_Restore_A800();
    if (disjunction_choice > 0 && disjunction_choice <= (num_players - 1) * 3)
        Draw_Pic_Frame(box_x + 0x49, box_y + (disjunction_choice - 1) * 13 + (disjunction_choice - 1) / 3 * 7 + 16,
                       disj_highlight_pic);
    for (player = 1; player < num_players; player++) {
        x = box_x + 8;
        y = box_y + (player - 1) * 46 + 14;
        if ((wizards[PLAYER_HUMAN].contacted[player] == 1 && p_fortresses[player].active == 1) || disj_owner_counts[player] > 0)
            Draw_Pic_Frame(x, y, magic_gem_pics[player]);
        else if (p_fortresses[player].active == 1)
            Draw_Pic_Frame(x, y, magic_gem_frame_pic);
        else
            Draw_Pic_Frame(x - 1, y - 1, magic_overlay_pic);
        if (disj_owner_counts[player] != 0) {
            x = box_x + 0x4c;
            y = box_y + (player - 1) * 46 + 18;
            for (i = 0; i < 3; i++) {
                if (disj_owner_counts[player] <= i + disj_list_first[player - 1])
                    break;
                Font_Select(2, 1, 0, 0);
                Set_Font_Color(0xba);
                far_strcpy(text, 0, p_spell_data[disj_owner_lists[player - 1][i + disj_list_first[player - 1]]].name);
                Print_Text(x, y + i * 13, text);
            }
        }
    }
    Font_Select_Palette(4, colors);
    if (cast_screen_live == 1) {
        strcpy(message_text, "Select a spell to ");
        strcat(message_text, ui_spell != SPELL_SPELL_BINDING ? "disjunct." : "bind.");
    } else if (cast_ui_slot == 1) {
        strcpy(message_text, ui_spell != SPELL_SPELL_BINDING ? "Spell succesfully dispelled." : "Spell succesfully bound.");
        if (cast_result_shown == 1)
            Draw_Pic_Frame(box_x + 0x49, box_y + (disjunction_choice - 1) * 13 + (disjunction_choice - 1) / 3 * 7 + 16,
                           disj_highlight_pic);
    } else {
        strcpy(message_text, ui_spell != SPELL_SPELL_BINDING ? "Disjunction unsuccesful" : "Spell Binding unsuccesful");
    }
    Print_Text_Centered(box_x + 0x69, box_y + 3, message_text);
}

/* (spell, caster) Disjunction / Disjunction True / Spell Binding: pick another wizard's global enchantment (a row
   of three per opponent, scrolled by the arrows), then the roll; "There are no global spells to ..." when none */
/* int with NO return value: Overland_Cast_Spell (OVERCAST.C) stores the result as its 'ok' (which gates
   Nature's Wrath), so it gets whatever AX the last statement left - BUG #408 */
int Screen_Disjunction(int spell, int caster)
{
    int input;
    int row;
    int box_x;
    int box_y;
    int target_spell;
    int target;
    int x1;
    int y1;
    int up_buttons[4];
    int down_buttons[4];
    int row_controls[12];
    int i;

    box_x = 50;
    box_y = (5 - num_players) * 23;            /* half a 46-pixel row per missing wizard: the rows stay centred */
    cast_screen_live = 1;
    cast_result_shown = 0;
    ui_spell = spell;
    Disjunction_Screen_Load(spell);
    if (magic_enchant_count - disj_owner_counts[0] <= 0) {
        strcpy(message_text, "There are no global spells to ");
        strcat(message_text, spell != SPELL_SPELL_BINDING ? "disjunct." : "bind.");
        Message_Box(message_text);
        target_spell = -1;
        target = -1;
    } else {
        UI_Set_Redraw_Fn(Disjunction_Screen_Redraw, 2);
        UI_Clear_Controls();
        for (i = 0; i < (num_players - 1) * 3; i++) {
            x1 = box_x + 0x4a;
            y1 = box_y + i % 3 * 13 + i / 3 * 46 + 16;
            row_controls[i] = UI_Add_Region(x1, y1, x1 + 0x78, y1 + 13, "", -1);
        }
        for (i = 0; i < num_players - 1; i++) {
            if (disj_list_first[i] > 0)
                up_buttons[i] = UI_Add_Button(box_x + 0x3d, box_y + i * 46 + 0x13, "", list_dialog_top_arrow_pic, "", -1);
            else
                up_buttons[i] = UI_NO_CONTROL;
            if (disj_list_first[i] + 3 < disj_owner_counts[i + 1])
                down_buttons[i] = UI_Add_Button(box_x + 0x3d, box_y + i * 46 + 0x2b, "", list_dialog_bottom_arrow_pic,
                                                "", -1);
            else
                down_buttons[i] = UI_NO_CONTROL;
        }
        cast_ui_done = 0;
        while (cast_ui_done == 0) {
            input = UI_Get_Input();
            disjunction_choice = UI_Hit_Test();
            if ((disjunction_choice - 1) % 3 >= disj_owner_counts[(disjunction_choice - 1) / 3 + 1])
                disjunction_choice = 0;
            for (i = 0; i < (num_players - 1) * 3; i++) {
                if (row_controls[i] == input && disjunction_choice != 0) {
                    Play_Click_Sound();
                    target_spell = disj_owner_lists[i / 3][i % 3 + disj_list_first[i / 3]];
                    target = i / 3 + 1;
                    row = i;
                    cast_ui_done = 1;
                }
            }
            for (i = 0; i < num_players - 1; i++) {
                if (up_buttons[i] == input && disj_list_first[i] > 0) {
                    Play_Click_Sound_Casting();
                    disj_list_first[i]--;
                }
                if (down_buttons[i] == input && disj_list_first[i] + 3 < disj_owner_counts[i + 1]) {
                    Play_Click_Sound_Casting();
                    disj_list_first[i]++;
                }
            }
            UI_Clear_Controls();
            for (i = 0; i < (num_players - 1) * 3; i++) {
                x1 = box_x + 0x4a;
                y1 = box_y + i % 3 * 13 + i / 3 * 46 + 16;
                row_controls[i] = UI_Add_Region(x1, y1, x1 + 0x78, y1 + 13, "", -1);
            }
            for (i = 0; i < num_players - 1; i++) {
                if (disj_list_first[i] > 0)
                    up_buttons[i] = UI_Add_Button(box_x + 0x3d, box_y + i * 46 + 0x13, "", list_dialog_top_arrow_pic,
                                                  "", -1);
                else
                    up_buttons[i] = UI_NO_CONTROL;
                if (disj_list_first[i] + 3 < disj_owner_counts[i + 1])
                    down_buttons[i] = UI_Add_Button(box_x + 0x3d, box_y + i * 46 + 0x2b, "",
                                                    list_dialog_bottom_arrow_pic, "", -1);
                else
                    down_buttons[i] = UI_NO_CONTROL;
            }
            if (cast_ui_done == 0) {
                Set_Draw_Page();
                Disjunction_Screen_Redraw();
                Screen_Present();
            }
        }
    }
    UI_Clear_Controls();
    row_controls[0] = UI_Add_Region(0, 0, 319, 199, "", -1);
    if (target_spell > -1) {
        if (spell != SPELL_SPELL_BINDING)
            Disjunction_Resolve(row, caster, spell, target);
        else
            Spell_Binding_Resolve(row, caster, spell, target);
    }
    Map_Redraw_Dissolve();
    Heap_Release(p_scratch);
    Near_Alloc_Release();
}

/* (row, caster, spell, target owner) Spell Binding: strength = casting cost x 20; bound if Random(250) <=
   Spell_Dispel_Resistance(250 x strength / (strength + the target's base cost), owner, Spell Binding's own realm - not
   the bound global's; the AI path in OVERCAST.C uses the target's); the global moves to
   the caster; the name shrinks into the caster's side while the animation plays */
void Spell_Binding_Resolve(int row, int caster, int spell, int target)
{
    int bound;
    char *globals;
    int box_x;
    int text_x;
    int box_y;
    int y;
    int x;
    int input;
    int target_spell;
    char spell_name[20];
    int i;
    int n;                                      /* the strength, the chance, then half the name's width x 5 */

    box_x = 50;
    box_y = (5 - num_players) * 23;            /* half a 46-pixel row per missing wizard: the rows stay centred */
    Remap_Build_Range(9, 10);
    target_spell = disj_owner_lists[target - 1][row % 3 + disj_list_first[target - 1]];
    wizards[caster].casting_cost_initial *= 20;
    bound = 0;
    n = p_spell_data[target_spell].base_cost + wizards[caster].casting_cost_initial;
    n = (int)((long)wizards[caster].casting_cost_initial * 250 / n);
    n = Spell_Dispel_Resistance(n, target, p_spell_data[spell].realm);   /* BUG #38: the target's resistance RAISES the chance
                                                   (OVERCAST.C's AI path adds it to the cost) */
    if (Random(250) <= n) {
        bound = 1;
        globals = (char *)&wizards[target].globals;
        globals[(int)p_spell_data[target_spell].effect_param] = 0;
        globals = (char *)&wizards[caster].globals;
        globals[(int)p_spell_data[target_spell].effect_param] = caster + 1;
    }
    cast_ui_slot = bound;
    text_x = box_x + 0x49;
    y = box_y + row * 13 + row / 3 * 7 + 16;
    far_strcpy(spell_name, 0, p_spell_data[target_spell].name);
    n = strlen(spell_name);
    n = n * 5 / 2;
    x = text_x + n - 18;
    input = 0;
    for (i = 0; i < 54 && input == 0; i++) {
        input = UI_Get_Input();
        Timer_Mark();
        Set_Draw_Page();
        Disjunction_Screen_Redraw();
        if (i > 23)
            cast_screen_live = 0;
        if (bound == 1) {
            Pic_Create_Blank(0x7e, 12, ui_temp_pic_b);
            if (i > 23 && i < 36) {
                cast_result_shown = 1;
                Font_Select(2, 1, 0, 0);
                Print_Text_To_Pic(3, 2, spell_name, ui_temp_pic_b);
                Pic_Scale_Percent(ui_temp_pic_b, 100 - (i - 23) * 8, 100);
                Draw_Bitmap(text_x + (i - 23) * n * 8 / 100, y, ui_temp_pic_b);
            }
        }
        Reset_Clip_Rect();
        if (i < 36)
            Draw_Pic_Frame(x, y - 1, p_anim_pic);
        else
            Draw_Pic_Clipped(x - x / 12 * (i - 35), y, p_anim_pic);
        Screen_Present();
        Wait_Ticks(2);
    }
    if (bound == 1)
        cast_result_shown = 1;
    UI_Wait_Any_Input();
}

/* (row, caster, spell, target owner) Disjunction / Disjunction True (casting cost x 3): dispelled if Random(250) <=
   Spell_Dispel_Resistance(250 x cost / (cost + the target's base cost), owner, Disjunction's own realm - not the
   dispelled global's); the row dissolves */
void Disjunction_Resolve(int row, int caster, int spell, int target)
{
    int success;
    char *globals;
    int box_x;
    int text_x;
    int box_y;
    int y;
    int input;
    int target_spell;
    int i;
    int n;                                      /* the strength, then the chance */

    box_x = 50;
    box_y = (5 - num_players) * 23;            /* half a 46-pixel row per missing wizard: the rows stay centred */
    target_spell = disj_owner_lists[target - 1][row % 3 + disj_list_first[target - 1]];
    if (spell == SPELL_DISJUNCTION_TRUE)
        wizards[caster].casting_cost_initial *= 3;
    success = 0;
    n = p_spell_data[target_spell].base_cost + wizards[caster].casting_cost_initial;
    n = (int)((long)wizards[caster].casting_cost_initial * 250 / n);
    n = Spell_Dispel_Resistance(n, target, p_spell_data[spell].realm);   /* BUG #38: the target's resistance RAISES the chance
                                                   (OVERCAST.C's AI path adds it to the cost) */
    if (Random(250) <= n) {
        success = 1;
        globals = (char *)&wizards[target].globals;
        globals[(int)p_spell_data[target_spell].effect_param] = 0;
    }
    cast_ui_slot = success;
    text_x = box_x + 0x49;
    y = box_y + row * 13 + row / 3 * 7 + 16;
    input = 0;
    for (i = 0; i < 24 && input == 0; i++) {
        input = UI_Get_Input();
        Timer_Mark();
        Set_Draw_Page();
        Disjunction_Screen_Redraw();
        if (i > 10)
            cast_screen_live = 0;
        if (success == 1 && i > 10) {
            Pic_Decode_Frame_To_Bitmap(disj_highlight_pic, ui_temp_pic_a);
            if ((i - 11) * 10 < 100)
                Pic_Dissolve(ui_temp_pic_a, (i - 11) * 10);
            Draw_Bitmap(text_x, y, ui_temp_pic_a);
        }
        Draw_Pic_Frame(text_x - 4, y - 12, p_anim_pic);
        Screen_Present();
        Wait_Ticks(2);
    }
    cast_result_shown = 1;
    UI_Wait_Any_Input();
}
