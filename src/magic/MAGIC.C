/* MAGIC.C - MAGIC.EXE code segment 1320 (MAGIC only): the start-up program's main module - main() (CONFIG.MOM sound
   setup, MAGIC.SET, engine start-up, intro), the main menu (Continue / Load / New Game / Quit / Hall of Fame) - and
   the game tables MAGIC.EXE uses (unit types, races, names, AI weights, difficulty) with their strings; its data
   block is DS:00aa .. 28e3, the first module data after the C runtime's.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 1320 src/magic/MAGIC.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     Palette_Load (2; PALETTE.C takes 3);
     UI_Add_Hotkey (2; UI.C takes 1). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"
#pragma warn -pro

extern int menu_return_mode;                    /* DS:290a */
extern int ems_reserved_pages;                  /* DS:4100 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];         /* DS:89f2 */
unsigned char (far *p_region_bits_land)[REGION_ROWS][REGION_COLS];        /* DS:89ee */
unsigned char (far *p_region_bits_water)[REGION_ROWS][REGION_COLS];        /* DS:89ea */
unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];    /* DS:89e6 */
struct Node far *p_nodes;                       /* DS:89e2 */
struct Fortress far *p_fortresses;              /* DS:89de */
struct Tower far *p_towers;                     /* DS:89da */
struct Lair far *p_lairs;                       /* DS:89d6 */
struct City far *p_cities;                      /* DS:89d2 */
unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];    /* DS:89ce */
unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89ca */
unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:89c6 */
unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH]; /* DS:89c2 */
void far *p_terrain_minimap_colors;             /* DS:89be  WIZARDS DS:9ca8 */
struct PathWork far *p_path_work;               /* DS:89ba */
unsigned char far *city_area_shared_bits;       /* DS:89b6 */
unsigned char far *city_area_bits;              /* DS:89b2 */
unsigned far *p_threat_maps[2];                 /* DS:89aa  WIZARDS DS:9c94 */
struct PathCacheEntry far *p_path_cache;        /* DS:89a6 */
struct GameConfig game_config;                  /* DS:87d4 */
int num_players;                                /* DS:87d2 */
int land_size;                                  /* DS:87d0 */
int magic_setting;                              /* DS:87ce */
int difficulty;                                 /* DS:87cc */
int num_cities;                                 /* DS:87ca */
int num_units;                                  /* DS:87c8 */
int game_turn;                                  /* DS:87c6 */
int top_unit_in_stack;                          /* DS:87c4 */
int current_player;                             /* DS:87c2  WIZARDS DS:bd8c; player whose turn/view it is (0 = human) */
int map_view_x;                                 /* DS:87c0  WIZARDS DS:bd8a; the overland map view (top-left square) */
int map_view_y;                                 /* DS:87be  WIZARDS DS:bd88 */
int map_view_plane;                             /* DS:87bc  WIZARDS DS:bd86; x 0x960 indexes the per-plane maps */
int next_unit_ref_x;                            /* DS:87ba  WIZARDS DS:bd84; reference point for Find_Next_Unit's
                                                   nearest-unit distance; set when a stack is selected (Armies unit
                                                   click, overland, Find_Next_Unit, summons, Earth Gate...). The Armies
                                                   hero-portrait click does NOT set it */
int next_unit_ref_y;                            /* DS:87b8  WIZARDS DS:bd82; y of the same point */
int config_mom_spare;                           /* DS:87b6  no code in either program reads it */
struct ConfigMom config_mom;                    /* DS:87a4 */
struct HelpEntry help_entries[50];              /* DS:85b0 */
struct Wizard wizards[6];                       /* DS:6900 */
struct HeroName far *p_hero_names;              /* DS:68fc */
struct Unit far *p_units;                       /* DS:68f8 */
int ems_cont_handle;                            /* DS:68f6 */
void far *p_landmass_tile_x[2];                 /* DS:68ee  WIZARDS DS:9eb8 */
void far *p_landmass_tile_y[2];                 /* DS:68e6  WIZARDS DS:9eb0 */
void far *p_landmass_tile_next[2];              /* DS:68de  WIZARDS DS:9ea8 */
void far *p_landmass_tile_head[2];              /* DS:68d6  WIZARDS DS:9ea0 */
signed char far *p_ai_square_stack_x;                  /* DS:68d2  WIZARDS DS:9e9c */
signed char far *p_ai_square_stack_y;                  /* DS:68ce  WIZARDS DS:9e98 */
signed char far *p_ai_square_stack_plane;              /* DS:68ca  WIZARDS DS:9e94 */
signed char far *p_ai_square_stack_class;              /* DS:68c6  WIZARDS DS:9e90 */
signed char far *p_ai_square_stack_size;               /* DS:68c2  WIZARDS DS:9e8c */
int far *p_ai_square_stack_units[80];                  /* DS:6782  WIZARDS DS:9d4c */
unsigned ai_square_stack_count;                        /* DS:6780  WIZARDS DS:9d4a */
void far *p_coast_tile_x[2];                    /* DS:6778  WIZARDS DS:9d42 */
void far *p_coast_tile_y[2];                    /* DS:6770  WIZARDS DS:9d3a */
void far *p_coast_tile_next[2];                 /* DS:6768  WIZARDS DS:9d32 */
void far *p_coast_tile_head[2];                 /* DS:6760  WIZARDS DS:9d2a */
int grand_vizier_spare[3];                      /* DS:675a  no code in either program reads it */
int grand_vizier;                               /* DS:6758 */
unsigned p_world_block;                       /* DS:6756 */
unsigned p_screen_gfx;                          /* DS:6754 */
unsigned world_block_rest;                     /* DS:6752 */
int world_block_paras;                          /* DS:6750 */
unsigned big_city_wall_pic;                     /* DS:674e  WIZARDS DS:9d18; type from the layout (unused in MAGIC);
                                                CITYSCAP.LBX 114 picture used by Popup_Building_Completed /
                                                Building_Pic_Opaque_Bounds */
char far *p_race_unrest_rows[14];              /* DS:6716  WIZARDS DS:9ce0; unused here */
struct Building far *p_building_data;                /* DS:6712 */
int ai_flyer_pool_strength_spare[7];                 /* DS:6704  no code in either program reads it */
int ai_flyer_pool_strength[MAX_STACK];                       /* DS:66f2  WIZARDS DS:d3fa (by layout; unused in MAGIC) */
int ai_pool_count;                              /* DS:66f0  WIZARDS DS:d3f8; type from the layout (unused in MAGIC); units
                                                   in the attack pool (max 9, AI_Pool_Add_Strongest) */
int ai_flyer_pool_count;                             /* DS:66ee  WIZARDS DS:d3f6; type from the layout (unused in MAGIC);
                                                        units in the second AI attack pool: units that fly or cross
                                                        water (max 9; AI_Pool_Add_Strongest; ai_pool_count = all units)
                                                        */
int ai_pool_min_strength;                       /* DS:66ec  WIZARDS DS:d3f4; type from the layout (unused in MAGIC); weakest
                                                   strength in the AI attack pool (10000 = empty): a stronger candidate
                                                   replaces that unit (AI_Pool_Add_Strongest) */
int ai_flyer_pool_min_strength;                      /* DS:66ea  WIZARDS DS:d3f2; type from the layout (unused in MAGIC);
                                                        the same for the second pool (fliers / water crossers) */
signed char *ai_lm_rally_x_row;                 /* DS:66e8  WIZARDS DS:d3f0 (by layout; unused in MAGIC) */
signed char *ai_lm_rally_y_row;                 /* DS:66e6  WIZARDS DS:d3ee (by layout; unused in MAGIC) */
int ai_lm_status_row;                           /* DS:66e4  WIZARDS DS:d3ec; type from the layout (unused in MAGIC): an int
                                                   holding a near address, ai_landmass_plan status[60] of the current AI
                                                   player (set by AI_Unit_Dispatch) */
int ai_foreign_node_on_circle_plane;                    /* DS:66e2  WIZARDS DS:d3ea; type from the layout (unused in MAGIC);
                                                           1 when a node the player does not own is on its summoning
                                                           circle's plane */
int ai_spell_pick_weights[50];                    /* DS:667e  WIZARDS DS:d386 (by layout; unused in MAGIC) */
long combat_att_floating_island;                /* DS:667a  WIZARDS DS:d382; type from the layout (unused in MAGIC) */
int combat_def_floating_island;                 /* DS:6678  WIZARDS DS:d380; type from the layout (unused in MAGIC);
                                                   defender brought a Floating Island */
int music_jingle_spare[33];                     /* DS:6636  no code in either program reads it */
int music_jingle;                               /* DS:6634  WIZARDS DS:9134; type from the layout (unused in MAGIC);
                                                   MUSIC.LBX entry loaded for a spell screen jingle (spell learned,
                                                   global spell, Mastery) */
unsigned p_sfx_buf;                             /* DS:6632  WIZARDS DS:9132; type from the layout (unused in MAGIC); sound
                                                   effect buffer (Sound_Play target for spell/combat sounds) */
