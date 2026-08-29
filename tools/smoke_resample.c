/**
 * Host smoke (no Godot): SpeexDSP resampler 48k→16k mono.
 *   cc -std=c99 -O2 -DOUTSIDE_SPEEX -DRANDOM_PREFIX=godot_speexdsp -DFLOATING_POINT -DEXPORT= \
 *     -I../thirdparty/speexdsp/include/speex -I../thirdparty/speexdsp/libspeexdsp \
 *     -o smoke_resample smoke_resample.c ../thirdparty/speexdsp/libspeexdsp/resample.c -lm
 */
#include "speex_resampler.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void)
{
	const int in_rate = 48000;
	const int out_rate = 16000;
	const int n_in = 480; /* 10 ms */
	float *in = (float *)calloc((size_t)n_in, sizeof(float));
	if (!in) {
		return 1;
	}
	for (int i = 0; i < n_in; i++) {
		in[i] = 0.25f * sinf(2.f * (float)M_PI * 440.f * (float)i / (float)in_rate);
	}

	int err = 0;
	SpeexResamplerState *st =
			speex_resampler_init(1, (spx_uint32_t)in_rate, (spx_uint32_t)out_rate, 5, &err);
	if (!st || err != RESAMPLER_ERR_SUCCESS) {
		fprintf(stderr, "init failed: %s\n", speex_resampler_strerror(err));
		free(in);
		return 1;
	}

	spx_uint32_t in_len = (spx_uint32_t)n_in;
	spx_uint32_t out_len = (spx_uint32_t)((n_in * out_rate) / in_rate + 32);
	float *out = (float *)calloc(out_len, sizeof(float));
	if (!out) {
		speex_resampler_destroy(st);
		free(in);
		return 1;
	}
	err = speex_resampler_process_float(st, 0, in, &in_len, out, &out_len);
	speex_resampler_destroy(st);
	free(in);
	if (err != RESAMPLER_ERR_SUCCESS || out_len < 100) {
		fprintf(stderr, "process failed err=%d out_len=%u\n", err, out_len);
		free(out);
		return 1;
	}
	float peak = 0.f;
	for (spx_uint32_t i = 0; i < out_len; i++) {
		float a = fabsf(out[i]);
		if (a > peak) {
			peak = a;
		}
	}
	free(out);
	printf("SPEEX_RESAMPLE_OK in=%d out=%u peak=%.4f\n", n_in, out_len, peak);
	return 0;
}
