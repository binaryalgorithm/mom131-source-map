"""P4 B4 combat cases, batch 5 (N246): human spells in a battle next to Dracon (cases_cb2._spell_battle) with the
wizard's / units' records read before and after."""
import time

import mom
from regress import case
from cases_cb2 import _spell_battle

N = mom.N


def _alone(s, att):
    """Every other human unit finished too: setup_attack's Patrol alone leaves Dracon's whole stack selected, and the
    stack attacks with the unit under test (#228's first run: a thrower was the active unit, 10/24)."""
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and u != att:
            s.set('units', u, 'finished', 1)


def _approach_click(d, a, t, mode='beside'):
    """Click for battle unit a against battle unit t: the target itself when adjacent, else (mode 'beside') the
    square beside it on a's side (a click on a target out of reach does nothing, and the neutrals stay put);
    mode 'step': one square from a toward t (a walker with 1 move: the square beside the target is out of its
    reach - a red X, #236 10/24); mode 'shoot': the target itself (a shooter fires from where it stands)."""
    gx, gy = d.lv.get('battle_units', t, 'combat_x'), d.lv.get('battle_units', t, 'combat_y')
    ux, uy = d.lv.get('battle_units', a, 'combat_x'), d.lv.get('battle_units', a, 'combat_y')
    if mode != 'shoot' and max(abs(gx - ux), abs(gy - uy)) > 1:
        if mode == 'step':
            gx, gy = ux + (gx > ux) - (gx < ux), uy + (gy > uy) - (gy < uy)
        else:
            gx += (ux > gx) - (ux < gx)
            gy += (uy > gy) - (uy < gy)
    sx, sy = mom.grid_to_screen(gx, gy)
    d.lv.g.click(sx, sy + 8, settle=0.2)


@case(22, 'CB', 'Life Drain cast by a wizard adds 3 casting SKILL points per drained point',
      setup='neutral unit 2 made Barbarian Bowmen (41: resistance 5), level 0; the human casts Life Drain (167) on '
            'it; the human\'s skill_points and the target\'s drain damage (damage[1]) read before and after',
      bug='skill_points grow by 3 x the drained points (Life_Drain_Apply, SPELLFX.C)',
      fixed='(intended?) no skill gain - mana or nothing')
def c22(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_LIFE_DRAIN')], log, edits=[(2, 'type', 41), (2, 'level', 0),
                                                                    (2, 'experience', 0)])
    t = idx[2]
    sp0, dr0 = lv.get('wizards', 0, 'skill_points'), lv.get('battle_units', t, 'damage[1]')
    mom.cast_spell(lv, N('SPELL_LIFE_DRAIN'), target=t, log=log)
    time.sleep(2)
    sp1, dr1 = lv.get('wizards', 0, 'skill_points'), lv.get('battle_units', t, 'damage[1]')
    mom.finish_combat(lv, log=log)
    obs = ('skill_points %d -> %d (+%d); the target\'s drain damage %d -> %d (+%d)'
           % (sp0, sp1, sp1 - sp0, dr0, dr1, dr1 - dr0))
    if dr1 == dr0:
        return dict(shown=None, observed=obs + ' - nothing drained (the target resisted)')
    return dict(shown=sp1 - sp0 == 3 * (dr1 - dr0), observed=obs)


def _can_attack(enchant, log, tag):
    """Dracon's first unit made Beastmen Halberdiers (48: a walker, no ranged attack) against neutral unit 2 (a
    walker) given `enchant` in its unit enchantments;
    in the human's first combat turn the target's square is clicked; Can_Attack_Target(unit, target 2) run to its
    return. -> (args, AX) or (None, None)."""
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = mom.setup_attack(s, att, [2, 13], (30, 28, 0))
    _alone(s, att)
    for f, v in (('type', 48), ('hero_slot', -1), ('level', 0)):   # Beastmen Halberdiers: melee only (a ranged
        s.set('units', att, f, v)                                  # attacker may hit an airborne target)
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    s.set('units', 2, 'enchantments', enchant)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        t = idx[2]
        flags = d.lv.get('battle_units', t, 'move_flags')
        gx, gy = d.lv.get('battle_units', t, 'combat_x'), d.lv.get('battle_units', t, 'combat_y')
        sx, sy = mom.grid_to_screen(gx, gy)
        d.want('Can_Attack_Target', when=lambda h: d.arg(1, h) == t)
        d.lv.g.click(sx, sy + 8, settle=0.2)
        h = d.wait(30, None)
        if h is None:
            log.append('%s: Can_Attack_Target(-, %d) not called' % (tag, t))
            return None, None, flags
        args = (d.arg(0, h), d.arg(1, h))
        ax = d.ret_ax()
        log.append('%s: Can_Attack_Target%s = %d (target move_flags 0x%x)' % (tag, args, ax, flags))
        return args, ax, flags
    finally:
        d.stop()


@case(228, 'CB', 'a Wind Walking unit cannot be attacked in melee by a non-flyer (counts as airborne in combat)',
      setup='neutral unit 2 (type 47, a walker) given the Wind Walking unit enchantment (0x10000); Dracon\'s first '
            'unit made Beastmen Halberdiers (48: a walker, no ranged attack) attacks it and unit 13; in the first '
            'human combat turn unit 2\'s '
            'square is clicked; Can_Attack_Target(attacker, unit 2) run to its return. Control: no enchantment',
      bug='0 with Wind Walking (Unit_Is_Airborne counts the overland movement spell as flight), 1 without',
      fixed='1 in both')
def c228(log):
    a1, r1, f1 = _can_attack(N('UNIT_ENCH_WIND_WALKING'), log, 'Wind Walking')
    a0, r0, f0 = _can_attack(0, log, 'control')
    obs = ('Can_Attack_Target(attacker, target) with the target Wind Walking: %s = %s (move_flags 0x%x); without: '
           '%s = %s' % (a1, r1, f1, a0, r0))
    if r1 is None or r0 is None:
        return dict(shown=None, observed=obs + ' - a call was not caught')
    if r0 != 1:
        return dict(shown=None, observed=obs + ' - the control target cannot be attacked either')
    return dict(shown=r1 == 0, observed=obs)


