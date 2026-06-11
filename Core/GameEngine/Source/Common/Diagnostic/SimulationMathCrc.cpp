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
#include <time.h>

static inline unsigned int f2h(float f) {
    union { float f; unsigned int i; } u;
    u.f = f;
    return u.i;
}

static void dumpMathDiagnostic(const char* filename)
{
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "================ MATH DEBUG DIAGNOSTIC ================\n");

    setFPMode();

    fprintf(f, "0. BUILD ENVIRONMENT\n");
    fprintf(f, "---------------------------------------------------------\n");
#ifdef HAS_GAMEMATH
    fprintf(f, "HAS_GAMEMATH:            %d\n", HAS_GAMEMATH);
#else
    fprintf(f, "HAS_GAMEMATH:            UNDEFINED\n");
#endif
#ifdef USE_DETERMINISTIC_MATH
    fprintf(f, "USE_DETERMINISTIC_MATH:  %d\n", USE_DETERMINISTIC_MATH);
#else
    fprintf(f, "USE_DETERMINISTIC_MATH:  UNDEFINED (native math fallback!)\n");
#endif
#ifdef RETAIL_COMPATIBLE_CRC
    fprintf(f, "RETAIL_COMPATIBLE_CRC:   %d\n", RETAIL_COMPATIBLE_CRC);
#else
    fprintf(f, "RETAIL_COMPATIBLE_CRC:   UNDEFINED\n");
