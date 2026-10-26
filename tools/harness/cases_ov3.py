"""P4 B4 overland cases with breakpoints (dbg.py) and EMS-held maps (Live.find_table), N229."""
import dbg
import mom
from cases_ov import _city_by_name, _name
from regress import case

N = mom.N
CORNWELL = 3                                    # human city at (14,29) plane 0, race 6
W = N('MAP_WIDTH')


def _built(s, city, *blds):
    for b in blds:
        s.set('cities', city, 'buildings[%d]' % N(b), N('BLDSTATE_BUILT'))


def _city_area(x, y):
    return [(x + dx, y + dy) for dy in range(-2, 3) for dx in range(-2, 3) if abs(dx) + abs(dy) < 4]


@case(103, 'OV', 'growth stops only at EXACTLY the maximum: a city above it keeps growing',
      setup='Cornwell (3): its 21 area squares swamp, minerals cleared (food cap 0, shown by #272), Granary and '
            "Farmers' Market built: maximum population 0 + 2 + 3 = 5; population set to 8, all farmers (food 16 >= "
            '8), population_tens 0; one turn change', bug='population_tens rises (a positive growth rate above the '
            'maximum)', fixed='no growth (or shrinking) above the maximum')
def c103(log):
    s = mom.SaveFile()
    cx, cy = s.get('cities', CORNWELL, 'x'), s.get('cities', CORNWELL, 'y')
    for x, y in _city_area(cx, cy):
        s.set('terrain', y * W + x, value=N('TILE_SWAMP'))
        s.set('minerals', y * W + x, value=0)
    _built(s, CORNWELL, 'BLD_GRANARY', 'BLD_FARMERS_MARKET')
    for f, v in (('population', 8), ('farmers', 8), ('population_tens', 0)):
        s.set('cities', CORNWELL, f, v)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    pop, tens = lv.get('cities', CORNWELL, 'population'), lv.get('cities', CORNWELL, 'population_tens')
    food = lv.get('cities', CORNWELL, 'food_income')
    mom.shot('map', log)
    obs = 'turn %d: Cornwell (maximum 5) population 8 -> %d, tens 0 -> %d, food %d' % (t, pop, tens, food)
    return dict(shown=pop >= 8 and (tens > 0 or pop > 8), observed=obs)


@case(272, 'OV', 'swamps give no food',
      setup='the 21 squares of Cornwell\'s (3) area made swamp (166), minerals cleared; breakpoint on '
            'City_Terrain_Food_Cap(3) (the area food / 4) during the turn change',
      bug='0', fixed='21 x 2 quarter units / 4 = 10 (1/2 food per swamp, the manual)')
def c272(log):
    s = mom.SaveFile()
    cx, cy = s.get('cities', CORNWELL, 'x'), s.get('cities', CORNWELL, 'y')
    for x, y in _city_area(cx, cy):
        s.set("terrain", y * W + x, value=N("TILE_SWAMP"))
        s.set("minerals", y * W + x, value=0)
    d = dbg.boot(s, log=log)
    try:
        args, cap = d.call('City_Terrain_Food_Cap', lambda a: a[0] == CORNWELL, 1, poll=d.end_turn_poll())
    finally:
        d.stop()
    obs = 'Cornwell area all swamp: City_Terrain_Food_Cap = %s' % cap
    return dict(shown=cap == 0, observed=obs)


@case(125, 'OV', 'a removed city leaves its square at cost 2 on every land movement map',
      setup='as #2 (city 9 Sunnydale a dying outpost), its square first given a mountain\'s terrain and movement '
            'costs (copied from a mountain square of the save); the costs read live from the EMS-held map after the '
            'turn change', bug='modes 0-4 cost 2', fixed='the mountain\'s costs')
