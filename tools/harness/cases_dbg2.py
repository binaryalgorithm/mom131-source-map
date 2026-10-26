"""P4 B4 debugger cases, batch 2 (N246): AI internals read at breakpoints during a turn change (the base save: the
AI wizards on Arcanus run their landmass planning every turn)."""
import mom
from regress import case

N = mom.N
AIU = 'src/wizards/AIUNITS.C'


@case(118, 'DBG', 'AI target list: an enemy fortress\'s defence is read BEFORE its x / y are loaded',
      setup='the base save, one turn change; tracepoint in AI_Collect_Landmass_Targets on "x = p_fortresses[i].x;" '
            '(right after the defence read): i (DI), the x / y locals the defence was read at ([bp-2] / [bp-4]), the '
            'defence [bp-34], player [bp+6] and plane [bp+10]; fortress i\'s own x / y compared',
      bug='the defence is read at the PREVIOUS fortress\'s square (or the stack garbage for the first)',
      fixed='at fortress i\'s square', also=(119,))
def c118(log):
    import dbg
    s = mom.SaveFile()
    fort = {i: tuple(s.get('fortresses', i, f) for f in ('x', 'y', 'plane')) for i in range(5)}
    owner = {c: s.get('cities', c, 'owner') for c in range(s.get('num_cities'))}
    d = dbg.boot(s, log=log)
    try:
        w = lambda h, k: d.word(h['SS'], h['BP'] + k)
        d.want_line(AIU, 'x = p_fortresses[i].x;', fn='AI_Collect_Landmass_Targets', label='fort',
                    trace=lambda h: (w(h, 6), w(h, 10), h['DI'], w(h, -2), w(h, -4), w(h, -34)))
        d.want_line(AIU, 'defence = (p_threat_maps[plane][cy * MAP_WIDTH + cx] & THREAT_STRENGTH_MASK) * 5 / 4;',
                    fn='AI_Collect_Landmass_Targets', label='city_x54', trace=lambda h: h['DI'])
        d.want_line(AIU, 'defence = p_threat_maps[plane][cy * MAP_WIDTH + cx] & THREAT_STRENGTH_MASK;',
                    fn='AI_Collect_Landmass_Targets', label='city_plain', trace=lambda h: h['DI'])
        d.wait(300, d.end_turn_poll())
        forts = list(d.traces.get('fort', []))
        x54 = set(d.traces.get('city_x54', []))
        plain = set(d.traces.get('city_plain', []))
    finally:
        d.stop()
    # #118: the defence of fortress i was read at (x, y) left from before
    stale = [(p, pl, i, (x, y), fort[i][:2]) for p, pl, i, x, y, _ in forts if i in fort and (x, y) != fort[i][:2]]
    good = [(p, pl, i) for p, pl, i, x, y, _ in forts if i in fort and (x, y) == fort[i][:2]]
    obs = ('fortress-loop reads (player, plane, i, the x / y the defence was read at, fortress i\'s x / y): %d, '
           'stale %d, e.g. %s; at the right square %d' % (len(forts), len(stale), stale[:4], len(good)))
    # #119: which city indexes take the x 5 / 4 (human) branch and which the plain one
    humans = sorted(c for c, o in owner.items() if o == 0)
    obs119 = ('city loop: x 5 / 4 branch for cities %s, plain branch for %s; the human\'s cities %s'
              % (sorted(x54), sorted(plain)[:30], humans))
    r = dict(shown=bool(stale) if forts else None, observed=obs + ('' if forts else ' - the loop was not reached'))
    hp = [c for c in humans if c in plain]
    r['also'] = {119: (bool(hp) if (x54 or plain) else None,
                       obs119 + ('' if (x54 or plain) else ' - the city loop was not reached'))}
    return r


@case(424, 'DBG', 'AI_More_Myrror_Cities_Than_Human never initialises the human\'s counters',
      setup='wizard 1 knows Planar Seal and casts nothing (casting_spell 0, mana 5000): its spell pick in the turn '
            'change asks AI_More_Myrror_Cities_Than_Human; breakpoint on "if (ai_myrror > human_myrror)": '
            'human_arcanus [bp-2] and human_myrror [bp-4] read; the human\'s real city counts (Arcanus 2, Myrror 0) '
            'from the save',
      bug='the counters hold stack leftovers plus the count (not 2 / 0)', fixed='2 and 0')
