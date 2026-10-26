"""Run a DOS batch file with Borland C++ 3.1 (or 3.0) in DOSBox-X on the hidden harness desktop and wait for it to finish
(step 78c-R, matching pilot). C: = the folder holding BORLANDC (setup.py --bc; the bc argument is ignored), D: = work/bcc (sources + outputs).
  python -P tools/bcc_run.py BATCH.BAT [timeout_s]     -> output of the batch in work/bcc/OUT.TXT (batch redirects)"""
import os
import subprocess
import sys
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__))))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'harness'))
import dosbox  # noqa: E402

ROOT = _mp.ROOT
WORK = ROOT + 'work/bcc/'
CONF = WORK + 'bcc.conf'


def write_conf(path, batch, bc='bc31'):
    open(path, 'w').write('''[sdl]
output=surface
[dosbox]
memsize=16
[cpu]
cycles=max
core=normal
[mixer]
nosound=true
[speaker]
pcspeaker=false
[sblaster]
sbtype=none
[dos]
xms=true
ems=false
[autoexec]
mount c "%s"
mount d "%s"
c:
set PATH=C:\\BORLANDC\\BIN
d:
call %s
exit
''' % (_mp.win(_mp.BC.rstrip('/')), WORK.rstrip('/').replace('/', '\\'), batch))


def alive(pid):
    r = subprocess.run(['tasklist', '/FI', 'PID eq %d' % pid, '/FO', 'CSV', '/NH'], capture_output=True, text=True)
    return 'dosbox' in r.stdout.lower()


def run_many(batches, timeout=300, parallel=6, bc='bc31'):
    """Each batch in its OWN DOSBox-X session (several at a time). One BCC run's code can depend on what earlier BCC
    runs in the same session left in memory (2026-10-21: a rename in other files changed ENDGAME.C's register choice
    in a full build), so a module must always be compiled in a fresh session - as tools/match_build.py does."""
    os.makedirs(WORK, exist_ok=True)
    todo, running, t0 = list(batches), {}, time.time()
    while todo or running:
        while todo and len(running) < parallel:
            b = todo.pop(0)
            conf = WORK + 'bcc_%s.conf' % os.path.splitext(b)[0]
            write_conf(conf, b, bc)
            running[dosbox.start(conf=conf, exe=dosbox.EXE)['pid']] = b
        for pid in [p for p in running if not alive(p)]:
            del running[pid]
        if time.time() - t0 > timeout * max(1, len(batches) // parallel + 1):
            for pid in running:
                subprocess.run(['taskkill', '/PID', str(pid), '/F'], capture_output=True)
            raise TimeoutError('batches %s did not finish' % ', '.join(running.values()))
        time.sleep(0.3)
    return time.time() - t0


def compile_cached(jobs, subdir, prefix, parallel=12, bc='bc31'):
    """jobs: [(command, source file, object file, log file)] to run in work/bcc/<subdir>. Every compile runs in a fresh
    session, so its object depends only on the command, the source and the headers: the object and its log are cached
    under work/bcc/objcache by a hash of those, and only changed modules are compiled. -> (seconds, compiled count)"""
    import hashlib
    import shutil
    d = WORK + subdir + '/'
    cache = WORK + 'objcache/'
    os.makedirs(cache, exist_ok=True)
    hdr = hashlib.sha256()
    for h in sorted(x for x in os.listdir(d) if x.upper().endswith(('.H', '.INC'))):
        hdr.update(h.encode() + open(d + h, 'rb').read())
    todo, keys = [], {}
    for k, (cmd, src, obj, log) in enumerate(jobs):
        key = hashlib.sha256(('%s|%s|' % (bc, cmd)).encode() + open(d + src, 'rb').read() + hdr.digest()).hexdigest()
        keys[k] = key
        if os.path.exists(cache + key + '.OBJ'):
            shutil.copy(cache + key + '.OBJ', d + obj)
            shutil.copy(cache + key + '.TXT', d + log)
        else:
            b = '%s%03d.BAT' % (prefix, k)
            open(WORK + b, 'w').write('cd %s\r\n%s > %s\r\ncd ..\r\n' % (subdir, cmd, log))
            todo.append((k, b))
    t = run_many([b for k, b in todo], 120, parallel, bc) if todo else 0.0
    for k, b in todo:
        cmd, src, obj, log = jobs[k]
        if os.path.exists(d + obj) and os.path.exists(d + log):     # a failed compile is never cached
            txt = open(d + log, errors='replace').read()
            if not any(l.startswith(('Error', 'Fatal', '**Error', '**Fatal')) and 'messages' not in l
                       for l in txt.splitlines()):
                shutil.copy(d + obj, cache + keys[k] + '.OBJ')
                shutil.copy(d + log, cache + keys[k] + '.TXT')
    return t, len(todo)


def run(batch, timeout=300, bc='bc31'):
    os.makedirs(WORK, exist_ok=True)
    write_conf(CONF, batch, bc)
    st = dosbox.start(conf=CONF, exe=dosbox.EXE)
    pid = st['pid']
    t0 = time.time()
    while time.time() - t0 < timeout:
        r = subprocess.run(['tasklist', '/FI', 'PID eq %d' % pid, '/FO', 'CSV', '/NH'], capture_output=True, text=True)
        if 'dosbox' not in r.stdout.lower():
            return time.time() - t0
        time.sleep(0.5)
    dosbox.stop()
    raise TimeoutError('batch %s did not finish in %d s' % (batch, timeout))


if __name__ == '__main__':
    print('done in %.1f s' % run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 300, os.environ.get('BC', 'bc31')))
