"""A test copy of 1.31's WIZARDS.EXE with community byte patches applied - the 'fixed build' a regression case is run
against to see its result change. Reads gog/official_131 and gog/community_patch/Patches/Patches.zip; writes ONLY
OUTDIR/WIZARDS.EXE (+ an unchanged MAGIC.EXE), never inside gog/ or the harness game folder. Each patch's old bytes
must match 1.31's, or it is refused.
  python -I tools/harness/fixed_copy.py OUTDIR 140/IW042.TXT 151/W656FIX.TXT ...
then: MOM_TEST_EXE_DIR=OUTDIR/ python -P tools/harness/regress.py 155 199   (static cases read that EXE)"""
import os
import re
import shutil
import sys
import zipfile
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT


def apply(outdir, names):
    out = os.path.abspath(outdir).replace('\\', '/')
    for bad in (ROOT + 'gog', ROOT + 'work/harness/game'):
        if out.lower().startswith(os.path.abspath(bad).replace('\\', '/').lower()):
            raise SystemExit('refused: %s is inside %s' % (out, bad))
    exe = bytearray(open(_mp.GAME + 'official_131/WIZARDS.EXE', 'rb').read())
    z = zipfile.ZipFile(_mp.GAME + 'community_patch_160/Patches/Patches.zip')
    for nm in names:
        zi = [i for i in z.infolist() if i.filename.endswith('/' + nm) or i.filename == nm]
        if len(zi) != 1:
            raise SystemExit('patch %s: %d matches' % (nm, len(zi)))
        lines = z.read(zi[0]).decode('latin1').splitlines()
        vals = [l.strip() for l in lines if re.fullmatch(r'[0-9A-Fa-f]{1,6}', l.strip())]
        trip = [(int(vals[k], 16), int(vals[k + 1], 16), int(vals[k + 2], 16)) for k in range(0, len(vals) - 2, 3)]
        bad = [t for t in trip if exe[t[0]] != t[1]]
        if bad:
            raise SystemExit('patch %s: %d old bytes do not match 1.31' % (nm, len(bad)))
        for f, old, new in trip:
            exe[f] = new
        print('%s: %d bytes' % (nm, len(trip)))
    os.makedirs(out, exist_ok=True)
    open(out + '/WIZARDS.EXE', 'wb').write(bytes(exe))
    shutil.copyfile(_mp.GAME + 'official_131/MAGIC.EXE', out + '/MAGIC.EXE')
    print('wrote', out + '/WIZARDS.EXE')


if __name__ == '__main__':
    apply(sys.argv[1], sys.argv[2:])
