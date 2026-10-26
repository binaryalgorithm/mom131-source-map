/* SPELLFX.C - WIZARDS.EXE overlay code segment b66a (stub 25c0): combat spell effects and their animations -
   Warp Creature, Wrack and Call Lightning (each turn), Life Drain, the three walls (Stone, Fire, Darkness), the
   "... spell to fizzle" message, the dispel roll, Cracks Call, the bolt spells (Ice Bolt, Fire Bolt, Fireball,
   Doom Bolt), a spell animation on a square, the Teleport / Merging moves, a combat summon rising out of the
   ground, a Lightning Bolt strike, Magic Vortices (creation, the human's steering, the random moves, a move with
   its damage) and Call Chaos (the effect animation and the effects).
   This module's data (DS:68fc..6993): its literals only ("the " is the tail of "The power of the ": -d); no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS b66a src/wizards/SPELLFX.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Call_Chaos_Cast(int caster, int animate);
void Call_Chaos_Apply(int player, int *effects);
void Warp_Creature_Apply(int unit);
void Wrack_Turn(int caster_player);
void Call_Lightning_Turn(int caster_player);
void Life_Drain_Apply(int target, int *damage, int caster, int mana);
void Combat_Wall_Create(int spell, int caster);
void Spell_Fizzle_Message(int caster, int blocker, int spell, char *blocker_spell);
int Dispel_Roll(int strength, int cost, int owner, int realm);
void Cracks_Call_Anim(int x, int y, int caster);
void Bolt_Spell_Anim(int x, int y, int spell, int caster);
void Spell_Anim_On_Square(int x, int y, int big, int caster, int spell);
void Combat_Teleport_Anim(int unit, int x, int y);
void Combat_Merge_Anim(int unit, int x, int y);
void Combat_Summon_Anim(int unit, int x, int y, int spell, int caster);
void Lightning_Strike_Anim(int x, int y, int caster);
void Vortex_Create(int caster_player, int x, int y);
void Vortex_Steer_Cursor(int vortex);
void Vortex_Human_Move(int vortex);
void Vortices_Turn(void);
void Vortex_Move(int vortex, int x, int y);

#include "GAME.H"

extern int combat_cursor_anim;                  /* DS:7080 */
extern int combat_effect_anim_frame;                  /* DS:708a */
extern int combat_special_anim;                 /* DS:708e  1 while combat_effect_pic plays at combat_special_anim_x / _y */
extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern int combat_wall_anim_frame;                   /* DS:9270 */
extern int combat_wall_rising;                /* DS:9272 */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern int combat_active_marker_y;              /* DS:9278 */
extern int combat_active_marker_x;              /* DS:927a */
extern int combat_active_marker_on;             /* DS:927c */
extern int combat_target_y;                     /* DS:927e */
extern int combat_target_x;                     /* DS:9280 */
extern int combat_cursor_mode;                  /* DS:9282 */
extern unsigned p_pic_work;                     /* DS:998c */
extern unsigned p_world_block;                /* DS:9d20 */
extern int city_damage_counter;                 /* DS:c418 */
extern unsigned combat_silence_sfx;             /* DS:c41a */
extern int combat_auto;                         /* DS:c432 */
extern int *p_combat_cursor_region;             /* DS:c522  one MouseRegion */
extern int combat_active_unit;                  /* DS:c582 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */
extern unsigned combat_figure_pic;              /* DS:c7b2  a battle unit's figure group picture */
extern unsigned combat_effect_pic;              /* DS:d13a */
extern int combat_special_anim_y;               /* DS:d14a */
extern int combat_special_anim_x;               /* DS:d14c */
extern struct CombatVortex far *p_vortices;     /* DS:d14e */
extern int vortex_count;                        /* DS:d152 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
void far_strcpy(char *dst, int dst_seg, char far *src);
void Battle_Apply_Damage(int unit, int *damage);
void Battle_Unit_Heal(int bu, int amount, int overflow);
void Battlefield_Draw_Terrain(void);
void Combat_Build_Unit_Figures_Pic(int unit);
void Combat_Cast_Banner(int caster, int spell);
void Combat_Draw_Banner_Text(void);
void Combat_Draw_Screen(void);
void Combat_Grid_To_Screen(int gx, int gy, int ox, int oy, int *sx, int *sy);
void Combat_Load_Spell_Sound(int spell);
int Combat_Screen_To_Grid_X(int sx, int sy);
int Combat_Screen_To_Grid_Y(int sx, int sy);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Load_Wall_Rise_Anim(int wall_kind);
int Mouse_X(void);
int Mouse_Y(void);
void Pic_Copy(unsigned dst, unsigned src);
void Pic_Create_Blank(int width, int height, unsigned pic);
void Pic_Dissolve(unsigned pic, int amount);
int Pic_Frame_Count(unsigned pic);
void Pic_Reset_Frame(unsigned pic);
void Pic_Set_Remap_Flag(unsigned pic);
void Play_Message_Sound(void);
int Resist_Bonus(struct BattleUnit bu, int realm);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
void Scratch_Restore_From_EMS(void);
void Scratch_Save_To_EMS(void);
void Screen_Copy_Front(void);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Pic_Frame(unsigned pic, unsigned frame);
int Sound_Play(unsigned sound);
int Spell_Load_Anim(int spell);
int Spell_Damage_Roll(int spell, int target, int *damage, int strength);
int Spell_Dispel_Resistance(int strength, int player, int realm);
int UI_Add_Grid_Control(int x, int y, int cell_w, int cell_h, int cols, int rows, int *col, int *row, int help);
int Unit_Is_Normal_Living(int unit);
void Warning_Box(char *text);

/* (unit): Warp Creature - one of the three warp flags (0x80, 0x100, 0x200) not yet set, at random (BUG #16: loops
   for ever when all three are set)
   Numbered: BUG #16 (BUGS_AND_QUIRKS.txt) */