def _shields(att_type, target_type, log, tag, rtype=None):
    """Dracon's first unit made att_type (level 0) shoots neutral unit 2 made target_type: in the human's first
    combat turn unit 2's square is clicked; Defender_Shields(unit 2, ...) - with ranged_type rtype if given - run to
    its return. -> (args, AX)."""
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = mom.setup_attack(s, att, [2, 13], (30, 28, 0))
    _alone(s, att)
    for f, v in (('type', att_type), ('hero_slot', -1), ('level', 0)):
        s.set('units', att, f, v)
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    s.set('units', 2, 'type', target_type)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        t = idx[2]
        ab = d.lv.get('battle_units', t, 'abilities')
        d.want('Defender_Shields', when=lambda h: d.arg(0, h) == t and (rtype is None or d.arg(1, h) == rtype))
        h = None
        a = idx[att]
        for turn in range(6):                   # a walker may need several combat turns to reach the target
            _approach_click(d, a, t)
            h = d.wait(15, None)
            if h is not None or not mom.in_combat(d.lv) and d.lv.glob('battle_unit_count') == 0:
                break
            d.lv.D.key('d')                     # Done: the next combat turn
            h = d.wait(20, None)
            if h is not None:
                break
            for _ in range(10):
                if mom.in_combat(d.lv):
                    break
                time.sleep(1)
        if h is None:
            log.append('%s: Defender_Shields(%d, ...) not called' % (tag, t))
            return None, None, ab
        args = tuple(d.arg(k, h) for k in range(5))
        ax = d.ret_ax()
        log.append('%s: Defender_Shields%s = %d' % (tag, args, ax))
        return args, ax, ab
    finally:
        d.stop()


@case(215, 'CB', 'Weapon Immunity overrides Missiles Immunity: a unit with both defends a normal arrow at 10, not 50',
      setup='Dracon\'s first unit made Barbarian Bowmen (41, a normal bow) shoots neutral unit 2 made a Demon (169: '
            'Missiles AND Weapon Immunity, defence 5); Defender_Shields(unit 2, ...) run to its return; its ability '
            'bits read (no control run: a Missiles-Immune-only unit cannot even be targeted by a bow - the cursor '
            'shows the X, 10/24)',
      bug='10 for the Demon (the Weapon Immunity level overwrites the total immunity of 50)', fixed='50')
def c215(log):
    a1, r1, ab = _shields(41, 169, log, 'Demon')
    both = N('ABILITY_MISSILES_IMMUNITY') | N('ABILITY_WEAPON_IMMUNITY')
    obs = ('Defender_Shields(defender, ranged_type, damage_flags, attack_flags, realm) for the Demon (abilities '
           '0x%x: Missiles Immunity 0x4, Weapon Immunity 0x100) %s = %s' % (ab, a1, r1))
    if r1 is None:
        return dict(shown=None, observed=obs + ' - the shot was not caught')
    if ab & both != both:
        return dict(shown=None, observed=obs + ' - the target lacks one of the immunities')
    return dict(shown=r1 < 50, observed=obs)


def _damage_flags(att_type, kind, log, tag):
    """Dracon's first unit made att_type (level 0) attacks neutral unit 2 made Werewolves (168) - a click on it (a
    walker walks up over several combat turns); Attack_Damage_Flags(that unit, kind) run to its return. -> (its
    ranged_type, weapon_icon, the flags) or None."""
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = mom.setup_attack(s, att, [2, 13], (30, 28, 0))
    _alone(s, att)
    for f, v in (('type', att_type), ('hero_slot', -1), ('level', 0)):
        s.set('units', att, f, v)
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    s.set('units', 2, 'type', 168)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        a, tg = idx[att], idx[2]
        rt, wi = d.lv.get('battle_units', a, 'ranged_type'), d.lv.get('battle_units', a, 'weapon_icon')
        d.want('Attack_Damage_Flags', when=lambda h: d.arg(0, h) == a and d.arg(1, h) == kind)
        h = None
        mode = 'shoot' if kind == N('ATTACK_KIND_MISSILE') else 'step'
        for turn in range(8):                   # a walker may need several combat turns to reach the target
            _approach_click(d, a, tg, mode)
            h = d.wait(15, None)
            if h is not None:
                break
            if turn == 0:
                mom.shot('%s_click' % tag.split(',')[-1].strip(), log)
            d.lv.D.key('d')                     # Done: the next combat turn
            h = d.wait(20, None)
            if h is not None:
                break
            for _ in range(10):
                if mom.in_combat(d.lv):
                    break
                time.sleep(1)
        if h is None:
            log.append('%s: Attack_Damage_Flags(%d, %d) not called' % (tag, a, kind))
            return None
        ax = d.ret_ax()
        log.append('%s: ranged_type %d, weapon_icon %d; Attack_Damage_Flags(%d, %d) = 0x%x' % (tag, rt, wi, a, kind, ax))
        return rt, wi, ax
    finally:
        d.stop()


@case(236, 'CB', 'thrown attacks ignore Weapon Immunity (ranged_type / 10 == 100 can never be true)',
      setup='Dracon\'s first unit made Barbarian Spearmen (39: thrown, ranged_type 100) attacks neutral unit 2 made '
            'Werewolves (168: Weapon Immunity) - the click walks it in and attacks; Attack_Damage_Flags(attacker, '
            'ATTACK_KIND_THROWN 1) run to its return (the flags Defender_Shields matches against the abilities). '
            'Control: Barbarian Bowmen (41) shooting (ATTACK_KIND_MISSILE 2)',
      bug='the thrown attack\'s flags lack ABILITY_WEAPON_IMMUNITY (0x100): not counted as a normal weapon; the '
          'arrow\'s have it', fixed='0x100 in both')
def c236(log):
    th = _damage_flags(39, N('ATTACK_KIND_THROWN'), log, 'thrown')
    ar = _damage_flags(41, N('ATTACK_KIND_MISSILE'), log, 'control, arrow')
    w = N('ABILITY_WEAPON_IMMUNITY')
    obs = ('(ranged_type, weapon_icon, Attack_Damage_Flags) thrown %s; arrow %s (Weapon Immunity bit 0x%x)'
           % (th, ar, w))
    if th is None or ar is None:
        return dict(shown=None, observed=obs + ' - an attack was not caught')
    if th[0] != N('RANGED_THROWN') or th[1] != ar[1]:
        return dict(shown=None, observed=obs + ' - not a thrown attack, or the weapons differ')
    if not ar[2] & w:
        return dict(shown=None, observed=obs + ' - the control arrow is not an ordinary weapon')
    return dict(shown=not th[2] & w, observed=obs)


HERO = 19                                       # the human's hero in slot 0 (type 19, level 4, no items)


