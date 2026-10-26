"""Per-screen UI input map: dump the live control table of the running game (read only).

The UI library keeps every clickable / keyable control of the screen that is waiting for input in one far array
(p_ui_controls, struct UIControl 0x26 bytes, src/include/STRUCTS.H; filled by the UI_Add_* routines of UI.C).
Entry 0 is the null control (UI_Clear_Controls sets the count to 1). UI_Control_At_Mouse returns the LOWEST index
whose box holds (mouse + pointer offset), bounds INCLUSIVE on all four sides. Right-click help is a separate list
(p_active_help: id, x1, y1, x2, y2 per entry; ids index HELP.LBX entry 2); the control's own 'help' word is stored
but never read by either program.

    python -P tools/harness/ui_dump.py snap NAME [wizards|magic] [note...]
        -> work/uimap/<prog>/NAME.json + NAME.png (the frame at 2x with every on-screen box drawn and numbered)
"""
import json
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dosbox as D  # noqa: E402

OUT = D.ROOT + 'work/uimap/'
TYPES = ['button', 'toggle', 'latch', 'pic_value', 'text_input', 'picture', 'slider', 'region', 'alt_hotkey',
         'value_region', 'text_button', 'text_line', 'grid']
# DGROUP offsets: p_ui_controls (far), ui_control_count, p_active_help (near), ui_help_count, ui_help_enabled,
# mouse_region_count, p_mouse_regions (near), current_screen (WIZARDS only)
OFS = {'wizards': dict(ctl=0xe8ac, n=0xe8a2, help=0xe872, nhelp=0x8298, help_on=0x8296, nreg=0x78c6, reg=0x78de,
                       screen=0x9990),
       'magic': dict(ctl=0xa8a2, n=0xa898, help=0xa868, nhelp=0x4d6a, help_on=0x4d68, nreg=0x4398, reg=0x43b0,
                     screen=None)}
# game key codes (Key_Read_Translated, docs/ui/HARNESS_UI_NOTES.txt): arrows either kind 1-4, keypad 7/9/1/3 apart
KEYNAME = {1: 'Left', 2: 'Right', 3: 'Up', 4: 'Down', 5: 'PgUp', 6: 'PgDn', 7: 'Home', 8: 'End', 9: 'Ins',
           0x0a: 'Del', 0x0b: 'Backspace', 0x0c: 'Enter', 0x0d: 'Tab', 0x1b: 'Esc', 0x19: 'Keypad9',
           0x1a: 'Keypad7', 0x1c: 'Keypad1', 0x1d: 'Keypad3', 0x1e: 'Keypad5', 0x20: 'Space', 0x91: 'F11',
           0x92: 'F12', **{0x0e + k: 'F%d' % (k + 1) for k in range(10)},
           **{0x80 + k: n for k, n in enumerate(('Ctrl+Right', 'Ctrl+Left', 'Ctrl+Up', 'Ctrl+Down', 'Ctrl+Home',
                                                 'Ctrl+PgUp', 'Ctrl+End', 'Ctrl+PgDn'))}}
_HELP = None


def help_title(i):
    global _HELP
    if _HELP is None:
        d = open(D.ROOT + 'work/harness/game/HELP.LBX', 'rb').read()       # = gog/official_131
        a, b = struct.unpack('<II', d[16:24])                     # entry 2
        e = d[a:b]
        nr, rs = struct.unpack('<HH', e[:4])
        _HELP = [e[4 + k * rs:4 + k * rs + 30].split(b'\0')[0].decode('latin1') for k in range(nr)]
    return _HELP[i] if 0 <= i < len(_HELP) else None


def label_from_help(ctl, hl):
    """Name each on-screen control after the right-click help area covering at least half of its box (the game's
    own title for that spot; the best-covering area wins)."""
    for c in ctl:
        c.pop('help_title', None)
        if c.get('offscreen'):
            continue
        area = c['w'] * c['h']
        best = 0
        for h in hl:
            ox = min(c['x2'], h['x2']) - max(c['x1'], h['x1']) + 1
            oy = min(c['y2'], h['y2']) - max(c['y1'], h['y1']) + 1
            if ox > 0 and oy > 0 and ox * oy * 2 >= area and ox * oy > best:
                best = ox * oy
                c['help_title'] = h['title']