unsigned p_anim_pic;                            /* DS:6630  WIZARDS DS:9130; type from the layout (unused in MAGIC);
                                                   spell/effect animation picture (Spell_Load_Anim,
                                                   Map_Spell_Effect_Anim) */
struct Spell far *p_spell_data;                 /* DS:662c */
int time_stop_player;                           /* DS:662a  WIZARDS DS:912a; the Time Stop caster + 1, 0 none */
int *ai_city_own_value;                         /* DS:6628  WIZARDS DS:9128 (by layout; unused in MAGIC) */
int *ai_city_target_value;                      /* DS:6626  WIZARDS DS:9126 (by layout; unused in MAGIC) */
int *ai_city_garrison;                          /* DS:6624  WIZARDS DS:9124 (by layout; unused in MAGIC) */
int *ai_city_threat;                            /* DS:6622  WIZARDS DS:9122 (by layout; unused in MAGIC) */
unsigned char *p_ai_spell_groups;               /* DS:6620  WIZARDS DS:9120 (by layout; unused in MAGIC) */
int ai_known_spell_count;                       /* DS:661e  WIZARDS DS:911e; type from the layout (unused in MAGIC); entries
                                                   in ai_known_spells */
unsigned char *ai_known_spells;                 /* DS:661c  WIZARDS DS:911c (by layout; unused in MAGIC) */
int ai_hostile_count;                           /* DS:661a  WIZARDS DS:911a; type from the layout (unused in MAGIC); entries
                                                   in ai_hostile_wizards */
int ai_hostile_wizards[6];                      /* DS:660e  WIZARDS DS:910e (by layout; unused in MAGIC) */
int ai_own_stack_count;                         /* DS:660c  WIZARDS DS:910c; type from the layout (unused in MAGIC); max 80
                                                   */
int ai_enemy_stack_count;                       /* DS:660a  WIZARDS DS:910a; type from the layout (unused in MAGIC) */
struct AIStack *p_ai_own_stacks;                    /* DS:6608  WIZARDS DS:9108 (by layout; unused in MAGIC) */
struct AIEnemyStack *p_ai_enemy_stacks;         /* DS:6606  WIZARDS DS:9106 (by layout; unused in MAGIC) */
int *ai_lm_own_strength[2];                     /* DS:6602  WIZARDS DS:9102 (by layout; unused in MAGIC) */
int *ai_lm_enemy_strength[2];                   /* DS:65fe  WIZARDS DS:90fe (by layout; unused in MAGIC) */
int *ai_lm_own_value[2];                        /* DS:65fa  WIZARDS DS:90fa (by layout; unused in MAGIC) */
int *ai_lm_target_value[2];                     /* DS:65f6  WIZARDS DS:90f6 (by layout; unused in MAGIC) */
int *ai_lm_surplus[2];                          /* DS:65f2  WIZARDS DS:90f2 (by layout; unused in MAGIC) */
struct Battlefield far *p_battlefield;          /* DS:65ee  WIZARDS DS:9274 */
int combat_wall_rising;                       /* DS:65ec  WIZARDS DS:9272; type from the layout (unused in MAGIC); 1 while a
                                                 Wall of Stone rises (set by SPELLFX.C's cast animation) */
int combat_wall_anim_frame;                          /* DS:65ea  WIZARDS DS:9270; type from the layout (unused in MAGIC);
                                                        wall-section loop counter kept global (Combat_Wall_Create / the
                                                        wall queue functions) */
unsigned combat_scan_icons[18];                 /* DS:65c6  WIZARDS DS:924c */
int p_hero_stats_spare;                         /* DS:65c4  no code in either program reads it */
struct HeroStats far *p_hero_stats[6];          /* DS:65ac */
unsigned char far *p_combat_enchants;           /* DS:65a8  WIZARDS DS:922e */
struct BattleUnit far *p_battle_units;          /* DS:65a4  WIZARDS DS:922a */
struct BattleUnit far *p_unit_stats_buf;        /* DS:65a0 */
unsigned item_icon_pics[116];                   /* DS:64b8  WIZARDS DS:913e */
unsigned char far *p_items_in_game;             /* DS:64b4 */
struct Item far *p_items;                       /* DS:64b0 */
int screen_fade_in;                             /* DS:64ae */
char report_globals_lost;                       /* DS:64ac */
unsigned char report_city_spells_lost[MAX_REPORT_ENTRIES];      /* DS:6498  WIZARDS DS:9c78 */
char report_city_spells_lost_count;             /* DS:6497 */
unsigned char report_unit_spells_lost[MAX_REPORT_ENTRIES];      /* DS:6483  WIZARDS DS:9c63 */
char report_unit_spells_lost_count;             /* DS:6482 */
unsigned char report_city_deaths[MAX_REPORT_ENTRIES];           /* DS:646e  WIZARDS DS:9c4e */
char report_city_deaths_count;                  /* DS:646d */
unsigned char report_city_growth[MAX_REPORT_ENTRIES];           /* DS:6459  WIZARDS DS:9c39 */
char report_city_growth_count;                  /* DS:6458 */
struct UnitKilledRecord report_units_killed[MAX_REPORT_ENTRIES]; /* DS:6408  WIZARDS DS:9be8 */
char report_units_killed_count;                 /* DS:6406 */
struct UnitLostRecord report_units_lost[MAX_REPORT_ENTRIES];    /* DS:63b6  WIZARDS DS:9b96 */
char report_units_lost_count;                   /* DS:63b4 */
struct BuildingLostRecord report_buildings_lost[MAX_REPORT_ENTRIES]; /* DS:6364  WIZARDS DS:9b44 */
char report_buildings_lost_count;                     /* DS:6362 */
struct CityNotice city_notices[MAX_REPORT_ENTRIES];             /* DS:6312  WIZARDS DS:9af2; city notice queue (count
                                                   city_notice_count): 4-byte records {city (0xFF = cleared), -, i16
                                                   producing}; shown by Human_End_Turn */
char city_notice_count;                         /* DS:6310  WIZARDS DS:9af0; the notices in city_notices (max 20) */
unsigned char report_outposts_grown[MAX_REPORT_ENTRIES];        /* DS:62fc  WIZARDS DS:9adc */
char report_outposts_grown_count;               /* DS:62fb */
char report_outposts_deserted_names[14][20];    /* DS:61e3  WIZARDS DS:99c3 (14 names: BUG #375, see WIZARDS.C) */
char report_outposts_deserted_count;            /* DS:61e2 */
struct StackSlot stack_units[MAX_STACK];                /* DS:61be  WIZARDS DS:999e; StackSlot[9] {i16 unit, i16 active}: the
                                                   selected stack - unit index and 1 = moves with the stack (step 78c:
                                                   was stack_units + stack_unit_active, two names for one record array)
                                                   */
int stack_unit_count;                           /* DS:61bc  WIZARDS DS:999c; units in the selected/active stack */
struct Events far *p_events;                    /* DS:61b8 */
int zoom_from_x;                                /* DS:61b6  WIZARDS DS:9996; type from the layout (unused in MAGIC);
                                                   Screen_Present mode 4 (zoom transition) origin x: Screen_Overland_Map
                                                   sets it to the clicked own city cell (cell x * 20) before Screen_City
                                                   / Screen_Outpost */
int zoom_from_y;                                /* DS:61b4  WIZARDS DS:9994; type from the layout (unused in MAGIC); zoom
                                                   origin y (cell y * 18 + 20) */
int zoom_steps;                                 /* DS:61b2  WIZARDS DS:9992; type from the layout (unused in MAGIC); zoom
                                                   step count (8) */
int current_screen;                             /* DS:61b0 */
unsigned p_scratch;                             /* DS:61ae */
unsigned p_pic_work;                            /* DS:61ac */
int move_type_icons_spare;                      /* DS:61aa  the rest of the slot, no reader */
unsigned move_type_icons[9];                    /* DS:6198  WIZARDS DS:9978 */
int wait_button_pic[9];                   /* DS:6186  WIZARDS DS:9966; type from the layout (unused in MAGIC); MAIN.LBX 10:
                                             the WAIT order button at (246,186), named after the first of the nine
                                             order-button words here (WIZARDS: *_button_pic, *_lock_pic greyed) */
int done_lock_pic[6];                   /* DS:617a  WIZARDS DS:995a; type from the layout (unused in MAGIC); MAIN.LBX 12:
                                           the DONE order button at (246,176) */
unsigned patrol_lock_pic;               /* DS:6178  WIZARDS DS:9958; type from the layout (unused in MAGIC); MAIN.LBX 13:
                                           the PATROL order button at (280,176) */
long special_order_blank_pic;                  /* DS:6174  WIZARDS DS:9954; type from the layout (unused in MAGIC); MAIN.LBX
                                                  15: picture drawn on the order button at (280,186) by
                                                  Overland_Draw_Order_Buttons */
