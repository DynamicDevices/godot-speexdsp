extends Control
## Live mic demo: SpeexPreprocess (VAD/AGC/denoise) + optional Speex down→up resample
## hear-back. Layout inspired by goatchurchprime mic_record (AudioEffectCapture bus).

const FRAME_MS := 10
const NARROW_RATES := [8000, 12000, 16000, 24000]

@onready var status_label: Label = %Status
@onready var vad_label: Label = %VadLabel
@onready var level_bar: ProgressBar = %LevelBar
@onready var mic_player: AudioStreamPlayer = %MicPlayer
@onready var monitor_player: AudioStreamPlayer = %MonitorPlayer

var capture: AudioEffectCapture
var preprocess: SpeexPreprocess
var aec: SpeexEchoCanceller
var down_rs: SpeexResampler
var up_rs: SpeexResampler

var mic_rate: int = 48000
var narrow_rate: int = 16000
var quality: int = 5
var frame_size: int = 160
var mono_carry: PackedFloat32Array = PackedFloat32Array()
var far_frames: Array = []  # queued far-end frames for AEC (what we played)
var vad_hits: int = 0
var frames_seen: int = 0
var listening: bool = false
var roundtrip: bool = true
var aec_on: bool = false
var playback: AudioStreamGeneratorPlayback


func _ready() -> void:
	var idx := AudioServer.get_bus_index(&"Record")
	if idx < 0:
		status_label.text = "Missing Record bus — check default_bus_layout.tres"
		set_process(false)
		return
	capture = AudioServer.get_bus_effect(idx, 0) as AudioEffectCapture
	if capture == null:
		status_label.text = "Record bus needs AudioEffectCapture"
		set_process(false)
		return

	mic_rate = int(AudioServer.get_mix_rate())
	%MicRate.text = "Mix: %d Hz" % mic_rate
	_fill_rate_option(%NarrowRate, NARROW_RATES, 16000)
	%Quality.value = quality
	%QualityLabel.text = "Quality: %d" % quality
	_rebuild_dsp()
	_setup_monitor()
	%Listen.toggled.connect(_on_listen_toggled)
	%Roundtrip.toggled.connect(_on_roundtrip_toggled)
	%VadCheck.toggled.connect(_on_vad_toggled)
	%AgcCheck.toggled.connect(_on_agc_toggled)
	%DenoiseCheck.toggled.connect(_on_denoise_toggled)
	%AecCheck.toggled.connect(_on_aec_toggled)
	%AgcLevel.value_changed.connect(_on_agc_level_changed)
	%Quality.value_changed.connect(_on_quality_changed)
	%NarrowRate.item_selected.connect(_on_narrow_rate_selected)
	status_label.text = "Ready — enable Listen (mic permission may prompt)"


func _fill_rate_option(opt: OptionButton, rates: Array, selected: int) -> void:
	opt.clear()
	var sel_i := 0
	for i in rates.size():
		opt.add_item("%d Hz" % rates[i], i)
		if int(rates[i]) == selected:
			sel_i = i
	opt.select(sel_i)


func _rebuild_dsp() -> void:
	frame_size = maxi(1, int(narrow_rate * FRAME_MS / 1000.0))
	preprocess = SpeexPreprocess.new()
	assert(preprocess.setup(frame_size, narrow_rate) == OK)
	preprocess.set_vad(%VadCheck.button_pressed)
	preprocess.set_agc(%AgcCheck.button_pressed)
	preprocess.set_denoise(%DenoiseCheck.button_pressed)
	preprocess.set_agc_level(%AgcLevel.value)

	down_rs = SpeexResampler.new()
	up_rs = SpeexResampler.new()
	assert(down_rs.setup(1, mic_rate, narrow_rate, quality) == OK)
	assert(up_rs.setup(1, narrow_rate, mic_rate, quality) == OK)
	# ~100 ms echo tail
	var filter_len: int = maxi(frame_size * 2, int(narrow_rate * 0.1))
	aec = SpeexEchoCanceller.new()
	assert(aec.setup(frame_size, filter_len, narrow_rate) == OK)
	aec_on = %AecCheck.button_pressed
	mono_carry.clear()
	far_frames.clear()
	%FrameInfo.text = "Frame: %d samples @ %d Hz (%d ms)  AEC filter: %d" % [
		frame_size, narrow_rate, FRAME_MS, filter_len
	]


