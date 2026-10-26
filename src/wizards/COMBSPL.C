/* COMBSPL.C - WIZARDS.EXE overlay code segment 8cf2 (stub 253a): casting a spell in combat - the caster's turn
   (innate spells, the combat spellbook or the AI's choice, extra power, the cost in skill / mana times the range
   multiplier, Counter Magic and the node aura fizzle rolls), the combat spellbook screen, the choice checks, the
   spells' resistance modifiers, the range multiplier and the AI's square / unit targets for Earth to Mud, Disrupt,
   Cracks Call, Word of Recall and Recall Hero.
   This module's data (DS:5af2..5b16): its literals only; no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 8cf2 src/wizards/COMBSPL.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Spell_Resist_Modifier(int spell);
int Combat_Range_Multiplier(int player);
int AI_Target_Earth_To_Mud(int player, int *x, int *y);
int AI_Target_Disrupt(int player, int *x, int *y);
int AI_Target_Cracks_Call(int player, int *x, int *y);
int AI_Target_Word_Of_Recall_Unused(int owner);
int AI_Target_Recall_Hero(int player);
void Combat_Spellbook_Draw_Backdrop(void);
void Combat_Spellbook_Redraw_CB(void);
int Combat_Spell_Action(int caster);
int Screen_Combat_Spellbook(int caster, int *chosen);
int Combat_Spell_Choice_Invalid(int spell);

#include "GAME.H"

extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_site;                      /* DS:9284  0 units, 1 city, 5 lair */
extern int combat_spellbook_page;               /* DS:944c */
extern unsigned spellbook_realm_icon_src_pics[];      /* DS:9454 */
extern unsigned spellbook_small_pic;            /* DS:946e */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern int combat_auto;                         /* DS:c432 */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int combat_battlefield_kind;                /* DS:c520 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int combat_spell_caster;                 /* DS:c8ac */
extern int combat_turns_since_ai_acted;                   /* DS:c8b0 */
extern struct SpellbookPage *p_spellbook_pages; /* DS:c8fa */
extern unsigned spellbook_realm_icon_pics[];          /* DS:c8fe */
extern int spend_per_turn;                      /* DS:c90a  the caster's skill / mana left this turn (SPELLBK.C) */
extern int spellbook_page_count;                /* DS:c914 */
extern int spellbook_mode;                 /* DS:c922 */
extern int spellbook_page;                      /* DS:c924 */
extern unsigned ui_temp_pic_a;                  /* DS:ca34 */
extern int vortex_count;                        /* DS:d152 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
void AI_Combat_Assess_Threats(void);
int AI_Combat_Choose_Spell(int caster);
int AI_Combat_Spell_Target(int spell, int player, int *x, int *y);
void Battlefield_Draw_Terrain(void);
void Combat_Build_Enchant_Icons(void);
void Combat_Cast_Spell(int spell, int target, int caster, int x, int y, int mana, int show_anim);
void Combat_Draw_Screen(void);
int Combat_Extra_Power_Dialog(int spell, int slot, int caster);
int Combat_Spell_Pick_Target_UI(int spell, int *x, int *y);
int Combat_Spellbook_Build(int caster);
int Combat_Unit_Visible_To_Enemy(int bu);
void Demon_Lord_Summon(int unit);
int Dispel_Roll(int strength, int cost, int owner, int realm);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Near_Alloc_Release(void);
void Pic_Apply_Palette(unsigned pic);
void Play_Click_Sound(void);
void Play_Click_Sound_Casting(void);
void Screen_Copy_Front(void);
int Spell_Casting_Cost(int player, int spell, int in_combat);
void Spell_Fizzle_Message(int caster, int blocker, int spell, char *blocker_spell);
void Spellbook_Combat_Redraw(int x, int y, int caster);
void Spellbook_Page_Turn_Anim(int forward, int is_combat, int caster);
void UI_Set_Help(struct HelpEntry *help, int count);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
int Unit_Combat_Value(int bu);
void Warning_Box(char *text);

/* () the combat spellbook's backdrop: the battlefield, the combat screen and the small book */
void Combat_Spellbook_Draw_Backdrop(void)
{
    Battlefield_Draw_Terrain();
    Set_Draw_Page();
    Combat_Draw_Screen();
    Draw_Pic_Frame(16, 10, spellbook_small_pic);
    Screen_Save_A800();
}

/* () redraw callback of the combat spellbook */
void Combat_Spellbook_Redraw_CB(void)
{
    Screen_Restore_A800();
    Spellbook_Combat_Redraw(16, 10, combat_spell_caster);
}

/* (caster: battle unit < 20 or 20 + player) -> 0 no spellbook, 1 nothing cast, 2 cast or fizzled (a Demon
   Lord's summoning returns 1 after summoning, 0 when the unit table is full): the innate spells,
   the spellbook or the AI's choice, extra power, the skill / mana charged (times the range multiplier), the Counter
   Magic and node aura fizzle rolls, then Combat_Cast_Spell. BUG #210: x is the Counter Magic slot and then the range
   multiplier; a wizard's node-fizzled spell is charged its strength, not its cost. The strength of a unit's spell and
   of the AI's variable spells is figured from base_cost - no Evil Omens, no cost reductions (as the extra-power
   dialogs, CASTUI.C); clarified using ReMoM repo
   Numbered: BUG #210, #230, #292 (BUGS_AND_QUIRKS.txt) */
