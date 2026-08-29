#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

struct SpeexPreprocessState_;
typedef struct SpeexPreprocessState_ SpeexPreprocessState;

namespace godot {

/**
 * SpeexDSP preprocessor: denoise / AGC / VAD on fixed-size mono frames (int16 path).
 * Frame size is typically 10–20 ms at the stream sample rate (e.g. 160 @ 16 kHz).
 * process2() accepts stereo Vector2 frames with mono_mix (see process2 docs).
 */
class SpeexPreprocess : public RefCounted {
	GDCLASS(SpeexPreprocess, RefCounted);

	SpeexPreprocessState *st = nullptr;
	SpeexPreprocessState *st_r = nullptr; // dual-channel process2 (mono_mix < 0)
	int frame_size = 0;
	int sample_rate = 0;
	bool last_vad = false;
	bool cfg_denoise = true;
	bool cfg_agc = false;
	bool cfg_vad = false;
	float cfg_agc_level = 8000.f;
	int cfg_noise_suppress = -15;

	void _destroy_states();
	void _apply_ctl(SpeexPreprocessState *target);
	Error _ensure_right();

protected:
	static void _bind_methods();

public:
	SpeexPreprocess() = default;
	~SpeexPreprocess() override;

	Error setup(int p_frame_size, int p_sample_rate);
	void set_denoise(bool enabled);
	void set_agc(bool enabled);
	void set_vad(bool enabled);
	void set_agc_level(float level);
	void set_noise_suppress(int neg_db);

	/**
	 * Process one mono float frame (length == frame_size).
	 * Returns processed float PCM; get_last_vad() after call when VAD is enabled.
	 */
	PackedFloat32Array process(const PackedFloat32Array &frame);

	/**
	 * Stereo frame (length == frame_size as Vector2 samples).
	 * mono_mix < 0: run two Speex states on L and R independently.
	 * mono_mix in [0,1]: mix L*(1-m)+R*m → mono → process → copy to both x,y.
	 * Typical: 0 (left-only), 1 (right-only), 0.5 (equal), -1 (dual).
	 */
	PackedVector2Array process2(const PackedVector2Array &frame, float mono_mix = -1.f);

	bool get_last_vad() const;
	int get_frame_size() const;
	int get_sample_rate() const;
};

} // namespace godot
