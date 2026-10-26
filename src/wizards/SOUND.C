/* SOUND.C - WIZARDS.EXE code segment 3151 (MAGIC.EXE 2fc5, the same file): music and sound effects through the
   Miles Audio Interface Library (AIL 2): driver load from SNDDRV.LBX, the BLASTER variable, XMIDI sequences with
   their timbres (FAT.<driver suffix>), VOC sound effects, volume, and the sound start-up error messages.
   A sound resource starts with the word DEAFh and its type (1 XMIDI, 2 VOC) in a 16-byte header; the header's
   +4 word (sound - 1, offset 4) becomes DEADh (SOUND_VOC_FORMATTED) once AIL_format_VOC_file has run on its VOC block.
   The string helpers are called without <string.h> / <ctype.h>: strlen compares signed, isalnum / toupper are
   calls, not the <ctype.h> macros.
   Matching target: Borland C++ 3.1, BCC -c -mm -a -d. Check: python -P tools/match_build.py WIZARDS 3151 src/wizards/SOUND.C */
/* Called here with no prototype in scope, as in the original (a prototype would reject these argument counts):
     DOS_File_Close (1; DOSFILE.C takes none);
     DOS_File_Read_Far (5 and 6; DOSFILE.C takes 4);
     DOS_File_Seek (4; DOSFILE.C takes 3). */
#pragma warn -pro
#include "STRUCTS.H"
#include "ENUMS.H"

/* BSS of this module (link test, batch 36), in DESCENDING address order: Borland lays _BSS out in
   reverse declaration order */
int timbre_file;                                /* DS:e918  MAGIC DS:a90e   */
char timbre_file_name[16];                      /* DS:e908  MAGIC DS:a8fe   */
void far *sound_xmidi_data;                     /* DS:e904  MAGIC DS:a8fa   */
void far *sound_digi_data;                      /* DS:e900  MAGIC DS:a8f6   */
struct AILDriverDesc far *music_drvr_desc;      /* DS:e8fc  MAGIC DS:a8f2   */
struct AILDriverDesc far *digi_drvr_desc;       /* DS:e8f8  MAGIC DS:a8ee   */
void far *music_drvr_image;                     /* DS:e8f4  MAGIC DS:a8ea   */
void far *digi_drvr_image;                      /* DS:e8f0  MAGIC DS:a8e6   */
void far *timbre_data;                          /* DS:e8ec  MAGIC DS:a8e2   */
int far *timbre_buf;                        /* DS:e8e8  MAGIC DS:a8de   */
char drvr_suffix[22];                           /* DS:e8d2  MAGIC DS:a8c8   */
struct TimbreEntry timbre_entry;                /* DS:e8cc  MAGIC DS:a8c2   */
int blaster_type;                               /* DS:e8ca  MAGIC DS:a8c0   */
int blaster_dma_spare[3];                       /* DS:e8c4  MAGIC DS:a8ba  no code in either program reads it */
int blaster_dma;                                /* DS:e8c2  MAGIC DS:a8b8   */
int blaster_irq;                                /* DS:e8c0  MAGIC DS:a8b6   */
int blaster_port;                               /* DS:e8be  MAGIC DS:a8b4   */
int timbre_size_spare[6];                       /* DS:e8b2  MAGIC DS:a8a8  no code in either program reads it */
unsigned timbre_size;                           /* DS:e8b0  MAGIC DS:a8a6   */

/* AIL 2 (Miles Design), the calls this module uses */
void far cdecl AIL_startup(void);
void far cdecl AIL_shutdown(char far *signoff_msg);
int far cdecl AIL_register_driver(void far *driver_base_addr);
struct AILDriverDesc far * cdecl far AIL_describe_driver(int driver);
unsigned far cdecl AIL_detect_device(int driver, unsigned io, unsigned irq, unsigned dma, unsigned drq);
void far cdecl AIL_init_driver(int driver, unsigned io, unsigned irq, unsigned dma, unsigned drq);
void far cdecl AIL_format_VOC_file(int driver, void far *voc_file, int block_marker);
void far cdecl AIL_play_VOC_file(int driver, void far *voc_file, int block_marker);
void far cdecl AIL_start_digital_playback(int driver);
void far cdecl AIL_stop_digital_playback(int driver);
unsigned far cdecl AIL_state_table_size(int driver);
int far cdecl AIL_register_sequence(int driver, void far *form_xmid, unsigned sequence_num, void far *state_table,
                                    void far *controller_table);
