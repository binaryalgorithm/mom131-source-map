"""Breakpoints for the regression cases (P4 B4): the harness game under the nightly DOSBox-X with its debugger channel
(dbgchan.py), Game Link input / RAM as in mom.py. READ ONLY: BP / BPDEL / EV / RUN / SAVESTATE / LOADSTATE only.

  d = dbg.boot(save)                  write the save, start the debug DOSBox-X, wait for the overland map
  d.want('City_Max_Population')       a breakpoint at a function (entry, or + offset, or a 'seg:off' Ghidra address)
  hit = d.wait(30, poll=fn)           run until a wanted breakpoint stops the game -> {'name', 'cs', 'ip', regs...};
                                      poll() is called while running (to press keys); None on timeout
  d.arg(k) / d.word(seg, off) / d.ds_word(off)   the k-th word argument of a far function at its entry, memory
  d.go()                              resume (the breakpoint stays armed; d.drop(name) removes it)
  d.want('X', when=lambda h: d.arg(0, h) == 3)       conditional (checked in Python at each stop; false -> resumed)
  d.want('X', nth=5) / d.want('X', trace=fn)         the 5th passing stop / a tracepoint (fn(hit) -> d.traces['X'])
  d.watch('own', ('cities', 21, 'owner'))            memory-write watchpoint (BPLM per byte): wait() returns
                                                      {'kind': 'mem', 'old', 'new', 'writer': 'Function+off'}
  d.where(cs, ip)                     runtime address -> 'Function+off (ghidra seg:off)'

Overlay code (VROOMM) sits at a segment that changes with each load: a wanted address in an overlay that is not
loaded is armed through the overlay's resident thunks (every entry of its stub); when one of them is hit the
overlay map is refreshed, the real breakpoint set and the thunk breakpoints removed. Each poll re-reads the map, so
a breakpoint follows its overlay when it is loaded somewhere else."""
import json
import os
import re
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dbgchan                                 # noqa: E402
import dosbox                                  # noqa: E402
import mom                                     # noqa: E402

ROOT = mom.ROOT
DEBUG_CONF = ROOT + 'tools/harness/mom_dbg.conf'
REGS = ('AX', 'BX', 'CX', 'DX', 'SI', 'DI', 'BP', 'SP', 'DS', 'ES', 'SS', 'CS', 'IP')


def names():
    d = {}
    for line in open(ROOT + 're/names.tsv', encoding='utf-8'):
        f = line.split('\t')
        if len(f) > 1 and ':' in f[0]:
            s, o = f[0].split(':')
            d.setdefault(f[1], (int(s, 16), int(o, 16)))
    return d


