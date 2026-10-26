"""Regression-harness library (P4 B4): the game's data by NAME, in the save file and in the running game.

  SaveFile  the save in Save_Game's write order (SAVELOAD.C), every record laid out from STRUCTS.H (layout.py);
            get / set by table, index and field path ('globals.time_stop', 'hired_heroes[2].item_index[1]').
            Built from work/harness/SAVE9_orig.GAM; written ONLY as the harness copy work/harness/game/SAVE9.GAM.
  Live      the running WIZARDS.EXE through Game Link (dosbox.py): globals by name (src/include/WIZDECL.H gives the
            DS offset and type of every one) and records through their far pointers - read only, as before.
  boot()    write the save, (re)start DOSBox-X on the hidden desktop, wait for the overland map.
  end_turn()  Done on every stack, Next Turn, answer whatever waits, until the next turn's map.

  python -I tools/harness/mom.py check       (self-check: the save map's size equals SAVE9_orig.GAM's)"""
import os
import re
import struct
import sys
import time
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from layout import L                          # noqa: E402

ROOT = _mp.ROOT
ORIG = ROOT + 'work/harness/SAVE9_orig.GAM'
GAME = ROOT + 'work/harness/game/SAVE9.GAM'
N = L.num


def _fmt(t, es):
    if '*' in t:
        return {2: 'H', 4: 'I'}[es]
    u = 'unsigned' in t
    return {1: 'B' if u else 'b', 2: 'H' if u else 'h', 4: 'I' if u else 'i'}[es]


def resolve(struct_name, path):
    """(offset in the record, element size, count, type) of a field path."""
    off = 0
    sname = struct_name
    toks = path.split('.')
    for k, tok in enumerate(toks):
        m = re.match(r'(\w+)((?:\[\d+\])*)$', tok)
        name, idx = m.group(1), [int(x) for x in re.findall(r'\[(\d+)\]', m.group(2))]
        f = L.field(sname, name)
        fo, es, cnt, t = f[1], f[2], f[3], f[4]
        dims = [N(d) for d in re.findall(r'\[([^\]]+)\]', _decl_dims(sname, name))]
        stride = es
        for j, i in enumerate(idx):
            inner = 1
            for d in dims[j + 1:]:
                inner *= d
            fo += i * inner * es
            cnt = inner
        off += fo
        mt = re.match(r'struct (\w+)', t)
        if k < len(toks) - 1:
            sname = mt.group(1)
        else:
            return off, stride, cnt, t
    raise KeyError(path)


_DIMS = {}


def _decl_dims(sname, field):
    key = (sname, field)
    if key not in _DIMS:
        body = re.sub(r'/\*.*?\*/', ' ', L.structs[sname], flags=re.S)
        m = re.search(r'\b%s\s*((?:\[[^\]]+\])*)\s*[;,]' % re.escape(field), body)
        _DIMS[key] = m.group(1) if m else ''
    return _DIMS[key]


def _order():
    """(table, struct name or None, element size, count) in Save_Game's write order (SAVELOAD.C)."""
    sq = N('MAP_SQUARES')
    return [('hero_stats', 'HeroStats', L.size('HeroStats'), 6 * N('NUM_HERO_TYPES')),
            ('num_players', None, 2, 1), ('land_size', None, 2, 1), ('magic_setting', None, 2, 1),
            ('difficulty', None, 2, 1), ('num_cities', None, 2, 1), ('num_units', None, 2, 1),
            ('game_turn', None, 2, 1), ('top_unit_in_stack', None, 2, 1),
            ('wizards', 'Wizard', L.size('Wizard'), N('MAX_PLAYERS')),
            ('terrain', None, 2, sq * 2), ('region_land', None, 1, N('REGION_ROWS') * N('REGION_COLS') * 2),
            ('region_water', None, 1, N('REGION_ROWS') * N('REGION_COLS') * 2), ('landmass', None, 1, sq * 2),
            ('nodes', 'Node', L.size('Node'), N('NUM_NODES')),
            ('fortresses', 'Fortress', L.size('Fortress'), N('MAX_PLAYERS')),
            ('towers', 'Tower', L.size('Tower'), N('NUM_TOWERS')), ('lairs', 'Lair', L.size('Lair'), N('NUM_LAIRS')),
            ('items', 'Item', L.size('Item'), N('NUM_ITEMS')), ('cities', 'City', L.size('City'), N('MAX_CITIES')),
            ('units', 'Unit', L.size('Unit'), N('MAX_UNITS') + 9), ('minerals', None, 1, sq * 2),
            ('exploration', None, 1, sq * 2), ('move_costs', None, 1, sq * 6 * 2),
            ('events', 'Events', L.size('Events'), 1), ('terrain_flags', None, 1, sq * 2),
            ('grand_vizier', None, 2, 1), ('items_in_game', None, 1, N('NUM_PREFAB_ITEMS')),
            ('hero_names', 'HeroName', L.size('HeroName'), N('NUM_HERO_TYPES'))]


