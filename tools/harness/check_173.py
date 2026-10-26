"""GAME check of bug #173 (City_Ward_Check 7dfd:07b9 overwrites a Chaos / Death creature's entry in the MOVING STACK's
unit list with -1; everything after it then works on p_units[-1]; predicted: the creature's Go-To is never cleared and
the next-unit loop retries it forever). Test save from the harness copy: every human unit on patrol except hero 19 in
Dracon (ready, selected first) and unit 107 turned into Hell Hounds (type 155, Chaos race 0x12) at (28,27) on Go-To
to Dracon (29,27); Dracon gets Chaos Ward (City +0x4e = owner + 1). Steps: D on the hero (the game then runs the
human's Go-Tos), wait, then probe: does the game still react (W / D / N advance anything)?
Also dumps the 32 bytes just before the unit array (p_units[-1]) before and after.
  --control: no Chaos Ward (the Hell Hounds simply walk in); --sorcery: Sorcery Ward + a Sorcery creature (type 191)
  python -P tools/harness/check_173.py [--control | --sorcery]   -> work/harness/check_173[_control].txt"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D
import save_edit as E

DS_PUNITS, DS_NUNITS, DS_STACK_N, DS_STACK, DS_TURN, DS_NOUNITS, DS_SCREEN = (0x9ec2, 0xbd92, 0x999c, 0x999e, 0xbd90,
                                                                             0x9444, 0x9990)
HOUNDS, HERO, START, CITY = 107, 19, (28, 27), (29, 27)
ctl = '--control' in sys.argv
sorc = '--sorcery' in sys.argv          # contrast: Sorcery Ward + a Sorcery creature (Phantom Beast, type 191)
tag = '_control' if ctl else '_sorcery' if sorc else ''
log = []

s = E.Save()
for i in s.units_of(0):
    s.set_unit(i, status=1, finished=1)
s.set_unit(HERO, status=0, finished=0, moves_left=s.unit(HERO)['moves_max'])
s.set_unit(HOUNDS, type=191 if sorc else 155, x=START[0], y=START[1], plane=0, moves_max=4, moves_left=4, status=3, finished=0,
           dest_x=CITY[0], dest_y=CITY[1])
dracon = [c for c in range(100) if s.b[E.CITY + c * 0x72:E.CITY + c * 0x72 + 6] == b'Dracon'][0]
if not ctl:
    s.b[E.CITY + dracon * 0x72 + (0x4d if sorc else 0x4e)] = 1     # Sorcery / Chaos Ward, cast by player 0
s.write()
D.stop()
time.sleep(1)
D.start()
time.sleep(10)
g = D.GameLink()


def hounds():
    r = g.ram(g.far(DS_PUNITS) + HOUNDS * 0x20, 0x20)
    return dict(xy=(r[0], r[1]), type=r[5], moves=r[8], status=r[0xb], fin=r[7], dest=(r[9], r[0xa]))


def guard():
    return g.ram(g.far(DS_PUNITS) - 0x20, 0x20).hex()


def tick():
    return int.from_bytes(g.ram(0x46c, 4), 'little')


def snap(label):
    log.append('%s: turn %d screen %d no_units %d selected %s hounds %s tick %d'
               % (label, g.dsw(DS_TURN), g.dsw(DS_SCREEN), g.dsw(DS_NOUNITS),
                  [g.dsw(DS_STACK + 4 * k) for k in range(min(9, g.dsw(DS_STACK_N)))], hounds(), tick()))


try:
    time.sleep(2)
    log.append('Dracon = city %d, sorcery_ward %d chaos_ward %d, unit %d type %d' % (dracon, s.b[E.CITY + dracon * 0x72 + 0x4d],
               s.b[E.CITY + dracon * 0x72 + 0x4e], HOUNDS, s.unit(HOUNDS)['type']))
    log.append('bytes before p_units: ' + guard())
    snap('start')
    D.key('d')                                  # hero done -> the game runs the human's Go-To orders
    time.sleep(6)
    snap('after D')
    D.shot('c173%s_1' % tag, big=True)
    for k in ('w', 'd', 'n'):                   # probe: does anything still change?
        D.key(k)
        time.sleep(6)
        snap('after %s' % k.upper())
    D.shot('c173%s_2' % tag, big=True)
    log.append('bytes before p_units: ' + guard())
finally:
    open(D.ROOT + 'work/harness/check_173%s.txt' % tag, 'w').write('\n'.join(log) + '\n')
    print('\n'.join(log))
    E.restore()
