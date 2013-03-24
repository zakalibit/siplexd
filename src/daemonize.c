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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "logger.h"
#include "config.h"

void daemonize(void)
{
    int rc = 0;
    int daemonize = 0;

    if (RC_ERR == cfg_get_int("siplexd.daemonize", &daemonize))
    {
        logWARNING("Failed to read daemonize config parameter, will not run as daemon");
        return;
    }

    if (0 == daemonize)
    {
        logINFO("Will not run as daemon, disabled in the config.");
        return;
    }

    logDEBUG("Daemonizing...");

    if ((rc = fork()) == 0)
    {
        /* Session leader so ^C doesn't whack us. */
        setsid();

        /* Move off any mount points we might be in. */
        if (chdir("/") != 0)
        {
            logDEBUG("Failed to chdir(\"/\"): %s", strerror(errno));
        }

        if ((rc = fork()) == 0)
        {
            /* detach STDIN, STDOUT, STDERR */
            int nullfd = open("/dev/null", O_RDWR);

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            if (dup2(nullfd, STDIN_FILENO) < 0)
            {
                logWARNING("Detaching STDIN failed: %s", strerror(errno));
            }

            if (dup2(nullfd, STDOUT_FILENO) < 0)
            {
                logWARNING("Detaching STDOUT failed: %s", strerror(errno));
            }

            if (dup2(nullfd, STDERR_FILENO) < 0)
            {
                logWARNING("detaching STDERR failed: %s", strerror(errno));
            }

            close(nullfd);

            logINFO("Daemonized, pid=%i", getpid());

            return;
        }
    }

    if (-1 == rc) logFATAL("Failed to daemonize: %s", strerror(errno));
    log_flush();
    exit(0);
}
