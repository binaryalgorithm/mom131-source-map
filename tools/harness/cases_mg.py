"""P4 B4 map-generator cases (kind MG, N246): judged on the 30 worlds MAGIC.EXE generated in the harness on 10/17
(tools/harness/mapgen_survey.py: New Game with the defaults, small / medium / large land; the SAVE9.GAM it wrote kept
as work/harness/mapgen/NNN_SIZE.GAM). Reading those files needs no DOSBox; regenerate them with mapgen_survey.py to
test another build of MAGIC.EXE."""
import collections
import glob
import re

import mom
from regress import case

N = mom.N
WORLDS = sorted(glob.glob(mom.ROOT + 'work/harness/mapgen/*.GAM'))


def _unit_races():
    """UnitType index -> its race token, from the unit_types table in src/wizards/WIZARDS.C (198 rows)."""
    rows = []
    on = False
    for l in open(mom.ROOT + 'src/wizards/WIZARDS.C', encoding='latin-1'):
        if l.startswith('struct UnitType unit_types[198]'):
            on = True
            continue
        if on:
            if l.startswith('};'):
                break
            m = re.match(r'\s*\{ "[^"]*", (.*)', l)
            if m:
                rows.append([x.strip() for x in m.group(1).split(',')][10])
    return rows


def _worlds():
    return [(w.replace('\\', '/').rsplit('/', 1)[1], mom.SaveFile(w)) for w in WORLDS]


@case(357, 'MG', 'Mysterious Caves, Dungeons, Abandoned Keeps and Monster Lairs are guarded by Nature creatures only',
      setup='the 30 generated worlds: every lair of those four types, the race of its guard1 / guard2 unit types '
            '(UnitType.race); the other lair types for comparison',
      bug='all four types: Nature creatures only (Lair_Create\'s switch falls through)', fixed='the five realms mixed')
def c357(log):
    races = _unit_races()
    kinds = {N('LAIR_MYSTERIOUS_CAVE'), N('LAIR_DUNGEON'), N('LAIR_ABANDONED_KEEP'), N('LAIR_MONSTER_LAIR')}
    four, other = collections.Counter(), collections.Counter()
    for name, s in _worlds():
        for k in range(N('NUM_LAIRS')):
            t = s.get('lairs', k, 'type')
            if s.get('lairs', k, 'active') != 1:
                continue
            for g in ('guard1', 'guard2'):
                if s.get('lairs', k, g + '_count') & 0x0f:
                    r = races[s.get('lairs', k, g + '_type')]
                    (four if t in kinds else other)[r] += 1
    obs = ('guardian races in the four types %s; in the other lair types %s (30 worlds)'
           % (dict(four), dict(other)))
    if not four:
        return dict(shown=None, observed=obs + ' - no such lair found')
    return dict(shown=set(four) == {'RACE_NATURE_CREATURE'}, observed=obs)


@case(365, 'MG', 'the third tundra picture (terrain 0xb6) is never placed',
      setup='the 30 generated worlds: every terrain square counted by picture; the tundra pictures 0xb4 .. 0xb6',
      bug='0xb6 never appears (Random(3) - 1 gives 0..1 + 0xb4)', fixed='all three tundra pictures')
def c365(log):
    cnt = collections.Counter()
    sq = N('MAP_SQUARES') * 2
    for name, s in _worlds():
        o = mom.TABLES['terrain'][0]
        import struct
        for i in range(sq):
            v = struct.unpack_from('<H', s.b, o + 2 * i)[0] % 0x2fa
            if 0xb0 <= v <= 0xb8:
                cnt[v] += 1
    obs = 'pictures 0xb0 .. 0xb8 over the 30 worlds: %s' % {hex(k): cnt[k] for k in sorted(cnt)}
    if not (cnt[0xb4] or cnt[0xb5]):
        return dict(shown=None, observed=obs + ' - no tundra variant at all')
    return dict(shown=cnt[0xb6] == 0, observed=obs)


