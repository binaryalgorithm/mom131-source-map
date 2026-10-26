"""Map-generator survey: create N new games with MAGIC.EXE in the harness (hidden desktop) and measure how close land
comes to the x = 59 / 0 seam. Each run: MAGIC.SET land size set (harness copy), intro skipped, New Game, options OK,
Merlin, name Enter, Barbarian, green banner; when WIZARDS.EXE is up with turn 0 the world is read from the live RAM
(p_terrain_maps DS:0x9cdc) and the new SAVE9.GAM MAGIC.EXE wrote is kept as work/harness/mapgen/NNN_SIZE.GAM.
The harness MAGIC.SET and SAVE9.GAM are restored at the end (work/harness/backup/, work/harness/SAVE9_orig.GAM).
  python -P tools/harness/mapgen_survey.py N [small|medium|large ...] [diff=D] [sub=NAME]
      -> work/harness/mapgen/[NAME/]survey.tsv; diff = the new-game difficulty 0..4 (MAGIC.SET newgame_difficulty, 0x28;
      the default leaves the harness MAGIC.SET's); sub = a subfolder for the worlds (cases_mg reads only the top
      folder's 30 Intro worlds)"""
import hashlib
import os
import shutil
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D

GAME = D.ROOT + 'work/harness/game/'
OUT = D.ROOT + 'work/harness/mapgen/'
SIZES = {'small': 0, 'medium': 1, 'large': 2}
W = {int(l.split('\t')[0], 16) for l in open(D.ROOT + 'data/terrain_types.tsv').read().splitlines()[1:]
     if l.split('\t')[6] == '1'}
n = int(sys.argv[1])
sizes = [a for a in sys.argv[2:] if a in SIZES] or ['large']
DIFF = [int(a[5:]) for a in sys.argv[2:] if a.startswith('diff=')]
SUB = [a[4:] for a in sys.argv[2:] if a.startswith('sub=')]
if SUB:
    OUT = OUT + SUB[0] + '/'
os.makedirs(OUT, exist_ok=True)


def set_size(v):
    b = bytearray(open(GAME + 'MAGIC.SET', 'rb').read())
    b[0x2c:0x2e] = struct.pack('<H', v)
    if DIFF:
        b[0x28:0x2a] = struct.pack('<H', DIFF[0])
    open(GAME + 'MAGIC.SET', 'wb').write(b)


def stats(m):
    out = []
    for p in (0, 1):
        def land(x, y):
            return struct.unpack('<H', m[p * 0x12c0 + y * 0x78 + x * 2:][:2])[0] % 0x2fa not in W
        body = [(x, y) for x in range(60) for y in range(2, 38) if land(x, y)]
        dist = [min(x, 59 - x) for x, y in body]
        near = min(dist) if dist else -1
        hist = [dist.count(k) for k in range(6)]
        polar = [y for y in (1, 38) for x in (59, 0) if land(x, y)]
        out.append((len(body), near, hist, polar))
    return out


def run_one():
    D.stop()
    time.sleep(1.5)
    D.start(D.ROOT + 'tools/harness/magic.conf')
    time.sleep(10)
    D.key('esc')
    time.sleep(4)
    D.key('n')
    time.sleep(3)
    g = D.GameLink('magic')
    g.find_ds()
    for xy, wait in (((283, 186), 3), ((203, 34), 3)):
        g.click(*xy)
        time.sleep(wait)
    D.key('enter')
    time.sleep(3)
    g.click(240, 41)
    time.sleep(3)
    g.click(240, 38)
    t0 = time.time()
    while time.time() - t0 < 90:              # world generation, then WIZARDS.EXE starts
        time.sleep(2)
        try:
            w = D.GameLink()
            w.find_ds()
            if w.dsw(0xbd90) == 0 and w.far(0x9cdc) > 0x10000:
                time.sleep(2)
                return w.ram(w.far(0x9cdc), 0x12c0 * 2)
        except RuntimeError:
            pass
    return None


log = []
try:
    k = 0
    for i in range(n):
        for sz in sizes:
            set_size(SIZES[sz])
            m = run_one()
            if m is None:
                D.shot('mapgen_fail_%03d' % k, big=True)
                log.append('%03d\t%s\tFAILED' % (k, sz))
                print(log[-1], flush=True)
                k += 1
                continue
            name = '%03d_%s.GAM' % (k, sz)
            shutil.copy(GAME + 'SAVE9.GAM', OUT + name)
            st = stats(m)
            log.append('%03d\t%s\t%s\t' % (k, sz, hashlib.md5(m).hexdigest()[:8]) + '\t'.join(
                'p%d land %d nearest %d hist0-5 %s polar_seam %s' % (p, *s) for p, s in enumerate(st)))
            print(log[-1], flush=True)
            k += 1
finally:
    D.stop()
    shutil.copy(D.ROOT + 'work/harness/backup/MAGIC.SET', GAME + 'MAGIC.SET')
    shutil.copy(D.ROOT + 'work/harness/SAVE9_orig.GAM', GAME + 'SAVE9.GAM')
    with open(OUT + 'survey.tsv', 'a') as f:
        f.write('\n'.join(log) + '\n')