def _shot_at_hero(item_powers, unit_ench, log, tag):
    """Hero 19 alone, holding item 1 (its powers = item_powers; its unit enchantments = unit_ench), attacks neutral
    unit 2 made
    Dark Elf Warlocks (64: fire magic ranged - Chaos) with unit 13; the human presses Done (up to three combat
    turns); in the neutral turn Defender_Shields(the hero, RANGED_MAGIC_FIRE, ...) run to its return. -> (args,
    AX)."""
    import dbg
    s = mom.SaveFile()
    key = mom.setup_attack(s, HERO, [2, 13], (30, 28, 0))
    _alone(s, HERO)
    s.set('wizards', 0, 'hired_heroes[2].item_index[0]', -1)    # item 1 moved from the Sage (69) to hero 19: the
    s.set('wizards', 0, 'hired_heroes[0].item_index[0]', 1)     # Sage alone never got a combat turn (10/24)
    s.set('items', 1, 'powers', item_powers)
    s.set('units', HERO, 'enchantments', unit_ench)
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    s.set('units', 2, 'type', 64)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        hb = idx[HERO]
        ie = d.lv.get('battle_units', hb, 'item_enchantments') & 0xffffffff
        d.want('Defender_Shields', when=lambda h: d.arg(0, h) == hb and d.arg(1, h) == N('RANGED_MAGIC_FIRE'))
        d.lv.D.key('d')                         # Done: the neutral side's turn
        h = None
        for turn in range(3):                   # the Warlocks shoot in their turn (Shadow Demons closed to melee)
            h = d.wait(45, None)
            if h is not None:
                break
            for _ in range(15):
                if mom.in_combat(d.lv):
                    d.lv.D.key('d')
                    break
                time.sleep(1)
        if h is None:
            log.append('%s: no Chaos (fire) shot at the hero' % tag)
            return None, None, ie
        args = tuple(d.arg(k, h) for k in range(5))
        ax = d.ret_ax()
        log.append('%s: hero item_enchantments 0x%08x; Defender_Shields%s = %d' % (tag, ie, args, ax))
        return args, ax, ie
    finally:
        d.stop()


@case(189, 'CB', 'a Righteousness ITEM does not make its hero immune to Chaos / Death ranged attacks',
      setup='the human\'s hero 19 given item 1 (moved from hero 69) with the Righteousness item power (powers '
            '0x40000000); it attacks alone neutral unit 2 made Dark Elf Warlocks (64: fire magic ranged, Chaos) and '
            'unit 13; Done; in the neutral turn Defender_Shields(hero, RANGED_MAGIC_FIRE, ...) run to its return. '
            'Control: no item power, Righteousness cast on the hero (unit enchantment 0x40000000)',
      bug='the item gives the hero its plain shields; the cast Righteousness gives 50', fixed='50 for both')
def c189(log):
    bit = N('ITEM_POWER_RIGHTEOUSNESS')
    a1, r1, e1 = _shot_at_hero(bit, 0, log, 'item')
    a0, r0, e0 = _shot_at_hero(0, N('UNIT_ENCH_RIGHTEOUSNESS'), log, 'control, cast')
    obs = ('Defender_Shields vs a fire (Chaos) shot: with the item (hero item_enchantments 0x%08x) %s = %s; with the '
           'cast %s = %s' % (e1, a1, r1, a0, r0))
    if r1 is None or r0 is None:
        return dict(shown=None, observed=obs + ' - a shot was not caught')
    if not e1 & bit:
        return dict(shown=None, observed=obs + ' - the item power did not reach the battle record')
    if r0 != 50:
        return dict(shown=None, observed=obs + ' - the cast Righteousness gives no immunity either')
    return dict(shown=r1 < 50, observed=obs)


@case(257, 'CB', 'Terror makes Death-Immune units roll to resist (and lose their turn) - no immunity test',
      setup='neutral unit 2 made Skeletons (165: Death Immunity), level 0; the human casts Terror (the attacker\'s '
            'combat enchantment) and presses Done; breakpoint in Combat_Turn_Start on the Terror roll '
            '"r = Roll_Resist(p_battle_units[bu], ..." for the Skeletons\' battle record (bu = SI)',
      bug='the roll is made for the Death-Immune unit (a failed roll freezes it: moves 0)',
      fixed='Death-Immune units skipped')
def c257(log):
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    mom.learn_spells(s, 0, [N('SPELL_TERROR')])
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    for f, v in (('type', N('UNIT_SKELETONS')), ('level', 0), ('experience', 0)):
        s.set('units', 2, f, v)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        t = idx[2]
        ab = d.lv.get('battle_units', t, 'abilities')
        mom.cast_spell(d.lv, N('SPELL_TERROR'), log=log)
        ench = d.lv.g.ram(d.lv.g.far(mom.SYM['p_combat_enchants'][0]), 40)
        d.want_line('src/wizards/COMBMOVE.C', 'r = Roll_Resist(p_battle_units[bu], p_aura_resistance',
                    fn='Combat_Turn_Start', label='terror', when=lambda h: h['SI'] == t)
        k = [0]

        def poll():
            k[0] += 1
            if k[0] % 3 == 0:
                d.lv.D.key('d')                 # Done for the human's units: on to the next combat turn
        h = d.wait_via_entry('Combat_Turn_Start', 120, poll)   # it calls other overlays before the roll
    finally:
        d.stop()
    obs = ('Skeletons (battle unit %d) abilities 0x%x (Death Immunity 0x%x); Terror after the cast: attacker\'s %d, '
           'defender\'s %d (combat enchantments %d / %d); the Terror roll %s for it'
           % (t, ab, N('ABILITY_DEATH_IMMUNITY'), ench[N('CENCH_TERROR_ATT')], ench[N('CENCH_TERROR_DEF')],
              N('CENCH_TERROR_ATT'), N('CENCH_TERROR_DEF'), 'made' if h is not None else 'not seen'))
    if not ab & N('ABILITY_DEATH_IMMUNITY'):
        return dict(shown=None, observed=obs + ' - the unit is not Death-Immune')
    if ench[N('CENCH_TERROR_ATT')] == 0:
        return dict(shown=None, observed=obs + ' - Terror was not cast')
    if h is None:
        return dict(shown=None, observed=obs + ' - no Terror roll reached')
    return dict(shown=True, observed=obs)


def _neutral_battle(s, spells, log):
    """Save edits + boot for a debugger battle: the human knows `spells`, Dracon's first unit attacks neutral units 2
    and 13 (13 made a plain unit, type 104) at (30,28). -> (d, idx by unit, key already pressed)."""
    import dbg
    import cases_cb as CB
    mom.learn_spells(s, 0, spells)
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    d = dbg.boot(s, log=log)
    d.start_combat(key)
    idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
    return d, idx


def _done_poll(d):
    k = [0]

    def poll():
        k[0] += 1
        if k[0] % 3 == 0:
            d.lv.D.key('d')                     # Done for the human's units: on to the next combat turn
    return poll


