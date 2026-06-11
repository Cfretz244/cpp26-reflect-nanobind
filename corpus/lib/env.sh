# Shared environment for the binding-test corpus. Source this (works in bash and zsh).
# Resolves the umbrella repo root from this file's location so it is path-portable.
#
# TWO BACKENDS, selected by $CORPUS_TOOLCHAIN (auto-detected when unset):
#   clang-p2996  -- the original macOS lane: repo-local clang-p2996 fork
#                   (./toolchain) builds the reflection TUs; Apple Clang +
#                   system libc++ is the emit lane's production compiler.
#   gcc16        -- the GCC 16 lane (PRIMARY since the 2026-06 re-home): runs
#                   on Linux inside the gcc16-reflect container (the umbrella
#                   repo mounted at /work). ONE stock compiler serves both
#                   lanes: g++ -freflection builds the reflection TUs and
#                   plain g++ (no reflection flags) is the production
#                   compiler. GCC build state lives under the git-ignored
#                   $REPO_ROOT/build-gcc16/ so the macOS artifacts survive.
if [ -n "${BASH_SOURCE:-}" ]; then _corpus_self="${BASH_SOURCE[0]}"; else _corpus_self="${(%):-%x}"; fi
_corpus_lib_dir="$(cd "$(dirname "$_corpus_self")" && pwd)"
export CORPUS_ROOT="$(cd "$_corpus_lib_dir/.." && pwd)"
export REPO_ROOT="$(cd "$CORPUS_ROOT/.." && pwd)"

if [ -z "${CORPUS_TOOLCHAIN:-}" ]; then
  if [ "$(uname -s)" = "Linux" ]; then CORPUS_TOOLCHAIN=gcc16; else CORPUS_TOOLCHAIN=clang-p2996; fi
fi
export CORPUS_TOOLCHAIN

export NBINC="$REPO_ROOT/nanobind/include"             # nanobind headers

if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
  # --- GCC 16 backend (Linux container) ---
  export GCC_BUILD_ROOT="$REPO_ROOT/build-gcc16"
  export CORPUS_CXX="g++"
  export VENV_PY="$REPO_ROOT/gcc16-proveout/venv/bin/python"
  # One g++-built nanobind support archive serves BOTH lanes (one compiler,
  # one libstdc++ runtime -- no two-runtime hazard). build_nanobind_prod.sh.
  export NBLIB="$GCC_BUILD_ROOT/prod/libnanobind-static.a"
  export NBLIB_PROD="$NBLIB"
  export NBSYM=""                                       # darwin-only export file
  export SDKROOT_PATH=""
  export ISYSROOT_FLAGS=""
  # -freflection is GCC 16's complete C++26 reflection switch (P2996 + P1306 +
  # P3096 + P3394 + P3491 + P3560); libstdc++ provides <meta>.
  export REFLECT_FLAGS="-std=c++26 -freflection"
  export NB_ABSEIL_PREFIX="${NB_ABSEIL_PREFIX:-$GCC_BUILD_ROOT/abseil-install}"
  export NB_ABSEIL_PREFIX_PROD="$NB_ABSEIL_PREFIX"      # same compiler, same archive
  export PROD_CXX="${PROD_CXX:-g++}"
  export PROD_CC="${PROD_CC:-gcc}"
  export TOOLCHAIN_COMMIT="gcc-$(g++ -dumpfullversion 2>/dev/null || echo unknown)"
else
  # --- clang-p2996 backend (macOS host; the original lane) ---
  export TC="$REPO_ROOT/toolchain"                      # the repo-local clang-p2996
  export CORPUS_CXX="$TC/bin/clang++"
  export VENV_PY="$REPO_ROOT/.venv/bin/python"
  export NBLIB="$REPO_ROOT/build/tests/libnanobind-static.a"   # prebuilt support lib
  export NBSYM="$REPO_ROOT/nanobind/cmake/darwin-ld-cpython.sym"
  export SDKROOT_PATH="$(xcrun --show-sdk-path)"
  export ISYSROOT_FLAGS="-isysroot $SDKROOT_PATH"
  # The mandatory reflection + libc++ flags (see CLAUDE.md). Used by probe,
  # compile, link. (-fentity-proxy-reflection is gone with the entity-proxy
  # feature -- P3687R1 deferred shadow-declaration reflection past C++26.)
  export REFLECT_FLAGS="-std=c++26 -freflection-latest -stdlib=libc++"
  # Prefix of the prebuilt Abseil static lib (built by build_abseil.sh; git-ignored under build/).
  # run_gates.py turns a meta.toml `link_abseil = true` into NB_EXTRA_LIBS pointing here.
  export NB_ABSEIL_PREFIX="${NB_ABSEIL_PREFIX:-$REPO_ROOT/build/abseil-install}"
  # Production toolchain (the emit lane's stage 2): Apple Clang + system libc++.
  export PROD_CXX="${PROD_CXX:-/usr/bin/clang++}"
  export PROD_CC="${PROD_CC:-/usr/bin/clang}"
  export NBLIB_PROD="$REPO_ROOT/build/prod/libnanobind-static.a"   # build_nanobind_prod.sh
  export NB_ABSEIL_PREFIX_PROD="${NB_ABSEIL_PREFIX_PROD:-$REPO_ROOT/build/abseil-install-prod}"
  export TOOLCHAIN_COMMIT="$(git -C "$REPO_ROOT" rev-parse --short HEAD:llvm-project 2>/dev/null || echo unknown)"
fi

export PYINC="$("$VENV_PY" -c 'import sysconfig; print(sysconfig.get_path("include"))')"
export EXT_SUFFIX="$("$VENV_PY" -c 'import sysconfig; print(sysconfig.get_config_var("EXT_SUFFIX"))')"
export PROD_STD_DEFAULT="c++20"

# Binder commit, recorded into every result for the feedback loop.
export BINDER_COMMIT="$(git -C "$REPO_ROOT" rev-parse --short HEAD:nanobind 2>/dev/null || echo unknown)"

# Runtime env to import a built module (constexpr lane).
corpus_run_python() {  # usage: corpus_run_python <build_dir> -c "import mod"
  local build_dir="$1"; shift
  if [ "$CORPUS_TOOLCHAIN" = "gcc16" ]; then
    PYTHONPATH="$build_dir" "$VENV_PY" "$@"
  else
    DYLD_LIBRARY_PATH="$TC/lib" PYTHONPATH="$build_dir" "$VENV_PY" "$@"
  fi
}

# Import a prod-built module. On macOS: NO DYLD_LIBRARY_PATH (dyld overrides by
# LEAF name, so exporting $TC/lib would load the toolchain's libc++.1.dylib into
# a module built against the system libc++ -- two-runtime corruption). On the
# gcc16 backend there is only one runtime; identical to corpus_run_python.
corpus_run_python_prod() {  # usage: corpus_run_python_prod <build_dir> -c "import mod"
  local build_dir="$1"; shift
  PYTHONPATH="$build_dir" "$VENV_PY" "$@"
}
