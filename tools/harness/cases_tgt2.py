"""P4 B4 cases where the human casts an overland spell at a square (as cases_tgt.py), N229."""
import struct
import time

import mom
from layout import L
from regress import case

N = mom.N


def _human_cast(s, spell, mana=5000):
    for f, v in (('casting_spell', spell), ('casting_cost_left', 0), ('casting_cost_initial', 500),
                 ('mana_reserve', mana)):
        s.set('wizards', 0, f, v)


def _answer(lv, n=6, keys=('y', 'enter')):
    for k in range(n):
        time.sleep(1.5)
        lv.D.key(keys[k % len(keys)])


def _report_bldgs(lv):
    n = lv.glob('report_buildings_lost_count')
    sz = L.size('ReportBldgLost')
    base = lv.g.ds + mom.SYM['report_buildings_lost'][0]
    return [(lv.g.ram(base + k * sz, 1)[0], struct.unpack('<h', lv.g.ram(base + k * sz + 2, 2))[0]) for k in range(n)]


@case(37, 'OV', 'Raise Volcano: the building-loss report records x for the city; the 2-slot nearby list overruns',
      setup='AI cities 1 (Tier) and 2 (Doraat) moved to (12,28) / (16,30) on Arcanus beside the human city Cornwell (3) '
            'at (14,29); the human has finished Raise Volcano (98) and targets Cornwell\'s square: three cities near '
            'the target (the list holds 2), Cornwell damaged', bug='report_buildings_lost entries name city 14 (the '
            'x coordinate), not 3', fixed='city 3')
def c37(log):
    s = mom.SaveFile()
    for c, (x, y) in ((1, (12, 28)), (2, (16, 30))):
        for f, v in (('x', x), ('y', y), ('plane', 0)):
            s.set('cities', c, f, v)
    _human_cast(s, N('SPELL_RAISE_VOLCANO'))
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    mom.target_square(lv, 14, 29, log=log)
    time.sleep(3)
    mom.shot('cast', log)
    rep = _report_bldgs(lv)
    _answer(lv)
    tile = lv.g.ram(lv.find_table('terrain', s, sig_at=4000) + (29 * 60 + 14) * 2, 2)
    obs = 'report_buildings_lost (city, building): %s; Cornwell square now terrain %d (volcano %d)' % (
        rep, struct.unpack('<H', tile)[0], N('TILE_VOLCANO'))
    return dict(shown=bool(rep) and all(c == 14 for c, _ in rep), observed=obs)


@case(131, 'OV', 'terrain re-tiling at the map seam writes the neighbour\'s blend tile one row off',
      setup='Arcanus (0,20) grassland (162), (59,20) desert (165), (59,19) and (59,21) ocean; the human has finished '
            'Change Terrain (15) and targets (0,20); Terrain_Retile_3x3 autotiles x - 1 = -1 unwrapped: the wrapped '
            'class of (59,20) is desert, its edge tile is written at 20 x 60 - 1 = (59,19)',
      bug='(59,19) - ocean - becomes a desert-edge land tile', fixed='(59,19) stays ocean')
def c131(log):
    s = mom.SaveFile()
    W = N('MAP_WIDTH')
    for (x, y), v in (((0, 20), N('TILE_GRASSLAND')), ((59, 20), N('TILE_DESERT')), ((59, 19), N('TILE_OCEAN')),
                      ((59, 21), N('TILE_OCEAN'))):   # Terrain_Autotile only blends desert / tundra / mountain /
                                                      # hills (10/23: an ocean square there changed nothing)
        s.set('terrain', y * W + x, value=v)
    _human_cast(s, N('SPELL_CHANGE_TERRAIN'))
    lv = mom.boot(s)
    base = lv.find_table('terrain', s, sig_at=4000)

    def tile(x, y):
        return struct.unpack('<H', lv.g.ram(base + (y * W + x) * 2, 2))[0]
    before = {xy: tile(*xy) for xy in ((0, 20), (59, 19), (59, 20), (59, 21))}
    mom.end_turn(lv, log=log)
    mom.target_square(lv, 0, 20, log=log)
    time.sleep(4)
    mom.shot('after_cast', log)
    _answer(lv, 3, ('enter',))
    after = {xy: tile(*xy) for xy in before}
    obs = 'terrain before -> after the cast: %s' % ', '.join('%s %d -> %d' % (xy, before[xy], after[xy])
                                                             for xy in before)
    land = N('TILE_LAND_FIRST')
    return dict(shown=before[(0, 20)] != after[(0, 20)] and before[(59, 19)] < land <= after[(59, 19)], observed=obs)
