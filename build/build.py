"""Steps 2-5 of the build (README): rebuild WIZARDS.EXE and MAGIC.EXE byte for byte and check them.

  python build.py --bc TOOLS --game GAME --dosbox DOSBOX_X [--work WORK] [--parallel N]
      TOOLS     the folder that holds BORLANDC (install_bc31.py's OUT); mounted as C: in DOSBox-X
      GAME      your Master of Magic v1.31 folder (WIZARDS.EXE / MAGIC.EXE: the AIL stand-ins and the final check)
      DOSBOX_X  the dosbox-x executable
      WORK      the build folder (default: ./work); W/ and L/ get the objects and the executables
      N         DOSBox-X sessions at once (default 8)

Every compile runs in its OWN DOSBox-X session (`dosbox-x -silent`: no window, exits when its batch ends): Borland
C++ 3.1 reads leftovers of the previous run from memory, so compiles that share a session can differ. Prints both
executables' SHA-256 against the originals; exit status 0 when both are identical."""
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
ORIGINAL = {'WIZARDS.EXE': '83e273105b49ebb758b041c965298e45d491513e448e84ede2c6152232ad4e3d',
            'MAGIC.EXE': 'e9b96da031097fee92d8f00213f7406d5d22003c63e486b7f6394eaa078c3ebb'}
PROGRAMS = (('WIZARDS.EXE', 'W', 'WIZARDS.RSP', 'S2FC1.ASM'), ('MAGIC.EXE', 'L', 'MAGIC.RSP', 'AIL.ASM'))


def overlays(rsp):
    """object files between /o and /o- of a response file: the modules compiled with -Y"""
    on, out = False, set()
    for line in open(rsp):
        for w in line.replace('+', ' ').split():
            if w == '/o':
                on = True
            elif w == '/o-':
                on = False
            elif on and w.upper().endswith('.OBJ'):
                out.add(w.upper())
    return out


def write_conf(path, tools, work, batch):
    open(path, 'w').write(
        '[dosbox]\nmemsize=16\n[cpu]\ncycles=max\ncore=normal\n[mixer]\nnosound=true\n[speaker]\npcspeaker=false\n'
        '[sblaster]\nsbtype=none\n[dos]\nxms=true\nems=false\n[autoexec]\nmount c "%s"\nmount d "%s"\nc:\n'
        'set PATH=C:\\BORLANDC\\BIN\nd:\ncall %s\nexit\n' % (os.path.abspath(tools), os.path.abspath(work), batch))


def run_sessions(dosbox, tools, work, batches, parallel):
    """each batch file in a fresh, silent DOSBox-X session, `parallel` at a time"""
    todo, running, t0 = list(batches), [], time.time()
    while todo or running:
        while todo and len(running) < parallel:
            b = todo.pop(0)
            conf = os.path.join(work, os.path.splitext(b)[0] + '.CNF')
            write_conf(conf, tools, work, b)
            running.append(subprocess.Popen([dosbox, '-silent', '-fastlaunch', '-nogui', '-conf', conf],
                                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL))
        running = [p for p in running if p.poll() is None]
        time.sleep(0.2)
    return time.time() - t0


def module_files(prog):
    if prog == 'WIZARDS.EXE':
        d = os.path.join(REPO, 'src', 'wizards')
        return [os.path.join(d, f) for f in os.listdir(d) if f.upper().endswith(('.C', '.ASM'))]
    lines = open(os.path.join(REPO, 'src', 'magic', 'MODULES.TSV'), encoding='utf-8').read().splitlines()
    return [os.path.normpath(os.path.join(REPO, 'src', 'magic', l.split('\t')[1].strip()))
            for l in lines if l.strip() and not l.startswith('#')]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--bc', required=True)
    ap.add_argument('--game', required=True)
    ap.add_argument('--dosbox', required=True)
    ap.add_argument('--work', default='work')
    ap.add_argument('--parallel', type=int, default=8)
    a = ap.parse_args()
    work = os.path.abspath(a.work)
    if os.path.isdir(work):
        shutil.rmtree(work)
    os.makedirs(work)
    # step 2: the Miles AIL stand-ins from your game
    subprocess.run([sys.executable, os.path.join(HERE, 'make_ail.py'), a.game, work], check=True)
    # step 3: one folder per program, one fresh session per module
    batches = []
    for prog, folder, rsp, standin in PROGRAMS:
        d = os.path.join(work, folder)
        os.makedirs(d)
        for h in os.listdir(os.path.join(REPO, 'src', 'include')):
            shutil.copy(os.path.join(REPO, 'src', 'include', h), os.path.join(d, h.upper()))
        files = module_files(prog)
        for f in files:
            shutil.copy(f, os.path.join(d, os.path.basename(f).upper()))
        shutil.copy(os.path.join(work, standin), os.path.join(d, standin))
        shutil.copy(os.path.join(HERE, rsp), os.path.join(d, rsp))
        ovl = overlays(os.path.join(HERE, rsp))
        for k, name in enumerate(sorted(os.path.basename(f).upper() for f in files) + [standin]):
            if name.endswith('.ASM'):
                cmd = 'TASM /ml %s' % name
            else:
                y = '-Y ' if os.path.splitext(name)[0] + '.OBJ' in ovl else ''
                cmd = 'BCC %s-c -mm -a -d -IC:\\BORLANDC\\INCLUDE %s' % (y, name)
            bat = '%s%03d.BAT' % (folder, k)
            open(os.path.join(work, bat), 'w', newline='').write(
                'cd %s\r\n%s > %s%03d.LOG\r\ncd ..\r\n' % (folder, cmd, folder, k))
            batches.append(bat)
    t = run_sessions(a.dosbox, a.bc, work, batches, a.parallel)
    print('compiled %d modules in %.0f s' % (len(batches), t))
    # step 3b: the WIZARDS stand-in's relocation in the original order
    subprocess.run([sys.executable, os.path.join(HERE, 'fix_ail_obj.py'), os.path.join(work, 'W', 'S2FC1.OBJ')],
                   check=True, stdout=subprocess.DEVNULL)
    # step 4: link at the original link date
    links = []
    for prog, folder, rsp, standin in PROGRAMS:
        bat = 'LINK%s.BAT' % folder
        open(os.path.join(work, bat), 'w', newline='').write(
            'cd %s\r\nDATE /F\r\nDATE 03/03/1995\r\nTLINK @%s > LINK.LOG\r\ncd ..\r\n' % (folder, rsp))
        links.append(bat)
    run_sessions(a.dosbox, a.bc, work, links, 2)
    # step 5: compare with the originals
    ok = True
    for prog, folder, rsp, standin in PROGRAMS:
        p = os.path.join(work, folder, prog)
        h = hashlib.sha256(open(p, 'rb').read()).hexdigest() if os.path.exists(p) else 'missing'
        ok = ok and h == ORIGINAL[prog]
        print('%-12s %s  %s' % (prog, h, 'IDENTICAL to v1.31' if h == ORIGINAL[prog] else 'DIFFERS'))
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
