/* ITEMSCR.C - WIZARDS.EXE overlay code segment 6356 (stub 247c): the Items screen - the six hired heroes' item slots and the
   four-slot vault (pick up, drop and swap items, with a 20-mana charge for a transfer between places and the
   drowning check for heroes on water), the anvil (break an item for half its cost in mana), Alchemy, the hero and
   item popups, and the award queue that places new items (treasure, events, merchants, spells) through it.
   This module's data (DS:36a0): its literals ("" is the tail of "\x1b": -d); BSS (DS:c25e..c296).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6356 src/wizards/ITEMSCR.C */
#include <string.h>
#include <stdlib.h>

/* the module's functions in their original declaration order */
void Screen_Items(void);
void Items_Screen_Redraw(void);
void Items_Screen_Draw(void);
void Items_Screen_Load(void);
int Items_Transfer_Is_Free(int source, int dest);
void Items_Draw_Held_Item(int x, int y, int item);
void Items_Break_Item(void);
void Items_Slot_Click(int owner, int slot);
void Items_Screen_Add_Controls(void);
void Items_Screen_Award(int count, int *items);
void Items_Hero_Portrait_Rect(int hero, int *x1, int *y1, int *x2, int *y2);
int Items_Removal_Allowed(int hero, int item);

#include "GAME.H"

int held_item;                                  /* DS:c294  the item on the cursor, -1 */
int held_item_source;                           /* DS:c292  where it came from: hero 0-5, ITEM_OWNER_VAULT (vault / new), ITEM_OWNER_AWARD */
int items_anvil_ctrl;                           /* DS:c290 */
int items_win_x;                                /* DS:c28e */
int items_win_y;                                /* DS:c28c */
int items_alchemy_btn;                          /* DS:c28a */
int items_ok_btn;                               /* DS:c288 */
int items_window_ctrl;                          /* DS:c286 */
int items_outside_ctrl;                         /* DS:c284 */
int *p_items_vault_ctrls;                       /* DS:c282  4 controls */
int *p_items_slot_ctrls[6];                     /* DS:c276  per hero: 3 slot controls */
int *p_items_portrait_ctrls;                    /* DS:c274  6 controls */
int award_count;                                /* DS:c272 */
int award_item;                                 /* DS:c270  the item being placed */
int award_next;                                 /* DS:c26e */
int award_pending;                                 /* DS:c26c  1 while items are queued */
int award_items[6];                             /* DS:c260  the queue (Items_Screen_Award) */
unsigned p_item_icon_pic;                       /* DS:c25e  30-paragraph item icon buffer */

extern struct Item far *p_items;                /* DS:9136 */
extern unsigned item_empty_slot_pics[6];        /* DS:957a  by HiredHero.slot_type - 1 */
extern unsigned items_ok_pic;                   /* DS:9586 */
extern unsigned items_alchemy_pic;              /* DS:9588 */
extern unsigned items_portrait_frame_pic;       /* DS:958a */
extern unsigned items_bg_pic;                   /* DS:958c */
extern unsigned items_hero_portraits[6];        /* DS:95f4 */
extern int current_screen;                      /* DS:9990 */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int current_player;                      /* DS:bd8c */
extern char *p_text_buf2;                       /* DS:bfba */
extern char *p_text_buf;                        /* DS:bfbc */
extern int award_loc_plane;                     /* DS:c300 */
extern int award_loc_y;                         /* DS:c302 */
extern int award_loc_x;                         /* DS:c304 */
extern int award_at_location;                   /* DS:c306 */
extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */

