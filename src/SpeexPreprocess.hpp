#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

struct SpeexPreprocessState_;
typedef struct SpeexPreprocessState_ SpeexPreprocessState;

namespace godot {

/**
 * SpeexDSP preprocessor: denoise / AGC / VAD on fixed-size mono frames (int16 path).
 * Frame size is typically 10–20 ms at the stream sample rate (e.g. 160 @ 16 kHz).
 */
class SpeexPreprocess : public RefCounted {
	GDCLASS(SpeexPreprocess, RefCounted);

	SpeexPreprocessState *st = nullptr;
	int frame_size = 0;
	int sample_rate = 0;
	bool last_vad = false;

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

	bool get_last_vad() const;
	int get_frame_size() const;
	int get_sample_rate() const;
};

} // namespace godot
