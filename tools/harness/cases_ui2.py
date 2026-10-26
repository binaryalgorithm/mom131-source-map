"""P4 B4 UI cases, batch 2 (N247): lists and values the screens draw, read where the game builds them."""
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

import mom
from regress import case

N = mom.N


SPARE44 = {1, 2, 7, 11, 17, 18, 28, 30, 31}


@case(44, 'UI', 'production list caps: 12 units (rebuilt without Spearmen / Swordsmen) and 20 entries in all',
      setup='AI wizard 1\'s city 21 made High Men (race 8) with every building built but nine that no High Men / '
            'generic unit needs (1 2 7 11 17 18 28 30 31: more than 8 left to offer), production on auto choose (-4, so AI_Wizard_Choose_Production builds its list in the '
            'turn change); breakpoint on City_Build_Options\' own last line for city 21: n_options, n_units and the '
            'options array read through its pointer arguments (unit entries = type + 100) - the same list the city screen\'s Change shows',
      bug='12+ units: Spearmen (104) and Swordsmen (105) left out; at most 20 entries, so at most 8 buildings beside '
          '12 units', fixed='every unit and building listed')
def c44(log):
    import dbg
    city = 21
    s = mom.SaveFile()
    s.set('cities', city, 'race', N('RACE_HIGH_MEN'))
    # the 14 High Men / generic units need buildings 3 5 6 8 9 12-15 21 24 25 32 33: all built, so the first pass
    # finds 12+ units; the nine left unbuilt are leaves whose own prerequisites stay built
    for b in range(1, 36):
        s.set('cities', city, 'buildings[%d]' % b, N('BLDSTATE_NONE') if b in SPARE44 else N('BLDSTATE_BUILT'))
    s.set('cities', city, 'producing', N('PROD_AUTO_CHOOSE'))
    import struct
    got = []

    def at_end(h):
        # the callee's own last line ("*n_options = count;"): *n_total already = count, *n_units written; its frame:
        # [bp+6] city, [bp+8] n_options, [bp+10] n_total, [bp+12] n_units, [bp+14] options (near pointers into DS).
        # 10/24: d.call's return stop in the caller (another overlay) missed and read a reused frame
        # the pointers sit on the stack (SS) and point into DS ("mov bx,[bp+10]; mov [bx],di"); read with the
        # debugger's MEMDUMPBIN (the CPU's view while stopped) - 10/24: Game Link reads here gave stale / garbage
        # counts (32, 16333) beside a plausible list
        ss, ds, bp = h['SS'], h['DS'], h['BP']
        fr = struct.unpack('<5h', d.dump(ss, bp + 6, 10))
        n = struct.unpack('<h', d.dump(ds, fr[2] & 0xffff, 2))[0]
        nu = struct.unpack('<h', d.dump(ds, fr[3] & 0xffff, 2))[0]
        opts = list(struct.unpack('<24h', d.dump(ds, fr[4] & 0xffff, 48)))
        got.append((n, nu, opts[:max(0, min(n, 24))], (ss, ds)))
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/CITYPROD.C', '*n_options = count;', fn='City_Build_Options',
                    when=lambda h: d.word(h['SS'], h['BP'] + 6) == city, trace=at_end)
        t0 = time.time()
        poll = d.end_turn_poll()
        while time.time() - t0 < 300 and not got:
            d.wait(10, poll)
    finally:
        d.stop()
    if not got:
        return dict(shown=None, observed='City_Build_Options(%d, ...) did not reach its end in the turn change' % city)
    n_opt, n_units, opts, segs = got[0]
    log.append('City_Build_Options end: SS, DS %s' % (segs,))
    units = [o - N('PROD_UNIT_BASE') for o in opts[:n_units]]
    blds = opts[n_units:]
    obs = ('City_Build_Options(21): n_options %d, n_units %d; unit types %s; buildings %s' % (n_opt, n_units, units,
                                                                                                 blds))
    obs += '; unbuilt candidates listed %s, left off %s' % (sorted(set(blds) & SPARE44), sorted(SPARE44 - set(blds)))
    if n_units < 12 and (104 in units or 105 in units):
        return dict(shown=None, observed=obs + ' - fewer than 12 units: the unit cap not in play')
    # the unit cap: 12 found -> rebuilt without Spearmen / Swordsmen (fixed: both listed)
    if 104 in units or 105 in units:
        return dict(shown=False, observed=obs + ' - 12+ units and Spearmen / Swordsmen still listed')
    # the total cap: the building loop stops at 20 entries, i.e. at exactly 20 - n_units buildings
    if n_opt < 20 or len(blds) != 20 - n_units:
        return dict(shown=None, observed=obs + ' - unit cap shown, but fewer buildings offered than the total cap '
                                               'allows: the 20-entry cap not in play')
    return dict(shown=bool(SPARE44 - set(blds)), observed=obs + ' - both caps: Spearmen / Swordsmen dropped and the '
                                                                'list cut at 20 with unbuilt candidates left off')


