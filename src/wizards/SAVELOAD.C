/* SAVELOAD.C - WIZARDS.EXE code segment 4558 (MAGIC.EXE 3c7e, the same file): SAVEn.GAM - the 29 blocks of the game
   state written / read in one fixed order (123,300 bytes; map in ORIGINAL_NOTES "SAVE FILE MAP"). The block calls go
   through DOS_File_Write_Far / DOS_File_Read_Far (DOSFILE.C) WITHOUT a prototype in scope: every call passes 5 words
   (buffer offset, segment - 0 = DS -, record size, count, and the handle, which those functions ignore: they use the
   handle DOS_File_Open stored). A failed open is never noticed (BUG #334).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 4558 src/wizards/SAVELOAD.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     DOS_File_Close (1 argument; the DOSFILE.C definition takes none);
     Mem_Clear_Far (2; UTIL.C takes 3). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GAME.H"

#define SAVE_FILE_SIZE 123300L          /* a save with the hero-name block; older ones lack it */

extern unsigned char far *p_items_in_game;      /* DS:913a */
extern struct Events far *p_events;             /* DS:9998 */
extern unsigned char (far *p_movement_cost_maps)[6][MAP_HEIGHT][MAP_WIDTH]; /* DS:9cac */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb8 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cd0 */
extern unsigned char (far *p_region_bits_water)[8][12]; /* DS:9cd4 */
extern unsigned char (far *p_region_bits_land)[8][12]; /* DS:9cd8 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern int grand_vizier;                        /* DS:9d22 */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */
extern int top_unit_in_stack;                   /* DS:bd8e */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern int magic_setting;                       /* DS:bd98 */
extern int land_size;                           /* DS:bd9a */

long DOS_File_Size(char *name);
int DOS_File_Read_Far();                       /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
int DOS_File_Write_Far();                      /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
void DOS_File_Close();                         /* no prototype here: called with the handle; DOSFILE.C takes none (it closes the stored one) */

/* (slot 0..8) writes SAVE<slot+1>.GAM; slot < 8 (not SAVE_SLOT_CONTINUE, SAVE9, the continue save the autosave writes) also marks the slot used in game_config and
   rewrites MAGIC.SET */
