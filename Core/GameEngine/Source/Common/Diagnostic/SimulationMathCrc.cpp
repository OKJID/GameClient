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
	for (Int i = 0; i < s_edgeCount; ++i)
	{
		double v = (double)s_edge[i];
		probeD1(out, xfer, "Atan", v, WWMath::Atan(v));
	}
	for (Int i = 0; i < s_pairCount; ++i)
	{
		probeD2(out, xfer, "Atan2", s_pairY[i], s_pairX[i], WWMath::Atan2(s_pairY[i], s_pairX[i]));
		probeD2(out, xfer, "Div_FixNaN", s_pairY[i], s_pairX[i], WWMath::Div_FixNaN(s_pairY[i], s_pairX[i]));
	}
}

static void sweepDoubleNoDownCast(FILE *out, XferCRC &xfer)
{
	for (Int i = 0; i < s_edgeCount; ++i)
	{
		double v = (double)s_edge[i];
		probeD1(out, xfer, "AtanNoDownCast", v, WWMath::AtanNoDownCast(v));
	}
	for (Int i = 0; i < s_pairCount; ++i)
	{
		probeD2(out, xfer, "Atan2NoDownCast", s_pairY[i], s_pairX[i], WWMath::Atan2NoDownCast(s_pairY[i], s_pairX[i]));
		probeD2(out, xfer, "Div_FixNaNNoDownCast", s_pairY[i], s_pairX[i], WWMath::Div_FixNaNNoDownCast(s_pairY[i], s_pairX[i]));
	}
}

static void sweepDoubleNative(FILE *out, XferCRC &xfer)
{
	for (Int i = 0; i < s_edgeCount; ++i)
	{
		double v = (double)s_edge[i];
		probeD1(out, xfer, "Sin", v, WWMath::Sin(v));
		probeD1(out, xfer, "Cos", v, WWMath::Cos(v));
		probeD1(out, xfer, "Sqrt", v, WWMath::Sqrt(WWMath::Fabs(v)));
		probeD1(out, xfer, "Exp", v, WWMath::Exp(v));
		probeD1(out, xfer, "Log", v, WWMath::Log(WWMath::Fabs(v) + DBL_MIN));
	}
	for (Int i = 0; i < s_pairCount; ++i)
	{
		double base = WWMath::Fabs(s_pairY[i]) + DBL_MIN;
		probeD2(out, xfer, "Pow", base, s_pairX[i], WWMath::Pow(base, s_pairX[i]));
	}
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

void SimulationMathCrc::writeParityLog(const char *path)
{
	FILE *out = (path != NULL) ? fopen(path, "wt") : NULL;

	setFPMode();

	XferCRC xfFloat;
	xfFloat.open("float");
	if (out != NULL) fprintf(out, "=== FLOAT (gm_*f, expected bit-identical) ===\n");
	sweepFloat(out, xfFloat);
	xfFloat.close();

	XferCRC xfDown;
	xfDown.open("downcast");
	if (out != NULL) fprintf(out, "\n=== DOUBLE DOWNCAST (current WWMath double overloads) ===\n");
	sweepDoubleDownCast(out, xfDown);
	xfDown.close();

	XferCRC xfNo;
	xfNo.open("nodowncast");
	if (out != NULL) fprintf(out, "\n=== DOUBLE NoDownCast (true gm_ double twins) ===\n");
	sweepDoubleNoDownCast(out, xfNo);
	xfNo.close();

	XferCRC xfNat;
	xfNat.open("native");
	if (out != NULL) fprintf(out, "\n=== DOUBLE native-deterministic (Sin/Cos/Sqrt/Exp/Log/Pow) ===\n");
	sweepDoubleNative(out, xfNat);
	xfNat.close();

#ifndef __APPLE__
	_fpreset();
#endif

	UnsignedInt crcFloat = xfFloat.getCRC();
	UnsignedInt crcDown = xfDown.getCRC();
	UnsignedInt crcNo = xfNo.getCRC();
	UnsignedInt crcNat = xfNat.getCRC();

	printf("\n================ SIMULATION MATH PARITY ================\n");
	printf("SimulationMathCrcFloat            = %08X\n", crcFloat);
	printf("SimulationMathCrcDouble           = %08X\n", crcDown);
	printf("SimulationMathCrcDoubleNoDownCast = %08X\n", crcNo);
	printf("SimulationMathCrcDoubleNative     = %08X\n", crcNat);
	printf("=======================================================\n");
	fflush(stdout);

	if (out != NULL)
	{
		fprintf(out, "\n=== AGGREGATE CRC ===\n");
		fprintf(out, "SimulationMathCrcFloat            = %08X\n", crcFloat);
		fprintf(out, "SimulationMathCrcDouble           = %08X\n", crcDown);
		fprintf(out, "SimulationMathCrcDoubleNoDownCast = %08X\n", crcNo);
		fprintf(out, "SimulationMathCrcDoubleNative     = %08X\n", crcNat);
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
