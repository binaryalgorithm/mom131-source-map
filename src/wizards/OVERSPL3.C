/* OVERSPL3.C - WIZARDS.EXE overlay code segment af2c (stub 25aa): overland spells, third part - the hero item slot
   types, Death Wish, Great Unsummoning, Black Wind, Drain Power, Earth Lore, Spell Binding, Change Terrain, Raise
   Volcano, Transmute, Corruption, Word of Recall, Enchant Road, Spell Ward, Holy Arms' end, Consecration's
   purification, Subversion and Nature Awareness.
   This module's data (DS:6768..6843): its literals only ("" is the tail of "Black Wind": -d); no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS af2c src/wizards/OVERSPL3.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Hero_Item_Slot_Types(int hero_type, int *slots);
void Nature_Awareness_Reveal(int player);
int Death_Wish_Cast(int player);
int Great_Unsummoning_Cast(int player);
int Black_Wind_Cast(int player);
int Drain_Power_Apply(int player);
int Earth_Lore_Cast(int player);
int Spell_Binding_Cast(int player);
int Change_Terrain_Cast(int player);
int Raise_Volcano_Cast(int player);
int Transmute_Cast(int player);
int Corruption_Cast(int player);
int Word_Of_Recall_Cast(int player);
int Enchant_Road_Cast(int player);
int Spell_Ward_Cast(int player);
void Subversion_Apply(int victim);
void Holy_Arms_Clear_Holy_Weapon(int player);
void City_Consecration_Turn(int city);

#include "GAME.H"

extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned p_sfx_buf;                      /* DS:9132  0xffff (-1): no sound */
extern char report_buildings_lost_count;              /* DS:9b42 */
extern struct ReportBldgLost report_buildings_lost[]; /* DS:9b44 */
extern int present_mode;                        /* DS:9c8e */
extern unsigned char (far *p_mineral_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb8 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH]; /* DS:9cb4 */
extern unsigned (far *p_terrain_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cdc */
extern int award_loc_plane;                     /* DS:c300 */
extern int award_loc_y;                         /* DS:c302 */
extern int award_loc_x;                         /* DS:c304 */
extern int award_at_location;                   /* DS:c306 */
extern int ai_replan;                           /* DS:c9f0 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
int AI_Overland_Spell_Target(int target_kind, int *x, int *y, int *plane, int spell, int caster);
int AI_Pick_Global_Enchant_Target(int *wizard, int *slot, int spell, int caster);
int AI_Pick_Unit_Enchant_Target(int target_kind, int *unit, int spell, int caster);
int AI_Target_Spell_Ward_Call(int *city, int *realm, int caster);
void Anim_EMS_Next_Frame(void);
void Anim_Load_To_EMS(char *lbx, int entry);
int CitySpell_Cast_Anim(int city, int spell, int wizard);
void CitySpell_View_Loop();                     /* no prototype here: called with 3 arguments (CASTANIM.C takes 2) */
void City_Collateral_Damage(int city, int pop_lost, int bldg_pct, int *destroyed);
void City_Garrison_List(int city, int *count, int *list);
int City_Spell_Protection_Check(int spell, int city, int caster, int show);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void GlobalEnch_Complete_Anim(int wizard, int spell);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Hero_Items_Redistribute(int player, int count, int *items);
void Hostile_Spell_Relations(int caster, int spell, int city);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int List_Select_Dialog(int count, char **names, int more, char *title);
int Map_Explore_Tile(int x, int y, int plane);
void Map_Full_Redraw(void);
void Map_Redraw(void);
void Map_Redraw_Dissolve(void);
void Map_Spell_Effect_Anim(int x, int y, int plane);
int Map_Square_To_Screen(int view_x, int view_y, int *x, int *y);
void Map_View_Ensure_Visible(int *view_x, int *view_y, int x, int y, int plane);
void Minimap_Alloc_Pic(void);
int Pic_Frame_Count(unsigned pic);
void Remap_Build_Range(int first, int last);
void Reveal_Map_All(void);
int Roll_Resist(struct BattleUnit bu, int modifier, int realm);
void Road_Enchant(int x, int y, int plane);
int Screen_Disjunction(int spell, int caster);
void Set_Pic_Frame(unsigned pic, unsigned frame);
int Sound_Play(unsigned sound);
int Spell_Load_Anim(int spell);
int Spell_Dispel_Resistance(int strength, int player, int realm);
void Spell_Load_Sound(int spell);
int Spell_Target_Map_Select(int target_type, int *target, int *y, int *plane, int *screen_x, int *screen_y,
                            char *spell_name);
void Stack_Limit_Enforce(int unit);
void Stack_Resist_Or_Die(int x, int y, int plane, int spell);
void Tile_Hook_Nop(int x, int y, int plane);
int Tile_Is_Corrupted(int x, int y, int plane);
int Tile_Is_Desert(int x, int y, int plane);
int Tile_Is_Forest_Explored(int x, int y, int plane);
int Tile_Is_Grassland(int x, int y, int plane);
int Tile_Is_Hills(int x, int y, int plane);
int Tile_Is_Mountain(int x, int y, int plane);
int Tile_Is_River(int x, int y, int plane);
int Tile_Is_Swamp(int x, int y, int plane);
int Tile_Is_Volcano(int x, int y, int plane);
int Tile_Is_Water(int x, int y, int plane);
void Tile_Make_Forest(int x, int y, int plane);
void Tile_Make_Grassland(int x, int y, int plane);
void Tile_Make_Hills(int x, int y, int plane);
void Tile_Make_Volcano(int x, int y, int plane, int owner);
void Tile_Purify(int x, int y, int plane);
int Tile_Visible(int x, int y, int plane);
void Unit_Kill_Or_Dismiss(int unit, int mode);
void Volcano_To_Mountain(int x, int y, int plane);
void Wizards_Count_Volcanoes(void);

/* (hero_type, slots[3]) the hero's three item slot types by its class (unit type +0x0f): 2 -> 3,5,6; 3 -> 4,6,6;
   1 -> 2,5,6; else 1,5,6 */
void Hero_Item_Slot_Types(int hero_type, int *slots)
{
    if (unit_types[hero_type].hero_item_class == HERO_ITEMS_WARRIOR_MAGE) {
        slots[0] = ITEM_SLOT_MELEE_OR_MAGIC;
        slots[1] = ITEM_SLOT_ARMOR;
        slots[2] = ITEM_SLOT_MISC;
    } else if (unit_types[hero_type].hero_item_class == HERO_ITEMS_MAGE) {
        slots[0] = ITEM_SLOT_MAGIC;
        slots[1] = ITEM_SLOT_MISC;
        slots[2] = ITEM_SLOT_MISC;
    } else if (unit_types[hero_type].hero_item_class == HERO_ITEMS_BOWMAN) {
        slots[0] = ITEM_SLOT_BOW;
        slots[1] = ITEM_SLOT_ARMOR;
        slots[2] = ITEM_SLOT_MISC;
    } else {
        slots[0] = ITEM_SLOT_MELEE;
        slots[1] = ITEM_SLOT_ARMOR;
        slots[2] = ITEM_SLOT_MISC;
    }
}

/* (player) Death Wish: every enemy non-fantastic unit not in a protected city resists at 0 (Death) or dies; a hero's
   items go to its owner. BUG #472: the hero's item slots are cleared to 0, not -1 */
/* int with NO return value: Overland_Cast_Spell (OVERCAST.C) stores the result as its 'ok' (which gates
   Nature's Wrath), so it gets whatever AX the last statement left - BUG #408 */
int Death_Wish_Cast(int player)
{
    char *unprotected;
    int n;
    int goes_through;
    int owner;
    int count;
    int items[3];
    int garrison[MAX_STACK];
    int i;
    int j;

    GlobalEnch_Complete_Anim(player, SPELL_DEATH_WISH);
    unprotected = Near_Alloc_Reset(num_units);
    for (j = 0; j < num_units; j++)
        unprotected[j] = 1;
    for (i = 0; i < num_cities; i++) {
        goes_through = City_Spell_Protection_Check(SPELL_DEATH_WISH, i, player, 0);
        if (goes_through == 0) {
            City_Garrison_List(i, &n, garrison);
            for (j = 0; j < n; j++)
                unprotected[garrison[j]] = 0;
        }
    }
    for (i = 0; i < num_units; i++) {
        if (unprotected[i] == 1) {
            Battle_Unit_Init(i, p_unit_stats_buf);
            if (p_unit_stats_buf->owner != player && (p_unit_stats_buf->ability_flags & AFLAG_FANTASTIC) == 0) {
                count = 0;
                owner = p_units[i].owner;
                if (Roll_Resist(*p_unit_stats_buf, 0, REALM_DEATH) > 0) {
                    if (p_units[i].hero_slot > -1) {
                        for (j = 0; j < 3; j++) {
                            if (wizards[owner].hired_heroes[p_units[i].hero_slot].item_index[j] > -1) {
                                items[count] = wizards[owner].hired_heroes[p_units[i].hero_slot].item_index[j];
                                count++;
                            }
                            wizards[owner].hired_heroes[p_units[i].hero_slot].item_index[j] = 0;
                        }
                    }
                    award_at_location = 1;
                    award_loc_x = p_units[i].x;
                    award_loc_y = p_units[i].y;
                    award_loc_plane = p_units[i].plane;
                    Unit_Kill_Or_Dismiss(i, UNIT_REMOVE_KILLED);
                    Hero_Items_Redistribute(owner, count, items);
                }
            }
        }
    }
}

/* (player) Great Unsummoning: every fantastic unit (AFLAG_FANTASTIC) without Spell Lock, not in a protected city,
   resists at -3 (REALM_SORCERY) or is unsummoned. BUG #472: hero item slots cleared to 0 */
/* int with NO return value: Overland_Cast_Spell (OVERCAST.C) stores the result as its 'ok' (which gates
   Nature's Wrath), so it gets whatever AX the last statement left - BUG #408 */
int Great_Unsummoning_Cast(int player)
{
    char *unprotected;
    int n;
    int goes_through;
    int count;
    int items[3];
    int owner;
    int garrison[MAX_STACK];
    int i;
    int j;

    GlobalEnch_Complete_Anim(player, SPELL_GREAT_UNSUMMONING);
    unprotected = Near_Alloc_Reset(num_units);
    for (j = 0; j < num_units; j++)
        unprotected[j] = 1;
    for (i = 0; i < num_cities; i++) {
        goes_through = City_Spell_Protection_Check(SPELL_GREAT_UNSUMMONING, i, player, 0);
        if (goes_through == 0) {
            City_Garrison_List(i, &n, garrison);
            for (j = 0; j < n; j++)
                unprotected[garrison[j]] = 0;
        }
    }
    for (i = 0; i < num_units; i++) {
        if (unprotected[i] == 1) {
            Battle_Unit_Init(i, p_unit_stats_buf);
            if ((p_unit_stats_buf->ability_flags & AFLAG_FANTASTIC) && (p_units[i].enchantments & UNIT_ENCH_SPELL_LOCK) == 0) {
                count = 0;
                owner = p_units[i].owner;
                if (Roll_Resist(*p_unit_stats_buf, -3, REALM_SORCERY) > 0) {
                    if (p_units[i].hero_slot > -1) {
                        for (j = 0; j < 3; j++) {
                            if (wizards[owner].hired_heroes[p_units[i].hero_slot].item_index[j] > -1) {
                                items[count] = wizards[owner].hired_heroes[p_units[i].hero_slot].item_index[j];
                                count++;
                            }
                            wizards[owner].hired_heroes[p_units[i].hero_slot].item_index[j] = 0;   /* BUG #472: 0, not -1 (none) - as Death Wish */
                        }
                        if (count > 0) {
                            award_at_location = 1;
                            award_loc_x = p_units[i].x;
                            award_loc_y = p_units[i].y;
                            award_loc_plane = p_units[i].plane;
                            Hero_Items_Redistribute(owner, count, items);
                        }
                    }
                    Unit_Kill_Or_Dismiss(i, UNIT_REMOVE_KILLED);
                }
            }
        }
    }
}

/* (player) -> 1 cast: Black Wind on a stack (the human picks a unit, the AI a square); the stack resists or dies,
   relations -3. BUG #34: the visibility and relation tests index p_units by the x coordinate
   Numbered: BUG #34 (BUGS_AND_QUIRKS.txt) */
int Black_Wind_Cast(int player)
{
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int picked;
    int i;

    if (player == PLAYER_HUMAN) {
        picked = Spell_Target_Map_Select(MAP_TARGET_ENEMY_STACK, &x, &y, &plane, &sx, &sy, "Black Wind");
        y = p_units[x].y;
        plane = p_units[x].plane;
        x = p_units[x].x;
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_ENEMY_STACK, &x, &y, &plane, SPELL_BLACK_WIND, player);
    }
    if (picked == 1) {
        for (i = 0; i < num_cities; i++) {
            if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane)
                picked = City_Spell_Protection_Check(SPELL_BLACK_WIND, i, player, p_cities[i].owner == PLAYER_HUMAN || player == PLAYER_HUMAN);
        }
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || p_units[x].owner == PLAYER_HUMAN || Tile_Visible(x, y, plane)) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            Spell_Load_Sound(SPELL_BLACK_WIND);
            Spell_Load_Anim(SPELL_BLACK_WIND);
            Map_Spell_Effect_Anim(x, y, plane);
            Map_Full_Redraw();
            Heap_Release(p_scratch);
        }
        Stack_Resist_Or_Die(x, y, plane, SPELL_BLACK_WIND);
        Diplo_Relation_Change(-3, player, p_units[x].owner, DIPLO_MSG_UNPROVOKED_DESTRUCTION, 0, x);
    }
    return picked;
}

