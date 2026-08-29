#include "SpeexResampler.hpp"
#include "speex_stereo_util.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cmath>
#include <cstring>
#include <vector>

/* SpeexDSP resampler — compiled OUTSIDE_SPEEX with RANDOM_PREFIX (see SConstruct). */
#include "speex_resampler.h"

using namespace godot;

void SpeexResampler::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("setup", "channels", "in_rate", "out_rate", "quality"),
			&SpeexResampler::setup, DEFVAL(5));
	ClassDB::bind_method(D_METHOD("set_rate", "in_rate", "out_rate"), &SpeexResampler::set_rate);
	ClassDB::bind_method(D_METHOD("process", "input"), &SpeexResampler::process);
	ClassDB::bind_method(D_METHOD("process2", "input", "mono_mix"), &SpeexResampler::process2,
			DEFVAL(-1.f));
	ClassDB::bind_method(D_METHOD("reset"), &SpeexResampler::reset);
	ClassDB::bind_method(D_METHOD("get_channels"), &SpeexResampler::get_channels);
	ClassDB::bind_method(D_METHOD("get_in_rate"), &SpeexResampler::get_in_rate);
	ClassDB::bind_method(D_METHOD("get_out_rate"), &SpeexResampler::get_out_rate);
	ClassDB::bind_method(D_METHOD("get_quality"), &SpeexResampler::get_quality);
	ClassDB::bind_method(D_METHOD("get_input_latency"), &SpeexResampler::get_input_latency);
	ClassDB::bind_method(D_METHOD("get_output_latency"), &SpeexResampler::get_output_latency);
}

SpeexResampler::~SpeexResampler()
{
	if (st) {
		speex_resampler_destroy(st);
		st = nullptr;
	}
}

Error SpeexResampler::setup(int p_channels, int p_in_rate, int p_out_rate, int p_quality)
{
	if (p_channels <= 0 || p_in_rate <= 0 || p_out_rate <= 0) {
		return ERR_INVALID_PARAMETER;
	}
	if (p_quality < 0) {
		p_quality = 0;
	}
	if (p_quality > 10) {
		p_quality = 10;
	}
	if (st) {
		speex_resampler_destroy(st);
		st = nullptr;
	}
	int err = 0;
	st = speex_resampler_init((spx_uint32_t)p_channels, (spx_uint32_t)p_in_rate,
			(spx_uint32_t)p_out_rate, p_quality, &err);
	if (!st || err != RESAMPLER_ERR_SUCCESS) {
		st = nullptr;
		UtilityFunctions::push_error(
				String("SpeexResampler.setup failed: ") + String(speex_resampler_strerror(err)));
		return FAILED;
	}
	channels = p_channels;
	in_rate = p_in_rate;
	out_rate = p_out_rate;
	quality = p_quality;
	return OK;
}

Error SpeexResampler::set_rate(int p_in_rate, int p_out_rate)
{
	if (!st) {
		UtilityFunctions::push_error("SpeexResampler.set_rate: call setup() first");
		return ERR_UNCONFIGURED;
	}
	if (p_in_rate <= 0 || p_out_rate <= 0) {
		return ERR_INVALID_PARAMETER;
	}
	int err = speex_resampler_set_rate(st, (spx_uint32_t)p_in_rate, (spx_uint32_t)p_out_rate);
	if (err != RESAMPLER_ERR_SUCCESS) {
		UtilityFunctions::push_error(
				String("SpeexResampler.set_rate failed: ") + String(speex_resampler_strerror(err)));
		return FAILED;
	}
	in_rate = p_in_rate;
	out_rate = p_out_rate;
	return OK;
}

PackedFloat32Array SpeexResampler::process(const PackedFloat32Array &input)
{
	PackedFloat32Array out;
	if (!st || input.is_empty() || channels <= 0) {
		return out;
	}
	const int64_t n = input.size();
	if (n % channels != 0) {
		UtilityFunctions::push_error("SpeexResampler.process: input length not multiple of channels");
		return out;
	}
	const spx_uint32_t in_len_frames = (spx_uint32_t)(n / channels);
	/* Worst-case output frames: ceil(in * out/in) + a little latency headroom */
	const double ratio = (double)out_rate / (double)in_rate;
	spx_uint32_t out_cap_frames =
			(spx_uint32_t)std::ceil(in_len_frames * ratio) + 16u +
			(spx_uint32_t)speex_resampler_get_output_latency(st);
	if (out_cap_frames < 1) {
		out_cap_frames = 1;
	}

	std::vector<float> in_buf((size_t)n);
	for (int64_t i = 0; i < n; i++) {
		in_buf[(size_t)i] = input[(int)i];
	}
	std::vector<float> out_buf((size_t)out_cap_frames * (size_t)channels);

	spx_uint32_t in_len = in_len_frames;
	spx_uint32_t out_len = out_cap_frames;
	int err = speex_resampler_process_interleaved_float(st, in_buf.data(), &in_len, out_buf.data(),
			&out_len);
	if (err != RESAMPLER_ERR_SUCCESS) {
		UtilityFunctions::push_error(
				String("SpeexResampler.process failed: ") + String(speex_resampler_strerror(err)));
		return out;
	}
	out.resize((int64_t)out_len * channels);
	for (spx_uint32_t i = 0; i < out_len * (spx_uint32_t)channels; i++) {
		out[(int)i] = out_buf[i];
	}
	return out;
}

PackedVector2Array SpeexResampler::process2(const PackedVector2Array &input, float mono_mix)
{
	PackedVector2Array empty;
	if (!st || input.is_empty()) {
		return empty;
	}
	if (mono_mix < 0.f) {
		if (channels != 2) {
			UtilityFunctions::push_error(
					"SpeexResampler.process2(mono_mix<0): call setup(channels=2, ...) first");
			return empty;
		}
		PackedFloat32Array interleaved = speex_stereo::interleaved_from_stereo(input);
		PackedFloat32Array out = process(interleaved);
		if (out.is_empty() || out.size() % 2 != 0) {
			return empty;
		}
		return speex_stereo::stereo_from_interleaved(out);
	}
	if (channels != 1) {
		UtilityFunctions::push_error(
				"SpeexResampler.process2(mono_mix in [0,1]): call setup(channels=1, ...) first");
		return empty;
	}
	PackedFloat32Array left, right, mono;
	speex_stereo::stereo_to_planes(input, mono_mix, left, right, mono);
	PackedFloat32Array out_m = process(mono);
	return speex_stereo::mono_to_stereo(out_m);
}

void SpeexResampler::reset()
{
	if (st) {
		speex_resampler_reset_mem(st);
	}
}

int SpeexResampler::get_channels() const
{
	return channels;
}
int SpeexResampler::get_in_rate() const
{
	return in_rate;
}
int SpeexResampler::get_out_rate() const
{
	return out_rate;
}
int SpeexResampler::get_quality() const
{
	return quality;
}
int SpeexResampler::get_input_latency() const
{
	return st ? (int)speex_resampler_get_input_latency(st) : 0;
}
int SpeexResampler::get_output_latency() const
{
	return st ? (int)speex_resampler_get_output_latency(st) : 0;
}