class Dbg:
    def __init__(self, ch, lv, log=None):
        self.ch, self.lv = ch, lv
        self.log = log if log is not None else []
        self.am = dbgchan.AddrMap(dosbox.Mem())
        self.nm = names()
        self.flat = json.load(open(dbgchan.FLAT_MAP))
        self.wanted = {}          # name -> Ghidra (seg, off)
        self.armed = {}           # runtime (cs, ip) -> name (or '#thunk')
        self.hits = []
        self.thunk_owner = {}     # thunk (cs, ip) -> Ghidra overlay segment (no longer armed)
        self.ovr_exit = self.am.to_runtime(*self.ghidra('Ovr_Int3F_Handler', 0x5c))
        self.running = True       # breakpoints take effect when the debugger RESUMES: set them while stopped
        self.ovr_always = False   # the overlay-load stop costs a round trip per load, and the overland map loads
        self.idle_map = True      # overlays all the time: armed only off the idle map unless ovr_always
        self.busy_until = 0
        self.cond = {}            # label -> {'when', 'nth', 'trace', 'seen'}: conditions checked in Python per stop
        self.traces = {}          # label -> [trace(hit) results] (tracepoints: recorded, the game resumed)
        self.skipped = {}         # label -> stops whose condition was false (resumed at once)
        self.watches = {}         # label -> {'lin', 'size', 'last', 'signed'}: memory-write watchpoints (BPLM)
        self.pending_load = False  # stopped at a thunk of a wanted module that is not loaded: the overlay-manager
        #                           exit stop stays armed until a wanted overlay is in (then its breakpoints go in
        #                           before its code runs - the first entry's calls were missed before, 10/23)
        self.follow = set()       # Ghidra overlay segments whose thunks stay armed even while loaded: every entry
        #                           from outside then re-places the breakpoints (a loaded overlay can be moved by
        #                           another's load right before the call - #245, 10/23); opt-in: a hot module stalls
        self.thunks = True        # overlay breakpoints follow their overlay through its stub thunks (a stop per
        #                           entry of the module from outside) instead of the overlay-manager stop (a stop per
        #                           load of ANY overlay: a turn change crawled, 10/23); calls inside the module are
        #                           caught once it has been entered. thunks = False: the old overlay-manager stop

    # ---- addresses
    def ghidra(self, name, off=0):
        if ':' in name:
            s, o = (int(x, 16) for x in name.split(':'))
        else:
            s, o = self.nm[name]
        return s, o + off

    def _thunks(self, gseg):
        for o in self.flat['overlays']:
            if o['ovl_seg'] + 0x1000 == gseg:
                for e in o['entries']:
                    ts, to = (int(x, 16) for x in e['thunk'].split(':'))
                    yield ts + 0x1000 - self.am.delta, to
        return

    def _sync(self, desired):
        """Make DOSBox-X's breakpoints exactly `desired` {(cs, ip): tag}, changing only what differs: a breakpoint
        deleted and set again where the game is stopped makes RUN stop there again at once (seen 10/23 at the
        overlay-manager stop), so unchanged ones are left alone. BPDEL n takes the number BPLIST shows."""
        for k in [k for k in self.armed if k not in desired]:
            for line in self._bp_lines():
                if line.split()[-1].upper() == '%04X:%04X' % k and 'BPMEM' not in line:
                    self.ch.ex('BPDEL %s' % line.split('.')[0].strip())
                    break
            del self.armed[k]
        for k, v in desired.items():
            if k not in self.armed:
                self.ch.ex('BP %04X %04X' % k)
            self.armed[k] = v

    def _bp(self, cs, ip, tag):
        if (cs, ip) not in self.armed:
            d = dict(self.armed)
            d[(cs, ip)] = tag
            self._sync(d)

    def _del(self, cs, ip):
        d = dict(self.armed)
        d.pop((cs, ip), None)
        self._sync(d)

    def want(self, name, off=0, label=None, when=None, nth=1, trace=None):
        """A code breakpoint (function NAME, + off, or a 'seg:off' Ghidra address). Conditions run in Python at each
        stop (DOSBox-X's debugger has none): when(hit) false -> resumed at once (counted in self.skipped); nth = report
        only the nth stop that passes; trace(hit) -> its result appended to self.traces[label] and the game resumed
        (a tracepoint: wait() never returns it). hit = the registers + 'name'; read with arg(k, hit) / local(...).
        Returns the label."""
        label = label or (name if not off else '%s+%x' % (name, off))
        self.wanted[label] = self.ghidra(name, off)
        self._set_cond(label, when, nth, trace)
        self.arm()
        return label

    def _set_cond(self, label, when, nth, trace):
        if when is None and nth == 1 and trace is None:
            self.cond.pop(label, None)
        else:
            self.cond[label] = {'when': when, 'nth': nth, 'trace': trace, 'seen': 0}

    def _passes(self, label, hit):
        """False: resume (condition false, not yet the nth, or a tracepoint that recorded)."""
        c = self.cond.get(label)
        if c is None:
            return True
        if c['when'] is not None and not c['when'](hit):
            self.skipped[label] = self.skipped.get(label, 0) + 1
            return False
        if c['trace'] is not None:
            self.traces.setdefault(label, []).append(c['trace'](hit))
            return False
        c['seen'] += 1
        return c['seen'] >= c['nth']

    def where(self, cs, ip):
        """Runtime CS:IP -> 'Function+off (ghidra seg:off)', or the raw address. Boundaries from Ghidra's
        functions.tsv, the NAME from re/names.tsv (the project's current names: functions.tsv still had
        'Diplomacy_Relation_Change' for Diplo_Relation_Change, and #66's writer filter missed it, 10/23)."""
        import bisect
        import whowrites
        if whowrites.FN is None:
            whowrites.fn_at((0, 0))
        if not hasattr(self, '_byaddr'):
            self._byaddr = {v: k for k, v in self.nm.items()}
        self.am.refresh()
        g = self.am.to_ghidra(cs, ip)
        if g is None:
            return '%04X:%04X (not mapped)' % (cs, ip)
        keys = [k for k, _, _ in whowrites.FN]
        i = bisect.bisect_right(keys, g) - 1
        if i < 0 or whowrites.FN[i][0][0] != g[0]:
            return '? (%04X:%04X)' % g
        (s, o), name, size = whowrites.FN[i]
        name = self._byaddr.get((s, o), name)
        return '%s+%X%s (%04X:%04X)' % (name, g[1] - o, '' if g[1] - o < size + 16 else ' (past end?)', g[0], g[1])

    # ---- memory-write watchpoints
    def addr_of(self, what):
        """Linear address and size of: ('table', i, 'field.path') (mom.LIVE tables: cities, units, wizards...),
        a global name ('num_units'), 'seg:off' (runtime) or an int (linear)."""
        if isinstance(what, int):
            return what, 2
        if isinstance(what, tuple):
            table, i, path = what
            ptr, s = mom.LIVE[table]
            fo, es, c, t = mom.resolve(s, path)
            return self.lv.base(table) + i * mom.L.size(s) + fo, es * c
        if ':' in what:
            sg, o = (int(x, 16) for x in what.split(':'))
            return sg * 16 + o, 2
        ds, t, dims = mom.SYM[what]
        es = 4 if 'far' in t and '*' in t else (2 if '*' in t else mom.L._type_size(t, False))
        return self.lv.g.ds + ds, es

    def _read(self, w):
        return self.lv.g.ram(w['lin'], w['size'])

    def watch(self, label, what, size=None, when=None, trace=None):
        """Stop when any byte of WHAT changes (see addr_of; size overrides, at most 8 bytes): one DOSBox-X linear
        memory breakpoint (BPLM) per byte. The debugger stops right AFTER the writing instruction; wait() returns
        {'name': label, 'kind': 'mem', 'old', 'new' (bytes), 'writer' (function+off), registers}. when(hit) false
        -> resumed; trace(hit) -> recorded in self.traces[label] and resumed. Linear addresses above 1 MB work too
        (EMS pages live there; the page frame maps them). Returns the label."""
        lin, sz = self.addr_of(what)
        sz = min(size or sz, 8)
        w = {'lin': lin, 'size': sz}
        w['last'] = self._read(w)
        self.watches[label] = w
        self._set_cond(label, when, 1, trace)
        was = self.running
        if was:
            self.ch.req('BREAK')
            self.running = False
        for k in range(sz):
            self.ch.ex('BPLM %08X' % (lin + k))
        if was:
            self.go()
        return label

    def unwatch(self, label):
        w = self.watches.pop(label, None)
        if w is None:
            return
        want = {'%08X' % (w['lin'] + k) for k in range(w['size'])}
        for _ in range(w['size']):
            for line in self._bp_lines():
                if 'BPLM' in line or 'BPMEM' in line:
                    if any(a in line.upper() for a in want):
                        self.ch.ex('BPDEL %s' % line.split('.')[0].strip())
                        break
        self.cond.pop(label, None)

    def _mem_hit(self):
        """A watched value changed and the CPU is stopped: the hit (or None)."""
        for label, w in self.watches.items():
            now = self._read(w)
            if now == w['last']:
                continue
            a = self.csip()
            time.sleep(0.05)
            old, w['last'] = w['last'], now
            if self.csip() != a:                    # changed while running (the breakpoint not active yet)
                continue
            self.running = False
            r = self.regs()
            r.update(name=label, kind='mem', old=old, new=now, writer=self.where(a[0], a[1]))
            return r
        return None

    def want_line(self, src, text, nth=0, label=None, fn=None, **cond):
        """A breakpoint where the source line containing TEXT starts (srcmap.py: the game's offset of that line in
        its function; fn = only that function's copies). Returns the label."""
        import srcmap
        pl = [p for p in srcmap.places(src, text, only=fn) if fn is None or p[0] == fn]
        fn, off, a = pl[nth]
        label = label or '%s:%s' % (os.path.basename(src), text[:30])
        return self.want(a, off, label, **cond)          # cond: when / nth / trace as in want()

    def drop(self, label):
        if label in self.watches:
            self.unwatch(label)
            return
        self.cond.pop(label, None)
        self.wanted.pop(label, None)
        for k, v in list(self.armed.items()):
            if v == label:
                self._del(*k)

    def arm(self):
        """Set what can be set now: loaded / resident addresses directly, else the overlay's thunks. Stale
        breakpoints of an overlay that moved are removed. DOSBox-X activates breakpoints when the debugger resumes,
        so a running game is stopped (BREAK) for the change and resumed."""
        was = self.running
        if was:
            self.ch.req('BREAK')
            self.running = False
            at = self.csip()
            if at in self.armed:                    # a breakpoint fired just before the BREAK: stay there, wait()
                self._arm()                         # picks it up (resuming here would skip the hit) - unless the
                if at in self.armed:                # new set no longer holds it (the overlay stop disarmed on the
                    return                          # idle map): then resume, or the game waits at an unknown stop
                self.go(at)
                return
        self._arm()
        if was:
            self.go()

    def _arm(self):
        """Breakpoints = the wanted addresses whose code is in memory now (resident, or an overlay at its current
        load segment) + '#ret' + one permanent '#ovr' at the overlay manager's exit (Ovr_Int3F_Handler + 0x5c,
        resident) while any wanted address is in an overlay: every overlay load passes there just before its code
        runs, and each stop there re-places the breakpoints at the overlay's new segment (an overlay is evicted and
        reloaded elsewhere often; calls inside one overlay never pass its stub thunks)."""
        self.am.refresh()
        desired = {k: v for k, v in self.armed.items() if v == '#ret'}
        in_ovl = False
        for label, (sg, o) in self.wanted.items():
            r = self.am.to_runtime(sg, o)
            if sg >= self.am.resident_end:
                in_ovl = True
                if self.thunks and (r is None or sg in self.follow):   # not loaded (or followed): every entry of
                    #                           the module from outside stops at a
                    for th in self._thunks(sg):     # thunk, and the real breakpoints are placed then. Loaded: real
                        desired.setdefault(th, '#thunk')   # breakpoints only - a hot module (TERRAIN.C's tile
                        #                                   helpers) stopped the map at every redraw (#353, 10/23);
                        #                                   the per-second stale check re-arms after an eviction
            if r is not None:
                desired[r] = label
        if in_ovl and (self.ovr_always or self.pending_load or (not self.thunks and not self.idle_map)):
            desired[self.ovr_exit] = '#ovr'
        self._sync(desired)

    def _thunk_unloaded(self, cs, ip):
        """The wanted overlay whose thunk is at (cs, ip) is not loaded now."""
        for sg in {g for g, _ in self.wanted.values()}:
            if (cs, ip) in set(self._thunks(sg)):
                return self.am.to_runtime(sg, 0) is None
        return False

    def _load_watch(self, stub_cs, gseg=None):
        """Stopped at a thunk of an overlay that is not loaded: its INT 3Fh traps into Ovr_Int3F_Handler, which loads
        the overlay, patches the stubs and returns into the stub (UI_MAP / names.tsv). A breakpoint at the handler's
        register restore (Ovr_Int3F_Handler + 0x5c, resident) stops there once the overlay is in; wait() then sets
        the real breakpoints."""
        cs, ip = self.am.to_runtime(*self.ghidra('Ovr_Int3F_Handler', 0x5c))
        self._bp(cs, ip, '#ovr')

    def wanted_stubs(self):
        """Runtime stub segments of the overlays that hold a wanted address."""
        segs = {sg for sg, _ in self.wanted.values()}
        return {stub - self.am.delta for stub, gseg, _ in self.am.ovls if gseg in segs}

    def _wanted_moved(self):
        """True when a wanted overlay address is no longer where it is armed: loading ANOTHER overlay can move a
        loaded one (VROOMM compacts its buffer; seen 10/23 in #127: City_Change_Owner's overlay 4A1C -> 4AB3 while
        an unwanted overlay loaded, so its entry breakpoint sat at the old segment and the call was missed)."""
        self.am.refresh()
        for label, (sg, o) in self.wanted.items():
            r = self.am.to_runtime(sg, o)
            if r is not None and self.armed.get(r) != label:
                return True
        return False

    def _bp_lines(self):
        """BPLIST's breakpoint lines. The numbers are HEX ('2E. BP 1234:00C5'): the old decimal test dropped every
        line numbered with a letter, so those breakpoints could not be deleted and kept stopping the game as 'unknown
        stops' (#353, 10/23: a Next_Turn module thunk)."""
        return [l for l in self.ch.ex('BPLIST')[1] if re.match(r'[0-9A-Fa-f]+\. ', l)]

    def _stale(self):
        """True when a WANTED overlay was loaded, moved or evicted since the breakpoints were placed (other overlays
        change all the time on the map: re-arming for them stopped and resumed the game every second)."""
        segs = {sg for sg, _ in self.wanted.values()}
        old = {g: s for g, s in self.am.loaded.items() if g in segs}
        self.am.refresh()
        return {g: s for g, s in self.am.loaded.items() if g in segs} != old

    # ---- run / stop
    def csip(self):
        v = self.ch.ex('EV CS IP')[1][-1].split()
        return int(v[0], 16), int(v[1], 16)

    def regs(self):
        out = {}
        for k in range(0, len(REGS), 7):
            part = REGS[k:k + 7]
            v = self.ch.ex('EV ' + ' '.join(part))[1][-1].split()
            out.update({r: int(x, 16) for r, x in zip(part, v)})
        return out

    def go(self, was_bp=None):
        """RUN. Resumed from where a breakpoint stopped the game (set, just deleted, or unknown), DOSBox-X executes
        one instruction and stops again (seen 10/23 at the overlay-manager stop: 2A67:0550 -> 0551): that stop is
        detected and resumed at once. was_bp is accepted for older callers; the check always runs."""
        at = self.csip()
        self.ch.ex('RUN')
        self.running = True
        cs, ip = self.csip()                    # no sleeps: this runs at every overlay-manager stop, and on the map
        if cs == at[0] and 0 < ip - at[1] <= 6 and (cs, ip) not in self.armed:   # overlays load all the time (a
            if self.csip() == (cs, ip):                                           # slow go() stalled #156 / #157)
                self.ch.ex('RUN')

    def wait(self, timeout=60, poll=None, poll_every=1.0):
        t0 = last = time.time()
        prev, same = None, 0
        while time.time() - t0 < timeout:
            if self.watches:
                m = self._mem_hit()
                if m is not None:
                    if len(self.hits) < 40:
                        self.log.append('  write %s: %s -> %s by %s turn %d' % (
                            m['name'], m['old'].hex(), m['new'].hex(), m['writer'], self.lv.turn()))
                    if not self._passes(m['name'], m):
                        self.go()
                        continue
                    self.hits.append(m)
                    return m
            cs, ip = self.csip()
            tag = self.armed.get((cs, ip))
            if tag == '#ovr':                       # fast path: ES = the stub of the overlay just loaded; for one
                es = int(self.ch.ex('EV ES')[1][-1].split()[0], 16)   # we do not want, resume at once
                if es not in self.wanted_stubs() and not self._wanted_moved():
                    self.go()                       # go(): absorbs the stop one instruction on (a raw RUN left
                    continue                        # 2A67:0551 stops, each resumed only after ~1 s)
            if tag == '#thunk':                     # an entry of a wanted overlay's module from outside: the overlay
                if self.csip() == (cs, ip):          # is in (or the loader re-runs this thunk once it is): place the
                    self.running = False            # real breakpoints at its segment now, then on
                    before = dict(self.am.loaded)
                    self._arm()
                    if self.armed.get((cs, ip)) == '#thunk' and self._thunk_unloaded(cs, ip):   # not loaded: its
                        #                                     INT 3Fh loads it next - stop
                        self.pending_load = True             # at the loader's exit to arm before its code runs
                        self._arm()
                    if self.am.loaded != before and len(self.hits) < 40:
                        self.log.append('  module entered at %s: breakpoints %s' % (
                            self.where(cs, ip), ['%04X:%04X %s' % (k + (v,)) for k, v in self.armed.items()
                                                  if v != '#thunk']))
                    self.go()
                    continue
            if tag is not None:
                time.sleep(0.15)
                if self.csip() == (cs, ip):          # really stopped there
                    self.running = False
                    if tag in self.wanted:          # an overlay evicted since: other code at the armed address
                        self.am.refresh()
                        if self.am.to_runtime(*self.wanted[tag]) != (cs, ip):
                            self._arm()
                            self.go()
                            continue
                    if tag == '#ovr':                   # an overlay was just loaded: re-place the breakpoints
                        self.pending_load = False
                        self._arm()
                        if len(self.hits) < 40:
                            self.log.append('  wanted overlay loaded: %s; breakpoints %s' % (
                                {'%04x' % g: '%04X' % sg for g, sg in self.am.loaded.items()
                                 if g in {w[0] for w in self.wanted.values()}},
                                ['%04X:%04X %s' % (k + (v,)) for k, v in self.armed.items()]))
                        self.go()
                        continue
                    r = self.regs()
                    r['name'] = tag
                    if tag != '#ret' and not self._passes(tag, r):
                        self.go()
                        continue
                    self.hits.append(r)
                    if len(self.hits) <= 40:
                        self.log.append('  stop %s at %04X:%04X (AX %04X SI %04X DI %04X) turn %d' % (
                            tag, cs, ip, r['AX'], r['SI'], r['DI'], self.lv.turn()))
                    return r
            same = same + 1 if (cs, ip) == prev else 0
            prev = (cs, ip)
            if same >= 20 and (cs, ip) not in self.armed:   # ~1 s stopped where no breakpoint of ours is: resume
                self.log.append('  resumed an unknown stop at %04X:%04X = %s; BPLIST %s; armed %s' % (
                    cs, ip, self.where(cs, ip), self._bp_lines(),
                    ['%04X:%04X %s' % (k + (v,)) for k, v in self.armed.items()]))
                self.go((cs, ip))
                same = 0
            if time.time() - last > poll_every:
                last = time.time()
                idle = mom.on_map(self.lv) and time.time() > self.busy_until
                if idle != self.idle_map:           # left / back on the idle map: (dis)arm the overlay stop
                    self.idle_map = idle
                    self.arm()
                if self._stale():
                    self.arm()
                if poll:
                    poll()
            time.sleep(0.05)
        return None

    # ---- memory (Game Link RAM, read only)
    def word(self, seg, off, signed=True):
        return struct.unpack('<h' if signed else '<H', self.lv.g.ram(seg * 16 + off, 2))[0]

    def start_combat(self, key, timeout=60):
        """mom.start_combat for a debugger session: the key that moves the stack onto the enemy, Y for a treaty
        question, until the human's combat turn - waiting through d.wait, which resumes the unknown debugger stops
        (F000:D186) a battle against an AI wizard produces; the plain helper froze there (#196 / #326, 10/24)."""
        import mom
        self.lv.D.key(key)
        t0, k = time.time(), 0
        while time.time() - t0 < timeout:
            self.wait(1.5, None)
            if mom.in_combat(self.lv):
                self.wait(1.5, None)
                return True
            k += 1
            if k % 3 == 0:
                self.lv.D.key('y')
            if k % 4 == 2 and mom.on_map(self.lv):  # still on the map with the stack selected: the key was lost
                self.action()                       # (sent while a debugger stop held the game: #326's second
                self.lv.D.key(key)                  # battle, 10/24) - send it again
        try:
            mom.shot('no_combat_turn', self.log)    # what is on screen instead (#196 / #326)
        except Exception:
            pass
        raise RuntimeError('no combat turn for the human (screen %d, battle units %d)'
                           % (self.lv.screen(), self.lv.glob('battle_unit_count')))

    def wait_via_entry(self, fn, timeout=60, poll=None):
        """wait() for the wanted lines inside FN, a function that calls other overlays before them (its own overlay
        can move under the breakpoints, README sec 4): a stop at FN's entry first; from there d.follow its module and
        the overlay-load stop for the rest of the call. Returns the first other hit, or None."""
        label = '#entry ' + fn
        self.want(fn, label=label)
        self.follow = set(self.follow) | {self.ghidra(fn)[0]}
        t0 = time.time()
        while time.time() - t0 < timeout:
            h = self.wait(timeout - (time.time() - t0), poll)
            if h is None or h['name'] != label:
                return h
            self.drop(label)
            self.ovr_always = True
            self.arm()
            self.go()
        return None

    def dump(self, seg, off, n):
        """n bytes at seg:off as the CPU sees them (the debugger's MEMDUMPBIN, while stopped): the EMS page frame
        included, which Game Link's RAM view does not map (p_path_work's cost map read 255 everywhere, #256)."""
        path = ROOT + 'work/harness/dbg/dump.bin'
        if os.path.exists(path):
            os.remove(path)
        was = self.running                      # a running game is stopped for the dump and resumed
        if was:
            self.ch.req('BREAK')
            self.running = False
        try:
            self.ch.ex('MEMDUMPBIN %04X:%04X %X %s' % (seg, off, n, path.replace('/', '\\')))
            for _ in range(20):
                if os.path.exists(path) and os.path.getsize(path) >= n:
                    return open(path, 'rb').read()[:n]
                time.sleep(0.1)
            raise RuntimeError('MEMDUMPBIN %04X:%04X wrote nothing' % (seg, off))
        finally:
            if was:
                self.go()

    def arg(self, k, r=None):
        """At a far function's entry (before push bp): word argument k (0-based) at SS:SP + 4 + 2k."""
        r = r or self.regs()
        return self.word(r['SS'], r['SP'] + 4 + 2 * k)

    def local(self, disp, r=None):
        """[bp + disp] inside the function (after its prologue)."""
        r = r or self.regs()
        return self.word(r['SS'], (r['BP'] + disp) & 0xffff)

    def ret_ax(self, timeout=10):
        """From a function entry: break at the far return address, run there, return AX (signed)."""
        r = self.regs()
        ret_ip, ret_cs = self.word(r['SS'], r['SP'], False), self.word(r['SS'], r['SP'] + 2, False)
        self._bp(ret_cs, ret_ip, '#ret')
        self.go()                                   # stopped at the entry: the new breakpoint is active on RUN
        t0 = time.time()
        while time.time() - t0 < timeout:
            if self.csip() == (ret_cs, ret_ip):
                time.sleep(0.1)
                if self.csip() == (ret_cs, ret_ip):
                    ax = self.regs()['AX']
                    self._del(ret_cs, ret_ip)
                    return ax - 0x10000 if ax & 0x8000 else ax
            time.sleep(0.03)
        cs, ip = self.csip()
        self.log.append('  return to %04X:%04X not reached in %d s (now %04X:%04X, %s)' % (
            ret_cs, ret_ip, timeout, cs, ip, self.armed.get((cs, ip), 'not a breakpoint')))
        self._del(ret_cs, ret_ip)
        return None

    def call(self, name, where=None, nargs=3, timeout=120, poll=None):
        """Wait for a call of function NAME whose word arguments satisfy where(args) (any call if None), run it to
        its return: (args, AX). The game is left stopped at the return (go() resumes)."""
        self.want(name)
        t0 = time.time()
        while time.time() - t0 < timeout:
            h = self.wait(timeout - (time.time() - t0), poll)
            if h is None:
                break
            if h['name'] != name:
                self.go()
                continue
            args = [self.arg(k, h) for k in range(nargs)]
            if where is None or where(args):
                ax = self.ret_ax()
                self.log.append('  %s%s = %s' % (name, tuple(args), ax))
                return args, ax
            self.go()
        self.log.append('  %s: no matching call in %d s' % (name, timeout))
        return None, None

    def action(self, hold=6):
        """Call right before the key / click that starts the action under test: the overlay-load stop is armed
        now (the idle-map check is slower than a turn change) and kept for `hold` seconds at least."""
        self.busy_until = time.time() + hold
        if self.idle_map:
            self.idle_map = False
            self.arm()

    def end_turn_poll(self):
        """A poll() that ends the current human turn (Done / Next Turn, dialogs answered as in turns_poll)."""
        return self.turns_poll(self.lv.turn() + 1)

    def press(self, key=None, click=None, settle=0.05):
        """A key (or click (x, y)) with the overlay-manager stop OFF, then armed (as action(): the input may start
        the action under test). For busy screens: with the stop armed the end-turn screen (10), which reloads
        overlays ~2.5 times a second, lost N and the Next Turn click in 3 tries; disarmed, N worked at once (10/23).
        On the map keys arrive while it is armed: use action() + the key there (arming only AFTER the key let a whole
        turn change run without the overlay stop and miss every breakpoint)."""
        if self.ovr_exit in self.armed:
            self.idle_map, self.busy_until = True, 0
            self.arm()
        if click is not None:
            self.lv.g.click(*click)
        else:
            self.lv.D.key(key)
        time.sleep(settle)
        self.action()

    def turns_poll(self, last_turn=None):
        """A poll() that keeps ending turns (until game_turn reaches last_turn): Done / Next Turn on the map and the
        end-turn screen, and what mom.end_turn answers elsewhere (items award, then Y / Enter / Esc in turn).
        A tracepoint on Next_Turn's entry tells when the turn change has started: game_turn++ comes near Next_Turn's
        END (NEXTTURN.C) and the end-turn screen (10) stays up throughout, so neither shows it. While it runs nothing
        is pressed (each press drops the overlay stop for a moment, and a stray N would end the next turn too)."""
        if 'Next_Turn' not in self.wanted:
            self.want('Next_Turn', trace=lambda h: self.lv.turn())
        k = [0, 0, 0, 0.0]                      # waits, answers, Next Turn tries, time of the last try
        lv = self.lv

        def changing():
            """The turn change of the current game_turn has started (and the next turn's map is not up yet)."""
            started = self.traces.get('Next_Turn', [])
            t = lv.turn()
            return bool(started) and (started[-1] == t or (started[-1] == t - 1 and not mom.on_map(lv)))

        def next_turn(busy):
            if time.time() - k[3] < 8:          # give the last press time to start Next_Turn
                return
            k[3] = time.time()
            k[2] += 1                           # the Next Turn panel / screen: N in one state, only a click in
            n_or_click = ('n', None) if k[2] % 2 else (None, (283, 187))   # another (#162: 400 s on N alone)
            if busy:                            # the end-turn screen: input only with the overlay stop off
                self.press(*n_or_click)
            else:
                self.action()                   # the map: arm first, then the input
                if n_or_click[0]:
                    lv.D.key('n')
                else:
                    lv.g.click(283, 187)

        def poll():
            if last_turn is not None and lv.turn() >= last_turn:
                return
            if changing() and lv.screen() == mom.N('SCREEN_END_TURN') and lv.glob('ui_control_count') <= 1:
                k[0] += 1                       # the turn change runs - or a message box waits mid-turn (one
                if k[0] % 3 == 0:               # whole-screen control: #136's 'Spell Blast ... fizzle' waited before
                    self.press('esc')           # the payment under test): Esc ends UI_Wait_Any_Input and does
                return                          # nothing on screen 10 / the map. A choice drawn mid-turn (the
                                                # research book, #162) has more controls: answered below
            if changing() and not mom.on_map(lv):
                answer()                        # never Next Turn / Done while the turn change runs
                return
            if mom.on_map(lv) and not changing():
                if lv.glob('no_units_to_move'):
                    next_turn(False)
                else:
                    self.action()               # 'd' on the last unit ends the turn by itself, too: arm first
                    lv.D.key('d')
                k[0] = 0
                return
            if lv.screen() == mom.N('SCREEN_END_TURN'):
                next_turn(True)
                k[0] = 0
                return
            if mom.on_map(lv):
                return
            answer()

        def answer():
            k[0] += 1
            if k[0] % 2 == 0 and not mom.items_award(lv):
                # mom.end_turn's answers: Y first ('allow the units to die?'), Enter, Esc, the research book's first
                # entry (60,33), a message's OK spot (255,185), a Confirm_Dialog's Yes (202,132)
                a = (('y', None), ('enter', None), ('esc', None), (None, (60, 33)), (None, (255, 185)),
                     (None, (202, 132)))[k[1] % 6]
                self.press(*a)
                k[1] += 1
        return poll

    def stop(self):
        try:
            self.ch.ex('BPDEL *')
            self.go()
        except Exception:
            pass
        try:
            self.ch.close()
        finally:
            dosbox.stop()


kill_harness = mom.kill_harness


def boot(save=None, wait=60, log=None):
    """mom.boot with the debug DOSBox-X: the debugger channel connected, the game on the overland map, running."""
    mom.lock()
    (save or mom.SaveFile()).write()
    try:
        dosbox.stop()
    except Exception:
        pass
    kill_harness()
    time.sleep(1)
    ch = dbgchan.Chan()
    dosbox.start(DEBUG_CONF, exe=dosbox.EXE_NIGHTLY)
    ch.accept()
    t0 = time.time()
    lv = None
    while time.time() - t0 < wait:
        time.sleep(1)
        try:
            lv = lv or mom.Live()
            if lv.screen() == mom.N('SCREEN_MAP'):
                time.sleep(1.5)
                return Dbg(ch, lv, log)
        except Exception:
            lv = None
    ch.close()
    raise RuntimeError('the overland map did not come up (debug)')