TABLES = {}
_o = 0
for _t, _s, _es, _c in _order():
    TABLES[_t] = (_o, _s, _es, _c)
    _o += _es * _c
SAVE_SIZE = _o


class SaveFile:
    def __init__(self, path=ORIG):
        self.b = bytearray(open(path, 'rb').read())

    def addr(self, table, i=0, path=None):
        o, s, es, c = TABLES[table]
        assert 0 <= i < c, (table, i)
        if path is None:
            return o + i * es, es, 1, 'unsigned' if es == 1 else 'int'
        fo, fes, fc, t = resolve(s, path)
        return o + i * es + fo, fes, fc, t

    def get(self, table, i=0, path=None):
        a, es, c, t = self.addr(table, i, path)
        f = _fmt(t, es)
        v = [struct.unpack_from('<' + f, self.b, a + k * es)[0] for k in range(c)]
        return v[0] if c == 1 else v

    def set(self, table, i=0, path=None, value=0):
        a, es, c, t = self.addr(table, i, path)
        f = _fmt(t, es)
        vals = value if isinstance(value, (list, tuple)) else [value]
        for k, v in enumerate(vals):
            struct.pack_into('<' + f.upper() if v >= 0 else '<' + f.lower(), self.b, a + k * es, v)

    def tile(self, x, y, plane):
        return self.get('terrain', plane * N('MAP_SQUARES') + y * N('MAP_WIDTH') + x)

    def write(self, path=GAME):
        open(path, 'wb').write(bytes(self.b))


def restore():
    open(GAME, 'wb').write(open(ORIG, 'rb').read())


CASE = {'num': 0, 'shots': []}                 # set by regress.run: screenshots of the running case


def shot(tag, log=None):
    """Evidence screenshot of the hidden DOSBox-X window -> work/harness/regress/shots/<case>_<tag>.png (2x)."""
    import dosbox as D
    name = '%d_%s' % (CASE['num'], tag)
    dst = ROOT + 'work/harness/regress/shots/%s.png' % name
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    try:                                        # PrintWindow of the game window: take it while the game RUNS (a
        os.replace(D.shot(name, big=True), dst)  # debugger stop leaves no window image to grab)
    except Exception as e:
        if log is not None:
            log.append('  screenshot %s failed: %s' % (name, e))
        return None
    CASE['shots'].append(dst[len(ROOT):])
    if log is not None:
        log.append('  screenshot %s' % dst[len(ROOT):])
    return dst


# ---- the running game ---------------------------------------------------------------------------------------
def _syms(path):
    out = {}
    for l in open(path, encoding='latin-1'):
        m = re.match(r'extern (.+?)\b(\w+)((?:\[[^\]]+\])*);\s*/\* DS:([0-9a-f]{4})', l)
        if m:
            out[m.group(2)] = (int(m.group(4), 16), m.group(1).strip(), m.group(3))
    return out


SYM = _syms(ROOT + 'src/include/WIZDECL.H')
# record tables in RAM: far pointer (or DS array) -> struct
LIVE = {'cities': ('p_cities', 'City'), 'units': ('p_units', 'Unit'), 'items': ('p_items', 'Item'),
        'nodes': ('p_nodes', 'Node'), 'lairs': ('p_lairs', 'Lair'), 'towers': ('p_towers', 'Tower'),
        'fortresses': ('p_fortresses', 'Fortress'), 'battle_units': ('p_battle_units', 'BattleUnit'),
        'events': ('p_events', 'Events'), 'wizards': ('wizards', 'Wizard'), 'unit_types': ('unit_types', 'UnitType'),
        'spells': ('p_spell_data', 'Spell')}


