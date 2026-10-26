/* COMBDRAW.C - WIZARDS.EXE overlay code segment 80d6: the combat screen - the battlefield and its figures, both sides'
   combat enchantment icons, the side names in their banner colors, the command buttons (locked during auto
   combat), the active unit's panel (figure, melee / ranged / moves, name, health bar; the Magic Vortex hint when no
   unit is active), the skill / mana / range panel, and the animation counters.
   This module's data (DS:56e8): the name color table and its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 80d6 src/wizards/COMBDRAW.C */
#include <stdlib.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Combat_Draw_Screen(void);
void Combat_Draw_Unit_Panel(void);
void Combat_Draw_Buttons(void);
void Combat_Draw_Mana_Panel(void);
void Combat_Draw_Unit_Stats(void);
void Combat_Draw_Battle_Units(void);
void Combat_Draw_Health_Bar(int bu, int x, int y);

#include "GAME.H"

#define ENEMY_KIND_MONSTERS 1          /* a neutral enemy with a fantastic unit */
#define ENEMY_KIND_RAIDERS 2

/* text colors 1..4 of a side's name, by banner color (row 5 = neutral) */
char combat_name_colors[6][4] = {
    { 0x61, 0x62, 0x63, 0x64 },
    { 0x42, 0x43, 0x44, 0x45 },
    { 0x21, 0x22, 0x23, 0x24 },
    { 0xc9, 0xca, 0xcb, 0xa6 },
    { 0xa0, 0xa1, 0xa2, 0xa3 },
    { 0x1c, 0x1b, 0x1a, 0x19 }
};                                              /* DS:56e8 */

extern char *lair_type_names[];                 /* DS:4ab4 */
extern int battlefield_water_anim_frame;                  /* DS:707a */
extern int combat_cursor_anim;                  /* DS:7080 */
extern int combat_vortex_frame;             /* DS:7088 */
extern int combat_effect_anim_frame;                  /* DS:708a */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_site_index;                         /* DS:9286 */
extern unsigned p_pic_work;                     /* DS:998c */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int combat_all_units_immobile;                     /* DS:c406 */
extern char combat_city_name[];                 /* DS:c40a  the attacked city's name (Combat_Main) */
extern unsigned combat_use_lock_pic;            /* DS:c424 */
extern unsigned combat_flee_lock_pic;           /* DS:c426 */
extern unsigned combat_done_lock_pic;           /* DS:c42a */
extern unsigned combat_wait_lock_pic;           /* DS:c42c */
extern unsigned combat_spell_lock_pic;          /* DS:c42e */
extern int combat_auto;                         /* DS:c432 */
extern int combat_human_side;                   /* DS:c436 */
extern int combat_wizard_can_cast;              /* DS:c438 */
extern int combat_def_icon_first;               /* DS:c43a */
extern int combat_att_icon_first;               /* DS:c43c */
extern int combat_enemy_wizard;                 /* DS:c43e */
extern int combat_att_icon_count;               /* DS:c482 */
extern int combat_def_icon_count;               /* DS:c484 */
extern struct PicHelp combat_def_icons[15];     /* DS:c486 */
extern struct PicHelp combat_att_icons[15];     /* DS:c4c2 */
extern unsigned combat_use_button_pic;          /* DS:c576 */
extern unsigned combat_flee_button_pic;         /* DS:c578 */
extern unsigned combat_auto_button_pic;         /* DS:c57a */
extern unsigned combat_done_button_pic;         /* DS:c57c */
extern unsigned combat_wait_button_pic;         /* DS:c57e */
extern unsigned combat_spell_button_pic;        /* DS:c580 */
extern int combat_active_unit;                  /* DS:c582 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern unsigned combat_small_attack_icons[];    /* DS:c5a8 */
extern int combat_human_turn;                   /* DS:c79a */
extern unsigned combat_small_move_icons[];      /* DS:c7aa */
extern unsigned combat_hit_bar_pic;             /* DS:c7b0 */
extern int combat_info_count;                   /* DS:c7c6 */
extern unsigned combat_vortex_pic;          /* DS:d13c */

