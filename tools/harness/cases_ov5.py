"""P4 B4 overland cases, batch 5 (N240): lair prisoner at a full fortress, the first-contact treaty reset, the
event cooldown after a load."""
import time

import mom
from regress import case

N = mom.N


def _nunits(src):
    return src.glob('num_units') if hasattr(src, 'glob') else src.get('num_units')


def _units_at(lv_or_s, x, y, plane, owner=0):
    return [u for u in range(_nunits(lv_or_s)) if lv_or_s.get('units', u, 'owner') == owner and
            (lv_or_s.get('units', u, 'x'), lv_or_s.get('units', u, 'y'), lv_or_s.get('units', u, 'plane')) ==
            (x, y, plane)]


def _answer_by_hotkey(lv, log=None, timeout=60, until=None):
    """Answer the boxes over the map by their controls: Y if offered, A (a hire screen's Accept), the items award,
    else Esc (the lair / treasure notices close on Esc only, UI_MAP). Stops on the idle map or when until() is
    true. -> the hotkeys pressed."""
    pressed = []
    t0 = time.time()
    time.sleep(2)
    while time.time() - t0 < timeout:
        if until is not None and until():
            break
        if mom.on_map(lv):
            time.sleep(1)
            if mom.on_map(lv):
                break
            continue
        keys = {c[4].upper() for c in mom.controls(lv)}
        if 'Y' in keys:
            k = 'y'
        elif 'A' in keys:
            k = 'a'
        elif mom.items_award(lv):
            pressed.append('award')
            continue
        else:
            k = 'esc'
        pressed.append(k)
        if log is not None and len(pressed) <= 12:
            mom.shot('box%d' % len(pressed), log)
        lv.D.key(k)
        time.sleep(2)
    return pressed


def _after_turn(d, turn0, arm, base):
    """A poll() that runs arm() (breakpoints, d.follow) once game_turn has passed turn0 - near the END of
    Next_Turn, before the human's finished spell is cast (MOVE.C) - then d.arm(); base() polls meanwhile."""
    done = [False]

    def poll():
        if not done[0] and d.lv.turn() > turn0:
            done[0] = True
            arm()
            d.arm()
        base()
    return poll


FORT = (29, 27, 0)                              # Dracon, the human's fortress


@case(466, 'OV', 'a prisoner hero is created at the fortress: a full fortress stack loses a unit',
      setup='Dracon (the fortress) holds 9 human units (2 moved in from (14,29)); lair 0 at (30,28) without guards, '
            'prisoner flag set (bit 0), no other treasure; unit 1 (a single unit) at (31,28) walks in; the prisoner '
            'accepted (A); the fortress square and the bumped unit read after',
      bug='the hero is made at the fortress (10th unit) and Stack_Limit_Enforce pushes a fortress unit out before '
          'the hero moves to the lair', fixed='the fortress keeps its 9 units')
def c466(log):
    s = mom.SaveFile()
    extra = [u for u in range(s.get('num_units')) if s.get('units', u, 'owner') == 0 and
             (s.get('units', u, 'x'), s.get('units', u, 'y'), s.get('units', u, 'plane')) == (14, 29, 1)]
    for u in extra[:9 - len(_units_at(s, *FORT))]:
        for f, v in zip(('x', 'y', 'plane'), FORT):
            s.set('units', u, f, v)
    walker = 1
    for f, v in (('x', 31), ('y', 28), ('plane', 0)):
        s.set('units', walker, f, v)
    for f, v in (('x', 30), ('y', 28), ('plane', 0), ('active', 1), ('guard1_count', 0), ('guard2_count', 0),
                 ('gold', 0), ('mana', 0), ('special', 0), ('flags', 1 << N('LAIR_BIT_PRISONER')),
                 ('item_count', 0)):
        s.set('lairs', 0, f, v)
    key = mom.setup_attack(s, walker, [], (30, 28, 0))
    for u in range(s.get('num_units')):           # Patrol alone does not stop the game selecting a unit
        if s.get('units', u, 'owner') == 0 and u != walker:   # (README sec 6): finished too
            s.set('units', u, 'finished', 1)
    fort0 = _units_at(s, *FORT)
    for u in fort0:                                 # Stack_Limit_Enforce bumps the LAST unit of the lowest upkeep +
        if s.get('units', u, 'hero_slot') == -1:    # level: the new hero (last) loses ties, so make the fortress's
            s.set('units', u, 'level', 0)           # normal units level 0 (run 1, 10/23: the hero bumped itself)
            s.set('units', u, 'experience', 0)
    n0 = s.get('num_units')
    import dbg
    d = dbg.boot(s, log=log)
    pressed = []
    try:
        lv = d.lv
        for f in ('Stack_Limit_Enforce', 'Unit_Bump_To_Adjacent'):
            d.follow.add(d.ghidra(f)[0])

        def where_now(h):
            u = d.arg(0, h)
            return (u, lv.get('units', u, 'type'), lv.get('units', u, 'x'), lv.get('units', u, 'y'),
                    lv.get('units', u, 'level'))
        d.want('Stack_Limit_Enforce', trace=where_now)
        d.want('Unit_Bump_To_Adjacent', trace=where_now)
        k = [0]

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                lv.D.key(key)
            elif k[0] % 2 == 0 and not mom.on_map(lv):
                keys = {c[4].upper() for c in mom.controls(lv)}
                kk = 'y' if 'Y' in keys else 'a' if 'A' in keys else 'esc'
                pressed.append(kk)
                d.press(kk)
        d.wait(120, poll)
        mom.shot('after', log)
        fort1 = _units_at(lv, *FORT)
        gone = [u for u in fort0 if u not in fort1]
        where = {u: (lv.get('units', u, 'x'), lv.get('units', u, 'y')) for u in gone}
        n1 = lv.glob('num_units')
        hero = n1 - 1 if n1 > n0 else None
        hxy = hero is not None and (lv.get('units', hero, 'x'), lv.get('units', hero, 'y'),
                                    lv.get('units', hero, 'type'))
        tr = dict(d.traces)
    finally:
        d.stop()
    obs = ('keys %s; fortress units %d -> %d; pushed out %s; units %d -> %d, the new unit at %s; Stack_Limit_Enforce '
           '(unit, type, x, y, level) %s; Unit_Bump_To_Adjacent %s' % (
               pressed, len(fort0), len(fort1), where, n0, n1, hxy, tr.get('Stack_Limit_Enforce'),
               tr.get('Unit_Bump_To_Adjacent')))
    if hero is None or 'a' not in pressed:
        return dict(shown=None, observed=obs + ' - no prisoner hero joined')
    return dict(shown=len(fort0) == 9 and len(fort1) == 8 and len(gone) == 1, observed=obs)


