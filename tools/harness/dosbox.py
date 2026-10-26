"""Instrumented DOSBox-X harness for Master of Magic v1.31 (work/harness/game, a copy; the GOG files are never touched).

DOSBox-X runs on a PRIVATE Windows desktop ("MoMHarness"): its window is never shown on your desktop and can never
take focus (you can go on using the machine). Everything goes through the window handle / process:
  - input:   PostMessage keyboard and mouse messages to the SDL window (no SendInput, no focus needed)
  - screen:  PrintWindow of the window into a PNG
  - memory:  ReadProcessMemory of the emulated RAM (DGROUP found by a string, linear 0 by the BIOS data area), so game
             state can be read live and compared with our formulas. READ ONLY: nothing is written into the game.
Window = an integer multiple of 320x200 (4x with mom.conf); DOS x = window x / scale.

  python -P tools/harness/dosbox.py start|shot NAME|stop|status      (-P: Pillow lives in the user site-packages)
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import struct
import subprocess
import sys
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT
EXE = _mp.DOSBOX                              # tools/harness/setup.py --dosbox
EXE_SDL1 = _mp.DOSBOX
# upstream CI nightly (main-ai 433ac897, 2026-10-17): has the save-state fix (PR #6682) and the debugger SAVESTATE /
# LOADSTATE / MEMREAD / STEPIN commands (PR #6683) - see docs/DOSBOX_X_DEBUG.txt
EXE_NIGHTLY = _mp.DOSBOX_DEBUG                # setup.py --dosbox-debug (a build with PRs #6682 / #6683)
CONF = ROOT + 'tools/harness/mom.conf'
STATE = ROOT + 'work/harness/state.json'
SHOTS = ROOT + 'work/harness/shots/'
DESKTOP = 'MoMHarness'

u32 = ctypes.WinDLL('user32', use_last_error=True)
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)

GENERIC_ALL = 0x10000000
WM_KEYDOWN, WM_KEYUP, WM_CHAR = 0x100, 0x101, 0x102
WM_SYSKEYDOWN, WM_SYSKEYUP = 0x104, 0x105
WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP = 0x200, 0x201, 0x202, 0x204, 0x205
MK_LBUTTON, MK_RBUTTON = 1, 2
PW_CLIENTONLY, PW_RENDERFULLCONTENT = 1, 2


class STARTUPINFOW(ctypes.Structure):
    _fields_ = [('cb', wt.DWORD), ('lpReserved', wt.LPWSTR), ('lpDesktop', wt.LPWSTR), ('lpTitle', wt.LPWSTR),
                ('dwX', wt.DWORD), ('dwY', wt.DWORD), ('dwXSize', wt.DWORD), ('dwYSize', wt.DWORD),
                ('dwXCountChars', wt.DWORD), ('dwYCountChars', wt.DWORD), ('dwFillAttribute', wt.DWORD),
                ('dwFlags', wt.DWORD), ('wShowWindow', wt.WORD), ('cbReserved2', wt.WORD),
                ('lpReserved2', ctypes.c_void_p), ('hStdInput', wt.HANDLE), ('hStdOutput', wt.HANDLE),
                ('hStdError', wt.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [('hProcess', wt.HANDLE), ('hThread', wt.HANDLE), ('dwProcessId', wt.DWORD), ('dwThreadId', wt.DWORD)]


class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', wt.DWORD),
                ('PartitionId', wt.WORD), ('RegionSize', ctypes.c_size_t), ('State', wt.DWORD),
                ('Protect', wt.DWORD), ('Type', wt.DWORD)]


u32.CreateDesktopW.restype = wt.HANDLE
u32.OpenDesktopW.restype = wt.HANDLE
k32.OpenProcess.restype = wt.HANDLE
WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)


def _desktop():
    h = u32.OpenDesktopW(DESKTOP, 0, False, GENERIC_ALL)
    if not h:
        h = u32.CreateDesktopW(DESKTOP, None, None, 0, GENERIC_ALL, None)
    if not h:
        raise OSError('desktop: %d' % ctypes.get_last_error())
    return h


def start(conf=CONF, exe=EXE):
    if '@ROOT' in open(conf, encoding='latin-1').read():     # a template (tools/harness/*.conf): the repository's
        conf = _mp.render(conf, _mp.ROOT + 'work/harness/conf/' + os.path.basename(conf))   # place filled in
    _desktop()
    si = STARTUPINFOW()
    si.cb = ctypes.sizeof(si)
    si.lpDesktop = DESKTOP
    pi = PROCESS_INFORMATION()
    cmd = '"%s" -conf "%s" -nopromptfolder -fastlaunch' % (exe.replace('/', '\\'), conf.replace('/', '\\'))
    ok = k32.CreateProcessW(None, ctypes.create_unicode_buffer(cmd), None, None, False, 0, None,
                            os.path.dirname(exe).replace('/', '\\'), ctypes.byref(si), ctypes.byref(pi))
    if not ok:
        raise OSError('CreateProcess: %d' % ctypes.get_last_error())
    st = {'pid': pi.dwProcessId, 'started': time.time()}
    os.makedirs(os.path.dirname(STATE), exist_ok=True)
    json.dump(st, open(STATE, 'w'))
    for _ in range(100):
        time.sleep(0.2)
        if window(st['pid']):
            break
    return st


def state():
    return json.load(open(STATE))


def window(pid=None):
    pid = pid or state()['pid']
    found = []
    hd = _desktop()

    def cb(h, _):
        p = wt.DWORD()
        u32.GetWindowThreadProcessId(h, ctypes.byref(p))
        if p.value == pid:
            r = wt.RECT()
            u32.GetClientRect(h, ctypes.byref(r))
            cls = ctypes.create_unicode_buffer(64)
            u32.GetClassNameW(h, cls, 64)
            if cls.value != 'ConsoleWindowClass':       # the debugger console (dbgchan BREAK) is not the game
                found.append((r.right * r.bottom, h))
        return True
    u32.EnumDesktopWindows(hd, WNDENUMPROC(cb), 0)
    found.sort(reverse=True)
    return found[0][1] if found and found[0][0] > 0 else None


def client_size(h=None):
    h = h or window()
    r = wt.RECT()
    u32.GetClientRect(h, ctypes.byref(r))
    return r.right, r.bottom


def grab(h=None):
    """PrintWindow of the client area -> PIL image in DOS pixels (320x200)."""
    from PIL import Image
    h = h or window()
    w, ht = client_size(h)
    hdc = u32.GetDC(h)
    mdc = g32.CreateCompatibleDC(hdc)
    bmp = g32.CreateCompatibleBitmap(hdc, w, ht)
    g32.SelectObject(mdc, bmp)
    u32.PrintWindow(h, mdc, PW_CLIENTONLY | PW_RENDERFULLCONTENT)
    buf = ctypes.create_string_buffer(w * ht * 4)
    bi = struct.pack('<IiiHHIIiiII', 40, w, -ht, 1, 32, 0, 0, 0, 0, 0, 0)
    g32.GetDIBits(mdc, bmp, 0, ht, buf, bi, 0)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    u32.ReleaseDC(h, hdc)
    img = Image.frombuffer('RGBA', (w, ht), buf, 'raw', 'BGRA', 0, 1).convert('RGB')
    k = scale(h)
    return img.resize((w // k, ht // k), Image.NEAREST)


def shot(name, h=None, big=False):
    """Screenshot -> work/harness/shots/NAME.png (DOS 320x200, or 2x when big for easier reading)."""
    from PIL import Image
    img = grab(h)
    if big:
        img = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
    os.makedirs(SHOTS, exist_ok=True)
    p = SHOTS + name + '.png'
    img.save(p)
    return p


# ---- input (posted messages; DOS coordinates 320x200) -------------------------------------------------------
_SCALE = {}


def scale(h):
    """Window pixels per DOS pixel (the window is an integer multiple of 320x200)."""
    if h not in _SCALE:
        _SCALE[h] = max(1, client_size(h)[0] // 320)
    return _SCALE[h]


def _lp(x, y, h):
    k = scale(h)
    return (int(y * k + k // 2) << 16) | (int(x * k + k // 2) & 0xffff)


def move(x, y, h=None):
    h = h or window()
    u32.PostMessageW(h, WM_MOUSEMOVE, 0, _lp(x, y, h))


def click(x, y, right=False, h=None, hold=0.08):
    h = h or window()
    down, up, mk = (WM_RBUTTONDOWN, WM_RBUTTONUP, MK_RBUTTON) if right else (WM_LBUTTONDOWN, WM_LBUTTONUP, MK_LBUTTON)
    move(x, y, h)
    time.sleep(0.05)
    u32.PostMessageW(h, down, mk, _lp(x, y, h))
    time.sleep(hold)
    u32.PostMessageW(h, up, 0, _lp(x, y, h))


VK = {'enter': 0x0d, 'esc': 0x1b, 'space': 0x20, 'left': 0x25, 'up': 0x26, 'right': 0x27, 'down': 0x28,
      'pgup': 0x21, 'pgdn': 0x22, 'end': 0x23, 'home': 0x24,
      'tab': 0x09, 'backspace': 0x08, 'alt': 0x12, **{'f%d' % i: 0x6f + i for i in range(1, 13)}}


# keypad keys with NumLock off (not extended: BIOS scan code with ASCII 0, which the game maps apart from the grey keys)
KP = {'kp7': (0x24, 0x47), 'kp8': (0x26, 0x48), 'kp9': (0x21, 0x49), 'kp4': (0x25, 0x4b), 'kp6': (0x27, 0x4d),
      'kp1': (0x23, 0x4f), 'kp2': (0x28, 0x50), 'kp3': (0x22, 0x51)}


def key(name, h=None, alt=False):
    """Press one key: a letter / digit, a VK name above or a keypad key kp1..kp9. alt=True sends it as Alt+key."""
    h = h or window()
    if name in KP:
        vk, sc = KP[name]
        ext = 0
    else:
        vk = VK[name] if name in VK else ord(name.upper())
        sc = u32.MapVirtualKeyW(vk, 0)
        ext = 1 << 24 if 0x21 <= vk <= 0x28 else 0
    ctx = (1 << 29) if alt else 0
    if alt:
        u32.PostMessageW(h, WM_SYSKEYDOWN, 0x12, 1 | (u32.MapVirtualKeyW(0x12, 0) << 16) | ctx)
    dn, upm = (WM_SYSKEYDOWN, WM_SYSKEYUP) if alt else (WM_KEYDOWN, WM_KEYUP)
    u32.PostMessageW(h, dn, vk, 1 | (sc << 16) | ext | ctx)
    time.sleep(0.06)
    u32.PostMessageW(h, upm, vk, 1 | (sc << 16) | ext | (3 << 30) | ctx)
    if alt:
        u32.PostMessageW(h, WM_KEYUP, 0x12, 1 | (u32.MapVirtualKeyW(0x12, 0) << 16) | (3 << 30))


# ---- memory (read only) -------------------------------------------------------------------------------------
PROCESS_VM_READ, PROCESS_QUERY_INFORMATION = 0x10, 0x400
SIG = b'Run INSTALL to configure MASTER OF MAGIC.'      # DGROUP 0x288c (WIZARDS.EXE)
SIG_OFF = 0x288c
BDA_COM1 = bytes([0xf8, 0x03])                         # linear 0x400: COM1 port 0x3f8
BDA_MEMKB = bytes([0x80, 0x02])                        # linear 0x413: 640 KB


class Mem:
    """Emulated memory of the running game. ds = host address of DGROUP:0, base = host address of linear 0."""

    def __init__(self, pid=None):
        self.pid = pid or state()['pid']
        self.h = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, self.pid)
        if not self.h:
            raise OSError('OpenProcess: %d' % ctypes.get_last_error())
        self.ds = self.base = self.ds_seg = None
        self.locate()

    def read(self, addr, n):
        b = ctypes.create_string_buffer(n)
        got = ctypes.c_size_t()
        if not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), b, n, ctypes.byref(got)):
            raise OSError('ReadProcessMemory %x: %d' % (addr, ctypes.get_last_error()))
        return b.raw[:got.value]

    def _regions(self, lo=0, hi=1 << 47):
        mbi = MEMORY_BASIC_INFORMATION()
        a = lo
        while a < hi and k32.VirtualQueryEx(self.h, ctypes.c_void_p(a), ctypes.byref(mbi), ctypes.sizeof(mbi)):
            if mbi.State == 0x1000 and mbi.Protect in (0x04, 0x40):
                yield mbi.BaseAddress, mbi.RegionSize
            a = (mbi.BaseAddress or 0) + mbi.RegionSize

    def locate(self):
        """DGROUP = the SIG string in a large read-write region; then linear 0 below it."""
        for lo, size in self._regions():
            if size < 0x100000:
                continue
            for a in range(lo, lo + size, 0x100000):
                try:
                    blk = self.read(a, min(0x100000 + 64, lo + size - a))
                except OSError:
                    continue
                i = blk.find(SIG)
                if i >= 0:
                    self.ds = a + i - SIG_OFF
                    self._find_base()
                    return
        raise RuntimeError('game memory not found (is WIZARDS.EXE running?)')

    def _find_base(self):
        """Linear 0 = the 16-byte aligned address (relative to DS) whose BIOS data area holds COM1 0x3f8 at +0x400 and
        640 KB at +0x413; it lies less than 640 KB below DGROUP. (DOSBox-X's RAM block starts 0x40 before linear 0 in
        the plain build; with Game Link the RAM follows the shared core block.)"""
        lo = self.ds - 0xa0000
        for a in range(lo, self.ds, 0x10000):
            try:
                blk = self.read(a, 0x10000 + 0x420)
            except OSError:
                continue
            k = (self.ds - a) % 16
            for c in range(k, 0x10000, 16):
                if blk[c + 0x400:c + 0x402] == BDA_COM1 and blk[c + 0x413:c + 0x415] == BDA_MEMKB:
                    self.base = a + c
                    self.ds_seg = (self.ds - self.base) // 16
                    return
        raise RuntimeError('linear 0 not found')

    def w(self, ds_off, signed=True):
        return struct.unpack('<h' if signed else '<H', self.read(self.ds + ds_off, 2))[0]

    def far(self, ds_off):
        """Host address of the far pointer stored at DS:ds_off."""
        off, seg = struct.unpack('<HH', self.read(self.ds + ds_off, 4))
        return self.base + seg * 16 + off



# ---- Game Link (DOSBox-X output=gamelink): input, frame and RAM through shared memory -----------------------
# DOSBox-X's own interface for external tools (Grid Cartographer). With mom.conf output=gamelink DOSBox-X takes
# keyboard / mouse ONLY from this block and copies every frame into it, so nothing depends on the window.
# Layout = DOSBox-X src/gamelink/gamelink.h sSharedMemoryMap_R4 (packed); RAM (linear 0) follows the core block.
GL_MMAP, GL_MUTEX = 'DWD_GAMELINK_MMAP_R4', 'DWD_GAMELINK_MUTEX_R4'
GL_FRAME = 342                       # seq u16, width u16, height u16, fmt u8, res u8, par_x u16, par_y u16, pixels
GL_PIX = GL_FRAME + 12
GL_INPUT = GL_PIX + 1280 * 1024 * 4  # mouse_dx f32, mouse_dy f32, ready u8, mouse_btn u8, keyb_state u32[8]
GL_CORE = GL_INPUT + 42 + (4 + 16384 * 4 + 16384) + 2 * (2 + 65536) + 2 + 4
# key -> bit = Windows set-1 scancode (extended keys + 0x80): src/gamelink/scancodes_windows.h
SC = {'esc': 1, 'backspace': 14, 'tab': 15, 'enter': 28, 'ctrl': 29, 'lshift': 42, 'alt': 56, 'space': 57,
      'up': 200, 'pgup': 201, 'left': 203, 'right': 205, 'down': 208, 'pgdn': 209, 'home': 199, 'end': 207,
      **{'f%d' % i: 0x3a + i for i in range(1, 11)}, 'f11': 0x57, 'f12': 0x58,
      **{c: i for c, i in zip('1234567890', range(2, 12))},
      **{c: i for c, i in zip('qwertyuiop', range(16, 26))},
      **{c: i for c, i in zip('asdfghjkl', range(30, 39))},
      **{c: i for c, i in zip('zxcvbnm', range(44, 51))}}
MOUSE_X, MOUSE_Y = 0x78c2, 0x78c4    # the game's own copy of the mouse position (DS), 0-319 / 0-199


class GameLink:
    """Shared-memory control of the running game. All waits are on the frame counter / ready flag."""

    # per-program DGROUP layout: (offset of SIG, mouse x / y, click x / y / buttons, click / event pending)
    PROGS = {'wizards': (0x288c, 0x78c2, 0x78c4, 0x78cc, 0x78ce, 0x78d0, 0x78ca, 0x78d2),
             'magic': (0x280a, 0x4394, 0x4396, 0x439e, 0x43a0, 0x43a2, 0x439c, 0x43a4)}   # MAGIC.EXE: same library

    CHECK = {'wizards': (0x2881, b'CONFIG.MOM\0Run INSTALL'), 'magic': (0x2880, b'Thank you for playing')}

    def __init__(self, prog='wizards'):
        import mmap
        self.prog = prog
        self.L = self.PROGS[prog]
        self.ram_size = 16 * 1024 * 1024
        self.m = mmap.mmap(-1, GL_CORE + self.ram_size, tagname=GL_MMAP)
        if self.m[0] == 0:
            raise RuntimeError('Game Link block empty: DOSBox-X not running with output=gamelink?')
        self.mutex = k32.OpenMutexW(0x00100000, False, GL_MUTEX)     # SYNCHRONIZE
        self.keys = [0] * 8
        self.btn = 0
        self.ds = None

    # -- locking around writes into the input block (DOSBox-X holds the same mutex in In / Out)
    def _write_input(self, dx=0.0, dy=0.0):
        if self.mutex:
            k32.WaitForSingleObject(self.mutex, 2000)
        try:
            self.m[GL_INPUT:GL_INPUT + 42] = struct.pack('<ffBB8I', dx, dy, 1, self.btn, *self.keys)
        finally:
            if self.mutex:
                k32.ReleaseMutex(self.mutex)
        for _ in range(200):                     # consumed when DOSBox-X clears ready
            if self.m[GL_INPUT + 8] == 0:
                return
            time.sleep(0.005)

    def seq(self):
        return struct.unpack('<H', self.m[GL_FRAME:GL_FRAME + 2])[0]

    def wait_frames(self, n=2, timeout=3.0):
        s0, t0 = self.seq(), time.time()
        while (self.seq() - s0) & 0xffff < n and time.time() - t0 < timeout:
            time.sleep(0.01)

    def key(self, name, hold_frames=2, alt=False):
        b = SC[name.lower()]
        if alt:
            self.keys[SC['alt'] >> 5] |= 1 << (SC['alt'] & 31)
            self._write_input()
            self.wait_frames(1)
        self.keys[b >> 5] |= 1 << (b & 31)
        self._write_input()
        self.wait_frames(hold_frames)
        self.keys[b >> 5] &= ~(1 << (b & 31))
        if alt:
            self.keys[SC['alt'] >> 5] &= ~(1 << (SC['alt'] & 31))
        self._write_input()
        self.wait_frames(2)

    def type(self, text):
        for ch in text:
            self.key('space' if ch == ' ' else ch)

    # -- RAM (read only)
    def ram(self, linear, n):
        return self.m[GL_CORE + linear:GL_CORE + linear + n]

    def find_ds(self):
        # both programs carry SIG, and a stale copy of the other one may still sit in freed memory: check a second
        # program-specific string at its known offset
        chk, txt = self.CHECK[self.prog]
        i = GL_CORE
        while True:
            i = self.m.find(SIG, i, GL_CORE + 0xa0000)
            if i < 0:
                raise RuntimeError('%s DGROUP not found in RAM' % self.prog)
            ds = i - GL_CORE - self.L[0]
            if self.m[GL_CORE + ds + chk:GL_CORE + ds + chk + len(txt)] == txt:
                break
            i += 1
        self.ds = ds                  # linear address of DGROUP:0
        return self.ds

    def dsw(self, off, signed=True):
        if self.ds is None:
            self.find_ds()
        return struct.unpack('<h' if signed else '<H', self.ram(self.ds + off, 2))[0]

    def far(self, off):
        """Linear address of the far pointer stored at DS:off."""
        if self.ds is None:
            self.find_ds()
        o, sg = struct.unpack('<HH', self.ram(self.ds + off, 4))
        return sg * 16 + o

    # -- mouse: relative moves (Mouse_CursorMoved emulate=1) closed-loop on the game's mouse_x / mouse_y
    def mouse(self):
        return self.dsw(self.L[1]), self.dsw(self.L[2])

    def move(self, x, y, tries=12):
        for _ in range(tries):
            cx, cy = self.mouse()
            if (cx, cy) == (x, y):
                return True
            self._write_input(float(x - cx), float(y - cy))
            self.wait_frames(2)
        return self.mouse() == (x, y)

    def click_relative(self, x, y, right=False):
        """Game Link mouse (only with output=gamelink; currently stuck at (0,0), see PROCESS_LOG step 45)."""
        ok = self.move(x, y)
        bit = 2 if right else 1
        self.btn |= bit
        self._write_input()
        self.wait_frames(2)
        self.btn &= ~bit
        self._write_input()
        self.wait_frames(3)
        return ok

    # -- clicks through the game's own input latch (works with any output). The UI reads clicks from these
    # DGROUP fields, exactly as its Mouse_Post_Click 2579:0712 / the INT 33h handler leave them: mouse_x / mouse_y
    # 0x78c2 / 0x78c4 (hover), event x / y / buttons 0x78cc / 0x78ce / 0x78d0, click / event pending 0x78ca / 0x78d2.
    # These are input-device variables only; no game state, file or code byte is touched.
    def poke_w(self, off, val):
        if self.ds is None:
            self.find_ds()
        a = GL_CORE + self.ds + off
        self.m[a:a + 2] = struct.pack('<H', val & 0xffff)

    def hover(self, x, y):
        self.poke_w(self.L[1], x)
        self.poke_w(self.L[2], y)

    def click(self, x, y, right=False, settle=0.6):
        self.hover(x, y)
        self.poke_w(self.L[3], x)
        self.poke_w(self.L[4], y)
        self.poke_w(self.L[5], 2 if right else 1)
        self.poke_w(self.L[6], 1)
        self.poke_w(self.L[7], 1)
        time.sleep(settle)
        return True

    # -- frame
    def frame(self):
        """Current frame as a PIL image at the emulator's render size (320x200 for MoM, doubled lines possible)."""
        from PIL import Image
        w, h = struct.unpack('<HH', self.m[GL_FRAME + 2:GL_FRAME + 6])
        px = self.m[GL_PIX:GL_PIX + w * h * 4]
        return Image.frombuffer('RGBA', (w, h), px, 'raw', 'BGRA', 0, 1).convert('RGB')

    def shot(self, name, big=True):
        from PIL import Image
        img = self.frame()
        if img.size != (320, 200):
            img = img.resize((320, 200), Image.NEAREST)
        if big:
            img = img.resize((640, 400), Image.NEAREST)
        os.makedirs(SHOTS, exist_ok=True)
        p = SHOTS + name + '.png'
        img.save(p)
        return p


PROTECTED = set(int(p) for p in os.environ.get('MOM_PROTECTED_PIDS', '').split(',') if p)   # DOSBox-X processes never to touch


def stop():
    """Kill the harness's own DOSBox-X - only if the saved PID still IS a dosbox-x process and not a protected one
    (a stale PID can be reused by any other program after the harness instance exited)."""
    pid = state()['pid']
    if pid in PROTECTED:
        return
    r = subprocess.run(['tasklist', '/FI', 'PID eq %d' % pid, '/FO', 'CSV', '/NH'], capture_output=True, text=True)
    if 'dosbox' not in r.stdout.lower():
        return
    subprocess.run(['taskkill', '/PID', str(pid), '/F'], capture_output=True)


if __name__ == '__main__':
    c = sys.argv[1]
    if c == 'start':
        print(start(), window())
    elif c == 'shot':
        print(shot(sys.argv[2], big=len(sys.argv) > 3))
    elif c == 'stop':
        stop()
    elif c == 'status':
        h = window()
        print('window', h, client_size(h) if h else None)