int Tile_Is_Water(int x, int y, int plane);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
int Unit_Has_Wind_Walking(int unit);
int Unit_Water_Move_Always_0(int unit);
int Hero_Item_Water_Walking(int unit);
int Hero_Item_Flight(int unit);
int Unit_Can_Cross_Water(int unit);
void Item_Build_Icon(int item, unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Print_Text_Clipped_Centered(int x, int y, char *text);
void Item_Award_Popup(void);
void Item_View_Popup(int item, int x, int y);
void Unit_Stats_Popup(int unit, int from_x1, int from_y1, int from_x2, int from_y2);
void Alchemy_Dialog(int x, int y);
void Help_Load_Items(void);
int UI_Add_Hotkey(char *key);
int UI_Hit_Test(void);
void Play_Click_Sound(void);
void Play_Click_Sound_Casting(void);
void Pic_Apply_Palette(unsigned pic);
void Swap_Words(int *a, int *b);
int Mouse_X(void);
int Mouse_Y(void);
void Item_Outline_Cycle_Tick(void);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Set_Font_Color(int color);
void Warning_Box(char *text);
int Confirm_Box(char *text);
void Wizard_Add_Mana(int player, int amount);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Screen_Copy_Front(void);

/* () the Items screen (Main_Loop screen 9, from the Armies screen; also nested by Items_Screen_Award): queued
   award items are offered one at a time and picked up; a click on a hero or vault slot picks up / drops / swaps
   (Items_Slot_Click), a right click views an item or (on a portrait) the hero, the anvil breaks the held item,
   Alchemy converts gold and mana; OK, Esc or a click outside leave only with nothing held. The right-click item
   views read wizard 0's slots (BUG #464 when another player is current) */
void Screen_Items(void)
{
    int i;
    int j;                                      /* the slot index, later the hero's unit (for the stats popup) */
    int done;
    int input;
    int hit;                                    /* UI_Hit_Test, never read */
    int ix;
    int iy;
    int hotkey;
    unsigned item_pic;                          /* allocated for the item view, never read */
    int x1;
    int y1;
    int x2;
    int y2;

    Items_Screen_Load();
    items_win_x = 17;
    items_win_y = 2;
    held_item = -1;
    held_item_source = -1;
    UI_Clear_Controls();
    Set_Draw_Page();
    Pic_Apply_Palette(items_bg_pic);
    Palette_Upload();
    Screen_Save_A800();
    UI_Clear_Help();
    UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
    Help_Load_Items();
    done = 0;
    while (done == 0) {
        if (award_pending == 1 && held_item == -1) {
            award_item = award_items[award_next];
            award_next++;
            if (award_next == award_count)
                award_pending = 0;
            Items_Screen_Redraw();
            Item_Award_Popup();
            held_item = award_item;
            held_item_source = ITEM_OWNER_VAULT;
            if (award_at_location == 1)
                held_item_source = ITEM_OWNER_AWARD;
            UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
            Items_Screen_Load();
            continue;
        }
        Timer_Mark();
        UI_Clear_Controls();
        Items_Screen_Add_Controls();
        hotkey = UI_Add_Hotkey("\x1b");
        input = UI_Get_Input();
        hit = UI_Hit_Test();
        if ((input == items_ok_btn && held_item == -1) || (input == items_outside_ctrl && held_item == -1)
            || (input == hotkey && held_item == -1)) {
            Play_Click_Sound();
            done = 1;
        }
        if (input == items_alchemy_btn) {
            Play_Click_Sound();
            Alchemy_Dialog(items_win_x + 63, items_win_y + 58);
            UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
            Items_Screen_Load();
            UI_Clear_Help();
            Help_Load_Items();
            present_mode = PRESENT_DISSOLVE;
        }
        for (i = 0; i < 6; i++) {
            if (held_item == -1 && -p_items_portrait_ctrls[i] == input) {
                Play_Click_Sound_Casting();
                UI_Clear_Help();
                Items_Hero_Portrait_Rect(i, &x1, &y1, &x2, &y2);
                j = wizards[current_player].hired_heroes[i].unit_index;
                Unit_Stats_Popup(j, x1, y1, x2, y2);
                if (held_item != -1)
                    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
                UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
                Items_Screen_Load();
                UI_Clear_Help();
                Help_Load_Items();
            }
        }
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 3; j++) {
                if (-p_items_slot_ctrls[i][j] == input && wizards[current_player].hired_heroes[i].item_index[j] > -1) {
                    UI_Clear_Help();
                    Play_Click_Sound_Casting();
                    ix = items_win_x + (i % 2) * 136 + j * 29 + 54;
                    iy = items_win_y + (i / 2) * 47 + 28;
                    if (held_item != -1)
                        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                    item_pic = Heap_Alloc_Front(p_scratch, 1);
                    Item_View_Popup(wizards[PLAYER_HUMAN].hired_heroes[i].item_index[j], ix + 8, iy + 8);
                    if (held_item != -1)
                        Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
                    UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
                    Items_Screen_Load();
                    UI_Clear_Help();
                    Help_Load_Items();
                    break;
                }
            }
        }
        for (i = 0; i < 4; i++) {
            if (-p_items_vault_ctrls[i] == input && wizards[current_player].vault_items[i] > -1) {
                Play_Click_Sound_Casting();
                UI_Clear_Help();
                if (held_item != -1)
                    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                item_pic = Heap_Alloc_Front(p_scratch, 1);
                Item_View_Popup(wizards[PLAYER_HUMAN].vault_items[i], items_win_x + i * 21 + 59, items_win_y + 179);
                if (held_item != -1)
                    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
                UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
                Items_Screen_Load();
                UI_Clear_Help();
                Help_Load_Items();
                break;
            }
        }
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 3; j++) {
                if (p_items_slot_ctrls[i][j] == input) {
                    Play_Click_Sound();
                    UI_Clear_Help();
                    Items_Slot_Click(i, j);
                    UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
                    UI_Clear_Help();
                    Help_Load_Items();
                    break;
                }
            }
        }
        for (i = 0; i < 4; i++) {
            if (p_items_vault_ctrls[i] == input) {
                Play_Click_Sound();
                UI_Clear_Help();
                Items_Slot_Click(ITEM_OWNER_VAULT, i);
                UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
                UI_Clear_Help();
                Help_Load_Items();
                break;
            }
        }
        if (input == items_anvil_ctrl && held_item > -1) {
            UI_Suspend_Redraw_Fn();
            Play_Click_Sound();
            UI_Clear_Help();
            Items_Break_Item();
            UI_Clear_Help();
            Help_Load_Items();
            UI_Set_Redraw_Fn(Items_Screen_Redraw, 1);
        }
        if (done == 0) {
            Items_Screen_Redraw();
            Screen_Present();
            if (held_item == -1)
                Wait_Ticks(1);
        }
    }
    UI_Clear_Controls();
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
    current_screen = SCREEN_ARMIES;
    present_mode = PRESENT_DISSOLVE;
}

