/* COMBSPRT.C - WIZARDS.EXE overlay code segment dc99 (stub 2641): the combat battlefield renderer - the isometric
   terrain with its animated water, rivers, node, volcano, roads and city base, the target and active-unit markers,
   the depth-sorted sprite list (terrain objects, houses, city walls, Wall of Darkness / Fire, vortices, unit
   figures, curse icons, missiles) and drawing it, and rendering the unit figures from the EMS figure cache.
   This module's data (DS:706e..70a0): the combat animation counters and the literal "E1"; BSS DS:cfa0..d383 (the
   battlefield pictures, the combat buffers and the sprite list - defined here, highest address first).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS dc99 src/wizards/COMBSPRT.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Battlefield_Setup(int x, int y, int plane);
void Combat_Draw_Battlefield(void);
void Combat_Draw_Sprite_List(void);
void Hit_Anim_Reset_Jitter(void);
void Combat_Queue_Terrain_Objects_A(void);
void Combat_Queue_Terrain_Objects_B(void);
void Combat_Queue_Features_And_Walls(void);
void Combat_Queue_City_Walls(int front);
void Combat_Queue_Wall_Of_Darkness(int front);
void Combat_Queue_Wall_Of_Fire(int front);
void Combat_Queue_Vortices(void);
void Combat_Queue_Unit_Figure(int slot, int x, int y, int dest_x, int dest_y, int step, int figure, int figures,
                              int owner, int aura, int intensity, int frame, int kills, int hit_set);
void Combat_Queue_Unit_Status_Icon(int x, int y, int dest_x, int dest_y, int step, int icon);
void Sprite_List_Clear(void);
void Figure_Render_Frame(int slot, int facing, int owner, int aura, int frame);
void Combat_Queue_Missiles(void);
void Figure_Formation_Offset(int figures, int figure, int *dx, int *dy);
void Sprite_List_Add(int x, int y, int pic, int x_ofs, int y_ofs, int frame, int kind, int unread_arg8, int unread_arg9,
                     int unread_arg10, int overlay, int unread_arg12, int overlay_frame);
int Sprite_List_Insert_Sorted(void);
void Figure_EMS_Map_Slot(int slot);
void Tiles_EMS_Map(void);
void Combat_Render_Unit_Figures(void);
void Combat_Draw_Uncalled_Nop(void);
void Combat_End_Nop(void);

#include "GAME.H"

int wall_set_index = 0;                         /* DS:706e  city-wall picture set (12 pictures per set) */
int battlefield_anim_toggle = 0;                /* DS:7070 */
int combsprt_unused_7072 = 1;                   /* DS:7072  no code reads it */
int combat_anim_frame4 = 0;                     /* DS:7074  0 .. 3, one step every second tick */
int battlefield_edge_anim_frame = 0;            /* DS:7076 */
int battlefield_tile_anim_frame = 0;            /* DS:7078 */
int battlefield_water_anim_frame = 0;           /* DS:707a */
int combsprt_unused_707c = 0;                   /* DS:707c  no code reads it */
int battlefield_anim_frame8 = 0;                /* DS:707e  0 .. 7: the node and volcano frames */
int combat_cursor_anim = 0;                     /* DS:7080 */
int combat_move_write_only = 0;                 /* DS:7082  zeroed before every move (and in AI_Move_Path), never read */
int missile_anim_frame_copy = 0;                /* DS:7084 */
int missile_anim_frame = 0;                     /* DS:7086 */
int combat_vortex_frame = 0;                    /* DS:7088  0 .. 5 */
int combat_effect_anim_frame = 0;                     /* DS:708a */
int combsprt_unused_708c = 0;                   /* DS:708c  no code reads it */
int combat_special_anim = 0;                    /* DS:708e */
int combat_anim_counter = 0;                    /* DS:7090 */
int combat_pingpong_dir = 0;                    /* DS:7092  0 counting up, 1 down */
int combat_pingpong_frame = 0;                  /* DS:7094  0, 1, 2, 1, 0 ...: a figure frame (COMBINFO.C) */
int battlefield_figure_anim_frame = 0;                   /* DS:7096 */
int combat_attack_anim_state = 1;               /* DS:7098 */
int combat_defend_anim_state = 3;               /* DS:709a */
int battlefield_reuse_map = 0;                  /* DS:709c  0 and never written; non-zero would make
                                                   Battlefield_Generate skip Battlefield_Generate_Map */

/* BSS, highest address first */
int combat_att_floating_island;                 /* DS:d382 */
int combat_def_floating_island;                 /* DS:d380 */
unsigned char sprite_order[256];                /* DS:d280  the sprite list's draw order (by depth) */
int wall_rise_kind;                      /* DS:d27e */
unsigned combat_red_burst_pic;                        /* DS:d27c  CMBTCITY 0x77: a 4-frame growing red burst, never drawn */
unsigned battlefield_missile_pics[15][8];                /* DS:d18c */
unsigned battlefield_target_marker_pic;                  /* DS:d18a */
unsigned battlefield_active_marker_pic;                  /* DS:d188 */
int hit_frame_counters[20];                     /* DS:d160  per figure (two sets of 10): a hit-frame counter */
/* hit_frame_counters: starts at Random(4) - 1 (0 .. 3), counts up to 5 as the figure is hit; the frame drawn is it - 2 */
int sprite_list_count;                          /* DS:d15e */
struct CombatSprite far *p_sprite_list;         /* DS:d15a */
int combat_missile_count;                       /* DS:d158 */
struct CombatMissile far *p_missile_list;       /* DS:d154 */
int vortex_count;                               /* DS:d152 */
struct CombatVortex far *p_vortices;            /* DS:d14e */
int combat_special_anim_x;                      /* DS:d14c */
int combat_special_anim_y;                      /* DS:d14a */
unsigned char *p_combat_move_cost;              /* DS:d148 */
unsigned char *p_combat_path_dist;              /* DS:d146 */
int *p_combat_path_prev;                         /* DS:d144 */
int combat_path_len;                            /* DS:d142 */
int *p_combat_path_x;                           /* DS:d140 */
int *p_combat_path_y;                           /* DS:d13e */
unsigned combat_vortex_pic;                     /* DS:d13c  CMBMAGIC.LBX 0x78: the Magic Vortex (6 frames) */
unsigned combat_effect_pic;                     /* DS:d13a */
unsigned battlefield_house_pics[15];            /* DS:d11c  the house pictures: 3 house styles x 5 (BFGEN.C) */
unsigned battlefield_fortress_pic;                        /* DS:d11a  CMBTCITY 0x11; it and the next seven are named by the backdrop they are drawn for */
unsigned battlefield_outpost_pic;                        /* DS:d118 */
unsigned battlefield_cave_pic;                        /* DS:d116 */
unsigned battlefield_tower_pic;                        /* DS:d114 */
unsigned battlefield_ruins_pic;                        /* DS:d112 */
unsigned battlefield_temple_pic;                        /* DS:d110 */
unsigned battlefield_fallen_temple_pic;                        /* DS:d10e */
unsigned battlefield_keep_pic;                        /* DS:d10c */
unsigned battlefield_city_base_pic;                      /* DS:d10a */
unsigned battlefield_outpost_base_pic;                   /* DS:d108 */
unsigned wall_pics[3][12];                      /* DS:d0c0 */
unsigned battlefield_wall_fire_pics[14];                 /* DS:d0a4 */
unsigned battlefield_wall_darkness_pics[14];             /* DS:d088 */
unsigned battlefield_volcano_pics[9];                    /* DS:d076 */
unsigned battlefield_nature_node_pic;                        /* DS:d074  CMBTCITY 0x41 'GREENODE': drawn for the nature node backdrop */
unsigned battlefield_sorcery_node_pic;          /* DS:d072  CMBTCITY 0x42 'CITYNODE': drawn only on the sorcery node (the nature node has its own, chaos the volcano) */
unsigned battlefield_water_pic;                          /* DS:d070 */
unsigned battlefield_floating_island_pic;                /* DS:d06e */
unsigned combsprt_unused_d068[3];               /* DS:d068  no code reads it */
unsigned battlefield_flying_fortress_pic;                /* DS:d066 */
unsigned battlefield_chaos_water_pics[4];       /* DS:d05e  CHRIVER.LBX 0x0c .. ('CHAOSWAT'): Myrror's water, chosen by plane, not by Chaos */
unsigned battlefield_water_pics[4];                      /* DS:d056 */
unsigned battlefield_road_pics[28];                      /* DS:d01e */
unsigned battlefield_chaos_river_pics[12];      /* DS:d006  CHRIVER.LBX 0 ..: Myrror's rivers ('chaos' is the file's name) */
unsigned battlefield_tile_anim_pics[12];                 /* DS:cfee */
unsigned battlefield_decor_a_pics[5];                    /* DS:cfe4 */
unsigned battlefield_decor_b_pics[5];                    /* DS:cfda */
unsigned battlefield_curse_pics[8];                      /* DS:cfca */
unsigned battlefield_blood_pics[6];                      /* DS:cfbe */
unsigned wall_rise_pics[14];                    /* DS:cfa2  the walls' rising animation (WALLRISE.LBX, wall_rise_kind) */
int battlefield_enchanted_road;                          /* DS:cfa0 */

