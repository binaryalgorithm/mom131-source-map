/* RESEARCH.C - WIZARDS.EXE overlay code segment 9c9a (stub 2560): the research book - the page turn animation
   (four sheared frames), the two-page redraw with its corners, a page of spells (casting cost or research cost
   with the turns it takes, the description from DESC.LBX), the wizard's lab where a learned spell is shown and the
   book opened at it (a sparkle runs along the spell's row), the Apprentice's browsing screen, choosing the next
   research and re-choosing when the current one became known.
   This module's data (DS:5dc0..5e8f): the two strip tables of the learning animation (initialisers of local
   arrays) and its literals ("" is the tail of "book"); BSS (DS:c928..c96f): the description buffers and the lab
   pictures.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d -Y. Check: python -P tools/match_build.py WIZARDS 9c9a src/wizards/RESEARCH.C */
#include <string.h>

/* the module's functions in their original declaration order (= the overlay stub's thunk order) */
void Research_Book_Page_Turn(int dir);
void Research_Book_Redraw(void);
void Wizlab_Draw(void);
void Wizlab_Load(void);
void Screen_Fill_Black(void);
int Screen_Spell_Learned(int spell, int researched);
void Spell_Learned_Anim_Frame(void);
void Research_Book_Load_Descs(int target_page);
void Screen_Apprentice(void);
void Research_Book_Draw_Page(int page, unsigned pic, int is_left);
int Screen_Choose_Research(void);
void Research_Recheck_Known(int fade);

#include "GAME.H"

unsigned wizlab_lectern_pic;                    /* DS:c96e  WIZLAB.LBX 19 the room, then 20 the lectern */
unsigned wizlab_top_pic;                        /* DS:c96c  WIZLAB.LBX 21 the magic column */
unsigned wizlab_familiar_pic;                   /* DS:c96a  WIZLAB.LBX 14 + primary realm */
                                                /* also: a MASTERY.C picture */
unsigned wizlab_portrait_pic;                   /* DS:c968 */
                                                /* also: a MASTERY.C picture */
char far *research_desc_texts[16];                    /* DS:c928  descriptions of the 4 + 4 spells shown and the next two pages */

extern unsigned p_anim_pic;                     /* DS:9130 */
extern unsigned music_jingle;                   /* DS:9134 */
extern unsigned p_music_buf;                    /* DS:9390 */
extern int research_first_page;                 /* DS:9448 */
extern unsigned scroll_bg_pic;                  /* DS:9460  SCROLL.LBX 6 */
extern unsigned research_big_right_corner_pic;  /* DS:9464 */
extern unsigned research_big_left_corner_pic;   /* DS:9466 */
extern int present_mode;                        /* DS:9c8e */
extern struct HelpEntry help_entries[];         /* DS:bb7a */
extern struct SpellbookPage *p_spellbook_pages; /* DS:c8fa */
extern int ui_anim_frame;                       /* DS:c8fc */
extern int spend_per_turn;                      /* DS:c90a */
extern int *research_per_turn_by_realm;         /* DS:c90c */
extern int spellbook_page_count;                /* DS:c914 */
extern int spellbook_mode;                 /* DS:c922 */
extern int spellbook_page;                      /* DS:c924 */
extern int list_count;                          /* DS:c926 */
extern int ui_spell;                            /* DS:ca16 */
extern unsigned ui_temp_pic_a;                  /* DS:ca34 */
extern unsigned ui_temp_pic_b;                  /* DS:ca36 */

int abs(int x);                                 /* no <stdlib.h>: its abs() is an inline intrinsic, the game calls the function */
char *itoa(int value, char *string, int radix);
char *ltoa(long value, char *string, int radix);
void far_strcpy();                              /* unprototyped: called with (near dst, 0, far src), (far dst, near src, 0) and
                                                   (far dst, far src) */
void Palette_Load();                            /* no prototype here: called as (entry, -1), so first = -1 (all colors) and last is never read */
void Anim_Load_To_EMS(char *lbx, int entry);
unsigned Heap_Alloc_Back(unsigned block, unsigned paragraphs);
void Heap_Mark(unsigned block);
void Heap_Release(unsigned block);
void Heap_Reset(unsigned block);
unsigned LBX_Reload_Next(char *name, int entry, unsigned block);
int Draw_Bitmap_Clipped(int x, int y, unsigned pic);
int Draw_Pic_Clipped(int x, int y, unsigned pic);
void Anim_Set_Frame(int frame);
void Anim_EMS_Next_Frame(void);
void Pic_Scale_Percent(unsigned pic, int x_pct, int y_pct);
void Pic_Shear_Columns(int x1, int dy1, int x2, int dy2, unsigned pic);
void Palette_Cycle_Color(int pal_index, int r1, int g1, int b1, int r2, int g2, int b2, int speed);
void Font_Select_Outline_Shadow(int font, int c1, int c2, int c3);
void Pic_Reset_Frame(unsigned pic);
void Pic_Apply_Palette(unsigned pic);
void Set_Draw_Page_Front(void);
int Sound_Stop(void);
int Sound_Play(unsigned sound);
void Remap_Build_Range(int first, int last);
void Spellbook_Build(int mode, int per_page);
void far *Seg_To_Far_Ptr(unsigned seg);
void Pic_Create_Blank(int width, int height, unsigned pic);
int Spell_Casting_Cost(int player, int spell, int in_combat);
int Print_Text_To_Pic(int x, int y, char *text, unsigned pic);
int Print_Text_Right_To_Pic(int x, int y, char *text, unsigned pic);
int Print_Text_Centered_To_Pic(int x, int y, char *text, unsigned pic);
void Set_Text_Char_Gap(int gap);
void Set_Line_Spacing(int line_gap);
void Paragraph_Print_To_Pic(int x, int y, int width, char *text, unsigned align, unsigned pic);
void Music_Play_Current(void);
void Minimap_Alloc_Pic(void);
void Map_Redraw(void);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x, int pic1_y,
                        unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
