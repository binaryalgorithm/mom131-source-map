"""GAME check of bug #266 (any city-damage spell removes an outpost: City_Collateral_Damage 4a3f:1292 ends with
'population < 1 or size < 1 -> City_Remove'; Meteor_Storm_Turn b425:05ed calls it every turn on every unprotected city
not owned by the caster). Test save from the harness copy: the human (wizard 0) gets Meteor Storm (global enchantment
slot 0xd = 1). SAVE9 holds one outpost, Salamis (AI wizard 2, (4,15) Arcanus, population 0, size 0). Steps: Done on
every stack, Next Turn, answer the end-of-turn screens until the map is back; list the cities by name before / after.
  bug: Salamis is gone after one turn; control (--control, no Meteor Storm): Salamis still there.
  python -P tools/harness/check_266.py [--control]   -> work/harness/check_266[_control].txt"""
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D
import save_edit as E

DS_PCITIES, DS_NCITIES, DS_TURN, DS_NOUNITS, DS_SCREEN = 0x9cbc, 0xbd94, 0xbd90, 0x9444, 0x9990
WIZ0, GLOBAL_METEOR = 0x9e8, 0x482 + 0xd          # save file: wizard 0 record, global_enchantments[0xd]
ctl = '--control' in sys.argv
tag = '_control' if ctl else ''
log = []

s = E.Save()
if not ctl:
    s.b[WIZ0 + GLOBAL_METEOR] = 1
s.write()
D.stop()
time.sleep(1)
D.start()
time.sleep(10)
g = D.GameLink()


def cities():
    base = g.far(DS_PCITIES)
    out = {}
    for i in range(g.dsw(DS_NCITIES)):
        r = g.ram(base + i * 0x72, 0x72)
        out[r[:14].split(b'\0')[0].decode('latin1')] = (r[0x12], (r[0xf], r[0x10], r[0x11]), r[0x14], r[0x13])
    return out


try:
    time.sleep(2)
    turn = g.dsw(DS_TURN)
    meteor = g.ram(g.ds + 0x9eca + 0x482 + 0xd, 1)[0]
    before = cities()
    log.append('turn %d, human Meteor Storm byte %d, %d cities; Salamis %s' % (turn, meteor, len(before), before.get('Salamis')))
    for _ in range(30):                      # Done on every stack until the Next Turn state
        if g.dsw(DS_NOUNITS) == 1:
            break
        D.key('d')
        time.sleep(0.6)
    time.sleep(2)
    if g.dsw(DS_TURN) == turn and g.dsw(DS_SCREEN) == 5:
        D.key('n')                           # (this copy may already have ended the turn by itself)
    t0 = time.time()
    k = 0
    while time.time() - t0 < 240:            # wait for the new turn and the map screen, answering what comes up
        time.sleep(2.5)
        if g.dsw(DS_TURN) != turn and g.dsw(DS_SCREEN) == 5:
            break
        if g.dsw(DS_TURN) != turn:
            k += 1
            D.shot('c266%s_wait%d' % (tag, k), big=True)
            [lambda: D.key('esc'), lambda: g.click(255, 185), lambda: g.click(60, 33), lambda: D.key('enter')][k % 4]()
    time.sleep(2)
    after = cities()
    D.shot('c266%s_after' % tag, big=True)
    gone = sorted(set(before) - set(after))
    log.append('turn %d, %d cities; gone: %s; Salamis now %s' % (g.dsw(DS_TURN), len(after), gone, after.get('Salamis')))
    log.append('=> ' + ('Salamis REMOVED' if 'Salamis' in gone else 'Salamis still there'))
finally:
    open(D.ROOT + 'work/harness/check_266%s.txt' % tag, 'w').write('\n'.join(log) + '\n')
    print('\n'.join(log))
    E.restore()
