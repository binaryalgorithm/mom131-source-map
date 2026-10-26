/* ITEMFORG.C - WIZARDS.EXE overlay code segment 94df (stub 254c): the item forge of Enchant Item / Create Artifact -
   the power list by item type, the type / icon / power selection screen with its auto name and cost, the Spell
   Charges sub-screen, writing the design into an item record - and the random / prefab item creation (ITEMDATA).
   The original module has no <string.h> and calls several functions without prototypes (see the declarations).
   This module's data (DS:5bcc..5d1a): its initialised arrays and literals. BSS DS:c8b6..c8f9: the forge state.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 94df src/wizards/ITEMFORG.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void ItemForge_Load(void);
void ItemForge_Redraw(void);
char *Item_Type_Name(int item);
char *ItemForge_Auto_Name(int item);
int ItemForge_Calc_Cost(int item);
void ItemForge_Build_Charge_Spell_List(void);
void ItemForge_Add_Controls(void);
int Screen_Item_Forge(int player, int artifact);
void ItemForge_Build_Item(int item);
int ItemForge_Build_Power_List(int player, int max_cost);
void Item_Charge_Spell_Redraw(void);
unsigned char Screen_Item_Charge_Spell(void);
int Item_Create_From_ITEMDATA(int record);
int Item_Alloc_Copy(int src_item);
void Item_Create_Random(int prefab_kind, int max_cost);
int Item_Create_Prefab(int prefab_kind, int *spellbooks, int max_cost);

#include "GAME.H"

int forge_type_mask;                            /* DS:c8f8  1 << item type */
int forge_cost;                                 /* DS:c8f6 */
int forge_column_rows[3];                       /* DS:c8f0 */
int forge_powers[4];                            /* DS:c8e8 */
int forge_power_count;                          /* DS:c8e6 */
unsigned forge_ok_btn_pic;                      /* DS:c8e4 */
int forge_icon;                                 /* DS:c8e2 */
int forge_has_charges;                          /* DS:c8e0 */
int forge_spell_list_sel;                       /* DS:c8de */
unsigned forge_icon_pic;                        /* DS:c8dc */
                                                /* also: the merchant's card buffer and the hire screen's item
                                                   picture (HIRE.C) */
int *p_forge_type_btns;                         /* DS:c8da */
int forge_ok_btn;                               /* DS:c8d8 */
int forge_icon_next_btn;                        /* DS:c8d6 */
int *p_forge_power_ctrls;                       /* DS:c8d4 */
int forge_name_field;                           /* DS:c8d2 */
int forge_icon_prev_btn;                        /* DS:c8d0 */
char *p_forge_name_colors;                      /* DS:c8ce */
int forge_spell_list_top;                       /* DS:c8cc */
struct ItemPower *p_item_powers;                /* DS:c8ca */
int *p_forge_power_slots;                       /* DS:c8c8  ITEMPOW records by row, -1 = a gap */
int *p_forge_power_selected;                    /* DS:c8c6 */
unsigned *p_forge_type_btn_pics;                /* DS:c8c4 */
unsigned *p_forge_type_btn_sel_pics;            /* DS:c8c2 */
unsigned char *p_forge_spell_list;              /* DS:c8c0 */
unsigned forge_charges_box_pic;                 /* DS:c8be */
unsigned forge_charge_btn_pics[4];              /* DS:c8b6  SPELLSCR 38..41: x1..x4 */

char forge_text_colors[4] = { 0x11, 9, 8, 8 };  /* DS:5bcc */

extern unsigned char far *p_items_in_game;      /* DS:913a */
extern unsigned spellbook_corner_right_pic;     /* DS:9468 */
extern unsigned spellbook_corner_left_pic;      /* DS:946a */
extern unsigned spellbook_small_pic;            /* DS:946e */
extern int present_mode;                        /* DS:9c8e */
extern int list_count;                          /* DS:c926 */
extern unsigned forge_icon_next_pic;            /* DS:ca40 */
extern unsigned forge_icon_prev_pic;            /* DS:ca42 */

char *strcpy();                                 /* no prototype here: Screen_Item_Forge calls it with 3 arguments */
char *strcat();                                 /* no prototype here (no <string.h> in this module) */
int strlen();                                   /* no prototype here: its result is compared signed */
char *itoa();                                   /* no prototype here (no <stdlib.h> in this module) */
void far_strcpy();                              /* no prototype here: ItemForge_Build_Item passes (far dst, near src, 0) */
int Mem_Copy_Far();                               /* no prototype here: far pointers */
int Mem_Clear_Far();                            /* no prototype here: called with a far pointer and a count */
/* Print_Far_Text is called undeclared (implicit int): ItemForge_Redraw uses its result, the x after the text */
void Font_Select_Outline();                     /* no prototype here: called with 2 arguments (TEXT.C takes 4) */
void Font_Select_Shadow();                      /* no prototype here: called with 2 and 4 arguments (TEXT.C takes 4) */
int UI_Add_Text_Input();                        /* no prototype here: passes the color table pointer where UI.C takes an int */
int abs(int x);
int Array_Delete_Near(int index, char *array, int record_size, int count);
void Item_Outline_Cycle_Tick(void);
int Confirm_Box(char *text);
void Draw_Bitmap(int x, int y, unsigned pic);
void Font_Select(int font, int c1, int c2, int c3);
void Font_Select_Shadow_UL(int font, int c1, int c2, int c3);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Item_Build_Icon(int item, unsigned pic);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
char *Near_Alloc(unsigned bytes);
char *Near_Alloc_Reset(unsigned bytes);
void Pic_Apply_Palette(unsigned pic);
void Pic_Reset_Frame(unsigned pic);
int Print_Number(int x, int y, int value);
int Print_Text_Centered(int x, int y, char *text);
int Print_Text_Right(int x, int y, char *text);
void Remap_Build_Shadow(void);
void Set_Draw_Page_Front(void);
void Set_Font_Color(int color);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void UI_Draw_Controls(void);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);