void Warp_Creature_Apply(int unit)
{
    int r;                                      /* the roll 1..3, or -1 / -2 / -3 to retry */

    r = -2;
    while (r < 0) {
        r = Random(3);
        switch (r) {
        case 1:
            if (p_battle_units[unit].combat_flags & COMBAT_FLAG_WARP_MELEE) {
                r = -1;
            } else {
                p_battle_units[unit].combat_flags = p_battle_units[unit].combat_flags | COMBAT_FLAG_WARP_MELEE;
            }
            break;
        case 2:
            if (p_battle_units[unit].combat_flags & COMBAT_FLAG_WARP_DEFENSE) {
                r = -2;
            } else {
                p_battle_units[unit].combat_flags = p_battle_units[unit].combat_flags | COMBAT_FLAG_WARP_DEFENSE;
            }
            break;
        case 3:
            if (p_battle_units[unit].combat_flags & COMBAT_FLAG_WARP_RESIST) {
                r = -3;
            } else {
                p_battle_units[unit].combat_flags = p_battle_units[unit].combat_flags | COMBAT_FLAG_WARP_RESIST;
            }
            break;
        }
    }
}

/* (caster player): Wrack - every figure of every enemy unit (not immune: Wraith Form, Death Immunity) resists at +1
   or takes 1 damage, shown on the victims during the animation. BUG #325: the list holds one entry per failed
   figure and each entry applies the unit's whole failure count (k failures: k x k damage), and victims[36] has no
   bound test - more than 36 failed figures overrun it
   Numbered: BUG #247 (BUGS_AND_QUIRKS.txt) */
void Wrack_Turn(int caster_player)
{
    int fig;
    int k;
    int i;
    int anim;
    long ench;
    int damage[3];
    int n;
    int x;
    int y;
    char victims[36];
    char wracked[36];

    damage[DAMAGE_NORMAL] = 1;
    damage[DAMAGE_DESTROY] = 0;
    damage[DAMAGE_DRAIN] = 0;
    n = 0;
    for (i = 0; i < 36; i++) {
        wracked[i] = 0;
    }
    Heap_Mark(p_scratch);
    anim = Spell_Load_Anim(SPELL_WRACK);
    for (i = 0; i < battle_unit_count; i++) {
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (p_battle_units[i].owner != caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (ench & UNIT_ENCH_WRAITH_FORM) == 0 &&
            (p_battle_units[i].abilities & ABILITY_DEATH_IMMUNITY) == 0) {
            for (fig = 0; fig < p_battle_units[i].figures_alive; fig++) {
                if (Roll_Resist(p_battle_units[i], 1, REALM_DEATH) > 0) {
                    victims[n] = i;
                    wracked[i] = wracked[i] + 1;
                    n++;
                }
            }
        }
    }
    for (i = 0; i < Pic_Frame_Count(p_anim_pic) + n; i++) {
        if ((Pic_Frame_Count(p_anim_pic) + n) / 2 == i) {
            for (fig = 0; fig < n; fig++) {
                if (wracked[victims[fig]] > 0) {
                    damage[DAMAGE_NORMAL] = wracked[victims[fig]];
                    Battle_Apply_Damage(victims[fig], damage);
                }
            }
        }
        Set_Draw_Page();
        Combat_Draw_Screen();
        for (k = 0; k < n; k++) {
            if (k <= i && k + Pic_Frame_Count(p_anim_pic) >= i) {
                Combat_Grid_To_Screen(p_battle_units[victims[k]].combat_x, p_battle_units[victims[k]].combat_y, 4, 4,
                                      &x, &y);
                if (anim == 0) {
                    x -= 14;
                    y -= 21;
                } else {
                    x -= 28;
                    y -= 30;
                }
                Set_Pic_Frame(p_anim_pic, i - k);
                Draw_Pic_Clipped(x, y, p_anim_pic);
            }
        }
        Screen_Present();
    }
    Heap_Release(p_scratch);
}

/* (caster player): Call Lightning - Random(3) + 2 bolts, each at a random enemy unit (not under Wraith Form - BUG #222:
   Magic Immunity is not tested; 1 in 2), at most
   30 misses in a row; each bolt rolls the damage of spell record 3 (Wall of Stone)
   Numbered: BUG #222, #260 (BUGS_AND_QUIRKS.txt) */
void Call_Lightning_Turn(int caster_player)
{
    int i;
    int tries;
    long ench;
    int bolts;
    int damage[3];

    for (i = 0; i < 3; i++) {
        damage[i] = 0;
    }
    Heap_Mark(p_scratch);
    Spell_Load_Anim(SPELL_CALL_LIGHTNING);
    Combat_Load_Spell_Sound(SPELL_LIGHTNING_BOLT);
    bolts = Random(3) + 2;
    tries = 0;
    while (bolts > 0 && tries < 30) {
        tries++;
        i = Random(battle_unit_count) - 1;
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (p_battle_units[i].owner != caster_player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE && (ench & UNIT_ENCH_WRAITH_FORM) == 0 &&
            Random(2) == 1) {
            Timer_Mark();
            Spell_Damage_Roll(SPELL_WALL_OF_STONE, i, damage, 0);   /* QUIRK #478: spell 3's record (Wall of Stone), not
                                                                       Call Lightning's, sets the bolt's damage */
            Lightning_Strike_Anim(p_battle_units[i].combat_x, p_battle_units[i].combat_y, -1);
            Battle_Apply_Damage(i, damage);
            Wait_Ticks(Random(10) + 3);
            bolts--;
            tries = 0;
        }
    }
    Timer_Mark();
    Wait_Ticks(12);
    Heap_Release(p_scratch);
    Heap_Release(p_world_block);                /* no Heap_Mark here: back to the block's last mark (PROVISIONAL, RC6) */
}

/* (target, damage[3], caster, mana): Life Drain - resisted at -(extra mana / 5); the drained points go in the
   second damage slot and heal a unit caster, or give a wizard caster (20 + player) 3 skill points each
   Numbered: BUG #22, #212 (BUGS_AND_QUIRKS.txt) */
