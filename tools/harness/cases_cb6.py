"""P4 B4 combat cases, batch 6 (N247): the AI's missile tally, Immolation on a shot, Animate Dead on an enemy corpse."""
import time

import mom
from regress import case
from cases_cb5 import _alone, _approach_click, HERO

N = mom.N


def _vs_neutrals(s, att, att_type, u2_type, u13_type, ench=0):
    """Dracon's first unit (or `att`) made att_type (level 0, enchantments `ench`) attacks neutral units 2 and 13 made
    u2_type / u13_type at (30,28); every other human unit finished. -> the keypad key."""
    key = mom.setup_attack(s, att, [2, 13], (30, 28, 0))
    _alone(s, att)
    if att_type is not None:
        for f, v in (('type', att_type), ('hero_slot', -1), ('level', 0), ('experience', 0), ('enchantments', ench)):
            s.set('units', att, f, v)
    for u, ty in ((2, u2_type), (13, u13_type)):
        for f, v in (('owner', 5), ('type', ty), ('hero_slot', -1), ('level', 0), ('experience', 0),
                     ('enchantments', 0)):
            s.set('units', u, f, v)
    return key


@case(18, 'CB', 'Side_Missile_Strength returns the LAST missile unit\'s value, not the sum',
      setup='Dracon\'s first unit made Barbarian Spearmen attacks neutral units 2 and 13, both made Barbarian Bowmen '
            '(41: shooters) at (30,28); the human ends its first combat turn (D); stop at Side_Missile_Strength(5) - '
            'from AI_Combat_Turn / the neutral side\'s tactics - run to its return; the neutral shooters\' ranged '
            'strengths read then',
      bug='the return is one shooter\'s strength (the last), not the sum of both', fixed='the sum')
def c18(log):
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 39, 41, 41)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        k = [0]

        def poll():
            k[0] += 1
            if mom.in_combat(d.lv) and k[0] % 3 == 1:
                d.lv.D.key('d')                 # Done: the neutral side's AI turn
        args, ax = d.call('Side_Missile_Strength', lambda a: a[0] == 5, 1, timeout=90, poll=poll)
        bus = mom.battle_units(d.lv, ('unit_index', 'owner', 'status', 'ranged'))
        d.go()
    finally:
        d.stop()
    shooters = [b['ranged'] for b in bus if b['owner'] == 5 and b['status'] == 0 and b['unit_index'] in (2, 13)]
    obs = ('Side_Missile_Strength(5) = %s; the living neutral shooters\' ranged strengths %s (sum %d)'
           % (ax, shooters, sum(shooters)))
    if args is None:
        return dict(shown=None, observed=obs + ' - not called')
    if len(shooters) < 2 or min(shooters) <= 0:
        return dict(shown=None, observed=obs + ' - not two living shooters')
    return dict(shown=ax == shooters[-1] and ax != sum(shooters), observed=obs)


@case(188, 'CB', 'Immolation fires on every attack kind: an immolated unit\'s SHOT burns the target too',
      setup='Dracon\'s first unit made Barbarian Bowmen (41) with the Immolation unit enchantment (0x1) shoots neutral '
            'unit 2 (Barbarian Spearmen, 39) at (30,28) in the human\'s first combat turn; tracepoints on '
            'Attack_Resolve(attacker, figures, defender, kind ...) and Spell_Damage_Roll(spell, target, damage, '
            'strength), recorded in order',
      bug='after Attack_Resolve(bowmen, ..., kind 2 = missile) a Spell_Damage_Roll(96 = Fireball, unit 2, ..., 4): '
          'the Immolation damage on a shot', fixed='Immolation only in melee')
