/* ARMYLIST.C - WIZARDS.EXE overlay code segment 585e (stub 2447): the Armies screen (main-loop screen 2): the player's stacks
   six rows at a time (scroll arrows / U / D), the six hired heroes' portraits, the upkeep totals, the hovered
   unit's name, enchantments and position on the minimap; a click on a unit or a portrait goes to its stack on the
   map, a right click opens the unit's statistics; Items (I) opens the items screen, OK (O / Esc) returns.
   No <string.h> here: strlen's value is divided as a signed int (implicit declarations, as in COMBDRAW.C).
   This module's data (DS:3244): its literals; BSS (DS:c0ba..c11c, with the cities list's c0be and the shared
   list_first_row / list_visible_rows).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 585e src/wizards/ARMYLIST.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Screen_Armies(void);
void Armies_Redraw(void);
void ArmyList_Add_Controls(void);
void ArmyList_Unit_Rect(int list_pos, int *x1, int *y1, int *x2, int *y2);
void ArmyList_Draw_Minimap(void);
int ArmyList_Scroll_To_Unit(int unit);
void ArmyList_Build_Stacks(void);
void ArmyList_Collect_Units(void);
void ArmyList_Calc_Visible_Rows(void);
void Armies_Load(void);
void ArmyList_Print_Hyphenated(int x, int y, int width, int line_dy, unsigned align, char *text);
void ArmyList_Hero_Rect(int slot, int *x1, int *y1, int *x2, int *y2);

#include "GAME.H"

int armylist_unused_c11a;                       /* DS:c11a  never referenced (inside this module's BSS block) */
char *p_armylist_stack_x;                       /* DS:c118  near char[1000]: x of each stack */
char *p_armylist_stack_y;                       /* DS:c116  near char[1000]: y of each stack */
char *p_armylist_row_counts;                    /* DS:c114  near char[54] (6 used): units in each visible row */
char *p_armylist_stack_plane;                   /* DS:c112  near char[1000]: plane of each stack (its first unit's) */
int *p_armylist_units;                          /* DS:c110  near int[54]: the visible rows' units, row by row */
char *armylist_ench_names[20];                  /* DS:c0e8  20-byte strings: the hovered unit's enchantments */
int armylist_gold_upkeep;                       /* DS:c0e6 */
int armylist_mana_upkeep;                       /* DS:c0e4 */
int armylist_food_upkeep;                       /* DS:c0e2 */
int armylist_x0;                                /* DS:c0e0  77: x of the first unit figure (figures 18 apart) */
int armylist_y0;                                /* DS:c0de  24: y of the first row (rows 22 apart) */
int *p_armylist_unit_ctls;                      /* DS:c0dc  near int[54]: the control of every visible unit */
int *p_armylist_hero_ctls;                      /* DS:c0da  near int[6]: the portraits' controls; -1000 for an empty slot */
int armylist_hero_ctl_count;                    /* DS:c0d8  always 6 */
int *p_armylist_hero_units;                     /* DS:c0d6  near int[6]: each portrait's unit; -1 for an empty slot */
int armylist_hero_x0;                           /* DS:c0d4  12: x of the left portrait column (the right one +265) */
int armylist_hero_y0;                           /* DS:c0d2  5: y of the first portrait row (rows 51 apart) */
int armylist_unused_c0d0;                       /* DS:c0d0  set to 5, never read */
int armylist_unused_c0ce;                       /* DS:c0ce  set to 168, never read */
int armylist_unit_ctl_count;                    /* DS:c0cc */
int armylist_unit_total;                        /* DS:c0ca  the player's units (written, never read) */
int list_first_row;                             /* DS:c0c8  the first row shown (also the cities list's, CITYLIST.C) */
int list_visible_rows;                          /* DS:c0c6  the rows shown (also the cities list's) */
int armylist_stack_count;                       /* DS:c0c4  the player's stacks */
int *p_armylist_row_ctls;                       /* DS:c0c2  near int[6]: whole-row controls, never tested */
int armylist_row_ctl_count;                     /* DS:c0c0 */
int citylist_unused_c0be;                       /* DS:c0be  (used by CITYLIST.C) set to 0, never read */
int armylist_unused_c0bc;                       /* DS:c0bc  never referenced (inside this module's BSS block) */
int armylist_highlight;                         /* DS:c0ba  the hovered unit (index into p_armylist_units) */

