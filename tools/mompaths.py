"""Where the regression harness finds things - no absolute paths in the code.
  ROOT          this repository (src/, BUGS_AND_QUIRKS.txt, tools/harness/ ...)
  WORK          ROOT/work/: everything generated - the game copies, the flattened executables' maps, results
                (git-ignored)
  GAME          WORK/game/: setup.py copies your game there - official_131/ (and community_patch_160/ when given)
  DOSBOX        the DOSBox-X executable for plain runs                      (setup.py --dosbox; env MOM_DOSBOX)
  DOSBOX_DEBUG  a DOSBox-X with the debugger channel's SAVESTATE / MEMREAD   (--dosbox-debug; env MOM_DOSBOX_DEBUG)
  BC            the folder holding BORLANDC (build/install_bc31.py's OUT)   (--bc; env MOM_BC; srcmap.py only)
setup.py stores its choices in work/paths.json; an environment variable overrides it. Every folder ends with '/'."""
import json
import os


def _dir(p):
    p = os.path.abspath(p).replace('\\', '/')
    return p if p.endswith('/') else p + '/'


ROOT = _dir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
WORK = ROOT + 'work/'
GAME = WORK + 'game/'
_saved = {}
if os.path.exists(WORK + 'paths.json'):
    _saved = json.load(open(WORK + 'paths.json'))
DOSBOX = (os.environ.get('MOM_DOSBOX') or _saved.get('dosbox') or '').replace('\\', '/')
DOSBOX_DEBUG = (os.environ.get('MOM_DOSBOX_DEBUG') or _saved.get('dosbox_debug') or DOSBOX).replace('\\', '/')
BC = _dir(os.environ.get('MOM_BC') or _saved.get('bc') or WORK + 'bc')


def win(p):
    """the same path with backslashes (DOSBox mount lines, Windows tools)"""
    return p.replace('/', '\\')


def render(template, out):
    """a DOSBox / tool config with @ROOT@ (forward slashes) and @ROOT_WIN@ (backslashes, no trailing one) filled
    in -> out (written); returns out"""
    t = open(template, encoding='latin-1').read()
    t = t.replace('@ROOT_WIN@', win(ROOT.rstrip('/'))).replace('@ROOT@', ROOT)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, 'w', encoding='latin-1').write(t)
    return out
