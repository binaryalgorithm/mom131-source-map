/* SPELLBK.C - WIZARDS.EXE overlay code segment 9a4b (stub 2559): the spellbook - collecting the known spells (overland,
   combat, research), counting them by group (book section with the Spell Book Ordering option, else realm), laying
   the groups out on pages, the overland and combat books (casting budget, turns or casts left per spell, the
   per-page drawing) and the four-frame page turn.
   This module's data (DS:5d1c..5dbf): its literals only; BSS (DS:c8fa..c927): the spell list, the group counts
   and the page / budget words below them (owner not fixed by the EXE, b126).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 9a4b src/wizards/SPELLBK.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Spellbook_Collect_Spells(int mode, int far *list);
void Spell_List_Add_Realm(int far *list, int realm);
void Spellbook_Count_Groups(void);
void Spellbook_Build(int mode, int per_page);
int Combat_Spellbook_Build(int caster);
int Wizard_Cast_Turns(int player, int spell);
void Spellbook_Draw_Page_Overland(struct SpellbookPage page, unsigned pic);
void Spellbook_Draw_Page_Combat(struct SpellbookPage page, unsigned pic, int caster);
void Spellbook_Overland_Redraw(int x, int y);
void Spellbook_Combat_Redraw(int x, int y, int caster);
void Spellbook_Page_Turn_Anim(int forward, int is_combat, int caster);
void Spellbook_Fill_Group(int n, int group, char *title, int per_page);
void Spellbook_Layout_Groups(int per_page);

#include "GAME.H"

/* the font colors (Font_Select_Palette: slot 0, then slots 1-4): a spell's name, a spell the caster cannot cast,
   a page title; the spell being cast is drawn in PAL_CYCLE_COLOR */
#define SB_NAME_COLOR0 0xb8
#define SB_NAME_COLOR 0xbb
#define SB_NO_CAST_COLOR 0x1a
#define SB_TITLE_COLOR0 0x35
#define SB_TITLE_COLOR 0x2e

int list_count;                                 /* DS:c926  spells in p_spellbook_spells */
                                                /* also: the research pool / charge list; the human's fame change
                                                   (COMBUTIL.C, COMBEND.C: combat_fame_change); the units disbanded
                                                   (TURNUPD.C); the learned spell's row (RESEARCH.C) */
int spellbook_page;                             /* DS:c924  the left page shown */
int spellbook_mode;                        /* DS:c922 */
                                                /* 1: the page corners turn (combat book, COMBSPL.C); RESEARCH.C: 0 the
                                                   choose prompt or the learned spell on the left page, 1 on the right page,
                                                   2 the Apprentice browse */
int sb_count_summon_arcane;                     /* DS:c920  spells per group: Summoning / Arcane */
int sb_count_special_chaos;                     /* DS:c91e  Special Spells / Chaos */
int sb_count_city_death;                        /* DS:c91c  City Spells / Death */
int sb_count_unit_nature;                       /* DS:c91a  Unit Spells / Nature */
int sb_count_enchant_life;                      /* DS:c918  Enchantment / Life */
int sb_count_combat_sorcery;                    /* DS:c916  Combat Spells / Sorcery */
int spellbook_page_count;                       /* DS:c914 */
int far *p_spellbook_spells;                    /* DS:c910  the book's spells (negative: not yet known) */
/* DS:c8fa..c90f, owner not fixed by the EXE (b126): the words between ITEMFORG.C's and this module's BSS; no module between
   them references them - this one and the research / magic screens do */
int net_mana_per_turn;                          /* DS:c90e */
int *research_per_turn_by_realm;                /* DS:c90c  [realm] the research per turn with the realm's bonus */
#define COST_ITEM_CHARGE 22222                  /* the cost shown as "Item": the spell cast from an item's charges */
int spend_per_turn;                             /* DS:c90a */
/* spend_per_turn: what the caster can put into spells - overland the human's skill (+ fortress hero bonus) or the
   mana when lower; in combat the wizard's skill left (capped by the mana) or the unit's spell points
   (Combat_Spellbook_Build) */
unsigned spellbook_realm_icon_pics[6];                /* DS:c8fe */
int ui_anim_frame;                              /* DS:c8fc */
struct SpellbookPage *p_spellbook_pages;        /* DS:c8fa */

extern struct Item far *p_items;                /* DS:9136 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int research_first_page;                 /* DS:9448 */
extern int combat_spellbook_page;               /* DS:944c */
extern unsigned spellbook_page_bg_pic;          /* DS:9452 */
extern unsigned spellbook_realm_icon_src_pics[];      /* DS:9454 */
extern unsigned spellbook_corner_right_pic;     /* DS:9468 */
extern unsigned spellbook_corner_left_pic;      /* DS:946a */
extern unsigned spellbook_small_pic;            /* DS:946e */
extern int combat_battlefield_kind;                /* DS:c520 */
extern int combat_attacker_player;              /* DS:c586 */
extern unsigned ui_temp_pic_a;                  /* DS:ca34 */
extern int *p_combat_path_prev;                  /* DS:d144 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
void far_strcpy(char *dst, int dst_seg, char far *src);
void Anim_EMS_Next_Frame(void);
void Anim_Load_To_EMS(char *lbx, int entry);
void Anim_Set_Frame(int frame);
int Combat_Range_Multiplier(int player);
void Combat_Spellbook_Redraw_CB(void);
void Draw_Bitmap(int x, int y, unsigned pic);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
int Fortress_Hero_Skill_Bonus(int player);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Minimap_Alloc_Pic(void);
void Near_Alloc_Mark(void);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
int Pic_Clear_Rect(int x1, int y1, int x2, int y2, unsigned pic);
void Pic_Create_Blank(int width, int height, unsigned pic);
int Pic_Draw_Into_Pic(int x, int y, unsigned dst, unsigned src);
void Pic_Scale_Percent(unsigned pic, int x_pct, int y_pct);
void Pic_Shear_Columns(int x1, int dy1, int x2, int dy2, unsigned pic);
int Print_Text_Centered(int x, int y, char *text);
int Print_Text_Centered_To_Pic(int x, int y, char *text, unsigned pic);
int Print_Text_Right_To_Pic(int x, int y, char *text, unsigned pic);
int Print_Text_To_Pic(int x, int y, char *text, unsigned pic);
int Research_Bonus_Pct(int player, int spell);
void far *Seg_To_Far_Ptr(unsigned seg);
int Spell_Casting_Cost(int player, int spell, int in_combat);
void Spellbook_Cast_Redraw(void);
int Text_Width(char *text);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Wizard_Magic_Split(int *mana, int *skill, int *research, int player);
int Wizard_Mana_Upkeep(int player);
void Wizard_Power_Base(void);

/* (mode, list): appends the human's spells to list - 0 known and castable overland, 1 known and castable in combat,
   2 every spell he can research or knows (known ones positive, the others negative) */
