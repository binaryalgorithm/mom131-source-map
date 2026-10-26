/* ASSETS.C - WIZARDS.EXE overlay code segment 45fc (stub 23e2): loading the art and data - at startup the LBX files
   kept in EMS, the map pictures and terrain tables, the main screen's buttons, the city screen's people and
   resources, the UI sounds, the spell and building data; per screen (into p_screen_gfx) the unit figures, the
   battlefield, the cityscape, the unit view icons, the items and the small spellbook pictures. Many loaders are
   empty (removed).
   This module's data (DS:291e): its literals (the LBX names); no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 45fc src/wizards/ASSETS.C */

/* the module's functions in their original declaration order */
void Load_Startup_Assets(void);
void Load_Map_Assets(void);
void Load_Removed_0466(void);
void Load_Main_Screen_Assets(void);
void Load_Removed_0af4(void);
void Load_Removed_0afe(void);
void Load_Removed_0aef(void);
void Load_Removed_0b03(void);
void Load_Backgrnd_Extra(void);
void Load_Removed_0b08(void);
void Load_Removed_0b35(void);
void Load_Removed_0b3a(void);
void Load_Spell_Data(void);
void Load_Building_Data(void);
void Load_Removed_0b99(void);
void Load_UI_Sounds(void);
void UnitTypes_Set_Upkeep_Early(void);
void Load_Backgrnd_UI_Assets(void);
void Load_Removed_0af9(void);
void Screen_Gfx_Reset(void);
void World_Rest_Reset(void);
void Load_Unit_Figure_Sets(void);
void Load_Battlefield_Art(void);
void Load_Removed_1046(void);
void Load_Cityscape_Art(void);
void Load_Diplomacy_Removed(void);
void Load_UnitView_Art(void);
void Item_Graphics_Load(void);
void Spellbook_Load_Small_Pictures(void);
void Load_Overland_Screen_Graphics(void);
void Load_Unit_View_Graphics(void);
void Load_Stack_Popup_Art(void);
void Load_Diplomacy_Art(void);
void Load_Combat_Graphics(void);

#include "GAME.H"