/* (player) -> the mana drained: the sum of ten Random(20) (10..200), at most the player's mana reserve
   Numbered: BUG #11 (BUGS_AND_QUIRKS.txt) */
int Drain_Power_Apply(int player)
{
    int i;
    int amount;

    amount = 0;
    for (i = 0; i < 10; i++)
        amount += Random(20);
    if (wizards[player].mana_reserve < amount)
        amount = wizards[player].mana_reserve;
    wizards[player].mana_reserve -= amount;
    return amount;
}

/* (player) -> 1 cast: Earth Lore - the human's animation and the explored map around the view */
int Earth_Lore_Cast(int player)
{
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int picked;
    int wx;
    int frame;
    unsigned sound;
    int mx;
    int my;

    picked = 0;
    if (player == PLAYER_HUMAN) {
        Minimap_Alloc_Pic();
        Heap_Mark(p_scratch);
        picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &x, &y, &plane, &sx, &sy, "Earth Lore");
    }
    sound = LBX_Reload_Next("soundfx", SFX_SILENCE, p_scratch);
    if (picked == 1 && player == PLAYER_HUMAN) {
        Remap_Build_Range(1, 4);                /* remap tables 1-4 for the animation */
        Anim_Load_To_EMS("specfx", 45);         /* Earth Lore's 22 frames over the map window */
        Spell_Load_Sound(SPELL_EARTH_LORE);
        if (p_sfx_buf != -1)
            Sound_Play(p_sfx_buf);
        for (frame = 0; frame < 22; frame++) {
            Timer_Mark();
            Set_Draw_Page();
            Fill_Rect(0, MAP_VIEW_TOP, MAP_VIEW_COLS * MAP_TILE_W, SCREEN_MAX_Y, 0);   /* the map window */
            Map_Invalidate();
            Map_Redraw();
            Anim_EMS_Next_Frame();
            Screen_Present();
            Wait_Ticks(2);
        }
        for (mx = map_view_x; mx <= map_view_x + (MAP_VIEW_COLS - 1); mx++) {
            if (mx >= MAP_WIDTH)
                wx = mx - MAP_WIDTH;
            else
                wx = mx;
            for (my = map_view_y; my <= map_view_y + (MAP_VIEW_ROWS - 1); my++) {
                if (my >= MAP_HEIGHT)
                    continue;
                Map_Explore_Tile(wx, my, plane);
                Tile_Hook_Nop(wx, my, plane);
            }
        }
    }
    Sound_Play(sound);
    Map_Redraw_Dissolve();
    Heap_Release(p_scratch);
    return picked;
}