void Spellbook_Collect_Spells(int mode, int far *list)
{
    int realm;
    int k;
    int kind;
    int spell;

    for (realm = 0; realm < NUM_REALMS; realm++) {
        for (k = 0; k < SPELLS_PER_REALM; k++) {
            kind = 0;
            spell = realm * SPELLS_PER_REALM + k + 1;
            switch (mode) {
            case SPELLBOOK_OVERLAND:
                if (wizards[PLAYER_HUMAN].spell_library[realm][k] == SPELLSTATE_KNOWN && (char)p_spell_data[spell].castable > CASTABLE_COMBAT_ONLY &&
                    p_spell_data[spell].castable != CASTABLE_NEVER) {
                    kind = 1;
                }
                break;
            case SPELLBOOK_COMBAT:
                if (wizards[PLAYER_HUMAN].spell_library[realm][k] == SPELLSTATE_KNOWN && p_spell_data[spell].castable != CASTABLE_OVERLAND_ONLY &&
                    p_spell_data[spell].castable != CASTABLE_NEVER) {
                    kind = 1;
                }
                break;
            case SPELLBOOK_RESEARCH:
                if (wizards[PLAYER_HUMAN].spell_library[realm][k] > 0) {
                    if (wizards[PLAYER_HUMAN].spell_library[realm][k] == SPELLSTATE_KNOWN) {
                        kind = 1;
                    } else {
                        kind = 2;
                    }
                }
                break;
            }
            if (kind > 0) {
                if (kind == 1) {
                    list[list_count] = spell;
                } else {
                    list[list_count] = -spell;
                }
                list_count++;
            }
        }
    }
}

/* (list, realm): appends the realm's 40 spells but the overland-only ones - CASTABLE_NEVER passes too - (creatures
   casting their realm) */
void Spell_List_Add_Realm(int far *list, int realm)
{
    int r;
    int unused;                                 /* set to 0, never read */
    int k;
    int spell;

    r = realm;
    for (k = 0; k < SPELLS_PER_REALM; k++) {
        unused = 0;
        spell = r * SPELLS_PER_REALM + k + 1;
        if (p_spell_data[spell].castable != CASTABLE_OVERLAND_ONLY) {
            list[list_count] = spell;
            list_count++;
        }
    }
}

/* the spells of the list per group: book section with the Spell Book Ordering option, else realm */
void Spellbook_Count_Groups(void)
{
    int i;

    if (game_config.book_ordering == 1) {
        for (i = 0; i < list_count; i++) {
            switch ((char)p_spell_data[abs(p_spellbook_spells[i])].book_section) {
            case BOOK_SECTION_SUMMONING:
                sb_count_summon_arcane++;
                break;
            case BOOK_SECTION_SPECIAL:
                sb_count_special_chaos++;
                break;
            case BOOK_SECTION_CITY:
                sb_count_city_death++;
                break;
            case BOOK_SECTION_ENCHANTMENT:
                sb_count_enchant_life++;
                break;
            case BOOK_SECTION_UNIT:
                sb_count_unit_nature++;
                break;
            case BOOK_SECTION_COMBAT:
                sb_count_combat_sorcery++;
                break;
            default:
                break;
            }
        }
    } else {
        for (i = 0; i < list_count; i++) {
            switch (p_spell_data[abs(p_spellbook_spells[i])].realm) {
            case REALM_ARCANE:
                sb_count_summon_arcane++;
                break;
            case REALM_CHAOS:
                sb_count_special_chaos++;
                break;
            case REALM_DEATH:
                sb_count_city_death++;
                break;
            case REALM_LIFE:
                sb_count_enchant_life++;
                break;
            case REALM_NATURE:
                sb_count_unit_nature++;
                break;
            case REALM_SORCERY:
                sb_count_combat_sorcery++;
                break;
            }
        }
    }
}

/* (mode, per_page): the overland / research book - the spell list (13 paragraphs: 104 spells, BUG #133: a full book
   overflows it), the research points per realm, the casting budget (casting skill + fortress heroes, at most the mana
   income + reserve), the pages; 4 per page (the research book) adds the Research and Spells candidate pages
   Numbered: BUG #40, #133, #269 (BUGS_AND_QUIRKS.txt) */
