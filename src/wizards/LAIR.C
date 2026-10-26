/* LAIR.C - WIZARDS.EXE overlay code segment 6d36 (stub 249f): entering a lair, node or tower - the prompt ("You have found a
   mysterious cave.  Scouts have spotted ... within the ...  Do you wish to enter?"), the fight with the guardians
   (Lair_Attack), the treasure roll (gold, mana, items, a spell, spell books or a retort, a prisoner hero), the
   "Inside you find ..." box, the hero hire / item award screens and clearing the site; also the treasure message
   box with its Yes / No buttons and the a / an article helper.
   This module's data (DS:4ab4): the lair type names and articles, then its literals; BSS (DS:c2fc..c308).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 6d36 src/wizards/LAIR.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Lair_At(int x, int y, int plane);
void Lair_Clear(int lair);
int Lair_Enter_At(int x, int y, int plane, int player);
int Lair_Enter(int lair, int player);
int Lair_Prompt(int lair);
void Treasure_Show(int lair, int y, int *items, int spell, int prisoner_slot, int total, int n_special, int *special);
void Treasure_Give_Gold_Mana(int lair, int player);
void Text_List_Separator(int *count, int total, char *text);
char *Text_Article_A_An(char *word);
void Treasure_Roll(int player, int lair, int *items, int *spell, int *prisoner_hero, int *prisoner_slot, int *total,
                   int *n_special, int *special);
int Screen_Treasure(int x, int y, char *text, unsigned pic, int pic_dx, int pic_dy, int yes_no, int color_cycle);
void Screen_Treasure_Redraw(void);
void Treasure_Roll_Book_Or_Retort(int player, int lair, int picks, int *n, int *out);
int Lair_Book_Realm(int lair);

#include "GAME.H"

char *lair_type_names[11] = { "tower", "chaos node", "nature node", "sorcery node", "mysterious cave", "dungeon",
    "ancient temple", "abandoned keep", "monster lair", "ruins", "fallen temple" }; /* DS:4ab4 */
char *lair_type_articles[11] = { "a", "a", "a", "a", "a", "a", "an", "an", "a", "some", "a" }; /* DS:4aca */

int award_at_location;                          /* DS:c306  1 while awarded items appear at award_loc_* (lair treasure here, battle loot in ATTACK.C) */
int award_loc_x;                                /* DS:c304 */
int award_loc_y;                                /* DS:c302 */
int award_loc_plane;                            /* DS:c300 */
int treasure_yes_no;                            /* DS:c2fe  1: the treasure box has Yes / No buttons */
char *p_treasure_text;                          /* DS:c2fc  500 bytes: the prompt / treasure text */

extern struct MouseRegion mouse_region_arrow;   /* DS:00ee */
extern char *retort_names[18];                  /* DS:0130  "Alchemy" .. "Artificer" */
extern char *arcane_first_realm_names[6];                    /* DS:1ed8  Arcane, Nature, Sorcery, Chaos, Life, Death */
extern char notify_shadow_colors[5];            /* DS:6f78 */
extern char confirm_text_colors[5];             /* DS:6f7d */
extern int confirm_box_dx;                      /* DS:6f96 */
extern int confirm_box_dy;                      /* DS:6f98 */
extern int notify_box_mode;                     /* DS:6f9a */
extern int combat_site_index;                         /* DS:9286 */
extern unsigned p_music_buf;                    /* DS:9390 */
extern unsigned lair_confirm_border_pic;        /* DS:991a */
extern unsigned lair_bottom_nobutton_pic;       /* DS:991c */
extern unsigned lair_bottom_button_pic;         /* DS:991e */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern unsigned confirm_left_button_pic;          /* DS:cacc */
extern unsigned confirm_right_button_pic;         /* DS:cace */
extern int notify_pic1_dy;                       /* DS:cad8 */
extern int notify_pic1_dx;                       /* DS:cada */
extern unsigned notify_pic1;                    /* DS:cae0 */
extern int notify_color_cycle;                  /* DS:cafe */
extern int notify_color_step;                   /* DS:cb00 */
extern char *notify_text;                       /* DS:cb4e */
extern int notify_box_y;                        /* DS:cb50 */
extern int notify_box_x;                        /* DS:cb52 */