void far cdecl AIL_release_sequence_handle(int driver, int sequence);
unsigned far cdecl AIL_default_timbre_cache_size(int driver);
void far cdecl AIL_define_timbre_cache(int driver, void far *cache_addr, unsigned cache_size);
unsigned far cdecl AIL_timbre_request(int driver, int sequence);
void far cdecl AIL_install_timbre(int driver, int bank, int patch, void far *src_addr);
void far cdecl AIL_start_sequence(int driver, int sequence);
void far cdecl AIL_stop_sequence(int driver, int sequence);
unsigned far cdecl AIL_sequence_status(int driver, int sequence);
unsigned far cdecl AIL_relative_volume(int driver, int sequence);
void far cdecl AIL_set_relative_volume(int driver, int sequence, unsigned percent, unsigned milliseconds);

void far *Seg_To_Far_Ptr(unsigned seg);
unsigned Heap_Alloc_Block(unsigned paragraphs);
void far_strcpy(char *dst, int dst_seg, char far *src);
int Sound_Play(unsigned sound);
int Music_Sequence_Status(void);
int Sound_Parse_Blaster(void);
void Sound_Fatal_Error(int code);
int DOS_File_Read_Far();                       /* no prototype here: called with 5 words - buffer offset, segment (0 = DS), size, count AND the handle, which DOSFILE.C ignores */
int DOS_File_Seek();                           /* no prototype here: the long position is passed as two words (offset, 0) */
void DOS_File_Close();                         /* no prototype here: called with the handle; DOSFILE.C takes none (it closes the stored one) */


/* (DS:82a2..82a7 are UI.C's last literals "\x05", "\x06", "_" - batch 31 corrected the earlier reading of them as
   three unused SOUND.C ints 5, 6, 95) */
int music_driver_type = 0;                      /* DS:82a8 */
int digi_driver_type = 0;                       /* DS:82aa */
int sound_initialized = 0;                      /* DS:82ac */
int music_driver_handle = -1;                   /* DS:82ae */
int digi_driver_handle = -1;                    /* DS:82b0 */
int music_sequence_handle = -2;                 /* DS:82b2 */
int sound_unused_seq = -2;                      /* DS:82b4  no reader */
void far *music_state_table = 0;                /* DS:82b6 */
int sound_unused_ba = 0;                        /* DS:82ba  no reader */
int sound_unused_bc = 0;                        /* DS:82bc  no reader */
#define SOUND_OUT_OFF 0       /* sound_output_mode: no sound, */
#define SOUND_OUT_EFFECTS 1   /* the sound effects only, */
#define SOUND_OUT_MUSIC 2     /* the effects and the XMIDI music */
#define SEQ_DONE 2            /* Music_Sequence_Status: the AIL status 0 stopped, 1 playing, 2 done */
int sound_output_mode = SOUND_OUT_OFF;          /* DS:82be */
char blaster_keys[] = "AIDT";                   /* DS:82c0 */
int blaster_radix[4] = { 16, 10, 10, 10 };      /* DS:82c6 */
int *blaster_targets[4] = { &blaster_port, &blaster_irq, &blaster_dma, &blaster_type };     /* DS:82ce */

/* (file, bank, patch) the timbre (bank, patch) from the open timbre file: scans the 6-byte index from the start
   (bank -1 ends it), reads the word length and the data into timbre_buf. Returns it, 0 when not found */
