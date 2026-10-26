/* UNITDRAW.C - WIZARDS.EXE code segment 55bb: the overland main screen's redraw (map, frame, minimap, the income or
   stack panel, the Gold / Mana reserve, the GoTo preview and the Moves line), the drawing of a unit's map figure
   (banner backdrop and colors, invisibility, the patrol shade, the enchantment outline in its realm color, the status
   letter), its badges (health bar, level medals, weapon quality), the movement-type icons, and the stack queries
   behind them (moves of the selected stack, Planar Travel, Astral Gate, a city at a square, Planar Seal).
   This module's data (DS:305c): the outline counters, realm_outline_colors and its literals; no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 55bb src/wizards/UNITDRAW.C */

/* the module's functions in their original declaration order */
void Draw_Gold_Mana_Reserve(void);
void Overland_Screen_Draw(int *vx, int *vy, int plane, int tx, int ty, int player);
void Draw_Unit_Enchant_Outline(int unit);
void Outline_Cycle_Tick(void);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void Draw_Unit_Figure(int x, int y, int unit, int icon_mode);
void Draw_Unit_Badges(int x, int y, int unit);
void Draw_Stack_Moves_Line(void);
int Draw_Move_Type_Icons(int x, int y, int unit);
int Stack_Can_Planar_Travel(void);
int Stack_In_Astral_Gate_City(void);
int Stack_Selected_Moves(void);
int Stack_Moves_WindWalk_Carry(void);
int City_At(int x, int y, int plane);
int Planar_Seal_Active(void);

#include "GAME.H"

