/* SPELLCST.C - WIZARDS.EXE overlay code segment b8ad (stub 25c9): casting an overland spell - the spellbook screen (main-loop
   screen 12: pick a spell, abort the one being cast), the casting start (cost, crafting spells through the Item
   Forge, Spell of Mastery, variable power, casting at once when this turn's skill and mana cover it), the
   warnings before casting (Planar Seal, no hero slot, nothing to disjoin, no contacted wizard, global already
   active, Holy Arms), the "spell has caused ... to fizzle" box, the realm music, the spell's effect animation and
   sound and the animation played on a map square; also two map redraw helpers.
   No <string.h> here (the string functions are implicit declarations); this module's data (DS:6994): its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS b8ad src/wizards/SPELLCST.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Spellbook_Cast_Init(void);
void Spellbook_Cast_Redraw(void);
void Screen_Spellbook_Cast(void);
int Wizard_Begin_Casting(int player, int spell, int page_slot);
void Spell_Cast_Warnings(int spell, int *confirm);
void Map_Full_Redraw(void);
void Map_Redraw_Dissolve(void);
int Spell_Fizzle_Message_Overland(int victim, int blocker, int spell, char *blocker_spell);
void Music_Load_Realm(int spell, int base);
int Spell_Load_Anim(int spell);
void Spell_Load_Sound(int spell);
void Map_Spell_Effect_Anim(int x, int y, int plane);

#include "GAME.H"

extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern unsigned music_jingle;                   /* DS:9134 */
extern struct Item far *p_items;                /* DS:9136 */
extern unsigned p_music_buf;                    /* DS:9390 */
extern int overland_spellbook_page;             /* DS:944a */
extern unsigned spellbook_small_pic;            /* DS:946e */
extern struct Fortress far *p_fortresses;       /* DS:9cc8 */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern char message_text[];                     /* DS:c7fc */
extern struct SpellbookPage *p_spellbook_pages; /* DS:c8fa */
extern int spellbook_page_count;                /* DS:c914 */
extern int spellbook_mode;                 /* DS:c922 */
extern int spellbook_page;                      /* DS:c924 */
extern int ai_replan;                           /* DS:c9f0 */

