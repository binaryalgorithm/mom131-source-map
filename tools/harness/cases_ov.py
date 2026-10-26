"""P4 B4 overland cases (kind OV): save edit -> end turn(s) -> read the game. Each case's docstring-free metadata is
in its @case line; tools/harness/B4_SUITE.tsv has the plan for every entry."""
import mom
from regress import case


def _name(v):
    return bytes(x & 0xff for x in v).split(b'\0')[0].decode('latin-1')


def _city_by_name(lv, name):
    for i in range(lv.glob('num_cities')):
        if _name(lv.get('cities', i, 'name')) == name:
            return i
    return None


@case(2, 'OV', 'outpost removal: the shifted city falls into the outpost test',
      setup='city 9 Sunnydale made a dying outpost (population 0, size 0, tens -5); city 10 Zho (pop 4, tens 96) '
            'shifts into its slot when it is removed', bug='Zho ends with population 1, size 1',
      fixed='Zho grows normally (population 4 or 5)')
def c2(log):
    s = mom.SaveFile()
    assert _name(s.get('cities', 9, 'name')) == 'Sunnydale' and _name(s.get('cities', 10, 'name')) == 'Zho'
    s.set('cities', 9, 'population', 0)
    s.set('cities', 9, 'size', 0)
    s.set('cities', 9, 'population_tens', -5)
    log.append('Zho before: pop %d size %d tens %d' % (s.get('cities', 10, 'population'), s.get('cities', 10, 'size'),
                                                       s.get('cities', 10, 'population_tens')))
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    i = _city_by_name(lv, 'Zho')
    gone = _city_by_name(lv, 'Sunnydale') is None
    pop, size = lv.get('cities', i, 'population'), lv.get('cities', i, 'size')
    obs = 'turn %d: Sunnydale %s; Zho now index %s, population %d, size %d' % (t, 'removed' if gone else 'still there',
                                                                              i, pop, size)
    return dict(shown=gone and pop == 1 and size == 1, observed=obs)


@case(104, 'OV', 'growth past 100 tens: the excess is dropped',
      setup='human city 0 Dracon (13/20, growth +10 a turn per tools/check_growth.py) set to tens 95; AI city 20 Bremen '
            '(tens 96, growth +16) unedited', bug='Dracon 14 / tens 0, Bremen 13 / tens 0',
      fixed='Dracon tens 5, Bremen tens 12')
def c104(log):
    s = mom.SaveFile()
    s.set('cities', 0, 'population_tens', 95)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    d = (lv.get('cities', 0, 'population'), lv.get('cities', 0, 'population_tens'))
    b = _city_by_name(lv, 'Bremen')
    bb = (lv.get('cities', b, 'population'), lv.get('cities', b, 'population_tens'))
    obs = 'turn %d: Dracon pop %d tens %d (was 13 / 95); Bremen pop %d tens %d (was 12 / 96)' % ((t,) + d + bb)
    return dict(shown=d == (14, 0) and bb == (13, 0), observed=obs)


@case(96, 'OV', 'Time Stop: the Historian and the turn counter keep running (also #267)',
      setup='wizard 0 (human) gets Time Stop (globals.time_stop = 1; mana 656 covers the 200 upkeep)',
      bug='power_history shifts / game_turn + 1 under Time Stop', fixed='history frozen while time stands still',
      also=(267,))
def c96(log):
    s = mom.SaveFile()
    s.set('wizards', 0, 'globals.time_stop', 1)
    lv = mom.boot(s)
    h0 = lv.get('wizards', 0, 'power_history')
    t0 = lv.turn()
    tsp = lv.glob('time_stop_player')
    t = mom.end_turn(lv, log=log)
    h1 = lv.get('wizards', 0, 'power_history')
    changed = [k for k in range(len(h0)) if h0[k] != h1[k]]
    obs = 'time_stop_player %d; turn %d -> %d; power_history bytes changed: %d (first %s)' % (
        tsp, t0, t, len(changed), changed[:4])
    return dict(shown=tsp == 1 and t == t0 + 1 and len(changed) > 0, observed=obs)


@case(98, 'UI', 'opening the Cities list switches producing None to Housing',
      setup='human city 0 Dracon producing 0 (None)', bug='after the Cities screen Dracon produces 2 (Housing)',
      fixed='still 0')
def c98(log):
    s = mom.SaveFile()
    log.append('Dracon producing before edit: %d' % s.get('cities', 0, 'producing'))
    s.set('cities', 0, 'producing', 0)
    lv = mom.boot(s)
    p0 = lv.get('cities', 0, 'producing')
    lv.g.click(150, 9)                          # Cities (no hotkey)
    import time
    time.sleep(3)
    scr = lv.screen()
    p1 = lv.get('cities', 0, 'producing')
    lv.D.key('esc')
    time.sleep(2)
    obs = 'producing %d on the map; screen %d after the click; producing %d on the Cities list' % (p0, scr, p1)
    return dict(shown=p0 == 0 and p1 == 2, observed=obs)