extern struct RaceInfo race_table[14];          /* DS:1d86 */
extern char *arcane_first_realm_names[6];                    /* DS:1ed8  the names of "races" 15..20 (the realms of summoned units) */
extern int no_units_to_move;                    /* DS:9444 */
extern unsigned armylist_ok_btn_pic;            /* DS:9570 */
extern unsigned armylist_items_btn_pic;         /* DS:9572 */
extern unsigned armylist_down_arrow_pic;        /* DS:9574 */
extern unsigned armylist_up_arrow_pic;          /* DS:9576 */
extern unsigned armylist_bg_pic;                /* DS:9578 */
extern unsigned items_hero_portraits[6];        /* DS:95f4 */
extern int stack_unit_count;                    /* DS:999c */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern int award_item;                          /* DS:c270 */

void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
int Wizard_Gold_Upkeep(int player);
int Wizard_Mana_Upkeep(int player);
int Wizard_Food_Upkeep(int player);
void Pic_Apply_Palette(unsigned pic);
void Remap_Build_Range(int first, int last);
void UI_Clear_Help(void);
void Help_Load_Armies(void);
void Remap_Build_Shadow(void);
void Reset_Clip_Rect(void);
void UI_Clear_Controls(void);
void Timer_Mark(void);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
int UI_Add_Hotkey(char *key);
int UI_Get_Input(void);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
int Player_Stacks_Changed_Nop(int player);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2);
void Font_Select_Palette(int font, char *colors);
int Print_Text_Centered(int x, int y, char *text);
void Set_Text_Shadow_Color(int color);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Number_Centered(int x, int y, int value);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Line_Spacing(int line_gap);
void Set_Space_Width(int width);
void Fill_Box_Styled();                           /* no prototype here: called with 6 arguments (the definition in GRAPHICS.C takes 9, read by style) */
void Draw_Unit_Figure(int x, int y, int unit, int icon_mode);
void Outline_Cycle_Tick(void);
void Str_To_Upper(char *str);
void Unit_Enchant_Name_List(int unit, char **out, int *n);
int Print_Text(int x, int y, char *text);
void Minimap_Draw_Blink(int px, int py, int w, int h, int plane, int cx, int cy);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
int Paragraph_Used_Width(int width, char *text, unsigned align);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);

/* () the Armies screen until OK / Esc (back to the map, reselecting the stack that was selected on entry) or
   Items (the items screen, 9); a click on a unit or a hero portrait selects its stack and centres the map on it;
   a right click shows the unit's statistics (zoomed from its figure) and rebuilds the lists; hovering a unit
   highlights it, hovering a portrait scrolls the list to the hero */
