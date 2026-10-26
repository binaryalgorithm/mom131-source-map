"""P4 B4 cases read at a breakpoint on the buggy source line (dbg.want_line + srcmap.py), N229: the values the 1.31
code uses there, from the registers and the stack frame of the running game."""
import struct
import time

import dbg
import mom
from regress import case

N = mom.N


@case(4, 'OV', 'the Housing x2 test reads the city at the option count, not the city being decided',
      setup='grand_vizier = 1, human city Cornwell (3) producing auto-choose (-4) with population 3; AI cities Tier, '
            'Doraat, Myrean, Bremen producing auto-choose too; breakpoint on "if (p_cities[i].population < 5)" in '
            'Vizier_Choose_Production and AI_Wizard_Choose_Production (AIPROD.C) during the turn change',
      bug='i (DI) = the option count, not the city: the population of city i is tested',
      fixed='the decided city\'s own population')
def c4(log):
    s = mom.SaveFile()
    s.set('grand_vizier', value=1)
    s.set('cities', 3, 'producing', N('PROD_AUTO_CHOOSE'))
    s.set('cities', 3, 'population', 3)
    for c in (1, 2, 4, 20):
        s.set('cities', c, 'producing', N('PROD_AUTO_CHOOSE'))
    d = dbg.boot(s, log=log)
    try:
        lab = [d.want_line('src/wizards/AIPROD.C', 'if (p_cities[i].population < 5)', k, label=n)
               for k, n in ((0, 'AI_Wizard_Choose_Production'), (1, 'Vizier_Choose_Production'))]
        log.append('breakpoints: %s' % {k: '%04x:%04x' % v for k, v in d.wanted.items()})
        h = d.wait(150, d.end_turn_poll())
        if h is None:
            return dict(shown=None, observed='neither chooser reached the line')
        i = h['DI']
        city = d.local(8, h) if h['name'] == lab[0] else d.local(6, h)
        pi = d.lv.get('cities', i, 'population') if i < d.lv.glob('num_cities') else None
        pc = d.lv.get('cities', city, 'population')
        d.go()
        mom.shot('after_line', log)
    finally:
        d.stop()
    obs = '%s(city %d, population %d): the test reads p_cities[%d] (DI = the option count), population %s' % (
        h['name'], city, pc, i, pi)
    return dict(shown=i != city, observed=obs)


def _only_summons_upkeep(s):
    """Save edit: the human's unit, city and global enchantments removed, so a mana shortfall can only be met by
    Mana_Shortfall_Summons (Mana_Shortfall picks the four categories in a random order and stops once enough is
    freed: with enchantments left the summons category was often never reached, #6 10/23)."""
    a, es, c, _ = s.addr('wizards', 0, 'globals')
    s.b[a:a + es * c] = bytes(es * c)
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'enchantments', 0)
    for c in range(s.get('num_cities')):
        for k, v in enumerate(s.get('cities', c, 'enchants')):
            if v == 1:                          # cast by player 0 (value = player + 1)
                s.set('cities', c, 'enchants[%d]' % k, 0)



def _human_summon(s, u, typ):
    """Save edit: unit u (the human's) retyped as a summoned creature (AFLAG_FANTASTIC: mana upkeep). The base save
    has NO human summons - its Dragon Turtles are Lizardman units (type 126), so a mana shortfall had nothing to
    disband (#6 / #7 never reached their lines, 10/23)."""
    s.set('units', u, 'type', typ)
    s.set('units', u, 'hero_slot', -1)
    s.set('units', u, 'moves_max', 0)       # recomputed from the type at load
    s.set('units', u, 'level', 0)
    s.set('units', u, 'experience', 0)


@case(6, 'OV', 'mana shortfall: the summon disbanding loop starts one past the last unit',
      setup='human unit 192 made War Bears (179, a summon: the base save has none); human mana 0 with all power to '
            'research (mana_percentage 0), so the summon\'s upkeep is short; the human\'s unit / city / global '
            'enchantments removed (only the summons can make up the shortfall); '
            'breakpoint on the unit test of Mana_Shortfall_Summons (TURNUPD.C) during the turn change',
      bug='the first unit examined is index num_units (SI), one past the last record', fixed='num_units - 1')
def c6(log):
    s = mom.SaveFile()
    for f, v in (('mana_reserve', 0), ('mana_percentage', 0), ('skill_percentage', 0), ('research_percentage', 100)):
        s.set('wizards', 0, f, v)
    _only_summons_upkeep(s)
    _human_summon(s, 192, N('UNIT_WAR_BEARS'))
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/TURNUPD.C', 'if (p_units[unit].owner == player && (unit_types[p_units[unit].type]'
                    '.ability_flags & AFLAG_FANTASTIC) != 0)', label='shortfall', fn='Mana_Shortfall_Summons')
        for f in ('Upkeep_Shortfall_Mana', 'Mana_Shortfall_Summons'):   # tracepoints: how far the path got
            d.want(f, trace=lambda h: (d.arg(0, h), d.arg(1, h), d.lv.get('wizards', 0, 'mana_reserve')))
        h = d.wait(150, d.end_turn_poll())
        if h is None:
            return dict(shown=None, observed='Mana_Shortfall_Summons did not reach its unit test; (player, mana needed, '
                                             'reserve) at the entries: %s; turn %d' % (d.traces, d.lv.turn()))
        n = d.lv.glob('num_units')
        rec = {f: d.lv.get('units', h['SI'], f) for f in ('owner', 'type', 'x', 'y')}
        d.go()
        mom.shot('after_line', log)
    finally:
        d.stop()
    obs = 'first unit tested: SI = %d with num_units = %d (the record there: %s)' % (h['SI'], n, rec)
    return dict(shown=h['SI'] == n, observed=obs)


@case(7, 'OV', 'Units_Stranded_By_Removal never fills its list: the stranded units come from stack garbage',
      setup='human unit 192 made a Djinn (195: a summon with innate Wind Walking - the base save has no human '
            'summons) and moved with heroes 33 and 69 onto the shore water square (29,26); human mana short as in #6, '
            'so Mana_Shortfall_Summons removes 192; breakpoint on "if (other_walker == 0 && self_walker == 1)" (UNITFIG.C) reads count [bp-2] and the '
            'tile list [bp-30]', bug='count / list are whatever the stack held (never filled from the square)',
      fixed='count 3, the list = 192, 33, 69')
def c7(log):
    s = mom.SaveFile()
    for f, v in (('mana_reserve', 0), ('mana_percentage', 0), ('skill_percentage', 0), ('research_percentage', 100)):
        s.set('wizards', 0, f, v)
    for u in (192, 33, 69):
        for f, v in (('x', 29), ('y', 26), ('plane', 0)):
            s.set('units', u, f, v)
    _only_summons_upkeep(s)
    _human_summon(s, 192, N('UNIT_DJINN'))      # innate Wind Walking (AFLAG_WIND_WALKING): no enchantment to dispel
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/UNITFIG.C', 'if (other_walker == 0 && self_walker == 1)', label='stranded',
                    fn='Units_Stranded_By_Removal')
        h = d.wait(150, d.end_turn_poll())
        if h is None:
            return dict(shown=None, observed='Units_Stranded_By_Removal did not reach the walker test')
        unit = h['DI']
        count = d.local(-2, h)
        lst = [d.local(-30 + 2 * k, h) for k in range(9)]
        d.go()
        mom.shot('after_line', log)
    finally:
        d.stop()
    obs = 'Units_Stranded_By_Removal(%d): count [bp-2] = %d, tile list [bp-30] = %s (the square holds 192, 33, 69)' % (
        unit, count, lst)
    return dict(shown=unit == 192 and sorted(lst[:max(0, min(count, 9))]) != [33, 69, 192], observed=obs)