def c188(log):
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 41, 39, 104, ench=N('UNIT_ENCH_IMMOLATION'))
    seq = []
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        a, tg = idx[att], idx[2]
        ab2 = d.lv.get('battle_units', a, 'abilities2')
        d.want('Attack_Resolve', trace=lambda h: seq.append(('Attack_Resolve', d.arg(0, h), d.arg(2, h), d.arg(3, h),
                                                             d.arg(5, h))))
        d.want('Spell_Damage_Roll', trace=lambda h: seq.append(('Spell_Damage_Roll', d.arg(0, h), d.arg(1, h),
                                                                d.arg(3, h))))
        gx, gy = d.lv.get('battle_units', tg, 'combat_x'), d.lv.get('battle_units', tg, 'combat_y')
        sx, sy = mom.grid_to_screen(gx, gy)
        d.lv.g.click(sx, sy + 8, settle=0.3)     # a shooter in range shoots on the click
        d.wait(12, None)
        d.drop('Attack_Resolve')
        d.drop('Spell_Damage_Roll')
    finally:
        d.stop()
    shot = [i for i, e in enumerate(seq) if e[0] == 'Attack_Resolve' and e[1] == a and e[2] == tg and e[3] == 2]
    burn = [e for i, e in enumerate(seq) if e[0] == 'Spell_Damage_Roll' and e[1] == N('SPELL_FIREBALL') and
            e[2] == tg and shot and i > shot[0]]
    obs = ('the bowmen (battle unit %d, abilities2 0x%x: Immolation 0x8) shoot unit 2 (battle unit %d); calls in '
           'order %s' % (a, ab2, tg, seq[:12]))
    if not ab2 & N('ABILITY2_IMMOLATION'):
        return dict(shown=None, observed=obs + ' - the shooter is not immolated')
    if not shot:
        return dict(shown=None, observed=obs + ' - no missile Attack_Resolve by the bowmen')
    if any(e[0] == 'Attack_Resolve' and e[3] == 0 and e[1] == a for e in seq):
        return dict(shown=None, observed=obs + ' - a melee attack by the bowmen too (not a pure shot)')
    return dict(shown=bool(burn), observed=obs)


