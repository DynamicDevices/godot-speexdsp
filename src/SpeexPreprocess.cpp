#include "SpeexPreprocess.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include "speex/speex_preprocess.h"

using namespace godot;

void SpeexPreprocess::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("setup", "frame_size", "sample_rate"), &SpeexPreprocess::setup);
	ClassDB::bind_method(D_METHOD("set_denoise", "enabled"), &SpeexPreprocess::set_denoise);
	ClassDB::bind_method(D_METHOD("set_agc", "enabled"), &SpeexPreprocess::set_agc);
	ClassDB::bind_method(D_METHOD("set_vad", "enabled"), &SpeexPreprocess::set_vad);
	ClassDB::bind_method(D_METHOD("set_agc_level", "level"), &SpeexPreprocess::set_agc_level);
	ClassDB::bind_method(D_METHOD("set_noise_suppress", "neg_db"), &SpeexPreprocess::set_noise_suppress);
	ClassDB::bind_method(D_METHOD("process", "frame"), &SpeexPreprocess::process);
	ClassDB::bind_method(D_METHOD("get_last_vad"), &SpeexPreprocess::get_last_vad);
	ClassDB::bind_method(D_METHOD("get_frame_size"), &SpeexPreprocess::get_frame_size);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &SpeexPreprocess::get_sample_rate);
}

SpeexPreprocess::~SpeexPreprocess()
{
	if (st) {
		speex_preprocess_state_destroy(st);
		st = nullptr;
	}
}

Error SpeexPreprocess::setup(int p_frame_size, int p_sample_rate)
{
	if (p_frame_size <= 0 || p_sample_rate <= 0) {
		return ERR_INVALID_PARAMETER;
	}
	if (st) {
		speex_preprocess_state_destroy(st);
		st = nullptr;
	}
	st = speex_preprocess_state_init(p_frame_size, p_sample_rate);
	if (!st) {
		UtilityFunctions::push_error("SpeexPreprocess.setup failed");
		return FAILED;
	}
	frame_size = p_frame_size;
	sample_rate = p_sample_rate;
	last_vad = false;
	return OK;
}

void SpeexPreprocess::set_denoise(bool enabled)
{
	if (!st) {
		return;
	}
	int v = enabled ? 1 : 0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &v);
}

void SpeexPreprocess::set_agc(bool enabled)
{
	if (!st) {
		return;
	}
	int v = enabled ? 1 : 0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &v);
}

void SpeexPreprocess::set_vad(bool enabled)
{
	if (!st) {
		return;
	}
	int v = enabled ? 1 : 0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_VAD, &v);
}

void SpeexPreprocess::set_agc_level(float level)
{
	if (!st) {
		return;
	}
	float v = level;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC_LEVEL, &v);
}

void SpeexPreprocess::set_noise_suppress(int neg_db)
{
	if (!st) {
		return;
	}
	int v = neg_db;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &v);
}

PackedFloat32Array SpeexPreprocess::process(const PackedFloat32Array &frame)
{
	PackedFloat32Array out;
	if (!st || frame.size() != frame_size) {
		UtilityFunctions::push_error("SpeexPreprocess.process: need exactly frame_size samples");
		return out;
	}
	std::vector<spx_int16_t> buf((size_t)frame_size);
	for (int i = 0; i < frame_size; i++) {
		float s = frame[i];
		s = std::max(-1.f, std::min(1.f, s));
		buf[(size_t)i] = (spx_int16_t)std::lround(s * 32767.f);
	}
	int vad = speex_preprocess_run(st, buf.data());
	last_vad = vad != 0;
	out.resize(frame_size);
	for (int i = 0; i < frame_size; i++) {
		out[i] = (float)buf[(size_t)i] / 32768.f;
	}
	return out;
}

bool SpeexPreprocess::get_last_vad() const
{
	return last_vad;
}
int SpeexPreprocess::get_frame_size() const
{
	return frame_size;
}
int SpeexPreprocess::get_sample_rate() const
{
	return sample_rate;
}
