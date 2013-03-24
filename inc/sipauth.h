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

#ifndef _SIPAUTH_H
#define	_SIPAUTH_H

#include <osip2/osip.h>
#include <osipparser2/osip_md5.h>

#ifdef	__cplusplus
extern "C" {
#endif


int add_proxy_auth_header(osip_message_t* osip_msg,
                           osip_proxy_authenticate_t * proxy_auth,
                           char* username,
                           char* password);

#ifdef	__cplusplus
}
#endif

#endif	/* _SIPAUTH_H */