extern unsigned item_icon_pics[];               /* DS:913e  ITEMS.LBX 0..115 */
extern unsigned unitview_land_bg_pic;           /* DS:92b6  CITYSCAP 0x67 */
extern unsigned building_pics[];                /* DS:92b8  CITYSCAP pictures by BuildingId */
extern unsigned cityscape_trade_goods_pic;      /* DS:9300 */
extern unsigned cityscape_housing_pics[];       /* DS:9302 */
extern unsigned cityscape_fortress_pic;         /* DS:9308 */
extern unsigned cityscape_city_walls_pic;       /* DS:930a  CITYSCAP 0x4c 'city walls': the wall ring drawn round the cityscape */
extern unsigned cityscape_altar_pic;            /* DS:930c */
extern unsigned cityscape_evil_presence_pic;    /* DS:930e */
extern unsigned cityscape_dark_rituals_pic;     /* DS:9310 */
extern unsigned cityscape_astral_gate_pic;      /* DS:9312 */
extern unsigned cityscape_stream_of_life_pic;   /* DS:9314 */
extern unsigned cityscape_earth_gate_pic;       /* DS:9316 */
extern unsigned cityscape_summon_circle_pic;    /* DS:9318 */
extern unsigned cityscape_rocks_pics[];         /* DS:931a */
extern unsigned cityscape_tree_pics[];          /* DS:9320 */
extern unsigned cityscape_house_pics[][5];      /* DS:9326  by house style */
extern unsigned cityscape_roads_pic;            /* DS:9344 */
extern unsigned cityscape_wall_of_darkness_pic;      /* DS:9346 */
extern unsigned cityscape_wall_of_fire_pic;     /* DS:934a */
extern unsigned cityscape_consecration_pic;     /* DS:934c */
extern unsigned cityscape_prosperity_pic;       /* DS:934e */
extern unsigned cityscape_inspirations_pic;     /* DS:9350 */
extern unsigned cityscape_natures_eye_pic;      /* DS:9352 */
extern unsigned cityscape_sorcery_ward_pic;     /* DS:9354 */
extern unsigned cityscape_life_ward_pic;        /* DS:9356 */
extern unsigned cityscape_nature_ward_pic;      /* DS:9358 */
extern unsigned cityscape_chaos_ward_pic;       /* DS:935a */
extern unsigned cityscape_death_ward_pic;       /* DS:935c */
extern unsigned cityscape_famine_pic;           /* DS:935e */
extern unsigned cityscape_far_chaos_rift_pic;   /* DS:9360 */
extern unsigned cityscape_far_heavenly_light_pic;        /* DS:9362 */
extern unsigned cityscape_far_dark_cloud_pic;   /* DS:9364 */
extern unsigned cityscape_mountain_bg_pic;       /* DS:9366 */
extern unsigned cityscape_hills_bg_pic;         /* DS:9368 */
extern unsigned cityscape_ocean_pic;            /* DS:936a */
extern unsigned cityscape_river_pic;            /* DS:936c */
extern unsigned cityscape_cursed_land_pic;      /* DS:936e */
extern unsigned cityscape_living_lands_pic;     /* DS:9370 */
extern unsigned cityscape_gaia_pic;             /* DS:9372 */
extern unsigned cityscape_flying_fortress_pic;  /* DS:9374 */
extern unsigned cityscape_arcanus_ground_pic;              /* DS:9376 */
extern unsigned cityscape_myrror_river_pic;      /* DS:9378 */
extern unsigned cityscape_myrror_ocean_pic;      /* DS:937a */
extern unsigned cityscape_plain_bg_pic;         /* DS:937c */
extern unsigned cityscape_myrror_chaos_rift_pic;       /* DS:937e */
extern unsigned cityscape_myrror_dark_cloud_pic; /* DS:9380 */
extern unsigned cityscape_myrror_heavenly_light_pic;      /* DS:9382 */
extern unsigned cityscape_myrror_plains_pic;     /* DS:9384 */
extern unsigned cityscape_myrror_mountain_pic;   /* DS:9386 */
extern unsigned cityscape_myrror_hills_pic;      /* DS:9388 */
extern unsigned cityscape_myrror_ground_pic;     /* DS:938a */
extern unsigned snd_click;                      /* DS:9414  SOUNDFX 2 */
extern unsigned snd_message;                    /* DS:941a  SOUNDFX 0 */
extern unsigned event_icon_pics[];              /* DS:942e  MAIN 0x3b..0x40 */
extern unsigned cityscape_rubble_pics[];        /* DS:943a */
extern unsigned spellbook_page_bg_pic;          /* DS:9452 */
extern unsigned spellbook_realm_icon_src_pics[];      /* DS:9454 */
extern unsigned spellbook_corner_right_pic;     /* DS:9468 */
extern unsigned spellbook_corner_left_pic;      /* DS:946a */
extern unsigned spellbook_small_pic;            /* DS:946e */
extern unsigned ok_button_pic;                  /* DS:9498 */
extern unsigned cityprod_right_tab_pic;         /* DS:949a */
extern unsigned cityprod_left_tab_pic;          /* DS:949c */
extern unsigned resource_x10_pics[];            /* DS:94a0 */
extern unsigned food_deficit_x10_pic;           /* DS:94ac */
extern unsigned city_rebel_pics[];              /* DS:94ae */
extern unsigned city_farmer_pics[];             /* DS:94ca */
extern unsigned resource_x1_pics[];             /* DS:94e6 */
extern unsigned food_deficit_x1_pic;           /* DS:94f0 */
extern unsigned city_worker_pics[];             /* DS:94f2 */
extern unsigned next_turn_button_pic;           /* DS:9516 */
extern unsigned survey_bg_pic;                  /* DS:9518 */
extern unsigned road_button_border_pic;         /* DS:951a */
extern unsigned road_bg_pic;                    /* DS:951c */
extern unsigned road_ok_pic;                    /* DS:951e */
extern unsigned button_blockout_pic;            /* DS:9522 */
extern unsigned cast_button_border_pic;         /* DS:9524 */
extern unsigned main_cancel_button_pic;         /* DS:9526 */
extern unsigned cast_bg_pic;                    /* DS:9528 */
extern unsigned next_turn_bg_pic;               /* DS:952c */
extern unsigned deselect_bg_pic;                /* DS:952e */
extern unsigned outpost_house_empty_pics[3];    /* DS:9530 */
extern unsigned outpost_house_pics[3];          /* DS:9536 */
extern unsigned name_entry_box_pic;             /* DS:953c */
extern unsigned outpost_bg_pic;                 /* DS:953e */
extern unsigned unitview_button_panel_pic;      /* DS:9540 */
extern unsigned unitview_stone_frame_pic;                 /* DS:9548  UNITVIEW 0x1e: a teal stone frame, never drawn */
extern unsigned wizinfo_bg_pic;                 /* DS:956e */
extern unsigned item_empty_slot_pics[6];        /* DS:957a */
extern unsigned city_shared_tile_pic;           /* DS:95f0  BACKGRND 0: the mark on a map square another city shares */
extern unsigned goto_marker_pic;                /* DS:95f2 */
extern unsigned combat_border_pic;              /* DS:973c */
extern unsigned unitlist_bottom_pic;            /* DS:9754 */
extern unsigned unitlist_bg_pic;                /* DS:9756 */
extern unsigned city_done_backdrop_pic;         /* DS:9758  BACKGRND 10: the grassland behind a completed building */
extern unsigned city_ench_down_arrow_pic;       /* DS:975a */
extern unsigned city_ench_up_arrow_pic;         /* DS:975c */
extern unsigned unitview_slot_icons[];          /* DS:975e */
extern unsigned item_power_bullet_pic;          /* DS:976a */
extern unsigned item_card_pic;                  /* DS:976c */
extern unsigned item_power_icons[];             /* DS:976e  ITEMISC 0..18 */
extern unsigned item_icon_dispel_evil_pic;      /* DS:9794  ITEMISC 0x21 'item dispel evil': a 16 x 16 item power icon, never drawn */
extern unsigned item_icon_giant_strength_pic;   /* DS:9796  ITEMISC 0x22 'item giant str': an item power icon, never drawn */
extern unsigned item_icon_guardian_wind_pic;    /* DS:9798  ITEMISC 0x23 'item guardian wind': an item power icon, never drawn */
extern unsigned unitview_water_bg_pic;          /* DS:979c */
extern unsigned unitview_portrait_frame_pic;    /* DS:979e */
extern unsigned gold_coin_pic;                  /* DS:97a0 */
extern unsigned gold_deficit_x1_pic;            /* DS:97a2  BACKGRND 0x49 'black gold': the grey coin of one gold short */
extern unsigned unitview_row_frame_pics[];      /* DS:97a4 */
extern unsigned unitview_arrow_down_pic;        /* DS:97b4 */
extern unsigned unitview_arrow_up_pic;          /* DS:97b6 */
extern unsigned unitview_bg_pic;                /* DS:97b8 */
extern unsigned unitview_build_bg_pic;          /* DS:97ba */
extern unsigned unitview_stat_icons_gold[];     /* DS:97bc */
extern unsigned unitview_stat_icons[];          /* DS:97da */
extern unsigned special_icons[];                /* DS:97f8 */
extern unsigned special2_icons[];               /* DS:98d6 */
extern unsigned lair_confirm_border_pic;        /* DS:991a */
extern unsigned lair_bottom_nobutton_pic;       /* DS:991c */
extern unsigned lair_bottom_button_pic;         /* DS:991e */
extern unsigned main_screen_bg_pic;             /* DS:9922 */
extern unsigned stack_slot_bg_pics[];           /* DS:9924 */
extern unsigned city_screen_bg_pic;             /* DS:9936  BACKGRND 6: the city screen */
extern unsigned city_no_product_pic;            /* DS:9938  BACKGRND 13: blank panel over the product (Trade Goods / Housing) */
extern unsigned prod_box_filled_pic;                 /* DS:993a */
extern unsigned prod_box_empty_pic;                 /* DS:993c */
extern unsigned city_buy_greyed_pic;            /* DS:993e  BACKGRND 14: Buy greyed out */
extern unsigned city_ok_button_pic;             /* DS:9940  BACKGRND 9: OK */
extern unsigned city_buy_button_pic;            /* DS:9942  BACKGRND 7: Buy */
extern unsigned city_change_button_pic;         /* DS:9944  BACKGRND 8: Change */
extern unsigned medal_red_pic;                  /* DS:9946 */
extern unsigned medal_gold_pic;                 /* DS:9948 */
extern unsigned medal_white_pic;                /* DS:994a */
extern unsigned weapon_adamantium_pic;          /* DS:994c */
extern unsigned weapon_mithril_pic;             /* DS:994e */
extern unsigned weapon_magic_pic;               /* DS:9950 */
extern unsigned purify_lock_pic;                /* DS:9952 */
extern unsigned special_order_blank_pic;       /* DS:9954 */
extern unsigned wait_lock_pic;                  /* DS:9956 */
extern unsigned patrol_lock_pic;        /* DS:9958 */
extern unsigned done_lock_pic;          /* DS:995a */
extern unsigned meld_button_pic;                /* DS:995c */
extern unsigned purify_button_pic;              /* DS:995e */
extern unsigned build_button_pic;               /* DS:9962 */
extern unsigned done_button_pic;                /* DS:9964 */
extern unsigned wait_button_pic;          /* DS:9966 */
extern unsigned patrol_button_pic;              /* DS:9968 */
extern unsigned topbar_spells_pic;              /* DS:996a */
extern unsigned topbar_info_pic;                /* DS:996c */
extern unsigned topbar_magic_pic;               /* DS:996e */
extern unsigned topbar_cities_pic;              /* DS:9970 */
extern unsigned topbar_game_pic;                /* DS:9972 */
extern unsigned topbar_armies_pic;              /* DS:9974 */
extern unsigned topbar_plane_pic;               /* DS:9976 */
extern unsigned move_type_icons[];              /* DS:9978 */
extern unsigned char far *p_terrain_minimap_colors;  /* DS:9ca8 */
extern char far *p_race_unrest_rows[];         /* DS:9ce0 */
extern unsigned big_city_wall_pic;              /* DS:9d18  CITYSCAP 0x72 'big city wall': the completed-walls popup */
extern unsigned world_block_rest;               /* DS:9d1c */
extern unsigned p_screen_gfx;                   /* DS:9d1e */
extern unsigned map_dashed_box_pic;                 /* DS:bd80  MAPBACK 0x59: a 6-frame blue dashed square, never drawn */
extern unsigned cityscape_vmask_pic;            /* DS:ca7e  CITYSCAP 0x6e: the roads' vertical mask (ReMoM cityscape_roads_vertical_mask_seg; clarified using ReMoM repo) */
extern unsigned cityscape_hmask_pic;            /* DS:ca80 */
extern unsigned map_warped_mask_pic;            /* DS:cb5e */
extern unsigned city_marker_pic;                /* DS:cb60 */
extern unsigned map_hunters_lodge_pic;          /* DS:cb62 */
extern unsigned map_nightshade_pic;             /* DS:cb64 */
extern unsigned map_wild_game_pic;              /* DS:cb66 */
extern unsigned map_site_mine_pic;              /* DS:cb68  MAPBACK 0x57 'mine' (SITES): a black square in 1.31; loaded, never drawn */
extern unsigned map_corruption_pic;             /* DS:cb6a */
extern unsigned map_site_mud_pic;               /* DS:cb6c  MAPBACK 0x4c 'mud' (SITES): a brown earth texture; loaded, never drawn */
extern unsigned map_mineral_pics[10];           /* DS:cb6e */
extern unsigned map_node_aura_pics[6];          /* DS:cb82 */
extern unsigned map_site_lumber_camp_pic;       /* DS:cb8e  MAPBACK 0x58 'lumber camp' (SITES): a black square in 1.31; loaded, never drawn */
extern unsigned node_aura_pic;                  /* DS:cb90 */
extern unsigned unit_banner_back_pics[6];       /* DS:cb92 */
extern unsigned map_eroad_pics[8];              /* DS:cba0 */
extern unsigned map_eroad_lone_pic;                 /* DS:cbb0 */
extern unsigned map_road_pics[8];               /* DS:cbb2 */
extern unsigned map_road_lone_pic;                 /* DS:cbc2 */
extern unsigned map_lair_pics[7];               /* DS:cbc4 */
extern unsigned map_tower_unowned_pic;          /* DS:cbd4 */
extern unsigned map_tower_owned_pic;            /* DS:cbd6 */
extern unsigned map_city_walled_pic;           /* DS:cbd8 */
extern unsigned map_city_unwalled_pic;           /* DS:cbda */
extern unsigned fog_edge_pics[16];              /* DS:cc02 */
extern struct TerrainGfx far *terrain_gfx_table;     /* DS:cc22 */
extern unsigned terrain_ems_handle;             /* DS:cc28 */
extern unsigned battlefield_blood_pics[];                /* DS:cfbe */
extern unsigned battlefield_curse_pics[];                /* DS:cfca */
extern unsigned battlefield_tile_anim_pics[];            /* DS:cfee */
extern unsigned battlefield_chaos_river_pics[];          /* DS:d006 */
extern unsigned battlefield_road_pics[];                 /* DS:d01e */
extern unsigned battlefield_water_pics[];                /* DS:d056 */
extern unsigned battlefield_chaos_water_pics[];          /* DS:d05e */
extern unsigned battlefield_flying_fortress_pic;         /* DS:d066 */
extern unsigned battlefield_floating_island_pic;         /* DS:d06e */
extern unsigned battlefield_water_pic;                   /* DS:d070 */
extern unsigned battlefield_sorcery_node_pic;                    /* DS:d072 */
extern unsigned battlefield_nature_node_pic;                 /* DS:d074  CMBTCITY 0x41 'GREENODE': drawn for the nature node backdrop (COMBSPRT.C) */
extern unsigned battlefield_volcano_pics[];              /* DS:d076 */
extern unsigned battlefield_wall_darkness_pics[];        /* DS:d088 */
extern unsigned battlefield_wall_fire_pics[];            /* DS:d0a4 */
extern unsigned wall_pics[][12];                /* DS:d0c0 */
extern unsigned battlefield_outpost_base_pic;            /* DS:d108 */
extern unsigned battlefield_city_base_pic;               /* DS:d10a */
extern unsigned battlefield_keep_pic;                 /* DS:d10c  CMBTCITY 0x16; it and the next seven are named by the backdrop they are drawn for (COMBSPRT.C) */
extern unsigned battlefield_fallen_temple_pic;                 /* DS:d10e */
extern unsigned battlefield_temple_pic;                 /* DS:d110 */
extern unsigned battlefield_ruins_pic;                 /* DS:d112 */
extern unsigned battlefield_tower_pic;                 /* DS:d114 */
extern unsigned battlefield_cave_pic;                 /* DS:d116 */
extern unsigned battlefield_outpost_pic;                 /* DS:d118 */
extern unsigned battlefield_fortress_pic;                 /* DS:d11a */
extern unsigned battlefield_house_pics[];            /* DS:d11c */
extern unsigned combat_effect_pic;              /* DS:d13a */
extern unsigned combat_vortex_pic;          /* DS:d13c */
extern unsigned battlefield_active_marker_pic;           /* DS:d188 */
extern unsigned battlefield_target_marker_pic;           /* DS:d18a */
extern unsigned battlefield_missile_pics[][8];           /* DS:d18c */
extern unsigned combat_red_burst_pic;                 /* DS:d27c  CMBTCITY 0x77: a 4-frame growing red burst, never drawn */

