# SpeexDSP GDExtension

Godot 4.5+ wrapper around [SpeexDSP](https://gitlab.xiph.org/xiph/speexdsp) as a
**git submodule**. Speex sources are **not modified** — perceptual DSP wisdom
stays in upstream.

## Status

| Class | Role |
|-------|------|
| `SpeexResampler` | Float interleaved resampler (`speex_resampler_*`) |
| `SpeexPreprocess` | Denoise / AGC / VAD on fixed mono frames (`speex_preprocess_*`) |
| `SpeexEchoCanceller` | Acoustic echo canceller (`speex_echo_*`; needs far-end ref) |

## Build

```bash
git submodule update --init --recursive
scons platform=linux target=template_debug
# → addons/speexdsp/bin/libspeexdsp.linux.template_debug.x86_64.so
```

## GDScript

```gdscript
var rs := SpeexResampler.new()
rs.setup(1, 48000, 16000, 5)  # channels, in_rate, out_rate, quality 0–10
var out: PackedFloat32Array = rs.process(pcm_in)
rs.set_rate(48000, 8000)  # live rate change without teardown

var pp := SpeexPreprocess.new()
pp.setup(160, 16000)  # 10 ms @ 16 kHz
pp.set_vad(true)
pp.set_agc(true)
pp.set_agc_level(8000.0)
var frame_out := pp.process(frame_in)  # length == frame_size
var speech := pp.get_last_vad()

var aec := SpeexEchoCanceller.new()
aec.setup(160, 1600, 16000)  # frame, filter_length (~100 ms), rate
var cleaned := aec.process(mic_frame, far_end_frame)

# Stereo Vector2 frames (mic / AudioEffectCapture style):
# mono_mix < 0 → dual Speex states on L/R; 0..1 → mix then process, copy to x,y
var stereo_out: PackedVector2Array = pp.process2(stereo_in, -1.0)
var stereo_mix := pp.process2(stereo_in, 0.5)  # equal L/R mix
rs.setup(2, 48000, 16000, 5)
var stereo_rs: PackedVector2Array = rs.process2(stereo_in, -1.0)
```

Headless: `demo/speex_smoke.tscn` → `SPEEXDSP_GODOT_SMOKE_OK`.

Live mic (VAD/AGC/denoise/AEC + down→up hear-back): open `demo/speex_live.tscn`
in the editor (based on the goatchurchprime mic_record Capture-bus pattern).
AEC uses the round-trip speaker path as the far-end reference.

## Vizemes

`vizemes-align` MelFrontend mic path (`push_pcm_stereo`) links the same Speex
resampler (submodule) instead of linear interpolation. GDScript
`VisemeUtils.resample_pcm` uses `SpeexResampler` when the addon is loaded.

## Layout

| Path | Role |
|------|------|
| `thirdparty/speexdsp/` | Upstream submodule (untouched) |
| `thirdparty/speexdsp_glue/` | Build defines / types only |
| `src/` | Godot C++ bindings |
| `addons/speexdsp/` | Ship unit (`.gdextension` + `bin/`) |

## License

- This wrapper: MIT (see `LICENSE`)
- SpeexDSP: BSD-style (see `thirdparty/speexdsp/COPYING`)
