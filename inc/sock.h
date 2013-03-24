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

#ifndef _SOCK_H
#define	_SOCK_H

#include <netinet/in.h>
#include <osip2/osip.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#define TCP_INVALID_PORT(p) ((p) < 0 || (p) > INT_MAX)
#define TCP_INVALID_SOCKET (-1)

typedef struct
{
   int sock;
   int port;
   const char* if_name;
   struct in_addr addr;
} sip_leg_t;

enum SIP_MSG_DIRECTION
{
    DIRECTION_UNSET,
    DIRECTION_OUT,
    DIRECTION_IN
};

#define MSG_IS_INCOMING(__msg_direction) (DIRECTION_IN == (__msg_direction))

typedef struct
{
    char*  buffer;
    size_t bufferlen;
    size_t msgsize;
    osip_message_t* osip_msg;
    enum SIP_MSG_DIRECTION direction;
    struct in_addr rcvr_addr;
    int            rcvr_port;
} sip_msg_str_t;

#define SIP_MSG_STR_CPY(__dest, __src)\
    memcpy((__dest), (__src), sizeof(sip_msg_str_t));
    
int initialize_transport(void);
void shutdown_transport(void);

int get_in_port(void);
int get_out_port(void);

struct in_addr* get_in_addr(void);
struct in_addr* get_out_addr(void);

void sip_events_process(void);

int sip_send
(
    sip_msg_str_t* sip_msg
);

int get_addr_by_name(const char* const ifname, struct in_addr* saddr);
int get_host_addr(const char* const host, struct in_addr* addr);

#ifdef	__cplusplus
}
#endif

#endif	/* _SOCK_H */