@case(9, 'OV', 'AI-vs-AI treaty roll: an alliance straight from war (no peace, no pact first)',
      setup='living AI wizards 1, 2, 4: all met, at war (status 3) with each other, relationship 100, treaty '
            'interest 100; breakpoint on "Diplo_Set_Status(i, j, TREATY_ALLIANCE)" (DIPLOMSG.C) over up to 8 turns '
            '(the pair roll is 1 in 15 - 2 x difficulty)', bug='the status before the alliance is war (3)',
      fixed='war -> peace first; a pact before an alliance')
def c9(log):
    s = mom.SaveFile()
    ais = (1, 2, 4)
    for i in ais:
        for j in ais:
            if i != j:
                for f, v in (('contacted[%d]', 1), ('diplomacy_status[%d]', N('TREATY_WAR')), ('relationship[%d]', 100),
                             ('treaty_interest[%d]', 100), ('met_stage[%d]', 2)):
                    s.set('wizards', i, f % j, v)
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/DIPLOMSG.C', 'Diplo_Set_Status(i, j, TREATY_ALLIANCE);', label='alliance')
        h = d.wait(600, d.turns_poll(s.get('game_turn') + 8))
        if h is None:
            return dict(shown=None, observed='no AI-AI alliance roll in 8 turns')
        i, j = h['DI'], h['SI']
        st = d.lv.get('wizards', i, 'diplomacy_status[%d]' % j)
        turn = d.lv.turn()
        d.go()
        mom.shot('after_line', log)
    finally:
        d.stop()
    obs = 'turn %d: wizards %d and %d are made allies while their status is %d (3 = war)' % (turn, i, j, st)
    return dict(shown=st == N('TREATY_WAR'), observed=obs)


def _ai_casts(s, ai, spell, mana=5000):
    """Save edit: AI wizard `ai` has finished casting `spell` (cost left 0): it is cast in its next turn."""
    for f, v in (('casting_spell', spell), ('casting_cost_left', 0), ('casting_cost_initial', 500),
                 ('mana_reserve', mana)):
        s.set('wizards', ai, f, v)


def _ai_hostile(s, ai):
    """Save edit: AI `ai` hostile (HOSTILITY_HOSTILE) to every other wizard, no peace turns: AI_Pick_Wizard_Target
    draws from AI_List_Hostile_Wizards and gives up when it is empty (the base save: all 0, so #10 / #11's spells
    were cancelled before they acted, 10/23)."""
    for v in range(5):
        if v != ai:
            s.set('wizards', ai, 'hostility[%d]' % v, N('HOSTILITY_HOSTILE'))
            s.set('wizards', ai, 'peace_turns[%d]' % v, 0)


def _all_casting(s, spell=N('SPELL_DISENCHANT_TRUE')):
    """Every wizard casting with progress 50 (AI_Pick_Wizard_Target wants a target that is casting, #102)."""
    for w in range(5):
        if s.get('wizards', w, 'casting_spell') == 0 or w == 0:
            s.set('wizards', w, 'casting_spell', spell)
        s.set('wizards', w, 'casting_cost_initial', 100)
        s.set('wizards', w, 'casting_cost_left', 50)


@case(10, 'OV', 'Subversion also lowers the AI caster\'s own relation with the victim',
      setup='AI wizard 1 has finished Subversion (177; mana 5000) and is hostile to every wizard, every wizard casting '
            '(so the target picker accepts '
            'one, #102); memory-write watchpoints on wizard 1\'s relationship toward each other wizard, stopping only '
            'when the writer is Subversion_Apply: the caster\'s own relation with the victim, old -> new', bug='the caster\'s own relation drops by 25 too', fixed='only the third wizards\' drop')
def c10(log):
    s = mom.SaveFile()
    _all_casting(s)
    _ai_casts(s, 1, N('SPELL_SUBVERSION'))
    _ai_hostile(s, 1)
    for w in range(5):
        for v in range(5):
            if w != v:
                s.set('wizards', w, 'contacted[%d]' % v, 1)
    d = dbg.boot(s, log=log)
    h = None
    try:
        for v in (0, 2, 3, 4):                  # who is the victim is the AI's choice: watch all four; the turn's
            d.watch('rel%d' % v, ('wizards', 1, 'relationship[%d]' % v), size=1,      # drift writes them too
                    when=lambda h: 'Subversion_Apply' in h['writer'])
        h = d.wait(300, d.turns_poll(s.get('game_turn') + 2))
        cast = d.lv.get('wizards', 1, 'casting_spell')
        if h is not None:
            victim = int(h['name'][3:])
            r0, r1 = struct.unpack('<b', h['old'])[0], struct.unpack('<b', h['new'])[0]
        for v in (0, 2, 3, 4):
            d.unwatch('rel%d' % v)
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if h is None:
        return dict(shown=None, observed='no write by Subversion_Apply to wizard 1\'s relations in 2 turns (other '
                                          'writes skipped: %s; wizard 1 now casting %d)' % (d.skipped, cast))
    obs = 'Subversion_Apply wrote wizard 1 (the caster)\'s relationship with victim %d: %d -> %d' % (victim, r0, r1)
    return dict(shown=r1 == max(-100, r0 - 25) and r1 != r0, observed=obs)


@case(11, 'OV', 'Drain Power drains 10..200 mana (the wiki: 50..150)',
      setup='AI wizard 1 has finished Drain Power (171) and is hostile to every wizard, every wizard casting; '
            'breakpoints on Drain_Power_Apply and '
            'then on Random: the calls it makes and the amount returned',
      bug='ten Random(20) (1..20 each) summed: 10..200', fixed='50..150')
def c11(log):
    s = mom.SaveFile()
    _all_casting(s)
    _ai_casts(s, 1, N('SPELL_DRAIN_POWER'))
    _ai_hostile(s, 1)
    for w in range(5):
        s.set('wizards', w, 'mana_reserve', max(s.get('wizards', w, 'mana_reserve'), 1000))
    d = dbg.boot(s, log=log)
    try:
        d.want('Drain_Power_Apply')
        h = d.wait(240, d.turns_poll(s.get('game_turn') + 2))
        if h is None:
            return dict(shown=None, observed='Drain_Power_Apply was not reached')
        victim = d.arg(0, h)
        d.drop('Drain_Power_Apply')
        rolls = []
        for _ in range(10):
            args, r = d.call('Random', nargs=1, timeout=20)
            rolls.append((args[0], r))
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    obs = 'victim %d: Random calls (n, result) %s, sum %d' % (victim, rolls, sum(r for _, r in rolls))
    return dict(shown=all(n == 20 for n, _ in rolls), observed=obs)


@case(102, 'OV', 'AI Drain Power / Subversion / Cruel Unminding need a target that is casting (the Spell Blast test)',
      setup='AI wizard 1 has finished Drain Power (171) and is hostile to every wizard (else its empty hostile list '
            'returns 0 before the casting test); NO wizard is casting; breakpoint on AI_Pick_Wizard_Target: '
            'its spell argument and result', bug='0 (no target): the spell is cancelled',
      fixed='a wizard target (Drain Power needs no casting victim)')
def c102(log):
    s = mom.SaveFile()
    for w in range(5):
        for f in ('casting_spell', 'casting_cost_left', 'casting_cost_initial'):
            s.set('wizards', w, f, 0)
    _ai_casts(s, 1, N('SPELL_DRAIN_POWER'))
    _ai_hostile(s, 1)                         # else the empty hostile list returns 0 first (10/23)
    d = dbg.boot(s, log=log)
    try:
        args, r = d.call('AI_Pick_Wizard_Target', lambda a: a[1] == N('SPELL_DRAIN_POWER'), 3,
                         timeout=240, poll=d.turns_poll(s.get('game_turn') + 2))
        d.go()
        cs = d.lv.get('wizards', 1, 'casting_spell')
        mom.shot('after', log)
    finally:
        d.stop()
    if args is None:
        return dict(shown=None, observed='AI_Pick_Wizard_Target was not called for Drain Power')
    obs = 'AI_Pick_Wizard_Target(spell %d, caster %d) = %d; wizard 1 casting now %d' % (args[1], args[2], r, cs)
    return dict(shown=r == 0, observed=obs)