void Spellbook_Build(int mode, int per_page)
{
    int i;
    int research;
    int scratch;                                /* Wizard_Magic_Split's discarded mana / skill outputs, the
                                                   research bonus percentage, then the page count */
    int skipped;                                /* the slot loop index, then the research-candidate skip count */
    int bonus;

    p_spellbook_spells = Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, 13));
    research_per_turn_by_realm = (int *)Near_Alloc_Reset(12);
    sb_count_city_death = 0;
    sb_count_special_chaos = 0;
    sb_count_summon_arcane = 0;
    sb_count_enchant_life = 0;
    sb_count_unit_nature = 0;
    sb_count_combat_sorcery = 0;
    list_count = 0;
    spellbook_page_count = 0;
    Wizard_Power_Base();
    Wizard_Magic_Split(&scratch, &scratch, &research, 0);
    for (i = 0; i < NUM_REALMS; i++) {
        scratch = Research_Bonus_Pct(0, i * SPELLS_PER_REALM + 1);
        research_per_turn_by_realm[i] = research + (int)((long)research * scratch / 100);
    }
    wizards[PLAYER_HUMAN].nominal_casting_skill = Wizard_Casting_Skill(0);
    net_mana_per_turn = wizards[PLAYER_HUMAN].mana_percentage * wizards[PLAYER_HUMAN].power_base / 100 - Wizard_Mana_Upkeep(0);
    bonus = Fortress_Hero_Skill_Bonus(0);
    if (net_mana_per_turn + wizards[PLAYER_HUMAN].mana_reserve > wizards[PLAYER_HUMAN].nominal_casting_skill + bonus) {
        spend_per_turn = wizards[PLAYER_HUMAN].nominal_casting_skill + bonus;
    } else if (net_mana_per_turn + wizards[PLAYER_HUMAN].mana_reserve > 0) {
        spend_per_turn = net_mana_per_turn + wizards[PLAYER_HUMAN].mana_reserve;
    } else {
        spend_per_turn = 0;
    }
    Spellbook_Collect_Spells(mode, p_spellbook_spells);
    Spellbook_Count_Groups();
    scratch = (sb_count_city_death + per_page - 1) / per_page;
    scratch += (sb_count_special_chaos + per_page - 1) / per_page;
    scratch += (sb_count_summon_arcane + per_page - 1) / per_page;
    scratch += (sb_count_enchant_life + per_page - 1) / per_page;
    scratch += (sb_count_unit_nature + per_page - 1) / per_page;
    scratch += (sb_count_combat_sorcery + per_page - 1) / per_page;
    p_spellbook_pages = (struct SpellbookPage *)Near_Alloc((scratch + 3) * 30);   /* sizeof(struct SpellbookPage) */
    for (i = 0; i < scratch + 3; i++) {
        p_spellbook_pages[i].count = 0;
        strcpy(p_spellbook_pages[i].title, "");
        for (skipped = 0; skipped < per_page; skipped++) {
            p_spellbook_pages[i].spells[skipped] = 0;
        }
    }
    Spellbook_Layout_Groups(per_page);
    if (per_page == RESEARCH_PAGE_SPELLS) {
        research_first_page = spellbook_page_count;
        skipped = 0;
        for (i = 0; i < 4; i++) {
            if (wizards[PLAYER_HUMAN].research_candidates[i] > 0) {
                p_spellbook_spells[i - skipped] = wizards[PLAYER_HUMAN].research_candidates[i];
            } else {
                skipped++;
            }
        }
        Spellbook_Fill_Group(4 - skipped, -1, "Research", per_page);
        skipped = 0;
        for (i = 0; i < 4; i++) {
            if (wizards[PLAYER_HUMAN].research_candidates[i + 4] > 0) {
                p_spellbook_spells[i - skipped] = wizards[PLAYER_HUMAN].research_candidates[i + 4];
            } else {
                skipped++;
            }
        }
        Spellbook_Fill_Group(4 - skipped, -1, "Spells", per_page);
    }
}

/* (player, spell): turns to cast the spell = cost / min(casting skill, mana income + reserve), 999 when that is 0
   (no callers) */
int Wizard_Cast_Turns(int player, int spell)
{
    int budget;
    int cost;
    int net;

    wizards[player].nominal_casting_skill = Wizard_Casting_Skill(player);
    net = wizards[player].mana_percentage * wizards[player].power_base / 100 - Wizard_Mana_Upkeep(player);
    if (net + wizards[player].mana_reserve > wizards[player].nominal_casting_skill) {
        budget = wizards[player].nominal_casting_skill;
    } else if (net + wizards[player].mana_reserve > 0) {
        budget = net + wizards[player].mana_reserve;
    } else {
        budget = 0;
    }
    cost = Spell_Casting_Cost(player, spell, 0);
    if (budget == 0) {
        return 999;
    } else {
        return cost / budget;
    }
}

/* (caster): the combat book of a battle unit (0..19) or wizard (20 + player) - 1 when anything can be cast. Budget:
   the unit's spell points, or the wizard's combat skill left (at most reserve x 10 / range multiplier) - both always
   wizards[PLAYER_HUMAN]'s, which is right: only the human's own casts build a book. A unit gets the human
   wizard's combat spells, its hero spells and its item's spell; a creature caster
   its realm's spells. On water no Cracks Call and no summons that neither fly nor are non-corporeal (swimmers too)
   Numbered: BUG #134 (BUGS_AND_QUIRKS.txt) */
