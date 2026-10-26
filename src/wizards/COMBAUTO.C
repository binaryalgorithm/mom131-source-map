/* COMBAUTO.C - WIZARDS.EXE overlay code segment 888b (stub 2531): the battle-unit tables, the STRATEGIC combat (the
   auto-resolve, also the human's strategic combat; the x1.5 applies when no human side fights: toughness / missile / attack totals per side, the wizards' magic by
   their spellbooks and casting budget, up to three missile rounds then melee rounds, the losers die and the winner
   takes damage in proportion to the toughness it lost), healing a battle unit, and the AI's toughness / missile /
   attack values of a unit.
   No data of its own; BSS (DS:c892..c8a6): the three per-side aura tables and a table that is allocated and never used.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 888b src/wizards/COMBAUTO.C */

/* the module's functions in their original declaration order */
void Combat_Alloc_Tables(void);
int Combat_Strategic(int *units, int n, int x, int y, int plane, int *item_count, int *items);
void Battle_Unit_Heal(int bu, int amount, int overflow);
int AI_Toughness_Value(int hits, int defense);
int AI_Missile_Value(int ranged, int figures, int attack_flags);
int AI_Attack_Value(int melee, int ranged, int figures, int attack_flags, int ranged_type);

#include "GAME.H"

void far *p_combat_unused_table;                /* DS:c8a2  4 paragraphs allocated by Combat_Alloc_Tables, never used */
int far *p_aura_holy_bonus;                     /* DS:c89e  [player] best Holy Bonus of the side (Combat_Start_Auras); owner not fixed by the EXE, as the next two (ReMoM puts them with the combat spells) */
int far *p_aura_resistance;                     /* DS:c89a  [player] best resistance aura of the side */
int far *p_aura_leadership;                     /* DS:c896  [player] best Leadership of the side */
int combauto_unused_c892[2];                    /* DS:c892  never referenced (may be the end of message_text) */

extern struct BattleUnit far *p_battle_units;   /* DS:922a */
extern char far *p_combat_enchants;             /* DS:922e */
extern int combat_loc_plane;                    /* DS:c47c */
extern int combat_loc_y;                        /* DS:c47e */
extern int combat_loc_x;                        /* DS:c480 */
extern int combat_defender_player;              /* DS:c584 */
extern int combat_attacker_player;              /* DS:c586 */
extern int battle_unit_count;                   /* DS:c588 */

unsigned Heap_Alloc_Front(unsigned block, unsigned paragraphs);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void far *Seg_To_Far_Ptr(unsigned seg);
void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
int Combat_Terrain_Kind(int x, int y, int plane, int in_city);
void Combat_Start_Auras(int battlefield_kind);
void Battle_Unit_Combat_Modifiers(struct BattleUnit far *bu);
int Water_Battle_Sideline(int player);
int Has_Shooting_Attack(int bu);
int Combat_Range_Multiplier(int player);
int Random_Weighted_Pick(int *weights, int count);
void Battle_Apply_Damage(int unit, int *damage);
void Combat_End(int winner, int *n_items, int *items, int end_code);
void Unit_Apply_Modifiers(struct BattleUnit far *bu);
int abs(int x);

/* () allocates the battle units (0xf9 paragraphs) and, from the back of the scratch block, the combat enchantments
   (cleared), an unused table and the three per-side aura tables */
void Combat_Alloc_Tables(void)
{
    int i;

    p_battle_units = Seg_To_Far_Ptr(Heap_Alloc_Front(p_scratch, BATTLE_UNIT_TABLE_PARAS));
    p_combat_enchants = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 3));
    for (i = 0; i < NUM_COMBAT_ENCHANTS; i++)
        p_combat_enchants[i] = 0;
    p_combat_unused_table = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 4));
    p_aura_holy_bonus = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 3));
    p_aura_resistance = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 3));
    p_aura_leadership = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 3));
}