def _list_first_row(lv, log):
    """Clicks the top row of an open List_Select_Dialog (NOTIFY.C): the widest short region with no hotkey."""
    rows = [c for c in mom.controls(lv) if c[4] == ' ' and 40 < c[2] - c[0] < 300 and 4 < c[3] - c[1] < 20]
    log.append('  list rows %s' % rows[:8])
    if not rows:
        return False
    x1, y1, x2, y2, _ = min(rows, key=lambda c: c[1])
    lv.g.click((x1 + x2) // 2, (y1 + y2) // 2, settle=1.5)
    return True


@case(221, 'CB', 'Animate Dead on an ENEMY corpse changes only the battle owner: on the map it stays the enemy\'s',
      setup='Dracon\'s first unit made Paladins (113; not a hero - heroes flee open-field battles) attacks neutral '
            'units 2 (Settlers, 44: one figure, damaged to 1 hit point) and 13 (Barbarian Spearmen, 39) at (30,28); '
            'the Paladins kill the Settlers; the human casts Animate Dead (195) and picks it from the list; auto combat to the end; unit 2\'s overland owner, mutation flags '
            'and square read',
      bug='unit 2 is undead but its overland owner is still 5 (neutral)', fixed='owner 0 (the caster)')
def c221(log):
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    # 10/24: hero 19 as the attacker never killed the Settlers (heroes flee open-field battles, README sec 6)
    key = _vs_neutrals(s, att, 113, 44, 39)
    s.set('units', 2, 'damage', 9)                 # 1 hit point left of 10: the first blow kills it
    mom.learn_spells(s, 0, [N('SPELL_ANIMATE_DEAD')])
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    mom.shot('start', log)
    idx = {lv.get('battle_units', i, 'unit_index'): i for i in range(lv.glob('battle_unit_count'))}
    a, tg = idx[att], idx[2]

    class _D:                                   # _approach_click wants d.lv
        pass
    dd = _D()
    dd.lv = lv
    for turn in range(8):
        if lv.get('battle_units', tg, 'status') != 0:
            break
        _approach_click(dd, a, tg, 'step')      # Paladins: 1 move
        time.sleep(4)
        if lv.get('battle_units', tg, 'status') != 0:
            break
        lv.D.key('d')
        for _ in range(20):
            time.sleep(1)
            if mom.in_combat(lv):
                break
    st = lv.get('battle_units', tg, 'status')
    if st == 0 or not mom.in_combat(lv):
        mom.finish_combat(lv, log=log)
        return dict(shown=None, observed='the Settlers (battle unit %d) status %d - not killed in a human turn' % (tg, st))
    mom.cast_spell(lv, N('SPELL_ANIMATE_DEAD'), log=log)
    time.sleep(2)
    mom.shot('animate_list', log)
    picked = _list_first_row(lv, log)
    time.sleep(3)
    st2, own2 = lv.get('battle_units', tg, 'status'), lv.get('battle_units', tg, 'owner')
    mom.finish_combat(lv, log=log)
    after = tuple(lv.get('units', 2, f) for f in ('owner', 'type', 'mutation_flags', 'x', 'y'))
    obs = ('Settlers killed (status %d); Animate Dead list row clicked %s; battle unit after: status %d, owner %d; '
           'unit 2 on the map after the battle (owner, type, mutation_flags, x, y) %s (MUT_UNDEAD 0x%x)'
           % (st, picked, st2, own2, after, N('MUT_UNDEAD')))
    if st2 != 0 or own2 != 0:
        return dict(shown=None, observed=obs + ' - the corpse was not raised for the human')
    if after[1] != 44 or not after[2] & N('MUT_UNDEAD'):
        return dict(shown=None, observed=obs + ' - unit 2 is not the risen Settlers any more')
    return dict(shown=after[0] == 5, observed=obs)


@case(222, 'CB', 'Call Lightning spares Wraith Form, not Magic Immunity',
      setup='Dracon\'s first unit made Barbarian Spearmen (not the hero) attacks neutral units 2 made Paladins (113: '
            'Magic Immunity) and 13 made Barbarian '
            'Spearmen (39) with Wraith Form (unit enchantment 0x20) at (30,28); the human casts Call Lightning (35) '
            'and ends its combat turns (D) for up to 90 s; tracepoint on Spell_Damage_Roll(3, target, ...) - the '
            'bolts of Call_Lightning_Turn: their targets; the Paladins\' abilities and figures read',
      bug='bolts strike the Magic-Immune Paladins (never the Wraith Form unit)', fixed='neither is struck')
def c222(log):
    import dbg
    import cases_cb as CB
    from cases_cb5 import _done_poll
    s = mom.SaveFile()
    mom.learn_spells(s, 0, [N('SPELL_CALL_LIGHTNING')])
    att = CB._human_unit_at_dracon(s)
    # Dracon's first unit is hero 19: made Barbarian Spearmen (10/24: with the hero the combat book never opened -
    # a hero caster, and heroes flee open-field battles)
    key = _vs_neutrals(s, att, 39, 113, 39)
    s.set('units', 13, 'enchantments', N('UNIT_ENCH_WRAITH_FORM'))
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        pal, wr = idx[2], idx[13]
        ab = d.lv.get('battle_units', pal, 'abilities')
        f0 = d.lv.get('battle_units', pal, 'figures_alive')
        mom.cast_spell(d.lv, N('SPELL_CALL_LIGHTNING'), log=log)
        d.want('Spell_Damage_Roll', when=lambda h: d.arg(0, h) == N('SPELL_WALL_OF_STONE'),
               trace=lambda h: d.arg(1, h))
        poll, t0 = _done_poll(d), time.time()
        while time.time() - t0 < 90 and len(d.traces.get('Spell_Damage_Roll', [])) < 6:
            d.wait(5, poll)
        bolts = list(d.traces.get('Spell_Damage_Roll', []))
        d.drop('Spell_Damage_Roll')
        f1 = d.lv.get('battle_units', pal, 'figures_alive')
    finally:
        d.stop()
    obs = ('Paladins (battle unit %d, abilities 0x%x, Magic Immunity 0x%x), Wraith Form spearmen (battle unit %d); '
           'bolt targets %s; Paladins\' figures %d -> %d' % (pal, ab, N('ABILITY_MAGIC_IMMUNITY'), wr, bolts, f0, f1))
    if not ab & N('ABILITY_MAGIC_IMMUNITY'):
        return dict(shown=None, observed=obs + ' - the Paladins are not Magic Immune')
    if not bolts:
        return dict(shown=None, observed=obs + ' - no bolt (the cast unconfirmed)')
    return dict(shown=pal in bolts and wr not in bolts, observed=obs)


def _aura(lv, name):
    import struct
    base = lv.g.far(mom.SYM[name][0])
    return list(struct.unpack('<7h', lv.g.ram(base, 14)))


@case(261, 'CB', 'neutral units never get their Holy Bonus resistance (the aura loop stops at num_players)',
      setup='Dracon\'s first unit made Paladins (113: Holy Bonus) attacks neutral units 2 (Paladins) and 13 at '
            '(30,28); at the human\'s first combat turn the aura tables p_aura_holy_bonus[0..6] and '
            'p_aura_resistance[0..6] read (Combat_Start_Auras: resistance += holy bonus for players < num_players)',
      bug='the neutral player (5): holy bonus set, resistance aura 0; the human (0): resistance = its holy bonus',
      fixed='the neutral resistance aura = its holy bonus too')
def c261(log):
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 113, 113, 104)
    np = s.get('num_players')
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    holy, res = _aura(lv, 'p_aura_holy_bonus'), _aura(lv, 'p_aura_resistance')
    mom.finish_combat(lv, log=log)
    obs = 'num_players %d; p_aura_holy_bonus %s; p_aura_resistance %s (index = player, 5 neutral)' % (np, holy, res)
    if holy[0] <= 0 or holy[5] <= 0:
        return dict(shown=None, observed=obs + ' - a Holy Bonus did not register')
    if res[0] != holy[0]:
        return dict(shown=None, observed=obs + ' - the human\'s aura is not its holy bonus either')
    return dict(shown=res[5] == 0 and np <= 5, observed=obs)