void Screen_Armies(void)
{
    int done;
    int hit_test;
    int up_right;
    int down_right;
    int up_left;
    int down_left;
    int key_u;
    int key_d;
    int ok_button;
    int items_button;
    int redraw;
    int x1;
    int y1;
    int x2;
    int y2;
    int no_stack;
    int key_esc;
    register int i;
    register int input;

    Armies_Load();
    armylist_x0 = 77;
    armylist_y0 = 24;
    armylist_hero_x0 = 12;
    armylist_hero_y0 = 5;
    armylist_unused_c0d0 = 5;
    armylist_unused_c0ce = 168;
    ArmyList_Build_Stacks();
    list_first_row = 0;
    ArmyList_Collect_Units();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Armies_Redraw, 1);
    UI_Input_Delay(1);
    no_stack = 0;
    if (stack_unit_count == 0)
        no_stack = 1;
    ArmyList_Draw_Minimap();
    armylist_gold_upkeep = Wizard_Gold_Upkeep(current_player);
    armylist_mana_upkeep = Wizard_Mana_Upkeep(current_player);
    armylist_food_upkeep = Wizard_Food_Upkeep(current_player);
    Pic_Apply_Palette(armylist_up_arrow_pic);
    Palette_Upload();
    Palette_Unmark_Range(0, 243);
    Palette_Mark_Range(0, 255);
    Remap_Build_Range(0, 1);
    UI_Clear_Help();
    Help_Load_Armies();
    armylist_highlight = 0;
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        ArmyList_Add_Controls();
        items_button = UI_Add_Button(273, 163, "", armylist_items_btn_pic, "I", -1);
        ok_button = UI_Add_Button(273, 182, "", armylist_ok_btn_pic, "O", -1);
        key_esc = UI_Add_Hotkey("\x1b");
        if (list_first_row == 0) {
            key_u = UI_NO_CONTROL;
            up_right = UI_NO_CONTROL;
            up_left = UI_NO_CONTROL;
        } else {
            key_u = UI_Add_Hotkey("U");
            up_right = UI_Add_Button(250, 26, "", armylist_up_arrow_pic, "", -1);
            up_left = UI_Add_Button(60, 26, "", armylist_up_arrow_pic, "", -1);
        }
        if (list_first_row + ARMYLIST_ROWS == armylist_stack_count || list_visible_rows == armylist_stack_count) {
            key_d = UI_NO_CONTROL;
            down_right = UI_NO_CONTROL;
            down_left = UI_NO_CONTROL;
        } else {
            key_d = UI_Add_Hotkey("D");
            down_right = UI_Add_Button(250, 139, "", armylist_down_arrow_pic, "", -1);
            down_left = UI_Add_Button(60, 139, "", armylist_down_arrow_pic, "", -1);
        }
        input = UI_Get_Input();
        hit_test = UI_Hit_Test();
        if (input == ok_button || input == key_esc) {
            Play_Click_Sound();
            done = 1;
            current_screen = SCREEN_MAP;
            Player_Stacks_Changed_Nop(current_player);
            if (no_stack == 1)
                stack_unit_count = 0;
            else
                Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, p_units[top_unit_in_stack].x,
                                p_units[top_unit_in_stack].y);
        }
        if (input == items_button) {
            Play_Click_Sound();
            done = 1;
            award_item = -1;
            current_screen = SCREEN_ITEMS;
        }
        if (input == up_right || input == up_left || input == key_u) {
            Play_Click_Sound();
            if (list_first_row != 0) {
                list_first_row--;
                ArmyList_Collect_Units();
                redraw = 1;
            }
        }
        if (input == down_right || input == down_left || input == key_d) {
            Play_Click_Sound();
            if (list_first_row + ARMYLIST_ROWS < armylist_stack_count) {
                list_first_row++;
                ArmyList_Collect_Units();
                redraw = 1;
            }
        }
        for (i = 0; i < armylist_unit_ctl_count; i++) {
            if (p_armylist_unit_ctls[i] == input) {
                Play_Click_Sound();
                top_unit_in_stack = p_armylist_units[i];
                map_view_plane = p_units[top_unit_in_stack].plane;
                Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, p_units[top_unit_in_stack].x,
                                p_units[top_unit_in_stack].y);
                if (no_units_to_move == 1)
                    no_units_to_move = 0;
                next_unit_ref_x = p_units[top_unit_in_stack].x;
                next_unit_ref_y = p_units[top_unit_in_stack].y;
                done = 1;
                current_screen = SCREEN_MAP;
                break;
            }
        }
        for (i = 0; i < armylist_unit_ctl_count; i++) {
            if (-p_armylist_unit_ctls[i] == input) {
                UI_Clear_Help();
                ArmyList_Unit_Rect(i, &x1, &y1, &x2, &y2);
                Unit_Stats_Popup(p_armylist_units[i], x1, y1, x2, y2);
                UI_Set_Redraw_Fn(Armies_Redraw, 1);
                Armies_Load();
                ArmyList_Build_Stacks();
                ArmyList_Collect_Units();
                ArmyList_Draw_Minimap();
                UI_Input_Delay(1);
                armylist_gold_upkeep = Wizard_Gold_Upkeep(current_player);
                armylist_mana_upkeep = Wizard_Mana_Upkeep(current_player);
                armylist_food_upkeep = Wizard_Food_Upkeep(current_player);
                UI_Clear_Help();
                Help_Load_Armies();
                redraw = 1;
                ArmyList_Calc_Visible_Rows();
                break;
            }
        }
        for (i = 0; i < armylist_unit_ctl_count; i++) {
            if (p_armylist_unit_ctls[i] == hit_test) {
                armylist_highlight = i;
                break;
            }
        }
        for (i = 0; i < armylist_hero_ctl_count; i++) {
            if (p_armylist_hero_ctls[i] == hit_test) {
                armylist_highlight = ArmyList_Scroll_To_Unit(p_armylist_hero_units[i]);
                break;
            }
        }
        for (i = 0; i < armylist_hero_ctl_count; i++) {
            if (p_armylist_hero_ctls[i] == input) {
                Play_Click_Sound();
                top_unit_in_stack = wizards[current_player].hired_heroes[i].unit_index;
                map_view_plane = p_units[top_unit_in_stack].plane;
                Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, p_units[top_unit_in_stack].x,
                                p_units[top_unit_in_stack].y);
                if (no_units_to_move == 1)
                    no_units_to_move = 0;
                done = 1;
                current_screen = SCREEN_MAP;
                break;
            }
        }
        for (i = 0; i < armylist_hero_ctl_count; i++) {
            if (-p_armylist_hero_ctls[i] == input) {
                UI_Clear_Help();
                ArmyList_Hero_Rect(i, &x1, &y1, &x2, &y2);
                Unit_Stats_Popup(p_armylist_hero_units[i], x1, y1, x2, y2);
                UI_Set_Redraw_Fn(Armies_Redraw, 1);
                Armies_Load();
                ArmyList_Build_Stacks();
                ArmyList_Collect_Units();
                ArmyList_Draw_Minimap();
                armylist_gold_upkeep = Wizard_Gold_Upkeep(current_player);
                armylist_mana_upkeep = Wizard_Mana_Upkeep(current_player);
                armylist_food_upkeep = Wizard_Food_Upkeep(current_player);
                UI_Clear_Help();
                Help_Load_Armies();
                ArmyList_Calc_Visible_Rows();
                redraw = 1;
                break;
            }
        }
        if (done == 0 && redraw == 0) {         /* redraw is uninitialised on the first pass (original) */
            Armies_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
        redraw = 0;
    }
    Palette_Unmark_Range(0, 255);
    Palette_Mark_Range(0, 223);
    Remap_Build_Shadow();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
    UI_Clear_Controls();
}