unsigned stack_slot_bg_pics[24];                /* DS:6144  WIZARDS DS:9924; MAIN.LBX 0x18..0x20 (unit backgrnd 1-9):
                                                   the frame behind each of the 9 stack panel slots
                                                   (Overland_Draw_Stack_Panel) (step 78c c15) */
unsigned main_screen_bg_pic;                    /* DS:6142  WIZARDS DS:9922; type from the layout (unused in MAGIC);
                                                   MAIN.LBX 0: the overland main-screen frame drawn at (0,0)
                                                   (Overland_Screen_Draw, RoadScr_Draw) */
int diplo_mirror_pic[5];                        /* DS:6138  WIZARDS DS:9918; type from the layout (unused in MAGIC);
                                                   BACKGRND 18 "diplomacy mirror" frame drawn at (0x5f,1) */
int special2_icons[33];                         /* DS:60f6  WIZARDS DS:98d6; type from the layout (unused in MAGIC);
                                                   SPECIAL2.LBX 0..0x21: 34 pictures for 33 words (ASSETS.C overruns by one)
                                                   (Load_Cityscape_Art) (step 78c c15) */
unsigned special_icons[111];                    /* DS:6018  WIZARDS DS:97f8 */
unsigned unitview_stat_icons[15];               /* DS:5ffa  WIZARDS DS:97da */
unsigned unitview_stat_icons_gold[15];          /* DS:5fdc  WIZARDS DS:97bc */
unsigned unitview_build_bg_pic;                 /* DS:5fda  WIZARDS DS:97ba; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 0: window background of the city production preview
                                                   (mode 0) */
unsigned unitview_bg_pic;                       /* DS:5fd8  WIZARDS DS:97b8; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 1: unit view window background (its palette is applied
                                                   when the view opens) */
unsigned unitview_arrow_up_pic;                 /* DS:5fd6  WIZARDS DS:97b6; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 3: list up arrow */
unsigned unitview_arrow_down_pic;               /* DS:5fd4  WIZARDS DS:97b4; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 4: list down arrow */
unsigned unitview_row_frame_pics[8];            /* DS:5fc4  WIZARDS DS:97a4 */
long gold_coin_pic;                             /* DS:5fc0  WIZARDS DS:97a0; type from the layout (unused in MAGIC);
                                                   BACKGRND.LBX 0x2a: one gold coin (building Maintenance row) */
unsigned unitview_portrait_frame_pic;           /* DS:5fbe  WIZARDS DS:979e; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 0x21: frame over the hero portrait */
unsigned unitview_water_bg_pic;                 /* DS:5fbc  WIZARDS DS:979c; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 0x22: water background behind ship / at-sea figures */
unsigned diplo_face_pic;                        /* DS:5fba  WIZARDS DS:979a; type from the layout (unused in MAGIC); MOODWIZ
                                                   entry = portrait: 3 faces, frame 0 good, 1 bad, 2 neutral (LBX
                                                   descriptions "g,b,n") */
int item_power_icons_spare[3];                  /* DS:5fb4  the rest of the slot, no reader */
unsigned item_power_icons[19];                  /* DS:5f8e  WIZARDS DS:976e */
unsigned item_card_pic;                         /* DS:5f8c  WIZARDS DS:976c; type from the layout (unused in MAGIC); ITEMISC
                                                   0x19 item card background */
unsigned item_power_bullet_pic;                 /* DS:5f8a  WIZARDS DS:976a; type from the layout (unused in MAGIC); ITEMISC
                                                   0x1a, blitted before each power line on the item card */
unsigned unitview_slot_icons[6];                /* DS:5f7e  WIZARDS DS:975e */
unsigned city_ench_up_arrow_pic;                /* DS:5f7c  WIZARDS DS:975c */
unsigned city_ench_down_arrow_pic;              /* DS:5f7a  WIZARDS DS:975a */
int city_hover_control[4];                      /* DS:5f72  WIZARDS DS:9752; type from the layout (unused in MAGIC); control
                                                   under the mouse on the city screen (UI_Hit_Test each frame, -1 none)
                                                   - picks the hotspot label / help */
int map_cursor_cell_x;                          /* DS:5f70  WIZARDS DS:9750 */
int map_cursor_cell_y;                          /* DS:5f6e  WIZARDS DS:974e */
int map_scroll_target_x;                                /* DS:5f6c  WIZARDS DS:974c; view position of the last full map
                                                           draw (Map_Redraw skips if unchanged) */
int map_scroll_target_y;                                /* DS:5f6a  WIZARDS DS:974a; the y twin of map_scroll_target_x */
int map_scroll_px_x;                            /* DS:5f68  WIZARDS DS:9748; map scroll in pixels (x*20) */
int map_scroll_px_y;                            /* DS:5f66  WIZARDS DS:9746; map scroll in pixels (y*18) */
int minimap_click_x;                            /* DS:5f64  WIZARDS DS:9744 */
int minimap_click_y;                            /* DS:5f62  WIZARDS DS:9742 */
int map_grid_control;                           /* DS:5f60  WIZARDS DS:9740; type from the layout (unused in MAGIC); control
                                                   id of the overland map grid (-1000 = none) */
int minimap_control;                            /* DS:5f5e  WIZARDS DS:973e; type from the layout (unused in MAGIC); control
                                                   id of the minimap grid (-1000 = none) */
unsigned combat_border_pic;                             /* DS:5f5c  WIZARDS DS:973c size from the layout */
int map_unit_grid[MAP_VIEW_ROWS][MAP_VIEW_COLS];                      /* DS:5e6c  WIZARDS DS:964c */
int unit_figure_pic_spare;                      /* DS:5e6a  no code in either program reads it */
unsigned unit_figure_pic;                       /* DS:5e68 */
int city_screen_city_spare;                     /* DS:5e66  no code in either program reads it */
int city_screen_city;                           /* DS:5e64  WIZARDS DS:9644; city shown on the city screen */
int items_hero_portraits_spare[34];             /* DS:5e20  the rest of the slot, no reader */
unsigned items_hero_portraits[6];               /* DS:5e14  WIZARDS DS:95f4 */
int magicscr_bg_pic[19];                        /* DS:5dee  WIZARDS DS:95ce; type from the layout (unused in MAGIC);
                                                   MAGIC.LBX 0: magic screen background, drawn at 0,0 (MagicScr_Load /
                                                   MagicScr_Draw; live: set on opening the magic screen) */
int magic_overlay_pic[6];                       /* DS:5de2  WIZARDS DS:95c2; type from the layout (unused in MAGIC);
                                                   MAGIC.LBX 0x33 picture used by MagicScr_Draw and the Disjunction
                                                   screen */
unsigned magic_gem_frame_pic;                   /* DS:5de0  WIZARDS DS:95c0; type from the layout (unused in MAGIC);
                                                   MAGIC.LBX 6: picture drawn at each wizard gem position on the magic
                                                   screen (also loaded by Disjunction_Screen_Load) */
int magic_gem_pics_spare[19];                   /* DS:5dba  the rest of the slot, no reader */
int magic_gem_pics[6];                          /* DS:5dae  WIZARDS DS:958e */
unsigned items_bg_pic;                          /* DS:5dac  WIZARDS DS:958c; type from the layout (unused in MAGIC);
                                                   ARMYLIST 5 (also its palette) */
unsigned items_portrait_frame_pic;              /* DS:5daa  WIZARDS DS:958a; type from the layout (unused in MAGIC);
                                                   ARMYLIST 6 */
unsigned items_alchemy_pic;                     /* DS:5da8  WIZARDS DS:9588; type from the layout (unused in MAGIC);
                                                   ARMYLIST 7 */
unsigned items_ok_pic;                          /* DS:5da6  WIZARDS DS:9586; type from the layout (unused in MAGIC);
                                                   ARMYLIST 8 */
unsigned item_empty_slot_pics[6];               /* DS:5d9a  WIZARDS DS:957a */
unsigned armylist_bg_pic;                       /* DS:5d98  WIZARDS DS:9578 */
unsigned armylist_up_arrow_pic;                 /* DS:5d96  WIZARDS DS:9576; type from the layout (unused in MAGIC);
                                                   ARMYLIST.LBX 1 (also the screen palette) */
unsigned armylist_down_arrow_pic;               /* DS:5d94  WIZARDS DS:9574; type from the layout (unused in MAGIC);
                                                   ARMYLIST.LBX 2 */
unsigned armylist_items_btn_pic;                /* DS:5d92  WIZARDS DS:9572; type from the layout (unused in MAGIC);
                                                   ARMYLIST.LBX 3 (Items button) */
unsigned armylist_ok_btn_pic;                   /* DS:5d90  WIZARDS DS:9570; type from the layout (unused in MAGIC);
                                                   ARMYLIST.LBX 4 (OK button) */
unsigned wizinfo_bg_pic;                                /* DS:5d8e  WIZARDS DS:956e size from the layout */
unsigned wizinfo_book_pics[6][3];               /* DS:5d6a  WIZARDS DS:954a */
int unitview_button_panel_pic[5];               /* DS:5d60  WIZARDS DS:9540; type from the layout (unused in MAGIC);
                                                   UNITVIEW.LBX 2: Ok/Dismiss panel at (x+0xd5,y+0x85), not drawn in
                                                   combat */