@case(251, 'CB', 'an invisible obstacle in the centre of open-field battles (Combat_Mark_Wall_Obstacles)',
      setup='Dracon\'s first unit made Barbarian Spearmen attacks neutral units 2 and 13 (Barbarian Spearmen / High '
            'Men Spearmen, ground units) at (30,28) in the open; the battlefield\'s own backdrop and cost_ground[11][6] '
            'read through the debugger (EMS page frame); a memory-write watchpoint on the movement map\'s centre cell '
            'p_combat_move_cost[BATTLEFIELD_CELL(6, 11)] while the human presses Done and the NEUTRALS move: every '
            'write, its writer and the value. Combat_Mark_Wall_Obstacles runs for the human\'s moves only with walls / '
            'Flying Fortress (COMBMOVE.C), but unconditionally in AI_Move_Toward / AI_Move_Path (COMBTACT.C) - the AI '
            'side is where an open field gets the obstacle (10/24: the human\'s map read 2, the wrong map)',
      bug='Combat_Mark_Wall_Obstacles writes 255 (impassable) into the centre cell of an open-field battle (backdrop 0) '
          'whose own cost map lets it pass', fixed='no such write outside city battles')
def c251(log):
    import struct
    import dbg
    import cases_cb as CB
    from cases_cb5 import _done_poll
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 39, 39, 104)
    cell = N('BF_FEATURE_Y') * N('BATTLEFIELD_W') + N('BF_FEATURE_X')
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        mc = d.lv.g.ram(d.lv.g.ds + (d.lv.glob('p_combat_move_cost') & 0xffff), N('BATTLEFIELD_CELLS'))
        off, seg = struct.unpack('<HH', d.lv.g.ram(d.lv.g.ds + mom.SYM['p_battlefield'][0], 4))
        bd = struct.unpack('<h', d.dump(seg, off + mom.resolve('Battlefield', 'backdrop')[0], 2))[0]
        own = d.dump(seg, off + mom.resolve('Battlefield', 'cost_ground')[0] + cell, 1)[0]
        mom.shot('battle', log)
        lin = d.lv.g.ds + (d.lv.glob('p_combat_move_cost') & 0xffff) + cell
        d.watch('centre', lin, size=1, trace=lambda h: (h['writer'], h['new'][0]))
        d.lv.D.key('d')                         # Done: the neutrals' turn
        poll, t0 = _done_poll(d), time.time()
        while time.time() - t0 < 60 and not any('Combat_Mark_Wall_Obstacles' in str(w) for w, _ in
                                                d.traces.get('centre', [])):
            d.wait(5, poll)
        writes = list(d.traces.get('centre', []))
        d.unwatch('centre')
    finally:
        d.stop()
    mark = [v for w, v in writes if 'Combat_Mark_Wall_Obstacles' in str(w)]
    by = {}
    for w, v in writes:
        k = str(w).split('+')[0]
        by.setdefault(k, set()).add(v)
    obs = ('backdrop %d; centre cell (6, 11): the battlefield\'s own ground cost %d, the human\'s movement map %d; '
           'writes to the movement map\'s centre cell in the neutral turn (writer: values) %s'
           % (bd, own, mc[cell], {k: sorted(v) for k, v in by.items()}))
    if bd != 0:
        return dict(shown=None, observed=obs + ' - not an open-field battle')
    if own == 255:
        return dict(shown=None, observed=obs + ' - the battlefield itself blocks the centre here')
    if not mark:
        return dict(shown=None, observed=obs + ' - no Combat_Mark_Wall_Obstacles write seen (no AI move?)')
    return dict(shown=all(v == 255 for v in mark), observed=obs)