@case(370, 'MG', 'a wizard whose primary realm is Nature always gets Nature as the secondary realm',
      setup='the 30 generated worlds: each wizard\'s primary / secondary realm (Wizard.primary_realm / '
            'secondary_realm) and books',
      bug='primary Nature -> secondary Nature, whatever the other books', fixed='the next realm by books')
def c370(log):
    nat, rows = [], []
    for name, s in _worlds():
        for w in range(s.get('num_players')):
            p, q = s.get('wizards', w, 'primary_realm'), s.get('wizards', w, 'secondary_realm')
            books = [s.get('wizards', w, 'spellbooks[%d]' % r) for r in range(5)]
            rows.append((name, w, p, q))
            if p == N('REALM_NATURE'):
                nat.append((name, w, q, books))
    obs = ('wizards with primary Nature (world, wizard, secondary, books): %s; all wizards %d'
           % (nat[:8], len(rows)))
    others = [x for x in nat if sum(1 for r, b in enumerate(x[3]) if r != 0 and b > 0) > 0]
    if not others:
        return dict(shown=None, observed=obs + ' - no Nature-primary wizard with books in another realm')
    return dict(shown=all(x[2] == N('REALM_NATURE') for x in others), observed=obs)


@case(356, 'MG', 'Myrror\'s city squares never get the enchanted-road bit (0x10) at the start',
      setup='the 30 generated worlds: the terrain flags (save table terrain_flags) of every city square, by plane',
      bug='Myrror city squares: road bit 8 without 0x10 (the 0x10 store indexes the road loop\'s leftover x / y)',
      fixed='8 | 0x10 on Myrror')
def c356(log):
    sq = N('MAP_SQUARES')
    o = mom.TABLES['terrain_flags'][0]
    rows = collections.Counter()
    for name, s in _worlds():
        for c in range(s.get('num_cities')):
            x, y, p = (s.get('cities', c, f) for f in ('x', 'y', 'plane'))
            f = s.b[o + p * sq + y * N('MAP_WIDTH') + x]
            rows[(p, f & 0x18)] += 1
    obs = 'city squares (plane, flags & 0x18): %s over the 30 worlds' % dict(rows)
    myr = {k: v for k, v in rows.items() if k[0] == 1}
    if not myr:
        return dict(shown=None, observed=obs + ' - no Myrror city')
    return dict(shown=all(k[1] & 0x10 == 0 for k in myr), observed=obs)


@case(358, 'MG', 'too many Sorcery nodes are "balanced" into a fixed Sorcery 4 / Nature 6 / Chaos 6 split on Arcanus',
      setup='the 30 generated worlds: the realms of the Arcanus nodes counted per world',
      bug='many worlds end at exactly S4 N6 C6 (the surplus is never lowered: the loops fill Chaos and Nature to 6)',
      fixed='up to 9 Sorcery kept; S4 N6 C6 about once in 1,200 worlds by the dice')
def c358(log):
    splits = collections.Counter()
    for name, s in _worlds():
        c = collections.Counter(s.get('nodes', k, 'realm') for k in range(N('NUM_NODES'))
                                if s.get('nodes', k, 'plane') == 0)
        splits[(c[N('NODE_SORCERY')], c[N('NODE_NATURE')], c[N('NODE_CHAOS')])] += 1
    n466 = splits[(4, 6, 6)]
    obs = 'Arcanus node splits (Sorcery, Nature, Chaos): %s; exactly 4 / 6 / 6 in %d of %d worlds' % (
        dict(splits), n466, len(WORLDS))
    return dict(shown=n466 >= 3, observed=obs)


