/* OVERSPEL.C - WIZARDS.EXE overlay code segment b425 (stub 25b8): overland spells cast on the map and their
   per-turn effects - Summon Hero / Summon Champion, Incarnation, Resurrection and the hero spawn they share; Chaos
   Rift, Stream of Life, Gaia's Blessing and Nightshade on a city; Meteor Storm, Great Wasting and Armageddon each
   turn; Plane Shift (its checks and the square's dissolve animation), Floating Island, Lycanthropy, Wall of Stone
   and Spell of Return.
   This module's data (DS:68a0..68fb): its literals only; no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS b425 src/wizards/OVERSPEL.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
int Summon_Hero_Cast(int player, int pool);
int Incarnation_Cast(int player);
void City_Chaos_Rift_Turn(int city);
void Meteor_Storm_Turn(int caster);
void City_Stream_Of_Life_Turn(int city);
int Hero_Spawn(int player, int hero, int slot, int from_dead);
int Resurrection_Cast(int player);
int Plane_Shift_Check(int x, int y, int plane);
void Plane_Shift_Grab_Tile_Pic(int x, int y, int plane);
int Plane_Shift_Cast(int player);
void Great_Wasting_Turn(int caster);
void City_Gaias_Blessing_Turn(int city);
void Armageddon_Turn(int caster);
void Floating_Island_Cast(int player);
void City_Nightshade_Dispel(int city);
int Lycanthropy_Cast(int player);
int Wall_Of_Stone_Cast(int player);
int Spell_Of_Return_Cast(int player);

#include "GAME.H"

extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern char report_buildings_lost_count;              /* DS:9b42 */
extern struct ReportBldgLost report_buildings_lost[]; /* DS:9b44 */
extern char report_units_killed_count;          /* DS:9be6 */
extern struct ReportUnitKilled report_units_killed[];   /* DS:9be8 */
extern int present_mode;                        /* DS:9c8e */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern struct HeroName far *p_hero_names;       /* DS:9ec6 */
extern int next_unit_ref_y;                     /* DS:bd82 */
extern int next_unit_ref_x;                     /* DS:bd84 */
extern int award_loc_plane;                     /* DS:c300 */
extern int award_loc_y;                         /* DS:c302 */
extern int award_loc_x;                         /* DS:c304 */
extern int award_at_location;                   /* DS:c306 */
extern int ai_replan;                           /* DS:c9f0 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
void far_strcpy(char *dst, int dst_seg, char far *src);
int AI_Overland_Spell_Target(int target_kind, int *x, int *y, int *plane, int spell, int caster);
int AI_Pick_City_Spell_Target(int target_kind, int *city, int spell, int caster);
int AI_Pick_Unit_Enchant_Target(int target_kind, int *unit, int spell, int caster);
int CitySlot_To_Spell(int slot);
int CitySpell_Cast_Anim(int city, int spell, int wizard);
void CitySpell_View_Loop();                     /* no prototype here: called with 3 arguments (CASTANIM.C takes 2) */
void City_Collateral_Damage(int city, int pop_lost, int bldg_pct, int *destroyed);
void City_Garrison_List(int city, int *count, int *list);
int City_Spell_Protection_Check(int spell, int city, int caster, int show);
void Draw_Bitmap(int x, int y, unsigned pic);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Hero_Item_Slot_Types(int hero_type, int *slots);
void Hero_Items_Redistribute(int player, int count, int *items);
int Hero_Pick_Random(int player, int mode, int pool);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int Map_Explore_Tile(int x, int y, int plane);
void Map_Full_Redraw(void);
void Map_Redraw(void);
void Map_Spell_Effect_Anim(int x, int y, int plane);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
void Map_Units_At(int x, int y, int plane, int *count, int *list);
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane);
int Mem_Copy(char *dst, char *src, int count);
void Minimap_Alloc_Pic(void);
int Move_Scout_Contacts(int player, int x, int y, int plane, int range);
void Pic_Copy(unsigned dst, unsigned src);
void Pic_Dissolve(unsigned pic, int amount);
int Resurrection_Pick_Hero(int n, int *hero_types);
void Screen_Grab_Rect(int x1, int y1, int x2, int y2, unsigned pic);
int Screen_Hire_Hero(int slot, int hero_type, int offer_kind);
void Select_Stack_Of_Unit();                    /* no prototype here: called with no argument (MAINSCR.C takes 1) */
void Set_Draw_Page_Front(void);
int Sound_Play(unsigned sound);
int Spell_Load_Anim(int spell);
int Spell_Dispel_Resistance(int strength, int player, int realm);
void Spell_Load_Sound(int spell);
int Spell_Target_Map_Select(int target_type, int *target, int *y, int *plane, int *screen_x, int *screen_y,
                            char *spell_name);