int main_cancel_button_pic[13];                 /* DS:5d46  WIZARDS DS:9526; type from the layout (unused in MAGIC);
                                                   MAIN.LBX 0x29: button picture of the road-building and spell-target
                                                   screens (at x 280 / 263, y 181) */
int citylist_background_pic[9];                 /* DS:5d34  WIZARDS DS:9514; type from the layout (unused in MAGIC);
                                                   RELOAD.LBX 21 (also the palette) */
unsigned citylist_up_arrow_pic;                 /* DS:5d32  WIZARDS DS:9512; type from the layout (unused in MAGIC);
                                                   ARMYLIST.LBX 1 */
unsigned citylist_down_arrow_pic;               /* DS:5d30  WIZARDS DS:9510; type from the layout (unused in MAGIC);
                                                   ARMYLIST.LBX 2 */
unsigned citylist_ok_btn_pic;                   /* DS:5d2e  WIZARDS DS:950e; type from the layout (unused in MAGIC);
                                                   RELOAD.LBX 22 */
unsigned city_worker_pics[14];                  /* DS:5d12  WIZARDS DS:94f2 */
unsigned food_deficit_x1_pic;                  /* DS:5d10  WIZARDS DS:94f0; type from the layout (unused in MAGIC);
                                                  BACKGRND.LBX 0x5e icon used in the city resource rows
                                                  (CityScr_Add_Resource_Control) */
unsigned resource_x1_pics[5];                   /* DS:5d06  WIZARDS DS:94e6 */
unsigned city_farmer_pics[14];                  /* DS:5cea  WIZARDS DS:94ca */
unsigned city_rebel_pics[14];                   /* DS:5cce  WIZARDS DS:94ae */
unsigned food_deficit_x10_pic;                  /* DS:5ccc  WIZARDS DS:94ac; type from the layout (unused in MAGIC);
                                                   BACKGRND.LBX 0x5f icon used in the city resource rows */
unsigned resource_x10_pics[6];                  /* DS:5cc0  WIZARDS DS:94a0 */
int ng_ok_pic_spare[7];                         /* DS:5cb2  no code in either program reads it (review 36: the
                                                   WIZARDS combat names once given to 5cb2..5cbb had no anchor - the
                                                   neighbour ng_ok_pic is MAGIC-only) */
unsigned ng_ok_pic;                             /* DS:5cb0 */
unsigned alchemy_ok_pic;                                /* DS:5cae  WIZARDS DS:948e size from the layout */
unsigned alchemy_cancel_pic;                            /* DS:5cac  WIZARDS DS:948c size from the layout */
unsigned alchemy_left_arrow_pic;                        /* DS:5caa  WIZARDS DS:948a size from the layout */
unsigned alchemy_right_arrow_pic;                       /* DS:5ca8  WIZARDS DS:9488 size from the layout */
unsigned alchemy_arrow_bar_pic;                         /* DS:5ca6  WIZARDS DS:9486 size from the layout */
unsigned alchemy_pow2gold_pic;                          /* DS:5ca4  WIZARDS DS:9484 size from the layout */
int magic_lock_skill;                           /* DS:5ca2  WIZARDS DS:9482 */
int magic_lock_research;                        /* DS:5ca0  WIZARDS DS:9480; the research staff lock */
int magic_lock_mana;                            /* DS:5c9e  WIZARDS DS:947e; the magic screen's mana staff lock */
int spellbook_small_pic[8];                     /* DS:5c8e  WIZARDS DS:946e; type from the layout (unused in MAGIC);
                                                   SPECIAL2.LBX 34: the small spellbook picture (combat spellbook,
                                                   combat spell info, item charge spell) */
long spellbook_corner_left_pic;                 /* DS:5c8a  WIZARDS DS:946a; type from the layout (unused in MAGIC);
                                                   SPECIAL2.LBX 0x23 drawn at the left page's top (+13, +4)
                                                   (Spellbook_Overland_Redraw, Item_Charge_Spell_Redraw) */
unsigned spellbook_corner_right_pic;            /* DS:5c88  WIZARDS DS:9468; type from the layout (unused in MAGIC);
                                                   SPECIAL2.LBX 0x24 drawn at the right page's top (+258, +4) */
int scroll_bg_pic[4];                           /* DS:5c80  WIZARDS DS:9460; type from the layout (unused in MAGIC);
                                                   SCROLL.LBX 6 background of the spell-learned and apprentice screens
                                                   */
int spellbook_realm_icon_src_pics[6];                 /* DS:5c74  WIZARDS DS:9454 */
unsigned spellbook_page_bg_pic;                 /* DS:5c72  WIZARDS DS:9452; type from the layout (unused in MAGIC);
                                                   SPECIAL2.LBX 0x2b page background decoded into each spellbook page
                                                   (Spellbook_Draw_Page_Overland / _Combat) */
int combat_spellbook_page_spare[2];             /* DS:5c6e  no code in either program reads it */
int combat_spellbook_page;                      /* DS:5c6c */
int overland_spellbook_page;                                   /* DS:5c6a */
int research_first_page;                        /* DS:5c68 */
int stack_fresh_selection;                                   /* DS:5c66  WIZARDS DS:9446; set to 1 by Game_Init_State */
int no_units_to_move;                           /* DS:5c64  WIZARDS DS:9444; 1 when Find_Next_Unit finds nothing (Next
                                                   Turn panel shown) */
int stack_blink_state;                          /* DS:5c62  WIZARDS DS:9442; type from the layout (unused in MAGIC); -2
                                                   hidden, -1 steady, 0..7 blink phase of the active stack */
unsigned cityscape_rubble_pics[4];              /* DS:5c5a  WIZARDS DS:943a */
unsigned event_icon_pics[6];                    /* DS:5c4e  WIZARDS DS:942e */
int never_set_flag_tile_count;                          /* DS:5c4c  WIZARDS DS:942c; type from the layout (unused in MAGIC);
                                                           0..5 (vestigial) */
char never_set_flag_tile_x_spare;                       /* DS:5c4b  the rest of the slot, no reader */
unsigned char never_set_flag_tile_x[5];                 /* DS:5c46  WIZARDS DS:9426 */
unsigned char never_set_flag_tile_y[5];                 /* DS:5c41  WIZARDS DS:9421 */
unsigned char never_set_flag_tile_plane[5];             /* DS:5c3c  WIZARDS DS:941c */
int snd_message;                                /* DS:5c3a  WIZARDS DS:941a; type from the layout (unused in MAGIC);
                                                   SOUNDFX.LBX entry 0 (Play_Message_Sound) */
int notify_realm_pics_spare[4];                 /* DS:5c32  no code in either program reads it */
unsigned notify_realm_pics[5];                  /* DS:5c28 */
int combat_terrain_pics_spare[7];               /* DS:5c1a  the rest of the slot, no reader */
unsigned combat_terrain_pics[48];               /* DS:5bba  WIZARDS DS:939a */
unsigned char far *vis_bitmap_arcanus;          /* DS:5bb6 */
unsigned char far *vis_bitmap_myrror;           /* DS:5bb2 */
unsigned p_music_buf;                           /* DS:5bb0 */
unsigned char (far *p_terrain_stats)[6];        /* DS:5bac */
/* the cityscape_myrror_* pictures: CITYSCAP.LBX labels them "chaos ..."; Cityscape_Draw_Background draws them for a
   Myrror city (the Arcanus ones for an Arcanus city) */
unsigned cityscape_myrror_ground_pic;            /* DS:5baa  WIZARDS DS:938a; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 8 (chaos ground); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_hills_pic;             /* DS:5ba8  WIZARDS DS:9388; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 9 (chaos hills); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_mountain_pic;          /* DS:5ba6  WIZARDS DS:9386; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 0x0a (chaos mountain); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_plains_pic;            /* DS:5ba4  WIZARDS DS:9384; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 0x0b (chaos plains); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_heavenly_light_pic;             /* DS:5ba2  WIZARDS DS:9382; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 0x71 (chaos alkar); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_dark_cloud_pic;        /* DS:5ba0  WIZARDS DS:9380; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 0x6f (chaos dark cloud); loaded by Load_Cityscape_Art
                                                    */
unsigned cityscape_myrror_chaos_rift_pic;              /* DS:5b9e  WIZARDS DS:937e; type from the layout (unused in MAGIC);
                                                          CITYSCAP.LBX 0x70 (chaos rift); loaded by Load_Cityscape_Art
                                                          */
unsigned cityscape_plain_bg_pic;                /* DS:5b9c  WIZARDS DS:937c; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 7 (plain background); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_ocean_pic;             /* DS:5b9a  WIZARDS DS:937a; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 0x74 (chaos ocean); loaded by Load_Cityscape_Art */
unsigned cityscape_myrror_river_pic;             /* DS:5b98  WIZARDS DS:9378; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 0x73 (chaos river); loaded by Load_Cityscape_Art */
unsigned cityscape_arcanus_ground_pic;                     /* DS:5b96  WIZARDS DS:9376; type from the layout
                                                              (unused in MAGIC); CITYSCAP.LBX 0: cityscape background whose
                                                              frame is the DAT_3f33_41f2 state
                                                              (Cityscape_Draw_Background) */