@case(366, 'MG', 'the ocean-variety pass runs one column (and row) past the map: column 0 rolls twice',
      setup='the 30 generated worlds: plain ocean (0) vs the ocean variant (0x259) counted in column 0 (rows 1..39, '
            'both planes: square [y][60] is [y + 1][0]) and in columns 1..58 (Myrror\'s top row, the other twice-rolled '
            'line, holds no ocean)',
      bug='about 36% variant in column 0 against about 20% elsewhere', fixed='about 20% everywhere')
def c366(log):
    import struct
    sq, W = N('MAP_SQUARES'), N('MAP_WIDTH')
    o = mom.TABLES['terrain'][0]
    col0, rest = collections.Counter(), collections.Counter()
    for name, s in _worlds():
        for p in (0, 1):
            for y in range(1, 40):
                for x in range(0, 59):
                    v = struct.unpack_from('<H', s.b, o + 2 * (p * sq + y * W + x))[0]
                    if v in (0, 0x259):
                        (col0 if x == 0 else rest)[v] += 1
    rc = col0[0x259] / max(1, col0[0] + col0[0x259])
    rr = rest[0x259] / max(1, rest[0] + rest[0x259])
    obs = ('variant share in column 0 %.1f%% (%d of %d ocean squares), in columns 1..58 %.1f%% (%d of %d)'
           % (100 * rc, col0[0x259], col0[0] + col0[0x259], 100 * rr, rest[0x259], rest[0] + rest[0x259]))
    if col0[0] + col0[0x259] < 200:
        return dict(shown=None, observed=obs + ' - too few squares')
    return dict(shown=rc > rr + 0.08, observed=obs)


def _hero_templates():
    """[(picks, pick_kind, base abilities)] per hero type, from src/magic/GAMESTRT.C hero_templates[35]."""
    out, on = [], False
    for l in open(mom.ROOT + 'src/magic/GAMESTRT.C', encoding='latin-1'):
        if l.startswith('struct HeroTemplate hero_templates[35]'):
            on = True
            continue
        if on:
            if l.startswith('};'):
                break
            m = re.match(r'\s*\{ (\d+), (\w+), ([^,]+), (\d+),', l)
            if m:
                ab = 0
                for tok in m.group(3).split('|'):
                    tok = tok.strip()
                    ab |= int(tok) if tok.isdigit() else N(tok)
                out.append((int(m.group(1)), N(m.group(2)), ab))
    return out


def _fisher_greater(a, b, c, d):
    """One-sided Fisher exact p for the 2 x 2 table [[a, b], [c, d]]: P(top-left >= a) with the margins fixed."""
    import math
    n, r1, c1 = a + b + c + d, a + b, a + c
    den = math.comb(n, c1)
    return sum(math.comb(r1, k) * math.comb(n - r1, c1 - k) for k in range(a, min(r1, c1) + 1)) / den


@case(364, 'MG', 'a hero\'s random Lucky roll falls through into the Agility roll (missing break)',
      setup='the 30 generated worlds: every hero type with fighter picks, for the 5 players (hero_stats[player * 35 + '
            'type].abilities against its template\'s base abilities, src/magic/GAMESTRT.C): heroes that rolled Lucky '
            '(0x08000000) and how many of them also rolled Agile / Super Agile, against the heroes without a Lucky '
            'roll; one-sided Fisher exact test pooled, and the Mantel-Haenszel test stratified per hero type (each '
            'template its own picks / pick kind / starting abilities; within a template a Lucky pick uses up a pick, so '
            'the fixed code leans AGAINST Agility with Lucky - a positive z is the fall-through)',
      bug='Agility far more often with Lucky (case 11 runs into case 12 while fighter picks remain)',
      fixed='Agility as often with or without Lucky')