/* (units, n, x, y, plane, &item_count, items) -> 1 when the attacker won: the strategic combat. The battle units are
   every unit on (x, y) not owned by the attacker (owner -1 -> 6) and the n attacking units, with auras and combat
   modifiers; a side of no wizard (owner < 0 or > 5) is to be marked BATTLE_UNIT_DESTROYED (out of the fight), but
   the test compares battle owners with that -1 after they became 6, so unowned (dead) units on the square stay in
   and count for the defender (BUG #183). Per side: toughness (AI_Toughness_Value), missile
   (AI_Missile_Value) and attack (AI_Attack_Value) totals; with no human side every non-neutral side's totals x1.5.
   A wizard's magic: budget min(skill, 10 x reserve / range multiplier), spent skill / 3 a round (the books prepared with a
   budget of 10 or more, spent while it is over 10), then the realms in the order Nature, Sorcery, Chaos, Life, Death: each one with books overwrites some of
   the damage / percentage / healing terms (Sorcery keeps Nature's percentage, Chaos the healing, Life the damage,
   Death the percentage) - BUG #182: no realm is chosen, the terms mix; a node battlefield of another realm blocks the
   attacker's books of that realm (Life and Death on any node), never the defender's. Up to 3 missile rounds while a side has missile > 10 and
   both have toughness, then melee rounds while both attacks are over 10; a round's damage = (total + magic) x
   Random(10) / 100, applied in proportion to the toughness. Winner: the attacker if its attack is over 10, else the
   defender if its is (BUG #181: else unset); the losers die and the winner loses (100 - toughness left %) of its hits, 3 at
   a time on units picked by weight (30 - defence, +50 per hit, a dead unit 0). Combat_End code COMBAT_RESULT_AUTO_WON / _AUTO_LOST
   Numbered: BUG #181, #182, #183, #270, #294, #318, #327 (BUGS_AND_QUIRKS.txt) */
