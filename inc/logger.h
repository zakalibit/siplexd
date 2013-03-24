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

#ifndef _LOGGER_H
#define	_LOGGER_H

#include <syslog.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define RC_OK (0)
#define RC_ERR (-1)
    
enum logLEVEL
{
    lvlFATAL,
    lvlERROR,
    lvlWARNING,
    lvlINFO,
    lvlDEBUG
};

void log_init(void);

void log_main
(
    const char*       label,
    const int         log_level,
    const char* const file,
    const size_t      line,
    const char*       format,
    ...
);

void log_flush(void);

#if defined(DEBUG)
#define logDEBUG(L...)\
log_main("DEBUG:", lvlDEBUG, __FILE__, __LINE__,L)

/* Trace function entry */
#define __tri(__FNAME__)\
 static const char* __fname = (#__FNAME__"(...)");\
 logDEBUG("Entering %s ...", __fname)

/* Trace function exit */
#define __tre(opr)\
 logDEBUG("Exiting %s ...", __fname);\
 opr

#else
#define logDEBUG(L...) 0
#define __tri(name) 0
#define __tre(opr) opr
#endif

#define logINFO(L...)\
log_main("INFO:", lvlINFO, __FILE__, __LINE__,L)

#define logWARNING(L...)\
log_main("WARNING:", lvlWARNING, __FILE__, __LINE__,L)

#define logERROR(L...)\
log_main("ERROR:", lvlERROR, __FILE__, __LINE__,L)

#define logFATAL(L...)\
log_main("FATAL:", lvlFATAL, __FILE__, __LINE__,L)


#ifdef	__cplusplus
}
#endif

#endif	/* _LOGGER_H */

