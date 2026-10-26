"""P4 B4 diplomacy cases (N233): the human's audience screens driven with mom.diplo_open / diplo_pick (UI_MAP)."""
import struct
import time

import mom
from regress import case

N = mom.N
MENU_PROPOSE, MENU_THREATEN, MENU_TRIBUTE, MENU_EXCHANGE, MENU_GOODBYE = range(5)
THREAT_BREAK_PACT, THREAT_BREAK_ALLIANCE, THREAT_ATTACK, THREAT_FORGET = range(4)
INTERESTS = ('treaty_interest', 'trade_interest', 'peace_interest')


def _met(s, a=0, b=1, relation=40, interest=50):
    """Save edit: a and b contacted and greeted (met_stage 2), no treaty, relation and interests both ways."""
    for x, y in ((a, b), (b, a)):
        for f, v in (('contacted[%d]', 1), ('met_stage[%d]', 2), ('diplomacy_status[%d]', 0),
                     ('relationship[%d]', relation)):
            s.set('wizards', x, f % y, v)
        for f in INTERESTS:
            s.set('wizards', x, '%s[%d]' % (f, y), interest)


def _interests(lv, a=0, b=1):
    return tuple(lv.get('wizards', a, '%s[%d]' % (f, b)) for f in INTERESTS)


def _leave(lv):
    for _ in range(4):
        if lv.screen() != N('SCREEN_DIPLOMACY'):
            return
        if mom.diplo_menu_up(lv):
            mom.diplo_pick(lv, MENU_GOODBYE)
        else:
            lv.g.click(160, 100, settle=1.5)


@case(52, 'DIP', 'leaving the Threaten / Break submenu by Forget It still costs -100 to all three interests',
      setup='the human and AI wizard 1 met, no treaty, relation 40, interests 50 both ways; the human opens an '
            'audience (Magic screen, wizard 1\'s gem), picks Threaten/Break Treaty, then Forget It; the human\'s '
            'treaty / trade / peace interest toward wizard 1 read before and after',
      bug='each -100 (Diplo_Interests_Minus10 x 10 on every exit)', fixed='unchanged')
def c52(log):
    s = mom.SaveFile()
    _met(s)
    lv = mom.boot(s)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    a = _interests(lv)
    mom.diplo_pick(lv, MENU_THREATEN)
    mom.shot('threaten_menu', log)
    mom.diplo_pick(lv, THREAT_FORGET)
    time.sleep(2)
    b = _interests(lv)
    mom.shot('after', log)
    _leave(lv)
    obs = 'human\'s treaty / trade / peace interest toward wizard 1: %s -> %s after Threaten -> Forget It' % (a, b)
    return dict(shown=all(y - x == -100 for x, y in zip(a, b)), observed=obs)


FORT1 = (49, 14, 1)                             # wizard 1's fortress (Myrror), landmass 14
CHEAP, DEAR = 104, 196                          # unit costs 10 and 1000 (UnitType cost, read live 10/23)


def _audience_map(portrait, plane=1):
    """What p_landmass_ids holds for one plane INSIDE an audience (#491): the partner's music. Measured 10/23 for
    portrait 1 (case 491 re-checks it live): Myrror (offset 2400) = MUSIC.LBX entry 2 x portrait + 42 from byte 1040.
    Only plane 1 of portrait 1 is known."""
    assert portrait == 1 and plane == 1
    import struct
    b = open(mom.ROOT + 'work/harness/game/MUSIC.LBX', 'rb').read()
    a, z = struct.unpack_from('<II', b, 8 + 4 * (2 * portrait + 42))
    return b[a + 1040:a + 1040 + N('MAP_SQUARES')]


