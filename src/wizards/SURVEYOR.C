/* SURVEYOR.C - WIZARDS.EXE overlay code segment 7a3c (stub 24de): the Surveyor screen - the overland map with a panel that
   describes the square under the mouse: whether a city may be built there (Tile_Settle_Blocker; "Cities cannot be
   built on water." etc.) or the City Resources a new city would get (City_Site_Evaluate: maximum population,
   production and gold bonus), the terrain type and its yield, a corruption mark, the mineral / special, a lair's or
   node's name and guardians, or a city's size and name; also the river-mouth test (a sea square beside the river).
   This module's data (DS:534e): its literals; BSS (DS:c5d6..c5dc).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7a3c src/wizards/SURVEYOR.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Tile_Next_To_Sea_4(int x, int y, int plane);   /* 4: the four orthogonal neighbours */
void Screen_Surveyor(void);
void Surveyor_Add_Controls(void);
void Surveyor_Redraw(void);
void Surveyor_Show_Tile(void);
void Surveyor_Terrain_Text(int category, int feature);
int Surveyor_Lair_Info(int x, int y, int plane);
int Surveyor_Node_Info(int x, int y, int plane);
int Surveyor_City_At(int x, int y, int plane);
void Surveyor_Font_Heading(void);
void Surveyor_Font_Body(void);

#include "GAME.H"

char *surveyor_title_text;                      /* DS:c5da  100 bytes: the lair's or node's name */
char *surveyor_text;                            /* DS:c5d8  50 bytes: the first guardian / "Unexplored" / the node's spirit */
char *surveyor_text2;                           /* DS:c5d6  50 bytes: the second guardian */

extern char *city_size_names[];                 /* DS:0156 */
extern unsigned survey_bg_pic;                  /* DS:9518 */
extern unsigned button_blockout_pic;            /* DS:9522 */
extern unsigned cast_button_border_pic;         /* DS:9524 */
extern unsigned main_cancel_button_pic;         /* DS:9526 */
extern int minimap_control;                     /* DS:973e */
extern int map_grid_control;                    /* DS:9740 */
extern int minimap_click_y;                     /* DS:9742 */
extern int minimap_click_x;                     /* DS:9744 */
extern int map_scroll_target_y;                         /* DS:974a */
extern int map_scroll_target_x;                         /* DS:974c */
extern int map_cursor_cell_y;                   /* DS:974e */
extern int map_cursor_cell_x;                   /* DS:9750 */
extern unsigned main_screen_bg_pic;             /* DS:9922 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb8 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern int topbar_plane_control;                /* DS:c072 */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern int map_tool_cancel_control;                 /* DS:c130 */