void far *Timbre_Load(int file, int bank, int patch)
{
    if (file == 0)
        return 0;
    DOS_File_Seek(file, 0L, 0);
    do {
        DOS_File_Read_Far(&timbre_entry, 0, 6, 1, file);
        if (timbre_entry.bank == -1)
            return 0;
    } while (timbre_entry.bank != bank || timbre_entry.patch != patch);
    DOS_File_Seek(file, timbre_entry.offset, 0, 0);
    DOS_File_Read_Far(&timbre_size, 0, 2, 1, file, 0);
    *timbre_buf = timbre_size;
    DOS_File_Read_Far(timbre_buf + 1, timbre_size - 2, 1, file);
    return timbre_buf;
}

/* (music driver, mode, io, irq, dma, digi driver, io, irq, dma) loads and starts the AIL drivers (-1 = the driver's
   default port / IRQ / DMA; on a failed detect the defaults are tried again), sets sound_output_mode (0 off, 1 sound
   effects only, 2 music too), allocates the state table, timbre cache and timbre buffer. Drivers 4 / 5 (Sound
   Blaster) are replaced by the BLASTER card type; drivers 9 / 10 play SNDDRV.LBX entry 20 / 21 to the end first.
   Errors end the program (Sound_Fatal_Error). Returns 1.
   Simtex name Audio_Init (MoO2 symbols, module sound); clarified using ReMoM repo */
