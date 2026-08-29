#include "SpeexPreprocess.hpp"
#include "speex_stereo_util.hpp"

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
	ClassDB::bind_method(D_METHOD("process2", "frame", "mono_mix"), &SpeexPreprocess::process2,
			DEFVAL(-1.f));
	ClassDB::bind_method(D_METHOD("get_last_vad"), &SpeexPreprocess::get_last_vad);
	ClassDB::bind_method(D_METHOD("get_frame_size"), &SpeexPreprocess::get_frame_size);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &SpeexPreprocess::get_sample_rate);
}

void SpeexPreprocess::_destroy_states()
{
	if (st_r) {
		speex_preprocess_state_destroy(st_r);
		st_r = nullptr;
	}
	if (st) {
		speex_preprocess_state_destroy(st);
		st = nullptr;
	}
}

void SpeexPreprocess::_apply_ctl(SpeexPreprocessState *target)
{
	if (!target) {
		return;
	}
	int v;
	v = cfg_denoise ? 1 : 0;
	speex_preprocess_ctl(target, SPEEX_PREPROCESS_SET_DENOISE, &v);
	v = cfg_agc ? 1 : 0;
	speex_preprocess_ctl(target, SPEEX_PREPROCESS_SET_AGC, &v);
	v = cfg_vad ? 1 : 0;
	speex_preprocess_ctl(target, SPEEX_PREPROCESS_SET_VAD, &v);
	float lvl = cfg_agc_level;
	speex_preprocess_ctl(target, SPEEX_PREPROCESS_SET_AGC_LEVEL, &lvl);
	v = cfg_noise_suppress;
	speex_preprocess_ctl(target, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &v);
}

Error SpeexPreprocess::_ensure_right()
{
	if (st_r) {
		return OK;
	}
	if (!st || frame_size <= 0 || sample_rate <= 0) {
		return ERR_UNCONFIGURED;
	}
	st_r = speex_preprocess_state_init(frame_size, sample_rate);
	if (!st_r) {
		UtilityFunctions::push_error("SpeexPreprocess: failed to init right-channel state");
		return FAILED;
	}
	_apply_ctl(st_r);
	return OK;
}

SpeexPreprocess::~SpeexPreprocess()
{
	_destroy_states();
}

Error SpeexPreprocess::setup(int p_frame_size, int p_sample_rate)
{
	if (p_frame_size <= 0 || p_sample_rate <= 0) {
		return ERR_INVALID_PARAMETER;
	}
	_destroy_states();
	st = speex_preprocess_state_init(p_frame_size, p_sample_rate);
	if (!st) {
		UtilityFunctions::push_error("SpeexPreprocess.setup failed");
		return FAILED;
	}
	frame_size = p_frame_size;
	sample_rate = p_sample_rate;
	last_vad = false;
	_apply_ctl(st);
	return OK;
}

void SpeexPreprocess::set_denoise(bool enabled)
{
	cfg_denoise = enabled;
	_apply_ctl(st);
	_apply_ctl(st_r);
}

void SpeexPreprocess::set_agc(bool enabled)
{
	cfg_agc = enabled;
	_apply_ctl(st);
	_apply_ctl(st_r);
}

void SpeexPreprocess::set_vad(bool enabled)
{
	cfg_vad = enabled;
	_apply_ctl(st);
	_apply_ctl(st_r);
}

void SpeexPreprocess::set_agc_level(float level)
{
	cfg_agc_level = level;
	_apply_ctl(st);
	_apply_ctl(st_r);
}

void SpeexPreprocess::set_noise_suppress(int neg_db)
{
	cfg_noise_suppress = neg_db;
	_apply_ctl(st);
	_apply_ctl(st_r);
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

PackedVector2Array SpeexPreprocess::process2(const PackedVector2Array &frame, float mono_mix)
{
	PackedVector2Array empty;
	if (!st || frame.size() != frame_size) {
		UtilityFunctions::push_error("SpeexPreprocess.process2: need exactly frame_size Vector2 samples");
		return empty;
	}
	PackedFloat32Array left, right, mono;
	speex_stereo::stereo_to_planes(frame, mono_mix, left, right, mono);
	if (mono_mix < 0.f) {
		if (_ensure_right() != OK) {
			return empty;
		}
		PackedFloat32Array out_l = process(left);
		// process right on st_r without clobbering last_vad from left oddly — OR both
		std::vector<spx_int16_t> buf((size_t)frame_size);
		for (int i = 0; i < frame_size; i++) {
			float s = std::max(-1.f, std::min(1.f, right[i]));
			buf[(size_t)i] = (spx_int16_t)std::lround(s * 32767.f);
		}
		int vad_r = speex_preprocess_run(st_r, buf.data());
		last_vad = last_vad || (vad_r != 0);
		PackedFloat32Array out_r;
		out_r.resize(frame_size);
		for (int i = 0; i < frame_size; i++) {
			out_r[i] = (float)buf[(size_t)i] / 32768.f;
		}
		return speex_stereo::planes_to_stereo(out_l, out_r);
	}
	PackedFloat32Array out_m = process(mono);
	return speex_stereo::mono_to_stereo(out_m);
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