@case(478, 'CB', 'Call Lightning bolts roll their damage from spell 3\'s record (Wall of Stone)',
      setup='the human casts Call Lightning (combat enchantment) and presses Done; at the next turn start '
            'Call_Lightning_Turn strikes: breakpoint on Spell_Damage_Roll(spell, target, damage, strength): its '
            'arguments read; p_spell_data[3] and [Call Lightning] effect_param (the strength) compared',
      bug='Spell_Damage_Roll(3, ...): Wall of Stone\'s record decides the bolt', fixed='Call Lightning\'s own record')
def c478(log):
    s = mom.SaveFile()
    d, idx = _neutral_battle(s, [N('SPELL_CALL_LIGHTNING')], log)
    try:
        mom.cast_spell(d.lv, N('SPELL_CALL_LIGHTNING'), log=log)
        p3 = (d.lv.get('spells', 3, 'effect_param'), d.lv.get('spells', 3, 'effect_type'))
        pc = (d.lv.get('spells', N('SPELL_CALL_LIGHTNING'), 'effect_param'),
              d.lv.get('spells', N('SPELL_CALL_LIGHTNING'), 'effect_type'))
        d.want('Spell_Damage_Roll')
        h = d.wait(90, _done_poll(d))
        args = tuple(d.arg(k, h) for k in range(4)) if h is not None else None
        caller = d.where(d.word(h['SS'], h['SP'] + 2, False), d.word(h['SS'], h['SP'], False)) if h else None
    finally:
        d.stop()
    obs = ('Spell_Damage_Roll(spell, target, damage, strength) %s called from %s; spell 3 (effect_param, type) %s, '
           'Call Lightning %s' % (args, caller, p3, pc))
    if args is None:
        return dict(shown=None, observed=obs + ' - no bolt reached')
    return dict(shown=args[0] == 3 and 'Call_Lightning' in str(caller), observed=obs)


@case(92, 'CB', 'a Heroism cast in combat does not raise the level hero abilities (Might ...) scale with',
      setup='the human\'s hero 19 (type 19) at level 0 / experience 0, its type given Might (hero_stats abilities | '
            '0x8000); it attacks neutral units 2 and 13; the human casts Heroism (130) on it in combat; breakpoint in '
            'Hero_Ability_Bonus on the Might line for unit 19: Unit.level and the battle record\'s combat '
            'enchantments read',
      bug='Might adds level 0 + 1 while the battle record carries Heroism (combat Heroism only counts as level 3 in '
          'Unit_Exp)', fixed='Might scales with the Heroism level (3) as for an overland Heroism')
def c92(log):
    s = mom.SaveFile()
    s.set('hero_stats', 19, 'abilities', s.get('hero_stats', 19, 'abilities') | N('HERO_ABILITY_MIGHT'))
    s.set('units', 19, 'level', 0)
    s.set('units', 19, 'experience', 0)
    d, idx = _neutral_battle(s, [N('SPELL_HEROISM')], log)
    try:
        hb = idx[19]
        m0 = d.lv.get('battle_units', hb, 'melee')
        mom.cast_spell(d.lv, N('SPELL_HEROISM'), target=hb, log=log)
        time.sleep(1)
        ce = d.lv.get('battle_units', hb, 'combat_enchantments') & 0xffffffff
        m1 = d.lv.get('battle_units', hb, 'melee')
        d.want_line('src/wizards/UNITSTAT.C', 'bu->melee = p_units[unit].level + bu->melee + 1;',
                    fn='Hero_Ability_Bonus', label='might', when=lambda h: d.word(h['SS'], h['BP'] + 6) == 19)
        h = d.wait(90, _done_poll(d))
        level = d.lv.get('units', 19, 'level')
        ce2 = d.lv.get('battle_units', hb, 'combat_enchantments') & 0xffffffff
    finally:
        d.stop()
    obs = ('hero 19 melee %d before the cast, %d after; battle combat_enchantments 0x%08x (Heroism 0x%08x); at the '
           'Might line (%s): Unit.level %d, combat_enchantments 0x%08x'
           % (m0, m1, ce, N('UNIT_ENCH_HEROISM'), 'reached' if h else 'not reached', level, ce2))
    if not ce & N('UNIT_ENCH_HEROISM'):
        return dict(shown=None, observed=obs + ' - Heroism did not land')
    if h is None:
        return dict(shown=None, observed=obs + ' - Hero_Ability_Bonus\'s Might line not reached')
    return dict(shown=level == 0 and bool(ce2 & N('UNIT_ENCH_HEROISM')), observed=obs)


def _melee_rolls(def_type, log, tag):
    """Dracon's first unit made Wolf Riders (88: melee 7, 3 moves, no ranged attack) attacks neutral unit 2 made
    def_type (unit 13 a plain unit far behind), walking up over several combat turns; tracepoints on Roll_To_Hit(points, to_hit) entries -
    the caller Attack_Resolve's attacker DI, defender SI, attack kind [bp+12], counter [bp+16], block_bonus [bp-30]
    with the to_hit argument - and on Roll_Blocks(shields, to_block). -> (block_bonus of unit 2, the Wolf Riders'
    own melee attacks on unit 2 (to_hit, block_bonus), Roll_Blocks calls)."""
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = mom.setup_attack(s, att, [2, 13], (30, 28, 0))
    _alone(s, att)
    for f, v in (('type', 88), ('hero_slot', -1), ('level', 0), ('experience', 0)):
        s.set('units', att, f, v)
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    for f, v in (('type', def_type), ('level', 0), ('experience', 0)):
        s.set('units', 2, f, v)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        t, a = idx[2], idx[att]
        bb = d.lv.get('battle_units', t, 'block_bonus')
        w = lambda h, k: d.word(h['SS'], (h['BP'] + k) & 0xffff)
        # Roll_To_Hit's ENTRY (10/24: the line in Attack_Resolve was missed - overlays move): the callee has not
        # pushed BP yet, so DI / SI / [bp+n] / [bp-n] are still Attack_Resolve's
        d.want('Roll_To_Hit', label='hit', trace=lambda h: (h['DI'], h['SI'], w(h, 12), w(h, 16), d.arg(1, h),
                                                            w(h, -30)))
        d.want('Roll_Blocks', trace=lambda h: (d.arg(0, h), d.arg(1, h)))

        def own():                              # the Wolf Riders' melee strikes on unit 2: attacks (counter 0) and
            return [(ctr, th, blk) for at, de, kind, ctr, th, blk in d.traces.get('hit', [])   # counter strikes (1)
                    if at == a and de == t and kind == 0]
        for turn in range(5):                   # the Wolf Riders may need two combat turns to reach the target
            _approach_click(d, a, t)
            d.wait(12, None)
            if own():
                break
            d.lv.D.key('d')
            d.wait(15, None)
            if own():
                break
        hits, blocks = own(), list(d.traces.get('Roll_Blocks', []))
        allhits = list(d.traces.get('hit', []))
    finally:
        d.stop()
    log.append('%s: unit 2 block_bonus %d; Attack_Resolve hit lines (attacker, defender, kind, counter, to_hit, '
               'block) %s; Roll_Blocks %s' % (tag, bb, allhits[:12], blocks[:8]))
    return bb, hits, blocks


