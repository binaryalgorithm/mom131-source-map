/* TRANSIT.C - WIZARDS.EXE code segment 1fad: screen transitions - reveal the back page onto the shown page in
   columns, blinds or shrinking square rings, and four effect loops over the VGA effect copies of GFX (1edd) that
   nothing calls. No data. (The module starts at 1fad:0009, after GFX's last byte.)
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1fad src/wizards/TRANSIT.C */

void VGA_Effect_Column_Copy(int x, int y);
void VGA_Reveal_Line(int x1, int y1, int x2, int y2, int page);
void VGA_Interlace_Shift(void);
void VGA_Interlace_Fill_Edges(int step);
void VGA_Interlace_Shift_Band(int offset, int rows);
void VGA_Interlace_Fill_Band_Edges(int first, int offset, int rows);
void VGA_Push_Left_Step(int step);
void VGA_Push_Right_Step(int step);
void Fill_Rect(int x1, int y1, int x2, int y2, int color);
void Page_Flip(void);
int Mouse_X(void);
int Mouse_Y(void);
void Mouse_Save_Under(int x, int y);
void Transition_Block_Step(int x, int y, int step);

/* (x, y): the column copy effect from (x, y), then black left of x and above y (CitySpell_Cast_Anim,
   Natures_Wrath_Strike) */
void Transition_Wipe_Left(int x, int y)
{
    VGA_Effect_Column_Copy(x, y);
    if (x != 0)
        Fill_Rect(0, 0, x - 1, 199, 0);
    if (y != 0)
        Fill_Rect(0, 0, 319, y - 1, 0);
}

/* horizontal blinds: 20 passes, each reveals one row in every 20 from the back page, then re-saves the screen under
   the mouse cursor; no callers */
void Transition_Blinds(void)
{
    int pass;
    int y;

    for (pass = 19; pass >= 0; pass--)
        for (y = 0; y < 200; y = y + 20)
            VGA_Reveal_Line(0, y + pass, 319, y + pass, 0);
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
}

/* reveal in 20 x 20 blocks: 10 steps of shrinking square rings over the whole screen; no callers */
void Transition_Blocks(void)
{
    int x;
    int y;
    int step;

    for (step = 0; step < 10; step++)
        for (x = 0; x < 320; x += 20)
            for (y = 0; y < 200; y += 20)
                Transition_Block_Step(x, y, step);
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
}

/* (x, y, step): reveals the square ring (x+step, y+step)-(x+19-step, y+19-step) of a 20 x 20 block */
void Transition_Block_Step(int x, int y, int step)
{
    int x2;
    int y2;

    x2 = x + 19;
    y2 = y + 19;
    x += step;
    y += step;
    x2 -= step;
    y2 -= step;
    VGA_Reveal_Line(x, y, x2, y, 0);
    VGA_Reveal_Line(x, y, x, y2, 0);
    VGA_Reveal_Line(x, y2, x2, y2, 0);
    VGA_Reveal_Line(x2, y, x2, y2, 0);
}

/* DEAD (no callers): 40 rounds of VGA_Interlace_Shift + VGA_Interlace_Fill_Edges, then Page_Flip */
void Transition_Interlace_Push(void)
{
    int i;

    for (i = 0; i < 40; i++) {
        VGA_Interlace_Shift();
        VGA_Interlace_Fill_Edges(i);
    }
    Page_Flip();
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
}

/* DEAD (no callers): 139 rounds of VGA_Interlace_Shift_Band + VGA_Interlace_Fill_Band_Edges over a band that widens to 40 rows, slides, then narrows */
void Transition_Interlace_Band_Push(void)
{
    int i;
    int row;
    int rows;
    int first;

    for (i = 0; i < 139; i++) {
        if (i < 40) {
            row = 0;
            rows = i + 1;
            first = i;
        } else {
            first = 39;
            row = i - 39;
            if (row < 60)
                rows = 40;
            else
                rows = 100 - row;
        }
        VGA_Interlace_Shift_Band(row * 160, rows);
        VGA_Interlace_Fill_Band_Edges(first, row * 160, rows);
    }
    Page_Flip();
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
}

/* DEAD (no callers): 40 rounds of VGA_Push_Left_Step, then Page_Flip */
void Transition_Push_Left(void)
{
    int i;

    for (i = 0; i < 40; i++)
        VGA_Push_Left_Step(i);
    Page_Flip();
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
}

/* DEAD (no callers): 40 rounds of VGA_Push_Right_Step, then Page_Flip */
void Transition_Push_Right(void)
{
    int i;

    for (i = 0; i < 40; i++)
        VGA_Push_Right_Step(i);
    Page_Flip();
    Mouse_Save_Under(Mouse_X(), Mouse_Y());
}