/* (player) Spell Binding: the human uses the Disjunction screen; the AI's pick takes a global enchantment over if
   Random(250) <= (long)strength * 250 / (its resistance + strength) */
/* int with NO return value: Overland_Cast_Spell (OVERCAST.C) stores the result as its 'ok' (which gates
   Nature's Wrath), so it gets whatever AX the last statement left - BUG #408 */
int Spell_Binding_Cast(int player)
{
    int slot;
    int target;
    char far *globals;
    int picked;
    int n;

    if (player == PLAYER_HUMAN) {
        Screen_Disjunction(SPELL_SPELL_BINDING, player);
    } else {
        /* BUG #473: the AI pick returns a GlobalEnchants slot (OVERCAST.C maps it with GlobalSlot_To_Spell); here it
           indexes p_spell_data as if it were a SpellId */
        picked = AI_Pick_Global_Enchant_Target(&target, &slot, SPELL_SPELL_BINDING, player);
        if (picked == 1) {
            n = Spell_Dispel_Resistance(p_spell_data[slot].base_cost, target, p_spell_data[slot].realm) +
                wizards[player].casting_cost_initial;
            n = (long)wizards[player].casting_cost_initial * 250 / n;
            if (Random(250) <= n) {
                globals = (char far *)&wizards[target].globals;
                globals[(int)p_spell_data[slot].effect_param] = 0;
                globals = (char far *)&wizards[player].globals;
                globals[(int)p_spell_data[slot].effect_param] = player + 1;
            }
        }
    }
}

