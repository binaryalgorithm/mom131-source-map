/* RESUME.C - WIZARDS.EXE code segment 82d2: the overland view after a load / at startup.
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 82d2 src/wizards/RESUME.C */
#include "GAME.H"

extern int stack_panel_y;                       /* DS:c082 */
extern int stack_panel_x;                       /* DS:c084 */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */

/* () the overland view after a load (Game_Rebuild_After_Load) or at the start (Game_Start_Init): the stack panel
   position, no combat square, the visibility
   rebuilt around running the GoTo orders */
void Overland_Resume_View(void)
{
    stack_panel_x = STACK_PANEL_HOME_X;
    stack_panel_y = STACK_PANEL_HOME_Y;
    combat_loc_x = -1;
    combat_loc_y = -1;
    Visibility_Rebuild();
    Player_Stacks_Changed_Nop(current_player);
    Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
    Visibility_Rebuild();
}