int Print_Text_Centered(int x, int y, char *text);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Combat_Draw_Battlefield(void);
int Combat_Info_Count(void);
void Combat_Draw_Hover_Info(void);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
void Set_Line_Spacing(int line_gap);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Get_Opaque_Bounds(unsigned pic, int *x, int *y, int *w, int *h);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Set_Space_Width(int width);
void Figure_Render_Frame(int slot, int facing, int owner, int aura, int frame);
void Unit_Pic_Apply_Effect(int effect);
void Pic_Outline_Recolor(unsigned pic, int color);
void Pic_Set_Remap_Flag(unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
int Unit_Has_Combat_Ability(int bu);
int Combat_Range_Multiplier(int player);
int Print_Text_Right(int x, int y, char *text);
int Unit_Melee_Icon(int bu);
int Unit_Ranged_Icon(int bu);
void Print_Half_Moves(int x, int y, int half_moves, int align);
int Unit_Combat_Move_Type(int bu);
void Sprite_List_Clear(void);
void Combat_Queue_Terrain_Objects_A(void);
void Combat_Queue_Terrain_Objects_B(void);
void Combat_Update_See_Invisible(void);
void Unit_Enchant_Aura_Color(int bu);
void Unit_Set_Anim_Flags(int bu);
int Unit_Combat_Status_Icon(int bu);
void Unit_Update_Visibility(int bu);
void Combat_Queue_Unit_Figure(int slot, int x, int y, int dest_x, int dest_y, int step, int figure, int figures,
                              int owner, int aura, int intensity, int frame, int kills, int hit_set);
void Combat_Queue_Unit_Status_Icon(int x, int y, int dest_x, int dest_y, int step, int icon);
void Combat_Queue_Missiles(void);
void Combat_Queue_Vortices(void);
void Combat_Queue_Features_And_Walls(void);
void Outline_Cycle_Tick(void);
void Draw_Line(int x1, int y1, int x2, int y2, int color);

/* () the whole combat screen: figures, battlefield, buttons, both sides' enchantment icons (four each, the human's on
   the right), the enemy's name ("Monsters" / "Raiders" for neutrals, the city's or lair's name) and the human's,
   the mana panel, the hover info, the active unit's panel or "All units are immobilized"; then the animation
   counters step */
void Combat_Draw_Screen(void)
{
    char colors[6];
    int icon_x;
    int found;
    int enemy_kind;
    int help_base;
    int i;
    int pos;

    Set_Draw_Page();
    Combat_Draw_Battle_Units();
    Combat_Draw_Battlefield();
    Reset_Clip_Rect();
    combat_info_count = Combat_Info_Count();
    Combat_Draw_Buttons();
    /* the right icon row: help slots 9-12, the left one 13-16 */
    if (combat_attacker_player == current_player) {
        icon_x = 247;
        help_base = 9;
    } else {
        icon_x = 8;
        help_base = 13;
    }
    help_entries[9].id = -1;
    help_entries[10].id = -1;
    help_entries[11].id = -1;
    help_entries[12].id = -1;
    help_entries[13].id = -1;
    help_entries[14].id = -1;
    help_entries[15].id = -1;
    help_entries[16].id = -1;
    for (i = 0; i + combat_att_icon_first < combat_att_icon_count && i < 4; i++) {
        Draw_Pic_Frame(icon_x + i * 17, 179, combat_att_icons[i + combat_att_icon_first].pic);
        help_entries[help_base + i].id = combat_att_icons[i + combat_att_icon_first].help;
    }
    /* the right icon row: help slots 9-12, the left one 13-16 */
    if (combat_defender_player == current_player) {
        icon_x = 247;
        help_base = 9;
    } else {
        icon_x = 8;
        help_base = 13;
    }
    for (i = 0; i + combat_def_icon_first < combat_def_icon_count && i < 4; i++) {
        Draw_Pic_Frame(icon_x + i * 17, 179, combat_def_icons[i + combat_def_icon_first].pic);
        help_entries[help_base + i].id = combat_def_icons[i + combat_def_icon_first].help;
    }
    colors[0] = 0xf1;
    enemy_kind = 0;
    if (combat_enemy_wizard == PLAYER_NEUTRAL) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == combat_enemy_wizard) {
                if (unit_types[p_units[p_battle_units[i].unit_index].type].ability_flags & AFLAG_FANTASTIC) {
                    enemy_kind = ENEMY_KIND_MONSTERS;
                    break;
                } else {
                    enemy_kind = ENEMY_KIND_RAIDERS;
                    break;
                }
            }
        }
    }
    if (combat_site == COMBAT_AT_UNITS) {
        for (i = 1; i < 5; i++)
            colors[i] = combat_name_colors[wizards[combat_enemy_wizard].banner_color][i - 1];
        Font_Select_Palette(4, colors);
        Font_Select_Shadow(4, 15, 0, 0);
        Set_Text_Shadow_Color(0xf1);
        if (combat_enemy_wizard != PLAYER_NEUTRAL)
            Print_Text_Centered(40, 168, wizards[combat_enemy_wizard].wizard_name);
        else if (enemy_kind == ENEMY_KIND_MONSTERS)
            Print_Text_Centered(40, 168, "Monsters");
        else
            Print_Text_Centered(40, 168, "Raiders");
    } else {
        switch (combat_site) {
        case COMBAT_AT_CITY:
            for (i = 1; i < 5; i++)
                colors[i] = combat_name_colors[wizards[combat_enemy_wizard].banner_color][i - 1];
            Font_Select_Palette(4, colors);
            Font_Select_Shadow(4, 15, 0, 0);
            Set_Text_Shadow_Color(0xf1);
            if (combat_enemy_wizard != PLAYER_NEUTRAL)
                Print_Text_Centered(40, 168, wizards[combat_enemy_wizard].wizard_name);
            else if (enemy_kind != 0 && combat_attacker_player == PLAYER_NEUTRAL) {
                if (enemy_kind == ENEMY_KIND_MONSTERS)
                    Print_Text_Centered(40, 168, "Monsters");
                else
                    Print_Text_Centered(40, 168, "Raiders");
            }
            else
                Print_Text_Centered(40, 168, combat_city_name);
            break;
        case COMBAT_AT_LAIR:
            strcpy(p_text_buf, lair_type_names[p_lairs[combat_site_index].type]);
            pos = 0;
            found = 0;
            for (i = 0; i < strlen(p_text_buf); i++) {
                if (p_text_buf[pos] == ' ') {
                    found = 1;
                    break;
                }
                pos++;
            }
            pos++;
            if (found == 0)
                pos = 0;
            p_text_buf[pos] = p_text_buf[pos] - 32;   /* printed from the letter after the first space, upper-cased:
                                                   "chaos node" -> "Node"; with no space from the start: "Tower" */
            for (i = 1; i < 5; i++)
                colors[i] = combat_name_colors[combat_enemy_wizard][i - 1];   /* by player here (elsewhere by banner_color):
                                                   a lair's side is PLAYER_NEUTRAL, the BANNER_NEUTRAL row (5) */
            Font_Select_Palette(4, colors);
            Font_Select_Shadow(4, 15, 0, 0);
            Set_Text_Shadow_Color(0xf1);
            Print_Text_Centered(40, 167, p_text_buf + pos);
            break;
        }
    }
    for (i = 1; i < 5; i++)
        colors[i] = combat_name_colors[wizards[current_player].banner_color][i - 1];
    Font_Select_Palette(4, colors);
    Font_Select_Shadow(4, 15, 0, 0);
    Set_Text_Shadow_Color(0xf1);
    Print_Text_Centered(278, 167, wizards[current_player].wizard_name);
    Combat_Draw_Mana_Panel();
    if (game_config.auto_unit_info == 1)
        Combat_Draw_Hover_Info();
    Palette_Cycle_Color(PAL_CYCLE_COLOR, 0, 0, 0, 0x37, 0, 0, 11);
    if (combat_all_units_immobile == 1 && combat_auto == 0 && combat_active_unit != -1) {
        if (p_battle_units[combat_active_unit].owner != combat_human_side) {
            Combat_Draw_Unit_Panel();
        } else if (combat_human_turn == 1) {
            colors[0] = 0xe3;
            colors[1] = 0xf3;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0xe3);
            Font_Select_Shadow(0, 15, 0, 0);
            Set_Line_Spacing(1);
            Paragraph_Print(84, 167, 58, "All units are immobilized. Select an action.", PARA_LEFT);
        }
    } else {
        Combat_Draw_Unit_Panel();
    }
    combat_vortex_frame = combat_vortex_frame + 1;
    if (combat_vortex_frame > 5)
        combat_vortex_frame = 0;
    combat_effect_anim_frame = combat_effect_anim_frame + 1;
    if (combat_effect_anim_frame > 7)
        combat_effect_anim_frame = 0;
    combat_cursor_anim = combat_cursor_anim + 1;
    if (combat_cursor_anim > 2)
        combat_cursor_anim = 0;
    battlefield_water_anim_frame = battlefield_water_anim_frame + 1;
    if (battlefield_water_anim_frame > 4)
        battlefield_water_anim_frame = 0;
}

