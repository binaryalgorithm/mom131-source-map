"""Source line -> code offset in the game, for breakpoints on one statement (P4 B4).

Compiles the matched module with the game's settings plus -S (Borland C++ 3.1 in DOSBox-X, tools/bcc_run.py) into
work/bcc/lst/ only - src/ is not touched - and reads the listing: the compiler writes each source line as a comment
before its instructions. The instructions of a function are counted up to the line, and the same count is taken in
the GAME's bytes of that function (exefn.py, capstone), so the offset is the shipped code's. The mnemonics are
compared on the way (a mismatch is an error, not a guess).

  python -P tools/harness/srcmap.py src/wizards/AIPROD.C "p_cities[i].population < 5"     (every place, with offsets)
  srcmap.find(path, text, nth=0) -> (function, offset, Ghidra 'seg:off' of the function)"""
import os
import re
import shutil
import sys
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

import capstone

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import exefn                                   # noqa: E402

ROOT = _mp.ROOT
LST = ROOT + 'work/bcc/lst/'
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)


def listing(src):
    """The -S listing of a C module (cached: rebuilt when the source is newer)."""
    name = os.path.basename(src).upper()
    asm = LST + name[:-2] + '.ASM'
    if os.path.exists(asm) and os.path.getmtime(asm) > os.path.getmtime(ROOT + src):
        return open(asm, encoding='latin-1').read()
    import bcc_run
    import mom
    if os.path.exists(mom.LOCK):                # the compiler runs in DOSBox-X too: never beside a harness run
        pid = open(mom.LOCK).read().strip()
        import subprocess
        if pid and pid != str(os.getpid()) and pid in subprocess.run(['tasklist', '/FI', 'PID eq %s' % pid, '/NH'],
                                                                     capture_output=True, text=True).stdout:
            raise RuntimeError('a harness run (pid %s) is active: compile listings before it (srcmap.py --all)' % pid)
    os.makedirs(LST, exist_ok=True)
    shutil.copy(ROOT + src, LST + name)
    for h in os.listdir(ROOT + 'src/include'):
        shutil.copy(ROOT + 'src/include/' + h, LST + h.upper())
    open(bcc_run.WORK + 'LST.BAT', 'w').write('cd LST\r\nBCC -S -c -mm -a -d -IC:\\BORLANDC\\INCLUDE %s > OUT.TXT\r\n'
                                             'cd ..\r\n' % name)
    bcc_run.run('LST.BAT', 120)
    return open(asm, encoding='latin-1').read()


def _functions(text):
    """{function: [(kind, text)]} kind 'src' (a source line comment) or 'ins' (an instruction)."""
    out, cur = {}, None
    for line in text.split('\n'):
        m = re.match(r'_(\w+)\tproc\t', line)
        if m:
            cur = m.group(1)
            out[cur] = []
            continue
        if cur and re.match(r'_\w+\tendp', line):
            cur = None
            continue
        if cur is None:
            continue
        if line.startswith('   ;\t'):
            out[cur].append(('src', line[5:]))
        elif line.startswith('\t') and not line.startswith(('\t?debug', '\tassume')) and line.strip():
            out[cur].append(('ins', line.strip().split('\t')[0]))
    return out


def _norm(m):
    m = m.lower()
    if m.startswith('j') and m != 'jmp':
        return 'jcc'                            # the listing may state a branch with the other polarity
    return {'jz': 'je', 'jnz': 'jne', 'jnae': 'jb', 'jc': 'jb', 'jnb': 'jae', 'jnc': 'jae', 'jnge': 'jl',
            'jnl': 'jge', 'jng': 'jle', 'jnle': 'jg', 'retf': 'ret', 'lcall': 'call', 'ljmp': 'jmp',
            'shl': 'sal', 'repe': 'rep', 'repz': 'rep', 'cwde': 'cbw', 'cdq': 'cwd'}.get(m, m)


def places(src, text, prog='WIZARDS', only=None):
    """Every (function, offset, ghidra address) where a source line containing TEXT starts; only = one function's
    name (the others are not aligned - a function whose listing and game code differ no longer stops the search)."""
    fns = _functions(listing(src))
    out = []
    for fn, items in fns.items():
        if only is not None and fn != only:
            continue
        k = 0
        for j, (kind, t) in enumerate(items):
            if kind == 'ins':
                k += 1
            elif text in t:
                code = exefn.fn_bytes(prog, fn)
                _, _, a = exefn.locate(prog, fn)
                ins = list(MD.disasm(code, 0))
                want = [_norm(x[1]) for x in items[:j] if x[0] == 'ins']
                got = [_norm(i.mnemonic) for i in ins]
                p = q = 0                       # align: the listing's 'jcc over; jmp target' may be one short jcc
                while p < len(want):
                    if q >= len(got):
                        raise RuntimeError('%s: game code ends before the line' % fn)
                    if (want[p] == 'jcc' and want[p + 1:p + 2] == ['jmp'] and got[q] == 'jcc' and
                            got[q + 1:q + 2] != ['jmp']):
                        p, q = p + 2, q + 1
                    elif want[p] == 'call' and got[q:q + 3] == ['nop', 'push', 'call']:
                        p, q = p + 1, q + 3         # TLINK: a far call within the segment -> nop / push cs / near call
                    elif want[p] == got[q]:
                        p, q = p + 1, q + 1
                    elif want[p] == 'jcc' and got[q] == 'jcc' and got[q + 1:q + 2] == ['jmp']:
                        p, q = p + 1, q + 2
                    else:
                        raise RuntimeError('%s: listing and game code differ at listing instruction %d (%s vs %s at '
                                           '0x%x)' % (fn, p, want[p], got[q], ins[q].address))
                out.append((fn, ins[q].address if q < len(ins) else len(code), a))
    return out


def find(src, text, nth=0, prog='WIZARDS'):
    return places(src, text, prog)[nth]


def build_all(prog_dir='src/wizards/'):
    """Every C module's listing, compiled in parallel fresh sessions (bcc_run.run_many) - run it while no harness
    case runs; places() then only reads the cached listings."""
    import bcc_run
    os.makedirs(LST, exist_ok=True)
    for h in os.listdir(ROOT + 'src/include'):
        shutil.copy(ROOT + 'src/include/' + h, LST + h.upper())
    bats = []
    for k, f in enumerate(sorted(os.listdir(ROOT + prog_dir))):
        if f.upper().endswith('.C'):
            name = f.upper()
            shutil.copy(ROOT + prog_dir + f, LST + name)
            b = 'L%03d.BAT' % k                 # DOS 8.3 names
            open(bcc_run.WORK + b, 'w').write('cd LST\r\nBCC -S -c -mm -a -d -IC:\\BORLANDC\\INCLUDE %s > %s.OUT\r\n'
                                              'cd ..\r\n' % (name, name[:-2]))
            bats.append(b)
    t = bcc_run.run_many(bats, 300, 6)
    print('%d listings in %.0f s' % (len(bats), t))


if __name__ == '__main__':
    if sys.argv[1:] == ['--all']:
        build_all()
        sys.exit(0)
    for fn, off, a in places(sys.argv[1], sys.argv[2]):
        print('%s (%s) + 0x%x' % (fn, a, off))