int Sound_Init(int music_drv, int sound_mode, int music_io, int music_irq, int music_dma, int digi_drv,
               int digi_io, int digi_irq, int digi_dma)
{
    int card;
    int seq_a;
    int seq_b;
    int music_override;
    unsigned cache_seg;
    int unused;
    unsigned state_seg;
    unsigned buf_seg;
    int digi_override;
    void far *cache;
    unsigned long state_size;
    int new_music_io;
    int new_music_irq;
    int new_music_dma;
    int old_music_io;
    int old_music_irq;
    int old_music_dma;
    int old_digi_io;
    int old_digi_dma;
    int old_digi_irq;
    int new_digi_io;
    int new_digi_irq;
    int new_digi_dma;
    unsigned cache_size;

    unused;                                     /* the frame has a slot here that no code uses (the bytes) */
    music_override = 0;
    digi_override = 0;
    music_driver_type = music_drv;
    digi_driver_type = digi_drv;
    if (sound_initialized != 0)
        return 1;
    if (music_io != -1 || music_irq != -1 || music_dma != -1) {
        music_override = 1;
        new_music_io = music_io;
        new_music_irq = music_irq;
        new_music_dma = music_dma;
    }
    if (digi_io != -1 || digi_irq != -1 || digi_dma != -1) {
        digi_override = 1;
        new_digi_io = digi_io;
        new_digi_irq = digi_irq;
        new_digi_dma = digi_dma;
    }
    if (music_drv == SOUND_DRV_NONE && digi_drv == SOUND_DRV_NONE) {
        sound_output_mode = SOUND_OUT_OFF;
        return 1;
    } else if (music_drv == SOUND_DRV_SPEAKER)
        sound_output_mode = SOUND_OUT_EFFECTS;
    else {
        if (sound_mode == 1 && digi_drv == 0) {
            sound_output_mode = SOUND_OUT_OFF;
            return 1;
        } else
            sound_output_mode = SOUND_OUT_EFFECTS;
        if (sound_mode == 2 && music_driver_type > SOUND_DRV_SPEAKER)
            sound_output_mode = SOUND_OUT_MUSIC;
        else if (digi_drv == 0) {
            sound_output_mode = SOUND_OUT_OFF;
            return 1;
        } else
            sound_output_mode = SOUND_OUT_EFFECTS;
    }
    if (music_driver_type == SOUND_DRV_SB_PRO || music_driver_type == SOUND_DRV_SB_PRO_2) {
        card = Sound_Parse_Blaster();
        if (card == 0)
            Sound_Fatal_Error(SND_ERR_BLASTER);
        switch (card) {
        case BLASTER_SB:
            music_driver_type = SOUND_DRV_SB;
            music_drv = SOUND_DRV_SB;
            break;
        case BLASTER_SB_20:
            music_driver_type = SOUND_DRV_SB;
            music_drv = SOUND_DRV_SB;
            break;
        case BLASTER_SB_PRO:
            music_driver_type = SOUND_DRV_SB_PRO;
            music_drv = SOUND_DRV_SB_PRO;
            break;
        case BLASTER_SB_PRO_2:
            music_driver_type = SOUND_DRV_SB_PRO_2;
            music_drv = SOUND_DRV_SB_PRO_2;
            break;
        }
    }
    if (music_driver_type > SOUND_DRV_SPEAKER)
        music_drvr_image = Seg_To_Far_Ptr(LBX_Load("SNDDRV.LBX", (music_drv - 1) * 2));
    if (digi_driver_type > 0)
        digi_drvr_image = Seg_To_Far_Ptr(LBX_Load("SNDDRV.LBX", (digi_driver_type - 1) * 2 + 1));
    AIL_startup();
    if (music_driver_type > SOUND_DRV_SPEAKER)
        music_driver_handle = AIL_register_driver(music_drvr_image);
    if (digi_driver_type > 0)
        digi_driver_handle = AIL_register_driver(digi_drvr_image);
    if (music_driver_handle == -1 && music_driver_type > SOUND_DRV_SPEAKER) {
        AIL_shutdown(0L);
        sound_output_mode = SOUND_OUT_OFF;
        Sound_Fatal_Error(SND_ERR_MUSIC_REGISTER);
    }
    if (digi_driver_handle == -1 && digi_driver_type > 0) {
        AIL_shutdown(0L);
        sound_output_mode = SOUND_OUT_OFF;
        Sound_Fatal_Error(SND_ERR_DIGI_REGISTER);
    }
    if (music_driver_type > SOUND_DRV_SPEAKER)
        music_drvr_desc = AIL_describe_driver(music_driver_handle);
    if (digi_driver_type > 0)
        digi_drvr_desc = AIL_describe_driver(digi_driver_handle);
    if (music_drvr_desc->drvr_type != AIL_DRV_XMIDI && music_driver_type > SOUND_DRV_SPEAKER) {
        AIL_shutdown(0L);
        sound_output_mode = SOUND_OUT_OFF;
        Sound_Fatal_Error(SND_ERR_NOT_XMIDI);
    }
    if (digi_drvr_desc->drvr_type != AIL_DRV_DSP && digi_driver_type > 0) {
        AIL_shutdown(0L);
        sound_output_mode = SOUND_OUT_OFF;
        Sound_Fatal_Error(SND_ERR_NOT_DSP);
    }
    if (music_override != 0) {
        if (music_dma != -1) {
            old_music_dma = music_drvr_desc->dma;
            music_drvr_desc->dma = new_music_dma;
        }
        if (music_io != -1) {
            old_music_io = music_drvr_desc->io;
            music_drvr_desc->io = new_music_io;
        }
        if (music_irq != -1) {
            old_music_irq = music_drvr_desc->irq;
            music_drvr_desc->irq = new_music_irq;
        }
    }
    if (digi_override != 0) {
        if (digi_dma != -1) {
            old_digi_dma = digi_drvr_desc->dma;
            digi_drvr_desc->dma = new_digi_dma;
        }
        if (digi_io != -1) {
            old_digi_io = digi_drvr_desc->io;
            digi_drvr_desc->io = new_digi_io;
        }
        if (digi_irq != -1) {
            old_digi_irq = digi_drvr_desc->irq;
            digi_drvr_desc->irq = new_digi_irq;
        }
    }
    if (music_driver_type > SOUND_DRV_SPEAKER) {
        if (AIL_detect_device(music_driver_handle, music_drvr_desc->io, music_drvr_desc->irq, music_drvr_desc->dma,
                              music_drvr_desc->drq) == 0) {
            if (music_override != 0) {
                music_drvr_desc->dma = old_music_dma;
                music_drvr_desc->io = old_music_io;
                music_drvr_desc->irq = old_music_irq;
                if (AIL_detect_device(music_driver_handle, music_drvr_desc->io, music_drvr_desc->irq,
                                      music_drvr_desc->dma, music_drvr_desc->drq) == 0) {
                    AIL_shutdown(0L);
                    sound_output_mode = SOUND_OUT_OFF;
                    Sound_Fatal_Error(SND_ERR_MUSIC_DETECT);
                }
            } else {
                AIL_shutdown(0L);
                sound_output_mode = SOUND_OUT_OFF;
                Sound_Fatal_Error(SND_ERR_MUSIC_NO_OVERRIDE);   /* detection failed and no override values to undo */
            }
        }
    }
    if (digi_driver_type > 0) {
        if (AIL_detect_device(digi_driver_handle, digi_drvr_desc->io, digi_drvr_desc->irq, digi_drvr_desc->dma,
                              digi_drvr_desc->drq) == 0) {
            if (digi_override != 0) {
                digi_drvr_desc->dma = old_digi_dma;
                digi_drvr_desc->io = old_digi_io;
                digi_drvr_desc->irq = old_digi_irq;
                if (AIL_detect_device(digi_driver_handle, digi_drvr_desc->io, digi_drvr_desc->irq,
                                      digi_drvr_desc->dma, digi_drvr_desc->drq) == 0) {
                    AIL_shutdown(0L);
                    sound_output_mode = SOUND_OUT_OFF;
                    Sound_Fatal_Error(SND_ERR_DIGI_DETECT);
                }
            } else {
                AIL_shutdown(0L);
                sound_output_mode = SOUND_OUT_OFF;
                Sound_Fatal_Error(SND_ERR_DIGI_NO_OVERRIDE);
            }
        }
    }
    if (digi_driver_type > 0)
        AIL_init_driver(digi_driver_handle, digi_drvr_desc->io, digi_drvr_desc->irq, digi_drvr_desc->dma,
                        digi_drvr_desc->drq);
    if (music_driver_type > SOUND_DRV_SPEAKER)
        AIL_init_driver(music_driver_handle, music_drvr_desc->io, music_drvr_desc->irq, music_drvr_desc->dma,
                        music_drvr_desc->drq);
    state_size = AIL_state_table_size(music_driver_handle);
    state_seg = Heap_Alloc_Block((state_size >> 4) + 1);
    music_state_table = Seg_To_Far_Ptr(state_seg);
    strcpy(timbre_file_name, "FAT.");
    far_strcpy(drvr_suffix, 0, music_drvr_desc->file_suffix);
    strcat(timbre_file_name, drvr_suffix);
    cache_size = AIL_default_timbre_cache_size(music_driver_handle);
    if (cache_size > 2000)                      /* the timbre cache: at most 2000 bytes */
        cache_size = 2000;
    if (cache_size != 0) {
        cache_seg = Heap_Alloc_Block(((unsigned long)cache_size >> 4) + 1);
        cache = Seg_To_Far_Ptr(cache_seg);
        AIL_define_timbre_cache(music_driver_handle, cache, cache_size);
    }
    sound_initialized = 1;
    buf_seg = Heap_Alloc_Block(20);
    timbre_buf = Seg_To_Far_Ptr(buf_seg);
    if (music_driver_type == SOUND_DRV_ROLAND) {
        seq_a = LBX_Load("SNDDRV.LBX", 20);
        Sound_Play(seq_a);
        while (Music_Sequence_Status() != SEQ_DONE)
            ;
    }
    if (music_driver_type == SOUND_DRV_ROLAND_2) {
        seq_b = LBX_Load("SNDDRV.LBX", 21);
        Sound_Play(seq_b);
        while (Music_Sequence_Status() != SEQ_DONE)
            ;
    }
    return 1;
}