int Combat_Strategic(int *units, int n, int x, int y, int plane, int *item_count, int *items)
{
    int winner;
    int def_count;
    int unit;
    int again;                                  /* the unit to hit again (-1: pick one), then the Combat_End code */
    int pick;
    int dmg[3];
    int battlefield_kind;
    long att_tough;
    long def_tough;
    long att_attack;
    long att_missile;
    long att_tough_start;
    long def_attack;
    long def_missile;
    long def_tough_start;
    long att_damage;
    long def_damage;
    int att_hits;
    int def_hits;
    int hits;
    int hits_kept;
    int tough_left_pct;
    int hp;                                     /* also the thrown / breath strength passed to AI_Attack_Value */
    int att_budget;
    int def_budget;
    int att_book_dmg;
    int def_book_dmg;
    int att_book_pct;
    int def_book_pct;
    int att_book_heal;
    int def_book_heal;
    int att_spend;
    int def_spend;
    int att_magic_pct;
    int def_magic_pct;
    int att_magic_heal;
    int def_magic_heal;
    int att_magic_dmg;
    int def_magic_dmg;
    int weights[MAX_BATTLE_UNITS];
    int i;
    int books;

    att_magic_pct = 0;
    att_magic_dmg = 0;
    att_magic_heal = 0;
    def_magic_pct = 0;
    def_magic_dmg = 0;
    def_magic_heal = 0;
    Set_Draw_Page();
    Minimap_Alloc_Pic();
    Map_Redraw();
    Screen_Save_A800();
    combat_loc_x = x;
    combat_loc_y = y;
    combat_loc_plane = plane;
    def_count = 0;
    battle_unit_count = 0;
    Combat_Alloc_Tables();
    battlefield_kind = Combat_Terrain_Kind(x, y, plane, 1);
    for (i = num_units - 1; i >= 0; i--) {
        if (p_units[i].x == x && p_units[i].y == y && p_units[i].plane == plane
            && p_units[i].owner != p_units[units[0]].owner) {
            def_count++;
            combat_defender_player = p_units[i].owner;
            Battle_Unit_Init(i, &p_battle_units[battle_unit_count]);
            if (p_battle_units[battle_unit_count].owner == PLAYER_NONE)
                p_battle_units[battle_unit_count].owner = PLAYER_NO_WIZARD;
            battle_unit_count++;
        }
    }
    for (i = 0; i < n; i++) {
        unit = units[i];
        combat_attacker_player = p_units[unit].owner;
        Battle_Unit_Init(unit, &p_battle_units[battle_unit_count]);
        battle_unit_count++;
    }
    Combat_Start_Auras(battlefield_kind);
    for (i = 0; i < battle_unit_count; i++)
        Battle_Unit_Combat_Modifiers(&p_battle_units[i]);
    if (battlefield_kind == BATTLEFIELD_WATER) {
        n -= Water_Battle_Sideline(combat_attacker_player);
        def_count -= Water_Battle_Sideline(combat_defender_player);
    }
    att_tough_start = 0;
    def_tough_start = 0;
    att_tough = 0;
    def_tough = 0;
    att_attack = 0;
    def_attack = 0;
    att_missile = 0;
    def_missile = 0;
    att_hits = 0;
    def_hits = 0;
    if (combat_attacker_player < 0 || combat_attacker_player > PLAYER_NEUTRAL)
        for (i = 0; i < battle_unit_count; i++)
            if (p_battle_units[i].owner == combat_attacker_player)
                p_battle_units[i].status = BATTLE_UNIT_DESTROYED;
    if (combat_defender_player < 0 || combat_defender_player > PLAYER_NEUTRAL)
        for (i = 0; i < battle_unit_count; i++)
            if (p_battle_units[i].owner == combat_defender_player)
                p_battle_units[i].status = BATTLE_UNIT_DESTROYED;
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].status == BATTLE_UNIT_ACTIVE) {
            weights[i] = 30 - p_battle_units[i].defense;   /* the chance to take the winner's lost hits: less armour, more */
            if (p_battle_units[i].owner == combat_attacker_player) {
                hp = p_battle_units[i].hits_per_figure * p_battle_units[i].figures_alive
                     - p_battle_units[i].front_damage;
                att_tough += AI_Toughness_Value(hp, p_battle_units[i].defense);
                if (Has_Shooting_Attack(i) != 0)
                    att_missile += AI_Missile_Value(p_battle_units[i].ranged, p_battle_units[i].figures_alive,
                                                    p_battle_units[i].attack_flags | p_battle_units[i].ranged_powers);
                if (p_battle_units[i].ranged_type >= RANGED_THROWN)
                    hp = p_battle_units[i].ranged;
                else
                    hp = 0;
                att_attack += AI_Attack_Value(p_battle_units[i].melee, hp, p_battle_units[i].figures_alive,
                                              p_battle_units[i].attack_flags | p_battle_units[i].melee_powers,
                                              p_battle_units[i].ranged_type);
                att_hits += p_battle_units[i].hits_per_figure * p_battle_units[i].figures_alive
                            - p_battle_units[i].front_damage;
            } else {
                hp = p_battle_units[i].hits_per_figure * p_battle_units[i].figures_alive
                     - p_battle_units[i].front_damage;
                def_tough += AI_Toughness_Value(hp, p_battle_units[i].defense);
                if (Has_Shooting_Attack(i) != 0)
                    def_missile += AI_Missile_Value(p_battle_units[i].ranged, p_battle_units[i].figures_alive,
                                                    p_battle_units[i].attack_flags | p_battle_units[i].ranged_powers);
                if (p_battle_units[i].ranged_type >= RANGED_THROWN)
                    hp = p_battle_units[i].ranged;
                else
                    hp = 0;
                def_attack += AI_Attack_Value(p_battle_units[i].melee, hp, p_battle_units[i].figures_alive,
                                              p_battle_units[i].attack_flags | p_battle_units[i].melee_powers,
                                              p_battle_units[i].ranged_type);
                def_hits += p_battle_units[i].hits_per_figure * p_battle_units[i].figures_alive
                            - p_battle_units[i].front_damage;
            }
        }
    }
    if (combat_attacker_player < num_players) {
        att_budget = 10 * (long)wizards[combat_attacker_player].mana_reserve
                     / Combat_Range_Multiplier(combat_attacker_player);
        if (wizards[combat_attacker_player].nominal_casting_skill < att_budget)
            att_budget = wizards[combat_attacker_player].nominal_casting_skill;
        att_spend = wizards[combat_attacker_player].nominal_casting_skill / 3;
        if (att_spend > att_budget)
            att_spend = att_budget;
        att_book_dmg = 0;
        att_book_pct = 0;
        att_book_heal = 0;
        if (att_budget >= 10) {
            books = wizards[combat_attacker_player].spellbooks[REALM_NATURE];
            if (battlefield_kind == BATTLEFIELD_SORCERY_NODE || battlefield_kind == BATTLEFIELD_CHAOS_NODE)
                books = 0;
            if (books > 0) {
                att_book_dmg = books * 50;
                att_book_pct = books * 10;
                att_book_heal = books * 25;
            }
            books = wizards[combat_attacker_player].spellbooks[REALM_SORCERY];
            if (battlefield_kind == BATTLEFIELD_NATURE_NODE || battlefield_kind == BATTLEFIELD_CHAOS_NODE)
                books = 0;
            if (books > 0) {
                att_book_dmg = books * 75;
                att_book_heal = books * 25;
            }
            books = wizards[combat_attacker_player].spellbooks[REALM_CHAOS];
            if (battlefield_kind == BATTLEFIELD_SORCERY_NODE || battlefield_kind == BATTLEFIELD_NATURE_NODE)
                books = 0;
            if (books > 0) {
                att_book_dmg = books * 100;
                att_book_pct = books * 30;
            }
            books = wizards[combat_attacker_player].spellbooks[REALM_LIFE];
            if (battlefield_kind == BATTLEFIELD_SORCERY_NODE || battlefield_kind == BATTLEFIELD_CHAOS_NODE || battlefield_kind == BATTLEFIELD_NATURE_NODE)
                books = 0;
            if (books > 0) {
                att_book_pct = books * 15;
                att_book_heal = books * 40;
            }
            books = wizards[combat_attacker_player].spellbooks[REALM_DEATH];
            if (battlefield_kind == BATTLEFIELD_SORCERY_NODE || battlefield_kind == BATTLEFIELD_CHAOS_NODE || battlefield_kind == BATTLEFIELD_NATURE_NODE)
                books = 0;
            if (books > 0) {
                att_book_dmg = books * 75;
                att_book_heal = books * 15;
            }
            att_book_dmg /= 10;
            att_book_pct /= 10;
            att_book_heal /= 20;
        }
    } else
        att_budget = 0;
    if (combat_defender_player < num_players) {
        def_budget = 10 * (long)wizards[combat_defender_player].mana_reserve
                     / Combat_Range_Multiplier(combat_defender_player);
        if (wizards[combat_defender_player].nominal_casting_skill < def_budget)
            def_budget = wizards[combat_defender_player].nominal_casting_skill;
        def_spend = wizards[combat_defender_player].nominal_casting_skill / 3;
        if (def_spend > def_budget)
            def_spend = def_budget;
        def_book_dmg = 0;
        def_book_pct = 0;
        def_book_heal = 0;
        if (def_budget >= 10) {
            books = wizards[combat_defender_player].spellbooks[REALM_NATURE];
            if (books > 0) {
                def_book_dmg = books * 50;
                def_book_pct = books * 10;
                def_book_heal = books * 25;
            }
            books = wizards[combat_defender_player].spellbooks[REALM_SORCERY];
            if (books > 0) {
                def_book_dmg = books * 75;
                def_book_heal = books * 25;
            }
            books = wizards[combat_defender_player].spellbooks[REALM_CHAOS];
            if (books > 0) {
                def_book_dmg = books * 100;
                def_book_pct = books * 30;
            }
            books = wizards[combat_defender_player].spellbooks[REALM_LIFE];
            if (books > 0) {
                def_book_pct = books * 15;
                def_book_heal = books * 40;
            }
            books = wizards[combat_defender_player].spellbooks[REALM_DEATH];
            if (books > 0) {
                def_book_dmg = books * 75;
                def_book_heal = books * 15;
            }
            def_book_dmg /= 10;
            def_book_pct /= 10;
            def_book_heal /= 20;
        }
    } else
        def_budget = 0;
    att_tough_start = att_tough;
    def_tough_start = def_tough;
    if (combat_attacker_player != PLAYER_HUMAN && combat_defender_player != PLAYER_HUMAN) {
        if (combat_attacker_player != PLAYER_NEUTRAL) {
            att_magic_pct += att_magic_pct / 2;
            att_magic_dmg += att_magic_dmg / 2;
            att_magic_heal += att_magic_heal / 2;
            att_tough_start += att_tough_start / 2;
            att_tough += att_tough / 2;
            att_attack += att_attack / 2;
            att_missile += att_missile / 2;
            att_book_dmg += att_book_dmg / 2;
            att_book_pct += att_book_pct / 2;
            att_book_heal += att_book_heal / 2;
        }
        if (combat_defender_player != PLAYER_NEUTRAL) {
            def_magic_pct += def_magic_pct / 2;
            def_magic_dmg += def_magic_dmg / 2;
            def_magic_heal += def_magic_heal / 2;
            def_tough_start += def_tough_start / 2;
            def_tough += def_tough / 2;
            def_attack += def_attack / 2;
            def_missile += def_missile / 2;
            def_book_dmg += def_book_dmg / 2;
            def_book_pct += def_book_pct / 2;
            def_book_heal += def_book_heal / 2;
        }
    }
    for (i = 0; i < 3 && (att_missile > 10 || def_missile > 10) && att_tough > 0 && def_tough > 0; i++) {
        if (att_budget > 10) {
            att_magic_dmg = att_book_dmg * att_spend / 5;
            att_magic_pct = att_book_pct * att_spend / 5;
            att_magic_heal = att_book_heal * att_spend / 5;
            att_magic_dmg += (int)(att_missile * att_magic_pct / 100);
            att_book_heal = att_book_heal / 2;
            wizards[combat_attacker_player].mana_reserve -=
                Combat_Range_Multiplier(combat_attacker_player) * att_spend / 10;
            att_budget -= att_spend;
            if (att_spend > att_budget)
                att_spend = att_budget;
        } else {
            att_magic_pct = 0;
            att_magic_dmg = 0;
            att_magic_heal = 0;
        }
        if (def_budget > 10) {
            def_magic_dmg = def_book_dmg * def_spend / 5;
            def_magic_pct = def_book_pct * def_spend / 5;
            def_magic_heal = def_book_heal * def_spend / 5;
            def_magic_dmg = def_missile * def_magic_pct / 100;     /* BUG #327: '=' - the channelled damage is lost */
            def_book_heal = def_book_heal / 2;
            wizards[combat_defender_player].mana_reserve -=
                Combat_Range_Multiplier(combat_defender_player) * def_spend / 10;
            def_budget -= def_spend;
            if (def_spend > def_budget)
                def_spend = def_budget;
        } else {
            def_magic_pct = 0;
            def_magic_dmg = 0;
            def_magic_heal = 0;
        }
        att_damage = Random(10) * (att_missile + att_magic_dmg) / 100;
        def_damage = Random(10) * (def_missile + def_magic_dmg) / 100;
        att_tough += att_magic_heal;
        def_tough += def_magic_heal;
        att_missile -= att_missile * def_damage / att_tough;
        def_missile -= def_missile * att_damage / def_tough;
        att_attack -= att_attack * def_damage / att_tough;
        def_attack -= def_attack * att_damage / def_tough;
        att_tough -= def_damage;
        def_tough -= att_damage;
        if (att_missile < 0)
            att_missile = 0;
        if (def_missile < 0)
            def_missile = 0;
    }
    while (att_attack > 10 && def_attack > 10) {
        if (att_budget > 10) {
            att_magic_dmg = att_book_dmg * att_spend / 5;
            att_magic_pct = att_book_pct * att_spend / 5;
            att_magic_heal = att_book_pct * att_spend / 5;           /* BUG #327: the percentage, not the healing */
            att_magic_dmg += (int)(att_missile * att_magic_pct / 100);
            wizards[combat_attacker_player].mana_reserve -=
                Combat_Range_Multiplier(combat_attacker_player) * att_spend / 10;
            att_budget -= att_spend / 2;
            if (att_budget / 2 < att_spend)
                att_spend = att_budget / 2;
        } else {
            att_magic_pct = 0;
            att_magic_dmg = 0;
            att_magic_heal = 0;
        }
        if (def_budget > 10) {
            def_magic_dmg = def_book_dmg * def_spend / 5;
            def_magic_pct = def_book_pct * def_spend / 5;
            def_magic_heal = def_book_pct * def_spend / 5;           /* BUG #327: as the attacker's (398) */
            def_magic_dmg = def_missile * def_magic_pct / 100;       /* BUG #327: = drops the book damage (the attacker's line adds) */
            wizards[combat_defender_player].mana_reserve -=
                Combat_Range_Multiplier(combat_defender_player) * def_spend / 10;
            def_budget -= def_spend / 2;
            if (def_budget / 2 < def_spend)
                def_spend = def_budget / 2;
        } else {
            def_magic_pct = 0;
            def_magic_dmg = 0;
            def_magic_heal = 0;
        }
        att_damage = Random(10) * (att_attack + att_magic_dmg) / 100;
        def_damage = Random(10) * (def_attack + def_magic_dmg) / 100;
        att_tough += att_magic_heal;
        def_tough += def_magic_heal;
        att_attack -= att_attack * def_damage / att_tough;
        def_attack -= def_attack * att_damage / def_tough;
        att_tough -= def_damage;
        def_tough -= att_damage;
    }
    if (att_attack > 10) {
        winner = combat_attacker_player;
        tough_left_pct = 100 * att_tough / att_tough_start;
        hits_kept = (long)att_hits * tough_left_pct / 100;
        hits = att_hits;
    } else if (def_attack > 10) {
        winner = combat_defender_player;
        tough_left_pct = 100 * def_tough / def_tough_start;
        hits_kept = (long)def_hits * tough_left_pct / 100;
        hits = def_hits;
    }
    for (i = 0; i < battle_unit_count; i++) {
        if (p_battle_units[i].owner != winner) {
            p_battle_units[i].status = BATTLE_UNIT_DEAD;
            weights[i] = 0;
        }
    }
    dmg[DAMAGE_NORMAL] = 3;
    dmg[DAMAGE_DRAIN] = 0;
    dmg[DAMAGE_DESTROY] = 0;
    if (winner != -1) {
        /* the winner's lost hits, 3 at a time, to a weighted pick (the picked unit's weight +50); a unit that survives
           a hit takes the next one too, a dead one drops out */
        for (again = -1; hits > hits_kept; hits -= 3) {
            pick = again > -1 ? again : Random_Weighted_Pick(weights, battle_unit_count);
            weights[pick] += 50;
            Battle_Apply_Damage(pick, dmg);
            if (p_battle_units[pick].status > 0) {
                weights[pick] = 0;
                again = -1;
            } else
                again = pick;
        }
    }
    if (winner == PLAYER_HUMAN)                 /* again reused: the Combat_End code */
        again = COMBAT_RESULT_AUTO_WON;
    else
        again = COMBAT_RESULT_AUTO_LOST;
    Combat_End(winner, item_count, items, again);
    if (winner == combat_attacker_player)
        return 1;
    else
        return 0;
}