void Life_Drain_Apply(int target, int *damage, int caster, int mana)
{
    int modifier;
    int drained;
    int k;

    for (k = 0; k < 3; k++) {
        damage[k] = 0;
    }
    mana = mana - p_spell_data[SPELL_LIFE_DRAIN].base_cost;
    mana /= 5;
    if (mana < 0) {
        mana = 0;
    }
    modifier = -mana;
    drained = Roll_Resist(p_battle_units[target], modifier, REALM_DEATH);
    if (drained > 0) {
        damage[DAMAGE_DRAIN] = drained;
        if (caster >= CASTER_WIZARD) {
            wizards[caster - CASTER_WIZARD].skill_points += drained * 3;
        } else {
            Battle_Unit_Heal(caster, drained, 1);
        }
    }
}

/* (spell, caster): Wall of Stone (the 16 wall segments), Wall of Fire or Wall of Darkness rising round the city.
   Wall of Stone is overland only in 1.31 (SPELLDAT castable 1), so its branch never runs; it would also stand the
   segments of the centre, where no wall is drawn. clarified using ReMoM repo
   Numbered: BUG #299, #326 (BUGS_AND_QUIRKS.txt) */
void Combat_Wall_Create(int spell, int caster)
{
    int wall_kind;
    int row;
    int col;
    int frames;

    frames = 10;
    if (spell == SPELL_WALL_OF_STONE) {
        wall_kind = WALL_RISE_STONE;
    } else if (spell == SPELL_WALL_OF_FIRE) {
        wall_kind = WALL_RISE_FIRE;
    } else {
        wall_kind = WALL_RISE_DARKNESS;
    }
    if (wall_kind == WALL_RISE_FIRE) {
        frames -= 2;
    }
    Load_Wall_Rise_Anim(wall_kind);
    combat_wall_rising = 1;
    switch (wall_kind) {
    case WALL_RISE_STONE:
        p_battlefield->has_city_walls = 1;
        for (row = 0; row < 4; row++) {
            for (col = 0; col < 4; col++) {
                p_battlefield->wall_segments[row][col] = WALL_STANDING;
            }
        }
        break;
    case WALL_RISE_FIRE:
        p_battlefield->wall_of_fire = 1;
        break;
    case WALL_RISE_DARKNESS:
        p_battlefield->wall_of_darkness = 1;
        break;
    }
    for (combat_wall_anim_frame = 0; combat_wall_anim_frame < frames; combat_wall_anim_frame++) {
        Set_Draw_Page();
        Combat_Draw_Screen();
        Combat_Cast_Banner(caster, spell);
        Screen_Present();
    }
    combat_wall_rising = 0;
}

/* (caster, blocker, spell, blocker's spell name): "<X>'s <Counter Magic> spell has caused <Y>'s <Z> spell to
   fizzle." - blocker 5 is the node ("The power of the Sorcery Node ..."); caster 0..19 a unit (a hero by name),
   20 + player a wizard; a message box, or 30 frames of the banner in auto combat */
void Spell_Fizzle_Message(int caster, int blocker, int spell, char *blocker_spell)
{
    int len;
    int k;
    int slot;
    char name[20];

    if (blocker != PLAYER_NEUTRAL) {
        if (blocker == PLAYER_HUMAN) {
            strcpy(message_text, "Your ");
        } else {
            strcpy(message_text, wizards[blocker].wizard_name);
            len = strlen(wizards[blocker].wizard_name);
            if (wizards[blocker].wizard_name[len - 1] == 's') {
                strcat(message_text, "' ");
            } else {
                strcat(message_text, "'s ");
            }
        }
        strcat(message_text, blocker_spell);
        strcat(message_text, " spell has caused ");
    } else {
        strcpy(message_text, "The power of the ");
        switch (p_battlefield->backdrop) {
        case BACKDROP_SORCERY_NODE:
            strcat(message_text, "Sorcery");
            break;
        case BACKDROP_CHAOS_NODE:
            strcat(message_text, "Chaos");
            break;
        case BACKDROP_NATURE_NODE:
            strcat(message_text, "Nature");
            break;
        }
        strcat(message_text, " Node has caused ");
    }
    if (caster == CASTER_WIZARD + PLAYER_HUMAN) {
        strcat(message_text, "your ");
    } else if (caster < CASTER_WIZARD) {
        slot = p_units[p_battle_units[caster].unit_index].hero_slot;
        if (slot > -1) {
            strcpy(name, wizards[p_battle_units[caster].owner].hired_heroes[slot].name);
            strcat(message_text, name);
            len = strlen(name);
            if (name[len - 1] == 's') {
                strcat(message_text, "' ");
            } else {
                strcat(message_text, "'s ");
            }
        } else {
            strcat(message_text, "the ");
            strcat(message_text, unit_types[p_units[p_battle_units[caster].unit_index].type].name);
            len = strlen(unit_types[p_units[p_battle_units[caster].unit_index].type].name);
            if (unit_types[p_units[p_battle_units[caster].unit_index].type].name[len - 1] == 's') {
                strcat(message_text, "' ");
            } else {
                strcat(message_text, "'s ");
            }
        }
    } else {
        strcat(message_text, wizards[caster - CASTER_WIZARD].wizard_name);
        len = strlen(&wizards[caster - CASTER_WIZARD].wizard_name[0]);
        if (wizards[caster - CASTER_WIZARD].wizard_name[len - 1] == 's') {
            strcat(message_text, "' ");
        } else {
            strcat(message_text, "'s ");
        }
    }
    far_strcpy(name, 0, p_spell_data[spell].name);
    strcat(message_text, name);
    strcat(message_text, " spell to fizzle.");
    if (combat_auto == 0) {
        Warning_Box(message_text);
    } else {
        Play_Message_Sound();
        for (k = 0; k < 30; k++) {
            Timer_Mark();
            Set_Draw_Page();
            Combat_Draw_Screen();
            Combat_Draw_Banner_Text();
            Screen_Present();
            Wait_Ticks(2);
        }
    }
}