int Pic_Clear_Rect(int x1, int y1, int x2, int y2, unsigned pic);
void Draw_Bitmap(int x, int y, unsigned pic);
void Set_Pic_Frame(unsigned pic, unsigned frame);
void UI_Set_Help(struct HelpEntry *help, int count);
void UI_Set_Redraw_Fn(void (far *fn)(void), int ticks);
void Screen_Copy_Front(void);
void Palette_Fade_From_Black(void);

/* turns the book: dir 1 forward, 0 back, 666 forward to the research pages */
void Research_Book_Page_Turn(int dir)
{
    unsigned pic;
    int page;
    int side;
    int frame;
    int widths[4];

    widths[0] = 90;
    widths[1] = 60;
    widths[2] = 60;
    widths[3] = 90;
    UI_Suspend_Redraw_Fn();
    Anim_Load_To_EMS("book", 1);
    Heap_Mark(p_scratch);
    pic = Heap_Alloc_Back(p_scratch, 0x708);
    research_big_left_corner_pic = LBX_Reload_Next("scroll", 7, p_scratch);
    research_big_right_corner_pic = LBX_Reload_Next("scroll", 8, p_scratch);
    for (frame = 0; frame < 4; frame++) {
        Set_Draw_Page();
        Screen_Restore_A800();
        page = dir >= 1 ? spellbook_page : spellbook_page - 2;
        Research_Book_Draw_Page(page, pic, 1);
        Draw_Bitmap_Clipped(25, -20, pic);
        if (page != 0 && spellbook_mode > 1) {
            Draw_Pic_Frame(15, 9, research_big_left_corner_pic);
        }
        if (dir == 1) {
            page = spellbook_page + 3;
        } else if (dir == 0) {
            page = spellbook_page + 1;
        } else {
            page = research_first_page + 1;
        }
        Research_Book_Draw_Page(page, pic, 0);
        Draw_Bitmap_Clipped(173, -20, pic);
        if (page < spellbook_page_count - 2 && spellbook_mode > 1) {
            Draw_Pic_Frame(289, 9, research_big_right_corner_pic);
        }
        if (dir == 0) {
            Anim_Set_Frame(3 - frame);
        }
        Anim_EMS_Next_Frame();
        if (frame < 2) {
            if (dir >= 1) {
                page = spellbook_page + 1;
                side = 0;
            } else {
                page = spellbook_page;
                side = 1;
            }
        } else if (dir == 1) {
            page = spellbook_page + 2;
            side = 1;
        } else if (dir == PAGE_TURN_TO_RESEARCH) {
            page = research_first_page;
            side = 1;
        } else {
            page = spellbook_page - 1;
            side = 0;
        }
        Research_Book_Draw_Page(page, pic, side);
        Pic_Scale_Percent(pic, widths[frame], 100);
        if ((dir >= 1 && frame == 0) || (dir == 0 && frame == 3)) {
            Pic_Shear_Columns(0, 0, 4, -6, pic);
            Pic_Shear_Columns(5, -7, 12, -12, pic);
            Pic_Shear_Columns(13, -13, 29, -19, pic);
            Pic_Shear_Columns(30, -19, 41, -19, pic);
            Pic_Shear_Columns(42, -19, 67, -14, pic);
            Pic_Shear_Columns(68, -13, 84, -7, pic);
            Pic_Shear_Columns(85, -6, 115, 8, pic);
            Draw_Bitmap_Clipped(165, -24, pic);
        }
        if ((dir >= 1 && frame == 1) || (dir == 0 && frame == 2)) {
            Pic_Shear_Columns(0, -4, 5, -9, pic);
            Pic_Shear_Columns(6, -10, 14, -18, pic);
            Pic_Shear_Columns(15, -19, 32, -26, pic);
            Pic_Shear_Columns(33, -26, 42, -28, pic);
            Pic_Shear_Columns(43, -28, 56, -25, pic);
            Pic_Shear_Columns(57, -25, 69, -18, pic);
            Pic_Shear_Columns(70, -18, 76, -12, pic);
            Draw_Bitmap_Clipped(165, -26, pic);
        }
        if ((dir >= 1 && frame == 2) || (dir == 0 && frame == 1)) {
            Pic_Shear_Columns(0, -20, 6, -25, pic);
            Pic_Shear_Columns(7, -25, 16, -30, pic);
            Pic_Shear_Columns(17, -30, 24, -32, pic);
            Pic_Shear_Columns(25, -32, 32, -33, pic);
            Pic_Shear_Columns(33, -33, 47, -29, pic);
            Pic_Shear_Columns(48, -29, 61, -21, pic);
            Pic_Shear_Columns(62, -20, 70, -12, pic);
            Pic_Shear_Columns(71, -11, 75, 0, pic);
            Draw_Bitmap_Clipped(80, -20, pic);
        }
        if ((dir >= 1 && frame == 3) || (dir == 0 && frame == 0)) {
            Pic_Shear_Columns(0, 0, 36, -18, pic);
            Pic_Shear_Columns(37, -18, 64, -26, pic);
            Pic_Shear_Columns(65, -26, 73, -27, pic);
            Pic_Shear_Columns(74, -27, 84, -26, pic);
            Pic_Shear_Columns(85, -25, 92, -23, pic);
            Pic_Shear_Columns(93, -22, 102, -18, pic);
            Pic_Shear_Columns(103, -17, 115, -6, pic);
            Draw_Bitmap_Clipped(39, -22, pic);
        }
        Screen_Present();
    }
    UI_Set_Redraw_Fn(Research_Book_Redraw, 2);
    Heap_Release(p_scratch);
}

