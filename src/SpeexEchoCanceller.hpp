#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

struct SpeexEchoState_;
typedef struct SpeexEchoState_ SpeexEchoState;

namespace godot {

/**
 * SpeexDSP acoustic echo canceller (speex_echo_cancellation).
 * Mono float frames in/out; int16 path inside (same as SpeexPreprocess).
 * Needs a far-end reference (what was played to the speakers).
 * process2() accepts stereo Vector2 frames with mono_mix (see process2 docs).
 */
class SpeexEchoCanceller : public RefCounted {
	GDCLASS(SpeexEchoCanceller, RefCounted);

	SpeexEchoState *st = nullptr;
	SpeexEchoState *st_r = nullptr;
	int frame_size = 0;
	int filter_length = 0;
	int sample_rate = 0;

	void _destroy_states();
	Error _ensure_right();

protected:
	static void _bind_methods();

public:
	SpeexEchoCanceller() = default;
	~SpeexEchoCanceller() override;

	/**
	 * frame_size: samples per call (10–20 ms, e.g. 160 @ 16 kHz).
	 * filter_length: echo tail in samples (≈100–500 ms, e.g. 1600 @ 16 kHz).
	 */
	Error setup(int p_frame_size, int p_filter_length, int p_sample_rate);

	/**
	 * Cancel echo for one aligned frame.
	 * rec = mic (near + echo), play = far-end reference (speaker), same length == frame_size.
	 */
	PackedFloat32Array process(const PackedFloat32Array &rec, const PackedFloat32Array &play);

	/**
	 * Stereo rec/play (length == frame_size as Vector2).
	 * mono_mix < 0: independent L/R echo states (rec.x/play.x and rec.y/play.y).
	 * mono_mix in [0,1]: mix both to mono, cancel, copy result to x and y.
	 */
	PackedVector2Array process2(const PackedVector2Array &rec, const PackedVector2Array &play,
			float mono_mix = -1.f);

	void reset();
	int get_frame_size() const;
	int get_filter_length() const;
	int get_sample_rate() const;
};

} // namespace godot
