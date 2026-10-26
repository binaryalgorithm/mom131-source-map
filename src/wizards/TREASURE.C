/* TREASURE.C - WIZARDS.EXE overlay code segment aba4: a wizard's spell research list (refilling the 8 research
   candidates from the lowest open rarity tier of each realm, Spell of Mastery once the pool is empty, sorting them by
   research turns), treasure / found spellbooks (a random unknown spell, the new spells of a spellbook), the spell
   trade values and tradeable lists of diplomacy, the relation penalties of hostile city spells and global
   enchantments, the Artificer spells and the spells looted from a conquered wizard.
   This module's data (DS:661c): its literals; no BSS.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS aba4 src/wizards/TREASURE.C */

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Wizard_Gain_Spellbook(int player, int realm);
int Research_Candidates_Refill(int player);
void Research_Pool_Build(int player, int *pool);
int Treasure_Pick_Spell(int player, int rarity);
int Spell_Trade_Value(int player, int spell);
int Diplo_Tradeable_Spells(int receiver, int giver, int min_value, unsigned char *spell_list);
void Hostile_Spell_Relations(int caster, int spell, int city);
void Global_Enchant_Relations(int caster, int spell, int divisor);
void Research_Candidates_Sort(int player, int n);
void Wizard_Grant_Artificer_Spells(int player);
int Conquest_Loot_Spells(int winner, int loser);

#include "GAME.H"

extern int list_count;                          /* DS:c926 */
extern int game_turn;                           /* DS:bd90 */

char *Near_Alloc_Reset(unsigned bytes);
int Random(int n);
int Screen_Choose_Research(void);
int AI_Choose_Research(int player);
int Diplo_Relation_Change(int amount, int actor, int observer, int msg, int city, int msg_arg);
void Wizard_Power_Base(void);
void Wizard_Magic_Split(int *mana, int *skill, int *research, int player);
int Research_Bonus_Pct(int player, int spell);
int Array_Delete_Near(int index, char *array, int record_size, int count);
void Wizard_Learn_Spell(int player, int spell, int show);
void Minimap_Alloc_Pic(void);
void Map_Full_Redraw(void);
void Screen_Copy_Front(void);
void Screen_Save_A800(void);
void far_strcpy();                              /* no prototype here (nor <string.h>): Conquest_Loot_Spells passes a char as the buffer */
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);

/* (player) -> the number of research candidates: the 8 slots refilled at random from the research pool (each pick
   marked SPELLSTATE_OFFERED), Spell of Mastery added once the pool is empty (unless known or listed); sorted by
   research turns; a new research spell chosen (human screen / AI) when none is set after turn 1 */
int Research_Candidates_Refill(int player)
{
    int max;
    int pick;
    int count;
    int realm;
    int slot;
    int has_mastery;
    int *pool;
    register int i;

    max = 8;
    list_count = 0;
    count = 0;
    has_mastery = 0;
    pool = (int *)Near_Alloc_Reset(200);
    Research_Pool_Build(player, pool);
    if (wizards[player].spell_library[REALM_ARCANE][SPELL_SLOT(SPELL_SPELL_OF_MASTERY)] == SPELLSTATE_KNOWN)
        has_mastery = 1;
    else {
        for (i = 0; i < max; i++) {
            if (wizards[player].research_candidates[i] > 0) {
                count++;
                if (wizards[player].research_candidates[i] == SPELL_SPELL_OF_MASTERY)
                    has_mastery = 1;
            }
        }
    }
    while (count < max && list_count > 0) {
        pick = Random(list_count) - 1;
        count++;
        for (i = 0; i < max; i++) {
            if (wizards[player].research_candidates[i] == 0) {
                wizards[player].research_candidates[i] = pool[pick];
                slot = SPELL_SLOT(pool[pick]);
                realm = SPELL_REALM(pool[pick]);
                wizards[player].spell_library[realm][slot] = SPELLSTATE_OFFERED;
                Research_Pool_Build(player, pool);
                break;
            }
        }
    }
    count = 0;
    for (i = 0; i < max; i++)
        if (wizards[player].research_candidates[i] > 0)
            count++;
    if (count < max && list_count == 0 && has_mastery == 0) {
        for (i = 0; i < max; i++) {
            if (wizards[player].research_candidates[i] == 0) {
                wizards[player].research_candidates[i] = SPELL_SPELL_OF_MASTERY;
                break;
            }
        }
    }
    Research_Candidates_Sort(player, count);
    if (wizards[player].research_spell == SPELL_NONE && count > 0 && game_turn > 1) {
        if (player == PLAYER_HUMAN)
            Screen_Choose_Research();
        else
            AI_Choose_Research(player);
        wizards[player].research_left = wizards[player].research_spell == SPELL_SPELL_OF_MASTERY ?
            wizards[player].mastery_research_cost : p_spell_data[wizards[player].research_spell].research_cost;
    }
    return count;
}