@case(297, 'OV', 'a treaty made in the turn of first contact is erased at the end of that turn',
      setup='wizard 1 contacted both ways but not yet greeted (met_stage 0 both ways, as a mid-turn contact leaves '
            'it), no treaty, relation 100, interests 100; the human opens its audience and proposes a Wizard Pact; '
            'status read after the reply and after the end of the turn',
      bug='status 1 (pact) after the reply, 0 after the turn (Diplo_Mark_Met resets it)', fixed='the pact stays')
def c297(log):
    import cases_dip as C
    s = mom.SaveFile()
    C._met(s, relation=100, interest=100)
    for a, b in ((0, 1), (1, 0)):
        s.set('wizards', a, 'met_stage[%d]' % b, 0)
    lv = mom.boot(s)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    mom.diplo_pick(lv, C.MENU_PROPOSE)
    items = C._menu_items(lv)
    k = next((i for i, t in enumerate(items) if 'Wizard Pact' in t), None)
    if k is None or not mom.diplo_pick_row(lv, k, settle=3.0):
        C._leave(lv)
        return dict(shown=None, observed='no Wizard Pact row: %s' % items)
    mom.shot('reply', log)
    st1 = lv.get('wizards', 0, 'diplomacy_status[1]')
    C._leave(lv)
    lv.D.key('esc')
    time.sleep(1.5)
    turn = mom.end_turn(lv, log=log)
    st2 = lv.get('wizards', 0, 'diplomacy_status[1]')
    st2b = lv.get('wizards', 1, 'diplomacy_status[0]')
    ms = (lv.get('wizards', 0, 'met_stage[1]'), lv.get('wizards', 1, 'met_stage[0]'))
    obs = ('status after the reply %d (1 pact); after the turn (now %d): %d / %d (both ways), met_stage %s'
           % (st1, turn, st2, st2b, ms))
    if st1 != N('TREATY_WIZARD_PACT'):
        return dict(shown=None, observed=obs + ' - the pact was not signed')
    return dict(shown=st2 == 0 and st2b == 0, observed=obs)


@case(287, 'OV', 'no random events for 5 turns after every load: the event cooldown is not saved',
      setup='the base save with p_events.last_event_turn = 0 (106 turns without an event); event_cooldown (DS:4a32) '
            'read after the load; breakpoint at "if (Random(512) <= chance)" (Event_Roll, EVENTGEN.C): chance '
            '[bp-2] and event_cooldown at the first turn change',
      bug='event_cooldown 0 after the load (the DS start value), counted up to 1; chance forced to 0',
      fixed='chance 106 x 3/4 (Normal) - an event possible at once')
def c287(log):
    import dbg
    s = mom.SaveFile()
    s.set('events', 0, 'last_event_turn', 0)
    d = dbg.boot(s, log=log)
    try:
        c0 = d.lv.glob('event_cooldown')
        d.want_line('src/wizards/EVENTGEN.C', 'if (Random(512) <= chance) {', label='roll')
        h = d.wait(300, d.turns_poll(s.get('game_turn') + 1))
        if h is None:
            return dict(shown=None, observed='cooldown after the load %d; the roll line was not reached' % c0)
        chance = d.local(-2, h)
        c1 = d.lv.glob('event_cooldown')
        diff = d.lv.glob('difficulty')
        d.drop('roll')
        d.go()
    finally:
        d.stop()
    obs = ('event_cooldown after the load %d, at the roll %d; chance %d (turns since the last event %d, difficulty '
           '%d)' % (c0, c1, chance, s.get('game_turn'), diff))
    return dict(shown=c0 == 0 and c1 == 1 and chance == 0, observed=obs)


@case(408, 'OV', 'Overland_Cast_Spell keeps Death Wish\'s missing return value as its "ok": Nature\'s Wrath skipped',
      setup='the human has finished casting Death Wish (casting_spell, cost left 0, mana 5000); wizard 1 has Nature\'s '
            'Wrath (globals.nature_s_wrath 2); the spell is cast at the start of the next turn (MOVE.C); breakpoint on '
            'Death_Wish_Cast\'s final RETF (af2c:0071 + 0x23b, 572 bytes, re/fn_census.tsv): AX there is what '
            'Overland_Cast_Spell stores as ok; a tracepoint on Natures_Wrath_Strike (OVERCAST.C: only "if (ok == 1)" '
            'gates it for a Chaos / Death spell)',
      bug='ok = the AX the last statement left (0 here): the cast went through, but Nature\'s Wrath does not strike',
      fixed='Death Wish returns 1: Nature\'s Wrath strikes the caster')
