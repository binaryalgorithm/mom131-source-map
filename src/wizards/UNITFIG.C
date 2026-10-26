/* UNITFIG.C - WIZARDS.EXE overlay code segment 7504 (stub 24c3): a unit's figures and enchantments in the unit view -
   the battle-unit record of a unit type (production preview), the view's buffers, the FIGUREnn.LBX picture, the
   figures in the owner's banner colors at their places on the square, the list of enchantment names, and turning
   an enchantment off (with the drowning warning for Flight / Water Walking / Wind Walking at sea); two unused
   drowning checks.
   This module's data (DS:500a): its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 7504 src/wizards/UNITFIG.C */
#include <stdlib.h>
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Battle_Unit_Init_From_Type(int unit_type, struct BattleUnit far *bu);
void UnitView_Alloc_Buffers(void);
void Load_Unit_Figure(int unit_or_type, int is_type);
void Unit_Enchant_Name_List(int unit, char **out, int *n);
void Draw_Unit_Figures(int x, int y, int type_or_unit, int is_unit);
void Unit_Figure_Offset(int figures, int figure, int *x, int *y);
int Unit_Dispel_Enchant_Confirm(int unit, long bits, char *text);
void Unit_Toggle_Enchant_Bits(int unit, long bits);
void Unit_Drown_Check(int unit);
int Units_Stranded_By_Removal(int unit, int *n, int *list);

#include "GAME.H"

extern unsigned char banner_shade_colors[][5];  /* DS:00aa  5 per banner color */
extern struct MutationName mutation_name_table[3];     /* DS:3c68  the Chaos Channels mutations */
extern struct EnchantName enchant_name_table[32];      /* DS:3c9e */
extern unsigned unit_figure_pic;                /* DS:9648 */
extern unsigned unit_figure_src_pic;            /* DS:c17c */
extern struct UnitAbilityList *p_unit_view_list;       /* DS:c192 */
extern unsigned p_unit_view_item_pic;           /* DS:c1e0 */

