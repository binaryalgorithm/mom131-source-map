/* MAINSCR.C - WIZARDS.EXE code segment 517e: overland stack panel, the music mood track, stack selection helpers.
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 517e src/wizards/MAINSCR.C */
#include "GAME.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order; batch 39) */
int Overland_Draw_Stack_Panel(void);
void Mouse_Default_Cursor(void);
void First_Move_Nop(void);
void Clear_First_Move_Flag(void);
void Music_Mood_Check(void);
int Music_Mood_Track(void);
void Music_Play_Current(void);
void Select_Stack_Of_Unit(int unit);
void Select_Stack_At_Summoning_Circle(void);

int music_mood_tick = 0;                        /* DS:2f84  Music_Mood_Check's call count, 0 .. 4 */
extern int weapon_badge_frame;                  /* DS:305c  0..3, stepped once per stack-panel draw: the weapon-quality badge animation (ReMoM unit_weapon_type_animation_count; clarified using ReMoM repo) */
extern unsigned p_music_buf;                    /* DS:9390  segment of the music buffer */
extern unsigned stack_slot_bg_pics[];           /* DS:9924 */
extern int stack_unit_count;                    /* DS:999c */
extern struct StackSlot stack_units[];          /* DS:999e */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int game_turn;                           /* DS:bd90 */
extern int first_move_pending;                     /* DS:c044 */
int music_cur_track;                            /* DS:c08a  the track playing (only this module reads it) */

void Stack_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2);
void Draw_Unit_Icon(int x, int y, int unit, int icon_mode);
void Draw_Unit_Badges(int x, int y, int unit);
void Outline_Cycle_Tick(void);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Stack_Clear_Draw_Priority(void);

/* () the selected stack's slots on the main screen (slot background, unit icon - highlighted when active -
   and badges), then the outline color cycle and the weapon badge frame (0..3). 0 with no stack, else no
   return value */
int Overland_Draw_Stack_Panel(void)
{
    int x;
    int y;
    int x2;
    int y2;
    int slot;
    int unit;

    if (top_unit_in_stack == -1 || stack_unit_count == 0)
        return 0;
    for (slot = 0; slot < stack_unit_count; slot++) {
        unit = stack_units[slot].unit;
        Stack_Slot_Rect(slot, &x, &y, &x2, &y2);
        Draw_Pic_Frame(x - 1, y - 1, stack_slot_bg_pics[slot]);
        Draw_Unit_Icon(x, y, unit, stack_units[slot].active);
        Draw_Unit_Badges(x, y, unit);
    }
    Outline_Cycle_Tick();
    weapon_badge_frame = weapon_badge_frame + 1;
    if (weapon_badge_frame > 3)
        weapon_badge_frame = 0;
}

/* () the plain arrow pointer over the whole screen */
void Mouse_Default_Cursor(void)
{
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
}

/* () empty; Stack_Move_Click calls it while first_move_pending is 0 */
void First_Move_Nop(void)
{
}

/* () first_move_pending = 0 */
void Clear_First_Move_Flag(void)
{
    first_move_pending = 0;
}

/* () every 5th call: restarts the music when the mood track (Music_Mood_Track) changed */
void Music_Mood_Check(void)
{
    int track;

    if (music_mood_tick > 3) {
        music_mood_tick = 0;
        track = Music_Mood_Track();
        if (track != music_cur_track)
            Music_Play_Current();
    } else
        music_mood_tick++;
}

/* () -> the MUSIC.LBX track for the current player's standing, from last turn's power history (turn capped at
   287) against the rival wizards 1 .. num_players - 1: 100 neutral, 99 when within a tenth of the strongest
   rival (or ahead), 101 when within a tenth of the weakest (tested last, so it wins a tie) */
int Music_Mood_Track(void)
{
    int track;
    int best;
    int turn;
    int player;
    int worst;

    track = MUSIC_MOOD_NEUTRAL;
    if (game_turn < POWER_HISTORY_TURNS)
        turn = game_turn - 1;
    else
        turn = POWER_HISTORY_TURNS - 1;
    if (turn < 0)
        turn = 0;
    best = 0;
    worst = 255;
    for (player = 1; player < num_players; player++) {
        if (wizards[player].power_history[turn] > best)
            best = wizards[player].power_history[turn];
        if (wizards[player].power_history[turn] < worst)
            worst = wizards[player].power_history[turn];
    }
    if (worst == 255)
        worst = 0;
    if (wizards[current_player].power_history[turn] + best / 10 >= best)
        track = MUSIC_MOOD_WINNING;
    if (worst > 0 && wizards[current_player].power_history[turn] - worst / 10 <= worst)
        track = MUSIC_MOOD_LOSING;
    return track;
}

/* () music on: stops the current piece and plays the mood track (music_cur_track); music off: stops */
void Music_Play_Current(void)
{
    unsigned block;
    int track;

    if (game_config.music == 1) {
        Sound_Stop();
        track = Music_Mood_Track();
        music_cur_track = track;
        block = LBX_Reload("MUSIC", track, p_music_buf);
        Sound_Play(block);
    } else
        Sound_Stop();
}

/* (unit) selects the current player's stack on the unit's square and refreshes the draw priorities and the
   unit grid
   Numbered: BUG #205 (BUGS_AND_QUIRKS.txt) */
void Select_Stack_Of_Unit(int unit)
{
    Player_Stacks_Changed_Nop(current_player);
    Stack_Select_At(current_player, &map_view_x, &map_view_y, p_units[unit].plane, p_units[unit].x, p_units[unit].y);
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    no_units_to_move = 0;
    Stack_Blink_Reset();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
}

/* () the same for the stack on the current player's summoning circle */
void Select_Stack_At_Summoning_Circle(void)
{
    Player_Stacks_Changed_Nop(current_player);
    Stack_Select_At(current_player, &map_view_x, &map_view_y, wizards[current_player].summoning_circle_xyp[XYP_PLANE],
                    wizards[current_player].summoning_circle_xyp[XYP_X], wizards[current_player].summoning_circle_xyp[XYP_Y]);
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    no_units_to_move = 0;
    Stack_Blink_Reset();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
}
