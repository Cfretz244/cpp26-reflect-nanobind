# json toolchain-bug reproducer

See `corpus/findings/TC-0002-sloc-ice-heavy-reflection.md`. The reproducer is this run's
`../binding/gen.cpp` (+ `../binding/jsontest.h`) compiled against nlohmann/json v3.11.3 with a
raised constexpr step limit:

```
TC=<repo>/toolchain
$TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
  -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ -isystem $TC/include/c++/v1 \
  -fconstexpr-steps=200000000 \
  -I <repo>/corpus/libs/json/single_include \
  -I <repo>/corpus/runs/json/binding \
  -I <repo>/nanobind/include \
  -I "$(python -c 'import sysconfig;print(sysconfig.get_path("include"))')" \
  <repo>/corpus/runs/json/binding/gen.cpp -o /tmp/gen
# -> Assertion failed: "Invalid SLocOffset or bad function choice", SourceManager.cpp:876 (after ~32s)
```

At the default step limit the same compile instead reports a clean
`constexpr evaluation hit maximum step limit` (the standard-build B.gen_compile outcome).

TODO: minimize to a standalone TU (no nanobind/json) that exhausts SLoc space via repeated
`std::define_static_string` / `std::meta::members_of` in one consteval call.