int Combat_Spellbook_Build(int caster)
{
    int i;
    int n;                                      /* by turns: range multiplier, unit type, hero slot, page count */
    int slot;
    int found;
    int k;
    int dup;
    int item_spell;

    Near_Alloc_Mark();
    Heap_Mark(p_scratch);
    p_spellbook_spells = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 13));
    sb_count_city_death = 0;
    sb_count_special_chaos = 0;
    sb_count_summon_arcane = 0;
    sb_count_enchant_life = 0;
    sb_count_unit_nature = 0;
    sb_count_combat_sorcery = 0;
    list_count = 0;
    spellbook_page_count = 0;
    if (caster > CASTER_WIZARD - 1) {
        n = Combat_Range_Multiplier(0);
        spend_per_turn = wizards[PLAYER_HUMAN].combat_skill_left;
        /* BUG #476: at the 0.5x range multiplier (n 5) a reserve over 16383 gives a quotient over 32767: the int is
           negative, so the budget becomes negative and nothing is castable - in a battle at the wizard's own fortress
           (Combat_Range_Multiplier 5). The same formula: the page's castable test below, COMBAI.C (the AI's budget), COMBSPL.C and
           CASTUI.C (a variable spell's extra strength). Below: a unit's test uses base_cost (no
           Evil Omens), item charges count as castable whatever the spell, and Cracks Call is compared without abs().
           clarified using ReMoM repo */
        if ((int)((long)wizards[PLAYER_HUMAN].mana_reserve * 10 / n) < spend_per_turn) {
            spend_per_turn = (long)wizards[PLAYER_HUMAN].mana_reserve * 10 / n;
        }
    } else {
        spend_per_turn = p_battle_units[caster].spell_points;
    }
    if (caster < CASTER_WIZARD) {
        if (p_battle_units[caster].abilities & ABILITY_CASTER) {
            Spell_List_Add_Realm(p_spellbook_spells, (char)p_battle_units[caster].race - RACE_NATURE_CREATURE);
        } else {
            Spellbook_Collect_Spells(SPELLBOOK_COMBAT, p_spellbook_spells);
            n = p_units[p_battle_units[caster].unit_index].type;
            for (i = 0; i < 4; i++) {
                dup = 0;
                if (p_hero_stats[PLAYER_HUMAN][n].spells[i] <= 0) {
                    break;
                } else {
                    for (k = 0; k < list_count; k++) {
                        if (p_hero_stats[PLAYER_HUMAN][n].spells[i] == p_spellbook_spells[k]) {
                            dup = 1;
                        }
                    }
                    if (dup == 0) {
                        p_spellbook_spells[list_count] = p_hero_stats[PLAYER_HUMAN][n].spells[i];
                        list_count++;
                    }
                }
            }
            if (p_battle_units[caster].item_charges > 0) {
                n = p_units[p_battle_units[caster].unit_index].hero_slot;
                item_spell = (unsigned char)
                    p_items[wizards[p_battle_units[caster].owner].hired_heroes[n].item_index[0]].spell;
                if (item_spell > 0) {
                    dup = 0;
                    for (k = 0; k < list_count; k++) {
                        if (p_spellbook_spells[k] == item_spell) {
                            dup = 1;
                        }
                    }
                    if (dup == 0) {
                        p_spellbook_spells[list_count] = item_spell;
                        list_count++;
                    }
                }
            }
        }
    } else {
        Spellbook_Collect_Spells(SPELLBOOK_COMBAT, p_spellbook_spells);
    }
    found = 0;
    if (caster > CASTER_WIZARD - 1) {
        for (i = 0; i < list_count; i++) {
            if (Spell_Casting_Cost(caster - CASTER_WIZARD, abs(p_spellbook_spells[i]), 1) <= spend_per_turn &&
                (p_spell_data[abs(p_spellbook_spells[i])].effect_type != SPELL_FX_SUMMON || combat_battlefield_kind != BATTLEFIELD_WATER ||
                 (unit_types[(int)p_spell_data[abs(p_spellbook_spells[i])].effect_param].move_flags & MOVE_FLYING) ||
                 (unit_types[(int)p_spell_data[abs(p_spellbook_spells[i])].effect_param].ability_flags & AFLAG_NON_CORPOREAL)) &&
                (p_spellbook_spells[i] != SPELL_CRACKS_CALL || combat_battlefield_kind != BATTLEFIELD_WATER)) {
                found = 1;
            }
        }
    } else if (p_battle_units[caster].item_charges > 0) {
        found = 1;
    } else {
        for (i = 0; i < list_count; i++) {
            if (p_spell_data[abs(p_spellbook_spells[i])].base_cost <= spend_per_turn &&
                (p_spell_data[abs(p_spellbook_spells[i])].effect_type != SPELL_FX_SUMMON || combat_battlefield_kind != BATTLEFIELD_WATER ||
                 (unit_types[(int)p_spell_data[abs(p_spellbook_spells[i])].effect_param].move_flags & MOVE_FLYING) ||
                 (unit_types[(int)p_spell_data[abs(p_spellbook_spells[i])].effect_param].ability_flags & AFLAG_NON_CORPOREAL)) &&
                (p_spellbook_spells[i] != SPELL_CRACKS_CALL || combat_battlefield_kind != BATTLEFIELD_WATER)) {
                found = 1;
            }
        }
    }
    Spellbook_Count_Groups();
    n = (sb_count_city_death + (BOOK_PAGE_SPELLS - 1)) / BOOK_PAGE_SPELLS;
    n += (sb_count_special_chaos + (BOOK_PAGE_SPELLS - 1)) / BOOK_PAGE_SPELLS;
    n += (sb_count_summon_arcane + (BOOK_PAGE_SPELLS - 1)) / BOOK_PAGE_SPELLS;
    n += (sb_count_enchant_life + (BOOK_PAGE_SPELLS - 1)) / BOOK_PAGE_SPELLS;
    n += (sb_count_unit_nature + (BOOK_PAGE_SPELLS - 1)) / BOOK_PAGE_SPELLS;
    n += (sb_count_combat_sorcery + (BOOK_PAGE_SPELLS - 1)) / BOOK_PAGE_SPELLS;
    p_spellbook_pages = (struct SpellbookPage *)p_combat_path_prev;   /* the combat path buffer, borrowed */
    for (i = 0; i < n + 3; i++) {
        p_spellbook_pages[i].count = 0;
        strcpy(p_spellbook_pages[i].title, "");
        for (slot = 0; slot < BOOK_PAGE_SPELLS; slot++) {
            p_spellbook_pages[i].spells[slot] = 0;
        }
    }
    Spellbook_Layout_Groups(6);
    Heap_Release(p_scratch);
    if (combat_spellbook_page > spellbook_page_count) {
        combat_spellbook_page = spellbook_page_count - 2;
    }
    return found;
}

/* (page, pic): an overland book page - per spell the name, the cost in MP and the turns it takes (an icon per turn,
   at most 40; "Instant" / "Special" when it can be cast this turn), the spell being cast highlighted
   Numbered: BUG #39 (BUGS_AND_QUIRKS.txt) */
