/* EVENTS.C - WIZARDS.EXE code segment 6c53: random-event announcement, message formatting, wizard / city picks.
   This module's data starts at DS:4a0c: event_is_good[19], event_cooldown, the mineral
   names (written to follow "a": "n iron", " coal" ...), then the literals "MUSIC" .. "s".
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 6c53 src/wizards/EVENTS.C */
#include <string.h>
#include <stdlib.h>
#include "GAME.H"

/* the module's functions in the order the original declared them (= its overlay stub's thunk order; batch 39) */
int Event_Announce(void);
void Event_Format_Message(void);
int Event_Pick_Wizard(int good);
int Event_Pick_City(int player);

/* event_is_good: by EventId (1 .. 18); entry 0 unused */
int event_is_good[19] = { 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0 };
int event_cooldown = 0;                         /* counts up to 5 after an event (no event till then); not saved */
char *event_mineral_names[10] = { "", "n iron", " coal", " silver", " gold", " gem", " mithril", "n adamantium",
                                  " quork", " crysx" };

extern unsigned p_music_buf;                    /* DS:9390 */
extern int event_pop_lost;                    /* DS:c2d8 */
extern int event_units_lost;                  /* DS:c2da */
extern int event_buildings_lost;                  /* DS:c2dc */
extern int event_victim;                     /* DS:c2de */
extern int event_msg_arg;                    /* DS:c2e2 */
extern int event_city;                          /* DS:c2e4 */
extern int event_msg_event;                     /* DS:c2e6 */
extern int event_msg_variant;                   /* DS:c2e8 */
/* this module's BSS (only EVENTS reads them; the 6ad6 events module's globals lie below, 6cba's above - WIZARDS census,
   batch 39); Borland lays BSS out in reverse declaration order */
char *p_event_text;                             /* DS:c2ee */
char *p_event_raw;                              /* DS:c2ec */
extern char far *p_race_unrest_rows[];         /* DS:9ce0 */

int Sound_Stop(void);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Sound_Play(unsigned sound);
void Minimap_Alloc_Pic(void);
void Stack_Blink_Off(void);
void Map_Full_Redraw(void);
void Map_Redraw_Dissolve(void);
void Screen_Copy_Front(void);
void UI_Input_Delay(int delay);
void Notify_Picture_Box(int x_unused, int y, int style, char *text, int use_gem_frame, unsigned pic1, int pic1_x,
                        int pic1_y, unsigned pic2, int pic2_x, int pic2_y, int color_cycle, int dim);
void Palette_Unmark_Range(int first, int last);
void Palette_Mark_Range(int first, int last);
void Remap_Build_Shadow(void);
void Music_Play_Current(void);

/* () shows the new event to the human unless it strikes a wizard the human has not met: the map redrawn,
   the message (Event_Format_Message) in a picture box with the event's picture, the good / bad event music
   (by the HUMAN's books: Good Moon plays the bad music for one with Death books, Bad Moon for one with Life
   books); then the palette, music and map come back.
   0 when not shown, else no return value */
