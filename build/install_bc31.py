"""Step 1 of the build (README): install the parts of Borland C++ 3.1 the build uses, from the WinWorld archive.

  python install_bc31.py DISKS OUT
      DISKS  the unpacked WinWorld archive "Borland CPP 3.1 and Application Frameworks (1992) (3.5-1.44mb)" - the
             folder with Disk01.img .. Disk15.img
      OUT    a folder; OUT/BORLANDC gets BIN, INCLUDE and LIB (mount OUT as C: in DOSBox-X)

Needs 7-Zip (7z on the PATH): it opens the floppy images, and the 1992 archives are PKZIP 1.x ('imploded'), which
Python's zipfile cannot read. The six files that decide the output are checked by SHA-256 at the end."""
import glob
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

WANT = {'BIN/BCC.EXE': '87c6dfbad817eeca31c069043f08063f7122663e51932ad089e6d1c563450d3b',
        'BIN/TLINK.EXE': 'ab8b8b96c1bde5ff6f56c8be2c3b928d21ead7430e72b01e3b22f1fa179ad006',
        'BIN/TASM.EXE': '80534cd6a1e2c2b4376ae3f21954c6dbfa77a6998d5e0bf8b8bee6153f3174ea',
        'LIB/C0M.OBJ': '54e9630201745b88a0f2d22c3ba18743b54f351974b7bd67f2a0902ed1f7b480',
        'LIB/CM.LIB': '8b658567c42c70e09543bd181ce4fff698d59d024b13e9e593c7003e08508e80',
        'LIB/OVERLAY.LIB': '3504bd256c5564c52d73ea3b4b08ddbe113a016d7fcfbf2f186dcd83f7fc9b24'}
CA = (('CMDLINE', 'BIN'), ('TASM', 'BIN'))                    # multi-disk archives: 5-byte header per part
ZIPS = (('BIN.ZIP', 'BIN'), ('INCLUDE.ZIP', 'INCLUDE'), ('MLIB.ZIP', 'LIB'), ('XLIB.ZIP', 'LIB'))


def sevenzip(archive, dest):
    subprocess.run(['7z', 'x', '-y', '-o' + dest, archive], check=True, capture_output=True)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    disks, out = sys.argv[1], os.path.join(sys.argv[2], 'BORLANDC')
    tmp = tempfile.mkdtemp()
    for img in sorted(glob.glob(os.path.join(disks, 'Disk*.img'))):
        sevenzip(img, os.path.join(tmp, os.path.splitext(os.path.basename(img))[0]))
    if os.path.isdir(out):
        shutil.rmtree(out)
    for d in ('BIN', 'INCLUDE', 'LIB'):
        os.makedirs(os.path.join(out, d))
    for name, dest in CA:
        parts = sorted(glob.glob(os.path.join(tmp, '*', name + '.CA*')), key=lambda p: p[-1])
        z = os.path.join(tmp, name + '.ZIP')
        open(z, 'wb').write(b''.join(open(p, 'rb').read()[5:] for p in parts))
        sevenzip(z, os.path.join(out, dest))
    for name, dest in ZIPS:
        z = glob.glob(os.path.join(tmp, '*', name))
        if len(z) != 1:
            sys.exit('%s not found on the disks' % name)
        sevenzip(z[0], os.path.join(out, dest))
    shutil.rmtree(tmp)
    bad = [f for f, h in WANT.items()
           if hashlib.sha256(open(os.path.join(out, f), 'rb').read()).hexdigest() != h]
    if bad:
        sys.exit('installed, but these files are not the expected Borland C++ 3.1 ones: %s' % bad)
    print('Borland C++ 3.1 installed in %s; the six key files check out' % out)


if __name__ == '__main__':
    main()