/* () loads the forge: the near buffers, ITEMPOW, the background on both pages, the buttons; type Sword */
void ItemForge_Load(void)
{
    int i;
    unsigned pic;

    p_item_powers = (struct ItemPower *)Near_Alloc_Reset(0x7da);
    p_forge_power_slots = (int *)Near_Alloc(0x6c);
    p_forge_power_selected = (int *)Near_Alloc(0x6c);
    p_forge_type_btn_pics = (unsigned *)Near_Alloc(0x14);
    p_forge_type_btns = (int *)Near_Alloc(0x14);
    p_forge_name_colors = Near_Alloc(0x10);
    p_forge_power_ctrls = (int *)Near_Alloc(0x6c);
    p_forge_type_btn_sel_pics = (unsigned *)Near_Alloc(0x14);
    p_forge_spell_list = (unsigned char *)Near_Alloc(0x78);
    LBX_Load_Records_Near("ITEMPOW", 0, p_item_powers, 0, 67, 30);
    pic = LBX_Reload("SPELLSCR", 13, p_scratch);
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, pic);
    Screen_Save_A800();
    Set_Draw_Page_Front();
    forge_ok_btn_pic = LBX_Reload("SPELLSCR", 24, p_scratch);
    for (i = 0; i < 16; i++)
        p_forge_name_colors[i] = i % 4 + 177;
    for (i = 0; i < 10; i++)
        p_forge_type_btn_pics[i] = LBX_Reload_Next("SPELLSCR", i + 14, p_scratch);
    for (i = 0; i < 10; i++)
        p_forge_type_btn_sel_pics[i] = LBX_Reload_Next("SPELLSCR", i + 25, p_scratch);
    forge_icon_prev_pic = LBX_Reload_Next("SPELLSCR", 35, p_scratch);
    forge_icon_next_pic = LBX_Reload_Next("SPELLSCR", 36, p_scratch);
    forge_charges_box_pic = LBX_Reload_Next("SPELLSCR", 37, p_scratch);
    for (i = 0; i < 4; i++)
        forge_charge_btn_pics[i] = LBX_Reload_Next("SPELLSCR", i + 38, p_scratch);
    forge_icon_pic = Heap_Alloc_Back(p_scratch, 30);
    forge_has_charges = 0;
    forge_type_mask = 1;
}

/* redraw callback of the forge: 3 columns of power rows (stat powers as " +N" right-aligned and the name; the
   selected ones highlighted; Spell Charges as "<spell> x<charges>"), the cost after Enchant Item's reduction, the
   item's icon */
void ItemForge_Redraw(void)
{
    int column;
    int i;                                      /* also the running x after Print_Far_Text */
    int y;
    int x;
    char colors_a[4] = { 0, 0xb1, 0xb2, 0xb3 };
    char colors_b[4] = { 0, 0x1f, 0x1d, 0x1c };
    int pct;
    int cost;
    int row;
    int base;

    Screen_Restore_A800();
    for (column = 0; column < 3; column++) {
        base = 0;
        for (i = 0; i < column; i++)
            base += forge_column_rows[i];
        if (p_forge_power_slots[base] == -1 && column != 2)
            y = 36;
        else
            y = 41;
        x = column * 85 + 10;
        if (column == 2)
            x += 17;
        for (row = 0; row < forge_column_rows[column]; row++) {
            if (p_forge_power_slots[row + base] == -1) {
                y += 3;
            } else {
                if (p_forge_power_selected[row + base] == 1) {
                    Set_Text_Shadow_Color(18);
                    Font_Select_Palette(3, colors_a);
                    Font_Select_Shadow_UL(3, 15, 0, 0);
                    if (p_forge_power_slots[row + base] == ITEMPOW_SPELL_CHARGES) {
                        i = Print_Far_Text(x, y, p_spell_data[(unsigned char)p_items[ITEM_REC_FORGE].spell].name);
                        i = Print_Text(i, y, " x");
                        Print_Number(i, y, p_items[ITEM_REC_FORGE].charges);
                    } else if (column != 2) {   /* ITEMPOW names read "+N  Text": the number right-aligned,
                                                   the text from +4 */
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = 0;
                        Print_Text_Right(x + 10, y, p_item_powers[p_forge_power_slots[row + base]].name);
                        Print_Text(x + 14, y, p_item_powers[p_forge_power_slots[row + base]].name + 4);
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = ' ';
                    } else {
                        Print_Text(x, y, p_item_powers[p_forge_power_slots[row + base]].name);
                    }
                    Set_Text_Shadow_Color(16);
                    Font_Select_Palette(3, colors_a);
                    Font_Select_Shadow(3, 15, 0, 0);
                    if (p_forge_power_slots[row + base] == ITEMPOW_SPELL_CHARGES) {
                        i = Print_Far_Text(x, y, p_spell_data[(unsigned char)p_items[ITEM_REC_FORGE].spell].name);
                        i = Print_Text(i, y, " x");
                        Print_Number(i, y, p_items[ITEM_REC_FORGE].charges);
                    } else if (column != 2) {   /* ITEMPOW names read "+N  Text": the number right-aligned,
                                                   the text from +4 */
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = 0;
                        Print_Text_Right(x + 10, y, p_item_powers[p_forge_power_slots[row + base]].name);
                        Print_Text(x + 14, y, p_item_powers[p_forge_power_slots[row + base]].name + 4);
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = ' ';
                    } else {
                        Print_Text(x, y, p_item_powers[p_forge_power_slots[row + base]].name);
                    }
                } else {
                    Set_Text_Shadow_Color(18);
                    Font_Select_Palette(3, colors_b);
                    Font_Select_Shadow_UL(3, 15, 0, 0);
                    if (column != 2) {
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = 0;
                        Print_Text_Right(x + 10, y, p_item_powers[p_forge_power_slots[row + base]].name);
                        Print_Text(x + 14, y, p_item_powers[p_forge_power_slots[row + base]].name + 4);
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = ' ';
                    } else {
                        Print_Text(x, y, p_item_powers[p_forge_power_slots[row + base]].name);
                    }
                    Set_Text_Shadow_Color(16);
                    Font_Select_Palette(3, colors_b);
                    Font_Select_Shadow(3, 15, 0, 0);
                    if (column != 2) {
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = 0;
                        Print_Text_Right(x + 10, y, p_item_powers[p_forge_power_slots[row + base]].name);
                        Print_Text(x + 14, y, p_item_powers[p_forge_power_slots[row + base]].name + 4);
                        p_item_powers[p_forge_power_slots[row + base]].name[3] = ' ';
                    } else {
                        Print_Text(x, y, p_item_powers[p_forge_power_slots[row + base]].name);
                    }
                }
                y += 11;
            }
        }
    }
    Font_Select_Palette(3, forge_text_colors);
    Font_Select_Shadow(3, 15, 4, 4);
    Set_Text_Shadow_Color(17);
    x = Print_Text(198, 185, "Cost: ");
    pct = Spell_Cost_Reduction_Pct(0, SPELL_ENCHANT_ITEM);   /* always the human's (original) */
    cost = forge_cost;
    cost = cost - (long)cost * pct / 100;
    x = Print_Number(x - 1, 185, cost);
    Item_Build_Icon(ITEM_REC_FORGE, forge_icon_pic);
    Draw_Bitmap(5, 4, forge_icon_pic);
    Item_Outline_Cycle_Tick();
}