/* () redraw callback of the Items screen */
void Items_Screen_Redraw(void)
{
    Items_Screen_Draw();
}

/* () the window: each hired hero's portrait, name and three slots (the item, or the empty-slot picture for the
   slot type and its help), "Same Location" / "Item Teleport" while an item is held, the vault, the treasury
   (wizard 0's gold and mana: BUG #464 for another current player; the small font from 20000), Alchemy, OK and the held
   item at the cursor */
void Items_Screen_Draw(void)
{
    int i;
    int k;
    int icon_x;
    int icon_y;
    int px;
    int py;
    int px2;
    int py2;
    struct HiredHero *hero;
    char colors[6];
    int type;                                   /* the hero's unit type, never read */

    Mouse_Set_Cursor_Regions(1, held_item > -1 ? &mouse_region_hidden : &mouse_region_arrow);
    Set_Draw_Page();
    Screen_Restore_A800();
    Draw_Pic_Frame(items_win_x, items_win_y, items_bg_pic);
    for (i = 0; i < 6; i++) {
        hero = &wizards[current_player].hired_heroes[i];
        if (hero->unit_index > -1) {
            type = p_units[hero->unit_index].type;
            Items_Hero_Portrait_Rect(i, &px, &py, &px2, &py2);
            Draw_Pic_Frame(px, py, items_portrait_frame_pic);
            Draw_Pic_Frame(px + 1, py + 1, items_hero_portraits[i]);
            colors[0] = 0xe1;
            colors[1] = 0xb3;
            Font_Select_Palette(1, colors);
            Font_Select_Shadow(1, 15, 0, 0);
            Set_Text_Shadow_Color(0);
            Print_Text(px + 37, py, hero->name);
            for (k = 0; k < 3; k++) {
                if (hero->item_index[k] > -1) {
                    Item_Build_Icon(hero->item_index[k], p_item_icon_pic);
                    Draw_Bitmap(px + k * 29 + 40, py + 15, p_item_icon_pic);
                } else {
                    Draw_Pic_Frame(px + k * 29 + 41, py + 17, item_empty_slot_pics[hero->slot_type[k] - 1]);
                    help_entries[i * 3 + k + 5].id = hero->slot_type[k] + HELP_ITEM_SLOT;   /* help slots 5 ..: 3 per hero */
                }
            }
            if (held_item_source > -1) {
                if (Items_Transfer_Is_Free(held_item_source, i) == 1) {
                    colors[0] = 0xe1;
                    colors[1] = 0x9d;
                    Font_Select_Palette(1, colors);
                    Font_Select_Shadow(1, 15, 0, 0);
                    Set_Text_Shadow_Color(0);
                    Print_Text(px + 37, py + 7, "Same Location");
                } else {
                    Font_Select_Shadow(1, 2, 0, 0);
                    Set_Font_Color(0xe1);
                    Print_Text(px + 37, py + 7, "Item Teleport");
                }
            }
        }
    }
    for (i = 0; i < 4; i++) {
        if (wizards[current_player].vault_items[i] > -1) {
            icon_x = items_win_x + i * 21 + 52;
            icon_y = items_win_y + 170;
            Item_Build_Icon(wizards[current_player].vault_items[i], p_item_icon_pic);
            Draw_Bitmap(icon_x, icon_y, p_item_icon_pic);
        }
    }
    if (wizards[current_player].gold > 19999 || wizards[current_player].mana_reserve > 19999)
        Font_Select_Shadow(0, 0, 0, 0);
    else
        Font_Select_Shadow(1, 0, 0, 0);
    Set_Text_Shadow_Color(0);
    Set_Font_Color(0xe1);
    Print_Number_Right(items_win_x + 166, items_win_y + 164, wizards[PLAYER_HUMAN].gold);
    Print_Number_Right(items_win_x + 200, items_win_y + 164, wizards[PLAYER_HUMAN].mana_reserve);
    Font_Select_Shadow(0, 0, 0, 0);
    Set_Text_Shadow_Color(0);
    Print_Text(items_win_x + 169, items_win_y + 164, "GP");
    Print_Text(items_win_x + 203, items_win_y + 164, "MP");
    Set_Pic_Frame(items_alchemy_pic, 0);
    Draw_Pic_Frame(items_win_x + 219, items_win_y + 155, items_alchemy_pic);
    Set_Pic_Frame(items_ok_pic, 0);
    Draw_Pic_Frame(items_win_x + 219, items_win_y + 175, items_ok_pic);
    if (held_item > -1) {
        icon_x = Mouse_X() - 5;
        icon_y = Mouse_Y() - 5;
        Items_Draw_Held_Item(icon_x, icon_y, held_item);
    }
    Item_Outline_Cycle_Tick();
}

