/* $Id: md5.h,v 1.2 2000/03/27 22:23:25 shmit Exp $ */

/*
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991.
 * All rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#ifndef MD5_H
#	define MD5_H

#include "compat.h"

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* MD5 context. */
typedef struct {
  uint32_t state[4];			/* state (ABCD) */
  uint32_t count[2];			/* number of bits, modulo 2^64 */
					/* (lsb first) */
  unsigned char buffer[64];		/* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, const unsigned char *, unsigned int);
void MD5Final(unsigned char *, MD5_CTX *, int outputlen);
void md5_calc(unsigned char *output, const unsigned char *input,
	      unsigned int inlen, int outputlen);

#endif /* MD5_H */