/* (player) -> 1 cast: Change Terrain - grassland to forest; swamp, forest, desert, hills to grassland; a volcano to
   a mountain, a mountain to hills (the node tiles refused with message 34, any other terrain with 35). The movement
   costs stay the old terrain's (TERRCHG.C header); clarified using ReMoM repo */
int Change_Terrain_Cast(int player)
{
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int done;
    int terrain;
    int nframes;
    int saved_x;
    int saved_y;
    int saved_plane;
    int frame;
    int picked;

    saved_x = map_view_x;
    saved_y = map_view_y;
    saved_plane = map_view_plane;
    picked = 1;
    if (player == PLAYER_HUMAN) {
        done = 0;
        while (done == 0 && picked == 1) {
            picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &x, &y, &plane, &sx, &sy, "Change Terrain");
            if (picked == 1) {
                if (Tile_Is_Grassland(x, y, plane) || Tile_Is_Forest_Explored(x, y, plane) || Tile_Is_Desert(x, y, plane) ||
                    Tile_Is_Volcano(x, y, plane) || Tile_Is_Hills(x, y, plane) || Tile_Is_Mountain(x, y, plane) ||
                    Tile_Is_Swamp(x, y, plane)) {
                    terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
                    if (terrain == TILE_CHAOS_NODE || terrain == TILE_SORCERY_NODE || terrain == TILE_NATURE_NODE) {
                        LBX_Load_Records_Near("message", 0, message_text, 34, 1, 150);
                        Message_Box(message_text);
                    } else {
                        done = 1;
                    }
                } else {
                    LBX_Load_Records_Near("message", 0, message_text, 35, 1, 150);
                    Message_Box(message_text);
                }
            }
        }
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &x, &y, &plane, SPELL_CHANGE_TERRAIN, player);
    }
    if (picked == 1) {
        if ((player == PLAYER_HUMAN || p_exploration_maps[plane][y][x] != 0) && game_config.spell_animations == 1) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Heap_Mark(p_scratch);
            Spell_Load_Sound(SPELL_CHANGE_TERRAIN);
            Spell_Load_Anim(SPELL_CHANGE_TERRAIN);
            for (frame = 0; frame < 2; frame++) {
                Set_Draw_Page();
                Map_Redraw();
                Screen_Present();
            }
            if (p_sfx_buf != -1)
                Sound_Play(p_sfx_buf);
            nframes = Pic_Frame_Count(p_anim_pic);
            if (nframes < 12)
                nframes = 12;
            Map_View_Ensure_Visible(&map_view_x, &map_view_y, x, y, plane);
            map_view_plane = plane;
            Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
            sx = x;
            sy = y;
            Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy);
            for (frame = 0; frame < nframes; frame++) {
                Timer_Mark();
                if (frame == 8) {               /* the terrain changes on frame 8 of the animation */
                    if (Tile_Is_Grassland(x, y, plane))
                        Tile_Make_Forest(x, y, plane);
                    else if (Tile_Is_Swamp(x, y, plane) || Tile_Is_Forest_Explored(x, y, plane) ||
                             Tile_Is_Desert(x, y, plane) || Tile_Is_Hills(x, y, plane))
                        Tile_Make_Grassland(x, y, plane);
                    if (Tile_Is_Volcano(x, y, plane)) {
                        Volcano_To_Mountain(x, y, plane);
                        Wizards_Count_Volcanoes();
                    } else if (Tile_Is_Mountain(x, y, plane)) {
                        Tile_Make_Hills(x, y, plane);
                    }
                }
                Set_Draw_Page();
                Map_Invalidate();
                Map_Redraw();
                Set_Pic_Frame(p_anim_pic, frame);
                Draw_Pic_Clipped(sx - MAP_TILE_W, sy - MAP_TILE_H, p_anim_pic);
                Screen_Present();
                Wait_Ticks(2);
            }
            Map_Full_Redraw();
            Heap_Release(p_scratch);
        } else {
            if (Tile_Is_Grassland(x, y, plane))
                Tile_Make_Forest(x, y, plane);
            else if (Tile_Is_Swamp(x, y, plane) || Tile_Is_Forest_Explored(x, y, plane) || Tile_Is_Desert(x, y, plane) ||
                     Tile_Is_Hills(x, y, plane))
                Tile_Make_Grassland(x, y, plane);
            if (Tile_Is_Volcano(x, y, plane)) {
                Volcano_To_Mountain(x, y, plane);
                Wizards_Count_Volcanoes();
            } else if (Tile_Is_Mountain(x, y, plane)) {
                Tile_Make_Hills(x, y, plane);
            }
        }
    }
    if (map_view_x != saved_x || map_view_y != saved_y || map_view_plane != saved_plane) {
        map_view_x = saved_x;
        map_view_y = saved_y;
        map_view_plane = saved_plane;
        Map_View_Ensure_Visible(&map_view_x, &map_view_y, map_view_x, map_view_y, map_view_plane);
        Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    }
    return picked;
}

