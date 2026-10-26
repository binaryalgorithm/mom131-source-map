/* AIUNITS.C - WIZARDS.EXE overlay code segment ebda (stub 266f): the AI's per-turn unit orders - per plane and
   landmass the stacks, the attack pools and the rally army, garrisons, settlers, purifiers, engineers, magic spirits,
   ships and units at sea, plane shifting, disbanding for upkeep, the landmass targets and the landmass replanning,
   and the single-unit order helpers (move, settle, road, seek transport, meld, purify, go overseas).
   Data DS:7116..7117 (ai_wants_war); BSS DS:d3ec..d493.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS ebda src/wizards/AIUNITS.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void AI_Unit_Dispatch(int player);
void AI_Release_Goto_Near_Weak_City(int plane, int player);
void AI_Spirits_To_Nodes(int player);
int AI_Plane_Shift_Stacks(int player);
void AI_Boats_Leave_Port(void);
void AI_Disband_For_Upkeep(int player);
void AI_Build_Stacks(int player, int landmass, int plane);
void AI_Eject_Noncombatants(int plane);
void AI_Build_Attack_Pool(void);
int AI_Settler_Orders(int player, int landmass);
int AI_Purify_Orders(int landmass, int plane);
void AI_Engineer_Orders(int landmass);
void AI_Collect_Landmass_Targets(int player, int landmass, int plane);
void AI_Field_Stack_Targets(int landmass, int plane, int player);
void AI_Flyers_To_Target_Landmass(int player, int plane);
void AI_Rally_Embark(int landmass, int plane, int player);
int AI_Send_Pool_To_Rally(int landmass, int plane, int player);
void AI_Send_Stacks_To_Garrisons(int player, int plane, int landmass);
int AI_Units_At_Sea_Orders(int player, int plane);
void AI_Ship_Orders(int player, int plane);
void AI_Replan_Landmass(int player, int landmass, int plane);
int AI_Order_Move(int unit, int x, int y, int stack, int slot);
int AI_Foreign_Unit_Near_On_Landmass(int x, int y, int plane, int radius, int owner, int landmass);
int AI_Order_Seek_Transport(int unit, int stack, int slot);
int AI_Stack_Pick_Target(int x, int y, int *tx, int *ty, int stack, int k);
void AI_Stack_Embark(int stack, int landmass, int plane, int rally_x, int rally_y, int room, int player);
int AI_Add_Transport_Request(int x, int y, int plane);
void AI_Add_Landmass_Target(int x, int y, int defence, int target_value);
int AI_Tower_Other_Side_Worth(int player, int x, int y, int plane);
int AI_Nearest_Enemy_Field_Stack(int stack, int landmass, int *x, int *y, int plane);
void AI_Pool_Add_Strongest(int stack, int size, int take);
int AI_Order_Meld(int unit, int stack, int slot);
int AI_Foreign_Unit_Near(int x, int y, int plane, int radius, int owner);
int AI_Order_Settle(int unit, int stack, int slot);
int AI_Settler_Tower_Target(int player, int unit, int *out_x, int *out_y, int plane);
int AI_Unit_Go_Overseas(int unit, int x, int y, int plane, int player, int stack, int slot);
int AI_Order_Purify(int unit, int stack, int slot);
int AI_Order_Build_Road(int unit, int dest_x, int dest_y, int stack, int slot);

#include "GAME.H"

int ai_wants_war = 0;                           /* DS:7116  1 when the AI is hostile to the human player (no peace); never read in either program */

/* BSS DS:d3ec..d493, highest address first */
int ai_bss_unused;                              /* DS:d492  never referenced; fills the gap before HELPSCR.C's BSS */
int ai_field_unused;                            /* DS:d490 */
int ai_rally_target_size;                       /* DS:d48e  the rally army size: game_turn / 30 + 2, at most 9 */
int ai_rally_inbound_count;                           /* DS:d48c */
int ai_rally_present_count;                           /* DS:d48a */
int ai_pool_units[MAX_STACK];                           /* DS:d478 */
int ai_flyer_pool_units[MAX_STACK];                          /* DS:d466 */
int ai_pool_slot[MAX_STACK];                            /* DS:d454 */
int ai_flyer_pool_slot[MAX_STACK];                           /* DS:d442 */
int ai_pool_stack[MAX_STACK];                           /* DS:d430 */
int ai_flyer_pool_stack[MAX_STACK];                          /* DS:d41e */
int ai_pool_strength[MAX_STACK];                        /* DS:d40c */
int ai_flyer_pool_strength[MAX_STACK];                       /* DS:d3fa */
int ai_pool_count;                              /* DS:d3f8 */
int ai_flyer_pool_count;                             /* DS:d3f6 */
int ai_pool_min_strength;                       /* DS:d3f4 */
int ai_flyer_pool_min_strength;                      /* DS:d3f2 */
char *ai_lm_rally_x_row;                        /* DS:d3f0  ai_landmass_plan[plane][ai].rally_x */
char *ai_lm_rally_y_row;                        /* DS:d3ee  ai_landmass_plan[plane][ai].rally_y */
char *ai_lm_status_row;                         /* DS:d3ec  ai_landmass_plan[plane][ai].status */

extern struct DifficultyInfo difficulty_table[];  /* DS:1fea */
extern struct LandmassPlan ai_landmass_plan[2][5];  /* DS:8890 */
extern int *p_ai_expand_y[2];                   /* DS:8f9a */
extern int *p_ai_expand_x[2];                   /* DS:8f9e */
extern int *p_ai_expand_landmass[2];            /* DS:8fac */
extern int *p_ai_attack_landmass[2];            /* DS:8fb0 */
extern int ai_target_value[25];                 /* DS:8fc8 */
extern int ai_target_defence[25];               /* DS:8ffa */
extern int ai_target_y[25];                     /* DS:902c */
extern int ai_target_x[25];                     /* DS:905e */
extern int ai_target_count;                     /* DS:9092 */
extern int ai_transport_req_count;              /* DS:9096 */
extern int ai_transport_req_plane[AI_MAX_TRANSPORT_REQS];          /* DS:9098 */
extern int ai_transport_req_y[AI_MAX_TRANSPORT_REQS];              /* DS:90b6 */
extern int ai_transport_req_x[AI_MAX_TRANSPORT_REQS];              /* DS:90d4 */
extern int *ai_lm_surplus[2];                   /* DS:90f2 */
extern struct AIStack *p_ai_own_stacks;             /* DS:9108 */
extern int ai_own_stack_count;                  /* DS:910c */
extern unsigned far *p_threat_maps[2];          /* DS:9c94 */
extern unsigned char (far *p_terrain_flags_maps)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cb4 */
extern struct Lair far *p_lairs;                /* DS:9cc0 */
extern struct Tower far *p_towers;              /* DS:9cc4 */
extern struct Node far *p_nodes;                /* DS:9ccc */
extern unsigned char (far *p_landmass_ids)[MAP_HEIGHT][MAP_WIDTH];  /* DS:9cd0 */
extern int far *p_coast_tile_head[2];           /* DS:9d2a */
extern int far *p_coast_tile_next[2];           /* DS:9d32 */
extern char far *p_coast_tile_y[2];             /* DS:9d3a */
extern char far *p_coast_tile_x[2];             /* DS:9d42 */
extern int ai_square_stack_count;                      /* DS:9d4a */
extern int far *p_ai_square_stack_units[AI_MAX_STACKS];           /* DS:9d4c */
#define AI_SLOT_ORDERED (-1)   /* p_ai_square_stack_units: a unit already given its orders */
/* a -1 slot: a unit already ordered this turn (AI_Build_Stacks enters moving units as -1, AI_Order_* clear the
   slot), not a free place */
extern char far *p_ai_square_stack_size;               /* DS:9e8c */
extern char far *p_ai_square_stack_class;              /* DS:9e90 */
extern char far *p_ai_square_stack_plane;              /* DS:9e94 */
extern char far *p_ai_square_stack_y;                  /* DS:9e98 */
extern char far *p_ai_square_stack_x;                  /* DS:9e9c */
extern int far *p_landmass_tile_head[2];        /* DS:9ea0 */
extern int far *p_landmass_tile_next[2];        /* DS:9ea8 */
extern char far *p_landmass_tile_y[2];          /* DS:9eb0 */
extern char far *p_landmass_tile_x[2];          /* DS:9eb8 */
extern int game_turn;                           /* DS:bd90 */
extern int difficulty;                          /* DS:bd96 */
extern char path_cost[];                        /* DS:c5f0 */
extern char path_y[];                           /* DS:c668 */
extern char path_x[];                           /* DS:c6e0 */

void EMS_Map_Default(void);
void AI_Map_EMS_Tables(void);
int Random(int n);                             /* -> 1 .. n */
int Map_Distance(int x1, int y1, int x2, int y2, int width);
int Unit_Can_Fly(int unit);
int Unit_Can_Cross_Water(int unit);
int Unit_Is_Non_Corporeal(int unit);
void Unit_Kill_Or_Dismiss(int unit, int mode);
int Tile_Is_Shore_Water(int x, int y, int plane);
int AI_Unit_Strength(int unit);
void Wizard_Income_Summary(int player, int *gold, int *food, int *mana);
int Stack_Can_Plane_Shift(int x, int y, int plane);
int Tile_Is_Sea(int x, int y, int plane);
void AI_Order_Transports_Goto(int stack, int tx, int ty);
int AI_Adjacent_Tile_Pick(int x, int y, int plane, int *fx, int *fy);
void AI_Nearest_Coast_Tile(int x, int y, int plane, int *cx, int *cy);
int AI_Adjacent_Free_Land(int x, int y, int plane, int *fx, int *fy);
int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
void Swap_Words(int *a, int *b);
int Tile_Settle_Blocker(int x, int y, int plane);
int Bit_Test(int bit, char far *map);
int Dist_Max_Plus_Half_Min(int x, int y, int tx, int ty);
void City_Site_Evaluate(int x, int y, int plane, int *max_pop, int *prod_pct, int *gold_pct, int *unit_cost_pct,
                        int *mineral_gold, int *mineral_power, int *nightshade, int *mithril, int *adamantium,
                        int *coastal, int *unexplored);
int Path_Find(int cavalry, int forester, int mountaineer, int swimming, int sailing, int flying, int x1, int y1,
              int x2, int y2, int plane, unsigned char *path_x, unsigned char *path_y, unsigned char *path_cost,
              int window, int mode, int needs_transport, int stack_size, int player);

/* (player) the AI's unit orders for the turn: war mode (hostility or diplomacy status >= 3 towards the human player and
   no peace), the rally army size, disbanding, plane shifting, boats leaving port and goto releases (bug: passed the
   plane before it is set); then per plane and landmass 1..59 the stacks, the attack pool, spirits, settlers,
   purifiers, engineers, the landmass targets, the field stacks, flyers and the rally army by the landmass status,
   the garrisons; per plane the units at sea and the ships
   Numbered: BUG #147 (BUGS_AND_QUIRKS.txt) */
void AI_Unit_Dispatch(int player)
{
    int plane;
    int landmass;

    ai_wants_war = 0;
    if ((wizards[player].hostility[PLAYER_HUMAN] >= HOSTILITY_WAR || wizards[player].diplomacy_status[PLAYER_HUMAN] >= TREATY_WAR) && wizards[player].peace_turns[PLAYER_HUMAN] == 0)
        ai_wants_war = 1;
    AI_Map_EMS_Tables();
    ai_transport_req_count = 0;
    ai_rally_target_size = game_turn / 30 + 2;
    if (ai_rally_target_size > MAX_STACK)
        ai_rally_target_size = MAX_STACK;
    AI_Disband_For_Upkeep(player);
    AI_Plane_Shift_Stacks(player);
    AI_Boats_Leave_Port();
    AI_Release_Goto_Near_Weak_City(plane, player);
    for (plane = 0; plane < NUM_PLANES; plane++) {
        ai_lm_rally_x_row = ai_landmass_plan[plane][player].rally_x;
        ai_lm_rally_y_row = ai_landmass_plan[plane][player].rally_y;
        ai_lm_status_row = ai_landmass_plan[plane][player].status;
        for (landmass = 1; landmass < MAX_LANDMASSES; landmass++) {
            AI_Build_Stacks(player, landmass, plane);
            AI_Eject_Noncombatants(plane);
            AI_Build_Attack_Pool();
            AI_Spirits_To_Nodes(player);
            AI_Settler_Orders(player, landmass);
            AI_Purify_Orders(landmass, plane);
            AI_Engineer_Orders(landmass);
            AI_Collect_Landmass_Targets(player, landmass, plane);
            AI_Field_Stack_Targets(landmass, plane, player);
            if (ai_lm_status_row[landmass] >= LANDMASS_STAGING || ai_lm_status_row[landmass] == LANDMASS_DOMINANT ||
                ai_lm_status_row[landmass] == LANDMASS_IGNORE || ai_lm_status_row[landmass] == LANDMASS_NO_CITY)
                AI_Flyers_To_Target_Landmass(player, plane);
            if (ai_lm_status_row[landmass] >= LANDMASS_STAGING || ai_lm_status_row[landmass] == LANDMASS_DOMINANT)
                AI_Rally_Embark(landmass, plane, player);
            AI_Send_Pool_To_Rally(landmass, plane, player);
            if (ai_lm_status_row[landmass] == LANDMASS_DOMINANT || ai_lm_status_row[landmass] == LANDMASS_CONTESTED || ai_lm_status_row[landmass] >= LANDMASS_STAGING)
                AI_Send_Stacks_To_Garrisons(player, plane, landmass);
        }
        AI_Units_At_Sea_Orders(player, plane);
        AI_Ship_Orders(player, plane);
    }
    EMS_Map_Default();
}

/* (landmass, plane, player) the attack pool joins the landmass's rally point while the rally army (present
   + inbound) is below ai_rally_target_size - up to 9 - inbound units are sent, so the army can exceed it - only when the landmass status is 3 or >= 5 or the pool can fill it; a full rally
   army (9) replans the landmass 1 time in 20. No value at the end */
