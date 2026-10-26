/* MAINPANL.C - WIZARDS.EXE overlay code segment 56d3 (stub 243c): the main screen's right-hand panels - the minimap
   picture, the Gold / Food / Mana income panel with the event banner (conjunctions, moons, Mana Short; negative
   incomes blink), the Next Turn panel, the greyed unit order buttons, the stack panel's slot rectangles and the
   GoTo path preview of a stack.
   This module's data (DS:31b6): its literals; BSS (DS:c092): income_warn_blink.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 56d3 src/wizards/MAINPANL.C */
#include "STRUCTS.H"
#include "ENUMS.H"

int income_warn_blink;                          /* DS:c092  0: negative incomes blink; -1: drawn without palette cycling */

extern int no_units_to_move;                    /* DS:9444 */
extern unsigned next_turn_button_pic;           /* DS:9516 */
extern unsigned next_turn_bg_pic;               /* DS:952c  MAIN.LBX 0x23 "next turn backgrnd" */
extern unsigned deselect_bg_pic;                /* DS:952e  MAIN.LBX 0x22 "deselect backgrnd" (the income panel) */
extern unsigned goto_marker_pic;                /* DS:95f2 */
extern unsigned purify_lock_pic;                /* DS:9952 */
extern unsigned special_order_blank_pic;       /* DS:9954 */
extern unsigned wait_lock_pic;                  /* DS:9956 */
extern unsigned patrol_lock_pic;        /* DS:9958 */
extern unsigned done_lock_pic;          /* DS:995a */
extern unsigned p_scratch;                      /* DS:998e  segment of the scratch buffer */
extern int current_screen;                      /* DS:9990 */
extern struct Events far *p_events;             /* DS:9998 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern struct Unit far *p_units;                /* DS:9ec2 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int map_view_plane;                      /* DS:bd86 */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int current_player;                      /* DS:bd8c */
extern int stack_special_action;                /* DS:c04e */
extern int goto_preview_len;                    /* DS:c066 */
extern int goto_preview_on;                     /* DS:c068 */
extern int stack_panel_y;                       /* DS:c082 */
extern int stack_panel_x;                       /* DS:c084 */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern char path_cost[];                        /* DS:c5f0 */
extern char path_y[];                           /* DS:c668 */
extern char path_x[];                           /* DS:c6e0 */

unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Minimap_Set_Size(int width, int height);
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
void Minimap_Build(int ox, int oy, int plane, unsigned pic, int w, int h, int hx, int hy, int hilite);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana);
void Font_Select_Palette(int font, char *colors);
void Set_Text_Shadow_Color(int color);
void Font_Select_Outline(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
void Set_Space_Width(int width);
int Print_Number_Right(int x, int y, int value);
int Print_Text(int x, int y, char *text);
int Print_Text_Centered(int x, int y, char *text);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
void Stack_Blink_Hide(void);
void Outline_Cycle_Tick(void);
void Stack_Movement_Modes(int *modes, int *units, int count);
int Stack_Needs_Transport(int n, int *units, int *needing_ride);
int Path_Find(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying, int x1, int y1,
              int x2, int y2, int plane, unsigned char *path_x, unsigned char *path_y, unsigned char *path_cost,
              int window, int mode, int needs_transport, int stack_size, int player);
int Map_Line_Path(int x1, int y1, int x2, int y2, char *xs, char *ys, int width);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);

/* the minimap picture (0x12f paragraphs in front of the scratch block) */
void Minimap_Alloc_Pic(void)
{
    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS);
}

/* the same as Minimap_Alloc_Pic */
void Minimap_Alloc_Pic_Copy(void)
{
    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS);
}

/* the 58 x 30 minimap around the view into p_minimap_pic */
void Minimap_Build_View(void)
{
    int width;
    int height;
    int ox;
    int oy;

    width = MINIMAP_W;
    height = MINIMAP_H;
    Minimap_Set_Size(width, height);
    Map_Window_Origin(&ox, &oy, (map_view_x + MAP_VIEW_COLS / 2) % MAP_WIDTH, map_view_y + MAP_VIEW_ROWS / 2, width, height);
    Minimap_Build(ox, oy, map_view_plane, p_minimap_pic, width, height, 0, 0, 0);
}

/* the income panel (only while no stack is selected): Gold / Food / Mana per turn (a negative value in black on a
   blinking color), then the banner of an active event (the last one tested wins) with its help area */
