#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace godot {

/** Thin Godot wrapper around SpeexDSP speex_resampler (float path). Sources untouched. */
class SpeexResampler : public RefCounted {
	GDCLASS(SpeexResampler, RefCounted);

	SpeexResamplerState *st = nullptr;
	int channels = 1;
	int in_rate = 0;
	int out_rate = 0;
	int quality = 5;

protected:
	static void _bind_methods();

public:
	SpeexResampler() = default;
	~SpeexResampler() override;

	/**
	 * Create / replace the Speex resampler state.
	 * quality: 0–10 (SpeexSPEEX_RESAMPLER_QUALITY_*; default 5).
	 */
	Error setup(int p_channels, int p_in_rate, int p_out_rate, int p_quality = 5);

	/**
	 * Change rates on a live resampler (speex_resampler_set_rate).
	 * Does not recreate the state; filter adapts. Call setup() to change quality/channels.
	 */
	Error set_rate(int p_in_rate, int p_out_rate);

	/** Resample mono or interleaved float PCM (length must be multiple of channels). */
	PackedFloat32Array process(const PackedFloat32Array &input);

	void reset();
	int get_channels() const;
	int get_in_rate() const;
	int get_out_rate() const;
	int get_quality() const;
	int get_input_latency() const;
	int get_output_latency() const;
};

} // namespace godot
