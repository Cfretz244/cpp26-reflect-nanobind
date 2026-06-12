#!/usr/bin/env bash
# Configure a GCC build tree for C++ frontend development.
#
# usage: configure.sh [master|gcc-16]      (default: master)
#
# One build tree per branch (build-master/, build-gcc-16/) so you can flip
# between trunk and the release branch without reconfiguring. Development
# configuration: C and C++ only, no bootstrap (the gcc:16 host compiler
# builds cc1plus directly -- the standard fast cycle for frontend work;
# do a bootstrapped build only for final patch validation), assertions ON
# (catches the ICE asserts we are chasing), no multilib.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

BRANCH="${1:-master}"
case "$BRANCH" in
  master) REF=master ;;
  gcc-16) REF=releases/gcc-16 ;;
  *) echo "usage: configure.sh [master|gcc-16]" >&2; exit 2 ;;
esac

[ -d gcc/.git ] || { echo "run scripts/clone.sh first" >&2; exit 1; }
git -C gcc checkout "$REF"

BUILD="build-$BRANCH"
mkdir -p "$BUILD"
cd "$BUILD"
../gcc/configure \
  --prefix="$PWD/../install-$BRANCH" \
  --enable-languages=c,c++ \
  --disable-bootstrap \
  --disable-multilib \
  --disable-libsanitizer \
  --enable-checking=yes \
  CC="ccache gcc" CXX="ccache g++"
echo
echo "Configured $BUILD (branch $REF). Next: bash scripts/build.sh $BRANCH"