def c408(log):
    import dbg
    import cases_dbg1 as D1
    s = mom.SaveFile()
    D1._ai_casts(s, 0, N('SPELL_DEATH_WISH'))
    s.set('wizards', 1, 'globals.nature_s_wrath', 2)
    s.set('wizards', 1, 'mana_reserve', 5000)        # its upkeep must not cancel it before the human's cast
    d = dbg.boot(s, log=log)
    try:
        def arm():                                   # armed once the turn has advanced (the followed modules are
            d.follow = {0xaf2c, d.ghidra('Natures_Wrath_Strike')[0]}   # hot in the AI turn); the human's spell is
            d.ovr_always = True                      # an overlay can be evicted while on the stack and reloaded
            #                                          elsewhere on return (Death Wish calls the animation's overlay)
            d.want('Death_Wish_Cast', off=0x23b, label='retf')         # cast at the start of the next turn
            d.want('Natures_Wrath_Strike', trace=lambda h: d.arg(0, h))
        h = d.wait(400, _after_turn(d, s.get('game_turn'), arm, d.turns_poll(s.get('game_turn') + 2)))
        if h is None:
            return dict(shown=None, observed='Death_Wish_Cast\'s return was not reached')
        ax = h['AX']
        wrath = d.lv.get('wizards', 1, 'globals.nature_s_wrath')   # at the cast, before the ok gate
        cs, ip = d.csip()
        ins = 'byte 0x%02X at %04X:%04X, 0xCB = RETF' % (d.lv.g.ram(cs * 16 + ip, 1)[0], cs, ip)
        d.drop('retf')
        d.wait(30, d.turns_poll(s.get('game_turn') + 2))   # the rest of the cast: would Nature's Wrath strike?
        strikes = d.traces.get('Natures_Wrath_Strike', [])
    finally:
        d.stop()
    sax = ax - 0x10000 if ax & 0x8000 else ax
    obs = ('at Death_Wish_Cast\'s RETF (%s): AX = %d (0x%04X), the value Overland_Cast_Spell takes as ok; wizard 1\'s '
           'Nature\'s Wrath %d at that moment; Natures_Wrath_Strike calls (caster) %s' % (ins.strip(), sax, ax, wrath,
                                                                                         strikes))
    if not wrath:
        return dict(shown=None, observed=obs + ' - wizard 1 had no Nature\'s Wrath at the cast')
    return dict(shown=sax != 1 and 0 not in strikes, observed=obs)


@case(245, 'OV', 'Summon Champion with no champion left gets ONE try at the whole hero pool',
      setup='every champion (hero types 24 .. 33) unavailable for the human (hero_stats status -20); the human has '
            'finished casting Summon Champion (cost left 0); at the turn change a tracepoint on "tries++;" in '
            'Hero_Pick_Random (WIZSTATS.C) counts the tries, and its return value is read',
      bug='101 tries: 100 in the champion pool, then the whole-pool retry runs once (tries not reset)',
      fixed='up to 100 more tries in the whole pool')
def c245(log):
    import dbg
    import cases_dbg1 as D1
    s = mom.SaveFile()
    for h in range(24, 34):
        s.set('hero_stats', h, 'status', N('HERO_STATUS_UNAVAILABLE'))
    D1._ai_casts(s, 0, N('SPELL_SUMMON_CHAMPION'))
    d = dbg.boot(s, log=log)
    try:
        def arm():                                   # armed only once the turn has advanced: WIZSTATS is hot in
            d.follow = {0xa0ec}                      # the AI turn (followed thunks there made it crawl, 10/23); the
            d.ovr_always = True
            d.want('Overland_Cast_Spell',
                   trace=lambda h: (d.arg(0, h), d.lv.get('wizards', d.arg(0, h), 'casting_spell')))
            d.want('Summon_Hero_Cast', trace=lambda h: (d.arg(0, h), d.arg(1, h)))
            d.want_line('src/wizards/WIZSTATS.C', 'tries++;', label='try', trace=lambda h: 1)   # human's spell is
            d.want('Hero_Pick_Random', label='entry', trace=lambda h: (d.arg(1, h), d.arg(2, h)))   # cast after
        d.wait(400, _after_turn(d, s.get('game_turn'), arm, d.turns_poll(s.get('game_turn') + 2)))
        tries = len(d.traces.get('try', []))
        calls = d.traces.get('entry', [])
    finally:
        d.stop()
    obs = ('Overland_Cast_Spell (player, spell) %s; Summon_Hero_Cast %s; Hero_Pick_Random calls (mode, pool) %s; '
           'tries counted %d' % (d.traces.get('Overland_Cast_Spell'), d.traces.get('Summon_Hero_Cast'), calls, tries))
    if not calls:
        return dict(shown=None, observed=obs + ' - not called')
    return dict(shown=len(calls) == 1 and tries == 101, observed=obs)


def _battle_terrain(tile, log, tag):
    """Dracon's stack attacks two neutral units on (30,28) whose terrain tile is set to `tile`; breakpoint on
    Load_Combat_Terrain_Tiles (BFIELD.C): its bf_terrain argument. -> (bf_terrain, Tile_Is_Tundra's verdict
    implied) or None."""
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    s.set('units', 2, 'owner', N('PLAYER_NEUTRAL'))
    s.set('units', 13, 'owner', N('PLAYER_NEUTRAL'))
    s.set('terrain', 28 * N('MAP_WIDTH') + 30, value=tile)
    d = dbg.boot(s, log=log)
    try:
        d.follow = {0xf900}                         # BFIELD: entered for other calls first, moved before this one
        d.want('Load_Combat_Terrain_Tiles', label='load')
        k = [0]

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                d.lv.D.key(key)
            elif k[0] % 3 == 0 and not mom.on_map(d.lv):
                d.press('y')                        # 'still wish to attack?' boxes
        h = d.wait(90, poll)
        if h is None:
            return None
        t = d.arg(0, h)
        d.drop('load')
        d.go()
        time.sleep(4)
        mom.shot(tag, log)
        mom.finish_combat(d.lv, log=log)
    finally:
        d.stop()
    return t


@case(410, 'OV', 'tile 602 (a tundra edge picture) is not tundra to Tile_Is_Tundra: a land battle there gets the '
                 'OCEAN battlefield',
      setup='two neutral units on (30,28) next to Dracon, whose terrain tile is set to 602 (then, as the control, '
            '603); '
            'Dracon\'s stack attacks; breakpoint on Load_Combat_Terrain_Tiles (BFIELD.C) reads the battlefield kind '
            'Battlefield_Generate chose (BFGEN.C: forest .. tundra tests, else ocean)',
      bug='602: BF_TERRAIN_OCEAN; 603: BF_TERRAIN_TUNDRA', fixed='602: tundra')
def c410(log):
    a = _battle_terrain(602, log, 'tile602')
    b = _battle_terrain(603, log, 'tile603')
    obs = 'battlefield kind: tile 602 -> %s, tile 603 -> %s (ocean %d, tundra %d)' % (
        a, b, N('BF_TERRAIN_OCEAN'), N('BF_TERRAIN_TUNDRA'))
    if a is None or b is None:
        return dict(shown=None, observed=obs + ' - a battle did not start')
    return dict(shown=a == N('BF_TERRAIN_OCEAN') and b == N('BF_TERRAIN_TUNDRA'), observed=obs)


