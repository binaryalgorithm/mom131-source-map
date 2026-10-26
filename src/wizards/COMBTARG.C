/* COMBTARG.C - WIZARDS.EXE overlay code segment 8f51 (stub 2540): combat spell targeting (the cursor, the prompt
   redraw, the target pick with its refusals), the missile and melee animations, a spell's damage roll and applying
   damage to a battle unit, the invisibility / Wall of Darkness ranged test, deploying a side at battle start, the
   Demon Lord's summoning, the city-wall melee test, Mana Leak, the cast banners, a free figure graphics slot and
   the side count that decides the winner.
   This module's data (DS:5b18..5bcb): its literals ("" is the tail of "cmbtfx"); BSS DS:c8a6..c8ad (defined here).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 8f51 src/wizards/COMBTARG.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Combat_Spell_Target_Cursor(void);
void Combat_Spell_Target_Redraw(void);
int Combat_Spell_Pick_Target_UI(int spell, int *x, int *y);
void Combat_Missile_Anim(int attacker, int target, int hit, int x, int y);
void Combat_Melee_Anim(int attacker, int target, int attacker_hit, int target_hit, int x, int y);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
void Battle_Apply_Damage(int unit, int *damage);
int Ranged_Target_Blocked(int attacker, int target);
void Combat_Deploy_Side(int player);
void Demon_Lord_Summon(int unit);
int Melee_Wall_Allowed(int attacker, int target);
void Mana_Leak_Turn(void);
void Combat_Cast_Banner(int caster, int spell);
int Battle_Free_Gfx_Slot(void);
int Combat_Count_Sides(void);
void Combat_Draw_Banner_Text(void);

#include "GAME.H"

/* DS:c8a6..c8ad: used by COMBDISP.C and COMBSPL.C; defined here because no module between here and COMBTACT.C's BSS
   owns them (owner not fixed by the EXE); highest address first */
int combat_spell_caster;                        /* DS:c8ac */
int *p_dispel_ench_spells;                       /* DS:c8aa  enchantment bit -> spell */
int *p_dispel_curse_spells;                    /* DS:c8a8  combat_flags bit -> spell */
int combat_spell_target_mode;                   /* DS:c8a6  what the targeting cursor accepts (1 an empty square, 2 a unit or
                                                   an empty square, 3 wall,
                                                   4 / 5 a square of the own half, 10 enemy, 11 enemy normal, 20 own,
                                                   21 own normal, 23 own hero, 30 vortex or unit) */

extern int combat_cursor_anim;                  /* DS:7080 */
extern int missile_anim_frame_copy;             /* DS:7084 */
extern int missile_anim_frame;                  /* DS:7086 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_target_y;                     /* DS:927e */
extern int combat_target_x;                     /* DS:9280 */
extern int combat_cursor_mode;                  /* DS:9282 */
extern unsigned p_world_block;                /* DS:9d20 */
extern unsigned combat_silence_sfx;             /* DS:c41a */
extern unsigned combat_cancel_button_pic;       /* DS:c422 */
extern int *p_combat_cursor_region;             /* DS:c522 */
extern char *combat_click_map[];                /* DS:c524  per grid row: a battle unit, CLICK_REACHABLE, CLICK_BLOCKED, CLICK_WALL */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int combat_hover_unit;                   /* DS:c58a */
extern int ui_anim_frame;                       /* DS:c8fc */
extern int combat_winner;                       /* DS:c972 */
extern struct CombatVortex far *p_vortices;     /* DS:d14e */
extern int vortex_count;                        /* DS:d152 */
extern struct CombatMissile far *p_missile_list; /* DS:d154 */
extern int combat_missile_count;                /* DS:d158 */

void far_strcpy(char *dst, int dst_seg, char far *src);
int Mouse_X(void);
int Mouse_Y(void);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
int Combat_Screen_To_Grid_X(int sx, int sy);
int Combat_Screen_To_Grid_Y(int sx, int sy);
int Is_City_Wall_Square(int x, int y);
void Combat_Draw_Screen(void);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Text_Shadow_Color(int color);
void Set_Font_Color(int color);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Space_Width(int width);
void Font_Select_Palette(int font, char *colors);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Print_Text_Centered(int x, int y, char *text);
int Paragraph_Used_Width(int width, char *text, unsigned align);
int Paragraph_Height();                         /* unprototyped: called here with a third (alignment) argument;
                                                   COMBUTIL.C declares the two-argument form */
int Text_Width(char *text);
void Fill_Box_Styled();                           /* unprototyped: called with 6 arguments */
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Clear_Controls(void);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Get_Input(void);
void Timer_Mark(void);
void Wait_Ticks(int ticks);
void Set_Draw_Page(void);
void Screen_Present(void);
void Warning_Box(char *text);
int Sound_Play(unsigned sound);
int Random(int n);
int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
int Combat_Distance(int bu1, int bu2);
void Combat_Missile_List_Build(int figures, int target_figures, int from_x, int from_y, int to_x, int to_y,
                               int kind);
void Battle_Unit_Set_Hit_Anim(int unit, int damage);
void Battle_Unit_Clear_Hit_Anim(int bu);
int Unit_Load_Attack_Sound(int bu);
int Defender_Shields(int defender, int ranged_type, int damage_flags, int attack_flags, int realm);
int Roll_To_Hit(int points, int to_hit);
int Roll_Blocks(int shields, int to_block);
int Is_Combat_Only_Summon(int bu);
void Combat_Update_See_Invisible(void);
int In_City_Area(int bu);
int Water_Battle_Sideline(int player);
int Has_Shooting_Attack(int bu);
int Array_Delete_Near(int index, char *array, int record_size, int count);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Combat_Summon_Add_Unit(int owner, int unit, int x, int y);
void Combat_Summon_Anim(int unit, int x, int y, int spell, int caster);

/* () the combat spell-targeting cursor: over the battlefield a red or green target (green: a legal target for
   combat_spell_target_mode - an enemy for 10 / 2 / 30, an enemy non-fantastic unit for 11, an own unit for 20 / 2 /
   30, an own normal unit for 21, an own hero for 23, a free square on the own half for 4 / 5, a wall for 3, a
   vortex for 30), animated; an arrow elsewhere */
