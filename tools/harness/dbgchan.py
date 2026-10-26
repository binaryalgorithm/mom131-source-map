"""DOSBox-X debugger control channel (mcp_server option, DOSBox-X 2026.10.01+; README.debugger upstream).

DOSBox-X is the TCP CLIENT: with mcp_server=<port> in [dosbox] it connects to 127.0.0.1:<port> and retries until a
listener is there. Requests are text lines; replies are blocks:
    REQ <id> PING | BREAK | EXEC <debugger command>
    BEGIN <id> OK|ERR / output lines / END <id>
EXEC runs one interactive-debugger command (BP, BPINT, LOGC, MEMDUMPBIN, EV, RUN, ...). RUN / RUNWATCH / VRT are
acknowledged before emulation resumes. No asynchronous breakpoint events: poll (see stopped()).

Harness use: python tools/harness/dbgchan.py smoke   (starts a debug harness DOSBox on the hidden desktop, runs a
short scripted session, stops it). Game RAM is never written: no SM / SMV / FM / SR commands are sent.
"""
import json
import os
import socket
import sys
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

PORT = 58991
ROOT = _mp.ROOT
DEBUG_CONF = ROOT + 'tools/harness/mom_debug.conf'


class Chan:
    def __init__(self, port=PORT, timeout=60):
        self.ls = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.ls.bind(('127.0.0.1', port))
        self.ls.listen(1)
        self.ls.settimeout(timeout)
        self.s = None
        self.buf = b''
        self.n = 0

    def accept(self):
        self.s, _ = self.ls.accept()
        self.s.settimeout(30)

    def _line(self):
        while b'\n' not in self.buf:
            d = self.s.recv(65536)
            if not d:
                raise EOFError('channel closed')
            self.buf += d
        line, self.buf = self.buf.split(b'\n', 1)
        return line.decode('latin-1').rstrip('\r')

    def req(self, what):
        """what = 'PING' | 'BREAK' | 'EXEC <cmd>' -> (ok, [lines])"""
        self.n += 1
        rid = str(self.n)
        self.s.sendall(('REQ %s %s\n' % (rid, what)).encode('latin-1'))
        ok, out = None, []
        while True:
            line = self._line()
            if line.startswith('BEGIN %s ' % rid):
                ok = line.endswith('OK')
            elif line == 'END %s' % rid:
                return ok, out
            elif ok is not None:
                out.append(line)
            # other lines (e.g. 'BEGIN event ...' blocks) are skipped before our BEGIN

    def ex(self, cmd):
        return self.req('EXEC ' + cmd)

    def close(self):
        for s in (self.s, self.ls):
            try:
                s and s.close()
            except OSError:
                pass


GHIDRA_DGROUP = 0x36AA          # DGROUP in our Ghidra project (image loaded at 1000:0000)
FLAT_MAP = ROOT + 'work/flat/WIZARDS_FLAT_map.json'


class AddrMap:
    """Runtime <-> Ghidra addresses for the running game (read-only RAM access through dosbox.Mem).
    Resident code (Ghidra segment < 0x4558): Ghidra = runtime + delta, delta = 0x36AA - runtime DGROUP segment.
    Overlay code (VROOMM): each overlay's stub (resident) holds 5-byte thunks at +0x20; while the overlay is loaded the
    manager rewrites them to EA off seg (JMP FAR) - seg = where the overlay sits now. Offsets inside an overlay are the
    same as in WIZARDS_FLAT.EXE, whose overlay segment = map ovl_seg + 0x1000."""

    def __init__(self, mem=None, ghidra_dgroup=GHIDRA_DGROUP, flat_map=FLAT_MAP):
        """MAGIC.EXE: AddrMap(mem, 0x31D1, ROOT + 'work/flat/MAGIC_FLAT_map.json')."""
        if mem is None:
            sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
            import dosbox
            mem = dosbox.Mem()
        self.mem = mem
        self.flat_map = flat_map
        self.delta = ghidra_dgroup - mem.ds_seg
        m = json.load(open(flat_map))
        self.resident_end = 0x1000 + m['resident_paras']
        self.ovls = [(o['stub_seg'] + 0x1000, o['ovl_seg'] + 0x1000, o['code_size']) for o in m['overlays']]
        self.loaded = {}

    def lin(self, seg, off, n):
        return self.mem.read(self.mem.base + seg * 16 + off, n)

    def refresh(self):
        """{ghidra overlay seg: runtime seg} for the overlays loaded right now."""
        self.loaded = {}
        for stub, gseg, _ in self.ovls:
            # the stub header's word +0x10 = the overlay's load segment, 0 when not loaded (Ovr_Unload clears it).
            # Checked live 10/23 (P4 B4): it equals the segment of every patched thunk - but the thunks are
            # patched one by one as they are called, so the old test of the first thunk (+0x20) missed loaded
            # overlays
            h = self.lin(stub - self.delta, 0x10, 2)
            seg = h[0] | h[1] << 8
            if seg:
                self.loaded[gseg] = seg
        return self.loaded

    def to_runtime(self, gseg, off):
        if gseg < self.resident_end:
            return gseg - self.delta, off
        if gseg in self.loaded:
            return self.loaded[gseg], off
        return None                                 # overlay not loaded now

    def to_ghidra(self, rseg, off):
        g = rseg + self.delta
        if g < self.resident_end:
            return g, off
        for gseg, r in self.loaded.items():
            if r == rseg:
                return gseg, off
        return None

    def stub_thunk(self, gseg, off):
        """Runtime address of the resident thunk that enters overlay function gseg:off from outside (stable BP spot;
        calls from inside the same overlay do not pass it)."""
        m = json.load(open(self.flat_map))
        for o in m['overlays']:
            if o['ovl_seg'] + 0x1000 == gseg:
                for e in o['entries']:
                    ts, to = (int(x, 16) for x in e['thunk'].split(':'))
                    gs, go = (int(x, 16) for x in e['target'].split(':'))
                    if go == off:
                        return ts + 0x1000 - self.delta, to
        return None


def save_state(ch, path):
    """Debugger stopped (BREAK first). path must end in lowercase .sav. Needs the nightly build (dosbox.EXE_NIGHTLY)."""
    return ch.ex('SAVESTATE ' + path)


def load_state(ch, path):
    return ch.ex('LOADSTATE ' + path)


def smoke(wait=25):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import dosbox
    ch = Chan()
    st = dosbox.start(DEBUG_CONF)
    print('started pid', st['pid'])
    try:
        ch.accept()
        print('connected')
        print('PING', ch.req('PING'))
        time.sleep(wait)                                 # let WIZARDS.EXE reach its first screen
        print('BREAK', ch.req('BREAK'))
        for c in ('EV CS', 'EV IP', 'EV DS', 'BPLIST', 'LOGC 20000', 'EV CS', 'EV IP'):
            ok, out = ch.ex(c)
            print(c, ok, out[:6])
        print('RUN', ch.ex('RUN'))
    finally:
        time.sleep(1)
        ch.close()
        dosbox.stop()


if __name__ == '__main__':
    if sys.argv[1] == 'smoke':
        smoke(int(sys.argv[2]) if len(sys.argv) > 2 else 25)
