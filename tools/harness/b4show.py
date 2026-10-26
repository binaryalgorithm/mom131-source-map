"""Print BUGS_AND_QUIRKS.txt entries with their B4_SUITE.tsv row (P4 B4 case writing aid).
  python -I tools/harness/b4show.py 45 58 62        (entries by number)
  python -I tools/harness/b4show.py --kind ST       (every entry of a suite kind)"""
import csv
import re
import sys
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT


def entries():
    txt = open(ROOT + 'BUGS_AND_QUIRKS.txt', encoding='utf-8').read().split('\n')
    out, cur = {}, None
    for line in txt:
        m = re.match(r'(\d+)\. (\S+)', line)
        if m:
            cur = int(m.group(1))
            out[cur] = [line]
        elif line.startswith('==='):
            cur = None
        elif cur is not None and line.strip():
            out[cur].append(line)
    return out


def suite():
    return {int(r['num']): r for r in csv.DictReader(open(ROOT + 'tools/harness/B4_SUITE.tsv', encoding='utf-8'),
                                                     delimiter='\t')}


if __name__ == '__main__':
    E, S = entries(), suite()
    a = sys.argv[1:]
    nums = [n for n in sorted(S) if S[n]['kind'] == a[1]] if a[:1] == ['--kind'] else [int(x) for x in a]
    for n in nums:
        print('\n'.join(E.get(n, ['%d. (no entry)' % n])))
        r = S.get(n)
        if r:
            print('    SUITE %s | %s | bug: %s | fixed: %s' % (r['kind'], r['case (setup -> action -> observe)'],
                                                             r['1.31 shows (bug)'], r['fixed build shows']))
        print()
