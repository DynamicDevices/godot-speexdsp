#pragma once

#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>

namespace godot {
namespace speex_stereo {

/** mono_mix < 0 → dual independent; else L*(1-m)+R*m into mono. */
inline void stereo_to_planes(const PackedVector2Array &in, float mono_mix, PackedFloat32Array &left,
		PackedFloat32Array &right, PackedFloat32Array &mono)
{
	const int64_t n = in.size();
	left.resize(n);
	right.resize(n);
	mono.resize(n);
	const bool dual = mono_mix < 0.f;
	const float m = dual ? 0.5f : std::max(0.f, std::min(1.f, mono_mix));
	const float om = 1.f - m;
	for (int64_t i = 0; i < n; i++) {
		const Vector2 v = in[(int)i];
		left[(int)i] = v.x;
		right[(int)i] = v.y;
		mono[(int)i] = dual ? 0.f : (v.x * om + v.y * m);
	}
}

inline PackedVector2Array planes_to_stereo(const PackedFloat32Array &left, const PackedFloat32Array &right)
{
	PackedVector2Array out;
	const int64_t n = left.size();
	out.resize(n);
	for (int64_t i = 0; i < n; i++) {
		out[(int)i] = Vector2(left[(int)i], right[(int)i]);
	}
	return out;
}

inline PackedVector2Array mono_to_stereo(const PackedFloat32Array &mono)
{
	PackedVector2Array out;
	const int64_t n = mono.size();
	out.resize(n);
	for (int64_t i = 0; i < n; i++) {
		const float s = mono[(int)i];
		out[(int)i] = Vector2(s, s);
	}
	return out;
}

inline PackedFloat32Array interleaved_from_stereo(const PackedVector2Array &in)
{
	PackedFloat32Array out;
	out.resize(in.size() * 2);
	for (int64_t i = 0; i < in.size(); i++) {
		const Vector2 v = in[(int)i];
		out[(int)(i * 2)] = v.x;
		out[(int)(i * 2 + 1)] = v.y;
	}
	return out;
}

inline PackedVector2Array stereo_from_interleaved(const PackedFloat32Array &in)
{
	PackedVector2Array out;
	const int64_t frames = in.size() / 2;
	out.resize(frames);
	for (int64_t i = 0; i < frames; i++) {
		out[(int)i] = Vector2(in[(int)(i * 2)], in[(int)(i * 2 + 1)]);
	}
	return out;
}

} // namespace speex_stereo
} // namespace godot
