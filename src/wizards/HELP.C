/* HELP.C - WIZARDS.EXE code segment 8546: per-screen loaders of the context-help records (HLPENTRY.LBX).
   Matching target: Borland C++ 3.1, BCC -c -mm -a. Check: python -P tools/match_build.py WIZARDS 8546 src/wizards/HELP.C */
#include "GAME.H"

extern struct HelpEntry help_entries[];         /* DS:bb7a */

void UI_Set_Help(struct HelpEntry *help, int count);

/* () HLPENTRY.LBX entry 1, records 0 .. 24, installed as the screen's help (UI_Set_Help) */
void Help_Load_Main_Screen(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_MAIN_SCREEN, help_entries, 0, 25, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 25);
}

/* () HLPENTRY.LBX entry 2, records 0 .. 15, installed as the screen's help (UI_Set_Help) */
void Help_Load_Armies(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_ARMIES, help_entries, 0, 16, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 16);
}

/* () HLPENTRY.LBX entry 3, records 0 .. 22, installed as the screen's help (UI_Set_Help) */
void Help_Load_Items(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_ITEMS, help_entries, 0, 23, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 23);
}

/* () HLPENTRY.LBX entry 4, records 0 .. 9, installed as the screen's help (UI_Set_Help) */
void Help_Load_City_List(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_CITY_LIST, help_entries, 0, 10, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 10);
}

/* () HLPENTRY.LBX entry 5, records 0 .. 46, installed as the screen's help (UI_Set_Help) */
void Help_Load_Magic_Screen(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_MAGIC_SCREEN, help_entries, 0, 47, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 47);
}

/* () HLPENTRY.LBX entry 6, records 0 .. 21, installed as the screen's help (UI_Set_Help) */
void Help_Load_City(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_CITY, help_entries, 0, 22, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 22);
}

/* () HLPENTRY.LBX entry 7, records 0 .. 17, installed as the screen's help (UI_Set_Help) */
void Help_Load_Wizard_Info(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_WIZARD_INFO, help_entries, 0, 18, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 18);
}

/* () HLPENTRY.LBX entry 8, records 0 .. 4, installed as the screen's help (UI_Set_Help) */
void Help_Load_Cartographer(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_CARTOGRAPHER, help_entries, 0, 5, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 5);
}

/* () HLPENTRY.LBX entry 9, records 0 .. 0, installed as the screen's help (UI_Set_Help) */
void Help_Load_Historian(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_HISTORIAN, help_entries, 0, 1, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 1);
}

/* () HLPENTRY.LBX entry 10, records 0 .. 0, installed as the screen's help (UI_Set_Help) */
void Help_Load_Astrologer(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_ASTROLOGER, help_entries, 0, 1, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 1);
}

/* () HLPENTRY.LBX entry 11, records 0 .. 5, installed as the screen's help (UI_Set_Help) */
void Help_Load_Alchemy(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_ALCHEMY, help_entries, 0, 6, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 6);
}

/* () HLPENTRY.LBX entry 12, records 0 .. 42, installed as the screen's help (UI_Set_Help) */
void Help_Load_City_Production(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_CITY_PRODUCTION, help_entries, 0, 43, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 43);
}

/* (view_mode) the unit view's help: HLPENTRY.LBX entry 13 (entry 22, 'combat view Help', for UNIT_VIEW_COMBAT),
   records 0 .. 22, installed with UI_Set_Help */
void Help_Load_Unit_View(int view_mode)
{
    if (view_mode != UNIT_VIEW_COMBAT)
        LBX_Load_Records_Near("hlpentry", HLPENTRY_UNIT_VIEW, help_entries, 0, 23, sizeof(struct HelpEntry));
    else
        LBX_Load_Records_Near("hlpentry", HLPENTRY_COMBAT_UNIT_VIEW, help_entries, 0, 23, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 23);
}

/* () HLPENTRY.LBX entry 14, records 0 .. 10, installed as the screen's help (UI_Set_Help) */
void Help_Load_City_View(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_ENEMY_CITY, help_entries, 0, 11, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 11);
}

/* () HLPENTRY.LBX entry 15, records 0 .. 7, installed as the screen's help (UI_Set_Help) */
void Help_Load_Hero_Level(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_HERO_LEVEL, help_entries, 0, 8, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 8);
}

/* () installs the 50 help records already loaded (the stack popup's) */
void Help_Set_Stack_Popup(void)
{
    UI_Set_Help(help_entries, 50);
}

/* () HLPENTRY.LBX entry 17, records 0 .. 3, installed as the screen's help (UI_Set_Help) */
void Help_Load_Outpost(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_OUTPOST, help_entries, 0, 4, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 4);
}

/* () HLPENTRY.LBX entry 18, records 0 .. 11, installed as the screen's help (UI_Set_Help) */
void Help_Load_Spell_Target(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_SPELL_TARGET, help_entries, 0, 12, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 12);
}

/* () HLPENTRY.LBX entry 19, records 0 .. 12, installed as the screen's help (UI_Set_Help) */
void Help_Load_Road_Building(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_ROAD_BUILDING, help_entries, 0, 13, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 13);
}

/* () HLPENTRY.LBX entry 20, records 0 .. 11, installed as the screen's help (UI_Set_Help) */
void Help_Load_Surveyor(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_SURVEYOR, help_entries, 0, 12, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 12);
}

/* () HLPENTRY.LBX entry 21, records 0 .. 16, installed as the screen's help (UI_Set_Help) */
void Help_Load_Combat(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_COMBAT, help_entries, 0, 17, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 17);
}

/* () HLPENTRY.LBX entry 38, records 0 .. 0, installed as the screen's help (UI_Set_Help) */
void Help_Load_Tax_Collector(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_TAX_COLLECTOR, help_entries, 0, 1, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 1);
}

/* () HLPENTRY.LBX entry 39, records 0 .. 0, installed as the screen's help (UI_Set_Help) */
void Help_Load_Info_Menu(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_INFO_MENU, help_entries, 0, 1, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 1);
}

/* () HLPENTRY.LBX entry 40, records 0 .. 6, installed as the screen's help (UI_Set_Help) */
void Help_Load_Hall_Of_Fame(void)
{
    LBX_Load_Records_Near("hlpentry", HLPENTRY_HALL_OF_FAME, help_entries, 0, 7, sizeof(struct HelpEntry));
    UI_Set_Help(help_entries, 7);
}