NATURE, LIFE = 0, 3


@case(282, 'OV', 'Diplo_Tradeable_Spells gathers every candidate spell into a 60-byte local list with no bound',
      setup='the human given one Nature book (spellbooks[0] = 1) and its Life spells unlearned; wizard 1 given Life '
            'books and every Nature, Life and Arcane spell (but Spell of Return); met, interests 100; the human '
            'opens Exchange Spells; breakpoint on "count = 0;" after the gathering loop of Diplo_Tradeable_Spells '
            '(TREASURE.C), the receiver argument the human: n (register DI) read; the session stopped there',
      bug='n = 93 > 60: list[60] at [bp-72] is written to [bp+20] - over the other locals, the saved BP and the '
          'return address', fixed='at most 60 gathered')
def c282(log):
    import dbg
    import cases_dip as C
    s = mom.SaveFile()
    C._met(s, interest=100)
    known = N('SPELLSTATE_KNOWN')
    s.set('wizards', 0, 'spellbooks[%d]' % NATURE, 1)
    s.set('wizards', 1, 'spellbooks[%d]' % LIFE, 5)
    for sp in list(range(NATURE * 40 + 1, NATURE * 40 + 41)) + list(range(LIFE * 40 + 1, LIFE * 40 + 41)) + \
            list(range(201, 214)):
        s.set('wizards', 1, C._lib(sp), known)
        s.set('wizards', 0, C._lib(sp), N('SPELLSTATE_RESEARCHABLE'))
    d = dbg.boot(s, log=log)
    try:
        if not mom.diplo_open(d.lv, 1, log=log):
            mom.shot('no_menu', log)
            return dict(shown=None, observed='the diplomacy menu did not come up')
        d.want_line('src/wizards/TREASURE.C', 'count = 0;', fn='Diplo_Tradeable_Spells', label='gathered',
                    when=lambda h: h['DI'] > 60)   # the overrun itself rewrites the arguments at [bp+6..]: test n
        mom.diplo_pick(d.lv, C.MENU_EXCHANGE, settle=0.5)
        h = d.wait(30, None)
        if h is None:
            return dict(shown=None, observed='the gathering loop was not reached')
        n = h['DI']
        args = tuple(d.word(h['SS'], h['BP'] + 6 + 2 * k) for k in range(3))
    finally:
        d.stop()
    obs = 'Diplo_Tradeable_Spells(receiver %d, giver %d, min %d): %d spells gathered into list[60]' % (args + (n,))
    return dict(shown=n > 60, observed=obs)


def _walker_lair(s, walker=1, flags=0, special=0, lair_type=None):
    """Save edit: lair 0 at (30,28) without guards (treasure as given), unit `walker` alone at (31,28) to walk in
    (keypad 4, west), every other human unit on patrol AND finished. -> the key."""
    for f, v in (('x', 31), ('y', 28), ('plane', 0)):
        s.set('units', walker, f, v)
    for f, v in (('x', 30), ('y', 28), ('plane', 0), ('active', 1), ('guard1_count', 0), ('guard2_count', 0),
                 ('gold', 0), ('mana', 0), ('special', special), ('flags', flags), ('item_count', 0)):
        s.set('lairs', 0, f, v)
    if lair_type is not None:
        s.set('lairs', 0, 'type', lair_type)
    key = mom.setup_attack(s, walker, [], (30, 28, 0))
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u != walker:
            s.set('units', u, 'finished', 1)
    return key


@case(238, 'OV', 'a spellbook found with 11+ books in its realm indexes past the 10-row grant table',
      setup='the human: 11 Life books (11 books in all, so a book special is not turned into a retort), 6 retorts '
            '(retort_count > 5 forces the book roll); an empty Ancient Temple (Life books) next to unit 1 with '
            'special 5 (one book); unit 1 walks in; breakpoint on Wizard_Gain_Spellbook (TREASURE.C): player, realm '
            'and spellbooks[realm]; the Life spell states before / after',
      bug='called with 11 Life books: tier_counts[11][0..3] is the return address and the arguments, '
          'decremented once per spell granted - the book goes to another realm, the return CS is changed',
      fixed='no grant past 10 books')
def c238(log):
    import dbg
    import cases_dbg1 as D1
    s = mom.SaveFile()
    s.set('wizards', 0, 'spellbooks[3]', 11)
    for r in range(6):
        s.set('wizards', 0, 'retorts[%d]' % r, 1)
    key = _walker_lair(s, special=5, lair_type=N('LAIR_ANCIENT_TEMPLE'))
    before = [s.get('wizards', 0, 'spell_library[3][%d]' % k) for k in range(40)]
    d = dbg.boot(s, log=log)
    try:
        d.want('Wizard_Gain_Spellbook', label='gain')
        h = d.wait(90, D1._walk_in_poll(d, key))
        if h is None:
            return dict(shown=None, observed='Wizard_Gain_Spellbook was not called')
        player, realm = d.arg(0, h), d.arg(1, h)
        books = d.lv.get('wizards', player, 'spellbooks[%d]' % realm)
        sp = h['SP']                                 # entry: [SP] return IP, +2 return CS, +4 player, +6 realm
        frame0 = [d.word(h['SS'], sp + k, False) for k in (0, 2, 4, 6)]
        d.drop('gain')
        d.want_line('src/wizards/TREASURE.C', 'wizards[player].spellbooks[realm]++;', label='inc')
        h2 = d.wait(20, None)                       # tier_counts[11][0..3] = [bp+2 .. +8] = that same frame
        frame1 = h2 and [d.word(h2['SS'], h2['BP'] + k, False) for k in (2, 4, 6, 8)]
        if h2:
            d.drop('inc')
            d.go()
        d.wait(20, D1._walk_in_poll(d, 'esc'))
        after = [d.lv.get('wizards', 0, 'spell_library[3][%d]' % k) for k in range(40)]
        books1 = [d.lv.get('wizards', 0, 'spellbooks[%d]' % r) for r in range(5)]
    finally:
        d.stop()
    changed = [(k, a, b) for k, (a, b) in enumerate(zip(before, after)) if a != b]
    obs = ('Wizard_Gain_Spellbook(player %d, realm %d) with %d books; tier_counts[11][0..3] = the return IP / CS and '
           'the player / realm arguments: %s at entry, %s at the "spellbooks[realm]++" line; the human\'s books after '
           '(Nature .. Death) %s (Life was 11, Chaos 0); Life spell states changed (slot, before, after) %s'
           % (player, realm, books, frame0, frame1, books1, changed))
    return dict(shown=player == 0 and realm == 3 and books >= 11 and frame1 is not None and frame1 != frame0,
                observed=obs)