/* () the screen's near buffers (text, control ids), the icon buffer, ARMYLIST 5-8 and each hired hero's PORTRAIT */
void Items_Screen_Load(void)
{
    int i;
    int unit;

    p_text_buf = Near_Alloc_Reset(100);
    p_text_buf2 = Near_Alloc(100);
    p_items_vault_ctrls = (int *)Near_Alloc(8);
    for (i = 0; i < 6; i++)
        p_items_slot_ctrls[i] = (int *)Near_Alloc(6);
    p_items_portrait_ctrls = (int *)Near_Alloc(12);
    p_item_icon_pic = Heap_Alloc_Front(p_scratch, 30);
    items_bg_pic = LBX_Reload_Next("ARMYLIST", 5, p_scratch);
    items_portrait_frame_pic = LBX_Reload_Next("ARMYLIST", 6, p_scratch);
    items_alchemy_pic = LBX_Reload_Next("ARMYLIST", 7, p_scratch);
    items_ok_pic = LBX_Reload_Next("ARMYLIST", 8, p_scratch);
    for (i = 0; i < 6; i++) {
        unit = wizards[current_player].hired_heroes[i].unit_index;
        if (unit > -1 && p_units[unit].owner != PLAYER_NONE)
            items_hero_portraits[i] = LBX_Reload_Next("PORTRAIT", unit_types[p_units[unit].type].hero_portrait, p_scratch);
    }
}