/* () the screen: the title "The Armies Of <name>" in layered shades, the upkeep totals, the scroll arrows (frame 1
   = greyed: up at the top, down at the end), Items / OK, the hero portraits with their names (an empty slot's
   help is EMPTY HERO SLOT), the unit figures (a gradient box behind the hovered one), and for the hovered unit its
   name in capitals - the hero's name, "<race> <type>" for a type with ABILITY_RACE_PREFIX, else the type - up to 4
   enchantments and the minimap */
void Armies_Redraw(void)
{
    int k;
    int x;
    int y;
    int x2;
    int y2;
    int n;
    int unit_type;
    int n_ench;
    int unit;
    int hero_unit;
    int race;
    char colors[6];
    char shades[6];
    register int i;
    register int t;

    ArmyList_Calc_Visible_Rows();
    Set_Draw_Page();
    Screen_Restore_A800();
    strcpy(p_text_buf, "The Armies Of ");
    strcat(p_text_buf, wizards[current_player].wizard_name);
    for (i = 0; i < 5; i++)
        shades[i] = 237;
    Font_Select_Palette(4, shades);
    Print_Text_Centered(162, 9, p_text_buf);
    Print_Text_Centered(161, 9, p_text_buf);
    Print_Text_Centered(162, 8, p_text_buf);
    for (i = 0; i < 5; i++)
        shades[i] = 238;
    Font_Select_Palette(4, shades);
    Print_Text_Centered(161, 7, p_text_buf);
    Print_Text_Centered(160, 8, p_text_buf);
    for (i = 0; i < 5; i++)
        shades[i] = 239;
    Font_Select_Palette(4, shades);
    Print_Text_Centered(161, 8, p_text_buf);
    colors[0] = 236;
    colors[1] = 177;
    colors[2] = 178;
    colors[3] = 177;
    colors[4] = 176;
    Font_Select_Palette(4, colors);
    Print_Text_Centered(160, 7, p_text_buf);
    Set_Text_Shadow_Color(231);
    colors[0] = 236;
    colors[1] = 129;
    colors[2] = 129;
    Font_Select_Palette(1, colors);
    Font_Select_Shadow(1, 15, 0, 0);
    Print_Number_Centered(45, 173, armylist_gold_upkeep);
    Print_Number_Centered(45, 183, armylist_mana_upkeep);
    Print_Number_Centered(45, 192, armylist_food_upkeep);
    Print_Text_Centered(30, 162, "UPKEEP");
    if (list_first_row == 0 && armylist_stack_count > ARMYLIST_ROWS) {
        Set_Pic_Frame(armylist_up_arrow_pic, 1);
        Draw_Pic_Frame(250, 26, armylist_up_arrow_pic);
        Set_Pic_Frame(armylist_up_arrow_pic, 1);
        Draw_Pic_Frame(60, 26, armylist_up_arrow_pic);
    } else {
        Set_Pic_Frame(armylist_up_arrow_pic, 0);
        Draw_Pic_Frame(250, 26, armylist_up_arrow_pic);
        Set_Pic_Frame(armylist_up_arrow_pic, 0);
        Draw_Pic_Frame(60, 26, armylist_up_arrow_pic);
    }
    if (armylist_stack_count > ARMYLIST_ROWS
        && (list_first_row + ARMYLIST_ROWS == armylist_stack_count || list_visible_rows == armylist_stack_count)) {
        Set_Pic_Frame(armylist_down_arrow_pic, 1);
        Draw_Pic_Frame(250, 139, armylist_down_arrow_pic);
        Set_Pic_Frame(armylist_down_arrow_pic, 1);
        Draw_Pic_Frame(60, 139, armylist_down_arrow_pic);
    } else {
        Set_Pic_Frame(armylist_down_arrow_pic, 0);
        Draw_Pic_Frame(250, 139, armylist_down_arrow_pic);
        Set_Pic_Frame(armylist_down_arrow_pic, 0);
        Draw_Pic_Frame(60, 139, armylist_down_arrow_pic);
    }
    Draw_Pic_Frame(273, 163, armylist_items_btn_pic);
    Draw_Pic_Frame(273, 182, armylist_ok_btn_pic);
    if (armylist_stack_count <= ARMYLIST_ROWS) {   /* help slots 12 .. 15: the scroll arrows; i + 6: the portraits */
        help_entries[12].id = -1;
        help_entries[13].id = -1;
        help_entries[14].id = -1;
        help_entries[15].id = -1;
    }
    unit = p_armylist_units[armylist_highlight];
    Set_Text_Shadow_Color(240);
    for (i = 0; i < 6; i++) {
        hero_unit = wizards[current_player].hired_heroes[i].unit_index;
        if (hero_unit > -1 && p_units[hero_unit].owner != PLAYER_NONE) {
            ArmyList_Hero_Rect(i, &x, &y, &x2, &y2);
            Draw_Pic_Frame(x, y, items_hero_portraits[i]);
            colors[0] = 236;
            colors[1] = 129;
            colors[2] = 129;
            Font_Select_Palette(0, colors);
            Font_Select_Shadow(0, 15, 0, 0);
            Set_Line_Spacing(0);
            Set_Space_Width(1);
            Print_Text_Centered(x + 15, y + 37, wizards[current_player].hired_heroes[i].name);
            help_entries[i + 6].id = -1;
        } else
            help_entries[i + 6].id = HELP_EMPTY_HERO_SLOT;
    }
    Set_Text_Shadow_Color(231);
    n = 0;
    for (i = 0; i < list_visible_rows; i++) {
        for (k = 0; k < p_armylist_row_counts[i]; k++) {
            x = armylist_x0 + k * 18;
            y = armylist_y0 + i * 22;
            if (n == armylist_highlight)
                Fill_Box_Styled(x + 1, y + 2, x + 19, y + 16, FILL_REMAP, 1);
            Draw_Unit_Figure(x, y, p_armylist_units[n], 1);
            n++;
        }
    }
    Outline_Cycle_Tick();
    if (armylist_stack_count > 0) {
        unit_type = p_units[unit].type;
        t = p_units[unit].hero_slot;
        if (t != -1)
            strcpy(p_text_buf, wizards[current_player].hired_heroes[p_units[unit].hero_slot].name);
        else if (unit_types[unit_type].abilities & ABILITY_RACE_PREFIX) {
            race = unit_types[unit_type].race;
            if (race >= RACE_ARCANE_CREATURE)
                strcpy(p_text_buf, arcane_first_realm_names[race - RACE_ARCANE_CREATURE]);
            else
                strcpy(p_text_buf, race_table[race].name);
            strcat(p_text_buf, " ");
            strcat(p_text_buf, unit_types[unit_type].name);
        } else
            strcpy(p_text_buf, unit_types[unit_type].name);
        Set_Text_Shadow_Color(231);
        colors[0] = 236;
        colors[1] = 129;
        colors[2] = 129;
        Font_Select_Palette(1, colors);
        Font_Select_Shadow(1, 15, 0, 0);
        Str_To_Upper(p_text_buf);
        Print_Text_Centered(188, 161, p_text_buf);
        Unit_Enchant_Name_List(unit, armylist_ench_names, &n_ench);
        if (n_ench > 0) {
            Font_Select_Shadow(1, 15, 0, 0);
            t = n_ench;
            if (t > 4)
                t = 4;
            for (i = 0; i < t; i++) {
                if (t < 4)
                    Print_Text(144, i * 7 + 172, armylist_ench_names[i]);
                else
                    Print_Text(144, i * 6 + 172, armylist_ench_names[i]);
            }
        }
        Set_Text_Shadow_Color(0);
        ArmyList_Draw_Minimap();
    }
}

