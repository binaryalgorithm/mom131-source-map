"""P4 B4 overland cases, N232: watchpoints / tracepoints (dbg.py) and plain before / after reads."""
import time

import dbg
import mom
from cases_dbg1 import _ai_casts, _ai_hostile
from cases_tgt2 import _human_cast
from regress import case

N = mom.N


def _stop_at_target_screen(d, poll):
    """A poll that ends the turn but leaves the human's spell target screen alone (turns_poll would answer it)."""
    def p():
        if d.lv.glob('spelltarget_prompt') == 0:
            poll()
    return p


@case(160, 'OV', 'breaking a treaty RAISES the breaker\'s relation_bias toward the victim (penalty subtracted)',
      setup='as #172: the human and AI wizard 1 have a Wizard Pact; a wizard-1 unit next to Dracon; Dracon\'s stack '
            'attacks it and confirms (Y); Diplo_Break_Treaty(1, 0) runs (#172: the victim is named the breaker); a '
            'memory-write watchpoint on wizards[1].relation_bias[0] (the breaker\'s toward the victim), writes by '
            'Diplo_Break_Treaty only', bug='+10 (the pact penalty -10 subtracted)', fixed='-10')
def c160(log):
    import cases_cb
    s = mom.SaveFile()
    for a, b in ((0, 1), (1, 0)):
        for f, v in (('diplomacy_status[%d]', N('TREATY_WIZARD_PACT')), ('contacted[%d]', 1), ('met_stage[%d]', 2)):
            s.set('wizards', a, f % b, v)
    s.set('wizards', 1, 'personality', N('PERSONALITY_PEACEFUL'))   # not Lawful: the penalty stays 10
    key = mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [2], (30, 28, 0))
    s.set('units', 2, 'owner', 1)
    d = dbg.boot(s, log=log)
    h = None
    try:
        d.watch('bias', ('wizards', 1, 'relation_bias[0]'), size=1, when=lambda h: 'Diplo_Break_Treaty' in h['writer'])
        k = [0]

        def poll():
            k[0] += 1
            if k[0] == 1:
                d.action()
                d.lv.D.key(key)
            elif k[0] % 3 == 0:
                d.lv.D.key('y')
        h = d.wait(90, poll)
        d.unwatch('bias')
        d.go()
        mom.shot('after', log)
        if mom.in_combat(d.lv) or not mom.on_map(d.lv):
            mom.finish_combat(d.lv, log=log)
    finally:
        d.stop()
    if h is None:
        return dict(shown=None, observed='no write to wizards[1].relation_bias[0] by Diplo_Break_Treaty')
    old, new = (int.from_bytes(h[x], 'little', signed=True) for x in ('old', 'new'))
    obs = 'Diplo_Break_Treaty (pact, breaker 1 not Lawful): wizards[1].relation_bias[0] %d -> %d (by %s)' % (
        old, new, h['writer'])
    return dict(shown=new - old == 10, observed=obs)


@case(129, 'OV', 'collateral damage rolls buildings already removed this turn (state 2) again',
      setup='AI city 2 (wizard 2, Arcanus (4,11); city 1 is on Myrror and the target screen views Arcanus): '
            'buildings 1-10 set to state 2 (removed this turn, cleared only at the turn end); '
            'the human has finished Call the Void (119: City_Collateral_Damage at 50 %) and targets the city; a '
            'tracepoint on City_Try_Remove_Building records (building, city) and the building\'s state at the call',
      bug='state-2 buildings are rolled and removed again', fixed='only standing buildings (state 1)')