DIS_AREA = 204                                  # Disenchant Area (Arcane: tradeable for everyone)


@case(315, 'OV', 'conquering a wizard\'s fortress: the looted spells\' names are copied over the start / end of DS',
      setup='wizard 1\'s fortress (49,14 Myrror) emptied (its 9 units marked dead, owner PLAYER_NONE); wizard 1 knows '
            'Disenchant Area, the human does not (Arcane: tradeable); human unit 122 on (50,14) walks in (no battle); '
            'the boxes answered (N to "destroy the city?", Esc / Y otherwise); DS:0000..00FF and DS:FF00..FFFF read '
            'before and after; message_text read',
      bug='"Disenchant Area" written at DS:0000..007F or DS:FF80..FFFF (far_strcpy gets the first byte of a char '
          'local as the buffer address)', fixed='the name only in the message')
def c315(log):
    s = mom.SaveFile()
    for u in range(s.get('num_units')):
        if (s.get('units', u, 'x'), s.get('units', u, 'y'), s.get('units', u, 'plane')) == (49, 14, 1):
            s.set('units', u, 'owner', N('PLAYER_NONE'))
    import cases_dip as C
    s.set('wizards', 1, C._lib(DIS_AREA), N('SPELLSTATE_KNOWN'))
    s.set('wizards', 0, C._lib(DIS_AREA), N('SPELLSTATE_RESEARCHABLE'))
    walker = 122
    for f, v in (('x', 50), ('y', 14), ('plane', 1)):
        s.set('units', walker, f, v)
    key = mom.setup_attack(s, walker, [], (49, 14, 1))
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u != walker:
            s.set('units', u, 'finished', 1)
    lv = mom.boot(s)
    ds = lv.g.ds
    lo0, hi0 = lv.g.ram(ds, 0x100), lv.g.ram(ds + 0xff00, 0x100)
    lv.D.key(key)
    pressed = []
    t0 = time.time()
    time.sleep(3)
    while time.time() - t0 < 120:
        if mom.on_map(lv) and lv.get('cities', 1, 'owner') == 0:
            break
        if not mom.on_map(lv):
            keys = {c[4].upper() for c in mom.controls(lv)}
            k = 'n' if 'R' in keys and 'N' in keys else 'y' if 'Y' in keys else 'esc'
            pressed.append(k)
            if len(pressed) <= 10:
                mom.shot('box%d' % len(pressed), log)
            lv.D.key(k)
        time.sleep(2)
    lo1, hi1 = lv.g.ram(ds, 0x100), lv.g.ram(ds + 0xff00, 0x100)
    msg = lv.g.ram(ds + mom.SYM['message_text'][0], 160).split(b'\0')[0].decode('latin-1')
    owner = lv.get('cities', 1, 'owner')
    name = b'Disenchant Area'
    where = ['DS:%04X' % i for i in range(0x100 - len(name)) if lo1[i:i + len(name)] == name] + \
            ['DS:%04X' % (0xff00 + i) for i in range(0x100 - len(name)) if hi1[i:i + len(name)] == name]
    diff_lo = sum(a != b for a, b in zip(lo0, lo1))
    diff_hi = sum(a != b for a, b in zip(hi0, hi1))
    obs = ('keys %s; city 1 owner %d (0 = human); message "%s"; "Disenchant Area" found at %s; bytes changed '
           'DS:0000-00FF %d, DS:FF00-FFFF %d' % (pressed, owner, msg, where or 'nowhere', diff_lo, diff_hi))
    if 'Disenchant' not in msg:
        return dict(shown=None, observed=obs + ' - the loot message did not come')
    return dict(shown=bool(where), observed=obs)


@case(285, 'OV', 'Wait never hands over to the other plane: the selection keeps cycling the waiting units of one plane',
      setup='every human unit finished except Dracon\'s stack (29,27) and unit 1 (36,24) on Arcanus and the stack at '
            '(14,29) on Myrror (all ready); W (Wait) pressed 10 times; after each, map_view_plane and the selected '
            'stack\'s first unit (stack_units[0], DS:999e) read',
      bug='the selection stays on one plane (the waiting units there are made ready again) - the other plane\'s '
          'ready units are never offered', fixed='after the last unit here waits, the other plane\'s units')
def c285(log):
    import struct
    s = mom.SaveFile()
    ready = set()
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') != 0:
            continue
        xyp = (s.get('units', u, 'x'), s.get('units', u, 'y'), s.get('units', u, 'plane'))
        if u == 1 or xyp in ((29, 27, 0), (14, 29, 1)):
            for f, v in (('order', N('ORDER_READY')), ('finished', 0)):
                s.set('units', u, f, v)
            s.set('units', u, 'moves_left', s.get('units', u, 'moves_max'))
            ready.add(xyp)
        else:
            s.set('units', u, 'order', N('ORDER_PATROL'))
            s.set('units', u, 'finished', 1)
    lv = mom.boot(s)
    seen = []
    ds = lv.g.ds + mom.SYM['stack_units'][0]

    def sel():
        u = struct.unpack('<h', lv.g.ram(ds, 2))[0]
        return (lv.glob('map_view_plane'), u, lv.get('units', u, 'plane'))
    seen.append(sel())
    for _ in range(10):
        lv.D.key('w')
        time.sleep(1.5)
        seen.append(sel())
    planes = {p for _, _, p in seen}
    obs = 'ready stacks at %s; after each W (view plane, unit, its plane): %s' % (sorted(ready), seen)
    if len(ready) < 3:
        return dict(shown=None, observed=obs + ' - the setup did not leave stacks on both planes')
    return dict(shown=len(planes) == 1, observed=obs)


