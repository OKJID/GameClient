/*
 * fdlibm_det.c - Deterministic math wrapper implementations
 *
 * Public fdlibm_* functions that call the internal __ieee754_* and
 * __kernel_* routines from Sun's fdlibm 5.3, producing bit-identical
 * IEEE 754 results on every platform.
 *
 * Original fdlibm: Copyright (C) 1993-2004 Sun Microsystems, Inc.
 */

#include "fdlibm_det.h"

double fdlibm_sin(double x)
{
    double y[2], z = 0.0;
    int n, ix;

    ix = __HI(x);
    ix &= 0x7fffffff;

    if (ix <= 0x3fe921fb) return __kernel_sin(x, z, 0);
    else if (ix >= 0x7ff00000) return x - x;
    else {
        n = __ieee754_rem_pio2(x, y);
        switch (n & 3) {
            case 0: return  __kernel_sin(y[0], y[1], 1);
            case 1: return  __kernel_cos(y[0], y[1]);
            case 2: return -__kernel_sin(y[0], y[1], 1);
            default: return -__kernel_cos(y[0], y[1]);
        }
    }
}

double fdlibm_cos(double x)
{
    double y[2], z = 0.0;
    int n, ix;

    ix = __HI(x);
    ix &= 0x7fffffff;

    if (ix <= 0x3fe921fb) return __kernel_cos(x, z);
    else if (ix >= 0x7ff00000) return x - x;
    else {
        n = __ieee754_rem_pio2(x, y);
        switch (n & 3) {
            case 0: return  __kernel_cos(y[0], y[1]);
            case 1: return -__kernel_sin(y[0], y[1], 1);
            case 2: return -__kernel_cos(y[0], y[1]);
            default: return  __kernel_sin(y[0], y[1], 1);
        }
    }
}

double fdlibm_tan(double x)
{
    double y[2], z = 0.0;
    int n, ix;

    ix = __HI(x);
    ix &= 0x7fffffff;

    if (ix <= 0x3fe921fb) return __kernel_tan(x, z, 1);
    else if (ix >= 0x7ff00000) return x - x;
    else {
        n = __ieee754_rem_pio2(x, y);
        return __kernel_tan(y[0], y[1], 1 - ((n & 1) << 1));
    }
}

double fdlibm_sqrt(double x)
{
    return __ieee754_sqrt(x);
}

double fdlibm_acos(double x)
{
    return __ieee754_acos(x);
}

double fdlibm_asin(double x)
{
    return __ieee754_asin(x);
}

double fdlibm_atan(double x)
{
    static const double atanhi[] = {
        4.63647609000806093515e-01,
        7.85398163397448278999e-01,
        9.82793723247329054082e-01,
        1.57079632679489655800e+00,
    };
    static const double atanlo[] = {
        2.26987774529616870924e-17,
        3.06161699786838301793e-17,
        1.39033110312309984516e-17,
        6.12323399573676603587e-17,
    };
    static const double aT[] = {
        3.33333333333329318027e-01,
       -1.99999999998764832476e-01,
        1.42857142725034663711e-01,
       -1.11111104054623557880e-01,
        9.09090909090613630880e-02,
       -7.69230544424387498824e-02,
        6.66661077914208411956e-02,
       -5.88174533015515605090e-02,
        4.97687799461593236017e-02,
       -3.65315727442169155270e-02,
        1.62858201153657823623e-02,
    };
    static const double one = 1.0, huge = 1.0e300;

    double w, s1, s2, z;
    int ix, hx, id;

    hx = __HI(x);
    ix = hx & 0x7fffffff;

    if (ix >= 0x44100000) {
        if (ix > 0x7ff00000 || (ix == 0x7ff00000 && (__LO(x) != 0)))
            return x + x;
        if (hx > 0) return  atanhi[3] + atanlo[3];
        else        return -atanhi[3] - atanlo[3];
    }

    if (ix < 0x3fdc0000) {
        if (ix < 0x3e200000) {
            if (huge + x > one) return x;
        }
        id = -1;
    } else {
        x = fdlibm_fabs(x);
        if (ix < 0x3ff30000) {
            if (ix < 0x3fe60000) {
                id = 0; x = (2.0 * x - one) / (2.0 + x);
            } else {
                id = 1; x = (x - one) / (x + one);
            }
        } else {
            if (ix < 0x40038000) {
                id = 2; x = (x - 1.5) / (one + 1.5 * x);
            } else {
                id = 3; x = -1.0 / x;
            }
        }
    }

    z = x * x;
    w = z * z;
    s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] + w * aT[10])))));
    s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));

    if (id < 0) return x - x * (s1 + s2);
    else {
        z = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
        return (hx < 0) ? -z : z;
    }
}

double fdlibm_atan2(double y, double x)
{
    return __ieee754_atan2(y, x);
}

double fdlibm_fabs(double x)
{
    __HI(x) &= 0x7fffffff;
    return x;
}

double fdlibm_floor(double x)
{
    static const double huge_val = 1.0e300;
    int i0, i1, j0;
    unsigned i, j;

    i0 = __HI(x);
    i1 = __LO(x);
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;

    if (j0 < 20) {
        if (j0 < 0) {
            if (huge_val + x > 0.0) {
                if (i0 >= 0) { i0 = 0; i1 = 0; }
                else if (((i0 & 0x7fffffff) | i1) != 0) {
                    i0 = (int)0xbff00000; i1 = 0;
                }
            }
        } else {
            i = (0x000fffff) >> j0;
            if (((i0 & i) | i1) == 0) return x;
            if (huge_val + x > 0.0) {
                if (i0 < 0) i0 += (0x00100000) >> j0;
                i0 &= (~i); i1 = 0;
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400) return x + x;
        else return x;
    } else {
        i = ((unsigned)(0xffffffff)) >> (j0 - 20);
        if ((i1 & i) == 0) return x;
        if (huge_val + x > 0.0) {
            if (i0 < 0) {
                if (j0 == 20) i0 += 1;
                else {
                    j = i1 + (1 << (52 - j0));
                    if (j < (unsigned)i1) i0 += 1;
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    __HI(x) = i0;
    __LO(x) = i1;
    return x;
}

double fdlibm_copysign(double x, double y)
{
    __HI(x) = (__HI(x) & 0x7fffffff) | (__HI(y) & 0x80000000);
    return x;
}

double fdlibm_scalbn(double x, int n)
{
    static const double
        two54  = 1.80143985094819840000e+16,
        twom54 = 5.55111512312578270212e-17;
    static const double huge_val = 1.0e+300, tiny_val = 1.0e-300;

    int k, hx, lx;
    hx = __HI(x);
    lx = __LO(x);
    k = (hx & 0x7ff00000) >> 20;

    if (k == 0) {
        if ((lx | (hx & 0x7fffffff)) == 0) return x;
        x *= two54;
        hx = __HI(x);
        k = ((hx & 0x7ff00000) >> 20) - 54;
        if (n < -50000) return tiny_val * x;
    }
    if (k == 0x7ff) return x + x;
    k = k + n;
    if (k > 0x7fe) return huge_val * fdlibm_copysign(huge_val, x);
    if (k > 0) {
        __HI(x) = (hx & 0x800fffff) | (k << 20);
        return x;
    }
    if (k <= -54) {
        if (n > 50000)
            return huge_val * fdlibm_copysign(huge_val, x);
        else
            return tiny_val * fdlibm_copysign(tiny_val, x);
    }
    k += 54;
    __HI(x) = (hx & 0x800fffff) | (k << 20);
    return x * twom54;
}
