# GCC development environment for the reflection bug campaign (Phase 4).
#
# Provides everything needed to BUILD GCC FROM SOURCE and run its C++
# testsuite inside the container, with the source/build trees living on the
# mounted umbrella repo (so they survive containers and are editable from
# the host):
#
#   docker build -t gcc-devenv gcc16-proveout/devenv
#   gcc16-proveout/devenv/dev.sh bash scripts/clone.sh        # once (~15 min)
#   gcc16-proveout/devenv/dev.sh bash scripts/configure.sh    # once per tree
#   gcc16-proveout/devenv/dev.sh bash scripts/build.sh        # ~30-60 min
#   gcc16-proveout/devenv/dev.sh bash scripts/test-reflection.sh
#
# Base: the same gcc:16 image as the corpus container, so the bootstrap
# compiler is the released 16.1 — building trunk or releases/gcc-16 with it
# is the standard one-stage dev configuration (--disable-bootstrap).
FROM gcc:16

RUN apt-get update && apt-get install -y --no-install-recommends \
    # GCC build prerequisites
    libgmp-dev libmpfr-dev libmpc-dev libisl-dev \
    flex bison m4 texinfo gawk autogen \
    zlib1g-dev libzstd-dev \
    # testsuite + debugging + workflow
    dejagnu expect tcl gdb valgrind \
    python3 python3-venv git ccache time file rsync less vim-tiny \
    && rm -rf /var/lib/apt/lists/*

# ccache for fast cc1plus rebuild cycles while hacking on cp/.
ENV PATH=/usr/lib/ccache:$PATH \
    CCACHE_DIR=/work/gcc16-proveout/devenv/.ccache