int weapon_badge_frame = 0;                     /* DS:305c  the weapon-quality badge's frame */
int outline_flash_phase = 0;                    /* DS:305e  0..7, the enchantment outline's color phase */
int outline_flash_div = 0;                      /* DS:3060  0..1, outline_flash_phase steps every 2nd tick */
unsigned char realm_outline_colors[5][8][8] = {   /* [OUTLINE_*][phase][pixel] */
    {{0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x2a, 0x2b, 0x2c},
     {0x2c, 0x2b, 0x2a, 0x29, 0x2a, 0x2b, 0x2c, 0x2d},
     {0x2b, 0x2a, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2c},
     {0x2a, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2c, 0x2b},
     {0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2c, 0x2b, 0x2a},
     {0x2a, 0x2b, 0x2c, 0x2d, 0x2c, 0x2b, 0x2a, 0x29},
     {0x2b, 0x2c, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x2a},
     {0x2c, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x2a, 0x2b}},
    {{0x24, 0x23, 0x22, 0x21, 0x20, 0x21, 0x22, 0x23},
     {0x23, 0x22, 0x21, 0x20, 0x21, 0x22, 0x23, 0x24},
     {0x22, 0x21, 0x20, 0x21, 0x22, 0x23, 0x24, 0x23},
     {0x21, 0x20, 0x21, 0x22, 0x23, 0x24, 0x23, 0x22},
     {0x20, 0x21, 0x22, 0x23, 0x24, 0x23, 0x22, 0x21},
     {0x21, 0x22, 0x23, 0x24, 0x23, 0x22, 0x21, 0x20},
     {0x22, 0x23, 0x24, 0x23, 0x22, 0x21, 0x20, 0x21},
     {0x23, 0x24, 0x23, 0x22, 0x21, 0x20, 0x21, 0x22}},
    {{0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c},
     {0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b},
     {0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0c},
     {0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0c, 0x0d},
     {0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0c, 0x0d, 0x0e},
     {0x0e, 0x0d, 0x0c, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
     {0x0d, 0x0c, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e},
     {0x0c, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d}},
    {{0x44, 0x43, 0x42, 0x41, 0x40, 0x41, 0x42, 0x43},
     {0x43, 0x42, 0x41, 0x40, 0x41, 0x42, 0x43, 0x44},
     {0x42, 0x41, 0x40, 0x41, 0x42, 0x43, 0x44, 0x43},
     {0x41, 0x40, 0x41, 0x42, 0x43, 0x44, 0x43, 0x42},
     {0x40, 0x41, 0x42, 0x43, 0x44, 0x43, 0x42, 0x41},
     {0x41, 0x42, 0x43, 0x44, 0x43, 0x42, 0x41, 0x40},
     {0x42, 0x43, 0x44, 0x43, 0x42, 0x41, 0x40, 0x41},
     {0x43, 0x44, 0x43, 0x42, 0x41, 0x40, 0x41, 0x42}},
    {{0x64, 0x63, 0x62, 0x61, 0x60, 0x61, 0x62, 0x63},
     {0x63, 0x62, 0x61, 0x60, 0x61, 0x62, 0x63, 0x64},
     {0x62, 0x61, 0x60, 0x61, 0x62, 0x63, 0x64, 0x63},
     {0x61, 0x60, 0x61, 0x62, 0x63, 0x64, 0x63, 0x62},
     {0x60, 0x61, 0x62, 0x63, 0x64, 0x63, 0x62, 0x61},
     {0x61, 0x62, 0x63, 0x64, 0x63, 0x62, 0x61, 0x60},
     {0x62, 0x63, 0x64, 0x63, 0x62, 0x61, 0x60, 0x61},
     {0x63, 0x64, 0x63, 0x62, 0x61, 0x60, 0x61, 0x62}}
};                                              /* DS:3062  per realm (0 Chaos .. 4 Sorcery) 8 phases x 8 colors */

extern unsigned char banner_shade_colors[][5];  /* DS:00aa */
extern unsigned unit_figure_pic;                /* DS:9648 */
extern unsigned main_screen_bg_pic;             /* DS:9922 */
extern unsigned medal_red_pic;                  /* DS:9946 */
extern unsigned medal_gold_pic;                 /* DS:9948 */
extern unsigned medal_white_pic;                /* DS:994a */
extern unsigned weapon_adamantium_pic;          /* DS:994c */
extern unsigned weapon_mithril_pic;             /* DS:994e */
extern unsigned weapon_magic_pic;               /* DS:9950 */
extern unsigned move_type_icons[];              /* DS:9978 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int goto_preview_len;                    /* DS:c066 */
extern int goto_preview_on;                     /* DS:c068 */
extern int stack_panel_y;                       /* DS:c082 */
extern int stack_panel_x;                       /* DS:c084 */
extern int stack_moves_cached;                  /* DS:c08e */
extern char path_y[];                           /* DS:c668 */
extern char path_x[];                           /* DS:c6e0 */
extern unsigned unit_banner_back_pics[6];       /* DS:cb92  MAPBACK.LBX 14..19, a unit's backdrop by banner color (Load_Map_Assets) */

void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
void Minimap_Set_Size(int width, int height);
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty, int player);
void Minimap_Draw(int x, int y, int w, int h);
void Draw_Income_Panel(void);
void Draw_Order_Buttons_Disabled(void);
void Draw_Next_Turn_Panel(void);
int Overland_Draw_Stack_Panel();                /* no prototype here: called with the panel x, y; the definition in MAINSCR.C takes none */
void Overland_Draw_Order_Buttons(void);
void Draw_GoTo_Path_Markers(int len, char *xs, char *ys);
void Overland_Draw_TopBar(void);
int Unit_Is_Invisible(int unit);
void Pic_Outline_Recolor2(unsigned pic, int color);
void Pic_Outline_Recolor(unsigned pic, int color);
void Pic_Replace_Color_Dithered(unsigned pic, int color, char *pattern);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Replace_Color(unsigned pic, int from, int to);
void Pic_Recolor_Opaque(unsigned pic, int color);
void Pic_Remap_Shade(unsigned pic, int shade);
void Pic_Set_Remap_Flag(unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
int Unit_Max_Hits(int unit);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Print_Half_Moves(int x, int y, int half_moves, int align);
void Stack_Active_Movement_Modes(int *modes);
void Stack_Movement_Modes(int *modes, int *units, int count);
int Hero_Item_Planar_Travel(int unit);

/* the current wizard's gold and mana reserve with "GP" / "MP" (the smaller font once either passes 9999) */
void Draw_Gold_Mana_Reserve(void)
{
    Set_Text_Shadow_Color(0);
    if (wizards[current_player].gold > 9999 || wizards[current_player].mana_reserve > 9999)
        Font_Select_Shadow(0, 0, 0, 0);
    else
        Font_Select_Shadow(1, 0, 0, 0);
    Set_Font_Color(8);
    Print_Number_Right(265, 68, wizards[current_player].gold);
    Print_Number_Right(303, 68, wizards[current_player].mana_reserve);
    Font_Select_Shadow(0, 0, 0, 0);
    Print_Text(268, 68, "GP");
    Print_Text(306, 68, "MP");
    Set_Text_Shadow_Color(0);
}

/* (&view_x, &view_y, plane, x, y, player) the whole main screen: the 12 x 10 map, the frame, the minimap, the stack
   panel and order buttons (or the income panel, greyed buttons and Next Turn panel), the reserve, the GoTo preview,
   the top bar and the Moves line */
void Overland_Screen_Draw(int *vx, int *vy, int plane, int tx, int ty, int player)
{
    Map_Invalidate();
    Minimap_Set_Size(MINIMAP_W, MINIMAP_H);
    Map_Draw_View(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, vx, vy, plane, tx, ty, player);
    Draw_Pic_Frame(0, 0, main_screen_bg_pic);
    Minimap_Draw(MINIMAP_X, MINIMAP_Y, MINIMAP_W, MINIMAP_H);
    help_entries[MAIN_HELP_NEXT_TURN].id = -1;  /* the income panel's help slots: set again below only when no
                                                   stack is selected */
    help_entries[MAIN_HELP_GOLD].id = -1;
    help_entries[MAIN_HELP_FOOD].id = -1;
    help_entries[MAIN_HELP_MANA].id = -1;
    if (stack_unit_count > 0) {
        Overland_Draw_Stack_Panel(stack_panel_x, stack_panel_y);
        Overland_Draw_Order_Buttons();
    } else {
        Draw_Income_Panel();
        Draw_Order_Buttons_Disabled();
        Draw_Next_Turn_Panel();
    }
    Draw_Gold_Mana_Reserve();
    if (goto_preview_on == 1)
        Draw_GoTo_Path_Markers(goto_preview_len, path_x, path_y);
    Overland_Draw_TopBar();
    Draw_Stack_Moves_Line();
}

/* (unit) the shimmering outline of unit_figure_pic in the realm color of the unit's highest-priority enchantment
   (none for an invisible unit) */
void Draw_Unit_Enchant_Outline(int unit)
{
    unsigned long ench;
    register int outline_row;

    ench = p_units[unit].enchantments;
    outline_row = -1;
    if (Unit_Is_Invisible(unit) == 1)
        ench = 0;
    if (ench != 0) {
        if (ench & UNIT_ENCH_REGENERATION)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_RESIST_MAGIC)
            outline_row = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_RIGHTEOUSNESS)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_PLANAR_TRAVEL)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_BLACK_CHANNELS)
            outline_row = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_LIONHEART)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_IRON_SKIN)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_MAGIC_IMMUNITY)
            outline_row = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_WIND_WALKING)
            outline_row = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_ELEMENTAL_ARMOR)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_TRUE_SIGHT)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_INVULNERABILITY)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_PATH_FINDING)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_FLIGHT)
            outline_row = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_ELDRITCH_WEAPON)
            outline_row = OUTLINE_CHAOS;
        else if (ench & UNIT_ENCH_WRAITH_FORM)
            outline_row = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_GIANT_STRENGTH)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_IMMOLATION)
            outline_row = OUTLINE_CHAOS;
        else if (ench & UNIT_ENCH_GUARDIAN_WIND)
            outline_row = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_SPELL_LOCK)
            outline_row = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_HEROISM)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_BERSERK)
            outline_row = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_HOLY_ARMOR)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_WATER_WALKING)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_ENDURANCE)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_CLOAK_OF_FEAR)
            outline_row = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_STONE_SKIN)
            outline_row = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_FLAME_BLADE)
            outline_row = OUTLINE_CHAOS;
        else if (ench & UNIT_ENCH_BLESS)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_HOLY_WEAPON)
            outline_row = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_RESIST_ELEMENTS)
            outline_row = OUTLINE_NATURE;
    }
    if (outline_row != -1) {
        Pic_Outline_Recolor2(unit_figure_pic, 255);
        Pic_Replace_Color_Dithered(unit_figure_pic, 255, realm_outline_colors[outline_row][outline_flash_phase]);
    } else {
    }
}

