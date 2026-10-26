"""P4 B4 combat cases (kind CB), N226: the human attacks from Dracon (29,27) into an enemy stack placed next to it;
mom.setup_attack / start_combat / battle_units / finish_combat."""
import mom
from regress import case

N = mom.N
DRACON = (29, 27, 0)


def _human_unit_at_dracon(s):
    for u in range(s.get('num_units')):
        if s.get('units', u, 'owner') == 0 and (s.get('units', u, 'x'), s.get('units', u, 'y'),
                                                s.get('units', u, 'plane')) == DRACON:
            return u


@case(15, 'CB', 'the AI defender gets an extra combat turn start before the human moves',
      setup='human unit from Dracon (29,27) attacks a unit of AI wizard 1 placed at (30,28)',
      bug='combat_turn_count is 2 at the human\'s first move (Combat_Turn_Start ran twice)', fixed='1')
def c15(log):
    s = mom.SaveFile()
    att = _human_unit_at_dracon(s)
    key = mom.setup_attack(s, att, [2], (30, 28, 0))
    s.set('units', 2, 'owner', 1)
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    n = lv.glob('combat_turn_count')
    bus = mom.battle_units(lv)
    log.append('battle units: %s' % bus)
    mom.finish_combat(lv, log=log)
    obs = 'combat_turn_count %d at the human\'s first move; %d battle units' % (n, len(bus))
    return dict(shown=n == 2, observed=obs)


def _battle(s, log):
    """Dracon's stack (hero 19 selected) attacks units 2 and 13, made neutral, placed at (30,28); returns the live
    game at the human's first combat turn and the battle records by unit index."""
    key = mom.setup_attack(s, _human_unit_at_dracon(s), [2, 13], (30, 28, 0))
    s.set('units', 2, 'owner', 5)              # neutral: an AI wizard's side may flee from combat turn 2 - which,
    s.set('units', 13, 'owner', 5)             # with #15, is before the human's first move; neutrals never flee
    lv = mom.boot(s)
    mom.start_combat(lv, key, log=log)
    fields = ('unit_index', 'owner', 'status', 'figures', 'figures_alive', 'hits_per_figure', 'defense', 'resist')
    bus = {b['unit_index']: b for b in mom.battle_units(lv, fields)}
    log.append('battle units: %s' % bus)
    return lv, bus


@case(233, 'CB', 'ghost units: a unit whose damage reaches its hit points enters the battle with 0 figures',
      setup="unit 2 (type 47: 6 figures, 3 hits each at its level) given damage 127, past its 18 hits - the save "
            "edit stands in for a unit whose maximum hits dropped below its damage; unit 13 with it so the "
            "battle does not end at once; Dracon's stack attacks",
      bug='its battle record: figures_alive 0 (or less), status active', fixed='at least one figure (1 hit) or out')
def c233(log):
    s = mom.SaveFile()
    lv, bus = _battle_with_damage(s, log)
    b = bus.get(2)
    mom.finish_combat(lv, log=log)
    if b is None:
        return dict(shown=None, observed='unit 2 is not in the battle')
    obs = 'unit 2: figures %d, figures_alive %d, status %d, hits per figure %d' % (
        b['figures'], b['figures_alive'], b['status'], b['hits_per_figure'])
    return dict(shown=b['figures_alive'] <= 0 and b['status'] == 0, observed=obs)


def _battle_with_damage(s, log):
    import struct
    # the type's hits and figures: unit_types is initialised data, the same in the EXE as in RAM - read it live
    lv = mom.boot()
    t = s.get('units', 2, 'type')
    hp, fig = lv.get('unit_types', t, 'hits'), lv.get('unit_types', t, 'figures')
    log.append('unit 2 type %d: %d figures x %d hits' % (t, fig, hp))
    s.set('units', 2, 'damage', 127)
    return _battle(s, log)


@case(89, 'CB', 'Crusade adds its level twice to combat hit points',
      setup='human Dragon Turtle 161 (type 126) with experience 60 (veteran) and the level the game stores for it '
            'with Warlord and Crusade, 2 + 1 + 1 = 4; human globals.crusade = 1; Dracon\'s stack attacks',
      bug='hits per figure = the type\'s + 2 (level 5: Battle_Unit_Max_Hits adds Crusade again; > 2 and > 4)',
      fixed='+ 1 (level 4)')
def c89(log):
    s = mom.SaveFile()
    s.set('units', 161, 'experience', 60)
    s.set('units', 161, 'level', 4)
    s.set('wizards', 0, 'globals.crusade', 1)
    lv, bus = _battle(s, log)
    b = bus.get(161)
    base = lv.get('unit_types', 126, 'hits')
    mom.finish_combat(lv, log=log)
    obs = 'Dragon Turtle 161: hits per figure %d in battle, type hits %d' % (b['hits_per_figure'], base)
    return dict(shown=b['hits_per_figure'] == base + 2, observed=obs)


@case(86, 'CB', 'Chaos Channels demon skin counts twice in combat',
      setup='human Dragon Turtles 173 and 192 (type 126) made equal (experience 21, level 2); 173 gets the Chaos '
            'Channels demon skin (mutation 0x04); Dracon\'s stack attacks',
      bug='battle defence of 173 = 192\'s + 6', fixed='+ 3')
def c86(log):
    s = mom.SaveFile()
    for u in (173, 192):
        s.set('units', u, 'experience', 21)
        s.set('units', u, 'level', 2)
    s.set('units', 173, 'mutation_flags', N('MUT_CHAOS_DEMON_SKIN'))
    lv, bus = _battle(s, log)
    d1, d0 = bus[173]['defense'], bus[192]['defense']
    mom.finish_combat(lv, log=log)
    return dict(shown=d1 - d0 == 6, observed='battle defence: 173 (demon skin) %d, 192 %d -> +%d' % (d1, d0, d1 - d0))
