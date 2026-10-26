/* GAMEINIT.C - WIZARDS.EXE code segment 45d6: game state set-up after a new game / load.
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 45d6 src/wizards/GAMEINIT.C */
#include "GAME.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order; batch 39) */
void Game_Session_Init(void);
void Game_Init_State(void);

extern int stack_fresh_selection;               /* DS:9446  1 after a new stack selection until the first unit click in the stack panel (that click drops the other active units) */
extern int research_first_page;                 /* DS:9448 */
extern int overland_spellbook_page;                            /* DS:944a  the overland book's page kept between casts (SPELLCST.C); cleared by Game_Init_State */
extern int combat_spellbook_page;               /* DS:944c */
extern int magic_lock_mana;                     /* DS:947e */
extern int magic_lock_research;                 /* DS:9480 */
extern int magic_lock_skill;                    /* DS:9482 */
extern int map_scroll_px_y;                     /* DS:9746 */
extern int map_scroll_px_x;                     /* DS:9748 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int stack_panel_y;                       /* DS:c082 */
extern int stack_panel_x;                       /* DS:c084 */

int Tile_Has_Tower();                           /* no prototype here: called with x, y, plane; the definition in MAPDRAW.C takes x, y */
void Minimap_Alloc_Pic(void);
void Stack_Clear_Draw_Priority(void);
void Load_Init_Map_Nop(void);
void Cities_Recalculate_All(void);
void Random_Seed(long seed);
void Random_Seed_From_Timer(void);
void City_Recalculate(int city);
void Wizard_Power_Base(void);
int Research_Candidates_Refill(int player);

/* () after a load / new game: Game_Init_State, every city's dummy building slot set, units on plane 2
   moved to 0 and their tower flags refreshed, the magic screen's locks cleared, the human's casting skill,
   the map view centred on the human's fortress, the unit grid and screen graphics, the GoTo orders run,
   the cities recalculated; a fixed random seed outside the release build */
void Game_Session_Init(void)
{
    int i;

    Game_Init_State();
    for (i = 0; i < num_cities; i++)
        p_cities[i].buildings[BLD_NONE] = BLDSTATE_REPLACED;
    for (i = 0; i < num_units; i++) {
        if (p_units[i].plane == PLANE_BOTH)
            p_units[i].plane = PLANE_ARCANUS;
        if (Tile_Has_Tower(p_units[i].x, p_units[i].y, p_units[i].plane) == 1)
            p_units[i].in_tower = 1;
        else
            p_units[i].in_tower = 0;
    }
    magic_lock_skill = 0;
    magic_lock_mana = 0;
    magic_lock_research = 0;
    wizards[PLAYER_HUMAN].nominal_casting_skill = Wizard_Casting_Skill(PLAYER_HUMAN);
    map_scroll_px_x = 0;
    map_scroll_px_y = 0;
    map_scroll_target_x = 0;
    map_scroll_target_y = 0;
    map_view_x = 0;
    map_view_y = 0;
    Player_Stacks_Changed_Nop(current_player);
    top_unit_in_stack = 0;
    next_unit_ref_x = p_fortresses[PLAYER_HUMAN].x;
    next_unit_ref_y = p_fortresses[PLAYER_HUMAN].y;
    stack_panel_x = STACK_PANEL_HOME_X;
    stack_panel_y = STACK_PANEL_HOME_Y;
    map_view_plane = p_fortresses[PLAYER_HUMAN].plane;
    Wizards_Update_Contacts();
    Minimap_Alloc_Pic();
    Map_Center_View(&map_view_x, &map_view_y, p_fortresses[PLAYER_HUMAN].x, p_fortresses[PLAYER_HUMAN].y, map_view_plane);
    Units_Set_Draw_Priority();
    Stack_Clear_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Load_Overland_Screen_Graphics();
    Next_Unit_Run_GoTos(current_player, &map_view_x, &map_view_y, &map_view_plane);
    Load_Init_Map_Nop();
    Cities_Recalculate_All();
    if (Check_Release_Version() == 0)
        Random_Seed(10039L);                    /* a fixed seed outside the release build (after a load; a new
                                                   game's Game_Init_State seeds 1000) */
}

/* () the per-session state: the random seed (fixed outside the release build), the stack selection, every
   city recomputed, the power bases, the spellbook pages, every wizard's research candidates
   Numbered: BUG #107 (BUGS_AND_QUIRKS.txt) */
void Game_Init_State(void)
{
    long seed;
    int city;
    int player;

    Random_Seed_From_Timer();
    seed = 1000;                                /* the fixed debug seed */
    if (Check_Release_Version() == 0)
        Random_Seed(seed);
    no_units_to_move = 0;
    stack_fresh_selection = 1;
    Stack_Blink_Reset();
    map_view_plane = 0;
    for (city = 0; city < num_cities; city++)
        City_Recalculate(city);
    Wizard_Power_Base();
    overland_spellbook_page = 0;
    combat_spellbook_page = 0;
    research_first_page = 0;
    for (player = 0; player < num_players; player++)
        Research_Candidates_Refill(player);
}
