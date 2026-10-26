"""P4 B4 DBG cases, batch 3 (N247): AI internals traced at a callee's ENTRY (reliable whatever the overlays do) during
one turn change, the caller named from the return address."""
import time

import mom
from regress import case

N = mom.N


def _caller(d, h):
    return str(d.where(d.word(h['SS'], h['SP'] + 2, False), d.word(h['SS'], h['SP'], False)))


def _turn(d, log, secs=300):
    """One turn change in pieces (tracepoints never end a wait)."""
    t0, start = time.time(), d.lv.turn()
    poll = d.end_turn_poll()
    while time.time() - t0 < secs and d.lv.turn() <= start:
        d.wait(10, poll)
    return d.lv.turn() > start


DRAKES = (22, 28, 36, 41, 61, 77)               # wizard 1's fantastic units in the base save (Myrror)


@case(148, 'DBG', 'AI mana-upkeep disbanding ranks each summon by AI_Unit_Strength(its TYPE) - a unit index',
      setup='AI wizard 1\'s fantastic units 22 28 36 41 61 77 made Great Drakes (164: 30 mana upkeep each) so its '
            'mana per turn is negative; one turn change; tracepoint on AI_Unit_Strength(unit) entries called from '
            'AI_Disband_For_Upkeep (the caller from the return address): the arguments vs those units\' indexes and '
            'types',
      bug='the arguments are unit TYPES (164, ...) - another unit\'s strength ranks each summon',
      fixed='the summons\' own indexes')
def c148(log):
    import dbg
    s = mom.SaveFile()
    for u in DRAKES:
        s.set('units', u, 'type', 164)
    fant = {u: s.get('units', u, 'type') for u in range(s.get('num_units'))
            if s.get('units', u, 'owner') == 1 and s.get('units', u, 'type') >= N('UNIT_MAGIC_SPIRIT')}
    d = dbg.boot(s, log=log)
    try:
        d.want('AI_Unit_Strength', when=lambda h: 'AI_Disband_For_Upkeep' in _caller(d, h),
               trace=lambda h: d.arg(0, h))
        done = _turn(d, log)
        args = list(d.traces.get('AI_Unit_Strength', []))
    finally:
        d.stop()
    types, idx = set(fant.values()), set(fant)
    obs = ('wizard 1\'s fantastic units (index: type) %s; AI_Unit_Strength arguments from AI_Disband_For_Upkeep %s '
           '(turn changed %s)' % (fant, sorted(set(args))[:30], done))
    if not args:
        return dict(shown=None, observed=obs + ' - no disbanding pass ran')
    as_type = set(args) & types - idx
    as_idx = set(args) & idx - types
    if not as_type and not as_idx:
        return dict(shown=None, observed=obs + ' - the arguments match neither (the gold / food loop only?)')
    return dict(shown=bool(as_type) and not as_idx, observed=obs + '; as types %s, as indexes %s'
                % (sorted(as_type), sorted(as_idx)))


def _pool_call(d, h):
    """(argument, the caller's unit [bp-2], its ut [bp-6], that unit's type now, the return offset in the caller) at
    an AI_Unit_Strength entry."""
    unit = d.word(h['SS'], (h['BP'] - 2) & 0xffff)
    real = d.lv.get('units', unit, 'type') if 0 <= unit < d.lv.glob('num_units') else None
    return d.arg(0, h), unit, d.word(h['SS'], (h['BP'] - 6) & 0xffff), real, d.word(h['SS'], h['SP'], False)


@case(149, 'DBG', 'AI attack pool: AI_Pool_Add_Strongest ranks units by AI_Unit_Strength(their TYPE)',
      setup='the base save; one turn change; tracepoint on AI_Unit_Strength(unit) entries called from '
            'AI_Pool_Add_Strongest: the argument and the caller\'s own locals at the call (the callee has not pushed '
            'BP yet: unit = [bp-2], ut = [bp-6], AIUNITS.ASM) - the unit being ranked and its type. Two call sites: '
            'the ranking loop (cand_str = AI_Unit_Strength(ut) / 10, the bug) and the flyer pool\'s minimum '
            '(AI_Unit_Strength(unit), right); told apart by the return offset - the ranking loop\'s is the lower '
            '(first in the code) - and only the ranking calls are judged',
      bug='the argument is the ranked unit\'s TYPE, not its index: another unit\'s strength ranks it',
      fixed='the unit index')