/* (sound) plays a sound resource: an XMIDI sequence (sound_output_mode 2: replaces the current one, requests its timbres
   from the timbre file and starts it) or a VOC effect (formatted once, then played). Returns 1 when nothing plays (sound off, no driver of
   the kind); after playing it falls off the end with no return value (the callers ignore it).
   Simtex name Play_Sound (MoO2 symbols, module sound); clarified using ReMoM repo */
int Sound_Play(unsigned sound)
{
    int sound_type;
    unsigned timbre;

    if (sound_initialized == 0 || sound_output_mode == SOUND_OUT_OFF)
        return 1;
    if (sound_output_mode == SOUND_OUT_MUSIC) {
        if (Far_Peek_Word(sound, 0) != SOUND_MAGIC) {
            AIL_shutdown(0L);
            Sound_Fatal_Error(SND_ERR_MUSIC_MODE_FILE);
        }
        sound_type = Far_Peek_Word(sound, 2);
        sound++;
        if (sound_type == SOUND_TYPE_XMIDI) {
            if (music_driver_type < SOUND_DRV_ADLIB)
                return 1;
            Music_Release_Sequence();
            sound_xmidi_data = Seg_To_Far_Ptr(sound);
            if (sound_xmidi_data == 0) {
                AIL_shutdown(0L);
                Sound_Fatal_Error(SND_ERR_XMIDI_POINTER);
            }
            if ((music_sequence_handle = AIL_register_sequence(music_driver_handle, sound_xmidi_data, 0,
                                                               music_state_table, 0L)) == -1)
                Sound_Fatal_Error(SND_ERR_SEQUENCE);
            if (music_driver_type != SOUND_DRV_ROLAND_2) {   /* no timbre file for driver 10 */
                timbre_file = DOS_File_Open(timbre_file_name, "rb");
                while ((timbre = AIL_timbre_request(music_driver_handle, music_sequence_handle)) != 0xffff) {
                    timbre_data = Timbre_Load(timbre_file, timbre >> 8, timbre & 0xff);
                    if (timbre_data != 0)
                        AIL_install_timbre(music_driver_handle, timbre >> 8, timbre & 0xff, timbre_data);
                    else {
                        AIL_shutdown(0L);
                        Sound_Fatal_Error(SND_ERR_TIMBRE);
                    }
                }
                if (timbre_file != 0)
                    DOS_File_Close(timbre_file);
            }
            AIL_start_sequence(music_driver_handle, music_sequence_handle);
        } else if (sound_type == SOUND_TYPE_VOC) {
            if (digi_driver_type == 0)
                return 1;
            sound_digi_data = Seg_To_Far_Ptr(sound);
            if (sound_digi_data == 0) {
                AIL_shutdown(0L);
                Sound_Fatal_Error(SND_ERR_VOC_POINTER);
            }
            if (Far_Peek_Word(sound - 1, 4) != SOUND_VOC_FORMATTED) {
                if (digi_driver_handle != -1 && digi_driver_type > 0)
                    AIL_stop_digital_playback(digi_driver_handle);
                Far_Poke_Word(sound - 1, 4, SOUND_VOC_FORMATTED);
                AIL_format_VOC_file(digi_driver_handle, sound_digi_data, -1);
            }
            AIL_play_VOC_file(digi_driver_handle, sound_digi_data, -1);
            AIL_start_digital_playback(digi_driver_handle);
        } else
            Sound_Fatal_Error(SND_ERR_KIND);
    }
    if (sound_output_mode == SOUND_OUT_EFFECTS) {
        if (digi_driver_type == 0)
            return 1;
        if (Far_Peek_Word(sound, 0) != SOUND_MAGIC) {
            AIL_shutdown(0L);
            Sound_Fatal_Error(SND_ERR_DIGI_MODE_FILE);
        }
        sound_type = Far_Peek_Word(sound, 2);
        sound++;
        if (sound_type == SOUND_TYPE_VOC) {
            sound_digi_data = Seg_To_Far_Ptr(sound);
            if (sound_digi_data == 0) {
                AIL_shutdown(0L);
                Sound_Fatal_Error(SND_ERR_DIGI_MODE_VOC_POINTER);
            }
            if (Far_Peek_Word(sound - 1, 4) != SOUND_VOC_FORMATTED) {
                if (digi_driver_handle != -1 && digi_driver_type > 0)
                    AIL_stop_digital_playback(digi_driver_handle);
                AIL_format_VOC_file(digi_driver_handle, sound_digi_data, -1);
                Far_Poke_Word(sound - 1, 4, SOUND_VOC_FORMATTED);
            }
            AIL_play_VOC_file(digi_driver_handle, sound_digi_data, -1);
            AIL_start_digital_playback(digi_driver_handle);
        }
    }
}

