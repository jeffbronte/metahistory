#include <sndfile.h>
#include <ctype.h>
#include "ipserver.h"
#include "debug.h"

static int wavefile_read_file(globals_t *g, const char *filename, int16_t **buf, int *bufsize)
{
    int result = 0;
    SF_INFO sfInfo = {0};
    
    // Open the WAV file
    SNDFILE* sndfile = sf_open(filename, SFM_READ, &sfInfo);
    
    // Check for errors during the open operation
    if(NULL == sndfile)
    {
        result = sf_error(sndfile);
    }
    else
    {
        int numChannels = sfInfo.channels;
        int numFrames = sfInfo.frames;
        if (debug) printf("%s: \tchannels %d frames \t%d buffersize \t%d\n", filename, numChannels, numFrames, numFrames*2);
        *buf = calloc(numFrames*2,1);
        sf_readf_short(sndfile, *buf, numFrames);
        *bufsize = numFrames;
    }
    
    // Close WAV file
    sf_close(sndfile);
    return result;
}

static void wavefile_handle_setting(globals_t *g, const char* sctrl, const char *filename, int lineno)
{
    wav_ctrls_e    ctrl;
    
    if      (!strcmp(sctrl, "VHF_L")) { ctrl = wVHF_L; }
    else if (!strcmp(sctrl, "VHF_C")) { ctrl = wVHF_C; }
    else if (!strcmp(sctrl, "VHF_R")) { ctrl = wVHF_R; }
    else if (!strcmp(sctrl, "CAB"))   { ctrl = wCAB;   }
    else if (!strcmp(sctrl, "PA"))    { ctrl = wPA;    }
    else if (!strcmp(sctrl, "HF_L"))  { ctrl = wHF_L;  }  
    else if (!strcmp(sctrl, "HF_R"))  { ctrl = wHF_R;  }  
    else if (!strcmp(sctrl, "SAT_1")) { ctrl = wSAT_1; }  
    else if (!strcmp(sctrl, "SAT_2")) { ctrl = wSAT_2; }  
    else if (!strcmp(sctrl, "SPKR"))  { ctrl = wSPKR;  }  
    else if (!strcmp(sctrl, "VOR_L")) { ctrl = wVOR_L; }  
    else if (!strcmp(sctrl, "VOR_R")) { ctrl = wVOR_R; }  
    else if (!strcmp(sctrl, "ADF_L")) { ctrl = wADF_L; }  
    else if (!strcmp(sctrl, "ADF_R")) { ctrl = wADF_R; }  
    else if (!strcmp(sctrl, "APP_L")) { ctrl = wAPP_L; }  
    else if (!strcmp(sctrl, "APP_R")) { ctrl = wAPP_R; }  
    else if (!strcmp(sctrl, "APP_MKR")) { ctrl = wAPP_MKR; }
    else { 
        fprintf(stderr,"error parsing line %d of config\n", lineno);
        exit(-1);
    }                                
    g->wavedata[ctrl].ctrl = ctrl;
    g->wavedata[ctrl].filename = strdup(filename);
    wavefile_read_file(g, filename, &g->wavedata[ctrl].buf, &g->wavedata[ctrl].bufsize16);
}

