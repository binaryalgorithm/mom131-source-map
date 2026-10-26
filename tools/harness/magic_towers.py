"""Place_Towers retry counter check (bug candidate: [bp-8] never initialised) - derived from magic_trace.py.
Runtime check of the MAGIC.EXE world generator against the static reading (work/reports/MAGIC_MAP.txt).

    python -P tools/harness/magic_trace.py      -> work/harness/dbg/magic_trace.txt

Nightly DOSBox-X + magic_debug.conf on the hidden desktop; New Game with the defaults (intro Esc, N, options OK,
Merlin, name Enter, Barbarian, green banner - the mapgen_survey.py clicks). Breakpoints:
  - World_Build's resident stub thunk (overlay 3 may not be loaded yet); on the hit, a breakpoint on the resident
    LBX_Load_Records catches World_Build's first call (BUILDDAT) with overlay 3 in memory -> AddrMap.refresh() ->
    breakpoints on every stage entry at its current runtime address.
  - Events_Reset's thunk = World_Build has returned: the finished world is read from RAM.
Gen_River is not traced: up to 2000 tries x 10 rivers x 2 planes = thousands of breakpoint round trips (the first
run timed out there). Every hit: the code bytes at CS:IP are compared with WIZARDS... MAGIC_FLAT.EXE (an overlay that moved would put a
breakpoint on foreign code), the plane argument is read from the stack, the time is noted, then STEPIN + RUN.
Checks at Gen_Height_To_Terrain (heights = land count, bounds), Place_Towers (nodes after balancing) and at the end
(towers, lairs, fortresses). Read only (BP / EV / MEMREAD / STEPIN / RUN). The harness MAGIC.SET and SAVE9.GAM are
restored afterwards (work/harness/backup/MAGIC.SET, work/harness/SAVE9_orig.GAM), as mapgen_survey.py does.
"""
import collections
import os
import shutil
import struct
import sys
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dbgchan  # noqa: E402
import dosbox as D  # noqa: E402

ROOT = _mp.ROOT
GAME = ROOT + 'work/harness/game/'
OUT = ROOT + 'work/harness/dbg/magic_towers.txt'
SNAP = _mp.win(_mp.ROOT) + r'work\harness\states\magic_worldbuild.sav'
MAP = ROOT + 'work/flat/MAGIC_FLAT_map.json'
DGROUP = 0x31D1
# MAGIC DS offsets (re/magic_data_names*.tsv)
P_LAIRS, P_TOWERS, P_FORTS, P_NODES, P_TERRAIN = 0x89d6, 0x89da, 0x89de, 0x89e2, 0x89f2
NG = {'difficulty': 0x87cc, 'magic': 0x87ce, 'land_size': 0x87d0, 'wizards': 0x87d2}
STAGES = ['Wizards_Setup', 'Landmass_Clear', 'Gen_Landmasses', 'Gen_Height_To_Terrain', 'Gen_Climate', 'Place_Nodes',
          'Balance_Node_Realms', 'Place_Towers', 'Gen_Islets_Around_Sites', 'Place_Lairs', 'Place_Fortresses',
          'Place_Neutral_Cities', 'Place_Minerals', 'Gen_Roads', 'Gen_Shorelines', 'Gen_Terrain_Edges',
          'Landmass_Fill_Shores', 'Gen_Terrain_Variety', 'Build_Movement_Maps', 'Build_Region_Bits', 'Explore_Start',
          'Gen_Ocean_Variety', 'Lairs_Record_Start_Guards']

flat = open(ROOT + 'work/flat/MAGIC_FLAT.EXE', 'rb').read()
FIMG = flat[struct.unpack_from('<H', flat, 8)[0] * 16:]
FN = {}
for line in open(ROOT + 're/current_MAGIC/functions.tsv', encoding='utf-8').read().splitlines()[1:]:
    f = line.split('\t')
    FN[f[1]] = tuple(int(x, 16) for x in f[0].split(':'))
log = []


def say(s):
    log.append(s)
    print(s, flush=True)
    with open(OUT, 'a') as f:                      # line by line: a killed run still leaves its log
        f.write(s + '\n')


def csip(ch):
    v = ch.ex('EV CS IP')[1][-1].split()
    return int(v[0], 16), int(v[1], 16)


def stopped_at(ch, targets, secs):
    """poll until CS:IP is one of targets and stays there; -> (cs, ip) or None"""
    t = time.time()
    while time.time() - t < secs:
        a = csip(ch)
        if a in targets:
            time.sleep(0.1)
            if csip(ch) == a:
                return a
        time.sleep(0.03)
    return None