/* (source, dest: a hero slot or ITEM_OWNER_VAULT) -> 1 when moving the held item costs nothing: into the vault; from the vault (or a new item)
   to a hero at the fortress; from an award place to a hero standing there; between heroes on the same tile */
int Items_Transfer_Is_Free(int source, int dest)
{
    int result;

    if (dest == ITEM_OWNER_VAULT)
        return 1;
    if (source == ITEM_OWNER_VAULT) {
        if (p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].x == p_fortresses[PLAYER_HUMAN].x
            && p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].y == p_fortresses[PLAYER_HUMAN].y
            && p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].plane == p_fortresses[PLAYER_HUMAN].plane)
            return 1;
        else
            return 0;
    }
    if (source == ITEM_OWNER_AWARD) {
        if (p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].x == award_loc_x
            && p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].y == award_loc_y
            && p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].plane == award_loc_plane)
            return 1;
        else
            return 0;
    }
    if (p_units[wizards[PLAYER_HUMAN].hired_heroes[source].unit_index].x == p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].x
        && p_units[wizards[PLAYER_HUMAN].hired_heroes[source].unit_index].y == p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].y
        && p_units[wizards[PLAYER_HUMAN].hired_heroes[source].unit_index].plane
           == p_units[wizards[PLAYER_HUMAN].hired_heroes[dest].unit_index].plane)
        result = 1;
    else
        result = 0;
    return result;
}

/* (x, y, item) the item's icon and its name centred below it */
void Items_Draw_Held_Item(int x, int y, int item)
{
    Item_Build_Icon(item, p_item_icon_pic);
    Draw_Bitmap_Clipped(x, y, p_item_icon_pic);
    far_strcpy(p_text_buf, 0, p_items[item].name);
    Print_Text_Clipped_Centered(x + 12, y + 19, p_text_buf);
}

/* () the anvil: "Do you want to destroy your <item> and gain <cost / 2> mana crystals?" - yes frees the item
   record (cost -1) and gives the mana
   Numbered: BUG #93 (BUGS_AND_QUIRKS.txt) */
void Items_Break_Item(void)
{
    int mana;
    int item;
    int x;
    int y;

    UI_Clear_Controls();
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    item = held_item;
    held_item = -1;
    Items_Screen_Redraw();
    held_item = item;
    x = items_win_x + 18;
    y = items_win_y + 160;
    Items_Draw_Held_Item(x, y, held_item);
    Screen_Present();
    Screen_Copy_Front();
    mana = p_items[held_item].cost / 2;
    strcpy(p_text_buf, "Do you want to destroy your \x02");
    far_strcpy(p_text_buf2, 0, p_items[held_item].name);
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, "\x01 and gain ");
    itoa(mana, p_text_buf2, 10);
    strcat(p_text_buf, p_text_buf2);
    strcat(p_text_buf, " mana crystals?");
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    if (Confirm_Box(p_text_buf) == 1) {
        Wizard_Add_Mana(current_player, mana);
        p_items[held_item].cost = -1;
        held_item_source = -1;
        held_item = -1;
        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    } else
        Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
}

