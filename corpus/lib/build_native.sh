#!/usr/bin/env bash
# Build a plain native C++ oracle harness (no nanobind) with the SAME toolchain, then
# the caller runs it to emit ground-truth JSON for differential testing. Sharing one
# compiler + one library build means any divergence is attributable to the binding layer.
#
# usage: build_native.sh <oracle.cpp> <out_bin> [-I extra_include ...]
# Honors NB_EXTRA_LIBS (extra linker flags, e.g. a prebuilt absl: "-L <prefix>/lib -labsl_merged"),
# so the native oracle links the SAME real library the bound module does.
set -uo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

src="$1"; out="$2"; shift 2
mkdir -p "$(dirname "$out")"

if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  # Plain C++26, stock g++ -- no reflection flags (the oracle uses no reflection).
  exec "$CORPUS_CXX" -std=c++26 -O2 \
    "$@" "$src" ${NB_EXTRA_LIBS:-} -o "$out"
fi

# Plain C++26 + libc++ — NO -freflection-latest (the oracle uses no reflection).
# -O2 is REQUIRED, not cosmetic: at -O0 this toolchain emits an *undefined weak* reference to
# `operator new[](size_t, std::__type_descriptor_t)` for any aggregate-with-method usage, and no
# toolchain library defines that symbol, so the executable aborts at load (dyld symbol-not-found).
# -O2 optimizes the spurious reference away. (Recorded as a toolchain-track finding; the nanobind
# modules dodge it only because they build at -O3.) See corpus/findings/.
exec "$CORPUS_CXX" -std=c++26 -stdlib=libc++ $ISYSROOT_FLAGS \
  -nostdinc++ -isystem "$TC/include/c++/v1" \
  -arch arm64 -O2 -L "$TC/lib" -Wl,-rpath,"$TC/lib" \
  "$@" "$src" ${NB_EXTRA_LIBS:-} -o "$out"