/* the open book: two pages, the corners when more pages follow, the prompt when choosing */
void Research_Book_Redraw(void)
{
    unsigned pic;

    ui_anim_frame = (ui_anim_frame + 1) % 8;
    Palette_Cycle_Color(PAL_CYCLE_COLOR, 29, 41, 63, 44, 56, 63, 2);
    Heap_Mark(p_scratch);
    pic = Heap_Alloc_Back(p_scratch, 0x708);
    research_big_left_corner_pic = LBX_Reload_Next("scroll", 7, p_scratch);
    research_big_right_corner_pic = LBX_Reload_Next("scroll", 8, p_scratch);
    Screen_Restore_A800();
    if (spellbook_mode > 1) {
        if (spellbook_page != 0) {
            Draw_Pic_Frame(15, 9, research_big_left_corner_pic);
        }
        if (spellbook_page != spellbook_page_count && spellbook_page < spellbook_page_count - 2) {
            Draw_Pic_Frame(289, 9, research_big_right_corner_pic);
        }
    }
    Research_Book_Draw_Page(spellbook_page, pic, 1);
    Draw_Bitmap_Clipped(25, -20, pic);
    Research_Book_Draw_Page(spellbook_page + 1, pic, 0);
    Draw_Bitmap_Clipped(173, -20, pic);
    if (spellbook_mode == 0) {
        Font_Select_Outline_Shadow(5, 9, 0, 0);
        Set_Text_Shadow_Color(1);
        Print_Text(43, 183, "Choose a new spell to research");
    }
    Heap_Release(p_scratch);
}

/* the lab: the room, the lectern, the portrait, the familiar and the magic column */
void Wizlab_Draw(void)
{
    Screen_Restore_A800();
    Draw_Pic_Frame(149, 133, wizlab_lectern_pic);
    Pic_Reset_Frame(wizlab_portrait_pic);
    Draw_Pic_Frame(69, 75, wizlab_portrait_pic);
    Draw_Pic_Frame(190, 160, wizlab_familiar_pic);
    Draw_Pic_Clipped(132, -3, wizlab_top_pic);
}

/* () the draw page filled with color 0 */
void Screen_Fill_Black(void)
{
    Fill_Rect(0, 0, 319, 199, 0);
}

/* () the wizard's lab backdrop: the human's portrait (its palette applied), the room (WIZLAB 19) drawn and
   kept in the A800 store (Screen_Save_A800), then the lectern (20); the familiar of the primary realm (WIZLAB 14 + realm) */
void Wizlab_Load(void)
{
    char realm;

    realm = wizards[PLAYER_HUMAN].primary_realm;
    wizlab_portrait_pic = LBX_Reload("Wizlab", wizards[PLAYER_HUMAN].portrait, p_scratch);
    Pic_Apply_Palette(wizlab_portrait_pic);
    wizlab_lectern_pic = LBX_Reload("Wizlab", 19, p_scratch);
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, wizlab_lectern_pic);
    Screen_Save_A800();
    Set_Draw_Page_Front();
    wizlab_lectern_pic = LBX_Reload("Wizlab", 20, p_scratch);
    wizlab_familiar_pic = LBX_Reload_Next("Wizlab", realm + 14, p_scratch);
    wizlab_top_pic = LBX_Reload_Next("Wizlab", 21, p_scratch);
    wizlab_portrait_pic = LBX_Reload_Next("Wizlab", wizards[PLAYER_HUMAN].portrait, p_scratch);
}

/* a spell learned: the lab, then the book opened at the spell with the sparkle; researched: turns on to the
   research pages. Without animations: "You have mastered the spell of ..." */
