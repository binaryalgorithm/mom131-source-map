"""Where a numbered bug sits in src/: each marker of 'BUG #n' / 'BUGS #n' / 'QUIRK #n' / 'Numbered: ... #n' with its
file, line and function - the function a header comment heads (the comment block ends right above its definition),
else the function enclosing the marker; markers in a file's module header have none - and whether exefn can locate
that function in the 1.31 EXE (P4 B4 static rows / breakpoint targets).
  python -I tools/harness/bugsite.py 1 147 318"""
import os
import re
import sys
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT
DEF_START = re.compile(r'^[A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\s*\(')


def _funcs(lines):
    """[(line index, name)] of function definitions: a '{' at column 0 below a column-0 'type name(' line (its
    parameter list may go on over indented lines)."""
    out = []
    for i, l in enumerate(lines):
        if not l.startswith('{'):
            continue
        k = i - 1
        while k >= 0 and lines[k].startswith((' ', '\t')):
            k -= 1
        m = DEF_START.match(lines[k]) if k >= 0 else None
        if m and not lines[k].startswith(('if', 'while', 'for', 'switch', 'return', 'struct', 'typedef')):
            out.append((k, m.group(1)))
    return out


def _owner(lines, defs, i):
    """The function a marker on line i belongs to: the one a header comment heads, else the enclosing one."""
    j = i
    while j < len(lines) and '*/' not in lines[j]:
        j += 1
    j += 1
    while j < len(lines) and not lines[j].strip():
        j += 1
    header = [n for k, n in defs if k == j]
    if header and not lines[i].startswith((' ' * 4, '\t')):   # a column-0 / 3-space header comment, not in a body
        return header[0]
    before = [n for k, n in defs if k < i]
    return before[-1] if before else None


def sites(num):
    pat = re.compile(r'(BUGS? #|QUIRK #|Numbered:[^*\n]*?#)%d\b' % num)
    found = []
    for sub in ('wizards', 'magic'):
        d = ROOT + 'src/' + sub
        for fn in sorted(os.listdir(d)):
            if not fn.upper().endswith('.C'):
                continue
            lines = open(os.path.join(d, fn), encoding='latin-1').read().split('\n')
            defs = _funcs(lines)
            for i, l in enumerate(lines):
                if pat.search(l):
                    found.append(('WIZARDS' if sub == 'wizards' else 'MAGIC', 'src/%s/%s' % (sub, fn), i + 1,
                                  _owner(lines, defs, i), l.strip()))
    return found


if __name__ == '__main__':
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import exefn
    for a in sys.argv[1:]:
        for prog, path, ln, func, text in sites(int(a)):
            try:
                fo, n, addr = exefn.locate(prog, func)
                where = '%s %d bytes' % (addr, n)
            except Exception as e:
                where = 'not located (%s)' % e
            print('#%s %s:%d %s [%s] | %s' % (a, path, ln, func, where, text[:110]))
