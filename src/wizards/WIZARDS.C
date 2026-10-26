/* WIZARDS.C - WIZARDS.EXE code segment 13ec (resident): the game program's main module - main() (CONFIG.MOM sound
   set-up, MAGIC.SET, engine start-up, the auto-save, the overlay manager's EMS), the top-level screen loop, the
   per-load game set-up, the screen presenter and palette fades, the weighted random picks, the click / message
   sounds - and the program's global variables: the game tables (unit types, races, names, upkeep tables, AI
   personality / objective tables, difficulty) with their strings, DS:00aa .. 2880, and the main block of
   uninitialised globals, DS:8890 .. bf7f (every other module reaches them through extern declarations).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 13ec src/wizards/WIZARDS.C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "STRUCTS.H"
#include "ENUMS.H"
#pragma warn -pro

/* the module's functions in code order */
void main(void);
void Main_Loop(void);
void Screen_Present(void);
void Palette_Fade_To_Black(void);
void Palette_Fade_From_Black(void);
void Game_Start_Init(void);
int Random_Weighted_Pick(int *weights, int count);
int Random_Weighted_Pick32(long *weights, int count);
int Weights_Jitter_Max(unsigned *weights, int count, int jitter);
int Weights_Jitter_Max32(long *weights, int count, int jitter);
void Window_Zoom_Skipped_Nop(void);
void Window_Zoom_Start_Nop(void);
void Play_Click_Sound(void);
void Play_Message_Sound(void);
void Play_Click_Sound_Casting(void);
void Quit_Hook_Empty(void);

extern int path_cache_index;                    /* DS:6f76 */
extern int ems_reserved_pages;                  /* DS:7628 */
extern int city_razed;                          /* DS:c974 */
extern int mastery_winner;                      /* DS:ca56  the player who cast the Spell of Mastery, -1 none */
extern int ems_main_handle;                     /* DS:e5ec */

int DOS_Find_File(char *pattern, char *out);
void Fatal_Exit(char *message);
void Settings_Load_Or_Create(void);
void DOS_Print_Dollar(char *text);
void Timer_Mark(void);
void Wait_Ticks(int ticks);
void EMS_Set_Required_K(int k);
void LBX_Set_Min_Free(int kb);
void Engine_Init(int input_mode, int sound_mode, char *lbx, int music_type, int music_io, int music_irq, int music_dma,
                 int digi_type, int digi_io, int digi_irq, int digi_dma);
void Release_Version(void);
void UI_Right_Click_Ids_On(void);
void Alloc_Game_Data(int scratch_paras);
void Load_Game(int slot);
void Load_Startup_Assets(void);
int EMS_Alloc_Named(int pages, char *name);
void EMS_Free_Handle(int handle);
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
void Remap_Build_All(void);
void UI_Set_Press_Offset(int dx, int dy);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
void Palette_Upload(void);
void UI_Clear_Controls(void);
void Memory_Report_Exit(void);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
void Screen_City(void);
void Screen_Game_Menu(void);
void Music_Play_Current(void);
int Music_Release_Sequence(void);
void Screen_Overland_Map(void);
void Screen_Armies(void);
void Screen_Cities(void);
void Screen_Magic(void);
int Screen_Road_Building(void);
void Screen_City_Production(void);
void Screen_Items(void);
void Human_End_Turn(void);
void Screen_Spellbook_Cast(void);
void Screen_Info_Menu(int choice);
int Screen_Diplomacy(void);
void VGA_Present_Wipe_LR(void);
void UI_Present_Frame(void);
void VGA_Present_Dissolve(void);
void Screen_Transition_Zoom(int x, int y, int steps, unsigned pic);
void Palette_Mark_Range(int first, int last);
void Palette_Darken_Percent(int percent);
void City_Area_Bitmaps_Rebuild(void);
void Game_Session_Init(void);
void UnitTypes_Init_Upkeep_Sounds(void);
void Human_Cities_Area_Nop(void);
void Never_Set_Flag_Tiles_Collect(void);
void Cities_Recalc_All_Farmers(void);
void Load_Overland_Screen_Graphics(void);
void Cities_Update_Road_Connections(void);
void Units_Compact(void);
void Units_Reset_Moves_NonCurrent(void);
void Overland_Resume_View(void);
void AI_Build_Landmass_Tile_Lists(void);
void AI_Build_Coast_Tile_Lists(void);
int Random(int n);
int Sound_Play(unsigned sound);

/* the main block of uninitialised globals, DS:8890 .. bf7f, in DESCENDING address order (Borland lays _BSS out in
   reverse declaration order); generated from the matched sources' extern declarations by
   work/reports/gen_wizards_globals.py. The bss_unused_xxxx fillers are the gaps no code reads (refcount over the complete source, N213). */