int Screen_Spell_Learned(int spell, int researched)
{
    int i;
    int k;
    int wait;                                   /* the 60-frame wait counter, then the color-fill index */
    int casting_cost;
    int right_page;
    int found;
    int control;
    char num_text[10];
    int input;
    int count;
    unsigned pic;
    char spell_name[20];
    unsigned char colors[16];
    char page_text[110];

    ui_spell = spell;
    if (game_config.spell_animations == 1) {
        if (game_config.music == 1) {
            Sound_Stop();
            music_jingle = LBX_Reload("music", MUSIC_NEW_SPELL, p_music_buf);
            Sound_Play(music_jingle);
        }
        Palette_Fade_To_Black();
        Mouse_Set_Cursor_Regions(1, &mouse_region_hidden);
        Wizlab_Load();
        Set_Draw_Page();
        Wizlab_Draw();
        present_mode = PRESENT_FADE_IN;
        Screen_Present();
        UI_Clear_Controls();
        i = UI_Add_Region(0, 0, 319, 199, "", -1);
        UI_Set_Redraw_Fn(Wizlab_Draw, 2);
        for (wait = 0; wait < 60 && UI_Get_Input() == 0; wait++) {
            Timer_Mark();
            Set_Draw_Page();
            Wizlab_Draw();
            Screen_Present();
            Wait_Ticks(2);
        }
        UI_Clear_Controls();
        UI_Set_Redraw_Fn(Screen_Fill_Black, 3);
        Palette_Fade_To_Black();
        Set_Draw_Page();
        Screen_Fill_Black();
        Screen_Present();
        Palette_Load(0, -1);
        Palette_Cycle_Reset();
        Palette_Mark_Range(0, 254);
        Remap_Build_Range(10, 11);
        Remap_Build_Range(0, 0);
        Spellbook_Build(SPELLBOOK_RESEARCH, RESEARCH_PAGE_SPELLS);
        scroll_bg_pic = LBX_Reload("Scroll", 6, p_scratch);
        Set_Draw_Page();
        Draw_Pic_Frame(0, 0, scroll_bg_pic);
        Screen_Save_A800();
        Set_Draw_Page_Front();
        found = 0;
        for (i = 0; i < spellbook_page_count - 2 && found == 0; i++) {
            for (k = 0; k < p_spellbook_pages[i].count && found == 0; k++) {
                if (abs(p_spellbook_pages[i].spells[k]) == spell) {
                    if (i % 2 == 0) {
                        spellbook_page = i;
                        right_page = 0;
                        spellbook_mode = 0;
                    } else {
                        right_page = 1;
                        spellbook_mode = 1;
                        spellbook_page = i - 1;
                    }
                    list_count = k;             /* reused: the learned spell's row (read at the draw) */
                    found = 1;
                }
            }
        }
        if (found == 0) {
            /* BUG #475: (spell - 2) / SPELLS_PER_REALM is not SPELL_REALM(spell): a realm's first spell marks the
               previous realm's first slot */
            wizards[PLAYER_HUMAN].spell_library[(spell - 2) / SPELLS_PER_REALM][SPELL_SLOT(spell)] = SPELLSTATE_RESEARCHABLE;
            Spellbook_Build(SPELLBOOK_RESEARCH, RESEARCH_PAGE_SPELLS);
            for (i = 0; i < spellbook_page_count - 1 && found == 0; i++) {
                for (k = 0; k < p_spellbook_pages[i].count && found == 0; k++) {
                    if (abs(p_spellbook_pages[i].spells[k]) == spell) {
                        if (i % 2 == 0) {
                            spellbook_page = i;
                            right_page = 0;
                            spellbook_mode = 0;
                        } else {
                            right_page = 1;
                            spellbook_mode = 1;
                            spellbook_page = i - 1;
                        }
                        list_count = k;             /* reused: the learned spell's row (read at the draw) */
                        found = 1;
                    }
                }
            }
        }
        Heap_Reset(p_scratch);
        for (i = 0; i < 16; i++) {
            research_desc_texts[i] = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 12));   /* a 110-byte description (9 or 12 paragraphs by caller) */
        }
        Research_Book_Load_Descs(spellbook_page);
        Heap_Mark(p_scratch);
        ui_temp_pic_b = Heap_Alloc_Back(p_scratch, 0x708);
        ui_temp_pic_a = Heap_Alloc_Back(p_scratch, 0x145);
        p_anim_pic = LBX_Reload_Next("SPECFX", 49, p_scratch);
        far_strcpy(page_text, 0, research_desc_texts[right_page * 4 + k - 1]);   /* k: the search loop's exit, the found row + 1 */
        Pic_Create_Blank(129, 37, ui_temp_pic_a);
        casting_cost = Spell_Casting_Cost(PLAYER_HUMAN, ui_spell, 0);
        for (wait = 0; wait < 16; wait++) {
            colors[wait] = 0xc6;
        }
        Font_Select_Palette(7, colors);
        far_strcpy(spell_name, 0, p_spell_data[ui_spell].name);
        Print_Text_To_Pic(0, 0, spell_name, ui_temp_pic_a);
        Font_Select_Palette(6, colors);
        Print_Text_To_Pic(0, 11, "SP Cost :", ui_temp_pic_a);
        itoa(casting_cost, num_text, 10);
        Print_Text_Right_To_Pic(42, 11, num_text, ui_temp_pic_a);
        Set_Line_Spacing(0);
        Set_Text_Char_Gap(1);
        Paragraph_Print_To_Pic(0, 17, 128, page_text, PARA_LEFT, ui_temp_pic_a);
        UI_Clear_Controls();
        ui_anim_frame = 0;
        control = UI_Add_Region(0, 0, 319, 199, UI_NO_HOTKEY, -1);
        input = 0;
        UI_Set_Redraw_Fn(Spell_Learned_Anim_Frame, 2);
        for (ui_anim_frame = 0; ui_anim_frame < 45 && input == 0; ui_anim_frame++) {
            input = UI_Get_Input();
            if (ui_anim_frame < 15) {
                Palette_Cycle_Color(PAL_CYCLE_COLOR, 18, 14, 9, 38, 38, 53, 2);
            }
            Set_Draw_Page();
            Spell_Learned_Anim_Frame();
            Screen_Present();
            if (ui_anim_frame == 0) {
                present_mode = PRESENT_NORMAL;
            }
        }
        Heap_Release(p_scratch);
        UI_Suspend_Redraw_Fn();
        if (researched == 1) {
            count = 0;
            for (i = 0; i < 8; i++) {
                if (wizards[PLAYER_HUMAN].research_candidates[i] > 0) {
                    count++;
                }
            }
            if (count == 0) {
                Sound_Stop();
                Music_Play_Current();
                return 0;
            }
            if (spellbook_page < research_first_page) {
                Research_Book_Load_Descs(spellbook_page + 2);
                Research_Book_Page_Turn(1);
                spellbook_page += 2;
                for (i = 0; i < 8; i++) {
                    far_strcpy(research_desc_texts[i], research_desc_texts[i + 8]);
                }
                spellbook_mode = 1;
                Set_Draw_Page();
                Research_Book_Redraw();
                Screen_Present();
            }
            if (spellbook_page < research_first_page) {   /* still short after the one turn: jump to them */
                Research_Book_Load_Descs(research_first_page);
                Research_Book_Page_Turn(PAGE_TURN_TO_RESEARCH);
                spellbook_page = research_first_page;
                for (i = 0; i < 8; i++) {
                    far_strcpy(research_desc_texts[i], research_desc_texts[i + 8]);
                }
                spellbook_mode = 1;
                Set_Draw_Page();
                Research_Book_Redraw();
                Screen_Present();
            }
            Palette_Mark_Range(0, 224);
            Remap_Build_Range(10, 11);
        }
        spellbook_mode = 1;
        Set_Draw_Page();
        Research_Book_Redraw();
        Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
        Screen_Present();
    } else {
        Heap_Reset(p_scratch);
        Minimap_Alloc_Pic();
        pic = LBX_Reload_Next("Spellscr", 60, p_scratch);
        UI_Set_Redraw_Fn(Screen_Fill_Black, 2);
        Set_Draw_Page();
        Map_Redraw();
        strcpy(message_text, "You have mastered the spell of ");
        far_strcpy(spell_name, 0, p_spell_data[spell].name);
        strcat(message_text, spell_name);
        Notify_Picture_Box(0, 40, 2, message_text, 0, pic, 0, 9, 0, 0, 0, 1, 0);
        UI_Suspend_Redraw_Fn();
    }
}

