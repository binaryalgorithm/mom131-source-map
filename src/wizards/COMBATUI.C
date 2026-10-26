/* COMBATUI.C - WIZARDS.EXE overlay code segment 8593: combat state and figure effects - the winner check (and the AI's
   flight), banner recoloring, the hovered figure's highlight, the figure effects by draw mode, each unit's aura realm
   from its enchantments, the enchantment outline, see-invisible and visibility, the status icon, the always-animate
   flags, the melee / ranged weapon icons, and the hover info panel.
   This module's data (DS:59a6): its literals.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 8593 src/wizards/COMBATUI.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Combat_Check_Winner(void);
void Banner_Colorize(int player);
void Unit_Pic_Hover_Highlight(int bu);
void Unit_Pic_Apply_Effect(int effect);
void Unit_Enchant_Aura_Color(int bu);
void Combat_Unit_Enchant_Outline(int aura_realm);
void Combat_Update_See_Invisible(void);
void Unit_Update_Visibility(int bu);
int Unit_Combat_Status_Icon(int bu);
void Unit_Set_Anim_Flags(int bu);
int Unit_Melee_Icon(int bu);
int Unit_Ranged_Icon(int bu);
void Combat_Draw_Hover_Info(void);

#include "GAME.H"

extern unsigned char banner_shade_colors[][5];  /* DS:00aa */
extern int outline_flash_phase;                 /* DS:305e */
extern unsigned char realm_outline_colors[5][8][8];  /* DS:3062 */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern int combat_ai_fled;                      /* DS:924a */
extern unsigned combat_scan_icons[];            /* DS:924c */
extern unsigned p_pic_work;                     /* DS:998c */
extern int combat_city_defended;                /* DS:c408 */
extern int combat_defender_sees_invis;          /* DS:c41e */
extern int combat_attacker_sees_invis;                /* DS:c420 */
extern int combat_ai_side;                      /* DS:c434 */
extern int combat_human_side;                   /* DS:c436 */
extern int combat_turn_count;                   /* DS:c51c */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern int combat_hover_unit;                   /* DS:c58a */

/* Fill_Box_Styled has no prototype here: this module calls it with 6 and with 5 arguments */
void Fill_Box_Styled();
int Print_Text_Centered(int x, int y, char *text);
void Font_Select_Shadow(int font, int c1, int c2, int c3);
int AI_Should_Flee(int player);
void Pic_Replace_Color(unsigned pic, int from, int to);
void Pic_Set_Remap_Flag(unsigned pic);
void Pic_Clear_Color_Range(unsigned pic, int lo, int hi);
void Pic_Remap_Shade(unsigned pic, int shade);
void Pic_Monochrome_Remap(unsigned pic, int base);
void Pic_Recolor_Opaque(unsigned pic, int color);
void Pic_Outline_Recolor2(unsigned pic, int color);
void Pic_Replace_Color_Dithered(unsigned pic, int color, char *pattern);
int Combat_Distance(int bu1, int bu2);
int Unit_Flying_Not_Held(int bu, int unused);
void Draw_Line(int x1, int y1, int x2, int y2, int color);
void Set_Font_Color(int color);
void Print_Half_Moves(int x, int y, int half_moves, int align);
int Unit_Combat_Move_Type(int bu);
void Combat_Draw_Health_Bar(int bu, int x, int y);

/* () -> the winner, or -1: a side with no able units loses (a unit turned by Confusion counts for the side it came from: the
   roll swapped its owner, and the test counts CONFUSION_CHANGED_SIDE units for the other side; clarified using ReMoM repo), after turn 50
   the defender wins; an AI side (not neutral, not in a battle at a wizard's city - attacker or defender -, from turn 2) that should flee flees -
   its units leave the battle and the human side wins */
int Combat_Check_Winner(void)
{
    int attackers;
    int defenders;
    int i;

    attackers = 0;
    defenders = 0;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            if (p_battle_units[i].owner == combat_attacker_player) {
                if (p_battle_units[i].confusion_result == CONFUSION_CHANGED_SIDE)
                    defenders++;
                else
                    attackers++;
            } else if (p_battle_units[i].owner == combat_defender_player) {
                if (p_battle_units[i].confusion_result == CONFUSION_CHANGED_SIDE)
                    attackers++;
                else
                    defenders++;
            }
        }
    }
    if (attackers == 0)
        return combat_defender_player;
    if (defenders == 0)
        return combat_attacker_player;
    if (combat_turn_count > COMBAT_TURN_LIMIT)
        return combat_defender_player;
    if (combat_ai_side != PLAYER_NEUTRAL && combat_city_defended == 0 && combat_turn_count > 1
            && AI_Should_Flee(combat_ai_side) == 1) {
        for (i = 0; i < battle_unit_count; i++) {
            if (p_battle_units[i].owner == combat_ai_side) {
                p_battle_units[i].action_state = ACTION_FLEE;
                if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE)
                    p_battle_units[i].status = BATTLE_UNIT_FLED;
            }
        }
        combat_ai_fled = 1;
        return combat_human_side;
    }
    return -1;
}