/* () the active unit's panel: its figure (centred in 32 x 25, outlined when its type is invisible - not for the
   enchantment), stats, name (a hero's own
   name) and health bar; with no active unit the vortex picture and "Move Magic Vortex 1 Space" */
void Combat_Draw_Unit_Panel(void)
{
    int hero_slot;
    int px;
    int py;
    int pw;
    int ph;
    int unit;
    int unit_type;
    int owner;
    char colors[4];
    int x;
    int y;

    if (combat_active_unit != -1) {
        Figure_Render_Frame(p_battle_units[combat_active_unit].gfx_slot, 2, p_battle_units[combat_active_unit].owner,
                            p_battle_units[combat_active_unit].aura_color, 0);
        Unit_Pic_Apply_Effect(p_battle_units[combat_active_unit].draw_mode);
        if (unit_types[p_units[p_battle_units[combat_active_unit].unit_index].type].ability_flags & AFLAG_INVISIBILITY)
            Pic_Outline_Recolor(p_pic_work, 1);
        unit = p_battle_units[combat_active_unit].unit_index;
        unit_type = p_units[unit].type;
        x = 84;
        y = 173;
        Pic_Get_Opaque_Bounds(p_pic_work, &px, &py, &pw, &ph);
        x = x + ((32 - pw) / 2 - px);
        y = y + ((25 - ph) / 2 - py);
        Pic_Set_Remap_Flag(p_pic_work);
        Draw_Bitmap(x, y, p_pic_work);
        Combat_Draw_Unit_Stats();
        colors[0] = 0xe3;
        colors[1] = 0xf3;
        Font_Select_Palette(0, colors);
        Set_Text_Shadow_Color(0xe3);
        Font_Select_Shadow(0, 15, 0, 0);
        Set_Space_Width(1);
        owner = p_units[unit].owner;
        hero_slot = p_units[unit].hero_slot;
        if (hero_slot != -1)
            strcpy(p_text_buf, wizards[owner].hired_heroes[hero_slot].name);
        else
            strcpy(p_text_buf, unit_types[unit_type].name);
        Print_Text_Centered(112, 167, p_text_buf);
        Draw_Pic_Frame(117, 194, combat_hit_bar_pic);
        Combat_Draw_Health_Bar(combat_active_unit, 118, 195);
    } else {
        Set_Pic_Frame(combat_vortex_pic, 1);
        Pic_Decode_Frame_To_Bitmap(combat_vortex_pic, p_pic_work);
        Set_Pic_Frame(combat_vortex_pic, combat_vortex_frame);
        x = 82;
        y = 170;
        Pic_Get_Opaque_Bounds(p_pic_work, &px, &py, &pw, &ph);
        x = x + ((32 - pw) / 2 - px);
        y = y + ((25 - ph) / 2 - py);
        Draw_Bitmap_Clipped(x, y, p_pic_work);
        colors[0] = 0xe3;
        colors[1] = 0xf3;
        Font_Select_Palette(0, colors);
        Set_Text_Shadow_Color(0xe3);
        Font_Select_Shadow(0, 15, 0, 0);
        Set_Space_Width(1);
        Print_Text(115, 172, "Move");
        Print_Text(115, 178, "Magic");
        Print_Text(115, 184, "Vortex");
        Print_Text(115, 190, "1 Space");
    }
}

