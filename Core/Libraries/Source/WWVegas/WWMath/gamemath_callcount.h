/*
**	Counts how often each GameMath function is called during play.
**
**	The per call cost of GameMath is known from a microbenchmark, but that only
**	answers half the question. This gives the other half: how many times a frame
**	each gm_ function is reached, so the two can be multiplied into a real cost.
**
**	The counters key on the gm_ function, not on the WWMath wrapper that reaches
**	it. WWMath::Fabsf and WWMath::Fabsf_Legacy both land on gm_fabsf and both
**	feed the same counter. Wrappers that never reach GameMath are not counted.
**
**	This is a measuring branch. Every GameMath call gains a memory increment,
**	which is cheap but not free, and the counters are not thread safe.
*/

#pragma once

/* How many frames each reported block covers. */
#define WWMATH_COUNT_DUMP_INTERVAL (100)

enum GameMathCallId
{
	GM_CALL_acos,
	GM_CALL_acosf,
	GM_CALL_asin,
	GM_CALL_asinf,
	GM_CALL_atanf,
	GM_CALL_atan2f,
	GM_CALL_ceil,
	GM_CALL_ceilf,
	GM_CALL_cos,
	GM_CALL_cosf,
	GM_CALL_cosh,
	GM_CALL_coshf,
	GM_CALL_exp,
	GM_CALL_expf,
	GM_CALL_fabs,
	GM_CALL_fabsf,
	GM_CALL_floor,
	GM_CALL_floorf,
	GM_CALL_log,
	GM_CALL_logf,
	GM_CALL_log10,
	GM_CALL_log10f,
	GM_CALL_lrint,
	GM_CALL_lrintf,
	GM_CALL_powf,
	GM_CALL_sin,
	GM_CALL_sinf,
	GM_CALL_sinh,
	GM_CALL_sinhf,
	GM_CALL_sqrt,
	GM_CALL_sqrtf,
	GM_CALL_tan,
	GM_CALL_tanf,
	GM_CALL_tanh,
	GM_CALL_tanhf,
	GM_CALL_COUNT
};

extern unsigned long long GameMathCallCounts[GM_CALL_COUNT];

/* Writes the counts to GameMathCallsDiag.txt and starts over. */
void GameMathDumpCallCounts(int frame);

/* Drops whatever has been counted so far without reporting it. */
void GameMathResetCallCounts(void);
