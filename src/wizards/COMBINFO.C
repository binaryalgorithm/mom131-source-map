/* COMBINFO.C - WIZARDS.EXE overlay code segment 83d2 (stub 250c): combat user-interface helpers - the Info screen (the field
   effects: node dispels and auras, Cloud of Shadow / Heavenly Light (shown "Cloud Of Darkness" / "Holy Light"), Chaos Surge, Eternal Night, and each wizard's
   Crusade / Holy Arms / Charm of Life / Zombie Mastery), the count that enables the Info button, the node under
   the battle, a battle unit's figure group picture, picking the next unit to move, the cast refusal messages, a
   confused unit's random move, the combat graphics / buffers load and the "Who Will Cast" choice.
   This module's data (DS:5852): its literals; BSS (DS:c7aa..c7e8).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 83d2 src/wizards/COMBINFO.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_Combat_Info(void);
void Combat_Info_Draw(void);
void Combat_Info_Build_Rows(void);
void Combat_Info_Build_Field_Rows(void);
int Combat_Info_Count(void);
void Combat_Find_Node(void);
void Combat_Build_Unit_Figures_Pic(int unit);
void Combat_Select_Next_Unit(int player);
int Combat_Find_Next_Unit(int player);
void Combat_Cast_Refusal_Msg(int reason);
void Confused_Unit_Random_Move(int unit);
void Combat_Load_UI(void);
void CombatUI_Alloc_Buffers(void);
void Combat_Who_Will_Cast(int unit);

#include "GAME.H"

int combat_info_box_x;                         /* DS:c7e6  the Info box's x (50) */
int combat_info_box_y;                         /* DS:c7e4  the Info box's y (centred on its height) */
struct CombatEffectRow *combat_effect_rows[14]; /* DS:c7c8  the Info screen's rows: field effects, then 2 columns of wizard spells */
int combat_info_count;                          /* DS:c7c6  */
/* Combat_Main / Combat_Draw_Screen store Combat_Info_Count() here (every Info row, field effects too);
   Combat_Info_Build_Rows counts the wizards' combat-relevant globals into it */
int combat_info_rows;                           /* DS:c7c4  field-effect rows (rounded up to even) */
int combat_node_type;                           /* DS:c7c2  the realm of the node aura under the battle; -1 */
int combat_info_wiz_rows;                       /* DS:c7c0  rows of the larger wizard column */
unsigned combat_info_top_pic;                   /* DS:c7be  COMPIX 0x3a */
unsigned combat_info_middle_pic;                /* DS:c7bc  COMPIX 0x39 */
unsigned combat_info_bottom_pic;                /* DS:c7ba  COMPIX 0x38 */
unsigned combat_info_box_pic;                   /* DS:c7b8  COMPIX 0x3b */
char combat_info_backdrop;                      /* DS:c7b6  the battlefield backdrop (9-11 the node dispels) */
char combat_info_cloud_of_shadow;               /* DS:c7b5  the city's Cloud of Shadow */
char combat_info_heavenly_light;                /* DS:c7b4  the city's Heavenly Light */
unsigned combat_figure_pic;                     /* DS:c7b2  a battle unit's figure group picture */
unsigned combat_hit_bar_pic;                    /* DS:c7b0  COMPIX 18 */
unsigned combat_small_move_icons[3];            /* DS:c7aa  COMPIX 38-40 */

extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */
extern int combat_pingpong_frame;                   /* DS:7094 */
extern int battlefield_figure_anim_frame;                /* DS:7096 */
extern unsigned ems_frame_seg;                  /* DS:760c */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern unsigned combat_scan_icons[];            /* DS:924c */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int no_units_to_move;                    /* DS:9444 */
extern unsigned p_pic_work;                     /* DS:998c */
extern int present_mode;                        /* DS:9c8e */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned p_world_block;                /* DS:9d20 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int combat_all_units_immobile;                     /* DS:c406 */
extern unsigned combat_cancel_button_pic;       /* DS:c422 */
extern unsigned combat_use_lock_pic;            /* DS:c424 */
extern unsigned combat_flee_lock_pic;           /* DS:c426 */
extern unsigned combat_auto_lock_pic;           /* DS:c428 */
extern unsigned combat_done_lock_pic;           /* DS:c42a */
extern unsigned combat_wait_lock_pic;           /* DS:c42c */
extern unsigned combat_spell_lock_pic;          /* DS:c42e */
extern int combat_human_side;                   /* DS:c436 */
extern int combat_wizard_can_cast;              /* DS:c438 */
extern int combat_enemy_wizard;                 /* DS:c43e */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern unsigned combat_enchant_pics[15];        /* DS:c4fe */
extern int combat_end_turn_pending;               /* DS:c51e */
extern int *p_combat_cursor_region;             /* DS:c522 */
extern char *combat_click_map[];                /* DS:c524  per grid row: the battle unit at each square, -1 */
extern unsigned combat_sprite_pics[COMBAT_SPRITE_SLOTS];         /* DS:c550 */
extern unsigned combat_use_button_pic;          /* DS:c576 */
extern unsigned combat_flee_button_pic;         /* DS:c578 */
extern unsigned combat_auto_button_pic;         /* DS:c57a */
extern unsigned combat_done_button_pic;         /* DS:c57c */
extern unsigned combat_wait_button_pic;         /* DS:c57e */
extern unsigned combat_spell_button_pic;        /* DS:c580 */
extern int combat_active_unit;                  /* DS:c582 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern unsigned combat_info_pics[14];           /* DS:c58c  COMPIX 0x2a..0x37 */
extern unsigned combat_small_attack_icons[];    /* DS:c5a8 */
extern int far *p_aura_leadership;              /* DS:c896 */
extern int far *p_aura_resistance;              /* DS:c89a */
extern int far *p_aura_holy_bonus;              /* DS:c89e */
extern int *p_combat_path_y;                    /* DS:d13e */
extern int *p_combat_path_x;                    /* DS:d140 */
extern int *p_combat_path_prev;                  /* DS:d144 */
extern unsigned char *p_combat_path_dist;       /* DS:d146 */
extern unsigned char *p_combat_move_cost;       /* DS:d148 */

