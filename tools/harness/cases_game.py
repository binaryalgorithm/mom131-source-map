"""P4 B4 GAME cases (N246): the standalone in-game checks of 10/17 (tools/harness/check_*.py, each with its own save
edits and DOSBox run) folded into the suite - a case runs the script and judges its printed lines."""
import os
import re
import subprocess
import sys

import mom
from regress import case

HERE = os.path.dirname(os.path.abspath(__file__))


def _run(script, *args, timeout=900):
    """Runs tools/harness/<script> (which writes / restores SAVE9.GAM and starts / stops DOSBox itself) -> stdout."""
    mom.lock()
    r = subprocess.run([sys.executable, '-P', os.path.join(HERE, script)] + list(args), capture_output=True,
                       text=True, timeout=timeout, cwd=mom.ROOT)
    return r.stdout + (('\nSTDERR: ' + r.stderr[-400:]) if r.returncode else '')


def _lines(out, pat):
    return [l.strip() for l in out.splitlines() if re.search(pat, l)]


@case(126, 'GAME', 'City_Recalculate stores city values as bytes: AI difficulty scaling past 255 wraps',
      setup='check_126.py: AI city Bremen made Dwarf, population 25, production buildings; loaded at difficulty 0 and '
            'at 4 (Impossible, x400%); the stored City bytes read after each load',
      bug='production / gold over 255 at Impossible stored mod 256 ("WRAPPED")', fixed='capped or kept as words')
def c126(log):
    out = _run('check_126.py')
    log.append(out)
    w = _lines(out, r'WRAPPED')
    rows = _lines(out, r'stored at Impossible')
    obs = '; '.join(w or rows[:3])
    if not rows:
        return dict(shown=None, observed='no result lines: ' + out[-300:])
    return dict(shown=bool(w), observed=obs)


@case(173, 'GAME', 'Chaos / Death Ward overwrite the moving stack\'s list entries with -1: the creature itself is '
                   'never updated, its Go-To never clears',
      setup='check_173.py: a Chaos creature (Hell Hounds, 155) on a Go-To into a Chaos-Warded Dracon; Done runs the '
            'Go-To, then W / D / N probe whether anything changes; the unit record and the 32 bytes before the unit '
            'array read each time. Control (check_173.py --sorcery): Sorcery Ward and a Sorcery creature (Phantom '
            'Beast, 191) - the ward code that does not touch the list. (10/24: the 32 bytes did not change in any run '
            '- the p_units[-1] accesses wrote what was there - so the verdict is the stuck creature, as the entry '
            'records)',
      bug='the Chaos creature keeps its Go-To (status 3, dest (29,27)) through every probe - the move code worked on '
          'p_units[-1]; the Sorcery control\'s Go-To is cleared', fixed='both Go-Tos cleared (the creature stopped)')
def c173(log):
    def states(out):
        return [(m.group(1), int(m.group(2)), tuple(int(x) for x in m.group(3).split(', ')))
                for m in re.finditer(r"^(after \w): turn \d+ .*?'status': (\d+), 'fin': \d+, 'dest': \(([\d, ]+)\)",
                                     out, re.M)]
    out = _run('check_173.py')
    log.append(out)
    ctl = _run('check_173.py', '--sorcery')
    log.append(ctl)
    g = _lines(out, r'^bytes before p_units')
    a, b = states(out), states(ctl)
    obs = ('Chaos Ward + Hell Hounds (probe, status, dest) %s; Sorcery Ward + Phantom Beast %s; the 32 bytes before '
           'p_units %s' % (a, b, 'unchanged' if len(g) >= 2 and g[0] == g[-1] else g))
    if len(a) < 4 or not b:
        return dict(shown=None, observed=obs + ' - probe lines missing: ' + (out + ctl)[-300:])
    if b[0][1] != 0:
        return dict(shown=None, observed=obs + ' - the control\'s Go-To was not cleared either (the move did not run)')
    stuck = all(st == mom.N('ORDER_GOTO') and dest == (29, 27) for _, st, dest in a)
    return dict(shown=stuck, observed=obs)


