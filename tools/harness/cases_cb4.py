"""P4 B4 combat cases, N229: the human casts in battle (cases_cb2._spell_battle) and reads the battle records."""
import mom
from cases_cb2 import _spell_battle
from regress import case

N = mom.N


def _bu(lv, idx, u, fields):
    return mom.stable(lv, 'battle_units', idx[u], fields)


@case(85, 'CB', 'Weakness never weakens a thrown attack',
      setup='neutral unit 2 made type 45 (thrown attack 3, ranged_type 100), level 0; the human casts Weakness (162) '
            'on it', bug='its battle ranged value unchanged', fixed='ranged - 2')
def c85(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_WEAKNESS')], log, edits=[(2, 'type', 45), (2, 'level', 0),
                                                                  (2, 'experience', 0)])
    f = ('ranged', 'melee', 'combat_enchantments')
    a = _bu(lv, idx, 2, f)
    mom.cast_spell(lv, N('SPELL_WEAKNESS'), target=idx[2], log=log)
    for _ in range(3):
        lv.D.key('enter')                       # a resisted / failed notice, if any
    b = _bu(lv, idx, 2, f)
    mom.shot('after_cast', log)
    mom.finish_combat(lv, log=log)
    weak = b['melee'] < a['melee']              # Weakness took hold (it is not a combat-enchantment bit)
    obs = 'unit 2 (type 45, thrown): ranged %d -> %d, melee %d -> %d, combat_enchantments 0x%x -> 0x%x' % (
        a['ranged'], b['ranged'], a['melee'], b['melee'], a['combat_enchantments'] & 0xffffffff,
        b['combat_enchantments'] & 0xffffffff)
    return dict(shown=weak and b['ranged'] == a['ranged'] and b['melee'] < a['melee'], observed=obs)


@case(84, 'CB', 'Chaos Surge counts only when the HUMAN holds it',
      setup='AI wizard 1 holds Chaos Surge (globals.chaos_surge = 1), the human does not; neutral unit 2 made the chaos '
            'creature type 155, level 0; Dracon\'s stack attacks it', bug='its battle melee = the type\'s (no +2)',
      fixed='+2 (any wizard\'s Chaos Surge)')
def c84(log):
    s = mom.SaveFile()
    s.set('wizards', 1, 'globals.chaos_surge', 1)
    lv, idx = _spell_battle(s, [], log, edits=[(2, 'type', 155), (2, 'level', 0), (2, 'experience', 0)])
    b = _bu(lv, idx, 2, ('melee', 'ranged', 'race'))
    base = lv.get('unit_types', 155, 'melee'), lv.get('unit_types', 155, 'ranged')
    hs = lv.get('wizards', 0, 'globals.chaos_surge'), lv.get('wizards', 1, 'globals.chaos_surge')
    mom.shot('battle', log)
    mom.finish_combat(lv, log=log)
    obs = 'Chaos Surge human %d / wizard 1 %d; unit 2 (race %d) battle melee %d ranged %d, type melee %d ranged %d' % (
        hs + (b['race'], b['melee'], b['ranged']) + base)
    return dict(shown=b['race'] == N('RACE_CHAOS_CREATURE') and b['melee'] == base[0], observed=obs)