/* () the six command buttons; Spell is lit while the active unit can still cast (moves left and 3+ spell points, a
   combat ability or item charges) or the wizard has not cast this turn; Use while there is combat info */
void Combat_Draw_Buttons(void)
{
    if (combat_auto == 1) {
        Set_Pic_Frame(combat_auto_button_pic, 0);
        Draw_Pic_Frame(170, 178, combat_auto_button_pic);
        Draw_Pic_Frame(144, 168, combat_spell_lock_pic);
        Draw_Pic_Frame(170, 168, combat_wait_lock_pic);
        Draw_Pic_Frame(170, 188, combat_done_lock_pic);
        Draw_Pic_Frame(144, 178, combat_use_lock_pic);
        Draw_Pic_Frame(144, 188, combat_flee_lock_pic);
    } else {
        /* BUG #441 (cosmetic): 3 spell points light Spell, but the cheapest combat spells cost 5 (Bless, Fire Bolt, Resist
           Elements, Resist Magic, Star Fires, Weakness) and a unit pays the base cost: with 3 or 4 points the book opens with every spell grayed.
           clarified using ReMoM repo */
        if ((p_battle_units[combat_active_unit].moves > 0
                && (p_battle_units[combat_active_unit].spell_points >= UNIT_MIN_CAST_MANA
                    || Unit_Has_Combat_Ability(combat_active_unit) == 1
                    || p_battle_units[combat_active_unit].item_charges > 0))
                || combat_wizard_can_cast == 1) {
            Set_Pic_Frame(combat_spell_button_pic, 0);
            Draw_Pic_Frame(144, 168, combat_spell_button_pic);
        } else {
            Draw_Pic_Frame(144, 168, combat_spell_lock_pic);
        }
        Set_Pic_Frame(combat_wait_button_pic, 0);
        Draw_Pic_Frame(170, 168, combat_wait_button_pic);
        Set_Pic_Frame(combat_done_button_pic, 0);
        Draw_Pic_Frame(170, 188, combat_done_button_pic);
        if (combat_info_count > 0) {
            Set_Pic_Frame(combat_use_button_pic, 0);
            Draw_Pic_Frame(144, 178, combat_use_button_pic);
        } else {
            Draw_Pic_Frame(144, 178, combat_use_lock_pic);
        }
        Set_Pic_Frame(combat_auto_button_pic, 0);
        Draw_Pic_Frame(170, 178, combat_auto_button_pic);
        Set_Pic_Frame(combat_flee_button_pic, 0);
        Draw_Pic_Frame(144, 188, combat_flee_button_pic);
    }
}

