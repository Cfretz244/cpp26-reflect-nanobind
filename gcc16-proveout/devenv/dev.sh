#!/usr/bin/env bash
# Enter (or run a command in) the GCC development container. The umbrella
# repo mounts at /work; the working directory is this devenv dir, so the
# scripts/ paths work as-is and the GCC source/build trees (git-ignored,
# under devenv/) are shared with the host.
#
#   gcc16-proveout/devenv/dev.sh                      # interactive shell
#   gcc16-proveout/devenv/dev.sh bash scripts/build.sh
#
# Build the image first:  docker build -t gcc-devenv gcc16-proveout/devenv
set -euo pipefail
_here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$_here/../.." && pwd)"
TTY_FLAGS=""
if [ -t 0 ] && [ -t 1 ]; then TTY_FLAGS="-it"; fi
exec docker run --rm $TTY_FLAGS -v "$REPO":/work \
  -w /work/gcc16-proveout/devenv gcc-devenv "${@:-bash}"
