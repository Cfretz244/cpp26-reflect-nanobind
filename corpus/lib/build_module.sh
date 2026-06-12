#!/usr/bin/env bash
# Gate 4 (single-stage): compile + link one binding .cpp into an importable Python
# extension module, reusing the prebuilt libnanobind-static.a. On the clang lane
# this mirrors the exact compile/link recipe emitted by the nanobind test CMake
# (see build/build.ninja); the gcc16 lane is the plain-Linux equivalent
# (g++ -freflection, -shared, one libstdc++ runtime).
#
# usage: build_module.sh <binding.cpp> <module_name> <out_dir> [-I extra_include ...]
#   produces <out_dir>/<module_name><EXT_SUFFIX>
# exit 0 => Gate 4 pass; nonzero => taxonomy B (binding fails to compile/link).
#
# Non-header-only libraries, two paths:
#   - NB_EXTRA_SOURCES: space-separated library .cc files compiled (plain C++26, no reflection) and
#     linked in. The minimal path for a one-or-two-symbol closure (e.g. Abseil throw_delegate.cc).
#   - NB_EXTRA_LIBS: extra linker flags appended to the link line (e.g. "-L <prefix>/lib
#     -labsl_merged" for a prebuilt static library). The path for a deep link closure.
set -uo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

src="$1"; mod="$2"; out_dir="$3"; shift 3
mkdir -p "$out_dir"
obj="$out_dir/$mod.o"
so="$out_dir/$mod$EXT_SUFFIX"

if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  # --- compile (g++ with reflection) ---
  "$CORPUS_CXX" \
    -O2 -DNDEBUG -fPIC -fvisibility=hidden \
    $REFLECT_FLAGS \
    -DNB_ABORT_ON_LEAK \
    -I "$PYINC" -I "$NBINC" -I "$MBINC" "$@" \
    -c "$src" -o "$obj" || { echo "BUILD_FAIL_STAGE=compile" >&2; exit 11; }

  # --- extra library sources (plain C++, no reflection flags) ---
  extra_objs=()
  if [ -n "${NB_EXTRA_SOURCES:-}" ]; then
    i=0
    for esrc in $NB_EXTRA_SOURCES; do
      eobj="$out_dir/extra_${i}.o"
      "$CORPUS_CXX" \
        -std=c++26 -O2 -DNDEBUG -fPIC -fvisibility=hidden \
        -I "$PYINC" -I "$NBINC" -I "$MBINC" "$@" \
        -c "$esrc" -o "$eobj" || { echo "BUILD_FAIL_STAGE=extra_compile" >&2; exit 12; }
      extra_objs+=("$eobj")
      i=$((i + 1))
    done
  fi

  # --- link (-shared; undefined CPython symbols resolve at import) ---
  "$CORPUS_CXX" \
    -shared -fvisibility=hidden -O2 \
    "$obj" ${extra_objs[@]+"${extra_objs[@]}"} "$NBLIB" ${NB_EXTRA_LIBS:-} \
    -o "$so" || { echo "BUILD_FAIL_STAGE=link" >&2; exit 13; }

  echo "$so"
  exit 0
fi

# --- compile ---
"$CORPUS_CXX" \
  -O3 -DNDEBUG -arch arm64 $ISYSROOT_FLAGS \
  -fPIC -fvisibility=hidden -fno-stack-protector -Os \
  $REFLECT_FLAGS \
  -DNB_ABORT_ON_LEAK \
  -I "$PYINC" -I "$NBINC" -I "$MBINC" "$@" \
  -c "$src" -o "$obj" || { echo "BUILD_FAIL_STAGE=compile" >&2; exit 11; }

# --- compile extra library sources (NB_EXTRA_SOURCES), if any ---
# These keep the -I include flags from "$@" but drop -freflection-latest: they are ordinary
# library translation units, not binder code.
extra_objs=()
if [ -n "${NB_EXTRA_SOURCES:-}" ]; then
  i=0
  for esrc in $NB_EXTRA_SOURCES; do
    eobj="$out_dir/extra_${i}.o"
    "$CORPUS_CXX" \
      -std=c++26 -stdlib=libc++ -O2 -DNDEBUG -arch arm64 $ISYSROOT_FLAGS \
      -nostdinc++ -isystem "$TC/include/c++/v1" \
      -fPIC -fvisibility=hidden \
      -I "$PYINC" -I "$NBINC" -I "$MBINC" "$@" \
      -c "$esrc" -o "$eobj" || { echo "BUILD_FAIL_STAGE=extra_compile" >&2; exit 12; }
    extra_objs+=("$eobj")
    i=$((i + 1))
  done
fi

# --- link ---  (-isysroot/-arch resolve libSystem; -L$TC/lib resolves libc++)
"$CORPUS_CXX" \
  -bundle -Wl,-headerpad_max_install_names \
  -arch arm64 $ISYSROOT_FLAGS \
  -L "$TC/lib" -Wl,-rpath,"$TC/lib" -Wl,@"$NBSYM" \
  -Wl,-dead_strip $REFLECT_FLAGS -O3 -DNDEBUG \
  "$obj" ${extra_objs[@]+"${extra_objs[@]}"} "$NBLIB" ${NB_EXTRA_LIBS:-} \
  -o "$so" || { echo "BUILD_FAIL_STAGE=link" >&2; exit 13; }

echo "$so"
