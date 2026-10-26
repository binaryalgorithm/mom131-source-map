/* OUTPOST.C - WIZARDS.EXE overlay code segment 66bf (stub 2488): founding a city with a settler (the settle test:
   water, lairs, towers, nodes, a city within distance 3) and the outpost screen of a size-0 city: the scene, the
   garrison (click: select the stack, right-click: its statistics), the city enchantments (own ones can be turned
   off), the growth houses, the "New Outpost" name entry; the Awareness reveal of every city.
   This module's data (DS:39d6): its literals; BSS (DS:c2a6..c2c0). No <string.h>: strlen() is undeclared (int),
   so 'strlen(name) < 1' is a signed test, as in the original.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 66bf src/wizards/OUTPOST.C */
/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Settler_Found_City(int x, int y, int plane, int race, int owner, int settler);
void Screen_Outpost(int is_new_outpost, int unused_zoom_x, int unused_zoom_y);
void Outpost_Redraw(void);
void Outpost_Screen_Alloc(void);
void Outpost_Garrison_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2);
void Outpost_Name_Dialog(int city, int owner);
void Outpost_Name_Redraw(void);
int Tile_Settle_Blocker(int x, int y, int plane);
void Wizard_Reveal_All_Cities(int player);

#include "GAME.H"

int outpost_garrison_units[10];                 /* DS:c2ac  City_Garrison_List of the outpost */
int outpost_garrison_count;                     /* DS:c2aa */
int outpost_is_new;                             /* DS:c2a8  1: "New Outpost Founded" (garrison not clickable) */
int outpost_unused;                             /* DS:c2a6  set to 0, never read */

extern struct RaceInfo race_table[14];          /* DS:1d86 */
extern char *city_enchant_names[];              /* DS:1ee4 */
extern unsigned p_music_buf;                    /* DS:9390 */
extern unsigned outpost_house_empty_pics[3];    /* DS:9530  BACKGRND.LBX 0x25..0x27, a growth house not yet reached */
extern unsigned outpost_house_pics[3];          /* DS:9536  BACKGRND.LBX 0x22..0x24, by race house style */
extern unsigned name_entry_box_pic;             /* DS:953c  BACKGRND.LBX 0x21: the name-entry box (city, outpost and hero names) */
extern unsigned outpost_bg_pic;                 /* DS:953e  BACKGRND.LBX 0x20 */
extern int present_mode;                        /* DS:9c8e */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int city_ench_down_button;                   /* DS:bf84 */
extern int city_ench_up_button;                   /* DS:bf86 */
extern int city_ench_shown;                  /* DS:bf92 */
extern int city_ench_scroll;                    /* DS:bf96 */
extern int city_ench_list_first;                /* DS:bf98 */
extern int city_ench_count;                     /* DS:bf9a */
extern int *p_city_ench_owners;                 /* DS:bf9c */
extern int *p_city_ench_slots;                  /* DS:bf9e */
extern int *p_city_ench_ctls;                   /* DS:bfa0 */
extern int ui_unit_slot_controls[MAX_STACK];            /* DS:c052 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
int Create_City(void);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Music_Play_Current(void);
int Map_Explore_Radius(int x, int y, int plane, int range);
void City_Pick_Default_Name(int race, char far *name);
void Human_Cities_Area_Nop(void);
void City_Recalculate(int city);
void Road_Build(int x, int y, int plane);
void City_Area_Bitmaps_Rebuild(void);
void Cities_Update_Road_Connections(void);
void Screen_Copy_Front(void);
void City_Garrison_List(int city, int *count, int *list);
void CityScr_Build_Enchant_List(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Help_Load_Outpost(void);
void UI_Input_Delay(int delay);
void CityScr_Add_Enchant_Controls(int x, int y);
void Play_Click_Sound(void);
int Confirm_Box(char *text);
void City_Clear_Enchantment(int city, int slot);
void City_Get_Enchant_List(int city, int *slots, int *owners, int *count);
int Stack_Select_At(int player, int *vx, int *vy, int plane, int x, int y);
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2);
void Outpost_Draw_Scene(int city, int x, int y);
void Draw_Unit_Figure(int x, int y, int unit, int icon_mode);
int City_Population_Tens(int city);
void Set_Font_Color(int color);
void CityScr_Draw_Enchant_List(int x, int y);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Font_Select(int font, int c1, int c2, int c3);
void Set_Space_Width(int width);
int UI_Text_Input_Box(int x, int y, int width, char *buffer, int max_len, int fill, int right_align, int caret_style,
                      int caret_colors, int help);
void Str_Trim(char *text);
void far_strcpy();                             /* unprototyped: also called with (far dst, near src, 0) */
int Tile_Is_Water(int x, int y, int plane);
int Map_Distance(int x1, int y1, int x2, int y2, int width);

