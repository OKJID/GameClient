/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PreRTS.h"

#include "Common/Diagnostic/SimulationMathCrc.h"
#include "Common/XferCRC.h"
#include "WWMath/matrix3d.h"
#include "WWMath/wwmath.h"
#include "GameLogic/FPUControl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <time.h>

// Boundary inputs that expose FPU precision divergence (x87 vs SSE2 vs NEON):
// signed zeros, unit values, sub/super-normal magnitudes, values straddling the asin/acos domain,
// and angles used across the simulation.
static const float s_edge[] = {
	0.0f, -0.0f, 1.0f, -1.0f, 0.5f, -0.5f,
	1.0e-30f, -1.0e-30f, 1.0e30f, -1.0e30f,
	FLT_MIN, FLT_MAX, 1.0e-40f,
	0.9999999f, 1.0000001f, -0.9999999f,
	3.14159265f, 6.28318531f, 100.5f, 55788.84375f, 0.967302263f
};
static const Int s_edgeCount = sizeof(s_edge) / sizeof(s_edge[0]);

// Ordered (y, x) pairs for atan2/div: quadrant coverage plus the singular (0,0) and x/0, 0/0 cases.
static const double s_pairY[] = { 0.4, -2.7, 187.66, -1116.46, 0.000123, 0.0,  1.0, -1.0, 5.0 };
static const double s_pairX[] = { 1.3, 11.9, -59.13,  1412.47, 9999.5,   0.0, -1.0,  0.0, 0.0 };
static const Int s_pairCount = sizeof(s_pairY) / sizeof(s_pairY[0]);

// Per-function boundary inputs as raw {hi,lo} bit words. Each set targets the numerically sensitive
// points of its own function; bit-word construction is byte-identical on every compiler and platform
// (unlike hex-float literals, unsupported by VC6). Every value is also probed at +-1 ULP (see bitStep).

// Shared across all double functions: signed zeros, unit values, denormals, DBL_MIN/MAX, 1-ULP of one.
static const UnsignedInt s_universalWords[][2] = {
	{ 0x00000000, 0x00000000 },  // 0
	{ 0x80000000, 0x00000000 },  // -0
	{ 0x3FF00000, 0x00000000 },  // 1
	{ 0xBFF00000, 0x00000000 },  // -1
	{ 0x3FE00000, 0x00000000 },  // 0.5
	{ 0x00000000, 0x00000001 },  // smallest denormal
	{ 0x00080000, 0x00000000 },  // mid denormal
	{ 0x00100000, 0x00000000 },  // DBL_MIN (min normal)
	{ 0x7FEFFFFF, 0xFFFFFFFF },  // DBL_MAX
	{ 0x3FF00000, 0x00000001 },  // 1+1ULP
	{ 0x3FEFFFFF, 0xFFFFFFFF }   // 1-1ULP
};
static const Int s_universalCount = sizeof(s_universalWords) / sizeof(s_universalWords[0]);

// Atan: fdlibm region breakpoints (7/16, 11/16, 19/16, 39/16), tan(pi/8), small (atan~x), large (->pi/2).
static const UnsignedInt s_atanWords[][2] = {
	{ 0x3BC79CA1, 0x0C924223 },  // tiny (atan~x)
	{ 0x3FDC0000, 0x00000000 },  // 7/16 break
	{ 0x3FE60000, 0x00000000 },  // 11/16 break
	{ 0x3FDA8279, 0x99FCEF33 },  // tan(pi/8)
	{ 0x3FF30000, 0x00000000 },  // 19/16 break
	{ 0x40038000, 0x00000000 },  // 39/16 break
	{ 0x4341C379, 0x37E08000 },  // large ->pi/2
	{ 0x43400000, 0x00000000 }   // 2^53
};
static const Int s_atanCount = sizeof(s_atanWords) / sizeof(s_atanWords[0]);