/* the outline color phase: one step every second call */
void Outline_Cycle_Tick(void)
{
    outline_flash_div = outline_flash_div + 1;
    if (outline_flash_div > 1)
        outline_flash_div = 0;
    if (outline_flash_div < 1) {
        outline_flash_phase = outline_flash_phase + 1;
        if (outline_flash_phase > 7)
            outline_flash_phase = 0;
    }
}

/* (x, y, unit, icon_mode) the unit's banner backdrop (mode != 0; a neutral unit writes BANNER_NEUTRAL (5) into
   wizards[5].banner_color - a lasting global write) and its figure */
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode)
{
    int banner;
    register int player;

    switch (icon_mode) {
    case UNIT_ICON_INACTIVE:
        break;
    case UNIT_ICON_ACTIVE:
    default:
        player = p_units[unit].owner;
        if (player > -1) {
            if (player == PLAYER_NEUTRAL) {
                banner = BANNER_NEUTRAL;
                wizards[player].banner_color = BANNER_NEUTRAL;
            } else
                banner = wizards[player].banner_color;
            Draw_Pic_Frame(x, y, unit_banner_back_pics[banner]);
        }
        break;
    }
    Draw_Unit_Figure(x, y, unit, icon_mode);
}

/* (x, y, unit, icon_mode) the unit's map figure in its banner colors; modes 0..3 add the invisible look, the patrol shade
   (own visible units on Patrol - ORDER_PATROL - not in mode 3) and the enchantment outline; own units in modes 0 / 1 get their status
   letter: B building a road (also GoTo on a road), G GoTo, C for the never-stored ORDER_UNUSED_6 / _7, P purifying */
