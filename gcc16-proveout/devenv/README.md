# GCC development environment (Phase 4: minimize + file + fix upstream)

## State as of 2026-06-12 (handoff)

Everything below is DONE and on disk; a fresh agent can start at "The loop".

- **Image**: `gcc-devenv` is built locally (gcc:16 base + build prereqs +
  DejaGnu + gdb + ccache). Rebuild anytime: `docker build -t gcc-devenv
  gcc16-proveout/devenv`.
- **Checkout**: `devenv/gcc/` is a blob-lazy clone of gcc.gnu.org/git/gcc.git
  with `master` checked out and `releases/gcc-16` tracked locally.
- **Built + installed**: `build-master/` is configured
  (c,c++; --disable-bootstrap; --enable-checking=yes; ccache) and fully
  built; `make install` has run, so the USABLE compiler is
  `install-master/bin/g++` (= 17.0.0 @ commit 7ce3a7b1beb). Set
  `LD_LIBRARY_PATH=$PWD/install-master/lib64` when running its output.
  NOTE: the bare in-tree `build-master/gcc/xg++ -B...` does NOT find
  libstdc++'s `<meta>` — use the installed driver for repros (or the
  testsuite runner, which wires its own paths).
- **No gcc-16 tree yet**: `scripts/configure.sh gcc-16 && scripts/build.sh
  gcc-16` when a release-branch build is needed (backport verification).
- **Probe matrix vs trunk is recorded** in ../UPSTREAM_RESEARCH.md
  ("Trunk verification matrix"): GCC-5 FIXED on trunk (next action =
  bisect the fixing commit here, then check releases/gcc-16 for it);
  GCC-1/2/6 reproduce (file); GCC-8 reproduces and trunk's checking
  build upgrades it to a same_comdat_group symtab verify (file with both
  signatures); GCC-3 + GCC-4-family = file as behavior questions;
  08_ann_spec's trunk break is GCC 17 API finalization, not a bug.
- **Suggested first task for the next agent**: `git bisect` the GCC-5 fix
  on master (good = 16.1 release tag `releases/gcc-16.1.0`, bad...
  inverted: ICE present at 16.1, gone at trunk — bisect with
  `gcc16-proveout/probes/xfail_gcc5_deferred_noexcept_partial_spec.cpp`
  as the test, rebuilding cc1plus only per step:
  `make -C build-master/gcc cc1plus` is the cycle), then check whether
  that commit is on `releases/gcc-16`; if not, comment on the relevant
  PR asking for backport. Then file GCC-8 (strongest case), then
  GCC-1+6 together, then the GCC-2/3/4 questions — citations and CC
  list in ../UPSTREAM_RESEARCH.md.

A dockerized GCC-from-source dev setup for working the reflection findings
(corpus/findings/GCC-000*.md, probes in ../probes/) into upstream bug
reports and candidate patches. The container holds only the toolchain and
testsuite dependencies; the GCC checkout and build trees live HERE on the
mounted repo (git-ignored), so they persist and are editable from the host.

## One-time setup

```bash
docker build -t gcc-devenv gcc16-proveout/devenv
gcc16-proveout/devenv/dev.sh bash scripts/clone.sh          # blob-lazy clone
gcc16-proveout/devenv/dev.sh bash scripts/configure.sh      # master tree
gcc16-proveout/devenv/dev.sh bash scripts/build.sh          # ~30-60 min
```

## The loop

```bash
gcc16-proveout/devenv/dev.sh                                 # shell in container
# reproduce against trunk:
bash scripts/test-reflection.sh master ../probes/xfail_gcc8_member_template_nttp_mangling.cpp
# hack on gcc/cp/*.cc (module*.cc / pt.cc / mangle.cc / except.cc / reflect*.cc), then:
bash scripts/build.sh master cc1plus                         # 1-3 min rebuild
bash scripts/test-reflection.sh master                       # reflection testsuite slice
```

- `build-master/gcc/xg++ -B build-master/gcc/` is the in-tree driver.
- `configure.sh gcc-16` / `build.sh gcc-16` give a second tree on
  `releases/gcc-16` for verifying what 16.1/16.2 actually do.
- ccache is wired in (`.ccache/` here), so branch flips rebuild fast.

## Filing (mirror of the TC-XXXX discipline)

1. Reproduce the probe on **master** first — if trunk already fixed it,
   note the commit in the finding and (if we need it) test the backport on
   the gcc-16 tree instead of filing.
2. File at gcc.gnu.org/bugzilla, component **c++**; attach the probe
   verbatim (they want self-contained, no includes if avoidable —
   `<meta>` is unavoidable here), the `xg++ -std=c++26 -freflection`
   command, expected-vs-actual, and the clang-p2996 cross-check.
   See ../UPSTREAM_RESEARCH.md for per-finding citations (113108/125630,
   123237, 123379, 125179) and the implementer CC list.
3. Record the filed number in corpus/findings/GCC-000N.md and a
   per-finding UPSTREAM.md (the umbrella working agreement).
4. Candidate fixes: patch on master, run the reflection slice plus
   `make -C build-master/gcc check-c++` for the touched area, and follow
   gcc/CONTRIBUTING + the MAINTAINERS file for gcc-patches submission
   (DCO sign-off, ChangeLog skeleton via contrib/mklog.py).

## Layout (everything below is git-ignored)

```
devenv/gcc/             the checkout (master + releases/gcc-16)
devenv/build-master/    configured build tree for trunk
devenv/build-gcc-16/    configured build tree for the release branch
devenv/install-*/       --prefix targets (only used by `make install`)
devenv/.ccache/         compiler cache for fast cc1plus cycles
```
