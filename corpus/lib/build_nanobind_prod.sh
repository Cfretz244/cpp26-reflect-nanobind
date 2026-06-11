#!/usr/bin/env bash
# One-time: build nanobind's support library with the PRODUCTION compiler
# (Apple Clang, system libc++) for the emit lane. Follows the single-TU
# recipe documented in nanobind/src/nb_combined.cpp -- no CMake tree (a
# nanobind configure with NB_TEST=ON would try to compile the reflection
# tests with a non-P2996 compiler, and NB_TEST=OFF builds no lib target).
# Flags mirror the constexpr lane's prebuilt $NBLIB (see build/build.ninja):
# same -DNB_ABORT_ON_LEAK/-DNB_COMPACT_ASSERTIONS so module behavior matches.
#
# usage: build_nanobind_prod.sh [--force]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

out_dir="$REPO_ROOT/build/prod"
lib="$out_dir/libnanobind-static.a"
if [ -f "$lib" ] && [ "${1:-}" != "--force" ]; then
  echo "$lib (cached; --force to rebuild)"
  exit 0
fi
mkdir -p "$out_dir"
obj="$out_dir/nb_combined.o"

"$PROD_CXX" -std=c++17 -O3 -DNDEBUG -arch arm64 -isysroot "$SDKROOT_PATH" \
  -fPIC -fvisibility=hidden -fno-strict-aliasing \
  -DNB_COMPACT_ASSERTIONS -DNB_ABORT_ON_LEAK \
  -I "$PYINC" -I "$NBINC" -I "$REPO_ROOT/nanobind/ext/robin_map/include" \
  -c "$REPO_ROOT/nanobind/src/nb_combined.cpp" -o "$obj"

libtool -static -o "$lib" "$obj"
echo "$lib"
