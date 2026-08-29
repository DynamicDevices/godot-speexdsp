# SpeexDSP GDExtension

Godot 4.5+ wrapper around [SpeexDSP](https://gitlab.xiph.org/xiph/speexdsp) as a
**git submodule**. Speex sources are **not modified** — perceptual DSP wisdom
stays in upstream.

## Status

- **SpeexResampler** — `RefCounted` wrapping `speex_resampler_*` (float interleaved)
- Later (same addon): AGC, VAD, etc. for vizemes / VoIP

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
```

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