void Spellbook_Draw_Page_Overland(struct SpellbookPage page, unsigned pic)
{
    int i;
    int j;
    char colors[6];
    int spell;
    int cost;
    int avail;
    int turns;
    int w;
    char cost_text[10];
    char name[20];

    Pic_Decode_Frame_To_Bitmap(spellbook_page_bg_pic, pic);
    for (i = 0; i < BOOK_PAGE_SPELLS; i++) {
        if (i < page.count) {
            colors[0] = SB_NAME_COLOR0;
            for (j = 1; j < 5; j++) {
                colors[j] = SB_NAME_COLOR;
            }
            Font_Select_Palette(1, colors);
            spell = page.spells[i];
            cost = abs(spell) == wizards[PLAYER_HUMAN].casting_spell ? wizards[PLAYER_HUMAN].casting_cost_left
                                                         : Spell_Casting_Cost(0, abs(spell), 0);
            if (spend_per_turn == 0) {
                turns = 40;
            } else {
                turns = cost / spend_per_turn;
            }
            if (turns > 40) {
                turns = 40;
            }
            if (turns < 1) {
                turns = 1;
                avail = wizards[PLAYER_HUMAN].skill_left_this_turn > wizards[PLAYER_HUMAN].mana_reserve ? wizards[PLAYER_HUMAN].mana_reserve
                                                                                   : wizards[PLAYER_HUMAN].skill_left_this_turn;
                if (avail >= cost) {
                    if (p_spell_data[abs(spell)].effect_type == SPELL_FX_CREATE_ITEM) {
                        w = Text_Width("Special");
                        Pic_Clear_Rect(9, i * 22 + 7, w + 12, i * 22 + 12, pic);
                        Print_Text_To_Pic(9, i * 22 + 7, "Special", pic);
                    } else {
                        w = Text_Width("Instant");
                        Pic_Clear_Rect(9, i * 22 + 7, w + 12, i * 22 + 12, pic);
                        Print_Text_To_Pic(9, i * 22 + 7, "Instant", pic);
                    }
                }
                Pic_Clear_Rect(0, i * 22 + 7, 8, i * 22 + 12, pic);
                Pic_Draw_Into_Pic(1, i * 22 + 7, pic, spellbook_realm_icon_pics[p_spell_data[abs(spell)].realm]);
            } else {
                if (turns < 20) {
                    Pic_Clear_Rect(0, i * 22 + 7, turns * 6 + 3, i * 22 + 12, pic);
                } else {
                    Pic_Clear_Rect(0, i * 22 + 7, 121, i * 22 + 12, pic);
                    Pic_Clear_Rect(0, i * 22 + 13, (turns - 20) * 6 + 3, i * 22 + 19, pic);
                }
                for (j = 0; j < turns; j++) {
                    if (j < 20) {
                        Pic_Draw_Into_Pic(j * 6 + 1, i * 22 + 7, pic,
                                          spellbook_realm_icon_pics[p_spell_data[abs(spell)].realm]);
                    } else {
                        Pic_Draw_Into_Pic(j % 20 * 6 + 1, i * 22 + 13, pic,
                                          spellbook_realm_icon_pics[p_spell_data[abs(spell)].realm]);
                    }
                }
            }
            if (abs(spell) == wizards[PLAYER_HUMAN].casting_spell) {
                for (j = 0; j < 5; j++) {
                    colors[j] = PAL_CYCLE_COLOR;
                }
            } else {
                colors[0] = SB_NAME_COLOR0;
                for (j = 1; j < 5; j++) {
                    colors[j] = SB_NAME_COLOR;
                }
            }
            Font_Select_Palette(1, colors);
            far_strcpy(name, 0, p_spell_data[abs(spell)].name);
            w = Text_Width(name);
            Pic_Clear_Rect(0, i * 22, w + 3, i * 22 + 5, pic);
            Print_Text_To_Pic(0, i * 22, name, pic);
            cost = abs(spell) == wizards[PLAYER_HUMAN].casting_spell ? wizards[PLAYER_HUMAN].casting_cost_left
                                                         : Spell_Casting_Cost(0, abs(spell), 0);
            if (cost < 0) {
                cost = 0;
            }
            if (cost == 0 && p_spell_data[abs(spell)].effect_type == SPELL_FX_CREATE_ITEM) {
                strcpy(cost_text, "?");
            } else {
                itoa(cost, cost_text, 10);
            }
            strcat(cost_text, " MP");
            w = Text_Width(cost_text);
            Pic_Clear_Rect(118 - w, i * 22, 121, i * 22 + 5, pic);
            Print_Text_Right_To_Pic(121, i * 22, cost_text, pic);
        } else {
            Pic_Clear_Rect(0, i * 22, 121, i * 22 + 17, pic);
        }
    }
}

/* (page, pic, caster): a combat book page - per spell the name, the cost and an icon per cast left (at most 40,
   greyed when none): town spells only for the defender of a city, no summons on water unless they fly or are non-corporeal, no
   Cracks Call on water; an item's spell shows "Item" and its charges (BUG #436: caster < 19 tests the unit budget) */
void Spellbook_Draw_Page_Combat(struct SpellbookPage page, unsigned pic, int caster)
{
    int i;
    int j;
    char colors[6];
    int spell;
    int cost;
    int casts;
    int avail;                                  /* also the text width */
    char cost_text[10];
    char name[20];

    Pic_Decode_Frame_To_Bitmap(spellbook_page_bg_pic, pic);
    for (i = 0; i < BOOK_PAGE_SPELLS; i++) {
        if (i < page.count) {
            spell = page.spells[i];
            cost = caster < CASTER_WIZARD - 1 ? p_spell_data[abs(spell)].base_cost : Spell_Casting_Cost(0, abs(spell), 1);
            if (cost == 0) {
                casts = 40;
            } else {
                if (caster > CASTER_WIZARD - 1) {
                    casts = Combat_Range_Multiplier(0);
                    avail = wizards[PLAYER_HUMAN].combat_skill_left;
                    if ((int)((long)wizards[PLAYER_HUMAN].mana_reserve * 10 / casts) < avail) {
                        avail = (long)wizards[PLAYER_HUMAN].mana_reserve * 10 / casts;
                    }
                    if (cost > avail) {
                        casts = 0;
                    } else {
                        casts = avail / cost;
                    }
                } else {
                    if (cost > spend_per_turn) {
                        casts = 0;
                    } else {
                        casts = spend_per_turn / cost;
                    }
                    if (p_battle_units[caster].item_charges > 0 &&
                        (unsigned char)p_items[wizards[PLAYER_HUMAN].hired_heroes[p_units[p_battle_units[caster].unit_index]
                                                   .hero_slot].item_index[0]].spell == spell) {
                        casts = p_battle_units[caster].item_charges;
                        cost = COST_ITEM_CHARGE;
                    }
                }
                if ((p_spell_data[abs(spell)].effect_type == SPELL_FX_CITY_ENCH || abs(spell) == SPELL_WALL_OF_STONE) &&
                    (combat_attacker_player == PLAYER_HUMAN || combat_site != COMBAT_AT_CITY)) {
                    casts = 0;
                }
                if (p_spell_data[abs(spell)].effect_type == SPELL_FX_SUMMON && combat_battlefield_kind == BATTLEFIELD_WATER &&
                    (unit_types[(int)p_spell_data[abs(spell)].effect_param].move_flags & MOVE_FLYING) == 0 &&
                    (unit_types[(int)p_spell_data[abs(spell)].effect_param].ability_flags & AFLAG_NON_CORPOREAL) == 0) {
                    casts = 0;
                }
                if (spell == SPELL_CRACKS_CALL && combat_battlefield_kind == BATTLEFIELD_WATER) {
                    casts = 0;
                }
            }
            if (casts > 40) {
                casts = 40;
            }
            if (casts < 1) {
                for (j = 0; j < 5; j++) {
                    colors[j] = SB_NO_CAST_COLOR;
                }
            } else {
                if (casts < 20) {
                    Pic_Clear_Rect(0, i * 22 + 7, casts * 6 + 3, i * 22 + 12, pic);
                } else {
                    Pic_Clear_Rect(0, i * 22 + 7, 121, i * 22 + 12, pic);
                    Pic_Clear_Rect(0, i * 22 + 13, (casts - 20) * 6 + 3, i * 22 + 19, pic);
                }
                for (j = 0; j < casts; j++) {
                    if (j < 20) {
                        Pic_Draw_Into_Pic(j * 6 + 1, i * 22 + 7, pic,
                                          spellbook_realm_icon_pics[p_spell_data[abs(spell)].realm]);
                    } else {
                        Pic_Draw_Into_Pic(j % 20 * 6 + 1, i * 22 + 13, pic,
                                          spellbook_realm_icon_pics[p_spell_data[abs(spell)].realm]);
                    }
                }
                colors[0] = SB_NAME_COLOR0;
                for (j = 1; j < 5; j++) {
                    colors[j] = SB_NAME_COLOR;
                }
            }
            Font_Select_Palette(1, colors);
            far_strcpy(name, 0, p_spell_data[abs(spell)].name);
            avail = Text_Width(name);
            Pic_Clear_Rect(0, i * 22, avail + 3, i * 22 + 5, pic);
            Print_Text_To_Pic(0, i * 22, name, pic);
            if (cost == 0) {
                strcpy(cost_text, "?");
            } else if (cost == COST_ITEM_CHARGE) {
                strcpy(cost_text, "Item");
            } else {
                itoa(cost, cost_text, 10);
                strcat(cost_text, " MP");
            }
            avail = Text_Width(cost_text);
            Pic_Clear_Rect(118 - avail, i * 22, 121, i * 22 + 5, pic);
            Print_Text_Right_To_Pic(120, i * 22, cost_text, pic);
        } else {
            Pic_Clear_Rect(0, i * 22, 121, i * 22 + 17, pic);
        }
    }
}

