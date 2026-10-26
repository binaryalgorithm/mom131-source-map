"""Dead-code check by execution: breakpoints on the entry of every function GRAPHICS_ENGINE.txt calls dead, plus live
controls, then a tour of the main screens. A function whose breakpoint is hit is NOT dead.

    python -P tools/harness/deadcode_bp.py            (nightly DOSBox-X, hidden desktop, ~3 minutes)

Hit detection: the debugger stops on a breakpoint and stays there; we poll CS:IP over the channel (ONE 'EV CS IP' -
two separate EVs while running can pair a CS and an IP from different moments: a false hit seen 2026-10-17), confirm
that it is still there 0.15 s later, record the hit, delete that breakpoint and RUN. Read only (BP / EV / RUN / SAVESTATE only). Results -> work/harness/dbg/deadcode_bp.txt
"""
import os
import sys
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dbgchan  # noqa: E402
import dosbox   # noqa: E402

ROOT = _mp.ROOT
OUT = ROOT + 'work/harness/dbg/deadcode_bp.txt'
BASE_SAV = _mp.win(_mp.ROOT) + r'work\harness\states\overland_start.sav'

DEAD = ['Transition_Interlace_Push', 'Transition_Interlace_Band_Push', 'Transition_Push_Left', 'Transition_Push_Right',
        'Transition_Blinds', 'Transition_Blocks', 'VGA_Reveal_Line',
        'VGA_Interlace_Shift', 'VGA_Interlace_Fill_Edges', 'VGA_Interlace_Shift_Band', 'VGA_Interlace_Fill_Band_Edges', 'VGA_Push_Left_Step',
        'VGA_Push_Right_Step', 'VGA_Present_Dissolve_Slow', 'VGA_Present_Wipe_Centre', 'VGA_Present_Wipe_LR',
        'VGA_Load_Planar_Image', 'Pic_Draw_Along_Line', 'Pic_Recolor_By_Brightness_All',
        'Pic_Recolor_By_Brightness_Range', 'Pic_Recolor_By_Brightness', 'Pic_Keep_Rect', 'Pic_Add_Rect',
        'Pic_Replace_Color_Rect', 'Pic_Draw_Rect_Outline', 'Palette_Gradient_Range', 'Palette_Rotate',
        'Pic_Remap_Shade2']
LIVE = ['Screen_Present', 'Palette_Fade_To_Black', 'Palette_Fade_From_Black', 'VGA_Present_Dissolve']

TOUR = [('S', 'spellbook'), ('A', 'armies'), ('click', 150, 9, 'cities'), ('M', 'magic'), ('I', 'info menu'),
        ('f1', 'info F1'), ('f2', 'info F2'), ('f3', 'info F3'), ('f4', 'info F4'), ('P', 'plane'),
        ('P', 'plane back'), ('G', 'game menu'), ('N', 'next turn'), ('N', 'next turn 2')]


def names():
    d = {}
    for line in open(ROOT + 're/names.tsv', encoding='utf-8'):
        f = line.split('\t')
        if len(f) > 1:
            s, o = f[0].split(':')
            d[f[1]] = (int(s, 16), int(o, 16))
    return d


class Run:
    def __init__(self, ch, am):
        self.ch, self.am = ch, am
        self.bps = {}            # runtime (seg, off) -> name
        self.hits = []

    def add(self, name, g):
        r = self.am.to_runtime(*g)
        if r is None:
            return False
        ok, out = self.ch.ex('BP %04X %04X' % r)
        if ok:
            self.bps[r] = name
        return ok

    def csip(self):
        v = self.ch.ex('EV CS IP')[1][-1].split()
        return int(v[0], 16), int(v[1], 16)

    def poll(self, secs, where):
        t = time.time()
        while time.time() - t < secs:
            cs, ip = self.csip()                       # one EV: CS and IP from the same moment
            if (cs, ip) in self.bps:
                time.sleep(0.15)
                if self.csip() != (cs, ip):             # still there = really stopped (not a passing sample)
                    continue
                name = self.bps.pop((cs, ip))
                self.hits.append((name, where, '%04X:%04X' % (cs, ip)))
                for line in self.ch.ex('BPLIST')[1]:
                    if line.endswith('%04X:%04X' % (cs, ip)):
                        self.ch.ex('BPDEL %s' % line.split('.')[0].strip())
                self.ch.ex('RUN')
            time.sleep(0.05)


def main():
    nm = names()
    ch = dbgchan.Chan()
    st = dosbox.start(dbgchan.DEBUG_CONF, exe=dosbox.EXE_NIGHTLY)
    log = []
    try:
        ch.accept()
        time.sleep(25)
        ch.req('BREAK')
        am = dbgchan.AddrMap()
        if not os.path.exists(BASE_SAV):
            log.append('baseline state: %s %s' % (BASE_SAV, dbgchan.save_state(ch, BASE_SAV)[0]))
        run = Run(ch, am)
        for n in DEAD + LIVE:
            log.append('BP %-34s %s' % (n, 'set' if run.add(n, nm[n]) else 'NOT SET'))
        ch.ex('RUN')
        gl = dosbox.GameLink()
        run.poll(2, 'start')
        for step in TOUR:
            if step[0] == 'click':
                gl.click(step[1], step[2])
            else:
                dosbox.key(step[0].lower() if len(step[0]) > 1 else step[0])
            where = step[-1]
            run.poll(3, where)
            dosbox.shot('dc_' + where.replace(' ', '_'))
            if where != 'plane':                                # back to the map; the shot shows each screen
                dosbox.key('esc')
                run.poll(1.5, where + ' (esc)')
            log.append('step %s' % where)
        log.append('')
        log.append('HITS (function ran -> not dead):')
        log += ['  %-34s during %-16s at %s' % h for h in run.hits]
        log.append('NOT HIT: ' + ', '.join(sorted(run.bps.values())))
    finally:
        try:
            ch.close()
        finally:
            dosbox.stop()
        open(OUT, 'w').write('\n'.join(log) + '\n')
        print('\n'.join(log))


if __name__ == '__main__':
    main()