void UI_Clear_Controls(void);
void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
void Screen_Copy_Front(void);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void UI_Clear_Help(void);
void UI_Set_Help(struct HelpEntry *help, int count);
void Timer_Mark(void);
int UI_Get_Input(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Reset_Clip_Rect(void);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Text_Shadow_Color(int color);
void Font_Select_Palette(int font, char *colors);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Space_Width(int width);
int Print_Text_Centered(int x, int y, char *text);
int Print_Text(int x, int y, char *text);
void Pic_Create_Blank(int width, int height, unsigned pic);
void Unit_Enchant_Aura_Color(int bu);
void Unit_Set_Anim_Flags(int bu);
void Unit_Update_Visibility(int bu);
void Figure_EMS_Map_Slot(int slot);
void far *Seg_To_Far_Ptr(unsigned seg);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Banner_Colorize(int player);
void Combat_Unit_Enchant_Outline(int aura_realm);
void Unit_Pic_Apply_Effect(int effect);
void Figure_Formation_Offset(int figures, int figure, int *dx, int *dy);
int Pic_Draw_Into_Pic(int x, int y, unsigned dst, unsigned src);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
void Combat_Set_Active_Unit(int bu);
void Warning_Box(char *text);
int Random(int n);
int Combat_Move_Unit(int unit, int x, int y);
int List_Select_Dialog(int count, char **names, int more, char *title);
int Combat_Spell_Action();                      /* no prototype here: called with 4 arguments (1 elsewhere) */
void Battlefield_Draw_Terrain(void);

/* () the combat Info screen (the spell effects in force) until a click anywhere */
void Screen_Combat_Info(void)
{
    int done;
    int input;
    int control;
    register int i;
    register int h;

    combat_info_backdrop = p_battlefield->backdrop;
    combat_info_cloud_of_shadow = p_battlefield->city_enchants[CITY_ENCH_CLOUD_OF_SHADOW];
    combat_info_heavenly_light = p_battlefield->city_enchants[CITY_ENCH_HEAVENLY_LIGHT];
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Combat_Info_Draw, 1);
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Screen_Copy_Front();
    Screen_Save_A800();
    p_text_buf = Near_Alloc_Reset(80);
    for (i = 0; i < 14; i++)
        combat_effect_rows[i] = (struct CombatEffectRow *)Near_Alloc(30);
    Heap_Mark(p_world_block);
    Heap_Mark(p_scratch);
    Heap_Alloc_Back(p_scratch, 990);           /* the back 990 paragraphs taken, never used: COMPIX 58 loads in front */
    combat_info_bottom_pic = LBX_Reload_Next("COMPIX", 56, p_world_block);
    combat_info_middle_pic = LBX_Reload_Next("COMPIX", 57, p_world_block);
    combat_info_box_pic = LBX_Reload_Next("COMPIX", 59, p_world_block);
    for (i = 0; i < 14; i++)
        combat_info_pics[i] = LBX_Reload_Next("COMPIX", i + 42, p_world_block);
    combat_info_top_pic = LBX_Reload_Next("COMPIX", 58, p_scratch);
    Combat_Info_Build_Rows();
    combat_info_box_x = 50;
    combat_info_box_y = 10;
    if (combat_info_rows > 0)
        h = combat_info_rows / 2 * 19 + 9;
    else
        h = 4;
    if (combat_info_wiz_rows > 0)
        h = combat_info_box_y + combat_info_wiz_rows * 20 + h + 21;
    else
        h = combat_info_box_y + combat_info_wiz_rows * 20 + h;
    combat_info_box_y = 100 - h / 2;
    h = combat_info_box_y + h;
    UI_Clear_Help();
    for (i = 0; i < 20; i++) {
        help_entries[i].id = -1;
        help_entries[i].x1 = 0;
        help_entries[i].y1 = 0;
        help_entries[i].x2 = 0;
        help_entries[i].y2 = 0;
    }
    UI_Set_Help(help_entries, 20);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (done == 0) {
            Screen_Restore_A800();
            Combat_Info_Draw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    Heap_Release(p_scratch);
    Heap_Release(p_world_block);
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    present_mode = PRESENT_DISSOLVE;
    UI_Clear_Help();
}

/* () the Info screen's redraw: the box sized to its rows (top, middle, bottom clipped), the two wizards' names over
   their columns, and every row's icon and name with its help */
void Combat_Info_Draw(void)
{
    int clip_y2;
    int x;
    int y;
    int text_y;
    char colors[4];
    int shown;
    register int i;
    register int h;

    Set_Draw_Page();
    if (combat_info_rows > 0)
        h = combat_info_rows / 2 * 19 + 9;
    else
        h = 4;
    h = combat_info_box_y + h;
    Set_Clip_Rect(0, 0, 319, h);
    Draw_Pic_Clipped(combat_info_box_x, combat_info_box_y, combat_info_top_pic);
    Reset_Clip_Rect();
    if (combat_info_wiz_rows > 0)
        clip_y2 = combat_info_wiz_rows * 20 + h + 21;
    else
        clip_y2 = combat_info_wiz_rows * 20 + h;
    Set_Clip_Rect(0, 0, 319, clip_y2);
    Draw_Pic_Clipped(combat_info_box_x, h, combat_info_middle_pic);
    if (combat_info_wiz_rows > 0)
        Reset_Clip_Rect();
    else {
        Set_Clip_Rect(0, clip_y2, 319, 199);
        clip_y2 -= 2;
    }
    Draw_Pic_Clipped(combat_info_box_x, clip_y2, combat_info_bottom_pic);
    Reset_Clip_Rect();
    colors[0] = 250;
    colors[1] = 177;
    Set_Text_Shadow_Color(254);
    Font_Select_Palette(2, colors);
    Font_Select_Shadow(2, 15, 0, 0);
    Set_Space_Width(2);
    if (combat_info_wiz_rows > 0) {
        if (combat_info_rows > 0)
            text_y = combat_info_rows / 2 * 19 + 15;
        else
            text_y = 11;
        if (combat_enemy_wizard < PLAYER_NEUTRAL) {
            strcpy(p_text_buf, wizards[combat_enemy_wizard].wizard_name);
            strcat(p_text_buf, " Spells");
            Print_Text_Centered(combat_info_box_x + 54, combat_info_box_y + text_y, p_text_buf);
        }
        strcpy(p_text_buf, wizards[combat_human_side].wizard_name);
        strcat(p_text_buf, " Spells");
        Print_Text_Centered(combat_info_box_x + 170, combat_info_box_y + text_y, p_text_buf);
    }
    if (combat_info_rows > 0)
        text_y = 10;
    else
        text_y = 26;
    shown = 0;
    for (i = 0; i < combat_info_rows + combat_info_wiz_rows * 2; i++) {
        if (i >= combat_info_rows && combat_info_rows > 0 && shown == 0) {
            shown = 1;
            text_y = 31;
        }
        x = combat_info_box_x + i % 2 * 112 + 11;
        y = combat_info_box_y + text_y + i / 2 * 19;
        Draw_Pic_Frame(x - 1, y - 1, combat_info_box_pic);
        if (combat_effect_rows[i]->pic != -1) {
            Draw_Pic_Frame(x, y, combat_effect_rows[i]->pic);
            Print_Text(x + 20, y + 5, combat_effect_rows[i]->name);
            help_entries[i].id = combat_effect_rows[i]->help;
            help_entries[i].x1 = x;
            help_entries[i].y1 = y;
            help_entries[i].x2 = x + 100;
            help_entries[i].y2 = y + 15;
        }
    }
}

/* () the field rows, then each wizard's column (the enemy wizard's on the left, the human's on the right; two
   rows per line): Crusade, Holy Arms, Charm of Life, Zombie Mastery */
void Combat_Info_Build_Rows(void)
{
    int side;
    int rows_max;
    register int row;
    register int w;

    Combat_Info_Build_Field_Rows();
    combat_info_count = 0;
    row = combat_info_rows;
    rows_max = 0;
    w = combat_enemy_wizard;
    for (side = 0; side < 2; side++) {
        if (w < PLAYER_NEUTRAL) {
            if (wizards[w].globals.crusade > 0) {
                combat_effect_rows[row]->pic = combat_info_pics[0];
                combat_effect_rows[row]->help = 622;   /* help: HELP.LBX 612 .. 625, one per effect row */
                strcpy(combat_effect_rows[row]->name, "Crusade");
                combat_info_count++;
                row += 2;
            }
            if (wizards[w].globals.holy_arms > 0) {
                combat_effect_rows[row]->pic = combat_info_pics[1];
                combat_effect_rows[row]->help = 623;
                strcpy(combat_effect_rows[row]->name, "Holy Arms");
                combat_info_count++;
                row += 2;
            }
            if (wizards[w].globals.charm_of_life > 0) {
                combat_effect_rows[row]->pic = combat_info_pics[3];
                combat_effect_rows[row]->help = 624;
                strcpy(combat_effect_rows[row]->name, "Charm Of Life");
                combat_info_count++;
                row += 2;
            }
            if (wizards[w].globals.zombie_mastery > 0) {
                combat_effect_rows[row]->pic = combat_info_pics[9];
                combat_effect_rows[row]->help = 625;
                strcpy(combat_effect_rows[row]->name, "Zombie Mastery");
                combat_info_count++;
                row += 2;
            }
        }
        w = current_player;                     /* combat_human_side, which titles this column: the shown battles hold the human */
        row = combat_info_rows + 1;
        if (side == 0)
            rows_max = combat_info_count;
    }
    combat_info_wiz_rows = combat_info_count - rows_max > rows_max ? combat_info_count - rows_max : rows_max;
}

/* () the field-effect rows: the node backdrop's dispel, the node aura, the city's Cloud of Shadow / Heavenly Light,
   Chaos Surge and Eternal Night (any wizard); padded to an even count */
void Combat_Info_Build_Field_Rows(void)
{
    register int n;
    register int i;

    for (i = 0; i < 14; i++)
        combat_effect_rows[i]->pic = -1;
    n = 0;
    switch (combat_info_backdrop) {
    case BACKDROP_SORCERY_NODE:
        combat_effect_rows[n]->pic = combat_info_pics[12];
        combat_effect_rows[n]->help = 612;
        strcpy(combat_effect_rows[n]->name, "Dispells Non-Sorcery");
        n++;
        break;
    case BACKDROP_CHAOS_NODE:
        combat_effect_rows[n]->pic = combat_info_pics[5];
        combat_effect_rows[n]->help = 613;
        strcpy(combat_effect_rows[n]->name, "Dispells Non-Chaos");
        n++;
        break;
    case BACKDROP_NATURE_NODE:
        combat_effect_rows[n]->pic = combat_info_pics[10];
        combat_effect_rows[n]->help = 614;
        strcpy(combat_effect_rows[n]->name, "Dispells Non-Nature");
        n++;
        break;
    }
    switch (combat_node_type) {
    case NODE_SORCERY:
        combat_effect_rows[n]->pic = combat_info_pics[13];
        combat_effect_rows[n]->help = 615;
        strcpy(combat_effect_rows[n]->name, "Sorcery Node Aura");
        n++;
        break;
    case NODE_NATURE:
        combat_effect_rows[n]->pic = combat_info_pics[11];
        combat_effect_rows[n]->help = 617;
        strcpy(combat_effect_rows[n]->name, "Nature Node Aura");
        n++;
        break;
    case NODE_CHAOS:
        combat_effect_rows[n]->pic = combat_info_pics[6];
        combat_effect_rows[n]->help = 616;
        strcpy(combat_effect_rows[n]->name, "Chaos Node Aura");
        n++;
        break;
    }
    if (combat_site == COMBAT_AT_CITY) {
        if (combat_info_cloud_of_shadow > 0) {
            combat_effect_rows[n]->pic = combat_info_pics[8];
            combat_effect_rows[n]->help = 618;
            strcpy(combat_effect_rows[n]->name, "Cloud Of Darkness");
            n++;
        }
        if (combat_info_heavenly_light > 0) {
            combat_effect_rows[n]->pic = combat_info_pics[2];
            combat_effect_rows[n]->help = 619;
            strcpy(combat_effect_rows[n]->name, "Holy Light");
            n++;
        }
    }
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (wizards[i].globals.chaos_surge > 0) {
            combat_effect_rows[n]->pic = combat_info_pics[4];
            combat_effect_rows[n]->help = 620;
            strcpy(combat_effect_rows[n]->name, "Chaos Surge");
            n++;
            break;
        }
    }
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (wizards[i].globals.eternal_night > 0) {
            combat_effect_rows[n]->pic = combat_info_pics[7];
            combat_effect_rows[n]->help = 621;
            strcpy(combat_effect_rows[n]->name, "Eternal Night");
            n++;
            break;
        }
    }
    if (n % 2 != 0) {
        combat_effect_rows[n]->pic = -1;
        n++;
    }
    combat_info_rows = n;
}