void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
void Fill_Box_Styled();                           /* no prototype here: called with 5 arguments (GRAPHICS.C takes 9, read by style) */
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Set_Draw_Page_Front(void);
void Spellbook_Overland_Redraw(int x, int y);
void Income_Warn_Blink_Off(void);
void Income_Warn_Blink_On(void);
void Pic_Apply_Palette(unsigned pic);
void Remap_Build_Shadow(void);
void Palette_Cycle_Reset(void);
void Spellbook_Build(int mode, int per_page);
int abs(int x);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void UI_Suspend_Redraw_Fn(void);
void Set_Text_Shadow_Color(int color);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
void UI_Clear_Controls(void);
void UI_Clear_Help(void);
void UI_Set_Help(struct HelpEntry *help, int count);
int UI_Get_Input(void);
void Play_Click_Sound(void);
void Play_Click_Sound_Casting(void);
void Spellbook_Page_Turn_Anim(int forward, int is_combat, int caster);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Confirm_Box(char *text);
void Message_Box(char *text);
int Spell_Casting_Cost(int player, int spell, int in_combat);
void Overland_Extra_Power_Dialog(int spell, int slot);
int Screen_Item_Forge(int player, int artifact);
int Item_Create_Prefab(int prefab_kind, int *spellbooks, int max_cost);
int Mem_Copy_Far();                               /* unprototyped (see HELPSCR.C) */
void Item_Free(int item);
void Screen_Mastery_Started(int player);
void Global_Enchant_Relations(int caster, int spell, int divisor);
int Overland_Cast_Spell(int player);
int Wizard_Free_Hero_Slot(int player);
void VGA_Present_Dissolve(void);
void Screen_Copy_Front(void);
int Sound_Stop(void);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
unsigned LBX_Reload(char *name, int entry, unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Sound_Play(unsigned sound);
void Music_Play_Current(void);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
int Pic_Frame_Count(unsigned pic);
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
void Timer_Mark(void);
int Draw_Pic_Clipped(int x, int y, unsigned pic);

/* () the overland spellbook's background: the map darkened behind the small book, copied to the back page; the
   book's help (HLPENTRY 0, 15 entries) */
void Spellbook_Cast_Init(void)
{
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Invalidate();
    Map_Redraw();
    Fill_Box_Styled(0, 0, 319, 199, FILL_SHADOW);
    Draw_Pic_Frame(16, 10, spellbook_small_pic);
    Screen_Save_A800();
    Set_Draw_Page_Front();
    LBX_Load_Records_Near("hlpentry", HLPENTRY_SPELLBOOK, help_entries, 0, 15, sizeof(struct HelpEntry));
}

/* () the redraw callback: the back page and the open book at (16, 10) */
void Spellbook_Cast_Redraw(void)
{
    Screen_Restore_A800();
    Spellbook_Overland_Redraw(16, 10);
}

/* () main-loop screen 12: the spellbook opened at the spell being cast (else where it was left); a click on a
   spell asks to abort the one in progress (clicking that one aborts it, refunding nothing), runs the warnings and
   starts casting (Wizard_Begin_Casting); F / B turn the pages, Esc closes. BUG #477: the right page's help test reads
   the left page's spell; the first Cancel control gets -1 as its hotkey string */
void Screen_Spellbook_Cast(void)
{
    int done;
    int input;
    int k;
    int found;
    int cast;
    int y;
    int cancel_control;
    int answer;
    int spell;
    int page_slot;
    int next_control;
    int prev_control;
    int right_controls[6];
    int left_controls[6];                       /* [6..11] run on into right_controls */
    char spell_name[20];
    register int i;
    register int x;

    cast = 0;
    Income_Warn_Blink_Off();
    Pic_Apply_Palette(spellbook_small_pic);
    Palette_Mark_Range(0, 223);
    Palette_Unmark_Range(224, 255);
    Remap_Build_Shadow();
    Palette_Cycle_Reset();
    Spellbook_Cast_Init();
    Spellbook_Build(SPELLBOOK_OVERLAND, BOOK_PAGE_SPELLS);
    spellbook_mode = 1;
    found = 0;
    for (i = 0; i < spellbook_page_count && found == 0; i++) {
        for (k = 0; k < p_spellbook_pages[i].count && found == 0; k++) {
            if (abs(p_spellbook_pages[i].spells[k]) == wizards[PLAYER_HUMAN].casting_spell) {
                if (i % 2 == 1)
                    spellbook_page = i - 1;
                else
                    spellbook_page = i;
                found = 1;
            }
        }
    }
    if (found == 0)
        spellbook_page = overland_spellbook_page;
    UI_Set_Redraw_Fn(Spellbook_Cast_Redraw, 2);
    Set_Text_Shadow_Color(0);
    Font_Select_Shadow(0, 3, 0, 0);
    UI_Clear_Controls();
    UI_Set_Help(help_entries, 15);
    x = 16;
    y = 12;
    for (i = 0; i < 6; i++)
        left_controls[i] = UI_Add_Region(x + 16, y + i * 22 + 17, x + 137, y + i * 22 + 34, "", -1);
    for (i = 0; i < 6; i++)
        right_controls[i] = UI_Add_Region(x + 148, y + i * 22 + 17, x + 268, y + i * 22 + 34, "", -1);
    cancel_control = UI_Add_Region(x + 159, y + 154, x + 177, y + 183, UI_NO_HOTKEY, -1);
    next_control = UI_Add_Region(x + 259, y + 2, x + 272, y + 15, "F", -1);
    prev_control = UI_Add_Region(x + 13, y + 2, x + 26, y + 14, "B", -1);
    done = 0;
    while (done == 0) {
        input = UI_Get_Input();
        for (i = 0; i < 6; i++) {
            if (i < p_spellbook_pages[spellbook_page].count) {
                if (abs(p_spellbook_pages[spellbook_page].spells[i]) == SPELL_SPELL_OF_RETURN)
                    help_entries[i + 3].id = HELP_SPELL_OF_RETURN;
                else
                    help_entries[i + 3].id = abs(p_spellbook_pages[spellbook_page].spells[i]);
            } else
                help_entries[i + 3].id = -1;
        }
        for (i = 0; i < 6; i++) {
            if (i < p_spellbook_pages[spellbook_page + 1].count) {
                /* BUG #477: tests the left page (spellbook_page), not spellbook_page + 1 */
                if (abs(p_spellbook_pages[spellbook_page].spells[i]) == SPELL_SPELL_OF_RETURN)
                    help_entries[i + 9].id = HELP_SPELL_OF_RETURN;
                else
                    help_entries[i + 9].id = abs(p_spellbook_pages[spellbook_page + 1].spells[i]);
            } else
                help_entries[i + 9].id = -1;
        }
        if (input == cancel_control) {
            Play_Click_Sound_Casting();
            done = 1;
        }
        if (input == next_control && spellbook_page < spellbook_page_count - 2) {
            Play_Click_Sound_Casting();
            Spellbook_Page_Turn_Anim(1, 0, 0);
            spellbook_page += 2;
        }
        if (input == prev_control && spellbook_page > 1) {
            Play_Click_Sound_Casting();
            Spellbook_Page_Turn_Anim(0, 0, 0);
            spellbook_page -= 2;
        }
        for (i = 0; i < 12; i++) {
            if (left_controls[i] == input) {
                if (i < 6) {
                    if (i < p_spellbook_pages[spellbook_page].count) {
                        Play_Click_Sound();
                        page_slot = i;
                        spell = p_spellbook_pages[spellbook_page].spells[i];
                        if (spell == wizards[PLAYER_HUMAN].casting_spell) {
                            spell = SPELL_NO_CHOICE;
                            strcpy(message_text, "Do you wish to abort your ");
                            far_strcpy(spell_name, 0, p_spell_data[wizards[PLAYER_HUMAN].casting_spell].name);
                            strcat(message_text, spell_name);
                            strcat(message_text, " spell?");
                            answer = Confirm_Box(message_text);
                            if (answer == 1) {
                                wizards[PLAYER_HUMAN].casting_cost_left = 0;
                                wizards[PLAYER_HUMAN].casting_cost_initial = 0;
                                wizards[PLAYER_HUMAN].casting_spell = 0;
                                spell = SPELL_NO_CHOICE;
                                done = 1;
                            }
                        }
                    } else
                        spell = SPELL_NO_CHOICE;
                } else if (i - 6 < p_spellbook_pages[spellbook_page + 1].count) {
                    Play_Click_Sound();
                    spell = p_spellbook_pages[spellbook_page + 1].spells[i - 6];
                    page_slot = i;
                    if (spell == wizards[PLAYER_HUMAN].casting_spell) {
                        spell = SPELL_NO_CHOICE;
                        strcpy(message_text, "Do you wish to abort your ");
                        far_strcpy(spell_name, 0, p_spell_data[wizards[PLAYER_HUMAN].casting_spell].name);
                        strcat(message_text, spell_name);
                        strcat(message_text, " spell?");
                        answer = Confirm_Box(message_text);
                        if (answer == 1) {
                            wizards[PLAYER_HUMAN].casting_cost_left = 0;
                            wizards[PLAYER_HUMAN].casting_cost_initial = 0;
                            wizards[PLAYER_HUMAN].casting_spell = 0;
                            done = 1;
                        }
                    }
                } else
                    spell = SPELL_NO_CHOICE;
                if (spell > -1) {
                    if (wizards[PLAYER_HUMAN].casting_spell > 0) {
                        strcpy(message_text, "Do you wish to abort your ");
                        far_strcpy(spell_name, 0, p_spell_data[wizards[PLAYER_HUMAN].casting_spell].name);
                        strcat(message_text, spell_name);
                        strcat(message_text, " spell?");
                        answer = Confirm_Box(message_text);
                        if (answer == 1) {   /* cost_left becomes minus the mana put in so far, overwritten when
                                                the new spell starts: the aborted spell's mana is lost */
                            wizards[PLAYER_HUMAN].casting_cost_left = wizards[PLAYER_HUMAN].casting_cost_left
                                                           - wizards[PLAYER_HUMAN].casting_cost_initial;
                            wizards[PLAYER_HUMAN].casting_cost_initial = 0;
                            wizards[PLAYER_HUMAN].casting_spell = 0;
                        } else
                            spell = SPELL_NO_CHOICE;
                    } else
                        answer = 1;
                    Spell_Cast_Warnings(spell, &answer);
                    if (answer == 1) {
                        done = 1;
                        cast = 1;
                        wizards[PLAYER_HUMAN].casting_spell = spell;
                    }
                }
            }
        }
        UI_Set_Redraw_Fn(Spellbook_Cast_Redraw, 2);
        if (done == 0) {
            UI_Clear_Controls();
            x = 16;
            y = 12;
            for (i = 0; i < 6; i++)
                left_controls[i] = UI_Add_Region(x + 16, y + i * 22 + 17, x + 137, y + i * 22 + 34, "", -1);
            for (i = 0; i < 6; i++)
                right_controls[i] = UI_Add_Region(x + 148, y + i * 22 + 17, x + 268, y + i * 22 + 34, "", -1);
            cancel_control = UI_Add_Region(x + 159, y + 154, x + 177, y + 183, "\x1b", -1);
            next_control = UI_Add_Region(x + 259, y + 2, x + 272, y + 15, "F", -1);
            prev_control = UI_Add_Region(x + 13, y + 2, x + 26, y + 14, "B", -1);
            Set_Draw_Page();
            Spellbook_Cast_Redraw();
            Screen_Present();
        }
    }
    UI_Suspend_Redraw_Fn();
    current_screen = SCREEN_MAP;
    overland_spellbook_page = spellbook_page;
    UI_Clear_Controls();
    Income_Warn_Blink_On();
    UI_Clear_Help();
    if (cast == 1)
        Wizard_Begin_Casting(PLAYER_HUMAN, spell, page_slot);
    else
        Map_Redraw_Dissolve();
}

/* (player, spell, page_slot) -> 1 when cast at once: sets casting_spell and its costs - the human's variable-power
   spells (effect type 17+) through the extra-power dialog; Enchant Item / Create Artifact through the Item Forge
   (the AI's prefab item, cost cut by an uninitialised percentage - BUG #135, unreachable); Spell of Mastery's
   announcement; else the base cost and Spell_Casting_Cost (x3 for the AI's variable-power spells); the human casts
   at once when the skill and mana left this turn cover the cost
   Numbered: BUG #93 (BUGS_AND_QUIRKS.txt) */
int Wizard_Begin_Casting(int player, int spell, int page_slot)
{
    int avail;
    int cast_now;
    int item;
    int pct;                                    /* never set */

    cast_now = 0;
    wizards[player].casting_spell = spell;
    avail = wizards[player].skill_left_this_turn > wizards[player].mana_reserve ? wizards[player].mana_reserve
                                                                                 : wizards[player].skill_left_this_turn;
    if (p_spell_data[spell].effect_type >= SPELL_FX_FIRST_VARIABLE && player == PLAYER_HUMAN) {
        wizards[player].casting_cost_left = Spell_Casting_Cost(0, spell, 0);
        Overland_Extra_Power_Dialog(spell, page_slot);
    } else if (p_spell_data[spell].effect_type == SPELL_FX_CREATE_ITEM) {
        if (spell == SPELL_ENCHANT_ITEM) {
            if (player == PLAYER_HUMAN) {
                wizards[player].casting_cost_left = Screen_Item_Forge(0, 0);
                wizards[player].casting_cost_initial = p_items[ITEM_REC_WORK].cost;   /* BUG #409: the forge built the item in ITEM_REC_FORGE (137), and OVERCAST.C hands out 137; 136 holds the last random / prefab item */
                Minimap_Alloc_Pic();
            } else {
                item = Item_Create_Prefab(PREFAB_NO_ARTIFACT, wizards[player].spellbooks, 1000);
                wizards[player].casting_cost_initial = p_items[item].cost;
                wizards[player].casting_cost_left = wizards[player].casting_cost_left
                    - (int)((long)wizards[player].casting_cost_left * pct / 100);
                Mem_Copy_Far(&p_items[ITEM_REC_WORK - player], &p_items[item], sizeof(struct Item));
                Item_Free(item);
            }
        }
        if (spell == SPELL_CREATE_ARTIFACT) {
            if (player == PLAYER_HUMAN) {
                wizards[player].casting_cost_left = Screen_Item_Forge(0, 1);
                wizards[player].casting_cost_initial = p_items[ITEM_REC_WORK].cost;   /* BUG #409: the forge built the item in ITEM_REC_FORGE (137), and OVERCAST.C hands out 137; 136 holds the last random / prefab item */
                Minimap_Alloc_Pic();
            } else {
                item = Item_Create_Prefab(PREFAB_ARTIFACT, wizards[player].spellbooks, 30000);
                wizards[player].casting_cost_initial = p_items[item].cost;
                wizards[player].casting_cost_left = wizards[player].casting_cost_left
                    - (int)((long)wizards[player].casting_cost_left * pct / 100);
                Mem_Copy_Far(&p_items[ITEM_REC_WORK - player], &p_items[item], sizeof(struct Item));
                Item_Free(item);
            }
        }
        if (wizards[player].casting_cost_left == 0) {
            wizards[player].casting_cost_initial = 0;
            wizards[player].casting_spell = 0;
        }
    } else {
        if (spell == SPELL_SPELL_OF_MASTERY) {
            Screen_Mastery_Started(player);
            Global_Enchant_Relations(player, SPELL_SPELL_OF_MASTERY, 1);
        }
        wizards[player].casting_cost_initial = p_spell_data[spell].base_cost;
        wizards[player].casting_cost_left = Spell_Casting_Cost(player, spell, 0);
        if (player != PLAYER_HUMAN && p_spell_data[spell].effect_type >= SPELL_FX_FIRST_VARIABLE) {
            wizards[player].casting_cost_left *= 3;
            wizards[player].casting_cost_initial *= 3;
        }
        if (wizards[player].casting_cost_left > wizards[player].casting_cost_initial)
            wizards[player].casting_cost_initial = wizards[player].casting_cost_left;
    }
    if (wizards[player].casting_spell != SPELL_NONE) {
        if (player == PLAYER_HUMAN) {
            if (wizards[player].casting_cost_left < avail)
                avail = wizards[player].casting_cost_left;
            if (avail < 0)
                avail = 0;
            if (wizards[player].casting_cost_left <= avail) {
                wizards[player].skill_left_this_turn -= avail;
                wizards[player].mana_reserve -= avail;
                if (player == PLAYER_HUMAN)
                    Map_Redraw_Dissolve();
                cast_now = 1;
                Overland_Cast_Spell(player);
            }
        }
    } else
        wizards[player].casting_cost_initial = 0;
    if (spell > 0 && p_spell_data[spell].effect_type < SPELL_FX_CREATE_ITEM && player == PLAYER_HUMAN)
        Map_Redraw_Dissolve();
    return cast_now;
}

/* (spell, &confirm) the confirmations before casting (MESSAGE.LBX 12-17): Plane Shift under anyone's Planar Seal,
   a hero summon with no free slot, Disjunction / Spell Binding with no rival global, a wizard-targeted spell with
   no contacted living rival, a global the human already has; Holy Weapon under Holy Arms is refused */
void Spell_Cast_Warnings(int spell, int *confirm)
{
    int k;
    int answer;
    int w;
    unsigned char far *globals;
    register int t;                             /* a found flag, a hero slot, a count - by turns */

    answer = *confirm;
    if (spell == SPELL_PLANE_SHIFT) {
        t = 0;
        for (k = 0; k < num_players; k++)
            if (wizards[k].globals.planar_seal > 0)
                t = 1;
        if (t == 1) {
            LBX_Load_Records_Near("message", 0, message_text, 12, 1, 150);
            answer = Confirm_Box(message_text);
        }
    }
    if (spell == SPELL_INCARNATION || spell == SPELL_SUMMON_HERO || spell == SPELL_SUMMON_CHAMPION) {
        t = Wizard_Free_Hero_Slot(0);
        if (t == -1) {
            LBX_Load_Records_Near("message", 0, message_text, 13, 1, 150);
            answer = Confirm_Box(message_text);
        }
    }
    if (spell == SPELL_DISJUNCTION_TRUE || spell == SPELL_DISJUNCTION || spell == SPELL_SPELL_BINDING) {
        t = 0;
        for (w = 1; w < num_players && t == 0; w++) {
            globals = (unsigned char far *)&wizards[w].globals;
            for (k = 0; k < NUM_GLOBAL_ENCHANTS && t == 0; k++)
                if (globals[k] > 0)
                    t++;
        }
        if (t == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 14, 1, 150);
            answer = Confirm_Box(message_text);
        }
    }
    if (p_spell_data[spell].effect_type == SPELL_FX_VS_WIZARD || p_spell_data[spell].effect_type == SPELL_FX_VS_WIZARD_VAR) {
        t = 0;
        for (w = 1; w < num_players; w++)
            if (wizards[PLAYER_HUMAN].contacted[w] == 1 && p_fortresses[w].active == 1)
                t = 1;
        if (t == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 15, 1, 150);
            answer = Confirm_Box(message_text);
        }
    }
    if (p_spell_data[spell].effect_type == SPELL_FX_GLOBAL_ENCH) {
        globals = (unsigned char far *)&wizards[PLAYER_HUMAN].globals;
        if (globals[(int)p_spell_data[spell].effect_param] > 0) {
            LBX_Load_Records_Near("message", 0, message_text, 16, 1, 150);
            answer = Confirm_Box(message_text);
        }
    }
    if (spell == SPELL_HOLY_WEAPON && wizards[PLAYER_HUMAN].globals.holy_arms > 0) {
        LBX_Load_Records_Near("message", 0, message_text, 17, 1, 150);
        Message_Box(message_text);
        answer = 0;
    }
    *confirm = answer;
}