void Stack_Limit_Enforce(int unit);
void Stack_Movement_Modes(int *modes, int *units, int count);
int Stack_Scout_Range(int player, int x, int y, int plane);
void Summon_Complete_Anim(int summoned, int music_tier, int wizard);
int Tile_Is_Corrupted(int x, int y, int plane);
int Tile_Is_Desert(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Volcano(int x, int y, int plane);
int Tile_Is_Water(int x, int y, int plane);
void Tile_Make_Grassland(int x, int y, int plane);
void Tile_Make_Hills(int x, int y, int plane);
void Tile_Make_Volcano(int x, int y, int plane, int owner);
void Tile_Purify(int x, int y, int plane);
int Tile_Visible(int x, int y, int plane);
int Unit_Create(int unit_type, int owner, int x, int y, int plane, int src);
void Unit_Spell_Damage(int unit, int spell, int *n_items, int *items, int strength);
int Wizard_Free_Hero_Slot(int player);
void Wizards_Count_Volcanoes(void);

/* (player, pool): Summon Hero (HPICK_POOL_BASIC) or Summon Champion - a random hero of the pool into a free hero slot (the
   human is offered him); messages 47 (no slot) / 48 (no hero or too many units) */
int Summon_Hero_Cast(int player, int pool)
{
    int hero;
    int slot;
    int hired;

    slot = Wizard_Free_Hero_Slot(player);
    if (slot == -1) {
        if (player == PLAYER_HUMAN) {
            Map_Full_Redraw();
            LBX_Load_Records_Near("message", 0, message_text, 47, 1, 150);
            Message_Box(message_text);
        }
        return 0;
    }
    hero = Hero_Pick_Random(player, HPICK_SUMMONED, pool);
    if (hero == -1 || (num_units >= UNIT_CAP_AI && player != PLAYER_HUMAN) || num_units >= UNIT_CAP) {
        if (player == PLAYER_HUMAN) {
            Map_Full_Redraw();
            LBX_Load_Records_Near("message", 0, message_text, 48, 1, 150);
            Message_Box(message_text);
        }
        return 0;
    }
    if (player == PLAYER_HUMAN) {
        if (p_hero_stats[player][hero].abilities & HERO_ABILITY_FEMALE) {
            if (pool == HPICK_POOL_BASIC) {
                Summon_Complete_Anim(SUMMON_HERO_FEMALE, MUSIC_TIER_VERY_RARE, player);
            } else {
                Summon_Complete_Anim(SUMMON_CHAMPION_FEMALE, MUSIC_TIER_VERY_RARE, player);
            }
        } else {
            if (pool == HPICK_POOL_BASIC) {
                Summon_Complete_Anim(SUMMON_HERO_MALE, MUSIC_TIER_VERY_RARE, player);
            } else {
                Summon_Complete_Anim(SUMMON_CHAMPION_MALE, MUSIC_TIER_VERY_RARE, player);
            }
        }
        Minimap_Alloc_Pic();
        hired = Screen_Hire_Hero(slot, hero, pool == HPICK_POOL_BASIC ? HIRE_OFFER_SUMMONED : HIRE_OFFER_CHAMPION);
        if (hired == 1) {
            next_unit_ref_x = p_units[num_units - 1].x;
            next_unit_ref_y = p_units[num_units - 1].y;
            map_view_plane = p_units[num_units - 1].plane;
            Select_Stack_Of_Unit();
        }
    } else {
        if (game_config.enemy_spells == 1 && wizards[PLAYER_HUMAN].globals.detect_magic > 0 && wizards[PLAYER_HUMAN].contacted[player] == 1) {
            if (p_hero_stats[player][hero].abilities & HERO_ABILITY_FEMALE) {
                Summon_Complete_Anim(SUMMON_HERO_FEMALE, MUSIC_TIER_VERY_RARE, player);
            } else {
                Summon_Complete_Anim(SUMMON_HERO_MALE, MUSIC_TIER_VERY_RARE, player);
            }
        }
        Hero_Spawn(player, hero, slot, 0);
        Stack_Limit_Enforce(num_units - 1);
        p_hero_stats[player][hero].status = HERO_STATUS_UNAVAILABLE;
    }
}

/* (player): Incarnation - Torin the Chosen (hero 34) unless already summoned; messages 49 / 50 / 51 */
int Incarnation_Cast(int player)
{
    int hero;
    int slot;
    int hired;

    slot = Wizard_Free_Hero_Slot(player);
    if (slot == -1) {
        if (player == PLAYER_HUMAN) {
            Map_Full_Redraw();
            LBX_Load_Records_Near("message", 0, message_text, 49, 1, 150);
            Message_Box(message_text);
        }
        return 0;
    }
    if (p_hero_stats[player][UNIT_HERO_CHOSEN].status != HERO_STATUS_UNAVAILABLE) {
        hero = UNIT_HERO_CHOSEN;
    } else {
        hero = -1;
    }
    if (num_units >= UNIT_CAP || (num_units >= UNIT_CAP_AI && player != PLAYER_HUMAN)) {
        if (player == PLAYER_HUMAN) {
            Map_Full_Redraw();
            LBX_Load_Records_Near("message", 0, message_text, 50, 1, 150);
            Message_Box(message_text);
        }
        return 0;
    }
    if (hero == -1) {
        if (player == PLAYER_HUMAN) {
            Map_Full_Redraw();
            LBX_Load_Records_Near("message", 0, message_text, 51, 1, 150);
            Message_Box(message_text);
        }
        return 0;
    }
    if (player == PLAYER_HUMAN) {
        Minimap_Alloc_Pic();
        hired = Screen_Hire_Hero(slot, hero, HIRE_OFFER_SUMMONED);
        if (hired == 1) {
            next_unit_ref_x = wizards[player].summoning_circle_xyp[XYP_X];
            next_unit_ref_y = wizards[player].summoning_circle_xyp[XYP_Y];
            map_view_plane = wizards[player].summoning_circle_xyp[XYP_PLANE];
            Select_Stack_Of_Unit();
        }
    } else {
        Hero_Spawn(player, hero, slot, 0);
        Stack_Limit_Enforce(num_units - 1);
        p_hero_stats[player][hero].status = HERO_STATUS_UNAVAILABLE;
    }
}

/* (city): Chaos Rift - 5 Lightning Bolts at random units of the garrison, then buildings at 5%; the human's losses
   go to the end of turn report */
void City_Chaos_Rift_Turn(int city)
{
    int i;
    int j;
    int pick;
    int count;
    int count_before;
    int k;
    int item_count;
    int garrison[MAX_STACK];
    int before[MAX_STACK];
    int items[18];
    int destroyed[NUM_BUILDINGS];

    City_Garrison_List(city, &count, garrison);
    count_before = count;
    Mem_Copy((char *)before, (char *)garrison, sizeof(garrison));
    item_count = 0;
    for (i = 0; i < 5 && count > 0; i++) {
        pick = Random(count) - 1;
        if (garrison[pick] > -1) {
            Unit_Spell_Damage(garrison[pick], SPELL_LIGHTNING_BOLT, &item_count, items, 8);
        }
        City_Garrison_List(city, &count, garrison);
    }
    award_at_location = 1;
    award_loc_x = p_cities[city].x;
    award_loc_y = p_cities[city].y;
    award_loc_plane = p_cities[city].plane;
    Hero_Items_Redistribute(p_cities[city].owner, item_count, items);
    if (count != count_before && p_cities[city].owner == PLAYER_HUMAN) {
        k = 0;
        for (i = 0; i < count_before; i++) {
            if (garrison[k] != before[i]) {
                if (report_units_killed_count < MAX_REPORT_ENTRIES) {
                    report_units_killed[report_units_killed_count].unit_type = p_units[before[i]].type;
                    report_units_killed[report_units_killed_count].city = city;
                    report_units_killed[report_units_killed_count].spell = SPELL_CHAOS_RIFT;
                    report_units_killed_count = report_units_killed_count + 1;
                }
            } else {
                k++;
            }
        }
    }
    for (j = 0; j < NUM_BUILDINGS; j++) {
        destroyed[j] = 0;
    }
    City_Collateral_Damage(city, 0, 5, destroyed);
    if (p_cities[city].owner == PLAYER_HUMAN) {
        for (j = 0; j < NUM_BUILDINGS; j++) {
            if (destroyed[j] > 0 && report_buildings_lost_count < MAX_REPORT_ENTRIES) {
                report_buildings_lost[report_buildings_lost_count].city = city;
                report_buildings_lost[report_buildings_lost_count].building = destroyed[j];
                report_buildings_lost_count++;
            }
        }
    }
}

/* (caster): Meteor Storm - every city not the caster's that the spell gets through to loses buildings at 1%, every
   unit outside a city (the caster's too) is hit with Call Chaos' damage (SPELL_CALL_CHAOS, 115 - not Meteor
   Storm's own); the human's losses are reported */
void Meteor_Storm_Turn(int caster)
{
    int j;
    char *in_city;
    int i;
    int count;
    int player;
    int item_count;
    int items[3];
    int garrison[MAX_STACK];
    int destroyed[NUM_BUILDINGS];

    in_city = Near_Alloc_Reset(num_units);
    for (i = 0; i < num_units; i++) {
        in_city[i] = 0;
    }
    for (i = 0; i < num_cities; i++) {
        City_Garrison_List(i, &count, garrison);
        for (j = 0; j < count; j++) {
            in_city[garrison[j]] = 1;
        }
        /* caster 0, not caster: the Nightshade roll reads the human's casting_cost_initial */
        if (City_Spell_Protection_Check(SPELL_METEOR_STORM, i, 0, 0) != 0 && p_cities[i].owner != caster) {
            for (j = 0; j < NUM_BUILDINGS; j++) {
                destroyed[j] = 0;
            }
            City_Collateral_Damage(i, 0, 1, destroyed);
            if (p_cities[i].owner == PLAYER_HUMAN) {
                for (j = 0; j < NUM_BUILDINGS; j++) {
                    if (destroyed[j] > 0 && report_buildings_lost_count < MAX_REPORT_ENTRIES) {
                        report_buildings_lost[report_buildings_lost_count].city = i;
                        report_buildings_lost[report_buildings_lost_count].building = destroyed[j];
                        report_buildings_lost_count++;
                    }
                }
            }
        }
    }
    for (i = 0; i < num_units; i++) {
        if (in_city[i] == 0) {
            player = p_units[i].owner;
            item_count = 0;
            award_at_location = 1;
            award_loc_x = p_units[i].x;
            award_loc_y = p_units[i].y;
            award_loc_plane = p_units[i].plane;
            Unit_Spell_Damage(i, SPELL_CALL_CHAOS, &item_count, items, 0);
            Hero_Items_Redistribute(player, item_count, items);
            if (player == PLAYER_HUMAN && p_units[i].owner == PLAYER_NONE && report_units_killed_count < MAX_REPORT_ENTRIES) {
                report_units_killed[report_units_killed_count].unit_type = p_units[i].type;
                report_units_killed[report_units_killed_count].city = -1;
                report_units_killed[report_units_killed_count].spell = SPELL_METEOR_STORM;
                report_units_killed_count = report_units_killed_count + 1;
            }
        }
    }
}

/* (city): Stream of Life - the garrison is healed (not Death creatures - RACE_DEATH_CREATURE, demons too -,
   not MUT_UNDEAD units) */
void City_Stream_Of_Life_Turn(int city)
{
    int i;
    int count;
    int garrison[MAX_STACK];

    City_Garrison_List(city, &count, garrison);
    for (i = 0; i < count; i++) {
        if (unit_types[p_units[garrison[i]].type].race != RACE_DEATH_CREATURE && (p_units[garrison[i]].mutation_flags & MUT_UNDEAD) == 0) {
            p_units[garrison[i]].damage = 0;
        }
    }
}

/* (player, hero, slot, from_dead): the hero created at the player's fortress into hero slot `slot' (item slot
   types, no items); a resurrected hero keeps the human's saved name and experience (AI: level |status| - 1), a new
   one gets a NAMES.LBX name and the experience of his level (at most 6) */
int Hero_Spawn(int player, int hero, int slot, int from_dead)
{
    int k;

    Unit_Create(hero, player, p_fortresses[player].x, p_fortresses[player].y, p_fortresses[player].plane, UNIT_SRC_NONE);
    p_units[num_units - 1].finished = 0;
    p_units[num_units - 1].moves_left = p_units[num_units - 1].moves_max;
    p_units[num_units - 1].hero_slot = slot;
    wizards[player].hired_heroes[slot].unit_index = num_units - 1;
    Hero_Item_Slot_Types(hero, wizards[player].hired_heroes[slot].slot_type);
    for (k = 0; k < 3; k++) {
        wizards[player].hired_heroes[slot].item_index[k] = -1;
    }
    if (from_dead == 1) {
        if (player == PLAYER_HUMAN) {
            far_strcpy(wizards[PLAYER_HUMAN].hired_heroes[slot].name, 0, p_hero_names[hero].name);
            p_units[num_units - 1].experience = p_hero_names[hero].experience;
            p_units[num_units - 1].level = Unit_Calc_Level(num_units - 1);
        } else {
            LBX_Load_Records_Near("NAMES", 0, wizards[player].hired_heroes[slot].name, player * NUM_HERO_TYPES + hero, 1, 13);
            p_units[num_units - 1].level = abs(p_hero_stats[player][hero].status) - 1;
            p_units[num_units - 1].experience = exp_level_thresholds[p_units[num_units - 1].level];
        }
    } else {
        LBX_Load_Records_Near("NAMES", 0, wizards[player].hired_heroes[slot].name, player * NUM_HERO_TYPES + hero, 1, 13);
        if (p_hero_stats[player][hero].status > 6) {
            p_hero_stats[player][hero].status = 6;
        }
        p_units[num_units - 1].level = p_hero_stats[player][hero].status;
        p_units[num_units - 1].experience = exp_level_thresholds[p_hero_stats[player][hero].status];
    }
    return 1;
}

/* (player): Resurrection - a dead hero (status < 0, not HERO_STATUS_UNAVAILABLE) back to life: the human's
   (picked) at the summoning circle, the AI's where Hero_Spawn puts it; messages 50 / 52 / 53 */
int Resurrection_Cast(int player)
{
    int n;
    int k;
    int hero;
    int slot;
    int heroes[34];

    n = 0;
    if (num_units >= UNIT_CAP || (num_units >= UNIT_CAP_AI && player != PLAYER_HUMAN)) {
        Map_Full_Redraw();
        LBX_Load_Records_Near("message", 0, message_text, 50, 1, 150);
        Message_Box(message_text);
        return 0;
    }
    for (k = 0; k < 34; k++) {                  /* every hero type but the last (NUM_HERO_TYPES - 1: Torin) */
        if (p_hero_stats[player][k].status < 0 && p_hero_stats[player][k].status != HERO_STATUS_UNAVAILABLE) {
            heroes[n] = k;
            n++;
        }
    }
    slot = Wizard_Free_Hero_Slot(player);
    if (player == PLAYER_HUMAN) {
        if (n == 0) {
            LBX_Load_Records_Near("message", 0, message_text, 52, 1, 150);
            Message_Box(message_text);
            return 0;
        } else if (slot == -1) {
            LBX_Load_Records_Near("message", 0, message_text, 53, 1, 150);
            Message_Box(message_text);
            return 0;
        } else {
            Minimap_Alloc_Pic();
            Set_Draw_Page();
            Map_Redraw();
            Screen_Present();
            hero = n == 1 ? heroes[0] : Resurrection_Pick_Hero(n, heroes);
            Hero_Spawn(player, hero, slot, 1);
            p_units[num_units - 1].x = wizards[player].summoning_circle_xyp[XYP_X];
            p_units[num_units - 1].y = wizards[player].summoning_circle_xyp[XYP_Y];
            p_units[num_units - 1].plane = wizards[player].summoning_circle_xyp[XYP_PLANE];
            Stack_Limit_Enforce(num_units - 1);
            Spell_Load_Anim(SPELL_RESURRECTION);
            Spell_Load_Sound(SPELL_RESURRECTION);
            Map_Spell_Effect_Anim(wizards[player].summoning_circle_xyp[XYP_X], wizards[player].summoning_circle_xyp[XYP_Y],
                                  wizards[player].summoning_circle_xyp[XYP_PLANE]);
            p_units[num_units - 1].finished = 0;
            p_units[num_units - 1].moves_left = p_units[num_units - 1].moves_max;
            if (player == PLAYER_HUMAN) {
                next_unit_ref_x = p_units[num_units - 1].x;
                next_unit_ref_y = p_units[num_units - 1].y;
                map_view_plane = p_units[num_units - 1].plane;
                Select_Stack_Of_Unit();
            }
        }
    } else {
        if (n == 0) {
            return 0;
        }
        if (slot == -1) {
            return 0;
        }
        hero = heroes[n - 1];
        Hero_Spawn(player, hero, slot, 1);
        Stack_Limit_Enforce(num_units - 1);
    }
    p_hero_stats[player][hero].status = HERO_STATUS_UNAVAILABLE;
}

/* (x, y, plane): can the stack at (x, y, plane) shift planes - 0 yes, 1 onto water (without swimmers / flyers / sailors),
   2 enemy units there, 3 more than 9 units together */
int Plane_Shift_Check(int x, int y, int plane)
{
    int result;
    int count;
    int dest_count;
    int other_plane;
    int units[MAX_STACK];
    int dest_units[MAX_STACK];
    int modes[6];

    result = PLANE_SHIFT_OK;
    other_plane = 1 - plane;
    Map_Units_At(x, y, plane, &count, units);
    Stack_Movement_Modes(modes, units, count);
    if (Tile_Is_Water(x, y, other_plane)) {
        result = PLANE_SHIFT_WATER;
        if (modes[MOVE_MODE_SWIMMING] > 0 || modes[MOVE_MODE_SAILING] > 0 || modes[MOVE_MODE_FLYING] > 0) {
            result = PLANE_SHIFT_OK;
        }
    }
    dest_count = 0;
    Map_Units_At(x, y, other_plane, &dest_count, dest_units);
    if (dest_count > 0) {
        if (p_units[dest_units[0]].owner == p_units[units[0]].owner) {
            if (count + dest_count > MAX_STACK) {
                result = PLANE_SHIFT_FULL;
            }
        } else {
            result = PLANE_SHIFT_ENEMY;
        }
    }
    return result;
}

/* (x, y, plane): the square's picture as it looks with its units moved to the other plane (into p_anim_pic) */
void Plane_Shift_Grab_Tile_Pic(int x, int y, int plane)
{
    int i;
    int count;
    int sx;
    int sy;
    int units[MAX_STACK];

    sx = x;
    sy = y;
    Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy);
    Map_Units_At(x, y, plane, &count, units);
    for (i = 0; i < count; i++) {
        p_units[units[i]].plane = 1 - p_units[units[i]].plane;
    }
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Set_Draw_Page();
    Map_Redraw();
    Screen_Grab_Rect(sx, sy, sx + MAP_TILE_W, sy + MAP_TILE_H, p_anim_pic);
    for (i = 0; i < count; i++) {
        p_units[units[i]].plane = (p_units[units[i]].plane + 1) % 2;
    }
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Set_Draw_Page_Front();
}