void Settings_Load_Or_Create(void);
void EMS_Cache_LBX_File_Required(char *name);
int EMS_Cache_LBX_File(char *name, int required);
unsigned LBX_Load(char *name, int entry);
unsigned LBX_Load_Records(char *name, int entry, unsigned first, unsigned count, unsigned size);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
unsigned Heap_Alloc_Block(unsigned paragraphs);
unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);

/* () at startup: the settings, the LBX files kept in EMS, then the permanent pictures and data */
void Load_Startup_Assets(void)
{
    Settings_Load_Or_Create();
    EMS_Cache_LBX_File_Required("MAIN.LBX");
    EMS_Cache_LBX_File_Required("BACKGRND.LBX");
    EMS_Cache_LBX_File_Required("UNITS1.LBX");
    EMS_Cache_LBX_File_Required("UNITS2.LBX");
    EMS_Cache_LBX_File_Required("UNITVIEW.LBX");
    EMS_Cache_LBX_File_Required("SPECIAL.LBX");
    EMS_Cache_LBX_File_Required("SPECIAL2.LBX");
    EMS_Cache_LBX_File_Required("ITEMS.LBX");
    EMS_Cache_LBX_File_Required("CHRIVER.LBX");
    EMS_Cache_LBX_File_Required("ITEMISC.LBX");
    EMS_Cache_LBX_File_Required("CITYSCAP.LBX");
    EMS_Cache_LBX_File_Required("MAPBACK.LBX");
    EMS_Cache_LBX_File_Required("CMBMAGIC.LBX");
    EMS_Cache_LBX_File_Required("CMBTCITY.LBX");
    EMS_Cache_LBX_File_Required("CITYWALL.LBX");
    Load_Map_Assets();
    Load_Removed_0466();
    Load_Main_Screen_Assets();
    Load_Removed_0af4();
    Load_Removed_0afe();
    Load_Removed_0aef();
    Load_Removed_0b03();
    Load_Backgrnd_Extra();
    Load_Removed_0b08();
    Load_Removed_0b35();
    Load_Removed_0b3a();
    Load_Spell_Data();
    Load_Building_Data();
    Load_Removed_0b99();
    Load_UI_Sounds();
    UnitTypes_Set_Upkeep_Early();
}