class Dump:
    def __init__(self, prog='wizards'):
        self.prog = prog
        self.o = OFS[prog]
        self.g = D.GameLink(prog)
        self.g.find_ds()

    def s(self, off, n=40):
        """NUL-terminated string at DS:off (None for 0 / out of range)."""
        off &= 0xffff
        if off == 0 or off > 0xfff0:
            return None
        return self.g.ram(self.g.ds + off, n).split(b'\0')[0].decode('latin1')

    def raw(self):
        g, o = self.g, self.o
        n = g.dsw(o['n'])
        if not 0 < n < 200:
            return None
        base = g.far(o['ctl'])
        return n, bytes(g.ram(base, n * 0x26))

    def stable(self, tries=10):
        """Two equal reads 0.25 s apart (a screen may be rebuilding its table)."""
        last = None
        for _ in range(tries):
            r = self.raw()
            if r is not None and r == last:
                return r
            last = r
            time.sleep(0.25)
        raise RuntimeError('control table not stable')

    def controls(self, n, blob):
        out = []
        for i in range(n):
            f = struct.unpack('<18hBB', blob[i * 0x26:(i + 1) * 0x26])
            x1, y1, x2, y2, t, hlp, font, col, fill, en, col2, p0, p1, p2, p3, p4, p5, p6 = f[:18]
            hk = f[18]
            c = dict(id=i, type=TYPES[t] if 0 <= t < len(TYPES) else t, x1=x1, y1=y1, x2=x2, y2=y2,
                     w=x2 - x1 + 1, h=y2 - y1 + 1, hotkey=hk, help=hlp)
            if hk:
                c['key'] = KEYNAME.get(hk) or (chr(hk) if 0x20 < hk < 0x7f else '0x%02x' % hk)
            if x1 >= 320 or y1 >= 200:
                c['offscreen'] = True
            if t in (0, 1, 2, 3) and p0:
                c['label'] = self.s(p0)
            if t == 10:
                c['text'] = self.s(p0)
                c['enabled'] = en
                c['value'] = p1
            if t in (4, 11):
                c['text'] = self.s(p0)
                c['max_len'] = p5
            if t == 8:
                c['code'] = self.s(p0)
                c['key'] = 'Alt+' + '+'.join(c['code'] or '?')      # the letters typed in turn with Alt held
            if t == 9:
                c['value'] = p0
                c['sets'] = p1
            if t == 12:
                c['cell_w'], c['cell_h'] = p1, p2
                if p1 > 0 and p2 > 0:
                    c['cols'], c['rows'] = (x2 - x1 + 1) // p1, (y2 - y1 + 1) // p2
            if t == 6:
                c['min'], c['max'] = p0, p1
                c['vertical'] = p5
            if t == 5:
                c['pic'] = 'seg %04x' % (p5 & 0xffff)
            out.append(c)
        return out

    def helps(self):
        g, o = self.g, self.o
        n, p = g.dsw(o['nhelp']), g.dsw(o['help'], signed=False)
        out = []
        if g.dsw(o['help_on']) and 0 < n < 300 and p:
            blob = g.ram(g.ds + p, n * 10)
            for k in range(n):
                hid, x1, y1, x2, y2 = struct.unpack('<5h', blob[k * 10:k * 10 + 10])
                if hid != -1:
                    out.append(dict(help=hid, title=help_title(hid), x1=x1, y1=y1, x2=x2, y2=y2))
        return out

    def regions(self):
        g, o = self.g, self.o
        n, p = g.dsw(o['nreg']), g.dsw(o['reg'], signed=False)
        out = []
        if 0 < n < 200 and p:
            blob = g.ram(g.ds + p, n * 12)
            for k in range(n):
                img, off, x1, y1, x2, y2 = struct.unpack('<6h', blob[k * 12:k * 12 + 12])
                out.append(dict(image=img, pointer_offset=off, x1=x1, y1=y1, x2=x2, y2=y2))
        return out

    def snap(self, name, note=''):
        n, blob = self.stable()
        ctl = self.controls(n, blob)
        hl = self.helps()
        label_from_help(ctl[1:], hl)
        rec = dict(prog=self.prog, name=name, note=note, time=time.strftime('%Y-%m-%d %H:%M:%S'),
                   current_screen=self.g.dsw(self.o['screen']) if self.o['screen'] else None,
                   control_count=n - 1, controls=ctl[1:], help=hl, pointer_regions=self.regions())
        d = OUT + self.prog + '/'
        os.makedirs(d, exist_ok=True)
        json.dump(rec, open(d + name + '.json', 'w'), indent=1)
        self.overlay(ctl[1:], d + name + '.png')
        return rec

    def overlay(self, ctl, path):
        from PIL import Image, ImageDraw
        img = D.grab()                                   # PrintWindow (the Game Link frame is empty with output=surface)
        img = img.resize((640, 400), Image.NEAREST)
        dr = ImageDraw.Draw(img)
        for c in ctl:
            if c.get('offscreen'):
                continue
            col = (255, 255, 0) if c['type'] != 'grid' else (0, 255, 255)
            dr.rectangle([c['x1'] * 2, c['y1'] * 2, c['x2'] * 2 + 1, c['y2'] * 2 + 1], outline=col)
            dr.text((c['x1'] * 2 + 2, c['y1'] * 2 + 1), str(c['id']), fill=(255, 0, 255))
        img.save(path)


