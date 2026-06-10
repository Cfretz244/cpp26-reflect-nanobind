# Shared environment for the binding-test corpus. Source this (works in bash and zsh).
# Resolves the umbrella repo root from this file's location so it is path-portable.
if [ -n "${BASH_SOURCE:-}" ]; then _corpus_self="${BASH_SOURCE[0]}"; else _corpus_self="${(%):-%x}"; fi
_corpus_lib_dir="$(cd "$(dirname "$_corpus_self")" && pwd)"
export CORPUS_ROOT="$(cd "$_corpus_lib_dir/.." && pwd)"
export REPO_ROOT="$(cd "$CORPUS_ROOT/.." && pwd)"

export TC="$REPO_ROOT/toolchain"                       # the repo-local clang-p2996
export NBINC="$REPO_ROOT/nanobind/include"             # nanobind headers
export NBLIB="$REPO_ROOT/build/tests/libnanobind-static.a"   # prebuilt support lib
export NBSYM="$REPO_ROOT/nanobind/cmake/darwin-ld-cpython.sym"
export VENV_PY="$REPO_ROOT/.venv/bin/python"

export SDKROOT_PATH="$(xcrun --show-sdk-path)"

# Prefix of the prebuilt Abseil static lib (built by build_abseil.sh; git-ignored under build/).
# run_gates.py turns a meta.toml `link_abseil = true` into NB_EXTRA_LIBS pointing here.
export NB_ABSEIL_PREFIX="${NB_ABSEIL_PREFIX:-$REPO_ROOT/build/abseil-install}"
export PYINC="$("$VENV_PY" -c 'import sysconfig; print(sysconfig.get_path("include"))')"
export EXT_SUFFIX="$("$VENV_PY" -c 'import sysconfig; print(sysconfig.get_config_var("EXT_SUFFIX"))')"

# The mandatory reflection + libc++ flags (see CLAUDE.md). Used by probe, compile, link.
export REFLECT_FLAGS="-std=c++26 -freflection-latest -fentity-proxy-reflection -stdlib=libc++"
export ISYSROOT_FLAGS="-isysroot $SDKROOT_PATH"

# Toolchain commit + binder commit, recorded into every result.json for the feedback loop.
export TOOLCHAIN_COMMIT="$(git -C "$REPO_ROOT" rev-parse --short HEAD:llvm-project 2>/dev/null || echo unknown)"
export BINDER_COMMIT="$(git -C "$REPO_ROOT" rev-parse --short HEAD:nanobind 2>/dev/null || echo unknown)"

# Runtime env to import a built module.
corpus_run_python() {  # usage: corpus_run_python <build_dir> -c "import mod"
  local build_dir="$1"; shift
  DYLD_LIBRARY_PATH="$TC/lib" PYTHONPATH="$build_dir" "$VENV_PY" "$@"
}
