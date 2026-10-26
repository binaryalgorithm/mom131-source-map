/* UNITMOVE.C - WIZARDS.EXE overlay code segment 5d70 (stub 2465): a stack's movement modes and the unit movement /
   visibility ability tests (flying, swimming, sailing, Wind Walking, Water Walking, Non-Corporeal, invisibility,
   Endurance, and the hero item versions of them).
   Movement modes (6 words, mode_bits = the UnitType.move_flags bit of each): 0 cavalry (1), 1 forester (0x20),
   2 mountaineer (0x40), 3 swimming (4), 4 sailing (2), 5 flying (8).
   This module's data (DS:33b8): the initialiser of Stack_Movement_Modes' mode_bits (a local array).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 5d70 src/wizards/UNITMOVE.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) - before GAME.H,
   which declares four of them */
void Stack_Active_Movement_Modes(int *modes);
void Stack_Movement_Modes(int *modes, int *units, int count);
int Unit_Can_Fly(int unit);
int Unit_Has_Wind_Walking(int unit);
int Unit_Can_Cross_Water(int unit);
int Unit_Type_Can_Sail(int unit);
int Unit_Can_Swim(int unit);
int Unit_Water_Move_Always_0(int unit);
int Hero_Item_Water_Walking(int unit);
int Hero_Item_Flight(int unit);
int Unit_Is_Invisible(int unit);
int Unit_Has_Endurance(int unit);
int Hero_Item_Planar_Travel(int unit);
int Unit_Is_Non_Corporeal(int unit);

#include "GAME.H"

extern struct StackSlot stack_units[MAX_STACK];         /* DS:999e */
extern int stack_unit_count;                    /* DS:999c */

long Hero_Item_Bonus(int unit, struct BattleUnit far *bu);

/* (&modes): Stack_Movement_Modes over the selected stack's active units (all of them if none is active) */
void Stack_Active_Movement_Modes(int *modes)
{
    int count;
    int none_active;
    int units[MAX_STACK];
    int i;

    none_active = 1;
    for (i = 0; i < stack_unit_count && none_active == 1; i++)
        if (stack_units[i].active != 0)
            none_active = 0;
    if (none_active == 1)
        for (i = 0; i < stack_unit_count; i++)
            stack_units[i].active = 1;
    count = 0;
    for (i = 0; i < stack_unit_count; i++) {
        if (stack_units[i].active == 1) {
            units[count] = stack_units[i].unit;
            count++;
        }
    }
    Stack_Movement_Modes(modes, units, count);
    if (none_active == 1)
        for (i = 0; i < stack_unit_count; i++)
            stack_units[i].active = 0;
}

/* (&modes, units, count): the movement modes of a stack - a mode is set when every unit has it, or when one unit
   carries the stack in it (a forester / mountaineer type, Pathfinding (0x80) for both, a sailing type (no transport test), Wind
   Walking (0x10000) or a flying transport for flying); swimming and flying combine when every unit swims or flies;
   mode 0 ends 0; all units Non-Corporeal (type ability 0x800 / Wraith Form 0x20) give modes 1 and 2.
   BUG #487: one test mixes && and || ('mode 8 && Non-Corporeal type || Wraith Form || item Wraith Form'): a Wraith
   Form unit takes one off the swimmer count for every mode whose type flag it has, not only for flying. BUG #254: a
   natural flyer with Flight / Wind Walking and a natural swimmer with Water Walking are counted twice (type and
   enchantment), and the closing all-Non-Corporeal test ignores item Wraith Form; clarified using ReMoM repo
   Numbered: BUG #254 (BUGS_AND_QUIRKS.txt) */