/* (player) -> 1 cast: Raise Volcano on a land square (not water, mountain, hills, river or the node tiles);
   a city there takes collateral damage (the human's losses reported), nearby cities' owners grow hostile.
   BUGS #37: the reported building loss records x, not the city; near_cities holds only 2; the city loop stops at the
   first nearby human city, so a target city later in the list is neither checked nor damaged; destroyed[] is
   not cleared first (City_Collateral_Damage writes only what fell), so stale values can be reported
   Numbered: BUG #37 (BUGS_AND_QUIRKS.txt) */
int Raise_Volcano_Cast(int player)
{
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int picked;
    int ok;
    int k;
    int human_near;
    int near_cities[2];
    int n;
    int destroyed[NUM_BUILDINGS];
    int i;

    picked = 1;
    human_near = 0;
    if (player == PLAYER_HUMAN) {
        ok = 0;
        while (ok == 0 && picked == 1) {
            ok = 1;
            picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &x, &y, &plane, &sx, &sy, "Raise Volcano");
            if (picked == 1) {
                if (Tile_Is_Water(x, y, plane)) {
                    ok = 0;
                    LBX_Load_Records_Near("message", 0, message_text, 36, 1, 150);
                    Message_Box(message_text);
                }
                if (Tile_Is_Mountain(x, y, plane)) {
                    ok = 0;
                    LBX_Load_Records_Near("message", 0, message_text, 37, 1, 150);
                    Message_Box(message_text);
                }
                if (Tile_Is_Hills(x, y, plane)) {
                    ok = 0;
                    LBX_Load_Records_Near("message", 0, message_text, 38, 1, 150);
                    Message_Box(message_text);
                }
                if (Tile_Is_River(x, y, plane)) {
                    ok = 0;
                    LBX_Load_Records_Near("message", 0, message_text, 86, 1, 150);
                    Message_Box(message_text);
                }
                i = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;   /* i: here the terrain */
                if (i == TILE_CHAOS_NODE || i == TILE_SORCERY_NODE || i == TILE_NATURE_NODE) {
                    ok = 0;
                    LBX_Load_Records_Near("message", 0, message_text, 39, 1, 150);
                    Message_Box(message_text);
                }
            }
        }
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &x, &y, &plane, SPELL_RAISE_VOLCANO, player);
    }
    if (picked == 1) {
        n = 0;
        for (i = 0; i < num_cities && human_near == 0; i++) {
            if ((abs(p_cities[i].x - x) <= 2 || abs(p_cities[i].x - x) >= MAP_WIDTH - 2) && abs(p_cities[i].y - y) <= 2 &&
                p_cities[i].plane == plane) {
                near_cities[n] = i;
                if (p_cities[i].owner == PLAYER_HUMAN)
                    human_near = 1;
                n++;
            }
            if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane) {
                picked = City_Spell_Protection_Check(SPELL_RAISE_VOLCANO, i, player, p_cities[i].owner == PLAYER_HUMAN || player == PLAYER_HUMAN);
                if (picked == 1) {
                    City_Collateral_Damage(i, 0, 15, destroyed);
                    if (p_cities[i].owner == PLAYER_HUMAN) {
                        for (k = 0; k < NUM_BUILDINGS; k++) {
                            if (destroyed[k] > 0 && report_buildings_lost_count < MAX_REPORT_ENTRIES) {
                                report_buildings_lost[report_buildings_lost_count].city = x;
                                report_buildings_lost[report_buildings_lost_count].building = destroyed[k];
                                report_buildings_lost_count++;
                            }
                        }
                    }
                }
            }
        }
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || human_near == 1 || p_exploration_maps[plane][y][x] != 0) {
            ai_replan = 1;
            Minimap_Alloc_Pic();
            Spell_Load_Sound(SPELL_RAISE_VOLCANO);
            Spell_Load_Anim(SPELL_RAISE_VOLCANO);
            Map_Spell_Effect_Anim(x, y, plane);
        }
        Tile_Make_Volcano(x, y, plane, player);
        Map_Full_Redraw();
        wizards[player].volcanoes++;
        Wizards_Count_Volcanoes();
        if (n > 0) {
            Hostile_Spell_Relations(player, SPELL_RAISE_VOLCANO, near_cities[0]);
            if (n > 1)
                Hostile_Spell_Relations(player, SPELL_RAISE_VOLCANO, near_cities[1]);
        }
    }
    Map_Full_Redraw();
    Heap_Release(p_scratch);
    return picked;
}

/* (player) -> 1 cast: Transmute a mineral (iron <-> gold, coal <-> gems, silver <-> mithril; adamantium,
   quork and crysx are not changed) */
int Transmute_Cast(int player)
{
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int done;
    int changed;
    int picked;

    changed = 0;
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    picked = 1;
    if (player == PLAYER_HUMAN) {
        done = 0;
        while (done == 0 && picked == 1) {
            picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &x, &y, &plane, &sx, &sy, "Transmute");
            if (picked == 1) {
                if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_COAL || (p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_IRON_ORE ||
                    (p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_SILVER || (p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_GEMS ||
                    (p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_GOLD || (p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_MITHRIL) {
                    done = 1;
                } else {
                    LBX_Load_Records_Near("message", 0, message_text, 40, 1, 150);
                    Message_Box(message_text);
                }
            }
        }
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &x, &y, &plane, SPELL_TRANSMUTE, player);
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || (game_config.enemy_spells == 1 && wizards[PLAYER_HUMAN].globals.detect_magic > 0 &&
                            p_exploration_maps[plane][y][x] != 0)) {
            Spell_Load_Sound(SPELL_TRANSMUTE);
            Spell_Load_Anim(SPELL_TRANSMUTE);
            Map_Spell_Effect_Anim(x, y, plane);
        }
        if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_MITHRIL) {
            p_mineral_maps[plane][y][x] = 0;
            p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] | MINERAL_SILVER;
            changed = 1;
        }
        if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_GOLD) {
            p_mineral_maps[plane][y][x] = 0;
            p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] | MINERAL_IRON_ORE;
            changed = 1;
        }
        if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_GEMS) {
            p_mineral_maps[plane][y][x] = 0;
            p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] | MINERAL_COAL;
            changed = 1;
        }
        if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_COAL && changed == 0) {
            p_mineral_maps[plane][y][x] = 0;
            p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] | MINERAL_GEMS;
            changed = 1;
        }
        if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_IRON_ORE && changed == 0) {
            p_mineral_maps[plane][y][x] = 0;
            p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] | MINERAL_GOLD;
            changed = 1;
        }
        if ((p_mineral_maps[plane][y][x] & MINERAL_KIND_MASK) == MINERAL_SILVER && changed == 0) {
            p_mineral_maps[plane][y][x] = 0;
            p_mineral_maps[plane][y][x] = p_mineral_maps[plane][y][x] | MINERAL_MITHRIL;
            changed = 1;
        }
    }
    Map_Redraw_Dissolve();
    Heap_Release(p_scratch);
    return picked;
}