/* (x, y, plane, race, owner, settler) -> 1 founded: the square must pass Tile_Settle_Blocker; Create_City (none left
   -> "No more outposts may be built."); the settler goes; the new city is an outpost (population tens 3); the
   human hears the music, sees the outpost screen and names it (explores radius 2), an AI gets a default name;
   wizards with Awareness see it at once; then the road on the square, the city areas and road connections */
int Settler_Found_City(int x, int y, int plane, int race, int owner, int settler)
{
    int ok;
    int sound;
    int city;
    int i;

    ok = Tile_Settle_Blocker(x, y, plane);
    if (ok == SETTLE_OK) {
        if (Create_City() == 1) {
            Unit_Kill_Or_Dismiss(settler, UNIT_REMOVE_DISMISSED);
            city = num_cities - 1;
            city_screen_city = city;
            p_cities[city].population_tens = 3;   /* an outpost starts with 300 people */
            p_cities[city].x = x;
            p_cities[city].y = y;
            p_cities[city].plane = plane;
            p_cities[city].owner = owner;
            p_cities[city].race = race;
            if (owner == current_player) {
                Sound_Stop();
                if (game_config.event_music == 1) {
                    sound = LBX_Reload("MUSIC", MUSIC_NEW_OUTPOST, p_music_buf);
                    Sound_Play(sound);
                }
                Screen_Outpost(1, -1, -1);
                Outpost_Name_Dialog(city_screen_city, owner);
                Music_Play_Current();
                Map_Explore_Radius(x, y, plane, 2);
            } else
                City_Pick_Default_Name(p_cities[city].race, p_cities[city].name);
            for (i = 0; i < num_players; i++) {
                if (wizards[i].globals.awareness > 0) {
                    if (i == current_player)
                        Map_Explore_Radius(x, y, plane, 1);
                    else
                        Bit_Set(i, &p_cities[city].seen_by);
                }
            }
            p_cities[city].x = x;                   /* the same five fields again (as compiled) */
            p_cities[city].y = y;
            p_cities[city].plane = plane;
            p_cities[city].owner = owner;
            p_cities[city].race = race;
            Human_Cities_Area_Nop();
            City_Recalculate(city);
            Road_Build(x, y, plane);
            City_Area_Bitmaps_Rebuild();
            Cities_Update_Road_Connections();
            ok = 1;
        } else {
            if (owner == current_player)
                Message_Box("No more outposts may be built.");
            ok = 0;
        }
    } else
        ok = 0;
    return ok;
}

/* (new city, unused, unused) the outpost screen of city_screen_city until a click outside the controls: the
   garrison (only for an existing outpost: click selects its stack, right-click shows the unit), the enchantment
   list (an own enchantment can be turned off after a confirmation) with its scroll arrows
   Numbered: BUG #337 (BUGS_AND_QUIRKS.txt) */
