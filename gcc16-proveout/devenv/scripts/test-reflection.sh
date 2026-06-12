#!/usr/bin/env bash
# Run the C++26 reflection slice of the GCC testsuite against a built tree,
# or compile a single repro with the freshly built compiler.
#
# usage:
#   test-reflection.sh [master|gcc-16]                 # the reflection testsuite
#   test-reflection.sh [master|gcc-16] <file.cpp> ...  # one-off repro compile
#
# The reflection tests live under gcc/testsuite/g++.dg/cpp26/ (reflection*,
# annotations*, splice*, expansion-stmt*...). RUNTESTFLAGS filters dg.exp
# to just those files.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

BRANCH="${1:-master}"; shift || true
BUILD="build-$BRANCH"
[ -x "$BUILD/gcc/xg++" ] || { echo "run scripts/build.sh $BRANCH first" >&2; exit 1; }

if [ "$#" -gt 0 ]; then
  # One-off repro: the in-tree driver needs -B to find cc1plus + its libs.
  exec "$BUILD/gcc/xg++" -B"$BUILD/gcc/" -std=c++26 -freflection "$@"
fi

make -C "$BUILD/gcc" check-c++ \
  RUNTESTFLAGS="dg.exp=cpp26/reflection* cpp26/annotations* cpp26/splice* cpp26/expansion-stmt* cpp26/consteval-prop*" \
  -j"$(nproc)" || true
echo
echo "Summary:"
grep -E '^(# of|FAIL|XPASS|UNRESOLVED)' "$BUILD/gcc/testsuite/g++/g++.sum" | head -40