extern unsigned ems_frame_seg;                  /* DS:760c */
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
extern unsigned combat_sprite_pics[COMBAT_SPRITE_SLOTS];         /* DS:c550 */
extern int battle_unit_count;                   /* DS:c588 */
extern int ems_tiles_handle;                    /* DS:cf9a */
extern int ems_figures_handle;                  /* DS:cf9c */

void Battlefield_Build(void);
void Battlefield_Generate(int x, int y, int plane);
void Set_Clip_Rect(int x1, int y1, int x2, int y2);
void Set_Pic_Frame(unsigned pic, unsigned frame);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
void Pic_Decode_Frame_To_Bitmap(unsigned pic, unsigned bitmap);
int Pic_Width(unsigned pic);
int Pic_Height(unsigned pic);
int Pic_Frame_Count(unsigned pic);
int Mouse_X(void);
int Mouse_Y(void);
void Combat_Grid_To_Screen(int gx, int gy, int ox, int oy, int *sx, int *sy);
void Combat_Screen_To_Grid_X_Sub(int sx, int sy, int *cell, int *sub);
void Combat_Screen_To_Grid_Y_Sub(int sx, int sy, int *cell, int *sub);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);
void EMS_Map_Pages(int first, int handle);
void Banner_Colorize(int player);
void Combat_Unit_Enchant_Outline(int aura_realm);
void Unit_Pic_Apply_Effect(int effect);
void Unit_Pic_Hover_Highlight(int bu);

/* (x, y, plane) builds the battlefield: allocates the combat blocks, then generates the map (no callers) */
void Battlefield_Setup(int x, int y, int plane)
{
    Battlefield_Build();
    Battlefield_Generate(x, y, plane);
}

/* draws the battlefield: the unit figures are rendered first; then per isometric row the terrain tiles (mud as
   animated water; animated tiles 48-51 and 56+ by plane; 52-55 always battlefield_chaos_water_pics[tile - 48] -
   BUG #450: entries 4..7 of a 4-entry array), the Sorcery node (backdrop 9) and the Chaos node's volcano
   (backdrop 10) at (6, 11), the roads (or a floating island), the outpost (backdrop 1) or, in a city or fortress
   battle (backdrops 2, 3), the city base - the Flying Fortress when the city has it - and the special animation square; then the target and active-unit markers; the animation counters
   advance and the sprite list is drawn */
