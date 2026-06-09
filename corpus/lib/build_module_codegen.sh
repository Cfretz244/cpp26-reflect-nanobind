#!/usr/bin/env bash
# Gate 4 (two-stage codegen): build the generator program, run it to emit the
# trampoline+STL-include header, then compile+link the binding module that includes it.
# Needed when bound classes have overridable virtuals (Python overrides C++ virtuals).
#
# usage: build_module_codegen.sh <gen.cpp> <binding.cpp> <module_name> <out_dir> [-I extra ...]
#   emits <out_dir>/trampolines.gen.h, then <out_dir>/<module_name><EXT_SUFFIX>
# exit codes: 21 gen-compile (B.gen), 22 gen-run, 11 module-compile, 13 link (B.module/B.link)
set -uo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

gen_src="$1"; bind_src="$2"; mod="$3"; out_dir="$4"; shift 4
mkdir -p "$out_dir"
gen_bin="$out_dir/gen"
gen_hdr="$out_dir/trampolines.gen.h"

# --- stage 1: build the generator (uses reflection to inspect the types) ---
"$TC/bin/clang++" $REFLECT_FLAGS $ISYSROOT_FLAGS \
  -nostdinc++ -isystem "$TC/include/c++/v1" \
  -I "$PYINC" -I "$NBINC" "$@" \
  "$gen_src" -o "$gen_bin" || { echo "BUILD_FAIL_STAGE=gen_compile" >&2; exit 21; }

# --- stage 1b: run the generator to emit the trampoline header ---
"$gen_bin" "$gen_hdr" || { echo "BUILD_FAIL_STAGE=gen_run" >&2; exit 22; }

# --- stage 2: build the binding module (includes the generated header) ---
exec "$(dirname "${BASH_SOURCE[0]}")/build_module.sh" "$bind_src" "$mod" "$out_dir" -I "$out_dir" "$@"