/* (bu, amount, overflow) heals damage buckets 0 then 1 (never 2, the permanent one) by amount (capped at their sum
   unless overflow); the front figure's damage drops and dead figures come back while the excess lasts, the last one
   keeping the leftover; with overflow (Life Steal) the excess / figures goes to heal_overflow - but the revive path
   reaches it too with its leftover (BUG #184); then the stats are recomputed with the moves kept. With overflow the
   amount is not capped, so the front damage and the figures heal past what buckets 0 / 1 hold: Life Steal undoes
   irreversible damage too; clarified using ReMoM repo */
void Battle_Unit_Heal(int bu, int amount, int overflow)
{
    int excess;
    int healable;
    int moves;
    int dmg[3];
    int i;

    healable = p_battle_units[bu].damage[DAMAGE_DRAIN] + p_battle_units[bu].damage[DAMAGE_NORMAL];
    excess = 0;
    if (overflow == 0 && amount > healable)
        amount = healable;
    dmg[DAMAGE_NORMAL] = p_battle_units[bu].damage[DAMAGE_NORMAL];
    dmg[DAMAGE_DRAIN] = p_battle_units[bu].damage[DAMAGE_DRAIN];
    dmg[DAMAGE_DESTROY] = p_battle_units[bu].damage[DAMAGE_DESTROY];
    dmg[DAMAGE_NORMAL] -= amount;
    if (dmg[DAMAGE_NORMAL] < 0) {
        dmg[DAMAGE_DRAIN] += dmg[DAMAGE_NORMAL];
        dmg[DAMAGE_NORMAL] = 0;
        if (dmg[DAMAGE_DRAIN] < 0)
            dmg[DAMAGE_DRAIN] = 0;
    }
    for (i = 0; i < 3; i++)
        p_battle_units[bu].damage[i] = dmg[i];
    p_battle_units[bu].front_damage = p_battle_units[bu].front_damage - amount;
    if (p_battle_units[bu].front_damage < 0) {
        excess = p_battle_units[bu].front_damage;
        p_battle_units[bu].front_damage = 0;
        while (excess < 0 && p_battle_units[bu].figures > p_battle_units[bu].figures_alive) {
            p_battle_units[bu].figures_alive++;
            excess += p_battle_units[bu].hits_per_figure;
        }
        if (excess > 0)
            p_battle_units[bu].front_damage = excess;
    }
    if (overflow == 0 && excess < 0)
        excess = 0;
    excess = abs(excess);
    if (p_battle_units[bu].figures <= excess)
        p_battle_units[bu].heal_overflow = p_battle_units[bu].heal_overflow + excess / p_battle_units[bu].figures;
    moves = p_battle_units[bu].moves;
    Unit_Apply_Modifiers(&p_battle_units[bu]);
    Battle_Unit_Combat_Modifiers(&p_battle_units[bu]);
    p_battle_units[bu].moves = moves;
}