/* () -> the number of Info rows (non-zero enables the Info button) */
int Combat_Info_Count(void)
{
    register int count;
    register int i;
    register int w;

    count = 0;
    w = combat_enemy_wizard;
    for (i = 0; i < 2; i++) {
        if (w < PLAYER_NEUTRAL) {
            if (wizards[w].globals.crusade > 0)
                count++;
            if (wizards[w].globals.holy_arms > 0)
                count++;
            if (wizards[w].globals.charm_of_life > 0)
                count++;
            if (wizards[w].globals.zombie_mastery > 0)
                count++;
        }
        w = current_player;
    }
    switch (p_battlefield->backdrop) {
    case BACKDROP_SORCERY_NODE:
        count++;
        break;
    case BACKDROP_CHAOS_NODE:
        count++;
        break;
    case BACKDROP_NATURE_NODE:
        count++;
        break;
    }
    switch (combat_node_type) {
    case NODE_SORCERY:
        count++;
        break;
    case NODE_NATURE:
        count++;
        break;
    case NODE_CHAOS:
        count++;
        break;
    }
    if (combat_site == COMBAT_AT_CITY) {
        if (p_battlefield->city_enchants[CITY_ENCH_CLOUD_OF_SHADOW] > 0)
            count++;
        if (p_battlefield->city_enchants[CITY_ENCH_HEAVENLY_LIGHT] > 0)
            count++;
    }
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (wizards[i].globals.chaos_surge > 0) {
            count++;
            break;
        }
    }
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (wizards[i].globals.eternal_night > 0) {
            count++;
            break;
        }
    }
    return count;
}