/* () the upkeep of every type before the summoned creatures = cost / 100 (heroes) or / 50 - overwritten later at the start of a game, so without a
   lasting effect; the loop bound is Magic Spirit's summoned type (the low word of its effect_param: UNIT_MAGIC_SPIRIT
   154), i.e. every type before the summoned creatures */
void UnitTypes_Set_Upkeep_Early(void)
{
    int unit_type;

    for (unit_type = 0; unit_type < (int)p_spell_data[SPELL_MAGIC_SPIRIT].effect_param; unit_type++) {
        if (unit_type < UNIT_HERO_CHOSEN + 1)
            unit_types[unit_type].upkeep = unit_types[unit_type].cost / 100;
        else
            unit_types[unit_type].upkeep = unit_types[unit_type].cost / 50;
    }
}

/* () TERRAIN.LBX (the terrain graphics table, the minimap colors, the graphics in EMS), TERRSTAT (the races' rebel
   tables, 14 bytes apart) and MAPBACK.LBX (fog edges, unit backdrops, cities, towers, lairs, minerals, specials,
   roads, node auras) */
void Load_Map_Assets(void)
{
    unsigned seg;
    int i;

    terrain_gfx_table = Seg_To_Far_Ptr(LBX_Load("TERRAIN.LBX", 1));
    city_marker_pic = Heap_Alloc_Block(0x46);
    p_terrain_minimap_colors = Seg_To_Far_Ptr(LBX_Load("TERRAIN.LBX", 2));
    terrain_ems_handle = EMS_Cache_LBX_File("TERRAIN.LBX", 1);
    seg = LBX_Load_Records("TERRSTAT", 1, 0, 1, 0xc4);
    p_race_unrest_rows[0] = Seg_To_Far_Ptr(seg);
    for (i = 1; i < 14; i++)
        p_race_unrest_rows[i] = p_race_unrest_rows[i - 1] + 14;
    for (i = 1; i < 15; i++)                    /* by exploration mask (EXPLORED_*): 15, all explored, needs none */
        fog_edge_pics[i] = LBX_Load("MAPBACK.LBX", i - 1);
    for (i = 0; i < 6; i++)
        unit_banner_back_pics[i] = LBX_Load("MAPBACK", i + 14);
    map_city_walled_pic = LBX_Load("MAPBACK", 0x14);
    map_city_unwalled_pic = LBX_Load("MAPBACK", 0x15);
    map_tower_unowned_pic = LBX_Load("MAPBACK", 0x45);
    map_tower_owned_pic = LBX_Load("MAPBACK", 0x46);
    map_lair_pics[0] = LBX_Load("MAPBACK", 0x47);   /* by lair type - LAIR_MYSTERIOUS_CAVE: cave, dungeon, ancient
                                                   temple, keep, monster lair, ruins, fallen temple */
    map_lair_pics[1] = LBX_Load("MAPBACK", 0x4a);
    map_lair_pics[2] = LBX_Load("MAPBACK", 0x48);
    map_lair_pics[3] = LBX_Load("MAPBACK", 0x49);
    map_lair_pics[4] = LBX_Load("MAPBACK", 0x47);
    map_lair_pics[5] = LBX_Load("MAPBACK", 0x4a);
    map_lair_pics[6] = LBX_Load("MAPBACK", 0x4b);
    for (i = 1; i < 10; i++)
        map_mineral_pics[i] = LBX_Load("MAPBACK", i + 0x4d);
    map_site_mud_pic = LBX_Load("MAPBACK", 0x4c);
    map_corruption_pic = LBX_Load("MAPBACK", 0x4d);
    map_site_mine_pic = LBX_Load("MAPBACK", 0x57);
    map_site_lumber_camp_pic = LBX_Load("MAPBACK", 0x58);
    map_road_lone_pic = LBX_Load("MAPBACK", 0x2d);
    for (i = 0; i < 8; i++)
        map_road_pics[i] = LBX_Load("MAPBACK", i + 0x2e);
    map_eroad_lone_pic = LBX_Load("MAPBACK", 0x36);
    for (i = 0; i < 8; i++)
        map_eroad_pics[i] = LBX_Load("MAPBACK", i + 0x37);
    node_aura_pic = Heap_Alloc_Block(0x18);
    for (i = 0; i < 5; i++)
        map_node_aura_pics[i] = LBX_Load("MAPBACK", i + 0x3f);
    map_dashed_box_pic = LBX_Load("MAPBACK", 0x59);
    map_hunters_lodge_pic = LBX_Load("MAPBACK", 0x5a);
    map_nightshade_pic = LBX_Load("MAPBACK", 0x5b);
    map_wild_game_pic = LBX_Load("MAPBACK", 0x5c);
    map_warped_mask_pic = LBX_Load("MAPBACK", 0x5d);
}

/* () empty. The Load_Removed_<code offset> functions are loaders emptied before release; Load_Startup_Assets still
   calls most of them, between the loaders that remain */
void Load_Removed_0466(void)
{
}

/* () MAIN.LBX: the main screen, its top bar and order buttons, medals, weapon marks, move type icons, the stack
   slots, the Next Turn / cast / road / survey panels, the Go-To marker, the event icons; BACKGRND 4 */
void Load_Main_Screen_Assets(void)
{
    int i;

    main_screen_bg_pic = LBX_Load("MAIN", 0);
    topbar_game_pic = LBX_Load("MAIN", 1);
    topbar_spells_pic = LBX_Load("MAIN", 2);
    topbar_armies_pic = LBX_Load("MAIN", 3);
    topbar_cities_pic = LBX_Load("MAIN", 4);
    topbar_magic_pic = LBX_Load("MAIN", 5);
    topbar_info_pic = LBX_Load("MAIN", 6);
    topbar_plane_pic = LBX_Load("MAIN", 7);
    done_button_pic = LBX_Load("MAIN", 8);
    patrol_button_pic = LBX_Load("MAIN", 9);
    wait_button_pic = LBX_Load("MAIN", 10);
    build_button_pic = LBX_Load("MAIN", 0xb);
    purify_button_pic = LBX_Load("MAIN", 0x2a);
    meld_button_pic = LBX_Load("MAIN", 0x31);
    done_lock_pic = LBX_Load("MAIN", 0xc);
    patrol_lock_pic = LBX_Load("MAIN", 0xd);
    wait_lock_pic = LBX_Load("MAIN", 0xe);
    special_order_blank_pic = LBX_Load("MAIN", 0xf);
    purify_lock_pic = LBX_Load("MAIN", 0x2b);
    medal_white_pic = LBX_Load("MAIN", 0x33);
    medal_gold_pic = LBX_Load("MAIN", 0x34);
    medal_red_pic = LBX_Load("MAIN", 0x35);
    weapon_magic_pic = LBX_Load("MAIN", 0x36);
    weapon_mithril_pic = LBX_Load("MAIN", 0x37);
    weapon_adamantium_pic = LBX_Load("MAIN", 0x38);
    move_type_icons[MOVE_ICON_SAIL] = LBX_Load("MAIN", 0x12);
    move_type_icons[MOVE_ICON_SWIM] = LBX_Load("MAIN", 0x13);
    move_type_icons[MOVE_ICON_MOUNTAIN] = LBX_Load("MAIN", 0x14);
    move_type_icons[MOVE_ICON_FOREST] = LBX_Load("MAIN", 0x15);
    move_type_icons[MOVE_ICON_FLY] = LBX_Load("MAIN", 0x16);
    move_type_icons[MOVE_ICON_PATH] = LBX_Load("MAIN", 0x17);
    move_type_icons[MOVE_ICON_ASTRAL] = LBX_Load("MAIN", 0x24);
    move_type_icons[MOVE_ICON_WIND] = LBX_Load("MAIN", 0x25);
    move_type_icons[MOVE_ICON_BOOT] = LBX_Load("MAIN", 0x26);
    for (i = 0; i < MAX_STACK; i++)
        stack_slot_bg_pics[i] = LBX_Load("MAIN", i + 0x18);
    next_turn_button_pic = LBX_Load("MAIN", 0x3a);
    deselect_bg_pic = LBX_Load("MAIN", 0x22);
    next_turn_bg_pic = LBX_Load("MAIN", 0x23);
    cast_bg_pic = LBX_Load("MAIN", 0x28);
    main_cancel_button_pic = LBX_Load("MAIN", 0x29);
    cast_button_border_pic = LBX_Load("MAIN", 0x2f);
    button_blockout_pic = LBX_Load("MAIN", 0x2c);
    road_bg_pic = LBX_Load("MAIN", 0x2d);
    road_ok_pic = LBX_Load("MAIN", 0x2e);
    road_button_border_pic = LBX_Load("MAIN", 0x30);
    survey_bg_pic = LBX_Load("MAIN", 0x39);
    wizinfo_bg_pic = LBX_Load("BACKGRND", 4);
    goto_marker_pic = LBX_Load("MAIN", 0x32);
    for (i = 0; i < 6; i++)
        event_icon_pics[i] = LBX_Load("MAIN", i + 0x3b);
}