/* (player) the work picture's colors 214..218 -> the player's banner shades */
void Banner_Colorize(int player)
{
    int banner;
    int i;

    banner = wizards[player].banner_color;
    for (i = 0; i < 5; i++)
        Pic_Replace_Color(p_pic_work, i + 214, banner_shade_colors[banner][i]);   /* 214 .. 218: a figure's banner
                                                   colour slots */
}

/* (bu) the hovered unit's figure: color 1 -> PAL_CYCLE_COLOR (the pulsing color) */
void Unit_Pic_Hover_Highlight(int bu)
{
    if (combat_hover_unit == bu)
        Pic_Replace_Color(p_pic_work, 1, PAL_CYCLE_COLOR);
}

/* (effect) the figure effect of a draw mode: 1 shaded, 2 / 3 monochrome, 4 recolored, 5 (invisible) cleared */
void Unit_Pic_Apply_Effect(int effect)
{
    Pic_Set_Remap_Flag(p_pic_work);
    switch (effect) {
    case DRAW_MODE_HELD:
        Pic_Clear_Color_Range(p_pic_work, 232, 232);
        Pic_Remap_Shade(p_pic_work, 1);
        break;
    case DRAW_MODE_MONOCHROME:
        Pic_Clear_Color_Range(p_pic_work, 232, 232);
        Pic_Monochrome_Remap(p_pic_work, 104);
        break;
    case DRAW_MODE_WARPED:
        Pic_Clear_Color_Range(p_pic_work, 232, 232);
        Pic_Monochrome_Remap(p_pic_work, 40);
        break;
    case DRAW_MODE_SEEN_INVISIBLE:
        Pic_Clear_Color_Range(p_pic_work, 232, 232);   /* 232 cleared in every mode */
        Pic_Recolor_Opaque(p_pic_work, COLOR_INVISIBLE);
        break;
    case DRAW_MODE_HIDDEN:
        Pic_Clear_Color_Range(p_pic_work, 1, 228);
        Pic_Clear_Color_Range(p_pic_work, 232, 232);
        break;
    }
}

/* (bu) the battle unit's aura realm (0 Chaos .. 4 Sorcery, -1 none) from the first matching unit or combat
   enchantment or combat flag; none while invisible */
void Unit_Enchant_Aura_Color(int bu)
{
    unsigned long ench;
    unsigned combat_flags;
    int aura;

    aura = -1;
    ench = p_battle_units[bu].combat_enchantments | p_units[p_battle_units[bu].unit_index].enchantments;
    combat_flags = p_battle_units[bu].combat_flags;
    if ((ench & UNIT_ENCH_INVISIBILITY) != 0 || (p_battle_units[bu].ability_flags & AFLAG_INVISIBILITY) != 0) {
        ench = 0;
        combat_flags = 0;
    }
    if (ench != 0 || combat_flags != 0) {
        if (ench & UNIT_ENCH_REGENERATION)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_RESIST_MAGIC)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_RIGHTEOUSNESS)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_PLANAR_TRAVEL)
            aura = OUTLINE_LIFE;
        else if (combat_flags & COMBAT_FLAG_HASTE)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_BLACK_CHANNELS)
            aura = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_LIONHEART)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_IRON_SKIN)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_MAGIC_IMMUNITY)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_WIND_WALKING)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_ELEMENTAL_ARMOR)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_TRUE_SIGHT)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_INVULNERABILITY)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_PATH_FINDING)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_FLIGHT)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_ELDRITCH_WEAPON)
            aura = OUTLINE_CHAOS;
        else if (ench & UNIT_ENCH_WRAITH_FORM)
            aura = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_GIANT_STRENGTH)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_IMMOLATION)
            aura = OUTLINE_CHAOS;
        else if (ench & UNIT_ENCH_GUARDIAN_WIND)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_SPELL_LOCK)
            aura = OUTLINE_SORCERY;
        else if (ench & UNIT_ENCH_HEROISM)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_BERSERK)
            aura = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_HOLY_ARMOR)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_WATER_WALKING)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_ENDURANCE)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_CLOAK_OF_FEAR)
            aura = OUTLINE_DEATH;
        else if (ench & UNIT_ENCH_STONE_SKIN)
            aura = OUTLINE_NATURE;
        else if (ench & UNIT_ENCH_FLAME_BLADE)
            aura = OUTLINE_CHAOS;
        else if (ench & UNIT_ENCH_BLESS)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_HOLY_WEAPON)
            aura = OUTLINE_LIFE;
        else if (ench & UNIT_ENCH_RESIST_ELEMENTS)
            aura = OUTLINE_NATURE;
    }
    p_battle_units[bu].aura_color = aura;
}

