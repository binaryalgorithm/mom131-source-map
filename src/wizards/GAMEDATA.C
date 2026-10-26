/* GAMEDATA.C - WIZARDS.EXE code segment dc56: start-up allocation of every game data block (EMS handles, the world
   block and its sub-blocks, EMS frame blocks, per-record heaps) and the AI per-plane table pointers (into the neutral player's spell_library).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS dc56 src/wizards/GAMEDATA.C */
#include "GAME.H"

extern int *p_ai_circle_timer;                  /* DS:8f98 */
extern int *p_ai_expand_y[2];                   /* DS:8f9a */
extern int *p_ai_expand_x[2];                   /* DS:8f9e */
extern int *p_ai_unused_table_2[2];                    /* DS:8fa2  never used after this setup */
extern int *p_ai_unused_table_1[2];                    /* DS:8fa6  never used after this setup */
extern int *p_ai_replan_timer;                  /* DS:8faa */
extern int *p_ai_expand_landmass[2];            /* DS:8fac */
extern int *p_ai_attack_landmass[2];            /* DS:8fb0 */
extern unsigned char far *p_items_in_game;      /* DS:913a */
extern unsigned char far *vis_bitmap_myrror;    /* DS:9392 */
extern unsigned char far *vis_bitmap_arcanus;   /* DS:9396 */
extern unsigned p_music_buf;                    /* DS:9390 */
extern unsigned unit_figure_pic;                /* DS:9648 */
extern unsigned p_pic_work;                     /* DS:998c */
extern struct Events far *p_events;             /* DS:9998 */
extern struct PathCacheEntry far *p_path_cache; /* DS:9c90 */
extern unsigned char far *city_area_bits;       /* DS:9c9c */
extern unsigned char far *city_area_shared_bits; /* DS:9ca0 */
extern struct PathWork far *p_path_work;        /* DS:9ca4 */
extern unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH]; /* DS:9cac */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb8 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cd0 */
extern unsigned char (far *p_region_bits_water)[REGION_ROWS][REGION_COLS]; /* DS:9cd4 */
extern unsigned char (far *p_region_bits_land)[REGION_ROWS][REGION_COLS]; /* DS:9cd8 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern int world_block_paras;                   /* DS:9d1a */
extern unsigned world_block_rest;              /* DS:9d1c */
extern unsigned p_screen_gfx;                   /* DS:9d1e */
extern unsigned p_world_block;                /* DS:9d20 */
extern int ems_cont_handle;                     /* DS:9ec0  EMS handle "CONTXXX" (ReMoM EmmHndl_CONTXXX; clarified using ReMoM repo) */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int ems_handle_figures_spare;                   /* DS:cf9e  MAGIC DS:9488  no code in either program reads it */
int ems_figures_handle;                         /* DS:cf9c  MAGIC DS:9486   */
int ems_tiles_handle;                           /* DS:cf9a  MAGIC DS:9484   */

