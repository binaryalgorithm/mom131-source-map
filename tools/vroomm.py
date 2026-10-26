"""Borland VROOMM overlay analyser / de-overlayer for DOS MZ executables (Master of Magic: WIZARDS.EXE, MAGIC.EXE).
Format (worked out from the binary, 2026-10-15):
  file = MZ header | resident load image | "FBOV" u32 ovl_size u32 stub_table_ofs u32 count | overlay data (ovl_size)
  stub_table_ofs = load-image offset of the first stub. Stubs are paragraph-aligned segments in the resident image:
    +0  CD 3F            INT 3Fh (overlay manager)
    +2  u16              (runtime use)
    +4  u32 ovl_ofs      offset of this overlay's code in the overlay data
    +8  u16 code_size
    +A  u16 reloc_size   bytes of relocation list that follows the code (u16 offsets of segment words)
    +C  u16 n_entries
    +E..+1F              (runtime use)
    +20 n_entries x 5 bytes: CD 3F u16 target_offset 00   (thunk: INT 3Fh, offset of the function in the overlay)
SEGMENT WORDS INSIDE OVERLAY CODE are not paragraph numbers but byte offsets into Borland's SEGMENT TABLE: `count`
  (the FBOV header's last field) records of 8 bytes {u16 segment, u16 size, u16 type (1 code, 3 overlay stub, 4/5
  data..), u16 ?}, which sits just before the first stub. Real segment = table[word].segment (load-relative).
  Verified on WIZARDS.EXE: 12,790 far calls in overlay code -> 8,446 function prologues + 3,399 stub thunks + 945
  resident asm helpers (push ds/es, xor cx,cx, ...) = 100% sensible targets.
De-overlaying: append every overlay as a normal paragraph-aligned segment after the resident image, keep its segment
words (load-relative) and add MZ relocations for them, and rewrite each thunk to EA off16 seg16 (JMP FAR, 5 bytes) with
an MZ relocation on seg16. The result loads in Ghidra/Reko/IDA as one ordinary MZ program (analysis only, not runnable).
  python -I vroomm.py info  WIZARDS.EXE
  python -I vroomm.py flat  WIZARDS.EXE OUT.EXE [OUT_map.json]"""
import json
import struct
import sys


INT3F = bytes([0xCD, 0x3F])
PROLOGUE = bytes([0x55, 0x8B, 0xEC])     # push bp; mov bp, sp


