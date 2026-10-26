/* LINEPATH.C - WIZARDS.EXE code segment 1df0 (from 1df0:0002): map distances and Bresenham tile paths.
   The path step (a 0.16 fixed-point fraction) is INLINE ASSEMBLY in the original (MOV AX,0 / DIV BX / ADD + JAE, which BCC never emits
   from C); BCC hands such a file to TASM.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 1df0 src/wizards/LINEPATH.C */

int Atan2_Deg(int dx, int dy);
int Cos_Mul(int angle, int value);
int Sin_Mul(int angle, int value);

/* (&x, &y, tx, ty, speed) moves the point speed pixels toward the target along the angle between them
   (Atan2 on half the distances when either exceeds 255), never past it; nothing for speed <= 0 or when
   there. No return value */
int Step_Toward(int *x, int *y, int tx, int ty, int speed)
{
    int dx;
    int dy;
    int angle;

    if (speed <= 0 || (*x == tx && *y == ty))
        return 0;
    dx = tx - *x;
    dy = ty - *y;
    if (abs(dx) > 255 || abs(dy) > 255)
        angle = Atan2_Deg(dx / 2, dy / 2);
    else
        angle = Atan2_Deg(dx, dy);
    if (*x < tx) {
        *x += Cos_Mul(angle, speed);
        if (*x > tx)
            *x = tx;
    } else {
        *x += Cos_Mul(angle, speed);
        if (*x < tx)
            *x = tx;
    }
    if (*y < ty) {
        *y += Sin_Mul(angle, speed);
        if (*y > ty)
            *y = ty;
    } else {
        *y += Sin_Mul(angle, speed);
        if (*y < ty)
            *y = ty;
    }
}

/* (x, y, tx, ty) -> the approximate distance max(|dx|, |dy|) + min(|dx|, |dy|) / 2 (no map wrap).
   Simtex name Range (MoO2 symbols, module special); clarified using ReMoM repo */
int Dist_Max_Plus_Half_Min(int x, int y, int tx, int ty)
{
    int dx;
    int dy;
    int dist;

    dx = tx - x;
    dy = ty - y;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx > dy)
        dist = dx + dy / 2;
    else
        dist = dy + dx / 2;
    return dist;
}

/* (x1, y1, x2, y2, width) -> the larger of |dy| and |dx|, dx the shorter way around a map width wide.
   Simtex name Delta_XY_With_Wrap (MoO2 symbols, module special); clarified using ReMoM repo */
int Map_Distance(int x1, int y1, int x2, int y2, int width)
{
    int dx;
    int dy;
    int wx;

    dx = x2 - x1;
    dy = y2 - y1;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (x2 < x1)
        wx = x2 + width - x1;
    else
        wx = x2 - width - x1;
    if (wx < 0)
        wx = -wx;
    if (dx > wx)
        dx = wx;
    if (dx > dy)
        return dx;
    return dy;
}

/* (x1, y1, x2, y2, xs, ys) -> n: the squares of a straight line after the start (16-bit fixed-point
   stepping along the longer axis; a diagonal for |dx| = |dy|) into xs / ys.
   Simtex name Path (MoO2 symbols, module special); clarified using ReMoM repo */
int Line_Path_No_Wrap(int x1, int y1, int x2, int y2, char *xs, char *ys)
{
    int ddx;
    int ddy;
    int sx;
    int sy;
    int x;
    int y;
    unsigned step;
    unsigned frac;
    int n;

    ddx = x2 - x1;
    ddy = y2 - y1;
    if (ddx < 0) {
        ddx = -ddx;
        sx = -1;
    } else
        sx = 1;
    if (ddy < 0) {
        ddy = -ddy;
        sy = -1;
    } else
        sy = 1;
    n = 0;
    x = x1;
    y = y1;
    if (ddx < ddy) {
        frac = 0x8000;                         /* 0.5: the steps round */
        asm mov dx, ddx
        asm mov ax, 0
        asm mov bx, ddy
        asm div bx
        asm mov step, ax
        if (sx == 1) {
            for (; n < ddy; n++) {
                y += sy;
                asm mov ax, frac
                asm add ax, step
                asm jae a1
                x++;
            a1:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        } else {
            for (; n < ddy; n++) {
                y += sy;
                asm mov ax, frac
                asm add ax, step
                asm jae a2
                x--;
            a2:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        }
    } else if (ddy < ddx) {
        frac = 0x8000;                         /* 0.5: the steps round */
        asm mov dx, ddy
        asm mov ax, 0
        asm mov bx, ddx
        asm div bx
        asm mov step, ax
        if (sy == 1) {
            for (; n < ddx; n++) {
                x += sx;
                asm mov ax, frac
                asm add ax, step
                asm jae a3
                y++;
            a3:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        } else {
            for (; n < ddx; n++) {
                x += sx;
                asm mov ax, frac
                asm add ax, step
                asm jae a4
                y--;
            a4:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        }
    } else {
        for (; n < ddy; n++) {
            y += sy;
            x += sx;
            xs[n] = x;
            ys[n] = y;
        }
    }
    return n;
}

/* (x1, y1, x2, y2, xs, ys, width) -> n: the same, going the shorter way around a map width wide */
int Map_Line_Path(int x1, int y1, int x2, int y2, char *xs, char *ys, int width)
{
    int ddx;
    int ddy;
    int sx;
    int sy;
    int x;
    int y;
    unsigned step;
    unsigned frac;
    int n;
    int i;

    /* the shorter way round the world: move one end by a world width */
    if (abs(width - x2 + x1) < abs(x2 - x1))
        x2 = x2 - width;
    if (abs(width - x1 + x2) < abs(x2 - x1))
        x2 = x2 + width;
    ddx = x2 - x1;
    ddy = y2 - y1;
    if (ddx < 0) {
        ddx = -ddx;
        sx = -1;
    } else
        sx = 1;
    if (ddy < 0) {
        ddy = -ddy;
        sy = -1;
    } else
        sy = 1;
    n = 0;
    x = x1;
    y = y1;
    if (ddx < ddy) {
        frac = 0x8000;                         /* 0.5: the steps round */
        asm mov dx, ddx
        asm mov ax, 0
        asm mov bx, ddy
        asm div bx
        asm mov step, ax
        if (sx == 1) {
            for (; n < ddy; n++) {
                y += sy;
                asm mov ax, frac
                asm add ax, step
                asm jae b1
                x++;
            b1:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        } else {
            for (; n < ddy; n++) {
                y += sy;
                asm mov ax, frac
                asm add ax, step
                asm jae b2
                x--;
            b2:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        }
    } else if (ddy < ddx) {
        frac = 0x8000;                         /* 0.5: the steps round */
        asm mov dx, ddy
        asm mov ax, 0
        asm mov bx, ddx
        asm div bx
        asm mov step, ax
        if (sy == 1) {
            for (; n < ddx; n++) {
                x += sx;
                asm mov ax, frac
                asm add ax, step
                asm jae b3
                y++;
            b3:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        } else {
            for (; n < ddx; n++) {
                x += sx;
                asm mov ax, frac
                asm add ax, step
                asm jae b4
                y--;
            b4:
                asm mov frac, ax
                xs[n] = x;
                ys[n] = y;
            }
        }
    } else {
        for (; n < ddy; n++) {
            y += sy;
            x += sx;
            xs[n] = x;
            ys[n] = y;
        }
    }
    for (i = 0; i < n; i++)
        if (xs[i] < 0)
            xs[i] = xs[i] + width;
    for (i = 0; i < n; i++)
        if (xs[i] >= width)
            xs[i] = xs[i] - width;
    return n;
}