@case(234, 'CB', 'a dispelled enchantment leaves its abilities on the battle unit (Unit_Enchant_Bonus only ORs bits in)',
      setup='Dracon\'s first unit made Barbarian Spearmen attacks neutral units 2 (Barbarian Spearmen, 39: no '
            'immunities) with Guardian Wind '
            '(unit enchantment 0x2: Missiles Immunity 0x4) and 13 at (30,28); the human casts Dispel Magic (OK at '
            'the default power) on unit 2, again in later combat turns until Guardian Wind is gone (up to 4 casts); '
            'unit 2\'s enchantments and its battle record\'s abilities read after each cast',
      bug='Guardian Wind removed, Missiles Immunity still in the battle abilities', fixed='the immunity removed too')
def c234(log):
    import cases_cb as CB
    s = mom.SaveFile()
    mom.learn_spells(s, 0, [N('SPELL_DISPEL_MAGIC')])
    att = CB._human_unit_at_dracon(s)
    # Dracon's first unit is hero 19: made Barbarian Spearmen (10/24: with the hero the combat book never opened -
    # a hero caster, and heroes flee open-field battles)
    key = _vs_neutrals(s, att, 39, 39, 104)
    s.set('units', 2, 'enchantments', N('UNIT_ENCH_GUARDIAN_WIND'))
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    idx = {lv.get('battle_units', i, 'unit_index'): i for i in range(lv.glob('battle_unit_count'))}
    t = idx[2]
    gw, mi = N('UNIT_ENCH_GUARDIAN_WIND'), N('ABILITY_MISSILES_IMMUNITY')
    def step():                                 # (unit 2's enchantments, the record's abilities, whose record it is)
        return (lv.get('units', 2, 'enchantments') & 0xffffffff, lv.get('battle_units', t, 'abilities'),
                lv.get('battle_units', t, 'unit_index'), mom.in_combat(lv))
    steps = [step()]
    for k in range(4):
        if not mom.in_combat(lv) or lv.get('battle_units', t, 'status') != 0:
            break
        mom.cast_spell(lv, N('SPELL_DISPEL_MAGIC'), target=t, log=log)
        time.sleep(2)
        steps.append(step())
        if not steps[-1][0] & gw:
            break
        lv.D.key('d')                           # the next combat turn: another cast
        for _ in range(25):
            time.sleep(1)
            if mom.in_combat(lv):
                break
    mom.finish_combat(lv, log=log)
    obs = ("unit 2 (enchantments, battle abilities, the record's unit, in combat) at the start and after each Dispel "
           'Magic: %s (Guardian Wind 0x%x, Missiles Immunity 0x%x)' % (['(0x%x, 0x%x, %d, %s)' % x for x in steps], gw, mi))
    if steps[-1][2] != 2 or not steps[-1][3]:   # r7d 10/26: abilities 0x1819 after the last cast - not unit 2's record
        return dict(shown=None, observed=obs + " - the last read is not unit 2's battle record in combat")
    if not steps[0][1] & mi:
        return dict(shown=None, observed=obs + ' - Guardian Wind gave no immunity at the start')
    if steps[-1][0] & gw:
        return dict(shown=None, observed=obs + ' - Guardian Wind was never dispelled')
    return dict(shown=bool(steps[-1][1] & mi), observed=obs)


