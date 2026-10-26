"""P4 B4 tally: the latest verdict of every case (work/harness/regress/results.tsv, the last row of a number wins; an
'also' entry takes its case's verdict) by kind - BUG SHOWN in game / static, LATENT (unreachable in 1.31), NOT SHOWN, NOT REACHED, ERROR, not run -
and the suite entries (tools/harness/B4_SUITE.tsv) still without a case.
  python -I tools/harness/b4tally.py [--list VERDICT]"""
import collections
import csv
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import regress as R                            # noqa: E402

R.load_cases()
ROOT = R.mom.ROOT
latest = {}
for row in csv.reader(open(R.OUT + 'results.tsv', encoding='utf-8'), delimiter='\t'):
    if row and row[0].isdigit():
        latest[int(row[0])] = row
verdict = {}
for n, c in R.CASES.items():
    v = latest[n][3] if n in latest else 'not run'
    for k in [n] + list(c.get('also') or ()):
        verdict[k] = (v, c['kind'])
suite = {int(r['num']): r['kind'] for r in csv.DictReader(open(ROOT + 'tools/harness/B4_SUITE.tsv',
                                                                 encoding='utf-8'), delimiter='\t')}
table = collections.defaultdict(collections.Counter)
by = collections.defaultdict(list)
for n, kind in suite.items():
    if n in verdict:
        v, ck = verdict[n]
        if v == 'BUG SHOWN':
            v = 'BUG SHOWN static' if ck == 'ST' else 'BUG SHOWN in game'
        # 'BUG SHOWN static' as a verdict: a play case's static fallback (cases_zst.FALLBACK) - counted as static
    else:
        v = 'no case'
    table[kind][v] += 1
    by[v].append(n)
cols = ['BUG SHOWN in game', 'BUG SHOWN static', 'LATENT', 'NOT SHOWN', 'NOT REACHED', 'ERROR', 'not run', 'no case']
print('%-5s' % 'kind' + ''.join('%18s' % c for c in cols))
for kind in sorted(table):
    print('%-5s' % kind + ''.join('%18d' % table[kind][c] for c in cols))
tot = collections.Counter()
for k in table.values():
    tot.update(k)
print('%-5s' % 'all' + ''.join('%18d' % tot[c] for c in cols) + '   (%d entries)' % len(suite))
if '--list' in sys.argv:
    want = sys.argv[sys.argv.index('--list') + 1:]
    for v in want or ['NOT SHOWN', 'NOT REACHED', 'ERROR', 'not run', 'no case']:
        print(v, sorted(by.get(v, [])))
