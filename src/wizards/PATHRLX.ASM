; PATHRLX.ASM - WIZARDS.EXE overlay code segment d511 (stub 2621): Path_Relax, the overland move-cost relaxation
; behind the path finder - a Bellman-Ford style sweep over a 60 x 40 map window, in hand-written assembly (Borland
; TASM). The work block (segment map_seg) holds the move cost of every square (0, 2400 bytes, FFh = impassable), the
; best cost found so far (960h, 2400 bytes, FFh = not reached) and every square's predecessor (12C0h, 2400 words: the
; cost offset of the square it is reached from). The start square gets cost 0; then sweeps run until nothing changes:
; from the start row DOWN, then from the start row again UP - each stopping after a row with no passable square -
; each row left to right, each passable square taking the cheapest of its 8 neighbours (cost of the neighbour + its
; own cost; a byte add that overflows is ignored). Columns wrap east-west (x 0 and 59 are neighbours).
; QUIRK #405: the down sweep stops after row 38 (si >= 910h) and the up sweep after row 1 (si < 60), so rows 39 and
; 0 are never relaxed unless the start square is in them.
; Two values IN THE CODE SEGMENT at the module start (d511:0000): the changed flag (byte) and the start row's offset.
; "; C:" lines = the same behaviour as C (Borland C 3.1) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS d511 src/wizards/PATHRLX.ASM
        .MODEL  MEDIUM, C
        LOCALS
        .CODE
        PUBLIC  Path_Relax
changed_cs      db      1               ; d511:0000  1 = a sweep improved some square
                db      0               ; d511:0001  (unused)
row_ofs_cs      dw      0               ; d511:0002  y * 60 of the start square

; RELAX n: the square at [bx] (its cost is DL) takes neighbour bx + n if that is cheaper; its predecessor word
; (at [di]) becomes the neighbour's cost offset (SI is already one past the square)
; C:  nc = best[cell + n] + cost; if (no byte overflow && nc < best[cell]) { best[cell] = nc; prev[cell] = cell + n;
; C:      changed = 1; }
RELAX   MACRO   n
        LOCAL   next
        mov     al, [bx+n]
        add     al, dl
        jb      next
        cmp     al, [bx]
        jae     next
        mov     [bx], al
        mov     ax, si
        add     ax, n-1
        stosw
        dec     di
        dec     di
        mov     changed_cs, 1
next:
        ENDM

; CELL n1..n8: one square - skipped if impassable (FFh), else counted in CH and relaxed from its 8 neighbours; then
; the next square (BX best cost, DI predecessor; SI moved on by lodsb)
CELL    MACRO   n1, n2, n3, n4, n5, n6, n7, n8
        LOCAL   ok, skip
        lodsb
        cmp     al, 0FFh
        jne     ok
        jmp     skip
ok:     inc     ch
        mov     dl, al
        RELAX   n1
        RELAX   n2
        RELAX   n3
        RELAX   n4
        RELAX   n5
        RELAX   n6
        RELAX   n7
        RELAX   n8
skip:   inc     bx
        inc     di
        inc     di
        ENDM

; ROW: one row left to right - column 0 (west neighbours wrap to column 59), 58 inner columns in pairs, column 59
; (east neighbours wrap to column 0)
ROW     MACRO
        LOCAL   pair, pairs_done
        CELL    -1, -60, -59, 59, 1, 119, 60, 61
        mov     cl, 29
pair:   CELL    -61, -60, -59, -1, 1, 59, 60, 61
        CELL    -61, -60, -59, -1, 1, 59, 60, 61
        dec     cl
        je      pairs_done
        jmp     pair
pairs_done:
        CELL    -61, -60, -119, -1, -59, 59, 60, 1
        ENDM

; (x, y, map, map_seg) relaxes the work block from start square (x, y) until no sweep changes anything (map's offset
; is not used: the block starts at map_seg:0000)
; C:  void Path_Relax(int x, int y, struct PathWork far *work)
; C:  { best[] = FFh; prev[i] = i; best[y * 60 + x] = 0;
; C:    do { changed = 0; rows y..38 (stop after a row with no passable square); rows y..1 upward (same) } while (changed); }
Path_Relax PROC USES es di si ds, x:WORD, y:WORD, map:WORD, map_seg:WORD
        mov     ax, y
        mov     bx, 60
        mul     bx
        mov     row_ofs_cs, ax
        add     ax, x
        mov     dx, ax
        mov     ax, map_seg
        mov     ds, ax
        mov     es, ax
        mov     di, 960h
        mov     cx, 4B0h
        mov     ax, 0FFFFh
        rep     stosw
        mov     di, 12C0h
        mov     cx, 960h
        mov     ax, 0
@@prev: stosw
        inc     ax
        loop    @@prev
        mov     di, dx
        add     di, 960h
        mov     ax, 0
        stosb
@@again:
        mov     changed_cs, 0
        mov     si, 0
        mov     bx, 960h
        mov     di, 12C0h
        add     si, row_ofs_cs
        add     bx, row_ofs_cs
        add     di, row_ofs_cs
        add     di, row_ofs_cs
@@down: mov     ch, 0
        ROW
        cmp     ch, 0
        je      @@up_start
        cmp     si, 910h
        jns     @@up_start
        jmp     @@down
@@up_start:
        mov     si, 0
        mov     bx, 960h
        mov     di, 12C0h
        add     si, row_ofs_cs
        add     bx, row_ofs_cs
        add     di, row_ofs_cs
        add     di, row_ofs_cs
@@up:   mov     ch, 0
        ROW
        sub     si, 120
        sub     bx, 120
        sub     di, 240
        cmp     ch, 0
        je      @@sweep_done
        cmp     si, 60
        js      @@sweep_done
        jmp     @@up
@@sweep_done:
        cmp     changed_cs, 1
        jne     @@done
        jmp     @@again
@@done:
        ret
Path_Relax ENDP
        END
