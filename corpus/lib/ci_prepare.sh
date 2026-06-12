#!/usr/bin/env bash
# CI prerequisite builder for one corpus run (gcc16 backend). Initializes the
# run's library submodule and builds the prebuilt static archives a compiled
# run links (idempotent: build_cmake_lib.sh / build_abseil.sh skip when the
# merged archive already exists). Used by the nanobind-reflect repo's corpus
# CI matrix; harmless to run locally inside the gcc16-reflect container.
#
# Usage: ci_prepare.sh <run-slug>
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."     # corpus/
SLUG="$1"

# slug -> corpus/libs submodule (fixtures need none; abseil themes share one)
case "$SLUG" in
  _fixture_*) LIB="" ;;
  abseil_*)   LIB="abseil" ;;
  *)          LIB="$SLUG" ;;
esac

if [ -n "$LIB" ]; then
  (cd .. && git submodule update --init --depth 1 "corpus/libs/$LIB")
fi

# Compiled runs: build the merged static archive the run's extra_libs links.
case "$SLUG" in
  abseil_*)  bash lib/build_abseil.sh ;;
  box2d)     bash lib/build_cmake_lib.sh box2d 17 \
                 -DBOX2D_BUILD_TESTBED=OFF -DBOX2D_BUILD_UNIT_TESTS=OFF ;;
  leveldb)   bash lib/build_cmake_lib.sh leveldb 17 \
                 -DLEVELDB_BUILD_BENCHMARKS=OFF -DLEVELDB_BUILD_TESTS=OFF ;;
  sqlitecpp) bash lib/build_cmake_lib.sh sqlitecpp 17 \
                 -DSQLITECPP_BUILD_EXAMPLES=OFF -DSQLITECPP_BUILD_TESTS=OFF ;;
  yamlcpp)   bash lib/build_cmake_lib.sh yamlcpp 17 \
                 -DYAML_CPP_BUILD_TESTS=OFF ;;
esac

echo "ci_prepare: $SLUG ready"