int AI_Send_Pool_To_Rally(int landmass, int plane, int player)
{
    int max_send;
    int slot;
    int unit;
    int stack;
    int i;

    if (ai_rally_present_count == MAX_STACK && Random(20) == 1)
        AI_Replan_Landmass(player, landmass, plane);
    if (ai_rally_present_count + ai_rally_inbound_count >= ai_rally_target_size)
        return 0;
    if (ai_landmass_plan[plane][player].status[landmass] < LANDMASS_STAGING && ai_landmass_plan[plane][player].status[landmass] != LANDMASS_NO_CITY &&
        ai_rally_present_count + ai_rally_inbound_count + ai_pool_count < ai_rally_target_size)
        return 0;
    max_send = MAX_STACK - ai_rally_inbound_count;
    for (i = 0; i < max_send && i < ai_pool_count; i++) {
        stack = ai_pool_stack[i];
        slot = ai_pool_slot[i];
        unit = ai_pool_units[i];
        AI_Order_Move(unit, ai_lm_rally_x_row[landmass], ai_lm_rally_y_row[landmass], stack, slot);
    }
}

/* (player, plane, landmass) garrisons: each own city on the landmass needs 9 units at the fortress city, else
   population / 3 + 2 (/ 4 + 2 for Dwarven / Troll / Draconian cities or when the landmass status is 1 or >= 5), at
   most 9, less the stack already there; an unguarded node (threat exactly 0x8000) needs 8 (status 1 / >= 5) or 4
   less its stack, 8 with none. Lone and field stacks (class 0 / 1) away from the rally point send units (not
   engineers / road builders, not settlers / meld units) to the city site with the least distance - need; node sites
   only when no city site has a need. Quirk: the
   fortress city index starts as the PLAYER number, so with no fortress city city #player counts as one; the stack
   size is read twice */
void AI_Send_Stacks_To_Garrisons(int player, int plane, int landmass)
{
    int slot;
    int stack_size;
    int unit_type;
    int i;
    int cx;
    int cy;
    int nx;
    int ny;
    char city_sites;
    char node_sites;
    int best_dist;
    int dist;
    int dest_x;
    int dest_y;
    int x1;
    int y1;
    int dest_need;
    int dest_is_node;
    int unit;
    int found;
    int secure_landmass;
    int fortress_city;
    int hardy_race;
    char city_site[100];
    char city_need[100];
    char node_site[30];
    char node_need[30];
    register int j;
    register int best;

    if (ai_landmass_plan[plane][player].status[landmass] >= LANDMASS_STAGING || ai_landmass_plan[plane][player].status[landmass] == LANDMASS_DOMINANT)
        secure_landmass = 1;
    else
        secure_landmass = 0;
    fortress_city = player;
    for (i = 0; i < num_cities; i++)
        if (p_cities[i].x == p_fortresses[player].x && p_cities[i].y == p_fortresses[player].y &&
            p_cities[i].plane == p_fortresses[player].plane)
            fortress_city = i;
    city_sites = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player && p_cities[i].plane == plane) {
            cx = p_cities[i].x;
            cy = p_cities[i].y;
            hardy_race = 0;
            if (p_cities[i].race == RACE_DWARVEN || p_cities[i].race == RACE_TROLL || p_cities[i].race == RACE_DRACONIAN)
                hardy_race = 1;
            if (p_landmass_ids[plane][cy][cx] == landmass) {
                found = 0;
                if (i == fortress_city)
                    city_need[city_sites] = MAX_STACK;
                else if (secure_landmass == 0 && hardy_race == 0)
                    city_need[city_sites] = p_cities[i].population / 3 + 2;
                else
                    city_need[city_sites] = p_cities[i].population / 4 + 2;
                if (city_need[city_sites] > MAX_STACK)
                    city_need[city_sites] = MAX_STACK;
                for (j = 0; j < ai_square_stack_count && found == 0; j++) {
                    if (p_ai_square_stack_x[j] == cx && p_ai_square_stack_y[j] == cy) {
                        city_need[city_sites] -= p_ai_square_stack_size[j];
                        city_site[city_sites] = i;
                        if (city_need[city_sites] > 0)
                            city_sites = city_sites + 1;
                        found = 1;
                    }
                }
                if (found == 0) {
                    city_site[city_sites] = i;
                    city_sites = city_sites + 1;
                }
            }
        }
    }
    node_sites = 0;
    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].plane == plane) {
            nx = p_nodes[i].x;
            ny = p_nodes[i].y;
            if (p_threat_maps[plane][ny * MAP_WIDTH + nx] == THREAT_SITE) {
                nx = p_nodes[i].x;
                ny = p_nodes[i].y;
                if (p_landmass_ids[plane][ny][nx] == landmass) {
                    found = 0;
                    for (j = 0; j < ai_square_stack_count && found == 0; j++) {
                        if (p_ai_square_stack_x[j] == nx && p_ai_square_stack_y[j] == ny) {
                            if (secure_landmass == 1)
                                node_need[node_sites] = 8 - p_ai_square_stack_size[j];
                            else
                                node_need[node_sites] = 4 - p_ai_square_stack_size[j];
                            node_site[node_sites] = i;
                            if (node_need[node_sites] > 0)
                                node_sites = node_sites + 1;
                            found = 1;
                        }
                    }
                    if (found == 0) {
                        node_need[node_sites] = 8;
                        node_site[node_sites] = i;
                        node_sites = node_sites + 1;
                    }
                }
            }
        }
    }
    for (j = 0; j < ai_square_stack_count; j++) {
        if ((p_ai_square_stack_class[j] == STACK_CLASS_LONE || p_ai_square_stack_class[j] == STACK_CLASS_FIELD) &&
            (p_ai_square_stack_x[j] != ai_lm_rally_x_row[landmass] || p_ai_square_stack_y[j] != ai_lm_rally_y_row[landmass])) {
            stack_size = p_ai_square_stack_size[j];
            x1 = p_ai_square_stack_x[j];
            y1 = p_ai_square_stack_y[j];
            stack_size = p_ai_square_stack_size[j];
            best_dist = 1000;
            for (i = 0; i < city_sites; i++) {
                if (city_need[i] > 0) {
                    dist = Map_Distance(x1, y1, p_cities[city_site[i]].x, p_cities[city_site[i]].y, MAP_WIDTH) - city_need[i];
                    if (dist < best_dist) {
                        best_dist = dist;
                        dest_x = p_cities[city_site[i]].x;
                        dest_y = p_cities[city_site[i]].y;
                        dest_need = city_need[i];
                        dest_is_node = 0;
                        best = i;
                    }
                }
            }
            if (best_dist == 1000) {
                for (i = 0; i < node_sites; i++) {
                    if (node_need[i] > 0) {
                        dist = Map_Distance(x1, y1, p_nodes[node_site[i]].x, p_nodes[node_site[i]].y, MAP_WIDTH) - node_need[i];
                        if (dist < best_dist) {
                            best_dist = dist;
                            dest_x = p_nodes[node_site[i]].x;
                            dest_y = p_nodes[node_site[i]].y;
                            dest_need = node_need[i];
                            dest_is_node = 1;
                            best = i;
                        }
                    }
                }
            }
            if (best_dist < 1000) {
                for (slot = 0; slot < stack_size && slot < dest_need; slot++) {
                    unit = p_ai_square_stack_units[j][slot];
                    if (unit != AI_SLOT_ORDERED) {
                        unit_type = p_units[unit].type;
                        if (unit_types[unit_type].construction == 0 && (unit_types[unit_type].ability_flags & (AFLAG_CREATE_OUTPOST | AFLAG_MELD_WITH_NODE)) == 0) {
                            AI_Order_Move(unit, dest_x, dest_y, j, slot);
                            if (dest_is_node == 0)
                                city_need[best] = city_need[best] - 1;
                            else
                                node_need[best] = node_need[best] - 1;
                        }
                    }
                }
            }
        }
    }
}

/* (player, plane) units at sea: with an attack landmass, its rally point (else the rally point of the status 2 / 3
   landmass nearest the fortress) - the player's idle units standing on the ocean (landmass 0; not meld units or
   ships) that fly, cross water or are non-corporeal go there; then every ocean stack with no ordered unit (-1 slot) and no such
   unit is disbanded (Unit_Kill_Or_Dismiss) - stranded units are lost. Returns 0 without an attack landmass, else no
   value
   Numbered: BUG #310, #321 (BUGS_AND_QUIRKS.txt) */
int AI_Units_At_Sea_Orders(int player, int plane)
{
    int unit;
    int rx;
    int ry;
    int dist;
    int best_dist;
    int fx;
    int fy;
    int has_ordered;
    int can_cross;
    int stack;
    int k;
    int size;
    int sx;
    int sy;
    int i;

    if (p_ai_attack_landmass[plane][player] == 0)
        return 0;
    rx = ai_landmass_plan[plane][player].rally_x[p_ai_attack_landmass[plane][player]];
    ry = ai_landmass_plan[plane][player].rally_y[p_ai_attack_landmass[plane][player]];
    fx = p_fortresses[player].x;
    fy = p_fortresses[player].y;
    if (rx == 0 && ry == 0) {
        best_dist = 1000;
        for (i = 0; i < MAX_LANDMASSES; i++) {
            if (ai_landmass_plan[plane][player].status[i] == LANDMASS_CONTESTED || ai_landmass_plan[plane][player].status[i] == LANDMASS_NO_CITY) {
                dist = Map_Distance(fx, fy, ai_landmass_plan[plane][player].rally_x[i],
                                    ai_landmass_plan[plane][player].rally_y[i], MAP_WIDTH);
                if (dist < best_dist) {
                    best_dist = dist;
                    rx = ai_landmass_plan[plane][player].rally_x[i];
                    ry = ai_landmass_plan[plane][player].rally_y[i];
                }
            }
        }
    }
    if (rx != 0 || ry != 0) {
        for (i = 0; i < num_units; i++) {
            if (p_units[i].owner == player && p_units[i].plane == plane &&
                p_landmass_ids[plane][p_units[i].y][p_units[i].x] == 0 && p_units[i].order == ORDER_READY &&
                (unit_types[p_units[i].type].ability_flags & AFLAG_MELD_WITH_NODE) == 0 &&
                unit_types[p_units[i].type].transport_capacity == 0 &&
                (Unit_Can_Fly(i) || Unit_Can_Cross_Water(i) || Unit_Is_Non_Corporeal(i))) {
                p_units[i].order = ORDER_GOTO;
                p_units[i].dest_x = rx;
                p_units[i].dest_y = ry;
            }
        }
    }
    AI_Build_Stacks(player, 0, plane);
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        has_ordered = 0;
        can_cross = 0;
        size = p_ai_square_stack_size[stack];
        for (k = 0; k < size && has_ordered == 0; k++) {
            unit = p_ai_square_stack_units[stack][k];
            if (unit == AI_SLOT_ORDERED)
                has_ordered = 1;
            else if (Unit_Can_Fly(unit) || Unit_Can_Cross_Water(unit) || Unit_Is_Non_Corporeal(unit))
                can_cross = 1;
        }
        if (has_ordered == 0 && can_cross == 0) {
            sx = p_ai_square_stack_x[stack];
            sy = p_ai_square_stack_y[stack];
            for (i = 0; i < num_units; i++)
                if (p_units[i].x == sx && p_units[i].y == sy && p_units[i].plane == plane && p_units[i].owner == player)
                    Unit_Kill_Or_Dismiss(i, UNIT_REMOVE_KILLED);
        }
    }
}

/* (player, plane) ship orders. Each transport request starts with room 9, less capacity + 1 for every own unit with a
   Go-To to it (not only ships) (bug: the capacity of unit number <request index>, not of the ship); full requests are closed.
   A loaded ship stack that is not moving lands on an adjacent safe land square (zero threat - bug: the threat map
   row is not scaled, dy + sy x 60 + dx + sx): with settlers aboard only onto the expansion landmass, or where no
   foreign unit is within 5 and the landmass status is 3; otherwise next to the attack landmass, or on a landmass
   with no own or allied city - the ships seek transport, the rest move. If it cannot land it sails next to the
   expansion point (settlers) or to the safe coast square of the attack landmass nearest to it. Empty ships sail to
   the nearest open transport request and take room for capacity + 1 per ship
   Numbered: BUG #117, #328, #329 (BUGS_AND_QUIRKS.txt) */