// Exp: k*ln2 (results at powers of 2), fdlibm reduction breaks, overflow/underflow edges.
static const UnsignedInt s_expWords[][2] = {
	{ 0x4005BF0A, 0x8B145769 },  // e (input 1 elsewhere)
	{ 0x3FE62E42, 0xFEFA39EF },  // ln2 ->2
	{ 0x3FF62E42, 0xFEFA39EF },  // 2ln2 ->4
	{ 0xBFE62E42, 0xFEFA39EF },  // -ln2 ->0.5
	{ 0x3FD62E42, 0xFEFA39EF },  // 0.5ln2 fdlibm break
	{ 0x3FF0A2B2, 0x3F3BAB73 },  // 1.5ln2 fdlibm break
	{ 0x40862E42, 0xFEFA39EF },  // overflow edge
	{ 0x40862E42, 0xFEFA39F0 },  // just-over ->Inf
	{ 0xC086232B, 0xDD7ABCD2 },  // min-normal edge
	{ 0xC0874910, 0xD52D3052 },  // min-denormal edge
	{ 0x3BC79CA1, 0x0C924223 }   // tiny (~1)
};
static const Int s_expCount = sizeof(s_expWords) / sizeof(s_expWords[0]);

// Log: catastrophic cancellation near 1.0, sqrt2 reduction bounds, e-powers, tiny.
static const UnsignedInt s_logWords[][2] = {
	{ 0x3FF00000, 0x1AD7F29B },  // just>1
	{ 0x3FEFFFFF, 0xCA501ACB },  // just<1
	{ 0x4005BF0A, 0x8B145769 },  // e ->1
	{ 0x401D8E64, 0xB8D4DDAD },  // e^2 ->2
	{ 0x3FD78B56, 0x362CEF38 },  // 1/e ->-1
	{ 0x40000000, 0x00000000 },  // 2 ->ln2
	{ 0x3FE6A09E, 0x667F3BCD },  // sqrt2/2 reduce bound
	{ 0x3FF6A09E, 0x667F3BCD },  // sqrt2 reduce bound
	{ 0x01A56E1F, 0xC2F8F359 }   // tiny ->big neg
};
static const Int s_logCount = sizeof(s_logWords) / sizeof(s_logWords[0]);

// Log10: integer results at powers of 10, near 1.0.
static const UnsignedInt s_log10Words[][2] = {
	{ 0x40240000, 0x00000000 },  // 10 ->1
	{ 0x40590000, 0x00000000 },  // 100 ->2
	{ 0x408F4000, 0x00000000 },  // 1000 ->3
	{ 0x3FB99999, 0x9999999A },  // 0.1 ->-1
	{ 0x3F847AE1, 0x47AE147B },  // 0.01 ->-2
	{ 0x4202A05F, 0x20000000 },  // 1e10 ->10
	{ 0x3FF00000, 0x1AD7F29B }   // just>1
};
static const Int s_log10Count = sizeof(s_log10Words) / sizeof(s_log10Words[0]);

// Sin/Cos: multiples of pi/2 (worst argument reduction), large-argument reduction stress.
static const UnsignedInt s_trigWords[][2] = {
	{ 0x3BC79CA1, 0x0C924223 },  // tiny
	{ 0x3FE0C152, 0x382D7365 },  // pi/6
	{ 0x3FE921FB, 0x54442D18 },  // pi/4
	{ 0x3FF0C152, 0x382D7365 },  // pi/3
	{ 0x3FF921FB, 0x54442D18 },  // pi/2
	{ 0x400921FB, 0x54442D18 },  // pi
	{ 0x4012D97C, 0x7F3321D2 },  // 3pi/2
	{ 0x401921FB, 0x54442D18 },  // 2pi
	{ 0x40763000, 0x00000000 },  // 355 ~113pi
	{ 0x412E8480, 0x00000000 },  // 1e6
	{ 0x4202A05F, 0x20000000 },  // 1e10
	{ 0x43300000, 0x00000000 }   // 2^52
};
static const Int s_trigCount = sizeof(s_trigWords) / sizeof(s_trigWords[0]);