def _threat_setup(s, human_n, human_type, partner_k):
    """Save edit for a threat at wizard 1. Inside the audience Diplo_Army_Near_Fortress counts the units on the
    fortress plane whose square holds the same byte of the audience map (#491: the music) as the fortress square, so:
    every such wizard 1 unit but partner_k (made cost 10) goes to the neutral player, and human_n non-hero human
    units of human_type stand 9 per square on the other land squares with that byte (Myrror)."""
    W = N('MAP_WIDTH')
    o = mom.TABLES['landmass'][0]
    fx, fy, fp = FORT1
    am = _audience_map(s.get('wizards', 1, 'portrait'), fp)
    home = am[fy * W + fx]

    def counted(x, y):
        return am[y * W + x] == home
    taken = {(s.get('cities', c, 'x'), s.get('cities', c, 'y')) for c in range(s.get('num_cities'))
             if s.get('cities', c, 'plane') == fp}
    mine = [u for u in range(s.get('num_units')) if s.get('units', u, 'owner') == 1 and s.get('units', u, 'plane') == fp
            and counted(s.get('units', u, 'x'), s.get('units', u, 'y'))]
    mine.sort(key=lambda u: s.get('units', u, 'hero_slot') != -1)    # keep plain units, give the heroes away
    for k, u in enumerate(mine):
        if k < partner_k:
            s.set('units', u, 'type', CHEAP)
        else:
            s.set('units', u, 'owner', N('PLAYER_NEUTRAL'))
    for u in range(s.get('num_units')):
        if s.get('units', u, 'plane') == fp:
            taken.add((s.get('units', u, 'x'), s.get('units', u, 'y')))
    squares = [(x, y) for y in range(N('MAP_HEIGHT')) for x in range(W)
               if counted(x, y) and s.b[o + fp * N('MAP_SQUARES') + y * W + x] != 0 and (x, y) not in taken]
    humans = [u for u in range(s.get('num_units')) if s.get('units', u, 'owner') == 0 and
              s.get('units', u, 'hero_slot') == -1][:human_n]
    for k, u in enumerate(humans):
        x, y = squares[k // 9]
        for f, v in (('x', x), ('y', y), ('plane', fp), ('type', human_type), ('order', N('ORDER_PATROL')),
                     ('finished', 1)):
            s.set('units', u, f, v)
    return min(len(mine), partner_k), len(humans)


def _long(d, h, disp):
    lo = d.word(h['SS'], (h['BP'] + disp) & 0xffff, False)
    hi = d.word(h['SS'], (h['BP'] + disp + 2) & 0xffff)
    return lo | hi << 16


def _threaten(s, log):
    """Open the audience with wizard 1, Threaten/Break -> Threaten To Attack; the roll line read at a breakpoint
    (DIPLOSCR listing: roll [bp-6], army_term [bp-18], human_army [bp-16], partner_army [bp-12], longs).
    Returns (vals, golds / status before, after, message type) or None."""
    import dbg
    d = dbg.boot(s, log=log)
    out = None
    try:
        before = (d.lv.get('wizards', 0, 'gold'), d.lv.get('wizards', 1, 'gold'),
                  d.lv.get('wizards', 0, 'diplomacy_status[1]'))
        if not mom.diplo_open(d.lv, 1, log=log):    # first: a thunk stop while this plain helper waits would leave
            mom.shot('no_menu', log)                 # the game stopped (10/23: the audience sat with no controls)
            return None
        lab = d.want_line('src/wizards/DIPLOSCR.C', 'if (roll < 90) {', label='roll')   # DIPLOSCR is loaded now
        mom.diplo_pick(d.lv, MENU_THREATEN)
        mom.diplo_pick(d.lv, THREAT_ATTACK, settle=0.5)
        h = d.wait(20, None)
        if h is None:
            return None
        r = dict(roll=d.local(-6, h), army_term=d.local(-18, h), human_army=_long(d, h, -16),
                 partner_army=_long(d, h, -12))
        d.drop(lab)
        d.go()
        time.sleep(3)
        mom.shot('reply', log)
        after = (d.lv.get('wizards', 0, 'gold'), d.lv.get('wizards', 1, 'gold'),
                 d.lv.get('wizards', 0, 'diplomacy_status[1]'))
        out = (r, before, after, d.lv.glob('diplo_msg_type'))
        _leave(d.lv)
    finally:
        d.stop()
    return out


@case(50, 'DIP', 'the threat\'s army term keeps only the low 16 bits of 100 x H / P: outnumbering the AI hugely '
                 'wraps it negative',
      setup='threat at wizard 1: on the squares the audience counts (#491: the fortress square\'s byte of the music '
            'that fills the landmass map) one of its units left (made cost 10), the others given to the neutral '
            'player; the 11 non-hero human units, made type 196 (cost 1000) on two such squares; '
            'the human threatens (Threaten/Break -> Threaten To Attack); breakpoint on "if (roll < 90)" (DIPLOSCR.C): '
            'army_term [bp-18], the armies, roll [bp-6]',
      bug='army_term = 100 x 11000 / 10 - 50 cut to 16 bits, negative: the roll goes negative, the AI declares war',
      fixed='a huge positive term: the AI gives in')
def c50(log):
    s = mom.SaveFile()
    _met(s)
    _threat_setup(s, 11, DEAR, 1)
    out = _threaten(s, log)
    if out is None:
        return dict(shown=None, observed='the threat line was not reached')
    r, before, after, msg = out
    H, P = r['human_army'], r['partner_army']
    obs = ('armies human %d / wizard 1 %d, army_term %d, roll %d; human-wizard 1 status %d -> %d (3 war); message %d'
           % (H, P, r['army_term'], r['roll'], before[2], after[2], msg))
    if not (P > 0 and H > P and H * 100 // P > 32767):   # only then can the 16-bit cut show (N233: the armies read
        return dict(shown=None, observed=obs + ' - the armies were not set up as planned')
    true = H * 100 // P - 50
    return dict(shown=r['army_term'] != true and r['army_term'] < 0, observed=obs)


@case(49, 'DIP', 'threat reparations are created from nothing: the human gets gold, the AI pays none',
      setup='threat at wizard 1 with equal armies on the squares the audience counts (#491; 3 of its units and 3 '
            'human units, all cost 10: army_term 50; the human\'s relation_bias toward it set so the roll centres '
            'on 145); the human threatens; breakpoint on "if (roll < 90)" reads the roll; golds of both before and '
            'after (the gold branch: 90 <= roll < 200)',
      bug='the human\'s gold rises, wizard 1\'s gold unchanged', fixed='wizard 1 pays it')
def c49(log):
    s = mom.SaveFile()
    _met(s)
    _threat_setup(s, 3, CHEAP, 3)
    mod = (0, 10, 20, 30, 40, 50)[s.get('wizards', 1, 'personality')]   # personality_treaty_mod (WIZARDS.C)
    s.set('wizards', 0, 'relation_bias[1]', -4 - mod)  # roll = -4 + 50 + Random(100) + Random(100): 90..199 ~78%
    out = _threaten(s, log)
    if out is None:
        return dict(shown=None, observed='the threat line was not reached')
    r, before, after, msg = out
    obs = ('roll %d (army_term %d): human gold %d -> %d, wizard 1 gold %d -> %d; message %d' % (
        r['roll'], r['army_term'], before[0], after[0], before[1], after[1], msg))
    if not 90 <= r['roll'] < 200 or after[0] <= before[0]:
        return dict(shown=None, observed=obs + ' - not the gold branch this time (the roll)')
    return dict(shown=after[1] == before[1], observed=obs)


@case(53, 'DIP', 'a threat scoring >= 200 when the AI has no spell to give is ignored instead of paying gold',
      setup='threat at wizard 1: 9 human units of cost 10 against 3 of its units of cost 10 on the squares the '
            'audience counts (#491; army_term 250); the human knows every spell wizard 1 knows (nothing to give); '
            'the human threatens; the roll read at "if (roll < 90)"; golds before and after',
      bug='roll >= 200, no gold, no spell (the threat ignored)', fixed='gold paid')
def c53(log):
    s = mom.SaveFile()
    _met(s)
    _threat_setup(s, 9, CHEAP, 3)
    for r_ in range(6):
        for k in range(40):
            if s.get('wizards', 1, 'spell_library[%d][%d]' % (r_, k)) == N('SPELLSTATE_KNOWN'):
                s.set('wizards', 0, 'spell_library[%d][%d]' % (r_, k), N('SPELLSTATE_KNOWN'))
    out = _threaten(s, log)
    if out is None:
        return dict(shown=None, observed='the threat line was not reached')
    r, before, after, msg = out
    obs = ('roll %d (army_term %d): human gold %d -> %d, wizard 1 gold %d -> %d; message %d' % (
        r['roll'], r['army_term'], before[0], after[0], before[1], after[1], msg))
    if r['roll'] < 200:
        return dict(shown=None, observed=obs + ' - roll below 200 this time')
    return dict(shown=after[0] == before[0], observed=obs)


@case(491, 'DIP', 'inside an audience the landmass map holds the partner\'s music',
      setup='the human and wizard 1 (portrait 1) met; the 4800 bytes at p_landmass_ids (DS:9cd0) read on the map, in '
            'the audience menu (Magic screen, wizard 1\'s gem) and after leaving; the Myrror half compared with '
            'MUSIC.LBX entry 44 from byte 1040 (the partner\'s Bad music, Diplo_Screen_Load)',
      bug='in the audience the map is music (the fortress square (49,14) Myrror is not landmass 14); restored after',
      fixed='the landmass map unchanged in the audience')
def c491(log):
    import struct
    s = mom.SaveFile()
    _met(s)
    lv = mom.boot(s)
    off, seg = struct.unpack('<HH', lv.g.ram(lv.g.ds + 0x9cd0, 4))
    lin = seg * 16 + off
    sq = N('MAP_SQUARES')
    fort = sq + FORT1[1] * N('MAP_WIDTH') + FORT1[0]
    on_map = lv.g.ram(lin, 2 * sq)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    aud = lv.g.ram(lin, 2 * sq)
    _leave(lv)
    time.sleep(3)
    after = lv.g.ram(lin, 2 * sq)
    music = aud[sq:] == _audience_map(s.get('wizards', 1, 'portrait'))
    obs = ('fortress square (49,14) Myrror: %d on the map, %d in the audience, %d after leaving; %d of %d bytes differ '
           'in the audience; Myrror half = MUSIC entry 44 from byte 1040: %s; restored exactly: %s'
           % (on_map[fort], aud[fort], after[fort], sum(x != y for x, y in zip(on_map, aud)), 2 * sq, music,
              after == on_map))
    return dict(shown=aud[fort] != on_map[fort] and music, observed=obs)


def _try_audience(lv, wizard=1, from_map=True, timeout=20):
    """Open an audience (M, then the wizard's gem) and click through it ONLY while the diplomacy screen is up (a click
    at the Magic screen's centre could move its staves). -> dict(menu: the main menu came up, msgs / modes: the
    diplo_msg_type / diplo_talk_mode values seen)."""
    if from_map:
        lv.D.key('m')
        time.sleep(2.5)
    lv.g.click((wizard - 1) * 77 + 29 + 15, 25, settle=2.0)
    out = dict(menu=False, msgs=set(), modes=set())
    t0 = time.time()
    while time.time() - t0 < timeout:
        if lv.screen() == N('SCREEN_DIPLOMACY'):
            out['msgs'].add(lv.glob('diplo_msg_type'))
            out['modes'].add(lv.glob('diplo_talk_mode'))
            if mom.diplo_menu_up(lv):
                out['menu'] = True
                return out
            lv.g.click(160, 100, settle=1.5)
        elif time.time() - t0 > 6:
            break                                    # back on the Magic screen: the audience is over
        else:
            time.sleep(0.5)
    return out


def _menu_items(lv):
    """The strings of the list menu (diplo_menu_items, DS:c3f0: 7 near pointers to 100-byte strings)."""
    out = []
    for k in range(7):
        p = struct.unpack('<H', lv.g.ram(lv.g.ds + 0xc3f0 + 2 * k, 2))[0]
        t = lv.g.ram(lv.g.ds + p, 100).split(b'\0')[0].decode('latin-1')
        if not t:
            break
        out.append(t)
    return out


@case(322, 'DIP', 'an AI greets the human and then at once is tired of talks (two different tests)',
      setup='the human and wizard 1 (Peaceful: personality_treaty_mod 50) met, relation bias 0; the human\'s trade '
            'interest toward wizard 1 -110, the other two 50; the human opens an audience; diplo_talk_mode / '
            'diplo_msg_type read while the screen is up',
      bug='greeted (mode 0: bias + lowest interest + modifier = -60 > -100), then message 0x47 "tired of talks" and '
          'no menu (the lowest interest < -100)', fixed='refused at the greeting, or the menu shown')
def c322(log):
    s = mom.SaveFile()
    _met(s)
    s.set('wizards', 0, 'trade_interest[1]', -110)
    lv = mom.boot(s)
    r = _try_audience(lv)
    mom.shot('after', log)
    lv.D.key('esc')
    end = N('DIPLO_MSG_END_TALKS')
    obs = 'talk modes seen %s, messages seen %s, main menu shown: %s' % (sorted(r['modes']), sorted(r['msgs']),
                                                                         r['menu'])
    if not r['modes']:
        return dict(shown=None, observed=obs + ' - the audience did not open')
    return dict(shown=N('DIPLO_TALK_GREETING') in r['modes'] and end in r['msgs'] and not r['menu'], observed=obs)


@case(65, 'DIP', 'a pending AI message blocks the human\'s diplomacy menu with that wizard for the whole turn',
      setup='the human and wizard 1 met; wizards[0].pending_rel_msg[1] = DIPLO_MSG_UNPROVOKED_ATTACK (amount -40), '
            'as ATTACK.C leaves it after the human attacks; the human opens an audience with wizard 1 twice in the '
            'same turn; the pending message read after',
      bug='both times only the message, no menu; the message still pending (cleared only at the turn change)',
      fixed='the menu after the message')
def c65(log):
    s = mom.SaveFile()
    _met(s)
    msg = N('DIPLO_MSG_UNPROVOKED_ATTACK')
    city = next(c for c in range(s.get('num_cities')) if s.get('cities', c, 'owner') == 1)
    for f, v in (('pending_rel_msg[1]', msg), ('pending_rel_amount[1]', -40), ('pending_msg_city[1]', city)):
        s.set('wizards', 0, f, v)
    lv = mom.boot(s)
    r1 = _try_audience(lv)
    mom.shot('first', log)
    r2 = _try_audience(lv, from_map=False)
    mom.shot('second', log)
    pend = lv.get('wizards', 0, 'pending_rel_msg[1]')
    lv.D.key('esc')
    obs = ('first audience: messages %s, menu %s; second: messages %s, menu %s; pending message after: %d (set %d)'
           % (sorted(r1['msgs']), r1['menu'], sorted(r2['msgs']), r2['menu'], pend, msg))
    if msg not in r1['msgs']:
        return dict(shown=None, observed=obs + ' - the first audience did not show the message')
    return dict(shown=not r1['menu'] and not r2['menu'] and msg in r2['msgs'] and pend == msg, observed=obs)


@case(63, 'DIP', 'Offer Tribute\'s gold choices are sized by the AI\'s treasury, not the human\'s',
      setup='the human (987 gold) and wizard 1 (gold set to 20) met; the human opens an audience, Offer Tribute; the '
            'menu strings read (diplo_menu_items)',
      bug='no gold choice at all (the partner has < 25), only Spells / Forget It',
      fixed='gold choices the human can pay')
def c63(log):
    s = mom.SaveFile()
    _met(s)
    s.set('wizards', 1, 'gold', 20)
    lv = mom.boot(s)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    mom.diplo_pick(lv, MENU_TRIBUTE)
    items = _menu_items(lv)
    rows = mom.diplo_rows(lv)
    mom.shot('tribute_menu', log)
    if rows:
        mom.diplo_pick_row(lv, len(rows) - 1)        # Forget It
    _leave(lv)
    obs = 'human gold %d, wizard 1 gold %d; tribute menu: %s' % (lv.get('wizards', 0, 'gold'),
                                                                  lv.get('wizards', 1, 'gold'), items)
    if not items or 'Spells' not in ''.join(items):
        return dict(shown=None, observed=obs + ' - the tribute menu was not read')
    return dict(shown=not any('gold' in t for t in items), observed=obs)


@case(61, 'DIP', 'the human\'s peace proposal is scored on the TREATY interest, not the peace interest',
      setup='the human and wizard 1 at war (status 3 both ways), relation 100; the human\'s treaty interest toward '
            'wizard 1 100, peace interest -100, trade 50; Dracon\'s Nightshade count slot (enchants[25]) set to 2 = '
            'wizard 1 + 1 (for #59); the human proposes a Peace Treaty; interests, status and the slot read after',
      bug='accepted although peace interest is -100; the treaty interest drops 50..79 (-10, -(20..49), -20), the '
          'peace interest only 30', fixed='scored on peace interest -100: refused; peace interest drops 50..99 more',
      also=(59,))
def c61(log):
    s = mom.SaveFile()
    _met(s, relation=100)
    for a, b in ((0, 1), (1, 0)):
        s.set('wizards', a, 'diplomacy_status[%d]' % b, N('TREATY_WAR'))
    s.set('wizards', 0, 'treaty_interest[1]', 100)
    s.set('wizards', 0, 'peace_interest[1]', -100)
    dracon = next(c for c in range(s.get('num_cities')) if s.get('cities', c, 'owner') == 0
                  and (s.get('cities', c, 'x'), s.get('cities', c, 'y')) == (29, 27))
    s.set('cities', dracon, 'enchants[25]', 2)
    lv = mom.boot(s)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    a = _interests(lv)
    mom.diplo_pick(lv, MENU_PROPOSE)
    items = _menu_items(lv)
    mom.shot('propose_menu', log)
    k = next((i for i, t in enumerate(items) if 'Peace' in t), None)
    if k is None or not mom.diplo_pick_row(lv, k, settle=3.0):
        _leave(lv)
        return dict(shown=None, observed='no Peace Treaty row: %s' % items)
    mom.shot('reply', log)
    b = _interests(lv)
    status = lv.get('wizards', 0, 'diplomacy_status[1]')
    slot = lv.get('cities', dracon, 'enchants[25]')
    _leave(lv)
    d = [y - x for x, y in zip(a, b)]
    obs = ('menu %s; status after %d (3 war); treaty / trade / peace interest %s -> %s (change %s); Dracon '
           'enchants[25] 2 -> %d' % (items, status, a, b, d, slot))
    if status == N('TREATY_WAR'):
        return dict(shown=None, observed=obs + ' - peace not made (the reply)')
    return dict(shown=-79 <= d[0] <= -50 and d[2] == -30, observed=obs,
                also={59: (slot == 0, 'Make Peace in the human\'s audience: Dracon\'s Nightshade count slot 2 '
                           '(= wizard 1 + 1) -> %d; ' % slot + obs)})


def _lib(sp):
    """The Wizard field of spell sp's state."""
    return 'spell_library[%d][%d]' % ((sp - 1) // 40, (sp - 1) % 40)


def _spell_names():
    """Spell number -> (name, research cost) from SPELLDAT.LBX (struct Spell: name +0, research_cost +0x1c)."""
    b = open(mom.ROOT + 'work/harness/game/SPELLDAT.LBX', 'rb').read()
    a, z = struct.unpack_from('<II', b, 8)
    n, sz = struct.unpack_from('<HH', b, a)
    out = {}
    for i in range(n):
        r = b[a + 4 + i * sz:a + 4 + (i + 1) * sz]
        out[i] = (r[:18].split(b'\0')[0].decode('latin-1'), struct.unpack_from('<H', r, 0x1c)[0])
    return out


DISENCHANT_AREA, DISPEL_MAGIC, SUMMONING_CIRCLE = 204, 202, 203   # Arcane: tradeable whatever the books


@case(54, 'DIP', 'spell trade values are kept as bytes: research costs compared mod 256',
      setup='wizard 1 knows Disenchant Area (research 300; the human\'s copy RESEARCHABLE: value 300, stored as 44) '
            'and nothing else the human lacks (the human given every other spell wizard 1 knows); the human knows '
            'Dispel Magic (100) and Summoning Circle (150), wizard 1 does not; relation -100, trade interest -100 '
            '(for #47); the human opens Exchange Spells, picks Disenchant Area; the counter-offer menu read, then '
            'Dispel Magic picked; both libraries read after',
      bug='Dispel Magic / Summoning Circle (worth 100 / 150 < 300) offered in exchange: they pass "worth at least 44"',
      fixed='only spells worth 300 or more', also=(47,))
def c54(log):
    s = mom.SaveFile()
    _met(s, relation=-100)
    s.set('wizards', 0, 'trade_interest[1]', -100)
    known = N('SPELLSTATE_KNOWN')
    for sp in range(1, 215):
        if s.get('wizards', 1, _lib(sp)) == known and sp != DISENCHANT_AREA:
            s.set('wizards', 0, _lib(sp), known)
    s.set('wizards', 1, _lib(DISENCHANT_AREA), known)
    s.set('wizards', 0, _lib(DISENCHANT_AREA), N('SPELLSTATE_RESEARCHABLE'))
    for sp in (DISPEL_MAGIC, SUMMONING_CIRCLE):
        s.set('wizards', 0, _lib(sp), known)
        s.set('wizards', 1, _lib(sp), N('SPELLSTATE_RESEARCHABLE'))
    names = _spell_names()
    lv = mom.boot(s)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    mom.diplo_pick(lv, MENU_EXCHANGE)
    offers = _menu_items(lv)
    mom.shot('offers', log)
    k = next((i for i, t in enumerate(offers) if names[DISENCHANT_AREA][0] in t), None)
    if k is None or not mom.diplo_pick_row(lv, k):
        _leave(lv)
        return dict(shown=None, observed='Disenchant Area not offered: %s' % offers)
    counters = _menu_items(lv)
    mom.shot('counters', log)
    j = next((i for i, t in enumerate(counters) if names[DISPEL_MAGIC][0] in t), None)
    if j is not None:
        mom.diplo_pick_row(lv, j, settle=3.0)
    else:
        mom.diplo_pick_row(lv, len(counters) - 1)   # Forget It
    mom.shot('after', log)
    got = lv.get('wizards', 0, _lib(DISENCHANT_AREA)) == known
    gave = lv.get('wizards', 1, _lib(DISPEL_MAGIC)) == known
    _leave(lv)
    cheap = [t for t in counters if names[DISPEL_MAGIC][0] in t or names[SUMMONING_CIRCLE][0] in t]
    obs = 'offers %s; counter-offers for Disenchant Area (300) %s' % (offers, counters)
    swap = ('Dispel Magic picked: the human now knows Disenchant Area %s, wizard 1 knows Dispel Magic %s; the unused '
            'accept roll = Random(100) + 0 bias - 50 (relation / 2) + 100 (2 x Peaceful 50) - 100 (trade) - 125 = '
            '-174 .. -75' % (got, gave))
    return dict(shown=bool(cheap), observed=obs,
                also={47: (None if j is None else got and gave, swap + '; ' + obs)})


MAGIC_SPIRIT = 201                              # Arcane, research 20


def _tribute_spells(lv, log, tag, pick=None):
    """Offer Tribute -> Spells: the list read; then `pick` (a name) or Forget It. -> the list (names), or None."""
    mom.diplo_pick(lv, MENU_TRIBUTE)
    first = _menu_items(lv)
    k = next((i for i, t in enumerate(first) if 'Spells' in t), None)
    if k is None or not mom.diplo_pick_row(lv, k):
        return None
    items = _menu_items(lv)
    mom.shot(tag, log)
    j = next((i for i, t in enumerate(items) if pick and pick in t), None)
    mom.diplo_pick_row(lv, len(items) - 1 if j is None else j, settle=3.0)
    if j is not None:
        lv.g.click(160, 100, settle=1.5)             # the thanks
    return [t[2:] for t in items]


@case(48, 'DIP', 'Offer Tribute (spells) uses a leftover byte as its minimum value: a stale value hides spells',
      setup='wizard 1 at 20 gold (no gold choice: n_gold 0, so the minimum is xchg_ai_values[0]); the only spell it '
            'knows that the human lacks is Disenchant Area (value 300, kept as 44); of the human\'s spells it lacks '
            'only Magic Spirit (20) and Dispel Magic (100); interests 120 / 200 / 150 (peace below its 200 cap). In '
            'one audience: Offer Tribute -> Spells listed, Forget It; Exchange Spells opened (fills '
            'xchg_ai_values[0] = 44), left by Forget It; Offer Tribute -> Spells listed again, Dispel Magic given '
            '(#67: '
            'the interests read before / after)',
      bug='Magic Spirit listed the first time, hidden after the exchange (worth 20 < the stale 44)',
      fixed='the same spells both times', also=(67,))
def c48(log):
    s = mom.SaveFile()
    _met(s)
    for f, v in (('treaty_interest[1]', 120), ('trade_interest[1]', 200), ('peace_interest[1]', 150)):
        s.set('wizards', 0, f, v)
    s.set('wizards', 1, 'gold', 20)
    known = N('SPELLSTATE_KNOWN')
    both = {sp for sp in range(1, 215) for w in (0, 1) if s.get('wizards', w, _lib(sp)) == known}
    both -= {DISENCHANT_AREA}
    for sp in both | {MAGIC_SPIRIT, DISPEL_MAGIC}:
        s.set('wizards', 0, _lib(sp), known)
        s.set('wizards', 1, _lib(sp), known)
    for sp in (MAGIC_SPIRIT, DISPEL_MAGIC):
        s.set('wizards', 1, _lib(sp), N('SPELLSTATE_RESEARCHABLE'))
    s.set('wizards', 1, _lib(DISENCHANT_AREA), known)
    s.set('wizards', 0, _lib(DISENCHANT_AREA), N('SPELLSTATE_RESEARCHABLE'))
    lv = mom.boot(s)
    if not mom.diplo_open(lv, 1, log=log):
        mom.shot('no_menu', log)
        return dict(shown=None, observed='the diplomacy menu did not come up')
    a = _tribute_spells(lv, log, 'tribute_1')
    mom.diplo_pick(lv, MENU_EXCHANGE)
    offers = _menu_items(lv)
    mom.shot('exchange', log)
    mom.diplo_pick_row(lv, 0)                       # the offer, then Forget It (Esc here also closed the
    mom.diplo_pick_row(lv, len(_menu_items(lv)) - 1)   # audience, 10/23 - the key read again by the main menu)
    rel0 = (lv.get('wizards', 0, 'relationship[1]'), lv.get('wizards', 0, 'peace_interest[1]'),
            lv.get('wizards', 1, 'peace_interest[0]'))
    b = _tribute_spells(lv, log, 'tribute_2', pick=_spell_names()[DISPEL_MAGIC][0])
    rel1 = (lv.get('wizards', 0, 'relationship[1]'), lv.get('wizards', 0, 'peace_interest[1]'),
            lv.get('wizards', 1, 'peace_interest[0]'))
    given = lv.get('wizards', 1, _lib(DISPEL_MAGIC)) == known
    _leave(lv)
    ms = _spell_names()[MAGIC_SPIRIT][0]
    obs = 'tribute spells before the exchange %s; exchange offers %s; after %s' % (a, offers, b)
    if not a or not b or ms not in a:
        return dict(shown=None, observed=obs + ' - the lists were not read as planned')
    gain = rel1[0] - rel0[0]
    o67 = ('Dispel Magic given %s: relation %d -> %d, wizards[0].peace_interest[1] %d -> %d, wizards[1].'
           'peace_interest[0] %d -> %d' % (given, rel0[0], rel1[0], rel0[1], rel1[1], rel0[2], rel1[2]))
    return dict(shown=ms not in b, observed=obs,
                also={67: (None if not given or gain <= 0 else rel1[1] - rel0[1] == gain and rel1[2] == rel0[2],
                           o67)})


LIVE_AIS = (1, 2, 4)                            # wizard 3's fortress is gone in the base save


def _ai_pact_setup(s, ais=LIVE_AIS):
    """Save edit so that each AI in `ais` proposes a Wizard Pact to the human at the next turn change with chance
    1 in 4 (AI_Diplo_Turn_Vs_Human, DIPLOTRN.C): met, no treaty, relation 20, the human's relation bias toward it -100
    and trade interest -100 (the lowest), so bias + 2 x relation + lowest interest + Peaceful 50 = -110 < -100 takes
    the proposal branch; pending amount 100 (chance 203 > Random(100)), no warnings; then the 1 in 4 and the pact roll
    (score -30 + Random(100) + treaty interest 120 >= 50) - AI_Diplo_Propose_To_Human. Peaceful (> Ruthless) and
    population 1 against the human's 100 make the incentive roll Random(100) <= 3000 always pass, leaving its 1 in 8."""
    for ai in ais:
        _met(s, 0, ai, relation=20)
        s.set('wizards', ai, 'personality', N('PERSONALITY_PEACEFUL'))
        s.set('wizards', ai, 'stat_population', 1)
        for f, v in (('relation_bias[%d]', -100), ('treaty_interest[%d]', 120), ('trade_interest[%d]', -100),
                     ('peace_interest[%d]', 50), ('pending_rel_amount[%d]', 100), ('warning_count[%d]', 0),
                     ('pending_rel_msg[%d]', 0), ('tribute_gold[%d]', 0), ('tribute_spell[%d]', 0)):
            s.set('wizards', 0, f % ai, v)
    s.set('wizards', 0, 'stat_population', 100)


def _text(lv):
    p = lv.glob('p_diplo_text')
    return lv.g.ram(lv.g.ds + p, 0xdc).split(b'\0')[0].decode('latin-1')


def _turn_audiences(lv, log, answer, timeout=300, ais=LIVE_AIS):
    """End the turn (Done on every stack, N) and drive the AI audiences of the turn change (Diplo_Show_Pending_Messages,
    NEXTTURN.C): while a proposal is pending, a list menu (Screen_Diplomacy_Offer) is answered by answer(lv, ai,
    record) -> the row to click, anything else is clicked through; other screens get mom.end_turn's answers.
    -> the list of records (ai, message, tribute gold / spell, human gold, the menu items and text at each menu)."""
    D, g = lv.D, lv.g
    turn0 = lv.turn()
    t0 = time.time()
    mp = N('SCREEN_MAP')
    recs = {}
    stalls = 0
    while time.time() - t0 < timeout:
        if lv.turn() != turn0 and lv.screen() == mp and mom.on_map(lv):
            break
        pend = {ai: lv.get('wizards', 0, 'pending_rel_msg[%d]' % ai) for ai in ais}
        cur = lv.glob('diplo_partner')
        if lv.turn() == turn0 and pend.get(cur, 0) and cur in ais and (mom.diplo_rows(lv) or any(
                c[:4] == (0, 0, 319, 199) for c in mom.controls(lv))):
            r = recs.setdefault(cur, dict(ai=cur, msg=pend[cur], menus=[], gold0=lv.get('wizards', 0, 'gold'),
                                          ai_gold0=lv.get('wizards', cur, 'gold'),
                                          tribute=(lv.get('wizards', 0, 'tribute_gold[%d]' % cur),
                                                   lv.get('wizards', 0, 'tribute_spell[%d]' % cur))))
            if mom.diplo_rows(lv):
                r['menus'].append((_menu_items(lv), _text(lv)))
                mom.shot('ai%d_menu%d' % (cur, len(r['menus'])), log)
                row = answer(lv, cur, r)
                mom.diplo_pick_row(lv, row, settle=2.5)
            else:
                g.click(160, 100, settle=1.5)
            continue
        for r in recs.values():
            if 'gold1' not in r and not pend.get(r['ai']):
                r['gold1'] = lv.get('wizards', 0, 'gold')
                r['ai_gold1'] = lv.get('wizards', r['ai'], 'gold')
                r['status1'] = lv.get('wizards', 0, 'diplomacy_status[%d]' % r['ai'])
        scr, nou = lv.screen(), lv.glob('no_units_to_move')
        if lv.turn() == turn0 and scr == mp and mom.on_map(lv):
            D.key('d' if nou == 0 else 'n')
            time.sleep(0.4 if nou == 0 else 1.5)
            continue
        time.sleep(1.0)
        stalls += 1
        if stalls % 5 == 0 and not any(pend.values()):
            [lambda: D.key('esc'), lambda: D.key('enter'), lambda: D.key('y'), lambda: g.click(60, 33),
             lambda: g.click(255, 185), lambda: g.click(202, 132)][(stalls // 5 - 1) % 6]()
    for r in recs.values():
        r.setdefault('gold1', lv.get('wizards', 0, 'gold'))
        r.setdefault('ai_gold1', lv.get('wizards', r['ai'], 'gold'))
        r.setdefault('status1', lv.get('wizards', 0, 'diplomacy_status[%d]' % r['ai']))
    if log is not None:
        log.append('  turn %d -> %d; audiences %s' % (turn0, lv.turn(), sorted(recs)))
    return list(recs.values())



CONFUSION = 46                                  # a Sorcery spell wizard 2 knows and the human lacks


def _proposal_setup(s):
    """_ai_pact_setup plus contact between wizards 1 and 4: with this save the turn change brings Wizard Pact
    proposals from wizards 1 and 2 (found 10/23 by varying the AI-AI contacts; the turn change is the same every run
    - turn_rng_seed is not saved, NEXTTURN.C), neither with an incentive of its own."""
    _ai_pact_setup(s)
    s.set('wizards', 1, 'contacted[4]', 1)
    s.set('wizards', 4, 'contacted[1]', 1)


@case(56, 'DIP', 'accepting an AI pact proposal at once still pays the prepared incentive gold, never offered',
      setup='AIs 1, 2, 4 set up to propose pacts at the turn change (_proposal_setup: wizards 1 and 2 do); the '
            'incentive fields are save values the proposal code leaves alone unless its own incentive roll passes '
            '(1 in 8; it does not here): wizards[0].tribute_gold[1] = 150; wizards[0].tribute_spell[2] = Confusion, '
            'tribute_gold[2] = 0 (#64). The human accepts wizard 1 at once; rejects wizard 2 first (the incentive '
            'text read), then accepts. Golds of the human and the AIs before / after',
      bug='wizard 1: accepted at once, the human gains 150 gold that was never mentioned; the AI\'s gold unchanged '
          '(#166: paid from nothing)', fixed='nothing paid when accepted at once (or the offer names it)',
      also=(166, 64))
def c56(log):
    s = mom.SaveFile()
    _proposal_setup(s)
    s.set('wizards', 0, 'tribute_gold[1]', 150)
    s.set('wizards', 0, 'tribute_spell[2]', CONFUSION)
    lv = mom.boot(s)

    def answer(lv, ai, r):
        return 1 if ai == 2 and len(r['menus']) == 1 else 0     # wizard 2: Reject once, then Accept
    recs = {r['ai']: r for r in _turn_audiences(lv, log, answer)}
    conf = lv.get('wizards', 0, _lib(CONFUSION)) == N('SPELLSTATE_KNOWN')
    r1, r2 = recs.get(1), recs.get(2)
    if not r1:
        return dict(shown=None, observed='no proposal from wizard 1 this time: %s' % sorted(recs))
    o1 = ('wizard 1 pact offer %s, accepted at once (status after %d): human gold %d -> %d, wizard 1 gold %d -> %d '
          '(tribute_gold 150 set, tribute_spell 0)' % (r1['menus'][0][1], r1['status1'], r1['gold0'], r1['gold1'],
                                                      r1['ai_gold0'], r1['ai_gold1']))
    paid = r1['status1'] == N('TREATY_WIZARD_PACT') and r1['gold1'] - r1['gold0'] == 150
    also = {166: (None if not paid else r1['ai_gold1'] == r1['ai_gold0'], o1)}
    if r2 and len(r2['menus']) >= 2:
        t = r2['menus'][1][1]
        o2 = ('wizard 2 (tribute spell Confusion, gold 0), rejected first: the incentive text "%s"; accepted then: '
              'the human knows Confusion %s' % (t, conf))
        also[64] = ('Confusion' not in t and 'gold' not in t, o2)
    else:
        also[64] = (None, 'wizard 2: no second (incentive) menu: %s' % (r2 and r2['menus']))
    return dict(shown=paid, observed=o1, also=also)


def _wname(src, k):
    """Wizard k's name (Wizard.wizard_name, 20 chars) from a SaveFile or a live game."""
    return bytes(src.get('wizards', k, 'wizard_name[%d]' % i) & 0xff for i in range(20)).split(b'\0')[0].decode(
        'latin-1')


@case(225, 'DIP', 'an AI\'s demand to break your alliance names the wrong wizard',
      setup='_proposal_setup (wizard 1 proposes a pact at the turn change) plus: the human allied with wizard 2 '
            '(status 2 both ways), wizard 1 at war with wizard 2, the human\'s war_request_target[1] = 4 (the field '
            'the text reads; AI_Ally_Pick_War_Request leaves it there). Before its offer wizard 1 demands the alliance '
            'be broken (Diplo_Demand_Drop_Ally): its text read, then agreed; the human-wizard 2 status read after',
      bug='the demand names wizard 4 (war_request_target), not the ally wizard 2 (stored in demand_drop_ally, never '
          'read); agreeing breaks the alliance with wizard 2', fixed='the text names wizard 2')
def c225(log):
    s = mom.SaveFile()
    _proposal_setup(s)
    for a, b, st in ((0, 2, N('TREATY_ALLIANCE')), (1, 2, N('TREATY_WAR'))):
        s.set('wizards', a, 'diplomacy_status[%d]' % b, st)
        s.set('wizards', b, 'diplomacy_status[%d]' % a, st)
    s.set('wizards', 0, 'war_request_target[1]', 4)
    names = {k: _wname(s, k) for k in range(5)}
    lv = mom.boot(s)
    recs = {r['ai']: r for r in _turn_audiences(lv, log, lambda lv, ai, r: 0)}
    st2 = lv.get('wizards', 0, 'diplomacy_status[2]')
    r1 = recs.get(1)
    demand = next((t for items, t in (r1 or {}).get('menus', []) if names[4] in t or names[2] in t
                   or 'alliance' in t.lower() or 'treaty' in t.lower()), None)
    obs = 'wizard 1 audience menus %s; names: 2 = %s, 4 = %s; human-wizard 2 status after %d (2 alliance)' % (
        r1 and [t for _, t in r1['menus']], names[2], names[4], st2)
    if not r1:
        return dict(shown=None, observed='no audience from wizard 1 this time: %s' % sorted(recs))
    if demand is None:
        return dict(shown=None, observed=obs + ' - no demand text seen')
    return dict(shown=names[4] in demand and names[2] not in demand, observed=obs)


def _spell_offer_setup(s, ais=LIVE_AIS, mask=0):
    """Save edit so that each AI in `ais` may offer the human a spell trade at the turn change
    (AI_Diplo_Propose_To_Human,
    AIDIPLO.C): relation -100 (no pact branch), the human's relation bias toward it 100, treaty interest -100 (the
    lowest; bias + 2 x relation + lowest + Peaceful 50 = -150 < -100), trade interest 50, pending amount 100; the
    spells roll bias + relation + 50 + Random(100) >= 25 always passes. Libraries: everyone knows what anyone knows
    (Spell of Mastery / Return left out) except that only the human knows Dispel Magic (100) and only the AIs know
    Disenchant Area (300): the AI prices Dispel Magic at 100 x 100 / (130 .. 229) = 43 .. 76 and keeps Disenchant Area
    only through its byte value 300 -> 44 (#54). mask: AI-AI contacts 1-2, 1-4, 2-4 (bits) to vary the turn's
    random stream."""
    known = N('SPELLSTATE_KNOWN')
    every = {sp for sp in range(1, 213) for w in (0,) + tuple(ais) if s.get('wizards', w, _lib(sp)) == known}
    for w in (0,) + tuple(ais):
        for sp in every - {DISPEL_MAGIC, DISENCHANT_AREA}:
            s.set('wizards', w, _lib(sp), known)
        s.set('wizards', w, _lib(DISPEL_MAGIC), known if w == 0 else N('SPELLSTATE_RESEARCHABLE'))
        s.set('wizards', w, _lib(DISENCHANT_AREA), N('SPELLSTATE_RESEARCHABLE') if w == 0 else known)
    for ai in ais:
        _met(s, 0, ai, relation=-100)
        s.set('wizards', ai, 'personality', N('PERSONALITY_PEACEFUL'))
        for f, v in (('relation_bias[%d]', 100), ('treaty_interest[%d]', -100), ('trade_interest[%d]', 50),
                     ('peace_interest[%d]', 50), ('pending_rel_amount[%d]', 100), ('warning_count[%d]', 0),
                     ('pending_rel_msg[%d]', 0), ('tribute_gold[%d]', 0), ('tribute_spell[%d]', 0)):
            s.set('wizards', 0, f % ai, v)
    for k, (a, b) in enumerate(((1, 2), (1, 4), (2, 4))):
        v = mask >> k & 1
        s.set('wizards', a, 'contacted[%d]' % b, v)
        s.set('wizards', b, 'contacted[%d]' % a, v)


@case(55, 'DIP', 'an AI\'s spell-trade offer: the human receives the chosen spell, the AI never gets the one it asked '
                 'for',
      setup='_spell_offer_setup (mask 0: wizard 2 offers a spell trade at the turn change - found 10/23): only the '
            'human knows Dispel Magic, only the AIs know Disenchant Area; the human picks Disenchant Area in "In '
            'exchange you will receive:"; wanted_spell[2] read during the audience, both libraries after',
      bug='the human knows Disenchant Area; wizard 2 still lacks the spell it asked for (Dispel Magic)',
      fixed='wizard 2 receives Dispel Magic')
def c55(log):
    s = mom.SaveFile()
    _spell_offer_setup(s, mask=0)
    lv = mom.boot(s)
    names = _spell_names()

    def answer(lv, ai, r):
        r['wanted'] = lv.get('wizards', 0, 'wanted_spell[%d]' % ai)
        items = _menu_items(lv)
        return next((i for i, t in enumerate(items) if names[DISENCHANT_AREA][0] in t), len(items) - 1)
    recs = {r['ai']: r for r in _turn_audiences(lv, log, answer)}
    known = N('SPELLSTATE_KNOWN')
    r2 = recs.get(2)
    if not r2 or r2['msg'] != N('DIPLO_MSG_OFFER_SPELL_TRADE') or 'wanted' not in r2:
        return dict(shown=None, observed='no spell-trade offer from wizard 2: %s' % {
            a: (r['msg'], [m[0] for m in r['menus']]) for a, r in recs.items()})
    got = lv.get('wizards', 0, _lib(DISENCHANT_AREA)) == known
    w = r2['wanted']
    gave = lv.get('wizards', 2, _lib(w)) == known if w else None
    obs = ('wizard 2 offer menus %s; it asked for spell %d (%s); after: the human knows Disenchant Area %s, wizard 2 '
           'knows %s %s' % ([m[0] for m in r2['menus']], w, names.get(w, ('?',))[0], got, names.get(w, ('?',))[0],
                            gave))
    return dict(shown=got and gave is False, observed=obs)


@case(60, 'DIP', 'two-player games: the diplomacy text variant is Random(n) - 2: the last variant never appears, '
                 'variant 0 twice as often',
      setup='num_players set to 2 (the human and wizard 1, met); the human opens the audience with wizard 1 again and '
            'again (Good Bye in between); at each greeting diplo_msg_type, diplo_msg_variant and '
            'diplo_msg_variant_count read (Diplo_Load_Message, DIPLOMSG.C)',
      bug='variants 0 .. n - 2 only; 0 about twice as often as each other', fixed='0 .. n - 1 evenly')
def c60(log):
    s = mom.SaveFile()
    _met(s)
    s.set('num_players', value=2)
    lv = mom.boot(s)
    seen = []
    lv.D.key('m')
    time.sleep(2.5)
    for k in range(40):
        lv.g.click(29 + 15, 25, settle=2.0)            # wizard 1's gem
        t0 = time.time()
        while time.time() - t0 < 12 and lv.screen() != N('SCREEN_DIPLOMACY'):
            time.sleep(0.3)
        time.sleep(1.0)
        if lv.screen() == N('SCREEN_DIPLOMACY'):
            seen.append((lv.glob('diplo_msg_type'), lv.glob('diplo_msg_variant'), lv.glob('diplo_msg_variant_count')))
        _leave(lv)
        t0 = time.time()
        while time.time() - t0 < 8 and lv.screen() == N('SCREEN_DIPLOMACY'):
            time.sleep(0.3)
    lv.D.key('esc')
    by = {}
    for m, v, n in seen:
        by.setdefault((m, n), []).append(v)
    obs = '%d greetings; per (message, count n): variants seen %s' % (
        len(seen), {k: sorted(v) for k, v in by.items()})
    if len(seen) < 20:
        return dict(shown=None, observed=obs + ' - too few audiences')
    top_missing = all(max(v) <= n - 2 for (m, n), v in by.items())
    p_fixed = 1.0
    for (m, n), v in by.items():
        p_fixed *= (1 - 1 / n) ** len(v)
    return dict(shown=top_missing, observed=obs + '; chance that an even 0 .. n - 1 draw never gives n - 1 here: '
                '%.2g' % p_fixed)


@case(51, 'DIP', 'a threat with NO units near the AI\'s fortress counts -100, a token force far worse',
      setup='two threats at wizard 1, each in its own run, with 3 of its units (cost 10) on the squares the audience '
            'counts (#491): (a) no human unit there, (b) one human unit of cost 10 there; army_term read at "if (roll '
            '< 90)" (DIPLOSCR.C)', bug='(a) -100, (b) -50 - 100 x 30 / 10 = -350: the token force is far worse',
      fixed='a token force no worse than none')
def c51(log):
    terms = []
    for human_n in (0, 1):
        s = mom.SaveFile()
        _met(s)
        _threat_setup(s, human_n, CHEAP, 3)
        out = _threaten(s, log)
        if out is None:
            return dict(shown=None, observed='the threat line was not reached (%d human units)' % human_n)
        r = out[0]
        terms.append((r['human_army'], r['partner_army'], r['army_term']))
    obs = 'no human unit: armies %d / %d, army_term %d; one unit: armies %d / %d, army_term %d' % (terms[0] + terms[1])
    if terms[0][:2] != (0, 30) or terms[1][:2] != (10, 30):
        return dict(shown=None, observed=obs + ' - the armies were not as planned')
    return dict(shown=terms[0][2] == -100 and terms[1][2] < terms[0][2], observed=obs)
