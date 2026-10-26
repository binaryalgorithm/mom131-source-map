"""P4 B4 UI cases, batch 1 (N246): the settings screen, the Yes / No box, the lair treasure text."""
import os
import time

import mom
from regress import case

N = mom.N


@case(106, 'UI', 'Settings: Esc does not write MAGIC.SET, but the toggles already changed game_config in memory',
      setup='the game menu (G) -> Settings; the Strategic Combat Only toggle clicked, then Esc, Esc (cases_ov6 '
            '_strategic_on); game_config.strategic_combat and the MAGIC.SET bytes read before and after',
      bug='MAGIC.SET unchanged, game_config.strategic_combat 0 -> 1: the "cancelled" change is in effect',
      fixed='Esc restores the old game_config')
def c106(log):
    import cases_ov6 as O6
    path = mom.ROOT + 'work/harness/game/MAGIC.SET'
    f0 = open(path, 'rb').read() if os.path.exists(path) else None
    lv = mom.boot()
    v0 = O6._config_strategic(lv)
    v1 = O6._strategic_on(lv, log=log)
    f1 = open(path, 'rb').read() if os.path.exists(path) else None
    obs = ('strategic_combat %d before, %d after Esc; MAGIC.SET %s (%s bytes)'
           % (v0, v1, 'unchanged' if f0 == f1 else 'CHANGED', len(f1) if f1 else None))
    if v0 != 0:
        return dict(shown=None, observed=obs + ' - the option was already on')
    return dict(shown=v1 == 1 and f0 == f1, observed=obs)


def _confirm_up(lv):
    keys = {c[4].upper() for c in mom.controls(lv)}
    return 'Y' in keys and 'N' in keys


@case(389, 'UI', 'Confirm_Box (Yes / No) cannot be cancelled with Esc or a click outside',
      setup='item 1 moved from hero 69 to the human\'s vault slot 0; Armies (A) -> Items (I), the item picked up and '
            'dropped on the anvil: "Do you want to destroy your ...?" (Confirm_Box); Esc pressed, then a click at '
            '(5,5) outside the box; the box\'s Y / N controls read after each; N closes it',
      bug='the box stays up after Esc and after the outside click (abs() of the input, the Esc control never tested)',
      fixed='Esc / outside closes it as No')
def c389(log):
    s = mom.SaveFile()
    s.set('wizards', 0, 'hired_heroes[2].item_index[0]', -1)
    s.set('wizards', 0, 'vault_items[0]', 1)
    for k in (1, 2, 3):
        s.set('wizards', 0, 'vault_items[%d]' % k, -1)
    lv = mom.boot(s)
    lv.D.key('a')
    time.sleep(3)
    lv.D.key('i')
    time.sleep(3)
    lv.g.click(79, 181, settle=1.5)                 # vault slot 0: the item picked up
    lv.g.click(45, 174, settle=2.0)                 # the anvil
    up0 = _confirm_up(lv)
    mom.shot('box', log)
    lv.D.key('esc')
    time.sleep(1.5)
    up1 = _confirm_up(lv)
    lv.g.click(5, 5, settle=1.5)
    up2 = _confirm_up(lv)
    mom.shot('after_outside_click', log)
    lv.D.key('n')
    time.sleep(1.5)
    up3 = _confirm_up(lv)
    cost = lv.get('items', 1, 'cost')
    for _ in range(4):
        if mom.on_map(lv):
            break
        lv.D.key('esc')
        time.sleep(1.5)
    obs = ('the Yes / No box up: after the anvil %s, after Esc %s, after the outside click %s, after N %s; item 1 '
           'cost %d (kept)' % (up0, up1, up2, up3, cost))
    if not up0:
        return dict(shown=None, observed=obs + ' - the box did not come up')
    return dict(shown=up1 and up2 and not up3, observed=obs)


def _treasure_text(gold, log, tag):
    import cases_ov5 as O5
    s = mom.SaveFile()
    key = O5._walker_lair(s, lair_type=N('LAIR_ANCIENT_TEMPLE'))   # lair 0 is a tower in the base save: a
    s.set('lairs', 0, 'gold', gold)                                  # tower has no treasure box (10/24)
    lv = mom.boot(s)
    g0 = lv.get('wizards', 0, 'gold')
    lv.D.key(key)
    got = []

    def inside():                               # the "You have found ..." prompt is answered first
        p = lv.glob('p_treasure_text') & 0xffff
        if p:
            raw = lv.g.ram(lv.g.ds + p, 120).split(b'\0')[0]
            if raw.startswith(b'Inside'):
                got.append(raw.decode('latin-1'))
                return True
        return False
    O5._answer_by_hotkey(lv, log=log, timeout=40, until=inside)
    text = got[0] if got else None
    mom.shot('treasure_%d' % gold, log)
    O5._answer_by_hotkey(lv, log=log, timeout=30)
    g1 = lv.get('wizards', 0, 'gold')
    log.append('%s: "%s"; gold %d -> %d' % (tag, text, g0, g1))
    return text, g0, g1


