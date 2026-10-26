"""P4 B4 overland movement cases (N230): ships, Go-To and boarding, set up by save edits next to Dracon (29,27);
the moving stack is the one the game selects after loading (the only unit left without orders)."""
import time

import mom
from regress import case

N = mom.N
SHORE = (29, 26)                                # shore water north of Dracon
TRIREME = 35                                    # transport capacity 2
SWORDSMEN = 40                                  # Barbarian Swordsmen: a plain walking unit


def _all_patrol(s):
    """Every human unit on Patrol and finished: Patrol alone left Dracon's stack selected after loading (#167 / #171,
    10/23), so the keypad moved it instead of the stack under test."""
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0:
            s.set('units', u, 'order', N('ORDER_PATROL'))
            s.set('units', u, 'finished', 1)


def _place(s, u, xy, typ=None, order=None):
    for f, v in (('x', xy[0]), ('y', xy[1]), ('plane', 0)):
        s.set('units', u, f, v)
    if typ is not None:
        s.set('units', u, 'type', typ)
        s.set('units', u, 'hero_slot', -1)
        s.set('units', u, 'moves_max', 0)       # recomputed from the type at load
    if order is not None:
        s.set('units', u, 'order', order)
    s.set('units', u, 'finished', 0)


def _selected(lv):
    """The selected stack: stack_units[] (struct StackSlot: unit, active) for stack_unit_count."""
    import struct
    base = lv.g.ds + mom.SYM['stack_units'][0]
    return [struct.unpack('<hh', lv.g.ram(base + 4 * k, 4)) for k in range(lv.glob('stack_unit_count'))]


def _units_at(lv, xy):
    return [u for u in range(lv.glob('num_units'))
            if (lv.get('units', u, 'x'), lv.get('units', u, 'y'), lv.get('units', u, 'plane')) == xy + (0,)
            and lv.get('units', u, 'owner') == 0]


@case(171, 'OV', 'moving onto a square where an own transport has moves sentries every other unit there',
      setup='unit 192 made a Trireme (35) with moves in Dracon; units 33 and 69 in Dracon left Ready but finished (so '
            'the game selects hero 19, not their stack); hero 19 placed at (30,28) (Ready) and moved into Dracon with '
            'the keypad (7)',
      bug='33 and 69 end on Patrol and finished (also in a port city, on land)', fixed='their orders unchanged')
def c171(log):
    s = mom.SaveFile()
    _all_patrol(s)
    _place(s, 192, (29, 27), typ=TRIREME, order=N('ORDER_PATROL'))
    s.set('units', 192, 'moves_left', 4)
    s.set('units', 192, 'finished', 1)            # moves left but done: else the game selects the Trireme (10/23)
    for u in (33, 69):
        _place(s, u, (29, 27), order=N('ORDER_READY'))
        s.set('units', u, 'finished', 1)        # Ready but done: else the game selects Dracon's stack, not hero 19
    _place(s, 19, (30, 28), order=N('ORDER_READY'))   # (10/23: kp7 moved the Dracon stack)
    s.set('units', 19, 'moves_left', s.get('units', 19, 'moves_max'))
    lv = mom.boot(s)
    before = {u: (lv.get('units', u, 'order'), lv.get('units', u, 'finished')) for u in (33, 69)}
    log.append('selected stack after loading: %s' % _selected(lv))
    lv.D.key('kp7')
    time.sleep(4)
    after = {u: (lv.get('units', u, 'order'), lv.get('units', u, 'finished')) for u in (33, 69)}
    pos19 = (lv.get('units', 19, 'x'), lv.get('units', 19, 'y'))
    mom.shot('after_move', log)
    obs = 'hero 19 now at %s; (order, finished) of 33 / 69: %s -> %s (1 = Patrol)' % (pos19, before, after)
    return dict(shown=pos19 == (29, 27) and all(after[u] == (N('ORDER_PATROL'), 1) for u in (33, 69)), observed=obs)