/* (hits, defense) -> the AI's toughness value: hits x 3, scaled x 0.5 / 0.6 / 0.7 / 0.8 for defence 0..3, else
   x (defence - 2)^2 / 2 */
int AI_Toughness_Value(int hits, int defense)
{
    int value;

    value = hits * 3;
    if (defense > 3)
        value *= (defense - 2) * (defense - 2) / 2;
    else
        switch (defense) {
        case 0:
            value = value / 2;
            break;
        case 1:
            value = value * 3 / 5;
            break;
        case 2:
            value = value * 7 / 10;
            break;
        case 3:
            value = value * 4 / 5;
            break;
        }
    return value;
}

/* (ranged, figures, attack_flags) -> the AI's value of a ranged attack: per figure 40 / 7, 40 / 3, 80 / 3 for strength 1..3,
   else (strength - 2) x 20; flags (attack and ranged abilities): automatic damage doubles (else armor piercing x 5/4),
   poison / life steal / destruction / stoning touch / death touch each add 60 per figure, illusion x 5, eldritch weapon x 5/4 */
int AI_Missile_Value(int ranged, int figures, int attack_flags)
{
    int value;

    value = 0;
    if (ranged == 0)
        return 0;
    if (ranged > 3)
        value = (ranged - 2) * figures * 20;
    else {
        value = figures * 40;
        switch (ranged) {
        case 1:
            value /= 7;
            break;
        case 2:
            value /= 3;
            break;
        case 3:
            value = value * 2 / 3;
            break;
        }
    }
    if (attack_flags > 0) {
        if (attack_flags & ATTACK_AUTOMATIC_DAMAGE)
            value = value * 2;
        else if (attack_flags & ATTACK_ARMOR_PIERCING)
            value = value * 5 / 4;
        if (attack_flags & ATTACK_POISON)
            value += figures * 60;
        if (attack_flags & ATTACK_LIFE_STEAL)
            value += figures * 60;
        if (attack_flags & ATTACK_DESTRUCTION)
            value += figures * 60;
        if (attack_flags & ATTACK_STONING_TOUCH)
            value += figures * 60;
        if (attack_flags & ATTACK_DEATH_TOUCH)
            value += figures * 60;
        if (attack_flags & ATTACK_ILLUSION)
            value = value * 5;
        if (attack_flags & ATTACK_ELDRITCH_WEAPON)
            value = value * 5 / 4;
    }
    return value;
}

