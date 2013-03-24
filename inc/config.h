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

#ifndef _CONFIG_H
#define	_CONFIG_H
#include <libconfig.h>

#ifdef	__cplusplus
extern "C" {
#endif
    config_t* cfg_instance(void);
    int  cfg_initialize(const char* const cfg_fname);
    void cfg_close(void);
    int  cfg_get_int(const char* const path, int* value);
    int  cfg_get_string(const char* const path, const char** string);
#ifdef	__cplusplus
}
#endif

#endif	/* _CONFIG_H */