static void wavefile_load_config(globals_t *g, const char *filename)
{
    char  line[256];
    int   n_lines=0,got_equals;
    char *token=NULL,*equals=NULL,*endptr=NULL,*rest=NULL,*value=NULL;
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
            fprintf(stderr,"could not open wavefile config %s, error %m\n", filename);
            exit(-1);
    }

   while (fgets(line,sizeof(line),fp)) {
        ++n_lines;

        token = strtok_r(line," \t\n",&endptr);
        if (!token)
            continue;   // skip blank lines

        if (token[0] == '#')
            continue;   // skip comment lines

        equals=strchr(token,'=');
        if (equals) {
            *equals = '\0';
            got_equals = 1;

            // we got the 'token =' but is it 'token = value' or 'token =value'?
            value   = equals+1;
            if (*value != '\0' && !isspace(*value)) {
                wavefile_handle_setting(g,token,value,n_lines);
                //printf("%s=%s\n", token,value);
                continue;
            }
            // otherwise, we will have to read more using strtok
        }else
            got_equals = 0;

        // cannot handle array values for now
        rest = strtok_r(NULL," \t\n",&endptr);
        if (!rest && !got_equals) {
            fprintf(stderr," missing '=' after %s on line %d of %s\n",token,n_lines,filename);
            exit(-1);
        }

        if (got_equals)
            value = rest;
        else{
            // we better get an '=' as first character
            if (rest[0] != '=') {
               fprintf(stderr," missing '=' after %s on line %d of %s\n",token,n_lines,filename);
               exit(-1);
            }

            // we got the 'token =' but is it 'token = value' or 'token =value'?
            value = rest+1;

            if (*value == '\0' || isspace(*value)) {
                value = strtok_r(NULL," \t\n",&endptr);
            }
        }
        if (value) {
            wavefile_handle_setting(g,token,value,n_lines);
            //printf("%s=%s\n", token,value);
        }
    }
}                                                                       

// this maps the wavfile to the buffers for mixing
static wav_ctrls_e wavefile_lookup_src(globals_t *g, r_sources_e src)
{
    switch(src) {
        case aVHF_L : return wVHF_L;
        case aVHF_C : return wVHF_C;
        case aVHF_R : return wVHF_R;
        case aCAB   : return wCAB;
        case aPA    : return wPA;
        case aHF_L  : return wHF_L;
        case aHF_R  : return wHF_R;
        case aSAT_1 : return wSAT_1;
        case aSAT_2 : return wSAT_2;
        case aSPK   : return wSPKR;
        case aNAV   :
            switch(g->tablet->Nav_Select_Code) {
                case NAV_SELECT_CODE_VOR_L: return wVOR_L;
                case NAV_SELECT_CODE_VOR_R: return wVOR_R;
                case NAV_SELECT_CODE_ADF_L: return wADF_L;
                case NAV_SELECT_CODE_ADF_R: return wADF_L;
                default: return -1;
            }
        break;
        case aAPP   :
            switch(g->tablet->App_Select_Code) {
                case APP_SELECT_CODE_APP_L : return wAPP_L;
                case APP_SELECT_CODE_APP_R : return wAPP_R;
                case APP_SELECT_CODE_MKR   : return wAPP_MKR;
                default: return -1;
            }
        break;
        default: break;
    }
    return -1;
}

// fetches a block of data from the wave file buffers
int wavefile_next_block(globals_t *g, wav_ctrls_e wav, int16_t *buf16, int size16)
{
    if (wav != g->wavedata[wav].ctrl)
        return 0;

    if (size16 > g->wavedata[wav].bufsize16) return -1;
    if (g->wavedata[wav].bufpos16 + size16 > g->wavedata[wav].bufsize16)  g->wavedata[wav].bufpos16 = 0;

    memcpy(buf16, g->wavedata[wav].buf+g->wavedata[wav].bufpos16, size16*2);
    g->wavedata[wav].bufpos16 += size16;    

    //printf("%d %d %d\n", wav, g->wavedata[wav].bufpos16, size16);
    return 0;
}


// for all active sources, update the wav data buffers for mixing
void wavefile_next_blocks(globals_t *g, int size)
{
    wav_ctrls_e wav;

    for (int i=aVHF_L; i<amax; i++) {
        // enables is set by the tablet_read()
        if (g->r_enables[i]) {
            wav = wavefile_lookup_src(g, i);
            if (wav == -1) {
                fprintf(stderr,"control to buffer loopup error, %d\n", i);
                exit(-1);
            }
//DEBUG("%d %d %d %d\n",i, wav, size,  ); 
            wavefile_next_block(g, wav, g->r_sources[i], size);
        }
    }    
}

void wavefile_init(globals_t *g)
{
    FUNC;
    memset(g->wavedata, 0, sizeof(g->wavedata));
    wavefile_load_config(g, g->wavefile_cfg);
}


