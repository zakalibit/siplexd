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
#include <signal.h>
#include "logger.h"

int siplexd_execute = 1;
int dump_dmalloc = 0;

static void sig_handler(int sig)
{
    switch (sig)
    {
        case SIGTERM:
        case SIGINT:
            siplexd_execute = 0;
            break;
        case SIGUSR2:
            dump_dmalloc = 1;
            break;
    }
}

void setup_sig_handlers(void)
{
   struct sigaction sa;

   sa.sa_handler=sig_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags=SA_RESTART;
   if (sigaction(SIGTERM, &sa, NULL))
   {
       logERROR("Failed to install SIGTERM handler");
   }
   if (sigaction(SIGINT, &sa, NULL)) {
      logERROR("Failed to install SIGINT handler");
   }
   if (sigaction(SIGUSR2, &sa, NULL))
   {
      logERROR("Failed to install SIGUSR2 handler");
   }
   if (sigaction(SIGPIPE, &sa, NULL))
   {
      logERROR("Failed to install SIGPIPE handler");
   }
}