@case(249, 'UI', 'lair treasure of exactly 1 gold (or mana) is granted but not mentioned',
      setup='lair 0 made an Ancient Temple at (30,28) without guards, gold 1, nothing else; unit 1 walks in; the '
            'treasure text (p_treasure_text) and the human\'s gold read. Control: gold 2',
      bug='"Inside you find " with no gold named, yet +1 gold; 2 gold are named', fixed='"Inside you find 1 gold"')
def c249(log):
    t1, a0, a1 = _treasure_text(1, log, 'gold 1')
    t2, b0, b1 = _treasure_text(2, log, 'control, gold 2')
    obs = 'gold 1: text "%s", gold %d -> %d; gold 2: text "%s", gold %d -> %d' % (t1, a0, a1, t2, b0, b1)
    if t1 is None or t2 is None:
        return dict(shown=None, observed=obs + ' - a treasure text was not read')
    if 'gold' not in t2:
        return dict(shown=None, observed=obs + ' - the control does not name its gold either')
    # both runs also gain the same other gold in the step (+15 seen 10/24): the 1-gold run gains exactly 1 less
    return dict(shown='gold' not in t1 and (b1 - b0) - (a1 - a0) == 1, observed=obs)


def _book_where(lv, spell):
    """(page, row) of `spell` in p_spellbook_pages (abs of the ids), and spellbook_page_count."""
    import struct
    from layout import L
    pages = lv.g.ds + (lv.glob('p_spellbook_pages') & 0xffff)
    sz, n = L.size('SpellbookPage'), lv.glob('spellbook_page_count')
    for pg in range(n):
        cnt = lv.g.ram(pages + pg * sz + L.off('SpellbookPage', 'count'), 1)[0]
        ids = struct.unpack_from('<6h', lv.g.ram(pages + pg * sz + L.off('SpellbookPage', 'spells'), 12))
        for k in range(min(cnt, 6)):
            if abs(ids[k]) == spell:
                return (pg, k), n
    return None, n


def _game_config(lv, field):
    import struct
    fo = mom.resolve('GameConfig', field)[0]
    return struct.unpack('<h', lv.g.ram(lv.g.ds + mom.SYM['game_config'][0] + fo, 2))[0]


