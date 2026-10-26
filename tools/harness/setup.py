"""Prepare the regression harness: your game, DOSBox-X, the generated maps. Run once (again after moving things).
  python tools/harness/setup.py --game GAME --dosbox DOSBOX_X [--dosbox-debug DOSBOX_X_DEBUG] [--bc TOOLS] [--cp CP]
    GAME           your Master of Magic v1.31 folder (WIZARDS.EXE / MAGIC.EXE are checked by SHA-256)
    DOSBOX_X       dosbox-x.exe for plain runs (2026.10.01 verified)
    DOSBOX_X_DEBUG a DOSBox-X build with the debugger channel's SAVESTATE / LOADSTATE / MEMREAD (PRs #6682 / #6683 -
                   a build newer than 2026.10.01); needed by the breakpoint cases (dbg.py). Default: DOSBOX_X
    TOOLS          the folder holding BORLANDC (build/install_bc31.py); only srcmap.py (source-line breakpoints)
    CP             the Community Patch install, for fixed_copy.py (a scratch WIZARDS.EXE with patch files applied)
Writes work/: game/official_131/ (a copy of the game's files), harness/game/ (the copy the harness runs: + the base
save and a clean MAGIC.SET), harness/SAVE9_orig.GAM, harness/backup/, flat/*_FLAT_map.json (tools/vroomm.py),
paths.json. Your game folder is only read."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
WORK = os.path.join(ROOT, 'work')
SHA = {'WIZARDS.EXE': '83e273105b49ebb758b041c965298e45d491513e448e84ede2c6152232ad4e3d',
       'MAGIC.EXE': 'e9b96da031097fee92d8f00213f7406d5d22003c63e486b7f6394eaa078c3ebb'}


def copy_top(src, dst):
    """the files of a folder (not its subfolders: GOG's DOSBox, manuals ...)"""
    os.makedirs(dst, exist_ok=True)
    for f in os.listdir(src):
        if os.path.isfile(os.path.join(src, f)):
            shutil.copy2(os.path.join(src, f), os.path.join(dst, f.upper() if f.lower().endswith(('.lbx', '.exe',
                         '.gam', '.set', '.mom')) else f))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--game', required=True)
    ap.add_argument('--dosbox', required=True)
    ap.add_argument('--dosbox-debug')
    ap.add_argument('--bc')
    ap.add_argument('--cp')
    a = ap.parse_args()
    for exe, want in SHA.items():
        p = os.path.join(a.game, exe)
        if not os.path.isfile(p):
            sys.exit('%s not found in %s' % (exe, a.game))
        if hashlib.sha256(open(p, 'rb').read()).hexdigest() != want:
            sys.exit('%s is not the v1.31 file this suite was made with' % exe)
    for p in [a.dosbox] + ([a.dosbox_debug] if a.dosbox_debug else []):
        if not os.path.isfile(p):
            sys.exit('DOSBox-X not found: %s' % p)
    game = os.path.join(WORK, 'game', 'official_131')
    copy_top(a.game, game)
    run = os.path.join(WORK, 'harness', 'game')
    if os.path.isdir(run):
        shutil.rmtree(run)
    copy_top(a.game, run)
    for f in ('SAVE1.GAM', 'SAVE2.GAM', 'SAVE3.GAM', 'SAVE4.GAM', 'SAVE5.GAM', 'SAVE6.GAM', 'SAVE7.GAM', 'SAVE8.GAM'):
        if os.path.exists(os.path.join(run, f)):
            os.remove(os.path.join(run, f))       # your own saves stay out of the harness copy
    base = os.path.join(HERE, 'base')
    backup = os.path.join(WORK, 'harness', 'backup')
    os.makedirs(backup, exist_ok=True)
    for f, dsts in (('SAVE9_orig.GAM', [os.path.join(WORK, 'harness', 'SAVE9_orig.GAM'), os.path.join(run, 'SAVE9.GAM'),
                                        os.path.join(backup, 'SAVE9.GAM')]),
                    ('SAVE9_turn108_auto.GAM', [os.path.join(WORK, 'harness', 'SAVE9_turn108_auto.GAM')]),
                    ('MAGIC.SET', [os.path.join(run, 'MAGIC.SET'), os.path.join(backup, 'MAGIC.SET')])):
        for d in dsts:
            shutil.copy(os.path.join(base, f), d)
    if a.cp:
        z = os.path.join(a.cp, 'Patches', 'Patches.zip')
        if os.path.isfile(z):
            d = os.path.join(WORK, 'game', 'community_patch_160', 'Patches')
            os.makedirs(d, exist_ok=True)
            shutil.copy(z, d)
        else:
            print('no Patches/Patches.zip in %s - fixed_copy.py will not work' % a.cp)
    flat = os.path.join(WORK, 'flat')
    os.makedirs(flat, exist_ok=True)
    for prog in ('WIZARDS', 'MAGIC'):
        subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'vroomm.py'), 'flat',
                        os.path.join(game, prog + '.EXE'), os.path.join(flat, prog + '_FLAT.EXE')], check=True)
    paths = {'dosbox': os.path.abspath(a.dosbox), 'dosbox_debug': os.path.abspath(a.dosbox_debug or a.dosbox)}
    if a.bc:
        paths['bc'] = os.path.abspath(a.bc)
    json.dump(paths, open(os.path.join(WORK, 'paths.json'), 'w'), indent=1)
    print('ready: work/ prepared; try  python -P tools/harness/regress.py --kind ST')


if __name__ == '__main__':
    main()