@case(187, 'CB', 'a melee defender\'s block bonus (Lucky, Prayer ...) counts twice: off the to-hit AND as block chance',
      setup='Dracon\'s first unit made Wolf Riders (88: melee 7) attacks neutral unit 2 made Halfling Spearmen (89: '
            'Lucky, block_bonus 1); tracepoints on Roll_To_Hit(points, to_hit) entries (Attack_Resolve\'s '
            'attacker, counter, to_hit) and on Roll_Blocks(shields, to_block): the Wolf Riders\' own attacks (not counter strikes). '
            'Control: Beastmen Spearmen (46: no Lucky)',
      bug='the attacker\'s to-hit is 1 lower against the Lucky unit AND its blocks roll with to_block 1',
      fixed='one of the two (IW081)')
def c187(log):
    bl, hl, kl = _melee_rolls(89, log, 'Lucky')
    bc, hc, kc = _melee_rolls(46, log, 'control')
    # compare like with like: the first strikes of one kind (an attack, else a counter strike - its to_hit also
    # loses times_attacked / 2) present in both runs
    kinds = [c for c in (0, 1) if any(h[0] == c for h in hl) and any(h[0] == c for h in hc)]
    th_l = [h[1] for h in hl if kinds and h[0] == kinds[0]]
    th_c = [h[1] for h in hc if kinds and h[0] == kinds[0]]
    tb_l, tb_c = sorted({tb for _, tb in kl}), sorted({tb for _, tb in kc})
    obs = ('the Wolf Riders\' melee strikes on unit 2 (counter flag, to_hit): on the Lucky defender (block_bonus %d) '
           '%s, Roll_Blocks to_block %s; on the control (block_bonus %d) %s, to_block %s; compared: the first %s'
           % (bl, hl[:6], tb_l, bc, hc[:6], tb_c, ('attacks', 'counter strikes')[kinds[0]] if kinds else 'none'))
    if not kinds:
        return dict(shown=None, observed=obs + ' - no strike of one kind in both runs')
    if bl < 1 or bc != 0:
        return dict(shown=None, observed=obs + ' - block bonuses not as set up')
    return dict(shown=th_l[0] == th_c[0] - bl and bl in tb_l, observed=obs)


@case(219, 'CB', 'Dispel Magic never tries the last enchantment bit (31, Invulnerability)',
      setup='neutral unit 2 given Invulnerability (bit 31) and Water Walking (bit 8) as unit enchantments; the human '
            'casts Dispel Magic (202) on it; tracepoint in Dispel_Square on the unit-enchantment roll line '
            '("chance = Spell_Dispel_Resistance(p_spell_data[p_dispel_ench_spells[bit]].base_cost * 5,"): bit '
            '[bp-6] per roll; the unit\'s enchantments read after',
      bug='bit 8 rolled, bit 31 never (the loop runs bit < 31): Invulnerability cannot be dispelled',
      fixed='bit 31 rolled too')
def c219(log):
    s = mom.SaveFile()
    ench = N('UNIT_ENCH_INVULNERABILITY') | N('UNIT_ENCH_WATER_WALKING')
    s.set('units', 2, 'enchantments', ench - (1 << 32) if ench >= 1 << 31 else ench)
    d, idx = _neutral_battle(s, [N('SPELL_DISPEL_MAGIC')], log)
    try:
        t = idx[2]
        e0 = d.lv.get('units', 2, 'enchantments') & 0xffffffff
        d.want_line('src/wizards/COMBDISP.C',
                    'chance = Spell_Dispel_Resistance(p_spell_data[p_dispel_ench_spells[bit]].base_cost * 5,',
                    fn='Dispel_Square', label='roll', trace=lambda h: d.word(h['SS'], h['BP'] - 6))
        mom.cast_spell(d.lv, N('SPELL_DISPEL_MAGIC'), target=t, log=log)
        d.wait(8, None)
        bits = list(d.traces.get('roll', []))
        e1 = d.lv.get('units', 2, 'enchantments') & 0xffffffff
    finally:
        d.stop()
    obs = 'unit 2 enchantments 0x%08x before, 0x%08x after; bits rolled %s' % (e0, e1, bits)
    if not e0 & 0x80000000 or 8 not in bits:
        return dict(shown=None, observed=obs + ' - the dispel roll loop was not seen')
    return dict(shown=31 not in bits, observed=obs)


@case(196, 'CB', 'the relation penalty for killing a wizard\'s units counts only the LAST battle unit\'s share',
      setup='wizard 1\'s heroes 13 and 50 moved into its city 21 Rastenford, placed at (30,28) on Arcanus (20 '
            'relation points each when killed; no fleeing from a city); '
            'Dracon\'s stack attacks them, auto combat; tracepoint on Diplo_Relation_Change(amount, actor, observer, '
            'msg 8 = unprovoked destruction): its arguments and the battle units\' statuses at that call',
      bug='-20 with both heroes dead (relation_hit is reset inside the per-unit loop)', fixed='-40')
def c196(log):
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):    # in a city the heroes cannot flee (10/24: in the open
        s.set('cities', 21, f, v)                           # field the human never got a combat turn)
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [13, 50], (30, 28, 0))
    msg = N('DIPLO_MSG_UNPROVOKED_DESTRUCTION')
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        fields = ('unit_index', 'owner', 'status')
        d.want('Diplo_Relation_Change', when=lambda h: d.arg(3, h) == msg)
        k = [0]

        def poll():
            k[0] += 1
            if mom.in_combat(d.lv):
                d.lv.D.key('a')                 # auto combat for the human side
        h = d.wait(240, poll)
        if h is None:
            return dict(shown=None, observed='no unprovoked-destruction relation change after the battle')
        args = tuple(d.arg(i, h) for i in range(4))
        bus = mom.battle_units(d.lv, fields)
    finally:
        d.stop()
    heroes = [b for b in bus if b['unit_index'] in (13, 50)]
    dead = [b for b in heroes if b['status'] > 0]
    obs = ('Diplo_Relation_Change(amount, actor, observer, msg) %s; the heroes\' battle records %s; last battle unit '
           '%s' % (args, heroes, bus[-1] if bus else None))
    if len(dead) != 2:
        return dict(shown=None, observed=obs + ' - not both heroes killed')
    return dict(shown=args[0] == -20, observed=obs)