/* () "Skill:" left this turn, "Mana:" the whole reserve, and "Range:" the spell cost multiplier (Combat_Range_Multiplier, tenths)
   as "0.5x", "1x", "1.5x" ... */
void Combat_Draw_Mana_Panel(void)
{
    char digits[6];
    char text[6];
    char colors[4];
    int multiplier;

    colors[0] = 0xe3;
    colors[1] = 0xf3;
    Font_Select_Palette(0, colors);
    Set_Text_Shadow_Color(0xe3);
    Font_Select_Shadow(0, 15, 0, 0);
    Print_Number_Right(236, 172, wizards[current_player].combat_skill_left);
    Print_Text(200, 172, "Skill:");
    Print_Number_Right(236, 181, wizards[current_player].mana_reserve);
    Print_Text(200, 181, "Mana:");
    multiplier = Combat_Range_Multiplier(current_player);
    itoa(multiplier, digits, 10);
    Print_Text(200, 190, "Range:");
    if (multiplier < 10) {
        text[0] = '0';
        text[1] = '.';
        text[2] = digits[0];
        text[3] = 'x';
        text[4] = 0;
    } else {
        text[0] = digits[0];
        text[1] = 'x';
        text[2] = 0;
        if (digits[1] != '0') {
            text[1] = '.';
            text[2] = digits[1];
            text[3] = 'x';
            text[4] = 0;
        }
    }
    Print_Text_Right(236, 190, text);
}

/* () the active unit's melee and ranged strength with their weapon icons, its moves (halves) and movement icon
   (walk, fly, or water: sail / swim) */