def _max_hits(item_powers, unit_ench, log, tag):
    """Hero 19 (hired slot 0) damaged by 2, holding item 1 (moved from the Sage's slot) with item_powers, its unit
    enchantments unit_ench; Unit_Max_Hits(19) - the map health bar / overland healing - run to its return."""
    import dbg
    s = mom.SaveFile()
    s.set('wizards', 0, 'hired_heroes[2].item_index[0]', -1)
    s.set('wizards', 0, 'hired_heroes[0].item_index[0]', 1)
    s.set('items', 1, 'powers', item_powers)
    s.set('units', 19, 'enchantments', unit_ench)
    s.set('units', 19, 'damage', 2)
    # Charm of Life adds 25% after the bonuses: off, so the two runs differ by exactly the item's 3
    s.set('wizards', 0, 'globals.charm_of_life', 0)
    d = dbg.boot(s, log=log)
    try:
        args, ax = d.call('Unit_Max_Hits', lambda a: a[0] == 19, 1, timeout=240, poll=d.end_turn_poll())
        d.go()
    finally:
        d.stop()
    log.append('%s: Unit_Max_Hits(19) = %s' % (tag, ax))
    return ax if args is not None else None


@case(273, 'UI', 'a Lion Heart ITEM gives no extra hits outside combat (Unit_Max_Hits reads the unit\'s own '
                 'enchantments only)',
      setup='hero 19 damaged by 2 (so the map health bar / the turn\'s healing ask for its maximum): run A with item 1 '
            '(moved to it) given the Lion Heart power (0x04000000); run B with no item power and the Lionheart unit '
            'enchantment (0x04000000 in the unit\'s own word); Charm of Life off in both; Unit_Max_Hits(19) run to its return in each (the map draw or the turn change)',
      bug='A is 3 below B: the item\'s +3 hit points are not counted', fixed='the same +3 for both')
def c273(log):
    a = _max_hits(N('ITEM_POWER_LION_HEART'), 0, log, 'Lion Heart item')
    b = _max_hits(0, N('UNIT_ENCH_LIONHEART'), log, 'control, Lionheart enchantment')
    obs = 'Unit_Max_Hits(hero 19): with the Lion Heart item %s; with the Lionheart enchantment %s' % (a, b)
    if a is None or b is None:
        return dict(shown=None, observed=obs + ' - Unit_Max_Hits(19) not called')
    return dict(shown=a == b - 3, observed=obs)


def _book_mana(mana_pct, log, tag):
    """The human owns every node, magic setting 2 (node power x 1.5), the power split mana_pct / rest to skill; the
    overland spellbook opened (S) so Spellbook_Build runs; power_base and net_mana_per_turn read."""
    import time
    s = mom.SaveFile()
    s.set('magic_setting', value=2)
    for k in range(N('NUM_NODES')):
        s.set('nodes', k, 'owner', 0)
    for f, v in (('mana_percentage', mana_pct), ('skill_percentage', 100 - mana_pct), ('research_percentage', 0)):
        s.set('wizards', 0, f, v)
    lv = mom.boot(s)
    lv.D.key('s')
    time.sleep(4)
    mom.shot('book_%d' % mana_pct, log)
    r = (lv.get('wizards', 0, 'power_base'), lv.get('wizards', 0, 'mana_percentage'), lv.glob('net_mana_per_turn'))
    for _ in range(3):
        if mom.on_map(lv):
            break
        lv.D.key('esc')
        time.sleep(1.5)
    log.append('%s: (power_base, mana %%, net_mana_per_turn) %s' % (tag, r))
    return r