/* (strength, cost, owner, realm): 1 when a dispel of this strength removes a spell of this cost (owner's dispel
   resistance: realm mastery, Archmage): Random(250) <= 250 * strength / (strength + resistance) */
int Dispel_Roll(int strength, int cost, int owner, int realm)
{
    int total;
    int chance;

    total = Spell_Dispel_Resistance(cost, owner, realm) + strength;
    chance = (long)strength * 250 / total;
    if (Random(250) <= chance) {
        return 1;
    } else {
        return 0;
    }
}

/* (x, y, caster): Cracks Call - every frame of p_anim_pic at the square, with the cast banner */
void Cracks_Call_Anim(int x, int y, int caster)
{
    int i;

    if (p_sfx_buf != -1) {
        Sound_Play(p_sfx_buf);
    }
    combat_special_anim_x = x;
    combat_special_anim_y = y;
    combat_effect_pic = p_anim_pic;
    combat_special_anim = 1;
    for (i = 0; i < Pic_Frame_Count(p_anim_pic); i++) {
        combat_effect_anim_frame = i;
        Set_Draw_Page();
        Combat_Draw_Screen();
        Combat_Cast_Banner(caster, SPELL_CRACKS_CALL);
        Screen_Present();
    }
    combat_special_anim = 0;
}

/* (x, y, spell, caster): Ice Bolt, Fire Bolt, Fireball or Doom Bolt - the bolt flies in from the upper right (Doom
   Bolt drops straight down) for 11 frames (Fire Bolt and Ice Bolt only fly, the last frame showing the
   hit picture 3); Fireball (5 more frames) and Doom Bolt (2) then play on the square */
void Bolt_Spell_Anim(int x, int y, int spell, int caster)
{
    int frames;
    int sx;
    int sy;
    int frame;
    int dx;
    int dy;

    Combat_Grid_To_Screen(x, y, 4, 4, &sx, &sy);
    switch (spell) {
    case SPELL_FIREBALL:
        sx -= 14;
        sy -= 21;
        x = sx + 110;
        y = sy - 66;
        dx = -10;
        dy = 6;
        frames = 16;
        break;
    case SPELL_FIRE_BOLT:
        sx -= 16;
        sy -= 20;
        x = sx + 110;
        y = sy - 66;
        dx = -10;
        dy = 6;
        frames = 11;
        break;
    case SPELL_ICE_BOLT:
        sx -= 17;
        sy -= 20;
        x = sx + 110;
        y = sy - 110;
        dx = -10;
        dy = 10;
        frames = 11;
        break;
    case SPELL_DOOM_BOLT:
        sx -= 14;
        sy -= 25;
        x = sx;
        y = sy - 72;
        dx = 0;
        dy = 6;
        frames = 13;
        break;
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Combat_Cast_Banner(caster, spell);
    Screen_Save_A800();
    Heap_Mark(p_scratch);
    Spell_Load_Anim(spell);
    if (p_sfx_buf != -1) {
        Sound_Play(p_sfx_buf);
    }
    for (frame = 0; frame < frames; frame++) {
        Timer_Mark();
        if (spell == SPELL_FIREBALL) {
            Set_Pic_Frame(p_anim_pic, frame);
        } else if (spell == SPELL_DOOM_BOLT) {
            Set_Pic_Frame(p_anim_pic, frame / 4);
        } else if (frames - 1 == frame) {
            Set_Pic_Frame(p_anim_pic, 3);
        } else {
            Set_Pic_Frame(p_anim_pic, frame % 3);
        }
        Set_Draw_Page();
        Screen_Restore_A800();
        if (frame > 10) {
            Draw_Pic_Clipped(sx, sy, p_anim_pic);
            Screen_Present();
            Wait_Ticks(2);
        } else {
            Draw_Pic_Clipped(x, y, p_anim_pic);
            x += dx;
            y += dy;
            Screen_Present();
            Wait_Ticks(1);
        }
    }
    Heap_Release(p_scratch);
    Battlefield_Draw_Terrain();
}

/* (x, y, big, caster, spell): every frame of p_anim_pic on the square (big: a 2x2 square picture), 2 ticks a
   frame, with the cast banner */
void Spell_Anim_On_Square(int x, int y, int big, int caster, int spell)
{
    int i;
    int count;
    int sx;
    int sy;

    Combat_Grid_To_Screen(x, y, 4, 4, &sx, &sy);
    if (big == 0) {
        sx -= 14;
        sy -= 25;
    } else {
        sx -= 28;
        sy -= 30;
    }
    if (p_sfx_buf != -1) {
        Sound_Play(p_sfx_buf);
    }
    count = Pic_Frame_Count(p_anim_pic);
    Pic_Reset_Frame(p_anim_pic);
    for (i = 0; i < count; i++) {
        Timer_Mark();
        Combat_Draw_Screen();
        Combat_Cast_Banner(caster, spell);
        Draw_Pic_Clipped(sx, sy, p_anim_pic);
        Screen_Present();
        Wait_Ticks(2);
    }
}

/* (unit, x, y): a Teleporting move - the unit dissolves out of its square and into (x, y) (steps of 20, of 50
   with movement animations off) */
void Combat_Teleport_Anim(int unit, int x, int y)
{
    int fade;
    int from_x;
    int from_y;
    int to_x;
    int to_y;
    int old_x;
    int old_y;
    int sound;
    int step;

    Combat_Grid_To_Screen(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y, 4, 4, &from_x, &from_y);
    Combat_Grid_To_Screen(x, y, 4, 4, &to_x, &to_y);
    from_x -= 13;
    to_x -= 13;
    from_y -= 27;
    to_y -= 27;
    if (game_config.movement_animations == 0) {
        step = 50;
    } else {
        step = 20;
    }
    old_x = p_battle_units[unit].combat_x;
    old_y = p_battle_units[unit].combat_y;
    Heap_Mark(p_world_block);
    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        sound = LBX_Reload_Next("soundfx", SFX_TELEPORT, p_world_block);
        Sound_Play(sound);
    }
    Combat_Build_Unit_Figures_Pic(unit);
    p_battle_units[unit].status = BATTLE_UNIT_DEAD;   /* hidden from the battlefield draw while the animation
                                                   draws it; ACTIVE again afterwards */
    Heap_Release(p_world_block);
    for (fade = 150; fade > 0; fade -= step) {
        Timer_Mark();
        Set_Draw_Page();
        Combat_Draw_Screen();
        if (fade > 50) {
            Pic_Create_Blank(45, 42, p_pic_work);
            Pic_Copy(p_pic_work, combat_figure_pic);
            Pic_Dissolve(p_pic_work, fade - 50);
            Pic_Set_Remap_Flag(p_pic_work);
            Draw_Bitmap_Clipped(from_x, from_y, p_pic_work);
        }
        if (fade <= 100) {
            Pic_Create_Blank(45, 42, p_pic_work);
            Pic_Copy(p_pic_work, combat_figure_pic);
            Pic_Dissolve(p_pic_work, 100 - fade);
            Pic_Set_Remap_Flag(p_pic_work);
            Draw_Bitmap_Clipped(to_x, to_y, p_pic_work);
        }
        Screen_Present();
        Wait_Ticks(2);
    }
    p_battle_units[unit].status = BATTLE_UNIT_ACTIVE;
    p_battle_units[unit].combat_x = x;
    p_battle_units[unit].combat_y = y;
    p_battle_units[unit].dest_x = p_battle_units[unit].dest_x - (old_x - x);
    p_battle_units[unit].dest_y = p_battle_units[unit].dest_y - (old_y - y);
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
}