/* (player) -> 1 cast: Corruption on a land square (not a node tile, not already corrupted, not water); nearby
   cities' owners grow hostile. The human's silence sample (SFX_SILENCE) is loaded and never played */
int Corruption_Cast(int player)
{
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int picked;
    int done;
    int terrain;
    int nframes;
    int human_near;
    unsigned sound;
    int saved_plane;
    int saved_x;
    int saved_y;
    int near_cities[2];
    int n;
    int i;

    human_near = 0;
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    saved_plane = map_view_plane;
    saved_x = map_view_x;
    saved_y = map_view_y;
    if (player == PLAYER_HUMAN) {
        sound = LBX_Reload_Next("soundfx", SFX_SILENCE, p_scratch);
        done = 0;
        picked = 1;
        while (done == 0 && picked == 1) {
            picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &x, &y, &plane, &sx, &sy, "Corruption");
            if (picked == 1) {
                terrain = p_terrain_maps[plane][y][x] % TERRAIN_IDS_PER_PLANE;
                if (terrain == TILE_CHAOS_NODE || terrain == TILE_SORCERY_NODE || terrain == TILE_NATURE_NODE) {
                    LBX_Load_Records_Near("message", 0, message_text, 41, 1, 150);
                    Message_Box(message_text);
                } else if (p_terrain_flags_maps[plane][y][x] & TERRAIN_FLAG_CORRUPTION) {
                    LBX_Load_Records_Near("message", 0, message_text, 42, 1, 150);
                    Message_Box(message_text);
                } else if (Tile_Is_Water(x, y, plane)) {
                    LBX_Load_Records_Near("message", 0, message_text, 43, 1, 150);
                    Message_Box(message_text);
                } else {
                    done = 1;
                }
            }
        }
    } else {
        picked = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &x, &y, &plane, SPELL_CORRUPTION, player);
    }
    n = 0;
    for (i = 0; i < num_cities; i++) {
        if ((abs(p_cities[i].x - x) <= 2 || abs(p_cities[i].x - x) >= MAP_WIDTH - 2) && abs(p_cities[i].y - y) <= 2 &&
            p_cities[i].plane == plane) {
            near_cities[n] = i;
            if (p_cities[i].owner == PLAYER_HUMAN)
                human_near = 1;
            n++;
        }
        if (p_cities[i].x == x && p_cities[i].y == y && p_cities[i].plane == plane)
            picked = City_Spell_Protection_Check(SPELL_CORRUPTION, i, player, p_cities[i].owner == PLAYER_HUMAN || player == PLAYER_HUMAN);
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || human_near == 1 ||
            (game_config.enemy_spells == 1 && wizards[PLAYER_HUMAN].globals.detect_magic > 0 && p_exploration_maps[plane][y][x] != 0)) {
            Spell_Load_Sound(SPELL_CORRUPTION);
            Spell_Load_Anim(SPELL_CORRUPTION);
            for (i = 0; i < 2; i++) {
                Set_Draw_Page();
                Map_Redraw();
                Screen_Present();
            }
            if (p_sfx_buf != -1)
                Sound_Play(p_sfx_buf);
            nframes = Pic_Frame_Count(p_anim_pic);
            if (nframes < 12)
                nframes = 12;
            Map_View_Ensure_Visible(&map_view_x, &map_view_y, x, y, plane);
            map_view_plane = plane;
            sx = x;
            sy = y;
            Map_Square_To_Screen(map_view_x, map_view_y, &sx, &sy);
            for (i = 0; i < nframes; i++) {
                Timer_Mark();
                if (i == 11)                    /* the square corrupts on frame 11 */
                    p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_CORRUPTION;
                Set_Draw_Page();
                Map_Invalidate();
                Map_Redraw();
                Set_Pic_Frame(p_anim_pic, i);
                Draw_Pic_Clipped(sx - MAP_TILE_W, sy - MAP_TILE_H, p_anim_pic);
                Screen_Present();
                Wait_Ticks(2);
            }
        } else {
            p_terrain_flags_maps[plane][y][x] = p_terrain_flags_maps[plane][y][x] | TERRAIN_FLAG_CORRUPTION;
        }
        if (n > 0) {
            Hostile_Spell_Relations(player, SPELL_CORRUPTION, near_cities[0]);
            if (n > 1)
                Hostile_Spell_Relations(player, SPELL_CORRUPTION, near_cities[1]);
        }
    }
    Map_Full_Redraw();
    map_view_x = saved_x;
    map_view_y = saved_y;
    map_view_plane = saved_plane;
    Heap_Release(p_scratch);
    return picked;
}

/* (player) -> 1 cast: Word of Recall - the unit to the caster's summoning circle (wizards[player])
   Numbered: BUG #115 (BUGS_AND_QUIRKS.txt) */