void Combat_Draw_Battlefield(void)
{
    int i;
    int row_start;
    int row_end;
    int x;
    int y;
    int gx;
    int gy;
    int road_set;
    int road_ofs;
    unsigned char road;
    int row;
    int tile;

    Screen_Restore_A800();
    Combat_Render_Unit_Figures();
    Set_Draw_Page();
    Set_Clip_Rect(0, 0, 319, 163);
    for (row = 0; row < 22; row++) {
        row_start = row / 2;
        row_end = (row + 1) / 2 + 9;
        for (i = 0; i < 11; i++) {
            gx = row_start + i;
            gy = row_end - i;
            x = i * 32 - (row & 1) * 16;
            y = row * 8 - 8;
            if (gx < 0)
                continue;
            if (gx >= 21)
                continue;
            if (gy < 0)
                continue;
            if (gy >= 22)
                continue;
            if (p_battlefield->mud[gy][gx] != 0) {
                Set_Pic_Frame(battlefield_water_pic, (gx + gy + battlefield_water_anim_frame) % 8);
                Draw_Pic_Clipped(x, y, battlefield_water_pic);
                continue;
            }
            tile = p_battlefield->tile[gy][gx];
            if (tile >= 56) {
                if (p_battlefield->plane == PLANE_ARCANUS) {
                    Set_Pic_Frame(battlefield_tile_anim_pics[tile - 56], battlefield_tile_anim_frame);
                    Draw_Pic_Clipped(x, y, battlefield_tile_anim_pics[tile - 56]);
                } else {
                    Set_Pic_Frame(battlefield_chaos_river_pics[tile - 56], battlefield_tile_anim_frame);
                    Draw_Pic_Clipped(x, y, battlefield_chaos_river_pics[tile - 56]);
                }
            } else if (tile >= 48 && tile < 52) {
                if (p_battlefield->plane == PLANE_ARCANUS) {
                    Set_Pic_Frame(battlefield_water_pics[tile - 48], battlefield_tile_anim_frame);
                    Draw_Pic_Clipped(x, y, battlefield_water_pics[tile - 48]);
                } else {
                    Set_Pic_Frame(battlefield_chaos_water_pics[tile - 48], battlefield_tile_anim_frame);
                    Draw_Pic_Clipped(x, y, battlefield_chaos_water_pics[tile - 48]);
                }
            } else if (tile >= 52 && tile < 56) {
                Set_Pic_Frame(battlefield_chaos_water_pics[tile - 48], battlefield_tile_anim_frame);
                Draw_Pic_Clipped(x, y, battlefield_chaos_water_pics[tile - 48]);
            }
            if (p_battlefield->backdrop == BACKDROP_SORCERY_NODE && gx == BF_FEATURE_X && gy == BF_FEATURE_Y) {
                Combat_Grid_To_Screen(BF_FEATURE_X, BF_FEATURE_Y, 0, 0, &x, &y);
                x -= 46;
                y -= 15;
                Set_Pic_Frame(battlefield_sorcery_node_pic, battlefield_anim_frame8);
                Draw_Pic_Clipped(x, y, battlefield_sorcery_node_pic);
                x += 46;
                y += 15;
            }
            if (p_battlefield->backdrop == BACKDROP_CHAOS_NODE && gx == BF_FEATURE_X && gy == BF_FEATURE_Y) {
                Combat_Grid_To_Screen(BF_FEATURE_X, BF_FEATURE_Y, 0, 0, &x, &y);
                x -= 10;
                y -= 2;
                Draw_Pic_Frame(x, y, battlefield_volcano_pics[battlefield_anim_frame8 % 8]);
                x += 10;
                y += 2;
            }
            road = p_battlefield->road[gy][gx];  /* BF_ROAD_ISLAND (BF_ROAD_ISLAND_PIC: its picture square); else
                                                   1 picks the first road picture set, 2 .. 0x40 six road pieces */
            if (road != 0) {
                if ((road & BF_ROAD_ISLAND) && (road & 1)) {
                    Set_Pic_Frame(battlefield_floating_island_pic, battlefield_edge_anim_frame);
                    Draw_Pic_Clipped(x - 32, y - 48, battlefield_floating_island_pic);
                } else {
                    if (battlefield_enchanted_road == 1)
                        road_set = 14;
                    else
                        road_set = 0;
                    if (road & 1)
                        road_ofs = 0;
                    else
                        road_ofs = 7;
                    if (road & 2) {
                        Set_Pic_Frame(battlefield_road_pics[road_set + road_ofs], battlefield_edge_anim_frame);
                        Draw_Pic_Clipped(x, y, battlefield_road_pics[road_set + road_ofs]);
                    }
                    if (road & 4) {
                        Set_Pic_Frame(battlefield_road_pics[road_set + road_ofs + 1], battlefield_edge_anim_frame);
                        Draw_Pic_Clipped(x, y, battlefield_road_pics[road_set + road_ofs + 1]);
                    }
                    if (road & 8) {
                        Set_Pic_Frame(battlefield_road_pics[road_set + road_ofs + 2], battlefield_edge_anim_frame);
                        Draw_Pic_Clipped(x, y, battlefield_road_pics[road_set + road_ofs + 2]);
                    }
                    if (road & 0x10) {
                        Set_Pic_Frame(battlefield_road_pics[road_set + road_ofs + 3], battlefield_edge_anim_frame);
                        Draw_Pic_Clipped(x, y, battlefield_road_pics[road_set + road_ofs + 3]);
                    }
                    if (road & 0x20) {
                        Set_Pic_Frame(battlefield_road_pics[road_set + road_ofs + 4], battlefield_edge_anim_frame);
                        Draw_Pic_Clipped(x, y, battlefield_road_pics[road_set + road_ofs + 4]);
                    }
                    if (road & 0x40) {
                        Set_Pic_Frame(battlefield_road_pics[road_set + road_ofs + 5], battlefield_edge_anim_frame);
                        Draw_Pic_Clipped(x, y, battlefield_road_pics[road_set + road_ofs + 5]);
                    }
                }
            }
            if (p_battlefield->backdrop == BACKDROP_OUTPOST && gx == BF_FEATURE_X && gy == BF_FEATURE_Y)
                Draw_Pic_Clipped(x, y, battlefield_outpost_base_pic);
            if ((p_battlefield->backdrop == BACKDROP_CITY || p_battlefield->backdrop == BACKDROP_FORTRESS) && gx == 8 && gy == 13) {
                if (p_battlefield->city_enchants[CITY_ENCH_FLYING_FORTRESS] != 0)
                    Draw_Pic_Clipped(x - 48, y - 48, battlefield_flying_fortress_pic);
                else
                    Draw_Pic_Clipped(x - 48, y - 48, battlefield_city_base_pic);
            }
            if (combat_special_anim == 1 && combat_special_anim_x == gx && combat_special_anim_y == gy) {
                Set_Pic_Frame(combat_effect_pic, combat_effect_anim_frame);
                Draw_Pic_Clipped(x, y - 18, combat_effect_pic);
            }
        }
    }
    for (row = 0; row < 22; row++) {
        row_start = row / 2;
        row_end = (row + 1) / 2 + 9;
        for (i = 0; i < 11; i++) {
            gx = row_start + i;
            gy = row_end - i;
            x = i * 32 - (row & 1) * 16;
            y = row * 8 - 8;
            if (combat_cursor_mode == 1 && gx == combat_target_x && gy == combat_target_y) {
                Set_Pic_Frame(battlefield_target_marker_pic, combat_cursor_anim);
                Draw_Pic_Clipped(x, y, battlefield_target_marker_pic);
            }
            if (combat_active_marker_on == 1 && gx == combat_active_marker_x && gy == combat_active_marker_y) {
                Set_Pic_Frame(battlefield_active_marker_pic, combat_cursor_anim);
                Draw_Pic_Clipped(x, y, battlefield_active_marker_pic);
            }
        }
    }
    battlefield_edge_anim_frame = (battlefield_edge_anim_frame + 1) % 5;
    battlefield_tile_anim_frame = (battlefield_tile_anim_frame + 1) % 5;
    battlefield_anim_frame8 = (battlefield_anim_frame8 + 1) % 8;
    if (battlefield_anim_toggle == 0)
        combat_anim_frame4 = (combat_anim_frame4 + 1) % 4;
    battlefield_anim_toggle = 1 - battlefield_anim_toggle;
    combat_anim_counter++;
    if (combat_anim_counter > 30000)
        combat_anim_counter = 0;
    if (combat_pingpong_dir == 0) {
        combat_pingpong_frame++;
        if (combat_pingpong_frame == 3) {
            combat_pingpong_dir = 1;
            combat_pingpong_frame = 1;
        }
    } else {
        combat_pingpong_frame--;
        if (combat_pingpong_frame == -1) {
            combat_pingpong_dir = 0;
            combat_pingpong_frame = 1;
        }
    }
    battlefield_figure_anim_frame = (battlefield_figure_anim_frame + 1) % 3;
    if (combat_attack_anim_state == 1)
        combat_attack_anim_state = 3;
    else
        combat_attack_anim_state = 1;
    if (combat_defend_anim_state == 1)
        combat_defend_anim_state = 3;
    else
        combat_defend_anim_state = 1;
    Combat_Draw_Sprite_List();
    Reset_Clip_Rect();
}

