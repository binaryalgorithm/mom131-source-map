"""P4 B4 overland cases, batch 6 (N245): the auto-resolve battle (Combat_Strategic, COMBAUTO.C). The human's own
battles use it when Strategic Combat Only is on: the case turns the option on in the settings screen and leaves
with Esc, which keeps it in memory only (SETTINGS.C: OK writes MAGIC.SET, Esc does not)."""
import time

import mom
from regress import case

N = mom.N
NATURE, SORCERY, CHAOS, LIFE, DEATH = range(5)
SRC = 'src/wizards/COMBAUTO.C'
# Combat_Strategic's locals (work/bcc/lst/COMBAUTO.ASM): longs as (low word, high word)
WINNER = -2
ATT_ATTACK, DEF_ATTACK = (-30, -28), (-42, -40)
ATT_BOOK_DMG, ATT_BOOK_PCT, ATT_BOOK_HEAL = -76, -80, -84
ATT_SPEND, ATT_MAGIC_DMG, ATT_BUDGET = -88, -100, -72


def _config_strategic(lv):
    import struct
    fo = mom.resolve('GameConfig', 'strategic_combat')[0]
    return struct.unpack('<h', lv.g.ram(lv.g.ds + mom.SYM['game_config'][0] + fo, 2))[0]


def _strategic_on(lv, log=None):
    """Game menu (G) -> Settings (172..229, 171) -> the Strategic Combat Only toggle (192..310, 53..62) -> Esc, Esc
    (GAMEMENU.C / SETTINGS.C coordinates). -> game_config.strategic_combat after."""
    lv.D.key('g')
    time.sleep(2.5)
    lv.g.click(200, 177, settle=2.5)
    if _config_strategic(lv) == 0:
        lv.g.click(250, 57, settle=1.5)
    if log is not None:
        mom.shot('settings', log)
    lv.D.key('esc')
    time.sleep(2)
    for _ in range(3):
        if mom.on_map(lv):
            break
        lv.D.key('esc')
        time.sleep(2)
    return _config_strategic(lv)


def _long(d, h, lohi):
    lo, hi = d.word(h['SS'], (h['BP'] + lohi[0]) & 0xffff, False), d.word(h['SS'], (h['BP'] + lohi[1]) & 0xffff)
    return hi * 65536 + lo


def _auto_battle(s, att, defenders, def_xy, log, lines):
    """The human's unit att attacks defenders at def_xy with Strategic Combat Only on; breakpoints on the given
    COMBAUTO.C lines (label -> (text, nth)). -> (d, first hit) or (d, None); the caller stops d."""
    import dbg
    key = mom.setup_attack(s, att, defenders, def_xy)
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u != att:
            s.set('units', u, 'finished', 1)
    d = dbg.boot(s, log=log)
    on = _strategic_on(d.lv, log=log)
    log.append('strategic_combat = %d after the settings screen' % on)
    if on != 1:
        return d, None
    for label, (text, nth) in lines.items():
        d.want_line(SRC, text, nth=nth, fn='Combat_Strategic', label=label)
    d.follow = {d.ghidra('Combat_Strategic')[0]}   # Combat_Strategic's calls load other overlays: its own can be
    d.ovr_always = True                            # moved / evicted mid-call (README sec 4-5)
    d.arm()
    d.lv.D.key(key)
    k = [0]

    def poll():
        k[0] += 1
        if k[0] % 4 == 0:
            d.lv.D.key('y')                     # a treaty's 'still wish to attack?'
    return d, d.wait(90, poll)


@case(181, 'OV', 'auto-resolve leaves the winner unset when neither side\'s attack total is above 10',
      setup='Strategic Combat Only on (settings screen, Esc: memory only); the human\'s unit 1 made Settlers (type 44, '
            'melee 0) attacks unit 2, also made Settlers and given to wizard 1, at (30,28); breakpoints in '
            'Combat_Strategic on "if (att_attack > 10) {" (att_attack [bp-30], def_attack [bp-42], winner [bp-2] '
            'read) and on "dmg[DAMAGE_NORMAL] = 3;" after the kill loop (the battle units\' statuses read)',
      bug='both totals <= 10: neither branch assigns winner; the kill loop compares owners with the stack garbage in '
          '[bp-2]', fixed='a defined result (e.g. the defender holds)')
def c181(log):
    s = mom.SaveFile()
    for u, owner in ((1, 0), (2, 1)):
        for f, v in (('type', 44), ('owner', owner), ('hero_slot', -1), ('level', 0)):
            s.set('units', u, f, v)
    s.set('units', 1, 'x', 29)
    s.set('units', 1, 'y', 27)
    s.set('units', 1, 'plane', 0)
    d, h = _auto_battle(s, 1, [2], (30, 28, 0), log, {'decide': ('if (att_attack > 10) {', 0),
                                                       'killed': ('dmg[DAMAGE_NORMAL] = 3;', 0)})
    try:
        if h is None:
            return dict(shown=None, observed='Combat_Strategic\'s winner test was not reached')
        att, df, w = _long(d, h, ATT_ATTACK), _long(d, h, DEF_ATTACK), d.local(WINNER, h)
        d.drop('decide')
        d.go()
        h2 = d.wait(30, None)
        bus = mom.battle_units(d.lv, ('unit_index', 'owner', 'status')) if h2 is not None else []
        w2 = d.local(WINNER, h2) if h2 is not None else None
    finally:
        d.stop()
    obs = ('at the winner test: att_attack %d, def_attack %d, winner (uninitialised local) %d; after the kill loop '
           'winner %s, battle units %s' % (att, df, w, w2, bus))
    if not (att <= 10 and df <= 10):
        return dict(shown=None, observed=obs + ' - a side\'s total is above 10, the unset path not taken')
    return dict(shown=True, observed=obs)


def _strong_wizard(s, books, skill=60, reserve=5000):
    for r in range(5):
        s.set('wizards', 0, 'spellbooks[%d]' % r, books.get(r, 0))
    s.set('wizards', 0, 'nominal_casting_skill', skill)
    s.set('wizards', 0, 'skill_points', skill * (skill + 1))   # the skill is recomputed from the points
    s.set('wizards', 0, 'mana_reserve', reserve)


def _dracon_attack(s, log, lines):
    import cases_cb as CB
    att = CB._human_unit_at_dracon(s)
    s.set('units', 2, 'owner', 1)
    s.set('units', 13, 'owner', 1)
    return _auto_battle(s, att, [2, 13], (30, 28, 0), log, lines)


MAGIC_LINES = {'missile': ('att_magic_dmg += (int)(att_missile * att_magic_pct / 100);', 0),
               'melee': ('att_magic_dmg += (int)(att_missile * att_magic_pct / 100);', 1)}


