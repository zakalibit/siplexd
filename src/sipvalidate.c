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
#include <osip2/osip.h>
#include "logger.h"
#include "sock.h"
#include "siputils.h"
/*
 * RFC 3261, Section 16.3 step 3
 * Max-Forwards check, refuse with 483 if too many hops
 */
int sip_check_max_forwards(const osip_message_t *sipmsg)
{
    osip_header_t* osip_fwd = NULL;
         
    int ret = osip_message_get_max_forwards(sipmsg, 0, &osip_fwd);
         
    if (!ret && osip_fwd && osip_fwd->hvalue)
    {
        const int fwd = atoi (osip_fwd->hvalue);

        if (0 == fwd) return RC_ERR;
    }

    return RC_OK;
}

int sip_check_via_loop (const osip_message_t* sipmsg)
{
   int own_via_count=0;
   /* do not check my own via */
   int pos = 1;
   while (!osip_list_eol (&sipmsg->vias, pos))
   {
      osip_via_t *via = (osip_via_t*)osip_list_get (&sipmsg->vias, pos);
      if (is_proxy_address (via->host, via->port)) own_via_count++;
      pos++;
   }

   return (own_via_count > 1)? RC_ERR: RC_OK;
}

int sip_via_loop_not_detected(const osip_message_t* osip_msg)
{
    if (!(MSG_IS_RESPONSE(osip_msg) &&
          MSG_TEST_CODE(osip_msg, SIP_LOOP_DETECTED)) &&
          sip_check_via_loop(osip_msg))
    {
        logWARNING("Via-loop detected...");
        gen_and_send_sip_response(osip_msg, SIP_LOOP_DETECTED);
        return RC_ERR;
    }

    return RC_OK;
}
