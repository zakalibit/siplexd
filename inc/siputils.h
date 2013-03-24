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

#ifndef _SIPUTILS_H
#define	_SIPUTILS_H
#include <osip2/osip.h>
#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @brief Gets the port number from a string, if it is not in the valid range
 *        it defaults do default sip port 5060
 * @param s_port - string representation of the fort number
 * @retval converted from string port number, or default value
 */
int get_port_from_str(const char* s_port);

int is_proxy_address(char * host, char* port);

/**
 * @brief Builds a sip message using an existing message as a template
 * @param sipmsg - source sip message used as a template.
 * @param code - sip message response code
 * @retval sip message built from source
 */
osip_message_t* init_sip_msg_from_src (const osip_message_t *sipmsg, const int code);

/**
 * @brief Generates a sip message from template and sends it back to the sender.
 * @param sipsrc - source sip message used as a template.
 * @param code - sip message response code
 * @retval Returns 0, or -1 for errors
 */
void gen_and_send_sip_response(const osip_message_t *sipsrc, const int code);

void update_max_fowards(osip_message_t* sipmsg);
int  get_stateles_branch_id (osip_message_t* sipmsg, char* branch_id);
int calculate_next_hop(sip_msg_str_t* sms);

int add_proxy_via (sip_msg_str_t* sms);
int remove_proxy_via(osip_message_t* sipmsg);
#ifdef	__cplusplus
}
#endif

#endif	/* _SIPUTILS_H */

