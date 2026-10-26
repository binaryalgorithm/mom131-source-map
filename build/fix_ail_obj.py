"""Put the WIZARDS AIL stand-in's code fixups in the order the original link had (step 2b of the build).

TLINK writes an executable's relocation entries in the order of the object files' fixups. Borland C wrote the
original objects' fixups in its own order (LEDATA records of up to 0x400 bytes, fixups descending inside each); TASM,
which assembles the stand-in, writes them ascending. This re-emits the stand-in segment's LEDATA / FIXUPP records so
that its relocated word comes out where the original had it: first the bytes no fixup touches, as plain LEDATA
records, then each relocated word, in the wanted order, as a small LEDATA + a one-fixup FIXUPP. Other records keep
their place; checksums are recomputed.

  python fix_ail_obj.py S2FC1.OBJ          (after TASM /ml S2FC1.ASM, before TLINK)
  python fix_ail_obj.py OBJ SEGMENT WORDS  (general form: segment name, comma-separated hex word offsets)"""
import struct
import sys


def index(d, i):
    """an OMF index field at d[i] -> (value, next position)"""
    v = d[i]
    if v & 0x80:
        return ((v & 0x7f) << 8) | d[i + 1], i + 2
    return v, i + 1


CHUNK = 0x400


def record(t, body):
    rec = bytes([t]) + struct.pack('<H', len(body) + 1) + body
    return rec + bytes([(-sum(rec)) & 0xff])


def subrecords(d):
    """FIXUPP body -> [(loc_type, data_offset, rest_bytes)] (fix data + datums, raw)"""
    out, i = [], 0
    while i < len(d):
        b0 = d[i]
        if not b0 & 0x80:
            raise SystemExit('thread subrecord - not supported')
        loc, off = (b0 >> 2) & 0x0f, ((b0 & 3) << 8) | d[i + 1]
        m = b0 & 0x40
        j = i + 2
        fd = d[j]
        j += 1
        if not fd & 0x80 and (fd >> 4) & 7 < 3:              # frame datum
            _, j = index(d, j)
        if not fd & 0x08:                                      # target datum
            _, j = index(d, j)
        if not fd & 0x04:                                      # displacement
            j += 2
        out.append((loc, off, m, d[i + 2:j]))
        i = j
    return out


def reorder(path, segname, wanted):
    b = open(path, 'rb').read()
    recs, pos, names, segs = [], 0, [''], ['']
    while pos < len(b):
        t = b[pos]
        n = struct.unpack_from('<H', b, pos + 1)[0]
        recs.append((t, b[pos + 3:pos + 3 + n - 1], b[pos:pos + 3 + n]))
        pos += 3 + n
    for t, d, raw in recs:
        if t == 0x96:
            k = 0
            while k < len(d):
                names.append(d[k + 1:k + 1 + d[k]].decode('latin-1'))
                k += 1 + d[k]
        elif t == 0x98:
            ni, _ = index(d, 1 + (3 if d[0] >> 5 == 0 else 0) + 2)
            segs.append(names[ni])
    if segname not in segs:
        raise SystemExit('%s: no segment %s' % (path, segname))
    si = segs.index(segname)
    # the segment's bytes and fixups
    img, fix, out, at, cur = {}, [], [], None, None
    for t, d, raw in recs:
        if t == 0xa0:
            s, j = index(d, 0)
            cur = (s, struct.unpack_from('<H', d, j)[0]) if s == si else None
            if cur:
                for k, x in enumerate(d[j + 2:]):
                    img[cur[1] + k] = x
                at = len(out) if at is None else at
                continue
        elif t == 0x9c and cur:
            for loc, off, m, rest in subrecords(d):
                fix.append((loc, cur[1] + off, m, rest))
            continue
        elif t not in (0x9c, 0x88, 0x8a, 0x8b):
            cur = None
        out.append(raw)
    size = {1: 2, 2: 2, 3: 4, 5: 2, 0: 1, 4: 1}
    word = {}
    for f in fix:
        loc, a = f[0], f[1]
        w = a + 2 if loc == 3 else a
        if w in word:
            raise SystemExit('%s: two fixups on word %x' % (path, w))
        word[w] = f
    # a fixup that makes no relocation (a far call to the same segment, turned near with no slot) is not in the list:
    # its place does not matter - emitted first, ascending
    extra = sorted(set(word) - set(wanted))
    absent = [w for w in wanted if w not in word]
    if absent:
        raise SystemExit('%s: wanted words without a fixup %s' % (path, [hex(x) for x in absent[:8]]))
    # bytes per fixup record (far call: with its 9a)
    owned, groups = set(), []
    for w in extra + list(wanted):
        loc, a, m, rest = word[w]
        lo, hi = a, a + size[loc]
        owned |= set(range(lo, hi))
        groups.append([lo, hi, loc, m, rest])
    for g in groups:
        if g[2] == 3 and img.get(g[0] - 1) == 0x9a and g[0] - 1 not in owned:
            g[0] -= 1
            owned.add(g[0])
    new = []
    plain = sorted(a for a in img if a not in owned)
    k = 0
    while k < len(plain):
        e = k
        while e + 1 < len(plain) and plain[e + 1] == plain[e] + 1 and plain[e + 1] - plain[k] < CHUNK:
            e += 1
        new.append(record(0xa0, omf_match_index(si) + struct.pack('<H', plain[k]) +
                          bytes(img[a] for a in range(plain[k], plain[e] + 1))))
        k = e + 1
    for lo, hi, loc, m, rest in groups:
        a = lo if not (loc == 3 and hi - lo == 5) else lo + 1
        rel = a - lo
        new.append(record(0xa0, omf_match_index(si) + struct.pack('<H', lo) + bytes(img[x] for x in range(lo, hi))))
        new.append(record(0x9c, bytes([0x80 | m | (loc << 2) | (rel >> 8), rel & 0xff]) + rest))
    out[at:at] = new
    open(path, 'wb').write(b''.join(out))
    return len(groups)


def omf_match_index(i):
    return bytes([i]) if i < 0x80 else bytes([0x80 | (i >> 8), i & 0xff])


if __name__ == '__main__':
    if len(sys.argv) == 2:                     # the build's one case: WIZARDS' AIL stand-in, its one relocated word
        print(reorder(sys.argv[1], 'S2FC1_TEXT', [0xb55]))
    else:
        print(reorder(sys.argv[1], sys.argv[2], [int(x, 16) for x in sys.argv[3].split(',') if x]))