/* draws the sprite list in its depth order: unit figures (kind 1) with their hit overlay (a blood frame; at
   overlay frame 3 neither the blood nor the figure - it vanishes; below 0 the figure without blood), pictures (0), EMS-mapped specials (4); then the missiles (3) and animations (2) on top.
   The mouse's grid square is computed at the end and not used */
void Combat_Draw_Sprite_List(void)
{
    int oy;
    int ox;
    int x_cell;
    int y_cell;
    int x_sub;
    int y_sub;
    int k;
    int i;

    for (i = 0; i < sprite_list_count; i++) {
        k = sprite_order[i];
        if (p_sprite_list[k].kind == SPRITE_FIGURE) {
            if (p_sprite_list[k].overlay != 0) {
                if (p_sprite_list[k].overlay_frame != 3) {
                    Draw_Bitmap_Clipped(p_sprite_list[k].x - p_sprite_list[k].x_ofs,
                                         p_sprite_list[k].y - p_sprite_list[k].y_ofs,
                                         combat_sprite_pics[p_sprite_list[k].pic]);
                    if (p_sprite_list[k].overlay_frame >= 0) {
                        Set_Pic_Frame(battlefield_blood_pics[p_sprite_list[k].overlay - 1], p_sprite_list[k].overlay_frame);
                        oy = 11;
                        ox = 3;
                        Draw_Pic_Clipped(p_sprite_list[k].x - ox, p_sprite_list[k].y - oy,
                                         battlefield_blood_pics[p_sprite_list[k].overlay - 1]);
                    }
                }
            } else
                Draw_Bitmap_Clipped(p_sprite_list[k].x - p_sprite_list[k].x_ofs,
                                     p_sprite_list[k].y - p_sprite_list[k].y_ofs,
                                     combat_sprite_pics[p_sprite_list[k].pic]);
        } else if (p_sprite_list[k].kind == SPRITE_PIC) {
            Set_Pic_Frame(p_sprite_list[k].pic, p_sprite_list[k].frame);
            Draw_Pic_Clipped(p_sprite_list[k].x - p_sprite_list[k].x_ofs, p_sprite_list[k].y - p_sprite_list[k].y_ofs,
                             p_sprite_list[k].pic);
        } else if (p_sprite_list[k].kind == SPRITE_EMS_PIC) {
            Tiles_EMS_Map();
            Draw_Pic_Clipped(p_sprite_list[k].x - p_sprite_list[k].x_ofs, p_sprite_list[k].y - p_sprite_list[k].y_ofs,
                             p_sprite_list[k].pic);
        }
    }
    for (i = 0; i < sprite_list_count; i++) {
        k = sprite_order[i];
        if (p_sprite_list[k].kind == SPRITE_MISSILE) {
            Set_Pic_Frame(p_sprite_list[k].pic, missile_anim_frame);
            Draw_Pic_Clipped(p_sprite_list[k].x - p_sprite_list[k].x_ofs, p_sprite_list[k].y - p_sprite_list[k].y_ofs,
                             p_sprite_list[k].pic);
        } else if (p_sprite_list[k].kind == SPRITE_ANIM) {
            Set_Pic_Frame(p_sprite_list[k].pic, (combat_anim_counter + i) % 8);
            Draw_Pic_Clipped(p_sprite_list[k].x - p_sprite_list[k].x_ofs, p_sprite_list[k].y - p_sprite_list[k].y_ofs,
                             p_sprite_list[k].pic);
        }
    }
    Combat_Screen_To_Grid_X_Sub(Mouse_X(), Mouse_Y(), &x_cell, &x_sub);
    Combat_Screen_To_Grid_Y_Sub(Mouse_X(), Mouse_Y(), &y_cell, &y_sub);
}

/* the 20 hit-animation counters start at a random 0..3 */
void Hit_Anim_Reset_Jitter(void)
{
    int i;

    for (i = 0; i < 20; i++)
        hit_frame_counters[i] = Random(4) - 1;
}

/* queues the battlefield's first set of terrain objects */
void Combat_Queue_Terrain_Objects_A(void)
{
    int i;

    for (i = 0; i < p_battlefield->decor_a_count; i++)
        Sprite_List_Add(p_battlefield->decor_a_x[i], p_battlefield->decor_a_y[i],
                        battlefield_decor_a_pics[p_battlefield->decor_a_index[i]], 8, 13, 0, SPRITE_EMS_PIC, 0, 0, 0, 0, 0, 0);
}

/* queues the second set of terrain objects: BUG #342 - decor_b's positions and picture table, but the count and the
   picture indices are the first set's
   Numbered: BUG #342 (BUGS_AND_QUIRKS.txt) */
void Combat_Queue_Terrain_Objects_B(void)
{
    int i;

    for (i = 0; i < p_battlefield->decor_a_count; i++)
        Sprite_List_Add(p_battlefield->decor_b_x[i], p_battlefield->decor_b_y[i],
                        battlefield_decor_b_pics[p_battlefield->decor_a_index[i]], 6, 12, 0, SPRITE_EMS_PIC, 0, 0, 0, 0, 0, 0);
}

/* queues the backdrop's centre feature (a tower, temple, node, ... from CMBTCITY), the houses of a city
   battlefield, and the walls: the back halves first, then the front halves */