@case(182, 'OV', 'auto-resolve wizard magic: each realm\'s books SET the damage / percentage / healing channels',
      setup='Strategic Combat Only on; the human given Nature 2, Chaos 3, Life 4 books, casting skill 60 (skill_points '
            '60 x 61), 5000 mana; '
            'Dracon\'s first unit attacks units 2 and 13 (wizard 1) at (30,28); breakpoint in Combat_Strategic on the '
            'first "att_magic_dmg += ..." line of a round: att_book_dmg [bp-76], att_book_pct [bp-80], att_book_heal '
            '[bp-84] read (already / 10, / 10, / 20)',
      bug='(30, 6, 8): Chaos\'s damage 300, Life\'s percentage 60 and healing 160 - the last realm in the order wins '
          'each channel', fixed='the realms add (40, 17, 10) or one realm is chosen')
def c182(log):
    s = mom.SaveFile()
    _strong_wizard(s, {NATURE: 2, CHAOS: 3, LIFE: 4})
    d, h = _dracon_attack(s, log, MAGIC_LINES)
    try:
        if h is None:
            return dict(shown=None, observed='no round of Combat_Strategic with the attacker\'s magic was reached')
        ch = tuple(d.local(o, h) for o in (ATT_BOOK_DMG, ATT_BOOK_PCT, ATT_BOOK_HEAL))
        budget = d.local(ATT_BUDGET, h)
        books = [d.lv.get('wizards', 0, 'spellbooks[%d]' % r) for r in range(5)]
    finally:
        d.stop()
    obs = ('stop at the %s round; books %s; channels (damage, percentage, healing) after the realm loop %s; '
           'budget %d' % (h['name'], books, ch, budget))
    return dict(shown=ch == (30, 6, 8), observed=obs)


@case(294, 'OV', 'auto-resolve battle magic: books x spend wraps at 16 bits (imul, then cdq drops DX)',
      setup='Strategic Combat Only on; the human given 13 Chaos books (damage channel 1300 / 10 = 130), casting skill '
            '900 (skill_points 900 x 901; spend 300, +3 with Archmage), 30000 mana; Dracon\'s first unit attacks '
            'units 2 and 13 (wizard 1) at (30,28); breakpoint on the first "att_magic_dmg += ..." line of a round: '
            'att_book_dmg [bp-76], att_spend [bp-88] '
            'and att_magic_dmg [bp-100] read',
      bug='att_magic_dmg = (int16)(130 x 300) / 5 = -5307 (the product 39000 wrapped)', fixed='7800')
def c294(log):
    s = mom.SaveFile()
    _strong_wizard(s, {CHAOS: 13}, skill=900, reserve=30000)
    d, h = _dracon_attack(s, log, MAGIC_LINES)
    try:
        if h is None:
            return dict(shown=None, observed='no round of Combat_Strategic with the attacker\'s magic was reached')
        dmg, spend, got = d.local(ATT_BOOK_DMG, h), d.local(ATT_SPEND, h), d.local(ATT_MAGIC_DMG, h)
    finally:
        d.stop()
    true = dmg * spend // 5
    wrapped = ((dmg * spend + 32768) % 65536 - 32768)
    wrapped = int(wrapped / 5)
    obs = ('stop at the %s round: att_book_dmg %d x att_spend %d = %d (> 32767: %s); att_magic_dmg read %d; the true '
           'value / 5 = %d, the 16-bit wrapped value / 5 = %d' % (h['name'], dmg, spend, dmg * spend,
                                                                    dmg * spend > 32767, got, true, wrapped))
    if dmg * spend <= 32767:
        return dict(shown=None, observed=obs + ' - the product does not pass 32767')
    return dict(shown=got == wrapped and got != true, observed=obs)


def _prefab_flags(lv):
    a = lv.g.far(mom.SYM['p_items_in_game'][0])
    return lv.g.ram(a, N('NUM_PREFAB_ITEMS'))


@case(79, 'OV', 'a destroyed prefab item never returns: its p_items_in_game mark is never cleared',
      setup='lair 0 at (30,28) without guards, one treasure item (item_count 1, item value 0 = PREFAB_ANY), the '
            'human\'s vault emptied; unit 1 walks in, the item is dropped into the vault (Items award); the prefab '
            'marks (p_items_in_game, 250 bytes) read before and after; then Armies (A) -> Items (I), the item picked '
            'up from the vault and dropped on the anvil, Y; the item record and its mark read',
      bug='the record is freed (cost -1) and the prefab stays marked 1: Item_Create_Prefab never offers it again',
      fixed='the mark cleared with the record')
def c79(log):
    import cases_ov5 as O5
    s = mom.SaveFile()
    key = O5._walker_lair(s)
    s.set('lairs', 0, 'item_count', 1)
    s.set('lairs', 0, 'item_values[0]', N('PREFAB_ANY'))
    for k in range(4):
        s.set('wizards', 0, 'vault_items[%d]' % k, -1)
    lv = mom.boot(s)
    f0 = _prefab_flags(lv)
    lv.D.key(key)
    pressed = O5._answer_by_hotkey(lv, log=log, timeout=90)
    f1 = _prefab_flags(lv)
    new = [i for i in range(len(f0)) if f0[i] == 0 and f1[i] == 1]
    vault = [lv.get('wizards', 0, 'vault_items[%d]' % k) for k in range(4)]
    obs = 'boxes %s; prefab marks set by the lair %s; vault %s' % (pressed, new, vault)
    held = [(k, v) for k, v in enumerate(vault) if v >= 0]
    if len(new) != 1 or len(held) != 1:
        return dict(shown=None, observed=obs + ' - not one new prefab item in the vault')
    slot, item = held[0]
    name = bytes(lv.get('items', item, 'name')).split(b'\0')[0].decode('latin-1')
    lv.D.key('a')
    time.sleep(3)
    lv.D.key('i')
    time.sleep(3)
    lv.g.click(79 + 21 * slot, 181, settle=1.5)     # the vault slot: the item is picked up
    lv.g.click(45, 174, settle=2.0)                 # the anvil
    mom.shot('anvil', log)
    lv.D.key('y')
    time.sleep(2)
    mom.shot('after', log)
    cost = lv.get('items', item, 'cost')
    vault2 = [lv.get('wizards', 0, 'vault_items[%d]' % k) for k in range(4)]
    mark = _prefab_flags(lv)[new[0]]
    for _ in range(4):
        if mom.on_map(lv):
            break
        lv.D.key('esc')
        time.sleep(2)
    obs += ('; item %d "%s" (prefab %d) destroyed at the anvil: its record cost now %d, vault %s, prefab mark %d'
            % (item, name, new[0], cost, vault2, mark))
    if cost > 0:
        return dict(shown=None, observed=obs + ' - the item was not destroyed')
    return dict(shown=mark == 1, observed=obs)


TOWER3 = (31, 27)                               # tower 3 of the base save, two squares east of Dracon