/* (item) -> the item type's word (Misc items by icon: Amulet, Ring, Cloak, Gauntlet, Helm, Orb) */
char *Item_Type_Name(int item)
{
    char *names[16];
    int icon;
    int k;

    names[0] = "Amulet";
    names[1] = "Ring";
    names[2] = "Cloak";
    names[3] = "Gauntlet";
    names[4] = "Helm";
    names[5] = "Orb";
    names[6] = "Sword";
    names[7] = "Mace";
    names[8] = "Axe";
    names[9] = "Bow";
    names[10] = "Staff";
    names[11] = "Wand";
    names[12] = "Misc";
    names[13] = "Shield";
    names[14] = "Chain Mail";
    names[15] = "Plate Mail";
    icon = p_items[item].icon;
    if (p_items[item].item_type == ITEM_TYPE_MISC) {
        if (icon <= 77)                         /* ITEMS.LBX icons by look: up to 77 amulets, 83 rings, 89 cloaks,
                                                   93 gauntlets, 100 helms, then orbs */
            k = 0;
        else if (icon <= 83)
            k = 1;
        else if (icon <= 89)
            k = 2;
        else if (icon <= 93)
            k = 3;
        else if (icon <= 100)
            k = 4;
        else
            k = 5;
        return names[k];
    } else {
        return names[(char)p_items[item].item_type + 6];
    }
}

/* (item) -> the automatic name of the design: "+N <type>" for an attack (weapons) or defense (armour) power, else
   the type; then " of <best power>" (the spell and its charges, the special power, or the stat's name); a "+N" name
   over 26 characters drops its first 3 (the "+N ") inside the 30-byte buffer only, so a name longer than 29 (which
   also runs past name[30]) ends cut to 27 characters (BUG #71).
   BUG #463: returns its own stack buffer
   Numbered: BUG #71 (BUGS_AND_QUIRKS.txt) */
char *ItemForge_Auto_Name(int item)
{
    int n;
    int best;
    int powers[4];
    int done;
    char num_text[10];
    char name[30];
    char *stat_names[7];
    char spell_buf[20];
    int i;

    stat_names[0] = "Strength";
    stat_names[1] = "Accuracy";
    stat_names[2] = "Defense";
    stat_names[3] = "Wizardry";
    stat_names[4] = "Power";
    stat_names[5] = "Speed";
    stat_names[6] = "Protection";
    i = forge_type_mask;
    n = 0;
    while (i > 1) {
        n++;
        i = i >> 1;
    }
    for (i = 0; i < 4; i++)
        powers[i] = forge_powers[i];
    done = 0;
    for (i = 0; i < 4; i++) {
        if (powers[i] > -1 && powers[i] < ITEMPOW_DEFENSE_FIRST && n < ITEM_TYPE_MISC) {
            strcpy(name, "+");
            itoa(powers[i] + 1, num_text, 10);
            strcat(name, num_text);
            strcat(name, " ");
            strcat(name, Item_Type_Name(item));
            powers[i] = -2;
            done = 1;
            break;
        }
    }
    for (i = 0; i < 4; i++) {
        if (powers[i] > ITEMPOW_DEFENSE_FIRST - 1 && powers[i] < ITEMPOW_TO_HIT_FIRST && n > ITEM_TYPE_MISC) {
            strcpy(name, "+");
            itoa(powers[i] - (ITEMPOW_DEFENSE_FIRST - 1), num_text, 10);
            strcat(name, num_text);
            strcat(name, " ");
            strcat(name, Item_Type_Name(item));
            powers[i] = -2;
            done = 1;
            break;
        }
    }
    if (done != 1) {
        if (n == ITEM_TYPE_MISC) {
        } else {
        }
        strcpy(name, Item_Type_Name(item));
    }
    best = -1;
    for (i = 0; i < 4; i++) {
        if (powers[i] > best)
            best = powers[i];
    }
    if (best > -1) {
        strcat(name, " of ");
        if (best >= ITEMPOW_SPECIAL_FIRST) {
            if (best == ITEMPOW_SPELL_CHARGES) {
                far_strcpy(spell_buf, 0, p_spell_data[(unsigned char)p_items[item].spell].name);
                strcat(name, spell_buf);
                if (strlen(name) < 26) {
                    strcat(name, " x");
                    itoa(p_items[item].charges, num_text, 10);
                    strcat(name, num_text);
                }
            } else {
                strcat(name, p_item_powers[best].name);
            }
        } else {
            strcat(name, stat_names[p_item_powers[best].group]);
        }
    }
    if (done == 1 && strlen(name) > 26) {
        for (i = 0; i < 3; i++)
            Array_Delete_Near(0, name, 1, 30);
    }
    return name;
}

/* (item) -> the design's cost: the type's base cost plus the powers (stat powers double on Misc items; Spell
   Charges = the spell's cost x charges x 20) */