/* () redraw the map and show it */
void Map_Full_Redraw(void)
{
    Set_Draw_Page();
    Map_Invalidate();
    Map_Redraw();
    Screen_Present();
}

/* () redraw the map and show it with the dissolve (after spell animations) */
void Map_Redraw_Dissolve(void)
{
    Set_Draw_Page();
    Map_Invalidate();
    Map_Redraw();
    VGA_Present_Dissolve();
}

/* (victim, blocker, spell, blocker_spell: the caster and name of the Suppress Magic, Disjunction or Spell Blast that
   stopped it) -> 0 when the human is neither, else Heap_Release's value: "<X>'s <ward>
   spell has caused <Y>'s <spell> spell to fizzle." with the SPECFX picture and the jingle */
int Spell_Fizzle_Message_Overland(int victim, int blocker, int spell, char *blocker_spell)
{
    int len;
    int pic;
    char spell_name[20];

    if (victim != PLAYER_HUMAN && blocker != PLAYER_HUMAN)
        return 0;
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Redraw();
    Screen_Present();
    Screen_Copy_Front();
    ai_replan = 1;
    Sound_Stop();
    Heap_Mark(p_scratch);
    pic = LBX_Reload_Next("Specfx", 50, p_scratch);
    music_jingle = LBX_Reload_Next("MUSIC", 17, p_scratch);
    if (game_config.music == 1)
        Sound_Play(music_jingle);
    if (blocker == 0)
        strcpy(message_text, "Your ");
    else {
        strcpy(message_text, wizards[blocker].wizard_name);
        len = strlen(wizards[blocker].wizard_name);
        if (wizards[blocker].wizard_name[len - 1] == 's')
            strcat(message_text, "' ");
        else
            strcat(message_text, "'s ");
    }
    strcat(message_text, blocker_spell);
    strcat(message_text, " spell has caused ");
    if (victim == 0)
        strcat(message_text, "your ");
    else {
        strcat(message_text, wizards[victim].wizard_name);
        len = strlen(wizards[victim].wizard_name);
        if (wizards[victim].wizard_name[len - 1] == 's')
            strcat(message_text, "' ");
        else
            strcat(message_text, "'s ");
    }
    far_strcpy(spell_name, 0, p_spell_data[spell].name);
    strcat(message_text, spell_name);
    strcat(message_text, " spell to fizzle.");
    Screen_Copy_Front();
    Screen_Present();
    Notify_Picture_Box(160, 80, 1, message_text, 0, pic, -1, 7, 0, 0, 0, 1, 0);
    Minimap_Alloc_Pic();
    Set_Draw_Page();
    Map_Redraw();
    Screen_Present();
    Sound_Stop();
    Music_Play_Current();
    Heap_Release(p_scratch);
}