void Screen_Outpost(int is_new_outpost, int unused_zoom_x, int unused_zoom_y)
{
    int done;
    int leave;
    int x1;
    int y1;
    int x2;
    int y2;
    int ox;
    int oy;
    int skip;
    register int i;
    register int input;

    skip = 0;
    present_mode = PRESENT_NORMAL;
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Present();
    Screen_Save_A800();
    ox = 30;
    oy = 50;
    outpost_is_new = is_new_outpost;
    outpost_garrison_count = 0;
    City_Garrison_List(city_screen_city, &outpost_garrison_count, outpost_garrison_units);
    Outpost_Screen_Alloc();
    CityScr_Build_Enchant_List();
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Outpost_Redraw, 1);
    UI_Clear_Help();
    Help_Load_Outpost();
    outpost_unused = 0;
    UI_Input_Delay(1);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        for (i = 0; i < MAX_STACK; i++)
            ui_unit_slot_controls[i] = UI_NO_SLOT;
        if (outpost_is_new == 0) {
            for (i = 0; i < outpost_garrison_count; i++) {
                Outpost_Garrison_Slot_Rect(i, &x1, &y1, &x2, &y2);
                ui_unit_slot_controls[i] = UI_Add_Region(x1, y1, x2, y2, "", -1);
            }
        }
        CityScr_Add_Enchant_Controls(ox + 114, oy + 54);
        leave = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == leave) {
            Play_Click_Sound();
            done = -1;
        }
        for (i = 0; i < city_ench_shown; i++) {
            if (p_city_ench_ctls[i] == input && p_city_ench_owners[city_ench_list_first + i] == current_player) {
                Play_Click_Sound();
                UI_Clear_Help();
                strcpy(p_text_buf, "Do you wish to turn off the \x02");
                strcat(p_text_buf, city_enchant_names[p_city_ench_slots[city_ench_list_first + i]]);
                strcat(p_text_buf, "\x01 spell?");
                if (Confirm_Box(p_text_buf) == 1) {
                    City_Clear_Enchantment(city_screen_city, p_city_ench_slots[city_ench_list_first + i]);
                    City_Get_Enchant_List(city_screen_city, p_city_ench_slots, p_city_ench_owners, &city_ench_count);
                    city_ench_scroll = 0;
                    if (city_ench_count > NUM_CITY_ENCH_ROWS)
                        city_ench_scroll = 1;
                    else
                        city_ench_list_first = 0;
                }
                UI_Suspend_Redraw_Fn();
                UI_Set_Redraw_Fn(Outpost_Redraw, 1);
                skip = 1;
                Map_Invalidate();
                UI_Clear_Help();
                Help_Load_Outpost();
                break;
            }
        }
        if (input == city_ench_up_button) {
            Play_Click_Sound();
            city_ench_list_first -= NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first < 0)
                city_ench_list_first = city_ench_count / NUM_CITY_ENCH_ROWS * NUM_CITY_ENCH_ROWS;
            skip = 1;
        }
        if (input == city_ench_down_button) {
            Play_Click_Sound();
            city_ench_list_first += NUM_CITY_ENCH_ROWS;
            if (city_ench_list_first >= city_ench_count)
                city_ench_list_first = 0;
            skip = 1;
        }
        for (i = 0; i < outpost_garrison_count; i++) {
            if (ui_unit_slot_controls[i] == input) {
                Play_Click_Sound();
                if (p_cities[city_screen_city].owner == current_player) {
                    next_unit_ref_x = p_cities[city_screen_city].x;
                    next_unit_ref_y = p_cities[city_screen_city].y;
                    Stack_Select_At(current_player, &map_view_x, &map_view_y, map_view_plane, next_unit_ref_x,
                                    next_unit_ref_y);
                }
                done = -1;
                break;
            }
        }
        for (i = 0; i < outpost_garrison_count; i++) {
            if (-ui_unit_slot_controls[i] == input) {
                Play_Click_Sound();
                UI_Clear_Help();
                Outpost_Garrison_Slot_Rect(i, &x1, &y1, &x2, &y2);
                Unit_Stats_Popup(outpost_garrison_units[i], x1 + 1, y1 + 1, x2 + 1, y2 + 1);
                Outpost_Screen_Alloc();
                UI_Suspend_Redraw_Fn();
                UI_Set_Redraw_Fn(Outpost_Redraw, 1);
                UI_Clear_Help();
                Help_Load_Outpost();
                skip = 1;
                break;
            }
        }
        if (done == 0 && skip == 0) {
            Screen_Restore_A800();
            Outpost_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
        skip = 0;
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    present_mode = PRESENT_DISSOLVE;
}

