#include "ipserver.h"
#include "debug.h"

#if 1

// handle when others talk  PTT INT or 
int process_loop(globals_t *g)
{
    mic_e mic       = MIC_OFF;  // local PTT
    mic_e mic_en    = MIC_OFF;  // local enables
    mic_e flt_off1  = MIC_OFF;  // other is talking
    mic_e flt_off2  = MIC_OFF;  // other is talking
    bool flt_en     = false;
    bool radio_en   = false;

    // read from network audio, this meters our loop for 48khz
    audio_read(g);

    // read from tablet
    tablet_read(g);

    // get state of local mic switches
    mic         = active_mic_int(g);  
    mic_en      = active_mic_en(g);   

    // get state of remote PTT mic switches, setup for listen
    active_offside(g, &flt_off1, &flt_off2, &flt_en);  // MIC-INT from others

    // get state of radio switches (except FLT)
    radio_en    = active_radio(g);

    // nothing on
    if (!radio_en && !flt_off1 && !flt_off2) 
        goto silence;

    // somebody else is talking, and my FLT vol en
    if ((flt_off1 || flt_off2) &&  flt_en ) {
        // turn radios off while listening to flt
        radio_en = false;
        perform_flight_mixing(g, flt_off1, flt_off2);
    }

    // I am talking
    if (mic) {
        // TODO next, handle mic en + vol en + PTT MIC, i.e. talking on VHF_L

        // turn radios off while talking
        radio_en = false;
    }

    if (radio_en)  {
        // update active wave data
        wavefile_next_blocks(g, DEFAULT_PERIOD);
        // mix 
        perform_radio_mixing(g, false);
    }

output:
    // output 
    audio_write(g);
    return 1;

silence:
    // output 
    audio_silence(g);

    return 1;
}
#endif


#if 0
// just handles when others talk by PTT INT or PTT MIC + FLT (mic | en)
int process_loop(globals_t *g)
{
    mic_e flt_off1  = MIC_OFF;  // other is talking
    mic_e flt_off2  = MIC_OFF;  // other is talking

    // read from network audio, this meters our loop for 48khz
    audio_read(g);

    // read from tablet
    tablet_read(g);

    // get state of remote PTT mic switches
    active_offside(g, &flt_off1, &flt_off2);  // MIC-INT from others

    // somebody else is talking
    if (flt_off1 || flt_off2) {
        perform_flight_mixing(g, flt_off1, flt_off2);
//    } else {
//        goto silence;
    }


    if (g->whoami != CAPTAIN) {
        goto silence;
    }

    audio_write(g);
    return 1;

silence:
    // output 
    audio_silence(g);
    
    return 1;

}
#endif


#if 0
// just handles radio enable/play/volume/mix
int process_loop(globals_t *g)
{
    // read from network audio, this meters our loop for 48khz
    audio_read(g);

    // read from tablet
    tablet_read(g);


    if (active_radio(g)) {
        // update active wave data
        wavefile_next_blocks(g, DEFAULT_PERIOD);
        // mix 
        perform_radio_mixing(g, false);
    } else {
        goto silence;
    }

    // output 
    audio_write(g);
    return 1;

silence:
    // output 
    audio_silence(g);
    return 1;

}
#endif

#if 0
// just polls offside switches
int process_loop(globals_t *g)
{
    mic_e flt_off1  = MIC_OFF;  // other is talking
    mic_e flt_off2  = MIC_OFF;  // other is talking

    // read from network audio, this meters our loop for 48khz
    audio_read(g);

    // read from tablet
    tablet_read(g);

    // get state of remote PTT mic switches
    active_offside(g, &flt_off1, &flt_off2);  // MIC-INT from others

    // output 
    audio_silence(g);
    return 1;
}
#endif