/* (aura_realm: an outline row OUTLINE_*, -1 none) the work picture outlined and recolored with the realm's flashing
   outline colors; role name corrected to ReMoM's reading (10/17 audit); clarified using ReMoM repo */
void Combat_Unit_Enchant_Outline(int aura_realm)
{
    if (aura_realm != -1) {
        Pic_Outline_Recolor2(p_pic_work, 255);
        Pic_Replace_Color_Dithered(p_pic_work, 255, realm_outline_colors[aura_realm][outline_flash_phase]);
    }
}

/* () whether either side has a living unit that sees invisible units */
void Combat_Update_See_Invisible(void)
{
    int bu;

    combat_attacker_sees_invis = 0;
    combat_defender_sees_invis = 0;
    for (bu = 0; bu < battle_unit_count; bu++) {
        if (p_battle_units[bu].status == BATTLE_UNIT_ACTIVE) {
            if (p_battle_units[bu].owner == combat_attacker_player) {
                if (p_battle_units[bu].abilities & ABILITY_ILLUSIONS_IMMUNITY)
                    combat_attacker_sees_invis = 1;
            } else if (p_battle_units[bu].owner == combat_defender_player && (p_battle_units[bu].abilities & ABILITY_ILLUSIONS_IMMUNITY)) {
                combat_defender_sees_invis = 1;
            }
        }
    }
}

/* (bu) an invisible unit is drawn as seen (draw mode 4) for the human, when the other side sees invisible units, or
   next to a living enemy; else hidden (5) */
void Unit_Update_Visibility(int bu)
{
    unsigned long ench;
    int owner;
    int distance;
    int other;
    int visible;

    owner = p_battle_units[bu].owner;
    ench = p_battle_units[bu].combat_enchantments | p_units[p_battle_units[bu].unit_index].enchantments;
    visible = 1;
    if ((ench & UNIT_ENCH_INVISIBILITY) != 0 || (p_battle_units[bu].ability_flags & AFLAG_INVISIBILITY) != 0) {
        visible = 0;
        if (owner == combat_defender_player && combat_attacker_sees_invis == 1) {
            visible = 1;
            p_battle_units[bu].draw_mode = DRAW_MODE_SEEN_INVISIBLE;
        }
        if (owner == combat_attacker_player && combat_defender_sees_invis == 1) {
            visible = 1;
            p_battle_units[bu].draw_mode = DRAW_MODE_SEEN_INVISIBLE;
        }
        if (owner == combat_human_side) {
            visible = 1;
            p_battle_units[bu].draw_mode = DRAW_MODE_SEEN_INVISIBLE;
        }
        if (visible == 0) {
            p_battle_units[bu].draw_mode = DRAW_MODE_HIDDEN;
            for (other = 0; other < battle_unit_count; other++) {
                if (p_battle_units[other].status == BATTLE_UNIT_ACTIVE && p_battle_units[other].owner != owner) {
                    distance = Combat_Distance(bu, other);
                    if (distance < 2) {
                        visible = 1;
                        p_battle_units[bu].draw_mode = DRAW_MODE_SEEN_INVISIBLE;
                    }
                }
            }
        }
    }
}

/* (bu) -> the status icon (0..7, -1 none) of the first combat flag set; also the held / warped draw modes */
int Unit_Combat_Status_Icon(int bu)
{
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_BLACK_SLEEP)
        p_battle_units[bu].draw_mode = DRAW_MODE_HELD;
    if ((p_battle_units[bu].combat_flags & COMBAT_FLAG_WARP_MELEE) || (p_battle_units[bu].combat_flags & COMBAT_FLAG_WARP_DEFENSE)
            || (p_battle_units[bu].combat_flags & COMBAT_FLAG_WARP_RESIST))
        p_battle_units[bu].draw_mode = DRAW_MODE_WARPED;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_VERTIGO)
        return 0;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_CONFUSION)
        return 1;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_WHIRLWIND)
        return 2;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_MIND_STORM)
        return 3;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_SHATTER)
        return 4;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_WEAKNESS)
        return 5;
    if (p_battle_units[bu].combat_flags & COMBAT_FLAG_MIND_TWIST)
        return 6;
    if ((p_battle_units[bu].combat_flags & COMBAT_FLAG_WEB) && p_battle_units[bu].web_hp > 0)
        return 7;
    return -1;
}