void Combat_Spell_Target_Cursor(void)
{
    int unit;
    int mx;
    int my;
    int gy;
    int off;
    register int gx;
    register int i;

    p_combat_cursor_region[MREG_IMAGE] = POINTER_HAND;
    combat_hover_unit = -1;
    off = 4;
    mx = Mouse_X() + off;
    my = Mouse_Y() + off;
    combat_cursor_mode = 0;
    if (off + 168 > my) {
        gx = Combat_Screen_To_Grid_X(mx, my);
        gy = Combat_Screen_To_Grid_Y(mx, my);
        switch (combat_click_map[gy][gx]) {
        case CLICK_BLOCKED:
            if (combat_spell_target_mode < 10)
                p_combat_cursor_region[MREG_IMAGE] = POINTER_SPELL;
            else
                p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
            break;
        case CLICK_REACHABLE:
            if (combat_spell_target_mode < 10)
                p_combat_cursor_region[MREG_IMAGE] = POINTER_SPELL;
            else
                p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
            break;
        default:
            combat_cursor_mode = 1;
            combat_target_x = gx;
            combat_target_y = gy;
            combat_cursor_anim = (combat_cursor_anim + 1) % 3;
            unit = combat_click_map[gy][gx];
            combat_hover_unit = unit;
            if (p_battle_units[unit].owner != PLAYER_HUMAN) {
                if (combat_spell_target_mode == CTARGET_ENEMY || combat_spell_target_mode == CTARGET_ANY_SQUARE || combat_spell_target_mode == CTARGET_VORTEX_OR_UNIT ||
                    (combat_spell_target_mode == CTARGET_ENEMY_NORMAL && (char)p_battle_units[unit].race < RACE_ARCANE_CREATURE))
                    p_combat_cursor_region[MREG_IMAGE] = POINTER_SPELL;
                else
                    p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
            } else {
                if (combat_spell_target_mode == CTARGET_OWN || combat_spell_target_mode == CTARGET_ANY_SQUARE || combat_spell_target_mode == CTARGET_VORTEX_OR_UNIT ||
                    (combat_spell_target_mode == CTARGET_OWN_NORMAL && (char)p_battle_units[unit].race < RACE_ARCANE_CREATURE) ||
                    (combat_spell_target_mode == CTARGET_OWN_HERO && p_units[p_battle_units[unit].unit_index].hero_slot > -1))
                    p_combat_cursor_region[MREG_IMAGE] = POINTER_SPELL;
                else
                    p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
            }
            break;
        }
    }
    if (combat_spell_target_mode == CTARGET_ATTACKER_HALF && gx < 11)
        p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
    if (combat_spell_target_mode == CTARGET_DEFENDER_HALF && gx > 10)
        p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
    if (combat_spell_target_mode == CTARGET_WALL) {
        p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
        if (Is_City_Wall_Square(gx, gy))
            p_combat_cursor_region[MREG_IMAGE] = POINTER_SPELL;
    }
    if (combat_spell_target_mode == CTARGET_VORTEX_OR_UNIT) {
        for (i = 0; i < vortex_count; i++) {
            if (p_vortices[i].x == gx && p_vortices[i].y == gy)
                p_combat_cursor_region[MREG_IMAGE] = POINTER_SPELL;
        }
    }
    if (p_combat_cursor_region[MREG_IMAGE] == 2)
        p_combat_cursor_region[MREG_IMAGE] = ui_anim_frame + POINTER_SPELL_ANIM;
    ui_anim_frame = (ui_anim_frame + 1) % 5;
    p_combat_cursor_region[MREG_POINTER_OFS] = 2;
    p_combat_cursor_region[MREG_X1] = 0;
    p_combat_cursor_region[MREG_Y1] = 0;
    p_combat_cursor_region[MREG_X2] = 319;
    p_combat_cursor_region[MREG_Y2] = 199;
    Mouse_Set_Cursor_Regions(1, (struct MouseRegion *)p_combat_cursor_region);
}

/* () the targeting screen's redraw: the battlefield, the prompt box (CMBTFX 28) with message_text, the cursor */
void Combat_Spell_Target_Redraw(void)
{
    unsigned pic;

    Combat_Draw_Screen();
    Heap_Mark(p_scratch);
    pic = LBX_Reload_Next("cmbtfx", 28, p_scratch);
    Heap_Release(p_scratch);
    Draw_Pic_Frame(238, 164, pic);
    Set_Text_Shadow_Color(240);
    Set_Font_Color(227);
    Font_Select_Shadow(0, 0, 0, 0);
    Set_Space_Width(1);
    Paragraph_Print(241, 168, 75, message_text, PARA_LEFT);
    Combat_Spell_Target_Cursor();
}

/* (spell, &x, &y) -> the battle unit picked as the spell's target, 99 for a square (summons, area spells), 999
   when cancelled. BUGS #455: a wall pick (CTARGET_WALL) ends the loop without setting the unit (uninitialised or the
   previous click's), a vortex pick returns the click-map value of its square (negative without a unit). A summon's square is tested
   only for the side's half and an occupant - never for being passable (water, walls, the corners outside the
   diamond); clarified using ReMoM repo.
   The target mode comes from the spell's effect type (with per-spell exceptions), then "Select
   a target for a <spell> spell." until a legal click; an illegal one explains itself (already cast, immune,
   occupied square)
   Numbered: BUG #211, #212 (BUGS_AND_QUIRKS.txt) */
