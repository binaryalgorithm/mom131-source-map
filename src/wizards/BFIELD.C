/* BFIELD.C - WIZARDS.EXE overlay code segment f900 (stub 26a1): the battlefield - loading the combat terrain tiles
   (CMBGRASS / CMBDESRT / CMBMOUNT / CMBTUNDR, the C variants on Myrror) and the wall-rise animation, drawing the
   battlefield's isometric diamond (22 screen rows of 11 tiles read diagonally from the 21 x 22 grid) and stepping the terrain / figure animation counters, allocating the combat blocks, building the
   missile list of a ranged attack, and loading a unit's eight figure facings into the EMS figure cache.
   This module's data (DS:7188): its literals only. With -d the compiler merges duplicate strings, so
   "S" and "" are no strings of their own: they point into the tail of "CMBGRASS".
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS f900 src/wizards/BFIELD.C */
#include <stdlib.h>
#include <string.h>
#include "STRUCTS.H"
#include "ENUMS.H"

extern int wall_set_index;                      /* DS:706e  city-wall picture set (12 pictures per set) */
extern int battlefield_anim_toggle;                      /* DS:7070 */
extern int combat_anim_frame4;                   /* DS:7074 */
extern int battlefield_edge_anim_frame;                  /* DS:7076 */
extern int battlefield_tile_anim_frame;                  /* DS:7078 */
extern int battlefield_anim_frame8;                      /* DS:707e */
extern int combat_anim_counter;                 /* DS:7090 */
extern int combat_pingpong_dir;               /* DS:7092 */
extern int combat_pingpong_frame;                   /* DS:7094 */
extern int battlefield_figure_anim_frame;                /* DS:7096 */
extern int combat_attack_anim_state;            /* DS:7098 */
extern int combat_defend_anim_state;            /* DS:709a */
extern unsigned ems_frame_seg;                  /* DS:760c */
extern struct Battlefield far *p_battlefield;   /* DS:9274 */
extern unsigned combat_terrain_pics[48];        /* DS:939a */
extern unsigned combat_border_pic;              /* DS:973c */
extern unsigned p_scratch;                      /* DS:998e  segment of the scratch buffer */
extern unsigned combat_sprite_pics[COMBAT_SPRITE_SLOTS];         /* DS:c550 */
extern unsigned wall_rise_pics[14];           /* DS:cfa2 */
extern unsigned battlefield_decor_b_pics[5];             /* DS:cfda */
extern unsigned battlefield_decor_a_pics[5];             /* DS:cfe4 */
extern unsigned battlefield_volcano_pics[];              /* DS:d076 */
extern int *p_combat_path_y;                    /* DS:d13e */
extern int *p_combat_path_x;                    /* DS:d140 */
extern int *p_combat_path_prev;                  /* DS:d144 */
extern unsigned char *p_combat_path_dist;       /* DS:d146 */
extern unsigned char *p_combat_move_cost;       /* DS:d148 */
extern struct CombatVortex far *p_vortices;     /* DS:d14e */
extern struct CombatMissile far *p_missile_list; /* DS:d154 */
extern int combat_missile_count;                /* DS:d158 */
extern struct CombatSprite far *p_sprite_list;  /* DS:d15a */
extern int wall_rise_kind;               /* DS:d27e */

void Tiles_EMS_Map(void);
void Figure_EMS_Map_Slot(int slot);
void Figure_Render_Frame(int slot, int facing, int owner, int aura, int frame);
void Figure_Formation_Offset(int figures, int figure, int *dx, int *dy);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void far *Seg_To_Far_Ptr(unsigned seg);
char *Near_Alloc(unsigned bytes);
char *Near_Alloc_Reset(unsigned bytes);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Set_Draw_Page(void);
void Reset_Clip_Rect(void);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Draw_Pic_Frame(int x, int y, unsigned pic);
void Screen_Save_A800(void);
void Combat_Grid_To_Screen(int gx, int gy, int ox, int oy, int *sx, int *sy);
int Atan2_Deg(int dx, int dy);

/* (bf_terrain, plane): the combat terrain tiles of a battlefield kind (BF_TERRAIN_*, BFGEN.C: 1 / 2 / 6 grass, 3 desert,
   4 mountain, 5 tundra; 0 ocean: none) - 48 tiles + 5 + 5 decorations from CMBxxxxx.LBX (the Myrror set on plane 1). The result is meaningless
   except for kind 0 (no return statement at the end) */