unsigned cityscape_flying_fortress_pic;         /* DS:5b94  WIZARDS DS:9374; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x57 (flying fortress); loaded by Load_Cityscape_Art */
unsigned cityscape_gaia_pic;                    /* DS:5b92  WIZARDS DS:9372; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x5a (gaia); loaded by Load_Cityscape_Art */
unsigned cityscape_living_lands_pic;            /* DS:5b90  WIZARDS DS:9370; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x59 (living lands); loaded by Load_Cityscape_Art */
unsigned cityscape_cursed_land_pic;             /* DS:5b8e  WIZARDS DS:936e; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x56 (cursed land mask); loaded by Load_Cityscape_Art */
unsigned cityscape_river_pic;                   /* DS:5b8c  WIZARDS DS:936c; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 3 (river); loaded by Load_Cityscape_Art */
unsigned cityscape_ocean_pic;                   /* DS:5b8a  WIZARDS DS:936a; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 4 (ocean); loaded by Load_Cityscape_Art */
unsigned cityscape_hills_bg_pic;                /* DS:5b88  WIZARDS DS:9368; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 1 (hills background); loaded by Load_Cityscape_Art */
unsigned cityscape_mountain_bg_pic;              /* DS:5b86  WIZARDS DS:9366; type from the layout (unused in MAGIC);
                                                    CITYSCAP.LBX 2: cityscape / outpost landscape background
                                                    (Cityscape_Draw_Background, Outpost_Draw_Landscape) */
unsigned cityscape_far_dark_cloud_pic;          /* DS:5b84  WIZARDS DS:9364; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x5b (FARBACKS cloud of dark); loaded by
                                                   Load_Cityscape_Art */
unsigned cityscape_far_heavenly_light_pic;               /* DS:5b82  WIZARDS DS:9362; type from the layout
                                                            (unused in MAGIC); CITYSCAP.LBX 0x5d (FARBACKS alkar); loaded by
                                                            Load_Cityscape_Art */
unsigned cityscape_far_chaos_rift_pic;          /* DS:5b80  WIZARDS DS:9360; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x5c (FARBACKS chaos rift); loaded by Load_Cityscape_Art
                                                   */
unsigned cityscape_famine_pic;                  /* DS:5b7e  WIZARDS DS:935e; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x58 (famine); loaded by Load_Cityscape_Art */
unsigned cityscape_death_ward_pic;              /* DS:5b7c  WIZARDS DS:935c; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x5e (black ward); loaded by Load_Cityscape_Art */
unsigned cityscape_chaos_ward_pic;              /* DS:5b7a  WIZARDS DS:935a; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x5f (red ward); loaded by Load_Cityscape_Art */
unsigned cityscape_nature_ward_pic;             /* DS:5b78  WIZARDS DS:9358; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x62 (green ward); loaded by Load_Cityscape_Art */
unsigned cityscape_life_ward_pic;               /* DS:5b76  WIZARDS DS:9356; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x60 (white ward); loaded by Load_Cityscape_Art */
unsigned cityscape_sorcery_ward_pic;            /* DS:5b74  WIZARDS DS:9354; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x61 (blue ward); loaded by Load_Cityscape_Art */
unsigned cityscape_natures_eye_pic;             /* DS:5b72  WIZARDS DS:9352; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x63 (nature's eye); loaded by Load_Cityscape_Art */
unsigned cityscape_inspirations_pic;            /* DS:5b70  WIZARDS DS:9350; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x64 (inspirations); loaded by Load_Cityscape_Art */
unsigned cityscape_prosperity_pic;              /* DS:5b6e  WIZARDS DS:934e; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x65 (prosperity); loaded by Load_Cityscape_Art */
unsigned cityscape_consecration_pic;            /* DS:5b6c  WIZARDS DS:934c; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x66 (consecration); loaded by Load_Cityscape_Art */
unsigned cityscape_wall_of_fire_pic;            /* DS:5b6a  WIZARDS DS:934a; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 0x4d (wall of fire); loaded by Load_Cityscape_Art */
long cityscape_wall_of_darkness_pic;                 /* DS:5b66  WIZARDS DS:9346; type from the layout (unused in MAGIC);
                                                        CITYSCAP.LBX 0x4f (labelled "wall of air" in the LBX; drawn for
                                                        Wall of Darkness); loaded by Load_Cityscape_Art */
unsigned cityscape_roads_pic;                   /* DS:5b64  WIZARDS DS:9344; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 5 (roads); loaded by Load_Cityscape_Art */
unsigned cityscape_house_pics[3][5];            /* DS:5b46  WIZARDS DS:9326 */
unsigned cityscape_tree_pics[3];                /* DS:5b40  WIZARDS DS:9320 */
unsigned cityscape_rocks_pics[3];               /* DS:5b3a  WIZARDS DS:931a */
unsigned cityscape_summon_circle_pic;           /* DS:5b38  WIZARDS DS:9318; type from the layout (unused in MAGIC);
                                                   CITYSCAP.LBX 6 (summon circle); loaded by Load_Cityscape_Art */
int ai_transport_req_plane[15];                 /* DS:5b1a  WIZARDS DS:9098 */
int ai_transport_req_count;                     /* DS:5b18  WIZARDS DS:9096; type from the layout (unused in MAGIC); max 15
                                                   (AI_Add_Transport_Request) */
int ai_own_transport_count;                     /* DS:5b16  WIZARDS DS:9094; type from the layout (unused in MAGIC) */
int ai_target_count;                            /* DS:5b14  WIZARDS DS:9092; type from the layout (unused in MAGIC); max 25
                                                   */
int ai_target_x_spare;                          /* DS:5b12  the rest of the slot, no reader */
int ai_target_x[25];                            /* DS:5ae0  WIZARDS DS:905e */
int ai_target_y[25];                            /* DS:5aae  WIZARDS DS:902c */
int ai_target_defence[25];                      /* DS:5a7c  WIZARDS DS:8ffa */
int ai_target_value[25];                        /* DS:5a4a  WIZARDS DS:8fc8 */
int p_ai_attack_landmass_spare[10];             /* DS:5a36  no code in either program reads it */
int *p_ai_attack_landmass[2];                   /* DS:5a32 */
int *p_ai_expand_landmass[2];                   /* DS:5a2e */
int *p_ai_replan_timer;                         /* DS:5a2c */
int *p_ai_unused_table_1[2];                           /* DS:5a28  set by WIZARDS' GAMEDATA.C, never read */
int *p_ai_unused_table_2[2];                           /* DS:5a24  set by WIZARDS' GAMEDATA.C, never read */
int *p_ai_expand_x[2];                          /* DS:5a20 */
int *p_ai_expand_y[2];                          /* DS:5a1c */
int *p_ai_circle_timer;                         /* DS:5a1a */
struct LandmassPlan ai_landmass_plan[2][5];     /* DS:5312  WIZARDS DS:8890 */
int menu_save_slots[9];                         /* DS:5300 */
int save_list_count;                            /* DS:52fe */
int menu_forced_choice;                         /* DS:52fc */
int load_btn_load_off_pic_spare;                /* DS:52fa  no code in either program reads it */
unsigned load_btn_load_off_pic;                 /* DS:52f8 */
int main_menu_pic2_spare;                       /* DS:52f6  no code in either program reads it */
int main_menu_lower_pic;                             /* DS:52f4 */
int continue_available;                         /* DS:52f2 */
int menu_continue_pic;                          /* DS:52f0 */
int ctl_continue;                               /* DS:52ee */
int menu_new_game_pic;                          /* DS:52ec */
int ctl_new_game;                               /* DS:52ea */
int main_menu_pic;                              /* DS:52e8 */
int load_available_spare;                       /* DS:52e6  no code in either program reads it (review 36: not
                                                   WIZARDS' city_view_y - the menu globals around it are MAGIC-only) */
int load_available;                             /* DS:52e4 */
int portrait_pics_spare;                        /* DS:52e2  no code in either program reads it (review 36: not
                                                   WIZARDS' cityview_unit_list, same reason) */
unsigned portrait_pics[14];                     /* DS:52c6 */
int menu_selection;                             /* DS:52c4 */
int menu_load_pic;                              /* DS:52c2 */
int ctl_cancel;                                 /* DS:52c0  Quit on the main menu, Cancel on the new-game screens */
int menu_hall_of_fame_pic;                      /* DS:52be */
int ctl_hall_of_fame;                           /* DS:52bc */
int menu_quit_pic;                              /* DS:52ba */
unsigned screen_background_pic;                 /* DS:52b8 */
int ctl_load;                                   /* DS:52b6 */
char *p_text_buf;                               /* DS:52b4 */
char *p_text_buf2;                              /* DS:52b2 */