class Live:
    def __init__(self):
        import dosbox as D
        self.D = D
        self.g = D.GameLink()
        self.g.find_ds()

    def glob(self, name, i=0):
        ds, t, dims = SYM[name]
        es = 4 if 'far' in t and '*' in t else (2 if '*' in t else L._type_size(t, False))
        b = self.g.ram(self.g.ds + ds + i * es, es)
        return struct.unpack('<' + _fmt(t, es), b)[0]

    def base(self, table):
        ptr, s = LIVE[table]
        ds, t, dims = SYM[ptr]
        if '*' in t:
            return self.g.far(ds)
        return self.g.ds + ds

    def get(self, table, i=0, path=None):
        ptr, s = LIVE[table]
        a = self.base(table) + i * L.size(s)
        fo, es, c, t = resolve(s, path)
        f = _fmt(t, es)
        raw = self.g.ram(a + fo, es * c)
        v = [struct.unpack_from('<' + f, raw, k * es)[0] for k in range(c)]
        return v[0] if c == 1 else v

    def find_table(self, table, save=None, sig_at=0, sig_len=256):
        """Linear address of a save table held anywhere in the emulated 16 MB (EMS included: the move-cost, mineral,
        exploration and terrain-flag maps live in EMS pages): the save's bytes of that table, sig_len bytes from
        sig_at, searched in RAM; must occur once. Read it with lv.g.ram(addr, n)."""
        import dosbox as D
        s = save or SaveFile()
        o, _, es, c = TABLES[table]
        sig = bytes(s.b[o + sig_at:o + sig_at + sig_len])
        m = self.g.m
        hits = []
        i = m.find(sig, D.GL_CORE)
        while i >= 0 and len(hits) < 3:
            hits.append(i - D.GL_CORE - sig_at)
            i = m.find(sig, i + 1)
        if len(hits) != 1:
            raise RuntimeError('%s: signature found %d times' % (table, len(hits)))
        return hits[0]

    def screen(self):
        return self.glob('current_screen')

    def turn(self):
        return self.glob('game_turn')


LOCK = ROOT + 'work/harness/RUN.LOCK'


def lock():
    """One harness run at a time (they share the hidden DOSBox-X, its state file and the debugger port): the lock
    holds the owner's PID; a lock of a process that has ended is taken over."""
    me = os.getpid()
    CASE['booted'] = True                       # regress.run restores the save / stops DOSBox only after this
    if os.path.exists(LOCK):
        try:
            pid = int(open(LOCK).read().strip())
        except ValueError:
            pid = 0
        if pid and pid != me:
            import subprocess
            r = subprocess.run(['tasklist', '/FI', 'PID eq %d' % pid, '/NH'], capture_output=True, text=True)
            if str(pid) in r.stdout:
                raise RuntimeError('another harness run (pid %d) holds %s' % (pid, LOCK))
    open(LOCK, 'w').write(str(me))


def kill_harness():
    """Stop every DOSBox-X started from the harness's executables (setup.py --dosbox / --dosbox-debug): a leftover
    one would share Game Link and the debugger port. Other DOSBox-X executables are not touched."""
    import subprocess
    ps = ('Get-Process dosbox-x -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq "%s" -or '
          '$_.Path -eq "%s" } | Stop-Process -Force' % (_mp.win(_mp.DOSBOX), _mp.win(_mp.DOSBOX_DEBUG)))
    subprocess.run(['powershell', '-NoProfile', '-Command', ps], capture_output=True)


def boot(save=None, wait=40):
    """Write the save (or the original), restart the harness DOSBox-X, wait until the overland map is up."""
    import dosbox as D
    lock()
    (save or SaveFile()).write()
    try:
        D.stop()
    except Exception:
        pass
    kill_harness()
    time.sleep(1)
    D.start()
    t0 = time.time()
    lv = None
    while time.time() - t0 < wait:
        time.sleep(1)
        try:
            lv = lv or Live()
            if lv.screen() == N('SCREEN_MAP'):
                time.sleep(1.5)
                return lv
        except Exception:
            lv = None
    raise RuntimeError('the overland map did not come up')