void UI_Clear_Controls(void);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Sound_Play(unsigned sound);
void Bit_Set(int bit, char far *map);
void Music_Play_Current(void);
int Lair_Attack(int lair, int player);
void Minimap_Alloc_Pic(void);
void Remap_Build_Shadow(void);
void Map_Redraw(void);
void Screen_Copy_Front(void);
void Stack_Blink_Reset(void);
int Screen_Hire_Hero(int slot, int hero_type, int offer_kind);
void Items_Screen_Award(int count, int *items);
void Wizard_Learn_Spell(int player, int spell, int show);
void Research_Recheck_Known(int fade);
void Mouse_Set_Cursor_Regions(int count, struct MouseRegion *regions);
char *Near_Alloc_Reset(unsigned bytes);
char *Near_Alloc(unsigned bytes);
void Scratch_Save_To_EMS(void);
void Scratch_Restore_From_EMS(void);
void far_strcpy(char *dst, int dst_seg, char far *src);
char *Item_Type_Name(int item);
void Notify_Picture_Box_Centered(int center_x, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                                 int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
void Wizard_Add_Gold(int player, int amount);
void Wizard_Add_Mana(int player, int amount);
int Item_Create_Prefab(int prefab_kind, int *spellbooks, int max_cost);
int Treasure_Pick_Spell(int player, int rarity);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
int Hero_Pick_Random(int player, int mode, int pool);
int Wizard_Free_Hero_Slot(int player);
void Font_Save_Colors(void);
void Font_Restore_Colors(void);
void Font_Select_Palette(int font, char *colors);
void Pic_Apply_Palette(unsigned pic);
void Font_Select();                             /* no prototype here: this module calls it with 3 arguments (FONT.ASM takes 4) */
void Paragraph_Add_Exclusion(int left, int top, int right, int bottom);
int Paragraph_Height(int width, char *text);
void Paragraph_Clear_Exclusions(void);
int UI_Add_Button(int x, int y, char *label, int pic, char *hotkey, int help);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
int UI_Get_Input(void);
int abs(int x);
void UI_Suspend_Redraw_Fn(void);
void Reset_Clip_Rect(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Line_Spacing(int line_gap);
void Paragraph_Print(int x, int y, int width, char *text, unsigned align);
void Set_Font_Color(int color);
void Palette_Rotate_Upload(int first, unsigned count, unsigned step);
void Wizard_Gain_Spellbook(int player, int realm);

/* (x, y, plane) -> the active lair on the square, or -1 */
int Lair_At(int x, int y, int plane)
{
    int found;
    int lair;

    found = -1;
    for (lair = 0; lair < NUM_LAIRS && found == -1; lair++)
        if (p_lairs[lair].plane == plane && p_lairs[lair].active == 1 && p_lairs[lair].y == y && p_lairs[lair].x == x)
            found = lair;
    return found;
}

/* (lair) deactivates it; a tower also deactivates the active tower record at the same x, y (its other plane) */
void Lair_Clear(int lair)
{
    int found;
    int i;

    p_lairs[lair].active = 0;
    if (p_lairs[lair].type == LAIR_TOWER) {
        found = -1;
        for (i = 0; i < NUM_LAIRS && found == -1; i++) {
            if (p_lairs[i].active == 1 && p_lairs[i].y == p_lairs[lair].y && p_lairs[i].x == p_lairs[lair].x) {
                p_lairs[i].active = 0;
                found = 0;
            }
        }
    }
}

/* (x, y, plane, player) -> 1 when no lair is there, else Lair_Enter's result; never called */
int Lair_Enter_At(int x, int y, int plane, int player)
{
    int lair;
    int result;

    lair = Lair_At(x, y, plane);
    result = 1;
    if (lair != -1)
        result = Lair_Enter(lair, player);
    return result;
}

/* (lair, player) -> 99 when the current player declines, else the battle result (1 won / no guardians): the
   current player (player == current_player, not a human test) gets the prompt (event music), guardians are fought (Lair_Attack); a win rolls and shows the treasure, gives gold and
   mana, offers a prisoner hero and awards items (current player only), teaches the spell and clears the lair
   Numbered: BUG #75 (BUGS_AND_QUIRKS.txt) */
int Lair_Enter(int lair, int player)
{
    int enter;                                  /* after Lair_Attack: the battle result this function returns */
    int y;
    int items[3];
    int spell;
    int prisoner_slot;
    int prisoner_hero;
    int total;
    int saved_screen;
    int sound;
    int n_special;
    int special[2];

    UI_Clear_Controls();
    notify_box_mode = 0;
    enter = 1;
    if (player == current_player) {
        if (game_config.event_music == 1) {
            sound = LBX_Reload("MUSIC", MUSIC_SITE_DISCOVERY, p_music_buf);
            Sound_Play(sound);
        }
        enter = Lair_Prompt(lair);
        Bit_Set(LAIR_BIT_VISITED, &p_lairs[lair].flags);
        Music_Play_Current();
        if (enter == 0)
            return LAIR_ENTER_DECLINED;
    }
    y = 100;
    notify_box_mode = 0;
    combat_loc_x = p_lairs[combat_site_index].x;   /* combat_site_index, not the lair parameter: the live caller (MOVESTK.C)
                                                   passes combat_site_index; Lair_Enter_At (never called) does not */
    combat_loc_y = p_lairs[combat_site_index].y;
    combat_loc_plane = p_lairs[combat_site_index].plane;
    if (enter == 1 && (p_lairs[lair].guard1_count & LAIR_GUARDS_LEFT_MASK) > 0) {
        notify_box_mode = 0;
        enter = Lair_Attack(lair, player);
        if (player == current_player)
            Bit_Set(LAIR_BIT_FOUGHT, &p_lairs[lair].flags);
    }
    if (enter == 1 && player == current_player) {
        y = 60;
        UI_Clear_Controls();
        Minimap_Alloc_Pic();
        Palette_Unmark_Range(0, 255);
        Palette_Mark_Range(0, 223);
        Remap_Build_Shadow();
        Set_Draw_Page();
        Map_Redraw();
        Screen_Present();
        Screen_Copy_Front();
    }
    if (enter == 1) {
        n_special = 0;
        Treasure_Roll(player, lair, items, &spell, &prisoner_hero, &prisoner_slot, &total, &n_special, special);
        if (player == current_player)
            Treasure_Show(lair, y, items, spell, prisoner_slot, total, n_special, special);
        Treasure_Give_Gold_Mana(lair, player);
        notify_box_mode = 0;
        if (prisoner_slot != -1 && player == current_player) {
            Stack_Blink_Off();
            /* BUG #466: the hero is created at the fortress and Screen_Hire_Hero's Stack_Limit_Enforce runs there, so a
               fortress holding 9 units has a unit pushed out (Unit_Bump_Or_Report) before the hero moves here - when
               one is strictly cheaper than the hero (ties bump the hero itself); a 9-unit stack here cannot get the
               prisoner (Treasure_Roll needs fewer than 9). clarified using ReMoM repo; checked in game (regress.py) */
            if (Screen_Hire_Hero(prisoner_slot, prisoner_hero, HIRE_OFFER_PRISONER) == 1) {
                p_units[num_units - 1].x = combat_loc_x;
                p_units[num_units - 1].y = combat_loc_y;
                p_units[num_units - 1].plane = combat_loc_plane;
            }
            Stack_Blink_Reset();
        }
        if (p_lairs[lair].item_count > 0 && player == current_player) {
            saved_screen = current_screen;
            UI_Clear_Controls();
            Minimap_Alloc_Pic();
            Palette_Unmark_Range(0, 255);
            Palette_Mark_Range(0, 223);
            Remap_Build_Shadow();
            Set_Draw_Page();
            Map_Redraw();
            Screen_Present();
            Screen_Copy_Front();
            award_at_location = 1;
            award_loc_x = p_lairs[lair].x;
            award_loc_y = p_lairs[lair].y;
            award_loc_plane = p_lairs[lair].plane;
            Items_Screen_Award(p_lairs[lair].item_count, items);
            award_at_location = 0;
            current_screen = saved_screen;
        }
        if (spell > -1)
            Wizard_Learn_Spell(player, spell, 1);
        Lair_Clear(lair);
    }
    confirm_box_dx = 0;
    confirm_box_dy = 0;
    notify_box_mode = 0;
    if (player == current_player)
        Research_Recheck_Known(0);
    return enter;
}

/* (lair) -> 1 enter / 0 not: "You have found <a> <site>." plus the guardians and the question when it is
   guarded (a site without guardians is entered without asking); the site's RELOAD picture, color cycling for the
   three nodes */
int Lair_Prompt(int lair)
{
    int pic;
    char *spare;                                /* 100 bytes, never used */
    int yes_no;
    int color_cycle;
    int answer;
    register int lair_type;

    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    lair_type = p_lairs[lair].type;
    p_treasure_text = Near_Alloc_Reset(500);
    spare = Near_Alloc(100);
    strcpy(p_treasure_text, "You have found ");
    strcat(p_treasure_text, lair_type_articles[lair_type]);
    strcat(p_treasure_text, " ");
    strcat(p_treasure_text, lair_type_names[lair_type]);
    strcat(p_treasure_text, ".");
    yes_no = 0;
    if ((p_lairs[lair].guard1_count & LAIR_GUARDS_LEFT_MASK) > 0) {
        yes_no = 1;
        strcat(p_treasure_text, "  Scouts have spotted ");
        if (unit_types[p_lairs[lair].guard1_type].figures == 1) {
            strcat(p_treasure_text, Text_Article_A_An(unit_types[p_lairs[lair].guard1_type].name));
            strcat(p_treasure_text, " ");
        }
        strcat(p_treasure_text, unit_types[p_lairs[lair].guard1_type].name);
        strcat(p_treasure_text, " within the ");
        strcat(p_treasure_text, lair_type_names[lair_type]);
        strcat(p_treasure_text, ".  Do you wish to enter?");
    }
    color_cycle = 0;
    if (lair_type > LAIR_TOWER && lair_type < LAIR_MYSTERIOUS_CAVE)   /* the three nodes */
        color_cycle = 1;
    Scratch_Save_To_EMS();
    pic = LBX_Reload("RELOAD", lair_type + 9, p_scratch);
    confirm_right_button_pic = LBX_Reload_Next("RESOURCE", 3, p_scratch);
    confirm_left_button_pic = LBX_Reload_Next("RESOURCE", 4, p_scratch);
    answer = Screen_Treasure(160, 60, p_treasure_text, pic, 0, 0, yes_no, color_cycle);
    Scratch_Restore_From_EMS();
    if (yes_no == 0)
        answer = 1;
    return answer;
}

/* (lair, y, items[], spell, prisoner_slot, total, n_special, special[]) the "Inside you find ..." box: gold and
   mana (when above 1), a prisoner, a spell, spell books (two of a realm counted as "2 ... spell books") or a
   retort, and the items (identical names counted); total counts the entries for the "and"
   Numbered: BUG #249 (BUGS_AND_QUIRKS.txt) */
void Treasure_Show(int lair, int y, int *items, int spell, int prisoner_slot, int total, int n_special, int *special)
{
    char *texts[4];                             /* [0..2] item type names, [3] the number scratch */
    int counts[3];
    unsigned pic;
    int item_count;
    int sep_state;
    char word[30];
    register int i;

    p_treasure_text = Near_Alloc_Reset(500);
    texts[3] = Near_Alloc(100);
    for (i = 0; i < 3; i++)
        texts[i] = Near_Alloc(20);
    pic = LBX_Reload("RELOAD", 20, p_scratch);
    strcpy(p_treasure_text, "Inside you find ");
    if (total < 1)
        strcat(p_treasure_text, "absolutely nothing");
    sep_state = 0;
    if (p_lairs[lair].gold > 1) {
        Text_List_Separator(&sep_state, total, p_treasure_text);
        itoa(p_lairs[lair].gold, texts[3], 10);
        strcat(p_treasure_text, texts[3]);
        strcat(p_treasure_text, " gold");
    }
    if (p_lairs[lair].mana > 1) {
        Text_List_Separator(&sep_state, total, p_treasure_text);
        itoa(p_lairs[lair].mana, texts[3], 10);
        strcat(p_treasure_text, texts[3]);
        strcat(p_treasure_text, " mana crystals");
    }
    if (prisoner_slot != -1) {
        Text_List_Separator(&sep_state, total, p_treasure_text);
        strcat(p_treasure_text, "a prisoner");
    }
    if (spell > -1) {
        Text_List_Separator(&sep_state, total, p_treasure_text);
        far_strcpy(word, 0, p_spell_data[spell].name);
        strcat(p_treasure_text, Text_Article_A_An(word));
        strcat(p_treasure_text, " ");
        strcat(p_treasure_text, word);
        strcat(p_treasure_text, " spell");
    }
    if (n_special > 0) {
        counts[0] = 1;
        counts[1] = 1;
        if (n_special > 1 && special[0] == special[1]) {
            counts[0] = 2;
            counts[1] = -1;
            n_special--;
            total--;
        }
        for (i = 0; i < n_special; i++) {
            Text_List_Separator(&sep_state, total, p_treasure_text);
            if (special[i] < TREASURE_RETORT_BASE) {
                itoa(counts[i], texts[3], 10);
                strcat(p_treasure_text, texts[3]);
                strcat(p_treasure_text, " ");
                strcat(p_treasure_text, arcane_first_realm_names[special[i] + 1]);   /* + 1: a Realm into the Arcane-first table */
                strcat(p_treasure_text, " spell book");
                if (counts[i] > 1)
                    strcat(p_treasure_text, "s");
            } else {
                strcat(p_treasure_text, "a retort of ");
                strcat(p_treasure_text, retort_names[special[i] - TREASURE_RETORT_BASE]);
            }
        }
    }
    item_count = p_lairs[lair].item_count;
    if (item_count > 0) {
        counts[0] = -1;
        counts[1] = -1;
        counts[2] = -1;
        for (i = 0; i < item_count; i++) {
            counts[i] = 1;
            strcpy(texts[i], Item_Type_Name(items[i]));
        }
        if (counts[1] != -1 && stricmp(texts[0], texts[1]) == 0) {
            counts[0]++;
            counts[1] = -1;
        }
        if (counts[2] != -1 && stricmp(texts[0], texts[2]) == 0) {
            counts[0]++;
            counts[2] = -1;
        }
        if (counts[1] != -1 && counts[2] != -1 && stricmp(texts[1], texts[2]) == 0) {
            counts[1]++;
            counts[2] = -1;
        }
        for (i = 0; i < item_count; i++)
            if (counts[i] == -1)
                total--;
        for (i = 0; i < item_count; i++) {
            if (counts[i] != -1) {
                Text_List_Separator(&sep_state, total, p_treasure_text);
                if (counts[i] > 1) {
                    itoa(counts[i], texts[3], 10);
                    strcat(p_treasure_text, texts[3]);
                    strcat(p_treasure_text, " ");
                    strcat(p_treasure_text, texts[i]);
                    strcat(p_treasure_text, "s");
                } else {
                    strcpy(texts[3], texts[i]);
                    strcat(p_treasure_text, Text_Article_A_An(texts[3]));
                    strcat(p_treasure_text, " ");
                    strcat(p_treasure_text, texts[3]);
                }
            }
        }
    }
    strcat(p_treasure_text, ".");
    Notify_Picture_Box_Centered(190, y, 3, p_treasure_text, 0, pic, 2, 8, 0, 0, 0, 2, 0);
}

/* (lair, player) gives the lair's gold and mana */
void Treasure_Give_Gold_Mana(int lair, int player)
{
    Wizard_Add_Gold(player, p_lairs[lair].gold);
    Wizard_Add_Mana(player, p_lairs[lair].mana);
}

/* (&count, total, text) counts the entry and appends ", " before it, or " and " before the last one */
void Text_List_Separator(int *count, int total, char *text)
{
    *count = *count + 1;
    if (*count == total) {
        if (*count > 1)
            strcat(text, " and ");
    } else if (*count > 1)
        strcat(text, ", ");
}

/* (word) -> "an" before a vowel, else "a" */
char *Text_Article_A_An(char *word)
{
    char *article[3];                           /* [2] never set */

    article[0] = "an";
    article[1] = "a";
    if (*word == 'A' || *word == 'a' || *word == 'E' || *word == 'e' || *word == 'I' || *word == 'i' || *word == 'O'
        || *word == 'o' || *word == 'U' || *word == 'u')
        return article[0];
    else
        return article[1];
}

/* (player, lair, items[], &spell, &prisoner_hero, &prisoner_slot, &total, &n_special, special[]) the treasure:
   one prefab item per lair item value (BUG #74: the value is passed as the kind, with no cost cap), the special 1-4 a
   spell of that rarity, 5 / 6 one / two books or retorts (none: an extra item up to 1200 while under 3, else
   +1000 mana), a prisoner (flag bit 0) when fewer than 9 units stand there and a hero slot is free; total = gold, mana
   (as rolled, before a +1000), the special, the prisoner, the items and the books / retorts - a book / retort
   special counts twice (as the special and per book), also when it became an item
   Numbered: BUG #74 (BUGS_AND_QUIRKS.txt) */
void Treasure_Roll(int player, int lair, int *items, int *spell, int *prisoner_hero, int *prisoner_slot, int *total,
                   int *n_special, int *special)
{
    int count;
    int i;
    int gold;
    int mana;
    int lair_special;
    int prisoner;
    int item_count;
    int tile_count;
    int tile_units[MAX_STACK];

    gold = p_lairs[lair].gold;
    mana = p_lairs[lair].mana;
    lair_special = p_lairs[lair].special;
    prisoner = Bit_Test(LAIR_BIT_PRISONER, &p_lairs[lair].flags);
    item_count = p_lairs[lair].item_count;
    for (i = 0; i < item_count; i++)
        items[i] = Item_Create_Prefab(p_lairs[lair].item_values[i], wizards[player].spellbooks, 0);
    *n_special = 0;
    *spell = -1;
    if (lair_special > 0) {
        switch (lair_special) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            *spell = Treasure_Pick_Spell(player, lair_special);
            if (*spell < 1) {
                lair_special = 0;
                *spell = -1;
            }
            break;
        case LAIR_SPECIAL_BOOK:
            Treasure_Roll_Book_Or_Retort(player, lair, 1, n_special, special);
            if (*n_special < 1) {
                if (p_lairs[lair].item_count < 3) {
                    p_lairs[lair].item_count = p_lairs[lair].item_count + 1;
                    item_count = p_lairs[lair].item_count;
                    items[item_count - 1] = Item_Create_Prefab(PREFAB_ANY, wizards[player].spellbooks, 1200);
                } else
                    p_lairs[lair].mana += 1000;
            }
            break;
        case LAIR_SPECIAL_TWO_BOOKS:
            Treasure_Roll_Book_Or_Retort(player, lair, 2, n_special, special);
            if (*n_special < 1) {
                if (p_lairs[lair].item_count < 3) {
                    p_lairs[lair].item_count = p_lairs[lair].item_count + 1;
                    item_count = p_lairs[lair].item_count;
                    items[item_count - 1] = Item_Create_Prefab(PREFAB_ANY, wizards[player].spellbooks, 1200);
                } else
                    p_lairs[lair].mana += 1000;
            }
            break;
        }
    }
    *prisoner_hero = -1;
    *prisoner_slot = -1;
    if (prisoner > 0) {
        Tile_Units_Of_Owner(combat_loc_x, combat_loc_y, combat_loc_plane, player, &tile_count, tile_units);
        if (tile_count >= MAX_STACK) {
            prisoner = 0;
            *prisoner_slot = -1;
        } else {
            *prisoner_hero = Hero_Pick_Random(player, HPICK_SUMMONED, prisoner);
            if (*prisoner_hero > -1) {
                *prisoner_slot = Wizard_Free_Hero_Slot(player);
                if (*prisoner_slot == -1)
                    prisoner = 0;
            } else
                prisoner = 0;
        }
    }
    count = 0;
    if (gold > 0)
        count++;
    if (mana > 0)
        count++;
    if (lair_special > 0)
        count++;
    if (prisoner > 0)
        count++;
    count += item_count;
    count = count + *n_special;
    *total = count;
}

/* (x, y, text, pic, pic_dx, pic_dy, yes_no, color_cycle) -> 1 Yes / 0 No: the lair / treasure message box (its
   text beside the picture), with Yes / No buttons when yes_no is 1; Esc, No or a click outside the box answers 0 */
int Screen_Treasure(int x, int y, char *text, unsigned pic, int pic_dx, int pic_dy, int yes_no, int color_cycle)
{
    int height;
    int box_h;
    int done;
    int yes_button;
    int no_button;
    int outside;
    int box_ctl;                                /* the box itself (clicks on it do nothing) */
    int result;
    register int bottom;
    register int input;

    Font_Save_Colors();
    Font_Select_Palette(0, confirm_text_colors);
    Screen_Copy_Front();
    Pic_Apply_Palette(pic);
    Palette_Upload();
    notify_text = text;
    treasure_yes_no = yes_no;
    notify_pic1 = pic;
    notify_pic1_dx = pic_dx;
    notify_pic1_dy = pic_dy;
    notify_color_cycle = color_cycle;
    Font_Select(4, 4, 15);
    Paragraph_Add_Exclusion(0, 0, 57, 45);
    height = Paragraph_Height(166, notify_text);
    box_h = height + 34;
    notify_box_x = 68;
    notify_box_y = (200 - box_h) / 2;
    bottom = height;
    if (bottom < 70)
        bottom = 75;
    bottom += notify_box_y + 6;
    UI_Clear_Controls();
    yes_button = UI_NO_CONTROL;
    no_button = UI_NO_CONTROL;
    box_ctl = UI_NO_CONTROL;
    if (treasure_yes_no == 1) {
        yes_button = UI_Add_Button(notify_box_x + 100, bottom + 5, "", confirm_right_button_pic, "Y", -1);
        no_button = UI_Add_Button(notify_box_x + 17, bottom + 5, "", confirm_left_button_pic, "N", -1);
        box_ctl = UI_Add_Region(notify_box_x, notify_box_y, notify_box_x + 184, bottom + 28, "", -1);
    }
    outside = UI_Add_Region(0, 0, 319, 199, "\x1b", -1);
    UI_Set_Redraw_Fn(Screen_Treasure_Redraw, 1);
    result = 1;
    done = 0;
    while (done == 0) {
        input = abs(UI_Get_Input());            /* never -1: that test below is dead */
        if (input == -1 || input == no_button || input == outside) {
            done = 1;
            result = 0;
        }
        if (input == yes_button) {
            done = 1;
            result = 1;
        }
        Screen_Treasure_Redraw();
        Screen_Present();
    }
    UI_Suspend_Redraw_Fn();
    UI_Clear_Controls();
    Font_Restore_Colors();
    Reset_Clip_Rect();
    Paragraph_Clear_Exclusions();
    return result;
}

/* () the treasure box's redraw: border clipped to the text's height, the picture, the bottom (with or without the
   button bar), the text in two shadow passes and the face color; the nodes' colors rotate */
void Screen_Treasure_Redraw(void)
{
    int height;
    register int bottom;

    Font_Select(4, 4, 15);
    Paragraph_Add_Exclusion(0, 0, 57, 45);
    height = Paragraph_Height(166, notify_text);
    Paragraph_Clear_Exclusions();
    Set_Draw_Page();
    bottom = height;
    if (bottom < 70)
        bottom = 75;
    bottom += notify_box_y + 6;
    Set_Clip_Rect(0, 0, 319, bottom);
    Draw_Pic_Clipped(notify_box_x, notify_box_y, lair_confirm_border_pic);
    Reset_Clip_Rect();
    Draw_Pic_Frame(notify_box_x + 7, notify_box_y + 7, notify_pic1);
    if (treasure_yes_no == 0)
        Draw_Pic_Frame(notify_box_x, bottom, lair_bottom_nobutton_pic);
    else
        Draw_Pic_Frame(notify_box_x, bottom, lair_bottom_button_pic);
    Font_Select_Palette(4, notify_shadow_colors);
    Font_Select(4, 15, 15);
    Set_Line_Spacing(2);
    Paragraph_Add_Exclusion(notify_box_x, notify_box_y, notify_box_x + 58, notify_box_y + 55);
    Paragraph_Print(notify_box_x + 10, notify_box_y + 11, 166, notify_text, PARA_LEFT);
    Paragraph_Add_Exclusion(notify_box_x, notify_box_y, notify_box_x + 57, notify_box_y + 55);
    Paragraph_Print(notify_box_x + 9, notify_box_y + 11, 166, notify_text, PARA_LEFT);
    Set_Font_Color(18);
    Font_Select_Palette(4, confirm_text_colors);
    Font_Select(4, 4, 15);
    Set_Line_Spacing(2);
    Paragraph_Add_Exclusion(notify_box_x, notify_box_y, notify_box_x + 57, notify_box_y + 55);
    Paragraph_Print(notify_box_x + 9, notify_box_y + 10, 166, notify_text, PARA_LEFT);
    if (notify_color_cycle == 1) {
        Palette_Rotate_Upload(247, 8, notify_color_step);
        notify_color_step = (notify_color_step + 1) % 8;
    }
}

/* (player, lair, picks, &n, out[]) per pick: 3 in 4 a spell book of the lair's realm (Lair_Book_Realm; not Life
   with Death books or the reverse), else a random unowned retort (100 + its index; never Myrran) - Warlord,
   Channeler, Divine Power, Famous and Infernal Power only as the first of two picks, which then stops; with more
   than 5 retorts always a book, more than 12 books always a retort, both: nothing. BUG #76: the opposite-realm and
   Famous (+10 fame) checks of the second branch are dead
   Numbered: BUG #76 (BUGS_AND_QUIRKS.txt) */
void Treasure_Roll_Book_Or_Retort(int player, int lair, int picks, int *n, int *out)
{
    int j;
    int pick;
    int k;
    char *retorts;
    int retort_count;
    int roll;
    int books;
    int special_retorts[5];
    int failed;
    int ok;
    int tries;
    int realm;

    special_retorts[0] = RETORT_WARLORD;
    special_retorts[1] = RETORT_CHANNELER;
    special_retorts[2] = RETORT_DIVINE_POWER;
    special_retorts[3] = RETORT_FAMOUS;
    special_retorts[4] = RETORT_INFERNAL_POWER;
    *n = 0;
    failed = 0;
    for (k = 0; k < picks && failed == 0; k++) {
        retort_count = 0;
        retorts = wizards[player].retorts;
        for (j = 0; j < 18; j++)
            if (retorts[j] == 1)
                retort_count++;
        roll = Random(100);
        books = 0;
        for (j = 0; j < 5; j++)
            books += wizards[player].spellbooks[j];
        if (retort_count > 5 && books > 12)
            failed = -1;
        if (failed == 0) {
            if (retort_count > 5)
                roll = 0;
            if (books > 12)
                roll = 100;
            if (roll < 75) {
                realm = Lair_Book_Realm(lair);
                ok = 1;
                if (realm == REALM_LIFE && wizards[player].spellbooks[REALM_DEATH] > 0)
                    ok = 0;
                if (realm == REALM_DEATH && wizards[player].spellbooks[REALM_LIFE] > 0)
                    ok = 0;
                if (ok == 1) {
                    Wizard_Gain_Spellbook(player, realm);
                    out[*n] = realm;
                    *n = *n + 1;
                }
            } else {
                for (tries = 0; tries < 100; tries++) {
                    pick = Random(18) - 1;
                    if (retorts[pick] == 0) {
                        ok = 0;
                        for (j = 0; j < 5 && ok == 0; j++)
                            if (special_retorts[j] == pick)
                                ok = 1;
                        if (pick == RETORT_MYRRAN)
                            ok = 0;
                        if (ok == 1) {
                            if (k == 0 && picks == 2) {
                                retorts[pick] = 1;
                                tries = 1000;
                                out[*n] = pick + TREASURE_RETORT_BASE;
                                *n = 1;
                                failed = -1;
                            }
                        } else {
                            ok = 1;
                            if (pick == RETORT_MYRRAN)
                                ok = 0;
                            if (pick == RETORT_INFERNAL_POWER && wizards[player].spellbooks[REALM_LIFE] > 0)
                                ok = 0;
                            if (pick == RETORT_DIVINE_POWER && wizards[player].spellbooks[REALM_DEATH] > 0)
                                ok = 0;
                            if (ok == 1) {
                                retorts[pick] = 1;
                                out[*n] = pick + TREASURE_RETORT_BASE;
                                *n = *n + 1;
                                if (pick == RETORT_FAMOUS)
                                    wizards[player].fame += 10;
                                tries = 1000;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* (lair) -> the realm of a treasure spell book: chaos node Chaos (2), nature node Nature (0), sorcery node Sorcery
   (1), dungeon / abandoned keep / ruins Death (4), ancient / fallen temple Life (3); tower, cave, monster lair a
   random 0..4 (Nature .. Death) */
int Lair_Book_Realm(int lair)
{
    int realm;

    switch (p_lairs[lair].type) {
    case LAIR_CHAOS_NODE:
        realm = REALM_CHAOS;
        break;
    case LAIR_NATURE_NODE:
        realm = REALM_NATURE;
        break;
    case LAIR_SORCERY_NODE:
        realm = REALM_SORCERY;
        break;
    case LAIR_ANCIENT_TEMPLE:
    case LAIR_FALLEN_TEMPLE:
        realm = REALM_LIFE;
        break;
    case LAIR_DUNGEON:
    case LAIR_ABANDONED_KEEP:
    case LAIR_RUINS:
        realm = REALM_DEATH;
        break;
    default:
        realm = Random(5) - 1;
        break;
    }
    return realm;
}