/* (player): Plane Shift - the target stack moves to the other plane (not while any Planar Seal is up); the square
   dissolves out and in; a refused shift shows message 55 / 56 / 57 to the human
   Numbered: BUG #307 (BUGS_AND_QUIRKS.txt) */
int Plane_Shift_Cast(int player)
{
    int i;
    int tx;                                     /* first the picked unit (Spell_Target_Map_Select), then its x */
    int ty;
    int tp;
    int sx;
    int sy;
    int ok;
    int count;
    int scout;
    unsigned back;
    int check;
    int plane;
    unsigned sound;
    int saved_x;
    int saved_y;
    int saved_plane;
    int units[MAX_STACK];

    saved_x = map_view_x;
    saved_y = map_view_y;
    saved_plane = map_view_plane;
    count = 0;
    for (i = 0; i < num_players; i++) {
        if (wizards[i].globals.planar_seal > 0) {
            if (player == PLAYER_HUMAN) {
                LBX_Load_Records_Near("message", 0, message_text, 54, 1, 150);
                Message_Box(message_text);
            }
            return 0;
        }
    }
    if (player == PLAYER_HUMAN) {
        ok = Spell_Target_Map_Select(MAP_TARGET_OWN_STACK, &tx, &ty, &tp, &sx, &sy, "Plane Shift");
        ty = p_units[tx].y;
        tp = p_units[tx].plane;
        tx = p_units[tx].x;
    } else {
        ok = AI_Overland_Spell_Target(MAP_TARGET_OWN_STACK, &tx, &ty, &tp, SPELL_PLANE_SHIFT, player);
    }
    plane = (tp + 1) % 2;
    if (ok == 1) {
        Map_Units_At(tx, ty, tp, &count, units);
        check = Plane_Shift_Check(tx, ty, tp);
        if (player == PLAYER_HUMAN || (check == PLANE_SHIFT_OK && Tile_Visible(tx, ty, tp))) {
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            sound = LBX_Reload_Next("soundfx", SFX_SILENCE, p_scratch);
            Spell_Load_Sound(SPELL_PLANE_SHIFT);
            p_anim_pic = Heap_Alloc_Back(p_scratch, 30);
            back = Heap_Alloc_Back(p_scratch, 30);
            Map_View_Ensure_Visible(&map_view_x, &map_view_y, tx, ty, tp);
            map_view_plane = tp;
            sx = tx;
            sy = ty;
            Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy);
            if (p_sfx_buf != -1) {
                Sound_Play(p_sfx_buf);
            }
            Plane_Shift_Grab_Tile_Pic(tx, ty, tp);
            if (player == PLAYER_HUMAN || Tile_Visible(tx, ty, tp)) {
                for (i = 0; i < 101; i += 5) {
                    Pic_Copy(back, p_anim_pic);
                    Set_Draw_Page();
                    Map_Redraw();
                    Pic_Dissolve(back, i);
                    Draw_Bitmap(sx, sy, back);
                    Palette_Upload();
                    Screen_Present();
                }
            }
        }
        if (check == PLANE_SHIFT_OK) {
            for (i = 0; i < count; i++) {
                p_units[units[i]].plane = (p_units[units[i]].plane + 1) % 2;
            }
            if (player == PLAYER_HUMAN) {
                map_view_plane = (map_view_plane + 1) % 2;
                scout = Stack_Scout_Range(player, tx, ty, map_view_plane);
                Move_Scout_Contacts(player, tx, ty, map_view_plane, scout);
                Map_Explore_Tile(tx, ty, map_view_plane);
                Visibility_Rebuild();
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
            } else if (Tile_Visible(tx, ty, plane)) {
                map_view_plane = (map_view_plane + 1) % 2;
            }
        } else if (player == PLAYER_HUMAN) {
            for (i = 0; i < count; i++) {
                p_units[units[i]].plane = (p_units[units[i]].plane + 1) % 2;
            }
            if (player == PLAYER_HUMAN || Tile_Visible(tx, ty, plane)) {
                map_view_plane = (map_view_plane + 1) % 2;
                Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
                Map_Explore_Tile(tx, ty, map_view_plane);
                present_mode = PRESENT_DISSOLVE;
                Set_Draw_Page();
                Map_Redraw();
                Screen_Present();
                present_mode = PRESENT_NORMAL;
                Visibility_Rebuild();
                Plane_Shift_Grab_Tile_Pic(tx, ty, map_view_plane);
                for (i = 100; i >= 50; i -= 10) {
                    Pic_Copy(back, p_anim_pic);
                    Set_Draw_Page();
                    Map_Invalidate();
                    Map_Redraw();
                    Pic_Dissolve(back, i);
                    Draw_Bitmap(sx, sy, back);
                    Palette_Upload();
                    Screen_Present();
                }
            }
            if (player == PLAYER_HUMAN) {
                for (i = 50; i <= 100; i += 10) {
                    Pic_Copy(back, p_anim_pic);
                    Set_Draw_Page();
                    Map_Invalidate();
                    Map_Redraw();
                    Pic_Dissolve(back, i);
                    Draw_Bitmap(sx, sy, back);
                    Palette_Upload();
                    Screen_Present();
                }
            }
            for (i = 0; i < count; i++) {
                p_units[units[i]].plane = 1 - p_units[units[i]].plane;
            }
            if (player == PLAYER_HUMAN) {
                switch (check) {
                case PLANE_SHIFT_WATER:
                    LBX_Load_Records_Near("message", 0, message_text, 55, 1, 150);
                    Message_Box(message_text);
                    break;
                case PLANE_SHIFT_ENEMY:
                    LBX_Load_Records_Near("message", 0, message_text, 56, 1, 150);
                    Message_Box(message_text);
                    break;
                case PLANE_SHIFT_FULL:
                    LBX_Load_Records_Near("message", 0, message_text, 57, 1, 150);
                    Message_Box(message_text);
                    break;
                }
            }
        }
        if (player == PLAYER_HUMAN || (check == PLANE_SHIFT_OK && Tile_Visible(tx, ty, map_view_plane))) {
            Sound_Play(sound);
            sx = tx;
            sy = ty;
            Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy);
            map_view_plane = p_units[units[0]].plane;
            Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
            Plane_Shift_Grab_Tile_Pic(tx, ty, map_view_plane);
            if (p_sfx_buf != -1) {
                Sound_Play(p_sfx_buf);
            }
            for (i = 100; i >= 0; i -= 5) {
                Pic_Copy(back, p_anim_pic);
                Set_Draw_Page();
                Map_Invalidate();
                Map_Redraw();
                Pic_Dissolve(back, i);
                Draw_Bitmap(sx, sy, back);
                Palette_Upload();
                Screen_Present();
            }
            Sound_Play(sound);
        }
    }
    Heap_Release(p_scratch);
    map_view_x = saved_x;
    map_view_y = saved_y;
    map_view_plane = saved_plane;
    return ok;
}