// Sqrt: perfect squares (exact), irrational results, quarter.
static const UnsignedInt s_sqrtWords[][2] = {
	{ 0x3FD00000, 0x00000000 },  // 0.25
	{ 0x40000000, 0x00000000 },  // 2 irr
	{ 0x40080000, 0x00000000 },  // 3 irr
	{ 0x40100000, 0x00000000 },  // 4=2
	{ 0x40220000, 0x00000000 },  // 9=3
	{ 0x40300000, 0x00000000 },  // 16=4
	{ 0x40390000, 0x00000000 }   // 25=5
};
static const Int s_sqrtCount = sizeof(s_sqrtWords) / sizeof(s_sqrtWords[0]);

// Atan2 (y,x): quadrant axes, signed zeros, equal magnitude, tiny/huge ratios. Words {hy,ly,hx,lx}.
static const UnsignedInt s_atan2Pairs[][4] = {
	{ 0x3FF00000, 0x00000000, 0x3FF00000, 0x00000000 },  // (1,1) pi/4
	{ 0x3FF00000, 0x00000000, 0xBFF00000, 0x00000000 },  // (1,-1) 3pi/4
	{ 0xBFF00000, 0x00000000, 0x3FF00000, 0x00000000 },  // (-1,1) -pi/4
	{ 0xBFF00000, 0x00000000, 0xBFF00000, 0x00000000 },  // (-1,-1) -3pi/4
	{ 0x3FF00000, 0x00000000, 0x00000000, 0x00000000 },  // (1,0) pi/2
	{ 0x00000000, 0x00000000, 0x3FF00000, 0x00000000 },  // (0,1) 0
	{ 0xBFF00000, 0x00000000, 0x00000000, 0x00000000 },  // (-1,0) -pi/2
	{ 0x00000000, 0x00000000, 0xBFF00000, 0x00000000 },  // (0,-1) pi
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },  // (0,0)
	{ 0x00000000, 0x00000000, 0x80000000, 0x00000000 },  // (0,-0)
	{ 0x80000000, 0x00000000, 0x00000000, 0x00000000 },  // (-0,0)
	{ 0x01A56E1F, 0xC2F8F359, 0x7E37E43C, 0x8800759C },  // tiny/huge
	{ 0x7E37E43C, 0x8800759C, 0x01A56E1F, 0xC2F8F359 },  // huge/tiny
	{ 0x3FF00000, 0x00000000, 0x3FDC0000, 0x00000000 }   // ratio@break
};
static const Int s_atan2Count = sizeof(s_atan2Pairs) / sizeof(s_atan2Pairs[0]);

// Pow (base,exp): identities, powers of 2, overflow/underflow, error amplification. Words {hb,lb,he,le}.
static const UnsignedInt s_powPairs[][4] = {
	{ 0x40000000, 0x00000000, 0x3FE00000, 0x00000000 },  // 2^.5=sqrt2
	{ 0x40000000, 0x00000000, 0x40000000, 0x00000000 },  // 2^2
	{ 0x40000000, 0x00000000, 0x40240000, 0x00000000 },  // 2^10
	{ 0x40000000, 0x00000000, 0xBFF00000, 0x00000000 },  // 2^-1
	{ 0x40000000, 0x00000000, 0x404A8000, 0x00000000 },  // 2^53
	{ 0x40140000, 0x00000000, 0x00000000, 0x00000000 },  // x^0=1
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },  // 0^0
	{ 0x3FF00000, 0x00000000, 0x4202A05F, 0x20000000 },  // 1^y=1
	{ 0x40240000, 0x00000000, 0x40734000, 0x00000000 },  // 10^308 near-of
	{ 0x40240000, 0x00000000, 0xC0734000, 0x00000000 },  // 10^-308 near-uf
	{ 0x3FF00000, 0x1AD7F29B, 0x416312D0, 0x00000000 },  // amplify
	{ 0x3FE00000, 0x00000000, 0x3FE00000, 0x00000000 },  // 0.5^.5
	{ 0x40080000, 0x00000000, 0x40080000, 0x00000000 },  // 3^3
	{ 0x401C0000, 0x00000000, 0x3FF00000, 0x00000000 }   // x^1
};
static const Int s_powCount = sizeof(s_powPairs) / sizeof(s_powPairs[0]);