class MZ:
    def __init__(self, path):
        self.path = path
        self.d = d = open(path, 'rb').read()
        assert d[:2] == b'MZ', path
        (self.cblp, self.cp, self.crlc, self.cparhdr, self.minalloc, self.maxalloc, self.ss, self.sp, self.csum,
         self.ip, self.cs, self.lfarlc, self.ovno) = struct.unpack('<13H', d[2:28])
        self.hdr = self.cparhdr * 16
        self.img_end = (self.cp - 1) * 512 + (self.cblp or 512)
        self.image = d[self.hdr:self.img_end]
        # MZ relocation entries are stored as (offset, segment); keep them as (segment, offset)
        self.relocs = [struct.unpack('<HH', d[self.lfarlc + 4 * i:self.lfarlc + 4 * i + 4])[::-1] for i in range(self.crlc)]
        fb = d[self.img_end:self.img_end + 16]
        assert fb[:4] == b'FBOV', f'no FBOV at {self.img_end:#x}'
        self.ovl_size, self.stub_ofs, self.count = struct.unpack('<III', fb[4:16])
        self.ovl = d[self.img_end + 16:self.img_end + 16 + self.ovl_size]
        assert len(self.ovl) == self.ovl_size
        self.stubs = self._parse_stubs()

    def _stub_at(self, p):
        """a well-formed stub at image offset p, or None"""
        im = self.image
        if im[p:p + 2] != INT3F or p + 0x20 > len(im):
            return None
        ovl_ofs, csz, rsz, ne = struct.unpack('<IHHH', im[p + 4:p + 14])
        if not (0 < csz and ne > 0 and ovl_ofs + csz + rsz <= self.ovl_size and rsz % 2 == 0):
            return None
        if p + 0x20 + 5 * ne > len(im):
            return None
        ents = []
        for i in range(ne):
            e = p + 0x20 + 5 * i
            if im[e:e + 2] != INT3F or im[e + 4] != 0:
                return None
            off = struct.unpack('<H', im[e + 2:e + 4])[0]
            if off >= csz:
                return None
            ents.append(off)
        code = self.ovl[ovl_ofs:ovl_ofs + csz]
        rel = list(struct.unpack(f'<{rsz // 2}H', self.ovl[ovl_ofs + csz:ovl_ofs + csz + rsz]))
        return dict(ofs=p, seg=p // 16, ovl_ofs=ovl_ofs, code_size=csz, reloc_size=rsz, entries=ents,
                    code=code, relocs=rel)

    def _parse_stubs(self):
        """stubs are paragraph-aligned segments scattered through the resident image (link order); scan them all"""
        stubs = []
        for p in range(0, len(self.image) - 0x20, 16):
            s = self._stub_at(p)
            if s:
                stubs.append(s)
        return stubs

    def find_segtable(self):
        """locate the segment table: just before the first stub; pick the start that makes the most overlay far calls
        land on a function prologue or a stub thunk"""
        ovs = self.overlays()
        im = self.image
        first = min(s['ofs'] for s in ovs)
        thunks = {(s['seg'], 0x20 + 5 * i) for s in ovs for i in range(len(s['entries']))}
        calls = [struct.unpack('<HH', s['code'][r - 2:r + 2]) for s in ovs for r in s['relocs']
                 if r >= 3 and s['code'][r - 3] == 0x9a][:3000]
        best = (-1, None)
        guess = first - 8 * self.count
        for t in range(max(0, guess - 256), min(len(im) - 8, guess + 64), 2):
            ok = 0
            for off, v in calls:
                if t + v + 2 > len(im):
                    continue
                seg = struct.unpack('<H', im[t + v:t + v + 2])[0]
                a = seg * 16 + off
                if (seg, off) in thunks or im[a:a + 3] == PROLOGUE:
                    ok += 1
            if ok > best[0]:
                best = (ok, t)
        self.segtable = best[1]
        self.segtable_score = (best[0], len(calls))
        return best[1]

    def seg_of(self, v):
        return struct.unpack('<H', self.image[self.segtable + v:self.segtable + v + 2])[0]

    def overlays(self):
        return [s for s in self.stubs if not s.get('header')]

    def check(self):
        ovs = self.overlays()
        problems = []
        # 1. overlay pieces tile the overlay data
        spans = sorted((s['ovl_ofs'], s['ovl_ofs'] + s['code_size'] + s['reloc_size']) for s in ovs)
        covered, pos, gaps = 0, 0, []
        for a, b in spans:
            if a > pos:
                gaps.append((pos, a))
            if a < pos:
                problems.append(f'overlap at {a:#x}')
            covered += b - a
            pos = max(pos, b)
        if pos < self.ovl_size:
            gaps.append((pos, self.ovl_size))
        # 2. entries inside code and on a function prologue
        prolog = sum(1 for s in ovs for e in s['entries'] if s['code'][e:e + 3] == b'\x55\x8b\xec')
        nent = sum(len(s['entries']) for s in ovs)
        bad = [(s['seg'], e) for s in ovs for e in s['entries'] if e >= s['code_size']]
        # 3. relocations inside code
        badrel = sum(1 for s in ovs for r in s['relocs'] if r + 2 > s['code_size'])
        # 4. resident far calls (9A off seg) that hit a stub entry
        stub_entry = {(s['seg'], 0x20 + 5 * i) for s in ovs for i in range(len(s['entries']))}
        reloc_sites = {seg * 16 + off for seg, off in self.relocs}
        far_hits = 0
        im = self.image
        for site in reloc_sites:
            if site >= 3 and im[site - 3] == 0x9a:
                off, seg = struct.unpack('<HH', im[site - 2:site + 2])
                if (seg, off) in stub_entry:
                    far_hits += 1
        return dict(overlays=len(ovs), header_records=len(self.stubs) - len(ovs), count_field=self.count,
                    entries=nent, entries_on_prologue=prolog, entries_outside_code=len(bad),
                    relocs=sum(len(s['relocs']) for s in ovs), relocs_outside_code=badrel,
                    ovl_bytes=self.ovl_size, covered=covered, gaps=gaps[:10], problems=problems[:10],
                    resident_far_calls_into_stubs=far_hits,
                    code_bytes=sum(s['code_size'] for s in ovs))

    def flatten(self):
        """resident image + overlays as normal segments; thunks -> JMP FAR; returns (exe_bytes, map)."""
        img = bytearray(self.image)
        while len(img) % 16:
            img.append(0)
        relocs = list(self.relocs)
        omap = []
        ovs = self.overlays()
        if getattr(self, 'segtable', None) is None:
            self.find_segtable()
        # where each overlay will land, so calls through thunks can be pointed straight at the function
        pos = len(img)
        new_seg_of = {}
        for s in ovs:
            new_seg_of[s['seg']] = pos // 16
            pos += (s['code_size'] + 15) // 16 * 16
        thunk_target = {(s['seg'], 0x20 + 5 * i): (new_seg_of[s['seg']], e)
                        for s in ovs for i, e in enumerate(s['entries'])}
        retargeted = 0
        # resident far calls into thunks -> direct
        for seg, off in self.relocs:
            site = seg * 16 + off
            if site >= 3 and img[site - 3] == 0x9a:
                o, sg = struct.unpack('<HH', img[site - 2:site + 2])
                if (sg, o) in thunk_target:
                    nsg, no = thunk_target[(sg, o)]
                    img[site - 2:site + 2] = struct.pack('<HH', no, nsg)
                    retargeted += 1
        for s in ovs:
            new_seg = new_seg_of[s['seg']]
            assert new_seg == len(img) // 16
            code = bytearray(s['code'])
            for r in s['relocs']:
                real = self.seg_of(struct.unpack('<H', code[r:r + 2])[0])      # table offset -> real segment
                code[r:r + 2] = struct.pack('<H', real)
                if r >= 3 and code[r - 3] == 0x9a:
                    o = struct.unpack('<H', code[r - 2:r])[0]
                    if (real, o) in thunk_target:
                        nsg, no = thunk_target[(real, o)]
                        code[r - 2:r + 2] = struct.pack('<HH', no, nsg)
                        retargeted += 1
                relocs.append((new_seg, r))            # segment word is load-relative; the loader adds the base
            img += code
            while len(img) % 16:
                img.append(0)
            for i, e in enumerate(s['entries']):
                t = s['ofs'] + 0x20 + 5 * i
                img[t:t + 5] = b'\xea' + struct.pack('<HH', e, new_seg)
                relocs.append((s['seg'], 0x20 + 5 * i + 3))
            omap.append(dict(stub_seg=s['seg'], ovl_seg=new_seg, ovl_ofs=s['ovl_ofs'], code_size=s['code_size'],
                             entries=[dict(thunk=f'{s["seg"]:04X}:{0x20 + 5 * i:04X}', target=f'{new_seg:04X}:{e:04X}')
                                      for i, e in enumerate(s['entries'])]))
        # build a fresh MZ: header (28 bytes + relocations), paragraph aligned
        hdr_len = 28 + 4 * len(relocs)
        hdr_len = (hdr_len + 15) // 16 * 16
        total = hdr_len + len(img)
        cp, cblp = (total + 511) // 512, total % 512
        extra_paras = 0x100
        h = struct.pack('<2s13H', b'MZ', cblp, cp, len(relocs), hdr_len // 16, extra_paras, 0xFFFF,
                        self.ss, self.sp, 0, self.ip, self.cs, 28, 0)
        h += b''.join(struct.pack('<HH', off, seg) for seg, off in
                      ((seg, off) for (seg, off) in relocs))
        h = h.ljust(hdr_len, b'\0')
        return h + bytes(img), dict(source=self.path, resident_paras=len(self.image) // 16, segtable=self.segtable,
                                    calls_retargeted_to_overlay=retargeted, overlays=omap,
                                    relocs_resident=len(self.relocs), relocs_total=len(relocs))