/* (player, pool) the research pool into pool / list_count: per realm the unresearched spells (status 1) of its lowest
   rarity tier that has any, then the Arcane ones among the first 12 - the scan stops at 3, or at 8, when one was
   already found before that point */
void Research_Pool_Build(int player, int *pool)
{
    int tier;
    register int realm;
    register int found;
    register int i;

    list_count = 0;
    for (realm = 0; realm <= REALM_DEATH; realm++) {   /* the five book realms; Arcane after */
        found = 0;
        for (i = 0; i < SPELLS_PER_REALM; i++) {
            if (found == 0)
                tier = i / 10;
            if (i / 10 > tier)
                break;
            if (wizards[player].spell_library[realm][i] == SPELLSTATE_RESEARCHABLE) {
                pool[list_count] = realm * SPELLS_PER_REALM + i + 1;
                list_count++;
                found = 1;
            }
        }
    }
    found = 0;
    for (i = 0; i < 12; i++) {
        if (found == 1 && (i == 3 || i == 8))
            break;
        if (wizards[player].spell_library[REALM_ARCANE][i] == SPELLSTATE_RESEARCHABLE) {
            pool[list_count] = i + SPELL_MAGIC_SPIRIT;   /* the Arcane spells from Magic Spirit on */
            list_count++;
            found = 1;
        }
    }
}

/* (player, rarity 1..4) -> a random spell the player does not know, or 0: a random realm with books, rarity tiers
   2 / 3 / 4 for 1 / 2 / 3+ books (from rarity - 1, wrapping), Arcane any of its first 11. BUGS #72: the Arcane test reads
   i == 6 (never true), so Arcane (i = 5) is admitted by spellbooks[5], past the 5-entry array: the word of
   retorts[0..1] (Alchemy / Warlord); and a failed realm is marked available again (realm_done[] = 0)
   Numbered: BUG #72, #73 (BUGS_AND_QUIRKS.txt) */
int Treasure_Pick_Spell(int player, int rarity)
{
    int tries;
    int slot;
    int n_realms;
    int pick;
    int found;
    int tiers;
    int realm_done[6];
    register int i;
    register int realm;

    n_realms = 0;
    found = 0;
    for (i = 0; i < 6; i++) {
        if (i == 6 || wizards[player].spellbooks[i] > 0) {
            n_realms++;
            realm_done[i] = 0;
        } else
            realm_done[i] = 1;
    }
    while (n_realms > 0 && found == 0) {
        pick = Random(n_realms);
        i = 0;
        realm = 0;
        while (pick != i) {
            if (realm_done[realm] == 0)
                i++;
            realm++;
        }
        realm--;
        if (realm == REALM_ARCANE)
            tiers = 3;
        else if (wizards[player].spellbooks[realm] == 1)
            tiers = 2;
        else if (wizards[player].spellbooks[realm] == 2)
            tiers = 3;
        else
            tiers = 4;
        for (tries = 0; tries < 4 && found == 0; tries++) {
            pick = Random(10) - 1;
            for (i = 0; i < 10 && found == 0; i++) {
                if (realm == REALM_ARCANE)
                    slot = Random(11) - 1;
                else
                    slot = (pick + i) % 10 + (rarity + tries - 1) % tiers * 10;
                if (wizards[player].spell_library[realm][slot] != SPELLSTATE_KNOWN) {
                    found = 1;
                    slot = realm * SPELLS_PER_REALM + slot + 1;
                }
            }
        }
        realm_done[realm] = 0;
        n_realms--;
    }
    if (found == 0)
        slot = 0;
    return slot;
}

