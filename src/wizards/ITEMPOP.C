/* ITEMPOP.C - WIZARDS.EXE overlay code segment 6cba (stub 249a): item cards - the card that grows out of a point
   when an item is viewed, the card of a newly awarded item, the card itself (background, icon, name, power lines)
   and the item icon with its realm outline (the realm of the first power found in a fixed priority list), whose
   colors cycle with item_outline_flash_phase.
   This module's data (DS:4ab2): its literal "\x1b"; BSS (DS:c2f0..c2fc).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6cba src/wizards/ITEMPOP.C */
#include "GAME.H"

unsigned p_item_card_pic;                       /* DS:c2fa  scratch picture the card is built in */
int item_popup_item;                            /* DS:c2f8 */
int item_popup_x;                               /* DS:c2f6  the point the card grows out of */
int item_popup_y;                               /* DS:c2f4 */
int item_outline_flash_div;                     /* DS:c2f2  0 / 1: item_outline_flash_phase advances every 2nd tick */
int item_outline_flash_phase;                   /* DS:c2f0  0..7 */

extern unsigned char realm_outline_colors[5][8][8];  /* DS:3062  per realm (0 Chaos .. 4 Sorcery) 8 phases x 8 colors */
extern unsigned item_icon_pics[];               /* DS:913e */
extern unsigned items_bg_pic;                   /* DS:958c */
extern unsigned item_power_bullet_pic;          /* DS:976a */
extern unsigned item_card_pic;                  /* DS:976c */
extern unsigned p_pic_work;                     /* DS:998c */
extern unsigned p_item_icon_pic;                /* DS:c25e */
extern int award_item;                          /* DS:c270 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
void Item_View_Popup(int item, int x, int y);
void Item_View_Popup_Redraw(void);
void Item_Card_Draw(int item, unsigned card_pic, unsigned icon_pic);
void Item_Award_Popup(void);
void Item_Award_Popup_Redraw(void);
void Item_Build_Icon(int item, unsigned pic);
void Item_Outline_Cycle_Tick(void);

void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Zoom_Reset(void);
void Zoom_Step_Rect(int cx, int cy, unsigned pic, int steps, int *x, int *y);
void Play_Click_Sound(void);
void Screen_Restore_AC00(void);
void Draw_Bitmap(int x, int y, unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
int Pic_Draw_Into_Pic(int x, int y, unsigned dst, unsigned src);
void Pic_Create_Blank(int width, int height, unsigned pic);
void Pic_Apply_Palette(unsigned pic);
void Pic_Outline_Recolor(unsigned pic, int color);
void Pic_Replace_Color_Dithered(unsigned pic, int color, char *pattern);
void Remap_Build_Shadow(void);
void Set_Font_Color(int color);
void Set_Space_Width(int width);
void Print_Far_Text_To_Pic(int x, int y, char far *text, unsigned pic);
int Print_Text_To_Pic(int x, int y, char *text, unsigned pic);
int Text_Width(char *text);
void UnitView_Item_Power_Texts(int item, char names[][20], char descs[][40], int *count);

/* (item, x, y): the item's card growing out of (x, y) (zoom), until any click */
void Item_View_Popup(int item, int x, int y)
{
    int input;
    int control;
    int done;

    item_popup_item = item;
    item_popup_x = x;
    item_popup_y = y;
    UI_Set_Redraw_Fn(Item_View_Popup_Redraw, 1);
    Screen_Save_AC00();
    p_item_card_pic = Heap_Alloc_Back(p_scratch, ITEM_CARD_PARAS);
    p_item_icon_pic = Heap_Alloc_Back(p_scratch, 50);
    Zoom_Reset();
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control) {
            Play_Click_Sound();
            done = -1;
        }
        if (done == 0) {
            Item_View_Popup_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
}

