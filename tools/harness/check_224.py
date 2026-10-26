"""GAME check of bug #224 (the autosave is written inside Next_Turn before the human's units get their moves back, and
loading runs Units_Reset_Moves_NonCurrent, which skips the human). Loads an autosave (default
work/harness/SAVE9_turn108_auto.GAM, written by the game at turn 108 during check_204_drown) as the harness copy's
SAVE9.GAM, starts WIZARDS.EXE (it loads slot 9) and reads every human unit from the live RAM: moves left, finished
flag, status, compared with the file. Bug: units saved 'done' (finished 1, moves 0) stay that way after loading.
  python -P tools/harness/check_224.py [SAVE]   -> work/harness/check_224.txt"""
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D
import save_edit as E

DS_PUNITS, DS_NUNITS, DS_STACK_N, DS_STACK, DS_TURN, DS_NOUNITS = 0x9ec2, 0xbd92, 0x999c, 0x999e, 0xbd90, 0x9444
save = sys.argv[1] if len(sys.argv) > 1 else D.ROOT + 'work/harness/SAVE9_turn108_auto.GAM'
f = E.Save(save)
shutil.copy(save, E.GAME)
D.stop()
time.sleep(1)
D.start()
time.sleep(12)
g = D.GameLink()
log = ['save %s' % save]
try:
    time.sleep(2)
    base = g.far(DS_PUNITS)
    log.append('turn %d, selected %s, no_units_to_move %d' % (g.dsw(DS_TURN),
               [g.dsw(DS_STACK + 4 * k) for k in range(g.dsw(DS_STACK_N))], g.dsw(DS_NOUNITS)))
    stuck = 0
    for i in f.units_of(0):
        u = f.unit(i)
        r = g.ram(base + i * 0x20, 0x20)
        live = dict(moves_left=r[8], finished=r[7], status=r[0xb])
        file_ = dict(moves_left=u['moves_left'], finished=u['finished'], status=u['status'])
        note = ''
        if u['status'] not in (1, 2, 6) and r[8] < u['moves_max']:
            note = '  <- not reset (moves %d of %d)' % (r[8], u['moves_max'])
            stuck += 1
        log.append('unit %3d type %3d file %s live %s%s' % (i, u['type'], file_, live, note))
    log.append('%d human unit(s) without orders start the turn with less than full moves' % stuck)
    D.shot('c224_loaded', big=True)
finally:
    open(D.ROOT + 'work/harness/check_224.txt', 'w').write('\n'.join(log) + '\n')
    print('\n'.join(log))
    E.restore()