int Load_Combat_Terrain_Tiles(int bf_terrain, int plane)
{
    unsigned seg;
    char lbx_name[30];
    int i;

    Tiles_EMS_Map();
    if (bf_terrain == BF_TERRAIN_OCEAN)
        return 0;
    if (bf_terrain == BF_TERRAIN_GRASSLAND || bf_terrain == BF_TERRAIN_HILLS || bf_terrain == BF_TERRAIN_FOREST)
        strcpy(lbx_name, plane == PLANE_ARCANUS ? "CMBGRASS" : "CMBGRASC");
    else if (bf_terrain == BF_TERRAIN_DESERT)
        strcpy(lbx_name, plane == PLANE_ARCANUS ? "CMBDESRT" : "CMBDESRC");
    else if (bf_terrain == BF_TERRAIN_MOUNTAIN)
        strcpy(lbx_name, plane == PLANE_ARCANUS ? "CMBMOUNT" : "CMBMOUNC");
    else if (bf_terrain == BF_TERRAIN_TUNDRA)
        strcpy(lbx_name, plane == PLANE_ARCANUS ? "CMBTUNDR" : "CMBTUNDC");
    seg = Heap_Alloc_Front(ems_frame_seg, 1);   /* never used */
    for (i = 0; i < 48; i++)
        combat_terrain_pics[i] = LBX_Reload_Next(lbx_name, i, ems_frame_seg);
    for (i = 0; i < 5; i++)
        battlefield_decor_a_pics[i] = LBX_Reload_Next(lbx_name, i + 48, ems_frame_seg);
    for (i = 0; i < 5; i++)
        battlefield_decor_b_pics[i] = LBX_Reload_Next(lbx_name, i + 53, ems_frame_seg);
}

/* draws the isometric combat terrain: 22 screen rows of 11 tiles, tile[row][col] read along a diagonal of the
   21 x 22 grid (col = i / 2 + j, row = (i + 1) / 2 + 9 - j) (a tile >= 56 is drawn as one of the 4 plain tiles, >= 48 not at all),
   the volcano of a Chaos Node battlefield and the border panel, stepping the terrain / figure animation counters on
   the way (before and after the volcano; COMBDRAW steps the vortex, effect, cursor and water ones) */
void Battlefield_Draw_Terrain(void)
{
    int i;
    int j;
    int col0;
    int row0;
    int x;
    int y;
    int col;
    int row;
    int pic;

    Set_Draw_Page();
    Reset_Clip_Rect();
    Tiles_EMS_Map();
    for (i = 0; i < 22; i++) {
        col0 = i / 2;
        row0 = (i + 1) / 2 + 9;
        for (j = 0; j < 11; j++) {
            col = col0 + j;
            row = row0 - j;
            x = j * 32 - (i & 1) * 16;
            y = i * 8 - 8;
            pic = p_battlefield->tile[row][col];
            if (pic >= 56)                      /* 56 and up: one of the four plain tiles by position */
                pic = (col + row) & 3;
            if (pic < 48)
                Draw_Pic_Clipped(x, y, combat_terrain_pics[pic]);
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
    if (p_battlefield->backdrop == BACKDROP_CHAOS_NODE) {
        Combat_Grid_To_Screen(BF_FEATURE_X, BF_FEATURE_Y, 0, 0, &x, &y);
        x -= 46;
        y -= 13;
        Draw_Pic_Frame(x, y, battlefield_volcano_pics[8]);
    }
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
    if (combat_attack_anim_state == 1)          /* the attack animation's two frames alternate */
        combat_attack_anim_state = 3;
    else
        combat_attack_anim_state = 1;
    if (combat_defend_anim_state == 1)
        combat_defend_anim_state = 3;
    else
        combat_defend_anim_state = 1;
    Draw_Pic_Frame(0, 164, combat_border_pic);
    Screen_Save_A800();
}

/* (wall_kind): the wall-rise animation pictures - 18 sprite buffers in the scratch block, then WALLRISE.LBX (in the
   1.31 data its entries 0 .. 35 are empty: only FIREWALL 36.. and DARKWALL 50.. hold pictures - RUNTIME_CHECKS RC2): kind 0 the
   12 stone pictures of the current wall set, 1 the 14 fire pictures (36..), 2 the 14 darkness pictures (50..) */
void Load_Wall_Rise_Anim(int wall_kind)
{
    int i;

    wall_rise_kind = wall_kind;
    Heap_Mark(p_scratch);
    for (i = 0; i < COMBAT_SPRITE_SLOTS; i++)
        combat_sprite_pics[i] = Heap_Alloc_Back(p_scratch, 55);
    switch (wall_kind) {
    case WALL_RISE_STONE:
        for (i = 0; i < 12; i++)
            wall_rise_pics[i] = LBX_Reload_Next("WALLRISE", wall_set_index * 12 + i, p_scratch);
        break;
    case WALL_RISE_FIRE:
        for (i = 0; i < 14; i++)
            wall_rise_pics[i] = LBX_Reload_Next("WALLRISE", i + 36, p_scratch);
        break;
    case WALL_RISE_DARKNESS:
        for (i = 0; i < 14; i++)
            wall_rise_pics[i] = LBX_Reload_Next("WALLRISE", i + 50, p_scratch);
        break;
    }
    Heap_Release(p_scratch);
}

/* allocates the combat blocks: the battlefield (0x15c paragraphs) in front of the scratch block, the sprite list,
   missile list and vortex table behind it, and five near buffers (move costs, path distances, work, path x / y) */
void Battlefield_Build(void)
{
    p_battlefield = Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, sizeof(struct Battlefield) / 16));
    p_sprite_list = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 0x1e2));
    p_missile_list = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 10));
    p_vortices = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 9));
    p_combat_move_cost = (unsigned char *)Near_Alloc_Reset(504);
    p_combat_path_dist = (unsigned char *)Near_Alloc(504);
    p_combat_path_prev = (int *)Near_Alloc(1008);
    p_combat_path_x = (int *)Near_Alloc(60);
    p_combat_path_y = (int *)Near_Alloc(60);
}