int Combat_Spell_Pick_Target_UI(int spell, int *x, int *y)
{
    int grid_ctl;
    int cancel_ctl;
    int mx;
    int my;
    int input;
    int unit;                                   /* also the result: a battle unit, 99 a square, 999 cancelled (walls
                                                   and vortexes: see the BUGS #455 above) */
    int k;
    long ench;
    int j;
    char name[20];
    int done;

    UI_Set_Redraw_Fn(Combat_Spell_Target_Redraw, 2);
    ui_anim_frame = 0;
    if (spell == SPELL_NONE)
        combat_spell_target_mode = CTARGET_EMPTY_SQUARE;
    else {
        switch (p_spell_data[spell].effect_type) {
        case SPELL_FX_DAMAGE:
        case SPELL_FX_DESTROY:
        case SPELL_FX_UNIT_CURSE:
        case SPELL_FX_CURSE_NO_RESIST:
        case SPELL_FX_DAMAGE_VAR:
        case SPELL_FX_DESTROY_VAR:
            combat_spell_target_mode = CTARGET_ENEMY;
            break;
        case SPELL_FX_NORMAL_UNIT_CURSE:
            combat_spell_target_mode = CTARGET_ENEMY_NORMAL;
            break;
        case SPELL_FX_DISPEL:
            combat_spell_target_mode = CTARGET_VORTEX_OR_UNIT;
            break;
        case SPELL_FX_UNIT_ENCH:
            combat_spell_target_mode = CTARGET_OWN;
            break;
        case SPELL_FX_NORMAL_UNIT_ENCH:
            combat_spell_target_mode = CTARGET_OWN_NORMAL;
            break;
        case SPELL_FX_SUMMON:
            if (combat_attacker_player == PLAYER_HUMAN)
                combat_spell_target_mode = CTARGET_ATTACKER_HALF;
            else
                combat_spell_target_mode = CTARGET_DEFENDER_HALF;
        case SPELL_FX_SPECIAL:
            if (spell == SPELL_HEALING)
                combat_spell_target_mode = CTARGET_OWN;
            if (spell == SPELL_RAISE_DEAD || spell == SPELL_ANIMATE_DEAD) {
                if (combat_attacker_player == PLAYER_HUMAN)
                    combat_spell_target_mode = CTARGET_ATTACKER_HALF;
                else
                    combat_spell_target_mode = CTARGET_DEFENDER_HALF;
            }
            if (spell == SPELL_WARP_CREATURE || spell == SPELL_CREATURE_BINDING || spell == SPELL_WARP_WOOD)
                combat_spell_target_mode = CTARGET_ENEMY;
            if (spell == SPELL_EARTH_TO_MUD || spell == SPELL_CRACKS_CALL)
                combat_spell_target_mode = CTARGET_ANY_SQUARE;
            if (spell == SPELL_MAGIC_VORTEX)
                combat_spell_target_mode = CTARGET_EMPTY_SQUARE;
            if (spell == SPELL_DISRUPT)
                combat_spell_target_mode = CTARGET_WALL;
            if (spell == SPELL_RECALL_HERO)
                combat_spell_target_mode = CTARGET_OWN_HERO;
            if (spell == SPELL_WORD_OF_RECALL)
                combat_spell_target_mode = CTARGET_OWN;
            break;
        }
    }
    UI_Clear_Controls();
    grid_ctl = UI_Add_Grid_Control(0, 0, 1, 1, 319, 168, &mx, &my, -1);
    cancel_ctl = UI_Add_Button(263, 186, "", combat_cancel_button_pic, "\x1b", -1);
    done = 0;
    while (done == 0) {
        strcpy(message_text, "Select a target for a ");
        far_strcpy(name, 0, p_spell_data[spell].name);
        strcat(message_text, name);
        strcat(message_text, " spell.");
        Timer_Mark();
        input = UI_Get_Input();
        if (input == cancel_ctl) {
            done = 1;
            unit = TARGET_CANCELLED;
        }
        if (input == grid_ctl) {
            *x = Combat_Screen_To_Grid_X(mx + 4, my + 4);
            *y = Combat_Screen_To_Grid_Y(mx + 4, my + 4);
            if (combat_spell_target_mode == CTARGET_WALL && Is_City_Wall_Square(*x, *y)) {
                done = 1;
            } else {
                if (combat_spell_target_mode == CTARGET_VORTEX_OR_UNIT) {
                    for (k = 0; k < vortex_count; k++) {
                        if (p_vortices[k].x == *x && p_vortices[k].y == *y)
                            done = 1;
                    }
                }
                unit = combat_click_map[*y][*x];
                if (unit >= 0) {
                    ench = p_units[p_battle_units[unit].unit_index].enchantments | p_battle_units[unit].item_enchantments |
                           p_battle_units[unit].combat_enchantments;
                    if (p_battle_units[unit].owner != PLAYER_HUMAN) {
                        if (combat_spell_target_mode == CTARGET_ENEMY || combat_spell_target_mode == CTARGET_ANY_SQUARE ||
                            combat_spell_target_mode == CTARGET_VORTEX_OR_UNIT)
                            done = 1;
                        if (combat_spell_target_mode == CTARGET_ENEMY_NORMAL && (char)p_battle_units[unit].race < RACE_ARCANE_CREATURE)
                            done = 1;
                        if (spell == SPELL_CREATURE_BINDING && (char)p_battle_units[unit].race < RACE_ARCANE_CREATURE) {
                            done = 0;
                            LBX_Load_Records_Near("message", 0, message_text, 69, 1, 150);
                            far_strcpy(name, 0, p_spell_data[spell].name);
                            strcat(message_text, name);
                            Warning_Box(message_text);
                        }
                        if (spell == SPELL_BANISH && (char)p_battle_units[unit].race < RACE_ARCANE_CREATURE) {
                            done = 0;
                            LBX_Load_Records_Near("message", 0, message_text, 69, 1, 150);
                            far_strcpy(name, 0, p_spell_data[spell].name);
                            strcat(message_text, name);
                            Warning_Box(message_text);
                        }
                        if (spell == SPELL_STAR_FIRES && p_battle_units[unit].race != RACE_CHAOS_CREATURE && p_battle_units[unit].race != RACE_DEATH_CREATURE) {
                            done = 0;
                            LBX_Load_Records_Near("message", 0, message_text, 70, 1, 150);
                            Warning_Box(message_text);
                        }
                        if (spell == SPELL_WEB && (p_battle_units[unit].ability_flags & AFLAG_NON_CORPOREAL)) {
                            done = 0;
                            LBX_Load_Records_Near("message", 0, message_text, 71, 1, 150);
                            Warning_Box(message_text);
                        }
                        if (spell == SPELL_WARP_WOOD && p_battle_units[unit].ranged_type / 10 != RANGED_CLASS_MISSILE) {
                            done = 0;
                            LBX_Load_Records_Near("message", 0, message_text, 72, 1, 150);
                            Warning_Box(message_text);
                        }
                        if (p_spell_data[spell].effect_type == SPELL_FX_UNIT_CURSE || p_spell_data[spell].effect_type == SPELL_FX_NORMAL_UNIT_CURSE ||
                            p_spell_data[spell].effect_type == SPELL_FX_CURSE_NO_RESIST) {
                            if (spell == SPELL_WEB) {
                                if (p_battle_units[unit].web_hp > 0) {
                                    done = 0;
                                    far_strcpy(name, 0, p_spell_data[spell].name);
                                    strcpy(message_text, "That unit already has ");
                                    strcat(message_text, name);
                                    strcat(message_text, " cast on it");
                                    Warning_Box(message_text);
                                }
                            } else if (p_battle_units[unit].combat_flags & p_spell_data[spell].effect_param) {
                                done = 0;
                                far_strcpy(name, 0, p_spell_data[spell].name);
                                strcpy(message_text, "That unit already has ");
                                strcat(message_text, name);
                                strcat(message_text, " cast on it");
                                Warning_Box(message_text);
                            } else {
                                if (p_spell_data[spell].realm == REALM_SORCERY && (p_battle_units[unit].abilities & ABILITY_ILLUSIONS_IMMUNITY)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message", 0, message_text, 73, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (p_spell_data[spell].realm == REALM_DEATH && (p_battle_units[unit].abilities & ABILITY_DEATH_IMMUNITY)) {
                                    done = 0;
                                    Warning_Box("That unit is immune to Death spells");
                                }
                            }
                        }
                    } else {
                        if (combat_spell_target_mode == CTARGET_OWN || combat_spell_target_mode == CTARGET_ANY_SQUARE ||
                            combat_spell_target_mode == CTARGET_VORTEX_OR_UNIT)
                            done = 1;
                        if (combat_spell_target_mode == CTARGET_OWN_NORMAL && (char)p_battle_units[unit].race < RACE_ARCANE_CREATURE)
                            done = 1;
                        if (combat_spell_target_mode == CTARGET_OWN_HERO && p_units[p_battle_units[unit].unit_index].hero_slot > -1)
                            done = 1;
                        if (spell == SPELL_HEALING && (p_battle_units[unit].race == RACE_DEATH_CREATURE ||
                                              (p_units[p_battle_units[unit].unit_index].mutation_flags & MUT_UNDEAD))) {
                            done = 0;
                            LBX_Load_Records_Near("message", 0, message_text, 74, 1, 150);
                            Warning_Box(message_text);
                        }
                        if (p_spell_data[spell].effect_type == SPELL_FX_UNIT_ENCH || p_spell_data[spell].effect_type == SPELL_FX_NORMAL_UNIT_ENCH) {
                            if (spell == SPELL_HASTE) {
                                if (p_battle_units[unit].combat_flags & COMBAT_FLAG_HASTE) {
                                    done = 0;
                                    far_strcpy(name, 0, p_spell_data[spell].name);
                                    strcpy(message_text, "That unit already has ");
                                    strcat(message_text, name);
                                    strcat(message_text, " cast on it");
                                    Warning_Box(message_text);
                                }
                            } else if (ench & p_spell_data[spell].effect_param) {
                                done = 0;
                                far_strcpy(name, 0, p_spell_data[spell].name);
                                strcpy(message_text, "That unit already has ");
                                strcat(message_text, name);
                                strcat(message_text, " cast on it");
                                Warning_Box(message_text);
                            } else {
                                if (spell == SPELL_IMMOLATION && (p_battle_units[unit].abilities2 & ABILITY2_IMMOLATION)) {
                                    done = 0;
                                    /* "message.lbx" or "message": the loader adds .LBX when it is missing */
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_CLOAK_OF_FEAR && (p_battle_units[unit].abilities2 & ABILITY2_CAUSE_FEAR_SPELL)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_TRUE_SIGHT && (p_battle_units[unit].abilities & ABILITY_ILLUSIONS_IMMUNITY)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_MAGIC_IMMUNITY && (p_battle_units[unit].abilities & ABILITY_MAGIC_IMMUNITY)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_PLANAR_TRAVEL && (p_battle_units[unit].ability_flags & AFLAG_PLANE_SHIFT)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_INVISIBILITY && (p_battle_units[unit].ability_flags & AFLAG_INVISIBILITY)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_WRAITH_FORM && (p_battle_units[unit].ability_flags & AFLAG_NON_CORPOREAL)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_WIND_WALKING && (p_battle_units[unit].ability_flags & AFLAG_WIND_WALKING)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                                if (spell == SPELL_REGENERATION && (p_battle_units[unit].ability_flags & AFLAG_REGENERATION)) {
                                    done = 0;
                                    LBX_Load_Records_Near("message.lbx", 0, message_text, 90, 1, 150);
                                    Warning_Box(message_text);
                                }
                            }
                        }
                    }
                } else {
                    if (combat_spell_target_mode == CTARGET_EMPTY_SQUARE || combat_spell_target_mode == CTARGET_ANY_SQUARE) {
                        done = 1;
                        unit = TARGET_SQUARE;
                    }
                    if (combat_spell_target_mode == CTARGET_DEFENDER_HALF && *x < 11) {
                        done = 1;
                        unit = TARGET_SQUARE;
                        for (j = 0; j < battle_unit_count && done == 1; j++) {
                            if (p_battle_units[j].status == BATTLE_UNIT_ACTIVE && p_battle_units[j].combat_x == *x &&
                                p_battle_units[j].combat_y == *y) {
                                done = 0;
                                LBX_Load_Records_Near("message", 0, message_text, 88, 1, 150);
                                Warning_Box(message_text);
                            }
                        }
                    }
                    if (combat_spell_target_mode == CTARGET_ATTACKER_HALF && *x >= 11) {
                        done = 1;
                        unit = TARGET_SQUARE;
                        for (j = 0; j < battle_unit_count && done == 1; j++) {
                            if (p_battle_units[j].status == BATTLE_UNIT_ACTIVE && p_battle_units[j].combat_x == *x &&
                                p_battle_units[j].combat_y == *y) {
                                done = 0;
                                LBX_Load_Records_Near("message", 0, message_text, 88, 1, 150);
                                Warning_Box(message_text);
                            }
                        }
                    }
                }
            }
        }
        if (done == 0) {
            strcpy(message_text, "Select a target for a ");
            far_strcpy(name, 0, p_spell_data[spell].name);
            strcat(message_text, name);
            strcat(message_text, " spell.");
            Combat_Spell_Target_Redraw();
            Screen_Present();
            Wait_Ticks(2);
        }
    }
    UI_Clear_Controls();
    UI_Set_Redraw_Fn(Combat_Draw_Screen, 2);
    return unit;
}

/* (attacker, target or CLICK_WALL (99) for a wall square, hit, x, y) a ranged attack's missiles: aimed at the
   target (or the square),
   speed 33 per frame (49 when closer than 3 or without movement animations), the missile kind and sound by the
   attacker's ranged type; the target shows its hit at the last frame */
void Combat_Missile_Anim(int attacker, int target, int hit, int x, int y)
{
    int step;
    int i;
    int projectile;
    int dist;
    int speed;
    int sound;

    if (target != CLICK_WALL) {
        dist = Combat_Distance(attacker, target);
        p_battle_units[attacker].dest_x = p_battle_units[target].combat_x;
        p_battle_units[attacker].dest_y = p_battle_units[target].combat_y;
    } else {
        p_battle_units[attacker].dest_x = x;
        p_battle_units[attacker].dest_y = y;
        speed = abs(p_battle_units[attacker].combat_x - x);
        dist = abs(p_battle_units[attacker].combat_y - y);
        if (speed > dist)
            dist = speed;
    }
    if (dist < 3 || game_config.movement_animations == 0)
        speed = 49;
    else
        speed = 33;
    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        Heap_Mark(p_world_block);
    }
    switch (p_battle_units[attacker].ranged_type) {
    case RANGED_BOULDER:
        projectile = PROJECTILE_BOULDER;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 15, p_world_block);
        break;
    case RANGED_UNUSED_11:
        projectile = PROJECTILE_CLOUD;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 16, p_world_block);
        break;
    case RANGED_ARROW:
        projectile = PROJECTILE_ARROW;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 17, p_world_block);
        break;
    case RANGED_SLING:
        projectile = PROJECTILE_SLING;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 18, p_world_block);
        break;
    case RANGED_MAGIC_LIGHTNING:
        projectile = PROJECTILE_LIGHTNING;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 19, p_world_block);
        break;
    case RANGED_MAGIC_FIRE:
        projectile = PROJECTILE_FIREBALL;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 20, p_world_block);
        break;
    case RANGED_MAGIC_ILLUSION:
        projectile = PROJECTILE_ILLUSION;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 21, p_world_block);
        break;
    case RANGED_MAGIC_DEATH:
        projectile = PROJECTILE_DEATH;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 22, p_world_block);
        break;
    case RANGED_MAGIC_WIND:
        projectile = PROJECTILE_WIND;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 23, p_world_block);
        break;
    case RANGED_MAGIC_HOLY:
        projectile = PROJECTILE_HOLY;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 24, p_world_block);
        break;
    case RANGED_MAGIC_DARK:
        projectile = PROJECTILE_DARK;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 25, p_world_block);
        break;
    case RANGED_MAGIC_SPRITE:
    case RANGED_MAGIC_NATURE:
        projectile = PROJECTILE_NATURE;
        if (game_config.sound_effects == 1)
            sound = LBX_Reload_Next("soundfx", 26, p_world_block);
        break;
    }
    if (game_config.sound_effects == 0)
        sound = -1;
    if (target != CLICK_WALL)
        Combat_Missile_List_Build(p_battle_units[attacker].figures_alive, p_battle_units[target].figures_alive,
                                  p_battle_units[attacker].combat_x, p_battle_units[attacker].combat_y,
                                  p_battle_units[target].combat_x, p_battle_units[target].combat_y, projectile);
    else
        Combat_Missile_List_Build(p_battle_units[attacker].figures_alive, 1, p_battle_units[attacker].combat_x,
                                  p_battle_units[attacker].combat_y, x, y, projectile);
    missile_anim_frame = 0;
    missile_anim_frame_copy = 0;
    if (sound != -1)
        Sound_Play(sound);
    for (step = 0; step < 100; step += speed) {
        if (step + speed > 99) {
            missile_anim_frame = 4;
            missile_anim_frame_copy = 4;
            if (target != CLICK_WALL)
                Battle_Unit_Set_Hit_Anim(target, hit);
        } else {
            missile_anim_frame = (missile_anim_frame + 1) % 3;
            missile_anim_frame_copy = (missile_anim_frame_copy + 1) % 3;
        }
        Combat_Draw_Screen();
        Screen_Present();
        for (i = 0; i < combat_missile_count; i++) {
            p_missile_list[i].progress += speed;
            if (p_missile_list[i].progress > 100)
                p_missile_list[i].progress = 100;
        }
    }
    combat_missile_count = 0;
    if (target != CLICK_WALL)
        Battle_Unit_Clear_Hit_Anim(target);
    if (game_config.sound_effects == 1)
        Heap_Release(p_world_block);
}