/* (x, y): redraws the overland book - the realm icons, the color cycling, the book, the page corners when there are
   more pages, both pages with their titles */
void Spellbook_Overland_Redraw(int x, int y)
{
    int i;
    char colors[6];
    unsigned pic;

    Heap_Mark(p_scratch);
    pic = Heap_Alloc_Back(p_scratch, 0x41a);    /* paragraphs: a page bitmap (Spellbook_Page_Turn_Anim takes 0x410
                                                   for the same bitmap) */
    for (i = 0; i < NUM_REALMS; i++) {
        spellbook_realm_icon_pics[i] = Heap_Alloc_Back(p_scratch, 4);   /* paragraphs: a realm icon bitmap */
        Pic_Decode_Frame_To_Bitmap(spellbook_realm_icon_src_pics[i], spellbook_realm_icon_pics[i]);
    }
    Palette_Cycle_Color(PAL_CYCLE_COLOR, 0x29, 0x21, 0x1d, 0x30, 0x2b, 0x2b, 3);
    Draw_Pic_Frame(x, y, spellbook_small_pic);
    if (spellbook_mode == 1) {
        if (spellbook_page != 0) {
            Draw_Pic_Frame(x + 13, y + 4, spellbook_corner_left_pic);
        }
        if (spellbook_page != spellbook_page_count && spellbook_page < spellbook_page_count - 2) {
            Draw_Pic_Frame(x + 258, y + 4, spellbook_corner_right_pic);
        }
    }
    Spellbook_Draw_Page_Overland(p_spellbook_pages[spellbook_page], pic);
    Draw_Bitmap(x + 16, y + 21, pic);
    colors[0] = SB_TITLE_COLOR0;
    for (i = 1; i < 5; i++) {
        colors[i] = SB_TITLE_COLOR;
    }
    Font_Select_Palette(4, colors);
    Print_Text_Centered(x + 70, y + 6, p_spellbook_pages[spellbook_page].title);
    Pic_Decode_Frame_To_Bitmap(spellbook_page_bg_pic, pic);
    Spellbook_Draw_Page_Overland(p_spellbook_pages[spellbook_page + 1], pic);
    Draw_Bitmap(x + 148, y + 21, pic);
    colors[0] = SB_TITLE_COLOR0;
    for (i = 1; i < 5; i++) {
        colors[i] = SB_TITLE_COLOR;
    }
    Font_Select_Palette(4, colors);
    Print_Text_Centered(x + 208, y + 6, p_spellbook_pages[spellbook_page + 1].title);
    Heap_Release(p_scratch);
}

/* (x, y, caster): redraws the combat book (pages drawn into ui_temp_pic_a) */
void Spellbook_Combat_Redraw(int x, int y, int caster)
{
    int i;
    char colors[6];

    Draw_Pic_Frame(x, y, spellbook_small_pic);
    if (spellbook_mode == 1) {
        if (spellbook_page != 0) {
            Draw_Pic_Frame(x + 13, y + 4, spellbook_corner_left_pic);
        }
        if (spellbook_page != spellbook_page_count && spellbook_page < spellbook_page_count - 2) {
            Draw_Pic_Frame(x + 258, y + 4, spellbook_corner_right_pic);
        }
    }
    Spellbook_Draw_Page_Combat(p_spellbook_pages[spellbook_page], ui_temp_pic_a, caster);
    Draw_Bitmap(x + 16, y + 21, ui_temp_pic_a);
    colors[0] = SB_TITLE_COLOR0;
    for (i = 1; i < 5; i++) {
        colors[i] = SB_TITLE_COLOR;
    }
    Font_Select_Palette(4, colors);
    Print_Text_Centered(x + 70, y + 6, p_spellbook_pages[spellbook_page].title);
    Pic_Decode_Frame_To_Bitmap(spellbook_page_bg_pic, ui_temp_pic_a);
    Spellbook_Draw_Page_Combat(p_spellbook_pages[spellbook_page + 1], ui_temp_pic_a, caster);
    Draw_Bitmap(x + 148, y + 21, ui_temp_pic_a);
    colors[0] = SB_TITLE_COLOR0;
    for (i = 1; i < 5; i++) {
        colors[i] = SB_TITLE_COLOR;
    }
    Font_Select_Palette(4, colors);
    Print_Text_Centered(x + 208, y + 6, p_spellbook_pages[spellbook_page + 1].title);
}

