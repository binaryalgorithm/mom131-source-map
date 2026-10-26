/* HIRE.C - WIZARDS.EXE overlay code segment aab2: the offers that arrive between turns - the merchant's magic item
   ("A merchant arrives and offers a magic <item> for sale. The price is only <n> gold pieces.", Buy / Reject,
   half price with Charismatic), the hero for hire / summoned hero / champion / prisoner (unit view, Hire or Accept /
   Reject, then "Name Your Hero"), the mercenaries for hire, and the AI's own hero hiring.
   This module's data (DS:6504): its literals; BSS (DS:c9de..c9ef).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS aab2 src/wizards/HIRE.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Merchant_Screen_Load(void);
void Merchant_Screen_Redraw(void);
void Screen_Merchant_Offer(void);
void Hire_Screen_Load(int unit_type);
void Hire_Screen_Redraw(void);
int Screen_Hire_Hero(int slot, int hero_type, int offer_kind);
void Mercenary_Screen_Redraw(void);
int Screen_Hire_Mercenaries(int unit_type, int count, int level, int cost);
int AI_Hire_Hero(int player, int slot, int unit_type);

#include "GAME.H"

int hire_offer_cost;                            /* DS:c9ee  the offer's price in gold (Charismatic: halved) */
int hire_merc_type;                             /* DS:c9ec */
int hire_merc_count;                            /* DS:c9ea */
unsigned hire_bg_pic;                           /* DS:c9e8  HIRE.LBX 0 (hero / mercenaries) or 2 (merchant) */
unsigned merchant_button_pic;                   /* DS:c9e6  HIRE.LBX 1 */
unsigned hire_name_box_pic;                     /* DS:c9e4  HIRE.LBX 4 (loaded, never drawn) */
int hire_offer_kind;                            /* DS:c9e2  0 hero for hire, 1 hero summoned, 3 champion, else prisoner */
int hire_name_entry;                            /* DS:c9e0  the hero screen: 1 = hired, the name box is shown; the
                                                   merchant screen: the offered item */
#define merchant_item hire_name_entry          /* the merchant screen's use: the offered item */
char *p_hire_hero_name;                         /* DS:c9de  13 bytes: the hero's name (NAMES.LBX) */

extern unsigned ok_button_pic;                  /* DS:9498 */
extern unsigned name_entry_box_pic;             /* DS:953c */
extern int present_mode;                        /* DS:9c8e */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern unsigned p_minimap_pic;                  /* DS:c086 */
extern int unit_view_abil_first;                /* DS:c18c */
extern int unit_view_abil_count;                /* DS:c190 */
extern struct UnitAbilityList *p_unit_view_list;       /* DS:c192 */
extern unsigned forge_icon_pic;                 /* DS:c8dc */
                                                /* ITEMFORG.C's icon buffer; here the merchant's item card and the hire
                                                   screen's item picture */
extern unsigned ui_temp_pic_b;                  /* DS:ca36 */

void Font_Select_Shadow(int font, int c1, int c2, int c3);
int Print_Text_Centered(int x, int y, char *text);
void far_strcpy();                              /* unprototyped: called with (near dst, 0, far src) and
                                                   (far dst, near src, 0) */