/* (attacker, target or 99, attacker hit, target hit, x, y) a melee exchange: both units face each other (or the
   attacker the square), 5 frames with movement animations (else 1), the attack sound */
void Combat_Melee_Anim(int attacker, int target, int attacker_hit, int target_hit, int x, int y)
{
    int i;
    int frames;
    unsigned sound;

    if (game_config.movement_animations == 1)
        frames = 5;
    else
        frames = 1;
    Battle_Unit_Set_Hit_Anim(attacker, attacker_hit);
    if (target != CLICK_WALL) {
        Battle_Unit_Set_Hit_Anim(target, target_hit);
        p_battle_units[target].attack_anim_role = ANIM_ROLE_DEFENDER;
        p_battle_units[target].dest_x = p_battle_units[attacker].combat_x;
        p_battle_units[target].dest_y = p_battle_units[attacker].combat_y;
        p_battle_units[attacker].dest_x = p_battle_units[target].combat_x;
        p_battle_units[attacker].dest_y = p_battle_units[target].combat_y;
    } else {
        p_battle_units[attacker].dest_x = x;
        p_battle_units[attacker].dest_y = y;
    }
    p_battle_units[attacker].attack_anim_role = ANIM_ROLE_ATTACKER;
    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        Heap_Mark(p_world_block);
        sound = Unit_Load_Attack_Sound(attacker);
        Sound_Play(sound);
    }
    for (i = 0; i < frames; i++) {
        Timer_Mark();
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
        Wait_Ticks(2);
    }
    Battle_Unit_Clear_Hit_Anim(attacker);
    if (target != CLICK_WALL) {
        Battle_Unit_Clear_Hit_Anim(target);
        p_battle_units[target].attack_anim_role = 0;
    }
    p_battle_units[attacker].attack_anim_role = 0;
    if (game_config.sound_effects == 1)
        Heap_Release(p_world_block);
}

