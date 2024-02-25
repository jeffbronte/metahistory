#ifndef _IPSERVER_H_
#define _IPSERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <alsa/asoundlib.h> 

#include "AoIPDataStateTabletTypes.h"
#include "udp.h"

#define EXIT { fprintf(stderr, "error at %s:%d\n", __FILE__,__LINE__);fflush(stderr);exit(-1);}


#define DEFAULT_PERIOD      48
#define BLOCK_SIZE          DEFAULT_PERIOD*sizeof(int16_t)

#define R_SOURCES           12
#define F_SOURCES           2

extern int debug;
extern pthread_barrier_t barrier;
typedef short fract16;

typedef enum
{
    CAPTAIN,
    FIRST_OFFICER,
    OBSERVER,
} whoami_e;

typedef enum 
{
    aVHF_L,
    aVHF_C,
    aVHF_R,
    aCAB,
    aPA,
    aHF_L,
    aHF_R,
    aSAT_1,
    aSAT_2,
    aSPK,
    aNAV,
    aAPP,
    amax,
} r_sources_e;

typedef enum 
{
    off_1,
    off_2,
} f_sources_e;

// general enums for both mic enables and flt mic
typedef enum
{
    MIC_OFF,
    MIC,
    INT,
    VHF_L,
    VHF_C,
    VHF_R,
    FLT,
    CAB,
    PA,
    HF_L,
    HF_R,
    SAT_1,
    SAT_2,
    micmax,
} mic_e;

typedef enum
{
    wVHF_L,
    wVHF_C,
    wVHF_R,
    wCAB,
    wPA,
    wHF_L,
    wHF_R,   
    wSAT_1,  
    wSAT_2,  
    wSPKR,   
    wVOR_L,  
    wVOR_R,  
    wADF_L,  
    wADF_R,  
    wAPP_L,  
    wAPP_R,  
    wAPP_MKR,
    wmax,
} wav_ctrls_e;

typedef struct _wav
{
    wav_ctrls_e     ctrl;       /* while this matches the index, it might need different mapping? */    
    char            *filename;
    int16_t         *buf;
    int             bufsize16;
    int             bufpos16;
} wav_t;

typedef struct globals_s
{
    // config
    whoami_e                    whoami;    
    char                        *audio_device;
    char                        *wavecfg;
    char                        *tablet_ip;
    char                        *tablet_port;

    // audio data int16
    snd_pcm_t                   *chandle;
    snd_pcm_t                   *phandle;
    snd_pcm_uframes_t           audio_bufsize;  // in int16_t
    int16_t                     *audio_buf;
    int16_t                     *flight_buf;
    int16_t                     silence_buf[BLOCK_SIZE];

    // radio buffers
    int16_t                     r_sources[R_SOURCES][BLOCK_SIZE];
    int                         r_enables[R_SOURCES];
    uint16_t                    r_volumes[R_SOURCES];

    // flight buffers
    int16_t                     f_sources[F_SOURCES][BLOCK_SIZE];
    int                         f_enables[F_SOURCES];
    uint16_t                    f_volumes[F_SOURCES];

    // tablet data
    mic_e                       last_mic_int;
    mic_e                       last_mic_en;
    mic_e                       last_mic_off1;
    mic_e                       last_mic_off2;
    unsigned int                last[40];
    udp_common_t                udpc;
    AoIPDataStateTabletType_t   *tablet;
    unsigned char               tablet_buffer[40];

    // wav file data
    wav_t                       wavedata[wmax];       
    char                        *wavefile_cfg;

} globals_t; 


extern pthread_t c_pthread;
extern pthread_t f_pthread;
extern pthread_t o_pthread;

extern globals_t *C;
extern globals_t *F; 
extern globals_t *O; 

extern void tablet_init(globals_t *g);
extern void tablet_read(globals_t *g);
extern bool active_radio(globals_t *g);
extern mic_e active_mic_int(globals_t *g);
extern mic_e active_mic_en(globals_t *g);
extern void active_offside(globals_t *g, mic_e *offs1, mic_e *offs2, bool *flt_en);

extern void audio_init(globals_t *g);
extern int audio_read(globals_t *g);
extern int audio_read_buf(globals_t *g, int16_t *buf, int size16);
extern int audio_write(globals_t *g);
extern int audio_write_buf(globals_t *g, int16_t *buf, int size16);
extern void audio_silence(globals_t *g);

extern void wavefile_init(globals_t *g);
extern void wavefile_next_blocks(globals_t *g, int size16);
extern int wavefile_next_block(globals_t *g, wav_ctrls_e ctrl, int16_t *buf16, int size16);

extern void perform_flight_mixing(globals_t *g, mic_e off1, mic_e off2);
extern void perform_radio_mixing(globals_t *g, bool mute);
extern const int16_t volume_gain_table[4096];

extern void flight_io_init(globals_t *g);
extern int  flight_read(globals_t *g, int size16);
extern void flight_write(globals_t *g, int16_t *buf, int size16);


#endif // _IPSERVER_H_