@case(302, 'CB', 'the battlefield\'s open diamond is two diagonal lines too big at the bottom-right edge',
      setup='Dracon\'s first unit attacks neutral units 2 and 13 at (30,28) in the open; at the human\'s first combat '
            'turn the battlefield\'s cost_flying map (terrain-free: only the corner blocks of '
            'Battlefield_Build_Move_Costs are 255) read through the debugger (EMS page frame): the range of x + y and '
            'x - y over the open cells',
      bug='open cells reach x + y = 31 while the other edges stop at x + y = 11 and x - y = -9 / 9 (symmetric would be '
          '11..29)', fixed='11 <= x + y <= 29')
def c302(log):
    import struct
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 39, 39, 104)
    W, H = N('BATTLEFIELD_W'), N('BATTLEFIELD_CELLS') // N('BATTLEFIELD_W')
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        off, seg = struct.unpack('<HH', d.lv.g.ram(d.lv.g.ds + mom.SYM['p_battlefield'][0], 4))
        fly = d.dump(seg, off + mom.resolve('Battlefield', 'cost_flying')[0], W * H)
    finally:
        d.stop()
    open_cells = [(x, y) for y in range(H) for x in range(W) if fly[y * W + x] != 255]
    sums = sorted({x + y for x, y in open_cells})
    diffs = sorted({x - y for x, y in open_cells})
    beyond = sorted({(x, y) for x, y in open_cells if x + y >= 30})
    obs = ('open cells %d; x + y from %d to %d; x - y from %d to %d; open cells with x + y >= 30: %d, e.g. %s'
           % (len(open_cells), sums[0], sums[-1], diffs[0], diffs[-1], len(beyond), beyond[:6]))
    if len(open_cells) < 100:
        return dict(shown=None, observed=obs + ' - the map was not read')
    return dict(shown=sums[0] == 11 and sums[-1] == 31 and diffs[0] == -9 and diffs[-1] == 9, observed=obs)


@case(313, 'CB', 'the combat AI scores a MELEE attack on a flyer by a non-flying missile-only unit',
      setup='Dracon\'s first unit made a Great Drake (164: flying) with Guardian Wind (Missiles Immunity) attacks '
            'neutral units 2 and 13 made Barbarian Bowmen (41: missile only, no flight, melee 1 / ranged 1) at '
            '(30,28): AI_Choose_Target drops a Missiles-Immune target for a bow unit whose melee x 3 >= ranged x 2 and '
            'retries the scan in MELEE mode (10/24: without the immunity only ranged scores were asked); the human '
            'ends its combat turns (D); stops at '
            'AI_Target_Score(attacker, target, ranged) entries with a bowman attacking the drake, run to the return: '
            'the mode and the score',
      bug='mode 0 (melee) scores above -200 (only ranged_type 255 is refused against a flyer)',
      fixed='-200 in melee mode')
def c313(log):
    import dbg
    import cases_cb as CB
    from cases_cb5 import _done_poll
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 164, 41, 41, ench=N('UNIT_ENCH_GUARDIAN_WIND'))
    calls = []
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        drake, bows = idx[att], {idx[2], idx[13]}
        d.want('AI_Target_Score', when=lambda h: d.arg(0, h) in bows and d.arg(1, h) == drake)
        poll, t0 = _done_poll(d), time.time()
        while time.time() - t0 < 120 and not any(c[2] == 0 for c in calls):
            h = d.wait(10, poll)
            if h is None or h['name'] != 'AI_Target_Score':
                continue
            a = tuple(d.arg(k, h) for k in range(3))
            calls.append(a + (d.ret_ax(),))
            d.go()
        flags = (d.lv.get('battle_units', drake, 'move_flags'), d.lv.get('battle_units', min(bows), 'ranged_type'))
    finally:
        d.stop()
    obs = ('AI_Target_Score(bowman, drake, mode) = score: %s; the drake\'s move_flags 0x%x, the bowmen\'s ranged_type %d'
           % (['%s = %s' % (c[:3], c[3]) for c in calls[:8]], flags[0], flags[1]))
    melee = [c for c in calls if c[2] == 0]
    if not melee:
        return dict(shown=None, observed=obs + ' - no melee-mode score asked')
    return dict(shown=any(c[3] is not None and c[3] > -200 for c in melee), observed=obs)


