"""A named function's bytes in a WIZARDS.EXE / MAGIC.EXE file (P4 B4 static checks).

Address and size: re/names.tsv (re/magic_names.tsv) + re/current_<PROG>/functions.tsv, in Ghidra segments (image at
1000:0000). File offset: resident code = MZ header + (seg - 0x1000) * 16 + off; overlay code (VROOMM) = after the
load image, at the overlay's offset from work/flat/<PROG>_FLAT_map.json + off - as tools/patch_map.py maps the
community patches. A byte patch keeps this layout, so the same offsets read a patched EXE.

  python -I tools/harness/exefn.py WIZARDS Diplo_Make_Peace [EXE]     (hex dump + 'same as 1.31' or the differences)"""
import json
import struct
import sys
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT
OFFICIAL = {'WIZARDS': _mp.GAME + 'official_131/WIZARDS.EXE', 'MAGIC': _mp.GAME + 'official_131/MAGIC.EXE'}
NAMES = {'WIZARDS': ROOT + 're/names.tsv', 'MAGIC': ROOT + 're/magic_names.tsv'}
_cache = {}


def _tables(prog):
    if prog not in _cache:
        addr = {}
        for line in open(NAMES[prog], encoding='utf-8'):
            f = line.rstrip('\n').split('\t')
            if len(f) > 1 and ':' in f[0]:
                addr.setdefault(f[1], f[0].lower())
        size = {}
        for line in open(ROOT + 're/current_%s/functions.tsv' % prog, encoding='utf-8').read().splitlines()[1:]:
            f = line.split('\t')
            size[f[0].lower()] = int(f[2])
            addr.setdefault(f[1], f[0].lower())
        m = json.load(open(ROOT + 'work/flat/%s_FLAT_map.json' % prog))
        ovl = {o['ovl_seg'] + 0x1000: o['ovl_ofs'] for o in m['overlays']}
        _cache[prog] = (addr, size, ovl, 0x1000 + m['resident_paras'])
    return _cache[prog]


def locate(prog, name):
    """(file offset, size, 'seg:off') of function NAME (or a 'seg:off' address) in the 1.31 layout."""
    addr, size, ovl, res_end = _tables(prog)
    a = name.lower() if ':' in name else addr[name]
    seg, off = (int(x, 16) for x in a.split(':'))
    exe = open(OFFICIAL[prog], 'rb').read(32)
    cblp, cp, _, cparhdr = struct.unpack('<4H', exe[2:10])
    hdr = cparhdr * 16
    if seg < res_end:
        fo = hdr + (seg - 0x1000) * 16 + off
    else:
        img_end = (cp - 1) * 512 + (cblp or 512)
        fo = img_end + 16 + ovl[seg] + off
    return fo, size[a], a


def fn_bytes(prog, name, exe=None):
    fo, n, _ = locate(prog, name)
    return open(exe or OFFICIAL[prog], 'rb').read()[fo:fo + n]


def same_as_131(prog, name, exe):
    """True when function NAME has 1.31's bytes in EXE; else the differing offsets (relative to the function)."""
    a, b = fn_bytes(prog, name), fn_bytes(prog, name, exe)
    diff = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
    return (not diff and len(a) == len(b)), diff


if __name__ == '__main__':
    prog, name = sys.argv[1], sys.argv[2]
    exe = sys.argv[3] if len(sys.argv) > 3 else OFFICIAL[prog]
    fo, n, a = locate(prog, name)
    b = fn_bytes(prog, name, exe)
    print('%s %s at %s: file offset 0x%x, %d bytes' % (prog, name, a, fo, n))
    print(b[:64].hex(' '))
    ok, diff = same_as_131(prog, name, exe)
    print('same as 1.31' if ok else 'differs at %s' % ['%x' % d for d in diff[:20]])