void AI_Ship_Orders(int player, int plane)
{
    int stack;
    int x;                                      /* the best coast square, then the best transport request */
    int y;
    int best_dist;
    int dist;
    int dx;
    int dy;
    int coast_tile;
    int stack_size;
    int unit;
    int has_ship;
    int ships_only;
    int has_settler;
    int unit_type;
    int can_land;
    int sx;
    int sy;
    int dest_x;
    int dest_y;
    int slot;
    int stack_unit;
    int best_req;
    int space;
    int landmass;
    int moving;
    int ally_city;
    int own_city;
    int req_space[AI_MAX_TRANSPORT_REQS];
    int i;

    moving = 0;
    for (i = 0; i < AI_MAX_TRANSPORT_REQS; i++)
        req_space[i] = MAX_STACK;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].order == ORDER_GOTO && p_units[unit].owner == player && p_units[unit].plane == plane) {
            for (i = 0; i < ai_transport_req_count; i++)
                if (p_units[unit].dest_x == ai_transport_req_x[i] && p_units[unit].dest_y == ai_transport_req_y[i] &&
                    ai_transport_req_plane[i] == plane)
                    req_space[i] -= unit_types[p_units[i].type].transport_capacity + 1;
        }
    }
    for (i = 0; i < ai_transport_req_count; i++)
        if (req_space[i] <= 0)
            ai_transport_req_plane[i] = -1;
    AI_Build_Stacks(player, 0, plane);
    AI_Spirits_To_Nodes(player);
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        has_ship = 0;
        ships_only = 1;
        has_settler = 0;
        moving = 0;
        stack_size = p_ai_square_stack_size[stack];
        sx = p_ai_square_stack_x[stack];
        sy = p_ai_square_stack_y[stack];
        for (stack_unit = 0; stack_unit < num_units; stack_unit++) {
            if (p_units[stack_unit].x == sx && p_units[stack_unit].y == sy && p_units[stack_unit].plane == plane) {
                unit_type = p_units[stack_unit].type;
                if (unit_types[unit_type].transport_capacity > 0)
                    has_ship = 1;
                else {
                    ships_only = 0;
                    if (unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST)
                        has_settler = 1;
                }
                if (p_units[stack_unit].order == ORDER_GOTO)
                    moving = 1;
            }
        }
        if (has_ship == 1) {
            if (ships_only == 0) {
                if (moving == 0) {
                    dest_x = 0;
                    dest_y = 0;
                    can_land = 0;
                    for (dy = -1; dy < 2; dy++) {
                        for (dx = -1; dx < 2; dx++) {
                            if (p_landmass_ids[plane][dy + sy][dx + sx] != 0 &&
                                p_threat_maps[plane][dy + sy * MAP_WIDTH + dx + sx] == 0) {
                                landmass = p_landmass_ids[plane][dy + sy][dx + sx];
                                dest_x = dx + sx;
                                dest_y = dy + sy;
                                can_land = 1;
                            }
                        }
                    }
                    if (can_land == 1) {
                        if (has_settler == 1) {
                            for (dy = -1; dy < 2; dy++) {
                                for (dx = -1; dx < 2; dx++) {
                                    if (p_landmass_ids[plane][dy + sy][dx + sx] == p_ai_expand_landmass[plane][player] &&
                                        p_ai_expand_landmass[plane][player] != 0) {
                                        landmass = p_landmass_ids[plane][dy + sy][dx + sx];
                                        dest_x = dx + sx;
                                        dest_y = dy + sy;
                                    }
                                }
                            }
                            if ((AI_Foreign_Unit_Near_On_Landmass(sx, sy, plane, 5, player, landmass) == 0 &&
                                 ai_landmass_plan[plane][player].status[landmass] == LANDMASS_NO_CITY) ||
                                p_ai_expand_landmass[plane][player] == landmass)
                                can_land = 1;
                            else
                                can_land = 0;
                        } else {
                            can_land = 0;
                            for (dy = -1; dy < 2; dy++) {
                                for (dx = -1; dx < 2; dx++) {
                                    if (p_landmass_ids[plane][dy + sy][dx + sx] == p_ai_attack_landmass[plane][player]) {
                                        landmass = p_landmass_ids[plane][dy + sy][dx + sx];
                                        dest_x = dx + sx;
                                        dest_y = dy + sy;
                                        can_land = 1;
                                    }
                                }
                            }
                            if (can_land == 0) {
                                own_city = 0;
                                ally_city = 0;
                                for (i = 0; i < num_cities; i++) {
                                    if (p_cities[i].plane == plane &&
                                        p_landmass_ids[plane][p_cities[i].y][p_cities[i].x] == landmass) {
                                        if (p_cities[i].owner == player)
                                            own_city = 1;
                                        else if (wizards[player].diplomacy_status[p_cities[i].owner] == TREATY_ALLIANCE)
                                            ally_city = 1;
                                    }
                                }
                                if (own_city == 1 || ally_city == 1)
                                    can_land = 0;
                                else
                                    can_land = 1;
                            }
                        }
                    }
                    if (can_land == 1) {
                        for (slot = 0; slot < stack_size; slot++) {
                            stack_unit = p_ai_square_stack_units[stack][slot];
                            unit_type = p_units[stack_unit].type;
                            if (unit_types[unit_type].transport_capacity > 0)
                                AI_Order_Seek_Transport(stack_unit, stack, slot);
                            else
                                AI_Order_Move(stack_unit, dest_x, dest_y, stack, slot);
                        }
                    } else if (has_settler == 1 && p_ai_expand_landmass[plane][player] != 0) {
                        dest_x = 0;
                        dest_y = 0;
                        for (dy = -1; dy < 2; dy++) {
                            for (dx = -1; dx < 2; dx++) {
                                if (p_landmass_ids[plane][dy + p_ai_expand_y[plane][player]][dx + p_ai_expand_x[plane][player]] == 0) {
                                    dest_x = dx + p_ai_expand_x[plane][player];
                                    dest_y = dy + p_ai_expand_y[plane][player];
                                    can_land = 1;
                                }
                            }
                        }
                        for (slot = 0; slot < stack_size; slot++) {
                            stack_unit = p_ai_square_stack_units[stack][slot];
                            AI_Order_Move(stack_unit, dest_x, dest_y, stack, slot);
                        }
                    } else if (p_ai_attack_landmass[plane][player] != 0) {
                        dest_x = sx;
                        dest_y = sy;
                        coast_tile = p_coast_tile_head[plane][p_ai_attack_landmass[plane][player]];
                        best_dist = 1000;
                        while (coast_tile != -1) {
                            dist = Map_Distance(p_coast_tile_x[plane][coast_tile], p_coast_tile_y[plane][coast_tile],
                                                dest_x, dest_y, MAP_WIDTH);
                            if (dist < best_dist) {
                                dx = p_coast_tile_x[plane][coast_tile];
                                dy = p_coast_tile_y[plane][coast_tile];
                                if (p_threat_maps[plane][dy * MAP_WIDTH + dx] == 0) {
                                    best_dist = dist;
                                    x = dx;
                                    y = dy;
                                }
                            }
                            coast_tile = p_coast_tile_next[plane][coast_tile];
                        }
                        if (best_dist < 1000) {
                            for (dy = -1; dy < 2; dy++) {
                                for (dx = -1; dx < 2; dx++) {
                                    if (p_landmass_ids[plane][y + dy][x + dx] == 0) {
                                        dest_x = x + dx;
                                        dest_y = y + dy;
                                    }
                                }
                            }
                            for (slot = 0; slot < stack_size; slot++) {
                                stack_unit = p_ai_square_stack_units[stack][slot];
                                AI_Order_Move(stack_unit, dest_x, dest_y, stack, slot);
                            }
                        }
                    }
                }
            } else {
                best_dist = 10000;
                for (i = 0; i < ai_transport_req_count; i++) {
                    if (ai_transport_req_plane[i] == plane) {
                        dist = Map_Distance(sx, sy, ai_transport_req_x[i], ai_transport_req_y[i], MAP_WIDTH);
                        if (dist < best_dist) {
                            best_dist = dist;
                            x = ai_transport_req_x[i];
                            y = ai_transport_req_y[i];
                            best_req = i;
                        }
                    }
                }
                if (best_dist < 10000) {
                    space = req_space[best_req];
                    for (slot = 0; slot < stack_size && space > 0; slot++) {
                        stack_unit = p_ai_square_stack_units[stack][slot];
                        space -= unit_types[p_units[stack_unit].type].transport_capacity + 1;
                        AI_Order_Move(stack_unit, x, y, stack, slot);
                    }
                    req_space[best_req] = space;
                    if (space <= 0)
                        ai_transport_req_plane[best_req] = -1;
                }
            }
        }
    }
}

/* (landmass, plane, player) field stacks (class 1) with no ordered unit (-1 slot) take a target from AI_Stack_Pick_Target and
   move there. If some field stack has 8+ units, none got a target and the attack landmass is elsewhere, the
   landmass's rally point becomes its safe coast square (zero threat) nearest the centre of the attack landmass's
   coast squares (averaged without the x wrap) and its status 5. The 'cannot cross water' flag is computed but never
   read; ai_field_unused is cleared per stack */
void AI_Field_Stack_Targets(int landmass, int plane, int player)
{
    int i;
    int sx;
    int sy;
    int tx;
    int ty;
    int slot;
    int no_cross;
    int size;
    int unit;
    int has_ordered;
    int cx_sum;
    int cy_sum;
    int tile;
    int stage_x;
    int x;
    int y;
    int none_moved;
    int stage_y;
    int best_dist;
    int count;                                  /* the coast tile count, then a distance */
    int big_stack;

    big_stack = 0;
    none_moved = 1;
    for (i = 0; i < ai_square_stack_count; i++) {
        if (p_ai_square_stack_class[i] == STACK_CLASS_FIELD) {
            size = p_ai_square_stack_size[i];
            sx = p_ai_square_stack_x[i];
            sy = p_ai_square_stack_y[i];
            if (size >= 8)
                big_stack = 1;
            no_cross = 0;
            has_ordered = 0;
            for (slot = 0; slot < size; slot++) {
                unit = p_ai_square_stack_units[i][slot];
                if (unit != AI_SLOT_ORDERED) {
                    if (Unit_Can_Fly(unit) == 0 && Unit_Can_Cross_Water(unit) == 0)
                        no_cross = 1;
                } else
                    has_ordered = 1;
            }
            if (has_ordered == 0 && AI_Stack_Pick_Target(sx, sy, &tx, &ty, i, slot) == 1) {
                none_moved = 0;
                for (slot = 0; slot < size; slot++) {
                    unit = p_ai_square_stack_units[i][slot];
                    AI_Order_Move(unit, tx, ty, i, slot);
                }
            }
            ai_field_unused = 0;
        }
    }
    if (big_stack == 1 && none_moved == 1 && p_ai_attack_landmass[plane][player] != 0 &&
        p_ai_attack_landmass[plane][player] != landmass) {
        cx_sum = 0;
        cy_sum = 0;
        count = 0;
        for (tile = p_coast_tile_head[plane][p_ai_attack_landmass[plane][player]]; tile != -1;
             tile = p_coast_tile_next[plane][tile]) {
            cx_sum += p_coast_tile_x[plane][tile];
            cy_sum += p_coast_tile_y[plane][tile];
            count++;
        }
        cx_sum = cx_sum / count;                /* from here cx_sum / cy_sum hold the centre; no test of count 0 (an
                                                   attack landmass with no coast square: a divide error); clarified
                                                   using ReMoM repo */
        cy_sum = cy_sum / count;
        best_dist = 1000;
        for (tile = p_coast_tile_head[plane][landmass]; tile != -1; tile = p_coast_tile_next[plane][tile]) {
            x = p_coast_tile_x[plane][tile];
            y = p_coast_tile_y[plane][tile];
            if (p_threat_maps[plane][y * MAP_WIDTH + x] == 0) {
                count = Map_Distance(cx_sum, cy_sum, x, y, MAP_WIDTH);
                if (count < best_dist) {
                    best_dist = count;
                    stage_x = x;
                    stage_y = y;
                }
            }
        }
        if (best_dist != 1000) {
            ai_landmass_plan[plane][player].status[landmass] = LANDMASS_STAGING;
            ai_landmass_plan[plane][player].rally_x[landmass] = stage_x;
            ai_landmass_plan[plane][player].rally_y[landmass] = stage_y;
        }
    }
}

/* (landmass, plane, player) the room at the landmass's rally point (9 - the size of the stack standing there; 0
   when no stack stands there, so nobody embarks);
   every field stack (class 1) with a first unit is offered to AI_Stack_Embark towards the rally point with that
   room, which shrinks by the stack's size */
void AI_Rally_Embark(int landmass, int plane, int player)
{
    int sx;
    int sy;
    int rally_x;
    int rally_y;
    int stack;
    int room;

    rally_x = ai_landmass_plan[plane][player].rally_x[landmass];
    rally_y = ai_landmass_plan[plane][player].rally_y[landmass];
    room = 0;
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        sx = p_ai_square_stack_x[stack];
        sy = p_ai_square_stack_y[stack];
        if (sx == rally_x && sy == rally_y)
            room = MAX_STACK - p_ai_square_stack_size[stack];
    }
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        if (p_ai_square_stack_class[stack] == STACK_CLASS_FIELD && p_ai_square_stack_units[stack][0] != AI_SLOT_ORDERED) {
            AI_Stack_Embark(stack, landmass, plane, rally_x, rally_y, room, player);
            room -= p_ai_square_stack_size[stack];
        }
    }
}

/* (player, plane) with an attack landmass: every flying or water-crossing unit (not a meld unit) of the lone and
   field stacks (class 0 / 1) on another landmass moves to the attack landmass's rally point (the -1 slots - units already
   ordered - are passed to the checks unchecked) */
void AI_Flyers_To_Target_Landmass(int player, int plane)
{
    int slot;
    int unit;
    int size;
    int sx;
    int sy;
    int stack;

    if (p_ai_attack_landmass[plane][player] != 0) {
        for (stack = 0; stack < ai_square_stack_count; stack++) {
            if (p_ai_square_stack_class[stack] == STACK_CLASS_FIELD || p_ai_square_stack_class[stack] == STACK_CLASS_LONE) {
                size = p_ai_square_stack_size[stack];
                sx = p_ai_square_stack_x[stack];
                sy = p_ai_square_stack_y[stack];
                if (p_landmass_ids[plane][sy][sx] != p_ai_attack_landmass[plane][player]) {
                    for (slot = 0; slot < size; slot++) {
                        unit = p_ai_square_stack_units[stack][slot];
                        if ((Unit_Can_Fly(unit) == 1 || Unit_Can_Cross_Water(unit) == 1) &&
                            (unit_types[p_units[unit].type].ability_flags & AFLAG_MELD_WITH_NODE) == 0)
                            AI_Order_Move(unit, ai_landmass_plan[plane][player].rally_x[p_ai_attack_landmass[plane][player]],
                                          ai_landmass_plan[plane][player].rally_y[p_ai_attack_landmass[plane][player]],
                                          stack, slot);
                    }
                }
            }
        }
    }
}

/* (stack, landmass, plane, rally_x, rally_y, room, player) a field stack heading overseas: a transport request at
   the last ocean square next to the rally point (no x wrap); away from the rally point up to 'room' units move
   there; the shore squares around the stack are looked up and never used; own units on ocean squares in the 3 x 3
   around the stack count as boats - ships are held there (ORDER_GOTO, destination their own square) and add their
   capacity, the others take room - and the stack's units board them (move onto the last boat's square) while room
   and capacity last; with no boat the units seek transport - but the capacity is 0 then, so nothing happens. The
   landmass parameter is copied and never read
   Numbered: BUG #146, #321 (BUGS_AND_QUIRKS.txt) */