def stack_words(ch, n):
    ss, sp = ch.ex('EV SS SP')[1][-1].split()
    hx = ch.ex('MEMREAD %s:%s %X' % (ss, sp, n * 2))[1][-1].split('HEX=')[1]
    b = bytes.fromhex(hx)
    return [b[i] | b[i + 1] << 8 for i in range(0, len(b), 2)]


def far(mem, ds_off):
    o, s = struct.unpack('<HH', mem.read(mem.base + mem.ds_seg * 16 + ds_off, 4))
    return s * 16 + o


def ram(mem, lin, n):
    return mem.read(mem.base + lin, n)


def dsw(mem, off):
    return struct.unpack('<h', mem.read(mem.base + mem.ds_seg * 16 + off, 2))[0]


def wrapdist(x1, y1, x2, y2):
    dx = abs(x1 - x2)
    dx = min(dx, 60 - dx)
    return max(dx, abs(y1 - y2))


def check_heights(mem, plane):
    t = ram(mem, far(mem, P_TERRAIN) + plane * 0x12c0, 0x12c0)
    h = [struct.unpack_from('<H', t, i * 2)[0] for i in range(2400)]
    land = [(i % 60, i // 60, v) for i, v in enumerate(h) if v]
    xs = [x for x, _, _ in land]
    ys = [y for _, y, _ in land]
    hist = collections.Counter(min(v, 9) for _, _, v in land)
    target = (360, 480, 720)[dsw(mem, NG['land_size'])]
    say('  heights plane %d: land %d (static: stops after > %d), x %d..%d, y %d..%d (static: inside x 2..57, y 4..35),'
        ' height histogram %s' % (plane, len(land), target, min(xs), max(xs), min(ys), max(ys), dict(sorted(hist.items()))))


def check_nodes(mem):
    n = ram(mem, far(mem, P_NODES), 30 * 0x30)
    nodes = [(n[i * 0x30], n[i * 0x30 + 1], n[i * 0x30 + 2], n[i * 0x30 + 4], n[i * 0x30 + 0x2d]) for i in range(30)]
    for p in (0, 1):
        ps = [x for x in nodes if x[2] == p]
        pw = [x[3] for x in ps]
        realms = collections.Counter({0: 'sorcery', 1: 'nature', 2: 'chaos'}.get(x[4], x[4]) for x in ps)
        md = min(wrapdist(a[0], a[1], b[0], b[1]) for i, a in enumerate(ps) for b in ps[i + 1:])
        say('  nodes plane %d: %d (static %d), power %d..%d (static %s), realms %s, min spacing %d (static >= 3)' %
            (p, len(ps), (16, 14)[p], min(pw), max(pw), ('5..10', '10..20')[p], dict(realms), md))


def check_end(mem):
    tw = ram(mem, far(mem, P_TOWERS), 6 * 4)
    towers = [(tw[i * 4], tw[i * 4 + 1]) for i in range(6)]
    md = min(wrapdist(*a, *b) for i, a in enumerate(towers) for b in towers[i + 1:])
    say('  towers %s; min spacing %d (static >= 10, relaxed after 500 tries)' % (towers, md))
    lr = ram(mem, far(mem, P_LAIRS), 102 * 0x18)
    lairs = [(lr[i * 0x18], lr[i * 0x18 + 1], lr[i * 0x18 + 2], lr[i * 0x18 + 3], lr[i * 0x18 + 4]) for i in range(102)]
    groups = {'0-11 towers': range(0, 12), '12-41 node guardians': range(12, 42), '42-66 normal': range(42, 67),
              '67-98 weak': range(67, 99), '99-101': range(99, 102)}
    for g, r in groups.items():
        act = [lairs[i] for i in r if lairs[i][3]]
        planes = collections.Counter(l[2] for l in act)
        say('  lairs %-22s active %2d of %2d, planes %s' % (g, len(act), len(r), dict(planes)))
    tow_ok = all(lairs[i][:2] == towers[i % 6] for i in range(12))
    say('  lair 0-11 positions = tower positions: %s; plane of 6-11 = %s' % (tow_ok, {lairs[i][2] for i in range(6, 12)}))
    nl = [l for l in lairs[42:99] if l[3]]
    md = min((wrapdist(a[0], a[1], b[0], b[1]) for i, a in enumerate(nl) for b in nl[i + 1:] if a[2] == b[2]), default=-1)
    say('  normal + weak lairs: min spacing on the same plane %d (static >= 2 from every earlier lair)' % md)
    fo = ram(mem, far(mem, P_FORTS), 6 * 4)
    nw = dsw(mem, NG['wizards'])
    forts = [(fo[i * 4], fo[i * 4 + 1], fo[i * 4 + 2], fo[i * 4 + 3]) for i in range(nw)]
    say('  fortresses (x, y, plane, active) for %d wizards: %s' % (nw, forts))


def frame_words(ch):
    ss, bp = ch.ex('EV SS BP')[1][-1].split()
    hx = ch.ex('MEMREAD %s:%04X 8' % (ss, (int(bp, 16) - 8) & 0xFFFF))[1][-1].split('HEX=')[1]
    b = bytes.fromhex(hx)
    return [struct.unpack_from('<h', b, i)[0] for i in range(0, 8, 2)]   # [bp-8], [bp-6], [bp-4], [bp-2]


def main():
    open(OUT, 'a').close()
    ch = dbgchan.Chan()
    D.start(ROOT + 'tools/harness/magic_debug.conf', exe=D.EXE_NIGHTLY)
    try:
        ch.accept()
        time.sleep(12)
        ch.req('BREAK')
        D.SIG_OFF = 0x280a
        mem = D.Mem()
        am = dbgchan.AddrMap(mem, DGROUP, MAP)
        wb = am.stub_thunk(*FN['World_Build'])
        if os.path.exists(SNAP):
            # snapshot taken at the World_Build thunk: same machine state, same RNG -> same world every load
            say('LOADSTATE %s: %s' % (SNAP, dbgchan.load_state(ch, SNAP)[0]))
            mem = D.Mem()
            am = dbgchan.AddrMap(mem, DGROUP, MAP)
            if csip(ch) != wb:
                raise RuntimeError('snapshot is not at the World_Build thunk: %s' % (csip(ch),))
        else:
            ch.ex('BP %04X %04X' % wb)
            ch.ex('RUN')
            D.key('esc'); time.sleep(5); D.key('n'); time.sleep(4)
            g = D.GameLink('magic'); g.find_ds()
            for xy in ((283, 186), (203, 34)):
                g.click(*xy); time.sleep(4)
            D.key('enter'); time.sleep(4); g.click(240, 41); time.sleep(4); g.click(240, 38)
            if not stopped_at(ch, {wb}, 120):
                raise RuntimeError('World_Build not reached')
            say('SAVESTATE %s: %s' % (SNAP, dbgchan.save_state(ch, SNAP)[0]))
        lbx = am.to_runtime(*FN['LBX_Load_Records'])
        ch.ex('BPDEL *'); ch.ex('BP %04X %04X' % lbx); ch.ex('RUN')
        if not stopped_at(ch, {lbx}, 60):
            raise RuntimeError('LBX_Load_Records not reached')
        ch.ex('BPDEL *')
        am.refresh()
        seg = am.loaded[0x4358]
        first, dec, done = (seg, 0x0727), (seg, 0x0736), am.to_runtime(*FN['Gen_Islets_Around_Sites'])
        for a in (first, dec, done):
            ch.ex('BP %04X %04X' % a)
        ch.ex('RUN')
        tries_seen, decs = False, 0
        while True:
            a = stopped_at(ch, {first, dec, done}, 120)
            if a is None:
                say('timeout'); break
            if a == done:
                break
            w = frame_words(ch)
            if a == first and not tries_seen:
                say('first try: counter [bp-8] = %d (never initialised), spacing [bp-6] = %d' % (w[0], w[1]))
                tries_seen = True
                ch.ex('BPDEL *')
                for b in (dec, done):
                    ch.ex('BP %04X %04X' % b)
            elif a == dec:
                decs += 1
                say('relax #%d: counter %d, spacing before dec %d' % (decs, w[0], w[1]))
            ch.ex('STEPIN'); ch.ex('RUN')
        tw = ram(mem, far(mem, P_TOWERS), 24)
        towers = [(tw[i * 4], tw[i * 4 + 1]) for i in range(6)]
        md = min(wrapdist(*p, *q) for i, p in enumerate(towers) for q in towers[i + 1:])
        say('towers %s, min spacing %d, relaxations %d' % (towers, md, decs))
        ch.ex('BPDEL *'); ch.ex('RUN')
    finally:
        try:
            ch.close()
        finally:
            D.stop(); time.sleep(1)
            shutil.copy(ROOT + 'work/harness/backup/MAGIC.SET', GAME + 'MAGIC.SET')
            shutil.copy(ROOT + 'work/harness/SAVE9_orig.GAM', GAME + 'SAVE9.GAM')


if __name__ == '__main__':
    main()