def _trunc_div(a, b):
    q = abs(a) // b
    return q if a >= 0 else -q


@case(269, 'UI', 'the spellbook\'s mana per turn wraps for powerful wizards (imul then cdq drops the high word)',
      setup='the human owns all 30 nodes, magic setting 2 (power base about 500); the overland spellbook opened (S) '
            'twice: all power to mana (100%: the product mana% x power base passes 32767) and a control at 30% (no '
            'overflow) that gives the mana upkeep U = 30 x power / 100 - net; Spellbook_Build\'s global '
            'net_mana_per_turn read each time',
      bug='at 100%: net = (the product\'s low 16 bits, signed) / 100 - U, not 100 x power / 100 - U',
      fixed='power - U')
def c269(log):
    pb1, mp1, net1 = _book_mana(100, log, 'all to mana')
    pb2, mp2, net2 = _book_mana(30, log, 'control, 30% mana')
    up = mp2 * pb2 // 100 - net2
    prod = mp1 * pb1
    low = (prod & 0xffff) - (0x10000 if prod & 0x8000 else 0)
    wrapped, right = _trunc_div(low, 100) - up, prod // 100 - up
    obs = ('power base %d; at 100%% net_mana_per_turn %d (product %d); control at 30%%: net %d -> upkeep %d; '
           'predicted with the wrap %d, without %d' % (pb1, net1, prod, net2, up, wrapped, right))
    if pb1 != pb2 or prod <= 32767:
        return dict(shown=None, observed=obs + ' - no overflow set up (or the power base changed)')
    return dict(shown=net1 == wrapped and net1 != right, observed=obs)


@case(130, 'UI', 'turn report "UNIT SPELLS DISSIPATED" logs unit INDEXES that the report prints as unit TYPE names',
      setup='the human: mana reserve 0 and no mana income (power all to skill / research), unit 1 (a normal unit) '
            'given unit enchantments 0x0003ffff (their upkeep cannot be paid); one turn change: Upkeep_Shortfall_Mana '
            'cuts categories until the upkeep is covered; the report list report_unit_spells_lost[] and its count '
            'read after the turn (TurnReport_Draw_Text prints unit_types[entry].name)',
      bug='an entry is unit 1\'s INDEX (1): the report names unit type 1, not unit 1\'s own type',
      fixed='the unit\'s own name')
def c130(log):
    s = mom.SaveFile()
    u = 1
    utype = s.get('units', u, 'type')
    s.set('units', u, 'enchantments', 0x0003ffff)
    for f, v in (('mana_reserve', 0), ('mana_percentage', 0), ('skill_percentage', 50), ('research_percentage', 50)):
        s.set('wizards', 0, f, v)
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    n = lv.glob('report_unit_spells_lost_count')
    entries = [lv.glob('report_unit_spells_lost', i) for i in range(max(0, min(n, 20)))]
    ench = lv.get('units', u, 'enchantments') & 0xffffffff
    obs = ('unit 1 (type %d) enchantments after the turn 0x%x; report_unit_spells_lost %s (count %d) - printed as the '
           'names of unit types %s' % (utype, ench, entries, n, entries))
    if ench == 0x0003ffff or u not in entries:
        return dict(shown=None, observed=obs + ' - unit 1\'s enchantments were not stripped / logged')
    return dict(shown=utype != u, observed=obs)


def _lbx_records(name, entry=0):
    """(count, size, bytes) of a records entry of gog/official_131/<name>.LBX (u16 count, u16 size, records)."""
    import struct
    b = open(_mp.GAME + 'official_131/%s.LBX' % name, 'rb').read()
    start, end = struct.unpack_from('<II', b, 8 + 4 * entry)
    count, size = struct.unpack_from('<HH', b, start)
    return count, size, b[start + 4:end]