/* () per frame: a control for every visible unit figure, one for every visible row (never tested), and one for
   every hired hero's portrait (-1000 / -1 for an empty or dead slot) */
void ArmyList_Add_Controls(void)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int hero_unit;
    register int i;
    register int k;

    armylist_unit_ctl_count = 0;
    ArmyList_Calc_Visible_Rows();
    for (i = 0; i < list_visible_rows; i++) {
        for (k = 0; k < p_armylist_row_counts[i]; k++) {
            x1 = armylist_x0 + k * 18 + 1;
            y1 = armylist_y0 + i * 22 + 1;
            x2 = x1 + 18;
            y2 = y1 + 15;
            p_armylist_unit_ctls[armylist_unit_ctl_count] = UI_Add_Region(x1, y1, x2, y2, "", -1);
            armylist_unit_ctl_count++;
        }
    }
    armylist_row_ctl_count = 0;
    for (i = 0; i < list_visible_rows; i++) {
        x1 = armylist_x0 + 1;
        y1 = armylist_y0 + i * 22 + 1;
        x2 = x1 + 162;
        y2 = y1 + 15;
        p_armylist_row_ctls[armylist_row_ctl_count] = UI_Add_Region(x1, y1, x2, y2, "", -1);
        armylist_row_ctl_count++;
    }
    armylist_hero_ctl_count = 6;
    for (i = 0; i < 6; i++) {
        hero_unit = wizards[current_player].hired_heroes[i].unit_index;
        if (hero_unit > -1 && p_units[hero_unit].owner != PLAYER_NONE) {
            ArmyList_Hero_Rect(i, &x1, &y1, &x2, &y2);
            p_armylist_hero_ctls[i] = UI_Add_Region(x1, y1, x2, y2, "", -1);
            p_armylist_hero_units[i] = wizards[current_player].hired_heroes[i].unit_index;
        } else {
            p_armylist_hero_ctls[i] = UI_NO_CONTROL;
            p_armylist_hero_units[i] = -1;
        }
    }
}

