# GCC development environment (Phase 4: minimize + file + fix upstream)

## State as of 2026-06-12 (Phase 4 COMPLETE)

The minimize/root-cause/fix pass over every GCC finding is DONE — final
per-finding dispositions in `../UPSTREAM_RESEARCH.md` ("FINAL DISPOSITION"
section). Summary: GCC-1/6, GCC-3, GCC-8 have verified fix patches on this
checkout's **`proveout-fixes` branch** (base master `7ce3a7b1beb`; the
built `build-master/` cc1plus has them); GCC-5 was fixed upstream
(PR 124628, backport already on releases/gcc-16 ⇒ 16.2); GCC-2 and
GCC-4/probe-09 reclassified as CONFORMING GCC behavior (do not file);
GCC-7 is root-caused (architectural — uncollectable constant-evaluation
garbage) with a report-ready write-up. Patches, repros, and ready-to-file
bugzilla material live in `corpus/findings/repros/GCC-000N/UPSTREAM.md`.
Nothing filed yet — filing is the user's call. The remaining devenv use is
filing support and 16.2 re-verification.

## Environment state (still true)

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
- **Probe matrix vs trunk + final dispositions are recorded** in
  ../UPSTREAM_RESEARCH.md ("Trunk verification matrix" + "FINAL
  DISPOSITION"). The GCC-5 bisect is DONE (PR 124628, trunk
  `05ea83ffd54`, backport `e1396e44961` on releases/gcc-16);
  08_ann_spec's trunk break is GCC 17 API finalization, not a bug.
- **Suggested next task**: FILE the prepared reports, strongest first:
  GCC-8 (wrong-code mangling, patch ready), GCC-1+6 together (patch
  ready), GCC-3 (patch ready), GCC-7 (performance/memory report) —
  material in each `corpus/findings/repros/GCC-000N/UPSTREAM.md`,
  citations + CC list in ../UPSTREAM_RESEARCH.md. After filing, follow
  gcc-patches submission for the three patches (DCO sign-off; ChangeLogs
  are in the patch files). On the 16.2 container bump: re-run the probe
  smoke (xfail_gcc5 should flip; retire the binder's nb_fn_type_of shim)
  and re-test the three GCC-0007-walled emit lanes.

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