void Near_Alloc_Release();                      /* called here with an argument */
void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
void Set_Draw_Page_Front(void);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Item_Outline_Cycle_Tick(void);
void Item_Card_Draw(int item, unsigned card_pic, unsigned icon_pic);
void Set_Font_Color(int color);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void Draw_Bitmap(int x, int y, unsigned pic);
void Income_Warn_Blink_Off(void);
void Income_Warn_Blink_On(void);
void UI_Set_Press_Offset(int dx, int dy);
void Font_Select(int font, int c1, int c2, int c3);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Input_Delay(int delay);
void Item_Free(int item);
void Map_Redraw_Dissolve(void);
void Items_Screen_Award(int count, int *items);
void Heap_Release(unsigned block);
void Heap_Reset(unsigned block);
void Near_Alloc_Reset_All(void);
void UnitView_Alloc_Buffers(void);
void Load_Unit_Figure(int unit_or_type, int is_type);
void Draw_Unit_View(int x, int y, int mode, int subject, struct UnitAbilityList *list, int count, unsigned pic);
void Pic_Reset_Frame(unsigned pic);
void Load_Unit_View_Graphics(void);
int Hero_Spawn(int player, int hero, int slot, int from_dead);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void UnitView_Build_Abilities(int unit, struct UnitAbilityList *list, int *count);
void UnitView_Init_Hire(int unit, int count);
void UnitView_Add_Ability_Controls(int x, int y, struct UnitAbilityList *list, int count, int first);
void UI_Draw_Controls(void);
void VGA_Present_Dissolve(void);
void UI_Set_Help(struct HelpEntry *help, int count);
void UnitView_Set_Ability_Help(struct UnitAbilityList *list, int count, int help_base);
void UnitView_Scroll_Abilities(int input, int *rows_out);
void Play_Click_Sound(void);
void Play_Click_Sound_Casting(void);
void Stack_Limit_Enforce(int unit);
void UI_Set_Control_Count(int n);
int UI_Text_Input_Box();                        /* no prototype here: called with 11 arguments (the definition in UI.C takes 10) */
void Map_Full_Redraw(void);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);

/* () the merchant screen's pictures: the map behind it, HIRE.LBX 2 / 1, room for the item picture */
void Merchant_Screen_Load(void)
{
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Redraw();
    Screen_Save_A800();
    Set_Draw_Page_Front();
    hire_bg_pic = LBX_Reload_Next("HIRE", 2, p_scratch);
    merchant_button_pic = LBX_Reload_Next("HIRE", 1, p_scratch);
    forge_icon_pic = Heap_Alloc_Back(p_scratch, ITEM_CARD_PARAS);
    ui_temp_pic_b = Heap_Alloc_Back(p_scratch, 0x1e);
}

/* () "A merchant arrives and offers a magic <item> for sale.  The price is only <n> gold pieces." and the item */
void Merchant_Screen_Redraw(void)
{
    char text[30];
    int x;
    int y;

    x = 5;
    y = 15;
    Item_Outline_Cycle_Tick();
    Item_Card_Draw(merchant_item, forge_icon_pic, ui_temp_pic_b);
    Screen_Restore_A800();
    Draw_Pic_Frame(x, y, hire_bg_pic);
    strcpy(message_text, "A merchant arrives and offers a magic ");
    far_strcpy(text, 0, p_items[merchant_item].name);
    strcat(message_text, text);
    strcat(message_text, " for sale.  The price is only ");
    itoa(hire_offer_cost, text, 10);
    strcat(message_text, text);
    strcat(message_text, " gold pieces.");
    Set_Font_Color(0xa3);
    Font_Select_Shadow(4, 4, 0, 0);
    Set_Text_Shadow_Color(0xfd);
    Paragraph_Print(x + 57, y + 9, 182, message_text, PARA_LEFT);
    Draw_Bitmap(x + 14, y + 65, forge_icon_pic);
}