int Combat_Spell_Action(int caster)
{
    int spell;
    int strength;
    int saved;
    int saved_spell;
    int cost;
    int slot;
    int target;
    int x;                                      /* by turns: the Counter Magic slot, the node realm, the range multiplier, the target x */
    int y;
    int owner;
    int n;
    int base;
    int blocker;
    int result;
    int innate;
    int mana;

    result = CAST_RESULT_NONE;
    innate = 0;
    combat_spell_caster = caster;
    if (caster > CASTER_WIZARD - 1) {
        owner = caster - CASTER_WIZARD;
    } else {
        owner = p_battle_units[caster].owner;
        mana = p_battle_units[caster].spell_points;
    }
    if (caster == CASTER_WIZARD + PLAYER_NEUTRAL)
        return result;
    if (owner == PLAYER_HUMAN && combat_auto == 0) {
        spell = SPELL_NONE;
        innate = 0;
        if (caster < CASTER_WIZARD) {
            if (p_battle_units[caster].abilities2 & ABILITY2_DOOMBOLT_SPELL) {
                spell = SPELL_DOOM_BOLT;
                innate = 1;
            }
            if (p_battle_units[caster].abilities2 & ABILITY2_FIRE_BALL_SPELL) {
                spell = SPELL_FIREBALL;
                innate = 1;
            }
            if (p_battle_units[caster].abilities2 & ABILITY2_WEB_SPELL) {
                spell = SPELL_WEB;
                innate = 1;
            }
            if (p_battle_units[caster].abilities2 & ABILITY2_HEALING_SPELL) {
                spell = SPELL_HEALING;
                innate = 1;
            }
            if (p_battle_units[caster].abilities & ABILITY_SUMMON_DEMONS) {
                if (num_units < MAX_UNITS) {
                    Demon_Lord_Summon(caster);
                    p_battle_units[caster].moves = 0;
                    return CAST_RESULT_NONE;
                } else {
                    return CAST_RESULT_UNABLE;
                }
            }
        }
        if (spell == SPELL_NONE) {
            saved = Combat_Spellbook_Build(caster);
            if (saved == 0)
                return CAST_RESULT_UNABLE;
            do {
                Combat_Spellbook_Draw_Backdrop();
                spell = Screen_Combat_Spellbook(caster, &slot);
                if (spell <= 0 || p_spell_data[spell].effect_type < SPELL_FX_FIRST_VARIABLE) {
                    present_mode = PRESENT_DISSOLVE;
                    Combat_Spellbook_Draw_Backdrop();
                }
            } while (Combat_Spell_Choice_Invalid(spell));
            if (p_spell_data[spell].effect_type < SPELL_FX_FIRST_VARIABLE || innate == 1) {
                Battlefield_Draw_Terrain();
                Set_Draw_Page();
                Combat_Draw_Screen();
                Screen_Present();
            }
        }
    } else {
        AI_Combat_Assess_Threats();
        if (caster < CASTER_WIZARD && p_battle_units[caster].item_charges > 0 && p_battle_units[caster].action_state == AI_ACTION_ITEM_SPELL)
            /* the spell-charges item is always in the hero's item slot 0 */
            spell = (unsigned char)p_items[wizards[owner].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[0]].spell;
        else
            spell = AI_Combat_Choose_Spell(caster);
        if (spell > 0 && caster < CASTER_WIZARD) {
            p_battle_units[caster].target_unit = caster;   /* the caster becomes its own target (purpose not established) */
            if (p_battle_units[caster].owner != PLAYER_HUMAN)
                combat_turns_since_ai_acted = -1;
        }
    }
    if (spell != SPELL_NONE) {
        if (p_spell_data[spell].effect_type >= SPELL_FX_FIRST_VARIABLE && innate == 0) {
            if (owner == PLAYER_HUMAN && combat_auto == 0) {
                saved = wizards[PLAYER_HUMAN].casting_cost_left;
                saved_spell = wizards[PLAYER_HUMAN].casting_spell;
                wizards[PLAYER_HUMAN].casting_spell = spell;
                if (caster > CASTER_WIZARD - 1)
                    wizards[PLAYER_HUMAN].casting_cost_left = Spell_Casting_Cost(PLAYER_HUMAN, spell, 1);
                else
                    wizards[PLAYER_HUMAN].casting_cost_left = p_spell_data[spell].base_cost;
                strength = Combat_Extra_Power_Dialog(spell, slot, caster);
                if (caster > CASTER_WIZARD - 1)
                    cost = strength - (int)((long)strength * Spell_Cost_Reduction_Pct(owner, spell) / 100);
                else
                    cost = strength;
                wizards[PLAYER_HUMAN].casting_cost_left = saved;
                wizards[PLAYER_HUMAN].casting_spell = saved_spell;
                Battlefield_Draw_Terrain();
                Set_Draw_Page();
                Combat_Draw_Screen();
                Screen_Present();
            } else {
                if (caster > CASTER_WIZARD - 1) {
                    strength = wizards[owner].combat_skill_left;
                    if (strength > p_spell_data[spell].base_cost * 4)
                        strength = p_spell_data[spell].base_cost * 4;
                    n = Combat_Range_Multiplier(owner);
                    if ((int)((long)wizards[owner].mana_reserve * 10 / n) < strength + p_spell_data[spell].base_cost)
                        strength = (int)((long)wizards[owner].mana_reserve * 10 / n) - p_spell_data[spell].base_cost;
                } else {
                    strength = mana - p_spell_data[spell].base_cost;
                    if (strength > p_spell_data[spell].base_cost * 4)
                        strength = p_spell_data[spell].base_cost * 4;
                }
                base = p_spell_data[spell].base_cost;
                n = 0;
                if (spell == SPELL_LIFE_DRAIN || spell == SPELL_BANISH)
                    n = (Random(strength / 5 + 1) - 1) * 5;
                else if (spell == SPELL_COUNTER_MAGIC)
                    n = (Random(strength / 5 + 1) - 1) * 5;
                else
                    n = strength;
                strength = base + n;
                if (caster > CASTER_WIZARD - 1)
                    cost = strength - (int)((long)strength * Spell_Cost_Reduction_Pct(owner, spell) / 100);
                else
                    cost = strength;
            }
        } else {
            if (caster > CASTER_WIZARD - 1)
                strength = Spell_Casting_Cost(owner, spell, 1);
            else
                strength = p_spell_data[spell].base_cost;
            cost = strength;
        }
        if (spell != SPELL_WEB || innate != 1) {
            if (owner == combat_attacker_player) {
                x = CENCH_COUNTER_MAGIC_DEF;   /* the other side's Counter Magic */
                blocker = combat_defender_player;
            } else {
                x = CENCH_COUNTER_MAGIC_ATT;
                blocker = combat_attacker_player;
            }
            if (p_combat_enchants[x] > 0) {
                if (Dispel_Roll(p_combat_enchants[x], strength, owner, p_spell_data[spell].realm)) {
                    Set_Draw_Page();
                    Combat_Draw_Screen();
                    Screen_Present();
                    Screen_Copy_Front();
                    Spell_Fizzle_Message(caster, blocker, spell, "Counter Magic");
                    if (caster > CASTER_WIZARD - 1) {
                        x = Combat_Range_Multiplier(caster - CASTER_WIZARD);   /* x: the range multiplier */
                        wizards[caster - CASTER_WIZARD].combat_skill_left -= cost;
                        wizards[caster - CASTER_WIZARD].mana_reserve -= cost * x / 10;
                    } else {
                        if (innate == 1) {
                            if (spell == SPELL_DOOM_BOLT)
                                p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_DOOMBOLT_SPELL;
                            if (spell == SPELL_FIREBALL)
                                p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_FIRE_BALL_SPELL;
                            if (spell == SPELL_HEALING)
                                p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_HEALING_SPELL;
                        } else {
                            if (p_battle_units[caster].item_charges > 0 &&
                                (unsigned char)p_items[wizards[owner].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[0]].spell == spell)
                                p_battle_units[caster].item_charges = p_battle_units[caster].item_charges - 1;
                            else
                                mana -= cost;
                            if (mana < 0)
                                mana = 0;
                        }
                        p_battle_units[caster].moves = 0;
                    }
                    result = CAST_RESULT_CAST;
                    spell = -1;
                }
                p_combat_enchants[x] = p_combat_enchants[x] - 5;
                if (p_combat_enchants[x] < 0) {
                    p_combat_enchants[x] = 0;
                    Combat_Build_Enchant_Icons();
                }
            }
            if (p_battlefield->backdrop == BACKDROP_SORCERY_NODE)   /* x: the node's realm */
                x = REALM_SORCERY;
            else if (p_battlefield->backdrop == BACKDROP_CHAOS_NODE)
                x = REALM_CHAOS;
            else if (p_battlefield->backdrop == BACKDROP_NATURE_NODE)
                x = REALM_NATURE;
            else
                x = -1;
            if (owner < num_players && wizards[owner].retorts[RETORT_NODE_MASTERY] > 0)
                x = -1;
            if (x > -1 && spell > -1 && p_spell_data[spell].realm != x &&
                Dispel_Roll(50, strength, owner, p_spell_data[spell].realm)) {   /* 50: the node aura's fizzle strength */
                Set_Draw_Page();
                Combat_Draw_Screen();
                Screen_Present();
                Screen_Copy_Front();
                Spell_Fizzle_Message(caster, PLAYER_NEUTRAL, spell, "");   /* the node fizzled it */
                if (caster > CASTER_WIZARD - 1) {
                    x = Combat_Range_Multiplier(caster - CASTER_WIZARD);   /* x: the range multiplier */
                    wizards[caster - CASTER_WIZARD].combat_skill_left -= strength;
                    wizards[caster - CASTER_WIZARD].mana_reserve -= strength * x / 10;
                } else {
                    if (innate == 1) {
                        if (spell == SPELL_DOOM_BOLT)
                            p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_DOOMBOLT_SPELL;
                        if (spell == SPELL_FIREBALL)
                            p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_FIRE_BALL_SPELL;
                        if (spell == SPELL_HEALING)
                            p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_HEALING_SPELL;
                    } else {
                        if (p_battle_units[caster].item_charges > 0 &&
                            (unsigned char)p_items[wizards[owner].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[0]].spell == spell)
                            p_battle_units[caster].item_charges = p_battle_units[caster].item_charges - 1;
                        else
                            mana -= cost;
                        if (mana < 0)
                            mana = 0;
                    }
                    p_battle_units[caster].moves = 0;
                }
                result = CAST_RESULT_CAST;
                spell = SPELL_NONE;
            }
        }
    }
    if (spell > 0) {
        if (p_spell_data[spell].effect_type != SPELL_FX_COMBAT_ENCH && p_spell_data[spell].effect_type != SPELL_FX_COMBAT_ENCH_VAR &&
            p_spell_data[spell].effect_type != SPELL_FX_DISENCHANT && p_spell_data[spell].effect_type != SPELL_FX_CITY_ENCH &&
            spell != SPELL_ANIMATE_DEAD) {
            if (owner == PLAYER_HUMAN && combat_auto == 0) {
                target = Combat_Spell_Pick_Target_UI(spell, &x, &y);
            } else {
                target = AI_Combat_Spell_Target(spell, owner, &x, &y);
                if (target != TARGET_SQUARE) {
                    x = p_battle_units[target].combat_x;   /* x: the target's square */
                    y = p_battle_units[target].combat_y;
                }
            }
        } else {
            target = TARGET_SQUARE;
            x = 0;
            y = 0;
        }
        if (target != TARGET_CANCELLED) {
            Combat_Cast_Spell(spell, target, caster, x, y, strength, 1);
            result = CAST_RESULT_CAST;
            if (caster > CASTER_WIZARD - 1) {
                x = Combat_Range_Multiplier(caster - CASTER_WIZARD);   /* x: the range multiplier */
                wizards[caster - CASTER_WIZARD].combat_skill_left -= cost;
                wizards[caster - CASTER_WIZARD].mana_reserve -= cost * x / 10;
            } else {
                if (innate == 1) {
                    if (spell == SPELL_DOOM_BOLT)
                        p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_DOOMBOLT_SPELL;
                    if (spell == SPELL_WEB)
                        p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_WEB_SPELL;
                    if (spell == SPELL_FIREBALL)
                        p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_FIRE_BALL_SPELL;
                    if (spell == SPELL_HEALING)
                        p_battle_units[caster].abilities2 = p_battle_units[caster].abilities2 ^ ABILITY2_HEALING_SPELL;
                } else {
                    if (p_battle_units[caster].item_charges > 0 &&
                        (unsigned char)p_items[wizards[owner].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[0]].spell == spell)
                        p_battle_units[caster].item_charges = p_battle_units[caster].item_charges - 1;
                    else
                        mana -= cost;
                    if (mana < 0)
                        mana = 0;
                }
                p_battle_units[caster].moves = 0;
            }
        }
    }
    if (caster < CASTER_WIZARD) {
        p_battle_units[caster].spell_points = mana;
        if (mana < 3 && p_battle_units[caster].ammo == 0) {   /* 3 points: one magic shot */
            p_battle_units[caster].ranged = 0;
            p_battle_units[caster].ranged_type = RANGED_NONE;
        }
    }
    return result;
}