def _city_walls_flag(d):
    import struct
    off, seg = struct.unpack('<HH', d.lv.g.ram(d.lv.g.ds + mom.SYM['p_battlefield'][0], 4))
    fo = mom.resolve('Battlefield', 'has_city_walls')[0]
    return struct.unpack('<h', d.dump(seg, off + fo, 2))[0]


def _end_battle(lv, log, d=None):
    """Auto combat, then the results; a 'destroy this city?' question answered N (keep). In a debugger session pass
    d: its wait() resumes the unknown stops an AI wizard's battle produces (dbg.start_combat)."""
    t0 = time.time()
    while time.time() - t0 < 240:
        if mom.on_map(lv):
            return True
        keys = {c[4].upper() for c in mom.controls(lv)}
        if mom.in_combat(lv):
            lv.D.key('a')
        elif 'R' in keys and 'N' in keys:
            lv.D.key('n')
        elif not mom.items_award(lv):
            lv.D.key('enter')
        if d is not None:
            d.wait(2, None)
        else:
            time.sleep(2)
    return False


@case(326, 'CB', 'city walls carry over into the next battle: has_city_walls is set only for city / fortress fields',
      setup='AI city 21 Rastenford (wizard 1) moved to (30,28) next to Dracon with City Walls, unit 2 (wizard 1) '
            'defending it; hero 19 attacks it (battle 1, auto, the city kept); then hero 33 attacks neutral unit 13 '
            'at (28,28) in the open (battle 2); Battlefield.has_city_walls read (through the debugger) in each battle',
      bug='1 in the open-field battle 2 (left from battle 1)', fixed='0')
def c326(log):
    import dbg
    s = mom.SaveFile()
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):
        s.set('cities', 21, f, v)
    s.set('cities', 21, 'buildings[%d]' % N('BLD_CITY_WALLS'), N('BLDSTATE_BUILT'))
    key1 = mom.setup_attack(s, 19, [2], (30, 28, 0))
    s.set('units', 2, 'owner', 1)
    _alone(s, 19)
    for f, v in (('order', N('ORDER_READY')), ('finished', 0)):
        s.set('units', 33, f, v)                # the next unit selected after battle 1
    s.set('units', 33, 'moves_left', s.get('units', 33, 'moves_max'))
    for f, v in (('x', 28), ('y', 28), ('plane', 0), ('owner', 5), ('type', 104), ('hero_slot', -1)):
        s.set('units', 13, f, v)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key1)
        w1 = _city_walls_flag(d)
        _end_battle(d.lv, log, d)
        mom.shot('after_battle1', log)
        time.sleep(2)
        sel = [d.lv.get('units', u, 'x') for u in (19, 33)]
        d.start_combat(mom.KEYPAD[(-1, 1)])
        w2 = _city_walls_flag(d)
        bk = None
        try:
            off = mom.resolve('Battlefield', 'backdrop')[0]
            import struct
            o, sg = struct.unpack('<HH', d.lv.g.ram(d.lv.g.ds + mom.SYM['p_battlefield'][0], 4))
            bk = struct.unpack('<h', d.dump(sg, o + off, 2))[0]
        except Exception as e:
            log.append('backdrop read: %s' % e)
        bus = mom.battle_units(d.lv, ('unit_index', 'owner'))
        _end_battle(d.lv, log, d)
    finally:
        d.stop()
    obs = ('battle 1 (walled city): has_city_walls %d; battle 2 (units %s, backdrop %s): has_city_walls %d'
           % (w1, [b['unit_index'] for b in bus], bk, w2))
    if w1 != 1 or not any(b['unit_index'] == 13 for b in bus):
        return dict(shown=None, observed=obs + ' - the two battles were not as set up (x of 19 / 33 %s)' % sel)
    return dict(shown=w2 == 1, observed=obs)


def _recall(regen, log, tag):
    """Heroes 19 (item 1 moved to it, powers = regen) and 33 attack neutral Settlers (unit 2, type 44) at (30,28)
    from Dracon; the human casts Recall Hero (205) on hero 19, then auto combat; hero 19's square read after."""
    s = mom.SaveFile()
    key = mom.setup_attack(s, HERO, [2], (30, 28, 0))
    _alone(s, HERO)
    for f, v in (('order', N('ORDER_READY')), ('finished', 0)):
        s.set('units', 33, f, v)
    s.set('units', 33, 'moves_left', s.get('units', 33, 'moves_max'))
    s.set('wizards', 0, 'hired_heroes[2].item_index[0]', -1)
    s.set('wizards', 0, 'hired_heroes[0].item_index[0]', 1)
    s.set('items', 1, 'powers', regen)
    for f, v in (('owner', 5), ('type', 44), ('hero_slot', -1)):
        s.set('units', 2, f, v)
    mom.learn_spells(s, 0, [N('SPELL_RECALL_HERO')])
    s.set('hero_stats', HERO, 'caster_level', 0)   # hero 19 active and a caster: S would ask who casts
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    idx = {lv.get('battle_units', i, 'unit_index'): i for i in range(lv.glob('battle_unit_count'))}
    ie = lv.get('battle_units', idx[HERO], 'item_enchantments') & 0xffffffff
    mom.cast_spell(lv, N('SPELL_RECALL_HERO'), target=idx[HERO], log=log)
    st = lv.get('battle_units', idx[HERO], 'status')
    mom.finish_combat(lv, log=log)
    u = lv.get('wizards', 0, 'hired_heroes[0].unit_index')   # the dead Settlers were compacted out: hero 19 moved
    at = tuple(lv.get('units', u, f) for f in ('x', 'y', 'plane', 'owner'))   # down (Units_Compact fixes this index)
    log.append('%s: hero item_enchantments 0x%08x, status after the cast %d, hero 19 (now unit %d, type %d) after the '
               'battle (x, y, plane, owner) %s' % (tag, ie, st, u, lv.get('units', u, 'type'), at))
    return ie, st, at


@case(216, 'CB', 'Regeneration cancels Recall: a recalled regenerating unit of the winning side stays at the battle',
      setup='heroes 19 and 33 attack neutral Settlers at (30,28) from Dracon; hero 19 holds item 1 (moved from hero '
            '69) with the Regeneration power (0x40); the human casts Recall Hero (205) on hero 19, auto combat wins; '
            'hero 19\'s square read after. Control: item 1 with no powers',
      bug='with Regeneration hero 19 stands at the battle square (30,28); without, it is back at Dracon (29,27)',
      fixed='Dracon in both')