def end_turn(lv, timeout=300, log=None):
    """Done on every stack, Next Turn, answer what waits (Esc, Enter, Y, first list entry, OK, Yes) until the next
    turn's map. Returns the new turn number."""
    D, g = lv.D, lv.g
    turn0 = lv.turn()
    stalls = 0
    t0 = time.time()
    mp = N('SCREEN_MAP')
    while time.time() - t0 < timeout:
        if lv.turn() != turn0 and lv.screen() == mp:
            break
        scr, nou = lv.screen(), lv.glob('no_units_to_move')
        if lv.turn() == turn0 and scr == mp and nou == 0:
            D.key('d')
            time.sleep(0.4)
            continue
        if lv.turn() == turn0 and scr == mp and nou == 1:
            D.key('n')
            time.sleep(1.5)
        tw = time.time()
        while time.time() - tw < 5 and not (lv.turn() != turn0 and lv.screen() == mp):
            time.sleep(0.3)
        if lv.turn() != turn0 and lv.screen() == mp:
            break
        stalls += 1
        if items_award(lv):
            if log is not None:
                log.append('  items award: put in the vault')
            continue
        if log is not None:
            log.append('  waiting (screen %d), answer %d' % (lv.screen(), (stalls - 1) % 6))
        [lambda: D.key('esc'), lambda: D.key('enter'), lambda: D.key('y'), lambda: g.click(60, 33),
         lambda: g.click(255, 185), lambda: g.click(202, 132)][(stalls - 1) % 6]()
    time.sleep(1.5)
    return lv.turn()


def target_square(lv, x, y, log=None, timeout=90):
    """The human's overland spell target screen (SPELLTGT.C): wait until it is up (spelltarget_prompt allocated),
    bring square (x, y) into the 12 x 10 view with minimap clicks (Map_Window_Origin: the 58 x 30 minimap starts at
    the view centre - 29 / - 15), then click the square. Same plane as the view assumed."""
    D, g = lv.D, lv.g
    t0 = time.time()
    while lv.glob('spelltarget_prompt') == 0:
        if time.time() - t0 > timeout:
            raise RuntimeError('the spell target screen did not come up')
        time.sleep(0.5)
    time.sleep(1.5)
    W, cols, rows = N('MAP_WIDTH'), N('MAP_VIEW_COLS'), N('MAP_VIEW_ROWS')
    for _ in range(6):
        vx, vy = lv.glob('map_view_x'), lv.glob('map_view_y')
        rx, ry = (x - vx) % W, y - vy
        if rx < cols and 0 <= ry < rows:
            px, py = rx * N('MAP_TILE_W') + 10, ry * N('MAP_TILE_H') + N('MAP_VIEW_TOP') + 9
            if log is not None:
                log.append('  target (%d,%d): view (%d,%d) -> click (%d,%d)' % (x, y, vx, vy, px, py))
            g.click(px, py, settle=1.5)
            return True
        cx, cy = (vx + cols // 2) % W, vy + rows // 2
        left = cx - N('MINIMAP_W') // 2
        ox = left if left > 0 else (left + W) % W
        oy = cy - N('MINIMAP_H') // 2
        mx = min(max((x - ox) % W, 0), N('MINIMAP_W') - 1)
        my = min(max(y - oy, 0), N('MINIMAP_H') - 1)
        if log is not None:
            log.append('  target (%d,%d) not in view (%d,%d): minimap click (%d,%d)' % (x, y, vx, vy, mx, my))
        g.click(N('MINIMAP_X') + mx, N('MINIMAP_Y') + my, settle=1.5)
    raise RuntimeError('could not bring (%d,%d) into view' % (x, y))


# ---- combat --------------------------------------------------------------------------------------------------
KEYPAD = {(-1, -1): 'kp7', (0, -1): 'kp8', (1, -1): 'kp9', (-1, 0): 'kp4', (1, 0): 'kp6', (-1, 1): 'kp1',
          (0, 1): 'kp2', (1, 1): 'kp3'}


def setup_attack(s, attacker, defenders, def_xy):
    """Save edits for a battle the human starts: every human unit on patrol except `attacker` (ready, so the game
    selects it after loading), the defender units moved to def_xy (x, y, plane). Returns the keypad key that moves the
    attacker onto def_xy (it must be adjacent)."""
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'order', N('ORDER_PATROL'))
    s.set('units', attacker, 'order', N('ORDER_READY'))
    s.set('units', attacker, 'finished', 0)
    s.set('units', attacker, 'moves_left', s.get('units', attacker, 'moves_max'))
    for d in defenders:
        for f, v in zip(('x', 'y', 'plane'), def_xy):
            s.set('units', d, f, v)
    ax, ay = s.get('units', attacker, 'x'), s.get('units', attacker, 'y')
    return KEYPAD[(def_xy[0] - ax, def_xy[1] - ay)]