static double doubleFromWords(UnsignedInt hi, UnsignedInt lo)
{
	UnsignedInt words[2];
	words[0] = lo;
	words[1] = hi;
	double value;
	memcpy(&value, words, sizeof(value));
	return value;
}

static Bool isNanOrInf(double value)
{
	UnsignedInt words[2];
	memcpy(words, &value, sizeof(words));
	return ((words[1] >> 20) & 0x7FFu) == 0x7FFu;
}

// One representable double away from value (64-bit increment across two 32-bit words with carry),
// so a single input ULP step exercises the round-to-nearest boundary of each transcendental.
static double bitStep(double value, Int delta)
{
	if (delta == 0)
		return value;

	UnsignedInt words[2];
	memcpy(words, &value, sizeof(words));

	if (delta > 0)
	{
		if (++words[0] == 0)
			++words[1];
	}
	else
	{
		if (words[0] == 0)
			--words[1];
		--words[0];
	}

	double result;
	memcpy(&result, words, sizeof(result));
	return result;
}

// Set the x87 mantissa precision explicitly so a single run can measure both modes back to back.
// No-op on Mac (NEON has no precision-control word; doubles are always 53-bit).
static void setFpuMantissa(Int precisionBits)
{
#ifndef __APPLE__
	_fpreset();
	UnsignedInt current = _statusfp();
	UnsignedInt updated = current;
	updated = (updated & ~_MCW_RC) | (_RC_NEAR & _MCW_RC);
	UnsignedInt pc = (precisionBits == 24) ? _PC_24 : _PC_53;
	updated = (updated & ~_MCW_PC) | (pc & _MCW_PC);
	_controlfp(updated, _MCW_PC | _MCW_RC);
#endif
}

static float clampUnit(float v)
{
	if (v > 1.0f) return 1.0f;
	if (v < -1.0f) return -1.0f;
	return v;
}

// Fold the exact bits of a result into the CRC and, when logging, print one itemized line so a
// Win/Mac diff of the file localizes the exact function and input that diverged.
static void emitF(FILE *out, XferCRC &xfer, const char *label, float value)
{
	UnsignedInt bits;
	memcpy(&bits, &value, sizeof(bits));
	if (out != NULL)
		fprintf(out, "  %-26s = %08X\n", label, bits);
	xfer.xferUnsignedInt(&bits);
}

static void emitD(FILE *out, XferCRC &xfer, const char *label, double value)
{
	UnsignedInt words[2];
	memcpy(words, &value, sizeof(words));
	if (out != NULL)
		fprintf(out, "  %-26s = %08X%08X\n", label, words[1], words[0]);
	Int64 bits;
	memcpy(&bits, &value, sizeof(bits));
	xfer.xferInt64(&bits);
}

static void probeF1(FILE *out, XferCRC &xfer, const char *fn, float in, float result)
{
	char label[64];
	snprintf(label, sizeof(label), "%s(%.9g)", fn, (double)in);
	emitF(out, xfer, label, result);
}

static void probeF2(FILE *out, XferCRC &xfer, const char *fn, float a, float b, float result)
{
	char label[64];
	snprintf(label, sizeof(label), "%s(%.9g,%.9g)", fn, (double)a, (double)b);
	emitF(out, xfer, label, result);
}

static void probeD1(FILE *out, XferCRC &xfer, const char *fn, double in, double result)
{
	char label[64];
	snprintf(label, sizeof(label), "%s(%.17g)", fn, in);
	emitD(out, xfer, label, result);
}