func _setup_monitor() -> void:
	var gen := AudioStreamGenerator.new()
	gen.mix_rate = float(mic_rate)
	gen.buffer_length = 0.1
	monitor_player.stream = gen
	monitor_player.bus = &"Master"


func _on_listen_toggled(on: bool) -> void:
	listening = on
	if on:
		if OS.get_name() == "Android":
			OS.request_permission("android.permission.RECORD_AUDIO")
		mic_player.play()
		if roundtrip and not monitor_player.playing:
			monitor_player.play()
			playback = monitor_player.get_stream_playback() as AudioStreamGeneratorPlayback
		status_label.text = "Listening…"
		capture.clear_buffer()
	else:
		mic_player.stop()
		monitor_player.stop()
		playback = null
		status_label.text = "Stopped"


func _on_roundtrip_toggled(on: bool) -> void:
	roundtrip = on
	if listening and on:
		if not monitor_player.playing:
			monitor_player.play()
			playback = monitor_player.get_stream_playback() as AudioStreamGeneratorPlayback
	elif not on:
		monitor_player.stop()
		playback = null


func _on_vad_toggled(on: bool) -> void:
	if preprocess:
		preprocess.set_vad(on)


func _on_agc_toggled(on: bool) -> void:
	if preprocess:
		preprocess.set_agc(on)


func _on_denoise_toggled(on: bool) -> void:
	if preprocess:
		preprocess.set_denoise(on)


func _on_aec_toggled(on: bool) -> void:
	aec_on = on
	if aec:
		aec.reset()
	far_frames.clear()


func _on_agc_level_changed(v: float) -> void:
	%AgcLevelLabel.text = "AGC level: %.0f" % v
	if preprocess:
		preprocess.set_agc_level(v)


func _on_quality_changed(v: float) -> void:
	quality = int(v)
	%QualityLabel.text = "Quality: %d" % quality
	if down_rs and up_rs:
		# Quality needs a fresh setup; rates can use set_rate later.
		assert(down_rs.setup(1, mic_rate, narrow_rate, quality) == OK)
		assert(up_rs.setup(1, narrow_rate, mic_rate, quality) == OK)


func _on_narrow_rate_selected(index: int) -> void:
	narrow_rate = int(NARROW_RATES[index])
	_rebuild_dsp()


func _process(_delta: float) -> void:
	if not listening or capture == null:
		return
	var avail := capture.get_frames_available()
	if avail < 1:
		return
	var stereo: PackedVector2Array = capture.get_buffer(avail)
	var mono := PackedFloat32Array()
	mono.resize(stereo.size())
	var peak := 0.0
	for i in stereo.size():
		var s: float = 0.5 * (stereo[i].x + stereo[i].y)
		mono[i] = s
		peak = maxf(peak, absf(s))
	level_bar.value = clampf(peak * 100.0, 0.0, 100.0)

	var narrow: PackedFloat32Array = down_rs.process(mono)
	# Append to carry and peel Speex frames
	for i in narrow.size():
		mono_carry.append(narrow[i])

	var out_for_up := PackedFloat32Array()
	while mono_carry.size() >= frame_size:
		var frame := PackedFloat32Array()
		frame.resize(frame_size)
		for i in frame_size:
			frame[i] = mono_carry[i]
		mono_carry = mono_carry.slice(frame_size)
		var far := PackedFloat32Array()
		far.resize(frame_size)
		if far_frames.size() > 0:
			far = far_frames.pop_front()
		var cleaned: PackedFloat32Array = frame
		if aec_on and aec:
			cleaned = aec.process(frame, far)
		var processed: PackedFloat32Array = preprocess.process(cleaned)
		frames_seen += 1
		if preprocess.get_last_vad():
			vad_hits += 1
		# Far-end reference for later mic frames = what we send to speakers.
		far_frames.append(processed)
		for i in processed.size():
			out_for_up.append(processed[i])

	vad_label.text = "VAD: %s  (%d/%d frames speech)" % [
		"SPEECH" if preprocess.get_last_vad() else "silence",
		vad_hits,
		frames_seen,
	]

	if roundtrip and playback and out_for_up.size() > 0:
		var wide: PackedFloat32Array = up_rs.process(out_for_up)
		var push_n: int = mini(wide.size(), playback.get_frames_available())
		for i in push_n:
			var a: float = wide[i]
			playback.push_frame(Vector2(a, a))
