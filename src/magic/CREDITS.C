/* CREDITS.C - MAGIC.EXE code segment 5624 (MAGIC only): the credits that scroll on the main menu - 48 role / name
   lines printed into an off-screen picture, the top and bottom lines faded through the text palette.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py MAGIC 5624 src/magic/CREDITS.C */
#pragma warn -pro
#include "ENUMS.H"

extern unsigned p_scratch;                      /* DS:61ae */

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
unsigned p_credits_pic;                         /* DS:948c */

int credits_top_line = 0;                       /* DS:38d8  first credits line shown */
int credits_scroll_y = 0;                       /* DS:38da  y of that line in the picture (scrolls up 1 px a frame) */
int credits_delay = 0;                          /* DS:38dc  redraws counted up since the (re)start; scroll from 70 */
char *credits[NUM_CREDIT_LINES][2] = {         /* DS:38de  role, name */
    { "Game Designer", "Steve Barcia" },
    { "", "" },
    { "Programmers", "Jim Cowlishaw" },
    { "", "Ken Burd" },
    { "", "Steve Barcia" },
    { "", "Grissel Barcia" },
    { "", "" },
    { "", "" },
    { "Producer", "Doug Caspian-Kaufman" },
    { "", "" },
    { "Art Director", "Jeff Dee" },
    { "Artists", "Shelly Hollen" },
    { "", "Amanda Dee" },
    { "", "Steve Austin" },
    { "", "George Purdy" },
    { "", "Patrick Owens" },
    { "", "Grissel Barcia" },
    { "", "" },
    { "Music Producer", "The Fat Man" },
    { "Composer", "Dave Govett" },
    { "", "" },
    { "QA Lead", "Destin Strader" },
    { "Play Test", "Mike Balogh" },
    { "", "Damon Harris" },
    { "", "Geoff Gessner" },
    { "", "Tammy Talbott" },
    { "", "Mick Uhl" },
    { "", "Jim Hendry" },
    { "", "Frank Brown" },
    { "", "Jim Tricario" },
    { "", "Jen MacLean" },
    { "", "Brian Wilson" },
    { "", "Brian Helleson" },
    { "", "Jeff Dinger" },
    { "", "Chris Bowling" },
    { "", "Charles Brubaker" },
    { "", "Tom Hughes" },
    { "", "" },
    { "Sound Effects", "Midian" },
    { "Speech", "Mark Reis" },
    { "", "Peter Woods" },
    { "", "David Ellis" },
    { "", "" },
    { "Manual", "Petra Schlunk" },
    { "", "" },
    { "", "" },
    { "Special Thanks", "Jenny Cowlishaw" },
    { "", "" },
};

/* () called on every main-menu redraw once the menu has faded in (screen_fade_in 0): after 70 redraws prints up to 10 credits lines ("role ..... name") into
   p_credits_pic, scrolls them up a pixel and draws the picture clipped at (85, 35); restarts after the last line. Declared int, but only the early exit returns a value (0) */
int Main_Menu_Draw_Credits(void)
{
    unsigned char colors[4];
    unsigned char leave_ramp[10];                /* 0xe1 .. 0xe9: a line leaving at the top darkens */
    unsigned char enter_ramp[10];                /* 0xe8 .. 0xe0: a line entering at the bottom brightens */
    int i;
    int width;
    int y;
    int x;

    credits_delay++;
    if (credits_delay > 30000)                  /* kept from overflowing */
        credits_delay = 70;
    if (credits_delay < 70)                     /* the scroll starts after 70 redraws */
        return 0;
    colors[0] = 0x44;                           /* the shadow; the text is 0xe0 (full) .. 0xe9 (darkest) */
    for (i = 0; i < 10; i++)
        leave_ramp[i] = i + 0xe0;
    for (i = 0; i < 10; i++)
        enter_ramp[i] = 0xe9 - i;
    enter_ramp[0] = 0xe8;                       /* so the darkest 0xe9 is never used at the bottom */
    Pic_Create_Blank(280, 122, p_credits_pic);
    Set_Clip_Rect(0, 40, 319, 137);             /* the picture goes to y 35: rows 5 .. 102 of it show; a line prints
                                                   while y < 99 and fades over its first / last 10 px */
    y = credits_scroll_y;
    for (i = 0; i < 10; i++) {
        if (y < 99 && credits_top_line + i < NUM_CREDIT_LINES) {
            colors[1] = 0xe0;
            if (y < 10)
                colors[1] = leave_ramp[10 - y];
            if (y > 88)
                colors[1] = enter_ramp[10 - (y - 88)];
            Font_Select_Palette(2, colors);
            Print_Text_To_Pic(0, y, credits[credits_top_line + i][0], p_credits_pic);
            width = Text_Width(credits[credits_top_line + i][0]);
            if (width > 1) {
                for (x = width + 1; x < 102; x += 4)
                    Print_Text_To_Pic(x, y, ".", p_credits_pic);
            }
            Print_Text_To_Pic(105, y, credits[credits_top_line + i][1], p_credits_pic);
            y += 10;
        }
    }
    credits_scroll_y--;
    if (credits_scroll_y < 1) {
        credits_top_line++;
        credits_scroll_y += 10;
        if (credits_top_line > NUM_CREDIT_LINES) {
            credits_delay = 0;
            credits_top_line = 0;
            credits_scroll_y = 95;
        }
    }
    Draw_Bitmap_Clipped(85, 35, p_credits_pic);
    Reset_Clip_Rect();
}

/* () allocates the credits picture (280 x 122) and restarts the scroll (top line 0, y 95) */
void Main_Menu_Alloc_Credits_Pic(void)
{
    credits_top_line = 0;
    credits_scroll_y = 95;
    p_credits_pic = Heap_Alloc_Front(p_scratch, 0xc35);   /* 50000 bytes, more than the 280 x 122 bitmap */
    Pic_Create_Blank(280, 122, p_credits_pic);
}