/* (unit, x, y): a Merging move - the unit sinks into its square and rises out of (x, y), 4 + 5 frames */
void Combat_Merge_Anim(int unit, int x, int y)
{
    int step;
    int from_x;
    int from_y;
    int to_x;
    int to_y;
    int old_x;
    int old_y;
    int sound;

    Combat_Grid_To_Screen(p_battle_units[unit].combat_x, p_battle_units[unit].combat_y, 4, 4, &from_x, &from_y);
    Combat_Grid_To_Screen(x, y, 4, 4, &to_x, &to_y);
    from_x -= 13;
    to_x -= 13;
    from_y -= 27;
    to_y -= 27;
    old_x = p_battle_units[unit].combat_x;
    old_y = p_battle_units[unit].combat_y;
    Heap_Mark(p_world_block);
    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        sound = LBX_Reload_Next("soundfx", SFX_TUNNELING, p_world_block);
        Sound_Play(sound);
    }
    Combat_Build_Unit_Figures_Pic(unit);
    Heap_Release(p_world_block);
    p_battle_units[unit].status = BATTLE_UNIT_DEAD;   /* hidden from the battlefield draw while the animation
                                                   draws it; ACTIVE again afterwards */
    for (step = 4; step > 0; step--) {
        Timer_Mark();
        Set_Draw_Page();
        Combat_Draw_Screen();
        Pic_Create_Blank(45, 42, p_pic_work);
        Pic_Copy(p_pic_work, combat_figure_pic);
        Pic_Dissolve(p_pic_work, step * 20);
        Set_Clip_Rect(0, 0, 319, from_y + 30);
        Pic_Set_Remap_Flag(p_pic_work);
        Draw_Bitmap_Clipped(from_x, from_y + (5 - step) * 5, p_pic_work);
        Reset_Clip_Rect();
        Screen_Present();
        Wait_Ticks(2);
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
    for (step = 0; step < 5; step++) {
        Timer_Mark();
        Set_Draw_Page();
        Combat_Draw_Screen();
        Pic_Create_Blank(45, 42, p_pic_work);
        Pic_Copy(p_pic_work, combat_figure_pic);
        Pic_Dissolve(p_pic_work, step * 20);
        Set_Clip_Rect(0, 0, 319, to_y + 30);
        Pic_Set_Remap_Flag(p_pic_work);
        Draw_Bitmap_Clipped(to_x, to_y + 20 - step * 5, p_pic_work);
        Reset_Clip_Rect();
        Screen_Present();
        Wait_Ticks(2);
    }
    p_battle_units[unit].status = BATTLE_UNIT_ACTIVE;
    p_battle_units[unit].combat_x = x;
    p_battle_units[unit].combat_y = y;
    p_battle_units[unit].dest_x = p_battle_units[unit].dest_x - (old_x - x);
    p_battle_units[unit].dest_y = p_battle_units[unit].dest_y - (old_y - y);
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
}

/* (unit, x, y, spell, caster): a summoned unit rising out of the ground at (x, y) under the summoning picture (the
   Fire Elemental's animation), 16 frames with the cast banner */