int ItemForge_Calc_Cost(int item)
{
    int mask;
    int n;
    int base_costs[10] = { 100, 100, 100, 100, 300, 200, 50, 100, 100, 300 };
    int cost;
    int i;

    mask = forge_type_mask;
    n = 0;
    while (mask > 1) {
        n++;
        mask = mask >> 1;
    }
    cost = base_costs[n];
    for (i = 0; i < 4; i++) {
        if (forge_powers[i] > -1) {
            if (forge_powers[i] == ITEMPOW_SPELL_CHARGES)
                cost += p_spell_data[(unsigned char)p_items[item].spell].base_cost * p_items[item].charges * 20;
            else if (n == ITEM_TYPE_MISC && forge_powers[i] < ITEMPOW_SPECIAL_FIRST)
                cost += p_item_powers[forge_powers[i]].cost * 2;
            else
                cost += p_item_powers[forge_powers[i]].cost;
        }
    }
    return cost;
}

/* () the Spell Charges list: every spell the human knows that can be embedded */
void ItemForge_Build_Charge_Spell_List(void)
{
    int i;
    int j;
    int unused;
    unsigned char spell;

    list_count = 0;
    for (i = 0; i < 92; i++)                    /* 92 of its 0x78 bytes cleared; at most 95 spells qualify (SPELLDAT
                                                   1.31: all but the overland-only and never-cast ones) */
        p_forge_spell_list[i] = 0;
    for (i = 0; i < 6; i++) {
        for (j = 0; j < SPELLS_PER_REALM; j++) {
            unused = 0;
            spell = i * SPELLS_PER_REALM + j + 1;
            if (wizards[PLAYER_HUMAN].spell_library[i][j] == SPELLSTATE_KNOWN && p_spell_data[spell].castable != CASTABLE_OVERLAND_ONLY &&
                p_spell_data[spell].castable != CASTABLE_NEVER) {
                p_forge_spell_list[list_count] = spell;
                list_count++;
            }
        }
    }
}

/* () the forge's controls: the name field, the 10 type buttons, OK, the icon arrows, one control per power row */
void ItemForge_Add_Controls(void)
{
    int column;
    int y;
    int i;
    int k;
    int base;
    int x;

    UI_Clear_Controls();
    Font_Select_Palette(1, forge_text_colors);
    Font_Select(1, 15, 4, 4);
    Set_Font_Color(17);
    forge_name_field = UI_Add_Text_Input(30, 13, 118, message_text, 26, 0, 0, 0, p_forge_name_colors, "", -1);
    for (i = 0; i < 10; i++) {
        if ((1 << i) == forge_type_mask)
            p_forge_type_btns[i] = UI_Add_Button(i % 5 * 33 + 156, i / 5 * 15 + 3, "", p_forge_type_btn_sel_pics[i], "", -1);
        else
            p_forge_type_btns[i] = UI_Add_Button(i % 5 * 33 + 156, i / 5 * 15 + 3, "", p_forge_type_btn_pics[i], "", -1);
    }
    forge_ok_btn = UI_Add_Button(281, 180, "", forge_ok_btn_pic, "\x1b", -1);
    forge_icon_prev_btn = UI_Add_Button(4, 24, "", forge_icon_prev_pic, "", -1);
    forge_icon_next_btn = UI_Add_Button(17, 24, "", forge_icon_next_pic, "", -1);
    for (column = 0; column < 3; column++) {
        base = 0;
        for (k = 0; k < column; k++)
            base += forge_column_rows[k];
        if (p_forge_power_slots[base] == -1 && column != 2)
            y = 36;
        else
            y = 41;
        x = column * 85 + 10;
        if (column == 2)
            x += 17;
        for (i = 0; i < forge_column_rows[column]; i++) {
            if (p_forge_power_slots[i + base] == -1) {
                p_forge_power_ctrls[i + base] = UI_NO_CONTROL;
                y += 3;
            } else {
                if (column != 2)
                    p_forge_power_ctrls[i + base] = UI_Add_Region(x, y - 2, x + 78, y + 7, "", -1);
                else
                    p_forge_power_ctrls[i + base] = UI_Add_Region(x, y - 2, x + 98, y + 7, "", -1);
                y += 11;
            }
        }
    }
}

/* (player, artifact) -> the cost of the designed item (0: cancelled): the forge screen - type buttons, icon arrows,
   name field, power rows (one per group; Spell Charges opens its sub-screen), at most 4 powers; the cost limit per
   power is 200 (Enchant Item) or 10000 (Create Artifact)
   Numbered: BUG #69, #93 (BUGS_AND_QUIRKS.txt) */