def _learn(spell, log, tag):
    """The human researching `spell` with 1 point left (learned at the turn change); the first slot of the realm
    before (spell_library[r - 1][0]) KNOWN; every other human spell cleared, so the research book holds those two
    spells on at most two pages: Screen_Spell_Learned's first search stops two pages short of the end
    (i < spellbook_page_count - 2), so the found == 0 branch (the bug's line) runs - as for any spell on the book's
    last two pages (10/24: with the whole library and Spell Book Ordering on, 41 was found on an earlier page). Tracepoint on that line (entered via Screen_Spell_Learned: it loads LBX / builds the book first); the turn
    is then finished. -> (learned state, slot before, slot after, the line hit (spell's (page, row), page count) or
    None, Screen_Spell_Learned entered)."""
    import dbg
    s = mom.SaveFile()
    r, slot = (spell - 1) // 40, (spell - 1) % 40
    for rr in range(6):                         # the book ordered by section (10/24): clear every other spell
        for k in range(40):
            if s.get('wizards', 0, 'spell_library[%d][%d]' % (rr, k)) != 0:
                s.set('wizards', 0, 'spell_library[%d][%d]' % (rr, k), 0)
    s.set('wizards', 0, 'research_spell', spell)
    for k in range(8):                          # the only candidate: once learned none is left, so the research book
        s.set('wizards', 0, 'research_candidates[%d]' % k, spell if k == 0 else 0)   # has no Research / Spells
    #                                             pages and its last two pages of known spells go unsearched (10/24)
    s.set('wizards', 0, 'research_left', 1)
    s.set('wizards', 0, 'spell_library[%d][%d]' % (r, slot), N('SPELLSTATE_RESEARCHABLE'))
    s.set('wizards', 0, 'spell_library[%d][0]' % (r - 1), N('SPELLSTATE_KNOWN'))
    d = dbg.boot(s, log=log)
    lv = d.lv
    try:
        anim, order = _game_config(lv, 'spell_animations'), _game_config(lv, 'book_ordering')
        before = lv.get('wizards', 0, 'spell_library[%d][0]' % (r - 1))
        t0 = lv.turn()
        d.want_line('src/wizards/RESEARCH.C', 'wizards[PLAYER_HUMAN].spell_library[(spell - 2) / SPELLS_PER_REALM]'
                    '[SPELL_SLOT(spell)] = SPELLSTATE_RESEARCHABLE;', fn='Screen_Spell_Learned', label='demote')
        poll = d.end_turn_poll()
        h = d.wait_via_entry('Screen_Spell_Learned', 240, poll)
        entered = any(x.get('name') == '#entry Screen_Spell_Learned' for x in d.hits)
        line = None
        if h is not None and h['name'] == 'demote':
            line = _book_where(lv, spell)
            d.drop('demote')
        d.go()
        t1 = time.time()
        while time.time() - t1 < 150 and not (lv.turn() > t0 and mom.on_map(lv)):
            d.wait(3, poll)
        for _ in range(6):
            if mom.on_map(lv):
                break
            d.press('enter')
            d.wait(2, None)
        after = lv.get('wizards', 0, 'spell_library[%d][0]' % (r - 1))
        learned = lv.get('wizards', 0, 'spell_library[%d][%d]' % (r, slot))
    finally:
        d.stop()
    log.append('%s: spell %d learned (state %d); spell_library[%d][0] %d -> %d; Screen_Spell_Learned entered %s; '
               'the found == 0 line %s (spell animations %d, book ordering %d)'
               % (tag, spell, learned, r - 1, before, after, entered,
                  'hit: (page, row), page count %s' % (line,) if line else 'NOT hit', anim, order))
    return learned, before, after, line, entered


@case(475, 'UI', 'the learned-spell screen\'s (spell - 2) / 40 realm: a realm\'s first spell demotes the previous '
                 'realm\'s first slot',
      setup='the human researching spell 41 (Sorcery\'s first slot) with 1 point left, Nature\'s first slot '
            '(spell_library[0][0]) KNOWN, every other human spell cleared and 41 the only research candidate: once '
            'it is learned no candidate is left (the end-game state; 10/24 the premise that the book then has no '
            'Research / Spells pages was wrong - Spellbook_Fill_Group adds them even empty, so the branch is never '
            'reached and the case reports NOT REACHED with that reason); tracepoint on that '
            'branch\'s line; the turn ended, the screens answered; the two states read. Control: spell 42 (Sorcery\'s '
            'second slot) - the same slot 0 of Nature untouched',
      bug='spell_library[0][0] KNOWN -> RESEARCHABLE (Screen_Spell_Learned marks [(41 - 2) / 40][0])',
      fixed='Nature\'s first spell stays known')
def c475(log):
    l1, b1, a1, h1, e1 = _learn(41, log, 'spell 41')
    l2, b2, a2, h2, e2 = _learn(42, log, 'control, spell 42')
    k, r = N('SPELLSTATE_KNOWN'), N('SPELLSTATE_RESEARCHABLE')
    obs = ('spell 41: learned state %d, Nature slot 0 %d -> %d, the found == 0 line %s; spell 42: learned state %d, '
           'Nature slot 0 %d -> %d, the line %s (known %d, researchable %d)'
           % (l1, b1, a1, h1 or 'not hit', l2, b2, a2, h2 or 'not hit', k, r))
    if not e1:
        return dict(shown=None, observed=obs + ' - the learned-spell screen was not entered')
    if h1 is None:
        return dict(shown='latent', observed=obs + ' - the found == 0 branch did not run (the spell was found on an '
                                               'earlier page). Latent: the branch is dead code in 1.31 - '
                                               'Wizard_Learn_Spell makes the spell KNOWN before this screen, the '
                                               'research book lists every known spell, and Spellbook_Fill_Group adds '
                                               'the Research / Spells pages even when empty, so the first search '
                                               '(all pages but those two) always finds it')
    if a2 != k:
        return dict(shown=None, observed=obs + ' - the control changed the slot too')
    return dict(shown=b1 == k and a1 == r, observed=obs)
