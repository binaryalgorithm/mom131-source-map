"""State-vs-state RAM diff for the running game (read only).

    snap = memdiff.snapshot(mem)          # all emulated RAM (mom.conf memsize=16 -> 16 MB), or snapshot(mem, CONV)
    ... one small action (a key, a click, one turn) ...
    for line in memdiff.report(mem, snap, memdiff.snapshot(mem)): print(line)

Changed bytes are grouped into runs; runs inside DGROUP are labelled with the nearest named global at or below them
(re/data_names.tsv: 3f33:X = DS X + 0x8890, 36aa:X = DS X) as name+delta. Everything else is shown as a linear
address (heap blocks, overlay buffer, EMS frame, video BIOS area ...). Use with dbgchan BREAK to freeze the game
between snapshots, or LOADSTATE the same .sav before each variant so the two runs start from identical RAM.
"""
import bisect
import os
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT
CONV = 0xA0000                 # conventional memory: ALL code runs here (real mode; overlays are copied into the
                               # overlay buffer from their EMS / XMS cache before they run)
FULL = 16 << 20                # memsize=16: + upper memory (A000 video, EMS page frame) + extended memory, where
                               # DOSBox-X keeps the EMS pages (LBX pictures, screen backup, overlay cache) and XMS
WINDOW = 0x100                 # label with the nearest name at most this far below (arrays: check the size!)
NOISE = [(0x400, 0x500, 'BIOS data area (timer ticks, keyboard)')]


def snapshot(mem, size=FULL):
    return mem.read(mem.base, size)


def _names():
    out = []
    for line in open(os.path.join(ROOT, 're/data_names.tsv'), encoding='utf-8'):
        f = line.rstrip('\n').split('\t')
        if len(f) < 2 or ':' not in f[0]:
            continue
        seg, off = f[0].split(':')
        try:
            o = int(off, 16)
        except ValueError:
            continue
        ds = o + 0x8890 if seg.lower() == '3f33' else o if seg.lower() == '36aa' else None
        if ds is not None and ds < 0x10000:
            out.append((ds, f[1]))
    out.sort()
    return out


_NAMES = None


def label(mem, lin):
    global _NAMES
    if _NAMES is None:
        _NAMES = _names()
    dg = mem.ds_seg * 16
    for lo, hi, what in NOISE:
        if lo <= lin < hi:
            return what
    if dg <= lin < dg + 0x10000:
        off = lin - dg
        keys = [k for k, _ in _NAMES]
        i = bisect.bisect_right(keys, off) - 1
        if i >= 0 and off - _NAMES[i][0] < WINDOW:
            n = _NAMES[i][1]
            d = off - _NAMES[i][0]
            return 'DS:%04X %s%s' % (off, n, '+%X' % d if d else '')
        return 'DS:%04X (unnamed)' % off
    if lin < CONV:
        return 'lin %05X (seg %04X:%04X)' % (lin, lin >> 4, lin & 15)
    if lin < 0x100000:
        return 'upper %05X (%s)' % (lin, 'video A000-BFFF' if lin < 0xC0000 else 'ROM / EMS page frame / UMB')
    return 'ext %07X (XMS / EMS page memory, 16 KB page %d)' % (lin, (lin - 0x100000) // 0x4000)


def runs(a, b, gap=2):
    """[(start, end)] of changed bytes, merging runs whose gap is < gap bytes (numpy: 16 MB in well under a second)."""
    import numpy as np
    n = min(len(a), len(b))
    d = np.flatnonzero(np.frombuffer(a, np.uint8, n) != np.frombuffer(b, np.uint8, n))
    if not len(d):
        return []
    cut = np.flatnonzero(np.diff(d) > gap)
    starts = np.concatenate(([d[0]], d[cut + 1]))
    ends = np.concatenate((d[cut], [d[-1]])) + 1
    return list(zip(starts.tolist(), ends.tolist()))


def report(mem, a, b, maxlen=24, skip_noise=True):
    lines = []
    for s, e in runs(a, b):
        lab = label(mem, s)
        if skip_noise and any(lo <= s < hi for lo, hi, _ in NOISE):
            continue
        n = e - s
        lines.append('%-44s %4d B  %s -> %s' % (lab, n, a[s:s + min(n, maxlen)].hex(), b[s:s + min(n, maxlen)].hex()))
    return lines