/* () the merchant's offer (item merchant_item): Buy pays the price and awards the item, Reject frees it */
void Screen_Merchant_Offer(void)
{
    int reject;
    int buy;
    int done;
    int input;
    int x;
    int y;

    Income_Warn_Blink_Off();
    UI_Set_Press_Offset(1, 1);
    Merchant_Screen_Load();
    x = 5;
    y = 15;
    UI_Clear_Controls();
    hire_offer_cost = p_items[merchant_item].cost;
    if (wizards[PLAYER_HUMAN].retorts[RETORT_CHARISMATIC] > 0)
        hire_offer_cost = hire_offer_cost / 2;
    Font_Select(4, 4, 0, 0);
    buy = UI_Add_Button(x + 252, y + 121, "Buy", merchant_button_pic, "", -1);
    reject = UI_Add_Button(x + 252, y + 140, "Reject", merchant_button_pic, UI_NO_HOTKEY, -1);
    UI_Set_Redraw_Fn(Merchant_Screen_Redraw, 2);
    done = 0;
    UI_Input_Delay(2);
    while (!done) {
        input = UI_Get_Input();
        if (input == reject) {
            Item_Free(merchant_item);
            done = 1;
        }
        if (input == buy) {
            wizards[PLAYER_HUMAN].gold -= hire_offer_cost;
            Map_Redraw_Dissolve();
            Items_Screen_Award(1, &merchant_item);
            current_screen = SCREEN_MAP;
            done = 1;
        }
        if (!done) {
            Set_Draw_Page();
            Merchant_Screen_Redraw();
            Screen_Present();
        }
    }
    Heap_Release(p_scratch);
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
}

/* (unit_type) the hero / mercenary screen's pictures; a hero type (< 35) also loads its name from NAMES.LBX */
void Hire_Screen_Load(int unit_type)
{
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    Heap_Reset(p_scratch);
    Near_Alloc_Reset_All();
    UnitView_Alloc_Buffers();
    p_minimap_pic = Heap_Alloc_Back(p_scratch, MINIMAP_PIC_PARAS);
    hire_bg_pic = LBX_Reload_Next("HIRE", 0, p_scratch);
    forge_icon_pic = Heap_Alloc_Back(p_scratch, 0x1e);
    hire_name_box_pic = LBX_Reload_Next("HIRE", 4, p_scratch);
    p_hire_hero_name = Near_Alloc(13);
    if (unit_type > UNIT_HERO_CHOSEN)
        Load_Unit_Figure(unit_type, 1);
    else
        LBX_Load_Records_Near("NAMES", 0, p_hire_hero_name, unit_type, 1, 13);
}

/* () the hero offer: the unit view and the title by offer kind; once hired, "Name Your Hero" */
void Hire_Screen_Redraw(void)
{
    char text[10];
    int x;
    int y;

    x = 25;
    y = 17;
    Screen_Restore_A800();
    Draw_Unit_View(x, y, UNIT_VIEW_OVERLAND, num_units - 1, p_unit_view_list, unit_view_abil_count, forge_icon_pic);
    Draw_Pic_Frame(x - 25, 0, hire_bg_pic);
    Set_Font_Color(0xa3);
    Font_Select(4, 4, 4, 4);
    if (hire_offer_kind == HIRE_OFFER_HIRE) {
        strcpy(message_text, "Hero for Hire: ");
        itoa(hire_offer_cost, text, 10);
        strcat(message_text, text);
        strcat(message_text, " gold");
    } else if (hire_offer_kind == HIRE_OFFER_SUMMONED) {
        strcpy(message_text, "Hero Summoned");
    } else if (hire_offer_kind == HIRE_OFFER_CHAMPION) {
        strcpy(message_text, "Champion Summoned");
    } else {
        strcpy(message_text, "A prisoner asks to join your army");
    }
    Print_Text_Centered(x + 103, y - 11, message_text);
    if (hire_name_entry == 1) {
        Font_Select(4, 4, 4, 4);
        Draw_Pic_Frame(x + 29, y + 55, name_entry_box_pic);
        Print_Text_Centered(x + 104, y + 61, "Name Your Hero");
        Pic_Reset_Frame(ok_button_pic);
        Draw_Pic_Frame(x + 221, y + 143, ok_button_pic);
        Pic_Reset_Frame(ok_button_pic);
        Draw_Pic_Frame(x + 221, y + 162, ok_button_pic);
        Print_Text_Centered(x + 246, y + 144, hire_offer_kind == HIRE_OFFER_HIRE ? "Hire" : "Accept");
        Print_Text_Centered(x + 246, y + 163, "Reject");
    }
}

