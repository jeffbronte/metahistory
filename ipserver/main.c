#include <stdio.h>
#include <signal.h>
#include <sched.h>
#include <sys/mman.h>   /* mlockall() */
#include <pthread.h>

#include "ipserver.h"
#include "debug.h"

/*---------------------------------------------------------------------------*/
/* System wide globals                                                       */
/*---------------------------------------------------------------------------*/

globals_t       *C;             // data for Captain, First Officer, Observer
globals_t       *F;             
globals_t       *O; 

pthread_t       c_pthread;      // pthread handles, for debug lookup only
pthread_t       f_pthread;
pthread_t       o_pthread;

pthread_barrier_t barrier;      // thread sync

#define PACKET_SIZE 96
int debug = 0;
char loop = 0;


// try the org model
#if 0
int MixFltInterphone(
        globals_t *g,
        fract16 *audio,
        fract16 *audioOffside1,
        fract16 *audioOffside2,
        AoIPDataStateTabletType_t *pTabletData,
        AoIPDataStateTabletType_t *pTabletDataOffside1,
        AoIPDataStateTabletType_t *pTabletDataOffside2)
{
    int have_mix = 0;        

    // Check if Flt Volume is sected or Flt Mic is on
    if ((pTabletData->FLT_VolSel) || (pTabletData->FLT_Mic_On))
    {
        // Mix in Offside1 audio if PTT is enabled for FLT Interphone
        if ((pTabletDataOffside1->Mic_Switch_Pos == FLT_POS) ||
            ((pTabletDataOffside1->Mic_Switch_Pos == MIC_POS) && (pTabletDataOffside1->FLT_Mic_On))) {

            memcpy(g->f_sources[fFLT_a],audioOffside1, PACKET_SIZE); 
            have_mix++;
        }
        // Mix in Offside2 audio if PTT is enabled for FLT Interphone
        if ((pTabletDataOffside2->Mic_Switch_Pos == FLT_POS) ||
                ((pTabletDataOffside2->Mic_Switch_Pos == MIC_POS) && (pTabletDataOffside2->FLT_Mic_On))) {

            memcpy(g->f_sources[fFLT_b],audioOffside2, PACKET_SIZE); 
            have_mix++;
        }
    } 

    // Mix in the local sidetone if PTT enabled
    if ((pTabletData->Mic_Switch_Pos == FLT_POS) ||
            ((pTabletData->Mic_Switch_Pos == MIC_POS) && (pTabletData->FLT_Mic_On))) {

        memcpy(g->f_sources[fFLT],audio, PACKET_SIZE); 
        have_mix++;
    }

    // Mix the buffer above, volumes are set in the arrays, output is in g->flight_buf
    if (have_mix)
        perform_flight_mixing(g);

    return have_mix;
}

static int setup_flight_mixing(globals_t *g)
{
    int have_mix = 0;
    switch(g->whoami) {
        case CAPTAIN: 
            have_mix = MixFltInterphone(g,
                g->audio_buf,
                F->audio_buf,
                O->audio_buf,
                g->tablet,
                F->tablet,
                O->tablet);
        break;
        case FIRST_OFFICER:
            have_mix = MixFltInterphone(g,
                g->audio_buf,
                C->audio_buf,
                O->audio_buf,
                g->tablet,
                C->tablet,
                O->tablet);
        break;
        case OBSERVER:
            have_mix = MixFltInterphone(g,
                g->audio_buf,
                C->audio_buf,
                F->audio_buf,
                g->tablet,
                F->tablet,
                C->tablet);
        break;
        default: break;
    }
    return have_mix;
}

static int process_loop(globals_t *g)
{
    int size;

    // read from network
    size = audio_read(g);

    // read from tablet
    tablet_read(g);

    pthread_barrier_wait(&barrier);

    if (setup_flight_mixing(g)) {
        memcpy(g->audio_buf, g->flight_buf, PACKET_SIZE);
    } else {
        // update active wave data
        wavefile_next_blocks(g, size);
        // mix 
        perform_radio_mixing(g);
    }

    // output 
    audio_write(g);
    return 1;
}