/* (caster): Great Wasting - 4..6 random land squares (not corrupted, 3 or more from every city of the caster on
   that plane; up to 50 picks each) become corrupted */
void Great_Wasting_Turn(int caster)
{
    int ok;
    int city;
    int k;
    int count;
    int tries;
    int x;
    int y;
    int plane;
    char *unused_buf1;
    char *unused_buf2;

    unused_buf1 = Near_Alloc_Reset(50);         /* both allocated, never used */
    unused_buf2 = Near_Alloc(50);
    count = Random(3) + 3;
    for (k = 0; k < count; k++) {
        tries = 0;
        ok = 0;
        while (ok == 0 && tries < 50) {
            tries++;
            ok = 1;
            x = Random(MAP_WIDTH) - 1;
            y = Random(MAP_HEIGHT) - 1;
            plane = Random(2) - 1;
            for (city = 0; city < num_cities; city++) {
                if (p_cities[city].plane == plane && p_cities[city].owner == caster &&
                    Map_Distance(p_cities[city].x, p_cities[city].y, x, y, MAP_WIDTH) <= 2) {
                    ok = 0;
                }
            }
            if (Tile_Is_Water(x, y, plane)) {
                ok = 0;
            }
            if (Tile_Is_Corrupted(x, y, plane)) {
                ok = 0;
            }
            if (ok == 1) {
                p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_CORRUPTION;
            }
        }
    }
}