def c216(log):
    e1, s1, a1 = _recall(N('ITEM_POWER_REGENERATION'), log, 'Regeneration')
    e0, s0, a0 = _recall(0, log, 'control')
    obs = ('hero 19 after a Recall Hero (status after the cast %d / %d): with the Regeneration item (0x%08x) at %s; '
           'without at %s' % (s1, s0, e1, a1, a0))
    if s1 != N('BATTLE_UNIT_RECALLED') or s0 != N('BATTLE_UNIT_RECALLED'):
        return dict(shown=None, observed=obs + ' - the recall did not land')
    if a0[:2] != (29, 27):
        return dict(shown=None, observed=obs + ' - the control hero did not go home')
    return dict(shown=a1[:2] == (30, 28), observed=obs)


@case(303, 'CB', 'killing the last enemy does not end an Auto / AI combat turn: the winner is counted before the death',
      setup='hero 19 alone attacks neutral unit 2 alone, made Settlers (44: one weak figure), at (30,28); Auto combat; '
            'breakpoint in Battle_Apply_Damage on the line after "combat_winner = Combat_Count_Sides();" (the dying '
            'unit\'s status not yet set): the unit ([bp+6]), combat_winner and the other defenders\' statuses read',
      bug='combat_winner -1 (both sides alive) although the dying unit is the last enemy',
      fixed='the attacker (0): the battle / the Auto turn ends at once')
def c303(log):
    import dbg
    s = mom.SaveFile()
    key = mom.setup_attack(s, HERO, [2], (30, 28, 0))
    _alone(s, HERO)
    for f, v in (('owner', 5), ('type', 44), ('hero_slot', -1)):
        s.set('units', 2, f, v)
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        d.want_line('src/wizards/COMBTARG.C', 'if (Is_Combat_Only_Summon(unit))', fn='Battle_Apply_Damage',
                    label='dies')
        k = [0]

        def poll():
            k[0] += 1
            if mom.in_combat(d.lv):
                d.lv.D.key('a')
        h = d.wait_via_entry('Battle_Apply_Damage', 150, poll)   # overlays move during the attack
        if h is None:
            return dict(shown=None, observed='no unit died')
        unit = d.word(h['SS'], h['BP'] + 6)
        winner = d.lv.glob('combat_winner')
        bus = mom.battle_units(d.lv, ('unit_index', 'owner', 'status', 'figures_alive'))
    finally:
        d.stop()
    dying = bus[unit] if 0 <= unit < len(bus) else None
    others = [b for i, b in enumerate(bus) if i != unit and b['owner'] == 5 and b['status'] == 0]
    obs = ('dying battle unit %d %s; combat_winner %d; other living defenders %s; all %s'
           % (unit, dying, winner, others, bus))
    if dying is None or dying['owner'] != 5:
        return dict(shown=None, observed=obs + ' - the first death was not the defender\'s')
    return dict(shown=winner == -1 and not others, observed=obs)


@case(191, 'CB', 'Cloak of Fear works backwards: the defender\'s failed fear rolls cut the ATTACKER\'s strikers',
      setup='Dracon\'s first unit made Wolf Riders (88: 5 figures, resistance 4, no enchantments) attacks neutral '
            'unit 2 made Barbarian Spearmen (39: no First Strike) WITH the Cloak of Fear unit enchantment (0x08) and '
            'unit 13; the human only ends its combat turns (D), so the neutral Spearmen attack the Wolf Riders (they '
            'always struck first, 10/24); stops at Cloak_Of_Fear_Count(attacker, defender) entries with the '
            'Spearmen as the attacker and the Wolf Riders as the defender: the call site (the return address in '
            'Combat_Exchange) and the result (run to the return). Combat_Exchange calls it twice per exchange: '
            'first to cut the defender\'s counter strikers (correct), then - the later site - as "feared" for the '
            'attacker\'s own strike',
      bug='the later call returns > 0: the Wolf Riders\' failed fear rolls cut the Spearmen\'s (the cloak owner\'s) '
          'own strikers', fixed='the attacker\'s figures uncut (no fear count for the cloak\'s owner)')
def c191(log):
    import dbg
    import cases_cb as CB
    s = mom.SaveFile()
    att = CB._human_unit_at_dracon(s)
    key = mom.setup_attack(s, att, [2, 13], (30, 28, 0))
    _alone(s, att)
    for f, v in (('type', 88), ('hero_slot', -1), ('level', 0), ('experience', 0), ('enchantments', 0)):
        s.set('units', att, f, v)
    for u in (2, 13):
        s.set('units', u, 'owner', 5)
    s.set('units', 13, 'type', 104)
    s.set('units', 13, 'hero_slot', -1)
    for f, v in (('type', 39), ('level', 0), ('experience', 0), ('enchantments', N('UNIT_ENCH_CLOAK_OF_FEAR'))):
        s.set('units', 2, f, v)
    calls = []                                  # (call site, return IP, result, (Spearmen, Wolf Riders figures))
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        a, tg = idx[2], idx[att]               # the cloaked attacker (neutral), the defender (the Wolf Riders)
        d.want('Cloak_Of_Fear_Count', label='fear', when=lambda h: d.arg(0, h) == a and d.arg(1, h) == tg)

        def done():                              # a call at the later site that cut figures
            return len({c[1] for c in calls}) >= 2 and any(c[1] == max(x[1] for x in calls) and (c[2] or 0) > 0
                                                           for c in calls)
        t0 = time.time()
        while time.time() - t0 < 150 and not done():
            if mom.in_combat(d.lv):
                d.lv.D.key('d')                 # Done: the neutral side's turn
            h = d.wait(10, None)
            if h is None:
                continue
            if h['name'] != 'fear':
                d.go()
                continue
            ret_ip = d.word(h['SS'], h['SP'], False)
            site = d.where(d.word(h['SS'], h['SP'] + 2, False), ret_ip)
            figs = (d.lv.get('battle_units', a, 'figures_alive'), d.lv.get('battle_units', tg, 'figures_alive'))
            calls.append((str(site), ret_ip, d.ret_ax(), figs))
            d.go()
    finally:
        d.stop()
    obs = ('Cloak_Of_Fear_Count(Spearmen with the cloak, Wolf Riders) calls (site, return IP, result, (Spearmen, '
           'Wolf Riders figures)): %s' % [(c[0], '%04X' % c[1], c[2], c[3]) for c in calls])
    sites = sorted({c[1] for c in calls})
    if len(sites) < 2:
        return dict(shown=None, observed=obs + ' - no full exchange by the cloaked Spearmen reached')
    own = [c[2] for c in calls if c[1] == sites[-1]]     # only Combat_Exchange calls it (COMBRES.C 270 / 289 / 303;
    #                                                      270 is the First Strike path): the later site is 303
    if not any(own):
        return dict(shown=None, observed=obs + ' - no defender figure failed its fear roll (no evidence either way)')
    return dict(shown=True, observed=obs + '; the attacker-strike site %04X results %s' % (sites[-1], own))