/* (caster, &chosen) -> the chosen spell (0 cancelled), *chosen its slot on the two pages (0 .. 11, the
   right page 6 .. 11; not written on a cancel): the combat spellbook - two pages of 6 spells, F / B turn
   them, Esc cancels (only once the controls are rebuilt: the first set gives the cancel button no hotkey); a choice is refused (-1) when it cannot be cast here (a summon on water for a non-flyer, a
   city-only spell, more than the turn's casting skill) */
int Screen_Combat_Spellbook(int caster, int *chosen)
{
    int i;
    int base_x;
    int done;
    int input;
    int top;
    int cancel_ctl;
    int chosen_spell;
    int chosen_slot;
    int next_ctl;
    int prev_ctl;
    int cost;
    int right_ctls[6];
    int left_ctls[6];

    Pic_Apply_Palette(spellbook_small_pic);
    Palette_Cycle_Reset();
    Heap_Mark(p_scratch);
    ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 0x442);
    for (i = 0; i < 6; i++) {
        spellbook_realm_icon_pics[i] = Heap_Alloc_Back(p_scratch, 4);
        Pic_Decode_Frame_To_Bitmap(spellbook_realm_icon_src_pics[i], spellbook_realm_icon_pics[i]);
    }
    spellbook_mode = SPELLBOOK_COMBAT;
    spellbook_page = combat_spellbook_page;
    UI_Set_Redraw_Fn(Combat_Spellbook_Redraw_CB, 2);
    LBX_Load_Records_Near("hlpentry", HLPENTRY_SPELLBOOK, help_entries, 0, 15, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 15);
    Set_Text_Shadow_Color(0);
    Font_Select_Shadow(0, 3, 0, 0);
    UI_Clear_Controls();
    base_x = 16;
    top = 12;
    for (i = 0; i < 6; i++)
        left_ctls[i] = UI_Add_Region(base_x + 16, top + i * 22 + 17, base_x + 137, top + i * 22 + 34, "", -1);
    for (i = 0; i < 6; i++)
        right_ctls[i] = UI_Add_Region(base_x + 148, top + i * 22 + 17, base_x + 268, top + i * 22 + 34, "", -1);
    cancel_ctl = UI_Add_Region(base_x + 159, top + 154, base_x + 177, top + 183, UI_NO_HOTKEY, -1);
    next_ctl = UI_Add_Region(base_x + 259, top + 2, base_x + 272, top + 15, "F", -1);
    prev_ctl = UI_Add_Region(base_x + 13, top + 2, base_x + 26, top + 14, "B", -1);
    done = 0;
    while (done == 0) {
        input = UI_Get_Input();
        if (input == cancel_ctl) {
            Play_Click_Sound_Casting();
            done = 1;
            chosen_spell = 0;
        }
        for (i = 0; i < 6; i++) {
            if (i < p_spellbook_pages[spellbook_page].count)
                help_entries[i + 3].id = abs(p_spellbook_pages[spellbook_page].spells[i]);
            else
                help_entries[i + 3].id = -1;
        }
        for (i = 0; i < 6; i++) {
            if (i < p_spellbook_pages[spellbook_page + 1].count)
                help_entries[i + 9].id = abs(p_spellbook_pages[spellbook_page + 1].spells[i]);
            else
                help_entries[i + 9].id = -1;
        }
        if (input == next_ctl && spellbook_page < spellbook_page_count - 2) {
            Heap_Release(p_scratch);
            Play_Click_Sound_Casting();
            Spellbook_Page_Turn_Anim(1, 1, caster);
            spellbook_page += 2;
            Heap_Mark(p_scratch);
            ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 0x442);
            for (i = 0; i < 6; i++) {
                spellbook_realm_icon_pics[i] = Heap_Alloc_Back(p_scratch, 4);
                Pic_Decode_Frame_To_Bitmap(spellbook_realm_icon_src_pics[i], spellbook_realm_icon_pics[i]);
            }
        }
        if (input == prev_ctl && spellbook_page > 1) {
            Heap_Release(p_scratch);
            Play_Click_Sound_Casting();
            Spellbook_Page_Turn_Anim(0, 1, caster);
            spellbook_page -= 2;
            Heap_Mark(p_scratch);
            ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 0x442);
            for (i = 0; i < 6; i++) {
                spellbook_realm_icon_pics[i] = Heap_Alloc_Back(p_scratch, 4);
                Pic_Decode_Frame_To_Bitmap(spellbook_realm_icon_src_pics[i], spellbook_realm_icon_pics[i]);
            }
        }
        /* 12 entries through left_ctls: it runs on into right_ctls, the array the compiler places after
           it (declared before it) - the original reads both lists this way */
        for (i = 0; i < 12; i++) {
            if (left_ctls[i] == input) {
                if (i < 6) {
                    if (i < p_spellbook_pages[spellbook_page].count) {
                        Play_Click_Sound();
                        chosen_slot = i;
                        chosen_spell = p_spellbook_pages[spellbook_page].spells[i];
                    } else {
                        chosen_spell = -1;
                    }
                } else {
                    if (i - 6 < p_spellbook_pages[spellbook_page + 1].count) {
                        Play_Click_Sound();
                        chosen_spell = p_spellbook_pages[spellbook_page + 1].spells[i - 6];
                        chosen_slot = i;
                    } else {
                        chosen_spell = -1;
                    }
                }
                if (caster < 19)                /* BUG #436: 19, not CASTER_WIZARD */
                    cost = p_spell_data[abs(chosen_spell)].base_cost;
                else
                    cost = Spell_Casting_Cost(PLAYER_HUMAN, abs(chosen_spell), 1);
                if (p_spell_data[abs(chosen_spell)].effect_type == SPELL_FX_SUMMON && combat_battlefield_kind == BATTLEFIELD_WATER &&
                    (unit_types[(int)p_spell_data[abs(chosen_spell)].effect_param].move_flags & MOVE_FLYING) == 0 &&
                    (unit_types[(int)p_spell_data[abs(chosen_spell)].effect_param].ability_flags & AFLAG_NON_CORPOREAL) == 0)
                    chosen_spell = -1;
                if ((p_spell_data[abs(chosen_spell)].effect_type == SPELL_FX_CITY_ENCH || abs(chosen_spell) == SPELL_WALL_OF_STONE) &&
                    (combat_attacker_player == PLAYER_HUMAN || combat_site != COMBAT_AT_CITY))
                    chosen_spell = -1;
                if (cost > spend_per_turn &&
                    (caster > CASTER_WIZARD - 1 ||
                     (unsigned char)p_items[wizards[p_battle_units[caster].owner].hired_heroes[p_units[p_battle_units[caster].unit_index].hero_slot].item_index[0]].spell != chosen_spell))
                    chosen_spell = -1;
                if (chosen_spell > -1)
                    done = 1;
            }
        }
        if (done == 0) {
            UI_Clear_Controls();
            base_x = 16;
            top = 12;
            for (i = 0; i < 6; i++)
                left_ctls[i] = UI_Add_Region(base_x + 16, top + i * 22 + 17, base_x + 137, top + i * 22 + 34, "", -1);
            for (i = 0; i < 6; i++)
                right_ctls[i] = UI_Add_Region(base_x + 148, top + i * 22 + 17, base_x + 268, top + i * 22 + 34, "", -1);
            cancel_ctl = UI_Add_Region(base_x + 159, top + 154, base_x + 177, top + 183, "\x1b", -1);
            next_ctl = UI_Add_Region(base_x + 259, top + 2, base_x + 272, top + 15, "F", -1);
            prev_ctl = UI_Add_Region(base_x + 13, top + 2, base_x + 26, top + 14, "B", -1);
            Set_Draw_Page();
            Combat_Spellbook_Redraw_CB();
            Screen_Present();
        }
    }
    combat_spellbook_page = spellbook_page;
    UI_Suspend_Redraw_Fn();
    UI_Clear_Help();
    Near_Alloc_Release();
    Heap_Release(p_scratch);
    UI_Clear_Controls();
    *chosen = chosen_slot;
    Battlefield_Draw_Terrain();
    return chosen_spell;
}