/* () the outpost screen: background, scene, the garrison figures, 10 growth houses (built up to the population
   tens), the title ("New Outpost Founded" / "Outpost Of <name>"), the enchantments, the race name */
void Outpost_Redraw(void)
{
    int oy;
    int x1;
    int y1;
    int x2;
    int y2;
    int tens;
    int house;
    char colors[6];
    register int i;
    register int ox;

    ox = 30;
    oy = 50;
    Reset_Clip_Rect();
    help_entries[0].id = race_table[p_cities[city_screen_city].race].house_style + HELP_OUTPOST_SIZE;
    Set_Draw_Page();
    Draw_Pic_Frame(ox, oy, outpost_bg_pic);
    Outpost_Draw_Scene(city_screen_city, ox + 185, oy + 30);
    Reset_Clip_Rect();
    if (outpost_garrison_count > 0)
        help_entries[1].id = -1;
    else
        help_entries[1].id = HELP_CITY_GARRISONS;
    for (i = 0; i < outpost_garrison_count; i++) {
        Outpost_Garrison_Slot_Rect(i, &x1, &y1, &x2, &y2);
        Draw_Unit_Figure(x1, y1, outpost_garrison_units[i], 1);
    }
    house = race_table[p_cities[city_screen_city].race].house_style;
    tens = City_Population_Tens(city_screen_city);
    for (i = 0; i < 10; i++) {
        x1 = ox + i * 14 + 8;
        y1 = oy + 31;
        if (i < tens)
            Draw_Pic_Frame(x1, y1, outpost_house_pics[house]);
        else
            Draw_Pic_Frame(x1, y1, outpost_house_empty_pics[house]);
    }
    Font_Select_Shadow(5, 5, 0, 0);
    Set_Font_Color(26);
    Set_Text_Shadow_Color(1);
    if (outpost_is_new == 1)
        Print_Text_Centered(ox + 124, oy + 6, "New Outpost Founded");
    else {
        strcpy(p_text_buf, "Outpost Of ");
        far_strcpy(p_text_buf2, 0, p_cities[city_screen_city].name);
        strcat(p_text_buf, p_text_buf2);
        Print_Text_Centered(ox + 124, oy + 6, p_text_buf);
    }
    CityScr_Draw_Enchant_List(ox + 114, oy + 54);
    colors[0] = 0xbe;
    colors[1] = 0xb3;
    Font_Select_Palette(0, colors);
    Set_Text_Shadow_Color(19);
    Font_Select_Shadow(1, 15, 0, 0);
    Print_Text(ox + 6, oy + 23, race_table[p_cities[city_screen_city].race].name);
}

/* () the screen's near buffers (texts, the enchantment lists) and a scratch paragraph */
void Outpost_Screen_Alloc(void)
{
    unsigned block;                             /* written, never read */

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    p_city_ench_slots = (int *)Near_Alloc(NUM_CITY_ENCHANTS * sizeof(int));
    p_city_ench_owners = (int *)Near_Alloc(NUM_CITY_ENCHANTS * sizeof(int));
    p_city_ench_ctls = (int *)Near_Alloc(NUM_CITY_ENCHANTS * sizeof(int));
    block = Heap_Alloc_Front(p_scratch, 1);
}

