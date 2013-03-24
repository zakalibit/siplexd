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
#include <limits.h>
#include <time.h>
#include <osip2/osip.h>
#include "logger.h"
#include "sock.h"

typedef struct
{
   time_t      expires;
   osip_uri_t *to_uri;
   osip_uri_t *contact_uri;
}register_uri_t;

#define REGISTER_URI_ARRAY_SIZE 128
register_uri_t register_map[REGISTER_URI_ARRAY_SIZE];

#define _free_register_uri(u)\
{osip_uri_free ((u).to_uri);\
osip_uri_free ((u).contact_uri);\
(u).expires = 0;}0

static int max_used_index_seat = 0;
/**
 * @brief Compares the masqueraded uri to the registered uri, it wont compare the hostnames
 *        as the incoming message will have the masqueraded hostname
 * @param m - masqueraded uri
 * @param r - registered uri
 * @return true if u1 matches u2, false otherwise
 */
static unsigned int sip_masqueraded_uri_match
(
    const osip_uri_t* const m,
    const osip_uri_t* const r
)
{
    return (m && r &&
            m->scheme && r->scheme &&
            m->username && r->username &&
            !osip_strcasecmp(m->scheme, r->scheme) &&
            !osip_strcasecmp(m->username, r->username));
}

void sip_register_map_init(void)
{
    max_used_index_seat = 0;
    memset (register_map, 0, sizeof(register_map));
}

const osip_uri_t* sip_register_map_find_contact(const osip_message_t* const osip_msg)
{
    const osip_uri_t* contact_uri = NULL;
    const osip_to_t* to = osip_message_get_to(osip_msg);

    if (NULL != to && NULL != to->url)
    {
        for(int i = 0; i < max_used_index_seat; i++)
        {
            if (register_map[i].expires > 0 &&
                sip_masqueraded_uri_match(register_map[i].to_uri, to->url))
            {
                contact_uri = register_map[i].contact_uri;
            }
        }
    }

    return contact_uri;
}

static int find_empty_slot(const osip_uri_t* uri, const time_t time)
{
    int i = 0;

    // do aging and check if we have the uac registered already
    for(; i < max_used_index_seat; i++)
    {
        if (register_map[i].expires > 0)
        {
            if ((register_map[i].expires - time) <= 0)
            {
                //registration has expired
                _free_register_uri(register_map[i]);

                //update the max index of a used seat
                if (i == (max_used_index_seat -1)) max_used_index_seat--;
            }
            // else if the uac has a non expired registration
            else if (sip_masqueraded_uri_match(register_map[i].to_uri, uri)) break;
        }
    }

    if (i < max_used_index_seat)
    {  // uac already registered, remove the old entry
       // so it can be added again with the new expires
        _free_register_uri(register_map[i]);
    }
    else
    {   // uac not yet registered, find first empty entry
        for(i=0; i < REGISTER_URI_ARRAY_SIZE && register_map[i].expires > 0; i++);

        if (i >= max_used_index_seat)
        {
            max_used_index_seat = (i < REGISTER_URI_ARRAY_SIZE)? i+1: REGISTER_URI_ARRAY_SIZE;
        }
    }

    return i;
}

int sip_register_map_update(const osip_message_t* osip_msg)
{
    osip_to_t* to = osip_message_get_to(osip_msg);

    if (NULL == to || NULL == to->url || NULL == to->url->host) return RC_ERR;
    osip_uri_t* to_uri = to->url;

    osip_contact_t* contact = NULL;
    osip_message_get_contact(osip_msg, 0, &contact);

    time_t t_now;
    time (&t_now);

    // in case of unREGISTER it will only remove the uri_map record
    const int i = find_empty_slot(to_uri, t_now);

    // if it looks like unREGISTER request just return
    if (NULL == contact ||
        NULL == contact->url ||
        NULL == contact->url->host) return RC_OK;

    int expires = 180;
    osip_uri_param_t* expires_p = NULL;
    if (osip_contact_param_get_byname(contact, EXPIRES, &expires_p) ||
            NULL == expires_p ||
            NULL == expires_p->gvalue)
    {
        osip_message_get_expires(osip_msg, 0, &expires_p);
    }

    if (expires_p && expires_p->gvalue)
    {
        expires = atoi(expires_p->gvalue);
        
        if (expires < 0 || expires > UINT_MAX)
            expires = 180;
    }
    
    // check if is unREGISTER
    if (expires == 0) return RC_OK;


    // add or update an UAC registration
    if (i < REGISTER_URI_ARRAY_SIZE)
    {        
        osip_uri_t* contact_uri = contact->url;
        if (osip_uri_clone(to_uri, &register_map[i].to_uri)) return RC_ERR;
        if (osip_uri_clone(contact_uri, &register_map[i].contact_uri))
        {
            osip_free(register_map[i].to_uri);
            register_map->expires = 0;
            return RC_ERR;
        }

        register_map->expires = t_now+expires;
        return RC_OK;
    }

    return RC_ERR;
}

void sip_register_map_free(void)
{
    for(int i = 0; i < max_used_index_seat; i++)
        if (register_map[i].expires > 0)
        {
            _free_register_uri(register_map[i]);
        }
}