/* (spell) -> 1: the human's choice is refused (the vortex limit, Word of Recall / Recall Hero while Spell of Return
   is being cast, no target for the spell, the combat enchantment already up, 9 own units or 1000 units for a
   summon), with a message. Mass Healing is refused while True Light is up (both read combat-enchant slot 0: BUG #14).
   BUG #448: the target tests ignore the unit's state - Star Fires and the normal-unit spells count any unit, Animate / Raise
   Dead any status above 0 (recalled, fled, set aside too) -, so the spell can pass here and find no target; and only
   summons are held to 9 own units, not Possession or Creature Binding, which also add a unit to the side.
   clarified using ReMoM repo
   Numbered: BUG #288 (BUGS_AND_QUIRKS.txt) */
int Combat_Spell_Choice_Invalid(int spell)
{
    int i;
    int found;
    int invalid;
    int own;

    invalid = 0;
    if (spell == 666)                           /* no caller passes it (the value of PAGE_TURN_TO_RESEARCH / SIDE_BOTH) */
        return 1;
    if (spell <= 0)
        return 0;
    if (spell == SPELL_MAGIC_VORTEX && vortex_count == MAX_VORTICES) {
        LBX_Load_Records_Near("message", 0, message_text, 76, 1, 150);
        Warning_Box(message_text);
        invalid = 1;
    }
    if (spell == SPELL_WORD_OF_RECALL && wizards[PLAYER_HUMAN].casting_spell == SPELL_SPELL_OF_RETURN) {
        LBX_Load_Records_Near("message", 0, message_text, 85, 1, 150);
        Warning_Box(message_text);
        invalid = 1;
    }
    if (spell == SPELL_RECALL_HERO) {
        found = 0;
        if (wizards[PLAYER_HUMAN].casting_spell == SPELL_SPELL_OF_RETURN) {
            LBX_Load_Records_Near("message", 0, message_text, 85, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        } else if (combat_loc_x == wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_X] && combat_loc_y == wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_Y] &&
                   combat_loc_plane == wizards[PLAYER_HUMAN].summoning_circle_xyp[XYP_PLANE]) {
            LBX_Load_Records_Near("message", 0, message_text, 77, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        } else {
            for (i = 0; i < battle_unit_count && found == 0; i++) {
                if (p_battle_units[i].owner == PLAYER_HUMAN && p_units[p_battle_units[i].unit_index].hero_slot > -1)
                    found = 1;
            }
            if (found == 0) {
                LBX_Load_Records_Near("message", 0, message_text, 78, 1, 150);
                Warning_Box(message_text);
                invalid = 1;
            }
        }
    }
    if (spell == SPELL_STAR_FIRES) {
        found = 0;
        for (i = 0; i < battle_unit_count && found == 0; i++) {
            if (p_battle_units[i].owner != PLAYER_HUMAN && (p_battle_units[i].race == RACE_DEATH_CREATURE || p_battle_units[i].race == RACE_CHAOS_CREATURE))
                found = 1;
        }
        if (found == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 79, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        }
    }
    if (spell == SPELL_ANIMATE_DEAD) {
        found = 0;
        own = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == PLAYER_HUMAN)
                own++;
            if (p_battle_units[i].status > 0 && p_battle_units[i].status != BATTLE_UNIT_DESTROYED && p_battle_units[i].race != RACE_DEATH_CREATURE &&
                ((p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY) == 0 || p_battle_units[i].owner == PLAYER_HUMAN) &&
                p_units[p_battle_units[i].unit_index].hero_slot == -1 && p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY)
                found = 1;
        }
        if (found == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 80, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        } else if (own == MAX_STACK) {
            LBX_Load_Records_Near("message", 0, message_text, 84, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        }
    }
    if (spell == SPELL_RAISE_DEAD) {
        found = 0;
        own = 0;
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].owner == PLAYER_HUMAN)
                own++;
            if (p_battle_units[i].status > 0 && p_battle_units[i].owner == PLAYER_HUMAN && (char)p_battle_units[i].race < RACE_ARCANE_CREATURE &&
                p_battle_units[i].status != BATTLE_UNIT_DESTROYED && p_units[p_battle_units[i].unit_index].plane != PLANE_COMBAT_ONLY)
                found = 1;
        }
        if (found == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 81, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        } else if (own == MAX_STACK) {
            LBX_Load_Records_Near("message", 0, message_text, 84, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        }
    }
    if ((p_spell_data[spell].effect_type == SPELL_FX_COMBAT_ENCH || p_spell_data[spell].effect_type == SPELL_FX_COMBAT_ENCH_VAR) && spell != SPELL_HOLY_WORD &&
        spell != SPELL_DEATH_SPELL && spell != SPELL_FLAME_STRIKE && spell != SPELL_CALL_CHAOS) {
        if (combat_attacker_player == PLAYER_HUMAN)   /* found reused: the side's offset in the enchantment pair */
            found = 0;
        else
            found = 1;
        if (p_combat_enchants[(int)p_spell_data[spell].effect_param + found] > 0) {
            LBX_Load_Records_Near("message", 0, message_text, 82, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        }
    }
    if (p_spell_data[spell].effect_type == SPELL_FX_NORMAL_UNIT_ENCH) {
        found = 0;
        for (i = 0; i < battle_unit_count && found == 0; i++) {
            if (p_battle_units[i].owner == PLAYER_HUMAN && (char)p_battle_units[i].race < RACE_ARCANE_CREATURE)
                found = 1;
        }
        if (found == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 83, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        }
    }
    if (p_spell_data[spell].effect_type == SPELL_FX_NORMAL_UNIT_CURSE) {
        found = 0;
        for (i = 0; i < battle_unit_count && found == 0; i++) {
            if (p_battle_units[i].owner != PLAYER_HUMAN && (char)p_battle_units[i].race < RACE_ARCANE_CREATURE)
                found = 1;
        }
        if (found == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 83, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        }
    }
    if (p_spell_data[spell].effect_type == SPELL_FX_SUMMON) {
        if (num_units == MAX_UNITS) {
            LBX_Load_Records_Near("message", 0, message_text, 50, 1, 150);
            Warning_Box(message_text);
            invalid = 1;
        } else {
            own = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].owner == PLAYER_HUMAN && p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
                    own++;
            }
            if (own == MAX_STACK) {
                LBX_Load_Records_Near("message", 0, message_text, 84, 1, 150);
                Warning_Box(message_text);
                invalid = 1;
            }
        }
    }
    return invalid;
}