int Event_Announce(void)
{
    unsigned pic;
    unsigned block;
    int track;

    if (event_victim != EVENT_NO_VICTIM && event_victim != current_player &&
        wizards[current_player].contacted[event_victim] != 1)
        return 0;
    Sound_Stop();
    Minimap_Alloc_Pic();
    Stack_Blink_Off();
    Units_Set_Draw_Priority();
    Map_Build_Unit_Grid(map_view_x, map_view_y, map_view_plane);
    Map_Full_Redraw();
    Screen_Copy_Front();
    p_event_text = Near_Alloc_Reset(500);
    p_event_raw = Near_Alloc(500);
    Event_Format_Message();
    UI_Clear_Controls();
    UI_Input_Delay(1);
    if (game_config.event_music == 1) {
        if (event_is_good[event_msg_event] == 1)
            track = MUSIC_GOOD_EVENT;
        else
            track = MUSIC_BAD_EVENT;
        if (event_msg_event == EVENT_GOOD_MOON && wizards[PLAYER_HUMAN].spellbooks[REALM_DEATH] > 0)
            track = MUSIC_BAD_EVENT;
        if (event_msg_event == EVENT_BAD_MOON && wizards[PLAYER_HUMAN].spellbooks[REALM_LIFE] > 0)
            track = MUSIC_BAD_EVENT;
        block = LBX_Reload("MUSIC", track, p_music_buf);
        Sound_Play(block);
    }
    pic = LBX_Reload("EVENTS", event_msg_event - 1, p_scratch);
    Notify_Picture_Box(160, 70, 2, p_event_text, 1, pic, 0, 8, 0, 0, 0, 1, 1);
    Sound_Stop();
    Palette_Unmark_Range(0, 0xff);
    Palette_Mark_Range(0, 0xdf);
    Remap_Build_Shadow();
    Music_Play_Current();
    Minimap_Alloc_Pic();
    if (event_msg_event != EVENT_GIFT)
        Map_Redraw_Dissolve();
}

/* EVENTMSG.LBX's placeholder bytes (Event_Format_Message) */
#define ETXT_CITY              0x80   /* the event city's name */
#define ETXT_ARG_CITY          0x81   /* the city in event_msg_arg */
#define ETXT_ARG_CITY_SIZE     0x82   /* its size name */
#define ETXT_NUMBER            0x83   /* event_msg_arg as a number */
#define ETXT_MINERAL           0x89   /* the mineral event_msg_arg */
#define ETXT_ITEM              0x8a   /* the item event_msg_arg */
#define ETXT_CITY_SIZE         0x8b   /* the event city's size name */
#define ETXT_VICTIM            0x8c   /* the victim wizard's name */
#define ETXT_UNITS_LOST        0x8f   /* event_units_lost, or "no" */
#define ETXT_POP_LOST          0x90   /* event_pop_lost " thousand", or "no" */
#define ETXT_BUILDINGS_LOST    0x91   /* event_buildings_lost, or "no" */
#define ETXT_UNITS_PLURAL      0x93   /* "s" unless one unit */
#define ETXT_BUILDINGS_PLURAL  0x94   /* "s" unless one building */

/* () the event message: EVENTMSG.LBX record (event - 1) x 7 + variant, its placeholder bytes 80h .. 94h
   replaced by the city, wizard, item, amount ... names, into p_event_text */
