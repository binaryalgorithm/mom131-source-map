"""P4 B4 regression runner: runs the case functions registered in tools/harness/cases_*.py on the hidden desktop and
records, per entry of BUGS_AND_QUIRKS.txt, whether 1.31's behaviour (the bug) shows.
A case returns {'shown': True / False / None / 'latent', 'observed': text} ('latent': the faulty code is
unreachable in 1.31, the observed text says why); shown = the bug's behaviour was seen (expected on 1.31; a
fixed build should give False). Results: work/harness/regress/results.tsv (one line per run) and NUM.txt (the log).
  python -P tools/harness/regress.py 2 104 96          (those cases)
  python -P tools/harness/regress.py --kind OV          (every registered case of a kind)
  python -P tools/harness/regress.py --list"""
import datetime
import glob
import importlib
import os
import sys
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mom                                     # noqa: E402

OUT = mom.ROOT + 'work/harness/regress/'
CASES = {}
BOOTED = [False]                               # any case of this run used DOSBox


def case(num, kind, title, setup='', bug='', fixed='', also=()):
    """Registers a case for entry NUM: kind as in B4_SUITE.tsv, the 1.31 result (bug) and a fixed build's (fixed);
    also = further entries the same run shows (their result line is written too)."""
    def deco(f):
        CASES[num] = dict(kind=kind, title=title, setup=setup, bug=bug, fixed=fixed, fn=f, also=tuple(also))
        return f
    return deco


def load_cases():
    for p in sorted(glob.glob(os.path.join(HERE, 'cases_*.py'))):
        importlib.import_module(os.path.basename(p)[:-3])


def run(num):
    c = CASES[num]
    log = []
    stamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
    mom.CASE.update(num=num, shots=[], booted=False)
    try:
        r = c['fn'](log)
        verdict = 'NOT REACHED' if r['shown'] is None else 'LATENT' if r['shown'] == 'latent' else \
            'BUG SHOWN' if r['shown'] else 'NOT SHOWN'   # None: the harness never got to the code under test
        # (setup / input / breakpoint problem) - no evidence either way; 'latent': the faulty code cannot run in 1.31
        # (the case's observed text says why)
        obs = r['observed']
        extra = {n: ('NOT REACHED' if v[0] is None else 'BUG SHOWN' if v[0] else 'NOT SHOWN', v[1])
                 for n, v in r.get('also', {}).items()}
    except Exception as e:
        verdict, obs = 'ERROR', '%s: %s' % (type(e).__name__, e)
        extra = {}
        log.append(traceback.format_exc())
    finally:
        if mom.CASE.get('booted'):              # only a case that took the DOSBox lock (mom.lock) touched the save:
            mom.restore()                       # a static / file-reading run next to a game batch killed its DOSBox
            BOOTED[0] = True                    # twice on 10/24
    zst = sys.modules.get('cases_zst')
    if verdict in ('NOT REACHED', 'ERROR') and zst and num in zst.FALLBACK:   # the play case could not reach the
        shown, sobs = zst.fallback(num, log)    # code: its static check (cases_zst.FALLBACK) stands in, both texts kept
        if shown:
            verdict, obs = 'BUG SHOWN static', 'static fallback: %s | play case %s: %s' % (sobs, verdict, obs)
    os.makedirs(OUT, exist_ok=True)
    open(OUT + '%d.txt' % num, 'w', encoding='utf-8').write(
        '#%d %s (%s) %s\nsetup: %s\n1.31: %s\nfixed: %s\nverdict: %s\nobserved: %s\nscreenshots: %s\n\n%s\n'
        % (num, c['title'], c['kind'], stamp, c['setup'], c['bug'], c['fixed'], verdict, obs,
           ' '.join(mom.CASE['shots']) or '-', '\n'.join(log)))
    with open(OUT + 'results.tsv', 'a', encoding='utf-8') as f:
        for n in (num,) + c['also']:                # entries the same run shows: their own verdict if the case gave one
            v, o = extra.get(n, (verdict, obs))
            f.write('%d\t%s\t%s\t%s\t%s\n' % (n, c['kind'], stamp, v, ' | '.join(o.replace('\t', ' ').splitlines())))   # one line per run
    print('#%d %s: %s - %s' % (num, c['title'], verdict, obs), flush=True)
    return verdict


def fallback_only(nums):
    """the static fallback (cases_zst.FALLBACK) of entries whose LAST play run was NOT REACHED / ERROR, without
    replaying them: one results row 'BUG SHOWN static' citing that run"""
    zst = sys.modules['cases_zst']
    last = {}
    for l in open(OUT + 'results.tsv', encoding='utf-8'):
        p = l.rstrip('\n').split('\t')
        if p[0].isdigit():
            last[int(p[0])] = p
    stamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
    for n in nums:
        p = last.get(n)
        if n not in zst.FALLBACK or not p or p[3] not in ('NOT REACHED', 'ERROR'):
            print('#%d: skipped (no fallback row, or its last play run is %s)' % (n, p[3] if p else 'missing'))
            continue
        log = []
        shown, sobs = zst.fallback(n, log)
        v = 'BUG SHOWN static' if shown else 'NOT SHOWN'
        obs = 'static fallback: %s | play case %s (%s): %s' % (sobs, p[3], p[2], p[4])
        with open(OUT + 'results.tsv', 'a', encoding='utf-8') as f:
            f.write('%d\t%s\t%s\t%s\t%s\n' % (n, CASES[n]['kind'], stamp, v, obs.replace('\t', ' ')))
        open(OUT + '%d.txt' % n, 'a', encoding='utf-8').write('\n--- static fallback %s: %s\n%s\n%s\n'
                                                              % (stamp, v, obs, '\n'.join(log)))
        print('#%d: %s - %s' % (n, v, obs[:200]))


if __name__ == '__main__':
    import regress as R                        # the cases register into the imported module, not __main__
    R.load_cases()
    a = sys.argv[1:]
    if a == ['--list']:
        for n in sorted(R.CASES):
            print('%4d %-4s %s' % (n, R.CASES[n]['kind'], R.CASES[n]['title']))
        sys.exit(0)
    if a[:1] == ['--fallback']:                 # the static fallback alone, for entries whose play case's last run
        R.fallback_only([int(x) for x in a[1:]])   # (results.tsv) did not reach the code - no replay
        sys.exit(0)
    nums = sorted(n for n in R.CASES if R.CASES[n]['kind'] == a[1]) if a[:1] == ['--kind'] else [int(x) for x in a]
    for n in nums:
        R.run(n)
    if R.BOOTED[0]:
        try:
            import dosbox
            dosbox.stop()
        except Exception:
            pass