void AI_Stack_Embark(int stack, int landmass, int plane, int rally_x, int rally_y, int room, int player)
{
    int k;
    int x;                                      /* the first loop's dx offset, then a wrapped x, then a unit x */
    int shore_found;
    int boat_found;
    int unit;
    int dy;
    int dx;
    int boat_x;
    int boat_y;
    int lm;
    int shore_x;
    int shore_y;
    int capacity_left;
    int tile_room;
    int sx;
    int sy;
    int size;
    int stack_unit;
    int req_x;
    int req_y;
    int x_lo;
    int x_hi;
    int y_lo;
    int y_hi;
    int unit_type;

    lm = landmass;
    sx = p_ai_square_stack_x[stack];
    sy = p_ai_square_stack_y[stack];
    req_x = sx;
    req_y = sy;
    for (dy = -1; dy < 2; dy++) {
        for (x = -1; x < 2; x++) {
            if (p_landmass_ids[plane][dy + rally_y][x + rally_x] == 0) {
                req_x = x + rally_x;
                req_y = dy + rally_y;
            }
        }
    }
    AI_Add_Transport_Request(req_x, req_y, plane);
    if (sx != rally_x || sy != rally_y) {
        size = p_ai_square_stack_size[stack];
        for (k = 0; k < size && room > 0; k++) {
            stack_unit = p_ai_square_stack_units[stack][k];
            AI_Order_Move(stack_unit, rally_x, rally_y, stack, k);
            room--;
        }
    }
    for (dy = sy - 1; dy < sy + 2; dy++) {
        if (dy > 0 && dy < MAP_HEIGHT) {
            for (dx = sx - 1; dx < sx + 2; dx++) {
                x = dx;
                if (x < 0)
                    x += MAP_WIDTH;
                if (x >= MAP_WIDTH)
                    x -= MAP_WIDTH;
                if (Tile_Is_Shore_Water(x, dy, plane)) {
                    shore_found = 1;
                    shore_x = dx;
                    shore_y = dy;
                }
            }
        }
    }
    boat_found = 0;
    x_lo = sx - 1;
    y_lo = sy - 1;
    x_hi = sx + 1;
    y_hi = sy + 1;
    capacity_left = 0;
    tile_room = MAX_STACK;
    for (unit = 0; unit < num_units; unit++) {
        if (p_units[unit].owner == player) {
            x = p_units[unit].x;
            dy = p_units[unit].y;               /* dy: here the unit's y */
            if (x >= x_lo && dy >= y_lo && x <= x_hi && dy <= y_hi && p_landmass_ids[plane][dy][x] == 0) {
                boat_found = 1;
                boat_x = p_units[unit].x;
                boat_y = p_units[unit].y;
                unit_type = p_units[unit].type;
                if (unit_types[unit_type].transport_capacity > 0) {
                    p_units[unit].dest_x = p_units[unit].x;
                    p_units[unit].dest_y = p_units[unit].y;
                    p_units[unit].order = ORDER_GOTO;
                    capacity_left += unit_types[unit_type].transport_capacity;
                } else if (Unit_Can_Fly(unit) == 0 && Unit_Can_Cross_Water(unit) == 0)
                    capacity_left--;
                tile_room--;
            }
        }
    }
    if (boat_found == 1) {
        for (k = 0; k < p_ai_square_stack_size[stack] && k < tile_room && capacity_left > 0; k++) {
            stack_unit = p_ai_square_stack_units[stack][k];
            AI_Order_Move(stack_unit, boat_x, boat_y, stack, k);
            if (Unit_Can_Fly(stack_unit) == 0 && Unit_Can_Cross_Water(stack_unit) == 0)
                capacity_left--;
        }
    } else {
        for (k = 0; k < p_ai_square_stack_size[stack] && k < tile_room && capacity_left > 0; k++) {
            stack_unit = p_ai_square_stack_units[stack][k];
            AI_Order_Seek_Transport(stack_unit, stack, k);
        }
    }
}

/* (player, landmass, plane) the landmass's target list (AI_Add_Landmass_Target, at most 25), defence = the threat
   & 0x3fff: wizard fortresses at hostility >= HOSTILITY_WAR (500, x 10 at diplomacy_status TREATY_WAR - == only, so not at TREATY_FINAL_WAR, here and for cities; clarified
   using ReMoM repo; the human's defence
   x 5 / 4 - bug: the defence is read BEFORE the fortress x / y are loaded, so from the previous fortress or
   uninitialised); cities - neutral ((population + buildings) x 10 + 100), other wizards' at hostility >= HOSTILITY_WAR (the same,
   x 10 at TREATY_WAR; the half value for HOSTILITY_HOSTILE is unreachable) - bug: the x 5 / 4 defence goes to CITY #0, not to
   the human's cities; foreign armies (threat without the 0x4000 / 0x8000 flags: value = threat on the fortress's
   landmass, else threat / 3); nodes not owned (unguarded ones skipped: power x 5 + 25) or owned by a wizard at
   hostility >= HOSTILITY_HOSTILE (power x 10 + 50); active lairs (50); on the fortress's plane towers worth crossing
   (AI_Tower_Other_Side_Worth: own 1, unowned 150, at hostility >= HOSTILITY_WAR 10). No target: LANDMASS_NO_TARGETS
   Numbered: BUG #118, #119, #320 (BUGS_AND_QUIRKS.txt) */
void AI_Collect_Landmass_Targets(int player, int landmass, int plane)
{
    int x;
    int y;
    int owner;
    int tile;
    int threat;
    int tx;
    int ty;
    int nx;
    int ny;
    int twx;
    int twy;
    int cx;
    int cy;
    int lx;
    int ly;
    int war_mult;
    int defence;
    int i;

    ai_target_count = 0;
    for (i = 0; i < num_players; i++) {
        if (p_fortresses[i].plane == plane) {
            if (wizards[player].diplomacy_status[i] == TREATY_WAR)
                war_mult = 10;
            else
                war_mult = 1;
            if (i == PLAYER_HUMAN)
                defence = (p_threat_maps[plane][y * MAP_WIDTH + x] & THREAT_STRENGTH_MASK) * 5 / 4;
            else
                defence = p_threat_maps[plane][y * MAP_WIDTH + x] & THREAT_STRENGTH_MASK;
            x = p_fortresses[i].x;
            y = p_fortresses[i].y;
            if (p_landmass_ids[plane][y][x] == landmass && wizards[player].hostility[i] >= HOSTILITY_WAR)
                AI_Add_Landmass_Target(x, y, defence, war_mult * 500);
        }
    }
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].plane == plane) {
            owner = p_cities[i].owner;
            cx = p_cities[i].x;
            cy = p_cities[i].y;
            if (owner != PLAYER_NEUTRAL && wizards[player].diplomacy_status[owner] == TREATY_WAR)
                war_mult = 10;
            else
                war_mult = 1;
            if (i == 0)
                defence = (p_threat_maps[plane][cy * MAP_WIDTH + cx] & THREAT_STRENGTH_MASK) * 5 / 4;
            else
                defence = p_threat_maps[plane][cy * MAP_WIDTH + cx] & THREAT_STRENGTH_MASK;
            if (owner == PLAYER_NEUTRAL) {
                if (p_landmass_ids[plane][cy][cx] == landmass)
                    AI_Add_Landmass_Target(cx, cy, defence,
                                           ((p_cities[i].population + p_cities[i].building_count) * 10 + 100) * war_mult);
            } else if (owner != player && wizards[player].hostility[owner] >= HOSTILITY_WAR) {
                cx = p_cities[i].x;
                cy = p_cities[i].y;
                if (p_landmass_ids[plane][cy][cx] == landmass) {
                    if (wizards[player].hostility[owner] >= HOSTILITY_WAR)
                        AI_Add_Landmass_Target(cx, cy, defence,
                                               ((p_cities[i].population + p_cities[i].building_count) * 10 + 100) * war_mult);
                    else if (wizards[player].hostility[owner] >= HOSTILITY_HOSTILE)
                        AI_Add_Landmass_Target(cx, cy, defence,
                                               (owner != PLAYER_HUMAN ? (p_cities[i].population + p_cities[i].building_count) * 5 + 50
                                                           : (p_cities[i].population + p_cities[i].building_count) * 10 + 100) * war_mult);
                }
            }
        }
    }
    for (tile = p_landmass_tile_head[plane][landmass]; tile != -1; tile = p_landmass_tile_next[plane][tile]) {
        tx = p_landmass_tile_x[plane][tile];
        ty = p_landmass_tile_y[plane][tile];
        threat = p_threat_maps[plane][ty * MAP_WIDTH + tx];
        if (threat != 0 && (threat & THREAT_AT_PEACE) == 0 && (threat & THREAT_SITE) == 0 && (threat & THREAT_STRENGTH_MASK) != 0)
            AI_Add_Landmass_Target(tx, ty, threat & THREAT_STRENGTH_MASK,
                                   p_fortresses[player].plane == plane &&
                                   p_landmass_ids[plane][p_fortresses[player].y][p_fortresses[player].x] == landmass
                                       ? threat & THREAT_STRENGTH_MASK : (threat & THREAT_STRENGTH_MASK) / 3);
    }
    for (i = 0; i < NUM_NODES; i++) {
        if (p_nodes[i].plane == plane) {
            owner = p_nodes[i].owner;
            if (owner != player && (wizards[player].hostility[owner] >= HOSTILITY_HOSTILE || owner == PLAYER_NONE)) {
                nx = p_nodes[i].x;
                ny = p_nodes[i].y;
                if (p_landmass_ids[plane][ny][nx] == landmass) {
                    if (owner != PLAYER_NONE)
                        AI_Add_Landmass_Target(nx, ny, p_threat_maps[plane][ny * MAP_WIDTH + nx] & THREAT_STRENGTH_MASK,
                                               p_nodes[i].power * 10 + 50);
                    else if (p_threat_maps[plane][ny * MAP_WIDTH + nx] & THREAT_STRENGTH_MASK)
                        AI_Add_Landmass_Target(nx, ny, p_threat_maps[plane][ny * MAP_WIDTH + nx] & THREAT_STRENGTH_MASK,
                                               p_nodes[i].power * 5 + 25);
                }
            }
        }
    }
    for (i = 0; i < NUM_LAIRS; i++) {
        if (p_lairs[i].plane == plane && p_lairs[i].active == 1) {
            lx = p_lairs[i].x;
            ly = p_lairs[i].y;
            if (p_landmass_ids[plane][ly][lx] == landmass)
                AI_Add_Landmass_Target(lx, ly, p_threat_maps[plane][ly * MAP_WIDTH + lx] & THREAT_STRENGTH_MASK, 50);
        }
    }
    if (p_fortresses[player].plane == plane) {
        for (i = 0; i < NUM_TOWERS; i++) {
            owner = p_towers[i].owner;
            twx = p_towers[i].x;
            twy = p_towers[i].y;
            if (p_landmass_ids[plane][twy][twx] == landmass && AI_Tower_Other_Side_Worth(player, twx, twy, plane) != 0) {
                if (owner != player) {
                    if (wizards[player].hostility[owner] >= HOSTILITY_HOSTILE || owner == PLAYER_NONE) {
                        if (owner > -1 && wizards[player].hostility[owner] >= HOSTILITY_WAR)
                            AI_Add_Landmass_Target(twx, twy, p_threat_maps[plane][twy * MAP_WIDTH + twx] & THREAT_STRENGTH_MASK, 10);
                        else if (owner == PLAYER_NONE)
                            AI_Add_Landmass_Target(twx, twy, p_threat_maps[plane][twy * MAP_WIDTH + twx] & THREAT_STRENGTH_MASK, 150);
                    }
                } else if (p_landmass_ids[plane][twy][twx] == landmass)
                    AI_Add_Landmass_Target(twx, twy, 0, 1);
            }
        }
    }
    if (ai_target_count == 0)
        ai_landmass_plan[plane][player].status[landmass] = LANDMASS_NO_TARGETS;
}

/* (x, y, defence, target_value) appends a landmass target (at most AI_MAX_TARGETS) */
void AI_Add_Landmass_Target(int x, int y, int defence, int target_value)
{
    if (ai_target_count < AI_MAX_TARGETS) {
        ai_target_x[ai_target_count] = x;
        ai_target_y[ai_target_count] = y;
        ai_target_defence[ai_target_count] = defence;
        ai_target_value[ai_target_count] = target_value;
        ai_target_count++;
    }
}

/* (x, y, *tx, *ty, stack, k) the stack's target: strength = the sum of AI_Unit_Strength / 10 over its units; the
   best target with value > 0 and defence x 3 / 4 < strength by value / (4 x distance) (at least 1); the path is
   checked with Path_Find only when the stack's LAST unit (it also gives the plane and owner) has goto_blocked set;
   the chosen target's value is zeroed (claimed). The 6th parameter is only the loop counter (callers pass a slot) */
int AI_Stack_Pick_Target(int x, int y, int *tx, int *ty, int stack, int k)
{
    int size;
    int unit;
    int strength;
    int best_score;
    int best;
    int score;
    int dist;
    int unit_strength;
    int plane;
    int player;
    int goto_blocked;
    int i;

    size = p_ai_square_stack_size[stack];
    strength = 0;
    for (k = 0; k < size; k++) {
        unit = p_ai_square_stack_units[stack][k];
        if (unit != AI_SLOT_ORDERED) {
            plane = p_units[unit].plane;
            player = p_units[unit].owner;
            goto_blocked = p_units[unit].goto_blocked;
            unit_strength = AI_Unit_Strength(unit) / 10;
            strength += unit_strength;
        }
    }
    best_score = 0;
    best = -1;
    for (i = 0; i < ai_target_count; i++) {
        if (ai_target_value[i] > 0 && ai_target_defence[i] * 3 / 4 < strength) {
            dist = Map_Distance(ai_target_x[i], ai_target_y[i], x, y, MAP_WIDTH);
            if (dist < 1)
                dist = 1;
            score = ai_target_value[i] / (dist * 4);
            if (score < 1)
                score = 1;
            if (score > best_score) {
                if (goto_blocked == 0 ||
                    Path_Find(0, 0, 0, 0, 0, 0, x, y, ai_target_x[i], ai_target_y[i], plane, path_x, path_y,
                              path_cost, 1, 4, size, size, player) > 0) {   /* 1, 4: window / mode, overwritten in Path_Find, never read;
                                                                              size as needs_transport, never read either */
                    best_score = score;
                    best = i;
                }
                AI_Map_EMS_Tables();
            }
        }
    }
    if (best == -1)
        return 0;
    *tx = ai_target_x[best];
    *ty = ai_target_y[best];
    ai_target_value[best] = 0;
    return 1;
}

/* (player, landmass, plane) groups the player's units on the landmass into at most 80 square stacks: units moving,
   (ORDER_GOTO / _PURIFY / _BUILD_ROAD) fill their slot with -1 and count towards the rally army when heading for the rally point
   (ai_rally_inbound_count); units standing on it count in ai_rally_present_count. Class: STACK_CLASS_FIELD open field (zero threat),
   _FORTRESS the fortress square, _SITE other sites. A field stack hunting (first unit ORDER_GOTO_ENEMY) is sent at the nearest enemy army
   on the landmass or released (ORDER_READY, STACK_CLASS_LONE); other field stacks of at most 1 unit become lone.
   BUG #425: a released stack's -1 slots (units already ordered) write p_units[-1].order (AI_Order_Move guards -1)
   Numbered: BUG #319 (BUGS_AND_QUIRKS.txt) */