int Word_Of_Recall_Cast(int player)
{
    int unit;
    int unused_y;
    int unused_plane;
    int unused_sx;
    int unused_sy;
    int picked;

    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    if (player == PLAYER_HUMAN)
        picked = Spell_Target_Map_Select(MAP_TARGET_OWN_UNIT, &unit, &unused_y, &unused_plane, &unused_sx, &unused_sy, "Word of Recall");
    else
        picked = AI_Pick_Unit_Enchant_Target(MAP_TARGET_OWN_UNIT, &unit, SPELL_WORD_OF_RECALL, player);
    if (picked == 1) {
        if (player == PLAYER_HUMAN || Tile_Visible(p_units[unit].x, p_units[unit].y, p_units[unit].plane)) {
            Spell_Load_Sound(SPELL_WORD_OF_RECALL);
            Spell_Load_Anim(SPELL_WORD_OF_RECALL);
            Map_Spell_Effect_Anim(p_units[unit].x, p_units[unit].y, p_units[unit].plane);
        }
        p_units[unit].x = wizards[player].summoning_circle_xyp[XYP_X];
        p_units[unit].y = wizards[player].summoning_circle_xyp[XYP_Y];
        p_units[unit].plane = wizards[player].summoning_circle_xyp[XYP_PLANE];
        Stack_Limit_Enforce(unit);
    }
    Map_Full_Redraw();
    Heap_Release(p_scratch);
    return picked;
}

/* (player) -> 1 cast: Enchant Road on the 5x5 squares around the target
   Numbered: BUG #35 (BUGS_AND_QUIRKS.txt) */
int Enchant_Road_Cast(int player)
{
    int wx;
    int frame;
    int x;
    int y;
    int plane;
    int sx;
    int sy;
    int picked;
    unsigned sound;
    int saved_plane;
    int saved_x;
    int saved_y;
    int ix;
    int iy;

    saved_plane = map_view_plane;
    saved_x = map_view_x;
    saved_y = map_view_y;
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    sound = LBX_Reload_Next("soundfx", SFX_SILENCE, p_scratch);
    if (player == PLAYER_HUMAN)
        picked = Spell_Target_Map_Select(MAP_TARGET_SQUARE, &x, &y, &plane, &sx, &sy, "Enchant Road");
    else
        picked = AI_Overland_Spell_Target(MAP_TARGET_SQUARE, &x, &y, &plane, SPELL_ENCHANT_ROAD, player);
    if (picked == 1) {
        if (player == PLAYER_HUMAN || (game_config.enemy_spells == 1 && p_exploration_maps[plane][y][x] != 0)) {
            Map_Center_View(&map_view_x, &map_view_y, x, y, plane);
            map_view_plane = plane;
            Timer_Mark();
            Wait_Ticks(3);
            Spell_Load_Sound(SPELL_ENCHANT_ROAD);
            Remap_Build_Range(1, 4);            /* remap tables 1-4 for the animation */
            Anim_Load_To_EMS("specfx", 46);     /* Enchant Road's 22 frames over the map window */
            for (frame = 0; frame < 22; frame++) {
                Timer_Mark();
                Set_Draw_Page();
                Fill_Rect(0, MAP_VIEW_TOP, MAP_VIEW_COLS * MAP_TILE_W, SCREEN_MAX_Y, 0);   /* the map window */
                Map_Invalidate();
                Map_Redraw();
                Anim_EMS_Next_Frame();
                Screen_Present();
                Wait_Ticks(2);
            }
            Sound_Play(sound);
        }
        for (ix = x - 2; ix <= x + 2; ix++) {
            if (ix < 0)
                wx = ix + MAP_WIDTH;
            else if (ix > MAP_WIDTH)            /* BUG #35: not >=, so column 60 slips through */
                wx = ix - MAP_WIDTH;
            else
                wx = ix;
            for (iy = y - 2; iy <= y + 2; iy++) {
                if (iy < 0 || iy > MAP_HEIGHT)  /* BUG #35: not >=, so row 40 slips through */
                    continue;
                Road_Enchant(wx, iy, plane);
            }
        }
    }
    Map_Redraw_Dissolve();
    Heap_Release(p_scratch);
    map_view_plane = saved_plane;
    map_view_x = saved_x;
    map_view_y = saved_y;
    return picked;
}

/* (player) Spell Ward: the human picks a city and one of the five realm wards it lacks; the AI's pick; the ward is
   set to the caster + 1 */
/* int with NO return value: Overland_Cast_Spell (OVERCAST.C) stores the result as its 'ok' (which gates
   Nature's Wrath), so it gets whatever AX the last statement left - BUG #408 */