/* () BACKGRND.LBX into p_screen_gfx: panels, the city enchantment arrows, OK, the workers / farmers / rebels by race,
   the resource icons, the outpost screen, the lair dialog */
void Load_Backgrnd_UI_Assets(void)
{
    int i;

    city_screen_bg_pic = LBX_Reload_Next("BACKGRND", 6, p_screen_gfx);
    city_buy_button_pic = LBX_Reload_Next("BACKGRND", 7, p_screen_gfx);
    city_change_button_pic = LBX_Reload_Next("BACKGRND", 8, p_screen_gfx);
    city_ok_button_pic = LBX_Reload_Next("BACKGRND", 9, p_screen_gfx);
    city_done_backdrop_pic = LBX_Reload_Next("BACKGRND", 10, p_screen_gfx);
    prod_box_empty_pic = LBX_Reload_Next("BACKGRND", 0xb, p_screen_gfx);
    prod_box_filled_pic = LBX_Reload_Next("BACKGRND", 0xc, p_screen_gfx);
    city_no_product_pic = LBX_Reload_Next("BACKGRND", 0xd, p_screen_gfx);
    city_buy_greyed_pic = LBX_Reload_Next("BACKGRND", 0xe, p_screen_gfx);
    city_ench_up_arrow_pic = LBX_Reload_Next("BACKGRND", 0xf, p_screen_gfx);
    city_ench_down_arrow_pic = LBX_Reload_Next("BACKGRND", 0x10, p_screen_gfx);
    ok_button_pic = LBX_Reload_Next("BACKGRND", 0x18, p_screen_gfx);
    for (i = 0; i < 14; i++)
        city_worker_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x2d, p_screen_gfx);
    for (i = 0; i < 14; i++)
        city_farmer_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x3b, p_screen_gfx);
    for (i = 0; i < 14; i++)
        city_rebel_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x4a, p_screen_gfx);
    for (i = 0; i < 5; i++)
        resource_x1_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x28, p_screen_gfx);
    food_deficit_x1_pic = LBX_Reload_Next("BACKGRND", 0x5e, p_screen_gfx);
    for (i = 0; i < 6; i++)
        resource_x10_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x58, p_screen_gfx);
    food_deficit_x10_pic = LBX_Reload_Next("BACKGRND", 0x5f, p_screen_gfx);
    outpost_bg_pic = LBX_Reload_Next("BACKGRND", 0x20, p_screen_gfx);
    name_entry_box_pic = LBX_Reload_Next("BACKGRND", 0x21, p_screen_gfx);
    for (i = 0; i < 3; i++)
        outpost_house_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x22, p_screen_gfx);
    for (i = 0; i < 3; i++)
        outpost_house_empty_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x25, p_screen_gfx);
    lair_confirm_border_pic = LBX_Reload_Next("BACKGRND", 0x19, p_screen_gfx);
    lair_bottom_button_pic = LBX_Reload_Next("BACKGRND", 0x1a, p_screen_gfx);
    lair_bottom_nobutton_pic = LBX_Reload_Next("BACKGRND", 0x1b, p_screen_gfx);
    city_shared_tile_pic = LBX_Reload_Next("BACKGRND", 0, p_screen_gfx);
}

/* () empty */
void Load_Removed_0aef(void)
{
}

/* () empty */
void Load_Removed_0af4(void)
{
}

/* () empty */
void Load_Removed_0af9(void)
{
}

/* () empty */
void Load_Removed_0afe(void)
{
}

/* () empty */
void Load_Removed_0b03(void)
{
}

/* () empty */
void Load_Removed_0b08(void)
{
}

/* () SOUNDFX.LBX: the message sound (0) and the click (2) */
void Load_UI_Sounds(void)
{
    snd_message = LBX_Load("SOUNDFX", 0);
    snd_click = LBX_Load("SOUNDFX", 2);
}

/* () empty */
void Load_Removed_0b35(void)
{
}

/* () empty */
void Load_Removed_0b3a(void)
{
}

/* () SPELLDAT.LBX: the 215 spell records */
void Load_Spell_Data(void)
{
    p_spell_data = Seg_To_Far_Ptr(LBX_Load_Records("SPELLDAT.LBX", 0, 0, NUM_SPELLS, sizeof(struct Spell)));
}

/* () BUILDDAT.LBX: the 36 building records */
void Load_Building_Data(void)
{
    p_building_data = Seg_To_Far_Ptr(LBX_Load_Records("BUILDDAT.LBX", 0, 0, NUM_BUILDINGS, sizeof(struct Building)));
}

/* () empty */
void Load_Removed_0b99(void)
{
}

/* () BACKGRND 3: the combat screen border */
void Load_Backgrnd_Extra(void)
{
    combat_border_pic = LBX_Load("BACKGRND", 3);
}

/* () empties the per-screen picture block */
void Screen_Gfx_Reset(void)
{
    Heap_Alloc_Front(p_screen_gfx, 1);
}

/* () empties the block after the world data (world_block_rest), as Screen_Gfx_Reset does p_screen_gfx; no caller */
void World_Rest_Reset(void)
{
    Heap_Alloc_Front(world_block_rest, 1);
}

/* () the map figures of the 198 unit types (UNITS1.LBX 0..119, UNITS2.LBX) */
void Load_Unit_Figure_Sets(void)
{
    int unit_type;

    for (unit_type = 0; unit_type < 120; unit_type++)
        unit_types[unit_type].map_figure_pic = LBX_Reload_Next("UNITS1", unit_type, p_screen_gfx);
    for (unit_type = 120; unit_type < 198; unit_type++)
        unit_types[unit_type].map_figure_pic = LBX_Reload_Next("UNITS2", unit_type - 120, p_screen_gfx);
}

