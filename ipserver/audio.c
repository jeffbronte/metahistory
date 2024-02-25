#include <alsa/asoundlib.h> 
#include "ipserver.h"
#include "debug.h"

#define DEFAULT_RATE         48000
#define ALSA_DEVICE         "hw:RAVENNA"
#define BUFFER_MULTIPLIER   2
#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_FORMAT      SND_PCM_FORMAT_S16
#define DEFAULT_CHANNELS    1

/* Initialize device and return the period */
static snd_pcm_uframes_t initialize_device(snd_pcm_t* handle, snd_pcm_format_t format, int rate, int channels, int period) 
{

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_t* hw_params;
    int err;
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        printf("ERROR: Cannot allocate hardware parameter structure: %s\n", snd_strerror(err));
        return 0;
    }

    if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0) {
        printf("ERROR: Cannot initialize hardware parameter structure: %s\n", snd_strerror(err));
        return 0;
    }

    /* Set parameters */
    snd_pcm_sframes_t ret;
    if ((ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(ret));
        return 0;
    }

    if ((ret = snd_pcm_hw_params_set_rate(handle, hw_params, rate, 0)) < 0) {
        printf("ERROR: Can't set rate. %s\n", snd_strerror(ret));
        return 0;
    }

    if ((ret = snd_pcm_hw_params_set_format(handle, hw_params, format)) < 0) {
        printf("ERROR: Can't set format. %s\n", snd_strerror(ret));
        return 0;
    }

    if ((ret = snd_pcm_hw_params_set_channels(handle, hw_params, channels)) < 0) {
        printf("ERROR: Can't set channels number to %d. %s\n", channels, snd_strerror(ret));
        return 0;
    }

    if((ret = snd_pcm_hw_params_set_period_size(handle, hw_params, period, 0)) < 0 ) {
      printf("ERROR: Can't set period size: %s\n", snd_strerror(ret));
      return 0;
    }

    /* Set the buffer size of the device */
    snd_pcm_hw_params_set_buffer_size(handle, hw_params, BUFFER_MULTIPLIER*period);

    /* Write parameters */
    if ((ret = snd_pcm_hw_params(handle, hw_params)) < 0){
        printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(ret));
        return 0;
    }

    /* Verify buffer size matches */
    snd_pcm_uframes_t buffer_size;
    snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
    if(buffer_size != BUFFER_MULTIPLIER*period) {
        printf("ERROR: returned buffer size does not match set period (requested %d, got %ld)\n", BUFFER_MULTIPLIER*period, buffer_size);
        return 0;
    }


    /* Verify number of channels matches */
    unsigned int returned_channels;
    snd_pcm_hw_params_get_channels(hw_params, &returned_channels);
    if(returned_channels != channels) {
        printf("ERROR: Channels set on %s = %d which does not match %d!\n", ALSA_DEVICE, returned_channels, channels);
        return 0;
    }

    /* Verify number of sample rate matches */
    unsigned int ret_rate;
    snd_pcm_hw_params_get_rate(hw_params, &ret_rate, 0);
    if (rate != ret_rate) {
        printf("ERROR: Rate doesn't match (requested %iHz, got %iHz)\n", rate, ret_rate);
        return 0;
    }

    /* Verify period size matches */
    snd_pcm_uframes_t ret_period;
    snd_pcm_hw_params_get_period_size(hw_params, &ret_period, 0);
    if(ret_period != period) {
        printf("ERROR: returned period does not match set period (requested %d, got %ld)\n", period, ret_period);
        return 0;
    }

    /* Free the malloced hw_params */
    snd_pcm_hw_params_free(hw_params);

    return buffer_size;
}


void audio_init(globals_t *g)
{
    int err;
    snd_pcm_uframes_t  buffer_size_send;
    snd_pcm_uframes_t  buffer_size_recv;
    FUNC;

    if ((err = snd_pcm_open(&g->chandle, g->audio_device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        printf("receive open error: %s\n", snd_strerror(err));
        exit(-1);
    }
    if ((err = snd_pcm_open(&g->phandle, g->audio_device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("send open error: %s\n", snd_strerror(err));
        exit(-1);
    }

    buffer_size_recv = initialize_device(g->chandle, DEFAULT_FORMAT, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS, DEFAULT_PERIOD);
    buffer_size_send = initialize_device(g->phandle, DEFAULT_FORMAT, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS, DEFAULT_PERIOD);

    if (buffer_size_recv != buffer_size_send) {
        fprintf(stderr,"Error with buffer sizes!\n");
        exit(-1);
    }

    g->audio_bufsize = buffer_size_send;
    g->audio_buf  = malloc(g->audio_bufsize*sizeof(int16_t));
    g->flight_buf = malloc(g->audio_bufsize*sizeof(int16_t));

    snd_pcm_prepare(g->chandle);
    snd_pcm_prepare(g->phandle);
    snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
    snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
    snd_pcm_start(g->phandle);
}



int audio_read(globals_t *g)
{
    snd_pcm_sframes_t pcm = snd_pcm_readi(g->chandle, g->audio_buf, DEFAULT_PERIOD);
    if (pcm < DEFAULT_PERIOD) {
        printf("!");fflush(stdout);
        snd_pcm_prepare(g->chandle);
    }
    return (int)pcm;
}

int audio_read_buf(globals_t *g, int16_t *buf, int size16)
{
    snd_pcm_sframes_t pcm = snd_pcm_readi(g->chandle, buf, size16);
    if (pcm < DEFAULT_PERIOD) {
        printf("!");fflush(stdout);
        snd_pcm_prepare(g->chandle);
    }
    return (int)pcm;
}

int audio_write(globals_t *g)
{
    snd_pcm_sframes_t pcm = snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
    if (pcm < DEFAULT_PERIOD) {
        printf("!");fflush(stdout);
        snd_pcm_prepare(g->phandle);
        snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
        snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
        snd_pcm_start(g->phandle);
    }
    return 0;
}

int audio_write_buf(globals_t *g, int16_t *buf, int size16)
{
    snd_pcm_sframes_t pcm = snd_pcm_writei(g->phandle, buf, size16);
    if (pcm < DEFAULT_PERIOD) {
        printf("!");fflush(stdout);
        snd_pcm_prepare(g->phandle);
        snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
        snd_pcm_writei(g->phandle, g->audio_buf, DEFAULT_PERIOD);
        snd_pcm_start(g->phandle);
    }
    return 0;
}

void audio_silence(globals_t *g) 
{
    audio_write_buf(g, g->silence_buf, DEFAULT_PERIOD);
}