def c129(log):
    s = mom.SaveFile()
    city = 2
    cx, cy = s.get('cities', city, 'x'), s.get('cities', city, 'y')
    for b in range(1, 11):
        s.set('cities', city, 'buildings[%d]' % b, N('BLDSTATE_REMOVED'))
    _human_cast(s, N('SPELL_CALL_THE_VOID'))
    d = dbg.boot(s, log=log)
    try:
        d.want('City_Try_Remove_Building', trace=lambda h: (
            d.arg(0, h), d.arg(1, h), d.lv.get('cities', d.arg(1, h), 'buildings[%d]' % d.arg(0, h))))
        poll, t0 = _stop_at_target_screen(d, d.end_turn_poll()), time.time()
        while time.time() - t0 < 200 and d.lv.glob('spelltarget_prompt') == 0:
            d.wait(5, poll)
        if d.lv.glob('spelltarget_prompt') == 0:
            return dict(shown=None, observed='the spell target screen did not come up')
        mom.target_square(d.lv, cx, cy, log=log)
        d.wait(15, None)
        mom.shot('after_cast', log)
        calls = d.traces.get('City_Try_Remove_Building', [])
        d.drop('City_Try_Remove_Building')
        d.go()
    finally:
        d.stop()
    again = [c for c in calls if c[1] == city and c[2] == N('BLDSTATE_REMOVED')]
    obs = 'City_Try_Remove_Building calls (building, city, state at the call): %s; on state-2 buildings: %d' % (
        calls, len(again))
    if not calls:
        return dict(shown=None, observed=obs)
    if not again:                               # 10/23: the saved state-2 buildings were reset by the turn end
        return dict(shown=None, observed=obs + ' - no state-2 building left at the cast: the turn end '   # before
                    '(Turn_Finalize_Removed_Buildings) cleared them; needs two damage events in ONE turn (two '   # the
                    'battles at the city)')                                                                     # cast
    return dict(shown=True, observed=obs)


@case(34, 'OV', 'AI Nature\'s Cures uses the target unit\'s x coordinate as a unit index',
      setup='AI wizard 1\'s first unit (its only damaged one: damage 5, its others 0) is the target '
            'AI_Target_Natures_Cures picks; wizard 1 has finished Nature\'s Cures (19); one turn change; damage read '
            'by owner / type / square (the unit table is compacted at the turn end)',
      bug='that unit is not healed (the square of unit #x is healed instead)', fixed='damage 0')
def c34(log):
    s = mom.SaveFile()
    mine = [u for u in range(s.get('num_units')) if s.get('units', u, 'owner') == 1]
    for u in mine:
        s.set('units', u, 'damage', 0)
    a = mine[0]
    s.set('units', a, 'damage', 5)
    key = (1, s.get('units', a, 'type'), s.get('units', a, 'x'), s.get('units', a, 'y'), s.get('units', a, 'plane'))
    _ai_casts(s, 1, N('SPELL_NATURES_CURES'))
    other = s.get('units', a, 'x')                  # the unit index the cast code uses: its square is healed
    s.set('units', other, 'damage', 6)              # damaged, to see that (natural healing is 1-2 a turn)
    okey = tuple(s.get('units', other, f) for f in ('owner', 'type', 'x', 'y', 'plane'))
    lv = mom.boot(s)

    def dmg():
        out = {}
        for u in range(lv.glob('num_units')):
            k = tuple(lv.get('units', u, f) for f in ('owner', 'type', 'x', 'y', 'plane'))
            out.setdefault(k, []).append(lv.get('units', u, 'damage'))
        return out
    before = dmg()
    t = mom.end_turn(lv, log=log)
    after = dmg()
    cs = lv.get('wizards', 1, 'casting_spell')
    mom.shot('after', log)
    healed = [k for k, v in after.items() if k in before and sum(before[k]) > 0 and sum(v) == 0]
    obs = ('turn %d: wizard 1\'s damaged unit %d %s damage %s -> %s; unit #%d (its x) is %s; squares healed to 0: %s; '
           'wizard 1 casting now %d' % (t, a, key, before.get(key), after.get(key), other, okey, healed, cs))
    if cs == N('SPELL_NATURES_CURES'):
        return dict(shown=None, observed=obs + ' - not cast')
    tgt_healed = after.get(key) is not None and max(after[key]) == 0      # max: other units can share the key
    wrong = okey in after and after[okey] and min(after[okey]) == 0
    obs += '; unit #%d\'s square: damage %s -> %s' % (other, before.get(okey), after.get(okey))
    return dict(shown=not tgt_healed and bool(wrong), observed=obs)


@case(78, 'OV', 'AI hero re-equip: the staff / wand slot falls through into the armour case',
      setup='AI wizard 4: its mage hero (hero slot 2, unit 93, slot types 4 / 6 / 6) keeps its slots, the other two '
            'heroes\' slots all made misc (6) so they cannot take armour; its item work record (132) made a shield '
            '(type 7, cost 300); wizard 4 has finished Enchant Item (207) - the cast hands the item to '
            'Hero_Items_AI_Reequip; one turn change: the mage\'s three items read',
      bug='the shield in the mage\'s staff / wand slot (slot 0)', fixed='slot 0 takes only a staff / wand')