/* (percent) music volume 0..100 */
int Music_Set_Volume(int percent)
{
    int volume;

    if (sound_initialized == 0)
        return 0;
    if (music_sequence_handle != -2) {
        if (percent < 0)
            percent = 0;
        if (percent > 100)
            percent = 100;
        volume = percent;
        AIL_set_relative_volume(music_driver_handle, music_sequence_handle, volume, 0);
    }
}

int Music_Get_Volume(void)
{
    int volume;

    if (sound_initialized == 0)
        return 0;
    if (music_sequence_handle != -2) {
        volume = AIL_relative_volume(music_driver_handle, music_sequence_handle);
        return volume;
    } else
        return 0;
}

/* () fades a playing sequence to volume 0 over a second and stops the sound effect */
int Sound_Stop(void)
{
    int volume;

    if (sound_initialized == 0)
        return 0;
    if (music_sequence_handle != -2) {
        volume = 0;
        if (AIL_sequence_status(music_driver_handle, music_sequence_handle) == 1)
            AIL_set_relative_volume(music_driver_handle, music_sequence_handle, volume, 1000);
    }
    if (digi_driver_handle != -1 && digi_driver_type > 0)
        AIL_stop_digital_playback(digi_driver_handle);
}

/* () the music sequence's AIL status (0 stopped, 1 playing, 2 done); 2 when there is no music */
int Music_Sequence_Status(void)
{
    int status;

    if (sound_initialized == 0 || sound_output_mode < 2)
        return 2;
    status = AIL_sequence_status(music_driver_handle, music_sequence_handle);
    return status;
}