int Mem_Copy_Far();                                /* unprototyped (see HELPSCR.C) */
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
void Pic_Replace_Color(unsigned pic, int from, int to);
void Pic_Recolor_Opaque(unsigned pic, int color);
void Draw_Unit_Enchant_Outline(int unit);
void Pic_Set_Remap_Flag(unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
void Outline_Cycle_Tick(void);
int Unit_Is_Invisible(int unit);
int Tile_Is_Water(int x, int y, int plane);
void Tile_Units_Of_Owner(int x, int y, int plane, int owner, int *count, int *list);
int Unit_Can_Cross_Water(int unit);
int Hero_Item_Water_Walking(int unit);
int Confirm_Box(char *text);
void Unit_Dismiss(int unit);
void Select_Stack_At_Summoning_Circle(void);

/* (unit_type, bu) the battle-unit record of a unit type (the production screen's preview): the 0x24-byte copy starts
   at the type's melee (+2: BattleUnit mirrors UnitType from melee on) - BUG #250: UnitType is 0x24 bytes, so it also
   takes 2 bytes of the next record; front_damage, weapon_icon and the *_bonus_shown / *_penalty_shown
   counters are cleared, touch strength =
   the type's special strength
   Numbered: BUG #250 (BUGS_AND_QUIRKS.txt) */
void Battle_Unit_Init_From_Type(int unit_type, struct BattleUnit far *bu)
{
    Mem_Copy_Far(bu, (char far *)&unit_types[unit_type].melee, 0x24);
    bu->front_damage = 0;
    bu->weapon_icon = 0;
    bu->melee_bonus_shown = 0;
    bu->ranged_bonus_shown = 0;
    bu->defense_bonus_shown = 0;
    bu->resist_bonus_shown = 0;
    bu->hits_bonus_shown = 0;
    bu->melee_penalty_shown = 0;
    bu->ranged_penalty_shown = 0;
    bu->defense_penalty_shown = 0;
    bu->resist_penalty_shown = 0;
    bu->touch_strength = unit_types[unit_type].special_strength;
}

/* () the unit view's buffers: text (100), the ability list (0x5f0 bytes), the item picture (30 paragraphs at the
   back of the scratch block) */
void UnitView_Alloc_Buffers(void)
{
    p_text_buf = Near_Alloc_Reset(100);
    p_unit_view_list = (struct UnitAbilityList *)Near_Alloc(sizeof(struct UnitAbilityList));
    p_unit_view_item_pic = Heap_Alloc_Back(p_scratch, 30);
}

/* (unit or type, is_type) the figure picture into unit_figure_src_pic: FIGURES1.LBX .. FIGURE14.LBX (198 types), 15 types
   per file, entry (type % 15) x 8 + 2 */
void Load_Unit_Figure(int unit_or_type, int is_type)
{
    int entry;
    char num_text[6];
    char name[20];
    int unit_type;

    if (is_type == 1)
        unit_type = unit_or_type;
    else
        unit_type = p_units[unit_or_type].type;
    itoa(unit_type / 15 + 1, num_text, 10);
    strcpy(name, "FIGURE");
    if (unit_type / 15 + 1 < 10)
        strcat(name, "S");
    strcat(name, num_text);
    strcat(name, ".LBX");
    entry = unit_type % 15 * 8 + 2;
    unit_figure_src_pic = LBX_Reload_Next(name, entry, p_scratch);
}

/* (unit, out, &n) the names of the unit's enchantments (32 bits) and Chaos Channels mutations (3) */
void Unit_Enchant_Name_List(int unit, char **out, int *n)
{
    long ench;
    unsigned char mut;
    int i;
    int count;

    ench = p_units[unit].enchantments;
    count = 0;
    for (i = 0; i < 32; i++) {
        if (ench & enchant_name_table[i].mask) {
            strcpy(out[count], enchant_name_table[i].name);
            count++;
        }
    }
    mut = p_units[unit].mutation_flags;
    for (i = 0; i < 3; i++) {
        if (mutation_name_table[i].mask & mut) {
            strcpy(out[count], mutation_name_table[i].name);
            count++;
        }
    }
    *n = count;
}

/* (x, y, type or unit, is_unit) the figures of a unit type (the current player's banner colors;
   types with AFLAG_INVISIBILITY recolored) or a unit (its live figures,
   1 for the Hydra summon; the owner's colors; invisible units recolored; the enchantment outline): frame 2 of the
   figure picture with colors 214..218 replaced by the banner's shades, each at Unit_Figure_Offset */
void Draw_Unit_Figures(int x, int y, int type_or_unit, int is_unit)
{
    int figure;
    int figures;
    int last;                                   /* written, never read */
    int unit_type;
    int player;
    int banner;
    int fx;
    int fy;
    register int unit;
    register int k;

    unit = type_or_unit;
    if (is_unit == 0) {
        unit_type = unit;
        figures = unit_types[unit_type].figures;
        banner = wizards[current_player].banner_color;
    } else {
        unit_type = p_units[unit].type;
        figures = p_unit_stats_buf->figures_alive;
        player = p_units[unit].owner;
        banner = wizards[player].banner_color;
        if ((int)p_spell_data[SPELL_HYDRA].effect_param == unit_type)
            figures = 1;
    }
    last = figures - 1;
    for (figure = 0; figure < figures; figure++) {
        Set_Pic_Frame(unit_figure_src_pic, 2);
        Pic_Decode_Frame_To_Bitmap(unit_figure_src_pic, unit_figure_pic);
        for (k = 0; k < 5; k++)
            Pic_Replace_Color(unit_figure_pic, k + CITY_MARKER_PAL_BASE, banner_shade_colors[banner][k]);
        if (is_unit != 0) {
            if (Unit_Is_Invisible(unit) == 1)
                Pic_Recolor_Opaque(unit_figure_pic, COLOR_INVISIBLE);
            Draw_Unit_Enchant_Outline(unit);
        } else if (unit_types[unit_type].ability_flags & AFLAG_INVISIBILITY)
            Pic_Recolor_Opaque(unit_figure_pic, COLOR_INVISIBLE);
        Pic_Set_Remap_Flag(unit_figure_pic);
        Unit_Figure_Offset(figures, figure, &fx, &fy);
        Draw_Bitmap(x + fx, y + fy, unit_figure_pic);
    }
    Outline_Cycle_Tick();
}

/* (figures, figure, &x, &y) the place of figure number figure on the square for 1..8 figures (x and y uninitialised otherwise) */
void Unit_Figure_Offset(int figures, int figure, int *x, int *y)
{
    int fx;
    int fy;

    switch (figures) {
    case 1:
        fx = 1;
        fy = 8;
        break;
    case 2:
        if (figure == 0)
            fx = -8;
        else
            fx = 8;
        fy = 9;
        break;
    case 3:
        if (figure == 0) {
            fx = 0;
            fy = 4;
        } else {
            if (figure == 1)
                fx = -7;
            else
                fx = 8;
            fy = 10;
        }
        break;
    case 4:
        if (figure == 0) {
            fx = 1;
            fy = 3;
        } else if (figure == 1) {
            fx = -9;
            fy = 8;
        } else if (figure == 2) {
            fx = 11;
            fy = 8;
        } else {
            fx = 1;
            fy = 13;
        }
        break;
    case 5:
        if (figure == 0) {
            fx = 1;
            fy = 3;
        } else if (figure == 1) {
            fx = -9;
            fy = 8;
        } else if (figure == 2) {
            fx = 1;
            fy = 8;
        } else if (figure == 3) {
            fx = 9;
            fy = 8;
        } else {
            fx = 1;
            fy = 14;
        }
        break;
    case 6:
        if (figure == 0) {
            fx = 1;
            fy = 3;
        } else if (figure == 1) {
            fx = 4;
            fy = 7;
        } else if (figure == 2) {
            fx = -10;
            fy = 8;
        } else if (figure == 3) {
            fx = 10;
            fy = 10;
        } else if (figure == 4) {
            fx = -3;
            fy = 11;
        } else {
            fx = 1;
            fy = 15;
        }
        break;
    case 7:
        if (figure == 0) {
            fx = 1;
            fy = 3;
        } else if (figure == 1) {
            fx = 6;
            fy = 6;
        } else if (figure == 2) {
            fx = -9;
            fy = 8;
        } else if (figure == 3) {
            fx = 1;
            fy = 8;
        } else if (figure == 4) {
            fx = 10;
            fy = 8;
        } else if (figure == 5) {
            fx = -3;
            fy = 11;
        } else {
            fx = 1;
            fy = 14;
        }
        break;
    case 8:
        if (figure == 0) {
            fx = 1;
            fy = 3;
        } else if (figure == 1) {
            fx = 6;
            fy = 6;
        } else if (figure == 2) {
            fx = -2;
            fy = 7;
        } else if (figure == 3) {
            fx = -10;
            fy = 8;
        } else if (figure == 4) {
            fx = 10;
            fy = 8;
        } else if (figure == 5) {
            fx = 3;
            fy = 9;
        } else if (figure == 6) {
            fx = -5;
            fy = 11;
        } else {
            fx = 1;
            fy = 14;
        }
        break;
    }
    *x = fx;
    *y = fy;
}

/* (unit, enchantment bits, text buffer) -> 1 when the unit died: "Do you wish to turn off the <enchantment> spell?".
   At sea with no transport and no other Wind Walker in the stack, removing Flight (unless the unit can cross water),
   Water Walking (unless it flies) or Wind Walking (the others die too) adds the warning (#170). Yes -> the bits are
   toggled and the unit (or every unit of the stack that can neither fly nor cross water) dies; the stack at the
   summoning circle is selected. Called once (the unit view's enchantment click)
   Numbered: BUG #170 (BUGS_AND_QUIRKS.txt) */
int Unit_Dispel_Enchant_Confirm(int unit, long bits, char *text)
{
    int ench_row;
    int will_die;
    int count;
    int stranded;
    int other_walker;
    int self_walker;
    int others_die;
    int tile_units[MAX_STACK];
    int i;

    ench_row = -1;
    for (i = 0; i < 32; i++) {
        if (enchant_name_table[i].mask == bits) {
            ench_row = i;
            break;
        }
    }
    if (ench_row == -1)
        return 0;
    strcpy(text, "Do you wish to turn off the \x02");
    strcat(text, enchant_name_table[ench_row].name);
    strcat(text, "\x01 spell?");
    will_die = 0;
    others_die = 0;
    if ((bits & UNIT_ENCH_FLIGHT) || (bits & UNIT_ENCH_WATER_WALKING) || (bits & UNIT_ENCH_WIND_WALKING)) {
        stranded = Tile_Is_Water(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
        if (unit_types[p_units[unit].type].transport_capacity > 0)
            stranded = 0;
        if (stranded == 1) {
            Tile_Units_Of_Owner(p_units[unit].x, p_units[unit].y, p_units[unit].plane, p_units[unit].owner, &count,
                           tile_units);
            for (i = 0; i < count; i++) {
                if (tile_units[i] != unit && unit_types[p_units[tile_units[i]].type].transport_capacity > 0)
                    stranded = 0;
            }
        }
        if (stranded == 1) {
            self_walker = 0;
            if (Unit_Has_Wind_Walking(unit) == 1 || Unit_Water_Move_Always_0(unit) == 1)
                self_walker = 1;
            other_walker = 0;
            for (i = 0; i < count; i++) {
                if (tile_units[i] != unit
                    && (Unit_Has_Wind_Walking(tile_units[i]) == 1 || Unit_Water_Move_Always_0(tile_units[i]) == 1))
                    other_walker = 1;
            }
            if (other_walker == 0) {
                will_die = 1;
                if ((bits & UNIT_ENCH_FLIGHT)
                    && (Unit_Can_Cross_Water(unit) == 1 || Hero_Item_Water_Walking(unit) == 1 || self_walker == 1))
                    will_die = 0;
                if ((bits & UNIT_ENCH_WATER_WALKING) && (Unit_Can_Fly(unit) == 1 || Hero_Item_Flight(unit) == 1 || self_walker == 1))
                    will_die = 0;
                if (bits & UNIT_ENCH_WIND_WALKING)
                    others_die = 1;
            }
        }
    }
    if (will_die == 1) {
        if (others_die == 0)
            strcat(text, "  This unit will die if the spell is removed!");
        else
            strcat(text, "  This unit and other units will die if the spell is removed!");
    }
    if (Confirm_Box(text) == 1) {
        Unit_Toggle_Enchant_Bits(unit, bits);
        if (will_die == 1) {
            if (others_die == 0)
                Unit_Dismiss(unit);
            else {
                for (i = 0; i < count; i++) {
                    if (Unit_Can_Fly(tile_units[i]) != 1 && Hero_Item_Flight(tile_units[i]) != 1
                        && Unit_Can_Cross_Water(tile_units[i]) != 1 && Hero_Item_Water_Walking(tile_units[i]) != 1)
                        Unit_Dismiss(tile_units[i]);
                }
            }
            Select_Stack_At_Summoning_Circle();
        }
    }
    if (p_units[unit].owner == PLAYER_NONE)
        return 1;
    return 0;
}

/* (unit, bits) flips the enchantment bits */
void Unit_Toggle_Enchant_Bits(int unit, long bits)
{
    p_units[unit].enchantments = p_units[unit].enchantments ^ bits;
}

/* (unit) DEAD (only its overlay thunk exists): on water outside a transport, with no Wind Walker in the stack, the
   unit dies unless it can cross water or fly (items included); its over-capacity pass is unreachable (capacity
   grows only where water is cleared to 0) and inverted besides - it would kill the passengers that COULD survive */
void Unit_Drown_Check(int unit)
{
    int dies;
    int count;
    int water;
    int other_walker;
    int self_walker;
    int unused;
    int capacity;
    int passengers;
    int tile_units[MAX_STACK];
    int i;

    dies = 0;
    unused = 0;
    water = Tile_Is_Water(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
    if (unit_types[p_units[unit].type].transport_capacity > 0)
        water = 0;
    if (water == 1) {
        Tile_Units_Of_Owner(p_units[unit].x, p_units[unit].y, p_units[unit].plane, p_units[unit].owner, &count,
                       tile_units);
        capacity = 0;
        for (i = 0; i < count; i++) {
            if (tile_units[i] != unit && unit_types[p_units[tile_units[i]].type].transport_capacity > 0) {
                water = 0;
                capacity += unit_types[p_units[tile_units[i]].type].transport_capacity;
            }
        }
    }
    if (water == 1) {
        self_walker = 0;
        if (Unit_Has_Wind_Walking(unit) == 1 || Unit_Water_Move_Always_0(unit) == 1)
            self_walker = 1;
        other_walker = 0;
        for (i = 0; i < count; i++) {
            if (tile_units[i] != unit
                && (Unit_Has_Wind_Walking(tile_units[i]) == 1 || Unit_Water_Move_Always_0(tile_units[i]) == 1))
                other_walker = 1;
        }
        if (other_walker == 0 && self_walker == 0) {
            dies = 1;
            if (Unit_Can_Cross_Water(unit) == 1 || Hero_Item_Water_Walking(unit) == 1 || Unit_Can_Fly(unit) == 1
                || Hero_Item_Flight(unit) == 1)
                dies = 0;
        }
        if (capacity > 0) {
            passengers = 0;
            for (i = 0; i < count; i++) {
                if (p_units[tile_units[i]].type <= UNIT_HERO_CHOSEN
                    || unit_types[p_units[tile_units[i]].type].transport_capacity > 0)
                    continue;
                if (passengers > capacity
                    && (Unit_Can_Cross_Water(tile_units[i]) == 1 || Hero_Item_Water_Walking(tile_units[i]) == 1
                        || Unit_Can_Fly(tile_units[i]) == 1 || Hero_Item_Flight(tile_units[i]) == 1))
                    Unit_Dismiss(tile_units[i]);
                passengers++;
            }
        }
    }
    if (dies == 1)
        Unit_Dismiss(unit);
}

/* (unit, &n, list) -> 0 for a dead unit (no return value otherwise): when this unit is the stack's only Wind Walker
   at sea, the units that could neither fly nor cross water. BUG #7: the stack list is never filled (count and the
   list are stale); no callers
   Numbered: BUG #7 (BUGS_AND_QUIRKS.txt) */
int Units_Stranded_By_Removal(int unit, int *n, int *list)
{
    int count;
    int water;
    int other_walker;
    int self_walker;
    int stranded;
    int n_out;
    int tile_units[MAX_STACK];
    int i;

    n_out = 0;
    *n = 0;
    if (p_units[unit].owner == PLAYER_NONE)
        return 0;
    stranded = 0;
    if (Unit_Has_Wind_Walking(unit) == 1 || Unit_Water_Move_Always_0(unit) == 1) {
        water = Tile_Is_Water(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
        if (unit_types[p_units[unit].type].transport_capacity > 0)
            water = 0;
        if (water == 1) {
            self_walker = 0;
            if (Unit_Has_Wind_Walking(unit) == 1 || Unit_Water_Move_Always_0(unit) == 1)
                self_walker = 1;
            other_walker = 0;
            for (i = 0; i < count; i++) {
                if (tile_units[i] != unit
                    && (Unit_Has_Wind_Walking(tile_units[i]) == 1 || Unit_Water_Move_Always_0(tile_units[i]) == 1))
                    other_walker = 1;
            }
            if (other_walker == 0 && self_walker == 1)
                stranded = 1;
        }
    }
    if (stranded == 1) {
        for (i = 0; i < count; i++) {
            if (Unit_Can_Fly(tile_units[i]) != 1 && Hero_Item_Flight(tile_units[i]) != 1
                && Unit_Can_Cross_Water(tile_units[i]) != 1 && Hero_Item_Water_Walking(tile_units[i]) != 1) {
                list[n_out] = tile_units[i];
                n_out++;
            }
        }
    }
    *n = n_out;
}