static void probeD2(FILE *out, XferCRC &xfer, const char *fn, double a, double b, double result)
{
	char label[80];
	snprintf(label, sizeof(label), "%s(%.17g,%.17g)", fn, a, b);
	emitD(out, xfer, label, result);
}

enum MathDomain { DOMAIN_ANY, DOMAIN_POSITIVE };
typedef double (*DoubleFn1)(double);
typedef double (*DoubleFn2)(double, double);

static double guardDomain(double v, MathDomain domain)
{
	if (domain == DOMAIN_POSITIVE)
		return WWMath::Fabs(v) + DBL_MIN;
	return v;
}

// Probe a single-argument function over its boundary table, each value plus its two 1-ULP neighbors.
static void sweepFn1(FILE *out, XferCRC &xfer, const char *name, DoubleFn1 fn, MathDomain domain,
	const UnsignedInt table[][2], Int count)
{
	for (Int i = 0; i < count; ++i)
	{
		double base = doubleFromWords(table[i][0], table[i][1]);
		for (Int step = -1; step <= 1; ++step)
		{
			double v = bitStep(base, step);
			if (isNanOrInf(v))
				continue;
			v = guardDomain(v, domain);
			probeD1(out, xfer, name, v, fn(v));
		}
	}
}

// Probe a two-argument function over its (arg0,arg1) boundary pairs.
static void sweepFn2(FILE *out, XferCRC &xfer, const char *name, DoubleFn2 fn, MathDomain domain,
	const UnsignedInt table[][4], Int count)
{
	for (Int i = 0; i < count; ++i)
	{
		double a = guardDomain(doubleFromWords(table[i][0], table[i][1]), domain);
		double b = doubleFromWords(table[i][2], table[i][3]);
		probeD2(out, xfer, name, a, b, fn(a, b));
	}
}

static void sweepFloat(FILE *out, XferCRC &xfer)
{
	for (Int i = 0; i < s_edgeCount; ++i)
	{
		float v = s_edge[i];
		float c = clampUnit(v);
		probeF1(out, xfer, "Sinf", v, WWMath::Sinf(v));
		probeF1(out, xfer, "Cosf", v, WWMath::Cosf(v));
		probeF1(out, xfer, "Tanf", v, WWMath::Tanf(v));
		probeF1(out, xfer, "Atanf", v, WWMath::Atanf(v));
		probeF1(out, xfer, "Asinf", c, WWMath::Asinf(c));
		probeF1(out, xfer, "Acosf", c, WWMath::Acosf(c));
		probeF1(out, xfer, "Sqrtf", v, WWMath::Sqrtf(WWMath::Fabsf(v)));
		probeF1(out, xfer, "Expf", v, WWMath::Expf(v));
		probeF1(out, xfer, "Logf", v, WWMath::Logf(WWMath::Fabsf(v) + FLT_MIN));
		probeF1(out, xfer, "Log10f", v, WWMath::Log10f(WWMath::Fabsf(v) + FLT_MIN));
		probeF1(out, xfer, "Sinhf", v, WWMath::Sinhf(v));
		probeF1(out, xfer, "Coshf", v, WWMath::Coshf(v));
		probeF1(out, xfer, "Tanhf", v, WWMath::Tanhf(v));
	}
	for (Int i = 0; i < s_pairCount; ++i)
	{
		float y = (float)s_pairY[i];
		float x = (float)s_pairX[i];
		probeF2(out, xfer, "Atan2f", y, x, WWMath::Atan2f(y, x));
		probeF2(out, xfer, "Powf", WWMath::Fabsf(y) + FLT_MIN, x, WWMath::Powf(WWMath::Fabsf(y) + FLT_MIN, x));
	}
}