/* () the battlefield: missiles (CMBMAGIC), city / node / water / road / animated tiles, blood (CMBTCITY), the chaos
   rivers, volcanoes and curses (CHRIVER), the walls of fire / darkness and the city walls (CITYWALL) */
void Load_Battlefield_Art(void)
{
    int i;
    int j;

    for (i = 0; i < 15; i++)
        for (j = 0; j < 8; j++)
            battlefield_missile_pics[i][j] = LBX_Reload_Next("CMBMAGIC", i * 8 + j, p_screen_gfx);
    combat_vortex_pic = LBX_Reload_Next("CMBMAGIC", 0x78, p_screen_gfx);
    for (i = 0; i < 5; i++)
        battlefield_blood_pics[i] = LBX_Reload_Next("CMBTCITY", i + 0x18, p_screen_gfx);
    combat_red_burst_pic = LBX_Reload_Next("CMBTCITY", 0x77, p_screen_gfx);
    combat_effect_pic = LBX_Reload_Next("CMBTCITY", 0x78, p_screen_gfx);
    for (i = 0; i < 15; i++)
        battlefield_house_pics[i] = LBX_Reload_Next("CMBTCITY", i + 2, p_screen_gfx);
    battlefield_fortress_pic = LBX_Reload_Next("CMBTCITY", 0x11, p_screen_gfx);
    battlefield_outpost_pic = LBX_Reload_Next("CMBTCITY", 0x12, p_screen_gfx);
    battlefield_cave_pic = LBX_Reload_Next("CMBTCITY", 0x13, p_screen_gfx);
    battlefield_tower_pic = LBX_Reload_Next("CMBTCITY", 0x14, p_screen_gfx);
    battlefield_ruins_pic = LBX_Reload_Next("CMBTCITY", 0x15, p_screen_gfx);
    battlefield_keep_pic = LBX_Reload_Next("CMBTCITY", 0x16, p_screen_gfx);
    battlefield_temple_pic = LBX_Reload_Next("CMBTCITY", 0x17, p_screen_gfx);
    battlefield_city_base_pic = LBX_Reload_Next("CMBTCITY", 0, p_screen_gfx);
    battlefield_outpost_base_pic = LBX_Reload_Next("CMBTCITY", 1, p_screen_gfx);
    battlefield_fallen_temple_pic = LBX_Reload_Next("CMBTCITY", 0x79, p_screen_gfx);
    battlefield_nature_node_pic = LBX_Reload_Next("CMBTCITY", 0x41, p_screen_gfx);
    battlefield_sorcery_node_pic = LBX_Reload_Next("CMBTCITY", 0x42, p_screen_gfx);
    battlefield_target_marker_pic = LBX_Reload_Next("CMBTCITY", 0x43, p_screen_gfx);
    battlefield_active_marker_pic = LBX_Reload_Next("CMBTCITY", 0x44, p_screen_gfx);
    battlefield_water_pic = LBX_Reload_Next("CMBTCITY", 0x76, p_screen_gfx);
    battlefield_floating_island_pic = LBX_Reload_Next("CMBTCITY", 0x75, p_screen_gfx);
    battlefield_flying_fortress_pic = LBX_Reload_Next("CMBTCITY", 0x71, p_screen_gfx);
    for (i = 0; i < 4; i++)
        battlefield_water_pics[i] = LBX_Reload_Next("CMBTCITY", i + 0x6d, p_screen_gfx);
    for (i = 0; i < 28; i++)
        battlefield_road_pics[i] = LBX_Reload_Next("CMBTCITY", i + 0x45, p_screen_gfx);
    for (i = 0; i < 12; i++)
        battlefield_tile_anim_pics[i] = LBX_Reload_Next("CMBTCITY", i + 0x61, p_screen_gfx);
    for (i = 0; i < 12; i++)
        battlefield_chaos_river_pics[i] = LBX_Reload_Next("CHRIVER", i, p_screen_gfx);
    for (i = 0; i < 4; i++)
        battlefield_chaos_water_pics[i] = LBX_Reload_Next("CHRIVER", i + 0xc, p_screen_gfx);
    for (i = 0; i < 9; i++)
        battlefield_volcano_pics[i] = LBX_Reload_Next("CHRIVER", i + 0x18, p_screen_gfx);
    for (i = 0; i < 14; i++)
        battlefield_wall_fire_pics[i] = LBX_Reload_Next("CITYWALL", i + 0x24, p_screen_gfx);
    for (i = 0; i < 14; i++)
        battlefield_wall_darkness_pics[i] = LBX_Reload_Next("CITYWALL", i + 0x32, p_screen_gfx);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 12; j++)
            wall_pics[i][j] = LBX_Reload_Next("CITYWALL", i * 12 + j, p_screen_gfx);
    for (i = 0; i < 8; i++)
        battlefield_curse_pics[i] = LBX_Reload_Next("CHRIVER", i + 0x10, p_screen_gfx);
}

/* () empty */
void Load_Removed_1046(void)
{
}

/* () the cityscape (CITYSCAP.LBX): backgrounds (normal and chaos), wards and enchantments, the buildings by
   BuildingId, houses / trees / rocks by house style, rubble and the masks; UNITVIEW 0x22, CITYSCAP 0x67 and BACKGRND 0x49
   (all loaded again by Load_UnitView_Art, as are the resource icons: the variables keep the last load) */