/* (spell) -> the resistance roll modifier of the spell's target */
int Spell_Resist_Modifier(int spell)
{
    int mod;

    mod = 0;
    if (spell == SPELL_SHATTER)
        mod = 0;
    if (spell == SPELL_WARP_CREATURE)
        mod = -1;
    if (spell == SPELL_WEAKNESS)
        mod = -2;
    if (spell == SPELL_BLACK_SLEEP)
        mod = -2;
    if (spell == SPELL_POSSESSION)
        mod = -1;
    if (spell == SPELL_BLACK_WIND)
        mod = -1;
    if (spell == SPELL_TERROR)
        mod = 1;
    if (spell == SPELL_WRACK)
        mod = 1;
    if (spell == SPELL_WORD_OF_DEATH)
        mod = -5;
    if (spell == SPELL_DEATH_SPELL)
        mod = -2;
    if (spell == SPELL_DEATH_WISH)
        mod = 0;
    if (spell == SPELL_DISPEL_EVIL)
        mod = -4;
    if (spell == SPELL_HOLY_WORD)
        mod = -2;
    if (spell == SPELL_PETRIFY)
        mod = 0;
    if (spell == SPELL_VERTIGO)
        mod = 0;
    if (spell == SPELL_BANISH)
        mod = -3;
    if (spell == SPELL_CONFUSION)
        mod = -4;
    if (spell == SPELL_STASIS)
        mod = -5;
    if (spell == SPELL_CREATURE_BINDING)
        mod = -2;
    if (spell == SPELL_GREAT_UNSUMMONING)
        mod = -3;
    return mod;
}

