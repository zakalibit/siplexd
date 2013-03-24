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
#include <osipparser2/osip_md5.h>
#include <osipparser2/osip_port.h>
#include "logger.h"
#include "sipdigest.h"

char* str_enquote (const char* const str)
{
    const size_t len = strlen(str);
    char* qstr = osip_malloc (len+3);

    if (qstr)
    {
        qstr[0] = '\"';
        memcpy (qstr+1, str, len);
        qstr[len+1] = '\"';
        qstr[len+2] = '\0';
    }

    return qstr;
}

int wwwauth_sanity(const osip_proxy_authenticate_t* proxy_auth)
{
    if (!proxy_auth ||
        !proxy_auth->auth_type ||
        !proxy_auth->nonce ||
        !proxy_auth->realm)
    {
        logERROR("www_authenticate header is not valid.");
        return RC_ERR;
    }

    if (osip_strcasecmp ("Digest", proxy_auth->auth_type))
    {
        logERROR("Authentication method [%s] not supported, [Digest] only.",
                 proxy_auth->auth_type);
        return RC_ERR;
    }

    if (proxy_auth->algorithm &&
        osip_strcasecmp ("MD5", proxy_auth->algorithm))
    {
        logERROR("Authentication method [%s] not supported, [MD5] only.",
                 proxy_auth->algorithm);
        return RC_ERR;
    }

    if (!proxy_auth->nonce)
    {
        logERROR("Proxy nonce not found.");
        return RC_ERR;
    }

    return RC_OK;
}

int add_proxy_auth_header(osip_message_t* osip_msg,
                          osip_proxy_authenticate_t * proxy_auth,
                          char* username,
                          char* password)
{
    osip_authorization_t *aut = NULL;


    if (RC_ERR == wwwauth_sanity(proxy_auth)) return RC_ERR;

    int rc = osip_authorization_init (&aut);

    if (rc)
    {
        logERROR("osip_authorization_init() failed.");
        return RC_ERR;
    }

    osip_proxy_authorization_set_auth_type (aut, osip_strdup ("Digest"));
    osip_proxy_authorization_set_algorithm (aut, osip_strdup ("md5"));
    osip_proxy_authorization_set_realm (aut, osip_strdup (proxy_auth->realm));

    if (proxy_auth->opaque)
        osip_proxy_authorization_set_opaque (aut, osip_strdup (proxy_auth->opaque));

    aut->username = str_enquote (username);
    if (!aut->username) goto ERR_EXIT;
    
    char* qop = osip_www_authenticate_get_qop_options (proxy_auth);
    //accept only auth
    if (qop != NULL && !strstr (qop, "auth")) goto ERR_EXIT;


    char* cleanRealm;
    if (osip_proxy_authorization_get_realm (aut) == NULL)
    {
        cleanRealm = osip_strdup ("");
    }
    else
    {
        cleanRealm = osip_strdup_without_quote (osip_proxy_authorization_get_realm (aut));
    }
    
    {
        HASHHEX HA1;
        HASHHEX HA2 = "";
        HASHHEX Response;
        char* Qop = NULL;
        char CNonce[9];
        char* lpCNonce = NULL;
        char* lpClientNonce = NULL;
        
        if (qop)
        {
            //hardcoded for the moment
            static int nonceCount = 1;

            static char* clientNonce = "0a4f113b";
            lpClientNonce = clientNonce;
            Qop = "auth";
            osip_proxy_authorization_set_message_qop (aut, osip_strdup (Qop));
                 
            snprintf (CNonce, sizeof(CNonce), "%.8i", nonceCount);
            lpCNonce = CNonce;

            osip_proxy_authorization_set_nonce_count (aut, osip_strdup (CNonce));
            osip_proxy_authorization_set_cnonce (aut, str_enquote(clientNonce));
        }
        
        osip_proxy_authorization_set_nonce (aut, osip_strdup (proxy_auth->nonce));

        char* reqUriStr = NULL;
        rc = osip_uri_to_str (osip_msg->req_uri, &reqUriStr);

        if (rc)
        {
            osip_free(cleanRealm);
            goto ERR_EXIT;
        }

        char* reqQUri = str_enquote (reqUriStr);

        if (!reqQUri)
        {
            osip_free(cleanRealm);
            osip_free(reqUriStr);
            goto ERR_EXIT;
        }

        osip_proxy_authorization_set_uri (aut, reqQUri);

        char* lpProxyNonce = osip_strdup_without_quote (proxy_auth->nonce);

        DigestCalcHA1 ("md5", username, cleanRealm, password, lpProxyNonce, lpClientNonce, HA1);
        DigestCalcResponse (HA1, lpProxyNonce, lpCNonce, lpClientNonce,
                            Qop, osip_msg->sip_method, reqUriStr, HA2, Response);
        
        
        osip_free(cleanRealm);
        osip_free(reqUriStr);

        char* resp = str_enquote(Response);

        if (resp)
        {
            osip_proxy_authorization_set_response(aut, resp);
            osip_list_add (&osip_msg->proxy_authorizations, aut, -1);
            return RC_OK;
        }
    }

ERR_EXIT:
    if (aut) osip_authorization_free (aut);
    return RC_ERR;
}