int Screen_Item_Forge(int player, int artifact)
{
    int limit;
    int i;
    int group;
    int input;
    int done;
    int name_edited;
    int toggled;                                /* read before its first assignment on the first click (original) */
    int pct;
    int j;
    int n;
    char msg[150];
    int icon_ranges[10][2] = { 0, 9, 9, 11, 20, 9, 29, 9, 38, 9, 107, 9, 72, 35, 62, 10, 47, 8, 55, 7 };   /* [ItemType]: first ITEMS
                                                   icon, icon count */
    int k;

    Mem_Clear_Far(&p_items[ITEM_REC_FORGE], sizeof(struct Item));
    if (artifact != 0)
        limit = 10000;
    else
        limit = 200;
    forge_icon = 0;
    n = 0;
    forge_type_mask = 1;
    name_edited = 0;
    ItemForge_Load();
    ItemForge_Build_Power_List(player, limit);
    UI_Set_Redraw_Fn(ItemForge_Redraw, 2);
    forge_power_count = 0;
    for (k = 0; k < 4; k++)
        forge_powers[k] = -2;
    present_mode = PRESENT_DISSOLVE;
    done = 0;
    for (j = 0; j < 54; j++)                    /* the ints of its 0x6c bytes: one per power row */
        p_forge_power_selected[j] = 0;
    forge_cost = ItemForge_Calc_Cost(ITEM_REC_FORGE);
    strcpy(message_text, ItemForge_Auto_Name(ITEM_REC_FORGE), 0);
    ItemForge_Build_Item(ITEM_REC_FORGE);
    ItemForge_Add_Controls();
again:
    while (done == 0) {
        input = UI_Get_Input();
        if (input == forge_ok_btn)
            done = 1;
        for (i = 0; i < forge_column_rows[0] + forge_column_rows[1] + forge_column_rows[2]; i++) {
            if (p_forge_power_ctrls[i] == input) {
                if (p_item_powers[p_forge_power_slots[i]].group != ITEMPOW_GROUP_NONE) {
                    group = p_item_powers[p_forge_power_slots[i]].group;
                    for (j = 0; p_item_powers[p_forge_power_slots[j]].group != group; j++)
                        ;
                    for (; p_item_powers[p_forge_power_slots[j]].group == group; j++) {
                        if (i != j) {
                            if (p_forge_power_selected[j] > 0) {
                                forge_power_count--;
                                for (k = 0; k < 4; k++) {
                                    if (forge_powers[k] == p_forge_power_slots[j]) {
                                        p_items[ITEM_REC_FORGE].powers = p_items[ITEM_REC_FORGE].powers ^ p_item_powers[p_forge_power_slots[j]].powers;
                                        Array_Delete_Near(k, (char *)forge_powers, 2, 4);
                                        forge_powers[3] = -2;
                                    }
                                }
                            }
                            p_forge_power_selected[j] = 0;
                        }
                    }
                }
                for (k = 0; k < 4; k++) {
                    if (forge_powers[k] == p_forge_power_slots[i]) {
                        p_items[ITEM_REC_FORGE].powers = p_items[ITEM_REC_FORGE].powers ^ p_item_powers[p_forge_power_slots[i]].powers;
                        Array_Delete_Near(k, (char *)forge_powers, 2, 4);
                        forge_powers[3] = -2;
                        forge_power_count--;
                        p_forge_power_selected[i] = 1 - p_forge_power_selected[i];
                        toggled = 1;
                    }
                }
                if (forge_power_count == 4) {
                    UI_Clear_Controls();
                    LBX_Load_Records_Near("message", 0, msg, 0, 1, 150);
                    Message_Box(msg);
                } else if (toggled != 1) {
                    if (p_forge_power_slots[i] == ITEMPOW_SPELL_CHARGES) {
                        ItemForge_Build_Charge_Spell_List();
                        j = Screen_Item_Charge_Spell();
                        if (j > 0) {
                            p_items[ITEM_REC_FORGE].spell = j;
                            forge_has_charges = 1;
                            forge_powers[forge_power_count] = p_forge_power_slots[i];
                            forge_power_count++;
                            p_forge_power_selected[i] = 1 - p_forge_power_selected[i];
                        } else {
                            p_items[ITEM_REC_FORGE].charges = 0;
                            p_items[ITEM_REC_FORGE].spell = SPELL_NONE;
                            forge_has_charges = 0;
                        }
                        UI_Set_Redraw_Fn(ItemForge_Redraw, 2);
                    } else {
                        forge_powers[forge_power_count] = p_forge_power_slots[i];
                        forge_power_count++;
                        p_forge_power_selected[i] = 1 - p_forge_power_selected[i];
                    }
                }
                toggled = 0;
                forge_cost = ItemForge_Calc_Cost(ITEM_REC_FORGE);
            }
        }
        for (i = 0; i < 10; i++) {
            if (p_forge_type_btns[i] == input) {
                forge_type_mask = 1 << i;
                j = forge_type_mask;
                n = 0;
                while (j > 1) {
                    n++;
                    j = j >> 1;
                }
                forge_icon = icon_ranges[n][0];
                p_items[ITEM_REC_FORGE].icon = forge_icon;
                ItemForge_Build_Power_List(player, limit);
                for (j = 0; j < forge_column_rows[0] + forge_column_rows[1] + forge_column_rows[2]; j++) {
                    p_forge_power_selected[j] = 0;
                    for (k = 0; k < 4; k++) {
                        if (p_forge_power_slots[j] == forge_powers[k]) {
                            forge_powers[k] += 100;   /* marks it kept: still in the new type's list */
                            p_forge_power_selected[j] = 1;
                        }
                    }
                }
                for (k = 0; k < 4; k++) {
                    if (forge_powers[k] < 100) {
                        if (forge_powers[k] > -1) {
                            if (forge_powers[k] == ITEMPOW_SPELL_CHARGES) {
                                p_items[ITEM_REC_FORGE].charges = 0;
                                p_items[ITEM_REC_FORGE].spell = SPELL_NONE;
                            }
                            p_items[ITEM_REC_FORGE].powers = p_items[ITEM_REC_FORGE].powers ^ p_item_powers[forge_powers[k]].powers;
                            Array_Delete_Near(k, (char *)forge_powers, 2, 4);
                            k--;
                            forge_powers[3] = -2;
                            forge_power_count--;
                        }
                    } else {
                        forge_powers[k] -= 100;
                    }
                }
                forge_cost = ItemForge_Calc_Cost(ITEM_REC_FORGE);
                break;
            }
        }
        if (input == forge_name_field)
            name_edited = 1;
        if (input == forge_icon_prev_btn) {
            forge_icon--;
            if (forge_icon < icon_ranges[n][0])
                forge_icon = forge_icon + icon_ranges[n][1];
        }
        if (input == forge_icon_next_btn) {
            forge_icon++;
            if (icon_ranges[n][0] + icon_ranges[n][1] == forge_icon)
                forge_icon = icon_ranges[n][0];
        }
        ItemForge_Add_Controls();
        ItemForge_Build_Item(ITEM_REC_FORGE);
        if (name_edited == 0)
            strcpy(message_text, ItemForge_Auto_Name(ITEM_REC_FORGE), 0);
        if (done == 0) {
            Set_Draw_Page();
            ItemForge_Redraw();
            UI_Draw_Controls();
            Screen_Present();
            present_mode = PRESENT_NORMAL;
        }
    }
    pct = Spell_Cost_Reduction_Pct(0, SPELL_ENCHANT_ITEM);   /* always the human's (original) */
    forge_cost = forge_cost - (long)forge_cost * pct / 100;
    if (forge_power_count == 0) {
        LBX_Load_Records_Near("message", 0, msg, 1, 1, 150);
        done = Confirm_Box(msg);
        forge_cost = 0;
        if (done == 0)
            goto again;
    }
    UI_Clear_Controls();
    return forge_cost;
}

