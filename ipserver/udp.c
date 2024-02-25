// udp.c
#include "ipserver.h"
#include "udp.h"

static int port_verbosity = 0;

//#define alpfw(...)
#define alpfw(x,format, ...) if (port_verbosity) printf(format, ##__VA_ARGS__)

#include <net/if.h>
#include <sys/ioctl.h>

#define MULTICAST(x)    (((x) & htonl(0xf0000000)) == htonl(0xe0000000))

// -----------------------------------------------------------------------------
int udp_common_setup( int channel, udp_common_t *udpc)
{
    int             sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (port_verbosity >= 3)
        alpfw(-1," ch %d; calling getaddrinfo for %s:%s\n",
              channel,udpc->ip, udpc->portstr);

    n = getaddrinfo(udpc->ip, udpc->portstr,
                    &hints, &res);
    if (n != 0) {
        alpfw(-1,"ch %d: getaddrinfo error for %s:%s (%s)\n",
              channel, udpc->ip, udpc->portstr,gai_strerror(n));
        return -1;
    }

    if (port_verbosity >= 3)
        alpfw(-1," ch %d; getaddrinfo for %s:%s returns n=%d\n",
            channel,udpc->ip, udpc->portstr,n);

    ressave = res;

    do {
        if (port_verbosity >= 3)
            alpfw(-1,
                  " ch %d: trying to create socket for %s:%s with family %d, "
                  "socktype %d, proto %d\n",
                  channel, udpc->ip, udpc->portstr,
                  res->ai_family, res->ai_socktype, res->ai_protocol);

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        // may be able to be more selective here
        if (sockfd < 0) {
            alpfw(-1," ch %d: opening socket FAILED for %s:%s with "
                  "family %d,socktype %d, proto %d \n\t%m\n",
                  channel, udpc->ip, udpc->portstr,
                  res->ai_family, res->ai_socktype, res->ai_protocol);
            alpfw(-1," ch %d: will try to use next resource from family\n",
                  channel);
        }else {
            if (port_verbosity >= 1) {
                alpfw(-1," ch %d: opening socket succeeded for %s:%s with "
                      "family %d,socktype %d, proto %d\n",
                      channel, udpc->ip, udpc->portstr,
                      res->ai_family, res->ai_socktype, res->ai_protocol);
            }
                
            {
                int one = 1;
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
            }
            break;
        }
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL) {
        alpfw(-1,"udp_client error for %s:%s, socket \n",
              udpc->ip, udpc->portstr );
        freeaddrinfo(ressave);
        return -1;
    }

    // ok, we got a socket!
    if (udpc->sa) {
        free(udpc->sa);
        udpc->sa = NULL;
    }

    udpc->sa = (struct sockaddr *)malloc(res->ai_addrlen);
    memcpy(udpc->sa, res->ai_addr, res->ai_addrlen);
    udpc->salen = res->ai_addrlen;

    udpc->mcast = MULTICAST(((struct sockaddr_in *)udpc->sa)->sin_addr.s_addr);
    udpc->cast_type_known=1;

    if (port_verbosity >= 4)
        alpfw(-1," %s is a %s address\n",
              udpc->addr, udpc->mcast ? "multicast" : "unicast");

    if (port_verbosity >= 4)
        alpfw(-1," saving sa %p, length %d for %s:%s\n",
              udpc->sa, udpc->salen,udpc->ip, udpc->portstr);

    // don't need this anymore
    freeaddrinfo(ressave);

    udpc->fd = sockfd;
    return sockfd;
}
// -----------------------------------------------------------------------------
static int generate_interface_addr(int channel,udp_common_t *udpc,
                            const char *ifspec, struct in_addr *pifaddr)
{
    struct ifreq req;
    int          ip_number;

    if ( ifspec == NULL ) {
        alpfw(-1," ch %d: no interface specified for %s\n",
              channel,udpc->addr);
        alpfw(-1," ch %d: guess we will rely on routing tables\n",
              channel);
        return -1;
    }

    // it's really easy if the clue is an internet address already
    ip_number = inet_aton(ifspec, pifaddr);
    if (ip_number) {
        // success! pifaddr should be filled with the right stuff now
        if (port_verbosity >= 2)
            alpfw(-1," ch %d: interface clue %s is an interface address\n",
                  channel,inet_ntoa(*pifaddr));
        return 0;
    }

    // if here, they either gave us an interface name or junk

    if (port_verbosity >= 2) {
        alpfw(-1," ch %d: inet_aton() failed to convert '%s' to a number\n",
              channel,ifspec);
        alpfw(-1,
              " ch %d: will try to get an in_addr using %s as interface name\n",
              channel,ifspec);
    }

    // copy interface name (eg, eth0) into request
    strncpy(req.ifr_name,(char*)ifspec,sizeof(req.ifr_name));

    // ask for the IP address assigned to that interface
    alpfw(-1," using ioctl(SIOCGIFADDR) to interface address\n");

    if ( ioctl(udpc->fd,SIOCGIFADDR,&req) < 0 ) {
        alpfw(-1," couldn't get iface IP address from interface '%s'\n",
              ifspec );
        return -2;
    }

    struct sockaddr_in *ipaddr=(struct sockaddr_in *)&req.ifr_addr;
    memcpy(pifaddr, &ipaddr->sin_addr, sizeof(*pifaddr) );

    alpfw(-1," ok, that was sucessfull. we have ifaddr %s\n",
          inet_ntoa(*pifaddr));

    return 0;
}
// -----------------------------------------------------------------------------
int udp_multicast_join( int channel,udp_common_t *udpc)
{
    struct ip_mreq mreq;
    int            rc;

    // join the multicast group
    memcpy(&mreq.imr_multiaddr,
           &(((struct sockaddr_in *)udpc->sa)->sin_addr),
           sizeof(struct in_addr));

    const char *   ifspec = udpc->interface_clue;
    struct in_addr ifaddr;
    rc = generate_interface_addr(channel,udpc,ifspec, &ifaddr);
    if (rc == 0) {
        alpfw(-1," cd %d; using %s as interface for %s\n",
              channel,inet_ntoa(ifaddr), udpc->addr);

        mreq.imr_interface.s_addr = ifaddr.s_addr;
    }else {
        alpfw(-1," ch %d: using %s as interface for %s\n",
              channel,"INADDR_ANY", udpc->addr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }

    if (port_verbosity)
        alpfw(-1," ch %d: trying to be a memberof multicast address %s\n",
              channel,udpc->addr);
    rc = setsockopt(udpc->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                    &mreq, sizeof(mreq));
    if ( rc < 0 ) {
        alpfw(-1," ch %d: could not join multicast group %s:%s (%m)\n\n",
              channel, udpc->ip, udpc->portstr);
        return -1;
    }else {
        alpfw(-1,"ch %d: we are now a member of multicast group %s:%s (%m)\n\n",
              channel, udpc->ip, udpc->portstr);
        return 0;
    }
}
// -----------------------------------------------------------------------------
int udp_common_bind( int channel, udp_common_t *udpc)
{
    int rc,sockfd=udpc->fd;
    int accumulated_delay,  delay_in_secs=5,max_delay_in_secs=80;

    if (port_verbosity >= 2)
        alpfw(-1," ch %d: trying to bind to %s using socket %d\n",
              channel, udpc->addr, udpc->fd);

    rc = bind(sockfd, udpc->sa, udpc->salen);
    if (rc < 0) {
        alpfw(-1," ch %d: couldn't bind to %s: errno %d, %m\n",
              channel,udpc->addr,errno);
        alpfw(-1," ch %d: will retry every %d seconds up to %d seconds\n",
              channel,delay_in_secs,max_delay_in_secs);

        putchar('B');

        accumulated_delay = 0;
        do {
            sleep(delay_in_secs);
            accumulated_delay += delay_in_secs;

            rc = bind(sockfd, udpc->sa, udpc->salen);
            if (rc == 0)
                break;

            alpfw(-1," ch %d: still couldn't bind to %s: %m\n",
                  channel,udpc->addr);

            putchar('B');
        } while (accumulated_delay < max_delay_in_secs);

        if (rc < 0) {
            alpfw(-1," ch %d: giving up binding to %s: %m\n",
                  channel, udpc->addr);
            close(sockfd);
            return -1;
        }
    }

    if (port_verbosity)
        alpfw(-1," ch %d: we are bound to %s using socket %d! \n",
              channel, udpc->addr, sockfd);

    if (udpc->mcast) 
        udp_multicast_join( channel, udpc);

    return(sockfd);
}
// -----------------------------------------------------------------------------
int udp_mcast_setup( int channel, udp_common_t *udpc)
{
    // don't loop back the multicast packets sent over this socket

    const char *  ifspec = udpc->interface_clue;
    unsigned char loop   = udpc->loopback;
    int           ret;

    setsockopt(udpc->fd, IPPROTO_IP, IP_MULTICAST_LOOP,
               (const char *)&loop, sizeof(loop));

    // note: purposely ignoring error code for IP_MULTICAST_LOOP --
    // some flavors of UNIX (e.g., Solaris complain about this setting)

    // if specified, select the output interface to send multicast packets

    // it can be specified as either an interface name or the address
    // associated with an interface

    if ( ifspec == NULL ) {
        if (port_verbosity >= 3) {
            alpfw(-1," ch %d: no interface specified for %s\n",
                  channel, udpc->addr);
            alpfw(-1," ch %d: guess we will rely on routing tables\n",
                  channel);
        }
        return 0;
    }

    struct in_addr ifaddr;

    if (port_verbosity >= 1)
        alpfw(-1," ch %d; we have interface clue %s for %s\n",
              channel, ifspec,udpc->addr);

    int rc;
    rc = generate_interface_addr(channel, udpc,ifspec, &ifaddr);
    if (rc < 0)
        return rc;

    // if here, we should have a valid interface address. use it

    if (port_verbosity >= 1)
        alpfw(-1,
              " ch %d: calling setsockopt to use interface %s to output %s\n",
              channel, inet_ntoa(ifaddr), udpc->addr);

    ret = setsockopt(udpc->fd, IPPROTO_IP, IP_MULTICAST_IF,
                     &ifaddr, sizeof(ifaddr));
    if ( ret ) {
        alpfw(-1, " ch %d: could not use interface address %s as mcast sender"
              " for %s (%m)\n", channel, inet_ntoa(ifaddr), udpc->addr);
        return -2;
    }else {
        if (port_verbosity >= 1)
            alpfw(-1, " ch %d: using interface address %s to mcast to %s\n",
                  channel, inet_ntoa(ifaddr),udpc->addr);
    }
    return 0;
}
// -----------------------------------------------------------------------------