/* one frame of the learning animation: the spell's row cleared in strips, the spell drawn, the sparkle */
void Spell_Learned_Anim_Frame(void)
{
    int strip_left[6] = { 53, 45, 37, 16, 6, 6 };
    int strip_right[6] = { 64, 71, 79, 101, 111, 111 };

    Screen_Restore_A800();
    if (spellbook_mode == 1) {
        Research_Book_Draw_Page(spellbook_page, ui_temp_pic_b, 1);
        Draw_Bitmap_Clipped(25, -20, ui_temp_pic_b);
    } else {
        Research_Book_Draw_Page(spellbook_page + 1, ui_temp_pic_b, 0);
        Draw_Bitmap_Clipped(173, -20, ui_temp_pic_b);
    }
    if (spellbook_mode == 0) {
        Research_Book_Draw_Page(spellbook_page, ui_temp_pic_b, 1);
    } else {
        Research_Book_Draw_Page(spellbook_page + 1, ui_temp_pic_b, 0);
    }
    if (ui_anim_frame < 22) {
        Pic_Clear_Rect(0, list_count * 37 + 48, 129, list_count * 37 + 82, ui_temp_pic_b);
    } else if (ui_anim_frame < 28) {
        Pic_Clear_Rect(0, list_count * 37 + 48, strip_left[ui_anim_frame - 22], list_count * 37 + 82, ui_temp_pic_b);
        Pic_Clear_Rect(strip_right[ui_anim_frame - 22], list_count * 37 + 48, 129, list_count * 37 + 82, ui_temp_pic_b);
        Pic_Clear_Rect(strip_left[ui_anim_frame - 22], 0, strip_right[ui_anim_frame - 22], 37, ui_temp_pic_a);
    }
    if (spellbook_mode == 0) {
        Draw_Bitmap_Clipped(25, -20, ui_temp_pic_b);
        if (ui_anim_frame < 27) {
            Draw_Bitmap(25, list_count * 37 + 28, ui_temp_pic_a);
        }
        if (ui_anim_frame > 15 && ui_anim_frame < 29) {
            Set_Pic_Frame(p_anim_pic, ui_anim_frame - 16);
            Draw_Pic_Frame(20, list_count * 37 + 21, p_anim_pic);
        }
    } else {
        Draw_Bitmap_Clipped(173, -20, ui_temp_pic_b);
        if (ui_anim_frame < 27) {
            Draw_Bitmap(173, list_count * 37 + 28, ui_temp_pic_a);
        }
        if (ui_anim_frame > 15 && ui_anim_frame < 29) {
            Set_Pic_Frame(p_anim_pic, ui_anim_frame - 16);
            Draw_Pic_Frame(168, list_count * 37 + 24, p_anim_pic);
        }
    }
}