/* (player) -> the combat casting cost multiplier in tenths: 1 for player 5; by the distance to the player's
   fortress (another plane 30; Channeler: 2), 10 + 5 per step over 5, 10, 15, 20; at the fortress 5 */
int Combat_Range_Multiplier(int player)
{
    int dx;
    int dist;
    int dy;

    if (player == PLAYER_NEUTRAL)
        return 1;
    if (p_fortresses[player].plane != combat_loc_plane) {
        dist = 30;
    } else {
        dy = abs(p_fortresses[player].y - combat_loc_y);
        dx = abs(p_fortresses[player].x - combat_loc_x);
        if (dx > 30)
            dx = 60 - dx;
        if (dx < dy)
            dist = dy;
        else
            dist = dx;
    }
    if (dist == 0) {
        return 5;
    } else {
        if (wizards[player].retorts[RETORT_CHANNELER] > 0)
            dist = 2;
        dx = 10;
        if (dist > 5)
            dx += 5;
        if (dist > 10)
            dx += 5;
        if (dist > 15)
            dx += 5;
        if (dist > 20)
            dx += 5;
    }
    return dx;
}

/* (player, &x, &y) -> TARGET_SQUARE and the square with the best score (3 - distance for each enemy ground
   unit within 2, minus the same for its own), -1 none;
   never on water battlefields */