void Combat_Summon_Anim(int unit, int x, int y, int spell, int caster)
{
    int frame;
    int sx;
    int sy;
    int ex;
    int ey;

    combat_special_anim_x = p_battle_units[unit].combat_x;
    combat_special_anim_y = p_battle_units[unit].combat_y;
    Combat_Grid_To_Screen(x, y, 4, 4, &sx, &sy);
    ex = sx - 14;
    ey = sy - 25;
    sx -= 13;
    sy -= 27;
    Heap_Mark(p_scratch);
    Spell_Load_Anim(SPELL_FIRE_ELEMENTAL);
    combat_effect_pic = p_anim_pic;
    combat_special_anim = 1;
    Heap_Mark(p_world_block);
    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        p_sfx_buf = LBX_Reload_Next("SOUNDFX", SFX_COMBAT_SUMMONING, p_world_block);
    } else {
        p_sfx_buf = -1;
    }
    if (p_battle_units[unit].owner == combat_attacker_player) {   /* the summoning square: (8 / 14, 12) by side */
        p_battle_units[unit].dest_x = 8;
    } else {
        p_battle_units[unit].dest_x = 14;
    }
    p_battle_units[unit].dest_y = 12;
    Combat_Build_Unit_Figures_Pic(unit);
    p_battle_units[unit].status = BATTLE_UNIT_DEAD;   /* hidden from the battlefield draw while the animation
                                                   draws it; ACTIVE again afterwards */
    if (p_sfx_buf != -1) {
        Sound_Play(p_sfx_buf);
    }
    for (frame = 0; frame < 16; frame++) {
        combat_effect_anim_frame = frame;
        Timer_Mark();
        if (frame == 14) {
            p_battle_units[unit].status = BATTLE_UNIT_ACTIVE;
        }
        Set_Draw_Page();
        Combat_Draw_Screen();
        Combat_Cast_Banner(caster, spell);
        Pic_Create_Blank(45, 42, p_pic_work);
        Pic_Copy(p_pic_work, combat_figure_pic);
        if (frame > 6 && frame < 14) {
            Pic_Dissolve(p_pic_work, (frame - 6) * 14);
            Set_Clip_Rect(0, 0, 319, sy + 30);
            Pic_Set_Remap_Flag(p_pic_work);
            Draw_Bitmap_Clipped(sx, sy + 21 - (frame - 6) * 3, p_pic_work);
            Reset_Clip_Rect();
        }
        Screen_Present();
        Wait_Ticks(2);
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
    combat_special_anim = 0;
    Heap_Release(p_scratch);
    Heap_Release(p_world_block);
}

/* (x, y, caster): a Lightning Bolt strike - 5 plain frames, 2 with a random bolt picture above the square (the
   sound with the first), 5 plain frames; the cast banner while caster >= 0 */
void Lightning_Strike_Anim(int x, int y, int caster)
{
    int i;
    int sx;
    int sy;

    for (i = 0; i < 5; i++) {
        Combat_Draw_Screen();
        if (caster > -1) {
            Combat_Cast_Banner(caster, SPELL_LIGHTNING_BOLT);
        }
        Screen_Present();
    }
    for (i = 0; i < 2; i++) {
        Set_Draw_Page();
        Combat_Draw_Screen();
        if (caster > -1) {
            Combat_Cast_Banner(caster, SPELL_LIGHTNING_BOLT);
        }
        Combat_Grid_To_Screen(x, y, 4, 4, &sx, &sy);
        if (i == 0 && game_config.sound_effects == 1) {
            Sound_Play(p_sfx_buf);
        }
        Set_Pic_Frame(p_anim_pic, Random(4) - 1);
        Draw_Pic_Clipped(sx, sy - 199, p_anim_pic);
        Screen_Present();
    }
    for (i = 0; i < 5; i++) {
        Combat_Draw_Screen();
        if (caster > -1) {
            Combat_Cast_Banner(caster, SPELL_LIGHTNING_BOLT);
        }
        Screen_Present();
    }
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
}

/* (caster player, x, y): a new Magic Vortex at (x, y) */
void Vortex_Create(int caster_player, int x, int y)
{
    p_vortices[vortex_count].x = x;
    p_vortices[vortex_count].y = y;
    p_vortices[vortex_count].dest_x = x;
    p_vortices[vortex_count].dest_y = y;
    p_vortices[vortex_count].anim_step = 0;
    p_vortices[vortex_count].owner = caster_player;
    vortex_count++;
}

/* (vortex): the steering cursor - the marker on the vortex; the move cursor (7) over a square next to it that a
   flyer can enter, else the plain one (3); one full-screen cursor region */
void Vortex_Steer_Cursor(int vortex)
{
    int gx;
    int mx;
    int my;
    int gy;
    int ofs;

    p_combat_cursor_region[MREG_IMAGE] = POINTER_REFUSED;
    ofs = 4;
    mx = Mouse_X() + ofs;
    my = Mouse_Y() + ofs;
    combat_active_marker_on = 1;
    combat_active_marker_x = p_vortices[vortex].x;
    combat_active_marker_y = p_vortices[vortex].y;
    combat_cursor_anim = (combat_cursor_anim + 1) % 3;
    if (my < ofs + 168) {
        combat_cursor_mode = 0;
        gx = Combat_Screen_To_Grid_X(mx, my);
        gy = Combat_Screen_To_Grid_Y(mx, my);
        if ((p_vortices[vortex].x != gx || p_vortices[vortex].y != gy) &&
            p_battlefield->cost_flying[gy][gx] != COST_IMPASSABLE && abs(gx - p_vortices[vortex].x) <= 1 &&
            abs(gy - p_vortices[vortex].y) <= 1) {
            combat_cursor_mode = 1;
            combat_target_x = gx;
            combat_target_y = gy;
            p_combat_cursor_region[MREG_IMAGE] = POINTER_MOVE;
        }
    }
    p_combat_cursor_region[MREG_POINTER_OFS] = 2;
    p_combat_cursor_region[MREG_X1] = 0;
    p_combat_cursor_region[MREG_Y1] = 0;
    p_combat_cursor_region[MREG_X2] = 319;
    p_combat_cursor_region[MREG_Y2] = 199;
    Mouse_Set_Cursor_Regions(1, (struct MouseRegion *)p_combat_cursor_region);
}

