"""GAME check of bug #126 (City_Recalculate 9ef8:0000 stores each city value as a BYTE and scales AI cities by the
difficulty % into the same byte: 256+ wraps). Test save from the harness copy: AI city Bremen (wizard 4) enlarged -
race Dwarf (6 production per worker), population 25, 5 farmers, the four production buildings (BUILDDAT 15, 31, 33,
34 standing). Loaded twice: difficulty 0 (Intro, AI x100%: the stored value = the calculator's own result B) and
difficulty 4 (Impossible, production / gold / power / food x400%). The game recalculates every city on load
(Game_After_Load_Init -> Cities_Recalc_All_Farmers), so the stored bytes are read right after loading.
  bug: Impossible stores (4 x B) mod 256 where 4 x B > 255.   python -P tools/harness/check_126.py
  -> work/harness/check_126.txt"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D
import save_edit as E

DS_PCITIES, DS_NCITIES = 0x9cbc, 0xbd94
FIELDS = {'production': 0x5d, 'gold': 0x60, 'upkeep': 0x61, 'power': 0x62, 'research': 0x63, 'food': 0x64}
MULT = {0: dict(production=100, gold=100, power=100, research=100, food=100, upkeep=100),
        4: dict(production=400, gold=400, power=400, research=150, food=400, upkeep=30)}
log = []


def run(diff):
    s = E.Save()
    c = [i for i in range(100) if s.b[E.CITY + i * 0x72:E.CITY + i * 0x72 + 6] == b'Bremen'][0]
    o = E.CITY + c * 0x72
    s.b[o + 0x0e] = 4                        # Dwarf
    s.b[o + 0x14] = 25                       # population
    s.b[o + 0x15] = 5                        # farmers
    for b in (15, 31, 33, 34):
        s.b[o + 0x1f + b] = 1                # production buildings standing
    s.b[0x9de:0x9e0] = bytes([diff, 0])      # difficulty
    s.write()
    D.stop()
    time.sleep(1)
    D.start()
    time.sleep(12)
    g = D.GameLink()
    time.sleep(2)
    r = g.ram(g.far(DS_PCITIES) + c * 0x72, 0x72)
    vals = {k: r[v] for k, v in FIELDS.items()}
    log.append('difficulty %d: Bremen (city %d, owner %d, pop %d, farmers %d) stored %s'
               % (diff, c, r[0x12], r[0x14], r[0x15], vals))
    return vals


try:
    base = run(0)
    imp = run(4)
    for k in FIELDS:
        want = base[k] * MULT[4][k] // 100
        log.append('%-10s base %3d  x%d%% = %4d  -> stored at Impossible %3d  %s'
                   % (k, base[k], MULT[4][k], want, imp[k],
                      'WRAPPED (%d mod 256 = %d)' % (want, want % 256) if want > 255 and imp[k] == want % 256 else
                      'ok' if imp[k] == want else 'differs'))
finally:
    open(D.ROOT + 'work/harness/check_126.txt', 'w').write('\n'.join(log) + '\n')
    print('\n'.join(log))
    E.restore()