void Draw_Unit_Figure(int x, int y, int unit, int icon_mode)
{
    int player;
    int banner;
    unsigned unit_type;
    char colors[2];
    register int i;

    unit_type = p_units[unit].type;
    Set_Pic_Frame(unit_types[unit_type].map_figure_pic, 0);
    switch (icon_mode) {
    case UNIT_ICON_INACTIVE:
        Set_Pic_Frame(unit_types[unit_type].map_figure_pic, 1);
        Pic_Decode_Frame_To_Bitmap(unit_types[unit_type].map_figure_pic, unit_figure_pic);
        player = p_units[unit].owner;
        if (player == PLAYER_NEUTRAL)
            banner = BANNER_NEUTRAL;
        else
            banner = wizards[player].banner_color;
        for (i = 0; i < 5; i++)
            Pic_Replace_Color(unit_figure_pic, i + CITY_MARKER_PAL_BASE, banner_shade_colors[banner][i]);
        break;
    case UNIT_ICON_ACTIVE:
    default:
        Set_Pic_Frame(unit_types[unit_type].map_figure_pic, 1);
        player = p_units[unit].owner;
        if (player == PLAYER_NEUTRAL)
            banner = BANNER_NEUTRAL;
        else
            banner = wizards[player].banner_color;
        Pic_Decode_Frame_To_Bitmap(unit_types[unit_type].map_figure_pic, unit_figure_pic);
        for (i = 0; i < 5; i++)
            Pic_Replace_Color(unit_figure_pic, i + CITY_MARKER_PAL_BASE, banner_shade_colors[banner][i]);
        break;
    }
    if (icon_mode == UNIT_ICON_ACTIVE || icon_mode == UNIT_ICON_INACTIVE || icon_mode == UNIT_ICON_MAP || icon_mode == UNIT_ICON_OTHER) {
        if (Unit_Is_Invisible(unit) == 1) {
            Pic_Recolor_Opaque(unit_figure_pic, COLOR_INVISIBLE);
            Pic_Outline_Recolor(unit_figure_pic, 1);
        }
        if (icon_mode != UNIT_ICON_OTHER && p_units[unit].order == ORDER_PATROL && Unit_Is_Invisible(unit) == 0 && player == current_player)
            Pic_Remap_Shade(unit_figure_pic, 1);
        Draw_Unit_Enchant_Outline(unit);
    } else {
    }
    Pic_Set_Remap_Flag(unit_figure_pic);
    Draw_Bitmap(x + 1, y + 1, unit_figure_pic);
    if (player == current_player) {
        if (icon_mode == UNIT_ICON_ACTIVE || icon_mode == UNIT_ICON_INACTIVE) {
            colors[0] = 10;
            colors[1] = 15;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(4);
            Font_Select_Shadow(0, 15, 0, 0);
            switch (p_units[unit].order) {
            case ORDER_BUILD_ROAD:
                Print_Text(x + 2, y + 2, "B");
                break;
            case ORDER_PURIFY:
                Print_Text(x + 2, y + 2, "P");
                break;
            case ORDER_GOTO:
                if (p_units[unit].road_turns != -1)
                    Print_Text(x + 2, y + 2, "B");
                else
                    Print_Text(x + 2, y + 2, "G");
                break;
            case ORDER_UNUSED_6:                /* no 1.31 code stores these orders */
            case ORDER_UNUSED_7:
                Print_Text(x + 4, y + 5, "C");
                break;
            }
        }
    }
}

