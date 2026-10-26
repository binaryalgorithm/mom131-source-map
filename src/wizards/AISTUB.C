/* AISTUB.C - WIZARDS.EXE overlay d50f: six empty functions: the overland and city screens call
   Overland_Frame_AI_Nop / City_Frame_AI_Nop once per frame; the *_Uncalled_Nop_<code offset> ones have no caller.
   Matching target: Borland C++ 3.1, BCC -c -mm -a.
   Check: python -P tools/match_build.py WIZARDS d50f src/wizards/AISTUB.C */

/* () empty; no caller */
void AI_Uncalled_Nop_0000(void)
{
}

/* () empty; no caller */
void AI_Uncalled_Nop_0005(void)
{
}

/* () empty; no caller */
void AI_Uncalled_Nop_000a(void)
{
}

/* () empty; the overland map loop calls it every frame */
void Overland_Frame_AI_Nop(void)
{
}

/* () empty; the city screen loop calls it every frame */
void City_Frame_AI_Nop(void)
{
}

/* () empty; no caller */
void AI_Uncalled_Nop_0019(void)
{
}
