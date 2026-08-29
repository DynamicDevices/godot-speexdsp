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

	print("SPEEXDSP_GODOT_SMOKE_OK resample_out=%d vad=%s" % [out.size(), str(pp.get_last_vad())])
	get_tree().quit(0)