def _tower_path(plane, log, tag, away=False):
    """A neutral unit (unit 2, owner 5) standing in tower 3 at (31,27) registered on `plane` (in_tower 1); Dracon's
    first unit, alone selected, is sent by a click to (31,28) (the obstacle loop marks every unit square on the
    plane, not only the path's); Path_Find's call (player 0) run to its return: the
    path and the cost map square of the tower (p_path_work->cost[27][31]) read. -> (args, path, tower cost)."""
    import dbg
    import cases_cb as CB
    import cases_ov5 as O5
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    mom.setup_attack(s, att, [], (30, 28, 0))
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u != att:
            s.set('units', u, 'finished', 1)
    spot = (30, 26) if away else TOWER3
    for f, v in (('owner', 5), ('x', spot[0]), ('y', spot[1]), ('plane', plane), ('in_tower', 0 if away else 1)):
        s.set('units', 2, f, v)
    s.set('lairs', 3, 'active', 0)              # the tower's lair record: cleared (an active one is an obstacle
    #                                             of its own, and units stand only in a cleared tower)
    d = dbg.boot(s, log=log)
    try:
        px = O5._square_px(d.lv, 31, 28)
        if px is None:
            return None, None, None
        # right after Path_Apply_Obstacles, before the search (which rewrites the cost map: 255 everywhere at the
        # return, 10/24); Path_Find's arguments at [bp+6+2k], player = k 18
        d.want_line('src/wizards/PATHFIND.C', 'if (p_path_work->cost[y2][x2] == COST_IMPASSABLE)', fn='Path_Find',
                    label='obstacles', when=lambda h: d.word(h['SS'], h['BP'] + 6 + 36) == 0)
        d.lv.g.click(px[0], px[1], settle=0.2)
        h = d.wait(60, None)
        if h is None:
            return None, None, None
        args = [d.word(h['SS'], h['BP'] + 6 + 2 * k) for k in range(19)]
        import struct
        woff, wseg = struct.unpack('<HH', d.lv.g.ram(d.lv.g.ds + mom.SYM['p_path_work'][0], 4))
        row = list(d.dump(wseg, woff + TOWER3[1] * 60 + 28, 6))    # in the EMS page frame: read by the debugger
        cost = row[TOWER3[0] - 28]
        log.append('%s: Path_Find%s: cost[27][31] %d (row 27, x 28..33: %s)' % (tag, tuple(args[6:11]), cost, row))
        return args, row, cost
    finally:
        d.stop()


@case(256, 'OV', 'pathfinding ignores an enemy standing in a Tower of Wizardry registered on the other plane',
      setup='a neutral unit (unit 2, owner 5) in tower 3 at (31,27) (its lair record 3 cleared: active 0), in_tower 1, '
            'plane 1 (Myrror); Dracon\'s first unit, alone, sent by a map click to (31,28) on Arcanus; breakpoint in '
            'Path_Find (player 0) on the line after Path_Apply_Obstacles: p_path_work->cost[27][31] read. Control: the '
            'same unit registered on plane 0 (and, if needed, no unit there)',
      bug='with plane 1 the tower square keeps its terrain cost (Path_Apply_Obstacles tests Unit.plane only); with '
          'plane 0 it is 255', fixed='255 in both: a unit in a tower is on both planes')
def c256(log):
    a1, p1, c1 = _tower_path(1, log, 'Myrror-registered')
    a0, p0, c0 = _tower_path(0, log, 'control, Arcanus-registered')
    obs = ('tower square cost after the obstacles with the neutral unit on plane 1: %s (row 27, x 28..33: %s); on '
           'plane 0: %s (%s)' % (c1, p1, c0, p0))
    if c1 is None or c0 is None:
        return dict(shown=None, observed=obs + ' - a Path_Find call of the human was not caught')
    if c0 != 255:
        return dict(shown=None, observed=obs + ' - the control did not mark the square')
    if c1 == 255:                               # marked: by the unit, or is the square itself impassable?
        _, _, cn = _tower_path(1, log, 'no unit (the unit moved off to (30,26))', away=True)
        obs += '; with no unit there: %s' % cn
        if cn == 255:
            return dict(shown=None, observed=obs + ' - the tower square is impassable by itself')
    return dict(shown=c1 != 255, observed=obs)


def _raid(garrison_types, log, tag):
    """Every unit standing in a neutral city retyped (cycling garrison_types), the neutral raid timer
    (wizards[5].casting_cost_initial) 29 so the turn change raids; breakpoint in AI_Neutral_City_Raiders on the
    line after the raider loop: city [bp-30], garrison [bp-20], its units [bp-56..], raiders [bp-22], created
    [bp-36]. -> dict or None."""
    import dbg
    s = mom.SaveFile()
    s.set('wizards', 5, 'casting_cost_initial', 29)
    sites = {tuple(s.get('cities', c, f) for f in ('x', 'y', 'plane')) for c in range(s.get('num_cities'))
             if s.get('cities', c, 'owner') == 5}
    k = 0
    for u in range(s.get('num_units')):
        if tuple(s.get('units', u, f) for f in ('x', 'y', 'plane')) in sites:
            s.set('units', u, 'type', garrison_types[k % len(garrison_types)])
            s.set('units', u, 'hero_slot', -1)
            k += 1
    d = dbg.boot(s, log=log)
    try:
        d.want_line('src/wizards/AINEUTRL.C', 'for (j = 0; j < created / 3; j++)', fn='AI_Neutral_City_Raiders',
                    label='raided')
        d.follow = {d.ghidra('AI_Neutral_City_Raiders')[0]}
        d.want('AI_Neutral_Stack_Orders', label='near')   # same module, called just before: from there on every
        poll = d.end_turn_poll()                           # overlay load re-places the breakpoint (the control run
        while True:                                        # missed it once the overlay moved, 10/24)
            h = d.wait(300, poll)
            if h is None or h['name'] != 'near':
                break
            d.drop('near')
            d.ovr_always = True
            d.arm()
            d.go()
        if h is None:
            return None
        loc = lambda o: d.local(o, h)
        g = loc(-20)
        units = [loc(-56 + 2 * i) for i in range(g)]
        r = dict(city=loc(-30), garrison=g, types=[d.lv.get('units', u, 'type') for u in units], raiders=loc(-22),
                 created=loc(-36))
        log.append('%s: %s' % (tag, r))
        return r
    finally:
        d.stop()


@case(283, 'OV', 'Barbarian Spearmen (39) and Swordsmen (40) never go raiding: the raider type test is > 40',
      setup='every unit standing in a neutral city retyped to Barbarian Spearmen / Swordsmen (39 / 40, alternating); '
            'the neutral raid timer (wizards[5].casting_cost_initial) 29, so the turn change raids; breakpoint in '
            'AI_Neutral_City_Raiders after the raider loop: the chosen city, its garrison types, raiders and created '
            'read. Control: the same with Barbarian Bowmen (41)',
      bug='raiders >= 1 picked from a 39 / 40 garrison, created 0; the Bowmen control creates its raiders',
      fixed='Spearmen / Swordsmen raid too (the bound is 38: ships and catapult)')
