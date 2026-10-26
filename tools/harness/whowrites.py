"""Who writes this global?  Memory-change breakpoints (BPM) on DGROUP offsets, from the baseline snapshot.

    python -P tools/harness/whowrites.py "P" 96D0 96F8          (action = harness keys, space separated; 'click:x,y')
    python -P tools/harness/whowrites.py "d d d" F9CBC+5E       (F<ds>+<delta>: the far pointer stored at DS:<ds> plus
                                                                 <delta> bytes - e.g. a field of record n of p_cities)

LOADSTATE work/harness/states/overland_start.sav, BPM DGROUP:off for each offset, RUN, do the action, and every time
the debugger stops: CS:IP (the instruction AFTER the write) -> Ghidra address -> containing function (functions.tsv),
plus the new value. RUN again; up to 12 stops. Stopped = CS:IP unchanged over three reads. Read only.
"""
import bisect
import struct
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
BASE = _mp.win(_mp.ROOT) + r'work\harness\states\overland_start.sav'


def functions():
    out = []
    for line in open(ROOT + 're/current_WIZARDS/functions.tsv', encoding='utf-8'):
        f = line.split('\t')
        if ':' in f[0] and f[0] != 'entry':
            s, o = f[0].split(':')
            out.append(((int(s, 16), int(o, 16)), f[1], int(f[2]) if f[2].isdigit() else 0))
    out.sort()
    return out


FN = None


def fn_at(g):
    global FN
    if FN is None:
        FN = functions()
    keys = [k for k, _, _ in FN]
    i = bisect.bisect_right(keys, g) - 1
    if i >= 0 and FN[i][0][0] == g[0]:
        (s, o), name, size = FN[i]
        return '%s+%X' % (name, g[1] - o) + ('' if g[1] - o < size + 16 else ' (past end?)')
    return '?'


def csip(ch):
    v = ch.ex('EV CS IP')[1][-1].split()
    return int(v[0], 16), int(v[1], 16)


def main(action, offs):
    ch = dbgchan.Chan()
    dosbox.start(dbgchan.DEBUG_CONF, exe=dosbox.EXE_NIGHTLY)
    out = []
    try:
        ch.accept()
        time.sleep(20)
        ch.req('BREAK')
        assert dbgchan.load_state(ch, BASE)[0]
        mem = dosbox.Mem()
        am = dbgchan.AddrMap(mem)
        ds = mem.ds_seg
        tg = []                                      # (seg, off) per target
        for o in offs:
            if isinstance(o, str):                   # 'F9CBC+5E': far pointer at DS:9CBC, then + 0x5E
                p, d = o[1:].split('+')
                off, seg = struct.unpack('<HH', mem.read(mem.ds + int(p, 16), 4))
                lin = seg * 16 + off + int(d, 16)
                tg.append((lin >> 4, lin & 15))
            else:
                tg.append((ds, o))
        for s_, o_ in tg:
            out.append('BPM %04X:%04X %s' % (s_, o_, ch.ex('BPM %04X:%04X' % (s_, o_))[0]))
        ch.ex('RUN')
        gl = dosbox.GameLink()
        for a in action.split():
            if a.startswith('click:'):
                x, y = a[6:].split(',')
                gl.click(int(x), int(y))
            else:
                dosbox.key(a if len(a) == 1 else a.lower())
            time.sleep(0.3)
        stops, t, last = 0, time.time(), None
        while time.time() - t < 8 and stops < 12:
            a = csip(ch)
            time.sleep(0.1)
            if a == csip(ch) == csip(ch) and a != last:
                am.refresh()
                g = am.to_ghidra(*a)
                vals = ' '.join('%04X:%04X=%s' % (s_, o_, mem.read(mem.base + s_ * 16 + o_, 2)[::-1].hex()) for s_, o_ in tg)
                out.append('stop %04X:%04X -> %s %s   %s' % (a[0], a[1], '%04X:%04X' % g if g else '(not mapped)',
                                                          fn_at(g) if g else '', vals))
                stops += 1
                last = a
                ch.ex('RUN')
                t = time.time()
    finally:
        try:
            ch.close()
        finally:
            dosbox.stop()
        print('\n'.join(out))


if __name__ == '__main__':
    main(sys.argv[1], [x if x.startswith('F') else int(x, 16) for x in sys.argv[2:]])