int Spell_Ward_Cast(int player)
{
    int k;
    int city;
    int unused_y;
    int unused_plane;
    int unused_sx;
    int unused_sy;
    int picked;
    int done;
    int ward;
    char *ward_names[5];
    char *names[5];
    char realms[5];
    char far *enchants;
    int n;

    ward_names[0] = "Nature Ward";
    ward_names[1] = "Sorcery Ward";
    ward_names[2] = "Chaos Ward";
    ward_names[3] = "Life Ward";
    ward_names[4] = "Death Ward";
    Minimap_Alloc_Pic();
    Heap_Mark(p_scratch);
    if (player == PLAYER_HUMAN) {
        done = 0;
        picked = 1;
        while (done == 0 && picked == 1) {
            picked = Spell_Target_Map_Select(MAP_TARGET_OWN_CITY, &city, &unused_y, &unused_plane, &unused_sx, &unused_sy, "Spell Ward");
            if (picked == 1) {
                if (p_cities[city].enchants[CITY_ENCH_NATURE_WARD] > 0 && p_cities[city].enchants[CITY_ENCH_SORCERY_WARD] > 0 && p_cities[city].enchants[CITY_ENCH_CHAOS_WARD] > 0 &&
                    p_cities[city].enchants[CITY_ENCH_LIFE_WARD] > 0 && p_cities[city].enchants[CITY_ENCH_DEATH_WARD] > 0) {
                    LBX_Load_Records_Near("message", 0, message_text, 44, 1, 150);
                    Message_Box(message_text);
                } else {
                    for (k = 0; k < 5; k++) {
                        names[k] = "";
                        realms[k] = -1;
                    }
                    n = 0;
                    if (p_cities[city].enchants[CITY_ENCH_NATURE_WARD] == 0) {
                        names[n] = ward_names[0];
                        realms[n] = REALM_NATURE;
                        n++;
                    }
                    if (p_cities[city].enchants[CITY_ENCH_SORCERY_WARD] == 0) {
                        names[n] = ward_names[1];
                        realms[n] = REALM_SORCERY;
                        n++;
                    }
                    if (p_cities[city].enchants[CITY_ENCH_CHAOS_WARD] == 0) {
                        names[n] = ward_names[2];
                        realms[n] = REALM_CHAOS;
                        n++;
                    }
                    if (p_cities[city].enchants[CITY_ENCH_LIFE_WARD] == 0) {
                        names[n] = ward_names[3];
                        realms[n] = REALM_LIFE;
                        n++;
                    }
                    if (p_cities[city].enchants[CITY_ENCH_DEATH_WARD] == 0) {
                        names[n] = ward_names[4];
                        realms[n] = REALM_DEATH;
                        n++;
                    }
                    ward = List_Select_Dialog(n, names, 0, "Select a Spell Ward To Cast");
                    if (ward != -1) {
                        done = 1;
                        ward = realms[ward] + CITY_ENCH_NATURE_WARD;
                    }
                }
            }
        }
    } else {
        picked = AI_Target_Spell_Ward_Call(&city, &ward, player);
        ward += CITY_ENCH_NATURE_WARD;
    }
    if (picked == 1) {
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0))
            CitySpell_Cast_Anim(city, SPELL_SPELL_WARD, player);
        enchants = (char far *)p_cities[city].enchants;
        enchants[ward] = player + 1;
        if (player == PLAYER_HUMAN || p_cities[city].owner == PLAYER_HUMAN ||
            (game_config.enemy_spells == 1 &&
             p_exploration_maps[p_cities[city].plane][p_cities[city].y][p_cities[city].x] != 0 &&
             wizards[PLAYER_HUMAN].globals.detect_magic > 0))
            CitySpell_View_Loop(city, SPELL_SPELL_WARD, player);
    }
}

/* (player) Holy Arms ended: the Holy Weapon bit (0x800000) cleared from the player's units */
void Holy_Arms_Clear_Holy_Weapon(int player)
{
    int i;

    for (i = 0; i < num_units; i++) {
        if (p_units[i].owner == player && (p_units[i].enchantments & UNIT_ENCH_HOLY_WEAPON) != 0)
            p_units[i].enchantments = p_units[i].enchantments ^ UNIT_ENCH_HOLY_WEAPON;
    }
}

/* (city) Consecration: the city's curses cleared (Chaos Rift, Evil Presence, Cursed Lands,
   Famine, Pestilence) and the corruption around it
   purified
   Numbered: BUG #35 (BUGS_AND_QUIRKS.txt) */
void City_Consecration_Turn(int city)
{
    int iy;
    int plane;
    int wx;
    int ix;

    p_cities[city].enchants[CITY_ENCH_CHAOS_RIFT] = 0;
    p_cities[city].enchants[CITY_ENCH_EVIL_PRESENCE] = 0;
    p_cities[city].enchants[CITY_ENCH_CURSED_LANDS] = 0;
    p_cities[city].enchants[CITY_ENCH_FAMINE] = 0;
    p_cities[city].enchants[CITY_ENCH_PESTILENCE] = 0;
    plane = p_cities[city].plane;
    for (ix = p_cities[city].x - 2; ix <= p_cities[city].x + 2; ix++) {
        if (ix < 0)
            wx = ix + MAP_WIDTH;
        else if (ix > MAP_WIDTH)                /* BUG #35: not >=, so column 60 slips through */
            wx = ix - MAP_WIDTH;
        else
            wx = ix;
        for (iy = p_cities[city].y - 2; iy <= p_cities[city].y + 2; iy++) {
            if (iy < 0 || iy > MAP_HEIGHT)      /* BUG #35: not >=, so row 40 slips through */
                continue;
            if (Tile_Is_Corrupted(wx, iy, plane))
                Tile_Purify(wx, iy, plane);
        }
    }
    present_mode = PRESENT_DISSOLVE;
}

/* (victim) Subversion: every other wizard (not the human) in contact loses 25 relation with the victim (to -100 at
   worst), mirrored, and both peace counters reset
   Numbered: BUG #10 (BUGS_AND_QUIRKS.txt) */
void Subversion_Apply(int victim)
{
    int i;

    for (i = 0; i < num_players; i++) {
        if (i == PLAYER_HUMAN || i == victim)
            continue;
        if (wizards[i].contacted[victim] == 1) {
            wizards[i].relationship[victim] = wizards[i].relationship[victim] - 25;
            if (wizards[i].relationship[victim] < -100)
                wizards[i].relationship[victim] = -100;
            wizards[victim].relationship[i] = wizards[i].relationship[victim];
            wizards[victim].peace_turns[i] = 0;
            wizards[i].peace_turns[victim] = 0;
        }
    }
}

/* (player) Nature Awareness for the human: the whole map revealed, contacts updated */
void Nature_Awareness_Reveal(int player)
{
    if (player == PLAYER_HUMAN) {
        Minimap_Alloc_Pic();
        Reveal_Map_All();
        Wizards_Update_Contacts();
        Heap_Release(p_scratch);
    }
}
