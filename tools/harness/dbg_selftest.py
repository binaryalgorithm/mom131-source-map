"""Self-test of dbg.py's watchpoints, conditional breakpoints and tracepoints (one boot, about a minute):
the human hero next to Dracon steps one square east; a watch on its x, a tracepoint on Stack_Animate_Path, a
conditional breakpoint on Path_Walk_Check that never passes (counted as skipped) - both run on every move.
    python -P tools/harness/dbg_selftest.py"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cases_cb                                 # noqa: E402
import dbg                                      # noqa: E402
import mom                                      # noqa: E402


def main():
    s = mom.SaveFile()
    u = cases_cb._human_unit_at_dracon(s)
    key = mom.setup_attack(s, u, [], (30, 27, 0))        # an empty square: a plain move
    log = []
    d = dbg.boot(s, log=log)
    ok = []
    try:
        x0 = d.lv.get('units', u, 'x')
        d.watch('x', ('units', u, 'x'))
        d.want('Stack_Animate_Path', trace=lambda h: [d.arg(i, h) for i in range(4)])   # (player, plane, len, x)
        d.want('Path_Walk_Check', when=lambda h: False)        # a condition that never holds
        print('BPLIST after arming:', d._bp_lines())
        k = [0]

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                d.lv.D.key(key)
        h = d.wait(40, poll)
        print('hit:', None if h is None else {f: h[f] for f in ('name', 'kind', 'old', 'new', 'writer') if f in h})
        ok.append(h is not None and h.get('kind') == 'mem' and h['old'][0] == x0)
        d.unwatch('x')
        d.go()
        d.wait(8, None)                                  # let the move finish (tracepoints keep recording)
        print('traces:', d.traces, 'skipped:', d.skipped)
        ok.append(bool(d.traces.get('Stack_Animate_Path')) and d.skipped.get('Path_Walk_Check', 0) > 0)
        print('BPLIST after unwatch:', d._bp_lines())
        ok.append(not any('BPLM' in l or 'BPMEM' in l for l in d._bp_lines()))
        print('unit %d x %d -> %d' % (u, x0, d.lv.get('units', u, 'x')))
    finally:
        d.stop()
        print('\n'.join(log))
    print('SELFTEST', 'PASS' if all(ok) and len(ok) == 3 else 'FAIL %s' % ok)


if __name__ == '__main__':
    main()