void AI_Build_Stacks(int player, int landmass, int plane)
{
    int i;
    int j;
    int ux;
    int uy;
    int up;
    int found;
    int tx;
    int ty;
    int first;

    ai_square_stack_count = 0;
    ai_rally_inbound_count = 0;
    ai_rally_present_count = 0;
    for (i = 0; i < AI_MAX_STACKS; i++) {
        p_ai_square_stack_x[i] = -1;
        p_ai_square_stack_y[i] = -1;
        p_ai_square_stack_plane[i] = -1;
        p_ai_square_stack_size[i] = 0;
    }
    for (i = 0; i < num_units; i++) {
        if (ai_square_stack_count < AI_MAX_STACKS && p_units[i].owner == player) {
            up = p_units[i].plane;
            if (up == plane) {
                ux = p_units[i].x;
                uy = p_units[i].y;
                if (ai_lm_rally_x_row[landmass] == ux && ai_lm_rally_y_row[landmass] == uy)
                    ai_rally_present_count++;
                if (p_landmass_ids[up][uy][ux] == landmass) {
                    found = -1;
                    for (j = 0; j < ai_square_stack_count; j++)
                        if (p_ai_square_stack_x[j] == ux && p_ai_square_stack_y[j] == uy && p_ai_square_stack_plane[j] == up)
                            found = j;
                    if (found == -1) {
                        p_ai_square_stack_x[ai_square_stack_count] = ux;
                        p_ai_square_stack_y[ai_square_stack_count] = uy;
                        p_ai_square_stack_plane[ai_square_stack_count] = up;
                        p_ai_square_stack_size[ai_square_stack_count] = 1;
                        if (p_units[i].order == ORDER_GOTO || p_units[i].order == ORDER_PURIFY || p_units[i].order == ORDER_BUILD_ROAD) {
                            p_ai_square_stack_units[ai_square_stack_count][0] = AI_SLOT_ORDERED;
                            if (p_units[i].dest_x == ai_lm_rally_x_row[landmass] &&
                                p_units[i].dest_y == ai_lm_rally_y_row[landmass])
                                ai_rally_inbound_count++;
                        } else
                            p_ai_square_stack_units[ai_square_stack_count][0] = i;
                        if (p_threat_maps[up][uy * MAP_WIDTH + ux] != 0) {
                            if (p_fortresses[player].x == ux && p_fortresses[player].y == uy &&
                                p_fortresses[player].plane == up)
                                p_ai_square_stack_class[ai_square_stack_count] = STACK_CLASS_FORTRESS;
                            else
                                p_ai_square_stack_class[ai_square_stack_count] = STACK_CLASS_SITE;
                        } else
                            p_ai_square_stack_class[ai_square_stack_count] = STACK_CLASS_FIELD;
                        ai_square_stack_count++;
                    } else {
                        if (p_units[i].order == ORDER_GOTO || p_units[i].order == ORDER_PURIFY || p_units[i].order == ORDER_BUILD_ROAD) {
                            p_ai_square_stack_units[found][p_ai_square_stack_size[found]] = -1;
                            if (p_units[i].dest_x == ai_lm_rally_x_row[landmass] &&
                                p_units[i].dest_y == ai_lm_rally_y_row[landmass])
                                ai_rally_inbound_count++;
                        } else
                            p_ai_square_stack_units[found][p_ai_square_stack_size[found]] = i;
                        p_ai_square_stack_size[found] = p_ai_square_stack_size[found] + 1;
                    }
                }
            }
        }
    }
    for (i = 0; i < ai_square_stack_count; i++) {
        if (p_ai_square_stack_class[i] == STACK_CLASS_FIELD) {
            first = p_ai_square_stack_units[i][0];
            if (first != -1 && p_units[first].order == ORDER_GOTO_ENEMY) {
                if (AI_Nearest_Enemy_Field_Stack(i, landmass, &tx, &ty, plane) == 1) {
                    for (j = 0; j < p_ai_square_stack_size[i]; j++)
                        AI_Order_Move(p_ai_square_stack_units[i][j], tx, ty, i, j);
                } else {
                    for (j = 0; j < p_ai_square_stack_size[i]; j++)
                        p_units[p_ai_square_stack_units[i][j]].order = ORDER_READY;
                    p_ai_square_stack_class[i] = STACK_CLASS_LONE;
                }
            } else if (p_ai_square_stack_size[i] + 1 < 3)
                p_ai_square_stack_class[i] = STACK_CLASS_LONE;
        }
    }
}

/* (stack, landmass, *x, *y, plane) the landmass square nearest the stack holding a foreign army in the open
   (threat & 0x3fff without the 0x4000 / 0x8000 flags); 0 if none */
int AI_Nearest_Enemy_Field_Stack(int stack, int landmass, int *x, int *y, int plane)
{
    int tile;
    int threat;
    int dist;
    int best_dist;
    int best_x;
    int best_y;
    int x1;
    int y1;
    int x2;
    int y2;

    best_dist = 1000;
    x1 = p_ai_square_stack_x[stack];
    y1 = p_ai_square_stack_y[stack];
    for (tile = p_landmass_tile_head[plane][landmass]; tile != -1; tile = p_landmass_tile_next[plane][tile]) {
        x2 = p_landmass_tile_x[plane][tile];
        y2 = p_landmass_tile_y[plane][tile];
        threat = p_threat_maps[plane][y2 * MAP_WIDTH + x2];
        if ((threat & THREAT_STRENGTH_MASK) != 0 && (threat & THREAT_AT_PEACE) == 0 && (threat & THREAT_SITE) == 0) {
            dist = Map_Distance(x1, y1, x2, y2, MAP_WIDTH);
            if (dist < best_dist) {
                best_dist = dist;
                best_x = x2;
                best_y = y2;
            }
        }
    }
    if (best_dist < 1000) {
        *x = best_x;
        *y = best_y;
        return 1;
    }
    return 0;
}

/* (plane, player) own stacks on the move (ORDER_GOTO) next to (3 x 3, no wrap) a foreign city on the plane holding
   fewer than 4 units stop: all units on the stack's square are released (ORDER_READY) and so is the stack. Called
   with an uninitialised plane (see AI_Unit_Dispatch) */
void AI_Release_Goto_Near_Weak_City(int plane, int player)
{
    int i;
    int stack;
    int unit;
    int x;
    int y;
    int sp;
    int found;
    int x_lo;
    int y_lo;
    int x_hi;
    int y_hi;
    int cx;
    int cy;
    int cp;
    int count;

    for (stack = 0; stack < ai_own_stack_count; stack++) {
        if (p_ai_own_stacks[stack].status == ORDER_GOTO) {
            x = p_ai_own_stacks[stack].x;
            y = p_ai_own_stacks[stack].y;
            sp = p_ai_own_stacks[stack].plane;
            x_lo = x - 1;
            y_lo = y - 1;
            x_hi = x + 1;
            y_hi = y + 1;
            found = 0;
            for (i = 0; i < num_cities && found == 0; i++) {
                if (p_cities[i].x >= x_lo && p_cities[i].x <= x_hi && p_cities[i].y >= y_lo && p_cities[i].y <= y_hi &&
                    p_cities[i].plane == plane && p_cities[i].owner != player) {
                    count = 0;
                    cx = p_cities[i].x;
                    cy = p_cities[i].y;
                    cp = p_cities[i].plane;
                    for (unit = 0; unit < num_units; unit++)
                        if (p_units[unit].x == cx && p_units[unit].y == cy && p_units[unit].plane == cp)
                            count++;
                    if (count < 4)
                        found = 1;
                }
            }
            if (found == 1) {
                for (i = 0; i < num_units; i++)
                    if (p_units[i].x == x && p_units[i].y == y && p_units[i].plane == sp)
                        p_units[i].order = ORDER_READY;
                p_ai_own_stacks[stack].status = ORDER_READY;
            }
        }
    }
}

/* (player) disbanding for upkeep, from Wizard_Income_Summary: while gold or food per turn is short (at most 200
   passes) the weakest normal unit standing on land dies (AI_Unit_Strength / 10, doubled on a landmass whose surplus
   is 0 - so those are kept; heroes included), each saving 1 food and its upkeep x the difficulty's upkeep % gold
   (at least 1); then while mana is short the same for fantastic units - but ranked by the strength of unit number
   <type> (bug). Dead units are moved to plane 0
   Numbered: BUG #148 (BUGS_AND_QUIRKS.txt) */
void AI_Disband_For_Upkeep(int player)
{
    int i;
    int gold;
    int food;
    int mana;
    int pass;
    int best;
    int score;
    int unit;
    int food_deficit;
    int gold_deficit;
    int mana_deficit;
    int up;
    int lm;
    int saved;
    int unused;

    Wizard_Income_Summary(player, &gold, &food, &mana);
    mana_deficit = 0;
    food_deficit = 0;
    gold_deficit = 0;
    unused = 0;
    if (food < 0)
        food_deficit += -food;
    if (gold < 0)
        gold_deficit += -gold;
    if (mana < 0)
        mana_deficit += -mana;
    for (pass = 0; (food_deficit > 0 || gold_deficit > 0) && pass < 200; pass++) {
        best = 10000;
        unit = -1;
        for (i = 0; i < num_units; i++) {
            if (p_units[i].type < UNIT_MAGIC_SPIRIT && p_units[i].owner == player) {
                up = p_units[i].plane;
                lm = p_landmass_ids[up][p_units[i].y][p_units[i].x];
                if (lm > 0) {
                    score = AI_Unit_Strength(i) / 10;
                    if (ai_lm_surplus[up][lm] == 0)
                        score = score * 2;
                    if (score < best) {
                        best = score;
                        unit = i;
                    }
                }
            }
        }
        if (unit != -1) {
            food_deficit--;
            saved = unit_types[p_units[unit].type].upkeep * difficulty_table[difficulty].upkeep_pct / 100;
            if (saved == 0)
                saved = 1;
            gold_deficit -= saved;
            Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
            p_units[unit].plane = 0;
        }
    }
    for (pass = 0; mana_deficit > 0 && pass < 200; pass++) {
        best = 10000;
        unit = -1;
        for (i = 0; i < num_units; i++) {
            if (p_units[i].type >= UNIT_MAGIC_SPIRIT && p_units[i].owner == player) {
                up = p_units[i].plane;
                lm = p_landmass_ids[up][p_units[i].y][p_units[i].x];
                if (lm > 0) {
                    score = AI_Unit_Strength(p_units[i].type) / 10;
                    if (ai_lm_surplus[up][lm] == 0)
                        score = score * 2;
                    if (score < best) {
                        best = score;
                        unit = i;
                    }
                }
            }
        }
        if (unit != -1) {
            saved = unit_types[p_units[unit].type].upkeep * difficulty_table[difficulty].upkeep_pct / 100;
            if (saved <= 0)
                saved = 1;
            mana_deficit -= saved;
            Unit_Kill_Or_Dismiss(unit, UNIT_REMOVE_KILLED);
            p_units[unit].plane = 0;
        }
    }
}

/* (player) unless some wizard has Planar Seal, own stacks ON the fortress's plane that can plane shift
   (Stack_Can_Plane_Shift) and stand on a tower or on an own Astral Gate city move to the other plane with
   every unit at that x / y (any owner, and on either plane: the plane is not tested). Returns 0 under Planar Seal, else no value */
int AI_Plane_Shift_Stacks(int player)
{
    int i;
    int stack;
    int tower;
    int other_plane;
    int x;
    int y;
    int sp;
    int city;

    for (i = 0; i < num_players; i++)
        if (wizards[i].globals.planar_seal)
            return 0;
    other_plane = 1 - p_fortresses[player].plane;
    for (stack = 0; stack < ai_own_stack_count; stack++) {
        sp = p_ai_own_stacks[stack].plane;
        if (sp != other_plane) {
            x = p_ai_own_stacks[stack].x;
            y = p_ai_own_stacks[stack].y;
            if (Stack_Can_Plane_Shift(x, y, sp) == 1) {
                for (tower = 0; tower < NUM_TOWERS; tower++) {
                    if (p_towers[tower].x == x && p_towers[tower].y == y) {
                        for (i = 0; i < num_units; i++)
                            if (p_units[i].x == x && p_units[i].y == y)
                                p_units[i].plane = other_plane;
                        p_ai_own_stacks[stack].plane = other_plane;
                    }
                }
                for (city = 0; city < num_cities; city++) {
                    if (p_cities[city].owner == player && p_cities[city].enchants[CITY_ENCH_ASTRAL_GATE] &&
                        p_cities[city].x == x && p_cities[city].y == y) {
                        for (i = 0; i < num_units; i++)
                            if (p_units[i].x == x && p_units[i].y == y)
                                p_units[i].plane = other_plane;
                        p_ai_own_stacks[stack].plane = other_plane;
                    }
                }
            }
        }
    }
}

/* own transport stacks (AISTACK_TRANSPORT) standing on land - in a port city - sail out to the last adjacent sea square
   (not ones already ordered: strength AISTACK_ORDERED) */
void AI_Boats_Leave_Port(void)
{
    int stack;
    int dx;
    int x;
    int y;
    int plane;
    int lm;
    int dy;
    int tx;
    int ty;
    int found;

    for (stack = 0; stack < ai_own_stack_count; stack++) {
        if (p_ai_own_stacks[stack].strength != AISTACK_ORDERED && (p_ai_own_stacks[stack].flags & AISTACK_TRANSPORT)) {
            x = p_ai_own_stacks[stack].x;
            y = p_ai_own_stacks[stack].y;
            plane = p_ai_own_stacks[stack].plane;
            lm = p_landmass_ids[plane][y][x];
            tx = 0;
            ty = 0;
            found = 0;
            if (lm != 0) {
                for (dy = -1; dy < 2; dy++) {
                    for (dx = -1; dx < 2; dx++) {
                        if ((dy != 0 || dx != 0) && dx + x < MAP_WIDTH && dy + y < MAP_HEIGHT && dx + x >= 0 && dy + y >= 0 &&
                            Tile_Is_Sea(dx + x, dy + y, plane) == 1) {
                            tx = x + dx;
                            ty = y + dy;
                            found = 1;
                        }
                    }
                }
                if (found == 1)
                    AI_Order_Transports_Goto(stack, tx, ty);
            }
        }
    }
}

/* (plane) non-combatants step out of the sites (stack class >= 3) onto a free adjacent square
   (AI_Adjacent_Tile_Pick): settlers and road builders always, meld units unless the site is a node */