def controls(lv):
    """The live UI control table (UI.C p_ui_controls, ui_control_count): (x1, y1, x2, y2, hotkey) per control."""
    base = lv.g.far(SYM['p_ui_controls'][0])
    sz = L.size('UIControl')
    out = []
    for i in range(lv.glob('ui_control_count')):
        r = lv.g.ram(base + i * sz, sz)
        x1, y1, x2, y2 = struct.unpack_from('<4h', r, 0)
        out.append((x1, y1, x2, y2, chr(r[L.off('UIControl', 'hotkey')] or 32)))
    return out


def diplo_open(lv, wizard, log=None, timeout=30):
    """The human opens an audience with `wizard` (contacted, met): Magic screen (M), its gem (i * 77 + 29, 10..40 -
    gem i = wizard i + 1, MAGICSCR.C), then clicks through the greeting until the main menu's list is up (rows at
    x 38..283 from y 150, 9 apart: Propose Treaty / Threaten-Break Treaty / Offer Tribute / Exchange Spells / Good
    Bye; seen 10/23, shots 9500_*). Returns True when the menu is up."""
    lv.D.key('m')
    time.sleep(2.5)
    i = wizard - 1
    lv.g.click(i * 77 + 29 + 15, 25, settle=2.0)
    t0 = time.time()
    while time.time() - t0 < timeout:
        if lv.screen() == N('SCREEN_DIPLOMACY') and diplo_menu_up(lv):
            return True
        lv.g.click(160, 100, settle=1.5)
    if log is not None:
        log.append('  diplomacy menu not reached (screen %d, controls %s)' % (lv.screen(), controls(lv)))
    return False


def diplo_menu_up(lv):
    """A diplomacy list menu is waiting: rows of 9 pixels at x 38..283."""
    return sum(1 for c in controls(lv) if c[0] == 38 and c[2] == 283 and c[3] - c[1] == 8) >= 2


def diplo_pick(lv, row, settle=2.0):
    """Click entry `row` (0-based) of the diplomacy list menu on screen (rows at y 150 + 9 * row; by layout, as a
    disabled entry may not be a control)."""
    lv.g.click(120, 154 + 9 * row, settle=settle)


def diplo_rows(lv):
    """The rows of the diplomacy list menu on screen, top to bottom: UI_Text_List_Select gives every item a control
    (disabled ones too) at x 38..283, one line below its title, which is a control too (the topmost; clicks on it
    are ignored) - at y 142 in the human's menus, lower in an AI's offer (y = text height + 133, DIPLOMSG.C). The
    pitch is font height + line spacing (Propose Treaty sets spacing 2), so rows are found, not computed."""
    return sorted((c for c in controls(lv) if c[0] == 38 and c[2] == 283), key=lambda c: c[1])[1:]


