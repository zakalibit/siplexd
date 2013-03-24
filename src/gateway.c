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

#define GATEWAY_MAP_SIZE (32)
#define INVITE_URI_MAP_SIZE (128)

typedef struct sip_gateway_t
{
    int   port;
    char* host;
    char* username;
    char* password;
    char* dp;
} sip_gateway_t;

#define free_gateway_entry(gw)\
{\
if(gw.dp) osip_free(gw.dp);\
gw.dp=NULL;\
if(gw.host) osip_free(gw.host);\
gw.host=NULL;\
if(gw.username) osip_free(gw.username);\
gw.username=NULL;\
gw.port=0;\
}0

sip_gateway_t gateway_map[GATEWAY_MAP_SIZE];

int gw_count = 0;

typedef struct invite_uri_map_t
{
    int         gateway_id;
    // call id used as index
    char*       call_id;
    osip_uri_t* org_url;
} invite_uri_map_t;

invite_uri_map_t invite_uri_map[INVITE_URI_MAP_SIZE];

void init_gateway_map(void)
{
    gw_count = 0;

    gateway_map[gw_count].dp = NULL;
    gateway_map[gw_count].host = osip_strdup("sip.sipgate.com");
    gateway_map[gw_count].username = osip_strdup("user");
    gateway_map[gw_count].password = osip_strdup("password");
    gateway_map[gw_count].port = 5060;

    gw_count++;
}

void free_gateway_map(void)
{
    while(gw_count--)
    {
        free_gateway_entry(gateway_map[gw_count]);
    }
}

static void sip_rewrite_osip_uri(osip_uri_t* uri, char* host, char* username)
{
    if (NULL != uri->host)     osip_free(uri->host);
    if (NULL != uri->username) osip_free(uri->username);

    uri->host     = osip_strdup(host);
    uri->username = osip_strdup(username);
}

int sip_rewrite_first_contact(osip_message_t* osip_msg, char* username, char* host)
{
    osip_contact_t* contact = NULL;
    osip_message_get_contact(osip_msg, 0 , &contact);

    if (NULL != contact &&
        NULL != contact->url)
    {
        sip_rewrite_osip_uri (contact->url, username, host);
        return RC_OK;
    }

    return RC_ERR;
}

/**
 * @brief Rewrites the host in the uri of a sip message
 * @param sip_msg - the target sip message
 * @param host - the new host value
 * @return - RC_OK if uri host rewritten successfully, RC_ERR otherwise
 */
int sip_rewrite_uri(osip_message_t* osip_msg, char* host, char* username)
{
    osip_uri_t* uri = osip_message_get_uri(osip_msg);

    if (NULL != uri)
    {
        sip_rewrite_osip_uri(uri, host, username);
        return RC_OK;
    }

    return RC_ERR;
}

int sip_rewrite_from(osip_message_t* osip_msg, char* host, char* username)
{
    osip_from_t* from = osip_message_get_from(osip_msg);

    if (NULL != from)
    {
        osip_uri_t*  uri = osip_from_get_url(from);
        if (NULL != uri)
        {
            sip_rewrite_osip_uri(uri, host, username);
            return RC_OK;
        }
    }

    return RC_ERR;
}


int sip_rewrite_to(osip_message_t* osip_msg, char* host, char* username)
{
    osip_to_t* from = osip_message_get_to(osip_msg);

    if (NULL != from)
    {
        osip_uri_t*  uri = osip_to_get_url(from);
        if (NULL != uri)
        {
            sip_rewrite_osip_uri(uri, host, username);
            return RC_OK;
        }
    }

    return RC_ERR;
}

//should rewrite the contact header hostname/IP address with external one
//leave the username intact, lets see what will happen
int sip_gateway_rewrite_message(osip_message_t* osip_msg, char* host, char* username)
{
    char* gw_username = "alex.revetchi";
    char* gw_host     = "sip.voipcheap.com";

    int rc = sip_rewrite_uri(osip_msg, host, username);

    if (RC_OK == rc) rc = sip_rewrite_from(osip_msg, host, username);
    if (RC_OK == rc) rc = sip_rewrite_to(osip_msg, host, username);

    return rc;
}

int sip_gateway_find_invite_transaction(osip_message_t* osip_msg)
{
    osip_via_t* via = osip_list_get (&osip_msg->vias, 0);

    if (NULL != via)
    {
        osip_uri_param_t* branch = NULL;
        osip_via_param_get_byname(via, "branch", &branch);

        if (NULL != branch)
        {
            osip_uri_header_free(branch);
        }
    }

    return RC_ERR;
}

int sip_find_gateway(osip_to_t* to, sip_gateway_t *gw)
{
    if (to->url != NULL
        && to->url->username != NULL
        && to->url->username[0] != '\0')
    {
        // check the gateway map for a matching dial plan
        for (int i = 0; i < GATEWAY_MAP_SIZE; i++)
        {
            sip_gateway_t *g = gateway_map + i;

            if (g->host != NULL &&
                0 == strncmp(to->url->username, g->dp, 6) &&
                0 != strcasecmp(to->url->host, g->host))
            {
                gw = g;
                return RC_OK;
            }
        }
    }

    return RC_ERR;
}


int sip_gateway_rewrite(osip_message_t* osip_msg)
{
    osip_to_t* to = osip_message_get_to(osip_msg);

    if (NULL != to)
    {
        sip_gateway_t *gw = NULL;
        int err = sip_find_gateway(to, gw);

        if (RC_OK == err && NULL != gw)
        {

        }
    }
}