/* redraw callback of Item_View_Popup: the saved screen, the card, one zoom step */
void Item_View_Popup_Redraw(void)
{
    int x;
    int y;

    Set_Draw_Page();
    Screen_Restore_AC00();
    Item_Card_Draw(item_popup_item, p_item_card_pic, p_item_icon_pic);
    Item_Outline_Cycle_Tick();
    Zoom_Step_Rect(item_popup_x, item_popup_y, p_item_card_pic, 5, &x, &y);
    Draw_Bitmap(x, y, p_item_card_pic);
}

/* (item, card pic, icon pic): builds the card - background, icon at 9,6, name, then the power lines (name + text)
   with a bullet each; 5 lines use the smaller font and spacing. Also used by the merchant screen */
void Item_Card_Draw(int item, unsigned card_pic, unsigned icon_pic)
{
    int width;
    int count;
    int line_h;
    int text_y;
    char colors[4];
    char names[5][20];
    char descs[5][40];
    int i;

    Pic_Decode_Frame_To_Bitmap(item_card_pic, card_pic);
    Item_Build_Icon(item, icon_pic);
    Pic_Draw_Into_Pic(9, 6, card_pic, icon_pic);
    Font_Select_Shadow(4, 4, 0, 0);
    Set_Font_Color(46);
    Set_Space_Width(1);
    Set_Text_Shadow_Color(47);
    Print_Far_Text_To_Pic(31, 12, p_items[item].name, card_pic);
    UnitView_Item_Power_Texts(item, names, descs, &count);
    Pic_Decode_Frame_To_Bitmap(item_power_bullet_pic, icon_pic);
    colors[0] = 46;
    colors[1] = 177;
    if (count == 5) {
        Font_Select_Palette(1, colors);
        Font_Select_Shadow(1, 15, 0, 0);
        line_h = 8;
        text_y = 34;
    } else {
        Font_Select_Palette(2, colors);
        Font_Select_Shadow(2, 15, 0, 0);
        line_h = 11;
        text_y = 33;
    }
    Set_Space_Width(1);
    Set_Text_Shadow_Color(47);
    for (i = 0; i < count; i++) {
        Pic_Draw_Into_Pic(11, i * line_h + 34, card_pic, icon_pic);
        Print_Text_To_Pic(20, text_y + i * line_h, names[i], card_pic);
        width = Text_Width(names[i]);
        Print_Text_To_Pic(width + 21, text_y + i * line_h, descs[i], card_pic);
    }
}

