"""Make the Miles AIL link stand-ins from YOUR copy of the game (Master of Magic v1.31).

Both executables link the Miles Design AIL 2.14 sound library. It is not Simtex code and its source is not part of
this repository, so the build links a stand-in instead: the library's code bytes taken from your own WIZARDS.EXE /
MAGIC.EXE, wrapped in an assembler file that gives each AIL function its public name. The templates in this folder
(ail_wizards.tpl, ail_magic.tpl) hold that wrapper - names, labels, the one relocated word - with every code byte left
out; this script fills the bytes in from your executables.

  python make_ail.py GAME_FOLDER OUT_FOLDER
      GAME_FOLDER  the folder with your v1.31 WIZARDS.EXE and MAGIC.EXE (GOG release; SHA-256 checked)
      OUT_FOLDER   where S2FC1.ASM (WIZARDS) and AIL.ASM (MAGIC) are written

Template lines: '@BYTES n' = the next n code bytes; '@SKIP n' = n bytes the next line writes itself (the relocated
word); '@HEADER ...' = where the code starts in the executable and its expected SHA-256. Everything else is copied."""
import hashlib
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TEMPLATES = (('ail_wizards.tpl', 'S2FC1.ASM'), ('ail_magic.tpl', 'AIL.ASM'))


def fill(tpl_path, game):
    data = open(tpl_path, 'rb').read()
    lines = data.splitlines(keepends=True)
    head = lines[0].decode('latin-1').split()
    assert head[0] == '@HEADER', tpl_path
    exe_name, start, want = head[1], int(head[2], 16), head[3]
    exe = open(os.path.join(game, exe_name), 'rb').read()
    have = hashlib.sha256(exe).hexdigest()
    if have != want:
        sys.exit('%s: SHA-256 %s - not the v1.31 file this build matches (%s)' % (exe_name, have, want))
    image = exe[struct.unpack('<H', exe[8:10])[0] * 16:]   # the load image: after the MZ header
    pos, out = start, []
    for raw in lines[1:]:
        text = raw.decode('latin-1')
        body = text.rstrip('\r\n')
        end = raw[len(body):]
        if body.startswith('@BYTES '):
            n = int(body.split()[1])
            out.append(('        db      ' + ', '.join('%03xh' % b for b in image[pos:pos + n])).encode('latin-1') + end)
            pos += n
        elif body.startswith('@SKIP '):
            pos += int(body.split()[1])
        else:
            out.append(raw)
    return exe_name, b''.join(out)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    game, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    for tpl, name in TEMPLATES:
        exe_name, text = fill(os.path.join(HERE, tpl), game)
        open(os.path.join(outdir, name), 'wb').write(text)
        print('%s from %s: %d bytes' % (name, exe_name, len(text)))


if __name__ == '__main__':
    main()