void Save_Game(int slot)
{
    FILE *fp;
    char name[20];
    char num_text[20];
    int handle;

    itoa(slot + 1, num_text, 10);
    strcpy(name, "SAVE");
    strcat(name, num_text);
    strcat(name, ".GAM");
    handle = DOS_File_Open(name, "wb");
    DOS_File_Write_Far(p_hero_stats[0], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Write_Far(p_hero_stats[1], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Write_Far(p_hero_stats[2], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Write_Far(p_hero_stats[3], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Write_Far(p_hero_stats[4], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Write_Far(p_hero_stats[5], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Write_Far(&num_players, 0, 2, 1, handle);
    DOS_File_Write_Far(&land_size, 0, 2, 1, handle);
    DOS_File_Write_Far(&magic_setting, 0, 2, 1, handle);
    DOS_File_Write_Far(&difficulty, 0, 2, 1, handle);
    DOS_File_Write_Far(&num_cities, 0, 2, 1, handle);
    DOS_File_Write_Far(&num_units, 0, 2, 1, handle);
    DOS_File_Write_Far(&game_turn, 0, 2, 1, handle);
    DOS_File_Write_Far(&top_unit_in_stack, 0, 2, 1, handle);
    DOS_File_Write_Far(wizards, 0, sizeof(struct Wizard), MAX_PLAYERS, handle);
    DOS_File_Write_Far(p_terrain_maps, MAP_SQUARES * 2, 2, handle);
    DOS_File_Write_Far(p_region_bits_land, REGION_ROWS * REGION_COLS, 2, handle);
    DOS_File_Write_Far(p_region_bits_water, REGION_ROWS * REGION_COLS, 2, handle);
    DOS_File_Write_Far(p_landmass_ids, MAP_SQUARES, 2, handle);
    DOS_File_Write_Far(p_nodes, sizeof(struct Node), NUM_NODES, handle);
    DOS_File_Write_Far(p_fortresses, sizeof(struct Fortress), MAX_PLAYERS, handle);
    DOS_File_Write_Far(p_towers, sizeof(struct Tower), NUM_TOWERS, handle);
    DOS_File_Write_Far(p_lairs, sizeof(struct Lair), NUM_LAIRS, handle);
    DOS_File_Write_Far(p_items, sizeof(struct Item), NUM_ITEMS, handle);
    DOS_File_Write_Far(p_cities, sizeof(struct City), MAX_CITIES, handle);
    DOS_File_Write_Far(p_units, sizeof(struct Unit), MAX_UNITS + 9, handle);   /* 9 records past MAX_UNITS, allocated too; why 9 is not known (MAX_STACK is 9; ReMoM does not know either) */
    DOS_File_Write_Far(p_mineral_maps, MAP_SQUARES, 2, handle);
    DOS_File_Write_Far(p_exploration_maps, MAP_SQUARES, 2, handle);
    EMS_Map_Default();
    DOS_File_Write_Far(p_movement_cost_maps, MAP_SQUARES * 6, 2, handle);
    DOS_File_Write_Far(p_events, sizeof(struct Events), 1, handle);
    DOS_File_Write_Far(p_terrain_flags_maps, MAP_SQUARES, 2, handle);
    DOS_File_Write_Far(&grand_vizier, 0, 2, 1, handle);
    DOS_File_Write_Far(p_items_in_game, 1, NUM_PREFAB_ITEMS, handle);
    DOS_File_Write_Far(p_hero_names, sizeof(struct HeroName), NUM_HERO_TYPES, handle);
    DOS_File_Close(handle);
    if (slot < SAVE_SLOT_CONTINUE) {
        game_config.save_slot_used[slot] = 1;
        fp = fopen("MAGIC.SET", "wb");
        fwrite(&game_config, sizeof(game_config), 1, fp);
        fclose(fp);
    }
}

/* (slot) reads the same blocks; the last (p_hero_names) only from a file of exactly 123,300 bytes, else it is cleared
   (only 0x221 of its 0x230 bytes) - saves from before that block existed still load
   Numbered: BUG #105 (BUGS_AND_QUIRKS.txt) */
void Load_Game(int slot)
{
    char file_name[20];
    char num_text[20];
    int handle;
    int old_save;

    itoa(slot + 1, num_text, 10);
    strcpy(file_name, "SAVE");
    strcat(file_name, num_text);
    strcat(file_name, ".GAM");
    old_save = 0;
    if (DOS_File_Size(file_name) != SAVE_FILE_SIZE)
        old_save = 1;
    handle = DOS_File_Open(file_name, "rb");
    DOS_File_Read_Far(p_hero_stats[0], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Read_Far(p_hero_stats[1], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Read_Far(p_hero_stats[2], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Read_Far(p_hero_stats[3], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Read_Far(p_hero_stats[4], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Read_Far(p_hero_stats[5], sizeof(struct HeroStats), NUM_HERO_TYPES, handle);
    DOS_File_Read_Far(&num_players, 0, 2, 1, handle);
    DOS_File_Read_Far(&land_size, 0, 2, 1, handle);
    DOS_File_Read_Far(&magic_setting, 0, 2, 1, handle);
    DOS_File_Read_Far(&difficulty, 0, 2, 1, handle);
    DOS_File_Read_Far(&num_cities, 0, 2, 1, handle);
    DOS_File_Read_Far(&num_units, 0, 2, 1, handle);
    DOS_File_Read_Far(&game_turn, 0, 2, 1, handle);
    DOS_File_Read_Far(&top_unit_in_stack, 0, 2, 1, handle);
    DOS_File_Read_Far(wizards, 0, sizeof(struct Wizard), MAX_PLAYERS, handle);
    DOS_File_Read_Far(p_terrain_maps, MAP_SQUARES * 2, 2, handle);
    DOS_File_Read_Far(p_region_bits_land, REGION_ROWS * REGION_COLS, 2, handle);
    DOS_File_Read_Far(p_region_bits_water, REGION_ROWS * REGION_COLS, 2, handle);
    DOS_File_Read_Far(p_landmass_ids, MAP_SQUARES, 2, handle);
    DOS_File_Read_Far(p_nodes, sizeof(struct Node), NUM_NODES, handle);
    DOS_File_Read_Far(p_fortresses, sizeof(struct Fortress), MAX_PLAYERS, handle);
    DOS_File_Read_Far(p_towers, sizeof(struct Tower), NUM_TOWERS, handle);
    DOS_File_Read_Far(p_lairs, sizeof(struct Lair), NUM_LAIRS, handle);
    DOS_File_Read_Far(p_items, sizeof(struct Item), NUM_ITEMS, handle);
    DOS_File_Read_Far(p_cities, sizeof(struct City), MAX_CITIES, handle);
    DOS_File_Read_Far(p_units, sizeof(struct Unit), MAX_UNITS + 9, handle);
    DOS_File_Read_Far(p_mineral_maps, MAP_SQUARES, 2, handle);
    DOS_File_Read_Far(p_exploration_maps, MAP_SQUARES, 2, handle);
    EMS_Map_Default();
    DOS_File_Read_Far(p_movement_cost_maps, MAP_SQUARES * 6, 2, handle);
    DOS_File_Read_Far(p_events, sizeof(struct Events), 1, handle);
    DOS_File_Read_Far(p_terrain_flags_maps, MAP_SQUARES, 2, handle);
    DOS_File_Read_Far(&grand_vizier, 0, 2, 1, handle);
    DOS_File_Read_Far(p_items_in_game, 1, NUM_PREFAB_ITEMS, handle);
    if (old_save == 0)
        DOS_File_Read_Far(p_hero_names, sizeof(struct HeroName), NUM_HERO_TYPES, handle);
    else
        Mem_Clear_Far(p_hero_names, 0x221);
    DOS_File_Close(handle);
}