class _Lv:                                      # _approach_click wants d.lv
    def __init__(self, lv):
        self.lv = lv


def _ghoul_strikes(target_type, log, tag):
    """Dracon's first unit made Ghouls (166: Create Undead) attacks neutral unit 2 made target_type (and unit 13) at
    (30,28), walking up over combat turns until unit 2 has taken damage; unit 2's battle damage buckets read."""
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = _vs_neutrals(s, att, 166, target_type, 104)
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    idx = {lv.get('battle_units', i, 'unit_index'): i for i in range(lv.glob('battle_unit_count'))}
    a, t = idx[att], idx[2]
    info = (lv.get('battle_units', a, 'ability_flags'), lv.get('battle_units', t, 'abilities'))
    dmg = None
    for turn in range(5):
        if not mom.in_combat(lv):
            break
        _approach_click(_Lv(lv), a, t)
        time.sleep(4)
        dmg = [lv.get('battle_units', t, 'damage[%d]' % k) for k in range(3)]
        if sum(dmg) > 0 or lv.get('battle_units', t, 'status') != 0:
            break
        lv.D.key('d')
        for _ in range(25):
            time.sleep(1)
            if mom.in_combat(lv):
                break
    dmg = [lv.get('battle_units', t, 'damage[%d]' % k) for k in range(3)]
    mom.finish_combat(lv, log=log)
    log.append('%s: Ghouls ability_flags 0x%x, unit 2 abilities 0x%x; unit 2 damage (normal, drain, destroy) %s'
               % (tag, info[0], info[1], dmg))
    return info, dmg


@case(258, 'CB', 'Death Immunity does not stop a Create Undead attacker\'s damage going to the undead bucket',
      setup='Dracon\'s first unit made Ghouls (166: Create Undead) attacks neutral unit 2 made Skeletons (Death '
            'Immunity, no Magic Immunity) at (30,28), walking up until it strikes; unit 2\'s battle damage buckets '
            '(normal, drain = "raises undead", destroy) read. Control: unit 2 made Paladins (113: Magic Immunity)',
      bug='the Death-immune Skeletons take drain-bucket damage (Attack_Resolve tests Magic Immunity only); the Paladins '
          'normal damage', fixed='normal damage for the Death-immune unit too')
def c258(log):
    (af, ab), dmg = _ghoul_strikes(N('UNIT_SKELETONS'), log, 'Skeletons')
    (af2, ab2), dmg2 = _ghoul_strikes(113, log, 'control, Paladins')
    obs = ('Ghouls ability_flags 0x%x (Create Undead 0x%x); Skeletons abilities 0x%x (Death Immunity 0x%x): damage '
           '(normal, drain, destroy) %s; Paladins abilities 0x%x (Magic Immunity 0x%x): %s'
           % (af, N('AFLAG_CREATE_UNDEAD'), ab, N('ABILITY_DEATH_IMMUNITY'), dmg, ab2, N('ABILITY_MAGIC_IMMUNITY'),
              dmg2))
    if not af & N('AFLAG_CREATE_UNDEAD') or not ab & N('ABILITY_DEATH_IMMUNITY') or ab & N('ABILITY_MAGIC_IMMUNITY'):
        return dict(shown=None, observed=obs + ' - the abilities are not as set up')
    if not sum(dmg) or not sum(dmg2):
        return dict(shown=None, observed=obs + ' - a target took no damage')
    return dict(shown=dmg[1] > 0 and dmg2[1] == 0, observed=obs)