@case(203, 'GAME', 'numpad movement across the x = 0 seam goes one row off',
      setup='check_203.py: Dragon Turtles alone on the ocean at x = 0, numpad 4 / 7 / 1 pressed; the arrival square vs '
            'the correct and the predicted wrong one',
      bug='every move ends one row off ("BUG (as predicted)")', fixed='the correct squares')
def c203(log):
    out = _run('check_203.py')
    log.append(out)
    v = _lines(out, r'=> ')
    obs = '; '.join(v)
    if not v:
        return dict(shown=None, observed='no result lines: ' + out[-300:])
    return dict(shown=all('BUG (as predicted)' in l for l in v), observed=obs)


@case(204, 'GAME', 'numpad movement while viewing the other plane plans the path on that plane\'s map',
      setup='check_204.py: Halfling Bowmen (no Planar Travel) on Arcanus, the view switched to Myrror, numpad 4; '
            'case A: West water on Arcanus / land on Myrror, B: land on Arcanus / water on Myrror, C: land on both',
      bug='B does not move (the Myrror water blocks), the unit stays on Arcanus', fixed='B walks West')
def c204(log):
    out = _run('check_204.py')
    log.append(out)
    v = _lines(out, r'^=> |^\s*=> ')
    obs = '; '.join(v)
    if len(v) < 3:
        return dict(shown=None, observed='result lines missing: ' + out[-300:])
    return dict(shown='did not move' in v[1], observed=obs)


@case(224, 'GAME', 'the autosave is written mid turn change: loading it leaves human units without their moves',
      setup='check_224.py: a turn change to an autosave turn, SAVE9.GAM loaded; each human unit\'s moves / finished / '
            'order in the file vs live',
      bug='human units without orders start the turn with less than full moves', fixed='0 such units')
def c224(log):
    out = _run('check_224.py')
    log.append(out)
    m = re.search(r'(\d+) human unit\(s\) without orders start the turn with less than full moves', out)
    if not m:
        return dict(shown=None, observed='no result line: ' + out[-300:])
    return dict(shown=int(m.group(1)) > 0, observed=m.group(0))


@case(266, 'GAME', 'any city-damaging spell or event removes an outpost (population 0, size 0)',
      setup='check_266.py: the human\'s Meteor Storm active, wizard 2\'s Salamis made an outpost; two turns ended',
      bug='Salamis removed', fixed='the outpost survives')
def c266(log):
    out = _run('check_266.py')
    log.append(out)
    v = _lines(out, r'^=> ')
    if not v:
        return dict(shown=None, observed='no result line: ' + out[-300:])
    return dict(shown='REMOVED' in v[-1], observed='; '.join(_lines(out, r'gone:|^=> ')))


@case(352, 'GAME', 'MAGIC.EXE Place_Towers: the retry counter is never initialised, the spacing relaxes at once',
      setup='magic_towers.py: a New Game in MAGIC.EXE under the nightly DOSBox-X debugger (harness MAGIC.SET and save '
            'restored after); breakpoints in Place_Towers: the try counter [bp-8] at the first try, the relaxations '
            'and the towers placed',
      bug='the first try sees the counter at a stack leftover (> 500): the spacing relaxes from the first tower',
      fixed='the counter starts at 0')
def c352(log):
    out = _run('magic_towers.py', timeout=1500)
    log.append(out[-3000:])
    m = re.search(r'first try: counter \[bp-8\] = (-?\d+)', out)
    t = _lines(out, r'^towers ')
    if not m:
        return dict(shown=None, observed='no first-try line: ' + out[-300:])
    c = int(m.group(1))
    return dict(shown=c not in (0, 1), observed='first try: counter %d; %s' % (c, t[0] if t else ''))