/* (spell, base) loads the MUSIC entry of the spell's realm: base + 3 Life, + 1 Chaos, + 5 Sorcery, + 2 Death,
   + 4 Nature, base for Arcane */
void Music_Load_Realm(int spell, int base)
{
    if (p_spell_data[spell].realm == REALM_LIFE)
        music_jingle = LBX_Reload("MUSIC", base + 3, p_music_buf);
    else if (p_spell_data[spell].realm == REALM_CHAOS)
        music_jingle = LBX_Reload("MUSIC", base + 1, p_music_buf);
    else if (p_spell_data[spell].realm == REALM_SORCERY)
        music_jingle = LBX_Reload("MUSIC", base + 5, p_music_buf);
    else if (p_spell_data[spell].realm == REALM_DEATH)
        music_jingle = LBX_Reload("MUSIC", base + 2, p_music_buf);
    else if (p_spell_data[spell].realm == REALM_NATURE)
        music_jingle = LBX_Reload("MUSIC", base + 4, p_music_buf);
    else
        music_jingle = LBX_Reload("MUSIC", base, p_music_buf);
}

/* (spell) -> 1 for a big picture (drawn 2 x 2 squares wide: Earth to Mud, the Black Wind / Word of Death /
   Death Spell entry and the realm SPECFX entries), else 0: loads the spell's effect animation into p_anim_pic - 46 spells have
   their own CMBTFX / SPECFX entry, the rest the generic summoning one (CMBTFX 22: summons, Raise / Animate Dead)
   or their realm's SPECFX entry */