/* (slot, hero_type, offer_kind) -> 1 = hired: the hero is created in the human's hero slot; offer kind 0 (for hire)
   costs its price plus a quarter per level (Charismatic: half), and a hero the human cannot afford is turned away
   at once. Hire / Accept names the hero (kinds 1 and 3 appear at the summoning circle); Reject dismisses it - a hero
   for hire comes back one level higher (up to Champion)
   Numbered: BUG #179 (BUGS_AND_QUIRKS.txt) */
int Screen_Hire_Hero(int slot, int hero_type, int offer_kind)
{
    int input;
    int done;
    int scratch;                                /* the caret-color loop index, then the scroll rows out-parameter */
    int reject;
    int hire;
    int x;
    int y;
    int spawn;
    int hired;
    char caret_colors[20];

    for (scratch = 0; scratch < 20; scratch++)
        caret_colors[scratch] = scratch % 10 + 176;   /* palette 176 .. 185: the caret's colour ramp, twice */
    hired = 0;
    hire_name_entry = 0;
    hire_offer_kind = offer_kind;
    Income_Warn_Blink_Off();
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Invalidate();
    Map_Redraw();
    Screen_Save_A800();
    Set_Draw_Page_Front();
    Load_Unit_View_Graphics();
    Hire_Screen_Load(hero_type);
    UI_Set_Redraw_Fn(Hire_Screen_Redraw, 2);
    UI_Set_Press_Offset(1, 1);
    present_mode = PRESENT_NORMAL;
    x = 25;
    y = 17;
    strcpy(wizards[PLAYER_HUMAN].hired_heroes[slot].name, p_hire_hero_name);
    spawn = Hero_Spawn(PLAYER_HUMAN, hero_type, slot, 0);
    if (spawn == 0) {
        Load_Overland_Screen_Graphics();
        return 0;
    }
    hire_offer_cost = unit_types[hero_type].cost + p_units[num_units - 1].level * unit_types[hero_type].cost / 4;
    if (wizards[PLAYER_HUMAN].gold < hire_offer_cost && offer_kind == HIRE_OFFER_HIRE) {
        if (p_units[num_units - 1].level < 4) {
            p_units[num_units - 1].level = p_units[num_units - 1].level + 1;
            p_units[num_units - 1].experience = exp_level_thresholds[p_units[num_units - 1].level];
        }
        Unit_Kill_Or_Dismiss(num_units - 1, UNIT_REMOVE_DISMISSED);
        Load_Overland_Screen_Graphics();
        return 0;
    }
    if (wizards[PLAYER_HUMAN].retorts[RETORT_CHARISMATIC] > 0)
        hire_offer_cost = hire_offer_cost / 2;
    Battle_Unit_Init(num_units - 1, p_unit_stats_buf);
    UnitView_Build_Abilities(num_units - 1, p_unit_view_list, &unit_view_abil_count);
    UnitView_Init_Hire(num_units - 1, unit_view_abil_count);
    UI_Clear_Controls();
    Font_Select(4, 4, 0, 0);
    if (hire_offer_kind == HIRE_OFFER_HIRE)
        hire = UI_Add_Button(x + 221, y + 143, "Hire", ok_button_pic, "H", -1);
    else
        hire = UI_Add_Button(x + 221, y + 143, "Accept", ok_button_pic, "A", -1);
    reject = UI_Add_Button(x + 221, y + 162, "Reject", ok_button_pic, UI_NO_HOTKEY, -1);
    UnitView_Add_Ability_Controls(x + 8, y + 108, p_unit_view_list, unit_view_abil_count, unit_view_abil_first);
    Font_Select(0, 0, 2, 0);
    Set_Draw_Page();
    Hire_Screen_Redraw();
    UI_Draw_Controls();
    Palette_Upload();
    VGA_Present_Dissolve();
    LBX_Load_Records_Near("hlpentry", HLPENTRY_HIRE, help_entries, 0, 23, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 23);
    done = 0;
    UI_Input_Delay(2);
    while (done == 0) {
        UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
        Timer_Mark();
        input = UI_Get_Input();
        UnitView_Scroll_Abilities(input, &scratch);
        if (input == reject) {
            done = 1;
            Play_Click_Sound_Casting();
            if (p_units[num_units - 1].level < 4 && offer_kind == HIRE_OFFER_HIRE) {
                p_units[num_units - 1].level = p_units[num_units - 1].level + 1;
                p_units[num_units - 1].experience = exp_level_thresholds[p_units[num_units - 1].level];
            }
            Unit_Kill_Or_Dismiss(num_units - 1, UNIT_REMOVE_DISMISSED);
        }
        if (input == hire) {
            Play_Click_Sound();
            hire_name_entry = 1;
            p_hero_stats[PLAYER_HUMAN][hero_type].status = HERO_STATUS_UNAVAILABLE;
            if (hire_offer_kind == HIRE_OFFER_HIRE)
                wizards[PLAYER_HUMAN].gold -= hire_offer_cost;
            if (hire_offer_kind % 2 == 1) {               /* the summoned kinds: at the summoning circle */
                p_units[num_units - 1].x = wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_X];
                p_units[num_units - 1].y = wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_Y];
                p_units[num_units - 1].plane = wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_PLANE];
            }
            Stack_Limit_Enforce(num_units - 1);   /* at the fortress even for a prisoner, whom Lair_Enter then moves
                                                     to the lair (see there); clarified using ReMoM repo */
            hired = 1;
            done = 1;
        }
        UI_Set_Control_Count(reject);           /* drop the controls after Reject: the ability rows are re-added each pass */
        UnitView_Add_Ability_Controls(x + 8, y + 108, p_unit_view_list, unit_view_abil_count, unit_view_abil_first);
        if (done == 0) {
            Set_Draw_Page();
            Hire_Screen_Redraw();
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    if (hired == 1) {
        Font_Select(4, 4, 4, 4);
        UI_Text_Input_Box(x + 45, y + 75, 120, wizards[PLAYER_HUMAN].hired_heroes[slot].name, 13, 0, 0, 0, caret_colors, "",
                          -1);
        far_strcpy(p_hero_names[hero_type].name, wizards[PLAYER_HUMAN].hired_heroes[slot].name, 0);
    }
    UI_Clear_Controls();
    UI_Suspend_Redraw_Fn();
    Load_Overland_Screen_Graphics();
    Map_Full_Redraw();
    Income_Warn_Blink_On();
    UI_Clear_Help();
    return hired;
}

