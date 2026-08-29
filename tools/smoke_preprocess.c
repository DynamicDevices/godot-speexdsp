/**
 * Host smoke: Speex preprocessor VAD/AGC init + one frame.
 */
#include "speex/speex_preprocess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	const int frame = 160;
	const int rate = 16000;
	SpeexPreprocessState *st = speex_preprocess_state_init(frame, rate);
	if (!st) {
		fprintf(stderr, "init failed\n");
		return 1;
	}
	int on = 1;
	float lvl = 8000.f;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_VAD, &on);
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &on);
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC_LEVEL, &lvl);
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &on);

	spx_int16_t *buf = (spx_int16_t *)calloc((size_t)frame, sizeof(spx_int16_t));
	if (!buf) {
		speex_preprocess_state_destroy(st);
		return 1;
	}
	for (int i = 0; i < frame; i++) {
		buf[i] = (spx_int16_t)(i * 40);
	}
	int vad = speex_preprocess_run(st, buf);
	speex_preprocess_state_destroy(st);
	free(buf);
	printf("SPEEX_PREPROCESS_OK vad=%d\n", vad);
	return 0;
}