int Spell_Load_Anim(int spell)
{
    int result;

    Heap_Alloc_Back(p_scratch, 1010);           /* skips the first 16160 bytes of p_scratch, untouched here: the animation
                                                   loads above what the caller keeps there (ReMoM: the battle figure space; clarified using ReMoM repo) */
    result = 0;
    switch (spell) {
    case SPELL_FIREBALL:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 23, p_scratch);
        break;
    case SPELL_MAGIC_VORTEX:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 25, p_scratch);
        break;
    case SPELL_FLAME_STRIKE:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 33, p_scratch);
        break;
    case SPELL_FIRE_BOLT:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 0, p_scratch);
        break;
    case SPELL_DISRUPT:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 1, p_scratch);
        break;
    case SPELL_WARP_WOOD:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 2, p_scratch);
        break;
    case SPELL_WARP_LIGHTNING:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 3, p_scratch);
        break;
    case SPELL_DISINTEGRATE:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 4, p_scratch);
        break;
    case SPELL_DOOM_BOLT:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 5, p_scratch);
        break;
    case SPELL_LIFE_DRAIN:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 6, p_scratch);
        break;
    case SPELL_POSSESSION:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 8, p_scratch);
        break;
    case SPELL_STAR_FIRES:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 9, p_scratch);
        break;
    case SPELL_DISPEL_EVIL:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 10, p_scratch);
        break;
    case SPELL_ICE_BOLT:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 11, p_scratch);
        break;
    case SPELL_PETRIFY:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 12, p_scratch);
        break;
    case SPELL_WEB:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 13, p_scratch);
        break;
    case SPELL_EARTH_TO_MUD:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 14, p_scratch);
        result = 1;
        break;
    case SPELL_CRACKS_CALL:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 15, p_scratch);
        break;
    case SPELL_PSIONIC_BLAST:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 16, p_scratch);
        break;
    case SPELL_VERTIGO:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 17, p_scratch);
        break;
    case SPELL_BANISH:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 19, p_scratch);
        break;
    case SPELL_CONFUSION:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 20, p_scratch);
        break;
    case SPELL_MIND_STORM:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 21, p_scratch);
        break;
    case SPELL_DISPEL_MAGIC_TRUE:
    case SPELL_DISPEL_MAGIC:
        p_anim_pic = LBX_Reload_Next("cmbtfx", 26, p_scratch);
        break;
    case SPELL_CHANGE_TERRAIN:
        p_anim_pic = LBX_Reload_Next("specfx", 8, p_scratch);
        break;
    case SPELL_STASIS:
        p_anim_pic = LBX_Reload_Next("specfx", 53, p_scratch);
        break;
    case SPELL_DISENCHANT_TRUE:
        p_anim_pic = LBX_Reload_Next("specfx", 9, p_scratch);
        break;
    case SPELL_SPELL_BLAST:
        p_anim_pic = LBX_Reload_Next("specfx", 40, p_scratch);
        break;
    case SPELL_DISJUNCTION_TRUE:
        p_anim_pic = LBX_Reload_Next("specfx", 47, p_scratch);
        break;
    case SPELL_SPELL_BINDING:
        p_anim_pic = LBX_Reload_Next("specfx", 44, p_scratch);
        break;
    case SPELL_CORRUPTION:
        p_anim_pic = LBX_Reload_Next("specfx", 7, p_scratch);
        break;
    case SPELL_WARP_NODE:
        p_anim_pic = LBX_Reload_Next("specfx", 13, p_scratch);
        break;
    case SPELL_RAISE_VOLCANO:
        p_anim_pic = LBX_Reload_Next("SPECFX", 11, p_scratch);
        break;
    case SPELL_CALL_THE_VOID:
        p_anim_pic = LBX_Reload_Next("specfx", 12, p_scratch);
        break;
    case SPELL_SUBVERSION:
        p_anim_pic = LBX_Reload_Next("specfx", 43, p_scratch);
        break;
    case SPELL_BLACK_WIND:
    case SPELL_WORD_OF_DEATH:
    case SPELL_DEATH_SPELL:
        result = 1;
        p_anim_pic = LBX_Reload_Next("specfx", 14, p_scratch);
        break;
    case SPELL_DRAIN_POWER:
        p_anim_pic = LBX_Reload_Next("specfx", 42, p_scratch);
        break;
    case SPELL_CRUEL_UNMINDING:
        p_anim_pic = LBX_Reload_Next("specfx", 41, p_scratch);
        break;
    case SPELL_DISENCHANT_AREA:
        p_anim_pic = LBX_Reload_Next("specfx", 9, p_scratch);
        break;
    case SPELL_DISJUNCTION:
        p_anim_pic = LBX_Reload_Next("specfx", 48, p_scratch);
        break;
    case SPELL_FIRE_STORM:
        p_anim_pic = LBX_Reload_Next("specfx", 6, p_scratch);
        break;
    case SPELL_ICE_STORM:
        p_anim_pic = LBX_Reload_Next("specfx", 10, p_scratch);
        break;
    case SPELL_CALL_LIGHTNING:
        p_anim_pic = LBX_Reload_Next("CMBTFX", 24, p_scratch);
        break;
    default:
        if ((p_spell_data[spell].effect_type == SPELL_FX_SUMMON && spell != SPELL_FLOATING_ISLAND) || spell == SPELL_RAISE_DEAD || spell == SPELL_ANIMATE_DEAD) {
            p_anim_pic = LBX_Reload_Next("cmbtfx", 22, p_scratch);
            result = 0;
        } else {
            p_anim_pic = LBX_Reload_Next("specfx", p_spell_data[spell].realm, p_scratch);
            result = 1;
        }
        break;
    }
    return result;
}