def c149(log):
    import dbg
    s = mom.SaveFile()
    d = dbg.boot(s, log=log)
    try:
        d.want('AI_Unit_Strength', when=lambda h: 'AI_Pool_Add_Strongest' in _caller(d, h),
               trace=lambda h: _pool_call(d, h))
        done = _turn(d, log)
        calls = list(d.traces.get('AI_Unit_Strength', []))
    finally:
        d.stop()
    obs = ('AI_Unit_Strength calls from AI_Pool_Add_Strongest (argument, ranked unit, its ut local, its type) %s (%d calls; '
           'turn changed %s)' % (calls[:12], len(calls), done))
    if not calls:
        return dict(shown=None, observed=obs + ' - no attack pool built')
    sites = sorted({c[4] for c in calls})
    rank = [c for c in calls if c[4] == sites[0]]
    obs += '; return offsets (calls) %s - the ranking loop\'s %04x' % (
        {'%04x' % o: sum(c[4] == o for c in calls) for o in sites}, sites[0])
    # a unit whose index equals its type cannot tell the two apart: those calls are left out
    told = [c for c in rank if c[1] != c[3]]
    by_type = [c for c in told if c[0] == c[2] == c[3]]
    by_unit = [c for c in told if c[0] == c[1]]
    obs += ('; ranking calls: argument = the unit\'s type in %d of %d telling calls (= the unit index in %d; %d left '
            'out: index == type)' % (len(by_type), len(told), len(by_unit), len(rank) - len(told)))
    if not told:
        return dict(shown=None, observed=obs + ' - no call where index and type differ')
    return dict(shown=len(by_type) == len(told) and not by_unit, observed=obs)


@case(147, 'DBG', 'AI_Unit_Dispatch passes AI_Release_Goto_Near_Weak_City a plane it has not set yet',
      setup='the base save; one turn change; tracepoint on AI_Release_Goto_Near_Weak_City(plane, player) entries: '
            'the arguments (AI_Unit_Dispatch calls it once per AI player, before its plane loop sets the local)',
      bug='plane is stack garbage (not 0 / 1)', fixed='a real plane (or the call inside the plane loop)')
def c147(log):
    import dbg
    s = mom.SaveFile()
    d = dbg.boot(s, log=log)
    try:
        d.want('AI_Release_Goto_Near_Weak_City', trace=lambda h: (d.arg(0, h), d.arg(1, h), _caller(d, h)))
        done = _turn(d, log)
        calls = list(d.traces.get('AI_Release_Goto_Near_Weak_City', []))
    finally:
        d.stop()
    obs = 'AI_Release_Goto_Near_Weak_City(plane, player) calls (with the caller) %s (turn changed %s)' % (calls, done)
    if not calls:
        return dict(shown=None, observed=obs + ' - not called')
    bad = [c for c in calls if c[0] not in (0, 1)]
    if not bad:
        return dict(shown=None, observed=obs + ' - the leftover happened to be a plane number (no evidence either '
                                                'way)')
    return dict(shown=True, observed=obs)


@case(150, 'DBG', 'AI settler site search evaluates sites on the plane in an unwritten local',
      setup='AI wizard 1\'s settler unit 37 (type 53, at (56,16) on Myrror: it reaches the settler loop - 10/24 a '
            'settler retyped inside the 9-unit stack at (48,16) was ejected first); the human given Planar Seal, so '
            'AI_Settler_Tower_Target returns 0 at once and the search runs (10/24: unit 37 went to a tower instead); '
            'one turn change; tracepoint on '
            'City_Site_Evaluate(x, y, plane, ...) entries called from AI_Settler_Orders: the arguments and the '
            'caller\'s settler (unit = DI, its plane up = [bp-56], AIUNITS.ASM; BP is still the caller\'s at the '
            'entry) - only the calls made for unit 37 are judged (other AI settlers search too)',
      bug='the plane argument is not the settler\'s plane 1 (site_plane is never written in AI_Settler_Orders)',
      fixed='plane 1 for every candidate tile on its landmass')