def main():
    cmd, path = sys.argv[1], sys.argv[2]
    m = MZ(path)
    if cmd == 'info':
        print(f'{path}: header {m.hdr:#x}, resident image {len(m.image)} B, relocs {m.crlc}, CS:IP {m.cs:04X}:{m.ip:04X}')
        print(f'  FBOV: overlay data {m.ovl_size} B, stub table at image {m.stub_ofs:#x}, count field {m.count}')
        t = m.find_segtable()
        print(f'  segment table at image {t:#x} ({m.count} x 8 B); far-call fit {m.segtable_score[0]}/{m.segtable_score[1]}')
        for k, v in m.check().items():
            print(f'  {k}: {v}')
        ov = m.overlays()
        print('  first overlays:', [(f"{s['seg']:04X}", s['code_size'], len(s['entries'])) for s in ov[:6]])
        print('  largest:', sorted(((s['code_size'], f"{s['seg']:04X}") for s in ov), reverse=True)[:5])
    elif cmd == 'flat':
        exe, mp = m.flatten()
        open(sys.argv[3], 'wb').write(exe)
        mp_path = sys.argv[4] if len(sys.argv) > 4 else sys.argv[3].rsplit('.', 1)[0] + '_map.json'
        json.dump(mp, open(mp_path, 'w'), indent=1)
        print(f'wrote {sys.argv[3]} ({len(exe)} B, {mp["relocs_total"]} relocs, {len(mp["overlays"])} overlay segments) '
              f'and {mp_path}')


if __name__ == '__main__':
    main()
