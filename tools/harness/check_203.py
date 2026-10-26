"""GAME check of bug #203 (numpad moves at map column 0, Stack_Key_Move_Dir 4fb6:1aa0). Test save from the harness
copy: every human unit on patrol except the three Dragon Turtles (swimmers, 2 moves), each alone on Arcanus ocean at
x = 0 (the seam columns hold no continent land in any map we have, so a walker cannot stand there):
  unit 161 at (0, 20)  West        (numpad 4)          correct (59, 20); bug predicts (59, 19)
  unit 173 at (0, 25)  North-West  (numpad 7)          correct (59, 24); bug predicts a 2-tile path to (59, 23)
  unit 192 at (0, 30)  South-West  (numpad 1)          correct (59, 31); bug predicts (59, 30) (due west)
Each turtle is selected with W (Wait) until it is the active stack, then the key is pressed once; positions and moves
left are read from the live RAM.  python -P tools/harness/check_203.py [--control]  -> work/harness/check_203[_control].txt
(--control: the same three keys at x = 2, away from the seam.)"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D
import save_edit as E

DS_PUNITS, DS_STACK_N, DS_STACK = 0x9ec2, 0x999c, 0x999e
TESTS = [(161, (0, 20), 'kp4', 'West', (59, 20), (59, 19)),
         (173, (0, 25), 'kp7', 'North-West', (59, 24), (59, 23)),
         (192, (0, 30), 'kp1', 'South-West', (59, 31), (59, 30))]

if '--control' in sys.argv:          # the same keys at x = 2 (no seam): every move must go to the neighbour
    TESTS = [(i, (2, y), k, n, (1, ok[1]), (1, ok[1])) for i, (x, y), k, n, ok, bug in TESTS]
    TESTS = [(i, st, k, n, (1, {'kp4': 0, 'kp7': -1, 'kp1': 1}[k] + st[1]), None) for i, st, k, n, ok, bug in TESTS]
s = E.Save()
for i in s.units_of(0):
    s.set_unit(i, status=1, finished=1)
for i, (x, y), *_ in TESTS:
    s.set_unit(i, x=x, y=y, plane=0, status=0, finished=0, moves_left=s.unit(i)['moves_max'])
s.write()

D.stop()
time.sleep(1)
D.start()
time.sleep(10)
g = D.GameLink()
log = []


def unit(i):
    r = g.ram(g.far(DS_PUNITS) + i * 0x20, 0x20)
    return (r[0], r[1]), r[8]


def sel():
    return [g.dsw(DS_STACK + 4 * k) for k in range(g.dsw(DS_STACK_N))]


try:
    for i, start, k, name, right, bug in TESTS:
        for _ in range(6):
            if sel() == [i]:
                break
            D.key('w')
            time.sleep(1.5)
        before = unit(i)
        D.key(k)
        time.sleep(3)
        after = unit(i)
        D.shot('c203_%d%s' % (i, '_control' if '--control' in sys.argv else ''), big=True)
        verdict = 'BUG (as predicted)' if after[0] == bug else 'correct' if after[0] == right else 'other'
        log.append('unit %d selected %s: %s %s moves %d -> %s %s moves %d  [correct %s, bug %s] => %s'
                   % (i, sel(), name, before[0], before[1], k, after[0], after[1], right, bug, verdict))
finally:
    open(D.ROOT + 'work/harness/check_203%s.txt' % ('_control' if '--control' in sys.argv else ''), 'w').write('\n'.join(log) + '\n')
    print('\n'.join(log))
    E.restore()
