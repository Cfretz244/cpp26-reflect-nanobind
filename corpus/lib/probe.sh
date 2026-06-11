#!/usr/bin/env bash
# Gate 1: header-compile probe. -fsyntax-only a TU that only #includes the library's
# public headers under the exact toolchain flags. Captures full stderr.
#
# usage: probe.sh <probe.cpp> [-I extra_include ...]
# exit 0 => Gate 1 pass; nonzero => taxonomy A (won't compile under toolchain).
set -uo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

probe_src="$1"; shift
if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  exec "$CORPUS_CXX" $REFLECT_FLAGS \
    -I "$PYINC" -I "$NBINC" "$@" \
    -fsyntax-only "$probe_src"
fi
"$CORPUS_CXX" $REFLECT_FLAGS $ISYSROOT_FLAGS \
  -nostdinc++ -isystem "$TC/include/c++/v1" \
  -I "$PYINC" -I "$NBINC" "$@" \
  -fsyntax-only "$probe_src"
