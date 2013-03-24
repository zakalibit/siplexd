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

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include "logger.h"
#include "sock.h"
#include "config.h"

struct log_cfg_t
{
    int log_to_file;
    int log_file_max_size;
    int log_to_stderr;
    int log_to_syslog;
    int log_to_udp;
}log_cfg;

static void logToFp
(
                FILE* fp,
    const char*       label,
    const int         log_level,
    const char* const file,
    const size_t      line,
    const char*       format,
    va_list           vl
)
{
    va_list vl_copy;
    time_t t_time;
    struct tm *ptime;

    time(&t_time);
    ptime = localtime(&t_time);

    fprintf(fp, "%i %2.2i:%2.2i:%2.2i ",
            log_level,
            ptime->tm_hour,
            ptime->tm_min,
            ptime->tm_sec);
            
#ifdef DEBUG
    fprintf(fp, "%s:%i ", file, line);
#endif

    va_copy(vl_copy, vl);
    vfprintf(fp, format, vl_copy);
    va_end(vl_copy);
   
    fprintf(fp, "\n");
}

static FILE* logFP = NULL;
static int udpLoggerScoket = TCP_INVALID_SOCKET;
static struct sockaddr_in udpLoggerAddr;

int initUdpLogger(void)
{
    int port = -1;
    log_cfg.log_to_udp = 0;

    if (RC_ERR == cfg_get_int("siplexd.logging.udplog.port", &port) ||
        TCP_INVALID_PORT(port))
    {
        return RC_ERR;
    }

    int sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (TCP_INVALID_SOCKET != sock)
    {
        int broadcast;

        if (RC_ERR == cfg_get_int("siplexd.logging.udplog.broadcast", &broadcast))
            broadcast = 0;

        if (!broadcast || 
            RC_OK == setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))
           )
        {
            const char* ipaddr;

            if (RC_ERR == cfg_get_string("siplexd.logging.udplog.ipaddr", &ipaddr))
            {
                return RC_ERR;
            }

            memset(&udpLoggerAddr, 0, sizeof(struct sockaddr_in));
            
            udpLoggerAddr.sin_family = AF_INET;
            udpLoggerAddr.sin_port   = htons(port);
            udpLoggerAddr.sin_addr.s_addr = inet_addr(ipaddr);

            udpLoggerScoket = sock;

            return RC_OK;
        }
        else logERROR("setsockopt() failed: %s", strerror(errno));

        close (sock);
    }
    else logERROR("Could not create UDP logging socket: %s", strerror(errno));

    return RC_ERR;
}

static void logToUdp
(
    const char*       label,
    const int         log_level,
    const char* const file,
    const size_t      line,
    const char*       format,
    va_list           vl
)
{
    char buffer[256];
    va_list vl_copy;

    va_copy(vl_copy, vl);
    vsnprintf(buffer, sizeof(buffer), format, vl_copy);
    va_end(vl_copy);

    if (log_cfg.log_to_syslog)
    {
        int syslevel;
        switch (log_level)
        {            
            case lvlFATAL:   syslevel = LOG_EMERG; break;
            case lvlERROR:   syslevel = LOG_ERR; break;
            case lvlWARNING: syslevel = LOG_WARNING; break;
            case lvlINFO:    syslevel = LOG_INFO; break;
            case lvlDEBUG:   syslevel = LOG_DEBUG; break;
            default:         syslevel = LOG_WARNING;
        }
        syslog(LOG_USER|syslevel,
                #ifdef DEBUG
                "%s:%i "
                #endif
                "%s %s",
                #ifdef DEBUG
                file, 
                line, 
                #endif
                label,
                buffer);
    }
    
    if (log_cfg.log_to_udp)
    {
        char tmp[256];
        time_t t_time;
        struct tm *ptime;

        time(&t_time);
        ptime = localtime(&t_time);

        int len = snprintf(tmp, sizeof(tmp),
                           "%s %2.2i:%2.2i:%2.2i %s",
                           label,
                           ptime->tm_hour,
                           ptime->tm_min,
                           ptime->tm_sec,
                           buffer);

        if (len > 0)
        {

            sendto (udpLoggerScoket,
                   tmp, len, 0 ,
                   (struct sockaddr*)&udpLoggerAddr,
                   sizeof (udpLoggerAddr));
        }
    }
}

void log_init(void)
{
    /* Init File logger */
    if (RC_OK == cfg_get_int("siplexd.logging.file.enabled", &log_cfg.log_to_file) &&
        log_cfg.log_to_file)
    {
        const char* path;

        if (NULL == logFP && RC_OK == cfg_get_string("siplexd.logging.file.name", &path))
        {
            logFP = fopen (path, "a+");
        }

        if (RC_ERR == cfg_get_int("siplexd.logging.file.size", &log_cfg.log_file_max_size))
            log_cfg.log_file_max_size = 10;

        log_cfg.log_file_max_size *= 1024*1000;
    }

    if (NULL == logFP) log_cfg.log_to_file = 0;

    /* Init stderr logger */
    if (RC_ERR == cfg_get_int("siplexd.logging.stderr", &log_cfg.log_to_stderr))
        log_cfg.log_to_stderr = 0;

    /* Init syslog */
    if (RC_OK == cfg_get_int("siplexd.logging.syslog", &log_cfg.log_to_syslog) &&
        log_cfg.log_to_syslog)
    {
        openlog(NULL, LOG_NDELAY|LOG_PID, LOG_DAEMON);
    }
    else log_cfg.log_to_syslog = 0;

    /* Init udp broadcast logger */
    if (RC_OK == cfg_get_int("siplexd.logging.udplog.enabled", &log_cfg.log_to_udp) &&
        log_cfg.log_to_udp &&
        RC_OK == initUdpLogger())
    {
        log_cfg.log_to_udp = 1;
    }
    else log_cfg.log_to_udp = 0;

}

void log_main
(
    const char*       label,
    const int         log_level,
    const char* const file,
    const size_t      line,
    const char*       format,
    ...
)
{        
    va_list vl;
    va_start(vl, format);
    if (log_cfg.log_to_syslog || log_cfg.log_to_udp)
    {
        logToUdp(label, log_level, file, line, format, vl);
    }

    if (log_cfg.log_to_stderr)
    {
        logToFp(stderr, label, log_level, file, line, format, vl);
    }

    if (log_cfg.log_to_file)
    {
        logToFp(logFP, label, log_level, file, line, format, vl);
    }

    va_end(vl);
    
#if defined(DEBUG)
    log_flush();
#endif
}

void log_flush(void)
{
    if (log_cfg.log_to_file && NULL != logFP)
    {
        fflush(logFP);
    
        if (log_cfg.log_file_max_size <= ftell(logFP))
        {
            ftruncate (fileno(logFP), 0);
        }
    }
}