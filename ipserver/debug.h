#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "ipserver.h"


static inline void dump_buffer( void * p, int iLen, int iWidth )
{
   unsigned char *pBuf   = (unsigned char *)p;
   int            iCount = 0;
   while( iCount < iLen) {
      if ( iCount && !(iCount % iWidth) ) {fprintf(stdout,"\n");fflush(stdout);}
      fprintf(stdout, "%2.2x ", *pBuf);
      ++pBuf;
      iCount++;
   }
   fprintf(stdout,"\n");fflush(stdout);
}

static inline char *mic_str(mic_e mic)
{
    switch(mic) {
        case MIC_OFF: return "MIC_OFF";   
        case MIC    : return "MIC";   
        case INT    : return "INT";
        case PA     : return "PA";  
        case SAT_1  : return "SAT_1";
        case SAT_2  : return "SAT_2";
        case VHF_L  : return "VHF_L";
        case VHF_C  : return "VHF_C";
        case VHF_R  : return "VHF_R";
        case FLT    : return "FLT";  
        case CAB    : return "CAB";  
        case HF_L   : return "HF_L"; 
        case HF_R   : return "HF_R"; 
        default: break;
    }
    return "";
}

static inline char whoami(int i)
{
    switch(i) {
        case 0: return 'C';
        case 1: return 'F';
        case 2: return 'O';
        default: break;
    }
    return ' ';
}

static inline char whois()
{
    pthread_t p = pthread_self();

    if (p == c_pthread) return 'C';
    if (p == f_pthread) return 'F';
    if (p == o_pthread) return 'O';
    return 'm';
}

#define HERE {printf("%c: %s %d\n", whois(),__FILE__,__LINE__);fflush(stdout);}
#define FUNC {printf("%c: %s()\n", whois(),__func__);fflush(stdout);};

static char ts_str[40];
static inline char *ts()
{
    static struct timespec last = {0};
    struct timespec current;
    struct timespec diff;

    if (!last.tv_sec) {
        clock_gettime(CLOCK_REALTIME, &last);
    }
    
    clock_gettime(CLOCK_REALTIME, &current);
    
    diff.tv_sec = current.tv_sec - last.tv_sec;
    diff.tv_nsec = current.tv_nsec - last.tv_nsec;
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1000000000; // nsec/sec
        diff.tv_sec--;
    }
    last = current;

    snprintf(ts_str,sizeof(ts_str),"[%05ld]", diff.tv_nsec/1000);
    
//    snprintf(ts_str,sizeof(ts_str),"[%02ld.%09ld]", diff.tv_sec, diff.tv_nsec);
    return ts_str;
}

#define HERE_TS             {printf("%c: %s %s:%d\n", whois(), ts(),__FILE__,__LINE__); fflush(stdout);}
#define DEBUG_TS(fmt,...)   {printf("%c: %s ", whois(), ts()); printf(fmt, ## __VA_ARGS__);fflush(stdout);}
#define DEBUG(fmt,...)      {printf("%c: ",whois()); printf(fmt, ## __VA_ARGS__);fflush(stdout);}
#define DEBUG_C(fmt,...)    {if (g->whoami == CAPTAIN) {printf("C: "); printf(fmt, ## __VA_ARGS__);fflush(stdout);}}


#endif // _DEBUG_H_