/* (list_pos, &x1, &y1, &x2, &y2) the screen box of the list_pos-th visible unit figure (walking the row counts): the
   statistics popup zooms from it */
void ArmyList_Unit_Rect(int list_pos, int *x1, int *y1, int *x2, int *y2)
{
    int done;
    int col;
    int row;
    int sum;

    row = 0;
    sum = p_armylist_row_counts[row];
    done = 0;
    while (done == 0) {
        if (sum > list_pos) {
            sum = sum - p_armylist_row_counts[row];
            col = list_pos - sum;
            sum = col;
            *x1 = armylist_x0 + sum * 18 - 1;
            *y1 = armylist_y0 + row * 22 - 1;
            *x2 = *x1 + 18;
            *y2 = *y1 + 16;
            done = 1;
        } else {
            row++;
            sum += p_armylist_row_counts[row];
        }
    }
}

/* () the minimap (86, 164, 49 x 33 squares) centred on the hovered unit, with its blinking dot
   Numbered: BUG #101 (BUGS_AND_QUIRKS.txt) */
void ArmyList_Draw_Minimap(void)
{
    int x;
    int y;
    int plane;

    x = p_units[p_armylist_units[armylist_highlight]].x;
    y = p_units[p_armylist_units[armylist_highlight]].y;
    plane = p_units[p_armylist_units[armylist_highlight]].plane;
    Minimap_Draw_Blink(86, 164, 49, 33, plane, x, y);
}

