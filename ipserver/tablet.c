#include "ipserver.h"
#include "debug.h"
#include "udp.h"


// updates the audio buffers with enables and volumes
void tablet_update_values(globals_t *g)
{
    // flight offside sources volumes set by local volume dial
    g->f_volumes[off_1]   = __bswap_16(g->tablet->FLT_Volume);
    g->f_volumes[off_2]   = __bswap_16(g->tablet->FLT_Volume);

    // radio sources
    g->r_enables[aVHF_L]  = g->tablet->VHF_L_VolSel;
    g->r_volumes[aVHF_L]  = __bswap_16(g->tablet->VHF_L_Volume);

    g->r_enables[aVHF_C]  = g->tablet->VHF_C_VolSel;
    g->r_volumes[aVHF_C]  = __bswap_16(g->tablet->VHF_C_Volume);

    g->r_enables[aVHF_R]  = g->tablet->VHF_R_VolSel;
    g->r_volumes[aVHF_R]  = __bswap_16(g->tablet->VHF_R_Volume);

    g->r_enables[aCAB]    = g->tablet->CAB_VolSel;
    g->r_volumes[aCAB]    = __bswap_16(g->tablet->CAB_Volume);

    g->r_enables[aPA]     = g->tablet->PA_VolSel;
    g->r_volumes[aPA]     = __bswap_16(g->tablet->PA_Volume);

    g->r_enables[aHF_R]   = g->tablet->HF_R_VolSel;
    g->r_volumes[aHF_R]   = __bswap_16(g->tablet->HF_R_Volume);

    g->r_enables[aHF_L]   = g->tablet->HF_L_VolSel;
    g->r_volumes[aHF_L]   = __bswap_16(g->tablet->HF_L_Volume);

    g->r_enables[aSAT_1]  = g->tablet->SAT_1_VolSel;
    g->r_volumes[aSAT_1]  = __bswap_16(g->tablet->SAT_1_Volume);

    g->r_enables[aSAT_2]  = g->tablet->SAT_2_VolSel;
    g->r_volumes[aSAT_2]  = __bswap_16(g->tablet->SAT_2_Volume);

    g->r_enables[aSPK]    = g->tablet->SPK_VolSel;
    g->r_volumes[aSPK]    = __bswap_16(g->tablet->SPK_Volume);

    g->r_enables[aNAV]    = g->tablet->NAV_VolSel;
    g->r_volumes[aNAV]    = __bswap_16(g->tablet->NAV_Volume);

    g->r_enables[aAPP]    = g->tablet->APP_VolSel;
    g->r_volumes[aAPP]    = __bswap_16(g->tablet->APP_Volume);
}

bool active_radio(globals_t *g)
{
    for (int i=0; i<R_SOURCES; i++) {
        if (g->r_enables[i]) 
            return 1;
    }
    return 0;
}

// returns PTT local mic currently active
mic_e active_mic_int(globals_t *g)
{
    mic_e mic;

    switch(g->tablet->Mic_Switch_Pos) {
        case MIC_POS: mic = MIC; break;
        case FLT_POS: mic = INT; break;
        default: mic = MIC_OFF; break;
    }

    if (g->last_mic_int != mic) {
        DEBUG("%s %s\n", mic_str(mic), (mic ? "on" : ""));
        g->last_mic_int = mic;
    }            
    return mic;
}


// returns mutually exclusive 'mic' selector currently active
mic_e active_mic_en(globals_t *g)
{
    mic_e mic = MIC_OFF;

    if (g->tablet->FLT_Mic_On)    mic = FLT;
    if (g->tablet->CAB_Mic_On)    mic = CAB;
    if (g->tablet->PA_Mic_On)     mic = PA;
    if (g->tablet->SAT_1_Mic_On)  mic = SAT_1;
    if (g->tablet->SAT_2_Mic_On)  mic = SAT_2;
    if (g->tablet->VHF_L_Mic_On)  mic = VHF_L;
    if (g->tablet->VHF_C_Mic_On)  mic = VHF_C;
    if (g->tablet->VHF_R_Mic_On)  mic = VHF_R;
    if (g->tablet->HF_L_Mic_On)   mic = HF_L;
    if (g->tablet->HF_R_Mic_On)   mic = HF_R;

    if (g->last_mic_en != mic) {
        DEBUG("%s %s\n", mic_str(mic), (mic ? "on" : ""));
        g->last_mic_en = mic;
    }            

    return mic;
}