/* () "Mercenary for Hire: <cost> gold" / "<n> Mercenaries for Hire: <cost> gold" over the unit view */
void Mercenary_Screen_Redraw(void)
{
    char text[10];
    int x;
    int y;

    x = 25;
    y = 17;
    Screen_Restore_A800();
    Draw_Unit_View(x, y, UNIT_VIEW_OVERLAND, num_units - 1, p_unit_view_list, unit_view_abil_count, forge_icon_pic);
    Draw_Pic_Frame(x - 25, 0, hire_bg_pic);
    Set_Font_Color(0xa3);
    Font_Select(4, 4, 0, 0);
    if (hire_merc_count == 1) {
        strcpy(message_text, "Mercenary for Hire: ");
    } else {
        itoa(hire_merc_count, text, 10);
        strcpy(message_text, text);
        strcat(message_text, " Mercenaries for Hire: ");
    }
    itoa(hire_offer_cost, text, 10);
    strcat(message_text, text);
    strcat(message_text, " gold");
    Print_Text_Centered(x + 103, y - 11, message_text);
}

/* (unit_type, count, level, cost) -> 1 = hired: the mercenary is created at the human's fortress with the level and
   its experience, shown in the unit view, and removed again (num_units - 1) whatever the answer - the caller
   creates the real units */
