/*
 * fdlibm_det.h - Deterministic Math Library Wrapper
 *
 * Wraps Sun fdlibm 5.3 for cross-platform deterministic IEEE 754
 * floating-point math. All functions produce bit-identical results
 * on Windows (x86/x64), macOS (ARM64/x86_64), and Linux.
 *
 * Original fdlibm: Copyright (C) 1993-2004 Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that the above notice is preserved.
 */

#ifndef FDLIBM_DET_H
#define FDLIBM_DET_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Endianness detection                                               */
/* ------------------------------------------------------------------ */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define FDLIBM_LITTLE_ENDIAN 1
  #else
    #define FDLIBM_LITTLE_ENDIAN 0
  #endif
#elif defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || \
      defined(__aarch64__) || defined(__arm__) || defined(_M_IX86) || \
      defined(_M_X64) || defined(_M_ARM64)
  #define FDLIBM_LITTLE_ENDIAN 1
#else
  #error "fdlibm_det.h: cannot determine endianness for this platform"
#endif

#if FDLIBM_LITTLE_ENDIAN
  #define __HI(x) *(1+(int*)&x)
  #define __LO(x) *(int*)&x
  #define __HIp(x) *(1+(int*)x)
  #define __LOp(x) *(int*)x
#else
  #define __HI(x) *(int*)&x
  #define __LO(x) *(1+(int*)&x)
  #define __HIp(x) *(int*)x
  #define __LOp(x) *(1+(int*)x)
#endif

/* ------------------------------------------------------------------ */
/*  Public deterministic math API                                      */
/* ------------------------------------------------------------------ */

double fdlibm_sin(double x);
double fdlibm_cos(double x);
double fdlibm_sqrt(double x);
double fdlibm_acos(double x);
double fdlibm_asin(double x);
double fdlibm_atan(double x);
double fdlibm_atan2(double y, double x);
double fdlibm_fabs(double x);
double fdlibm_tan(double x);
double fdlibm_floor(double x);
double fdlibm_copysign(double x, double y);
double fdlibm_scalbn(double x, int n);

/* ------------------------------------------------------------------ */
/*  Internal kernel functions (used by the .c files)                   */
/* ------------------------------------------------------------------ */

double __kernel_sin(double x, double y, int iy);
double __kernel_cos(double x, double y);
int    __ieee754_rem_pio2(double x, double *y);
int    __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2);
double __ieee754_sqrt(double x);
double __ieee754_acos(double x);
double __ieee754_asin(double x);
double __ieee754_atan2(double y, double x);
double __kernel_tan(double x, double y, int iy);

#ifdef __cplusplus
}
#endif

#endif /* FDLIBM_DET_H */
