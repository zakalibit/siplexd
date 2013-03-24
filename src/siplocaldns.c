
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
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#include <bits/local_lim.h>
#include <string.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_list.h>
#include "config.h"
#include "logger.h"

typedef struct
{
    char host[HOST_NAME_MAX];    
    struct in_addr addr;
}dns_cache_entry_t;

static int cacheLength = 0;

/*static dns_cache_entry_t* dnsCache = NULL;*/
static osip_list_t* dnsCache = NULL;

void local_dns_add(const char* host)
{
    __tri(local_dns_add);

    if (NULL != dnsCache && NULL != host && '\0' != host[0])
    {
        dns_cache_entry_t* dcEnt = (dns_cache_entry_t*)osip_malloc(sizeof(dns_cache_entry_t));

        if (NULL == dcEnt)
        {
            logERROR("Failed to allocate memory for a local DNS entry.");
            __tre(return);
        }

        strncpy(dcEnt->host, host, HOST_NAME_MAX -1);

        const int ret = inet_pton (AF_INET, host, &(dcEnt->addr));

        if (ret > 0)
        {
            if (osip_list_add(dnsCache, dcEnt, -1))   __tre(return);
        }
        /*if the input is not a valid IPv4 dotted-decimal string
         * try local DNS then gethostbyname()
         */
        else if (0 == ret)
        {            
            struct hostent* hostentry=gethostbyname(host);
            if (hostentry)
            {
                int i = 0;
                logINFO("%s resolved to:", host);
                while (hostentry->h_addr_list[i])
                {
                    logINFO("    %s", inet_ntoa(*((struct in_addr*)hostentry->h_addr_list[i++])));
                }

                memcpy(&(dcEnt->addr), hostentry->h_addr_list[0], sizeof(struct in_addr));

                if (osip_list_add(dnsCache, dcEnt, -1)) __tre(return);
            }
            else   errno = h_errno;
        }

        osip_free(dcEnt);
    }

    __tre(return);
}
        

void local_dns_init(void)
{
    __tri(local_dns_init);

    if (dnsCache)
    {
        logERROR("Looks like local dns cache is already initialized.");
        __tre(return);
    }

    int localDnsEnabled = 0;

    if (RC_ERR == cfg_get_int("siplexd.sip.interface.localdns.enabled", &localDnsEnabled) ||
        0 == localDnsEnabled)
    {
        logINFO("Local DNS disabled or not present in the configuration.");
        __tre(return);
    }

    dnsCache = (osip_list_t *) osip_malloc (sizeof (osip_list_t));

    if (NULL == dnsCache || RC_OK != osip_list_init(dnsCache))
    {
        logERROR("Could not initialize dns cache list.");
        dnsCache = NULL;
        __tre(return);
    }

    config_t* hCfg = cfg_instance();

    config_setting_t* setting = config_lookup(hCfg, "siplexd.sip.interface.localdns.cache");

    if (NULL != setting)
    {
        const int count = config_setting_length(setting);        

        for (int i = 0; i < count; i++)
        {
            const char *host, *ip;

            config_setting_t *dnsEnt = config_setting_get_elem(setting, i);

            if(config_setting_lookup_string(dnsEnt, "host", &host) &&
                    NULL != host && '\0' != host[0]
               && config_setting_lookup_string(dnsEnt, "ip", &ip) &&
                    NULL != ip && '\0' != ip)
            {
                dns_cache_entry_t* dcEnt = (dns_cache_entry_t*)osip_malloc(sizeof(dns_cache_entry_t));
                
                if (dcEnt && 
                    inet_pton (AF_INET, ip, &(dcEnt->addr)) > 0 &&
                    osip_list_add(dnsCache, dcEnt, -1))
                {
                    strncpy(dcEnt->host, host, HOST_NAME_MAX -1);                    
                }
                else osip_free(dcEnt);
            }
        }
    }

    if (osip_list_eol(dnsCache, 0)) logWARNING("Config DNS cache not found or empty.");

    __tre(return);
}

void* local_dns_free(void* ptr)
{
    osip_free(ptr);
    return ptr;
}

void local_dns_close(void)
{
    __tri(local_dns_close);

    osip_list_special_free(dnsCache, local_dns_free);
    dnsCache = NULL;

    __tre(return);
}

int local_dns_lookup(const char* const host,
                      const struct in_addr** addr)
{
    __tri(local_dns_lookup);

    if (NULL != dnsCache && NULL != host && '\0' != host[0])
    {
        int i = 0;
        while (!osip_list_eol(dnsCache, i))
        {
            dns_cache_entry_t* dcEnt = (dns_cache_entry_t*)osip_list_get(dnsCache, i);

            if (0 == strcasecmp(host, dcEnt->host))
            {                
                if (NULL != addr && NULL != *addr)
                {
                    *addr   = &(dcEnt->addr);
                }
                __tre(return) RC_OK;
            }

            i++;
        }
    }
    
    __tre(return) RC_ERR;
}