/* (owner: hero 0-5 / 6 vault, slot) with nothing held, pick up the slot's item (a hero's only when
   Items_Removal_Allowed); else drop the held item into an empty slot that fits (the slot type, or class 1 / 4 in
   type-3 and class 1 / 2 in type-2 slots; anything in the vault) or swap it with the slot's item (same class, or
   the same slot rules); a transfer that is not free costs 20 mana after a question. BUG #465: the slot type is read
   once from wizards[current_player], elsewhere from wizards[PLAYER_HUMAN]; for the vault (owner 6) the fit tests
   read hired_heroes[6].slot_type, one record past the table (a read, no effect: the vault takes anything);
   clarified using ReMoM repo */
void Items_Slot_Click(int owner, int slot)
{
    int fits;
    int *slots;
    int allowed;

    if (owner == ITEM_OWNER_VAULT)
        slots = wizards[current_player].vault_items;
    else
        slots = wizards[current_player].hired_heroes[owner].item_index;
    if (held_item == -1) {
        if (slots[slot] > -1) {
            allowed = 1;
            if (owner != ITEM_OWNER_VAULT && Items_Removal_Allowed(owner, slots[slot]) == 0)
                allowed = 0;
            if (allowed == 1) {
                held_item_source = owner;
                held_item = slots[slot];
                slots[slot] = -1;
                Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
            }
        }
    } else {
        fits = 0;
        if (slots[slot] < 0) {
            if (wizards[current_player].hired_heroes[owner].slot_type[slot] == ITEM_SLOT_MELEE_OR_MAGIC
                && (p_items[held_item].slot_type == ITEM_SLOT_MELEE || p_items[held_item].slot_type == ITEM_SLOT_MAGIC))
                fits = 1;
            if (wizards[PLAYER_HUMAN].hired_heroes[owner].slot_type[slot] == ITEM_SLOT_BOW
                && (p_items[held_item].slot_type == ITEM_SLOT_MELEE || p_items[held_item].slot_type == ITEM_SLOT_BOW))
                fits = 1;
            if (p_items[held_item].slot_type == wizards[PLAYER_HUMAN].hired_heroes[owner].slot_type[slot])
                fits = 1;
            if (owner == ITEM_OWNER_VAULT)
                fits = 1;
        } else {
            if (owner == ITEM_OWNER_VAULT)
                fits = 1;
            if (p_items[held_item].slot_type == p_items[slots[slot]].slot_type)
                fits = 1;
            if (wizards[PLAYER_HUMAN].hired_heroes[owner].slot_type[slot] == ITEM_SLOT_MELEE_OR_MAGIC
                && (p_items[held_item].slot_type == ITEM_SLOT_MELEE || p_items[held_item].slot_type == ITEM_SLOT_MAGIC))
                fits = 1;
            if (wizards[PLAYER_HUMAN].hired_heroes[owner].slot_type[slot] == ITEM_SLOT_BOW
                && (p_items[held_item].slot_type == ITEM_SLOT_MELEE || p_items[held_item].slot_type == ITEM_SLOT_BOW))
                fits = 1;
        }
        if (fits == 1) {
            allowed = 1;
            if (owner != ITEM_OWNER_VAULT && slots[slot] > -1 && Items_Removal_Allowed(owner, slots[slot]) == 0) {
                allowed = 0;
                fits = 0;
            }
            if (allowed == 1) {
                if (Items_Transfer_Is_Free(held_item_source, owner) == 1)
                    fits = 1;
                else if (wizards[current_player].mana_reserve >= ITEM_TRANSFER_MANA) {
                    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                    Screen_Present();
                    strcpy(p_text_buf, "Do you wish to make this transfer at a cost of \x02");
                    itoa(ITEM_TRANSFER_MANA, p_text_buf2, 10);
                    strcat(p_text_buf, p_text_buf2);
                    strcat(p_text_buf, " mana crystals\x01?");
                    fits = Confirm_Box(p_text_buf);
                    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
                    if (fits == 1)
                        wizards[current_player].mana_reserve = wizards[current_player].mana_reserve - ITEM_TRANSFER_MANA;
                } else {
                    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                    Screen_Copy_Front();
                    Screen_Present();
                    Warning_Box("You do not have enough mana crystals to transfer this item.");
                    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
                    fits = 0;
                }
            }
            if (slots[slot] < 0) {
                if (fits == 1) {
                    held_item_source = -1;
                    slots[slot] = held_item;
                    held_item = -1;
                    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
                }
            } else if (fits == 1) {
                Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
                Swap_Words(&slots[slot], &held_item);
                held_item_source = owner;
            }
        }
    }
}

