// Gate 1 probe for Abseil (20250814.2). The bound subset is absl::InlinedVector<T,N>; the header
// compiles under clang-p2996 (the .cc-defined throw_delegate symbols it references are supplied at
// link time via meta.extra_sources — Abseil is not header-only).
#include "absl/container/inlined_vector.h"
