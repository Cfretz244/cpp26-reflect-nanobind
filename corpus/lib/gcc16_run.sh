#!/usr/bin/env bash
# Run a corpus command inside the gcc16-reflect container (the GCC 16 backend's
# home). The umbrella repo is mounted at /work and the working directory is
# /work/corpus, so run paths read exactly like on the host:
#
#   corpus/lib/gcc16_run.sh python3 lib/run_gates.py runs/glm
#   corpus/lib/gcc16_run.sh bash lib/build_nanobind_prod.sh
#   corpus/lib/gcc16_run.sh bash lib/build_cmake_lib.sh box2d 17
#
# First-time setup (idempotent; the venv must live under /work, a /tmp venv
# dies with the container):
#   corpus/lib/gcc16_run.sh bash -c 'python3 -m venv /work/gcc16-proveout/venv \
#     && /work/gcc16-proveout/venv/bin/pip -q install pytest'
#
# The image is built from gcc16-proveout/Dockerfile:
#   docker build -t gcc16-reflect gcc16-proveout
set -euo pipefail
_lib="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$_lib/../.." && pwd)"
exec docker run --rm -v "$REPO":/work -e CORPUS_TOOLCHAIN=gcc16 \
  -w /work/corpus gcc16-reflect "$@"