int EMS_Named_Handle(int pages, char *name, int reserved);
unsigned EMS_Frame_Block_First(int paragraphs);
unsigned EMS_Frame_Block_Next(int paragraphs);
unsigned Heap_Alloc_Block(unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned Heap_Free_Paragraphs(unsigned block);
void far *Seg_To_Far_Ptr(unsigned seg);

/* (scratch paragraphs) the game's memory: the named EMS handles for the unit figures and the terrain
   tiles, the scratch / picture / screen-graphics heap blocks, and one world block carved into every map
   layer, the cities, units, nodes, lairs, items, spell data ... (sizes in paragraphs); the AI planning
   tables live in the neutral player's spell_library
   Numbered: BUG #382 (BUGS_AND_QUIRKS.txt) */
void Alloc_Game_Data(int scratch_paras)
{
    unsigned block;

    ems_figures_handle = EMS_Named_Handle(28, "FIGUREX", 1);   /* pages: the combat figure cache */
    ems_tiles_handle = EMS_Named_Handle(3, "TILEXXX", 1);   /* the terrain tiles */
    world_block_paras = 0xdbf;                  /* sizes below in paragraphs of 16 bytes, rounded up */
    p_scratch = Heap_Alloc_Block(scratch_paras + 0x203);
    p_pic_work = Heap_Alloc_Block(0x96);
    p_screen_gfx = Heap_Alloc_Block(0x27b);
    block = Heap_Alloc_Block(world_block_paras);
    p_world_block = block;
    p_cities = Seg_To_Far_Ptr(Heap_Alloc_Front(block, 0x2ca));   /* MAX_CITIES x sizeof(struct City) 0x72 */
    p_terrain_maps = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x25a));   /* 2 planes x 40 x 60 terrain words */
    p_region_bits_land = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0xe));   /* 2 planes x 8 x 12 bytes */
    p_region_bits_water = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0xe));   /* 2 planes x 8 x 12 bytes */
    p_landmass_ids = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x12e));   /* 2 planes x 40 x 60 bytes, as the next three */
    p_mineral_maps = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x12e));
    p_terrain_flags_maps = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x12e));
    p_exploration_maps = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x12e));
    vis_bitmap_arcanus = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x13));   /* a bit per square of one plane (300 bytes) */
    vis_bitmap_myrror = Seg_To_Far_Ptr(Heap_Alloc_Back(block, 0x13));
    world_block_rest = Heap_Alloc_Back(block, Heap_Free_Paragraphs(block) - 1);
    p_path_work = Seg_To_Far_Ptr(EMS_Frame_Block_First(0x278));   /* struct PathWork, 0x2780 bytes */
    p_movement_cost_maps = Seg_To_Far_Ptr(EMS_Frame_Block_Next(0x70a));   /* 2 planes x 6 movement kinds x 40 x 60 bytes */
    city_area_shared_bits = Seg_To_Far_Ptr(EMS_Frame_Block_Next(0x12d));
    city_area_bits = Seg_To_Far_Ptr(EMS_Frame_Block_Next(0x12d));
    p_path_cache = Seg_To_Far_Ptr(EMS_Frame_Block_Next(0x409));   /* PATH_CACHE_SIZE x 0x76 */
    p_hero_stats[0] = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1c));   /* NUM_HERO_TYPES x 0xc for each player */
    p_hero_stats[1] = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1b));
    p_hero_stats[2] = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1b));
    p_hero_stats[3] = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1b));
    p_hero_stats[4] = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1b));
    p_hero_stats[5] = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1b));
    p_units = Seg_To_Far_Ptr(Heap_Alloc_Block(0x7ec));   /* (MAX_UNITS + 9) x 0x20 (the save writes as many) */
    p_unit_stats_buf = Seg_To_Far_Ptr(Heap_Alloc_Block(8));   /* one struct BattleUnit, 0x6e */
    p_nodes = Seg_To_Far_Ptr(Heap_Alloc_Block(0x5c));   /* NUM_NODES x 0x30 */
    p_fortresses = Seg_To_Far_Ptr(Heap_Alloc_Block(3));   /* 6 x 4, as the towers */
    p_towers = Seg_To_Far_Ptr(Heap_Alloc_Block(3));
    p_lairs = Seg_To_Far_Ptr(Heap_Alloc_Block(0x15f));   /* more than the NUM_LAIRS x 0x18 = 2448 bytes used */
    p_events = Seg_To_Far_Ptr(Heap_Alloc_Block(7));   /* struct Events, 0x64 */
    p_hero_names = Seg_To_Far_Ptr(Heap_Alloc_Block(0x25));   /* NUM_HERO_TYPES x 0x10 */
    p_items = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1b1));   /* NUM_ITEMS x 0x32 */
    p_items_in_game = Seg_To_Far_Ptr(Heap_Alloc_Block(0x11));
    p_spell_data = Seg_To_Far_Ptr(Heap_Alloc_Block(0x1e5));   /* NUM_SPELLS x 0x24; orphaned: Load_Spell_Data
                                                   (ASSETS.C) loads SPELLDAT into a new block - about 7.7 KB lost;
                                                   clarified using ReMoM repo */
    unit_figure_pic = Heap_Alloc_Block(0x3c);
    p_music_buf = Heap_Alloc_Block(0x15e);
    /* the AI planning tables live in the NEUTRAL player's spell_library (wizards[5] +0x264, 240 bytes; the neutral
       player never researches): 118 ints, so they are saved and loaded with the Wizard records (step 78c-Z) */
    p_ai_attack_landmass[0] = (int *)wizards[PLAYER_NEUTRAL].spell_library[0];
    p_ai_attack_landmass[1] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x0c);   /* 6 ints per table: one per player */
    p_ai_replan_timer = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x18);
    p_ai_unused_table_1[0] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x28);
    p_ai_unused_table_1[1] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x3c);
    p_ai_unused_table_2[0] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x50);
    p_ai_unused_table_2[1] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x64);
    p_ai_expand_landmass[0] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x78);
    p_ai_expand_landmass[1] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0x8c);
    p_ai_expand_x[0] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0xa0);
    p_ai_expand_x[1] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0xb4);
    p_ai_expand_y[0] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0xc8);
    p_ai_expand_y[1] = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0xd4);
    p_ai_circle_timer = (int *)(wizards[PLAYER_NEUTRAL].spell_library[0] + 0xe0);
    ems_cont_handle = EMS_Named_Handle(4, "CONTXXX", 1);
}