/* () combat_node_type = the realm of the node whose aura covers the battle square, else -1 */
void Combat_Find_Node(void)
{
    int i;
    int node_realm;
    int node;

    node_realm = -1;
    for (node = 0; node < NUM_NODES && node_realm == -1; node++) {
        if (p_nodes[node].plane == combat_loc_plane) {
            for (i = 0; i < p_nodes[node].power && node_realm == -1; i++)
                if (p_nodes[node].aura_x[i] == combat_loc_x && p_nodes[node].aura_y[i] == combat_loc_y)
                    node_realm = p_nodes[node].realm;
        }
    }
    combat_node_type = node_realm;
}

/* (unit) builds combat_figure_pic: the battle unit's figures (one for Hydra) in formation, facing toward its
   destination (BUG #445: the facing is then forced to 7), animated when moving or always-animated, with its banner,
   enchantment outline and draw effect */
void Combat_Build_Unit_Figures_Pic(int unit)
{
    int figures;
    int figures_n;
    int x;
    int fy;
    int owner;
    int aura;
    int draw_mode;
    int slot;
    int frame;
    int cur_x;
    int cur_y;
    int dst_x;
    int dst_y;
    unsigned far *pics;
    unsigned seg;
    int ems_ofs;
    int figure;
    int facing;

    combat_figure_pic = Heap_Alloc_Back(p_world_block, 124);
    if (p_battle_units[unit].status == BATTLE_UNIT_ACTIVE) {
        Pic_Create_Blank(47, 42, combat_figure_pic);
        figures = p_battle_units[unit].figures_alive;
        figures_n = p_battle_units[unit].figures;
        if (p_units[p_battle_units[unit].unit_index].type == (int)p_spell_data[SPELL_HYDRA].effect_param) {
            figures = 1;
            figures_n = 1;
        }
        Unit_Enchant_Aura_Color(unit);
        Unit_Set_Anim_Flags(unit);
        Unit_Update_Visibility(unit);
        slot = p_battle_units[unit].gfx_slot;
        owner = p_battle_units[unit].owner;
        aura = p_battle_units[unit].aura_color;
        draw_mode = p_battle_units[unit].draw_mode;
        if (p_battle_units[unit].always_animate == 1)
            frame = battlefield_figure_anim_frame;
        else if (p_battle_units[unit].moving == 1) {
            if (p_battle_units[unit].anim_ability == 1)
                frame = battlefield_figure_anim_frame;
            else
                frame = combat_pingpong_frame;
        } else
            frame = 1;
        cur_x = p_battle_units[unit].combat_x;
        cur_y = p_battle_units[unit].combat_y;
        dst_x = p_battle_units[unit].dest_x;
        dst_y = p_battle_units[unit].dest_y;
        if (cur_x > dst_x) {
            if (cur_y > dst_y)
                facing = 0;
            else if (cur_y < dst_y)
                facing = 6;
            else
                facing = 7;
        } else if (cur_x < dst_x) {
            if (cur_y > dst_y)
                facing = 2;
            else if (cur_y < dst_y)
                facing = 4;
            else
                facing = 3;
        } else if (cur_y > dst_y)
            facing = 1;
        else if (cur_y < dst_y)
            facing = 5;
        else
            facing = 2;
        Figure_EMS_Map_Slot(slot);
        if ((slot & 1) == 0)
            ems_ofs = 0;
        else
            ems_ofs = 512;
        ems_ofs += slot * 56;
        seg = Heap_Alloc_Front(ems_frame_seg + ems_ofs, 2);
        pics = (unsigned far *)Seg_To_Far_Ptr(seg);
        facing = 7;
        Set_Pic_Frame(pics[facing], frame);
        Pic_Decode_Frame_To_Bitmap(pics[facing], p_pic_work);
        Banner_Colorize(owner);
        Combat_Unit_Enchant_Outline(aura);
        Unit_Pic_Apply_Effect(draw_mode);
        for (figure = 0; figure < figures; figure++) {
            Figure_Formation_Offset(figures_n, figure, &x, &fy);
            Pic_Draw_Into_Pic(x, fy - 4, combat_figure_pic, p_pic_work);
        }
    }
}