/* Spell_Damage_Roll's own mark, set from the spell's 0x1000: a figure dies outright when the damage exceeds its hits */
#define DMG_WHOLE_FIGURES 0x2000
/* the attack-flag word of a damage spell (SPELLDAT effect_param's high word) also carries: */
#define SPELL_DMG_EACH_FIGURE 0x1000   /* one attack per living figure (DMG_WHOLE_FIGURES set) */
#define SPELL_DMG_STR_ATTACKS 0x8000   /* 'strength' attacks of that strength */

/* (spell, target, damage[3], strength) -> 0 when the target is immune (Magic Immunity, or Righteousness against
   chaos and death); else rolls the spell's damage figure by figure into damage[0]: the spell's strength (or the
   given one), its to-hit rolls against the target's shields and defense (Invulnerability -2 per block roll), the front
   figure's previous damage carried over; flag bits of the spell: 0x10 ignores defense, 0x1000 hits every figure
   (each killed outright when the damage exceeds its hits), 0x8000 one attack per strength point (strength falling),
   0x4000 defense -1 */
int Spell_Damage_Roll(int spell, int target, int *damage, int strength)
{
    int k;
    int front;
    int flags;
    int damage_flags;
    int shields;
    int str;
    int attacks;
    int defense;
    int kills;
    int total;
    long ench;
    int dmg;

    kills = 0;
    dmg = 0;
    ench = p_units[p_battle_units[target].unit_index].enchantments | p_battle_units[target].combat_enchantments |
           p_battle_units[target].item_enchantments;
    total = 0;
    for (k = 0; k < 3; k++)
        damage[k] = 0;
    if (((ench & UNIT_ENCH_RIGHTEOUSNESS) && (p_spell_data[spell].realm == REALM_CHAOS || p_spell_data[spell].realm == REALM_DEATH)) ||
        (p_battle_units[target].abilities & ABILITY_MAGIC_IMMUNITY))
        return 0;
    front = p_battle_units[target].front_damage;
    /* a damage spell's effect_param: byte 0 the strength, byte 1 the damage flags, the high word the attack flags */
    flags = *((int far *)&p_spell_data[spell].effect_param + 1);
    defense = p_battle_units[target].block_bonus;
    if (flags & ATTACK_ELDRITCH_WEAPON)
        defense--;
    damage_flags = *((unsigned char far *)&p_spell_data[spell].effect_param + 1);
    str = strength > 0 ? strength : *((unsigned char far *)&p_spell_data[spell].effect_param);
    shields = Defender_Shields(target, 0x26, damage_flags, flags, p_spell_data[spell].realm);   /* 0x26: a magic
                                                   ranged type (30..38), so the spell is shielded as ranged magic */
    if (flags & SPELL_DMG_EACH_FIGURE) {
        attacks = p_battle_units[target].figures_alive;
        flags = flags | DMG_WHOLE_FIGURES;
    } else if (flags & SPELL_DMG_STR_ATTACKS)
        attacks = str;
    else
        attacks = 1;
    if (p_battle_units[target].combat_flags & COMBAT_FLAG_BLACK_SLEEP)
        flags = flags | ATTACK_AUTOMATIC_DAMAGE;
    for (k = 0; k < attacks; k++) {
        if (flags & ATTACK_AUTOMATIC_DAMAGE)
            dmg = str;
        else {
            dmg += Roll_To_Hit(str, 0);
            dmg -= Roll_Blocks(shields, defense);
            if (ench & UNIT_ENCH_INVULNERABILITY)
                dmg -= 2;
            if (dmg < 0)
                dmg = 0;
        }
        if (k == 0 && front >= 0) {
            dmg += front;
            front = 0;
        }
        if (front < 0) {
            front += dmg;
            if (front > 0) {
                dmg = front;
                front = 0;
            } else
                dmg = 0;
        }
        if (flags & DMG_WHOLE_FIGURES) {
            if (p_battle_units[target].hits_per_figure < dmg) {
                kills++;
                dmg = 0;
            }
        } else {
            while (p_battle_units[target].hits_per_figure < dmg) {
                kills++;
                dmg -= p_battle_units[target].hits_per_figure;
                if ((flags & ATTACK_AUTOMATIC_DAMAGE) == 0) {
                    dmg -= Roll_Blocks(shields, defense);
                    if (ench & UNIT_ENCH_INVULNERABILITY)
                        dmg -= 2;
                }
            }
            if (dmg < 0)
                dmg = 0;
        }
        total += dmg + p_battle_units[target].hits_per_figure * kills;
        dmg = 0;
        kills = 0;
        if (flags & 0x8000)
            str--;
    }
    total = total - p_battle_units[target].front_damage;
    if (total < 0)
        total = 0;
    damage[DAMAGE_NORMAL] = total;
}