/* (player, spell) -> the spell's research cost as a trade value: -1 when the player cannot hold it (no books of the
   realm, 1 book and position 20+, 2 books and 30+; Arcane always), 0 when known, -10 % when offered (a current research
   candidate), +50 % when not yet available */
int Spell_Trade_Value(int player, int spell)
{
    int col;
    register int value;
    register int realm;

    value = p_spell_data[spell].research_cost;
    realm = p_spell_data[spell].realm;
    col = SPELL_SLOT(spell);
    if (realm != REALM_ARCANE && wizards[player].spellbooks[realm] == 0)
        return -1;
    if (realm != REALM_ARCANE && wizards[player].spellbooks[realm] == 1 && col > SPELL_SLOT_RARE - 1)
        return -1;
    if (realm != REALM_ARCANE && wizards[player].spellbooks[realm] == 2 && col > SPELL_SLOT_VERY_RARE - 1)
        return -1;
    if (wizards[player].spell_library[realm][col] == SPELLSTATE_KNOWN)
        return 0;
    if (wizards[player].spell_library[realm][col] == SPELLSTATE_OFFERED)
        value = value - value / 10;
    if (wizards[player].spell_library[realm][col] == SPELLSTATE_UNAVAILABLE)
        value = value + value / 2;
    return value;
}

/* (receiver, giver, minimum value, spell_list) -> up to 10 random spells giver knows and receiver lacks (a realm both
   have books in, or Arcane) worth at least the minimum to receiver; BUG #282: buf has only 10 bytes cleared, list 60 for up to 214 spells
   Numbered: BUG #9, #282 (BUGS_AND_QUIRKS.txt) */
int Diplo_Tradeable_Spells(int receiver, int giver, int min_value, unsigned char *spell_list)
{
    int spell;
    int trade_value;
    int count;
    int cand;
    int slot;
    int pick;
    unsigned char list[60];
    register int realm;
    register int n;

    for (spell = 0; spell < 10; spell++)        /* spell: also this clear's index and the 100 tries below */
        spell_list[spell] = 0;
    n = 0;
    for (spell = 1; spell < SPELL_SPELL_OF_RETURN; spell++) {   /* the Spell of Return is never traded or looted */
        realm = p_spell_data[spell].realm;
        slot = SPELL_SLOT(spell);
        if (realm != REALM_ARCANE && (wizards[receiver].spellbooks[realm] == 0 || wizards[giver].spellbooks[realm] == 0))
            spell += 39;
        else {
            if (wizards[giver].spell_library[realm][slot] != SPELLSTATE_KNOWN || wizards[receiver].spell_library[realm][slot] == SPELLSTATE_KNOWN)
                continue;
            list[n] = spell;
            n++;
        }
    }
    count = 0;
    if (n > 0) {
        for (spell = 0; spell < 100 && count < 10; spell++) {
            pick = Random(n) - 1;
            cand = list[pick];
            list[pick] = 0;
            if (cand == 0)
                continue;
            trade_value = Spell_Trade_Value(receiver, cand);
            if (trade_value < min_value)
                continue;
            realm = p_spell_data[cand].realm;
            slot = SPELL_SLOT(cand);
            spell_list[count] = cand;
            count++;
        }
    }
    return count;
}

