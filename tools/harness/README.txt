MASTER OF MAGIC 1.31 TEST HARNESS - START HERE
==============================================
Drives the real game (WIZARDS.EXE / MAGIC.EXE 1.31) inside DOSBox-X on a hidden Windows desktop, edits saves,
reads the emulated RAM and sets debugger breakpoints. Used for the regression suite: every bug in
BUGS_AND_QUIRKS.txt gets a case that shows 1.31's behaviour (BUG SHOWN) - a fixed build should give NOT SHOWN.
Written so a person (or their own AI assistant) can run and extend it from what is checked in.

Deeper references (read when needed):
  docs/ui/HARNESS_UI_NOTES.txt   every screen / key / click the harness uses, how each was verified
  docs/DOSBOX_X_DEBUG.txt        the DOSBox-X debugger channel, savestates, the debugger lessons
  tools/harness/TEST_CASES.txt   all cases with setup / 1.31 / fixed / last result (regress_doc.py regenerates it)
  tools/harness/B4_SUITE.tsv     the plan: one row per numbered entry, its kind and what the case does
  BUGS_AND_QUIRKS.txt            the bug list; 'In game (date, regress.py N)' lines come from the suite

1. WHAT YOU NEED, AND SETUP
  - Windows (the input driver uses a private desktop + PostMessage; another OS needs a new driver in dosbox.py).
  - Python 3 (3.13 used) with Pillow (screenshots, dosbox.shot) and capstone (srcmap.py); Win32 calls go through
    ctypes (no pywin32). numpy only for memdiff.py.
  - Your own Master of Magic v1.31 (GOG). setup.py copies its files into work/ (git-ignored); the harness runs that
    copy (work/harness/game/) and only ever writes its SAVE9.GAM (restored afterwards). Your install is only read.
  - DOSBox-X: the release build (2026.10.01 verified) for plain runs; for the breakpoint cases (dbg.py) a newer build
    with upstream PRs #6682 / #6683 (the debugger channel's SAVESTATE / LOADSTATE / MEMREAD; a CI build of the main-ai
    branch after 2026-10-17, or a release after 2026.10.01). See docs/DOSBOX_X_DEBUG.txt.
  - For source-line breakpoints (d.want_line, srcmap.py): Borland C++ 3.1, installed by build/install_bc31.py.
  - Once:
      python tools/harness/setup.py --game "path/to/Master of Magic" --dosbox path/to/dosbox-x.exe
             [--dosbox-debug path/to/newer/dosbox-x.exe] [--bc path/holding/BORLANDC] [--cp path/to/Community Patch]
    It checks your WIZARDS.EXE / MAGIC.EXE (SHA-256), prepares work/ and stores the paths in work/paths.json.
  - The base save tools/harness/base/SAVE9_orig.GAM (an autosave of the project owner's game: turn 106, Merlin at
    Dracon (29,27)); every case edits a copy. base/MAGIC.SET holds the settings the cases were run with (no names).
  - The MG census cases read worlds made by the real MAGIC.EXE: make them first with
      python -P tools/harness/mapgen_survey.py 10 small medium large     (and: 10 ... diff=3 sub=hard)
    New worlds differ from the ones in TEST_CASES.txt (the generator is seeded from the clock); the census cases
    test rates, so their verdicts should hold.
  - Comments, this file and TEST_CASES.txt sometimes name project files that are not published (work/reports/...,
    tools/check_turn_economy.py, re/fn_census.tsv ...): they are the project's records; no case needs them.

