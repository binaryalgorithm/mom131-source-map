"""GAME check of bug #204 (numpad move while viewing the other plane: Stack_Key_Move_Dir 4fb6:1aa0 skips the plane test
for stacks without Planar Travel and Stack_Move_Click hands map_view_plane to Stack_Move). Test save from the harness
copy: every human unit on patrol except hero unit 19 in Dracon (kept ready so the turn does not end by itself) and
Halfling Bowmen (unit 107, no Planar Travel) alone on Arcanus. Bowmen selected (W skips the hero), the Plane button
switches the view to Myrror, then numpad 4 (West). Cases (start tile land on Arcanus):
  A (25,24): West tile water on Arcanus, land on Myrror  - correct: no move;   other plane's map used: walks onto water
  B (28,27): West tile land on Arcanus, water on Myrror  - correct: walks;     other plane's map used: no move
  C (27,29): West tile land on both planes (the first run: the Bowmen walked West on Arcanus and stayed there)
Each case is a fresh start; --control skips the plane switch. The Bowmen are found by type (91), not by index.
  python -P tools/harness/check_204.py [--control]   -> work/harness/check_204[_control].txt"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D
import save_edit as E

DS_PUNITS, DS_NUNITS, DS_STACK_N, DS_STACK, DS_VP, DS_TURN = 0x9ec2, 0xbd92, 0x999c, 0x999e, 0xbd86, 0xbd90
U, HERO = 107, 19
CASES = [('A', (25, 24)), ('B', (28, 27)), ('C', (27, 29))]
ctl = '--control' in sys.argv
tag = '_control' if ctl else ''
log = []


def bowmen(g):
    base = g.far(DS_PUNITS)
    for i in range(g.dsw(DS_NUNITS)):
        r = g.ram(base + i * 0x20, 0x20)
        if r[3] == 0 and r[5] == 91:
            return i, dict(xy=(r[0], r[1]), plane=r[2], moves=r[8], status=r[0xb])
    return None, None


def sel(g):
    return [g.dsw(DS_STACK + 4 * k) for k in range(g.dsw(DS_STACK_N))]


try:
    for name, start in CASES:
        s = E.Save()
        for i in s.units_of(0):
            s.set_unit(i, status=1, finished=1)
        s.set_unit(HERO, status=0, finished=0, moves_left=s.unit(HERO)['moves_max'])
        s.set_unit(U, x=start[0], y=start[1], plane=0, status=0, finished=0, moves_left=s.unit(U)['moves_max'])
        s.write()
        D.stop()
        time.sleep(1)
        D.start()
        time.sleep(10)
        g = D.GameLink()
        time.sleep(2)
        for _ in range(4):
            if sel(g) == [bowmen(g)[0]]:
                break
            D.key('w')
            time.sleep(1.5)
        i, b = bowmen(g)
        turn = g.dsw(DS_TURN)
        log.append('case %s start %s: selected %s (bowmen = unit %d) view plane %d %s'
                   % (name, start, sel(g), i, g.dsw(DS_VP), b))
        if not ctl:
            g.click(290, 9)                  # Plane button on the top bar
            time.sleep(3)
            log.append('  after Plane button: selected %s view plane %d' % (sel(g), g.dsw(DS_VP)))
        D.key('kp4')
        time.sleep(3)
        D.shot('c204%s_%s' % (tag, name), big=True)
        i, b = bowmen(g)
        log.append('  after numpad 4: turn %d (was %d) view plane %d bowmen %s' % (g.dsw(DS_TURN), turn, g.dsw(DS_VP), b))
        log.append('  => ' + ('changed plane' if b['plane'] != 0 else
                              'walked West on Arcanus' if b['xy'] != start else 'did not move'))
finally:
    open(D.ROOT + 'work/harness/check_204%s.txt' % tag, 'w').write('\n'.join(log) + '\n')
    print('\n'.join(log))
    E.restore()