/* (caster, spell, city) the city owner's relation to the caster after a hostile city spell: -5 .. -50 by spell */
void Hostile_Spell_Relations(int caster, int spell, int city)
{
    switch (spell) {
    case SPELL_CHAOS_RIFT:
        Diplo_Relation_Change(-5, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_EVIL_PRESENCE:
        Diplo_Relation_Change(-5, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_CURSED_LANDS:
        Diplo_Relation_Change(-5, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_PESTILENCE:
        Diplo_Relation_Change(-8, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_FAMINE:
        Diplo_Relation_Change(-5, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_EARTHQUAKE:
        Diplo_Relation_Change(-9, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_CALL_THE_VOID:
        Diplo_Relation_Change(-20, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_CORRUPTION:
        Diplo_Relation_Change(-25, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    case SPELL_RAISE_VOLCANO:
        Diplo_Relation_Change(-50, caster, p_cities[city].owner, DIPLO_MSG_CITY_SPELL_THREAT, city, spell);
        break;
    }
}

/* (caster, spell, divisor) every other wizard's relation to the caster after a global enchantment: base / divisor
   by spell (some only for wizards of the offended realms) */
void Global_Enchant_Relations(int caster, int spell, int divisor)
{
    int observer;

    for (observer = 0; observer < num_players; observer++) {
        if (observer != caster) {
            if (spell == SPELL_SPELL_OF_MASTERY)
                Diplo_Relation_Change(-50 / divisor, caster, observer, DIPLO_MSG_CEASE_MASTERY, 0, spell);
            if (spell == SPELL_ETERNAL_NIGHT)
                Diplo_Relation_Change(-12 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_EVIL_OMENS &&
                (wizards[observer].spellbooks[REALM_LIFE] > 0 || wizards[observer].spellbooks[REALM_NATURE] > 0))
                Diplo_Relation_Change(-20 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_ZOMBIE_MASTERY)
                Diplo_Relation_Change(-14 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_AURA_OF_MAJESTY)
                Diplo_Relation_Change(10 / divisor, caster, observer, 0, 0, spell);
            if (spell == SPELL_WIND_MASTERY)
                Diplo_Relation_Change(-4 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_SUPPRESS_MAGIC)
                Diplo_Relation_Change(-25 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_TIME_STOP)
                Diplo_Relation_Change(0 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_NATURE_AWARENESS)
                Diplo_Relation_Change(0 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_NATURES_WRATH &&
                (wizards[observer].spellbooks[REALM_CHAOS] > 0 || wizards[observer].spellbooks[REALM_DEATH] > 0))
                Diplo_Relation_Change(-20 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_HERB_MASTERY)
                Diplo_Relation_Change(0 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_CHAOS_SURGE && wizards[observer].spellbooks[REALM_CHAOS] == 0)
                Diplo_Relation_Change(-10 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_DOOM_MASTERY)
                Diplo_Relation_Change(-8 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_GREAT_WASTING)
                Diplo_Relation_Change(-20 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_METEOR_STORM)
                Diplo_Relation_Change(-15 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_ARMAGEDDON)
                Diplo_Relation_Change(-25 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_TRANQUILITY && wizards[observer].spellbooks[REALM_CHAOS] > 0)
                Diplo_Relation_Change(-20 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_LIFE_FORCE && wizards[observer].spellbooks[REALM_DEATH] > 0)
                Diplo_Relation_Change(-20 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_CRUSADE)
                Diplo_Relation_Change(-10 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_JUST_CAUSE)
                Diplo_Relation_Change(0 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
            if (spell == SPELL_HOLY_ARMS)
                Diplo_Relation_Change(-5 / divisor, caster, observer, DIPLO_MSG_DISPEL_OR_WRATH, 0, spell);
        }
    }
}

/* (player, n) the first n research candidates sorted by research turns (cost / the realm's research per turn,
   insertion sort) */
void Research_Candidates_Sort(int player, int n)
{
    int realm;
    int bonus_pct;
    int research;
    int spell;
    int i;
    int prev_key;
    int key;
    unsigned realm_rate[6];
    register int j;

    Wizard_Power_Base();
    Wizard_Magic_Split(&bonus_pct, &bonus_pct, &research, player);   /* mana and skill into a dummy: research only */
    for (realm = 0; realm < 6; realm++) {
        bonus_pct = Research_Bonus_Pct(player, realm * SPELLS_PER_REALM + 1);
        realm_rate[realm] = research + (int)((long)research * bonus_pct / 100L);
    }
    for (i = 1; i < n; i++) {
        spell = wizards[player].research_candidates[i];
        j = i - 1;
        prev_key = realm_rate[p_spell_data[wizards[player].research_candidates[j]].realm] == 0 ?
            p_spell_data[wizards[player].research_candidates[j]].research_cost :
            p_spell_data[wizards[player].research_candidates[j]].research_cost /
            realm_rate[p_spell_data[wizards[player].research_candidates[j]].realm];
        key = realm_rate[p_spell_data[spell].realm] == 0 ? p_spell_data[spell].research_cost :
            p_spell_data[spell].research_cost / realm_rate[p_spell_data[spell].realm];
        while (j > -1 && prev_key > key) {
            wizards[player].research_candidates[j + 1] = wizards[player].research_candidates[j];
            j--;
            if (j > -1)
                prev_key = realm_rate[p_spell_data[wizards[player].research_candidates[j]].realm] == 0 ?
                    p_spell_data[wizards[player].research_candidates[j]].research_cost :
                    p_spell_data[wizards[player].research_candidates[j]].research_cost /
                    realm_rate[p_spell_data[wizards[player].research_candidates[j]].realm];
        }
        wizards[player].research_candidates[j + 1] = spell;
    }
}

/* (player, realm) a new spellbook: per rarity tier, random unknown spells of the realm become researchable (status
   1), as many as the table gives for the books already held (none at 10 books); then one more book and the research
   list refilled. BUG #238: 11+ books index past the 10-row table - row 11 is the return address and the player /
   realm arguments, decremented per spell granted (in game: the book went to another realm)
   Numbered: BUG #238 (BUGS_AND_QUIRKS.txt) */
void Wizard_Gain_Spellbook(int player, int realm)
{
    int slot;
    int books;
    int pick;
    int tier_counts[10][4];
    int missing[10];
    register int tier;
    register int n;

    tier_counts[0][0] = 3;
    tier_counts[0][1] = 1;
    tier_counts[0][2] = 0;
    tier_counts[0][3] = 0;
    tier_counts[1][0] = 2;
    tier_counts[1][1] = 1;
    tier_counts[1][2] = 1;
    tier_counts[1][3] = 1;
    tier_counts[2][0] = 1;
    tier_counts[2][1] = 1;
    tier_counts[2][2] = 1;
    tier_counts[2][3] = 1;
    tier_counts[3][0] = 1;
    tier_counts[3][1] = 1;
    tier_counts[3][2] = 1;
    tier_counts[3][3] = 1;
    tier_counts[4][0] = 1;
    tier_counts[4][1] = 1;
    tier_counts[4][2] = 1;
    tier_counts[4][3] = 1;
    tier_counts[5][0] = 1;
    tier_counts[5][1] = 1;
    tier_counts[5][2] = 1;
    tier_counts[5][3] = 1;
    tier_counts[6][0] = 1;
    tier_counts[6][1] = 2;
    tier_counts[6][2] = 1;
    tier_counts[6][3] = 1;
    tier_counts[7][0] = 0;
    tier_counts[7][1] = 2;
    tier_counts[7][2] = 1;
    tier_counts[7][3] = 1;
    tier_counts[8][0] = 0;
    tier_counts[8][1] = 0;
    tier_counts[8][2] = 2;
    tier_counts[8][3] = 1;
    tier_counts[9][0] = 0;
    tier_counts[9][1] = 0;
    tier_counts[9][2] = 2;
    tier_counts[9][3] = 1;
    books = wizards[player].spellbooks[realm];
    if (books != 10) {
        for (tier = 0; tier < 4; tier++) {
            n = 0;
            for (slot = 0; slot < 10; slot++) {
                if (wizards[player].spell_library[realm][slot + tier * 10] == SPELLSTATE_UNAVAILABLE) {
                    missing[n] = slot + tier * 10;
                    n++;
                }
            }
            while (n > 0 && tier_counts[books][tier] > 0) {
                pick = Random(n) - 1;
                wizards[player].spell_library[realm][missing[pick]] = SPELLSTATE_RESEARCHABLE;
                Array_Delete_Near(pick, missing, 2, n);
                n--;
                tier_counts[books][tier]--;
            }
        }
    }
    wizards[player].spellbooks[realm]++;
    Research_Candidates_Refill(player);
}

/* (player) learns Enchant Item and Create Artifact (the Artificer retort's spells); no caller in
   WIZARDS.EXE, only its overlay stub entry */
void Wizard_Grant_Artificer_Spells(int player)
{
    Wizard_Learn_Spell(player, SPELL_ENCHANT_ITEM, 0);
    Wizard_Learn_Spell(player, SPELL_CREATE_ARTIFACT, 0);
}

/* (winner, loser) the winner learns up to 2 random spells the loser knows (Diplo_Tradeable_Spells); the human is
   told "You found a X spell (and a Y spell) in the ruins." / "You find no new spells in the ruins."; nothing when
   either side is the neutral player (-> 0 then; no return value otherwise). BUG #315: the spell name is copied through a
   char (name), not a buffer
   Numbered: BUG #264, #315 (BUGS_AND_QUIRKS.txt) */
int Conquest_Loot_Spells(int winner, int loser)
{
    int n_tradeable;
    int spell;
    int loot[4];
    char name;
    unsigned pic;
    int pick;
    unsigned char tradeable[60];
    register int count;

    if (winner == PLAYER_NEUTRAL || loser == PLAYER_NEUTRAL)
        return 0;
    n_tradeable = Diplo_Tradeable_Spells(winner, loser, 0, tradeable);
    count = 0;
    while (count < n_tradeable && count < 2) {
        pick = Random(n_tradeable) - 1;
        spell = tradeable[pick];
        if (spell != SPELL_NONE) {
            Wizard_Learn_Spell(winner, spell, 0);
            tradeable[pick] = 0;
            loot[count] = spell;
            count++;
        }
    }
    if (winner == PLAYER_HUMAN) {
        Minimap_Alloc_Pic();
        Map_Full_Redraw();
        Screen_Copy_Front();
        Screen_Save_A800();
        switch (count) {
        case 0:
            strcpy(message_text, "You find no new spells in the ruins.");
            break;
        case 1:
            strcpy(message_text, "You found a ");
            far_strcpy(name, 0, p_spell_data[loot[0]].name);
            strcat(message_text, name);
            strcat(message_text, " spell in the ruins.");
            break;
        case 2:
            strcpy(message_text, "You found a ");
            far_strcpy(name, 0, p_spell_data[loot[0]].name);
            strcat(message_text, name);
            strcat(message_text, " spell and a ");
            far_strcpy(name, 0, p_spell_data[loot[1]].name);
            strcat(message_text, name);
            strcat(message_text, " spell in the ruins.");
            break;
        case 3:
            strcpy(message_text, "You found a ");
            far_strcpy(name, 0, p_spell_data[loot[0]].name);
            strcat(message_text, name);
            strcat(message_text, " spell, a ");
            far_strcpy(name, 0, p_spell_data[loot[1]].name);
            strcat(message_text, name);
            strcat(message_text, " spell and a ");
            far_strcpy(name, 0, p_spell_data[loot[2]].name);
            strcat(message_text, name);
            strcat(message_text, " spell in the ruins.");
            break;
        }
        pic = LBX_Reload_Next("Spellscr", 0x3c, p_scratch);
        Notify_Picture_Box(0, 40, 2, message_text, 0, pic, 0, 9, 0, 0, 0, 1, 0);
    }
}