@case(213, 'OV', 'a Chaos Channeled unit counts as a normal living unit: normal-unit enchantments still accept it',
      setup='human unit 161 (a Lizardman, a normal unit) moved alone to (27,28) and given the Chaos Channels demon '
            'wings (mutation_flags 0x08), finished; the human '
            'finishes Heroism (130; a normal-units-only enchantment, SPELL_FX_NORMAL_UNIT_ENCH) and targets that '
            'square; the unit\'s enchantments read after',
      bug='accepted: the unit gets Heroism (Unit_Is_Normal_Living tests only fantastic types and Undead)',
      fixed='"Only normal units may have Heroism cast on them"')
def c213(log):
    import cases_tgt as T
    s = mom.SaveFile()
    for f, v in (('x', 27), ('y', 28), ('plane', 0), ('mutation_flags', N('MUT_CHAOS_DEMON_WINGS')),
                 ('order', N('ORDER_PATROL')), ('finished', 1)):
        s.set('units', 161, f, v)
    for f, v in (('casting_spell', N('SPELL_HEROISM')), ('casting_cost_left', 0), ('casting_cost_initial', 200),
                 ('mana_reserve', 5000)):
        s.set('wizards', 0, f, v)
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    mom.target_square(lv, 27, 28, log=log)
    mom.shot('after_target', log)
    time.sleep(1.5)
    lv.g.click(120, 108, settle=2.0)                # the unit list popup ('Dracon Units'): its row
    mom.shot('after_pick', log)
    for _ in range(3):
        time.sleep(1.5)
        lv.D.key('enter')
    u = T._find_unit(lv, 0, 27, 28)
    if u is None:
        return dict(shown=None, observed='the unit was not found after the cast')
    e = lv.get('units', u, 'enchantments') & 0xffffffff
    m = lv.get('units', u, 'mutation_flags')
    obs = 'unit %d at (27,28): mutation_flags 0x%02x, enchantments 0x%08x (Heroism 0x%08x); casting now %d' % (
        u, m, e, N('UNIT_ENCH_HEROISM'), lv.get('wizards', 0, 'casting_spell'))
    if lv.get('wizards', 0, 'casting_spell') == N('SPELL_HEROISM'):
        return dict(shown=None, observed=obs + ' - the spell was not cast (still casting)')
    if not m & N('MUT_CHAOS_DEMON_WINGS'):
        return dict(shown=None, observed=obs + ' - the mutation was not there')
    return dict(shown=bool(e & N('UNIT_ENCH_HEROISM')), observed=obs)


def _wraith_pair_move(a_type, log, tag):
    """Units 173 (a_type, Wraith Form) and 192 (Draconian Spearmen, a plain flyer) alone on (31,28), every other human
    unit finished; the stack moves east onto the water square (32,28). -> where the two units are after."""
    s = mom.SaveFile()
    a, c = 173, 192
    for u, typ, ench in ((a, a_type, N('UNIT_ENCH_WRAITH_FORM')), (c, 66, 0)):
        for f, v in (('x', 31), ('y', 28), ('plane', 0), ('type', typ), ('enchantments', ench), ('level', 0),
                     ('experience', 0)):
            s.set('units', u, f, v)
    key = mom.setup_attack(s, a, [], (32, 28, 0))
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u not in (a, c):
            s.set('units', u, 'finished', 1)
    for f, v in (('order', N('ORDER_READY')), ('finished', 0)):
        s.set('units', c, f, v)
    s.set('units', c, 'moves_left', s.get('units', c, 'moves_max'))
    lv = mom.boot(s)
    lv.D.key(key)
    time.sleep(4)
    mom.shot(tag, log)
    return [(lv.get('units', u, 'x'), lv.get('units', u, 'y')) for u in (a, c)]


@case(487, 'OV', 'Stack_Movement_Modes: a Wraith Form unit loses a swimmer for every mode its type has (&& / || mixed)',
      setup='a stack of two at (31,28): a Wraith Form unit and a Draconian Spearmen (flying, no Wind Walking); it '
            'moves east onto water (32,28). Run 1 (control): the Wraith Form unit is Spearmen (type 39, no movement '
            'mode): 1 swimmer + 1 flyer = the stack -> swimming mode. Run 2: it is Cavalry (type 42, mode cavalry): '
            'the misgrouped test also takes a swimmer off for the cavalry mode -> 0 swimmers',
      bug='run 1 enters the water, run 2 is refused', fixed='both enter the water')
def c487(log):
    a = _wraith_pair_move(39, log, 'control')
    b = _wraith_pair_move(42, log, 'cavalry')
    obs = 'positions after the move east: control (Spearmen + flyer) %s, cavalry (Cavalry + flyer) %s' % (a, b)
    if a != [(32, 28), (32, 28)]:
        return dict(shown=None, observed=obs + ' - the control stack did not enter the water')
    return dict(shown=b == [(31, 28), (31, 28)], observed=obs)


GATE_CITY = 21                                  # wizard 1's city at (53,16) Myrror; Arcanus (53,16) is open water