void AI_Eject_Noncombatants(int plane)
{
    int stack;
    int slot;
    int x;
    int y;
    int size;
    int unit_type;
    int unit;
    int ux;
    int uy;
    int node;
    int at_node;

    for (stack = 0; stack < ai_square_stack_count; stack++) {
        if (p_ai_square_stack_class[stack] >= STACK_CLASS_SITE) {
            size = p_ai_square_stack_size[stack];
            for (slot = 0; slot < size; slot++) {
                unit = p_ai_square_stack_units[stack][slot];
                if (unit != AI_SLOT_ORDERED) {
                    unit_type = p_units[unit].type;
                    if ((unit_types[unit_type].ability_flags & AFLAG_CREATE_OUTPOST) || unit_types[unit_type].construction > 0) {
                        if (AI_Adjacent_Tile_Pick(p_ai_square_stack_x[stack], p_ai_square_stack_y[stack], p_ai_square_stack_plane[stack],
                                                  &x, &y) == 1)
                            AI_Order_Move(unit, x, y, stack, slot);
                    } else if (unit_types[unit_type].ability_flags & AFLAG_MELD_WITH_NODE) {
                        ux = p_units[unit].x;
                        uy = p_units[unit].y;
                        at_node = 0;
                        for (node = 0; node < NUM_NODES && at_node == 0; node++)
                            if (p_nodes[node].x == ux && p_nodes[node].y == uy && p_nodes[node].plane == plane)
                                at_node = 1;
                        if (at_node == 0 &&
                            AI_Adjacent_Tile_Pick(p_ai_square_stack_x[stack], p_ai_square_stack_y[stack], p_ai_square_stack_plane[stack],
                                                  &x, &y) == 1)
                            AI_Order_Move(unit, x, y, stack, slot);
                    }
                }
            }
        }
    }
}

/* the attack pools from this landmass's stacks: the fortress gives all but 5 units before turn 100, other sites
   all but 5, lone stacks and field stacks with a first unit all of theirs (AI_Pool_Add_Strongest) */
void AI_Build_Attack_Pool(void)
{
    int stack;
    int count;

    ai_pool_count = 0;
    ai_flyer_pool_count = 0;
    ai_pool_min_strength = 10000;
    ai_flyer_pool_min_strength = 10000;
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        count = p_ai_square_stack_size[stack];
        if (p_ai_square_stack_class[stack] == STACK_CLASS_FORTRESS) {
            if (game_turn < 100 && count > 5)
                AI_Pool_Add_Strongest(stack, count, count - 5);
        } else if (p_ai_square_stack_class[stack] == STACK_CLASS_SITE) {
            if (count > 5)
                AI_Pool_Add_Strongest(stack, count, count - 5);
        } else if (p_ai_square_stack_class[stack] == STACK_CLASS_LONE || (p_ai_square_stack_class[stack] == STACK_CLASS_FIELD && p_ai_square_stack_units[stack][0] != AI_SLOT_ORDERED))
            AI_Pool_Add_Strongest(stack, count, count);
    }
}

/* (stack, size, take) the stack's combat units (no road builders, settlers, meld units or ships) ranked by
   AI_Unit_Strength / 10 of unit number <type> (bug), the best 'take' join the 9-unit attack pool (the weakest
   replaced when full) and, if they fly or cross water, the 9-unit flyer pool (ai_flyer_pool_*; its minimum after an
   append is taken from the unit's real strength); both pools are re-sorted strongest first
   Numbered: BUG #149 (BUGS_AND_QUIRKS.txt) */
void AI_Pool_Add_Strongest(int stack, int size, int take)
{
    int pos;
    int a;
    int unit;
    int slot;
    int ut;
    int flyer;
    int weakest;
    int i;
    int str;
    int cand_count;
    int cand_slot[10];
    int cand_str[10];
    int cand_unit[10];

    cand_count = 0;
    for (slot = 0; slot < size; slot++) {
        unit = p_ai_square_stack_units[stack][slot];
        if (unit != AI_SLOT_ORDERED) {
            ut = p_units[unit].type;
            if (unit_types[ut].construction == 0 && (unit_types[ut].ability_flags & AFLAG_CREATE_OUTPOST) == 0 &&
                (unit_types[ut].ability_flags & AFLAG_MELD_WITH_NODE) == 0 && unit_types[ut].transport_capacity == 0) {
                cand_slot[cand_count] = slot;
                cand_unit[cand_count] = unit;
                cand_str[cand_count] = AI_Unit_Strength(ut) / 10;
                cand_count++;
            }
        }
    }
    for (a = 0; a < cand_count - 1; a++) {
        for (pos = 0; pos < cand_count - 1; pos++) {
            if (cand_str[pos] < cand_str[pos + 1]) {
                Swap_Words(&cand_str[pos], &cand_str[pos + 1]);
                Swap_Words(&cand_slot[pos], &cand_slot[pos + 1]);
                Swap_Words(&cand_unit[pos], &cand_unit[pos + 1]);
            }
        }
    }
    if (take < cand_count)
        cand_count = take;
    for (i = 0; i < cand_count; i++) {
        slot = cand_slot[i];
        unit = cand_unit[i];
        str = cand_str[i];
        if (Unit_Can_Fly(unit) != 0 || Unit_Can_Cross_Water(unit) != 0)
            flyer = 1;
        else
            flyer = 0;
        if (ai_pool_count < MAX_STACK) {
            ai_pool_slot[ai_pool_count] = slot;
            ai_pool_stack[ai_pool_count] = stack;
            ai_pool_strength[ai_pool_count] = str;
            ai_pool_units[ai_pool_count] = unit;
            ai_pool_count++;
            if (str < ai_pool_min_strength)
                ai_pool_min_strength = str;
        } else if (str > ai_pool_min_strength) {
            weakest = 0;
            for (a = 0; a < MAX_STACK; a++)
                if (ai_pool_strength[a] == ai_pool_min_strength)
                    weakest = a;
            ai_pool_slot[weakest] = slot;
            ai_pool_stack[weakest] = stack;
            ai_pool_strength[weakest] = str;
            ai_pool_units[weakest] = unit;
            ai_pool_min_strength = 10000;
            for (a = 0; a < MAX_STACK; a++)
                if (ai_pool_strength[a] < ai_pool_min_strength)
                    ai_pool_min_strength = ai_pool_strength[a];
        }
        if (flyer == 1) {
            if (ai_flyer_pool_count < MAX_STACK) {
                ai_flyer_pool_slot[ai_flyer_pool_count] = slot;
                ai_flyer_pool_stack[ai_flyer_pool_count] = stack;
                ai_flyer_pool_strength[ai_flyer_pool_count] = str;
                ai_flyer_pool_units[ai_flyer_pool_count] = unit;
                ai_flyer_pool_count++;
                if (str < ai_flyer_pool_min_strength)
                    ai_flyer_pool_min_strength = AI_Unit_Strength(unit) / 10;
            } else if (str > ai_flyer_pool_min_strength) {
                weakest = 0;
                for (a = 0; a < MAX_STACK; a++)
                    if (ai_flyer_pool_strength[a] == ai_flyer_pool_min_strength)
                        weakest = a;
                ai_flyer_pool_slot[weakest] = slot;
                ai_flyer_pool_stack[weakest] = stack;
                ai_flyer_pool_strength[weakest] = str;
                ai_flyer_pool_units[weakest] = unit;
                ai_flyer_pool_min_strength = 10000;
                for (a = 0; a < MAX_STACK; a++)
                    if (ai_flyer_pool_strength[a] < ai_flyer_pool_min_strength)
                        ai_flyer_pool_min_strength = ai_flyer_pool_strength[a];
            }
        }
    }
    for (a = 0; a < ai_pool_count - 1; a++) {
        for (pos = 0; pos < ai_pool_count - 1; pos++) {
            if (ai_pool_strength[pos] < ai_pool_strength[pos + 1]) {
                Swap_Words(&ai_pool_slot[pos], &ai_pool_slot[pos + 1]);
                Swap_Words(&ai_pool_stack[pos], &ai_pool_stack[pos + 1]);
                Swap_Words(&ai_pool_strength[pos], &ai_pool_strength[pos + 1]);
                Swap_Words(&ai_pool_units[pos], &ai_pool_units[pos + 1]);
            }
        }
    }
    for (a = 0; a < ai_flyer_pool_count - 1; a++) {
        for (pos = 0; pos < ai_flyer_pool_count - 1; pos++) {
            if (ai_flyer_pool_strength[pos] < ai_flyer_pool_strength[pos + 1]) {
                Swap_Words(&ai_flyer_pool_slot[pos], &ai_flyer_pool_slot[pos + 1]);
                Swap_Words(&ai_flyer_pool_stack[pos], &ai_flyer_pool_stack[pos + 1]);
                Swap_Words(&ai_flyer_pool_strength[pos], &ai_flyer_pool_strength[pos + 1]);
                Swap_Words(&ai_flyer_pool_units[pos], &ai_flyer_pool_units[pos + 1]);
            }
        }
    }
}

/* (player) meld units (magic spirits) go to the nearest node not owned by the AI that is unguarded (threat exactly
   0x8000) on a landmass of status 1 or >= 5 or with an own unit standing on the node, and meld when already there */
void AI_Spirits_To_Nodes(int player)
{
    int stack;
    int node;
    int best_node;
    int best_dist;
    int slot;
    int size;
    int unit;
    int i;
    int ux;
    int uy;
    int nx;
    int ny;
    int dist;
    int own_unit;
    int up;
    int lm;

    for (stack = 0; stack < ai_square_stack_count; stack++) {
        size = p_ai_square_stack_size[stack];
        for (slot = 0; slot < size; slot++) {
            unit = p_ai_square_stack_units[stack][slot];
            if ((unit_types[p_units[unit].type].ability_flags & AFLAG_MELD_WITH_NODE) && unit != AI_SLOT_ORDERED) {   /* p_units[-1] read first */
                ux = p_units[unit].x;
                uy = p_units[unit].y;
                up = p_units[unit].plane;
                best_node = -1;
                best_dist = 1000;
                for (node = 0; node < NUM_NODES; node++) {
                    if (p_nodes[node].plane == up && p_nodes[node].owner != player) {
                        nx = p_nodes[node].x;
                        ny = p_nodes[node].y;
                        if (p_threat_maps[up][ny * MAP_WIDTH + nx] == THREAT_SITE) {
                            lm = p_landmass_ids[up][ny][nx];
                            own_unit = 0;
                            for (i = 0; i < num_units; i++)
                                if (p_units[i].owner == player && p_units[i].x == nx && p_units[i].y == ny &&
                                    p_units[i].plane == up)
                                    own_unit = 1;
                            if (ai_landmass_plan[up][player].status[lm] == LANDMASS_DOMINANT || ai_landmass_plan[up][player].status[lm] >= LANDMASS_STAGING ||
                                own_unit == 1) {
                                dist = Map_Distance(nx, ny, ux, uy, MAP_WIDTH);
                                if (dist < best_dist) {
                                    best_dist = dist;
                                    best_node = node;
                                }
                            }
                        }
                    }
                }
                if (best_node != -1) {
                    if (best_dist == 0)
                        AI_Order_Meld(unit, stack, slot);
                    else {
                        nx = p_nodes[best_node].x;
                        ny = p_nodes[best_node].y;
                        AI_Order_Move(unit, nx, ny, stack, slot);
                    }
                }
            }
        }
    }
}

/* (player, landmass) settlers: none while some site stack (class 3) is empty (never: AI_Build_Stacks
   makes no empty stack). Each settler settles where it stands
   (no foreign unit within 3, or before turn 100, or 1 time in 5; a legal square; and Random(5) <= the objective's
   chance: expansionist 4, theurgist 3, militarist / pragmatist 2, perfectionist 1), else goes to an own tower
   (AI_Settler_Tower_Target), else to the landmass square with the best City_Site_Evaluate value per
   Dist_Max_Plus_Half_Min - max_pop x 10 + production % + gold % + unit cost % + 3 x mineral gold + 5 x mineral
   power + 10 nightshade + 20 mithril + 50 adamantium + 50 coastal - among the legal unthreatened squares (bug: the
   plane passed to City_Site_Evaluate is uninitialised), else overseas (AI_Unit_Go_Overseas). Returns 0 when a site
   stack is empty, else no value
   Numbered: BUG #150 (BUGS_AND_QUIRKS.txt) */
int AI_Settler_Orders(int player, int landmass)
{
    int stack;
    int unit;
    int site_plane;
    int tower_x;
    int tower_y;
    int slot;
    int stack_size;
    int x;
    int y;
    int dist_factor;
    int coastal;
    int unexplored;
    int settle_chance;
    int site_stack_empty;
    int best_score;
    int ty;
    int tx;
    int max_pop;
    int prod_pct;
    int gold_pct;
    int unit_cost_pct;
    int mineral_gold;
    int mineral_power;
    int nightshade;
    int score;
    int mithril;
    int adamantium;
    int best_x;
    int best_y;
    int up;
    int tile;

    settle_chance = 1;
    if (wizards[player].objective == OBJECTIVE_EXPANSIONIST)
        settle_chance = 4;
    else if (wizards[player].objective == OBJECTIVE_MILITARIST)
        settle_chance = 2;
    else if (wizards[player].objective == OBJECTIVE_PRAGMATIST)
        settle_chance = 2;
    else if (wizards[player].objective == OBJECTIVE_PERFECTIONIST)
        settle_chance = 1;
    else if (wizards[player].objective == OBJECTIVE_THEURGIST)
        settle_chance = 3;
    site_stack_empty = 0;
    for (stack = 0; stack < ai_square_stack_count; stack++)
        if (p_ai_square_stack_class[stack] == STACK_CLASS_SITE && p_ai_square_stack_size[stack] < 1)
            site_stack_empty = 1;
    if (site_stack_empty == 1)
        return 0;
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        stack_size = p_ai_square_stack_size[stack];
        for (slot = 0; slot < stack_size; slot++) {
            unit = p_ai_square_stack_units[stack][slot];
            if (unit != AI_SLOT_ORDERED && (unit_types[p_units[unit].type].ability_flags & AFLAG_CREATE_OUTPOST)) {
                x = p_units[unit].x;
                y = p_units[unit].y;
                up = p_units[unit].plane;
                if ((AI_Foreign_Unit_Near(x, y, up, 3, player) == 0 || game_turn < 100 || Random(5) == 1) &&
                    Tile_Settle_Blocker(x, y, up) == SETTLE_OK && Random(5) <= settle_chance)
                    AI_Order_Settle(unit, stack, slot);
                else if (AI_Settler_Tower_Target(player, unit, &tower_x, &tower_y, up) == 1)
                    AI_Order_Move(unit, tower_x, tower_y, stack, slot);
                else {
                    best_score = -1;
                    for (tile = p_landmass_tile_head[up][landmass]; tile != -1; tile = p_landmass_tile_next[up][tile]) {
                        tx = p_landmass_tile_x[up][tile];
                        ty = p_landmass_tile_y[up][tile];
                        if (Tile_Settle_Blocker(tx, ty, up) == SETTLE_OK && (p_threat_maps[up][ty * MAP_WIDTH + tx] & THREAT_STRENGTH_MASK) == 0) {
                            dist_factor = Dist_Max_Plus_Half_Min(x, y, tx, ty);
                            if (dist_factor != 0) {
                                City_Site_Evaluate(tx, ty, site_plane, &max_pop, &prod_pct, &gold_pct, &unit_cost_pct,
                                                   &mineral_gold, &mineral_power, &nightshade, &mithril, &adamantium,
                                                   &coastal, &unexplored);
                                score = (max_pop * 10 + prod_pct + gold_pct + unit_cost_pct + mineral_gold * 3 +
                                         mineral_power * 5 + nightshade * 10 + mithril * 20 + adamantium * 50 +
                                         coastal * 50) / dist_factor;
                                AI_Map_EMS_Tables();
                                if (score > best_score) {
                                    best_score = score;
                                    best_x = tx;
                                    best_y = ty;
                                }
                            }
                        }
                    }
                    if (best_score == -1)
                        AI_Unit_Go_Overseas(unit, x, y, up, player, stack, slot);
                    else
                        AI_Order_Move(unit, best_x, best_y, stack, slot);
                }
            }
        }
    }
}