void Stack_Movement_Modes(int *modes, int *units, int count)
{
    int k;
    int unit;
    int unit_type;
    int with_mode;
    long item_ench;
    int flyers;
    int swimmers;
    int mode_bits[6] = { MOVE_CAVALRY, MOVE_FORESTER, MOVE_MOUNTAINEER, MOVE_SWIMMING, MOVE_SAILING, MOVE_FLYING };
    int move_mode;

    flyers = 0;
    swimmers = 0;
    for (move_mode = 0; move_mode < 6; move_mode++) {
        modes[move_mode] = 0;
        with_mode = 0;
        item_ench = 0;
        for (k = 0; k < count; k++) {
            unit = units[k];
            unit_type = p_units[unit].type;
            item_ench = 0;
            if (p_units[unit].hero_slot != -1) {
                item_ench = Hero_Item_Bonus(unit, p_unit_stats_buf);   /* the stat bonus; the enchantments come from the buffer */
                item_ench = p_unit_stats_buf->item_enchantments;
            }
            if ((unit_types[unit_type].move_flags & mode_bits[move_mode]) == MOVE_FORESTER)
                modes[move_mode] = 1;
            if ((unit_types[unit_type].move_flags & mode_bits[move_mode]) == MOVE_MOUNTAINEER)
                modes[move_mode] = 1;
            if ((unit_types[unit_type].move_flags & mode_bits[move_mode]) == MOVE_SAILING)
                modes[move_mode] = 1;
            if ((unit_types[unit_type].move_flags & mode_bits[move_mode]) == MOVE_FLYING && unit_types[unit_type].transport_capacity > 0)
                modes[move_mode] = 1;
            if (mode_bits[move_mode] == MOVE_FORESTER && ((p_units[unit].enchantments & UNIT_ENCH_PATH_FINDING) != 0 || (item_ench & UNIT_ENCH_PATH_FINDING) != 0))
                modes[move_mode] = 1;
            if (mode_bits[move_mode] == MOVE_MOUNTAINEER && ((p_units[unit].enchantments & UNIT_ENCH_PATH_FINDING) != 0 || (item_ench & UNIT_ENCH_PATH_FINDING) != 0))
                modes[move_mode] = 1;
            if (mode_bits[move_mode] == MOVE_FLYING && ((p_units[unit].enchantments & UNIT_ENCH_WIND_WALKING) != 0
                                      || (unit_types[unit_type].ability_flags & AFLAG_WIND_WALKING) != 0))
                modes[move_mode] = 1;
            if (mode_bits[move_mode] == MOVE_FLYING && (p_units[unit].enchantments & (UNIT_ENCH_WIND_WALKING | UNIT_ENCH_FLIGHT)) != 0
                && unit_types[unit_type].transport_capacity > 0)
                modes[move_mode] = 1;
            if ((unit_types[unit_type].move_flags & mode_bits[move_mode]) == mode_bits[move_mode]) {
                with_mode++;
                if (mode_bits[move_mode] == MOVE_SWIMMING)
                    swimmers++;
                if (mode_bits[move_mode] == MOVE_FLYING)
                    flyers++;
                if ((unit_types[unit_type].ability_flags & AFLAG_NON_CORPOREAL) != 0 || (p_units[unit].enchantments & UNIT_ENCH_WRAITH_FORM) != 0
                    || (item_ench & UNIT_ENCH_WRAITH_FORM) != 0)
                    with_mode--;
                if (mode_bits[move_mode] == MOVE_FLYING && (unit_types[unit_type].ability_flags & AFLAG_NON_CORPOREAL) != 0
                    || (p_units[unit].enchantments & UNIT_ENCH_WRAITH_FORM) != 0 || (item_ench & UNIT_ENCH_WRAITH_FORM) != 0)
                    swimmers--;
            }
            if (mode_bits[move_mode] == MOVE_SWIMMING && ((p_units[unit].enchantments & UNIT_ENCH_WATER_WALKING) != 0 || (item_ench & UNIT_ENCH_WATER_WALKING) != 0)) {
                with_mode++;
                swimmers++;
            }
            if (mode_bits[move_mode] == MOVE_SWIMMING && ((unit_types[unit_type].ability_flags & AFLAG_NON_CORPOREAL) != 0
                                      || (p_units[unit].enchantments & UNIT_ENCH_WRAITH_FORM) != 0 || (item_ench & UNIT_ENCH_WRAITH_FORM) != 0)) {
                with_mode++;
                swimmers++;
            }
            if (mode_bits[move_mode] == MOVE_FLYING && ((p_units[unit].enchantments & (UNIT_ENCH_WIND_WALKING | UNIT_ENCH_FLIGHT)) != 0 || (item_ench & (UNIT_ENCH_WIND_WALKING | UNIT_ENCH_FLIGHT)) != 0
                                      || (p_units[unit].mutation_flags & MUT_CHAOS_DEMON_WINGS) == MUT_CHAOS_DEMON_WINGS)) {
                with_mode++;
                flyers++;
                if ((p_units[unit].enchantments & UNIT_ENCH_WATER_WALKING) != 0 || (item_ench & UNIT_ENCH_WATER_WALKING) != 0
                    || (unit_types[unit_type].ability_flags & AFLAG_NON_CORPOREAL) != 0 || (p_units[unit].enchantments & UNIT_ENCH_WRAITH_FORM) != 0
                    || (item_ench & UNIT_ENCH_WRAITH_FORM) != 0)
                    swimmers--;
            }
        }
        if (with_mode >= count)
            modes[move_mode] = 1;
    }
    if (flyers + swimmers >= count) {
        modes[MOVE_MODE_SWIMMING] = 1;
        if (flyers < 1 || swimmers < 1) {
            modes[MOVE_MODE_SWIMMING] = 1;
            modes[MOVE_MODE_FLYING] = 1;
            if (flyers == 0)
                modes[MOVE_MODE_FLYING] = 0;
            if (swimmers == 0)
                modes[MOVE_MODE_SWIMMING] = 0;
        }
    }
    modes[MOVE_MODE_CAVALRY] = 0;
    with_mode = 0;
    for (k = 0; k < count; k++) {
        unit = units[k];
        unit_type = p_units[unit].type;
        if ((unit_types[unit_type].ability_flags & AFLAG_NON_CORPOREAL) != 0 || (p_units[unit].enchantments & UNIT_ENCH_WRAITH_FORM) != 0)
            with_mode++;
    }
    if (with_mode == count) {
        modes[MOVE_MODE_FORESTER] = 1;
        modes[MOVE_MODE_MOUNTAINEER] = 1;
    }
}

