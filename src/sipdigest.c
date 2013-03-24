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

#include <string.h>
#include <osipparser2/osip_md5.h>
#include <osipparser2/osip_port.h>
#include "sipdigest.h"


/**
 * The routines below have been taken from linphone
 * (osipua/src/authentication.c)
 */
void CvtHex(
	    IN  HASH Bin,
	    OUT HASHHEX Hex
	    )
{
  unsigned short i;
  unsigned char j;

  for (i = 0; i < HASHLEN; i++)
  {
    j = (Bin[i] >> 4) & 0xf;
    Hex[i*2] = (j <= 9)? (j + '0'): (j + 'a' - 10);

    j = Bin[i] & 0xf;
    Hex[i*2+1] = (j <= 9)? (j + '0'): (j + 'a' - 10);
  }
  Hex[HASHHEXLEN] = '\0';
}

/* calculate H(A1) as per spec */
void DigestCalcHA1(
		   IN const char * pszAlg,
		   IN const char * pszUserName,
		   IN const char * pszRealm,
		   IN const char * pszPassword,
		   IN const char * pszNonce,
		   IN const char * pszCNonce,
		   OUT HASHHEX SessionKey
		   )
{
  osip_MD5_CTX Md5Ctx;
  HASH HA1;

  osip_MD5Init(&Md5Ctx);
  if (pszUserName) osip_MD5Update(&Md5Ctx, (unsigned char*)pszUserName,
                                  strlen(pszUserName));
  osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  if (pszRealm)    osip_MD5Update(&Md5Ctx, (unsigned char*)pszRealm,
                                  strlen(pszRealm));
  osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  if (pszPassword) osip_MD5Update(&Md5Ctx, (unsigned char*)pszPassword,
                                  strlen(pszPassword));
  osip_MD5Final(HA1, &Md5Ctx);

  if ((pszAlg!=NULL) && (osip_strcasecmp(pszAlg, "md5-sess") == 0))
  {
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, HA1, HASHLEN);
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    if (pszNonce)  osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonce,
                                  strlen(pszNonce));
    osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
    if (pszCNonce) osip_MD5Update(&Md5Ctx, (unsigned char*)pszCNonce,
                                  strlen(pszCNonce));
    osip_MD5Final(HA1, &Md5Ctx);
  }
  CvtHex(HA1, SessionKey);
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
			IN HASHHEX HA1,         /* H(A1) */
			IN const char * pszNonce,     /* nonce from server */
			IN const char * pszNonceCount,  /* 8 hex digits */
			IN const char * pszCNonce,    /* client nonce */
			IN const char * pszQop,       /* qop-value: "", "auth", "auth-int" */
			IN const char * pszMethod,    /* method from the request */
			IN const char * pszDigestUri, /* requested URL */
			IN HASHHEX HEntity,     /* H(entity body) if qop="auth-int" */
			OUT HASHHEX Response    /* request-digest or response-digest */
			)
{
  osip_MD5_CTX Md5Ctx;
  HASH HA2;
  HASH RespHash;
  HASHHEX HA2Hex;

  /* calculate H(A2) */
  osip_MD5Init(&Md5Ctx);
  if (pszMethod)    osip_MD5Update(&Md5Ctx, (unsigned char*)pszMethod,
                                   strlen(pszMethod));
  osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  if (pszDigestUri) osip_MD5Update(&Md5Ctx, (unsigned char*)pszDigestUri,
                                   strlen(pszDigestUri));

  if (NULL == pszQop)
  {
      /* auth_withoutqop: */
     osip_MD5Final(HA2, &Md5Ctx);
     CvtHex(HA2, HA2Hex);

     /* calculate response */
     osip_MD5Init(&Md5Ctx);
     osip_MD5Update(&Md5Ctx, HA1, HASHHEXLEN);
     osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
     if (pszNonce)  osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
     osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  }
  else
  {
      osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
      osip_MD5Update(&Md5Ctx, HEntity, HASHHEXLEN);
      osip_MD5Final(HA2, &Md5Ctx);
      CvtHex(HA2, HA2Hex);

      /* calculate response */
      osip_MD5Init(&Md5Ctx);
      osip_MD5Update(&Md5Ctx, HA1, HASHHEXLEN);
      osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
      if (pszNonce)    osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonce, strlen(pszNonce));
      osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
      if (pszNonceCount) osip_MD5Update(&Md5Ctx, (unsigned char*)pszNonceCount, strlen(pszNonceCount));
      osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
      if (pszCNonce)  osip_MD5Update(&Md5Ctx, (unsigned char*)pszCNonce, strlen(pszCNonce));
      osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
      if (pszQop)  osip_MD5Update(&Md5Ctx, (unsigned char*)pszQop, strlen(pszQop));
      osip_MD5Update(&Md5Ctx, (unsigned char*)":", 1);
  }

  osip_MD5Update(&Md5Ctx, HA2Hex, HASHHEXLEN);
  osip_MD5Final(RespHash, &Md5Ctx);
  CvtHex(RespHash, Response);
}