@case(246, 'CB', 'resistance auras count twice against Terror (in the unit\'s resistance and in the roll\'s modifier)',
      setup='AI wizard 1\'s city 21 moved to (30,28); its units 2 (Paladins, 113: Holy Bonus - a resistance aura for '
            'wizard 1) and 13 (High Men Spearmen) defending it; Dracon\'s stack attacks; the human casts Terror and '
            'presses Done; stops at Roll_Resist entries called from Combat_Turn_Start (the Terror roll): the '
            'BattleUnit is passed BY VALUE (110 bytes), so the copy\'s resist byte and the modifier after it are read '
            'on the stack; p_aura_resistance[1] and the units\' type resistance read too',
      bug='modifier = aura + 1 while the copy\'s resistance already holds the aura (type resistance + aura): the aura '
          'counted twice', fixed='the aura once')
def c246(log):
    import struct
    import dbg
    from layout import L
    import cases_cb as CB
    from cases_cb5 import _done_poll
    s = mom.SaveFile()
    mom.learn_spells(s, 0, [N('SPELL_TERROR')])
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):
        s.set('cities', 21, f, v)
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    for u, ty in ((2, 113), (13, 104)):
        for f, v in (('owner', 1), ('type', ty), ('hero_slot', -1), ('level', 0), ('experience', 0),
                     ('enchantments', 0)):
            s.set('units', u, f, v)
    size, roff = L.size('BattleUnit'), mom.resolve('BattleUnit', 'resist')[0]
    rolls = []
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        aura = struct.unpack('<7h', d.lv.g.ram(d.lv.g.far(mom.SYM['p_aura_resistance'][0]), 14))
        mom.cast_spell(d.lv, N('SPELL_TERROR'), log=log)
        ench = d.lv.g.ram(d.lv.g.far(mom.SYM['p_combat_enchants'][0]), 40)

        def roll(h):
            sp = h['SP']
            copy = d.dump(h['SS'], (sp + 4) & 0xffff, size)
            mod = d.word(h['SS'], (sp + 4 + size) & 0xffff)
            return copy[roff], mod, struct.unpack_from('<h', copy, mom.resolve('BattleUnit', 'unit_index')[0])[0]
        d.want('Roll_Resist', when=lambda h: 'Combat_Turn_Start' in str(d.where(d.word(h['SS'], h['SP'] + 2, False),
                                                                               d.word(h['SS'], h['SP'], False))),
               trace=lambda h: rolls.append(roll(h)))
        poll, t0 = _done_poll(d), time.time()
        while time.time() - t0 < 90 and not rolls:
            d.wait(5, poll)
        d.drop('Roll_Resist')
        types = {u: (d.lv.get('units', u, 'type')) for u in (2, 13)}
        # the type's own resistance (unit_types is initialised data): the baseline the copy is judged against
        base = {u: d.lv.get('unit_types', t, 'resist') for u, t in types.items()}
    finally:
        d.stop()
    obs = ('p_aura_resistance %s (wizard 1 = index 1); Terror (attacker\'s) %d; Terror rolls from Combat_Turn_Start '
           '(the copy\'s resist, modifier, unit) %s; unit types %s, their type resistance %s'
           % (list(aura), ench[N('CENCH_TERROR_ATT')], rolls, types, base))
    if not rolls:
        return dict(shown=None, observed=obs + ' - no Terror roll reached')
    if aura[1] <= 0:
        return dict(shown=None, observed=obs + ' - wizard 1 has no resistance aura')
    if any(u not in base for _, _, u in rolls):
        return dict(shown=None, observed=obs + ' - a roll for a unit that was not set up')
    # counted twice = the aura in the copy's resistance AND in the modifier; once = in only one of them
    twice = [r == base[u] + aura[1] and m == aura[1] + 1 for r, m, u in rolls]
    once = [(r == base[u] + aura[1]) != (m == aura[1] + 1) and r in (base[u], base[u] + aura[1])
            and m in (1, aura[1] + 1) for r, m, u in rolls]
    if all(twice):
        return dict(shown=True, observed=obs + ' - every roll: resist = type + aura and modifier = aura + 1')
    if all(once):
        return dict(shown=False, observed=obs + ' - the aura counted once per roll')
    return dict(shown=None, observed=obs + ' - the copy\'s resist / modifier fit neither reading (another bonus?)')
