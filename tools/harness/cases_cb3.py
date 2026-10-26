"""P4 B4 combat cases with a human spell on the human's own units or on a whole side (N228)."""
import mom
from regress import case
from cases_cb2 import _spell_battle

N = mom.N
TURTLE = 161                                    # a human Dragon Turtle (type 126, 2 moves) at Dracon


def _bu(lv, idx, u, fields):
    return mom.stable(lv, 'battle_units', idx[u], fields)


@case(248, 'CB', 'Holy Weapon cast in combat gives no to-hit bonus',
      setup='the human casts Holy Weapon (124) on its Dragon Turtle 161 during the battle',
      bug='melee_hit_bonus unchanged (Unit_Apply_Modifiers sees only unit / item enchantments)', fixed='+1')
def c248(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_HOLY_WEAPON')], log)
    f = ('melee_hit_bonus', 'combat_enchantments')
    a = _bu(lv, idx, TURTLE, f)
    mom.cast_spell(lv, N('SPELL_HOLY_WEAPON'), target=idx[TURTLE], log=log)
    b = _bu(lv, idx, TURTLE, f)
    mom.finish_combat(lv, log=log)
    obs = 'turtle melee_hit_bonus %d -> %d; combat_enchantments 0x%x -> 0x%x' % (
        a['melee_hit_bonus'], b['melee_hit_bonus'], a['combat_enchantments'] & 0xffffffff,
        b['combat_enchantments'] & 0xffffffff)
    holy = b['combat_enchantments'] & N('UNIT_ENCH_HOLY_WEAPON')
    return dict(shown=bool(holy) and b['melee_hit_bonus'] == a['melee_hit_bonus'], observed=obs)


@case(268, 'CB', 'Haste adds no movement on the turn it is cast',
      setup='the human casts Haste (70) on its Dragon Turtle 161 (full moves) during the battle',
      bug='its moves this turn stay as they were', fixed='doubled at once')
def c268(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_HASTE')], log)
    a = _bu(lv, idx, TURTLE, ('moves', 'combat_flags'))
    mom.cast_spell(lv, N('SPELL_HASTE'), target=idx[TURTLE], log=log)
    b = _bu(lv, idx, TURTLE, ('moves', 'combat_flags'))
    mom.next_human_turn(lv, log=log)
    c = _bu(lv, idx, TURTLE, ('moves', 'combat_flags'))
    mom.finish_combat(lv, log=log)
    obs = 'turtle moves %d (flags 0x%x) -> %d after Haste (flags 0x%x) -> %d next turn' % (
        a['moves'], a['combat_flags'], b['moves'], b['combat_flags'], c['moves'])
    return dict(shown=b['combat_flags'] & 0x800 and b['moves'] == a['moves'] and c['moves'] > a['moves'], observed=obs)


@case(235, 'CB', 'Flight cast in combat: no movement minimum',
      setup='the human casts Flight (56) on its Dragon Turtle 161 (2 moves = 4 half moves) during the battle; its moves '
            'read then and on the next combat turn', bug='no 3-move minimum on the turn it is cast (moves stay 4); from '
            'the next turn Unit_Combat_Moves gives 6', fixed='6 half moves at once')
def c235(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_FLIGHT')], log)
    a = _bu(lv, idx, TURTLE, ('moves', 'move_flags'))
    mom.cast_spell(lv, N('SPELL_FLIGHT'), target=idx[TURTLE], log=log)
    b = _bu(lv, idx, TURTLE, ('moves', 'move_flags', 'combat_enchantments'))
    mom.next_human_turn(lv, log=log)
    c = _bu(lv, idx, TURTLE, ('moves', 'move_flags'))
    mom.finish_combat(lv, log=log)
    obs = 'turtle moves %d -> %d after Flight (enchantments 0x%x, move_flags 0x%x) -> %d next turn' % (
        a['moves'], b['moves'], b['combat_enchantments'] & 0xffffffff, b['move_flags'], c['moves'])
    return dict(shown=b['moves'] == a['moves'] < 6 and c['moves'] >= 6, observed=obs)


@case(247, 'CB', 'Wraith Form shields its unit from Flame Strike',
      setup='neutral unit 2 given Wraith Form (unit enchantment 0x20); the human casts Flame Strike (109) on the '
            'enemy side', bug='unit 2 takes no damage while unit 13 does', fixed='both take damage')
def c247(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_FLAME_STRIKE')], log, edits=[(2, 'enchantments', N('UNIT_ENCH_WRAITH_FORM'))])
    f = ('figures_alive', 'front_damage')
    a2, a13 = _bu(lv, idx, 2, f), _bu(lv, idx, 13, f)
    mom.cast_spell(lv, N('SPELL_FLAME_STRIKE'), log=log)
    b2, b13 = _bu(lv, idx, 2, f), _bu(lv, idx, 13, f)
    mom.finish_combat(lv, log=log)
    obs = 'unit 2 (Wraith Form) %s -> %s; unit 13 %s -> %s' % (a2, b2, a13, b13)
    return dict(shown=a2 == b2 and a13 != b13, observed=obs)


@case(14, 'CB', 'Mass Healing is refused while True Light is up',
      setup='the human casts True Light (128) in the first combat turn, then tries Mass Healing (149) in the next',
      bug='Mass Healing refused: the combat skill does not change', fixed='Mass Healing cast')
def c14(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_TRUE_LIGHT'), N('SPELL_MASS_HEALING')], log)
    mom.cast_spell(lv, N('SPELL_TRUE_LIGHT'), log=log)
    k0 = lv.get('wizards', 0, 'combat_skill_left')
    mom.next_human_turn(lv, log=log)
    k1 = lv.get('wizards', 0, 'combat_skill_left')
    mom.cast_spell(lv, N('SPELL_MASS_HEALING'), log=log)
    import time
    for _ in range(3):
        time.sleep(1)
        lv.D.key('enter')
    k2 = lv.get('wizards', 0, 'combat_skill_left')
    mom.finish_combat(lv, log=log)
    obs = 'skill after True Light %d; next turn %d; after the Mass Healing attempt %d' % (k0, k1, k2)
    return dict(shown=k2 == k1, observed=obs)
