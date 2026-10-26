"""P4 B4 overland cases, batch 2 (N224): deterministic save edits read after one or more turns."""
import mom
from regress import case
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

N = mom.N


def _name(v):
    return bytes(x & 0xff for x in v).split(b'\0')[0].decode('latin-1')


@case(274, 'OV', 'a shrinking big city gets size 6',
      setup='AI city 21 Rastenford set to population 25 with population_tens -100 (it must shrink by one)',
      bug='population 24, size (24 + 3) / 4 = 6 - a size that does not exist', fixed='size capped at 5')
def c274(log):
    s = mom.SaveFile()
    assert _name(s.get('cities', 21, 'name')) == 'Rastenford'
    s.set('cities', 21, 'population', 25)
    s.set('cities', 21, 'size', 5)
    s.set('cities', 21, 'population_tens', -100)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    pop, size = lv.get('cities', 21, 'population'), lv.get('cities', 21, 'size')
    return dict(shown=size == 6, observed='turn %d: Rastenford population %d, size %d' % (t, pop, size))


@case(239, 'OV', "an AI's truce after peace runs at double speed",
      setup="wizards[1].peace_turns[0] = 10 (AI 1 toward the human), wizards[0].peace_turns[1] = 10",
      bug="after one turn the AI's counter is 8 (two decrements), the human's 9", fixed='both 9')
def c239(log):
    s = mom.SaveFile()
    s.set('wizards', 1, 'peace_turns[0]', 10)
    s.set('wizards', 0, 'peace_turns[1]', 10)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    a, h = lv.get('wizards', 1, 'peace_turns[0]'), lv.get('wizards', 0, 'peace_turns[1]')
    return dict(shown=a == 8 and h == 9, observed='turn %d: AI 1 peace_turns %d, human %d (both were 10)' % (t, a, h))


@case(242, 'OV', 'an AI that runs out of mana keeps the world frozen one more turn',
      setup='AI wizard 2 holds Time Stop (globals.time_stop = 1) with mana 0; the human cannot move under it, so turn '
            '106 ends by itself while the game loads',
      bug="AI_Drop_Bad_Globals clears Time Stop but not time_stop_player: turn 106 stays frozen - the human's gold "
          "does not move (987 -> 987: no income, no upkeep)", fixed="a normal turn: the human's gold 987 -> 1001")
def c242(log):
    s = mom.SaveFile()
    s.set('wizards', 2, 'globals.time_stop', 1)
    s.set('wizards', 2, 'mana_reserve', 0)
    g0 = s.get('wizards', 0, 'gold')
    lv = mom.boot(s)
    if lv.turn() == 106:
        mom.end_turn(lv, log=log)
    t = lv.turn()
    g1 = lv.get('wizards', 0, 'gold')
    ts2 = lv.get('wizards', 2, 'globals.time_stop')
    obs = ('turn 106 -> %d: human gold %d -> %d; wizard 2 time_stop now %d, time_stop_player %d'
           % (t, g0, g1, ts2, lv.glob('time_stop_player')))
    return dict(shown=t == 107 and g0 == g1 and ts2 == 0, observed=obs)


@case(482, 'OV', 'Nightshade: a city that stops qualifying keeps its count',
      setup='neutral city 5 Cremona (no Shrine / Temple / ... in its list) given a Nightshade count of 6 = its owner + 1 '
            '(so the self-dispel of #489 skips it)', bug='the count is still 6 after the turn', fixed='reset to 0')
def c482(log):
    s = mom.SaveFile()
    assert _name(s.get('cities', 5, 'name')) == 'Cremona' and s.get('cities', 5, 'owner') == 5
    s.set('cities', 5, 'enchants[25]', 6)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    v = lv.get('cities', 5, 'enchants[25]')
    return dict(shown=v == 6, observed='turn %d: Cremona Nightshade count %d (set to 6)' % (t, v))


@case(489, 'OV', 'Nightshade dispels its own count',
      setup='neutral city 5 Cremona (not counted by Cities_Count_Nightshade: no qualifying building) given a Nightshade '
            'count of 3; nothing else writes the slot', bug='City_Nightshade_Dispel rolls slot 25 too (as spell 0): the '
            'count is 0 after one turn', fixed='the count is not rolled against itself (stays 3 - then #482)')
def c489(log):
    s = mom.SaveFile()
    assert _name(s.get('cities', 5, 'name')) == 'Cremona'
    s.set('cities', 5, 'enchants[25]', 3)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    v = lv.get('cities', 5, 'enchants[25]')
    return dict(shown=v == 0, observed='turn %d: Cremona Nightshade count %d (set to 3)' % (t, v))


@case(178, 'OV', 'Plague can grow a size-1 town',
      setup='events: Plague running (state 2, 0 turns) on neutral city 26 Naples, population 1',
      bug='Naples set to population 2 when the Plague ends', fixed='population stays 1')
def c178(log):
    s = mom.SaveFile()
    assert _name(s.get('cities', 26, 'name')) == 'Naples' and s.get('cities', 26, 'population') == 1
    s.set('events', 0, 'plague', 2)
    s.set('events', 0, 'plague_city', 26)
    s.set('events', 0, 'plague_player', s.get('cities', 26, 'owner'))
    s.set('events', 0, 'plague_turns', 0)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    i = [k for k in range(lv.glob('num_cities')) if _name(lv.get('cities', k, 'name')) == 'Naples'][0]
    pop = lv.get('cities', i, 'population')
    pl = lv.get('events', 0, 'plague')
    return dict(shown=pop == 2 and pl == 0, observed='turn %d: Naples population %d, plague state %d' % (t, pop, pl))


