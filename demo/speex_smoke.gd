extends Node
## Headless smoke: SpeexResampler 48k→16k + SpeexPreprocess AGC/VAD.

func _ready() -> void:
	var rs := SpeexResampler.new()
	assert(rs.setup(1, 48000, 16000, 5) == OK)
	var pcm := PackedFloat32Array()
	pcm.resize(480)
	for i in pcm.size():
		pcm[i] = 0.2 * sin(TAU * 440.0 * float(i) / 48000.0)
	var out: PackedFloat32Array = rs.process(pcm)
	assert(out.size() > 100)
	assert(rs.set_rate(48000, 8000) == OK)
	assert(rs.get_out_rate() == 8000)
	var out_narrow: PackedFloat32Array = rs.process(pcm)
	assert(out_narrow.size() > 0 and out_narrow.size() < out.size())

	var pp := SpeexPreprocess.new()
	assert(pp.setup(160, 16000) == OK)
	pp.set_vad(true)
	pp.set_agc(true)
	pp.set_agc_level(8000.0)
	pp.set_denoise(true)
	var frame := PackedFloat32Array()
	frame.resize(160)
	for i in 160:
		frame[i] = out[mini(i, out.size() - 1)]
	var processed: PackedFloat32Array = pp.process(frame)
	assert(processed.size() == 160)

	# AEC: cancel a delayed far-end tone mixed into "mic"
	var aec := SpeexEchoCanceller.new()
	assert(aec.setup(160, 1600, 16000) == OK)
	var prev_play := PackedFloat32Array()
	prev_play.resize(160)
	var energy_before := 0.0
	var energy_after := 0.0
	for n in 40:
		var play := PackedFloat32Array()
		play.resize(160)
		var rec := PackedFloat32Array()
		rec.resize(160)
		for i in 160:
			var t: float = 0.3 * sin(TAU * 440.0 * float(n * 160 + i) / 16000.0)
			play[i] = t
			rec[i] = 0.5 * prev_play[i]  # pure echo of previous far-end
		var cleaned: PackedFloat32Array = aec.process(rec, play)
		assert(cleaned.size() == 160)
		if n == 5:
			for i in 160:
				energy_before += absf(rec[i])
		if n == 35:
			for i in 160:
				energy_after += absf(cleaned[i])
		prev_play = play
	assert(energy_after < energy_before * 0.85)

	print("SPEEXDSP_GODOT_SMOKE_OK resample_out=%d set_rate_out=%d vad=%s aec_e=%.3f→%.3f" % [
		out.size(), out_narrow.size(), str(pp.get_last_vad()), energy_before, energy_after
	])
	get_tree().quit(0)
