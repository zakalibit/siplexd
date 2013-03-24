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

#ifndef _DPPARSE_H
#define	_DPPARSE_H

#ifdef	__cplusplus
extern "C" {
#endif

/* struct that holds a parsed dial plan */
typedef struct
{
    char*   dpptr;
    size_t  noffsets;
    size_t* offsets;
}dpps;

dpps* dial_plan_parse(const char* rawDialPlan, const char** errptr);
void  dial_plan_free(dpps* __dpps);


#ifdef	__cplusplus
}
#endif

#endif	/* _DPPARSE_H */

