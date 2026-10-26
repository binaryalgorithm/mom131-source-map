"""Test saves for harness checks: start from work/harness/SAVE9_orig.GAM (the base save: an autosave of the owner's game, turn 106), apply edits, write
the result as work/harness/game/SAVE9.GAM (the slot WIZARDS.EXE loads at start). Only the harness copy is written; the
game install and the original saves are never touched.  restore() puts the unedited copy back."""
import os
import shutil
import struct
import os as _os  # noqa: E402
import sys as _sys  # noqa: E402
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..'))
import mompaths as _mp  # noqa: E402  (ROOT / GAME / TOOLCHAIN / REF: tools/mompaths.py)

ROOT = _mp.ROOT
ORIG = ROOT + 'work/harness/SAVE9_orig.GAM'
GAME = ROOT + 'work/harness/game/SAVE9.GAM'
TER = 0x9e8 + 0x4c8 * 6
CITY = TER + 0x12c0 * 2 + 0xc0 * 2 + 0x960 * 2 + 0x30 * 30 + 24 * 2 + 0x18 * 102 + 0x32 * 138
UNIT = CITY + 0x72 * 100
UF = {'x': 0, 'y': 1, 'plane': 2, 'owner': 3, 'moves_max': 4, 'type': 5, 'finished': 7, 'moves_left': 8,
      'dest_x': 9, 'dest_y': 0xa, 'status': 0xb}


class Save:
    def __init__(self, path=ORIG):
        self.b = bytearray(open(path, 'rb').read())

    def num_units(self):
        return struct.unpack('<H', self.b[0x9d8 + 10:0x9d8 + 12])[0]

    def unit(self, i):
        o = UNIT + i * 0x20
        return {k: self.b[o + v] for k, v in UF.items()}

    def set_unit(self, i, **kv):
        o = UNIT + i * 0x20
        for k, v in kv.items():
            self.b[o + UF[k]] = v & 0xff

    def units_of(self, owner):
        return [i for i in range(self.num_units()) if self.b[UNIT + i * 0x20 + 3] == owner]

    def tile(self, x, y, p):
        return struct.unpack('<H', self.b[TER + p * 0x12c0 + y * 0x78 + x * 2:][:2])[0] % 0x2fa

    def write(self, path=GAME):
        open(path, 'wb').write(self.b)
        return path


def restore():
    shutil.copy(ORIG, GAME)


if __name__ == '__main__':
    s = Save()
    print('units', s.num_units(), 'human', s.units_of(0))