def c78(log):
    s = mom.SaveFile()
    for h in (0, 1):
        for k in range(3):
            s.set('wizards', 4, 'hired_heroes[%d].slot_type[%d]' % (h, k), N('ITEM_SLOT_MISC'))
    rec = N('ITEM_REC_WORK') - 4
    s.set('items', rec, 'item_type', N('ITEM_TYPE_SHIELD'))
    s.set('items', rec, 'cost', 300)
    _ai_casts(s, 4, N('SPELL_ENCHANT_ITEM'))
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    slots = [lv.get('wizards', 4, 'hired_heroes[2].slot_type[%d]' % k) for k in range(3)]
    items = [lv.get('wizards', 4, 'hired_heroes[2].item_index[%d]' % k) for k in range(3)]
    types = [lv.get('items', i, 'item_type') if i > -1 else None for i in items]
    cs = lv.get('wizards', 4, 'casting_spell')
    mom.shot('after', log)
    obs = 'turn %d: wizard 4\'s mage hero slots %s hold items %s of types %s (7 shield); wizard 4 casting now %d' % (
        t, slots, items, types, cs)
    if cs == N('SPELL_ENCHANT_ITEM'):
        return dict(shown=None, observed=obs + ' - not cast')
    return dict(shown=slots[0] == N('ITEM_SLOT_MAGIC') and types[0] == N('ITEM_TYPE_SHIELD'), observed=obs)


@case(227, 'OV', 'taking an OUTPOST skips the conquest rules (relation penalty, war, City_Change_Owner)',
      setup='AI city 21 Rastenford (wizard 1) made an outpost (population 0, size 0) at (30,28) next to Dracon, no '
            'defenders there (its units stay on Myrror); human and wizard 1 met, at peace; Dracon\'s stack walks in; '
            'tracepoints on Diplo_Relation_Change (actor 0, observer 1), Diplo_Declare_War and City_Change_Owner '
            '(10/23: City_Change_Owner DOES run on this path - ATTACK.C destroyed: owner change, then City_Remove)',
      bug='no relation event and no war declaration for the capture', fixed='the -40 relation event and war')
def c227(log):
    import cases_cb
    from cases_dbg1 import _walk_in_poll
    s = mom.SaveFile()
    c = 21
    for f, v in (('x', 30), ('y', 28), ('plane', 0), ('population', 0), ('size', 0)):
        s.set('cities', c, f, v)
    for a, b in ((0, 1), (1, 0)):
        for f, v in (('contacted[%d]', 1), ('met_stage[%d]', 2), ('diplomacy_status[%d]', 0)):
            s.set('wizards', a, f % b, v)
    name = bytes(v & 0xff for v in s.get('cities', c, 'name')).split(b'\0')[0].decode('latin-1')
    key = mom.setup_attack(s, cases_cb._human_unit_at_dracon(s), [], (30, 28, 0))
    d = dbg.boot(s, log=log)
    try:
        d.want('Diplo_Relation_Change', trace=lambda h: tuple(d.arg(i, h) for i in range(4)))
        d.want('City_Change_Owner', trace=lambda h: (d.arg(0, h), d.arg(1, h)))
        d.want('Diplo_Declare_War', trace=lambda h: (d.arg(0, h), d.arg(1, h)))
        poll, t0 = _walk_in_poll(d, key), time.time()
        while time.time() - t0 < 60 and (mom.on_map(d.lv) is False or time.time() - t0 < 25):
            d.wait(5, poll)
        rel = [a for a in d.traces.get('Diplo_Relation_Change', []) if a[1] == 0 and a[2] == 1]
        own = d.traces.get('City_Change_Owner', [])
        war = d.traces.get('Diplo_Declare_War', [])
        here = [i for i in range(d.lv.glob('num_cities')) if (d.lv.get('cities', i, 'x'), d.lv.get('cities', i, 'y'),
                                                              d.lv.get('cities', i, 'plane')) == (30, 28, 0)]
        mom.shot('after', log)
    finally:
        d.stop()
    obs = ('outpost %s: Diplo_Relation_Change(actor 0, observer 1) calls %s; Diplo_Declare_War calls %s; '
           'City_Change_Owner calls %s; a city at (30,28) now: %s' % (name, rel, war, own, here))
    if here:
        return dict(shown=None, observed=obs + ' - the outpost was not taken')
    return dict(shown=not rel and not war, observed=obs)