/* the descriptions (DESC.LBX) of the two pages shown and the two of target_page into research_desc_texts */
void Research_Book_Load_Descs(int target_page)
{
    int page;
    int k;
    char dest[110];

    for (page = 0; page < 16; page++) {
        far_strcpy(research_desc_texts[page], "", 0);
    }
    for (page = spellbook_page; page <= spellbook_page + 1; page++) {
        if (page >= 0 && page < spellbook_page_count) {
            for (k = 0; k < 4; k++) {
                if (k < p_spellbook_pages[page].count) {
                    LBX_Load_Records_Near("Desc", 0, dest, abs(p_spellbook_pages[page].spells[k]), 1, 110);
                    far_strcpy(research_desc_texts[(page - spellbook_page) * 4 + k], dest, 0);
                } else {
                    far_strcpy(research_desc_texts[(page - spellbook_page) * 4 + k], "", 0);
                }
            }
        } else {
            for (k = 0; k < 4; k++) {
                far_strcpy(research_desc_texts[(page - spellbook_page) * 4 + k], "", 0);
            }
        }
    }
    for (page = target_page; page <= target_page + 1; page++) {
        if (page >= 0 && page < spellbook_page_count) {
            for (k = 0; k < 4; k++) {
                if (k < p_spellbook_pages[page].count) {
                    LBX_Load_Records_Near("Desc", 0, dest, abs(p_spellbook_pages[page].spells[k]), 1, 110);
                    far_strcpy(research_desc_texts[(page - target_page + 2) * 4 + k], dest, 0);
                } else {
                    far_strcpy(research_desc_texts[(page - target_page + 2) * 4 + k], "", 0);
                }
            }
        } else {
            for (k = 0; k < 4; k++) {
                far_strcpy(research_desc_texts[(page - target_page + 2) * 4 + k], "", 0);
            }
        }
    }
}

/* Info menu: the Apprentice - browse the research book (Spell of Return shows help entry 657) */
void Screen_Apprentice(void)
{
    int i;
    int done;
    int input;
    int exit_ctl;
    int next_ctl;
    int prev_ctl;

    Palette_Cycle_Reset();
    Spellbook_Build(SPELLBOOK_RESEARCH, RESEARCH_PAGE_SPELLS);
    spellbook_page = research_first_page;
    spellbook_mode = 2;
    scroll_bg_pic = LBX_Reload("Scroll", 6, p_scratch);
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, scroll_bg_pic);
    Screen_Save_A800();
    Set_Draw_Page_Front();
    LBX_Load_Records_Near("hlpentry", HLPENTRY_BIG_BOOK, help_entries, 0, 10, sizeof(struct HelpEntry));
    Heap_Reset(p_scratch);
    for (i = 0; i < 16; i++) {
        research_desc_texts[i] = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 9));   /* a 110-byte description (9 or 12 paragraphs by caller) */
    }
    Research_Book_Load_Descs(spellbook_page);
    UI_Set_Redraw_Fn(Research_Book_Redraw, 2);
    UI_Clear_Controls();
    prev_ctl = UI_Add_Region(15, 7, 30, 22, "", -1);
    next_ctl = UI_Add_Region(290, 7, 305, 22, "", -1);
    exit_ctl = UI_Add_Region(0, 25, 319, 199, "\x1b", -1);
    done = 0;
    UI_Set_Help(help_entries, 10);
    while (done == 0) {
        input = UI_Get_Input();
        for (i = 0; i < 4; i++) {
            if (i < p_spellbook_pages[spellbook_page].count && p_spellbook_pages[spellbook_page].spells[i] > 0) {
                if (abs(p_spellbook_pages[spellbook_page].spells[i]) == SPELL_SPELL_OF_RETURN) {
                    help_entries[i + 2].id = HELP_SPELL_OF_RETURN;
                } else {
                    help_entries[i + 2].id = abs(p_spellbook_pages[spellbook_page].spells[i]);
                }
            } else {
                help_entries[i + 2].id = -1;
            }
        }
        for (i = 0; i < 4; i++) {
            if (i < p_spellbook_pages[spellbook_page + 1].count && p_spellbook_pages[spellbook_page + 1].spells[i] > 0) {
                if (abs(p_spellbook_pages[spellbook_page + 1].spells[i]) == SPELL_SPELL_OF_RETURN) {
                    help_entries[i + 6].id = HELP_SPELL_OF_RETURN;
                } else {
                    help_entries[i + 6].id = abs(p_spellbook_pages[spellbook_page + 1].spells[i]);
                }
            } else {
                help_entries[i + 6].id = -1;
            }
        }
        if (input == exit_ctl) {
            done = 1;
        }
        if (input == next_ctl && spellbook_page < spellbook_page_count - 2) {
            Research_Book_Load_Descs(spellbook_page + 2);
            Research_Book_Page_Turn(1);
            spellbook_page += 2;
            for (i = 0; i < 8; i++) {
                far_strcpy(research_desc_texts[i], research_desc_texts[i + 8]);
            }
            Set_Draw_Page();
            Research_Book_Redraw();
            Screen_Present();
        }
        if (input == prev_ctl && spellbook_page > 1) {
            Research_Book_Load_Descs(spellbook_page - 2);
            Research_Book_Page_Turn(0);
            spellbook_page -= 2;
            for (i = 0; i < 8; i++) {
                far_strcpy(research_desc_texts[i], research_desc_texts[i + 8]);
            }
            Set_Draw_Page();
            Research_Book_Redraw();
            Screen_Present();
        }
        if (done == 0) {
            Set_Draw_Page();
            Research_Book_Redraw();
            Screen_Present();
        }
    }
    research_first_page = spellbook_page;
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
}

/* one page: each spell's name, its cost line (casting cost, or research cost and turns on research pages) and
   its description; the realm title on a section's first page
   Numbered: BUG #39 (BUGS_AND_QUIRKS.txt) */