def diplo_pick_row(lv, row, settle=2.0):
    """Click row `row` (0-based) of the list menu by its control (diplo_rows). Returns False if there is no such row."""
    rows = diplo_rows(lv)
    if row >= len(rows):
        return False
    c = rows[row]
    lv.g.click(120, (c[1] + c[3]) // 2, settle=settle)
    return True


def items_award(lv):
    """The Items screen offering an award (loot after a battle, a lair, an event): drop the held item into the first
    vault slot (ITEMSCR.C: regions of 19 x 15 at the window's bottom), then OK ("O", shown once nothing is held).
    Returns True when the screen was there."""
    slots = [c for c in controls(lv) if c[2] - c[0] == 19 and c[3] - c[1] == 15]
    if not slots:
        return False
    for c in slots:
        lv.g.click(c[0] + 9, c[1] + 7, settle=1.0)
        if any(k[4] == 'O' for k in controls(lv)):
            break
    lv.D.key('o')
    time.sleep(1.5)
    return True


def learn_spells(s, player, spells, rich=True):
    """Save edit: the wizard knows the spells (spell s at library offset s - 1, SPELLSTATE_KNOWN); rich = mana 5000
    and casting skill 200 so combat casting is never short."""
    for sp in spells:
        s.set('wizards', player, 'spell_library[%d][%d]' % ((sp - 1) // 40, (sp - 1) % 40), N('SPELLSTATE_KNOWN'))
    if rich:
        for f, v in (('mana_reserve', 5000), ('skill_points', 30000), ('nominal_casting_skill', 200),
                     ('combat_skill_left', 200), ('skill_left_this_turn', 200)):
            s.set('wizards', player, f, v)


def grid_to_screen(gx, gy):
    """BFGEN.C Combat_Grid_To_Screen: the top corner of square (gx, gy); its centre is 8 pixels lower."""
    return (gx - gy) * 16 + 158, (gx + gy) * 8 - 80


def book_rows(lv):
    """The combat spellbook is up: its 12 spell rows (COMBSPL.C Screen_Combat_Spellbook, 121 x 17 regions)."""
    return [c for c in controls(lv) if c[2] - c[0] in (120, 121) and c[3] - c[1] == 17]


def cast_spell(lv, spell, target=None, log=None):
    """The human's combat turn: Spell (S), the page holding `spell` (F / B), its row, then the target battle unit's
    square (target = a battle unit index) if the spell takes one. Returns once the book has closed."""
    D, g = lv.D, lv.g
    D.key('s')
    t0, sent = time.time(), 1
    while not book_rows(lv):
        if time.time() - t0 > 15:
            raise RuntimeError('the combat spellbook did not open')
        if time.time() - t0 > 5 * sent and sent < 3:    # a key posted while a debugger stop held the game is lost
            D.key('s')
            sent += 1
        time.sleep(0.5)
    time.sleep(1)
    pages = lv.g.ds + (lv.glob('p_spellbook_pages') & 0xffff)
    sz = L.size('SpellbookPage')
    where = None
    for p in range(lv.glob('spellbook_page_count')):
        cnt = lv.g.ram(pages + p * sz + L.off('SpellbookPage', 'count'), 1)[0]
        ids = struct.unpack_from('<6h', lv.g.ram(pages + p * sz + L.off('SpellbookPage', 'spells'), 12))
        for i in range(cnt):
            if abs(ids[i]) == spell:
                where = (p, i)
    if where is None:
        raise RuntimeError('spell %d is not in the combat book' % spell)
    p, i = where
    for _ in range(20):
        cur = lv.glob('spellbook_page')
        if cur <= p <= cur + 1:
            break
        D.key('f' if p > cur + 1 else 'b')
        time.sleep(2)
    side = p - lv.glob('spellbook_page')
    x = (16 + 16 + 60) if side == 0 else (16 + 148 + 60)
    y = 12 + i * 22 + 17 + 8
    if log is not None:
        log.append('  spell %d: page %d row %d -> click (%d,%d)' % (spell, p, i, x, y))
    g.click(x, y, settle=2.0)
    if any(c[4] == 'O' for c in controls(lv)):  # a variable spell's extra-power dialog (CASTUI.C): OK at the default
        if log is not None:
            log.append('  extra-power dialog: OK')
        D.key('o')
        time.sleep(1.5)
    if target is not None:
        gx, gy = lv.get('battle_units', target, 'combat_x'), lv.get('battle_units', target, 'combat_y')
        sx, sy = grid_to_screen(gx, gy)
        if log is not None:
            log.append('  target unit %d at (%d,%d) -> click (%d,%d)' % (target, gx, gy, sx, sy + 8))
        time.sleep(1)
        g.click(sx, sy + 8, settle=2.0)
    time.sleep(2)


def stable(lv, table, i, fields, n=3, gap=1.2):
    """Reads that agree n times in a row (a battle record caught mid-recompute or under a dialog reads wrong)."""
    last, same = None, 0
    for _ in range(12):
        v = tuple(lv.get(table, i, f) for f in fields)
        same = same + 1 if v == last else 1
        last = v
        if same >= n:
            return dict(zip(fields, v))
        time.sleep(gap)
    raise RuntimeError('unstable reads of %s %d' % (table, i))


def next_human_turn(lv, log=None, timeout=120):
    """End the human's combat turn (Done on every unit) and wait for the next one."""
    n0 = lv.glob('combat_turn_count')
    t0 = time.time()
    while in_combat(lv) and lv.glob('combat_turn_count') == n0 and time.time() - t0 < 40:
        lv.D.key('d')
        time.sleep(1.2)
    while time.time() - t0 < timeout:
        if in_combat(lv) and lv.glob('combat_turn_count') > n0:
            time.sleep(1.5)
            return True
        time.sleep(1)
    raise RuntimeError('no next human combat turn')


def in_combat(lv):
    """The human's combat turn: the combat buttons are in the control table (Auto at (170,178), COMBAT.C). Note:
    current_screen stays SCREEN_MAP during a battle and 'battle_over' is a local of Combat_Main."""
    return any(c[:2] == (170, 178) and c[4] == 'A' for c in controls(lv))


def on_map(lv):
    """The overland map waits for input: its top bar is in the control table (Game at x 7, y 4, hotkey G)."""
    return lv.screen() == N('SCREEN_MAP') and any(c[4] == 'G' and c[1] == 4 for c in controls(lv))


def start_combat(lv, key, log=None, timeout=60):
    """Press the keypad key that moves the selected stack onto the enemy; answer a confirmation with Y; wait until
    the human has the combat turn (the combat buttons are up)."""
    lv.D.key(key)
    t0 = time.time()
    k = 0
    while time.time() - t0 < timeout:
        time.sleep(1)
        if in_combat(lv):
            time.sleep(1.5)
            return True
        k += 1
        if k % 4 == 0:
            lv.D.key('y')
    raise RuntimeError('no combat turn for the human (screen %d, battle units %d)'
                       % (lv.screen(), lv.glob('battle_unit_count')))


def battle_units(lv, fields=('unit_index', 'owner', 'status', 'combat_x', 'combat_y', 'figures_alive',
                             'hits_per_figure', 'melee', 'ranged', 'defense', 'resist', 'moves')):
    return [{f: lv.get('battle_units', i, f) for f in fields} for i in range(lv.glob('battle_unit_count'))]


def finish_combat(lv, log=None, timeout=240):
    """Auto combat for the human side, then answer the results until the overland map is back."""
    D = lv.D
    t0 = time.time()
    while time.time() - t0 < timeout:
        if on_map(lv):
            return True
        if in_combat(lv):
            D.key('a')
        elif not items_award(lv):
            D.key('enter')                      # the results scroll and what follows it
        time.sleep(2)
    return False


if __name__ == '__main__':
    if sys.argv[1:] == ['check']:
        size = os.path.getsize(ORIG)
        print('save map %d bytes, SAVE9_orig.GAM %d bytes -> %s' % (SAVE_SIZE, size, 'OK' if size == SAVE_SIZE else 'MISMATCH'))
        s = SaveFile()
        print('turn %d, players %d, cities %d, units %d, difficulty %d' % (s.get('game_turn'), s.get('num_players'),
              s.get('num_cities'), s.get('num_units'), s.get('difficulty')))
        print('wizard 0 gold %d mana %d fame %d' % (s.get('wizards', 0, 'gold'), s.get('wizards', 0, 'mana_reserve'),
              s.get('wizards', 0, 'fame')))
        print('city 0', bytes(s.get('cities', 0, 'name')).split(b'\0')[0], 'pop', s.get('cities', 0, 'population'),
              'enchants', s.get('cities', 0, 'enchants'))