def c150(log):
    import dbg
    s = mom.SaveFile()
    SET = 37
    s.set('wizards', 0, 'globals.planar_seal', 1)
    ty = s.get('units', SET, 'type')
    pos = tuple(s.get('units', SET, f) for f in ('owner', 'x', 'y', 'plane'))
    d = dbg.boot(s, log=log)
    try:
        d.want('City_Site_Evaluate', when=lambda h: 'AI_Settler_Orders' in _caller(d, h),
               trace=lambda h: (d.arg(0, h), d.arg(1, h), d.arg(2, h), h['DI'],
                                d.word(h['SS'], (h['BP'] - 56) & 0xffff),
                                tuple(d.lv.get('units', h['DI'], f) for f in ('owner', 'type', 'x', 'y', 'plane'))))
        # which way AI_Settler_Orders sent unit 37 (settle here / a tower / the site search): 10/24 no search ran
        d.want('AI_Settler_Orders', when=lambda h: d.arg(0, h) == 1, trace=lambda h: d.arg(1, h))
        d.want('AI_Order_Settle', trace=lambda h: d.arg(0, h))
        d.want('AI_Settler_Tower_Target', trace=lambda h: d.arg(1, h))
        done = _turn(d, log)
        every = list(d.traces.get('City_Site_Evaluate', []))
        ways = {k: list(d.traces.get(k, [])) for k in ('AI_Settler_Orders', 'AI_Order_Settle',
                                                       'AI_Settler_Tower_Target')}
        after = tuple(d.lv.get('units', SET, f) for f in ('owner', 'type', 'order', 'x', 'y', 'plane'))
    finally:
        d.stop()
    log.append('AI_Settler_Orders(1, landmass) landmasses %s; AI_Order_Settle units %s; AI_Settler_Tower_Target '
               'units %s; unit 37 after (owner, type, order, x, y, plane) %s'
               % (ways['AI_Settler_Orders'], ways['AI_Order_Settle'], ways['AI_Settler_Tower_Target'], after))
    mine = (pos[0], ty) + pos[1:]
    calls = [c for c in every if c[5] == mine]
    planes = sorted({c[2] for c in calls})
    obs = ('settler unit 37 (owner, x, y, plane) %s; City_Site_Evaluate from AI_Settler_Orders: %d calls in all, %d '
           'for unit 37 (DI\'s record = its owner, type, square), their planes %s, its up local %s, first %s (turn changed '
           '%s)' % (pos, len(every), len(calls), planes, sorted({c[4] for c in calls}), [c[:5] for c in calls[:6]],
                    done))
    if not calls:
        return dict(shown=None, observed=obs + ' - no site search ran for unit 37; ' + log[-1])
    if planes == [pos[3]]:
        return dict(shown=None, observed=obs + ' - the leftover happened to be the right plane (no evidence either '
                                               'way)')
    return dict(shown=True, observed=obs)


