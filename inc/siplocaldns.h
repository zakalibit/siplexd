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

#ifndef _SIPLOCALDNS_H
#define	_SIPLOCALDNS_H

#ifdef	__cplusplus
extern "C" {
#endif

    void local_dns_init(void);
    void local_dns_close(void);

    void local_dns_add(const char* host);

    int local_dns_lookup(const char* const hostName,
                          const struct in_addr** addr);


#ifdef	__cplusplus
}
#endif

#endif	/* _SIPLOCALDNS_H */