@case(70, 'UI', 'Merging needs 22 Nature books in ITEMPOW.LBX, so the item forge can never offer it',
      setup='data: gog/official_131/ITEMPOW.LBX entry 0, record 62 - name, realm, books_required (struct ItemPower: '
            'name[18], type_mask, cost, group, realm, books_required); the forge offers a power when the wizard\'s '
            'books of its realm >= books_required (ITEMFORG.C), and a wizard holds at most 13 books of a realm (12 '
            'at the start, lair books stop above 12)',
      bug='Merging: realm 0 (Nature), books_required 22 (unreachable)', fixed='(data) a reachable count')
def c70(log):
    import struct
    count, size, recs = _lbx_records('ITEMPOW')
    r = recs[62 * size:63 * size]
    name = r[:18].split(b'\0')[0].decode('latin-1')
    realm, books = r[23], r[24]
    others = sorted({recs[k * size + 24] for k in range(count) if recs[k * size + 22] == 9 and k != 62})
    obs = ('ITEMPOW.LBX: %d records of %d bytes; record 62 "%s": realm %d, books_required %d; the other special '
           'powers (group 9) need %s books' % (count, size, name, realm, books, others))
    if name != 'Merging' or size != 30:
        return dict(shown=None, observed=obs + ' - not the record expected')
    return dict(shown=books > 13, observed=obs)


def _raze_run(via_scroll, log, tag):
    """Neutral city 12 (size 2, 6 buildings) moved to (30,28) with neutral units 2 and 13 (Barbarian Spearmen)
    defending it; Dracon's stack attacks, auto combat. via_scroll: answer the Raze dialog N (capture), then R on the
    results scroll and R in its confirmation; else R at the dialog. -> (gold before, gold after, fame before, fame
    after, combat_end_value on the scroll, raze option, the dialog answers given, (the city's size, the buildings'
    tenth T at the start, T at the scroll)). T = the sum of p_building_data[i].cost / 10 over buildings[i] > -1 from
    Barracks on, as Combat_End and the scroll's R pay it."""
    import cases_cb as CB
    city = 12
    s = mom.SaveFile()
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):
        s.set('cities', city, f, v)
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    for u in (2, 13):
        for f, v in (('owner', 5), ('type', 39), ('hero_slot', -1), ('level', 0), ('enchantments', 0)):
            s.set('units', u, f, v)
    lv = mom.boot(s)
    import struct
    fo = mom.resolve('GameConfig', 'raze_city')[0]
    raze_opt = struct.unpack('<h', lv.g.ram(lv.g.ds + mom.SYM['game_config'][0] + fo, 2))[0]
    g0, f0 = lv.get('wizards', 0, 'gold'), lv.get('wizards', 0, 'fame')
    bsz, cfo = mom.L.size('Building'), mom.resolve('Building', 'cost')[0]
    bd = lv.g.far(mom.SYM['p_building_data'][0])

    def tenth():
        return sum(struct.unpack('<h', lv.g.ram(bd + i * bsz + cfo, 2))[0] // 10
                   for i in range(N('BLD_BARRACKS'), N('NUM_BUILDINGS'))
                   if lv.get('cities', city, 'buildings[%d]' % i) > -1)
    size, t_start, t_scroll = lv.get('cities', city, 'size'), tenth(), None
    mom.start_combat(lv, key, log=log)
    answers, cev, scroll_r = [], None, False
    t0 = time.time()
    while time.time() - t0 < 240:
        if mom.on_map(lv):
            break
        keys = {c[4].upper() for c in mom.controls(lv)}
        if mom.in_combat(lv):
            lv.D.key('a')
        elif 'R' in keys and 'N' in keys:       # a Raze / Keep dialog
            if not answers:                     # Combat_End's, before the capture
                k = 'n' if via_scroll else 'r'
            else:                               # the scroll's confirmation
                k = 'r'
            answers.append(k)
            mom.shot('dialog_%d' % len(answers), log)
            lv.D.key(k)
        elif 'R' in keys and via_scroll and not scroll_r:   # the results scroll of a sacked city
            cev = lv.glob('combat_end_value')
            t_scroll = tenth()                  # after the capture's collateral damage
            mom.shot('scroll', log)
            scroll_r = True
            lv.D.key('r')
        elif not mom.items_award(lv):
            lv.D.key('esc' if scroll_r or not via_scroll else 'enter')
        time.sleep(2)
    g1, f1 = lv.get('wizards', 0, 'gold'), lv.get('wizards', 0, 'fame')
    log.append('%s: raze option %d; gold %d -> %d, fame %d -> %d; combat_end_value on the scroll %s; answers %s; '
               'city 12 owner %d, population %d' % (tag, raze_opt, g0, g1, f0, f1, cev, answers,
                                                    lv.get('cities', city, 'owner'), lv.get('cities', city, 'population')))
    log.append('%s: city size %d, buildings\' tenth at the start %d, at the scroll %s' % (tag, size, t_start, t_scroll))
    return g0, g1, f0, f1, cev, raze_opt, answers, (size, t_start, t_scroll)