/* (city): Gaia's Blessing - in the 5 x 5 squares round the city a desert may become grassland and a volcano hills
   (10%), corruption is purified (20%) */
void City_Gaias_Blessing_Turn(int city)
{
    int y;
    int cx;
    int plane;
    int x;

    plane = p_cities[city].plane;
    for (cx = p_cities[city].x - 2; cx <= p_cities[city].x + 2; cx++) {
        if (cx < 0) {
            x = cx + MAP_WIDTH;
        } else if (cx > MAP_WIDTH) {            /* BUG #35: not >=, so column 60 slips through */
            x = cx - MAP_WIDTH;
        } else {
            x = cx;
        }
        for (y = p_cities[city].y - 2; y <= p_cities[city].y + 2; y++) {
            if (y < 0 || y >= MAP_HEIGHT) {
            } else {
                if (Tile_Is_Desert(x, y, plane) && Random(100) <= 10) {
                    Tile_Make_Grassland(x, y, plane);
                }
                if (Tile_Is_Volcano(x, y, plane) && Random(100) <= 10) {
                    Tile_Make_Hills(x, y, plane);
                }
                if (Tile_Is_Corrupted(x, y, plane) && Random(100) <= 20) {
                    Tile_Purify(x, y, plane);
                }
            }
        }
    }
}