/* (item) writes the design into the item record: the name from message_text, icon, type, slot class, cost and
   the powers (stat powers into their fields, the rest into the power bits); chain mail +1 and plate mail +2 defense */
void ItemForge_Build_Item(int item)
{
    int n;
    int spell;
    int charges;
    int k;

    spell = (unsigned char)p_items[item].spell;
    charges = p_items[item].charges;
    Mem_Clear_Far(&p_items[item], sizeof(struct Item));
    far_strcpy(p_items[item].name, message_text, 0);
    p_items[item].spell = spell;
    p_items[item].charges = charges;
    k = forge_type_mask;
    n = 0;
    while (k > 1) {
        n++;
        k = k >> 1;
    }
    p_items[item].icon = forge_icon;
    p_items[item].item_type = n;
    p_items[item].cost = forge_cost;
    switch (n) {
    case ITEM_TYPE_SWORD:
    case ITEM_TYPE_MACE:
    case ITEM_TYPE_AXE:
        p_items[item].slot_type = ITEM_SLOT_MELEE;
        break;
    case ITEM_TYPE_BOW:
        p_items[item].slot_type = ITEM_SLOT_BOW;
        break;
    case ITEM_TYPE_STAFF:
    case ITEM_TYPE_WAND:
        p_items[item].slot_type = ITEM_SLOT_MAGIC;
        break;
    case ITEM_TYPE_MISC:
        p_items[item].slot_type = ITEM_SLOT_MISC;
        break;
    case ITEM_TYPE_SHIELD:
    case ITEM_TYPE_CHAIN_MAIL:
    case ITEM_TYPE_PLATE_MAIL:
        p_items[item].slot_type = ITEM_SLOT_ARMOR;
        break;
    }
    for (k = 0; k < 4; k++) {
        if (forge_powers[k] > -1) {
            if (forge_powers[k] < ITEMPOW_DEFENSE_FIRST)
                p_items[item].attack = forge_powers[k] + 1;
            else if (forge_powers[k] < ITEMPOW_TO_HIT_FIRST)
                p_items[item].defense = forge_powers[k] - (ITEMPOW_DEFENSE_FIRST - 1);
            else if (forge_powers[k] < ITEMPOW_MOVES_FIRST)
                p_items[item].to_hit = forge_powers[k] - (ITEMPOW_TO_HIT_FIRST - 1);
            else if (forge_powers[k] < ITEMPOW_RESIST_FIRST)
                p_items[item].moves = (forge_powers[k] - (ITEMPOW_MOVES_FIRST - 1)) * 2;
            else if (forge_powers[k] < ITEMPOW_SKILL_FIRST)
                p_items[item].resist = forge_powers[k] - (ITEMPOW_RESIST_FIRST - 1);
            else if (forge_powers[k] < ITEMPOW_SAVE_FIRST)
                p_items[item].spell_skill = (forge_powers[k] - (ITEMPOW_SKILL_FIRST - 1)) * 5;
            else if (forge_powers[k] < ITEMPOW_SPECIAL_FIRST)
                p_items[item].spell_save = forge_powers[k] - (ITEMPOW_SAVE_FIRST - 1);
            else
                p_items[item].powers = p_items[item].powers | p_item_powers[forge_powers[k]].powers;
        }
    }
    if (n == ITEM_TYPE_CHAIN_MAIL)
        p_items[item].defense = p_items[item].defense + 1;
    if (n == ITEM_TYPE_PLATE_MAIL)
        p_items[item].defense = p_items[item].defense + 2;
}

/* (player, max_cost) -> the rows: the stat powers for the type costing at most max_cost, by group (a gap row
   between groups, a new column after 16 rows), then in column 3 the special powers the player has the books for
   Numbered: BUG #70 (BUGS_AND_QUIRKS.txt) */
int ItemForge_Build_Power_List(int player, int max_cost)
{
    int count;
    int column;
    int i;
    int group;
    int last_gap;

    count = 0;
    last_gap = 0;
    column = 0;
    for (i = 0; i < 3; i++)
        forge_column_rows[i] = 0;
    for (i = 0; i < 33; i++) {
        if ((p_item_powers[i].type_mask & forge_type_mask) > 0 && p_item_powers[i].cost <= max_cost) {
            if (count != 0 && p_item_powers[i].group != group) {
                p_forge_power_slots[count] = -1;
                last_gap = count;
                forge_column_rows[column] = forge_column_rows[column] + 1;
                count++;
            }
            group = p_item_powers[i].group;
            forge_column_rows[column] = forge_column_rows[column] + 1;
            if (forge_column_rows[column] > 17 && p_forge_power_slots[count - 1] == -1) {
                count--;
                forge_column_rows[column] = forge_column_rows[column] - 1;
            }
            if (forge_column_rows[column] > 16) {
                forge_column_rows[column + 1] = forge_column_rows[column] - last_gap;
                forge_column_rows[column] = last_gap;
                column++;
            }
            p_forge_power_slots[count] = i;
            count++;
        }
    }
    column = 2;
    for (i = 33; i < 66; i++) {
        if ((p_item_powers[i].type_mask & forge_type_mask) > 0 &&
            wizards[player].spellbooks[p_item_powers[i].realm] >= p_item_powers[i].books_required &&
            p_item_powers[i].cost <= max_cost) {
            p_forge_power_slots[count] = i;
            forge_column_rows[column] = forge_column_rows[column] + 1;
            count++;
        }
    }
    return count;
}

/* redraw callback of the Spell Charges screen: the forge, "Choose a spell..." (message 2), two columns of 14 spells,
   the page corners, the charges box at the chosen spell */
