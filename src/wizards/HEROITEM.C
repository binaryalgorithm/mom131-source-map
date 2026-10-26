/* HEROITEM.C - WIZARDS.EXE overlay code segment 87aa (stub 252d): the items of heroes who died - the human gets the
   item award screen, an AI re-equips its living heroes from the pool (the best item allowed by each slot, then the
   empty slots) and sells the rest for mana; and freeing an item record.
   This module's data (DS:5aea): its literal ("message"); BSS (DS:c7fc): message_text (owner not fixed by the EXE).
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 87aa src/wizards/HEROITEM.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Hero_Items_Redistribute(int player, int count, int *items);
void Item_Free(int item);
void Hero_Items_AI_Reequip(int player, int count, int *items);
void Hero_Items_AI_Equip_Best(int player, int hero_slot, int *pool, int *count);
void Hero_Items_AI_Fill_Empty(int player, int hero_slot, int *pool, int *count);

#include "GAME.H"

char message_text[150];                         /* DS:c7fc  a MESSAGE.LBX record (0x96 bytes) for Message_Box; owner not fixed by the EXE: any module between FINSCORE.C and COMBAUTO.C gives the same bytes */
                                                /* also: the item forge's item name under edit (ITEMFORG.C) */

void Minimap_Alloc_Pic(void);
void Map_Full_Redraw(void);
void Map_Redraw_Dissolve(void);
void Items_Screen_Award(int count, int *items);
void Swap_Words(int *a, int *b);
void Wizard_Add_Mana(int player, int amount);
int Array_Delete_Near(int index, char *array, int record_size, int count);

/* (player, count, items) the items of heroes who died go to the winner: the human (0) gets the message "At least
   one hero has died..." (MESSAGE.LBX 0x43) and the item award screen (back to the main screen); an AI re-equips */
void Hero_Items_Redistribute(int player, int count, int *items)
{
    if (count > 0) {
        if (player == PLAYER_HUMAN) {
            Minimap_Alloc_Pic();
            Map_Full_Redraw();
            LBX_Load_Records_Near("message", 0, message_text, 0x43, 1, 0x96);
            Message_Box(message_text);
            Map_Redraw_Dissolve();
            Items_Screen_Award(count, items);
            current_screen = SCREEN_MAP;
            Map_Redraw_Dissolve();
        } else
            Hero_Items_AI_Reequip(player, count, items);
    }
}

/* (item) cost 0 frees the record (p_items_in_game is not cleared). A record with cost <= 0 is free: ITEMSCR.C
   breaks items with -1, Item_Alloc_Copy tests <= 0
   Numbered: BUG #79 (BUGS_AND_QUIRKS.txt) */
void Item_Free(int item)
{
    p_items[item].cost = 0;
}

/* (player, count, items) the neutral player (5) frees them; else the items are pooled with every item of the
   player's living heroes (their slots emptied), the heroes sorted by unit type (highest first) take the best item
   per slot (Hero_Items_AI_Equip_Best), then fill their empty slots (Hero_Items_AI_Fill_Empty); every item left is
   sold for half its cost in mana (an AI never uses the vault) */
void Hero_Items_AI_Reequip(int player, int count, int *items)
{
    int pool_count;
    int k;
    int hero_count;
    int pool[40];
    int heroes[6];
    int i;

    if (player == PLAYER_NEUTRAL) {
        for (i = 0; i < count; i++)
            Item_Free(items[i]);
    } else {
        hero_count = 0;
        pool_count = 0;
        for (i = 0; i < count; i++) {
            pool[pool_count] = items[i];
            pool_count++;
        }
        for (i = 0; i < 6; i++) {
            if (wizards[player].hired_heroes[i].unit_index > -1) {
                heroes[hero_count] = wizards[player].hired_heroes[i].unit_index;
                hero_count++;
                for (k = 0; k < 3; k++) {
                    if (wizards[player].hired_heroes[i].item_index[k] > -1) {
                        pool[pool_count] = wizards[player].hired_heroes[i].item_index[k];
                        pool_count++;
                        wizards[player].hired_heroes[i].item_index[k] = -1;
                    }
                }
            }
        }
        if (hero_count > 1) {
            for (k = 1; k < hero_count; k++)
                for (i = 0; i < hero_count - k; i++)
                    if (p_units[heroes[i]].type < p_units[heroes[i + 1]].type)
                        Swap_Words(&heroes[i], &heroes[i + 1]);
        }
        for (i = 0; i < hero_count; i++)
            Hero_Items_AI_Equip_Best(player, p_units[heroes[i]].hero_slot, pool, &pool_count);
        for (i = 0; i < hero_count; i++)
            Hero_Items_AI_Fill_Empty(player, p_units[heroes[i]].hero_slot, pool, &pool_count);
        for (i = 0; i < pool_count; i++) {
            Wizard_Add_Mana(player, p_items[pool[i]].cost / 2);
            Item_Free(pool[i]);
        }
    }
}