int AI_Target_Earth_To_Mud(int player, int *x, int *y)
{
    int i;
    int score;
    int dx;
    int dy;
    int result;
    int tx;
    int ty;
    int best;

    result = -1;
    best = 0;
    if (combat_battlefield_kind == BATTLEFIELD_WATER)
        return -1;
    for (tx = 0; tx < 21; tx++) {
        for (ty = 0; ty < 22; ty++) {
            score = 0;
            for (i = 0; i < battle_unit_count; i++) {
                if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                    (p_battle_units[i].move_flags & MOVE_FLYING) == 0 && (p_battle_units[i].move_flags & MOVE_MERGING) == 0 &&
                    (p_battle_units[i].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[i].ability_flags & AFLAG_NON_CORPOREAL) == 0) {
                    dx = abs(tx - p_battle_units[i].combat_x);
                    dy = abs(ty - p_battle_units[i].combat_y);
                    if (dx > dy) {
                        if (dx <= 2)
                            score = score + 3 - dx;
                    } else {
                        if (dy <= 2)
                            score = score + 3 - dy;
                    }
                }
                if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
                    (p_battle_units[i].move_flags & MOVE_FLYING) == 0 && (p_battle_units[i].move_flags & MOVE_MERGING) == 0 &&
                    (p_battle_units[i].move_flags & MOVE_TELEPORTING) == 0 && (p_battle_units[i].ability_flags & AFLAG_NON_CORPOREAL) == 0) {
                    dx = abs(tx - p_battle_units[i].combat_x);
                    dy = abs(ty - p_battle_units[i].combat_y);
                    if (dx > dy) {
                        if (dx <= 2)
                            score = score - (3 - dx);
                    } else {
                        if (dy <= 2)
                            score = score - (3 - dy);
                    }
                }
            }
            if (score > best) {
                best = score;
                *x = tx;
                *y = ty;
                result = TARGET_SQUARE;
            }
        }
    }
    return result;
}

