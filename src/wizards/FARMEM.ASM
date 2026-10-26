; FARMEM.ASM - WIZARDS.EXE code segment 14f8 (MAGIC.EXE 1466, the same file): DOS memory blocks and byte / word /
; dword access to far memory by segment and offset. Hand-written assembly (Borland TASM).
; Far_Block_Alloc writes a 4-word header 12FAh 4ECFh paras+1 1 at offset 0 of the block, but Heap_Block_Valid looks
; for the signature at offset 4 - the two do not describe the same header (the heap blocks of HEAP.C carry it there).
; "; C:" lines = the same behaviour as C (Borland C 3.1 library names) - a reference for a port, not compiled.
; Matching target: TASM from the Borland C++ 3.1 tree.
; Check: python -P tools/match_build.py WIZARDS 14f8 src/wizards/FARMEM.ASM
        .MODEL  MEDIUM, C
        LOCALS
        .CODE
        PUBLIC  Far_Block_Alloc, Far_Block_Alloc_Raw, Far_Block_Free_Header, Heap_Block_Valid
        PUBLIC  Far_Peek_Byte, Far_Peek_Word, Far_Peek_DWord, Far_Poke_Byte, Far_Poke_Word, Far_Ptr_Store

; (paras) -> segment of a new DOS block of paras + 1 paragraphs with a header, 0 if DOS has no memory
; C:  unsigned Far_Block_Alloc(unsigned paras) { unsigned s; unsigned far *h;
; C:      if (allocmem(paras + 1, &s) != -1) return 0;            (allocmem: -1 = success)
; C:      h = MK_FP(s, 0); h[0] = 0x12FA; h[1] = 0x4ECF; h[2] = paras + 1; h[3] = 1; return s; }
Far_Block_Alloc PROC USES si di es ds, paras:WORD
        mov     bx, paras
        inc     bx
        mov     ah, 48h
        int     21h
        jb      @@fail
        mov     es, ax
        mov     di, 0
        mov     ax, 12FAh
        stosw
        mov     ax, 4ECFh
        stosw
        mov     ax, paras
        inc     ax
        stosw
        mov     ax, 1
        stosw
        mov     ax, es
        ret
@@fail:
        mov     ax, 0
        ret
Far_Block_Alloc ENDP

; (paras) -> segment of a new DOS block, no header; 0 if DOS has no memory
; C:  unsigned Far_Block_Alloc_Raw(unsigned paras) { unsigned s; return allocmem(paras, &s) == -1 ? s : 0; }
Far_Block_Alloc_Raw PROC USES si di es ds, paras:WORD
        mov     bx, paras
        mov     ah, 48h
        int     21h
        jb      @@fail
        ret
@@fail:
        mov     ax, 0
        ret
Far_Block_Alloc_Raw ENDP

; (seg) clears the first word of the paragraph before seg and frees that DOS block. No callers.
; C:  void Far_Block_Free_Header(unsigned block) { *(unsigned far *)MK_FP(block - 1, 0) = 0; freemem(block - 1); }
Far_Block_Free_Header PROC USES si di es, block:WORD
        mov     bx, block
        dec     bx
        mov     es, bx
        mov     bx, 0
        mov     word ptr es:[bx], 0
        mov     ah, 49h
        int     21h
        ret
Far_Block_Free_Header ENDP

; (seg) -> -1 if the words at seg:4 are the heap signature 12FAh 4ECFh, else 0
; C:  int Heap_Block_Valid(unsigned block) { unsigned far *h = MK_FP(block, 4);
; C:      return h[0] == 0x12FA && h[1] == 0x4ECF ? -1 : 0; }
Heap_Block_Valid PROC USES ds si, block:WORD
        mov     ax, block
        mov     ds, ax
        mov     si, 4
        lodsw
        cmp     ax, 12FAh
        jne     @@no
        lodsw
        cmp     ax, 4ECFh
        jne     @@no
        mov     ax, 0FFFFh
        ret
@@no:
        mov     ax, 0
        ret
Heap_Block_Valid ENDP

; (seg, off) -> the byte at seg:off
; C:  unsigned Far_Peek_Byte(unsigned seg, unsigned off) { return *(unsigned char far *)MK_FP(seg, off); }
Far_Peek_Byte PROC USES ds si, sgm:WORD, off:WORD
        mov     si, off
        mov     ax, sgm
        mov     ds, ax
        lodsb
        xor     ah, ah
        ret
Far_Peek_Byte ENDP

; (seg, off) -> the word at seg:off
; C:  unsigned Far_Peek_Word(unsigned seg, unsigned off) { return *(unsigned far *)MK_FP(seg, off); }
Far_Peek_Word PROC USES ds si, sgm:WORD, off:WORD
        mov     si, off
        mov     ax, sgm
        mov     ds, ax
        lodsw
        ret
Far_Peek_Word ENDP

; (seg, off) -> the dword at seg:off in DX:AX
; C:  long Far_Peek_DWord(unsigned seg, unsigned off) { return *(long far *)MK_FP(seg, off); }
Far_Peek_DWord PROC USES ds si, sgm:WORD, off:WORD
        mov     si, off
        mov     ax, sgm
        mov     ds, ax
        lodsw
        mov     bx, ax
        lodsw
        mov     dx, ax
        mov     ax, bx
        ret
Far_Peek_DWord ENDP

; (seg, off, value) stores the byte
; C:  void Far_Poke_Byte(unsigned seg, unsigned off, int value) { *(unsigned char far *)MK_FP(seg, off) = value; }
Far_Poke_Byte PROC USES es di, sgm:WORD, off:WORD, value:WORD
        mov     ax, sgm
        mov     es, ax
        mov     di, off
        mov     ax, value
        stosb
        ret
Far_Poke_Byte ENDP

; (seg, off, value) stores the word
; C:  void Far_Poke_Word(unsigned seg, unsigned off, unsigned value) { *(unsigned far *)MK_FP(seg, off) = value; }
Far_Poke_Word PROC USES es di, sgm:WORD, off:WORD, value:WORD
        mov     ax, sgm
        mov     es, ax
        mov     di, off
        mov     ax, value
        stosw
        ret
Far_Poke_Word ENDP

; (seg, off, lo, hi) stores the dword (a far pointer: offset, segment)
; C:  void Far_Ptr_Store(unsigned seg, unsigned off, unsigned lo, unsigned hi)
; C:      { unsigned far *p = MK_FP(seg, off); p[0] = lo; p[1] = hi; }
Far_Ptr_Store PROC USES es di, sgm:WORD, off:WORD, lo:WORD, hi:WORD
        mov     ax, sgm
        mov     es, ax
        mov     di, off
        mov     ax, lo
        stosw
        mov     ax, hi
        stosw
        ret
Far_Ptr_Store ENDP

        END