int *p_city_citizen_ctls;                        /* DS:bf7e  control id of each worker in the citizens row (ReMoM city_population_row_fields; clarified using ReMoM repo) */
int city_breakdown_ctls[7];                     /* DS:bf70  the breakdown controls (a click opens Screen_City_Breakdown) */
struct GameConfig game_config;                  /* DS:bd9e  MAGIC.SET, read / written whole */
int num_players;                                /* DS:bd9c */
int land_size;                                  /* DS:bd9a */
int magic_setting;                              /* DS:bd98 */
int difficulty;                                 /* DS:bd96 */
int num_cities;                                 /* DS:bd94 */
int num_units;                                  /* DS:bd92 */
int game_turn;                                  /* DS:bd90 */
int top_unit_in_stack;                          /* DS:bd8e */
int current_player;                             /* DS:bd8c */
int map_view_x;                                 /* DS:bd8a */
int map_view_y;                                 /* DS:bd88 */
int map_view_plane;                             /* DS:bd86 */
int next_unit_ref_x;                            /* DS:bd84 */
int next_unit_ref_y;                            /* DS:bd82 */
unsigned map_dashed_box_pic;                        /* DS:bd80  MAPBACK 0x59: a 6-frame blue dashed square, never drawn */
struct ConfigMom config_mom;                    /* DS:bd6e  CONFIG.MOM (the sound setup INSTALL writes) */
struct HelpEntry help_entries[50];              /* DS:bb7a */
struct Wizard wizards[6];                       /* DS:9eca */
struct HeroName far *p_hero_names;              /* DS:9ec6 */
struct Unit far *p_units;                       /* DS:9ec2 */
int ems_cont_handle;                            /* DS:9ec0  EMS handle "CONTXXX" (ReMoM EmmHndl_CONTXXX; clarified using ReMoM repo) */
char far *p_landmass_tile_x[2];                 /* DS:9eb8  signed: f48f reads it with cbw */
char far *p_landmass_tile_y[2];                 /* DS:9eb0  signed: f48f reads it with cbw */
int far *p_landmass_tile_next[2];               /* DS:9ea8 */
int far *p_landmass_tile_head[2];               /* DS:9ea0 */
char far *p_ai_square_stack_x;                         /* DS:9e9c */
char far *p_ai_square_stack_y;                         /* DS:9e98 */
char far *p_ai_square_stack_plane;                     /* DS:9e94 */
char far *p_ai_square_stack_class;                     /* DS:9e90 */
char far *p_ai_square_stack_size;                      /* DS:9e8c */
int far *p_ai_square_stack_units[80];                  /* DS:9d4c */
int ai_square_stack_count;                             /* DS:9d4a */
char far *p_coast_tile_x[2];                    /* DS:9d42  signed: f48f reads it with cbw */
char far *p_coast_tile_y[2];                    /* DS:9d3a  signed: f48f reads it with cbw */
int far *p_coast_tile_next[2];                  /* DS:9d32 */
int far *p_coast_tile_head[2];                  /* DS:9d2a */
char bss_unused_9d24[6];                        /* DS:9d24  never referenced (a gap in the block) */
int grand_vizier;                               /* DS:9d22 */
unsigned p_world_block;                       /* DS:9d20 */
/* p_world_block, p_screen_gfx, p_scratch ...: segment words of heap blocks, not C pointers */
unsigned p_screen_gfx;                          /* DS:9d1e */
unsigned world_block_rest;                      /* DS:9d1c */
int world_block_paras;                          /* DS:9d1a */
unsigned big_city_wall_pic;                     /* DS:9d18  CITYSCAP 0x72 'big city wall': the completed-walls popup */
char far *p_race_unrest_rows[14];              /* DS:9ce0  TERRSTAT record 1: [home race][city race] unrest, x 10 % */
unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];         /* DS:9cdc */
unsigned char (far *p_region_bits_land)[REGION_ROWS][REGION_COLS];        /* DS:9cd8 */
unsigned char (far *p_region_bits_water)[REGION_ROWS][REGION_COLS];        /* DS:9cd4 */
unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];    /* DS:9cd0 */
struct Node far *p_nodes;                       /* DS:9ccc */
struct Fortress far *p_fortresses;              /* DS:9cc8 */
struct Tower far *p_towers;                     /* DS:9cc4 */
struct Lair far *p_lairs;                       /* DS:9cc0 */
struct City far *p_cities;                      /* DS:9cbc */
unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];    /* DS:9cb8 */
unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];/* DS:9cb4  2400 bytes per plane */
unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH];/* DS:9cb0 */
unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH];/* DS:9cac */
unsigned char far *p_terrain_minimap_colors;    /* DS:9ca8 */
struct PathWork far *p_path_work;               /* DS:9ca4 */
unsigned char far *city_area_shared_bits;       /* DS:9ca0 */
unsigned char far *city_area_bits;              /* DS:9c9c */
unsigned far *p_threat_maps[2];                 /* DS:9c94 */
struct PathCacheEntry far *p_path_cache;        /* DS:9c90  140 records */
int present_mode;                               /* DS:9c8e */
char report_globals_lost;                       /* DS:9c8c */
unsigned char report_city_spells_lost[MAX_REPORT_ENTRIES];      /* DS:9c78 */
char report_city_spells_lost_count;             /* DS:9c77 */
unsigned char report_unit_spells_lost[MAX_REPORT_ENTRIES];      /* DS:9c63 */
char report_unit_spells_lost_count;             /* DS:9c62 */
unsigned char report_city_deaths[MAX_REPORT_ENTRIES];           /* DS:9c4e */
char report_city_deaths_count;                  /* DS:9c4d */
unsigned char report_city_growth[MAX_REPORT_ENTRIES];           /* DS:9c39 */
char report_city_growth_count;                  /* DS:9c38 */
struct ReportUnitKilled report_units_killed[MAX_REPORT_ENTRIES];/* DS:9be8 */
char report_units_killed_count;                 /* DS:9be6 */
struct ReportUnitLost report_units_lost[MAX_REPORT_ENTRIES];    /* DS:9b96 */
char report_units_lost_count;                   /* DS:9b94 */
struct ReportBldgLost report_buildings_lost[MAX_REPORT_ENTRIES]; /* DS:9b44 */
char report_buildings_lost_count;                     /* DS:9b42 */
struct CityNotice city_notices[MAX_REPORT_ENTRIES];             /* DS:9af2 */
char city_notice_count;                         /* DS:9af0 */
unsigned char report_outposts_grown[MAX_REPORT_ENTRIES];        /* DS:9adc */
char report_outposts_grown_count;               /* DS:9adb */
char report_outposts_deserted_names[14][20];    /* DS:99c3  BUG #375: 14 names, Turn_City_Growth accepts 20 */
char report_outposts_deserted_count;            /* DS:99c2 */
struct StackSlot stack_units[MAX_STACK];                /* DS:999e */
int stack_unit_count;                           /* DS:999c */
struct Events far *p_events;                    /* DS:9998 */
int zoom_from_x;                                /* DS:9996 */
int zoom_from_y;                                /* DS:9994 */
int zoom_steps;                                 /* DS:9992 */
int current_screen;                             /* DS:9990 */
unsigned p_scratch;                             /* DS:998e  segment of the scratch buffer */
unsigned p_pic_work;                            /* DS:998c */
unsigned move_type_icons[10];                   /* DS:9978 */
unsigned topbar_plane_pic;                      /* DS:9976 */
unsigned topbar_armies_pic;                     /* DS:9974 */
unsigned topbar_game_pic;                       /* DS:9972 */
unsigned topbar_cities_pic;                     /* DS:9970 */
unsigned topbar_magic_pic;                      /* DS:996e */
unsigned topbar_info_pic;                       /* DS:996c */
unsigned topbar_spells_pic;                     /* DS:996a */
unsigned patrol_button_pic;                     /* DS:9968 */
unsigned wait_button_pic;                 /* DS:9966 */
unsigned done_button_pic;                       /* DS:9964 */
unsigned build_button_pic;                      /* DS:9962 */
char bss_unused_9960[2];                        /* DS:9960  never referenced (a gap in the block) */
unsigned purify_button_pic;                     /* DS:995e */
unsigned meld_button_pic;                       /* DS:995c */
unsigned done_lock_pic;                 /* DS:995a */
unsigned patrol_lock_pic;               /* DS:9958 */
unsigned wait_lock_pic;                         /* DS:9956 */
unsigned special_order_blank_pic;              /* DS:9954 */
unsigned purify_lock_pic;                       /* DS:9952  MAIN 0x2b: "PURIFY" greyed (shown for every special) */
unsigned weapon_magic_pic;                      /* DS:9950 */
unsigned weapon_mithril_pic;                    /* DS:994e */
unsigned weapon_adamantium_pic;                 /* DS:994c */
unsigned medal_white_pic;                       /* DS:994a */
unsigned medal_gold_pic;                        /* DS:9948 */
unsigned medal_red_pic;                         /* DS:9946 */
unsigned city_change_button_pic;                /* DS:9944  BACKGRND 8: Change */
unsigned city_buy_button_pic;                   /* DS:9942  BACKGRND 7: Buy */
unsigned city_ok_button_pic;                    /* DS:9940  BACKGRND 9: OK */
unsigned city_buy_greyed_pic;                   /* DS:993e  BACKGRND 14: Buy greyed out */
unsigned prod_box_empty_pic;                        /* DS:993c */
unsigned prod_box_filled_pic;                        /* DS:993a */
unsigned city_no_product_pic;                   /* DS:9938  BACKGRND 13: blank panel over the product (Trade Goods / Housing) */
unsigned city_screen_bg_pic;                    /* DS:9936  BACKGRND 6: the city screen */
unsigned stack_slot_bg_pics[MAX_STACK];                 /* DS:9924 */
unsigned main_screen_bg_pic;                    /* DS:9922 */
int debug_alt_t_toggle;                         /* DS:9920  debug Alt+T flips it; never read */
unsigned lair_bottom_button_pic;                /* DS:991e */
unsigned lair_bottom_nobutton_pic;              /* DS:991c */
unsigned lair_confirm_border_pic;               /* DS:991a */
unsigned diplo_mirror_pic;                      /* DS:9918 */
unsigned special2_icons[33];                    /* DS:98d6  SPECIAL2.LBX */
unsigned special_icons[111];                    /* DS:97f8  SPECIAL.LBX */
unsigned unitview_stat_icons[15];               /* DS:97da */
unsigned unitview_stat_icons_gold[15];          /* DS:97bc */
unsigned unitview_build_bg_pic;                 /* DS:97ba */
unsigned unitview_bg_pic;                       /* DS:97b8 */
unsigned unitview_arrow_up_pic;                 /* DS:97b6 */
unsigned unitview_arrow_down_pic;               /* DS:97b4 */
unsigned unitview_row_frame_pics[8];            /* DS:97a4 */
unsigned gold_deficit_x1_pic;                   /* DS:97a2  BACKGRND 0x49 'black gold': the grey coin of one gold short */
unsigned gold_coin_pic;                         /* DS:97a0 */
unsigned unitview_portrait_frame_pic;           /* DS:979e */
unsigned unitview_water_bg_pic;                 /* DS:979c  UNITVIEW.LBX 0x22 */
unsigned diplo_face_pic;                        /* DS:979a  MOODWIZ entry = portrait: frame 0 good, 1 bad, 2 neutral */
unsigned item_icon_guardian_wind_pic;           /* DS:9798  ITEMISC 0x23 'item guardian wind': an item power icon, never drawn */
unsigned item_icon_giant_strength_pic;          /* DS:9796  ITEMISC 0x22 'item giant str': an item power icon, never drawn */
unsigned item_icon_dispel_evil_pic;             /* DS:9794  ITEMISC 0x21 'item dispel evil': a 16 x 16 item power icon, never drawn */
unsigned item_power_icons[19];                  /* DS:976e  ITEMISC 0..18 */
unsigned item_card_pic;                         /* DS:976c */
unsigned item_power_bullet_pic;                 /* DS:976a */
unsigned unitview_slot_icons[6];                /* DS:975e  by HiredHero.slot_type - 1 */
unsigned city_ench_up_arrow_pic;                /* DS:975c  BACKGRND.LBX 0x0f */
unsigned city_ench_down_arrow_pic;              /* DS:975a  BACKGRND.LBX 0x10 */
unsigned city_done_backdrop_pic;                /* DS:9758  BACKGRND 10: the grassland behind a completed building */
unsigned unitlist_bg_pic;                       /* DS:9756 */
unsigned unitlist_bottom_pic;                   /* DS:9754 */
int city_hover_control;                         /* DS:9752  control under the mouse (UI_Hit_Test) */
int map_cursor_cell_x;                          /* DS:9750 */
int map_cursor_cell_y;                          /* DS:974e */
int map_scroll_target_x;                                /* DS:974c */
int map_scroll_target_y;                                /* DS:974a */
int map_scroll_px_x;                            /* DS:9748 */
int map_scroll_px_y;                            /* DS:9746 */
int minimap_click_x;                            /* DS:9744 */
int minimap_click_y;                            /* DS:9742 */
int map_grid_control;                           /* DS:9740 */
int minimap_control;                            /* DS:973e */
unsigned combat_border_pic;                     /* DS:973c */
int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];                      /* DS:964c  unit, 1000 + city or 1100 + node per view square; -1 */
char bss_unused_964a[2];                        /* DS:964a  never referenced (a gap in the block) */
unsigned unit_figure_pic;                       /* DS:9648 */
char bss_unused_9646[2];                        /* DS:9646  never referenced (a gap in the block) */
int city_screen_city;                           /* DS:9644 */
char bss_unused_9600[68];                       /* DS:9600  never referenced (a gap in the block) */
unsigned items_hero_portraits[6];               /* DS:95f4 */
unsigned goto_marker_pic;                       /* DS:95f2 */
unsigned city_shared_tile_pic;                  /* DS:95f0  BACKGRND 0: the mark on a map square another city shares */
char bss_unused_95d0[32];                       /* DS:95d0  never referenced (a gap in the block) */
unsigned magicscr_bg_pic;                       /* DS:95ce  MAGIC.LBX 0 */
char bss_unused_95c4[10];                       /* DS:95c4  never referenced (a gap in the block) */
unsigned magic_overlay_pic;                     /* DS:95c2  MAGIC.LBX 51 */
unsigned magic_gem_frame_pic;                   /* DS:95c0  MAGIC.LBX 6 */
unsigned staff_mana_empty_pic;                  /* DS:95be  MAGIC.LBX 7 */
unsigned staff_mana_full_pic;                   /* DS:95bc  MAGIC.LBX 8 */
unsigned staff_research_empty_pic;              /* DS:95ba  MAGIC.LBX 9 */
unsigned staff_research_full_pic;               /* DS:95b8  MAGIC.LBX 10 */
unsigned staff_skill_empty_pic;                 /* DS:95b6  MAGIC.LBX 11 */
unsigned staff_skill_full_pic;                  /* DS:95b4  MAGIC.LBX 12 */
unsigned magic_alchemy_button_pic;              /* DS:95b2  MAGIC.LBX 13 */
unsigned magic_ok_button_pic;                   /* DS:95b0  MAGIC.LBX 14 */
unsigned staff_mana_locked_pic;                 /* DS:95ae  MAGIC.LBX 15 */
unsigned staff_research_locked_pic;             /* DS:95ac  MAGIC.LBX 16 */
unsigned staff_skill_locked_pic;                /* DS:95aa  MAGIC.LBX 17 */
unsigned magic_gem_pics[14];                    /* DS:958e */
unsigned items_bg_pic;                          /* DS:958c */
unsigned items_portrait_frame_pic;              /* DS:958a */
unsigned items_alchemy_pic;                     /* DS:9588 */
unsigned items_ok_pic;                          /* DS:9586 */
unsigned item_empty_slot_pics[6];               /* DS:957a  by HiredHero.slot_type - 1 */
unsigned armylist_bg_pic;                       /* DS:9578 */
unsigned armylist_up_arrow_pic;                 /* DS:9576 */
unsigned armylist_down_arrow_pic;               /* DS:9574 */
unsigned armylist_items_btn_pic;                /* DS:9572 */
unsigned armylist_ok_btn_pic;                   /* DS:9570 */
unsigned wizinfo_bg_pic;                        /* DS:956e */
unsigned wizinfo_book_pics[18];                 /* DS:954a  MAGIC.LBX 33..50: 3 per realm */
unsigned unitview_stone_frame_pic;                        /* DS:9548  UNITVIEW 0x1e: a teal stone frame, never drawn */
char bss_unused_9542[6];                        /* DS:9542  never referenced (a gap in the block) */
unsigned unitview_button_panel_pic;             /* DS:9540 */
unsigned outpost_bg_pic;                        /* DS:953e  BACKGRND.LBX 0x20 */
unsigned name_entry_box_pic;                    /* DS:953c  BACKGRND.LBX 0x21: the name-entry box (city, outpost and hero names) */
unsigned outpost_house_pics[3];                 /* DS:9536  BACKGRND.LBX 0x22..0x24, by race house style */
unsigned outpost_house_empty_pics[3];           /* DS:9530  BACKGRND.LBX 0x25..0x27, a growth house not yet reached */
unsigned deselect_bg_pic;                       /* DS:952e  MAIN.LBX 0x22 "deselect backgrnd" (the income panel) */
unsigned next_turn_bg_pic;                      /* DS:952c  MAIN.LBX 0x23 "next turn backgrnd" */
char bss_unused_952a[2];                        /* DS:952a  never referenced (a gap in the block) */
unsigned cast_bg_pic;                           /* DS:9528 */
unsigned main_cancel_button_pic;                /* DS:9526 */
unsigned cast_button_border_pic;                /* DS:9524 */
unsigned button_blockout_pic;                   /* DS:9522 */
char bss_unused_9520[2];                        /* DS:9520  never referenced (a gap in the block) */
unsigned road_ok_pic;                           /* DS:951e */
unsigned road_bg_pic;                           /* DS:951c */
unsigned road_button_border_pic;                /* DS:951a */
unsigned survey_bg_pic;                         /* DS:9518 */
unsigned next_turn_button_pic;                  /* DS:9516 */
unsigned citylist_background_pic;               /* DS:9514  RELOAD.LBX 0x15 */
unsigned citylist_up_arrow_pic;                 /* DS:9512  ARMYLIST.LBX 1 */
unsigned citylist_down_arrow_pic;               /* DS:9510  ARMYLIST.LBX 2 */
unsigned citylist_ok_btn_pic;                   /* DS:950e  RELOAD.LBX 0x16 */
unsigned city_worker_pics[14];                  /* DS:94f2 */
unsigned food_deficit_x1_pic;                  /* DS:94f0 */
unsigned resource_x1_pics[5];                   /* DS:94e6 */
unsigned city_farmer_pics[14];                  /* DS:94ca */
unsigned city_rebel_pics[14];                   /* DS:94ae */
unsigned food_deficit_x10_pic;                  /* DS:94ac */
unsigned resource_x10_pics[6];                  /* DS:94a0 */
unsigned cityview_bg_pic;                       /* DS:949e  RELOAD.LBX 0x1a 'ENEMYCIT': the City View window background (ReMoM _enemy_city_seg; clarified using ReMoM repo) */
unsigned cityprod_left_tab_pic;                 /* DS:949c */
unsigned cityprod_right_tab_pic;                /* DS:949a */
unsigned ok_button_pic;                         /* DS:9498 */
unsigned alchemy_bg_pic;                        /* DS:9496  MAGIC.LBX 52 */
unsigned alchemy_star_pic;                      /* DS:9494  MAGIC.LBX 58 */
char bss_unused_9490[4];                        /* DS:9490  never referenced (a gap in the block) */
unsigned alchemy_ok_pic;                        /* DS:948e  MAGIC.LBX 54 */
unsigned alchemy_cancel_pic;                    /* DS:948c  MAGIC.LBX 53 */
unsigned alchemy_left_arrow_pic;                /* DS:948a  MAGIC.LBX 56 */
unsigned alchemy_right_arrow_pic;               /* DS:9488  MAGIC.LBX 55 */
unsigned alchemy_arrow_bar_pic;                 /* DS:9486  MAGIC.LBX 57 */
unsigned alchemy_pow2gold_pic;                  /* DS:9484  MAGIC.LBX 59 */
int magic_lock_skill;                           /* DS:9482 */
int magic_lock_research;                        /* DS:9480 */
int magic_lock_mana;                            /* DS:947e */
char bss_unused_9472[12];                       /* DS:9472  never referenced (a gap in the block) */
unsigned combat_results_bottom_pic;             /* DS:9470 */
unsigned spellbook_small_pic;                   /* DS:946e */
unsigned cityspell_bg_pic;                      /* DS:946c */
unsigned spellbook_corner_left_pic;             /* DS:946a */
unsigned spellbook_corner_right_pic;            /* DS:9468 */
unsigned research_big_left_corner_pic;          /* DS:9466 */
unsigned research_big_right_corner_pic;         /* DS:9464 */
char bss_unused_9462[2];                        /* DS:9462  never referenced (a gap in the block) */
unsigned scroll_bg_pic;                         /* DS:9460  SCROLL.LBX 6 */
unsigned spellbook_realm_icon_src_pics[6];            /* DS:9454 */
unsigned spellbook_page_bg_pic;                 /* DS:9452 */
char bss_unused_944e[4];                        /* DS:944e  never referenced (a gap in the block) */
int combat_spellbook_page;                      /* DS:944c */
int overland_spellbook_page;                    /* DS:944a  the overland book's page kept between casts (SPELLCST.C); cleared by Game_Init_State */
int research_first_page;                        /* DS:9448 */
int stack_fresh_selection;                      /* DS:9446  1 after a new stack selection until the first unit click in the stack panel (that click drops the other active units) */
int no_units_to_move;                           /* DS:9444 */
int stack_blink_state;                          /* DS:9442  0..7 blinking, -1 steady, -2 hidden */
unsigned cityscape_rubble_pics[4];              /* DS:943a */
unsigned event_icon_pics[6];                    /* DS:942e  MAIN 0x3b..0x40 */
int never_set_flag_tile_count;                  /* DS:942c  the first 5 squares with TERRAIN_FLAG_NEVER_SET; nothing reads them */
char never_set_flag_tile_x[5];                          /* DS:9426 */
char never_set_flag_tile_y[5];                          /* DS:9421 */
char never_set_flag_tile_plane[5];                      /* DS:941c */
unsigned snd_message;                           /* DS:941a  SOUNDFX 0 */
char bss_unused_9416[4];                        /* DS:9416  never referenced (a gap in the block) */
unsigned snd_click;                             /* DS:9414  SOUNDFX 2 */
char bss_unused_9412[2];                        /* DS:9412  never referenced (a gap in the block) */
unsigned notify_realm_pics[5];                  /* DS:9408 */
char bss_unused_93fa[14];                       /* DS:93fa  never referenced (a gap in the block) */
unsigned combat_terrain_pics[48];               /* DS:939a */
unsigned char far *vis_bitmap_arcanus;          /* DS:9396 */
unsigned char far *vis_bitmap_myrror;           /* DS:9392 */
unsigned p_music_buf;                           /* DS:9390  segment of the music buffer */
struct Building far *p_building_data;           /* DS:938c */
/* the cityscape_myrror_* pictures: CITYSCAP.LBX labels them "chaos ..."; Cityscape_Draw_Background draws them for a
   Myrror city (the Arcanus ones for an Arcanus city) */