def c424(log):
    import dbg
    s = mom.SaveFile()
    sp = N('SPELL_PLANAR_SEAL')
    s.set('wizards', 1, 'spell_library[%d][%d]' % ((sp - 1) // 40, (sp - 1) % 40), N('SPELLSTATE_KNOWN'))
    for f, v in (('casting_spell', 0), ('casting_cost_left', 0), ('mana_reserve', 5000)):
        s.set('wizards', 1, f, v)
    real = [0, 0]
    for c in range(s.get('num_cities')):
        if s.get('cities', c, 'owner') == 0:
            real[s.get('cities', c, 'plane')] += 1
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/AISPELL.C', 'if (ai_myrror > human_myrror)', fn='AI_More_Myrror_Cities_Than_Human',
                    label='cmp')
        h = d.wait_via_entry('AI_More_Myrror_Cities_Than_Human', 240, d.end_turn_poll())   # the line alone was
        #                                                       never hit (10/24): arm from the entry
        entered = any(x.get('name') == '#entry AI_More_Myrror_Cities_Than_Human' for x in d.hits)
        got = (d.word(h['SS'], h['BP'] - 2), d.word(h['SS'], h['BP'] - 4)) if h else None
        player = d.word(h['SS'], h['BP'] + 6) if h else None
    finally:
        d.stop()
    obs = ('AI_More_Myrror_Cities_Than_Human(player %s): human_arcanus / human_myrror %s; the human\'s real cities '
           '(Arcanus, Myrror) %s' % (player, got, tuple(real)))
    if got is None:
        return dict(shown=None, observed=obs + ' - the comparison was not reached (function entered: %s)'
                    % entered)
    if got == tuple(real):
        return dict(shown=None, observed=obs + ' - the leftovers happened to be 0 this time (no evidence either way)')
    return dict(shown=True, observed=obs)


DEAD = 16                                       # AI wizard 2's unit outside a city at (7,15) on Arcanus


@case(137, 'DBG', 'AI strength tables count a dead unit record (not yet compacted) as an enemy on Arcanus',
      setup='the save set to turn 125: AI_Disband_Obsolete_Units, run in Next_Turn just before AI_Overland_Turn, '
            'recomputes the AI wizards\' average_unit_cost on a turn divisible by 25 (Game_Start_Init zeroes it at '
            'every load, 10/24) and kills every AI unit outside a city cheaper than half of it; wizard 2\'s unit 16 '
            '(at (7,15), Arcanus) made High Men Spearmen (104, cost 10) so it is disbanded there (owner and plane -1 '
            'until Units_Compact after the AI turn; a record killed '
            'in the human\'s turn never reaches the AI: Human_End_Turn compacts first). One turn change; '
            'tracepoints in AI_Build_Strength_Tables on "if (ai_enemy_stack_count >= AI_MAX_ENEMY_STACKS)" (the '
            'not-own branch; i = DI; armed through the function\'s entry - it calls other overlays first) and in '
            'AI_Build_Threat_Map on "p_threat_maps[plane][cell] += AI_Unit_Strength(i) / 10;" (i = SI, plane = '
            'DI): the AI players (and the record\'s owner / plane then) that took unit 16',
      bug='both take unit 16 (owner -1; on plane 0: AI_Overland_Turn reset its plane -1 to 0) as an enemy / threat',
      fixed='dead records skipped', also=(418,))
def c137(log):
    import dbg
    s = mom.SaveFile()
    pos = tuple(s.get('units', DEAD, f) for f in ('owner', 'x', 'y', 'plane'))
    s.set('game_turn', value=125)
    s.set('units', DEAD, 'type', 104)
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/AITURN.C', 'if (ai_enemy_stack_count >= AI_MAX_ENEMY_STACKS)',
                    fn='AI_Build_Strength_Tables', label='enemy', when=lambda h: h['DI'] == DEAD,
                    trace=lambda h: (d.word(h['SS'], h['BP'] + 6), d.lv.get('units', DEAD, 'owner'),
                                     d.lv.get('units', DEAD, 'plane')))
        d.want_line('src/wizards/AIORDERS.C', 'p_threat_maps[plane][cell] += AI_Unit_Strength(i) / 10;',
                    fn='AI_Build_Threat_Map', label='threat', when=lambda h: h['SI'] == DEAD,
                    trace=lambda h: (d.word(h['SS'], h['BP'] + 6), h['DI'], d.lv.get('units', DEAD, 'owner')))
        # AI_Unit_Strength / Unit_Can_Fly are in other overlays: the line can move under its breakpoint - arm
        # through the function's entry (README sec 4); the threat line is traced on the way
        d.wait_via_entry('AI_Build_Strength_Tables', 300, d.end_turn_poll())
        entered = any(x.get('name') == '#entry AI_Build_Strength_Tables' for x in d.hits)
        enemy = list(d.traces.get('enemy', []))
        threat = list(d.traces.get('threat', []))
    finally:
        d.stop()
    obs = ('unit 16 (owner, x, y, plane in the save %s): AI_Build_Strength_Tables took it as an enemy for (player, '
           'owner, plane) %s; AI_Build_Threat_Map added it for (player, plane, owner) %s' % (pos, enemy[:8], threat[:8]))
    if not entered:
        return dict(shown=None, observed=obs + ' - AI_Build_Strength_Tables was not entered')
    dead_e = [e for e in enemy if e[1] in (-1, 255)]
    dead_t = [e for e in threat if e[2] in (-1, 255)]
    if not dead_e and not dead_t:
        return dict(shown=None, observed=obs + ' - no trace of unit 16 as a dead record (its disbanding unconfirmed)')
    r = dict(shown=bool(dead_e), observed=obs)
    # #418 is AI_Build_Threat_Map's missing dead-unit test: shown by the dead record's threat; its second half, the
    # write through p_threat_maps[-1] (a unit killed later in the AI pass keeps plane -1), is NOT exercised here
    r['also'] = {418: (bool(dead_t), obs + ' - #418: the dead record added threat (its own function); the plane -1 '
                                           'write is not exercised (the record\'s plane is 0 here)')}
    return r