/* (bu) always_animate = Unit_Flying_Not_Held for a unit with ability 0x200 (and 1 for the Great Wyrm and Fire
   Elemental), anim_ability = 1 for that ability, else 0 (a flag, not the bit) */
void Unit_Set_Anim_Flags(int bu)
{
    int anim;
    int always;

    anim = 0;
    always = 0;
    if (p_battle_units[bu].abilities & ABILITY_FLYING_ANIM) {
        anim = 1;
        always = Unit_Flying_Not_Held(bu, 0);
    }
    if (p_units[p_battle_units[bu].unit_index].type == (int)p_spell_data[SPELL_GREAT_WYRM].effect_param
            || p_units[p_battle_units[bu].unit_index].type == (int)p_spell_data[SPELL_FIRE_ELEMENTAL].effect_param)
        always = 1;
    p_battle_units[bu].always_animate = always;
    p_battle_units[bu].anim_ability = anim;
}

/* (bu) -> the melee weapon icon (by weapon quality), -1 without a melee attack */
int Unit_Melee_Icon(int bu)
{
    int icon;
    int weapon;

    icon = -1;
    if (p_battle_units[bu].melee > 0) {
        weapon = p_battle_units[bu].weapon_icon;
        if (weapon > 0) {
            switch (weapon) {
            case WEAPON_MAGIC + 1:
                icon = SCAN_ICON_MAGIC_SWORD;
                break;
            case WEAPON_MITHRIL + 1:
                icon = SCAN_ICON_MITHRIL;
                break;
            case WEAPON_ADAMANTIUM + 1:
                icon = SCAN_ICON_ADAMANTIUM;
                break;
            default:
                icon = SCAN_ICON_SWORD;
                break;
            }
        } else {
            icon = SCAN_ICON_SWORD;
        }
    }
    return icon;
}

/* (bu) -> the ranged attack icon by ranged type, -1 without one. BUG #438: only the boulder / missile / magic types
   (class 1..3) reach the switch, so its thrown / breath / gaze cases are dead and those units get -1 */
int Unit_Ranged_Icon(int bu)
{
    int icon;
    int ranged_type;

    icon = -1;
    ranged_type = p_battle_units[bu].ranged_type;
    if (ranged_type != -1) {
        if (ranged_type / 10 == RANGED_CLASS_BOULDER || ranged_type / 10 == RANGED_CLASS_MISSILE || ranged_type / 10 == RANGED_CLASS_MAGIC) {
        switch (ranged_type) {
        case RANGED_THROWN:
            icon = SCAN_ICON_THROWN;
            break;
        case RANGED_MAGIC_LIGHTNING:
        case RANGED_MAGIC_FIRE:
        case RANGED_MAGIC_ILLUSION:
        case RANGED_MAGIC_DEATH:
        case RANGED_MAGIC_WIND:
        case RANGED_MAGIC_HOLY:
        case RANGED_MAGIC_DARK:
        case RANGED_MAGIC_SPRITE:
        case RANGED_MAGIC_NATURE:
            icon = SCAN_ICON_MAGIC_RANGED;
            break;
        case RANGED_FIRE_BREATH:
        case RANGED_LIGHTNING:
        case RANGED_STONING_GAZE:
            icon = SCAN_ICON_BREATH;
            break;
        case RANGED_BOULDER:
        case RANGED_UNUSED_11:
        case RANGED_SLING:
        case 22:                                /* no name: no unit has it */
            icon = SCAN_ICON_BOULDER;
            break;
        default:
            icon = SCAN_ICON_BOW;
            break;
        }
        }
    }
    return icon;
}

/* () the hovered unit's info panel (top right): name, melee / ranged / moves with icons, hits bar, defense, resist,
   mana or ammo, and the rank badges */