@case(168, 'OV', 'a ship on Go-To carries one unit more than its capacity',
      setup='unit 192 made a Trireme (capacity 2) on the shore water (29,26) with three Barbarian Swordsmen (units 161, '
            '173, 107 retyped) aboard; all four on Go-To to the ocean square (32,25); one turn change',
      bug='three swordsmen move with the ship (GoTo_Build_Group takes a unit while carried <= capacity)',
      fixed='two')
def c168(log):
    s = mom.SaveFile()
    _all_patrol(s)
    _place(s, 192, SHORE, typ=TRIREME, order=N('ORDER_GOTO'))
    for u in (161, 173, 107):
        _place(s, u, SHORE, typ=SWORDSMEN, order=N('ORDER_GOTO'))
    for u in (192, 161, 173, 107):
        s.set('units', u, 'dest_x', 32)
        s.set('units', u, 'dest_y', 25)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    time.sleep(2)

    def at(typ):                                # by type and position: the unit table is compacted at the end of
        out = {}                                # the turn (10/23: index 192 was another unit afterwards)
        for u in range(lv.glob('num_units')):
            if lv.get('units', u, 'owner') == 0 and lv.get('units', u, 'type') == typ and lv.get('units', u, 'plane') == 0:
                xy = (lv.get('units', u, 'x'), lv.get('units', u, 'y'))
                out[xy] = out.get(xy, 0) + 1
        return out
    ships, swords = at(TRIREME), at(SWORDSMEN)
    near = {xy: n for xy, n in swords.items() if abs(xy[0] - SHORE[0]) <= 4 and abs(xy[1] - SHORE[1]) <= 4}
    mom.shot('after_goto', log)
    obs = 'turn %d: human Triremes at %s; Barbarian Swordsmen near the start %s (start %s, Go-To (32,25))' % (
        t, ships, near, SHORE)
    moved = [xy for xy in near if xy != SHORE and xy in ships]
    return dict(shown=any(near[xy] == 3 for xy in moved), observed=obs)


@case(167, 'OV', 'a ship stack that needs no carrying leaves its Go-To units behind',
      setup='unit 192 made a Trireme on the shore water (29,26) (Ready, selected); unit 173 made a Djinn (195, Wind '
            'Walking: Stack_Needs_Transport answers nobody) there '
            'Ready; unit 161 made Barbarian Swordsmen there on Go-To to (40,10), finished (not in the active selection, '
            'so the selected ships need no carrying); keypad 9 moves the selected stack to '
            '(30,25)', bug='161 stays on (29,26) (Stack_Needs_Transport says nobody needs carrying, Go-To units are '
            'skipped)', fixed='161 carried along')
def c167(log):
    s = mom.SaveFile()
    _all_patrol(s)
    _place(s, 192, SHORE, typ=TRIREME, order=N('ORDER_READY'))
    s.set('units', 192, 'moves_left', 4)
    _place(s, 173, SHORE, typ=N('UNIT_DJINN'), order=N('ORDER_READY'))   # Wind Walking: Stack_Needs_Transport
    #                 answers 'nobody' (10/23: with a swimming Lizardman Dragon Turtle the swordsmen were carried)
    _place(s, 161, SHORE, typ=SWORDSMEN, order=N('ORDER_GOTO'))
    s.set('units', 161, 'dest_x', 40)
    s.set('units', 161, 'dest_y', 10)
    s.set('units', 161, 'finished', 1)           # not in the active selection: an active 161 made the stack 'need
                                                 # carrying' and took the capacity branch (10/23: carried along)
    lv = mom.boot(s)
    log.append('selected stack after loading: %s' % _selected(lv))
    lv.D.key('kp9')
    time.sleep(4)
    pos = {u: (lv.get('units', u, 'x'), lv.get('units', u, 'y')) for u in (192, 173, 161)}
    mom.shot('after_move', log)
    obs = 'after the move: %s (started on %s)' % (pos, SHORE)
    return dict(shown=pos[192] != SHORE and pos[161] == SHORE, observed=obs)