void Event_Format_Message(void)
{
    char num_text[30];
    int record;
    int out;
    int in;

    out = 0;
    record = (event_msg_event - 1) * NUM_EVENT_MSG_VARIANTS + event_msg_variant;
    LBX_Load_Records_Near("EVENTMSG", 0, p_event_raw, record, 1, 200);
    for (in = 0; in < 200 && p_event_raw[in] != 0; in++) {
        p_event_text[out] = p_event_raw[in];
        switch ((unsigned char)p_event_text[out]) {
        case ETXT_CITY:
            far_strcpy(p_event_text + out, 0, p_cities[event_city].name);
            out = strlen(p_event_text);
            break;
        case ETXT_CITY_SIZE:
            strcpy(p_event_text + out, city_size_names[p_cities[event_city].size]);
            out = strlen(p_event_text);
            break;
        case ETXT_ARG_CITY:
            far_strcpy(p_event_text + out, 0, p_cities[event_msg_arg].name);
            out = strlen(p_event_text);
            break;
        case ETXT_ARG_CITY_SIZE:
            strcpy(p_event_text + out, city_size_names[p_cities[event_msg_arg].size]);
            out = strlen(p_event_text);
            break;
        case ETXT_VICTIM:
            strcpy(p_event_text + out, wizards[event_victim].wizard_name);
            out = strlen(p_event_text);
            break;
        case ETXT_NUMBER:
            p_event_text[out] = 0;
            itoa(event_msg_arg, num_text, 10);
            strcat(p_event_text, num_text);
            out = strlen(p_event_text);
            break;
        case ETXT_MINERAL:
            p_event_text[out] = 0;
            strcat(p_event_text, event_mineral_names[event_msg_arg]);
            out = strlen(p_event_text);
            break;
        case ETXT_ITEM:
            far_strcpy(p_event_text + out, 0, p_items[event_msg_arg].name);
            out = strlen(p_event_text);
            break;
        case ETXT_UNITS_LOST:
            if (event_units_lost > 0) {
                p_event_text[out] = 0;
                itoa(event_units_lost, num_text, 10);
                strcat(p_event_text, num_text);
                out = strlen(p_event_text);
            } else {
                p_event_text[out] = 0;
                strcat(p_event_text, "no");
                out = strlen(p_event_text);
            }
            break;
        case ETXT_POP_LOST:
            if (event_pop_lost > 0) {
                p_event_text[out] = 0;
                itoa(event_pop_lost, num_text, 10);
                strcat(p_event_text, num_text);
                strcat(p_event_text, " thousand");
                out = strlen(p_event_text);
            } else {
                p_event_text[out] = 0;
                strcat(p_event_text, "no");
                out = strlen(p_event_text);
            }
            break;
        case ETXT_BUILDINGS_LOST:
            if (event_buildings_lost > 0) {
                p_event_text[out] = 0;
                itoa(event_buildings_lost, num_text, 10);
                strcat(p_event_text, num_text);
                out = strlen(p_event_text);
            } else {
                p_event_text[out] = 0;
                strcat(p_event_text, "no");
                out = strlen(p_event_text);
            }
            break;
        case ETXT_UNITS_PLURAL:
            if (event_units_lost != 1) {
                p_event_text[out] = 0;
                strcat(p_event_text, "s");
                out = strlen(p_event_text);
            } else
                out++;
            break;
        case ETXT_BUILDINGS_PLURAL:
            if (event_buildings_lost != 1) {
                p_event_text[out] = 0;
                strcat(p_event_text, "s");
                out = strlen(p_event_text);
            } else
                out++;
            break;
        default:
            out++;
            break;
        }
    }
    p_event_text[out] = 0;
}

/* (good) -> a wizard weighted by power + research + army statistics (good events: 600 - that, so the weak
   get them), the weights halved until they total at most 500; -1 when all are 0 */
int Event_Pick_Wizard(int good)
{
    int i;
    int total;
    int k;
    int picked;
    int scaled;                                 /* the weights are scaled down enough */
    int weights[6];

    for (i = 0; i < 6; i++)
        weights[i] = 0;
    for (i = 0; i < num_players; i++)
        weights[i] = wizards[i].stat_magic_power + wizards[i].stat_spell_research + wizards[i].stat_army_strength;
    if (good == 1) {
        for (i = 0; i < num_players; i++) {
            weights[i] = 600 - weights[i];
            if (weights[i] < 1)
                weights[i] = 0;
        }
    }
    scaled = 0;
    while (scaled == 0) {
        total = 0;
        for (i = 0; i < num_players; i++)
            total = total + weights[i];
        if (total > 500)
            for (i = 0; i < num_players; i++)
                weights[i] = weights[i] / 2;
        else
            scaled = 1;
    }
    if (total < 1)
        return -1;
    k = Random(total) - 1;
    picked = 0;
    while (k >= 0 && picked < num_players - 1) {
        k -= weights[picked];
        if (k >= 0)
            picked++;
    }
    return picked;
}

/* (player) -> a city of the player, scanning from a random one; a city whose race is unhappy under the
   wizard's home race (the race table above 1) is passed over; -1 when none */
int Event_Pick_City(int player)
{
    int found;
    int k;
    int city;

    if (num_cities < 1)
        return -1;
    city = Random(num_cities) - 1;
    found = -1;
    k = 0;
    while (k < num_cities && found == -1) {
        if (p_cities[city].owner == player)
            found = city;
        if (found != -1 && p_race_unrest_rows[wizards[player].home_race][p_cities[city].race] > 1)   /* the races hostile to the home race */
            found = -1;
        city = (city + 1) % num_cities;
        k++;
    }
    return found;
}