@case(314, 'UI', 'razing a captured city from the results scroll (R) pays the plunder twice and changes fame '
                 'differently from the Raze dialog',
      setup='neutral city 12 (size 2, 6 buildings) moved to (30,28) next to Dracon, two neutral Barbarian Spearmen '
            'in it; Dracon\'s stack attacks it, auto combat. Run A: the Raze dialog answered N (the city captured: '
            'fame + max(size - 2, 0), the plunder P paid = combat_end_value), then R on the results scroll and R in '
            'its confirmation (fame - size - 1, combat_end_value + the buildings\' tenth T paid). Run B: R at the '
            'dialog (fame - size, P + T). The human\'s gold and fame read before and after',
      bug='A gains P more gold than B (the plunder twice) less the tenth of the buildings the capture destroyed '
          '(Combat_End\'s collateral damage runs between the capture and the scroll; B\'s tenth is counted before '
          'it): A - B = P + T(scroll) - T(start); fame A max(size - 2, 0) - size - 1 = -3 against B -size',
      fixed='the same gold and fame by both ways')
def c314(log):
    a = _raze_run(True, log, 'A, raze from the scroll')
    b = _raze_run(False, log, 'B, raze at the dialog')
    ga, gb = a[1] - a[0], b[1] - b[0]
    fa, fb = a[3] - a[2], b[3] - b[2]
    obs = ('gold gained: A (scroll) %d, B (dialog) %d, difference %d; the plunder P on the scroll %s; fame change A %d, '
           'B %d; answers A %s, B %s; raze option %d' % (ga, gb, ga - gb, a[4], fa, fb, a[6], b[6], a[5]))
    if a[5] != 1:
        return dict(shown=None, observed=obs + ' - the Raze City option is off (no dialog)')
    if a[6] != ['n', 'r'] or b[6] != ['r'] or a[4] is None:
        return dict(shown=None, observed=obs + ' - the dialogs did not go as planned')
    size, t0, ts = a[7]
    want = a[4] + ts - t0 if ts is not None else None
    fame_bug = (fa == max(size - 2, 0) - size - 1, fb == -size)
    obs += ('; city size %d, buildings\' tenth T at the start %d (B) / at A\'s scroll %s: predicted A - B = P + T(scroll) '
            '- T(start) = %s; fame predicted A %d, B %d' % (size, t0, ts, want, max(size - 2, 0) - size - 1, -size))
    if want is None:
        return dict(shown=None, observed=obs + ' - the scroll\'s buildings were not read')
    if fa == fb and ga == gb:
        return dict(shown=False, observed=obs)
    # 10/26: judged on A alone - B's plunder is not A's P (the dialog's raze computes it again: r7d A 133 = 2 x 51 + 31
    # exactly while B got 65 = 34 + 31), so A - B = P was the wrong test; the bug is A = P (capture) + P (scroll) + T
    twice = ga == 2 * a[4] + ts
    obs += '; A = 2 x P + T(scroll) = %d: %s' % (2 * a[4] + ts, 'yes' if twice else 'no')
    return dict(shown=twice and a[4] > 0 and all(fame_bug), observed=obs)
