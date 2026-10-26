"""P4 B4 cases where the human casts an overland spell at a target (N225): the spell is set as finished in the save
(casting_cost_left 0); Human_End_Turn casts it when turn 107 starts, and mom.target_square() picks the square."""
import time

import mom
from regress import case

N = mom.N


def _find_unit(lv, owner, x, y):
    for u in range(lv.glob('num_units')):
        if (lv.get('units', u, 'owner'), lv.get('units', u, 'x'), lv.get('units', u, 'y')) == (owner, x, y):
            return u
    return None


@case(33, 'OV', 'Disenchant True never removes Invulnerability',
      setup='unit 2 given to the eliminated wizard 3 (it does not move) at (30,28) next to Dracon with Invulnerability '
            '(bit 31) and enchantment bits 0-5; the human (no Runemaster: retorts Warlord, Famous) finishes Disenchant '
            'True (52) with 5000 mana paid - strength 5000 x 3 = 15000 - and targets that square', bug='bits 0-5 dispelled, Invulnerability kept (the loop stops at bit 30)',
      fixed='Invulnerability can go too')
def c33(log):
    s = mom.SaveFile()
    for f, v in (('owner', 3), ('x', 30), ('y', 28), ('plane', 0), ('enchantments', 0x8000003F)):
        s.set('units', 2, f, v)
    for f, v in (('casting_spell', N('SPELL_DISENCHANT_TRUE')), ('casting_cost_left', 0), ('casting_cost_initial', 5000),
                 ('mana_reserve', 5000)):
        s.set('wizards', 0, f, v)
    lv = mom.boot(s)
    mom.end_turn(lv, log=log)
    mom.target_square(lv, 30, 28, log=log)
    for _ in range(6):                          # the 'X has been dispelled' boxes
        time.sleep(1.5)
        lv.D.key('enter')
    time.sleep(2)
    u = _find_unit(lv, 3, 30, 28)
    if u is None:
        return dict(shown=None, observed='the target unit was not found after the cast')
    e = lv.get('units', u, 'enchantments') & 0xffffffff
    obs = 'unit %d enchantments 0x%08x after the cast (was 0x8000003f); casting now %d' % (
        u, e, lv.get('wizards', 0, 'casting_spell'))
    return dict(shown=bool(e & 0x80000000) and (e & 0x3f) != 0x3f, observed=obs)
