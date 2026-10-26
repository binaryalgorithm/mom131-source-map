/* AUTOSAVE.C - WIZARDS.EXE code segment c74a: the every-4th-turn autosave to SAVE9.GAM (SAVE_SLOT_CONTINUE, the continue save).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS c74a src/wizards/AUTOSAVE.C */
#include "GAME.H"

extern int game_turn;                           /* DS:bd90 */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int autosave_unused_flag;                                   /* DS:ca74  MAGIC DS:904c   */

int autosave_unused = 0;                        /* MAGIC DS:34c2  no reader; the module's only data (link test, batch 36) */

void Save_Game(int slot);

/* () every 4th turn: saves to slot 8 (SAVE9.GAM, the autosave) under the busy pointer; 0 on other turns,
   else no return value
   Numbered: BUG #224 (BUGS_AND_QUIRKS.txt) */
int Autosave(void)
{
    struct MouseRegion region;

    if (game_turn % 4 != 0)
        return 0;
    region.image = POINTER_BUSY;                /* as mouse_region_busy */
    region.pointer_offset = 0;
    region.x1 = 0;
    region.y1 = 0;
    region.x2 = 319;
    region.y2 = 199;
    Mouse_Set_Cursor_Regions(1, &region);
    Save_Game(SAVE_SLOT_CONTINUE);
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    autosave_unused_flag = 0;
}