/* (unit) -> its index among the visible units: when it is not visible the list restarts at the first row and
   scrolls one row per try. BUG #99: no upper bound, and it searches the previous frame's control count
   Numbered: BUG #99 (BUGS_AND_QUIRKS.txt) */
int ArmyList_Scroll_To_Unit(int unit)
{
    int result;
    int tried;
    register int i;
    register int found;

    tried = 0;
    found = 0;
    while (found == 0) {
        for (i = 0; i < armylist_unit_ctl_count; i++) {
            if (p_armylist_units[i] == unit) {
                result = i;
                found = 1;
                break;
            }
        }
        if (found == 0) {
            if (tried == 0) {
                list_first_row = 0;
                tried = 1;
            } else
                list_first_row++;
            ArmyList_Collect_Units();
        }
    }
    return result;
}

/* () one stack per distinct square (x, y, plane) of the player's units, in unit-table order; a unit in a tower
   joins a stack on the same x, y on either plane; also counts the player's units
   Numbered: BUG #99 (BUGS_AND_QUIRKS.txt) */
void ArmyList_Build_Stacks(void)
{
    int unit;
    int i;
    int found;

    armylist_unit_total = 0;
    armylist_stack_count = 0;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner == current_player) {
            armylist_unit_total++;
            found = -1;
            for (i = 0; i < armylist_stack_count; i++) {
                if (p_units[unit].x == p_armylist_stack_x[i] && p_units[unit].y == p_armylist_stack_y[i]
                    && (p_units[unit].plane == p_armylist_stack_plane[i] || p_units[unit].in_tower == 1)) {
                    found = 1;
                    break;
                }
            }
            if (found == -1) {
                p_armylist_stack_x[armylist_stack_count] = p_units[unit].x;
                p_armylist_stack_y[armylist_stack_count] = p_units[unit].y;
                p_armylist_stack_plane[armylist_stack_count] = p_units[unit].plane;
                armylist_stack_count++;
            }
        }
    }
}

/* () the units of the visible rows (stacks list_first_row ..), row by row, and each row's count. BUG #99: units match
   on the exact plane, so a tower unit on the other plane - joined to the stack by ArmyList_Build_Stacks - is left out */
void ArmyList_Collect_Units(void)
{
    int n;
    register int row;
    register int unit;

    ArmyList_Calc_Visible_Rows();
    n = 0;
    for (row = 0; row < list_visible_rows; row++) {
        p_armylist_row_counts[row] = 0;
        for (unit = 0; unit < num_units; unit++) {
            if (p_units[unit].owner == current_player && p_units[unit].x == p_armylist_stack_x[row + list_first_row]
                && p_units[unit].y == p_armylist_stack_y[row + list_first_row]
                && p_units[unit].plane == p_armylist_stack_plane[row + list_first_row]) {
                p_armylist_units[n] = unit;
                n++;
                p_armylist_row_counts[row] = p_armylist_row_counts[row] + 1;
            }
        }
    }
}