/* (forward, is_combat, caster): the four-frame page turn (BOOK animation in EMS) - the page being turned is drawn,
   scaled and sheared column by column into a curl; then the book's redraw function is set again */
void Spellbook_Page_Turn_Anim(int forward, int is_combat, int caster)
{
    unsigned curl;
    int page;
    unsigned pic;
    int frame;
    int j;
    char colors[6];
    int x;
    int y;
    int scale[4];

    x = 16;
    y = 10;
    scale[0] = scale[3] = 85;
    scale[1] = scale[2] = 55;
    UI_Suspend_Redraw_Fn();
    Anim_Load_To_EMS("book", 0);
    if (is_combat == 0) {
        Minimap_Alloc_Pic();
    }
    Heap_Mark(p_scratch);
    pic = Heap_Alloc_Back(p_scratch, 0x410);    /* paragraphs: a page bitmap (0x41a in the redraws) */
    curl = Heap_Alloc_Back(p_scratch, 0x578);   /* the page-turn bitmap, 122 x 175 */
    for (frame = 0; frame < 6; frame++) {      /* frame counts the six realm icons here */
        spellbook_realm_icon_pics[frame] = Heap_Alloc_Back(p_scratch, 4);   /* paragraphs: a realm icon bitmap */
        Pic_Decode_Frame_To_Bitmap(spellbook_realm_icon_src_pics[frame], spellbook_realm_icon_pics[frame]);
    }
    Palette_Cycle_Color(PAL_CYCLE_COLOR, 0x27, 0x1f, 0x19, 0x32, 0x2e, 0x2f, 3);
    for (frame = 0; frame < 4; frame++) {
        Set_Draw_Page();
        Screen_Restore_A800();
        if (forward == 1) {
            page = spellbook_page;
        } else {
            page = spellbook_page - 2;
        }
        if (page != 0 && spellbook_mode == 1) {
            Draw_Pic_Frame(x + 13, y + 4, spellbook_corner_left_pic);
        }
        if (is_combat == 0) {
            Spellbook_Draw_Page_Overland(p_spellbook_pages[page], pic);
        } else {
            Spellbook_Draw_Page_Combat(p_spellbook_pages[page], pic, caster);
        }
        Draw_Bitmap(x + 16, y + 21, pic);
        colors[0] = SB_TITLE_COLOR0;
        for (j = 1; j < 5; j++) {
            colors[j] = SB_TITLE_COLOR;
        }
        Font_Select_Palette(4, colors);
        Print_Text_Centered(x + 70, y + 6, p_spellbook_pages[page].title);
        if (forward == 1) {
            page = spellbook_page + 3;
        } else {
            page = spellbook_page + 1;
        }
        if (page < spellbook_page_count && spellbook_mode == 1) {
            Draw_Pic_Frame(x + 258, y + 4, spellbook_corner_right_pic);
        }
        if (is_combat == 0) {
            Spellbook_Draw_Page_Overland(p_spellbook_pages[page], pic);
        } else {
            Spellbook_Draw_Page_Combat(p_spellbook_pages[page], pic, caster);
        }
        Draw_Bitmap(x + 148, y + 21, pic);
        colors[0] = SB_TITLE_COLOR0;
        for (j = 1; j < 5; j++) {
            colors[j] = SB_TITLE_COLOR;
        }
        Font_Select_Palette(4, colors);
        if (p_spell_data[p_spellbook_pages[page - 1].spells[0]].book_section !=
            p_spell_data[p_spellbook_pages[page].spells[0]].book_section) {
            Print_Text_Centered(x + 208, y + 6, p_spellbook_pages[page].title);
        }
        if (forward == 0) {
            Anim_Set_Frame(3 - frame);
        }
        Anim_EMS_Next_Frame();
        if (frame < 2) {
            if (forward == 1) {
                page = spellbook_page + 1;
            } else {
                page = spellbook_page;
            }
        } else {
            if (forward == 1) {
                page = spellbook_page + 2;
            } else {
                page = spellbook_page - 1;
            }
        }
        if (is_combat == 0) {
            Spellbook_Draw_Page_Overland(p_spellbook_pages[page], pic);
        } else {
            Spellbook_Draw_Page_Combat(p_spellbook_pages[page], pic, caster);
        }
        Pic_Create_Blank(122, 175, curl);
        Pic_Draw_Into_Pic(0, 48, curl, pic);
        colors[0] = SB_TITLE_COLOR0;
        for (j = 1; j < 5; j++) {
            colors[j] = SB_TITLE_COLOR;
        }
        Font_Select_Palette(4, colors);
        if (abs(page - spellbook_page) == 2 ||
            p_spell_data[p_spellbook_pages[page - 1].spells[0]].book_section !=
            p_spell_data[p_spellbook_pages[page].spells[0]].book_section) {
            Print_Text_Centered_To_Pic(62, 30, p_spellbook_pages[page].title, curl);
        }
        Pic_Scale_Percent(curl, scale[frame], 100);
        if ((forward == 1 && frame == 0) || (forward == 0 && frame == 3)) {
            Pic_Shear_Columns(0, 0, 3, -4, curl);
            Pic_Shear_Columns(4, -5, 9, -10, curl);
            Pic_Shear_Columns(10, -11, 21, -16, curl);
            Pic_Shear_Columns(22, -16, 32, -19, curl);
            Pic_Shear_Columns(33, -19, 40, -19, curl);
            Pic_Shear_Columns(41, -18, 62, -13, curl);
            Pic_Shear_Columns(63, -12, 101, 5, curl);
            Draw_Bitmap_Clipped(x + 148, y - 28, curl);
        }
        if ((forward == 1 && frame == 1) || (forward == 0 && frame == 2)) {
            Pic_Shear_Columns(0, 0, 5, -8, curl);
            Pic_Shear_Columns(6, -11, 16, -21, curl);
            Pic_Shear_Columns(17, -22, 26, -27, curl);
            Pic_Shear_Columns(27, -27, 38, -29, curl);
            Pic_Shear_Columns(39, -29, 49, -27, curl);
            Pic_Shear_Columns(50, -26, 61, -21, curl);
            Pic_Shear_Columns(62, -20, 70, -12, curl);
            Draw_Bitmap_Clipped(x + 148, y - 30, curl);
        }
        if ((forward == 1 && frame == 2) || (forward == 0 && frame == 1)) {
            Pic_Shear_Columns(0, -16, 9, -25, curl);
            Pic_Shear_Columns(10, -25, 17, -29, curl);
            Pic_Shear_Columns(18, -29, 29, -32, curl);
            Pic_Shear_Columns(30, -33, 40, -30, curl);
            Pic_Shear_Columns(41, -29, 52, -25, curl);
            Pic_Shear_Columns(53, -24, 63, -14, curl);
            Pic_Shear_Columns(64, -12, 70, 0, curl);
            Draw_Bitmap_Clipped(x + 71, y - 28, curl);
        }
        if ((forward == 1 && frame == 3) || (forward == 0 && frame == 0)) {
            Pic_Shear_Columns(0, 0, 31, -16, curl);
            Pic_Shear_Columns(32, -16, 47, -22, curl);
            Pic_Shear_Columns(48, -22, 68, -27, curl);
            Pic_Shear_Columns(69, -27, 82, -23, curl);
            Pic_Shear_Columns(83, -23, 92, -18, curl);
            Pic_Shear_Columns(93, -18, 101, -10, curl);
            Draw_Bitmap_Clipped(x + 35, y - 22, curl);
        }
        Screen_Present();
    }
    Heap_Release(p_scratch);
    if (is_combat == 0) {
        UI_Set_Redraw_Fn(Spellbook_Cast_Redraw, 2);
    } else {
        UI_Set_Redraw_Fn(Combat_Spellbook_Redraw_CB, 2);
    }
}

