"""P4 B4 combat cases with a human spell (N227): the wizard learns the spell (mom.learn_spells: mana and skill
plentiful), Dracon's stack attacks neutral units 2 and 13 at (30,28), mom.cast_spell picks it in the combat book and
clicks the target's square."""
import mom
from regress import case
from cases_cb import _human_unit_at_dracon

N = mom.N


def _spell_battle(s, spells, log, edits=()):
    mom.learn_spells(s, 0, spells)
    key = mom.setup_attack(s, _human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    s.set('units', 2, 'owner', 5)
    s.set('units', 13, 'owner', 5)
    s.set('units', 13, 'type', 104)             # unit 13 is wizard 1's hero: as a 'neutral hero' it would give the
    s.set('units', 13, 'hero_slot', -1)         # neutral side a Prayermaster aura (+4 resistance); a plain unit instead
    for u, f, v in edits:
        s.set('units', u, f, v)
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    idx = {lv.get('battle_units', i, 'unit_index'): i for i in range(lv.glob('battle_unit_count'))}
    return lv, idx


@case(91, 'CB', 'resistance is not clamped at 0 in combat',
      setup='neutral unit 2 made type 84 (resistance 4; Zombies, 3, are immune to Sorcery), level 0; the human '
            'casts Mind Storm (67: -5 resistance and more) on it',
      bug='its battle resistance goes below 0', fixed='0 at least (melee / ranged / defence are clamped)')
def c91(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_MIND_STORM')], log, edits=[(2, 'type', 84), (2, 'level', 0),
                                                                    (2, 'experience', 0)])
    t = idx[2]
    r0 = lv.get('battle_units', t, 'resist')
    mom.cast_spell(lv, N('SPELL_MIND_STORM'), target=t, log=log)
    import time
    reads = []
    for _ in range(4):                          # stable values, not a record caught mid-recompute
        time.sleep(1.5)
        reads.append(tuple(lv.get('battle_units', t, f) for f in ('resist', 'defense', 'melee', 'combat_flags',
                                                                    'unit_index', 'status')))
    log.append('reads after the cast (resist, defense, melee, combat_flags, unit, status): %s' % reads)
    r1, d1 = reads[-1][0], reads[-1][1]
    if len(set(reads)) != 1:
        return dict(shown=None, observed='unstable reads after the cast: %s' % reads)
    mom.finish_combat(lv, log=log)
    return dict(shown=r1 < 0, observed='unit 2 resistance %d -> %d after Mind Storm (defence now %d)' % (r0, r1, d1))


def _cast_counts(lv, spell, target, log):
    k0 = lv.get('wizards', 0, 'combat_skill_left')
    m0 = lv.get('wizards', 0, 'mana_reserve')
    mom.cast_spell(lv, spell, target=target, log=log)
    for _ in range(3):
        lv.D.key('enter')                       # a refusal's warning box, if any
    k1 = lv.get('wizards', 0, 'combat_skill_left')
    m1 = lv.get('wizards', 0, 'mana_reserve')
    return k0, k1, m0, m1


@case(211, 'CB', 'Dispel Evil can be cast on a creature of any realm',
      setup='the human casts Dispel Evil (134) on neutral unit 13 (type 5, a normal unit - not Chaos or Death)',
      bug='the cast goes ahead (skill and mana spent)', fixed='refused like Star Fires (Chaos / Death only)')
def c211(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_DISPEL_EVIL')], log)
    race = lv.get('battle_units', idx[13], 'race')
    k0, k1, m0, m1 = _cast_counts(lv, N('SPELL_DISPEL_EVIL'), idx[13], log)
    mom.finish_combat(lv, log=log)
    obs = 'target race %d; combat skill %d -> %d, mana %d -> %d' % (race, k0, k1, m0, m1)
    return dict(shown=race < N('RACE_CHAOS_CREATURE') and k1 < k0, observed=obs)


@case(212, 'CB', 'Life Drain can be cast on a Death creature',
      setup='neutral unit 13 made Skeletons (165, race 20, Death-immune); the human casts Life Drain (167) on it',
      bug='the cast goes ahead (skill and mana spent)', fixed='refused, as the AI picker does')
def c212(log):
    s = mom.SaveFile()
    lv, idx = _spell_battle(s, [N('SPELL_LIFE_DRAIN')], log, edits=[(13, 'type', N('UNIT_SKELETONS'))])
    race = lv.get('battle_units', idx[13], 'race')
    k0, k1, m0, m1 = _cast_counts(lv, N('SPELL_LIFE_DRAIN'), idx[13], log)
    mom.finish_combat(lv, log=log)
    obs = 'target race %d; combat skill %d -> %d, mana %d -> %d' % (race, k0, k1, m0, m1)
    return dict(shown=race == N('RACE_DEATH_CREATURE') and k1 < k0, observed=obs)
