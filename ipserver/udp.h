// udp.h

#ifndef _UDP_HDR_
#define _UDP_HDR_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/igmp.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct {
    long long          byte_count;
    int                interdot_count;
    int                cast_type_known;
    int                mcast;
    struct sockaddr *  sa;
    socklen_t          salen;
    char *             addr;
    char *             ip;
    uint16_t           port;
    char *             portstr;
    // this is just a piece of 'addr' so don't free it!

    // mostly associated with multi-cast streaming
    char *             interface_clue;
    int                loopback;

    int                fd;
} udp_common_t;

extern int udp_common_setup( int channel, udp_common_t *udpc);
extern int udp_common_bind( int channel, udp_common_t *udpc);
extern int udp_multicast_join( int channel, udp_common_t *udpc);
extern int udp_mcast_setup( int channel, udp_common_t *udpc);

#endif // _UDP_HDR_