@case(259, 'OV', 'Gaia\'s Blessing treats a Chaos Node square as a volcano (can turn it into hills)',
      setup='Cornwell (3, at (14,29)) with Gaia\'s Blessing; the square (15,29) in its area made a Chaos Node tile '
            '(TILE_CHAOS_NODE); during the turn change, after City_Gaias_Blessing_Turn(3) is entered, the '
            'Tile_Is_Volcano call for (15,29): its answer', bug='1 (the node counts as a volcano: 10 % a turn to '
            'become hills)', fixed='0')
def c259(log):
    s = mom.SaveFile()
    W = N('MAP_WIDTH')
    cx, cy = s.get('cities', 3, 'x'), s.get('cities', 3, 'y')
    nx, ny = cx + 1, cy
    s.set('terrain', ny * W + nx, value=N('TILE_CHAOS_NODE'))
    s.set('cities', 3, 'enchants[%d]' % N('CITY_ENCH_GAIAS_BLESSING'), 1)
    d = dbg.boot(s, log=log)
    try:
        d.want('City_Gaias_Blessing_Turn', when=lambda h: d.arg(0, h) == 3)
        h = d.wait(200, d.end_turn_poll())
        if h is None:
            return dict(shown=None, observed='City_Gaias_Blessing_Turn(3) was not reached')
        d.drop('City_Gaias_Blessing_Turn')
        args, r = d.call('Tile_Is_Volcano', lambda a: tuple(a) == (nx, ny, 0), nargs=3, timeout=30)
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if args is None:
        return dict(shown=None, observed='no Tile_Is_Volcano(%d, %d, 0) call after City_Gaias_Blessing_Turn(3)' % (nx, ny))
    obs = 'City_Gaias_Blessing_Turn(3): Tile_Is_Volcano(%d, %d, 0) on the Chaos Node tile = %d' % (nx, ny, r)
    return dict(shown=r == 1, observed=obs)


@case(298, 'OV', 'a Healer\'s mark overwrites the natural-heal divisor: units after it (by index) heal less',
      setup='on the field square (30,28): human units 107 and 173 made Hydras (163: 9 figures x 10 hits = 90; a '
            'Chaos creature, heals normally), damage 60 each, and unit 161 between them made High Men Priests (108, '
            'Healer); all on Patrol; one turn change; the two Hydras read by index order (the lower is 107\'s)',
      bug='the lower one heals natural (90 / 20: about 4-5) + 18, the higher 18 + about 1 (divisor 66)',
      fixed='the same healing')
def c298(log):
    from cases_move import _place
    s = mom.SaveFile()
    for u, t in ((107, N('UNIT_HYDRA')), (161, N('UNIT_HIGH_MEN_PRIESTS')), (173, N('UNIT_HYDRA'))):
        _place(s, u, (30, 28), typ=t, order=N('ORDER_PATROL'))
        s.set('units', u, 'finished', 1)
        s.set('units', u, 'enchantments', 0)
    for u in (107, 173):
        s.set('units', u, 'damage', 60)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    hydras = sorted(u for u in range(lv.glob('num_units')) if lv.get('units', u, 'type') == N('UNIT_HYDRA')
                    and lv.get('units', u, 'owner') == 0 and (lv.get('units', u, 'x'), lv.get('units', u, 'y'),
                                                              lv.get('units', u, 'plane')) == (30, 28, 0))
    dmg = [lv.get('units', u, 'damage') for u in hydras]
    mom.shot('after', log)
    obs = 'turn %d: the Hydras at (30,28) by index %s: damage 60 -> %s (healed %s)' % (t, hydras, dmg,
                                                                                       [60 - x for x in dmg])
    if len(dmg) != 2:
        return dict(shown=None, observed=obs)
    return dict(shown=(60 - dmg[0]) - (60 - dmg[1]) >= 2, observed=obs)


@case(324, 'OV', 'an AI with nothing left to research REPLACES its skill share with half its research share',
      setup='AI wizard 1: every RESEARCHABLE spell (state 1) in its library made KNOWN (candidates, state 3, made '
            'known too), research / skill / mana 40 / 30 / 30, ai_magic_timer 1 (the re-split runs in its next '
            'turn); one turn change: the percentages read', bug='skill 20 (40 / 2, the old 30 dropped), research 0',
            fixed='skill 30 + 20 = 50')