/* (player) activates the player's next unit (Combat_Find_Next_Unit); none left ends the turn's moves
   (combat_end_turn_pending); an active unit that is not the human side's is replaced by the attacker's first unit
   Numbered: BUG #309 (BUGS_AND_QUIRKS.txt) */
void Combat_Select_Next_Unit(int player)
{
    int none_left;
    register int i;
    register int done;

    none_left = 0;
    done = 0;
    while (done == 0) {                         /* runs once (done = 1 at its end) */
        none_left = Combat_Find_Next_Unit(player);
        if (none_left == 1)
            combat_end_turn_pending = 1;
        else
            combat_all_units_immobile = 0;
        done = 1;
    }
    if (no_units_to_move == 0) {
        if (p_battle_units[combat_active_unit].moves > 0
            && p_battle_units[combat_active_unit].owner == combat_human_side)
            combat_all_units_immobile = 0;
    } else {                                    /* empty: kept for the byte match */
    }
    if (p_battle_units[combat_active_unit].owner != combat_human_side) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == combat_attacker_player) {
                combat_active_unit = i;
                break;
            }
        }
    }
}

/* (player) -> 1 when none of the player's units can move: activates the nearest unit with moves left, units
   that are not waiting first; when only waiting units remain, the nearest of them, and Wait is cleared */
