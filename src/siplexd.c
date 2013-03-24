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
#include <stdlib.h>
#include <sys/time.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include "config.h"
#include "sock.h"
#include "sipregister.h"
#include "logger.h"
#include "sighandler.h"
#include "daemonize.h"
/*
 * 
 */

int main(int argc, char** argv)
{
    static char cfg_fname[PATH_MAX] = "siplexd.cfg";

    int opt = -1;

    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
        switch (opt)
        {
            case 'f':
                strncpy (cfg_fname, optarg, sizeof(cfg_fname)-1);
                cfg_fname[sizeof(cfg_fname)-1]='\0';
                break;

            default:
                break;
        }
    }

    if (RC_ERR == cfg_initialize(cfg_fname)) return (EXIT_FAILURE);

    log_init();

    if (RC_ERR == initialize_transport())
    {
        cfg_close();
        return (EXIT_FAILURE);
    }

    setup_sig_handlers();

    daemonize();
    
    /* init the oSIP parser */
    parser_init();

    sip_register_map_init();

    logDEBUG("Going in to message processing loop...");
    

    /* main sip processing loop */
    sip_events_process();

    logINFO("siplexd is shutting down...");
    shutdown_transport();
    sip_register_map_free();
    cfg_close();
    return (EXIT_SUCCESS);
}