/* (x, y, unit) the health bar (only when damaged: green / yellow / red), the level medals (white 1..3, gold 4..6,
   red 7+, one per level above the color's first) and the weapon-quality icon (magic / mithril / adamantium) */
void Draw_Unit_Badges(int x, int y, int unit)
{
    int color;
    unsigned unit_type;
    int lvl;
    int px;
    int py;
    int wx;
    int wy;
    int max_hp;
    int hp;
    int len;
    int k;
    int first;
    int weapon_grade;
    unsigned pic;

    unit_type = p_units[unit].type;
    max_hp = Unit_Max_Hits(unit) * unit_types[unit_type].figures;
    hp = max_hp - p_units[unit].damage;
    if (hp != max_hp && hp > 0) {
        len = (hp * 10) / max_hp - 1;
        if (len >= 6)
            color = HEALTH_BAR_GREEN;
        else if (len >= 3)
            color = HEALTH_BAR_YELLOW;
        else
            color = HEALTH_BAR_RED;
        Draw_Line(x + 5, y + 19, x + len + 5, y + 19, color);
    }
    if (p_units[unit].level > 0) {
        if (p_units[unit].level > 6) {
            pic = medal_red_pic;
            first = 6;
        } else if (p_units[unit].level > 3) {
            pic = medal_gold_pic;
            first = 3;
        } else {
            pic = medal_white_pic;
            first = 0;
        }
        k = 0;
        for (lvl = first; lvl < p_units[unit].level; lvl++) {
            px = x + k * 4 + 2;
            py = y + 21;
            Draw_Pic_Frame(px, py, pic);
            k++;
        }
    }
    weapon_grade = p_units[unit].mutation_flags & MUT_WEAPON_GRADE;
    if (weapon_grade > WEAPON_NORMAL) {
        wx = x + 13;
        wy = y + 19;
        switch (weapon_grade) {
        case WEAPON_MAGIC:
            pic = weapon_magic_pic;
            break;
        case WEAPON_MITHRIL:
            pic = weapon_mithril_pic;
            break;
        case WEAPON_ADAMANTIUM:
            pic = weapon_adamantium_pic;
            break;
        }
        Set_Pic_Frame(pic, weapon_badge_frame);
        Draw_Pic_Frame(wx, wy, pic);
    }
}

/* "Moves:" with the selected stack's half moves (drawn three times for the shadow) and its movement-type icons; also
   the Moves help area */
void Draw_Stack_Moves_Line(void)
{
    register int half_moves;

    help_entries[MAIN_HELP_MOVES].id = -1;
    if (stack_unit_count > 0) {
        half_moves = Stack_Selected_Moves();
        if (half_moves != 0) {
            help_entries[MAIN_HELP_MOVES].id = HELP_MOVES;
            Font_Select_Shadow(1, 1, 0, 0);
            Set_Text_Shadow_Color(0);
            Set_Font_Color(0);
            Print_Text(246, 167, "Moves:");
            Print_Half_Moves(275, 167, half_moves, 0);
            Font_Select(1, 1, 0, 0);
            Set_Text_Shadow_Color(0);
            Set_Font_Color(0);
            Print_Text(246, 168, "Moves:");
            Print_Half_Moves(275, 168, half_moves, 0);
            Font_Select(1, 0, 0, 0);
            Set_Font_Color(0);
            Print_Text(246, 167, "Moves:");
            Print_Half_Moves(275, 167, half_moves, 0);
            Draw_Move_Type_Icons(286, 167, -1);
        }
    }
}