void Item_Charge_Spell_Redraw(void)
{
    int x0;
    int i;
    int y0;
    int col;
    int row;
    char colors_a[6];
    char colors_b[6];

    x0 = 28;
    y0 = 12;
    ItemForge_Redraw();
    Pic_Reset_Frame(forge_ok_btn_pic);
    Draw_Pic_Frame(281, 180, forge_ok_btn_pic);
    Draw_Pic_Frame(x0, y0, spellbook_small_pic);
    colors_a[0] = 0x13;
    colors_a[1] = 0xb1;
    colors_a[2] = 0xb2;
    colors_a[3] = 0xb3;
    colors_a[4] = 0xb3;
    Set_Text_Shadow_Color(0);
    Set_Font_Color(19);
    Font_Select_Palette(4, colors_a);
    Font_Select_Outline(4, 15);
    LBX_Load_Records_Near("message", 0, message_text, 2, 1, 150);
    Print_Text_Centered(160, 2, message_text);
    for (i = 1; i < 5; i++)
        colors_a[i] = 0x27;
    for (i = 1; i < 5; i++)
        colors_b[i] = 0xa6;
    if (forge_spell_list_top > 0)
        Draw_Pic_Frame(x0 + 13, y0 + 4, spellbook_corner_left_pic);
    if (forge_spell_list_top + 28 < list_count)
        Draw_Pic_Frame(x0 + 258, y0 + 4, spellbook_corner_right_pic);
    for (i = 0; i < 28 && i + forge_spell_list_top < list_count; i++) {
        if (forge_spell_list_sel == i) {
            Set_Text_Shadow_Color(0xba);
            Font_Select_Palette(3, colors_b);
        } else {
            Set_Text_Shadow_Color(0xc0);
            Font_Select_Palette(3, colors_a);
        }
        Font_Select_Shadow(3, 15);
        Print_Far_Text(x0 + i / 14 * 130 + 30, y0 + i % 14 * 10 + 7,
                       p_spell_data[p_forge_spell_list[i + forge_spell_list_top]].name);
    }
    if (forge_spell_list_sel >= 0) {
        if (forge_spell_list_sel > 13) {
            row = forge_spell_list_sel - 13;
            col = 1;
        } else {
            row = forge_spell_list_sel;
            col = 0;
        }
        if (row > 11)
            row = 11;
        if (forge_spell_list_sel == 0)
            row = 1;
        Draw_Pic_Frame(x0 + 155 - col * 140, y0 + row * 10 - 4, forge_charges_box_pic);
        Set_Text_Shadow_Color(0xc0);
        Font_Select_Palette(3, colors_a);
        Font_Select_Shadow(3, 15);
        Print_Text_Centered(x0 + 204 - col * 140, y0 + row * 10 + 1, "Charges");
    }
}

/* () -> the spell to embed (0: cancelled): the Spell Charges screen - pick a spell, then x1..x4 charges */
unsigned char Screen_Item_Charge_Spell(void)
{
    int i;
    int done;                                   /* also the charge buttons' column (x0 + 158 - done * 140) */
    int spell;
    int input;
    int cancel;
    int k;
    int x0;
    int y0;
    int next;
    int prev;
    int ctrls[28];
    char saved[30];

    strcpy(saved, message_text);
    x0 = 28;
    y0 = 12;
    Pic_Apply_Palette(spellbook_small_pic);
    Remap_Build_Shadow();
    forge_spell_list_sel = -1;
    UI_Clear_Controls();
    forge_spell_list_top = 0;
    UI_Set_Redraw_Fn(Item_Charge_Spell_Redraw, 2);
    cancel = UI_Add_Region(187, 168, 205, 199, UI_NO_HOTKEY, -1);
    next = UI_Add_Region(x0 + 259, y0 + 2, x0 + 272, y0 + 15, "F", -1);
    prev = UI_Add_Region(x0 + 13, y0 + 2, x0 + 26, y0 + 14, "B", -1);
    Font_Select_Shadow(3, 15);
    for (i = 0; i < 28; i++)
        ctrls[i] = UI_Add_Region(x0 + i / 14 * 130 + 28, y0 + i % 14 * 10 + 7, x0 + i / 14 * 130 + 133,
                                  y0 + i % 14 * 10 + 16, "", -1);
    done = 0;
    while (done == 0) {
        input = UI_Get_Input();
        if (input == cancel) {
            spell = SPELL_NONE;
            done = 1;
        }
        if (input == next && forge_spell_list_top + 28 < list_count)
            forge_spell_list_top += 28;
        if (input == prev && forge_spell_list_top > 0)
            forge_spell_list_top -= 28;
        for (i = 0; i < 28; i++) {
            if (ctrls[i] == input && p_forge_spell_list[i + forge_spell_list_top] > 0) {
                spell = p_forge_spell_list[i + forge_spell_list_top];
                forge_spell_list_sel = i;
                p_items[ITEM_REC_FORGE].charges = 0;
                UI_Clear_Controls();
                if (i > 13) {                   /* done here: 1 a right-hand spell, its buttons to the left */
                    done = 1;
                    i = i - 14;
                    if (i > 10)
                        i = 10;
                } else {
                    done = 0;
                    if (i > 11)
                        i = 11;
                }
                for (k = 0; k < 4; k++)
                    ctrls[k] = UI_Add_Button(x0 + 158 - done * 140 + k * 23, y0 + done * 10 + i * 10 + 11, "",
                                             forge_charge_btn_pics[k], "", -1);
                i = list_count;
                while (p_items[ITEM_REC_FORGE].charges == 0) {
                    input = UI_Get_Input();
                    for (k = 0; k < 4; k++) {
                        if (ctrls[k] == input)
                            p_items[ITEM_REC_FORGE].charges = k + 1;
                    }
                    if (p_items[ITEM_REC_FORGE].charges == 0) {
                        Set_Draw_Page();
                        Item_Charge_Spell_Redraw();
                        Screen_Present();
                    }
                }
                done = 1;
                break;
            }
        }
        if (done == 0) {
            Set_Draw_Page();
            Item_Charge_Spell_Redraw();
            Screen_Present();
        }
    }
    strcpy(message_text, saved);
    return spell;
}

