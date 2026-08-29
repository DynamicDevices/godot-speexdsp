#include "SpeexEchoCanceller.hpp"
#include "speex_stereo_util.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include "speex/speex_echo.h"

using namespace godot;

void SpeexEchoCanceller::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("setup", "frame_size", "filter_length", "sample_rate"),
			&SpeexEchoCanceller::setup);
	ClassDB::bind_method(D_METHOD("process", "rec", "play"), &SpeexEchoCanceller::process);
	ClassDB::bind_method(D_METHOD("process2", "rec", "play", "mono_mix"),
			&SpeexEchoCanceller::process2, DEFVAL(-1.f));
	ClassDB::bind_method(D_METHOD("reset"), &SpeexEchoCanceller::reset);
	ClassDB::bind_method(D_METHOD("get_frame_size"), &SpeexEchoCanceller::get_frame_size);
	ClassDB::bind_method(D_METHOD("get_filter_length"), &SpeexEchoCanceller::get_filter_length);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &SpeexEchoCanceller::get_sample_rate);
}

void SpeexEchoCanceller::_destroy_states()
{
	if (st_r) {
		speex_echo_state_destroy(st_r);
		st_r = nullptr;
	}
	if (st) {
		speex_echo_state_destroy(st);
		st = nullptr;
	}
}

Error SpeexEchoCanceller::_ensure_right()
{
	if (st_r) {
		return OK;
	}
	if (!st || frame_size <= 0 || filter_length <= 0) {
		return ERR_UNCONFIGURED;
	}
	st_r = speex_echo_state_init(frame_size, filter_length);
	if (!st_r) {
		UtilityFunctions::push_error("SpeexEchoCanceller: failed to init right-channel state");
		return FAILED;
	}
	int rate = sample_rate;
	speex_echo_ctl(st_r, SPEEX_ECHO_SET_SAMPLING_RATE, &rate);
	return OK;
}

SpeexEchoCanceller::~SpeexEchoCanceller()
{
	_destroy_states();
}

Error SpeexEchoCanceller::setup(int p_frame_size, int p_filter_length, int p_sample_rate)
{
	if (p_frame_size <= 0 || p_filter_length <= 0 || p_sample_rate <= 0) {
		return ERR_INVALID_PARAMETER;
	}
	_destroy_states();
	st = speex_echo_state_init(p_frame_size, p_filter_length);
	if (!st) {
		UtilityFunctions::push_error("SpeexEchoCanceller.setup failed");
		return FAILED;
	}
	int rate = p_sample_rate;
	speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &rate);
	frame_size = p_frame_size;
	filter_length = p_filter_length;
	sample_rate = p_sample_rate;
	return OK;
}

PackedFloat32Array SpeexEchoCanceller::process(const PackedFloat32Array &rec,
		const PackedFloat32Array &play)
{
	PackedFloat32Array out;
	if (!st || rec.size() != frame_size || play.size() != frame_size) {
		UtilityFunctions::push_error(
				"SpeexEchoCanceller.process: need rec and play of length frame_size");
		return out;
	}
	std::vector<spx_int16_t> rec_i((size_t)frame_size);
	std::vector<spx_int16_t> play_i((size_t)frame_size);
	std::vector<spx_int16_t> out_i((size_t)frame_size);
	for (int i = 0; i < frame_size; i++) {
		float r = std::max(-1.f, std::min(1.f, rec[i]));
		float p = std::max(-1.f, std::min(1.f, play[i]));
		rec_i[(size_t)i] = (spx_int16_t)std::lround(r * 32767.f);
		play_i[(size_t)i] = (spx_int16_t)std::lround(p * 32767.f);
	}
	speex_echo_cancellation(st, rec_i.data(), play_i.data(), out_i.data());
	out.resize(frame_size);
	for (int i = 0; i < frame_size; i++) {
		out[i] = (float)out_i[(size_t)i] / 32768.f;
	}
	return out;
}

PackedVector2Array SpeexEchoCanceller::process2(const PackedVector2Array &rec,
		const PackedVector2Array &play, float mono_mix)
{
	PackedVector2Array empty;
	if (!st || rec.size() != frame_size || play.size() != frame_size) {
		UtilityFunctions::push_error(
				"SpeexEchoCanceller.process2: need rec and play of length frame_size");
		return empty;
	}
	PackedFloat32Array rec_l, rec_r, rec_m, play_l, play_r, play_m;
	speex_stereo::stereo_to_planes(rec, mono_mix, rec_l, rec_r, rec_m);
	speex_stereo::stereo_to_planes(play, mono_mix, play_l, play_r, play_m);
	if (mono_mix < 0.f) {
		if (_ensure_right() != OK) {
			return empty;
		}
		PackedFloat32Array out_l = process(rec_l, play_l);
		std::vector<spx_int16_t> rec_i((size_t)frame_size);
		std::vector<spx_int16_t> play_i((size_t)frame_size);
		std::vector<spx_int16_t> out_i((size_t)frame_size);
		for (int i = 0; i < frame_size; i++) {
			float r = std::max(-1.f, std::min(1.f, rec_r[i]));
			float p = std::max(-1.f, std::min(1.f, play_r[i]));
			rec_i[(size_t)i] = (spx_int16_t)std::lround(r * 32767.f);
			play_i[(size_t)i] = (spx_int16_t)std::lround(p * 32767.f);
		}
		speex_echo_cancellation(st_r, rec_i.data(), play_i.data(), out_i.data());
		PackedFloat32Array out_r;
		out_r.resize(frame_size);
		for (int i = 0; i < frame_size; i++) {
			out_r[i] = (float)out_i[(size_t)i] / 32768.f;
		}
		return speex_stereo::planes_to_stereo(out_l, out_r);
	}
	PackedFloat32Array out_m = process(rec_m, play_m);
	return speex_stereo::mono_to_stereo(out_m);
}

void SpeexEchoCanceller::reset()
{
	if (st) {
		speex_echo_state_reset(st);
	}
	if (st_r) {
		speex_echo_state_reset(st_r);
	}
}

int SpeexEchoCanceller::get_frame_size() const
{
	return frame_size;
}
int SpeexEchoCanceller::get_filter_length() const
{
	return filter_length;
}
int SpeexEchoCanceller::get_sample_rate() const
{
	return sample_rate;
}