void Draw_Income_Panel(void)
{
    int gold;
    int food;
    int mana;
    char colors[4];
    int unused[6];
    int event;
    int i;

    help_entries[MAIN_HELP_GOLD].id = -1;
    help_entries[MAIN_HELP_FOOD].id = -1;
    help_entries[MAIN_HELP_MANA].id = -1;
    if (stack_unit_count < 1) {
        help_entries[MAIN_HELP_DONE].id = -1;
        help_entries[MAIN_HELP_PATROL].id = -1;
        help_entries[MAIN_HELP_WAIT].id = -1;
        help_entries[MAIN_HELP_SPECIAL].id = -1;
        help_entries[MAIN_HELP_GOLD].id = HELP_GOLD_INCOME;
        help_entries[MAIN_HELP_FOOD].id = HELP_FOOD_RESERVE;
        help_entries[MAIN_HELP_MANA].id = HELP_MANA_INCOME;
        Draw_Pic_Frame(240, 76, deselect_bg_pic);
        Wizard_Income_Summary(current_player, &gold, &food, &mana);
        colors[0] = PAL_CYCLE_COLOR;            /* the cycled colour: a short income blinks */
        colors[1] = PAL_CYCLE_COLOR;
        colors[2] = PAL_CYCLE_COLOR;
        Font_Select_Palette(0, colors);
        Set_Text_Shadow_Color(0);
        if (gold < 0) {
            if (income_warn_blink > -1)
                Font_Select_Outline(0, 15, 0, 0);
            else
                Font_Select_Outline(0, 2, 0, 0);
            Set_Font_Color(0);
        } else {
            Font_Select_Outline(0, 4, 0, 0);
            Set_Font_Color(184);
        }
        Print_Number_Right(273, 101, gold);
        Print_Text(278, 101, "Gold");
        if (food < 0) {
            if (income_warn_blink > -1)
                Font_Select_Outline(0, 15, 0, 0);
            else
                Font_Select_Outline(0, 2, 0, 0);
            Set_Font_Color(0);
        } else {
            Font_Select_Outline(0, 4, 0, 0);
            Set_Font_Color(184);
        }
        Print_Number_Right(273, 133, food);
        Print_Text(278, 133, "Food");
        if (mana < 0) {
            if (income_warn_blink > -1)
                Font_Select_Outline(0, 15, 0, 0);
            else
                Font_Select_Outline(0, 2, 0, 0);
            Set_Font_Color(0);
        } else {
            Font_Select_Outline(0, 4, 0, 0);
            Set_Font_Color(184);
        }
        Print_Number_Right(273, 165, mana);
        Print_Text(278, 165, "Mana");
        Set_Space_Width(1);
        event = 0;
        for (i = 0; i < 6; i++)
            unused[i] = -1;
        help_entries[MAIN_HELP_EVENT_FIRST].id = -1;
        help_entries[MAIN_HELP_EVENT_FIRST + 1].id = -1;
        help_entries[MAIN_HELP_EVENT_FIRST + 2].id = -1;
        help_entries[MAIN_HELP_EVENT_FIRST + 3].id = -1;
        help_entries[MAIN_HELP_EVENT_FIRST + 4].id = -1;
        help_entries[MAIN_HELP_EVENT_FIRST + 5].id = -1;
        event = -1;                             /* the banner: 0 .. 2 the sorcery / chaos / nature conjunction, 3 Bad
                                                   Moon, 4 Good Moon, 5 Mana Short */
        if (p_events->conjunction_sorcery == EVENT_STATE_RUNNING) {
            colors[0] = 0;
            colors[1] = 172;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Text_Centered(280, 157, "Conjunction");
            event = 0;
        }
        if (p_events->conjunction_chaos == EVENT_STATE_RUNNING) {
            colors[0] = 0;
            colors[1] = 201;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Text_Centered(280, 158, "Conjunction");
            event = 1;
        }
        if (p_events->conjunction_nature == EVENT_STATE_RUNNING) {
            colors[0] = 0;
            colors[1] = 217;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Text_Centered(280, 158, "Conjunction");
            event = 2;
        }
        if (p_events->bad_moon == EVENT_STATE_RUNNING) {
            colors[0] = 0;
            colors[1] = 9;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Text_Centered(280, 158, "Bad Moon");
            event = 3;
        }
        if (p_events->good_moon == EVENT_STATE_RUNNING) {
            colors[0] = 0;
            colors[1] = 15;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Text_Centered(280, 158, "Good Moon");
            event = 4;
        }
        if (p_events->mana_short == EVENT_STATE_RUNNING) {
            colors[0] = 0;
            colors[1] = 123;
            Font_Select_Palette(0, colors);
            Set_Text_Shadow_Color(0);
            Font_Select_Outline(0, 15, 0, 0);
            Print_Text_Centered(280, 158, "Mana Short");
            event = 5;
        }
        if (event > -1) {
            help_entries[event + MAIN_HELP_EVENT_FIRST].id = event + HELP_EVENT_BANNER;
            help_entries[event + MAIN_HELP_EVENT_FIRST].x1 = 260;
            help_entries[event + MAIN_HELP_EVENT_FIRST].y1 = 157;
            help_entries[event + MAIN_HELP_EVENT_FIRST].x2 = 300;
            help_entries[event + MAIN_HELP_EVENT_FIRST].y2 = 163;
        }
        if (income_warn_blink > -1)
            Palette_Cycle_Color(PAL_CYCLE_COLOR, 40, 0, 0, 63, 0, 0, 3);
    }
}

/* the Next Turn panel, while no unit is left to move: the outline animation, or the Next Turn button picture
   only in the turn-ending frame (SCREEN_ENDING_TURN) */
