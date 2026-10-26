/* GAME.H - declarations shared by the matched WIZARDS modules (step 78c-V).
   Every line here was proved by a byte-match + fixup check of a module that uses it; add new ones only from a
   matched module. Externs carry their DS offset in a trailing 'DS:xxxx' comment (read by tools/match_fixups.py).
   Modules that must NOT see a prototype (unprototyped calls in the original) declare locally instead. */
#ifndef GAME_H
#define GAME_H
#include "STRUCTS.H"
#include "ENUMS.H"

/* ---- globals (DGROUP order) ---- */
extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */
extern struct MouseRegion mouse_region_hidden;  /* DS:00fa */
extern char *city_size_names[];                 /* DS:0156 */
extern struct UnitType unit_types[];            /* DS:019c */
extern int exp_level_thresholds[9];             /* DS:1d74 */
extern char *race_plural_names[];               /* DS:59b4 */
extern unsigned near_alloc_top;                 /* DS:727c */
extern struct Spell far *p_spell_data;          /* DS:912c */
extern struct Item far *p_items;                /* DS:9136 */
extern struct BattleUnit far *p_unit_stats_buf; /* DS:9226 */
extern struct HeroStats far *p_hero_stats[6];   /* DS:9232 */
extern struct Building far *p_building_data;    /* DS:938c */
extern unsigned char far *vis_bitmap_myrror;    /* DS:9392 */
extern unsigned char far *vis_bitmap_arcanus;   /* DS:9396 */
extern int no_units_to_move;                    /* DS:9444 */
extern int city_screen_city;                    /* DS:9644 */
extern unsigned p_scratch;                      /* DS:998e  segment of the scratch buffer */
extern int current_screen;                      /* DS:9990 */
extern char city_notice_count;                  /* DS:9af0 */
extern struct CityNotice city_notices[];        /* DS:9af2 */
extern unsigned char (far *p_exploration_maps)[MAP_HEIGHT][MAP_WIDTH];          /* DS:9cb0 */
extern struct City far *p_cities;               /* DS:9cbc */
extern struct Fortress far *p_fortresses;       /* DS:9cc8 */
extern struct Unit far *p_units;                /* DS:9ec2 */
extern struct Wizard wizards[6];                /* DS:9eca */
extern int map_view_plane;                      /* DS:bd86 */
extern int map_view_y;                          /* DS:bd88 */
extern int map_view_x;                          /* DS:bd8a */
extern int current_player;                      /* DS:bd8c */
extern int num_units;                           /* DS:bd92 */
extern int num_cities;                          /* DS:bd94 */
extern int num_players;                         /* DS:bd9c */
extern struct GameConfig game_config;           /* DS:bd9e  MAGIC.SET, read / written whole */
extern char *p_text_buf2;                       /* DS:bfba */
extern char *p_text_buf;                        /* DS:bfbc */
extern int building_done_type;                  /* DS:c08c */
                                                /* the building just completed: Popup_Building_Completed names it */
extern char message_text[];                     /* DS:c7fc */
extern unsigned near_alloc_mark;                /* DS:d498 */
extern char near_heap[];                        /* DS:d49a  0x1130 bytes; the first 0x100 are a text scratch */
extern void far *heap_last_farmalloc;           /* DS:e5ca */
extern int required_free_kb;                         /* DS:e5ce */

/* ---- functions ---- */
int AI_Attack_Value(int melee, int ranged, int figures, int attack_flags, int ranged_type);
int AI_Missile_Value(int ranged, int figures, int attack_flags);
int AI_Toughness_Value(int hits, int defense);
void Battle_Unit_Init(int unit, struct BattleUnit far *out);
void Bit_Clear(int bit, char far *map);
void Bit_Set(int bit, char far *map);
int Bit_Test(int bit, char far *map);
void Chaos_Channels_Apply(int unit);
int Check_Release_Version(void);
void City_Remove(int city);
int City_Weapon_Grade(int city);
void Screen_Save_A800(void);
void Screen_Restore_A800(void);
void Screen_Save_AC00(void);
void Draw_Pic_Frame(int x, int y, unsigned pic);
unsigned Far_Peek_Word(unsigned seg, unsigned off);
void Far_Poke_Word(unsigned seg, unsigned off, unsigned value);
void Fatal_Exit(char *message);
void Fill_Rect(int x1, int y1, int x2, int y2, int color);
void Hall_Of_Fame_List_Redraw(void);
int Heap_Block_Valid(unsigned block);
void Heap_Fatal_Error(int error_kind, int amount);
unsigned Heap_Free_Paragraphs(unsigned block);
int Hero_Item_Flight(int unit);
void LBX_Load_Records_Near(char *name, int entry, void *dest, int first, int count, unsigned size);
unsigned LBX_Reload(char *name, int entry, unsigned seg);
void Load_Overland_Screen_Graphics(void);
void Map_Build_Unit_Grid(int x, int y, int plane);
void Map_Center_View(int *view_x, int *view_y, int x, int y, int plane);
void Map_Invalidate(void);
void Mem_Track_Min_Free(void);
void Message_Box(char *text);
void UI_Present_Frame(void);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
int Player_Stacks_Changed_Nop(int player);
char *Near_Alloc(unsigned bytes);
void Near_Alloc_Error(int amount);
char *Near_Alloc_Reset(unsigned bytes);
void Next_Turn(void);
void Next_Unit_Run_GoTos(int player, int *view_x, int *view_y, int *view_plane);
int Overland_Cast_Spell(int player);   /* returns no value: whatever AX holds */
void Palette_Cycle_Reset(void);
void Palette_Fade_To_Black(void);
void Palette_Mark_Range(int first, int last);
void Palette_Unmark_Range(int first, int last);
void Palette_Upload(void);
void Popup_Building_Completed(int unused_a, int unused_b, int city, int building);
int Print_Number_Right(int x, int y, int value);
int Print_Text(int x, int y, char *text);
int Print_Text_Right(int x, int y, char *text);
int Random(int n);                             /* -> 1 .. n (not 0 .. n - 1) */
void Remap_Build_All(void);
void Reset_Clip_Rect(void);
void Screen_City(void);
void Screen_Present(void);
int Screen_Turn_Report(int mode);
void Set_Draw_Page(void);
void Font_Select_Palette(int font, char *colors);
void Set_Text_Shadow_Color(int color);
int Spell_Cost_Reduction_Pct(int player, int spell);
void Stack_Blink_Off(void);
void Stack_Blink_Reset(void);
void Units_Moved_Nop(void);
char *Text_Article_A_An(char *word);
int Tile_Explored(int x, int y, int plane);
void Timer_Mark(void);
int UI_Add_Region(int x1, int y1, int x2, int y2, char *hotkey, int help);
void UI_Clear_Controls(void);
void UI_Clear_Help(void);
void UI_Suspend_Redraw_Fn(void);
int UI_Get_Input(void);
int Unit_Calc_Level(int unit);
int Unit_Can_Fly(int unit);
void Unit_Check_Contact(int unit, int plane);
int Unit_Has_Wind_Walking(int unit);
int Unit_Water_Move_Always_0(int unit);
void Units_Reset_Moves(void);
void Units_Compact(void);
void Units_Set_Draw_Priority(void);
void Units_Update_In_Tower(void);
int Visibility_Mark_Square(int x, int y, int plane, int range);
void Visibility_Rebuild(void);
void Wait_Ticks(int ticks);
int Wizard_Casting_Skill(int player);
int Wizard_Effective_Fame(int player);
void Wizards_Update_Contacts(void);

#endif