/* (x, y, unit or -1 for the selected stack) -> 0 with nothing to show (no value otherwise): up to three movement-type icons, right-aligned in a 30-pixel field:
   planar travel (from the selected stack, Stack_Can_Planar_Travel, even for a single unit), then flying, else sailing,
   else forester + mountaineer as one icon (5) or each, and swimming; icon 8 when none
   Numbered: BUG #380 (BUGS_AND_QUIRKS.txt) */
int Draw_Move_Type_Icons(int x, int y, int unit)
{
    int ix;
    int one;
    int modes[9];
    int icons[9];
    register int count;
    register int i;

    if (stack_unit_count < 1 && unit == -1)
        return 0;
    for (i = 0; i < 9; i++)
        modes[i] = 0;
    if (unit == -1)
        Stack_Active_Movement_Modes(modes);
    else {
        one = unit;
        Stack_Movement_Modes(modes, &one, 1);
    }
    if (Stack_Can_Planar_Travel() == 1)
        modes[MOVE_MODE_PLANAR_TRAVEL] = 1;
    count = 0;
    if (modes[MOVE_MODE_PLANAR_TRAVEL] == 1) {
        icons[count] = MOVE_ICON_ASTRAL;
        count++;
    }
    if (modes[MOVE_MODE_FLYING] == 1) {
        icons[count] = MOVE_ICON_FLY;
        count++;
    } else if (modes[MOVE_MODE_SAILING] == 1) {
        icons[count] = MOVE_ICON_SAIL;
        count++;
    } else {
        if (modes[MOVE_MODE_FORESTER] == 1 && modes[MOVE_MODE_MOUNTAINEER] == 1) {
            icons[count] = MOVE_ICON_PATH;
            count++;
        } else {
            if (modes[MOVE_MODE_FORESTER] == 1) {
                icons[count] = MOVE_ICON_FOREST;
                count++;
            }
            if (modes[MOVE_MODE_MOUNTAINEER] == 1) {
                icons[count] = MOVE_ICON_MOUNTAIN;
                count++;
            }
        }
        if (modes[MOVE_MODE_SWIMMING] == 1) {
            icons[count] = MOVE_ICON_SWIM;
            count++;
        }
    }
    if (count == 0) {
        icons[count] = MOVE_ICON_BOOT;
        count++;
    }
    x += (3 - count) * 10;
    for (i = 0; i < count; i++) {
        ix = x + i * 10;
        Draw_Pic_Frame(ix, y, move_type_icons[icons[i]]);
    }
}

/* () 1 when every active unit of the selected stack (all of them when none is active) has Planar Travel (the
   enchantment, the type ability or an item) or the stack stands in an Astral Gate city */
int Stack_Can_Planar_Travel(void)
{
    int unused;
    int unit;
    int in_gate;
    int can;
    register int none_active;
    register int i;

    none_active = 1;
    for (i = 0; i < stack_unit_count && none_active == 1; i++)
        if (stack_units[i].active != 0)
            none_active = 0;
    if (none_active == 1)
        for (i = 0; i < stack_unit_count; i++)
            stack_units[i].active = 1;
    can = 0;
    in_gate = Stack_In_Astral_Gate_City();
    unused = 0;
    for (i = 0; i < stack_unit_count; i++) {
        unit = stack_units[i].unit;
        if (stack_units[i].active == 1) {
            if ((p_units[unit].enchantments & UNIT_ENCH_PLANAR_TRAVEL) || (unit_types[p_units[unit].type].ability_flags & AFLAG_PLANE_SHIFT) ||
                Hero_Item_Planar_Travel(unit) == 1 || in_gate != 0)
                can = 1;
            else {
                can = 0;
                break;
            }
        }
    }
    if (none_active == 1)
        for (i = 0; i < stack_unit_count; i++)
            stack_units[i].active = 0;
    return can;
}