/* () the controls: hero portraits and slots, the vault, Alchemy ("A") and OK ("O") only with nothing held, the
   anvil, the window and the whole screen */
void Items_Screen_Add_Controls(void)
{
    int i;
    int k;
    int x;
    int y;
    int px;
    int py;
    int x2;
    int y2;

    for (i = 0; i < 6; i++) {
        p_items_slot_ctrls[i][0] = UI_NO_CONTROL;
        p_items_slot_ctrls[i][1] = UI_NO_CONTROL;
        p_items_slot_ctrls[i][2] = UI_NO_CONTROL;
        p_items_portrait_ctrls[i] = UI_NO_CONTROL;
        if (wizards[PLAYER_HUMAN].hired_heroes[i].unit_index > -1) {
            Items_Hero_Portrait_Rect(i, &px, &py, &x2, &y2);
            p_items_portrait_ctrls[i] = UI_Add_Region(px, py, x2, y2, "", -1);
            for (k = 0; k < 3; k++)
                p_items_slot_ctrls[i][k] = UI_Add_Region(px + k * 29 + 40, py + 16, px + k * 29 + 61, py + 33, "", -1);
        }
    }
    for (i = 0; i < 4; i++) {
        x = items_win_x + i * 21 + 52;
        y = items_win_y + 172;
        p_items_vault_ctrls[i] = UI_Add_Region(x, y, x + 19, y + 15, "", -1);
    }
    items_alchemy_btn = UI_NO_CONTROL;
    items_ok_btn = UI_NO_CONTROL;
    if (held_item == -1) {
        items_alchemy_btn = UI_Add_Button(items_win_x + 219, items_win_y + 155, "", items_alchemy_pic, "A", -1);
        items_ok_btn = UI_Add_Button(items_win_x + 219, items_win_y + 175, "", items_ok_pic, "O", -1);
    }
    items_anvil_ctrl = UI_Add_Region(items_win_x + 9, items_win_y + 155, items_win_x + 48, items_win_y + 189, "", -1);
    items_window_ctrl = UI_Add_Region(items_win_x, items_win_y, items_win_x + 283, items_win_y + 195, "", -1);
    items_outside_ctrl = UI_Add_Region(0, 0, 319, 199, "", -1);
}

/* (count, items) queues up to 6 items (any more are dropped, their records not freed) and runs the Items screen
   until each is placed or destroyed
   Numbered: BUG #77 (BUGS_AND_QUIRKS.txt) */
void Items_Screen_Award(int count, int *items)
{
    int i;

    if (count > 6)
        count = 6;
    award_pending = 1;
    award_count = count;
    award_next = 0;
    for (i = 0; i < count; i++)
        award_items[i] = items[i];
    Screen_Items();
    present_mode = PRESENT_NORMAL;
    award_item = -1;
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    award_pending = 0;
}