void far *Seg_To_Far_Ptr(unsigned seg);
void Main_Menu_Loop(void);
void Main_Menu_Load_Art(void);
void Main_Menu_Redraw(void);
void Load_Terrain_Stats(void);
void Load_Spell_Data(void);

unsigned char banner_color_bytes[36] = { 105, 106, 107, 108, 109, 73, 74, 75, 76, 77, 121, 122, 123, 124, 125, 201, 165, 203, 166, 45, 160, 161, 162, 178, 180, 48, 49, 50, 51, 52, 98, 216, 123, 43, 179, 50 };    /* DS:00aa  no reader found; identical in WIZARDS at the same address */
int banner_color_words[16] = { 201, 165, 203, 121, 122, 123, 13, 14, 15, 73, 74, 75, 105, 106, 107, 0 };    /* DS:00ce  no reader in MAGIC; with banner_color_bytes the same bytes
                                       as WIZARDS.C's banner_shade_colors + banner_text_colors, split differently */
char *city_size_names[6] = {    /* DS:00ee  Outpost .. Capital */
    "Outpost", "Hamlet", "Village", "Town", "City", "Capital"
};
char *cityscape_special_names[7] = {    /* DS:00fa  as WIZARDS.C's (no reader in MAGIC) */
    "Summoning Circle", "Earth Gate", "Stream Of Life", "Astral Gate", "Fortress", "Dark Rituals", "Altar of Battle"
};
int unused_pair_table[22] = { 140, 0, 130, 5, 120, 10, 110, 15, 100, 20, 90, 25, 80, 30, 70, 35, 60, 40, 50, 45, 40, 50 };    /* DS:0108  no reader; as in WIZARDS.C */
char *city_enchant_names[26] = {    /* DS:0134 */
    "Wall of Fire", "Chaos Rift", "Dark Rituals", "Evil Presence", "Cursed Lands", "Pestilence", "Cloud of Shadow", "Famine", "Flying", "Nature Ward", "Sorcery Ward", "Chaos Ward", "Life Ward", "Death Ward", "Nature's Eye", "Earth Gate", "Stream of Life", "Gaia's Blessing", "Inspirations", "Prosperity", "Astral Gate", "Heavenly Light", "Consecration", "Wall of Darkness", "Altar of Battle", "Nightshade"
};
int unit_ench_upkeep[32] = { 2, 2, 0, 1, 1, 3, 10, 1, 1, 1, 5, 1, 5, 1, 1, 10, 10, 3, 1, 5, 2, 1, 2, 1, 2, 1, 4, 1, 5, 2, 2, 5 };    /* DS:0168  the same values as WIZARDS.C's unit_ench_upkeep, before city_ench_upkeep as there */
int city_ench_upkeep[26] = { 2, 10, 0, 4, 2, 5, 3, 5, 25, 5, 5, 5, 5, 5, 1, 5, 8, 3, 2, 2, 5, 2, 8, 5, 5, 0 };    /* DS:01a8 */
int global_ench_upkeep[24] = { 15, 10, 40, 5, 5, 50, 200, 7, 10, 10, 40, 15, 20, 10, 40, 10, 10, 10, 3, 10, 5, 10, 3, 3 };    /* DS:01dc */
/* columns (struct UnitType): name, melee, ranged, ranged_type, ammo, to_hit, defense, resist, moves, cost,
   upkeep, race, req_building_1, req_building_2, map_figure_pic, hits, scouting, transport_capacity, figures,
   construction, special_strength, move_flags, unused_19, abilities, abilities2, ability_flags, attack_flags,
   sounds, pad_23 */