def c364(log):
    tpl = _hero_templates()
    lucky, agile = N('HERO_ABILITY_LUCKY'), N('HERO_ABILITY_AGILE') | N('HERO_ABILITY_SUPER_AGILE')
    a = b = c = d = 0                           # lucky & agile, lucky only, agile only, neither
    per = collections.defaultdict(lambda: [0, 0, 0, 0])
    for name, s in _worlds():
        for p in range(5):
            for h, (picks, kind, base) in enumerate(tpl):
                if picks == 0 or kind == N('HERO_ABIL_PICK_MAGE'):
                    continue
                ab = s.get('hero_stats', p * 35 + h, 'abilities') & 0xffffffff
                lk = bool(ab & lucky and not base & lucky)
                ag = bool(ab & agile & ~base)
                cell = 0 if lk and ag else 1 if lk else 2 if ag else 3
                per[h][cell] += 1
                if lk and ag:
                    a += 1
                elif lk:
                    b += 1
                elif ag:
                    c += 1
                else:
                    d += 1
    if len(tpl) != 35 or a + b < 10:
        return dict(shown=None, observed='templates %d, Lucky rolls %d - too few' % (len(tpl), a + b))
    p = _fisher_greater(a, b, c, d)
    z, strata = _mantel_haenszel(per.values())
    obs = ('heroes with fighter picks in 30 worlds x 5 players: with a rolled Lucky %d, of them %d (%.0f%%) also rolled '
           'Agility; without Lucky %d, of them %d (%.1f%%); one-sided Fisher p = %.2g (pooled); Mantel-Haenszel per '
           'hero type z = %.2f over %d informative types; per type [lucky&agile, lucky, agile, neither] %s'
           % (a + b, a, 100.0 * a / (a + b), c + d, c, 100.0 * c / max(1, c + d), p, z, strata,
              {h: v for h, v in sorted(per.items()) if v[0] + v[1]}))
    return dict(shown=z > 3, observed=obs)


def _mantel_haenszel(tables):
    """(z, informative strata) of the Mantel-Haenszel test over 2 x 2 tables [a, b, c, d]: (sum a - sum E[a]) /
    sqrt(sum Var[a]), E and Var hypergeometric with each table's margins fixed."""
    import math
    num = var = 0.0
    k = 0
    for a, b, c, d in tables:
        n, r1, r2, c1, c2 = a + b + c + d, a + b, c + d, a + c, b + d
        if n < 2 or not r1 or not r2 or not c1 or not c2:
            continue
        k += 1
        num += a - r1 * c1 / n
        var += r1 * r2 * c1 * c2 / (n * n * (n - 1))
    return (num / math.sqrt(var) if var else 0.0), k


@case(412, 'MG', 'neutral Dark Elf towns get half their garrison (no melee type: every Dark Elf unit shoots)',
      setup='the 30 generated worlds: every neutral town (owner 5), its population and race, and the neutral units '
            'on its square; Place_Neutral_Cities creates population / 4 of the race\'s last melee type and as many of '
            'its last ranged type',
      bug='Dark Elf towns: population / 4 units (the ranged half only); other races 2 x (population / 4)',
      fixed='the full garrison for Dark Elves too')