void Load_Cityscape_Art(void)
{
    int i;
    int j;

    cityscape_arcanus_ground_pic = LBX_Reload_Next("CITYSCAP", 0, p_screen_gfx);
    cityscape_flying_fortress_pic = LBX_Reload_Next("CITYSCAP", 0x57, p_screen_gfx);
    cityscape_gaia_pic = LBX_Reload_Next("CITYSCAP", 0x5a, p_screen_gfx);
    cityscape_living_lands_pic = LBX_Reload_Next("CITYSCAP", 0x59, p_screen_gfx);
    cityscape_cursed_land_pic = LBX_Reload_Next("CITYSCAP", 0x56, p_screen_gfx);
    cityscape_river_pic = LBX_Reload_Next("CITYSCAP", 3, p_screen_gfx);
    cityscape_ocean_pic = LBX_Reload_Next("CITYSCAP", 4, p_screen_gfx);
    cityscape_hills_bg_pic = LBX_Reload_Next("CITYSCAP", 1, p_screen_gfx);
    cityscape_mountain_bg_pic = LBX_Reload_Next("CITYSCAP", 2, p_screen_gfx);
    cityscape_far_dark_cloud_pic = LBX_Reload_Next("CITYSCAP", 0x5b, p_screen_gfx);
    cityscape_far_heavenly_light_pic = LBX_Reload_Next("CITYSCAP", 0x5d, p_screen_gfx);
    cityscape_far_chaos_rift_pic = LBX_Reload_Next("CITYSCAP", 0x5c, p_screen_gfx);
    cityscape_famine_pic = LBX_Reload_Next("CITYSCAP", 0x58, p_screen_gfx);
    cityscape_plain_bg_pic = LBX_Reload_Next("CITYSCAP", 7, p_screen_gfx);
    cityscape_myrror_ground_pic = LBX_Reload_Next("CITYSCAP", 8, p_screen_gfx);
    cityscape_myrror_hills_pic = LBX_Reload_Next("CITYSCAP", 9, p_screen_gfx);
    cityscape_myrror_mountain_pic = LBX_Reload_Next("CITYSCAP", 10, p_screen_gfx);
    cityscape_myrror_plains_pic = LBX_Reload_Next("CITYSCAP", 0xb, p_screen_gfx);
    cityscape_myrror_heavenly_light_pic = LBX_Reload_Next("CITYSCAP", 0x71, p_screen_gfx);
    cityscape_myrror_dark_cloud_pic = LBX_Reload_Next("CITYSCAP", 0x6f, p_screen_gfx);
    cityscape_myrror_chaos_rift_pic = LBX_Reload_Next("CITYSCAP", 0x70, p_screen_gfx);
    cityscape_myrror_river_pic = LBX_Reload_Next("CITYSCAP", 0x73, p_screen_gfx);
    cityscape_myrror_ocean_pic = LBX_Reload_Next("CITYSCAP", 0x74, p_screen_gfx);
    cityscape_death_ward_pic = LBX_Reload_Next("CITYSCAP", 0x5e, p_screen_gfx);
    cityscape_chaos_ward_pic = LBX_Reload_Next("CITYSCAP", 0x5f, p_screen_gfx);
    cityscape_nature_ward_pic = LBX_Reload_Next("CITYSCAP", 0x62, p_screen_gfx);
    cityscape_life_ward_pic = LBX_Reload_Next("CITYSCAP", 0x60, p_screen_gfx);
    cityscape_sorcery_ward_pic = LBX_Reload_Next("CITYSCAP", 0x61, p_screen_gfx);
    cityscape_natures_eye_pic = LBX_Reload_Next("CITYSCAP", 99, p_screen_gfx);
    cityscape_inspirations_pic = LBX_Reload_Next("CITYSCAP", 100, p_screen_gfx);
    cityscape_prosperity_pic = LBX_Reload_Next("CITYSCAP", 0x65, p_screen_gfx);
    cityscape_consecration_pic = LBX_Reload_Next("CITYSCAP", 0x66, p_screen_gfx);
    cityscape_wall_of_fire_pic = LBX_Reload_Next("CITYSCAP", 0x4d, p_screen_gfx);
    cityscape_wall_of_darkness_pic = LBX_Reload_Next("CITYSCAP", 0x4f, p_screen_gfx);
    for (i = 3; i <= 30; i++)
        building_pics[i] = LBX_Reload_Next("CITYSCAP", i + 0x2a, p_screen_gfx);
    building_pics[31] = LBX_Reload_Next("CITYSCAP", 0x4e, p_screen_gfx);
    for (i = 32; i <= 34; i++)
        building_pics[i] = LBX_Reload_Next("CITYSCAP", i + 0x29, p_screen_gfx);
    cityscape_roads_pic = LBX_Reload_Next("CITYSCAP", 5, p_screen_gfx);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 5; j++)
            cityscape_house_pics[i][j] = LBX_Reload_Next("CITYSCAP", i * 5 + j + 0x19, p_screen_gfx);
    for (i = 0; i < 3; i++)
        cityscape_tree_pics[i] = LBX_Reload_Next("CITYSCAP", i + 0x13, p_screen_gfx);
    for (i = 0; i < 3; i++)
        cityscape_rocks_pics[i] = LBX_Reload_Next("CITYSCAP", i + 0x16, p_screen_gfx);
    cityscape_summon_circle_pic = LBX_Reload_Next("CITYSCAP", 6, p_screen_gfx);
    cityscape_earth_gate_pic = LBX_Reload_Next("CITYSCAP", 0x53, p_screen_gfx);
    cityscape_stream_of_life_pic = LBX_Reload_Next("CITYSCAP", 0x54, p_screen_gfx);
    cityscape_astral_gate_pic = LBX_Reload_Next("CITYSCAP", 0x55, p_screen_gfx);
    cityscape_altar_pic = LBX_Reload_Next("CITYSCAP", 0xc, p_screen_gfx);
    cityscape_dark_rituals_pic = LBX_Reload_Next("CITYSCAP", 0x51, p_screen_gfx);
    cityscape_evil_presence_pic = LBX_Reload_Next("CITYSCAP", 0x52, p_screen_gfx);
    cityscape_city_walls_pic = LBX_Reload_Next("CITYSCAP", 0x4c, p_screen_gfx);
    cityscape_fortress_pic = LBX_Reload_Next("CITYSCAP", 0x28, p_screen_gfx);
    cityscape_trade_goods_pic = LBX_Reload_Next("CITYSCAP", 0x29, p_screen_gfx);
    cityscape_housing_pics[0] = LBX_Reload_Next("CITYSCAP", 0x2a, p_screen_gfx);
    cityscape_housing_pics[1] = LBX_Reload_Next("CITYSCAP", 0x2b, p_screen_gfx);
    cityscape_housing_pics[2] = LBX_Reload_Next("CITYSCAP", 0x2c, p_screen_gfx);
    unitview_water_bg_pic = LBX_Reload_Next("UNITVIEW", 0x22, p_screen_gfx);
    unitview_land_bg_pic = LBX_Reload_Next("CITYSCAP", 0x67, p_screen_gfx);
    building_pics[BLD_CITY_WALLS] = LBX_Reload_Next("CITYSCAP", 0x68, p_screen_gfx);   /* 'city walls build' */
    for (i = 0; i < 4; i++)
        cityscape_rubble_pics[i] = LBX_Reload_Next("CITYSCAP", i + 0x69, p_screen_gfx);
    cityscape_hmask_pic = LBX_Reload_Next("CITYSCAP", 0x6d, p_screen_gfx);
    cityscape_vmask_pic = LBX_Reload_Next("CITYSCAP", 0x6e, p_screen_gfx);
    big_city_wall_pic = LBX_Reload_Next("CITYSCAP", 0x72, p_screen_gfx);
    gold_deficit_x1_pic = LBX_Reload_Next("BACKGRND", 0x49, p_screen_gfx);
}

/* () empty (the diplomacy screen's loader) */
void Load_Diplomacy_Removed(void)
{
}

/* () the unit view: SPECIAL / SPECIAL2 ability icons, UNITVIEW frames, arrows, stat icons, the unit list, the city
   production tabs; BACKGRND OK, gold coin and resource icons
   Numbered: BUG #404 (BUGS_AND_QUIRKS.txt) */