int Combat_Find_Next_Unit(int player)
{
    int done;
    int none_left;
    int dist;
    int bu;
    int wait_dist;
    int best_waiting;
    int best_dist;
    int best;
    int n_own_active;
    int unused;                                 /* set to -1, never read */
    int k;                                      /* the own units seen */
    int x1;
    int y1;
    register int i;
    register int u;                             /* a register copy of bu */

    x1 = p_battle_units[combat_active_unit].combat_x;
    y1 = p_battle_units[combat_active_unit].combat_y;
    n_own_active = 0;
    for (i = 0; i < battle_unit_count; i++)
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == player)
            n_own_active++;
    wait_dist = 1000;
    best_waiting = -1;
    best_dist = 1000;
    best = -1;
    unused = -1;
    none_left = 0;
    done = 0;
    bu = 0;
    k = 0;
    while (done == 0) {
        u = bu;
        if (p_battle_units[u].status == BATTLE_UNIT_ACTIVE && p_battle_units[u].owner == player) {
            if (p_battle_units[u].moves > 0) {
                dist = Map_Distance(x1, y1, p_battle_units[u].combat_x, p_battle_units[u].combat_y, MAP_WIDTH);
                switch (p_battle_units[u].action_state) {
                case ACTION_WAIT:
                    if (wait_dist > dist) {
                        wait_dist = dist;
                        best_waiting = u;
                    }
                    break;
                default:
                    if (best_dist > dist) {
                        best_dist = dist;
                        best = u;
                    }
                    break;
                }
            }
            k++;
        }
        bu++;
        if (k == n_own_active) {
            if (best == -1) {
                if (best_waiting == -1) {
                    done = 1;
                    none_left = 1;
                } else {
                    Combat_Set_Active_Unit(best_waiting);
                    done = 1;
                    for (i = 0; i < battle_unit_count; i++)
                        if (p_battle_units[i].action_state == ACTION_WAIT)
                            p_battle_units[i].action_state = ACTION_NONE;
                }
            } else {
                done = 1;
                Combat_Set_Active_Unit(best);
            }
        }
    }
    return none_left;
}

/* (reason) 1: "You may only cast once per turn."; 2: "You are unable to throw spells at this time." */
void Combat_Cast_Refusal_Msg(int reason)
{
    UI_Clear_Controls();
    switch (reason) {
    case CAST_REFUSED_UNABLE:
        Warning_Box("You are unable to throw spells at this time.");
        break;
    case CAST_REFUSED_ONCE_PER_TURN:
        Warning_Box("You may only cast once per turn.");
        break;
    }
}

/* (unit) a confused unit moves to a random reachable square (up to 600 tries) and loses its turn */
void Confused_Unit_Random_Move(int unit)
{
    int i;
    int x;
    int y;
    int ry;
    int rx;
    int found;

    found = 0;
    for (i = 0; i < 600 && found == 0; i++) {
        rx = Random(21) - 1;
        ry = Random(22) - 1;
        if (combat_click_map[ry][rx] == CLICK_REACHABLE) {
            x = rx;
            y = ry;
            found = 1;
        }
    }
    if (found == 1)
        Combat_Move_Unit(unit, x, y);
    p_battle_units[unit].action_state = ACTION_DONE;
    p_battle_units[unit].moves = 0;
}

/* () the combat tables (battle units, the three per-player aura arrays, the combat enchantments) and the COMPIX
   buttons, locks, enchantment icons, attack / move / scan icons, the hit bar and 18 sprite buffers; then the near
   buffers */