void Combat_Queue_Features_And_Walls(void)
{
    int x;
    int y;
    int pic;
    int i;

    if (p_battlefield->backdrop != BACKDROP_FIELD) {
        Combat_Grid_To_Screen(BF_FEATURE_X, BF_FEATURE_Y, 0, 0, &x, &y);
        switch (p_battlefield->backdrop) {
        case BACKDROP_OUTPOST:
            pic = battlefield_outpost_pic;
            break;
        case BACKDROP_CITY:
            pic = 0;
            break;
        case BACKDROP_FORTRESS:
            pic = battlefield_fortress_pic;
            break;
        case BACKDROP_DUNGEON_RUINS:
            pic = battlefield_ruins_pic;
            break;
        case BACKDROP_TOWER:
            pic = battlefield_tower_pic;
            break;
        case BACKDROP_CAVE_LAIR:
            pic = battlefield_cave_pic;
            break;
        case BACKDROP_ANCIENT_TEMPLE:
            pic = battlefield_temple_pic;
            break;
        case BACKDROP_KEEP:
            pic = battlefield_keep_pic;
            break;
        case BACKDROP_SORCERY_NODE:
            pic = 0;
            break;
        case BACKDROP_CHAOS_NODE:
            pic = 0;
            break;
        case BACKDROP_NATURE_NODE:
            pic = battlefield_nature_node_pic;
            break;
        case BACKDROP_FALLEN_TEMPLE:
            pic = battlefield_fallen_temple_pic;
            break;
        }
        if (pic != 0) {
            if (p_battlefield->backdrop == BACKDROP_NATURE_NODE)
                Sprite_List_Add(x, y, pic, 16, 15, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
            else
                Sprite_List_Add(x, y, pic, 15, Pic_Height(pic) - 21, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
    if (p_battlefield->backdrop == BACKDROP_CITY || p_battlefield->backdrop == BACKDROP_FORTRESS) {
        for (i = 0; i < p_battlefield->house_count; i++) {
            Combat_Grid_To_Screen(p_battlefield->house_x[i], p_battlefield->house_y[i], 0, 0, &x, &y);
            y += 2;
            Sprite_List_Add(x, y, p_battlefield->house_pic[i], Pic_Width(p_battlefield->house_pic[i]) / 2,
                            Pic_Height(p_battlefield->house_pic[i]) - 14, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
    if (p_battlefield->has_city_walls == 1)
        Combat_Queue_City_Walls(0);
    if (p_battlefield->wall_of_darkness == 1)
        Combat_Queue_Wall_Of_Darkness(0);
    if (p_battlefield->wall_of_fire == 1)
        Combat_Queue_Wall_Of_Fire(0);
    if (p_battlefield->wall_of_fire == 1)
        Combat_Queue_Wall_Of_Fire(1);
    if (p_battlefield->wall_of_darkness == 1)
        Combat_Queue_Wall_Of_Darkness(1);
    if (p_battlefield->has_city_walls == 1)
        Combat_Queue_City_Walls(1);
}

/* (front) queues the twelve city-wall pieces of the back (0) or front (1) half: the rising walls while the
   wall-rise animation runs, else each standing (1) or broken (2) segment of the wall set */
void Combat_Queue_City_Walls(int front)
{
    int x;
    int y;
    int i;

    for (i = 0; i < 1 && front == 0; i++) {
        Combat_Grid_To_Screen(5, i + 10, 0, 0, &x, &y);
        y++;
        x++;
        if (wall_rise_kind == WALL_RISE_STONE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 15, 18, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else {
            if (p_battlefield->wall_segments[i][0] == WALL_STANDING)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], Pic_Width(wall_pics[wall_set_index][i]) / 2, 18,
                                0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
            if (p_battlefield->wall_segments[i][0] == WALL_BROKEN)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], Pic_Width(wall_pics[wall_set_index][i]) / 2, 18,
                                1, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
    for (i = 1; i < 4 && front == 0; i++) {
        Combat_Grid_To_Screen(5, i + 10, 0, 0, &x, &y);
        x++;
        if (i > 0)
            y++;
        if (wall_rise_kind == WALL_RISE_STONE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 17, 17, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else {
            if (p_battlefield->wall_segments[i][0] == WALL_STANDING)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 17, 17, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
            if (p_battlefield->wall_segments[i][0] == WALL_BROKEN)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 17, 17, 1, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
    for (i = 4; i < 7 && front == 0; i++) {
        Combat_Grid_To_Screen(i + 2, 10, 0, 0, &x, &y);
        x++;
        if (wall_rise_kind == WALL_RISE_STONE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 17, 17, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else {
            if (p_battlefield->wall_segments[0][i - 3] == WALL_STANDING)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 17, 17, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
            if (p_battlefield->wall_segments[0][i - 3] == WALL_BROKEN)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 17, 17, 1, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
    for (i = 7; i < 10 && front == 1; i++) {
        Combat_Grid_To_Screen(i - 1, 13, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_STONE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 30, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else {
            if (p_battlefield->wall_segments[3][i - 6] == WALL_STANDING)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 16, 30, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
            if (p_battlefield->wall_segments[3][i - 6] == WALL_BROKEN)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 16, 30, 1, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
    for (i = 10; i < 12 && front == 1; i++) {
        Combat_Grid_To_Screen(8, i + 1, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_STONE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 30, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else {
            if (p_battlefield->wall_segments[i - 9][3] == WALL_STANDING)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 16, 30, 0, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
            if (p_battlefield->wall_segments[i - 9][3] == WALL_BROKEN)
                Sprite_List_Add(x, y, wall_pics[wall_set_index][i], 16, 30, 1, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        }
    }
}

/* (front) queues the fourteen Wall of Darkness pieces of the back (0) or front (1) half: the rising wall while
   its wall-rise animation (2) runs */
void Combat_Queue_Wall_Of_Darkness(int front)
{
    int x;
    int y;
    int i;

    for (i = 0; i < 1 && front == 0; i++) {
        Combat_Grid_To_Screen(5, i + 10, 0, 0, &x, &y);
        y++;
        x++;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 15, 19, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], Pic_Width(battlefield_wall_darkness_pics[i]) / 2, 19,
                            combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 1; i < 4 && front == 0; i++) {
        Combat_Grid_To_Screen(5, i + 10, 0, 0, &x, &y);
        x++;
        if (i > 0)
            y++;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 17, 18, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], 17, 18, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 4; i < 7 && front == 0; i++) {
        Combat_Grid_To_Screen(i + 2, 10, 0, 0, &x, &y);
        x++;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 17, 17, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], 17, 17, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 7; i < 10 && front == 1; i++) {
        Combat_Grid_To_Screen(i - 1, 13, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 29, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], 16, 29, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 10; i < 12 && front == 1; i++) {
        Combat_Grid_To_Screen(8, i + 1, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 29, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], 16, 29, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 12; i < 13 && front == 1; i++) {
        Combat_Grid_To_Screen(5, 13, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 30, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], 16, 30, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 13; i < 14 && front == 1; i++) {
        Combat_Grid_To_Screen(8, 10, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_DARKNESS && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 30, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_darkness_pics[i], 16, 30, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
}

/* (front) the Wall of Fire, as the Wall of Darkness (its wall-rise animation is 1) */
void Combat_Queue_Wall_Of_Fire(int front)
{
    int x;
    int y;
    int i;

    for (i = 0; i < 1 && front == 0; i++) {
        Combat_Grid_To_Screen(5, i + 10, 0, 0, &x, &y);
        y++;
        x++;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 15, 19, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], Pic_Width(battlefield_wall_fire_pics[i]) / 2, 19,
                            combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 1; i < 4 && front == 0; i++) {
        Combat_Grid_To_Screen(5, i + 10, 0, 0, &x, &y);
        x++;
        if (i > 0)
            y++;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 17, 18, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], 17, 18, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 4; i < 7 && front == 0; i++) {
        Combat_Grid_To_Screen(i + 2, 10, 0, 0, &x, &y);
        x++;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 17, 17, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], 17, 17, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 7; i < 10 && front == 1; i++) {
        Combat_Grid_To_Screen(i - 1, 13, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 29, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], 16, 29, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 10; i < 12 && front == 1; i++) {
        Combat_Grid_To_Screen(8, i + 1, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 29, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], 16, 29, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 12; i < 13 && front == 1; i++) {
        Combat_Grid_To_Screen(5, 13, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 30, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], 16, 30, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
    for (i = 13; i < 14 && front == 1; i++) {
        Combat_Grid_To_Screen(8, 10, 0, 0, &x, &y);
        y += 14;
        if (wall_rise_kind == WALL_RISE_FIRE && combat_wall_rising == 1)
            Sprite_List_Add(x, y, wall_rise_pics[i], 16, 30, combat_wall_anim_frame, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
        else
            Sprite_List_Add(x, y, battlefield_wall_fire_pics[i], 16, 30, combat_anim_frame4, SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
}

/* queues the magic vortices, each part-way (anim_step / 8) from its square to its destination */
void Combat_Queue_Vortices(void)
{
    int x;
    int y;
    int x1;
    int x2;
    int y1;
    int y2;
    int i;

    for (i = 0; i < vortex_count; i++) {
        Combat_Grid_To_Screen(p_vortices[i].x, p_vortices[i].y, 0, 0, &x1, &y1);
        Combat_Grid_To_Screen(p_vortices[i].dest_x, p_vortices[i].dest_y, 0, 0, &x2, &y2);
        y1 += 8;
        y2 += 8;
        x = p_vortices[i].anim_step * (x2 - x1) / 8 + x1;
        y = p_vortices[i].anim_step * (y2 - y1) / 8 + y1;
        Sprite_List_Add(x, y, combat_vortex_pic, 13, 25,
                        (combat_vortex_frame + i) % Pic_Frame_Count(combat_vortex_pic), SPRITE_PIC, 0, 0, 0, 0, 0, 0);
    }
}

/* queues one figure of a unit, part-way (step / 8) from its square to its destination, at its formation offset,
   facing the way it moves; a figure hit by an attack (figure < kills) steps its own hit-animation counter. The
   formation offsets are inline: the positions of Figure_Formation_Offset (the missiles, BFIELD; the info screen,
   COMBINFO) in another figure order (4 figures: figure 1 at (-7, 8) here, (8, 8) there) */
void Combat_Queue_Unit_Figure(int slot, int x, int y, int dest_x, int dest_y, int step, int figure, int figures,
                              int owner, int aura, int intensity, int frame, int kills, int hit_set)
{
    int x1;
    int x2;
    int sx;
    int y1;
    int y2;
    int sy;
    int oy;
    int hit_count;
    int facing;
    int hit_frame;
    int ox;

    Combat_Grid_To_Screen(x, y, 0, 0, &x1, &y1);
    Combat_Grid_To_Screen(dest_x, dest_y, 0, 0, &x2, &y2);
    switch (figures) {
    case 1:
        ox = 1;
        oy = 8;
        break;
    case 2:
        if (figure == 0)
            ox = -7;
        else
            ox = 7;
        oy = 9;
        break;
    case 3:
        if (figure == 0) {
            ox = 0;
            oy = 4;
        } else if (figure == 1) {
            ox = -6;
            oy = 10;
        } else {
            ox = 7;
            oy = 10;
        }
        break;
    case 4:
        if (figure == 0) {
            ox = 1;
            oy = 4;
        } else if (figure == 1) {
            ox = -7;
            oy = 8;
        } else if (figure == 2) {
            ox = 8;
            oy = 8;
        } else {
            ox = 1;
            oy = 11;
        }
        break;
    case 5:
        if (figure == 0) {
            ox = 1;
            oy = 4;
        } else if (figure == 1) {
            ox = -7;
            oy = 8;
        } else if (figure == 2) {
            ox = 1;
            oy = 8;
        } else if (figure == 3) {
            ox = 8;
            oy = 8;
        } else {
            ox = 1;
            oy = 11;
        }
        break;
    case 6:
        if (figure == 0) {
            ox = 1;
            oy = 4;
        } else if (figure == 1) {
            ox = 4;
            oy = 7;
        } else if (figure == 2) {
            ox = -8;
            oy = 8;
        } else if (figure == 3) {
            ox = 9;
            oy = 8;
        } else if (figure == 4) {
            ox = -3;
            oy = 9;
        } else {
            ox = 1;
            oy = 11;
        }
        break;
    case 7:
        if (figure == 0) {
            ox = 1;
            oy = 4;
        } else if (figure == 1) {
            ox = 6;
            oy = 6;
        } else if (figure == 2) {
            ox = -8;
            oy = 8;
        } else if (figure == 3) {
            ox = 1;
            oy = 8;
        } else if (figure == 4) {
            ox = 10;
            oy = 8;
        } else if (figure == 5) {
            ox = -3;
            oy = 11;
        } else {
            ox = 1;
            oy = 11;
        }
        break;
    case 8:
        if (figure == 0) {
            ox = 1;
            oy = 4;
        } else if (figure == 1) {
            ox = 6;
            oy = 6;
        } else if (figure == 2) {
            ox = -2;
            oy = 7;
        } else if (figure == 3) {
            ox = -8;
            oy = 8;
        } else if (figure == 4) {
            ox = 10;
            oy = 8;
        } else if (figure == 5) {
            ox = 3;
            oy = 9;
        } else if (figure == 6) {
            ox = -3;
            oy = 11;
        } else {
            ox = 1;
            oy = 11;
        }
        break;
    }
    /* facing by the step: 0 (-x, -y), 1 (0, -y), 2 (+x, -y), 3 (+x, 0), 4 (+x, +y), 5 (0, +y), 6 (-x, +y), 7 (-x, 0);
       no move: 2 (Combat_Render_Unit_Figures repeats the ladder) */
    if (x > dest_x) {
        if (y > dest_y)
            facing = 0;
        else if (y < dest_y)
            facing = 6;
        else
            facing = 7;
    } else if (x < dest_x) {
        if (y > dest_y)
            facing = 2;
        else if (y < dest_y)
            facing = 4;
        else
            facing = 3;
    } else {
        if (y > dest_y)
            facing = 1;
        else if (y < dest_y)
            facing = 5;
        else
            facing = 2;
    }
    sx = (x2 - x1) * step / 8 + x1 + ox;
    sy = (y2 - y1) * step / 8 + y1 + oy;
    if (intensity > 0 && figure < kills) {
        if (hit_set == 0) {
            hit_count = hit_frame_counters[figure];
            hit_frame_counters[figure]++;
            if (hit_frame_counters[figure] > 5)
                hit_frame_counters[figure] = 5;
        } else {
            hit_count = hit_frame_counters[figure + 10];
            hit_frame_counters[figure + 10]++;
            if (hit_frame_counters[figure + 10] > 5)
                hit_frame_counters[figure + 10] = 5;
        }
        hit_count -= 2;
        hit_frame = hit_count;
        if (kills - 1 == figure && hit_frame == 3)
            hit_frame = 2;
    } else {
        hit_frame = 0;
        intensity = 0;
    }
    Sprite_List_Add(sx, sy, slot, 13, 23, frame, SPRITE_FIGURE, owner, facing, aura, intensity, frame, hit_frame);
}

/* queues a unit's curse / status icon, part-way (step / 8) from its square to its destination */
void Combat_Queue_Unit_Status_Icon(int x, int y, int dest_x, int dest_y, int step, int icon)
{
    int x1;
    int x2;
    int sx;
    int y1;
    int y2;
    int sy;

    Combat_Grid_To_Screen(x, y, 0, 0, &x1, &y1);
    Combat_Grid_To_Screen(dest_x, dest_y, 0, 0, &x2, &y2);
    sx = (x2 - x1) * step / 8 + x1;
    sy = (y2 - y1) * step / 8 + y1;
    Sprite_List_Add(sx, sy, battlefield_curse_pics[icon], 13, 15, combat_anim_counter, SPRITE_ANIM, 0, 0, 0, 0, 0, 0);
}

/* empties the sprite list (the draw order back to 0..255) */
void Sprite_List_Clear(void)
{
    int i;

    sprite_list_count = 0;
    for (i = 0; i < 256; i++)
        sprite_order[i] = i;
}

/* (slot, facing, owner, aura, frame) renders one figure picture of an EMS figure-cache slot into the work
   picture, in the owner's banner color with its enchantment outline */
void Figure_Render_Frame(int slot, int facing, int owner, int aura, int frame)
{
    unsigned seg;
    unsigned far *pics;
    int ofs;

    if ((slot & 1) == 0)                        /* a cache slot's paragraph offset in the EMS frame once Figure_EMS_Map_Slot
                                                   has mapped it: 0 / 0x200 for an even / odd slot, + slot x 56 (also
                                                   below, in BFIELD and COMBINFO; the x 56 term is not explained) */
        ofs = 0;
    else
        ofs = 0x200;
    ofs += slot * 56;
    Figure_EMS_Map_Slot(slot);
    seg = Heap_Alloc_Front(ems_frame_seg + ofs, 2);
    pics = Seg_To_Far_Ptr(seg);
    Set_Pic_Frame(pics[facing], frame);
    Pic_Decode_Frame_To_Bitmap(pics[facing], p_pic_work);
    Banner_Colorize(owner);
    Combat_Unit_Enchant_Outline(aura);
}

/* queues the missiles, each progress / 100 of the way; the arcing kinds (1, 4, 5) rise by |progress - 50| / 4 */
void Combat_Queue_Missiles(void)
{
    int x;
    int y;
    int arc;
    int i;

    for (i = 0; i < combat_missile_count; i++) {
        x = (long)(p_missile_list[i].x2 - p_missile_list[i].x1) * p_missile_list[i].progress / 100 +
            p_missile_list[i].x1;
        y = (long)(p_missile_list[i].y2 - p_missile_list[i].y1) * p_missile_list[i].progress / 100 +
            p_missile_list[i].y1;
        if (p_missile_list[i].kind == PROJECTILE_ARROW || p_missile_list[i].kind == PROJECTILE_BOULDER || p_missile_list[i].kind == PROJECTILE_SLING)
            arc = (p_missile_list[i].progress - 50) / 2 / 2;
        else
            arc = 0;
        if (arc < 0)
            arc = -arc;
        Sprite_List_Add(x, y + arc, battlefield_missile_pics[p_missile_list[i].kind][p_missile_list[i].direction], 14, 22, 0,
                        SPRITE_MISSILE, 0, 0, 0, 0, 0, 0);
    }
}

/* (figures, figure, &dx, &dy) a figure's offset in its unit's formation */
void Figure_Formation_Offset(int figures, int figure, int *dx, int *dy)
{
    switch (figures) {
    case 1:
        *dx = 1;
        *dy = 8;
        break;
    case 2:
        if (figure == 0)
            *dx = -7;
        else
            *dx = 7;
        *dy = 9;
        break;
    case 3:
        if (figure == 0) {
            *dx = 0;
            *dy = 4;
        } else if (figure == 1) {
            *dx = -6;
            *dy = 10;
        } else {
            *dx = 7;
            *dy = 10;
        }
        break;
    case 4:
        if (figure == 0) {
            *dx = 1;
            *dy = 4;
        } else if (figure == 1) {
            *dx = 8;
            *dy = 8;
        } else if (figure == 2) {
            *dx = 1;
            *dy = 12;
        } else {
            *dx = -7;
            *dy = 8;
        }
        break;
    case 5:
        if (figure == 0) {
            *dx = 1;
            *dy = 4;
        } else if (figure == 1) {
            *dx = 8;
            *dy = 8;
        } else if (figure == 2) {
            *dx = 1;
            *dy = 12;
        } else if (figure == 3) {
            *dx = -7;
            *dy = 8;
        } else {
            *dx = 1;
            *dy = 8;
        }
        break;
    case 6:
        if (figure == 0) {
            *dx = 1;
            *dy = 4;
        } else if (figure == 1) {
            *dx = 9;
            *dy = 8;
        } else if (figure == 2) {
            *dx = -8;
            *dy = 8;
        } else if (figure == 3) {
            *dx = 1;
            *dy = 12;
        } else if (figure == 4) {
            *dx = -3;
            *dy = 9;
        } else {
            *dx = 4;
            *dy = 7;
        }
        break;
    case 7:
        if (figure == 0) {
            *dx = 1;
            *dy = 4;
        } else if (figure == 1) {
            *dx = 10;
            *dy = 8;
        } else if (figure == 2) {
            *dx = -8;
            *dy = 8;
        } else if (figure == 3) {
            *dx = 1;
            *dy = 12;
        } else if (figure == 4) {
            *dx = 6;
            *dy = 6;
        } else if (figure == 5) {
            *dx = -3;
            *dy = 11;
        } else {
            *dx = 1;
            *dy = 8;
        }
        break;
    case 8:
        if (figure == 0) {
            *dx = 1;
            *dy = 4;
        } else if (figure == 1) {
            *dx = 10;
            *dy = 8;
        } else if (figure == 2) {
            *dx = -8;
            *dy = 8;
        } else if (figure == 3) {
            *dx = 2;
            *dy = 12;
        } else if (figure == 4) {
            *dx = 6;
            *dy = 6;
        } else if (figure == 5) {
            *dx = -3;
            *dy = 12;
        } else if (figure == 6) {
            *dx = -2;
            *dy = 7;
        } else {
            *dx = 3;
            *dy = 9;
        }
        break;
    }
}

/* adds a sprite to the list, with its depth (grid row, column, then the sub-square position) for the draw order;
   more than 256 sprites is fatal ("E1")
   Numbered: BUG #343 (BUGS_AND_QUIRKS.txt) */
void Sprite_List_Add(int x, int y, int pic, int x_ofs, int y_ofs, int frame, int sprite_kind, int unread_arg8, int unread_arg9,
                     int unread_arg10, int overlay, int unread_arg12, int overlay_frame)
{
    int gx;
    int gy;
    int sub_x;
    int sub_y;

    if (sprite_list_count > 256)                /* BUG #343: lets 256 through, one past sprite_order */
        Fatal_Exit("E1");
    p_sprite_list[sprite_list_count].x = x;
    p_sprite_list[sprite_list_count].y = y;
    p_sprite_list[sprite_list_count].pic = pic;
    p_sprite_list[sprite_list_count].frame = frame;
    p_sprite_list[sprite_list_count].x_ofs = x_ofs;
    p_sprite_list[sprite_list_count].y_ofs = y_ofs;
    p_sprite_list[sprite_list_count].kind = sprite_kind;
    p_sprite_list[sprite_list_count].unread_arg8 = unread_arg8;
    p_sprite_list[sprite_list_count].unread_arg9 = unread_arg9;
    p_sprite_list[sprite_list_count].unread_arg10 = unread_arg10;
    p_sprite_list[sprite_list_count].unread_arg12 = unread_arg12;
    p_sprite_list[sprite_list_count].overlay_frame = overlay_frame;
    p_sprite_list[sprite_list_count].overlay = overlay;
    Combat_Screen_To_Grid_X_Sub(x, y, &gx, &sub_x);
    Combat_Screen_To_Grid_Y_Sub(x, y, &gy, &sub_y);
    p_sprite_list[sprite_list_count].depth = gy * 8000L + gx * 320L + ((long)sub_y << 4) + sub_x;
    Sprite_List_Insert_Sorted();
    sprite_list_count++;
}

/* inserts the newest sprite into the draw order before the first deeper one; the result is meaningless
   (0 when the order is unchanged, no return statement otherwise) */
int Sprite_List_Insert_Sorted(void)
{
    long depth;
    int i;
    int j;
    int pos;

    if (sprite_list_count == 0)
        return 0;
    depth = p_sprite_list[sprite_list_count].depth;
    for (i = 0; i < sprite_list_count && p_sprite_list[sprite_order[i]].depth < depth; i++)
        ;
    if (i == sprite_list_count)
        return 0;
    pos = i;
    for (j = sprite_list_count; j >= pos; j--)   /* one step too many: the j == pos copy is overwritten below, and
                                                   at pos 0 it reads the word before sprite_order (no effect);
                                                   clarified using ReMoM repo */
        sprite_order[j] = sprite_order[j - 1];
    sprite_order[pos] = sprite_list_count;
}

/* maps the EMS pages of a figure-cache slot (two slots share three pages) into the page frame and writes a heap
   block header there (0x637 paragraphs, 1 used: the header) so the figure loaders can allocate in it */
void Figure_EMS_Map_Slot(int slot)
{
    int page;
    int ofs;

    page = slot * 3 / 2;
    if ((slot & 1) == 0)
        ofs = 0;
    else
        ofs = 0x200;
    ofs += slot * 56;
    EMS_Map_Pages(page, ems_figures_handle);
    Far_Poke_Word(ems_frame_seg + ofs, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(ems_frame_seg + ofs, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(ems_frame_seg + ofs, HEAP_HDR_SIZE, 0x637);
    Far_Poke_Word(ems_frame_seg + ofs, HEAP_HDR_USED, 1);
}

/* maps the terrain-tile EMS pages (page 0) into the page frame, with a heap block header (0xbff paragraphs) */
void Tiles_EMS_Map(void)
{
    EMS_Map_Pages(0, ems_tiles_handle);
    Far_Poke_Word(ems_frame_seg, HEAP_HDR_MAGIC1, HEAP_MAGIC1);
    Far_Poke_Word(ems_frame_seg, HEAP_HDR_MAGIC2, HEAP_MAGIC2);
    Far_Poke_Word(ems_frame_seg, HEAP_HDR_SIZE, 0xbff);
    Far_Poke_Word(ems_frame_seg, HEAP_HDR_USED, 1);
}

/* () empty; no caller */
void Combat_Draw_Uncalled_Nop(void)
{
}

/* renders every live battle unit's current figure picture (frame by its animation state, facing by its move)
   into its combat sprite buffer */
void Combat_Render_Unit_Figures(void)
{
    int x;
    int dest_x;
    int y;
    int dest_y;
    unsigned seg;
    int ofs;
    unsigned far *pics;
    int slot;
    int owner;
    int aura;
    int frame;
    int draw_mode;
    unsigned saved_work;
    int i;
    int facing;

    saved_work = p_pic_work;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status != BATTLE_UNIT_ACTIVE)
            continue;
        slot = p_battle_units[i].gfx_slot;
        owner = p_battle_units[i].owner;
        aura = p_battle_units[i].aura_color;
        draw_mode = p_battle_units[i].draw_mode;
        if (p_battle_units[i].always_animate == 1)
            frame = battlefield_figure_anim_frame;
        else if (p_battle_units[i].moving == 1)
            frame = p_battle_units[i].anim_ability == 1 ? battlefield_figure_anim_frame : combat_pingpong_frame;
        else
            frame = 1;
        if (p_battle_units[i].attack_anim_role != 0)
            frame = p_battle_units[i].attack_anim_role == ANIM_ROLE_ATTACKER ? combat_attack_anim_state : combat_defend_anim_state;
        x = p_battle_units[i].combat_x;
        y = p_battle_units[i].combat_y;
        dest_x = p_battle_units[i].dest_x;
        dest_y = p_battle_units[i].dest_y;
        if (x > dest_x) {
            if (y > dest_y)
                facing = 0;
            else if (y < dest_y)
                facing = 6;
            else
                facing = 7;
        } else if (x < dest_x) {
            if (y > dest_y)
                facing = 2;
            else if (y < dest_y)
                facing = 4;
            else
                facing = 3;
        } else {
            if (y > dest_y)
                facing = 1;
            else if (y < dest_y)
                facing = 5;
            else
                facing = 2;
        }
        Figure_EMS_Map_Slot(slot);
        if ((slot & 1) == 0)
            ofs = 0;
        else
            ofs = 0x200;
        ofs += slot * 56;
        seg = Heap_Alloc_Front(ems_frame_seg + ofs, 2);
        pics = Seg_To_Far_Ptr(seg);
        Set_Pic_Frame(pics[facing], frame);
        p_pic_work = combat_sprite_pics[p_battle_units[i].gfx_slot];
        Pic_Decode_Frame_To_Bitmap(pics[facing], p_pic_work);
        Banner_Colorize(owner);
        Combat_Unit_Enchant_Outline(aura);
        Unit_Pic_Apply_Effect(draw_mode);
        Unit_Pic_Hover_Highlight(i);
    }
    p_pic_work = saved_work;
}

/* () empty; Overland_Attack calls it after a battle */
void Combat_End_Nop(void)
{
}