def c125(log):
    s = mom.SaveFile()
    sx, sy = s.get('cities', 9, 'x'), s.get('cities', 9, 'y')
    mc = mom.TABLES['move_costs'][0]

    def costs(b, x, y, at=0):
        return [b[at + (m * 40 + y) * W + x] for m in range(6)]
    src = next((x, y) for y in range(2, 38) for x in range(W)
               if s.tile(x, y, 0) == N('TILE_MOUNTAIN') and costs(s.b, x, y, mc)[1] > 2)
    mcost = costs(s.b, *src, at=mc)
    s.set("terrain", sy * W + sx, value=N("TILE_MOUNTAIN"))
    for m in range(6):
        s.b[mc + (m * 40 + sy) * W + sx] = mcost[m]
    log.append('mountain square %s costs %s copied to Sunnydale (%d,%d)' % (src, mcost, sx, sy))
    s.set('cities', 9, 'population', 0)
    s.set('cities', 9, 'size', 0)
    s.set('cities', 9, 'population_tens', -5)
    lv = mom.boot(s)
    base = lv.find_table('move_costs', s, sig_at=(3 * 40 + 20) * W, sig_len=600)
    before = costs(lv.g.ram(base, 6 * 40 * W), sx, sy)
    t = mom.end_turn(lv, log=log)
    gone = _city_by_name(lv, 'Sunnydale') is None
    after = costs(lv.g.ram(base, 6 * 40 * W), sx, sy)
    obs = 'turn %d: Sunnydale %s; its square\'s costs %s -> %s (mountain %s)' % (
        t, 'removed' if gone else 'NOT removed', before, after, mcost)
    return dict(shown=gone and after[:5] == [2] * 5 and before[:5] != [2] * 5, observed=obs)


def _area_bits(cities, plane, wrap):
    """City_Area_Bitmaps_Rebuild's city_area_bits for one plane (bit y * 60 + x, LSB first) - 1.31 clips x at the
    seam (wrap=False); City_Area_Tiles wraps it (wrap=True)."""
    b = bytearray(N('MAP_SQUARES') // 8)
    for x, y, p in cities:
        if p != plane:
            continue
        for dy in range(-2, 3):
            for dx in range(-2, 3):
                if abs(dx) == 2 and abs(dy) == 2:
                    continue
                tx, ty = x + dx, y + dy
                if wrap:
                    tx %= W
                if 0 <= ty < 40 and 0 <= tx < W:
                    bit = ty * W + tx
                    b[bit >> 3] |= 1 << (bit & 7)
    return bytes(b)


@case(27, 'OV', 'the city-area bitmap does not wrap at the x = 0 / 59 seam',
      setup='Cornwell (3) moved to (1,29); after loading, the 300-byte city_area_bits map of Arcanus searched in the '
            'emulated memory (EMS included) in two versions computed from the cities: x clipped at the seam (as '
            'City_Area_Bitmaps_Rebuild does) and x wrapped (as City_Area_Tiles, the city\'s own food area, does)',
      bug='the clipped version is in memory: squares x 58 / 59 of Cornwell\'s area are not marked',
      fixed='the wrapped version')
def c27(log):
    s = mom.SaveFile()
    s.set('cities', 3, 'x', 1)
    cities = [(s.get('cities', c, 'x'), s.get('cities', c, 'y'), s.get('cities', c, 'plane'))
              for c in range(s.get('num_cities'))]
    clip, wrap = _area_bits(cities, 0, False), _area_bits(cities, 0, True)
    lv = mom.boot(s)
    import dosbox as D
    m = lv.g.m
    found = {k: m.find(v, D.GL_CORE) for k, v in (('clipped', clip), ('wrapped', wrap))}
    mom.shot('map', log)
    obs = 'Arcanus city_area_bits in memory: clipped version %s, wrapped version %s (they differ in %d bytes)' % (
        'FOUND' if found['clipped'] >= 0 else 'not found', 'FOUND' if found['wrapped'] >= 0 else 'not found',
        sum(1 for a, b in zip(clip, wrap) if a != b))
    return dict(shown=found['clipped'] >= 0 and found['wrapped'] < 0, observed=obs)