@case(25, 'OV', 'a city with an Oracle and Nature\'s Eye sees 4 squares (the Oracle is tested last)',
      setup='Cornwell (3) given an Oracle and Nature\'s Eye; breakpoint on the city\'s Visibility_Mark_Square line in '
            'Visibility_Rebuild (MAPVIS.C): range [bp-6] for i (SI) = 3, during the turn change',
      bug='range 4', fixed='5 (the larger of the two)')
def c25(log):
    s = mom.SaveFile()
    s.set('cities', 3, 'buildings[%d]' % N('BLD_ORACLE'), N('BLDSTATE_BUILT'))
    s.set('cities', 3, 'enchants[%d]' % N('CITY_ENCH_NATURES_EYE'), 1)
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/MAPVIS.C', 'Visibility_Mark_Square(p_cities[i].x, p_cities[i].y, plane, range);',
                    label='city_vis')
        poll = d.end_turn_poll()
        rng = None
        for _ in range(60):
            h = d.wait(120, poll)
            if h is None:
                break
            if h['SI'] == 3:
                rng = d.local(-6, h)
                break
            d.go()
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if rng is None:
        return dict(shown=None, observed='Cornwell\'s visibility line was not reached')
    return dict(shown=rng == 4, observed='Cornwell (Oracle + Nature\'s Eye): Visibility_Mark_Square range %d' % rng)


@case(35, 'OV', 'Gaia\'s Blessing at the map edge reaches column 60 (the next row\'s x 0)',
      setup='Cornwell (3) moved to (58,29) with Gaia\'s Blessing; breakpoint on Tile_Is_Desert during the turn change: '
            'the squares City_Gaias_Blessing_Turn tests', bug='a call with x = 60 (one past the row)',
      fixed='x 58..59, then 0..1 wrapped')
def c35(log):
    s = mom.SaveFile()
    s.set('cities', 3, 'x', 58)
    s.set('cities', 3, 'enchants[%d]' % N('CITY_ENCH_GAIAS_BLESSING'), 1)
    d = dbg.boot(s, log=log)
    xs = []
    try:
        d.want('City_Gaias_Blessing_Turn')
        h = d.wait(200, d.end_turn_poll())
        while h is not None and d.arg(0, h) != 3:
            d.go()
            h = d.wait(60, d.end_turn_poll())
        if h is None:
            return dict(shown=None, observed='City_Gaias_Blessing_Turn(3) was not reached')
        d.drop('City_Gaias_Blessing_Turn')
        for _ in range(25):
            args, r = d.call('Tile_Is_Desert', nargs=3, timeout=20)
            if args is None:
                break
            xs.append(tuple(args[:2]))
            d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    cols = sorted(set(x for x, _ in xs))
    obs = 'Tile_Is_Desert columns for Cornwell at x 58: %s (%d squares)' % (cols, len(xs))
    return dict(shown=60 in cols, observed=obs)


@case(41, 'OV', 'Evil Omens raises the caster\'s own Life / Nature costs; the cost reduction has no cap',
      setup='the human (Life books 7 -> 20: 130 % reduction) holds Evil Omens itself; the overland spellbook is opened '
            '(S); breakpoint on Spell_Casting_Cost(0, a Life spell): its result against the base cost',
      bug='the caster\'s cost x 3 / 2 too, and 130 % off: a cost of 0 or below',
      fixed='no Omens penalty for the caster; the reduction capped')
def c41(log):
    s = mom.SaveFile()
    s.set('wizards', 0, 'spellbooks[%d]' % N('REALM_LIFE'), 20)
    s.set('wizards', 0, 'globals.evil_omens', 1)
    mom.learn_spells(s, 0, [N('SPELL_GUARDIAN_SPIRIT'), N('SPELL_HEROISM')])   # the book prices only the spells
    d = dbg.boot(s, log=log)                                                # on its open pages: a Life summon
    try:
        pressed = [0]

        def poll():
            if not pressed[0]:
                d.action()
                d.lv.D.key('s')
                pressed[0] = 1
        args, cost = d.call('Spell_Casting_Cost',
                            lambda a: a[0] == 0 and d.lv.get('spells', a[1], 'realm') == N('REALM_LIFE'), 3,
                            timeout=60, poll=poll)
        if args is None:
            d.go()
            mom.shot('no_call', log)
            return dict(shown=None, observed='no Spell_Casting_Cost call for a Life spell')
        base = d.lv.get('spells', args[1], 'base_cost')
        castable = d.lv.get('spells', args[1], 'castable')
        d.go()
        time.sleep(2)
        mom.shot('spellbook', log)
    finally:
        d.stop()
    c = base * 3 // 2
    if castable in (0, 2, 3) and args[2] == 0:
        c *= 5
    c = c - c * 130 // 100
    obs = 'Spell_Casting_Cost(0, spell %d, combat %d) = %d; base %d (castable %d); with Omens x 3/2 and -130 %%: %d' % (
        args[1], args[2], cost, base, castable, c)
    return dict(shown=cost == c and cost <= 0, observed=obs)


@case(127, 'OV', 'conquering a city keeps the curses the conqueror had cast on it (also #128: no Oracle radius)',
      setup='AI city 21 Rastenford (wizard 1, not its fortress) moved to (30,28) on Arcanus next to Dracon, emptied of '
            'defenders (its units stay on Myrror), given an Oracle and a Famine cast by the human (enchants value '
            '1 = player 0 + 1); Dracon\'s stack walks in; breakpoints on City_Change_Owner and Map_Explore_Radius',
      bug='Famine still on the city after the capture; explored radius 2', fixed='Famine ends; the Oracle\'s radius',
      also=(128,))
