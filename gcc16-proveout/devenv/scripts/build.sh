#!/usr/bin/env bash
# Build GCC (or just the C++ compiler proper, for the inner dev loop).
#
# usage: build.sh [master|gcc-16] [cc1plus]
#   build.sh master          # full build of the configured tree (~30-60 min)
#   build.sh master cc1plus  # rebuild ONLY cc1plus after editing gcc/cp/*
#                            # (~1-3 min with ccache; the inner loop)
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

BRANCH="${1:-master}"
BUILD="build-$BRANCH"
[ -f "$BUILD/Makefile" ] || { echo "run scripts/configure.sh $BRANCH first" >&2; exit 1; }

JOBS="$(nproc)"
if [ "${2:-}" = "cc1plus" ]; then
  make -C "$BUILD/gcc" -j"$JOBS" cc1plus
  echo
  echo "cc1plus: $BUILD/gcc/cc1plus"
  echo "Drive it through the in-tree driver: $BUILD/gcc/xg++ -B$BUILD/gcc/"
else
  make -C "$BUILD" -j"$JOBS"
  echo
  echo "Built. In-tree compiler: $BUILD/gcc/xg++ -B$BUILD/gcc/"
fi
