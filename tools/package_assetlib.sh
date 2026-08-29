#!/usr/bin/env bash
# Build Asset Library / consumer zip: addons/speexdsp/ (debug .so + .gdextension + README).
# Output: /tmp/godot-speexdsp-<ver>-linux-x86_64.zip
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG_VER="${PKG_VER:-$(git -C "$ROOT" describe --tags --always --dirty 2>/dev/null || echo dev)}"
OUT_DIR="${OUT_DIR:-/tmp/godot-speexdsp-assetlib}"
ZIP="${ZIP:-/tmp/godot-speexdsp-${PKG_VER}-linux-x86_64.zip}"

cd "$ROOT"
git submodule update --init --recursive
if [[ "${SKIP_SCONS:-0}" != "1" ]]; then
	scons -j"$(nproc)" platform=linux target=template_debug
	scons -j"$(nproc)" platform=linux target=template_release || true
fi

DBG="addons/speexdsp/bin/libspeexdsp.linux.template_debug.x86_64.so"
test -f "$DBG"

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR/addons/speexdsp/bin"
cp -a addons/speexdsp/speexdsp.gdextension addons/speexdsp/README.md "$OUT_DIR/addons/speexdsp/"
cp -a "$DBG" "$OUT_DIR/addons/speexdsp/bin/"
if [[ -f addons/speexdsp/bin/libspeexdsp.linux.template_release.x86_64.so ]]; then
	cp -a addons/speexdsp/bin/libspeexdsp.linux.template_release.x86_64.so "$OUT_DIR/addons/speexdsp/bin/"
fi
# optional LICENSE at addon root
[[ -f LICENSE ]] && cp -a LICENSE "$OUT_DIR/addons/speexdsp/"

rm -f "$ZIP"
( cd "$OUT_DIR" && zip -qr "$ZIP" addons )
echo "wrote $ZIP"
unzip -l "$ZIP" | head -20
