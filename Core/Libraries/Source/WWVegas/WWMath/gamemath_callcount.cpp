/*
**	Storage and reporting for the GameMath call counters. See gamemath_callcount.h.
*/

#include "gamemath_callcount.h"

#include <stdio.h>

unsigned long long GameMathCallCounts[GM_CALL_COUNT];

static const char *GameMathCallNames[GM_CALL_COUNT] =
{
	"gm_acos",
	"gm_acosf",
	"gm_asin",
	"gm_asinf",
	"gm_atanf",
	"gm_atan2f",
	"gm_ceil",
	"gm_ceilf",
	"gm_cos",
	"gm_cosf",
	"gm_cosh",
	"gm_coshf",
	"gm_exp",
	"gm_expf",
	"gm_fabs",
	"gm_fabsf",
	"gm_floor",
	"gm_floorf",
	"gm_log",
	"gm_logf",
	"gm_log10",
	"gm_log10f",
	"gm_lrint",
	"gm_lrintf",
	"gm_powf",
	"gm_sin",
	"gm_sinf",
	"gm_sinh",
	"gm_sinhf",
	"gm_sqrt",
	"gm_sqrtf",
	"gm_tan",
	"gm_tanf",
	"gm_tanh",
	"gm_tanhf"
};

/* Relative path on purpose: on macOS this lands in the install directory, and
** the Diag suffix is what the log collectors pick up on both platforms. */
static FILE *GameMathCallFile = NULL;
static bool GameMathCallFileTried = false;

void GameMathResetCallCounts(void)
{
	int i;

	for (i = 0; i < GM_CALL_COUNT; ++i) {
		GameMathCallCounts[i] = 0;
	}
}

void GameMathDumpCallCounts(int frame)
{
	int i;
	unsigned long long total = 0;

	if (!GameMathCallFileTried) {
		GameMathCallFileTried = true;
		GameMathCallFile = fopen("GameMathCallsDiag.txt", "w");
	}

	if (GameMathCallFile == NULL) {
		return;
	}

	for (i = 0; i < GM_CALL_COUNT; ++i) {
		total += GameMathCallCounts[i];
	}

	fprintf(GameMathCallFile, "==== frame %d, %d frames, %llu calls ====\n",
		frame, WWMATH_COUNT_DUMP_INTERVAL, total);

	for (i = 0; i < GM_CALL_COUNT; ++i) {
		if (GameMathCallCounts[i] != 0) {
			fprintf(GameMathCallFile, "%-10s %12llu\n",
				GameMathCallNames[i], GameMathCallCounts[i]);
		}
	}

	fprintf(GameMathCallFile, "\n");
	fflush(GameMathCallFile);

	GameMathResetCallCounts();
}
