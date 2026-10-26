/* MAPSTUB.C - WIZARDS.EXE overlay 87a9 (stub segment 252a): three empty functions; Game_Session_Init calls the second,
   Screen_Overland_Map the third; an overlay of their own between HALLFAME.C's and
   NOTIFY.C's. File name ours (the original's is unknown): named for its callers.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d.
   Check: python -P tools/match_build.py WIZARDS 87a9 src/wizards/MAPSTUB.C */

/* () empty; no caller */
void Map_Uncalled_Nop_0bf0(void)
{
}

/* () empty; Game_Session_Init calls it */
void Load_Init_Map_Nop(void)
{
}

/* () empty; the overland map loop calls it every frame */
void Overland_Frame_Map_Nop(void)
{
}