@case(57, 'DBG', 'AI-vs-AI peace test adds diplo_msg_variant, the diplomacy screen\'s last text variant',
      setup='AI wizards 1..4 met and at war with each other (diplomacy_status TREATY_WAR both ways), peace interest '
            '90; the human first opens the diplomacy screen with wizard 1 and leaves (diplo_msg_variant set from the '
            'text pick); turns ended (up to 3); tracepoint in Diplo_AI_Vs_AI on the peace line "if '
            '(wizards[i].diplomacy_status[j] == TREATY_WAR && diplo_msg_variant + ...": i, j (locals) and the '
            'diplo_msg_variant global read at each pass',
      bug='the war / peace decision adds a UI text variant (stale, often -1 or 0..14) to the peace interest',
      fixed='a real term (or none)')
def c57(log):
    import dbg
    import cases_dip as C
    s = mom.SaveFile()
    C._met(s)
    for a in range(1, 5):
        for b in range(1, 5):
            if a != b:
                for f, v in (('contacted[%d]', 1), ('met_stage[%d]', 2), ('diplomacy_status[%d]', N('TREATY_WAR')),
                             ('peace_interest[%d]', 90), ('peace_turns[%d]', 0)):
                    s.set('wizards', a, f % b, v)
    d = dbg.boot(s, log=log)
    try:
        opened = mom.diplo_open(d.lv, 1, log=log)
        for _ in range(4):
            if mom.on_map(d.lv):
                break
            d.lv.D.key('esc')
            d.wait(2, None)
        v0 = d.lv.glob('diplo_msg_variant')
        d.want_line('src/wizards/DIPLOMSG.C', 'if (wizards[i].diplomacy_status[j] == TREATY_WAR && diplo_msg_variant',
                    label='peace', trace=lambda h: d.lv.glob('diplo_msg_variant'))
        d.wait(500, d.turns_poll(d.lv.turn() + 3))
        reads = list(d.traces.get('peace', []))
    finally:
        d.stop()
    obs = ('diplomacy screen opened %s; diplo_msg_variant after it %d; the AI-vs-AI peace line read diplo_msg_variant '
           '%d time(s): values %s' % (bool(opened), v0, len(reads), sorted(set(reads))))
    if not reads:
        return dict(shown=None, observed=obs + ' - no AI pair at war reached the peace test')
    return dict(shown=True, observed=obs)