/* (caster): Armageddon - 4..6 random land squares (not water, volcano, river or a node, 3 or more from every city
   of the caster on that plane; up to 50 picks each) become the caster's volcanoes */
void Armageddon_Turn(int caster)
{
    int ok;
    int city;
    int k;
    int count;
    int tries;
    int x;
    int y;
    int plane;
    unsigned terrain;

    count = Random(3) + 3;
    for (k = 0; k < count; k++) {
        tries = 0;
        ok = 0;
        while (ok == 0 && tries < 50) {
            tries++;
            ok = 1;
            x = Random(MAP_WIDTH) - 1;
            y = Random(MAP_HEIGHT) - 1;
            plane = Random(2) - 1;
            if (Tile_Is_Water(x, y, plane)) {
                ok = 0;
            }
            if (Tile_Is_Volcano(x, y, plane)) {
                ok = 0;
            }
            if (Tile_Is_River(x, y, plane)) {
                ok = 0;
            }
            terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
            if (terrain == TILE_CHAOS_NODE || terrain == TILE_SORCERY_NODE || terrain == TILE_NATURE_NODE) {
                ok = 0;
            }
            for (city = 0; city < num_cities; city++) {
                if (p_cities[city].plane == plane && p_cities[city].owner == caster &&
                    Map_Distance(p_cities[city].x, p_cities[city].y, x, y, MAP_WIDTH) <= 2) {
                    ok = 0;
                }
            }
            if (ok == 1) {
                Tile_Make_Volcano(x, y, plane, caster);
            }
        }
    }
    Wizards_Count_Volcanoes();
}