// try the new model
#else
extern int process_loop(globals_t *g);
#endif

static int process_cmdline(int argc, char **argv)
{
    int c;

    /* parse the command options */
    while ((c = getopt(argc, argv, "l:v:")) != -1)
    {
        switch (c) {
            case 'v': debug = atoi(optarg); break;
            case 'l': loop = optarg[0]; break;
            default:
            exit(-1);
        }
    }
    return 0;
}


#define MAX_SAFE_STACK (8 * 1024) /* guaranteed safe without faulting */
static void stack_prefault(void)
{
    unsigned char dummy[MAX_SAFE_STACK];
    memset(dummy, 0, MAX_SAFE_STACK);
}

static void setscheduler(void) {
    struct sched_param sched_param;
    FUNC;

    // Get scheduler paramater data for this process
    if (sched_getparam(0, &sched_param) < 0) {
        printf("sched_getparam failed to obtain process info. Default scheduling in place!\n");
        return;
    }

    // Set the process to be max priority
    sched_param.sched_priority = sched_get_priority_max(SCHED_RR);
    if(sched_setscheduler(0, SCHED_RR, &sched_param)) {
        printf("sched_get_priority failed\n");
        return;
    }

    /* Lock memory */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        fprintf(stderr, "Warning: Failed to lock memory: %s\n",
                strerror(errno));
    }
    stack_prefault();
    printf("Starting RT process\n");
}

void* ipserver_thread(void * pConfig)
{
    globals_t *G = (globals_t *)pConfig;
    printf("starting %s\n", G->audio_device);fflush(stdout);
    while (process_loop(G));
    return NULL;
}

static void loop_only(globals_t *g)
{
    printf("loop only: %c\n", whoami(g->whoami));
    audio_init(g);
    while(1) {
        audio_read(g);
        audio_write(g);
    }    
}


int main(int argc, char **argv)
{
    int result;

    process_cmdline(argc, argv);
    setscheduler();

    C = calloc(sizeof(globals_t),1);
    C->audio_device = strdup("captain");
    C->whoami       = CAPTAIN;
    C->tablet_port  = strdup("51001");
    C->tablet_ip    = strdup("224.224.130.1");
    C->wavefile_cfg = strdup("wavefile.cfg");

    F = calloc(sizeof(globals_t),1);
    F->audio_device = strdup("first_officer");
    F->whoami       = FIRST_OFFICER;
    F->tablet_port  = strdup("51003");
    F->tablet_ip    = strdup("224.224.130.3");
    F->wavefile_cfg = C->wavefile_cfg;

    O = calloc(sizeof(globals_t),1);
    O->audio_device = strdup("observer");
    O->whoami       = OBSERVER;
    O->tablet_port  = strdup("51002");
    O->tablet_ip    = strdup("224.224.130.2");
    O->wavefile_cfg = C->wavefile_cfg;

    if (loop) {
        switch(loop) {
            case 'C': loop_only(C); break;
            case 'F': loop_only(F); break;
            case 'O': loop_only(O); break;
            default: exit(-2);
        }
        return 0;
    }

    wavefile_init(C);
    tablet_init(C);
    audio_init(C);

    wavefile_init(F);
    tablet_init(F);
    audio_init(F);

    wavefile_init(O);
    tablet_init(O);
    audio_init(O);

    result = pthread_create(&c_pthread, NULL, ipserver_thread, C);
    if (0 != result) {
        printf("pthread error: %s\n", strerror(errno));
        EXIT;
    }

    result = pthread_create(&f_pthread, NULL, ipserver_thread, F);
    if (0 != result) {
        printf("pthread error: %s\n", strerror(errno));
        EXIT;
    }

    result = pthread_create(&o_pthread, NULL, ipserver_thread, O);
    if (0 != result) {
        printf("pthread error: %s\n", strerror(errno));
        EXIT;
    }

    while(1) pause();    
    return 0;
}