void Combat_Draw_Hover_Info(void)
{
    int n;                                      /* the hero slot, then a scan icon, then the first pip level */
    int owner;
    int unit_type;
    int unit;
    unsigned pic;
    int move_type;
    int k;
    int x2;
    int y2;
    int value;
    char colors[2];
    int defense;
    int resist;
    int x1;
    int y1;

    if (combat_hover_unit != -1 && combat_hover_unit < MAX_BATTLE_UNITS) {
        x1 = 250;
        y1 = 4;
        x2 = 318;
        y2 = 43;
        Fill_Box_Styled(x1, y1, x2, y2, FILL_REMAP, 8);
        Draw_Line(x1, y1, x1, y2, 0xe6);
        Draw_Line(x1, y1, x2 - 1, y1, 0xe6);
        Draw_Line(x2, y1, x2, y2, 0xed);
        Draw_Line(x1 + 1, y2, x2, y2, 0xed);
        colors[0] = 0xb6;
        colors[1] = 0xb1;
        Font_Select_Palette(0, colors);
        Font_Select_Shadow(0, 15, 0, 0);
        Set_Font_Color(0xb6);
        Set_Text_Shadow_Color(2);
        unit = p_battle_units[combat_hover_unit].unit_index;
        owner = p_units[unit].owner;
        n = p_units[unit].hero_slot;
        unit_type = p_units[unit].type;
        if (n != -1)
            strcpy(p_text_buf, wizards[owner].hired_heroes[n].name);
        else
            strcpy(p_text_buf, unit_types[unit_type].name);
        Print_Text_Centered(x1 + (x2 - x1) / 2, y1 + 2, p_text_buf);
        n = Unit_Melee_Icon(combat_hover_unit);
        if (n != -1) {
            value = p_battle_units[combat_hover_unit].melee;
            Print_Number_Right(x1 + 9, y1 + 10, value);
            Draw_Pic_Frame(x1 + 11, y1 + 8, combat_scan_icons[n]);
        }
        n = Unit_Ranged_Icon(combat_hover_unit);
        if (n != -1) {
            value = p_battle_units[combat_hover_unit].ranged;
            Print_Number_Right(x1 + 9, y1 + 17, value);
            Draw_Pic_Frame(x1 + 11, y1 + 15, combat_scan_icons[n]);
        }
        if (p_battle_units[combat_hover_unit].moves != 0)
            Print_Half_Moves(x1 + 9, y1 + 24, p_battle_units[combat_hover_unit].moves, 1);
        else
            Print_Number_Right(x1 + 9, y1 + 24, p_battle_units[combat_hover_unit].moves);
        move_type = Unit_Combat_Move_Type(combat_hover_unit);
        n = SCAN_ICON_BOOT;
        if (move_type == COMBAT_MOVE_FLY)
            n = SCAN_ICON_WINGS;
        if (move_type == COMBAT_MOVE_SAIL || move_type == COMBAT_MOVE_SWIM)
            n = SCAN_ICON_WAVE;
        Draw_Pic_Frame(x1 + 11, y1 + 22, combat_scan_icons[n]);
        Print_Text(x1 + 3, y1 + 32, "Hits");
        Fill_Box_Styled(x1 + 19, y1 + 34, x1 + 38, y1 + 34, FILL_SHADOW);
        Draw_Line(x1 + 19, y1 + 35, x1 + 38, y1 + 35, 1);
        Combat_Draw_Health_Bar(combat_hover_unit, x1 + 19, y1 + 34);
        defense = p_battle_units[combat_hover_unit].defense;
        Print_Number_Right(x1 + 48, y1 + 10, defense);
        Draw_Pic_Frame(x1 + 50, y1 + 8, combat_scan_icons[SCAN_ICON_SHIELD]);
        resist = p_battle_units[combat_hover_unit].resist;
        Print_Number_Right(x1 + 48, y1 + 17, resist);
        Draw_Pic_Frame(x1 + 50, y1 + 15, combat_scan_icons[SCAN_ICON_RESIST]);
        if (p_battle_units[combat_hover_unit].spell_points > 0) {
            Print_Number_Right(x1 + 48, y1 + 24, p_battle_units[combat_hover_unit].spell_points);
            Print_Text(x1 + 51, y1 + 24, "mp");
        } else if (p_battle_units[combat_hover_unit].ammo > 0) {
            Print_Number_Right(x1 + 48, y1 + 24, p_battle_units[combat_hover_unit].ammo);
            Print_Text(x1 + 51, y1 + 24, "ammo");
        }
        if (p_units[unit].level > 0) {
            if (p_units[unit].level > 6) {
                pic = combat_scan_icons[SCAN_ICON_PIP_RED];
                n = 6;
            } else if (p_units[unit].level > 3) {
                pic = combat_scan_icons[SCAN_ICON_PIP_GOLD];
                n = 3;
            } else {
                pic = combat_scan_icons[SCAN_ICON_PIP_GREY];
                n = 0;
            }
            k = 0;
            for (value = n; value < p_units[unit].level; value++) {
                Draw_Pic_Frame(x1 + k * 5 + 48, y1 + 33, pic);
                k++;
            }
        }
    }
}
