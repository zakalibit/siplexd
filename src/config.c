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

#include <libconfig.h>
//#include "logger.h"

static struct config_t cfg;
static int cfg_initialized = 0;

config_t* cfg_instance(void)
{
    return (cfg_initialized)? &cfg: NULL;
}

int cfg_initialize(const char* const cfg_fname)
{
    /* Initialize the configuration */
    config_init(&cfg);

    if (config_read_file(&cfg, cfg_fname))
    {
        cfg_initialized = 1;
        return 0;
    }

    cfg_initialized = 0;
    
    fprintf (stderr, "config_read_file() - failed: %s line number %d.\n", cfg.error_text, cfg.error_line);
    
    return -1;
}

void cfg_close(void)
{
    /* Free the configuration */
    if (cfg_initialized) config_destroy(&cfg);
}

int cfg_get_int(const char* const path, int* value)
{
    return (config_lookup_int (&cfg, path, value))? 0: -1;
}

int cfg_get_string(const char* const path, const char** string)
{
    return (config_lookup_string(&cfg, path, string))? 0: -1;
}