// returns offside active
void active_offside(globals_t *g, mic_e *offs1, mic_e *offs2, bool *flt_en)
{
    switch(g->whoami) {
        case CAPTAIN:
            *offs1 = ((F->tablet->Mic_Switch_Pos == MIC_POS) && (F->tablet->FLT_Mic_On || F->tablet->FLT_VolSel)) ? MIC : (F->tablet->Mic_Switch_Pos == FLT_POS) ? INT : MIC_OFF;
            *offs2 = ((O->tablet->Mic_Switch_Pos == MIC_POS) && (O->tablet->FLT_Mic_On || O->tablet->FLT_VolSel)) ? MIC : (O->tablet->Mic_Switch_Pos == FLT_POS) ? INT : MIC_OFF;
        break;
        case FIRST_OFFICER: 
            *offs1 = ((C->tablet->Mic_Switch_Pos == MIC_POS) && (C->tablet->FLT_Mic_On || C->tablet->FLT_VolSel)) ? MIC : (C->tablet->Mic_Switch_Pos == FLT_POS) ? INT : MIC_OFF;
            *offs2 = ((O->tablet->Mic_Switch_Pos == MIC_POS) && (O->tablet->FLT_Mic_On || O->tablet->FLT_VolSel)) ? MIC : (O->tablet->Mic_Switch_Pos == FLT_POS) ? INT : MIC_OFF;
        break;
        case OBSERVER: 
            *offs1 = ((F->tablet->Mic_Switch_Pos == MIC_POS) && (F->tablet->FLT_Mic_On || F->tablet->FLT_VolSel)) ? MIC : (F->tablet->Mic_Switch_Pos == FLT_POS) ? INT : MIC_OFF;
            *offs2 = ((C->tablet->Mic_Switch_Pos == MIC_POS) && (C->tablet->FLT_Mic_On || C->tablet->FLT_VolSel)) ? MIC : (C->tablet->Mic_Switch_Pos == FLT_POS) ? INT : MIC_OFF;
        break;
        default: 
            *offs1 = MIC_OFF;
            *offs2 = MIC_OFF;
        break;
    }
    *flt_en = g->tablet->FLT_VolSel ? true : false;

    if (g->last_mic_off1 != *offs1) {
        DEBUG("Offside1 %s %s\n", mic_str(*offs1), (*offs1 ? "on" : ""));
        g->last_mic_off1 = *offs1;
    }
    if (g->last_mic_off2 != *offs2) {
        DEBUG("Offside2 %s %s\n", mic_str(*offs2), (*offs2 ? "on" : ""));
        g->last_mic_off2 = *offs2;
    }
}

void tablet_read(globals_t *g)
{
    unsigned int len;
    int n;
    unsigned char buf[40];

    len = g->udpc.salen;

    n = recvfrom(g->udpc.fd, buf, 40, MSG_DONTWAIT, g->udpc.sa, &len);
    if (n > 0)
    {
        memcpy(g->tablet_buffer, buf, sizeof(g->tablet_buffer));
        if (debug) 
        {
            if (memcmp(buf, g->last, 40)) {
                printf("%c: ",whoami(g->whoami));dump_buffer(buf, n, n);
                memcpy(g->last, buf, 40);
            }
        }
        tablet_update_values(g);
    }
}


// open udp mcast
void tablet_init(globals_t *g)
{
    memset(&g->udpc, 0, sizeof(g->udpc));
    g->udpc.ip             = g->tablet_ip;
    g->udpc.portstr        = g->tablet_port;
    //udpc.interface_clue = iface;

    FUNC;

    if (udp_common_setup( 0, &g->udpc) < 0) {
        fprintf(stderr, "udp_common_setup() failed, exiting\n");
        exit(-1);
    }

    if (udp_common_bind(0, &g->udpc) < 0) {
        fprintf(stderr, "udp_common_bind() failed, exiting\n");
        exit(-1);
    }
    g->tablet = (AoIPDataStateTabletType_t *)g->tablet_buffer;
}