/* (vortex): the human steers his vortex one square (not in auto combat) */
void Vortex_Human_Move(int vortex)
{
    int gx;
    int done;
    int input;
    int grid;
    int mx;
    int my;
    int gy;

    if (p_vortices[vortex].owner == PLAYER_HUMAN && combat_auto == 0) {
        UI_Clear_Controls();
        grid = UI_Add_Grid_Control(0, 0, 1, 1, 319, 168, &mx, &my, -1);
        done = 0;
        combat_active_unit = -1;
        while (done == 0) {
            Timer_Mark();
            input = UI_Get_Input();
            if (input == grid) {
                gx = Combat_Screen_To_Grid_X(mx + 4, my + 4);
                gy = Combat_Screen_To_Grid_Y(mx + 4, my + 4);
                if ((p_vortices[vortex].x != gx || p_vortices[vortex].y != gy) &&
                    p_battlefield->cost_flying[gy][gx] != COST_IMPASSABLE && abs(gx - p_vortices[vortex].x) <= 1 &&
                    abs(gy - p_vortices[vortex].y) <= 1) {
                    Vortex_Move(vortex, gx, gy);
                    done = 1;
                }
            }
            if (done == 0) {
                Combat_Draw_Screen();
                Vortex_Steer_Cursor(vortex);
                Screen_Present();
                Wait_Ticks(2);
            }
        }
    }
}

/* each vortex makes 3 random moves (away from where it came from, at most 50 tries), then the human may steer
   his */
void Vortices_Turn(void)
{
    int vortex;
    int nx;
    int step;
    int ny;
    int ox;
    int oy;
    int tries;

    if (game_config.sound_effects == 1) {
        Sound_Play(combat_silence_sfx);
        Heap_Mark(p_world_block);
        p_sfx_buf = LBX_Reload_Next("soundfx", SFX_FLYERS_SMALL, p_world_block);
        Heap_Release(p_world_block);
    }
    for (vortex = 0; vortex < vortex_count; vortex++) {
        for (step = 0; step < 3; step++) {
            ox = p_vortices[vortex].x;
            oy = p_vortices[vortex].y;
            nx = ox;
            ny = oy;
            tries = 0;
            /* BUG #479: > lets column BATTLEFIELD_W and row BATTLEFIELD_H through, and cost_flying is read before the
               bounds tests */
            while ((nx == ox && ny == oy) || p_battlefield->cost_flying[ny][nx] == COST_IMPASSABLE || nx < 0 ||
                   nx > BATTLEFIELD_W || ny < 0 || ny > BATTLEFIELD_H ||
                   (abs(nx - p_vortices[vortex].dest_x) + abs(ny - p_vortices[vortex].dest_y) <= 1 && tries < 50)) {
                tries++;
                nx = ox + Random(3) - 2;
                ny = oy + Random(3) - 2;
            }
            Vortex_Move(vortex, nx, ny);
        }
        p_vortices[vortex].dest_x = p_vortices[vortex].x;
        p_vortices[vortex].dest_y = p_vortices[vortex].y;
        Vortex_Human_Move(vortex);
    }
}

/* (vortex, x, y): the vortex moves to (x, y) - 5 points of damage to a unit there (not one with Magic Immunity, the
   ability, nor under Righteousness, UNIT_ENCH_RIGHTEOUSNESS), a Lightning Bolt to each unit next to it 1 time in 3; entering the city adds 5 to the city
   damage counter */
void Vortex_Move(int vortex, int x, int y)
{
    int i;
    int dx;
    int dy;
    int damage[3];
    long ench;
    int ox;
    int oy;

    ox = p_vortices[vortex].x;
    oy = p_vortices[vortex].y;
    combat_active_marker_on = 0;
    p_vortices[vortex].dest_x = x;
    p_vortices[vortex].dest_y = y;
    if (game_config.sound_effects == 1) {
        Sound_Play(p_sfx_buf);
    }
    for (i = 0; i < 8; i++) {
        p_vortices[vortex].anim_step = i;
        Set_Draw_Page();
        Combat_Draw_Screen();
        Screen_Present();
    }
    p_vortices[vortex].x = x;
    p_vortices[vortex].y = y;
    if (x >= BF_CITY_X1 && x <= BF_CITY_X2 && y >= BF_CITY_Y1 && y <= BF_CITY_Y2) {
        city_damage_counter += 5;
    }
    p_vortices[vortex].dest_x = ox;
    p_vortices[vortex].dest_y = oy;
    p_vortices[vortex].anim_step = 0;
    for (i = 0; i < battle_unit_count; i++) {
        ench = p_units[p_battle_units[i].unit_index].enchantments | p_battle_units[i].combat_enchantments |
               p_battle_units[i].item_enchantments;
        if (p_battle_units[i].combat_x == x && p_battle_units[i].combat_y == y &&
            (p_battle_units[i].abilities & ABILITY_MAGIC_IMMUNITY) == 0 && (ench & UNIT_ENCH_RIGHTEOUSNESS) == 0) {
            damage[DAMAGE_NORMAL] = 5;
            damage[DAMAGE_DRAIN] = 0;
            damage[DAMAGE_DESTROY] = 0;
            Battle_Apply_Damage(i, damage);
        }
        dx = abs(p_battle_units[i].combat_x - x);
        dy = abs(p_battle_units[i].combat_y - y);
        if (dx <= 1 && dy <= 1 && dx + dy != 0 && Random(3) == 1) {
            Spell_Damage_Roll(SPELL_LIGHTNING_BOLT, i, damage, 0);
            Battle_Apply_Damage(i, damage);
        }
    }
}

/* (caster, animate): Call Chaos - an effect 0..7 for every unit (the caster's own and dead ones show nothing),
   applied half way through the effect pictures' animation. BUG #13: without the animation the effects are applied
   at once from effects[] before it is rolled (uninitialised)
   Numbered: BUG #13 (BUGS_AND_QUIRKS.txt) */
