#!/usr/bin/env python
"""Build SpeexDSP GDExtension (resampler + preprocess + echo; Speex sources untouched).

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

speex_root = "thirdparty/speexdsp"
# Upstream expects generated speexdsp_config_types.h next to speexdsp_types.h — do not edit submodule.
_config_types_dst = os.path.join(speex_root, "include", "speex", "speexdsp_config_types.h")
_config_types_src = os.path.join("thirdparty", "speexdsp_glue", "speexdsp_config_types.h")
if not os.path.isfile(_config_types_dst):
    import shutil

    shutil.copyfile(_config_types_src, _config_types_dst)
speex_inc = [
    f"{speex_root}/include",
    f"{speex_root}/include/speex",
    f"{speex_root}/libspeexdsp",
    "thirdparty/speexdsp_glue",
]

resample_c = [f"{speex_root}/libspeexdsp/resample.c"]
resample_defs = [
    "OUTSIDE_SPEEX",
    "RANDOM_PREFIX=godot_speexdsp",
    "FLOATING_POINT",
    "EXPORT=",
]

preprocess_c = [
    f"{speex_root}/libspeexdsp/preprocess.c",
    f"{speex_root}/libspeexdsp/mdf.c",
    f"{speex_root}/libspeexdsp/fftwrap.c",
    f"{speex_root}/libspeexdsp/filterbank.c",
    f"{speex_root}/libspeexdsp/kiss_fft.c",
    f"{speex_root}/libspeexdsp/kiss_fftr.c",
]
preprocess_defs = [
    "HAVE_CONFIG_H",
    "FLOATING_POINT",
    "USE_KISS_FFT",
    "EXPORT=",
]

env_rs = env.Clone()
env_rs.Append(
    CCFLAGS=["-std=c99", "-Wall", "-Wextra", "-fPIC", "-O2", "-Wno-sign-compare"],
    CPPPATH=speex_inc,
    CPPDEFINES=resample_defs,
)
lib_rs = env_rs.StaticLibrary("build/libspeex_resample", resample_c)

env_pp = env.Clone()
env_pp.Append(
    CCFLAGS=["-std=c99", "-Wall", "-Wextra", "-fPIC", "-O2", "-Wno-sign-compare"],
    CPPPATH=speex_inc,
    CPPDEFINES=preprocess_defs,
)
lib_pp = env_pp.StaticLibrary("build/libspeex_preprocess", preprocess_c)

cxx_common = [
    "-std=c++17",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-Wno-unused-parameter",
    "-Wno-unused-variable",
    "-Wno-unused-but-set-parameter",
    "-fPIC",
]

env_rs_cpp = env.Clone()
env_rs_cpp.Append(CXXFLAGS=cxx_common, CPPPATH=["src"] + speex_inc, CPPDEFINES=resample_defs)
obj_rs = env_rs_cpp.SharedObject("build/SpeexResampler", "src/SpeexResampler.cpp")

env_pp_cpp = env.Clone()
env_pp_cpp.Append(CXXFLAGS=cxx_common, CPPPATH=["src"] + speex_inc, CPPDEFINES=preprocess_defs)
obj_pp = env_pp_cpp.SharedObject("build/SpeexPreprocess", "src/SpeexPreprocess.cpp")
obj_echo = env_pp_cpp.SharedObject("build/SpeexEchoCanceller", "src/SpeexEchoCanceller.cpp")
obj_reg = env_pp_cpp.SharedObject("build/register_types", "src/register_types.cpp")

env_link = env.Clone()
env_link.Append(LIBS=[lib_rs, lib_pp, "m"])

addon_bin = "addons/speexdsp/bin"
os.makedirs(addon_bin, exist_ok=True)

lib = env_link.SharedLibrary(
    f"{addon_bin}/libspeexdsp{env['suffix']}{env['SHLIBSUFFIX']}",
    source=[obj_rs, obj_pp, obj_echo, obj_reg],
)
Default(lib)