int Screen_Hire_Mercenaries(int unit_type, int count, int level, int cost)
{
    int input;
    int reject;
    int hire;
    int x;
    int y;
    int hired;
    int done;

    x = 25;
    y = 17;
    Income_Warn_Blink_Off();
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Invalidate();
    Map_Redraw();
    Screen_Save_A800();
    Set_Draw_Page_Front();
    Load_Unit_View_Graphics();
    hire_merc_type = unit_type;
    hire_merc_count = count;
    hire_offer_cost = cost;
    UI_Set_Press_Offset(1, 1);
    Unit_Create(hire_merc_type, PLAYER_HUMAN, p_fortresses[PLAYER_HUMAN].x, p_fortresses[PLAYER_HUMAN].y, p_fortresses[PLAYER_HUMAN].plane, UNIT_SRC_NONE);
    p_units[num_units - 1].level = level;
    p_units[num_units - 1].experience = exp_level_thresholds[level];
    Hire_Screen_Load(unit_type);
    Battle_Unit_Init(num_units - 1, p_unit_stats_buf);
    UnitView_Build_Abilities(num_units - 1, p_unit_view_list, &unit_view_abil_count);
    UnitView_Init_Hire(num_units - 1, unit_view_abil_count);
    UI_Set_Redraw_Fn(Mercenary_Screen_Redraw, 2);
    UI_Clear_Controls();
    Font_Select(4, 4, 0, 0);
    hire = UI_Add_Button(x + 221, y + 143, "Hire", ok_button_pic, "", -1);
    reject = UI_Add_Button(x + 221, y + 162, "Reject", ok_button_pic, UI_NO_HOTKEY, -1);
    Font_Select(0, 0, 2, 0);
    present_mode = PRESENT_DISSOLVE;
    Set_Draw_Page();
    Mercenary_Screen_Redraw();
    UI_Draw_Controls();
    Screen_Present();
    present_mode = PRESENT_NORMAL;
    done = 0;
    UI_Input_Delay(2);
    LBX_Load_Records_Near("hlpentry", HLPENTRY_HIRE, help_entries, 0, 23, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 23);
    while (!done) {
        UnitView_Set_Ability_Help(p_unit_view_list, unit_view_abil_count, 0);
        input = UI_Get_Input();
        if (input == reject) {
            done = 1;
            Play_Click_Sound_Casting();
            num_units = num_units - 1;
            hired = 0;
        }
        if (input == hire) {
            Play_Click_Sound();
            done = 1;
            num_units = num_units - 1;
            hired = 1;
        }
        if (!done) {
            Set_Draw_Page();
            Mercenary_Screen_Redraw();
            Screen_Present();
        }
    }
    UI_Clear_Controls();
    Load_Overland_Screen_Graphics();
    Map_Full_Redraw();
    UI_Suspend_Redraw_Fn();
    Heap_Release(p_scratch);                    /* no Heap_Mark in this screen: it returns to whatever mark was set
                                                   last (no effect found); clarified using ReMoM repo */
    Near_Alloc_Release(p_scratch);
    Income_Warn_Blink_On();
    UI_Clear_Help();
    return hired;
}

/* (player, slot, unit_type) an AI wizard hires the hero when its price (Charismatic: half) is at most 90% of its gold;
   returns 0 when the hero could not be created (no return value otherwise)
   Numbered: BUG #179 (BUGS_AND_QUIRKS.txt) */
int AI_Hire_Hero(int player, int slot, int unit_type)
{
    int gold_limit;
    int spawn;
    int cost;

    cost = unit_types[unit_type].cost;
    if (wizards[player].retorts[RETORT_CHARISMATIC] > 0)
        cost = cost / 2;
    gold_limit = (long)wizards[player].gold * 90 / 100;
    if (gold_limit >= cost) {
        spawn = Hero_Spawn(player, unit_type, slot, 0);
        if (spawn == 0)
            return 0;
        Stack_Limit_Enforce(num_units - 1);
        wizards[player].gold -= cost;
        p_hero_stats[player][unit_type].status = HERO_STATUS_UNAVAILABLE;
    }
}
