/*
 * fdlibm.h - Compatibility shim
 *
 * This replaces the original fdlibm.h so that all fdlibm .c files
 * automatically pick up our deterministic wrapper header with correct
 * endianness detection for all target platforms.
 *
 * Original: Copyright (C) 2004 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#ifndef FDLIBM_H
#define FDLIBM_H

#include "fdlibm_det.h"

#ifdef __STDC__
#define	__P(p)	p
#else
#define	__P(p)	()
#endif

#define atan      fdlibm_atan
#define fabs      fdlibm_fabs
#define floor     fdlibm_floor
#define scalbn    fdlibm_scalbn
#define copysign  fdlibm_copysign
#define sqrt      __ieee754_sqrt

#endif /* FDLIBM_H */