/* (n, group, title, per_page): puts the next n spells of the list that belong to the group (book section or realm,
   -1 any) on pages of per_page, a new page when one is full (a right-hand page keeps the title only when it starts
   a new group); with per_page != 4 also sets spellbook_page to the slot of the spell being cast (a dead write) */
void Spellbook_Fill_Group(int n, int group, char *title, int per_page)
{
    int k;
    int count;
    int match;

    count = 0;
    strcpy(p_spellbook_pages[spellbook_page_count].title, title);
    for (k = 0; n > 0; k++) {
        match = 0;
        if (game_config.book_ordering == 1) {
            if ((char)p_spell_data[abs(p_spellbook_spells[k])].book_section == group || group == -1) {
                match = 1;
            }
        } else {
            if (p_spell_data[abs(p_spellbook_spells[k])].realm == group || group == -1) {
                match = 1;
            }
        }
        if (match == 1) {
            count++;
            if (per_page + 1 == count) {
                p_spellbook_pages[spellbook_page_count].count = per_page;
                count = 1;
                spellbook_page_count++;
                if (spellbook_page_count % 2 == 1) {
                    if (game_config.book_ordering == 1) {
                        if ((char)p_spell_data[p_spellbook_pages[spellbook_page_count - 1].spells[0]].book_section != group) {
                            strcpy(p_spellbook_pages[spellbook_page_count].title, title);
                        }
                    } else {
                        if (p_spell_data[p_spellbook_pages[spellbook_page_count - 1].spells[0]].realm != group) {
                            strcpy(p_spellbook_pages[spellbook_page_count].title, title);
                        }
                    }
                } else {
                    strcpy(p_spellbook_pages[spellbook_page_count].title, title);
                }
            }
            n--;
            if (per_page != RESEARCH_PAGE_SPELLS && abs(p_spellbook_spells[k]) == wizards[PLAYER_HUMAN].casting_spell) {
                spellbook_page = count;
            }
            p_spellbook_pages[spellbook_page_count].spells[count - 1] = p_spellbook_spells[k];
        }
    }
    p_spellbook_pages[spellbook_page_count].count = count;
    spellbook_page_count++;
}

/* (per_page): the six groups in book order - by section (Summoning, Special Spells, City Spells, Enchantment, Unit
   Spells, Combat Spells) or by realm (Arcane, Chaos, Death, Life, Nature, Sorcery); an odd page count gets an empty
   right-hand page */
void Spellbook_Layout_Groups(int per_page)
{
    if (game_config.book_ordering == 1) {
        if (sb_count_summon_arcane > 0) {
            Spellbook_Fill_Group(sb_count_summon_arcane, BOOK_SECTION_SUMMONING, "Summoning", per_page);
        }
        if (sb_count_special_chaos > 0) {
            Spellbook_Fill_Group(sb_count_special_chaos, BOOK_SECTION_SPECIAL, "Special Spells", per_page);
        }
        if (sb_count_city_death > 0) {
            Spellbook_Fill_Group(sb_count_city_death, BOOK_SECTION_CITY, "City Spells", per_page);
        }
        if (sb_count_enchant_life > 0) {
            Spellbook_Fill_Group(sb_count_enchant_life, BOOK_SECTION_ENCHANTMENT, "Enchantment", per_page);
        }
        if (sb_count_unit_nature > 0) {
            Spellbook_Fill_Group(sb_count_unit_nature, BOOK_SECTION_UNIT, "Unit Spells", per_page);
        }
        if (sb_count_combat_sorcery > 0) {
            Spellbook_Fill_Group(sb_count_combat_sorcery, BOOK_SECTION_COMBAT, "Combat Spells", per_page);
        }
    } else {
        if (sb_count_summon_arcane > 0) {
            Spellbook_Fill_Group(sb_count_summon_arcane, REALM_ARCANE, "Arcane", per_page);
        }
        if (sb_count_special_chaos > 0) {
            Spellbook_Fill_Group(sb_count_special_chaos, REALM_CHAOS, "Chaos", per_page);
        }
        if (sb_count_city_death > 0) {
            Spellbook_Fill_Group(sb_count_city_death, REALM_DEATH, "Death", per_page);
        }
        if (sb_count_enchant_life > 0) {
            Spellbook_Fill_Group(sb_count_enchant_life, REALM_LIFE, "Life", per_page);
        }
        if (sb_count_unit_nature > 0) {
            Spellbook_Fill_Group(sb_count_unit_nature, REALM_NATURE, "Nature", per_page);
        }
        if (sb_count_combat_sorcery > 0) {
            Spellbook_Fill_Group(sb_count_combat_sorcery, REALM_SORCERY, "Sorcery", per_page);
        }
    }
    if (spellbook_page_count % 2 == 1) {
        p_spellbook_pages[spellbook_page_count].count = 0;
        spellbook_page_count++;
    }
}