/* (player, hero slot, pool, &count) each of the hero's 3 slots takes the most expensive pool item its slot type
   allows: 1 sword / mace / axe, 2 the same or a bow (+2000), 3 sword / mace / axe / staff / wand, 4 staff / wand,
   5 shield / chain / plate, 6 misc; an axe scores 0 unless the hero's ranged type is 100; spell skill / save items
   score 0 for a non-caster (slot types 1, 2, 6). BUG #78: slot type 4 falls through into the armour case, so a staff
   slot takes armour. An unknown slot type keeps the previous slot's choice (idx is not reset) */
void Hero_Items_AI_Equip_Best(int player, int hero_slot, int *pool, int *count)
{
    int slot;
    int best;
    int pick;
    int n;
    int cost;
    int unit_type;
    int i;

    n = *count;
    for (slot = 0; slot < 3; slot++) {
        unit_type = p_units[wizards[player].hired_heroes[hero_slot].unit_index].type;
        switch (wizards[player].hired_heroes[hero_slot].slot_type[slot]) {
        case ITEM_SLOT_MELEE:
            pick = -1;
            best = 0;
            for (i = 0; i < n; i++) {
                if (p_items[pool[i]].item_type == ITEM_TYPE_SWORD || p_items[pool[i]].item_type == ITEM_TYPE_MACE
                    || p_items[pool[i]].item_type == ITEM_TYPE_AXE) {
                    cost = p_items[pool[i]].cost;
                    if (p_items[pool[i]].item_type == ITEM_TYPE_AXE && unit_types[unit_type].ranged_type != RANGED_THROWN)
                        cost = 0;
                    if ((p_items[pool[i]].spell_skill > 0 || p_items[pool[i]].spell_save > 0)
                        && p_hero_stats[player][unit_type].caster_level == 0)
                        cost = 0;
                    if (cost > best) {
                        pick = i;
                        best = cost;
                    }
                }
            }
            break;
        case ITEM_SLOT_BOW:
            pick = -1;
            best = 0;
            for (i = 0; i < n; i++) {
                if (p_items[pool[i]].item_type == ITEM_TYPE_SWORD || p_items[pool[i]].item_type == ITEM_TYPE_MACE
                    || p_items[pool[i]].item_type == ITEM_TYPE_AXE || p_items[pool[i]].item_type == ITEM_TYPE_BOW) {
                    cost = p_items[pool[i]].cost;
                    if (p_items[pool[i]].item_type == ITEM_TYPE_BOW)
                        cost += 2000;           /* the AI's preference for a bow in that slot */
                    if (p_items[pool[i]].item_type == ITEM_TYPE_AXE && unit_types[unit_type].ranged_type != RANGED_THROWN)
                        cost = 0;
                    if ((p_items[pool[i]].spell_skill > 0 || p_items[pool[i]].spell_save > 0)
                        && p_hero_stats[player][unit_type].caster_level == 0)
                        cost = 0;
                    if (cost > best) {
                        pick = i;
                        best = cost;
                    }
                }
            }
            break;
        case ITEM_SLOT_MELEE_OR_MAGIC:
            pick = -1;
            best = 0;
            for (i = 0; i < n; i++) {
                if (p_items[pool[i]].item_type == ITEM_TYPE_SWORD || p_items[pool[i]].item_type == ITEM_TYPE_MACE
                    || p_items[pool[i]].item_type == ITEM_TYPE_AXE || p_items[pool[i]].item_type == ITEM_TYPE_STAFF
                    || p_items[pool[i]].item_type == ITEM_TYPE_WAND) {
                    cost = p_items[pool[i]].cost;
                    if (p_items[pool[i]].item_type == ITEM_TYPE_AXE && unit_types[unit_type].ranged_type != RANGED_THROWN)
                        cost = 0;
                    if (cost > best) {
                        pick = i;
                        best = cost;
                    }
                }
            }
            break;
        case ITEM_SLOT_MAGIC:
            pick = -1;
            best = 0;
            for (i = 0; i < n; i++) {
                if (p_items[pool[i]].item_type == ITEM_TYPE_STAFF || p_items[pool[i]].item_type == ITEM_TYPE_WAND) {
                    cost = p_items[pool[i]].cost;
                    if (cost > best) {
                        pick = i;
                        best = cost;
                    }
                }
            }
        case ITEM_SLOT_ARMOR:
            pick = -1;
            best = 0;
            for (i = 0; i < n; i++) {
                if (p_items[pool[i]].item_type == ITEM_TYPE_SHIELD || p_items[pool[i]].item_type == ITEM_TYPE_CHAIN_MAIL
                    || p_items[pool[i]].item_type == ITEM_TYPE_PLATE_MAIL) {
                    cost = p_items[pool[i]].cost;
                    if (cost > best) {
                        pick = i;
                        best = cost;
                    }
                }
            }
            break;
        case ITEM_SLOT_MISC:
            pick = -1;
            best = 0;
            for (i = 0; i < n; i++) {
                if (p_items[pool[i]].item_type == ITEM_TYPE_MISC) {
                    cost = p_items[pool[i]].cost;
                    if ((p_items[pool[i]].spell_skill > 0 || p_items[pool[i]].spell_save > 0)
                        && p_hero_stats[player][unit_type].caster_level == 0)
                        cost = 0;
                    if (cost > best) {
                        pick = i;
                        best = cost;
                    }
                }
            }
            break;
        }
        if (pick > -1) {
            wizards[player].hired_heroes[hero_slot].item_index[slot] = pool[pick];
            Array_Delete_Near(pick, pool, sizeof(int), n);
            n--;
        }
    }
    *count = n;
}