void Research_Book_Draw_Page(int page, unsigned pic, int is_left)
{
    int i;
    int k;
    int casting_cost;
    int turns;
    long cost_shown;
    int slot;
    unsigned char colors[16];
    char line[110];
    char num_text[20];                          /* a spell's name, then the numbers of the cost lines */

    slot = page - spellbook_page;               /* the description quad: 0 / 1 the open pages; -2 and 2 -> 2,
                                                   -1 and 3 -> 3 */
    if (slot < 0) {
        slot = abs(slot) + (slot + 2) * 2;
    }
    if (slot > 2) {
        slot = slot % 2 + 2;
    }
    colors[0] = 0xb8;
    for (i = 1; i < 16; i++) {
        colors[i] = 0xba;
    }
    Pic_Create_Blank(129, 200, pic);
    for (i = 0; i < p_spellbook_pages[page].count; i++) {
        if (p_spellbook_pages[page].spells[i] == wizards[PLAYER_HUMAN].research_spell) {
            for (k = 0; k < 16; k++) {
                colors[k] = 0xc6;
            }
            Font_Select_Palette(4, colors);
        } else {
            for (k = 0; k < 16; k++) {
                colors[k] = 0xba;
            }
            if (p_spellbook_pages[page].spells[i] < 0) {
                Font_Select_Palette(7, colors);
            } else {
                Font_Select_Palette(4, colors);
            }
        }
        far_strcpy(num_text, 0, p_spell_data[abs(p_spellbook_pages[page].spells[i])].name);
        Print_Text_To_Pic(0, i * 37 + 48, num_text, pic);
        if (p_spellbook_pages[page].spells[i] == wizards[PLAYER_HUMAN].research_spell) {
            for (k = 0; k < 16; k++) {
                colors[k] = 0xc6;
            }
            Font_Select_Palette(0, colors);
        } else {
            colors[0] = 0xb8;
            colors[1] = 0xb9;
            if (p_spellbook_pages[page].spells[i] < 0) {
                Font_Select_Palette(6, colors);
            } else {
                Font_Select_Palette(0, colors);
            }
        }
        if (page < research_first_page) {
            casting_cost = Spell_Casting_Cost(PLAYER_HUMAN, abs(p_spellbook_pages[page].spells[i]), 0);
            if (spend_per_turn == 0) {
                turns = 999;
            } else {
                turns = (casting_cost + spend_per_turn - 1) / spend_per_turn;
            }
            strcpy(line, "Casting Cost:");
            itoa(casting_cost, num_text, 10);
            strcat(line, num_text);
            strcat(line, " (");
            itoa(turns, num_text, 10);
            strcat(line, num_text);
            if (turns == 1) {
                strcat(line, " turn)");
            } else {
                strcat(line, " turns)");
            }
        } else {
            if (p_spellbook_pages[page].spells[i] == wizards[PLAYER_HUMAN].research_spell) {
                cost_shown = (unsigned)wizards[PLAYER_HUMAN].research_left;
            } else if (abs(p_spellbook_pages[page].spells[i]) == SPELL_SPELL_OF_MASTERY) {
                cost_shown = (unsigned)wizards[PLAYER_HUMAN].mastery_research_cost;
            } else {
                cost_shown = (unsigned)p_spell_data[abs(p_spellbook_pages[page].spells[i])].research_cost;
            }
            if (research_per_turn_by_realm[p_spell_data[abs(p_spellbook_pages[page].spells[i])].realm] == 0) {
                turns = 999;
            } else {
                turns = (cost_shown + research_per_turn_by_realm[p_spell_data[abs(p_spellbook_pages[page].spells[i])].realm] - 1)
                      / research_per_turn_by_realm[p_spell_data[abs(p_spellbook_pages[page].spells[i])].realm];
            }
            strcpy(line, "Research Cost:");
            ltoa(cost_shown, num_text, 10);
            strcat(line, num_text);
            strcat(line, " (");
            itoa(turns, num_text, 10);
            strcat(line, num_text);
            if (turns == 1) {
                strcat(line, " turn)");
            } else {
                strcat(line, " turns)");
            }
        }
        Print_Text_To_Pic(0, i * 37 + 59, line, pic);
        if (abs(p_spellbook_pages[page].spells[i]) > 0) {
            Set_Line_Spacing(0);
            Set_Text_Char_Gap(1);
            far_strcpy(line, 0, research_desc_texts[slot * 4 + i]);
            Paragraph_Print_To_Pic(0, i * 37 + 65, 128, line, PARA_LEFT, pic);
        }
    }
    if (p_spellbook_pages[page].count > 0 || page >= research_first_page) {
        colors[0] = 0xb8;
        colors[8] = 0x2e;
        colors[1] = 0x37;
        colors[9] = 0x2e;
        colors[2] = 0x2f;
        colors[10] = 0x2e;
        colors[3] = 0x27;
        colors[11] = 0x2e;
        colors[4] = 0x2e;
        colors[12] = 0x2e;
        colors[5] = 0x2e;
        colors[13] = 0x2e;
        colors[6] = 0x2e;
        colors[14] = 0x2e;
        colors[7] = 0x2e;
        colors[15] = 0x2e;
        Font_Select_Palette(5, colors);
        if (abs(spellbook_page - page) == 2 || page == spellbook_page
         || p_spell_data[abs(p_spellbook_pages[page].spells[0])].book_section
            != p_spell_data[abs(p_spellbook_pages[page - 1].spells[0])].book_section
         || page >= research_first_page) {
            if (is_left == 1) {
                Print_Text_Centered_To_Pic(66, 30, p_spellbook_pages[page].title, pic);
            } else {
                Print_Text_Centered_To_Pic(58, 30, p_spellbook_pages[page].title, pic);
            }
        }
    }
}