/* (figures, target_figures, from x, y, to x, y, kind): does not draw - one p_missile_list entry per figure: start /
   end screen position from the two squares and the figure formation offsets, the missile kind, progress 10 and the
   direction 0..7 from Atan2_Deg; sets combat_missile_count */
void Combat_Missile_List_Build(int figures, int target_figures, int from_x, int from_y, int to_x, int to_y,
                               int missile_kind)
{
    int dir;
    int deg;
    int from_dx;
    int from_dy;
    int to_dx;
    int to_dy;
    int x1;
    int y1;
    int x2;
    int y2;
    int x0;
    int y0;
    int figure;

    for (figure = 0; figure < figures; figure++) {
        Figure_Formation_Offset(figures, figure, &from_dx, &from_dy);
        Figure_Formation_Offset(figures, figure % target_figures, &to_dx, &to_dy);
        Combat_Grid_To_Screen(from_x, from_y, 0, 0, &x1, &y1);
        Combat_Grid_To_Screen(to_x, to_y, 0, 0, &x2, &y2);
        from_dy -= 8;
        to_dy -= 8;
        x0 = x1;
        y0 = y1 + 8;
        x1 += from_dx;
        y1 += from_dy;
        x2 += to_dx;
        y2 += to_dy;
        deg = Atan2_Deg(x2 - x0, y2 - y0);
        if (deg < 22 || deg > 337)
            dir = 2;
        else if (deg < 69)
            dir = 3;
        else if (deg < 112)
            dir = 4;
        else if (deg < 158)
            dir = 5;
        else if (deg < 202)
            dir = 6;
        else if (deg < 248)
            dir = 7;
        else if (deg < 292)
            dir = 0;
        else
            dir = 1;
        p_missile_list[figure].x1 = x1;
        p_missile_list[figure].y1 = y1;
        p_missile_list[figure].x2 = x2;
        p_missile_list[figure].y2 = y2;
        p_missile_list[figure].kind = missile_kind;
        p_missile_list[figure].progress = 10;
        p_missile_list[figure].direction = dir;
    }
    combat_missile_count = figures;
}

/* (unit_type, slot): loads unit type's 8 facings from FIGUREnn.LBX (FIGURES1..9, FIGURE10..; 15 types per file, 8
   entries per type) into the EMS figure cache slot (two slots per 16 KB page: odd slots at +0x200 paragraphs, and every slot
   + slot x 56 paragraphs),
   renders frame 0, returns the slot */
int Load_Unit_Figure_Frames(int unit_type, int slot)
{
    int j;
    unsigned front;
    int entry;
    int slot_ofs;
    unsigned far *frames;
    char num_text[6];
    char lbx_name[20];

    Figure_EMS_Map_Slot(slot);
    if ((slot & 1) == 0)
        slot_ofs = 0;
    else
        slot_ofs = 0x200;
    slot_ofs += slot * 56;
    front = Heap_Alloc_Front(ems_frame_seg + slot_ofs, 2);
    frames = Seg_To_Far_Ptr(front);
    itoa(unit_type / 15 + 1, num_text, 10);
    strcpy(lbx_name, "FIGURE");
    if (unit_type / 15 + 1 < 10)
        strcat(lbx_name, "S");
    strcat(lbx_name, num_text);
    strcat(lbx_name, "");
    entry = unit_type % 15 * 8;
    for (j = 0; j < 8; j++)
        frames[j] = LBX_Reload_Next(lbx_name, entry + j, ems_frame_seg + slot_ofs);
    Figure_Render_Frame(slot, 0, 0, 0, 0);
    return slot;
}