2. RUNNING
  python -P tools/harness/regress.py 74 72 127          run cases (one DOSBox-X at a time: work/harness/RUN.LOCK)
  python -P tools/harness/regress.py --kind ST          a kind: ST static (no DOSBox), OV overland, CB combat ...
  python -P tools/harness/b4show.py 74                  what a case does and its last result
  python -P tools/harness/regress_doc.py [--write]      results -> TEST_CASES.txt + BUGS_AND_QUIRKS.txt lines
  python -P tools/harness/dbg_selftest.py               checks watchpoints / conditions / tracepoints (~1 min)
  Results: work/harness/regress/results.tsv (one line per run), NUM.txt (the case's log), shots/NUM_tag.png.
  Static checks against another build: set MOM_TEST_EXE_DIR to its folder.
  Never run two DOSBox users at once (a case run, srcmap.py's compiler, build/build.py): they share the
  debugger port, Game Link memory and the leftover-process killer.

3. WRITING A CASE  (tools/harness/cases_*.py; see cases_dbg1.py for many examples)
  @case(NUM, 'OV', 'title', setup='...', bug='what 1.31 does', fixed='what a fix does', also=(other bug nums,))
  def cNUM(log):
      s = mom.SaveFile()                       # the base save; s.set('units', 19, 'x', 30), s.set('cities', 3,
      ...                                      #   'buildings[12]', ...), s.set('wizards', 1, 'globals.evil_omens', 1)
      lv = mom.boot(s)                         # plain run; or d = dbg.boot(s) for breakpoints (d.lv = the same)
      ...                                      # act through the game's own UI (keys, clicks), then read
      return dict(shown=<bool>, observed='what was seen, with numbers')
  Setup = save edits only (the game loads them normally). Actions = real input. Observation = reads only.
  Helpers: mom.setup_attack (a battle next to Dracon), cases_cb2._spell_battle (the human casts in battle),
  mom.cast_spell / finish_combat / end_turn / items_award, cases_dbg1._empty_lair / _ai_casts / _all_casting /
  _only_summons_upkeep, cases_move._place / _selected. mom.shot('tag', log) saves evidence (only while running).

4. BREAKPOINTS, CONDITIONS, WATCHPOINTS  (dbg.py)
  d.want('City_Change_Owner')                         function entry (names: re/names.tsv), or + off, or 'seg:off'
  d.want_line('src/wizards/AIPROD.C', 'if (p_cities[i].population < 5)')   a C source line (srcmap.py)
  d.want('X', when=lambda h: d.arg(0, h) == 21)       condition, checked in Python at each stop (false: resumed)
  d.want('X', nth=3) / d.want('X', trace=fn)          the 3rd passing stop / a tracepoint (fn(hit) -> d.traces)
  d.watch('own', ('cities', 21, 'owner'), when=lambda h: 'City_Change_Owner' in h['writer'])
                                                      stop after a write; hit: old / new bytes, writer function
  h = d.wait(timeout, poll)                           run until a stop; poll() presses keys meanwhile
  d.arg(k, h) / d.local(-6, h) / d.regs() / d.lv.get(...)   read; d.ret_ax() runs to the return; d.go() resumes
  d.call('F', where=lambda a: a[0] == 1, nargs=2)     wait for a matching call, run it: (args, return value)
  d.action() right before the key that starts the tested action; d.end_turn_poll() / d.turns_poll(turn) end turns.
  d.arg(k, h) reads SS:SP + 4 + 2k: valid at a FUNCTION ENTRY only; at a line inside a function the arguments
  are at SS:BP + 6 + 2k (d.word(h['SS'], h['BP'] + 6 + 2 * k)) - a when= test with d.arg there never passes (#282).
  d.follow = {ghidra overlay seg, ...}: keep that module's stub thunks armed even while it is loaded, so every
  entry from outside re-places its breakpoints - for a call that comes after other overlay traffic (a loaded
  overlay can be MOVED by another's load; #245 #410). A breakpoint on the line after a call into ANOTHER overlay
  can miss for the same reason on the return path: stop in the callee instead (its RETF: #408).
  A function's RETF offset: from its BCC -S listing (srcmap.py) or a disassembly of the compiled object; the
  function sizes of the map undercount functions in several ranges (#471: 528 vs 0x270).
  Prefer a watchpoint when you know WHAT changes but not which code path: it names the writer and does not care
  where overlays are (#66, #10). Prefer a breakpoint for a function's arguments or a local at a source line.
  A function that calls other overlays before the line you want (Combat_Strategic, Plane_Shift_Cast, the neutral
  raid): d.follow its module AND switch d.ovr_always on - from the start for a short human action (cases_ov6
  _auto_battle), or, inside a turn change, only once a breakpoint on the function's entry (or a sibling called
  just before it) fires (cases_ov6 _raid, c307): ovr_always for the whole AI turn crawls.
  d.dump(seg, off, n) reads memory through the debugger (MEMDUMPBIN): use it for anything in the EMS page frame
  (p_path_work's cost map, #256) - Game Link's RAM view does not show the mapped pages (it read 255 everywhere).
  srcmap.places(..., only=fn) / d.want_line(..., fn=...) align only that function: one function of the file whose
  listing does not match the game no longer stops the lookup (OVERSPEL.C's Wall_Of_Stone_Cast).

5. THINGS THAT COST HOURS (details and dates in DOSBOX_X_DEBUG.txt)
  - Debugger operands are expressions: send 4-digit hex ('BP 2E1C AC' set 2E1C:0000 - AC is a flag).
  - BPLIST numbers its entries in HEX ('2E. BP ...'); BPDEL takes that number. Parse it as hex.
  - Thunk breakpoints are armed only while a wanted overlay is not loaded (a hot module's thunks stop the game at
    every call).
  - In a dbg session, plain mom helpers (diplo_open, target_square, end_turn ...) do not resume the game when a
    breakpoint stops it: reach the state first and arm afterwards, or wait with d.wait(timeout, poll).
  - Breakpoints take effect when the debugger resumes; resuming from a breakpoint can stop one instruction later
    (dbg.go absorbs it); never delete and re-add the breakpoint the CPU sits on.
  - Overlays (Borland VROOMM) move: the load segment is the stub's word +0x10, and loading ONE overlay can move
    ANOTHER that is already loaded. dbg follows a wanted overlay through its module's stub THUNKS (d.thunks, the
    default since 10/23): every entry of the module from outside stops once at a resident thunk, and the real
    breakpoints are placed at the overlay's current segment right then; a hit is checked against the current
    overlay map before it is reported. The old way - a stop at the overlay manager's exit on EVERY overlay load
    (d.thunks = False, or d.ovr_always) - made a turn change crawl (the envy line: not reached in 150 s; with
    thunks: 7.6 s). When the thunk stop finds the overlay NOT loaded (its INT 3Fh loads it next), the overlay
    manager's exit stop is armed until a wanted overlay is in (d.pending_load), so the breakpoints are placed before
    the module's code runs - without this the whole first entry (and every call it makes inside its own module) was
    missed: AI_Diplo_Turn_Vs_Human's calls during a turn change, 10/23.
  - Input is LOST while the overlay-manager stop is armed on a busy screen (the end-turn screen reloads overlays
    ~2.5 times a second): send keys / clicks with d.press(key) or d.press(click=(x, y)), which drops that stop
    for the input and sets it again. d.turns_poll() does this, and stops pressing once a tracepoint on Next_Turn's
    entry fires (the end-turn screen stays up for the whole turn change and game_turn++ comes near Next_Turn's END,
    NEXTTURN.C - neither shows that the turn change has started).
  - Dialogs drawn over the map keep current_screen = 5 (map): mom.on_map() checks for the top bar's controls.
    A lair / tower notice closes on Esc only (one whole-screen control); 'destroy this city?' takes R / N; a
    treaty 'still wish to attack?' takes Y; the Items award screen needs a drop + O (mom.items_award).
  - Many rules are random in order or amount (Mana_Shortfall picks its four categories at random): set the save
    up so only the path under test can act (#6 removes the human's enchantments).
  - The release build seeds Random from the timer at load: human-turn battles differ run to run; the end of turn
    reseeds from turn_rng_seed. Cases test rules (arguments, writers, fields), not dice.
  - Saves can create states normal play cannot (two cities side by side): fine for most rules, but keep them away
    from what the case measures (shared city squares halve food / production).
  - Screenshots only while the game runs (not at a breakpoint stop).

6. BASE-SAVE FACTS AND SETUP PITFALLS  (each one made a case fail or lie; learned 10/23)
  - The base save (turn 106, Merlin at Dracon (29,27), Arcanus) has NO human summons: its 'Dragon Turtles' are
    Lizardman units (type 126). Retype units for summons (War Bears 179, Djinn 195 = innate Wind Walking).
  - AI wizard 1 is hostile to no one (hostility all 0): its wizard-target spells (Spell Blast, Drain Power,
    Subversion, Cruel Unminding) are cancelled before they act. cases_dbg1._ai_hostile.
  - The human's army stat is the weakest (5 vs 27 / 10 / 0 / 13); the stats are recomputed only AFTER the drift
    in Next_Turn, so the first turn change uses the saved values.
  - Wizard 1's fortress (49,14) and city 1 are on Myrror; mom.target_square works on the viewed plane (Arcanus):
    pick Arcanus targets (city 2 at (4,11), Cornwell 3 at (14,29)).
  - (29,26) north of Dracon is shore WATER (tile 82; land tiles start at 162).
  - The unit table is COMPACTED at the end of every turn change: after end_turn find units by owner / type /
    square, never by index. Two identical units on one square share that key - use max / min with care.
  - The turn ends by itself when no human unit is left to move: keep a Ready unit (with a HIGHER index than the
    stack under test, or the game selects it first) when a case must stay in the turn.
  - The game selects the first unit needing orders: Patrol alone does not prevent it - set 'finished' too
    (cases_move._all_patrol).
  - A save-set building state 2 (removed this turn) is cleared by the turn end before a spell cast at the next
    turn start: in-turn states cannot be prepared by the save for turn-start events.
  - Rules that pick a random category / order (Mana_Shortfall, AI spell categories) need the setup to leave only
    the path under test (e.g. remove the human's enchantments for the summons category).
  - A cast global stores player + 1 (wizard 1's Tranquility = 2), not 1.
  - The HUMAN's finished overland spell (casting_cost_left 0) is cast at the START of the human's next turn
    (MOVE.C, after Next_Turn and its notices), not during the turn change: keep answering screens past the new turn
    (d.turns_poll(turn + 2)); an AI's spell is cast inside the turn change (AITURN.C).
  - mom.setup_attack sets Patrol only: give the other human units 'finished' too, or the game may select another
    stack than the attacker (#466: Dracon's stack walked instead).
  - The turn change is the same every run for a given save (turn_rng_seed is not saved): to get a different
    outcome of its dice, vary something the AI turn reads (AI-AI contacts worked for the diplomacy proposals).
  - Ghidra's functions.tsv names can be stale: dbg.where names functions from re/names.tsv.
  - BUGS_AND_QUIRKS claims are checked, not trusted: #227's 'City_Change_Owner skipped' was wrong (traced: it runs)
    - correct the entry when the game says otherwise.
  - A case's verdict test must not be satisfiable by a different branch (#50: a human army of 0 took the -100
    branch and looked like the wrap) - require the inputs the bug needs before judging.
  - A battle against an AI wizard in a debugger session makes DOSBox-X stop at F000:D186 (unknown stops; d.wait
    resumes them, a plain helper or time.sleep does not and the game freezes): use d.start_combat(key) and wait with
    d.wait(...) inside such battles (#196 / #326 failed with mom.start_combat, 10/24).
  - Never start a second regress.py while a batch runs: it restores SAVE9.GAM after each case and stopped DOSBox at
    the end (10/24: a static-only run killed a batch's DOSBox mid-case, twice). Now only a case that took the DOSBox
    lock (mom.lock: mom.boot, dbg.boot, cases_game) restores the save and stops DOSBox - static and file-reading
    (MG) runs may go beside a batch.
  - The human's own battles are auto-resolved (Combat_Strategic) only with Strategic Combat Only on: cases_ov6
    _strategic_on turns it on in the settings screen and leaves with Esc - memory only (OK would write MAGIC.SET).
  - The casting skill is recomputed from skill_points (the smallest n with n (n + 1) >= points; +10 Archmage):
    setting nominal_casting_skill alone does not hold (#294's first run had skill 25).
  - A tower square is in the lair table too (lair 3 = tower 3 at (31,27), active): an uncleared tower is an
    obstacle of its own; clear it (active 0) to stand units in it.
  - A human battle for ONE unit: _alone (cases_cb5) as well as setup_attack, or Dracon's whole stack fights and
    another unit is the active one (#228's first run: a thrower answered Can_Attack_Target through its ranged branch).
  - The AI does not attack a weak human city just because its army stands next to it and it is at war (10/24
    probe: wizard 4's army next to Cornwell walked off toward its rally point): an AI capture needs more setup.
  - (10/24, N247) Prefer a breakpoint at a CALLEE's entry to one on a line after calls into other overlays: entry
    stops are reliable, and at the entry the callee has not pushed BP yet, so the caller's frame ([bp+n] / [bp-n])
    and registers (SI / DI) are still readable - #187 read Attack_Resolve's attacker / defender / counter at
    Roll_To_Hit's entry, #149 AI_Pool_Add_Strongest's unit [bp-2] at AI_Unit_Strength's. d.call(fn, where, nargs)
    runs one to its return (args, AX); _caller(d, h) (cases_dbg3) names the caller from the return address.
  - Human_End_Turn compacts the unit table BEFORE Next_Turn: a dead record from the human's turn never reaches the AI
    turn (#137's first form traced a renumbered unit). Game_Start_Init zeroes every AI's average_unit_cost at load:
    AI_Disband_Obsolete_Units recomputes it only on a turn divisible by 25 (set game_turn, value=125).
  - GoTo_Build_Group groups only units with the lead's road_turns: engineers with different counters never share a
    Stack_Move (#469 latent).
  - The research book's last two pages are the Research / Spells candidate pages - ALWAYS: Spellbook_Fill_Group
    adds a page even for an empty group. Wizard_Learn_Spell makes the spell KNOWN before Screen_Spell_Learned and the
    research book lists every known spell, so the first search always finds it: the found == 0 branch (#475) is
    dead code in 1.31 (corrected 10/24 - an earlier note here said it ran when no candidate was left).
  - Wizards_Init_Spell_Lists makes only (books - 1) start picks known below 11 books: Wizard_Apply_Preset's one-late
    pick count has no effect (#354, NOT SHOWN on 30 worlds).
  - MG census cases read the 30 stored worlds; water = data/terrain_types.tsv's water flag (cases_mg._water).
    Hard / Impossible worlds: mapgen_survey.py N sizes diff=3 sub=hard (a subfolder the Intro-world cases ignore).
  - In a debugger session a key sent while a debugger stop holds the game is lost: d.start_combat resends the
    attack key while the map is still up (#326's second battle).
  - Static rows (cases_zst.py) for entries a scripted game cannot reach: generate them with tools/harness/bugsite.py
    (marker -> file, line, the function it heads or sits in) and check every title / function by hand.
  - (10/24, review) A verdict test must name the CALL it judges: a callee with two call sites in
    one caller (AI_Pool_Add_Strongest: the ranking loop passes ut - the bug - and the flyer pool passes unit) is
    told apart by its return offset (#149); a trace over every caller's calls (City_Site_Evaluate from all AI
    settlers) is filtered to the unit set up, read from the caller's frame at the callee's entry (#150: DI, [bp-56]).
  - Exclude the coincidences that cannot tell bug from fix (a unit whose index equals its type, #149; a spell id
    equal to its realm, #139) instead of letting them vote.
  - A random weighted pick proves nothing in one draw unless the setup leaves the bug's choice the ONLY weight
    (#241: every other attack spell made researchable). The AI picks an overland spell only while idle: casting
    skill 1000 (skill_points 1000 x 1001) and mana 30000 give a new pick every turn (cases_dbg1._fast_caster).
  - Entry of a function is not its effect: #158's war roll is proven by the Random(100) called from
    Diplo_Relation_Change (it runs only when msg_arg == 10), not by the call's arguments.
  - MG census: a later world-generation pass can rewrite the square judged - Place_Minerals' unwrapped sites (x
    60..64, y 40..45) turn forest into mountain / hills / swamp at x 0..4 of the next row (#359 leaves those out);
    stratify a pooled test by the factor that changes the odds (#364: Mantel-Haenszel per hero type).
  - mom.cast_spell resends S (up to 3 times) when the combat book has not opened: the key is lost under a
    debugger stop like the attack key (#222).