def summary(rec):
    lines = ['%s/%s: screen %s, %d controls, %d help areas' % (rec['prog'], rec['name'], rec['current_screen'],
                                                              rec['control_count'], len(rec['help']))]
    for c in rec['controls']:
        extra = ' '.join('%s=%s' % (k, c[k]) for k in ('key', 'label', 'text', 'code', 'help_title', 'cols', 'rows',
                                                      'value', 'enabled') if c.get(k) not in (None, ''))
        lines.append('  %3d %-12s (%3d,%3d)-(%3d,%3d) %s' % (c['id'], c['type'], c['x1'], c['y1'], c['x2'], c['y2'],
                                                           extra))
    return '\n'.join(lines)


def run_steps(prog, steps):
    """Steps: k:KEY (dosbox.key name or letter; a:LETTER = Alt+letter), c:X,Y (left click), r:X,Y (right click),
    w:SECONDS."""
    g = None
    for st in steps:
        op, arg = st.split(':', 1)
        if op == 'k':
            D.key(arg)
        elif op == 'a':
            D.key(arg, alt=True)
        elif op in ('c', 'r'):
            g = g or D.GameLink(prog)
            x, y = (int(v) for v in arg.split(','))
            g.click(x, y, right=op == 'r')
        elif op == 'map':                    # Esc until the overland map (current_screen 5) is up
            g = g or D.GameLink(prog)
            for _ in range(6):
                if g.dsw(OFS['wizards']['screen']) == 5:
                    break
                D.key('esc')
                time.sleep(1.5)
            continue
        elif op == 'w':
            time.sleep(float(arg))
        else:
            raise ValueError(st)
        if op != 'w':
            time.sleep(0.8)


if __name__ == '__main__':
    if sys.argv[1] == 'snap':
        prog = sys.argv[3] if len(sys.argv) > 3 else 'wizards'
        print(summary(Dump(prog).snap(sys.argv[2], ' '.join(sys.argv[4:]))))
    elif sys.argv[1] == 'go':                 # go PROG NAME STEP ... : run the steps from the current screen, then snap
        prog, name, steps = sys.argv[2], sys.argv[3], sys.argv[4:]
        run_steps(prog, steps)
        time.sleep(1.5)
        print(summary(Dump(prog).snap(name, 'route: ' + ' '.join(steps))))
    elif sys.argv[1] == 'do':                 # do PROG STEP ... : steps only
        run_steps(sys.argv[2], sys.argv[3:])