void Combat_Draw_Unit_Stats(void)
{
    int icon;
    int stat;
    int move_type;
    char colors[4];

    colors[0] = 0xe3;
    colors[1] = 0xf3;
    Font_Select_Palette(0, colors);
    Set_Text_Shadow_Color(0xe3);
    Font_Select_Shadow(0, 15, 0, 0);
    icon = Unit_Melee_Icon(combat_active_unit);
    if (icon != -1) {
        stat = p_battle_units[combat_active_unit].melee;
        Print_Number_Right(126, 174, stat);
        Draw_Pic_Frame(128, 172, combat_small_attack_icons[icon]);
    }
    icon = Unit_Ranged_Icon(combat_active_unit);
    if (icon != -1) {
        stat = p_battle_units[combat_active_unit].ranged;
        Print_Number_Right(126, 181, stat);
        Draw_Pic_Frame(128, 179, combat_small_attack_icons[icon]);
    }
    if (p_battle_units[combat_active_unit].moves != 0)
        Print_Half_Moves(126, 188, p_battle_units[combat_active_unit].moves, 1);
    else
        Print_Number_Right(126, 188, p_battle_units[combat_active_unit].moves);
    move_type = Unit_Combat_Move_Type(combat_active_unit);
    icon = 0;                                   /* combat_small_move_icons: 0 walk, 1 fly, 2 swim / sail */
    if (move_type == COMBAT_MOVE_FLY)
        icon = 1;
    if (move_type == COMBAT_MOVE_SAIL || move_type == COMBAT_MOVE_SWIM)
        icon = 2;
    Draw_Pic_Frame(128, 186, combat_small_move_icons[icon]);
}

/* () every living battle unit's figures (a Hydra as one) and status icon queued, then missiles, vortices, walls */
void Combat_Draw_Battle_Units(void)
{
    int alive;
    int figures;
    int icon;
    int unit;
    int figure;

    Sprite_List_Clear();
    Combat_Queue_Terrain_Objects_A();
    Combat_Queue_Terrain_Objects_B();
    Combat_Update_See_Invisible();
    for (unit = 0; unit < battle_unit_count; unit++) {
        if (p_battle_units[unit].status == BATTLE_UNIT_ACTIVE) {
            alive = p_battle_units[unit].figures_alive;
            figures = p_battle_units[unit].figures;
            if (p_units[p_battle_units[unit].unit_index].type == (int)p_spell_data[SPELL_HYDRA].effect_param) {
                alive = 1;
                figures = 1;
            }
            p_battle_units[unit].draw_mode = DRAW_MODE_NORMAL;
            Unit_Enchant_Aura_Color(unit);
            Unit_Set_Anim_Flags(unit);
            icon = Unit_Combat_Status_Icon(unit);
            Unit_Update_Visibility(unit);
            for (figure = 0; figure < alive; figure++)
                Combat_Queue_Unit_Figure(p_battle_units[unit].gfx_slot, p_battle_units[unit].combat_x,
                                         p_battle_units[unit].combat_y, p_battle_units[unit].dest_x,
                                         p_battle_units[unit].dest_y, p_battle_units[unit].move_anim_step, figure,
                                         figures, p_battle_units[unit].owner, p_battle_units[unit].aura_color,
                                         p_battle_units[unit].hit_anim_intensity, p_battle_units[unit].moving,   /* -> frame */
                                         p_battle_units[unit].hit_anim_kills, 0);
            if (icon != -1)
                Combat_Queue_Unit_Status_Icon(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y,
                                              p_battle_units[unit].dest_x, p_battle_units[unit].dest_y,
                                              p_battle_units[unit].move_anim_step, icon);
        }
    }
    Combat_Queue_Missiles();
    Combat_Queue_Vortices();
    Combat_Queue_Features_And_Walls();
    Outline_Cycle_Tick();
}

/* (bu, x, y) the health bar: hits left * 20 / total pixels long (len, one less, is the last pixel's offset), green /
   yellow / red */
void Combat_Draw_Health_Bar(int bu, int x, int y)
{
    int total;
    int left;
    int len;
    int lost;
    int color;

    total = p_battle_units[bu].hits_per_figure * p_battle_units[bu].figures;
    lost = (p_battle_units[bu].figures - p_battle_units[bu].figures_alive) * p_battle_units[bu].hits_per_figure;
    if (p_battle_units[bu].front_damage > 0)
        lost += p_battle_units[bu].front_damage;
    left = total - lost;
    if (left > 0) {
        len = left * 20 / total - 1;
        if (len >= 13)                          /* the bar (0 .. 19): long, middle, short colors */
            color = HEALTH_BAR_GREEN;
        else if (len >= 6)
            color = HEALTH_BAR_YELLOW;
        else
            color = HEALTH_BAR_RED;
        Draw_Line(x, y, x + len, y, color);
    }
}
