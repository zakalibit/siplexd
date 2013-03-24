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
#include <string.h>
#include "dpparse.h"

/*
 * Maximum length of a dial plan. 2047 limit is for Linksys PAP2
*/
#define DP_MAX_LEN 2047

static const char VALID_DP_CHARS[]="0123456789|Sgwx*!,.#:<>@-()[] ";
static const int  VALID_DP_CHARS_LEN=sizeof(VALID_DP_CHARS);

static const char* err_strs[]={
  "Dial plan contains invalid characters.\0",
  "Dial plan is empty.\0",
  0
};


char* dp_remove_blanks(const char* const raw_dp,
                          const size_t      raw_dp_len,
                          size_t* const     clean_dp_len)
{
    char* const clean_dp = (char*)malloc (raw_dp_len);

    int j = 0;

    for (int i = 0; NULL != clean_dp && i < raw_dp_len; i++)
    {
        if (raw_dp[i] != ' ')  clean_dp[j++] = raw_dp[i];
    }
    
    if (j > 0)
    {
        clean_dp[j] = '\0';
        if (NULL != clean_dp_len) *clean_dp_len = j;
    }
    else
    {
        if (NULL != clean_dp) free (clean_dp);
        return NULL;
    }
    
    return clean_dp;
}

int dp_validate_chars (const char* const raw_dp, const size_t raw_dp_len)
{
    int i = raw_dp_len;
    int j;

    while (i > 0)
    {
        i--;
        for (j=0; j < VALID_DP_CHARS_LEN ;j++)
        {
            if (raw_dp[i] == VALID_DP_CHARS[j]) break;
        }

        if (j>=VALID_DP_CHARS_LEN) break;
    }

    return (i > 0)? -1: 0;
}

void  dp_free (dpps* dpps)
{
    if (dpps != 0){
        free (dpps->dpptr);
        free (dpps->offsets);
        free (dpps);
    }
};

dpps* dial_plan_parse(const char* raw_dp, const char** errptr)
{
    if (NULL == raw_dp || *raw_dp == '\0') return NULL;

    int clean_dp_len = 0;
    int raw_dp_len = strlen (raw_dp);

    if (-1 == dp_validate_chars (raw_dp, raw_dp_len))
    {
        *errptr = err_strs[0];
        return NULL;
    }
    
    char* const clean_dp = dp_remove_blanks(raw_dp,
                                                 raw_dp_len,
                                                 &clean_dp_len);

    if (clean_dp)
    {
        dpps* pdpps = (dpps*)malloc (sizeof(dpps));

        pdpps->noffsets = 1;

        /* calculate number of dialplan components */
        char* p=clean_dp;
        for (; *p!='\0';p++)
        {
            if (*p =='|' && *(p+1) != '|' && *(p+1) != '\0') pdpps->noffsets++;
        }

        pdpps->offsets = (int*)malloc (pdpps->noffsets * sizeof (size_t));
        pdpps->offsets[0] = 0;

        int noff = 1;
        p = clean_dp;
        for (; *p!='\0'; p++)
        {
            if (*p =='|' && *(p+1) != '|' && *(p+1) != '\0')
            {
                *p = '\0';                            
                 pdpps->offsets[noff++] = (p+1) - clean_dp;
            }
        }

        pdpps->dpptr = clean_dp;

        return pdpps;
    }
    else{
        *errptr = err_strs[1];
    }

    return NULL;
}

int dp_match (const dpps * pdpps, const char* const pnumber, char** pnew_number)
{
    int i;
    char* cdp;

    for (i=0;i<pdpps->noffsets;i++){
        cdp = pdpps->dpptr + pdpps->offsets[i];
    }
 
    return -1;
}