void Set_Text_Shadow_Color(int color);
void UI_Suspend_Redraw_Fn(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
void Minimap_Build_View(void);
void UI_Input_Delay(int delay);
void UI_Clear_Help(void);
void Help_Load_Surveyor(void);
void Timer_Mark(void);
void UI_Clear_Controls(void);
int UI_Add_Hotkey(char *key);
int UI_Get_Input(void);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
void Overland_Plane_Button(int player, int *vx, int *vy, int *plane);
void Map_Window_Origin(int *ox, int *oy, int cx, int cy, int w, int h);
void Mouse_Default_Cursor(void);
void Map_Normalize_Scroll_Target(void);
void Reset_Clip_Rect(void);
void Overland_Add_TopBar_Buttons(void);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void Map_Draw_View(int px, int py, int w, int h, int *vx, int *vy, int plane, int tx, int ty,
                   int player);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Minimap_Draw(int x, int y, int w, int h);
void Overland_Draw_TopBar(void);
void Draw_Gold_Mana_Reserve(void);
int Mouse_X(void);
int Mouse_Y(void);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
int Tile_Settle_Blocker(int x, int y, int plane);
void City_Site_Evaluate(int x, int y, int plane, int *max_pop, int *prod_pct, int *gold_pct, int *unit_cost_pct,
                        int *mineral_gold, int *mineral_power, int *nightshade, int *mithril, int *adamantium,
                        int *coastal, int *unexplored);
int Print_Text(int x, int y, char *text);
int Print_Number_Right(int x, int y, int value);
int Print_Text_Right(int x, int y, char *text);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
int Tile_Is_Forest_Explored(int x, int y, int plane);
int Tile_Is_Volcano(int x, int y, int plane);
int Tile_Is_Mountain(int x, int y, int plane);
int Tile_Is_Hills(int x, int y, int plane);
int Tile_Is_Desert(int x, int y, int plane);
int Tile_Is_Swamp(int x, int y, int plane);
int Tile_Is_Grassland(int x, int y, int plane);
int Tile_Is_Tundra(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Water(int x, int y, int plane);
int Tile_Is_Ocean(int x, int y, int plane);
int Tile_Is_Sea(int x, int y, int plane);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Bit_Test(int bit, char far *map);
void Font_Select_Palette(int font, char *colors);
void Set_Space_Width(int width);
void Set_Line_Spacing(int line_gap);

/* () the Surveyor screen until Cancel (C) / Esc (back to the main screen, 5): the map grid shows the square under the
   mouse in the panel, a right click on the map scrolls it, the minimap recentres the view, the plane button flips
   planes; restores map_view_plane on exit */
void Screen_Surveyor(void)
{
    int hit_test;                               /* read, never used */
    int ox;
    int oy;
    int saved_plane;
    int hotkey;
    register int input;
    register int done;

    Stack_Blink_Off();
    Set_Text_Shadow_Color(0);
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Surveyor_Redraw, 1);
    saved_plane = map_view_plane;
    p_minimap_pic = Heap_Alloc_Front(p_scratch, MINIMAP_PIC_PARAS_SMALL);
    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    surveyor_title_text = Near_Alloc(100);
    surveyor_text = Near_Alloc(50);
    surveyor_text2 = Near_Alloc(50);
    Units_Set_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Map_Invalidate();
    Minimap_Build_View();
    UI_Input_Delay(1);
    UI_Clear_Help();
    Help_Load_Surveyor();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        Surveyor_Add_Controls();
        hotkey = UI_Add_Hotkey("\x1b");
        input = UI_Get_Input();
        hit_test = UI_Hit_Test();
        if (input == map_tool_cancel_control || input == hotkey) {
            Play_Click_Sound();
            current_screen = SCREEN_MAP;
            done = -1;
        }
        if (input == topbar_plane_control) {
            Play_Click_Sound();
            Overland_Plane_Button(current_player, &map_view_x, &map_view_y, &map_view_plane);
            Map_Invalidate();
            Minimap_Build_View();
            UI_Suspend_Redraw_Fn();
            UI_Set_Redraw_Fn(Surveyor_Redraw, 1);
        }
        if (input == minimap_control) {
            Play_Click_Sound();
            Map_Window_Origin(&ox, &oy, (map_view_x + MAP_VIEW_COLS / 2) % MAP_WIDTH, map_view_y + MAP_VIEW_ROWS / 2, MINIMAP_W, MINIMAP_H);
            map_scroll_target_x = minimap_click_x + ox;
            map_scroll_target_y = minimap_click_y + oy;
            map_view_x = map_scroll_target_x;
            map_view_y = map_scroll_target_y;
            Map_Center_View(&map_view_x, &map_view_y, map_scroll_target_x, map_scroll_target_y, map_view_plane);
            Minimap_Build_View();
            Mouse_Default_Cursor();
            Map_Invalidate();
        }
        if (-map_grid_control == input) {
            map_scroll_target_x += map_cursor_cell_x - MAP_VIEW_COLS / 2;
            map_scroll_target_y += map_cursor_cell_y - MAP_VIEW_ROWS / 2;
            Map_Normalize_Scroll_Target();
        }
        if (done == 0) {
            Surveyor_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Reset_Clip_Rect();
    map_view_plane = saved_plane;
}

/* () the top-bar buttons; the map grid and the minimap only while the view is not scrolling; Cancel (C) */
void Surveyor_Add_Controls(void)
{
    map_grid_control = UI_NO_CONTROL;
    minimap_control = UI_NO_CONTROL;
    Overland_Add_TopBar_Buttons();
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        map_grid_control = UI_Add_Grid_Control(0, MAP_VIEW_TOP, MAP_TILE_W, MAP_TILE_H, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_cursor_cell_x, &map_cursor_cell_y, -1);
    if (map_view_x == map_scroll_target_x && map_view_y == map_scroll_target_y)
        minimap_control = UI_Add_Grid_Control(MINIMAP_X, MINIMAP_Y, 1, 1, MINIMAP_W, MINIMAP_H, &minimap_click_x, &minimap_click_y, -1);
    map_tool_cancel_control = UI_Add_Button(263, 181, "", main_cancel_button_pic, "C", -1);
}

/* () redraw: the 12 x 10 map view, the screen frame, minimap, top bar, gold / mana, the panel pictures and the
   square under the mouse (Surveyor_Show_Tile) */
void Surveyor_Redraw(void)
{
    Reset_Clip_Rect();
    Set_Draw_Page();
    Map_Draw_View(0, MAP_VIEW_TOP, MAP_VIEW_COLS, MAP_VIEW_ROWS, &map_view_x, &map_view_y, map_view_plane, map_scroll_target_x, map_scroll_target_y, current_player);
    Draw_Pic_Frame(0, 0, main_screen_bg_pic);
    Minimap_Draw(MINIMAP_X, MINIMAP_Y, MINIMAP_W, MINIMAP_H);
    Overland_Draw_TopBar();
    Draw_Gold_Mana_Reserve();
    Draw_Pic_Frame(242, 173, button_blockout_pic);
    Draw_Pic_Frame(240, 76, survey_bg_pic);
    Draw_Pic_Frame(240, 173, cast_button_border_pic);
    Surveyor_Show_Tile();
    Map_Invalidate();
}

/* () the panel for the explored square under the mouse (nothing off the map): why no city can be built there
   ("less than 3 squares from any other city" when Tile_Settle_Blocker says 5, unless the square IS a city) or the
   City Resources a city would have; then the feature (a city 300+, corruption +1000 / 0x20, mineral, wild game,
   nightshade, lair 100+, node 200+) and the terrain category for Surveyor_Terrain_Text */
void Surveyor_Show_Tile(void)
{
    int y;
    int blocker;
    int max_pop;
    int prod_pct;
    int gold_pct;
    int unit_cost_pct;
    int mineral_gold;
    int mineral_power;
    int nightshade;
    int mithril;
    int adamantium;
    int coastal;
    int unexplored;
    register int x;
    register int feature;

    x = Mouse_X();
    y = Mouse_Y();
    Font_Select_Shadow(4, 0, 0, 0);
    Set_Text_Shadow_Color(148);
    Print_Text_Centered(280, 80, "Surveyor");
    if (x >= MAP_VIEW_RIGHT + 1)
        return;
    if (y <= MAP_VIEW_TOP)
        return;
    x = (x / MAP_TILE_W + map_view_x) % MAP_WIDTH;
    y = y / MAP_TILE_H + map_view_y - 1;        /* QUIRK #481: - 1 row for the 20-pixel bar, so the last 2 pixel rows of a
                                                   square read the square below (MAPDRAW.C: (y - MAP_VIEW_TOP) / MAP_TILE_H) */
    max_pop = 0;
    prod_pct = 0;
    gold_pct = 0;
    if (p_exploration_maps[map_view_plane][y][x] != 0) {
        Surveyor_Font_Heading();
        blocker = Tile_Settle_Blocker(x, y, map_view_plane);
        if (blocker == SETTLE_NEAR_CITY) {
            feature = Surveyor_City_At(x, y, map_view_plane);
            if (feature > 0)
                blocker = SETTLE_OK;
        }
        switch (blocker) {
        case SETTLE_OK:
            City_Site_Evaluate(x, y, map_view_plane, &max_pop, &prod_pct, &gold_pct, &unit_cost_pct, &mineral_gold,
                               &mineral_power, &nightshade, &mithril, &adamantium, &coastal, &unexplored);
            Surveyor_Font_Heading();
            Print_Text_Centered(280, 143, "City Resources");
            Surveyor_Font_Body();
            Print_Text(245, 151, "Maximum Pop");
            Print_Number_Right(306, 151, max_pop);
            Print_Text(245, 158, "Prod Bonus");
            itoa(prod_pct, p_text_buf, 10);
            strcpy(p_text_buf2, "+");
            strcat(p_text_buf2, p_text_buf);
            strcat(p_text_buf2, "%");
            Print_Text_Right(312, 158, p_text_buf2);
            Print_Text(245, 165, "Gold Bonus");
            itoa(gold_pct, p_text_buf, 10);
            strcpy(p_text_buf2, "+");
            strcat(p_text_buf2, p_text_buf);
            strcat(p_text_buf2, "%");
            Print_Text_Right(312, 165, p_text_buf2);
            break;
        case SETTLE_WATER:
            strcpy(p_text_buf, "on water.");
            break;
        case SETTLE_LAIR:
            strcpy(p_text_buf, "on lairs.");
            break;
        case SETTLE_TOWER:
            strcpy(p_text_buf, "on towers.");
            break;
        case SETTLE_NODE:
            strcpy(p_text_buf, "on magic nodes.");
            break;
        case SETTLE_NEAR_CITY:
            strcpy(p_text_buf, "less than 3 squares from any other city.");
            break;
        }
        if (blocker > 0) {
            strcpy(p_text_buf2, "Cities cannot be built ");
            strcat(p_text_buf2, p_text_buf);
            Paragraph_Print(246, 143, 67, p_text_buf2, PARA_LEFT);
        }
    }
    if (p_exploration_maps[map_view_plane][y][x] != 0) {
        feature = Surveyor_City_At(x, y, map_view_plane);
        if (feature == 0)
            feature = p_terrain_flags_maps[map_view_plane][y][x] & TERRAIN_FLAG_CORRUPTION;
        else if (p_terrain_flags_maps[map_view_plane][y][x] & TERRAIN_FLAG_CORRUPTION)
            feature = feature + (p_terrain_flags_maps[map_view_plane][y][x] & TERRAIN_FLAG_CORRUPTION) + SURVEY_CITY_CORRUPT;
        if (feature == 0)
            feature = p_mineral_maps[map_view_plane][y][x] & MINERAL_KIND_MASK;
        if (feature == 0)
            feature = p_mineral_maps[map_view_plane][y][x] & MINFLAG_WILD_GAME;
        if (feature == 0)
            feature = p_mineral_maps[map_view_plane][y][x] & MINFLAG_NIGHTSHADE;
        if (feature == 0)
            feature = Surveyor_Lair_Info(x, y, map_view_plane);
        if (feature == 0)
            feature = Surveyor_Node_Info(x, y, map_view_plane);
        if (Tile_Is_Forest_Explored(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_FOREST, feature);
        else if (Tile_Is_Volcano(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_VOLCANO, feature);
        else if (Tile_Is_Mountain(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_MOUNTAIN, feature);
        else if (Tile_Is_Hills(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_HILLS, feature);
        else if (Tile_Is_Desert(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_DESERT, feature);
        else if (Tile_Is_Swamp(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_SWAMP, feature);
        else if (Tile_Is_Grassland(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_GRASSLAND, feature);
        else if (Tile_Is_Tundra(x, y, map_view_plane) == 1)
            Surveyor_Terrain_Text(TERR_CAT_TUNDRA, feature);
        else if (Tile_Is_River(x, y, map_view_plane) == 1) {
            if (Tile_Next_To_Sea_4(x, y, map_view_plane) == 1)
                Surveyor_Terrain_Text(TERR_CAT_RIVER_MOUTH, feature);
            else
                Surveyor_Terrain_Text(TERR_CAT_RIVER, feature);
        } else if (Tile_Is_Water(x, y, map_view_plane) == 1) {
            if (Tile_Is_Ocean(x, y, map_view_plane) == 1)
                Surveyor_Terrain_Text(TERR_CAT_OCEAN, feature);
            else
                Surveyor_Terrain_Text(TERR_CAT_SHORE, feature);
        } else if (feature == SURVEY_NODE_BASE + 1)   /* node 1 only: no other unclassified square gets a text */
            Surveyor_Terrain_Text(TERR_CAT_FOREST, feature);
    }
}

/* (category, feature) the terrain name and yield: 0 Ocean, 1 Grasslands, 2 Forest, 3 Mountain, 4 Desert, 5 Swamp,
   6 Tundra, 7 Shore, 8 River Mouth, 10 Hills, 11 Plains, 12 River, 13 Volcano (9 prints nothing); then the
   feature: "Corruption" (0x20, or a city square + 1000 + 0x20, reduced back by 1032), a city's size and name
   (300+), a mineral / wild game / nightshade, or the lair's (100+) or node's (200+) texts */
void Surveyor_Terrain_Text(int category, int feature)
{
    Surveyor_Font_Heading();
    strcpy(p_text_buf, "");
    strcpy(p_text_buf2, "");
    switch (category) {
    case TERR_CAT_RIVER:
        Print_Text_Centered(280, 91, "River");
        strcpy(p_text_buf, "2 food");
        strcpy(p_text_buf2, "+20% gold");
        break;
    case TERR_CAT_RIVER_MOUTH:
        Print_Text_Centered(280, 91, "River Mouth");
        strcpy(p_text_buf, "1/2 food");
        strcpy(p_text_buf2, "+30% gold");
        break;
    case TERR_CAT_GRASSLAND:
        Print_Text_Centered(280, 91, "Grasslands");
        strcpy(p_text_buf, "1   1/2 food");
        break;
    case TERR_CAT_FOREST:
        Print_Text_Centered(280, 91, "Forest");
        strcpy(p_text_buf, "1/2 food");
        strcpy(p_text_buf2, "+3% production");
        break;
    case TERR_CAT_MOUNTAIN:
        Print_Text_Centered(280, 91, "Mountain");
        strcpy(p_text_buf, "+5% production");
        break;
    case TERR_CAT_DESERT:
        Print_Text_Centered(280, 91, "Desert");
        strcpy(p_text_buf, "+3% production");
        break;
    case TERR_CAT_SWAMP:
        Print_Text_Centered(280, 91, "Swamp");
        strcpy(p_text_buf, "1/2 food");
        break;
    case TERR_CAT_TUNDRA:
        Print_Text_Centered(280, 91, "Tundra");
        strcpy(p_text_buf, "");
        break;
    case TERR_CAT_HILLS:
        Print_Text_Centered(280, 91, "Hills");
        strcpy(p_text_buf, "1/2 food");
        strcpy(p_text_buf2, "+3% production");
        break;
    case TERR_CAT_PLAINS:
        Print_Text_Centered(280, 91, "Plains");
        strcpy(p_text_buf, "1 food");
        break;
    case TERR_CAT_OCEAN:
        Print_Text_Centered(280, 91, "Ocean");
        strcpy(p_text_buf, "");
        strcpy(p_text_buf2, "");
        break;
    case TERR_CAT_SHORE:
        Print_Text_Centered(280, 91, "Shore");
        strcpy(p_text_buf, "1/2 food");
        strcpy(p_text_buf2, "+10% gold");
        break;
    case TERR_CAT_VOLCANO:
        Print_Text_Centered(280, 91, "Volcano");
        strcpy(p_text_buf, "");
        break;
    }
    Surveyor_Font_Body();
    if (feature == TERRAIN_FLAG_CORRUPTION || feature > SURVEY_CITY_CORRUPT) {
        strcpy(p_text_buf, "Corruption");
        strcpy(p_text_buf2, "");
        if (feature > SURVEY_CITY_CORRUPT)
            feature = feature - (SURVEY_CITY_CORRUPT + TERRAIN_FLAG_CORRUPTION);
    }
    Print_Text_Centered(280, 98, p_text_buf);
    Print_Text_Centered(280, 105, p_text_buf2);
    Surveyor_Font_Heading();
    if (feature >= SURVEY_CITY_BASE) {
        feature -= SURVEY_CITY_BASE;
        strcpy(p_text_buf, city_size_names[p_cities[feature].size]);
        strcat(p_text_buf, " of");
        Print_Text_Centered(280, 114, p_text_buf);
        far_strcpy(p_text_buf, 0, p_cities[feature].name);
        Print_Text_Centered(280, 121, p_text_buf);
    } else {
        strcpy(p_text_buf, "");
        switch (feature) {
        case MINERAL_GOLD:
            Print_Text_Centered(280, 114, "Gold Ore");
            strcpy(p_text_buf, "+3 gold");
            break;
        case MINERAL_SILVER:
            Print_Text_Centered(280, 114, "Silver Ore");
            strcpy(p_text_buf, "+2 gold");
            break;
        case MINERAL_GEMS:
            Print_Text_Centered(280, 114, "Gems");
            strcpy(p_text_buf, "+5 gold");
            break;
        case MINERAL_MITHRIL:
            Print_Text_Centered(280, 114, "Mithril Ore");
            strcpy(p_text_buf, "+1 power");
            break;
        case MINERAL_ADAMANTIUM:
            Print_Text_Centered(280, 114, "Adamantium Ore");
            strcpy(p_text_buf, "+2 power");
            break;
        case MINERAL_QUORK:
            Print_Text_Centered(280, 114, "Quork Crystals");
            strcpy(p_text_buf, "+3 power");
            break;
        case MINERAL_CRYSX:
            Print_Text_Centered(280, 114, "Crysx Crystals");
            strcpy(p_text_buf, "+5 power");
            break;
        case MINERAL_IRON_ORE:
            Print_Text_Centered(280, 114, "Iron Ore");
            strcpy(p_text_buf, "Reduces normal unit cost by 5%");
            break;
        case MINERAL_COAL:
            Print_Text_Centered(280, 114, "Coal");
            strcpy(p_text_buf, "Reduces normal unit cost by 10%");
            break;
        case MINFLAG_WILD_GAME:
            Print_Text_Centered(280, 114, "Wild Game");
            strcpy(p_text_buf, "+2 food");
            break;
        case MINFLAG_NIGHTSHADE:
            Print_Text_Centered(280, 114, "Nightshade");
            strcpy(p_text_buf, "Protects city from spells");
            break;
        case SURVEY_LAIR_BASE + 0:
        case SURVEY_LAIR_BASE + 4:
        case SURVEY_LAIR_BASE + 5:
        case SURVEY_LAIR_BASE + 6:
        case SURVEY_LAIR_BASE + 7:
        case SURVEY_LAIR_BASE + 8:
        case SURVEY_LAIR_BASE + 9:
        case SURVEY_LAIR_BASE + 10:
            Print_Text_Centered(280, 114, surveyor_title_text);
            Surveyor_Font_Body();
            Print_Text_Centered(280, 121, surveyor_text);
            Print_Text_Centered(280, 128, surveyor_text2);
            Surveyor_Font_Heading();
            break;
        }
        if (feature >= SURVEY_NODE_BASE) {
            Print_Text_Centered(280, 114, surveyor_title_text);
            Surveyor_Font_Body();
            Print_Text_Centered(280, 121, surveyor_text);
            Print_Text_Centered(280, 128, surveyor_text2);
        }
        Surveyor_Font_Body();
        Paragraph_Print(247, 121, 66, p_text_buf, PARA_CENTER);
    }
}

/* (x, y, plane) -> 100 + lair type, or 0: an active tower / cave .. fallen temple on the square (the nodes,
   types 1-3, are not shown); sets surveyor_title_text to its name and surveyor_text / surveyor_text2 to its guardians once explored
   ("Unexplored" before) */
int Surveyor_Lair_Info(int x, int y, int plane)
{
    register int lair;
    register int found;

    found = 0;
    for (lair = 0; lair < NUM_LAIRS && found == 0; lair++) {
        if (p_lairs[lair].plane == plane && p_lairs[lair].active == 1
            && (p_lairs[lair].type == LAIR_TOWER || p_lairs[lair].type > LAIR_SORCERY_NODE) && p_lairs[lair].x == x && p_lairs[lair].y == y) {
            switch (p_lairs[lair].type) {
            case LAIR_TOWER:
                strcpy(surveyor_title_text, "Tower");
                break;
            case LAIR_MYSTERIOUS_CAVE:
                strcpy(surveyor_title_text, "Cave");
                break;
            case LAIR_DUNGEON:
                strcpy(surveyor_title_text, "Dungeon");
                break;
            case LAIR_ANCIENT_TEMPLE:
                strcpy(surveyor_title_text, "Temple");
                break;
            case LAIR_ABANDONED_KEEP:
                strcpy(surveyor_title_text, "Keep");
                break;
            case LAIR_MONSTER_LAIR:
                strcpy(surveyor_title_text, "Lair");
                break;
            case LAIR_RUINS:
                strcpy(surveyor_title_text, "Ruins");
                break;
            case LAIR_FALLEN_TEMPLE:
                strcpy(surveyor_title_text, "Temple");
                break;
            }
            strcpy(surveyor_text2, "");
            if (Bit_Test(LAIR_BIT_VISITED, &p_lairs[lair].flags) != 0) {
                strcpy(surveyor_text, unit_types[p_lairs[lair].guard1_type].name);
                if (Bit_Test(LAIR_BIT_FOUGHT, &p_lairs[lair].flags) != 0) {
                    if ((p_lairs[lair].guard2_count & LAIR_GUARDS_LEFT_MASK) > 0)
                        strcpy(surveyor_text2, unit_types[p_lairs[lair].guard2_type].name);
                }
            } else
                strcpy(surveyor_text, "Unexplored");
            found = p_lairs[lair].type + SURVEY_LAIR_BASE;   /* the cases 100 .. 110 below */
        }
    }
    return found;
}

/* (x, y, plane) -> 200 + node, or 0: the node on the square; sets surveyor_title_text to its realm's name and
   surveyor_text to "Warped" / "Guardian Spirit" / "Magic Spirit" when owned, else to the guardians of the lair on
   the same square */
int Surveyor_Node_Info(int x, int y, int plane)
{
    int found;
    int lair_found;
    register int lair;
    register int node;

    found = 0;
    for (node = 0; node < NUM_NODES && found == 0; node++) {
        if (p_nodes[node].plane == plane && p_nodes[node].x == x && p_nodes[node].y == y) {
            switch (p_nodes[node].realm) {
            case NODE_SORCERY:
                strcpy(surveyor_title_text, "Sorcery Node");
                break;
            case NODE_NATURE:
                strcpy(surveyor_title_text, "Nature Node");
                break;
            case NODE_CHAOS:
                strcpy(surveyor_title_text, "Chaos Node");
                break;
            }
            strcpy(surveyor_text, "");
            strcpy(surveyor_text2, "");
            if (p_nodes[node].owner != PLAYER_NONE) {
                if (p_nodes[node].meld_flags & NODE_WARPED)
                    strcpy(surveyor_text, "Warped");
                else if (p_nodes[node].meld_flags & NODE_GUARDIAN_SPIRIT)
                    strcpy(surveyor_text, "Guardian Spirit");
                else
                    strcpy(surveyor_text, "Magic Spirit");
            } else {
                lair_found = 0;
                for (lair = 0; lair < NUM_LAIRS && lair_found == 0; lair++) {
                    if (p_lairs[lair].plane == plane && p_lairs[lair].active == 1 && p_lairs[lair].x == x
                        && p_lairs[lair].y == y) {
                        lair_found = 1;
                        strcpy(surveyor_text2, "");
                        if (Bit_Test(LAIR_BIT_VISITED, &p_lairs[lair].flags) != 0) {
                            strcpy(surveyor_text, unit_types[p_lairs[lair].guard1_type].name);
                            if (Bit_Test(LAIR_BIT_FOUGHT, &p_lairs[lair].flags) != 0) {
                                if ((p_lairs[lair].guard2_count & LAIR_GUARDS_LEFT_MASK) > 0)
                                    strcpy(surveyor_text2, unit_types[p_lairs[lair].guard2_type].name);
                            }
                        } else
                            strcpy(surveyor_text, "Unexplored");
                    }
                }
            }
            found = node + SURVEY_NODE_BASE;
        }
    }
    return found;
}

/* (x, y, plane) -> 300 + the city on the square, or 0 */
int Surveyor_City_At(int x, int y, int plane)
{
    register int found;
    register int city;

    found = 0;
    for (city = 0; city < num_cities && found == 0; city++)
        if (p_cities[city].plane == plane && p_cities[city].x == x && p_cities[city].y == y)
            found = city + SURVEY_CITY_BASE;
    return found;
}

/* () the panel's heading font: font 1 in colors 8 / 161, shadow 148 */
void Surveyor_Font_Heading(void)
{
    char colors[4];

    colors[0] = 8;
    colors[1] = 161;
    colors[2] = 161;
    Font_Select_Palette(1, colors);
    Font_Select_Shadow(1, 15, 0, 0);
    Set_Text_Shadow_Color(148);
    Set_Space_Width(1);
    Set_Line_Spacing(1);
}

/* () the panel's body font: font 1, shadow 148 */
void Surveyor_Font_Body(void)
{
    Font_Select_Shadow(1, 0, 0, 0);
    Set_Text_Shadow_Color(148);
    Set_Space_Width(1);
    Set_Line_Spacing(1);
}

/* (x, y, plane) -> 1 when a square beside it (up / down / right / left, x wrapping) is sea: a river square next to
   the sea is a River Mouth. BUG #201: the square above is checked only when y > 1 (row 1's neighbour, row 0, is
   skipped) */
int Tile_Next_To_Sea_4(int x, int y, int plane)
{
    int nx;

    if (y - 1 > 0 && Tile_Is_Sea(x, y - 1, plane) == 1)
        return 1;
    if (y + 1 < MAP_HEIGHT && Tile_Is_Sea(x, y + 1, plane) == 1)
        return 1;
    nx = (x + 1) % MAP_WIDTH;
    if (Tile_Is_Sea(nx, y, plane) == 1)
        return 1;
    nx = x - 1;
    if (nx < 0)
        nx = MAP_WIDTH - 1;
    if (Tile_Is_Sea(nx, y, plane) == 1)
        return 1;
    return 0;
}