/* (unit, damage[3]) applies the three damage kinds to a living battle unit (each capped at 200): whole figures
   die, the rest stays as the front figure's damage; at no figures left the unit dies, its status by the largest
   damage kind: DAMAGE_DESTROY -> BATTLE_UNIT_DESTROYED, DAMAGE_DRAIN -> BATTLE_UNIT_DRAINED (DESTROYED for a
   combat-only summon), DAMAGE_NORMAL -> BATTLE_UNIT_DEAD; a combat-only summon's map plane set to PLANE_COMBAT_ONLY
   Numbered: BUG #303 (BUGS_AND_QUIRKS.txt) */
void Battle_Apply_Damage(int unit, int *damage)
{
    int total;
    int kills;
    int i;

    total = 0;
    for (i = 0; i < 3; i++)
        total += damage[i];
    if (total > 0 && p_battle_units[unit].status == BATTLE_UNIT_ACTIVE) {
        for (i = 0; i < 3; i++) {
            if (p_battle_units[unit].damage[i] + damage[i] > 200)
                p_battle_units[unit].damage[i] = 200;
            else
                p_battle_units[unit].damage[i] = p_battle_units[unit].damage[i] + damage[i];
        }
        total += p_battle_units[unit].front_damage;
        if (total > 0) {
            kills = total / p_battle_units[unit].hits_per_figure;
            if (p_battle_units[unit].figures_alive < kills)
                kills = p_battle_units[unit].figures_alive;
            p_battle_units[unit].figures_alive = p_battle_units[unit].figures_alive - kills;
            p_battle_units[unit].front_damage = total % p_battle_units[unit].hits_per_figure;
        }
        if (p_battle_units[unit].figures_alive <= 0) {
            p_battle_units[unit].figures_alive = 0;
            combat_winner = Combat_Count_Sides();   /* BUG #303: still BATTLE_UNIT_ACTIVE (set below), so it counts: this
                                                       death cannot end the battle here */
            if (Is_Combat_Only_Summon(unit))
                p_units[p_battle_units[unit].unit_index].plane = PLANE_COMBAT_ONLY;
            if (p_battle_units[unit].damage[DAMAGE_DESTROY] >= p_battle_units[unit].damage[DAMAGE_DRAIN] &&
                p_battle_units[unit].damage[DAMAGE_DESTROY] >= p_battle_units[unit].damage[DAMAGE_NORMAL])
                p_battle_units[unit].status = BATTLE_UNIT_DESTROYED;
            else if (p_battle_units[unit].damage[DAMAGE_DRAIN] > p_battle_units[unit].damage[DAMAGE_DESTROY] &&
                     p_battle_units[unit].damage[DAMAGE_DRAIN] >= p_battle_units[unit].damage[DAMAGE_NORMAL]) {
                if (p_units[p_battle_units[unit].unit_index].plane == PLANE_COMBAT_ONLY)
                    p_battle_units[unit].status = BATTLE_UNIT_DESTROYED;
                else
                    p_battle_units[unit].status = BATTLE_UNIT_DRAINED;
            } else if (p_battle_units[unit].damage[DAMAGE_NORMAL] > p_battle_units[unit].damage[DAMAGE_DESTROY] &&
                       p_battle_units[unit].damage[DAMAGE_NORMAL] > p_battle_units[unit].damage[DAMAGE_DRAIN])
                p_battle_units[unit].status = BATTLE_UNIT_DEAD;
            Combat_Update_See_Invisible();
        }
    }
}

/* (attacker, target) -> 1 when the attacker cannot see the invisible target (no True Sight, no Illusions
   Immunity), 2 when a Wall of Darkness hides a target inside the city from an attacker outside it, else 0. The Wall
   of Darkness test checks True Sight only, not Illusions Immunity (which the invisibility test accepts): #263
   Numbered: BUG #263 (BUGS_AND_QUIRKS.txt) */
int Ranged_Target_Blocked(int attacker, int target)
{
    long att_ench;
    long tgt_ench;
    int result;

    tgt_ench = p_battle_units[target].combat_enchantments | p_battle_units[target].item_enchantments |
               p_units[p_battle_units[target].unit_index].enchantments;
    att_ench = p_battle_units[attacker].combat_enchantments | p_battle_units[attacker].item_enchantments |
               p_units[p_battle_units[attacker].unit_index].enchantments;
    result = 0;
    if ((att_ench & UNIT_ENCH_TRUE_SIGHT) || (p_battle_units[attacker].abilities & ABILITY_ILLUSIONS_IMMUNITY))
        result = 0;
    else if ((tgt_ench & UNIT_ENCH_INVISIBILITY) || (p_battle_units[target].ability_flags & AFLAG_INVISIBILITY))
        result = 1;
    if (p_battlefield->wall_of_darkness == 1 && (att_ench & UNIT_ENCH_TRUE_SIGHT) == 0 && In_City_Area(target) == 1 &&
        In_City_Area(attacker) == 0)
        result = 2;
    return result;
}

/* (player) places the side's units at the start of a battle: melee units in the front ranks, missile units behind
   (when more than 5 missile units, up to all but 4 of them move to the front while fewer than 4
   stand there, the best melee first - BUG #19: chosen by battle unit index k, not
   ranged[k]); the defender skips the wall-corner squares (8, 13) and (8, 10) of city walls and square 9 - the feature square (6, 11) - unless the
   backdrop is a city; all face the
   enemy
   Numbered: BUG #19, #255 (BUGS_AND_QUIRKS.txt) */