/* (player): Floating Island - on a water square (messages 58 / 59 / 60 for the human) */
void Floating_Island_Cast(int player)
{
    int picked;
    int tx;
    int ty;
    int tp;
    int sx;
    int sy;
    int valid;
    int count;
    int units[MAX_STACK];

    if (player == PLAYER_HUMAN) {
        valid = 0;
        picked = 1;
        while (valid == 0 && picked == 1) {
            picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &tx, &ty, &tp, &sx, &sy, "Floating Island");
            if (picked == 1) {
                valid = 1;
                if (!Tile_Is_Water(tx, ty, tp)) {
                    valid = 0;
                    LBX_Load_Records_Near("message", 0, message_text, 58, 1, 150);
                    Message_Box(message_text);
                }
                Map_Units_At(tx, ty, tp, &count, units);
                if (count > 0) {
                    if (p_units[units[0]].owner != player) {
                        valid = 0;
                        LBX_Load_Records_Near("message", 0, message_text, 59, 1, 150);
                        Message_Box(message_text);
                    } else if (count == MAX_STACK) {
                        valid = 0;
                        LBX_Load_Records_Near("message", 0, message_text, 60, 1, 150);
                        Message_Box(message_text);
                    }
                }
            }
        }
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &tx, &ty, &tp, SPELL_FLOATING_ISLAND, player);
    }
    if (picked == 1) {
        if ((player == PLAYER_HUMAN || Tile_Visible(tx, ty, tp)) && game_config.spell_animations == 1) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            Spell_Load_Anim(SPELL_FLOATING_ISLAND);
            Spell_Load_Sound(SPELL_FLOATING_ISLAND);
            Map_Spell_Effect_Anim(tx, ty, tp);
            Map_Full_Redraw();
            Heap_Release(p_scratch);
        }
        Unit_Create((int)p_spell_data[SPELL_FLOATING_ISLAND].effect_param, player, tx, ty, tp, UNIT_SRC_NONE);
    }
}

/* (city): Nightshade - every city enchantment cast by another wizard is dispelled on Random(250) <= 25000 n /
   (resistance + 100 n), n = the Nightshade count (slot 25 - the count - can be rolled away too when it differs from owner + 1; Next_Turn
   recounts it). BUG #489: the recount covers only cities with a qualifying building, so a city that no longer
   qualifies loses its stale count here (#482), unless the count equals owner + 1
   Numbered: BUG #489 (BUGS_AND_QUIRKS.txt) */
void City_Nightshade_Dispel(int city)
{
    int slot;
    int chance;
    int owner;
    unsigned char far *ench;
    int spell;

    ench = (unsigned char far *)p_cities[city].enchants;
    for (slot = 0; slot < NUM_CITY_ENCHANTS; slot++) {
        if (ench[slot] > 0 && ench[slot] != p_cities[city].owner + 1) {
            spell = CitySlot_To_Spell(slot);
            owner = ench[slot] - 1;
            chance = Spell_Dispel_Resistance(p_spell_data[spell].base_cost, owner, p_spell_data[spell].realm) +
                     p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] * 100;
            chance = (long)p_cities[city].enchants[CITY_ENCH_NIGHTSHADE] * 25000 / chance;
            if (Random(250) <= chance) {
                ench[slot] = 0;
            }
        }
    }
}