#endif
#if defined(__clang__)
    fprintf(f, "COMPILER:                Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(_MSC_VER)
    fprintf(f, "COMPILER:                MSVC %d\n", _MSC_VER);
#elif defined(__GNUC__)
    fprintf(f, "COMPILER:                GCC %d.%d\n", __GNUC__, __GNUC_MINOR__);
#else
    fprintf(f, "COMPILER:                Unknown\n");
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
    fprintf(f, "ARCH:                    ARM64\n");
#elif defined(__x86_64__) || defined(_M_X64)
    fprintf(f, "ARCH:                    x86_64\n");
#elif defined(__i386__) || defined(_M_IX86)
    fprintf(f, "ARCH:                    x86 (32-bit)\n");
#else
    fprintf(f, "ARCH:                    Unknown\n");
#endif
    fprintf(f, "sizeof(Real):            %zu\n", sizeof(Real));
    fprintf(f, "sizeof(void*):           %zu\n", sizeof(void*));
    fprintf(f, "\n");

    fprintf(f, "1. BASIC WWMath vs NATIVE TRANSCENDENTALS\n");
    fprintf(f, "---------------------------------------------------------\n");
    
    // Evaluate the exact same expressions as SimulationMathCrc
    float s1_w = WWMath::Sinf(0.7f);
    float l1_w = WWMath::Log10f(2.3f);
    float p1_w = s1_w * l1_w;

    float s1_n = (float)::sin(0.7);
    float l1_n = (float)::log10(2.3);
    float p1_n = (float)(::sin(0.7) * ::log10(2.3));

    fprintf(f, "Sinf(0.7f):        WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(s1_w), s1_w, f2h(s1_n), s1_n);
    fprintf(f, "Log10f(2.3f):      WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(l1_w), l1_w, f2h(l1_n), l1_n);
    fprintf(f, "Sin * Log10:       WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(p1_w), p1_w, f2h(p1_n), p1_n);
    
    float c2_w = WWMath::Cosf(1.1f);
    float po2_w = WWMath::Powf(1.1f, 2.0f);
    float p2_w = c2_w * po2_w;

    float c2_n = (float)::cos(1.1);
    float po2_n = (float)::pow(1.1, 2.0);
    float p2_n = (float)(::cos(1.1) * ::pow(1.1, 2.0));

    fprintf(f, "Cosf(1.1f):        WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(c2_w), c2_w, f2h(c2_n), c2_n);
    fprintf(f, "Powf(1.1f, 2.0f):  WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(po2_w), po2_w, f2h(po2_n), po2_n);
    fprintf(f, "Cos * Pow:         WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(p2_w), p2_w, f2h(p2_n), p2_n);

    float t3_w = WWMath::Tanf(0.3f);
    float t3_n = (float)::tan(0.3);
    fprintf(f, "Tanf(0.3f):        WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(t3_w), t3_w, f2h(t3_n), t3_n);

    float as4_w = WWMath::Asinf(0.967302263f);
    float as4_n = (float)::asin(0.967302263);
    fprintf(f, "Asinf(0.9673...):  WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(as4_w), as4_w, f2h(as4_n), as4_n);

    float ac5_w = WWMath::Acosf(0.967302263f);
    float ac5_n = (float)::acos(0.967302263);
    fprintf(f, "Acosf(0.9673...):  WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(ac5_w), ac5_w, f2h(ac5_n), ac5_n);

    float at6_w = WWMath::Atanf(0.967302263f);
    float p6_w = WWMath::Powf(1.1f, 2.0f);
    float m6_w = at6_w * p6_w;
    
    float at6_n = (float)::atan(0.967302263);
    float p6_n = (float)::pow(1.1, 2.0);
    float m6_n = at6_n * p6_n;
    
    fprintf(f, "Atanf(0.9673...):  WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(at6_w), at6_w, f2h(at6_n), at6_n);
    fprintf(f, "Atan * Pow:        WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(m6_w), m6_w, f2h(m6_n), m6_n);

    float at27_w = WWMath::Atan2f(0.4f, 1.3f);
    float at27_n = (float)::atan2(0.4, 1.3);
    fprintf(f, "Atan2f(0.4, 1.3):  WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(at27_w), at27_w, f2h(at27_n), at27_n);

    float sh8_w = WWMath::Sinhf(0.2f);
    float sh8_n = (float)::sinh(0.2);
    fprintf(f, "Sinhf(0.2f):       WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(sh8_w), sh8_w, f2h(sh8_n), sh8_n);

    float ch9_w = WWMath::Coshf(0.4f);
    float th9_w = WWMath::Tanhf(0.5f);
    float m9_w = ch9_w * th9_w;

    float ch9_n = (float)::cosh(0.4);
    float th9_n = (float)::tanh(0.5);
    float m9_n = (float)(::cosh(0.4) * ::tanh(0.5));

    fprintf(f, "Coshf(0.4f):       WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(ch9_w), ch9_w, f2h(ch9_n), ch9_n);
    fprintf(f, "Tanhf(0.5f):       WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(th9_w), th9_w, f2h(th9_n), th9_n);
    fprintf(f, "Cosh * Tanh:       WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(m9_w), m9_w, f2h(m9_n), m9_n);

    float sq10_w = WWMath::Sqrtf(55788.84375f);
    float sq10_n = (float)::sqrt(55788.84375);
    fprintf(f, "Sqrtf(55788.8):    WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(sq10_w), sq10_w, f2h(sq10_n), sq10_n);

    float ex11_w = WWMath::Expf(0.1f);
    float m11_w = ex11_w * l1_w; // Log10f(2.3) calculated above

    float ex11_n = (float)::exp(0.1);
    float m11_n = (float)(::exp(0.1) * ::log10(2.3));
    fprintf(f, "Expf(0.1f):        WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(ex11_w), ex11_w, f2h(ex11_n), ex11_n);
    fprintf(f, "Exp * Log10:       WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(m11_w), m11_w, f2h(m11_n), m11_n);

    float lg12_w = WWMath::Logf(1.4f);
    float lg12_n = (float)::log(1.4);
    fprintf(f, "Logf(1.4f):        WWMath=%08X (%f)  Native=%08X (%f)\n", f2h(lg12_w), lg12_w, f2h(lg12_n), lg12_n);

    fprintf(f, "\n1a. ATAN2F CROSS-PLATFORM (real game values from LocoDiag)\n");
    fprintf(f, "---------------------------------------------------------\n");
    {
        union { float f; unsigned int i; } upx, upy, ugx, ugy;
        upx.i = 0x44F33AF3; upy.i = 0x4433C913;
        ugx.i = 0x44E628C2; ugy.i = 0x443C1F3A;
        float dy = ugy.f - upy.f;
        float dx = ugx.f - upx.f;
        float a2_w = WWMath::Atan2f(dy, dx);
        float a2_n = (float)::atan2(dy, dx);
        fprintf(f, "Case1 dy=%08X dx=%08X  WWMath=%08X  Native=%08X\n",
            f2h(dy), f2h(dx), f2h(a2_w), f2h(a2_n));

        upx.i = 0x44CD5B1B; upy.i = 0x44D3E48E;
        ugx.i = 0x44E628C2; ugy.i = 0x443C1F3A;
        dy = ugy.f - upy.f;
        dx = ugx.f - upx.f;
        a2_w = WWMath::Atan2f(dy, dx);
        a2_n = (float)::atan2(dy, dx);
        fprintf(f, "Case2 dy=%08X dx=%08X  WWMath=%08X  Native=%08X\n",
            f2h(dy), f2h(dx), f2h(a2_w), f2h(a2_n));

        float test_angles[] = { 0.1f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, -0.3f, -1.2f, -2.7f };
        for (int i = 0; i < 10; i++) {
            float y_v = WWMath::Sinf(test_angles[i]) * 100.0f;
            float x_v = WWMath::Cosf(test_angles[i]) * 100.0f;
            float r_w = WWMath::Atan2f(y_v, x_v);
            fprintf(f, "Sweep[%d] y=%08X x=%08X  Atan2=%08X\n", i, f2h(y_v), f2h(x_v), f2h(r_w));
        }

        float atanf_inputs[] = { 0.1f, 0.5f, 0.999f, 1.5f, 3.0f, 10.0f, 100.0f };
        for (int i = 0; i < 7; i++) {
            float r_w = WWMath::Atanf(atanf_inputs[i]);
            fprintf(f, "Atanf[%d] in=%08X  out=%08X\n", i, f2h(atanf_inputs[i]), f2h(r_w));
        }

        fprintf(f, "\n1a2. ATAN2 FLOAT vs DOUBLE (Locomotor calls Atan2 double overload!)\n");
        fprintf(f, "---------------------------------------------------------\n");
        {
            union { float f; unsigned int i; } udy, udx;

            udy.i = 0x446C04C6; udx.i = 0xC3A4DDB0;
            float r1f = WWMath::Atan2f(udy.f, udx.f);
            float r1d = (float)WWMath::Atan2((double)udy.f, (double)udx.f);
            fprintf(f, "o312 Atan2f=%08X  Atan2_dbl=%08X  %s  (game_Mac=3FF4128C game_Win=3FF4128B)\n",
                f2h(r1f), f2h(r1d), r1f == r1d ? "SAME" : "DIFFER");

            udy.i = 0x42056270; udx.i = 0xC2D12310;
            float r2f = WWMath::Atan2f(udy.f, udx.f);
            float r2d = (float)WWMath::Atan2((double)udy.f, (double)udx.f);
            fprintf(f, "o326 Atan2f=%08X  Atan2_dbl=%08X  %s  (game_Mac=40354E24 game_Win=40354E25)\n",
                f2h(r2f), f2h(r2d), r2f == r2d ? "SAME" : "DIFFER");

            float test_dy[] = {944.0746f, 33.3461f, -100.5f, 0.001f, 500.0f};
            float test_dx[] = {-329.732f, -104.568f, 200.3f, -0.001f, -500.0f};
            for (int i = 0; i < 5; i++) {
                float rf = WWMath::Atan2f(test_dy[i], test_dx[i]);
                float rd = (float)WWMath::Atan2((double)test_dy[i], (double)test_dx[i]);
                fprintf(f, "Pair[%d] dy=%08X dx=%08X  f=%08X d=%08X  %s\n",
                    i, f2h(test_dy[i]), f2h(test_dx[i]), f2h(rf), f2h(rd),
                    rf == rd ? "SAME" : "DIFFER");
            }
        }
    }

    fprintf(f, "\n1b. EDGE CASE (ASIN AND DIVISION PRECISION)\n");
    fprintf(f, "---------------------------------------------------------\n");
    
    struct MockCoord3D {
        float x, y, z;
        inline float length_w() const { return WWMath::Sqrtf(x*x + y*y + z*z); }
        inline float length_n() const { return (float)::sqrt(x*x + y*y + z*z); }
    };

    float vx[] = { 0.0f, 0.0001f, 0.00001f, 0.03f, 0.015f };
    float vy[] = { 0.0f, 0.0001f, 0.00001f, 0.04f, 0.020f };
    float vz[] = { 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
    
    for (int idx = 0; idx < 5; ++idx) {
        MockCoord3D v = { vx[idx], vy[idx], vz[idx] };
        
        // 1:1 In-place evaluation exactly as in game code (no intermediate variables)
        float pitch_w = WWMath::Asinf( v.z / v.length_w() );
        float pitch_n = (float)::asin( v.z / v.length_n() );
        
        fprintf(f, "Vec[%d] (%f, %f, %f):\n", idx, v.x, v.y, v.z);
        fprintf(f, "  WWMath in-place ASin(v.z/v.length()):  %08X (%f)\n", f2h(pitch_w), pitch_w);
        fprintf(f, "  Native in-place ASin(v.z/v.length()):  %08X (%f)\n", f2h(pitch_n), pitch_n);
    }

    fprintf(f, "\n2. MATRIX OPERATIONS (WWMath)\n");
    fprintf(f, "---------------------------------------------------------\n");
    
    Matrix3D m1, m2, res;
    m1.Set(4.1f, 1.2f, 0.3f, 0.4f, 0.5f, 3.6f, 0.7f, 0.8f, 0.9f, 1.0f, 2.1f, 1.2f);
    m2.Set(p1_w, p2_w, t3_w, as4_w, ac5_w, m6_w, at27_w, sh8_w, m9_w, sq10_w, m11_w, lg12_w);
    
    Matrix3D::Multiply(m1, m2, &res);
    
    fprintf(f, "Matrix Multiplication Result (WWMath):\n");
    fprintf(f, "Row0: %08X %08X %08X %08X\n", f2h(res[0][0]), f2h(res[0][1]), f2h(res[0][2]), f2h(res[0][3]));
    fprintf(f, "Row1: %08X %08X %08X %08X\n", f2h(res[1][0]), f2h(res[1][1]), f2h(res[1][2]), f2h(res[1][3]));
    fprintf(f, "Row2: %08X %08X %08X %08X\n", f2h(res[2][0]), f2h(res[2][1]), f2h(res[2][2]), f2h(res[2][3]));

    res.Get_Inverse(res);
    
    fprintf(f, "\nMatrix Inverse Result (WWMath):\n");
    fprintf(f, "Row0: %08X %08X %08X %08X\n", f2h(res[0][0]), f2h(res[0][1]), f2h(res[0][2]), f2h(res[0][3]));
    fprintf(f, "Row1: %08X %08X %08X %08X\n", f2h(res[1][0]), f2h(res[1][1]), f2h(res[1][2]), f2h(res[1][3]));
    fprintf(f, "Row2: %08X %08X %08X %08X\n", f2h(res[2][0]), f2h(res[2][1]), f2h(res[2][2]), f2h(res[2][3]));

    fprintf(f, "\n2. MATRIX OPERATIONS (Native)\n");
    fprintf(f, "---------------------------------------------------------\n");
    
    Matrix3D m2_n, res_n;
    m2_n.Set(p1_n, p2_n, t3_n, as4_n, ac5_n, m6_n, at27_n, sh8_n, m9_n, sq10_n, m11_n, lg12_n);
    
    Matrix3D::Multiply(m1, m2_n, &res_n);
    
    fprintf(f, "Matrix Multiplication Result (Native):\n");
    fprintf(f, "Row0: %08X %08X %08X %08X\n", f2h(res_n[0][0]), f2h(res_n[0][1]), f2h(res_n[0][2]), f2h(res_n[0][3]));
    fprintf(f, "Row1: %08X %08X %08X %08X\n", f2h(res_n[1][0]), f2h(res_n[1][1]), f2h(res_n[1][2]), f2h(res_n[1][3]));
    fprintf(f, "Row2: %08X %08X %08X %08X\n", f2h(res_n[2][0]), f2h(res_n[2][1]), f2h(res_n[2][2]), f2h(res_n[2][3]));

    res_n.Get_Inverse(res_n);
    
    fprintf(f, "\nMatrix Inverse Result (Native):\n");
    fprintf(f, "Row0: %08X %08X %08X %08X\n", f2h(res_n[0][0]), f2h(res_n[0][1]), f2h(res_n[0][2]), f2h(res_n[0][3]));
    fprintf(f, "Row1: %08X %08X %08X %08X\n", f2h(res_n[1][0]), f2h(res_n[1][1]), f2h(res_n[1][2]), f2h(res_n[1][3]));
    fprintf(f, "Row2: %08X %08X %08X %08X\n", f2h(res_n[2][0]), f2h(res_n[2][1]), f2h(res_n[2][2]), f2h(res_n[2][3]));

    fclose(f);
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
    
    // EDGE CASES: Real-world situations that can break determinism due to FPU precision (x87 vs SSE)
    float z1 = 1.0f;
    float len1 = WWMath::Sqrtf(z1 * z1);
    float ratio1 = z1 / len1; // Can be slightly > 1.0f on x87 due to intermediate precision
    
    float clampedRatio1 = ratio1;
    if (clampedRatio1 > 1.0f) clampedRatio1 = 1.0f;
    if (clampedRatio1 < -1.0f) clampedRatio1 = -1.0f;
    
    Matrix3D edgeCasesMatrix;
    edgeCasesMatrix.Set(
        WWMath::Asinf(ratio1), // Unclamped ASin that might receive > 1.0f
        WWMath::Acosf(-ratio1), // Unclamped ACos that might receive < -1.0f
        WWMath::Asinf(clampedRatio1), // Clamped ASin (Safe)
        WWMath::Acosf(-clampedRatio1), // Clamped ACos (Safe)
        WWMath::Atan2f(0.0f, 0.0f), // ATan2(0,0) edge case
        WWMath::Sqrtf(-0.0f), // Negative zero sqrt
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
    );
    Matrix3D::Multiply(matrix, edgeCasesMatrix, &matrix);

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
    
    // EDGE CASES: Real-world situations that can break determinism due to FPU precision (x87 vs SSE)
    float z1 = 1.0f;
    float len1 = (float)::sqrt(z1 * z1);
    float ratio1 = z1 / len1; // Can be slightly > 1.0f on x87 due to intermediate precision
    
    float clampedRatio1 = ratio1;
    if (clampedRatio1 > 1.0f) clampedRatio1 = 1.0f;
    if (clampedRatio1 < -1.0f) clampedRatio1 = -1.0f;
    
    Matrix3D edgeCasesMatrix;
    edgeCasesMatrix.Set(
        (float)::asin(ratio1), // Unclamped ASin that might receive > 1.0f
        (float)::acos(-ratio1), // Unclamped ACos that might receive < -1.0f
        (float)::asin(clampedRatio1), // Clamped ASin (Safe)
        (float)::acos(-clampedRatio1), // Clamped ACos (Safe)
        (float)::atan2(0.0, 0.0), // ATan2(0,0) edge case
        (float)::sqrt(-0.0), // Negative zero sqrt
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
    );
    Matrix3D::Multiply(matrix, edgeCasesMatrix, &matrix);

    matrix.Get_Inverse(matrix);

    xfer.xferMatrix3D(&matrix);
}

UnsignedInt SimulationMathCrc::calculate()
{
    XferCRC xfer;
    xfer.open("SimulationMathCrc");

    setFPMode();

    appendSimulationMathCrc_Deterministic(xfer);

#ifdef _WIN32
    _fpreset();
#endif

    xfer.close();

    return xfer.getCRC();
}

void SimulationMathCrc::runBenchmark(int iterations)
{
    int i;
    clock_t startDet = clock();
    UnsignedInt crcDet = 0;
    
    // Create detailed debug log of math precision differences
    dumpMathDiagnostic("MathPrecisionDiag.txt");

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