/* (player, &x, &y) -> TARGET_SQUARE and a standing city wall square for the attacker, -1 none. BUG #278: wall_segments
   is indexed by the battlefield y / x without the -10 / -5 (both read past the 4 x 4 array) */
int AI_Target_Disrupt(int player, int *x, int *y)
{
    int result;

    result = -1;
    if (combat_defender_player != player && p_battlefield->has_city_walls == 1) {
        if (p_battlefield->wall_segments[13][8] == WALL_STANDING) {
            *x = 8;
            *y = 13;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[11][8] == WALL_STANDING) {
            *x = 8;
            *y = 11;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[10][8] == WALL_STANDING) {
            *x = 8;
            *y = 10;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[13][7] == WALL_STANDING) {
            *x = 7;
            *y = 13;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[10][7] == WALL_STANDING) {
            *x = 7;
            *y = 10;
            result = TARGET_SQUARE;
        }
    }
    return result;
}

/* (player, &x, &y) -> the most valuable targetable enemy ground unit (+30 on a wall square), or for the attacker
   without one a standing wall square (99); -1 none. BUG #278: wall_segments indexed as in AI_Target_Disrupt */
int AI_Target_Cracks_Call(int player, int *x, int *y)
{
    int i;
    int best;
    int result;
    int unit_value;

    best = 0;
    result = -1;
    for (i = 0; i < battle_unit_count; i++) {
        unit_value = -1;
        if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (p_battle_units[i].move_flags & MOVE_FLYING) == 0 &&
            (p_battle_units[i].move_flags & MOVE_MERGING) == 0 && (p_battle_units[i].ability_flags & AFLAG_NON_CORPOREAL) == 0) {
            if (!Combat_Unit_Visible_To_Enemy(i))
                continue;
            unit_value = Unit_Combat_Value(i);
            if (combat_defender_player != player && p_battlefield->has_city_walls == 1 &&
                p_battle_units[i].combat_x >= 5 && p_battle_units[i].combat_x <= 7 &&
                p_battle_units[i].combat_y >= 11 && p_battle_units[i].combat_y <= 13 &&
                p_battlefield->wall_segments[p_battle_units[i].combat_y][p_battle_units[i].combat_x] == WALL_STANDING)
                unit_value += 30;
        }
        if (unit_value > best) {
            best = unit_value;
            result = i;
            *x = p_battle_units[i].combat_x;
            *y = p_battle_units[i].combat_y;
        }
    }
    if (combat_defender_player != player && p_battlefield->has_city_walls == 1 && result == -1) {
        if (p_battlefield->wall_segments[13][8] == WALL_STANDING) {
            *x = 8;
            *y = 13;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[11][8] == WALL_STANDING) {
            *x = 8;
            *y = 11;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[10][8] == WALL_STANDING) {
            *x = 8;
            *y = 10;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[13][7] == WALL_STANDING) {
            *x = 7;
            *y = 13;
            result = TARGET_SQUARE;
        } else if (p_battlefield->wall_segments[10][7] == WALL_STANDING) {
            *x = 7;
            *y = 10;
            result = TARGET_SQUARE;
        }
    }
    return result;
}

/* (owner) -> the own unit of cost >= 200 with the highest value scaled up by its lost hits (unused)
   role name corrected to ReMoM's reading (10/17 audit); clarified using ReMoM repo
   Numbered: BUG #115 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Word_Of_Recall_Unused(int owner)
{
    int i;
    int unit_value;
    int best;
    int result;
    int full;
    int cur;

    best = 0;
    result = -1;
    for (i = 0; i < battle_unit_count; i++) {
        unit_value = -1;
        if (p_battle_units[i].owner == owner && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && p_battle_units[i].cost >= 200) {
            unit_value = Unit_Combat_Value(i);
            full = p_battle_units[i].figures * p_battle_units[i].hits_per_figure;
            cur = p_battle_units[i].figures_alive * p_battle_units[i].hits_per_figure - p_battle_units[i].front_damage;
            if (cur != 0)
                unit_value = unit_value * full / cur;
            if (unit_value > best) {
                best = unit_value;
                result = i;
            }
        }
    }
    return result;
}

/* (player) -> the own hero with the highest value scaled up by his lost hits (Recall Hero), -1 none or when the
   battle is at the player's fortress
   Numbered: BUG #288 (BUGS_AND_QUIRKS.txt) */
int AI_Target_Recall_Hero(int player)
{
    int i;
    int unit_value;
    int best;
    int result;
    int full;
    int cur;

    best = 0;
    result = -1;
    if (p_fortresses[player].x == combat_loc_x && p_fortresses[player].y == combat_loc_y &&
        p_fortresses[player].plane == combat_loc_plane)
        return result;
    for (i = 0; i < battle_unit_count; i++) {
        unit_value = -1;
        if (p_battle_units[i].owner == player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE &&
            p_units[p_battle_units[i].unit_index].hero_slot > -1) {
            unit_value = Unit_Combat_Value(i);
            full = p_battle_units[i].figures * p_battle_units[i].hits_per_figure;
            cur = p_battle_units[i].figures_alive * p_battle_units[i].hits_per_figure - p_battle_units[i].front_damage;
            if (cur != 0)
                unit_value = unit_value * full / cur;   /* BUG #449: a 16-bit product - over 32767 it wraps (negative: the
                                                   hero is skipped); clarified using ReMoM repo */
            if (unit_value > best) {
                best = unit_value;
                result = i;
            }
        }
    }
    return result;
}
