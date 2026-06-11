#!/usr/bin/env bash
# One-time: build nanobind's support library with the PRODUCTION compiler for
# the emit lane. Follows the single-TU recipe documented in
# nanobind/src/nb_combined.cpp -- no CMake tree (a nanobind configure with
# NB_TEST=ON would try to compile the reflection tests with a non-P2996
# compiler, and NB_TEST=OFF builds no lib target).
#
# clang-p2996 backend: Apple Clang + system libc++ into build/prod/. Flags
# mirror the constexpr lane's prebuilt $NBLIB (see build/build.ninja): same
# -DNB_ABORT_ON_LEAK/-DNB_COMPACT_ASSERTIONS so module behavior matches.
# gcc16 backend: stock g++ into build-gcc16/prod/. The SAME archive serves
# the constexpr lane's $NBLIB too (one compiler, one runtime).
#
# usage: build_nanobind_prod.sh [--force]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

lib="$NBLIB_PROD"
out_dir="$(dirname "$lib")"
if [ -f "$lib" ] && [ "${1:-}" != "--force" ]; then
  echo "$lib (cached; --force to rebuild)"
  exit 0
fi
mkdir -p "$out_dir"
obj="$out_dir/nb_combined.o"

if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  "$PROD_CXX" -std=c++17 -O3 -DNDEBUG \
    -fPIC -fvisibility=hidden -fno-strict-aliasing \
    -DNB_COMPACT_ASSERTIONS -DNB_ABORT_ON_LEAK \
    -I "$PYINC" -I "$NBINC" -I "$REPO_ROOT/nanobind/ext/robin_map/include" \
    -c "$REPO_ROOT/nanobind/src/nb_combined.cpp" -o "$obj"
  ar rcs "$lib" "$obj"
else
  "$PROD_CXX" -std=c++17 -O3 -DNDEBUG -arch arm64 -isysroot "$SDKROOT_PATH" \
    -fPIC -fvisibility=hidden -fno-strict-aliasing \
    -DNB_COMPACT_ASSERTIONS -DNB_ABORT_ON_LEAK \
    -I "$PYINC" -I "$NBINC" -I "$REPO_ROOT/nanobind/ext/robin_map/include" \
    -c "$REPO_ROOT/nanobind/src/nb_combined.cpp" -o "$obj"
  libtool -static -o "$lib" "$obj"
fi
echo "$lib"
