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
#include "sock.h"
#include "logger.h"
#include "sipregister.h"
#include "siputils.h"
#include "sipvalidate.h"

/*
 * Unsmaquerades SIP request that came to the oubound interface with a correct internal
 * uri, so we can correctly calculate next hop. It does use a lookup table that is
 * dynamically built using information from the REGISTER requests.
 * */
static int proxy_unmasquerade_request_uri(osip_message_t* osip_msg)
{
    __tri(proxy_unmasquerade_request_uri);

    int rc = RC_ERR;

    const osip_uri_t* contact = sip_register_map_find_contact(osip_msg);

    if (NULL != contact)
    {
        osip_uri_t* cloneUri = NULL;
        
        if (0 == osip_uri_clone(contact, &cloneUri))
        {
            osip_uri_t* uri = osip_message_get_uri(osip_msg);
        
            if (NULL != uri) osip_uri_free(uri);

            osip_message_set_uri(osip_msg, cloneUri);
        
            rc = RC_OK;
        }
        else logDEBUG("osip_uri_clone() failed...");
    }
    else logDEBUG("Contact not found by findSipUriMapContact()");

    gen_and_send_sip_response(osip_msg, SIP_NOT_FOUND);

    __tre(return) rc;
}

/** 16.6 Request Forwarding
    Stateless proxy behavior

      2.  Update the Request-URI
      3.  Update the Max-Forwards header field
      6.  Postprocess routing information
      7.  Determine the next-hop address, port, and transport
      8.  Add a Via header field value
      9.  Add a Content-Length header field if necessary
      10. Forward the new request
 */
static int proxy_sip_request(sip_msg_str_t* sms)
{
    __tri(proxy_sip_request);

    char* buffer;
    int   len;
    int ret = RC_ERR;

    if(MSG_IS_INCOMING(sms->direction))
    {
        if (proxy_unmasquerade_request_uri(sms->osip_msg)) __tre(return) RC_ERR;
    }
    else  /* outgoing requests */
    {
        if (MSG_IS_INVITE(sms->osip_msg) || MSG_IS_BYE(sms->osip_msg))
        {
           // TODO: rewrite the gateway for the outgoing call
        }
    }
    
    /*3.  Update the Max-Forwards header field*/
    update_max_fowards (sms->osip_msg);

    /*6.  Postprocess routing information*/
    //NOT IMPLIMENTED

    /*7.  Determine the next-hop address, port, and transport
     *8.  Add a Via header field value
     */
    if (!calculate_next_hop (sms) &&
        !add_proxy_via (sms) &&
        // osip needs osip_message_to_str() that works with the preallocated
        // buffer, so it wont spend time allocating memory each time
        !osip_message_to_str (sms->osip_msg, &buffer, &len))
    {
        // point to the new allocated buffer
        sms->buffer    = buffer;
        sms->msgsize   = len;

        /*10. Forward the new request*/
        ret = sip_send (sms);
        
        osip_free(buffer);
    }
    else logERROR("Failed to proxy request...");
    
    __tre(return) ret;
}

static int proxy_sip_response(sip_msg_str_t* sms)
{
    __tri(proxy_sip_response);
    
    char* buffer;
    int   len;
    int   ret = RC_ERR;

    if (!remove_proxy_via(sms->osip_msg) &&
        !calculate_next_hop (sms) &&
        !osip_message_to_str (sms->osip_msg, &buffer, &len))
    {
        // point to the new allocated buffer
        sms->buffer    = buffer;
        sms->msgsize   = len;

        /*10. Forward the new request*/
        ret = sip_send (sms);

        osip_free(buffer);
    }
    else logERROR("Failed to proxy response...");
    
    __tre(return) ret;
}

static int proxy_sip_msg(sip_msg_str_t* sms)
{
    __tri(proxy_sip_msg);

    if (MSG_IS_REGISTER(sms->osip_msg))
    {
        if (DIRECTION_IN == sms->direction)
        {
            logWARNING("REGISTER requests on the outbound interface are not accepted!");
            gen_and_send_sip_response(sms->osip_msg, SIP_FORBIDDEN);
            __tre(return) RC_OK;
        }
        else if (RC_OK == sip_register_map_update(sms->osip_msg))
        {            
            __tre(return) proxy_sip_request(sms);
        }
        else logERROR("proxySipMessage() - failed to update SIP uri map.");
    }
    else if (MSG_IS_REQUEST(sms->osip_msg)){ __tre(return) proxy_sip_request(sms);}
    else if (MSG_IS_RESPONSE(sms->osip_msg)){ __tre(return) proxy_sip_response(sms);}

    logDEBUG("proxySipMessage() - can not handle request type %s", sms->osip_msg->sip_method);
    
    __tre(return) RC_ERR;
}

void sip_process_msg(sip_msg_str_t* sms)
{
   int res = osip_message_init (&sms->osip_msg);

   if (res)
   {
      logWARNING("osip_message_init() failed rtn[%i]", res);
      return;
   }

   res = osip_message_parse (sms->osip_msg, sms->buffer, sms->msgsize);

   if (RC_OK == res)
   {
      /*
       * RFC 3261, Section 16.3 step 3
       * Max-Forwards check, refuse with 483 if too many hops
       */
      if (RC_OK == sip_check_max_forwards (sms->osip_msg))
      {
         /*
          * RFC 3261, Section 16.3 step 4
          * Loop Detection check, return 482 if a loop is detected
          */
         if (RC_ERR == sip_via_loop_not_detected(sms->osip_msg) ||
             RC_ERR == proxy_sip_msg(sms))
         {
            logDEBUG("Failed to proxy SIP message...");
         }
      }
      else
      {
         gen_and_send_sip_response(sms->osip_msg, SIP_TOO_MANY_HOPS);
         logERROR("Max-Forwads check error, too many hops...");
      }
   }
   else
   {
      logERROR("osip_message_parse() failed rtn[%i]", res);
   }

   osip_message_free (sms->osip_msg);
   log_flush();
}