/* (spell) loads the spell's sound (SOUNDFX entry, or NEWSOUND |entry| when negative) into p_sfx_buf; -1 with
   sound effects off */
void Spell_Load_Sound(int spell)
{
    if (game_config.sound_effects == 1) {
        if (p_spell_data[spell].sound_index > -1)
            p_sfx_buf = LBX_Reload_Next("SOUNDFX", p_spell_data[spell].sound_index, p_scratch);
        else
            p_sfx_buf = LBX_Reload_Next("NEWSOUND", abs(p_spell_data[spell].sound_index), p_scratch);
    } else
        p_sfx_buf = -1;
}

/* (x, y, plane) brings the square into view (Map_View_Ensure_Visible) and plays p_anim_pic over it frame by frame (at least 12, after 3
   blank frames, 2 ticks each) with its sound, then a closing sound; restores the view */
void Map_Spell_Effect_Anim(int x, int y, int plane)
{
    int sx;
    int sy;
    int sound;
    int saved_x;
    int saved_y;
    int saved_plane;
    register int i;
    register int frames;

    sx = x;
    sy = y;
    sound = LBX_Reload_Next("soundfx", SFX_SILENCE, p_scratch);
    frames = Pic_Frame_Count(p_anim_pic);
    if (frames < 12)
        frames = 12;
    saved_x = map_view_x;
    saved_y = map_view_y;
    saved_plane = map_view_plane;
    Map_View_Ensure_Visible(&map_view_x, &map_view_y, x, y, plane);
    map_view_plane = plane;
    Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy);
    if (p_sfx_buf != -1)
        Sound_Play(p_sfx_buf);
    for (i = 0; i < frames + 3; i++) {
        Timer_Mark();
        Map_Redraw();
        if (i >= 3)
            Draw_Pic_Clipped(sx - MAP_TILE_W, sy - MAP_TILE_H, p_anim_pic);
        Screen_Present();
        Wait_Ticks(2);
    }
    Timer_Mark();
    Wait_Ticks(5);
    Sound_Play(sound);
    map_view_x = saved_x;
    map_view_y = saved_y;
    map_view_plane = saved_plane;
}