def _alloc(research_spell, log, tag):
    s = mom.SaveFile()
    sp = research_spell
    s.set('wizards', 1, 'spell_library[%d][%d]' % ((sp - 1) // 40, (sp - 1) % 40), N('SPELLSTATE_RESEARCHABLE'))
    for f, v in (('research_spell', sp), ('research_left', 5000), ('ai_magic_timer', 1)):
        s.set('wizards', 1, f, v)
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    r = tuple(lv.get('wizards', 1, f) for f in ('research_percentage', 'skill_percentage', 'mana_percentage'))
    log.append('%s: wizard 1 researching %d -> (research, skill, mana) %%%s' % (tag, sp, r))
    return r


@case(154, 'DBG', 'stale spell 90: an AI researching Fire Elemental splits its power 70 / 10 / 20',
      setup='AI wizard 1 researching spell 90 (Fire Elemental, set researchable, 5000 points left), its magic timer '
            'at 1 so AI_Magic_Allocation re-splits in the turn change; the percentages read after one turn. Control: '
            'the same researching spell 19 (its own pick in the save). Without the spell-90 branch research cannot '
            'pass 60 (10 + five +10 picks)',
      bug='research 70, skill 10, mana 20 (the research_spell == 90 branch, AITURN.C; the two casting_spell == 90 '
          'tests of the entry can never be true - Fire Elemental is combat-only)',
      fixed='a normal split (or the branch for the Spell of Mastery)')
def c154(log):
    a = _alloc(N('SPELL_FIRE_ELEMENTAL'), log, 'Fire Elemental')
    b = _alloc(19, log, 'control, spell 19')
    obs = '(research, skill, mana) %% after the re-split: researching 90 %s; researching 19 %s' % (a, b)
    if b[0] == 70:
        return dict(shown=None, observed=obs + ' - the control reached 70 too')
    return dict(shown=a == (70, 10, 20), observed=obs)


def _replan_chance(bias, log, tag, other=0):
    """AI wizard 1 replans its hostility in the turn change (timer 1) toward the contacted human (no treaty, no peace
    turns, relationship 0) with relation_bias[1][0] = bias; tracepoint in AI_Hostility_Replan on
    "if (Random(100) < chance) {" (ai = SI, player = DI, chance = [bp-2], AINEUTRL.ASM). -> chance for (1, 0)."""
    import dbg
    s = mom.SaveFile()
    for a, b in ((1, 0), (0, 1)):
        for f, v in (('contacted[%d]', 1), ('diplomacy_status[%d]', 0), ('peace_turns[%d]', 0),
                     ('relationship[%d]', 0)):
            s.set('wizards', a, f % b, v)
    s.set('wizards', 1, 'relation_bias[0]', bias)
    s.set('wizards', 0, 'relation_bias[1]', other)
    s.set('wizards', 1, 'hostility_replan_timer', 1)
    pers = s.get('wizards', 1, 'personality')
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/AINEUTRL.C', 'if (Random(100) < chance) {', fn='AI_Hostility_Replan', label='roll',
                    when=lambda h: h['SI'] == 1 and h['DI'] == 0,
                    trace=lambda h: d.word(h['SS'], (h['BP'] - 2) & 0xffff))
        d.wait_via_entry('AI_Hostility_Replan', 240, d.end_turn_poll())
        got = list(d.traces.get('roll', []))
    finally:
        d.stop()
    log.append('%s: relation_bias[1][0] %d, [0][1] %d, personality %d: chance(s) %s' % (tag, bias, other, pers,
                                                                                       got))
    return got, pers


@case(253, 'DBG', 'the AI\'s hidden liking makes it MORE hostile: relation_bias is added to the hostility chance',
      setup='AI wizard 1 replans its hostility toward the human in the turn change (replan timer 1; contacted, no '
            'treaty, no peace turns, relationship 0); relation_bias[1][0] +50 (likes the human), then 0; tracepoint in '
            'AI_Hostility_Replan on "if (Random(100) < chance) {" for (ai 1, player 0): the chance',
      bug='the chance is 50 higher with the liking', fixed='lower (the bias subtracted, W372FIX)')
def c253(log):
    a, pers = _replan_chance(50, log, 'likes')
    b, _ = _replan_chance(0, log, 'control')
    obs = 'hostility chance of wizard 1 (personality %d) toward the human: with relation_bias +50 %s; with 0 %s' % (
        pers, a, b)
    if not a or not b:
        return dict(shown=None, observed=obs + ' - the roll line was not reached')
    return dict(shown=a[0] - b[0] == 50, observed=obs)


@case(305, 'DBG', 'AI hostility reads the hidden relation transposed: the OTHER wizard\'s opinion of the AI',
      setup='as #253 (wizard 1 replans its hostility toward the human; tracepoint on the chance): relation_bias[0][1] '
            '+50 - the slot the audience code reads as wizard 1\'s hidden opinion of the human (AI_Diplo_Turn_Vs_Human '
            'reads wizards[0].relation_bias[partner]; Diplo_Break_Treaty raises [breaker][victim]) - with [1][0] 0; '
            'control: both 0',
      bug='the chance does not move (AI_Hostility_Replan reads [1][0])', fixed='it moves with [0][1]')
def c305(log):
    a, pers = _replan_chance(0, log, 'opinion in [0][1]', other=50)
    b, _ = _replan_chance(0, log, 'control')
    obs = ('hostility chance of wizard 1 toward the human: relation_bias[0][1] +50 %s; both 0 %s' % (a, b))
    if not a or not b:
        return dict(shown=None, observed=obs + ' - the roll line was not reached')
    return dict(shown=a[0] == b[0], observed=obs)


@case(3, 'DBG', 'AI wizards roll their mercenary / hero offers with the HUMAN\'s fame',
      setup='the human\'s fame 500, every AI wizard\'s fame 0; one turn change; tracepoint in Turn_Random_Offers on the '
            'mercenary roll "if (Random(100) <= chance) {" (the 2nd such line; player = SI, chance = DI, '
            'TURNUPD.ASM), armed from the function\'s entry: the chance per player',
      bug='each AI\'s chance is 20 (min(500 / 20 + 1, 10) + 10: the human\'s fame), not 11 (its own fame 0: 0 / 20 + 1 '
          '+ 10; x2 before the +10 with Famous)', fixed='each wizard\'s own fame')
def c3(log):
    import dbg
    s = mom.SaveFile()
    s.set('wizards', 0, 'fame', 500)
    for p in range(1, 5):
        s.set('wizards', p, 'fame', 0)
    famous = {p: s.get('wizards', p, 'retorts[%d]' % N('RETORT_FAMOUS')) for p in range(1, 5)}
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/TURNUPD.C', 'if (Random(100) <= chance) {', nth=1, fn='Turn_Random_Offers',
                    label='merc', trace=lambda h: (h['SI'], h['DI']))
        d.wait_via_entry('Turn_Random_Offers', 300, d.end_turn_poll())
        rolls = list(d.traces.get('merc', []))
    finally:
        d.stop()
    ai = [(p, c) for p, c in rolls if 1 <= p <= 4]
    own = {p: (2 if famous[p] else 1) + 10 for p in range(1, 5)}
    obs = ('mercenary rolls (player, chance) %s; the chance from each AI\'s own fame 0 would be %s (Famous %s)'
           % (rolls, own, famous))
    if not ai:
        return dict(shown=None, observed=obs + ' - no AI mercenary roll traced')
    return dict(shown=all(c == 20 for _, c in ai), observed=obs)