void Load_UnitView_Art(void)
{
    int i;

    for (i = 0; i < 111; i++)
        special_icons[i] = LBX_Reload_Next("SPECIAL", i, p_screen_gfx);
    for (i = 0; i < 34; i++)                    /* BUG #404: 34 pictures into the 33-word special2_icons: the last lands
                                                   in the next word (DS:9918, diplo_mirror_pic[0]) */
        special2_icons[i] = LBX_Reload_Next("SPECIAL2", i, p_screen_gfx);
    unitview_build_bg_pic = LBX_Reload_Next("UNITVIEW", 0, p_screen_gfx);
    unitview_bg_pic = LBX_Reload_Next("UNITVIEW", 1, p_screen_gfx);
    unitview_button_panel_pic = LBX_Reload_Next("UNITVIEW", 2, p_screen_gfx);
    unitview_arrow_up_pic = LBX_Reload_Next("UNITVIEW", 3, p_screen_gfx);
    unitview_arrow_down_pic = LBX_Reload_Next("UNITVIEW", 4, p_screen_gfx);
    for (i = 0; i < 8; i++)
        unitview_row_frame_pics[i] = LBX_Reload_Next("UNITVIEW", i + 5, p_screen_gfx);
    for (i = 0; i < 15; i++)
        unitview_stat_icons[i] = LBX_Reload_Next("UNITVIEW", i + 0xd, p_screen_gfx);
    for (i = 0; i < 15; i++)
        unitview_stat_icons_gold[i] = LBX_Reload_Next("UNITVIEW", i + 0x23, p_screen_gfx);
    unitlist_bg_pic = LBX_Reload_Next("UNITVIEW", 0x1c, p_screen_gfx);
    unitlist_bottom_pic = LBX_Reload_Next("UNITVIEW", 0x1d, p_screen_gfx);
    unitview_stone_frame_pic = LBX_Reload_Next("UNITVIEW", 0x1e, p_screen_gfx);
    ok_button_pic = LBX_Reload_Next("BACKGRND", 0x18, p_screen_gfx);
    gold_deficit_x1_pic = LBX_Reload_Next("BACKGRND", 0x49, p_screen_gfx);
    gold_coin_pic = LBX_Reload_Next("BACKGRND", 0x2a, p_screen_gfx);
    cityprod_left_tab_pic = LBX_Reload_Next("UNITVIEW", 0x1f, p_screen_gfx);
    cityprod_right_tab_pic = LBX_Reload_Next("UNITVIEW", 0x20, p_screen_gfx);
    unitview_portrait_frame_pic = LBX_Reload_Next("UNITVIEW", 0x21, p_screen_gfx);
    unitview_water_bg_pic = LBX_Reload_Next("UNITVIEW", 0x22, p_screen_gfx);
    unitview_land_bg_pic = LBX_Reload_Next("CITYSCAP", 0x67, p_screen_gfx);
    for (i = 0; i < 5; i++)
        resource_x1_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x28, p_screen_gfx);
    food_deficit_x1_pic = LBX_Reload_Next("BACKGRND", 0x5e, p_screen_gfx);
    for (i = 0; i < 6; i++)
        resource_x10_pics[i] = LBX_Reload_Next("BACKGRND", i + 0x58, p_screen_gfx);
    food_deficit_x10_pic = LBX_Reload_Next("BACKGRND", 0x5f, p_screen_gfx);
}

/* () ITEMS.LBX 0..115 (item pictures) and ITEMISC (power icons, empty slots per slot type, the item card and bullet,
   the slot icons) */
void Item_Graphics_Load(void)
{
    int i;

    for (i = 0; i < 116; i++)
        item_icon_pics[i] = LBX_Reload_Next("ITEMS", i, p_screen_gfx);
    for (i = 0; i < 19; i++)
        item_power_icons[i] = LBX_Reload_Next("ITEMISC", i, p_screen_gfx);
    item_icon_dispel_evil_pic = LBX_Reload_Next("ITEMISC", 0x21, p_screen_gfx);
    item_icon_giant_strength_pic = LBX_Reload_Next("ITEMISC", 0x22, p_screen_gfx);
    item_icon_guardian_wind_pic = LBX_Reload_Next("ITEMISC", 0x23, p_screen_gfx);
    item_empty_slot_pics[0] = LBX_Reload_Next("ITEMISC", 0x13, p_screen_gfx);
    item_empty_slot_pics[1] = LBX_Reload_Next("ITEMISC", 0x14, p_screen_gfx);
    item_empty_slot_pics[2] = LBX_Reload_Next("ITEMISC", 0x15, p_screen_gfx);
    item_empty_slot_pics[3] = LBX_Reload_Next("ITEMISC", 0x16, p_screen_gfx);
    item_empty_slot_pics[4] = LBX_Reload_Next("ITEMISC", 0x18, p_screen_gfx);
    item_empty_slot_pics[5] = LBX_Reload_Next("ITEMISC", 0x17, p_screen_gfx);
    item_card_pic = LBX_Reload_Next("ITEMISC", 0x19, p_screen_gfx);
    item_power_bullet_pic = LBX_Reload_Next("ITEMISC", 0x1a, p_screen_gfx);
    unitview_slot_icons[0] = LBX_Reload_Next("ITEMISC", 0x1b, p_screen_gfx);
    unitview_slot_icons[1] = LBX_Reload_Next("ITEMISC", 0x1c, p_screen_gfx);
    unitview_slot_icons[2] = LBX_Reload_Next("ITEMISC", 0x1d, p_screen_gfx);
    unitview_slot_icons[3] = LBX_Reload_Next("ITEMISC", 0x1e, p_screen_gfx);
    unitview_slot_icons[4] = LBX_Reload_Next("ITEMISC", 0x20, p_screen_gfx);
    unitview_slot_icons[5] = LBX_Reload_Next("ITEMISC", 0x1f, p_screen_gfx);
}

/* () SPECIAL2.LBX 0x22..0x2b: the small spellbook, its corners and page pictures
   role name corrected to ReMoM's reading (10/17 audit); clarified using ReMoM repo */
void Spellbook_Load_Small_Pictures(void)
{
    int i;

    spellbook_small_pic = LBX_Reload_Next("special2", 0x22, p_screen_gfx);
    spellbook_corner_left_pic = LBX_Reload_Next("special2", 0x23, p_screen_gfx);
    spellbook_corner_right_pic = LBX_Reload_Next("special2", 0x24, p_screen_gfx);
    for (i = 0; i <= 5; i++)
        spellbook_realm_icon_src_pics[i] = LBX_Reload_Next("special2", i + 0x25, p_screen_gfx);
    spellbook_page_bg_pic = LBX_Reload_Next("special2", 0x2b, p_screen_gfx);
}

/* () refills the screen-graphics block (Screen_Gfx_Reset) with what the overland map and the screens opened from it
   draw: figures, items, the cityscape, BACKGRND, the small spellbook - after combat, diplomacy and the production
   screen */
void Load_Overland_Screen_Graphics(void)
{
    Screen_Gfx_Reset();
    Load_Unit_Figure_Sets();
    Item_Graphics_Load();
    Load_Cityscape_Art();
    Load_Backgrnd_UI_Assets();
    Spellbook_Load_Small_Pictures();
}

/* () the unit view's pictures */
void Load_Unit_View_Graphics(void)
{
    Screen_Gfx_Reset();
    Load_UnitView_Art();
    Item_Graphics_Load();
    Load_Cityscape_Art();
}

/* () the stack popup's pictures */
void Load_Stack_Popup_Art(void)
{
    Screen_Gfx_Reset();
    Load_Unit_Figure_Sets();
    Load_UnitView_Art();
}

/* () the diplomacy screen's (an empty loader) */
void Load_Diplomacy_Art(void)
{
    Screen_Gfx_Reset();
    Load_Diplomacy_Removed();
}

/* () the battlefield's pictures */
void Load_Combat_Graphics(void)
{
    Screen_Gfx_Reset();
    Load_Battlefield_Art();
    Spellbook_Load_Small_Pictures();
}