/* () 1 when the selected stack's first unit stands on a city with the human's Astral Gate (enchants == 1: the
   caster player + 1) - looked up on Arcanus first, then Myrror,
   at the same x / y whatever the unit's plane */
int Stack_In_Astral_Gate_City(void)
{
    int found;
    register int unit;
    register int city;

    found = 0;
    unit = stack_units[0].unit;
    city = City_At(p_units[unit].x, p_units[unit].y, 0);
    if (city == -1)
        city = City_At(p_units[unit].x, p_units[unit].y, 1);
    if (city != -1 && p_cities[city].enchants[CITY_ENCH_ASTRAL_GATE] == 1)
        found = 1;
    return found;
}

/* () the fewest moves left among the selected stack's active units (0 for none), also into stack_moves_cached; a
   transport gives its own moves; BUG #208: a Wind Walker sets its moves but the loop keeps taking the minimum
   Numbered: BUG #208 (BUGS_AND_QUIRKS.txt) */
int Stack_Selected_Moves(void)
{
    int slot;
    int wind_walker;                            /* written, never read here (as wind_walker_unit) */
    int wind_walker_unit;
    register int moves;
    register int unit;

    moves = 1000;
    wind_walker = 0;
    for (slot = 0; slot < stack_unit_count; slot++) {
        unit = stack_units[slot].unit;
        if (stack_units[slot].active == 1) {
            if (p_units[unit].moves_left < moves)
                moves = p_units[unit].moves_left;
            if (unit_types[p_units[unit].type].transport_capacity > 0) {
                moves = p_units[unit].moves_left;
                break;
            }
            if (Unit_Has_Wind_Walking(unit) == 1) {
                wind_walker = 1;
                wind_walker_unit = unit;
                moves = p_units[unit].moves_left;
            }
        }
    }
    if (moves == 1000)
        moves = 0;
    stack_moves_cached = moves;
    return stack_moves_cached;
}

/* () the same over the whole stack; a Wind Walker with moves left gives each 0-move unit 1 move (not finished)
   Numbered: BUG #208 (BUGS_AND_QUIRKS.txt) */
int Stack_Moves_WindWalk_Carry(void)
{
    int moves;
    int wind_walker;
    int wind_walker_unit;
    register int unit;
    register int i;

    moves = 1000;
    wind_walker = 0;
    for (i = 0; i < stack_unit_count; i++) {
        unit = stack_units[i].unit;
        if (p_units[unit].moves_left < moves)
            moves = p_units[unit].moves_left;
        if (unit_types[p_units[unit].type].transport_capacity > 0) {
            moves = p_units[unit].moves_left;
            break;
        }
        if (Unit_Has_Wind_Walking(unit) == 1) {
            wind_walker = 1;
            moves = p_units[unit].moves_left;
            wind_walker_unit = unit;
        }
    }
    if (wind_walker == 1 && p_units[wind_walker_unit].moves_left > 0) {
        for (i = 0; i < stack_unit_count; i++) {
            unit = stack_units[i].unit;
            if (p_units[unit].moves_left < 1 && Unit_Has_Wind_Walking(unit) != 1) {
                p_units[unit].moves_left = 1;
                p_units[unit].finished = 0;
            }
        }
    }
    if (moves == 1000)
        moves = 0;
    stack_moves_cached = moves;
    return stack_moves_cached;
}

/* (x, y, plane) the city at the square, or -1 */
int City_At(int x, int y, int plane)
{
    register int found;
    register int city;

    found = -1;
    for (city = 0; city < num_cities && found == -1; city++)
        if (p_cities[city].x == x && p_cities[city].y == y && p_cities[city].plane == plane)
            found = city;
    return found;
}

/* () 1 when any wizard has Planar Seal in effect */
int Planar_Seal_Active(void)
{
    register int found;
    register int player;

    found = 0;
    for (player = 0; player < num_players && found == 0; player++)
        if (wizards[player].globals.planar_seal > 0)
            found = 1;
    return found;
}