/* (melee, ranged, figures, attack_flags, ranged_type) -> the AI's value of an attack: melee per figure 40 / 7, 40 / 3,
   80 / 3 for strength 1..3, else (strength - 2) x 20, plus a thrown / breath (ranged) part at 1.5 times that; the
   flag multipliers of AI_Missile_Value plus first strike x 5/4; ranged type 103+ adds 300 (104: 1200). BUG #439: melee 0 -> 0
   Numbered: BUG #318 (BUGS_AND_QUIRKS.txt) */
int AI_Attack_Value(int melee, int ranged, int figures, int attack_flags, int ranged_type)
{
    int value;
    int ranged_value;

    if (melee == 0)
        return 0;
    if (melee > 3)
        value = (melee - 2) * figures * 20;
    else {
        value = figures * 40;
        switch (melee) {
        case 0:
            value = 0;
            break;
        case 1:
            value /= 7;
            break;
        case 2:
            value /= 3;
            break;
        case 3:
            value = value * 2 / 3;
            break;
        }
    }
    ranged_value = 0;
    if (ranged > 0) {
        if (ranged > 3)
            ranged_value = (ranged - 2) * figures * 30;
        else {
            ranged_value = figures * 60;
            switch (ranged) {
            case 0:
                ranged_value = 0;
                break;
            case 1:
                ranged_value /= 7;
                break;
            case 2:
                ranged_value /= 3;
                break;
            case 3:
                ranged_value = ranged_value * 2 / 3;
                break;
            }
        }
    }
    value += ranged_value;
    if (attack_flags > 0) {
        if (attack_flags & ATTACK_AUTOMATIC_DAMAGE)
            value = value * 2;
        else if (attack_flags & ATTACK_ARMOR_PIERCING)
            value = value * 5 / 4;
        if (attack_flags & ATTACK_POISON)
            value += figures * 60;
        if (attack_flags & ATTACK_LIFE_STEAL)
            value += figures * 60;
        if (attack_flags & ATTACK_DESTRUCTION)
            value += figures * 60;
        if (attack_flags & ATTACK_STONING_TOUCH)
            value += figures * 60;
        if (attack_flags & ATTACK_DEATH_TOUCH)
            value += figures * 60;
        if (attack_flags & ATTACK_ILLUSION)
            value = value * 5;
        if (attack_flags & ATTACK_FIRST_STRIKE)
            value = value * 5 / 4;
        if (attack_flags & ATTACK_ELDRITCH_WEAPON)
            value = value * 5 / 4;
    }
    if (ranged_type >= RANGED_STONING_GAZE) {
        value += 300;
        if (ranged_type == RANGED_DOOM_GAZE)
            value += 900;
    }
    return value;
}