def _gate_shift(n, ench, log, tag):
    """Wizard 1's city 21 given an Astral Gate; n of its units kept there (type 39 Spearmen, enchantments `ench`),
    the rest moved to (50,12). At the AI turn AI_Plane_Shift_Stacks shifts a stack in an own Astral Gate city to the
    other plane when Stack_Can_Plane_Shift (ORDERS.C) says 1. -> the planes of the kept units after the turn."""
    s = mom.SaveFile()
    s.set('cities', GATE_CITY, 'enchants[%d]' % N('CITY_ENCH_ASTRAL_GATE'), 2)
    here = [u for u in range(s.get('num_units')) if (s.get('units', u, 'x'), s.get('units', u, 'y'),
                                                      s.get('units', u, 'plane')) == (53, 16, 1)]
    for k, u in enumerate(here):
        if k < n:
            for f, v in (('type', 39), ('enchantments', ench), ('hero_slot', -1), ('mutation_flags', 0)):
                s.set('units', u, f, v)
        else:
            s.set('units', u, 'x', 50)
            s.set('units', u, 'y', 12)
    keep = here[:n]
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    mom.shot(tag, log)
    planes = []
    for u in range(lv.glob('num_units')):           # the unit table is compacted at the turn end: find by owner /
        if lv.get('units', u, 'owner') == 1 and lv.get('units', u, 'type') == 39 and \
                lv.get('units', u, 'enchantments') & ench:   # type / enchantment (a shifted stack may move on)
            planes.append((lv.get('units', u, 'plane'), lv.get('units', u, 'x'), lv.get('units', u, 'y')))
    log.append('  %s: kept units after the turn (plane, x, y) %s' % (tag, planes))
    return len(keep), sorted(p for p, _, _ in planes)


@case(471, 'OV', 'the AI\'s plane-shift test ignores Non-Corporeal / Wraith Form: such a stack cannot shift onto water',
      setup='wizard 1\'s city 21 (53,16 Myrror, Arcanus square open water) given an Astral Gate; at the AI turn '
            'AI_Plane_Shift_Stacks shifts its stack there when Stack_Can_Plane_Shift says 1. Three runs, the city\'s '
            'other units moved out: (A) 4 Spearmen with Wraith Form; (B) 4 with Water Walking (control); (C) 9 with '
            'Water Walking (#174: exactly 9)', bug='A stays on Myrror, B shifts to Arcanus; C stays (#174)',
      fixed='A shifts too; C shifts', also=(174,))
def c471(log):
    a = _gate_shift(4, N('UNIT_ENCH_WRAITH_FORM'), log, 'wraith')
    b = _gate_shift(4, N('UNIT_ENCH_WATER_WALKING'), log, 'waterwalk')
    c = _gate_shift(9, N('UNIT_ENCH_WATER_WALKING'), log, 'nine')
    obs = ('units kept / their planes after the turn (1 Myrror, 0 Arcanus): Wraith Form %s, Water Walking %s, nine '
           'Water Walking %s' % (a, b, c))
    shifted = b[1] == [0] * b[0]
    also = {174: (None if not shifted or len(c[1]) != 9 else c[1] == [1] * 9, obs)}
    if not shifted:
        return dict(shown=None, observed=obs + ' - the control stack did not shift', also=also)
    if len(a[1]) != 4:
        return dict(shown=None, observed=obs + ' - the Wraith Form units were not all found', also=also)
    return dict(shown=a[1] == [1] * 4, observed=obs, also=also)


def _active_flags(lv):
    import struct
    base = lv.g.ds + mom.SYM['stack_units'][0]
    n = lv.glob('stack_unit_count')
    raw = lv.g.ram(base, 4 * max(n, 1))
    return [struct.unpack_from('<hh', raw, 4 * k) for k in range(n)]


@case(277, 'OV', 'a partial stack is refused a plane shift for the units it leaves behind',
      setup='six human units with Planar Travel on Arcanus (48,11), four human units on Myrror (48,11); the six are '
            'selected, four of them deselected in the stack panel (clicks on their icons), then Plane (290,9); '
            'stack_units read before the click, the units\' planes after',
      bug='"cannot Planar Travel": stack_unit_count 6 + 4 > 9 although only 2 move (2 + 4 = 6)',
      fixed='the two active units shift')