def c283(log):
    r = _raid([39, 40], log, 'spearmen / swordsmen')
    c = _raid([41], log, 'control, bowmen')
    obs = 'Spearmen / Swordsmen garrisons: %s; Bowmen control: %s' % (r, c)
    if r is None or c is None:
        return dict(shown=None, observed=obs + ' - a raid was not reached')
    if c['created'] != c['raiders'] or c['created'] < 1:
        return dict(shown=None, observed=obs + ' - the control did not raid')
    if not set(r['types']) <= {39, 40} or r['raiders'] < 1:
        return dict(shown=None, observed=obs + ' - the garrison is not spearmen / swordsmen only')
    return dict(shown=r['created'] == 0, observed=obs)


@case(38, 'OV', 'Disjunction: the target owner\'s Archmage multiplies the caster\'s success chance',
      setup='wizard 2 has Evil Omens cast (globals.evil_omens = 3, player + 1) and the Archmage retort (no Mastery); '
            'the human has finished Disjunction (210; casting_cost_left 0, casting_cost_initial 300) - cast at the '
            'start of the next turn: the turn is ended, wizard 2\'s first row is clicked in the Disjunction box '
            '(CASTUI.C: rows at x 124..244, y 16 + 46 per wizard); Spell_Dispel_Resistance (strength, owner, realm) '
            'read at its entry and run to its return',
      bug='returns 2 x strength for owner 2: the chance compared with Random(250) doubles (resistance in the '
          'numerator)', fixed='the owner\'s Archmage lowers the chance (in the denominator, as Dispel_Square)')
def c38(log):
    import dbg
    s = mom.SaveFile()
    s.set('wizards', 2, 'globals.evil_omens', 3)
    s.set('wizards', 2, 'retorts[%d]' % N('RETORT_ARCHMAGE'), 1)
    for r in ('RETORT_NATURE_MASTERY', 'RETORT_SORCERY_MASTERY', 'RETORT_CHAOS_MASTERY'):
        s.set('wizards', 2, 'retorts[%d]' % N(r), 0)
    for f, v in (('casting_spell', N('SPELL_DISJUNCTION')), ('casting_cost_left', 0), ('casting_cost_initial', 300),
                 ('mana_reserve', 5000)):
        s.set('wizards', 0, f, v)
    d = dbg.boot(s, log=log)
    try:
        mom.end_turn(d.lv, log=log)
        time.sleep(3)
        mom.shot('disjunction_box', log)
        d.want('Spell_Dispel_Resistance')
        d.lv.g.click(180, 68, settle=0.2)
        h = d.wait(30, None)
        if h is None:
            return dict(shown=None, observed='Spell_Dispel_Resistance was not called after the row click')
        args = [d.arg(k, h) for k in range(3)]
        ax = d.ret_ax()
    finally:
        d.stop()
    obs = ('Spell_Dispel_Resistance(strength %d, owner %d, realm %d) = %d (250 x 300 / (300 + Evil Omens\' cost) '
           'expected as the strength)' % (args[0], args[1], args[2], ax))
    if args[1] != 2:
        return dict(shown=None, observed=obs + ' - not the call for wizard 2\'s global')
    return dict(shown=ax == 2 * args[0], observed=obs)


def _globals(src, w):
    return [src.get('wizards', w, 'globals.%s' % f) for f in GLOBAL_FIELDS]


GLOBAL_FIELDS = ('eternal_night evil_omens zombie_mastery aura_of_majesty wind_mastery suppress_magic time_stop '
                 'nature_awareness nature_s_wrath herb_mastery chaos_surge doom_mastery great_wasting meteor_storm '
                 'armageddon tranquility life_force crusade just_cause holy_arms planar_seal charm_of_life '
                 'detect_magic awareness').split()


@case(473, 'OV', 'AI Spell Binding takes the GlobalEnchants SLOT of its pick as a spell id',
      setup='every wizard\'s globals cleared except the human\'s Suppress Magic (slot 5, value 1 = player + 1); AI '
            'wizard 1 has finished Spell Binding (76; cost left 0, casting_cost_initial 30000 so the roll passes, mana '
            '5000) - cast in the turn change; every wizard\'s 24 global bytes read before and after, and '
            'p_spell_data[5] (the spell whose id is the slot)',
      bug='the binding moves globals[p_spell_data[5].effect_param] (not Suppress Magic): the human keeps Suppress '
          'Magic', fixed='Suppress Magic moves to wizard 1')
