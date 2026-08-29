#!/usr/bin/env python
"""Build SpeexDSP GDExtension (resampler first; Speex sources untouched).

  git submodule update --init --recursive
  scons platform=linux target=template_debug
"""
import os
import sys

from SCons.Script import Default, Dir, SConscript

godot_cpp = Dir("godot-cpp")
if not godot_cpp.exists() or not os.listdir(str(godot_cpp.srcnode())):
    print("godot-cpp missing. Run: git submodule update --init --recursive", file=sys.stderr)
    sys.exit(1)

speex = Dir("thirdparty/speexdsp")
if not speex.exists() or not os.listdir(str(speex.srcnode())):
    print("speexdsp missing. Run: git submodule update --init --recursive", file=sys.stderr)
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct")

# SpeexDSP resampler only — OUTSIDE_SPEEX keeps library sources unmodified.
speex_c = ["thirdparty/speexdsp/libspeexdsp/resample.c"]
speex_inc = [
    "thirdparty/speexdsp/include/speex",
    "thirdparty/speexdsp/libspeexdsp",
    "thirdparty/speexdsp_glue",
]
speex_defs = [
    "OUTSIDE_SPEEX",
    "RANDOM_PREFIX=godot_speexdsp",
    "FLOATING_POINT",
    "EXPORT=",
]

env_c = env.Clone()
env_c.Append(
    CCFLAGS=["-std=c99", "-Wall", "-Wextra", "-fPIC", "-O2"],
    CPPPATH=speex_inc,
    CPPDEFINES=speex_defs,
)
# Speex upstream has intentional style; do not -Werror their tree.
speex_lib = env_c.StaticLibrary("build/libspeexdsp_resample", speex_c)

sources = [
    "src/SpeexResampler.cpp",
    "src/register_types.cpp",
]

env_cpp = env.Clone()
env_cpp.Append(
    CXXFLAGS=[
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-Wno-unused-but-set-parameter",
        "-fPIC",
    ],
    CPPPATH=["src"] + speex_inc,
    CPPDEFINES=speex_defs,
)
env_cpp.Append(LIBS=[speex_lib, "m"])

addon_bin = "addons/speexdsp/bin"
os.makedirs(addon_bin, exist_ok=True)

lib = env_cpp.SharedLibrary(
    f"{addon_bin}/libspeexdsp{env['suffix']}{env['SHLIBSUFFIX']}",
    source=sources,
)
Default(lib)