/* () stops and releases the music sequence; two empty tests on the digi driver are left in */
int Music_Release_Sequence(void)
{
    if (sound_initialized == 0)
        return 0;
    if (music_driver_type == SOUND_DRV_NONE || sound_output_mode == SOUND_OUT_OFF)
        return 0;
    if (music_driver_type == SOUND_DRV_SPEAKER) {
        if (digi_driver_handle != -1 && digi_driver_type > 0)
            ;
        return;
    }
    if (music_sequence_handle != -2) {
        if (AIL_sequence_status(music_driver_handle, music_sequence_handle) == 1)
            AIL_stop_sequence(music_driver_handle, music_sequence_handle);
        AIL_release_sequence_handle(music_driver_handle, music_sequence_handle);
        music_sequence_handle = -2;
    }
    if (digi_driver_handle != -1)
        ;
}

/* () the AIL drivers shut down (when any was loaded); no sound until Sound_Init again */
void Sound_Shutdown(void)
{
    if ((music_driver_type != SOUND_DRV_NONE || digi_driver_type != 0) && sound_initialized != 0)
        AIL_shutdown("");
    music_driver_handle = -1;
    digi_driver_handle = -1;
    sound_initialized = 0;
}

/* (mode) 0 off, 1 sound effects only, 2 music too (as far as the drivers allow) */
void Sound_Set_Mode(int mode)
{
    switch (mode) {
    case 0:
        if (music_driver_type != SOUND_DRV_NONE) {
            Music_Release_Sequence();
            sound_output_mode = SOUND_OUT_OFF;
        }
        break;
    case 1:
        if (music_driver_type != SOUND_DRV_NONE && digi_driver_type > 0) {
            Music_Release_Sequence();
            sound_output_mode = SOUND_OUT_EFFECTS;
        } else {
            Music_Release_Sequence();
            sound_output_mode = SOUND_OUT_OFF;
        }
        break;
    case 2:
        if (music_driver_type > SOUND_DRV_SPEAKER)
            sound_output_mode = SOUND_OUT_MUSIC;
        else if (music_driver_type == SOUND_DRV_SPEAKER)
            sound_output_mode = SOUND_OUT_EFFECTS;
        else
            sound_output_mode = SOUND_OUT_OFF;
        break;
    }
}

