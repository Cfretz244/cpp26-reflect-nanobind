dedup_key: taskflow-v4-missing-bit-include

# taskflow v4.0.0 uses std::bit_ceil / std::bit_width without #include <bit>

Layer: LIB (library-side, taskflow)

## Summary

Taskflow v4.0.0's headers call `std::bit_ceil` and `std::bit_width` (both `<bit>`
facilities) but never `#include <bit>`. On stdlibs that pull `<bit>` in transitively
through some other header the umbrella `<taskflow/taskflow.hpp>` happens to compile; on
this repo's from-source libc++ it does not, so the header fails with:

```
libs/taskflow/taskflow/core/wsq.hpp:105:36: error: no member named 'bit_ceil' in namespace 'std'
libs/taskflow/taskflow/core/executor.hpp:1223:19: error: no member named 'bit_width' in namespace 'std'
```

These are genuine compile ERRORS (the rest of the output is unrelated
`is_trivial` deprecation warnings).

## Smallest trigger

```cpp
#include <taskflow/taskflow.hpp>   // -std=c++26 -stdlib=libc++ (from-source libc++)
int main() {}
```

Fails to compile. Prepending `#include <bit>` makes it compile cleanly.

## Call sites

- `taskflow/core/wsq.hpp:105`  -> `std::bit_ceil(N)`  (bounded task-queue sizing)
- `taskflow/core/executor.hpp:1223` -> `std::bit_width(...)` (`_buffers(std::bit_width(N))`)

Neither `wsq.hpp` nor `executor.hpp` (nor any header they include) has `#include <bit>`.

## Fix (upstream, library-side)

Add `#include <bit>` to `taskflow/core/wsq.hpp` and `taskflow/core/executor.hpp` (the TUs
that use the facilities). This is a missing-include bug, not a toolchain or binder bug.

## Run-side workaround

The taskflow run's fixture header (binding/tftest.h) does `#include <bit>` before
`#include <taskflow/taskflow.hpp>`; the binding and native oracle both include tftest.h
first, so both sides see the same well-formed headers without editing the library source.