void Combat_Deploy_Side(int player)
{
    int melee_n;
    int ranged_n;
    int k;
    int best;
    int best_k;
    int ranged_extra;
    int melee[9];
    int ranged[9];
    int pos_x[12];
    int pos_y[12];
    int i;
    int pos;

    Water_Battle_Sideline(player);
    if (player == combat_defender_player) {
        pos_x[0] = 8;
        pos_x[1] = 8;
        pos_x[2] = 8;
        pos_x[3] = 8;
        pos_x[4] = 7;
        pos_x[5] = 7;
        pos_x[6] = 7;
        pos_x[7] = 7;
        pos_x[8] = 6;
        pos_x[9] = 6;
        pos_x[10] = 6;
        pos_x[11] = 6;
    } else {
        pos_x[0] = 14;
        pos_x[1] = 14;
        pos_x[2] = 14;
        pos_x[3] = 14;
        pos_x[4] = 15;
        pos_x[5] = 15;
        pos_x[6] = 15;
        pos_x[7] = 15;
        pos_x[8] = 16;
        pos_x[9] = 16;
        pos_x[10] = 16;
        pos_x[11] = 16;
    }
    pos_y[0] = 12;
    pos_y[1] = 11;
    pos_y[2] = 13;
    pos_y[3] = 10;
    pos_y[4] = 12;
    pos_y[5] = 11;
    pos_y[6] = 13;
    pos_y[7] = 10;
    pos_y[8] = 12;
    pos_y[9] = 11;
    pos_y[10] = 13;
    pos_y[11] = 10;
    melee_n = 0;
    ranged_n = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner == player) {
            if (Has_Shooting_Attack(i)) {
                ranged[ranged_n] = i;
                ranged_n++;
            } else {
                melee[melee_n] = i;
                melee_n++;
            }
        }
    }
    if (ranged_n > 5) {
        ranged_extra = ranged_n - 4;
        for (i = 0; i < ranged_extra && melee_n < 4; i++) {
            best = -1;
            best_k = -1;
            for (k = 0; k < ranged_n; k++) {
                if (p_battle_units[k].melee > best) {
                    best = p_battle_units[k].melee;
                    best_k = k;
                }
            }
            if (melee_n < 4) {
                melee[melee_n] = ranged[best_k];
                melee_n++;
                Array_Delete_Near(best_k, (char *)ranged, 2, ranged_n);
                ranged_n--;
            }
        }
    }
    pos = 0;
    for (i = 0; i < melee_n; i++) {
        if (combat_defender_player == player) {
            while (p_battlefield->has_city_walls == 1 && (pos == 2 || pos == 3))
                pos++;
            if (p_battlefield->backdrop != BACKDROP_CITY && pos == 9)
                pos++;
        }
        p_battle_units[melee[i]].combat_x = pos_x[pos];
        p_battle_units[melee[i]].combat_y = pos_y[pos];
        pos++;
    }
    for (i = 0; i < ranged_n; i++) {
        if (combat_defender_player == player) {
            while (p_battlefield->has_city_walls == 1 && (pos == 2 || pos == 3))
                pos++;
            if (p_battlefield->backdrop != BACKDROP_CITY && pos == 9)
                pos++;
        }
        p_battle_units[ranged[i]].combat_x = pos_x[pos];
        p_battle_units[ranged[i]].combat_y = pos_y[pos];
        pos++;
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner == player) {
            if (combat_attacker_player == player)
                p_battle_units[i].dest_x = p_battle_units[i].combat_x - 1;
            else
                p_battle_units[i].dest_x = p_battle_units[i].combat_x + 1;
            p_battle_units[i].dest_y = p_battle_units[i].combat_y;
        }
    }
}

/* (unit) a Demon Lord's summoning: uses up one of its summons (the 0x1800 count bits), creates a Demon (type 169)
   for its owner and places it on a free square of the owner's half near the middle. BUG #232: the square loop has no
   try limit - when all 9 squares of the 3 x 3 are taken (by either side) it never ends (AI_Combat_Spell_Target's
   summon gives up after 200) */
void Demon_Lord_Summon(int unit)
{
    int demons;
    int gy;
    int gx;

    demons = p_battle_units[unit].abilities & ABILITY_SUMMON_DEMONS;
    if (demons == ABILITY_SUMMON_DEMONS_1)
        p_battle_units[unit].abilities = p_battle_units[unit].abilities ^ ABILITY_SUMMON_DEMONS_1;
    else if (demons == ABILITY_SUMMON_DEMONS_2) {
        p_battle_units[unit].abilities = p_battle_units[unit].abilities ^ ABILITY_SUMMON_DEMONS_2;
        p_battle_units[unit].abilities = p_battle_units[unit].abilities | ABILITY_SUMMON_DEMONS_1;
    } else
        p_battle_units[unit].abilities = p_battle_units[unit].abilities ^ ABILITY_SUMMON_DEMONS_1;
    Unit_Create(UNIT_DEMON, p_battle_units[unit].owner, 0, 0, PLANE_COMBAT_ONLY, UNIT_SRC_NO_CAP);
    do {
        if (p_battle_units[unit].owner == combat_attacker_player)
            gx = 14 - Random(3);
        else
            gx = Random(3) + 7;
        gy = Random(3) + 8;
    } while (combat_click_map[gy][gx] >= 0);
    Combat_Summon_Add_Unit(p_battle_units[unit].owner, num_units - 1, gx, gy);
    Combat_Summon_Anim(battle_unit_count - 1, gx, gy, SPELL_NONE, unit);
}

/* (attacker, target) -> 0 when city walls stop the melee attack: a walking attacker (not flying, teleporting,
   merging or non-corporeal) outside the city against a target inside it, standing on a wall square other than the
   gate (8, 12); else 1
   Numbered: BUG #350 (BUGS_AND_QUIRKS.txt) */
int Melee_Wall_Allowed(int attacker, int target)
{
    if (p_battlefield->has_city_walls == 1 && (p_battle_units[attacker].move_flags & MOVE_FLYING) == 0 &&
        (p_battle_units[attacker].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[attacker].move_flags & MOVE_MERGING) == 0 &&
        (p_battle_units[attacker].ability_flags & AFLAG_NON_CORPOREAL) == 0 && In_City_Area(target) == 1 &&
        In_City_Area(attacker) == 0 &&
        (p_battle_units[target].combat_x != BF_GATE_X || p_battle_units[target].combat_y != BF_GATE_Y) &&
        Is_City_Wall_Square(p_battle_units[target].combat_x, p_battle_units[target].combat_y) == 1)
        return 0;
    else
        return 1;
}

/* () Mana Leak at the start of a combat turn: the side under it (both: 666) loses 5 spell points per unit, magic
   ranged units an ammunition (out of both: no ranged attack left), and its wizard 5 mana */
