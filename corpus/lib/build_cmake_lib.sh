#!/usr/bin/env bash
# Generic prebuilt-static-library helper for compiled corpus libraries (the
# generalization of build_abseil.sh, added for the Phase 3 wave-2 compiled tier:
# yaml-cpp, leveldb, SQLiteCpp, Box2D ...).
#
# Builds a CMake library from corpus/libs/<slug> as STATIC libs with the repo
# clang-p2996 + from-source libc++ (same ABI as the C++26 binder modules; see
# build_abseil.sh's ABI note), installs under build/<slug>-install/, and merges
# every installed lib<*>.a into one lib<slug>_merged.a so a run's meta.toml can
# carry a single `extra_libs` entry:
#
#   extra_libs = "-L {repo}/build/<slug>-install/lib -l<slug>_merged"
#
# Usage: build_cmake_lib.sh [--prod] <slug> <cxx_standard> [extra cmake args...]
#        (pass NB_FORCE_REBUILD=1 to rebuild an existing archive)
#
# --prod: build with the PRODUCTION compiler (Apple Clang + system libc++)
# into build/<slug>-install-prod/ for the emit lane -- the toolchain-libc++
# archives must not be linked into a system-libc++ module (two-runtime/ABI
# hazard). run_gates.py rewrites a run's extra_libs to the -prod prefix for
# its emit lane mechanically.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

PROD=0
if [ "${1:-}" = "--prod" ]; then PROD=1; shift; fi
SLUG="$1"; STD="$2"; shift 2
SRC="$CORPUS_ROOT/libs/$SLUG"
if [ "$PROD" = "1" ]; then
  BUILD_DIR="$REPO_ROOT/build/$SLUG-build-prod"
  PREFIX="$REPO_ROOT/build/$SLUG-install-prod"
else
  BUILD_DIR="$REPO_ROOT/build/$SLUG-build"
  PREFIX="$REPO_ROOT/build/$SLUG-install"
fi
MERGED="$PREFIX/lib/lib${SLUG}_merged.a"

if [ "${NB_FORCE_REBUILD:-0}" != "1" ] && [ -f "$MERGED" ]; then
  echo "$SLUG already built: $MERGED (NB_FORCE_REBUILD=1 to rebuild)"
  exit 0
fi

if [ ! -f "$SRC/CMakeLists.txt" ]; then
  echo "$SLUG submodule not initialized at $SRC" >&2
  exit 1
fi

if [ "$PROD" = "1" ]; then
  # Production: Apple Clang, default (system) libc++, no toolchain paths.
  cmake -S "$SRC" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$PROD_CC" \
    -DCMAKE_CXX_COMPILER="$PROD_CXX" \
    -DCMAKE_CXX_STANDARD="$STD" \
    -DCMAKE_CXX_FLAGS="-isysroot $SDKROOT_PATH -fPIC" \
    -DCMAKE_C_FLAGS="-isysroot $SDKROOT_PATH -fPIC" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DBUILD_SHARED_LIBS=OFF \
    "$@"
else
  cmake -S "$SRC" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$TC/bin/clang" \
    -DCMAKE_CXX_COMPILER="$TC/bin/clang++" \
    -DCMAKE_CXX_STANDARD="$STD" \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++ -nostdinc++ -isystem $TC/include/c++/v1 -isysroot $SDKROOT_PATH -fPIC" \
    -DCMAKE_C_FLAGS="-isysroot $SDKROOT_PATH -fPIC" \
    -DCMAKE_EXE_LINKER_FLAGS="-L $TC/lib -Wl,-rpath,$TC/lib" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DBUILD_SHARED_LIBS=OFF \
    "$@"
fi

ninja -C "$BUILD_DIR"
cmake --install "$BUILD_DIR" >/dev/null

rm -f "$MERGED"
LIBS=$(find "$PREFIX/lib" -name 'lib*.a' ! -name "lib${SLUG}_merged.a")
if [ -z "$LIBS" ]; then
  echo "no static libs installed under $PREFIX/lib" >&2
  exit 1
fi
# shellcheck disable=SC2086
libtool -static -o "$MERGED" $LIBS

echo "built merged static lib: $MERGED"
ls -la "$MERGED"