int Call_Chaos_Cast(int caster, int animate)
{
    int i;
    int frame;
    int max_frames;
    int n;
    int player;
    int x;
    int y;
    int effects[36];
    unsigned pics[8];
    int start[36];
    int unit_x[36];
    int unit_y[36];

    player = caster > CASTER_WIZARD - 1 ? caster - CASTER_WIZARD : p_battle_units[caster].owner;
    max_frames = 0;
    if (animate == 0) {
        Call_Chaos_Apply(player, effects);
        return 0;
    }
    Screen_Copy_Front();
    Screen_Save_A800();
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            start[i] = Random(5) - 1;
            unit_x[i] = p_battle_units[i].combat_x;
            unit_y[i] = p_battle_units[i].combat_y;
        } else {
            start[i] = -1;
        }
    }
    Battlefield_Draw_Terrain();
    Set_Draw_Page();
    Combat_Draw_Screen();
    Screen_Present();
    Screen_Copy_Front();
    Screen_Save_A800();
    Scratch_Save_To_EMS();
    pics[0] = LBX_Reload("monster", 47, p_scratch);
    pics[1] = LBX_Reload_Next("specfx", 3, p_scratch);
    pics[2] = LBX_Reload_Next("specfx", 2, p_scratch);
    pics[3] = pics[2];
    pics[4] = LBX_Reload_Next("cmbtfx", 33, p_scratch);
    pics[5] = LBX_Reload_Next("cmbtfx", 3, p_scratch);
    pics[6] = pics[2];
    pics[7] = LBX_Reload_Next("cmbtfx", 4, p_scratch);
    for (i = 0; i < battle_unit_count; i++) {
        effects[i] = Random(8) - 1;
        n = Pic_Frame_Count(pics[effects[i]]);
        if (n > max_frames) {
            max_frames = n;
        }
    }
    if (game_config.sound_effects == 1) {
        Combat_Load_Spell_Sound(SPELL_CALL_CHAOS);
        if (p_sfx_buf != -1) {
            Sound_Play(p_sfx_buf);
        }
    }
    for (frame = 0; frame < max_frames + 4; frame++) {
        Timer_Mark();
        Set_Draw_Page();
        if ((max_frames + 4) / 2 == frame) {
            Scratch_Restore_From_EMS();
            Call_Chaos_Apply(player, effects);
            Set_Draw_Page();
            Combat_Draw_Screen();
            Combat_Cast_Banner(caster, SPELL_CALL_CHAOS);
            Screen_Save_A800();
            Scratch_Save_To_EMS();
            pics[0] = LBX_Reload("monster", 47, p_scratch);
            pics[1] = LBX_Reload_Next("specfx", 3, p_scratch);
            pics[2] = LBX_Reload_Next("specfx", 2, p_scratch);
            pics[3] = pics[2];
            pics[4] = LBX_Reload_Next("cmbtfx", 33, p_scratch);
            pics[5] = LBX_Reload_Next("cmbtfx", 3, p_scratch);
            pics[6] = pics[2];
            pics[7] = LBX_Reload_Next("cmbtfx", 4, p_scratch);
        }
        Screen_Restore_A800();
        for (i = 0; i < battle_unit_count; i++) {
            if (start[i] > -1) {
                Combat_Grid_To_Screen(unit_x[i], unit_y[i], 4, 4, &x, &y);
                if (effects[i] == 2 || effects[i] == 3 || effects[i] == 6) {   /* they share pics[2] (SPECFX 2):
                                                   the larger offset */
                    x -= 28;
                    y -= 30;
                } else {
                    x -= 14;
                    y -= 27;
                }
                if (frame - start[i] < max_frames && frame - start[i] >= 0 &&
                    Pic_Frame_Count(pics[effects[i]]) > frame - start[i]) {
                    Set_Pic_Frame(pics[effects[i]], frame - start[i]);
                    Draw_Pic_Clipped(x, y, pics[effects[i]]);
                }
            }
        }
        Screen_Present();
        Wait_Ticks(2);
    }
    Scratch_Restore_From_EMS();
    Battlefield_Draw_Terrain();
}

/* (player, effects[]): Call Chaos on every enemy unit alive - 0 nothing, 1 heals 5 (not Death creatures), 2 Chaos Channels
   (normal living units), 3 Warp Creature (resist -1), 4 a strength 15 Fire Bolt, 5 Warp Lightning, 6 Doom Bolt,
   7 disintegrated below 10 resistance (BUG #17: on a resist the damage array of the previous unit is applied)
   Numbered: BUG #17 (BUGS_AND_QUIRKS.txt) */
void Call_Chaos_Apply(int player, int *effects)
{
    int i;
    int damage[3];
    int resist_roll;

    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner != player && p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            switch (effects[i]) {
            case CALL_CHAOS_NOTHING:
                break;
            case CALL_CHAOS_HEAL:
                if (p_battle_units[i].race != RACE_DEATH_CREATURE) {
                    Battle_Unit_Heal(i, 5, 0);
                }
                break;
            case CALL_CHAOS_CHAOS_CHANNELS:
                if (Unit_Is_Normal_Living(p_battle_units[i].unit_index)) {
                    Chaos_Channels_Apply(p_battle_units[i].unit_index);
                }
                break;
            case CALL_CHAOS_WARP_CREATURE:
                resist_roll = Roll_Resist(p_battle_units[i], -1, REALM_CHAOS);
                if (resist_roll > 0) {
                    Warp_Creature_Apply(i);
                }
                break;
            case CALL_CHAOS_FIRE_BOLT:
                Spell_Damage_Roll(SPELL_FIRE_BOLT, i, damage, 15);
                Battle_Apply_Damage(i, damage);
                break;
            case CALL_CHAOS_WARP_LIGHTNING:
                Spell_Damage_Roll(SPELL_WARP_LIGHTNING, i, damage, 0);
                Battle_Apply_Damage(i, damage);
                break;
            case CALL_CHAOS_DOOM_BOLT:
                Spell_Damage_Roll(SPELL_DOOM_BOLT, i, damage, 0);
                Battle_Apply_Damage(i, damage);
                break;
            case CALL_CHAOS_DISINTEGRATE:
                if (Resist_Bonus(p_battle_units[i], REALM_CHAOS) < 10) {
                    damage[DAMAGE_NORMAL] = 0;
                    damage[DAMAGE_DRAIN] = 0;
                    damage[DAMAGE_DESTROY] = 200;
                }
                Battle_Apply_Damage(i, damage);
                break;
            }
        }
    }
}