def c473(log):
    import cases_dbg1 as D1
    s = mom.SaveFile()
    for w in range(5):
        for f in GLOBAL_FIELDS:
            s.set('wizards', w, 'globals.%s' % f, 0)
    s.set('wizards', 0, 'globals.suppress_magic', 1)
    D1._ai_casts(s, 1, N('SPELL_SPELL_BINDING'))
    s.set('wizards', 1, 'casting_cost_initial', 30000)
    lv = mom.boot(s)
    before = {w: _globals(lv, w) for w in range(5)}
    sp = (lv.get('spells', 5, 'effect_param'), lv.get('spells', 5, 'base_cost'), lv.get('spells', 5, 'realm'),
          lv.get('spells', 5, 'effect_type'))
    wsize, goff = mom.L.size('Wizard'), mom.resolve('Wizard', 'globals')[0]
    far = [lv.g.ds + mom.SYM['wizards'][0] + w * wsize + goff + sp[0] for w in (0, 1)]   # globals[effect_param]
    far0 = [lv.g.ram(a, 1)[0] for a in far]
    mom.end_turn(lv, log=log)
    after = {w: _globals(lv, w) for w in range(5)}
    far1 = [lv.g.ram(a, 1)[0] for a in far]
    cast = lv.get('wizards', 1, 'casting_spell')
    changed = {w: [(GLOBAL_FIELDS[i], before[w][i], after[w][i]) for i in range(24) if before[w][i] != after[w][i]]
               for w in range(5)}
    obs = ('p_spell_data[5]: effect_param %d, base cost %d, realm %d, effect type %d; globals changed in the turn: %s; '
           'human Suppress Magic %d -> %d, wizard 1\'s %d; wizard 1 casting now %d; the bytes at globals + %d (in '
           'wizard %d / %d\'s records, offset %d): human\'s %d -> %d, wizard 1\'s %d -> %d'
           % (sp + (changed, before[0][5], after[0][5], after[1][5], cast, sp[0], (goff + sp[0]) // wsize,
                    (wsize + goff + sp[0]) // wsize, (goff + sp[0]) % wsize, far0[0], far1[0], far0[1], far1[1])))
    if cast == N('SPELL_SPELL_BINDING'):
        return dict(shown=None, observed=obs + ' - Spell Binding was not cast')
    if far1[1] != 2 or far1[0] != 0:            # the binding ran: target byte cleared, caster's set to player + 1
        return dict(shown=None, observed=obs + ' - no binding seen at globals[effect_param] (the roll failed?)')
    return dict(shown=after[0][5] == 1 and after[1][5] == 0, observed=obs)


@case(307, 'OV', 'AI Plane Shift away from the human\'s view releases a scratch-heap mark it never set',
      setup='AI wizard 1 (on Myrror, out of the human\'s sight) has finished Plane Shift (132; cost left 0) - cast in '
            'the turn change; breakpoints in Plane_Shift_Cast on "Heap_Mark(p_scratch);" (the human / visible '
            'branch) and on the final "Heap_Release(p_scratch);": the caster ([bp+6]) and the scratch block\'s used '
            '(+0x0a) and mark (+0x0e) words read at the release',
      bug='the release is reached with no Heap_Mark in this call: used <- whatever mark earlier code left',
      fixed='a mark set on every path (W663FIX: the AI always takes the setup branch)')
def c307(log):
    import dbg
    import cases_dbg1 as D1
    s = mom.SaveFile()
    D1._ai_casts(s, 1, N('SPELL_PLANE_SHIFT'))
    d = dbg.boot(s, log=log)
    try:
        src = 'src/wizards/OVERSPEL.C'
        d.want_line(src, 'Heap_Mark(p_scratch);', fn='Plane_Shift_Cast', label='mark')
        d.want_line(src, 'Heap_Release(p_scratch);', fn='Plane_Shift_Cast', label='release')
        d.want('Plane_Shift_Cast', label='entry')
        d.follow = {d.ghidra('Plane_Shift_Cast')[0]}      # OVERSPEL is loaded early in the turn change and moved
        marks = []                                         # before the AI's cast (10/24: stale breakpoints)
        h = None
        poll = d.end_turn_poll()
        while True:
            h = d.wait(240, poll)
            if h is None or h['name'] == 'release':
                break
            if h['name'] == 'entry':                       # from here every overlay load re-places the lines:
                d.drop('entry')                            # the function calls other overlays first
                d.ovr_always = True
                d.arm()
            else:
                marks.append(d.word(h['SS'], h['BP'] + 6))
            d.go()
        if h is None:
            return dict(shown=None, observed='Plane_Shift_Cast\'s release was not reached (marks seen %s)' % marks)
        player = d.word(h['SS'], h['BP'] + 6)
        seg = d.lv.glob('p_scratch')
        used, mark = d.word(seg, 0x0a, False), d.word(seg, 0x0e, False)
    finally:
        d.stop()
    obs = ('Plane_Shift_Cast(player %d): Heap_Mark line reached %d time(s) before the release; at the release the '
           'scratch block (seg %04x) has used %d, mark %d (used <- %d)' % (player, len(marks), seg, used, mark, mark))
    if player == 0:
        return dict(shown=None, observed=obs + ' - the human\'s cast')
    return dict(shown=not marks, observed=obs)


SPECIALS = {1: 'Warlord', 8: 'Channeler', 6: 'Divine Power', 13: 'Famous', 5: 'Infernal Power'}


@case(76, 'OV', 'treasure retorts Divine / Infernal Power and Famous skip the Life / Death check and the +10 fame',
      setup='the human: Nature 11, Life 1, Death 1 books (13 > 12: the treasure gives a retort, LAIR.C), retorts '
            'cleared but Alchemy (count 1); lair 0 at (30,28) without guards, special 6 (two books or retorts); unit '
            '1 walks in, the boxes answered; retorts, fame and books read before and after. Up to 6 tries (the pick is '
            'random): a try counts when Divine Power, Infernal Power or Famous is awarded',
      bug='Divine Power granted with Death books / Infernal Power with Life books / Famous with no +10 fame (the '
          'special-retort branch has none of those checks)', fixed='refused / +10 fame as for the other retorts')
def c76(log):
    import cases_ov5 as O5
    tries = []
    for attempt in range(6):
        s = mom.SaveFile()
        for r, n in enumerate((11, 0, 0, 1, 1)):
            s.set('wizards', 0, 'spellbooks[%d]' % r, n)
        for r in range(18):
            s.set('wizards', 0, 'retorts[%d]' % r, 1 if r == N('RETORT_ALCHEMY') else 0)
        key = O5._walker_lair(s, special=6)
        lv = mom.boot(s)
        r0 = [lv.get('wizards', 0, 'retorts[%d]' % r) for r in range(18)]
        f0 = lv.get('wizards', 0, 'fame')
        lv.D.key(key)
        O5._answer_by_hotkey(lv, log=log, timeout=60)
        r1 = [lv.get('wizards', 0, 'retorts[%d]' % r) for r in range(18)]
        f1 = lv.get('wizards', 0, 'fame')
        books = [lv.get('wizards', 0, 'spellbooks[%d]' % r) for r in range(5)]
        new = [r for r in range(18) if r1[r] and not r0[r]]
        tries.append((new, f0, f1, books))
        log.append('try %d: new retorts %s, fame %d -> %d, books %s' % (attempt + 1, new, f0, f1, books))
        if any(r in (N('RETORT_DIVINE_POWER'), N('RETORT_INFERNAL_POWER'), N('RETORT_FAMOUS')) for r in new):
            break                                   # Warlord / Channeler have no check to skip: try again
    new, f0, f1, books = tries[-1]
    obs = ('tries (new retorts, fame before, after, books): %s; specials %s'
           % (tries, {r: SPECIALS[r] for r in new if r in SPECIALS}))
    sp = [r for r in new if r in (N('RETORT_DIVINE_POWER'), N('RETORT_INFERNAL_POWER'), N('RETORT_FAMOUS'))]
    if not sp:
        return dict(shown=None, observed=obs + ' - no Divine / Infernal Power or Famous in 6 tries')
    r = sp[0]
    shown = ((r == N('RETORT_DIVINE_POWER') and books[4] > 0) or (r == N('RETORT_INFERNAL_POWER') and books[3] > 0)
             or (r == N('RETORT_FAMOUS') and f1 - f0 < 10))
    return dict(shown=shown, observed=obs)


@case(158, 'OV', 'the Spell of Mastery war roll tests the message detail: destroying unit #10 rolls a war declaration',
      setup='unit 10 given to wizard 1, made Settlers with 1 hit point left and moved alone to (30,28) on Arcanus; the human and wizard 1 met, no treaty '
            '(not at war: a war zeroes the amount and skips the roll); the human has finished Fire Storm (107: '
            'overland damage, SPELL_FX_DAMAGE) and targets that square at the next turn start; tracepoints on '
            'Diplo_Relation_Change(amount, actor, observer, msg, city, msg_arg) entries and on Diplo_Declare_War: the '
            'arguments (with amount != 0 the war roll "msg_arg == 10 && Random(100) < 11" is evaluated from them '
            'alone; 10/24: a breakpoint on that line was never hit) - and on Random entries called from '
            'Diplo_Relation_Change: past its guards (contact, amount != 0, not final war, the at-war offense zeroing) '
            'the only Random there is the war roll\'s, made only when msg_arg == 10 (the && short-circuits; the '
            'diplo_test_mode 3 branch\'s Random is told apart by diplo_test_mode)',
      bug='a call (-3, 0, 1, msg 8 = unprovoked destruction, 0, msg_arg 10 = the unit index): the 10% war roll is '
          'made for unit #10', fixed='the roll only for the Spell of Mastery message (10)')
def c158(log):
    import dbg
    import cases_dip as C
    s = mom.SaveFile()
    C._met(s)
    for a, b in ((0, 1), (1, 0)):
        s.set('wizards', a, 'diplomacy_status[%d]' % b, 0)
    # made Settlers (44, one figure of 10 hits) with 9 damage: the Fire Storm destroys it (10/24: a full unit lived)
    for f, v in (('owner', 1), ('x', 30), ('y', 28), ('plane', 0), ('type', 44), ('hero_slot', -1), ('damage', 9),
                 ('enchantments', 0)):
        s.set('units', 10, f, v)
    for f, v in (('casting_spell', N('SPELL_FIRE_STORM')), ('casting_cost_left', 0), ('casting_cost_initial', 200),
                 ('mana_reserve', 5000)):
        s.set('wizards', 0, f, v)
    d = dbg.boot(s, log=log)
    try:
        mom.end_turn(d.lv, log=log)
        d.want('Diplo_Relation_Change', trace=lambda h: tuple(d.arg(k, h) for k in range(6)))
        d.want('Diplo_Declare_War', trace=lambda h: (d.arg(0, h), d.arg(1, h)))

        def rcaller(h):
            return str(d.where(d.word(h['SS'], h['SP'] + 2, False), d.word(h['SS'], h['SP'], False)))
        d.want('Random', when=lambda h: 'Diplo_Relation_Change' in rcaller(h), trace=lambda h: d.arg(0, h))
        mom.target_square(d.lv, 30, 28, log=log)
        t0 = time.time()
        while time.time() - t0 < 30:
            d.wait(5, None)
        calls = list(d.traces.get('Diplo_Relation_Change', []))
        war = list(d.traces.get('Diplo_Declare_War', []))
        rolls = list(d.traces.get('Random', []))
        test_mode = d.lv.glob('diplo_test_mode')
        st = (d.lv.get('wizards', 1, 'diplomacy_status[0]'), d.lv.get('units', 10, 'owner'))
    finally:
        d.stop()
    msg = N('DIPLO_MSG_UNPROVOKED_DESTRUCTION')
    hit = [c for c in calls if c[3] == msg and c[5] == 10 and c[1] == 0 and c[0] != 0]
    obs = ('Diplo_Relation_Change(amount, actor, observer, msg, city, msg_arg) calls %s; Diplo_Declare_War calls %s; '
           'Random calls from Diplo_Relation_Change (argument) %s, diplo_test_mode %d; wizard 1\'s status to the human '
           '%d (war %d), unit 10\'s owner after %d'
           % (calls[:8], war, rolls, test_mode, st[0], N('TREATY_WAR'), st[1]))
    if not calls:
        return dict(shown=None, observed=obs + ' - no relation change (the spell did not land)')
    if not hit:
        return dict(shown=None, observed=obs + ' - no destruction message (amount != 0) for unit 10')
    if test_mode == 3:
        return dict(shown=None, observed=obs + ' - diplo_test_mode 3: its own Random branch')
    if 100 not in rolls:
        return dict(shown=None, observed=obs + ' - the call returned before the war roll (no Random(100) from it)')
    return dict(shown=True, observed=obs + '; the war roll Random(100) ran for msg_arg 10 = unit #10 (it declares '
                                          'war 1 time in 9)')


def _held_items(lv):
    held = set()
    for w in range(6):
        held |= {lv.get('wizards', w, 'vault_items[%d]' % k) for k in range(4)}
        for sl in range(6):
            held |= {lv.get('wizards', w, 'hired_heroes[%d].item_index[%d]' % (sl, j)) for j in range(3)}
    return held


def _gift(player, log, tag):
    """Item 1's record copied into free record 2; the events record holds a NEW Gift event for `player` with item 2
    (as Event_Roll leaves it: the item made at once, EVENTGEN.C); the turn is ended, the award screen answered."""
    s = mom.SaveFile()
    o, _, size, _ = mom.TABLES['items']
    s.b[o + 2 * size:o + 3 * size] = s.b[o + size:o + 2 * size]
    for f, v in (('gift', N('EVENT_STATE_NEW')), ('gift_player', player), ('gift_item', 2)):
        s.set('events', 0, f, v)
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    for _ in range(5):                              # the Items award (the human's gift) or messages
        if mom.on_map(lv):
            break
        if not mom.items_award(lv):
            lv.D.key('enter')
        time.sleep(2)
    r = dict(gift=lv.get('events', 0, 'gift'), cost=lv.get('items', 2, 'cost'), held=2 in _held_items(lv))
    log.append('%s: %s' % (tag, r))
    return r


@case(177, 'OV', 'the Gift event gives an AI wizard nothing and leaks the item record',
      setup='item 1 copied into free record 2; the events record holds a NEW Gift event (gift 1) for wizard 1 with '
            'item 2 - the state Event_Roll leaves (the item made at once); the turn is ended; after: the event state, '
            'item 2\'s cost and whether any hero / vault of any wizard holds it. Control: the same gift to the human',
      bug='for wizard 1: the event ends, item 2 stays allocated (cost > 0) and nobody holds it; the human\'s is in his '
          'vault', fixed='the AI wizard receives the item (or it is freed)')
def c177(log):
    a = _gift(1, log, 'wizard 1')
    h = _gift(0, log, 'control, the human')
    obs = 'gift to wizard 1: %s; to the human: %s' % (a, h)
    if a['gift'] != 0 or h['gift'] != 0:
        return dict(shown=None, observed=obs + ' - the event was not processed')
    if not h['held']:
        return dict(shown=None, observed=obs + ' - the human\'s gift was not placed either')
    return dict(shown=a['cost'] > 0 and not a['held'], observed=obs)


def _obstacle_costs(s, mover, dest, cells, log, tag):
    """Dracon's unit `mover` alone (every other human unit finished) is sent by a click to dest; breakpoint in
    Path_Find (player 0) right after Path_Apply_Obstacles: p_path_work->cost at `cells` read through the debugger
    (EMS page frame). -> {cell: cost} or None."""
    import struct
    import dbg
    import cases_ov5 as O5
    mom.setup_attack(s, mover, [], (30, 28, 0))
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u != mover:
            s.set('units', u, 'finished', 1)
    d = dbg.boot(s, log=log)
    try:
        px = O5._square_px(d.lv, *dest)
        if px is None:
            return None
        d.want_line('src/wizards/PATHFIND.C', 'if (p_path_work->cost[y2][x2] == COST_IMPASSABLE)', fn='Path_Find',
                    label='obstacles', when=lambda h: d.word(h['SS'], h['BP'] + 6 + 36) == 0)
        d.lv.g.click(px[0], px[1], settle=0.2)
        h = d.wait(60, None)
        if h is None:
            return None
        args = [d.word(h['SS'], h['BP'] + 6 + 2 * k) for k in range(19)]
        woff, wseg = struct.unpack('<HH', d.lv.g.ram(d.lv.g.ds + mom.SYM['p_path_work'][0], 4))
        out = {c: d.dump(wseg, woff + c[1] * 60 + c[0], 1)[0] for c in cells}
        log.append('%s: Path_Find%s: costs %s' % (tag, tuple(args[6:11]), out))
        return out
    finally:
        d.stop()


SEA_SHIP, SEA_OTHER = (29, 26), (31, 26)        # shore water north of Dracon; water east of it


@case(301, 'OV', 'moving toward a sea square holding your own ship makes every square with your units cost 2',
      setup='human unit 161 made a Trireme (35, capacity 2) on the water at (29,26), unit 173 made a Trireme on the '
            'water at (31,26); Dracon\'s hero 19 alone is sent by a click to (29,26) (its own ship); breakpoint in '
            'Path_Find after Path_Apply_Obstacles: the cost of (31,26) read. Control: the same with the click on the '
            'land square (30,28)',
      bug='(31,26) - water, impassable for a walker - costs 2 toward the ship (the transport test reads the '
          'DESTINATION\'s landmass and ships); 255 toward the land square', fixed='255 in both')
def c301(log):
    res = {}
    for tag, dest in (('to the ship', SEA_SHIP), ('control, to land', (30, 28))):
        s = mom.SaveFile()
        for u, xy in ((161, SEA_SHIP), (173, SEA_OTHER)):
            for f, v in (('x', xy[0]), ('y', xy[1]), ('plane', 0), ('type', 35), ('hero_slot', -1)):
                s.set('units', u, f, v)
        res[tag] = _obstacle_costs(s, 19, dest, [SEA_OTHER, SEA_SHIP], log, tag)
    obs = 'cost of (31,26) / (29,26) after the obstacles: %s' % res
    if any(v is None for v in res.values()):
        return dict(shown=None, observed=obs + ' - a Path_Find call was not caught')
    if res['control, to land'][SEA_OTHER] != 255:
        return dict(shown=None, observed=obs + ' - the water square is not impassable in the control')
    return dict(shown=res['to the ship'][SEA_OTHER] == 2, observed=obs)


DRACON_RING = [(28, 26), (30, 26), (28, 27), (30, 27), (28, 28), (29, 28), (30, 28)]   # the land squares round
#                                                                                        Dracon; (29,26) is water


def _full_circle_summon(utype, spell, log, tag):
    """Dracon (the summoning circle) holds 9 human units, all retyped to utype (equal upkeep: any one may be the
    unit Stack_Limit_Enforce moves out); active lair records 90..96 on its seven land neighbours (Unit_Bump_To_Adjacent
    skips lair squares), so the only square left is the water at (29,26); the human has finished `spell` (summons
    utype) - cast at the next turn start. -> units of utype at Dracon / on (29,26) / in all, before and after."""
    s = mom.SaveFile()
    here = [u for u in range(s.get('num_units')) if s.get('units', u, 'owner') == 0 and
            (s.get('units', u, 'x'), s.get('units', u, 'y'), s.get('units', u, 'plane')) == (29, 27, 0)]
    away = [u for u in range(s.get('num_units')) if s.get('units', u, 'owner') == 0 and u not in here]
    for u in here + away[:9 - len(here)]:
        for f, v in (('x', 29), ('y', 27), ('plane', 0), ('type', utype), ('hero_slot', -1), ('level', 0),
                     ('experience', 0), ('order', N('ORDER_PATROL')), ('finished', 1)):
            s.set('units', u, f, v)
    for k, (x, y) in enumerate(DRACON_RING):
        for f, v in (('x', x), ('y', y), ('plane', 0), ('active', 1), ('guard1_count', 0), ('guard2_count', 0)):
            s.set('lairs', 90 + k, f, v)
    for f, v in (('casting_spell', spell), ('casting_cost_left', 0), ('casting_cost_initial', 100),
                 ('mana_reserve', 5000)):
        s.set('wizards', 0, f, v)

    def count(lv):
        units = [u for u in range(lv.glob('num_units')) if lv.get('units', u, 'owner') == 0 and
                 lv.get('units', u, 'type') == utype]
        at = lambda xy: sum(1 for u in units if (lv.get('units', u, 'x'), lv.get('units', u, 'y'),
                                                 lv.get('units', u, 'plane')) == xy + (0,))
        return at((29, 27)), at((29, 26)), len(units)
    lv = mom.boot(s)
    c0 = count(lv)
    mom.end_turn(lv, log=log)
    for _ in range(6):                              # the summon at the turn start and its messages
        time.sleep(2)
        if mom.on_map(lv) and lv.get('wizards', 0, 'casting_spell') != spell:
            break
        lv.D.key('enter')
    time.sleep(2)
    c1 = count(lv)
    log.append('%s: units of type %d (Dracon, the water (29,26), all) %s -> %s; casting now %d'
               % (tag, utype, c0, c1, lv.get('wizards', 0, 'casting_spell')))
    return c0, c1, lv.get('wizards', 0, 'casting_spell')


@case(310, 'OV', 'the placement code does not let a Non-Corporeal unit onto water: no room next to a full circle',
      setup='Dracon (the summoning circle) holds 9 human units made Magic Spirits (154: Non-Corporeal, not flying); '
            'active lair records on its seven land neighbours, so the one square left is the water at (29,26); the '
            'human has finished Magic Spirit (summons one more): at the turn start the 10th unit makes '
            'Stack_Limit_Enforce move one out (Unit_Bump_To_Adjacent -> Tile_Room_For_Unit). Control: the same with '
            'Sprites (180, flying) and the Sprites spell',
      bug='the Magic Spirit moved out is dismissed (the water refused: Tile_Room_For_Unit tests flight / water '
          'crossing, not Non-Corporeal); the Sprite goes onto the water', fixed='the Magic Spirit stands on the water')
def c310(log):
    a0, a1, ac = _full_circle_summon(154, N('SPELL_MAGIC_SPIRIT'), log, 'Magic Spirits')
    b0, b1, bc = _full_circle_summon(180, N('SPELL_SPRITES'), log, 'control, Sprites')
    obs = ('Magic Spirits (Dracon, water (29,26), all): %s before, %s after the summon; Sprites: %s -> %s'
           % (a0, a1, b0, b1))
    if ac == N('SPELL_MAGIC_SPIRIT') or bc == N('SPELL_SPRITES'):
        return dict(shown=None, observed=obs + ' - a summon was not cast')
    if b1[1] != 1:
        return dict(shown=None, observed=obs + ' - the control Sprite did not go onto the water')
    return dict(shown=a1[1] == 0 and a1[2] == a0[2], observed=obs)


def _road_pair(counters, log, tag):
    """Human units 161 and 173 made Beastmen Engineers (52) at (28,28), both on a road-building Go-To to (26,28)
    (order Go-To, road start (28,28), road_turns = counters), every other human unit finished; the turn is ended
    and the Go-To runs (Stack_Continue_GoTo -> Stack_Move mode 2). -> per unit (order, road_turns, x, y)."""
    s = mom.SaveFile()
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'order', N('ORDER_PATROL'))
            s.set('units', u, 'finished', 1)
    for u, c in zip((161, 173), counters):
        for f, v in (('type', 52), ('hero_slot', -1), ('x', 28), ('y', 28), ('plane', 0), ('order', N('ORDER_GOTO')),
                     ('dest_x', 26), ('dest_y', 28), ('road_start_x', 28), ('road_start_y', 28), ('road_turns', c),
                     ('finished', 0), ('moves_left', 2)):
            s.set('units', u, f, v)
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    time.sleep(3)
    # the turn change compacts the unit table (Units_Compact keeps the order): find the two engineers again by
    # owner, type and the road's squares, in index order (the first is 161's, the second 173's)
    found = [u for u in range(lv.glob('num_units')) if lv.get('units', u, 'owner') == 0 and
             lv.get('units', u, 'type') == 52 and lv.get('units', u, 'plane') == 0 and
             lv.get('units', u, 'y') == 28 and 26 <= lv.get('units', u, 'x') <= 28]
    if len(found) != 2:
        log.append('%s: engineers found %s - not two' % (tag, found))
        return None
    r = {u: tuple(lv.get('units', i, f) for f in ('order', 'road_turns', 'x', 'y')) for u, i in zip((161, 173), found)}
    log.append('%s (counters 161 / 173 = %s): now units %s; (order, road_turns, x, y) %s' % (tag, counters, found, r))
    return r


@case(469, 'OV', 'road building: whether the stack steps is set per engineer in turn - the last unit decides',
      setup='human units 161 and 173 made Engineers at (28,28) on one road-building Go-To to (26,28); run A: 161\'s '
            'counter 2 (still building), 173\'s 0 (done); run B: the counters swapped; the turn is ended; orders and '
            'counters read (units are processed in index order, MOVESTK.C)',
      bug='the outcome follows the unit order: in A 173 builds here and stops (Ready) - path_len was zeroed by 161 - '
          'while in B nobody stops and 161 restarts its counter', fixed='the same for the stack in both orders')
def c469(log):
    a = _road_pair((2, 0), log, 'A')
    b = _road_pair((0, 2), log, 'B')
    if a is None or b is None:
        return dict(shown=None, observed='the two engineers were not found after the turn (A %s, B %s)' % (a, b))
    obs = ('(order, road_turns, x, y) after the turn: A (161: 2, 173: 0) %s; B (161: 0, 173: 2) %s (Ready %d, Go-To '
           '%d)' % (a, b, N('ORDER_READY'), N('ORDER_GOTO')))
    if a[161][:2] == (N('ORDER_GOTO'), 2) and b[173][:2] == (N('ORDER_GOTO'), 2):
        return dict(shown=None, observed=obs + ' - the road order did not run')
    # a fixed rule treats the counter-2 engineer and the counter-0 engineer the same whatever their index
    shown = a[161][:2] != b[173][:2] or a[173][:2] != b[161][:2]
    if not shown and a[161][2:] != a[173][2:]:
        return dict(shown='latent', observed=obs + (
            '; the two engineers ended on different squares: GoTo_Build_Group (STACKCMD.C) groups only units with the '
            'lead\'s road_turns, so engineers with different counters never share a Stack_Move - the shared call the '
            'bug needs never happens (latent in 1.31)'))
    return dict(shown=shown, observed=obs)


@case(139, 'DBG', 'AI research choice: the turns estimate passes the candidate\'s REALM where a spell id is expected',
      setup='AI wizard 1 with 1 research point left (research_left 1): it learns its spell in the turn change and '
            'AI_Choose_Research picks the next; tracepoint on Research_Bonus_Pct(player, spell): the arguments and '
            'the caller (the return address) of every call during the turn change; at AI_Choose_Research\'s entry '
            'the player\'s research_candidates are read, so its i-th call is matched with the i-th candidate and the '
            'argument compared with that candidate\'s realm AND its id (only candidates whose realm differs from the '
            'id tell the two apart)',
      bug='from AI_Choose_Research the "spell" argument is 0..5 (p_spell_data[cand].realm): every candidate gets the '
          'research bonuses of spell #realm', fixed='the candidate spell ids')
def c139(log):
    import dbg
    s = mom.SaveFile()
    s.set('wizards', 1, 'research_left', 1)

    seq = []                                    # ('choose', player, candidates) / ('bonus', player, argument)

    def tr(h):
        ret_ip, ret_cs = d.word(h['SS'], h['SP'], False), d.word(h['SS'], h['SP'] + 2, False)
        w = d.where(ret_cs, ret_ip)
        if 'AI_Choose_Research' in str(w):
            seq.append(('bonus', d.arg(0, h), d.arg(1, h)))
        return d.arg(0, h), d.arg(1, h), w

    def ch(h):
        p = d.arg(0, h)
        cands = [d.lv.get('wizards', p, 'research_candidates[%d]' % k) for k in range(8)]
        seq.append(('choose', p, [(c, d.lv.get('spells', c, 'realm')) for c in cands if c]))
        return p
    d = dbg.boot(s, log=log)
    try:
        d.want('Research_Bonus_Pct', trace=tr)
        d.want('AI_Choose_Research', trace=ch)
        d.wait(240, d.end_turn_poll())
        calls = list(d.traces.get('Research_Bonus_Pct', []))
        chose = list(d.traces.get('AI_Choose_Research', []))
    finally:
        d.stop()
    ai = [(p, a) for p, a, w in calls if 'AI_Choose_Research' in str(w)]
    callers = {}
    for p, a, w in calls:
        k = str(w).split('+')[0]
        callers[k] = callers.get(k, 0) + 1
    obs = ('AI_Choose_Research entered for players %s; Research_Bonus_Pct calls from it (player, argument) %s; all '
           'callers (calls) %s' % (chose, ai[:20], callers))
    if not chose:
        return dict(shown=None, observed=obs + ' - AI_Choose_Research did not run')
    if not ai:
        return dict(shown=None, observed=obs + ' - its Research_Bonus_Pct calls were not identified')
    # match each AI_Choose_Research's calls with its candidates, in order (one call per candidate)
    pairs, cur = [], None
    for ev in seq:
        if ev[0] == 'choose':
            cur = list(ev[2])
        elif cur:
            spell, realm = cur.pop(0)
            pairs.append((spell, realm, ev[2]))
    told = [x for x in pairs if x[0] != x[1]]
    obs += ('; (candidate, its realm, the argument) %s; %d telling (realm != id): argument = realm in %d, = the id in '
            '%d' % (pairs[:16], len(told), sum(a == r for _, r, a in told), sum(a == c for c, _, a in told)))
    if not told:
        return dict(shown=None, observed=obs + ' - no candidate whose realm differs from its id was matched')
    if all(a == r for _, r, a in told):
        return dict(shown=True, observed=obs)
    if all(a == c for c, _, a in told):
        return dict(shown=False, observed=obs)
    return dict(shown=None, observed=obs + ' - the arguments match neither the realms nor the ids throughout')