struct UnitType unit_types[198] = {    /* DS:020c */
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
    { "Great Wyrm", 25, 0, RANGED_NONE, 0, 3, 12, 12, 6, 1000, 20, RACE_NATURE_CREATURE, 6, 0, 0x0, 45, 1, 0, 1, 0, 15, MOVE_MERGING, 0, 0, 0, AFLAG_FANTASTIC, ATTACK_POISON, 0, 0 },
    { "Floating Island", 0, 0, RANGED_NONE, 0, 0, 0, 10, 4, 50, 5, RACE_SORCERY_CREATURE, 6, 0, 0x0, 45, 1, 8, 1, 0, 0, MOVE_SAILING, 0, ABILITY_STONING_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Phantom Beast", 18, 0, RANGED_NONE, 0, 1, 0, 8, 4, 225, 4, RACE_SORCERY_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, ABILITY_STONING_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_NON_CORPOREAL, ATTACK_ILLUSION, 0, 0 },
    { "Phantom Warriors", 3, 0, RANGED_NONE, 0, 0, 0, 6, 2, 20, 2, RACE_SORCERY_CREATURE, 6, 0, 0x0, 1, 1, 0, 6, 0, 0, 0, 0, ABILITY_STONING_IMMUNITY | ABILITY_DEATH_IMMUNITY | ABILITY_POISON_IMMUNITY, 0, AFLAG_FANTASTIC | AFLAG_NON_CORPOREAL, ATTACK_ILLUSION, 0, 0 },
    { "Storm Giant", 12, 10, RANGED_MAGIC_LIGHTNING, 4, 2, 7, 9, 4, 500, 10, RACE_SORCERY_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, 0, 0, 0, 0, AFLAG_FANTASTIC | AFLAG_WALL_CRUSHER, ATTACK_ARMOR_PIERCING, 0, 0 },
    { "Air Elemental", 15, 0, RANGED_NONE, 0, 1, 8, 9, 10, 170, 8, RACE_SORCERY_CREATURE, 6, 0, 0x0, 10, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_STONING_IMMUNITY | ABILITY_POISON_IMMUNITY | ABILITY_WEAPON_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC | AFLAG_INVISIBILITY, 0, 0, 0 },
    { "Djinn", 15, 8, RANGED_MAGIC_WIND, 0, 3, 8, 10, 6, 650, 17, RACE_SORCERY_CREATURE, 6, 0, 0x0, 20, 1, 0, 1, 0, 0, MOVE_FLYING | MOVE_TELEPORTING, 0, ABILITY_FLYING_ANIM | ABILITY_CASTER_MP20, 0, AFLAG_FANTASTIC | AFLAG_WIND_WALKING, 0, 0, 0 },
    { "Sky Drake", 20, 20, RANGED_LIGHTNING, 0, 3, 10, 14, 8, 1000, 25, RACE_SORCERY_CREATURE, 6, 0, 0x0, 25, 1, 0, 1, 0, 0, MOVE_FLYING, 0, ABILITY_ILLUSIONS_IMMUNITY | ABILITY_MAGIC_IMMUNITY | ABILITY_FLYING_ANIM, 0, AFLAG_FANTASTIC, 0, 0, 0 },
    { "Nagas", 4, 0, RANGED_NONE, 0, 1, 3, 7, 2, 120, 2, RACE_SORCERY_CREATURE, 6, 0, 0x0, 6, 1, 0, 2, 0, 4, MOVE_SWIMMING, 0, 0, 0, AFLAG_FANTASTIC, ATTACK_FIRST_STRIKE | ATTACK_POISON, 0, 0 }
};
int exp_level_thresholds[9] = { 0, 20, 60, 120, 200, 300, 450, 600, 1000 };    /* DS:1de4 */
struct RaceInfo race_table[14] = {    /* DS:1df6 */
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
char *unused_empty_string = "";    /* DS:1f46  no reader (as in WIZARDS.C) */
char *arcane_first_realm_names[6] = {    /* DS:1f48  Arcane first: index REALM_* + 1 (not REALM_* order) */
    "Arcane", "Nature", "Sorcery", "Chaos", "Life", "Death"
};
struct MouseRegion mouse_region_arrow = { 1, 0, 0, 0, 319, 199 };    /* DS:1f54 */
/* the full-screen arrow; the same region again, each its own copy in the data: mouse_region_arrow_b (no
   reader), mouse_region_main_menu, NEWGAME.C's mouse_region_new_game, WIZARDS' SETTINGS.C mouse_region_menu */
struct MouseRegion mouse_region_hidden = { 0, 0, 0, 0, 319, 199 };    /* DS:1f60 */
static struct MouseRegion mouse_region_busy = { 6, 0, 0, 0, 319, 199 };    /* DS:1f6c  static here: WORLDGEN.C defines the public one (link test, batch 36) */
struct MouseRegion mouse_region_arrow_b = { 1, 0, 0, 0, 319, 199 };    /* DS:1f78  never referenced */
struct MouseRegion mouse_region_image7 = { 7, 4, 0, 0, 319, 158 };    /* DS:1f84  never referenced */
static unsigned char bit_masks6[6] = { 1, 2, 4, 8, 16, 32 };    /* DS:1f90  never referenced; static: UTIL.C's int bit_masks[8] is the public one */
char *retort_names[18] = {    /* DS:1f96 */
    "Alchemy", "Warlord", "Chaos Mastery", "Nature Mastery", "Sorcery Mastery", "Infernal Power", "Divine Power", "Sage Master", "Channeler", "Myrran", "Archmage", "Mana Focusing", "Node Mastery", "Famous", "Runemaster", "Conjurer", "Charismatic", "Artificer"
};
int random_events_enabled = 1;    /* DS:1fba  WIZARDS.C's variable at the same place after retort_names; MAGIC never reads it */
int personality_hostility_mod[6] = { 40, 20, 10, 10, -40, -20 };    /* DS:1fbc */
struct DifficultyInfo difficulty_table[5] = {    /* DS:1fc8 */
    { 100, 100, 100, 100, 100, 100, 100, 100 },
    { 100, 100, 125, 125, 125, 100, 125, 90 },
    { 150, 150, 150, 150, 150, 100, 150, 75 },
    { 200, 200, 200, 200, 200, 110, 200, 60 },
    { 400, 400, 400, 400, 400, 150, 400, 30 }
};
int UI_Add_Hotkey();                            /* no prototype here: called with an extra word the routine never reads */
unsigned char ai_bldg_objective_bonus[40] = { 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 10, 0, 0, 0, 0, 10, 0, 0, 0, 10, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 5, 0, 0, 5 };    /* DS:2018  WIZARDS DS:203a: [Building.category 0..7][Objective] */
unsigned char ai_unit_objective_bonus[5] = { 0, 5, 0, 0, 3 };    /* DS:2040  the AI tables: data only here, read by WIZARDS' AIPROD */
unsigned char ai_civilian_objective_bonus[5] = { 0, 0, 0, 0, 10 };    /* DS:2045 */
int ai_bldg_category_weight[10] = { 10, 15, 30, 10, 10, 15, 15, 10, 10, 10 };    /* DS:204a */
/* Screen_Main_Menu's: the same full-screen arrow as mouse_region_arrow */
struct MouseRegion mouse_region_main_menu = { 1, 0, 0, 0, 319, 199 };    /* DS:205e */
int magic_unused_206a = 0;    /* DS:206a  no reader */
int menu_tick = 0;    /* DS:206c  reset by Screen_Main_Menu, cycled 0 .. 19 by Main_Menu_Redraw, never read */
unsigned _ovrbuffer = 4096;    /* DS:206e  Borland VROOMM: the overlay buffer, 4096 paragraphs = 64 KB (the overlay manager reads it; defining it keeps OVERLAY.LIB's OVRBUFF default out - link test, batch 36) */

/* (argc, argv) MAGIC.EXE: CONFIG.MOM (the sound drivers chosen by INSTALL) and MAGIC.SET (drops the names of save slots
   whose SAVEn.GAM is gone), engine start-up, the intro unless started with "JENNY" (WIZARDS.EXE passes it on the way back: ENDGAME.C, GAMEMENU.C;
   argv[1] is read with no argc check - with no argument it is NULL and the compare reads DS:0, which never holds
   JENNY),
   the menu art and data, then the main menu, which ends the program itself (Fatal_Exit "Thank you for playing")
   or starts WIZARDS.EXE: the continue-slot save and the memory report after it are never reached */
void main(int argc, char *argv[])
{
    int music;
    FILE *file;
    int i;
    int music_io;
    int music_irq;
    int digi_driver;
    int digi_io;
    int digi_irq;
    int digi_dma;
    char save_filename[40];
    char find_buf[30];
    int music_driver;
    int music_dma;

    ems_reserved_pages = 158;                   /* 16 KB EMS pages that must stay free after the LBX caches
                                                   (about 2.5 MB; EMSLBX.C's default is 40) */
    if (DOS_Find_File("CONFIG.MOM", find_buf) == 0)
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
    game_config.input_type = 1;
    for (i = 1; i < 9; i++) {
        if (game_config.save_slot_used[i - 1] != 0) {
            itoa(i, find_buf, 10);
            strcpy(save_filename, "SAVE");
            strcat(save_filename, find_buf);
            strcat(save_filename, ".GAM");
            DOS_Find_File(save_filename, find_buf);
            if (find_buf[0] == 0) {
                game_config.save_slot_used[i - 1] = 0;
                strcpy(game_config.save_names[i - 1], "");
                file = fopen("MAGIC.SET", "wb");
                fwrite(&game_config, sizeof(struct GameConfig), 1, file);
                fclose(file);
            }
        }
    }
    EMS_Set_Required_K(2700);                   /* the memory messages ask for 2700 KB of EMS and 583 KB free */
    LBX_Set_Min_Free(583);
    game_config.input_type = 1;                 /* forced, whatever MAGIC.SET says (the clamps above are overruled) */
    game_config.sound_channels = 2;
    Engine_Init(game_config.input_type, game_config.sound_channels, "FONTS.LBX", music_driver, music_io, music_irq,
                music_dma, digi_driver, digi_io, digi_irq, digi_dma);
    Release_Version();
    Alloc_Game_Data(6100);                      /* the scratch block, in paragraphs */
    UI_Right_Click_Ids_On();
    Palette_Load(0, -1);
    Palette_Upload();
    if (argv[1][0] == 'J' && argv[1][1] == 'E' && argv[1][2] == 'N' && argv[1][3] == 'N' && argv[1][4] == 'Y') {
    } else {
        Intro_Play();
    }
    Main_Menu_Load_Art();
    Load_Terrain_Stats();
    Load_Spell_Data();
    Main_Menu_Alloc_Credits_Pic();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page_Front();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page();
    Music_Release_Sequence();
    music = LBX_Load("music", 0x68);            /* MUSIC.LBX "MOM50": the main-menu tune */
    if (game_config.music == 1)
        Sound_Play(music);
    Palette_Load(0, -1);
    Palette_Upload();
    Main_Menu_Loop();
    Save_Game(SAVE_SLOT_CONTINUE);
    Memory_Report_Exit();
}

/* () empty in MAGIC (WIZARDS' plays the message sound): the shared message box and Screen_Load_Game call it */
void Play_Message_Sound(void)
{
}

/* () re-reads CONFIG.MOM, then runs the main menu: Continue -> WIZARDS.EXE with the autosave, Load Game, New Game,
   Hall of Fame; Quit leaves the program here through Fatal_Exit (the farewell line) - it never returns */
void Main_Menu_Loop(void)
{
    int choice;
    char found[30];
    FILE *file;
    int quit;

    if (DOS_Find_File("CONFIG.MOM", found) == 0)
        Fatal_Exit("Run INSTALL to configure MASTER OF MAGIC.\n\n");
    file = fopen("CONFIG.MOM", "rb");
    fread(&config_mom, sizeof(struct ConfigMom), 1, file);
    fclose(file);
    quit = 0;
    screen_fade_in = 1;
    menu_forced_choice = -1;
    UI_Right_Click_Ids_On();
    while (quit == 0) {
        Set_Text_Shadow_Color(0);
        UI_Clear_Help();
        choice = Screen_Main_Menu();
        UI_Clear_Help();
        screen_fade_in = 0;
        switch (choice) {
        case MAIN_MENU_CONTINUE:
            Music_Release_Sequence();
            Launch_Wizards(SAVE_SLOT_CONTINUE);
            menu_return_mode = MENU_RETURN_MAP;
            break;
        case MAIN_MENU_LOAD:
            Screen_Load_Game();
            break;
        case MAIN_MENU_NEW_GAME:
            NewGame_Run();
            menu_return_mode = MENU_RETURN_MAP;
            break;
        case MAIN_MENU_QUIT:
            quit = 1;
            menu_return_mode = MENU_RETURN_MAP;
            break;
        case MAIN_MENU_HALL_OF_FAME:
            Screen_Hall_Of_Fame_List();
            Main_Menu_Alloc_Credits_Pic();
            break;
        }
    }
    Fatal_Exit("Thank you for playing Master of Magic!");
}

/* () the main menu pictures (MAINSCRN, VORTEX), the 14 wizard portraits; SPELLDAT.LBX into EMS */
void Main_Menu_Load_Art(void)
{
    int i;

    EMS_Cache_LBX_File_Required("MAINSCRN");
    main_menu_pic = LBX_Load("MAINSCRN", 0);
    menu_continue_pic = LBX_Load("VORTEX", 1);
    menu_hall_of_fame_pic = LBX_Load("VORTEX", 2);
    menu_load_pic = LBX_Load("VORTEX", 5);
    menu_new_game_pic = LBX_Load("VORTEX", 4);
    menu_quit_pic = LBX_Load("VORTEX", 3);
    main_menu_lower_pic = LBX_Load("MAINSCRN", 5);
    EMS_Cache_LBX_File_Required("WIZARDS");
    for (i = 0; i < NUM_WIZARD_PORTRAITS; i++)
        portrait_pics[i] = LBX_Load("WIZARDS", i);
    EMS_Cache_LBX_File_Required("SPELLDAT");
}

/* () -> 0 Continue, 1 Load Game, 2 New Game, 3 Quit, 4 Hall of Fame (or the choice preset in menu_forced_choice):
   Continue only when SAVE9.GAM (the autosave) exists, Load Game when any of SAVE1..8.GAM does */
int Screen_Main_Menu(void)
{
    int i;
    int row;
    int hk_continue;
    int hk_new;
    int hk_quit;
    int faded;
    int hk_esc;
    int hk_hall_of_fame;
    int hk_load;
    char save_filename[30];
    char slot_text[30];
    char find_buf[30];
    int input;
    int done;

    if (menu_forced_choice != -1) {
        i = menu_forced_choice;
        menu_forced_choice = -1;
        return i;
    }
    menu_return_mode = MENU_RETURN_MAP;
    faded = 0;
    continue_available = 0;
    menu_selection = -1;
    load_available = 0;
    menu_tick = 0;
    done = 0;
    screen_fade_in = 1;
    if (screen_fade_in != 0)                    /* always true: as compiled */
        Palette_Fade_To_Black();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page_Front();
    Fill_Rect(0, 0, 319, 199, 0);
    Set_Draw_Page();
    Palette_Load(2, -1);
    Settings_Load_Or_Create();
    help_entries[0].id = -1;
    help_entries[1].id = -1;
    help_entries[2].id = -1;
    help_entries[3].id = -1;
    help_entries[4].id = -1;
    save_list_count = 0;
    for (i = 1; i < 10; i++) {
        strcpy(save_filename, "SAVE");
        itoa(i, slot_text, 10);
        strcat(save_filename, slot_text);
        strcat(save_filename, ".GAM");
        if (DOS_Find_File(save_filename, find_buf) != 0) {
            if (i < 9) {
                menu_save_slots[save_list_count] = i - 1;
                save_list_count++;
            } else {
                continue_available = 1;
            }
        } else {
            menu_save_slots[save_list_count] = -1;
        }
    }
    if (save_list_count > 0)
        load_available = 1;
    UI_Clear_Controls();
    row = 0;
    if (load_available != 0) {
        ctl_load = UI_Add_Region(108, 150, 211, 161, "", -1);
        help_entries[0].id = HELP_LOAD;
        help_entries[0].x1 = 108;
        help_entries[0].y1 = 148;
        help_entries[0].x2 = 211;
        help_entries[0].y2 = 162;
    } else {
        ctl_load = UI_NO_CONTROL;
        row = 1;
    }
    if (continue_available != 0) {
        ctl_continue = UI_Add_Region(108, row * 12 + 138, 211, row * 12 + 149, "", -1);
        help_entries[1].id = HELP_CONTINUE;
        help_entries[1].x1 = 108;
        help_entries[1].y1 = row * 12 + 138;
        help_entries[1].x2 = 211;
        help_entries[1].y2 = row * 12 + 149;
    } else {
        ctl_continue = UI_NO_CONTROL;
    }
    ctl_new_game = UI_Add_Region(108, 162, 211, 173, "", -1);
    help_entries[2].id = HELP_NEW_GAME;
    help_entries[2].x1 = 108;
    help_entries[2].y1 = 162;
    help_entries[2].x2 = 211;
    help_entries[2].y2 = 173;
    ctl_hall_of_fame = UI_Add_Region(108, 174, 211, 185, "", -1);
    help_entries[3].id = HELP_HALL_OF_FAME;
    help_entries[3].x1 = 108;
    help_entries[3].y1 = 174;
    help_entries[3].x2 = 211;
    help_entries[3].y2 = 185;
    ctl_cancel = UI_Add_Region(108, 186, 211, 199, "", -1);
    help_entries[4].id = HELP_QUIT_TO_DOS;
    help_entries[4].x1 = 108;
    help_entries[4].y1 = 186;
    help_entries[4].x2 = 211;
    help_entries[4].y2 = 199;
    if (continue_available != 0)
        hk_continue = UI_Add_Hotkey("C", -1);
    else
        hk_continue = UI_NO_CONTROL;
    if (load_available != 0)
        hk_load = UI_Add_Hotkey("L", -1);
    else
        hk_load = UI_NO_CONTROL;
    hk_new = UI_Add_Hotkey("N", -1);
    hk_hall_of_fame = UI_Add_Hotkey("H", -1);
    hk_quit = UI_Add_Hotkey("Q", -1);
    hk_esc = UI_Add_Hotkey("\x1b", -1);
    Mouse_Set_Cursor_Regions(1, &mouse_region_main_menu);
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
    UI_Warp_Mouse_To_Control(ctl_new_game);
    Pic_Reset_Frame(main_menu_pic);
    menu_selection = -1;
    UI_Set_Redraw_Fn(Main_Menu_Redraw, 2);
    UI_Set_Help(help_entries, 5);
    UI_Input_Delay(4);
    while (done == 0) {
        Timer_Mark();
        input = UI_Get_Input();
        if (input != 0)
            done = 1;
        if (input == hk_continue || input == ctl_continue) {
            done = 1;
            menu_selection = MAIN_MENU_CONTINUE;
        }
        if (input == hk_load || input == ctl_load) {
            done = 1;
            menu_selection = MAIN_MENU_LOAD;
        }
        if (input == hk_new || input == ctl_new_game) {
            done = 1;
            menu_selection = MAIN_MENU_NEW_GAME;
        }
        if (input == hk_quit || input == ctl_cancel || input == hk_esc) {
            done = 1;
            menu_selection = MAIN_MENU_QUIT;
        }
        if (input == hk_hall_of_fame || input == ctl_hall_of_fame) {
            done = 1;
            menu_selection = MAIN_MENU_HALL_OF_FAME;
        }
        if (menu_selection == -1)
            done = 0;
        if (done == 0) {
            Main_Menu_Redraw();
            UI_Present_Frame();
            if (screen_fade_in != 0 && faded == 0) {
                Palette_Fade_From_Black();
                Screen_Copy_Front();
                faded = 1;
                screen_fade_in = 0;
            }
            Wait_Ticks(2);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    return menu_selection;
}

/* () the main menu's redraw callback: the background animation, the credits, the five entries (lit under the mouse) */
void Main_Menu_Redraw(void)
{
    int hit;
    int row;
    int i;
    int frame;
    register int x;
    register int y;

    x = 123;                                    /* the entries' pictures at (123, 141 + 12 x row): Screen_Main_Menu's
                                                   click regions are x 108 .. 211, y 138 + 12 x row .. + 11 */
    y = 141;
    hit = UI_Hit_Test();
    Set_Draw_Page();
    Fill_Rect(0, 0, 319, 199, 0);
    frame = Pic_Get_Frame(main_menu_pic);
    Set_Pic_Frame(main_menu_pic, 0);
    for (i = 0; i <= frame; i++)
        Draw_Pic_Frame(0, 0, main_menu_pic);
    Draw_Pic_Frame(0, 41, main_menu_lower_pic);
    if (screen_fade_in == 0)
        Main_Menu_Draw_Credits();
    row = 0;
    if (load_available != 0) {
        if (hit == ctl_load)
            Pic_Reset_Frame(menu_load_pic);
        else
            Set_Pic_Frame(menu_load_pic, 1);
        Draw_Pic_Frame(x, y + 12, menu_load_pic);
    } else {
        row = 1;
    }
    if (continue_available != 0) {
        if (hit == ctl_continue)
            Pic_Reset_Frame(menu_continue_pic);
        else
            Set_Pic_Frame(menu_continue_pic, 1);
        Draw_Pic_Frame(x, y + row * 12, menu_continue_pic);
    }
    if (hit == ctl_new_game)
        Pic_Reset_Frame(menu_new_game_pic);
    else
        Set_Pic_Frame(menu_new_game_pic, 1);
    Draw_Pic_Frame(x, y + 24, menu_new_game_pic);
    if (hit == ctl_hall_of_fame)
        Pic_Reset_Frame(menu_hall_of_fame_pic);
    else
        Set_Pic_Frame(menu_hall_of_fame_pic, 1);
    Draw_Pic_Frame(x, y + 36, menu_hall_of_fame_pic);
    if (hit == ctl_cancel)
        Pic_Reset_Frame(menu_quit_pic);
    else
        Set_Pic_Frame(menu_quit_pic, 1);
    Draw_Pic_Frame(x, y + 48, menu_quit_pic);
    menu_tick = (menu_tick + 1) % 20;
}

/* () TERRSTAT.LBX -> p_terrain_stats (the world generator's terrain values) */
void Load_Terrain_Stats(void)
{
    p_terrain_stats = Seg_To_Far_Ptr(LBX_Load_Records("TERRSTAT", 0, 0, 770, 6));   /* 770 terrain records x 6 movement-cost layers */
}

/* () SPELLDAT.LBX -> p_spell_data (215 spells x 36 bytes) */
void Load_Spell_Data(void)
{
    p_spell_data = Seg_To_Far_Ptr(LBX_Load_Records("SPELLDAT", 0, 0, NUM_SPELLS, sizeof(struct Spell)));
}

/* () the main menu's 4 help areas from HLPENTRY.LBX */
void Help_Load_Main_Menu(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_MAIN_MENU, help_entries, 0, 4, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 4);
}