def c277(log):
    s = mom.SaveFile()
    here, there = [107, 161, 173, 192, 1, 0], [122, 130, 147, 167]
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'order', N('ORDER_PATROL'))
            s.set('units', u, 'finished', 1)
    for u in here + there:
        for f, v in (('x', 48), ('y', 11), ('plane', 0 if u in here else 1), ('hero_slot', -1), ('type', 39),
                     ('level', 0)):
            s.set('units', u, f, v)
    for u in here:
        for f, v in (('enchantments', N('UNIT_ENCH_PLANAR_TRAVEL')), ('order', N('ORDER_READY')), ('finished', 0)):
            s.set('units', u, f, v)
        s.set('units', u, 'moves_left', s.get('units', u, 'moves_max'))
    lv = mom.boot(s)
    time.sleep(2)
    icons = sorted((c for c in mom.controls(lv) if c[0] >= 240 and 60 <= c[1] <= 175 and 15 <= c[2] - c[0] <= 30
                    and 15 <= c[3] - c[1] <= 35), key=lambda c: (c[1], c[0]))
    before = _active_flags(lv)
    lv.g.click((icons[0][0] + icons[0][2]) // 2, (icons[0][1] + icons[0][3]) // 2, settle=1.0)   # the first click
    for c in icons[1:]:                             # left ONE unit active (run 2): add units back one by one
        if sum(1 for _, a in _active_flags(lv) if a) >= 2:
            break
        lv.g.click((c[0] + c[2]) // 2, (c[1] + c[3]) // 2, settle=1.0)
    flags = _active_flags(lv)
    mom.shot('deselected', log)
    lv.g.click(290, 9, settle=2.5)
    mom.shot('plane', log)
    planes = {u: lv.get('units', u, 'plane') for u in here}
    lv.D.key('esc')
    n_active = sum(1 for _, a in flags if a)
    obs = ('stack panel icons %d; stack_units (unit, active) before %s, after the clicks %s; planes after Plane: %s'
           % (len(icons), before, flags, planes))
    if len(flags) != 6 or n_active != 2:
        return dict(shown=None, observed=obs + ' - the selection was not 2 active of 6')
    return dict(shown=all(p == 0 for p in planes.values()), observed=obs)


def _square_px(lv, x, y):
    """Screen position of map square (x, y) in the overland view (as mom.target_square), or None off the view."""
    W, cols, rows = N('MAP_WIDTH'), N('MAP_VIEW_COLS'), N('MAP_VIEW_ROWS')
    vx, vy = lv.glob('map_view_x'), lv.glob('map_view_y')
    rx, ry = (x - vx) % W, y - vy
    if rx < cols and 0 <= ry < rows:
        return rx * N('MAP_TILE_W') + 10, ry * N('MAP_TILE_H') + N('MAP_VIEW_TOP') + 9
    return None


@case(169, 'OV', 'Go-To ignores the stack selection: every ready / done / Go-To unit on the tile gets the order',
      setup='three human units at (27,28) with no moves left: 107 and 161 Ready, 173 Done (the player told it to '
            'wait out the turn); every other human unit finished; a click on the map at (25,26) with the stack '
            'selected and no moves gives Go-To (OVERLAND.C, Stack_Set_Order ORDER_GOTO, STACKSEL.C: every stack '
            'unit whose order is Done / Go-To / Ready, the active flag not tested); the orders read after',
      bug='all three on Go-To to (25,26), the Done unit included', fixed='the Done (unselected) unit keeps Done')
def c169(log):
    s = mom.SaveFile()
    trio = [107, 161, 173]
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'order', N('ORDER_PATROL'))
            s.set('units', u, 'finished', 1)
    for u in trio:
        for f, v in (('x', 27), ('y', 28), ('plane', 0), ('order', N('ORDER_READY')), ('finished', 0),
                     ('moves_left', 0)):
            s.set('units', u, f, v)
    s.set('units', 173, 'order', N('ORDER_DONE'))
    s.set('units', 173, 'finished', 1)
    lv = mom.boot(s)
    time.sleep(2)
    flags = _active_flags(lv)
    if not {107, 161} <= {u for u, _ in flags}:
        return dict(shown=None, observed='the Ready units were not the selected stack: stack_units %s' % flags)
    px = _square_px(lv, 25, 26)
    if px is None:
        return dict(shown=None, observed='(25,26) is not on the view')
    lv.g.click(px[0], px[1], settle=2.0)
    mom.shot('goto', log)
    orders = {u: (lv.get('units', u, 'order'), lv.get('units', u, 'dest_x'), lv.get('units', u, 'dest_y'))
              for u in trio}
    obs = 'stack_units (unit, active) %s; orders after the click (order, dest x, y) %s (Done %d, Go-To %d)' % (
        flags, orders, N('ORDER_DONE'), N('ORDER_GOTO'))
    if orders[107][0] != N('ORDER_GOTO'):
        return dict(shown=None, observed=obs + ' - no Go-To was given')
    return dict(shown=orders[173][0] == N('ORDER_GOTO') and orders[173][1:] == (25, 26), observed=obs)


@case(202, 'OV', 'after Settle the overland screen re-selects the stack at the last RIGHT-CLICKED square',
      setup='human units 107 and 161 waiting (order Wait) at (27,28), unit 173 made Settlers (type 44) at (25,28), '
            'unit 192 Ready at (30,27) (the normal next unit), every other human unit finished; the stack at '
            '(27,28) is right-clicked (sel_x / sel_y = that square, '
            'OVERLAND.C), W sends it back to waiting and the next unit, the settler, is selected; B founds a city '
            '(the name box answered with Enter); orders of 107 / 161 and the selected stack read before B and after',
      bug='after the settle Stack_Select_At(27,28) collects that tile again: its waiting units woken (Wait -> Ready) '
          'and selected instead of the Ready unit 192', fixed='192 selected; (27,28) keeps waiting')
def c202(log):
    s = mom.SaveFile()
    pair, settler = [107, 161], 173
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'order', N('ORDER_PATROL'))
            s.set('units', u, 'finished', 1)
    for u in pair:
        for f, v in (('x', 27), ('y', 28), ('plane', 0), ('order', N('ORDER_WAIT')), ('finished', 0)):
            s.set('units', u, f, v)
        s.set('units', u, 'moves_left', s.get('units', u, 'moves_max'))
    for f, v in (('x', 25), ('y', 28), ('plane', 0), ('type', 44), ('order', N('ORDER_READY')), ('finished', 0),
                 ('hero_slot', -1), ('level', 0)):
        s.set('units', settler, f, v)
    s.set('units', settler, 'moves_left', 2)
    other = 192                                     # a Ready unit elsewhere: the normal next unit after the settle
    for f, v in (('x', 30), ('y', 27), ('plane', 0), ('order', N('ORDER_READY')), ('finished', 0)):
        s.set('units', other, f, v)
    s.set('units', other, 'moves_left', s.get('units', other, 'moves_max'))
    n0 = s.get('num_cities')
    lv = mom.boot(s)
    time.sleep(2)
    px = _square_px(lv, 27, 28)
    if px is None:
        return dict(shown=None, observed='(27,28) is not on the view')
    lv.g.click(px[0], px[1], right=True, settle=2.0)
    sel1 = [u for u, _ in _active_flags(lv)]
    lv.D.key('esc')                                 # a unit view may open on the right-click
    time.sleep(1)
    lv.D.key('w')
    time.sleep(2)
    sel2 = [u for u, _ in _active_flags(lv)]
    orders_before = {u: lv.get('units', u, 'order') for u in pair}
    if sel2 != [settler]:
        return dict(shown=None, observed='right-click selected %s; after W %s (the settler not selected); orders %s'
                    % (sel1, sel2, orders_before))
    lv.D.key('b')
    time.sleep(3)
    for _ in range(4):                              # the city name box, a message
        if mom.on_map(lv):
            break
        lv.D.key('enter')
        time.sleep(2)
    mom.shot('after_settle', log)
    sel3 = [u for u, _ in _active_flags(lv)]
    orders_after = {u: lv.get('units', u, 'order') for u in pair}
    n1 = lv.glob('num_cities')
    obs = ('right-click on (27,28) selected %s; after W %s; orders of 107 / 161 (Wait %d, Ready %d) before B %s, '
           'after the settle %s; cities %d -> %d; selected after the settle %s' % (
               sel1, sel2, N('ORDER_WAIT'), N('ORDER_READY'), orders_before, orders_after, n0, n1, sel3))
    if n1 != n0 + 1:
        return dict(shown=None, observed=obs + ' - no city was founded')
    woke = all(orders_before[u] == N('ORDER_WAIT') and orders_after[u] != N('ORDER_WAIT') for u in pair)
    return dict(shown=woke and set(pair) <= set(sel3) and other not in sel3, observed=obs)