@case(132, 'OV', 'ghost buildings: a completed building in state 2 is incremented',
      setup='human city 0 Dracon: Library (16) in state 2 (removed), producing the Library, progress 5000',
      bug='Library state 3 after completion', fixed='state 1 (built)')
def c132(log):
    s = mom.SaveFile()
    assert s.get('cities', 0, 'buildings[16]') == -1
    s.set('cities', 0, 'buildings[16]', 2)
    s.set('cities', 0, 'producing', 16)
    s.set('cities', 0, 'production_progress', 5000)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    v = lv.get('cities', 0, 'buildings[16]')
    return dict(shown=v == 3, observed='turn %d: Dracon Library state %d (was 2); producing now %d'
                % (t, v, lv.get('cities', 0, 'producing')))


@case(275, 'OV', 'a banished AI wizard loses its mana every turn',
      setup='AI wizard 4 (no Alchemy) casting Spell of Return (214, 5000 left), mana 1000, gold 400',
      bug='AI_Balance_Gold_Mana sets the mana to gold / 2 = 200 (and gold 0): far below 1000 after the turn',
      fixed='the gold is added to the mana')
def c275(log):
    s = mom.SaveFile()
    for f, v in (('casting_spell', 214), ('casting_cost_left', 5000), ('casting_cost_initial', 5000),
                 ('mana_reserve', 1000), ('gold', 400)):
        s.set('wizards', 4, f, v)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    m, g = lv.get('wizards', 4, 'mana_reserve'), lv.get('wizards', 4, 'gold')
    return dict(shown=m < 700, observed='turn %d: wizard 4 mana %d (was 1000), gold %d (was 400), casting %d'
                % (t, m, g, lv.get('wizards', 4, 'casting_spell')))


@case(226, 'OV', 'an eliminated wizard keeps casting',
      setup='wizard 3 is eliminated in this save (fortress inactive) and still has spell 166 in progress; given '
            'mana 1000 and 500 cost left', bug='its casting_cost_left goes down over the turn', fixed='no casting')
def c226(log):
    s = mom.SaveFile()
    assert s.get('fortresses', 3, 'active') == 0 and s.get('wizards', 3, 'casting_spell') == 166
    s.set('wizards', 3, 'mana_reserve', 1000)
    s.set('wizards', 3, 'casting_cost_left', 500)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    c, m = lv.get('wizards', 3, 'casting_cost_left'), lv.get('wizards', 3, 'mana_reserve')
    return dict(shown=c < 500, observed='turn %d: eliminated wizard 3 cost left %d (was 500), mana %d (was 1000), '
                'casting %d' % (t, c, m, lv.get('wizards', 3, 'casting_spell')))


def _terrstat(tile):
    import struct
    d = open(_mp.GAME + 'official_131/TERRSTAT.LBX', 'rb').read()
    o0 = struct.unpack_from('<I', d, 8)[0]
    cnt, sz = struct.unpack_from('<HH', d, o0)
    return tuple(d[o0 + 4 + tile * sz:o0 + 4 + tile * sz + 6])


@case(24, 'OV', 'new volcanoes never get a mineral (also #484: the movement costs are not updated)',
      setup='AI wizards 1, 2 and 4 hold Armageddon (3-5 volcanoes each a turn) with mana 5000; 6 turns (106 -> 112), '
            'then the autosave the game writes at turn 112 is read', bug='no new volcano has a mineral (a fixed build '
            'gives 1 in 5); the movement-cost maps keep the costs of the old terrain', fixed='minerals on about 1 in 5; '
            'the TERRSTAT costs of a volcano (record 179)', also=(484,))
def c24(log):
    s0 = mom.SaveFile()
    s = mom.SaveFile()
    for w in (1, 2, 4):
        s.set('wizards', w, 'globals.armageddon', 1)
        s.set('wizards', w, 'mana_reserve', 5000)
    lv = mom.boot(s)
    t = lv.turn()
    while t < 112:
        t = mom.end_turn(lv, log=log)
    import time
    time.sleep(3)
    a = mom.SaveFile(mom.GAME)
    assert a.get('game_turn') == 112, a.get('game_turn')
    sq, w = N('MAP_SQUARES'), N('MAP_WIDTH')
    vol = _terrstat(N('TILE_VOLCANO'))
    new, mineral, same, would_differ = [], 0, 0, 0
    for p in range(2):
        for k in range(sq):
            t0, t1 = s0.get('terrain', p * sq + k) % 0x2fa, a.get('terrain', p * sq + k) % 0x2fa
            if t1 == N('TILE_VOLCANO') and t0 != t1:
                new.append((p, k % w, k // w))
                if a.get('minerals', p * sq + k) & N('MINERAL_KIND_MASK'):
                    mineral += 1
                c0 = tuple(s0.get('move_costs', (p * 6 + m) * sq + k) for m in range(6))
                c1 = tuple(a.get('move_costs', (p * 6 + m) * sq + k) for m in range(6))
                same += c0 == c1
                would_differ += c0 != vol
    log.append('new volcanoes %s' % new)
    n = len(new)
    o24 = ('turn 112 autosave: %d new volcanoes, %d with a mineral (a fixed build: 1 in 5; no mineral in %d by chance '
           '%.1f %%)' % (n, mineral, n, 100 * 0.8 ** n))
    o484 = ('turn 112 autosave: %d new volcanoes, %d keep their old movement costs; TERRSTAT volcano costs %s differ '
            'from the old costs on %d of them' % (n, same, vol, would_differ))
    return dict(shown=n >= 15 and mineral == 0, observed=o24, also={484: (n > 0 and same == n and would_differ > 0, o484)})