static void sweepDoubleDownCast(FILE *out, XferCRC &xfer)
{
	sweepFn1(out, xfer, "Atan", (DoubleFn1)&WWMath::Atan, DOMAIN_ANY, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Atan", (DoubleFn1)&WWMath::Atan, DOMAIN_ANY, s_atanWords, s_atanCount);
	sweepFn2(out, xfer, "Atan2", (DoubleFn2)&WWMath::Atan2, DOMAIN_ANY, s_atan2Pairs, s_atan2Count);
	for (Int i = 0; i < s_pairCount; ++i)
		probeD2(out, xfer, "Div_FixNaN", s_pairY[i], s_pairX[i], WWMath::Div_FixNaN(s_pairY[i], s_pairX[i]));
}

static void sweepDoubleNoDownCast(FILE *out, XferCRC &xfer)
{
	sweepFn1(out, xfer, "AtanNoDownCast", &WWMath::AtanNoDownCast, DOMAIN_ANY, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "AtanNoDownCast", &WWMath::AtanNoDownCast, DOMAIN_ANY, s_atanWords, s_atanCount);
	sweepFn2(out, xfer, "Atan2NoDownCast", &WWMath::Atan2NoDownCast, DOMAIN_ANY, s_atan2Pairs, s_atan2Count);
	for (Int i = 0; i < s_pairCount; ++i)
		probeD2(out, xfer, "Div_FixNaNNoDownCast", s_pairY[i], s_pairX[i], WWMath::Div_FixNaNNoDownCast(s_pairY[i], s_pairX[i]));
}

static void sweepDoubleNative(FILE *out, XferCRC &xfer)
{
	sweepFn1(out, xfer, "Sin", (DoubleFn1)&WWMath::Sin, DOMAIN_ANY, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Sin", (DoubleFn1)&WWMath::Sin, DOMAIN_ANY, s_trigWords, s_trigCount);
	sweepFn1(out, xfer, "Cos", (DoubleFn1)&WWMath::Cos, DOMAIN_ANY, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Cos", (DoubleFn1)&WWMath::Cos, DOMAIN_ANY, s_trigWords, s_trigCount);
	sweepFn1(out, xfer, "Sqrt", (DoubleFn1)&WWMath::Sqrt, DOMAIN_POSITIVE, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Sqrt", (DoubleFn1)&WWMath::Sqrt, DOMAIN_POSITIVE, s_sqrtWords, s_sqrtCount);
	sweepFn1(out, xfer, "Exp", (DoubleFn1)&WWMath::Exp, DOMAIN_ANY, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Exp", (DoubleFn1)&WWMath::Exp, DOMAIN_ANY, s_expWords, s_expCount);
	sweepFn1(out, xfer, "Log", (DoubleFn1)&WWMath::Log, DOMAIN_POSITIVE, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Log", (DoubleFn1)&WWMath::Log, DOMAIN_POSITIVE, s_logWords, s_logCount);
	sweepFn1(out, xfer, "Log10", (DoubleFn1)&WWMath::Log10, DOMAIN_POSITIVE, s_universalWords, s_universalCount);
	sweepFn1(out, xfer, "Log10", (DoubleFn1)&WWMath::Log10, DOMAIN_POSITIVE, s_log10Words, s_log10Count);
	sweepFn2(out, xfer, "Pow", (DoubleFn2)&WWMath::Pow, DOMAIN_POSITIVE, s_powPairs, s_powCount);
}

static void appendSimulationMathCrc_Deterministic(XferCRC &xfer)
{
	Matrix3D matrix;
	Matrix3D factorsMatrix;

	matrix.Set(
		4.1f, 1.2f, 0.3f, 0.4f,
		0.5f, 3.6f, 0.7f, 0.8f,
		0.9f, 1.0f, 2.1f, 1.2f);

	factorsMatrix.Set(
		WWMath::Sinf(0.7f) * WWMath::Log10f(2.3f),
		WWMath::Cosf(1.1f) * WWMath::Powf(1.1f, 2.0f),
		WWMath::Tanf(0.3f),
		WWMath::Asinf(0.967302263f),
		WWMath::Acosf(0.967302263f),
		WWMath::Atanf(0.967302263f) * WWMath::Powf(1.1f, 2.0f),
		WWMath::Atan2f(0.4f, 1.3f),
		WWMath::Sinhf(0.2f),
		WWMath::Coshf(0.4f) * WWMath::Tanhf(0.5f),
		WWMath::Sqrtf(55788.84375f),
		WWMath::Expf(0.1f) * WWMath::Log10f(2.3f),
		WWMath::Logf(1.4f));

	Matrix3D::Multiply(matrix, factorsMatrix, &matrix);
	matrix.Get_Inverse(matrix);

	xfer.xferMatrix3D(&matrix);
}

static void appendSimulationMathCrc_Native(XferCRC &xfer)
{
	Matrix3D matrix;
	Matrix3D factorsMatrix;

	matrix.Set(
		4.1f, 1.2f, 0.3f, 0.4f,
		0.5f, 3.6f, 0.7f, 0.8f,
		0.9f, 1.0f, 2.1f, 1.2f);

	factorsMatrix.Set(
		(float)(::sin(0.7) * ::log10(2.3)),
		(float)(::cos(1.1) * ::pow(1.1, 2.0)),
		(float)::tan(0.3),
		(float)::asin(0.967302263),
		(float)::acos(0.967302263),
		(float)(::atan(0.967302263) * ::pow(1.1, 2.0)),
		(float)::atan2(0.4, 1.3),
		(float)::sinh(0.2),
		(float)(::cosh(0.4) * ::tanh(0.5)),
		(float)::sqrt(55788.84375),
		(float)(::exp(0.1) * ::log10(2.3)),
		(float)::log(1.4));

	Matrix3D::Multiply(matrix, factorsMatrix, &matrix);
	matrix.Get_Inverse(matrix);

	xfer.xferMatrix3D(&matrix);
}

UnsignedInt SimulationMathCrc::calculate()
{
	XferCRC xfer;
	xfer.open("SimulationMathCrc");

	setFPMode();
	appendSimulationMathCrc_Deterministic(xfer);

#ifndef __APPLE__
	_fpreset();
#endif

	xfer.close();
	return xfer.getCRC();
}

UnsignedInt SimulationMathCrc::calculateDouble()
{
	XferCRC xfer;
	xfer.open("SimulationMathCrcDouble");

	setFPMode();
	sweepDoubleDownCast(NULL, xfer);

#ifndef __APPLE__
	_fpreset();
#endif

	xfer.close();
	return xfer.getCRC();
}

UnsignedInt SimulationMathCrc::calculateDoubleNoDownCast()
{
	XferCRC xfer;
	xfer.open("SimulationMathCrcDoubleNoDownCast");

	setFPMode();
	sweepDoubleNoDownCast(NULL, xfer);

#ifndef __APPLE__
	_fpreset();
#endif

	xfer.close();
	return xfer.getCRC();
}

// Run the three double batteries under one x87 precision mode; on Mac the mode is a no-op so both
// passes are identical (NEON is always 53-bit) and serve as the platform reference.
static void writeDoubleSections(FILE *out, Int mode, UnsignedInt *crcDown, UnsignedInt *crcNo, UnsignedInt *crcNat)
{
	setFpuMantissa(mode);

	XferCRC xfDown;
	xfDown.open("downcast");
	if (out != NULL) fprintf(out, "\n=== PC%d DOWNCAST (float-cast overloads) ===\n", (int)mode);
	sweepDoubleDownCast(out, xfDown);
	xfDown.close();
	*crcDown = xfDown.getCRC();

	XferCRC xfNo;
	xfNo.open("nodowncast");
	if (out != NULL) fprintf(out, "\n=== PC%d NoDownCast (true double) ===\n", (int)mode);
	sweepDoubleNoDownCast(out, xfNo);
	xfNo.close();
	*crcNo = xfNo.getCRC();

	XferCRC xfNat;
	xfNat.open("native");
	if (out != NULL) fprintf(out, "\n=== PC%d native (Sin/Cos/Sqrt/Exp/Log/Log10/Pow) ===\n", (int)mode);
	sweepDoubleNative(out, xfNat);
	xfNat.close();
	*crcNat = xfNat.getCRC();
}

void SimulationMathCrc::writeParityLog(const char *path)
{
	FILE *out = (path != NULL) ? fopen(path, "wt") : NULL;

	setFpuMantissa(53);
	XferCRC xfFloat;
	xfFloat.open("float");
	if (out != NULL) fprintf(out, "=== FLOAT (gm_*f, mode-independent) ===\n");
	sweepFloat(out, xfFloat);
	xfFloat.close();
	UnsignedInt crcFloat = xfFloat.getCRC();

	UnsignedInt crcDown[2], crcNo[2], crcNat[2];
	const Int modes[2] = { 24, 53 };
	for (Int m = 0; m < 2; ++m)
		writeDoubleSections(out, modes[m], &crcDown[m], &crcNo[m], &crcNat[m]);

	setFpuMantissa(53);
#ifndef __APPLE__
	_fpreset();
#endif

	printf("\n================ SIMULATION MATH PARITY ================\n");
	printf("Float          = %08X\n", crcFloat);
	printf("                    PC24       PC53\n");
	printf("DownCast       = %08X   %08X\n", crcDown[0], crcDown[1]);
	printf("NoDownCast     = %08X   %08X\n", crcNo[0], crcNo[1]);
	printf("Native         = %08X   %08X\n", crcNat[0], crcNat[1]);
	printf("=======================================================\n");
	fflush(stdout);

	if (out != NULL)
	{
		fprintf(out, "\n=== AGGREGATE CRC ===\n");
		fprintf(out, "Float          = %08X\n", crcFloat);
		fprintf(out, "                    PC24       PC53\n");
		fprintf(out, "DownCast       = %08X   %08X\n", crcDown[0], crcDown[1]);
		fprintf(out, "NoDownCast     = %08X   %08X\n", crcNo[0], crcNo[1]);
		fprintf(out, "Native         = %08X   %08X\n", crcNat[0], crcNat[1]);
		fclose(out);
	}
}

void SimulationMathCrc::runBenchmark(int iterations)
{
	int i;
	clock_t startDet = clock();
	UnsignedInt crcDet = 0;

	setFPMode();

	for (i = 0; i < iterations; ++i)
	{
		XferCRC xfer;
		xfer.open("SimMathDet");
		appendSimulationMathCrc_Deterministic(xfer);
		xfer.close();
		if (i == 0)
			crcDet = xfer.getCRC();
	}
#ifndef __APPLE__
	_fpreset();
#endif
	clock_t endDet = clock();
	double timeDetMs = (double)(endDet - startDet) / CLOCKS_PER_SEC * 1000.0;

	clock_t startNat = clock();
	UnsignedInt crcNat = 0;

	setFPMode();

	for (i = 0; i < iterations; ++i)
	{
		XferCRC xfer;
		xfer.open("SimMathNat");
		appendSimulationMathCrc_Native(xfer);
		xfer.close();
		if (i == 0)
			crcNat = xfer.getCRC();
	}
#ifndef __APPLE__
	_fpreset();
#endif
	clock_t endNat = clock();
	double timeNatMs = (double)(endNat - startNat) / CLOCKS_PER_SEC * 1000.0;

	printf("\n================ MATH BENCHMARK (%d iterations) ================\n", iterations);
	printf("Deterministic (WWMath): CRC = %08X, Time = %.2f ms\n", crcDet, timeDetMs);
	printf("Native (system math):   CRC = %08X, Time = %.2f ms\n", crcNat, timeNatMs);
	printf("===========================================================\n\n");
}