/* (i, &x1, &y1, &x2, &y2) garrison slot i: 5 per row of 20 x 24 pixels */
void Outpost_Garrison_Slot_Rect(int i, int *x1, int *y1, int *x2, int *y2)
{
    int ox;
    int oy;

    ox = 30;
    oy = 50;
    *x1 = ox + i % 5 * 20 + 6;
    *y1 = oy + i / 5 * 24 + 53;
    *x2 = *x1 + 17;
    *y2 = *y1 + 15;
}

/* (city, owner) the "New Outpost" name entry (12 characters) over the default name; an empty entry keeps it */
void Outpost_Name_Dialog(int city, int owner)
{
    int box;                                    /* written, never read */
    int box_x;
    int box_y;
    char name[16];
    char default_name[16];
    char colors[14];

    present_mode = PRESENT_NORMAL;
    colors[0] = 0x61;
    colors[1] = 0x61;
    colors[2] = 0x62;
    colors[3] = 0x62;
    colors[4] = 0x63;
    colors[5] = 0x63;
    colors[6] = 0x64;
    colors[7] = 0x64;
    colors[8] = 0x65;
    colors[9] = 0x65;
    colors[10] = 0x66;
    colors[11] = 0x66;
    colors[12] = 0x67;
    colors[13] = 0x67;
    box_x = 85;
    box_y = 100;
    UI_Clear_Controls();
    Set_Draw_Page();
    Screen_Copy_Front();
    Screen_Present();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    City_Pick_Default_Name(p_cities[city].race, p_cities[city].name);
    far_strcpy(name, 0, p_cities[city].name);
    strcpy(default_name, name);
    UI_Suspend_Redraw_Fn();
    UI_Set_Redraw_Fn(Outpost_Name_Redraw, 1);
    UI_Clear_Controls();
    Font_Select(4, 4, 3, 3);
    Set_Space_Width(1);
    Set_Font_Color(0x67);
    box = UI_Text_Input_Box(box_x + 16, box_y + 21, 75, name, 12, 0, 0, 0, (int)colors, -1);
    Str_Trim(name);
    if (strlen(name) < 1)
        strcpy(name, default_name);
    far_strcpy(p_cities[city].name, name, 0);
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
}

/* () the name entry's box and its "New Outpost" title */
void Outpost_Name_Redraw(void)
{
    int x;
    int y;

    x = 85;
    y = 100;
    Draw_Pic_Frame(x, y, name_entry_box_pic);
    Font_Select(4, 4, 0, 0);
    Print_Text(x + 10, y + 6, "New Outpost");
}

/* (x, y, plane) -> why a city cannot be founded there: 1 water, 3 a tower, 4 a node, 2 an intact lair, 5 a city
   within distance 3; 0 settleable */
int Tile_Settle_Blocker(int x, int y, int plane)
{
    int dist;
    int i;

    if (Tile_Is_Water(x, y, plane) == 1)
        return SETTLE_WATER;
    for (i = 0; i < NUM_TOWERS; i++) {
        if (p_towers[i].x == x && p_towers[i].y == y)
            return SETTLE_TOWER;
    }
    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].plane == plane && p_nodes[i].y == y && p_nodes[i].x == x)
            return SETTLE_NODE;
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].plane == plane && p_lairs[i].active == 1 && p_lairs[i].y == y && p_lairs[i].x == x)
            return SETTLE_LAIR;
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].plane == plane) {
            dist = Map_Distance(x, y, p_cities[i].x, p_cities[i].y, MAP_WIDTH);
            if (dist <= 3)
                return SETTLE_NEAR_CITY;
        }
    }
    return SETTLE_OK;
}

/* (player) Awareness: the human explores radius 1 around every city, an AI sees every city (seen_by bit) */
void Wizard_Reveal_All_Cities(int player)
{
    int city;

    for (city = 0; city < num_cities; city++) {
        if (player == current_player)
            Map_Explore_Radius(p_cities[city].x, p_cities[city].y, p_cities[city].plane, 1);
        else
            Bit_Set(player, &p_cities[city].seen_by);
    }
}
