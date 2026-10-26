"""Field offsets of the game's records from src/include/STRUCTS.H (the matched source), for the regression harness:
save-file edits and RAM reads by field name instead of hand-kept offsets.

Rules: char 1, int / unsigned / short 2, long 4, a near pointer 2, a far pointer 4, nested structs and arrays as
declared; Borland -a word alignment (anything wider than a char, and every struct, on an even offset; a struct's
size rounded up to even) - the saved records carry their pad bytes as named fields, so -a changes nothing there. Array sizes may be #define
names from the include files. Every struct's size is checked against its '};   /* 0xNN */' comment.

  python -I tools/harness/layout.py              (self-check: every struct's computed size vs its comment)
  python -I tools/harness/layout.py City         (one struct's fields: offset, size, type, count)

  from layout import L; L.off('City', 'enchants'); L.size('Unit'); L.fields('Wizard')"""
import glob
import re
import sys
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT
INC = ROOT + 'src/include/'
SIZES = {'char': 1, 'int': 2, 'short': 2, 'long': 4, 'unsigned': 2, 'signed': 2, 'float': 4, 'double': 8}


def _defines():
    d = {}
    for f in glob.glob(INC + '*.H'):
        for l in open(f, encoding='latin-1'):
            m = re.match(r'\s*#define\s+([A-Z_][A-Z0-9_]*)\s+(.+?)\s*(?:/\*.*)?$', l)
            if m:
                d[m.group(1)] = m.group(2)
    return d


class Layout:
    def __init__(self):
        self.defs = _defines()
        self.structs = {}       # name -> [(field, offset, elem_size, count, type)]
        self.size_comment = {}
        self._parse()

    def num(self, expr):
        expr = expr.strip()
        for _ in range(10):
            new = re.sub(r'\b([A-Z_][A-Z0-9_]*)\b', lambda m: '(%s)' % self.defs.get(m.group(1), m.group(1)), expr)
            if new == expr:
                break
            expr = new
        expr = re.sub(r'(?<=[0-9a-fA-F])[uUlL]+\b', '', expr)
        return int(eval(expr, {'__builtins__': {}}))

    def _parse(self):
        txt = open(INC + 'STRUCTS.H', encoding='latin-1').read()
        txt = re.sub(r'/\*.*?\*/', lambda m: m.group(0) if m.group(0).startswith('/* 0x') else ' ', txt, flags=re.S)
        for m in re.finditer(r'^struct (\w+) \{[ \t]*\n(.*?)^\};\s*(/\* (0x[0-9a-f]+) \*/)?', txt, flags=re.S | re.M):
            name, body = m.group(1), m.group(2)
            if m.group(4):
                self.size_comment[name] = int(m.group(4), 16)
            self.structs[name] = body
        self.fields_cache = {}

    def _type_size(self, t, ptr):
        if ptr:
            return 4 if 'far' in t or 'huge' in t else 2
        t = t.replace('volatile', '').replace('const', '').strip()
        m = re.match(r'struct (\w+)', t)
        if m:
            return self.size(m.group(1))
        for w in ('long', 'short', 'char', 'int', 'float', 'double'):
            if re.search(r'\b%s\b' % w, t):
                return SIZES[w]
        if re.search(r'\b(unsigned|signed)\b', t):
            return 2
        raise ValueError('type ' + t)

    def fields(self, name):
        if name in self.fields_cache:
            return self.fields_cache[name]
        out = []
        off = 0
        for decl in self.structs[name].split(';'):
            decl = ' '.join(decl.split())
            if not decl:
                continue
            m = re.match(r'((?:(?:unsigned|signed|struct \w+|char|int|long|short|far|near|huge|const|volatile)\b\s*)+)(.*)$',
                         decl)
            if not m:
                raise ValueError('%s: %r' % (name, decl))
            base, rest = m.group(1).strip(), m.group(2)
            for d in rest.split(','):
                d = d.strip()
                ptr = '*' in d or '(' in d
                tquals = base + (' ' + d.split('*')[0] if '*' in d else '')
                nm = re.search(r'(\w+)\s*((?:\[[^\]]+\])*)\s*$', d.replace('(', ' ').replace(')', ' '))
                fname, dims = nm.group(1), nm.group(2)
                count = 1
                for dm in re.findall(r'\[([^\]]+)\]', dims):
                    count *= self.num(dm)
                es = self._type_size(tquals if ptr else base, ptr)
                if (es > 1 or base.startswith('struct ')) and off % 2:
                    off += 1                    # -a: words, longs, pointers and structs on even offsets
                out.append((fname, off, es, count, base + (' *' if ptr else '')))
                off += es * count
        self.fields_cache[name] = out
        return out

    def size(self, name):
        f = self.fields(name)
        s = f[-1][1] + f[-1][2] * f[-1][3] if f else 0
        return s + (s & 1)                      # -a: a struct's size is even

    def off(self, name, field):
        for f in self.fields(name):
            if f[0] == field:
                return f[1]
        raise KeyError('%s.%s' % (name, field))

    def field(self, name, field):
        for f in self.fields(name):
            if f[0] == field:
                return f
        raise KeyError('%s.%s' % (name, field))


L = Layout()

if __name__ == '__main__':
    if len(sys.argv) > 1:
        for f in L.fields(sys.argv[1]):
            print('%#06x %3d x %-4d %-24s %s' % (f[1], f[2], f[3], f[0], f[4]))
        print('size %#x (comment %s)' % (L.size(sys.argv[1]), hex(L.size_comment.get(sys.argv[1], -1))))
    else:
        bad = 0
        for n in L.structs:
            try:
                s = L.size(n)
            except Exception as e:
                print('%-20s ERROR %s' % (n, e))
                bad += 1
                continue
            c = L.size_comment.get(n)
            if c is not None and c != s:
                print('%-20s computed %#x, comment %#x' % (n, s, c))
                bad += 1
        print('%d structs, %d with a size comment, %d problems' % (len(L.structs), len(L.size_comment), bad))