def c127(log):
    import cases_cb
    s = mom.SaveFile()
    c = 21
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):
        s.set('cities', c, f, v)
    s.set('cities', c, 'enchants[%d]' % N('CITY_ENCH_FAMINE'), 1)
    s.set('cities', c, 'buildings[%d]' % N('BLD_ORACLE'), N('BLDSTATE_BUILT'))
    key = mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [], (30, 28, 0))
    d = dbg.boot(s, log=log)
    try:
        d.want('City_Change_Owner')
        d.want('Map_Explore_Radius')
        k = [0]

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                d.lv.D.key(key)
            elif not mom.on_map(d.lv) and k[0] % 3 == 0:
                d.lv.D.key(('n', 'enter')[(k[0] // 3) % 2])
        got = {}
        while len(got) < 2:
            h = d.wait(120, poll)
            if h is None:
                break
            if h['name'] == 'City_Change_Owner':
                got['owner'] = (d.arg(0, h), d.arg(1, h))
            elif got.get('owner') and 'radius' not in got:
                got['radius'] = [d.arg(i, h) for i in range(4)]
            d.go()
        d.drop('City_Change_Owner')             # else a later Map_Explore_Radius call stops the game mid-capture and
        d.drop('Map_Explore_Radius')            # the owner / Famine reads see the half-done state (owner still 1)
        d.go(d.csip())
        for _ in range(6):
            time.sleep(1.5)
            if not mom.on_map(d.lv):
                d.lv.D.key(('esc', 'enter')[_ % 2])
        fam = d.lv.get('cities', c, 'enchants[%d]' % N('CITY_ENCH_FAMINE'))
        own = d.lv.get('cities', c, 'owner')
        mom.shot('captured', log)
    finally:
        d.stop()
    if 'owner' not in got:
        return dict(shown=None, observed='City_Change_Owner was not reached')
    rad = got.get('radius')
    obs = 'City_Change_Owner(city %d, new owner %d); city owner now %d, Famine value %d; Map_Explore_Radius %s' % (
        got['owner'] + (own, fam, rad))
    return dict(shown=fam == 1 and own == 0, observed=obs,
                also={128: (rad is not None and rad[3] == 2, 'Oracle city captured: Map_Explore_Radius(x, y, plane, '
                            'radius) = %s' % rad)})


@case(46, 'OV', 'AI audiences: the offer / proposal logic runs only when the AI\'s score is BELOW -100',
      setup='AI wizard 1 met and friendly toward the human (relationship 100 both ways, treaty interest 100); '
            'breakpoint on "if (score + ...relationship[diplo_partner] >= -100)" in AI_Diplo_Turn_Vs_Human '
            '(DIPLOTRN.C): score [bp-4] + relationship, for ai (SI) = 1, during the turn change',
      bug='the friendly AI\'s value is >= -100: its pending message is cleared, no proposal',
      fixed='(design) friendly AIs reach the proposal roll')
def c46(log):
    s = mom.SaveFile()
    for a, b in ((0, 1), (1, 0)):
        for f, v in (('contacted[%d]', 1), ('relationship[%d]', 100), ('treaty_interest[%d]', 100),
                     ('met_stage[%d]', 2)):
            s.set('wizards', a, f % b, v)
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/DIPLOTRN.C', 'if (score + wizards[PLAYER_HUMAN].relationship[diplo_partner] >= -100)',
                    label='score')
        d.want('AI_Diplo_Turn_Vs_Human', trace=lambda h: d.lv.turn())   # was the function entered at all?
        h = None
        for _ in range(8):
            h = d.wait(150, d.end_turn_poll())
            if h is None or h['SI'] == 1:
                break
            d.go()
        if h is None or h['SI'] != 1:
            return dict(shown=None, observed='the audience test for AI 1 was not reached (AI_Diplo_Turn_Vs_Human '
                                             'entered on turns %s; now turn %d)' % (
                                                 d.traces.get('AI_Diplo_Turn_Vs_Human'), d.lv.turn()))
        score = d.local(-4, h)
        rel = d.lv.get('wizards', 0, 'relationship[1]')
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    obs = 'AI 1 (relationship 100): score %d + relationship %d = %d (>= -100: message cleared)' % (score, rel,
                                                                                                    score + rel)
    return dict(shown=score + rel >= -100, observed=obs)


@case(66, 'OV', 'a relation event copies the AI\'s interests over the human\'s spent ones',
      setup='human and AI wizard 1 met, with a Wizard Pact; the human\'s trade / treaty interest toward wizard 1 spent '
            '(0), wizard 1\'s toward the human 77; a human stack attacks a wizard-1 unit next to Dracon and confirms '
            '(Y) the attack despite the treaty (ATTACK.C: Diplo_Relation_Change(-40, 0, 1)); a memory-write watchpoint '
            'on wizards[0].trade_interest[1], stopping only for writes by Diplo_Relation_Change (Diplo_Break_Treaty '
            'writes it too)',
      bug='0 -> 77 + the scaled amount (wizard 1\'s value copied onto the human\'s row)', fixed='stays spent')
def c66(log):
    import cases_cb
    s = mom.SaveFile()
    for a, b, v in ((0, 1, 0), (1, 0, 77)):   # 77: odd, so the copied interest (77 + a scaled even amount) is not 0
        for f in ('trade_interest[%d]', 'treaty_interest[%d]'):
            s.set('wizards', a, f % b, v)
        s.set('wizards', a, 'contacted[%d]' % b, 1)
        s.set('wizards', a, 'met_stage[%d]' % b, 2)
        s.set('wizards', a, 'diplomacy_status[%d]' % b, N('TREATY_WIZARD_PACT'))   # a plain attack calls no relation
    key = mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [2], (30, 28, 0))   # change before the battle;
    s.set('units', 2, 'owner', 1)                                                  # breaking a treaty does
    d = dbg.boot(s, log=log)
    try:
        t0 = (d.lv.get('wizards', 0, 'trade_interest[1]'), d.lv.get('wizards', 0, 'treaty_interest[1]'))
        o = d.lv.get('wizards', 1, 'trade_interest[0]')
        d.watch('trade', ('wizards', 0, 'trade_interest[1]'),   # who writes the human's spent interest (10/23:
                when=lambda h: 'Diplo_Relation_Change' in h['writer'])   # Diplo_Break_Treaty writes it too
        k = [0]                                                  # the call breakpoint was missed twice)

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                d.lv.D.key(key)
            elif k[0] % 3 == 0:
                d.lv.D.key('y')                 # 'You have a treaty ... still wish to attack?'
        h = d.wait(90, poll)
        writer = h['writer'] if h else None
        d.unwatch('trade')
        d.go()
        t1 = (d.lv.get('wizards', 0, 'trade_interest[1]'), d.lv.get('wizards', 0, 'treaty_interest[1]'))
        mom.shot('after', log)
        if mom.in_combat(d.lv) or not mom.on_map(d.lv):
            mom.finish_combat(d.lv, log=log)
    finally:
        d.stop()
    if h is None:
        return dict(shown=None, observed='wizards[0].trade_interest[1] was not written (still %d)' % t0[0])
    new = int.from_bytes(h['new'], 'little', signed=True)
    obs = ('human\'s trade interest toward wizard 1: %d -> %d written by %s (wizard 1\'s toward the human %d before); '
           'trade / treaty after the attack %s (Diplo_Break_Treaty sets -200 later)' % (t0[0], new, writer, o, t1))
    return dict(shown=t0[0] == 0 and new != 0, observed=obs)


def _empty_lair(s, lair=0, xy=(30, 28), special=0, items=(), gold=0, mana=0, flags=0):
    """Save edit: lair `lair` moved next to Dracon with no guards, the given treasure; returns the keypad key that
    walks Dracon's stack in (all other human units on patrol)."""
    import cases_cb
    for f, v in (('x', xy[0]), ('y', xy[1]), ('plane', 0), ('active', 1), ('guard1_count', 0), ('guard2_count', 0),
                 ('gold', gold), ('mana', mana), ('special', special), ('flags', flags), ('item_count', len(items))):
        s.set('lairs', lair, f, v)
    for k, v in enumerate(items):
        s.set('lairs', lair, 'item_values[%d]' % k, v)
    return mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [], xy + (0,))