void Combat_Load_UI(void)
{
    register int i;

    p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, BATTLE_UNIT_TABLE_PARAS));   /* MAX_BATTLE_UNITS x 0x6e bytes */
    p_aura_holy_bonus = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 6));
    p_aura_resistance = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 6));
    p_aura_leadership = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 6));
    combat_spell_button_pic = LBX_Reload_Next("COMPIX", 1, p_scratch);
    combat_wait_button_pic = LBX_Reload_Next("COMPIX", 2, p_scratch);
    combat_done_button_pic = LBX_Reload_Next("COMPIX", 3, p_scratch);
    combat_auto_button_pic = LBX_Reload_Next("COMPIX", 4, p_scratch);
    combat_use_button_pic = LBX_Reload_Next("COMPIX", 20, p_scratch);
    combat_flee_button_pic = LBX_Reload_Next("COMPIX", 21, p_scratch);
    p_combat_enchants = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 3));
    for (i = 0; i < 30; i++)
        p_combat_enchants[i] = 0;
    combat_enchant_pics[0] = LBX_Reload_Next("COMPIX", 5, p_scratch);
    combat_enchant_pics[1] = LBX_Reload_Next("COMPIX", 6, p_scratch);
    combat_enchant_pics[2] = LBX_Reload_Next("COMPIX", 7, p_scratch);
    combat_enchant_pics[3] = LBX_Reload_Next("COMPIX", 8, p_scratch);
    combat_enchant_pics[4] = LBX_Reload_Next("COMPIX", 9, p_scratch);
    combat_enchant_pics[5] = LBX_Reload_Next("COMPIX", 10, p_scratch);
    combat_enchant_pics[6] = LBX_Reload_Next("COMPIX", 11, p_scratch);
    combat_enchant_pics[7] = LBX_Reload_Next("COMPIX", 12, p_scratch);
    combat_enchant_pics[8] = LBX_Reload_Next("COMPIX", 13, p_scratch);
    combat_enchant_pics[9] = LBX_Reload_Next("COMPIX", 14, p_scratch);
    combat_enchant_pics[10] = LBX_Reload_Next("COMPIX", 15, p_scratch);
    combat_enchant_pics[11] = LBX_Reload_Next("COMPIX", 41, p_scratch);
    combat_enchant_pics[12] = LBX_Reload_Next("COMPIX", 60, p_scratch);
    combat_enchant_pics[13] = LBX_Reload_Next("COMPIX", 79, p_scratch);
    combat_enchant_pics[14] = LBX_Reload_Next("COMPIX", 80, p_scratch);
    combat_cancel_button_pic = LBX_Reload_Next("COMPIX", 22, p_scratch);
    combat_spell_lock_pic = LBX_Reload_Next("COMPIX", 23, p_scratch);
    combat_wait_lock_pic = LBX_Reload_Next("COMPIX", 24, p_scratch);
    combat_done_lock_pic = LBX_Reload_Next("COMPIX", 28, p_scratch);
    combat_auto_lock_pic = LBX_Reload_Next("COMPIX", 26, p_scratch);
    combat_use_lock_pic = LBX_Reload_Next("COMPIX", 25, p_scratch);
    combat_flee_lock_pic = LBX_Reload_Next("COMPIX", 27, p_scratch);
    for (i = 0; i < 9; i++)
        combat_small_attack_icons[i] = LBX_Reload_Next("COMPIX", i + 29, p_scratch);
    for (i = 0; i < 3; i++)
        combat_small_move_icons[i] = LBX_Reload_Next("COMPIX", i + 38, p_scratch);
    for (i = 0; i < 18; i++)
        combat_scan_icons[i] = LBX_Reload_Next("COMPIX", i + 61, p_scratch);
    combat_hit_bar_pic = LBX_Reload_Next("COMPIX", 18, p_scratch);
    Heap_Mark(p_scratch);
    for (i = 0; i < COMBAT_SPRITE_SLOTS; i++)
        combat_sprite_pics[i] = Heap_Alloc_Back(p_scratch, 55);
    Heap_Release(p_scratch);
    CombatUI_Alloc_Buffers();
}

/* () the combat near buffers: move cost / path distance, the path predecessors, path x / y, the cursor region, the 22
   click-map rows and the text buffer */
void CombatUI_Alloc_Buffers(void)
{
    register int i;

    p_combat_move_cost = (unsigned char *)Near_Alloc_Reset(504);
    p_combat_path_dist = (unsigned char *)Near_Alloc(504);
    p_combat_path_prev = (int *)Near_Alloc(1008);
    p_combat_path_x = (int *)Near_Alloc(60);
    p_combat_path_y = (int *)Near_Alloc(60);
    p_combat_cursor_region = (int *)Near_Alloc(12);
    for (i = 0; i < BATTLEFIELD_H; i++)
        combat_click_map[i] = Near_Alloc(BATTLEFIELD_W);
    p_text_buf = Near_Alloc(20);
}

/* (unit) a hero or unit that can cast: "Who Will Cast" - the wizard (once per turn), the unit or Cancel; a failed
   cast shows the refusal; then the battlefield is redrawn */
void Combat_Who_Will_Cast(int unit)
{
    int choice;
    char *choices[4];
    register int result;

    if (combat_wizard_can_cast == 1) {
        choices[0] = wizards[PLAYER_HUMAN].wizard_name;
        choices[1] = unit_types[p_units[p_battle_units[unit].unit_index].type].name;
        choices[2] = "Cancel";
        choices[3] = "";
        UI_Clear_Controls();
        choice = List_Select_Dialog(3, choices, 0, "Who Will Cast");
        switch (choice) {
        case 0:
            result = Combat_Spell_Action(current_player + CASTER_WIZARD, combat_loc_x, combat_loc_y, combat_loc_plane);
            switch (result) {
            case CAST_RESULT_CAST:
                combat_wizard_can_cast = 0;
                break;
            }
            break;
        case 1:
            result = Combat_Spell_Action(unit, combat_loc_x, combat_loc_y, combat_loc_plane);
            break;
        default:
            result = CAST_RESULT_NONE;
        }
    } else
        result = Combat_Spell_Action(unit, combat_loc_x, combat_loc_y, combat_loc_plane);
    if (result == CAST_RESULT_UNABLE)
        Combat_Cast_Refusal_Msg(CAST_REFUSED_UNABLE);
    Battlefield_Draw_Terrain();
}