/* () reads the BLASTER variable: A (port, hex), I, D, T (decimal) into blaster_port / _irq / _dma / _type.
   Returns the card type capped at 4, 0 when not set */
int Sound_Parse_Blaster(void)
{
    int digit;
    int key;
    int ch;
    int value;
    int pos;
    char *env;
    char text[128];
    register int i;
    register int k;

    env = getenv("BLASTER");
    if (env == 0)
        return 0;
    strncpy(text, env, 127);
    if (strlen(text) == 0)
        return 0;
    strupr(text);
    for (i = 0; i < strlen(text); i++) {
        if (i == 0 || text[i] == ' ') {
            i += text[i] == ' ' ? 1 : 0;
            key = text[i];
            for (k = 0; k < 4; k++)
                if (blaster_keys[k] == key) {
                    pos = i + 1;
                    value = 0;
                    do {
                        ch = text[pos];
                        pos++;
                        for (digit = 0; digit < blaster_radix[k]; digit++)
                            if (toupper(ch) == "0123456789ABCDEF"[digit])
                                value = blaster_radix[k] * value + digit;
                    } while (isalnum(ch));
                    *blaster_targets[k] = value;
                    break;
                }
        }
    }
    if (blaster_type == 0)
        return 0;
    if (blaster_type > BLASTER_SB_PRO_2)
        blaster_type = BLASTER_SB_PRO_2;
    return blaster_type;
}

/* (code) ends the program with a sound set-up message. Codes 0-8 build theirs with strcpy calls only (meant
   strcpy + strcat), so all of them show just "reconfigure hardware options" (#397)
   Numbered: BUG #346, #397 (BUGS_AND_QUIRKS.txt) */
void Sound_Fatal_Error(int code)
{
    char message[120];

    switch (code) {
    case SND_ERR_BLASTER:
        strcpy(message, "You selected an invalid sound driver, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_MUSIC_REGISTER:
        strcpy(message, "You selected an invalid sound driver, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_DIGI_REGISTER:
        strcpy(message, "You selected an invalid digi driver, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_NOT_XMIDI:
        strcpy(message, "Sound driver is not a valid XMIDI driver, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_NOT_DSP:
        strcpy(message, "Digi driver is not a valid DSP driver, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_MUSIC_DETECT:
        strcpy(message, "Music ");
        strcpy(message, "I/O parameters are incorrect, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_MUSIC_NO_OVERRIDE:
        strcpy(message, "Music I/O parameters are incorrect, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_DIGI_DETECT:
        strcpy(message, "Digi Driver ");
        strcpy(message, "I/O parameters are incorrect, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_DIGI_NO_OVERRIDE:
        strcpy(message, "Digi Driver ");
        strcpy(message, "I/O parameters are incorrect, ");
        strcpy(message, "reconfigure hardware options");
        break;
    case SND_ERR_MUSIC_MODE_FILE:
        strcpy(message, "Not a valid sound file");
        break;
    case SND_ERR_XMIDI_POINTER:
        strcpy(message, "Couldn't load XMIDI file");
        break;
    case SND_ERR_SEQUENCE:
        strcpy(message, "Sequence did not register");
        break;
    case SND_ERR_TIMBRE:
        strcpy(message, "Timbre not found");
        break;
    case SND_ERR_VOC_POINTER:
        strcpy(message, "Could not load");
        strcat(message, " sound file");
        break;
    case SND_ERR_KIND:
        strcpy(message, "Not a valid");
        strcat(message, " sound file");
        break;
    case SND_ERR_DIGI_MODE_FILE:
        strcpy(message, "Not a valid");
        strcat(message, " sound file");
        break;
    case SND_ERR_DIGI_MODE_VOC_POINTER:
        strcpy(message, "Could not load sound file");
        break;
    }
    Fatal_Exit(message);
}