def c412(log):
    de = N('RACE_DARK_ELF')
    rows = []                                   # (race, population, units on the square)
    for name, s in _worlds():
        at = collections.Counter()
        for u in range(s.get('num_units')):
            if s.get('units', u, 'owner') == 5:
                at[(s.get('units', u, 'x'), s.get('units', u, 'y'), s.get('units', u, 'plane'))] += 1
        for c in range(s.get('num_cities')):
            if s.get('cities', c, 'owner') != 5:
                continue
            xyp = tuple(s.get('cities', c, f) for f in ('x', 'y', 'plane'))
            rows.append((s.get('cities', c, 'race'), s.get('cities', c, 'population'), at[xyp]))
    dark = [r for r in rows if r[0] == de and r[1] >= 4]
    other = [r for r in rows if r[0] != de and r[1] >= 4]
    d_half = sum(1 for _, p, n in dark if n == p // 4)
    o_full = sum(1 for _, p, n in other if n == 2 * (p // 4))
    obs = ('neutral towns of population 4+ in 30 worlds: Dark Elf %d, of them %d with population / 4 units (e.g. %s); '
           'other races %d, of them %d with 2 x (population / 4)' % (len(dark), d_half, dark[:6], len(other), o_full))
    if not dark:
        return dict(shown=None, observed=obs + ' - no Dark Elf town')
    if o_full < len(other) * 0.8:
        return dict(shown=None, observed=obs + ' - the other races do not follow the 2 x rule either')
    return dict(shown=d_half == len(dark), observed=obs)


@case(359, 'MG', 'the "High Elf AI capitals need a forest" rule tests the wrong square (the last fortress site tried)',
      setup='the 30 generated worlds: every AI capital (the city on p_fortresses[1..4]), its race and its square\'s '
            'terrain (mod 762) against Tile_Is_Forest\'s three forest ids 163 / 183 / 184; the other races\' capitals '
            'show that a forest tile under a city survives the later passes. Capitals Place_Minerals can rewrite are '
            'left out (#355: its unwrapped sites x 60..64 / y 40..45 write row y + 1 at x 0..4, or plane 1\'s rows '
            '0..5, past its city test, and turn forest into mountain / hills / swamp)',
      bug='High Elf AI capitals on non-forest squares (the test looked at the 6th fortress site)',
      fixed='every High Elf AI capital on forest')
def c359(log):
    forest = {N('TILE_FOREST'), N('TILE_FOREST_2'), N('TILE_FOREST_3')}
    he, other, aliased = [], collections.Counter(), []
    for name, s in _worlds():
        for p in range(1, s.get('num_players')):
            xyp = tuple(s.get('fortresses', p, f) for f in ('x', 'y', 'plane'))
            for k in range(s.get('num_cities')):
                if tuple(s.get('cities', k, f) for f in ('x', 'y', 'plane')) != xyp:
                    continue
                t = s.tile(*xyp) % N('TERRAIN_IDS_PER_PLANE')
                if xyp[0] <= 4 or (xyp[2] == 1 and xyp[1] <= 5):
                    aliased.append((name[:3], p, xyp, t))
                elif s.get('cities', k, 'race') == N('RACE_HIGH_ELF'):
                    he.append((name[:3], p, t))
                else:
                    other[t in forest] += 1
    off = [h for h in he if h[2] not in forest]
    obs = ('High Elf AI capitals (world, player, terrain) %s: %d of %d not on forest; other AI capitals on forest %d, '
           'elsewhere %d; left out (Place_Minerals\' wrap can reach them) %s'
           % (he, len(off), len(he), other[True], other[False], aliased))
    if not he:
        return dict(shown=None, observed=obs + ' - no High Elf AI capital')
    if not other[True]:
        return dict(shown=None, observed=obs + ' - no capital on forest at all (the tile may be rewritten)')
    return dict(shown=bool(off), observed=obs)


WATER = {int(x) for x in open(mom.ROOT + 'tools/harness/water_tiles.txt').read().split()}   # water tile ids


def _water(t):
    """A water tile id (mod 762), by tools/harness/water_tiles.txt."""
    return t % N('TERRAIN_IDS_PER_PLANE') in WATER


@case(360, 'MG', 'the land added next to a node in open water only grows east, south (and south-west)',
      setup='the 30 generated worlds: every node whose ring at distance 2 is all water: which of its 8 neighbours '
            'are land. Judged on the 4 CARDINAL neighbours only - Gen_Islets_Around_Sites runs only where those four '
            'are water, so cardinal land is its islet, while a diagonal (e.g. north-west) may be land from before',
      bug='land only east (1, 0), south (0, 1) or south-west (-1, 1): the 3 x 3 index is computed as dy * 3 + dx',
      fixed='islets in every direction')
def c360(log):
    dirs, sites, with_land = collections.Counter(), 0, 0
    for name, s in _worlds():
        for k in range(N('NUM_NODES')):
            x, y, p = (s.get('nodes', k, f) for f in ('x', 'y', 'plane'))
            if not 2 <= y <= 37:
                continue
            ring = [(dx, dy) for dx in range(-2, 3) for dy in range(-2, 3) if max(abs(dx), abs(dy)) == 2]
            if not all(_water(s.tile((x + dx) % 60, y + dy, p)) for dx, dy in ring):
                continue
            sites += 1
            land = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                    if (dx, dy) != (0, 0) and not _water(s.tile((x + dx) % 60, y + dy, p))]
            with_land += bool(land)
            dirs.update(land)
    other = {d: n for d, n in dirs.items() if d not in ((1, 0), (0, 1), (-1, 1))}
    es, nw = dirs[(1, 0)] + dirs[(0, 1)], dirs[(-1, 0)] + dirs[(0, -1)]
    obs = ('open-water nodes %d, %d with land beside them; land squares by direction (dx, dy) %s; outside east / '
           'south / south-west %s; cardinal: east + south %d, west + north %d' % (sites, with_land, dict(dirs), other,
                                                                              es, nw))
    if es + nw < 20:
        return dict(shown=None, observed=obs + ' - too few islets')
    # fixed: the four cardinal directions about equally often; 1.31: west / north (nearly) never
    return dict(shown=nw <= 0.1 * es, observed=obs)


@case(355, 'MG', 'mineral sites ignore the map edge: the edge test is written with AND (never true)',
      setup='the 30 generated worlds: every square with a mineral (only Place_Minerals writes the mineral map) inside '
            'the band the edge test means to skip - y <= 2, y >= 38, x <= 2 or x >= 58 - on both planes',
      bug='minerals inside the band (nothing is skipped; sites also reach x 64 / y 45, wrapping into the next row or '
          'the other plane)', fixed='none in the band')
def c355(log):
    W, H = N('MAP_WIDTH'), N('MAP_HEIGHT')
    band, inner = [], 0
    for name, s in _worlds():
        for p in (0, 1):
            for y in range(H):
                for x in range(W):
                    v = s.get('minerals', p * W * H + y * W + x)
                    if not v:
                        continue
                    if y <= 2 or y >= H - 2 or x <= 2 or x >= W - 2:
                        band.append((name[:3], p, x, y, v))
                    else:
                        inner += 1
    obs = 'minerals in the edge band (world, plane, x, y, value) %s; elsewhere %d (30 worlds)' % (band, inner)
    if inner < 100:
        return dict(shown=None, observed=obs + ' - too few minerals')
    return dict(shown=bool(band), observed=obs)


@case(413, 'MG', 'the Myrror nodes test Arcanus\' land: they sit where ARCANUS has land',
      setup='the 30 generated worlds: for each Myrror node, whether the ARCANUS square with the same x, y is water; '
            'compared with Arcanus\' water share over the same window (x 3..56, y 2..36). Place_Nodes rejects a site '
            'whose p_terrain_maps[0] square is ocean (1 in 25 kept)',
      bug='Myrror nodes over Arcanus water far less often than Arcanus\' water share (an ocean site is kept 1 try in '
          '25: about 9% expected)',
      fixed='no link to Arcanus\' map (about Arcanus\' water share)')
def c413(log):
    on_water = total = sq_water = sq_total = 0
    for name, s in _worlds():
        for k in range(N('NUM_NODES')):
            if s.get('nodes', k, 'plane') != 1:
                continue
            x, y = s.get('nodes', k, 'x'), s.get('nodes', k, 'y')
            total += 1
            on_water += _water(s.tile(x, y, 0))
        for y in range(2, 37):
            for x in range(3, 57):
                sq_total += 1
                sq_water += _water(s.tile(x, y, 0))
    share = sq_water / max(1, sq_total)
    obs = ('Myrror nodes over Arcanus water %d of %d (%.1f%%); Arcanus water share in the window %.1f%%'
           % (on_water, total, 100.0 * on_water / max(1, total), 100 * share))
    if total < 100:
        return dict(shown=None, observed=obs + ' - too few nodes')
    return dict(shown=on_water / total < share / 2, observed=obs)


@case(354, 'MG', 'a PRESET wizard\'s start picks are read one entry late: one more spell per realm than a custom wizard',
      setup='the 30 generated worlds: the human is the preset Merlin (5 Life / 5 Nature books, mapgen_survey.py picks '
            'him): his KNOWN Life and Nature spells (library state 2) against default_spells (src/magic/NEWGAME.C). '
            'Wizard_Apply_Preset stores start_spells_common[books - 1] = 5 picks per realm; the custom spell screen '
            'leaves books - 1 = 4',
      bug='5 known commons in each realm (the effect the entry predicts)',
      fixed='4 (as a custom wizard with 5 books)')
def c354(log):
    rows = collections.Counter()
    for name, s in _worlds():
        name0 = bytes(s.b[s.addr('wizards', 0, 'wizard_name')[0]:][:7]).split(b'\0')[0]
        known = tuple(sum(1 for k in range(10) if s.get('wizards', 0, 'spell_library[%d][%d]' % (r, k)) == 2)
                      for r in (N('REALM_LIFE'), N('REALM_NATURE')))
        books = tuple(s.get('wizards', 0, 'spellbooks[%d]' % r) for r in (N('REALM_LIFE'), N('REALM_NATURE')))
        rows[(name0.decode('latin-1'), books, known)] += 1
    obs = ('the human per world (name, Life / Nature books, known Life / Nature commons): %s. Wizards_Init_Spell_Lists '
           '(src/magic/GAMESTRT.C) makes only the first (books - 1) start picks known below 11 books, so the extra '
           'pick Wizard_Apply_Preset stores never becomes a known spell; at 11 books every common is known anyway '
           '(the 10-book Ariel / Jafar / Freya / Tauron case not run)' % dict(rows))
    merlin = [(b, k) for (n, b, k), c in rows.items() if n == 'Merlin']
    if not merlin:
        return dict(shown=None, observed=obs + ' - no preset human')
    shown = any(k[0] == b[0] or k[1] == b[1] for b, k in merlin)
    if not shown and all(k == (b[0] - 1, b[1] - 1) for b, k in merlin):
        # 10/26: the late read is real but cannot reach play - Wizards_Init_Spell_Lists makes known only picks
        # 0 .. books - 2 below 11 books (the same first entries either way) and every common at 11 books (no count read)
        return dict(shown='latent', observed=obs + ' - latent: the extra pick is never made known for any book count')
    return dict(shown=shown, observed=obs)


def _presets():
    """[(name, retort)] of src/magic/NEWGAME.C wizard_presets[15]."""
    out, on = [], False
    for l in open(mom.ROOT + 'src/magic/NEWGAME.C', encoding='latin-1'):
        if l.startswith('struct WizardPreset wizard_presets[15]'):
            on = True
            continue
        if on:
            if l.startswith('};'):
                break
            m = re.match(r'\s*\{ "([^"]*)", [\d, ]+, (\w+) \}', l)
            if m:
                out.append((m.group(1), N(m.group(2))))
    return out


@case(363, 'MG', 'AI wizards practically never lose their preset\'s retort: the "clear the retorts" roll clears one byte',
      setup='worlds generated at Hard (mapgen_survey.py 30 ... diff=3 sub=hard -> work/harness/mapgen/hard/): every AI '
            'wizard\'s preset (portrait) and whether it still holds that preset\'s retort. On Hard each AI takes '
            'Random(3) + Random(3) reductions, each "clear the retorts" 1 time in 6: about half should lose the '
            'retort (E[(5/6)^n], n = 2..6: 49%); the top-up picks rarely add it back',
      bug='nearly all keep it (the loop clears retorts[k], k the reduction counter, not the 18 retorts)',
      fixed='about half keep it')
def c363(log):
    worlds = sorted(glob.glob(mom.ROOT + 'work/harness/mapgen/hard/*.GAM'))
    pre = _presets()
    kept = total = 0
    diffs = collections.Counter()
    for w in worlds:
        s = mom.SaveFile(w)
        diffs[s.get('difficulty')] += 1
        for p in range(1, s.get('num_players')):
            name, r = pre[s.get('wizards', p, 'portrait')]
            if r < 0:
                continue
            total += 1
            kept += s.get('wizards', p, 'retorts[%d]' % r) > 0
    obs = ('%d worlds (difficulty %s): AI wizards with a preset retort %d, still holding it %d (%.0f%%; the intended '
           'rule keeps about 49%% on Hard)' % (len(worlds), dict(diffs), total, kept, 100.0 * kept / max(1, total)))
    if total < 30 or set(diffs) - {3, 4}:
        return dict(shown=None, observed=obs + ' - too few Hard / Impossible worlds (generate them first)')
    return dict(shown=kept / total >= 0.85, observed=obs)


@case(362, 'MG', 'the Hard / Impossible AI\'s extra uncommon summon is guarded by the wrong spell\'s library byte',
      setup='worlds generated at Hard (work/harness/mapgen/hard/): every AI wizard whose primary realm is Life, Death, '
            'Chaos or Sorcery - the realm\'s uncommon summon (Unicorns / Shadow Demons / Chimeras / Phantom Beast) '
            'should be set researchable "if still 0"; the guard reads Nature\'s Basilisk byte (Life: Path Finding\'s) '
            'instead: the summon\'s library state and the guard byte read',
      bug='AIs whose summon is still 0 (not in the library) because the guard byte was not 0 (the AI had Nature '
          'spells), and / or a known summon (2) taken back to 1', fixed='the summon at least researchable for every '
          'such AI, a known one kept')
def c362(log):
    summon = {N('REALM_LIFE'): (N('SPELL_UNICORNS'), N('SPELL_PATH_FINDING')),
              N('REALM_DEATH'): (N('SPELL_SHADOW_DEMONS'), N('SPELL_BASILISK')),
              N('REALM_CHAOS'): (N('SPELL_CHIMERAS'), N('SPELL_BASILISK')),
              N('REALM_SORCERY'): (N('SPELL_PHANTOM_BEAST'), N('SPELL_BASILISK'))}
    worlds = sorted(glob.glob(mom.ROOT + 'work/harness/mapgen/hard/*.GAM'))

    def lib(s, p, sp):
        return s.get('wizards', p, 'spell_library[%d][%d]' % ((sp - 1) // 40, (sp - 1) % 40))
    rows = collections.Counter()
    missing = []
    for w in worlds:
        s = mom.SaveFile(w)
        if s.get('difficulty') < N('DIFFICULTY_HARD'):
            continue
        for p in range(1, s.get('num_players')):
            pr = s.get('wizards', p, 'primary_realm')
            if pr not in summon:
                continue
            sm, guard = summon[pr]
            st, g = lib(s, p, sm), lib(s, p, guard)
            rows[(st, g != 0)] += 1
            if st == 0:
                missing.append((w.replace('\\', '/').rsplit('/', 1)[1][:3], p, pr, g))
    obs = ('%d Hard worlds; AIs with a Life / Death / Chaos / Sorcery primary by (summon state, guard byte set): %s; '
           'summon not in the library (world, player, realm, guard byte): %s' % (len(worlds), dict(rows), missing[:8]))
    if sum(rows.values()) < 20:
        return dict(shown=None, observed=obs + ' - too few Hard worlds (generate them first)')
    if not missing:
        return dict(shown=None, observed=obs + ' - every such summon in the library (no evidence either way)')
    return dict(shown=all(g != 0 for *_, g in missing), observed=obs)
