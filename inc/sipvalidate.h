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

#ifndef _SIPVALIDATE_H
#define	_SIPVALIDATE_H

#include <osip2/osip.h>

#ifdef	__cplusplus
extern "C" {
#endif


int sip_check_max_forwards(const osip_message_t *sipmsg);
int sip_check_via_loop (const osip_message_t* sipmsg);
int sip_via_loop_not_detected(const osip_message_t* sipmsg);

#ifdef	__cplusplus
}
#endif

#endif	/* _SIPVALIDATE_H */

