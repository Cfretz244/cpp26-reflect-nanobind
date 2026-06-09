#!/usr/bin/env bash
# Gate 4 (single-stage): compile + link one binding .cpp into an importable Python
# extension module, reusing the prebuilt libnanobind-static.a. Mirrors the exact
# compile/link recipe emitted by the nanobind test CMake (see build/build.ninja).
#
# usage: build_module.sh <binding.cpp> <module_name> <out_dir> [-I extra_include ...]
#   produces <out_dir>/<module_name><EXT_SUFFIX>
# exit 0 => Gate 4 pass; nonzero => taxonomy B (binding fails to compile/link).
set -uo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

src="$1"; mod="$2"; out_dir="$3"; shift 3
mkdir -p "$out_dir"
obj="$out_dir/$mod.o"
so="$out_dir/$mod$EXT_SUFFIX"

# --- compile ---
"$TC/bin/clang++" \
  -O3 -DNDEBUG -arch arm64 $ISYSROOT_FLAGS \
  -fPIC -fvisibility=hidden -fno-stack-protector -Os \
  $REFLECT_FLAGS \
  -DNB_ABORT_ON_LEAK \
  -I "$PYINC" -I "$NBINC" "$@" \
  -c "$src" -o "$obj" || { echo "BUILD_FAIL_STAGE=compile" >&2; exit 11; }

# --- link ---  (-isysroot/-arch resolve libSystem; -L$TC/lib resolves libc++)
"$TC/bin/clang++" \
  -bundle -Wl,-headerpad_max_install_names \
  -arch arm64 $ISYSROOT_FLAGS \
  -L "$TC/lib" -Wl,-rpath,"$TC/lib" -Wl,@"$NBSYM" \
  -Wl,-dead_strip $REFLECT_FLAGS -O3 -DNDEBUG \
  "$obj" "$NBLIB" \
  -o "$so" || { echo "BUILD_FAIL_STAGE=link" >&2; exit 13; }

echo "$so"