/* the card of award_item at 50,50 until a click, before it becomes the held item of the items screen */
void Item_Award_Popup(void)
{
    int input;
    int control;
    int done;

    Screen_Save_A800();
    Pic_Apply_Palette(items_bg_pic);
    Remap_Build_Shadow();
    p_item_card_pic = Heap_Alloc_Front(p_scratch, ITEM_CARD_PARAS);
    p_item_icon_pic = Heap_Alloc_Back(p_scratch, 60);
    UI_Set_Redraw_Fn(Item_Award_Popup_Redraw, 1);
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    done = 0;
    while (done == 0) {
        Timer_Mark();
        UI_Clear_Controls();
        control = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
        input = UI_Get_Input();
        if (input == control)
            done = -1;
        if (done == 0) {
            Item_Award_Popup_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
}

/* redraw callback of Item_Award_Popup */
void Item_Award_Popup_Redraw(void)
{
    Set_Draw_Page();
    Screen_Restore_A800();
    Item_Card_Draw(award_item, p_item_card_pic, p_item_icon_pic);
    Draw_Bitmap(50, 50, p_item_card_pic);
    Item_Outline_Cycle_Tick();
}

/* (item, pic): the 19 x 19 item icon (picture at 2,2) with an outline in the color of a realm - the first power of
   this priority list that the item has decides (Cloak of Fear is tested twice); no powers: no outline */
void Item_Build_Icon(int item, unsigned pic)
{
    long powers;
    int outline_row;

    Pic_Decode_Frame_To_Bitmap(item_icon_pics[p_items[item].icon], p_pic_work);
    Pic_Create_Blank(19, 19, pic);
    Pic_Draw_Into_Pic(2, 2, pic, p_pic_work);
    powers = p_items[item].powers;
    outline_row = -1;
    if (powers != 0) {
        if (powers & ITEM_POWER_HASTE)                   /* Haste */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_PHANTASMAL)            /* Phantasmal */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_DOOM)               /* Doom */
            outline_row = OUTLINE_CHAOS;
        else if (powers & ITEM_POWER_TRUE_SIGHT)            /* True Sight */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_LION_HEART)           /* Lion Heart */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_REGENERATION)                /* Regeneration */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_INVISIBILITY)              /* Invisibility */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_WRAITHFORM)                /* Wraith Form */
            outline_row = OUTLINE_DEATH;
        else if (powers & ITEM_POWER_PLANAR_TRAVEL)          /* Planar Travel */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_INVULNERABILITY)          /* Invulnerability */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_VAMPIRIC)                 /* Vampiric */
            outline_row = OUTLINE_CHAOS;
        else if (powers & ITEM_POWER_LIGHTNING)                 /* Lightning */
            outline_row = OUTLINE_CHAOS;
        else if (powers & ITEM_POWER_DESTRUCTION)                /* Destruction */
            outline_row = OUTLINE_CHAOS;
        else if (powers & ITEM_POWER_POWER_DRAIN)           /* Power Drain */
            outline_row = OUTLINE_DEATH;
        else if (powers & ITEM_POWER_ELEMENTAL_ARMOR)               /* Elemental Armor */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_MAGIC_IMMUNITY)             /* Magic Immunity */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_RIGHTEOUSNESS)          /* Righteousness */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_MERGING)          /* Merging */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_FLIGHT)             /* Flight */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_PATHFINDING)                /* Pathfinding */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_GUARDIAN_WIND)                 /* Guardian Wind */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_GIANT_STRENGTH)           /* Giant Strength */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_HOLY_AVENGER)            /* Holy Avenger */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_DEATH)             /* Death */
            outline_row = OUTLINE_DEATH;
        else if (powers & ITEM_POWER_FLAMING)            /* Flaming */
            outline_row = OUTLINE_CHAOS;
        else if (powers & ITEM_POWER_CLOAK_OF_FEAR)                 /* Cloak of Fear */
            outline_row = OUTLINE_DEATH;
        else if (powers & ITEM_POWER_RESIST_ELEMENTS)               /* Resist Elements */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_STONING)              /* Stoning */
            outline_row = OUTLINE_NATURE;
        else if (powers & ITEM_POWER_RESIST_MAGIC)             /* Resist Magic */
            outline_row = OUTLINE_SORCERY;
        else if (powers & ITEM_POWER_CLOAK_OF_FEAR)                 /* Cloak of Fear again: never reached */
            outline_row = OUTLINE_DEATH;
        else if (powers & ITEM_POWER_BLESS)           /* Bless */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_ENDURANCE)              /* Endurance */
            outline_row = OUTLINE_LIFE;
        else if (powers & ITEM_POWER_WATER_WALKING)               /* Water Walking */
            outline_row = OUTLINE_NATURE;
    }
    if (outline_row != -1) {
        Pic_Outline_Recolor(pic, 0xff);
        Pic_Replace_Color_Dithered(pic, 0xff, realm_outline_colors[outline_row][item_outline_flash_phase]);
    }
}

/* the item aura animation (the item outline: UNITDRAW.C's Outline_Cycle_Tick does the same for units):
   item_outline_flash_phase 0..7 advances every 2nd call */
void Item_Outline_Cycle_Tick(void)
{
    item_outline_flash_div = item_outline_flash_div + 1;
    if (item_outline_flash_div > 1)
        item_outline_flash_div = 0;
    if (item_outline_flash_div < 1) {
        item_outline_flash_phase = item_outline_flash_phase + 1;
        if (item_outline_flash_phase > 7)
            item_outline_flash_phase = 0;
    }
}