/* the 8 research candidates on the research pages: one click sets research_spell */
int Screen_Choose_Research(void)
{
    int i;
    int done;
    int input;
    int x1;
    int y1;
    int controls[8];

    done = 0;                                   /* done: first the candidates counted */
    for (i = 0; i < 8; i++) {
        if (wizards[PLAYER_HUMAN].research_candidates[i] > 0) {
            done++;
        }
    }
    if (done == 0) {
        return 0;
    }
    scroll_bg_pic = LBX_Reload("Scroll", 6, p_scratch);
    Set_Draw_Page();
    Draw_Pic_Frame(0, 0, scroll_bg_pic);
    Screen_Save_A800();
    Spellbook_Build(SPELLBOOK_RESEARCH, RESEARCH_PAGE_SPELLS);
    Mouse_Set_Cursor_Regions(1, &mouse_region_arrow);
    spellbook_mode = 0;
    Heap_Reset(p_scratch);
    for (i = 0; i < 16; i++) {
        research_desc_texts[i] = Seg_To_Far_Ptr(Heap_Alloc_Back(p_scratch, 9));   /* a 110-byte description (9 or 12 paragraphs by caller) */
    }
    spellbook_page = research_first_page;
    Research_Book_Load_Descs(spellbook_page);
    UI_Set_Redraw_Fn(Research_Book_Redraw, 2);
    UI_Clear_Controls();
    for (i = 0; i < 8; i++) {
        x1 = i / 4 * 147 + 25;
        y1 = i % 4 * 37 + 25;
        controls[i] = UI_Add_Region(x1, y1, x1 + 110, y1 + 33, "", -1);
    }
    present_mode = PRESENT_DISSOLVE;
    Set_Draw_Page();
    Research_Book_Redraw();
    Screen_Present();
    done = 0;
    LBX_Load_Records_Near("hlpentry", HLPENTRY_BIG_BOOK, help_entries, 0, 10, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 10);
    help_entries[0].id = -1;
    help_entries[1].id = -1;
    while (done == 0) {
        input = UI_Get_Input();
        for (i = 0; i < 4; i++) {
            if (i < p_spellbook_pages[spellbook_page].count && p_spellbook_pages[spellbook_page].spells[i] > 0) {
                if (abs(p_spellbook_pages[spellbook_page].spells[i]) == SPELL_SPELL_OF_RETURN) {
                    help_entries[i + 2].id = HELP_SPELL_OF_RETURN;
                } else {
                    help_entries[i + 2].id = abs(p_spellbook_pages[spellbook_page].spells[i]);
                }
            } else {
                help_entries[i + 2].id = -1;
            }
        }
        for (i = 0; i < 4; i++) {
            if (i < p_spellbook_pages[spellbook_page + 1].count && p_spellbook_pages[spellbook_page + 1].spells[i] > 0) {
                if (abs(p_spellbook_pages[spellbook_page + 1].spells[i]) == SPELL_SPELL_OF_RETURN) {
                    help_entries[i + 6].id = HELP_SPELL_OF_RETURN;
                } else {
                    help_entries[i + 6].id = abs(p_spellbook_pages[spellbook_page + 1].spells[i]);
                }
            } else {
                help_entries[i + 6].id = -1;
            }
        }
        for (i = 0; i < 8; i++) {
            if (controls[i] == input && wizards[PLAYER_HUMAN].research_candidates[i] > 0) {
                wizards[PLAYER_HUMAN].research_spell = wizards[PLAYER_HUMAN].research_candidates[i];
                done = 1;
            }
        }
        if (done == 0) {
            Set_Draw_Page();
            Research_Book_Redraw();
            Screen_Present();
        }
    }
    UI_Clear_Help();
    UI_Suspend_Redraw_Fn();
}

/* the current research became known (treasure, trade): choose again */
void Research_Recheck_Known(int fade)
{
    int spell;

    spell = abs(wizards[PLAYER_HUMAN].research_spell);
    if (wizards[PLAYER_HUMAN].spell_library[SPELL_REALM(spell)][SPELL_SLOT(spell)] == SPELLSTATE_KNOWN) {
        if (fade == 1) {
            Set_Draw_Page();
            Fill_Rect(0, 0, 319, 199, 0);
            UI_Present_Frame();
            Screen_Copy_Front();
            Palette_Fade_From_Black();
        }
        Screen_Choose_Research();
        wizards[PLAYER_HUMAN].research_left = wizards[PLAYER_HUMAN].research_spell == SPELL_SPELL_OF_MASTERY
                                 ? wizards[PLAYER_HUMAN].mastery_research_cost
                                 : p_spell_data[wizards[PLAYER_HUMAN].research_spell].research_cost;
        if (fade == 1) {
            Palette_Fade_To_Black();
            Set_Draw_Page();
            Fill_Rect(0, 0, 319, 199, 0);
            UI_Present_Frame();
            Screen_Copy_Front();
            Palette_Load(0, -1);
        }
    }
}
