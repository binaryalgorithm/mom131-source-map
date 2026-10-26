/* NEXTTURN.C - WIZARDS.EXE code segment 9ec1: the end-of-turn driver and two city recalculation loops.
   This module's data (DS:5e90..5e95): two words and the turn seed (moved here in b105: data after RESEARCH.C's literals).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 9ec1 src/wizards/NEXTTURN.C */
#include "GAME.H"

int nextturn_unused_5e90 = 0;                   /* DS:5e90  never referenced */
long turn_rng_seed = 0x2a57;                    /* DS:5e92  Next_Turn reseeds Random from it each turn */

extern int debug_autoplay;                      /* DS:2e12 */
extern int time_stop_player;                    /* DS:912a  player + 1 of the Time Stop caster, 0 = none */
extern char report_globals_lost;                /* DS:9c8c */
extern int grand_vizier;                        /* DS:9d22 */
extern int game_turn;                           /* DS:bd90 */
extern struct MouseRegion mouse_region_busy;    /* DS:0106 */

void Cities_Recalculate_All(void);
int AI_Disband_Obsolete_Units(void);
void AI_Overland_Turn(void);
void Purify_Turn(void);
void Turn_Report_Clear(void);
void Diplo_Tick_Peace_Turns(void);
void Turn_Gold_Income(void);
void Wizard_Power_Base(void);
void Turn_Apply_Magic_Income(void);
void Turn_Research_Complete(void);
void Income_Warn_Blink_Off(void);
void Income_Warn_Blink_On(void);
int Event_Roll(void);
void Events_Process(void);
void Turn_Pay_Upkeep(void);
void EMS_Map_Default(void);
void Outposts_Grow(void);
void Turn_City_Growth(void);
void Turn_Global_Enchant_Diplomacy(void);
int Vizier_Choose_Production(int city);
void Turn_Random_Offers(void);
void Cities_Count_Nightshade(void);
void Diplo_Relation_Drift(void);
void Diplo_Mark_Met(void);
int AI_Diplo_Turn_AI_Vs_AI(void);
int AI_Diplo_Turn_Vs_Human(void);
int Diplo_Show_Pending_Messages(void);
void Diplo_Interest_Regrow(void);
void Diplo_Interest_Drift(void);
void Turn_Volcano_Decay(void);
void Turn_Overland_Casting(void);
void Turn_Finalize_Removed_Buildings(void);
void Turn_Unit_Experience(void);
void Turn_Heal_Units(void);
void Wizard_Update_Power_Stats(void);
void Music_Mood_Check(void);
int Autosave(void);
void Random_Seed(long seed);
long Random_Get_Seed(void);
void City_Recalculate(int city);
int City_Min_Farmers(int city);
int City_Rebels(int city);

/* () the end of a turn for everyone: the cities, the AI's obsolete units disbanded, the AI turn, Purify and the
   turn report cleared, then (unless a Time Stop runs) gold,
   power, magic income and research, the random events, upkeep, outpost and city growth, the Grand
   Vizier's production picks, offers, Nightshade, diplomacy and the volcanoes; under Time Stop its caster
   pays 200 mana (it ends at 0). Then overland casting, the unit table, experience, healing, the power
   statistics and the music mood; the turn counter, the 30000 caps on gold and mana, the autosave and
   the random seed
   Numbered: BUG #224, #242, #252, #267 (BUGS_AND_QUIRKS.txt) */
void Next_Turn(void)
{
    int city;                                   /* reused as the player in the treasury cap loop */
    int choose;

    Random_Seed(turn_rng_seed);
    Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
    Cities_Recalculate_All();
    AI_Disband_Obsolete_Units();
    AI_Overland_Turn();
    Purify_Turn();
    Turn_Report_Clear();
    if (time_stop_player == 0) {
        Diplo_Tick_Peace_Turns();
        Turn_Gold_Income();
        Wizard_Power_Base();
        Turn_Apply_Magic_Income();
        Turn_Research_Complete();
        Income_Warn_Blink_Off();
        if (debug_autoplay == 0 && game_config.random_events == 1)
            Event_Roll();
        Events_Process();
        Turn_Pay_Upkeep();
        EMS_Map_Default();
        Outposts_Grow();
        Turn_City_Growth();
        Turn_Global_Enchant_Diplomacy();
        if (grand_vizier == 1) {
            for (city = 0; city < num_cities; city++) {
                if (p_cities[city].owner == PLAYER_HUMAN) {
                    choose = 0;
                    if (p_cities[city].producing == PROD_AUTO_CHOOSE)
                        choose = 1;
                    if ((p_cities[city].producing == PROD_HOUSING || p_cities[city].producing == PROD_TRADE_GOODS) && Random(4) == 1)
                        choose = 1;
                    if (choose == 1)
                        Vizier_Choose_Production(city);
                }
            }
        }
        Turn_Random_Offers();
        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
        Cities_Count_Nightshade();
        Diplo_Relation_Drift();
        Diplo_Mark_Met();
        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
        AI_Diplo_Turn_AI_Vs_AI();
        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
        AI_Diplo_Turn_Vs_Human();
        Diplo_Show_Pending_Messages();
        Mouse_Set_Cursor_Regions(1, &mouse_region_busy);
        Diplo_Interest_Regrow();
        Diplo_Interest_Drift();
        Turn_Volcano_Decay();
    } else {
        wizards[time_stop_player - 1].mana_reserve -= TIME_STOP_UPKEEP;
        if ((&wizards[time_stop_player - 1])->mana_reserve < 0) {   /* this spelling compiles as the original */
            wizards[time_stop_player - 1].mana_reserve = 0;
            wizards[time_stop_player - 1].globals.time_stop = 0;
            if (time_stop_player == 1)
                report_globals_lost = 1;
            time_stop_player = 0;
        }
    }
    Turn_Overland_Casting();
    Units_Compact();
    Units_Set_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Turn_Finalize_Removed_Buildings();
    Turn_Unit_Experience();
    Turn_Heal_Units();
    Wizard_Update_Power_Stats();
    Music_Mood_Check();
    game_turn++;
    for (city = 0; city < num_players; city++) {   /* city: here a player */
        if (wizards[city].gold > RESOURCE_CAP)
            wizards[city].gold = RESOURCE_CAP;
        if (wizards[city].mana_reserve > RESOURCE_CAP)
            wizards[city].mana_reserve = RESOURCE_CAP;
    }
    Income_Warn_Blink_On();
    Autosave();
    Cities_Recalculate_All();
    turn_rng_seed = Random_Get_Seed();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
}

/* () every city recomputed, its farmers raised to the minimum that feeds it, its rebels recounted */
void Cities_Recalc_All_Farmers(void)
{
    int min_farmers;
    int city;

    for (city = 0; city < num_cities; city++) {
        City_Recalculate(city);
        min_farmers = City_Min_Farmers(city);
        if (p_cities[city].farmers < min_farmers)
            p_cities[city].farmers = min_farmers;
        City_Rebels(city);
    }
}

/* (player) the same for the player's cities */
void Player_Cities_Recalc(int player)
{
    int min_farmers;
    int city;

    for (city = 0; city < num_cities; city++) {
        if (p_cities[city].owner == player) {
            City_Recalculate(city);
            min_farmers = City_Min_Farmers(city);
            if (p_cities[city].farmers < min_farmers)
                p_cities[city].farmers = min_farmers;
            City_Rebels(city);
        }
    }
}
