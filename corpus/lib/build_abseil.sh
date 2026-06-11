#!/usr/bin/env bash
# Build Abseil (corpus/libs/abseil submodule) as a STATIC library usable by the corpus binding
# modules + native oracles, then merge every libabsl_*.a into a single libabsl_merged.a.
#
# WHY a full build (not the per-.cc NB_EXTRA_SOURCES path): the marquee Abseil types
# (flat_hash_map/set, btree_*, Status, Cord, Time/Duration, int128) reference symbols across a deep
# closure of absl .cc units (raw_hash_set, hash, city, hashtablez_sampler, base, synchronization,
# throw_delegate, raw_logging_internal, ...). One merged archive + dead-stripping lets the linker
# pull exactly what each module uses.
#
# ABI (clang-p2996 backend): Abseil is built at C++17 (its supported standard) but with the SAME
# clang-p2996 compiler and the SAME from-source libc++ the C++26 binder modules use, so the C++17
# archive is ABI-compatible with the C++26 modules (one libc++ runtime, no ODR skew).
# gcc16 backend: one stock g++ + one libstdc++ for everything; --prod is accepted and ignored
# (the same archive serves both lanes); installs under build-gcc16/abseil-install/.
#
# Output (all under the git-ignored build tree; safe to delete and rebuild):
#   <build root>/abseil-build/            CMake build tree
#   <build root>/abseil-install/include/  installed headers (absl/*)
#   <build root>/abseil-install/lib/      libabsl_*.a + the merged libabsl_merged.a
#
# Idempotent: skips straight to success if libabsl_merged.a already exists (pass --force to rebuild).
# --prod (clang backend): build with the PRODUCTION compiler (Apple Clang + system libc++)
# into build/abseil-install-prod/ for the emit lane (run_gates.py rewrites a
# link_abseil run's link line to the -prod prefix mechanically).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

PROD=0
if [ "${1:-}" = "--prod" ]; then PROD=1; shift; fi
ABSL_SRC="$CORPUS_ROOT/libs/abseil"
if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  BUILD_DIR="$GCC_BUILD_ROOT/abseil-build"
  PREFIX="${NB_ABSEIL_PREFIX:-$GCC_BUILD_ROOT/abseil-install}"
elif [ "$PROD" = "1" ]; then
  BUILD_DIR="$REPO_ROOT/build/abseil-build-prod"
  PREFIX="${NB_ABSEIL_PREFIX_PROD:-$REPO_ROOT/build/abseil-install-prod}"
else
  BUILD_DIR="$REPO_ROOT/build/abseil-build"
  PREFIX="${NB_ABSEIL_PREFIX:-$REPO_ROOT/build/abseil-install}"
fi
MERGED="$PREFIX/lib/libabsl_merged.a"

if [ "${1:-}" != "--force" ] && [ -f "$MERGED" ]; then
  echo "abseil already built: $MERGED (pass --force to rebuild)"
  exit 0
fi

if [ ! -f "$ABSL_SRC/CMakeLists.txt" ]; then
  echo "abseil submodule not initialized at $ABSL_SRC" >&2
  exit 1
fi

# --- configure ---
if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  cmake -S "$ABSL_SRC" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER="$PROD_CXX" \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-fPIC" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DABSL_ENABLE_INSTALL=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DABSL_PROPAGATE_CXX_STD=ON \
    -DABSL_BUILD_TESTING=OFF
else
  if [ "$PROD" = "1" ]; then
    CXX_FOR_ABSL="$PROD_CXX"
    CXXFLAGS_FOR_ABSL="-isysroot $SDKROOT_PATH -fPIC"
  else
    CXX_FOR_ABSL="$TC/bin/clang++"
    CXXFLAGS_FOR_ABSL="-stdlib=libc++ -nostdinc++ -isystem $TC/include/c++/v1 -isysroot $SDKROOT_PATH -fPIC"
  fi
  cmake -S "$ABSL_SRC" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER="$CXX_FOR_ABSL" \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS_FOR_ABSL" \
    -DCMAKE_EXE_LINKER_FLAGS="-L $TC/lib -Wl,-rpath,$TC/lib" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DABSL_ENABLE_INSTALL=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DABSL_PROPAGATE_CXX_STD=ON \
    -DABSL_BUILD_TESTING=OFF
fi

# --- build + install ---
ninja -C "$BUILD_DIR"
cmake --install "$BUILD_DIR" >/dev/null

# --- merge every static lib into one archive ---
rm -f "$MERGED"
LIBS=$(find "$PREFIX/lib" -name 'libabsl_*.a' ! -name 'libabsl_merged.a')
if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  { echo "create $MERGED"
    for l in $LIBS; do echo "addlib $l"; done
    echo "save"; echo "end"; } | ar -M
else
  # shellcheck disable=SC2086
  libtool -static -o "$MERGED" $LIBS
fi

echo "built merged Abseil static lib: $MERGED"
ls -la "$MERGED"
