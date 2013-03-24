/**
 * Copyright (C) 2009  Alex Revetchi
 *
 * Siplexd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <limits.h>
#include "logger.h"
#include "config.h"
#include "sock.h"
#include "siplocaldns.h"
#include "sipproxy.h"
   

sip_leg_t in_leg;
sip_leg_t out_leg;

extern int siplexd_execute;
extern int dump_dmalloc;

static int transport_get_cfg_ports(void)
{
    static const char* PORT_READ_ERR="Could not read configuration %s port number, or not configured";
    static const char* PORT_RANGE_INVALID="%s port value[%i] is not in a valid range[0 - %i]";
    
    if (RC_ERR == cfg_get_int ("siplexd.sip.interface.in.port", &in_leg.port))
    {
        logFATAL(PORT_READ_ERR, "inbound");
        return RC_ERR;
    }
    else if (TCP_INVALID_PORT(in_leg.port))
    {
        logFATAL(PORT_RANGE_INVALID, "Inbound", in_leg.port, UINT_MAX);
        return RC_ERR;
    }

    if (RC_ERR == cfg_get_int ("siplexd.sip.interface.out.port", &out_leg.port))
    {
        logFATAL(PORT_READ_ERR, "outbound");
        return RC_ERR;
    }
    else if (TCP_INVALID_PORT(out_leg.port))
    {
        logFATAL(PORT_RANGE_INVALID, "Outbound", out_leg.port, UINT_MAX);
        return RC_ERR;
    }

    return RC_OK;
}

static int transport_get_cfg_interface_names(void)
{
    static const char* IF_READ_ERR="Could not read configuration %s interface name, or not configured";
    if (RC_ERR == cfg_get_string ("siplexd.sip.interface.in.ifname" , &in_leg.if_name))
    {
        logFATAL(IF_READ_ERR, "inbound");
        return RC_ERR;
    }

    if (RC_ERR == cfg_get_string ("siplexd.sip.interface.out.ifname", &out_leg.if_name))
    {
        logFATAL(IF_READ_ERR, "outbound");
        return RC_ERR;
    }

    return RC_OK;
}

static int get_addrs_from_names(void)
{
    if (RC_OK == get_addr_by_name (in_leg.if_name, &in_leg.addr) &&
        RC_OK == get_addr_by_name (out_leg.if_name, &out_leg.addr))
    {
        return RC_OK;
    }

    return RC_ERR;
}

int get_in_port(void)
{
    return in_leg.port;
}

int get_out_port(void)
{
    return out_leg.port;
}

struct in_addr* get_in_addr(void)
{
    return &in_leg.addr;
}

struct in_addr* get_out_addr(void)
{
    return &out_leg.addr;
}

void shutdown_transport(void)
{
    if (TCP_INVALID_SOCKET != in_leg.sock)  close (in_leg.sock);
    if (TCP_INVALID_SOCKET != out_leg.sock) close (out_leg.sock);

    in_leg.sock  = TCP_INVALID_SOCKET;
    out_leg.sock = TCP_INVALID_SOCKET;

    local_dns_close();
}

static int init_leg(sip_leg_t* leg)
{        
    leg->sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (leg->sock >= 0)
    {
        // 10 miliseconds
        struct timeval tv = {0, 10000};
        int reuse = 1;

        if (setsockopt (leg->sock, SOL_SOCKET, SO_REUSEADDR, &reuse , sizeof(reuse)) ||
            setsockopt (leg->sock, SOL_SOCKET, SO_RCVTIMEO, &tv , sizeof(tv)) ||
            setsockopt (leg->sock, SOL_SOCKET, SO_SNDTIMEO, &tv , sizeof(tv)))
        {
            logERROR("Setting sockefreeifaddrs(ifaddr);t options failed errno[%i]", errno);
        }

        struct sockaddr_in my_addr;
        memset(&my_addr, 0, sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(leg->port);
        memcpy (&my_addr.sin_addr, &leg->addr, sizeof (struct in_addr));

        if (bind (leg->sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == 0)
        {
            return RC_OK;
        }

        logFATAL("binding the socket failed on port[%i], errno[%i]", leg->port, errno);
        close (leg->sock);
    }
    else logFATAL("Could not open datagram socket errno[%i]", errno);

    return RC_ERR;
}

static int sip_init_legs(void)
{
    if (in_leg.port != out_leg.port ||
        memcmp(&in_leg.addr, &out_leg.addr, sizeof(struct in_addr)))
    {
        if (RC_OK == init_leg (&in_leg) &&
            RC_OK == init_leg (&out_leg))
        {
            return RC_OK;
        }
    }
    else logFATAL("Inbound and outbound endpoints are the same, can not function!");

    return RC_ERR;
}

int initialize_transport(void)
{
    int rc = RC_ERR;

    if (RC_OK == transport_get_cfg_ports() &&
        RC_OK == transport_get_cfg_interface_names() &&
        RC_OK == get_addrs_from_names())
    {
        local_dns_init();

        rc = sip_init_legs();

        if (RC_ERR == rc)
        {
            logFATAL("Could not initialize proxy endpoints.");
        }
    }

    return rc;
}

static int sip_read_msg(const int fd, const int direction, sip_msg_str_t* sms)
{
    struct sockaddr_in src_addr;
    socklen_t src_size = sizeof (src_addr);

    int rcv = recvfrom (fd,
                        sms->buffer,
                        sms->bufferlen -1,
                        0,
                        (struct sockaddr*)&src_addr,
                        &src_size);

    if (rcv > 0)
    {
        char ip_addr[INET_ADDRSTRLEN];
        sms->direction = direction;
        sms->msgsize = rcv;
        sms->buffer[rcv] = '\0';

        if (inet_ntop(AF_INET, &src_addr.sin_addr.s_addr, ip_addr, sizeof(ip_addr)))
        {
            logINFO("Received from udp[%s:%i] %i bytes\n\n%s",
                  ip_addr, ntohs(src_addr.sin_port), rcv, sms->buffer);
        }
        else
        {
            logWARNING("inet_ntop() failed errno[%i]", errno);
            logINFO("Received from udp[could not resolve] %i bytes\n\n%s",
                  rcv, sms->buffer);
        }
    }
    else  logERROR("recvfrom() failed fd[%i] errno[%i]", fd, errno);

    return rcv;
}

void sip_events_process(void)
{    
    fd_set fdset;
    const int max_fd = MAX(out_leg.sock,in_leg.sock) + 1;

    char buffer [SIP_MESSAGE_MAX_LENGTH];
    sip_msg_str_t sms;

    while (siplexd_execute)
    {
       FD_ZERO(&fdset);
       FD_SET (in_leg.sock, &fdset);
       FD_SET (out_leg.sock, &fdset);

       int rtn = select (max_fd, &fdset, NULL, NULL, NULL);

       if (rtn < 1)
       {
           if (EINTR != errno)
           {
               logERROR("select() failed errno[%i]", errno);
               return;
           }

          continue;
       }

       sms.buffer    = buffer;
       sms.bufferlen = sizeof (buffer);

       if (FD_ISSET(in_leg.sock, &fdset) &&
           sip_read_msg(in_leg.sock, DIRECTION_OUT, &sms) > 0)
       {
           sip_process_msg(&sms);
       }

       sms.buffer    = buffer;
       sms.bufferlen = sizeof (buffer);

       if (FD_ISSET(out_leg.sock, &fdset) &&
           sip_read_msg(out_leg.sock, DIRECTION_IN, &sms) > 0)
       {
           sip_process_msg(&sms);
       }
    }
}

int sip_send(sip_msg_str_t* sms)
{
    struct sockaddr_in dest_addr;
    int fd = (DIRECTION_OUT == sms->direction) ? out_leg.sock : in_leg.sock;


    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons (sms->rcvr_port);
    memcpy (&dest_addr.sin_addr, &sms->rcvr_addr, sizeof(struct in_addr));

    int sent = sendto (fd,
                       sms->buffer,
                       sms->msgsize,
                       0,
                       (const struct sockaddr*)&dest_addr,
                       sizeof (dest_addr));

    char ip_addr[INET_ADDRSTRLEN];
    const char* pIP = (char*)inet_ntop(AF_INET, &sms->rcvr_addr, ip_addr, sizeof(ip_addr));

    if (!pIP)
    {
        logWARNING("inet_ntop() failed errno[%i]", errno);
        pIP = "unknown";
    }
    
    if (sent == sms->msgsize)
    {        
        logINFO("Sent to udp[%s:%i] %i bytes\n\n%s",
              pIP, sms->rcvr_port, sent, sms->buffer);
    }
    else
    {
        logERROR("Failed to send %i bytes to[%s:%i] errno[%i]\n\n%s",
               sms->msgsize, pIP, sms->rcvr_port, errno, sms->buffer);
    }

    return sent;
}


int get_addr_by_name(const char* const ifname, struct in_addr* saddr)
{
    struct ifaddrs *ifaddr, *ifa;

    if (!getifaddrs(&ifaddr))
    {
        /* Walk through linked list, maintaining head pointer so we
           can free list later */
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr &&
                AF_INET == ifa->ifa_addr->sa_family &&
                (IFF_UP  & ifa->ifa_flags) &&
                !osip_strcasecmp(ifname, ifa->ifa_name))
            {
                struct sockaddr_in* ain = (struct sockaddr_in*)ifa->ifa_addr;
                memcpy (saddr, &ain->sin_addr, sizeof (struct in_addr));
                freeifaddrs(ifaddr);
                return RC_OK;
            }
        }
        freeifaddrs(ifaddr);
    }
    else logERROR("getifaddrs() failed errno[%i]", errno);

    logERROR("Failed to find interface [%s] address by name!", ifname);
    
    return RC_ERR;
}

int get_host_addr(const char* const host, struct in_addr* addr)
{
    const struct in_addr* laddr;

    if (NULL != addr)
    {
        if (RC_OK == local_dns_lookup(host, &laddr))
        {
            memcpy(addr, laddr, sizeof(struct in_addr));
            return RC_OK;
        }

        // host not found in local DNS cache
        // try to resolve it and add
        local_dns_add(host);

        //
        if (RC_OK == local_dns_lookup(host, &laddr))
        {
            memcpy(addr, laddr, sizeof(struct in_addr));
            return RC_OK;
        }
    }
    
    return RC_ERR;
}