@case(205, 'OV', 'after an overland summon the new unit is not selected',
      setup='the human has finished Guardian Spirit (129, a summon), cast when the turn changes; a conditional '
            'breakpoint on Select_Stack_Of_Unit, stopping only when called from Overland_Cast_Spell (its return '
            'address): the unit index it reads at [bp+6] (nothing is pushed) against the new unit (num_units - 1) '
            '(10/23: reading the selection after the turn found none selected - no evidence either way)',
      bug='Select_Stack_Of_Unit reads a stale word, not the new unit\'s index',
      fixed='the new unit (num_units - 1) is selected')
def c205(log):
    import dbg
    s = mom.SaveFile()
    for f, v in (('casting_spell', N('SPELL_GUARDIAN_SPIRIT')), ('casting_cost_left', 0), ('casting_cost_initial', 80),
                 ('mana_reserve', 5000)):
        s.set('wizards', 0, f, v)
    d = dbg.boot(s, log=log)
    got = None
    try:
        def caller(h):
            return d.where(d.word(h['SS'], h['SP'] + 2, False), d.word(h['SS'], h['SP'], False))
        d.want('Select_Stack_Of_Unit', when=lambda h: 'Overland_Cast_Spell' in caller(h))
        h = d.wait(300, d.turns_poll(s.get('game_turn') + 1))
        if h is not None:
            got = (d.arg(0, h), d.lv.glob('num_units'), caller(h),
                   d.lv.get('units', d.lv.glob('num_units') - 1, 'type'))
        d.drop('Select_Stack_Of_Unit')
        d.go()
        mom.shot('after_summon', log)
    finally:
        d.stop()
    if got is None:
        return dict(shown=None, observed='no Select_Stack_Of_Unit call from Overland_Cast_Spell (skipped other calls: '
                                         '%s)' % d.skipped)
    arg, n, who, t = got
    obs = ('Select_Stack_Of_Unit called from %s reads unit %d; the new unit is %d (num_units - 1, type %d)' % (
        who, arg, n - 1, t))
    return dict(shown=arg != n - 1, observed=obs)


@case(254, 'OV', 'Water Walking + non-corporeal counts twice for crossing water: a land unit is taken onto the sea',
      setup='every human unit on Patrol and done; units 161 and 173 made Barbarian Swordsmen (land units) in Dracon '
            '(29,27), Ready, full moves, 161 enchanted with Water Walking (0x100) and Wraith Form (0x20); keypad 8 '
            'moves the selected stack north onto the shore water (29,26) (tile 82); the last unit made a human '
            'Ready swordsman at (28,28) so the turn does not end by itself', bug='both move onto the water (161 adds '
            '2 to the swimmers count: Stack_Movement_Modes sees 2 of 2)', fixed='refused (173 cannot cross water)')
def c254(log):
    s = mom.SaveFile()
    _all_patrol(s)
    for u in (161, 173):
        _place(s, u, (29, 27), typ=SWORDSMEN, order=N('ORDER_READY'))
        s.set('units', u, 'moves_left', 2)
    s.set('units', 161, 'enchantments', N('UNIT_ENCH_WATER_WALKING') | N('UNIT_ENCH_WRAITH_FORM'))
    s.set('units', 173, 'enchantments', 0)
    last = s.get('num_units') - 1               # a Ready unit elsewhere with a higher index: else the turn ends by
    s.set('units', last, 'owner', 0)            # itself after the move and the unit table is compacted before the
    _place(s, last, (28, 28), typ=SWORDSMEN, order=N('ORDER_READY'))   # read (10/23: 161 / 173 read as (40,31))
    lv = mom.boot(s)
    log.append('selected stack after loading: %s' % _selected(lv))
    lv.D.key('kp8')
    time.sleep(4)
    pos = {u: (lv.get('units', u, 'x'), lv.get('units', u, 'y')) for u in (161, 173)}
    mom.shot('after_move', log)
    obs = 'after keypad 8: %s (water square %s)' % (pos, SHORE)
    if pos[161] == (29, 27) and pos[173] == (29, 27):
        sel = _selected(lv)
        if not any(u in (161, 173) for u, _ in sel):
            return dict(shown=None, observed=obs + ' - the stack was not selected: %s' % sel)
    return dict(shown=pos[173] == SHORE, observed=obs)
