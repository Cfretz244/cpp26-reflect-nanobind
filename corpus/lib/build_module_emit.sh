#!/usr/bin/env bash
# Gate 4 (emit lane): two stages, two compilers.
#   stage 1 (P2996 toolchain): compile the emit GENERATOR (gen_emit.cpp, which
#     calls nb::emit_bindings<...> at constexpr time) and run it to render
#     <out_dir>/binding.gen.cpp -- the complete binding TU as plain C++.
#   stage 2 (PRODUCTION compiler, Apple Clang + system libc++): compile and
#     link the generated source into the module. No reflection toolchain, no
#     toolchain libc++, no $TC rpath anywhere in the artifact.
#
# usage: build_module_emit.sh <gen_emit.cpp> <module_name> <out_dir> [-I extra ...]
#   produces <out_dir>/binding.gen.cpp and <out_dir>/<module_name><EXT_SUFFIX>
# env:
#   NB_GEN_CFLAGS       stage-1 extras (the run's extra_cflags, incl. -fconstexpr-steps)
#   NB_PROD_STD         stage-2 -std (default $PROD_STD_DEFAULT)
#   NB_PROD_CFLAGS      stage-2 extras (extra_cflags minus reflection-only flags)
#   NB_EXTRA_SOURCES    library .cc files recompiled HERE by the prod compiler
#   NB_EXTRA_LIBS_PROD  prod link flags (e.g. -L .../<slug>-install-prod/lib -l<slug>_merged)
# exit codes: 31 emit_gen_compile, 32 emit_gen_run, 33 emit_compile,
#             34 emit_extra_compile, 35 emit_link
set -uo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

gen_src="$1"; mod="$2"; out_dir="$3"; shift 3
mkdir -p "$out_dir"
gen_bin="$out_dir/gen_emit"
gen_cpp="$out_dir/binding.gen.cpp"
obj="$out_dir/$mod.o"
so="$out_dir/$mod$EXT_SUFFIX"
# The run's binding/ dir holds binding_includes.h, which the generated TU's
# preamble re-includes -- both stages need it on the include path.
bind_dir="$(cd "$(dirname "$gen_src")" && pwd)"

[ -f "$NBLIB_PROD" ] || {
  echo "missing $NBLIB_PROD -- run corpus/lib/build_nanobind_prod.sh first" >&2
  exit 35
}

# --- stage 1: build the generator with the reflection toolchain ---
"$TC/bin/clang++" $REFLECT_FLAGS $ISYSROOT_FLAGS \
  -nostdinc++ -isystem "$TC/include/c++/v1" \
  -I "$PYINC" -I "$NBINC" -I "$bind_dir" "$@" ${NB_GEN_CFLAGS:-} \
  "$gen_src" -o "$gen_bin" \
  || { echo "BUILD_FAIL_STAGE=emit_gen_compile" >&2; exit 31; }

# --- stage 1b: run it to render the binding TU ---
DYLD_LIBRARY_PATH="$TC/lib" "$gen_bin" "$gen_cpp" \
  || { echo "BUILD_FAIL_STAGE=emit_gen_run" >&2; exit 32; }

# --- stage 2: compile the GENERATED source with the production compiler ---
"$PROD_CXX" \
  -std="${NB_PROD_STD:-$PROD_STD_DEFAULT}" -O2 -DNDEBUG -arch arm64 \
  -isysroot "$SDKROOT_PATH" \
  -fPIC -fvisibility=hidden \
  -DNB_ABORT_ON_LEAK \
  -I "$PYINC" -I "$NBINC" -I "$bind_dir" "$@" ${NB_PROD_CFLAGS:-} \
  -c "$gen_cpp" -o "$obj" \
  || { echo "BUILD_FAIL_STAGE=emit_compile" >&2; exit 33; }

# --- stage 2b: recompile any library sources with the production compiler ---
extra_objs=()
if [ -n "${NB_EXTRA_SOURCES:-}" ]; then
  i=0
  for esrc in $NB_EXTRA_SOURCES; do
    eobj="$out_dir/extra_${i}.o"
    "$PROD_CXX" \
      -std="${NB_PROD_STD:-$PROD_STD_DEFAULT}" -O2 -DNDEBUG -arch arm64 \
      -isysroot "$SDKROOT_PATH" \
      -fPIC -fvisibility=hidden \
      -I "$PYINC" -I "$NBINC" "$@" ${NB_PROD_CFLAGS:-} \
      -c "$esrc" -o "$eobj" \
      || { echo "BUILD_FAIL_STAGE=emit_extra_compile" >&2; exit 34; }
    extra_objs+=("$eobj")
    i=$((i + 1))
  done
fi

# --- link with the production compiler (system libc++; no $TC anywhere) ---
"$PROD_CXX" \
  -bundle -Wl,-headerpad_max_install_names \
  -arch arm64 -isysroot "$SDKROOT_PATH" \
  -Wl,@"$NBSYM" -Wl,-dead_strip -O2 \
  "$obj" ${extra_objs[@]+"${extra_objs[@]}"} "$NBLIB_PROD" ${NB_EXTRA_LIBS_PROD:-} \
  -o "$so" \
  || { echo "BUILD_FAIL_STAGE=emit_link" >&2; exit 35; }

echo "$so"