def _walk_in_poll(d, key):
    k = [0]

    def poll():
        k[0] += 1
        if k[0] == 1:
            d.action()
            d.lv.D.key(key)
        elif k[0] % 3 == 0 and not mom.on_map(d.lv):
            if not mom.items_award(d.lv):       # the lair notices close on Esc only (their one control: the
                d.lv.D.key(('esc', 'n', 'y')[(k[0] // 3) % 3])   # whole screen, hotkey Esc; screen stays MAP);
                                                    # N: 'destroy this city?' Raze / No
    return poll


@case(72, 'OV', 'treasure spells: Arcane is admitted only for wizards with Alchemy or Warlord',
      setup='the human without Warlord (retorts[1] = 0) or Alchemy; an empty lair next to Dracon with special 1 (a '
            'common spell); Dracon\'s stack walks in; breakpoint after the realm loop of Treasure_Pick_Spell '
            '(TREASURE.C): realm_done[5] (Arcane) at [bp-14]', bug='1: Arcane out (spellbooks[5] = the retorts word)',
      fixed='0: Arcane in for everyone')
def c72(log):
    s = mom.SaveFile()
    s.set('wizards', 0, 'retorts[0]', 0)
    s.set('wizards', 0, 'retorts[1]', 0)
    key = _empty_lair(s, special=1)
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/TREASURE.C', 'while (n_realms > 0 && found == 0) {', label='realms')
        h = d.wait(120, _walk_in_poll(d, key))
        if h is None:
            mom.shot('no_call', log)
            return dict(shown=None, observed='Treasure_Pick_Spell was not reached')
        done = [d.local(-24 + 2 * k, h) for k in range(6)]
        n = d.local(-6, h)
        d.go()
        time.sleep(2)
        mom.shot('treasure', log)
    finally:
        d.stop()
    obs = 'human (no Alchemy / Warlord): realm_done Nature..Arcane %s, realms open %d' % (done, n)
    return dict(shown=done[5] == 1, observed=obs)


@case(74, 'OV', 'lair items: the stored item value is passed as the prefab KIND, with no cost cap',
      setup='an empty lair next to Dracon holding one item of value 150 (no special); Dracon\'s stack walks in; '
            'breakpoint on Item_Create_Prefab: its arguments, then the cost of the item it returns',
      bug='Item_Create_Prefab(150, books, 0): kind = the value, max cost 0 (no window) - any prefab the books allow',
      fixed='an item of at most the value')
def c74(log):
    s = mom.SaveFile()
    key = _empty_lair(s, items=(150,))
    d = dbg.boot(s, log=log)
    try:
        args, item = d.call('Item_Create_Prefab', nargs=3, timeout=120, poll=_walk_in_poll(d, key))
        if args is None:
            d.go()
            mom.shot('no_call', log)
            return dict(shown=None, observed='Item_Create_Prefab was not called')
        cost = d.lv.get('items', item, 'cost') if 0 <= item < N('NUM_ITEMS') else None
        d.go()
        time.sleep(3)
        mom.shot('treasure', log)
    finally:
        d.stop()
    obs = 'Item_Create_Prefab(kind %d, books, max_cost %d) = item %d, cost %s (lair value 150)' % (args[0], args[2],
                                                                                                 item, cost)
    return dict(shown=args[0] == 150 and args[2] == 0, observed=obs)


@case(115, 'OV', 'AI Word of Recall never finds a target (the spell is not in the unit-enchant picker\'s list)',
      setup='AI wizard 1 has finished Word of Recall (47; mana 5000); breakpoint on AI_Pick_Unit_Enchant_Target with '
            'spell 47: its result', bug='0: no unit, the cast does nothing', fixed='a unit of the caster')
def c115(log):
    s = mom.SaveFile()
    _ai_casts(s, 1, N('SPELL_WORD_OF_RECALL'))
    d = dbg.boot(s, log=log)
    try:
        args, r = d.call('AI_Pick_Unit_Enchant_Target', lambda a: a[2] == N('SPELL_WORD_OF_RECALL'), 4,
                         timeout=240, poll=d.turns_poll(s.get('game_turn') + 2))
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if args is None:
        return dict(shown=None, observed='AI_Pick_Unit_Enchant_Target was not called for Word of Recall')
    obs = 'AI_Pick_Unit_Enchant_Target(kind %d, &unit, spell %d, caster %d) = %d' % (args[0], args[2], args[3], r)
    return dict(shown=r == 0, observed=obs)


def _outpost_grow(s, mineral, log):
    """Cornwell (3) made a human outpost (population 0, tens 50), minerals of its area cleared except area square 3
    (12,28) = mineral; breakpoints on three lines of Outposts_Grow (CITYCALC.C) read grow (DI) for city (SI) 3:
    before the mineral loop, after it, after the difficulty multiplier."""
    s.set('cities', 3, 'population', 0)
    s.set('cities', 3, 'size', 0)
    s.set('cities', 3, 'population_tens', 50)
    cx, cy = s.get('cities', 3, 'x'), s.get('cities', 3, 'y')
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            s.set('minerals', (cy + dy) * 60 + cx + dx, value=0)
    s.set('minerals', 28 * 60 + 12, value=mineral)
    d = dbg.boot(s, log=log)
    lines = ('plane = p_cities[city].plane;', 'chance = 5;', 'if (Random(100) <= grow)')
    got = {}
    try:
        for t in lines:
            d.want_line('src/wizards/CITYCALC.C', t, label=t, fn='Outposts_Grow')
        poll = d.end_turn_poll()
        for _ in range(200):
            h = d.wait(150, poll)
            if h is None:
                break
            if h['SI'] == 3:
                got[h['name']] = h['DI'] - 0x10000 if h['DI'] & 0x8000 else h['DI']
            d.go()
            if len(got) == 3:
                break
        pct = d.lv.get('wizards', 0, 'gold') * 0 + d.lv.glob('difficulty')
        mom.shot('after', log)
    finally:
        d.stop()
    return [got.get(t) for t in lines], pct


@case(5, 'OV', 'outpost growth reads the mineral of area square [city number] for every square (also #121)',
      setup='Cornwell (3) a human outpost; one iron ore at its area square 3 (12,28) (the square 1.31 reads for city '
            '3), no other mineral; grow (DI) at three lines of Outposts_Grow', also=(121,),
      bug='the mineral loop adds 5 for every one of the 21 squares (+105); the difficulty multiplier applies to the '
          'human\'s outpost', fixed='+5 once; (#121) no multiplier for the human')
def c5(log):
    s = mom.SaveFile()
    (g0, g1, g2), diff = _outpost_grow(s, N('MINERAL_IRON_ORE'), log)
    if None in (g0, g1, g2):
        return dict(shown=None, observed='Outposts_Grow lines not all reached for city 3: %s' % [g0, g1, g2])
    obs = 'Cornwell outpost: grow %d before the mineral loop, %d after it (+%d), %d after the difficulty (%d) ' \
          'multiplier' % (g0, g1, g1 - g0, g2, diff)
    return dict(shown=g1 - g0 == 105, observed=obs,
                also={121: (g2 != g1, 'human outpost grow %d -> %d by the difficulty %d table' % (g1, g2, diff))})


@case(122, 'OV', 'Wild Game adds nothing to outpost growth',
      setup='as #5, the square read is Wild Game (mineral flag 0x40) instead of iron', bug='the mineral loop adds 0',
      fixed='a bonus for the Wild Game')
def c122(log):
    s = mom.SaveFile()
    (g0, g1, g2), diff = _outpost_grow(s, N('MINFLAG_WILD_GAME'), log)
    if None in (g0, g1):
        return dict(shown=None, observed='Outposts_Grow lines not reached for city 3: %s' % [g0, g1, g2])
    return dict(shown=g1 == g0, observed='Cornwell outpost with Wild Game: grow %d before the mineral loop, %d after' % (
        g0, g1))


@case(156, 'OV', 'the expansion penalty counts the HUMAN\'s cities for every wizard',
      setup='the human given 12 more cities (neutral cities 5-16 made owner 0); up to two turn changes (the block '
            'runs on an even game_turn; Next_Turn increments it after the drift); breakpoint on the '
            '"(land_size + 1) * 3 < count" line of Diplo_Relation_Drift (DIPLOACT.C): count [bp-6] for each wizard i '
            '(SI)', bug='count = the human\'s city count for the AI wizards too',
      fixed='each wizard\'s own city count')
def c156(log):
    s = mom.SaveFile()
    for c in range(5, 17):
        s.set('cities', c, 'owner', 0)
    own = {w: sum(1 for c in range(s.get('num_cities')) if s.get('cities', c, 'owner') == w) for w in range(6)}
    d = dbg.boot(s, log=log)
    seen = {}
    try:
        d.want_line('src/wizards/DIPLOACT.C', 'if ((land_size + 1) * 3 < count && Random(4) == 1)', label='count')
        d.want('Diplo_Relation_Drift', trace=lambda h: d.lv.turn())    # entered? (the line was missed twice)
        poll = d.turns_poll(s.get('game_turn') + 2)   # the block runs when game_turn is even (106 at the first
        for _ in range(8):                             # drift: Next_Turn increments it after Diplo_Relation_Drift)
            h = d.wait(300, poll)
            if h is None:
                break
            seen[h['SI']] = d.local(-6, h)
            d.go()
            if len(seen) >= 5:
                break
        mom.shot('after', log)
    finally:
        d.stop()
    if not seen:
        return dict(shown=None, observed='Diplo_Relation_Drift\'s count line was not reached (entered on turns %s)'
                                         % d.traces.get('Diplo_Relation_Drift'))
    obs = 'count per wizard (i: count / own cities): %s' % ', '.join('%d: %d / %d' % (i, c, own[i])
                                                                    for i, c in sorted(seen.items()))
    ais = [i for i in seen if i > 0]
    return dict(shown=bool(ais) and all(seen[i] == own[0] for i in ais), observed=obs)


@case(157, 'OV', 'army envy works backwards from the guide: the weaker a wizard, the less envy',
      setup='the human\'s stat_army_strength set to 40 (the others 27 / 10 / 0 / 13: the first turn change uses the '
            'saved values); breakpoint on "if (Random(100) >= threshold && Random(20) == 1)" in Diplo_Relation_Drift '
            '(DIPLOACT.C): '
            'the human\'s army [bp-10], threshold [bp-12] = army x 50 / theirs, for each wizard i (SI) weaker than '
            'the human', bug='(QUIRK) threshold >= 100 at 2:1 and beyond: the envy roll can never pass',
      fixed='- (the guide: 2:1 always triggers on the d20 roll)')
def c157(log):
    s = mom.SaveFile()
    s.set('wizards', 0, 'stat_army_strength', 40)   # the base save: human 5, the others 27 / 10 / 0 / 13 - nobody
    d = dbg.boot(s, log=log)                        # weaker; Diplo_Relation_Drift runs before Next_Turn recomputes it
    seen = {}
    try:
        d.want_line('src/wizards/DIPLOACT.C', 'if (Random(100) >= threshold && Random(20) == 1)', label='envy')
        poll = d.end_turn_poll()
        for _ in range(6):
            h = d.wait(150, poll)
            if h is None:
                break
            i = h['SI']
            seen[i] = (d.local(-10, h), d.lv.get('wizards', i, 'stat_army_strength'), d.local(-12, h))
            d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if not seen:
        return dict(shown=None, observed='the envy line was not reached (no wizard weaker than the human)')
    obs = 'wizard i: human army / theirs -> threshold: %s' % ', '.join(
        '%d: %d / %d -> %d' % ((i,) + v) for i, v in sorted(seen.items()))
    return dict(shown=any(a >= 2 * t and th >= 100 for a, t, th in seen.values()), observed=obs)


@case(159, 'OV', 'relation overflow: a large negative change wraps the byte and IMPROVES the relation',
      setup='wizard 1 made Lawful, its relationship with the human 20, at peace (status 0); its city Rastenford (21) '
            'moved next to Dracon and emptied as in #127; Dracon\'s stack walks in; breakpoint on Diplo_Relation_Change'
            '(amount, actor 0, observer 1): wizards[1].relationship[0] at entry and return',
      bug='e.g. -40 x 2 (relation > 0) x 2 (Lawful) = -160 added as a byte: the relation goes UP',
      fixed='clamped at -100')
def c159(log):
    import cases_cb
    s = mom.SaveFile()
    c = 21
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):
        s.set('cities', c, f, v)
    s.set('wizards', 1, 'personality', N('PERSONALITY_LAWFUL'))
    for a, b in ((0, 1), (1, 0)):
        for f, v in (('relationship[%d]', 20), ('diplomacy_status[%d]', 0), ('contacted[%d]', 1),
                     ('met_stage[%d]', 2)):
            s.set('wizards', a, f % b, v)
    key = mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [], (30, 28, 0))
    d = dbg.boot(s, log=log)
    calls = []
    try:
        d.want('Diplo_Relation_Change')
        poll = _walk_in_poll(d, key)
        for _ in range(12):
            h = d.wait(60, poll)
            if h is None:
                break
            a = [d.arg(i, h) for i in range(4)]
            if a[1] == 0 and a[2] == 1:
                r0 = d.lv.get('wizards', 1, 'relationship[0]')
                d.ret_ax()
                calls.append((a[0], a[3], r0, d.lv.get('wizards', 1, 'relationship[0]')))
            d.go()
        d.drop('Diplo_Relation_Change')
        mom.shot('after', log)
    finally:
        d.stop()
    if not calls:
        return dict(shown=None, observed='no Diplo_Relation_Change(actor 0, observer 1)')
    obs = 'Diplo_Relation_Change(amount, msg): relation before -> after: %s' % '; '.join(
        '(%d, msg %d): %d -> %d' % c for c in calls)
    return dict(shown=any(am < 0 and r1 > r0 for am, m, r0, r1 in calls), observed=obs)


@case(162, 'OV', 'AI-vs-AI hostility checks run only on the rare friendly-treaty roll',
      setup='living AI wizards 1, 2, 4 all met (6 ordered pairs); breakpoint on AI_War_Check over three turn changes, '
            'counting the calls for AI-AI pairs (the human\'s pairs are checked every turn)',
      bug='AI-AI calls only when the pair passes Random(15 - 2 x difficulty) == 1: about 2 of 18',
      fixed='every pair every turn: 18')
def c162(log):
    s = mom.SaveFile()
    for i in (1, 2, 4):
        for j in (1, 2, 4):
            if i != j:
                s.set('wizards', i, 'contacted[%d]' % j, 1)
                s.set('wizards', i, 'met_stage[%d]' % j, 2)
    t0 = s.get('game_turn')
    d = dbg.boot(s, log=log)
    pairs = []
    try:
        d.want('AI_War_Check')
        poll = d.turns_poll(t0 + 3)
        import time as _t
        start = _t.time()
        while _t.time() - start < 400 and d.lv.turn() < t0 + 3:
            h = d.wait(60, poll)
            if h is None:
                continue
            a = (d.arg(0, h), d.arg(1, h))
            if a[0] > 0 and a[1] > 0:
                pairs.append(a)
            d.go()
        turns = d.lv.turn() - t0
        mom.shot('after', log)
    finally:
        d.stop()
    obs = '%d turn changes: %d AI_War_Check calls for AI-AI pairs %s (6 ordered pairs a turn)' % (turns, len(pairs),
                                                                                                pairs)
    return dict(shown=turns >= 3 and len(pairs) < 6 * turns, observed=obs)


@case(163, 'OV', 'Break_Conflicting_Allies fixes a conflict only when the wizard allied to both has the lowest index',
      setup='AI wizard 4 allied (status 2) to wizards 1 and 2, which are at war (3) with each other; all met; '
            'relationships 100 (4 with 1 and 2) / -100 (1 with 2), 1 and 2 with peace interest -100 and hostility WAR, '
            'so the treaties hold; breakpoint on '
            'Diplo_Break_Conflicting_Allies: the first call (up to 6 turns) with the conflict still in place at entry, '
            'the treaties at its return',
      bug='4 stays allied to both (its loops start the other two above it)', fixed='one alliance broken')
def c163(log):
    s = mom.SaveFile()

    def st(a, b, v):
        for x, y in ((a, b), (b, a)):
            s.set('wizards', x, 'diplomacy_status[%d]' % y, v)
            s.set('wizards', x, 'contacted[%d]' % y, 1)
            s.set('wizards', x, 'met_stage[%d]' % y, 2)
    st(4, 1, N('TREATY_ALLIANCE'))
    st(4, 2, N('TREATY_ALLIANCE'))
    st(1, 2, N('TREATY_WAR'))
    for a, b, r in ((4, 1, 100), (4, 2, 100), (1, 2, -100)):   # keep the treaties from drifting before the call
        for x, y in ((a, b), (b, a)):                           # (10/23: by the first call 4-1 was a pact, 4-2 war;
            s.set('wizards', x, 'relationship[%d]' % y, r)      # next run 1-2 had made peace)
    for x, y in ((1, 2), (2, 1)):
        s.set('wizards', x, 'peace_interest[%d]' % y, -100)
        s.set('wizards', x, 'hostility[%d]' % y, N('HOSTILITY_WAR'))
    d = dbg.boot(s, log=log)
    seen = []
    before = after = None
    try:
        def tr():
            g = lambda a, b: d.lv.get('wizards', a, 'diplomacy_status[%d]' % b)
            return g(4, 1), g(4, 2), g(1, 2)
        d.want('Diplo_Break_Conflicting_Allies')
        poll = d.turns_poll(s.get('game_turn') + 6)
        t0 = time.time()
        while time.time() - t0 < 500:
            h = d.wait(500 - (time.time() - t0), poll)
            if h is None:
                break
            b = tr()
            seen.append(b)
            if b == (2, 2, 3):                  # the conflict is there at entry: what does the call do with it?
                before = b
                d.ret_ax()
                after = tr()
                break
            d.go()
        d.drop('Diplo_Break_Conflicting_Allies')
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if before is None:
        return dict(shown=None, observed='no Diplo_Break_Conflicting_Allies call with the conflict still in place in 6 '
                                         'turns (treaties 4-1 / 4-2 / 1-2 at the calls: %s)' % seen)
    obs = 'Diplo_Break_Conflicting_Allies: 4-1 / 4-2 / 1-2 %s at entry -> %s after (2 alliance, 3 war)' % (before, after)
    return dict(shown=after == (2, 2, 3), observed=obs)


@case(136, 'OV', 'Spell Blast cost differs by caster: the AI pays the target spell\'s base cost - cost left',
      setup='the human is casting Armageddon (120) with 2000 invested (casting_cost_initial 2000, left 100); no other '
            'wizard casting; AI wizard 1 (hostile to all) has finished Spell Blast (58) with mana 5000; a memory-write '
            'watchpoint on wizard 1\'s mana_reserve, stopping only for writes by Overland_Cast_Spell (the Spell Blast '
            'payment): old - new', bug='wizard 1 pays Armageddon\'s base cost - 100 (the human rule: initial - left = '
            '1900)', fixed='the same rule for both casters')
def c136(log):
    s = mom.SaveFile()
    for w in range(5):
        for f in ('casting_spell', 'casting_cost_left', 'casting_cost_initial'):
            s.set('wizards', w, f, 0)
    for f, v in (('casting_spell', N('SPELL_ARMAGEDDON')), ('casting_cost_initial', 2000), ('casting_cost_left', 100)):
        s.set('wizards', 0, f, v)
    _ai_casts(s, 1, N('SPELL_SPELL_BLAST'))
    _ai_hostile(s, 1)                         # AI_Pick_Wizard_Target needs a hostile wizard (10/23)
    d = dbg.boot(s, log=log)
    try:
        base = d.lv.get('spells', N('SPELL_ARMAGEDDON'), 'base_cost')
        d.watch('mana', ('wizards', 1, 'mana_reserve'), when=lambda h: 'Overland_Cast_Spell' in h['writer'])
        h = d.wait(300, d.turns_poll(s.get('game_turn') + 2))     # 10/23: the old before / after turn read mixed
        if h is not None:                                       # in the AI's income (and a 50-cost spell gave -50)
            old, new = (int.from_bytes(h[k], 'little', signed=True) for k in ('old', 'new'))
        d.unwatch('mana')
        d.go()
        cs = d.lv.get('wizards', 0, 'casting_spell')
        mom.shot('after', log)
    finally:
        d.stop()
    if h is None:
        return dict(shown=None, observed='no write to wizard 1\'s mana by Overland_Cast_Spell in 2 turns (other writes '
                                         'skipped: %s)' % d.skipped)
    paid = old - new
    obs = ('wizard 1\'s Spell Blast: mana %d -> %d (paid %d) by %s; Armageddon base cost %d: base - left = %d, '
           'initial - left = 1900; the human now casting %d' % (old, new, paid, h['writer'], base, base - 100, cs))
    return dict(shown=paid == base - 100, observed=obs)


def _knows(s, w, spell, state=None):
    s.set('wizards', w, 'spell_library[%d][%d]' % ((spell - 1) // 40, (spell - 1) % 40),
          N('SPELLSTATE_KNOWN') if state is None else state)


@case(143, 'OV', 'the AI drops a global it casts when its realms make it self-harming (Tranquility for Chaos)',
      setup='AI wizard 1 given Chaos as its primary realm and a finished Tranquility (157) to cast; a traced '
            'memory-write watchpoint on its globals.tranquility over two turn changes: every write, its writer and the '
            'turn (10/23: reads after each turn saw 0 both times - set and dropped within one turn change)',
      bug='set to 1 by the cast, then 0 by AI_Drop_Bad_Globals (the whole cast wasted; the AI pickers choose these '
          'spells with no realm test)', fixed='the AI does not cast it')
def c143(log):
    s = mom.SaveFile()
    s.set('wizards', 1, 'primary_realm', N('REALM_CHAOS'))
    _ai_casts(s, 1, N('SPELL_TRANQUILITY'))
    t0 = s.get('game_turn')
    d = dbg.boot(s, log=log)
    try:
        d.watch('tq', ('wizards', 1, 'globals.tranquility'), size=1,
                trace=lambda h: (h['old'][0], h['new'][0], h['writer'], d.lv.turn()))
        poll, t1 = d.turns_poll(t0 + 2), time.time()
        while time.time() - t1 < 400 and d.lv.turn() < t0 + 2:   # tracepoints never end a wait: wait in pieces
            d.wait(10, poll)
        d.wait(5, None)
        writes = d.traces.get('tq', [])
        turns = d.lv.turn() - t0
        d.unwatch('tq')
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    obs = 'wizard 1 (primary realm Chaos), %d turn changes: writes to globals.tranquility (old, new, writer, turn): %s' % (
        turns, writes)
    if not writes:
        return dict(shown=None if turns < 2 else False, observed=obs)
    on = [w for w in writes if w[0] == 0 and w[1] != 0]          # the cast stores 2 (seen 10/23), not 1
    off = [w for w in writes if w[0] != 0 and w[1] == 0 and 'AI_Drop_Bad_Globals' in w[2]]
    return dict(shown=bool(on) and bool(off), observed=obs)


def _ai_pick_case(s, func, log, turns=6):
    _ai_pick_case.picks = 'the overland picks not traced'
    d = dbg.boot(s, log=log)
    try:
        # how many overland picks wizard 1 made on the way (AI_Choose_Overland_Spell runs only when it is idle)
        d.want('AI_Choose_Overland_Spell', when=lambda h: d.arg(0, h) == 1, trace=lambda h: d.lv.glob('game_turn'))
        args, r = d.call(func, lambda a: a[0] == 1, 1, timeout=150 * turns,
                         poll=d.turns_poll(s.get('game_turn') + turns))
        _ai_pick_case.picks = 'AI_Choose_Overland_Spell(1) entered on turns %s' % d.traces.get(
            'AI_Choose_Overland_Spell', [])
        log.append(_ai_pick_case.picks)
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    return args, r


def _fast_caster(s, w):
    """Wizard w's casting skill made 1000 (skill_points 1000 x 1001: the smallest n with n (n + 1) >= them) and its
    mana 30000: every overland spell is finished the turn after it is chosen, so the AI picks a new one every turn
    (AITURN.C: AI_Choose_Overland_Spell runs only while casting_spell is none) - many category draws in a few turns."""
    s.set('wizards', w, 'skill_points', 1000 * 1001)
    s.set('wizards', w, 'mana_reserve', 30000)


ATTACK_SPELLS = ('ICE_STORM', 'EARTHQUAKE', 'SPELL_BLAST', 'STASIS', 'CORRUPTION', 'FIRE_STORM', 'RAISE_VOLCANO',
                 'CHAOS_RIFT', 'CALL_THE_VOID', 'FAMINE', 'WARP_NODE', 'BLACK_WIND', 'DRAIN_POWER', 'EVIL_PRESENCE',
                 'SUBVERSION', 'PESTILENCE', 'CURSED_LANDS', 'CRUEL_UNMINDING')   # AI_Pick_Attack_Spell's others


@case(240, 'OV', 'the AI picks an anti-realm global it already has',
      setup='AI wizard 1 not casting (it picks a new spell), Nature\'s Wrath (40) known and active (globals.'
            'natures_wrath 1), the other three anti-realm globals unknown, mana 5000; Suppress Magic known, not '
            'active: it alone gives the anti-realm category its weight (+100, BUG #419; a held Nature\'s Wrath adds '
            'none - 10/24: without it the category was never picked) but has no slot in the pick; breakpoint on '
            'AI_Pick_Anti_Realm_Global(1) over up to 8 turns: its result', bug='40 (weighted on "known" alone)',
      fixed='never a global already held')
def c240(log):
    s = mom.SaveFile()
    for f, v in (('casting_spell', 0), ('casting_cost_left', 0), ('mana_reserve', 5000), ('globals.nature_s_wrath', 1)):
        s.set('wizards', 1, f, v)
    _knows(s, 1, N('SPELL_NATURES_WRATH'))
    for sp in (N('SPELL_LIFE_FORCE'), N('SPELL_TRANQUILITY'), N('SPELL_EVIL_OMENS')):
        _knows(s, 1, sp, 0)
    _knows(s, 1, N('SPELL_SUPPRESS_MAGIC'))
    s.set('wizards', 1, 'globals.suppress_magic', 0)
    _fast_caster(s, 1)
    args, r = _ai_pick_case(s, 'AI_Pick_Anti_Realm_Global', log, turns=15)
    if args is None:
        return dict(shown=None, observed='AI_Pick_Anti_Realm_Global(1) was not called in 15 turns (%s)'
                                         % _ai_pick_case.picks)
    return dict(shown=r == N('SPELL_NATURES_WRATH'), observed='AI_Pick_Anti_Realm_Global(1) = %d while wizard 1 '
                                                            'holds Nature\'s Wrath (40)' % r)


@case(241, 'OV', 'the AI can pick Time Stop while its Time Stop is running',
      setup='AI wizard 1 not casting, Time Stop (80) known and active (globals.time_stop 1), mana 5000; at war with '
            'the human (diplomacy_status TREATY_WAR, hostility HOSTILITY_WAR, relationship -100, contacted, replan '
            'timer 50 so AI_Hostility_Replan keeps it): the attack category needs a hostile wizard (10/24: without '
            'one it was never picked); Time Stop the ONLY attack spell it knows (the other 18 of AI_Pick_Attack_Spell '
            'made researchable), so the pick has one non-zero weight; casting skill 1000 and mana 30000 (a new pick '
            'every turn); breakpoint on AI_Pick_Attack_Spell(1) over up to 15 turns: its result',
      bug='80 comes out (its only weight: no own-global test)', fixed='none while it runs (no weight left)')
def c241(log):
    s = mom.SaveFile()
    for f, v in (('casting_spell', 0), ('casting_cost_left', 0), ('mana_reserve', 5000), ('globals.time_stop', 1)):
        s.set('wizards', 1, f, v)
    _knows(s, 1, N('SPELL_TIME_STOP'))
    cleared = []
    for name in ATTACK_SPELLS:
        sp = N('SPELL_' + name)
        if s.get('wizards', 1, 'spell_library[%d][%d]' % ((sp - 1) // 40, (sp - 1) % 40)) == N('SPELLSTATE_KNOWN'):
            _knows(s, 1, sp, N('SPELLSTATE_RESEARCHABLE'))
            cleared.append(sp)
    for f, v in (('diplomacy_status[0]', N('TREATY_WAR')), ('hostility[0]', N('HOSTILITY_WAR')),
                 ('relationship[0]', -100), ('contacted[0]', 1), ('peace_turns[0]', 0), ('hostility_replan_timer', 50)):
        s.set('wizards', 1, f, v)
    _fast_caster(s, 1)
    args, r = _ai_pick_case(s, 'AI_Pick_Attack_Spell', log, turns=15)
    obs_set = 'other attack spells it knew, made researchable: %s; %s' % (cleared, _ai_pick_case.picks)
    if args is None:
        return dict(shown=None, observed='AI_Pick_Attack_Spell(1) was not called in 15 turns; ' + obs_set)
    return dict(shown=r == N('SPELL_TIME_STOP'), observed='AI_Pick_Attack_Spell(1) = %d while wizard 1 holds Time '
                                                        'Stop (80), its only attack spell; %s' % (r, obs_set))


@case(172, 'OV', 'attacking a treaty partner names the VICTIM as the treaty breaker',
      setup='the human and AI wizard 1 have a Wizard Pact (status 1 both ways), met; a wizard-1 unit placed next to '
            'Dracon; Dracon\'s stack attacks it and confirms the treaty question (Y); breakpoint on '
            'Diplo_Break_Treaty(breaker, victim)', bug='Diplo_Break_Treaty(1, 0): the defender is the breaker',
      fixed='(0, 1)')
def c172(log):
    import cases_cb
    s = mom.SaveFile()
    for a, b in ((0, 1), (1, 0)):
        for f, v in (('diplomacy_status[%d]', N('TREATY_WIZARD_PACT')), ('contacted[%d]', 1), ('met_stage[%d]', 2)):
            s.set('wizards', a, f % b, v)
    key = mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [2], (30, 28, 0))
    s.set('units', 2, 'owner', 1)
    d = dbg.boot(s, log=log)
    try:
        k = [0]

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                d.lv.D.key(key)
            elif k[0] % 2 == 0:
                d.lv.D.key('y')
        args, r = d.call('Diplo_Break_Treaty', nargs=2, timeout=90, poll=poll)
        d.drop('Diplo_Break_Treaty')
        d.go()
        mom.shot('after', log)
        if mom.in_combat(d.lv) or not mom.on_map(d.lv):
            mom.finish_combat(d.lv, log=log)
    finally:
        d.stop()
    if args is None:
        return dict(shown=None, observed='Diplo_Break_Treaty was not called')
    return dict(shown=args == [1, 0], observed='Diplo_Break_Treaty(breaker %d, victim %d) when the human attacked '
                                               'wizard 1' % tuple(args))


@case(419, 'DBG', 'Suppress Magic is weighted into the anti-realm category, whose picker has no slot for it',
      setup='AI wizard 1 not casting, mana 5000, Suppress Magic (79) known and not active, the four anti-realm globals '
            '(Nature\'s Wrath, Life Force, Tranquility, Evil Omens) unknown: only Suppress Magic weights category 8; '
            'casting skill 1000 and mana 30000 (a pick every turn); breakpoint on AI_Pick_Anti_Realm_Global(1) over '
            'up to 15 turns: its result',
      bug='0 (no spell): the category picked for Suppress Magic casts nothing', fixed='Suppress Magic (or another '
      'category)')
def c419(log):
    s = mom.SaveFile()
    for f, v in (('casting_spell', 0), ('casting_cost_left', 0), ('mana_reserve', 5000),
                 ('globals.suppress_magic', 0)):
        s.set('wizards', 1, f, v)
    for sp in (N('SPELL_NATURES_WRATH'), N('SPELL_LIFE_FORCE'), N('SPELL_TRANQUILITY'), N('SPELL_EVIL_OMENS')):
        _knows(s, 1, sp, 0)
    _knows(s, 1, N('SPELL_SUPPRESS_MAGIC'))
    _fast_caster(s, 1)                          # a new overland pick every turn (10/24: 8 turns gave no pick)
    args, r = _ai_pick_case(s, 'AI_Pick_Anti_Realm_Global', log, turns=15)
    if args is None:
        return dict(shown=None, observed='AI_Pick_Anti_Realm_Global(1) was not called in 15 turns (%s)'
                                         % _ai_pick_case.picks)
    return dict(shown=r == 0, observed='AI_Pick_Anti_Realm_Global(1) = %d with only Suppress Magic (79) weighting '
                                       'the category' % r)