/* () list_visible_rows = min(ARMYLIST_ROWS, armylist_stack_count) (list_first_row >= 0, so its term never
   matters)
   Numbered: BUG #101 (BUGS_AND_QUIRKS.txt) */
void ArmyList_Calc_Visible_Rows(void)
{
    list_visible_rows = list_first_row + ARMYLIST_ROWS;
    if (list_visible_rows > ARMYLIST_ROWS)
        list_visible_rows = ARMYLIST_ROWS;
    if (list_visible_rows > armylist_stack_count)
        list_visible_rows = armylist_stack_count;
}

/* () the screen's buffers, the ARMYLIST background (drawn and copied to the back page), arrows and buttons, the
   hired heroes' PORTRAIT pictures and the minimap's picture */
void Armies_Load(void)
{
    register int i;
    register int hero_unit;

    p_armylist_stack_x = Near_Alloc_Reset(1000);
    p_armylist_stack_y = Near_Alloc(1000);
    p_armylist_stack_plane = Near_Alloc(1000);
    p_armylist_row_counts = Near_Alloc(54);     /* 6 rows x 9 units */
    p_armylist_units = (int *)Near_Alloc(108);  /* the same as ints */
    p_text_buf = Near_Alloc(100);
    for (i = 0; i < 20; i++)
        armylist_ench_names[i] = Near_Alloc(20);
    p_armylist_unit_ctls = (int *)Near_Alloc(108);
    p_armylist_hero_ctls = (int *)Near_Alloc(12);
    p_armylist_hero_units = (int *)Near_Alloc(12);
    p_armylist_row_ctls = (int *)Near_Alloc(12);
    Set_Draw_Page();
    armylist_bg_pic = LBX_Reload("ARMYLIST", 0, p_scratch);
    Draw_Pic_Frame(0, 0, armylist_bg_pic);
    Screen_Save_A800();
    armylist_up_arrow_pic = LBX_Reload("ARMYLIST", 1, p_scratch);
    armylist_down_arrow_pic = LBX_Reload_Next("ARMYLIST", 2, p_scratch);
    armylist_items_btn_pic = LBX_Reload_Next("ARMYLIST", 3, p_scratch);
    armylist_ok_btn_pic = LBX_Reload_Next("ARMYLIST", 4, p_scratch);
    for (i = 0; i < 6; i++) {
        hero_unit = wizards[current_player].hired_heroes[i].unit_index;
        if (hero_unit > -1 && p_units[hero_unit].owner != PLAYER_NONE)
            items_hero_portraits[i] = LBX_Reload_Next("PORTRAIT", unit_types[p_units[hero_unit].type].hero_portrait,
                                                      p_scratch);
    }
    p_minimap_pic = Heap_Alloc_Back(p_scratch, MINIMAP_PIC_PARAS_SMALL);
}

/* (x, y, width, line_dy, align, text) prints text as a paragraph; a word too wide for the width is split in half
   with a hyphen, the second half line_dy lower */
void ArmyList_Print_Hyphenated(int x, int y, int width, int line_dy, unsigned align, char *text)
{
    int fit;
    int rest;
    char part[12];
    register int i;
    register int half;

    fit = Paragraph_Used_Width(width, text, align);
    if (fit < 1) {
        half = strlen(text) / 2;
        rest = strlen(text) - half;
        for (i = 0; i < half; i++)
            part[i] = text[i];
        part[half] = '-';
        part[half + 1] = 0;
        Paragraph_Print(x, y, width, part, align);
        for (i = 0; i < rest; i++)
            part[i] = text[half + i];
        part[rest] = 0;
        Paragraph_Print(x, y + line_dy, width, part, align);
    } else
        Paragraph_Print(x, y, width, text, align);
}

/* (slot, &x1, &y1, &x2, &y2) the 32 x 32 box of a hero portrait: two columns (265 apart), rows 51 apart */
void ArmyList_Hero_Rect(int slot, int *x1, int *y1, int *x2, int *y2)
{
    *x1 = armylist_hero_x0 + slot % 2 * 265;
    *y1 = armylist_hero_y0 + slot / 2 * 51;
    *x2 = *x1 + 32;
    *y2 = *y1 + 32;
}