/* (ITEMDATA record or -1) -> the new item: the record (marked in game, its moves doubled) or the random item in
   temp record 136, copied into the first free item record
   Numbered: BUG #79 (BUGS_AND_QUIRKS.txt) */
int Item_Create_From_ITEMDATA(int record)
{
    int result;
    struct ItemData rec;

    if (record != -1) {
        p_items_in_game[record] = 1;
        LBX_Load_Records_Near("Itemdata", 0, &rec, record, 1, sizeof(struct ItemData));
        rec.item.moves = rec.item.moves << 1;
        Mem_Copy_Far(&p_items[ITEM_REC_WORK], (struct ItemData far *)&rec, sizeof(struct Item));
    }
    result = Item_Alloc_Copy(ITEM_REC_WORK);
    return result;
}

/* (src item) -> the first free item record (cost <= 0) of 0..131, now a copy of src; -1 none */
int Item_Alloc_Copy(int src_item)
{
    int i;
    int result;

    result = -1;
    for (i = 0; i < ITEMS_IN_PLAY; i++) {
        if (p_items[i].cost <= 0) {
            Mem_Copy_Far(&p_items[i], &p_items[src_item], sizeof(struct Item));
            result = i;
            break;
        }
    }
    return result;
}

/* (prefab_kind - Item_Create_Prefab's PREFAB_*: PREFAB_NO_ARTIFACT a weak item -, max_cost, 0 = 800..1700) a random item in temp record 136: random type and icon, random powers of
   different groups - Random(33) - 1: only the stat powers 0..32, never a special power - until the type's power
   count, the cost limit or 50 tries; the three tests run only once a power was taken, so with no fitting power the
   loop never ends. BUG #68: the power count reads the type before it is rolled
   Numbered: BUG #68 (BUGS_AND_QUIRKS.txt) */
void Item_Create_Random(int prefab_kind, int max_cost)
{
    int item_type;
    int limit;
    int power;
    int again;
    int max_powers;
    int tries;
    int icon_ranges[10][2] = { 0, 9, 9, 11, 20, 9, 29, 9, 38, 9, 107, 9, 72, 35, 62, 10, 47, 8, 55, 7 };   /* [ItemType]: first ITEMS
                                                   icon, icon count */
    register int i;
    register int count;

    count = 0;
    forge_cost = 0;
    for (i = 0; i < 4; i++)
        forge_powers[i] = -2;
    if (max_cost == 0)
        max_cost = Random(10) * 100 + 700;
    if (prefab_kind == PREFAB_NO_ARTIFACT) {
        limit = 200;
        max_powers = 2;
    } else {
        limit = 20000;
        if (item_type < ITEM_TYPE_AXE || item_type == ITEM_TYPE_STAFF || item_type == ITEM_TYPE_WAND)
            max_powers = 4;
        else
            max_powers = 3;
    }
    item_type = Random(10) - 1;
    forge_type_mask = 1 << item_type;
    forge_icon = icon_ranges[item_type][0] + Random(icon_ranges[item_type][1]) - 1;
    p_items[ITEM_REC_WORK].spell = SPELL_NONE;
    p_items[ITEM_REC_WORK].charges = 0;
    p_items[ITEM_REC_WORK].item_type = item_type;
    p_items[ITEM_REC_WORK].icon = forge_icon;
    p_item_powers = (struct ItemPower *)Near_Alloc_Reset(0x7da);
    LBX_Load_Records_Near("ITEMPOW", 0, p_item_powers, 0, 67, 30);
    again = 1;
    tries = 0;
    while (again != 0) {
        tries++;
        power = Random(33) - 1;
        if ((p_item_powers[power].type_mask & forge_type_mask) > 0 && p_item_powers[power].cost <= limit) {
            for (i = 0; i < count; i++) {
                if (p_item_powers[power].group == p_item_powers[forge_powers[i]].group) {
                    power = -1;
                    break;
                }
            }
            if (power >= 0) {
                forge_powers[count] = power;
                forge_cost = ItemForge_Calc_Cost(ITEM_REC_WORK);
                strcpy(message_text, ItemForge_Auto_Name(ITEM_REC_WORK));
                ItemForge_Build_Item(ITEM_REC_WORK);
                count++;
            }
        }
        if (count > 0) {
            if (count == max_powers)
                again = 0;
            if (forge_cost > max_cost)
                again = 0;
            if (tries > 50)
                again = 0;
        }
    }
}

/* (prefab_kind PREFAB_*: any / no artifacts / only artifacts, the spellbooks, max_cost) -> a new item: up to 50 tries of a random
   ITEMDATA record not yet in the game, within the books and between max_cost / 2 and max_cost (no cost window for max_cost 0); else a random
   item */
int Item_Create_Prefab(int prefab_kind, int *spellbooks, int max_cost)
{
    int record;
    int i;
    int tries;
    struct ItemData rec;

    record = -1;
    tries = 0;
    while (record == -1 && tries < 50) {
        tries++;
        record = Random(NUM_PREFAB_ITEMS) - 1;
        LBX_Load_Records_Near("Itemdata", 0, &rec, record, 1, sizeof(struct ItemData));
        if (p_items_in_game[record] == 1)
            record = -1;
        for (i = 0; i < 5; i++) {
            if ((char)rec.books_required[i] > spellbooks[i])
                record = -1;
        }
        if (prefab_kind == PREFAB_NO_ARTIFACT && (char)rec.is_artifact > 0)
            record = -1;
        if (prefab_kind == PREFAB_ARTIFACT && rec.is_artifact == 0)
            record = -1;
        if ((rec.item.cost > max_cost || max_cost / 2 > rec.item.cost) && max_cost != 0)
            record = -1;
    }
    if (record == -1)
        Item_Create_Random(prefab_kind, max_cost);
    record = Item_Create_From_ITEMDATA(record);
    return record;
}