/* (hero, &x1, &y1, &x2, &y2) the hero's 32 x 32 portrait: 2 columns 135 apart, 3 rows 46 apart */
void Items_Hero_Portrait_Rect(int hero, int *x1, int *y1, int *x2, int *y2)
{
    *x1 = items_win_x + (hero % 2) * 135 + 14;
    *y1 = items_win_y + (hero / 2) * 46 + 12;
    *x2 = *x1 + 32;
    *y2 = *y1 + 32;
}

/* (hero, item) -> 0 with "This unit will drown if the item is removed." when the hero stands on water, no other
   unit there carries others, no other equipped (or held) item gives Water Walking or Flight, nobody there walks
   on wind or water, and the hero cannot fly or cross water without the item; else 1. BUG #80: the last tests
   (Hero_Item_Water_Walking / Hero_Item_Flight) still count the item being removed, so an item that gives both passes
   (ReMoM marks the same) */
int Items_Removal_Allowed(int hero, int item)
{
    int unit;
    int k;
    int drown;
    int tile_count;
    int water;
    int others_carry;
    int self_carries;
    int unused;                                 /* set to 0, never read */
    long powers;
    int other_item;
    int tile_units[MAX_STACK];

    unit = wizards[current_player].hired_heroes[hero].unit_index;
    if (unit == -1)
        return 1;
    drown = 0;
    unused = 0;
    water = Tile_Is_Water(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
    if (water == 1) {
        Tile_Units_Of_Owner(p_units[unit].x, p_units[unit].y, p_units[unit].plane, p_units[unit].owner, &tile_count, tile_units);
        for (k = 0; k < tile_count; k++)
            if (tile_units[k] != unit && unit_types[p_units[tile_units[k]].type].transport_capacity > 0)
                water = 0;
    }
    if (water == 1) {
        for (k = 0; k < 3; k++) {
            other_item = wizards[current_player].hired_heroes[hero].item_index[k];
            if (other_item > -1 && other_item != item) {
                powers = p_items[other_item].powers;
                if ((powers & ITEM_POWER_WATER_WALKING) != 0 || (powers & ITEM_POWER_FLIGHT) != 0)
                    water = 0;
            }
        }
    }
    if (water == 1 && held_item > -1) {
        powers = p_items[held_item].powers;
        if ((powers & ITEM_POWER_WATER_WALKING) != 0 || (powers & ITEM_POWER_FLIGHT) != 0)
            water = 0;
    }
    if (water == 1) {
        Tile_Units_Of_Owner(p_units[unit].x, p_units[unit].y, p_units[unit].plane, p_units[unit].owner, &tile_count, tile_units);
        self_carries = 0;
        if (Unit_Has_Wind_Walking(unit) == 1 || Unit_Water_Move_Always_0(unit) == 1)
            self_carries = 1;
        others_carry = 0;
        for (k = 0; k < tile_count; k++)
            if (tile_units[k] != unit
                && (Unit_Has_Wind_Walking(tile_units[k]) == 1 || Unit_Water_Move_Always_0(tile_units[k]) == 1))
                others_carry = 1;
        if (others_carry == 0 && self_carries == 0) {
            powers = p_items[item].powers;
            drown = 1;
            if ((powers & ITEM_POWER_FLIGHT) != 0
                && (Unit_Can_Cross_Water(unit) == 1 || Hero_Item_Water_Walking(unit) == 1 || self_carries == 1))
                drown = 0;
            if ((powers & ITEM_POWER_WATER_WALKING) != 0 && (Unit_Can_Fly(unit) == 1 || Hero_Item_Flight(unit) == 1 || self_carries == 1))
                drown = 0;
            if (Unit_Can_Fly(unit) == 1 || Unit_Can_Cross_Water(unit) == 1)
                drown = 0;
        }
    }
    if (drown == 1) {
        Warning_Box("This unit will drown if the item is removed.");
        return 0;
    }
    return 1;
}