void Draw_Next_Turn_Panel(void)
{
    help_entries[MAIN_HELP_NEXT_TURN].id = -1;
    if (no_units_to_move == 1) {
        help_entries[MAIN_HELP_NEXT_TURN].id = HELP_NEXT_TURN;
        Draw_Pic_Frame(240, 173, next_turn_bg_pic);
        Stack_Blink_Hide();
        if (current_screen == SCREEN_ENDING_TURN)
            Draw_Pic_Frame(246, 178, next_turn_button_pic);
        else
            Outline_Cycle_Tick();
    }
}

/* () the negative-income warning stops blinking (-1) */
void Income_Warn_Blink_Off(void)
{
    income_warn_blink = -1;
}

/* () it blinks again (0) */
void Income_Warn_Blink_On(void)
{
    income_warn_blink = 0;
}

/* the four order buttons at frame 0 (the inactive look while a spell target is picked); the last one is the special
   action button when the stack has one */
void Draw_Order_Buttons_Disabled(void)
{
    Set_Pic_Frame(done_lock_pic, 0);
    Draw_Pic_Frame(246, 176, done_lock_pic);
    Set_Pic_Frame(patrol_lock_pic, 0);
    Draw_Pic_Frame(280, 176, patrol_lock_pic);
    Set_Pic_Frame(wait_lock_pic, 0);
    Draw_Pic_Frame(246, 186, wait_lock_pic);
    if (stack_special_action != STACK_SPECIAL_NONE) {  /* the greyed PURIFY picture for ANY special action */
        Set_Pic_Frame(purify_lock_pic, 0);
        Draw_Pic_Frame(280, 186, purify_lock_pic);
    } else {
        Set_Pic_Frame(special_order_blank_pic, 0);
        Draw_Pic_Frame(280, 186, special_order_blank_pic);
    }
}

/* (i, &x1, &y1, &x2, &y2): the rectangle of slot i of the 3 x 3 stack panel (23 x 29 apart) */
void Stack_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2)
{
    *x1 = stack_panel_x + i % 3 * 23;
    *y1 = stack_panel_y + i / 3 * 29;
    *x2 = *x1 + 22;
    *y2 = *y1 + 28;
}

/* (unit): the path preview of the selected stack's GoTo (Path_Find) or road-building order (the straight line from
   the road's start, from the unit's square on) into path_x / path_y, goto_preview_len; 0 if no unit of the stack has a
   GoTo order (no return value otherwise) */
int Stack_Preview_GoTo_Path(int unit)
{
    int goto_count;
    int x;
    int y;
    int x2;
    int y2;
    int needs_transport;
    int stack_size;
    int player;
    int road_x;
    int road_y;
    int found;
    int modes[6];
    int units[MAX_STACK];
    int transports[MAX_STACK];
    int i;

    x = p_units[unit].x;
    y = p_units[unit].y;
    x2 = p_units[unit].dest_x;
    y2 = p_units[unit].dest_y;
    player = p_units[unit].owner;
    stack_size = stack_unit_count;
    goto_count = 0;
    for (i = 0; i < stack_size; i++) {
        units[i] = stack_units[i].unit;
        if (p_units[units[i]].order == ORDER_GOTO)
            goto_count++;
    }
    if (goto_count < 1)
        return 0;
    Stack_Movement_Modes(modes, units, stack_size);
    needs_transport = Stack_Needs_Transport(stack_size, units, transports);
    if (p_units[unit].road_turns == -1)
        goto_preview_len = Path_Find(modes[MOVE_MODE_CAVALRY], modes[MOVE_MODE_FORESTER], modes[MOVE_MODE_MOUNTAINEER], modes[MOVE_MODE_SWIMMING], modes[MOVE_MODE_SAILING], modes[MOVE_MODE_FLYING], x, y, x2, y2,
                                     map_view_plane, path_x, path_y, path_cost, 1, 30, needs_transport, stack_size,   /* 1, 30: window / mode,
                                                   overwritten in Path_Find */
                                     player);
    else {
        road_x = p_units[unit].road_start_x;
        road_y = p_units[unit].road_start_y;
        goto_preview_len = Map_Line_Path(road_x, road_y, x2, y2, path_x, path_y, MAP_WIDTH);
        found = -1;
        for (i = 0; i < goto_preview_len; i++) {
            if (path_x[i] == x && path_y[i] == y) {
                found = i;
                break;
            }
        }
        if (found != -1) {
            goto_preview_len = goto_preview_len - found - 1;
            for (i = 0; i < goto_preview_len; i++) {
                path_x[i] = path_x[found + i + 1];
                path_y[i] = path_y[found + i + 1];
            }
        }
    }
    if (goto_preview_len > 0)
        goto_preview_on = 1;
}

/* (len, xs, ys): a GoTo marker on every path square that is on screen */
void Draw_GoTo_Path_Markers(int len, char *xs, char *ys)
{
    int sx;
    int sy;
    int i;

    for (i = 0; i < len; i++) {
        sx = xs[i];
        sy = ys[i];
        if (Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy) == 1)
            Draw_Pic_Frame(sx + 8, sy + 6, goto_marker_pic);
    }
}