/* (unit): flying - the type's flag 8, Flight / Wind Walking (enchantments) or the Chaos wings mutation */
int Unit_Can_Fly(int unit)
{
    int unit_type;
    int result;

    result = 0;
    unit_type = p_units[unit].type;
    if ((unit_types[unit_type].move_flags & MOVE_FLYING) == MOVE_FLYING)
        result = 1;
    if ((p_units[unit].enchantments & (UNIT_ENCH_WIND_WALKING | UNIT_ENCH_FLIGHT)) != 0 || (p_units[unit].mutation_flags & MUT_CHAOS_DEMON_WINGS) == MUT_CHAOS_DEMON_WINGS)
        result = 1;
    return result;
}

/* (unit): Wind Walking - the enchantment (0x10000) or the type ability 0x1000; carries the whole stack */
int Unit_Has_Wind_Walking(int unit)
{
    int unit_type;
    int result;

    result = 0;
    unit_type = p_units[unit].type;
    if ((p_units[unit].enchantments & UNIT_ENCH_WIND_WALKING) != 0 || (unit_types[unit_type].ability_flags & AFLAG_WIND_WALKING) != 0)
        result = 1;
    return result;
}

/* (unit): sailing or swimming type, or Water Walking (enchantments 0x100)
   Numbered: BUG #310 (BUGS_AND_QUIRKS.txt) */
int Unit_Can_Cross_Water(int unit)
{
    int unit_type;
    int result;

    result = 0;
    unit_type = p_units[unit].type;
    if (unit_types[unit_type].move_flags & MOVE_SAILING)
        result = 1;
    if (unit_types[unit_type].move_flags & MOVE_SWIMMING)
        result = 1;
    if ((p_units[unit].enchantments & UNIT_ENCH_WATER_WALKING) != 0)
        result = 1;
    return result;
}