/* (landmass, plane) purifiers: the first corrupted unthreatened square of the landmass with a site mark (threat
   exactly THREAT_SITE, whoever owns the site) within 2; lone stacks' purifiers (AFLAG_PURIFY) purify where they stand on corruption,
   else go there. Returns 0 when there is no such square, else no value */
int AI_Purify_Orders(int landmass, int plane)
{
    int stack;
    int ux;
    int uy;
    int found;
    int tile;
    int x;
    int y;
    int dx;
    int dy;
    int tx;
    int ty;
    int size;
    int slot;
    int unit;

    found = 0;
    for (tile = p_landmass_tile_head[plane][landmass]; tile != -1 && found == 0; tile = p_landmass_tile_next[plane][tile]) {
        tx = p_landmass_tile_x[plane][tile];
        ty = p_landmass_tile_y[plane][tile];
        if ((p_terrain_flags_maps[plane][ty][tx] & TERRAIN_FLAG_CORRUPTION) && (p_threat_maps[plane][ty * MAP_WIDTH + tx] & THREAT_STRENGTH_MASK) == 0) {
            for (dy = -2; dy < 3; dy++) {
                for (dx = -2; dx < 3; dx++) {
                    if (p_threat_maps[plane][(dy + ty) * MAP_WIDTH + dx + tx] == THREAT_SITE) {
                        found = 1;
                        x = tx;
                        y = ty;
                    }
                }
            }
        }
    }
    if (found == 0)
        return 0;
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        if (p_ai_square_stack_class[stack] == STACK_CLASS_LONE) {
            size = p_ai_square_stack_size[stack];
            for (slot = 0; slot < size; slot++) {
                unit = p_ai_square_stack_units[stack][slot];
                if (unit != AI_SLOT_ORDERED && (unit_types[p_units[unit].type].ability_flags & AFLAG_PURIFY)) {
                    ux = p_units[unit].x;
                    uy = p_units[unit].y;
                    if (p_terrain_flags_maps[plane][uy][ux] & TERRAIN_FLAG_CORRUPTION)
                        AI_Order_Purify(unit, stack, slot);
                    else
                        AI_Order_Move(unit, x, y, stack, slot);
                }
            }
        }
    }
}

/* (landmass) engineers (road builders): the nearest city of ANY owner on the engineer's plane and landmass (any
   landmass for water-crossing stacks - AISTACK_CROSSES_WATER of the OTHER stack table, p_ai_own_stacks, at this index: bug); farther than 1
   they go there, else they build a road towards the nearest city on that city's landmass not yet road-connected to
   it. Two near heap scratch pointers are set and never used
   Numbered: BUG #151 (BUGS_AND_QUIRKS.txt) */
void AI_Engineer_Orders(int landmass)
{
    int i;
    int stack;
    int x1;
    int y1;
    int lm;
    int best_dist;
    int near_lm;
    int dist;
    int near_city;
    int road_to;
    int size;
    int slot;
    int unit;
    int up;
    char *scratch_a;
    char *scratch_b;

    scratch_a = near_heap + 80;
    scratch_b = near_heap + 140;
    for (stack = 0; stack < ai_square_stack_count; stack++) {
        size = p_ai_square_stack_size[stack];
        for (slot = 0; slot < size; slot++) {
            unit = p_ai_square_stack_units[stack][slot];
            if (unit != AI_SLOT_ORDERED && unit_types[p_units[unit].type].construction > 0) {
                x1 = p_units[unit].x;
                y1 = p_units[unit].y;
                up = p_units[unit].plane;
                lm = landmass;
                near_city = -1;
                best_dist = 1000;
                for (i = 0; i < num_cities; i++) {
                    if (p_cities[i].plane == up &&
                        ((p_ai_own_stacks[stack].flags & AISTACK_CROSSES_WATER) ||   /* BUG #151: stack indexes the square stacks, not p_ai_own_stacks */ p_landmass_ids[up][p_cities[i].y][p_cities[i].x] == lm)) {
                        dist = Map_Distance(x1, y1, p_cities[i].x, p_cities[i].y, MAP_WIDTH);
                        if (dist < best_dist) {
                            best_dist = dist;
                            near_city = i;
                        }
                    }
                }
                if (near_city != -1) {
                    if (best_dist > 1)
                        AI_Order_Move(unit, p_cities[near_city].x, p_cities[near_city].y, stack, slot);
                    else {
                        road_to = -1;
                        best_dist = 1000;
                        near_lm = p_landmass_ids[up][p_cities[near_city].y][p_cities[near_city].x];
                        for (i = 0; i < num_cities; i++) {
                            if (i != near_city && p_cities[i].plane == up &&
                                Bit_Test(i, p_cities[near_city].road_connections) == 0 &&
                                p_landmass_ids[up][p_cities[i].y][p_cities[i].x] == near_lm) {
                                dist = Map_Distance(p_cities[near_city].x, p_cities[near_city].y, p_cities[i].x,
                                                    p_cities[i].y, MAP_WIDTH);
                                if (dist < best_dist) {
                                    best_dist = dist;
                                    road_to = i;
                                }
                            }
                        }
                        if (road_to != -1)
                            AI_Order_Build_Road(unit, p_cities[road_to].x, p_cities[road_to].y, stack, slot);
                    }
                }
            }
        }
    }
}

/* (unit, x, y, stack, slot) sends the unit to (x, y): ORDER_GOTO_ENEMY (hunting) onto a foreign army in the open (threat
   & 0x3fff without the 0x8000 flag), else ORDER_GOTO (moving); the stack slot is cleared. Returns 0 for a unit index outside
   0..MAX_UNITS (this guard lets 1000 through; the ones below stop at 999), else no value */
int AI_Order_Move(int unit, int x, int y, int stack, int slot)
{
    int up;
    int threat;

    if (unit < 0 || unit > MAX_UNITS)
        return 0;
    up = p_units[unit].plane;
    threat = p_threat_maps[up][y * MAP_WIDTH + x];
    if ((threat & THREAT_SITE) == 0 && (threat & THREAT_STRENGTH_MASK) != 0)
        p_units[unit].order = ORDER_GOTO_ENEMY;
    else
        p_units[unit].order = ORDER_GOTO;
    p_units[unit].dest_x = x;
    p_units[unit].dest_y = y;
    p_ai_square_stack_units[stack][slot] = AI_SLOT_ORDERED;
}

/* (unit, stack, slot) the settler builds a city (ORDER_SETTLE); the slot is cleared. 0 for a bad unit index */
int AI_Order_Settle(int unit, int stack, int slot)
{
    if (unit < 0 || unit >= MAX_UNITS)
        return 0;
    p_units[unit].order = ORDER_SETTLE;
    p_ai_square_stack_units[stack][slot] = AI_SLOT_ORDERED;
}

/* (unit, dest_x, dest_y, stack, slot) the engineer builds a road from where it stands to (dest_x, dest_y)
   (ORDER_BUILD_ROAD); the slot is cleared. 0 for a bad unit index */
int AI_Order_Build_Road(int unit, int dest_x, int dest_y, int stack, int slot)
{
    if (unit < 0 || unit >= MAX_UNITS)
        return 0;
    p_units[unit].road_start_x = p_units[unit].x;
    p_units[unit].road_start_y = p_units[unit].y;
    p_units[unit].order = ORDER_BUILD_ROAD;
    p_units[unit].dest_x = dest_x;
    p_units[unit].dest_y = dest_y;
    p_ai_square_stack_units[stack][slot] = AI_SLOT_ORDERED;
}

/* (unit, stack, slot) the unit waits for a transport (status 11, dest_x 10); the slot is cleared. 0 for a bad unit
   index */
int AI_Order_Seek_Transport(int unit, int stack, int slot)
{
    if (unit < 0 || unit >= MAX_UNITS)
        return 0;
    p_units[unit].order = ORDER_AI_WAIT;
    p_units[unit].dest_x = 10;              /* ORDER_AI_WAIT's countdown (ORDERS.C counts dest_x down) */
    p_ai_square_stack_units[stack][slot] = AI_SLOT_ORDERED;
}

/* (unit, stack, slot) the spirit melds with the node (ORDER_MELD); the slot is cleared. 0 for a bad unit index */
int AI_Order_Meld(int unit, int stack, int slot)
{
    if (unit < 0 || unit >= MAX_UNITS)
        return 0;
    p_units[unit].order = ORDER_MELD;
    p_ai_square_stack_units[stack][slot] = AI_SLOT_ORDERED;
}

/* (unit, stack, slot) the unit purifies its square (ORDER_PURIFY, dest_x 20); the slot is cleared. 0 for a bad unit
   index */
int AI_Order_Purify(int unit, int stack, int slot)
{
    if (unit < 0 || unit >= MAX_UNITS)
        return 0;
    p_units[unit].order = ORDER_PURIFY;
    p_units[unit].dest_x = 20;                  /* the purify work counter (dest_x doubles as it) */
    p_ai_square_stack_units[stack][slot] = AI_SLOT_ORDERED;
}

/* (x, y, plane, radius, owner) 1 if a unit not of 'owner' stands within radius on the plane (no x wrap) */
int AI_Foreign_Unit_Near(int x, int y, int plane, int radius, int owner)
{
    int x_lo;
    int x_hi;
    int y_lo;
    int y_hi;
    int unit;

    x_lo = x - radius;
    y_lo = y - radius;
    x_hi = x + radius;
    y_hi = y + radius;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].x >= x_lo && p_units[unit].x <= x_hi && p_units[unit].y >= y_lo && p_units[unit].y <= y_hi &&
            p_units[unit].plane == plane && p_units[unit].owner != owner)
            return 1;
    return 0;
}

/* (x, y, plane, radius, owner, landmass) 1 if a unit not of 'owner' stands within radius (no x wrap) on the plane
   and on the landmass */
int AI_Foreign_Unit_Near_On_Landmass(int x, int y, int plane, int radius, int owner, int landmass)
{
    int x_lo;
    int x_hi;
    int y_lo;
    int y_hi;
    int unit;

    x_lo = x - radius;
    y_lo = y - radius;
    x_hi = x + radius;
    y_hi = y + radius;
    for (unit = 0; unit < num_units; unit++)
        if (p_units[unit].x >= x_lo && p_units[unit].x <= x_hi && p_units[unit].y >= y_lo && p_units[unit].y <= y_hi &&
            p_units[unit].plane == plane && p_units[unit].owner != owner &&
            p_landmass_ids[plane][p_units[unit].y][p_units[unit].x] == landmass)
            return 1;
    return 0;
}

/* (player, unit, *out_x, *out_y, plane) a settler ON the fortress's plane and not on a tower heads for the nearest
   own tower on its landmass, unless Planar Seal is up or the landmass beyond the tower has status 2 or 4 */
int AI_Settler_Tower_Target(int player, int unit, int *out_x, int *out_y, int plane)
{
    int i;
    int x1;
    int y1;
    int up;
    int tx;
    int ty;
    int best_x;
    int best_y;
    int best_dist;
    int other_plane;
    int on_tower;
    int lm;
    int dist;
    int unused;
    int dest_lm;

    for (i = 0; i < num_players; i++)
        if (wizards[i].globals.planar_seal)
            return 0;
    other_plane = 1 - p_fortresses[player].plane;
    if (plane == other_plane)
        return 0;
    x1 = p_units[unit].x;
    y1 = p_units[unit].y;
    up = p_units[unit].plane;
    on_tower = 0;
    for (i = 0; i < NUM_TOWERS; i++)
        if (p_towers[i].x == x1 && p_towers[i].y == y1)
            on_tower = 1;
    if (on_tower == 1)
        return 0;
    lm = p_landmass_ids[up][y1][x1];
    best_dist = 1000;
    for (i = 0; i < NUM_TOWERS; i++) {
        if (p_towers[i].owner == player && p_landmass_ids[up][p_towers[i].y][p_towers[i].x] == lm) {
            dist = Map_Distance(x1, y1, p_towers[i].x, p_towers[i].y, MAP_WIDTH);
            if (dist < best_dist) {
                best_dist = dist;
                best_x = p_towers[i].x;
                best_y = p_towers[i].y;
            }
        }
    }
    if (best_dist == 1000)
        return 0;
    tx = best_x;
    ty = best_y;
    dest_lm = p_landmass_ids[other_plane][ty][tx];
    unused = 0;
    if (ai_landmass_plan[other_plane][player].status[dest_lm] == LANDMASS_CONTESTED || ai_landmass_plan[other_plane][player].status[dest_lm] == LANDMASS_IGNORE)
        return 0;
    *out_x = tx;
    *out_y = ty;
    return 1;
}