/* (player, hero slot, pool, &count) the second pass: each still empty slot takes the most expensive pool item of
   its slot type (no axe or caster test; slot type 4 is staff / wand here) */
void Hero_Items_AI_Fill_Empty(int player, int hero_slot, int *pool, int *count)
{
    int slot;
    int best;
    int pick;
    int n;
    int cost;
    int unit_type;                              /* written, never read */
    int i;

    n = *count;
    for (slot = 0; slot < 3; slot++) {
        unit_type = p_units[wizards[player].hired_heroes[hero_slot].unit_index].type;
        if (wizards[player].hired_heroes[hero_slot].item_index[slot] < 0) {
            switch (wizards[player].hired_heroes[hero_slot].slot_type[slot]) {
            case ITEM_SLOT_MELEE:
                pick = -1;
                best = 0;
                for (i = 0; i < n; i++) {
                    if (p_items[pool[i]].item_type == ITEM_TYPE_SWORD || p_items[pool[i]].item_type == ITEM_TYPE_MACE
                        || p_items[pool[i]].item_type == ITEM_TYPE_AXE) {
                        cost = p_items[pool[i]].cost;
                        if (cost > best) {
                            pick = i;
                            best = cost;
                        }
                    }
                }
                break;
            case ITEM_SLOT_BOW:
                pick = -1;
                best = 0;
                for (i = 0; i < n; i++) {
                    if (p_items[pool[i]].item_type == ITEM_TYPE_SWORD || p_items[pool[i]].item_type == ITEM_TYPE_MACE
                        || p_items[pool[i]].item_type == ITEM_TYPE_AXE || p_items[pool[i]].item_type == ITEM_TYPE_BOW) {
                        cost = p_items[pool[i]].cost;
                        if (cost > best) {
                            pick = i;
                            best = cost;
                        }
                    }
                }
                break;
            case ITEM_SLOT_MELEE_OR_MAGIC:
                pick = -1;
                best = 0;
                for (i = 0; i < n; i++) {
                    if (p_items[pool[i]].item_type == ITEM_TYPE_SWORD || p_items[pool[i]].item_type == ITEM_TYPE_MACE
                        || p_items[pool[i]].item_type == ITEM_TYPE_AXE || p_items[pool[i]].item_type == ITEM_TYPE_STAFF
                        || p_items[pool[i]].item_type == ITEM_TYPE_WAND) {
                        cost = p_items[pool[i]].cost;
                        if (cost > best) {
                            pick = i;
                            best = cost;
                        }
                    }
                }
                break;
            case ITEM_SLOT_MAGIC:
                pick = -1;
                best = 0;
                for (i = 0; i < n; i++) {
                    if (p_items[pool[i]].item_type == ITEM_TYPE_STAFF || p_items[pool[i]].item_type == ITEM_TYPE_WAND) {
                        cost = p_items[pool[i]].cost;
                        if (cost > best) {
                            pick = i;
                            best = cost;
                        }
                    }
                }
                break;
            case ITEM_SLOT_ARMOR:
                pick = -1;
                best = 0;
                for (i = 0; i < n; i++) {
                    if (p_items[pool[i]].item_type == ITEM_TYPE_SHIELD || p_items[pool[i]].item_type == ITEM_TYPE_CHAIN_MAIL
                        || p_items[pool[i]].item_type == ITEM_TYPE_PLATE_MAIL) {
                        cost = p_items[pool[i]].cost;
                        if (cost > best) {
                            pick = i;
                            best = cost;
                        }
                    }
                }
                break;
            case ITEM_SLOT_MISC:
                pick = -1;
                best = 0;
                for (i = 0; i < n; i++) {
                    if (p_items[pool[i]].item_type == ITEM_TYPE_MISC) {
                        cost = p_items[pool[i]].cost;
                        if (cost > best) {
                            pick = i;
                            best = cost;
                        }
                    }
                }
                break;
            }
            if (pick > -1) {
                wizards[player].hired_heroes[hero_slot].item_index[slot] = pool[pick];
                Array_Delete_Near(pick, pool, sizeof(int), n);
                n--;
            }
        }
    }
    *count = n;
}