/* (unit): the unit type's sailing flag (2) */
int Unit_Type_Can_Sail(int unit)
{
    int unit_type;
    int result;

    result = 0;
    unit_type = p_units[unit].type;
    if (unit_types[unit_type].move_flags & MOVE_SAILING)
        result = 1;
    return result;
}

/* (unit): the type's swimming flag (4) or Water Walking */
int Unit_Can_Swim(int unit)
{
    int unit_type;
    int result;

    result = 0;
    unit_type = p_units[unit].type;
    if (unit_types[unit_type].move_flags & MOVE_SWIMMING)
        result = 1;
    if ((p_units[unit].enchantments & UNIT_ENCH_WATER_WALKING) != 0)
        result = 1;
    return result;
}

/* (unit) always 0; every caller ORs it with Unit_Has_Wind_Walking - a removed water-movement ability */
int Unit_Water_Move_Always_0(int unit)
{
    return 0;
}

/* (unit): a hero whose items give Water Walking */
int Hero_Item_Water_Walking(int unit)
{
    long bonus;

    if (p_units[unit].hero_slot != -1) {
        bonus = Hero_Item_Bonus(unit, p_unit_stats_buf);
        if ((p_unit_stats_buf->item_enchantments & UNIT_ENCH_WATER_WALKING) != 0)
            return 1;
    }
    return 0;
}

/* (unit): a hero whose items give Flight / Wind Walking */
int Hero_Item_Flight(int unit)
{
    long bonus;

    if (p_units[unit].hero_slot != -1) {
        bonus = Hero_Item_Bonus(unit, p_unit_stats_buf);
        if ((p_unit_stats_buf->item_enchantments & (UNIT_ENCH_WIND_WALKING | UNIT_ENCH_FLIGHT)) != 0)
            return 1;
    }
    return 0;
}

/* (unit): invisible - by a hero's items, the Invisibility enchantment (0x8000) or the type ability 0x40 */
int Unit_Is_Invisible(int unit)
{
    long bonus;

    if (p_units[unit].hero_slot != -1) {
        bonus = Hero_Item_Bonus(unit, p_unit_stats_buf);
        if ((p_unit_stats_buf->item_enchantments & UNIT_ENCH_INVISIBILITY) != 0)
            return 1;
    }
    if ((p_units[unit].enchantments & UNIT_ENCH_INVISIBILITY) != 0 || (unit_types[p_units[unit].type].ability_flags & AFLAG_INVISIBILITY) != 0)
        return 1;
    return 0;
}

/* (unit): the Endurance enchantment (0x2000) */
int Unit_Has_Endurance(int unit)
{
    if ((p_units[unit].enchantments & UNIT_ENCH_ENDURANCE) != 0)
        return 1;
    return 0;
}

/* (unit): a hero whose items give Planar Travel */
int Hero_Item_Planar_Travel(int unit)
{
    long bonus;

    if (p_units[unit].hero_slot != -1) {
        bonus = Hero_Item_Bonus(unit, p_unit_stats_buf);
        if ((p_unit_stats_buf->item_enchantments & UNIT_ENCH_PLANAR_TRAVEL) != 0)
            return 1;
    }
    return 0;
}

/* (unit): Non-Corporeal - the type ability 0x800, Wraith Form (enchantments 0x20) or a hero item with Wraith Form
   Numbered: BUG #310 (BUGS_AND_QUIRKS.txt) */
int Unit_Is_Non_Corporeal(int unit)
{
    int unit_type;
    long bonus;

    unit_type = p_units[unit].type;
    if ((unit_types[unit_type].ability_flags & AFLAG_NON_CORPOREAL) != 0 || (p_units[unit].enchantments & UNIT_ENCH_WRAITH_FORM) != 0)
        return 1;
    if (p_units[unit].hero_slot != -1) {
        bonus = Hero_Item_Bonus(unit, p_unit_stats_buf);
        if ((p_unit_stats_buf->item_enchantments & UNIT_ENCH_WRAITH_FORM) != 0)
            return 1;
    }
    return 0;
}