/* (unit, x, y, plane, player, stack, slot) a unit with nothing to do on its landmass heads for the expansion
   landmass (none: returns 0): flyers / water crossers go straight to the expansion point; walkers next to shore
   water board an own transport stack within 1 square, else wait for a transport (ORDER_AI_WAIT) and file a request at
   the last shore square; walkers inland go to the free land next to the nearest coast square. The plane parameter
   is replaced by the unit's plane; the unit's y is read from its x (bug, unused); no value at the end
   Numbered: BUG #351 (BUGS_AND_QUIRKS.txt) */
int AI_Unit_Go_Overseas(int unit, int x, int y, int plane, int player, int stack, int slot)
{
    int try_x;                                      /* the unit's x, then the wrapped x, then the ship stack's x */
    int try_y;                                      /* the loop y, then the ship stack's y */
    int at_shore;
    int ship_found;
    int i;
    int nx;
    int ship_x;
    int ship_y;
    int here_lm;
    int shore_x;
    int shore_y;
    int ship_cap;
    int room;
    int can_cross;
    int ex;
    int ey;

    here_lm = p_landmass_ids[plane][y][x];   /* never read */
    try_x = p_units[unit].x;
    try_y = p_units[unit].x;
    plane = p_units[unit].plane;
    can_cross = 0;
    if (Unit_Can_Fly(unit) || Unit_Can_Cross_Water(unit))
        can_cross = 1;
    if (p_ai_expand_landmass[plane][player] == 0)
        return 0;
    ex = p_ai_expand_x[plane][player];
    ey = p_ai_expand_y[plane][player];
    if (can_cross == 1)
        AI_Order_Move(unit, ex, ey, stack, slot);
    else {
        at_shore = 0;
        for (try_y = y - 1; try_y < y + 2; try_y++) {
            if (try_y > 0 && try_y < MAP_HEIGHT) {
                for (nx = x - 1; nx < x + 2; nx++) {
                    try_x = nx;
                    if (try_x < 0)
                        try_x += MAP_WIDTH;
                    if (try_x >= MAP_WIDTH)
                        try_x -= MAP_WIDTH;
                    if (Tile_Is_Shore_Water(try_x, try_y, plane)) {
                        at_shore = 1;
                        shore_x = nx;
                        shore_y = try_y;
                    }
                }
            }
        }
        if (at_shore == 0) {
            AI_Nearest_Coast_Tile(x, y, plane, &shore_x, &shore_y);
            if (AI_Adjacent_Free_Land(shore_x, shore_y, plane, &shore_x, &shore_y) == 1)
                AI_Order_Move(unit, shore_x, shore_y, stack, slot);
        } else {
            ship_found = 0;
            for (i = 0; i < ai_own_stack_count && ship_found == 0; i++) {
                if (p_ai_own_stacks[i].plane == plane && (p_ai_own_stacks[i].flags & AISTACK_TRANSPORT)) {
                    try_x = p_ai_own_stacks[i].x;
                    try_y = p_ai_own_stacks[i].y;
                    if (abs(x - try_x) < 2 && abs(y - try_y) < 2) {
                        ship_found = 1;
                        ship_x = try_x;
                        ship_y = try_y;
                        ship_cap = p_ai_own_stacks[i].spare_transport;   /* never read */
                        room = MAX_STACK - p_ai_own_stacks[i].count;     /* never read */
                    }
                }
            }
            if (ship_found == 1)
                AI_Order_Move(unit, ship_x, ship_y, stack, slot);
            else if (ai_transport_req_count < AI_MAX_TRANSPORT_REQS) {
                AI_Order_Seek_Transport(unit, stack, slot);
                ai_transport_req_x[ai_transport_req_count] = shore_x;
                ai_transport_req_y[ai_transport_req_count] = shore_y;
                ai_transport_req_plane[ai_transport_req_count] = plane;
                ai_transport_req_count++;
            }
        }
    }
}

/* (x, y, plane) files a transport pickup point (no duplicates, at most 15). Returns 0 when not added, else no value */
int AI_Add_Transport_Request(int x, int y, int plane)
{
    int i;

    for (i = 0; i < ai_transport_req_count; i++)
        if (ai_transport_req_x[i] == x && ai_transport_req_y[i] == y && ai_transport_req_plane[i] == plane)
            return 0;
    if (ai_transport_req_count >= AI_MAX_TRANSPORT_REQS)
        return 0;
    ai_transport_req_x[ai_transport_req_count] = x;
    ai_transport_req_y[ai_transport_req_count] = y;
    ai_transport_req_plane[ai_transport_req_count] = plane;
    ai_transport_req_count++;
}

/* (player, x, y, plane) 1 if the landmass at (x, y) on the OTHER plane is worth crossing to by a tower: (not status
   1 / 4) it holds a neutral city or one of a wizard at hostility >= HOSTILITY_HOSTILE (BUG #426: the city's plane is
   not tested, so a city with that landmass id on the tower's own plane counts too), or (not status 3) it has more than 10 squares */
int AI_Tower_Other_Side_Worth(int player, int x, int y, int plane)
{
    int i;
    int worth;
    int lm;
    int other_plane;
    int owner;
    int tile;

    worth = 0;
    other_plane = 1 - plane;
    lm = p_landmass_ids[other_plane][y][x];
    if (ai_landmass_plan[other_plane][player].status[lm] != LANDMASS_DOMINANT && ai_landmass_plan[other_plane][player].status[lm] != LANDMASS_IGNORE) {
        for (i = 0; i < num_cities && worth == 0; i++) {
            if (p_landmass_ids[p_cities[i].plane][p_cities[i].y][p_cities[i].x] == lm) {
                owner = p_cities[i].owner;
                if (owner == PLAYER_NEUTRAL || wizards[player].hostility[owner] >= HOSTILITY_HOSTILE)
                    worth = 1;
            }
        }
    }
    if (ai_landmass_plan[other_plane][player].status[lm] != LANDMASS_NO_CITY) {
        i = 0;
        tile = p_landmass_tile_head[other_plane][lm];
        while (tile != -1 && worth == 0) {
            i++;
            tile = p_landmass_tile_next[other_plane][tile];
            if (i > 10)
                worth = 1;
        }
    }
    return worth;
}

/* (player, landmass, plane) replans the landmass: the player's units on it (not road builders) drop their orders;
   the rally point becomes the centre of the own cities there; status 3 without an own city, else 1 when the own
   units' cost exceeds 10 x the foreign units' (settlers not counted), else 2; 4 when an allied wizard (TREATY_ALLIANCE)
   owns a city there and the player none; the ocean (landmass 0) is always 4. For 1 / 2 / 4 the rally point moves to
   the unthreatened square nearest it (+ Random(5)), for 3 to the nearest unthreatened coast square (+ Random(20)) -
   none: status 4, rally point 0, 0; with status 1 it then moves to the unthreatened own coast square nearest the
   centre of the attack landmass's coast (+ Random(5)). Bug: the plane parameter is overwritten with each own city's
   and each unit's plane inside the loops, so the cost count mixes both planes and the status may land on the other
   plane
   Numbered: BUG #120 (BUGS_AND_QUIRKS.txt) */
void AI_Replan_Landmass(int player, int landmass, int plane)
{
    int i;
    int count;
    int rx;
    int ry;
    int tile;
    int best;
    int cx_sum;
    int cy_sum;
    int threat;
    int tx;                                     /* also the coast tile counter */
    int ty;
    int best_x;
    int best_y;
    unsigned x_sum;                             /* the city x sum, then the own unit cost sum (y_sum: the foreign one);
                                                   BUG #427: not reset in between, the cost sums start from the coordinate sums */
    unsigned y_sum;
    int ut;
    int d;

    AI_Map_EMS_Tables();
    for (i = 0; i < num_units; i++)
        if (p_units[i].owner == player && unit_types[p_units[i].type].construction == 0 &&
            p_landmass_ids[p_units[i].plane][p_units[i].y][p_units[i].x] == landmass)
            p_units[i].order = ORDER_READY;
    ai_landmass_plan[plane][player].status[landmass] = LANDMASS_UNPLANNED;
    x_sum = 0;
    y_sum = 0;
    for (i = 0; i < num_cities; i++) {
        if (p_cities[i].owner == player) {
            plane = p_cities[i].plane;
            if (p_landmass_ids[plane][p_cities[i].y][p_cities[i].x] == landmass) {
                x_sum += p_cities[i].x;
                y_sum += p_cities[i].y;
                /* status[] counts the own cities here first; the real status is stored below */
                ai_landmass_plan[plane][player].status[landmass] = ai_landmass_plan[plane][player].status[landmass] + 1;
            }
        }
    }
    count = ai_landmass_plan[plane][player].status[landmass];
    if (count > 0) {
        ai_landmass_plan[plane][player].rally_x[landmass] = x_sum / count;
        ai_landmass_plan[plane][player].rally_y[landmass] = y_sum / count;
    }
    for (i = 0; i < num_units; i++) {
        ut = p_units[i].type;
        if ((unit_types[ut].ability_flags & AFLAG_CREATE_OUTPOST) == 0) {
            plane = p_units[i].plane;
            if (p_landmass_ids[plane][p_units[i].y][p_units[i].x] == landmass) {
                if (p_units[i].owner == player)
                    x_sum += unit_types[ut].cost;
                else
                    y_sum += unit_types[ut].cost;
            }
        }
    }
    if (ai_landmass_plan[plane][player].status[landmass] > 0) {
        if (y_sum * 10 < x_sum)
            ai_landmass_plan[plane][player].status[landmass] = LANDMASS_DOMINANT;
        else
            ai_landmass_plan[plane][player].status[landmass] = LANDMASS_CONTESTED;
    } else
        ai_landmass_plan[plane][player].status[landmass] = LANDMASS_NO_CITY;
    for (i = 0; i < num_cities; i++)
        if (p_cities[i].owner != player && p_cities[i].owner != PLAYER_NEUTRAL &&
            p_landmass_ids[plane][p_cities[i].y][p_cities[i].x] == landmass &&
            wizards[player].diplomacy_status[p_cities[i].owner] == TREATY_ALLIANCE && ai_landmass_plan[plane][player].status[landmass] == LANDMASS_NO_CITY)
            ai_landmass_plan[plane][player].status[landmass] = LANDMASS_IGNORE;
    ai_landmass_plan[0][player].status[0] = LANDMASS_IGNORE;
    ai_landmass_plan[1][player].status[0] = LANDMASS_IGNORE;
    if (ai_landmass_plan[plane][player].status[landmass] == LANDMASS_CONTESTED || ai_landmass_plan[plane][player].status[landmass] == LANDMASS_DOMINANT ||
        ai_landmass_plan[plane][player].status[landmass] == LANDMASS_IGNORE) {
        rx = ai_landmass_plan[plane][player].rally_x[landmass];
        ry = ai_landmass_plan[plane][player].rally_y[landmass];
        best = 1000;
        for (tile = p_landmass_tile_head[plane][landmass]; tile != -1; tile = p_landmass_tile_next[plane][tile]) {
            d = Map_Distance(p_landmass_tile_x[plane][tile], p_landmass_tile_y[plane][tile], rx, ry, MAP_WIDTH) + Random(5);
            if (d < best) {
                tx = p_landmass_tile_x[plane][tile];
                ty = p_landmass_tile_y[plane][tile];
                threat = p_threat_maps[plane][ty * MAP_WIDTH + tx];
                if (threat == 0) {
                    best = d;
                    best_x = p_landmass_tile_x[plane][tile];
                    best_y = p_landmass_tile_y[plane][tile];
                }
            }
        }
        if (best < 1000) {
            ai_landmass_plan[plane][player].rally_x[landmass] = best_x;
            ai_landmass_plan[plane][player].rally_y[landmass] = best_y;
        } else {
            ai_landmass_plan[plane][player].rally_x[landmass] = 0;
            ai_landmass_plan[plane][player].rally_y[landmass] = 0;
            ai_landmass_plan[plane][player].status[landmass] = LANDMASS_IGNORE;
        }
    }
    if (ai_landmass_plan[plane][player].status[landmass] == LANDMASS_NO_CITY) {
        rx = ai_landmass_plan[plane][player].rally_x[landmass];
        ry = ai_landmass_plan[plane][player].rally_y[landmass];
        best = 1000;
        for (tile = p_coast_tile_head[plane][landmass]; tile != -1; tile = p_coast_tile_next[plane][tile]) {
            tx = p_coast_tile_x[plane][tile];
            ty = p_coast_tile_y[plane][tile];
            if (p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0) {
                d = Map_Distance(tx, ty, rx, ry, MAP_WIDTH) + Random(20);
                if (d < best) {
                    best = d;
                    best_x = p_coast_tile_x[plane][tile];
                    best_y = p_coast_tile_y[plane][tile];
                }
            }
        }
        if (best < 1000) {
            ai_landmass_plan[plane][player].rally_x[landmass] = best_x;
            ai_landmass_plan[plane][player].rally_y[landmass] = best_y;
        } else {
            ai_landmass_plan[plane][player].rally_x[landmass] = 0;
            ai_landmass_plan[plane][player].rally_y[landmass] = 0;
            ai_landmass_plan[plane][player].status[landmass] = LANDMASS_IGNORE;
        }
    }
    if (ai_landmass_plan[plane][player].status[landmass] == LANDMASS_DOMINANT) {
        cx_sum = 0;
        cy_sum = 0;
        tx = 0;
        for (tile = p_coast_tile_head[plane][p_ai_attack_landmass[plane][player]]; tile != -1;
             tile = p_coast_tile_next[plane][tile]) {
            cx_sum += p_coast_tile_x[plane][tile];
            cy_sum += p_coast_tile_y[plane][tile];
            tx++;
        }
        cx_sum = cx_sum / tx;                   /* from here cx_sum / cy_sum hold the centre */
        cy_sum = cy_sum / tx;
        best = 1000;
        for (tile = p_coast_tile_head[plane][landmass]; tile != -1; tile = p_coast_tile_next[plane][tile]) {
            d = Map_Distance(p_coast_tile_x[plane][tile], p_coast_tile_y[plane][tile], cx_sum, cy_sum, MAP_WIDTH) + Random(5);
            if (d < best) {
                tx = p_coast_tile_x[plane][tile];
                ty = p_coast_tile_y[plane][tile];
                if (p_threat_maps[plane][ty * MAP_WIDTH + tx] == 0) {
                    best = d;
                    best_x = tx;
                    best_y = ty;
                }
            }
        }
        if (best < 1000) {
            ai_landmass_plan[plane][player].rally_x[landmass] = best_x;
            ai_landmass_plan[plane][player].rally_y[landmass] = best_y;
        }
    }
}