@case(231, 'CB', 'Raise Dead lists living units: a recalled (or fled) unit can be raised back into the battle',
      setup='heroes 19 and 33 attack neutral Settlers (unit 2) at (30,28); turn 1: the human casts Recall Hero (205) '
            'on hero 33 (status 1), Done; turn 2: Raise Dead (the pick list built in Raise_Dead: tracepoint on '
            '"candidates[count] = i;", i = DI) and a free square clicked; hero 33\'s battle status and '
            'figures read after',
      bug='hero 33 (recalled, alive) is the candidate and comes back (status 0)', fixed='only dead units listed')
def c231(log):
    import dbg
    s = mom.SaveFile()
    key = mom.setup_attack(s, HERO, [2], (30, 28, 0))
    _alone(s, HERO)
    for f, v in (('order', N('ORDER_READY')), ('finished', 0)):
        s.set('units', 33, f, v)
    s.set('units', 33, 'moves_left', s.get('units', 33, 'moves_max'))
    for f, v in (('owner', 5), ('type', 44), ('hero_slot', -1)):
        s.set('units', 2, f, v)
    mom.learn_spells(s, 0, [N('SPELL_RECALL_HERO'), N('SPELL_RAISE_DEAD')])
    s.set('hero_stats', HERO, 'caster_level', 0)   # hero 19 active and a caster: S would ask who casts
    d = dbg.boot(s, log=log)
    try:
        d.start_combat(key)
        idx = {d.lv.get('battle_units', i, 'unit_index'): i for i in range(d.lv.glob('battle_unit_count'))}
        h33 = idx[33]
        mom.cast_spell(d.lv, N('SPELL_RECALL_HERO'), target=h33, log=log)
        st1 = d.lv.get('battle_units', h33, 'status')
        turn0 = d.lv.glob('combat_turn_count')
        for _ in range(30):                     # Done until the human's next combat turn
            if mom.in_combat(d.lv) and d.lv.glob('combat_turn_count') > turn0:
                break
            if mom.in_combat(d.lv):
                d.lv.D.key('d')
            d.wait(1.5, None)
        d.want_line('src/wizards/COMBDISP.C', 'candidates[count] = i;', fn='Raise_Dead', label='cand',
                    trace=lambda h: h['DI'])     # i is DI (COMBDISP.ASM: mov word ptr [bx],di)
        mom.cast_spell(d.lv, N('SPELL_RAISE_DEAD'), log=log)
        hx, hy = d.lv.get('battle_units', idx[HERO], 'combat_x'), d.lv.get('battle_units', idx[HERO], 'combat_y')
        sx, sy = mom.grid_to_screen(hx - 1, hy)  # a square beside hero 19
        d.lv.g.click(sx, sy + 8, settle=0.5)
        d.wait_via_entry('Raise_Dead', 12, None)   # it draws the battlefield (other overlays) before the list
        for _ in range(3):
            d.lv.D.key('enter')
            time.sleep(1)
        cands = list(d.traces.get('cand', []))
        st2 = d.lv.get('battle_units', h33, 'status')
        fig = d.lv.get('battle_units', h33, 'figures_alive')
    finally:
        d.stop()
    obs = ('hero 33 (battle unit %d): status %d after Recall Hero; Raise Dead candidates (i) %s; hero 33 after Raise '
           'Dead: status %d, figures %d' % (h33, st1, cands, st2, fig))
    if st1 != N('BATTLE_UNIT_RECALLED'):
        return dict(shown=None, observed=obs + ' - the recall did not land')
    if not cands:
        return dict(shown=None, observed=obs + ' - Raise Dead\'s list was not built')
    return dict(shown=h33 in cands, observed=obs)


@case(77, 'CB', 'hero items past the 6th won in one battle are never shown nor freed (the award screen takes 6)',
      setup='item 1\'s record copied into free item records 2..10, three each given to wizard 1\'s heroes 13, 50 and '
            '121, moved into wizard 1\'s city 21 Rastenford placed at (30,28) on Arcanus (no fleeing from a city); '
            'Dracon\'s stack attacks them, auto combat, the award screen answered '
            '(items dropped, OK); after: which of records 2..10 are still allocated (cost > 0) with no hero or vault '
            'of any wizard holding them',
      bug='3 (7th..9th) records left allocated and unowned for the rest of the game', fixed='none')
def c77(log):
    import cases_cb as CB
    s = mom.SaveFile()
    o, _, size, _ = mom.TABLES['items']
    recs = list(range(2, 11))
    for r in recs:
        s.b[o + r * size:o + (r + 1) * size] = s.b[o + 1 * size:o + 2 * size]
    for k, h in enumerate((13, 50, 121)):
        sl = s.get('units', h, 'hero_slot')
        for j in range(3):
            s.set('wizards', 1, 'hired_heroes[%d].item_index[%d]' % (sl, j), recs[k * 3 + j])
    for f, v in (('x', 30), ('y', 28), ('plane', 0)):    # in a city the heroes cannot flee with their items
        s.set('cities', 21, f, v)                           # (10/24: hero 13 fled from the open field)
    key = mom.setup_attack(s, CB._human_unit_at_dracon(s), [13, 50, 121], (30, 28, 0))
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    _end_battle(lv, log)
    held = set()
    for w in range(5):
        held |= {lv.get('wizards', w, 'vault_items[%d]' % k) for k in range(4)}
        for sl in range(6):
            held |= {lv.get('wizards', w, 'hired_heroes[%d].item_index[%d]' % (sl, j)) for j in range(3)}
    cost = {r: lv.get('items', r, 'cost') for r in recs}
    heroes = {h: (lv.get('units', h, 'owner'), lv.get('units', h, 'x'), lv.get('units', h, 'y')) for h in (13, 50, 121)}
    leaked = [r for r in recs if cost[r] > 0 and r not in held]
    obs = ('records 2..10 cost after the battle %s; held by a hero / vault %s; allocated and unowned %s; the AI '
           'heroes (owner, x, y) %s' % (cost, sorted(r for r in recs if r in held), leaked, heroes))
    if any(heroes[h][0] != -1 for h in heroes):   # all three dead: 9 items collected (a survivor keeps its 3)
        return dict(shown=None, observed=obs + ' - not every AI hero died')
    return dict(shown=len(leaked) >= 3, observed=obs)