def c324(log):
    s = mom.SaveFile()
    n = 0
    for r in range(6):
        for k in range(40):
            v = s.get('wizards', 1, 'spell_library[%d][%d]' % (r, k))
            if v in (N('SPELLSTATE_RESEARCHABLE'), N('SPELLSTATE_OFFERED')):
                s.set('wizards', 1, 'spell_library[%d][%d]' % (r, k), N('SPELLSTATE_KNOWN'))
                n += 1
    for f, v in (('research_percentage', 40), ('skill_percentage', 30), ('mana_percentage', 30), ('ai_magic_timer', 1),
                 ('research_spell', 0)):
        s.set('wizards', 1, f, v)
    lv = mom.boot(s)
    t = mom.end_turn(lv, log=log)
    p = {f: lv.get('wizards', 1, f) for f in ('research_percentage', 'skill_percentage', 'mana_percentage',
                                             'ai_magic_timer')}
    mom.shot('after', log)
    obs = 'turn %d: wizard 1 (%d spells made known, nothing to research): %s (was 40 / 30 / 30, timer 1)' % (t, n, p)
    if p['ai_magic_timer'] < 15:
        return dict(shown=None, observed=obs + ' - the re-split did not run')
    return dict(shown=p['skill_percentage'] == 20 and p['research_percentage'] == 0, observed=obs)


@case(353, 'OV', 'a Miners\' Guild DOUBLES the iron / coal unit-cost cut instead of adding 50 %',
      setup='Cornwell (3, Halflings, producing unit type 94) given a Miners\' Guild (34, built) and iron ore (mineral '
            '1) on the area square (15,29); during the turn change, the Tile_Mineral_Unit_Cost_Pct(15, 29, 0, guild 1, '
            'dwarf 0) call City_Production_Cost makes: its result', bug='10 (5 doubled)', fixed='7 or 8 (5 + 50 %)')
def c353(log):
    s = mom.SaveFile()
    W = N('MAP_WIDTH')
    s.set('cities', 3, 'buildings[%d]' % N('BLD_MINERS_GUILD'), N('BLDSTATE_BUILT'))
    s.set('minerals', 29 * W + 15, value=N('MINERAL_IRON_ORE'))
    d = dbg.boot(s, log=log)
    try:
        args, r = d.call('Tile_Mineral_Unit_Cost_Pct', lambda a: tuple(a) == (15, 29, 0, 1, 0), nargs=5, timeout=200,
                         poll=d.end_turn_poll())
        d.go()
        mom.shot('after', log)
    finally:
        d.stop()
    if args is None:
        return dict(shown=None, observed='no Tile_Mineral_Unit_Cost_Pct(15, 29, 0, 1, 0) call (Cornwell\'s unit cost)')
    obs = 'Tile_Mineral_Unit_Cost_Pct(15, 29, plane 0, guild 1, dwarf 0) on iron ore = %d (5 without the guild)' % r
    return dict(shown=r == 10, observed=obs)


@case(472, 'OV', 'a hero killed by Death Wish gets its item slots cleared to 0 (an item index), not -1',
      setup='the human has finished Death Wish (every enemy non-fantastic unit resists at 0 or dies); the AI wizards\' '
            'hired heroes (10, all slots empty = -1 in the save) read before and after the cast at the turn start',
      bug='a killed hero\'s three slots read 0 (item record 0), not -1', fixed='-1 (empty)')
def c472(log):
    s = mom.SaveFile()
    _human_cast(s, N('SPELL_DEATH_WISH'))
    lv = mom.boot(s)

    def heroes():
        out = {}
        for w in (1, 2, 3, 4):
            for h in range(6):
                u = lv.get('wizards', w, 'hired_heroes[%d].unit_index' % h)
                if u > -1:
                    out[(w, h)] = [lv.get('wizards', w, 'hired_heroes[%d].item_index[%d]' % (h, k)) for k in range(3)]
        return out
    before = heroes()
    t = mom.end_turn(lv, log=log)
    time.sleep(2)
    after = {}
    for (w, h) in before:
        after[(w, h)] = [lv.get('wizards', w, 'hired_heroes[%d].item_index[%d]' % (h, k)) for k in range(3)]
    zeroed = [k for k, v in after.items() if v == [0, 0, 0]]
    cs = lv.get('wizards', 0, 'casting_spell')
    mom.shot('after', log)
    obs = ('turn %d: AI heroes (wizard, slot) items before %s -> after %s; cleared to 0: %s; human casting now %d' % (
        t, before, after, zeroed, cs))
    if cs == N('SPELL_DEATH_WISH'):
        return dict(shown=None, observed=obs + ' - not cast')
    if not zeroed and all(v == [-1, -1, -1] for v in after.values()):
        return dict(shown=None, observed=obs + ' - no hero died (resist rolls)')
    return dict(shown=bool(zeroed), observed=obs)