/* (player): Lycanthropy - a normal unit (not a hero / summoned / undead, no Chaos Channels or MUT_UNDEAD mutation) becomes werewolves;
   messages 62 / 63 / 64 for the human */
int Lycanthropy_Cast(int player)
{
    int picked;
    int unit;
    int ty;
    int tp;
    int sx;
    int sy;
    int valid;

    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    if (player == PLAYER_HUMAN) {
        picked = 1;
        valid = 0;
        while (picked == 1 && valid == 0) {
            picked = Spell_Target_Map_Select(MAP_TARGET_OWN_UNIT, &unit, &ty, &tp, &sx, &sy, "Lycanthropy");
            if (picked == 1) {
                if (p_units[unit].type < UNIT_TRIREME) {
                    Map_Full_Redraw();
                    LBX_Load_Records_Near("message", 0, message_text, 62, 1, 150);
                    Message_Box(message_text);
                } else if (unit_types[p_units[unit].type].ability_flags & AFLAG_FANTASTIC) {
                    Map_Full_Redraw();
                    LBX_Load_Records_Near("message", 0, message_text, 63, 1, 150);
                    Message_Box(message_text);
                } else if (p_units[unit].mutation_flags & (MUT_CHAOS_DEMON_SKIN | MUT_CHAOS_DEMON_WINGS | MUT_CHAOS_FIRE_BREATH | MUT_UNDEAD)) {
                    Map_Full_Redraw();
                    LBX_Load_Records_Near("message", 0, message_text, 64, 1, 150);
                    Message_Box(message_text);
                } else {
                    valid = 1;
                }
            }
        }
    } else {
        picked = AI_Pick_Unit_Enchant_Target(MAP_TARGET_OWN_UNIT, &unit, SPELL_LYCANTHROPY, player);
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || Tile_Visible(p_units[unit].x, p_units[unit].y, p_units[unit].plane)) {
            Spell_Load_Sound(SPELL_LYCANTHROPY);
            Spell_Load_Anim(SPELL_LYCANTHROPY);
            Map_Spell_Effect_Anim(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
            Map_Full_Redraw();
        }
        p_units[unit].type = p_spell_data[SPELL_LYCANTHROPY].effect_param;
        p_units[unit].experience = 0;
        p_units[unit].level = 0;
    }
    Heap_Release(p_scratch);
    return picked;
}

/* (player): Wall of Stone - city walls in a city without them (message 65 for the human) */
int Wall_Of_Stone_Cast(int player)
{
    int city;
    int ty;
    int tp;
    int sx;
    int sy;
    int ok;
    int valid;

    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    if (player == PLAYER_HUMAN) {
        valid = 0;
        ok = 1;
        while (valid == 0 && ok == 1) {
            ok = Spell_Target_Map_Select(MAP_TARGET_OWN_CITY, &city, &ty, &tp, &sx, &sy, "Wall of Stone");
            if (ok == 1) {
                if (p_cities[city].buildings[BLD_CITY_WALLS] > 0) {
                    LBX_Load_Records_Near("message", 0, message_text, 65, 1, 150);
                    Message_Box(message_text);
                } else {
                    valid = 1;
                }
            }
        }
    } else {
        ok = AI_Pick_City_Spell_Target(MAP_TARGET_OWN_CITY, &city, SPELL_WALL_OF_STONE, player);
    }
    if (ok == 1) {
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            CitySpell_Cast_Anim(city, SPELL_WALL_OF_STONE, player);
        }
        p_cities[city].buildings[BLD_CITY_WALLS] = BLDSTATE_BUILT;
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0)) {
            CitySpell_View_Loop(city, SPELL_WALL_OF_STONE, player);
        }
    }
    Heap_Release(p_scratch);
    return ok;
}

/* (player): Spell of Return - the fortress and the summoning circle move to the chosen city (the wizard's home race
   becomes the city's) */
int Spell_Of_Return_Cast(int player)
{
    int picked;
    int city;
    int ty;
    int tp;
    int sx;
    int sy;

    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    if (player == PLAYER_HUMAN) {
        do {
            picked = Spell_Target_Map_Select(MAP_TARGET_OWN_CITY, &city, &ty, &tp, &sx, &sy, "Spell of Return");
        } while (picked == 0);                       /* no cancel: Cancel only asks again */
    } else {
        picked = AI_Pick_City_Spell_Target(MAP_TARGET_OWN_CITY, &city, SPELL_MOVE_FORTRESS, player);   /* Move Fortress's pick, reused */
    }
    if (picked == 1) {
        CitySpell_Cast_Anim(city, SPELL_SPELL_OF_RETURN, player);
        wizards[player].summoning_circle_xyp[XYP_X] = p_cities[city].x;
        wizards[player].summoning_circle_xyp[XYP_Y] = p_cities[city].y;
        wizards[player].summoning_circle_xyp[XYP_PLANE] = p_cities[city].plane;
        p_fortresses[player].x = p_cities[city].x;
        p_fortresses[player].y = p_cities[city].y;
        p_fortresses[player].plane = p_cities[city].plane;
        p_fortresses[player].active = 1;
        wizards[player].home_race = p_cities[city].race;
        CitySpell_View_Loop(city, SPELL_SPELL_OF_RETURN, player);
    }
    Heap_Release(p_scratch);
    return picked;
}