unsigned cityscape_myrror_ground_pic;            /* DS:938a */
unsigned cityscape_myrror_hills_pic;             /* DS:9388 */
unsigned cityscape_myrror_mountain_pic;          /* DS:9386 */
unsigned cityscape_myrror_plains_pic;            /* DS:9384 */
unsigned cityscape_myrror_heavenly_light_pic;             /* DS:9382 */
unsigned cityscape_myrror_dark_cloud_pic;        /* DS:9380 */
unsigned cityscape_myrror_chaos_rift_pic;              /* DS:937e */
unsigned cityscape_plain_bg_pic;                /* DS:937c */
unsigned cityscape_myrror_ocean_pic;             /* DS:937a */
unsigned cityscape_myrror_river_pic;             /* DS:9378 */
unsigned cityscape_arcanus_ground_pic;                     /* DS:9376 */
unsigned cityscape_flying_fortress_pic;         /* DS:9374 */
unsigned cityscape_gaia_pic;                    /* DS:9372 */
unsigned cityscape_living_lands_pic;            /* DS:9370 */
unsigned cityscape_cursed_land_pic;             /* DS:936e */
unsigned cityscape_river_pic;                   /* DS:936c */
unsigned cityscape_ocean_pic;                   /* DS:936a */
unsigned cityscape_hills_bg_pic;                /* DS:9368 */
unsigned cityscape_mountain_bg_pic;              /* DS:9366 */
unsigned cityscape_far_dark_cloud_pic;          /* DS:9364 */
unsigned cityscape_far_heavenly_light_pic;               /* DS:9362 */
unsigned cityscape_far_chaos_rift_pic;          /* DS:9360 */
unsigned cityscape_famine_pic;                  /* DS:935e */
unsigned cityscape_death_ward_pic;              /* DS:935c */
unsigned cityscape_chaos_ward_pic;              /* DS:935a */
unsigned cityscape_nature_ward_pic;             /* DS:9358 */
unsigned cityscape_life_ward_pic;               /* DS:9356 */
unsigned cityscape_sorcery_ward_pic;            /* DS:9354 */
unsigned cityscape_natures_eye_pic;             /* DS:9352 */
unsigned cityscape_inspirations_pic;            /* DS:9350 */
unsigned cityscape_prosperity_pic;              /* DS:934e */
unsigned cityscape_consecration_pic;            /* DS:934c */
unsigned cityscape_wall_of_fire_pic;            /* DS:934a */
char bss_unused_9348[2];                        /* DS:9348  never referenced (a gap in the block) */
unsigned cityscape_wall_of_darkness_pic;             /* DS:9346  drawn for Wall of Darkness */
unsigned cityscape_roads_pic;                   /* DS:9344 */
unsigned cityscape_house_pics[3][5];            /* DS:9326  by house style */
unsigned cityscape_tree_pics[3];                /* DS:9320 */
unsigned cityscape_rocks_pics[3];               /* DS:931a */
unsigned cityscape_summon_circle_pic;           /* DS:9318 */
unsigned cityscape_earth_gate_pic;              /* DS:9316 */
unsigned cityscape_stream_of_life_pic;          /* DS:9314 */
unsigned cityscape_astral_gate_pic;             /* DS:9312 */
unsigned cityscape_dark_rituals_pic;            /* DS:9310 */
unsigned cityscape_evil_presence_pic;           /* DS:930e */
unsigned cityscape_altar_pic;                   /* DS:930c */
unsigned cityscape_city_walls_pic;              /* DS:930a  CITYSCAP 0x4c 'city walls': the wall ring drawn round the cityscape */
unsigned cityscape_fortress_pic;                /* DS:9308 */
unsigned cityscape_housing_pics[3];             /* DS:9302  CITYSCAP.LBX 0x2a..0x2c, by race house style */
unsigned cityscape_trade_goods_pic;             /* DS:9300  CITYSCAP.LBX 0x29 */
unsigned building_pics[36];                     /* DS:92b8  CITYSCAP pictures by BuildingId */
unsigned unitview_land_bg_pic;                  /* DS:92b6  CITYSCAP.LBX 0x67 */
int cityscape_hotspot_count;                    /* DS:92b4 */
struct CityscapeHotspot far *p_cityscape_hotspots;/* DS:92b0 */
int ai_attacker_threat_realms;                  /* DS:92ae  the attacker's: never cleared - piles up (BUG #331) */
int ai_defender_threat_realms;                  /* DS:92ac  realm bits of the defender's castable hostile spells (never cleared) */
int ai_attacker_attack_profile[5];              /* DS:92a2 */
int ai_defender_attack_profile[5];              /* DS:9298 */
int ai_attacker_spell_threat;                   /* DS:9296  added to the AI's defensive spell scores when the enemy has
                                                   hostile realms (COMBAI.C, COMBCAST.C); never written, so always 0 */
int ai_defender_spell_threat;                   /* DS:9294  the same for the defender: never written, always 0 */
char bss_unused_9288[12];                       /* DS:9288  never referenced (a gap in the block) */
int combat_site_index;                                /* DS:9286  the city / lair attacked */
int combat_site;                             /* DS:9284  CombatAt: COMBAT_AT_UNITS / _CITY / _LAIR */
int combat_cursor_mode;                         /* DS:9282 */
int combat_target_x;                            /* DS:9280 */
int combat_target_y;                            /* DS:927e */
int combat_active_marker_on;                    /* DS:927c */
int combat_active_marker_x;                     /* DS:927a */
int combat_active_marker_y;                     /* DS:9278 */
struct Battlefield far *p_battlefield;          /* DS:9274 */
int combat_wall_rising;                       /* DS:9272 */
int combat_wall_anim_frame;                          /* DS:9270 */
unsigned combat_scan_icons[18];                 /* DS:924c */
int combat_ai_fled;                             /* DS:924a */
struct HeroStats far *p_hero_stats[6];          /* DS:9232  [player] the hero type table */
char far *p_combat_enchants;                    /* DS:922e */
struct BattleUnit far *p_battle_units;          /* DS:922a */
struct BattleUnit far *p_unit_stats_buf;        /* DS:9226 */
unsigned item_icon_pics[116];                   /* DS:913e  ITEMS.LBX 0..115 */
unsigned char far *p_items_in_game;             /* DS:913a */
struct Item far *p_items;                       /* DS:9136 */
unsigned music_jingle;                          /* DS:9134 */
unsigned p_sfx_buf;                             /* DS:9132  0xffff (-1): no sound */
unsigned p_anim_pic;                            /* DS:9130 */
struct Spell far *p_spell_data;                 /* DS:912c */
int time_stop_player;                           /* DS:912a  player + 1 of the Time Stop caster, 0 = none */
int *ai_city_own_value;                         /* DS:9128 */
int *ai_city_target_value;                      /* DS:9126 */
int *ai_city_garrison;                          /* DS:9124 */
int *ai_city_threat;                            /* DS:9122 */
unsigned char *p_ai_spell_groups;               /* DS:9120 */
int ai_known_spell_count;                       /* DS:911e */
unsigned char *ai_known_spells;                 /* DS:911c */
int ai_hostile_count;                           /* DS:911a */
int ai_hostile_wizards[6];                      /* DS:910e */
int ai_own_stack_count;                         /* DS:910c */
int ai_enemy_stack_count;                       /* DS:910a */
struct AIStack *p_ai_own_stacks;                    /* DS:9108 */
struct AIEnemyStack *p_ai_enemy_stacks;         /* DS:9106 */
unsigned *ai_lm_own_strength[2];                /* DS:9102 */
unsigned *ai_lm_enemy_strength[2];              /* DS:90fe */
int *ai_lm_own_value[2];                        /* DS:90fa */
int *ai_lm_target_value[2];                     /* DS:90f6 */
int *ai_lm_surplus[2];                          /* DS:90f2 */
int ai_transport_req_x[AI_MAX_TRANSPORT_REQS];                     /* DS:90d4 */
int ai_transport_req_y[AI_MAX_TRANSPORT_REQS];                     /* DS:90b6 */
int ai_transport_req_plane[AI_MAX_TRANSPORT_REQS];                 /* DS:9098 */
int ai_transport_req_count;                     /* DS:9096 */
int ai_own_transport_count;                     /* DS:9094 */
int ai_target_count;                            /* DS:9092 */
char bss_unused_9090[2];                        /* DS:9090  never referenced (a gap in the block) */
int ai_target_x[25];                            /* DS:905e */
int ai_target_y[25];                            /* DS:902c */
int ai_target_defence[25];                      /* DS:8ffa */
int ai_target_value[25];                        /* DS:8fc8 */
int ai_turn_unused_8fc6;                        /* DS:8fc6  as ai_turn_unused_8fc4 */
int ai_turn_unused_8fc4;                        /* DS:8fc4  cleared each AI turn, never read */
char ai_landmass_claimed_bits[2][8];            /* DS:8fb4  per plane, a bit per landmass; never written (all 0), read in AIORDERS.C */
int *p_ai_attack_landmass[2];                   /* DS:8fb0 */
int *p_ai_expand_landmass[2];                   /* DS:8fac */
int *p_ai_replan_timer;                         /* DS:8faa  per player */
int *p_ai_unused_table_1[2];                    /* DS:8fa6  per plane, 10 ints in the neutral player's spell_library (+0x28);
                                                   set up by GAMEDATA.C, never read or written through */
int *p_ai_unused_table_2[2];                    /* DS:8fa2  the same at +0x50; never used */
int *p_ai_expand_x[2];                          /* DS:8f9e */
int *p_ai_expand_y[2];                          /* DS:8f9a */
int *p_ai_circle_timer;                         /* DS:8f98 */
struct LandmassPlan ai_landmass_plan[2][5];     /* DS:8890 */

/* the game tables, DS:00aa .. 2081, in address order, from the game's bytes (work/reports/gen_wizards_globals.py +
   wizards_data_layout.py); their strings follow at 2082 .. 2880, merged by -d as in the original */
unsigned char banner_shade_colors[6][5] = {    /* DS:00aa */
    { 105, 106, 107, 108, 109 }, { 73, 74, 75, 76, 77 }, { 121, 122, 123, 124, 125 }, { 201, 165, 203, 166, 45 },
    { 160, 161, 162, 178, 180 }, { 48, 49, 50, 51, 52 }
};
unsigned char banner_text_colors[38] = {    /* DS:00c8 */
    98, 216, 123, 43, 179, 50, 201, 0, 165, 0, 203, 0, 121, 0, 122, 0, 123, 0, 13, 0, 14, 0, 15, 0, 73, 0, 74, 0, 75,
    0, 105, 0, 106, 0, 107, 0, 0, 0
};
struct MouseRegion mouse_region_arrow = { 1, 0, 0, 0, 319, 199 };    /* DS:00ee */
struct MouseRegion mouse_region_hidden = { 0, 0, 0, 0, 319, 199 };    /* DS:00fa */
struct MouseRegion mouse_region_busy = { 6, 0, 0, 0, 319, 199 };    /* DS:0106 */
struct MouseRegion mouse_region_arrow_b = {    /* DS:0112  no reader (as in MAGIC.C) */
    1, 0, 0, 0, 319, 199
};
struct MouseRegion mouse_region_image7 = {    /* DS:011e  no reader (as in MAGIC.C) */
    7, 4, 0, 0, 319, 158
};
static unsigned char bit_masks6[6] = {    /* DS:012a  no reader; static (UTIL.C: bit_masks) */
    1, 2, 4, 8, 16, 32
};
char *retort_names[18] = {    /* DS:0130 */
    "Alchemy", "Warlord", "Chaos Mastery", "Nature Mastery", "Sorcery Mastery", "Infernal Power", "Divine Power",
    "Sage Master", "Channeler", "Myrran", "Archmage", "Mana Focusing", "Node Mastery", "Famous", "Runemaster",
    "Conjurer", "Charismatic", "Artificer"
};
int random_events_enabled = 1;    /* DS:0154  (MAGIC.C has the same variable at the same place after retort_names) */
char *city_size_names[6] = {    /* DS:0156  Outpost .. Capital */
    "Outpost", "Hamlet", "Village", "Town", "City", "Capital"
};
char *cityscape_special_names[7] = {    /* DS:0162 */
    "Summoning Circle", "Earth Gate", "Stream Of Life", "Astral Gate", "Fortress", "Dark Rituals",
    "Altar of Battle"
};
int unused_pair_table[22] = {    /* DS:0170  no reader found (MAGIC.C has the same table): 11 pairs, the first
                                       falling 140 .. 40 by 10, the second rising 0 .. 50 by 5 */
    140, 0, 130, 5, 120, 10, 110, 15, 100, 20, 90, 25, 80, 30, 70, 35, 60, 40, 50, 45, 40, 50
};
/* columns (struct UnitType): name, melee, ranged, ranged_type, ammo, to_hit, defense, resist, moves, cost,
   upkeep, race, req_building_1, req_building_2, map_figure_pic, hits, scouting, transport_capacity, figures,
   construction, special_strength, move_flags, unused_19, abilities, abilities2, ability_flags, attack_flags,
   sounds, pad_23 */