void Mana_Leak_Turn(void)
{
    int side;
    int i;

    side = -1;
    if (p_combat_enchants[CENCH_MANA_LEAK_ATT] > 0) {
        side = combat_defender_player;
        if (p_combat_enchants[CENCH_MANA_LEAK_DEF] > 0)
            side = SIDE_BOTH;
    } else if (p_combat_enchants[CENCH_MANA_LEAK_DEF] > 0)
        side = combat_attacker_player;
    if (side >= 0) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (p_battle_units[i].owner == side || side == SIDE_BOTH)) {
                if (p_battle_units[i].spell_points >= 5)
                    p_battle_units[i].spell_points = p_battle_units[i].spell_points - 5;
                else
                    p_battle_units[i].spell_points = 0;
                if (p_battle_units[i].ranged_type / 10 == RANGED_CLASS_MAGIC) {
                    if (p_battle_units[i].ammo > 0)
                        p_battle_units[i].ammo = p_battle_units[i].ammo - 1;
                    if (p_battle_units[i].ammo == 0 && p_battle_units[i].spell_points < 3) {
                        p_battle_units[i].ranged_type = RANGED_NONE;
                        p_battle_units[i].ranged = 0;
                    }
                }
            }
        }
        if (side < num_players) {
            wizards[side].mana_reserve -= 5;
            if (wizards[side].mana_reserve < 0)
                wizards[side].mana_reserve = 0;
        } else if (side != PLAYER_NEUTRAL) {    /* side SIDE_BOTH: both wizards pay */
            if (combat_attacker_player < num_players)
                wizards[combat_attacker_player].mana_reserve -= 5;
            if (wizards[combat_attacker_player].mana_reserve < 0)
                wizards[combat_attacker_player].mana_reserve = 0;
            if (combat_defender_player < num_players)
                wizards[combat_defender_player].mana_reserve -= 5;
            if (wizards[combat_defender_player].mana_reserve < 0)
                wizards[combat_defender_player].mana_reserve = 0;
        }
    }
}

/* () message_text as a paragraph in a shaded, framed banner at the top of the battlefield */
void Combat_Draw_Banner_Text(void)
{
    int x2;
    int y2;
    char colors[2];
    int width;
    int height;
    register int x1;
    register int y1;

    colors[0] = 0xb6;
    colors[1] = 0xb1;
    Font_Select_Palette(0, colors);
    Font_Select_Shadow(1, 15, 0, 0);
    Set_Font_Color(182);
    Set_Text_Shadow_Color(2);
    width = Paragraph_Used_Width(150, message_text, PARA_CENTER);
    height = Paragraph_Height(150, message_text, 2);
    x1 = 160 - width / 2 - 5;
    x2 = width / 2 + 168;
    y1 = 5;
    y2 = height + 8;
    Fill_Box_Styled(x1, y1, x2, y2, FILL_REMAP, 8);
    Draw_Line(x1, y1, x1, y2, 0xe6);
    Draw_Line(x1, y1, x2 - 1, y1, 0xe6);
    Draw_Line(x2, y1, x2, y2, 0xed);
    Draw_Line(x1 + 1, y2, x2, y2, 0xed);
    Paragraph_Print(x1 + 6, 8, 150, message_text, PARA_CENTER);
}

/* (caster: battle unit < 20 or 20 + player, spell or 0 for a Demon Lord's summoning) the "<wizard / hero / The
   units> has (have) cast <spell>" banner. The hero's name is read from the battle owner's hired_heroes, not the unit's
   own owner: a hero turned by Confusion (owner swapped) shows the other wizard's hero in that slot (#289) */
void Combat_Cast_Banner(int caster, int spell)
{
    int x1;
    int y1;
    int x2;
    int y2;
    char colors[2];
    int width;
    char name[20];

    if (caster >= CASTER_WIZARD) {
        strcpy(message_text, wizards[caster - CASTER_WIZARD].wizard_name);
        strcat(message_text, " has cast ");
    } else if (p_units[p_battle_units[caster].unit_index].hero_slot > -1) {
        strcpy(message_text,
               wizards[p_battle_units[caster].owner].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].name);
        strcat(message_text, " has cast ");
    } else {
        strcpy(message_text, "The ");
        strcat(message_text, unit_types[p_units[p_battle_units[caster].unit_index].type].name);
        if (p_battle_units[caster].figures > 1)
            strcat(message_text, " have cast ");
        else
            strcat(message_text, " has cast ");
    }
    if (spell != SPELL_NONE) {
        far_strcpy(name, 0, p_spell_data[spell].name);
        strcat(message_text, name);
    } else
        strcat(message_text, "summon demon");
    colors[0] = 0xb6;
    colors[1] = 0xb1;
    Font_Select_Palette(0, colors);
    Font_Select_Shadow(1, 15, 0, 0);
    Set_Font_Color(182);
    Set_Text_Shadow_Color(2);
    width = Text_Width(message_text);
    x1 = 160 - width / 2 - 5;
    x2 = width / 2 + 165;
    y1 = 5;
    y2 = 15;
    Fill_Box_Styled(x1, y1, x2, y2, FILL_REMAP, 8);
    Draw_Line(x1, y1, x1, y2, 0xe6);
    Draw_Line(x1, y1, x2 - 1, y1, 0xe6);
    Draw_Line(x2, y1, x2, y2, 0xed);
    Draw_Line(x1 + 1, y2, x2, y2, 0xed);
    Print_Text_Centered(160, 8, message_text);
}

/* () -> the first of the 18 figure graphics slots no living battle unit uses (none free: no value) */
int Battle_Free_Gfx_Slot(void)
{
    int used[COMBAT_SPRITE_SLOTS];
    int i;

    for (i = 0; i < COMBAT_SPRITE_SLOTS; i++)
        used[i] = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].gfx_slot > -1 && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
            used[p_battle_units[i].gfx_slot] = 1;
    }
    for (i = 0; i < COMBAT_SPRITE_SLOTS; i++) {
        if (used[i] == 0)
            return i;
    }
}

/* () -> the winner when one side has no living units left (units whose Confusion changed their side - confusion_result 2 - count for the other side),
   else -1
   Numbered: BUG #303 (BUGS_AND_QUIRKS.txt) */
int Combat_Count_Sides(void)
{
    int attackers;
    int defenders;
    int i;

    attackers = 0;
    defenders = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            if (p_battle_units[i].owner == combat_attacker_player) {
                if (p_battle_units[i].confusion_result == CONFUSION_CHANGED_SIDE)
                    defenders++;
                else
                    attackers++;
            } else if (p_battle_units[i].owner == combat_defender_player) {
                if (p_battle_units[i].confusion_result == CONFUSION_CHANGED_SIDE)
                    attackers++;
                else
                    defenders++;
            }
        }
    }
    if (attackers == 0)
        return combat_defender_player;
    if (defenders == 0)
        return combat_attacker_player;
    return -1;
}