struct UnitType unit_types[198] = {    /* DS:019c */
    { "Dwarf", 5, 0, RANGED_NONE, 0, 0, 4, 10, 4, 100, 2, RACE_DWARVEN, 10, 0, 0x0, 10, 1, 0, 1, 0, 0, MOVE_CAVALRY | MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Barbarian", 5, 3, RANGED_THROWN, 0, 0, 3, 6, 4, 100, 2, RACE_BARBARIAN, 17, 0, 0x1, 9, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Sage", 1, 6, RANGED_MAGIC_ILLUSION, 0, 0, 4, 6, 4, 100, 2, RACE_HIGH_MEN, 28, 3, 0x2, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Dervish", 4, 4, RANGED_ARROW, 8, 0, 4, 6, 4, 100, 2, RACE_HIGH_MEN, 2, 1, 0x3, 6, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Beastmaster", 5, 0, RANGED_NONE, 0, 0, 4, 6, 4, 100, 2, RACE_BEASTMEN, 18, 0, 0x4, 7, 3, 0, 1, 0, 0, MOVE_CAVALRY | MOVE_FORESTER, 0, 0, 0, 0, 0, 0, 0 },
    { "Bard", 4, 0, RANGED_NONE, 0, 0, 5, 6, 4, 100, 2, RACE_HIGH_MEN, 22, 0, 0x5, 6, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Orc Warrior", 6, 3, RANGED_THROWN, 0, 0, 4, 6, 4, 100, 2, RACE_ORC, 6, 0, 0x6, 8, 1, 0, 1, 0, 0, MOVE_CAVALRY | MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Healer", 3, 6, RANGED_MAGIC_HOLY, 0, 0, 5, 7, 4, 100, 2, RACE_HIGH_MEN, 30, 0, 0x7, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, AFLAG_HEALER, 0, 0, 0 },
    { "Huntress", 5, 4, RANGED_ARROW, 8, 0, 3, 6, 4, 100, 2, RACE_HIGH_MEN, 31, 1, 0x8, 7, 1, 0, 1, 0, 0, MOVE_FORESTER | MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Thief", 5, 0, RANGED_NONE, 0, 0, 5, 6, 4, 100, 2, RACE_HIGH_MEN, 32, 0, 0x9, 7, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Druid", 1, 8, RANGED_MAGIC_NATURE, 0, 0, 5, 6, 4, 150, 3, RACE_HIGH_MEN, 12, 3, 0xa, 5, 3, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, AFLAG_PURIFY, 0, 0, 0 },
    { "War Monk", 6, 0, RANGED_NONE, 0, 0, 5, 6, 4, 150, 3, RACE_HIGH_MEN, 20, 0, 0xb, 6, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Warrior Mage", 4, 4, RANGED_MAGIC_FIRE, 0, 0, 4, 6, 4, 150, 3, RACE_HIGH_MEN, 26, 2, 0xc, 7, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Magician", 1, 8, RANGED_MAGIC_FIRE, 0, 0, 5, 10, 4, 150, 3, RACE_HIGH_MEN, 15, 3, 0xd, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY, 0, 0, 0, 0, 0 },
    { "Assassin", 3, 0, RANGED_NONE, 0, 0, 5, 6, 4, 150, 3, RACE_HIGH_MEN, 33, 0, 0xe, 6, 1, 0, 1, 0, 5, MOVE_CAVALRY, 0, 0, 0, 0, ATTACK_POISON, 0, 0 },
    { "Wind Mage", 1, 6, RANGED_MAGIC_WIND, 0, 0, 5, 6, 4, 200, 4, RACE_HIGH_MEN, 19, 3, 0xf, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY, 0, AFLAG_WIND_WALKING, 0, 0, 0 },
    { "Ranger", 6, 5, RANGED_ARROW, 8, 0, 5, 6, 4, 200, 4, RACE_HIGH_MEN, 25, 1, 0x10, 8, 2, 0, 1, 0, 0, MOVE_CAVALRY | MOVE_FORESTER | MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Draconian", 7, 5, RANGED_FIRE_BREATH, 0, 0, 5, 6, 6, 200, 4, RACE_DRACONIAN, 1, 0, 0x11, 8, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, 0, 0, 0 },
    { "Witch", 1, 8, RANGED_MAGIC_FIRE, 0, 0, 5, 6, 4, 200, 4, RACE_HIGH_MEN, 3, 3, 0x12, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY, 0, 0, 0, 0, 0 },
    { "Golden One", 6, 6, RANGED_MAGIC_FIRE, 0, 0, 6, 6, 4, 200, 4, RACE_HIGH_MEN, 34, 0, 0x13, 6, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Ninja", 6, 0, RANGED_NONE, 0, 0, 5, 6, 4, 300, 6, RACE_HIGH_MEN, 29, 0, 0x14, 7, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, AFLAG_INVISIBILITY, 0, 0, 0 },
    { "Rogue", 7, 0, RANGED_NONE, 0, 0, 5, 6, 4, 300, 6, RACE_HIGH_MEN, 8, 0, 0x15, 8, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Amazon", 7, 4, RANGED_THROWN, 0, 0, 4, 6, 4, 300, 6, RACE_HIGH_MEN, 13, 0, 0x16, 8, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Warlock", 1, 8, RANGED_MAGIC_LIGHTNING, 0, 0, 5, 10, 4, 300, 6, RACE_HIGH_MEN, 21, 3, 0x17, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY, 0, 0, 0, 0, 0 },
    { "Unknown", 5, 5, RANGED_MAGIC_FIRE, 0, 0, 4, 10, 4, 300, 6, RACE_HIGH_MEN, 5, 2, 0x18, 8, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Illusionist", 1, 5, RANGED_MAGIC_ILLUSION, 0, 0, 4, 6, 4, 500, 10, RACE_HIGH_MEN, 24, 3, 0x19, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY, 0, 0, ATTACK_ILLUSION, 0, 0 },
    { "Swordsman", 6, 0, RANGED_NONE, 0, 0, 5, 6, 4, 500, 10, RACE_HIGH_MEN, 7, 0, 0x1a, 10, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Priestess", 2, 8, RANGED_MAGIC_HOLY, 0, 0, 5, 6, 4, 500, 10, RACE_HIGH_MEN, 23, 3, 0x1b, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Paladin", 9, 0, RANGED_NONE, 0, 0, 5, 6, 4, 500, 10, RACE_HIGH_MEN, 14, 0, 0x1c, 8, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MAGIC_IMMUNITY, 0, AFLAG_HEALER, ATTACK_ARMOR_PIERCING | ATTACK_FIRST_STRIKE, 0, 0 },
    { "Black Knight", 9, 0, RANGED_NONE, 0, 0, 5, 6, 4, 500, 10, RACE_HIGH_MEN, 16, 0, 0x1d, 10, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MAGIC_IMMUNITY, 0, 0, ATTACK_ARMOR_PIERCING | ATTACK_FIRST_STRIKE, 0, 0 },
    { "Elven Archer", 5, 8, RANGED_ARROW, 8, 0, 6, 6, 4, 500, 10, RACE_HIGH_ELF, 27, 1, 0x1e, 6, 1, 0, 1, 0, 0, MOVE_CAVALRY | MOVE_FORESTER, 0, 0, 0, 0, 0, 0, 0 },
    { "Knight", 8, 0, RANGED_NONE, 0, 0, 5, 6, 4, 500, 10, RACE_HIGH_MEN, 9, 0, 0x1f, 9, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Necromancer", 1, 7, RANGED_MAGIC_DEATH, 0, 0, 5, 6, 4, 500, 10, RACE_HIGH_MEN, 11, 3, 0x20, 5, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY, 0, 0, ATTACK_LIFE_STEAL, 0, 0 },
    { "Chaos Warrior", 8, 8, RANGED_MAGIC_LIGHTNING, 0, 0, 5, 9, 4, 500, 10, RACE_HIGH_MEN, 4, 2, 0x21, 8, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, ATTACK_ARMOR_PIERCING, 0, 0 },
    { "Chosen", 12, 0, RANGED_NONE, 0, 0, 8, 12, 4, 600, 0, RACE_LIFE_CREATURE, 0, 0, 0x27, 12, 1, 0, 1, 0, 0, MOVE_CAVALRY, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_MAGIC_IMMUNITY, 0, 0, 0, 0, 0 },
    { "Trireme", 6, 0, RANGED_NONE, 0, 0, 4, 4, 4, 60, 2, RACE_GENERIC, 12, 0, 0x0, 10, 1, 2, 1, 0, 0, MOVE_SAILING, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Galley", 8, 2, RANGED_ARROW, 8, 0, 4, 6, 6, 100, 2, RACE_GENERIC, 13, 0, 0x0, 20, 1, 5, 1, 0, 0, MOVE_SAILING, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Catapult", 0, 10, RANGED_BOULDER, 10, 0, 2, 4, 2, 100, 2, RACE_GENERIC, 33, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER | AFLAG_LONG_RANGE, 0, 0, 0 },
    { "Warship", 10, 10, RANGED_BOULDER, 99, 0, 5, 7, 8, 160, 2, RACE_GENERIC, 14, 3, 0x0, 30, 1, 3, 1, 0, 0, MOVE_SAILING, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LONG_RANGE, 0, 0, 0 },
    { "Spearmen", 1, 1, RANGED_THROWN, 0, 0, 2, 5, 2, 15, 1, RACE_BARBARIAN, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 1, RANGED_THROWN, 0, 0, 2, 5, 2, 30, 1, RACE_BARBARIAN, 8, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Bowmen", 1, 1, RANGED_ARROW, 8, 0, 1, 5, 2, 30, 1, RACE_BARBARIAN, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Cavalry", 4, 1, RANGED_THROWN, 0, 0, 2, 5, 4, 60, 1, RACE_BARBARIAN, 9, 3, 0x0, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY, 0, ABILITY_RACE_PREFIX, 0, 0, ATTACK_FIRST_STRIKE, 0, 0 },
    { "Shaman", 2, 2, RANGED_MAGIC_HOLY, 4, 0, 3, 7, 2, 50, 1, RACE_BARBARIAN, 22, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 5, 2, 60, 2, RACE_BARBARIAN, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Berserkers", 7, 3, RANGED_THROWN, 0, 0, 3, 7, 2, 120, 2, RACE_BARBARIAN, 6, 0, 0x0, 3, 1, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "Spearmen", 2, 0, RANGED_NONE, 0, 0, 2, 5, 2, 20, 1, RACE_BEASTMEN, 0, 0, 0x0, 2, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 4, 0, RANGED_NONE, 0, 0, 2, 5, 2, 40, 1, RACE_BEASTMEN, 3, 8, 0x0, 2, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 5, 0, RANGED_NONE, 0, 0, 3, 5, 2, 80, 2, RACE_BEASTMEN, 4, 0, 0x0, 2, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Bowmen", 2, 1, RANGED_ARROW, 8, 0, 1, 5, 2, 60, 2, RACE_BEASTMEN, 15, 3, 0x0, 2, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Priests", 4, 4, RANGED_MAGIC_HOLY, 4, 0, 4, 8, 2, 150, 3, RACE_BEASTMEN, 24, 0, 0x0, 2, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, ABILITY2_HEALING_SPELL, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Magicians", 2, 5, RANGED_MAGIC_FIRE, 4, 0, 3, 9, 2, 180, 4, RACE_BEASTMEN, 21, 0, 0x0, 2, 1, 0, 4, 0, 0, 0, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_RACE_PREFIX, ABILITY2_FIRE_BALL_SPELL, 0, 0, 0, 0 },
    { "Engineers", 2, 0, RANGED_NONE, 0, 0, 1, 5, 2, 60, 2, RACE_BEASTMEN, 32, 0, 0x0, 2, 1, 0, 6, 1, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Settlers", 1, 0, RANGED_NONE, 0, 0, 1, 5, 2, 120, 4, RACE_BEASTMEN, 0, 0, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Centaurs", 3, 2, RANGED_ARROW, 6, 0, 3, 5, 4, 100, 2, RACE_BEASTMEN, 9, 3, 0x0, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Manticores", 5, 0, RANGED_NONE, 0, 0, 3, 6, 4, 160, 3, RACE_BEASTMEN, 10, 3, 0x0, 7, 2, 0, 2, 0, 6, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, ATTACK_POISON, 0, 0 },
    { "Minotaurs", 12, 0, RANGED_NONE, 0, 2, 4, 7, 2, 200, 4, RACE_BEASTMEN, 6, 0, 0x0, 12, 1, 0, 2, 0, 0, 0, 0, 0, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Spearmen", 1, 1, RANGED_MAGIC_DARK, 4, 0, 2, 7, 2, 25, 1, RACE_DARK_ELF, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 1, RANGED_MAGIC_DARK, 4, 0, 2, 7, 2, 50, 1, RACE_DARK_ELF, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 1, RANGED_MAGIC_DARK, 4, 0, 3, 7, 2, 100, 2, RACE_DARK_ELF, 4, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Cavalry", 4, 1, RANGED_MAGIC_DARK, 4, 0, 2, 7, 4, 100, 2, RACE_DARK_ELF, 3, 9, 0x0, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY, 0, ABILITY_RACE_PREFIX, 0, 0, ATTACK_FIRST_STRIKE, 0, 0 },
    { "Priests", 3, 6, RANGED_MAGIC_HOLY, 4, 0, 4, 10, 2, 200, 2, RACE_DARK_ELF, 24, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, ABILITY2_HEALING_SPELL, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 7, 2, 150, 3, RACE_DARK_ELF, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Nightblades", 4, 0, RANGED_NONE, 0, 0, 3, 7, 2, 120, 3, RACE_DARK_ELF, 5, 0, 0x0, 1, 1, 0, 6, 0, 1, 0, 0, 0, 0, AFLAG_INVISIBILITY, ATTACK_POISON, 0, 0 },
    { "Warlocks", 1, 7, RANGED_MAGIC_FIRE, 4, 0, 4, 9, 2, 240, 4, RACE_DARK_ELF, 21, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_MISSILES_IMMUNITY, ABILITY2_DOOMBOLT_SPELL, 0, 0, 0, 0 },
    { "Nightmares", 8, 5, RANGED_MAGIC_DARK, 4, 0, 4, 8, 6, 160, 4, RACE_DARK_ELF, 11, 3, 0x0, 10, 2, 0, 2, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, 0, 0, 0 },
    { "Spearmen", 1, 1, RANGED_FIRE_BREATH, 0, 0, 3, 6, 4, 25, 1, RACE_DRACONIAN, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 1, RANGED_FIRE_BREATH, 0, 0, 3, 6, 4, 50, 1, RACE_DRACONIAN, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 1, RANGED_FIRE_BREATH, 0, 0, 4, 6, 4, 100, 2, RACE_DRACONIAN, 4, 0, 0x0, 1, 1, 0, 6, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Bowmen", 1, 1, RANGED_ARROW, 8, 0, 2, 6, 4, 45, 2, RACE_DRACONIAN, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Shaman", 2, 2, RANGED_MAGIC_HOLY, 4, 0, 4, 8, 4, 75, 2, RACE_DRACONIAN, 22, 0, 0x0, 1, 1, 0, 4, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Magicians", 1, 5, RANGED_MAGIC_FIRE, 4, 0, 4, 10, 4, 180, 3, RACE_DRACONIAN, 21, 0, 0x0, 1, 1, 0, 4, 0, 0, MOVE_FLYING, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, ABILITY2_FIRE_BALL_SPELL, 0, 0, 0, 0 },
    { "Engineers", 1, 1, RANGED_FIRE_BREATH, 0, 0, 2, 6, 4, 100, 2, RACE_DRACONIAN, 10, 33, 0x0, 1, 1, 0, 6, 1, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 2, 6, 2, 150, 3, RACE_DRACONIAN, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM | ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Doom Drakes", 8, 6, RANGED_FIRE_BREATH, 0, 0, 3, 9, 6, 160, 3, RACE_DRACONIAN, 9, 3, 0x0, 10, 2, 0, 2, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, 0, 0, 0 },
    { "Air Ship", 5, 10, RANGED_BOULDER, 10, 0, 5, 8, 8, 200, 4, RACE_DRACONIAN, 13, 0, 0x0, 20, 2, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 0, 2, 8, 2, 50, 2, RACE_DWARVEN, 3, 8, 0x0, 3, 1, 0, 6, 0, 0, MOVE_MOUNTAINEER, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 0, RANGED_NONE, 0, 0, 3, 8, 2, 100, 2, RACE_DWARVEN, 4, 0, 0x0, 3, 1, 0, 6, 0, 0, MOVE_MOUNTAINEER, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Engineers", 1, 0, RANGED_NONE, 0, 0, 1, 8, 2, 40, 2, RACE_DWARVEN, 32, 0, 0x0, 3, 1, 0, 6, 2, 0, MOVE_MOUNTAINEER, 0, ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Hammerhands", 8, 0, RANGED_NONE, 0, 0, 4, 9, 2, 160, 4, RACE_DWARVEN, 5, 0, 0x0, 4, 1, 0, 6, 0, 0, MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Steam Cannon", 0, 12, RANGED_BOULDER, 10, 0, 2, 9, 2, 180, 4, RACE_DWARVEN, 34, 3, 0x0, 12, 1, 0, 1, 0, 0, MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Golem", 12, 0, RANGED_NONE, 0, 0, 8, 15, 2, 200, 4, RACE_DWARVEN, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, 0, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 8, 2, 150, 2, RACE_DWARVEN, 0, 0, 0x0, 30, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Spearmen", 3, 0, RANGED_NONE, 0, 0, 2, 4, 2, 10, 1, RACE_GNOLL, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 5, 0, RANGED_NONE, 0, 0, 2, 4, 2, 20, 1, RACE_GNOLL, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 6, 0, RANGED_NONE, 0, 0, 3, 4, 2, 40, 1, RACE_GNOLL, 4, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_NEGATE_FIRST_STRIKE, 0, 0, 0 },
    { "Bowmen", 3, 1, RANGED_ARROW, 8, 0, 1, 4, 2, 30, 1, RACE_GNOLL, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Settlers", 2, 0, RANGED_NONE, 0, 0, 1, 4, 2, 60, 2, RACE_GNOLL, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Wolf Riders", 7, 0, RANGED_NONE, 0, 0, 3, 4, 6, 100, 2, RACE_GNOLL, 9, 3, 0x0, 5, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 0, 2, 6, 2, 15, 1, RACE_HALFLING, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_LUCKY | ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 2, 0, RANGED_NONE, 0, 0, 2, 6, 2, 30, 1, RACE_HALFLING, 3, 8, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_LUCKY | ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Bowmen", 1, 1, RANGED_ARROW, 8, 0, 1, 6, 2, 45, 1, RACE_HALFLING, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_LUCKY | ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Shaman", 1, 2, RANGED_MAGIC_HOLY, 4, 0, 3, 8, 2, 75, 1, RACE_HALFLING, 22, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_LUCKY | ABILITY_RACE_PREFIX, 0, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 6, 2, 90, 1, RACE_HALFLING, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_LUCKY | ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Slingers", 1, 2, RANGED_SLING, 6, 0, 2, 6, 2, 100, 1, RACE_HALFLING, 4, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_LUCKY, 0, 0, 0, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 1, 2, 6, 2, 15, 1, RACE_HIGH_ELF, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, MOVE_FORESTER, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 1, 2, 6, 2, 30, 1, RACE_HIGH_ELF, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, MOVE_FORESTER, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 0, RANGED_NONE, 0, 1, 3, 6, 2, 60, 1, RACE_HIGH_ELF, 4, 0, 0x0, 1, 1, 0, 6, 0, 0, MOVE_FORESTER, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Cavalry", 4, 0, RANGED_NONE, 0, 1, 2, 6, 4, 60, 1, RACE_HIGH_ELF, 3, 9, 0x1, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY | MOVE_FORESTER, 0, ABILITY_RACE_PREFIX, 0, 0, ATTACK_FIRST_STRIKE, 0, 0 },
    { "Magicians", 1, 5, RANGED_MAGIC_FIRE, 4, 1, 3, 10, 4, 180, 2, RACE_HIGH_ELF, 21, 0, 0x0, 1, 1, 0, 4, 0, 0, MOVE_FORESTER, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_RACE_PREFIX, ABILITY2_FIRE_BALL_SPELL, 0, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 1, 1, 6, 2, 90, 2, RACE_HIGH_ELF, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, MOVE_FORESTER, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Longbowmen", 1, 3, RANGED_ARROW, 8, 1, 2, 6, 2, 80, 2, RACE_HIGH_ELF, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, MOVE_FORESTER, 0, 0, 0, 0, 0, 0, 0 },
    { "Elven Lords", 5, 0, RANGED_NONE, 0, 2, 4, 9, 4, 160, 3, RACE_HIGH_ELF, 6, 0, 0x0, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY | MOVE_FORESTER, 0, 0, 0, 0, ATTACK_ARMOR_PIERCING | ATTACK_FIRST_STRIKE, 0, 0 },
    { "Pegasai", 5, 3, RANGED_ARROW, 8, 1, 4, 8, 6, 160, 3, RACE_HIGH_ELF, 11, 3, 0x0, 5, 2, 0, 2, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, 0, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 0, 2, 4, 2, 10, 1, RACE_HIGH_MEN, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 0, 2, 4, 2, 20, 1, RACE_HIGH_MEN, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Bowmen", 1, 1, RANGED_ARROW, 8, 0, 1, 4, 2, 30, 1, RACE_HIGH_MEN, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Cavalry", 4, 0, RANGED_NONE, 0, 0, 2, 4, 4, 40, 1, RACE_HIGH_MEN, 3, 9, 0x0, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY, 0, ABILITY_RACE_PREFIX, 0, 0, ATTACK_FIRST_STRIKE, 0, 0 },
    { "Priests", 3, 4, RANGED_MAGIC_HOLY, 4, 0, 4, 7, 2, 100, 1, RACE_HIGH_MEN, 24, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, ABILITY2_HEALING_SPELL, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Magicians", 1, 5, RANGED_MAGIC_FIRE, 4, 0, 3, 8, 2, 120, 2, RACE_HIGH_MEN, 21, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_RACE_PREFIX, ABILITY2_FIRE_BALL_SPELL, 0, 0, 0, 0 },
    { "Engineers", 1, 0, RANGED_NONE, 0, 0, 1, 4, 2, 40, 1, RACE_HIGH_MEN, 32, 0, 0x0, 1, 1, 0, 6, 1, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 4, 2, 60, 2, RACE_HIGH_MEN, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Pikemen", 5, 0, RANGED_NONE, 0, 0, 3, 5, 2, 80, 1, RACE_HIGH_MEN, 5, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, 0, 0, AFLAG_NEGATE_FIRST_STRIKE, ATTACK_ARMOR_PIERCING, 0, 0 },
    { "Paladins", 6, 0, RANGED_NONE, 0, 0, 5, 8, 4, 200, 3, RACE_HIGH_MEN, 25, 6, 0x0, 4, 1, 0, 4, 0, 1, MOVE_CAVALRY, 0, ABILITY_MAGIC_IMMUNITY, ABILITY2_HOLY_BONUS, 0, ATTACK_ARMOR_PIERCING | ATTACK_FIRST_STRIKE, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 0, 4, 5, 2, 20, 1, RACE_KLACKON, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 0, 4, 5, 2, 40, 1, RACE_KLACKON, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 0, RANGED_NONE, 0, 0, 5, 5, 2, 80, 1, RACE_KLACKON, 4, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Engineers", 1, 0, RANGED_NONE, 0, 0, 1, 5, 2, 80, 1, RACE_KLACKON, 32, 0, 0x0, 1, 1, 0, 6, 1, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 5, 2, 120, 2, RACE_KLACKON, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Stag Beetle", 15, 5, RANGED_FIRE_BREATH, 0, 0, 7, 6, 4, 160, 2, RACE_KLACKON, 9, 6, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 0, 3, 4, 2, 10, 1, RACE_LIZARDMAN, 0, 0, 0x0, 2, 1, 0, 8, 0, 0, MOVE_SWIMMING, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 0, 3, 4, 2, 20, 1, RACE_LIZARDMAN, 3, 8, 0x0, 2, 1, 0, 6, 0, 0, MOVE_SWIMMING, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 0, RANGED_NONE, 0, 0, 4, 4, 2, 40, 1, RACE_LIZARDMAN, 4, 0, 0x0, 2, 1, 0, 6, 0, 0, MOVE_SWIMMING, 0, ABILITY_RACE_PREFIX, 0, AFLAG_NEGATE_FIRST_STRIKE, 0, 0, 0 },
    { "Javelineers", 4, 3, RANGED_ARROW, 6, 0, 4, 5, 2, 80, 2, RACE_LIZARDMAN, 5, 0, 0x0, 2, 1, 0, 6, 0, 0, MOVE_SWIMMING, 0, 0, 0, 0, 0, 0, 0 },
    { "Shaman", 2, 2, RANGED_MAGIC_HOLY, 4, 0, 3, 6, 2, 60, 2, RACE_LIZARDMAN, 22, 0, 0x0, 2, 1, 0, 4, 0, 0, MOVE_SWIMMING, 0, ABILITY_RACE_PREFIX, 0, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 2, 4, 2, 120, 2, RACE_LIZARDMAN, 0, 0, 0x0, 20, 1, 0, 1, 0, 0, MOVE_SWIMMING, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Dragon Turtle", 10, 5, RANGED_FIRE_BREATH, 0, 0, 8, 8, 4, 100, 1, RACE_LIZARDMAN, 9, 6, 0x0, 15, 1, 0, 1, 0, 0, MOVE_SWIMMING, 0, 0, 0, 0, 0, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 0, 2, 4, 2, 10, 1, RACE_NOMAD, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 0, 2, 4, 2, 20, 1, RACE_NOMAD, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Bowmen", 1, 1, RANGED_ARROW, 8, 0, 1, 4, 2, 30, 1, RACE_NOMAD, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Priests", 3, 4, RANGED_MAGIC_HOLY, 4, 0, 4, 7, 2, 100, 1, RACE_NOMAD, 24, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, ABILITY2_HEALING_SPELL, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Magicians", 1, 5, RANGED_MAGIC_FIRE, 4, 0, 3, 8, 2, 120, 2, RACE_NOMAD, 21, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_RACE_PREFIX, ABILITY2_FIRE_BALL_SPELL, 0, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 4, 2, 60, 2, RACE_NOMAD, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Horsebowmen", 4, 2, RANGED_ARROW, 8, 0, 2, 4, 4, 60, 1, RACE_NOMAD, 9, 3, 0x0, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY, 0, 0, 0, 0, 0, 0, 0 },
    { "Pikemen", 5, 0, RANGED_NONE, 0, 0, 3, 5, 2, 80, 3, RACE_NOMAD, 5, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, 0, 0, AFLAG_NEGATE_FIRST_STRIKE, ATTACK_ARMOR_PIERCING, 0, 0 },
    { "Rangers", 4, 3, RANGED_ARROW, 8, 0, 4, 6, 4, 120, 2, RACE_NOMAD, 10, 3, 0x0, 2, 1, 0, 4, 0, 0, MOVE_FORESTER | MOVE_MOUNTAINEER, 0, 0, 0, 0, 0, 0, 0 },
    { "Griffins", 9, 0, RANGED_NONE, 0, 0, 5, 7, 4, 200, 2, RACE_NOMAD, 11, 3, 0x0, 10, 1, 0, 2, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, ATTACK_ARMOR_PIERCING | ATTACK_FIRST_STRIKE, 0, 0 },
    { "Spearmen", 1, 0, RANGED_NONE, 0, 0, 2, 4, 2, 10, 1, RACE_ORC, 0, 0, 0x0, 1, 1, 0, 8, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Swordsmen", 3, 0, RANGED_NONE, 0, 0, 2, 4, 2, 20, 1, RACE_ORC, 3, 8, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD, 0, 0, 0 },
    { "Halberdiers", 4, 0, RANGED_NONE, 0, 0, 3, 4, 2, 40, 1, RACE_ORC, 4, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Bowmen", 1, 1, RANGED_ARROW, 8, 0, 1, 4, 2, 30, 1, RACE_ORC, 15, 3, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Cavalry", 4, 0, RANGED_NONE, 0, 0, 2, 4, 4, 40, 1, RACE_ORC, 3, 9, 0x1, 3, 1, 0, 4, 0, 0, MOVE_CAVALRY, 0, ABILITY_RACE_PREFIX, 0, 0, 0, 0, 0 },
    { "Shaman", 2, 2, RANGED_MAGIC_HOLY, 4, 0, 3, 6, 2, 60, 1, RACE_ORC, 22, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_HEALER | AFLAG_PURIFY, 0, 0, 0 },
    { "Magicians", 1, 5, RANGED_MAGIC_FIRE, 4, 0, 3, 8, 2, 120, 2, RACE_ORC, 21, 0, 0x0, 1, 1, 0, 4, 0, 0, 0, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_RACE_PREFIX, ABILITY2_FIRE_BALL_SPELL, 0, 0, 0, 0 },
    { "Engineers", 1, 0, RANGED_NONE, 0, 0, 1, 4, 2, 40, 1, RACE_ORC, 32, 0, 0x0, 1, 1, 0, 6, 1, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 4, 2, 60, 2, RACE_ORC, 0, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST, 0, 0, 0 },
    { "Wyvern Riders", 5, 0, RANGED_NONE, 0, 0, 5, 7, 6, 200, 2, RACE_ORC, 11, 3, 0x0, 10, 1, 0, 2, 0, 6, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, 0, ATTACK_POISON, 0, 0 },
    { "Spearmen", 3, 0, RANGED_NONE, 0, 0, 2, 7, 2, 30, 2, RACE_TROLL, 0, 0, 0x0, 4, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_REGENERATION, 0, 0, 0 },
    { "Swordsmen", 5, 0, RANGED_NONE, 0, 0, 2, 7, 2, 60, 3, RACE_TROLL, 3, 8, 0x0, 4, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_LARGE_SHIELD | AFLAG_REGENERATION, 0, 0, 0 },
    { "Halberdiers", 6, 0, RANGED_NONE, 0, 0, 3, 7, 2, 120, 3, RACE_TROLL, 4, 0, 0x0, 4, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_REGENERATION | AFLAG_NEGATE_FIRST_STRIKE, 0, 0, 0 },
    { "Shaman", 4, 2, RANGED_MAGIC_HOLY, 4, 0, 3, 8, 2, 180, 2, RACE_TROLL, 22, 0, 0x0, 4, 1, 0, 4, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_HEALER | AFLAG_REGENERATION | AFLAG_PURIFY, 0, 0, 0 },
    { "Settlers", 0, 0, RANGED_NONE, 0, 0, 1, 7, 2, 180, 2, RACE_TROLL, 0, 0, 0x0, 40, 1, 0, 1, 0, 0, 0, 0, ABILITY_RACE_PREFIX, 0, AFLAG_CREATE_OUTPOST | AFLAG_REGENERATION, 0, 0, 0 },
    { "War Trolls", 8, 0, RANGED_NONE, 0, 0, 4, 8, 4, 160, 3, RACE_TROLL, 5, 0, 0x0, 5, 1, 0, 4, 0, 0, 0, 0, 0, 0, AFLAG_REGENERATION, 0, 0, 0 },
    { "War Mammoths", 10, 0, RANGED_NONE, 0, 0, 6, 9, 4, 240, 4, RACE_TROLL, 9, 6, 0x0, 12, 1, 0, 2, 0, 0, 0, 0, 0, 0, AFLAG_WALL_CRUSHER, ATTACK_FIRST_STRIKE, 0, 0 },
    { "Magic Spirit", 5, 0, RANGED_NONE, 0, 0, 4, 8, 2, 30, 1, RACE_ARCANE_CREATURE, 6, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, 0, 0, AFLAG_FANTASTIC | AFLAG_MELD_WITH_NODE | AFLAG_NON_CORPOREAL, 0, 0, 0 },
    { "Hell Hounds", 3, 3, RANGED_FIRE_BREATH, 0, 1, 2, 6, 4, 40, 1, RACE_CHAOS_CREATURE, 6, 0, 0x0, 4, 1, 0, 4, 0, 0, 0, 0, 0, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Gargoyles", 4, 0, RANGED_NONE, 0, 1, 8, 7, 4, 200, 5, RACE_CHAOS_CREATURE, 6, 0, 0x0, 4, 1, 0, 4, 0, 0, MOVE_FLYING, 0, ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Fire Giant", 10, 10, RANGED_BOULDER, 2, 1, 5, 7, 4, 150, 3, RACE_CHAOS_CREATURE, 6, 0, 0x0, 15, 1, 0, 1, 0, 0, MOVE_MOUNTAINEER, 0, ABILITY_FIRE_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Fire Elemental", 12, 0, RANGED_NONE, 0, 0, 4, 6, 2, 100, 4, RACE_CHAOS_CREATURE, 6, 0, 0x0, 10, 1, 0, 1, 0, 0, 0, 0, ABILITY_FIRE_IMMUNITY | ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Chaos Spawn", 1, 4, RANGED_DOOM_GAZE, 0, 0, 6, 10, 2, 400, 12, RACE_CHAOS_CREATURE, 6, 0, 0x0, 15, 1, 0, 1, 0, 4, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, ABILITY2_CAUSE_FEAR_SPELL, AFLAG_FANTASTIC, ATTACK_POISON, 0, 0 },
    { "Chimera", 7, 4, RANGED_FIRE_BREATH, 0, 1, 5, 8, 4, 350, 10, RACE_CHAOS_CREATURE, 6, 0, 0x0, 8, 1, 0, 4, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Doom Bat", 10, 0, RANGED_NONE, 0, 1, 5, 9, 8, 300, 8, RACE_CHAOS_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, ABILITY2_IMMOLATION, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Efreet", 9, 9, RANGED_MAGIC_FIRE, 0, 2, 7, 10, 6, 550, 15, RACE_CHAOS_CREATURE, 6, 0, 0x0, 12, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_FIRE_IMMUNITY | ABILITY_FLYING_ANIM | ABILITY_CASTER_MP20, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Hydra", 6, 5, RANGED_FIRE_BREATH, 0, 1, 4, 11, 2, 650, 14, RACE_CHAOS_CREATURE, 6, 0, 0x0, 10, 1, 0, 9, 0, 0, 0, 0, 0, 0, AFLAG_FANTASTIC | AFLAG_REGENERATION, 0, 0, 0 },
    { "Great Drake", 30, 30, RANGED_FIRE_BREATH, 0, 3, 10, 12, 4, 900, 30, RACE_CHAOS_CREATURE, 6, 0, 0x0, 30, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Skeletons", 3, 0, RANGED_NONE, 0, 1, 4, 5, 2, 25, 1, RACE_DEATH_CREATURE, 6, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Ghouls", 4, 0, RANGED_NONE, 0, 1, 3, 6, 2, 80, 1, RACE_DEATH_CREATURE, 6, 0, 0x0, 3, 1, 0, 4, 0, 1, 0, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_CREATE_UNDEAD, ATTACK_POISON, 0, 0 },
    { "Night Stalker", 7, 1, RANGED_DEATH_GAZE, 0, 1, 3, 8, 4, 250, 1, RACE_DEATH_CREATURE, 6, 0, 0x0, 10, 1, 0, 1, 0, -2, 0, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_INVISIBILITY, 0, 0, 0 },
    { "Werewolves", 5, 0, RANGED_NONE, 0, 1, 1, 6, 4, 250, 5, RACE_DEATH_CREATURE, 6, 0, 0x0, 5, 1, 0, 6, 0, 0, 0, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_REGENERATION, 0, 0, 0 },
    { "Demon", 14, 0, RANGED_NONE, 0, 1, 5, 7, 4, 80, 3, RACE_DEATH_CREATURE, 6, 0, 0x0, 12, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_MISSILES_IMMUNITY | ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Wraiths", 7, 0, RANGED_NONE, 0, 2, 6, 8, 4, 500, 5, RACE_DEATH_CREATURE, 6, 0, 0x0, 8, 1, 0, 4, 0, -3, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC | AFLAG_NON_CORPOREAL, ATTACK_LIFE_STEAL, 0, 0 },
    { "Shadow Demons", 5, 4, RANGED_MAGIC_DEATH, 8, 2, 4, 8, 2, 325, 7, RACE_DEATH_CREATURE, 6, 0, 0x0, 5, 1, 0, 4, 0, 0, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC | AFLAG_PLANE_SHIFT | AFLAG_NON_CORPOREAL | AFLAG_REGENERATION, 0, 0, 0 },
    { "Death Knights", 9, 0, RANGED_NONE, 0, 3, 8, 10, 6, 600, 8, RACE_DEATH_CREATURE, 6, 0, 0x0, 8, 1, 0, 4, 0, -4, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, ATTACK_ARMOR_PIERCING | ATTACK_FIRST_STRIKE | ATTACK_LIFE_STEAL, 0, 0 },
    { "Demon Lord", 20, 10, RANGED_MAGIC_DEATH, 8, 3, 10, 12, 4, 900, 15, RACE_DEATH_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, -5, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM | ABILITY_SUMMON_DEMONS_1 | ABILITY_SUMMON_DEMONS_2, ABILITY2_CAUSE_FEAR_SPELL, AFLAG_FANTASTIC, ATTACK_LIFE_STEAL, 0, 0 },
    { "Zombies", 4, 0, RANGED_NONE, 0, 1, 3, 3, 2, 30, 0, RACE_DEATH_CREATURE, 6, 0, 0x0, 3, 1, 0, 6, 0, 0, 0, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_COLD_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Unicorns", 5, 0, RANGED_NONE, 0, 2, 3, 7, 4, 250, 5, RACE_LIFE_CREATURE, 6, 0, 0x0, 6, 1, 0, 4, 0, 2, MOVE_TELEPORTING, 0, ABILITY_POISON_IMMUNITY, ABILITY2_RESISTANCE_TO_ALL, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Guardian Spirit", 10, 0, RANGED_NONE, 0, 0, 4, 10, 2, 50, 1, RACE_LIFE_CREATURE, 6, 0, 0x0, 10, 1, 0, 1, 0, 1, 0, 0, 0, ABILITY2_RESISTANCE_TO_ALL, AFLAG_FANTASTIC | AFLAG_MELD_WITH_NODE | AFLAG_NON_CORPOREAL, 0, 0, 0 },
    { "Angel", 13, 0, RANGED_NONE, 0, 2, 7, 8, 6, 550, 15, RACE_LIFE_CREATURE, 6, 0, 0x0, 15, 1, 0, 1, 0, 1, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_FLYING_ANIM, ABILITY2_HOLY_BONUS, AFLAG_FANTASTIC, ATTACK_DISPEL_EVIL, 0, 0 },
    { "Arch Angel", 15, 0, RANGED_NONE, 0, 3, 10, 12, 8, 950, 20, RACE_LIFE_CREATURE, 6, 0, 0x0, 18, 1, 0, 1, 0, 2, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_FLYING_ANIM | ABILITY_CASTER_MP40, ABILITY2_HOLY_BONUS, AFLAG_FANTASTIC, 0, 0, 0 },
    { "War Bears", 7, 0, RANGED_NONE, 0, 0, 3, 6, 4, 70, 2, RACE_NATURE_CREATURE, 6, 0, 0x0, 8, 1, 0, 2, 0, 0, MOVE_FORESTER, 0, 0, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Sprites", 2, 3, RANGED_MAGIC_SPRITE, 4, 1, 2, 8, 4, 100, 3, RACE_NATURE_CREATURE, 6, 0, 0x0, 1, 1, 0, 4, 0, 0, MOVE_FLYING | MOVE_FORESTER, 0, ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Cockatrices", 4, 0, RANGED_NONE, 0, 1, 3, 7, 4, 275, 8, RACE_NATURE_CREATURE, 6, 0, 0x0, 3, 1, 0, 4, 0, -3, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, ATTACK_STONING_TOUCH, 0, 0 },
    { "Basilisk", 15, 1, RANGED_STONING_GAZE, 0, 1, 4, 7, 4, 325, 7, RACE_NATURE_CREATURE, 6, 0, 0x0, 30, 1, 0, 1, 0, -1, 0, 0, 0, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Giant Spiders", 4, 0, RANGED_NONE, 0, 1, 3, 7, 4, 200, 4, RACE_NATURE_CREATURE, 6, 0, 0x0, 10, 1, 0, 2, 0, 4, 0, 0, 0, ABILITY2_WEB_SPELL, AFLAG_FANTASTIC, ATTACK_POISON, 0, 0 },
    { "Stone Giant", 15, 15, RANGED_BOULDER, 2, 2, 8, 9, 4, 450, 9, RACE_NATURE_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, MOVE_MOUNTAINEER, 0, ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Colossus", 20, 20, RANGED_BOULDER, 2, 3, 10, 15, 4, 800, 17, RACE_NATURE_CREATURE, 6, 0, 0x0, 30, 1, 0, 1, 0, 0, 0, 0, ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_WALL_CRUSHER, ATTACK_FIRST_STRIKE, 0, 0 },
    { "Gorgons", 8, 1, RANGED_STONING_GAZE, 0, 2, 7, 9, 4, 599, 15, RACE_NATURE_CREATURE, 6, 0, 0x0, 9, 1, 0, 4, 0, -2, MOVE_FLYING, 0, ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Earth Elemental", 25, 0, RANGED_NONE, 0, 1, 4, 8, 2, 160, 7, RACE_NATURE_CREATURE, 6, 0, 0x0, 30, 1, 0, 1, 0, 0, 0, 0, ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_WALL_CRUSHER, 0, 0, 0 },
    { "Behemoth", 25, 0, RANGED_NONE, 0, 2, 9, 10, 4, 700, 15, RACE_NATURE_CREATURE, 6, 0, 0x0, 45, 1, 0, 1, 0, 0, 0, 0, 0, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Great Wyrm", 25, 0, RANGED_NONE, 0, 3, 12, 12, 6, 1000, 20, RACE_NATURE_CREATURE, 6, 0, 0x0, 45, 1, 0, 1, 0, 15, -128, 0, 0, 0, AFLAG_FANTASTIC, ATTACK_POISON, 0, 0 },
    { "Floating Island", 0, 0, RANGED_NONE, 0, 0, 0, 10, 4, 50, 5, RACE_SORCERY_CREATURE, 6, 0, 0x0, 45, 1, 8, 1, 0, 0, MOVE_SAILING, 0, ABILITY_STONING_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Phantom Beast", 18, 0, RANGED_NONE, 0, 1, 0, 8, 4, 225, 4, RACE_SORCERY_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, ABILITY_STONING_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_NON_CORPOREAL, ATTACK_ILLUSION, 0, 0 },
    { "Phantom Warriors", 3, 0, RANGED_NONE, 0, 0, 0, 6, 2, 20, 2, RACE_SORCERY_CREATURE, 6, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_STONING_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_NON_CORPOREAL, ATTACK_ILLUSION, 0, 0 },
    { "Storm Giant", 12, 10, RANGED_MAGIC_LIGHTNING, 4, 2, 7, 9, 4, 500, 10, RACE_SORCERY_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, 0, 0, AFLAG_FANTASTIC | AFLAG_WALL_CRUSHER, ATTACK_ARMOR_PIERCING, 0, 0 },
    { "Air Elemental", 15, 0, RANGED_NONE, 0, 1, 8, 9, 10, 170, 8, RACE_SORCERY_CREATURE, 6, 0, 0x0, 10, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC | AFLAG_INVISIBILITY, 0, 0, 0 },
    { "Djinn", 15, 8, RANGED_MAGIC_WIND, 0, 3, 8, 10, 6, 650, 17, RACE_SORCERY_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, MOVE_FLYING | MOVE_TELEPORTING, 0, ABILITY_FLYING_ANIM | ABILITY_CASTER_MP20, 0, AFLAG_FANTASTIC | AFLAG_WIND_WALKING, 0, 0, 0 },
    { "Sky Drake", 20, 20, RANGED_LIGHTNING, 0, 3, 10, 14, 8, 1000, 25, RACE_SORCERY_CREATURE, 6, 0, 0x0, 25, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_MAGIC_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Nagas", 4, 0, RANGED_NONE, 0, 1, 3, 7, 2, 120, 2, RACE_SORCERY_CREATURE, 6, 0, 0x0, 6, 1, 0, 2, 0, 4, MOVE_SWIMMING, 0, 0, 0, AFLAG_FANTASTIC, ATTACK_FIRST_STRIKE | ATTACK_POISON, 0, 0 }
};
int exp_level_thresholds[9] = { 0, 20, 60, 120, 200, 300, 450, 600, 1000 };    /* DS:1d74 */
struct RaceInfo race_table[14] = {    /* DS:1d86 */
    { "Barbarian", 3, { BLD_UNIVERSITY, BLD_ANIMISTS_GUILD, BLD_CATHEDRAL, 0, 0, 0, 0 }, 15, 2, HOUSE_STYLE_ROOFED },
    { "Beastmen", 2, { BLD_FANTASTIC_STABLE, BLD_SHIP_YARD, 0, 0, 0, 0, 0 }, 5, 0, HOUSE_STYLE_ROOFED },
    { "Dark Elf", 2, { BLD_CATHEDRAL, BLD_MARITIME_GUILD, 0, 0, 0, 0, 0 }, 2, -2, HOUSE_STYLE_TREE },
    { "Draconian", 3, { BLD_MECHANICIANS_GUILD, BLD_FANTASTIC_STABLE, BLD_MARITIME_GUILD, 0, 0, 0, 0 }, 5, -1, HOUSE_STYLE_ROOFED },
    { "Dwarven", 4, { BLD_STABLES, BLD_SHIP_YARD, BLD_UNIVERSITY, BLD_PARTHENON, 0, 0, 0 }, 7, -2, HOUSE_STYLE_ROOFED },
    { "Gnoll", 5, { BLD_ANIMISTS_GUILD, BLD_SAGES_GUILD, BLD_PARTHENON, BLD_ARMORERS_GUILD, BLD_MARITIME_GUILD, 0, 0 }, 5, -1, HOUSE_STYLE_HUT },
    { "Halfling", 4, { BLD_UNIVERSITY, BLD_ARMORERS_GUILD, BLD_STABLES, BLD_SHIP_YARD, 0, 0, 0 }, 15, 0, HOUSE_STYLE_ROOFED },
    { "High Elf", 2, { BLD_PARTHENON, BLD_MARITIME_GUILD, 0, 0, 0, 0, 0 }, 5, -2, HOUSE_STYLE_TREE },
    { "High Men", 1, { BLD_FANTASTIC_STABLE, 0, 0, 0, 0, 0, 0 }, 10, 0, HOUSE_STYLE_ROOFED },
    { "Klackon", 3, { BLD_TEMPLE, BLD_SAGES_GUILD, BLD_SHIP_YARD, 0, 0, 0, 0 }, 5, -1, HOUSE_STYLE_HUT },
    { "Lizardman", 7, { BLD_WAR_COLLEGE, BLD_ANIMISTS_GUILD, BLD_SAGES_GUILD, BLD_PARTHENON, BLD_MINERS_GUILD, BLD_SHIP_WRIGHTS_GUILD, BLD_SAWMILL }, 10, 1, HOUSE_STYLE_HUT },
    { "Nomad", 2, { BLD_MARITIME_GUILD, BLD_WIZARDS_GUILD, 0, 0, 0, 0, 0 }, 10, -1, HOUSE_STYLE_ROOFED },
    { "Orc", 0, { 0, 0, 0, 0, 0, 0, 0 }, 10, 0, HOUSE_STYLE_ROOFED },
    { "Troll", 4, { BLD_FANTASTIC_STABLE, BLD_SAGES_GUILD, BLD_MINERS_GUILD, BLD_SHIP_YARD, 0, 0, 0 }, 3, -2, HOUSE_STYLE_HUT }
};
char *unused_empty_string = "";    /* DS:1ed6  no reader found (MAGIC.C unused_empty_string is the same "") */
char *arcane_first_realm_names[6] = {    /* DS:1ed8  Arcane first: index REALM_* + 1 (not REALM_* order) */
    "Arcane", "Nature", "Sorcery", "Chaos", "Life", "Death"
};
char *city_enchant_names[26] = {    /* DS:1ee4 */
    "Wall of Fire", "Chaos Rift", "Dark Rituals", "Evil Presence", "Cursed Lands", "Pestilence", "Cloud of Shadow",
    "Famine", "Flying", "Nature Ward", "Sorcery Ward", "Chaos Ward", "Life Ward", "Death Ward", "Nature's Eye",
    "Earth Gate", "Stream of Life", "Gaia's Blessing", "Inspirations", "Prosperity", "Astral Gate",
    "Heavenly Light", "Consecration", "Wall of Darkness", "Altar of Battle", "Nightshade"
};
int unit_ench_upkeep[32] = {    /* DS:1f18 */
    2, 2, 0, 1, 1, 3, 10, 1, 1, 1, 5, 1, 5, 1, 1, 10, 10, 3, 1, 5, 2, 1, 2, 1, 2, 1, 4, 1, 5, 2, 2, 5
};
int city_ench_upkeep[26] = {    /* DS:1f58 */
    2, 10, 0, 4, 2, 5, 3, 5, 25, 5, 5, 5, 5, 5, 1, 5, 8, 3, 2, 2, 5, 2, 8, 5, 5, 0
};
int global_ench_upkeep[24] = {    /* DS:1f8c */
    15, 10, 40, 5, 5, 50, 200, 7, 10, 10, 40, 15, 20, 10, 40, 10, 10, 10, 3, 10, 5, 10, 3, 3
};
char *personality_names[6] = {    /* DS:1fbc */
    "Maniacal", "Ruthless", "Aggressive", "Chaotic", "Lawful", "Peaceful"
};
int personality_treaty_mod[6] = { 0, 10, 20, 30, 40, 50 };    /* DS:1fc8 */
char *objective_names[5] = {    /* DS:1fd4 */
    "Pragmatist", "Militarist", "Theurgist", "Perfectionist", "Expansionist"
};
int personality_hostility_mod[6] = { 40, 20, 10, 10, -40, -20 };    /* DS:1fde */
struct DifficultyInfo difficulty_table[5] = {    /* DS:1fea */
    { 100, 100, 100, 100, 100, 100, 100, 100 },
    { 100, 100, 125, 125, 125, 100, 125, 90 },
    { 150, 150, 150, 150, 150, 100, 150, 75 },
    { 200, 200, 200, 200, 200, 110, 200, 60 },
    { 400, 400, 400, 400, 400, 150, 400, 30 }
};
char ai_bldg_objective_bonus[8][5] = {    /* DS:203a */
    { 0, 0, 0, 5, 0 }, { 0, 0, 0, 5, 0 }, { 0, 0, 10, 0, 0 }, { 0, 0, 10, 0, 0 }, { 0, 10, 0, 0, 2 },
    { 0, 0, 0, 5, 0 }, { 0, 0, 0, 5, 0 }, { 0, 5, 0, 0, 5 }
};
char ai_unit_objective_bonus[5] = { 0, 5, 0, 0, 3 };    /* DS:2062 */
char ai_civilian_objective_bonus[5] = { 0, 0, 0, 0, 10 };    /* DS:2067 */
int ai_bldg_category_weight[10] = { 10, 15, 30, 10, 10, 15, 15, 10, 10, 10 };    /* DS:206c */
/* Borland VROOMM: the overlay buffer, 0x1000 paragraphs = 64 KB (read by the overlay manager; defining it keeps
   OVERLAY.LIB's OVRBUFF default out) */
unsigned _ovrbuffer = 4096;                     /* DS:2080 */

/* () WIZARDS.EXE: CONFIG.MOM (the sound drivers chosen by INSTALL; "Run INSTALL to configure MASTER OF MAGIC." when
   it is missing), MAGIC.SET (the names of save slots whose SAVEn.GAM is gone are dropped and the file rewritten),
   engine start-up, the game data, the auto-save (slot 8) the start-up program left, the screen assets, the free EMS
   pages for the overlay manager (an "OVERXYZ" probe handle), the palette, then the game until the player quits */
void main(void)
{
    int pages;
    FILE *file;
    int i;
    int music_io;
    int music_irq;
    int digi_driver;
    int digi_io;
    int digi_irq;
    int digi_dma;
    char name[40];
    char found[30];
    int music_driver;
    int music_dma;

    ems_reserved_pages = 158;                   /* 16 KB EMS pages that must stay free after the LBX caches
                                                   (about 2.5 MB; EMSLBX.C's default is 40) */
    if (DOS_Find_File("CONFIG.MOM", found) == 0)
        Fatal_Exit("Run INSTALL to configure MASTER OF MAGIC.\n\n");
    file = fopen("CONFIG.MOM", "rb");
    fread(&config_mom, sizeof(struct ConfigMom), 1, file);
    fclose(file);
    Settings_Load_Or_Create();
    music_io = config_mom.music_io;
    music_irq = config_mom.music_irq;
    music_dma = -1;
    /* INSTALL's CONFIG.MOM card codes (CFG_SND_*) -> the engine's drivers (SOUND_DRV_*, SNDDRV.LBX pairs) */
    switch (config_mom.music_driver) {
    case CFG_SND_NONE:
        music_driver = SOUND_DRV_NONE;
        music_io = -1;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_SPEAKER:
        music_driver = SOUND_DRV_SPEAKER;
        music_io = -1;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_ADLIB:
        music_driver = SOUND_DRV_ADLIB;
        music_io = -1;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_SB:
        music_driver = SOUND_DRV_SB;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_SB_PRO:
        music_driver = SOUND_DRV_SB_PRO;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_SB_PRO_2:
        music_driver = SOUND_DRV_SB_PRO_2;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_PAS:
        music_driver = SOUND_DRV_PAS;
        music_io = -1;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_PAS_16:
        music_driver = SOUND_DRV_PAS_16;
        music_io = -1;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_ADLIB_GOLD:
        music_driver = SOUND_DRV_ADLIB_GOLD;
        music_io = -1;
        music_irq = -1;
        music_dma = -1;
        break;
    case CFG_SND_ROLAND:
        music_driver = SOUND_DRV_ROLAND;
        music_dma = -1;
        break;
    case CFG_SND_ROLAND_B:
        music_driver = SOUND_DRV_ROLAND;
        music_dma = -1;
        break;
    case CFG_SND_ROLAND_2:
        music_driver = SOUND_DRV_ROLAND_2;
        music_dma = -1;
        break;
    }
    if (music_driver >= SOUND_DRV_ROLAND) {
        DOS_Print_Dollar("Initializing Roland Drivers...$");
        Timer_Mark();
        Wait_Ticks(2);
    }
    digi_io = config_mom.digi_io;
    digi_irq = config_mom.digi_irq;
    digi_dma = config_mom.digi_dma;
    switch (config_mom.digi_driver) {
    case CFG_SND_NONE:
        digi_driver = SOUND_DRV_NONE;
        digi_io = -1;
        digi_irq = -1;
        digi_dma = -1;
        break;
    case CFG_SND_SPEAKER:
        digi_driver = SOUND_DRV_SPEAKER;
        digi_io = -1;
        digi_irq = -1;
        digi_dma = -1;
        break;
    case CFG_SND_ADLIB:
        digi_driver = SOUND_DRV_NONE;
        digi_io = -1;
        digi_irq = -1;
        digi_dma = -1;
        break;
    case CFG_SND_SB:
        digi_driver = SOUND_DRV_SB;
        break;
    case CFG_SND_SB_PRO:
        digi_driver = SOUND_DRV_SB_PRO;
        break;
    case CFG_SND_SB_PRO_2:
        digi_driver = SOUND_DRV_SB_PRO_2;
        /* the SB Pro 2 driver plays music and sound on the one card: the music takes its IRQ / DMA */
        music_irq = digi_irq;
        music_dma = digi_dma;
        break;
    case CFG_SND_PAS:
        digi_driver = SOUND_DRV_PAS;
        digi_io = -1;
        break;
    case CFG_SND_PAS_16:
        digi_driver = SOUND_DRV_PAS_16;
        digi_io = -1;
        break;
    case CFG_SND_ADLIB_GOLD:
        digi_driver = SOUND_DRV_ADLIB_GOLD;
        break;
    case CFG_SND_ROLAND:
        digi_driver = SOUND_DRV_ROLAND;
        digi_io = -1;
        digi_irq = -1;
        digi_dma = -1;
        break;
    case CFG_SND_ROLAND_B:
        digi_driver = SOUND_DRV_ROLAND;
        digi_io = -1;
        digi_irq = -1;
        digi_dma = -1;
        break;
    case CFG_SND_ROLAND_2:
        digi_driver = SOUND_DRV_ROLAND_2;
        digi_io = -1;
        digi_irq = -1;
        digi_dma = -1;
        break;
    }
    if (music_driver == SOUND_DRV_ADLIB)
        digi_driver = SOUND_DRV_ADLIB;
    if (game_config.sound_channels > 2 || game_config.sound_channels < 0)
        game_config.sound_channels = 0;
    game_config.input_type = 1;                 /* Engine_Init's input mode 1: the mouse (set again below) */
    for (i = 1; i < SAVE_SLOT_CONTINUE + 1; i++) {   /* SAVE1 .. SAVE8.GAM */
        if (game_config.save_slot_used[i - 1] != 0) {
            itoa(i, found, 10);
            strcpy(name, "SAVE");
            strcat(name, found);
            strcat(name, ".GAM");
            DOS_Find_File(name, found);
            if (found[0] == 0) {
                game_config.save_slot_used[i - 1] = 0;
                strcpy(game_config.save_names[i - 1], "");
                file = fopen("MAGIC.SET", "wb");
                fwrite(&game_config, sizeof(struct GameConfig), 1, file);
                fclose(file);
            }
        }
    }
    EMS_Set_Required_K(2700);
    LBX_Set_Min_Free(583);
    game_config.input_type = 1;
    game_config.sound_channels = 2;
    Engine_Init(game_config.input_type, game_config.sound_channels, "FONTS.LBX", music_driver, music_io, music_irq,
                music_dma, digi_driver, digi_io, digi_irq, digi_dma);
    Release_Version();
    UI_Right_Click_Ids_On();
    Alloc_Game_Data(4600);
    Load_Game(SAVE_SLOT_CONTINUE);
    Load_Startup_Assets();
    _AX = 0x5901;                               /* EMS 4.0 Get Raw Page Counts: BX = the free raw pages */
    geninterrupt(0x67);
    pages = _BX;
    if (pages != 0) {
        ems_main_handle = EMS_Alloc_Named(pages, "OVERXYZ");
        EMS_Free_Handle(ems_main_handle);
        _OvrInitEms(0, 0, pages);
    } else
        ems_main_handle = 0;
    Palette_Load(0, -1);
    Remap_Build_All();
    UI_Set_Press_Offset(1, 1);
    Palette_Cycle_Color(PAL_CYCLE_COLOR, 40, 0, 0, 63, 0, 0, 1);
    Palette_Upload();
    Palette_Fade_From_Black();
    current_screen = SCREEN_MAP;
    wizards[PLAYER_NEUTRAL].banner_color = BANNER_NEUTRAL;
    UI_Clear_Controls();
    Game_Start_Init();
    mastery_winner = -1;
    Main_Loop();
    Quit_Hook_Empty();
    Memory_Report_Exit();
}

/* () the top-level screen loop on current_screen: 0 city, 1 game menu (the music restarts or stops when its option
   changed), 2 armies, 3 cities, 4 quit, 5 overland map, 6 magic, 7 road building, 8 city production, 9 items,
   10 end of turn, 12 spellbook, 14 info menu, 15 diplomacy.
   Simtex name Screen_Control (MoO2 symbols, module MOX2); clarified using ReMoM repo */
void Main_Loop(void)
{
    int quit;
    int old_music;

    quit = 0;
    while (quit == 0) {
        UI_Clear_Controls();
        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
        switch (current_screen) {
        case SCREEN_CITY:
            Screen_City();
            break;
        case SCREEN_GAME_MENU:
            old_music = game_config.music;
            Screen_Game_Menu();
            mastery_winner = -1;
            city_razed = 0;
            if (old_music != game_config.music) {
                if (game_config.music == 1)
                    Music_Play_Current();
                else
                    Music_Release_Sequence();
            }
            break;
        case SCREEN_QUIT:
            quit = 1;
            break;
        case SCREEN_MAP:
            Screen_Overland_Map();
            break;
        case SCREEN_ARMIES:
            Screen_Armies();
            break;
        case SCREEN_CITIES:
            Screen_Cities();
            break;
        case SCREEN_MAGIC:
            Screen_Magic();
            break;
        case SCREEN_ROAD_BUILDING:
            Screen_Road_Building();
            break;
        case SCREEN_CITY_PRODUCTION:
            Screen_City_Production();
            break;
        case SCREEN_ITEMS:
            Screen_Items();
            break;
        case SCREEN_END_TURN:
            Human_End_Turn();
            break;
        case SCREEN_SPELLBOOK:
            Screen_Spellbook_Cast();
            break;
        case SCREEN_INFO:
            Screen_Info_Menu(-1);
            break;
        case SCREEN_DIPLOMACY:
            Screen_Diplomacy();
            break;
        }
    }
}

/* () shows the drawn frame by present_mode, then clears it: 1 left-to-right wipe, 2 fade in from black, 3 dissolve,
   4 the zoom wipe from (zoom_from_x, zoom_from_y), 5 palette only, else the palette upload and the frame shown (UI_Present_Frame) */
void Screen_Present(void)
{
    switch (present_mode) {
    case PRESENT_WIPE:
        VGA_Present_Wipe_LR();
        break;
    case PRESENT_FADE_IN:
        UI_Present_Frame();
        Palette_Fade_From_Black();
        break;
    case PRESENT_DISSOLVE:
        Palette_Upload();
        VGA_Present_Dissolve();
        break;
    case PRESENT_PALETTE_ONLY:
        Palette_Upload();
        break;
    case PRESENT_ZOOM:
        Screen_Transition_Zoom(zoom_from_x, zoom_from_y, zoom_steps, p_scratch + 400);   /* the zoom source: 400 paragraphs in */
        break;
    default:
        Palette_Upload();
        UI_Present_Frame();
        break;
    }
    present_mode = PRESENT_NORMAL;
}

/* () darkens the whole palette in four steps of 25 % (one tick each) */
void Palette_Fade_To_Black(void)
{
    int percent;

    for (percent = 25; percent < 101; percent += 25) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () brings the palette back from 75 % dark in steps of 25 % (one tick each) */
void Palette_Fade_From_Black(void)
{
    int percent;

    for (percent = 75; percent >= 0; percent -= 25) {
        Timer_Mark();
        Palette_Mark_Range(0, 255);
        Palette_Darken_Percent(percent);
        Wait_Ticks(1);
    }
}

/* () the per-load game set-up: city areas, the view, unit upkeep / sounds, farmers, music, the combat graphics, road
   connections, the unit list, the AI wizards' average_unit_cost (0), random events off on the intro difficulty, the
   path cache (empty), the AI landmass / coast tile lists and replan timers, and the Time Stop caster (+1, 0 none).
   GAMEMENU.C's Game_Rebuild_After_Load repeats it but for city_razed */
void Game_Start_Init(void)
{
    int i;

    city_razed = 0;
    current_player = 0;
    present_mode = PRESENT_NORMAL;
    City_Area_Bitmaps_Rebuild();
    Game_Session_Init();
    UnitTypes_Init_Upkeep_Sounds();
    Human_Cities_Area_Nop();
    Never_Set_Flag_Tiles_Collect();
    Cities_Recalc_All_Farmers();
    stack_unit_count = 0;
    Music_Play_Current();
    Load_Overland_Screen_Graphics();
    Cities_Update_Road_Connections();
    Units_Compact();
    Units_Reset_Moves_NonCurrent();
    for (i = 1; i < num_players; i++)
        wizards[i].average_unit_cost = 0;
    Overland_Resume_View();
    if (difficulty == DIFFICULTY_INTRO)
        game_config.random_events = 0;
    for (i = 0; i < 100; i++)                   /* the first 100 of the 140 records */
        p_path_cache[i].x1 = -1;
    path_cache_index = -1;
    AI_Build_Landmass_Tile_Lists();
    AI_Build_Coast_Tile_Lists();
    for (i = 0; i < num_players; i++)
        p_ai_replan_timer[i] = 0;
    time_stop_player = 0;
    for (i = 0; i < num_players; i++)
        if (wizards[i].globals.time_stop > 0)
            time_stop_player = i + 1;
}

/* (weights, count) a weighted random pick: halves every weight while their (unsigned) sum reaches 512, then
   Random(sum) walks the weights; returns the index (0 when all weights are 0) */
int Random_Weighted_Pick(int *weights, int count)
{
    unsigned total;
    int i;
    int pick;
    int k;
    int j;

restart:
    total = 0;
    for (i = 0; i < count; i++) {
        total += weights[i];
        if (total >= 512) {
            for (j = 0; j < count; j++)
                weights[j] = (unsigned)weights[j] >> 1;
            goto restart;
        }
    }
    if (total == 0)
        return 0;
    pick = Random(total) - weights[0];
    k = 0;
    while (pick > 0 && k < count - 1) {
        k++;
        pick = pick - weights[k];
    }
    return k;
}

/* (weights, count) Random_Weighted_Pick for long weights (halved by long division while the sum reaches 512) */
int Random_Weighted_Pick32(long *weights, int count)
{
    long total;
    int i;
    int pick;
    int k;
    int j;

restart:
    total = 0;
    for (i = 0; i < count; i++) {
        total += weights[i];
        if (total >= 512) {
            for (j = 0; j < count; j++)
                weights[j] = weights[j] / 2;
            goto restart;
        }
    }
    if (total == 0)
        return 0;
    pick = Random(total) - (int)weights[0];
    k = 0;
    while (pick > 0 && k < count - 1) {
        k++;
        pick -= (int)weights[k];
    }
    return k;
}

/* (weights, count, jitter) adds Random(jitter) to every non-zero weight, returns the index of the largest (no
   callers in WIZARDS) */
int Weights_Jitter_Max(unsigned *weights, int count, int jitter)
{
    int i;
    unsigned best;
    int best_index;

    for (i = 0; i < count; i++)
        if (weights[i] != 0)
            weights[i] += Random(jitter);
    best = weights[0];
    best_index = 0;
    for (i = 1; i < count; i++) {
        if (weights[i] > best) {
            best_index = i;
            best = weights[i];
        }
    }
    return best_index;
}

/* (weights, count, jitter) Weights_Jitter_Max for long weights (no callers in WIZARDS) */
int Weights_Jitter_Max32(long *weights, int count, int jitter)
{
    int i;
    long best;
    int best_index;

    for (i = 0; i < count; i++)
        if (weights[i] != 0)
            weights[i] += Random(jitter);
    best = weights[0];
    best_index = 0;
    for (i = 1; i < count; i++) {
        if (weights[i] > best) {
            best_index = i;
            best = weights[i];
        }
    }
    return best_index;
}

/* () an empty hook of the window zoom (left over) */
void Window_Zoom_Skipped_Nop(void)
{
}

/* () empty; Window_Zoom_Open calls it before the zoom animation */
void Window_Zoom_Start_Nop(void)
{
}

/* () the button click sound, when sound effects are on */
void Play_Click_Sound(void)
{
    if (game_config.sound_effects == 1)
        Sound_Play(snd_click);
}

/* () the message sound, when sound effects are on */
void Play_Message_Sound(void)
{
    if (game_config.sound_effects == 1)
        Sound_Play(snd_message);
}

/* () the click sound again (a second copy, used by the spell casting screens) */
void Play_Click_Sound_Casting(void)
{
    if (game_config.sound_effects == 1)
        Sound_Play(snd_click);
}

/* () empty hook called on quitting */
void Quit_Hook_Empty(void)
{
}
